/* Copyright (C) 2000-2006 by George Williams */
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
#include <gdraw.h>		/* For COLOR_DEFAULT */

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
    char *upt;
    uint32 uch;

    any = 0;
    /* We don't need to check in bygid order. We just want to know existance */
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
			    offset += sizeof(unichar_t)*(utf82u_strlen(sc->comment)+1);
			}
		    }
		    putlong(cmnt,offset);	/* Guard data, to let us calculate the string lengths */
		} else if ( j==3 ) {
		    for ( ; i<=last; ++i ) {
			if ( gi->bygid[i]==-1 || (sc=sf->glyphs[gi->bygid[i]])->comment==NULL )
		    continue;
			for ( upt = sc->comment; (uch = utf8_ildb((const char **) &upt))!='\0'; ) {
			    if ( uch<0x10000 )
				putshort(cmnt,uch);
			    else {
				putshort(cmnt,0xd800|(uch/0x400));
				putshort(cmnt,0xdc00|(uch&0x3ff));
			    }
			}
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

static char *ReadUnicodeStr(FILE *ttf,uint32 offset,int len) {
    char *pt, *str;
    uint32 uch, uch2;
    int i;

    len>>=1;
    pt = str = galloc(3*len);
    fseek(ttf,offset,SEEK_SET);
    for ( i=0; i<len; ++i ) {
	uch = getushort(ttf);
	if ( uch>=0xd800 && uch<0xdc00 ) {
	    uch2 = getushort(ttf);
	    if ( uch2>=0xdc00 && uch2<0xe000 )
		uch = ((uch-0xd800)<<10) | (uch2&0x3ff);
	    else {
		pt = utf8_idpb(pt,uch);
		uch = uch2;
	    }
	}
	pt = utf8_idpb(pt,uch);
    }
    *pt++ = 0;
return( grealloc(str,pt-str) );
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
	    LogError( _("Bad glyph range specified in glyph comment subtable of PfEd table\n") );
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
	    LogError( _("Bad glyph range specified in colour subtable of PfEd table\n") );
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
	LogError( _("Unknown subtable '%c%c%c%c' in 'PfEd' table, ignored\n"),
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

/* ************************************************************************** */
/* *************************    The 'TeX ' table    ************************* */
/* *************************         Output         ************************* */
/* ************************************************************************** */

static void TeX_dumpFontParams(SplineFont *sf, struct TeX_subtabs *tex, struct alltabs *at ) {
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

static void TeX_dumpHeightDepth(SplineFont *sf, struct TeX_subtabs *tex, struct alltabs *at ) {
    FILE *htdp;
    int i,j,k,last_g, gid;
    DBounds b;

    for ( i=at->gi.gcnt-1; i>=0; --i ) {
	gid = at->gi.bygid[i];
	if ( gid!=-1 && sf->glyphs[gid]!=NULL &&
		(sf->glyphs[gid]->tex_height!=TEX_UNDEF || sf->glyphs[gid]->tex_depth!=TEX_UNDEF))
    break;
    }
    if ( i<0 )		/* No height/depth info */
return;

    tex->subtabs[tex->next].tag = CHR('h','t','d','p');
    tex->subtabs[tex->next++].data = htdp = tmpfile();

    putshort(htdp,0);				/* sub-table version number */
    putshort(htdp,sf->glyphs[gid]->ttf_glyph+1);/* data for this many glyphs */

    last_g = -1;
    for ( j=0; j<=i; ++j ) {
	gid = at->gi.bygid[j];
	if ( gid!=-1 && sf->glyphs[gid]!=NULL ) {
	    SplineChar *sc = sf->glyphs[gid];
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
    }
    /* always aligned */
}

static void TeX_dumpSubSuper(SplineFont *sf, struct TeX_subtabs *tex, struct alltabs *at ) {
    FILE *sbsp;
    int i,j,k,last_g, gid;

    for ( i=at->gi.gcnt-1; i>=0; --i ) {
	gid = at->gi.bygid[i];
	if ( gid!=-1 && sf->glyphs[gid]!=NULL &&
		(sf->glyphs[gid]->tex_sub_pos!=TEX_UNDEF || sf->glyphs[gid]->tex_super_pos!=TEX_UNDEF))
    break;
    }
    if ( i<0 )		/* No sub/super info */
return;

    tex->subtabs[tex->next].tag = CHR('s','b','s','p');
    tex->subtabs[tex->next++].data = sbsp = tmpfile();

    putshort(sbsp,0);				/* sub-table version number */
    putshort(sbsp,sf->glyphs[gid]->ttf_glyph+1);/* data for this many glyphs */

    last_g = -1;
    for ( j=0; j<=i; ++j ) {
	gid = at->gi.bygid[j];
	if ( gid!=-1 && sf->glyphs[gid]!=NULL ) {
	    SplineChar *sc = sf->glyphs[gid];
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
    TeX_dumpFontParams(sf,&tex,at);
    TeX_dumpHeightDepth(sf,&tex,at);
    TeX_dumpSubSuper(sf,&tex,at);

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
	LogError( _("Unknown subtable '%c%c%c%c' in 'TeX ' table, ignored\n"),
		tagoff[i].tag>>24, (tagoff[i].tag>>16)&0xff, (tagoff[i].tag>>8)&0xff, tagoff[i].tag&0xff );
      break;
    }
}

/* ************************************************************************** */
/* *************************    The 'BDF ' table    ************************* */
/* *************************         Output         ************************* */
/* ************************************************************************** */

/* the BDF table is used to store BDF properties so that we can do round trip */
/* conversion from BDF->otb->BDF without losing anything. */
/* Format:
	USHORT	version		: 'BDF' table version number, must be 0x0001
	USHORT	strikeCount	: number of strikes in table
	ULONG	stringTable	: offset (from start of BDF table) to string table

followed by an array of 'strikeCount' descriptors that look like: 
	USHORT	ppem		: vertical pixels-per-EM for this strike
	USHORT	num_items	: number of items (properties and atoms), max is 255

this array is followed by 'strikeCount' value sets. Each "value set" is 
an array of (num_items) items that look like:
	ULONG	item_name	: offset in string table to item name
	USHORT	item_type	: item type: 0 => non-property string (e.g. COMMENT)
					     1 => non-property atom (e.g. FONT)
					     2 => non-property int32
					     3 => non-property uint32
					  0x10 => flag for a property, ored
						  with above value types)
	ULONG	item_value	: item value. 
				strings	 => an offset into the string table
					  to the corresponding string,
					  without the surrending double-quotes

				atoms	 => an offset into the string table

				integers => the corresponding 32-bit value
Then the string table of null terminated strings. These strings should be in
ASCII.
*/

/* Internally FF stores BDF comments as one psuedo property per line. As you */
/*  might expect. But FreeType merges them into one large lump with newlines */
/*  between lines. Which means that BDF tables created by FreeType will be in*/
/*  that format. So we might as well be compatible. We will pack & unpack    */
/*  comment lines */

static int BDFPropCntMergedComments(BDFFont *bdf) {
    int i, cnt;

    cnt = 0;
    for ( i=0; i<bdf->prop_cnt; ++i ) {
	++cnt;
	if ( strmatch(bdf->props[i].name,"COMMENT")==0 )
    break;
    }
    for ( ; i<bdf->prop_cnt; ++i ) {
	if ( strmatch(bdf->props[i].name,"COMMENT")==0 )
    continue;
	++cnt;
    }
return( cnt );
}

static char *MergeComments(BDFFont *bdf) {
    int len, i;
    char *str;

    len = 0;
    for ( i=0; i<bdf->prop_cnt; ++i ) {
	if ( strmatch(bdf->props[i].name,"COMMENT")==0 &&
		((bdf->props[i].type & ~prt_property)==prt_string ||
		 (bdf->props[i].type & ~prt_property)==prt_atom ))
	    len += strlen( bdf->props[i].u.atom ) + 1;
    }
    if ( len==0 )
return( copy( "" ));

    str = galloc( len+1 );
    len = 0;
    for ( i=0; i<bdf->prop_cnt; ++i ) {
	if ( strmatch(bdf->props[i].name,"COMMENT")==0 &&
		((bdf->props[i].type & ~prt_property)==prt_string ||
		 (bdf->props[i].type & ~prt_property)==prt_atom )) {
	    strcpy(str+len,bdf->props[i].u.atom );
	    len += strlen( bdf->props[i].u.atom ) + 1;
	    str[len-1] = '\n';
	}
    }
    str[len-1] = '\0';
return( str );
}
    
#define AMAX	50

int ttf_bdf_dump(SplineFont *sf,struct alltabs *at,int32 *sizes) {
    FILE *strings;
    struct atomoff { char *name; int pos; } atomoff[AMAX];
    int acnt=0;
    int spcnt = 0;
    int i,j,k;
    BDFFont *bdf;
    long pos;

    for ( i=0; sizes[i]!=0; ++i ) {
	for ( bdf=sf->bitmaps; bdf!=NULL && (bdf->pixelsize!=(sizes[i]&0xffff) || BDFDepth(bdf)!=(sizes[i]>>16)); bdf=bdf->next );
	if ( bdf!=NULL && bdf->prop_cnt!=0 )
	    ++spcnt;
    }
    if ( spcnt==0 )	/* No strikes with properties */
return(true);
	
    at->bdf = tmpfile();
    strings = tmpfile();

    putshort(at->bdf,0x0001);
    putshort(at->bdf,spcnt);
    putlong(at->bdf,0);		/* offset to string table */

    for ( i=0; sizes[i]!=0; ++i ) {
	for ( bdf=sf->bitmaps; bdf!=NULL && (bdf->pixelsize!=(sizes[i]&0xffff) || BDFDepth(bdf)!=(sizes[i]>>16)); bdf=bdf->next );
	if ( bdf!=NULL && bdf->prop_cnt!=0 ) {
	    putshort(at->bdf,bdf->pixelsize);
	    putshort(at->bdf,BDFPropCntMergedComments(bdf));
	}
    }

    for ( i=0; sizes[i]!=0; ++i ) {
	for ( bdf=sf->bitmaps; bdf!=NULL && (bdf->pixelsize!=(sizes[i]&0xffff) || BDFDepth(bdf)!=(sizes[i]>>16)); bdf=bdf->next );
	if ( bdf!=NULL && bdf->prop_cnt!=0 ) {
	    int saw_comment=0;
	    char *str;
	    for ( j=0; j<bdf->prop_cnt; ++j ) {
		if ( strmatch(bdf->props[j].name,"COMMENT")==0 && saw_comment )
	    continue;
		/* Try to reuse keyword names in string space */
		for ( k=0 ; k<acnt; ++k ) {
		    if ( strcmp(atomoff[k].name,bdf->props[j].name)==0 )
		break;
		}
		if ( k>=acnt && k<AMAX ) {
		    atomoff[k].name = bdf->props[j].name;
		    atomoff[k].pos = ftell(strings);
		    ++acnt;
		    fwrite(atomoff[k].name,1,strlen(atomoff[k].name)+1,strings);
		}
		if ( k<acnt )
		    putlong(at->bdf,atomoff[k].pos);
		else {
		    putlong(at->bdf,ftell(strings));
		    fwrite(bdf->props[j].name,1,strlen(bdf->props[j].name)+1,strings);
		}
		str = bdf->props[j].u.str;
		if ( strmatch(bdf->props[j].name,"COMMENT")==0 ) {
		    str = MergeComments(bdf);
		    saw_comment = true;
		}
		putshort(at->bdf,bdf->props[j].type);
		if ( (bdf->props[j].type & ~prt_property)==prt_string ||
			(bdf->props[j].type & ~prt_property)==prt_atom ) {
		    putlong(at->bdf,ftell(strings));
		    fwrite(str,1,strlen(str)+1,strings);
		    if ( str!=bdf->props[j].u.str )
			free(str);
		} else {
		    putlong(at->bdf,bdf->props[j].u.val);
		}
	    }
	}
    }

    pos = ftell(at->bdf);
    fseek(at->bdf,4,SEEK_SET);
    putlong(at->bdf,pos);
    fseek(at->bdf,0,SEEK_END);

    if ( !ttfcopyfile(at->bdf,strings,pos,"BDF string table"))
return( false );

    at->bdflen = ftell( at->bdf );

    /* Align table */
    if ( at->bdflen&1 )
	putc('\0',at->bdf);
    if ( (at->bdflen+1)&2 )
	putshort(at->bdf,0);

return( true );
}

/* *************************    The 'BDF ' table    ************************* */
/* *************************          Input         ************************* */

static char *getstring(FILE *ttf,long start) {
    long here = ftell(ttf);
    int len, ch;
    char *str, *pt;

    fseek(ttf,start,SEEK_SET);
    for ( len=1; (ch=getc(ttf))>0 ; ++len );
    fseek(ttf,start,SEEK_SET);
    pt = str = galloc(len);
    while ( (ch=getc(ttf))>0 )
	*pt++ = ch;
    *pt = '\0';
    fseek(ttf,here,SEEK_SET);
return( str );
}

/* COMMENTS get stored all in one lump by freetype. De-lump them */
static int CheckForNewlines(BDFFont *bdf,int k) {
    char *pt, *start;
    int cnt, i;

    for ( cnt=0, pt = bdf->props[k].u.atom; *pt; ++pt )
	if ( *pt=='\n' )
	    ++cnt;
    if ( cnt==0 )
return( k );

    bdf->prop_cnt += cnt;
    bdf->props = grealloc(bdf->props, bdf->prop_cnt*sizeof( BDFProperties ));

    pt = strchr(bdf->props[k].u.atom,'\n');
    *pt = '\0'; ++pt;
    for ( i=1; i<=cnt; ++i ) {
	start = pt;
	while ( *pt!='\n' && *pt!='\0' ) ++pt;
	bdf->props[k+i].name = copy(bdf->props[k].name);
	bdf->props[k+i].type = bdf->props[k].type;
	bdf->props[k+i].u.atom = copyn(start,pt-start);
	if ( *pt=='\n' ) ++pt;
    }
    pt = copy( bdf->props[k].u.atom );
    free( bdf->props[k].u.atom );
    bdf->props[k].u.atom = pt;
return( k+cnt );
}

void ttf_bdf_read(FILE *ttf,struct ttfinfo *info) {
    int strike_cnt, i,j,k;
    long string_start;
    struct bdfinfo { BDFFont *bdf; int cnt; } *bdfinfo;
    BDFFont *bdf;

    if ( info->bdf_start==0 )
return;
    fseek(ttf,info->bdf_start,SEEK_SET);
    if ( getushort(ttf)!=1 )
return;
    strike_cnt = getushort(ttf);
    string_start = getlong(ttf) + info->bdf_start;

    bdfinfo = galloc(strike_cnt*sizeof(struct bdfinfo));
    for ( i=0; i<strike_cnt; ++i ) {
	int ppem, num_items;
	ppem = getushort(ttf);
	num_items = getushort(ttf);
	for ( bdf=info->bitmaps; bdf!=NULL; bdf=bdf->next )
	    if ( bdf->pixelsize==ppem )
	break;
	bdfinfo[i].bdf = bdf;
	bdfinfo[i].cnt = num_items;
    }

    for ( i=0; i<strike_cnt; ++i ) {
	if ( (bdf = bdfinfo[i].bdf) ==NULL )
	    fseek(ttf,10*bdfinfo[i].cnt,SEEK_CUR);
	else {
	    bdf->prop_cnt = bdfinfo[i].cnt;
	    bdf->props = galloc(bdf->prop_cnt*sizeof(BDFProperties));
	    for ( j=k=0; j<bdfinfo[i].cnt; ++j, ++k ) {
		long name = getlong(ttf);
		int type = getushort(ttf);
		long value = getlong(ttf);
		bdf->props[k].type = type;
		bdf->props[k].name = getstring(ttf,string_start+name);
		switch ( type&~prt_property ) {
		  case prt_int: case prt_uint:
		    bdf->props[k].u.val = value;
		    if ( strcmp(bdf->props[k].name,"FONT_ASCENT")==0 &&
			    value<=bdf->pixelsize ) {
			bdf->ascent = value;
			bdf->descent = bdf->pixelsize-value;
		    }
		  break;
		  case prt_string: case prt_atom:
		    bdf->props[k].u.str = getstring(ttf,string_start+value);
		    k = CheckForNewlines(bdf,k);
		  break;
		}
	    }
	}
    }
}


/* ************************************************************************** */
/* *************************    The 'FFTM' table    ************************* */
/* *************************         Output         ************************* */
/* ************************************************************************** */

/* FontForge timestamp table */
/* Contains: */
/*  date of fontforge sources */
/*  date of font's (not file's) creation */
/*  date of font's modification */
int ttf_fftm_dump(SplineFont *sf,struct alltabs *at) {
    int32 results[2];
    extern const time_t source_modtime;

    at->fftmf = tmpfile();

    putlong(at->fftmf,0x00000001);	/* Version */

    cvt_unix_to_1904(source_modtime,results);
    putlong(at->fftmf,results[1]);
    putlong(at->fftmf,results[0]);

    cvt_unix_to_1904(sf->creationtime,results);
    putlong(at->fftmf,results[1]);
    putlong(at->fftmf,results[0]);

    cvt_unix_to_1904(sf->modificationtime,results);
    putlong(at->fftmf,results[1]);
    putlong(at->fftmf,results[0]);

    at->fftmlen = ftell(at->fftmf);	/* had better be 7*4 */
	    /* It will never be misaligned */
    if ( (at->fftmlen&1)!=0 )
	putc(0,at->fftmf);
    if ( ((at->fftmlen+1)&2)!=0 )
	putshort(at->fftmf,0);
return( true );
}
