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
table format = tgetfixed(tab,0);
italicangle = tgetfixed(tab,4);
upos = (short) tgetushort(tab,8);
uwidth = (short) tgetushort(tab,10);
isfixed = tgetlong(tab,12);
minMemType42 =tgetlong(tab,16);
maxMemType42 =tgetlong(tab,20);
minMemType1 =tgetlong(tab,24);
maxMemType1 =tgetlong(tab,28);
if ( tableformat==0x10000 )
    /* table ends here, names are those of standard apple */
if ( tableformat==0x30000 )
    /* table ends here, no names */
if ( tableformat==0x28000 )
    char offset[numglyphs]	/* allows you to reorder apple's names
if ( tableformat==0x20000 )
    numglyphs = tgetushort(tab,32)
    glyphnameindex[numglyphs]	/* [0,257] stdnames, else index-258 */
    char string data
