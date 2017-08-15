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

#include "fvfonts.h"

#include "encoding.h"
#include "fontforgevw.h"
#include "lookups.h"
#include "namelist.h"
#include "splinefill.h"
#include "splineorder2.h"
#include "splinesaveafm.h"
#include "splineutil.h"
#include "splineutil2.h"
#include "tottfgpos.h"
#include "ustring.h"
#include "utype.h"
#include "gfile.h"
#include "chardata.h"
#include "namehash.h"

RefChar *RefCharsCopy(RefChar *ref) {
    RefChar *rhead=NULL, *last=NULL, *cur;

    while ( ref!=NULL ) {
	cur = RefCharCreate();
	{ struct reflayer *layers = cur->layers; int layer;
	layers = realloc(layers,ref->layer_cnt*sizeof(struct reflayer));
	memcpy(layers,ref->layers,ref->layer_cnt*sizeof(struct reflayer));
	*cur = *ref;
	cur->layers = layers;
	for ( layer=0; layer<cur->layer_cnt; ++layer ) {
	    cur->layers[layer].splines = NULL;
	    cur->layers[layer].images = NULL;
	}
	}
	if ( cur->sc!=NULL )
	    cur->orig_pos = cur->sc->orig_pos;
	cur->next = NULL;
	if ( rhead==NULL )
	    rhead = cur;
	else
	    last->next = cur;
	last = cur;
	ref = ref->next;
    }
return( rhead );
}

static OTLookup *MCConvertLookup(struct sfmergecontext *mc,OTLookup *otl) {
    int l;
    OTLookup *newotl;

    if ( mc==NULL || mc->sf_from==mc->sf_to )
return( otl );		/* No translation needed */

    for ( l=0; l<mc->lcnt; ++l ) {
	if ( mc->lks[l].from == otl )
    break;
    }
    if ( l==mc->lcnt )
return( NULL );
    if ( mc->lks[l].to!=NULL )
return( mc->lks[l].to );

    mc->lks[l].to = newotl = chunkalloc(sizeof(OTLookup));
    newotl->lookup_name = strconcat(mc->prefix,otl->lookup_name);
    newotl->lookup_type = otl->lookup_type;
    newotl->lookup_flags = otl->lookup_flags;
    newotl->features = FeatureListCopy(otl->features);
    newotl->store_in_afm = otl->store_in_afm;
return( newotl );
}

struct lookup_subtable *MCConvertSubtable(struct sfmergecontext *mc,struct lookup_subtable *sub) {
    int s;
    struct lookup_subtable *newsub;

    if ( mc==NULL || mc->sf_from==mc->sf_to )
return( sub );		/* No translation needed */
    if ( mc->prefix==NULL ) {
	int lcnt, scnt;
	OTLookup *otl;
	struct lookup_subtable *subs;
	int isgpos, doit;
	char *temp;

	/* Not initialized */
	if ( mc->sf_from->cidmaster ) mc->sf_from = mc->sf_from->cidmaster;
	else if ( mc->sf_from->mm!=NULL ) mc->sf_from = mc->sf_from->mm->normal;
	if ( mc->sf_to->cidmaster ) mc->sf_to = mc->sf_to->cidmaster;
	else if ( mc->sf_to->mm!=NULL ) mc->sf_to = mc->sf_to->mm->normal;
	if ( mc->sf_from == mc->sf_to )
return( sub );
	mc->prefix = strconcat(mc->sf_from->fontname,"-");
	for ( doit = 0; doit<2; ++doit ) {
	    lcnt = scnt = 0;
	    for ( isgpos=0; isgpos<2; ++isgpos ) {
		for ( otl = isgpos ? mc->sf_from->gpos_lookups : mc->sf_from->gsub_lookups; otl!=NULL; otl=otl->next ) {
		    if ( doit ) {
			mc->lks[lcnt].from = otl;
			temp = strconcat(mc->prefix,otl->lookup_name);
			mc->lks[lcnt].to = SFFindLookup(mc->sf_to,temp);
			free(temp);
			mc->lks[lcnt].old = mc->lks[lcnt].to!=NULL;
		    }
		    ++lcnt;
		    for ( subs=otl->subtables; subs!=NULL; subs=subs->next ) {
			if ( doit ) {
			    mc->subs[scnt].from = subs;
			    temp = strconcat(mc->prefix,subs->subtable_name);
			    mc->subs[scnt].to = SFFindLookupSubtable(mc->sf_to,temp);
			    free(temp);
			    mc->subs[scnt].old = mc->subs[scnt].to!=NULL;
			}
			++scnt;
		    }
		}
	    }
	    if ( !doit ) {
		mc->lcnt = lcnt; mc->scnt = scnt;
		mc->lks = calloc(lcnt,sizeof(struct lookup_cvt));
		mc->subs = calloc(scnt,sizeof(struct sub_cvt));
	    }
	}
    }

    for ( s=0; s<mc->scnt; ++s ) {
	if ( mc->subs[s].from == sub )
    break;
    }
    if ( s==mc->scnt )
return( NULL );
    if ( mc->subs[s].to!=NULL )
return( mc->subs[s].to );

    mc->subs[s].to = newsub = chunkalloc(sizeof(struct lookup_subtable));
    newsub->subtable_name = strconcat(mc->prefix,sub->subtable_name);
    newsub->lookup = MCConvertLookup(mc,sub->lookup);
    newsub->anchor_classes = sub->anchor_classes;
    newsub->per_glyph_pst_or_kern = sub->per_glyph_pst_or_kern;
    newsub->separation = sub->separation;
    newsub->minkern = sub->minkern;
return( newsub );
}

AnchorClass *MCConvertAnchorClass(struct sfmergecontext *mc,AnchorClass *ac) {
    int a;
    AnchorClass *newac;

    if ( mc==NULL || mc->sf_from==mc->sf_to )
return( ac );		/* No translation needed */
    if ( mc->acnt==0 ) {
	int acnt;
	AnchorClass *testac, *testac2;
	int doit;
	char *temp;

	/* Not initialized */
	for ( doit = 0; doit<2; ++doit ) {
	    acnt = 0;
	    for ( testac=mc->sf_from->anchor; testac!=NULL; testac=testac->next ) {
		if ( doit ) {
		    mc->acs[acnt].from = testac;
		    temp = strconcat(mc->prefix,testac->name);
		    for ( testac2 = mc->sf_to->anchor; testac2!=NULL; testac2=testac2->next )
			if ( strcmp(testac2->name,temp)==0 )
		    break;
		    mc->acs[acnt].to = testac2;
		    free(temp);
		    mc->acs[acnt].old = mc->acs[acnt].to!=NULL;
		}
		++acnt;
	    }
	    if ( !doit ) {
		mc->acnt = acnt;
		mc->acs = calloc(acnt,sizeof(struct ac_cvt));
	    }
	}
    }

    for ( a=0; a<mc->acnt; ++a ) {
	if ( mc->acs[a].from == ac )
    break;
    }
    if ( a==mc->acnt )
return( NULL );
    if ( mc->acs[a].to!=NULL )
return( mc->acs[a].to );

    mc->acs[a].to = newac = chunkalloc(sizeof(AnchorClass));
    newac->name = strconcat(mc->prefix,ac->name);
    newac->subtable = ac->subtable ? MCConvertSubtable(mc,ac->subtable) : NULL;
    newac->next = mc->sf_to->anchor;
    mc->sf_to->anchor = newac;
return( newac );
}

void SFFinishMergeContext(struct sfmergecontext *mc) {
    int l,s,slast, isgpos;
    OTLookup *otl, *last;
    struct lookup_subtable *sub_last;

    if ( mc->prefix==NULL )		/* Nothing to do, never initialized, no lookups needed */
return;

    /* Get all the subtables in the right order */
    for ( s=0; s<mc->scnt; s=slast ) {
	if ( mc->subs[s].to==NULL ) {
	    slast = s+1;
    continue;
	}
	otl = mc->subs[s].to->lookup;
	sub_last = otl->subtables = mc->subs[s].to;
	for ( slast=s+1; slast<mc->scnt; ++slast ) {
	    if ( mc->subs[slast].to==NULL )
	continue;
	    if ( mc->subs[slast].to->lookup!=otl )
	break;
	    sub_last->next = mc->subs[slast].to;
	    sub_last = sub_last->next;
	}
	sub_last->next = NULL;
    }

    /* Then order the lookups. Leave the old lookups as they are, insert the */
    /*  new at the end of the list */
    last = NULL;
    for ( l=0; l<mc->lcnt; ++l ) {
	if ( mc->lks[l].to==NULL || mc->lks[l].old )
    continue;
	otl = mc->lks[l].to;
	isgpos = (otl->lookup_type>=gpos_start);
	if ( last==NULL || (last->lookup_type>=gpos_start)!=isgpos) {
	    last = isgpos ? mc->sf_to->gpos_lookups : mc->sf_to->gsub_lookups;
	    if ( last!=NULL )
		while ( last->next!=NULL )
		    last = last->next;
	}
	if ( last!=NULL )
	    last->next = otl;
	else if ( isgpos )
	    mc->sf_to->gpos_lookups = otl;
	else
	    mc->sf_to->gsub_lookups = otl;
	last = otl;
    }

    free(mc->prefix);
    free(mc->lks);
    free(mc->subs);
    free(mc->acs);
}

PST *PSTCopy(PST *base,SplineChar *sc,struct sfmergecontext *mc) {
    PST *head=NULL, *last=NULL, *cur;

    for ( ; base!=NULL; base = base->next ) {
	cur = chunkalloc(sizeof(PST));
	*cur = *base;
	cur->subtable = MCConvertSubtable(mc,base->subtable);
	if ( cur->type==pst_ligature ) {
	    cur->u.lig.components = copy(cur->u.lig.components);
	    cur->u.lig.lig = sc;
	} else if ( cur->type==pst_pair ) {
	    cur->u.pair.paired = copy(cur->u.pair.paired);
	    cur->u.pair.vr = chunkalloc(sizeof( struct vr [2]));
	    memcpy(cur->u.pair.vr,base->u.pair.vr,sizeof(struct vr [2]));
	    cur->u.pair.vr[0].adjust = ValDevTabCopy(base->u.pair.vr[0].adjust);
	    cur->u.pair.vr[1].adjust = ValDevTabCopy(base->u.pair.vr[1].adjust);
	} else if ( cur->type==pst_lcaret ) {
	    cur->u.lcaret.carets = malloc(cur->u.lcaret.cnt*sizeof(uint16));
	    memcpy(cur->u.lcaret.carets,base->u.lcaret.carets,cur->u.lcaret.cnt*sizeof(uint16));
	} else if ( cur->type==pst_substitution || cur->type==pst_multiple || cur->type==pst_alternate )
	    cur->u.subs.variant = copy(cur->u.subs.variant);
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;
    }
return( head );
}

static AnchorPoint *AnchorPointsDuplicate(AnchorPoint *base,SplineChar *sc) {
    AnchorPoint *head=NULL, *last=NULL, *cur;
    AnchorClass *ac;

    for ( ; base!=NULL; base = base->next ) {
	cur = chunkalloc(sizeof(AnchorPoint));
	*cur = *base;
	cur->next = NULL;
	for ( ac=sc->parent->anchor; ac!=NULL; ac=ac->next )
	    if ( strcmp(ac->name,base->anchor->name)==0 )
	break;
	cur->anchor = ac;
	if ( ac==NULL ) {
	    LogError(_("No matching AnchorClass for %s"), base->anchor->name);
	    chunkfree(cur,sizeof(AnchorPoint));
	} else {
	    if ( head==NULL )
		head = cur;
	    else
		last->next = cur;
	    last = cur;
	}
    }
return( head );
}

static void AnchorClassesAdd(SplineFont *into, SplineFont *from, struct sfmergecontext *mc) {
    AnchorClass *fac, *iac, *last, *cur;

    if ( into->cidmaster!=NULL )
	into = into->cidmaster;
    if ( from->cidmaster!=NULL )
	from = from->cidmaster;

    for ( fac = from->anchor; fac!=NULL; fac=fac->next ) {
	last = NULL;
	for ( iac=into->anchor; iac!=NULL; iac=iac->next ) {
	    if ( strcmp(iac->name,fac->name)==0 )
	break;
	    last = iac;
	}
	if ( iac==NULL ) {
	    cur = chunkalloc(sizeof(AnchorClass));
	    *cur = *fac;
	    cur->next = NULL;
	    cur->name = copy(cur->name);
	    cur->subtable = cur->subtable ? MCConvertSubtable(mc,cur->subtable) : NULL;
	    if ( last==NULL )
		into->anchor = cur;
	    else
		last->next = cur;
	} else {
	    /* Just use the one that's there and hope it's right. Not sure */
	    /*  what else we can do... */
	}
    }
}

static void FPSTsAdd(SplineFont *into, SplineFont *from,struct sfmergecontext *mc) {
    FPST *fpst, *nfpst, *last;
    int i,k;

    last = NULL;
    if ( into->possub!=NULL )
	for ( last = into->possub; last->next!=NULL; last=last->next );
    for ( fpst = from->possub; fpst!=NULL; fpst=fpst->next ) {
	nfpst = FPSTCopy(fpst);
	nfpst->subtable = MCConvertSubtable(mc,fpst->subtable);
	nfpst->subtable->fpst = nfpst;
	for ( i=0; i<nfpst->rule_cnt; ++i ) {
	    struct fpst_rule *r = &nfpst->rules[i], *oldr = &fpst->rules[i];
	    for ( k=0; k<r->lookup_cnt; ++k ) {
		r->lookups[k].lookup = MCConvertLookup(mc,oldr->lookups[k].lookup);
	    }
	}
	if ( last==NULL )
	    into->possub = nfpst;
	else
	    last->next = nfpst;
	last = nfpst;
    }
}

static void ASMsAdd(SplineFont *into, SplineFont *from,struct sfmergecontext *mc) {
    ASM *sm, *nsm, *last;
    int i;

    last = NULL;
    if ( into->sm!=NULL )
	for ( last = into->sm; last->next!=NULL; last=last->next );
    for ( sm = from->sm; sm!=NULL; sm=sm->next ) {
	nsm = chunkalloc(sizeof(ASM));
	*nsm = *sm;
	nsm->subtable = MCConvertSubtable(mc,sm->subtable);
	nsm->subtable->sm = nsm;
	nsm->classes = malloc(nsm->class_cnt*sizeof(char *));
	for ( i=0; i<nsm->class_cnt; ++i )
	    nsm->classes[i] = copy(sm->classes[i]);
	nsm->state = malloc(nsm->class_cnt*nsm->state_cnt*sizeof(struct asm_state));
	memcpy(nsm->state,sm->state,nsm->class_cnt*nsm->state_cnt*sizeof(struct asm_state));
	if ( nsm->type == asm_kern ) {
	    for ( i=nsm->class_cnt*nsm->state_cnt-1; i>=0; --i ) {
		nsm->state[i].u.kern.kerns = malloc(nsm->state[i].u.kern.kcnt*sizeof(int16));
		memcpy(nsm->state[i].u.kern.kerns,sm->state[i].u.kern.kerns,nsm->state[i].u.kern.kcnt*sizeof(int16));
	    }
	} else if ( nsm->type == asm_context ) {
	    for ( i=0; i<nsm->class_cnt*nsm->state_cnt; ++i ) {
		nsm->state[i].u.context.mark_lookup = MCConvertLookup(mc,
			nsm->state[i].u.context.mark_lookup);
		nsm->state[i].u.context.cur_lookup = MCConvertLookup(mc,
			nsm->state[i].u.context.cur_lookup);
	    }
	} else if ( nsm->type == asm_insert ) {
	    for ( i=nsm->class_cnt*nsm->state_cnt-1; i>=0; --i ) {
		nsm->state[i].u.insert.mark_ins = copy(sm->state[i].u.insert.mark_ins);
		nsm->state[i].u.insert.cur_ins = copy(sm->state[i].u.insert.cur_ins);
	    }
	}
    }
}

static KernClass *_KernClassCopy(KernClass *kc,SplineFont *into,SplineFont *from,struct sfmergecontext *mc) {
    KernClass *nkc;

    nkc = KernClassCopy(kc);
    nkc->subtable = MCConvertSubtable(mc,kc->subtable);
    nkc->subtable->kc = nkc;
return( nkc );
}

static void KernClassesAdd(SplineFont *into, SplineFont *from,struct sfmergecontext *mc) {
    /* Is this a good idea? We could end up with two kern classes that do */
    /*  the same thing and strange sets of slis so that they didn't all fit */
    /*  in one lookup... */
    KernClass *kc, *last, *cur;

    last = NULL;
    if ( into->kerns!=NULL )
	for ( last = into->kerns; last->next!=NULL; last=last->next );
    for ( kc=from->kerns; kc!=NULL; kc=kc->next ) {
	cur = _KernClassCopy(kc,into,from,mc);
	if ( last==NULL )
	    into->kerns = cur;
	else
	    last->next = cur;
	last = cur;
    }

    last = NULL;
    if ( into->vkerns!=NULL )
	for ( last = into->vkerns; last->next!=NULL; last=last->next )
    for ( kc=from->vkerns; kc!=NULL; kc=kc->next ) {
	cur = _KernClassCopy(kc,into,from,mc);
	if ( last==NULL )
	    into->vkerns = cur;
	else
	    last->next = cur;
	last = cur;
    }
}

struct altuni *AltUniCopy(struct altuni *altuni,SplineFont *noconflicts) {
    struct altuni *head=NULL, *last=NULL, *cur;

    while ( altuni!=NULL ) {
	if ( noconflicts==NULL || SFGetChar(noconflicts,altuni->unienc,NULL)==NULL ) {
	    cur = chunkalloc(sizeof(struct altuni));
	    cur->unienc = altuni->unienc;
	    cur->vs = altuni->vs;
	    cur->fid = altuni->fid;
	    if ( head==NULL )
		head = cur;
	    else
		last->next = cur;
	    last = cur;
	}
	altuni = altuni->next;
    }
return( head );
}

SplineChar *SplineCharCopy(SplineChar *sc,SplineFont *into,struct sfmergecontext *mc) {
    SplineChar *nsc = SFSplineCharCreate(into);
    Layer *layers = nsc->layers;
    int layer, lycopy;

	*nsc = *sc;

	/* Copy the instrs from the given sc to the new splinechar */
	if ( sc->ttf_instrs_len!=0 ) {
		nsc->ttf_instrs = malloc(sc->ttf_instrs_len);
		memcpy(nsc->ttf_instrs,sc->ttf_instrs,sc->ttf_instrs_len);
	}

	/* We copy the layers just below */
    nsc->layer_cnt = into==NULL?2:into->layer_cnt;
    nsc->layers = layers;
    lycopy = sc->layer_cnt>nsc->layer_cnt ? nsc->layer_cnt : sc->layer_cnt;
    memcpy(layers,sc->layers,lycopy*sizeof(Layer));
    if ( nsc->layer_cnt>lycopy )
	memset(layers+lycopy,0,(nsc->layer_cnt-lycopy)*sizeof(Layer));
    for ( layer = ly_back; layer<lycopy; ++layer ) {
	layers[layer].splines = SplinePointListCopy(layers[layer].splines);
	layers[layer].refs = RefCharsCopy(layers[layer].refs);
	layers[layer].images = ImageListCopy(layers[layer].images);
	layers[layer].undoes = NULL;
	layers[layer].redoes = NULL;
	if ( into!=NULL ) {
	    if ( into->layers[layer].order2!=sc->layers[layer].order2 ) {
		nsc->layers[layer].order2 = into->layers[layer].order2;
		if ( into->layers[layer].order2 ) {
		    SCConvertLayerToOrder2(nsc,layer);
		} else {
		    SCConvertLayerToOrder3(nsc,layer);
		}
	    }
	} else {
	    /* Happens in Apple's gvar (MM) fonts */
	    nsc->layers[layer].order2 = sc->layers[layer].order2;
	}
    }
    nsc->parent = into;
    nsc->orig_pos = -2;
    nsc->name = copy(sc->name);
    nsc->hstem = StemInfoCopy(nsc->hstem);
    nsc->vstem = StemInfoCopy(nsc->vstem);
    nsc->dstem = DStemInfoCopy(nsc->dstem);
    nsc->md = NULL;
    if ( sc->countermask_cnt!=0 ) {
	nsc->countermasks = malloc(sc->countermask_cnt*sizeof(HintMask));
	memcpy(nsc->countermasks,sc->countermasks,sc->countermask_cnt*sizeof(HintMask));
    }
    nsc->anchor = AnchorPointsDuplicate(nsc->anchor,nsc);
    nsc->changed = true;
    /* Fix up dependents later when we know more */
    nsc->dependents = NULL;
    
    nsc->kerns = NULL;
    nsc->vkerns = NULL;
    nsc->possub = PSTCopy(nsc->possub,nsc,mc);
    nsc->altuni = AltUniCopy(nsc->altuni,into);
    nsc->charinfo = NULL;
    nsc->views = NULL;
return(nsc);
}

static int _SFFindExistingSlot(SplineFont *sf, int unienc, const char *name );

static KernPair *KernsCopy(KernPair *kp,int *mapping,SplineFont *into,
	struct sfmergecontext *mc) {
    KernPair *head = NULL, *last=NULL, *new;
    int index;

    while ( kp!=NULL ) {
	if ( (index=mapping[kp->sc->orig_pos])<0 && mc->preserveCrossFontKerning )
	    index =  _SFFindExistingSlot(into,kp->sc->unicodeenc,kp->sc->name);
	if ( index>=0 && index<into->glyphcnt &&
		into->glyphs[index]!=NULL ) {
	    new = chunkalloc(sizeof(KernPair));
	    new->off = kp->off;
	    new->subtable = MCConvertSubtable(mc,kp->subtable);
	    new->sc = into->glyphs[index];
	    if ( head==NULL )
		head = new;
	    else
		last->next = new;
	    last = new;
	}
	kp = kp->next;
    }
return( head );
}

BDFChar *BDFCharCopy(BDFChar *bc) {
    BDFChar *nbc = malloc(sizeof( BDFChar ));

    *nbc = *bc;
    nbc->views = NULL;
    nbc->selection = NULL;
    nbc->undoes = NULL;
    nbc->redoes = NULL;
    nbc->bitmap = malloc((nbc->ymax-nbc->ymin+1)*nbc->bytes_per_line);
    memcpy(nbc->bitmap,bc->bitmap,(nbc->ymax-nbc->ymin+1)*nbc->bytes_per_line);
return( nbc );
}

void BitmapsCopy(SplineFont *to, SplineFont *from, int to_index, int from_index ) {
    BDFFont *f_bdf, *t_bdf;

    /* We assume that the bitmaps are ordered. They should be */
    for ( t_bdf=to->bitmaps, f_bdf=from->bitmaps; t_bdf!=NULL && f_bdf!=NULL; ) {
	if ( t_bdf->pixelsize == f_bdf->pixelsize ) {
	    if ( f_bdf->glyphs[from_index]!=NULL ) {
		BDFCharFree(t_bdf->glyphs[to_index]);
		t_bdf->glyphs[to_index] = BDFCharCopy(f_bdf->glyphs[from_index]);
		t_bdf->glyphs[to_index]->sc = to->glyphs[to_index];
		t_bdf->glyphs[to_index]->orig_pos = to_index;
	    }
	    t_bdf = t_bdf->next;
	    f_bdf = f_bdf->next;
	} else if ( t_bdf->pixelsize < f_bdf->pixelsize ) {
	    t_bdf = t_bdf->next;
	} else if ( t_bdf->pixelsize > f_bdf->pixelsize ) {
	    /* Insert these bitmaps? Or skip? */
	    f_bdf = f_bdf->next;
	}
    }
}

void __GlyphHashFree(struct glyphnamehash *hash) {
    struct glyphnamebucket *test, *next;
    int i;

    if ( hash==NULL )
return;
    for ( i=0; i<GN_HSIZE; ++i ) {
	for ( test = hash->table[i]; test!=NULL; test = next ) {
	    next = test->next;
	    chunkfree(test,sizeof(struct glyphnamebucket));
	}
    }
}

static void _GlyphHashFree(SplineFont *sf) {

    if ( sf->glyphnames==NULL )
return;
    __GlyphHashFree(sf->glyphnames);
    free(sf->glyphnames);
    sf->glyphnames = NULL;
}

void GlyphHashFree(SplineFont *sf) {
    _GlyphHashFree(sf);
    if ( sf->cidmaster )
	_GlyphHashFree(sf->cidmaster);
}

static void GlyphHashCreate(SplineFont *sf) {
    int i, k, hash;
    SplineFont *_sf;
    struct glyphnamehash *gnh;
    struct glyphnamebucket *new;

    if ( sf->glyphnames!=NULL )
return;
    sf->glyphnames = gnh = calloc(1,sizeof(*gnh));
    k = 0;
    do {
	_sf = k<sf->subfontcnt ? sf->subfonts[k] : sf;
	/* I walk backwards because there are some ttf files where multiple */
	/*  glyphs get the same name. In the cases I've seen only one of these */
	/*  has an encoding. That's the one we want. It will be earlier in the */
	/*  font than the others. If we build the list backwards then it will */
	/*  be the top name in the bucket, and will be the one we return */
	for ( i=_sf->glyphcnt-1; i>=0; --i ) if ( _sf->glyphs[i]!=NULL ) {
	    new = chunkalloc(sizeof(struct glyphnamebucket));
	    new->sc = _sf->glyphs[i];
	    hash = hashname(new->sc->name);
	    new->next = gnh->table[hash];
	    gnh->table[hash] = new;
	}
	++k;
    } while ( k<sf->subfontcnt );
}

void SFHashGlyph(SplineFont *sf,SplineChar *sc) {
    /* sc just got added to the font. Put it in the lookup */
    int hash;
    struct glyphnamebucket *new;

    if ( sf->glyphnames==NULL )
return;		/* No hash table, nothing to update */

    new = chunkalloc(sizeof(struct glyphnamebucket));
    new->sc = sc;
    hash = hashname(sc->name);
    new->next = sf->glyphnames->table[hash];
    sf->glyphnames->table[hash] = new;
}

SplineChar *SFHashName(SplineFont *sf,const char *name) {
    struct glyphnamebucket *test;

    if ( sf->glyphnames==NULL )
	GlyphHashCreate(sf);

    for ( test=sf->glyphnames->table[hashname(name)]; test!=NULL; test = test->next )
	if ( strcmp(test->sc->name,name)==0 )
return( test->sc );

return( NULL );
}

static int SCUniMatch(SplineChar *sc,int unienc) {
    struct altuni *alt;

    if ( sc->unicodeenc==unienc )
return( true );
    for ( alt=sc->altuni; alt!=NULL; alt=alt->next )
	if ( alt->unienc==unienc )
return( true );

return( false );
}

/* Find the position in the glyph list where this code point/name is found. */
/*  Returns -1 else on error */
int SFFindGID(SplineFont *sf, int unienc, const char *name ) {
    int gid;
    SplineChar *sc;

    if ( unienc!=-1 ) {
	for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( sf->glyphs[gid]!=NULL ) {
	    if ( SCUniMatch(sf->glyphs[gid],unienc) )
return( gid );
	}
    }
    if ( name!=NULL ) {
	sc = SFHashName(sf,name);
	if ( sc!=NULL )
return( sc->orig_pos );
    }

return ( -1 );
}

/* Find the position in the current encoding where this code point/name should*/
/*  be found. (or for unencoded glyphs where it is found). Returns -1 else */
int SFFindSlot(SplineFont *sf, EncMap *map, int unienc, const char *name ) {
    int index=-1, pos;
    struct cidmap *cidmap;

    if ( sf->cidmaster!=NULL && !map->enc->is_compact &&
		(cidmap = FindCidMap(sf->cidmaster->cidregistry,
				    sf->cidmaster->ordering,
				    sf->cidmaster->supplement,
				    sf->cidmaster))!=NULL )
	index = NameUni2CID(cidmap,unienc,name);
    if ( index!=-1 )
	/* All done */;
    else if ( (map->enc->is_custom || map->enc->is_compact ||
	    map->enc->is_original) && unienc!=-1 ) {
	/* Just on the off-chance that it is unicode after all */
	if ( unienc<map->enccount && map->map[unienc]!=-1 &&
		sf->glyphs[map->map[unienc]]!=NULL &&
		sf->glyphs[map->map[unienc]]->unicodeenc==unienc )
	    index = unienc;
	else for ( index = map->enccount-1; index>=0; --index ) {
	    if ( (pos = map->map[index])!=-1 && sf->glyphs[pos]!=NULL &&
			SCUniMatch(sf->glyphs[pos],unienc) )
	break;
	}
    } else if ( unienc!=-1 &&
	    ((unienc<0x10000 && map->enc->is_unicodebmp) ||
	     (unienc<0x110000 && map->enc->is_unicodefull))) {
	 index = unienc;
    } else if ( unienc!=-1 ) {
	index = EncFromUni(unienc,map->enc);
	if ( index<0 || index>=map->enccount ) {
	    for ( index=map->enc->char_cnt; index<map->enccount; ++index )
		if ( (pos = map->map[index])!=-1 && sf->glyphs[pos]!=NULL &&
			SCUniMatch(sf->glyphs[pos],unienc) )
	    break;
	    if ( index>=map->enccount )
		index = -1;
	}
    }
    if ( index==-1 && name!=NULL ) {
	SplineChar *sc = SFHashName(sf,name);
	if ( sc!=NULL ) index = map->backmap[sc->orig_pos];
	if ( index==-1 ) {
	    unienc = UniFromName(name,sf->uni_interp,map->enc);
	    if ( unienc!=-1 )
return( SFFindSlot(sf,map,unienc,NULL));
	    if ( map->enc->psnames!=NULL ) {
		for ( index = map->enc->char_cnt-1; index>=0; --index )
		    if ( map->enc->psnames[index]!=NULL &&
			    strcmp(map->enc->psnames[index],name)==0 )
return( index );
	    }
	}
    }

return( index );
}

int SFCIDFindExistingChar(SplineFont *sf, int unienc, const char *name ) {
    int j,ret;

    if ( sf->subfonts==NULL && sf->cidmaster==NULL )
return( SFFindExistingSlot(sf,unienc,name));
    if ( sf->cidmaster!=NULL )
	sf=sf->cidmaster;
    for ( j=0; j<sf->subfontcnt; ++j )
	if (( ret = SFFindExistingSlot(sf,unienc,name))!=-1 )
return( ret );
return( -1 );
}

int SFCIDFindCID(SplineFont *sf, int unienc, const char *name ) {
	// For a given SplineFont *sf, find the index of the SplineChar with code unienc or name *name.
    int j,ret;
    struct cidmap *cidmap;
	
	// If there is a cidmap or if there are multiple subfonts, do complicated things.
    if ( sf->cidmaster!=NULL || sf->subfontcnt!=0 ) {
		if ( sf->cidmaster!=NULL )
			sf=sf->cidmaster;
		cidmap = FindCidMap(sf->cidregistry,sf->ordering,sf->supplement,sf);
		ret = NameUni2CID(cidmap,unienc,name);
		if ( ret!=-1 )
		    return( ret );
    }

	// If things are simple, perform a flat map.
    if ( sf->subfonts==NULL && sf->cidmaster==NULL )
	return( SFFindGID(sf,unienc,name));

	// If the cid lookup from before failed, look through subfonts.
    if ( sf->cidmaster!=NULL )
		sf=sf->cidmaster;
    for ( j=0; j<sf->subfontcnt; ++j )
		if (( ret = SFFindGID(sf,unienc,name))!=-1 )
			return( ret );

	return( -1 );
}

int SFHasCID(SplineFont *sf,int cid) {
    int i;
    /* What subfont (if any) contains this cid? */
    if ( sf->cidmaster!=NULL )
	sf=sf->cidmaster;
    for ( i=0; i<sf->subfontcnt; ++i )
	if ( cid<sf->subfonts[i]->glyphcnt &&
		SCWorthOutputting(sf->subfonts[i]->glyphs[cid]) )
return( i );
    for ( i=0; i<sf->subfontcnt; ++i )
	if ( cid<sf->subfonts[i]->glyphcnt && sf->subfonts[i]->glyphs[cid]!=NULL )
return( i );

return( -1 );
}

SplineChar *SFGetChar(SplineFont *sf, int unienc, const char *name ) {
	// This function presumably finds a glyph matching the code or name supplied. Undefined code is unienc = -1. Undefined name is name = NULL.
    int ind = -1;
    int j;
    char *pt, *start; int ch;

    if ( name==NULL )
		ind = SFCIDFindCID(sf,unienc,NULL);
    else {
		for ( start=(char *) name; *start==' '; ++start );
		for ( pt=start; *pt!='\0' && *pt!='('; ++pt );
		ch = *pt;
		// We truncate any glyph name before parentheses.
		if ( ch=='\0' )
			ind = SFCIDFindCID(sf,unienc,start);
		else {
			char *tmp;
			if ( (tmp = copy(name)) ) {
				tmp[pt-name] = '\0';
				ind = SFCIDFindCID(sf,unienc,tmp+(start-name));
				tmp[pt-name] = ch;
				free(tmp);
			}
		}
    }
    if ( ind==-1 )
		return( NULL );

	// If the typeface is simple, return the result from the flat glyph collection.
    if ( sf->subfonts==NULL && sf->cidmaster==NULL )
		return( sf->glyphs[ind]);

    if ( sf->cidmaster!=NULL )
		sf=sf->cidmaster;

	// Find a subfont that contains the glyph in question.
    j = SFHasCID(sf,ind);
    if ( j==-1 )
		return( NULL );

	return( sf->subfonts[j]->glyphs[ind] );
}

SplineChar *SFGetOrMakeChar(SplineFont *sf, int unienc, const char *name ) {
    SplineChar *sc=NULL;

    if ( sf->fv!=NULL ) {
	int ind = SFFindSlot(sf,sf->fv->map,unienc,name);
	if ( ind!=-1 )
	    sc = SFMakeChar(sf,sf->fv->map,ind);
    } else
	sc = SFGetChar(sf,unienc,name);
    if ( sc==NULL && (unienc!=-1 || name!=NULL)) {
	sc = SFSplineCharCreate(sf);
	if ( sf->strokedfont ) {
	    sc->layers[ly_fore].dostroke = true;
	    sc->layers[ly_fore].dofill = false;
	}
	sc->unicodeenc = unienc;
	if ( name!=NULL )
	    sc->name = copy(name);
	else {
	    char buffer[40];
	    sprintf(buffer,"glyph%d", sf->glyphcnt);
	    sc->name = copy(buffer);
	}
	SFAddGlyphAndEncode(sf,sc,NULL,-1);
	/*SCLigDefault(sc);*/
    }
return( sc );
}

SplineChar *SFGetOrMakeCharFromUnicode( SplineFont *sf, EncMap *map, int ch )
{
    int i;
    SplineChar *sc;

    i = SFFindSlot(sf,map,ch,NULL);
    if ( i==-1 )
	return( NULL );
    else
    {
	sc = SFMakeChar(sf,map,i);
    }
    return( sc );
}
SplineChar *SFGetOrMakeCharFromUnicodeBasic( SplineFont *sf, int ch )
{
    return SFGetOrMakeCharFromUnicode( sf, sf->fv->map, ch );
}



static int _SFFindExistingSlot(SplineFont *sf, int unienc, const char *name ) {
    int gid = -1;
    struct altuni *altuni;

    if ( unienc!=-1 ) {
	for ( gid=sf->glyphcnt-1; gid>=0; --gid ) if ( sf->glyphs[gid]!=NULL ) {
	    if ( sf->glyphs[gid]->unicodeenc==unienc )
	break;
	    for ( altuni=sf->glyphs[gid]->altuni ; altuni!=NULL &&
		    (altuni->unienc!=unienc || altuni->vs!=-1 || altuni->fid!=0);
		    altuni=altuni->next );
	    if ( altuni!=NULL )
	break;
	}
    }
    if ( gid==-1 && name!=NULL ) {
	SplineChar *sc = SFHashName(sf,name);
	if ( sc==NULL )
return( -1 );
	gid = sc->orig_pos;
	if ( gid<0 || gid>=sf->glyphcnt ) {
	    IError("Invalid glyph location when searching for %s", name );
return( -1 );
	}
    }
return( gid );
}

int SFFindExistingSlot(SplineFont *sf, int unienc, const char *name ) {
    int gid = _SFFindExistingSlot(sf,unienc,name);

    if ( gid==-1 || !SCWorthOutputting(sf->glyphs[gid]) )
return( -1 );

return( gid );
}

static void MFixupSC(SplineFont *sf, SplineChar *sc,int i) {
    RefChar *ref, *prev;
    int l;

    sc->orig_pos = i;
    sc->parent = sf;
    sc->ticked = true;
    for ( l=0; l<sc->layer_cnt; ++l ) {
     retry:
	for ( ref=sc->layers[l].refs; ref!=NULL; ref=ref->next ) {
	    /* The sc in the ref is from the old font. It's got to be in the */
	    /*  new font too (was either already there or just got copied) */
	    ref->orig_pos =  SFFindExistingSlot(sf,ref->sc->unicodeenc,ref->sc->name);
	    if ( ref->orig_pos==-1 ) {
		IError("Bad reference, can't fix it up");
		if ( ref==sc->layers[l].refs ) {
		    sc->layers[l].refs = ref->next;
     goto retry;
		} else {
		    for ( prev=sc->layers[l].refs; prev->next!=ref; prev=prev->next );
		    prev->next = ref->next;
		    chunkfree(ref,sizeof(*ref));
		    ref = prev;
		}
	    } else {
		ref->sc = sf->glyphs[ref->orig_pos];
		if ( !ref->sc->ticked )
		    MFixupSC(sf,ref->sc,ref->orig_pos);
		SCReinstanciateRefChar(sc,ref,l);
		SCMakeDependent(sc,ref->sc);
	    }
	}
    }
    /* I shan't automagically generate bitmaps for any bdf fonts */
    /*  but I have copied over the ones which match */
}

static void MergeFixupRefChars(SplineFont *sf) {
    int i;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	sf->glyphs[i]->ticked = false;
    }
    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL && !sf->glyphs[i]->ticked ) {
	MFixupSC(sf,sf->glyphs[i], i);
    }
}

int SFHasChar(SplineFont *sf, int unienc, const char *name ) {
    SplineChar *sc = SFGetChar(sf,unienc,name);

return( SCWorthOutputting(sc) );
}

static void FVMergeRefigureMapSel(FontViewBase *fv,SplineFont *into,SplineFont *o_sf,
	int *mapping, int emptypos, int cnt) {
    int extras, doit, i;
    EncMap *map = fv->map;
    int base = map->enccount;

    base = map->enccount;

    for ( doit=0; doit<2; ++doit ) {
	extras = 0;
	for ( i=0; i<o_sf->glyphcnt; ++i ) if ( mapping[i]>=emptypos ) {
	    int index = SFFindSlot(into,map,o_sf->glyphs[i]->unicodeenc,o_sf->glyphs[i]->name);
	    if ( !doit ) {
		if ( index==-1 )
		    ++extras;
	    } else {
		if ( index==-1 )
		    index = base+ extras++;
		map->map[index] = mapping[i];
		map->backmap[mapping[i]] = index;
	    }
	}
	if ( !doit ) {
	    if ( into->glyphcnt+cnt>map->backmax ) {
		map->backmap = realloc(map->backmap,(into->glyphcnt+cnt)*sizeof(int));
		map->backmax = into->glyphcnt+cnt;
	    }
	    memset(map->backmap+into->glyphcnt,-1,cnt*sizeof(int));
	    if ( map->enccount+extras>map->encmax ) {
		map->map = realloc(map->map,(map->enccount+extras)*sizeof(int));
		map->encmax = map->enccount+extras;
	    }
	    memset(map->map+map->enccount,-1,extras*sizeof(int));
	    map->enccount += extras;
	}
    }
    if ( extras!=0 ) {
	fv->selected = realloc(fv->selected,map->enccount);
	memset(fv->selected+map->enccount-extras,0,extras);
    }
}

static void _MergeFont(SplineFont *into,SplineFont *other,struct sfmergecontext *mc) {
    int i, cnt, doit, emptypos, index, k;
    SplineFont *o_sf, *bitmap_into;
    BDFFont *bdf;
    FontViewBase *fvs;
    int *mapping;

    emptypos = into->glyphcnt;
    mapping = malloc(other->glyphcnt*sizeof(int));
    memset(mapping,-1,other->glyphcnt*sizeof(int));

    bitmap_into = into->cidmaster!=NULL? into->cidmaster : into;

    for ( doit=0; doit<2; ++doit ) {
	cnt = 0;
	k = 0;
	do {
	    o_sf = ( other->subfonts==NULL ) ? other : other->subfonts[k];
	    for ( i=0; i<o_sf->glyphcnt; ++i ) if ( o_sf->glyphs[i]!=NULL ) {
		if ( doit && (index = mapping[i])!=-1 ) {
		    /* Bug here. Suppose someone has a reference to our empty */
		    /*  char */
		    SplineCharFree(into->glyphs[index]);
		    into->glyphs[index] = SplineCharCopy(o_sf->glyphs[i],into,mc);
		    into->glyphs[index]->orig_pos = index;
		    if ( into->bitmaps!=NULL && other->bitmaps!=NULL )
			BitmapsCopy(bitmap_into,other,index,i);
		} else if ( !doit ) {
		    if ( !SCWorthOutputting(o_sf->glyphs[i] ))
			/* Don't bother to copy it */;
		    else if ( !SFHasChar(into,o_sf->glyphs[i]->unicodeenc,o_sf->glyphs[i]->name)) {
			/* Possible for a font to contain a glyph not worth outputting */
			if ( (index = _SFFindExistingSlot(into,o_sf->glyphs[i]->unicodeenc,o_sf->glyphs[i]->name))==-1 )
			    index = emptypos+cnt++;
			mapping[i] = index;
		    }
		}
	    }
	    if ( !doit ) {
		if ( emptypos+cnt >= into->glyphcnt && emptypos+cnt>0 ) {
		    into->glyphs = realloc(into->glyphs,(emptypos+cnt)*sizeof(SplineChar *));
		    memset(into->glyphs+emptypos,0,cnt*sizeof(SplineChar *));
		    for ( bdf = bitmap_into->bitmaps; bdf!=NULL; bdf=bdf->next )
			if ( emptypos+cnt > bdf->glyphcnt ) {
			    bdf->glyphs = realloc(bdf->glyphs,(emptypos+cnt)*sizeof(BDFChar *));
			    memset(bdf->glyphs+emptypos,0,cnt*sizeof(BDFChar *));
			    bdf->glyphmax = bdf->glyphcnt = emptypos+cnt;
			}
		    for ( fvs = into->fv; fvs!=NULL; fvs=fvs->nextsame )
			FVBiggerGlyphCache(fvs,emptypos+cnt);
		    for ( fvs = into->fv; fvs!=NULL; fvs=fvs->nextsame )
			if ( fvs->sf == into )
			    FVMergeRefigureMapSel(fvs,into,o_sf,mapping,emptypos,cnt);
		    into->glyphcnt = into->glyphmax = emptypos+cnt;
		}
	    }
	    ++k;
	} while ( k<other->subfontcnt );
    }
    for ( i=0; i<other->glyphcnt; ++i ) {
	if ( (index=mapping[i])!=-1 )
	    into->glyphs[index]->kerns = KernsCopy(other->glyphs[i]->kerns,mapping,into,mc);
	else if ( mc->preserveCrossFontKerning && other->glyphs[i]!=NULL ) {
	    KernPair *kpnew, *kpend;
	    index = _SFFindExistingSlot(into,other->glyphs[i]->unicodeenc,other->glyphs[i]->name);
	    if ( index!=-1 ) {
		mc->preserveCrossFontKerning = false;
		kpnew = KernsCopy(other->glyphs[i]->kerns,mapping,into,mc);
		mc->preserveCrossFontKerning = true;
		if ( kpnew ) {
		    for ( kpend=kpnew; kpend->next!=NULL; kpend=kpend->next );
		    kpend->next = into->glyphs[index]->kerns;
		    into->glyphs[index]->kerns = kpnew;
		}
	    }
	}
    }

    free(mapping);
    GlyphHashFree(into);
    MergeFixupRefChars(into);
    if ( other->fv==NULL )
	SplineFontFree(other);
    into->changed = true;
    FontViewReformatAll(into);
    GlyphHashFree(into);
}

static void __MergeFont(SplineFont *into,SplineFont *other, int preserveCrossFontKerning) {
    struct sfmergecontext mc;

    memset(&mc,0,sizeof(mc));
    mc.sf_from = other; mc.sf_to = into;
    mc.preserveCrossFontKerning = preserveCrossFontKerning;

    AnchorClassesAdd(into,other,&mc);
    FPSTsAdd(into,other,&mc);
    ASMsAdd(into,other,&mc);
    KernClassesAdd(into,other,&mc);

    _MergeFont(into,other,&mc);

    SFFinishMergeContext(&mc);
}

static void CIDMergeFont(SplineFont *into,SplineFont *other, int preserveCrossFontKerning) {
    int i,j,k;
    SplineFont *i_sf, *o_sf;
    FontViewBase *fvs;
    struct sfmergecontext mc;

    memset(&mc,0,sizeof(mc));
    mc.sf_from = other; mc.sf_to = into;
    mc.preserveCrossFontKerning = preserveCrossFontKerning;

    AnchorClassesAdd(into,other,&mc);
    FPSTsAdd(into,other,&mc);
    ASMsAdd(into,other,&mc);
    KernClassesAdd(into,other,&mc);

    k = 0;
    do {
	o_sf = other->subfonts[k];
	i_sf = into->subfonts[k];
	for ( i=o_sf->glyphcnt-1; i>=0 && o_sf->glyphs[i]==NULL; --i );
	if ( i>=i_sf->glyphcnt ) {
	    i_sf->glyphs = realloc(i_sf->glyphs,(i+1)*sizeof(SplineChar *));
	    for ( j=i_sf->glyphcnt; j<=i; ++j )
		i_sf->glyphs[j] = NULL;
	    for ( fvs = i_sf->fv; fvs!=NULL; fvs=fvs->nextsame )
		if ( fvs->sf==i_sf ) {
		    fvs->selected = realloc(fvs->selected,i+1);
		    for ( j=i_sf->glyphcnt; j<=i; ++j )
			fvs->selected[j] = false;
		}
	    i_sf->glyphcnt = i_sf->glyphmax = i+1;
	}
	for ( i=0; i<o_sf->glyphcnt; ++i ) if ( o_sf->glyphs[i]!=NULL ) {
	    if ( o_sf->glyphs[i]->layers[ly_fore].splines==NULL && o_sf->glyphs[i]->layers[ly_fore].refs==NULL &&
		    !o_sf->glyphs[i]->widthset )
		/* Don't bother to copy it */;
	    else if ( SFHasCID(into,i)==-1 ) {
		SplineCharFree(i_sf->glyphs[i]);
		i_sf->glyphs[i] = SplineCharCopy(o_sf->glyphs[i],i_sf,&mc);
		i_sf->glyphs[i]->orig_pos = i;
		if ( into->bitmaps!=NULL && other->bitmaps!=NULL )
		    BitmapsCopy(into,other,i,i);
	    }
	}
	MergeFixupRefChars(i_sf);
	++k;
    } while ( k<other->subfontcnt );
    FontViewReformatAll(into);
    into->changed = true;
    GlyphHashFree(into);

    SFFinishMergeContext(&mc);
}

void MergeFont(FontViewBase *fv,SplineFont *other, int preserveCrossFontKerning) {

    if ( fv->sf==other ) {
	ff_post_error(_("Merging Problem"),_("Merging a font with itself achieves nothing"));
return;
    }
    if ( fv->sf->cidmaster!=NULL && other->subfonts!=NULL &&
	    (strcmp(fv->sf->cidmaster->cidregistry,other->cidregistry)!=0 ||
	     strcmp(fv->sf->cidmaster->ordering,other->ordering)!=0 ||
	     fv->sf->cidmaster->supplement<other->supplement ||
	     fv->sf->cidmaster->subfontcnt<other->subfontcnt )) {
	ff_post_error(_("Merging Problem"),_("When merging two CID keyed fonts, they must have the same Registry and Ordering, and the font being merged into (the mergee) must have a supplement which is at least as recent as the other's. Furthermore the mergee must have at least as many subfonts as the merger."));
return;
    }
    /* Ok. when merging CID fonts... */
    /*  If fv is normal and other is CID then just flatten other and merge everything into fv */
    /*  If fv is CID and other is normal then merge other into the currently active font */
    /*  If both are CID then merge each subfont separately */
    if ( fv->sf->cidmaster!=NULL && other->subfonts!=NULL )
	CIDMergeFont(fv->sf->cidmaster,other,preserveCrossFontKerning);
    else
	__MergeFont(fv->sf,other,preserveCrossFontKerning);
}

/******************************************************************************/
/* *************************** Font Interpolation *************************** */
/******************************************************************************/

static RefChar *InterpRefs(RefChar *base, RefChar *other, real amount, SplineChar *sc) {
    RefChar *head=NULL, *last=NULL, *cur;
    RefChar *test;
    int i;

    for ( test = other; test!=NULL; test=test->next )
	test->checked = false;

    while ( base!=NULL ) {
	for ( test = other; test!=NULL; test=test->next ) {
	    if ( test->checked )
		/* Do nothing */;
	    else if ( test->unicode_enc==base->unicode_enc &&
		    (test->unicode_enc!=-1 || strcmp(test->sc->name,base->sc->name)==0 ) )
	break;
	}
	if ( test!=NULL ) {
	    test->checked = true;
	    cur = RefCharCreate();
	    free(cur->layers);
	    *cur = *base;
	    cur->orig_pos = cur->sc->orig_pos;
	    for ( i=0; i<6; ++i )
		cur->transform[i] = base->transform[i] + amount*(other->transform[i]-base->transform[i]);
	    cur->layers = NULL;
	    cur->layer_cnt = 0;
	    cur->checked = false;
	    if ( head==NULL )
		head = cur;
	    else
		last->next = cur;
	    last = cur;
	} else
	    LogError( _("In character %s, could not find reference to %s\n"),
		    sc->name, base->sc->name );
	base = base->next;
	if ( test==other && other!=NULL )
	    other = other->next;
    }
return( head );
}

static void InterpPoint(SplineSet *cur, SplinePoint *base, SplinePoint *other, real amount ) {
    SplinePoint *p = chunkalloc(sizeof(SplinePoint));
    int order2 = base->prev!=NULL ? base->prev->order2 : base->next!=NULL ? base->next->order2 : false;

    p->me.x = base->me.x + amount*(other->me.x-base->me.x);
    p->me.y = base->me.y + amount*(other->me.y-base->me.y);
    if ( order2 && base->prev!=NULL && (base->prev->islinear || other->prev->islinear ))
	p->prevcp = p->me;
    else {
	p->prevcp.x = base->prevcp.x + amount*(other->prevcp.x-base->prevcp.x);
	p->prevcp.y = base->prevcp.y + amount*(other->prevcp.y-base->prevcp.y);
	if ( order2 && cur->first!=NULL ) {
	    /* In the normal course of events this isn't needed, but if we have */
	    /*  a different number of points on one path than on the other then */
	    /*  all hell breaks lose without this */
	    cur->last->nextcp.x = p->prevcp.x = (cur->last->nextcp.x+p->prevcp.x)/2;
	    cur->last->nextcp.y = p->prevcp.y = (cur->last->nextcp.y+p->prevcp.y)/2;
	}
    }
    if ( order2 && base->next!=NULL && (base->next->islinear || other->next->islinear ))
	p->nextcp = p->me;
    else {
	p->nextcp.x = base->nextcp.x + amount*(other->nextcp.x-base->nextcp.x);
	p->nextcp.y = base->nextcp.y + amount*(other->nextcp.y-base->nextcp.y);
    }
    p->nonextcp = ( p->nextcp.x==p->me.x && p->nextcp.y==p->me.y );
    p->noprevcp = ( p->prevcp.x==p->me.x && p->prevcp.y==p->me.y );
    p->prevcpdef = base->prevcpdef && other->prevcpdef;
    p->nextcpdef = base->nextcpdef && other->nextcpdef;
    p->selected = false;
    p->pointtype = (base->pointtype==other->pointtype)?base->pointtype:pt_corner;
    /*p->flex = 0;*/
    if ( cur->first==NULL ) {
	cur->first = p;
	cur->start_offset = 0;
    } else
	SplineMake(cur->last,p,order2);
    cur->last = p;
}
    
static SplineSet *InterpSplineSet(SplineSet *base, SplineSet *other, real amount, SplineChar *sc) {
    SplineSet *cur = chunkalloc(sizeof(SplineSet));
    SplinePoint *bp, *op;

    for ( bp=base->first, op = other->first; ; ) {
	InterpPoint(cur,bp,op,amount);
	if ( bp->next == NULL && op->next == NULL )
return( cur );
	if ( bp->next!=NULL && op->next!=NULL &&
		bp->next->to==base->first && op->next->to==other->first ) {
	    SplineMake(cur->last,cur->first,bp->next->order2);
	    cur->last = cur->first;
return( cur );
	}
	if ( bp->next == NULL || bp->next->to==base->first ) {
	    LogError( _("In character %s, there are too few points on a path in the base\n"), sc->name);
	    if ( bp->next!=NULL ) {
		if ( bp->next->order2 ) {
		    cur->last->nextcp.x = cur->first->prevcp.x = (cur->last->nextcp.x+cur->first->prevcp.x)/2;
		    cur->last->nextcp.y = cur->first->prevcp.y = (cur->last->nextcp.y+cur->first->prevcp.y)/2;
		}
		SplineMake(cur->last,cur->first,bp->next->order2);
		cur->last = cur->first;
	    }
return( cur );
	} else if ( op->next==NULL || op->next->to==other->first ) {
	    LogError( _("In character %s, there are too many points on a path in the base\n"), sc->name);
	    while ( bp->next!=NULL && bp->next->to!=base->first ) {
		bp = bp->next->to;
		InterpPoint(cur,bp,op,amount);
	    }
	    if ( bp->next!=NULL ) {
		if ( bp->next->order2 ) {
		    cur->last->nextcp.x = cur->first->prevcp.x = (cur->last->nextcp.x+cur->first->prevcp.x)/2;
		    cur->last->nextcp.y = cur->first->prevcp.y = (cur->last->nextcp.y+cur->first->prevcp.y)/2;
		}
		SplineMake(cur->last,cur->first,bp->next->order2);
		cur->last = cur->first;
	    }
return( cur );
	}
	bp = bp->next->to;
	op = op->next->to;
    }
}

SplineSet *SplineSetsInterpolate(SplineSet *base, SplineSet *other, real amount, SplineChar *sc) {
    SplineSet *head=NULL, *last=NULL, *cur;

    /* we could do something really complex to try and figure out which spline*/
    /*  set goes with which, but I'm not sure that it would really accomplish */
    /*  much, if things are off the computer probably won't figure it out */
    /* Could use path open/closed, direction, point count */
    while ( base!=NULL && other!=NULL ) {
	cur = InterpSplineSet(base,other,amount,sc);
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;
	base = base->next;
	other = other->next;
    }
return( head );
}

static int SCSameChar(SplineChar *sc1,SplineChar *sc2) {
    if ( sc1->unicodeenc!=-1 )
return( sc1->unicodeenc==sc2->unicodeenc );

return( strcmp(sc1->name,sc2->name)==0 );
}

static KernPair *InterpKerns(KernPair *kp1, KernPair *kp2, real amount,
	SplineFont *new, SplineChar *scnew) {
    KernPair *head=NULL, *last, *nkp, *k;

    if ( kp1==NULL || kp2==NULL )
return( NULL );
    while ( kp1!=NULL ) {
	for ( k=kp2; k!=NULL && !SCSameChar(k->sc,kp1->sc); k=k->next );
	if ( k!=NULL ) {
	    if ( k==kp2 ) kp2 = kp2->next;
	    nkp = chunkalloc(sizeof(KernPair));
	    nkp->sc = new->glyphs[kp1->sc->orig_pos];
	    nkp->off = kp1->off + amount*(k->off-kp1->off);
	    nkp->subtable = SFSubTableFindOrMake(new,CHR('k','e','r','n'),
		    SCScriptFromUnicode(scnew),gpos_pair);
	    if ( head==NULL )
		head = nkp;
	    else
		last->next = nkp;
	    last = nkp;
	}
	kp1 = kp1->next;
    }
return( head );
}

static uint32 InterpColor( uint32 col1,uint32 col2, real amount ) {
    int r1, g1, b1, r2, b2, g2;

    r1 = (col1>>16)&0xff;
    r2 = (col2>>16)&0xff;
    g1 = (col1>>8 )&0xff;
    g2 = (col2>>8 )&0xff;
    b1 = (col1    )&0xff;
    b2 = (col2    )&0xff;
    r1 += amount * (r2-r1);
    g1 += amount * (g2-g1);
    b1 += amount * (b2-b1);
return( (r1<<16) | (g1<<8) | b1 );
}

static void LayerInterpolate(Layer *to,Layer *base,Layer *other,real amount,SplineChar *sc, int lc) {

    /* to already has things set to default values, so when an error occurs */
    /*  I just leave things as the default (and don't need to set it again) */
    if ( base->dostroke==other->dostroke )
	to->dostroke = base->dostroke;
    else
	LogError( _("Different settings on whether to stroke in layer %d of %s\n"), lc, sc->name );
    if ( base->dofill==other->dofill )
	to->dofill = base->dofill;
    else
	LogError(_("Different settings on whether to fill in layer %d of %s\n"), lc, sc->name );
    if ( base->fill_brush.col==COLOR_INHERITED && other->fill_brush.col==COLOR_INHERITED )
	to->fill_brush.col = COLOR_INHERITED;
    else if ( base->fill_brush.col!=COLOR_INHERITED && other->fill_brush.col!=COLOR_INHERITED )
	to->fill_brush.col = InterpColor( base->fill_brush.col,other->fill_brush.col, amount );
    else
	LogError(_("Different settings on whether to inherit fill color in layer %d of %s\n"), lc, sc->name );
    if ( base->fill_brush.opacity<0 && other->fill_brush.opacity<0 )
	to->fill_brush.opacity = WIDTH_INHERITED;
    else if ( base->fill_brush.opacity>=0 && other->fill_brush.opacity>=0 )
	to->fill_brush.opacity = base->fill_brush.opacity + amount*(other->fill_brush.opacity-base->fill_brush.opacity);
    else
	LogError(_("Different settings on whether to inherit fill opacity in layer %d of %s\n"), lc, sc->name );
    if ( base->stroke_pen.brush.col==COLOR_INHERITED && other->stroke_pen.brush.col==COLOR_INHERITED )
	to->stroke_pen.brush.col = COLOR_INHERITED;
    else if ( base->stroke_pen.brush.col!=COLOR_INHERITED && other->stroke_pen.brush.col!=COLOR_INHERITED )
	to->stroke_pen.brush.col = InterpColor( base->stroke_pen.brush.col,other->stroke_pen.brush.col, amount );
    else
	LogError(_("Different settings on whether to inherit fill color in layer %d of %s\n"), lc, sc->name );
    if ( base->stroke_pen.brush.opacity<0 && other->stroke_pen.brush.opacity<0 )
	to->stroke_pen.brush.opacity = WIDTH_INHERITED;
    else if ( base->stroke_pen.brush.opacity>=0 && other->stroke_pen.brush.opacity>=0 )
	to->stroke_pen.brush.opacity = base->stroke_pen.brush.opacity + amount*(other->stroke_pen.brush.opacity-base->stroke_pen.brush.opacity);
    else
	LogError(_("Different settings on whether to inherit stroke opacity in layer %d of %s\n"), lc, sc->name );
    if ( base->stroke_pen.width<0 && other->stroke_pen.width<0 )
	to->stroke_pen.width = WIDTH_INHERITED;
    else if ( base->stroke_pen.width>=0 && other->stroke_pen.width>=0 )
	to->stroke_pen.width = base->stroke_pen.width + amount*(other->stroke_pen.width-base->stroke_pen.width);
    else
	LogError(_("Different settings on whether to inherit stroke width in layer %d of %s\n"), lc, sc->name );
    if ( base->stroke_pen.linecap==other->stroke_pen.linecap )
	to->stroke_pen.linecap = base->stroke_pen.linecap;
    else
	LogError(_("Different settings on stroke linecap in layer %d of %s\n"), lc, sc->name );
    if ( base->stroke_pen.linejoin==other->stroke_pen.linejoin )
	to->stroke_pen.linejoin = base->stroke_pen.linejoin;
    else
	LogError(_("Different settings on stroke linejoin in layer %d of %s\n"), lc, sc->name );
    if ( base->fill_brush.gradient!=NULL || other->fill_brush.gradient!=NULL ||
	    base->stroke_pen.brush.gradient!=NULL || other->stroke_pen.brush.gradient!=NULL )
	LogError(_("I can't even imagine how to attempt to interpolate gradients in layer %d of %s\n"), lc, sc->name );
    if ( base->fill_brush.pattern!=NULL || other->fill_brush.pattern!=NULL )
	LogError(_("Different fill patterns in layer %d of %s\n"), lc, sc->name );
    if ( base->stroke_pen.brush.pattern!=NULL || other->stroke_pen.brush.pattern!=NULL )
	LogError(_("Different stroke patterns in layer %d of %s\n"), lc, sc->name );

    to->splines = SplineSetsInterpolate(base->splines,other->splines,amount,sc);
    to->refs = InterpRefs(base->refs,other->refs,amount,sc);
    if ( base->images!=NULL || other->images!=NULL )
	LogError(_("I can't even imagine how to attempt to interpolate images in layer %d of %s\n"), lc, sc->name );
}

SplineChar *SplineCharInterpolate(SplineChar *base, SplineChar *other,
	real amount, SplineFont *newfont) {
    SplineChar *sc;
    int i;

    if ( base==NULL || other==NULL )
return( NULL );
    sc = SFSplineCharCreate(newfont);
    sc->unicodeenc = base->unicodeenc;
    sc->changed = true;
    sc->views = NULL;
    sc->dependents = NULL;
    sc->layers[ly_back].splines = NULL;
    sc->layers[ly_back].images = NULL;
    sc->layers[ly_fore].undoes = sc->layers[ly_back].undoes = NULL;
    sc->layers[ly_fore].redoes = sc->layers[ly_back].redoes = NULL;
    sc->kerns = NULL;
    sc->name = copy(base->name);
    sc->width = base->width + amount*(other->width-base->width);
    sc->vwidth = base->vwidth + amount*(other->vwidth-base->vwidth);
    sc->lsidebearing = base->lsidebearing + amount*(other->lsidebearing-base->lsidebearing);
    if ( base->parent->multilayer && other->parent->multilayer ) {
	int lc = base->layer_cnt;
	if ( lc!=other->layer_cnt ) {
	    LogError(_("Different numbers of layers in %s\n"), base->name );
	    if ( other->layer_cnt<lc ) lc = other->layer_cnt;
	}
	if ( lc>2 ) {
	    sc->layers = realloc(sc->layers,lc*sizeof(Layer));
	    for ( i=ly_fore+1; i<lc; ++i )
		LayerDefault(&sc->layers[i]);
	}
	for ( i=ly_fore; i<lc; ++i )
	    LayerInterpolate(&sc->layers[i],&base->layers[i],&other->layers[i],amount,sc,i);
    } else {
	for ( i=0; i<sc->layer_cnt; ++i ) {
	    if ( i>=base->layer_cnt || i>= other->layer_cnt )
	break;
	    sc->layers[i].splines = SplineSetsInterpolate(base->layers[i].splines,other->layers[i].splines,amount,sc);
	    sc->layers[i].refs = InterpRefs(base->layers[i].refs,other->layers[i].refs,amount,sc);
	}
    }
    sc->changedsincelasthinted = true;
    sc->widthset = base->widthset;
    sc->glyph_class = base->glyph_class;
return( sc );
}

static void _SplineCharInterpolate(SplineFont *new, int orig_pos, SplineChar *base, SplineChar *other, real amount) {
    SplineChar *sc;

    sc = SplineCharInterpolate(base,other,amount,new);
    if ( sc==NULL )
return;
    sc->orig_pos = orig_pos;
    new->glyphs[orig_pos] = sc;
    if ( orig_pos+1>new->glyphcnt )
	new->glyphcnt = orig_pos+1;
    sc->parent = new;
}

static void IFixupSC(SplineFont *sf, SplineChar *sc,int i) {
    RefChar *ref;

    for ( ref=sc->layers[ly_fore].refs; ref!=NULL; ref=ref->next ) if ( !ref->checked ) {
	/* The sc in the ref is from the base font. It's got to be in the */
	/*  new font too (the ref only gets created if it's present in both fonts)*/
	ref->checked = true;
	ref->orig_pos = SFFindExistingSlot(sf,ref->sc->unicodeenc,ref->sc->name);
	ref->sc = sf->glyphs[ref->orig_pos];
	IFixupSC(sf,ref->sc,ref->orig_pos);
	SCReinstanciateRefChar(sc,ref,ly_fore);
	SCMakeDependent(sc,ref->sc);
    }
}

static void InterpFixupRefChars(SplineFont *sf) {
    int i;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	IFixupSC(sf,sf->glyphs[i], i);
    }
}

SplineFont *InterpolateFont(SplineFont *base, SplineFont *other, real amount,
	Encoding *enc) {
    SplineFont *new;
    int i, index, lc;

    if ( base==other ) {
	ff_post_error(_("Interpolating Problem"),_("Interpolating a font with itself achieves nothing"));
return( NULL );
    } else if ( base->layers[ly_fore].order2!=other->layers[ly_fore].order2 ) {
	ff_post_error(_("Interpolating Problem"),_("Interpolating between fonts with different spline orders (i.e. between postscript and truetype)"));
return( NULL );
    } else if ( base->multilayer && other->multilayer ) {
	ff_post_error(_("Interpolating Problem"),_("Interpolating between fonts with different editing types (ie. between type3 and type1)"));
return( NULL );
    }
    new = SplineFontBlank(base->glyphcnt);
    new->ascent = base->ascent + amount*(other->ascent-base->ascent);
    new->descent = base->descent + amount*(other->descent-base->descent);
    if ( (lc=base->layer_cnt)>other->layer_cnt )
	lc = other->layer_cnt;
    if ( lc!=new->layer_cnt ) {
	new->layer_cnt = lc;
	new->layers = realloc(new->layers,lc*sizeof(LayerInfo));
	if ( lc>2 )
	    memset(new->layers+2,0,(lc-2)*sizeof(LayerInfo));
	for ( i=2; i<lc; ++i ) {
	    new->layers[i].name = copy(base->layers[i].name);
	    new->layers[i].background = base->layers[i].background;
	    new->layers[i].order2 = base->layers[i].order2;
	}
    }
    for ( i=0; i<2; ++i ) {
	new->layers[i].background = base->layers[i].background;
	new->layers[i].order2 = base->layers[i].order2;
    }
    for ( i=0; i<base->glyphcnt; ++i ) if ( base->glyphs[i]!=NULL ) {
	index = SFFindExistingSlot(other,base->glyphs[i]->unicodeenc,base->glyphs[i]->name);
	if ( index!=-1 && other->glyphs[index]!=NULL ) {
	    _SplineCharInterpolate(new,i,base->glyphs[i],other->glyphs[index],amount);
	    if ( new->glyphs[i]!=NULL )
		new->glyphs[i]->kerns = InterpKerns(base->glyphs[i]->kerns,
			other->glyphs[index]->kerns,amount,new,new->glyphs[i]);
	}
    }
    InterpFixupRefChars(new);
    new->changed = true;
    new->map = EncMapFromEncoding(new,enc);
return( new );
}
