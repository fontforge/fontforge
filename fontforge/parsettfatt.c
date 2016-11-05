/* Copyright (C) 2000-2012 by George Williams */
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

#include "parsettfatt.h"

#include "fontforge.h"
#include "lookups.h"
#include "mem.h"
#include "parsettf.h"
#include "splineutil.h"
#include <chardata.h>
#include <utype.h>
#include <ustring.h>
#include <math.h>
#include <locale.h>
#include <stdlib.h>
#include <ggadget.h>		/* For GTextInfo */
#include "ttf.h"

static uint16 *getAppleClassTable(FILE *ttf, int classdef_offset, int cnt, int sub, int div, struct ttfinfo *info) {
    uint16 *class = calloc(cnt,sizeof(uint16));
    int first, i, n;
    /* Apple stores its class tables as containing offsets. I find it hard to */
    /*  think that way and convert them to indeces (by subtracting off a base */
    /*  offset and dividing by the item's size) before doing anything else */

    fseek(ttf,classdef_offset,SEEK_SET);
    first = getushort(ttf);
    n = getushort(ttf);
    if ( first+n-1>=cnt ) {
	LogError( _("Bad Apple Kern Class\n") );
	info->bad_gx = true;
    }
    for ( i=0; i<n && i+first<cnt; ++i )
	class[first+i] = (getushort(ttf)-sub)/div;
return( class );
}

static char **ClassToNames(struct ttfinfo *info,int class_cnt,uint16 *class,int glyph_cnt) {
    char **ret = malloc(class_cnt*sizeof(char *));
    int *lens = calloc(class_cnt,sizeof(int));
    int i;

    ret[0] = NULL;
    for ( i=0 ; i<glyph_cnt; ++i ) if ( class[i]!=0 && info->chars[i]!=NULL && class[i]<class_cnt )
	lens[class[i]] += strlen(info->chars[i]->name)+1;
    for ( i=1; i<class_cnt ; ++i )
	ret[i] = malloc(lens[i]+1);
    memset(lens,0,class_cnt*sizeof(int));
    for ( i=0 ; i<glyph_cnt; ++i ) if ( class[i]!=0 && info->chars[i]!=NULL ) {
	if ( class[i]<class_cnt ) {
	    strcpy(ret[class[i]]+lens[class[i]], info->chars[i]->name );
	    lens[class[i]] += strlen(info->chars[i]->name)+1;
	    ret[class[i]][lens[class[i]]-1] = ' ';
	} else {
	    LogError( _("Class index out of range %d (must be <%d)\n"),class[i], class_cnt );
	    info->bad_ot = true;
	}
    }
    for ( i=1; i<class_cnt ; ++i )
	if ( lens[i]==0 )
	    ret[i][0] = '\0';
	else
	    ret[i][lens[i]-1] = '\0';
    free(lens);
return( ret );
}

static char *CoverageMinusClasses(uint16 *coverageglyphs,uint16 *classed,
	struct ttfinfo *info ) {
    int i, j, len;
    uint8 *glyphs = calloc(info->glyph_cnt,1);
    char *ret;

    for ( i=0; coverageglyphs[i]!=0xffff; ++i )
	glyphs[coverageglyphs[i]] = 1;
    for ( i=0; i<info->glyph_cnt; ++i )
	if ( classed[i]!=0 )
	    glyphs[i] = 0;
    for ( i=0; i<info->glyph_cnt; ++i )
	if ( glyphs[i]!=0 )
    break;
    /* coverage table matches glyphs in classes. No need for special treatment*/
    if ( i==info->glyph_cnt ) {
	free(glyphs);
return( NULL );
    }
    /* Otherwise we need to generate a class string of glyph names in the coverage */
    /*  table but not in any class. These become the glyphs in class 0 */
    ret = NULL;
    for ( j=0; j<2; ++j ) {
	len = 0;
	for ( i=0; i<info->glyph_cnt; ++i ) {
	    if ( glyphs[i]!=0 ) {
		if ( j ) {
		    strcpy( ret+len, info->chars[i]->name );
		    strcat( ret+len, " ");
		}
		len += strlen(info->chars[i]->name)+1;
	    }
	}
	if ( j==0 )
	    ret = malloc(len+1);
	else
	    ret[len-1] = '\0';
    }
    free(glyphs);
return( ret );
}

static int ClassFindCnt(uint16 *class,int tot) {
    int i, max=0;

    for ( i=0; i<tot; ++i )
	if ( class[i]>max ) max = class[i];
return( max+1 );
}

static int cmpuint16(const void *u1, const void *u2) {
return( ((int) *((const uint16 *) u1)) - ((int) *((const uint16 *) u2)) );
}

static char *GlyphsToNames(struct ttfinfo *info,uint16 *glyphs,int make_uniq) {
    int i, j, len, off;
    char *ret, *pt;

    if ( glyphs==NULL )
return( copy(""));

    /* Adobe produces coverage tables containing duplicate glyphs in */
    /*  GaramondPremrPro.otf. We want unique glyphs, so enforce that */
    if ( make_uniq ) {
	for ( i=0 ; glyphs[i]!=0xffff; ++i );
	qsort(glyphs,i,sizeof(uint16),cmpuint16);
	for ( i=0; glyphs[i]!=0xffff; ++i ) {
	    if ( glyphs[i+1]==glyphs[i] ) {
		for ( j=i+1; glyphs[j]==glyphs[i]; ++j );
		off = j-i -1;
		for ( j=i+1; ; ++j ) {
		    glyphs[j] = glyphs[j+off];
		    if ( glyphs[j]==0xffff )
		break;
		}
	    }
	}
    }

    for ( i=len=0 ; glyphs[i]!=0xffff; ++i ) {
	if ( glyphs[i]>=info->glyph_cnt ) {
	    if ( !info->bad_ot ) {
		LogError( _("GID out of range.\n") );
		info->bad_ot = true;
	    }
return( copy(""));
	}
	if ( info->chars[glyphs[i]]!=NULL )
	    len += strlen(info->chars[glyphs[i]]->name)+1;
    }
    ret = pt = malloc(len+1); *pt = '\0';
    for ( i=0 ; glyphs[i]!=0xffff; ++i ) if ( info->chars[glyphs[i]]!=NULL ) {
	strcpy(pt,info->chars[glyphs[i]]->name);
	pt += strlen(pt);
	*pt++ = ' ';
    }
    if ( pt>ret ) pt[-1] = '\0';
return( ret );
}

struct language {	/* normal entry with lang tag 'dflt' */
    uint32 tag;
    uint32 offset;
    uint16 req;		/* required feature index. 0xffff for null */
    int fcnt;
    uint16 *features;
};

struct scripts {
    uint32 offset;
    uint32 tag;
    int langcnt;		/* the default language is included as a */
    struct language *languages;
};

struct feature {
    uint32 offset;
    uint32 tag;
    int lcnt;
    uint16 *lookups;
};

struct lookup {
    uint16 type;
    uint32 flags;	/* low order 16 bits: traditional flags, high order: mark filtering set index if pst_usemarkfilteringset */
    uint32 offset;
    int subtabcnt;
    int32 *subtab_offsets;
    OTLookup *otlookup;
};

static uint16 *getCoverageTable(FILE *ttf, int coverage_offset, struct ttfinfo *info) {
    int format, cnt, i,j, rcnt;
    uint16 *glyphs=NULL;
    int start, end, ind, max;

    fseek(ttf,coverage_offset,SEEK_SET);
    format = getushort(ttf);
    if ( format==1 ) {
	cnt = getushort(ttf);
	glyphs = malloc((cnt+1)*sizeof(uint16));
	if ( ftell(ttf)+2*cnt > info->g_bounds ) {
	    LogError( _("coverage table extends beyond end of table\n") );
	    info->bad_ot = true;
	    if ( ftell(ttf)>info->g_bounds ) {
            free(glyphs);
return( NULL );
        }
	    cnt = (info->g_bounds-ftell(ttf))/2;
	}
	for ( i=0; i<cnt; ++i ) {
	    if ( cnt&0xffff0000 ) {
		LogError( _("Bad count.\n"));
		info->bad_ot = true;
	    }
	    glyphs[i] = getushort(ttf);
	    if ( feof(ttf) ) {
		LogError( _("End of file found in coverage table.\n") );
		info->bad_ot = true;
		free(glyphs);
return( NULL );
	    }
	    if ( glyphs[i]>=info->glyph_cnt ) {
		LogError( _("Bad coverage table. Glyph %d out of range [0,%d)\n"), glyphs[i], info->glyph_cnt );
		info->bad_ot = true;
		glyphs[i] = 0;
	    }
	}
    } else if ( format==2 ) {
	glyphs = calloc((max=256),sizeof(uint16));
	rcnt = getushort(ttf); cnt = 0;
	if ( ftell(ttf)+6*rcnt > info->g_bounds ) {
	    LogError( _("coverage table extends beyond end of table\n") );
	    info->bad_ot = true;
	    rcnt = (info->g_bounds-ftell(ttf))/6;
	}

	for ( i=0; i<rcnt; ++i ) {
	    start = getushort(ttf);
	    end = getushort(ttf);
	    ind = getushort(ttf);
	    if ( feof(ttf) ) {
		LogError( _("End of file found in coverage table.\n") );
		info->bad_ot = true;
		free(glyphs);
return( NULL );
	    }
	    if ( start>end || end>=info->glyph_cnt ) {
		LogError( _("Bad coverage table. Glyph range %d-%d out of range [0,%d)\n"), start, end, info->glyph_cnt );
		info->bad_ot = true;
		start = end = 0;
	    }
	    if ( ind+end-start+2 >= max ) {
		int oldmax = max;
		max = ind+end-start+2;
		glyphs = realloc(glyphs,max*sizeof(uint16));
		memset(glyphs+oldmax,0,(max-oldmax)*sizeof(uint16));
	    }
	    for ( j=start; j<=end; ++j ) {
		glyphs[j-start+ind] = j;
		if ( j>=info->glyph_cnt )
		    glyphs[j-start+ind] = 0;
	    }
	    if ( ind+end-start+1>cnt )
		cnt = ind+end-start+1;
	}
    } else {
	LogError( _("Bad format for coverage table %d\n"), format );
	info->bad_ot = true;
return( NULL );
    }
    glyphs[cnt] = 0xffff;

return( glyphs );
}

struct valuerecord {
    int16 xplacement, yplacement;
    int16 xadvance, yadvance;
    uint16 offXplaceDev, offYplaceDev;
    uint16 offXadvanceDev, offYadvanceDev;
};

static uint16 *getClassDefTable(FILE *ttf, int classdef_offset, struct ttfinfo *info) {
    int format, i, j;
    uint16 start, glyphcnt, rangecnt, end, class;
    uint16 *glist=NULL;
    int warned = false;
    int cnt = info->glyph_cnt;
    uint32 g_bounds = info->g_bounds;

    fseek(ttf, classdef_offset, SEEK_SET);
    glist = calloc(cnt,sizeof(uint16));	/* Class 0 is default */
    format = getushort(ttf);
    if ( format==1 ) {
	start = getushort(ttf);
	glyphcnt = getushort(ttf);
	if ( start+(int) glyphcnt>cnt ) {
	    LogError( _("Bad class def table. start=%d cnt=%d, max glyph=%d\n"), start, glyphcnt, cnt );
	    info->bad_ot = true;
	    glyphcnt = cnt-start;
	} else if ( ftell(ttf)+2*glyphcnt > g_bounds ) {
	    LogError( _("Class definition sub-table extends beyond end of table\n") );
	    info->bad_ot = true;
	    glyphcnt = (g_bounds-ftell(ttf))/2;
	}
	for ( i=0; i<glyphcnt; ++i )
	    glist[start+i] = getushort(ttf);
    } else if ( format==2 ) {
	rangecnt = getushort(ttf);
	if ( ftell(ttf)+6*rangecnt > g_bounds ) {
	    LogError( _("Class definition sub-table extends beyond end of table\n") );
	    info->bad_ot = true;
	    rangecnt = (g_bounds-ftell(ttf))/6;
	}
	for ( i=0; i<rangecnt; ++i ) {
	    start = getushort(ttf);
	    end = getushort(ttf);
	    if ( start>end || end>=cnt ) {
		LogError( _("Bad class def table. Glyph range %d-%d out of range [0,%d)\n"), start, end, cnt );
		info->bad_ot = true;
	    }
	    class = getushort(ttf);
	    for ( j=start; j<=end; ++j ) if ( j<cnt )
		glist[j] = class;
	}
    } else {
	LogError( _("Unknown class table format: %d\n"), format );
	info->bad_ot = true;
	/* Put everything in class 0 and return that */
    }

    /* Do another validity test */
    for ( i=0; i<cnt; ++i ) {
	if ( glist[i]>=cnt+1 ) {
	    if ( !warned ) {
		LogError( _("Nonsensical class assigned to a glyph-- class=%d is too big. Glyph=%d\n"),
			glist[i], i );
		info->bad_ot = true;
		warned = true;
	    }
	    glist[i] = 0;
	}
    }

return glist;
}

static void readvaluerecord(struct valuerecord *vr,int vf,FILE *ttf) {
    memset(vr,'\0',sizeof(struct valuerecord));
    if ( vf&1 )
	vr->xplacement = getushort(ttf);
    if ( vf&2 )
	vr->yplacement = getushort(ttf);
    if ( vf&4 )
	vr->xadvance = getushort(ttf);
    if ( vf&8 )
	vr->yadvance = getushort(ttf);
    if ( vf&0x10 )
	vr->offXplaceDev = getushort(ttf);
    if ( vf&0x20 )
	vr->offYplaceDev = getushort(ttf);
    if ( vf&0x40 )
	vr->offXadvanceDev = getushort(ttf);
    if ( vf&0x80 )
	vr->offYadvanceDev = getushort(ttf);
}

static void ReadDeviceTable(FILE *ttf,DeviceTable *adjust,uint32 devtab,
	struct ttfinfo *info) {
    long here;
    int pack;
    int w,b,i,c;

    if ( devtab==0 )
return;
    here = ftell(ttf);
    fseek(ttf,devtab,SEEK_SET);
    adjust->first_pixel_size = getushort(ttf);
    adjust->last_pixel_size  = getushort(ttf);
    pack = getushort(ttf);
    if ( adjust->first_pixel_size>adjust->last_pixel_size || pack==0 || pack>3 ) {
	LogError(_("Bad device table\n" ));
	info->bad_ot = true;
	adjust->first_pixel_size = adjust->last_pixel_size = 0;
    } else {
	c = adjust->last_pixel_size-adjust->first_pixel_size+1;
	adjust->corrections = malloc(c);
	if ( pack==1 ) {
	    for ( i=0; i<c; i+=8 ) {
		w = getushort(ttf);
		for ( b=0; b<8 && i+b<c; ++b )
		    adjust->corrections[i+b] = ((int16) ((w<<(b*2))&0xc000))>>14;
	    }
	} else if ( pack==2 ) {
	    for ( i=0; i<c; i+=4 ) {
		w = getushort(ttf);
		for ( b=0; b<4 && i+b<c; ++b )
		    adjust->corrections[i+b] = ((int16) ((w<<(b*4))&0xf000))>>12;
	    }
	} else {
	    for ( i=0; i<c; ++i )
		adjust->corrections[i] = (int8) getc(ttf);
	}
    }
    fseek(ttf,here,SEEK_SET);
}

static ValDevTab *readValDevTab(FILE *ttf,struct valuerecord *vr,uint32 base,
	struct ttfinfo *info) {
    ValDevTab *ret;

    if ( vr->offXplaceDev==0 && vr->offYplaceDev==0 &&
	    vr->offXadvanceDev==0 && vr->offYadvanceDev==0 )
return( NULL );
    ret = chunkalloc(sizeof(ValDevTab));
    if ( vr->offXplaceDev!=0 )
	ReadDeviceTable(ttf,&ret->xadjust,base + vr->offXplaceDev,info);
    if ( vr->offYplaceDev!=0 )
	ReadDeviceTable(ttf,&ret->yadjust,base + vr->offYplaceDev,info);
    if ( vr->offXadvanceDev!=0 )
	ReadDeviceTable(ttf,&ret->xadv,base + vr->offXadvanceDev,info);
    if ( vr->offYadvanceDev!=0 )
	ReadDeviceTable(ttf,&ret->yadv,base + vr->offYadvanceDev,info);
return( ret );
}

static void addPairPos(struct ttfinfo *info, int glyph1, int glyph2,
	struct lookup_subtable *subtable, struct valuerecord *vr1,struct valuerecord *vr2,
	uint32 base,FILE *ttf) {
    
    if ( glyph1<info->glyph_cnt && glyph2<info->glyph_cnt &&
	    info->chars[glyph1]!=NULL && info->chars[glyph2]!=NULL ) {
	PST *pos = chunkalloc(sizeof(PST));
	pos->type = pst_pair;
	pos->subtable = subtable;
	pos->next = info->chars[glyph1]->possub;
	info->chars[glyph1]->possub = pos;
	pos->u.pair.vr = chunkalloc(sizeof(struct vr [2]));
	pos->u.pair.paired = copy(info->chars[glyph2]->name);
	pos->u.pair.vr[0].xoff = vr1->xplacement;
	pos->u.pair.vr[0].yoff = vr1->yplacement;
	pos->u.pair.vr[0].h_adv_off = vr1->xadvance;
	pos->u.pair.vr[0].v_adv_off = vr1->yadvance;
	pos->u.pair.vr[1].xoff = vr2->xplacement;
	pos->u.pair.vr[1].yoff = vr2->yplacement;
	pos->u.pair.vr[1].h_adv_off = vr2->xadvance;
	pos->u.pair.vr[1].v_adv_off = vr2->yadvance;
	pos->u.pair.vr[0].adjust = readValDevTab(ttf,vr1,base,info);
	pos->u.pair.vr[1].adjust = readValDevTab(ttf,vr2,base,info);
    } else {
	LogError( _("Bad pair position: glyphs %d & %d should have been < %d\n"),
		glyph1, glyph2, info->glyph_cnt );
	info->bad_ot = true;
    }
}

static int addKernPair(struct ttfinfo *info, int glyph1, int glyph2,
	int16 offset, uint32 devtab, struct lookup_subtable *subtable,int isv,
	FILE *ttf) {
    KernPair *kp;
    if ( glyph1<info->glyph_cnt && glyph2<info->glyph_cnt &&
	    info->chars[glyph1]!=NULL && info->chars[glyph2]!=NULL ) {
	for ( kp=isv ? info->chars[glyph1]->vkerns : info->chars[glyph1]->kerns;
		kp!=NULL; kp=kp->next ) {
	    if ( kp->sc == info->chars[glyph2] )
	break;
	}
	if ( kp==NULL ) {
	    kp = chunkalloc(sizeof(KernPair));
	    kp->sc = info->chars[glyph2];
	    kp->off = offset;
	    kp->subtable = subtable;
	    if ( devtab!=0 ) {
		kp->adjust = chunkalloc(sizeof(DeviceTable));
		ReadDeviceTable(ttf,kp->adjust,devtab,info);
	    }
	    if ( isv ) {
		kp->next = info->chars[glyph1]->vkerns;
		info->chars[glyph1]->vkerns = kp;
	    } else {
		kp->next = info->chars[glyph1]->kerns;
		info->chars[glyph1]->kerns = kp;
	    }
	} else if ( kp->subtable!=subtable )
return( true );
    } else if ( glyph1>=info->glyph_cnt || glyph2>=info->glyph_cnt ) {
	/* Might be NULL in a ttc file where we omit glyphs */
	LogError( _("Bad kern pair: glyphs %d & %d should have been < %d\n"),
		glyph1, glyph2, info->glyph_cnt );
	info->bad_ot = true;
    }
return( false );
}

static void gposKernSubTable(FILE *ttf, int stoffset, struct ttfinfo *info, struct lookup *l, struct lookup_subtable *subtable) {
    int coverage, cnt, i, j, pair_cnt, vf1, vf2, glyph2;
    int cd1, cd2, c1_cnt, c2_cnt;
    uint16 format;
    uint16 *ps_offsets;
    uint16 *glyphs, *class1, *class2;
    struct valuerecord vr1, vr2;
    long foffset;
    KernClass *kc;
    int isv, r2l;

    format=getushort(ttf);
    if ( format!=1 && format!=2 )	/* Unknown subtable format */
return;
    coverage = getushort(ttf);
    vf1 = getushort(ttf);
    vf2 = getushort(ttf);
    r2l = 0;

    /* Accept forms both with and without device tables */
    if ( (vf1==0x0008 || vf1==0x0088) && vf2==0x0000 )
	isv = 1;		/* Top to bottom */
    else if ( vf1==0x0000 && (vf2==0x0004 || vf2==0x0044) && (l->flags&pst_r2l)) {
	isv = 0;		/* Right to left */
	r2l = 1;
    } else if ( (vf1==0x0004 || vf1==0x0044) && vf2==0x0000 && !(l->flags&pst_r2l) )
	isv = 0;		/* Left to right */
    else
	isv = 2;		/* Can't optimize, store all 8 settings */
    if ( format==1 ) {
	subtable->per_glyph_pst_or_kern = true;
	cnt = getushort(ttf);
	ps_offsets = malloc(cnt*sizeof(uint16));
	for ( i=0; i<cnt; ++i )
	    ps_offsets[i]=getushort(ttf);
	glyphs = getCoverageTable(ttf,stoffset+coverage,info);
	if ( glyphs==NULL ) {
/* GT: This continues a multi-line error message, hence the leading space */
	    LogError( _(" Bad pairwise kerning table, ignored\n") );
	    free(ps_offsets);
return;
	}
	for ( i=0; i<cnt; ++i ) if ( glyphs[i]<info->glyph_cnt ) {
	    fseek(ttf,stoffset+ps_offsets[i],SEEK_SET);
	    pair_cnt = getushort(ttf);
	    for ( j=0; j<pair_cnt; ++j ) {
		glyph2 = getushort(ttf);
		readvaluerecord(&vr1,vf1,ttf);
		readvaluerecord(&vr2,vf2,ttf);
		if ( isv==2 )
		    addPairPos(info, glyphs[i], glyph2,subtable,&vr1,&vr2, stoffset,ttf);
		else if ( isv ) {
		    if ( addKernPair(info, glyphs[i], glyph2, vr1.yadvance,
			    vr1.offYadvanceDev==0?0:stoffset+vr1.offYadvanceDev,
			    subtable,isv,ttf))
			addPairPos(info, glyphs[i], glyph2,subtable,&vr1,&vr2, stoffset,ttf);
			/* If we've already got kern data for this pair of */
			/*  glyphs, then we can't make it be a true KernPair */
			/*  but we can save the info as a pst_pair */
		} else if ( r2l ) {	/* R2L */
		    if ( addKernPair(info, glyphs[i], glyph2, vr2.xadvance,
			    vr2.offXadvanceDev==0?0:stoffset+vr2.offXadvanceDev,
			    subtable,isv,ttf))
			addPairPos(info, glyphs[i], glyph2,subtable,&vr1,&vr2,stoffset,ttf);
		} else {
		    if ( addKernPair(info, glyphs[i], glyph2, vr1.xadvance,
			    vr1.offXadvanceDev==0?0:stoffset+vr1.offXadvanceDev,
			    subtable,isv,ttf))
			addPairPos(info, glyphs[i], glyph2,subtable,&vr1,&vr2,stoffset,ttf);
		}
	    }
	}
	free(ps_offsets); free(glyphs);
    } else if ( format==2 ) {	/* Class-based kerning */
	cd1 = getushort(ttf);
	cd2 = getushort(ttf);
	foffset = ftell(ttf);
	class1 = getClassDefTable(ttf, stoffset+cd1, info);
	class2 = getClassDefTable(ttf, stoffset+cd2, info);
	glyphs = getCoverageTable(ttf,stoffset+coverage,info);
	if ( glyphs==NULL ) {
/* GT: This continues a multi-line error message, hence the leading space */
	    LogError( _(" Bad kerning class table, ignored\n") );
        free(class1);
        free(class2);
return;
	}
	fseek(ttf, foffset, SEEK_SET);	/* come back */
	c1_cnt = getushort(ttf);
	c2_cnt = getushort(ttf);
	if ( isv!=2 ) {
	    if ( isv ) {
		if ( info->vkhead==NULL )
		    info->vkhead = kc = chunkalloc(sizeof(KernClass));
		else
		    kc = info->vklast->next = chunkalloc(sizeof(KernClass));
		info->vklast = kc;
	    } else {
		if ( info->khead==NULL )
		    info->khead = kc = chunkalloc(sizeof(KernClass));
		else
		    kc = info->klast->next = chunkalloc(sizeof(KernClass));
		info->klast = kc;
	    }
	    subtable->vertical_kerning = isv;
	    subtable->kc = kc;
	    kc->first_cnt = c1_cnt; kc->second_cnt = c2_cnt;
	    kc->subtable = subtable;
	    kc->offsets = malloc(c1_cnt*c2_cnt*sizeof(int16));
	    kc->adjusts = calloc(c1_cnt*c2_cnt,sizeof(DeviceTable));
	    kc->firsts = ClassToNames(info,c1_cnt,class1,info->glyph_cnt);
	    kc->seconds = ClassToNames(info,c2_cnt,class2,info->glyph_cnt);
	    /* Now if the coverage table contains entries which are not in */
	    /*  the list of first classes, then those glyphs are the real */
	    /*  values for kc->firsts[0] */
	    kc->firsts[0] = CoverageMinusClasses(glyphs,class1,info);
	    for ( i=0; i<c1_cnt; ++i) {
		for ( j=0; j<c2_cnt; ++j) {
		    readvaluerecord(&vr1,vf1,ttf);
		    readvaluerecord(&vr2,vf2,ttf);
		    if ( isv )
			kc->offsets[i*c2_cnt+j] = vr1.yadvance;
		    else if ( r2l )
			kc->offsets[i*c2_cnt+j] = vr2.xadvance;
		    else
			kc->offsets[i*c2_cnt+j] = vr1.xadvance;
		    if ( isv ) {
			if ( vr1.offYadvanceDev!=0 )
			    ReadDeviceTable(ttf,&kc->adjusts[i*c2_cnt+j],stoffset+vr1.offYadvanceDev,info);
		    } else if ( r2l ) {
			if ( vr2.offXadvanceDev!=0 )
			    ReadDeviceTable(ttf,&kc->adjusts[i*c2_cnt+j],stoffset+vr2.offXadvanceDev,info);
		    } else {
			if ( vr1.offXadvanceDev!=0 )
			    ReadDeviceTable(ttf,&kc->adjusts[i*c2_cnt+j],stoffset+vr1.offXadvanceDev,info);
		    }
		}
	    }
	} else {	/* This happens when we have a feature which is neither 'kern' nor 'vkrn' we don't know what to do with it so we make it into kern pairs */
	    int k,m;
	    subtable->per_glyph_pst_or_kern = true;
	    for ( i=0; i<c1_cnt; ++i) {
		for ( j=0; j<c2_cnt; ++j) {
		    readvaluerecord(&vr1,vf1,ttf);
		    readvaluerecord(&vr2,vf2,ttf);
		    if ( vr1.xadvance!=0 || vr1.xplacement!=0 || vr1.yadvance!=0 || vr1.yplacement!=0 ||
			    vr2.xadvance!=0 || vr2.xplacement!=0 || vr2.yadvance!=0 || vr2.yplacement!=0 )
			for ( k=0; k<info->glyph_cnt; ++k )
			    if ( class1[k]==i )
				for ( m=0; m<info->glyph_cnt; ++m )
				    if ( class2[m]==j )
					addPairPos(info, k,m,subtable,&vr1,&vr2,stoffset,ttf);
		}
	    }
	}
	free(class1); free(class2);
	free(glyphs);
    }
}

static AnchorPoint *readAnchorPoint(FILE *ttf,uint32 base,AnchorClass *class,
	enum anchor_type type,AnchorPoint *last, struct ttfinfo *info) {
    AnchorPoint *ap;
    int format;

    fseek(ttf,base,SEEK_SET);

    ap = chunkalloc(sizeof(AnchorPoint));
    ap->anchor = class;
    /* All anchor types have the same initial 3 entries, format */
    /*  x,y pos. format 2 contains a truetype positioning point, and */
    /*  format==3 may also have device tables */
    format = getushort(ttf);
    ap->me.x = (int16) getushort(ttf);
    ap->me.y = (int16) getushort(ttf);
    ap->type = type;
    if ( format==2 ) {
	ap->ttf_pt_index = getushort(ttf);
	ap->has_ttf_pt = true;
    }
    else if ( format==3 ) {
	int devoff;
	devoff = getushort(ttf);
	if ( devoff!=0 )
	    ReadDeviceTable(ttf,&ap->xadjust,base+devoff,info);
	devoff = getushort(ttf);
	if ( devoff!=0 )
	    ReadDeviceTable(ttf,&ap->yadjust,base+devoff,info);
    }
    ap->next = last;
return( ap );
}

static void gposCursiveSubTable(FILE *ttf, int stoffset, struct ttfinfo *info,struct lookup_subtable *subtable) {
    int coverage, cnt, format, i;
    struct ee_offsets { int entry, exit; } *offsets;
    uint16 *glyphs;
    AnchorClass *class;
    SplineChar *sc;
    char buf[50];

    format=getushort(ttf);
    if ( format!=1 )	/* Unknown subtable format */
return;
    coverage = getushort(ttf);
    cnt = getushort(ttf);
    if ( cnt==0 )
return;
    offsets = malloc(cnt*sizeof(struct ee_offsets));
    for ( i=0; i<cnt; ++i ) {
	offsets[i].entry = getushort(ttf);
	offsets[i].exit  = getushort(ttf);
    }
    glyphs = getCoverageTable(ttf,stoffset+coverage,info);
    if ( glyphs==NULL ) {
/* GT: This continues a multi-line error message, hence the leading space */
	LogError( _(" Bad cursive alignment table, ignored\n") );
	free(offsets);
return;
    }

    class = chunkalloc(sizeof(AnchorClass));
    snprintf(buf,sizeof(buf),_("Cursive-%d"),
	    info->anchor_class_cnt++ );
    class->name = copy(buf);
    subtable->anchor_classes = true;
    class->subtable = subtable;
    class->type = act_curs;
    if ( info->ahead==NULL )
	info->ahead = class;
    else
	info->alast->next = class;
    info->alast = class;

    for ( i=0; i<cnt; ++i ) {
	sc = info->chars[glyphs[i]];
	if ( offsets[i].entry!=0 ) {
	    sc->anchor = readAnchorPoint(ttf,stoffset+offsets[i].entry,class,
		    at_centry,sc->anchor,info);
	}
	if ( offsets[i].exit!=0 ) {
	    sc->anchor = readAnchorPoint(ttf,stoffset+offsets[i].exit,class,
		    at_cexit,sc->anchor,info);
	}
    }
    free(offsets);
    free(glyphs);
}

static AnchorClass **MarkGlyphsProcessMarks(FILE *ttf,int markoffset,
	struct ttfinfo *info,struct lookup *l, struct lookup_subtable *subtable,uint16 *markglyphs,
	int classcnt) {
    AnchorClass **classes = calloc(classcnt,sizeof(AnchorClass *)), *ac;
    char buf[50];
    int i, cnt;
    struct mr { uint16 class, offset; } *at_offsets;
    SplineChar *sc;

    fseek(ttf,markoffset,SEEK_SET);
    cnt = getushort(ttf);
    if ( feof(ttf) ) {
	LogError( _("Bad mark table.\n") );
	info->bad_ot = true;
return( NULL );
    }

    for ( i=0; i<classcnt; ++i ) {
	snprintf(buf,sizeof(buf),_("Anchor-%d"),
		info->anchor_class_cnt+i );
	classes[i] = ac = chunkalloc(sizeof(AnchorClass));
	ac->name = copy(buf);
	subtable->anchor_classes = true;
	ac->subtable = subtable;
	/*ac->merge_with = info->anchor_merge_cnt+1;*/
	ac->type = l->otlookup->lookup_type==gpos_mark2mark ? act_mkmk : act_mark;
	    /* I don't distinguish between mark to base and mark to lig */
	if ( info->ahead==NULL )
	    info->ahead = ac;
	else
	    info->alast->next = ac;
	info->alast = ac;
    }

    at_offsets = malloc(cnt*sizeof(struct mr));
    for ( i=0; i<cnt; ++i ) {
	at_offsets[i].class = getushort(ttf);
	at_offsets[i].offset = getushort(ttf);
	if ( at_offsets[i].class>=classcnt ) {
	    at_offsets[i].class = 0;
	    if ( markglyphs[i]>=info->glyph_cnt )
		LogError( _("Class out of bounds in GPOS mark sub-table\n") );
	    else
		LogError( _("Class out of bounds in GPOS mark sub-table for mark %.30s\n"), info->chars[markglyphs[i]]->name);
	    info->bad_ot = true;
	}
    }
    for ( i=0; i<cnt; ++i ) {
	if ( markglyphs[i]>=info->glyph_cnt )
    continue;
	sc = info->chars[markglyphs[i]];
	if ( sc==NULL || at_offsets[i].offset==0 )
    continue;
	sc->anchor = readAnchorPoint(ttf,markoffset+at_offsets[i].offset,
		classes[at_offsets[i].class],at_mark,sc->anchor,info);
    }
    free(at_offsets);
return( classes );
}

static void MarkGlyphsProcessBases(FILE *ttf,int baseoffset,
	struct ttfinfo *info,struct lookup *l, struct lookup_subtable *subtable,uint16 *baseglyphs,int classcnt,
	AnchorClass **classes,enum anchor_type at) {
    int basecnt,i, j, ibase;
    uint16 *offsets;
    SplineChar *sc;

    fseek(ttf,baseoffset,SEEK_SET);
    basecnt = getushort(ttf);
    if ( feof(ttf) ) {
	LogError( _("Bad base table.\n") );
	info->bad_ot = true;
return;
    }
    offsets = malloc(basecnt*classcnt*sizeof(uint16));
    for ( i=0; i<basecnt*classcnt; ++i )
	offsets[i] = getushort(ttf);
    for ( i=ibase=0; i<basecnt; ++i, ibase+= classcnt ) {
	if ( baseglyphs[i]>=info->glyph_cnt )
    continue;
	sc = info->chars[baseglyphs[i]];
	if ( sc==NULL )
    continue;
	for ( j=0; j<classcnt; ++j ) if ( offsets[ibase+j]!=0 ) {
	    sc->anchor = readAnchorPoint(ttf,baseoffset+offsets[ibase+j],
		    classes[j], at,sc->anchor,info);
	}
    }
    free(offsets);
}

static void MarkGlyphsProcessLigs(FILE *ttf,int baseoffset,
	struct ttfinfo *info,struct lookup *l, struct lookup_subtable *subtable,uint16 *baseglyphs,int classcnt,
	AnchorClass **classes) {
    int basecnt,compcnt, i, j, k, kbase;
    uint16 *loffsets, *aoffsets;
    SplineChar *sc;

    fseek(ttf,baseoffset,SEEK_SET);
    basecnt = getushort(ttf);
    if ( feof(ttf) ) {
	LogError( _("Bad ligature base table.\n") );
	info->bad_ot = true;
return;
    }
    loffsets = malloc(basecnt*sizeof(uint16));
    for ( i=0; i<basecnt; ++i )
	loffsets[i] = getushort(ttf);
    for ( i=0; i<basecnt; ++i ) {
	sc = info->chars[baseglyphs[i]];
	if ( baseglyphs[i]>=info->glyph_cnt || sc==NULL )
    continue;
	fseek(ttf,baseoffset+loffsets[i],SEEK_SET);
	compcnt = getushort(ttf);
	if ( feof(ttf)) {
	    LogError(_("Bad ligature anchor count.\n"));
	    info->bad_ot = true;
    continue;
	}
	aoffsets = malloc(compcnt*classcnt*sizeof(uint16));
	for ( k=0; k<compcnt*classcnt; ++k )
	    aoffsets[k] = getushort(ttf);
	for ( k=kbase=0; k<compcnt; ++k, kbase+=classcnt ) {
	    for ( j=0; j<classcnt; ++j ) if ( aoffsets[kbase+j]!=0 ) {
		sc->anchor = readAnchorPoint(ttf,baseoffset+loffsets[i]+aoffsets[kbase+j],
			classes[j], at_baselig,sc->anchor,info);
		sc->anchor->lig_index = k;
	    }
	}
	free(aoffsets);
    }
    free(loffsets);
}

static void gposMarkSubTable(FILE *ttf, uint32 stoffset,
	struct ttfinfo *info, struct lookup *l, struct lookup_subtable *subtable) {
    int markcoverage, basecoverage, classcnt, markoffset, baseoffset;
    uint16 *markglyphs, *baseglyphs;
    AnchorClass **classes;

	/* The header for the three different mark tables is the same */
    /* Type = */ getushort(ttf);
    markcoverage = getushort(ttf);
    basecoverage = getushort(ttf);
    classcnt = getushort(ttf);
    markoffset = getushort(ttf);
    baseoffset = getushort(ttf);
    markglyphs = getCoverageTable(ttf,stoffset+markcoverage,info);
    baseglyphs = getCoverageTable(ttf,stoffset+basecoverage,info);
    if ( baseglyphs==NULL || markglyphs==NULL ) {
	free(baseglyphs); free(markglyphs);
	LogError( _(" Bad mark attachment table, ignored\n") );
return;
    }
	/* as is the (first) mark table */
    classes = MarkGlyphsProcessMarks(ttf,stoffset+markoffset,
	    info,l,subtable,markglyphs,classcnt);
    if ( classes==NULL ) {
        free(baseglyphs);
        free(markglyphs);
return;
    }
    switch ( l->otlookup->lookup_type ) {
      case gpos_mark2base:
      case gpos_mark2mark:
	  MarkGlyphsProcessBases(ttf,stoffset+baseoffset,
	    info,l,subtable,baseglyphs,classcnt,classes,
	    l->otlookup->lookup_type==gpos_mark2base?at_basechar:at_basemark);
      break;
      case gpos_mark2ligature:
	  MarkGlyphsProcessLigs(ttf,stoffset+baseoffset,
	    info,l,subtable,baseglyphs,classcnt,classes);
      break;
    }
    info->anchor_class_cnt += classcnt;
    ++ info->anchor_merge_cnt;
    free(markglyphs); free(baseglyphs);
    free(classes);
}

static void gposSimplePos(FILE *ttf, int stoffset, struct ttfinfo *info,
	struct lookup *l, struct lookup_subtable *subtable) {
    int coverage, cnt, i, vf;
    uint16 format;
    uint16 *glyphs;
    struct valuerecord *vr=NULL, _vr, *which;

    format=getushort(ttf);
    if ( format!=1 && format!=2 )	/* Unknown subtable format */
return;
    coverage = getushort(ttf);
    vf = getushort(ttf);
    if ( vf==0 )
return;
    if ( format==1 ) {
	memset(&_vr,0,sizeof(_vr));
	readvaluerecord(&_vr,vf,ttf);
    } else {
	cnt = getushort(ttf);
	vr = calloc(cnt,sizeof(struct valuerecord));
	for ( i=0; i<cnt; ++i )
	    readvaluerecord(&vr[i],vf,ttf);
    }
    glyphs = getCoverageTable(ttf,stoffset+coverage,info);
    if ( glyphs==NULL ) {
	free(vr);
	LogError( _(" Bad simple positioning table, ignored\n") );
return;
    }
    for ( i=0; glyphs[i]!=0xffff; ++i ) if ( glyphs[i]<info->glyph_cnt && info->chars[glyphs[i]]!=NULL ) {
	PST *pos = chunkalloc(sizeof(PST));
	pos->type = pst_position;
	pos->subtable = subtable;
	pos->next = info->chars[glyphs[i]]->possub;
	info->chars[glyphs[i]]->possub = pos;
	which = format==1 ? &_vr : &vr[i];
	pos->u.pos.xoff = which->xplacement;
	pos->u.pos.yoff = which->yplacement;
	pos->u.pos.h_adv_off = which->xadvance;
	pos->u.pos.v_adv_off = which->yadvance;
	pos->u.pos.adjust = readValDevTab(ttf,which,stoffset,info);
    }
    subtable->per_glyph_pst_or_kern = true;
    free(vr);
    free(glyphs);
}

static void ProcessSubLookups(FILE *ttf,struct ttfinfo *info,int gpos,
	struct lookup *alllooks,struct seqlookup *sl) {
    int i;

    i = (intpt) sl->lookup;
    if ( i<0 || i>=info->lookup_cnt ) {
	LogError( _("Attempt to reference lookup %d (within a contextual lookup), but there are\n only %d lookups in %s\n"),
		i, info->lookup_cnt, gpos ? "'GPOS'" : "'GSUB'" );
	info->bad_ot = true;
	sl->lookup = NULL;
return;
    }
    sl->lookup = alllooks[i].otlookup;
}

static void g___ContextSubTable1(FILE *ttf, int stoffset,
	struct ttfinfo *info, struct lookup *l, struct lookup_subtable *subtable, int justinuse,
	struct lookup *alllooks, int gpos) {
    int i, j, k, rcnt, cnt;
    uint16 coverage;
    uint16 *glyphs;
    struct subrule {
	uint32 offset;
	int gcnt;
	int scnt;
	uint16 *glyphs;
	struct seqlookup *sl;
    };
    struct rule {
	uint32 offsets;
	int scnt;
	struct subrule *subrules;
    } *rules;
    FPST *fpst;
    struct fpst_rule *rule;
    int warned = false, warned2 = false;

    coverage = getushort(ttf);
    rcnt = getushort(ttf);		/* glyph count in coverage table */
    rules = malloc(rcnt*sizeof(struct rule));
    for ( i=0; i<rcnt; ++i )
	rules[i].offsets = getushort(ttf)+stoffset;
    glyphs = getCoverageTable(ttf,stoffset+coverage,info);
    if ( glyphs==NULL ) {
/* GT: This continues a multi-line error message, hence the leading space */
	LogError( _(" Bad contextual table, ignored\n") );
        free(rules);
return;
    }
    cnt = 0;
    for ( i=0; i<rcnt; ++i ) {
	fseek(ttf,rules[i].offsets,SEEK_SET);
	rules[i].scnt = getushort(ttf);
	cnt += rules[i].scnt;
	rules[i].subrules = malloc(rules[i].scnt*sizeof(struct subrule));
	for ( j=0; j<rules[i].scnt; ++j )
	    rules[i].subrules[j].offset = getushort(ttf)+rules[i].offsets;
	for ( j=0; j<rules[i].scnt; ++j ) {
	    fseek(ttf,rules[i].subrules[j].offset,SEEK_SET);
	    rules[i].subrules[j].gcnt = getushort(ttf);
	    rules[i].subrules[j].scnt = getushort(ttf);
	    rules[i].subrules[j].glyphs = malloc((rules[i].subrules[j].gcnt+1)*sizeof(uint16));
	    rules[i].subrules[j].glyphs[0] = glyphs[i];
	    for ( k=1; k<rules[i].subrules[j].gcnt; ++k ) {
		rules[i].subrules[j].glyphs[k] = getushort(ttf);
		if ( rules[i].subrules[j].glyphs[k]>=info->glyph_cnt ) {
		    if ( !warned )
			LogError( _("Bad contextual or chaining sub table. Glyph %d out of range [0,%d)\n"),
				 rules[i].subrules[j].glyphs[k], info->glyph_cnt );
		    info->bad_ot = true;
		    warned = true;
		    rules[i].subrules[j].glyphs[k] = 0;
		 }
	    }
	    rules[i].subrules[j].glyphs[k] = 0xffff;
	    rules[i].subrules[j].sl = malloc(rules[i].subrules[j].scnt*sizeof(struct seqlookup));
	    for ( k=0; k<rules[i].subrules[j].scnt; ++k ) {
		rules[i].subrules[j].sl[k].seq = getushort(ttf);
		if ( rules[i].subrules[j].sl[k].seq >= rules[i].subrules[j].gcnt+1 )
		    if ( !warned2 ) {
			LogError( _("Attempt to apply a lookup to a location out of the range of this contextual\n lookup seq=%d max=%d\n"),
				rules[i].subrules[j].sl[k].seq, rules[i].subrules[j].gcnt );
			info->bad_ot = true;
			warned2 = true;
		    }
		rules[i].subrules[j].sl[k].lookup = (void *) (intpt) getushort(ttf);
	    }
	}
    }

    if ( justinuse==git_justinuse ) {
	/* Nothing to do. This lookup doesn't really reference any glyphs */
	/*  any lookups it invokes will be processed on their own */
    } else {
	fpst = chunkalloc(sizeof(FPST));
	fpst->type = gpos ? pst_contextpos : pst_contextsub;
	fpst->format = pst_glyphs;
	fpst->subtable = subtable;
	fpst->next = info->possub;
	info->possub = fpst;
	subtable->fpst = fpst;

	fpst->rules = rule = calloc(cnt,sizeof(struct fpst_rule));
	fpst->rule_cnt = cnt;

	cnt = 0;
	for ( i=0; i<rcnt; ++i ) for ( j=0; j<rules[i].scnt; ++j ) {
	    rule[cnt].u.glyph.names = GlyphsToNames(info,rules[i].subrules[j].glyphs,false);
	    rule[cnt].lookup_cnt = rules[i].subrules[j].scnt;
	    rule[cnt].lookups = rules[i].subrules[j].sl;
	    rules[i].subrules[j].sl = NULL;
	    for ( k=0; k<rule[cnt].lookup_cnt; ++k )
		ProcessSubLookups(ttf,info,gpos,alllooks,&rule[cnt].lookups[k]);
	    ++cnt;
	}
    }

    for ( i=0; i<rcnt; ++i ) {
	for ( j=0; j<rules[i].scnt; ++j ) {
	    free(rules[i].subrules[j].glyphs);
	    free(rules[i].subrules[j].sl);
	}
	free(rules[i].subrules);
    }
    free(rules);
    free(glyphs);
}

static void g___ChainingSubTable1(FILE *ttf, int stoffset,
	struct ttfinfo *info, struct lookup *l, struct lookup_subtable *subtable, int justinuse,
	struct lookup *alllooks, int gpos) {
    int i, j, k, rcnt, cnt, which;
    uint16 coverage;
    uint16 *glyphs;
    struct subrule {
	uint32 offset;
	int gcnt, bcnt, fcnt;
	int scnt;
	uint16 *glyphs, *bglyphs, *fglyphs;
	struct seqlookup *sl;
    };
    struct rule {
	uint32 offsets;
	int scnt;
	struct subrule *subrules;
    } *rules;
    FPST *fpst;
    struct fpst_rule *rule;
    int warned = false, warned2 = false;

    coverage = getushort(ttf);
    rcnt = getushort(ttf);		/* glyph count in coverage table */
    rules = malloc(rcnt*sizeof(struct rule));
    for ( i=0; i<rcnt; ++i )
	rules[i].offsets = getushort(ttf)+stoffset;
    glyphs = getCoverageTable(ttf,stoffset+coverage,info);
    if ( glyphs==NULL ) {
	free(rules);
	LogError( _(" Bad contextual chaining table, ignored\n") );
return;
    }
    cnt = 0;
    for ( i=0; i<rcnt; ++i ) {
	fseek(ttf,rules[i].offsets,SEEK_SET);
	rules[i].scnt = getushort(ttf);
	cnt += rules[i].scnt;
	rules[i].subrules = malloc(rules[i].scnt*sizeof(struct subrule));
	for ( j=0; j<rules[i].scnt; ++j )
	    rules[i].subrules[j].offset = getushort(ttf)+rules[i].offsets;
	for ( j=0; j<rules[i].scnt; ++j ) {
	    fseek(ttf,rules[i].subrules[j].offset,SEEK_SET);
	    rules[i].subrules[j].bcnt = getushort(ttf);
	    if ( feof(ttf)) {
		LogError( _("Unexpected end of file in contextual chaining subtable.\n") );
		info->bad_ot = true;
return;
	    }
	    rules[i].subrules[j].bglyphs = malloc((rules[i].subrules[j].bcnt+1)*sizeof(uint16));
	    for ( k=0; k<rules[i].subrules[j].bcnt; ++k )
		rules[i].subrules[j].bglyphs[k] = getushort(ttf);
	    rules[i].subrules[j].bglyphs[k] = 0xffff;

	    rules[i].subrules[j].gcnt = getushort(ttf);
	    if ( feof(ttf)) {
		LogError( _("Unexpected end of file in contextual chaining subtable.\n") );
		info->bad_ot = true;
return;
	    }
	    rules[i].subrules[j].glyphs = malloc((rules[i].subrules[j].gcnt+1)*sizeof(uint16));
	    rules[i].subrules[j].glyphs[0] = glyphs[i];
	    for ( k=1; k<rules[i].subrules[j].gcnt; ++k )
		rules[i].subrules[j].glyphs[k] = getushort(ttf);
	    rules[i].subrules[j].glyphs[k] = 0xffff;

	    rules[i].subrules[j].fcnt = getushort(ttf);
	    if ( feof(ttf)) {
		LogError( _("Unexpected end of file in contextual chaining subtable.\n") );
		info->bad_ot = true;
return;
	    }
	    rules[i].subrules[j].fglyphs = malloc((rules[i].subrules[j].fcnt+1)*sizeof(uint16));
	    for ( k=0; k<rules[i].subrules[j].fcnt; ++k )
		rules[i].subrules[j].fglyphs[k] = getushort(ttf);
	    rules[i].subrules[j].fglyphs[k] = 0xffff;

	    for ( which = 0; which<3; ++which ) {
		for ( k=0; k<(&rules[i].subrules[j].gcnt)[which]; ++k ) {
		    if ( (&rules[i].subrules[j].glyphs)[which][k]>=info->glyph_cnt ) {
			if ( !warned )
			    LogError( _("Bad contextual or chaining sub table. Glyph %d out of range [0,%d)\n"),
				    (&rules[i].subrules[j].glyphs)[which][k], info->glyph_cnt );
			info->bad_ot = true;
			warned = true;
			(&rules[i].subrules[j].glyphs)[which][k] = 0;
		    }
		}
	    }

	    rules[i].subrules[j].scnt = getushort(ttf);
	    if ( feof(ttf)) {
		LogError( _("Unexpected end of file in contextual chaining subtable.\n") );
		info->bad_ot = true;
return;
	    }
	    rules[i].subrules[j].sl = malloc(rules[i].subrules[j].scnt*sizeof(struct seqlookup));
	    for ( k=0; k<rules[i].subrules[j].scnt; ++k ) {
		rules[i].subrules[j].sl[k].seq = getushort(ttf);
		if ( rules[i].subrules[j].sl[k].seq >= rules[i].subrules[j].gcnt+1 )
		    if ( !warned2 ) {
			LogError( _("Attempt to apply a lookup to a location out of the range of this contextual\n lookup seq=%d max=%d\n"),
				rules[i].subrules[j].sl[k].seq, rules[i].subrules[j].gcnt );
			info->bad_ot = true;
			warned2 = true;
		    }
		rules[i].subrules[j].sl[k].lookup = (void *) (intpt) getushort(ttf);
	    }
	}
    }

    if ( justinuse==git_justinuse ) {
	/* Nothing to do. This lookup doesn't really reference any glyphs */
	/*  any lookups it invokes will be processed on their own */
    } else {
	fpst = chunkalloc(sizeof(FPST));
	fpst->type = gpos ? pst_chainpos : pst_chainsub;
	fpst->format = pst_glyphs;
	fpst->subtable = subtable;
	fpst->next = info->possub;
	info->possub = fpst;
	subtable->fpst = fpst;

	fpst->rules = rule = calloc(cnt,sizeof(struct fpst_rule));
	fpst->rule_cnt = cnt;

	cnt = 0;
	for ( i=0; i<rcnt; ++i ) for ( j=0; j<rules[i].scnt; ++j ) {
	    rule[cnt].u.glyph.back = GlyphsToNames(info,rules[i].subrules[j].bglyphs,false);
	    rule[cnt].u.glyph.names = GlyphsToNames(info,rules[i].subrules[j].glyphs,false);
	    rule[cnt].u.glyph.fore = GlyphsToNames(info,rules[i].subrules[j].fglyphs,false);
	    rule[cnt].lookup_cnt = rules[i].subrules[j].scnt;
	    rule[cnt].lookups = rules[i].subrules[j].sl;
	    rules[i].subrules[j].sl = NULL;
	    for ( k=0; k<rule[cnt].lookup_cnt; ++k )
		ProcessSubLookups(ttf,info,gpos,alllooks,&rule[cnt].lookups[k]);
	    ++cnt;
	}
    }

    for ( i=0; i<rcnt; ++i ) {
	for ( j=0; j<rules[i].scnt; ++j ) {
	    free(rules[i].subrules[j].bglyphs);
	    free(rules[i].subrules[j].glyphs);
	    free(rules[i].subrules[j].fglyphs);
	    free(rules[i].subrules[j].sl);
	}
	free(rules[i].subrules);
    }
    free(rules);
    free(glyphs);
}

static void g___ContextSubTable2(FILE *ttf, int stoffset,
	struct ttfinfo *info, struct lookup *l, struct lookup_subtable *subtable, int justinuse,
	struct lookup *alllooks, int gpos) {
    int i, j, k, rcnt, cnt;
    uint16 coverage;
    uint16 classoff;
    struct subrule {
	uint32 offset;
	int ccnt;
	int scnt;
	uint16 *classindeces;
	struct seqlookup *sl;
    };
    struct rule {
	uint32 offsets;
	int scnt;
	struct subrule *subrules;
    } *rules;
    FPST *fpst;
    struct fpst_rule *rule;
    uint16 *glyphs, *class;
    int warned2 = false;

    coverage = getushort(ttf);
    classoff = getushort(ttf);
    rcnt = getushort(ttf);		/* class count in coverage table *//* == number of top level rules */
    rules = calloc(rcnt,sizeof(struct rule));
    for ( i=0; i<rcnt; ++i )
	rules[i].offsets = getushort(ttf)+stoffset;
    cnt = 0;
    for ( i=0; i<rcnt; ++i ) if ( rules[i].offsets!=stoffset ) { /* some classes might be unused */
	fseek(ttf,rules[i].offsets,SEEK_SET);
	rules[i].scnt = getushort(ttf);
	if ( rules[i].scnt<0 ) {
	    LogError( _("Bad count in context chaining sub-table.\n") );
	    info->bad_ot = true;
            free(rules);
return;
	}
	cnt += rules[i].scnt;
	rules[i].subrules = malloc(rules[i].scnt*sizeof(struct subrule));
	for ( j=0; j<rules[i].scnt; ++j )
	    rules[i].subrules[j].offset = getushort(ttf)+rules[i].offsets;
	for ( j=0; j<rules[i].scnt; ++j ) {
	    fseek(ttf,rules[i].subrules[j].offset,SEEK_SET);
	    rules[i].subrules[j].ccnt = getushort(ttf);
	    rules[i].subrules[j].scnt = getushort(ttf);
	    if ( rules[i].subrules[j].ccnt<0 ) {
		LogError( _("Bad class count in contextual chaining sub-table.\n") );
		info->bad_ot = true;
		free(rules);
return;
	    }
	    rules[i].subrules[j].classindeces = malloc(rules[i].subrules[j].ccnt*sizeof(uint16));
	    rules[i].subrules[j].classindeces[0] = i;
	    for ( k=1; k<rules[i].subrules[j].ccnt; ++k )
		rules[i].subrules[j].classindeces[k] = getushort(ttf);
	    if ( rules[i].subrules[j].scnt<0 ) {
		LogError( _("Bad count in contextual chaining sub-table.\n") );
		info->bad_ot = true;
		free(rules);
return;
	    }
	    rules[i].subrules[j].sl = malloc(rules[i].subrules[j].scnt*sizeof(struct seqlookup));
	    for ( k=0; k<rules[i].subrules[j].scnt; ++k ) {
		rules[i].subrules[j].sl[k].seq = getushort(ttf);
		if ( rules[i].subrules[j].sl[k].seq >= rules[i].subrules[j].ccnt )
		    if ( !warned2 ) {
			LogError( _("Attempt to apply a lookup to a location out of the range of this contextual\n lookup seq=%d max=%d\n"),
				rules[i].subrules[j].sl[k].seq, rules[i].subrules[j].ccnt-1);
			info->bad_ot = true;
			warned2 = true;
		    }
		rules[i].subrules[j].sl[k].lookup = (void *) (intpt) getushort(ttf);
	    }
	}
    }

    if ( justinuse==git_justinuse ) {
	/* Nothing to do. This lookup doesn't really reference any glyphs */
	/*  any lookups it invokes will be processed on their own */
    } else {
	fpst = chunkalloc(sizeof(FPST));
	fpst->type = gpos ? pst_contextpos : pst_contextsub;
	fpst->format = pst_class;
	fpst->subtable = subtable;
	subtable->fpst = fpst;
	fpst->next = info->possub;
	info->possub = fpst;

	fpst->rules = rule = calloc(cnt,sizeof(struct fpst_rule));
	fpst->rule_cnt = cnt;
	class = getClassDefTable(ttf, stoffset+classoff, info);
	fpst->nccnt = ClassFindCnt(class,info->glyph_cnt);
	fpst->nclass = ClassToNames(info,fpst->nccnt,class,info->glyph_cnt);
	fpst->nclassnames = calloc(fpst->nccnt,sizeof(char *));

	/* Just in case they used the coverage table to redefine class 0 */
	glyphs = getCoverageTable(ttf,stoffset+coverage,info);
	if ( glyphs==NULL ) {
/* GT: This continues a multi-line error message, hence the leading space */
	    LogError( _(" Bad contextual substitution table, ignored\n") );
            free(rules);
return;
	}
	fpst->nclass[0] = CoverageMinusClasses(glyphs,class,info);
	free(glyphs); free(class); class = NULL;

	cnt = 0;
	for ( i=0; i<rcnt; ++i ) for ( j=0; j<rules[i].scnt; ++j ) {
	    rule[cnt].u.class.nclasses = rules[i].subrules[j].classindeces;
	    rule[cnt].u.class.ncnt = rules[i].subrules[j].ccnt;
	    rules[i].subrules[j].classindeces = NULL;
	    rule[cnt].lookup_cnt = rules[i].subrules[j].scnt;
	    rule[cnt].lookups = rules[i].subrules[j].sl;
	    rules[i].subrules[j].sl = NULL;
	    for ( k=0; k<rule[cnt].lookup_cnt; ++k )
		ProcessSubLookups(ttf,info,gpos,alllooks,&rule[cnt].lookups[k]);
	    ++cnt;
	}
    }

    for ( i=0; i<rcnt; ++i ) {
	for ( j=0; j<rules[i].scnt; ++j ) {
	    free(rules[i].subrules[j].classindeces);
	    free(rules[i].subrules[j].sl);
	}
	free(rules[i].subrules);
    }
    free(rules);
}

static void g___ChainingSubTable2(FILE *ttf, int stoffset,
	struct ttfinfo *info, struct lookup *l, struct lookup_subtable *subtable, int justinuse,
	struct lookup *alllooks, int gpos) {
    int i, j, k, rcnt, cnt;
    uint16 coverage, offset;
    uint16 bclassoff, classoff, fclassoff;
    struct subrule {
	uint32 offset;
	int ccnt, bccnt, fccnt;
	int scnt;
	uint16 *classindeces, *bci, *fci;
	struct seqlookup *sl;
    };
    struct rule {
	uint32 offsets;
	int scnt;
	struct subrule *subrules;
    } *rules;
    FPST *fpst;
    struct fpst_rule *rule;
    uint16 *glyphs, *class;
    int warned2 = false;

    coverage = getushort(ttf);
    bclassoff = getushort(ttf);
    classoff = getushort(ttf);
    fclassoff = getushort(ttf);
    rcnt = getushort(ttf);		/* class count *//* == max number of top level rules */
    rules = calloc(rcnt,sizeof(struct rule));
    for ( i=0; i<rcnt; ++i ) {
	offset = getushort(ttf);
	rules[i].offsets = offset==0 ? 0 : offset+stoffset;
    }
    cnt = 0;
    for ( i=0; i<rcnt; ++i ) if ( rules[i].offsets!=0 ) { /* some classes might be unused */
	fseek(ttf,rules[i].offsets,SEEK_SET);
	rules[i].scnt = getushort(ttf);
	if ( rules[i].scnt<0 ) {
	    LogError( _("Bad count in context chaining sub-table.\n") );
	    info->bad_ot = true;
            free(rules);
return;
	}
	cnt += rules[i].scnt;
	rules[i].subrules = malloc(rules[i].scnt*sizeof(struct subrule));
	for ( j=0; j<rules[i].scnt; ++j )
	    rules[i].subrules[j].offset = getushort(ttf)+rules[i].offsets;
	for ( j=0; j<rules[i].scnt; ++j ) {
	    fseek(ttf,rules[i].subrules[j].offset,SEEK_SET);
	    rules[i].subrules[j].bccnt = getushort(ttf);
	    if ( rules[i].subrules[j].bccnt<0 ) {
		LogError( _("Bad class count in contextual chaining sub-table.\n") );
		info->bad_ot = true;
		free(rules);
return;
	    }
	    rules[i].subrules[j].bci = malloc(rules[i].subrules[j].bccnt*sizeof(uint16));
	    for ( k=0; k<rules[i].subrules[j].bccnt; ++k )
		rules[i].subrules[j].bci[k] = getushort(ttf);
	    rules[i].subrules[j].ccnt = getushort(ttf);
	    if ( rules[i].subrules[j].ccnt<0 ) {
		LogError( _("Bad class count in contextual chaining sub-table.\n") );
		info->bad_ot = true;
		free(rules);
return;
	    }
	    rules[i].subrules[j].classindeces = malloc(rules[i].subrules[j].ccnt*sizeof(uint16));
	    rules[i].subrules[j].classindeces[0] = i;
	    for ( k=1; k<rules[i].subrules[j].ccnt; ++k )
		rules[i].subrules[j].classindeces[k] = getushort(ttf);
	    rules[i].subrules[j].fccnt = getushort(ttf);
	    if ( rules[i].subrules[j].fccnt<0 ) {
		LogError( _("Bad class count in contextual chaining sub-table.\n") );
		info->bad_ot = true;
		free(rules);
return;
	    }
	    rules[i].subrules[j].fci = malloc(rules[i].subrules[j].fccnt*sizeof(uint16));
	    for ( k=0; k<rules[i].subrules[j].fccnt; ++k )
		rules[i].subrules[j].fci[k] = getushort(ttf);
	    rules[i].subrules[j].scnt = getushort(ttf);
	    if ( rules[i].subrules[j].scnt<0 ) {
		LogError( _("Bad count in contextual chaining sub-table.\n") );
		info->bad_ot = true;
		free(rules);
return;
	    }
	    rules[i].subrules[j].sl = malloc(rules[i].subrules[j].scnt*sizeof(struct seqlookup));
	    for ( k=0; k<rules[i].subrules[j].scnt; ++k ) {
		rules[i].subrules[j].sl[k].seq = getushort(ttf);
		if ( rules[i].subrules[j].sl[k].seq >= rules[i].subrules[j].ccnt )
		    if ( !warned2 ) {
			LogError( _("Attempt to apply a lookup to a location out of the range of this contextual\n lookup seq=%d max=%d\n"),
				rules[i].subrules[j].sl[k].seq, rules[i].subrules[j].ccnt-1);
			info->bad_ot = true;
			warned2 = true;
		    }
		rules[i].subrules[j].sl[k].lookup = (void *) (intpt) getushort(ttf);
	    }
	}
    }

    if ( justinuse==git_justinuse ) {
	/* Nothing to do. This lookup doesn't really reference any glyphs */
	/*  any lookups it invokes will be processed on their own */
    } else {
	fpst = chunkalloc(sizeof(FPST));
	fpst->type = gpos ? pst_chainpos : pst_chainsub;
	fpst->format = pst_class;
	fpst->subtable = subtable;
	subtable->fpst = fpst;
	fpst->next = info->possub;
	info->possub = fpst;

	fpst->rules = rule = calloc(cnt,sizeof(struct fpst_rule));
	fpst->rule_cnt = cnt;

	class = getClassDefTable(ttf, stoffset+classoff, info);
	fpst->nccnt = ClassFindCnt(class,info->glyph_cnt);
	fpst->nclass = ClassToNames(info,fpst->nccnt,class,info->glyph_cnt);
	fpst->nclassnames = calloc(fpst->nccnt,sizeof(char *));

	/* Just in case they used the coverage table to redefine class 0 */
	glyphs = getCoverageTable(ttf,stoffset+coverage,info);
	if ( glyphs==NULL ) {
/* GT: This continues a multi-line error message, hence the leading space */
	    LogError( _(" Bad contextual chaining substitution table, ignored\n") );
            free(rules);
return;
	}
	fpst->nclass[0] = CoverageMinusClasses(glyphs,class,info);
	free(glyphs); free(class); class = NULL;

	/* The docs don't mention this, but in mangal.ttf fclassoff==0 NULL */
	if ( bclassoff!=0 )
	    class = getClassDefTable(ttf, stoffset+bclassoff, info);
	else
	    class = calloc(info->glyph_cnt,sizeof(uint16));
	fpst->bccnt = ClassFindCnt(class,info->glyph_cnt);
	fpst->bclass = ClassToNames(info,fpst->bccnt,class,info->glyph_cnt);
	fpst->bclassnames = calloc(fpst->bccnt,sizeof(char *));
	free(class);
	if ( fclassoff!=0 )
	    class = getClassDefTable(ttf, stoffset+fclassoff, info);
	else
	    class = calloc(info->glyph_cnt,sizeof(uint16));
	fpst->fccnt = ClassFindCnt(class,info->glyph_cnt);
	fpst->fclass = ClassToNames(info,fpst->fccnt,class,info->glyph_cnt);
	fpst->fclassnames = calloc(fpst->fccnt,sizeof(char *));
	free(class);

	cnt = 0;
	for ( i=0; i<rcnt; ++i ) for ( j=0; j<rules[i].scnt; ++j ) {
	    rule[cnt].u.class.nclasses = rules[i].subrules[j].classindeces;
	    rule[cnt].u.class.ncnt = rules[i].subrules[j].ccnt;
	    rules[i].subrules[j].classindeces = NULL;
	    rule[cnt].u.class.bclasses = rules[i].subrules[j].bci;
	    rule[cnt].u.class.bcnt = rules[i].subrules[j].bccnt;
	    rules[i].subrules[j].bci = NULL;
	    rule[cnt].u.class.fclasses = rules[i].subrules[j].fci;
	    rule[cnt].u.class.fcnt = rules[i].subrules[j].fccnt;
	    rules[i].subrules[j].fci = NULL;
	    rule[cnt].lookup_cnt = rules[i].subrules[j].scnt;
	    rule[cnt].lookups = rules[i].subrules[j].sl;
	    rules[i].subrules[j].sl = NULL;
	    for ( k=0; k<rule[cnt].lookup_cnt; ++k )
		ProcessSubLookups(ttf,info,gpos,alllooks,&rule[cnt].lookups[k]);
	    ++cnt;
	}
    }

    for ( i=0; i<rcnt; ++i ) {
	for ( j=0; j<rules[i].scnt; ++j ) {
	    free(rules[i].subrules[j].classindeces);
	    free(rules[i].subrules[j].sl);
	}
	free(rules[i].subrules);
    }
    free(rules);
}

static void g___ContextSubTable3(FILE *ttf, int stoffset,
	struct ttfinfo *info, struct lookup *l, struct lookup_subtable *subtable, int justinuse,
	struct lookup *alllooks, int gpos) {
    int i, k, scnt, gcnt;
    uint16 *coverage;
    struct seqlookup *sl;
    uint16 *glyphs;
    FPST *fpst;
    struct fpst_rule *rule;
    int warned2 = false;

    gcnt = getushort(ttf);
    scnt = getushort(ttf);
    if ( feof(ttf) ) {
	LogError( _("End of file in context chaining sub-table.\n") );
	info->bad_ot = true;
return;
    }
    coverage = malloc(gcnt*sizeof(uint16));
    for ( i=0; i<gcnt; ++i )
	coverage[i] = getushort(ttf);
    sl = malloc(scnt*sizeof(struct seqlookup));
    for ( k=0; k<scnt; ++k ) {
	sl[k].seq = getushort(ttf);
	if ( sl[k].seq >= gcnt && !warned2 ) {
	    LogError( _("Attempt to apply a lookup to a location out of the range of this contextual\n lookup seq=%d, max=%d\n"),
		    sl[k].seq, gcnt-1 );
	    info->bad_ot = true;
	    warned2 = true;
	}
	sl[k].lookup = (void *) (intpt) getushort(ttf);
    }

    if ( justinuse==git_justinuse ) {
	/* Nothing to do. This lookup doesn't really reference any glyphs */
	/*  any lookups it invokes will be processed on their own */
        free(sl);
    } else {
	fpst = chunkalloc(sizeof(FPST));
	fpst->type = gpos ? pst_contextpos : pst_contextsub;
	fpst->format = pst_coverage;
	fpst->subtable = subtable;
	subtable->fpst = fpst;
	fpst->next = info->possub;
	info->possub = fpst;

	fpst->rules = rule = calloc(1,sizeof(struct fpst_rule));
	fpst->rule_cnt = 1;
	rule->u.coverage.ncnt = gcnt;
	rule->u.coverage.ncovers = malloc(gcnt*sizeof(char *));
	for ( i=0; i<gcnt; ++i ) {
	    glyphs =  getCoverageTable(ttf,stoffset+coverage[i],info);
	    rule->u.coverage.ncovers[i] = GlyphsToNames(info,glyphs,true);
	    free(glyphs);
	}
	rule->lookup_cnt = scnt;
	rule->lookups = sl;
	for ( k=0; k<scnt; ++k )
	    ProcessSubLookups(ttf,info,gpos,alllooks,&sl[k]);
    }

    free(coverage);
}

static void g___ChainingSubTable3(FILE *ttf, int stoffset,
	struct ttfinfo *info, struct lookup *l, struct lookup_subtable *subtable, int justinuse,
	struct lookup *alllooks, int gpos) {
    int i, k, scnt, gcnt, bcnt, fcnt;
    uint16 *coverage, *bcoverage, *fcoverage;
    struct seqlookup *sl;
    uint16 *glyphs;
    FPST *fpst;
    struct fpst_rule *rule;
    int warned2 = false;

    bcnt = getushort(ttf);
    if ( feof(ttf)) {
	LogError( _("End of file in context chaining subtable.\n") );
	info->bad_ot = true;
return;
    }
    bcoverage = malloc(bcnt*sizeof(uint16));
    for ( i=0; i<bcnt; ++i )
	bcoverage[i] = getushort(ttf);
    gcnt = getushort(ttf);
    if ( feof(ttf)) {
	LogError( _("End of file in context chaining subtable.\n") );
	info->bad_ot = true;
        free(bcoverage);
return;
    }
    coverage = malloc(gcnt*sizeof(uint16));
    for ( i=0; i<gcnt; ++i )
	coverage[i] = getushort(ttf);
    fcnt = getushort(ttf);
    if ( feof(ttf)) {
	LogError( _("End of file in context chaining subtable.\n") );
	info->bad_ot = true;
        free(bcoverage);
        free(coverage);
return;
    }
    fcoverage = malloc(fcnt*sizeof(uint16));
    for ( i=0; i<fcnt; ++i )
	fcoverage[i] = getushort(ttf);
    scnt = getushort(ttf);
    if ( feof(ttf)) {
	LogError( _("End of file in context chaining subtable.\n") );
	info->bad_ot = true;
        free(fcoverage);
        free(bcoverage);
        free(coverage);
return;
    }
    sl = malloc(scnt*sizeof(struct seqlookup));
    for ( k=0; k<scnt; ++k ) {
	sl[k].seq = getushort(ttf);
	if ( sl[k].seq >= gcnt && !warned2 ) {
	    LogError( _("Attempt to apply a lookup to a location out of the range of this contextual\n lookup seq=%d, max=%d\n"),
		    sl[k].seq, gcnt-1 );
	    info->bad_ot = true;
	    warned2 = true;
	}
	sl[k].lookup = (void *) (intpt) getushort(ttf);
    }

    if ( justinuse==git_justinuse ) {
	/* Nothing to do. This lookup doesn't really reference any glyphs */
	/*  any lookups it invokes will be processed on their own */
        free(sl);
    } else {
	fpst = chunkalloc(sizeof(FPST));
	fpst->type = gpos ? pst_chainpos : pst_chainsub;
	fpst->format = pst_coverage;
	fpst->subtable = subtable;
	subtable->fpst = fpst;
	fpst->next = info->possub;
	info->possub = fpst;

	fpst->rules = rule = calloc(1,sizeof(struct fpst_rule));
	fpst->rule_cnt = 1;

	rule->u.coverage.bcnt = bcnt;
	rule->u.coverage.bcovers = malloc(bcnt*sizeof(char *));
	for ( i=0; i<bcnt; ++i ) {
	    glyphs =  getCoverageTable(ttf,stoffset+bcoverage[i],info);
	    rule->u.coverage.bcovers[i] = GlyphsToNames(info,glyphs,true);
	    free(glyphs);
	}

	rule->u.coverage.ncnt = gcnt;
	rule->u.coverage.ncovers = malloc(gcnt*sizeof(char *));
	for ( i=0; i<gcnt; ++i ) {
	    glyphs =  getCoverageTable(ttf,stoffset+coverage[i],info);
	    rule->u.coverage.ncovers[i] = GlyphsToNames(info,glyphs,true);
	    free(glyphs);
	}

	rule->u.coverage.fcnt = fcnt;
	rule->u.coverage.fcovers = malloc(fcnt*sizeof(char *));
	for ( i=0; i<fcnt; ++i ) {
	    glyphs =  getCoverageTable(ttf,stoffset+fcoverage[i],info);
	    rule->u.coverage.fcovers[i] = GlyphsToNames(info,glyphs,true);
	    free(glyphs);
	}

	rule->lookup_cnt = scnt;
	rule->lookups = sl;
	for ( k=0; k<scnt; ++k )
	    ProcessSubLookups(ttf,info,gpos,alllooks,&sl[k]);
    }

    free(bcoverage);
    free(coverage);
    free(fcoverage);
}

static void gposContextSubTable(FILE *ttf, int stoffset,
	struct ttfinfo *info, struct lookup *l, struct lookup_subtable *subtable, 
	struct lookup *alllooks) {
    switch( getushort(ttf)) {
      case 1:
	g___ContextSubTable1(ttf,stoffset,info,l,subtable,git_normal,alllooks,true);
      break;
      case 2:
	g___ContextSubTable2(ttf,stoffset,info,l,subtable,git_normal,alllooks,true);
      break;
      case 3:
	g___ContextSubTable3(ttf,stoffset,info,l,subtable,git_normal,alllooks,true);
      break;
    }
}

static void gposChainingSubTable(FILE *ttf, int stoffset,
	struct ttfinfo *info, struct lookup *l, struct lookup_subtable *subtable, 
	struct lookup *alllooks) {
    switch( getushort(ttf)) {
      case 1:
	g___ChainingSubTable1(ttf,stoffset,info,l,subtable,git_normal,alllooks,true);
      break;
      case 2:
	g___ChainingSubTable2(ttf,stoffset,info,l,subtable,git_normal,alllooks,true);
      break;
      case 3:
	g___ChainingSubTable3(ttf,stoffset,info,l,subtable,git_normal,alllooks,true);
      break;
    }
}

static struct { uint32 tag; char *str; } tagstr[] = {
    { CHR('v','r','t','2'), "vert" },
    { CHR('s','m','c','p'), "sc" },
    { CHR('s','m','c','p'), "small" },
    { CHR('o','n','u','m'), "oldstyle" },
    { CHR('s','u','p','s'), "superior" },
    { CHR('s','u','b','s'), "inferior" },
    { CHR('s','w','s','h'), "swash" },
    { 0, NULL }
};

static void gsubSimpleSubTable(FILE *ttf, int stoffset, struct ttfinfo *info,
	struct lookup *l, struct lookup_subtable *subtable, int justinuse) {
    int coverage, cnt, i, j, which;
    uint16 format;
    uint16 *glyphs, *glyph2s=NULL;
    int delta=0;

    format=getushort(ttf);
    if ( format!=1 && format!=2 )	/* Unknown subtable format */
return;
    coverage = getushort(ttf);
    if ( format==1 ) {
	delta = getushort(ttf);
    } else {
	cnt = getushort(ttf);
	glyph2s = malloc(cnt*sizeof(uint16));
	for ( i=0; i<cnt; ++i )
	    glyph2s[i] = getushort(ttf);
	    /* in range check comes later */
    }
    glyphs = getCoverageTable(ttf,stoffset+coverage,info);
    if ( glyphs==NULL ) {
	free(glyph2s);
	LogError( _(" Bad simple substitution table, ignored\n") );
return;
    }
    if ( justinuse==git_findnames ) {
	FeatureScriptLangList *fl;
	fl = l->otlookup->features;
	/* Unnamed glyphs get a name built of the base name and the feature tag */
	/*  assuming this lookup is tagged with a feature... */
	if ( fl!=NULL )
	for ( i=0; glyphs[i]!=0xffff; ++i ) if ( glyphs[i]<info->glyph_cnt ) {
	    if ( info->chars[glyphs[i]]->name!=NULL ) {
		which = format==1 ? (uint16) (glyphs[i]+delta) : glyph2s[i];
		if ( which<info->glyph_cnt && which>=0 && info->chars[which]!=NULL &&
			info->chars[which]->name==NULL ) {
		    char *basename = info->chars[glyphs[i]]->name;
		    char *str;
		    char tag[5], *pt=tag;
		    for ( j=0; tagstr[j].tag!=0 && tagstr[j].tag!=fl->featuretag; ++j );
		    if ( tagstr[j].tag!=0 )
			pt = tagstr[j].str;
		    else {
			tag[0] = fl->featuretag>>24;
			if ( (tag[1] = (fl->featuretag>>16)&0xff)==' ' ) tag[1] = '\0';
			if ( (tag[2] = (fl->featuretag>>8)&0xff)==' ' ) tag[2] = '\0';
			if ( (tag[3] = (fl->featuretag)&0xff)==' ' ) tag[3] = '\0';
			tag[4] = '\0';
			pt = tag;
		    }
		    str = malloc(strlen(basename)+strlen(pt)+2);
		    sprintf(str,"%s.%s", basename, pt );
		    info->chars[which]->name = str;
		}
	    }
	}
    } else if ( justinuse==git_justinuse ) {
	for ( i=0; glyphs[i]!=0xffff; ++i ) if ( glyphs[i]<info->glyph_cnt ) {
	    info->inuse[glyphs[i]]= true;
	    which = format==1 ? (uint16) (glyphs[i]+delta) : glyph2s[i];
	    info->inuse[which]= true;
	}
    } else if ( justinuse==git_normal ) {
	for ( i=0; glyphs[i]!=0xffff; ++i ) if ( glyphs[i]<info->glyph_cnt && info->chars[glyphs[i]]!=NULL ) {
	    which = format==1 ? (uint16) (glyphs[i]+delta) : glyph2s[i];
	    if ( which>=info->glyph_cnt ) {
		LogError( _("Bad substitution glyph: GID %d not less than %d\n"),
			which, info->glyph_cnt);
		info->bad_ot = true;
		which = 0;
	    }
	    if ( info->chars[which]!=NULL && info->chars[glyphs[i]]!=NULL ) {
			/* Might be in a ttc file */
		PST *pos = chunkalloc(sizeof(PST));
		pos->type = pst_substitution;
		pos->subtable = subtable;
		pos->next = info->chars[glyphs[i]]->possub;
		info->chars[glyphs[i]]->possub = pos;
		pos->u.subs.variant = copy(info->chars[which]->name);
	    }
	}
    }
    subtable->per_glyph_pst_or_kern = true;
    free(glyph2s);
    free(glyphs);
}

/* Multiple and alternate substitution lookups have the same format */
static void gsubMultipleSubTable(FILE *ttf, int stoffset, struct ttfinfo *info,
	struct lookup *l, struct lookup_subtable *subtable, int justinuse) {
    int coverage, cnt, i, j, len, max;
    uint16 format;
    uint16 *offsets;
    uint16 *glyphs, *glyph2s;
    char *pt;
    int bad;
    int badcnt = 0;

    if ( justinuse==git_findnames )
return;

    format=getushort(ttf);
    if ( format!=1 )	/* Unknown subtable format */
return;
    coverage = getushort(ttf);
    cnt = getushort(ttf);
    if ( feof(ttf)) {
	LogError( _("Unexpected end of file in GSUB sub-table.\n"));
	info->bad_ot = true;
return;
    }
    offsets = malloc(cnt*sizeof(uint16));
    for ( i=0; i<cnt; ++i )
	offsets[i] = getushort(ttf);
    glyphs = getCoverageTable(ttf,stoffset+coverage,info);
    if ( glyphs==NULL ) {
	free(offsets);
	LogError( _(" Bad multiple substitution table, ignored\n") );
return;
    }
    for ( i=0; glyphs[i]!=0xffff; ++i );
    if ( i!=cnt ) {
	LogError( _("Coverage table specifies a different number of glyphs than the sub-table expects.\n" ));
	info->bad_ot = true;
	if ( cnt<i )
	    glyphs[cnt] = 0xffff;
	else
	    cnt = i;
    }
    max = 20;
    glyph2s = malloc(max*sizeof(uint16));
    for ( i=0; glyphs[i]!=0xffff; ++i ) {
		PST *alt;
		fseek(ttf,stoffset+offsets[i],SEEK_SET);
		cnt = getushort(ttf);
		if ( feof(ttf)) {
			LogError( _("Unexpected end of file in GSUB sub-table.\n"));
			info->bad_ot = true;
			free(offsets);
			free(glyphs);
			free(glyph2s);
			return;
		}
		if ( cnt>max ) {
			max = cnt+30;
			glyph2s = realloc(glyph2s,max*sizeof(uint16));
		}
		len = 0; bad = false;
		for ( j=0; j<cnt; ++j ) {
			glyph2s[j] = getushort(ttf);
			if ( feof(ttf)) {
				LogError( _("Unexpected end of file in GSUB sub-table.\n" ));
				info->bad_ot = true;
				free(offsets);
				free(glyphs);
				free(glyph2s);
				return;
			}
			if ( glyph2s[j]>=info->glyph_cnt ) {
			if ( !justinuse )
				LogError( _("Bad Multiple/Alternate substitution glyph. GID %d not less than %d\n"),
					glyph2s[j], info->glyph_cnt );
			info->bad_ot = true;
			if ( ++badcnt>20 ) {
				free(offsets);
				free(glyphs);
				free(glyph2s);
				return;
			}
			glyph2s[j] = 0;
			}
			if ( justinuse==git_justinuse )
			/* Do Nothing */;
			else if ( info->chars[glyph2s[j]]==NULL )
			bad = true;
			else
			len += strlen( info->chars[glyph2s[j]]->name) +1;
		}
		if ( justinuse==git_justinuse ) {
			info->inuse[glyphs[i]] = 1;
			for ( j=0; j<cnt; ++j )
			info->inuse[glyph2s[j]] = 1;
		} else if ( info->chars[glyphs[i]]!=NULL && !bad ) {
			alt = chunkalloc(sizeof(PST));
			alt->type = l->otlookup->lookup_type==gsub_multiple?pst_multiple:pst_alternate;
			alt->subtable = subtable;
			alt->next = info->chars[glyphs[i]]->possub;
			info->chars[glyphs[i]]->possub = alt;
			pt = alt->u.subs.variant = malloc(len+1);
			*pt = '\0';
			for ( j=0; j<cnt; ++j ) {
			strcat(pt,info->chars[glyph2s[j]]->name);
			strcat(pt," ");
			}
			if ( *pt!='\0' && pt[strlen(pt)-1]==' ' )
			pt[strlen(pt)-1] = '\0';
		}
		if (glyphs[i] > info->glyph_cnt) {
		        fprintf(stderr, "This glyph is out of bounds.\n");
		} else if (info->chars[glyphs[i]] == NULL || info->chars[glyphs[i]]->possub == NULL) {
		        if (justinuse != git_justinuse) fprintf( stderr , "This glyph isn't loaded yet.\n" );
		} else if (info->chars[glyphs[i]]->possub->u.subs.variant == NULL) {
			fprintf( stderr , "info->chars[glyphs[%d]]->possub->u.subs.variant is null. glyphs[%d] = %d. info->chars[%d]->name = \"%s\".\n" , i , i , glyphs[i] , glyphs[i] , info->chars[glyphs[i]]->name ) ;
		}
    }
    subtable->per_glyph_pst_or_kern = true;
    free(glyphs);
    free(glyph2s);
    free(offsets);
}

static void gsubLigatureSubTable(FILE *ttf, int stoffset,
	struct ttfinfo *info, struct lookup *l, struct lookup_subtable *subtable, int justinuse) {
    int coverage, cnt, i, j, k, lig_cnt, cc, len;
    uint16 *ls_offsets, *lig_offsets;
    uint16 *glyphs, *lig_glyphs, lig;
    char *pt;
    PST *liga;

    /* Format = */ getushort(ttf);
    coverage = getushort(ttf);
    cnt = getushort(ttf);
    if ( feof(ttf)) {
	LogError( _("Unexpected end of file in GSUB ligature sub-table.\n" ));
	info->bad_ot = true;
return;
    }
    ls_offsets = malloc(cnt*sizeof(uint16));
    for ( i=0; i<cnt; ++i )
	ls_offsets[i]=getushort(ttf);
    glyphs = getCoverageTable(ttf,stoffset+coverage,info);
    if ( glyphs==NULL ) {
	LogError( _(" Bad ligature table, ignored\n") );
        free(ls_offsets);
return;
    }
    for ( i=0; i<cnt; ++i ) {
	fseek(ttf,stoffset+ls_offsets[i],SEEK_SET);
	lig_cnt = getushort(ttf);
	if ( feof(ttf)) {
	    LogError( _("Unexpected end of file in GSUB ligature sub-table.\n" ));
	    info->bad_ot = true;
            free(ls_offsets);
return;
	}
	lig_offsets = malloc(lig_cnt*sizeof(uint16));
	for ( j=0; j<lig_cnt; ++j )
	    lig_offsets[j] = getushort(ttf);
	if ( feof(ttf)) {
	    LogError( _("Unexpected end of file in GSUB ligature sub-table.\n" ));
	    info->bad_ot = true;
            free(lig_offsets);
            free(ls_offsets);
return;
	}
	for ( j=0; j<lig_cnt; ++j ) {
	    fseek(ttf,stoffset+ls_offsets[i]+lig_offsets[j],SEEK_SET);
	    lig = getushort(ttf);
	    if ( lig>=info->glyph_cnt ) {
		LogError( _("Bad ligature glyph. GID %d not less than %d\n"),
			lig, info->glyph_cnt );
		info->bad_ot = true;
		lig = 0;
	    }
	    cc = getushort(ttf);
	    if ( cc<0 || cc>100 ) {
		LogError( _("Unlikely count of ligature components (%d), I suspect this ligature sub-\n table is garbage, I'm giving up on it.\n"), cc );
		info->bad_ot = true;
		free(glyphs); free(lig_offsets); free(ls_offsets);
return;
	    }
	    lig_glyphs = malloc(cc*sizeof(uint16));
	    lig_glyphs[0] = glyphs[i];
	    for ( k=1; k<cc; ++k ) {
		lig_glyphs[k] = getushort(ttf);
		if ( lig_glyphs[k]>=info->glyph_cnt ) {
		    if ( justinuse==git_normal )
			LogError( _("Bad ligature component glyph. GID %d not less than %d (in ligature %d)\n"),
				lig_glyphs[k], info->glyph_cnt, lig );
		    info->bad_ot = true;
		    lig_glyphs[k] = 0;
		}
	    }
	    if ( justinuse==git_justinuse ) {
		info->inuse[lig] = 1;
		for ( k=0; k<cc; ++k )
		    info->inuse[lig_glyphs[k]] = 1;
	    } else if ( justinuse==git_findnames ) {
		FeatureScriptLangList *fl = l->otlookup->features;
		/* If our ligature glyph has no name (and its components do) */
		/*  give it a name by concatenating components with underscores */
		/*  between them, and appending the tag */
		if ( fl!=NULL && info->chars[lig]!=NULL && info->chars[lig]->name==NULL ) {
		    int len=0;
		    for ( k=0; k<cc; ++k ) {
			if ( info->chars[lig_glyphs[k]]==NULL || info->chars[lig_glyphs[k]]->name==NULL )
		    break;
			len += strlen(info->chars[lig_glyphs[k]]->name)+1;
		    }
		    if ( k==cc ) {
			char *str = malloc(len+6), *pt;
			char tag[5];
			tag[0] = fl->featuretag>>24;
			if ( (tag[1] = (fl->featuretag>>16)&0xff)==' ' ) tag[1] = '\0';
			if ( (tag[2] = (fl->featuretag>>8)&0xff)==' ' ) tag[2] = '\0';
			if ( (tag[3] = (fl->featuretag)&0xff)==' ' ) tag[3] = '\0';
			tag[4] = '\0';
			*str='\0';
			for ( k=0; k<cc; ++k ) {
			    strcat(str,info->chars[lig_glyphs[k]]->name);
			    strcat(str,"_");
			}
			pt = str+strlen(str);
			pt[-1] = '.';
			strcpy(pt,tag);
			info->chars[lig]->name = str;
		    }
		}
	    } else if ( info->chars[lig]!=NULL ) {
		int err=false;
		for ( k=len=0; k<cc; ++k )
		    if ( lig_glyphs[k]<info->glyph_cnt &&
			    info->chars[lig_glyphs[k]]!=NULL )
			len += strlen(info->chars[lig_glyphs[k]]->name)+1;
		    else
			err = true;
		if ( !err ) {
		    liga = chunkalloc(sizeof(PST));
		    liga->type = pst_ligature;
		    liga->subtable = subtable;
		    liga->next = info->chars[lig]->possub;
		    info->chars[lig]->possub = liga;
		    liga->u.lig.lig = info->chars[lig];
		    liga->u.lig.components = pt = malloc(len);
		    for ( k=0; k<cc; ++k ) {
			if ( lig_glyphs[k]<info->glyph_cnt &&
				info->chars[lig_glyphs[k]]!=NULL ) {
			    strcpy(pt,info->chars[lig_glyphs[k]]->name);
			    pt += strlen(pt);
			    *pt++ = ' ';
			}
		    }
		    pt[-1] = '\0';
		}
	    }
	    free(lig_glyphs);
	}
	free(lig_offsets);
    }
    subtable->per_glyph_pst_or_kern = true;
    free(ls_offsets); free(glyphs);
}

static void gsubContextSubTable(FILE *ttf, int stoffset,
	struct ttfinfo *info, struct lookup *l, struct lookup_subtable *subtable, int justinuse,
	struct lookup *alllooks) {
    if ( justinuse==git_findnames )
return;		/* Don't give names to these guys, they might not be unique */
	/* ie. because these are context based there is not a one to one */
	/*  mapping between input glyphs and output glyphs. One input glyph */
	/*  may go to several output glyphs (depending on context) and so */
	/*  <input-glyph-name>"."<tag-name> would be used for several glyphs */
    switch( getushort(ttf)) {
      case 1:
	g___ContextSubTable1(ttf,stoffset,info,l,subtable,justinuse,alllooks,false);
      break;
      case 2:
	g___ContextSubTable2(ttf,stoffset,info,l,subtable,justinuse,alllooks,false);
      break;
      case 3:
	g___ContextSubTable3(ttf,stoffset,info,l,subtable,justinuse,alllooks,false);
      break;
    }
}

static void gsubChainingSubTable(FILE *ttf, int stoffset,
	struct ttfinfo *info, struct lookup *l, struct lookup_subtable *subtable, int justinuse,
	struct lookup *alllooks) {
    if ( justinuse==git_findnames )
return;		/* Don't give names to these guys, the names might not be unique */
    switch( getushort(ttf)) {
      case 1:
	g___ChainingSubTable1(ttf,stoffset,info,l,subtable,justinuse,alllooks,false);
      break;
      case 2:
	g___ChainingSubTable2(ttf,stoffset,info,l,subtable,justinuse,alllooks,false);
      break;
      case 3:
	g___ChainingSubTable3(ttf,stoffset,info,l,subtable,justinuse,alllooks,false);
      break;
    }
}

static void gsubReverseChainSubTable(FILE *ttf, int stoffset,
	struct ttfinfo *info, struct lookup *l, struct lookup_subtable *subtable, int justinuse) {
    int scnt, bcnt, fcnt, i;
    uint16 coverage, *bcoverage, *fcoverage, *sglyphs, *glyphs;
    FPST *fpst;
    struct fpst_rule *rule;

    if ( justinuse==git_findnames )
return;		/* Don't give names to these guys, they might not be unique */
    if ( getushort(ttf)!=1 )
return;		/* Don't understand this format type */

    coverage = getushort(ttf);
    bcnt = getushort(ttf);
    bcoverage = malloc(bcnt*sizeof(uint16));
    for ( i = 0 ; i<bcnt; ++i )
	bcoverage[i] = getushort(ttf);
    fcnt = getushort(ttf);
    fcoverage = malloc(fcnt*sizeof(uint16));
    for ( i = 0 ; i<fcnt; ++i )
	fcoverage[i] = getushort(ttf);
    scnt = getushort(ttf);
    sglyphs = malloc((scnt+1)*sizeof(uint16));
    for ( i = 0 ; i<scnt; ++i )
	if (( sglyphs[i] = getushort(ttf))>=info->glyph_cnt ) {
	    LogError( _("Bad reverse contextual chaining substitution glyph: %d is not less than %d\n"),
		    sglyphs[i], info->glyph_cnt );
	    info->bad_ot = true;
	    sglyphs[i] = 0;
	}
    sglyphs[i] = 0xffff;

    if ( justinuse==git_justinuse ) {
	for ( i = 0 ; i<scnt; ++i )
	    info->inuse[sglyphs[i]] = 1;
    } else {
	fpst = chunkalloc(sizeof(FPST));
	fpst->type = pst_reversesub;
	fpst->format = pst_reversecoverage;
	fpst->subtable = subtable;
	fpst->next = info->possub;
	info->possub = fpst;
	subtable->fpst = fpst;

	fpst->rules = rule = calloc(1,sizeof(struct fpst_rule));
	fpst->rule_cnt = 1;

	rule->u.rcoverage.always1 = 1;
	rule->u.rcoverage.bcnt = bcnt;
	rule->u.rcoverage.fcnt = fcnt;
	rule->u.rcoverage.ncovers = malloc(sizeof(char *));
	rule->u.rcoverage.bcovers = malloc(bcnt*sizeof(char *));
	rule->u.rcoverage.fcovers = malloc(fcnt*sizeof(char *));
	rule->u.rcoverage.replacements = GlyphsToNames(info,sglyphs,false);
	glyphs = getCoverageTable(ttf,stoffset+coverage,info);
	rule->u.rcoverage.ncovers[0] = GlyphsToNames(info,glyphs,false);
	free(glyphs);
	for ( i=0; i<bcnt; ++i ) {
	    glyphs = getCoverageTable(ttf,stoffset+bcoverage[i],info);
	    rule->u.rcoverage.bcovers[i] = GlyphsToNames(info,glyphs,true);
	    free(glyphs);
	}
	for ( i=0; i<fcnt; ++i ) {
	    glyphs = getCoverageTable(ttf,stoffset+fcoverage[i],info);
	    rule->u.rcoverage.fcovers[i] = GlyphsToNames(info,glyphs,true);
	    free(glyphs);
	}
	rule->lookup_cnt = 0;		/* substitution lookups needed for reverse chaining */
    }
    free(sglyphs);
    free(fcoverage);
    free(bcoverage);
}

static void readttfsizeparameters(FILE *ttf,int32 broken_pos,int32 correct_pos,
	struct ttfinfo *info) {
    int32 here;
    /* Both of the two fonts I've seen that contain a 'size' feature */
    /*  have multiple features all of which point to the same parameter */
    /*  area. Odd. */
    /* When Adobe first released fonts containing the 'size' feature */
    /*  they did not follow the spec, and the offset to the size parameters */
    /*  was relative to the wrong location. They claim (Aug 2006) that */
    /*  this has been fixed. Be prepared to read either style of 'size' */
    /*  following the heuristics Adobe provides */
    int32 test[2];
    int i, nid;

    if ( info->last_size_pos==broken_pos || info->last_size_pos==correct_pos )
return;

    if ( info->last_size_pos!=0 ) {
	LogError( _("This font, %s, has multiple GPOS 'size' features. I'm not sure how to interpret that. I shall pick one arbitrarily.\n"),
		info->fontname==NULL? _("<Untitled>") : info->fontname );
	info->bad_ot = true;
return;
    }

    test[0] = correct_pos; test[1] = broken_pos;
    here = ftell(ttf);
    for ( i=0; i<2; ++i ) {
	fseek(ttf,test[i],SEEK_SET);
	info->last_size_pos = test[i];
	info->design_size = getushort(ttf);
	if ( info->design_size==0 )
    continue;
	info->fontstyle_id = getushort(ttf);
	nid = getushort(ttf);
	info->design_range_bottom = getushort(ttf);
	info->design_range_top = getushort(ttf);
	if ( info->fontstyle_id == 0 && nid==0 && info->design_size!=0 &&
		info->design_range_bottom==0 && info->design_range_top==0 ) {
	    /* Reasonable spec, only design size provided */
	    info->fontstyle_name = NULL;
    break;
	}
	if ( info->design_size==0 ||
		info->design_size < info->design_range_bottom ||
		info->design_size > info->design_range_top ||
		info->design_range_bottom > info->design_range_top )
    continue;
	if ( info->fontstyle_id == 0 && nid==0 ) {
	    /* Not really allowed, but we'll accept it anyway */
	    /* If a range is provided than a style name is expected */
	    LogError(_("This font contains a 'size' feature with a design size and design range but no stylename. That is technically an error, but we'll let it pass"));
	    info->fontstyle_name = NULL;
    break;
	}
	if ( nid<256 || nid>32767 )
    continue;
	info->fontstyle_name = FindAllLangEntries(ttf,info,nid);
	if ( info->fontstyle_name==NULL )
    continue;
	else
    break;
    }
    if ( i==2 ) {
	LogError(_("The 'size' feature does not seem to follow the standard,\nnor does it conform to Adobe's early misinterpretation of\nthe standard. I cannot parse it.\n") );
	info->bad_ot = true;
	info->design_size = info->design_range_bottom = info->design_range_top = info->fontstyle_id = 0;
	info->fontstyle_name = NULL;
    } else if ( i==1 ) {
	LogError(_("The 'size' feature of this font conforms to Adobe's early misinterpretation of the otf standard.\n") );
    }
    fseek(ttf,here,SEEK_SET);
}

static void readttffeatnameparameters(FILE *ttf,int32 pos,uint32 tag,
	struct ttfinfo *info) {
    int version, nid;
    struct otffeatname *fn;
    uint32 here;

    here = ftell(ttf);
    fseek(ttf,pos,SEEK_SET);
    version = getushort(ttf);	/* Minor version #, currently (2009) 0 */
    nid = getushort(ttf);
    fseek(ttf,here,SEEK_SET);

    if ( version>=10 || nid<256 || nid>32767 ) {
	if ( nid<256 || nid>32767 )
	    LogError(_("The name parameter of the '%c%c%c%c' feature does not contain a valid name id.\n"),
		    tag>>24, tag>>16, tag>>8, tag );
	else
	    LogError(_("The name parameter of the '%c%c%c%c' feature has an unlikely version number %d.\n"),
		    tag>>24, tag>>16, tag>>8, tag, version );
	info->bad_ot = true;
return;
    }
    for ( fn=info->feat_names; fn!=NULL && fn->tag!=tag; fn=fn->next );
    if ( fn!=NULL ) {
	if ( fn->nid == nid )
return;
	LogError(_("There are multiple name ids naming the '%c%c%c%c' feature\n this is technically legitimate, but fontforge can't handle it.\n"),
		tag>>24, tag>>16, tag>>8, tag );
return;
    }

    fn = chunkalloc( sizeof(*fn) );
    fn->tag = tag;
    fn->nid = nid;
    fn->next = info->feat_names;
    info->feat_names = fn;
    fn->names = FindAllLangEntries(ttf,info,nid);
}

static struct scripts *readttfscripts(FILE *ttf,int32 pos, struct ttfinfo *info, int isgpos) {
    int i,j,k,cnt;
    int deflang, lcnt;
    struct scripts *scripts;

    if ( pos>=info->g_bounds ) {
	LogError(_("Attempt to read script data beyond end of %s table"), isgpos ? "GPOS" : "GSUB" );
	info->bad_ot = true;
return( NULL );
    }
    fseek(ttf,pos,SEEK_SET);
    cnt = getushort(ttf);
    if ( cnt<=0 )
return( NULL );
    else if ( cnt>1000 ) {
	LogError( _("Too many scripts %d\n"), cnt );
	info->bad_ot = true;
return( NULL );
    }

    scripts = calloc(cnt+1,sizeof(struct scripts));
    for ( i=0; i<cnt; ++i ) {
	scripts[i].tag = getlong(ttf);
	scripts[i].offset = getushort(ttf);
    }
    for ( i=0; i<cnt; ++i ) {
	fseek(ttf,pos+scripts[i].offset,SEEK_SET);
	deflang = getushort(ttf);
	lcnt = getushort(ttf);
	lcnt += (deflang!=0);
	scripts[i].langcnt = lcnt;
	scripts[i].languages = calloc(lcnt+1,sizeof(struct language));
	j = 0;
	if ( deflang!=0 ) {
	    scripts[i].languages[0].tag = CHR('d','f','l','t');
	    scripts[i].languages[0].offset = deflang+scripts[i].offset;
	    ++j;
	}
	for ( ; j<lcnt; ++j ) {
	    scripts[i].languages[j].tag = getlong(ttf);
	    scripts[i].languages[j].offset = scripts[i].offset+getushort(ttf);
	}
	for ( j=0; j<lcnt; ++j ) {
	    if ( pos+scripts[i].languages[j].offset>=info->g_bounds ) {
		LogError(_("Attempt to read script data beyond end of %s table"), isgpos ? "GPOS" : "GSUB" );
		info->bad_ot = true;
return( NULL );
	    }
	    fseek(ttf,pos+scripts[i].languages[j].offset,SEEK_SET);
	    (void) getushort(ttf);	/* lookup ordering table undefined */
	    scripts[i].languages[j].req = getushort(ttf);
	    scripts[i].languages[j].fcnt = getushort(ttf);
	    if ( feof(ttf)) {
		LogError(_("End of file when reading scripts in %s table"), isgpos ? "GPOS" : "GSUB" );
		info->bad_ot = true;
return( NULL );
	    }
	    scripts[i].languages[j].features = malloc(scripts[i].languages[j].fcnt*sizeof(uint16));
	    for ( k=0; k<scripts[i].languages[j].fcnt; ++k )
		scripts[i].languages[j].features[k] = getushort(ttf);
	}
    }

    if ( feof(ttf)) {
	LogError(_("End of file in %s table"), isgpos ? "GPOS" : "GSUB" );
	info->bad_ot = true;
return( NULL );
    }

return( scripts );
}

static struct feature *readttffeatures(FILE *ttf,int32 pos,int isgpos, struct ttfinfo *info) {
    /* read the features table returning an array containing all interesting */
    /*  features */
    int cnt;
    int i,j;
    struct feature *features;
    int parameters;

    if ( pos>=info->g_bounds ) {
	LogError(_("Attempt to read feature data beyond end of %s table"), isgpos ? "GPOS" : "GSUB" );
	info->bad_ot = true;
return( NULL );
    }
    fseek(ttf,pos,SEEK_SET);
    info->feature_cnt = cnt = getushort(ttf);
    if ( cnt<=0 )
return( NULL );
    else if ( cnt>1000 ) {
	LogError( _("Too many features %d\n"), cnt );
	info->bad_ot = true;
return( NULL );
    }

    features = calloc(cnt+1,sizeof(struct feature));
    for ( i=0; i<cnt; ++i ) {
	features[i].tag = getlong(ttf);
	features[i].offset = getushort(ttf);
    }

    for ( i=0; i<cnt; ++i ) {
	if ( pos+features[i].offset>=info->g_bounds ) {
	    LogError(_("Attempt to read feature data beyond end of %s table"), isgpos ? "GPOS" : "GSUB" );
	    info->bad_ot = true;
return( NULL );
	}
	fseek(ttf,pos+features[i].offset,SEEK_SET);
	parameters = getushort(ttf);
	if ( features[i].tag==CHR('s','i','z','e') && parameters!=0 && !feof(ttf)) {
	    readttfsizeparameters(ttf,pos+parameters,
		    pos+parameters+features[i].offset,info);
	} else if ( features[i].tag>=CHR('s','s','0','1') && features[i].tag<=CHR('s','s','2','0') &&
		parameters!=0 && !feof(ttf)) {
	    readttffeatnameparameters(ttf,pos+parameters+features[i].offset,
		    features[i].tag,info);
	}
	features[i].lcnt = getushort(ttf);
	if ( feof(ttf) ) {
	    LogError(_("End of file when reading features in %s table"), isgpos ? "GPOS" : "GSUB" );
	    info->bad_ot = true;
return( NULL );
	}
	features[i].lookups = malloc(features[i].lcnt*sizeof(uint16));
	for ( j=0; j<features[i].lcnt; ++j )
	    features[i].lookups[j] = getushort(ttf);
    }

return( features );
}

static struct lookup *readttflookups(FILE *ttf,int32 pos, struct ttfinfo *info, int isgpos) {
    int cnt,i,j;
    struct lookup *lookups;
    OTLookup *otlookup, *last=NULL;
    struct lookup_subtable *st;

    if ( pos>=info->g_bounds ) {
	LogError(_("Attempt to read lookup data beyond end of %s table"),
		isgpos==2? "JSTF" : isgpos ? "GPOS" : "GSUB" );
	info->bad_ot = true;
return( NULL );
    }

    fseek(ttf,pos,SEEK_SET);
    info->lookup_cnt = cnt = getushort(ttf);
    info->cur_lookups = NULL;
    if ( cnt<=0 )
return( NULL );
    else if ( cnt>1000 ) {
	LogError( _("Too many lookups %d\n"), cnt );
	info->bad_ot = true;
return( NULL );
    }

    lookups = calloc(cnt+1,sizeof(struct lookup));
    for ( i=0; i<cnt; ++i )
	lookups[i].offset = getushort(ttf);
    for ( i=0; i<cnt; ++i ) {
	if ( pos+lookups[i].offset>=info->g_bounds ) {
	    LogError(_("Attempt to read lookup data beyond end of %s table"),
		    isgpos==2? "JSTF" : isgpos ? "GPOS" : "GSUB" );
	    info->bad_ot = true;
return( NULL );
	}
	fseek(ttf,pos+lookups[i].offset,SEEK_SET);
	lookups[i].type = getushort(ttf);
	lookups[i].flags = getushort(ttf);
	lookups[i].subtabcnt = getushort(ttf);
	lookups[i].subtab_offsets = malloc(lookups[i].subtabcnt*sizeof(int32));
	for ( j=0; j<lookups[i].subtabcnt; ++j )
	    lookups[i].subtab_offsets[j] = pos+lookups[i].offset+getushort(ttf);
	if ( lookups[i].flags&pst_usemarkfilteringset )
	    lookups[i].flags |= (getushort(ttf)<<16);

	lookups[i].otlookup = otlookup = chunkalloc(sizeof(OTLookup));
	otlookup->lookup_index = i;
	if ( last==NULL )
	    info->cur_lookups = otlookup;
	else
	    last->next = otlookup;
	last = otlookup;
	otlookup->lookup_type = ((isgpos>0)<<8) | lookups[i].type;
	otlookup->lookup_flags = lookups[i].flags;
	otlookup->lookup_index = i;
	if ( feof(ttf) ) {
	    LogError(_("End of file when reading lookups in %s table"), isgpos ? "GPOS" : "GSUB" );
	    info->bad_ot = true;
return( NULL );
	}
	for ( j=0; j<lookups[i].subtabcnt; ++j ) {
	    st = chunkalloc(sizeof(struct lookup_subtable));
	    st->next = otlookup->subtables;
	    st->lookup = otlookup;
	    otlookup->subtables = st;
	}
    }
    if ( isgpos==2 ) {
	OTLookup *end;
	/* Add any JSTF lookups to the end of the GPOS list. they have the */
	/*  same format, we'll treat them as GPOS internally, and separate */
	/*  them out when we generate the new font */
	if ( info->gpos_lookups==NULL )
	    info->gpos_lookups = info->cur_lookups;
	else {
	    for ( end=info->gpos_lookups; end->next!=NULL; end = end->next );
	    end->next = info->cur_lookups;
	}
    } else if ( isgpos )
	info->gpos_lookups = info->cur_lookups;
    else
	info->gsub_lookups = info->cur_lookups;
return( lookups );
}

static void tagLookupsWithFeature(uint32 script_tag,uint32 lang_tag,
	int required_feature, struct feature *feature, struct lookup *lookups,
	struct ttfinfo *info) {
    uint32 feature_tag = required_feature ? REQUIRED_FEATURE : feature->tag;
    int i;
    OTLookup *otlookup;
    FeatureScriptLangList *fl;

    /* The otf docs are ambiguous as to the capitalization of the default */
    /*  script. The capitalized version is correct (uncapitalized is used for languages) */
    if ( script_tag == DEFAULT_LANG )
	script_tag = DEFAULT_SCRIPT;

    for ( i=0; i < feature->lcnt; ++i ) {
	if ( feature->lookups[i]>=info->lookup_cnt ) {
	    LogError( _("Lookup out of bounds in feature table.\n") );
	    info->bad_ot = true;
	} else {
	    otlookup = lookups[feature->lookups[i]].otlookup;
	    for ( fl = otlookup->features; fl!=NULL && fl->featuretag!=feature_tag; fl=fl->next );
	    if ( fl==NULL ) {
		fl = chunkalloc(sizeof(FeatureScriptLangList));
		fl->featuretag = feature_tag;
		fl->next = otlookup->features;
		otlookup->features = fl;
	    }
	    FListAppendScriptLang(fl,script_tag,lang_tag);
	}
    }
}

static void tagLookupsWithScript(struct scripts *scripts,
	struct feature *features, struct lookup *lookups,struct ttfinfo *info ) {
    int i,j;
    struct scripts *s;
    struct language *lang;
    struct lookup *l;

    if ( scripts==NULL || features==NULL )
return;		/* Legal, I'd guess, but not very interesting. Perhaps all lookups are controlled by the JSTF table or something */

    /* First tag every lookup with all script, lang, feature combinations that*/
    /*  invoke it */
    for ( s=scripts; s->tag!=0; ++s ) {
	for ( lang=s->languages, i=0; i<s->langcnt; ++i, ++lang ) {
	    if ( lang->req==0xffff )
		/* Do Nothing */;
	    else if ( lang->req>= info->feature_cnt ) {
		LogError( _("Required feature out of bounds in script table.\n") );
		info->bad_ot = true;
	    } else
		tagLookupsWithFeature(s->tag,lang->tag,true,&features[lang->req],
			lookups,info);
	    for ( j=0; j<lang->fcnt; ++j ) {
		if ( lang->features[j]>=info->feature_cnt ) {
		    LogError( _("Feature out of bounds in script table.\n") );
		    info->bad_ot = true;
		} else
		    tagLookupsWithFeature(s->tag,lang->tag,false,&features[lang->features[j]],
			    lookups,info);
	    }
	}
    }

    /* The scripts got added backwards so reverse to put them in */
    /*  alphabetic order again */
    for ( l=lookups, i=0; l->offset!=0; ++l, ++i ) {
	OTLookup *otl = l->otlookup;
	FeatureScriptLangList *fl;
	struct scriptlanglist *sl, *next, *prev;
	for ( fl=otl->features; fl!=NULL; fl=fl->next ) {
	    prev = NULL;
	    for ( sl=fl->scripts; sl!=NULL; sl = next ) {
		next = sl->next;
		sl->next = prev;
		prev = sl;
	    }
	    fl->scripts = prev;
	}
    }
}

static void gposExtensionSubTable(FILE *ttf, int stoffset,
	struct ttfinfo *info, struct lookup *l, struct lookup_subtable *subtable,
	struct lookup *alllooks) {
    uint32 base = ftell(ttf), st, offset;
    int lu_type;

    /* Format = */ getushort(ttf);
    lu_type = getushort(ttf);
    offset = getlong(ttf);

    l->otlookup->lookup_type = 0x100|lu_type;

    fseek(ttf,st = base+offset,SEEK_SET);
    switch ( lu_type ) {
      case 1:
	gposSimplePos(ttf,st,info,l,subtable);
      break;  
      case 2:
	gposKernSubTable(ttf,st,info,l,subtable);
      break;  
      case 3:
	gposCursiveSubTable(ttf,st,info,subtable);
      break;
      case 4: case 5: case 6:
	gposMarkSubTable(ttf,st,info,l,subtable);
      break;
      case 7:
	gposContextSubTable(ttf,st,info,l,subtable,alllooks);
      break;
      case 8:
	gposChainingSubTable(ttf,st,info,l,subtable,alllooks);
      break;
      case 9:
	LogError( _("This font is erroneous: it has a GPOS extension subtable that points to\nanother extension sub-table.\n") );
	info->bad_ot = true;
      break;
/* Any cases added here also need to go in the gposLookupSwitch */
      default:
	LogError( _("Unknown GPOS sub-table type: %d\n"), lu_type );
	info->bad_ot = true;
      break;
    }
    if ( ftell(ttf)>info->g_bounds ) {
	LogError( _("Subtable extends beyond end of GPOS table\n") );
	info->bad_ot = true;
    }
}

static void gsubExtensionSubTable(FILE *ttf, int stoffset,
	struct ttfinfo *info, struct lookup *l, struct lookup_subtable *subtable, int justinuse,
	struct lookup *alllooks) {
    uint32 base = ftell(ttf), st, offset;
    int lu_type;

    /* Format = */ getushort(ttf);
    lu_type = getushort(ttf);
    offset = getlong(ttf);

    l->otlookup->lookup_type = lu_type;

    fseek(ttf,st = base+offset,SEEK_SET);
    switch ( lu_type ) {
      case 1:
	gsubSimpleSubTable(ttf,st,info,l,subtable,justinuse);
      break;
      case 2: case 3:	/* Multiple and alternate have same format, different semantics */
	gsubMultipleSubTable(ttf,st,info,l,subtable,justinuse);
      break;
      case 4:
	gsubLigatureSubTable(ttf,st,info,l,subtable,justinuse);
      break;
      case 5:
	gsubContextSubTable(ttf,st,info,l,subtable,justinuse,alllooks);
      break;
      case 6:
	gsubChainingSubTable(ttf,st,info,l,subtable,justinuse,alllooks);
      break;
      case 7:
	LogError( _("This font is erroneous: it has a GSUB extension subtable that points to\nanother extension sub-table.\n") );
	info->bad_ot = true;
      break;
      case 8:
	gsubReverseChainSubTable(ttf,st,info,l,subtable,justinuse);
      break;
/* Any cases added here also need to go in the gsubLookupSwitch */
      default:
	LogError( _("Unknown GSUB sub-table type: %d\n"), lu_type );
	info->bad_ot = true;
      break;
    }
    if ( ftell(ttf)>info->gsub_start+info->gsub_length ) {
	LogError( _("Subtable extends beyond end of GSUB table\n") );
	info->bad_ot = true;
    }
}

static void gposLookupSwitch(FILE *ttf, int st,
	struct ttfinfo *info, struct lookup *l, struct lookup_subtable *subtable,
	struct lookup *alllooks) {

    switch ( l->type | 0x100 ) {
      case gpos_single:
	gposSimplePos(ttf,st,info,l,subtable);
      break;  
      case gpos_pair:
	gposKernSubTable(ttf,st,info,l,subtable);
      break;  
      case gpos_cursive:
	gposCursiveSubTable(ttf,st,info,subtable);
      break;
      case gpos_mark2base: case gpos_mark2ligature: case gpos_mark2mark:
	gposMarkSubTable(ttf,st,info,l,subtable);
      break;
      case gpos_context:
	gposContextSubTable(ttf,st,info,l,subtable,alllooks);
      break;
      case gpos_contextchain:
	gposChainingSubTable(ttf,st,info,l,subtable,alllooks);
      break;
      case 0x109:
	gposExtensionSubTable(ttf,st,info,l,subtable,alllooks);
      break;
/* Any cases added here also need to go in the gposExtensionSubTable */
      default:
	LogError( _("Unknown GPOS sub-table type: %d\n"), l->otlookup->lookup_type );
	info->bad_ot = true;
      break;
    }
    if ( ftell(ttf)>info->g_bounds ) {
	LogError( _("Subtable extends beyond end of GPOS table\n") );
	info->bad_ot = true;
    }
}

static void gsubLookupSwitch(FILE *ttf, int st,
	struct ttfinfo *info, struct lookup *l, struct lookup_subtable *subtable, int justinuse,
	struct lookup *alllooks) {

    switch ( l->type ) {
      case gsub_single:
	gsubSimpleSubTable(ttf,st,info,l,subtable,justinuse);
      break;
      case gsub_multiple: case gsub_alternate:	/* Multiple and alternate have same format, different semantics */
	gsubMultipleSubTable(ttf,st,info,l,subtable,justinuse);
      break;
      case gsub_ligature:
	gsubLigatureSubTable(ttf,st,info,l,subtable,justinuse);
      break;
      case gsub_context:
	gsubContextSubTable(ttf,st,info,l,subtable,justinuse,alllooks);
      break;
      case gsub_contextchain:
	gsubChainingSubTable(ttf,st,info,l,subtable,justinuse,alllooks);
      break;
      case 7:
	gsubExtensionSubTable(ttf,st,info,l,subtable,justinuse,alllooks);
      break;
      case gsub_reversecchain:
	gsubReverseChainSubTable(ttf,st,info,l,subtable,justinuse);
      break;
/* Any cases added here also need to go in the gsubExtensionSubTable */
      default:
	LogError( _("Unknown GSUB sub-table type: %d\n"), l->otlookup->lookup_type );
	info->bad_ot = true;
      break;
    }
    if ( ftell(ttf)>info->g_bounds ) {
	LogError( _("Subtable extends beyond end of GSUB table\n" ));
	info->bad_ot = true;
    }
}

static void ScriptsFree(struct scripts *scripts) {
    int i,j;

    if ( scripts==NULL )
return;
    for ( i=0; scripts[i].offset!=0 ; ++i ) {
	for ( j=0; j<scripts[i].langcnt; ++j )
	    free( scripts[i].languages[j].features);
	free(scripts[i].languages);
    }
    free(scripts);
}

static void FeaturesFree(struct feature *features) {
    int i;

    if ( features==NULL )
return;
    for ( i=0; features[i].offset!=0 ; ++i )
	free(features[i].lookups);
    free(features);
}

static void LookupsFree(struct lookup *lookups) {
    int i;

    for ( i=0; lookups[i].offset!=0 ; ++i ) {
	free( lookups[i].subtab_offsets );
    }
    free(lookups);
}

static void ProcessGPOSGSUB(FILE *ttf,struct ttfinfo *info,int gpos,int inusetype) {
    int k;
    int32 base, lookup_start, st;
    int32 script_off, feature_off;
    struct scripts *scripts;
    struct feature *features;
    struct lookup *lookups, *l;
    struct lookup_subtable *subtable;

    if ( gpos ) {
	base = info->gpos_start;
	info->g_bounds = base + info->gpos_length;
    } else {
	base = info->gsub_start;
	info->g_bounds = base + info->gsub_length;
    }
    fseek(ttf,base,SEEK_SET);
    /* version = */ getlong(ttf);
    script_off = getushort(ttf);
    feature_off = getushort(ttf);
    lookup_start = base+getushort(ttf);

    scripts = readttfscripts(ttf,base+script_off,info,gpos);
    features = readttffeatures(ttf,base+feature_off,gpos,info);
    /* It is legal to have lookups with no features or scripts */
    /* For example if all the lookups were controlled by the JSTF table */
    lookups = readttflookups(ttf,lookup_start,info,gpos);
    if ( lookups==NULL ) {
	ScriptsFree(scripts);
	FeaturesFree(features);
return;
    }
    tagLookupsWithScript(scripts,features,lookups,info);
    ScriptsFree(scripts); scripts = NULL;
    FeaturesFree(features); features = NULL;

    for ( l = lookups; l->offset!=0; ++l ) {
	for ( k=0, subtable=l->otlookup->subtables; k<l->subtabcnt; ++k, subtable=subtable->next ) {
	    st = l->subtab_offsets[k];
	    fseek(ttf,st,SEEK_SET);
	    if ( gpos ) {
		gposLookupSwitch(ttf,st,info,l,subtable,lookups);
	    } else {
		gsubLookupSwitch(ttf,st,info,l,subtable,inusetype,lookups);
	    }
	}
    }

    /* Then generate some user-friendly names for the all the lookups */
    if ( inusetype==git_normal )
	for ( l=lookups; l->offset!=0; ++l )
	    NameOTLookup(l->otlookup,NULL);

    LookupsFree(lookups);
    if ( inusetype!=git_normal && !gpos ) {
	OTLookupListFree(info->gsub_lookups);
	info->gsub_lookups = info->cur_lookups = NULL;
    }
}

void readttfgsubUsed(FILE *ttf,struct ttfinfo *info) {
    ProcessGPOSGSUB(ttf,info,false,git_justinuse);
    info->g_bounds = 0;
}

void GuessNamesFromGSUB(FILE *ttf,struct ttfinfo *info) {
    ProcessGPOSGSUB(ttf,info,false,git_findnames);
    info->g_bounds = 0;
}

void readttfgpossub(FILE *ttf,struct ttfinfo *info,int gpos) {
    ProcessGPOSGSUB(ttf,info,gpos,git_normal);
    info->g_bounds = 0;
}

void readttfgdef(FILE *ttf,struct ttfinfo *info) {
    int lclo, gclass, mac, mas=0;
    int coverage, cnt, i,j, format;
    int version;
    uint16 *glyphs, *lc_offsets, *offsets;
    uint32 caret_base;
    PST *pst;
    SplineChar *sc;

    fseek(ttf,info->gdef_start,SEEK_SET);
    version = getlong(ttf);
    if ( version!=0x00010000 && version != 0x00010002 )
return;
    info->g_bounds = info->gdef_start + info->gdef_length;
    gclass = getushort(ttf);
    /* attach list = */ getushort(ttf);
    lclo = getushort(ttf);		/* ligature caret list */
    mac = getushort(ttf);		/* mark attach class */
    if ( version==0x00010002 )
	mas = getushort(ttf);

    if ( gclass!=0 ) {
	uint16 *gclasses = getClassDefTable(ttf,info->gdef_start+gclass, info);
	for ( i=0; i<info->glyph_cnt; ++i )
	    if ( info->chars[i]!=NULL && gclasses[i]!=0 )
		info->chars[i]->glyph_class = gclasses[i]+1;
	free(gclasses);
    }

    if ( mac!=0 ) {
	uint16 *mclasses = getClassDefTable(ttf,info->gdef_start+mac, info);
	const char *format_spec = _("MarkClass-%d");
	info->mark_class_cnt = ClassFindCnt(mclasses,info->glyph_cnt);
	info->mark_classes = ClassToNames(info,info->mark_class_cnt,mclasses,info->glyph_cnt);
	info->mark_class_names = malloc(info->mark_class_cnt*sizeof(char *));
	info->mark_class_names[0] = NULL;
	for ( i=1; i<info->mark_class_cnt; ++i ) {
	    info->mark_class_names[i] = malloc((strlen(format_spec)+10));
	    sprintf( info->mark_class_names[i], format_spec, i );
	}
	free(mclasses);
    }
    if ( mas!=0 ) {
	const char *format_spec = _("MarkSet-%d");
	fseek(ttf,info->gdef_start+mas,SEEK_SET);
	if ( getushort(ttf)==1 ) {	/* Version number of Mark GLyph Sets Table */
	    uint32 *offsets;
	    uint16 *glyphs;
	    info->mark_set_cnt = getushort(ttf);
	    offsets = malloc(info->mark_set_cnt*sizeof(uint32));
	    for ( i=0; i<info->mark_set_cnt; ++i )
		offsets[i]=getlong(ttf);
	    info->mark_sets = malloc(info->mark_set_cnt*sizeof(char *));
	    info->mark_set_names = malloc(info->mark_set_cnt*sizeof(char *));
	    info->mark_set_names[0] = NULL;
	    for ( i=0; i<info->mark_set_cnt; ++i ) {
		info->mark_set_names[i] = malloc((strlen(format_spec)+10));
		sprintf( info->mark_set_names[i], format_spec, i );
		if ( offsets[i]!=0 ) {
		    glyphs = getCoverageTable(ttf,info->gdef_start+mas+offsets[i],info);
		    info->mark_sets[i] = GlyphsToNames(info,glyphs,true);
		    free(glyphs);
		} else
		    info->mark_sets[i] = NULL;		/* Should not happen */
	    }
	    free(offsets);
	}
    }

    if ( lclo!=0 ) {
	lclo += info->gdef_start;
	fseek(ttf,lclo,SEEK_SET);
	coverage = getushort(ttf);
	cnt = getushort(ttf);
	if ( cnt==0 )
return;
	lc_offsets = malloc(cnt*sizeof(uint16));
	for ( i=0; i<cnt; ++i )
	    lc_offsets[i]=getushort(ttf);
	glyphs = getCoverageTable(ttf,lclo+coverage,info);
	if ( glyphs==NULL ) {
        free(lc_offsets);
        return;
    }
	for ( i=0; i<cnt; ++i ) if ( glyphs[i]<info->glyph_cnt && (sc = info->chars[glyphs[i]])!=NULL ) {
	    fseek(ttf,lclo+lc_offsets[i],SEEK_SET);
	    for ( pst=sc->possub; pst!=NULL && pst->type!=pst_lcaret; pst=pst->next );
	    if ( pst==NULL ) {
		pst = chunkalloc(sizeof(PST));
		pst->next = sc->possub;
		sc->possub = pst;
		pst->type = pst_lcaret;
		pst->subtable = NULL;
		sc->lig_caret_cnt_fixed = true;
	    }
	    caret_base = ftell(ttf);
	    pst->u.lcaret.cnt = getushort(ttf);
	    free(pst->u.lcaret.carets);
	    offsets = malloc(pst->u.lcaret.cnt*sizeof(uint16));
	    for ( j=0; j<pst->u.lcaret.cnt; ++j )
		offsets[j] = getushort(ttf);
	    pst->u.lcaret.carets = malloc(pst->u.lcaret.cnt*sizeof(int16));
	    for ( j=0; j<pst->u.lcaret.cnt; ++j ) {
		fseek(ttf,caret_base+offsets[j],SEEK_SET);
		format=getushort(ttf);
		if ( format==1 ) {
		    pst->u.lcaret.carets[j] = getushort(ttf);
		} else if ( format==2 ) {
		    pst->u.lcaret.carets[j] = 0;
		    /* point = */ getushort(ttf);
		} else if ( format==3 ) {
		    pst->u.lcaret.carets[j] = getushort(ttf);
		    /* in device table = */ getushort(ttf);
		} else {
		    LogError( _("!!!! Unknown caret format %d !!!!\n"), format );
		    info->bad_ot = true;
		}
	    }
	    free(offsets);
	}
	free(lc_offsets);
	free(glyphs);
    }
    info->g_bounds = 0;
}

static void readttf_applelookup(FILE *ttf,struct ttfinfo *info,
	void (*apply_values)(struct ttfinfo *info, int gfirst, int glast,FILE *ttf),
	void (*apply_value)(struct ttfinfo *info, int gfirst, int glast,FILE *ttf),
	void (*apply_default)(struct ttfinfo *info, int gfirst, int glast,void *def),
	void *def, int allow_out_of_bounds) {
    int format, i, first, last, data_off, cnt, prev;
    uint32 here;
    uint32 base = ftell(ttf);

    format = getushort(ttf);
    switch ( format ) {
      case 0:	/* Simple array */
	apply_values(info,0,info->glyph_cnt-1,ttf);
      break;
      case 2:	/* Segment Single */
	/* Entry size  */ getushort(ttf);
	cnt = getushort(ttf);
	/* search range */ getushort(ttf);
	/* log2(cnt)    */ getushort(ttf);
	/* range shift  */ getushort(ttf);
	prev = 0;
	for ( i=0; i<cnt; ++i ) {
	    last = getushort(ttf);
	    first = getushort(ttf);
	    if ( last<first || last>=0xffff ||
		    (!allow_out_of_bounds && last>=info->glyph_cnt )) {
		LogError( _("Bad lookup table: format=2 (%d/%d), first=%d last=%d total glyphs in font=%d\n"),
			i,cnt,first,last,info->glyph_cnt );
		info->bad_gx = true;
	    } else {
		if ( apply_default!=NULL )
		    apply_default(info,prev,first-1,def);
		apply_value(info,first,last,ttf);
		prev = last+1;
	    }
	}
      break;
      case 4:	/* Segment multiple */
	/* Entry size  */ getushort(ttf);
	cnt = getushort(ttf);
	/* search range */ getushort(ttf);
	/* log2(cnt)    */ getushort(ttf);
	/* range shift  */ getushort(ttf);
	prev = 0;
	for ( i=0; i<cnt; ++i ) {
	    last = getushort(ttf);
	    first = getushort(ttf);
	    data_off = getushort(ttf);
	    if ( last<first || last>=0xffff ||
		    (!allow_out_of_bounds && last>=info->glyph_cnt )) {
		LogError( _("Bad lookup table: format=4 (%d/%d), first=%d last=%d total glyphs in font=%d\n"),
			i,cnt,first,last,info->glyph_cnt );
		info->bad_gx = true;
	    } else {
		here = ftell(ttf);
		if ( apply_default!=NULL )
		    apply_default(info,prev,first-1,def);
		fseek(ttf,base+data_off,SEEK_SET);
		apply_values(info,first,last,ttf);
		fseek(ttf,here,SEEK_SET);
		prev = last+1;
	    }
	}
      break;
      case 6:	/* Single table */
	/* Entry size  */ getushort(ttf);
	cnt = getushort(ttf);
	/* search range */ getushort(ttf);
	/* log2(cnt)    */ getushort(ttf);
	/* range shift  */ getushort(ttf);
	prev = 0;
	for ( i=0; i<cnt; ++i ) {
	    first = getushort(ttf);
	    if ( first>=0xffff || (!allow_out_of_bounds && first>=info->glyph_cnt )) {
		LogError( _("Bad lookup table: format=6, first=%d total glyphs in font=%d\n"),
			first,info->glyph_cnt );
		info->bad_gx = true;
	    } else {
		if ( apply_default!=NULL )
		    apply_default(info,prev,first-1,def);
		apply_value(info,first,first,ttf);
		prev = first+1;
	    }
	}
      break;
      case 8:	/* Simple array */
	first = getushort(ttf);
	cnt = getushort(ttf);
	if ( first+cnt>=0xffff || (!allow_out_of_bounds && first+cnt>=info->glyph_cnt )) {
	    LogError( _("Bad lookup table: format=8, first=%d cnt=%d total glyphs in font=%d\n"),
		    first,cnt,info->glyph_cnt );
	    info->bad_gx = true;
	} else {
	    if ( apply_default!=NULL ) {
		apply_default(info,0,first-1,def);
		apply_default(info,first+cnt,info->glyph_cnt-1,def);
	    }
	    apply_values(info,first,first+cnt-1,ttf);
	}
      break;
      default:
	LogError( _("Invalid lookup table format. %d\n"), format );
	info->bad_gx = true;
      break;
    }
}

static void OTLAppend(struct ttfinfo *info,OTLookup *otl,int gpos) {
    OTLookup *prev;
    int pos=0;

    if ( gpos && info->gpos_lookups == NULL )
	info->gpos_lookups = otl;
    else if ( !gpos && info->gsub_lookups == NULL )
	info->gsub_lookups = otl;
    else {
	prev = gpos ? info->gpos_lookups : info->gsub_lookups;
	pos = 1;
	while ( prev->next!=NULL ) {
	    prev = prev->next;
	    ++pos;
	}
	prev->next = otl;
    }
    otl->lookup_index = pos;
}

static void OTLRemove(struct ttfinfo *info,OTLookup *otl,int gpos) {
    /* Remove the most recent lookup. We got bad data and can't use it */
    OTLookup *prev, **base;

    base = gpos ? &info->gpos_lookups : &info->gsub_lookups;
    if ( *base==otl )
	*base = NULL;
    else if ( *base!=NULL ) {
	for ( prev = *base; prev->next!=NULL && prev->next!=otl; prev = prev->next );
	prev->next = NULL;
    }
    OTLookupFree(otl);
}

static OTLookup *NewMacLookup(struct ttfinfo *info,int gpos) {
    OTLookup *otl;

    otl = chunkalloc(sizeof(OTLookup));
    otl->lookup_type = gpos ? kern_statemachine : morx_context;
    otl->subtables = chunkalloc(sizeof(struct lookup_subtable));
    otl->subtables->lookup = otl;
    otl->features = chunkalloc(sizeof(FeatureScriptLangList));
    if ( gpos ) {
	otl->features->featuretag = CHR('k','e','r','n');
    } else {
	otl->features->featuretag = (info->mort_feat<<16) | (info->mort_setting);
	otl->features->ismac = true;
    }
    OTLAppend(info,otl,gpos);
return( otl );
}

static OTLookup *NewMacSubsLookup(struct ttfinfo *info,OTLookup *parent,
	int nest_index, OTLookup **subs) {
    OTLookup *otl;
    char *name, *format;

    if ( subs[nest_index]!=NULL )
return( subs[nest_index] );

    /* These are nested lookups, only to be activated by a state machine */
    /*  as such they have no feature tags nor scripts of their own. */
    otl = chunkalloc(sizeof(OTLookup));
    otl->lookup_type = gsub_single;
    otl->subtables = chunkalloc(sizeof(struct lookup_subtable));
    otl->subtables->lookup = otl;
/* GT: This is to give the name to a nested substitution lookup invoked by */
/* GT: a statemachine. The %s is the name of the statemachine('s lookup) */
/* GT: and the %d is n, where this lookup is the n'th defined for this state */
/* GT: machine */
    format = _("%s nested-substitutions %d");
    name = malloc(strlen(parent->lookup_name)+strlen(format)+10);
    sprintf( name, format, parent->lookup_name, nest_index );
    otl->lookup_name = name;
    otl->subtables->subtable_name = strconcat3(name," ",_("subtable"));
    OTLAppend(info,otl,false);
    subs[nest_index] = otl;
return( otl );
}

static void InfoNameOTLookup(OTLookup *otl,struct ttfinfo *info) {
    SplineFont sf;

    memset(&sf,0,sizeof(sf));
    sf.features = info->features;
    NameOTLookup(otl,&sf);
}

static void TTF_SetProp(struct ttfinfo *info,int gnum, int prop) {
    int offset;
    PST *pst;

    if ( gnum<0 || gnum>=info->glyph_cnt ) {
	LogError( _("Glyph out of bounds in 'prop' table %d\n"), gnum );
	info->bad_gx = true;
return;
    }

    if ( prop&0x1000 ) {	/* Mirror */
	offset = (prop<<20)>>28;
	if ( gnum+offset>=0 && gnum+offset<info->glyph_cnt &&
		info->chars[gnum]!=NULL &&
		info->chars[gnum+offset]!=NULL &&
		info->chars[gnum+offset]->name!=NULL ) {
	    pst = chunkalloc(sizeof(PST));
	    pst->type = pst_substitution;
	    pst->subtable = info->mort_subs_lookup->subtables;
	    FListAppendScriptLang(info->mort_subs_lookup->features,SCScriptFromUnicode(info->chars[gnum]),
		    DEFAULT_LANG);
	    pst->next = info->chars[gnum]->possub;
	    info->chars[gnum]->possub = pst;
	    pst->u.subs.variant = copy(info->chars[gnum+offset]->name);
	}
    }
}

static void prop_apply_values(struct ttfinfo *info, int gfirst, int glast,FILE *ttf) {
    int i;

    for ( i=gfirst; i<=glast; ++i )
	TTF_SetProp(info,i, getushort(ttf));
}

static void prop_apply_value(struct ttfinfo *info, int gfirst, int glast,FILE *ttf) {
    int i;
    int prop;

    prop = getushort(ttf);
    for ( i=gfirst; i<=glast; ++i )
	TTF_SetProp(info,i, prop);
}

static void prop_apply_default(struct ttfinfo *info, int gfirst, int glast,void *def) {
    int def_prop, i;

    def_prop = (intpt) def;
    for ( i=gfirst; i<=glast; ++i )
	TTF_SetProp(info,i, def_prop);
}

void readttfprop(FILE *ttf,struct ttfinfo *info) {
    int def;

    fseek(ttf,info->prop_start,SEEK_SET);
    /* The one example that I've got has a wierd version, so I don't check it */
    /*  the three versions that I know about are all pretty much the same, just a few extra flags */
    /* version = */ getlong(ttf);
    /* format = */ getushort(ttf);
    def = getushort(ttf);
    info->mort_subs_lookup = NewMacLookup(info,false);
    info->mort_subs_lookup->lookup_type = gsub_single;
    info->mort_subs_lookup->features->featuretag = CHR('r','t','l','a');
    info->mort_subs_lookup->features->ismac = false;
    info->mort_subs_lookup->subtables->per_glyph_pst_or_kern = true;
    readttf_applelookup(ttf,info,
	    prop_apply_values,prop_apply_value,
	    prop_apply_default,(void *) (intpt) def, false);
    InfoNameOTLookup(info->mort_subs_lookup,info);
}

static void TTF_SetLcaret(struct ttfinfo *info, int gnum, int offset, FILE *ttf) {
    uint32 here = ftell(ttf);
    PST *pst;
    SplineChar *sc;
    int cnt, i;

    if ( gnum<0 || gnum>=info->glyph_cnt ) {
	LogError( _("Glyph out of bounds in 'lcar' table %d\n"), gnum );
	info->bad_gx = true;
return;
    } else if ( (sc=info->chars[gnum])==NULL )
return;

    fseek(ttf,info->lcar_start+offset,SEEK_SET);
    cnt = getushort(ttf);
    pst = chunkalloc(sizeof(PST));
    pst->type = pst_lcaret;
    pst->subtable = NULL;
    pst->next = sc->possub;
    sc->possub = pst;
    sc->lig_caret_cnt_fixed = true;
    pst->u.lcaret.cnt = cnt;
    pst->u.lcaret.carets = malloc(cnt*sizeof(uint16));
    for ( i=0; i<cnt; ++i )
	pst->u.lcaret.carets[i] = getushort(ttf);
    fseek(ttf,here,SEEK_SET);
}

static void lcar_apply_values(struct ttfinfo *info, int gfirst, int glast,FILE *ttf) {
    int i;

    for ( i=gfirst; i<=glast; ++i )
	TTF_SetLcaret(info,i, getushort(ttf), ttf);
}

static void lcar_apply_value(struct ttfinfo *info, int gfirst, int glast,FILE *ttf) {
    int i;
    int offset;

    offset = getushort(ttf);
    for ( i=gfirst; i<=glast; ++i )
	TTF_SetLcaret(info,i, offset, ttf);
}
    
void readttflcar(FILE *ttf,struct ttfinfo *info) {

    fseek(ttf,info->lcar_start,SEEK_SET);
    /* version = */ getlong(ttf);
    if ( getushort(ttf)!=0 )	/* A format type of 1 has the caret locations */
return;				/*  indicated by points */
    readttf_applelookup(ttf,info,
	    lcar_apply_values,lcar_apply_value,NULL,NULL,false);
}

static void TTF_SetOpticalBounds(struct ttfinfo *info, int gnum, int left, int right) {
    PST *pst;
    SplineChar *sc;

    if ( left==0 && right==0 )
return;

    if ( gnum<0 || gnum>=info->glyph_cnt ) {
	LogError( _("Glyph out of bounds in 'opbd' table %d\n"), gnum );
	info->bad_gx = true;
return;
    } else if ( (sc=info->chars[gnum])==NULL )
return;

    if ( left!=0 ) {
	pst = chunkalloc(sizeof(PST));
	pst->type = pst_position;
	pst->subtable = info->mort_subs_lookup->subtables;
	FListAppendScriptLang(info->mort_subs_lookup->features,SCScriptFromUnicode(sc),
		DEFAULT_LANG);
	pst->next = sc->possub;
	sc->possub = pst;
	pst->u.pos.xoff = left;
	pst->u.pos.h_adv_off = left;
    }
    if ( right!=0 ) {
	pst = chunkalloc(sizeof(PST));
	pst->type = pst_position;
	pst->subtable = info->mort_pos_lookup2->subtables;
	FListAppendScriptLang(info->mort_pos_lookup2->features,SCScriptFromUnicode(sc),
		DEFAULT_LANG);
	pst->next = sc->possub;
	sc->possub = pst;
	pst->u.pos.h_adv_off = -right;
    }
}

static void opbd_apply_values(struct ttfinfo *info, int gfirst, int glast,FILE *ttf) {
    int i, left, right, offset;
    uint32 here;

    for ( i=gfirst; i<=glast; ++i ) {
	offset = getushort(ttf);
	here = ftell(ttf);
	fseek(ttf,info->opbd_start+6/*opbd header*/+offset,SEEK_SET);
	left = (int16) getushort(ttf);
	/* top = (int16) */ getushort(ttf);
	right = (int16) getushort(ttf);
	/* bottom = (int16) */ getushort(ttf);
	fseek(ttf,here,SEEK_SET);
	TTF_SetOpticalBounds(info,i, left, right);
    }
}

static void opbd_apply_value(struct ttfinfo *info, int gfirst, int glast,FILE *ttf) {
    int i, left, right, offset;
    uint32 here;

    offset = getushort(ttf);
    here = ftell(ttf);
    fseek(ttf,info->opbd_start+offset,SEEK_SET);
    left = (int16) getushort(ttf);
    /* top = (int16) */ getushort(ttf);
    right = (int16) getushort(ttf);
    /* bottom = (int16) */ getushort(ttf);
    fseek(ttf,here,SEEK_SET);

    for ( i=gfirst; i<=glast; ++i )
	TTF_SetOpticalBounds(info,i, left, right);
}
    
void readttfopbd(FILE *ttf,struct ttfinfo *info) {

    fseek(ttf,info->opbd_start,SEEK_SET);
    /* version = */ getlong(ttf);
    if ( getushort(ttf)!=0 )	/* A format type of 1 has the bounds */
return;				/*  indicated by points */
    info->mort_subs_lookup = NewMacLookup(info,true);
    info->mort_subs_lookup->lookup_type = gpos_single;
    info->mort_subs_lookup->features->featuretag = CHR('l','f','b','d');
    info->mort_subs_lookup->features->ismac = false;
    info->mort_subs_lookup->subtables->per_glyph_pst_or_kern = true;

    info->mort_pos_lookup2 = NewMacLookup(info,true);
    info->mort_pos_lookup2->lookup_type = gpos_single;
    info->mort_pos_lookup2->features->featuretag = CHR('r','t','b','d');
    info->mort_pos_lookup2->features->ismac = false;
    info->mort_pos_lookup2->subtables->per_glyph_pst_or_kern = true;

    readttf_applelookup(ttf,info,
	    opbd_apply_values,opbd_apply_value,NULL,NULL,false);
    InfoNameOTLookup(info->mort_subs_lookup,info);
    InfoNameOTLookup(info->mort_pos_lookup2,info);
}

/* Interesting. The mac allows the creation of temporary gids beyond the */
/*  range specified by the font, as long as the user never sees them. So */
/*  it seems perfectly legal for one substitution to use a gid of 1111   */
/*  if that gid never reaches output but will be converted into a real gid */
/*  by a subsequent substitution. I saw this used in a conditional situation */
/*  to provide a temporary context for a later match. */
static SplineChar *CreateBadGid(struct ttfinfo *info,int badgid) {
    int i;
    SplineChar *fake;
    char name[60];

    if ( badgid<0 || badgid>=0xffff )		/* <0 should never happen, 0xffff is the special "deleted" glyph, >0xffff should never happen */
return( NULL );

    for ( i=0; i<info->badgid_cnt; ++i )
	if ( info->badgids[i]->orig_pos == badgid )
return( info->badgids[i] );

    if ( info->badgid_cnt>=info->badgid_max )
	info->badgids = realloc(info->badgids,(info->badgid_max += 20)*sizeof(SplineChar *));
    fake = SplineCharCreate(2);
    fake->orig_pos = badgid;
    sprintf( name, "Out-Of-Range-GID-%d", badgid );
    fake->name = copy(name);
    fake->widthset = true;		/* So it doesn't just vanish on us */
    fake->width = fake->vwidth = info->emsize;
    info->badgids[info->badgid_cnt++] = fake;
return( fake );
}

static void TTF_SetMortSubs(struct ttfinfo *info, int gnum, int gsubs) {
    PST *pst;
    SplineChar *sc, *ssc;

    if ( gsubs==0 )
return;

    if ( gnum<0 || gnum>=info->glyph_cnt ) {
	if ( info->justinuse != git_normal )
return;
	if ( !info->warned_morx_out_of_bounds_glyph ) {
	    LogError( _("Glyph out of bounds in 'mort'/'morx' table %d\n"), gnum );
	    info->bad_gx = true;
	    info->warned_morx_out_of_bounds_glyph = true;
	}
	sc = CreateBadGid(info,gnum);
    } else
	sc = info->chars[gnum];
    ssc = NULL;
    if ( gsubs<0 || (gsubs>=info->glyph_cnt && gsubs!=0xffff)) {
	if ( info->justinuse != git_normal )
return;
	if ( !info->warned_morx_out_of_bounds_glyph ) {
	    LogError( _("Substitute glyph out of bounds in 'mort'/'morx' table %d\n"), gsubs );
	    info->bad_gx = true;
	    info->warned_morx_out_of_bounds_glyph = true;
	}
	ssc = CreateBadGid(info,gsubs);
    } else if ( gsubs!=0xffff ) {
	if ( info->justinuse == git_justinuse ) {
	    info->inuse[gsubs] = true;
return;
	}
	ssc=info->chars[gsubs];
    }
    if ( sc==NULL || (gsubs!=0xffff && ssc==NULL) )
return;

    pst = chunkalloc(sizeof(PST));
    pst->type = pst_substitution;
    pst->subtable = info->mort_subs_lookup->subtables;
    if ( info->mort_subs_lookup->features!=NULL )
	FListsAppendScriptLang(info->mort_subs_lookup->features,SCScriptFromUnicode(sc),
		DEFAULT_LANG);
    pst->next = sc->possub;
    sc->possub = pst;
    pst->u.subs.variant = gsubs!=0xffff ? copy(ssc->name) : copy(MAC_DELETED_GLYPH_NAME);
}

static void mort_apply_values(struct ttfinfo *info, int gfirst, int glast,FILE *ttf) {
    uint16 gnum;
    int i;

    for ( i=gfirst; i<=glast; ++i ) {
	gnum = getushort(ttf);
	TTF_SetMortSubs(info,i, gnum);
    }
}

static void mort_apply_value(struct ttfinfo *info, int gfirst, int glast,FILE *ttf) {
    uint16 gnum;
    int i;

    gnum = getushort(ttf);

    for ( i=gfirst; i<=glast; ++i )
	TTF_SetMortSubs(info,i, gnum );
}

static void mortclass_apply_values(struct ttfinfo *info, int gfirst, int glast,FILE *ttf) {
    int i;

    for ( i=gfirst; i<=glast; ++i )
	info->morx_classes[i] = getushort(ttf);
}

static void mortclass_apply_value(struct ttfinfo *info, int gfirst, int glast,FILE *ttf) {
    uint16 class;
    int i;

    class = getushort(ttf);

    for ( i=gfirst; i<=glast; ++i )
	info->morx_classes[i] = class;
}

#define MAX_LIG_COMP	16
struct statemachine {
    uint8 *data;
    int length;
    uint32 nClasses;
    uint32 classOffset, stateOffset, entryOffset, ligActOff, compOff, ligOff;
    uint16 *classes;
    uint16 lig_comp_classes[MAX_LIG_COMP];
    uint16 lig_comp_glyphs[MAX_LIG_COMP];
    int lcp;
    uint8 *states_in_use;
    int smax;
    struct ttfinfo *info;
    int cnt;
};

static void mort_figure_ligatures(struct statemachine *sm, int lcp, int off, int32 lig_offset,
	struct ttfinfo *info) {
    uint32 lig;
    int i, j, lig_glyph;
    PST *pst;
    int len;

    if ( lcp<0 || off+3>sm->length )
return;

    lig = memlong(sm->data,sm->length, off);
    off += sizeof(int32);

    for ( i=0; i<sm->info->glyph_cnt; ++i ) if ( sm->classes[i]==sm->lig_comp_classes[lcp] ) {
	sm->lig_comp_glyphs[lcp] = i;
	lig_offset += memushort(sm->data,sm->length,2*( ((((int32) lig)<<2)>>2) + i ) );
	if ( lig&0xc0000000 ) {
	    if ( lig_offset+1 > sm->length ) {
		LogError( _("Invalid ligature offset\n") );
		info->bad_gx = true;
    break;
	    }
	    lig_glyph = memushort(sm->data,sm->length,lig_offset);
	    if ( lig_glyph>=sm->info->glyph_cnt ) {
		if ( info->justinuse != git_normal )
return;
		LogError( _("Attempt to make a ligature for glyph %d out of "),
			lig_glyph );
		for ( j=lcp; j<sm->lcp; ++j )
		    LogError("%d ",sm->lig_comp_glyphs[j]);
		LogError("\n");
		info->bad_gx = true;
	    } else if ( info->justinuse == git_justinuse ) {
		info->inuse[lig_glyph] = true;
	    } else if ( sm->info->chars[lig_glyph]==NULL ) {
		/* Do Nothing, ttc file probably */
	    } else {
		char *comp;
		int err=false;
		for ( len=0, j=lcp; j<sm->lcp; ++j )
		    if ( sm->lig_comp_glyphs[j]<sm->info->glyph_cnt &&
			    sm->info->chars[sm->lig_comp_glyphs[j]]!=NULL )
			len += strlen(sm->info->chars[sm->lig_comp_glyphs[j]]->name)+1;
		    else
			err = true;
		if ( sm->info->chars[lig_glyph]!=NULL && !err ) {
		    comp = malloc(len+1);
		    *comp = '\0';
		    for ( j=lcp; j<sm->lcp; ++j ) {
			if ( sm->lig_comp_glyphs[j]<sm->info->glyph_cnt &&
				sm->info->chars[sm->lig_comp_glyphs[j]]!=NULL ) {
			    if ( *comp!='\0' )
				strcat(comp," ");
			    strcat(comp,sm->info->chars[sm->lig_comp_glyphs[j]]->name);
			}
		    }
		    for ( pst=sm->info->chars[lig_glyph]->possub; pst!=NULL; pst=pst->next )
			if ( pst->type==pst_ligature && pst->subtable==sm->info->mort_subs_lookup->subtables &&
				strcmp(comp,pst->u.lig.components)==0 )
		    break;
		    /* There are cases where there will be multiple entries for */
		    /*  the same lig. ie. if we have "ff" and "ffl" then there */
		    /*  will be multiple entries for "ff" */
		    if ( pst == NULL ) {
			pst = chunkalloc(sizeof(PST));
			pst->type = pst_ligature;
			pst->subtable = sm->info->mort_subs_lookup->subtables;
			if ( sm->info->mort_subs_lookup->features!=NULL )
			    FListsAppendScriptLang(sm->info->mort_subs_lookup->features,
				    SCScriptFromUnicode(sm->info->chars[lig_glyph]),
				    DEFAULT_LANG);
			pst->u.lig.components = comp;
			pst->u.lig.lig = sm->info->chars[lig_glyph];
			pst->next = sm->info->chars[lig_glyph]->possub;
			sm->info->chars[lig_glyph]->possub = pst;
		    } else
			free(comp);
		}
	    }
	} else
	    mort_figure_ligatures(sm,lcp-1,off,lig_offset,info);
	lig_offset -= memushort(sm->data,sm->length,2*( ((((int32) lig)<<2)>>2) + i ) );
    }
}

static void follow_mort_state(struct statemachine *sm,int offset,int class,
	struct ttfinfo *info) {
    int state = (offset-sm->stateOffset)/sm->nClasses;
    int class_top, class_bottom;

    if ( state<0 || state>=sm->smax || sm->states_in_use[state] || sm->lcp>=MAX_LIG_COMP )
return;
    ++ sm->cnt;
    if ( sm->cnt>=10000 ) {
	if ( sm->cnt==10000 )
/* GT: This is a reference to "Much Ado About Nothing". The string should read */
/* GT: "A ligature sub-table in Apple's 'mort'/'morx' table is too\ncomplex for me to understand. I shall give up on it.\nYour ligatures may not be complete." */
	    LogError(_("In an attempt to process the ligatures of this font, I've concluded\nthat the state machine in Apple's mort/morx table is\n(like the learned constable) too cunning to be understood.\nI shall give up on it. Your ligatures may be incomplete.\n") );
	info->bad_gx = true;
return;
    }
    sm->states_in_use[state] = true;

    if ( class==-1 ) { class_bottom = 0; class_top = sm->nClasses; }
    else { class_bottom = class; class_top = class+1; }
    for ( class=class_bottom; class<class_top; ++class ) {
	int ent = sm->data[offset+class];
	int newState = memushort(sm->data,sm->length,sm->entryOffset+4*ent);
	int flags = memushort(sm->data,sm->length,sm->entryOffset+4*ent+2);
	/* If we have the same entry as state 0, then presumably we are */
	/*  ignoring the components read so far and starting over with a new */
	/*  lig (similarly for state 1) */
	if (( state!=0 && sm->data[sm->stateOffset+class] == ent ) ||
		(state>1 && sm->data[sm->stateOffset+sm->nClasses+class]==ent ))
    continue;
	if ( flags&0x8000 )	/* Set component */
	    sm->lig_comp_classes[sm->lcp++] = class;
	if ( flags&0x3fff ) {
	    mort_figure_ligatures(sm, sm->lcp-1, flags & 0x3fff, 0,info);
	} else if ( flags&0x8000 )
	    follow_mort_state(sm,newState,(flags&0x4000)?class:-1,info);
	if ( flags&0x8000 )
	    --sm->lcp;
    }
    sm->states_in_use[state] = false;
}

static void morx_figure_ligatures(struct statemachine *sm, int lcp, int ligindex, int32 lig_offset,
	struct ttfinfo *info) {
    uint32 lig;
    int i, j, lig_glyph;
    PST *pst;
    int len;

    if ( lcp<0 || sm->ligActOff+4*ligindex+3>sm->length )
return;

    lig = memlong(sm->data,sm->length, sm->ligActOff+4*ligindex);
    ++ligindex;

    for ( i=0; i<sm->info->glyph_cnt; ++i ) if ( sm->classes[i]==sm->lig_comp_classes[lcp] ) {
	sm->lig_comp_glyphs[lcp] = i;
	lig_offset += memushort(sm->data,sm->length,sm->compOff + 2*( ((((int32) lig)<<2)>>2) + i ) );
	if ( lig&0xc0000000 ) {
	    if ( sm->ligOff+2*lig_offset+1 > sm->length ) {
		LogError( _("Invalid ligature offset\n") );
		info->bad_gx = true;
    break;
	    }
	    lig_glyph = memushort(sm->data,sm->length,sm->ligOff+2*lig_offset);
	    if ( lig_glyph>=sm->info->glyph_cnt ) {
		if ( info->justinuse != git_normal )
return;
		LogError( _("Attempt to make a ligature for (non-existent) glyph %d out of "),
			lig_glyph );
		info->bad_gx = true;
		for ( j=lcp; j<sm->lcp; ++j )
		    LogError("%d ",sm->lig_comp_glyphs[j]);
		LogError("\n");
	    } else if ( info->justinuse == git_justinuse ) {
		info->inuse[lig_glyph] = true;
	    } else if ( sm->info->chars[lig_glyph]==NULL ) {
		/* Nothing to do */
	    } else {
		char *comp;
		int err=false;
		for ( len=0, j=lcp; j<sm->lcp; ++j ) {
		    if ( sm->lig_comp_glyphs[j]>=sm->info->glyph_cnt ||
			    sm->info->chars[ sm->lig_comp_glyphs[j]]==NULL )
			err = true;
		    else
			len += strlen(sm->info->chars[sm->lig_comp_glyphs[j]]->name)+1;
		}
		if ( !err ) {
		    comp = malloc(len);
		    *comp = '\0';
		    for ( j=lcp; j<sm->lcp; ++j ) {
			if ( *comp!='\0' )
			    strcat(comp," ");
			strcat(comp,sm->info->chars[sm->lig_comp_glyphs[j]]->name);
		    }
		    for ( pst=sm->info->chars[lig_glyph]->possub; pst!=NULL; pst=pst->next )
			if ( pst->type==pst_ligature && pst->subtable==sm->info->mort_subs_lookup->subtables &&
				strcmp(comp,pst->u.lig.components)==0 )
		    break;
		    /* There are cases where there will be multiple entries for */
		    /*  the same lig. ie. if we have "ff" and "ffl" then there */
		    /*  will be multiple entries for "ff" */
		    if ( pst == NULL ) {
			pst = chunkalloc(sizeof(PST));
			pst->type = pst_ligature;
			pst->subtable = sm->info->mort_subs_lookup->subtables;
			if ( sm->info->mort_subs_lookup->features!=NULL )
			    FListsAppendScriptLang(sm->info->mort_subs_lookup->features,
				    SCScriptFromUnicode(sm->info->chars[lig_glyph]),
				    DEFAULT_LANG);
			pst->u.lig.components = comp;
			pst->u.lig.lig = sm->info->chars[lig_glyph];
			pst->next = sm->info->chars[lig_glyph]->possub;
			sm->info->chars[lig_glyph]->possub = pst;
		    } else
			free(comp);
		}
	    }
	} else
	    morx_figure_ligatures(sm,lcp-1,ligindex,lig_offset,info);
	lig_offset -= memushort(sm->data,sm->length,sm->compOff + 2*( ((((int32) lig)<<2)>>2) + i ) );
    }
}

static void follow_morx_state(struct statemachine *sm,int state,int class,
	struct ttfinfo *info) {
    int class_top, class_bottom;

    if ( state<0 || state>=sm->smax || sm->states_in_use[state] || sm->lcp>=MAX_LIG_COMP )
return;
    ++ sm->cnt;
    if ( sm->cnt>=10000 ) {
	if ( sm->cnt==10000 )
	    LogError(_("In an attempt to process the ligatures of this font, I've concluded\nthat the state machine in Apple's mort/morx table is\n(like the learned constable) too cunning to be understood.\nI shall give up on it. Your ligatures may be incomplete.\n") );
	    info->bad_gx = true;
return;
    }
    sm->states_in_use[state] = true;

    if ( class==-1 ) { class_bottom = 0; class_top = sm->nClasses; }
    else { class_bottom = class; class_top = class+1; }
    for ( class=class_bottom; class<class_top; ++class ) {
	int ent = memushort(sm->data, sm->length,sm->stateOffset + 2*(state*sm->nClasses+class) );
	int newState = memushort(sm->data,sm->length,sm->entryOffset+6*ent);
	int flags = memushort(sm->data,sm->length,sm->entryOffset+6*ent+2);
	int ligindex = memushort(sm->data,sm->length,sm->entryOffset+6*ent+4);
	/* If we have the same entry as state 0, then presumably we are */
	/*  ignoring the components read so far and starting over with a new */
	/*  lig (similarly for state 1) */
	if (( state!=0 && memushort(sm->data, sm->length,sm->stateOffset + 2*class) == ent ) ||
		(state>1 && memushort(sm->data,sm->length, sm->stateOffset + 2*(sm->nClasses+class))==ent ))
    continue;
	if ( flags&0x8000 )	/* Set component */
	    sm->lig_comp_classes[sm->lcp++] = class;
	if ( flags&0x2000 ) {
	    morx_figure_ligatures(sm, sm->lcp-1, ligindex, 0,info);
	} else if ( flags&0x8000 )
	    follow_morx_state(sm,newState,(flags&0x4000)?class:-1,info);
	if ( flags&0x8000 )
	    --sm->lcp;
    }
    sm->states_in_use[state] = false;
}

static void readttf_mortx_lig(FILE *ttf,struct ttfinfo *info,int ismorx,uint32 base,uint32 length) {
    uint32 here;
    struct statemachine sm;
    int first, cnt, i;

    memset(&sm,0,sizeof(sm));
    sm.info = info;
    here = ftell(ttf);
    length -= here-base;
    sm.data = malloc(length);
    sm.length = length;
    if ( fread(sm.data,1,length,ttf)!=length ) {
	free(sm.data);
	LogError( _("Bad mort ligature table. Not long enough\n"));
	info->bad_gx = true;
return;
    }
    fseek(ttf,here,SEEK_SET);
    if ( ismorx ) {
	sm.nClasses = memlong(sm.data,sm.length, 0);
	sm.classOffset = memlong(sm.data,sm.length, sizeof(int32));
	sm.stateOffset = memlong(sm.data,sm.length, 2*sizeof(int32));
	sm.entryOffset = memlong(sm.data,sm.length, 3*sizeof(int32));
	sm.ligActOff = memlong(sm.data,sm.length, 4*sizeof(int32));
	sm.compOff = memlong(sm.data,sm.length, 5*sizeof(int32));
	sm.ligOff = memlong(sm.data,sm.length, 6*sizeof(int32));
	fseek(ttf,here+sm.classOffset,SEEK_SET);
	/* I used only to allocate space for info->glyph_cnt entries */
	/*  but some fonts use out of bounds gids as flags to contextual */
	/*  morx subtables, so allocate a full 65536 */
	sm.classes = info->morx_classes = malloc(65536*sizeof(uint16));
	for ( i=0; i<65536; ++i )
	    sm.classes[i] = 1;			/* Out of bounds */
	readttf_applelookup(ttf,info,
		mortclass_apply_values,mortclass_apply_value,NULL,NULL,true);
	sm.smax = length/(2*sm.nClasses);
	sm.states_in_use = calloc(sm.smax,sizeof(uint8));
	follow_morx_state(&sm,0,-1,info);
    } else {
	sm.nClasses = memushort(sm.data,sm.length, 0);
	sm.classOffset = memushort(sm.data,sm.length, sizeof(uint16));
	sm.stateOffset = memushort(sm.data,sm.length, 2*sizeof(uint16));
	sm.entryOffset = memushort(sm.data,sm.length, 3*sizeof(uint16));
	sm.ligActOff = memushort(sm.data,sm.length, 4*sizeof(uint16));
	sm.compOff = memushort(sm.data,sm.length, 5*sizeof(uint16));
	sm.ligOff = memushort(sm.data,sm.length, 6*sizeof(uint16));
	sm.classes = malloc(info->glyph_cnt*sizeof(uint16));
	for ( i=0; i<info->glyph_cnt; ++i )
	    sm.classes[i] = 1;			/* Out of bounds */
	first = memushort(sm.data,sm.length, sm.classOffset);
	cnt = memushort(sm.data,sm.length, sm.classOffset+sizeof(uint16));
	for ( i=0; i<cnt; ++i )
	    sm.classes[first+i] = sm.data[sm.classOffset+2*sizeof(uint16)+i];
	sm.smax = length/sm.nClasses;
	sm.states_in_use = calloc(sm.smax,sizeof(uint8));
	follow_mort_state(&sm,sm.stateOffset,-1,info);
    }
    free(sm.data);
    free(sm.states_in_use);
    free(sm.classes);
}

struct statetable {
    uint32 state_start;
    int nclasses;
    int nstates;
    int nentries;
    int state_offset;
    int entry_size;	/* size of individual entry */
    int entry_extras;	/* Number of extra glyph offsets */
    int first_glyph;	/* that's classifyable */
    int nglyphs;
    uint8 *classes;
    uint8 *state_table;	/* state_table[nstates][nclasses], each entry is an */
	/* index into the following array */
    uint16 *state_table2;	/* morx version. States are have 2 byte entries */
    uint16 *classes2;
    uint8 *transitions;
    uint32 extra_offsets[3];
};

static struct statetable *read_statetable(FILE *ttf, int ent_extras, int ismorx, struct ttfinfo *info) {
    struct statetable *st = calloc(1,sizeof(struct statetable));
    uint32 here = ftell(ttf);
    int nclasses, class_off, state_off, entry_off;
    int state_max, ent_max, old_state_max, old_ent_max;
    int i, j, ent, new_state, ent_size;
    int error;

    st->state_start = here;

    if ( ismorx ) {
	nclasses = getlong(ttf);
	class_off = getlong(ttf);
	state_off = getlong(ttf);
	entry_off = getlong(ttf);
	st->extra_offsets[0] = getlong(ttf);
	st->extra_offsets[1] = getlong(ttf);
	st->extra_offsets[2] = getlong(ttf);
    } else {
	nclasses = getushort(ttf);	/* Number of bytes per state in state subtable, equal to number of classes */
	class_off = getushort(ttf);
	state_off = getushort(ttf);
	entry_off = getushort(ttf);
	st->extra_offsets[0] = getushort(ttf);
	st->extra_offsets[1] = getushort(ttf);
	st->extra_offsets[2] = getushort(ttf);
    }
    st->nclasses = nclasses;
    st->state_offset = state_off;

	/* parse class subtable */
    fseek(ttf,here+class_off,SEEK_SET);
    error = 0;
    if ( ismorx ) {
	/* I used only to allocate space for info->glyph_cnt entries */
	/*  but some fonts use out of bounds gids as flags to contextual */
	/*  morx subtables, so allocate a full 65536 */
	st->classes2 = info->morx_classes = malloc(65536*sizeof(uint16));
	for ( i=0; i<65536; ++i )
	    st->classes2[i] = 1;			/* Out of bounds */
	readttf_applelookup(ttf,info,
		mortclass_apply_values,mortclass_apply_value,NULL,NULL,true);
	for ( i=0; i<65536; ++i ) {
	    if ( /*st->classes2[i]<0 ||*/ st->classes2[i]>=st->nclasses ) {
		if ( !error )
		    LogError( _("Bad class in state machine.\n" ));
		info->bad_gx = true;
		error = true;
		st->classes2[i] = 1;			/* Out of bounds */
	    }
	}
    } else {
	st->first_glyph = getushort(ttf);
	st->nglyphs = getushort(ttf);
	if ( feof(ttf)) {
	    LogError(_("Bad glyph count in mort table.\n"));
	    info->bad_gx = true;
	    st->nglyphs = 0;
	}
	st->classes = malloc(st->nglyphs);
	fread(st->classes,1,st->nglyphs,ttf);
	for ( i=0; i<st->nglyphs; ++i ) {
	    if ( /*st->classes[i]<0 ||*/ st->classes[i]>=st->nclasses ) {
		if ( !error )
		    LogError( _("Bad class in state machine.\n" ));
		info->bad_gx = true;
		error = true;
		st->classes[i] = 1;			/* Out of bounds */
	    }
	}
    }


    /* The size of an entry is variable. There are 2 uint16 fields at the begin-*/
    /*  ning of all entries. There may be some number of shorts following these*/
    /*  used for indexing special tables. */
    ent_size = 4 + 2*ent_extras;
    st->entry_size = ent_size;
    st->entry_extras = ent_extras;

    /* Apple does not provide a way of figuring out the size of either of the */
    /*  state or entry tables, so we must parse both as we go and try to work */
    /*  out the maximum values... */
    /* There are always at least 2 states defined. Parse them and find what */
    /*  is the biggest entry they use, then parse those entries and find what */
    /*  is the biggest state they use, and then repeat until we don't find any*/
    /*  more states or entries */
    old_state_max = 0; old_ent_max = 0;
    state_max = 2; ent_max = 0;
    while ( old_state_max!=state_max ) {
	i = old_state_max*nclasses;
	fseek(ttf,here+state_off+(ismorx?i*sizeof(uint16):i),SEEK_SET);
	old_state_max = state_max;
	for ( ; i<state_max*nclasses; ++i ) {
	    ent = ismorx ? getushort(ttf) : getc(ttf);
	    if ( ent+1 > ent_max )
		ent_max = ent+1;
	}
	if ( ent_max==old_ent_max )		/* Nothing more */
    break;
	if ( ent_max>1000 ) {
	    LogError( _("It looks to me as though there's a morx sub-table with more than 1000\n transitions. Which makes me think there's probably an error\n" ));
	    info->bad_gx = true;
	    free(st);
return( NULL );
	}
	fseek(ttf,here+entry_off+old_ent_max*ent_size,SEEK_SET);
	i = old_ent_max;
	old_ent_max = ent_max;
	for ( ; i<ent_max; ++i ) {
	    new_state = getushort(ttf);
	    if ( !ismorx )
		new_state = (new_state-state_off)/nclasses;
	    /* flags = */ getushort(ttf);
	    for ( j=0; j<ent_extras; ++j )
		/* glyphOffsets[j] = */ getushort(ttf);
	    if ( new_state+1>state_max )
		state_max = new_state+1;
	}
	if ( state_max>1000 ) {
	    LogError( _("It looks to me as though there's a morx sub-table with more than 1000\n states. Which makes me think there's probably an error\n" ));
	    info->bad_gx = true;
	    free(st);
return( NULL );
	}
    }

    st->nstates = state_max;
    st->nentries = ent_max;
    
    fseek(ttf,here+state_off,SEEK_SET);
    /* an array of arrays of state transitions, each represented by one byte */
    /*  which is an index into the Entry subtable, which comes next. */
    /* One dimension is the number of states, and the other the */
    /*  number of classes (classes vary faster than states) */
    /* The first two states are predefined, 0 is start of text, 1 start of line*/
    if ( ismorx ) {
	st->state_table2 = malloc(st->nstates*st->nclasses*sizeof(uint16));
	for ( i=0; i<st->nstates*st->nclasses; ++i )
	    st->state_table2[i] = getushort(ttf);
    } else {
	st->state_table = malloc(st->nstates*st->nclasses);
	fread(st->state_table,1,st->nstates*st->nclasses,ttf);
    }

	/* parse the entry subtable */
    fseek(ttf,here+entry_off,SEEK_SET);
    st->transitions = malloc(st->nentries*st->entry_size);
    fread(st->transitions,1,st->nentries*st->entry_size,ttf);
return( st );
}

static void statetablefree(struct statetable *st) {
    free( st->classes );
    free( st->state_table );
    free( st->classes2 );
    free( st->state_table2 );
    free( st->transitions );
    free( st );
}

static void tagSMWithScriptLang(FeatureScriptLangList *fl,
	struct statetable *st,int ismorx,struct ttfinfo *info) {
    int i;

    if ( ismorx ) {
	for ( i=0; i<info->glyph_cnt; ++i )
	    if ( st->classes2[i]>=4 && info->chars[i]!=NULL )
		FListsAppendScriptLang(fl,SCScriptFromUnicode(info->chars[i]),
			DEFAULT_LANG);
    } else {
	for ( i=st->first_glyph; i<st->first_glyph+st->nglyphs && i<info->glyph_cnt; ++i )
	    if ( info->chars[i]!=NULL )
		FListsAppendScriptLang(fl,SCScriptFromUnicode(info->chars[i]),
			DEFAULT_LANG);
    }
}

static char **ClassesFromStateTable(struct statetable *st,int ismorx,struct ttfinfo *info) {
    /* On the mac the first four classes should be left blank. only class 1 */
    /*  (out of bounds) is supposed to be used in the class array anyway */
    char **classes = malloc(st->nclasses*sizeof(char *));
    int *lens = calloc(st->nclasses,sizeof(int));
    int i;


    if ( ismorx ) {
	for ( i=0; i<info->glyph_cnt; ++i ) if ( info->chars[i]!=NULL )
	    lens[st->classes2[i]] += strlen( info->chars[i]->name )+1;
	if ( info->badgids!=NULL )
	    for ( i=0; i<info->badgid_cnt; ++i ) if ( info->badgids[i]!=NULL )
		lens[st->classes2[info->badgids[i]->orig_pos]] += strlen( info->badgids[i]->name )+1;
    } else {
	for ( i=st->first_glyph; i<st->first_glyph+st->nglyphs && i<info->glyph_cnt; ++i )
	    if ( info->chars[i]!=NULL )
		lens[st->classes[i-st->first_glyph]] += strlen( info->chars[i]->name )+1;
    }
    classes[0] = classes[1] = classes[2] = classes[3] = NULL;
    for ( i=4; i<st->nclasses; ++i ) {
	classes[i] = malloc(lens[i]+1);
	*classes[i] = '\0';
    }
    if ( ismorx ) {
	for ( i=0; i<info->glyph_cnt; ++i ) if ( st->classes2[i]>=4 && info->chars[i]!=NULL ) {
	    strcat(classes[st->classes2[i]],info->chars[i]->name );
	    strcat(classes[st->classes2[i]]," ");
	}
	if ( info->badgids!=NULL )
	    for ( i=0; i<info->badgid_cnt; ++i ) if ( info->badgids[i]!=NULL && st->classes2[info->badgids[i]->orig_pos]>=4) {
		strcat(classes[st->classes2[info->badgids[i]->orig_pos]],info->badgids[i]->name );
		strcat(classes[st->classes2[info->badgids[i]->orig_pos]]," ");
	    }
    } else {
	for ( i=st->first_glyph; i<st->first_glyph+st->nglyphs && i<info->glyph_cnt; ++i ) if ( st->classes[i-st->first_glyph]>=4 && info->chars[i]!=NULL ) {
	    strcat(classes[st->classes[i-st->first_glyph]],info->chars[i]->name );
	    strcat(classes[st->classes[i-st->first_glyph]]," " );
	}
    }
    for ( i=4; i<st->nclasses; ++i ) {
	int len = strlen(classes[i]);
	if ( len!=0 )
	    classes[i][len-1] = '\0';	/* Remove trailing space */
    }
    free(lens);
return( classes );
}

static char *NamesOfList(uint32 pos,int cnt, FILE *ttf, struct ttfinfo *info) {
    int i, len, glyph;
    char *str;

    if ( cnt==0 )
return(NULL);

    fseek(ttf,pos,SEEK_SET);
    for ( i=len=0; i<cnt; ++i ) {
	glyph = getushort(ttf);
	if ( glyph<info->glyph_cnt )
	    len += strlen(info->chars[glyph]->name)+1;
    }
    if ( len==0 )
return( NULL );
    str = malloc(len+1);
    fseek(ttf,pos,SEEK_SET);
    for ( i=len=0; i<cnt; ++i ) {
	glyph = getushort(ttf);
	if ( glyph<info->glyph_cnt ) {
	    strcpy(str+len,info->chars[glyph]->name);
	    len += strlen(info->chars[glyph]->name);
	    str[len++] = ' ';
	}
    }
    str[len-1] = '\0';
return( str );
}

static void KernReadKernList(FILE *ttf,uint32 pos, struct asm_state *trans) {
/* Apple does not document how to detect the end of the list */
/* They say "an odd value that depends on coverage" */
/* They should say "an odd value". Any odd value terminates the list. */
/*  coverage is irrelevant */
/* Note: List is backwards (glyphs are popped of LIFO so last glyph on */
/*  in stack gets first kern value) */
/*  There are at most 8 glyphs */
    int i,j,k;
    int16 buffer[8];		/* At most 8 kerns are supported */

    fseek(ttf,pos,SEEK_SET);
    for ( i=0; i<8; ++i ) {
	buffer[i]=(int16) getushort(ttf);
	if ( buffer[i]&1 ) {
	    buffer[i] &= ~1;
	    ++i;
    break;
	}
    }
    if ( i==0 ) {
	trans->u.kern.kerns = NULL;
    } else {
	trans->u.kern.kerns = malloc(i*sizeof(int16));
	for ( j=i-1, k=0; k<i; ++k, --j )
	    trans->u.kern.kerns[k] = buffer[j];
    }
    trans->u.kern.kcnt = i;
}

static void read_perglyph_subs(FILE *ttf,struct ttfinfo *info,
	int subs_base,int subs_end,struct statetable *st,
	uint8 *classes_subbed, int evermarked, uint8 *used) {
    /* The file is positioned at the start of a per-glyph substitution table */
    /* Sadly great chunks of this table have been omitted. We are where glyph */
    /*  0 would be if it were present. We've no idea what has been omitted */
    /* Simple checks: if the file pointer is outside of the area devoted to */
    /*   substitutions then we know it is ignorable. */
    /*  If the current glyph is not in the list of glyphs which could ever */
    /*   be substituted then we know it is ignorable. */
    /*  Note: the above list is easily figured for substitutions on the current*/
    /*   glyph, but if a substitution ever happens to a marked glyph then we */
    /*   can't guess. We could check for all classes that get marked, but that*/
    /*   doesn't work if there is a current substitution before the class is */
    /*   marked, after that we don't know what class the glyph might have */
    /*   Instead, for marked subs, we keep track of all locations which were */
    /*   used in a current-only sub, and assume that they aren't valid for us */
    /*  If the putative substitution glyph is not a valid glyph then we know */
    /*   it is ignorable */
    int i, subs, was = info->mort_tag_mac;
    uint32 here;

    info->mort_tag_mac = false;
    for ( i=0; i<info->glyph_cnt; ++i ) {
	here = ftell(ttf);
	subs = getushort(ttf);
	if ( subs>=info->glyph_cnt && subs!=0xffff )	/* 0xffff means delete the substituted glyph */
    continue;
	if ( subs==0 )			/* A little risky, one could substitute notdef */
    continue;				/*  but they shouldn't */
	if ( here<subs_base )
    continue;
	if ( here>=subs_end )
    break;
	if ( evermarked ) {
	    if ( used[(here-subs_base)/2] )
    continue;
	} else if ( i<st->first_glyph || i>=st->first_glyph+st->nglyphs ) {
	    if ( !classes_subbed[1]) {	/* Out of bounds class */
		if ( i>=st->first_glyph+st->nglyphs )
    break;
    continue;
	    }
	} else {
	    if ( !classes_subbed[st->classes[i-st->first_glyph]] )
    continue;
	}

	if ( !evermarked )
	    used[(here-subs_base)/2] = true;
	TTF_SetMortSubs(info, i, subs);
    }
    info->mort_tag_mac = was;
}

static int sm_lookupfind(int32 *lookups,int *_lm,int off) {
    int lm = *_lm, i;
    for ( i=0; i<=lm; ++i )
	if ( lookups[i]==off )
return( i );
    (*_lm)++;
    lookups[i] = off;
return( i );
}

static ASM *readttf_mortx_asm(FILE *ttf,struct ttfinfo *info,int ismorx,
	uint32 subtab_len,enum asm_type type,int extras,
	uint32 coverage, OTLookup *otl) {
    struct statetable *st;
    ASM *as;
    int i,j;
    uint32 here = ftell(ttf);

    st = read_statetable(ttf,extras,ismorx,info);
    if ( st==NULL )
return(NULL);

    as = chunkalloc(sizeof(ASM));
    as->type = type;
    as->flags = coverage>>16;
    as->class_cnt = st->nclasses;
    as->state_cnt = st->nstates;
    as->classes = ClassesFromStateTable(st,ismorx,info);
    as->state = malloc(st->nclasses*st->nstates*sizeof(struct asm_state));
    if ( otl==NULL )
	otl = NewMacLookup(info,false);
    otl->lookup_type = type==asm_indic ? morx_indic : type==asm_context ? morx_context : morx_insert;
    as->subtable = otl->subtables; otl->subtables->sm = as;
    tagSMWithScriptLang(otl->features,st,ismorx,info);
    InfoNameOTLookup(otl,info);
    for ( i=0; i<st->nclasses*st->nstates; ++i ) {
	int trans;
	if ( ismorx ) {
	    trans = st->state_table2[i];
	    as->state[i].next_state = memushort(st->transitions,st->nentries*st->entry_size,trans*st->entry_size);
	} else {
	    trans = st->state_table[i];
	    as->state[i].next_state = (memushort(st->transitions,st->nentries*st->entry_size,trans*st->entry_size)-st->state_offset)/st->nclasses;
	}
	as->state[i].flags = memushort(st->transitions,st->nentries*st->entry_size,trans*st->entry_size+2);
	if ( extras>0 )
	    as->state[i].u.context.mark_lookup = (void *) (intpt) memushort(st->transitions,st->nentries*st->entry_size, trans*st->entry_size+2+2);
	if ( extras>1 )
	    as->state[i].u.context.cur_lookup = (void *) (intpt) memushort(st->transitions,st->nentries*st->entry_size, trans*st->entry_size+2+2+2);
    }
    /* Indic tables have no attached subtables, just a verb in the flag field */
    /*  so for them we are done. For the others... */
    if ( !ismorx && type==asm_insert ) {
	for ( i=0; i<st->nclasses*st->nstates; ++i ) {
	    char *cur=NULL, *mark=NULL;
	    if ( (as->state[i].flags&0x3e0)!=0 && as->state[i].u.context.mark_lookup!=NULL ) {
		cur = NamesOfList(here+(intpt) as->state[i].u.context.mark_lookup,
			(as->state[i].flags&0x3e0)>>5,ttf,info);
	    }
	    if ( (as->state[i].flags&0x01f)!=0 && as->state[i].u.context.cur_lookup!=NULL ) {
		mark = NamesOfList(here+(intpt) as->state[i].u.context.cur_lookup,
			as->state[i].flags&0x01f,ttf,info);
	    }
	    as->state[i].u.insert.cur_ins=cur;
	    as->state[i].u.insert.mark_ins=mark;
	}
    } else if ( ismorx && type == asm_insert ) {
	for ( i=0; i<st->nclasses*st->nstates; ++i ) {
	    char *cur=NULL, *mark=NULL;
	    if ( (as->state[i].flags&0x3e0)!=0 && (intpt) as->state[i].u.context.mark_lookup!=0xffff ) {
		cur = NamesOfList(here+st->extra_offsets[0]+((intpt) as->state[i].u.context.mark_lookup)*2,
			(as->state[i].flags&0x3e0)>>5,ttf,info);
	    }
	    if ( (as->state[i].flags&0x01f)!=0 && ((intpt) as->state[i].u.context.cur_lookup)!=0xffff ) {
		mark = NamesOfList(here+st->extra_offsets[0]+((intpt) as->state[i].u.context.cur_lookup)*2,
			as->state[i].flags&0x01f,ttf,info);
	    }
	    as->state[i].u.insert.cur_ins=cur;
	    as->state[i].u.insert.mark_ins=mark;
	}
    } else if ( !ismorx && type == asm_context ) {
	/* I don't see any good way to parse a per-glyph substitution table */
	/*  the problem being that most of the per-glyph table is missing */
	/*  but I don't know which bits. The offsets I'm given point to */
	/*  where glyph 0 would be if it were present in the table, but */
	/*  mostly only the bit used is present... */
	/* I could walk though the state machine and find all classes that */
	/*  go to a specific substitution (which would tell me what glyphs */
	/*  were active). That's not hard for substitutions of the current */
	/*  glyph, but it is intractibable for marked glyphs. And I can't  */
	/*  do one without the other. So I do neither. */
	/* One thing I could test fairly easily would be to see whether    */
	/*  class 1 (out of bounds) is ever available for a substitution   */
	/*  (if it ever has a mark set on it or has a current substitution)*/
	/*  if not, then I can ignore any putative substitutions for class */
	/*  1 glyphs (actually I should do this for all classes)*/
	/* Damn. That doesn't work for marks. Because a current substitution*/
	/*  may be applied, and then the glyph gets marked. So we've no idea*/
	/*  what class the newly marked glyph might live in		   */
	/* Apple's docs say the substitutions are offset from the "state   */
	/*  subtable", but it seems much more likely that they are offset  */
	/*  from the substitution table (given that Apple's docs are often */
	/*  wrong */ /* Apple's docs are right. not clear why that offset  */
	/*  is there */
	uint8 *classes_subbed = calloc(st->nclasses,1);
	int lookup_max = -1, index, index_max;
	int32 *lookups = malloc(st->nclasses*st->nstates*sizeof(int32));
	uint8 *evermarked = calloc(st->nclasses*st->nstates,sizeof(uint8));
	uint8 *used;
	OTLookup **subs;

	index_max = 0;
	for ( i=0; i<st->nclasses*st->nstates; ++i ) {
	    if ( as->state[i].u.context.mark_lookup!=NULL ) {
		index = sm_lookupfind(lookups,&lookup_max,(int16) (intpt) as->state[i].u.context.mark_lookup);
		if ( index>index_max ) index_max = index;
	    }
	    if ( as->state[i].u.context.cur_lookup!=NULL ) {
		index = sm_lookupfind(lookups,&lookup_max,(int16) (intpt) as->state[i].u.context.cur_lookup);
		if ( index>index_max ) index_max = index;
	    }
	}
	subs = calloc(index_max+1,sizeof(OTLookup *));

	for ( i=0; i<st->nclasses*st->nstates; ++i ) {
	    if ( as->state[i].u.context.mark_lookup!=NULL ) {
		index = sm_lookupfind(lookups,&lookup_max,(int16) (intpt) as->state[i].u.context.mark_lookup);
		evermarked[index] = true;
		as->state[i].u.context.mark_lookup = NewMacSubsLookup(info,otl,index,subs);
	    }
	    if ( as->state[i].u.context.cur_lookup!=NULL ) {
		index = sm_lookupfind(lookups,&lookup_max,(int16) (intpt) as->state[i].u.context.cur_lookup);
		as->state[i].u.context.cur_lookup = NewMacSubsLookup(info,otl,index,subs);
	    }
	}
	used = calloc((subtab_len-st->extra_offsets[0]+1)/2,sizeof(uint8));
	/* first figure things that only appear in current subs */
	/*  then go back and work on things that apply to things which are also in marked subs */
	for ( j=0; j<2; ++j ) for ( i=0; i<=lookup_max; ++i ) if ( evermarked[i]==j ) {
	    info->mort_subs_lookup = NewMacSubsLookup(info,otl,i,subs);
	    info->mort_is_nested = true;
	    if ( !evermarked[i] ) { int k,l;
		memset(classes_subbed,0,st->nclasses);
		for ( k=0; k<st->nstates; ++k ) for ( l=0; l<st->nclasses; ++l ) {
		    if ( as->state[k*st->nclasses+l].u.context.cur_lookup == info->mort_subs_lookup )
			classes_subbed[l] = true;
		}
	    }
	    fseek(ttf,here/*+st->extra_offsets[0]*/+lookups[i]*2,SEEK_SET);
	    read_perglyph_subs(ttf,info,here+st->extra_offsets[0],here+subtab_len,
		    st,classes_subbed,evermarked[i],used);
	}
	info->mort_is_nested = false;
	free(classes_subbed);
	free(lookups);
	free(used);
	free(evermarked);
	free(subs);
    } else if ( ismorx && type == asm_context ) {
	int lookup_max= -1;
	uint32 *lookups;
	OTLookup **subs;

	for ( i=0; i<st->nclasses*st->nstates; ++i ) {
	    if ( (intpt) as->state[i].u.context.mark_lookup!=0xffff ) {
		if ( ((int) (intpt) as->state[i].u.context.mark_lookup)>lookup_max )
		    lookup_max = (intpt) as->state[i].u.context.mark_lookup;
	    }
	    if ( (intpt) as->state[i].u.context.cur_lookup!=0xffff ) {
		if ( ((int) (intpt) as->state[i].u.context.cur_lookup)>lookup_max )
		    lookup_max = (intpt) as->state[i].u.context.cur_lookup;
	    }
	}
	++lookup_max;
	subs = calloc(lookup_max,sizeof(OTLookup *));
	for ( i=0; i<st->nclasses*st->nstates; ++i ) {
	    if ( (intpt) as->state[i].u.context.mark_lookup!=0xffff ) {
		as->state[i].u.context.mark_lookup = NewMacSubsLookup(info,otl,(intpt) as->state[i].u.context.mark_lookup,subs);
	    } else
		as->state[i].u.context.mark_lookup = NULL;
	    if ( (intpt) as->state[i].u.context.cur_lookup!=0xffff ) {
		as->state[i].u.context.cur_lookup = NewMacSubsLookup(info,otl,(intpt) as->state[i].u.context.cur_lookup,subs);
	    } else
		as->state[i].u.context.cur_lookup = NULL;
	}
	lookups = malloc(lookup_max*sizeof(uint32));
	fseek(ttf,here+st->extra_offsets[0],SEEK_SET);
	for ( i=0; i<lookup_max; ++i )
	    lookups[i] = getlong(ttf) + here+st->extra_offsets[0];
	for ( i=0; i<lookup_max; ++i ) {
	    fseek(ttf,lookups[i],SEEK_SET);
	    info->mort_subs_lookup = NewMacSubsLookup(info,otl,i,subs);
	    info->mort_is_nested = true;
	    readttf_applelookup(ttf,info,
		    mort_apply_values,mort_apply_value,NULL,NULL,true);
	}
	info->mort_is_nested = false;
	free(subs);
	free(lookups);
    } else if ( type == asm_kern ) {
	for ( i=0; i<st->nclasses*st->nstates; ++i ) {
	    if ( (as->state[i].flags&0x3fff)!=0 ) {
		KernReadKernList(ttf,here+(as->state[i].flags&0x3fff),
			&as->state[i]);
		as->state[i].flags &= ~0x3fff;
	    } else {
		as->state[i].u.kern.kcnt = 0;
		as->state[i].u.kern.kerns = NULL;
	    }
	}
    }
    as->next = info->sm;
    info->sm = as;
    statetablefree(st);
return( as );
}

static int InfoHasGSUBTag(struct ttfinfo *info, uint32 tag, int apple_lookup_type) {
    OTLookup *otl;
    FeatureScriptLangList *feat;

    if ( apple_lookup_type==0 ||		/* Indic rearrangement */
	    apple_lookup_type==1 ||		/* Contextual substitution */
	    apple_lookup_type==5 )		/* Contextual insertion */
return( false ); /* These types can either not be represented in OT */
	/* or not converted to AAT -- so we'd better read them */
    /* We can't really do contextual ligatures either, but we parse that table*/
    /*  for the non-contextual ligs (which is most of them) */

    for ( otl = info->gsub_lookups; otl!=NULL; otl=otl->next ) {
	for ( feat = otl->features; feat!=NULL; feat=feat->next ) {
	    if ( feat->featuretag == tag &&
		    Macable(NULL,otl))
return( true );
	}
    }
return( false );
}

static void FeatMarkAsEnabled(struct ttfinfo *info,int featureType,
	int featureSetting);

static uint32 readmortchain(FILE *ttf,struct ttfinfo *info, uint32 base,
	int ismorx) {
    uint32 chain_len, nfeatures, nsubtables, default_flags;
    uint32 enable_flags, disable_flags, flags;
    int featureType, featureSetting;
    int i,j,k;
    uint32 length, coverage;
    uint32 here;
    uint32 tag;
    struct tagmaskfeature { uint32 tag, enable_flags; uint16 ismac, feat, set; } tmf[32];
    int r2l;

    default_flags = getlong(ttf);
    chain_len = getlong(ttf);
    if ( ismorx ) {
	nfeatures = getlong(ttf);
	nsubtables = getlong(ttf);
    } else {
	nfeatures = getushort(ttf);
	nsubtables = getushort(ttf);
    }

    k = 0;
    for ( i=0; i<nfeatures; ++i ) {
	featureType = getushort(ttf);
	featureSetting = getushort(ttf);
	enable_flags = getlong(ttf);
	disable_flags = getlong(ttf);
	if ( feof(ttf))
return( chain_len );
	if ( info->justinuse == git_normal && ( enable_flags & default_flags ))
	    FeatMarkAsEnabled(info,featureType,featureSetting);
	tag = MacFeatureToOTTag(featureType,featureSetting);
	if ( enable_flags!=0 && k<32 ) {
	    if ( tag==0 ) {
		tmf[k].tag = (featureType<<16) | featureSetting;
		tmf[k].ismac = true;
	    } else {
		tmf[k].tag = tag;
		tmf[k].ismac = false;
	    }
	    tmf[k].feat = featureType;
	    tmf[k].set = featureSetting;
	    tmf[k++].enable_flags = enable_flags;
	}
    }
    if ( k==0 )
return( chain_len );

    for ( i=0; i<nsubtables; ++i ) {
	here = ftell(ttf);
	if ( ismorx ) {
	    length = getlong(ttf);
	    coverage = getlong(ttf);
	} else {
	    length = getushort(ttf);
	    coverage = getushort(ttf);
	    coverage = ((coverage&0xe000)<<16) | (coverage&7);	/* convert to morx format */
	}
	r2l = (coverage & 0x40000000)? 1 : 0;
	flags = getlong(ttf);
	for ( j=k-1; j>=0 && (!(flags&tmf[j].enable_flags) || (tmf[j].enable_flags&~flags)!=0); --j );
	if ( j==-1 )
	    for ( j=k-1; j>=0 && (!(flags&tmf[j].enable_flags) || tmf[j].feat==0); --j );
	if ( j>=0 ) {
	    if ( !tmf[j].ismac &&
		    ((coverage&0xff)==0 ||
		     (coverage&0xff)==1 ||
		     (coverage&0xff)==5 )) {
		/* Only do the opentype tag conversion if we've got a format */
		/*  we can convert to opentype. Otherwise it is useless and */
		/*  confusing */
		tmf[j].ismac = true;
		tmf[j].tag = (tmf[j].feat<<16) | tmf[j].set;
	    }
	    info->mort_r2l = r2l;
	    info->mort_tag_mac = tmf[j].ismac;
	    info->mort_feat = tmf[j].feat; info->mort_setting = tmf[j].set;
	    /* If we've already read gsub. And this feature setting matches */
	    /*  and opentype feature tag. And we've got an OpenType lookup  */
	    /*  attached to that feature tag. And it's a lookup type we can */
	    /*  convert... Then don't parse this sub-table, we shall assume */
	    /*  it is simply a duplicate of the OT version */
	    if ( info->gsub_start!=0 &&
		    (tag = MacFeatureToOTTag(tmf[j].feat,tmf[j].set))!=0 &&
		    InfoHasGSUBTag(info,tag,coverage&0xff))
		/* Skip it */;
	    else switch( coverage&0xff ) {
	      case 0:	/* Indic rearangement */
		if ( info->justinuse == git_normal )
		    readttf_mortx_asm(ttf,info,ismorx,length,asm_indic,0,
			    coverage,NULL);
	      break;
	      case 1:	/* contextual glyph substitution */
		if ( info->justinuse == git_normal )
		    readttf_mortx_asm(ttf,info,ismorx,length,asm_context,2,
			    coverage,NULL);
	      break;
	      case 2:	/* ligature substitution */
		/* Apple's ligature state machines are too weird to be */
		/*  represented easily, but I can parse them into a set */
		/*  of ligatures -- assuming they are unconditional */
		if ( info->justinuse == git_normal ) {
		    info->mort_subs_lookup = NewMacLookup(info,false);
		    info->mort_subs_lookup->lookup_type = gsub_ligature;
		    if ( !tmf[j].ismac ) {
			info->mort_subs_lookup->features->next = chunkalloc(sizeof(FeatureScriptLangList));
			info->mort_subs_lookup->features->next->featuretag = tmf[j].tag;
			info->mort_subs_lookup->features->next->ismac = false;
		    }
		    info->mort_subs_lookup->subtables->per_glyph_pst_or_kern = true;
		}
		readttf_mortx_lig(ttf,info,ismorx,here,length);
		if ( info->justinuse == git_normal )
		    InfoNameOTLookup(info->mort_subs_lookup,info);
		/* We can give the lookup a better name after we've made a */
		/*  guess at what scripts it involves => substitutions first */
	      break;
	      case 4:	/* non-contextual glyph substitutions */
		if ( info->justinuse == git_normal ) {
		    info->mort_subs_lookup = NewMacLookup(info,false);
		    info->mort_subs_lookup->lookup_type = gsub_single;
		    if ( !tmf[j].ismac ) {
			info->mort_subs_lookup->features->next = chunkalloc(sizeof(FeatureScriptLangList));
			info->mort_subs_lookup->features->next->featuretag = tmf[j].tag;
			info->mort_subs_lookup->features->next->ismac = false;
		    }
		    info->mort_subs_lookup->subtables->per_glyph_pst_or_kern = true;
		}
		readttf_applelookup(ttf,info,
			mort_apply_values,mort_apply_value,NULL,NULL,true);
		if ( info->justinuse == git_normal )
		    InfoNameOTLookup(info->mort_subs_lookup,info);
	      break;
	      case 5:	/* contextual glyph insertion */
		if ( info->justinuse == git_normal )
		    readttf_mortx_asm(ttf,info,ismorx,length,asm_insert,2,
			    coverage,NULL);
	      break;
	    }
	}
	fseek(ttf, here+length, SEEK_SET );
    }

return( chain_len );
}

static void _readttfmort(FILE *ttf,struct ttfinfo *info) {
    uint32 base = info->morx_start!=0 ? info->morx_start : info->mort_start;
    uint32 here, len;
    int ismorx;
    int32 version;
    int i, nchains;

    fseek(ttf,base,SEEK_SET);
    version = getlong(ttf);
    ismorx = version == 0x00020000;
    if ( version!=0x00010000 && version != 0x00020000 )
return;
    nchains = getlong(ttf);
    if ( feof(ttf)) {
	LogError( _("Unexpected end of file found in morx chain.\n" ));
	info->bad_gx = true;
return;
    }
    info->mort_max = nchains*33;		/* Maximum of one feature per bit ? */
    for ( i=0; i<nchains; ++i ) {
	here = ftell(ttf);
	len = readmortchain(ttf,info,base,ismorx);
	if ( feof(ttf)) {
	    LogError( _("Unexpected end of file found in morx chain.\n"));
	    info->bad_gx = true;
    break;
	}
	fseek(ttf,here+len,SEEK_SET);
    }
    /* Some Apple fonts use out of range gids as flags in conditional substitutions */
    /*  generally to pass information from one sub-table to another which then */
    /*  removes the flag */
    if ( info->badgid_cnt!=0 ) {
	/* Merge the fake glyphs in with the real ones */
	int oldgc = info->glyph_cnt;
	info->chars = realloc(info->chars,(info->glyph_cnt+info->badgid_cnt)*sizeof(SplineChar *));
	for ( i=0; i<info->badgid_cnt; ++i ) {
	    info->chars[info->glyph_cnt+i] = info->badgids[i];
	    info->badgids[i]->orig_pos = info->glyph_cnt+i;
	}
	info->glyph_cnt += info->badgid_cnt;
	free(info->badgids);
	/* We also need to adjust the variations. */
	if (info->variations) {
	    int ctup;
	    for (ctup = 0; ctup < info->variations->tuple_count; ctup++) {
		SplineChar ** tscs = info->variations->tuples[ctup].chars;
		info->variations->tuples[ctup].chars = calloc(info->glyph_cnt, sizeof(SplineChar *));
		memcpy(info->variations->tuples[ctup].chars, tscs, oldgc * sizeof(SplineChar *));
		free(tscs);
		tscs = NULL;
	    }
	}
    }
}

void readttfmort(FILE *ttf,struct ttfinfo *info) {
    info->justinuse = git_normal;
    _readttfmort(ttf,info);
}

void readttfmort_glyphsused(FILE *ttf,struct ttfinfo *info) {
    info->justinuse = git_justinuse;
    _readttfmort(ttf,info);
    info->justinuse = git_normal;
}

/* Apple's docs imply that kerning info is always provided left to right, even*/
/*  for right to left scripts. My guess is that their docs are wrong, as they */
/*  often are, but if that be so then we need code in here to reverse */
/*  the order of the characters for right to left since pfaedit's convention */
/*  is to follow writing order rather than to go left to right */
void readttfkerns(FILE *ttf,struct ttfinfo *info) {
    int tabcnt, len, coverage,i,j, npairs, version, format, flags_good, tab;
    int left, right, offset, array, rowWidth;
    int header_size;
    KernPair *kp;
    KernClass *kc;
    uint32 begin_table;
    uint16 *class1, *class2;
    int tupleIndex;
    int isv;
    SplineChar **chars;
    OTLookup *otl;

    fseek(ttf,info->kern_start,SEEK_SET);
    version = getushort(ttf);
    tabcnt = getushort(ttf);
    if ( version!=0 ) {
	fseek(ttf,info->kern_start,SEEK_SET);
	version = getlong(ttf);
	tabcnt = getlong(ttf);
	if ( version!=0x10000 ) {
	    LogError(_("Invalid or unsupported version (0x%x) for 'kern' table"), version );
	    info->bad_gx = true;
return;
	}
    }
    if ( tabcnt<0 || tabcnt>0x1000 )
	LogError(_("Warning: Unlikely number of subtables (%d) for 'kern' table"), tabcnt );
    for ( tab=0; tab<tabcnt; ++tab ) {
	begin_table = ftell(ttf);
	if ( version==0 ) {
	    /* version = */ getushort(ttf);
	    len = getushort(ttf);
	    coverage = getushort(ttf);
	    format = coverage>>8;
	    flags_good = ((coverage&7)<=1);
	    isv = !(coverage&1);
	    tupleIndex = -1;
	    header_size = 6;
	} else {
	    len = getlong(ttf);
	    coverage = getushort(ttf);
	    /* Apple has reordered the bits */
	    format = (coverage&0xff);
	    flags_good = ((coverage&0xdf00)==0 || (coverage&0xdf00)==0x8000);
	    isv = coverage&0x8000? 1 : 0;
	    tupleIndex = getushort(ttf);
	    if ( coverage&0x2000 ) {
		if ( info->variations==NULL )
		    flags_good = false;	/* Ignore if we failed to load the tuple data */
		else if ( tupleIndex>=info->variations->tuple_count )
		    flags_good = false;	/* Bad tuple */
	    } else
		tupleIndex = -1;
	    header_size = 8;
	}
	otl = NULL;
	if ( flags_good ) {
	    otl = NewMacLookup(info,true);
	    otl->lookup_type = gpos_pair;
	    if ( isv ) {
		otl->features->featuretag = CHR('v','k','r','n');
		otl->features->ismac = false;
	    }
	    otl->subtables->per_glyph_pst_or_kern = true;
	    otl->subtables->vertical_kerning = isv;
	}
	if ( flags_good && format==0 ) {
	    /* format 0, horizontal kerning data (as pairs) not perpendicular */
	    chars = tupleIndex==-1 ? info->chars : info->variations->tuples[tupleIndex].chars;
	    npairs = getushort(ttf);
	    if ( version==0 && (len-14 != 6*npairs || npairs>10920 )) {
		LogError( _("In the 'kern' table, a subtable's length does not match the number of kerning pairs.") );
		info->bad_gx = true;
	    }
	    /* searchRange = */ getushort(ttf);
	    /* entrySelector = */ getushort(ttf);
	    /* rangeShift = */ getushort(ttf);
	    otl->subtables[0].per_glyph_pst_or_kern = true;
	    for ( j=0; j<npairs; ++j ) {
		left = getushort(ttf);
		right = getushort(ttf);
		offset = (short) getushort(ttf);
		if ( left<0 || right<0 ) {
		    /* We've seen such buggy fonts... */
		    LogError( _("Bad kern pair: glyphs %d & %d mustn't be negative\n"),
			    left, right );
		    info->bad_gx = true;
		} else if ( left>=info->glyph_cnt || right>=info->glyph_cnt ) {
		    /* Holes happen when reading ttc files. They are probably ok */
		    LogError( _("Bad kern pair: glyphs %d & %d must be less than %d\n"),
			    left, right, info->glyph_cnt );
		    info->bad_gx = true;
		} else if (chars[left]==NULL || chars[right]==NULL ) {
                    /* Shouldn't happen. */
		    LogError( _("Bad kern pair: glyphs at %d & %d are null\n"),
			    left, right);
		    info->bad_gx = true;
		} else {
		    kp = chunkalloc(sizeof(KernPair));
		    kp->sc = chars[right];
		    kp->off = offset;
		    kp->subtable = otl->subtables;
		    FListsAppendScriptLang(otl->features,SCScriptFromUnicode(chars[left]),
			    DEFAULT_LANG);
		    if ( isv ) {
			kp->next = chars[left]->vkerns;
			chars[left]->vkerns = kp;
		    } else {
			kp->next = chars[left]->kerns;
			chars[left]->kerns = kp;
		    }
		}
	    }
	    InfoNameOTLookup(otl,info);
	} else if ( flags_good && format==1 ) {
	    /* format 1 is an apple state machine which can handle weird cases */
	    /*  OpenType's spec doesn't document this */
	    /* Apple's docs are wrong about this table, they claim */
	    /*  there is a special value which marks the end of the kerning */
	    /*  lists. In fact there is no such value, the list is as long */
	    /*  as there are things on the kern stack */
	    otl->lookup_type = kern_statemachine;
	    readttf_mortx_asm(ttf,info,false,len-header_size,asm_kern,0,
		isv ? 0x80000000 : 0 /* coverage doesn't really apply */,otl);
	    fseek(ttf,begin_table+len,SEEK_SET);
	} else if ( flags_good && (format==2 || format==3 )) {
	    /* two class based formats */
	    KernClass **khead, **klast;
	    if ( isv && tupleIndex==-1 ) {
		khead = &info->vkhead;
		klast = &info->vklast;
	    } else if ( tupleIndex==-1 ) {
		khead = &info->khead;
		klast = &info->klast;
	    } else if ( isv ) {
		khead = &info->variations->tuples[tupleIndex].vkhead;
		klast = &info->variations->tuples[tupleIndex].vklast;
	    } else {
		khead = &info->variations->tuples[tupleIndex].khead;
		klast = &info->variations->tuples[tupleIndex].klast;
	    }
	    if ( *khead==NULL )
		*khead = kc = chunkalloc(sizeof(KernClass));
	    else
		kc = (*klast)->next = chunkalloc(sizeof(KernClass));
	    *klast = kc;
	    if ( format==2 ) {
		rowWidth = getushort(ttf);
		left = getushort(ttf);
		right = getushort(ttf);
		array = getushort(ttf);
		kc->second_cnt = rowWidth/sizeof(uint16);
		class1 = getAppleClassTable(ttf, begin_table+left, info->glyph_cnt, array, rowWidth, info );
		class2 = getAppleClassTable(ttf, begin_table+right, info->glyph_cnt, 0, sizeof(uint16), info );
		for ( i=0; i<info->glyph_cnt; ++i )
		    if ( class1[i]>kc->first_cnt )
			kc->first_cnt = class1[i];
		++ kc->first_cnt;
		kc->offsets = malloc(kc->first_cnt*kc->second_cnt*sizeof(int16));
		kc->adjusts = calloc(kc->first_cnt*kc->second_cnt,sizeof(DeviceTable));
		fseek(ttf,begin_table+array,SEEK_SET);
		for ( i=0; i<kc->first_cnt*kc->second_cnt; ++i )
		    kc->offsets[i] = getushort(ttf);
	    } else {
		/* format 3, horizontal kerning data (as classes limited to 256 entries) */
		/*  OpenType's spec doesn't document this */
		int gc, kv, flags;
		int16 *kvs;
		gc = getushort(ttf);
		kv = getc(ttf);
		kc->first_cnt = getc(ttf);
		kc->second_cnt = getc(ttf);
		flags = getc(ttf);
		if ( gc>info->glyph_cnt ) {
		    LogError( _("Kerning subtable 3 says the glyph count is %d, but maxp says %d\n"),
			    gc, info->glyph_cnt );
		    info->bad_gx = true;
		}
		class1 = calloc(gc>info->glyph_cnt?gc:info->glyph_cnt,sizeof(uint16));
		class2 = calloc(gc>info->glyph_cnt?gc:info->glyph_cnt,sizeof(uint16));
		kvs = malloc(kv*sizeof(int16));
		kc->offsets = malloc(kc->first_cnt*kc->second_cnt*sizeof(int16));
		kc->adjusts = calloc(kc->first_cnt*kc->second_cnt,sizeof(DeviceTable));
		for ( i=0; i<kv; ++i )
		    kvs[i] = (int16) getushort(ttf);
		for ( i=0; i<gc; ++i )
		    class1[i] = getc(ttf);
		for ( i=0; i<gc; ++i )
		    class2[i] = getc(ttf);
		for ( i=0; i<kc->first_cnt*kc->second_cnt; ++i )
		    kc->offsets[i] = kvs[getc(ttf)];
		free(kvs);
	    }
	    kc->firsts = ClassToNames(info,kc->first_cnt,class1,info->glyph_cnt);
	    kc->seconds = ClassToNames(info,kc->second_cnt,class2,info->glyph_cnt);
	    for ( i=0; i<info->glyph_cnt; ++i ) {
		if ( class1[i]>=4 && info->chars[i]!=NULL )
		    FListsAppendScriptLang(otl->features,
			    SCScriptFromUnicode(info->chars[i]),
			    DEFAULT_LANG);
	    }
	    free(class1); free(class2);
	    fseek(ttf,begin_table+len,SEEK_SET);
	    otl->subtables[0].kc = kc;
	    kc->subtable = otl->subtables;
	    InfoNameOTLookup(otl,info);
	} else {
	    LogError(_("Invalid or unsupported format (%d) for subtable of 'kern' table"), format );
	    info->bad_gx = true;
	    fseek(ttf,len-header_size,SEEK_CUR);
	    if ( otl!=NULL )
		OTLRemove(info,otl,true);
return;		/* Give up */
	}
    }
}

void readmacfeaturemap(FILE *ttf,struct ttfinfo *info) {
    MacFeat *last=NULL, *cur;
    struct macsetting *slast, *scur;
    struct fs { int n; int off; } *fs;
    int featcnt, i, j, flags;

    fseek(ttf,info->feat_start,SEEK_SET);
    /* version =*/ getfixed(ttf);
    featcnt = getushort(ttf);
    /* reserved */ getushort(ttf);
    /* reserved */ getlong(ttf);
    if ( feof(ttf)) {
	LogError( _("End of file in feat table.\n" ));
	info->bad_gx = true;
return;
    }

    fs = malloc(featcnt*sizeof(struct fs));
    for ( i=0; i<featcnt; ++i ) {
	cur = chunkalloc(sizeof(MacFeat));
	if ( last==NULL )
	    info->features = cur;
	else
	    last->next = cur;
	last = cur;

	cur->feature = getushort(ttf);
	fs[i].n = getushort(ttf);
	fs[i].off = getlong(ttf);
	flags = getushort(ttf);
	cur->strid = getushort(ttf);
	if ( flags&0x8000 ) cur->ismutex = true;
	if ( flags&0x4000 )
	    cur->default_setting = flags&0xff;
	if ( feof(ttf)) {
	    free(fs);
	    LogError( _("End of file in feat table.\n" ));
	    info->bad_gx = true;
return;
	}
    }

    for ( i=0, cur=info->features; i<featcnt; ++i, cur = cur->next ) {
	fseek(ttf,info->feat_start+fs[i].off,SEEK_SET);
	slast = NULL;
	for ( j=0; j<fs[i].n; ++j ) {
	    scur = chunkalloc(sizeof(struct macsetting));
	    if ( slast==NULL )
		cur->settings = scur;
	    else
		slast->next = scur;
	    slast = scur;

	    scur->setting = getushort(ttf);
	    scur->strid = getushort(ttf);
	    if ( feof(ttf)) {
		free(fs);
		LogError( _("End of file in feat table.\n") );
		info->bad_gx = true;
return;
	    }
	}
    }
    free(fs);
}

static void FeatMarkAsEnabled(struct ttfinfo *info,int featureType,
	int featureSetting) {
    MacFeat *f;
    struct macsetting *s;

    for ( f = info->features; f!=NULL && f->feature!=featureType; f=f->next );
    if ( f==NULL )
return;
    if ( f->ismutex ) {
	for ( s=f->settings ; s!=NULL; s=s->next )
	    s->initially_enabled = ( s->setting==featureSetting );
	f->default_setting = featureSetting;
    } else {
	for ( s=f->settings ; s!=NULL && s->setting!=featureSetting; s=s->next );
	if ( s!=NULL )
	    s->initially_enabled = true;
    }
return;
}

/******************************************************************************/
/* ******************************* MATH Table ******************************* */
/* ********************** (Not strictly OpenType yet) *********************** */
/******************************************************************************/

/* ******************************** Read MATH ******************************* */

static void ttf_math_read_constants(FILE *ttf,struct ttfinfo *info, uint32 start) {
    struct MATH *math;
    int i;
    uint16 off;

    fseek(ttf,start,SEEK_SET);
    info->math = math = calloc(1,sizeof(struct MATH));

    for ( i=0; math_constants_descriptor[i].script_name!=NULL; ++i ) {
	int16 *pos = (int16 *) (((char *) (math)) + math_constants_descriptor[i].offset );
	if ( pos == (int16 *) &math->MinConnectorOverlap )
    continue;		/* Actually lives in the Variant table, not here */
	*pos = getushort(ttf);
	if ( math_constants_descriptor[i].devtab_offset >= 0 ) {
	    DeviceTable **devtab = (DeviceTable **) (((char *) (math)) + math_constants_descriptor[i].devtab_offset );
	    off = getushort(ttf);
	    if ( off!=0 ) {
		*devtab = chunkalloc(sizeof(DeviceTable));
		ReadDeviceTable(ttf,*devtab,start+off,info);
	    }
	}
    }
}

static void ttf_math_read_icta(FILE *ttf,struct ttfinfo *info, uint32 start, int is_ic) {
    /* The italic correction and top accent sub-tables have the same format */
    int coverage, cnt, i, val, offset;
    uint16 *glyphs;

    fseek(ttf,start,SEEK_SET);
    coverage = getushort(ttf);
    cnt = getushort(ttf);
    glyphs = getCoverageTable(ttf,start+coverage,info);
    if ( glyphs==NULL )
return;
    fseek(ttf,start+4,SEEK_SET);
    for ( i=0; i<cnt; ++i ) {
      val = (int16) getushort(ttf);
      offset = getushort(ttf);
      if ( glyphs[i]<info->glyph_cnt && info->chars[ glyphs[i]]!=NULL ) {
	if ( is_ic )
	    info->chars[ glyphs[i] ]->italic_correction = val;
	else
	    info->chars[ glyphs[i] ]->top_accent_horiz = val;
	if ( offset!=0 ) {
	    DeviceTable *dv = chunkalloc(sizeof(DeviceTable));
	    ReadDeviceTable(ttf,dv,start+offset,info);
	    if ( is_ic )
		info->chars[ glyphs[i] ]->italic_adjusts = dv;
	    else
		info->chars[ glyphs[i] ]->top_accent_adjusts = dv;
	}
      }
    }
    free(glyphs);
}

static void ttf_math_read_extended(FILE *ttf,struct ttfinfo *info, uint32 start) {
    int i;
    uint16 *glyphs;

    glyphs = getCoverageTable(ttf,start,info);
    if ( glyphs==NULL )
return;
    for ( i=0; glyphs[i]!=0xffff; ++i ) if ( glyphs[i]<info->glyph_cnt && info->chars[ glyphs[i]]!=NULL )
	info->chars[ glyphs[i] ]->is_extended_shape = true;
    free(glyphs);
}

static void ttf_math_read_mathkernv(FILE *ttf, uint32 start,struct mathkernvertex *mkv,
	SplineChar *sc, int istop, struct ttfinfo *info) {
    int cnt, i;

    fseek(ttf,start,SEEK_SET);
    /* There is one more width than height. I store the width count */
    /*  and guess a dummy height later */
    mkv->cnt = cnt = getushort(ttf)+1;
    mkv->mkd = calloc(cnt,sizeof(struct mathkerndata));

    for ( i=0; i<cnt-1; ++i ) {
	mkv->mkd[i].height = getushort(ttf);
	mkv->mkd[i].height_adjusts = (void *) (intpt) getushort(ttf);
    }

    for ( i=0; i<cnt; ++i ) {
	mkv->mkd[i].kern = getushort(ttf);
	mkv->mkd[i].kern_adjusts = (void *) (intpt) getushort(ttf);
    }

    for ( i=0; i<cnt; ++i ) {
	DeviceTable *dv;
	uint32 offset;
	if ( mkv->mkd[i].height_adjusts!=NULL ) {
	    offset = start + (intpt) mkv->mkd[i].height_adjusts;
	    mkv->mkd[i].height_adjusts = dv = chunkalloc(sizeof(DeviceTable));
	    ReadDeviceTable(ttf,dv,offset,info);
	}
	if ( mkv->mkd[i].kern_adjusts!=NULL ) {
	    offset = start + (intpt) mkv->mkd[i].kern_adjusts;
	    mkv->mkd[i].kern_adjusts = dv = chunkalloc(sizeof(DeviceTable));
	    ReadDeviceTable(ttf,dv,offset,info);
	}
    }

    if ( cnt>=3 )
	mkv->mkd[cnt-1].height = 2*mkv->mkd[cnt-2].height - mkv->mkd[cnt-3].height;
    else if ( cnt>=2 )
	mkv->mkd[cnt-1].height = mkv->mkd[cnt-2].height + 100;
    else if ( cnt==1 ) {
	if ( istop ) {
	    DBounds b;
	    SplineCharQuickBounds(sc,&b);
	    mkv->mkd[cnt-1].height = b.maxy;
	} else
	    mkv->mkd[cnt-1].height = 0;
    }
}

static void ttf_math_read_mathkern(FILE *ttf,struct ttfinfo *info, uint32 start) {
    int coverage, cnt, i;
    uint16 *glyphs;
    struct koff { uint16 tr, tl, br, bl; } *koff;

    fseek(ttf,start,SEEK_SET);
    coverage = getushort(ttf);
    cnt = getushort(ttf);
    koff = malloc(cnt*sizeof(struct koff));
    for ( i=0; i<cnt; ++i ) {
	koff[i].tr = getushort(ttf);
	koff[i].tl = getushort(ttf);
	koff[i].br = getushort(ttf);
	koff[i].bl = getushort(ttf);
    }
    glyphs = getCoverageTable(ttf,start+coverage,info);
    if ( glyphs==NULL ) {
	free(koff);
return;
    }
    for ( i=0; i<cnt; ++i ) if ( glyphs[i]<info->glyph_cnt && info->chars[ glyphs[i]]!=NULL ) {
	SplineChar *sc = info->chars[ glyphs[i]];
	sc->mathkern = chunkalloc(sizeof(struct mathkern));
	if ( koff[i].tr!=0 )
	    ttf_math_read_mathkernv(ttf,start+koff[i].tr,&sc->mathkern->top_right,sc,true,info);
	if ( koff[i].tl!=0 )
	    ttf_math_read_mathkernv(ttf,start+koff[i].tl,&sc->mathkern->top_left,sc,true,info);
	if ( koff[i].br!=0 )
	    ttf_math_read_mathkernv(ttf,start+koff[i].br,&sc->mathkern->bottom_right,sc,false,info);
	if ( koff[i].bl!=0 )
	    ttf_math_read_mathkernv(ttf,start+koff[i].bl,&sc->mathkern->bottom_left,sc,false,info);
    }
    free(koff);
    free(glyphs);
}

static void ttf_math_read_glyphinfo(FILE *ttf,struct ttfinfo *info, uint32 start) {
    int icoff,taoff,esoff,kioff;

    fseek(ttf,start,SEEK_SET);
    icoff = getushort(ttf);
    taoff = getushort(ttf);
    esoff = getushort(ttf);
    kioff = getushort(ttf);

    if ( icoff!=0 )
	ttf_math_read_icta(ttf,info,start+icoff,true);
    if ( taoff!=0 )
	ttf_math_read_icta(ttf,info,start+taoff,false);
    if ( esoff!=0 )
	ttf_math_read_extended(ttf,info,start+esoff);
    if ( kioff!=0 )
	ttf_math_read_mathkern(ttf,info,start+kioff);
}

static struct glyphvariants *ttf_math_read_gvtable(FILE *ttf,struct ttfinfo *info, uint32 start,
	enum gsub_inusetype justinuse, SplineChar *basesc, int isv ) {
    struct glyphvariants *gv = chunkalloc(sizeof(struct glyphvariants));
    int ga_offset;
    int vcnt;
    uint16 *glyphs;
    int i, j, len;
    char *pt;
    int ic_offset, pcnt;
    SplineChar *sc;
    char ebuf[10], buffer[50], *ext;

    fseek(ttf,start,SEEK_SET);
    ga_offset = getushort(ttf);
    vcnt      = getushort(ttf);
    if ( vcnt!=0 ) {
	if ( justinuse==git_justinuse ) {
	    for ( i=0; i<vcnt; ++i ) {
		int gid = getushort(ttf);
		/* sizes[i] = */ getushort(ttf);
		if ( gid>=0 && gid<info->glyph_cnt )
		    info->inuse[gid] = true;
	    }
	} else if ( justinuse==git_findnames ) {
	    for ( i=0; i<vcnt; ++i ) {
		int gid = getushort(ttf);
		/* sizes[i] = */ getushort(ttf);
		if ( basesc!=NULL && basesc->name!=NULL &&
			gid>=0 && gid<info->glyph_cnt &&
			(sc = info->chars[gid])!=NULL && sc->name==NULL ) {
		    snprintf(buffer,sizeof(buffer),"%.30s.%csize%d",
			    basesc->name, isv?'v':'h', i);
		    sc->name = copy(buffer);
		}
	    }
	} else {
	    glyphs    = malloc(vcnt*sizeof(uint16));
	    len = 0;
	    for ( i=0; i<vcnt; ++i ) {
		glyphs[i]      = getushort(ttf);
		/* sizes[i] = */ getushort(ttf);
		if ( glyphs[i]<info->glyph_cnt && (sc = info->chars[ glyphs[i]])!=NULL )
		    len += strlen(sc->name)+1;
	    }
	    if ( len!=0 ) {
		gv->variants = pt = malloc(len);
		for ( i=len=0; i<vcnt; ++i ) {
		    if ( glyphs[i]<info->glyph_cnt && (sc = info->chars[ glyphs[i]])!=NULL ) {
			strcpy(pt+len,sc->name);
			len += strlen(sc->name);
			pt[len++] = ' ';
		    }
		}
		pt[len-1] = '\0';
	    }
	    free(glyphs);
	}
    }
    if ( ga_offset!=0 ) {
	start += ga_offset;
	fseek(ttf,start,SEEK_SET);
	gv->italic_correction = getushort(ttf);
	ic_offset = getushort(ttf);
	gv->part_cnt = pcnt = getushort(ttf);
	if ( justinuse==git_normal )
	    gv->parts = calloc(pcnt,sizeof(struct gv_part));
	for ( i=j=0; i<pcnt; ++i ) {
	    int gid, start, end, full, flags;
	    gid = getushort(ttf);
	    start = getushort(ttf);
	    end = getushort(ttf);
	    full = getushort(ttf);
	    flags = getushort(ttf);
	    if ( feof(ttf)) {
		LogError( _("Bad glyph variant subtable of MATH table.\n") );
		info->bad_ot = true;
		chunkfree(gv,sizeof(*gv));
return( NULL );
	    }
	    if ( justinuse==git_justinuse ) {
		if ( gid<info->glyph_cnt )
		    info->inuse[gid] = true;
	    } else if ( justinuse==git_findnames ) {
		if ( basesc!=NULL && basesc->name!=NULL &&
			gid>=0 && gid<info->glyph_cnt &&
			(sc = info->chars[gid])!=NULL && sc->name==NULL ) {
		    if ( pcnt==1 )
			ext = "repeat";
		    if ( i==0 )
			ext = isv ? "bot" : "left";
		    else if ( i==pcnt-1 )
			ext = isv ? "top" : "right";
		    else if ( i==1 && pcnt==3 )
			ext = "mid";
		    else {
			sprintf( ebuf, "%cpart%d", isv?'v':'h', i );
			ext = ebuf;
		    }
		    snprintf(buffer,sizeof(buffer),"%.30s.%s",
			    basesc->name, ext );
		    sc->name = copy(buffer);
		}
	    } else {
		if ( gid<info->glyph_cnt && (sc = info->chars[gid])!=NULL ) {
		    gv->parts[j].component = copy( sc->name );
		    gv->parts[j].startConnectorLength = start;
		    gv->parts[j].endConnectorLength = end;
		    gv->parts[j].fullAdvance = full;
		    gv->parts[j++].is_extender = flags&1;
		}
	    }
	}
	gv->part_cnt = j;
	if ( ic_offset!=0 && justinuse==git_normal ) {
	    gv->italic_adjusts = chunkalloc(sizeof(DeviceTable));
	    ReadDeviceTable(ttf,gv->italic_adjusts,start+ic_offset,info);
	}
    }
    if ( justinuse==git_justinuse ) {
	chunkfree(gv,sizeof(*gv));
return( NULL );
    }
return( gv );
}

static void ttf_math_read_variants(FILE *ttf,struct ttfinfo *info, uint32 start,
	enum gsub_inusetype justinuse) {
    int vcoverage, hcoverage, vcnt, hcnt;
    int *hoffs, *voffs;
    uint16 *hglyphs, *vglyphs;
    int i;

    fseek(ttf,start,SEEK_SET);
    if ( info->math==NULL )
	info->math = calloc(1,sizeof(struct MATH));
    info->math->MinConnectorOverlap = getushort(ttf);
    vcoverage = getushort(ttf);
    hcoverage = getushort(ttf);
    vcnt = getushort(ttf);
    hcnt = getushort(ttf);
    hoffs = malloc(hcnt*sizeof(int));
    voffs = malloc(vcnt*sizeof(int));

    for ( i=0; i<vcnt; ++i )
	voffs[i] = getushort(ttf);
    for ( i=0; i<hcnt; ++i )
	hoffs[i] = getushort(ttf);
    vglyphs = hglyphs = NULL;
    if ( vcoverage!=0 )
	vglyphs = getCoverageTable(ttf,start+vcoverage,info);
    if ( hcoverage!=0 )
	hglyphs = getCoverageTable(ttf,start+hcoverage,info);

    if ( vglyphs!=NULL ) {
	for ( i=0; i<vcnt; ++i ) if ( vglyphs[i]<info->glyph_cnt && voffs[i]!=0) {
	    if ( justinuse == git_normal || justinuse == git_findnames ) {
		SplineChar *sc = info->chars[ vglyphs[i]];
		if ( sc!=NULL )
		    sc->vert_variants = ttf_math_read_gvtable(ttf,info,start+voffs[i],justinuse,sc,true);
	    } else if ( info->inuse[ vglyphs[i]])
		ttf_math_read_gvtable(ttf,info,start+voffs[i],justinuse,NULL,true);
	}
    }
    if ( hglyphs!=NULL ) {
	for ( i=0; i<hcnt; ++i ) if ( hglyphs[i]<info->glyph_cnt && hoffs[i]!=0) {
	    if ( justinuse == git_normal || justinuse == git_findnames ) {
		SplineChar *sc = info->chars[ hglyphs[i]];
		if ( sc!=NULL )
		    sc->horiz_variants = ttf_math_read_gvtable(ttf,info,start+hoffs[i],justinuse,sc,false);
	    } else if ( info->inuse[ hglyphs[i]])
		    ttf_math_read_gvtable(ttf,info,start+hoffs[i],justinuse,NULL,false);
	}
    }

    free(vglyphs); free(voffs);
    free(hglyphs); free(hoffs);
}

static void _otf_read_math(FILE *ttf,struct ttfinfo *info,
	enum gsub_inusetype justinuse) {
    int constants, glyphinfo, variants;
    if ( info->math_start==0 )
return;
    fseek(ttf,info->math_start,SEEK_SET);

    info->g_bounds = info->math_start+info->math_length;

    if ( getlong(ttf)!=0x00010000 )
return;
    constants = getushort(ttf);
    glyphinfo = getushort(ttf);
    variants = getushort(ttf);

    if ( justinuse == git_normal ) {
	if ( constants!=0 )
	    ttf_math_read_constants(ttf,info,info->math_start+constants);
	if ( glyphinfo!=0 )
	    ttf_math_read_glyphinfo(ttf,info,info->math_start+glyphinfo);
    }
    if ( variants!=0 )
	ttf_math_read_variants(ttf,info,info->math_start+variants,justinuse);
    if ( ftell(ttf)>info->g_bounds ) {
	LogError(_("MATH table extends beyond table bounds"));
	info->bad_ot = true;
    }
    info->g_bounds = 0;
}

void otf_read_math(FILE *ttf,struct ttfinfo *info) {
    _otf_read_math(ttf,info,git_normal);
}

void otf_read_math_used(FILE *ttf,struct ttfinfo *info) {
    _otf_read_math(ttf,info,git_justinuse);
}

void GuessNamesFromMATH(FILE *ttf,struct ttfinfo *info) {
    _otf_read_math(ttf,info,git_findnames);
}

static struct baselangextent *readttfbaseminmax(FILE *ttf,uint32 offset,struct ttfinfo *info,
	uint32 script_tag,uint32 lang_tag) {
    int j,feat_cnt;
    struct baselangextent *lang, *cur, *last;

    fseek(ttf,offset,SEEK_SET);
    lang = chunkalloc(sizeof(struct baselangextent));
    lang->lang = lang_tag;
    lang->descent = (short) getushort(ttf);
    lang->ascent  = (short) getushort(ttf);

    feat_cnt = getushort(ttf);
    last = NULL;
    for ( j=0; j<feat_cnt; ++j ) {
	cur = chunkalloc(sizeof(struct baselangextent));
	if ( last==NULL )
	    lang->features = cur;
	else
	    last->next = cur;
	last = cur;
	cur->lang = getlong(ttf);		/* Actually feature tag here */
	cur->descent = (short) getushort(ttf);
	cur->ascent  = (short) getushort(ttf);
    }
return( lang );
}

void readttfbase(FILE *ttf,struct ttfinfo *info) {
    int version;
    uint32 axes[2];
    uint32 basetags, basescripts;
    int basescriptcnt;
    struct tagoff { uint32 tag; uint32 offset; } *bs;
    int axis,i,j, tot;
    struct Base *curBase;
    struct basescript *curScript, *last;
    struct baselangextent *cur, *lastLang;
    
    if ( info->base_start==0 )
return;
    fseek(ttf,info->base_start,SEEK_SET);

    version = getlong(ttf);
    if ( version!=0x00010000 )
return;
    axes[0] = getushort(ttf);	/* Horizontal */
    axes[1] = getushort(ttf);	/* Vertical */

    for ( axis=0; axis<2; ++axis ) {
	if ( axes[axis]==0 )
    continue;
	fseek(ttf,info->base_start+axes[axis],SEEK_SET);
	curBase = chunkalloc(sizeof(struct Base));
	if ( axis==0 ) info->horiz_base = curBase; else info->vert_base = curBase;
	basetags    = getushort(ttf);
	basescripts = getushort(ttf);
	if ( basetags==0 ) {
	    curBase->baseline_cnt = 0;
	    curBase->baseline_tags = NULL;
	} else {
	    fseek(ttf,info->base_start+axes[axis]+basetags,SEEK_SET);
	    curBase->baseline_cnt = getushort(ttf);
	    curBase->baseline_tags = calloc(curBase->baseline_cnt,sizeof(uint32));
	    for ( i=0; i<curBase->baseline_cnt; ++i )
		curBase->baseline_tags[i] = getlong(ttf);
	}
	if ( basescripts!=0 ) {
	    fseek(ttf,info->base_start+axes[axis]+basescripts,SEEK_SET);
	    basescriptcnt = getushort(ttf);
	    bs = calloc(basescriptcnt,sizeof(struct tagoff));
	    for ( i=0; i<basescriptcnt; ++i ) {
		bs[i].tag    = getlong(ttf);
		bs[i].offset = getushort(ttf);
		if ( bs[i].offset != 0 )
		    bs[i].offset += info->base_start+axes[axis]+basescripts;
	    }
	    last = NULL;
	    for ( i=0; i<basescriptcnt; ++i ) if ( bs[i].offset!=0 ) {
		int basevalues, defminmax;
		int langsyscnt;
		struct tagoff *ls;
		fseek(ttf,bs[i].offset,SEEK_SET);
		basevalues = getushort(ttf);
		defminmax  = getushort(ttf);
		langsyscnt = getushort(ttf);
		ls = calloc(langsyscnt,sizeof(struct tagoff));
		for ( j=0; j<langsyscnt; ++j ) {
		    ls[j].tag    = getlong(ttf);
		    ls[j].offset = getushort(ttf);
		}
		curScript = chunkalloc(sizeof(struct basescript));
		if ( last==NULL )
		    curBase->scripts = curScript;
		else
		    last->next = curScript;
		last = curScript;
		curScript->script = bs[i].tag;
		if ( basevalues!=0 ) {
		    int coordcnt;
		    int *coords;

		    fseek( ttf,bs[i].offset+basevalues,SEEK_SET);
		    curScript->def_baseline = getushort(ttf);
		    tot = coordcnt = getushort(ttf);
		    if ( coordcnt!=curBase->baseline_cnt ) {
			info->bad_ot = true;
			LogError(_("!!!!! Coord count (%d) for '%c%c%c%c' script does not match base tag count (%d) in 'BASE' table\n"),
				coordcnt,
				bs[i].tag>>24, bs[i].tag>>16, bs[i].tag>>8, bs[i].tag,
				curBase->baseline_cnt );
			if ( tot<curBase->baseline_cnt )
			    tot = curBase->baseline_cnt;
		    }
		    coords = calloc(coordcnt,sizeof(int));
		    curScript->baseline_pos = calloc(tot,sizeof(int16));
		    for ( j=0; j<coordcnt; ++j )
			coords[j] = getushort(ttf);
		    for ( j=0; j<coordcnt; ++j ) if ( coords[j]!=0 ) {
			int format;
			fseek( ttf,bs[i].offset+basevalues+coords[j],SEEK_SET);
			format = getushort(ttf);
			curScript->baseline_pos[j]  = (short) getushort(ttf);
			if ( format!=1 && format!=2 && format!=3 ) {
			    info->bad_ot = true;
			    LogError(_("!!!!! Bad Base Coord format (%d) for '%c%c%c%c' in '%c%c%c%c' script in 'BASE' table\n"),
					format,
					curBase->baseline_tags[j]>>24, curBase->baseline_tags[j]>>16, curBase->baseline_tags[j]>>8, curBase->baseline_tags[j],
					bs[i].tag>>24, bs[i].tag>>16, bs[i].tag>>8, bs[i].tag );
			}
		    }
		    free(coords);
		} else {
		    curScript->baseline_pos = calloc(curBase->baseline_cnt,sizeof(int16));
		}
		lastLang = NULL;
		if ( defminmax!=0 )
		    curScript->langs = lastLang = readttfbaseminmax(ttf,bs[i].offset+defminmax,info,bs[i].tag,DEFAULT_LANG);
		if ( langsyscnt!=0 ) {
		    for ( j=0; j<langsyscnt; ++j ) if ( ls[j].offset!=0 ) {
			cur = readttfbaseminmax(ttf,bs[i].offset+ls[j].offset,info,bs[i].tag,ls[j].tag);
			if ( last==NULL )
			    curScript->langs = cur;
			else
			    lastLang->next = cur;
			lastLang = cur;
		    }
		}
		free(ls);
	    }
	    free(bs);
	}
    }
}

static void bsln_apply_values(struct ttfinfo *info, int gfirst, int glast,FILE *ttf) {
    int i;

    for ( i=gfirst; i<=glast; ++i )
	info->bsln_values[i]= getushort(ttf);
}

static void bsln_apply_value(struct ttfinfo *info, int gfirst, int glast,FILE *ttf) {
    int i;
    int bsln;

    bsln = getushort(ttf);
    for ( i=gfirst; i<=glast; ++i )
	info->bsln_values[i]= bsln;
}

static void bsln_apply_default(struct ttfinfo *info, int gfirst, int glast,void *def) {
    int def_bsln, i;

    def_bsln = (intpt) def;
    for ( i=gfirst; i<=glast; ++i )
	info->bsln_values[i]= def_bsln;
}

void readttfbsln(FILE *ttf,struct ttfinfo *info) {
    int def, ap_def, version, format;
    uint16 *values;
    int offsets[32];
    SplineChar *sc;
    BasePoint pos;
    int mapping[32];
    int i;
    struct Base *base;
    struct basescript *bs;

    fseek(ttf,info->bsln_start,SEEK_SET);
    version = getlong(ttf);
    if ( version!=0x00010000 )
return;
    format = getushort(ttf);
    def = getushort(ttf);
    if ( format==0 || format==1 ) {
	for ( i=0; i<32; ++i )
	    offsets[i] = (int16) getushort(ttf);
    } else if ( format==2 || format==3 ) {
	int stdGID = getushort(ttf);
	int ptnum;
	if ( stdGID>=info->glyph_cnt || (sc = info->chars[stdGID])==NULL )
return;
	for ( i=0; i<32; ++i ) {
	    ptnum = getushort(ttf);
	    if ( ttfFindPointInSC(sc,ly_fore,ptnum,&pos,NULL)!=-1 )
return;
	    offsets[i] = pos.y;
	}
    }

    if ( format&1 ) {
	info->bsln_values = values = calloc(info->glyph_cnt,sizeof(uint16));
	readttf_applelookup(ttf,info,
		bsln_apply_values,bsln_apply_value,
		bsln_apply_default,(void *) (intpt) def, false);
    } else
	values = NULL;

    for ( i=1; i<32; ++i ) mapping[i] = 3;		/* Roman */

    info->horiz_base = base = chunkalloc(sizeof(struct Base));
    base->baseline_cnt = 4;
    base->baseline_tags = malloc(4*sizeof(uint32));
    base->baseline_tags[0] = CHR('h','a','n','g');	/* Apple 3 */
    if ( offsets[1]!= offsets[2] ) {
	base->baseline_tags[1] = CHR('i','d','e','o');	/* Apple 2 */
	base->baseline_tags[2] = CHR('m','a','t','h');	/* Apple 4 */
	base->baseline_tags[3] = CHR('r','o','m','n');	/* Apple 0 */

	/* Map from Apple's baseline indeces, to OT tag positions */
	mapping[3] = 0;
	mapping[2] = 1;
	mapping[4] = 2;
	mapping[0] = 3;
	/* Apple baseline 1 does not map to an OT tag */
	/* I assume baseline 1 is the normal baseline for CJK on Macs */
	/* (because baseline 2 often (and wrongly) contains the same value */
    } else {
	/* baseline 1 (centered ideographic) and baseline 2 (low ideographic) */
	/*  are documented to be different. Yet most of the fonts I have looked*/
	/*  at (and I've only got about 4 test cases) have the low ideographic*/
	/*  baseline set to the centered baseline. If I were to copy that to  */
	/*  OT it would be really bad. So even though baseline data is provided*/
	/*  I shall ignore it if it looks WRONG! */
	base->baseline_cnt = 3;
	base->baseline_tags[1] = CHR('m','a','t','h');	/* Apple 4 */
	base->baseline_tags[2] = CHR('r','o','m','n');	/* Apple 0 */
	mapping[3] = 0;
	mapping[4] = 1;
	mapping[0] = 2;
    }

    for ( i=0; i<info->glyph_cnt; ++i ) if ( (sc=info->chars[i])!=NULL ) {
	uint32 script = SCScriptFromUnicode(sc);
	if ( script==DEFAULT_SCRIPT )
    continue;
	for ( bs=base->scripts; bs!=NULL && bs->script!=script; bs=bs->next );
	if ( bs!=NULL )
    continue;
	bs = chunkalloc(sizeof(struct basescript));
	bs->script = script;
	ap_def = def;
	if ( values!=NULL )
	    ap_def = values[i];
	bs->def_baseline = mapping[ap_def];
	bs->baseline_pos = malloc((base->baseline_cnt<5?5:base->baseline_cnt)*sizeof(int16));
	for ( i=0; i<5; ++i ) if ( i!=1 )
	    bs->baseline_pos[mapping[i]] = offsets[i] - offsets[ap_def];
	bs->next = base->scripts;
	base->scripts = bs;
    }
}

static char *jstf_read_extenders(FILE *ttf,uint32 spos,int extendOff,
	struct ttfinfo *info) {
    uint16 *glyphs;
    int cnt, i;
    char *ret;

    if ( extendOff==0 )
return( NULL );
    if ( spos+extendOff+2>info->g_bounds ) {
	LogError( _("JSTF table is too long.\n") );
	info->bad_ot = true;
return( NULL );
    }

    fseek(ttf,spos+extendOff,SEEK_SET);
    cnt = getushort(ttf);
    if ( spos+extendOff+2+2*cnt>info->g_bounds || cnt<0 ) {
	LogError( _("JSTF table is too long.\n") );
	info->bad_ot = true;
return( NULL );
    }
    if ( cnt==0 )
return( NULL );
    glyphs = malloc((cnt+1)*sizeof(uint16));
    for ( i=0; i<cnt; ++i ) {
	glyphs[i] = getushort(ttf);
	if ( glyphs[i]>=info->glyph_cnt ) {
	    LogError( _("Bad GID in JSTF extenser table.\n") );
	    glyphs[i] = 0;
	    info->bad_ot = true;
	}
    }
    glyphs[i] = 0xffff;
    ret = GlyphsToNames(info,glyphs,false);
    free(glyphs);
return( ret );
}

static OTLookup *findLookupByIndex(OTLookup *lookups,int index) {

    while ( index>0 ) {
	if ( lookups==NULL )
return( NULL );
	lookups = lookups->next;
	--index;
    }
return( lookups );
}

static OTLookup **jstf_subpos(FILE *ttf,uint32 base,int Sub,int Pos,
	struct ttfinfo *info) {
    int scnt=0, pcnt=0;
    OTLookup **ret;
    int i;

    if ( Sub>0 ) {
	fseek(ttf,base+Sub,SEEK_SET);
	scnt = getushort(ttf);
	if ( base+Sub+2+scnt*sizeof(int16)>info->g_bounds || scnt<0 ) {
	    LogError( _("JSTF table is too long.\n") );
	    info->bad_ot = true;
return( NULL );
	}
    }
    if ( Pos>0 ) {
	fseek(ttf,base+Pos,SEEK_SET);
	pcnt = getushort(ttf);
	if ( base+Pos+2+pcnt*sizeof(int16)>info->g_bounds || pcnt<0 ) {
	    LogError( _("JSTF table is too long.\n") );
	    info->bad_ot = true;
return( NULL );
	}
    }
    if ( scnt==0 && pcnt==0 )
return( NULL );

    ret = malloc((scnt+pcnt+1)*sizeof(OTLookup *));
    if ( Sub>0 ) {
	fseek(ttf,base+Sub+2,SEEK_SET);
	for ( i=0; i<scnt; ++i ) {
	    int index = getushort(ttf);
	    if ( index<0 ) {
		LogError( _("JSTF table is too long.\n") );
		info->bad_ot = true;
		free(ret);
return( NULL );
	    }
	    ret[i] = findLookupByIndex(info->gsub_lookups,index);
	    if ( ret[i]==NULL ) {
		LogError( _("Lookup index (%d) out of bounds in GSUB from JSTF table.\n"), index );
		info->bad_ot = true;
		free(ret);
return( NULL );
	    }
	}
	ret[i] = NULL;
    }
    if ( Pos>0 ) {
	fseek(ttf,base+Pos+2,SEEK_SET);
	for ( i=0; i<pcnt; ++i ) {
	    int index = getushort(ttf);
	    if ( index<0 ) {
		LogError( _("JSTF table is too long.\n") );
		info->bad_ot = true;
		free(ret);
return( NULL );
	    }
	    ret[i+scnt] = findLookupByIndex(info->gpos_lookups,index);
	    if ( ret[i+scnt]==NULL ) {
		LogError( _("Lookup index (%d) out of bounds in GPOS from JSTF table.\n"), index );
		info->bad_ot = true;
		free(ret);
return( NULL );
	    }
	}
	ret[i+scnt] = NULL;
    }
return( ret );
}

static void NameOTJSTFLookup(OTLookup *otl,struct ttfinfo *info) {
    char buffer[300], *format;
    struct lookup_subtable *subtable;
    int cnt;

    if ( info->jstf_isShrink )
/* GT: This string is used to generate a name for an OpenType lookup. */
/* GT:  the %c%c... is the language followed by the script (OT tags) */
	snprintf(buffer,sizeof(buffer), _("JSTF shrinkage max at priority %d #%d for %c%c%c%c in %c%c%c%c"),
		info->jstf_prio, info->jstf_lcnt++,
		info->jstf_lang>>24,
		 info->jstf_lang>>16,
		 info->jstf_lang>>8,
		 info->jstf_lang,
		info->jstf_script>>24,
		 info->jstf_script>>16,
		 info->jstf_script>>8,
		 info->jstf_script );
     else
	snprintf(buffer,sizeof(buffer), _("JSTF extension max at priority %d #%d for %c%c%c%c in %c%c%c%c"),
		info->jstf_prio, info->jstf_lcnt++,
		info->jstf_lang>>24,
		 info->jstf_lang>>16,
		 info->jstf_lang>>8,
		 info->jstf_lang,
		info->jstf_script>>24,
		 info->jstf_script>>16,
		 info->jstf_script>>8,
		 info->jstf_script );
    otl->lookup_name = copy(buffer);

    cnt = 0;
    for ( subtable = otl->subtables; subtable!=NULL; subtable=subtable->next, ++cnt ) {
	if ( subtable->subtable_name==NULL ) {
	    if ( subtable==otl->subtables && subtable->next==NULL )
		format = _("%s subtable");
	    else
		format = _("%s subtable %d");
	    snprintf(buffer,sizeof(buffer),format,otl->lookup_name,cnt );
	    subtable->subtable_name=copy(buffer);
	}
    }
}

static OTLookup **jstf_processlookups(FILE *ttf,uint32 base,int lookup_off,
	struct ttfinfo *info) {
    OTLookup **ret;
    struct lookup *lookups, *l;
    struct lookup_subtable *subtable;
    int cnt,k;
    int32 st;

    if ( lookup_off==0 )
return( NULL );
    lookups = readttflookups(ttf,base+lookup_off,info,2);

    if ( lookups==NULL )
return( NULL );

    for ( l = lookups, cnt=0; l->offset!=0; ++l, ++cnt ) {
	for ( k=0, subtable=l->otlookup->subtables; k<l->subtabcnt; ++k, subtable=subtable->next ) {
	    st = l->subtab_offsets[k];
	    fseek(ttf,st,SEEK_SET);
	    gposLookupSwitch(ttf,st,info,l,subtable,lookups);
	}
    }
    ret = malloc((cnt+1)*sizeof(OTLookup*));

    for ( l=lookups, cnt=0; l->offset!=0; ++l, ++cnt ) {
	NameOTJSTFLookup(l->otlookup,info);
	ret[cnt] = l->otlookup;
    }
    ret[cnt] = NULL;
    LookupsFree(lookups);
return( ret );
}

static struct jstf_lang *jstf_lang(FILE *ttf,uint32 base,
	int off, uint32 tag, struct ttfinfo *info) {
    int cnt,i;
    struct jstf_lang *ret;

    if ( off<=0 )
return( NULL );
    if ( base+off+2>info->g_bounds ) {
	LogError( _("JSTF table is too long.\n") );
	info->bad_ot = true;
return( NULL );
    }
    fseek(ttf,base+off,SEEK_SET);
    cnt = getushort(ttf);
    if ( base+off+2+2*cnt>info->g_bounds || cnt<0 ) {
	LogError( _("JSTF table is too long.\n") );
	info->bad_ot = true;
return( NULL );
    }
    if ( cnt==0 )
return( NULL );

    ret = chunkalloc(sizeof(struct jstf_lang));
    ret->lang = info->jstf_lang = tag;
    ret->cnt = cnt;
    ret->prios = calloc(cnt,sizeof(struct jstf_prio));
    for ( i=0; i<cnt; ++i )
	ret->prios[i].maxExtend = (void *) (intpt) getushort(ttf);
    for ( i=0; i<cnt; ++i ) {
	int enSub, disSub, enPos, disPos, maxShrink, maxExtend;
	int enSub2, disSub2, enPos2, disPos2;
	uint32 pbase = base+off+(intpt) ret->prios[i].maxExtend;
	fseek(ttf,pbase,SEEK_SET);
	info->jstf_prio = i;
	enSub = getushort(ttf);
	disSub = getushort(ttf);
	enPos = getushort(ttf);
	disPos = getushort(ttf);
	maxShrink = getushort(ttf);
	enSub2 = getushort(ttf);
	disSub2 = getushort(ttf);
	enPos2 = getushort(ttf);
	disPos2 = getushort(ttf);
	maxExtend = getushort(ttf);
	ret->prios[i].enableShrink = jstf_subpos(ttf,pbase,enSub,enPos, info);
	ret->prios[i].disableShrink = jstf_subpos(ttf,pbase,disSub,disPos, info);
	ret->prios[i].enableExtend = jstf_subpos(ttf,pbase,enSub2,enPos2, info);
	ret->prios[i].disableExtend = jstf_subpos(ttf,pbase,disSub2,disPos2, info);

	info->jstf_isShrink = true;
	ret->prios[i].maxShrink = jstf_processlookups(ttf,pbase,maxShrink, info);
	info->jstf_isShrink = false;
	ret->prios[i].maxExtend = jstf_processlookups(ttf,pbase,maxExtend, info);
    }
return( ret );
}

void readttfjstf(FILE *ttf,struct ttfinfo *info) {
    int version;
    int scnt, lcnt, lmax;
    int i,j;
    struct tagoff { uint32 tag; int offset; } *soff, *loff;
    Justify *last=NULL, *cur;
    struct jstf_lang *llast, *lcur;
    int extendOff, defOff;
    
    if ( info->jstf_start==0 )
return;
    fseek(ttf,info->jstf_start,SEEK_SET);
    info->g_bounds = info->jstf_start + info->jstf_length;

    version = getlong(ttf);
    if ( version!=0x00010000 )
return;
    scnt = getushort(ttf);
    if ( scnt<0 || scnt>1000 ) {
	LogError( _("Unlikely script count (%d), I suspect the JSTF-\n table is garbage, I'm giving up on it.\n"), scnt );
	info->bad_ot = true;
return;
    }
    soff = malloc(scnt*sizeof(struct tagoff));

    for ( i=0; i<scnt; ++i ) {
	soff[i].tag    = getlong(ttf);
	soff[i].offset = getushort(ttf);
	if ( soff[i].offset<0 ) {
	    LogError( _("End of file found in JSTF table.\n") );
	    info->bad_ot = true;
	    free(soff);
return;
	}
    }
    if ( ftell(ttf)>info->g_bounds ) {
	LogError( _("JSTF table is too long.\n") );
	info->bad_ot = true;
	free(soff);
return;
    }
    lmax = 0; loff = NULL;
    for ( i=0; i<scnt; ++i ) {
	fseek(ttf,info->jstf_start+soff[i].offset,SEEK_SET);
	extendOff = getushort(ttf);
	defOff = getushort(ttf);
	lcnt = getushort(ttf);
	if ( info->jstf_start+soff[i].offset > info->g_bounds-6-6*lcnt || lcnt<0 ) {
	    LogError( _("JSTF table is too long.\n") );
	    info->bad_ot = true;
	    free(soff);
return;
	}

	if ( lcnt>lmax )
	    loff = realloc(loff,(lmax=lcnt)*sizeof(struct tagoff));
	for ( j=0; j<lcnt; ++j ) {
	    loff[j].tag    = getlong(ttf);
	    loff[j].offset = getushort(ttf);
	    if ( loff[j].offset<0 ) {
		LogError( _("End of file found in JSTF table.\n") );
		info->bad_ot = true;
		free(soff);
		free(loff);
return;
	    }
	}

	cur = chunkalloc(sizeof(Justify));
	cur->script = info->jstf_script = soff[i].tag;
	if ( last==NULL )
	    info->justify = cur;
	else
	    last->next = cur;
	last = cur;
	cur->extenders = jstf_read_extenders(ttf,info->jstf_start+soff[i].offset,extendOff, info);
	llast = NULL;
	if ( defOff!=0 )
	    llast = cur->langs = jstf_lang(ttf,info->jstf_start+soff[i].offset,defOff,DEFAULT_LANG, info);
	for ( j=0; j<lcnt; ++j ) {
	    lcur = jstf_lang(ttf,info->jstf_start+soff[i].offset,
		    loff[j].offset, loff[j].tag, info);
	    if ( lcur!=NULL ) {
		if ( llast==NULL )
		    cur->langs = lcur;
		else
		    llast->next = lcur;
		llast = lcur;
	    }
	}
    }

    free(loff);
    free(soff);
}
