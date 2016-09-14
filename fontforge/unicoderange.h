/* Copyright (C) 2006-2012 by George Williams */
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

#ifndef FONTFORGE_UNICODERANGE_H
#define FONTFORGE_UNICODERANGE_H

extern struct unicoderange {
    char *name;		/* The range's name */
    int32 first, last, defined;
    			/* The first codepoint, last codepoint in the range */
			/*  and a codepoint which actually has a character */
			/*  associated with it */
    uint8 display;
    uint8 unassigned;	/* No characters in this range are assigned */
    int actual;		/* Count of assigned codepoints in this range */
} unicoderange[];

#define UNICODERANGE_EMPTY { NULL, 0, 0, 0, 0, 0, 0 }


struct rangeinfo {
    struct unicoderange *range;
    int cnt;
    int actual;
};

#define RANGEINFO_EMPTY { NULL, 0, 0 }


enum ur_flags { ur_includeempty = 1, ur_sortbyname = 2, ur_sortbyunicode = 4 };
extern struct rangeinfo *SFUnicodeRanges(SplineFont *sf, enum ur_flags flags);
extern int unicoderange_cnt;

#endif /* FONTFORGE_UNICODERANGE_H */
