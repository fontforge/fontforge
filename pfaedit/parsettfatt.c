/* Copyright (C) 2000-2003 by George Williams */
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
#include "pfaeditui.h"
#include <chardata.h>
#include <utype.h>
#include <ustring.h>
#include <math.h>
#include <locale.h>
#include "ttf.h"

static int SLIFromInfo(struct ttfinfo *info,SplineChar *sc,uint32 lang) {
    uint32 script = SCScriptFromUnicode(sc);
    int j;

    if ( script==0 ) script = CHR('l','a','t','n');
    if ( info->script_lang==NULL ) {
	info->script_lang = galloc(2*sizeof(struct script_record *));
	j = 0;
    } else {
	for ( j=0; info->script_lang[j]!=NULL; ++j ) {
	    if ( info->script_lang[j][0].script==script &&
		    info->script_lang[j][1].script == 0 &&
		    info->script_lang[j][0].langs[0] == lang &&
		    info->script_lang[j][0].langs[1] == 0 )
return( j );
	}
    }
    info->script_lang = grealloc(info->script_lang,(j+2)*sizeof(struct script_record *));
    info->script_lang[j+1] = NULL;
    info->script_lang[j]= gcalloc(2,sizeof(struct script_record));
    info->script_lang[j][0].script = script;
    info->script_lang[j][0].langs = gcalloc(2,sizeof(uint32));
    info->script_lang[j][0].langs[0] = lang;
return( j );
}

static uint16 *getAppleClassTable(FILE *ttf, int classdef_offset, int cnt, int sub, int div) {
    uint16 *class = gcalloc(cnt,sizeof(uint16));
    int first, i, n;
    /* Apple stores its class tables as containing offsets. I find it hard to */
    /*  think that way and convert them to indeces (by subtracting off a base */
    /*  offset and dividing by the item's size) before doing anything else */

    fseek(ttf,classdef_offset,SEEK_SET);
    first = getushort(ttf);
    n = getushort(ttf);
    if ( first+n+1>=cnt )
	fprintf( stderr, "Bad Apple Kern Class\n" );
    for ( i=0; i<=n && i+first<cnt; ++i )
	class[first+i] = (getushort(ttf)-sub)/div;
return( class );
}

static char **ClassToNames(struct ttfinfo *info,int class_cnt,uint16 *class,int glyph_cnt) {
    char **ret = galloc(class_cnt*sizeof(char *));
    int *lens = gcalloc(class_cnt,sizeof(int));
    int i;

    ret[0] = NULL;
    for ( i=0 ; i<glyph_cnt; ++i ) if ( class[i]!=0 && info->chars[i]!=NULL && class[i]<class_cnt )
	lens[class[i]] += strlen(info->chars[i]->name)+1;
    for ( i=1; i<class_cnt ; ++i )
	ret[i] = galloc(lens[i]+1);
    memset(lens,0,class_cnt*sizeof(int));
    for ( i=0 ; i<glyph_cnt; ++i ) if ( class[i]!=0 && info->chars[i]!=NULL ) {
	if ( class[i]<class_cnt ) {
	    strcpy(ret[class[i]]+lens[class[i]], info->chars[i]->name );
	    lens[class[i]] += strlen(info->chars[i]->name)+1;
	    ret[class[i]][lens[class[i]]-1] = ' ';
	} else
	    fprintf( stderr, "Class index out of range %d (must be <%d)\n",class[i], class_cnt );
    }
    for ( i=1; i<class_cnt ; ++i )
	if ( lens[i]==0 )
	    ret[i][0] = '\0';
	else
	    ret[i][lens[i]-1] = '\0';
    free(lens);
return( ret );
}

static int ClassFindCnt(uint16 *class,int tot) {
    int i, max=0;

    for ( i=0; i<tot; ++i )
	if ( class[i]>max ) max = class[i];
return( max+1 );
}

static char *GlyphsToNames(struct ttfinfo *info,uint16 *glyphs) {
    int i, len;
    char *ret, *pt;

    if ( glyphs==NULL )
return( copy(""));
    for ( i=len=0 ; glyphs[i]!=0xffff; ++i )
	len += strlen(info->chars[glyphs[i]]->name)+1;
    ret = pt = galloc(len+1); *pt = '\0';
    for ( i=0 ; glyphs[i]!=0xffff; ++i ) {
	strcpy(pt,info->chars[glyphs[i]]->name);
	pt += strlen(pt);
	*pt++ = ' ';
    }
    if ( pt>ret ) pt[-1] = '\0';
return( ret );
}

#define MAX_LANG 		20		/* Don't support more than 20 languages per feature (only remember first 20) */
struct scriptlist {
    uint32 script;
    uint32 langs[MAX_LANG];
    int lang_cnt;
    struct scriptlist *next;
};

struct feature {
    uint32 offset;
    uint32 tag;
    struct scriptlist *sl, *reqsl;
    /*int script_lang_index;*/
    int lcnt;
    uint16 *lookups;
};

struct lookup {
    uint32 tag, subtag;
    struct scriptlist *sl;
    int script_lang_index;
    uint16 flags;
    uint16 lookup;
    uint32 offset;
    struct lookup *alttags;
    int make_subtag;
};

enum gsub_inusetype { git_normal, git_justinuse, git_findnames };

static uint16 *getCoverageTable(FILE *ttf, int coverage_offset, struct ttfinfo *info) {
    int format, cnt, i,j, rcnt;
    uint16 *glyphs=NULL;
    int start, end, ind, max;

    fseek(ttf,coverage_offset,SEEK_SET);
    format = getushort(ttf);
    if ( format==1 ) {
	cnt = getushort(ttf);
	glyphs = galloc((cnt+1)*sizeof(uint16));
	for ( i=0; i<cnt; ++i ) {
	    glyphs[i] = getushort(ttf);
	    if ( glyphs[i]>=info->glyph_cnt ) {
		fprintf( stderr, "Bad coverage table. Glyph %d out of range [0,%d)\n", glyphs[i], info->glyph_cnt );
		glyphs[i] = 0;
	    }
	}
    } else if ( format==2 ) {
	glyphs = gcalloc((max=256),sizeof(uint16));
	rcnt = getushort(ttf); cnt = 0;
	for ( i=0; i<rcnt; ++i ) {
	    start = getushort(ttf);
	    end = getushort(ttf);
	    ind = getushort(ttf);
	    if ( start>end || end>=info->glyph_cnt ) {
		fprintf( stderr, "Bad coverage table. Glyph range %d-%d out of range [0,%d)\n", start, end, info->glyph_cnt );
		start = end = 0;
	    }
	    if ( ind+end-start+2 >= max ) {
		int oldmax = max;
		max = ind+end-start+2;
		glyphs = grealloc(glyphs,max*sizeof(uint16));
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
	fprintf( stderr, "Bad format for coverage table %d\n", format );
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

static uint16 *getClassDefTable(FILE *ttf, int classdef_offset, int cnt) {
    int format, i, j;
    uint16 start, glyphcnt, rangecnt, end, class;
    uint16 *glist=NULL;

    fseek(ttf, classdef_offset, SEEK_SET);
    glist = galloc(cnt*sizeof(uint16));
    for ( i=0; i<cnt; ++i )
	glist[i] = 0;	/* Class 0 is default */
    format = getushort(ttf);
    if ( format==1 ) {
	start = getushort(ttf);
	glyphcnt = getushort(ttf);
	if ( start+(int) glyphcnt>cnt ) {
	    fprintf( stderr, "Bad class def table. start=%d cnt=%d, max glyph=%d\n", start, glyphcnt, cnt );
	    glyphcnt = cnt-start;
	}
	for ( i=0; i<glyphcnt; ++i )
	    glist[start+i] = getushort(ttf);
    } else if ( format==2 ) {
	rangecnt = getushort(ttf);
	for ( i=0; i<rangecnt; ++i ) {
	    start = getushort(ttf);
	    end = getushort(ttf);
	    if ( start>end || end>=cnt )
		fprintf( stderr, "Bad class def table. Glyph range %d-%d out of range [0,%d)\n", start, end, cnt );
	    class = getushort(ttf);
	    for ( j=start; j<=end; ++j )
		glist[j] = class;
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

static void addPairPos(struct ttfinfo *info, int glyph1, int glyph2,
	struct lookup *lookup,struct valuerecord *vr1,struct valuerecord *vr2) {
    
    if ( glyph1<info->glyph_cnt && glyph2<info->glyph_cnt ) {
	PST *pos = chunkalloc(sizeof(PST));
	pos->type = pst_pair;
	pos->tag = lookup->tag;
	pos->script_lang_index = lookup->script_lang_index;
	pos->flags = lookup->flags;
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
    } else
	fprintf( stderr, "Bad pair position: glyphs %d & %d should have been < %d\n",
		glyph1, glyph2, info->glyph_cnt );
}

static int addKernPair(struct ttfinfo *info, int glyph1, int glyph2,
	int16 offset, uint16 sli, uint16 flags,int isv) {
    KernPair *kp;
    if ( glyph1<info->glyph_cnt && glyph2<info->glyph_cnt ) {
	for ( kp=isv ? info->chars[glyph1]->vkerns : info->chars[glyph1]->kerns;
		kp!=NULL; kp=kp->next ) {
	    if ( kp->sc == info->chars[glyph2] )
	break;
	}
	if ( kp==NULL ) {
	    kp = chunkalloc(sizeof(KernPair));
	    kp->sc = info->chars[glyph2];
	    kp->off = offset;
	    kp->sli = sli;
	    kp->flags = flags;
	    if ( isv ) {
		kp->next = info->chars[glyph1]->vkerns;
		info->chars[glyph1]->vkerns = kp;
	    } else {
		kp->next = info->chars[glyph1]->kerns;
		info->chars[glyph1]->kerns = kp;
	    }
	} else if ( kp->sli!=sli || kp->flags!=flags )
return( true );
    } else
	fprintf( stderr, "Bad kern pair: glyphs %d & %d should have been < %d\n",
		glyph1, glyph2, info->glyph_cnt );
return( false );
}

static void gposKernSubTable(FILE *ttf, int stoffset, struct ttfinfo *info, struct lookup *lookup,int isv) {
    int coverage, cnt, i, j, pair_cnt, vf1, vf2, glyph2;
    int cd1, cd2, c1_cnt, c2_cnt;
    uint16 format;
    uint16 *ps_offsets;
    uint16 *glyphs, *class1, *class2;
    struct valuerecord vr1, vr2;
    long foffset;
    KernClass *kc;

    format=getushort(ttf);
    if ( format!=1 && format!=2 )	/* Unknown subtable format */
return;
    coverage = getushort(ttf);
    vf1 = getushort(ttf);
    vf2 = getushort(ttf);
    if ( isv==1 ) {
	if ( vf1&0xff77 )
	    isv = 2;
	if ( vf2&0xff77 )
	    isv = 2;
    } else if ( isv==0 ) {
	if ( vf1&0xffbb)	/* can't represent things that deal with y advance/placement nor with x placement as kerning */
	    isv = 2;
	if ( vf2&0xffaa )
	    isv = 2;
    }
    if ( format==1 ) {
	cnt = getushort(ttf);
	ps_offsets = galloc(cnt*sizeof(uint16));
	for ( i=0; i<cnt; ++i )
	    ps_offsets[i]=getushort(ttf);
	glyphs = getCoverageTable(ttf,stoffset+coverage,info);
	if ( glyphs==NULL )
return;
	for ( i=0; i<cnt; ++i ) if ( glyphs[i]<info->glyph_cnt ) {
	    fseek(ttf,stoffset+ps_offsets[i],SEEK_SET);
	    pair_cnt = getushort(ttf);
	    for ( j=0; j<pair_cnt; ++j ) {
		glyph2 = getushort(ttf);
		readvaluerecord(&vr1,vf1,ttf);
		readvaluerecord(&vr2,vf2,ttf);
		if ( isv==2 )
		    addPairPos(info, glyphs[i], glyph2,lookup,&vr1,&vr2);
		else if ( isv ) {
		    if ( addKernPair(info, glyphs[i], glyph2, vr1.yadvance,lookup->script_lang_index,lookup->flags,isv))
			addPairPos(info, glyphs[i], glyph2,lookup,&vr1,&vr2);
			/* If we've already got kern data for this pair of */
			/*  glyphs, then we can't make it be a true KernPair */
			/*  but we can save the info as a pst_pair */
		} else if ( lookup->flags&1 ) {	/* R2L */
		    if ( addKernPair(info, glyphs[i], glyph2, vr2.xadvance,lookup->script_lang_index,lookup->flags,isv))
			addPairPos(info, glyphs[i], glyph2,lookup,&vr1,&vr2);
		} else {
		    if ( addKernPair(info, glyphs[i], glyph2, vr1.xadvance,lookup->script_lang_index,lookup->flags,isv))
			addPairPos(info, glyphs[i], glyph2,lookup,&vr1,&vr2);
		}
	    }
	}
	free(ps_offsets); free(glyphs);
    } else if ( format==2 ) {	/* Class-based kerning */
	cd1 = getushort(ttf);
	cd2 = getushort(ttf);
	foffset = ftell(ttf);
	class1 = getClassDefTable(ttf, stoffset+cd1, info->glyph_cnt);
	class2 = getClassDefTable(ttf, stoffset+cd2, info->glyph_cnt);
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
	    kc->first_cnt = c1_cnt; kc->second_cnt = c2_cnt;
	    kc->sli = lookup->script_lang_index;
	    kc->flags = lookup->flags;
	    kc->offsets = galloc(c1_cnt*c2_cnt*sizeof(int16));
	    kc->firsts = ClassToNames(info,c1_cnt,class1,info->glyph_cnt);
	    kc->seconds = ClassToNames(info,c2_cnt,class2,info->glyph_cnt);
	    for ( i=0; i<c1_cnt; ++i) {
		for ( j=0; j<c2_cnt; ++j) {
		    readvaluerecord(&vr1,vf1,ttf);
		    readvaluerecord(&vr2,vf2,ttf);
		    if ( isv )
			kc->offsets[i*c2_cnt+j] = vr1.yadvance;
		    else if ( lookup->flags&1 )	/* R2L */
			kc->offsets[i*c2_cnt+j] = vr2.xadvance;
		    else
			kc->offsets[i*c2_cnt+j] = vr1.xadvance;
		}
	    }
	} else {
	    int k,l;
	    for ( i=0; i<c1_cnt; ++i) {
		for ( j=0; j<c2_cnt; ++j) {
		    readvaluerecord(&vr1,vf1,ttf);
		    readvaluerecord(&vr2,vf2,ttf);
		    if ( vr1.xadvance!=0 || vr1.xplacement!=0 || vr1.yadvance!=0 || vr1.yplacement!=0 ||
			    vr2.xadvance!=0 || vr2.xplacement!=0 || vr2.yadvance!=0 || vr2.yplacement!=0 )
			for ( k=0; k<info->glyph_cnt; ++k )
			    if ( class1[k]==i )
				for ( l=0; l<info->glyph_cnt; ++l )
				    if ( class2[l]==j )
					addPairPos(info, k,l,lookup,&vr1,&vr2);
		}
	    }
	}
	free(class1); free(class2);
    }
}

static void gposCursiveSubTable(FILE *ttf, int stoffset, struct ttfinfo *info,struct lookup *lookup) {
    int coverage, cnt, format, i;
    struct ee_offsets { int entry, exit; } *offsets;
    uint16 *glyphs;
    AnchorClass *class;
    AnchorPoint *ap;
    SplineChar *sc;

    format=getushort(ttf);
    if ( format!=1 )	/* Unknown subtable format */
return;
    coverage = getushort(ttf);
    cnt = getushort(ttf);
    if ( cnt==0 )
return;
    offsets = galloc(cnt*sizeof(struct ee_offsets));
    for ( i=0; i<cnt; ++i ) {
	offsets[i].entry = getushort(ttf);
	offsets[i].exit  = getushort(ttf);
    }
    glyphs = getCoverageTable(ttf,stoffset+coverage,info);

    class = chunkalloc(sizeof(AnchorClass));
    class->name = uc_copy("Cursive");
    class->feature_tag = lookup->tag;
    class->script_lang_index = lookup->script_lang_index;
    class->type = act_curs;
    if ( info->ahead==NULL )
	info->ahead = class;
    else
	info->alast->next = class;
    info->alast = class;

    for ( i=0; i<cnt; ++i ) {
	sc = info->chars[glyphs[i]];
	if ( offsets[i].entry!=0 ) {
	    ap = chunkalloc(sizeof(AnchorPoint));
	    ap->anchor = class;
	    fseek(ttf,stoffset+offsets[i].entry,SEEK_SET);
	    /* All anchor types have the same initial 3 entries, and I only care */
	    /*  about two of them, so I can ignore all the complexities of the */
	    /*  format type */
	    /* format = */ getushort(ttf);
	    ap->me.x = (int16) getushort(ttf);
	    ap->me.y = (int16) getushort(ttf);
	    ap->type = at_centry;
	    ap->next = sc->anchor;
	    sc->anchor = ap;
	}
	if ( offsets[i].exit!=0 ) {
	    ap = chunkalloc(sizeof(AnchorPoint));
	    ap->anchor = class;
	    fseek(ttf,stoffset+offsets[i].exit,SEEK_SET);
	    /* format = */ getushort(ttf);
	    ap->me.x = (int16) getushort(ttf);
	    ap->me.y = (int16) getushort(ttf);
	    ap->type = at_cexit;
	    ap->next = sc->anchor;
	    sc->anchor = ap;
	}
    }
    free(offsets);
    free(glyphs);
}

static AnchorClass **MarkGlyphsProcessMarks(FILE *ttf,int markoffset,
	struct ttfinfo *info,struct lookup *lookup,uint16 *markglyphs,
	int classcnt,int lu_type) {
    AnchorClass **classes = gcalloc(classcnt,sizeof(AnchorClass *)), *ac;
    unichar_t ubuf[50];
    int i, cnt;
    struct mr { uint16 class, offset; } *at_offsets;
    AnchorPoint *ap;
    SplineChar *sc;

    for ( i=0; i<classcnt; ++i ) {
	u_snprintf(ubuf,sizeof(ubuf)/sizeof(ubuf[0]),GStringGetResource(_STR_UntitledAnchor_n,NULL),
		info->anchor_class_cnt+i );
	classes[i] = ac = chunkalloc(sizeof(AnchorClass));
	ac->name = u_copy(ubuf);
	ac->feature_tag = lookup->tag;
	ac->script_lang_index = lookup->script_lang_index;
	ac->flags = lookup->flags;
	ac->merge_with = info->anchor_merge_cnt+1;
	ac->type = lu_type==6 ? act_mkmk : act_mark;
	    /* I don't distinguish between mark to base and mark to lig */
	if ( info->ahead==NULL )
	    info->ahead = ac;
	else
	    info->alast->next = ac;
	info->alast = ac;
    }

    fseek(ttf,markoffset,SEEK_SET);
    cnt = getushort(ttf);
    at_offsets = galloc(cnt*sizeof(struct mr));
    for ( i=0; i<cnt; ++i ) {
	at_offsets[i].class = getushort(ttf);
	at_offsets[i].offset = getushort(ttf);
	if ( at_offsets[i].class>=classcnt ) {
	    at_offsets[i].class = 0;
	    fprintf( stderr, "Class out of bounds in GPOS mark sub-table\n" );
	}
    }
    for ( i=0; i<cnt; ++i ) {
	if ( markglyphs[i]>=info->glyph_cnt )
    continue;
	sc = info->chars[markglyphs[i]];
	if ( sc==NULL || at_offsets[i].offset==0 )
    continue;
	ap = chunkalloc(sizeof(AnchorPoint));
	ap->anchor = classes[at_offsets[i].class];
	fseek(ttf,markoffset+at_offsets[i].offset,SEEK_SET);
	/* All anchor types have the same initial 3 entries, and I only care */
	/*  about two of them, so I can ignore all the complexities of the */
	/*  format type */
	/* format = */ getushort(ttf);
	ap->me.x = (int16) getushort(ttf);
	ap->me.y = (int16) getushort(ttf);
	ap->type = at_mark;
	ap->next = sc->anchor;
	sc->anchor = ap;
    }
    free(at_offsets);
return( classes );
}

static void MarkGlyphsProcessBases(FILE *ttf,int baseoffset,
	struct ttfinfo *info,struct lookup *lookup,uint16 *baseglyphs,int classcnt,
	AnchorClass **classes,enum anchor_type at) {
    int basecnt,i, j, ibase;
    uint16 *offsets;
    SplineChar *sc;
    AnchorPoint *ap;

    fseek(ttf,baseoffset,SEEK_SET);
    basecnt = getushort(ttf);
    offsets = galloc(basecnt*classcnt*sizeof(uint16));
    for ( i=0; i<basecnt*classcnt; ++i )
	offsets[i] = getushort(ttf);
    for ( i=ibase=0; i<basecnt; ++i, ibase+= classcnt ) {
	if ( baseglyphs[i]>=info->glyph_cnt )
    continue;
	sc = info->chars[baseglyphs[i]];
	if ( sc==NULL )
    continue;
	for ( j=0; j<classcnt; ++j ) if ( offsets[ibase+j]!=0 ) {
	    fseek(ttf,baseoffset+offsets[ibase+j],SEEK_SET);
	    ap = chunkalloc(sizeof(AnchorPoint));
	    ap->anchor = classes[j];
	    /* All anchor types have the same initial 3 entries, and I only care */
	    /*  about two of them, so I can ignore all the complexities of the */
	    /*  format type */
	    /* format = */ getushort(ttf);
	    ap->me.x = (int16) getushort(ttf);
	    ap->me.y = (int16) getushort(ttf);
	    ap->type = at;
	    ap->next = sc->anchor;
	    sc->anchor = ap;
	}
    }
}

static void MarkGlyphsProcessLigs(FILE *ttf,int baseoffset,
	struct ttfinfo *info,struct lookup *lookup,uint16 *baseglyphs,int classcnt,
	AnchorClass **classes) {
    int basecnt,compcnt, i, j, k, kbase;
    uint16 *loffsets, *aoffsets;
    SplineChar *sc;
    AnchorPoint *ap;

    fseek(ttf,baseoffset,SEEK_SET);
    basecnt = getushort(ttf);
    loffsets = galloc(basecnt*sizeof(uint16));
    for ( i=0; i<basecnt; ++i )
	loffsets[i] = getushort(ttf);
    for ( i=0; i<basecnt; ++i ) {
	sc = info->chars[baseglyphs[i]];
	if ( baseglyphs[i]>=info->glyph_cnt || sc==NULL )
    continue;
	fseek(ttf,baseoffset+loffsets[i],SEEK_SET);
	compcnt = getushort(ttf);
	aoffsets = galloc(compcnt*classcnt*sizeof(uint16));
	for ( k=0; k<compcnt*classcnt; ++k )
	    aoffsets[k] = getushort(ttf);
	for ( k=kbase=0; k<compcnt; ++k, kbase+=classcnt ) {
	    for ( j=0; j<classcnt; ++j ) if ( aoffsets[kbase+j]!=0 ) {
		fseek(ttf,baseoffset+loffsets[i]+aoffsets[kbase+j],SEEK_SET);
		ap = chunkalloc(sizeof(AnchorPoint));
		ap->anchor = classes[j];
		/* All anchor types have the same initial 3 entries, and I only care */
		/*  about two of them, so I can ignore all the complexities of the */
		/*  format type */
		/* format = */ getushort(ttf);
		ap->me.x = (int16) getushort(ttf);
		ap->me.y = (int16) getushort(ttf);
		ap->type = at_baselig;
		ap->lig_index = k;
		ap->next = sc->anchor;
		sc->anchor = ap;
	    }
	}
    }
}

static void gposMarkSubTable(FILE *ttf, uint32 stoffset,
	struct ttfinfo *info, struct lookup *lookup,int lu_type) {
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
return;
    }
	/* as is the (first) mark table */
    classes = MarkGlyphsProcessMarks(ttf,stoffset+markoffset,
	    info,lookup,markglyphs,classcnt,lu_type);
    switch ( lu_type ) {
      case 4:			/* Mark to Base */
      case 6:			/* Mark to Mark */
	  MarkGlyphsProcessBases(ttf,stoffset+baseoffset,
	    info,lookup,baseglyphs,classcnt,classes,
	    lu_type==4?at_basechar:at_basemark);
      break;
      case 5:			/* Mark to Ligature */
	  MarkGlyphsProcessLigs(ttf,stoffset+baseoffset,
	    info,lookup,baseglyphs,classcnt,classes);
      break;
    }
    info->anchor_class_cnt += classcnt;
    ++ info->anchor_merge_cnt;
    free(markglyphs); free(baseglyphs);
    free(classes);
}

static void gposSimplePos(FILE *ttf, int stoffset, struct ttfinfo *info, struct lookup *lookup) {
    int coverage, cnt, i, vf;
    uint16 format;
    uint16 *glyphs;
    struct valuerecord *vr=NULL, _vr, *which;

    format=getushort(ttf);
    if ( format!=1 && format!=2 )	/* Unknown subtable format */
return;
    coverage = getushort(ttf);
    vf = getushort(ttf);
    if ( (vf&0xf)==0 )	/* Not interested in things whose data lives in device tables */
return;
    if ( format==1 ) {
	memset(&_vr,0,sizeof(_vr));
	readvaluerecord(&_vr,vf,ttf);
    } else {
	cnt = getushort(ttf);
	vr = gcalloc(cnt,sizeof(struct valuerecord));
	for ( i=0; i<cnt; ++i )
	    readvaluerecord(&vr[i],vf,ttf);
    }
    glyphs = getCoverageTable(ttf,stoffset+coverage,info);
    if ( glyphs==NULL ) {
	free(vr);
return;
    }
    for ( i=0; glyphs[i]!=0xffff; ++i ) if ( glyphs[i]<info->glyph_cnt ) {
	PST *pos = chunkalloc(sizeof(PST));
	pos->type = pst_position;
	pos->tag = lookup->tag;
	pos->script_lang_index = lookup->script_lang_index;
	pos->flags = lookup->flags;
	pos->next = info->chars[glyphs[i]]->possub;
	info->chars[glyphs[i]]->possub = pos;
	which = format==1 ? &_vr : &vr[i];
	pos->u.pos.xoff = which->xplacement;
	pos->u.pos.yoff = which->yplacement;
	pos->u.pos.h_adv_off = which->xadvance;
	pos->u.pos.v_adv_off = which->yadvance;
    }
    free(vr);
    free(glyphs);
}

static void ProcessGPOSGSUBlookup(FILE *ttf,struct ttfinfo *info,int gpos,
	struct lookup *lookup, int inusetype, struct lookup *alllooks);

static void ProcessSubLookups(FILE *ttf,struct ttfinfo *info,int gpos,
	struct lookup *alllooks,struct seqlookup *sl) {
    int i;

    i = sl->lookup_tag;
    if ( alllooks[i].subtag==0 ) {
	alllooks[i].make_subtag = true;
	ProcessGPOSGSUBlookup(ttf,info,gpos,&alllooks[i],git_normal,alllooks);
	alllooks[i].make_subtag = false;
    }
    sl->lookup_tag = alllooks[i].subtag;
}

static void g___ContextSubTable1(FILE *ttf, int stoffset,
	struct ttfinfo *info, struct lookup *lookup, int justinuse,
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
    int warned = false;

    coverage = getushort(ttf);
    rcnt = getushort(ttf);		/* glyph count in coverage table */
    rules = galloc(rcnt*sizeof(struct rule));
    for ( i=0; i<rcnt; ++i )
	rules[i].offsets = getushort(ttf)+stoffset;
    glyphs = getCoverageTable(ttf,stoffset+coverage,info);
    cnt = 0;
    for ( i=0; i<rcnt; ++i ) {
	fseek(ttf,rules[i].offsets,SEEK_SET);
	rules[i].scnt = getushort(ttf);
	cnt += rules[i].scnt;
	rules[i].subrules = galloc(rules[i].scnt*sizeof(struct subrule));
	for ( j=0; j<rules[i].scnt; ++j )
	    rules[i].subrules[j].offset = getushort(ttf)+rules[i].offsets;
	for ( j=0; j<rules[i].scnt; ++j ) {
	    fseek(ttf,rules[i].subrules[j].offset,SEEK_SET);
	    rules[i].subrules[j].gcnt = getushort(ttf);
	    rules[i].subrules[j].glyphs = galloc((rules[i].subrules[j].gcnt+1)*sizeof(uint16));
	    rules[i].subrules[j].glyphs[0] = glyphs[i];
	    for ( k=1; k<rules[i].subrules[j].gcnt; ++k ) {
		rules[i].subrules[j].glyphs[k] = getushort(ttf);
		if ( rules[i].subrules[j].glyphs[k]>=info->glyph_cnt ) {
		    if ( !warned )
			fprintf( stderr, "Bad contextual or chaining sub table. Glyph %d out of range [0,%d)\n",
				 rules[i].subrules[j].glyphs[k], info->glyph_cnt );
		    warned = true;
		     rules[i].subrules[j].glyphs[k] = 0;
		 }
	    }
	    rules[i].subrules[j].glyphs[k] = 0xffff;
	    rules[i].subrules[j].scnt = getushort(ttf);
	    rules[i].subrules[j].sl = galloc(rules[i].subrules[j].scnt*sizeof(struct seqlookup));
	    for ( k=0; k<rules[i].subrules[j].scnt; ++k ) {
		rules[i].subrules[j].sl[k].seq = getushort(ttf);
		rules[i].subrules[j].sl[k].lookup_tag = getushort(ttf);
	    }
	}
    }

    if ( justinuse==git_justinuse ) {
	for ( i=0; i<rcnt; ++i ) {
	    for ( j=0; j<rules[i].scnt; ++j ) {
		for ( k=0; k<rules[i].subrules[j].scnt; ++k )
		    ProcessGPOSGSUBlookup(ttf,info,gpos,
			    &alllooks[rules[i].subrules[j].sl[k].lookup_tag],
			    justinuse,alllooks);
	    }
	}
    } else {
	fpst = chunkalloc(sizeof(FPST));
	fpst->type = gpos ? pst_contextpos : pst_contextsub;
	fpst->format = pst_glyphs;
	fpst->tag = lookup->tag;
	fpst->script_lang_index = lookup->script_lang_index;
	fpst->flags = lookup->flags;
	fpst->next = info->possub;
	info->possub = fpst;

	fpst->rules = rule = gcalloc(cnt,sizeof(struct fpst_rule));
	fpst->rule_cnt = cnt;

	cnt = 0;
	for ( i=0; i<rcnt; ++i ) for ( j=0; j<rules[i].scnt; ++j ) {
	    rule[cnt].u.glyph.names = GlyphsToNames(info,rules[i].subrules[j].glyphs);
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
	struct ttfinfo *info, struct lookup *lookup, int justinuse,
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
    int warned = false;

    coverage = getushort(ttf);
    rcnt = getushort(ttf);		/* glyph count in coverage table */
    rules = galloc(rcnt*sizeof(struct rule));
    for ( i=0; i<rcnt; ++i )
	rules[i].offsets = getushort(ttf)+stoffset;
    glyphs = getCoverageTable(ttf,stoffset+coverage,info);
    cnt = 0;
    for ( i=0; i<rcnt; ++i ) {
	fseek(ttf,rules[i].offsets,SEEK_SET);
	rules[i].scnt = getushort(ttf);
	cnt += rules[i].scnt;
	rules[i].subrules = galloc(rules[i].scnt*sizeof(struct subrule));
	for ( j=0; j<rules[i].scnt; ++j )
	    rules[i].subrules[j].offset = getushort(ttf)+rules[i].offsets;
	for ( j=0; j<rules[i].scnt; ++j ) {
	    fseek(ttf,rules[i].subrules[j].offset,SEEK_SET);
	    rules[i].subrules[j].bcnt = getushort(ttf);
	    rules[i].subrules[j].bglyphs = galloc((rules[i].subrules[j].bcnt+1)*sizeof(uint16));
	    for ( k=0; k<rules[i].subrules[j].bcnt; ++k )
		rules[i].subrules[j].bglyphs[k] = getushort(ttf);
	    rules[i].subrules[j].bglyphs[k] = 0xffff;

	    rules[i].subrules[j].gcnt = getushort(ttf);
	    rules[i].subrules[j].glyphs = galloc((rules[i].subrules[j].gcnt+1)*sizeof(uint16));
	    rules[i].subrules[j].glyphs[0] = glyphs[i];
	    for ( k=1; k<rules[i].subrules[j].gcnt; ++k )
		rules[i].subrules[j].glyphs[k] = getushort(ttf);
	    rules[i].subrules[j].glyphs[k] = 0xffff;

	    rules[i].subrules[j].fcnt = getushort(ttf);
	    rules[i].subrules[j].fglyphs = galloc((rules[i].subrules[j].fcnt+1)*sizeof(uint16));
	    for ( k=0; k<rules[i].subrules[j].fcnt; ++k )
		rules[i].subrules[j].fglyphs[k] = getushort(ttf);
	    rules[i].subrules[j].fglyphs[k] = 0xffff;

	    for ( which = 0; which<3; ++which ) {
		for ( k=0; k<(&rules[i].subrules[j].gcnt)[which]; ++k ) {
		    if ( (&rules[i].subrules[j].glyphs)[which][k]>=info->glyph_cnt ) {
			if ( !warned )
			    fprintf( stderr, "Bad contextual or chaining sub table. Glyph %d out of range [0,%d)\n",
				    (&rules[i].subrules[j].glyphs)[which][k], info->glyph_cnt );
			warned = true;
			(&rules[i].subrules[j].glyphs)[which][k] = 0;
		    }
		}
	    }

	    rules[i].subrules[j].scnt = getushort(ttf);
	    rules[i].subrules[j].sl = galloc(rules[i].subrules[j].scnt*sizeof(struct seqlookup));
	    for ( k=0; k<rules[i].subrules[j].scnt; ++k ) {
		rules[i].subrules[j].sl[k].seq = getushort(ttf);
		rules[i].subrules[j].sl[k].lookup_tag = getushort(ttf);
	    }
	}
    }

    if ( justinuse==git_justinuse ) {
	for ( i=0; i<rcnt; ++i ) {
	    for ( j=0; j<rules[i].scnt; ++j ) {
		for ( k=0; k<rules[i].subrules[j].scnt; ++k )
		    ProcessGPOSGSUBlookup(ttf,info,gpos,
			    &alllooks[rules[i].subrules[j].sl[k].lookup_tag],
			    justinuse,alllooks);
	    }
	}
    } else {
	fpst = chunkalloc(sizeof(FPST));
	fpst->type = gpos ? pst_chainpos : pst_chainsub;
	fpst->format = pst_glyphs;
	fpst->tag = lookup->tag;
	fpst->script_lang_index = lookup->script_lang_index;
	fpst->flags = lookup->flags;
	fpst->next = info->possub;
	info->possub = fpst;

	fpst->rules = rule = gcalloc(cnt,sizeof(struct fpst_rule));
	fpst->rule_cnt = cnt;

	cnt = 0;
	for ( i=0; i<rcnt; ++i ) for ( j=0; j<rules[i].scnt; ++j ) {
	    rule[cnt].u.glyph.back = GlyphsToNames(info,rules[i].subrules[j].bglyphs);
	    rule[cnt].u.glyph.names = GlyphsToNames(info,rules[i].subrules[j].glyphs);
	    rule[cnt].u.glyph.fore = GlyphsToNames(info,rules[i].subrules[j].fglyphs);
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
	struct ttfinfo *info, struct lookup *lookup, int justinuse,
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
    uint16 *class;

    coverage = getushort(ttf);
    classoff = getushort(ttf);
    rcnt = getushort(ttf);		/* class count in coverage table *//* == number of top level rules */
    rules = gcalloc(rcnt,sizeof(struct rule));
    for ( i=0; i<rcnt; ++i )
	rules[i].offsets = getushort(ttf)+stoffset;
    cnt = 0;
    for ( i=0; i<rcnt; ++i ) if ( rules[i].offsets!=stoffset ) { /* some classes might be unused */
	fseek(ttf,rules[i].offsets,SEEK_SET);
	rules[i].scnt = getushort(ttf);
	cnt += rules[i].scnt;
	rules[i].subrules = galloc(rules[i].scnt*sizeof(struct subrule));
	for ( j=0; j<rules[i].scnt; ++j )
	    rules[i].subrules[j].offset = getushort(ttf)+rules[i].offsets;
	for ( j=0; j<rules[i].scnt; ++j ) {
	    fseek(ttf,rules[i].subrules[j].offset,SEEK_SET);
	    rules[i].subrules[j].ccnt = getushort(ttf);
	    rules[i].subrules[j].classindeces = galloc(rules[i].subrules[j].ccnt*sizeof(uint16));
	    rules[i].subrules[j].classindeces[0] = i;
	    for ( k=1; k<rules[i].subrules[j].ccnt; ++k )
		rules[i].subrules[j].classindeces[k] = getushort(ttf);
	    rules[i].subrules[j].scnt = getushort(ttf);
	    rules[i].subrules[j].sl = galloc(rules[i].subrules[j].scnt*sizeof(struct seqlookup));
	    for ( k=0; k<rules[i].subrules[j].scnt; ++k ) {
		rules[i].subrules[j].sl[k].seq = getushort(ttf);
		rules[i].subrules[j].sl[k].lookup_tag = getushort(ttf);
	    }
	}
    }

    if ( justinuse==git_justinuse ) {
	for ( i=0; i<rcnt; ++i ) {
	    for ( j=0; j<rules[i].scnt; ++j ) {
		for ( k=0; k<rules[i].subrules[j].scnt; ++k )
		    ProcessGPOSGSUBlookup(ttf,info,gpos,
			    &alllooks[rules[i].subrules[j].sl[k].lookup_tag],
			    justinuse,alllooks);
	    }
	}
    } else {
	fpst = chunkalloc(sizeof(FPST));
	fpst->type = gpos ? pst_contextpos : pst_contextsub;
	fpst->format = pst_class;
	fpst->tag = lookup->tag;
	fpst->script_lang_index = lookup->script_lang_index;
	fpst->flags = lookup->flags;
	fpst->next = info->possub;
	info->possub = fpst;

	fpst->rules = rule = gcalloc(cnt,sizeof(struct fpst_rule));
	fpst->rule_cnt = cnt;
	class = getClassDefTable(ttf, stoffset+classoff, info->glyph_cnt);
	fpst->nccnt = ClassFindCnt(class,info->glyph_cnt);
	fpst->nclass = ClassToNames(info,fpst->nccnt,class,info->glyph_cnt);

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
	struct ttfinfo *info, struct lookup *lookup, int justinuse,
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
    uint16 *class;

    coverage = getushort(ttf);
    bclassoff = getushort(ttf);
    classoff = getushort(ttf);
    fclassoff = getushort(ttf);
    rcnt = getushort(ttf);		/* class count *//* == max number of top level rules */
    rules = gcalloc(rcnt,sizeof(struct rule));
    for ( i=0; i<rcnt; ++i ) {
	offset = getushort(ttf);
	rules[i].offsets = offset==0 ? 0 : offset+stoffset;
    }
    cnt = 0;
    for ( i=0; i<rcnt; ++i ) if ( rules[i].offsets!=0 ) { /* some classes might be unused */
	fseek(ttf,rules[i].offsets,SEEK_SET);
	rules[i].scnt = getushort(ttf);
	cnt += rules[i].scnt;
	rules[i].subrules = galloc(rules[i].scnt*sizeof(struct subrule));
	for ( j=0; j<rules[i].scnt; ++j )
	    rules[i].subrules[j].offset = getushort(ttf)+rules[i].offsets;
	for ( j=0; j<rules[i].scnt; ++j ) {
	    fseek(ttf,rules[i].subrules[j].offset,SEEK_SET);
	    rules[i].subrules[j].bccnt = getushort(ttf);
	    rules[i].subrules[j].bci = galloc(rules[i].subrules[j].bccnt*sizeof(uint16));
	    for ( k=0; k<rules[i].subrules[j].bccnt; ++k )
		rules[i].subrules[j].bci[k] = getushort(ttf);
	    rules[i].subrules[j].ccnt = getushort(ttf);
	    rules[i].subrules[j].classindeces = galloc(rules[i].subrules[j].ccnt*sizeof(uint16));
	    rules[i].subrules[j].classindeces[0] = i;
	    for ( k=1; k<rules[i].subrules[j].ccnt; ++k )
		rules[i].subrules[j].classindeces[k] = getushort(ttf);
	    rules[i].subrules[j].fccnt = getushort(ttf);
	    rules[i].subrules[j].fci = galloc(rules[i].subrules[j].fccnt*sizeof(uint16));
	    for ( k=0; k<rules[i].subrules[j].fccnt; ++k )
		rules[i].subrules[j].fci[k] = getushort(ttf);
	    rules[i].subrules[j].scnt = getushort(ttf);
	    rules[i].subrules[j].sl = galloc(rules[i].subrules[j].scnt*sizeof(struct seqlookup));
	    for ( k=0; k<rules[i].subrules[j].scnt; ++k ) {
		rules[i].subrules[j].sl[k].seq = getushort(ttf);
		rules[i].subrules[j].sl[k].lookup_tag = getushort(ttf);
	    }
	}
    }

    if ( justinuse==git_justinuse ) {
	for ( i=0; i<rcnt; ++i ) {
	    for ( j=0; j<rules[i].scnt; ++j ) {
		for ( k=0; k<rules[i].subrules[j].scnt; ++k )
		    ProcessGPOSGSUBlookup(ttf,info,gpos,
			    &alllooks[rules[i].subrules[j].sl[k].lookup_tag],
			    justinuse,alllooks);
	    }
	}
    } else {
	fpst = chunkalloc(sizeof(FPST));
	fpst->type = gpos ? pst_chainpos : pst_chainsub;
	fpst->format = pst_class;
	fpst->tag = lookup->tag;
	fpst->script_lang_index = lookup->script_lang_index;
	fpst->flags = lookup->flags;
	fpst->next = info->possub;
	info->possub = fpst;

	fpst->rules = rule = gcalloc(cnt,sizeof(struct fpst_rule));
	fpst->rule_cnt = cnt;

	class = getClassDefTable(ttf, stoffset+classoff, info->glyph_cnt);
	fpst->nccnt = ClassFindCnt(class,info->glyph_cnt);
	fpst->nclass = ClassToNames(info,fpst->nccnt,class,info->glyph_cnt);
	free(class);
	class = getClassDefTable(ttf, stoffset+bclassoff, info->glyph_cnt);
	fpst->bccnt = ClassFindCnt(class,info->glyph_cnt);
	fpst->bclass = ClassToNames(info,fpst->bccnt,class,info->glyph_cnt);
	free(class);
	class = getClassDefTable(ttf, stoffset+fclassoff, info->glyph_cnt);
	fpst->fccnt = ClassFindCnt(class,info->glyph_cnt);
	fpst->fclass = ClassToNames(info,fpst->fccnt,class,info->glyph_cnt);
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
	struct ttfinfo *info, struct lookup *lookup, int justinuse,
	struct lookup *alllooks, int gpos) {
    int i, k, scnt, gcnt;
    uint16 *coverage;
    struct seqlookup *sl;
    uint16 *glyphs;
    FPST *fpst;
    struct fpst_rule *rule;

    gcnt = getushort(ttf);
    coverage = galloc(gcnt*sizeof(uint16));
    for ( i=0; i<gcnt; ++i )
	coverage[i] = getushort(ttf);
    scnt = getushort(ttf);
    sl = galloc(scnt*sizeof(struct seqlookup));
    for ( k=0; k<scnt; ++k ) {
	sl[k].seq = getushort(ttf);
	sl[k].lookup_tag = getushort(ttf);
    }

    if ( justinuse==git_justinuse ) {
	for ( k=0; k<scnt; ++k )
	    ProcessGPOSGSUBlookup(ttf,info,gpos,
		    &alllooks[sl[k].lookup_tag],
		    justinuse,alllooks);
    } else {
	fpst = chunkalloc(sizeof(FPST));
	fpst->type = gpos ? pst_contextpos : pst_contextsub;
	fpst->format = pst_coverage;
	fpst->tag = lookup->tag;
	fpst->script_lang_index = lookup->script_lang_index;
	fpst->flags = lookup->flags;
	fpst->next = info->possub;
	info->possub = fpst;

	fpst->rules = rule = gcalloc(1,sizeof(struct fpst_rule));
	fpst->rule_cnt = 1;
	rule->u.coverage.ncnt = gcnt;
	rule->u.coverage.ncovers = galloc(gcnt*sizeof(char **));
	for ( i=0; i<gcnt; ++i ) {
	    glyphs =  getCoverageTable(ttf,stoffset+coverage[i],info);
	    rule->u.coverage.ncovers[i] = GlyphsToNames(info,glyphs);
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
	struct ttfinfo *info, struct lookup *lookup, int justinuse,
	struct lookup *alllooks, int gpos) {
    int i, k, scnt, gcnt, bcnt, fcnt;
    uint16 *coverage, *bcoverage, *fcoverage;
    struct seqlookup *sl;
    uint16 *glyphs;
    FPST *fpst;
    struct fpst_rule *rule;

    bcnt = getushort(ttf);
    bcoverage = galloc(bcnt*sizeof(uint16));
    for ( i=0; i<bcnt; ++i )
	bcoverage[i] = getushort(ttf);
    gcnt = getushort(ttf);
    coverage = galloc(gcnt*sizeof(uint16));
    for ( i=0; i<gcnt; ++i )
	coverage[i] = getushort(ttf);
    fcnt = getushort(ttf);
    fcoverage = galloc(fcnt*sizeof(uint16));
    for ( i=0; i<fcnt; ++i )
	fcoverage[i] = getushort(ttf);
    scnt = getushort(ttf);
    sl = galloc(scnt*sizeof(struct seqlookup));
    for ( k=0; k<scnt; ++k ) {
	sl[k].seq = getushort(ttf);
	sl[k].lookup_tag = getushort(ttf);
    }

    if ( justinuse==git_justinuse ) {
	for ( k=0; k<scnt; ++k )
	    ProcessGPOSGSUBlookup(ttf,info,gpos,
		    &alllooks[sl[k].lookup_tag],
		    justinuse,alllooks);
    } else {
	fpst = chunkalloc(sizeof(FPST));
	fpst->type = gpos ? pst_chainpos : pst_chainsub;
	fpst->format = pst_coverage;
	fpst->tag = lookup->tag;
	fpst->script_lang_index = lookup->script_lang_index;
	fpst->flags = lookup->flags;
	fpst->next = info->possub;
	info->possub = fpst;

	fpst->rules = rule = gcalloc(1,sizeof(struct fpst_rule));
	fpst->rule_cnt = 1;

	rule->u.coverage.bcnt = bcnt;
	rule->u.coverage.bcovers = galloc(bcnt*sizeof(char **));
	for ( i=0; i<bcnt; ++i ) {
	    glyphs =  getCoverageTable(ttf,stoffset+bcoverage[i],info);
	    rule->u.coverage.bcovers[i] = GlyphsToNames(info,glyphs);
	    free(glyphs);
	}

	rule->u.coverage.ncnt = gcnt;
	rule->u.coverage.ncovers = galloc(gcnt*sizeof(char **));
	for ( i=0; i<gcnt; ++i ) {
	    glyphs =  getCoverageTable(ttf,stoffset+coverage[i],info);
	    rule->u.coverage.ncovers[i] = GlyphsToNames(info,glyphs);
	    free(glyphs);
	}

	rule->u.coverage.fcnt = fcnt;
	rule->u.coverage.fcovers = galloc(fcnt*sizeof(char **));
	for ( i=0; i<fcnt; ++i ) {
	    glyphs =  getCoverageTable(ttf,stoffset+fcoverage[i],info);
	    rule->u.coverage.fcovers[i] = GlyphsToNames(info,glyphs);
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
	struct ttfinfo *info, struct lookup *lookup, 
	struct lookup *alllooks) {
    switch( getushort(ttf)) {
      case 1:
	g___ContextSubTable1(ttf,stoffset,info,lookup,git_normal,alllooks,true);
      break;
      case 2:
	g___ContextSubTable2(ttf,stoffset,info,lookup,git_normal,alllooks,true);
      break;
      case 3:
	g___ContextSubTable3(ttf,stoffset,info,lookup,git_normal,alllooks,true);
      break;
    }
}

static void gposChainingSubTable(FILE *ttf, int stoffset,
	struct ttfinfo *info, struct lookup *lookup, 
	struct lookup *alllooks) {
    switch( getushort(ttf)) {
      case 1:
	g___ChainingSubTable1(ttf,stoffset,info,lookup,git_normal,alllooks,true);
      break;
      case 2:
	g___ChainingSubTable2(ttf,stoffset,info,lookup,git_normal,alllooks,true);
      break;
      case 3:
	g___ChainingSubTable3(ttf,stoffset,info,lookup,git_normal,alllooks,true);
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
	struct lookup *lookup, int justinuse) {
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
	glyph2s = galloc(cnt*sizeof(uint16));
	for ( i=0; i<cnt; ++i )
	    glyph2s[i] = getushort(ttf);
	    /* in range check comes later */
    }
    glyphs = getCoverageTable(ttf,stoffset+coverage,info);
    if ( glyphs==NULL ) {
	free(glyph2s);
return;
    }
    if ( justinuse==git_findnames ) {
	/* Unnamed glyphs get a name built of the base name and the lookup tag */
	for ( i=0; glyphs[i]!=0xffff; ++i ) if ( glyphs[i]<info->glyph_cnt ) {
	    if ( info->chars[glyphs[i]]->name!=NULL ) {
		which = format==1 ? (uint16) (glyphs[i]+delta) : glyph2s[i];
		if ( info->chars[which]->name==NULL ) {
		    char *basename = info->chars[glyphs[i]]->name;
		    char *str = galloc(strlen(basename)+6);
		    char tag[5], *pt=tag;
		    for ( j=0; tagstr[j].tag!=0 && tagstr[j].tag!=lookup->tag; ++j );
		    if ( tagstr[j].tag!=0 )
			pt = tagstr[j].str;
		    else {
			tag[0] = lookup->tag>>24;
			if ( (tag[1] = (lookup->tag>>16)&0xff)==' ' ) tag[1] = '\0';
			if ( (tag[2] = (lookup->tag>>8)&0xff)==' ' ) tag[2] = '\0';
			if ( (tag[3] = (lookup->tag)&0xff)==' ' ) tag[3] = '\0';
			tag[4] = '\0';
			pt = tag;
		    }
		    str = galloc(strlen(basename)+strlen(pt)+2);
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
		fprintf( stderr, "Bad substitution glyph: %d not less than %d\n",
			which, info->glyph_cnt);
		which = 0;
	    }
	    if ( info->chars[which]!=NULL ) {	/* Might be in a ttc file */
		PST *pos = chunkalloc(sizeof(PST));
		pos->type = pst_substitution;
		pos->tag = lookup->tag;
		pos->script_lang_index = lookup->script_lang_index;
		pos->flags = lookup->flags;
		pos->next = info->chars[glyphs[i]]->possub;
		info->chars[glyphs[i]]->possub = pos;
		pos->u.subs.variant = copy(info->chars[which]->name);
	    }
	}
    }
    free(glyph2s);
    free(glyphs);
}

/* Multiple and alternate substitution lookups have the same format */
static void gsubMultipleSubTable(FILE *ttf, int stoffset, struct ttfinfo *info,
	struct lookup *lookup, int lu_type, int justinuse) {
    int coverage, cnt, i, j, len, max;
    uint16 format;
    uint16 *offsets;
    uint16 *glyphs, *glyph2s;
    char *pt;
    int bad;

    if ( justinuse==git_findnames )
return;

    format=getushort(ttf);
    if ( format!=1 )	/* Unknown subtable format */
return;
    coverage = getushort(ttf);
    cnt = getushort(ttf);
    offsets = galloc(cnt*sizeof(uint16));
    for ( i=0; i<cnt; ++i )
	offsets[i] = getushort(ttf);
    glyphs = getCoverageTable(ttf,stoffset+coverage,info);
    if ( glyphs==NULL ) {
	free(offsets);
return;
    }
    max = 20;
    glyph2s = galloc(max*sizeof(uint16));
    for ( i=0; glyphs[i]!=0xffff; ++i ) {
	PST *sub;
	fseek(ttf,stoffset+offsets[i],SEEK_SET);
	cnt = getushort(ttf);
	if ( cnt>max ) {
	    max = cnt+30;
	    glyph2s = grealloc(glyph2s,max*sizeof(uint16));
	}
	len = 0; bad = false;
	for ( j=0; j<cnt; ++j ) {
	    glyph2s[j] = getushort(ttf);
	    if ( glyph2s[j]>=info->glyph_cnt ) {
		if ( !justinuse )
		    fprintf( stderr, "Bad Multiple/Alternate substitution glyph %d not less than %d\n",
			    glyph2s[j], info->glyph_cnt );
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
	    sub = chunkalloc(sizeof(PST));
	    sub->type = lu_type==2?pst_multiple:pst_alternate;
	    sub->tag = lookup->tag;
	    sub->script_lang_index = lookup->script_lang_index;
	    sub->flags = lookup->flags;
	    sub->next = info->chars[glyphs[i]]->possub;
	    info->chars[glyphs[i]]->possub = sub;
	    pt = sub->u.subs.variant = galloc(len+1);
	    *pt = '\0';
	    for ( j=0; j<cnt; ++j ) {
		strcat(pt,info->chars[glyph2s[j]]->name);
		strcat(pt," ");
	    }
	}
    }
    free(glyphs);
    free(glyph2s);
    free(offsets);
}

static void gsubLigatureSubTable(FILE *ttf, int stoffset,
	struct ttfinfo *info, struct lookup *lookup, int justinuse) {
    int coverage, cnt, i, j, k, lig_cnt, cc, len;
    uint16 *ls_offsets, *lig_offsets;
    uint16 *glyphs, *lig_glyphs, lig;
    char *pt;
    PST *liga;

    /* Format = */ getushort(ttf);
    coverage = getushort(ttf);
    cnt = getushort(ttf);
    ls_offsets = galloc(cnt*sizeof(uint16));
    for ( i=0; i<cnt; ++i )
	ls_offsets[i]=getushort(ttf);
    glyphs = getCoverageTable(ttf,stoffset+coverage,info);
    if ( glyphs==NULL )
return;
    for ( i=0; i<cnt; ++i ) {
	fseek(ttf,stoffset+ls_offsets[i],SEEK_SET);
	lig_cnt = getushort(ttf);
	lig_offsets = galloc(lig_cnt*sizeof(uint16));
	for ( j=0; j<lig_cnt; ++j )
	    lig_offsets[j] = getushort(ttf);
	for ( j=0; j<lig_cnt; ++j ) {
	    fseek(ttf,stoffset+ls_offsets[i]+lig_offsets[j],SEEK_SET);
	    lig = getushort(ttf);
	    if ( lig>=info->glyph_cnt ) {
		fprintf( stderr, "Bad ligature glyph %d not less than %d\n",
			lig, info->glyph_cnt );
		lig = 0;
	    }
	    cc = getushort(ttf);
	    if ( cc>100 ) {
		fprintf( stderr, "Unlikely count of ligature components (%d), I suspect this ligature sub-\n table is garbage, I'm giving up on it.\n", cc );
		free(glyphs); free(lig_offsets);
return;
	    }
	    lig_glyphs = galloc(cc*sizeof(uint16));
	    lig_glyphs[0] = glyphs[i];
	    for ( k=1; k<cc; ++k ) {
		lig_glyphs[k] = getushort(ttf);
		if ( lig_glyphs[k]>=info->glyph_cnt ) {
		    if ( justinuse==git_normal )
			fprintf( stderr, "Bad ligature component glyph %d not less than %d (in ligature %d)\n",
				lig_glyphs[k], info->glyph_cnt, lig );
		    lig_glyphs[k] = 0;
		}
	    }
	    if ( justinuse==git_justinuse ) {
		info->inuse[lig] = 1;
		for ( k=0; k<cc; ++k )
		    info->inuse[lig_glyphs[k]] = 1;
	    } else if ( justinuse==git_findnames ) {
		/* If our ligature glyph has no name (and its components do) */
		/*  give it a name by concatenating components with underscores */
		/*  between them, and appending the tag */
		if ( info->chars[lig]!=NULL && info->chars[lig]->name==NULL ) {
		    int len=0;
		    for ( k=0; k<cc; ++k ) {
			if ( info->chars[lig_glyphs[k]]==NULL || info->chars[lig_glyphs[k]]->name==NULL )
		    break;
			len += strlen(info->chars[lig_glyphs[k]]->name)+1;
		    }
		    if ( k==cc ) {
			char *str = galloc(len+6), *pt;
			char tag[5];
			tag[0] = lookup->tag>>24;
			if ( (tag[1] = (lookup->tag>>16)&0xff)==' ' ) tag[1] = '\0';
			if ( (tag[2] = (lookup->tag>>8)&0xff)==' ' ) tag[2] = '\0';
			if ( (tag[3] = (lookup->tag)&0xff)==' ' ) tag[3] = '\0';
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
		for ( k=len=0; k<cc; ++k )
		    len += strlen(info->chars[lig_glyphs[k]]->name)+1;
		liga = chunkalloc(sizeof(PST));
		liga->type = pst_ligature;
		liga->tag = lookup->tag;
		liga->script_lang_index = lookup->script_lang_index;
		liga->flags = lookup->flags;
		liga->next = info->chars[lig]->possub;
		info->chars[lig]->possub = liga;
		liga->u.lig.lig = info->chars[lig];
		liga->u.lig.components = pt = galloc(len);
		for ( k=0; k<cc; ++k ) {
		    strcpy(pt,info->chars[lig_glyphs[k]]->name);
		    pt += strlen(pt);
		    *pt++ = ' ';
		}
		pt[-1] = '\0';
		free(lig_glyphs);
	    }
	}
	free(lig_offsets);
    }
    free(ls_offsets); free(glyphs);
}

static void gsubContextSubTable(FILE *ttf, int stoffset,
	struct ttfinfo *info, struct lookup *lookup, int justinuse,
	struct lookup *alllooks) {
    if ( justinuse==git_findnames )
return;		/* Don't give names to these guys, they might not be unique */
	/* ie. because these are context based there is not a one to one */
	/*  mapping between input glyphs and output glyphs. One input glyph */
	/*  may go to several output glyphs (depending on context) and so */
	/*  <input-glyph-name>"."<tag-name> would be used for several glyphs */
    switch( getushort(ttf)) {
      case 1:
	g___ContextSubTable1(ttf,stoffset,info,lookup,justinuse,alllooks,false);
      break;
      case 2:
	g___ContextSubTable2(ttf,stoffset,info,lookup,justinuse,alllooks,false);
      break;
      case 3:
	g___ContextSubTable3(ttf,stoffset,info,lookup,justinuse,alllooks,false);
      break;
    }
}

static void gsubChainingSubTable(FILE *ttf, int stoffset,
	struct ttfinfo *info, struct lookup *lookup, int justinuse,
	struct lookup *alllooks) {
    if ( justinuse==git_findnames )
return;		/* Don't give names to these guys, the names might not be unique */
    switch( getushort(ttf)) {
      case 1:
	g___ChainingSubTable1(ttf,stoffset,info,lookup,justinuse,alllooks,false);
      break;
      case 2:
	g___ChainingSubTable2(ttf,stoffset,info,lookup,justinuse,alllooks,false);
      break;
      case 3:
	g___ChainingSubTable3(ttf,stoffset,info,lookup,justinuse,alllooks,false);
      break;
    }
}

static void gsubReverseChainSubTable(FILE *ttf, int stoffset,
	struct ttfinfo *info, struct lookup *lookup, int justinuse) {
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
    bcoverage = galloc(bcnt*sizeof(uint16));
    for ( i = 0 ; i<bcnt; ++i )
	bcoverage[i] = getushort(ttf);
    fcnt = getushort(ttf);
    fcoverage = galloc(fcnt*sizeof(uint16));
    for ( i = 0 ; i<fcnt; ++i )
	fcoverage[i] = getushort(ttf);
    scnt = getushort(ttf);
    sglyphs = galloc((scnt+1)*sizeof(uint16));
    for ( i = 0 ; i<scnt; ++i )
	if (( sglyphs[i] = getushort(ttf))>=info->glyph_cnt ) {
	    fprintf( stderr, "Bad reverse contextual chaining substitution glyph: %d is not less than %d\n",
		    sglyphs[i], info->glyph_cnt );
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
	fpst->tag = lookup->tag;
	fpst->script_lang_index = lookup->script_lang_index;
	fpst->flags = lookup->flags;
	fpst->next = info->possub;
	info->possub = fpst;

	fpst->rules = rule = gcalloc(1,sizeof(struct fpst_rule));
	fpst->rule_cnt = 1;

	rule->u.rcoverage.always1 = 1;
	rule->u.rcoverage.bcnt = bcnt;
	rule->u.rcoverage.fcnt = fcnt;
	rule->u.rcoverage.ncovers = galloc(sizeof(char *));
	rule->u.rcoverage.bcovers = galloc(bcnt*sizeof(char *));
	rule->u.rcoverage.fcovers = galloc(fcnt*sizeof(char *));
	rule->u.rcoverage.replacements = GlyphsToNames(info,sglyphs);
	glyphs = getCoverageTable(ttf,stoffset+coverage,info);
	rule->u.rcoverage.ncovers[0] = GlyphsToNames(info,glyphs);
	free(glyphs);
	for ( i=0; i<bcnt; ++i ) {
	    glyphs = getCoverageTable(ttf,stoffset+bcoverage[i],info);
	    rule->u.rcoverage.bcovers[i] = GlyphsToNames(info,glyphs);
	    free(glyphs);
	}
	for ( i=0; i<fcnt; ++i ) {
	    glyphs = getCoverageTable(ttf,stoffset+fcoverage[i],info);
	    rule->u.rcoverage.fcovers[i] = GlyphsToNames(info,glyphs);
	    free(glyphs);
	}
	rule->lookup_cnt = 0;		/* substitution lookups needed for reverse chaining */
    }
    free(sglyphs);
    free(fcoverage);
    free(bcoverage);
}

static struct feature *readttffeatures(FILE *ttf,int32 pos,int isgpos, struct ttfinfo *info) {
    /* read the features table returning an array containing all interesting */
    /*  features */
    int cnt;
    uint32 tag;
    int i,j;
    struct feature *features;

    fseek(ttf,pos,SEEK_SET);
    cnt = getushort(ttf);
    if ( cnt<=0 )
return( NULL );
    features = gcalloc(cnt+1,sizeof(struct feature));
    info->feats[isgpos] = galloc((3*cnt+1)*sizeof(uint32));
    info->feats[isgpos][0] = 0;
    for ( i=0; i<cnt; ++i ) {
	features[i].tag = tag = getlong(ttf);
	features[i].offset = getushort(ttf);
    }

    for ( i=0; i<cnt; ++i ) {
	fseek(ttf,pos+features[i].offset,SEEK_SET);
	/* feature parameters = */ getushort(ttf);
	features[i].lcnt = getushort(ttf);
	features[i].lookups = galloc(features[i].lcnt*sizeof(uint16));
	for ( j=0; j<features[i].lcnt; ++j )
	    features[i].lookups[j] = getushort(ttf);
    }

return( features );
}

static struct scriptlist *ScriptListCopy(const struct scriptlist *sl) {
    struct scriptlist *head=NULL, *last=NULL, *new;

    while ( sl ) {
	new = galloc(sizeof(struct scriptlist));
	*new = *sl;
	if ( head==NULL )
	    head = new;
	else
	    last->next = new;
	last = new;
	sl = sl->next;
    }
return( head );
}

static struct scriptlist *ScriptListMerge(struct scriptlist *into,const struct scriptlist *from) {
    struct scriptlist *sl, *new;
    int i,j;

    while ( from ) {
	for ( sl=into; sl!=NULL && sl->script!=from->script; sl=sl->next );
	if ( sl==NULL ) {
	    new = galloc(sizeof(struct scriptlist));
	    *new = *from;
	    new->next = into;
	    into = new;
	} else {
	    for ( i=0; i<from->lang_cnt; ++i ) {
		if ( sl->lang_cnt<MAX_LANG ) {
		    for ( j=0; j<sl->lang_cnt && sl->langs[j]!=from->langs[i]; ++j );
		    if ( j>=sl->lang_cnt )
			sl->langs[sl->lang_cnt++] = from->langs[i];
		}
	    }
	}
	from = from->next;
    }
return( into );
}

static struct lookup *compactttflookups(struct feature *features,uint32 *feats, int lu_cnt) {
    /* go through the feature table we read, and return an array containing */
    /*  all lookup indeces which match the feature tags */
    int cnt, extras=0;
    int i,j,k,l;
    struct lookup *lookups, *cur, *alt;

    lookups = gcalloc(lu_cnt+1,sizeof(struct lookup));
    for ( i=0; i<lu_cnt; ++i ) { lookups[i].lookup = i; lookups[i].script_lang_index = SLI_NESTED; }
    for ( i=0; features[i].tag!=0; ++i ) {
	for ( k=0; k<features[i].lcnt; ++k ) {
	    j = features[i].lookups[k];
	    if ( j>lu_cnt ) {
		fprintf( stderr, "Feature '%c%c%c%c' refers to lookup %d which is not within the lookup array[%d]\n",
			features[i].tag>>24, (features[i].tag>>16)&0xff,
			(features[i].tag>>8)&0xff, features[i].tag&0xff,
			j, lu_cnt );
	continue;
	    }
	    cur = &lookups[j];
	    while ( cur!=NULL && cur->tag!=0 && cur->tag!=features[i].tag )
		cur = cur->alttags;
	    if ( cur==NULL ) {
		cur = gcalloc(1,sizeof(struct lookup));
		cur->alttags = lookups[j].alttags;
		lookups[j].alttags = cur;
		++extras;
#if 0
 printf( "Extra tag for %c%c%c%c is %c%c%c%c\n",
     lookups[j].tag>>24, (lookups[j].tag>>16)&0xff, (lookups[j].tag>>8)&0xff, (lookups[j].tag)&0xff,
     features[i].tag>>24, (features[i].tag>>16)&0xff, (features[i].tag>>8)&0xff, (features[i].tag)&0xff );
#endif
	    }
	    if ( cur->tag==0 ) {
		cur->tag = features[i].tag;
		cur->sl = ScriptListCopy(features[i].sl);
		cur->lookup = j;
	    } else {
		cur->sl = ScriptListMerge(cur->sl,features[i].sl);
	    }
	}
	free( features[i].lookups );
    }
#if 0 	/* Now that we work with contextual pos/sub we need to keep these around */
    /* Some lookups may not be "interesting" so remove holes */
    for ( i=j=0; i<lu_cnt; ++i ) {
	if ( lookups[i].tag!=0 )
	    lookups[j++] = lookups[i];
    }
#endif
    cnt = lu_cnt;
    if ( extras!=0 ) {
	lookups = grealloc(lookups,(lu_cnt+extras+1)*sizeof(struct lookup));
	for ( i=0; i<lu_cnt; ++i ) {
	    for ( cur = lookups[i].alttags; cur!=NULL; cur = alt ) {
		alt = cur->alttags;
		lookups[cnt] = *cur;
		lookups[cnt++].alttags = NULL;
		free(cur);
	    }
	}
    }

    free( features );
    for ( i=0; i<cnt; ++i )
	if ( lookups[i].tag!=0 )
    break;
    if ( i==cnt ) {			/* if i==cnt then no lookup has an associated feature so we don't have a handled on any of them */
	free(lookups);
return( NULL );
    }

    lookups[cnt].lookup = -1;
    lookups[cnt].tag = 0;
    lookups[cnt].script_lang_index = 0xffff;

    for ( i=0; i<cnt ; ++i ) for ( k=0; k<cnt; ++k ) {
	if ( lookups[k].lookup==i && lookups[k].tag!=0 ) {
	    for ( l=0; feats[l]!=0 && feats[l]!=lookups[k].tag; ++l );
	    if ( feats[l]==0 ) {
		feats[l] = lookups[k].tag;
		feats[l+1] = 0;
	    }
	}
    }
return( lookups );
}

static void LSysAddToFeature(struct feature *feature,uint32 script,uint32 lang,
	int is_required) {
    int j;
    struct scriptlist *sl;

    for ( sl=is_required ? feature->reqsl : feature->sl; sl!=NULL && sl->script!=script; sl=sl->next );
    if ( sl==NULL ) {
	sl = gcalloc(1,sizeof(struct scriptlist));
	if ( is_required ) {
	    sl->next = feature->reqsl;
	    feature->reqsl = sl;
	} else {
	    sl->next = feature->sl;
	    feature->sl = sl;
	}
	sl->script = script;
    }
    if ( sl->lang_cnt<MAX_LANG ) {
	for ( j=sl->lang_cnt-1; j>=0 ; --j )
	    if ( sl->langs[j]==lang )
	break;
	if ( j<0 )
	    sl->langs[sl->lang_cnt++] = lang;
    }
}

static void ProcessLangSys(FILE *ttf,uint32 langsysoff,
	struct feature *features, uint32 script, uint32 lang) {
    int cnt, feature, i;

    fseek(ttf,langsysoff,SEEK_SET);
    /* lookuporder = */ getushort(ttf);	/* not meaningful yet */
    feature = getushort(ttf); /* required feature */
    if ( feature==0xffff )
	/* No required feature */;
    else {
	LSysAddToFeature(&features[feature],script,lang,true);
    }
    cnt = getushort(ttf);
    for ( i=0; i<cnt; ++i ) {
	feature = getushort(ttf);
	LSysAddToFeature(&features[feature],script,lang,false);
    }
}

static struct feature *RequiredFeatureFixup( struct feature *features) {
    int i, extra=0;

    for ( i=0; features[i].tag!=0; ++i ) {
	if ( features[i].reqsl==NULL )
	    /* Nothing to be done */;
	else if ( features[i].sl==NULL ) {
	    features[i].tag = REQUIRED_FEATURE;
	    features[i].sl = features[i].reqsl;
	    features[i].reqsl = NULL;
	} else
	    ++extra;
    }
    if ( extra == 0 )
return( features );
    features = grealloc(features,(i+extra+1)*sizeof(struct feature));
    memset(features+i,0,(extra+1)*sizeof(struct feature));
    extra = i;
    for ( i=0; features[i].tag!=0; ++i ) {
	if ( features[i].reqsl!=NULL && features[i].sl!=NULL ) {
	    features[extra] = features[i];
	    features[extra].tag = REQUIRED_FEATURE;
	    features[extra].sl = features[i].reqsl;
	    features[extra].lookups = galloc(features[extra].lcnt*sizeof(uint16));
	    memcpy(features[extra].lookups,features[i].lookups,features[extra].lcnt*sizeof(uint16));
	    features[i].reqsl = NULL; features[extra].reqsl = NULL;
	    ++extra;
	}
    }
return( features );
}

static struct feature *tagTtfFeaturesWithScript(FILE *ttf,uint32 script_pos,
	struct feature *features) {
    int cnt, i, j;
    struct scriptrec {
	uint32 script;
	uint32 offset;
    } *scripts;
    struct stab {
	uint16 deflangsys;
	int langcnt, maxcnt;
	uint16 *langsys;
	uint32 *lang;
    } stab;

    fseek(ttf,script_pos,SEEK_SET);
    cnt = getushort(ttf);
    scripts = galloc(cnt*sizeof(struct scriptrec));
    for ( i=0; i<cnt; ++i ) {
	scripts[i].script = getlong(ttf);
	scripts[i].offset = script_pos+getushort(ttf);
    }

    memset(&stab,0,sizeof(stab));
    stab.maxcnt = 30;
    stab.langsys = galloc(30*sizeof(uint16));
    stab.lang = galloc(30*sizeof(uint32));
    for ( i=0; i<cnt; ++i ) {
	fseek(ttf,scripts[i].offset,SEEK_SET);
	stab.deflangsys = getushort(ttf);
	stab.langcnt = getushort(ttf);
	if ( stab.langcnt>=stab.maxcnt ) {
	    stab.maxcnt = stab.langcnt+30;
	    stab.langsys = grealloc(stab.langsys,stab.maxcnt*sizeof(uint16));
	    stab.lang = grealloc(stab.langsys,stab.maxcnt*sizeof(uint32));
	}
	for ( j=0; j<stab.langcnt; ++j ) {
	    stab.lang[j] = getlong(ttf);
	    stab.langsys[j] = getushort(ttf);
	}
	if ( stab.deflangsys!=0 )
	    ProcessLangSys(ttf,scripts[i].offset+stab.deflangsys,features,scripts[i].script,DEFAULT_LANG);
	for ( j=0; j<stab.langcnt; ++j )
	    ProcessLangSys(ttf,scripts[i].offset+stab.langsys[j],features,scripts[i].script,stab.lang[j]);
    }
    free(stab.langsys);
    free(stab.lang);
    free(scripts);

return( RequiredFeatureFixup(features));
}

static int SLMatch(struct script_record *sr,struct scriptlist *sl) {
    int i,j;
    struct scriptlist *slt;

    for ( j=0, slt=sl; slt!=NULL; slt=slt->next, ++j );
    for ( i=0; sr[i].script!=0; ++i );
    if ( i!=j )
return( false );

    for ( slt=sl; slt!=NULL; slt=slt->next ) {
	for ( i=0; sr[i].script!=0 && sr[i].script!=slt->script; ++i );
	if ( sr[i].script==0 )
return( false );
	for ( j=0 ; sr[i].langs[j]!=0; ++j );
	if ( j!=slt->lang_cnt )
return( false );
	for ( j=0 ; sr[i].langs[j]!=0; ++j )
	    if ( sr[i].langs[j]!=slt->langs[j] )
return( false );
    }
return( true );
}

static void FigureScriptIndeces(struct ttfinfo *info,struct lookup *lookups) {
    int i,j,k;
    struct scriptlist *sl, *snext;

    for ( i=0; lookups[i].script_lang_index!=0xffff; ++i );
    if ( info->script_lang==NULL ) {
	info->script_lang = gcalloc(i+1,sizeof(struct script_record *));
    } else {
	for ( j=0; info->script_lang[j]!=NULL; ++j );
	info->script_lang = grealloc(info->script_lang,(i+j+1)*sizeof(struct script_record *));
    }
    for ( i=0; lookups[i].script_lang_index!=0xffff; ++i ) {
	for ( j=0; info->script_lang[j]!=NULL; ++j )
	    if ( SLMatch(info->script_lang[j],lookups[i].sl))
	break;
	lookups[i].script_lang_index = j;
	if ( info->script_lang[j]==NULL ) {
	    for ( k=0, sl=lookups[i].sl; sl!=NULL; sl=sl->next, ++k );
	    info->script_lang[j] = galloc((k+1)*sizeof(struct script_record));
	    for ( k=0, sl=lookups[i].sl; sl!=NULL; sl=sl->next, ++k ) {
		info->script_lang[j][k].script = sl->script;
		info->script_lang[j][k].langs = galloc((sl->lang_cnt+1)*sizeof(uint32));
		memcpy(info->script_lang[j][k].langs,sl->langs,sl->lang_cnt*sizeof(uint32));
		info->script_lang[j][k].langs[sl->lang_cnt]=0;
	    }
	    info->script_lang[j][k].script = 0;
	    info->script_lang[j+1] = NULL;
	}
	for ( sl=lookups[i].sl; sl!=NULL; sl=snext ) {
	    snext = sl->next;
	    free( sl );
	}
    }
}

static void gposExtensionSubTable(FILE *ttf, int stoffset,
	struct ttfinfo *info, struct lookup *lookup,
	struct lookup *alllooks) {
    uint32 base = ftell(ttf), st, offset;
    int lu_type;

    /* Format = */ getushort(ttf);
    lu_type = getushort(ttf);
    offset = getlong(ttf);

    fseek(ttf,st = base+offset,SEEK_SET);
    switch ( lu_type ) {
      case 1:
	gposSimplePos(ttf,st,info,lookup);
      break;  
      case 2:
	if ( lookup->tag==CHR('k','e','r','n') )
	    gposKernSubTable(ttf,st,info,lookup,false);
	else if ( lookup->tag==CHR('v','k','r','n') )
	    gposKernSubTable(ttf,st,info,lookup,true);
	else
	    gposKernSubTable(ttf,st,info,lookup,2);
      break;  
      case 3:
	gposCursiveSubTable(ttf,st,info,lookup);
      break;
      case 4: case 5: case 6:
	gposMarkSubTable(ttf,st,info,lookup,lu_type);
      break;
      case 7:
	gposContextSubTable(ttf,st,info,lookup,alllooks);
      break;
      case 8:
	gposChainingSubTable(ttf,st,info,lookup,alllooks);
      break;
/* Any cases added here also need to go in the gposLookupSwitch */
      case 9:
	fprintf( stderr, "This font is erroneous it has a GPOS extension subtable that points to\nanother extension sub-table.\n" );
      break;
    }
}

static void gsubExtensionSubTable(FILE *ttf, int stoffset,
	struct ttfinfo *info, struct lookup *lookup, int justinuse,
	struct lookup *alllooks) {
    uint32 base = ftell(ttf), st, offset;
    int lu_type;

    /* Format = */ getushort(ttf);
    lu_type = getushort(ttf);
    offset = getlong(ttf);

    fseek(ttf,st = base+offset,SEEK_SET);
    switch ( lu_type ) {
      case 1:
	gsubSimpleSubTable(ttf,st,info,lookup,justinuse);
      break;
      case 2: case 3:	/* Multiple and alternate have same format, different semantics */
	gsubMultipleSubTable(ttf,st,info,lookup,lu_type,justinuse);
      break;
      case 4:
	gsubLigatureSubTable(ttf,st,info,lookup,justinuse);
      break;
      case 5:
	gsubContextSubTable(ttf,st,info,lookup,justinuse,alllooks);
      break;
      case 6:
	gsubChainingSubTable(ttf,st,info,lookup,justinuse,alllooks);
      break;
      case 7:
	fprintf( stderr, "This font is erroneous it has a GSUB extension subtable that points to\nanother extension sub-table.\n" );
      break;
      case 8:
	gsubReverseChainSubTable(ttf,st,info,lookup,justinuse);
      break;
/* Any cases added here also need to go in the gsubLookupSwitch */
    }
}

static void gposLookupSwitch(FILE *ttf, int st,
	struct ttfinfo *info, struct lookup *lookup,
	struct lookup *alllooks, int lu_type) {

    switch ( lu_type ) {
      case 1:
	gposSimplePos(ttf,st,info,lookup);
      break;  
      case 2:
	if ( lookup->tag==CHR('k','e','r','n') )
	    gposKernSubTable(ttf,st,info,lookup,false);
	else if ( lookup->tag==CHR('v','k','r','n') )
	    gposKernSubTable(ttf,st,info,lookup,true);
	else
	    gposKernSubTable(ttf,st,info,lookup,2);
      break;  
      case 3:
	gposCursiveSubTable(ttf,st,info,lookup);
      break;
      case 4: case 5: case 6:
	gposMarkSubTable(ttf,st,info,lookup,lu_type);
      break;
      case 7:
	gposContextSubTable(ttf,st,info,lookup,alllooks);
      break;
      case 8:
	gposChainingSubTable(ttf,st,info,lookup,alllooks);
      break;
      case 9:
	gposExtensionSubTable(ttf,st,info,lookup,alllooks);
      break;
/* Any cases added here also need to go in the gposExtensionSubTable */
    }
}

static void gsubLookupSwitch(FILE *ttf, int st,
	struct ttfinfo *info, struct lookup *lookup, int justinuse,
	struct lookup *alllooks, int lu_type) {

    switch ( lu_type ) {
      case 1:
	gsubSimpleSubTable(ttf,st,info,lookup,justinuse);
      break;
      case 2: case 3:	/* Multiple and alternate have same format, different semantics */
	gsubMultipleSubTable(ttf,st,info,lookup,lu_type,justinuse);
      break;
      case 4:
	gsubLigatureSubTable(ttf,st,info,lookup,justinuse);
      break;
      case 5:
	gsubContextSubTable(ttf,st,info,lookup,justinuse,alllooks);
      break;
      case 6:
	gsubChainingSubTable(ttf,st,info,lookup,justinuse,alllooks);
      break;
      case 7:
	gsubExtensionSubTable(ttf,st,info,lookup,justinuse,alllooks);
      break;
      case 8:
	gsubReverseChainSubTable(ttf,st,info,lookup,justinuse);
      break;
/* Any cases added here also need to go in the gsubExtensionSubTable */
    }
}

static uint32 InfoGenerateNewFeatureTag(struct gentagtype *gentags,int lu_type,
	int gpos, uint32 suggested_tag, int lookup_index) {
    int type = pst_null;
    char buf[8];

    switch ( gpos ) {
      case 0:
	switch ( lu_type ) {
	  case 1:
	    type = pst_substitution;
	  break;
	  case 2:
	    type = pst_multiple;
	  break;
	  case 3:
	    type = pst_alternate;
	  break;
	  case 4:
	    type = pst_ligature;
	  break;
	  case 5:
	    type = pst_contextsub;
	  break;
	  case 6:
	    type = pst_chainsub;
	  break;
	  case 8:
	    type = pst_reversesub;
	  break;
	    }
      break;
      case 1:
	switch ( lu_type ) {
	  case 1:
	    type=pst_position;
	  break;
	  case 2:
	    type = pst_pair;
	  break;  
	  case 3:
	  case 4:
	  case 5:
	  case 6:
	    type = pst_anchors;
	  break;
	  case 7:
	    type = pst_contextpos;
	  break;
	  case 8:
	    type = pst_chainpos;
	  break;
	}
      break;
    }
    if ( suggested_tag==0 && lookup_index<1000 ) {
	sprintf( buf, "L%03d", lookup_index );
	suggested_tag = CHR(buf[0],buf[1],buf[2],buf[3]);
    }
return( SFGenerateNewFeatureTag(gentags,type,suggested_tag));
}

static void ProcessGPOSGSUBlookup(FILE *ttf,struct ttfinfo *info,int gpos,
	struct lookup *lookup, int inusetype, struct lookup *alllooks) {
    int j, lu_type, cnt, st;
    uint16 *st_offsets;
    uint32 oldtag; int oldsli;

    if ( lookup->offset==0 )
return;
    fseek(ttf,lookup->offset,SEEK_SET);
    lu_type = getushort(ttf);
    lookup->flags = getushort(ttf);
    cnt = getushort(ttf);
    st_offsets = galloc(cnt*sizeof(uint16));
    for ( j=0; j<cnt; ++j )
	st_offsets[j] = getushort(ttf);

    oldtag = lookup->tag; oldsli = lookup->script_lang_index;
    if ( lookup->make_subtag ) {
	lookup->tag = lookup->subtag = InfoGenerateNewFeatureTag(&info->gentags,
		lu_type,gpos,oldtag,lookup->lookup);
	lookup->script_lang_index = SLI_NESTED;
    }

    for ( j=0; j<cnt; ++j ) {
	fseek(ttf,st = lookup->offset+st_offsets[j],SEEK_SET);
	if ( gpos ) {
	    gposLookupSwitch(ttf,st,info,lookup,alllooks,lu_type);
	} else {
	    gsubLookupSwitch(ttf,st,info,lookup,inusetype,alllooks,lu_type);
	}
    }
    free(st_offsets);

    lookup->tag = oldtag;
    lookup->script_lang_index = oldsli;
}

static void ProcessGPOSGSUB(FILE *ttf,struct ttfinfo *info,int gpos,int inusetype) {
    int i, k, lu_cnt;
    uint16 offset;
    int32 base, lookup_start;
    int32 script_off, feature_off;
    struct feature *features;
    struct lookup *lookups;

    base = gpos?info->gpos_start:info->gsub_start;
    fseek(ttf,base,SEEK_SET);
    /* version = */ getlong(ttf);
    script_off = getushort(ttf);
    feature_off = getushort(ttf);
    lookup_start = base+getushort(ttf);
    fseek(ttf,lookup_start,SEEK_SET);
    lu_cnt = getushort(ttf);

    features = readttffeatures(ttf,base+feature_off,gpos,info);
    if ( features==NULL )		/* None of the data we care about */
return;
    features = tagTtfFeaturesWithScript(ttf,base+script_off,features);
    lookups = compactttflookups( features,info->feats[gpos],lu_cnt );
    if ( lookups==NULL )
return;
    FigureScriptIndeces(info,lookups);

    fseek(ttf,lookup_start,SEEK_SET);

    lu_cnt = getushort(ttf);
    for ( i=0; i<lu_cnt; ++i ) {
	offset = getushort(ttf);
	for ( k=0; lookups[k].lookup!=(uint16) -1; ++k ) {
	    if ( lookups[k].lookup==i )
		lookups[k].offset = lookup_start+offset;
	}
    }

    for ( k=0; lookups[k].lookup!=(uint16) -1; ++k ) if ( lookups[k].tag!=0 ) {
	ProcessGPOSGSUBlookup(ttf,info,gpos,&lookups[k],inusetype,lookups);
    }
    free(lookups);
}

void readttfgsubUsed(FILE *ttf,struct ttfinfo *info) {
    ProcessGPOSGSUB(ttf,info,false,git_justinuse);
}

void GuessNamesFromGSUB(FILE *ttf,struct ttfinfo *info) {
    ProcessGPOSGSUB(ttf,info,false,git_findnames);
}

void readttfgpossub(FILE *ttf,struct ttfinfo *info,int gpos) {
    ProcessGPOSGSUB(ttf,info,gpos,git_normal);
}

void readttfgdef(FILE *ttf,struct ttfinfo *info) {
    int lclo;
    int coverage, cnt, i,j, format;
    uint16 *glyphs, *lc_offsets, *offsets;
    uint32 caret_base;
    PST *pst;
    SplineChar *sc;

    fseek(ttf,info->gdef_start,SEEK_SET);
    if ( getlong(ttf)!=0x00010000 )
return;
    /* glyph class def = */ getushort(ttf);
    /* attach list = */ getushort(ttf);
    lclo = getushort(ttf);		/* ligature caret list */
    /* mark attach class = */ getushort(ttf);
    if ( lclo==0 )
return;

    lclo += info->gdef_start;
    fseek(ttf,lclo,SEEK_SET);
    coverage = getushort(ttf);
    cnt = getushort(ttf);
    if ( cnt==0 )
return;
    lc_offsets = galloc(cnt*sizeof(uint16));
    for ( i=0; i<cnt; ++i )
	lc_offsets[i]=getushort(ttf);
    glyphs = getCoverageTable(ttf,lclo+coverage,info);
    for ( i=0; i<cnt; ++i ) if ( glyphs[i]<info->glyph_cnt ) {
	fseek(ttf,lclo+lc_offsets[i],SEEK_SET);
	sc = info->chars[glyphs[i]];
	for ( pst=sc->possub; pst!=NULL && pst->type!=pst_lcaret; pst=pst->next );
	if ( pst==NULL ) {
	    pst = chunkalloc(sizeof(PST));
	    pst->next = sc->possub;
	    sc->possub = pst;
	    pst->type = pst_lcaret;
	}
	caret_base = ftell(ttf);
	pst->u.lcaret.cnt = getushort(ttf);
	if ( pst->u.lcaret.carets!=NULL ) free(pst->u.lcaret.carets);
	offsets = galloc(pst->u.lcaret.cnt*sizeof(uint16));
	for ( j=0; j<pst->u.lcaret.cnt; ++j )
	    offsets[j] = getushort(ttf);
	pst->u.lcaret.carets = galloc(pst->u.lcaret.cnt*sizeof(int16));
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
		fprintf( stderr, "!!!! Unknown caret format %d !!!!\n", format );
	    }
	}
	free(offsets);
    }
    free(lc_offsets);
    free(glyphs);
}

static void readttf_applelookup(FILE *ttf,struct ttfinfo *info,
	void (*apply_values)(struct ttfinfo *info, int gfirst, int glast,FILE *ttf),
	void (*apply_value)(struct ttfinfo *info, int gfirst, int glast,FILE *ttf),
	void (*apply_default)(struct ttfinfo *info, int gfirst, int glast,void *def),
	void *def) {
    int format, i, first, last, data_off, cnt, prev;
    uint32 here;
    uint32 base = ftell(ttf);

    switch ( format = getushort(ttf)) {
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
	    if ( apply_default!=NULL )
		apply_default(info,prev,first-1,def);
	    apply_value(info,first,last,ttf);
	    prev = last+1;
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
	    here = ftell(ttf);
	    if ( apply_default!=NULL )
		apply_default(info,prev,first-1,def);
	    fseek(ttf,base+data_off,SEEK_SET);
	    apply_values(info,first,last,ttf);
	    fseek(ttf,here,SEEK_SET);
	    prev = last+1;
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
	    if ( apply_default!=NULL )
		apply_default(info,prev,first-1,def);
	    apply_value(info,first,first,ttf);
	    prev = first+1;
	}
      break;
      case 8:	/* Simple array */
	first = getushort(ttf);
	cnt = getushort(ttf);
	if ( apply_default!=NULL ) {
	    apply_default(info,0,first-1,def);
	    apply_default(info,first+cnt,info->glyph_cnt-1,def);
	}
	apply_values(info,first,first+cnt-1,ttf);
      break;
      default:
	fprintf( stderr, "Invalid lookup table format. %d\n", format );
      break;
    }
}

static void TTF_SetProp(struct ttfinfo *info,int gnum, int prop) {
    int offset;
    PST *pst;

    if ( gnum<0 || gnum>=info->glyph_cnt ) {
	fprintf( stderr, "Glyph out of bounds in 'prop' table %d\n", gnum );
return;
    }

    if ( prop&0x1000 ) {	/* Mirror */
	offset = (prop<<20)>>28;
	if ( gnum+offset>=0 && gnum+offset<info->glyph_cnt &&
		info->chars[gnum+offset]->name!=NULL ) {
	    pst = chunkalloc(sizeof(PST));
	    pst->type = pst_substitution;
	    pst->tag = CHR('r','t','l','a');
	    pst->script_lang_index = SLIFromInfo(info,info->chars[gnum],DEFAULT_LANG);
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

    def_prop = (int) def;
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
    readttf_applelookup(ttf,info,
	    prop_apply_values,prop_apply_value,
	    prop_apply_default,(void *) def);
}

static void TTF_SetLcaret(struct ttfinfo *info, int gnum, int offset, FILE *ttf) {
    uint32 here = ftell(ttf);
    PST *pst;
    SplineChar *sc;
    int cnt, i;

    if ( gnum<0 || gnum>=info->glyph_cnt ) {
	fprintf( stderr, "Glyph out of bounds in 'lcar' table %d\n", gnum );
return;
    } else if ( (sc=info->chars[gnum])==NULL )
return;

    fseek(ttf,info->lcar_start+offset,SEEK_SET);
    cnt = getushort(ttf);
    pst = chunkalloc(sizeof(PST));
    pst->type = pst_lcaret;
    pst->tag = CHR(' ',' ',' ',' ');
    pst->next = sc->possub;
    sc->possub = pst;
    pst->u.lcaret.cnt = cnt;
    pst->u.lcaret.carets = galloc(cnt*sizeof(uint16));
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
	    lcar_apply_values,lcar_apply_value,NULL,NULL);
}

static void TTF_SetOpticalBounds(struct ttfinfo *info, int gnum, int left, int right) {
    PST *pst;
    SplineChar *sc;

    if ( left==0 && right==0 )
return;

    if ( gnum<0 || gnum>=info->glyph_cnt ) {
	fprintf( stderr, "Glyph out of bounds in 'opbd' table %d\n", gnum );
return;
    } else if ( (sc=info->chars[gnum])==NULL )
return;

    if ( left!=0 ) {
	pst = chunkalloc(sizeof(PST));
	pst->type = pst_position;
	pst->tag = CHR('l','f','b','d');
	pst->script_lang_index = SLIFromInfo(info,sc,DEFAULT_LANG);
	pst->next = sc->possub;
	sc->possub = pst;
	pst->u.pos.xoff = left;
	pst->u.pos.h_adv_off = left;
    }
    if ( right!=0 ) {
	pst = chunkalloc(sizeof(PST));
	pst->type = pst_position;
	pst->tag = CHR('r','t','b','d');
	pst->script_lang_index = SLIFromInfo(info,sc,DEFAULT_LANG);
	pst->next = sc->possub;
	sc->possub = pst;
	pst->u.pos.h_adv_off = -right;
    }
}

static void opbd_apply_values(struct ttfinfo *info, int gfirst, int glast,FILE *ttf) {
    int i, left, right, offset;
    uint32 here;
    /* the apple docs say that the bounds live in the lookup table */
    /* the picture in the docs shows the bounds pointed to */
    /* the example shows the bounds pointed to */
    /* We conclude the original authors were bad writers */

    for ( i=gfirst; i<=glast; ++i ) {
	offset = getushort(ttf);
	here = ftell(ttf);
	fseek(ttf,info->opbd_start+offset,SEEK_SET);
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
    readttf_applelookup(ttf,info,
	    opbd_apply_values,opbd_apply_value,NULL,NULL);
}

static void TTF_SetMortSubs(struct ttfinfo *info, int gnum, int gsubs) {
    PST *pst;
    SplineChar *sc, *ssc;

    if ( gsubs==0 )
return;

    if ( gnum<0 || gnum>=info->glyph_cnt ) {
	fprintf( stderr, "Glyph out of bounds in 'mort' table %d\n", gnum );
return;
    } else if ( gsubs<0 || gsubs>=info->glyph_cnt ) {
	fprintf( stderr, "Substitute glyph out of bounds in 'mort' table %d\n", gsubs );
return;
    } else if ( (sc=info->chars[gnum])==NULL || (ssc=info->chars[gsubs])==NULL )
return;

    pst = chunkalloc(sizeof(PST));
    pst->type = pst_substitution;
    pst->tag = info->mort_subs_tag;
    pst->flags = info->mort_r2l ? pst_r2l : 0;
    pst->script_lang_index = SLIFromInfo(info,sc,DEFAULT_LANG);
    pst->next = sc->possub;
    sc->possub = pst;
    pst->u.subs.variant = copy(ssc->name);
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

int32 memlong(uint8 *data,int offset) {
    int ch1 = data[offset], ch2 = data[offset+1], ch3 = data[offset+2], ch4 = data[offset+3];
return( (ch1<<24)|(ch2<<16)|(ch3<<8)|ch4 );
}

int memushort(uint8 *data,int offset) {
    int ch1 = data[offset], ch2 = data[offset+1];
return( (ch1<<8)|ch2 );
}

void memputshort(uint8 *data,int offset,uint16 val) {
    data[offset] = (val>>8);
    data[offset+1] = val&0xff;
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

static void mort_figure_ligatures(struct statemachine *sm, int lcp, int off, int32 lig_offset) {
    uint32 lig;
    int i, j, lig_glyph;
    PST *pst;
    int len;

    if ( lcp<0 || off+3>sm->length )
return;

    lig = memlong(sm->data,off);
    off += sizeof(long);

    for ( i=0; i<sm->info->glyph_cnt; ++i ) if ( sm->classes[i]==sm->lig_comp_classes[lcp] ) {
	sm->lig_comp_glyphs[lcp] = i;
	lig_offset += memushort(sm->data,2*( ((((int32) lig)<<2)>>2) + i ) );
	if ( lig&0xc0000000 ) {
	    if ( lig_offset+1 > sm->length ) {
		fprintf( stderr, "Invalid ligature offset\n" );
    break;
	    }
	    lig_glyph = memushort(sm->data,lig_offset);
	    if ( lig_glyph>=sm->info->glyph_cnt ) {
		fprintf(stderr, "Attempt to make a ligature for glyph %d out of " );
		for ( j=lcp; j<sm->lcp; ++j )
		    fprintf(stderr,"%d ",sm->lig_comp_glyphs[j]);
		fprintf(stderr,"\n");
	    } else {
		char *comp;
		for ( len=0, j=lcp; j<sm->lcp; ++j )
		    len += strlen(sm->info->chars[sm->lig_comp_glyphs[j]]->name)+1;
		comp = galloc(len);
		*comp = '\0';
		for ( j=lcp; j<sm->lcp; ++j ) {
		    strcat(comp,sm->info->chars[sm->lig_comp_glyphs[j]]->name);
		    if ( j!=sm->lcp-1 )
			strcat(comp," ");
		}
		for ( pst=sm->info->chars[lig_glyph]->possub; pst!=NULL; pst=pst->next )
		    if ( pst->type==pst_ligature && pst->tag==sm->info->mort_subs_tag &&
			    strcmp(comp,pst->u.lig.components)==0 )
		break;
		/* There are cases where there will be multiple entries for */
		/*  the same lig. ie. if we have "ff" and "ffl" then there */
		/*  will be multiple entries for "ff" */
		if ( pst == NULL ) {
		    pst = chunkalloc(sizeof(PST));
		    pst->type = pst_ligature;
		    pst->tag = sm->info->mort_subs_tag;
		    pst->flags = sm->info->mort_r2l ? (pst_r2l|pst_ignorecombiningmarks) : pst_ignorecombiningmarks;
		    pst->script_lang_index = SLIFromInfo(sm->info,sm->info->chars[lig_glyph],DEFAULT_LANG);
		    pst->u.lig.components = comp;
		    pst->u.lig.lig = sm->info->chars[lig_glyph];
		    pst->next = sm->info->chars[lig_glyph]->possub;
		    sm->info->chars[lig_glyph]->possub = pst;
		} else
		    free(comp);
	    }
	} else
	    mort_figure_ligatures(sm,lcp-1,off,lig_offset);
	lig_offset -= memushort(sm->data,2*( ((((int32) lig)<<2)>>2) + i ) );
    }
}

static void follow_mort_state(struct statemachine *sm,int offset,int class) {
    int state = (offset-sm->stateOffset)/sm->nClasses;
    int class_top, class_bottom;

    if ( state<0 || state>=sm->smax || sm->states_in_use[state] || sm->lcp>=MAX_LIG_COMP )
return;
    ++ sm->cnt;
    if ( sm->cnt>=10000 ) {
	if ( sm->cnt==10000 )
	    fprintf(stderr,"In an attempt to process the ligatures of this font, I've concluded\nthat the state machine in Apple's mort/morx table is\n(like the learned constable) too cunning to be understood.\nI shall give up on it. Your ligatures may be incomplete.\n" );
return;
    }
    sm->states_in_use[state] = true;

    if ( class==-1 ) { class_bottom = 0; class_top = sm->nClasses; }
    else { class_bottom = class; class_top = class+1; }
    for ( class=class_bottom; class<class_top; ++class ) {
	int ent = sm->data[offset+class];
	int newState = memushort(sm->data,sm->entryOffset+4*ent);
	int flags = memushort(sm->data,sm->entryOffset+4*ent+2);
	/* If we have the same entry as state 0, then presumably we are */
	/*  ignoring the components read so far and starting over with a new */
	/*  lig (similarly for state 1) */
	if (( state!=0 && sm->data[sm->stateOffset+class] == ent ) ||
		(state>1 && sm->data[sm->stateOffset+sm->nClasses+class]==ent ))
    continue;
	if ( flags&0x8000 )	/* Set component */
	    sm->lig_comp_classes[sm->lcp++] = class;
	if ( flags&0x3fff ) {
	    mort_figure_ligatures(sm, sm->lcp-1, flags & 0x3fff, 0);
	} else if ( flags&0x8000 )
	    follow_mort_state(sm,newState,(flags&0x4000)?class:-1);
	if ( flags&0x8000 )
	    --sm->lcp;
    }
    sm->states_in_use[state] = false;
}

static void morx_figure_ligatures(struct statemachine *sm, int lcp, int ligindex, int32 lig_offset) {
    uint32 lig;
    int i, j, lig_glyph;
    PST *pst;
    int len;

    if ( lcp<0 || sm->ligActOff+4*ligindex+3>sm->length )
return;

    lig = memlong(sm->data,sm->ligActOff+4*ligindex);
    ++ligindex;

    for ( i=0; i<sm->info->glyph_cnt; ++i ) if ( sm->classes[i]==sm->lig_comp_classes[lcp] ) {
	sm->lig_comp_glyphs[lcp] = i;
	lig_offset += memushort(sm->data,sm->compOff + 2*( ((((int32) lig)<<2)>>2) + i ) );
	if ( lig&0xc0000000 ) {
	    if ( sm->ligOff+2*lig_offset+1 > sm->length ) {
		fprintf( stderr, "Invalid ligature offset\n" );
    break;
	    }
	    lig_glyph = memushort(sm->data,sm->ligOff+2*lig_offset);
	    if ( lig_glyph>=sm->info->glyph_cnt ) {
		fprintf(stderr, "Attempt to make a ligature for glyph %d out of " );
		for ( j=lcp; j<sm->lcp; ++j )
		    fprintf(stderr,"%d ",sm->lig_comp_glyphs[j]);
		fprintf(stderr,"\n");
	    } else {
		char *comp;
		for ( len=0, j=lcp; j<sm->lcp; ++j )
		    len += strlen(sm->info->chars[sm->lig_comp_glyphs[j]]->name)+1;
		comp = galloc(len);
		*comp = '\0';
		for ( j=lcp; j<sm->lcp; ++j ) {
		    strcat(comp,sm->info->chars[sm->lig_comp_glyphs[j]]->name);
		    if ( j!=sm->lcp-1 )
			strcat(comp," ");
		}
		for ( pst=sm->info->chars[lig_glyph]->possub; pst!=NULL; pst=pst->next )
		    if ( pst->type==pst_ligature && pst->tag==sm->info->mort_subs_tag &&
			    strcmp(comp,pst->u.lig.components)==0 )
		break;
		/* There are cases where there will be multiple entries for */
		/*  the same lig. ie. if we have "ff" and "ffl" then there */
		/*  will be multiple entries for "ff" */
		if ( pst == NULL ) {
		    pst = chunkalloc(sizeof(PST));
		    pst->type = pst_ligature;
		    pst->flags = sm->info->mort_r2l ? (pst_r2l|pst_ignorecombiningmarks) : pst_ignorecombiningmarks;
		    pst->tag = sm->info->mort_subs_tag;
		    pst->script_lang_index = SLIFromInfo(sm->info,sm->info->chars[lig_glyph],DEFAULT_LANG);
		    pst->u.lig.components = comp;
		    pst->u.lig.lig = sm->info->chars[lig_glyph];
		    pst->next = sm->info->chars[lig_glyph]->possub;
		    sm->info->chars[lig_glyph]->possub = pst;
		}
	    }
	} else
	    morx_figure_ligatures(sm,lcp-1,ligindex,lig_offset);
	lig_offset -= memushort(sm->data,sm->compOff + 2*( ((((int32) lig)<<2)>>2) + i ) );
    }
}

static void follow_morx_state(struct statemachine *sm,int state,int class) {
    int class_top, class_bottom;

    if ( state<0 || state>=sm->smax || sm->states_in_use[state] || sm->lcp>=MAX_LIG_COMP )
return;
    ++ sm->cnt;
    if ( sm->cnt>=10000 ) {
	if ( sm->cnt==10000 )
	    fprintf(stderr,"In an attempt to process the ligatures of this font, I've concluded\nthat the state machine in Apple's mort/morx table is\n(like the learned constable) too cunning to be understood.\nI shall give up on it. Your ligatures may be incomplete.\n" );
return;
    }
    sm->states_in_use[state] = true;

    if ( class==-1 ) { class_bottom = 0; class_top = sm->nClasses; }
    else { class_bottom = class; class_top = class+1; }
    for ( class=class_bottom; class<class_top; ++class ) {
	int ent = memushort(sm->data, sm->stateOffset + 2*(state*sm->nClasses+class) );
	int newState = memushort(sm->data,sm->entryOffset+6*ent);
	int flags = memushort(sm->data,sm->entryOffset+6*ent+2);
	int ligindex = memushort(sm->data,sm->entryOffset+6*ent+4);
	/* If we have the same entry as state 0, then presumably we are */
	/*  ignoring the components read so far and starting over with a new */
	/*  lig (similarly for state 1) */
	if (( state!=0 && memushort(sm->data, sm->stateOffset + 2*class) == ent ) ||
		(state>1 && memushort(sm->data, sm->stateOffset + 2*(sm->nClasses+class))==ent ))
    continue;
	if ( flags&0x8000 )	/* Set component */
	    sm->lig_comp_classes[sm->lcp++] = class;
	if ( flags&0x2000 ) {
	    morx_figure_ligatures(sm, sm->lcp-1, ligindex, 0);
	} else if ( flags&0x8000 )
	    follow_morx_state(sm,newState,(flags&0x4000)?class:-1);
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
    sm.data = galloc(length);
    sm.length = length;
    if ( fread(sm.data,1,length,ttf)!=length ) {
	free(sm.data);
	fprintf( stderr, "Bad mort ligature table. Not long enough\n");
return;
    }
    fseek(ttf,here,SEEK_SET);
    if ( ismorx ) {
	sm.nClasses = memlong(sm.data,0);
	sm.classOffset = memlong(sm.data,sizeof(long));
	sm.stateOffset = memlong(sm.data,2*sizeof(long));
	sm.entryOffset = memlong(sm.data,3*sizeof(long));
	sm.ligActOff = memlong(sm.data,4*sizeof(long));
	sm.compOff = memlong(sm.data,5*sizeof(long));
	sm.ligOff = memlong(sm.data,6*sizeof(long));
	fseek(ttf,here+sm.classOffset,SEEK_SET);
	sm.classes = info->morx_classes = galloc(info->glyph_cnt*sizeof(uint16));
	for ( i=0; i<info->glyph_cnt; ++i )
	    sm.classes[i] = 1;			/* Out of bounds */
	readttf_applelookup(ttf,info,
		mortclass_apply_values,mortclass_apply_value,NULL,NULL);
	sm.smax = length/(2*sm.nClasses);
	sm.states_in_use = gcalloc(sm.smax,sizeof(uint8));
	follow_morx_state(&sm,0,-1);
    } else {
	sm.nClasses = memushort(sm.data,0);
	sm.classOffset = memushort(sm.data,sizeof(uint16));
	sm.stateOffset = memushort(sm.data,2*sizeof(uint16));
	sm.entryOffset = memushort(sm.data,3*sizeof(uint16));
	sm.ligActOff = memushort(sm.data,4*sizeof(uint16));
	sm.compOff = memushort(sm.data,5*sizeof(uint16));
	sm.ligOff = memushort(sm.data,6*sizeof(uint16));
	sm.classes = galloc(info->glyph_cnt*sizeof(uint16));
	for ( i=0; i<info->glyph_cnt; ++i )
	    sm.classes[i] = 1;			/* Out of bounds */
	first = memushort(sm.data,sm.classOffset);
	cnt = memushort(sm.data,sm.classOffset+sizeof(uint16));
	for ( i=0; i<cnt; ++i )
	    sm.classes[first+i] = sm.data[sm.classOffset+2*sizeof(uint16)+i];
	sm.smax = length/sm.nClasses;
	sm.states_in_use = gcalloc(sm.smax,sizeof(uint8));
	follow_mort_state(&sm,sm.stateOffset,-1);
    }
    free(sm.data);
    free(sm.states_in_use);
    free(sm.classes);
}

static uint32 readmortchain(FILE *ttf,struct ttfinfo *info, uint32 base, int ismorx) {
    uint32 chain_len, nfeatures, nsubtables;
    uint32 enable_flags, disable_flags, flags;
    int featureType, featureSetting;
    int i,j,k,l;
    uint32 length, coverage;
    uint32 here;
    uint32 tag;
    struct tagmaskfeature { uint32 tag, enable_flags; } tmf[32];
    int r2l;

    /* default flags = */ getlong(ttf);
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
	tag = MacFeatureToOTTag(featureType,featureSetting);
	if ( enable_flags!=0 && tag!=0 && k<32 ) {
	    tmf[k].tag = tag;
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
	for ( j=k-1; j>=0 && !(flags&tmf[j].enable_flags); --j );
	if ( j>=0 ) {
	    info->mort_subs_tag = tmf[j].tag;
	    info->mort_r2l = r2l;
	    for ( l=0; info->feats[0][l]!=0; ++l )
		if ( info->feats[0][l]==tmf[j].tag )
	    break;
	    if ( info->feats[0][l]==0 && l<info->mort_max ) {
		info->feats[0][l] = tmf[j].tag;
		info->feats[0][l+1] = 0;
	    }
	    switch( coverage&0xff ) {
	      case 0:
		/* Indic rearangement */
	      break;
	      case 1:
		/* contextual glyph substitution */
	      break;
	      case 2:	/* ligature substitution */
		readttf_mortx_lig(ttf,info,ismorx,here,length);
	      break;
	      case 4:	/* non-contextual glyph substitutions */
		/* Another case of that isn't specified in the docs */
		/* It seems unlikely that (in morx at least!) the base for */
		/*  offsets in the lookup table should be the start of the */
		/*  mor[tx] table, it would make more sense for it to be the*/
		/*  start of the lookup table instead (for format 4 lookups) */
		readttf_applelookup(ttf,info,
			mort_apply_values,mort_apply_value,NULL,NULL);
	      break;
	      case 5:
		/* contextual glyph insertion */
	      break;
	    }
	}
	fseek(ttf, here+length, SEEK_SET );
    }

return( chain_len );
}

void readttfmort(FILE *ttf,struct ttfinfo *info) {
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
    info->mort_max = nchains*33;		/* Maximum of one feature per bit ? */
    info->feats[0] = galloc((info->mort_max+1)*sizeof(uint32));
    info->feats[0][0] = 0;
    for ( i=0; i<nchains; ++i ) {
	here = ftell(ttf);
	len = readmortchain(ttf,info,base,ismorx);
	fseek(ttf,here+len,SEEK_SET);
    }
}

/* Apple's docs imply that kerning info is always provided left to right, even*/
/*  for right to left scripts. My guess is that their docs are wrong, as they */
/*  often are, but if that be so then we need code in here to reverse */
/*  the order of the characters for right to left since pfaedit's convention */
/*  is to follow writing order rather than to go left to right */
void readttfkerns(FILE *ttf,struct ttfinfo *info) {
    int tabcnt, len, coverage,i,j, npairs, version, format, flags_good;
    int left, right, offset, array, rowWidth;
    int header_size;
    KernPair *kp;
    KernClass *kc;
    uint32 begin_table;
    uint16 *class1, *class2;
    int isv;

    fseek(ttf,info->kern_start,SEEK_SET);
    version = getushort(ttf);
    tabcnt = getushort(ttf);
    if ( version!=0 ) {
	fseek(ttf,info->kern_start,SEEK_SET);
	version = getlong(ttf);
	tabcnt = getlong(ttf);
    }
    for ( i=0; i<tabcnt; ++i ) {
	begin_table = ftell(ttf);
	if ( version==0 ) {
	    /* version = */ getushort(ttf);
	    len = getushort(ttf);
	    coverage = getushort(ttf);
	    format = coverage>>8;
	    flags_good = ((coverage&7)<=1);
	    isv = !(coverage&1);
	    header_size = 6;
	} else {
	    len = getlong(ttf);
	    coverage = getushort(ttf);
	    /* Apple has reordered the bits */
	    format = (coverage&0xff);
	    flags_good = ((coverage&0xff00)==0 || (coverage&0xff00)==0x8000);
	    isv = coverage&0x8000? 1 : 0;
	    /* tupleIndex = */ getushort(ttf);
	    header_size = 8;
	}
	if ( flags_good && format==0 ) {
	    /* format 0, horizontal kerning data (as pairs) not perpendicular */
	    npairs = getushort(ttf);
	    /* searchRange = */ getushort(ttf);
	    /* entrySelector = */ getushort(ttf);
	    /* rangeShift = */ getushort(ttf);
	    for ( j=0; j<npairs; ++j ) {
		left = getushort(ttf);
		right = getushort(ttf);
		offset = (short) getushort(ttf);
		if ( left<info->glyph_cnt && right<info->glyph_cnt ) {
		    kp = chunkalloc(sizeof(KernPair));
		    kp->sc = info->chars[right];
		    kp->off = offset;
		    kp->sli = SLIFromInfo(info,info->chars[left],DEFAULT_LANG);
		    if ( isv ) {
			kp->next = info->chars[left]->vkerns;
			info->chars[left]->vkerns = kp;
		    } else {
			kp->next = info->chars[left]->kerns;
			info->chars[left]->kerns = kp;
		    }
		} else
		    fprintf( stderr, "Bad kern pair glyphs %d & %d must be less than %d\n",
			    left, right, info->glyph_cnt );
	    }
	} else if ( flags_good && format==1 ) {
	    /* format 1 is an apple state machine which can handle weird cases */
	    /*  OpenType's spec doesn't document this */
	    /*  I shan't support it */
	    fseek(ttf,len-header_size,SEEK_CUR);
	    fprintf( stderr, "This font has a format 1 kerning table (a state machine).\nPfaEdit doesn't parse these\nSorry.\n" );
	} else if ( flags_good && format==2 ) {
	    /* format 2, horizontal kerning data (as classes) not perpendicular */
	    /*  OpenType's spec documents this, but says windows won't support it */
	    /*  OpenType's spec also contradicts Apple's as to the data stored */
	    /*  OTF says class indeces are stored, Apple says byte offsets into array */
	    /*  Apple says offsets are stored in uint8, otf says indeces are in uint16 */
	    /* Bleah!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
	    /*  Ah. Apple's docs are incorrect. the values stored are uint16 offsets */
	    rowWidth = getushort(ttf);
	    left = getushort(ttf);
	    right = getushort(ttf);
	    array = getushort(ttf);
	    if ( isv ) {
		if ( info->khead==NULL )
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
	    kc->second_cnt = rowWidth/sizeof(uint16);
	    class1 = getAppleClassTable(ttf, begin_table+left, info->glyph_cnt, array, rowWidth );
	    class2 = getAppleClassTable(ttf, begin_table+right, info->glyph_cnt, 0, sizeof(uint16) );
	    for ( i=0; i<info->glyph_cnt; ++i ) {
		if ( class1[i]!=0 ) {
		    kc->sli = SLIFromInfo(info,info->chars[i],DEFAULT_LANG);
	    break;
		}
	    }
	    for ( i=0; i<info->glyph_cnt; ++i )
		if ( class1[i]>kc->first_cnt )
		    kc->first_cnt = class1[i];
	    ++ kc->first_cnt;
	    kc->offsets = galloc(kc->first_cnt*kc->second_cnt*sizeof(int16));
	    kc->firsts = ClassToNames(info,kc->first_cnt,class1,info->glyph_cnt);
	    kc->seconds = ClassToNames(info,kc->second_cnt,class2,info->glyph_cnt);
	    fseek(ttf,begin_table+array,SEEK_SET);
	    for ( i=0; i<kc->first_cnt*kc->second_cnt; ++i )
		kc->offsets[i] = getushort(ttf);
	    free(class1); free(class2);
	    fseek(ttf,begin_table+len,SEEK_SET);
	} else if ( flags_good && format==3 ) {
	    /* format 3, horizontal kerning data (as classes limited to 256 entries) not perpendicular */
	    /*  OpenType's spec doesn't document this */
	    fseek(ttf,len-header_size,SEEK_CUR);
    fprintf( stderr, "This font has a format 3 kerning table. I've never seen that and don't know\nhow to parse it. Could you send a copy of %s to gww@silcom.com?\nThanks!\n",
	info->fontname );
	} else {
	    fseek(ttf,len-header_size,SEEK_CUR);
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

    fs = galloc(featcnt*sizeof(struct fs));
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
	}
    }
    free(fs);
}
