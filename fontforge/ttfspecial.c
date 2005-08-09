/* Copyright (C) 2000-2005 by George Williams */
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
#include "pfaedit.h"
#include <utype.h>
#include <ustring.h>

#include "ttf.h"

/* This file contains routines to generate non-standard true/opentype tables */
/* The first is the 'PfEd' table containing PfaEdit specific information */
/*	glyph comments & colours ... perhaps other info later */

/* ************************************************************************** */
/* *************************    The 'PfEd' table    ************************* */
/* *************************         Output         ************************* */
/* ************************************************************************** */

/* 'PfEd' table format is as follows...				 */
/* uint32  version number 0x00010000				 */
/* uint32  subtable count					 */
/* struct { uint32 tab, offset } tag/offset for first subtable	 */
/* struct { uint32 tab, offset } tag/offset for second subtable	 */
/* ...								 */

/* 'PfEd' 'fcmt' font comment subtable format			 */
/*  short version number 0					 */
/*  short string length						 */
/*  String in latin1 (ASCII is better)				 */

/* 'PfEd' 'cmnt' glyph comment subtable format			 */
/*  short  version number 0					 */
/*  short  count-of-ranges					 */
/*  struct { short start-glyph, end-glyph, short offset }	 */
/*  ...								 */
/*  foreach glyph >=start-glyph, <=end-glyph(+1)		 */
/*   uint32 offset		to glyph comment string (in UCS2) */
/*  ...								 */
/*  And one last offset pointing beyong the end of the last string to enable length calculations */
/*  String table in UCS2 (NUL terminated). All offsets from start*/
/*   of subtable */

/* 'PfEd' 'colr' glyph colour subtable				 */
/*  short  version number 0					 */
/*  short  count-of-ranges					 */
/*  struct { short start-glyph, end-glyph, uint32 colour (rgb) } */

#define MAX_SUBTABLE_TYPES	3

struct PfEd_subtabs {
    int next;
    struct {
	FILE *data;
	uint32 tag;
	uint32 offset;
    } subtabs[MAX_SUBTABLE_TYPES];
};

static void PfEd_FontComment(SplineFont *sf, struct PfEd_subtabs *pfed ) {
    FILE *fcmt;
    char *pt;

    if ( sf->comments==NULL || *sf->comments=='\0' )
return;
    pfed->subtabs[pfed->next].tag = CHR('f','c','m','t');
    pfed->subtabs[pfed->next++].data = fcmt = tmpfile();

    putshort(fcmt,0);			/* sub-table version number */
    putshort(fcmt,strlen(sf->comments));
    for ( pt = sf->comments; *pt; ++pt )
	putshort(fcmt,*pt);
    putshort(fcmt,0);
    if ( ftell(fcmt)&2 ) putshort(fcmt,0);
}

static void PfEd_GlyphComments(SplineFont *sf, struct PfEd_subtabs *pfed,
	struct glyphinfo *gi ) {
    int i, j, k, any, cnt, last, skipped, subcnt;
    uint32 offset;
    SplineChar *sc, *sc2;
    FILE *cmnt;
    unichar_t *upt;

    any = 0;
    for ( i=0; i<sf->glyphcnt; ++i ) {
	if ( sf->glyphs[i]!=NULL && sf->glyphs[i]->ttf_glyph!=-1 &&
		sf->glyphs[i]->comment!=NULL ) {
	    any = true;
    break;
	}
    }

    if ( !any )
return;

    pfed->subtabs[pfed->next].tag = CHR('c','m','n','t');
    pfed->subtabs[pfed->next++].data = cmnt = tmpfile();

    putshort(cmnt,0);			/* sub-table version number */

    offset = 0;
    for ( j=0; j<4; ++j ) {
	cnt = 0;
	for ( i=0; i<gi->gcnt; ++i ) if ( gi->bygid[i]!=-1 ) {
	    sc=sf->glyphs[gi->bygid[i]];
	    if ( sc!=NULL && sc->comment!=NULL ) {
		last = i; skipped = false;
		subcnt = 1;
		for ( k=i+1; k<gi->gcnt; ++k ) {
		    if ( gi->bygid[k]!=-1 )
			sc2 = sf->glyphs[gi->bygid[i]];
		    if ( (gi->bygid[k]==-1 || sc2->comment==NULL) && skipped )
		break;
		    if ( gi->bygid[k]!=-1 && sc2->comment!=NULL ) {
			last = k;
			skipped = false;
		    } else
			skipped = true;
		    ++subcnt;
		}
		++cnt;
		if ( j==1 ) {
		    putshort(cmnt,i);
		    putshort(cmnt,last);
		    putlong(cmnt,offset);
		    offset += sizeof(uint32)*(subcnt+1);
		} else if ( j==2 ) {
		    for ( ; i<=last; ++i ) {
			if ( gi->bygid[i]==-1 || (sc=sf->glyphs[gi->bygid[i]])->comment==NULL )
			    putlong(cmnt,0);
			else {
			    putlong(cmnt,offset);
			    offset += sizeof(unichar_t)*(u_strlen(sc->comment)+1);
			}
		    }
		    putlong(cmnt,offset);	/* Guard data, to let us calculate the string lengths */
		} else if ( j==3 ) {
		    for ( ; i<=last; ++i ) {
			if ( gi->bygid[i]==-1 || (sc=sf->glyphs[gi->bygid[i]])->comment==NULL )
		    continue;
			for ( upt = sc->comment; *upt; ++upt )
			    putshort(cmnt,*upt);
			putshort(cmnt,0);
		    }
		}
		i = last;
	    }
	}
	if ( j==0 ) {
	    putshort(cmnt,cnt);
	    offset = 2*sizeof(short) + cnt*(2*sizeof(short)+sizeof(uint32));
	}
    }
    if ( ftell(cmnt) & 2 )
	putshort(cmnt,0);
}

static void PfEd_Colours(SplineFont *sf, struct PfEd_subtabs *pfed, struct glyphinfo *gi ) {
    int i, j, k, any, cnt, last;
    SplineChar *sc, *sc2;
    FILE *colr;

    any = 0;
    for ( i=0; i<sf->glyphcnt; ++i ) {
	if ( sf->glyphs[i]!=NULL && sf->glyphs[i]->ttf_glyph!=-1 &&
		sf->glyphs[i]->color!=COLOR_DEFAULT ) {
	    any = true;
    break;
	}
    }

    if ( !any )
return;

    pfed->subtabs[pfed->next].tag = CHR('c','o','l','r');
    pfed->subtabs[pfed->next++].data = colr = tmpfile();

    putshort(colr,0);			/* sub-table version number */
    for ( j=0; j<2; ++j ) {
	cnt = 0;
	for ( i=0; i<gi->gcnt; ++i ) if ( gi->bygid[i]!=-1 ) {
	    sc = sf->glyphs[gi->bygid[i]];
	    if ( sc!=NULL && sc->color!=COLOR_DEFAULT ) {
		last = i;
		for ( k=i+1; k<gi->gcnt; ++k ) {
		    if ( gi->bygid[k]==-1 )
		break;
		    sc2 = sf->glyphs[gi->bygid[k]];
		    if ( sc2->color != sc->color )
		break;
		    last = k;
		}
		++cnt;
		if ( j==1 ) {
		    putshort(colr,i);
		    putshort(colr,last);
		    putlong(colr,sc->color);
		}
		i = last;
	    }
	}
	if ( j==0 )
	    putshort(colr,cnt);
    }
    if ( ftell(colr) & 2 )
	putshort(colr,0);
}

void pfed_dump(struct alltabs *at, SplineFont *sf) {
    struct PfEd_subtabs pfed;
    FILE *file;
    int i;
    uint32 offset;

    memset(&pfed,0,sizeof(pfed));
    if ( at->gi.flags & ttf_flag_pfed_comments ) {
	PfEd_FontComment(sf, &pfed );
	PfEd_GlyphComments(sf, &pfed, &at->gi );
    }
    if ( at->gi.flags & ttf_flag_pfed_colors )
	PfEd_Colours(sf, &pfed, &at->gi );

    if ( pfed.next==0 )
return;		/* No subtables */

    at->pfed = file = tmpfile();
    putlong(file, 0x00010000);		/* Version number */
    putlong(file, pfed.next);		/* sub-table count */
    offset = 2*sizeof(uint32) + 2*pfed.next*sizeof(uint32);
    for ( i=0; i<pfed.next; ++i ) {
	putlong(file,pfed.subtabs[i].tag);
	putlong(file,offset);
	fseek(pfed.subtabs[i].data,0,SEEK_END);
	pfed.subtabs[i].offset = offset;
	offset += ftell(pfed.subtabs[i].data);
    }
    for ( i=0; i<pfed.next; ++i ) {
	fseek(pfed.subtabs[i].data,0,SEEK_SET);
	ttfcopyfile(file,pfed.subtabs[i].data,pfed.subtabs[i].offset,"PfEd-subtable");
    }
    if ( ftell(file)&3 )
	IError("'PfEd' table not properly aligned");
    at->pfedlen = ftell(file);
}

/* *************************    The 'PfEd' table    ************************* */
/* *************************          Input         ************************* */

static void pfed_readfontcomment(FILE *ttf,struct ttfinfo *info,uint32 base) {
    int len;
    char *pt, *end;

    fseek(ttf,base,SEEK_SET);
    if ( getushort(ttf)!=0 )
return;			/* Bad version number */
    len = getushort(ttf);
    pt = galloc(len+1);		/* data are stored as UCS2, but currently are ASCII */
    info->fontcomments = pt;
    end = pt+len;
    while ( pt<end )
	*pt++ = getushort(ttf);
    *pt = '\0';
}

static unichar_t *ReadUnicodeStr(FILE *ttf,uint32 offset,int len) {
    unichar_t *pt, *str, *end;

    pt = str = galloc(len);
    end = str+len/2;
    fseek(ttf,offset,SEEK_SET);
    while ( pt<end )
	*pt++ = getushort(ttf);
    *pt = 0;
return( str );
}
    
static void pfed_readglyphcomments(FILE *ttf,struct ttfinfo *info,uint32 base) {
    int n, i, j;
    struct grange { int start, end; uint32 offset; } *grange;
    uint32 offset, next;

    fseek(ttf,base,SEEK_SET);
    if ( getushort(ttf)!=0 )
return;			/* Bad version number */
    n = getushort(ttf);
    grange = galloc(n*sizeof(struct grange));
    for ( i=0; i<n; ++i ) {
	grange[i].start = getushort(ttf);
	grange[i].end = getushort(ttf);
	grange[i].offset = getlong(ttf);
	if ( grange[i].start>grange[i].end || grange[i].end>info->glyph_cnt ) {
	    fprintf( stderr, "Bad glyph range specified in glyph comment subtable of PfEd table\n" );
	    grange[i].start = 1; grange[i].end = 0;
	}
    }
    for ( i=0; i<n; ++i ) {
	for ( j=grange[i].start; j<=grange[i].end; ++j ) {
	    fseek( ttf,base+grange[i].offset+(j-grange[i].start)*sizeof(uint32),SEEK_SET);
	    offset = getlong(ttf);
	    next = getlong(ttf);
	    info->chars[j]->comment = ReadUnicodeStr(ttf,base+offset,next-offset);
	}
    }
    free(grange);
}

static void pfed_readcolours(FILE *ttf,struct ttfinfo *info,uint32 base) {
    int n, i, j, start, end;
    uint32 col;

    fseek(ttf,base,SEEK_SET);
    if ( getushort(ttf)!=0 )
return;			/* Bad version number */
    n = getushort(ttf);
    for ( i=0; i<n; ++i ) {
	start = getushort(ttf);
	end = getushort(ttf);
	col = getlong(ttf);
	if ( start>end || end>info->glyph_cnt )
	    fprintf( stderr, "Bad glyph range specified in colour subtable of PfEd table\n" );
	else {
	    for ( j=start; j<=end; ++j )
		info->chars[j]->color = col;
	}
    }
}

void pfed_read(FILE *ttf,struct ttfinfo *info) {
    int n,i;
    struct tagoff { uint32 tag, offset; } tagoff[MAX_SUBTABLE_TYPES+30];

    fseek(ttf,info->pfed_start,SEEK_SET);

    if ( getlong(ttf)!=0x00010000 )
return;
    n = getlong(ttf);
    if ( n>=MAX_SUBTABLE_TYPES+30 )
	n = MAX_SUBTABLE_TYPES+30;
    for ( i=0; i<n; ++i ) {
	tagoff[i].tag = getlong(ttf);
	tagoff[i].offset = getlong(ttf);
    }
    for ( i=0; i<n; ++i ) switch ( tagoff[i].tag ) {
      case CHR('f','c','m','t'):
	pfed_readfontcomment(ttf,info,info->pfed_start+tagoff[i].offset);
      break;
      case CHR('c','m','n','t'):
	pfed_readglyphcomments(ttf,info,info->pfed_start+tagoff[i].offset);
      break;
      case CHR('c','o','l','r'):
	pfed_readcolours(ttf,info,info->pfed_start+tagoff[i].offset);
      break;
      default:
	fprintf( stderr, "Unknown subtable '%c%c%c%c' in 'PfEd' table, ignored\n",
		tagoff[i].tag>>24, (tagoff[i].tag>>16)&0xff, (tagoff[i].tag>>8)&0xff, tagoff[i].tag&0xff );
      break;
    }
}

/* 'TeX ' table format is as follows...				 */
/* uint32  version number 0x00010000				 */
/* uint32  subtable count					 */
/* struct { uint32 tab, offset } tag/offset for first subtable	 */
/* struct { uint32 tab, offset } tag/offset for second subtable	 */
/* ...								 */

/* 'TeX ' 'ftpm' font parameter subtable format			 */
/*  short version number 0					 */
/*  parameter count						 */
/*  array of { 4chr tag, value }				 */

/* 'TeX ' 'htdp' per-glyph height/depth subtable format		 */
/*  short  version number 0					 */
/*  short  glyph-count						 */
/*  array[glyph-count] of { int16 height,depth }		 */

/* 'TeX ' 'sbsp' per-glyph sub/super script positioning subtable */
/*  short  version number 0					 */
/*  short  glyph-count						 */
/*  array[glyph-count] of { int16 sub,super }			 */

#undef MAX_SUBTABLE_TYPES
#define MAX_SUBTABLE_TYPES	3

struct TeX_subtabs {
    int next;
    struct {
	FILE *data;
	uint32 tag;
	uint32 offset;
    } subtabs[MAX_SUBTABLE_TYPES];
};

static uint32 tex_text_params[] = {
    TeX_Slant,
    TeX_Space,
    TeX_Stretch,
    TeX_Shrink,
    TeX_XHeight,
    TeX_Quad,
    TeX_ExtraSp,
    0
};
static uint32 tex_math_params[] = {
    TeX_Slant,
    TeX_Space,
    TeX_Stretch,
    TeX_Shrink,
    TeX_XHeight,
    TeX_Quad,
    TeX_MathSp,
    TeX_Num1,
    TeX_Num2,
    TeX_Num3,
    TeX_Denom1,
    TeX_Denom2,
    TeX_Sup1,
    TeX_Sup2,
    TeX_Sup3,
    TeX_Sub1,
    TeX_Sub2,
    TeX_SupDrop,
    TeX_SubDrop,
    TeX_Delim1,
    TeX_Delim2,
    TeX_AxisHeight,
    0};
static uint32 tex_mathext_params[] = {
    TeX_Slant,
    TeX_Space,
    TeX_Stretch,
    TeX_Shrink,
    TeX_XHeight,
    TeX_Quad,
    TeX_MathSp,
    TeX_DefRuleThick,
    TeX_BigOpSpace1,
    TeX_BigOpSpace2,
    TeX_BigOpSpace3,
    TeX_BigOpSpace4,
    TeX_BigOpSpace5,
    0};

/* *************************    The 'TeX ' table    ************************* */
/* *************************         Output         ************************* */

static void TeX_dumpFontParams(SplineFont *sf, struct TeX_subtabs *tex ) {
    FILE *fprm;
    int i,pcnt;
    uint32 *tags;

    if ( sf->texdata.type==tex_unset )
return;
    tex->subtabs[tex->next].tag = CHR('f','t','p','m');
    tex->subtabs[tex->next++].data = fprm = tmpfile();

    putshort(fprm,0);			/* sub-table version number */
    pcnt = sf->texdata.type==tex_math ? 22 : sf->texdata.type==tex_mathext ? 13 : 7;
    tags = sf->texdata.type==tex_math ? tex_math_params :
	    sf->texdata.type==tex_mathext ? tex_mathext_params :
					    tex_text_params;
    putshort(fprm,pcnt);
    for ( i=0; i<pcnt; ++i ) {
	putlong(fprm,tags[i]);
	putlong(fprm,sf->texdata.params[i]);
    }
    /* always aligned */
}

static void TeX_dumpHeightDepth(SplineFont *sf, struct TeX_subtabs *tex ) {
    FILE *htdp;
    int i,j,k,last_g;
    DBounds b;

    for ( i=sf->glyphcnt-1; i>=0; --i )
	if ( sf->glyphs[i]!=NULL &&
		(sf->glyphs[i]->tex_height!=TEX_UNDEF || sf->glyphs[i]->tex_depth!=TEX_UNDEF))
    break;
    if ( i<0 )		/* No height/depth info */
return;

    tex->subtabs[tex->next].tag = CHR('h','t','d','p');
    tex->subtabs[tex->next++].data = htdp = tmpfile();

    putshort(htdp,0);				/* sub-table version number */
    putshort(htdp,sf->glyphs[i]->ttf_glyph+1);	/* data for this many glyphs */

    last_g = -1;
    for ( j=0; j<=i; ++j ) if ( sf->glyphs[j]!=NULL ) {
	SplineChar *sc = sf->glyphs[j];
	for ( k=last_g+1; k<sc->ttf_glyph; ++k ) {
	    putshort(htdp,0);
	    putshort(htdp,0);
	}
	if ( sc->tex_depth==TEX_UNDEF || sc->tex_height==TEX_UNDEF )
	    SplineCharFindBounds(sc,&b);
	putshort( htdp, sc->tex_height==TEX_UNDEF ? b.maxy : sc->tex_height );
	putshort( htdp, sc->tex_depth==TEX_UNDEF ? -b.miny : sc->tex_depth );
	last_g = sc->ttf_glyph;
    }
    /* always aligned */
}

static void TeX_dumpSubSuper(SplineFont *sf, struct TeX_subtabs *tex ) {
    FILE *sbsp;
    int i,j,k,last_g;

    for ( i=sf->glyphcnt-1; i>=0; --i )
	if ( sf->glyphs[i]!=NULL &&
		(sf->glyphs[i]->tex_sub_pos!=TEX_UNDEF || sf->glyphs[i]->tex_super_pos!=TEX_UNDEF))
    break;
    if ( i<0 )		/* No sub/super info */
return;

    tex->subtabs[tex->next].tag = CHR('s','b','s','p');
    tex->subtabs[tex->next++].data = sbsp = tmpfile();

    putshort(sbsp,0);				/* sub-table version number */
    putshort(sbsp,sf->glyphs[i]->ttf_glyph+1);	/* data for this many glyphs */

    last_g = -1;
    for ( j=0; j<=i; ++j ) if ( sf->glyphs[j]!=NULL ) {
	SplineChar *sc = sf->glyphs[j];
	for ( k=last_g+1; k<sc->ttf_glyph; ++k ) {
	    putshort(sbsp,0);
	    putshort(sbsp,0);
	}
	putshort( sbsp, sc->tex_sub_pos==TEX_UNDEF ? sc->width : sc->tex_sub_pos );
	putshort( sbsp, sc->tex_super_pos!=TEX_UNDEF ? sc->tex_super_pos :
			sc->tex_sub_pos!=TEX_UNDEF ? sc->width - sc->tex_sub_pos :
				0 );
	last_g = sc->ttf_glyph;
    }
    /* always aligned */
}

void tex_dump(struct alltabs *at, SplineFont *sf) {
    struct TeX_subtabs tex;
    FILE *file;
    int i;
    uint32 offset;

    if ( !(at->gi.flags & ttf_flag_TeXtable ))
return;

    memset(&tex,0,sizeof(tex));
    TeX_dumpFontParams(sf,&tex);
    TeX_dumpHeightDepth(sf,&tex);
    TeX_dumpSubSuper(sf,&tex);

    if ( tex.next==0 )
return;		/* No subtables */

    at->tex = file = tmpfile();
    putlong(file, 0x00010000);		/* Version number */
    putlong(file, tex.next);		/* sub-table count */
    offset = 2*sizeof(uint32) + 2*tex.next*sizeof(uint32);
    for ( i=0; i<tex.next; ++i ) {
	putlong(file,tex.subtabs[i].tag);
	putlong(file,offset);
	fseek(tex.subtabs[i].data,0,SEEK_END);
	tex.subtabs[i].offset = offset;
	offset += ftell(tex.subtabs[i].data);
    }
    for ( i=0; i<tex.next; ++i ) {
	fseek(tex.subtabs[i].data,0,SEEK_SET);
	ttfcopyfile(file,tex.subtabs[i].data,tex.subtabs[i].offset,"TeX-subtable");
    }
    if ( ftell(file)&3 )
	IError("'TeX ' table not properly aligned");
    at->texlen = ftell(file);
}

/* *************************    The 'TeX ' table    ************************* */
/* *************************          Input         ************************* */

static void TeX_readFontParams(FILE *ttf,struct ttfinfo *info,uint32 base) {
    int i,pcnt;
    static uint32 *alltags[] = { tex_text_params, tex_math_params, tex_mathext_params };
    int j,k;
    uint32 tag;
    int32 val;

    fseek(ttf,base,SEEK_SET);
    if ( getushort(ttf)!=0 )	/* Don't know how to read this version of the subtable */
return;
    pcnt = getushort(ttf);
    if ( pcnt==22 ) info->texdata.type = tex_math;
    else if ( pcnt==13 ) info->texdata.type = tex_mathext;
    else if ( pcnt>=7 ) info->texdata.type = tex_text;
    for ( i=0; i<pcnt; ++i ) {
	tag = getlong(ttf);
	val = getlong(ttf);
	for ( j=0; j<3; ++j ) {
	    for ( k=0; alltags[j][k]!=0; ++k )
		if ( alltags[j][k]==tag )
	    break;
	    if ( alltags[j][k]==tag )
	break;
	}
	if ( j<3 )
	    info->texdata.params[k] = val;
    }
}

static void TeX_readHeightDepth(FILE *ttf,struct ttfinfo *info,uint32 base) {
    int i,gcnt;

    fseek(ttf,base,SEEK_SET);
    if ( getushort(ttf)!=0 )	/* Don't know how to read this version of the subtable */
return;
    gcnt = getushort(ttf);
    for ( i=0; i<gcnt && i<info->glyph_cnt; ++i ) {
	int h, d;
	h = getushort(ttf);
	d = getushort(ttf);
	if ( info->chars[i]!=NULL ) {
	    info->chars[i]->tex_height = h;
	    info->chars[i]->tex_depth = d;
	}
    }
}

static void TeX_readSubSuper(FILE *ttf,struct ttfinfo *info,uint32 base) {
    int i,gcnt;

    fseek(ttf,base,SEEK_SET);
    if ( getushort(ttf)!=0 )	/* Don't know how to read this version of the subtable */
return;
    gcnt = getushort(ttf);
    for ( i=0; i<gcnt && i<info->glyph_cnt; ++i ) {
	int super, sub;
	sub = getushort(ttf);
	super = getushort(ttf);
	if ( info->chars[i]!=NULL ) {
	    info->chars[i]->tex_sub_pos = sub;
	    info->chars[i]->tex_super_pos = super;
	}
    }
}

void tex_read(FILE *ttf,struct ttfinfo *info) {
    int n,i;
    struct tagoff { uint32 tag, offset; } tagoff[MAX_SUBTABLE_TYPES+30];

    fseek(ttf,info->tex_start,SEEK_SET);

    if ( getlong(ttf)!=0x00010000 )
return;
    n = getlong(ttf);
    if ( n>=MAX_SUBTABLE_TYPES+30 )
	n = MAX_SUBTABLE_TYPES+30;
    for ( i=0; i<n; ++i ) {
	tagoff[i].tag = getlong(ttf);
	tagoff[i].offset = getlong(ttf);
    }
    for ( i=0; i<n; ++i ) switch ( tagoff[i].tag ) {
      case CHR('f','t','p','m'):
	TeX_readFontParams(ttf,info,info->tex_start+tagoff[i].offset);
      break;
      case CHR('h','t','d','p'):
	TeX_readHeightDepth(ttf,info,info->tex_start+tagoff[i].offset);
      break;
      case CHR('s','b','s','p'):
	TeX_readSubSuper(ttf,info,info->tex_start+tagoff[i].offset);
      break;
      default:
	fprintf( stderr, "Unknown subtable '%c%c%c%c' in 'TeX ' table, ignored\n",
		tagoff[i].tag>>24, (tagoff[i].tag>>16)&0xff, (tagoff[i].tag>>8)&0xff, tagoff[i].tag&0xff );
      break;
    }
}
