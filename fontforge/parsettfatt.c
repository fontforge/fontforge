/* Copyright (C) 2000-2004 by George Williams */
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
    info->sli_cnt = j+1;
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
    glist = gcalloc(cnt,sizeof(uint16));	/* Class 0 is default */
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
    } else
	fprintf( stderr, "Unknown class table format: %d\n", format );
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
    if ( i>=info->lookup_cnt ) {
	fprintf( stderr, "Attempt to reference lookup %d (within a contextual lookup), but there are\n only %d lookups in %s\n",
		i, info->lookup_cnt, gpos ? "'GPOS'" : "'GSUB'" );
	sl->lookup_tag = CHR('!','!','!','!');
return;
    }
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
    int warned = false, warned2 = false;

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
		if ( rules[i].subrules[j].sl[k].seq >= rules[i].subrules[j].gcnt+1 )
		    if ( !warned2 ) {
			fprintf( stderr, "Attempt to apply a lookup to a location out of the range of this contextual\n lookup seq=%d max=%d\n",
				rules[i].subrules[j].sl[k].seq, rules[i].subrules[j].gcnt );
			warned2 = true;
		    }
		rules[i].subrules[j].sl[k].lookup_tag = getushort(ttf);
	    }
	}
    }

    if ( justinuse==git_justinuse ) {
	for ( i=0; i<rcnt; ++i ) {
	    for ( j=0; j<rules[i].scnt; ++j ) {
		for ( k=0; k<rules[i].subrules[j].scnt; ++k )
		    if ( rules[i].subrules[j].sl[k].lookup_tag<info->lookup_cnt )
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
    int warned = false, warned2 = false;

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
		if ( rules[i].subrules[j].sl[k].seq >= rules[i].subrules[j].gcnt+1 )
		    if ( !warned2 ) {
			fprintf( stderr, "Attempt to apply a lookup to a location out of the range of this contextual\n lookup seq=%d max=%d\n",
				rules[i].subrules[j].sl[k].seq, rules[i].subrules[j].gcnt );
			warned2 = true;
		    }
		rules[i].subrules[j].sl[k].lookup_tag = getushort(ttf);
	    }
	}
    }

    if ( justinuse==git_justinuse ) {
	for ( i=0; i<rcnt; ++i ) {
	    for ( j=0; j<rules[i].scnt; ++j ) {
		for ( k=0; k<rules[i].subrules[j].scnt; ++k )
		    if ( rules[i].subrules[j].sl[k].lookup_tag<info->lookup_cnt )
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
    int warned2 = false;

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
		if ( rules[i].subrules[j].sl[k].seq >= rules[i].subrules[j].ccnt )
		    if ( !warned2 ) {
			fprintf( stderr, "Attempt to apply a lookup to a location out of the range of this contextual\n lookup seq=%d max=%d\n",
				rules[i].subrules[j].sl[k].seq, rules[i].subrules[j].ccnt-1);
			warned2 = true;
		    }
		rules[i].subrules[j].sl[k].lookup_tag = getushort(ttf);
	    }
	}
    }

    if ( justinuse==git_justinuse ) {
	for ( i=0; i<rcnt; ++i ) {
	    for ( j=0; j<rules[i].scnt; ++j ) {
		for ( k=0; k<rules[i].subrules[j].scnt; ++k )
		    if ( rules[i].subrules[j].sl[k].lookup_tag<info->lookup_cnt )
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
    int warned2 = false;

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
		if ( rules[i].subrules[j].sl[k].seq >= rules[i].subrules[j].ccnt )
		    if ( !warned2 ) {
			fprintf( stderr, "Attempt to apply a lookup to a location out of the range of this contextual\n lookup seq=%d max=%d\n",
				rules[i].subrules[j].sl[k].seq, rules[i].subrules[j].ccnt-1);
			warned2 = true;
		    }
		rules[i].subrules[j].sl[k].lookup_tag = getushort(ttf);
	    }
	}
    }

    if ( justinuse==git_justinuse ) {
	for ( i=0; i<rcnt; ++i ) {
	    for ( j=0; j<rules[i].scnt; ++j ) {
		for ( k=0; k<rules[i].subrules[j].scnt; ++k )
		    if ( rules[i].subrules[j].sl[k].lookup_tag<info->lookup_cnt )
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
    int warned2 = false;

    gcnt = getushort(ttf);
    scnt = getushort(ttf);
    coverage = galloc(gcnt*sizeof(uint16));
    for ( i=0; i<gcnt; ++i )
	coverage[i] = getushort(ttf);
    sl = galloc(scnt*sizeof(struct seqlookup));
    for ( k=0; k<scnt; ++k ) {
	sl[k].seq = getushort(ttf);
	if ( sl[k].seq >= gcnt && !warned2 ) {
	    fprintf( stderr, "Attempt to apply a lookup to a location out of the range of this contextual\n lookup seq=%d, max=%d\n",
		    sl[k].seq, gcnt-1 );
	    warned2 = true;
	}
	sl[k].lookup_tag = getushort(ttf);
    }

    if ( justinuse==git_justinuse ) {
	for ( k=0; k<scnt; ++k )
	    if ( sl[k].lookup_tag<info->lookup_cnt )
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
    int warned2 = false;

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
	if ( sl[k].seq >= gcnt && !warned2 ) {
	    fprintf( stderr, "Attempt to apply a lookup to a location out of the range of this contextual\n lookup seq=%d, max=%d\n",
		    sl[k].seq, gcnt-1 );
	    warned2 = true;
	}
	sl[k].lookup_tag = getushort(ttf);
    }

    if ( justinuse==git_justinuse ) {
	for ( k=0; k<scnt; ++k )
	    if ( sl[k].lookup_tag<info->lookup_cnt )
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
		    char *str;
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
	    info->sli_cnt = j+1;
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
    if ( fseek(ttf,lookup->offset,SEEK_SET)==-1 ) {
	fprintf( stderr, "Bad seek when processing lookup\n" );
return;
    }
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

    info->lookup_cnt = lu_cnt = getushort(ttf);
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
    int lclo, gclass;
    int coverage, cnt, i,j, format;
    uint16 *glyphs, *lc_offsets, *offsets;
    uint32 caret_base;
    PST *pst;
    SplineChar *sc;

    fseek(ttf,info->gdef_start,SEEK_SET);
    if ( getlong(ttf)!=0x00010000 )
return;
    gclass = getushort(ttf);
    /* attach list = */ getushort(ttf);
    lclo = getushort(ttf);		/* ligature caret list */
    /* mark attach class = */ getushort(ttf);

    if ( gclass!=0 ) {
	uint16 *gclasses = getClassDefTable(ttf,info->gdef_start+gclass,info->glyph_cnt);
	for ( i=0; i<info->glyph_cnt; ++i )
	    if ( info->chars[i]!=NULL && gclasses[i]!=0 )
		info->chars[i]->glyph_class = gclasses[i]+1;
	free(gclasses);
    }

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
	    if ( last<first || last>=info->glyph_cnt ) {
		fprintf( stderr, "Bad lookup table format=2 (%d/%d), first=%d last=%d total glyphs in font=%d\n",
			i,cnt,first,last,info->glyph_cnt );
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
	    if ( last<first || last>=info->glyph_cnt ) {
		fprintf( stderr, "Bad lookup table format=4 (%d/%d), first=%d last=%d total glyphs in font=%d\n",
			i,cnt,first,last,info->glyph_cnt );
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
	    if ( first>=info->glyph_cnt ) {
		fprintf( stderr, "Bad lookup table format=6, first=%d total glyphs in font=%d\n",
			first,info->glyph_cnt );
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
	if ( first+cnt>=info->glyph_cnt ) {
	    fprintf( stderr, "Bad lookup table format=8, first=%d cnt=%d total glyphs in font=%d\n",
		    first,cnt,info->glyph_cnt );
	} else {
	    if ( apply_default!=NULL ) {
		apply_default(info,0,first-1,def);
		apply_default(info,first+cnt,info->glyph_cnt-1,def);
	    }
	    apply_values(info,first,first+cnt-1,ttf);
	}
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
	fprintf( stderr, "Glyph out of bounds in 'mort'/'morx' table %d\n", gnum );
return;
    } else if ( gsubs<0 || (gsubs>=info->glyph_cnt && gsubs!=0xffff)) {
	fprintf( stderr, "Substitute glyph out of bounds in 'mort'/'morx' table %d\n", gsubs );
return;
    } else if ( (sc=info->chars[gnum])==NULL ||
	    (gsubs!=0xffff && (ssc=info->chars[gsubs])==NULL) )
return;

    pst = chunkalloc(sizeof(PST));
    pst->type = pst_substitution;
    pst->tag = info->mort_subs_tag;
    pst->macfeature = info->mort_tag_mac;
    pst->flags = info->mort_r2l ? pst_r2l : 0;
    pst->script_lang_index = info->mort_is_nested ? SLI_NESTED :
	    SLIFromInfo(info,sc,DEFAULT_LANG);
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

int32 memlong(uint8 *data,int offset) {
    int ch1 = data[offset], ch2 = data[offset+1], ch3 = data[offset+2], ch4 = data[offset+3];
return( (ch1<<24)|(ch2<<16)|(ch3<<8)|ch4 );
}

int memushort(uint8 *data,int offset) {
    int ch1 = data[offset], ch2 = data[offset+1];
    if ( offset<0 )
	fprintf( stderr, "Internal error\n" );
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
		fprintf(stderr, "Attempt to make a ligature for glyph %d out of ",
			lig_glyph );
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
		    pst->macfeature = sm->info->mort_tag_mac;
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
		fprintf(stderr, "Attempt to make a ligature for glyph %d out of ",
			lig_glyph );
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
		    pst->macfeature = sm->info->mort_tag_mac;
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
    struct statetable *st = gcalloc(1,sizeof(struct statetable));
    uint32 here = ftell(ttf);
    int nclasses, class_off, state_off, entry_off;
    int state_max, ent_max, old_state_max, old_ent_max;
    int i, j, ent, new_state, ent_size;

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
    if ( ismorx ) {
	st->classes2 = info->morx_classes = galloc(info->glyph_cnt*sizeof(uint16));
	for ( i=0; i<info->glyph_cnt; ++i )
	    st->classes2[i] = 1;			/* Out of bounds */
	readttf_applelookup(ttf,info,
		mortclass_apply_values,mortclass_apply_value,NULL,NULL);
    } else {
	st->first_glyph = getushort(ttf);
	st->nglyphs = getushort(ttf);
	st->classes = galloc(st->nglyphs);
	fread(st->classes,1,st->nglyphs,ttf);
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
	    fprintf( stderr, "It looks to me as though there's a morx sub-table with more than 1000\n transitions. Which makes me think there's probably an error\n" );
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
	    fprintf( stderr, "It looks to me as though there's a morx sub-table with more than 1000\n states. Which makes me think there's probably an error\n" );
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
	st->state_table2 = galloc(st->nstates*st->nclasses*sizeof(uint16));
	for ( i=0; i<st->nstates*st->nclasses; ++i )
	    st->state_table2[i] = getushort(ttf);
    } else {
	st->state_table = galloc(st->nstates*st->nclasses);
	fread(st->state_table,1,st->nstates*st->nclasses,ttf);
    }

	/* parse the entry subtable */
    fseek(ttf,here+entry_off,SEEK_SET);
    st->transitions = galloc(st->nentries*st->entry_size);
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

static char **ClassesFromStateTable(struct statetable *st,int ismorx,struct ttfinfo *info) {
    /* On the mac the first four classes should be left blank. only class 1 */
    /*  (out of bounds) is supposed to be used in the class array anyway */
    char **classes = galloc(st->nclasses*sizeof(char *));
    int *lens = gcalloc(st->nclasses,sizeof(int));
    int i;

    if ( ismorx ) {
	for ( i=0; i<info->glyph_cnt; ++i )
	    lens[st->classes2[i]] += strlen( info->chars[i]->name )+1;
    } else {
	for ( i=st->first_glyph; i<st->first_glyph+st->nglyphs && i<info->glyph_cnt; ++i )
	    lens[st->classes[i-st->first_glyph]] += strlen( info->chars[i]->name )+1;
    }
    classes[0] = classes[1] = classes[2] = classes[3] = NULL;
    for ( i=4; i<st->nclasses; ++i ) {
	classes[i] = galloc(lens[i]+1);
	*classes[i] = '\0';
    }
    if ( ismorx ) {
	for ( i=0; i<info->glyph_cnt; ++i ) if ( st->classes2[i]>=4 ) {
	    strcat(classes[st->classes2[i]],info->chars[i]->name );
	    strcat(classes[st->classes2[i]]," ");
	}
    } else {
	for ( i=st->first_glyph; i<st->first_glyph+st->nglyphs && i<info->glyph_cnt; ++i ) if ( st->classes[i-st->first_glyph]>=4 ) {
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
    str = galloc(len+1);
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

#if 0
static void RunStateFindKernDepth_(ASM *as,int state,int kdepth,uint8 *used) {
    int j, kd;

    if ( used[state] )
return;
    used[state] = true;

    for ( j=0; j<as->class_cnt; ++j ) {
	kd = kdepth;
	flags = as->state[state*as->class_cnt+j];
	if ( flags&0x8000 )
	    ++kd;
	if ( (flags&0x3fff)!=0 ) {
	    as->state[state*as->class_cnt+j].u.kern.kcnt = kd;
	    kd = 0;
	}
	RunStateFindKernDepth_(as,as->state[state*as->class_cnt+j].next_state,kd,used);
    }
}

static void RunStateFindKernDepth(ASM *as) {
    uint8 *used = gcalloc(as->class_cnt);
    int i;

    for ( i=0; i<as->class_cnt*as->state_cnt; ++i ) {
	as->state[i].u.kern.kerns = NULL;
	as->state[i].u.kern.kcnt = (as->state[i].flags&0x3fff)==0 ? 0 : -1;
    }
    RunStateFindKernDepth_(as,0,0,used);
    RunStateFindKernDepth_(as,1,0,used);
}
#endif

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
	trans->u.kern.kerns = galloc(i*sizeof(int16));
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

static uint32 TagFromInfo(struct ttfinfo *info,int i) {
    char buf[8];
    uint32 tag;

    sprintf( buf, "M%03d", info->gentags.tt_cur + i );
    if ( info->gentags.tt_cur + i >= info->gentags.tt_max ) {
	if ( info->gentags.tt_max==0 )
	    info->gentags.tagtype = galloc((info->gentags.tt_max=30)*sizeof(struct tagtype));
	else
	    info->gentags.tagtype = grealloc(info->gentags.tagtype,(info->gentags.tt_max+=30)*sizeof(struct tagtype));
    }
    info->gentags.tagtype[info->gentags.tt_cur+i].type = pst_substitution;
    tag = CHR(buf[0], buf[1], buf[2], buf[3] );
    info->gentags.tagtype[info->gentags.tt_cur+i].tag = tag;
return( tag );
}

static ASM *readttf_mortx_asm(FILE *ttf,struct ttfinfo *info,int ismorx,
	uint32 subtab_len,enum asm_type type,int extras,
	uint32 coverage) {
    struct statetable *st;
    ASM *as;
    int i,j;
    uint32 here = ftell(ttf);

    st = read_statetable(ttf,extras,ismorx,info);
    if ( st==NULL )
return(NULL);

    as = chunkalloc(sizeof(ASM));
    as->type = type;
    as->feature = info->mort_feat; as->setting = info->mort_setting;
    as->flags = coverage>>16;
    as->class_cnt = st->nclasses;
    as->state_cnt = st->nstates;
    as->classes = ClassesFromStateTable(st,ismorx,info);
    as->state = galloc(st->nclasses*st->nstates*sizeof(struct asm_state));
    for ( i=0; i<st->nclasses*st->nstates; ++i ) {
	int trans;
	if ( ismorx ) {
	    trans = st->state_table2[i];
	    as->state[i].next_state = memushort(st->transitions,trans*st->entry_size);
	} else {
	    trans = st->state_table[i];
	    as->state[i].next_state = (memushort(st->transitions,trans*st->entry_size)-st->state_offset)/st->nclasses;
	}
	as->state[i].flags = memushort(st->transitions,trans*st->entry_size+2);
	if ( extras>0 )
	    as->state[i].u.context.mark_tag = memushort(st->transitions,trans*st->entry_size+2+2);
	if ( extras>1 )
	    as->state[i].u.context.cur_tag = memushort(st->transitions,trans*st->entry_size+2+2+2);
    }
    /* Indic tables have no attached subtables, just a verb in the flag field */
    /*  so for them we are done. For the others... */
    if ( !ismorx && type==asm_insert ) {
	for ( i=0; i<st->nclasses*st->nstates; ++i ) {
	    char *cur=NULL, *mark=NULL;
	    if ( (as->state[i].flags&0x3e0)!=0 && as->state[i].u.context.mark_tag!=0 ) {
		cur = NamesOfList(here+as->state[i].u.context.mark_tag,
			(as->state[i].flags&0x3e0)>>5,ttf,info);
	    }
	    if ( (as->state[i].flags&0x01f)!=0 && as->state[i].u.context.cur_tag!=0 ) {
		mark = NamesOfList(here+as->state[i].u.context.cur_tag,
			as->state[i].flags&0x01f,ttf,info);
	    }
	    as->state[i].u.insert.cur_ins=cur;
	    as->state[i].u.insert.mark_ins=mark;
	}
    } else if ( ismorx && type == asm_insert ) {
	for ( i=0; i<st->nclasses*st->nstates; ++i ) {
	    char *cur=NULL, *mark=NULL;
	    if ( (as->state[i].flags&0x3e0)!=0 && as->state[i].u.context.mark_tag!=0xffff ) {
		cur = NamesOfList(here+st->extra_offsets[0]+as->state[i].u.context.mark_tag*2,
			(as->state[i].flags&0x3e0)>>5,ttf,info);
	    }
	    if ( (as->state[i].flags&0x01f)!=0 && as->state[i].u.context.cur_tag!=0xffff ) {
		mark = NamesOfList(here+st->extra_offsets[0]+as->state[i].u.context.cur_tag*2,
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
	uint8 *classes_subbed = gcalloc(st->nclasses,1);
	int lookup_max = -1, index;
	int32 *lookups = galloc(st->nclasses*st->nstates*sizeof(int32));
	uint8 *evermarked = gcalloc(st->nclasses*st->nstates,sizeof(uint8));
	uint8 *used;

	for ( i=0; i<st->nclasses*st->nstates; ++i ) {
	    if ( as->state[i].u.context.mark_tag!=0 ) {
		index = sm_lookupfind(lookups,&lookup_max,(int16) as->state[i].u.context.mark_tag);
		evermarked[index] = true;
		as->state[i].u.context.mark_tag = TagFromInfo(info,index);
	    }
	    if ( as->state[i].u.context.cur_tag!=0 ) {
		index = sm_lookupfind(lookups,&lookup_max,(int16) as->state[i].u.context.cur_tag);
		as->state[i].u.context.cur_tag = TagFromInfo(info,index);
	    }
	}
	used = gcalloc((subtab_len-st->extra_offsets[0])/2,sizeof(uint8));
	/* first figure things that only appear in current subs */
	/*  then go back and work on things that apply to things which are also in marked subs */
	for ( j=0; j<2; ++j ) for ( i=0; i<=lookup_max; ++i ) if ( evermarked[i]==j ) {
	    info->mort_subs_tag = TagFromInfo(info,i);
	    info->mort_is_nested = true;
	    if ( !evermarked[i] ) { int k,l;
		memset(classes_subbed,0,st->nclasses);
		for ( k=0; k<st->nstates; ++k ) for ( l=0; l<st->nclasses; ++l ) {
		    if ( as->state[k*st->nclasses+l].u.context.cur_tag == info->mort_subs_tag )
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
	info->gentags.tt_cur += lookup_max+1;
    } else if ( ismorx && type == asm_context ) {
	int lookup_max= -1;
	uint32 *lookups;
	for ( i=0; i<st->nclasses*st->nstates; ++i ) {
	    if ( as->state[i].u.context.mark_tag!=0xffff ) {
		if ( ((int) as->state[i].u.context.mark_tag)>lookup_max )
		    lookup_max = as->state[i].u.context.mark_tag;
		as->state[i].u.context.mark_tag = TagFromInfo(info,as->state[i].u.context.mark_tag);
	    } else
		as->state[i].u.context.mark_tag=0;
	    if ( as->state[i].u.context.cur_tag!=0xffff ) {
		if ( ((int) as->state[i].u.context.cur_tag)>lookup_max )
		    lookup_max = as->state[i].u.context.cur_tag;
		as->state[i].u.context.cur_tag = TagFromInfo(info,as->state[i].u.context.cur_tag);
	    } else
		as->state[i].u.context.cur_tag = 0;
	}
	++lookup_max;
	lookups = galloc(lookup_max*sizeof(uint32));
	fseek(ttf,here+st->extra_offsets[0],SEEK_SET);
	for ( i=0; i<lookup_max; ++i )
	    lookups[i] = getlong(ttf) + here+st->extra_offsets[0];
	for ( i=0; i<lookup_max; ++i ) {
	    fseek(ttf,lookups[i],SEEK_SET);
	    info->mort_subs_tag = TagFromInfo(info,i);
	    info->mort_is_nested = true;
	    readttf_applelookup(ttf,info,
		    mort_apply_values,mort_apply_value,NULL,NULL);
	}
	info->mort_is_nested = false;
	free(lookups);
	info->gentags.tt_cur += lookup_max;
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

static void FeatMarkAsEnabled(struct ttfinfo *info,int featureType,
	int featureSetting);

static uint32 readmortchain(FILE *ttf,struct ttfinfo *info, uint32 base, int ismorx) {
    uint32 chain_len, nfeatures, nsubtables, default_flags;
    uint32 enable_flags, disable_flags, flags;
    int featureType, featureSetting;
    int i,j,k,l;
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
	if ( enable_flags & default_flags )
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
	    info->mort_subs_tag = tmf[j].tag;
	    info->mort_r2l = r2l;
	    info->mort_tag_mac = tmf[j].ismac;
	    info->mort_feat = tmf[j].feat; info->mort_setting = tmf[j].set;
	    for ( l=0; info->feats[0][l]!=0; ++l )
		if ( info->feats[0][l]==tmf[j].tag )
	    break;
	    if ( info->feats[0][l]==0 && l<info->mort_max ) {
		info->feats[0][l] = tmf[j].tag;
		info->feats[0][l+1] = 0;
	    }
	    switch( coverage&0xff ) {
	      case 0:	/* Indic rearangement */
		readttf_mortx_asm(ttf,info,ismorx,length,asm_indic,0,
			coverage);
	      break;
	      case 1:	/* contextual glyph substitution */
		readttf_mortx_asm(ttf,info,ismorx,length,asm_context,2,
			coverage);
	      break;
	      case 2:	/* ligature substitution */
		/* Apple's ligature state machines are too weird to be */
		/*  represented easily, but I can parse them into a set */
		/*  of ligatures -- assuming they are unconditional */
		readttf_mortx_lig(ttf,info,ismorx,here,length);
	      break;
	      case 4:	/* non-contextual glyph substitutions */
		readttf_applelookup(ttf,info,
			mort_apply_values,mort_apply_value,NULL,NULL);
	      break;
	      case 5:	/* contextual glyph insertion */
		readttf_mortx_asm(ttf,info,ismorx,length,asm_insert,2,
			coverage);
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

    fseek(ttf,info->kern_start,SEEK_SET);
    version = getushort(ttf);
    tabcnt = getushort(ttf);
    if ( version!=0 ) {
	fseek(ttf,info->kern_start,SEEK_SET);
	version = getlong(ttf);
	tabcnt = getlong(ttf);
    }
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
	if ( flags_good && format==0 ) {
	    /* format 0, horizontal kerning data (as pairs) not perpendicular */
	    chars = tupleIndex==-1 ? info->chars : info->variations->tuples[tupleIndex].chars;
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
		    kp->sc = chars[right];
		    kp->off = offset;
		    kp->sli = SLIFromInfo(info,chars[left],DEFAULT_LANG);
		    if ( isv ) {
			kp->next = chars[left]->vkerns;
			chars[left]->vkerns = kp;
		    } else {
			kp->next = chars[left]->kerns;
			chars[left]->kerns = kp;
		    }
		} else
		    fprintf( stderr, "Bad kern pair glyphs %d & %d must be less than %d\n",
			    left, right, info->glyph_cnt );
	    }
	} else if ( flags_good && format==1 ) {
	    /* format 1 is an apple state machine which can handle weird cases */
	    /*  OpenType's spec doesn't document this */
	    /* Apple's docs are wrong about this table, they claim */
	    /*  there is a special value which marks the end of the kerning */
	    /*  lists. In fact there is no such value, the list is as long */
	    /*  as there are things on the kern stack */
#if 1
	    readttf_mortx_asm(ttf,info,false,len-header_size,asm_kern,0,
		isv ? 0x80000000 : 0 /* coverage doesn't really apply */);
	    fseek(ttf,begin_table+len,SEEK_SET);
#else
	    fseek(ttf,len-header_size,SEEK_CUR);
	    fprintf( stderr, "This font has a format 1 kerning table (a state machine).\nPfaEdit doesn't parse these\nCould you send a copy of %s to gww@silcom.com?  Thanks.\n",
		info->fontname );
#endif
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
		class1 = getAppleClassTable(ttf, begin_table+left, info->glyph_cnt, array, rowWidth );
		class2 = getAppleClassTable(ttf, begin_table+right, info->glyph_cnt, 0, sizeof(uint16) );
		for ( i=0; i<info->glyph_cnt; ++i )
		    if ( class1[i]>kc->first_cnt )
			kc->first_cnt = class1[i];
		++ kc->first_cnt;
		kc->offsets = galloc(kc->first_cnt*kc->second_cnt*sizeof(int16));
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
		if ( gc>info->glyph_cnt )
		    fprintf( stderr, "Kerning subtable 3 says the glyph count is %d, but maxp says %d\n",
			    gc, info->glyph_cnt );
		class1 = gcalloc(gc>info->glyph_cnt?gc:info->glyph_cnt,sizeof(uint16));
		class2 = gcalloc(gc>info->glyph_cnt?gc:info->glyph_cnt,sizeof(uint16));
		kvs = galloc(kv*sizeof(int16));
		kc->offsets = galloc(kc->first_cnt*kc->second_cnt*sizeof(int16));
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
		if ( class1[i]!=0 ) {
		    kc->sli = SLIFromInfo(info,info->chars[i],DEFAULT_LANG);
	    break;
		}
	    }
	    free(class1); free(class2);
	    fseek(ttf,begin_table+len,SEEK_SET);
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
