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
#include "pfaedit.h"
#include "ttf.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <utype.h>
#include <ustring.h>

/* Adobe's opentype feature file */

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
	    }
#if 0
	    else if ( otl->lookup_type==gpos_mark2ligature ) {
		AnchorPoint **aps;
		int cnt;
		aps = apfind_ligatures(sf,start,otl, &cnt);
		for ( i=0; i<cnt; ++i ) {
		    dump_anchorpoint(out,aps[i]);
		    putc(' ',out);
		}
		free(aps);
		fprintf( out, "mark " );
		/* But there is no way to specify the mark's anchor */
	    } else
#endif
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
	    }
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
	    if ( otl->lookup_type==gpos_single ) {
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
	    if ( otl->lookup_type==gsub_single ) {
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
	    } else if ( otl->lookup_type==gsub_ligature ) {
		/* I don't think it is possible to do this correctly */
		/* adobe doesn't say how to order glyphs for [f longs]' [i l]' */
		/* so just assume there is only one ligature */
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
	    if ( otl->lookup_type==gpos_single ) {
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
	    if ( otl->lookup_type==gsub_single ) {
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

static void dump_lookup(FILE *out, SplineFont *sf, OTLookup *otl) {
    struct lookup_subtable *sub;
    static char *flagnames[] = { "RightToLeft", "IgnoreBaseGlyphs", "IgnoreLigatures", "IgnoreMarks", NULL };
    int i, k, first;
    SplineFont *_sf;
    SplineChar *sc;
    PST *pst;
    int isv;
    KernPair *kp;

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

void OTFFDumpOneLookup(FILE *out,SplineFont *sf, OTLookup *otl) {
    FeatureScriptLangList *fl;
    struct scriptlanglist *sl;
    int l;

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

void OTFFDumpFontLookups(FILE *out,SplineFont *sf) {

    if ( sf->cidmaster!=NULL ) sf=sf->cidmaster;
    preparenames(sf);
    dump_gsubgpos(out,sf);
    dump_gdef(out,sf);
    cleanupnames(sf);
}

	
    
