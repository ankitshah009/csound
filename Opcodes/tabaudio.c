/*
    tabaudio.c:

    Copyright (C) 2018 John ffitch

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#include "csoundCore.h"
#include "interlocks.h"
#include "soundio.h"

typedef struct {
    OPDS    h;
    MYFLT   *kans;
    MYFLT   *itab;
    STRINGDAT *file;
    MYFLT   *format;
    MYFLT   *beg;
    MYFLT   *end;
    /* Local */
} TABAUDIO;

typedef struct {
    OPDS    h;
    MYFLT   *kans;
    MYFLT   *trig;
    MYFLT   *itab;
    STRINGDAT *file;
    MYFLT   *format;
    MYFLT   *sync;
    MYFLT   *beg;
    MYFLT   *end;
    /* Local */
} TABAUDIOK;

typedef struct {
    CSOUND *csound;
    MYFLT*   t;
    uint32_t size;
    SNDFILE* ff;
    MYFLT*   ans;
    void     *thread;
    OPDS     *h;
} SAVE_THREAD;

static const int32_t format_table[51] = {
    /* 0 - 9 */
    (AE_FLOAT | TYP2SF(TYP_RAW)), (AE_SHORT | TYP2SF(TYP_RAW)),
    AE_SHORT, AE_ULAW, AE_SHORT, AE_LONG,
    AE_FLOAT, AE_UNCH, AE_24INT, AE_DOUBLE,
    /* 10 - 19 */
    TYP2SF(TYP_WAV), (AE_CHAR | TYP2SF(TYP_WAV)),
    (AE_ALAW | TYP2SF(TYP_WAV)), (AE_ULAW | TYP2SF(TYP_WAV)),
    (AE_SHORT | TYP2SF(TYP_WAV)), (AE_LONG | TYP2SF(TYP_WAV)),
    (AE_FLOAT | TYP2SF(TYP_WAV)), (AE_UNCH | TYP2SF(TYP_WAV)),
    (AE_24INT | TYP2SF(TYP_WAV)), (AE_DOUBLE | TYP2SF(TYP_WAV)),
    /* 20 - 29 */
    TYP2SF(TYP_AIFF), (AE_CHAR | TYP2SF(TYP_AIFF)),
    (AE_ALAW | TYP2SF(TYP_AIFF)), (AE_ULAW | TYP2SF(TYP_AIFF)),
    (AE_SHORT | TYP2SF(TYP_AIFF)), (AE_LONG | TYP2SF(TYP_AIFF)),
    (AE_FLOAT | TYP2SF(TYP_AIFF)), (AE_UNCH | TYP2SF(TYP_AIFF)),
    (AE_24INT | TYP2SF(TYP_AIFF)), (AE_DOUBLE | TYP2SF(TYP_AIFF)),
    /* 30 - 39 */
    TYP2SF(TYP_RAW), (AE_CHAR | TYP2SF(TYP_RAW)),
    (AE_ALAW | TYP2SF(TYP_RAW)), (AE_ULAW | TYP2SF(TYP_RAW)),
    (AE_SHORT | TYP2SF(TYP_RAW)), (AE_LONG | TYP2SF(TYP_RAW)),
    (AE_FLOAT | TYP2SF(TYP_RAW)), (AE_UNCH | TYP2SF(TYP_RAW)),
    (AE_24INT | TYP2SF(TYP_RAW)), (AE_DOUBLE | TYP2SF(TYP_RAW)),
    /* 40 - 49 */
    TYP2SF(TYP_IRCAM), (AE_CHAR | TYP2SF(TYP_IRCAM)),
    (AE_ALAW | TYP2SF(TYP_IRCAM)), (AE_ULAW | TYP2SF(TYP_IRCAM)),
    (AE_SHORT | TYP2SF(TYP_IRCAM)), (AE_LONG | TYP2SF(TYP_IRCAM)),
    (AE_FLOAT | TYP2SF(TYP_IRCAM)), (AE_UNCH | TYP2SF(TYP_IRCAM)),
    (AE_24INT | TYP2SF(TYP_IRCAM)), (AE_DOUBLE | TYP2SF(TYP_IRCAM)),
    /* 50 */
    (TYP2SF(TYP_OGG) | AE_VORBIS)
};


static uintptr_t write_tab(void* pp)
{
    SAVE_THREAD *p = (SAVE_THREAD*)pp;
    MYFLT*   t = p->t;
    uint32_t size = p->size;
    SNDFILE* ff = p->ff;
    MYFLT*   ans = p->ans;
    CSOUND*  csound = p->csound;
    OPDS     *h = p->h;
    //free(pp);
    //printf("t=%p size=%d ff=%p\n", t, size, ff);
    if (sflib_writef_MYFLT(ff, t, size) != size) {
      sflib_close(ff);
      csound->PerfError(csound, h,
                           Str("tabaudio: failed to write data %d"),size);
      *ans = -FL(1.0);
    }
    else *ans = FL(1.0);
    sflib_close(ff);
    return 0;
}

int on_reset_audio(CSOUND *csound, void *pp)
{
    SAVE_THREAD *p =  (SAVE_THREAD *) pp;
    csound->JoinThread(p->thread);
    return 0;
}

static int32_t tabaudiok(CSOUND *csound, TABAUDIOK *p)
{
    if (*p->trig) {
      FUNC  *ftp;
      MYFLT *t;
      int32_t size, n;
      SNDFILE *ff;
      SFLIB_INFO sfinfo;
      int32_t  format = MYFLT2LRND(*p->format);
      int32_t  skip = MYFLT2LRND(*p->beg);
      int32_t  end = MYFLT2LRND(*p->end);

      if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->itab)) == NULL)) {
        return csound->PerfError(csound, &(p->h), Str("tabaudio: No table %g"), *p->itab);
      }
      *p->kans = FL(0.0);
      t = ftp->ftable + skip;
      size = ftp->flenfrms;
      if (end<=0) size -= skip;
      else size = end - skip;
      if (UNLIKELY(size<0 || size>ftp->flenfrms))
        return csound->PerfError(csound, &(p->h), Str("ftudio: ilegal size"));
      memset(&sfinfo, 0, sizeof(SFLIB_INFO));
      if (format >= 51)
        sfinfo.format = AE_SHORT | TYP2SF(TYP_RAW);
      else if (format < 0) {
        sfinfo.format = FORMAT2SF(csound->oparms->outformat);
        sfinfo.format |= TYPE2SF(csound->oparms->filetyp);
      }
      else sfinfo.format = format_table[format];
      if (!SF2FORMAT(sfinfo.format))
        sfinfo.format |= FORMAT2SF(csound->oparms->outformat);
      if (!SF2TYPE(sfinfo.format))
        sfinfo.format |= TYPE2SF(csound->oparms->filetyp);
      sfinfo.samplerate = (int32_t) MYFLT2LRND(CS_ESR);
      sfinfo.channels = ftp->nchanls;
      ff = sflib_open(p->file->data, SFM_WRITE, &sfinfo);
      if (ff==NULL)
        return csound->PerfError(csound, &(p->h),
                                 Str("tabaudio: failed to open file %s"),
                                 p->file->data);
      if (*p->sync==FL(0.0)) {  /* write in perf thread */
        if ((n=sflib_writef_MYFLT(ff, t, size)) != size) {
          printf("%s\n", sflib_strerror(ff));
          sflib_close(ff);
          return csound->PerfError(csound, &(p->h),
                                   Str("tabaudio: failed to write data %d %d"),
                                   n,size);
        }
        sflib_close(ff);
      }
      else {                    /* Use a helper thread */
        SAVE_THREAD *q = (SAVE_THREAD*)csound->Malloc(csound, sizeof(SAVE_THREAD));
        q->t = t;
        q->size = size;
        q->ff = ff;
        q->ans = p->kans;
        q->csound = csound;
        q->h = &(p->h);
        if ((q->thread = csound->CreateThread(write_tab, (void*)q))==NULL) {
          OPDS * i = q->h;
          free(q);
          return csound->PerfError(csound, i,
                                   Str("Error creating thread"));
        }
        csound->RegisterResetCallback(csound, (void*)q, on_reset_audio);
        /* if (fork() == 0) { */
        /*   ff = sflib_open(p->file->data, SFM_WRITE, &sfinfo); */
        /*   if (ff==NULL) { */
        /*     printf(Str("tabaudio: failed to open file %s"), p->file->data); */
        /*     exit(1); */
        /*   } */
        /*   if ((n=sflib_writef_MYFLT(ff, t, size)) != size) { */
        /*     sflib_close(ff); */
        /*     printf("%s %s", Str("tabaudio: failed to write data:"), */
        /*            sflib_strerror(ff)); */
        /*     exit(1); */
        /*   } */
        /*   sflib_close(ff); */
        /*   exit(0); */
        /* } */
      }
      *p->kans = FL(1.0);
    }
    else *p->kans = FL(0.0);
    return OK;
}

static int32_t tabaudioi(CSOUND *csound, TABAUDIO *p)
{
    FUNC  *ftp;
    MYFLT *t;
    int32_t size, n;
    SNDFILE *ff;
    SFLIB_INFO sfinfo;
    int32_t  format = MYFLT2LRND(*p->format);
    int32_t  skip = MYFLT2LRND(*p->beg);
    int32_t  end = MYFLT2LRND(*p->end);

    if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->itab)) == NULL)) {
      return csound->InitError(csound, Str("tabaudio: No table"));
    }
    *p->kans = FL(0.0);
    t = ftp->ftable + skip;
    size = ftp->flenfrms;
    if (end<=0) size -= skip;
    else size = end - skip;
    if (UNLIKELY(size<0 || size>ftp->flenfrms))
      return csound->InitError(csound, Str("ftudio: ilegal size"));
    memset(&sfinfo, 0, sizeof(SFLIB_INFO));
    if (format >= 51)
      sfinfo.format = AE_SHORT | TYP2SF(TYP_RAW);
    else if (format < 0) {
      sfinfo.format = FORMAT2SF(csound->oparms->outformat);
      sfinfo.format |= TYPE2SF(csound->oparms->filetyp);
    }
    else sfinfo.format = format_table[format];
    if (!SF2FORMAT(sfinfo.format))
      sfinfo.format |= FORMAT2SF(csound->oparms->outformat);
    if (!SF2TYPE(sfinfo.format))
      sfinfo.format |= TYPE2SF(csound->oparms->filetyp);
    sfinfo.samplerate = (int32_t) MYFLT2LRND(CS_ESR);
    sfinfo.channels = ftp->nchanls;

    ff = sflib_open(p->file->data, SFM_WRITE, &sfinfo);
    if (ff==NULL)
      return csound->InitError(csound, Str("tabaudio: failed to open file %s"),
                               p->file->data);
    if ((n=sflib_writef_MYFLT(ff, t, size)) != size) {
      printf("%s\n", sflib_strerror(ff));
      sflib_close(ff);
      return csound->InitError(csound, Str("tabaudio: failed to write data: %s"),
                               sflib_strerror(ff));
    }
    *p->kans = FL(1.0);
    sflib_close(ff);
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY tabaudio_localops[] =
  {
   { "ftaudio.i",     S(TABAUDIO),  TR, 1, "i", "iSioo",   (SUBR)tabaudioi, NULL },
   { "ftaudio.k",     S(TABAUDIOK), TR, 2, "k", "kkSkpOO",  NULL, (SUBR)tabaudiok },
  };

LINKAGE_BUILTIN(tabaudio_localops)



