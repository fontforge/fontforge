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

#include "gdraw.h"		/* For GDrawIError */
#include "charset.h"

#ifdef USE_DOUBLE
# define real	double
#else
# define real	float
#endif

#define CHAR_UNDEF	0xffff
struct enctab {
    int platform, specific, offset;
    int format, len, language;
	/* MS says this is a version, apple/adobe says a language. */
	/* MBZ for ms platform, apple,adobe says 0 is "language independant" */
    int cnt;
    unichar_t *uenc;		/* map from glyph num => unicode */
    unichar_t *enc;		/* map from glyph num => char sequence */
    struct enctab *next;
};

/* The EBDT and bdat tags could reasonable point to the same table */
/* as could EBLC and bloc. Why haven't Apple and MS used the same tag? */
/* they use the same formats... */
/* The same table may be used by several fonts */
typedef struct table {
    int32 name;
    int32 start;
    int32 len;
    int32 newlen;			/* actual length, but data will be */
    uint8 *data;			/*  padded out to 32bit boundary with 0*/
    int32 oldchecksum;
    int32 newstart;			/* used during saving */
    int32 newchecksum;			/* used during saving */
    int32 othernames[4];		/* for bdat/EBDT etc. */
    unsigned int changed: 1;		/* someone has changed either data or table_data */
    unsigned int td_changed: 1;		/* it's table_data that has changed */
    unsigned int special: 1;		/* loca, hmtx, glyph all are bound together */
    unsigned int new: 1;		/* table is new, nothing to revert to */
    unsigned int freeing: 1;		/* table has been put on list of tables to be freed */
    unsigned int inserted: 1;		/* table has been inserted into ordered table list (for save) */
    unsigned int processed: 1;
    int orderingval;
    struct ttffile *container;
    /* No pointer to the font, because a given table may be part of several */
    /*  different fonts in a ttc */
    struct tableview *tv;
    void *table_data;
    void (*free_tabledata)(void *);
    void (*write_tabledata)(FILE *ttf,struct table *tab);
} Table;
    
typedef struct ttffont {
    unichar_t *fontname;
    int32 version;
    int tbl_cnt, tbl_max;
    Table **tbls;
    int32 glyph_cnt;
    struct enctab *enc;
    struct ttffile *container;
    unsigned int expanded: 1;		/* should be in TtfView */
    int32 version_pos;
} TtfFont;

typedef struct ttffile {
    char *filename;
    FILE *file;
    int font_cnt;
    TtfFont **fonts;
    int is_ttc: 1;
    int changed: 1;
    unsigned int gcchanged: 1;		/* If the glyph count in any font changes then set this... */
    unsigned int backedup: 1;		/* a backup file has been created */
    struct ttfview *tfv;
} TtfFile;

extern int getushort(FILE *ttf);
extern int32 getlong(FILE *ttf);
extern real getfixed(FILE *ttf);
extern real getvfixed(FILE *ttf);		/* Reads table version numbers which are some weird (undocumented) bcd format */
extern real get2dot14(FILE *ttf);
Table *TableFind(TtfFont *tfont, int name);
void TableFillup(Table *tbl);
int tgetushort(Table *tab,int pos);
int32 tgetlong(Table *tab,int pos);
real tgetfixed(Table *tab,int pos);
real tgetvfixed(Table *tab,int pos);
real tget2dot14(Table *tab,int pos);
int ptgetushort(uint8 *data);
int32 ptgetlong(uint8 *data);
real ptgetfixed(uint8 *data);
real ptgetvfixed(uint8 *data);

void putushort(FILE *file,uint16 val);
void putshort(FILE *file,uint16 val);
void putlong(FILE *file,uint32 val);
void ptputushort(uint8 *data, uint16 val);
void ptputlong(uint8 *data, uint32 val);
void ptputfixed(uint8 *data,real val);
void ptputvfixed(uint8 *data,real val);

void readttfencodings(struct ttffont *font);
void TTFFileFreeData(TtfFile *ttf);
void TTFFileFree(TtfFile *ttf);
extern TtfFile *ReadTtfFont(char *filename);
extern TtfFile *LoadTtfFont(char *filename);
extern int TtfSave(TtfFile *ttf,char *newpath);

#endif
