/* Copyright (C) 2001 by George Williams */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "ttfmodui.h"
#include <ustring.h>
#include <utype.h>

    TableFillup(tab);
table version = tgetfixed(tab,0);
fontrevision = tgetfixed(tab,4);
checksum = tgetlong(tab,8);
magic = tgetlong(tab,12);
flags = tgetushort(tab,16);
    0 baseline@y==0
    1 lbearing@x==0
    2 instrs depend on pointsize
    3 force ppem to integer
    4 instrs alter advance width
unitsPerEm = tgetushort(tab,18);
quaddate = tgetlong(tab,20)<<32 | tgetlong(tab,24)
xMin = (short) tgetushort(tab,28);
yMin = (short) tgetushort(tab,30);
xMax = (short) tgetushort(tab,32);
yMax = (short) tgetushort(tab,34);
macStyle = tgetushort(tab,36)
    0 bold
    1 italic
lowestReadablePixelSize = tgetushort(tab,38)
fontdirection = (short) tgetushort(tab,40)
    0 fully mixed
    1 only stronly ltor
    2 ltor or neutral
    -1 only strongly rtol
    -2 rtol or neutral
locaformat = (short) tgetushort(tab,42)
    0 shorts, 1 longs
glyphdataformat = (short) tgetushort(tab,44)
