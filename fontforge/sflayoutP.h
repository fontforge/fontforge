/* Copyright (C) 2007-2012 by George Williams */
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

#ifndef FONTFORGE_SFLAYOUTP_H
#define FONTFORGE_SFLAYOUTP_H

enum sftf_fonttype { sftf_pfb, sftf_ttf, sftf_otf, sftf_nohints, sftf_bitmap, sftf_pfaedit };

typedef struct fontdata {
    SplineFont *sf;
    enum sftf_fonttype fonttype;
    int pointsize;
    int antialias;
    int layer;
    BDFFont *bdf;
    struct fontdata *next;
    struct fontdata *depends_on;	/* We use much of the ftc allocated for depends_on */
					/* Can't free depends_on until after we get freed */
    struct _GImage base;
    GImage gi;
    GClut clut;
    struct sfmaps *sfmap;
} FontData;

struct lineheights {
    int32_t y;
    int16_t as, fh;
    uint16_t p, linelen;
    uint32_t start_pos;
};

struct fontlist {
    int start, end;		/* starting and ending characters [start,end) */
				/*  always break at newline & will omit it between fontlists */
    uint32_t *feats;		/* Ends with a 0 entry */
    uint32_t script, lang;
    FontData *fd;
    SplineChar **sctext;
    int scmax;
    struct opentype_str *ottext;
    struct fontlist *next;
};

struct sfmaps {
    SplineFont *sf;
    EncMap *map;
    int16_t sfbit_id;
    int16_t notdef_gid;
    SplineChar *fake_notdef;
    struct sfmaps *next;
};

struct paras {
    struct opentype_str **para;	/* An array of pointers to ottext entries */
    int start_pos;
};

typedef struct layoutinfo {
    unichar_t *text, *oldtext;	/* Input glyphs (in unicode) */
    int16_t lcnt, lmax;
    struct opentype_str ***lines;	/* pointers into the paras array */
    int16_t xmax;
    struct lineheights *lineheights;
    struct fontlist *fontlist, *oldfontlist;
    struct sfmaps *sfmaps;
    struct paras *paras;
    int pcnt, pmax;
    int ps, pe, ls, le;
    struct fontlist *oldstart, *oldend;
    FontData *generated;
    float dpi;
    unsigned int wrap: 1;
} LayoutInfo;

extern void GImageDrawRect(GImage *img,GRect *r,Color col);
extern void GImageDrawImage(GImage *dest,GImage *src,GRect *junk,int x, int y);
extern int LI_FDDrawChar(void *data,
	void (*drawImage)(void *,GImage *,GRect *,int x, int y),
	void (*drawRect)(void *,GRect *,Color col),
	struct opentype_str *osc,int x,int y,Color col);
extern uint32_t *LI_TagsCopy(uint32_t *tags);
extern struct fontlist *LI_fontlistcopy(struct fontlist *fl );
extern void LI_fontlistmergecheck(LayoutInfo *li);
extern void LayoutInfoRefigureLines(LayoutInfo *li, int start_of_change,
	int end_of_change, int width);
extern int LayoutInfoReplace(LayoutInfo *li, const unichar_t *str,
	int sel_start, int sel_end,int width);
extern void LayoutInfo_Destroy(LayoutInfo *li);
extern void SFMapFill(struct sfmaps *sfmaps,SplineFont *sf);
extern struct sfmaps *SFMapOfSF(LayoutInfo *li,SplineFont *sf);
extern FontData *LI_FindFontData(LayoutInfo *li, SplineFont *sf,
	int layer, enum sftf_fonttype fonttype, int size, int antialias);
extern FontData *LI_RegenFontData(LayoutInfo *li, FontData *ret);
extern void LayoutInfoInitLangSys(LayoutInfo *li, int end, uint32_t script, uint32_t lang);
extern LayoutInfo *LIConvertToPrint(LayoutInfo *li, int width, int height, int dpi);
extern SplineSet *LIConvertToSplines(LayoutInfo *li,double dpi,int order2);
extern void LayoutInfoSetTitle(LayoutInfo *li,const unichar_t *tit,int width);
extern struct fontlist *LI_BreakFontList(LayoutInfo *li,int start,int end);
extern int LI_SetFontData(LayoutInfo *li, int start, int end, SplineFont *sf,
	int layer, enum sftf_fonttype fonttype, int size, int antialias,int width);

#endif /* FONTFORGE_SFLAYOUTP_H */
