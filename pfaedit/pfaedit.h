/* Copyright (C) 2000,2001 by George Williams */
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
#ifndef _PFAEDIT_H_
#define _PFAEDIT_H_

#include "basics.h"
#include <stdio.h>
#include <string.h>
#include <gprogress.h>
#include "splinefont.h"

typedef struct enc {
    int enc_num;
    char *enc_name;
    int char_cnt;
    unichar_t *unicode;
    char **psnames;
    struct enc *next;
    unsigned int builtin: 1;
} Encoding;

enum { em_base = 0x100 };	/* an addition to enum charset, used as the base value for the encoding list above */

extern void GDrawIError(const char *fmt,...);
extern void GDrawError(const char *fmt,...);

extern char *AdobeStandardEncoding[256];
extern int psunicodenames_cnt;
extern const char *psunicodenames[];
extern unsigned short unicode_from_adobestd[256];

extern int default_fv_font_size;
extern int default_fv_antialias;
extern int default_encoding;
extern int adjustwidth;
extern int adjustlbearing;
extern int autohint_before_rasterize;
extern int seperate_hint_controls;
extern int ItalicConstrained;

extern char *BDFFoundry;
extern char *xuid;

extern int pagewidth, pageheight, printtype;	/* Printer defaults */

extern Encoding *enclist;

#define RECENT_MAX	4
extern char *RecentFiles[RECENT_MAX];

#endif
