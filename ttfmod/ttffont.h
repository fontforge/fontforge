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
#ifndef _TTFFONT_H
#define _TTFFONT_H

#include "basics.h"
#include "charset.h"

#ifdef USE_DOUBLE
# define real	double
#else
# define real	float
#endif

typedef struct table {
    int32 name;
    int32 start;
    int32 len;
    int32 newlen;			/* actual length, but data will be */
    uint8 *data;			/*  padded out to 32bit boundary with 0*/
    int32 oldchecksum;
    unsigned int changed: 1;
    unsigned int special: 1;		/* loca, hmtx, glyph all are bound together */
    struct ttffile *container;
    /* No pointer to the font, because a given table may be part of several */
    /*  different fonts in a ttc */
    struct tableview *tv;
    void *table_data;
    void (*free_tabledata)(void *);
} Table;
    
typedef struct ttffont {
    unichar_t *fontname;
    int32 version;
    int tbl_cnt, tbl_max;
    Table **tbls;
    int32 glyph_cnt, enc_glyph_cnt;
    uint16 *unicode_enc;
    uint16 *enc;
    struct ttffile *container;
    unsigned int expanded: 1;		/* should be in TtfView */
} TtfFont;

typedef struct ttffile {
    char *filename;
    FILE *file;
    int font_cnt;
    TtfFont **fonts;
    int is_ttc: 1;
    int changed: 1;
    struct ttfview *tfv;
} TtfFile;

extern int getushort(FILE *ttf);
extern int32 getlong(FILE *ttf);
extern real getfixed(FILE *ttf);
extern real get2dot14(FILE *ttf);
void TableFillup(Table *tbl);
int tgetushort(Table *tab,int pos);
int32 tgetlong(Table *tab,int pos);
real tgetfixed(Table *tab,int pos);
real tget2dot14(Table *tab,int pos);
extern TtfFile *ReadTtfFont(char *filename);
extern TtfFile *LoadTtfFont(char *filename);

#endif
