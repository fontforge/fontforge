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

/* could probably merge with vhea.c */
    TableFillup(tab);
table version = tgetfixed(tab,0);
ascender = (short) tgetushort(tab,4);
descender = (short) tgetushort(tab,6);
linegap = (short) tgetushort(tab,8);
advanceWidthmax = tgetushort(tab,10);
minlbearing = (short) tgetushort(tab,12);
minrbearing = (short) tgetushort(tab,14);
xmaxextent = (short) tgetushort(tab,16);
caretSlopeRise = (short) tgetushort(tab,18);
caretSlopeRun = (short) tgetushort(tab,20);
mbz1 = (short) tgetushort(tab,22);
mbz2 = (short) tgetushort(tab,24);
mbz3 = (short) tgetushort(tab,26);
mbz4 = (short) tgetushort(tab,28);
mbz5 = (short) tgetushort(tab,30);
metricdataformat = (short) tgetushort(tab,32);
numberhmetrics = tgetushort(tab,34);
