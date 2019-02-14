/* Copyright (C) 2000-2012 by George Williams */
/* Copyright (C) 2012-2013 by Khaled Hosny */
/* Copyright (C) 2013 by Matthew Skala */
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
#include <fontforge-config.h>

#include "featurefile.h"

#include "encoding.h"
#include "fontforgevw.h"
#include "fvfonts.h"
#include "lookups.h"
#include "namelist.h"
#include "ttf.h"
#include "splineutil.h"
#include "tottf.h"
#include "tottfgpos.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#ifdef __need_size_t
/* This is a bug on the mac, someone defines this and leaves it defined */
/*  that means when I load stddef.h it only defines size_t and doesn't */
/*  do offset_of, which is what I need */
# undef __need_size_t
#endif
#include <stddef.h>
#include <string.h>
#include <utype.h>
#include <ustring.h>
#include <locale.h>

#include <ffglib.h>

/* Adobe's opentype feature file */
/* Which suffers incompatible changes according to Adobe's whim */
/* Currently trying to support the version of december 2008, Version 1.8. */

/* Ambiguities */
/* In section 4.d we are told that MarkAttachmentType/UseMarkFilteringSet */
/*  both take <glyph class name>s, yet in 9.b we are told they take */
/*  <mark class name>s */
/*   Read Roberts says that either will be accepted */
/* In 6.e a NULL anchor is defined by <anchorNULL>, shouldn't this be */
/*  <anchor NULL>? In that same example the NULL anchor is not followed by */
/*  a mark keyword and class, is this correct syntax? */
/*   Read Roberts says <anchorNULL> is a typo, but that not having a mark class*/
/*   is correct */
/* 2.e.vi The syntax for contourpoints has changed. In v1.6 it was */
/*  "<contourpoint 2>", but in v1.8 it is "contourpoint 2". Yet this change */
/*  is not mentioned in the v1.8 changelog, was it intentional? */

/* Adobe says glyph names are 31 chars, but Mangal and Source Code Sans use longer names. */
#define MAXG	63

/* ************************************************************************** */
/* ******************************* Output feat ****************************** */
/* ************************************************************************** */

/* Do GPOS and GSUB features have to share the same feature statement? */

static void dump_ascii(FILE *out, char *name) {
    /* Strip out any characters which don't fit in a name token */
    while ( *name ) {
	if ( *name==' ' )
	    putc('_',out);
	else if ( *name&0x80 )
	    /* Skip */;
	else if ( isalnum(*name) || *name=='.' || *name=='_' )
	    putc(*name,out);
	++name;
    }
}

static void dump_glyphname(FILE *out, SplineChar *sc) {
    if ( sc->parent->cidmaster!=NULL )
	fprintf( out, "\\%d", sc->orig_pos /* That's the CID */ );
    else
	fprintf( out, "\\%s", sc->name );
}

static void dump_glyphbyname(FILE *out, SplineFont *sf, char *name) {
    SplineChar *sc = SFGetChar(sf,-1,name);

    if ( sc==NULL )
	LogError(_("No glyph named %s."), name );
    if ( sc==NULL || sc->parent->cidmaster==NULL )
	fprintf( out, "\\%s", name );
    else
	fprintf( out, "\\%s", sc->name );
}

static void dump_glyphnamelist(FILE *out, SplineFont *sf, char *names) {
    char *pt, *start;
    int ch;
    SplineChar *sc2;
    int len=0;
    char cidbuf[20], *nm;

    if ( sf->subfontcnt==0 ) {
	for ( pt=names; ; ) {
	    while ( *pt==' ' ) ++pt;
	    if ( *pt=='\0' )
	break;
	    for ( start=pt; *pt!=' ' && *pt!='\0'; ++pt );
	    ch = *pt; *pt = '\0';
	    if ( pt-start+len+1 >72 ) {
		fprintf( out, "\n\t" );
		len = 8;
	    }
	    fprintf( out, "\\%s ", start );
	    len += strlen(start)+1;
	    *pt = ch;
	}
    } else {
	for ( pt=names; ; ) {
	    while ( *pt==' ' ) ++pt;
	    if ( *pt=='\0' )
	break;
	    for ( start=pt; *pt!=' ' && *pt!='\0'; ++pt );
	    ch = *pt; *pt = '\0';
	    sc2 = SFGetChar(sf,-1,start);
	    if ( sc2==NULL ) {
		LogError(_("No CID named %s"), start);
		nm = start;
	    } else {
		sprintf( cidbuf, "\\%d", sc2->orig_pos );
		nm = cidbuf;
	    }
	    if ( strlen(nm)+len+1 >72 ) {
		fprintf( out, "\n\t" );
		len = 8;
	    }
	    fprintf( out, "%s ", nm );
	    len += strlen(nm)+1;
	    *pt = ch;
	}
    }
}

static int MarkNeeded(uint8 *needed,uint8 *setsneeded,OTLookup *otl) {
    int index = (otl->lookup_flags>>8)&0xff;
    int sindex = (otl->lookup_flags>>16)&0xffff;
    int any = false;
    struct lookup_subtable *sub;
    int i,l;

    if ( index!=0 ) {
	any |= 1;
	needed[index] = true;
    }
    if ( otl->lookup_flags&pst_usemarkfilteringset ) {
	any |= 2;
	setsneeded[sindex] = true;
    }
    for ( sub = otl->subtables; sub!=NULL; sub=sub->next ) {
	if ( sub->fpst!=NULL ) {
	    for ( i=sub->fpst->rule_cnt-1; i>=0; --i ) {
		struct fpst_rule *r = &sub->fpst->rules[i];
		for ( l=0; l<r->lookup_cnt; ++l )
		    any |= MarkNeeded(needed,setsneeded,r->lookups[l].lookup);
	    }
	}
    }
return( any );
}

static void gdef_markclasscheck(FILE *out,SplineFont *sf,OTLookup *otl) {
    uint8 *needed;
    uint8 *setsneeded;
    int any = false;
    int gpos;

    if ( sf->mark_class_cnt==0 && sf->mark_set_cnt==0 )
return;

    needed = calloc(sf->mark_class_cnt,1);
    setsneeded = calloc(sf->mark_set_cnt,1);
    if ( otl!=NULL ) {
	any = MarkNeeded(needed,setsneeded,otl);
    } else {
	for ( gpos=0; gpos<2; ++gpos ) {
	    for ( otl=gpos?sf->gpos_lookups:sf->gsub_lookups; otl!=NULL; otl=otl->next ) {
		int index = (otl->lookup_flags>>8)&0xff;
		int sindex = (otl->lookup_flags>>16)&0xffff;
		if ( index!=0 ) {
		    any |= 1;
		    needed[index] = true;
		}
		if ( otl->lookup_flags&pst_usemarkfilteringset ) {
		    any |= 2;
		    setsneeded[sindex] = true;
		}
	    }
	}
    }
    if ( any&1 ) {
	int i;
	fprintf( out, "# GDEF Mark Attachment Classes\n" );
	for ( i=1; i<sf->mark_class_cnt; ++i ) if ( needed[i] ) {
	    putc( '@',out );
	    dump_ascii( out, sf->mark_class_names[i]);
	    putc( '=',out );
	    putc( '[',out );
	    dump_glyphnamelist(out,sf,sf->mark_classes[i]);
	    fprintf( out, "];\n" );
	}
	fprintf( out,"\n" );
    }
    if ( any&2 ) {
	int i;
	fprintf( out, "# GDEF Mark Attachment Sets\n" );
	for ( i=0; i<sf->mark_set_cnt; ++i ) if ( setsneeded[i] ) {
	    putc( '@',out );
	    dump_ascii( out, sf->mark_set_names[i]);
	    putc( '=',out );
	    putc( '[',out );
	    dump_glyphnamelist(out,sf,sf->mark_sets[i]);
	    fprintf( out, "];\n" );
	}
	fprintf( out,"\n" );
    }
    free(needed);
    free(setsneeded);
}

static void dump_fpst_everythingelse(FILE *out, SplineFont *sf,char **classes,
	int ccnt, OTLookup *otl) {
    /* Every now and then we need to dump out class 0, the "Everything Else" */
    /*  Opentype class. So... find all the glyphs which aren't listed in one */
    /*  of the other classes, and dump 'em. Even less frequently we will be  */
    /*  asked to apply a transformation to those elements, so we must dump   */
    /*  out the transformed things in the same order */
    int gid, i, k, ch;
    char *pt, *start;
    const char *text;
    SplineFont *sub;
    SplineChar *sc;
    PST *pst;
    int len = 8;

    if ( sf->subfontcnt==0 ) {
	for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( (sc=sf->glyphs[gid])!=NULL)
	    sc->ticked = false;
    } else {
	for ( k=0; k<sf->subfontcnt; ++k ) {
	    sub = sf->subfonts[k];
	    for ( gid=0; gid<sub->glyphcnt; ++gid ) if ( (sc=sub->glyphs[gid])!=NULL)
		sc->ticked = false;
	}
    }

    for ( i=1; i<ccnt; ++i ) {
	for ( pt=classes[i] ; *pt ; ) {
	    while ( *pt==' ' ) ++pt;
	    start = pt;
	    while ( *pt!=' ' && *pt!='\0' ) ++pt;
	    if ( pt==start )
	break;
	    ch = *pt; *pt = '\0';
	    sc = SFGetChar(sf,-1,start);
	    if ( sc!=NULL )
		sc->ticked = true;
	    *pt = ch;
	}
    }

    if ( sf->subfontcnt==0 ) {
	for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( (sc=sf->glyphs[gid])!=NULL && !sc->ticked ) {
	    if ( otl!=NULL ) {
		for ( pst=sc->possub; pst!=NULL; pst=pst->next )
		    if ( pst->subtable!=NULL && pst->subtable->lookup == otl )
		break;
		if ( pst!=NULL )
		    text = pst->u.subs.variant;
		else
		    text = NULL;
	    } else
		text = sc->name;
	    if ( (text==NULL && len+6 > 72 ) ||
		    ( text!=NULL && strlen(text)+len+2 > 72 )) {
		fprintf( out, "\n\t" );
		len = 8;
	    }
	    if ( text==NULL ) {
		fprintf( out, "NULL " );
		len += 5;
	    } else {
		fprintf( out, "\\%s ", text );
		len += strlen(text)+2;
	    }
	}
    } else {
	for ( k=0; k<sf->subfontcnt; ++k ) {
	    sub = sf->subfonts[k];
	    for ( gid=0; gid<sub->glyphcnt; ++gid ) if ( (sc=sub->glyphs[gid])!=NULL && !sc->ticked ) {
		if ( otl!=NULL ) {
		    for ( pst=sc->possub; pst!=NULL; pst=pst->next )
			if ( pst->subtable!=NULL && pst->subtable->lookup == otl )
		    break;
		    if ( pst!=NULL )
			sc = SFGetChar(sf,-1,pst->u.subs.variant);
		    else
			sc = NULL;
		}
		if ( len+8> 76 ) {
		    fprintf( out, "\n\t" );
		    len = 8;
		}
		if ( sc!=NULL ) {
		    fprintf( out, "\\%d ", sc->orig_pos );
		    len += 8;
		} else {
		    fprintf( out, "NULL " );
		    len += 5;
		}
	    }
	}
    }
}

static char *lookupname(OTLookup *otl) {
    char *pt1, *pt2;
    static char space[MAXG+1];

    if ( otl->tempname != NULL )
return( otl->tempname );

    for ( pt1=otl->lookup_name,pt2=space; *pt1 && pt2<space+MAXG; ++pt1 ) {
	if ( !(*pt1&0x80) && (isalpha(*pt1) || *pt1=='_' || *pt1=='.' ||
		(pt1!=otl->lookup_name && isdigit(*pt1))))
	    *pt2++ = *pt1;
    }
    *pt2 = '\0';
return( space );
}

static void dumpdevice(FILE *out,DeviceTable *devtab) {
    int i, any = false;

    fprintf( out, "<device " );
    if ( devtab!=NULL && devtab->corrections!=NULL ) {
	for ( i=devtab->first_pixel_size; i<=devtab->last_pixel_size; ++i ) if ( devtab->corrections[i-devtab->first_pixel_size]!=0 ) {
	    if ( any )
		putc(',',out);
	    else
		any = true;
	    fprintf( out, "%d %d", i, devtab->corrections[i-devtab->first_pixel_size]);
	}
    }
    if ( any )
	fprintf( out, ">" );
    else
	fprintf( out, "NULL>" );
}

static void dump_valuerecord(FILE *out, struct vr *vr) {
    fprintf( out, "<%d %d %d %d", vr->xoff, vr->yoff, vr->h_adv_off, vr->v_adv_off );
    if ( vr->adjust!=NULL ) {
	putc( ' ', out);
	dumpdevice(out,&vr->adjust->xadjust);
	putc( ' ', out);
	dumpdevice(out,&vr->adjust->yadjust);
	putc( ' ', out);
	dumpdevice(out,&vr->adjust->xadv);
	putc( ' ', out);
	dumpdevice(out,&vr->adjust->yadv);
    }
    putc('>',out);
}

static void dump_anchorpoint(FILE *out,AnchorPoint *ap) {

    if ( ap==NULL ) {
	fprintf( out, "<anchor NULL>" );
return;
    }

    fprintf( out, "<anchor %g %g", rint(ap->me.x), rint(ap->me.y) );
    if ( ap->has_ttf_pt )
	fprintf( out, " contourpoint %d", ap->ttf_pt_index );
    else if ( ap->xadjust.corrections!=NULL || ap->yadjust.corrections!=NULL ) {
	putc(' ',out);
	dumpdevice(out,&ap->xadjust);
	putc(' ',out);
	dumpdevice(out,&ap->yadjust);
    }
    putc('>',out);
}

static int kernclass_for_feature_file(struct splinefont *sf, struct kernclass *kc, int flags) {
  // Note that this is not a complete logical inverse of sister function kernclass_for_groups_plist.
  return ((flags & FF_KERNCLASS_FLAG_FEATURE) ||
  (!(flags & FF_KERNCLASS_FLAG_NATIVE) && (kc->feature || sf->preferred_kerning != 1)));
}

static void dump_kernclass(FILE *out,SplineFont *sf,struct lookup_subtable *sub) {
    int i,j;
    KernClass *kc = sub->kc;

    // We only export classes and rules here that have not been emitted in groups.plist and kerning.plist.
    // The feature file can reference classes from groups.plist, but kerning.plist cannot reference groups from the feature file.

    for ( i=0; i<kc->first_cnt; ++i ) if ( kc->firsts[i]!=NULL && kernclass_for_feature_file(sf, kc, kc->firsts_flags ? kc->firsts_flags[i] : 0)) {
	fprintf( out, "    @kc%d_first_%d = [", sub->subtable_offset, i );
	dump_glyphnamelist(out,sf,kc->firsts[i] );
	fprintf( out, "];\n" );
    }
    for ( i=0; i<kc->second_cnt; ++i ) if ( kc->seconds[i]!=NULL && kernclass_for_feature_file(sf, kc, kc->seconds_flags ? kc->seconds_flags[i] : 0)) {
	fprintf( out, "    @kc%d_second_%d = [", sub->subtable_offset, i );
	dump_glyphnamelist(out,sf,kc->seconds[i] );
	fprintf( out, "];\n" );
    }
    for ( i=0; i<kc->first_cnt; ++i ) if ( kc->firsts[i]!=NULL ) {
	for ( j=0; j<kc->second_cnt; ++j ) if ( kc->seconds[j]!=NULL ) {
	    if ( kc->offsets[i*kc->second_cnt+j]!=0 && kernclass_for_feature_file(sf, kc, kc->offsets_flags ? kc->offsets_flags[i] : 0) )
		fprintf( out, "    pos @kc%d_first_%d @kc%d_second_%d %d;\n",
			sub->subtable_offset, i,
			sub->subtable_offset, j,
			kc->offsets[i*kc->second_cnt+j]);
	}
    }
}

static char *nameend_from_class(char *glyphclass) {
    char *pt = glyphclass;

    while ( *pt==' ' ) ++pt;
    while ( *pt!=' ' && *pt!='\0' ) ++pt;
return( pt );
}

static OTLookup *lookup_in_rule(struct fpst_rule *r,int seq,int *index, int *pos) {
    int i;
    OTLookup *otl;
    /* Suppose there are two lookups which affect the same sequence pos? */
    /* That's legal in otf, but I don't think it can be specified in the */
    /*  feature file. It doesn't seem likely to be used so I ignore it */

    for ( i=0; i<r->lookup_cnt && seq<r->lookups[i].seq; ++i );
    if ( i>=r->lookup_cnt )
return( NULL );
    *index = i;
    *pos = seq-r->lookups[i].seq;
    otl = r->lookups[i].lookup;
    if ( seq==r->lookups[i].seq )
return( otl );
    if ( otl->lookup_type == gsub_ligature )
return( otl );
    else if ( otl->lookup_type == gpos_pair && *pos==1 )
return( otl );

return( NULL );
}

static PST *pst_from_single_lookup(SplineFont *sf, OTLookup *otl, char *name ) {
    SplineChar *sc = SFGetChar(sf,-1,name);
    PST *pst;

    if ( sc==NULL )
return( NULL );
    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
	if ( pst->subtable!=NULL && pst->subtable->lookup == otl )
return( pst );
    }
return( NULL );
}

static PST *pst_from_pos_pair_lookup(SplineFont *sf, OTLookup *otl, char *name1, char *name2, PST *space ) {
    SplineChar *sc = SFGetChar(sf,-1,name1), *sc2;
    PST *pst;
    int isv;
    KernPair *kp;

    if ( sc==NULL )
return( NULL );
    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
	if ( pst->subtable!=NULL && pst->subtable->lookup == otl &&
		strcmp(pst->u.pair.paired,name2)==0 )
return( pst );
    }

    sc2 = SFGetChar(sf,-1,name2);
    if ( sc2==NULL )
return( NULL );
    /* Ok. not saved as a pst. If a kp then build a pst of it */
    for ( isv=0; isv<2; ++isv ) {
	for ( kp= isv ? sc->vkerns : sc->kerns; kp!=NULL; kp=kp->next ) {
	    if ( kp->subtable->lookup==otl && kp->sc == sc2 ) {
		memset(space->u.pair.vr,0,2*sizeof(struct vr));
		/* !!! Bug. Lose device tables here */
		if ( isv )
		    space->u.pair.vr[0].v_adv_off = kp->off;
		else if ( otl->lookup_flags&pst_r2l )
		    space->u.pair.vr[1].h_adv_off = kp->off;
		else
		    space->u.pair.vr[0].h_adv_off = kp->off;
return( space );
	    }
	}
    }
return( NULL );
}

static PST *pst_from_ligature(SplineFont *sf, OTLookup *otl, char *components ) {
    PST *pst;
    SplineFont *_sf;
    int k,gid;
    SplineChar *sc;

    k=0;
    do {
	_sf = sf->subfontcnt==0 ? sf : sf->subfonts[k];
	for ( gid=0; gid<_sf->glyphcnt; ++gid ) if ( (sc = _sf->glyphs[gid])!=NULL ) {
	    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
		if ( pst->subtable!=NULL && pst->subtable->lookup == otl &&
			strcmp(pst->u.lig.components,components)==0 ) {
		    pst->u.lig.lig = sc;
return( pst );
		}
	    }
	}
	++k;
    } while ( k<sf->subfontcnt );
return( NULL );
}

static PST *pst_any_from_otl(SplineFont *sf, OTLookup *otl ) {
    PST *pst;
    SplineFont *_sf;
    int k,gid;
    SplineChar *sc;

    k=0;
    do {
	_sf = sf->subfontcnt==0 ? sf : sf->subfonts[k];
	for ( gid=0; gid<_sf->glyphcnt; ++gid ) if ( (sc = _sf->glyphs[gid])!=NULL ) {
	    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
		if ( pst->subtable!=NULL && pst->subtable->lookup == otl ) {
		    if ( otl->lookup_type == gsub_ligature )
			pst->u.lig.lig = sc;
return( pst );
		}
	    }
	}
	++k;
    } while ( k<sf->subfontcnt );
return( NULL );
}

static AnchorPoint *apfind_entry_exit(SplineFont *sf,OTLookup *otl, char *name,
	AnchorPoint **_exit) {
    AnchorPoint *entry = NULL, *exit = NULL;
    AnchorPoint *ap;
    SplineChar *sc = SFGetChar(sf,-1,name);

    if ( sc==NULL )
return( NULL );
    for ( ap = sc->anchor ; ap!=NULL; ap=ap->next ) {
	if ( ap->anchor->subtable->lookup == otl ) {
	    if ( ap->type == at_centry )
		entry = ap;
	    else if ( ap->type == at_cexit )
		exit = ap;
	}
    }
    *_exit = exit;
return( entry );
}

static void dump_contextpstglyphs(FILE *out,SplineFont *sf,
	struct lookup_subtable *sub, struct fpst_rule *r) {
    int i, j, pos, index;
    OTLookup *otl;
    struct vr pairvr[2];
    PST *pst, space;
    char *start, *pt, *last_start, *last_end;
    int ch, ch2, uses_lookups=false;

    space.u.pair.vr = pairvr;

    if ( r->u.glyph.back!=NULL ) {
	char *temp = reverseGlyphNames(r->u.glyph.back);
	dump_glyphnamelist(out,sf,temp);
	free (temp);
	putc(' ',out);
    }
    last_start = last_end = NULL;
    for ( pt=r->u.glyph.names; ; ) {
	while ( *pt==' ' ) ++pt;
	if ( *pt=='\0' )
    break;
	for ( start=pt; *pt!=' ' && *pt!='\0'; ++pt );
	ch = *pt; *pt = '\0';
	otl = lookup_in_rule(r,i,&index, &pos);
	if ( otl!=NULL && !otl->ticked ) {
	    if ( otl->lookup_type == gpos_cursive )
		fprintf( out, "cursive " );
	    else if ( otl->lookup_type == gpos_mark2mark )
		fprintf( out, "mark " );
	}
	dump_glyphbyname(out,sf,start);
	putc('\'',out);
	if ( otl!=NULL ) {
	    if ( otl->ticked ) {
		fprintf(out, "lookup %s ", lookupname(otl) );
		uses_lookups = true;
	    } else if ( otl->lookup_type==gpos_single ) {
		pst = pst_from_single_lookup(sf, otl, start);
		if ( pst!=NULL )
		    dump_valuerecord(out,&pst->u.pos);
	    } else if ( otl->lookup_type==gpos_pair ) {
		if ( pos==1 ) {
		    ch2 = *last_end; *last_end = '\0';
		    pst = pst_from_pos_pair_lookup(sf,otl,last_start,start,&space);
		    *last_end = ch2;
		} else {
		    char *next_start, *next_end;
		    next_start = pt;
		    while ( *next_start==' ' ) ++next_start;
		    if ( *next_start!='\0' ) {
			for ( next_end=next_start; *next_end!=' ' && *next_end!='\0'; ++next_end );
			ch2 = *next_end; *next_end = '\0';
			pst = pst_from_pos_pair_lookup(sf,otl,start,next_start,&space);
			*next_end = ch2;
		    } else
			pst = NULL;
		}
		if ( pst!=NULL )
		    dump_valuerecord(out,&pst->u.pair.vr[pos]);
	    } else if ( otl->lookup_type==gpos_cursive ) {
		AnchorPoint *entry, *exit;
		entry = apfind_entry_exit(sf,otl,start,&exit);
		dump_anchorpoint(out,entry);
		putc(' ',out);
		dump_anchorpoint(out,exit);
		putc(' ',out);
	    } else if ( otl->lookup_type>gpos_start && otl->ticked )
		fprintf(out, "<lookup %s> ", lookupname(otl) );
	}
	last_start = start; last_end = pt;
	*pt = ch;
	putc(' ',out);
    }
    if ( r->u.glyph.fore!=NULL ) {
	dump_glyphnamelist(out,sf,r->u.glyph.fore );
	putc(' ',out);
    }
    if ( r->lookup_cnt!=0 && sub->lookup->lookup_type<gpos_start && !uses_lookups ) {
	fprintf( out, " by " );
	for ( i=0; i<r->lookup_cnt; ++i ) {
	    otl = r->lookups[i].lookup;
	    for ( pt=r->u.glyph.names, j=0; ; j++ ) {
		while ( *pt==' ' ) ++pt;
		if ( *pt=='\0' || j>=r->lookups[i].seq )
	    break;
		while ( *pt!=' ' && *pt!='\0' ) ++pt;
	    }
	    if ( otl->lookup_type == gsub_single ) {
		pst = pst_from_single_lookup(sf,otl,pt);
		if ( pst!=NULL )
		    dump_glyphbyname(out,sf,pst->u.subs.variant );
	    } else if ( otl->lookup_type==gsub_ligature ) {
		pst = pst_from_ligature(sf,otl,pt);
		if ( pst!=NULL )
		    dump_glyphname(out,pst->u.lig.lig);
		putc( ' ',out );
	    } else if ( otl->lookup_type<gpos_start && otl->ticked )
		fprintf(out, "<lookup %s> ", lookupname(otl) );
	}
    }
    putc(';',out); putc('\n',out);
}

static void dump_contextpstcoverage(FILE *out,SplineFont *sf,
	struct lookup_subtable *sub, struct fpst_rule *r, int in_ignore) {
    int i, pos, index;
    OTLookup *otl;
    PST *pst, space;
    char *start, *pt, *last_end;
    int ch, ch2, uses_lookups=false;

    /* bcovers glyph classes are in reverse order, but they should be in */
    /* natural order in the feature file, so we dump them in the reverse */
    /* order */
    for ( i=r->u.coverage.bcnt-1; i>=0; --i ) {
	putc('[',out);
	dump_glyphnamelist(out,sf,r->u.coverage.bcovers[i] );
	fprintf( out, "] ");
    }
    for ( i=0; i<r->u.coverage.ncnt; ++i ) {
	putc('[',out);
	dump_glyphnamelist(out,sf,r->u.coverage.ncovers[i] );
	putc(']',out);
	if ( in_ignore )
	    putc('\'',out);
	else {
	    otl = lookup_in_rule(r,i,&index, &pos);
	    putc( '\'', out );
	    if ( otl!=NULL ) {
		/* Ok, I don't see any way to specify a class of value records */
		/*  so just assume all vr will be the same for the class */
		pt = nameend_from_class(r->u.coverage.ncovers[i]);
		ch = *pt; *pt = '\0';
		if ( otl->ticked ) {
		    fprintf(out, "lookup %s ", lookupname(otl) );
		    uses_lookups = true;
		} else if ( otl->lookup_type==gpos_single ) {
		    pst = pst_from_single_lookup(sf,otl,r->u.coverage.ncovers[i]);
		    if ( pst!=NULL )
			dump_valuerecord(out,&pst->u.pos);
		} else if ( otl->lookup_type==gpos_pair ) {
		    if ( pos==1 ) {
			last_end = nameend_from_class(r->u.coverage.ncovers[i-1]);
			ch2 = *last_end; *last_end = '\0';
			pst = pst_from_pos_pair_lookup(sf,otl,r->u.coverage.ncovers[i-1],r->u.coverage.ncovers[i],&space);
			*last_end = ch2;
		    } else if ( i+1<r->u.coverage.ncnt ) {
			char *next_end;
			next_end = nameend_from_class(r->u.coverage.ncovers[i+1]);
			ch2 = *next_end; *next_end = '\0';
			pst = pst_from_pos_pair_lookup(sf,otl,r->u.coverage.ncovers[i],r->u.coverage.ncovers[i+1],&space);
			*next_end = ch2;
		    } else
			pst = NULL;
		    if ( pst!=NULL )
			dump_valuerecord(out,&pst->u.pair.vr[pos]);
		}
		*pt = ch;
	    }
	}
	putc(' ',out);
    }
    for ( i=0; i<r->u.coverage.fcnt; ++i ) {
	putc('[',out);
	dump_glyphnamelist(out,sf,r->u.coverage.fcovers[i] );
	fprintf( out, "] ");
    }
    if ( sub->lookup->lookup_type == gsub_reversecchain ) {
	fprintf( out, " by " );
	if ( strchr(r->u.rcoverage.replacements,' ')==NULL )
	    dump_glyphnamelist(out,sf,r->u.rcoverage.replacements);
	else {
	    putc('[',out);
	    dump_glyphnamelist(out,sf,r->u.rcoverage.replacements);
	    putc(']',out);
	}
    } else if ( r->lookup_cnt!=0 && sub->lookup->lookup_type<gpos_start && !uses_lookups ) {
	fprintf( out, " by " );
	for ( i=0; i<r->lookup_cnt; ++i ) {
	    int len;
	    otl = r->lookups[i].lookup;
	    if ( otl->lookup_type==gsub_single ) {
		len = 40;
		putc('[',out);
		for ( pt=r->u.coverage.ncovers[r->lookups[i].seq]; ; ) {
		    while ( *pt==' ' ) ++pt;
		    if ( *pt=='\0' )
		break;
		    for ( start=pt; *pt!=' ' && *pt!='\0'; ++pt );
		    ch = *pt; *pt = '\0';
		    pst = pst_from_single_lookup(sf,otl,start);
		    if ( len+(pst==NULL?4:strlen(pst->u.subs.variant))>80 ) {
			fprintf(out,"\n\t" );
			len = 8;
		    }
		    if ( pst!=NULL ) {
			dump_glyphbyname(out,sf,pst->u.subs.variant);
			len += strlen(pst->u.subs.variant);
		    } else {
			fprintf( out, "NULL" );
			len += 4;
		    }
		    *pt = ch;
		    putc(' ',out);
		    ++len;
		}
		putc(']',out);
	    } else if ( otl->lookup_type==gsub_ligature ) {
		/* If we get here assume there is only one ligature */
		/*  or there is only one combination of input glyphs (all coverage tables contain one glyph) */
		if ( sub->fpst->effectively_by_glyphs ) {
		    /* Build up THE combination of input glyphs */
		    int len, n;
		    char *start;
		    for ( n=len=0; n<r->u.coverage.ncnt; ++n )
			len += strlen(r->u.coverage.ncovers[n])+1;
		    start = malloc(len+1);
		    for ( n=len=0; n<r->u.coverage.ncnt; ++n ) {
			strcpy(start+len,r->u.coverage.ncovers[n]);
			len += strlen(r->u.coverage.ncovers[n]);
			if ( start[len-1]!=' ' )
			    start[len++] = ' ';
		    }
		    if ( len!=0 )
			start[len-1] = '\0';
		    pst = pst_from_ligature(sf,otl,start);
		    free( start );
		} else
		    pst = pst_any_from_otl(sf,otl);
		if ( pst!=NULL )
		    dump_glyphname(out,pst->u.lig.lig);
	    }
	}
    }
    putc(';',out); putc('\n',out);
}

static int ClassUsed(FPST *fpst, int which, int class_num) {
    int i,j;

    for ( j=0; j<fpst->rule_cnt; ++j ) {
	struct fpst_rule *r = &fpst->rules[j];
	int cnt = which==0? r->u.class.ncnt : which==1 ? r->u.class.bcnt : r->u.class.fcnt;
	uint16 *checkme = which==0? r->u.class.nclasses : which==1 ? r->u.class.bclasses : r->u.class.fclasses;
	for ( i=0; i<cnt; ++i )
	    if ( checkme[i] == class_num )
return( true );
    }
return( false );
}

static void dump_contextpstclass(FILE *out,SplineFont *sf,
	struct lookup_subtable *sub, struct fpst_rule *r, int in_ignore) {
    FPST *fpst = sub->fpst;
    int i, pos, index;
    OTLookup *otl;
    PST *pst, space;
    char *start, *pt, *last_start, *last_end;
    int ch, ch2, uses_lookups=false;

    for ( i=0; i<r->u.class.bcnt; ++i )
	fprintf( out, "@cc%d_back_%d ", sub->subtable_offset, r->u.class.bclasses[i] );
    for ( i=0; i<r->u.class.ncnt; ++i ) {
	if ( i==0 && fpst->nclass[r->u.class.nclasses[i]]==NULL )
    continue;
	fprintf( out, "@cc%d_match_%d", sub->subtable_offset, r->u.class.nclasses[i] );
	if ( in_ignore )
	    putc( '\'',out );
	else {
	    otl = lookup_in_rule(r,i,&index, &pos);
	    putc( '\'', out );
	    if ( otl!=NULL ) {
		/* Ok, I don't see any way to specify a class of value records */
		/*  so just assume all vr will be the same for the class */
		start = fpst->nclass[r->u.class.nclasses[i]];
		pt = nameend_from_class(start);
		ch = *pt; *pt = '\0';
		if ( otl->ticked ) {
		    fprintf(out, "lookup %s ", lookupname(otl) );
		    uses_lookups = true;
		} else if ( otl->lookup_type==gpos_single ) {
		    pst = pst_from_single_lookup(sf,otl,start);
		    if ( pst!=NULL )
			dump_valuerecord(out,&pst->u.pos);
		} else if ( otl->lookup_type==gpos_pair ) {
		    if ( pos==1 ) {
			last_start = fpst->nclass[r->u.class.nclasses[i-1]];
			last_end = nameend_from_class(last_start);
			ch2 = *last_end; *last_end = '\0';
			pst = pst_from_pos_pair_lookup(sf,otl,last_start,start,&space);
			*last_end = ch2;
		    } else if ( i+1<r->u.coverage.ncnt ) {
			char *next_start, *next_end;
			next_start = fpst->nclass[r->u.class.nclasses[i+1]];
			next_end = nameend_from_class(next_start);
			ch2 = *next_end; *next_end = '\0';
			pst = pst_from_pos_pair_lookup(sf,otl,start,next_start,&space);
			*next_end = ch2;
		    } else
			pst = NULL;
		    if ( pst!=NULL )
			dump_valuerecord(out,&pst->u.pair.vr[pos]);
		}
		*pt = ch;
	    }
	}
	putc(' ',out);
    }
    for ( i=0; i<r->u.class.fcnt; ++i )
	fprintf( out, "@cc%d_ahead_%d ", sub->subtable_offset, r->u.class.fclasses[i] );
    if ( r->lookup_cnt!=0 && sub->lookup->lookup_type<gpos_start && !uses_lookups ) {
	fprintf( out, " by " );
	for ( i=0; i<r->lookup_cnt; ++i ) {
	    otl = r->lookups[i].lookup;
	    if ( otl->lookup_type==gsub_single ) {
		putc('[',out);
		if ( fpst->nclass[r->u.class.nclasses[r->lookups[i].seq]]!=NULL ) {
		    for ( pt=fpst->nclass[r->u.class.nclasses[r->lookups[i].seq]]; ; ) {
			while ( *pt==' ' ) ++pt;
			if ( *pt=='\0' )
		    break;
			for ( start=pt; *pt!=' ' && *pt!='\0'; ++pt );
			ch = *pt; *pt = '\0';
			pst = pst_from_single_lookup(sf,otl,start);
			if ( pst!=NULL )
			    dump_glyphbyname(out,sf,pst->u.subs.variant);
			else
			    fprintf( out, "NULL" );
			*pt = ch;
			putc(' ',out);
		    }
		} else
		    dump_fpst_everythingelse(out,sf,fpst->nclass,fpst->nccnt,otl);
		putc(']',out);
	    } else if ( otl->lookup_type==gsub_ligature ) {
		/* I don't think it is possible to do this correctly */
		/* adobe doesn't say how to order glyphs for [f longs]' [i l]' */
		/* so just assume there is only one ligature */
		pst = pst_any_from_otl(sf,otl);
		if ( pst!=NULL )
		    dump_glyphname(out,pst->u.lig.lig);
	    }
	    putc( ' ',out );
	}
    }
    putc( ';',out ); putc( '\n',out );
}

static void dump_contextpst(FILE *out,SplineFont *sf,struct lookup_subtable *sub) {
    FPST *fpst = sub->fpst;
    int i,j,k;

    if ( fpst->format==pst_reversecoverage ) {
	IError( "I can find no documentation on how to describe reverse context chaining lookups. Lookup %s will be skipped.",
		sub->lookup->lookup_name );
return;
    }
    if ( fpst->format==pst_class ) {
	if ( fpst->nclass!=NULL ) {
	    for ( i=0; i<fpst->nccnt; ++i ) {
		if ( fpst->nclass[i]!=NULL || (i==0 && ClassUsed(fpst,0,0))) {
		    fprintf( out, "    @cc%d_match_%d = [", sub->subtable_offset, i );
		    if ( fpst->nclass[i]!=NULL )
			dump_glyphnamelist(out,sf,fpst->nclass[i] );
		    else
			dump_fpst_everythingelse(out,sf,fpst->nclass,fpst->nccnt,NULL);
		    fprintf( out, "];\n" );
		}
	    }
	}
	if ( fpst->bclass!=NULL ) {
	    for ( i=0; i<fpst->bccnt; ++i ) {
		if ( fpst->bclass[i]!=NULL || (i==0 && ClassUsed(fpst,1,0))) {
		    fprintf( out, "    @cc%d_back_%d = [", sub->subtable_offset, i );
		    if ( fpst->bclass[i]!=NULL )
			dump_glyphnamelist(out,sf,fpst->bclass[i] );
		    else
			dump_fpst_everythingelse(out,sf,fpst->bclass,fpst->bccnt,NULL);
		    fprintf( out, "];\n" );
		}
	    }
	}
	if ( fpst->fclass!=NULL ) {
	    for ( i=0; i<fpst->fccnt; ++i ) {
		if ( fpst->fclass[i]!=NULL || (i==0 && ClassUsed(fpst,2,0))) {
		    fprintf( out, "    @cc%d_ahead_%d = [", sub->subtable_offset, i );
		    if ( fpst->fclass[i]!=NULL )
			dump_glyphnamelist(out,sf,fpst->fclass[i] );
		    else
			dump_fpst_everythingelse(out,sf,fpst->fclass,fpst->fccnt,NULL);
		    fprintf( out, "];\n" );
		}
	    }
	}
    }
    for ( j=0; j<fpst->rule_cnt; ++j ) {
	struct fpst_rule *r = &fpst->rules[j];
	for ( i=0; i<r->lookup_cnt; ++i ) for ( k=i+1; k<r->lookup_cnt; ++k ) {
	    if ( r->lookups[i].seq > r->lookups[k].seq ) {
		int s = r->lookups[i].seq;
		OTLookup *otl = r->lookups[i].lookup;
		r->lookups[i].seq = r->lookups[k].seq;
		r->lookups[k].seq = s;
		r->lookups[i].lookup = r->lookups[k].lookup;
		r->lookups[k].lookup = otl;
	    }
	}
	if ( sub->lookup->lookup_type>=gpos_start )
	    fprintf( out, r->lookup_cnt==0 ? "    ignore pos " : "    pos " );
	else if ( sub->lookup->lookup_type==gsub_reversecchain )
	    fprintf( out, r->lookup_cnt==0 ? "    ignore reversesub " : "    reversesub " );
	else
	    fprintf( out, r->lookup_cnt==0 ? "    ignore sub " : "    sub " );
	if ( fpst->format==pst_class ) {
	    dump_contextpstclass(out,sf,sub,r,r->lookup_cnt==0);
	} else if ( fpst->format==pst_glyphs ) {
	    dump_contextpstglyphs(out,sf,sub,r);
	} else { /* And reverse coverage */
	    dump_contextpstcoverage(out,sf,sub,r,r->lookup_cnt==0);
	}
    }
}

static int HasBaseAP(SplineChar *sc,struct lookup_subtable *sub) {
    AnchorPoint *ap;

    for ( ap=sc->anchor; ap!=NULL; ap=ap->next ) {
	if ( ap->anchor->subtable==sub && ap->type!=at_mark )
return( true );
    }

return( false );
}

static int SameBaseAP(SplineChar *sc1,SplineChar *sc2,struct lookup_subtable *sub) {
    AnchorPoint *ap1, *ap2;

    for ( ap1=sc1->anchor; ap1!=NULL; ap1=ap1->next ) {
	if ( ap1->anchor->subtable == sub &&
		(ap1->type==at_basechar || ap1->type==at_basemark)) {
	    for ( ap2=sc2->anchor; ap2!=NULL ; ap2=ap2->next ) {
		if ( ap1->anchor==ap2->anchor &&
			(ap2->type==at_basechar || ap2->type==at_basemark)) {
		    if ( ap1->me.x!=ap2->me.x || ap1->me.y!=ap2->me.y ||
			ap1->has_ttf_pt!=ap2->has_ttf_pt ||
			    (ap1->has_ttf_pt && ap1->ttf_pt_index != ap2->ttf_pt_index ))
return( false );
		    else
	    break;
		}
	    }
return( ap2!=NULL );
	}
    }
return( false );
}

static void dump_anchors(FILE *out,SplineFont *sf,struct lookup_subtable *sub) {
    int i, j, k;
    SplineFont *_sf;
    SplineChar *sc;
    AnchorClass *ac;
    AnchorPoint *ap, *ap_entry, *ap_exit;

    for ( ac = sf->anchor; ac!=NULL; ac=ac->next ) if ( ac->subtable==sub ) {
	if ( sub->lookup->lookup_type==gpos_cursive ) {
	    k=0;
	    do {
		_sf = k<sf->subfontcnt ? sf->subfonts[k] : sf;
		for ( i=0; i<_sf->glyphcnt; ++i ) if ( (sc=_sf->glyphs[i])!=NULL ) {
		    ap_entry = ap_exit = NULL;
		    for ( ap=sc->anchor; ap!=NULL; ap=ap->next ) if ( ap->anchor==ac ) {
			if ( ap->type == at_centry )
			    ap_entry = ap;
			else if ( ap->type == at_cexit )
			    ap_exit = ap;
		    }
		    if ( ap_entry!=NULL || ap_exit!=NULL ) {
			fprintf( out, "    pos cursive " );
			dump_glyphname(out,sc);
			putc(' ',out);
			dump_anchorpoint(out,ap_entry);
			putc(' ',out);
			dump_anchorpoint(out,ap_exit);
			putc(';',out);
			putc('\n',out);
		    }
		}
		++k;
	    } while ( k<sf->subfontcnt );
	} else {
	    struct amarks { SplineChar *sc; AnchorPoint *ap; } *marks = NULL;
	    int cnt, max;
	    cnt = max = 0;
	    k=0;
	    /* Gather all the marks in this class */
	    do {
		_sf = k<sf->subfontcnt ? sf->subfonts[k] : sf;
		for ( i=0; i<_sf->glyphcnt; ++i ) if ( (sc=_sf->glyphs[i])!=NULL ) {
		    for ( ap=sc->anchor; ap!=NULL; ap=ap->next ) {
			if ( ap->anchor==ac && ap->type==at_mark ) {
			    if ( cnt>=max )
				marks = realloc(marks,(max+=20)*sizeof(struct amarks));
			    marks[cnt].sc = sc;
			    marks[cnt].ap = ap;
			    sc->ticked = false;
			    ++cnt;
		    break;
			}
		    }
		}
		++k;
	    } while ( k<sf->subfontcnt );
	    if ( cnt==0 )
    continue;		/* No marks? Nothing to be done */
	    /* Now output the marks */
	    for ( k=0; k<cnt; ++k ) if ( !(sc = marks[k].sc)->ticked ) {
		SplineChar *osc;
		ap = marks[k].ap;
		fprintf( out, "  markClass [" );
		for ( j=k; j<cnt; ++j ) if ( !(osc = marks[j].sc)->ticked ) {
		    AnchorPoint *other = marks[j].ap;
		    if ( other->me.x == ap->me.x && other->me.y == ap->me.y &&
			    (other->has_ttf_pt == ap->has_ttf_pt &&
			     (!ap->has_ttf_pt || ap->ttf_pt_index==other->ttf_pt_index))) {
			dump_glyphname(out,osc);
			putc(' ',out);
			osc->ticked = true;
		    }
		}
		fprintf( out, "] " );
		dump_anchorpoint(out,ap);
		fprintf( out, " @" );
		dump_ascii( out, ac->name );
		fprintf( out, ";\n" );
	    }
	    /* When positioning, we dump out all of a base glyph's anchors */
	    /*  for the sub-table at once rather than class by class */
	    free(marks);
	}
    }
    if ( sub->lookup->lookup_type==gpos_cursive )
return;

    k=0;
    do {
	_sf = k<sf->subfontcnt ? sf->subfonts[k] : sf;
	for ( i=0; i<_sf->glyphcnt; ++i ) if ( (sc=_sf->glyphs[i])!=NULL ) {
	    sc->ticked = false;
	}
	++k;
    } while ( k<sf->subfontcnt );
    k=0;
    do {
	_sf = k<sf->subfontcnt ? sf->subfonts[k] : sf;
	for ( i=0; i<_sf->glyphcnt; ++i ) {
	    if ( (sc=_sf->glyphs[i])!=NULL && !sc->ticked && HasBaseAP(sc,sub)) {
		fprintf( out, "  pos %s ",
			sub->lookup->lookup_type==gpos_mark2base ? "base" :
			sub->lookup->lookup_type==gpos_mark2mark ? "mark" :
			"ligature" );
		if ( sub->lookup->lookup_type!=gpos_mark2ligature ) {
		    putc('[',out);
		    for ( j=i; j<_sf->glyphcnt; ++j ) {
			SplineChar *osc;
			if ( (osc=_sf->glyphs[j])!=NULL && !osc->ticked && SameBaseAP(osc,sc,sub)) {
			    osc->ticked = true;
			    dump_glyphname(out,osc);
			    putc(' ',out);
			}
		    }
		    fprintf(out, "] " );
		} else {
		    dump_glyphname(out,sc);
		    putc(' ',out);
		}
		if ( sub->lookup->lookup_type!=gpos_mark2ligature ) {
		    int first = true;
		    for ( ap = sc->anchor; ap!=NULL; ap=ap->next ) if ( ap->anchor->subtable==sub && ap->type!=at_mark ) {
			if ( !first )
			    fprintf(out,"\n\t");
			first = false;
			dump_anchorpoint(out,ap);
			fprintf( out, " mark @");
			dump_ascii(out, ap->anchor->name );
		    }
		    fprintf(out,";\n");
		} else {
		    int li, anymore=true, any;
		    for ( li=0; anymore; ++li ) {
			any = anymore = false;
			for ( ap = sc->anchor; ap!=NULL; ap=ap->next ) if ( ap->anchor->subtable==sub ) {
			    if ( ap->lig_index>li )
				anymore = true;
			    else if ( ap->lig_index==li ) {
				if ( li!=0 && !any )
				    fprintf( out, "\n    ligComponent\n      " );
				dump_anchorpoint(out,ap);
				fprintf( out, " mark @");
				dump_ascii(out, ap->anchor->name );
				putc( ' ',out );
			        any = true;
			    }
			}
			if ( !any && anymore ) {
			    if ( li!=0 )
				fprintf( out, "\n    ligComponent\n      " );
			    fprintf( out, "<anchor NULL>" );	/* In adobe's example no anchor class is given */
			}
		    }
                    fprintf(out,";\n");
		}
	    }
	}
	++k;
    } while ( k<sf->subfontcnt );
}

static void number_subtables(SplineFont *sf) {
    OTLookup *otl;
    struct lookup_subtable *sub;
    int isgpos, cnt;

    cnt = 0;
    for ( isgpos=0; isgpos<2; ++isgpos ) {
	for ( otl = isgpos ? sf->gpos_lookups : sf->gsub_lookups; otl!=NULL; otl = otl->next ) {
	    for ( sub = otl->subtables; sub!=NULL; sub=sub->next )
		sub->subtable_offset = cnt++;
	}
    }
}

static int fea_bad_contextual_nestedlookup(SplineFont *sf,FPST *fpst, OTLookup *nested) {
    int gid, k;
    SplineFont *_sf;
    SplineChar *sc;
    PST *pst, *found;
    /* non-cursive anchors can't currently be expressed in contextuals */
	/* No longer true, but it's still easier to do them outside */
    /* Nor can other contextuals */
    /* Nor can alternate/multiple substitutions */
    /* In v1.8, nor can contextuals with more than one nested lookup */
    /* If we are class based or coverage table based (and interesting classes */
    /*  contain more than one glyph name) then we can't express single_pos, */
    /*  kerning, ligatures, cursive anchors (unless everything in the lookup */
    /*  results in the same transformation) */

    switch ( nested->lookup_type ) {
      case gsub_single:
return( false );			/* This is the one thing that can always be expressed */
      case gsub_multiple:
      case gsub_alternate:
      case gsub_context:
      case gsub_contextchain:
      case gsub_reversecchain:
      case gpos_mark2base:
      case gpos_mark2ligature:
      case gpos_mark2mark:
      case gpos_contextchain:
      case gpos_context:
return( true );		/* These can never be expressed */
      case gpos_cursive:
      case gpos_pair:
return( fpst->format!=pst_glyphs /* && !fpst->effectively_by_glyphs*/ );
      case gsub_ligature:
      case gpos_single:
	if ( fpst->format==pst_glyphs || fpst->effectively_by_glyphs )
return( false );
	/* One can conceive of a fraction lookup */
	/*   [one one.proportion] [slash fraction] [two two.lining] => onehalf */
	/*  where all inputs go to one output. That can be expressed */
	/* One can also conceive of a positioning lookup (subscript->superscript) */
	/*  where all value records are the same */
	found = NULL;
	k=0;
	do {
	    _sf = sf->subfontcnt==0 ? sf : sf->subfonts[k];
	    for ( gid=0; gid<_sf->glyphcnt; ++gid ) if ( (sc = _sf->glyphs[gid])!=NULL ) {
		for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
		    if ( pst->subtable!=NULL && pst->subtable->lookup==nested ) {
			if ( found==NULL ) {
			    found = pst;
		break;
			} else if ( nested->lookup_type==gsub_ligature )
return( true );			/* Different glyphs */
			else if ( found->u.pos.xoff!=pst->u.pos.xoff ||
				found->u.pos.yoff!=pst->u.pos.yoff ||
				found->u.pos.h_adv_off!=pst->u.pos.h_adv_off ||
				found->u.pos.v_adv_off!=pst->u.pos.v_adv_off )
return( true );
			else
		break;
		    }
		}
	    }
	    ++k;
	} while ( k<sf->subfontcnt );
	if ( found==NULL )
return( true );
return( false );
      default:
return( true );
    }
}

static void dump_lookup(FILE *out, SplineFont *sf, OTLookup *otl);

static void dump_needednestedlookups(FILE *out, SplineFont *sf, OTLookup *otl) {
    struct lookup_subtable *sub;
    int r, s, n;
    /* So we cheat, and extend the fea format to allow us to specify a lookup */
    /*  in contextuals */
    /* With 1.8 this is part of the spec, but done differently from what I used*/
    /*  to do */

    for ( sub = otl->subtables; sub!=NULL; sub=sub->next ) {
	FPST *fpst = sub->fpst;
	fpst->effectively_by_glyphs = false;
	if ( fpst->format==pst_coverage ) {
	    fpst->effectively_by_glyphs = true;
	    for ( r=0; r<fpst->rule_cnt; ++r ) {
		for ( n=0; n<fpst->rules[r].u.coverage.ncnt; ++n ) {
		    if ( strchr(fpst->rules[r].u.coverage.ncovers[n],' ')!=NULL )
			fpst->effectively_by_glyphs = false;
		break;
		}
	    }
	}
	for ( r=0; r<fpst->rule_cnt; ++r ) {
	    /* In 1.8 if a contextual lookup invokes more than one nested */
	    /*  lookup, then all lookups must be specified with the lookup */
	    /*  keyword */
	    if ( fpst->rules[r].lookup_cnt>1 ) {
		for ( s=0; s<fpst->rules[r].lookup_cnt; ++s ) {
		    OTLookup *nested = fpst->rules[r].lookups[s].lookup;
		    if ( nested!=NULL && nested->features==NULL && !nested->ticked )
			dump_lookup(out,sf,nested);
		}
	    } else if ( fpst->rules[r].lookup_cnt==1 ) {
		OTLookup *nested = fpst->rules[r].lookups[0].lookup;
		if ( nested!=NULL && nested->features==NULL && !nested->ticked &&
/* The number of times the lookup is used is stored in "lookup_length" */
/*  so any lookup used in two places will be output even if it could be */
/*  expressed inline */
			(nested->lookup_length>1 ||
			 fea_bad_contextual_nestedlookup(sf,fpst,nested)))
		    dump_lookup(out,sf,nested);
	    }
	}
    }
}

static void dump_lookup(FILE *out, SplineFont *sf, OTLookup *otl) {
    struct lookup_subtable *sub;
    static char *flagnames[] = { "RightToLeft", "IgnoreBaseGlyphs", "IgnoreLigatures", "IgnoreMarks", NULL };
    int i, k, first;
    SplineFont *_sf;
    SplineChar *sc;
    PST *pst;
    int isv;
    KernPair *kp;

    if ( otl->lookup_type == morx_indic || otl->lookup_type==morx_context ||
	    otl->lookup_type==morx_insert || otl->lookup_type==kern_statemachine )
return;					/* No support for apple "lookups" */

    otl->ticked = true;

    if ( otl->lookup_type==gsub_context || otl->lookup_type==gsub_contextchain ||
	    otl->lookup_type==gpos_context || otl->lookup_type==gpos_contextchain )
	dump_needednestedlookups(out,sf,otl);

    if ( sf->cidmaster ) sf = sf->cidmaster;
    number_subtables(sf);

    fprintf( out, "\nlookup %s {\n", lookupname(otl) );
    if ( otl->lookup_flags==0 || (otl->lookup_flags&0xe0)!=0 )
	fprintf( out, "  lookupflag %d;\n", otl->lookup_flags );
    else {
	fprintf( out, "  lookupflag" );
	first = true;
	for ( i=0; i<4; ++i ) if ( otl->lookup_flags&(1<<i)) {
	    if ( !first ) {
		// putc(',',out); // The specification says to do this, but Adobe's software uses spaces.
	    } else
		first = false;
	    fprintf( out, " %s", flagnames[i] );
	}
	if ( (otl->lookup_flags&0xff00)!=0 ) {
	    int index = (otl->lookup_flags>>8)&0xff;
	    if ( index<sf->mark_class_cnt ) {
		// if ( !first )
		//     putc(',',out);
		fprintf( out, " MarkAttachmentType @" );
		dump_ascii( out, sf->mark_class_names[index]);
	    }
	}
	if ( otl->lookup_flags&pst_usemarkfilteringset ) {
	    int index = (otl->lookup_flags>>16)&0xffff;
	    if ( index<sf->mark_set_cnt ) {
		// if ( !first )
		//     putc(',',out);
		fprintf( out, " UseMarkFilteringSet @" );
		dump_ascii( out, sf->mark_set_names[index]);
	    }
	}
	putc(';',out);
	putc('\n',out);
    }
    for ( sub=otl->subtables; sub!=NULL; sub=sub->next ) {
	/* The `subtable` keyword is only supported in class kerning lookups. */
	if ( sub!=otl->subtables && sub->kc!=NULL )
	    fprintf( out, "  subtable;\n" );
	if ( sub->kc!=NULL )
	    dump_kernclass(out,sf,sub);
	else if ( sub->fpst!=NULL )
	    dump_contextpst(out,sf,sub);
	else if ( sub->anchor_classes )
	    dump_anchors(out,sf,sub);
	else {
	    k=0;
	    do {
		_sf = k<sf->subfontcnt ? sf->subfonts[k] : sf;
		for ( i=0; i<_sf->glyphcnt; ++i ) if ( (sc=_sf->glyphs[i])!=NULL ) {
		    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) if ( pst->subtable==sub ) {
			switch ( otl->lookup_type ) {
			  case gsub_single:
			  case gsub_multiple:
			    fprintf( out, "    sub " );
			    dump_glyphname(out,sc);
			    fprintf( out, " by " );
			    dump_glyphnamelist(out,sf,pst->u.subs.variant );
			    fprintf( out,";\n" );
			  break;
			  case gsub_alternate:
			    fprintf( out, "    sub " );
			    dump_glyphname(out,sc);
			    fprintf( out, " from [" );
			    dump_glyphnamelist(out,sf,pst->u.alt.components );
			    fprintf( out,"];\n" );
			  break;
			  case gsub_ligature:
			    fprintf( out, "    sub " );
			    dump_glyphnamelist(out,sf,pst->u.lig.components );
			    fprintf( out, " by " );
			    dump_glyphname(out,sc);
			    fprintf( out,";\n" );
			  break;
			  case gpos_single:
			    fprintf( out, "    pos " );
			    dump_glyphname(out,sc);
			    putc(' ',out);
			    dump_valuerecord(out,&pst->u.pos);
			    fprintf( out,";\n" );
			  break;
			  case gpos_pair:
			    fprintf( out, "    pos " );
			    dump_glyphname(out,sc);
			    putc(' ',out);
			    dump_valuerecord(out,&pst->u.pair.vr[0]);
			    putc(' ',out);
			    dump_glyphnamelist(out,sf,pst->u.pair.paired );
			    putc(' ',out);
			    dump_valuerecord(out,&pst->u.pair.vr[0]);
			    fprintf( out,";\n" );
			  break;
			  default:
			    /* Eh? How'd we get here? An anchor class with */
			    /* no anchors perhaps? */
			  break;
			}
		    }
		    // We skip outputting these here if the SplineFont says to use native kerning.
		    if (sf->preferred_kerning != 1) for ( isv=0; isv<2; ++isv ) {
			for ( kp=isv ? sc->vkerns : sc->kerns; kp!=NULL; kp=kp->next ) if ( kp->subtable==sub ) {
			    fprintf( out, "    pos " );
			    dump_glyphname(out,sc);
			    putc(' ',out);
			    if ( kp->adjust!=NULL ) {
				/* If we have a device table then we must use the long */
			        /* format */
				if ( isv ) {
				    fprintf( out," < 0 0 0 %d <device NULL> <device NULL> <device NULL> ",
					    kp->off );
				    dumpdevice(out,kp->adjust);
			            fprintf( out, " > " );
			            dump_glyphname(out,kp->sc);
			            fprintf( out, " < 0 0 0 0 >;\n" );
				} else if ( otl->lookup_flags&pst_r2l ) {
				    fprintf( out, " < 0 0 0 0 > " );
			            dump_glyphname(out,kp->sc);
				    fprintf( out," < 0 0 %d 0 <device NULL> <device NULL> ",
					    kp->off );
				    dumpdevice(out,kp->adjust);
			            fprintf( out, " <device NULL>>;\n" );
				} else {
				    fprintf( out," < 0 0 %d 0 <device NULL> <device NULL> ",
					    kp->off );
				    dumpdevice(out,kp->adjust);
			            fprintf( out, " <device NULL>> " );
			            dump_glyphname(out,kp->sc);
			            fprintf( out, " < 0 0 0 0 >;\n" );
				}
			    } else
			    if ( otl->lookup_flags&pst_r2l ) {
				fprintf( out, " < 0 0 0 0 > " );
				dump_glyphname(out,kp->sc);
				fprintf( out," < 0 0 %d 0 >;\n", kp->off );
			    } else {
				dump_glyphname(out,kp->sc);
				putc(' ',out);
			        fprintf( out, "%d;\n", kp->off );
			    }
			}
		    }		/* End isv/kp loops */
		}
		++k;
	    } while ( k<sf->subfontcnt );
	}
    }				/* End subtables */
    fprintf( out, "} %s;\n", lookupname(otl) );
}

static void note_nested_lookups_used_twice(OTLookup *base) {
    OTLookup *otl;
    struct lookup_subtable *sub;
    int r,s;

    for ( otl=base; otl!=NULL; otl=otl->next )
	otl->lookup_length = 0;
    for ( otl=base; otl!=NULL; otl=otl->next ) {
	if ( otl->lookup_type==gsub_context || otl->lookup_type==gsub_contextchain ||
		otl->lookup_type==gpos_context || otl->lookup_type==gpos_contextchain ) {
	    for ( sub = otl->subtables; sub!=NULL; sub=sub->next ) {
		FPST *fpst = sub->fpst;
		if (fpst != NULL) {
		    for ( r=0; r<fpst->rule_cnt; ++r ) {
			for ( s=0; s<fpst->rules[r].lookup_cnt; ++s ) {
			    OTLookup *nested = fpst->rules[r].lookups[s].lookup;
			    ++ nested->lookup_length;
			}
		    }
		}
	    }
	}
    }
}

static void untick_lookups(SplineFont *sf) {
    OTLookup *otl;
    int isgpos;

    for ( isgpos=0; isgpos<2; ++isgpos )
	for ( otl= isgpos ? sf->gpos_lookups : sf->gsub_lookups; otl!=NULL; otl=otl->next )
	    otl->ticked = false;
}

void FeatDumpOneLookup(FILE *out,SplineFont *sf, OTLookup *otl) {
    FeatureScriptLangList *fl;
    struct scriptlanglist *sl;
    int l;

    untick_lookups(sf);
    gdef_markclasscheck(out,sf,otl);

    dump_lookup(out,sf,otl);

    for ( fl = otl->features; fl!=NULL; fl=fl->next ) {
	fprintf( out, "\nfeature %c%c%c%c {\n", fl->featuretag>>24, fl->featuretag>>16, fl->featuretag>>8, fl->featuretag );
	for ( sl = fl->scripts; sl!=NULL; sl=sl->next ) {
	    fprintf( out, "  script %c%c%c%c;\n",
		    sl->script>>24, sl->script>>16, sl->script>>8, sl->script );
	    for ( l=0; l<sl->lang_cnt; ++l ) {
		uint32 lang = l<MAX_LANG ? sl->langs[l] : sl->morelangs[l-MAX_LANG];
		fprintf( out, "     language %c%c%c%c %s;\n",
			lang>>24, lang>>16, lang>>8, lang,
			lang!=DEFAULT_LANG ? "exclude_dflt" : "" );
		fprintf( out, "      lookup %s;\n", lookupname(otl));
	    }
	}
	fprintf( out, "\n} %c%c%c%c;\n", fl->featuretag>>24, fl->featuretag>>16, fl->featuretag>>8, fl->featuretag );
    }
}

static void dump_gdef(FILE *out,SplineFont *sf) {
    PST *pst;
    int i,gid,j,k,l, lcnt, needsclasses, hasclass[4], clsidx;
    SplineChar *sc;
    SplineFont *_sf;
    struct lglyphs { SplineChar *sc; PST *pst; } *glyphs;
    static char *clsnames[] = { "@GDEF_Simple", "@GDEF_Ligature", "@GDEF_Mark", "@GDEF_Component" };

    glyphs = NULL;
    needsclasses = false;
    memset(hasclass,0,sizeof(hasclass));
    for ( l=0; l<2; ++l ) {
	lcnt = 0;
	k=0;
	do {
	    _sf = sf->subfontcnt==0 ? sf : sf->subfonts[k];
	    for ( gid=0; gid<_sf->glyphcnt; ++gid ) if ( (sc=_sf->glyphs[gid])!=NULL ) {
		if ( l==0 ) {
		    clsidx = sc->glyph_class!=0 ? sc->glyph_class: gdefclass(sc)+1;
		    if (clsidx > 1) needsclasses = hasclass[clsidx-2] = true;
		}
		for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
		    if ( pst->type == pst_lcaret ) {
			for ( j=pst->u.lcaret.cnt-1; j>=0; --j )
			    if ( pst->u.lcaret.carets[j]!=0 )
			break;
			if ( j!=-1 )
		break;
		    }
		}
		if ( pst!=NULL ) {
		    if ( glyphs!=NULL ) { glyphs[lcnt].sc = sc; glyphs[lcnt].pst = pst; }
		    ++lcnt;
		}
	    }
	    ++k;
	} while ( k<sf->subfontcnt );
	if ( lcnt==0 )
    break;
	if ( glyphs!=NULL )
    break;
	glyphs = malloc((lcnt+1)*sizeof(struct lglyphs));
	glyphs[lcnt].sc = NULL;
    }

    if ( !needsclasses && lcnt==0 && sf->mark_class_cnt==0 )
return;					/* No anchor positioning, no ligature carets */

    if ( sf->mark_class_cnt!=0 ) {
	fprintf( out, "#Mark attachment classes (defined in GDEF, used in lookupflags)\n" );
	for ( i=1; i<sf->mark_class_cnt; ++i ) {
	    putc('@',out);
	    dump_ascii( out, sf->mark_class_names[i]);
	    fprintf( out, " = [ %s ];\n", sf->mark_classes[i] );
	}
    }

    if ( needsclasses ) {
	int len;
	putc('\n',out);
	/* Class definitions must go outside the table itself. Stupid */
	for ( i=0; i<4; ++i ) {
	    len = strlen(clsnames[i])+8;
	    k=0;
	    if ( hasclass[i] ) {
		k = 0;
		fprintf( out, "%s = [", clsnames[i] );
		do {
		    _sf = sf->subfontcnt==0 ? sf : sf->subfonts[k];
		    for ( gid=0; gid<_sf->glyphcnt; ++gid ) if ( (sc=_sf->glyphs[gid])!=NULL ) {
			if ( sc->glyph_class==i+2 || (sc->glyph_class==0 && gdefclass(sc)==i+1 )) {
			    if ( len+strlen(sc->name)+1 >80 ) {
				putc('\n',out); putc('\t',out);
				len = 8;
			    }
			    dump_glyphname(out,sc);
			    putc(' ',out);
			    len += strlen(sc->name)+1;
			}
		    }
		    ++k;
		} while ( k<sf->subfontcnt );
		fprintf( out, "];\n");
	    }
	}
    }
    fprintf( out, "\ntable GDEF {\n" );
    if ( needsclasses ) {
	/* AFDKO does't like empty classes, there should be just a placeholder */
	fprintf( out, "  GlyphClassDef %s, %s, %s, %s;\n\n",
		hasclass[0]? clsnames[0]: "",
		hasclass[1]? clsnames[1]: "",
		hasclass[2]? clsnames[2]: "",
		hasclass[3]? clsnames[3]: "");
    }

    for ( i=0; i<lcnt; ++i ) {
	PST *pst = glyphs[i].pst;
	fprintf( out, "  LigatureCaretByPos " ); //by position
	dump_glyphname(out,glyphs[i].sc);
	for ( k=0; k<pst->u.lcaret.cnt; ++k ) {
	    fprintf( out, " %d", pst->u.lcaret.carets[k]); //output <caret %d>
	}
	fprintf( out, ";\n" );
    }
    free( glyphs );

    /* no way to specify mark classes */

    fprintf( out, "} GDEF;\n\n" );
}

static void dump_baseaxis(FILE *out,struct Base *axis,char *key) {
    int i;
    struct basescript *script;

    if ( axis==NULL )
return;
    fprintf( out, "  %sAxis.BaseTagList", key );
    for ( i=0; i<axis->baseline_cnt; ++i ) {
	uint32 tag = axis->baseline_tags[i];
	fprintf( out, " %c%c%c%c", tag>>24, tag>>16, tag>>8, tag );
    }
    fprintf( out, ";\n");

    fprintf( out, "  %sAxis.BaseScriptList\n", key );
    for ( script=axis->scripts; script!=NULL; script=script->next ) {
	uint32 scrtag = script->script;
	uint32 tag = axis->baseline_tags[script->def_baseline];
	fprintf( out, "\t%c%c%c%c", scrtag>>24, scrtag>>16, scrtag>>8, scrtag );
	fprintf( out, " %c%c%c%c", tag>>24, tag>>16, tag>>8, tag );
	for ( i=0; i<axis->baseline_cnt; ++i )
	    fprintf( out, " %d", script->baseline_pos[i]);
	if ( script->next!=NULL )
	    fprintf( out, ",");
	else
	    fprintf( out, ";");
	fprintf( out, "\n" );
    };

    /* The spec for MinMax only allows for one script, and only one language */
    /*  in that script, and only one feature in the language.  Presumably this*/
    /*  is an error in the spec, but since I can't guess what they intend */
    /*  and they provide no example, and it is unimplemented (so subject to */
    /*  change) I shan't support it either. */
}

static void dump_base(FILE *out,SplineFont *sf) {
    if ( sf->horiz_base==NULL && sf->vert_base==NULL )
return;

    fprintf( out, "table BASE {\n" );
    dump_baseaxis(out,sf->horiz_base,"Horiz");
    dump_baseaxis(out,sf->vert_base,"Vert");
    fprintf( out, "} BASE;\n\n" );
}

static void UniOut(FILE *out,char *name ) {
    int ch;

    while ( (ch = utf8_ildb((const char **) &name))!=0 ) {
	if ( ch<' ' || ch>='\177' || ch=='\\' || ch=='"' )
	    fprintf( out, "\\%04x", ch );
	else
	    putc(ch,out);
    }
}

static gboolean dump_header_languagesystem_hash_fe( gpointer key,
						gpointer value,
						gpointer user_data )
{
    FILE *out = (FILE*)user_data;
    fprintf( out, "languagesystem %s;\n", (char*)key );
    return 0;
}

static gint tree_strcasecmp (gconstpointer a, gconstpointer b, gpointer user_data) {
    (void)user_data;
    return g_ascii_strcasecmp (a, b);
}

static void dump_header_languagesystem(FILE *out, SplineFont *sf) {
    int isgpos;
    int i,l,s, subl;
    OTLookup *otl;
    FeatureScriptLangList *fl;
    struct scriptlanglist *sl;
    int has_DFLT = 0;

    GTree* ht = g_tree_new_full( tree_strcasecmp, 0, free, NULL );

    for ( isgpos=0; isgpos<2; ++isgpos ) {
	uint32 *feats = SFFeaturesInScriptLang(sf,isgpos,0xffffffff,0xffffffff);
	if ( feats[0]!=0 ) {
	    uint32 *scripts = SFScriptsInLookups(sf,isgpos);
	    note_nested_lookups_used_twice(isgpos ? sf->gpos_lookups : sf->gsub_lookups);
	    for ( i=0; feats[i]!=0; ++i ) {

		for ( s=0; scripts[s]!=0; ++s ) {
		    uint32 *langs = SFLangsInScript(sf,isgpos,scripts[s]);
		    for ( l=0; langs[l]!=0; ++l ) {
			for ( otl = isgpos ? sf->gpos_lookups : sf->gsub_lookups; otl!=NULL; otl=otl->next ) {
			    for ( fl=otl->features; fl!=NULL; fl=fl->next ) if ( fl->featuretag==feats[i] ) {
				    for ( sl=fl->scripts; sl!=NULL; sl=sl->next ) if ( sl->script==scripts[s] ) {
					    for ( subl=0; subl<sl->lang_cnt; ++subl ) {
						char key[100];
                                                const uint32 DFLT_int = (uint32)'D' << 24 | (uint32)'F' << 16 |
                                                (uint32)'L' << 8 | (uint32)'T';
                                                const uint32 dflt_int = (uint32)'d' << 24 | (uint32)'f' << 16 |
                                                (uint32)'l' << 8 | (uint32)'t';
						if ((scripts[s] == DFLT_int) && (langs[l] == dflt_int)) {
						  has_DFLT = 1;
						} else {
						  snprintf(key,sizeof key,"%c%c%c%c %c%c%c%c",
							 scripts[s]>>24, scripts[s]>>16, scripts[s]>>8, scripts[s],
							 langs[l]>>24, langs[l]>>16, langs[l]>>8, langs[l] );
						  g_tree_insert( ht, copy(key), "" );
						}
					    }
					}
				}
			}
		    }
		}
	    }
	}
    }
    if (has_DFLT) { dump_header_languagesystem_hash_fe((gpointer)"DFLT dflt", (gpointer)"", (gpointer)out); }
    g_tree_foreach( ht, dump_header_languagesystem_hash_fe, out );
    fprintf( out, "\n" );
}

static void dump_gsubgpos(FILE *out, SplineFont *sf) {
    int isgpos;
    int i,l,s, subl;
    OTLookup *otl;
    FeatureScriptLangList *fl;
    struct scriptlanglist *sl;
    struct otffeatname *fn;
    struct otfname *on;

    for ( isgpos=0; isgpos<2; ++isgpos ) {
	uint32 *feats = SFFeaturesInScriptLang(sf,isgpos,0xffffffff,0xffffffff);
	if ( feats[0]!=0 ) {
	    uint32 *scripts = SFScriptsInLookups(sf,isgpos);
	    fprintf( out, "\n# %s \n\n", isgpos ? "GPOS" : "GSUB" );
	    note_nested_lookups_used_twice(isgpos ? sf->gpos_lookups : sf->gsub_lookups);
	    for ( otl= isgpos ? sf->gpos_lookups : sf->gsub_lookups; otl!=NULL; otl=otl->next )
		if ( otl->features!=NULL && !otl->unused )	/* Nested lookups will be output with the lookups which invoke them */
		    dump_lookup( out, sf, otl );
	    for ( i=0; feats[i]!=0; ++i ) {
		fprintf( out, "\nfeature %c%c%c%c {\n", feats[i]>>24, feats[i]>>16, feats[i]>>8, feats[i] );
		if ( feats[i]>=CHR('s','s','0','1') &&  feats[i]<=CHR('s','s','2','0') &&
			(fn = findotffeatname(feats[i],sf))!=NULL ) {
		    fprintf( out, "  featureNames {\n" );
		    for ( on = fn->names; on!=NULL; on=on->next ) {
			fprintf( out, "    name 3 1 0x%x \"", on->lang );
			UniOut(out,on->name );
			fprintf( out, "\";\n" );
		    }
		    fprintf( out, "  };\n" );
		}
		if ( feats[i]==CHR('s','i','z','e') ) {
		    struct otfname *nm;
		    fprintf( out, "  parameters %.1f", sf->design_size/10.0 );
		    if ( sf->fontstyle_id!=0 ) {
			fprintf( out, " %d %.1f %.1f;\n",
				sf->fontstyle_id,
			        sf->design_range_bottom/10.0, sf->design_range_top/10.0 );
			for ( nm = sf->fontstyle_name; nm!=NULL; nm=nm->next ) {
			    char *pt;
			    fprintf( out, "  sizemenuname 3 1 0x%x \"", nm->lang );
			    for ( pt = nm->name; ; ) {
				int ch = utf8_ildb((const char **) &pt);
			        if ( ch=='\0' )
			    break;
				if ( ch>=' ' && ch<=0x7f && ch!='"' && ch!='\\' )
				    putc(ch,out);
				else
				    fprintf( out, "\\%04x", ch );
			    }
			    fprintf( out, "\";\n" );
			}
		    } else
			fprintf( out, " 0 0 0;\n" );
		} else for ( s=0; scripts[s]!=0; ++s ) {
		    uint32 *langs = SFLangsInScript(sf,isgpos,scripts[s]);
		    int firsts = true;
		    for ( l=0; langs[l]!=0; ++l ) {
			int first = true;
			for ( otl = isgpos ? sf->gpos_lookups : sf->gsub_lookups; otl!=NULL; otl=otl->next ) {
			    for ( fl=otl->features; fl!=NULL; fl=fl->next ) if ( fl->featuretag==feats[i] ) {
				for ( sl=fl->scripts; sl!=NULL; sl=sl->next ) if ( sl->script==scripts[s] ) {
				    for ( subl=0; subl<sl->lang_cnt; ++subl ) {
					uint32 lang = subl<MAX_LANG ? sl->langs[subl] : sl->morelangs[subl-MAX_LANG];
			                if ( lang==langs[l] )
			    goto found;
				    }
				}
			    }
			    found:
			    if ( fl!=NULL ) {
				if ( firsts ) {
				    fprintf( out, "\n script %c%c%c%c;\n",
					    scripts[s]>>24, scripts[s]>>16, scripts[s]>>8, scripts[s] );
				    firsts = false;
				}
				if ( first ) {
				    fprintf( out, "     language %c%c%c%c %s;\n",
					    langs[l]>>24, langs[l]>>16, langs[l]>>8, langs[l],
					    langs[l]!=DEFAULT_LANG ? "exclude_dflt" : "" );
				    first = false;
				}
				fprintf( out, "      lookup %s;\n", lookupname(otl));
			    }
			}
		    }
		    free(langs);
		}
		fprintf( out, "} %c%c%c%c;\n", feats[i]>>24, feats[i]>>16, feats[i]>>8, feats[i] );
	    }
	    free(scripts);
	}
	free(feats);
    }
}

/* Sadly the lookup names I generate are often not distinguishable in the */
/*  first 31 characters. So provide a fall-back mechanism if we get a name */
/*  collision */
static void preparenames(SplineFont *sf) {
    int isgpos, cnt, try, i;
    OTLookup *otl;
    char **names, *name;
    char namebuf[MAXG+1], featbuf[8], scriptbuf[8], *feat, *script;
    struct scriptlanglist *sl;

    cnt = 0;
    for ( isgpos=0; isgpos<2; ++isgpos )
	for ( otl=isgpos ? sf->gpos_lookups : sf->gsub_lookups; otl!=NULL; otl=otl->next )
	    ++cnt;
    if ( cnt==0 )
return;
    names = malloc(cnt*sizeof(char *));
    featbuf[4] = scriptbuf[4] = 0;
    cnt = 0;
    for ( isgpos=0; isgpos<2; ++isgpos ) {
	for ( otl=isgpos ? sf->gpos_lookups : sf->gsub_lookups; otl!=NULL; otl=otl->next ) {
	    name = lookupname(otl);
	    for ( try=0; ; ++try ) {
		for ( i=0; i<cnt; ++i )
		    if ( strcmp(name,names[i])==0 )
		break;
		if ( i==cnt ) {
		    /* It's a unique name, use it */
		    otl->tempname = names[cnt++] = copy(name);
	    break;
		}

		feat = ""; script = "";
		if ( otl->features!=NULL ) {
		    featbuf[0] = otl->features->featuretag>>24;
		    featbuf[1] = otl->features->featuretag>>16;
		    featbuf[2] = otl->features->featuretag>>8;
		    featbuf[3] = otl->features->featuretag&0xff;
		    feat = featbuf;
		    for ( sl=otl->features->scripts; sl!=NULL; sl=sl->next ) {
			if ( sl->script!=DEFAULT_SCRIPT ) {
			    scriptbuf[0] = sl->script>>24;
			    scriptbuf[1] = sl->script>>16;
			    scriptbuf[2] = sl->script>>8;
			    scriptbuf[3] = sl->script&0xff;
			    script = scriptbuf;
		    break;
			}
		    }
		}
		sprintf( namebuf, "%s_%s_%s%s_%d", isgpos ? "pos" : "sub",
			    otl->lookup_type== gsub_single ? "single" :
			    otl->lookup_type== gsub_multiple ? "mult" :
			    otl->lookup_type== gsub_alternate ? "alt" :
			    otl->lookup_type== gsub_ligature ? "ligature" :
			    otl->lookup_type== gsub_context ? "context" :
			    otl->lookup_type== gsub_contextchain ? "chain" :
			    otl->lookup_type== gsub_reversecchain ? "reversecc" :
			    otl->lookup_type== gpos_single ? "single" :
			    otl->lookup_type== gpos_pair ? "pair" :
			    otl->lookup_type== gpos_cursive ? "cursive" :
			    otl->lookup_type== gpos_mark2base ? "mark2base" :
			    otl->lookup_type== gpos_mark2ligature ? "mark2liga" :
			    otl->lookup_type== gpos_mark2mark ? "mark2mark" :
			    otl->lookup_type== gpos_context ? "context" :
			    otl->lookup_type== gpos_contextchain ? "chain" :
				"unknown",
			feat, script, try++ );
		name = namebuf;
	    }
	}
    }
    free(names);
}

static void cleanupnames(SplineFont *sf) {
    int isgpos;
    OTLookup *otl;

    for ( isgpos=0; isgpos<2; ++isgpos )
	for ( otl=isgpos ? sf->gpos_lookups : sf->gsub_lookups; otl!=NULL; otl=otl->next ) {
	    free( otl->tempname );
	    otl->tempname = NULL;
	}
}

void FeatDumpFontLookups(FILE *out,SplineFont *sf) {

    if ( sf->cidmaster!=NULL ) sf=sf->cidmaster;

    SFFindUnusedLookups(sf);


    locale_t tmplocale; locale_t oldlocale; // Declare temporary locale storage.
    switch_to_c_locale(&tmplocale, &oldlocale); // Switch to the C locale temporarily and cache the old locale.
    untick_lookups(sf);
    preparenames(sf);
    gdef_markclasscheck(out,sf,NULL);
    dump_header_languagesystem(out,sf);
    dump_gsubgpos(out,sf);
    dump_gdef(out,sf);
    dump_base(out,sf);
    cleanupnames(sf);
    switch_to_old_locale(&tmplocale, &oldlocale); // Switch to the cached locale.
}


/* ************************************************************************** */
/* ******************************* Parse feat ******************************* */
/* ************************************************************************** */

#include <gfile.h>

struct nameid {
    uint16 strid;
    uint16 platform, specific, language;
    char *utf8_str;
    struct nameid *next;
};

struct tablekeywords {
    char *name;
    int size;			/* 1=>byte, 2=>short, 4=>int32 */
    int cnt;			/* normally 1, but 10 for panose, -1 for infinite */
    int offset;			/* -1 => parse but don't store */
};
#define TABLEKEYWORDS_EMPTY { NULL, 0, 0, 0 }

struct tablevalues {
    int index;			/* in the table key structure above */
    int value;
    uint8 panose_vals[10];
    struct tablevalues *next;
};

enum feat_type { ft_lookup_start, ft_lookup_end, ft_feat_start, ft_feat_end,
    ft_table, ft_names, ft_gdefclasses, ft_lcaret, ft_tablekeys,
    ft_sizeparams,
    ft_subtable, ft_script, ft_lang, ft_lookupflags, ft_langsys,
    ft_pst, ft_pstclass, ft_fpst, ft_ap, ft_lookup_ref, ft_featname };
struct feat_item {
    uint16 /* enum feat_type */ type;
    uint8 ticked;
    union {
	SplineChar *sc;		/* For psts, aps */
	char *class;		/* List of glyph names for kerning by class, lcarets */
	char *lookup_name;	/* for lookup_start/ lookup_ref */
	uint32 tag;		/* for feature/script/lang tag */
	int *params;		/* size params */
	struct tablekeywords *offsets;
	char **gdef_classes;
    } u1;
    union {
	PST *pst;
		/* For kerning by class we'll generate an invalid pst with the class as the "paired" field */
	FPST *fpst;
	AnchorPoint *ap;
	int lookupflags;		/* Low order: traditional flags, High order: markset index */
	struct scriptlanglist *sl;	/* Default langsyses for features/langsys */
	int exclude_dflt;		/* for lang tags */
	struct nameid *names;		/* size params */
	struct tablevalues *tvals;
	int16 *lcaret;
	struct otffeatname *featnames;
    } u2;
    struct gpos_mark *mclass;		/* v1.8 For mark to base-ligature-mark, names of all marks which attach to this anchor */
    struct feat_item *next, *lookup_next;
};

static int strcmpD(const void *_str1, const void *_str2) {
    const char *str1 = *(const char **)_str1, *str2 = *(const char **) _str2;
return( strcmp(str1,str2));
}

/* Order glyph classes just so we can do a simple string compare to check for */
/*  class match. So the order doesn't really matter, just so it is consistent */
static char *fea_canonicalClassOrder(char *class) {
    int name_cnt, i;
    char *pt, **names, *cpt;
    char *temp = copy(class);

    name_cnt = 0;
    for ( pt = class; ; ) {
	while ( *pt==' ' ) ++pt;
	if ( *pt=='\0' )
    break;
	for ( ; *pt!=' ' && *pt!='\0'; ++pt );
	++name_cnt;
    }

    names = malloc(name_cnt*sizeof(char *));
    name_cnt = 0;
    for ( pt = temp; ; ) {
	while ( *pt==' ' ) ++pt;
	if ( *pt=='\0' )
    break;
	for ( names[name_cnt++]=pt ; *pt!=' ' && *pt!='\0'; ++pt );
	if ( *pt==' ' )
	    *pt++ = '\0';
    }

    qsort(names,name_cnt,sizeof(char *),strcmpD);
    cpt = class;
    for ( i=0; i<name_cnt; ++i ) {
	strcpy(cpt,names[i]);
	cpt += strlen(cpt);
	*cpt++ = ' ';
    }
    if ( name_cnt!=0 )
	cpt[-1] = '\0';
    free(names);
    free(temp);

return( class );
}

#ifdef FF_UTHASH_GLIF_NAMES
#include "glif_name_hash.h"
#endif

static int fea_classesIntersect(char *class1, char *class2) {
    char *pt1, *start1, *pt2, *start2;
    int ch1, ch2;

#ifdef FF_UTHASH_GLIF_NAMES
    struct glif_name_index _glif_name_hash;
    struct glif_name_index * glif_name_hash = &_glif_name_hash; // Open the hash table.
    memset(glif_name_hash, 0, sizeof(struct glif_name_index));
    long int index = 0;
    long int break_point = 0;
    int output = 0;
    if (class1[0] == '\0' || class2[0] == '\0') return 0; // We cancel further action if one list is blank.
    // Parse the first input.
    for ( pt1=class1 ; output == 0 && pt1[0] != '\0'; ) {
        while ( *pt1==' ' ) ++pt1;
        for ( start1 = pt1; *pt1!=' ' && *pt1!='\0'; ++pt1 );
        ch1 = *pt1; *pt1 = '\0'; // Cache the byte and terminate.
        // We do not want to add the same name twice. It breaks the hash.
        if (glif_name_search_glif_name(glif_name_hash, start1) == NULL) {
          glif_name_track_new(glif_name_hash, index++, start1);
        }
        *pt1 = ch1; // Restore the byte.
    }
    break_point = index; // Divide the entries from the two sources by index.
    // Parse the second input.
    for ( pt2=class2 ; output == 0 && pt2[0] != '\0'; ) {
        while ( *pt2==' ' ) ++pt2;
        for ( start2 = pt2; *pt2!=' ' && *pt2!='\0'; ++pt2 );
        ch2 = *pt2; *pt2 = '\0'; // Cache the byte and terminate.
        struct glif_name * tmp = NULL;
        if ((tmp = glif_name_search_glif_name(glif_name_hash, start2)) == NULL) {
          glif_name_track_new(glif_name_hash, index++, start2);
        } else if (tmp->gid < break_point) {
          output = 1;
        }
        *pt2 = ch2; // Restore the byte.
    }
    glif_name_hash_destroy(glif_name_hash); // Close the hash table.
    if (output == 1) return 1;
    return 0;
#else
    for ( pt1=class1 ; ; ) {
        while ( *pt1==' ' ) ++pt1;
        if ( *pt1=='\0' )
            return( 0 );
        for ( start1 = pt1; *pt1!=' ' && *pt1!='\0'; ++pt1 );
        ch1 = *pt1; *pt1 = '\0';
        for ( pt2=class2 ; ; ) {
            while ( *pt2==' ' ) ++pt2;
            if ( *pt2=='\0' )
                break;
            for ( start2 = pt2; *pt2!=' ' && *pt2!='\0'; ++pt2 );
            ch2 = *pt2; *pt2 = '\0';
            if ( strcmp(start1,start2)==0 ) {
                *pt2 = ch2; *pt1 = ch1;
                return( 1 );
            }
            *pt2 = ch2;
        }
        *pt1 = ch1;
    }
#endif
}


#define SKIP_SPACES(s, i)                       \
    do {                                        \
        while ((s)[i] == ' ')                   \
            i++;                                \
    }                                           \
    while (0)

#define FIND_SPACE(s, i)                        \
    do {                                        \
        while ((s)[i] != ' ' && (s)[i] != '\0') \
            i++;                                \
    }                                           \
    while (0)


static char *fea_classesSplit(char *class1, char *class2) {
    char *intersection;
    int len = strlen(class1), len2 = strlen(class2);
    int ix;
    int i, j, i_end, j_end;
    int length;
    int match_found;

    if ( len2>len ) len = len2;
    intersection = malloc(len+1);
    ix = 0;

    i = 0;
    SKIP_SPACES(class1, i);
    while (class1[i] != '\0') {
        i_end = i;
        FIND_SPACE(class1, i_end);

        length = i_end - i;

        match_found = 0;
        j = 0;
        SKIP_SPACES(class2, j);
        while (!match_found && class2[j] != '\0') {
            j_end = j;
            FIND_SPACE(class2, j_end);

            if (length == j_end - j && strncmp(class1 + i, class2 + j, length) == 0) {
                match_found = 1;

                if (ix != 0) {
                    intersection[ix] = ' ';
                    ix++;
                }
                memcpy(intersection + ix, class1 + i, length * sizeof (char));
                ix += length;

                SKIP_SPACES(class1, i_end);
                memmove(class1 + i, class1 + i_end, (strlen(class1 + i_end) + 1) * sizeof (char));
                SKIP_SPACES(class2, j_end);
                memmove(class2 + j, class2 + j_end, (strlen(class2 + j_end) + 1) * sizeof (char));
            } else {
                j = j_end;
                SKIP_SPACES(class2, j);
            }
        }
        if (!match_found) {
            i = i_end;
            SKIP_SPACES(class1, i);
        }
    }
    intersection[ix] = '\0';
    return( intersection );
}

enum toktype { tk_name, tk_class, tk_int, tk_char, tk_cid, tk_eof,
/* keywords */
	       tk_firstkey,
	       tk_anchor=tk_firstkey, tk_anonymous, tk_by, tk_caret, tk_cursive, tk_device,
	       tk_enumerate, tk_excludeDFLT, tk_exclude_dflt, tk_feature, tk_from,
	       tk_ignore, tk_ignoreDFLT, tk_ignoredflt, tk_IgnoreBaseGlyphs,
	       tk_IgnoreLigatures, tk_IgnoreMarks, tk_include, tk_includeDFLT,
	       tk_include_dflt, tk_language, tk_languagesystem, tk_lookup,
	       tk_lookupflag, tk_mark, tk_nameid, tk_NULL, tk_parameters, tk_position,
	       tk_required, tk_RightToLeft, tk_script, tk_substitute, tk_subtable,
	       tk_table, tk_useExtension,
/* Additional keywords in the 2008 draft */
	       tk_anchorDef, tk_valueRecordDef, tk_contourpoint,
	       tk_MarkAttachmentType, tk_UseMarkFilteringSet,
	       tk_markClass, tk_reversesub, tk_base, tk_ligature, tk_ligComponent,
	       tk_featureNames
};

struct glyphclasses {
    char *classname, *glyphs;
    struct glyphclasses *next;
};

struct namedanchor {
    char *name;
    AnchorPoint *ap;
    struct namedanchor *next;
};

struct namedvalue {
    char *name;
    struct vr *vr;
    struct namedvalue *next;
};

struct gdef_mark { char *name; int index; char *glyphs; };

/* GPOS mark classes may have multiple definitions each added a glyph
 * class and anchor, these are linked under "same" */
struct gpos_mark {
    char *name;
    char *glyphs;
    AnchorPoint *ap;
    struct gpos_mark *same, *next;
    int name_used;	/* Same "markClass" can be used in any mark type lookup, or indeed in multiple lookups of the same type */
} *gpos_mark;

#define MAXT	80
#define MAXI	5
struct parseState {
    char tokbuf[MAXT+1];
    long value;
    enum toktype type;
    uint32 tag;
    int could_be_tag;
    FILE *inlist[MAXI];
    int inc_depth;
    int line[MAXI];
    char *filename[MAXI];
    int err_count;
    unsigned int warned_about_not_cid: 1;
    unsigned int lookup_in_sf_warned: 1;
    unsigned int in_vkrn: 1;
    unsigned int backedup: 1;
    unsigned int skipping: 1;
    SplineFont *sf;
    struct scriptlanglist *def_langsyses;
    struct glyphclasses *classes; // TODO: This eventually needs to merge with the SplineFont group storage. For now, it needs to copy from it at first invocation.
    struct namedanchor *namedAnchors;
    struct namedvalue *namedValueRs;
    struct feat_item *sofar;
    int base;			/* normally numbers are base 10, but in the case of languages in stringids, they can be octal or hex */
    OTLookup *created, *last;	/* Ordered, but not sorted into GSUB, GPOS yet */
    AnchorClass *accreated;
    int gm_cnt[2], gm_max[2], gm_pos[2];
    struct gdef_mark *gdef_mark[2];
    struct gpos_mark *gpos_mark;
};

static struct keywords {
    char *name;
    enum toktype tok;
} fea_keywords[] = {
/* non-keyword tokens (must come first) */
    { "name", tk_name }, { "glyphclass", tk_class }, { "integer", tk_int },
    { "random character", tk_char}, { "cid", tk_cid }, { "EOF", tk_eof },
/* keywords now */
    { "anchor", tk_anchor },
    { "anonymous", tk_anonymous },
    { "by", tk_by },
    { "caret", tk_caret },
    { "cursive", tk_cursive },
    { "device", tk_device },
    { "enumerate", tk_enumerate },
    { "excludeDFLT", tk_excludeDFLT },
    { "exclude_dflt", tk_exclude_dflt },
    { "feature", tk_feature },
    { "from", tk_from },
    { "ignore", tk_ignore },
    { "IgnoreBaseGlyphs", tk_IgnoreBaseGlyphs },
    { "IgnoreLigatures", tk_IgnoreLigatures },
    { "IgnoreMarks", tk_IgnoreMarks },
    { "include", tk_include },
    { "includeDFLT", tk_includeDFLT },
    { "include_dflt", tk_include_dflt },
    { "language", tk_language },
    { "languagesystem", tk_languagesystem },
    { "lookup", tk_lookup },
    { "lookupflag", tk_lookupflag },
    { "mark", tk_mark },
    { "nameid", tk_nameid },
    { "NULL", tk_NULL },
    { "parameters", tk_parameters },
    { "position", tk_position },
    { "required", tk_required },
    { "RightToLeft", tk_RightToLeft },
    { "script", tk_script },
    { "substitute", tk_substitute },
    { "subtable", tk_subtable },
    { "table", tk_table },
    { "useExtension", tk_useExtension },
/* Additional keywords in the 2008 draft */
    { "anchorDef", tk_anchorDef },
    { "valueRecordDef", tk_valueRecordDef },
    { "contourpoint", tk_contourpoint },
    { "MarkAttachmentType", tk_MarkAttachmentType },
    { "UseMarkFilteringSet", tk_UseMarkFilteringSet },
    { "reversesub", tk_reversesub },
    { "markClass", tk_markClass },
    { "base", tk_base },
    { "ligature", tk_ligature },
    { "ligComponent", tk_ligComponent },
    { "featureNames", tk_featureNames },
/* synonyms */
    { "sub", tk_substitute },
    { "pos", tk_position },
    { "rsub", tk_reversesub },
    { "enum", tk_enumerate },
    { "anon", tk_anonymous },
    { NULL, 0 }
};

static struct tablekeywords hhead_keys[] = {
    { "CaretOffset", sizeof(short), 1, -1 },		/* Don't even know what this is! */
    { "Ascender", sizeof(short), 1, offsetof(struct pfminfo,hhead_ascent)+offsetof(SplineFont,pfminfo) },
    { "Descender", sizeof(short), 1, offsetof(struct pfminfo,hhead_descent)+offsetof(SplineFont,pfminfo) },
    { "LineGap", sizeof(short), 1, offsetof(struct pfminfo,linegap)+offsetof(SplineFont,pfminfo) },
    TABLEKEYWORDS_EMPTY
};

static struct tablekeywords vhead_keys[] = {
    { "VertTypoAscender", sizeof(short), 1, -1 },
    { "VertTypoDescender", sizeof(short), 1, -1 },
    { "VertTypoLineGap", sizeof(short), 1, offsetof(struct pfminfo,vlinegap)+offsetof(SplineFont,pfminfo) },
    TABLEKEYWORDS_EMPTY
};

static struct tablekeywords os2_keys[] = {
    { "FSType", sizeof(short), 1, offsetof(struct pfminfo,fstype)+offsetof(SplineFont,pfminfo) },
    { "Panose", sizeof(uint8), 10, offsetof(struct pfminfo,panose)+offsetof(SplineFont,pfminfo) },
    { "UnicodeRange", sizeof(short), -1, -1 },
    { "CodePageRange", sizeof(short), -1, -1 },
    { "TypoAscender", sizeof(short), 1, offsetof(struct pfminfo,os2_typoascent)+offsetof(SplineFont,pfminfo) },
    { "TypoDescender", sizeof(short), 1, offsetof(struct pfminfo,os2_typodescent)+offsetof(SplineFont,pfminfo) },
    { "TypoLineGap", sizeof(short), 1, offsetof(struct pfminfo,os2_typolinegap)+offsetof(SplineFont,pfminfo) },
    { "winAscent", sizeof(short), 1, offsetof(struct pfminfo,os2_winascent)+offsetof(SplineFont,pfminfo) },
    { "winDescent", sizeof(short), 1, offsetof(struct pfminfo,os2_windescent)+offsetof(SplineFont,pfminfo) },
    { "XHeight", sizeof(short), 1, -1 },
    { "CapHeight", sizeof(short), 1, -1 },
    { "WeightClass", sizeof(short), 1, offsetof(struct pfminfo,weight)+offsetof(SplineFont,pfminfo) },
    { "WidthClass", sizeof(short), 1, offsetof(struct pfminfo,width)+offsetof(SplineFont,pfminfo) },
    { "Vendor", sizeof(short), 1, offsetof(struct pfminfo,os2_vendor)+offsetof(SplineFont,pfminfo) },
    TABLEKEYWORDS_EMPTY
};


static void fea_ParseTok(struct parseState *tok);

static void fea_handle_include(struct parseState *tok) {
    FILE *in;
    char namebuf[1025], *pt, *filename;
    int ch;

    fea_ParseTok(tok);
    if ( tok->type!=tk_char || tok->tokbuf[0]!='(' ) {
	LogError(_("Unparseable include on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
return;
    }

    in = tok->inlist[tok->inc_depth];
    ch = getc(in);
    while ( isspace(ch))
	ch = getc(in);
    pt = namebuf;
    while ( ch!=EOF && ch!=')' && pt<namebuf+sizeof(namebuf)-1 ) {
	*pt++ = ch;
	ch = getc(in);
    }
    if ( ch!=EOF && ch!=')' ) {
	while ( ch!=EOF && ch!=')' )
	    ch = getc(in);
	LogError(_("Include filename too long on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    }
    while ( pt>=namebuf+1 && isspace(pt[-1]) )
	--pt;
    *pt = '\0';
    if ( ch!=')' ) {
	if ( ch==EOF )
	    LogError(_("End of file in include on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	else
	    LogError(_("Missing close parenthesis in include on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
return;
    }

    if ( pt==namebuf ) {
	LogError(_("No filename specified in include on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
return;
    }

    if ( tok->inc_depth>=MAXI-1 ) {
	LogError(_("Includes nested too deeply on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
return;
    }

    if ( *namebuf=='/' ||
	    ( pt = strrchr(tok->filename[tok->inc_depth],'/') )==NULL )
	filename=copy(namebuf);
    else {
	*pt = '\0';
	filename = GFileAppendFile(tok->filename[tok->inc_depth],namebuf,false);
	*pt = '/';
    }
    in = fopen(filename,"r");
    if ( in==NULL ) {
	LogError(_("Could not open include file (%s) on line %d of %s"),
		filename, tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	free(filename);
return;
    }

    ++tok->inc_depth;
    tok->filename[tok->inc_depth] = filename;
    tok->inlist[tok->inc_depth] = in;
    tok->line[tok->inc_depth] = 1;
    fea_ParseTok(tok);
}

static void fea_ParseTokWithKeywords(struct parseState *tok, int do_keywords) {
    FILE *in = tok->inlist[tok->inc_depth];
    int ch, peekch;
    char *pt, *start;

    if ( tok->backedup ) {
	tok->backedup = false;
return;
    }

  skip_whitespace:
    ch = getc(in);
    while ( isspace(ch) || ch=='#' ) {
	if ( ch=='#' )
	    while ( (ch=getc(in))!=EOF && ch!='\n' && ch!='\r' );
	if ( ch=='\n' || ch=='\r' ) {
	    if ( ch=='\r' ) {
		ch = getc(in);
		if ( ch!='\n' )
		    ungetc(ch,in);
	    }
	    ++tok->line[tok->inc_depth];
	}
	ch = getc(in);
    }

    tok->could_be_tag = 0;
    if ( ch==EOF ) {
	if ( tok->inc_depth>0 ) {
	    fclose(tok->inlist[tok->inc_depth]);
	    free(tok->filename[tok->inc_depth]);
	    in = tok->inlist[--tok->inc_depth];
  goto skip_whitespace;
	}
	tok->type = tk_eof;
	strcpy(tok->tokbuf,"EOF");
return;
    }

    start = pt = tok->tokbuf;
    if ( ch=='\\' || ch=='-' ) {
	peekch=getc(in);
	ungetc(peekch,in);
    }

    if ( isdigit(ch) || ch=='+' || ((ch=='-' || ch=='\\') && isdigit(peekch)) ) {
	tok->type = tk_int;
	if ( ch=='-' || ch=='+' ) {
	    if ( ch=='-' ) {
		*pt++ = ch;
		start = pt;
	    }
	    ch = getc(in);
	} else if ( ch=='\\' ) {
	    ch = getc(in);
	    tok->type = tk_cid;
	}
	while ( (isdigit( ch ) ||
		(tok->base==0 && (ch=='x' || ch=='X' || (ch>='a' && ch<='f') || (ch>='A' && ch<='F'))))
		&& pt<tok->tokbuf+15 ) {
	    *pt++ = ch;
	    ch = getc(in);
	}
	if ( isdigit(ch)) {
	    LogError(_("Number too long on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	} else if ( pt==start ) {
	    LogError(_("Missing number on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	}
	ungetc(ch,in);
	*pt = '\0';
	tok->value = strtol(tok->tokbuf,NULL,tok->base);
return;
    } else if ( ch=='@' || ch=='_' || ch=='\\' || isalnum(ch) || ch=='.') {	/* Most names can't start with dot */
	int check_keywords = true;
	tok->type = tk_name;
	if ( ch=='@' ) {
	    tok->type = tk_class;
	    *pt++ = ch;
	    start = pt;
	    ch = getc(in);
	    check_keywords = false;
	} else if ( ch=='\\' ) {
	    ch = getc(in);
	    check_keywords = false;
	}
	while ( isalnum(ch) || ch=='_' || ch=='.' || (ch=='-' && tok->type==tk_class) ) {
	    if ( pt<tok->tokbuf+MAXT )
		*pt++ = ch;
	    ch = getc(in);
	}
	*pt = '\0';
	ungetc(ch,in);
	// We are selective about names starting with a dot. .notdef and .null are acceptable.
	if ((start[0] == '.') && (strcmp(start, ".notdef") != 0) && (strcmp(start, ".null") != 0)) {
	    if ( !tok->skipping ) {
		LogError(_("Unexpected character (0x%02X) on line %d of %s"), start[0], tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
	    }
	} else {
		/* Adobe says glyphnames are 31 chars, but Mangal uses longer names */
		if ( pt>start+MAXG ) {
		    LogError(_("Name, %s%s, too long on line %d of %s"),
			    tok->tokbuf, pt>=tok->tokbuf+MAXT?"...":"",
			    tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		    if ( pt>=tok->tokbuf+MAXT )
			++tok->err_count;
		} else if ( pt==start ) {
		    LogError(_("Missing name on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		    ++tok->err_count;
		}

		if ( check_keywords && do_keywords) {
		    int i;
		    for ( i=tk_firstkey; fea_keywords[i].name!=NULL; ++i ) {
			if ( strcmp(fea_keywords[i].name,tok->tokbuf)==0 ) {
			    tok->type = fea_keywords[i].tok;
		    break;
			}
		    }
		    if ( tok->type==tk_include )
			fea_handle_include(tok);
		}
		if ( tok->type==tk_name && pt-tok->tokbuf<=4 && pt!=tok->tokbuf ) {
		    unsigned char tag[4];
		    tok->could_be_tag = true;
		    memset(tag,' ',4);
		    tag[0] = tok->tokbuf[0];
		    if ( tok->tokbuf[1]!='\0' ) {
			tag[1] = tok->tokbuf[1];
			if ( tok->tokbuf[2]!='\0' ) {
			    tag[2] = tok->tokbuf[2];
			    if ( tok->tokbuf[3]!='\0' )
				tag[3] = tok->tokbuf[3];
			}
		    }
		    tok->tag = (tag[0]<<24) | (tag[1]<<16) | (tag[2]<<8) | tag[3];
		}
	}
    } else {
	/* I've already handled the special characters # @ and \ */
	/*  so don't treat them as errors here, if they occur they will be out of context */
	if ( ch==';' || ch==',' || ch=='-' || ch=='=' || ch=='\'' || ch=='"' ||
		ch=='{' || ch=='}' ||
		ch=='[' || ch==']' ||
		ch=='<' || ch=='>' ||
		ch=='(' || ch==')' ) {
	    tok->type = tk_char;
	    tok->tokbuf[0] = ch;
	    tok->tokbuf[1] = '\0';
	} else {
	    if ( !tok->skipping ) {
		LogError(_("Unexpected character (0x%02X) on line %d of %s"), ch, tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
	    }
  goto skip_whitespace;
	}
    }
}

static void fea_ParseTok(struct parseState *tok) {
    fea_ParseTokWithKeywords(tok, true);
}

static void fea_ParseTag(struct parseState *tok) {
    /* The tag used for OS/2 doesn't get parsed properly */
    /* So if we know we are looking for a tag do some fixups */

    fea_ParseTok(tok);
    if ( tok->type==tk_name && tok->could_be_tag &&
	    tok->tag==CHR('O','S',' ',' ') ) {
	FILE *in = tok->inlist[tok->inc_depth];
	int ch;
	ch = getc(in);
	if ( ch=='/' ) {
	    ch = getc(in);
	    if ( ch=='2' ) {
		tok->tag = CHR('O','S','/','2');
	    } else {
		tok->tag = CHR('O','S','/',' ');
		ungetc(ch,in);
	    }
	} else
	    ungetc(ch,in);
    }
    if ( tok->type!=tk_name && tok->type!=tk_eof &&
	    strlen(tok->tokbuf)==4 && isalnum(tok->tokbuf[0])) {
	tok->type = tk_name;
	tok->could_be_tag = true;
	tok->tag = CHR(tok->tokbuf[0], tok->tokbuf[1], tok->tokbuf[2], tok->tokbuf[3]);
    }
}

static void fea_UnParseTok(struct parseState *tok) {
    tok->backedup = true;
}

static int fea_ParseDeciPoints(struct parseState *tok) {
    /* When parsing size features floating point numbers are allowed */
    /*  but they should be converted to ints by multiplying by 10 */
    /* (not my convention) */

    fea_ParseTok(tok);
    if ( tok->type==tk_int ) {
	FILE *in = tok->inlist[tok->inc_depth];
	char *pt = tok->tokbuf + strlen(tok->tokbuf);
	int ch;
	ch = getc(in);
	if ( ch=='.' ) {
	    *pt++ = ch;
	    while ( (ch = getc(in))!=EOF && isdigit(ch)) {
		if ( pt<tok->tokbuf+sizeof(tok->tokbuf)-1 )
		    *pt++ = ch;
	    }
	    *pt = '\0';
	    tok->value = rint(strtod(tok->tokbuf,NULL)*10.0);
	}
	if ( ch!=EOF )
	    ungetc(ch,in);
    } else {
	LogError(_("Expected '%s' on line %d of %s"), fea_keywords[tk_int].name,
		tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	tok->value = -1;
    }
return( tok->value );
}

static void fea_TokenMustBe(struct parseState *tok, enum toktype type, int ch) {
    int tk;

    fea_ParseTok(tok);
    if ( type==tk_char && (tok->type!=type || tok->tokbuf[0]!=ch) ) {
	LogError(_("Expected '%c' on line %d of %s"), ch, tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    } else if ( type!=tk_char && tok->type!=type ) {
	for ( tk=0; fea_keywords[tk].name!=NULL; ++tk )
	    if ( fea_keywords[tk].tok==type )
	break;
	if ( fea_keywords[tk].name!=NULL )
	    LogError(_("Expected '%s' on line %d of %s"), fea_keywords[tk].name,
		    tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	else
	    LogError(_("Expected unknown token (internal error) on line %d of %s"),
		    tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    }
}

static void fea_skip_to_semi(struct parseState *tok) {
    int nest=0;

    while ( tok->type!=tk_char || tok->tokbuf[0]!=';' || nest>0 ) {
	fea_ParseTok(tok);
	if ( tok->type==tk_char ) {
	    if ( tok->tokbuf[0]=='{' ) ++nest;
	    else if ( tok->tokbuf[0]=='}' ) --nest;
	    if ( nest<0 )
    break;
	}
	if ( tok->type==tk_eof )
    break;
    }
}

static void fea_skip_to_close_curly(struct parseState *tok) {
    int nest=0;

    tok->skipping = true;
    /* The table blocks have slightly different syntaxes and can take strings */
    /* and floating point numbers. So don't complain about unknown chars when */
    /*  in a table (that's skipping) */
    while ( tok->type!=tk_char || tok->tokbuf[0]!='}' || nest>0 ) {
	fea_ParseTok(tok);
	if ( tok->type==tk_char ) {
	    if ( tok->tokbuf[0]=='{' ) ++nest;
	    else if ( tok->tokbuf[0]=='}' ) --nest;
	}
	if ( tok->type==tk_eof )
    break;
    }
    tok->skipping = false;
}

static void fea_now_semi(struct parseState *tok) {
    if ( tok->type!=tk_char || tok->tokbuf[0]!=';' ) {
	LogError(_("Expected ';' at statement end on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	fea_skip_to_semi(tok);
	++tok->err_count;
return;
    }
}

static void fea_end_statement(struct parseState *tok) {
    fea_ParseTok(tok);
    fea_now_semi(tok);
}

static struct glyphclasses *fea_lookup_class(struct parseState *tok,char *classname) {
    struct glyphclasses *test;

    for ( test=tok->classes; test!=NULL; test=test->next ) {
	if ( strcmp(classname,test->classname)==0 )
return( test );
    }
return( NULL );
}

static char *fea_lookup_class_complain(struct parseState *tok,char *classname) {
    struct glyphclasses *test;
    struct gpos_mark *mtest;

    for ( test=tok->classes; test!=NULL; test=test->next ) {
	if ( strcmp(classname,test->classname)==0 )
return( copy( test->glyphs) );
    }

    /* Mark classes can also be used as normal classes */
    for ( mtest=tok->gpos_mark; mtest!=NULL; mtest=mtest->next ) {
	if ( strcmp(classname,mtest->name)==0 ) {
	    struct gpos_mark *sames;
	    int len=0;
	    char *ret, *pt;
	    for ( sames=mtest; sames!=NULL; sames=sames->same )
		len += strlen(sames->glyphs)+1;
	    pt = ret = malloc(len+1);
	    for ( sames=mtest; sames!=NULL; sames=sames->same ) {
		strcpy(pt,sames->glyphs);
		pt += strlen(pt);
		if ( sames->next!=NULL )
		    *pt++ = ' ';
	    }
return( ret );
	}
    }

    LogError(_("Use of undefined glyph class, %s, on line %d of %s"), classname, tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
    ++tok->err_count;
return( NULL );
}

static struct gpos_mark *fea_lookup_markclass_complain(struct parseState *tok,char *classname) {
    struct gpos_mark *test;

    for ( test=tok->gpos_mark; test!=NULL; test=test->next ) {
	if ( strcmp(classname,test->name)==0 )
return( test );
    }
    LogError(_("Use of undefined mark class, %s, on line %d of %s"), classname, tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
    ++tok->err_count;
return( NULL );
}

static void fea_AddClassDef(struct parseState *tok,char *classname,char *contents) {
    struct glyphclasses *test;

    test = fea_lookup_class(tok,classname);
    if ( test==NULL ) {
	test=chunkalloc(sizeof(struct glyphclasses));
	test->classname = classname;
	test->next = tok->classes;
	tok->classes = test;
    } else {
	free(classname);
	free(test->glyphs);
    }
    test->glyphs = contents;
}

static int fea_AddGlyphs(char **_glyphs, int *_max, int cnt, char *contents ) {
    int len = strlen(contents);
    char *glyphs = *_glyphs;
    /* Append a glyph name, etc. to a glyph class */

    if ( glyphs==NULL ) {
	glyphs = copy(contents);
	cnt = *_max = len;
    } else {
	if ( *_max-cnt <= len+1 )
	    glyphs = realloc(glyphs,(*_max+=200+len+1)+1);
	glyphs[cnt++] = ' ';
	strcpy(glyphs+cnt,contents);
	cnt += strlen(contents);
    }
    free(contents);
    *_glyphs = glyphs;
return( cnt );
}

static char *fea_cid_validate(struct parseState *tok,int cid) {
    int i, max;
    SplineFont *maxsf;
    SplineChar *sc;
    EncMap *map;

    if ( tok->sf->subfontcnt==0 ) {
	if ( !tok->warned_about_not_cid ) {
	    LogError(_("Reference to a CID in a non-CID-keyed font on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    tok->warned_about_not_cid = true;
	}
	++tok->err_count;
return(NULL);
    }
    max = 0; maxsf = NULL;
    for ( i=0; i<tok->sf->subfontcnt; ++i ) {
	SplineFont *sub = tok->sf->subfonts[i];
	if ( cid<sub->glyphcnt && sub->glyphs[cid]!=NULL )
return( sub->glyphs[cid]->name );
	if ( sub->glyphcnt>max ) {
	    max = sub->glyphcnt;
	    maxsf = sub;
	}
    }
    /* Not defined, try to create it */
    if ( maxsf==NULL )		/* No subfonts */
return( NULL );
    if ( cid>=maxsf->glyphcnt ) {
	struct cidmap *cidmap = FindCidMap(tok->sf->cidregistry,tok->sf->ordering,tok->sf->supplement,tok->sf);
	if ( cidmap==NULL || cid>=MaxCID(cidmap) )
return( NULL );
	SFExpandGlyphCount(maxsf,MaxCID(cidmap));
    }
    if ( cid>=maxsf->glyphcnt )
return( NULL );
    map = EncMap1to1(maxsf->glyphcnt);
    sc = SFMakeChar(maxsf,map,cid);
    EncMapFree(map);
    if ( sc==NULL )
return( NULL );
return( copy( sc->name ));
}

static SplineChar *fea_glyphname_get(struct parseState *tok,char *name) {
    SplineFont *sf = tok->sf;
    EncMap *map = sf->fv==NULL ? sf->map : sf->fv->map;
    SplineChar *sc = SFGetChar(sf,-1,name);
    int enc, gid;

    if ( sf->subfontcnt!=0 ) {
	LogError(_("Reference to a glyph name in a CID-keyed font on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
return(sc);
    }

    if ( sc!=NULL || strcmp(name,"NULL")==0 )
return( sc );
    enc = SFFindSlot(sf,map,-1,name);
    if ( enc!=-1 ) {
#if 0
	sc = SFMakeChar(sf,map,enc);
	if ( sc!=NULL ) {
	    sc->widthset = true;
	    free(sc->name);
	    sc->name = copy(name);
	}
#else
	sc = SFGetChar(sf,enc,NULL);
#endif // 0
	if (sc != NULL) return( sc );
    }

    // It is unclear why the first call to SFGetChar would not find this.
    for ( gid=sf->glyphcnt-1; gid>=0; --gid ) if ( (sc=sf->glyphs[gid])!=NULL ) {
	if ( strcmp(sc->name,name)==0 )
return( sc );
    }

#if 0
// Adding a blank glyph based upon a bad reference in a feature file seems to be bad practice.
// And the method of extending the encoding here is dangerous.
/* Not in the encoding, so add it */
    enc = map->enccount;
    sc = SFMakeChar(sf,map,enc);
    if ( sc!=NULL ) {
	sc->widthset = true;
	free(sc->name);
	sc->name = copy(name);
	sc->unicodeenc = UniFromName(name,ui_none,&custom);
    }
return( sc );
#else
    LogError(_("Reference to a non-existent glyph name on line %d of %s: %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth], name);
    return NULL;
#endif // 0
}

static char *fea_glyphname_validate(struct parseState *tok,char *name) {
    SplineChar *sc = fea_glyphname_get(tok,name);

    if ( sc==NULL )
return( NULL );

return( copy( sc->name ));
}

static char *fea_ParseGlyphClass(struct parseState *tok) {
    char *glyphs = NULL;

    if ( tok->type==tk_class ) {
	// If the class references another class, just copy that.
	glyphs = fea_lookup_class_complain(tok,tok->tokbuf);
    } else if ( tok->type!=tk_char || tok->tokbuf[0]!='[' ) {
	// If it is not a class, we want a list to parse. Anything else is wrong.
	LogError(_("Expected '[' in glyph class definition on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
return( NULL );
    } else {
	// Start parsing the list.
	char *contents = NULL; // This is a temporary buffer used for each cycle below.
	int cnt=0, max=0;
	int last_val, range_type, range_len;
	char last_glyph[MAXT+1];
	char *pt1, *start1, *pt2, *start2;
	int v1, v2;

	last_val = -1; last_glyph[0] = '\0';
	for (;;) {
	    fea_ParseTok(tok);
	    if ( tok->type==tk_char && tok->tokbuf[0]==']' )
	break; // End of list.
	    if ( tok->type==tk_class ) {
		// Stash the entire contents of the referenced class for inclusion in this class (later).
		contents = fea_lookup_class_complain(tok,tok->tokbuf);
		last_val=-1; last_glyph[0] = '\0';
	    } else if ( tok->type==tk_cid ) {
		last_val = tok->value; last_glyph[0] = '\0';
		contents = fea_cid_validate(tok,tok->value);
	    } else if ( tok->type==tk_name ) {
		strcpy(last_glyph,tok->tokbuf); last_val = -1;
		contents = fea_glyphname_validate(tok,tok->tokbuf);
	    } else if ( tok->type==tk_char && tok->tokbuf[0]=='-' ) {
		// It's a range extending from the previous token.
		fea_ParseTok(tok);
		if ( last_val!=-1 && tok->type==tk_cid ) {
		    if ( last_val>=tok->value ) {
			LogError(_("Invalid CID range in glyph class on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
			++tok->err_count;
		    }
		    /* Last val has already been added to the class */
		    /* and we'll add the current value later */
		    for ( ++last_val; last_val<tok->value; ++last_val ) {
			contents = fea_cid_validate(tok,last_val);
			if ( contents!=NULL ) {
			    cnt = fea_AddGlyphs(&glyphs,&max,cnt,contents); contents = NULL;
			}
		    }
		    contents = fea_cid_validate(tok,tok->value);
		} else if ( last_glyph[0]!='\0' && tok->type==tk_name ) {
		    range_type=0;
		    if ( strlen(last_glyph)==strlen(tok->tokbuf) &&
			    strcmp(last_glyph,tok->tokbuf)<0 ) {
			start1=NULL;
			for ( pt1=last_glyph, pt2=tok->tokbuf;
				*pt1!='\0'; ++pt1, ++pt2 ) {
			    if ( *pt1!=*pt2 ) {
				if ( start1!=NULL ) {
				    range_type=0;
			break;
				}
			        start1 = pt1; start2 = pt2;
			        if ( !isdigit(*pt1) || !isdigit(*pt2))
				    range_type = 1;
				else {
				    for ( range_len=0; range_len<3 && isdigit(*pt1) && isdigit(*pt2);
					    ++range_len, ++pt1, ++pt2 );
				    range_type = 2;
			            --pt1; --pt2;
				}
			    }
			}
		    }
		    if ( range_type==0 ) {
			LogError(_("Invalid glyph name range in glyph class on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
			++tok->err_count;
		    } else if ( range_type==1 || range_len==1 ) {
			/* Single letter changes */
			v1 = *start1; v2 = *start2;
			for ( ++v1; v1<=v2; ++v1 ) {
			    sprintf( last_glyph, "%.*s%c%s", (int) (start2-tok->tokbuf),
			             tok->tokbuf, v1, start2+1);
			    contents = fea_glyphname_validate(tok,last_glyph);
			    if ( v1==v2 )
			break;
			    if ( contents!=NULL ) {
				cnt = fea_AddGlyphs(&glyphs,&max,cnt,contents); contents = NULL;
			    }
			}
		    } else {
			v1 = strtol(start1,NULL,10);
			v2 = strtol(start2,NULL,10);
			for ( ++v1; v1<=v2; ++v1 ) {
			    if ( range_len==2 )
				sprintf( last_glyph, "%.*s%02d%s", (int) (start2-tok->tokbuf),
					tok->tokbuf, v1, start2+2 );
			    else
				sprintf( last_glyph, "%.*s%03d%s", (int) (start2-tok->tokbuf),
					tok->tokbuf, v1, start2+3 );
			    contents = fea_glyphname_validate(tok,last_glyph);
			    if ( v1==v2 )
			break;
			    if ( contents!=NULL ) {
				cnt = fea_AddGlyphs(&glyphs,&max,cnt,contents); contents = NULL;
			    }
			}
		    }
		} else {
		    LogError(_("Unexpected token in glyph class range on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		    ++tok->err_count;
		    if ( tok->type==tk_char && tok->tokbuf[0]==']' )
	break;
		}
		last_val=-1; last_glyph[0] = '\0';
	    } else if ( tok->type == tk_NULL ) {
		contents = copy("NULL");
	    } else {
		LogError(_("Expected glyph name, cid, or class in glyph class definition on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
	break;
	    }
	    if ( contents!=NULL ) {
		cnt = fea_AddGlyphs(&glyphs,&max,cnt,contents); contents = NULL;
	    }
	}
	if ( glyphs==NULL )
	    glyphs = copy("");	/* Is it legal to have an empty class? I can't think of any use for one */
    }
return( glyphs );
}

static char *fea_ParseGlyphClassGuarded(struct parseState *tok) {
    char *ret = fea_ParseGlyphClass(tok);
    if ( ret==NULL )
	ret = copy("");
return( ret );
}

static int fea_ParseMarkAttachClass(struct parseState *tok, int is_set) {
    int i;
    SplineFont *sf = tok->sf;
    char *glyphs;

    for ( i=0; i<tok->gm_cnt[is_set]; ++i ) {
	if ( strcmp(tok->tokbuf,tok->gdef_mark[is_set][i].name)==0 )
return( tok->gdef_mark[is_set][i].index << 8 );
    }
    glyphs = fea_lookup_class_complain(tok,tok->tokbuf);
    if ( glyphs==NULL )
return( 0 );
    if ( tok->gm_cnt[is_set]>=tok->gm_max[is_set] ) {
	tok->gdef_mark[is_set] = realloc(tok->gdef_mark[is_set],(tok->gm_max[is_set]+=30)*sizeof(struct gdef_mark));
	if ( tok->gm_pos[is_set]==0 ) {
	    memset(&tok->gdef_mark[is_set][0],0,sizeof(tok->gdef_mark[is_set][0]));
	    if ( is_set )
		tok->gm_pos[is_set] = sf->mark_set_cnt;
	    else
		tok->gm_pos[is_set] = sf->mark_class_cnt==0 ? 1 : sf->mark_class_cnt;
	}
    }
    tok->gdef_mark[is_set][tok->gm_cnt[is_set]].name   = copy(tok->tokbuf);
    tok->gdef_mark[is_set][tok->gm_cnt[is_set]].glyphs = glyphs;
    /* see if the mark class is already in the font? */
    if ( is_set ) {
	for ( i=sf->mark_set_cnt-1; i>=0; --i ) {
	    if ( strcmp(sf->mark_set_names[i],tok->tokbuf+1)==0 ||
		    strcmp(sf->mark_sets[i],glyphs)==0 )
	break;
	}
    } else {
	for ( i=sf->mark_class_cnt-1; i>0; --i ) {
	    if ( strcmp(sf->mark_class_names[i],tok->tokbuf+1)==0 ||
		    strcmp(sf->mark_classes[i],glyphs)==0 )
	break;
	}
	if ( i==0 ) i=-1;
    }
    if ( i>=0 )
	tok->gdef_mark[is_set][tok->gm_cnt[is_set]].index = i;
    else
	tok->gdef_mark[is_set][tok->gm_cnt[is_set]].index = tok->gm_pos[is_set]++;
    if ( is_set )
return( (tok->gdef_mark[is_set][tok->gm_cnt[is_set]++].index << 16) | pst_usemarkfilteringset );
    else
return( tok->gdef_mark[is_set][tok->gm_cnt[is_set]++].index << 8 );
}

static void fea_ParseLookupFlags(struct parseState *tok) {
    int val = 0;
    struct feat_item *item;

    fea_ParseTok(tok);
    if ( tok->type==tk_int ) {
	val = tok->value;
	fea_end_statement(tok);
    } else {
	while ( tok->type==tk_RightToLeft || tok->type==tk_IgnoreBaseGlyphs ||
		tok->type==tk_IgnoreMarks || tok->type==tk_IgnoreLigatures ||
		tok->type==tk_MarkAttachmentType || tok->type==tk_UseMarkFilteringSet ) {
	    if ( tok->type == tk_RightToLeft )
		val |= pst_r2l;
	    else if ( tok->type == tk_IgnoreBaseGlyphs )
		val |= pst_ignorebaseglyphs;
	    else if ( tok->type == tk_IgnoreMarks )
		val |= pst_ignorecombiningmarks;
	    else if ( tok->type == tk_IgnoreLigatures )
		val |= pst_ignoreligatures;
	    else {
		int is_set = tok->type == tk_UseMarkFilteringSet;
		fea_TokenMustBe(tok,tk_class,'\0');
		val |= fea_ParseMarkAttachClass(tok,is_set);
	    }
	    fea_ParseTok(tok);
	    if ( tok->type == tk_char && tok->tokbuf[0]==';' )
	break;
	    else if ( tok->type==tk_char && tok->tokbuf[0]!=',' ) {
		LogError(_("Expected ';' in lookupflags on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
		fea_skip_to_semi(tok);
	break;
	    }
	    fea_ParseTok(tok);
	}
	if ( tok->type != tk_char || tok->tokbuf[0]!=';' ) {
	    LogError(_("Unexpected token in lookupflags on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	    fea_skip_to_semi(tok);
	} else if ( val==0 ) {
	    LogError(_("No flags specified in lookupflags on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	}
    }

    item = chunkalloc(sizeof(struct feat_item));
    item->type = ft_lookupflags;
    item->u2.lookupflags = val;
    item->next = tok->sofar;
    tok->sofar = item;
}

static void fea_ParseGlyphClassDef(struct parseState *tok) {
    char *classname = copy(tok->tokbuf );
    char *contents;

    fea_ParseTok(tok);
    if ( tok->type!=tk_char || tok->tokbuf[0]!='=' ) {
	LogError(_("Expected '=' in glyph class definition on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	fea_skip_to_semi(tok);
return;
    }
    fea_ParseTok(tok);
    contents = fea_ParseGlyphClass(tok); // Make a list of referenced glyphs.
    if ( contents==NULL ) {
	fea_skip_to_semi(tok);
return;
    }
    fea_AddClassDef(tok,classname,copy(contents)); // Put the list into a class.
    fea_end_statement(tok);
}

static void fea_ParseLangSys(struct parseState *tok, int inside_feat) {
    uint32 script, lang;
    struct scriptlanglist *sl;
    int l;

    fea_ParseTok(tok);
    if ( tok->type!=tk_name || !tok->could_be_tag ) {
	LogError(_("Expected tag in languagesystem on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	fea_skip_to_semi(tok);
return;
    }
    script = tok->tag;

    fea_ParseTok(tok);
    if ( tok->type!=tk_name || !tok->could_be_tag ) {
	LogError(_("Expected tag in languagesystem on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	fea_skip_to_semi(tok);
return;
    }
    lang = tok->tag;

    for ( sl=tok->def_langsyses; sl!=NULL && sl->script!=script; sl=sl->next );
    if ( sl==NULL ) {
	sl = chunkalloc(sizeof(struct scriptlanglist));
	sl->script = script;
	sl->next = tok->def_langsyses;
	tok->def_langsyses = sl;
    }
    for ( l=0; l<sl->lang_cnt; ++l ) {
	uint32 language = l<MAX_LANG ? sl->langs[l] : sl->morelangs[l-MAX_LANG];
	if ( language==lang )
    break;
    }
    if ( l<sl->lang_cnt )
	/* Er... this combination is already in the list. I guess that's ok */;
    else if ( sl->lang_cnt<MAX_LANG )
	sl->langs[sl->lang_cnt++] = lang;
    else {
	sl->morelangs = realloc(sl->morelangs,(sl->lang_cnt+1)*sizeof(uint32));
	sl->morelangs[sl->lang_cnt++ - MAX_LANG] = lang;
    }
    fea_end_statement(tok);

    if ( inside_feat ) {
	struct feat_item *item = chunkalloc(sizeof(struct feat_item));
	item->type = ft_langsys;
	item->u2.sl = SListCopy(tok->def_langsyses);
	item->next = tok->sofar;
	tok->sofar = item;
    }
}

struct apmark {
    AnchorPoint *ap;
    struct gpos_mark *mark_class;
    uint16 mark_count;
};

struct ligcomponent {
    int apm_cnt;
    struct apmark *apmark;
};

struct markedglyphs {
    unsigned int has_marks: 1;		/* Are there any marked glyphs in the entire sequence? */
    unsigned int is_cursive: 1;		/* Only in a position sequence */
    unsigned int is_mark: 1;		/* Only in a position sequence/mark keyword=>mark2mark */
    unsigned int is_lookup: 1;		/* Or a lookup when parsing a subs replacement list */
    unsigned int is_mark2base: 1;
    unsigned int is_mark2mark: 1;
    unsigned int is_mark2lig: 1;
    unsigned int is_name: 1;		/* Otherwise a class */
    unsigned int hidden_marked_glyphs: 1;/* for glyphs with marked marks in a mark2base sequence */
    uint16 mark_count;			/* 0=>unmarked, 1=>first mark, etc. */
    char *name_or_class;		/* Glyph name / class contents */
    struct vr *vr;			/* A value record. Only in position sequences */
    int ap_cnt;				/* Number of anchor points */
    AnchorPoint **anchors;
    int apm_cnt;
    struct apmark *apmark;
    int lc_cnt;
    struct ligcomponent *ligcomp;
    char *lookupname;
    struct markedglyphs *next;
};

static void NameIdFree(struct nameid *nm);

static void fea_ParseDeviceTable(struct parseState *tok,DeviceTable *adjust)
	{
    int first = true;
    int min=0, max= -1;
    int8 values[512];

    memset(values,0,sizeof(values));

    fea_TokenMustBe(tok,tk_device,'\0');
    if ( tok->type!=tk_device )
return;

    for (;;) {
	fea_ParseTok(tok);
	if ( first && tok->type==tk_NULL ) {
	    fea_TokenMustBe(tok,tk_char,'>');
    break;
	} else if ( tok->type==tk_char && tok->tokbuf[0]=='>' ) {
    break;
	} else if ( tok->type!=tk_int ) {
	    LogError(_("Expected integer in device table on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	} else {
	    int pixel = tok->value;
	    fea_TokenMustBe(tok,tk_int,'\0');
	    if ( pixel>=sizeof(values) || pixel<0 )
		LogError(_("Pixel size too big in device table on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    else {
		values[pixel] = tok->value;
		if ( max==-1 )
		    min=max=pixel;
		else if ( pixel<min ) min = pixel;
		else if ( pixel>max ) max = pixel;
	    }
	    fea_ParseTok(tok);
	    if ( tok->type==tk_char && tok->tokbuf[0]==',' )
		/* That's right... */;
	    else if ( tok->type==tk_char && tok->tokbuf[0]=='>' )
    break;
	    else {
		LogError(_("Expected comma in device table on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
	    }
	}
	first = false;
    }
    if ( max!=-1 ) {
	int i;
	adjust->first_pixel_size = min;
	adjust->last_pixel_size = max;
	adjust->corrections = malloc(max-min+1);
	for ( i=min; i<=max; ++i )
	    adjust->corrections[i-min] = values[i];
    }
}

static void fea_ParseCaret(struct parseState *tok) {
    int val=0;

    fea_TokenMustBe(tok,tk_caret,'\0');
    if ( tok->type!=tk_caret )
return;
    fea_ParseTok(tok);
    if ( tok->type!=tk_int ) {
	LogError(_("Expected integer in caret on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    } else
	val = tok->value;
    fea_ParseTok(tok);
    if ( tok->type!=tk_char || tok->tokbuf[0]!='>' ) {
	LogError(_("Expected '>' in caret on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    }
    tok->value = val;
}

static AnchorPoint *fea_ParseAnchor(struct parseState *tok) {
    AnchorPoint *ap = NULL;
    struct namedanchor *nap;

    if ( tok->type==tk_anchor || tok->type==tk_anchorDef ) {
	fea_ParseTok(tok);
	if ( tok->type==tk_NULL ) {
	    ap = NULL;
	    fea_ParseTok(tok);
	} else if ( tok->type==tk_name ) {
	    for ( nap=tok->namedAnchors; nap!=NULL; nap=nap->next ) {
		if ( strcmp(nap->name,tok->tokbuf)==0 ) {
		    ap = AnchorPointsCopy(nap->ap);
	    break;
		}
	    }
	    if ( nap==NULL ) {
		LogError(_("\"%s\" is not the name of a known named anchor on line %d of %s."),
			tok->tokbuf, tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
	    }
	    fea_ParseTok(tok);
	} else if ( tok->type==tk_int ) {
	    ap = chunkalloc(sizeof(AnchorPoint));
	    ap->me.x = tok->value;
	    fea_TokenMustBe(tok,tk_int,'\0');
	    ap->me.y = tok->value;
	    fea_ParseTok(tok);
	    if ( tok->type == tk_contourpoint )
		fea_TokenMustBe(tok,tk_int,' ');
	    /* FF had a bug and produced anchor points with x y c instead of x y <contourpoint c> */
	    if ( tok->type==tk_int ) {
		ap->ttf_pt_index = tok->value;
		ap->has_ttf_pt = true;
		fea_ParseTok(tok);
	    } else if ( tok->type==tk_char && tok->tokbuf[0]=='<' ) {
		fea_ParseTok(tok);
		if ( tok->type == tk_contourpoint ) {
		    fea_TokenMustBe(tok,tk_int,' ');
		    ap->ttf_pt_index = tok->value;
		    ap->has_ttf_pt = true;
		    fea_TokenMustBe(tok,tk_int,'>');
		} else {
		    fea_UnParseTok(tok);
		    fea_ParseDeviceTable(tok,&ap->xadjust);
		    fea_TokenMustBe(tok,tk_char,'<');
		    fea_ParseDeviceTable(tok,&ap->yadjust);
		}
		fea_ParseTok(tok);
	    }
	} else {
	    LogError(_("Expected integer in anchor on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	}
    } else {
	LogError(_("Expected 'anchor' keyword in anchor on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    }
return( ap );
}

static AnchorPoint *fea_ParseAnchorClosed(struct parseState *tok) {
    int ecnt = tok->err_count;
    AnchorPoint *ap = fea_ParseAnchor(tok);
    if ( tok->err_count==ecnt && ( tok->type!=tk_char || tok->tokbuf[0]!='>' )) {
	LogError(_("Expected '>' in anchor on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    }
return( ap );
}

static void fea_ParseAnchorDef(struct parseState *tok) {
    AnchorPoint *ap;
    struct namedanchor *nap;

    ap = fea_ParseAnchor(tok);
    if ( tok->type!=tk_name ) {
	LogError(_("Expected name in anchor definition on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	fea_skip_to_semi(tok);
return;
    }
    for ( nap=tok->namedAnchors; nap!=NULL; nap=nap->next )
	if ( strcmp(nap->name,tok->tokbuf)==0 )
    break;
    if ( nap!=NULL ) {
	LogError(_("Attempt to redefine anchor definition of \"%s\" on line %d of %s"),
		tok->tokbuf, tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
    } else {
	nap = chunkalloc(sizeof(struct namedanchor));
	nap->next = tok->namedAnchors;
	tok->namedAnchors = nap;
	nap->name = copy(tok->tokbuf);
    }
    nap->ap = ap;

    fea_end_statement(tok);
}

static int fea_findLookup(struct parseState *tok,char *name ) {
    struct feat_item *feat;

    for ( feat=tok->sofar; feat!=NULL; feat=feat->next ) {
	if ( feat->type==ft_lookup_start && strcmp(name,feat->u1.lookup_name)==0 )
return( true );
    }

    if ( SFFindLookup(tok->sf,name)!=NULL ) {
	if ( !tok->lookup_in_sf_warned ) {
	    ff_post_notice(_("Refers to Font"),_("Reference to a lookup which is not in the feature file but which is in the font, %.50s"), name );
	    tok->lookup_in_sf_warned = true;
	}
return( true );
    }

return( false );
}

static struct vr *ValueRecordCopy(struct vr *ovr) {
    struct vr *nvr;

    nvr = chunkalloc(sizeof(*nvr));
    memcpy(nvr,ovr,sizeof(struct vr));
    nvr->adjust = ValDevTabCopy(ovr->adjust);
return( nvr );
}

static struct vr *fea_ParseValueRecord(struct parseState *tok) {
    struct vr *vr=NULL;
    struct namedvalue *nvr;

    if ( tok->type==tk_name ) {
	for ( nvr=tok->namedValueRs; nvr!=NULL; nvr=nvr->next ) {
	    if ( strcmp(nvr->name,tok->tokbuf)==0 ) {
		vr = ValueRecordCopy(nvr->vr);
	break;
	    }
	}
	if ( nvr==NULL ) {
	    LogError(_("\"%s\" is not the name of a known named value record on line %d of %s."),
		    tok->tokbuf, tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	}
	fea_ParseTok(tok);
    } else if ( tok->type==tk_int ) {
	vr = chunkalloc(sizeof( struct vr ));
	vr->xoff = tok->value;
	fea_ParseTok(tok);
	if ( tok->type==tk_int ) {
	    vr->yoff = tok->value;
	    fea_TokenMustBe(tok,tk_int,'\0');
	    vr->h_adv_off = tok->value;
	    fea_TokenMustBe(tok,tk_int,'\0');
	    vr->v_adv_off = tok->value;
	    fea_ParseTok(tok);
	    if ( tok->type==tk_char && tok->tokbuf[0]=='<' ) {
		vr->adjust = chunkalloc(sizeof(struct valdev));
		fea_ParseDeviceTable(tok,&vr->adjust->xadjust);
		fea_TokenMustBe(tok,tk_char,'<');
		fea_ParseDeviceTable(tok,&vr->adjust->yadjust);
		fea_TokenMustBe(tok,tk_char,'<');
		fea_ParseDeviceTable(tok,&vr->adjust->xadv);
		fea_TokenMustBe(tok,tk_char,'<');
		fea_ParseDeviceTable(tok,&vr->adjust->yadv);
		fea_ParseTok(tok);
	    }
	} else if ( tok->type==tk_char && tok->tokbuf[0]=='>' ) {
	    if ( tok->in_vkrn )
		vr->v_adv_off = vr->xoff;
	    else
		vr->h_adv_off = vr->xoff;
	    vr->xoff = 0;
	}
    } else {
	LogError(_("Unexpected token in value record on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    }
return( vr );
}

static void fea_ParseValueRecordDef(struct parseState *tok) {
    struct vr *vr;
    struct namedvalue *nvr;

    fea_ParseTok(tok);
    vr = fea_ParseValueRecord(tok);
    if ( tok->type!=tk_name ) {
	LogError(_("Expected name in value record definition on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	fea_skip_to_semi(tok);
return;
    }
    for ( nvr=tok->namedValueRs; nvr!=NULL; nvr=nvr->next )
	if ( strcmp(nvr->name,tok->tokbuf)==0 )
    break;
    if ( nvr!=NULL ) {
	LogError(_("Attempt to redefine value record definition of \"%s\" on line %d of %s"),
		tok->tokbuf, tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
    } else {
	nvr = chunkalloc(sizeof(struct namedanchor));
	nvr->next = tok->namedValueRs;
	tok->namedValueRs = nvr;
	nvr->name = copy(tok->tokbuf);
    }
    nvr->vr = vr;

    fea_end_statement(tok);
}

static void fea_ParseMarkClass(struct parseState *tok) {
    char *class_string;
    AnchorPoint *ap;
    struct gpos_mark *gm, *ngm;

    fea_ParseTok(tok);
    if ( tok->type==tk_name )
        class_string = fea_glyphname_validate(tok,tok->tokbuf);
    else if ( tok->type==tk_cid )
        class_string = fea_cid_validate(tok,tok->value);
    else if ( tok->type == tk_class ||
              ( tok->type==tk_char && tok->tokbuf[0]=='[' ))
        class_string = fea_ParseGlyphClassGuarded(tok);
    else {
        LogError(_("Expected name or class on line %d of %s"),
            tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
        ++tok->err_count;
        fea_skip_to_semi(tok);
return;
    }
    fea_ParseTok(tok);
    if ( tok->type!=tk_char || tok->tokbuf[0]!='<' ) {
	LogError(_("Expected anchor in mark class definition on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	fea_skip_to_semi(tok);
return;
    }
    fea_ParseTok(tok);
    ap = fea_ParseAnchorClosed(tok);
    fea_ParseTok(tok);

    if ( tok->type!=tk_class ) {
	LogError(_("Expected class name in mark class definition on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	fea_skip_to_semi(tok);
return;
    }
    gm = chunkalloc(sizeof(*gm));
    gm->glyphs = class_string;
    gm->ap = ap;
    for ( ngm=tok->gpos_mark; ngm!=NULL; ngm=ngm->next )
	if ( strcmp(ngm->name,tok->tokbuf)==0 )
    break;
    if ( ngm!=NULL ) {
	/* Multiple anchor points for the same name */
	gm->same = ngm->same;
	ngm->same = gm;
    } else {
	gm->next = tok->gpos_mark;
	tok->gpos_mark = gm;
	gm->name = copy(tok->tokbuf);
    }

    fea_end_statement(tok);
}

static void fea_ParseBroket(struct parseState *tok,struct markedglyphs *last) {
    /* We've read the open broket. Might find: value record, anchor, lookup */
    /* (lookups are my extension) */

    fea_ParseTok(tok);
    if ( tok->type==tk_lookup ) {
	/* This is a concept I introduced, Adobe has done something similar */
	/*  but, of course incompatible */
	fea_TokenMustBe(tok,tk_name,'\0');
	if ( last->mark_count==0 ) {
	    LogError(_("Lookups may only be specified after marked glyphs on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	}
	if ( !fea_findLookup(tok,tok->tokbuf) ) {
	    LogError(_("Lookups must be defined before being used on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	} else
	    last->lookupname = copy(tok->tokbuf);
	fea_TokenMustBe(tok,tk_char,'>');
    } else if ( tok->type==tk_anchor ) {
	last->anchors = realloc(last->anchors,(++last->ap_cnt)*sizeof(AnchorPoint *));
	last->anchors[last->ap_cnt-1] = fea_ParseAnchorClosed(tok);
    } else if ( tok->type==tk_NULL ) {
	/* NULL value record. Adobe documents it and doesn't implement it */
	/* Not sure what it's good for */
	fea_TokenMustBe(tok,tk_char,'>');
    } else if ( tok->type==tk_int || tok->type==tk_name ) {
	last->vr = fea_ParseValueRecord(tok);
	if ( tok->type!=tk_char || tok->tokbuf[0]!='>' ) {
	    LogError(_("Expected '>' in value record on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	}
    } else {
	LogError(_("Unexpected token in value record on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    }
}

struct mark_state {
    int last_mark;
    int mark_cnt;
};

static struct markedglyphs *fea_parseCursiveSequence(struct parseState *tok,
	int allow_marks, struct mark_state *mark_state) {
    struct markedglyphs *cur;
    char *contents;

    fea_ParseTok(tok);
    if ( tok->type==tk_name || tok->type == tk_cid ) {
	if ( tok->type == tk_name )
	    contents = fea_glyphname_validate(tok,tok->tokbuf);
	else
	    contents = fea_cid_validate(tok,tok->value);
	if ( contents!=NULL ) {
	    cur = chunkalloc(sizeof(struct markedglyphs));
	    cur->is_cursive = true;
	    cur->is_name = true;
	    cur->name_or_class = contents;
	} else {
	    LogError(_("Expected a valid glyph/CID name on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	    return( NULL );
	}
    } else if ( tok->type == tk_class || (tok->type==tk_char && tok->tokbuf[0]=='[')) {
	cur = chunkalloc(sizeof(struct markedglyphs));
	cur->is_cursive = true;
	cur->is_name = false;
	cur->name_or_class = fea_ParseGlyphClassGuarded(tok);
    } else {
	LogError(_("Expected glyph or glyphclass (after cursive) on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
return( NULL );
    }

    fea_ParseTok(tok);
    if ( allow_marks && tok->type==tk_char && (tok->tokbuf[0]=='\'' || tok->tokbuf[0]=='"') ) {
	if ( mark_state->last_mark!=tok->tokbuf[0] ) {
	    ++(mark_state->mark_cnt);
	    mark_state->last_mark = tok->tokbuf[0];
	}
	cur->mark_count = mark_state->mark_cnt;
	fea_ParseTok(tok);
    }

    if ( tok->type!=tk_char || tok->tokbuf[0]!='<' ) {
	LogError(_("Expected two anchors (after cursive) on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
return( NULL );
    }
    fea_TokenMustBe(tok,tk_anchor,' ');
    cur->anchors = calloc(cur->ap_cnt=2,sizeof(AnchorPoint *));
    cur->anchors[0] = fea_ParseAnchorClosed(tok);
    fea_TokenMustBe(tok,tk_char,'<');
    fea_TokenMustBe(tok,tk_anchor,' ');
    cur->anchors[1] = fea_ParseAnchorClosed(tok);
return( cur );
}

static struct markedglyphs *fea_parseBaseMarkSequence(struct parseState *tok,
	int is_base, int allow_marks, struct mark_state *mark_state) {
    struct markedglyphs *cur;
    char *contents;
    int apm_max=0;

    fea_ParseTok(tok);
    if ( tok->type==tk_name || tok->type == tk_cid ) {
	if ( tok->type == tk_name )
	    contents = fea_glyphname_validate(tok,tok->tokbuf);
	else
	    contents = fea_cid_validate(tok,tok->value);
	if ( contents!=NULL ) {
	    cur = chunkalloc(sizeof(struct markedglyphs));
	    cur->is_name = true;
	    cur->name_or_class = contents;
	}
    } else if ( tok->type == tk_class || (tok->type==tk_char && tok->tokbuf[0]=='[')) {
	cur = chunkalloc(sizeof(struct markedglyphs));
	cur->is_name = false;
	cur->name_or_class = fea_ParseGlyphClassGuarded(tok);
    } else {
	LogError(_("Expected glyph or glyphclass (after cursive) on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
return( NULL );
    }
    cur->is_mark2base = is_base;
    cur->is_mark2mark = !is_base;

    fea_ParseTok(tok);
    if ( allow_marks && tok->type==tk_char && tok->tokbuf[0]=='\'' ) {
	/* only one type of mark in v1.8 */
	cur->mark_count = ++mark_state->mark_cnt;
	fea_ParseTok(tok);
    }

    if ( tok->type!=tk_char || tok->tokbuf[0]!='<' ) {
	LogError(_("Expected an anchor (after base/mark) on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
return( NULL );
    }
    apm_max = 5;
    cur->apmark = calloc(apm_max,sizeof(struct apmark));
    while ( tok->type==tk_char && tok->tokbuf[0]=='<' ) {
	if ( cur->apm_cnt>=apm_max )
	    cur->apmark = realloc(cur->apmark,(apm_max+=5)*sizeof(struct apmark));
	fea_TokenMustBe(tok,tk_anchor,' ');
	cur->apmark[cur->apm_cnt].ap = fea_ParseAnchorClosed(tok);
	fea_TokenMustBe(tok,tk_mark,' ');
	fea_TokenMustBe(tok,tk_class,' ');
	cur->apmark[cur->apm_cnt++].mark_class = fea_lookup_markclass_complain(tok,tok->tokbuf);

	fea_ParseTok(tok);
	if ( allow_marks && tok->type==tk_char && tok->tokbuf[0]=='\'' ) {
	    cur->apmark[cur->apm_cnt-1].mark_count = ++mark_state->mark_cnt;
	    fea_ParseTok(tok);
	}
    }
    fea_UnParseTok(tok);
return( cur );
}

static struct markedglyphs *fea_parseLigatureSequence(struct parseState *tok,
	int allow_marks, struct mark_state *mark_state) {
    struct markedglyphs *cur;
    char *contents;
    int apm_max=0, lc_max=0;
    struct ligcomponent *lc;
    int skip;

    fea_ParseTok(tok);
    if ( tok->type==tk_name || tok->type == tk_cid ) {
	if ( tok->type == tk_name )
	    contents = fea_glyphname_validate(tok,tok->tokbuf);
	else
	    contents = fea_cid_validate(tok,tok->value);
	if ( contents!=NULL ) {
	    cur = chunkalloc(sizeof(struct markedglyphs));
	    cur->is_name = true;
	    cur->name_or_class = contents;
	}
    } else if ( tok->type == tk_class || (tok->type==tk_char && tok->tokbuf[0]=='[')) {
	cur = chunkalloc(sizeof(struct markedglyphs));
	cur->is_name = false;
	cur->name_or_class = fea_ParseGlyphClassGuarded(tok);
    } else {
	LogError(_("Expected glyph or glyphclass (after ligature) on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
return( NULL );
    }
    cur->is_mark2lig = true;

    fea_ParseTok(tok);
    if ( allow_marks && tok->type==tk_char && tok->tokbuf[0]=='\'' ) {
	/* only one type of mark in v1.8 */
	cur->mark_count = ++mark_state->mark_cnt;
	fea_ParseTok(tok);
    }

    if ( tok->type!=tk_char || tok->tokbuf[0]!='<' ) {
	LogError(_("Expected an anchor (after ligature) on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	free(cur->name_or_class); free(cur);
return( NULL );
    }
    lc_max = 8;
    cur->ligcomp = calloc(lc_max,sizeof(struct ligcomponent));
    for (;;) {
	if ( cur->lc_cnt>=lc_max )
	    cur->ligcomp = realloc(cur->ligcomp,(lc_max+=5)*sizeof(struct ligcomponent));
	lc = &cur->ligcomp[cur->lc_cnt++];
	apm_max = 5;
	lc->apm_cnt = 0;
	lc->apmark = calloc(apm_max,sizeof(struct apmark));
	while ( tok->type==tk_char && tok->tokbuf[0]=='<' ) {
	    if ( lc->apm_cnt>=apm_max )
		lc->apmark = realloc(lc->apmark,(apm_max+=5)*sizeof(struct apmark));
	    fea_TokenMustBe(tok,tk_anchor,' ');
	    lc->apmark[lc->apm_cnt].ap = fea_ParseAnchorClosed(tok);
	    /* This isn't documented, but Adobe gives an example of an */
	    /*  <anchor NULL> with no following mark class */
	    skip = 0;
	    if ( lc->apmark[lc->apm_cnt].ap == NULL ) {
		fea_ParseTok(tok);
		fea_UnParseTok(tok);
		if ( tok->type!=tk_mark )
		    skip = true;
	    }
	    if ( !skip ) {
		fea_TokenMustBe(tok,tk_mark,' ');
		fea_TokenMustBe(tok,tk_class,' ');
		lc->apmark[lc->apm_cnt++].mark_class = fea_lookup_markclass_complain(tok,tok->tokbuf);
	    }

	    fea_ParseTok(tok);
	    if ( allow_marks && tok->type==tk_char && tok->tokbuf[0]=='\'' ) {
		lc->apmark[lc->apm_cnt-1].mark_count = ++mark_state->mark_cnt;
		fea_ParseTok(tok);
	    }
	}
	if ( tok->type!=tk_ligComponent )
    break;
	fea_TokenMustBe(tok,tk_char,'<');
    }
    fea_UnParseTok(tok);
return( cur );
}

static struct markedglyphs *fea_ParseMarkedGlyphs(struct parseState *tok,
	int is_pos, int allow_marks, int allow_lookups) {
    struct markedglyphs *head=NULL, *last=NULL, *prev=NULL, *cur;
    char *contents;
/* v1.6 pos <glyph>|<class> <anchor> mark <glyph>|<class> */
/* v1.8 pos base <glyph>|<class> [<anchor> mark <named mark class>]+ */
/* v1.8 pos mark <glyph>|<class> [<anchor> mark <named mark class>]+ */
/* v1.8 pos ligature <glyph>|<class> [<anchor> mark <named mark class>]+ */
/*                 [ligComponent [<anchor> mark <named mark class>]+]* */
    struct mark_state mark_state;

    memset(&mark_state,0,sizeof(mark_state));
    for (;;) {
	fea_ParseTok(tok);
	cur = NULL;
	if ( is_pos && tok->type == tk_cursive )
	    cur = fea_parseCursiveSequence(tok,allow_marks,&mark_state);
	else if ( is_pos && tok->type == tk_mark ) {
	    cur = fea_parseBaseMarkSequence(tok,false,allow_marks,&mark_state);
	} else if ( is_pos && tok->type == tk_base ) {
	    cur = fea_parseBaseMarkSequence(tok,true,allow_marks,&mark_state);
	} else if ( is_pos && tok->type == tk_ligature ) {
	    cur = fea_parseLigatureSequence(tok,allow_marks,&mark_state);
	} else if ( tok->type==tk_name || tok->type == tk_cid ) {
	    if ( tok->type == tk_name )
		contents = fea_glyphname_validate(tok,tok->tokbuf);
	    else
		contents = fea_cid_validate(tok,tok->value);
	    if ( contents!=NULL ) {
		cur = chunkalloc(sizeof(struct markedglyphs));
		cur->is_name = true;
		cur->name_or_class = contents;
	    }
	} else if ( tok->type == tk_class || (tok->type==tk_char && tok->tokbuf[0]=='[')) {
	    cur = chunkalloc(sizeof(struct markedglyphs));
	    cur->is_name = false;
	    cur->name_or_class = fea_ParseGlyphClassGuarded(tok);
	} else if ( allow_marks && tok->type==tk_char &&
		(tok->tokbuf[0]=='\'' || tok->tokbuf[0]=='"') && last!=NULL ) {
	    if ( mark_state.last_mark!=tok->tokbuf[0] || (prev!=NULL && prev->mark_count==0)) {
		++mark_state.mark_cnt;
		mark_state.last_mark = tok->tokbuf[0];
	    }
	    last->mark_count = mark_state.mark_cnt;
	} else if ( is_pos && last!=NULL && last->vr==NULL &&
		tok->type == tk_int ) {
	    last->vr = chunkalloc(sizeof(struct vr));
	    if ( tok->in_vkrn )
		last->vr->v_adv_off = tok->value;
	    else
		last->vr->h_adv_off = tok->value;
	} else if ( is_pos && last!=NULL && tok->type == tk_char &&
		tok->tokbuf[0]=='<' ) {
	    fea_ParseBroket(tok,last);
	} else if ( tok->type == tk_lookup && last!=NULL ) {
	    fea_TokenMustBe(tok,tk_name,'\0');
	    if ( last->mark_count==0 ) {
		LogError(_("Lookups may only be specified after marked glyphs on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
	    }
	    if ( !fea_findLookup(tok,tok->tokbuf) ) {
		LogError(_("Lookups must be defined before being used on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
	    }
	    last->lookupname = copy(tok->tokbuf);
	} else if ( !is_pos && allow_lookups && tok->type == tk_char && tok->tokbuf[0]=='<' ) {
	    /* When I came up with my own syntax I put brokets around the lookup*/
	    /*  token, in keeping, I thought, with the rest of the syntax. */
	    /*  Adobe now allows lookups here, but does not use brokets */
	    /* ParseBroket handles my lookups ... but it only is invoked for is_pos */
	    /*  so this is the substitute case */
	    fea_TokenMustBe(tok,tk_lookup,'\0');
	    fea_TokenMustBe(tok,tk_name,'\0');
	    cur = chunkalloc(sizeof(struct markedglyphs));
	    cur->is_name = false;
	    cur->is_lookup = true;
	    cur->lookupname = copy(tok->tokbuf);
	    fea_TokenMustBe(tok,tk_char,'>');
	} else
    break;
	if ( cur!=NULL ) {
	    prev = last;
	    if ( last==NULL )
		head = cur;
	    else
		last->next = cur;
	    last = cur;
	}
    }
    if ( head!=NULL && mark_state.mark_cnt!=0 )
	head->has_marks = true;
    fea_UnParseTok(tok);
return( head );
}

static void fea_markedglyphsFree(struct markedglyphs *gl) {
    struct markedglyphs *next;
    int i,j;

    while ( gl!=NULL ) {
	next = gl->next;
	free(gl->name_or_class);
	free(gl->lookupname);
	for ( i=0; i<gl->ap_cnt; ++i ) {
	    if ( gl->anchors[i]!=NULL ) {	/* NULL anchors are permitted */
		gl->anchors[i]->next = NULL;
		AnchorPointsFree(gl->anchors[i]);
	    }
	}
	free(gl->anchors);
	for ( i=0; i<gl->apm_cnt; ++i )
	    AnchorPointsFree(gl->apmark[i].ap);
	free(gl->apmark);
	for ( i=0; i<gl->lc_cnt; ++i ) {
	    for ( j=0; j<gl->ligcomp[i].apm_cnt; ++j )
		AnchorPointsFree(gl->ligcomp[i].apmark[j].ap);
	    free( gl->ligcomp[i].apmark);
	}
	free(gl->ligcomp);
	if ( gl->vr!=NULL ) {
	    ValDevFree(gl->vr->adjust);
	    chunkfree(gl->vr,sizeof(struct vr));
	}
	gl = next;
    }
}

static struct feat_item *fea_AddAllLigPosibilities(struct parseState *tok,struct markedglyphs *glyphs,
	SplineChar *sc,char *sequence_start,char *next, struct feat_item *sofar) {
    char *start, *pt, ch;
    SplineChar *temp;
    char *after;
    struct feat_item *item;

    start = glyphs->name_or_class;
    for (;;) {
	while ( *start==' ' ) ++start;
	if ( *start=='\0' )
    break;
	for ( pt=start; *pt!='\0' && *pt!=' '; ++pt );
	ch = *pt; *pt = '\0';
	temp = fea_glyphname_get(tok,start);
	*pt = ch; start = pt;
	if ( temp==NULL )
    continue;
	strcpy(next,temp->name);
	after = next+strlen(next);
	if ( glyphs->next!=NULL && glyphs->next->mark_count == glyphs->mark_count ) {
	    *after++ = ' ';
	    sofar = fea_AddAllLigPosibilities(tok,glyphs->next,sc,sequence_start,after,sofar);
	} else {
	    *after = '\0';
	    item = chunkalloc(sizeof(struct feat_item));
	    item->type = ft_pst;
	    item->next = sofar;
	    sofar = item;
	    item->u1.sc = sc;
	    item->u2.pst = chunkalloc(sizeof(PST));
	    item->u2.pst->type = pst_ligature;
	    item->u2.pst->u.lig.components = copy(sequence_start);
	    item->u2.pst->u.lig.lig = sc;
	}
    }
return( sofar );
}

static struct markedglyphs *fea_glyphs_to_names(struct markedglyphs *glyphs,
	int cnt,char **to) {
    struct markedglyphs *g;
    int len, i;
    char *names, *pt;

    len = 0;
    for ( g=glyphs, i=0; i<cnt; ++i, g=g->next )
	len += strlen( g->name_or_class ) +1;
    names = pt = malloc(len+1);
    for ( g=glyphs, i=0; i<cnt; ++i, g=g->next ) {
	strcpy(pt,g->name_or_class);
	pt += strlen( pt );
	*pt++ = ' ';
    }
    if ( pt!=names )
	pt[-1] = '\0';
    else
	*pt = '\0';
    *to = names;
return( g );
}

static struct feat_item *fea_process_pos_single(struct parseState *tok,
	struct markedglyphs *glyphs, struct feat_item *sofar) {
    char *start, *pt, ch;
    struct feat_item *item;
    SplineChar *sc;

    start = glyphs->name_or_class;
    for (;;) {
	while ( *start==' ' ) ++start;
	if ( *start=='\0' )
    break;
	for ( pt=start; *pt!='\0' && *pt!=' '; ++pt );
	ch = *pt; *pt = '\0';
	sc = fea_glyphname_get(tok,start);
	*pt = ch; start = pt;
	if ( sc!=NULL ) {
	    item = chunkalloc(sizeof(struct feat_item));
	    item->type = ft_pst;
	    item->next = sofar;
	    sofar = item;
	    item->u1.sc = sc;
	    item->u2.pst = chunkalloc(sizeof(PST));
	    item->u2.pst->type = pst_position;
	    item->u2.pst->u.pos = glyphs->vr[0];
	}
    }
return( sofar );
}

static struct feat_item *fea_process_pos_pair(struct parseState *tok,
	struct markedglyphs *glyphs, struct feat_item *sofar, int enumer) {
    char *start, *pt, ch, *start2, *pt2, ch2;
    struct feat_item *item;
    struct vr vr[2];
    SplineChar *sc, *sc2;
    int i;

    memset(vr,0,sizeof(vr));
    if ( glyphs->vr==NULL )
	vr[0] = *glyphs->next->vr;
    else {
	vr[0] = *glyphs->vr;
	if ( glyphs->next->vr!=NULL )
	    vr[1] = *glyphs->next->vr;
    }

    /* FontForge only supports kerning classes in one direction at a time, not full value records */
    /* so if there is a full value record, assume "enumerate" */
    for ( i=0; i<2; ++i ) {
	if ( vr[i].xoff!=0 || vr[i].yoff!=0 || ( vr[i].h_adv_off!=0 && vr[i].v_adv_off!=0 ) )
	    enumer = true;
    }

    if ( enumer || (glyphs->is_name && glyphs->next->is_name)) {
	start = glyphs->name_or_class;
	for (;;) {
	    while ( *start==' ' ) ++start;
	    if ( *start=='\0' )
	break;
	    for ( pt=start; *pt!='\0' && *pt!=' '; ++pt );
	    ch = *pt; *pt = '\0';
	    sc = fea_glyphname_get(tok,start);
	    *pt = ch; start = pt;
	    if ( sc!=NULL ) {
		start2 = glyphs->next->name_or_class;
		for (;;) {
		    while ( *start2==' ' ) ++start2;
		    if ( *start2=='\0' )
		break;
		    for ( pt2=start2; *pt2!='\0' && *pt2!=' '; ++pt2 );
		    ch2 = *pt2; *pt2 = '\0';
		    sc2 = fea_glyphname_get(tok,start2);
		    *pt2 = ch2; start2 = pt2;
		    if ( sc2!=NULL ) {
			item = chunkalloc(sizeof(struct feat_item));
			item->type = ft_pst;
			item->next = sofar;
			sofar = item;
			item->u1.sc = sc;
			item->u2.pst = chunkalloc(sizeof(PST));
			item->u2.pst->type = pst_pair;
			item->u2.pst->u.pair.paired = copy(sc2->name);
			item->u2.pst->u.pair.vr = chunkalloc(sizeof( struct vr[2]));
			memcpy(item->u2.pst->u.pair.vr,vr,sizeof(vr));
		    }
		}
	    }
	}
    } else {
	item = chunkalloc(sizeof(struct feat_item));
	item->type = ft_pstclass;
	item->next = sofar;
	sofar = item;
	item->u1.class = copy(glyphs->name_or_class);
	item->u2.pst = chunkalloc(sizeof(PST));
	item->u2.pst->type = pst_pair;
	item->u2.pst->u.pair.paired = copy(glyphs->next->name_or_class);
	item->u2.pst->u.pair.vr = chunkalloc(sizeof( struct vr[2]));
	memcpy(item->u2.pst->u.pair.vr,vr,sizeof(vr));
    }
return( sofar );
}

static struct feat_item *fea_process_pos_cursive(struct parseState *tok,
	struct markedglyphs *glyphs, struct feat_item *sofar) {
    struct feat_item *item;
    char *start, *pt, ch;
    SplineChar *sc;

    start = glyphs->name_or_class;
    if ( glyphs->anchors[1]!=NULL )
	glyphs->anchors[1]->type = at_cexit;
    for (;;) {
	while ( *start==' ' ) ++start;
	if ( *start=='\0' )
    break;
	for ( pt=start; *pt!='\0' && *pt!=' '; ++pt );
	ch = *pt; *pt = '\0';
	sc = fea_glyphname_get(tok,start);
	*pt = ch; start = pt;
	if ( sc!=NULL ) {
	    item = chunkalloc(sizeof(struct feat_item));
	    item->type = ft_ap;
	    item->next = sofar;
	    sofar = item;
	    item->u1.sc = sc;
	    if ( glyphs->anchors[0]!=NULL ) {
		glyphs->anchors[0]->type = at_centry;
		glyphs->anchors[0]->next = glyphs->anchors[1];
		item->u2.ap = AnchorPointsCopy(glyphs->anchors[0]);
	    } else
		item->u2.ap = AnchorPointsCopy(glyphs->anchors[1]);
	}
    }
return( sofar );
}

static struct feat_item *fea_process_pos_markbase(struct parseState *tok,
	struct markedglyphs *glyphs, struct feat_item *sofar) {
    int i;
    struct feat_item *item;
    char *start, *pt, ch;
    SplineChar *sc;

    start = glyphs->name_or_class;
    for (;;) {
	while ( *start==' ' ) ++start;
	if ( *start=='\0' )
    break;
	for ( pt=start; *pt!='\0' && *pt!=' '; ++pt );
	ch = *pt; *pt = '\0';
	sc = fea_glyphname_get(tok,start);
	*pt = ch; start = pt;
	if ( sc!=NULL ) {
	    for ( i=0; i<glyphs->apm_cnt; ++i ) if ( glyphs->apmark[i].ap!=NULL ) {
		item = chunkalloc(sizeof(struct feat_item));
		item->type = ft_ap;
		item->next = sofar;
		sofar = item;
		item->u1.sc = sc;
		item->u2.ap = AnchorPointsCopy(glyphs->apmark[i].ap);
		if ( glyphs->is_mark2base )
		    item->u2.ap->type = at_basechar;
		else
		    item->u2.ap->type = at_basemark;
		item->mclass = glyphs->apmark[i].mark_class;
	    }
	}
    }
return( sofar );
}

static struct feat_item *fea_process_pos_ligature(struct parseState *tok,
	struct markedglyphs *glyphs, struct feat_item *sofar) {
    int i, lc;
    struct feat_item *item;
    char *start, *pt, ch;
    SplineChar *sc;

    start = glyphs->name_or_class;
    for (;;) {
	while ( *start==' ' ) ++start;
	if ( *start=='\0' )
    break;
	for ( pt=start; *pt!='\0' && *pt!=' '; ++pt );
	ch = *pt; *pt = '\0';
	sc = fea_glyphname_get(tok,start);
	*pt = ch; start = pt;
	if ( sc!=NULL ) {
	    for ( lc=0; lc<glyphs->lc_cnt; ++lc ) {
		struct ligcomponent *ligc = &glyphs->ligcomp[lc];
		for ( i=0; i<ligc->apm_cnt; ++i ) if ( ligc->apmark[i].ap!=NULL ) {
		    item = chunkalloc(sizeof(struct feat_item));
		    item->type = ft_ap;
		    item->next = sofar;
		    sofar = item;
		    item->u1.sc = sc;
		    item->u2.ap = AnchorPointsCopy(ligc->apmark[i].ap);
		    item->u2.ap->type = at_baselig;
		    item->u2.ap->lig_index = lc;
		    item->mclass = ligc->apmark[i].mark_class;
		}
	    }
	}
    }
return( sofar );
}

static struct feat_item *fea_process_sub_single(struct parseState *tok,
	struct markedglyphs *glyphs, struct markedglyphs *rpl,
	struct feat_item *sofar ) {
    char *start, *pt, ch, *start2, *pt2, ch2;
    struct feat_item *item;
    SplineChar *sc, *temp;

    if ( rpl->is_name ) {
	temp = fea_glyphname_get(tok,rpl->name_or_class);
	if ( temp!=NULL ) {
	    start = glyphs->name_or_class;
	    if ( start==NULL ) {
		LogError(_("Internal state messed up on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
return( sofar );
	    }
	    for (;;) {
		while ( *start==' ' ) ++start;
		if ( *start=='\0' )
	    break;
		for ( pt=start; *pt!='\0' && *pt!=' '; ++pt );
		ch = *pt; *pt = '\0';
		sc = fea_glyphname_get(tok,start);
		*pt = ch; start = pt;
		if ( sc!=NULL ) {
		    item = chunkalloc(sizeof(struct feat_item));
		    item->type = ft_pst;
		    item->next = sofar;
		    sofar = item;
		    item->u1.sc = sc;
		    item->u2.pst = chunkalloc(sizeof(PST));
		    item->u2.pst->type = pst_substitution;
		    item->u2.pst->u.subs.variant = copy(temp->name);
		}
	    }
	}
    } else if ( !glyphs->is_name ) {
	start = glyphs->name_or_class;
	start2 = rpl->name_or_class;
	for (;;) {
	    while ( *start==' ' ) ++start;
	    while ( *start2==' ' ) ++start2;
	    if ( *start=='\0' && *start2=='\0' )
	break;
	    else if ( *start=='\0' || *start2=='\0' ) {
		LogError(_("When a single substitution is specified by glyph classes, those classes must be of the same length on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
	break;
	    }
	    for ( pt=start; *pt!='\0' && *pt!=' '; ++pt );
	    ch = *pt; *pt = '\0';
	    for ( pt2=start2; *pt2!='\0' && *pt2!=' '; ++pt2 );
	    ch2 = *pt2; *pt2 = '\0';
	    sc = fea_glyphname_get(tok,start);
	    temp = fea_glyphname_get(tok,start2);
	    *pt = ch; start = pt;
	    *pt2 = ch2; start2 = pt2;
	    if ( sc==NULL || temp==NULL )
	continue;
	    item = chunkalloc(sizeof(struct feat_item));
	    item->type = ft_pst;
	    item->next = sofar;
	    sofar = item;
	    item->u1.sc = sc;
	    item->u2.pst = chunkalloc(sizeof(PST));
	    item->u2.pst->type = pst_substitution;
	    item->u2.pst->u.subs.variant = copy(temp->name);
	}
    } else {
	LogError(_("When a single substitution's replacement is specified by a glyph class, the thing being replaced must also be a class on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    }
return( sofar );
}

static struct feat_item *fea_process_sub_multiple(struct parseState *tok,
	struct markedglyphs *glyphs, struct markedglyphs *rpl,
	struct feat_item *sofar ) {
    int len=0;
    char *mult;
    struct markedglyphs *g;
    struct feat_item *item;
    SplineChar *sc;

    for ( g=rpl; g!=NULL; g=g->next )
	len += strlen(g->name_or_class)+1;
    mult = malloc(len+1);
    len = 0;
    for ( g=rpl; g!=NULL; g=g->next ) {
	strcpy(mult+len,g->name_or_class);
	len += strlen(g->name_or_class);
	mult[len++] = ' ';
    }
    mult[len-1] = '\0';
    sc = fea_glyphname_get(tok,glyphs->name_or_class);
    if ( sc!=NULL ) {
	item = chunkalloc(sizeof(struct feat_item));
	item->type = ft_pst;
	item->next = sofar;
	sofar = item;
	item->u1.sc = sc;
	item->u2.pst = chunkalloc(sizeof(PST));
	item->u2.pst->type = pst_multiple;
	item->u2.pst->u.mult.components = mult;
    }
return( sofar );
}

static struct feat_item *fea_process_sub_ligature(struct parseState *tok,
	struct markedglyphs *glyphs, struct markedglyphs *rpl,
	struct feat_item *sofar ) {
    SplineChar *sc;
    struct markedglyphs *g;

    /* I store ligatures backwards, in the ligature glyph not the glyphs being substituted */
    sc = fea_glyphname_get(tok,rpl->name_or_class);
    if ( sc!=NULL ) {
	int len=0;
	char *space;
	for ( g=glyphs; g!=NULL && g->mark_count==glyphs->mark_count; g=g->next )
	    len += strlen(g->name_or_class)+1;
	space = malloc(len+1);
	sofar = fea_AddAllLigPosibilities(tok,glyphs,sc,space,space,sofar);
	free(space);
    }
return( sofar );
}

static char *fea_mergeTickedMarks(struct markedglyphs *g, int only_ticked) {
    int i, len=0;
    struct gpos_mark *sames;
    char *ret, *pt;

    for ( i=0; i<g->apm_cnt; ++i ) {
	if ( !only_ticked || g->apmark[i].mark_count!=0 ) {
	    for ( sames=g->apmark[i].mark_class; sames!=NULL; sames=sames->same )
		len += strlen(sames->glyphs)+1;
	}
    }
    pt = ret = malloc(len+1);
    for ( i=0; i<g->apm_cnt; ++i ) {
	if ( !only_ticked || g->apmark[i].mark_count!=0 ) {
	    for ( sames=g->apmark[i].mark_class; sames!=NULL; sames=sames->same ) {
		strcpy(pt,sames->glyphs);
		pt += strlen(pt);
		*pt++ = ' ';
	    }
	}
    }
    if ( pt>ret )
	pt[-1] = '\0';
    else
	*pt = '\0';
return( ret );
}

static int fea_AddAGlyphSet(char **covers,char **ncovers,int i, struct markedglyphs *g) {
    int j;

    covers[i] = copy(g->name_or_class);
    if ( g->apm_cnt>0 ) {
	j = ++i;
	if ( g->hidden_marked_glyphs && ncovers!=NULL ) {
	    covers = ncovers;
	    j=0;
	}
	covers[j] = fea_mergeTickedMarks(g, g->hidden_marked_glyphs );
    }
return( i );
}

static FPST *fea_markedglyphs_to_fpst(struct parseState *tok,struct markedglyphs *glyphs,
	int is_pos,int is_ignore, int is_reverse) {
    struct markedglyphs *g;
    int bcnt=0, ncnt=0, fcnt=0, lookup_cnt=0;
    char **bcovers;
    int all_single=true;
    int mmax = 0;
    int i, j, k, lc;
    FPST *fpst;
    struct fpst_rule *r;
    struct feat_item *item, *head = NULL;

    for ( g=glyphs; g!=NULL; g=g->next ) {
	if ( g->apm_cnt!=0 ) {
	    all_single = false;
	    for ( i=0; i<g->apm_cnt; ++i ) {
		if ( g->apmark[i].mark_count!=0 ) {
		    g->hidden_marked_glyphs = true;
	    break;
		}
	    }
	}
    }

    /* Mark glyphs need to be added to the fpst but they are hidden from the */
    /*  normal glyph run */
    for ( g=glyphs; g!=NULL && g->mark_count==0; g=g->next ) {
	++bcnt;
	if ( !g->is_name ) all_single = false;
	if ( g->apm_cnt!=0 ) {
	    if ( g->hidden_marked_glyphs ) {
		ncnt=1;
		mmax=1;
		g=g->next;
    break;
	    } else
		++bcnt;
	}
    }
    for ( ; g!=NULL ; g=g->next ) {
	if ( !g->is_name ) all_single = false;
	if ( g->mark_count==0 )
	    ++fcnt;
	else {
	    /* if we found some unmarked glyphs between two runs of marked */
	    /*  they don't count as lookaheads */
	    ncnt += fcnt + 1;
	    fcnt = 0;
	    if ( g->mark_count!=0 ) ++mmax;
	    if ( g->lookupname!=NULL ) ++lookup_cnt;
	}
	if ( g->apm_cnt!=0 ) {
	    if ( g->hidden_marked_glyphs ) {
		ncnt += fcnt + 1;
		fcnt = 0;
		++mmax;
	    } else
		++fcnt;
	}
    }

    fpst = chunkalloc(sizeof(FPST));
    fpst->type = is_reverse? pst_reversesub : is_pos ? pst_chainpos : pst_chainsub;
    fpst->format = is_reverse ? pst_reversecoverage : all_single ? pst_glyphs : pst_coverage;
    fpst->rule_cnt = 1;
    fpst->rules = r = calloc(1,sizeof(struct fpst_rule));
    if ( is_ignore )
	mmax = 0;
    if ( !is_pos && lookup_cnt==0 && mmax>1 )
	mmax = 1;
    r->lookup_cnt = mmax;
    r->lookups = calloc(mmax,sizeof(struct seqlookup));
    for ( i=0; i<mmax; ++i )
	r->lookups[i].seq = i;

    if ( all_single ) {
	char *temp = NULL;
	// backtrack glyphs should be in reverse order, but they are in
	// natural order in the feature file, so we reverse them
	g = fea_glyphs_to_names(glyphs,bcnt,&temp);
	r->u.glyph.back = reverseGlyphNames (temp);
	free (temp);
	g = fea_glyphs_to_names(g,ncnt,&r->u.glyph.names);
	g = fea_glyphs_to_names(g,fcnt,&r->u.glyph.fore);
    } else {
	r->u.coverage.ncnt = ncnt;
	r->u.coverage.bcnt = bcnt;
	r->u.coverage.fcnt = fcnt;
	r->u.coverage.ncovers = malloc(ncnt*sizeof(char*));
	r->u.coverage.bcovers = malloc(bcnt*sizeof(char*));
	r->u.coverage.fcovers = malloc(fcnt*sizeof(char*));

	/* bcovers glyph classes should be in reverse order, but they */
	/* are in natural order in the feature file, so we reverse them */
	bcovers = malloc(bcnt*sizeof(char*));
	for ( i=0, g=glyphs; i<bcnt; ++i, g=g->next )
	    i = fea_AddAGlyphSet(bcovers,r->u.coverage.ncovers,i,g);
	for ( j=0, k=bcnt-1; j<bcnt; j++ ) {
		r->u.coverage.bcovers[j] = copy(bcovers[k]);
		k--;
	}

	i = i>bcnt ? 1 : 0;
	for (    ; i<ncnt ; ++i, g=g->next )
		i = fea_AddAGlyphSet(r->u.coverage.ncovers,NULL,i,g);
	for ( i=0; i<fcnt; ++i, g=g->next )
	    i = fea_AddAGlyphSet(r->u.coverage.fcovers,NULL,i,g);
    }

    item = chunkalloc(sizeof(struct feat_item));
    item->type = ft_fpst;
    item->next = tok->sofar;
    tok->sofar = item;
    item->u2.fpst = fpst;

    if ( is_pos || lookup_cnt!=0 ) {
	for ( g=glyphs; g!=NULL && g->mark_count==0; g=g->next ) {
	    if ( g->hidden_marked_glyphs )
	break;
	}
	for ( i=lc=0; g!=NULL; ++i, g=g->next ) {
	    head = NULL;
	    if ( g->mark_count==0 && !g->hidden_marked_glyphs ) {
		if ( g->lookupname || g->vr || g->anchors || g->apmark || g->ligcomp ) {
		    LogError(_("Lookup information attached to unmarked glyph on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		    ++tok->err_count;
		}
	    } else if ( g->lookupname!=NULL ) {
		head = chunkalloc(sizeof(struct feat_item));
		head->type = ft_lookup_ref;
		head->u1.lookup_name = copy(g->lookupname);
	    /* The difference between single positioning and pair positioning */
	    /*  isn't important here. The contextual sequence already contains */
	    /*  the info a kern pair would need, so no need to duplicate. Always */
	    /*  use single positioning */
	    } else if ( is_pos && g->vr!=NULL ) {
		head = fea_process_pos_single(tok,g,NULL);
	    } else if ( is_pos && g->is_cursive && g->anchors!=NULL ) {
		head = fea_process_pos_cursive(tok,g,NULL);
	    } else if ( is_pos && g->apmark!=NULL ) {
		++i;	/* There's an implied glyph class here */
		head = fea_process_pos_markbase(tok,g,NULL);
	    } else if ( is_pos && g->ligcomp!=NULL ) {
		head = fea_process_pos_ligature(tok,g,NULL);
	    } else if ( is_pos ) {
		LogError(_("Unparseable contextual sequence on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
	    }
	    if ( head!=NULL ) {
		r->lookups[lc].lookup = (OTLookup *) head;
		r->lookups[lc++].seq = i;
	    }
	}
	r->lookup_cnt = lc;
    }

return( fpst );
}

static void fea_ParseIgnore(struct parseState *tok) {
    struct markedglyphs *glyphs;
    int is_pos;
    FPST *fpst;
    /* ignore [pos|sub] <marked glyph sequence> (, <marked g sequence>)* */

    fea_ParseTok(tok);
    if ( tok->type==tk_position )
	is_pos = true;
    else if ( tok->type == tk_substitute )
	is_pos = false;
/* I don't think rsub is allowed here */
    else {
	LogError(_("The ignore keyword must be followed by either position or substitute on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	is_pos = true;
    }
    for (;;) {
	glyphs = fea_ParseMarkedGlyphs(tok,false/* don't parse value records, etc*/,
		true/*allow marks*/,false/* no lookups */);
	fpst = fea_markedglyphs_to_fpst(tok,glyphs,false,true,false);
	if ( is_pos )
	    fpst->type = pst_chainpos;
	fea_markedglyphsFree(glyphs);
	fea_ParseTok(tok);
	if ( tok->type!=tk_char || tok->tokbuf[0]!=',' )
    break;
    }

    fea_now_semi(tok);
}

static int spacecount(char *str) {
    int scnt=0;

    if ( str==NULL )
return( 0 );
    while ( *str!='\0' ) {
	if ( *str==' ' ) {
	    ++scnt;
	    while ( *str==' ' ) ++str;
	} else
	    ++str;
    }
return( scnt );
}

static void fea_ParseSubstitute(struct parseState *tok) {
    /* name by name => single subs */
    /* class by name => single subs */
    /* class by class => single subs */
    /* name by <glyph sequence> => multiple subs */
    /* name from <class> => alternate subs */
    /* <glyph sequence> by name => ligature */
    /* <marked glyph sequence> by <name> => context chaining */
    /* <marked glyph sequence> by <glyph sequence> => context chaining */
    /* <marked glyph sequence> by <lookup name>* => context chaining */
    /* [ignore sub] <marked glyph sequence> (, <marked g sequence>)* */
    /* reversesub <marked glyph sequence> by <name> => reverse context chaining */
    int is_reverse = tok->type == tk_reversesub;
    struct markedglyphs *glyphs = fea_ParseMarkedGlyphs(tok,false,true,false),
	    *g, *rpl, *rp;
    int cnt, i, has_lookups, mk_num;
    SplineChar *sc;
    struct feat_item *item, *head;

    fea_ParseTok(tok);
    for ( cnt=has_lookups=mk_num=0, g=glyphs; g!=NULL; g=g->next, ++cnt ) {
	if ( g->lookupname!=NULL )
	    has_lookups = 1;
	if ( g->mark_count!=0 )
	    ++mk_num;
    }
    if ( glyphs==NULL ) {
	LogError(_("Empty substitute on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    } else if ( is_reverse && (mk_num!=1 || has_lookups!=0)) {
	LogError(_("Reverse substitute must have exactly one marked glyph and no lookups on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    } else if ( !glyphs->has_marks ) {
	/* Non-contextual */
	if ( cnt==1 && glyphs->is_name && tok->type==tk_from ) {
	    /* Alternate subs */
	    char *alts;
	    fea_ParseTok(tok);
	    alts = fea_ParseGlyphClassGuarded(tok);
	    sc = fea_glyphname_get(tok,glyphs->name_or_class);
	    if ( sc!=NULL ) {
		item = chunkalloc(sizeof(struct feat_item));
		item->type = ft_pst;
		item->next = tok->sofar;
		tok->sofar = item;
		item->u1.sc = sc;
		item->u2.pst = chunkalloc(sizeof(PST));
		item->u2.pst->type = pst_alternate;
		item->u2.pst->u.alt.components = alts;
	    }
	} else if ( cnt>=1 && tok->type==tk_by ) {
	    rpl = fea_ParseMarkedGlyphs(tok,false,false,false);
	    if ( rpl==NULL ) {
		LogError(_("No substitution specified on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
	    } else if ( rpl->has_marks ) {
		LogError(_("No marked glyphs allowed in replacement on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
	    } else {
		if ( cnt==1 && rpl->next==NULL ) {
		    tok->sofar = fea_process_sub_single(tok,glyphs,rpl,tok->sofar);
		} else if ( cnt==1 && glyphs->is_name && rpl->next!=NULL && rpl->is_name ) {
		    /* Multiple substitution */
		    tok->sofar = fea_process_sub_multiple(tok,glyphs,rpl,tok->sofar);
		} else if ( cnt>1 && rpl->is_name && rpl->next==NULL ) {
		    tok->sofar = fea_process_sub_ligature(tok,glyphs,rpl,tok->sofar);
		    /* Ligature */
		} else {
		    LogError(_("Unparseable glyph sequence in substitution on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		    ++tok->err_count;
		}
	    }
	    fea_markedglyphsFree(rpl);
	} else {
	    LogError(_("Expected 'by' or 'from' keywords in substitution on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	}
    } else {
	/* Contextual */
	FPST *fpst = fea_markedglyphs_to_fpst(tok,glyphs,false,false,is_reverse);
	struct fpst_rule *r = fpst->rules;
	if ( tok->type!=tk_by ) {
	    /* In the old syntax this would be an error, but in the new lookups*/
	    /*  can appear within the marked glyph list */
	    if ( has_lookups==0 ) {
		LogError(_("Expected 'by' keyword in substitution on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
	    }
	    fea_UnParseTok(tok);
	} else {
	    rpl = fea_ParseMarkedGlyphs(tok,false,false,true);
	    for ( g=glyphs; g!=NULL && g->mark_count==0; g=g->next );
	    if ( is_reverse ) {
		if ( rpl==NULL || rpl->next!=NULL || rpl->lookupname!=NULL ||
			g==NULL ||
			spacecount(g->name_or_class) != spacecount(rpl->name_or_class) ) {
		    LogError(_("Expected a single glyph name in reverse substitution on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		    ++tok->err_count;
		} else {
		    r->u.rcoverage.replacements = copy(rpl->name_or_class );
		}
	    } else {
		if ( rpl==NULL ) {
		    LogError(_("No substitution specified on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		    ++tok->err_count;
		} else {
		    if ( rpl->lookupname!=NULL ) {
			head = chunkalloc(sizeof(struct feat_item));
			head->type = ft_lookup_ref;
			head->u1.lookup_name = copy(rpl->lookupname);
		    } else if ( g->next==NULL || g->next->mark_count!=g->mark_count ) {
			if (rpl->next)
			    head = fea_process_sub_multiple(tok,g,rpl,NULL);
			else
			    head = fea_process_sub_single(tok,g,rpl,NULL);
		    } else if ( g->next!=NULL && g->mark_count==g->next->mark_count ) {
			head = fea_process_sub_ligature(tok,g,rpl,NULL);
		    } else {
			head = NULL;
			LogError(_("Unparseable contextual sequence on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
			++tok->err_count;
		    }
		    r->lookups[0].lookup = (OTLookup *) head;
		    cnt = g->mark_count;
		    while ( g!=NULL && g->mark_count == cnt )	/* skip everything involved here */
			g=g->next;
		    for ( ; g!=NULL && g->mark_count!=0; g=g->next ); /* skip any uninvolved glyphs */
		}
	    }
	    fea_markedglyphsFree(rpl);
	}
    }

    fea_end_statement(tok);
    fea_markedglyphsFree(glyphs);
}

static void fea_ParsePosition(struct parseState *tok, int enumer) {
    /* name <vr> => single pos */
    /* class <vr> => single pos */
    /* name|class <vr> name|class <vr> => pair pos */
    /* name|class name|class <vr> => pair pos */
    /* cursive name|class <anchor> <anchor> => cursive positioning */
    /* base name|class {<anchor> "mark" mark-class}+ */
    /* mark name|class {<anchor> "mark" mark-class}+ */
    /* ligature name|class {<anchor> "mark" mark-class}+ {"ligComponent" {<anchor> "mark" mark-class}+ }* */

  /* These next are the obsolete syntax used in V1.6 */
    /* name|class <anchor> mark name|class => mark to base pos */
	/* Must be preceded by a mark statement */
    /* name|class <anchor> <anchor>+ mark name|class => mark to ligature pos */
	/* Must be preceded by a mark statement */
    /* mark name|class <anchor> mark name|class => mark to base pos */
	/* Must be preceded by a mark statement */
  /* End obsolete */

    /* <marked glyph pos sequence> => context chaining */
    /* [ignore pos] <marked glyph sequence> (, <marked g sequence>)* */
    struct markedglyphs *glyphs = fea_ParseMarkedGlyphs(tok,true,true,false), *g;
    int cnt;

    fea_ParseTok(tok);
    for ( cnt=0, g=glyphs; g!=NULL; g=g->next, ++cnt );
    if ( glyphs==NULL ) {
	LogError(_("Empty position on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    } else if ( !glyphs->has_marks ) {
	/* Non-contextual */
	if ( glyphs->is_cursive ) {
	    if ( cnt!=1 || glyphs->ap_cnt!=2 ) {
		LogError(_("Invalid cursive position on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
	    } else
		tok->sofar = fea_process_pos_cursive(tok,glyphs,tok->sofar);
	} else if ( cnt==1 && glyphs->vr!=NULL ) {
	    tok->sofar = fea_process_pos_single(tok,glyphs,tok->sofar);
	} else if ( cnt==2 && (glyphs->vr!=NULL || glyphs->next->vr!=NULL) ) {
	    tok->sofar = fea_process_pos_pair(tok,glyphs,tok->sofar, enumer);
	} else if ( glyphs->apm_cnt!=0 ) {
	    /* New syntax for mark to base/mark to mark lookups */
	    tok->sofar = fea_process_pos_markbase(tok,glyphs,tok->sofar);
	} else if ( glyphs->lc_cnt!=0 ) {
	    /* New syntax for mark to ligature lookups */
	    tok->sofar = fea_process_pos_ligature(tok,glyphs,tok->sofar);
	} else {
	    LogError(_("Unparseable glyph sequence in position on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	}
    } else {
	/* Contextual */
	(void) fea_markedglyphs_to_fpst(tok,glyphs,true,false,false);
    }
    fea_now_semi(tok);
    fea_markedglyphsFree(glyphs);
}

static enum otlookup_type fea_LookupTypeFromItem(struct feat_item *item) {
    switch ( item->type ) {
      case ft_pst: case ft_pstclass:
	switch ( item->u2.pst->type ) {
	  case pst_position:
return( gpos_single );
	  case pst_pair:
return( gpos_pair );
	  case pst_substitution:
return( gsub_single );
	  case pst_alternate:
return( gsub_alternate );
	  case pst_multiple:
return( gsub_multiple );
	  case pst_ligature:
return( gsub_ligature );
	  default:
return( ot_undef );		/* Can't happen */
	}
      break;
      case ft_ap:
	switch( item->u2.ap->type ) {
	  case at_centry: case at_cexit:
return( gpos_cursive );
	  case at_mark:
return( ot_undef );		/* Can be used in three different lookups. Not enough info */
	  case at_basechar:
return( gpos_mark2base );
	  case at_baselig:
return( gpos_mark2ligature );
	  case at_basemark:
return( gpos_mark2mark );
	  default:
return( ot_undef );		/* Can't happen */
	}
      break;
      case ft_fpst:
	switch( item->u2.fpst->type ) {
	  case pst_chainpos:
return( gpos_contextchain );
	  case pst_chainsub:
return( gsub_contextchain );
	  case pst_reversesub:
return( gsub_reversecchain );
	  default:
return( ot_undef );		/* Can't happen */
	}
      break;
      default:
return( ot_undef );		/* Can happen */
    }
}

static struct feat_item *fea_AddFeatItem(struct parseState *tok,enum feat_type type,uint32 tag) {
    struct feat_item *item;

    item = chunkalloc(sizeof(struct feat_item));
    item->type = type;
    item->u1.tag = tag;
    item->next = tok->sofar;
    tok->sofar = item;
return( item );
}

static int fea_LookupSwitch(struct parseState *tok) {
    int enumer = false;

    switch ( tok->type ) {
      case tk_class:
	fea_ParseGlyphClassDef(tok);
      break;
      case tk_anchorDef:
	fea_ParseAnchorDef(tok);
      break;
      case tk_valueRecordDef:
	fea_ParseValueRecordDef(tok);
      break;
      case tk_markClass:
	fea_ParseMarkClass(tok);
      break;
      case tk_lookupflag:
	fea_ParseLookupFlags(tok);
      break;
      case tk_ignore:
	fea_ParseIgnore(tok);
      break;
      case tk_enumerate:
	fea_TokenMustBe(tok,tk_position,'\0');
	enumer = true;
	/* Fall through */;
      case tk_position:
	fea_ParsePosition(tok,enumer);
      break;
      case tk_substitute: case tk_reversesub:
	fea_ParseSubstitute(tok);
	enumer = false;
      break;
      case tk_subtable:
	fea_AddFeatItem(tok,ft_subtable,0);
	fea_TokenMustBe(tok,tk_char,';');
      break;
      case tk_char:
	if ( tok->tokbuf[0]=='}' )
return( 2 );
	/* Fall through */
      default:
return( 0 );
    }
return( 1 );
}

static void fea_ParseLookupDef(struct parseState *tok, int could_be_stat ) {
    char *lookup_name;
    struct feat_item *item, *first_after_mark;
    enum otlookup_type lookuptype;
    int ret;
    int has_single, has_multiple, has_ligature, has_alternate;

    /* keywords are allowed in lookup names */
    fea_ParseTokWithKeywords(tok, false);
    if ( tok->type!=tk_name ) {
	LogError(_("Expected name in lookup on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	fea_skip_to_semi(tok);
return;
    }
    lookup_name = copy(tok->tokbuf);
    fea_ParseTok(tok);
    if ( could_be_stat && tok->type==tk_char && tok->tokbuf[0]==';' ) {
	item = chunkalloc(sizeof(struct feat_item));
	item->type = ft_lookup_ref;
	item->u1.lookup_name = lookup_name;
	item->next = tok->sofar;
	tok->sofar = item;
return;
    } else if ( tok->type==tk_useExtension )		/* I just ignore this */
	fea_ParseTok(tok);
    if ( tok->type!=tk_char || tok->tokbuf[0]!='{' ) {
	LogError(_("Expected '{' in feature definition on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	fea_skip_to_semi(tok);
return;
    }

    item = chunkalloc(sizeof(struct feat_item));
    item->type = ft_lookup_start;
    item->u1.lookup_name = lookup_name;
    item->next = tok->sofar;
    tok->sofar = item;

    first_after_mark = NULL;
    for (;;) {
	fea_ParseTok(tok);
	if ( tok->err_count>100 )
    break;
	if ( tok->type==tk_eof ) {
	    LogError(_("Unexpected end of file in lookup definition on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
return;
	} else if ( (ret = fea_LookupSwitch(tok))==0 ) {
	    LogError(_("Unexpected token, %s, in lookup definition on line %d of %s"), tok->tokbuf, tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    if ( tok->type==tk_name && strcmp(tok->tokbuf,"subs")==0 )
		LogError(_(" Perhaps you meant to use the keyword 'sub' rather than 'subs'?") );
	    ++tok->err_count;
return;
	} else if ( ret==2 )
    break;
	/* Ok, mark classes must either contain exactly the same glyphs, or */
	/*  they may not intersect at all */
	/* Mark2* lookups are not well documented (because adobe FDK doesn't */
	/*  support them) but I'm going to assume that if I have some mark */
	/*  statements, then some pos statement, then another mark statement */
	/*  that I begin a new subtable with the second set of marks (and a */
	/*  different set of mark classes) */
	if ( tok->sofar!=NULL && tok->sofar->type==ft_subtable )
	    first_after_mark = NULL;
	else if ( tok->sofar!=NULL && tok->sofar->type==ft_ap ) {
	    if ( tok->sofar->u2.ap->type == at_mark )
		first_after_mark = NULL;
	    else if ( first_after_mark == NULL )
		first_after_mark = tok->sofar;
	    else {
		struct feat_item *f;
		for ( f = tok->sofar->next; f!=NULL; f=f->next ) {
		    if ( f->type==ft_lookup_start || f->type==ft_subtable )
		break;
		    if ( f->type!=ft_ap )
		continue;
		    if ( f==first_after_mark )
		break;
		}
	    }
	}
    }
    fea_ParseTokWithKeywords(tok, false);
    if ( tok->type!=tk_name || strcmp(tok->tokbuf,lookup_name)!=0 ) {
	LogError(_("Expected %s in lookup definition on line %d of %s"),
		lookup_name, tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    }
    fea_end_statement(tok);

    /* Any of a multiple, alternate, or ligature substitution may have */
    /* a single destination. In either case it will look just like a single */
    /* substitution. So if there are both multiple/alternate/ligature and */
    /* single subs in a lookup translate all the singles into the */
    /* corresponding type */
    has_single = has_multiple = has_ligature = has_alternate = false;
    for ( item=tok->sofar ; item!=NULL && item->type!=ft_lookup_start; item=item->next ) {
	enum otlookup_type cur = fea_LookupTypeFromItem(item);
	if ( cur==gsub_multiple )
	    has_multiple = true;
	else if ( cur==gsub_alternate )
	    has_alternate = true;
	else if ( cur==gsub_ligature )
	    has_ligature = true;
	else if ( cur==gsub_single )
	    has_single = true;
    }
    if ( has_single ) {
	enum possub_type psttype = pst_pair;
	if ( has_multiple && !( has_alternate || has_ligature ) )
	    psttype = pst_multiple;
	else if ( has_alternate && !( has_multiple || has_ligature ) )
	    psttype = pst_alternate;
	else if ( has_ligature && !( has_multiple || has_alternate ) )
	    psttype = pst_ligature;
	if ( psttype!=pst_pair ) {
	    for ( item=tok->sofar ; item!=NULL && item->type!=ft_lookup_start; item=item->next ) {
		enum otlookup_type cur = fea_LookupTypeFromItem(item);
		if ( cur==gsub_single )
		    item->u2.pst->type = psttype;
	    }
	}
    }

    /* Make sure all entries in this lookup of the same lookup type */
    lookuptype = ot_undef;
    for ( item=tok->sofar ; item!=NULL && item->type!=ft_lookup_start; item=item->next ) {
	enum otlookup_type cur = fea_LookupTypeFromItem(item);
	if ( cur==ot_undef )	/* Some entries in the list (lookupflags) have no type */
	    /* Tum, ty, tum tum */;
	else if ( lookuptype==ot_undef )
	    lookuptype = cur;
	else if ( lookuptype!=cur ) {
	    LogError(_("All entries in a lookup must have the same type on line %d of %s"),
		    tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
    break;
	}
    }
    if ( lookuptype==ot_undef ) {
	LogError(_("This lookup has no effect, I can't figure out its type on line %d of %s"),
		tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    }

    item = chunkalloc(sizeof(struct feat_item));
    item->type = ft_lookup_end;
    /* item->u1.lookup_name = lookup_name; */
    item->next = tok->sofar;
    tok->sofar = item;
}

static struct nameid *fea_ParseNameId(struct parseState *tok,int strid) {
    int platform = 3, specific = 1, language = 0x409;
    struct nameid *nm;
    char *start, *pt;
    int max, ch, value;
    FILE *in = tok->inlist[tok->inc_depth];
    /* nameid <id> [<string attibute>] string; */
    /*  "nameid" and <id> will already have been parsed when we get here */
    /* <string attribute> := <platform> | <platform> <specific> <language> */
    /* <patform>==3 => <specific>=1 <language>=0x409 */
    /* <platform>==1 => <specific>=0 <lang>=0 */
    /* string in double quotes \XXXX escapes to UCS2 (for 3) */
    /* string in double quotes \XX escapes to MacLatin (for 1) */
    /* I only store 3,1 strings */

    fea_ParseTok(tok);
    if ( tok->type == tk_int ) {
	if ( tok->value!=3 && tok->value!=1 ) {
	    LogError(_("Invalid platform for string on line %d of %s"),
		    tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	} else if ( tok->value==1 ) {
	    specific = language = 0;
	}
	fea_ParseTok(tok);
	if ( tok->type == tk_int ) {
	    specific = tok->value;
	    tok->base = 0;
	    fea_TokenMustBe(tok,tk_int,'\0');
	    language = tok->value;
	    tok->base = 10;
	    fea_ParseTok(tok);
	}
    }
    if ( tok->type!=tk_char || tok->tokbuf[0]!='"' ) {
	LogError(_("Expected string on line %d of %s"),
		tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	fea_skip_to_semi(tok);
	nm = NULL;
    } else {
	if ( platform==3 && specific==1 ) {
	    nm = chunkalloc(sizeof(struct nameid));
	    nm->strid = strid;
	    nm->platform = platform;
	    nm->specific = specific;
	    nm->language = language;
	} else
	    nm = NULL;
	max = 0;
	pt = 0; start = NULL;
	while ( (ch=getc(in))!=EOF && ch!='"' ) {
	    if ( ch=='\n' || ch=='\r' )
	continue;		/* Newline characters are ignored here */
				/*  may be specified with backslashes */
	    if ( ch=='\\' ) {
		int i, dmax = platform==3 ? 4 : 2;
		value = 0;
		for ( i=0; i<dmax; ++i ) {
		    ch = getc(in);
		    if ( !ishexdigit(ch)) {
			ungetc(ch,in);
		break;
		    }
		    if ( ch>='a' && ch<='f' )
			ch -= ('a'-10);
		    else if ( ch>='A' && ch<='F' )
			ch -= ('A'-10);
		    else
			ch -= '0';
		    value <<= 4;
		    value |= ch;
		}
	    } else
		value = ch;
	    if ( nm!=NULL ) {
		if ( pt-start+3>=max ) {
		    int off = pt-start;
		    start = realloc(start,(max+=100)+1);
		    pt = start+off;
		}
		pt = utf8_idpb(pt,value,0);
	    }
	}
	if ( nm!=NULL ) {
	    if ( pt ) {
		*pt = '\0';
		nm->utf8_str = copy(start);
		free(start);
	    } else
		nm->utf8_str = copy("");
	}
	if ( tok->type!=tk_char || tok->tokbuf[0]!='"' ) {
	    LogError(_("End of file found in string on line %d of %s"),
		    tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	} else
	    fea_end_statement(tok);
    }
return( nm );
}

static struct feat_item *fea_ParseParameters(struct parseState *tok, struct feat_item *feat) {
    /* Ok. The only time a parameter keyword may be used is inside a 'size' */
    /*  feature and then it takes 4 numbers */
    /* The first, third and fourth are in decipoints and may be either */
    /*  integers or floats (in which case we must multiply them by 10) */
    int params[4];
    int i;

    memset(params,0,sizeof(params));
    for ( i=0; i<4; ++i ) {
	params[i] = fea_ParseDeciPoints(tok);
	if ( tok->type==tk_char && tok->tokbuf[0]==';' )
    break;
    }
    fea_end_statement(tok);

    if ( feat==NULL ) {
	feat = chunkalloc(sizeof(struct feat_item));
	feat->type = ft_sizeparams;
	feat->next = tok->sofar;
	tok->sofar = feat;
    }
    feat->u1.params = malloc(sizeof(params));
    memcpy(feat->u1.params,params,sizeof(params));
return( feat );
}

static struct feat_item *fea_ParseSizeMenuName(struct parseState *tok, struct feat_item *feat) {
    /* Sizemenuname takes either 0, 1 or 3 numbers and a string */
    /* if no numbers are given (or the first number is 3) then the string is */
    /*  unicode. Otherwise a mac encoding, treated as single byte */
    /* Since fontforge only supports windows strings here I shall parse and */
    /*  ignore mac strings */
    struct nameid *string;

    string = fea_ParseNameId(tok,-1);

    if ( string!=NULL ) {
	if ( feat==NULL ) {
	    feat = chunkalloc(sizeof(struct feat_item));
	    feat->type = ft_sizeparams;
	    feat->next = tok->sofar;
	    tok->sofar = feat;
	}
	string->next = feat->u2.names;
	feat->u2.names = string;
    }
return( feat );
}

static void fea_ParseFeatureNames(struct parseState *tok,uint32 tag) {
    struct otffeatname *cur;
    struct otfname *head=NULL, *string;
    struct nameid *temp;
    struct feat_item *item;
    /* name [<string attibute>] string; */

    for (;;) {
	fea_ParseTok(tok);
	if ( tok->type != tk_name && strcmp(tok->tokbuf,"name")!=0 )	/* "name" is only a keyword here */
    break;
	temp = fea_ParseNameId(tok,-1);
	if ( temp!=NULL ) {
	    if ( temp->platform==3 && temp->specific==1 ) {
		string = chunkalloc(sizeof(*string));
		string->lang = temp->language;
		string->name = temp->utf8_str;
		string->next = head;
		head = string;
		chunkfree(temp,sizeof(*temp));
	    } else
		NameIdFree(temp);
	}
    }

    if ( head!=NULL ) {
	item = chunkalloc(sizeof(struct feat_item));
	item->type = ft_featname;
	item->next = tok->sofar;
	tok->sofar = item;
	item->u2.featnames = cur = chunkalloc(sizeof(*cur));
	cur->tag = tag;
	cur->names = head;
    }
    if ( tok->type!=tk_char || tok->tokbuf[0]!='}' ) {
	LogError(_("Expected closing curly brace on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    }
}

static void fea_ParseFeatureDef(struct parseState *tok) {
    uint32 feat_tag;
    struct feat_item *item, *size_item = NULL;
    int type, ret;
    enum otlookup_type lookuptype;
    int has_single, has_multiple, has_ligature, has_alternate;

    fea_ParseTag(tok);
    if ( tok->type!=tk_name || !tok->could_be_tag ) {
	LogError(_("Expected tag in feature on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	fea_skip_to_semi(tok);
return;
    }
    feat_tag = tok->tag;
    tok->in_vkrn = feat_tag == CHR('v','k','r','n');

    item = chunkalloc(sizeof(struct feat_item));
    item->type = ft_feat_start;
    item->u1.tag = feat_tag;
    if ( tok->def_langsyses!=NULL )
	item->u2.sl = SListCopy(tok->def_langsyses);
    else {
	item->u2.sl = chunkalloc(sizeof(struct scriptlanglist));
	item->u2.sl->script = DEFAULT_SCRIPT;
	item->u2.sl->lang_cnt = 1;
	item->u2.sl->langs[0] = DEFAULT_LANG;
    }
    item->next = tok->sofar;
    tok->sofar = item;

    fea_ParseTok(tok);
    if ( tok->type!=tk_char || tok->tokbuf[0]!='{' ) {
	LogError(_("Expected '{' in feature definition on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	fea_skip_to_semi(tok);
return;
    }

    for (;;) {
	fea_ParseTok(tok);
	if ( tok->err_count>100 )
    break;
	if ( tok->type==tk_eof ) {
	    LogError(_("Unexpected end of file in feature definition on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
return;
	} else if ( (ret = fea_LookupSwitch(tok))==0 ) {
	    switch ( tok->type ) {
	      case tk_lookup:
		fea_ParseLookupDef(tok,true);
	      break;
	      case tk_languagesystem:
		fea_ParseLangSys(tok,true);
	      break;
	      case tk_feature:
		/* can appear inside an 'aalt' feature. I don't support it, but */
		/*  just parse and ignore it */
		if ( feat_tag!=CHR('a','a','l','t')) {
		    LogError(_("Features inside of other features are only permitted for 'aalt' features on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		    ++tok->err_count;
		}
		fea_ParseTok(tok);
		if ( tok->type!=tk_name || !tok->could_be_tag ) {
		    LogError(_("Expected tag on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		    ++tok->err_count;
		}
		fea_end_statement(tok);
	      break;
	      case tk_script:
	      case tk_language:
		/* If no lang specified after script use 'dflt', if no script specified before a language use 'latn' */
		type = tok->type==tk_script ? ft_script : ft_lang;
		fea_ParseTok(tok);
		if ( tok->type!=tk_name || !tok->could_be_tag ) {
		    LogError(_("Expected tag on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		    ++tok->err_count;
		} else {
		    item = fea_AddFeatItem(tok,type,tok->tag);
		    if ( type==ft_lang ) {
			for (;;) {
			    fea_ParseTok(tok);
			    if ( tok->type==tk_include_dflt )
				/* Unneeded */;
			    else if ( tok->type==tk_exclude_dflt )
				item->u2.exclude_dflt = true;
			    else if ( tok->type==tk_required )
				/* Not supported by adobe (or me) */;
			    else if ( tok->type==tk_char && tok->tokbuf[0]==';' )
			break;
			    else {
				LogError(_("Expected ';' on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
				++tok->err_count;
			break;
			    }
			}
		    } else
			fea_end_statement(tok);
		}
	      break;
	      case tk_featureNames:
	        /* I don't handle these yet, so ignore 'em */
		fea_TokenMustBe(tok,tk_char,'{');
		fea_ParseFeatureNames(tok,feat_tag);
		fea_end_statement(tok);
	      break;
	      case tk_parameters:
		if ( feat_tag==CHR('s','i','z','e') ) {
		    size_item = fea_ParseParameters(tok, size_item);
	      break;
		}
		/* Fall on through */
	      case tk_name:
		if ( feat_tag==CHR('s','i','z','e') && strcmp(tok->tokbuf,"sizemenuname")==0 ) {
		    size_item = fea_ParseSizeMenuName(tok, size_item);
	      break;
		}
		/* Fall on through */
	      case tk_char:
		/* Ignore blank statement. */
		if (tok->tokbuf[0]==';')
		  break;
	      default:
		LogError(_("Unexpected token, %s, in feature definition on line %d of %s"), tok->tokbuf, tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
return;
	    }
	} else if ( ret==2 )
    break;
    }

    fea_ParseTag(tok);
    if ( tok->type!=tk_name || !tok->could_be_tag || tok->tag!=feat_tag ) {
	LogError(_("Expected '%c%c%c%c' in lookup definition on line %d of %s"),
		feat_tag>>24, feat_tag>>16, feat_tag>>8, feat_tag,
		tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    }
    fea_end_statement(tok);

    /* Any of a multiple, alternate, or ligature substitution may have */
    /* a single destination. In either case it will look just like a single */
    /* substitution. So if there are both multiple/alternate/ligature and */
    /* single subs in a lookup translate all the singles into the */
    /* corresponding type */
    has_single = has_multiple = has_ligature = has_alternate = false;
    for ( item=tok->sofar ; item!=NULL && item->type!=ft_feat_start; item=item->next ) {
	enum otlookup_type cur = fea_LookupTypeFromItem(item);
	if ( cur==gsub_multiple )
	    has_multiple = true;
	else if ( cur==gsub_alternate )
	    has_alternate = true;
	else if ( cur==gsub_ligature )
	    has_ligature = true;
	else if ( cur==gsub_single )
	    has_single = true;
    }
    if ( has_single ) {
	enum possub_type psttype = pst_pair;
	if ( has_multiple && !( has_alternate || has_ligature ) )
	    psttype = pst_multiple;
	else if ( has_alternate && !( has_multiple || has_ligature ) )
	    psttype = pst_alternate;
	else if ( has_ligature && !( has_multiple || has_alternate ) )
	    psttype = pst_ligature;
	if ( psttype!=pst_pair ) {
	    for ( item=tok->sofar ; item!=NULL && item->type!=ft_feat_start; item=item->next ) {
		enum otlookup_type cur = fea_LookupTypeFromItem(item);
		if ( cur==gsub_single )
		    item->u2.pst->type = psttype;
	    }
	}
    }

    /* Make sure all entries in this lookup of the same lookup type */
    lookuptype = ot_undef;
    for ( item=tok->sofar ; item!=NULL && item->type!=ft_feat_start; item=item->next ) {
	enum otlookup_type cur = fea_LookupTypeFromItem(item);
	if ( cur==ot_undef )	/* Some entries in the list (lookupflags) have no type */
	    /* Tum, ty, tum tum */;
	else if ( lookuptype==ot_undef )
	    lookuptype = cur;
	else if ( lookuptype!=cur ) {
	    LogError(_("All entries in a lookup must have the same type on line %d of %s"),
		    tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	    break;
	}
    }

    item = chunkalloc(sizeof(struct feat_item));
    item->type = ft_feat_end;
    item->u1.tag = feat_tag;
    item->next = tok->sofar;
    tok->sofar = item;

    tok->in_vkrn = false;
}

static void fea_ParseNameTable(struct parseState *tok) {
    struct nameid *head=NULL, *string;
    struct feat_item *item;
    /* nameid <id> [<string attibute>] string; */

    for (;;) {
	fea_ParseTok(tok);
	if ( tok->type != tk_nameid )
    break;
	fea_TokenMustBe(tok,tk_int,'\0');
	string = fea_ParseNameId(tok,tok->value);
	if ( string!=NULL ) {
	    string->next = head;
	    head = string;
	}
    }

    if ( head!=NULL ) {
	item = chunkalloc(sizeof(struct feat_item));
	item->type = ft_names;
	item->next = tok->sofar;
	tok->sofar = item;
	item->u2.names = head;
    }
    if ( tok->type!=tk_char || tok->tokbuf[0]!='}' ) {
	LogError(_("Expected closing curly brace on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    }
}

static void fea_ParseTableKeywords(struct parseState *tok, struct tablekeywords *keys) {
    int index;
    struct tablevalues *tv, *head = NULL;
    int i;
    struct feat_item *item;

    for (;;) {
	fea_ParseTok(tok);
	if ( tok->type != tk_name )
    break;
	for ( index=0; keys[index].name!=NULL; ++index ) {
	    if ( strcmp(keys[index].name,tok->tokbuf)==0 )
	break;
	}
	if ( keys[index].name==NULL ) {
	    LogError(_("Unknown field %s on line %d of %s"), tok->tokbuf, tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	    index = -1;
	}
	if ( index!=-1 && keys[index].offset!=-1 ) {
	    tv = chunkalloc(sizeof(struct tablevalues));
	    tv->index = index;
	} else
	    tv = NULL;
	if ( strcmp(tok->tokbuf,"Vendor")==0 && tv!=NULL) {
	    /* This takes a 4 character string */
	    /* of course strings aren't part of the syntax, but it takes one anyway */
	    fea_ParseTok(tok);
	    if ( tok->type==tk_name && tok->could_be_tag )
		/* Accept a normal tag, since that's what it really is */
		tv->value = tok->tag;
	    else if ( tok->type==tk_char && tok->tokbuf[0]=='"' ) {
		uint8 foo[4]; int ch;
		FILE *in = tok->inlist[tok->inc_depth];
		memset(foo,' ',sizeof(foo));
		for ( i=0; i<4; ++i ) {
		    ch = getc(in);
		    if ( ch==EOF )
		break;
		    else if ( ch=='"' ) {
			ungetc(ch,in);
		break;
		    }
		    foo[i] = ch;
		}
		while ( (ch=getc(in))!=EOF && ch!='"' );
		tok->value=(foo[0]<<24) | (foo[1]<<16) | (foo[2]<<8) | foo[3];
	    } else {
		LogError(_("Expected string on line %d of %s"),
			tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
		chunkfree(tv,sizeof(*tv));
		tv = NULL;
	    }
	    fea_ParseTok(tok);
	} else {
	    fea_ParseTok(tok);
	    if ( tok->type!=tk_int ) {
		LogError(_("Expected integer on line %d of %s"),
			tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
		chunkfree(tv,sizeof(*tv));
		tv = NULL;
		fea_ParseTok(tok);
	    } else {
		if ( tv!=NULL )
		    tv->value = tok->value;
		if ( strcmp(keys[index].name,"FontRevision")==0 ) {
		    /* Can take a float */
		    FILE *in = tok->inlist[tok->inc_depth];
		    int ch = getc(in);
		    if ( ch=='.' )
			for ( ch=getc(in); isdigit(ch); ch=getc(in));
		    ungetc(ch,in);
		}
		if ( index!=-1 && keys[index].cnt!=1 ) {
		    int is_panose = strcmp(keys[index].name,"Panose")==0 && tv!=NULL;
		    if ( is_panose )
			tv->panose_vals[0] = tv->value;
		    for ( i=1; ; ++i ) {
			fea_ParseTok(tok);
			if ( tok->type!=tk_int )
		    break;
			if ( is_panose && i<10 && tv!=NULL )
			    tv->panose_vals[i] = tok->value;
		    }
		} else
		    fea_ParseTok(tok);
	    }
	}
	if ( tok->type!=tk_char || tok->tokbuf[0]!=';' ) {
	    LogError(_("Expected semicolon on line %d of %s"),
		    tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	    fea_skip_to_close_curly(tok);
	    chunkfree(tv,sizeof(*tv));
    break;
	}
	if ( tv!=NULL ) {
	    tv->next = head;
	    head = tv;
	}
    }
    if ( tok->type!=tk_char || tok->tokbuf[0]!='}' ) {
	LogError(_("Expected '}' on line %d of %s"),
		tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	fea_skip_to_close_curly(tok);
    }
    if ( head!=NULL ) {
	item = chunkalloc(sizeof(struct feat_item));
	item->type = ft_tablekeys;
	item->u1.offsets = keys;
	item->u2.tvals = head;
	item->next = tok->sofar;
	tok->sofar = item;
    }
}

static void fea_ParseGDEFTable(struct parseState *tok) {
    /* GlyphClassDef <base> <lig> <mark> <component>; */
    /* Attach <glyph>|<glyph class> <number>+; */	/* parse & ignore */
    /* LigatureCaretByPos <glyph>|<glyph class> <number>+; */
    /* LigatureCaretByIndex <glyph>|<glyph class> <number>+; */	/* parse & ignore */
    int i;
    struct feat_item *item;
    int16 *carets=NULL; int len=0, max=0;

    for (;;) {
	fea_ParseTok(tok);
	if ( tok->type!=tk_name )
    break;
	if ( strcmp(tok->tokbuf,"Attach")==0 ) {
	    fea_ParseTok(tok);
	    /* Bug. Not parsing inline classes */
	    if ( tok->type!=tk_class && tok->type!=tk_name ) {
		LogError(_("Expected name or class on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
		fea_skip_to_semi(tok);
	    } else {
		for (;;) {
		    fea_ParseTok(tok);
		    if ( tok->type!=tk_int )
		break;
		}
	    }
	} else if ( strcmp(tok->tokbuf,"LigatureCaret")==0 || /* FF backwards compatibility */ \
		    strcmp(tok->tokbuf,"LigatureCaretByPos")==0 ) {
	    // Ligature carets by single coordinate (format 1).
	    carets=NULL;
	    len=0;
	    item = chunkalloc(sizeof(struct feat_item));
	    item->type = ft_lcaret;
	    item->next = tok->sofar;
	    tok->sofar = item;

	    fea_ParseTok(tok);
	    if ( tok->type==tk_name )
		item->u1.class = fea_glyphname_validate(tok,tok->tokbuf);
	    else if ( tok->type==tk_cid )
		item->u1.class = fea_cid_validate(tok,tok->value);
	    else if ( tok->type == tk_class || (tok->type==tk_char && tok->tokbuf[0]=='['))
		item->u1.class = fea_ParseGlyphClassGuarded(tok);
	    else {
		LogError(_("Expected name or class on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
		fea_skip_to_semi(tok);
    continue;
	    }
	    for (;;) {
		fea_ParseTok(tok);
		if ( tok->type==tk_int )
		    /* Not strictly cricket, but I'll accept it */;
		else if ( tok->type==tk_char && tok->tokbuf[0]=='<' )
		    fea_ParseCaret(tok);
		else
	    break;
		carets = realloc(carets,(max+=10)*sizeof(int16));
		carets[len++] = tok->value;
	    }
	    if ( tok->type!=tk_char || tok->tokbuf[0]!=';' ) {
		LogError(_("Expected semicolon on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
		fea_skip_to_semi(tok);
	    }
	    if (carets != NULL) {
		item->u2.lcaret = malloc((len+1)*sizeof(int16));
		memcpy(item->u2.lcaret,carets,len*sizeof(int16));
		item->u2.lcaret[len] = 0;
	    } else {
		LogError(_("Expected integer or list of integers after %s on line %d of %s"), item->u1.class,
			tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    }
	} else if (strcmp (tok->tokbuf, "LigatureCaretByIndex") == 0) {
	    // Ligature carets by contour point index (format 2).
	    // Unsupported, will be parsed and ignored.
	    fea_ParseTok (tok);
	    if (tok->type != tk_class && tok->type != tk_name && tok->type != tk_cid) {
	        LogError (_("Expected name or class on line %d of %s"),
	            tok->line[tok->inc_depth],
	            tok->filename[tok->inc_depth]);
	        ++tok->err_count;
	        fea_skip_to_semi (tok);
	        continue;
	    } else {
	        while (true) {
	            fea_ParseTok (tok);
	            if (tok->type != tk_int)
	                break;
	        }
	    }
	} else if ( strcmp(tok->tokbuf,"GlyphClassDef")==0 ) {
	    item = chunkalloc(sizeof(struct feat_item));
	    item->type = ft_gdefclasses;
	    item->u1.gdef_classes = chunkalloc(sizeof(char *[4]));
	    item->next = tok->sofar;
	    tok->sofar = item;
	    for ( i=0; i<4; ++i ) {
		fea_ParseTok(tok);
		if ( tok->type==tk_char && (( i<3 && tok->tokbuf[0]==',' ) || ( i==3 && tok->tokbuf[0]==';' )) )
		    ; /* Skip the placeholder for a missing class definition or a final semicolon */
		else if ( tok->type==tk_class || ( tok->type==tk_char && tok->tokbuf[0]=='[' ) ) {
		    item->u1.gdef_classes[i] = fea_ParseGlyphClassGuarded(tok);
		    fea_ParseTok(tok);
		    if ( tok->type==tk_char && (( i<3 && tok->tokbuf[0]==',' ) || ( i==3 && tok->tokbuf[0]==';' )) )
			; /* skip the delimiting comma or a final semicolon */
		    else
			LogError(_("Expected comma or semicolon on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		} else
		    LogError(_("Expected class on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    }
	} else {
	    LogError(_("Expected Attach or LigatureCaret or GlyphClassDef on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
    break;
	}
    }
    if ( tok->type!=tk_char || tok->tokbuf[0]!='}' ) {
	LogError(_("Unexpected token in GDEF on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	fea_skip_to_close_curly(tok);
    }
    free(carets);
}

static void fea_ParseBaseTable(struct parseState *tok) {
/*   HorizAxis.BaseTagList              ideo romn;	*/
/*   HorizAxis.BaseScriptList latn romn -120    0,	*/
/*			      cyrl romn -120    0,	*/
/*			      grek romn -120    0,	*/
/*			      hani ideo -120    0,	*/
/*			      kana idea -120    0;	*/
/*   HorizAxis.MinMax unparseable-garbage;		*/
/* (Also VertAxis). Sadly the spec for the "MinMax" is wrong and I can't guess*/
/*  what might be right */
    struct Base h, v, *active;
    int cnt=0, i, off;
    uint32 baselines[50];
    int16 poses[50];
    struct basescript *last=NULL, *cur;

    memset(&h,0,sizeof(h));
    memset(&v,0,sizeof(v));
    fea_ParseTok(tok);
    while ( (tok->type!=tk_char || tok->tokbuf[0]!='}') && tok->type!=tk_eof ) {
	active = NULL; off = 0;
	if ( tok->type==tk_name ) {
	    if ( strncmp(tok->tokbuf,"HorizAxis.",10)==0 ) {
		active=&h;
		off = 10;
	    } else if ( strncmp(tok->tokbuf,"VertAxis.",9)==0 ) {
		active=&v;
		off = 9;
	    }
	}
	if ( active==NULL ) {
	    LogError(_("Expected either \"HorizAxis\" or \"VertAxis\" in BASE table on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	    fea_skip_to_semi(tok);
	    if ( tok->type==tk_char && tok->tokbuf[0]=='}' )
		fea_UnParseTok(tok);
	    fea_ParseTok(tok);
    continue;
	}
	if ( strcmp(tok->tokbuf+off,"BaseTagList")==0 ) {
	    cnt = 0;
	    while ( (fea_ParseTag(tok), tok->could_be_tag) ) {
		if ( cnt<=sizeof(baselines)/sizeof(baselines[0]) )
		    baselines[cnt++] = tok->tag;
	    }
	    active->baseline_cnt = cnt;
	    active->baseline_tags = malloc(cnt*sizeof(uint32));
	    memcpy(active->baseline_tags,baselines,cnt*sizeof(uint32));
	} else if ( strcmp(tok->tokbuf+off,"BaseScriptList")==0 ) {
	    last = NULL;
	    while ( (fea_ParseTag(tok), tok->could_be_tag) ) {
		uint32 script_tag = tok->tag;
		uint32 base_tag;
		int err=0;
		fea_ParseTag(tok);
		if ( !tok->could_be_tag ) {
		    err=1;
		    LogError(_("Expected baseline tag in BASE table on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		    ++tok->err_count;
		} else
		    base_tag = tok->tag;
		for ( i=0; i<cnt && !err; ++i ) {
		    fea_ParseTok(tok);
		    if ( tok->type!=tk_int ) {
			err=1;
			LogError(_("Expected an integer specifying baseline positions in BASE table on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
			++tok->err_count;
		    } else
			poses[i] = tok->value;
		}
		if ( !err ) {
		    cur = chunkalloc(sizeof(struct basescript));
		    if ( last!=NULL )
			last->next = cur;
		    else
			active->scripts = cur;
		    last = cur;
		    cur->script = script_tag;
		    cur->baseline_pos = calloc(cnt,sizeof(int16));
		    memcpy(cur->baseline_pos,poses,i*sizeof(int16));
		    for ( i=0; i<active->baseline_cnt; ++i ) {
			if ( base_tag == active->baseline_tags[i] ) {
			    cur->def_baseline = i;
		    break;
			}
		    }
		}
		fea_ParseTag(tok);
		if ( tok->type==tk_char && tok->tokbuf[0]==';' )
	    break;
		else if ( tok->type!=tk_char || tok->tokbuf[0]!=',' ) {
		    err=1;
		    LogError(_("Expected comma or semicolon in BASE table on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		    ++tok->err_count;
		}
	    }
	} else {
	    if ( strcmp(tok->tokbuf+off,"MinMax")!=0 ) {
		LogError(_("Unexpected token, %s, in BASE table on line %d of %s"), tok->tokbuf, tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
	    }	/* Min Max isn't parseable, but it is "expected" */
	    fea_skip_to_semi(tok);
	}
	if ( tok->type!=tk_char || tok->tokbuf[0]!=';' ) {
	    LogError(_("Expected semicolon in BASE table on line %d of %s"), tok->tokbuf, tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	    if ( tok->type==tk_char && tok->tokbuf[0]=='}' )
		fea_UnParseTok(tok);
	}
	fea_ParseTok(tok);
    }
    if ( tok->err_count==0 ) {
	if ( h.baseline_cnt!=0 ) {
	    BaseFree(tok->sf->horiz_base);
	    tok->sf->horiz_base = chunkalloc(sizeof(struct Base));
	    *(tok->sf->horiz_base) = h;
	}
	if ( v.baseline_cnt!=0 ) {
	    BaseFree(tok->sf->vert_base);
	    tok->sf->vert_base = chunkalloc(sizeof(struct Base));
	    *(tok->sf->vert_base) = v;
	}
    }
}

static void fea_ParseTableDef(struct parseState *tok) {
    uint32 table_tag;
    struct feat_item *item;

    fea_ParseTag(tok);
    if ( tok->type!=tk_name || !tok->could_be_tag ) {
	LogError(_("Expected tag in table on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	fea_skip_to_semi(tok);
return;
    }
    table_tag = tok->tag;

    item = chunkalloc(sizeof(struct feat_item));
    item->type = ft_table;
    item->u1.tag = table_tag;
    item->next = tok->sofar;
    tok->sofar = item;
    fea_TokenMustBe(tok,tk_char,'{');
    switch ( table_tag ) {
      case CHR('G','D','E','F'):
	fea_ParseGDEFTable(tok);
      break;
      case CHR('n','a','m','e'):
	fea_ParseNameTable(tok);
      break;

      case CHR('h','h','e','a'):
	fea_ParseTableKeywords(tok,hhead_keys);
      break;
      case CHR('v','h','e','a'):
	fea_ParseTableKeywords(tok,vhead_keys);
      break;
      case CHR('O','S','/','2'):
	fea_ParseTableKeywords(tok,os2_keys);
      break;
      case CHR('B','A','S','E'):
	fea_ParseBaseTable(tok);
      break;

      case CHR('h','e','a','d'):
	/* FontRevision <number>.<number>; */
	/* Only one field here, and I don't really support it */
      case CHR('v','m','t','x'):
	/* I don't support 'vmtx' tables */
      default:
	fea_skip_to_close_curly(tok);
      break;
    }

    fea_ParseTag(tok);
    if ( tok->type!=tk_name || !tok->could_be_tag || tok->tag!=table_tag ) {
	LogError(_("Expected matching tag in table on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	fea_skip_to_semi(tok);
return;
    }
    fea_end_statement(tok);
}

/* ************************************************************************** */
/* ******************************* Free feat ******************************** */
/* ************************************************************************** */

static void NameIdFree(struct nameid *nm) {
    struct nameid *nmnext;

    while ( nm!=NULL ) {
	nmnext = nm->next;
	free( nm->utf8_str );
	chunkfree(nm,sizeof(*nm));
	nm = nmnext;
    }
}

static void TableValsFree(struct tablevalues *tb) {
    struct tablevalues *tbnext;

    while ( tb!=NULL ) {
	tbnext = tb->next;
	chunkfree(tb,sizeof(*tb));
	tb = tbnext;
    }
}

static void fea_featitemFree(struct feat_item *item) {
    struct feat_item *next;
    int i,j;

    while ( item!=NULL ) {
	next = item->next;
	switch ( item->type ) {
	  case ft_lookup_end:
	  case ft_feat_end:
	  case ft_table:
	  case ft_subtable:
	  case ft_script:
	  case ft_lang:
	  case ft_lookupflags:
	    /* Nothing needs freeing */;
	  break;
	  case ft_feat_start:
	  case ft_langsys:
	    ScriptLangListFree( item->u2.sl);
	  break;
	  case ft_lookup_start:
	  case ft_lookup_ref:
	    free( item->u1.lookup_name );
	  break;
	  case ft_sizeparams:
	    free( item->u1.params );
	    NameIdFree( item->u2.names );
	  break;
	  case ft_names:
	    NameIdFree( item->u2.names );
	  break;
	  case ft_featname:
	    OtfFeatNameListFree( item->u2.featnames );
	  break;
	  case ft_gdefclasses:
	    for ( i=0; i<4; ++i )
		free(item->u1.gdef_classes[i]);
	    chunkfree(item->u1.gdef_classes,sizeof(char *[4]));
	  break;
	  case ft_lcaret:
	    free( item->u2.lcaret );
	  break;
	  case ft_tablekeys:
	    TableValsFree( item->u2.tvals );
	  break;
	  case ft_pst:
	    PSTFree( item->u2.pst );
	  break;
	  case ft_pstclass:
	    free( item->u1.class );
	    PSTFree( item->u2.pst );
	  break;
	  case ft_ap:
	    AnchorPointsFree( item->u2.ap );
	  break;
	  case ft_fpst:
	    if ( item->u2.fpst!=NULL ) {
		for ( i=0; i<item->u2.fpst->rule_cnt; ++i ) {
		    struct fpst_rule *r = &item->u2.fpst->rules[i];
		    for ( j=0; j<r->lookup_cnt; ++j ) {
			if ( r->lookups[j].lookup!=NULL ) {
			    struct feat_item *nested = (struct feat_item *) (r->lookups[j].lookup);
			    fea_featitemFree(nested);
			    r->lookups[j].lookup = NULL;
			}
		    }
		}
		FPSTFree(item->u2.fpst);
	    }
	  break;
	  default:
	    IError("Don't know how to free a feat_item of type %d", item->type );
	  break;
	}
	chunkfree(item,sizeof(*item));
	item = next;
    }
}

static void fea_ParseFeatureFile(struct parseState *tok) {

    for (;;) {
	fea_ParseTok(tok);
	if ( tok->err_count>100 )
    break;
	switch ( tok->type ) {
	  case tk_class:
	    fea_ParseGlyphClassDef(tok);
	  break;
	  case tk_anchorDef:
	    fea_ParseAnchorDef(tok);
	  break;
	  case tk_valueRecordDef:
	    fea_ParseValueRecordDef(tok);
	  break;
	  case tk_markClass:
	    fea_ParseMarkClass(tok);
	  break;
	  case tk_lookup:
	    fea_ParseLookupDef(tok,false);
	  break;
	  case tk_languagesystem:
	    fea_ParseLangSys(tok,false);
	  break;
	  case tk_feature:
	    fea_ParseFeatureDef(tok);
	  break;
	  case tk_table:
	    fea_ParseTableDef(tok);
	  break;
	  case tk_anonymous:
	    LogError(_("FontForge does not support anonymous tables on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    fea_skip_to_close_curly(tok);
	  break;
	  case tk_eof:
  goto end_loop;
	  case tk_char:
	    /* Ignore blank statement. */
	    if (tok->tokbuf[0]==';')
	      break;
	  default:
	    LogError(_("Unexpected token, %s, on line %d of %s"), tok->tokbuf, tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
  goto end_loop;
      }
    }
  end_loop:;
}

/* ************************************************************************** */
/* ******************************* Apply feat ******************************* */
/* ************************************************************************** */
static int fea_FeatItemEndsLookup(enum feat_type type) {
return( type==ft_lookup_end || type == ft_feat_end ||
	    type == ft_table || type == ft_script ||
	    type == ft_lang || type == ft_langsys ||
	    type == ft_lookup_ref );
}

static struct feat_item *fea_SetLookupLink(struct feat_item *nested,
	enum otlookup_type type) {
    struct feat_item *prev = NULL;
    enum otlookup_type found_type;

    while ( nested!=NULL ) {
	/* Stop when we find something which forces a new lookup */
	if ( fea_FeatItemEndsLookup(nested->type) )
    break;
	if ( nested->ticked ) {
	    nested = nested->next;
    continue;
	}
	found_type = fea_LookupTypeFromItem(nested);
	if ( type==ot_undef || found_type == ot_undef || found_type == type ) {
	    if ( nested->type!=ft_ap || nested->u2.ap->type!=at_mark )
		nested->ticked = true;		/* Marks might get used in more than one lookup */
	    if ( prev!=NULL )
		prev->lookup_next = nested;
	    prev = nested;
	}
	nested = nested->next;
    }
return( nested );
}

static void fea_ApplyLookupListPST(struct parseState *tok,
	struct feat_item *lookup_data,OTLookup *otl) {
    struct lookup_subtable *sub = NULL, *last=NULL;
    struct feat_item *l;
    /* Simple pst's are easy. We just attach them to their proper glyphs */
    /*  and then clear the feat_item pst slot (so we don't free them later) */
    /* There might be a subtable break */
    /* There might be a lookupflags */

    for ( l = lookup_data; l!=NULL; l=l->lookup_next ) {
	switch ( l->type ) {
	  case ft_lookup_start:
	  case ft_lookupflags:
	    /* Ignore these, already handled them */;
	  break;
	  case ft_subtable:
	    sub = NULL;
	  break;
	  case ft_pst:
	    if ( sub==NULL ) {
		sub = chunkalloc(sizeof(struct lookup_subtable));
		sub->lookup = otl;
		sub->per_glyph_pst_or_kern = true;
		if ( last==NULL )
		    otl->subtables = sub;
		else
		    last->next = sub;
		last = sub;
	    }
	    l->u2.pst->subtable = sub;
	    l->u2.pst->next = l->u1.sc->possub;
	    l->u1.sc->possub = l->u2.pst;
	    l->u2.pst = NULL;			/* So we don't free it later */
	  break;
	  default:
	    IError("Unexpected feature type %d in a PST feature", l->type );
	  break;
	}
    }
}

static OTLookup *fea_ApplyLookupList(struct parseState *tok,
	struct feat_item *lookup_data,int lookup_flag);

static void fea_ApplyLookupListContextual(struct parseState *tok,
	struct feat_item *lookup_data,OTLookup *otl) {
    struct lookup_subtable *sub = NULL, *last=NULL;
    struct feat_item *l;
    int i,j;
    /* Fpst's are almost as easy as psts. We don't worry about subtables */
    /*  (every fpst gets a new subtable, so the statement is irrelevant) */
    /* the only complication is that we must recursively handle a lookup list */
    /* There might be a lookupflags */

    for ( l = lookup_data; l!=NULL; l=l->lookup_next ) {
	switch ( l->type ) {
	  case ft_lookup_start:
	  case ft_lookupflags:
	  case ft_subtable:
	    /* Ignore these, already handled them */;
	  break;
	  case ft_fpst:
	    sub = chunkalloc(sizeof(struct lookup_subtable));
	    sub->lookup = otl;
	    if ( last==NULL )
		otl->subtables = sub;
	    else
		last->next = sub;
	    last = sub;
	    sub->fpst = l->u2.fpst;
	    l->u2.fpst->next = tok->sf->possub;
	    tok->sf->possub = l->u2.fpst;
	    l->u2.fpst = NULL;
	    sub->fpst->subtable = sub;
	    for ( i=0; i<sub->fpst->rule_cnt; ++i ) {
		struct fpst_rule *r = &sub->fpst->rules[i];
		for ( j=0; j<r->lookup_cnt; ++j ) {
		    if ( r->lookups[j].lookup!=NULL ) {
			struct feat_item *nested = (struct feat_item *) (r->lookups[j].lookup);
			fea_SetLookupLink(nested,ot_undef);
			r->lookups[j].lookup = fea_ApplyLookupList(tok,nested,otl->lookup_flags);	/* Not really sure what the lookup flag should be here. */
			fea_featitemFree(nested);
		    }
		}
	    }
	  break;
	  default:
	    IError("Unexpected feature type %d in a FPST feature", l->type );
	  break;
	}
    }
}

static void fea_ApplyLookupListCursive(struct parseState *tok,
	struct feat_item *lookup_data,OTLookup *otl) {
    struct lookup_subtable *sub = NULL, *last=NULL;
    struct feat_item *l;
    AnchorPoint *aplast, *ap;
    AnchorClass *ac = NULL;
    /* Cursive's are also easy. There might be two ap's in the list so slight */
    /*  care needed when adding them to a glyph, and we must create an anchorclass */
    /* There might be a subtable break */
    /* There might be a lookupflags */

    for ( l = lookup_data; l!=NULL; l=l->lookup_next ) {
	switch ( l->type ) {
	  case ft_lookup_start:
	  case ft_lookupflags:
	    /* Ignore these, already handled them */;
	  break;
	  case ft_subtable:
	    sub = NULL;
	  break;
	  case ft_ap:
	    if ( sub==NULL ) {
		sub = chunkalloc(sizeof(struct lookup_subtable));
		sub->lookup = otl;
		sub->anchor_classes = true;
		if ( last==NULL )
		    otl->subtables = sub;
		else
		    last->next = sub;
		last = sub;
		ac = chunkalloc(sizeof(AnchorClass));
		ac->subtable = sub;
		ac->type = act_curs;
		ac->next = tok->accreated;
		tok->accreated = ac;
	    }
	    aplast = NULL;
	    for ( ap=l->u2.ap; ap!=NULL; ap=ap->next ) {
		aplast = ap;
		ap->anchor = ac;
	    }
	    aplast->next = l->u1.sc->anchor;
	    l->u1.sc->anchor = l->u2.ap;
	    l->u2.ap = NULL;			/* So we don't free them later */
	  break;
	  default:
	    IError("Unexpected feature type %d in a cursive feature", l->type );
	  break;
	}
    }
}

static void fea_ApplyLookupListMark2(struct parseState *tok,
	struct feat_item *lookup_data,OTLookup *otl) {
    /* The same markClasses can be shared among several lookups */
    struct gpos_mark **classes;
    AnchorClass **acs;
    int ac_cnt, i;
    struct lookup_subtable *sub = NULL, *last=NULL;
    AnchorPoint *ap, *aplast;
    int ac_max=10;
    struct gpos_mark *sames;
    char *start, *pt;
    int ch;
    SplineChar *sc;

    classes = malloc(ac_max*sizeof(struct gpos_mark *));
    acs = malloc(ac_max*sizeof(AnchorClass *));
    ac_cnt = 0;
    while ( lookup_data != NULL && lookup_data->type!=ft_lookup_end ) {
	struct feat_item *orig = lookup_data;
	sub = NULL;
	/* Skip any subtable marks */
	while ( lookup_data!=NULL &&
		(lookup_data->type==ft_subtable ||
		 lookup_data->type==ft_lookup_start ||
		 lookup_data->type==ft_lookupflags ) )
	    lookup_data = lookup_data->lookup_next;

	/* Now process the base glyphs and figure out the mark classes */
	while ( lookup_data!=NULL &&
		((lookup_data->type==ft_ap && lookup_data->mclass!=NULL) ||
		 lookup_data->type==ft_lookup_start ||
		 lookup_data->type==ft_lookupflags ) ) {
	    if ( fea_LookupTypeFromItem(lookup_data)!=otl->lookup_type )
		/* Skip it */;
	    else if ( lookup_data->type == ft_ap ) {
		for ( i=0; i<ac_cnt; ++i ) {
		    if ( classes[i]==lookup_data->mclass )
		break;
		}
		if ( i==ac_cnt ) {
		    ++ac_cnt;
		    if ( ac_cnt>=ac_max ) {
			classes = realloc(classes,(ac_max+=10)*sizeof(struct gpos_mark *));
			acs = realloc(acs,ac_max*sizeof(AnchorClass *));
		    }
		    classes[i] = lookup_data->mclass;
		    acs[i] = chunkalloc(sizeof(AnchorClass));
		    if ( sub==NULL ) {
			sub = chunkalloc(sizeof(struct lookup_subtable));
			sub->lookup = otl;
			sub->anchor_classes = true;
			if ( last==NULL )
			    otl->subtables = sub;
			else
			    last->next = sub;
			last = sub;
		    }
		    acs[i]->subtable = sub;
		    acs[i]->type = otl->lookup_type==gpos_mark2mark ? act_mkmk :
				    otl->lookup_type==gpos_mark2base ? act_mark :
					    act_mklg;
		    /* Skip the initial '@' in the named mark class name */
		    if ( classes[i]->name_used==0 )
			acs[i]->name = copy(classes[i]->name+1);
		    else {
			acs[i]->name = malloc(strlen(classes[i]->name)+10);
			sprintf(acs[i]->name,"%s_%d", classes[i]->name+1, classes[i]->name_used);
		    }
		    ++(classes[i]->name_used);
		    acs[i]->next = tok->accreated;
		    tok->accreated = acs[i];
		}
		aplast = NULL;
		for ( ap=lookup_data->u2.ap; ap!=NULL; ap=ap->next ) {
		    aplast = ap;
		    ap->anchor = acs[i];
		}
		aplast->next = lookup_data->u1.sc->anchor;
		lookup_data->u1.sc->anchor = lookup_data->u2.ap;
		lookup_data->u2.ap = NULL;	/* So we don't free them later */
	    }
	    lookup_data = lookup_data->next;
	}
	if ( lookup_data==orig )		/* Infinite loop preventer */
    break;
    }

    /* Now go back and assign the mark classes to the correct anchor classes */
    for ( i=0; i<ac_cnt; ++i ) {
	for ( sames=classes[i]; sames!=NULL; sames=sames->same ) {
	    start = sames->glyphs;
	    for (;;) {
		while ( *start==' ' ) ++start;
		if ( *start=='\0' )
	    break;
		for ( pt=start; *pt!='\0' && *pt!=' '; ++pt );
		ch = *pt; *pt = '\0';
		sc = fea_glyphname_get(tok,start);
		*pt = ch; start = pt;
		if ( sc==NULL )
	    continue;
/* what if an existing AP exists with the same name, in the glyph? */
		ap = AnchorPointsCopy(sames->ap);
		ap->type = at_mark;
		ap->anchor = acs[i];
		ap->next = sc->anchor;
		sc->anchor = ap;
	    }
	}
    }

    free(classes);
    free(acs);
}


static int is_blank(const char *s) {
    int i;

    i = 0;
    while (s[i] != '\0' && s[i] == ' ')
        i++;
    return( s[i] == '\0');
}

struct class_set {
    char **classes;
    int cnt, max;
};

/* We've got a set of glyph classes -- but they are the classes that make sense */
/*  to the user and so there's no guarantee that there aren't two classes with */
/*  the same glyph(s) */
/* Simplify the list so that: There are no duplicates classes and each name */
/*  appears in at most one class. This is what we need */
static void fea_canonicalClassSet(struct class_set *set) {
    int i,j,k;

    /* Remove any duplicate classes */
    qsort(set->classes,set->cnt,sizeof(char *), strcmpD);
    for ( i=0; i<set->cnt; ++i ) {
	for ( j=i+1; j<set->cnt; ++j )
	    if ( strcmp(set->classes[i],set->classes[j])!=0 )
	break;
	if ( j>i+1 ) {
	    int off = j-(i+1);
	    for ( k=i+1; k<j; ++k )
		free(set->classes[k]);
	    for ( k=j ; k<set->cnt; ++k )
		set->classes[k-off] = set->classes[k];
	    set->cnt -= off;
	}
    }

    for ( i=0; i < set->cnt - 1; ++i ) {
        for ( j=i+1; j < set->cnt; ++j ) {
            if ( fea_classesIntersect(set->classes[i],set->classes[j]) ) {
                if ( set->cnt>=set->max )
                    set->classes = realloc(set->classes,(set->max+=20)*sizeof(char *));
                set->classes[set->cnt++] = fea_classesSplit(set->classes[i],set->classes[j]);
            }
        }
    }

    /* Remove empty classes */
    i = 0;
    while (i < set->cnt) {
        if (is_blank(set->classes[i])) {
            free(set->classes[i]);
            for ( k=i+1 ; k < set->cnt; ++k )
                set->classes[k-1] = set->classes[k];
            set->cnt -= 1;
        } else {
            i++;
        }
    }
}

static void KCFillDevTab(KernClass *kc,int index,DeviceTable *dt) {
    if ( dt==NULL || dt->corrections == NULL )
return;
    if ( kc->adjusts == NULL )
	kc->adjusts = calloc(kc->first_cnt*kc->second_cnt,sizeof(DeviceTable));
    kc->adjusts[index] = *dt;
    kc->adjusts[index].corrections = malloc(dt->last_pixel_size-dt->first_pixel_size+1);
    memcpy(kc->adjusts[index].corrections,dt->corrections,dt->last_pixel_size-dt->first_pixel_size+1);

}

static void KPFillDevTab(KernPair *kp,DeviceTable *dt) {
    if ( dt==NULL || dt->corrections == NULL )
return;
    kp->adjust = chunkalloc(sizeof(DeviceTable));
    *kp->adjust = *dt;
    kp->adjust->corrections = malloc(dt->last_pixel_size-dt->first_pixel_size+1);
    memcpy(kp->adjust->corrections,dt->corrections,dt->last_pixel_size-dt->first_pixel_size+1);
}

static void fea_fillKernClass(KernClass *kc,struct feat_item *l) {
    int i,j;
    PST *pst;

    while ( l!=NULL && l->type!=ft_subtable ) {
	if ( l->type==ft_pstclass ) {
	    pst = l->u2.pst;
	    for ( i=1; i<kc->first_cnt; ++i ) {
		if ( fea_classesIntersect(kc->firsts[i],l->u1.class) ) {
		    for ( j=1; j<kc->second_cnt; ++j ) {
			if ( fea_classesIntersect(kc->seconds[j],pst->u.pair.paired) ) {
			    /* FontForge only supports kerning classes in one direction at a time, not full value records */
			    if ( pst->u.pair.vr[0].h_adv_off != 0 ) {
				kc->offsets[i*kc->second_cnt+j] = pst->u.pair.vr[0].h_adv_off;
				if ( pst->u.pair.vr[0].adjust!=NULL )
				    KCFillDevTab(kc,i*kc->second_cnt+j,&pst->u.pair.vr[0].adjust->xadv);
			    } else if ( pst->u.pair.vr[0].v_adv_off != 0 ) {
				kc->offsets[i*kc->second_cnt+j] = pst->u.pair.vr[0].v_adv_off;
				if ( pst->u.pair.vr[0].adjust!=NULL )
				    KCFillDevTab(kc,i*kc->second_cnt+j,&pst->u.pair.vr[0].adjust->yadv);
			    } else if ( pst->u.pair.vr[1].h_adv_off != 0 ) {
				kc->offsets[i*kc->second_cnt+j] = pst->u.pair.vr[1].h_adv_off;
				if ( pst->u.pair.vr[1].adjust!=NULL )
				    KCFillDevTab(kc,i*kc->second_cnt+j,&pst->u.pair.vr[1].adjust->xadv);
			    }
			    if ( strcmp(kc->seconds[j],pst->u.pair.paired)==0 )
		    break;
			}
		    }
		    if ( strcmp(kc->firsts[i],l->u1.class)==0 )
	    break;
		}
	    }
	}
	l = l->lookup_next;
    }
}

static void SFKernClassRemoveFree(SplineFont *sf,KernClass *kc) {
    KernClass *prev;

    if ( sf->kerns==kc )
	sf->kerns = kc->next;
    else if ( sf->vkerns==kc )
	sf->vkerns = kc->next;
    else {
	prev = NULL;
	if ( sf->kerns!=NULL )
	    for ( prev=sf->kerns; prev!=NULL && prev->next!=kc; prev=prev->next );
	if ( prev==NULL && sf->vkerns!=NULL )
	    for ( prev=sf->vkerns; prev!=NULL && prev->next!=kc; prev=prev->next );
	if ( prev!=NULL )
	    prev->next = kc->next;
    }
    kc->next = NULL;
    KernClassListFree(kc);
}

static void fea_ApplyLookupListPair(struct parseState *tok,
	struct feat_item *lookup_data,int kmax,OTLookup *otl) {
    /* kcnt is the number of left/right glyph-name-lists we must sort into classes */
    struct feat_item *l, *first;
    struct class_set lefts, rights;
    struct lookup_subtable *sub = NULL, *lastsub=NULL;
    SplineChar *sc, *other;
    PST *pst;
    KernPair *kp;
    KernClass *kc;
    int vkern, kcnt, i;

    memset(&lefts,0,sizeof(lefts));
    memset(&rights,0,sizeof(rights));
    if ( kmax!=0 ) {
	lefts.classes = malloc(kmax*sizeof(char *));
	rights.classes = malloc(kmax*sizeof(char *));
	lefts.max = rights.max = kmax;
    }
    vkern = false;
    for ( l = lookup_data; l!=NULL; ) {
	first = l;
	kcnt = 0;
	while ( l!=NULL && l->type!=ft_subtable ) {
	    if ( l->type == ft_pst ) {
		if ( sub==NULL ) {
		    sub = chunkalloc(sizeof(struct lookup_subtable));
		    sub->lookup = otl;
		    sub->per_glyph_pst_or_kern = true;
		    if ( lastsub==NULL )
			otl->subtables = sub;
		    else
			lastsub->next = sub;
		    lastsub = sub;
		}
		pst = l->u2.pst;
		sc = l->u1.sc;
		l->u2.pst = NULL;
		kp = NULL;
		other = SFGetChar(sc->parent,-1,pst->u.pair.paired);
		if ( pst->u.pair.vr[0].xoff==0 && pst->u.pair.vr[0].yoff==0 &&
			pst->u.pair.vr[1].xoff==0 && pst->u.pair.vr[1].yoff==0 &&
			pst->u.pair.vr[1].v_adv_off==0 &&
			other!=NULL ) {
		    if ( (otl->lookup_flags&pst_r2l) &&
			    (pst->u.pair.vr[0].h_adv_off==0 && pst->u.pair.vr[0].v_adv_off==0 )) {
			kp = chunkalloc(sizeof(KernPair));
			kp->off = pst->u.pair.vr[1].h_adv_off;
			if ( pst->u.pair.vr[1].adjust!=NULL )
			    KPFillDevTab(kp,&pst->u.pair.vr[1].adjust->xadv);
		    } else if ( !(otl->lookup_flags&pst_r2l) &&
			    (pst->u.pair.vr[1].h_adv_off==0 && pst->u.pair.vr[0].v_adv_off==0 )) {
			kp = chunkalloc(sizeof(KernPair));
			kp->off = pst->u.pair.vr[0].h_adv_off;
			if ( pst->u.pair.vr[0].adjust!=NULL )
			    KPFillDevTab(kp,&pst->u.pair.vr[0].adjust->xadv);
		    } else if ( (pst->u.pair.vr[0].h_adv_off==0 && pst->u.pair.vr[1].h_adv_off==0 )) {
			vkern = sub->vertical_kerning = true;
			kp = chunkalloc(sizeof(KernPair));
			kp->off = pst->u.pair.vr[0].v_adv_off;
			if ( pst->u.pair.vr[0].adjust!=NULL )
			    KPFillDevTab(kp,&pst->u.pair.vr[0].adjust->yadv);
		    }
		}
		if ( kp!=NULL ) {
		    // We want to add to the ends of the lists.
		    KernPair *lastkp = NULL;
		    KernPair *tmpkp = NULL;
		    for ( tmpkp=(vkern?sc->vkerns:sc->kerns); tmpkp!=NULL && (tmpkp->sc != other || tmpkp->subtable != sub); lastkp = tmpkp, tmpkp=tmpkp->next );
		    if (tmpkp == NULL) {
		      // Populate the kerning pair.
		      kp->sc = other;
		      kp->subtable = sub;
		      // Add to the list.
		      if ( vkern ) {
			if (lastkp) lastkp->next = kp;
			else sc->vkerns = kp;
			lastkp = kp;
		      } else {
			if (lastkp) lastkp->next = kp;
			else sc->kerns = kp;
			lastkp = kp;
		      }
		      PSTFree(pst);
		    } else {
		      LogError(_("Discarding a duplicate kerning pair."));
		      SplineCharFree(sc); sc = NULL;
		      free(kp); kp = NULL;
		    }
		} else {
		    // We want to add to the end of the list.
		    PST *lastpst = NULL;
		    PST *tmppst = NULL;
		    for ( tmppst=sc->possub; tmppst!=NULL; lastpst = tmppst, tmppst=tmppst->next );
		    // Populate.
		    pst->subtable = sub;
		    // Add to the list.
		    if (lastpst) lastpst->next = pst;
		    else sc->possub = pst;
		    lastpst = pst;
		}
	    } else if ( l->type == ft_pstclass ) {
		lefts.classes[kcnt] = copy(fea_canonicalClassOrder(l->u1.class));
		rights.classes[kcnt++] = copy(fea_canonicalClassOrder(l->u2.pst->u.pair.paired));
	    }
	    l = l->lookup_next;
	}
	if ( kcnt!=0 ) {
	    lefts.cnt = rights.cnt = kcnt;
	    fea_canonicalClassSet(&lefts);
	    fea_canonicalClassSet(&rights);

	    sub = chunkalloc(sizeof(struct lookup_subtable));
	    sub->lookup = otl;
	    if ( lastsub==NULL )
		otl->subtables = sub;
	    else
		lastsub->next = sub;
	    lastsub = sub;

	    if ( sub->kc!=NULL )
		SFKernClassRemoveFree(tok->sf,sub->kc);
	    sub->kc = kc = chunkalloc(sizeof(KernClass));
	    kc->first_cnt = lefts.cnt+1; kc->second_cnt = rights.cnt+1;
	    kc->firsts = malloc(kc->first_cnt*sizeof(char *));
	    kc->seconds = malloc(kc->second_cnt*sizeof(char *));
	    kc->firsts[0] = kc->seconds[0] = NULL;
	    for ( i=0; i<lefts.cnt; ++i )
		kc->firsts[i+1] = lefts.classes[i];
	    for ( i=0; i<rights.cnt; ++i )
		kc->seconds[i+1] = rights.classes[i];
	    kc->subtable = sub;
	    kc->offsets = calloc(kc->first_cnt*kc->second_cnt,sizeof(int16));
	    kc->adjusts = calloc(kc->first_cnt*kc->second_cnt,sizeof(DeviceTable));
	    fea_fillKernClass(kc,first);
	    KernClass *lastkc = NULL;
	    KernClass *tmpkc = NULL;
	    for ( tmpkc=(sub->vertical_kerning?tok->sf->vkerns:tok->sf->kerns); tmpkc!=NULL; lastkc = tmpkc, tmpkc=tmpkc->next );
	    if ( sub->vertical_kerning ) {
		if (lastkc) lastkc->next = kc;
		else tok->sf->vkerns = kc;
		lastkc = kc;
	    } else {
		if (lastkc) lastkc->next = kc;
		else tok->sf->kerns = kc;
		lastkc = kc;
	    }
	}
	sub = NULL;
	while ( l!=NULL && l->type==ft_subtable )
	    l = l->lookup_next;
    }
    if ( kmax!=0 ) {
	free(lefts.classes);
	free(rights.classes);
    }
}

static OTLookup *fea_ApplyLookupList(struct parseState *tok,
	struct feat_item *lookup_data,int lookup_flag) {
    /* A lookup list might consist just of a lookup_ref so find the lookup named u1.lookup_name */
    /* A lookup_start is optional and provides the lookup name */
    /* A lookupflags is optional and may occur anywhere u2.lookupflags */
    /* An ap is for mark2 types for the mark u1.sc and u2.ap (not grouped in anchor classes yet) */
    /* A fpst is for contextuals u2.fpst (rule.lookups[i].lookup are lookup lists in their own rights that need to become lookups) */
    /* A subtable means a subtable break, make up a new name, ignore multiple subtable entries */
    /* A pst is for simple things u1.sc, u2.pst */
    /* A pstclass is for kerning classes u1.class, u2.pst (paired may be a class list too) */
    /* An ap is for cursive types for the u1.sc and u2.ap (an entry and an exit ap) */
    /* An ap is for mark2 types for the base u1.sc and u2.ap and mark_class */
    OTLookup *otl;
    int kcnt, mcnt;
    struct feat_item *l;
    enum otlookup_type temp;

    if ( lookup_data->type == ft_lookup_ref ) {
	for ( otl=tok->created; otl!=NULL; otl=otl->next )
	    if ( otl->lookup_name!=NULL &&
		    strcmp(otl->lookup_name,lookup_data->u1.lookup_name)==0)
return( otl );
	otl = SFFindLookup(tok->sf,lookup_data->u1.lookup_name);
	if ( otl==NULL )
	    LogError( _("No lookup named %s"),lookup_data->u1.lookup_name );
	    /* Can't give a line number, this is second pass */
return( otl );
    }

    otl = chunkalloc(sizeof(OTLookup));
    otl->lookup_flags = lookup_flag;
    otl->lookup_type = ot_undef;
    if ( tok->last==NULL )
	tok->created = otl;
    else
	tok->last->next = otl;
    tok->last = otl;

    /* Search first for class counts */
    kcnt = mcnt = 0;
    for ( l = lookup_data; l!=NULL; l=l->lookup_next ) {
	if ( l->type == ft_pstclass )
	    ++kcnt;
	else if ( l->type == ft_lookupflags )
	    otl->lookup_flags = l->u2.lookupflags;
	else if ( l->type == ft_lookup_start ) {
	    otl->lookup_name = l->u1.lookup_name;
	    l->u1.lookup_name = NULL;			/* So we don't free it later */
	}
	temp = fea_LookupTypeFromItem(l);
	if ( temp==ot_undef )
	    /* Tum ty tum tum. No information */;
	else if ( otl->lookup_type == ot_undef )
	    otl->lookup_type = temp;
	else if ( otl->lookup_type != temp )
	    IError(_("Mismatch lookup types inside a parsed lookup"));
    }
    if ( otl->lookup_type == ot_undef )
	IError(_("Could not figure out a lookup type"));
    if ( otl->lookup_type==gpos_mark2base ||
	    otl->lookup_type==gpos_mark2ligature ||
	    otl->lookup_type==gpos_mark2mark )
	fea_ApplyLookupListMark2(tok,lookup_data,otl);
    else if ( mcnt!=0 )
	IError(_("Mark anchors provided when nothing can use them"));
    else if ( otl->lookup_type==gpos_cursive )
	fea_ApplyLookupListCursive(tok,lookup_data,otl);
    else if ( otl->lookup_type==gpos_pair )
	fea_ApplyLookupListPair(tok,lookup_data,kcnt,otl);
    else if ( otl->lookup_type==gpos_contextchain ||
	    otl->lookup_type==gsub_contextchain ||
	    otl->lookup_type==gsub_reversecchain )
	fea_ApplyLookupListContextual(tok,lookup_data,otl);
    else
	fea_ApplyLookupListPST(tok,lookup_data,otl);
return( otl );
}

static struct otfname *fea_NameID2OTFName(struct nameid *names) {
    struct otfname *head=NULL, *cur;

    while ( names!=NULL ) {
	cur = chunkalloc(sizeof(struct otfname));
	cur->lang = names->language;
	cur->name = names->utf8_str;
	names->utf8_str = NULL;
	cur->next = head;
	head = cur;
	names = names->next;
    }
return( head );
}

static void fea_AttachFeatureToLookup(OTLookup *otl,uint32 feat_tag,
	struct scriptlanglist *sl) {
    FeatureScriptLangList *fl;

    if ( otl==NULL )
return;

    for ( fl = otl->features; fl!=NULL && fl->featuretag!=feat_tag; fl=fl->next );
    if ( fl==NULL ) {
	fl = chunkalloc(sizeof(FeatureScriptLangList));
	fl->next = otl->features;
	otl->features = fl;
	fl->featuretag = feat_tag;
	fl->scripts = SListCopy(sl);
    } else
	SLMerge(fl,sl);
}

static void fea_NameID2NameTable(SplineFont *sf, struct nameid *names) {
    struct ttflangname *cur;

    while ( names!=NULL ) {
	for ( cur = sf->names; cur!=NULL && cur->lang!=names->language; cur=cur->next );
	if ( cur==NULL ) {
	    cur = chunkalloc(sizeof(struct ttflangname));
	    cur->lang = names->language;
	    cur->next = sf->names;
	    sf->names = cur;
	}
	free(cur->names[names->strid]);
	cur->names[names->strid] = names->utf8_str;
	names->utf8_str = NULL;
	names = names->next;
    }
}

static void fea_TableByKeywords(SplineFont *sf, struct feat_item *f) {
    struct tablevalues *tv;
    struct tablekeywords *offsets = f->u1.offsets, *cur;
    int i;

    if ( !sf->pfminfo.pfmset ) {
	SFDefaultOS2Info(&sf->pfminfo,sf,sf->fontname);
	sf->pfminfo.pfmset = sf->pfminfo.subsuper_set = sf->pfminfo.panose_set =
	    sf->pfminfo.hheadset = sf->pfminfo.vheadset = true;
    }
    for ( tv = f->u2.tvals; tv!=NULL; tv=tv->next ) {
	cur = &offsets[tv->index];
	if ( cur->offset==-1 )
	    /* We don't support this guy, whatever he may be, but we did parse it */;
	else if ( cur->cnt==1 ) {
	    if ( cur->size==4 )
		*((uint32 *) (((uint8 *) sf) + cur->offset)) = tv->value;
	    else if ( cur->size==2 )
		*((uint16 *) (((uint8 *) sf) + cur->offset)) = tv->value;
	    else
		*((uint8 *) (((uint8 *) sf) + cur->offset)) = tv->value;
	    if ( strcmp(cur->name,"Ascender")==0 )
		sf->pfminfo.hheadascent_add = false;
	    else if ( strcmp(cur->name,"Descender")==0 )
		sf->pfminfo.hheaddescent_add = false;
	    else if ( strcmp(cur->name,"winAscent")==0 )
		sf->pfminfo.winascent_add = false;
	    else if ( strcmp(cur->name,"winDescent")==0 )
		sf->pfminfo.windescent_add = false;
	    else if ( strcmp(cur->name,"TypoAscender")==0 )
		sf->pfminfo.typoascent_add = false;
	    else if ( strcmp(cur->name,"TypoDescender")==0 )
		sf->pfminfo.typodescent_add = false;
	} else if ( cur->cnt==10 && cur->size==1 ) {
	    for ( i=0; i<10; ++i )
		(((uint8 *) sf) + cur->offset)[i] = tv->panose_vals[i];
	}
    }
}

static void fea_GDefGlyphClasses(SplineFont *sf, struct feat_item *f) {
    int i, ch;
    char *pt, *start;
    SplineChar *sc;

    for ( i=0; i<4; ++i ) if ( f->u1.gdef_classes[i]!=NULL ) {
	for ( pt=f->u1.gdef_classes[i]; ; ) {
	    while ( *pt==' ' ) ++pt;
	    if ( *pt=='\0' )
	break;
	    for ( start = pt; *pt!=' ' && *pt!='\0'; ++pt );
	    ch = *pt; *pt = '\0';
	    sc = SFGetChar(sf,-1,start);
	    *pt = ch;
	    if ( sc!=NULL )
		sc->glyph_class = i+2;
	}
    }
}

static void fea_GDefLigCarets(SplineFont *sf, struct feat_item *f) {
    int i, ch;
    char *pt, *start;
    SplineChar *sc;
    PST *pst, *prev, *next;

    for ( pt=f->u1.class; ; ) {
	while ( *pt==' ' ) ++pt;
	if ( *pt=='\0' )
    break;
	for ( start = pt; *pt!=' ' && *pt!='\0'; ++pt );
	ch = *pt; *pt = '\0';
	sc = SFGetChar(sf,-1,start);
	*pt = ch;
	if ( sc!=NULL ) {
	    for ( prev=NULL, pst=sc->possub; pst!=NULL; pst=next ) {
		next = pst->next;
		if ( pst->type!=pst_lcaret )
		    prev = pst;
		else {
		    if ( prev==NULL )
			sc->possub = next;
		    else
			prev->next = next;
		    pst->next = NULL;
		    PSTFree(pst);
		}
	    }
	    for ( i=0; f->u2.lcaret[i]!=0; ++i );
	    pst = chunkalloc(sizeof(PST));
	    pst->next = sc->possub;
	    sc->possub = pst;
	    pst->type = pst_lcaret;
	    pst->u.lcaret.cnt = i;
	    pst->u.lcaret.carets = f->u2.lcaret;
	}
    }
    // Should be done in fea_featitemFree()
    f->u2.lcaret = NULL;
}

static struct feat_item *fea_ApplyFeatureList(struct parseState *tok,
	struct feat_item *feat_data) {
    int lookup_flags = 0;
    uint32 feature_tag = feat_data->u1.tag;
    struct scriptlanglist *sl = feat_data->u2.sl;
    struct feat_item *f, *start;
    OTLookup *otl;
    int saw_script = false;
    enum otlookup_type ltype;
    struct otffeatname *fn;

    feat_data->u2.sl = NULL;

    for ( f=feat_data->next; f!=NULL && f->type!=ft_feat_end ; ) {
	if ( f->ticked ) {
	    f = f->next;
    continue;
	}
	switch ( f->type ) {
	  case ft_lookupflags:
	    lookup_flags = f->u2.lookupflags;
	    f = f->next;
    continue;
	  case ft_lookup_ref:
	    otl = fea_ApplyLookupList(tok,f,lookup_flags);
	    fea_AttachFeatureToLookup(otl,feature_tag,sl);
	    f = f->next;
    continue;
	  case ft_lookup_start:
	    start = f;
	    start->lookup_next = f->next;
	    f = fea_SetLookupLink(start->next,ot_undef);
	    if ( f!=NULL && f->type == ft_lookup_end )
		f = f->next;
	    otl = fea_ApplyLookupList(tok,start,lookup_flags);
	    fea_AttachFeatureToLookup(otl,feature_tag,sl);
    continue;
	  case ft_script:
	    ScriptLangListFree(sl);
	    sl = chunkalloc(sizeof(struct scriptlanglist));
	    sl->script = f->u1.tag;
	    sl->lang_cnt = 1;
	    sl->langs[0] = DEFAULT_LANG;
	    saw_script = true;
	    f = f->next;
    continue;
	  case ft_lang:
	    if ( !saw_script ) {
		ScriptLangListFree(sl);
		sl = chunkalloc(sizeof(struct scriptlanglist));
		sl->script = CHR('l','a','t','n');
	    }
	    sl->langs[0] = f->u1.tag;
	    sl->lang_cnt = 1;
	    if ( !f->u2.exclude_dflt ) {
		if ( sl->langs[0]!=DEFAULT_LANG ) {
		    sl->langs[1] = DEFAULT_LANG;
		    sl->lang_cnt = 2;
		}
	    }
	    f = f->next;
    continue;
	  case ft_langsys:
	    ScriptLangListFree(sl);
	    saw_script = false;
	    sl = f->u2.sl;
	    f->u2.sl = NULL;
	    f = f->next;
    continue;
	  case ft_sizeparams:
	    if ( f->u1.params!=NULL ) {
		tok->sf->design_size = f->u1.params[0];
		tok->sf->fontstyle_id = f->u1.params[1];
		tok->sf->design_range_bottom = f->u1.params[2];
		tok->sf->design_range_top = f->u1.params[3];
	    }
	    OtfNameListFree(tok->sf->fontstyle_name);
	    tok->sf->fontstyle_name = fea_NameID2OTFName(f->u2.names);
	    f = f->next;
    continue;
	  case ft_subtable:
	    f = f->next;
    continue;
	  case ft_pst:
	  case ft_pstclass:
	  case ft_ap:
	  case ft_fpst:
	    if ( f->type==ft_ap && f->u2.ap->type==at_mark ) {
		struct feat_item *n, *a;
		/* skip over the marks */
		for ( n=f; n!=NULL && n->type == ft_ap && n->u2.ap->type==at_mark; n=n->next );
		/* find the next thing which can use those marks (might not be anything) */
		for ( a=n; a!=NULL; a=a->next ) {
		    if ( a->ticked )
		continue;
		    if ( fea_FeatItemEndsLookup(a->type) ||
			    a->type==ft_subtable || a->type==ft_ap )
		break;
		}
		if ( a==NULL || fea_FeatItemEndsLookup(a->type) || a->type==ft_subtable ||
			(a->type==ft_ap && a->u2.ap->type == at_mark )) {
		    /* There's nothing else that can use these marks so we are */
		    /*  done with them. Skip over all of them */
		    f = n;
    continue;
		}
		ltype = fea_LookupTypeFromItem(a);
	    } else
		ltype = fea_LookupTypeFromItem(f);
	    start = f;
	    f = fea_SetLookupLink(start,ltype);
	    for ( f=start; f!=NULL && f->ticked; f=f->next );
	    otl = fea_ApplyLookupList(tok,start,lookup_flags);
	    fea_AttachFeatureToLookup(otl,feature_tag,sl);
    continue;
	  case ft_featname:
	    for ( fn = tok->sf->feat_names; fn!=NULL && fn->tag!=f->u2.featnames->tag; fn=fn->next );
	    if ( fn!=NULL ) {
		OtfNameListFree(fn->names);
		fn->names = f->u2.featnames->names;
		chunkfree(f->u2.featnames,sizeof(struct otffeatname));
		f->u2.featnames = NULL;
	    } else {
		f->u2.featnames->next = tok->sf->feat_names;
		tok->sf->feat_names = f->u2.featnames;
		f->u2.featnames = NULL;
	    }
	    f = f->next;
    continue;
	  default:
	    IError("Unexpected feature item in feature definition %d", f->type );
	    f = f->next;
	}
    }
    if ( f!=NULL && f->type == ft_feat_end )
	f = f->next;
return( f );
}

static void fea_ApplyFile(struct parseState *tok, struct feat_item *item) {
    struct feat_item *f, *start;

    for ( f=item; f!=NULL ; ) {
	switch ( f->type ) {
	  case ft_lookup_start:
	    start = f;
	    start->lookup_next = f->next;
	    f = fea_SetLookupLink(start->next,ot_undef);
	    if ( f!=NULL && f->type == ft_lookup_end )
		f = f->next;
	    fea_ApplyLookupList(tok,start,0);
    continue;
	  case ft_feat_start:
	    f = fea_ApplyFeatureList(tok,f);
    continue;
	  case ft_table:
	    /* I store things all mushed together, so this tag is useless to me*/
	    /*  ignore it. The stuff inside the table matters though... */
	    f = f->next;
    continue;
	  case ft_names:
	    fea_NameID2NameTable(tok->sf,f->u2.names);
	    f = f->next;
    continue;
	  case ft_tablekeys:
	    fea_TableByKeywords(tok->sf,f);
	    f = f->next;
    continue;
	  case ft_gdefclasses:
	    fea_GDefGlyphClasses(tok->sf,f);
	    f = f->next;
    continue;
	  case ft_lcaret:
	    fea_GDefLigCarets(tok->sf,f);
	    f = f->next;
    continue;
	  default:
	    IError("Unexpected feature item in feature file %d", f->type );
	    f = f->next;
	}
    }
}

static struct feat_item *fea_reverseList(struct feat_item *f) {
    struct feat_item *n = NULL, *p = NULL;

    p = NULL;
    while ( f!=NULL ) {
	n = f->next;
	f->next = p;
	p = f;
	f = n;
    }
return( p );
}

static void fea_NameLookups(struct parseState *tok) {
    SplineFont *sf = tok->sf;
    OTLookup *gpos_last=NULL, *gsub_last=NULL, *otl, *otlnext;
    int gp_cnt=0, gs_cnt=0, acnt;
    AnchorClass *ac, *acnext, *an;

    for ( otl = sf->gpos_lookups; otl!=NULL; otl=otl->next ) {
	otl->lookup_index = gp_cnt++;
	gpos_last = otl;
    }
    for ( otl = sf->gsub_lookups; otl!=NULL; otl=otl->next ) {
	otl->lookup_index = gs_cnt++;
	gsub_last = otl;
    }

    for ( otl = tok->created; otl!=NULL; otl=otlnext ) {
	otlnext = otl->next;
	otl->next = NULL;
	if ( otl->lookup_name!=NULL && SFFindLookup(sf,otl->lookup_name)!=NULL ) {
	    int cnt=0;
	    char *namebuf = malloc(strlen( otl->lookup_name )+8 );
	    /* Name already in use, modify it */
	    do {
		sprintf(namebuf,"%s-%d", otl->lookup_name, cnt++ );
	    } while ( SFFindLookup(sf,namebuf)!=NULL );
	    free(otl->lookup_name);
	    otl->lookup_name = namebuf;
	}
	if ( otl->lookup_type < gpos_start ) {
	    if ( gsub_last==NULL )
		sf->gsub_lookups = otl;
	    else
		gsub_last->next = otl;
	    gsub_last = otl;
	    otl->lookup_index = gs_cnt++;
	} else {
	    if ( gpos_last==NULL )
		sf->gpos_lookups = otl;
	    else
		gpos_last->next = otl;
	    gpos_last = otl;
	    otl->lookup_index = gp_cnt++;
	}
	NameOTLookup(otl,sf);		/* But only if it has no name */
    }

    /* Now name and attach any unnamed anchor classes (order here doesn't matter) */
    acnt = 0;
    for ( ac=tok->accreated; ac!=NULL; ac=acnext ) {
	acnext = ac->next;
	if ( ac->name==NULL ) {
	    char buf[50];
	    do {
		snprintf(buf,sizeof(buf),_("Anchor-%d"), acnt++ );
		for ( an=sf->anchor; an!=NULL && strcmp(an->name,buf)!=0; an=an->next );
	    } while ( an!=NULL );
	    ac->name = copy(buf);
	}
        for ( an=sf->anchor; an!=NULL && strcmp(an->name,ac->name)!=0; an=an->next );
        if ( an!=NULL ) {
            /* merge acs, only one ap mark and one ap base per ac per glyph */
            int gid;
            SplineChar *sc;
            AnchorPoint *newapb, *newapbprev, *newapm, *newapmprev, *oldapb, *oldapm, *ap, *lastap;
            for ( gid=0; gid<sf->glyphcnt; ++gid ) {
                newapb = NULL;
                newapm = NULL;
                oldapb = NULL;
                oldapm = NULL;
                lastap = NULL;
                sc = sf->glyphs[gid];
                if (!sc)
                    continue;
                for ( ap=sc->anchor; ap!=NULL; ap=ap->next ) {
                    if ( ap->anchor==ac ) {
                        if ( ap->type==at_mark || ap->type==at_centry ) {
                            newapm = ap;
                            newapmprev = lastap;
                        } else {
                            newapb = ap;
                            newapbprev = lastap;
                        }
                    } else if ( ap->anchor==an ) {
                        if (ap->type==at_mark || ap->type==at_centry )
                            oldapm = ap;
                        else
                            oldapb = ap;
                    }
                    lastap = ap;
                }
                if ( oldapb!=NULL && newapb!=NULL ) {
                    oldapb->type = newapb->type;
                    if ( newapbprev!=NULL )
                        newapbprev->next = newapb->next;
                    else
                        sc->anchor = newapb->next;
                } else if ( newapb!=NULL )
                    newapb->anchor = an;

                if ( oldapm!=NULL && newapm!=NULL ) {
                    oldapm->type = newapm->type;
                    if ( newapmprev!=NULL )
                        newapmprev->next = newapm->next;
                    else
                        sc->anchor = newapm->next;
                } else if ( newapm!=NULL )
                    newapm->anchor = an;
            }
            an->subtable = ac->subtable;
            an->type = ac->type;
            an->has_base = ac->has_base;
        } else {
	    ac->next = sf->anchor;
	    sf->anchor = ac;
        }
    }

    /* attach any new gdef mark classes */
    if ( tok->gm_pos[0]>sf->mark_class_cnt ) {
	int i;
	sf->mark_classes = realloc(sf->mark_classes,tok->gm_pos[0]*sizeof(char *));
	sf->mark_class_names = realloc(sf->mark_class_names,tok->gm_pos[0]*sizeof(char *));
	for ( i=0; i<tok->gm_cnt[0]; ++i ) if ( tok->gdef_mark[0][i].index>=sf->mark_class_cnt ) {
	    int index = tok->gdef_mark[0][i].index;
	    sf->mark_class_names[index] = copy(tok->gdef_mark[0][i].name+1);
	    sf->mark_classes[index]     = copy(tok->gdef_mark[0][i].glyphs);
	}
	sf->mark_class_cnt = tok->gm_pos[0];
    }
    if ( tok->gm_pos[1]>sf->mark_set_cnt ) {
	int i;
	sf->mark_sets = realloc(sf->mark_sets,tok->gm_pos[1]*sizeof(char *));
	sf->mark_set_names = realloc(sf->mark_set_names,tok->gm_pos[1]*sizeof(char *));
	for ( i=0; i<tok->gm_cnt[1]; ++i ) if ( tok->gdef_mark[1][i].index>=sf->mark_set_cnt ) {
	    int index = tok->gdef_mark[1][i].index;
	    sf->mark_set_names[index] = copy(tok->gdef_mark[1][i].name+1);
	    sf->mark_sets[index]      = copy(tok->gdef_mark[1][i].glyphs);
	}
	sf->mark_set_cnt = tok->gm_pos[1];
    }

    sf->changed = true;
    FVSetTitles(sf);
    FVRefreshAll(sf);
}

static void CopySplineFontGroupsForFeatureFile(SplineFont *sf, struct parseState* tok) {
  struct ff_glyphclasses *ff_current = sf->groups;
  struct glyphclasses *feature_current = tok->classes;
  // It may be useful to run this multiple times, appending to an existing list, so we traverse to the end.
  while (feature_current != NULL && feature_current->next != NULL) feature_current = feature_current->next;
  struct glyphclasses *feature_tmp = NULL;
  while (ff_current != NULL) {
    // Only groups with feature-compatible names get copied.
    if (ff_current->classname != NULL && ff_current->classname[0] == '@') {
      feature_tmp = calloc(1, sizeof(struct glyphclasses));
      feature_tmp->classname = copy(ff_current->classname);
      feature_tmp->glyphs = copy(ff_current->glyphs);
      if (feature_current != NULL) feature_current->next = feature_tmp;
      else tok->classes = feature_tmp;
      feature_current = feature_tmp;
    }
    ff_current = ff_current->next;
  }
}

void SFApplyFeatureFile(SplineFont *sf,FILE *file,char *filename) {
    struct parseState tok;
    struct glyphclasses *gc, *gcnext;
    struct namedanchor *nap, *napnext;
    struct namedvalue *nvr, *nvrnext;

    memset(&tok,0,sizeof(tok));
    tok.line[0] = 1;
    tok.filename[0] = filename;
    tok.inlist[0] = file;
    tok.base = 10;
    if ( sf->cidmaster ) sf = sf->cidmaster;
    tok.sf = sf;
    CopySplineFontGroupsForFeatureFile(sf, &tok);

    locale_t tmplocale; locale_t oldlocale; // Declare temporary locale storage.
    switch_to_c_locale(&tmplocale, &oldlocale); // Switch to the C locale temporarily and cache the old locale.
    fea_ParseFeatureFile(&tok);
    switch_to_old_locale(&tmplocale, &oldlocale); // Switch to the cached locale.
    if ( tok.err_count==0 ) {
	tok.sofar = fea_reverseList(tok.sofar);
	fea_ApplyFile(&tok, tok.sofar);
	fea_NameLookups(&tok);
    } else
	ff_post_error("Not applied","There were errors when parsing the feature file and the features have not been applied");
    fea_featitemFree(tok.sofar);
    ScriptLangListFree(tok.def_langsyses);
    for ( gc = tok.classes; gc!=NULL; gc=gcnext ) {
	gcnext = gc->next;
	free(gc->classname); free(gc->glyphs);
	chunkfree(gc,sizeof(struct glyphclasses));
    }
    for ( nap = tok.namedAnchors; nap!=NULL; nap=napnext ) {
	napnext = nap->next;
	free(nap->name); AnchorPointsFree(nap->ap);
	chunkfree(nap,sizeof(*nap));
    }
    for ( nvr = tok.namedValueRs; nvr!=NULL; nvr=nvrnext ) {
	nvrnext = nvr->next;
	free(nvr->name); chunkfree(nvr->vr,sizeof(struct vr));
	chunkfree(nvr,sizeof(*nvr));
    }
    for ( int j=0; j<2; ++j ) {
	for ( int i=0; i<tok.gm_cnt[j]; ++i ) {
	    free(tok.gdef_mark[j][i].name);
	    free(tok.gdef_mark[j][i].glyphs);
	}
	free(tok.gdef_mark[j]);
    }
}

void SFApplyFeatureFilename(SplineFont *sf,char *filename) {
    FILE *in = fopen(filename,"r");

    if ( in==NULL ) {
	ff_post_error(_("Cannot open file"),_("Cannot open feature file %.120s"), filename );
return;
    }
    SFApplyFeatureFile(sf,in,filename);
    fclose(in);
}
