/*
    interlocks.h:

    Copyright (C) 2011 John ffitch

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

// ZAK
#define ZR (0x0001)
#define ZW (0x0002)
#define ZB (0x0003)

// Writes to inputs
#define WI (0x0004)

//Tables
#define TR (0x0008)
#define TW (0x0010)
#define TB (0x0018)

//Channels
#define _CR (0x0020)
#define _CW (0x0040)
#define _CB (0x0060)

//Stack
#define SK (0x0080)

//Printing
#define WR (0x0100)

// Internal oddities -- SPOUT
#define IR (0x0200)
#define IW (0x0400)
#define IB (0x0600)

// Declare but not defined
#define UNDEFINED (0x0800)

//Deprecated
#define _QQ (0x8000)
