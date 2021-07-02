<CsoundSynthesizer>
<CsOptions>
-m165 -d -o test_csscore_bin_python3.wav
</CsOptions>
<CsInstruments>

sr = 48000
ksmps = 128
nchnls = 2
nchnls_i = 1
0dbfs  = 1

instr 1
kcps = 440
kcar = 1
kmod = p4
kndx line 0, p3, 20	;intensivy sidebands
asig foscili .5, kcps, kcar, kmod, kndx, 1
outs asig, asig
endin

</CsInstruments>
<CsScore bin="python3">
import sys

score = '''
; This is a Python-generated score.

; sine
f 1 0 16384 10 1

i 1 0  9 .01	;vibrato
i 1 10 .  1
i 1 20 . 1.414	;gong-ish
i 1 30 5 2.05	;with "beat"
e
'''
print("Opening: {}\n".format(sys.argv[1]))
with open(sys.argv[1], 'w') as f:
    f.write(score)

</CsScore>
</CsoundSynthesizer>
