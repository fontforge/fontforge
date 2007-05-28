/* Copyright (C) 2000-2007 by George Williams */
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
#include "ttf.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <utype.h>
#include <ustring.h>

/* Adobe's opentype feature file */

/* ************************************************************************** */
/* ******************************* Output feat ****************************** */
/* ************************************************************************** */

static void dump_glyphname(FILE *out, SplineChar *sc) {
    if ( sc->parent->cidmaster!=NULL )
	fprintf( out, "\%d", sc->orig_pos /* That's the CID */ );
    else
	fprintf( out, "\%s", sc->name );
}

static void dump_glyphbyname(FILE *out, SplineFont *sf, char *name) {
    SplineChar *sc = SFGetChar(sf,-1,name);

    if ( sc==NULL )
	LogError( "No glyph named %s.", name );
    if ( sc->parent->cidmaster==NULL || sc==NULL )
	fprintf( out, "\%s", name );
    else
	fprintf( out, "\%s", sc->name );
}

static void dump_glyphnamelist(FILE *out, SplineFont *sf, char *names) {
    char *pt, *start;
    int ch;
    SplineChar *sc2;

    if ( sf->subfontcnt==0 )
	fprintf( out, "%s", names );
    else {
	for ( pt=names; ; ) {
	    while ( *pt==' ' ) ++pt;
	    if ( *pt=='\0' )
	break;
	    for ( start=pt; *pt!=' ' && *pt!='\0'; ++pt );
	    ch = *pt; *pt = '\0';
	    sc2 = SFGetChar(sf,-1,start);
	    if ( sc2==NULL ) {
		LogError( "No CID named %s", start);
		fprintf( out, "%s", start );
	    } else
		fprintf( out, "\%d", sc2->orig_pos );
	    *pt = ch;
	}
    }
}

static char *lookupname(OTLookup *otl) {
    char *pt1, *pt2;
    static char space[32];

    if ( otl->tempname != NULL )
return( otl->tempname );

    for ( pt1=otl->lookup_name,pt2=space; *pt1 && pt2<space+31; ++pt1 ) {
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
#ifdef FONTFORGE_CONFIG_DEVICETABLES
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
#endif
    putc('>',out);
}

static void dump_anchorpoint(FILE *out,AnchorPoint *ap) {

    if ( ap==NULL ) {
	fprintf( out, "<anchor NULL>" );
return;
    }

    fprintf( out, "<anchor %g %g", ap->me.x, ap->me.y );
    if ( ap->has_ttf_pt )
	fprintf( out, " %d", ap->ttf_pt_index );
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    else if ( ap->xadjust.corrections!=NULL || ap->yadjust.corrections!=NULL ) {
	putc(' ',out);
	dumpdevice(out,&ap->xadjust);
	putc(' ',out);
	dumpdevice(out,&ap->yadjust);
    }
#endif
    putc('>',out);
}

static void dump_kernclass(FILE *out,SplineFont *sf,struct lookup_subtable *sub) {
    int i,j;
    KernClass *kc = sub->kc;

    for ( i=0; i<kc->first_cnt; ++i ) if ( kc->firsts[i]!=NULL ) {
	fprintf( out, "    @kc%d_first_%d = [", sub->subtable_offset, i );
	dump_glyphnamelist(out,sf,kc->firsts[i] );
	fprintf( out, "];\n" );
    }
    for ( i=0; i<kc->second_cnt; ++i ) if ( kc->seconds[i]!=NULL ) {
	fprintf( out, "    @kc%d_second_%d = [", sub->subtable_offset, i );
	dump_glyphnamelist(out,sf,kc->seconds[i] );
	fprintf( out, "];\n" );
    }
    for ( i=0; i<kc->first_cnt; ++i ) if ( kc->firsts[i]!=NULL ) {
	for ( j=0; j<kc->second_cnt; ++i ) if ( kc->seconds[j]!=NULL ) {
	    fprintf( out, "    pos @kc%d_first_%d @kc%d_second_%d %d\n",
		    sub->subtable_offset, i,
		    sub->subtable_offset, j,
		    kc->offsets[i*kc->second_cnt+j]);
	}
    }
}

static char *nameend_from_class(char *glyphclass) {
    char *pt;

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
    int ch, ch2;

    space.u.pair.vr = pairvr;

    if ( r->u.glyph.back!=NULL ) {
	dump_glyphnamelist(out,sf,r->u.glyph.back );
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
	if ( otl!=NULL ) {
	    if ( otl->lookup_type == gpos_cursive )
		fprintf( out, "cursive " );
	    else if ( otl->lookup_type == gpos_mark2mark )
		fprintf( out, "mark " );
	}
	dump_glyphbyname(out,sf,start);
	if ( otl!=NULL ) {
	    putc( index&1 ? '"' : '\'', out );
	    if ( otl->lookup_type==gpos_single ) {
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
    if ( r->lookup_cnt!=0 && sub->lookup->lookup_type<gpos_start ) {
	fprintf( out, " by " );
	for ( i=0; i<r->lookup_cnt; ++i ) {
	    otl = r->lookups[i].lookup;
	    for ( pt=r->u.glyph.names, j=0; ; ) {
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
}

static void dump_contextpstcoverage(FILE *out,SplineFont *sf,
	struct lookup_subtable *sub, struct fpst_rule *r) {
    int i, pos, index;
    OTLookup *otl;
    PST *pst, space;
    char *start, *pt, *last_end;
    int ch, ch2;

    for ( i=0; i<r->u.coverage.bcnt; ++i ) {
	putc('[',out);
	dump_glyphnamelist(out,sf,r->u.coverage.bcovers[i] );
	fprintf( out, "] ");
    }
    for ( i=0; i<r->u.coverage.ncnt; ++i ) {
	putc('[',out);
	dump_glyphnamelist(out,sf,r->u.coverage.ncovers[i] );
	otl = lookup_in_rule(r,i,&index, &pos);
	if ( otl!=NULL ) {
	    putc( index&1 ? '"' : '\'', out );
	    /* Ok, I don't see any way to specify a class of value records */
	    /*  so just assume all vr will be the same for the class */
	    pt = nameend_from_class(r->u.coverage.ncovers[i]);
	    ch = *pt; *pt = '\0';
	    if ( otl->ticked && otl->lookup_type>=gpos_start )
		fprintf(out, "<lookup %s> ", lookupname(otl) );
	    else if ( otl->lookup_type==gpos_single ) {
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
	putc(' ',out);
    }
    for ( i=0; i<r->u.coverage.fcnt; ++i ) {
	putc('[',out);
	dump_glyphnamelist(out,sf,r->u.coverage.fcovers[i] );
	fprintf( out, "] ");
    }
    if ( r->lookup_cnt!=0 && sub->lookup->lookup_type<gpos_start ) {
	fprintf( out, " by " );
	for ( i=0; i<r->lookup_cnt; ++i ) {
	    otl = r->lookups[i].lookup;
	    if ( otl->ticked )
		fprintf(out, "<lookup %s> ", lookupname(otl) );
	    else if ( otl->lookup_type==gsub_single ) {
		putc('[',out);
		for ( pt=r->u.coverage.ncovers[r->lookups[i].seq]; ; ) {
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
		    putc(' ',out);
		}
		putc(']',out);
	    } else if ( otl->ticked ) {
		fprintf(out, "<lookup %s> ", lookupname(otl) );
	    } else if ( otl->lookup_type==gsub_ligature ) {
		/* If we get here assume there is only one ligature */
		pst = pst_any_from_otl(sf,otl);
		if ( pst!=NULL )
		    dump_glyphname(out,pst->u.lig.lig);
	    }
	}
    }
}

static void dump_contextpstclass(FILE *out,SplineFont *sf,
	struct lookup_subtable *sub, struct fpst_rule *r) {
    FPST *fpst = sub->fpst;
    int i, pos, index;
    OTLookup *otl;
    PST *pst, space;
    char *start, *pt, *last_start, *last_end;
    int ch, ch2;

    for ( i=0; i<r->u.class.bcnt; ++i )
	fprintf( out, "@cc%d_back_%d ", sub->subtable_offset, r->u.class.bclasses[i] );
    for ( i=0; i<r->u.class.ncnt; ++i ) {
	fprintf( out, "@cc%d_match_%d ", sub->subtable_offset, r->u.class.nclasses[i] );
	otl = lookup_in_rule(r,i,&index, &pos);
	if ( otl!=NULL ) {
	    putc( index&1 ? '"' : '\'', out );
	    /* Ok, I don't see any way to specify a class of value records */
	    /*  so just assume all vr will be the same for the class */
	    start = fpst->nclass[r->u.class.nclasses[i]];
	    pt = nameend_from_class(start);
	    ch = *pt; *pt = '\0';
	    if ( otl->ticked && otl->lookup_type>=gpos_start )
		fprintf(out, "<lookup %s> ", lookupname(otl) );
	    else if ( otl->lookup_type==gpos_single ) {
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
	putc(' ',out);
    }
    for ( i=0; i<r->u.class.fcnt; ++i )
	fprintf( out, "@cc%d_ahead_%d ", sub->subtable_offset, r->u.class.fclasses[i] );
    if ( r->lookup_cnt!=0 && sub->lookup->lookup_type<gpos_start ) {
	fprintf( out, " by " );
	for ( i=0; i<r->lookup_cnt; ++i ) {
	    otl = r->lookups[i].lookup;
	    if ( otl->ticked )
		fprintf(out, "<lookup %s> ", lookupname(otl) );
	    else if ( otl->lookup_type==gsub_single ) {
		putc('[',out);
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
		    putc(' ',out);
		}
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
	    for ( i=0; i<fpst->nccnt; ++i ) if ( fpst->nclass[i]!=NULL ) {
		fprintf( out, "    @cc%d_match_%d = [", sub->subtable_offset, i );
		dump_glyphnamelist(out,sf,fpst->nclass[i] );
		fprintf( out, "];\n" );
	    }
	}
	if ( fpst->bclass!=NULL ) {
	    for ( i=0; i<fpst->bccnt; ++i ) if ( fpst->bclass[i]!=NULL ) {
		fprintf( out, "    @cc%d_back_%d = [", sub->subtable_offset, i );
		dump_glyphnamelist(out,sf,fpst->bclass[i] );
		fprintf( out, "];\n" );
	    }
	}
	if ( fpst->fclass!=NULL ) {
	    for ( i=0; i<fpst->fccnt; ++i ) if ( fpst->fclass[i]!=NULL ) {
		fprintf( out, "    @cc%d_ahead_%d = [", sub->subtable_offset, i );
		dump_glyphnamelist(out,sf,fpst->fclass[i] );
		fprintf( out, "];\n" );
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
	fprintf( out, r->lookup_cnt==0 ? "    ignore subs " : "    subs " );
	if ( fpst->format==pst_class ) {
	    dump_contextpstclass(out,sf,sub,r);
	} else if ( fpst->format==pst_glyphs ) {
	    dump_contextpstglyphs(out,sf,sub,r);
	} else {
	    dump_contextpstcoverage(out,sf,sub,r);
	}
    }
}

static void dump_anchors(FILE *out,SplineFont *sf,struct lookup_subtable *sub) {
    int i, j, k, l;
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
	    do {
		_sf = k<sf->subfontcnt ? sf->subfonts[k] : sf;
		for ( i=0; i<_sf->glyphcnt; ++i ) if ( (sc=_sf->glyphs[i])!=NULL ) {
		    for ( ap=sc->anchor; ap!=NULL; ap=ap->next ) {
			if ( ap->anchor==ac && ap->type==at_mark ) {
			    if ( cnt>=max )
				marks = grealloc(marks,(max+=20)*sizeof(struct amarks));
			    marks[cnt].sc = sc;
			    marks[cnt].ap = ap;
			    fprintf( out, "    mark ");
			    dump_glyphname(out,sc);
			    putc(' ',out);
			    dump_anchorpoint(out,ap);
			    fprintf( out, ";\n");
			    ++cnt;
		    break;
			}
		    }
		}
		++k;
	    } while ( k<sf->subfontcnt );
	    if ( cnt==0 )
    continue;		/* No marks? Nothing to be done */
	    if ( sub->lookup->lookup_type!=gpos_mark2ligature ) {
		do {
		    _sf = k<sf->subfontcnt ? sf->subfonts[k] : sf;
		    for ( i=0; i<_sf->glyphcnt; ++i ) if ( (sc=_sf->glyphs[i])!=NULL ) {
			for ( ap=sc->anchor; ap!=NULL; ap=ap->next ) {
			    if ( ap->anchor==ac && ap->type!=at_mark ) {
				fprintf( out, "    pos ");
			        if ( sub->lookup->lookup_type == gpos_mark2mark )
				    fprintf( out, "mark " );
				dump_glyphname(out,sc);
				putc(' ',out);
				dump_anchorpoint(out,ap);
				fprintf( out, " mark [" );
				for ( j=0; j<cnt; ++j ) {
				    dump_glyphname(out,marks[j].sc);
				    putc(' ',out);
				}
				fprintf(out,"]\n" );
			    }
			}
		    }
		    ++k;
		} while ( k<sf->subfontcnt );
	    } else {
		struct amarks *lig=NULL;
		int lcnt, lmax=0, ligmax;
		do {
		    _sf = k<sf->subfontcnt ? sf->subfonts[k] : sf;
		    for ( i=0; i<_sf->glyphcnt; ++i ) if ( (sc=_sf->glyphs[i])!=NULL ) {
			lcnt = 0; ligmax=1;
			for ( ap=sc->anchor; ap!=NULL; ap=ap->next ) {
			    if ( ap->anchor==ac && ap->type!=at_mark ) {
				if ( lcnt>=lmax )
				    lig = grealloc(lig,(lmax+=20)*sizeof(struct amarks));
				lig[lcnt].sc = sc;
				lig[lcnt].ap = ap;
			        if ( ap->lig_index>ligmax )
				    ligmax = ap->lig_index;
				++lcnt;
			    }
			}
			if ( lcnt>0 ) {
			    fprintf( out, "    pos " );
			    dump_glyphname(out,sc);
			    for ( j=0; j<=ligmax; ++j ) {
				putc(' ',out );
			        for ( l=0; l<lcnt; ++l ) {
				    if ( lig[l].ap->lig_index==ligmax ) {
					dump_anchorpoint(out,lig[l].ap);
				break;
				    }
				}
			        if ( l==lcnt )
				    dump_anchorpoint(out,NULL);
			    }
			    fprintf( out, " mark [" );
			    for ( j=0; j<cnt; ++j ) {
				dump_glyphname(out,marks[j].sc);
				putc(' ',out);
			    }
			    fprintf(out,"]\n" );
			}
		    }
		    ++k;
		} while ( k<sf->subfontcnt );
		free(lig);
	    }
	    free(marks);
	}
    }
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
    /* Nor can other contextuals */
    /* Nor can alternate/multiple substitutions */
    /* If we are class based or coverage table based (and interesting classes */
    /*  contain more than one glyph name) then we can't express single_pos, */
    /*  kerning, ligatures, cursive anchors (unless everything in the lookup */
    /*  results in the same transformation) */

    switch ( nested->lookup_type ) {
      case gsub_single:
return( true );			/* This is the one thing that can always be expressed */
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
return( false );		/* These can never be expressed */
      case gpos_cursive:
      case gpos_pair:
return( fpst->format==pst_glyphs );
      case gsub_ligature:
      case gpos_single:
	if ( fpst->format==pst_glyphs )
return( true );
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
return( false );			/* Different glyphs */
			else if ( found->u.pos.xoff!=pst->u.pos.xoff ||
				found->u.pos.yoff!=pst->u.pos.yoff ||
				found->u.pos.h_adv_off!=pst->u.pos.h_adv_off ||
				found->u.pos.v_adv_off!=pst->u.pos.v_adv_off )
return( false );
			else
		break;
		    }
		}
	    }
	    ++k;
	} while ( k<sf->subfontcnt );
	if ( found==NULL )
return( false );
return( true );
      default:
return( false );
    }
}

static void dump_lookup(FILE *out, SplineFont *sf, OTLookup *otl);

static void dump_needednestedlookups(FILE *out, SplineFont *sf, OTLookup *otl) {
    struct lookup_subtable *sub;
    int r, s;
    /* So we cheat, and extend the fea format to allow us to specify a lookup */
    /*  in contextuals */

    for ( sub = otl->subtables; sub!=NULL; sub=sub->next ) {
	FPST *fpst = sub->fpst;
	for ( r=0; r<fpst->rule_cnt; ++r ) {
	    for ( s=0; s<fpst->rules[r].lookup_cnt; ++r ) {
		OTLookup *nested = fpst->rules[r].lookups[s].lookup;
		if ( nested!=NULL && nested->features==NULL && !nested->ticked &&
			fea_bad_contextual_nestedlookup(sf,fpst,nested))
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
    if ( otl->lookup_flags==0 || otl->lookup_flags>15 )
	fprintf( out, "  lookupflag %d\n", otl->lookup_flags );
    else {
	fprintf( out, "  lookupflag" );
	first = true;
	for ( i=0; i<4; ++i ) if ( otl->lookup_flags&(1<<i)) {
	    if ( !first )
		putc(',',out);
	    else
		first = false;
	    fprintf( out, " %s", flagnames[i] );
	}
	putc('\n',out);
    }
    for ( sub=otl->subtables; sub!=NULL; sub=sub->next ) {
	if ( sub!=otl->subtables )
	    fprintf( out, "  subtable\n" );
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
			    fprintf( out,"\n" );
			  break;
			  case gsub_alternate:
			    fprintf( out, "    sub " );
			    dump_glyphname(out,sc);
			    fprintf( out, " from [" );
			    dump_glyphnamelist(out,sf,pst->u.alt.components );
			    fprintf( out,"]\n" );
			  break;
			  case gsub_ligature:
			    fprintf( out, "    sub " );
			    dump_glyphnamelist(out,sf,pst->u.lig.components );
			    fprintf( out, " by " );
			    dump_glyphname(out,sc);
			    fprintf( out,"\n" );
			  break;
			  case gpos_single:
			    fprintf( out, "    pos " );
			    dump_glyphname(out,sc);
			    putc(' ',out);
			    dump_valuerecord(out,&pst->u.pos);
			    fprintf( out,"\n" );
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
			    fprintf( out,"\n" );
			  break;
			  default:
			    /* Eh? How'd we get here? An anchor class with */
			    /* no anchors perhaps? */
			  break;
			}
		    }
		    for ( isv=0; isv<2; ++isv ) {
			for ( kp=isv ? sc->vkerns : sc->kerns; kp!=NULL; kp=kp->next ) if ( kp->subtable==sub ) {
			    fprintf( out, "    pos " );
			    dump_glyphname(out,sc);
			    putc(' ',out);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
			    if ( kp->adjust!=NULL ) {
				/* If we have a device table then we must use the long */
			        /* format */
				if ( isv ) {
				    fprintf( out," < 0 0 0 %d <device NULL> <device NULL> <device NULL> ",
					    kp->off );
				    dumpdevice(out,kp->adjust);
			            fprintf( out, " > " );
			            dump_glyphname(out,kp->sc);
			            fprintf( out, " < 0 0 0 0 >\n" );
				} else if ( otl->lookup_flags&pst_r2l ) {
				    fprintf( out, " < 0 0 0 0 > " );
			            dump_glyphname(out,kp->sc);
				    fprintf( out," < 0 0 %d 0 <device NULL> <device NULL> ",
					    kp->off );
				    dumpdevice(out,kp->adjust);
			            fprintf( out, " <device NULL>>\n" );
				} else {
				    fprintf( out," < 0 0 %d 0 <device NULL> <device NULL> ",
					    kp->off );
				    dumpdevice(out,kp->adjust);
			            fprintf( out, " <device NULL>> " );
			            dump_glyphname(out,kp->sc);
			            fprintf( out, " < 0 0 0 0 >\n" );
				}
			    } else
#endif
			    if ( otl->lookup_flags&pst_r2l ) {
				fprintf( out, " < 0 0 0 0 > " );
				dump_glyphname(out,kp->sc);
				fprintf( out," < 0 0 %d 0 >\n", kp->off );
			    } else {
				dump_glyphname(out,kp->sc);
				putc(' ',out);
			        fprintf( out, "%d\n", kp->off );
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

    dump_lookup(out,sf,otl);

    for ( fl = otl->features; fl!=NULL; fl=fl->next ) {
	fprintf( out, "\nfeature %c%c%c%c {\n",
		fl->featuretag>>24, fl->featuretag>16, fl->featuretag>>8, fl->featuretag );
	for ( sl = fl->scripts; sl!=NULL; sl=sl->next ) {
	    fprintf( out, "  script %c%c%c%c;\n",
		    sl->script>>24, sl->script>16, sl->script>>8, sl->script );
	    for ( l=0; l<sl->lang_cnt; ++l ) {
		uint32 lang = l<MAX_LANG ? sl->langs[l] : sl->morelangs[l-MAX_LANG];
		fprintf( out, "     language %c%c%c%c %s;\n",
			lang>>24, lang>16, lang>>8, lang,
			lang!=DEFAULT_LANG ? "exclude_dflt" : "" );
		fprintf( out, "      lookup %s;\n", lookupname(otl));
	    }
	}
	fprintf( out, "\n} %c%c%c%c\n",
		fl->featuretag>>24, fl->featuretag>16, fl->featuretag>>8, fl->featuretag );
    }
}

static void dump_gdef(FILE *out,SplineFont *sf) {
    PST *pst;
    int i,gid,j,k,l, lcnt, needsclass;
    SplineChar *sc;
    SplineFont *_sf;
    struct lglyphs { SplineChar *sc; PST *pst; } *glyphs;

    glyphs = NULL;
    for ( l=0; l<2; ++l ) {
	lcnt = 0;
	needsclass = false;
	k=0;
	do {
	    _sf = sf->subfontcnt==0 ? sf : _sf;
	    for ( gid=0; gid<_sf->glyphcnt; ++gid ) if ( (sc=_sf->glyphs[gid])!=NULL ) {
		if ( sc->glyph_class!=0 || gdefclass(sc)!=1 )
		    needsclass = true;
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
	glyphs = galloc((lcnt+1)*sizeof(struct lglyphs));
	glyphs[lcnt].sc = NULL;
    }

    if ( !needsclass && lcnt==0 /* && sf->mark_class_cnt==0*/ )
return;					/* No anchor positioning, no ligature carets */

    fprintf( out, "\ntable GDEF {\n" );
    if ( needsclass ) {
	static char *clsnames[] = { "@GDEF_Simple", "@GDEF_Ligature", "@GDEF_Mark", "@GDEF_Component" };
	for ( i=0; i<4; ++i ) {
	    fprintf( out, "  %s = [", clsnames[i] );
	    k=0;
	    do {
		_sf = sf->subfontcnt==0 ? sf : _sf;
		for ( gid=0; gid<_sf->glyphcnt; ++gid ) if ( (sc=_sf->glyphs[gid])!=NULL ) {
		    if ( sc->glyph_class==i+1 || (sc->glyph_class==0 && gdefclass(sc)==i+1 )) {
			dump_glyphname(out,sc);
			putc(' ',out);
		    }
		}
		++k;
	    } while ( k<sf->subfontcnt );
	    fprintf( out, "]\n");
	}
	fprintf( out, "  GlyphClassDef %s %s %s %s;\n\n",
		clsnames[0], clsnames[1], clsnames[2], clsnames[3]);
    }

    for ( i=0; i<lcnt; ++i ) {
	PST *pst = glyphs[i].pst;
	fprintf( out, "  LigatureCaret " );
	dump_glyphname(out,glyphs[i].sc);
	for ( k=0; k<pst->u.lcaret.cnt; ++k ) {
	    fprintf( out, " <caret %d>", pst->u.lcaret.carets[k]);
	}
	fprintf( out, ";\n" );
    }
    free( glyphs );

    /* no way to specify mark classes */

    fprintf( out, "} GDEF;\n\n" );
}

static void dump_gsubgpos(FILE *out, SplineFont *sf) {
    int isgpos;
    int i,l,s, subl;
    OTLookup *otl;
    FeatureScriptLangList *fl;
    struct scriptlanglist *sl;

    for ( isgpos=0; isgpos<2; ++isgpos ) {
	uint32 *feats = SFFeaturesInScriptLang(sf,isgpos,0xffffffff,0xffffffff);
	if ( feats[0]!=0 ) {
	    uint32 *scripts = SFScriptsInLookups(sf,isgpos);
	    fprintf( out, "\n# %s \n\n", isgpos ? "GPOS" : "GSUB" );
	    for ( otl= isgpos ? sf->gpos_lookups : sf->gsub_lookups; otl!=NULL; otl=otl->next )
		if ( otl->features!=NULL )	/* Nested lookups will be output with the lookups which invoke them */
		    dump_lookup( out, sf, otl );
	    for ( i=0; feats[i]!=0; ++i ) {
		fprintf( out, "\nfeature %c%c%c%c {\n",
			feats[i]>>24, feats[i]>>16, feats[i]>>8, feats[i] );
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
				    fprintf( out, "\%04x", ch );
			    }
			    fprintf( out, "\";\n" );
			}
		    } else
			fprintf( out, " 0 0 0 0;\n" );
		} else for ( s=0; scripts[s]!=0; ++s ) {
		    uint32 *langs = SFLangsInScript(sf,isgpos,scripts[s]);
		    int firsts = true;
		    for ( l=0; langs[l]!=0; ++l ) {
			int first = true;
			for ( otl = isgpos ? sf->gpos_lookups : sf->gsub_lookups; otl!=NULL; otl=otl->next ) {
			    for ( fl=otl->features; fl!=NULL; fl=fl->next ) if ( fl->featuretag==feats[i] ) {
				for ( sl=fl->scripts; sl!=NULL; sl=sl->next ) if ( sl->script==scripts[s] ) {
				    for ( subl=0; subl<sl->lang_cnt; ++subl ) {
					uint32 lang = subl<MAX_LANG ? sl->langs[subl] : sl->morelangs[l-MAX_LANG];
			                if ( lang==langs[l] )
			    goto found;
				    }
				}
			    }
			    found:
			    if ( fl!=NULL ) {
				if ( firsts ) {
				    fprintf( out, "\n  script %c%c%c%c;\n",
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
		fprintf( out, "} %c%c%c%c;\n",
			    feats[i]>>24, feats[i]>>16, feats[i]>>8, feats[i] );
	    }
	    free(scripts);
	}
	free(feats);
    }
}

/* Sadly the lookup names I generate are often not distinguishable in the */
/*  first 31 characters. So provide a fall back mechanism if we get a name */
/*  collision */
static void preparenames(SplineFont *sf) {
    int isgpos, cnt, try, i;
    OTLookup *otl;
    char **names, *name;
    char namebuf[32], featbuf[8], scriptbuf[8], *feat, *script;
    struct scriptlanglist *sl;

    cnt = 0;
    for ( isgpos=0; isgpos<2; ++isgpos )
	for ( otl=isgpos ? sf->gpos_lookups : sf->gsub_lookups; otl!=NULL; otl=otl->next )
	    ++cnt;
    if ( cnt==0 )
return;
    names = galloc(cnt*sizeof(char *));
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
		sprintf( namebuf, "%s_%s_%s%s_%d", isgpos ? "pos" : "subs",
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

    untick_lookups(sf);
    preparenames(sf);
    dump_gsubgpos(out,sf);
    dump_gdef(out,sf);
    cleanupnames(sf);
}


/* ************************************************************************** */
/* ******************************* Parse feat ******************************* */
/* ************************************************************************** */

#include <gfile.h>

struct feat_item {
    enum feat_type { ft_lookup_start, ft_lookup_end, ft_feat_start, ft_feat_end,
	ft_table,
	ft_subtable, ft_script, ft_lang, ft_lookupflags, ft_langsys,
	ft_pst, ft_pstclass, ft_fpst, ft_ap, ft_lookup_ref } type;
    union {
	SplineChar *sc;		/* For psts, aps */
	char *class;		/* List of glyph names for kerning by class */
	char *lookup_name;	/* for lookup_start/ lookup_ref */
	uint32 tag;		/* for feature/script/lang tag */
    } u1;
    union {
	PST *pst;
		/* For kerning by class we'll generate an invalide pst with the class as the "paired" field */
	FPST *fpst;
	AnchorPoint *ap;
	int lookupflags;
	struct scriptlanglist *sl;	/* Default langsyses for features/langsys */
	int exclude_dflt;		/* for lang tags */
    } u2;
    char *mark_class;			/* For mark to base-ligature-mark, names of all marks which attach to this anchor */
    struct feat_item *next;
};

#define MAXT	40
#define MAXI	5
struct parseState {
    char tokbuf[MAXT+1];
    long value;
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
	tk_table, tk_useExtension
    } type;
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
    SplineFont *sf;
    struct scriptlanglist *def_langsyses;
    struct glyphclasses { char *classname, *glyphs; struct glyphclasses *next; } *classes;
    struct feat_item *sofar;
};

static struct keywords {
    char *name;
    enum toktype tok;
} fea_keywords[] = {
/* list must be in toktype order */
    { "name", tk_name }, { "glyphclass", tk_class }, { "integer", tk_int },
    { "random character", tk_char}, { "cid", tk_cid }, { "EOF", tk_eof },
/* keywords now */
    { "anchor", tk_anchor },
    { "anonymous", tk_anonymous },
    { "anon", tk_anonymous },
    { "by", tk_by },
    { "caret", tk_caret },
    { "cursive", tk_cursive },
    { "device", tk_device },
    { "enumerate", tk_enumerate },
    { "enum", tk_enumerate },
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
    { "pos", tk_position },
    { "required", tk_required },
    { "RightToLeft", tk_RightToLeft },
    { "script", tk_script },
    { "substitute", tk_substitute },
    { "sub", tk_substitute },
    { "subtable", tk_subtable },
    { "table", tk_table },
    { "useExtension", tk_useExtension },
    NULL
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
}

static void fea_ParseTok(struct parseState *tok) {
    FILE *in = tok->inlist[tok->inc_depth];
    int ch, peekch;
    char *pt, *start;

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
    if ( ch=='\\' ) {
	peekch=getc(in);
	ungetc(peekch,in);
    }

    if ( isdigit(ch) || ch=='-' || ch=='+' || (ch=='\\' && isdigit(peekch)) ) {
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
	while ( isdigit( ch ) && pt<tok->tokbuf+15 ) {
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
	tok->value = strtol(tok->tokbuf,NULL,10);
return;
    } else if ( ch=='@' || ch=='_' || ch=='\\' || isalnum(ch)) {	/* Names can't start with dot */
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
	while (( isalnum(ch) || ch=='_' || ch=='.' ) && pt<start+31 ) {
	    *pt++ = ch;
	    ch = getc(in);
	}
	*pt = '\0';
	if ( isalnum(ch) || ch=='_' || ch=='.' ) {
	    LogError(_("Name too long on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	} else if ( pt==start ) {
	    LogError(_("Missing name on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	}

	if ( check_keywords ) {
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
	    LogError(_("Unexpected character (0x%02X) on line %d of %s"), ch, tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
  goto skip_whitespace;
	}
    }
}

static void fea_TokenMustBe(struct parseState *tok, enum toktype type, int ch) {
    fea_ParseTok(tok);
    if ( type==tk_char && (tok->type!=type || tok->tokbuf[0]!=ch) ) {
	LogError(_("Expected '%c' on line %d of %s"), ch, tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    } else if ( type!=tk_char && tok->type!=type ) {
	LogError(_("Expected '%s' on line %d of %s"), fea_keywords[type],
		tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    }
}

static void fea_skip_to_semi(struct parseState *tok) {

    while ( tok->type!=tk_char || tok->tokbuf[0]!=';' ) {
	fea_ParseTok(tok);
	if ( tok->type==tk_eof )
    break;
    }
}

static void fea_skip_to_close_curly(struct parseState *tok) {
    int nest=0;

    while ( tok->type!=tk_char || tok->tokbuf[0]!=';' || nest>0 ) {
	fea_ParseTok(tok);
	if ( tok->type==tk_char ) {
	    if ( tok->tokbuf[0]=='{' ) ++nest;
	    else if ( tok->tokbuf[0]=='}' ) --nest;
	}
	if ( tok->type==tk_eof )
    break;
    }
}

static void fea_end_statement(struct parseState *tok) {
    fea_ParseTok(tok);
    if ( tok->type!=tk_char || tok->tokbuf[0]!=';' ) {
	LogError(_("Expected ';' at statement end on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	fea_skip_to_semi(tok);
	++tok->err_count;
return;
    }
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

    for ( test=tok->classes; test!=NULL; test=test->next ) {
	if ( strcmp(classname,test->classname)==0 )
return( test->glyphs );
    }
    LogError(_("Use of undefined glyph class, %s, on line %d of %s"), classname, tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
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
	    glyphs = grealloc(glyphs,(*_max+=200+len+1)+1);
	glyphs[cnt++] = ' ';
	strcpy(glyphs+cnt,contents);
    }
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
return( sc->name );
}

static SplineChar *fea_glyphname_get(struct parseState *tok,char *name) {
    SplineFont *sf = tok->sf;
    SplineChar *sc = SFGetChar(sf,-1,name);
    int enc;

    if ( sf->subfontcnt!=0 ) {
	LogError(_("Reference to a glyph name in a CID-keyed font on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
return(sc);
    }

    if ( sc!=NULL )
return( sc );
    enc = SFFindSlot(sf,sf->fv->map,-1,name);
    if ( enc!=-1 )
return( SFMakeChar(sf,sf->fv->map,enc));
/* Don't encode it (not in current encoding), just add it, so we needn't */
/*  mess with maps or selections */
    SFExpandGlyphCount(sf,sf->glyphcnt+1);
    sc = SplineCharCreate();
    sc->name = copy(name);
    sc->unicodeenc = UniFromName(name,ui_none,&custom);
    sc->parent = sf;
    sc->orig_pos = sf->glyphcnt-1;
    sf->glyphs[sc->orig_pos] = sc;
return( sc );
}

static char *fea_glyphname_validate(struct parseState *tok,char *name) {
    SplineChar *sc = fea_glyphname_get(tok,name);

    if ( sc==NULL )
return( NULL );

return( sc->name );
}

static char *fea_ParseGlyphClass(struct parseState *tok) {
    char *contents;

    if ( tok->type==tk_class ) {
	contents = fea_lookup_class_complain(tok,tok->tokbuf);
    } else if ( tok->type!=tk_char || tok->tokbuf[0]!='[' ) {
	LogError(_("Expected '[' in glyph class definition on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
return(NULL);
    } else {
	char *glyphs = NULL;
	int cnt=0, max=0;
	forever {
	    fea_ParseTok(tok);
	    if ( tok->type==tk_char && tok->tokbuf[0]==']' )
	break;
	    if ( tok->type==tk_class ) {
		contents = fea_lookup_class_complain(tok,tok->tokbuf);
	    } else if ( tok->type==tk_cid ) {
		contents = fea_cid_validate(tok,tok->value);
	    } else if ( tok->type==tk_name ) {
		contents = fea_glyphname_validate(tok,tok->tokbuf);
	    } else {
		LogError(_("Expected glyph name, cid, or class in glyph class definition on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
return(NULL);
	    }
	    if ( contents==NULL ) {
return(NULL);
	    }
	    cnt = fea_AddGlyphs(&glyphs,&max,cnt,contents);
	}
	if ( contents==NULL )
	    contents = copy("");	/* Is it legal to have an empty class? I can't think of any use for one */
    }
return( contents );
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
		tok->type==tk_IgnoreMarks || tok->type==tk_IgnoreLigatures ) {
	    if ( tok->type == tk_RightToLeft )
		val |= pst_r2l;
	    else if ( tok->type == tk_IgnoreBaseGlyphs )
		val |= pst_ignorebaseglyphs;
	    else if ( tok->type == tk_IgnoreMarks )
		val |= pst_ignorecombiningmarks;
	    else if ( tok->type == tk_IgnoreLigatures )
		val |= pst_ignoreligatures;
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
    contents = fea_ParseGlyphClass(tok);
    if ( contents==NULL ) {
	fea_skip_to_semi(tok);
return;
    }
    fea_AddClassDef(tok,classname,copy(contents));
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
	sl->morelangs = grealloc(sl->morelangs,(sl->lang_cnt+1)*sizeof(uint32));
	sl->morelangs[sl->lang_cnt++ - MAX_LANG] = lang;
    }
    fea_end_statement(tok);

    if ( inside_feat ) {
	struct feat_item *item = chunkalloc(sizeof(struct feat_item));
	item->type = ft_langsys;
	item->u2.sl = SLCopy(tok->def_langsyses);
	item->next = tok->sofar;
	tok->sofar = item;
    }
}

struct markedglyphs {
    unsigned int has_marks: 1;		/* Are there any marked glyphs in the entire sequence? */
    unsigned int is_cursive: 1;		/* Only in a position sequence */
    unsigned int is_mark: 1;		/* Only in a position sequence/mark keyword=>mark2mark */
    unsigned int is_name: 1;		/* Otherwise a class */
    unsigned int is_lookup: 1;		/* Or a lookup when parsing a subs replacement list */
    uint16 mark_count;			/* 0=>unmarked, 1=>first mark, etc. */
    char *name_or_class;		/* Glyph name / class contents */
    struct vr *vr;			/* A value record. Only in position sequences */
    int ap_cnt;				/* Number of anchor points */
    AnchorPoint **anchors;
    char *lookupname;
    struct markedglyphs *next;
};

#ifdef FONTFORGE_CONFIG_DEVICETABLES
static void fea_ParseDeviceTable(struct parseState *tok,DeviceTable *adjust)
#else
static void fea_ParseDeviceTable(struct parseState *tok)
#endif
	{
    int first = true;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    int min=0, max= -1;
    int8 values[512];

    memset(values,0,sizeof(values));
#endif

    fea_TokenMustBe(tok,tk_device,'\0');
    if ( tok->type!=tk_device )
return;

    forever {
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
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    int pixel = tok->value;
#endif
	    fea_TokenMustBe(tok,tk_int,'\0');
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    if ( pixel>=sizeof(values) || pixel<0 )
		LogError(_("Pixel size too big in device table on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    else {
		values[pixel] = tok->value;
		if ( max==-1 )
		    min=max=pixel;
		else if ( pixel<min ) min = pixel;
		else if ( pixel>max ) max = pixel;
	    }
#endif
	}
	first = false;
    }
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    if ( max!=-1 ) {
	int i;
	adjust->first_pixel_size = min;
	adjust->last_pixel_size = max;
	adjust->corrections = galloc(max-min+1);
	for ( i=min; i<=max; ++i )
	    adjust->corrections[i-min] = values[i];
    }
#endif
}

static AnchorPoint *fea_ParseAnchor(struct parseState *tok) {
    AnchorPoint *ap = NULL;

    if ( tok->type==tk_anchor ) {
	fea_ParseTok(tok);
	if ( tok->type==tk_NULL )
	    ap = NULL;
	else if ( tok->type==tk_int ) {
	    ap = chunkalloc(sizeof(AnchorPoint));
	    ap->me.x = tok->value;
	    fea_TokenMustBe(tok,tk_int,'\0');
	    ap->me.y = tok->value;
	    fea_ParseTok(tok);
	    if ( tok->type==tk_char && tok->tokbuf[0]=='<' ) {
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		fea_ParseDeviceTable(tok,&ap->xadjust);
		fea_TokenMustBe(tok,tk_char,'<');
		fea_ParseDeviceTable(tok,&ap->yadjust);
#else
		fea_ParseDeviceTable(tok);
		fea_TokenMustBe(tok,tk_char,'<');
		fea_ParseDeviceTable(tok);
#endif
		fea_TokenMustBe(tok,tk_char,'>');
	    } else if ( tok->type!=tk_char || tok->tokbuf[0]!='>' ) {
		LogError(_("Expected '>' in anchor on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
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

static int fea_findLookup(struct parseState *tok,char *name ) {
    struct feat_item *feat;

    for ( feat=tok->sofar; feat!=NULL; feat=feat->next ) {
	if ( feat->type==ft_lookup_start && strcmp(name,feat->u1.lookup_name)==0 )
return( true );
    }

    if ( SFFindLookup(tok->sf,name)!=NULL ) {
	if ( !tok->lookup_in_sf_warned ) {
	    gwwv_post_notice(_("Refers to Font"),_("Reference to a lookup which is not in the feature file but which is in the font, %.50s"), name );
	    tok->lookup_in_sf_warned = true;
	}
return( true );
    }

return( false );
}

static void fea_ParseBroket(struct parseState *tok,struct markedglyphs *last) {
    /* We've read the open broket. Might find: value record, anchor, lookup */
    /* (lookups are my extension) */
    struct vr *vr;

    fea_ParseTok(tok);
    if ( tok->type==tk_lookup ) {
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
	last->anchors = grealloc(last->anchors,(++last->ap_cnt)*sizeof(AnchorPoint *));
	last->anchors[last->ap_cnt-1] = fea_ParseAnchor(tok);
    } else if ( tok->type==tk_NULL ) {
	/* NULL value record. Adobe documents it and doesn't implement it */
	/* Not sure what it's good for */
	fea_TokenMustBe(tok,tk_char,'>');
    } else if ( tok->type==tk_int ) {
	last->vr = vr = chunkalloc(sizeof( struct vr ));
	vr->xoff = tok->value;
	fea_ParseTok(tok);
	if ( tok->type==tk_char && tok->tokbuf[0]=='>' ) {
	    if ( tok->in_vkrn )
		vr->v_adv_off = vr->xoff;
	    else
		vr->h_adv_off = vr->xoff;
	    vr->xoff = 0;
	} else if ( tok->type!=tk_int ) {
	    LogError(_("Expected integer in value record on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	} else {
	    vr->yoff = tok->value;
	    fea_TokenMustBe(tok,tk_int,'\0');
	    vr->h_adv_off = tok->value;
	    fea_TokenMustBe(tok,tk_int,'\0');
	    vr->v_adv_off = tok->value;
	    fea_ParseTok(tok);
	    if ( tok->type==tk_char && tok->tokbuf[0]=='<' ) {
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		vr->adjust = chunkalloc(sizeof(struct valdev));
		fea_ParseDeviceTable(tok,&vr->adjust->xadjust);
		fea_TokenMustBe(tok,tk_char,'<');
		fea_ParseDeviceTable(tok,&vr->adjust->yadjust);
		fea_TokenMustBe(tok,tk_char,'<');
		fea_ParseDeviceTable(tok,&vr->adjust->xadv);
		fea_TokenMustBe(tok,tk_char,'<');
		fea_ParseDeviceTable(tok,&vr->adjust->yadv);
#else
		fea_ParseDeviceTable(tok);
		fea_TokenMustBe(tok,tk_char,'<');
		fea_ParseDeviceTable(tok);
		fea_TokenMustBe(tok,tk_char,'<');
		fea_ParseDeviceTable(tok);
		fea_TokenMustBe(tok,tk_char,'<');
		fea_ParseDeviceTable(tok);
#endif
		fea_TokenMustBe(tok,tk_char,'>');
	    } else if ( tok->type!=tk_char || tok->tokbuf[0]!='>' ) {
		LogError(_("Expected '>' in value record on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
	    }
	}
    }
}

static struct markedglyphs *fea_ParseMarkedGlyphs(struct parseState *tok,
	int is_pos, int allow_marks, int allow_lookups) {
    int mark_cnt = 0, last_mark=0, is_cursive = false, is_mark=false;
    struct markedglyphs *head=NULL, *last=NULL, *prev=NULL, *cur;
    int first = true;

    forever {
	fea_ParseTok(tok);
	cur = NULL;
	if ( first && is_pos && tok->type == tk_cursive )
	    is_cursive = true;
	else if ( first && is_pos && tok->type == tk_mark )
	    is_mark = true;
	else if ( tok->type==tk_name ) {
	    cur = chunkalloc(sizeof(struct markedglyphs));
	    cur->is_cursive = is_cursive;
	    cur->is_mark = is_mark;
	    cur->is_name = true;
	    cur->name_or_class = copy(tok->tokbuf);
	} else if ( tok->type == tk_class || (tok->type==tk_char && tok->tokbuf[0]=='[')) {
	    cur = chunkalloc(sizeof(struct markedglyphs));
	    cur->is_cursive = is_cursive;
	    cur->is_mark = is_mark;
	    cur->is_name = false;
	    cur->name_or_class = fea_ParseGlyphClass(tok);
	} else if ( allow_marks && tok->type==tk_char &&
		(tok->tokbuf[0]=='\'' || tok->tokbuf[0]=='"') && last!=NULL ) {
	    if ( last_mark!=tok->tokbuf[0] || (prev!=NULL && !prev->has_marks)) {
		++mark_cnt;
		last_mark = tok->tokbuf[0];
	    }
	    last->mark_count = mark_cnt;
	} else if ( is_pos && last!=NULL && last->vr==NULL && tok->type == tk_int ) {
	    last->vr = chunkalloc(sizeof(struct vr));
	    if ( tok->in_vkrn )
		last->vr->v_adv_off = tok->value;
	    else
		last->vr->h_adv_off = tok->value;
	} else if ( is_pos && last!=NULL && tok->type == tk_char && tok->tokbuf[0]=='<' ) {
	    fea_ParseBroket(tok,last);
	} else if ( !is_pos && allow_lookups && tok->type == tk_char && tok->tokbuf[0]=='<' ) {
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
	first = false;
    }
    if ( head!=NULL && mark_cnt!=0 )
	head->has_marks = true;
return( head );	
}

static void fea_markedglyphsFree(struct markedglyphs *gl) {
    struct markedglyphs *next;
    int i;

    while ( gl!=NULL ) {
	next = gl->next;
	free(gl->name_or_class);
	free(gl->lookupname);
	for ( i=0; i<gl->ap_cnt; ++i )
	    AnchorPointsFree(gl->anchors[i]);
	free(gl->anchors);
	if ( gl->vr!=NULL ) {
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    ValDevFree(gl->vr->adjust);
#endif
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
    forever {
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
	if ( glyphs->next!=NULL ) {
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
    names = pt = galloc(len+1);
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
    forever {
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

    memset(vr,0,sizeof(vr));
    if ( glyphs->vr==NULL )
	vr[0] = *glyphs->next->vr;
    else {
	vr[0] = *glyphs->vr;
	if ( glyphs->next->vr!=NULL )
	    vr[1] = *glyphs->next->vr;
    }
    if ( enumer || (glyphs->is_name && glyphs->next->is_name)) {
	start = glyphs->name_or_class;
	forever {
	    while ( *start==' ' ) ++start;
	    if ( *start=='\0' )
	break;
	    for ( pt=start; *pt!='\0' && *pt!=' '; ++pt );
	    ch = *pt; *pt = '\0';
	    sc = fea_glyphname_get(tok,start);
	    *pt = ch; start = pt;
	    if ( sc!=NULL ) {
		start2 = glyphs->next->name_or_class;
		forever {
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
	    forever {
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
	forever {
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

static struct feat_item *fea_process_sub_ligature(struct parseState *tok,
	struct markedglyphs *glyphs, struct markedglyphs *rpl,
	struct feat_item *sofar ) {
    SplineChar *sc;
    struct markedglyphs *g;

    /* I store ligatures backwards, in the ligature glyph not the glyphs being substituted */
    sc = fea_glyphname_get(tok,rpl->name_or_class);
    if ( sc==NULL ) {
	int len=0;
	char *space;
	for ( g=glyphs; g!=NULL && g->mark_count==glyphs->mark_count; g=g->next )
	    len += strlen(g->name_or_class)+1;
	space = galloc(len+1);
	sofar = fea_AddAllLigPosibilities(tok,glyphs,sc,space,space,sofar);
	free(space);
    }
return( sofar );
}

static FPST *fea_markedglyphs_to_fpst(struct parseState *tok,struct markedglyphs *glyphs,
	int is_pos) {
    struct markedglyphs *g;
    int bcnt=0, ncnt=0, fcnt=0, cnt;
    int all_single=true;
    int mmax = 0;
    int i;
    FPST *fpst;
    struct fpst_rule *r;
    struct feat_item *item, *head;

    for ( g=glyphs; g!=NULL && g->mark_count!=0; g=g->next ) {
	++bcnt;
	if ( !g->is_name ) all_single = false;
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
	    if ( g->mark_count>mmax ) mmax = g->mark_count;
	}
    }

    fpst = chunkalloc(sizeof(FPST));
    fpst->type = is_pos ? pst_chainpos : pst_chainsub;
    fpst->format = all_single ? pst_glyphs : pst_coverage;
    fpst->rule_cnt = 1;
    fpst->rules = r = gcalloc(1,sizeof(struct fpst_rule));
    r->lookup_cnt = mmax;
    r->lookups = gcalloc(mmax,sizeof(struct seqlookup));
    for ( i=0; i<mmax; ++i )
	r->lookups[i].seq = i;

    if ( all_single ) {
	g = fea_glyphs_to_names(glyphs,bcnt,&r->u.glyph.back);
	g = fea_glyphs_to_names(g,ncnt,&r->u.glyph.names);
	g = fea_glyphs_to_names(g,fcnt,&r->u.glyph.fore);
    } else {
	r->u.coverage.ncnt = ncnt;
	r->u.coverage.bcnt = bcnt;
	r->u.coverage.fcnt = fcnt;
	r->u.coverage.ncovers = galloc(ncnt*sizeof(char*));
	r->u.coverage.bcovers = galloc(bcnt*sizeof(char*));
	r->u.coverage.fcovers = galloc(fcnt*sizeof(char*));
	for ( i=0, g=glyphs; i<bcnt; ++i, g=g->next )
	    r->u.coverage.bcovers[i] = copy(g->name_or_class);
	for ( i=0; i<ncnt; ++i, g=g->next )
	    r->u.coverage.ncovers[i] = copy(g->name_or_class);
	for ( i=0; i<fcnt; ++i, g=g->next )
	    r->u.coverage.fcovers[i] = copy(g->name_or_class);
    }

    item = chunkalloc(sizeof(struct feat_item));
    item->type = ft_fpst;
    item->next = tok->sofar;
    tok->sofar = item;
    item->u2.fpst = fpst;

    if ( is_pos ) {
	for ( g=glyphs; g!=NULL && g->mark_count!=0; g=g->next );
	for ( i=0; g!=NULL; ++i ) {
	    if ( g->lookupname!=NULL ) {
		head = chunkalloc(sizeof(struct feat_item));
		head->type = ft_lookup_ref;
		head->u1.lookup_name = copy(g->lookupname);
	    } else if ( (g->next==NULL || g->next->mark_count!=g->mark_count) &&
		    g->vr!=NULL ) {
		head = fea_process_pos_single(tok,g,NULL);
	    } else if ( g->next!=NULL && g->mark_count==g->next->mark_count &&
		    (g->vr!=NULL || g->next->vr!=NULL ) &&
		    ( g->next->next==NULL || g->next->next->mark_count!=g->mark_count)) {
		head = fea_process_pos_pair(tok,g,NULL,false);
	    } else {
		LogError(_("Unparseable contetual sequence on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
	    }
	    r->lookups[i].lookup = (OTLookup *) head;
	    cnt = g->mark_count;
	    while ( g!=NULL && g->mark_count == cnt )	/* skip everything involved here */
		g=g->next;
	    for ( ; g!=NULL && g->mark_count!=0; g=g->next ); /* skip any uninvolved glyphs */
	}
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
    else {
	LogError(_("The ignore keyword must be followed by either position or substitute on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	is_pos = true;
    }
    forever {
	glyphs = fea_ParseMarkedGlyphs(tok,false/* don't parse value records, etc*/,
		true/*allow marks*/,false/* no lookups */);
	fpst = fea_markedglyphs_to_fpst(tok,glyphs,false);
	if ( is_pos )
	    fpst->type = pst_chainpos;
	fea_markedglyphsFree(glyphs);
	if ( tok->type!=tk_char || tok->tokbuf[0]!=',' )
    break;
	fea_ParseTok(tok);
    }
	    
    fea_end_statement(tok);
}    
    
static void fea_ParseSubstitute(struct parseState *tok) {
    /* name by name => single subs */
    /* class by name => single subs */
    /* class by class => single subs */
    /* name by <glyph sequence> => multiple subs */
    /* name from <class> => alternate subs */
    /* <glyph sequence> by name => ligature */
    /* <marked glyph sequence> by <name> => context chaining */
    /* <marked glyph sequence> by <lookup name>* => context chaining */
    /* [ignore sub] <marked glyph sequence> (, <marked g sequence>)* */
    struct markedglyphs *glyphs = fea_ParseMarkedGlyphs(tok,false,true,false),
	    *g, *rpl, *rp;
    int cnt, i;
    SplineChar *sc;
    struct feat_item *item, *head;

    for ( cnt=0, g=glyphs; g!=NULL; g=g->next, ++cnt );
    if ( glyphs==NULL ) {
	LogError(_("Empty subsitute on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    } else if ( !glyphs->has_marks ) {
	/* Non-contextual */
	if ( cnt==1 && glyphs->is_name && tok->type==tk_from ) {
	    /* Alternate subs */
	    char *alts;
	    fea_ParseTok(tok);
	    alts = fea_ParseGlyphClass(tok);
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
		    int len=0;
		    char *mult;
		    for ( g=rpl; g!=NULL; g=g->next )
			len += strlen(g->name_or_class)+1;
		    mult = galloc(len+1);
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
			item->next = tok->sofar;
			tok->sofar = item;
			item->u1.sc = sc;
			item->u2.pst = chunkalloc(sizeof(PST));
			item->u2.pst->type = pst_multiple;
			item->u2.pst->u.mult.components = mult;
		    }
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
	FPST *fpst = fea_markedglyphs_to_fpst(tok,glyphs,false);
	struct fpst_rule *r = fpst->rules;
	if ( tok->type!=tk_by ) {
	    LogError(_("Expected 'by' keyword in substitution on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	}
	fea_ParseTok(tok);
	rpl = fea_ParseMarkedGlyphs(tok,false,false,true);
	for ( g=glyphs; g!=NULL && g->mark_count!=0; g=g->next );
	for ( i=0, rp=rpl; g!=NULL && rp!=NULL; ++i, rp=rp->next ) {
	    if ( rp->lookupname!=NULL ) {
		head = chunkalloc(sizeof(struct feat_item));
		head->type = ft_lookup_ref;
		head->u1.lookup_name = copy(rp->lookupname);
	    } else if ( g->next==NULL || g->next->mark_count!=g->mark_count ) {
		head = fea_process_sub_single(tok,g,rp,NULL);
	    } else if ( g->next!=NULL && g->mark_count==g->next->mark_count ) {
		head = fea_process_sub_ligature(tok,g,rpl,NULL);
	    } else {
		LogError(_("Unparseable contetual sequence on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
	    }
	    r->lookups[i].lookup = (OTLookup *) head;
	    cnt = g->mark_count;
	    while ( g!=NULL && g->mark_count == cnt )	/* skip everything involved here */
		g=g->next;
	    for ( ; g!=NULL && g->mark_count!=0; g=g->next ); /* skip any uninvolved glyphs */
	}
	
	fea_markedglyphsFree(rpl);
    }
		    
    fea_end_statement(tok);
    fea_markedglyphsFree(glyphs);
}

static void fea_ParseMarks(struct parseState *tok) {
    /* mark name|class <anchor> */
    char *contents = NULL;
    SplineChar *sc = NULL;
    AnchorPoint *ap;
    char *start, *pt;
    int ch;

    fea_end_statement(tok);
    if ( tok->type==tk_name )
	sc = fea_glyphname_get(tok,tok->tokbuf);
    else if ( tok->type==tk_class )
	contents = fea_lookup_class_complain(tok,tok->tokbuf);
    else if ( tok->type==tk_char && tok->tokbuf[0]=='[' )
	contents = fea_ParseGlyphClass(tok);
    else {
	LogError(_("Expected glyph name or class in mark statement on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	fea_skip_to_semi(tok);
return;
    }
    if ( sc==NULL && contents==NULL ) {
	fea_skip_to_semi(tok);
return;
    }

    fea_TokenMustBe(tok,tk_char,'<');
    fea_TokenMustBe(tok,tk_anchor,'\0');
    ap = fea_ParseAnchor(tok);
    ap->type = at_mark;
    fea_end_statement(tok);

    if ( ap!=NULL ) {
	pt = contents;
	forever {
	    struct feat_item *item = chunkalloc(sizeof(struct feat_item));
	    item->type = ft_ap;
	    item->u2.ap = ap;
	    item->next = tok->sofar;
	    tok->sofar = item;
	    start = pt;
	    if ( contents==NULL ) {
		item->u1.sc = sc;
	break;
	    }
	    while ( *pt!='\0' && *pt!=' ' )
		++pt;
	    ch = *pt; *pt = '\0';
	    sc = fea_glyphname_get(tok,start);
	    *pt = ch;
	    while ( isspace(*pt)) ++pt;
	    if ( sc==NULL ) {
		tok->sofar = item->next;	/* Oops, remove it */
		chunkfree(item,sizeof(*item));
		if ( *pt=='\0' ) {
		    AnchorPointsFree(ap);
	break;
		}
	    } else {
		item->u1.sc = sc;
		if ( *pt=='\0' )
	break;
		ap = AnchorPointsCopy(ap);
	    }
	}
    }
    free(contents);
}

static void fea_ParsePosition(struct parseState *tok, int enumer) {
    /* name <vr> => single pos */
    /* class <vr> => single pos */
    /* name|class <vr> name|class <vr> => pair pos */
    /* name|class name|class <vr> => pair pos */
    /* cursive name|class <anchor> <anchor> => cursive positioning */
    /* name|class <anchor> mark name|class => mark to base pos */
	/* Must be preceded by a mark statement */
    /* name|class <anchor> <anchor>+ mark name|class => mark to ligature pos */
	/* Must be preceded by a mark statement */
    /* mark name|class <anchor> mark name|class => mark to base pos */
	/* Must be preceded by a mark statement */
    /* <marked glyph pos sequence> => context chaining */
    /* [ignore pos] <marked glyph sequence> (, <marked g sequence>)* */
    struct markedglyphs *glyphs = fea_ParseMarkedGlyphs(tok,true,true,false), *g;
    int cnt, i;
    struct feat_item *item;
    char *start, *pt, ch;
    SplineChar *sc;

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
	    } else {
		start = glyphs->name_or_class;
		if ( glyphs->anchors[1]!=NULL )
		    glyphs->anchors[1]->type = at_cexit;
		forever {
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
			item->next = tok->sofar;
			tok->sofar = item;
			item->u1.sc = sc;
			if ( glyphs->anchors[0]!=NULL ) {
			    glyphs->anchors[0]->type = at_centry;
			    glyphs->anchors[0]->next = glyphs->anchors[1];
			    item->u2.ap = AnchorPointsCopy(glyphs->anchors[0]);
			} else
			    item->u2.ap = AnchorPointsCopy(glyphs->anchors[1]);
		    }
		}
	    }
	} else if ( cnt==1 && glyphs->vr!=NULL ) {
	    tok->sofar = fea_process_pos_single(tok,glyphs,tok->sofar);
	} else if ( cnt==2 && (glyphs->vr!=NULL || glyphs->next->vr!=NULL) ) {
	    tok->sofar = fea_process_pos_pair(tok,glyphs,tok->sofar, enumer);
	} else if ( cnt==1 && glyphs->ap_cnt>=1 && tok->type == tk_mark ) {
	    /* Mark to base, mark to mark, mark to ligature */
	    char *mark_class;
	    AnchorPoint *head=NULL, *last=NULL;
	    if ( tok->type!=tk_mark ) {
		LogError(_("A mark glyph (or class of marks) must be specified here on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
	    }
	    fea_ParseTok(tok);
	    if ( tok->type==tk_name )
		mark_class = copy(tok->tokbuf);
	    else
		mark_class = fea_ParseGlyphClass(tok);
	    if ( glyphs->is_mark && glyphs->ap_cnt>1 ) {
		LogError(_("Mark to base anchor statements may only have one anchor on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
	    }
	    if ( mark_class!=NULL ) {
		for ( i=0; i<glyphs->ap_cnt; ++i ) {
		    if ( glyphs->anchors[i]==NULL )
			/* Nothing to be done */;
		    else {
			if ( glyphs->ap_cnt>1 ) {
			    glyphs->anchors[i]->type = at_baselig;
			    glyphs->anchors[i]->lig_index = i;
			} else if ( glyphs->is_mark )
			    glyphs->anchors[i]->type = at_basemark;
			else
			    glyphs->anchors[i]->type = at_basechar;
			if ( head==NULL )
			    head = glyphs->anchors[i];
			else
			    last->next = glyphs->anchors[i];
			last = glyphs->anchors[i];
		    }
		}

		start = glyphs->name_or_class;
		forever {
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
			item->next = tok->sofar;
			tok->sofar = item;
			item->u1.sc = sc;
			item->u2.ap = AnchorPointsCopy(head);
			item->mark_class = copy(mark_class);
		    }
		}

		/* So we can free them properly */
		for ( ; head!=NULL; head = last ) {
		    last = head->next;
		    head->next = NULL;
		}
	    }
	} else {
	    LogError(_("Unparseable glyph sequence in position on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	}
    } else {
	/* Contextual */
	(void) fea_markedglyphs_to_fpst(tok,glyphs,true);
    }
    fea_end_statement(tok);
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
	  default:
return( ot_undef );		/* Can't happen */
	}
      break;
      default:
return( ot_undef );
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
      case tk_lookupflag:
	fea_ParseLookupFlags(tok);
      break;
      case tk_mark:
	fea_ParseMarks(tok);
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
      case tk_substitute:
	fea_ParseSubstitute(tok);
	enumer = false;
      break;
      case tk_subtable:
	fea_AddFeatItem(tok,ft_subtable,0);
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
    struct feat_item *item;
    enum otlookup_type lookuptype;
    int ret;

    fea_end_statement(tok);
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

    forever {
	fea_ParseTok(tok);
	if ( tok->err_count>100 )
    break;
	if ( tok->type==tk_eof ) {
	    LogError(_("Unexpected end of file in lookup definition on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
return;
	} else if ( (ret = fea_LookupSwitch(tok))==0 ) {
	    LogError(_("Unexpected token, %s, in lookup definition on line %d of %s"), tok->tokbuf, tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
return;
	} else if ( ret==2 )
    break;
    }
    fea_ParseTok(tok);
    if ( tok->type!=tk_name || strcmp(tok->tokbuf,lookup_name)==0 ) {
	LogError(_("Expected %s in lookup definition on line %d of %s"),
		lookup_name, tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    }
    fea_end_statement(tok);

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

static void fea_ParseFeatureDef(struct parseState *tok) {
    uint32 feat_tag;
    struct feat_item *item;
    int type, ret;

    fea_ParseTok(tok);
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
	item->u2.sl = SLCopy(tok->def_langsyses);
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

    forever {
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
			forever {
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
	      default:
		LogError(_("Unexpected token, %s, in feature definition on line %d of %s"), tok->tokbuf, tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
return;
	    }
	} else if ( ret==2 )
    break;
    }
    
    fea_ParseTok(tok);
    if ( tok->type!=tk_name || !tok->could_be_tag || tok->tag!=feat_tag ) {
	LogError(_("Expected '%c%c%c%c' in lookup definition on line %d of %s"),
		feat_tag>>24, feat_tag>>16, feat_tag>>8, feat_tag,
		tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    }
    fea_end_statement(tok);

    item = chunkalloc(sizeof(struct feat_item));
    item->type = ft_feat_end;
    item->u1.tag = feat_tag;
    item->next = tok->sofar;
    tok->sofar = item;

    tok->in_vkrn = false;
}

static void fea_ParseTableDef(struct parseState *tok) {
    uint32 table_tag;
    struct feat_item *item;

    fea_ParseTok(tok);
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
	/* GlyphClassDef <base> <lig> <mark> <component>; */
	/* Attach <glyph>|<glyph class> <number>+; */	/* parse & ignore */
	/* LigatureCaret <glyph>|<glyph class> <caret value>+ */
      break;
      case CHR('h','e','a','d'):
	/* FontRevision <number>.<number>; */
      break;
      case CHR('h','h','e','a'):
	/* CaretOffset <number>; */
	/* Ascender <number>; */
	/* Descender <number>; */
	/* LineGap <number>; */
      break;
      case CHR('v','h','e','a'):
	/* VertTypoAscender <number>; */
	/* VertTypoDescender <number>; */
	/* VertTypoLineGap <number>; */
      break;
      case CHR('n','a','m','e'):
	/* nameid <id> [<string attibute>] string;
	/* <string attribute> := <platform> | <platform> <specific> <language> */
	/* <patform>==3 => <specific>=1 <language>=0x409 */
	/* <platform>==1 => <specific>=0 <lang>=0 */
	/* string in double quotes \XXXX escapes to UCS2 (for 3) */
	/* string in double quotes \XX escapes to MacLatin (for 1) */
      break;
      case CHR('O','S','/','2'):
	/* FSType <number> */
	/* Panose 10*<number> */
	/* UnicodeRange n*<number> */
	/* CodePageRange n*<number> */
	/* TypoAscender <number> */
	/* TypoDescender <number> */
	/* TypoLineGap <number> */
	/* winAscent <number> */
	/* winDescent <number> */
	/* XHeight <number> */
	/* CapHeight <number> */
	/* WeightClass <number> */
	/* WidthClass <number> */
	/* Vendor <tag-string> */
      break;
      case CHR('v','m','t','x'):
	/* I don't support 'vmtx' tables */
      case CHR('B','A','S','E'):
	/* I don't support 'BASE' tables */
      default:
	fea_skip_to_close_curly(tok);
      break;
    }
}

void SFApplyFeatureFile(SplineFont *sf,FILE *file,char *filename) {
    struct parseState tok;

    memset(&tok,0,sizeof(tok));
    tok.line[0] = 1;
    tok.filename[0] = filename;
    tok.inlist[0] = file;
    if ( sf->cidmaster ) sf = sf->cidmaster;
    tok.sf = sf;

    forever {
	fea_ParseTok(&tok);
	if ( tok.err_count>100 )
    break;
	switch ( tok.type ) {
	  case tk_class:
	    fea_ParseGlyphClassDef(&tok);
	  break;
	  case tk_lookup:
	    fea_ParseLookupDef(&tok,false);
	  break;
	  case tk_languagesystem:
	    fea_ParseLangSys(&tok,false);
	  break;
	  case tk_feature:
	    fea_ParseFeatureDef(&tok);
	  break;
	  case tk_table:
	    fea_ParseTableDef(&tok);
	  break;
	  case tk_anonymous:
	    LogError(_("FontForge does not support anonymous tables on line %d of %s"), tok.line[tok.inc_depth], tok.filename[tok.inc_depth] );
	    fea_skip_to_close_curly(&tok);
	  break;
	  case tk_eof:
  goto end_loop;
	  default:
	    LogError(_("Unexpected token, %s, on line %d of %s"), tok.tokbuf, tok.line[tok.inc_depth], tok.filename[tok.inc_depth] );
  goto end_loop;
      }
    }
  end_loop:
    /* !!!!! Free the default langsystems */;
    /* Free the glyph classes */;
}

void SFApplyFeatureFilename(SplineFont *sf,char *filename) {
    FILE *in = fopen(filename,"r");

    if ( in==NULL ) {
	gwwv_post_error(_("Cannot open file"),_("Cannot open feature file %.120s"), filename );
return;
    }
    SFApplyFeatureFile(sf,in,filename);
    fclose(in);
}
