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
#include "ustring.h"
#include "utype.h"
#include "gfile.h"
#include "chardata.h"

static RefChar *RefCharsCopy(RefChar *ref) {
    RefChar *rhead=NULL, *last=NULL, *cur;

    while ( ref!=NULL ) {
	cur = RefCharCreate();
#ifdef FONTFORGE_CONFIG_TYPE3
	{ struct reflayer *layers = cur->layers; int layer;
	layers = grealloc(layers,ref->layer_cnt*sizeof(struct reflayer));
	memcpy(layers,ref->layers,ref->layer_cnt*sizeof(struct reflayer));
	*cur = *ref;
	cur->layers = layers;
	for ( layer=0; layer<cur->layer_cnt; ++layer ) {
	    cur->layers[layer].splines = NULL;
	    cur->layers[layer].images = NULL;
	}
	}
#else
	*cur = *ref;
	cur->layers[0].splines = NULL;	/* Leave the old sc, we'll fix it later */
#endif
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


static int FixupSLI(int sli,SplineFont *from,SplineFont *to) {
    int i,j,k;

    if ( from->cidmaster ) from = from->cidmaster;
    else if ( from->mm!=NULL ) from = from->mm->normal;
    if ( to->cidmaster ) to = to->cidmaster;
    else if ( to->mm!=NULL ) to = to->mm->normal;
    if ( from==to )
return( sli );

    /* does the new font have exactly what we want? */
    i = 0;
    if ( to->script_lang!=NULL ) {
	for ( i=0; to->script_lang[i]!=NULL; ++i ) {
	    for ( j=0; to->script_lang[i][j].script!=0 &&
		    to->script_lang[i][j].script==from->script_lang[sli][j].script; ++j ) {
		for ( k=0; to->script_lang[i][j].langs[k]!=0 &&
			to->script_lang[i][j].langs[k]==from->script_lang[sli][j].langs[k]; ++k );
		if ( to->script_lang[i][j].langs[k]!=0 || from->script_lang[sli][j].langs[k]!=0 )
	    break;
	    }
	    if ( to->script_lang[i][j].script==0 && from->script_lang[sli][j].script==0 )
return( i );
	}
    }

    /* Add it */
    if ( to->script_lang==NULL )
	to->script_lang = galloc(2*sizeof(struct script_record *));
    else
	to->script_lang = grealloc(to->script_lang,(i+2)*sizeof(struct script_record *));
    to->sli_cnt = i+1;
    to->script_lang[i+1] = NULL;
    for ( j=0; from->script_lang[sli][j].script!=0; ++j );
    to->script_lang[i] = galloc((j+1)*sizeof(struct script_record));
    for ( j=0; from->script_lang[sli][j].script!=0; ++j ) {
	to->script_lang[i][j].script = from->script_lang[sli][j].script;
	for ( k=0; from->script_lang[sli][j].langs[k]!=0; ++k );
	to->script_lang[i][j].langs = galloc((k+1)*sizeof(uint32));
	for ( k=0; from->script_lang[sli][j].langs[k]!=0; ++k )
	    to->script_lang[i][j].langs[k] = from->script_lang[sli][j].langs[k];
	to->script_lang[i][j].langs[k] = 0;
    }
    to->script_lang[i][j].script = 0;
return( i );
}

PST *PSTCopy(PST *base,SplineChar *sc,SplineFont *from) {
    PST *head=NULL, *last=NULL, *cur;

    for ( ; base!=NULL; base = base->next ) {
	cur = chunkalloc(sizeof(PST));
	*cur = *base;
	if ( from!=sc->parent && base->script_lang_index<SLI_NESTED )
	    cur->script_lang_index = FixupSLI(cur->script_lang_index,from,sc->parent);
	if ( cur->type==pst_ligature ) {
	    cur->u.lig.components = copy(cur->u.lig.components);
	    cur->u.lig.lig = sc;
	} else if ( cur->type==pst_pair ) {
	    cur->u.pair.paired = copy(cur->u.pair.paired);
	    cur->u.pair.vr = chunkalloc(sizeof( struct vr [2]));
	    memcpy(cur->u.pair.vr,base->u.pair.vr,sizeof(struct vr [2]));
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    cur->u.pair.vr[0].adjust = ValDevTabCopy(base->u.pair.vr[0].adjust);
	    cur->u.pair.vr[1].adjust = ValDevTabCopy(base->u.pair.vr[1].adjust);
#endif
	} else if ( cur->type==pst_lcaret ) {
	    cur->u.lcaret.carets = galloc(cur->u.lcaret.cnt*sizeof(uint16));
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
	    LogError( "No matching AnchorClass for %s", base->anchor->name);
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

static void AnchorClassesAdd(SplineFont *into, SplineFont *from) {
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
	    cur->script_lang_index = FixupSLI(cur->script_lang_index,from,into);
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

static void FPSTsAdd(SplineFont *into, SplineFont *from) {
    FPST *fpst, *nfpst, *last;

    last = NULL;
    if ( into->possub!=NULL )
	for ( last = into->possub; last->next!=NULL; last=last->next );
    for ( fpst = from->possub; fpst!=NULL; fpst=fpst->next ) {
	nfpst = FPSTCopy(fpst);
	nfpst->script_lang_index = FixupSLI(nfpst->script_lang_index,from,into);
	if ( last==NULL )
	    into->possub = nfpst;
	else
	    last->next = nfpst;
	last = nfpst;
    }
}

static void ASMsAdd(SplineFont *into, SplineFont *from) {
    ASM *sm, *nsm, *last;
    int i;

    last = NULL;
    if ( into->sm!=NULL )
	for ( last = into->sm; last->next!=NULL; last=last->next );
    for ( sm = from->sm; sm!=NULL; sm=sm->next ) {
	nsm = chunkalloc(sizeof(ASM));
	*nsm = *sm;
	nsm->classes = galloc(nsm->class_cnt*sizeof(char *));
	for ( i=0; i<nsm->class_cnt; ++i )
	    nsm->classes[i] = copy(sm->classes[i]);
	nsm->state = galloc(nsm->class_cnt*nsm->state_cnt*sizeof(struct asm_state));
	memcpy(nsm->state,sm->state,nsm->class_cnt*nsm->state_cnt*sizeof(struct asm_state));
	if ( nsm->type == asm_kern ) {
	    for ( i=nsm->class_cnt*nsm->state_cnt-1; i>=0; --i ) {
		nsm->state[i].u.kern.kerns = galloc(nsm->state[i].u.kern.kcnt*sizeof(int16));
		memcpy(nsm->state[i].u.kern.kerns,sm->state[i].u.kern.kerns,nsm->state[i].u.kern.kcnt*sizeof(int16));
	    }
	} else if ( nsm->type == asm_insert ) {
	    for ( i=nsm->class_cnt*nsm->state_cnt-1; i>=0; --i ) {
		nsm->state[i].u.insert.mark_ins = copy(sm->state[i].u.insert.mark_ins);
		nsm->state[i].u.insert.cur_ins = copy(sm->state[i].u.insert.cur_ins);
	    }
	}
    }
}

static KernClass *_KernClassCopy(KernClass *kc,SplineFont *into,SplineFont *from) {
    KernClass *nkc;

    nkc = KernClassCopy(kc);
    nkc->sli = FixupSLI(nkc->sli,from,into);
return( nkc );
}

static void KernClassesAdd(SplineFont *into, SplineFont *from) {
    /* Is this a good idea? We could end up with two kern classes that do */
    /*  the same thing and strange sets of slis so that they didn't all fit */
    /*  in one lookup... */
    KernClass *kc, *last, *cur;

    last = NULL;
    if ( into->kerns!=NULL )
	for ( last = into->kerns; last->next!=NULL; last=last->next );
    for ( kc=from->kerns; kc!=NULL; kc=kc->next ) {
	cur = _KernClassCopy(kc,into,from);
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
	cur = _KernClassCopy(kc,into,from);
	if ( last==NULL )
	    into->vkerns = cur;
	else
	    last->next = cur;
	last = cur;
    }
}

static struct altuni *AltUniCopy(struct altuni *altuni,SplineFont *noconflicts) {
    struct altuni *head=NULL, *last=NULL, *cur;

    while ( altuni!=NULL ) {
	if ( noconflicts==NULL || SFGetChar(noconflicts,altuni->unienc,NULL)==NULL ) {
	    cur = chunkalloc(sizeof(struct altuni));
	    cur->unienc = altuni->unienc;
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

SplineChar *SplineCharCopy(SplineChar *sc,SplineFont *into) {
    SplineChar *nsc = SplineCharCreate();
#ifdef FONTFORGE_CONFIG_TYPE3
    Layer *layers = nsc->layers;
    int layer;

    *nsc = *sc;
    if ( sc->layer_cnt!=2 )
	layers = grealloc(layers,sc->layer_cnt*sizeof(Layer));
    memcpy(layers,sc->layers,sc->layer_cnt*sizeof(Layer));
    nsc->layers = layers;
    for ( layer = ly_back; layer<sc->layer_cnt; ++layer ) {
	layers[layer].splines = SplinePointListCopy(layers[layer].splines);
	layers[layer].refs = RefCharsCopy(layers[layer].refs);
	layers[layer].images = ImageListCopy(layers[layer].images);
	layers[layer].undoes = NULL;
	layers[layer].redoes = NULL;
    }
#else
    *nsc = *sc;
    nsc->layers[ly_fore].splines = SplinePointListCopy(nsc->layers[ly_fore].splines);
    nsc->layers[ly_fore].refs = RefCharsCopy(nsc->layers[ly_fore].refs);
    nsc->layers[ly_back].splines = SplinePointListCopy(nsc->layers[ly_back].splines);
    nsc->layers[ly_back].images = ImageListCopy(nsc->layers[ly_back].images);
#endif
    nsc->parent = into;
    nsc->orig_pos = -2;
    nsc->name = copy(sc->name);
    nsc->hstem = StemInfoCopy(nsc->hstem);
    nsc->vstem = StemInfoCopy(nsc->vstem);
    nsc->dstem = DStemInfoCopy(nsc->dstem);
    nsc->md = MinimumDistanceCopy(nsc->md);
    nsc->anchor = AnchorPointsDuplicate(nsc->anchor,nsc);
    nsc->views = NULL;
    nsc->changed = true;
    nsc->dependents = NULL;		/* Fix up later when we know more */
    nsc->layers[ly_fore].undoes = nsc->layers[ly_back].undoes = NULL;
    nsc->layers[ly_fore].redoes = nsc->layers[ly_back].redoes = NULL;
    if ( nsc->ttf_instrs_len!=0 ) {
	nsc->ttf_instrs = galloc(nsc->ttf_instrs_len);
	memcpy(nsc->ttf_instrs,sc->ttf_instrs,nsc->ttf_instrs_len);
    }
    nsc->kerns = NULL;
    nsc->possub = PSTCopy(nsc->possub,nsc,sc->parent);
    nsc->altuni = AltUniCopy(nsc->altuni,into);
    if ( sc->parent!=NULL && into->order2!=sc->parent->order2 )
	SCConvertOrder(nsc,into->order2);
return(nsc);
}

static KernPair *KernsCopy(KernPair *kp,int *mapping,SplineFont *into,SplineFont *from) {
    KernPair *head = NULL, *last=NULL, *new;
    int index;

    while ( kp!=NULL ) {
	if ( (index=mapping[kp->sc->orig_pos])>=0 && index<into->glyphcnt &&
		into->glyphs[index]!=NULL ) {
	    new = chunkalloc(sizeof(KernPair));
	    new->off = kp->off;
	    new->sli = FixupSLI(kp->sli,from,into);
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
    BDFChar *nbc = galloc(sizeof( BDFChar ));

    *nbc = *bc;
    nbc->views = NULL;
    nbc->selection = NULL;
    nbc->undoes = NULL;
    nbc->redoes = NULL;
    nbc->bitmap = galloc((nbc->ymax-nbc->ymin+1)*nbc->bytes_per_line);
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
	    f_bdf = f_bdf->next;
	}
    }
}

#define GN_HSIZE	257

struct glyphnamebucket {
    SplineChar *sc;
    struct glyphnamebucket *next;
};

struct glyphnamehash {
    struct glyphnamebucket *table[GN_HSIZE];
};

#ifndef __GNUC__
# define __inline__
#endif

static __inline__ int hashname(const char *pt) {
    int val = 0;

    while ( *pt ) {
	val = (val<<3)|((val>>29)&0x7);
	val ^= (unsigned char)(*pt-'!');
	pt++;
    }
    val ^= (val>>16);
    val &= 0xffff;
    val %= GN_HSIZE;
return( val );
}

static void _GlyphHashFree(SplineFont *sf) {
    struct glyphnamebucket *test, *next;
    int i;

    if ( sf->glyphnames==NULL )
return;
    for ( i=0; i<GN_HSIZE; ++i ) {
	for ( test = sf->glyphnames->table[i]; test!=NULL; test = next ) {
	    next = test->next;
	    chunkfree(test,sizeof(struct glyphnamebucket));
	}
    }
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
    sf->glyphnames = gnh = gcalloc(1,sizeof(*gnh));
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

static SplineChar *SFHashName(SplineFont *sf,const char *name) {
    struct glyphnamebucket *test;

    if ( sf->glyphnames==NULL )
	GlyphHashCreate(sf);

    for ( test=sf->glyphnames->table[hashname(name)]; test!=NULL; test = test->next )
	if ( strcmp(test->sc->name,name)==0 )
return( test->sc );

return( NULL );
}

/* Find the position in the glyph list where this code point/name is found. */
/*  Returns -1 else on error */
int SFFindGID(SplineFont *sf, int unienc, const char *name ) {
    struct altuni *altuni;
    int gid;
    SplineChar *sc;

    if ( unienc!=-1 ) {
	for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( sf->glyphs[gid]!=NULL ) {
	    if ( sf->glyphs[gid]->unicodeenc == unienc )
return( gid );
	    for ( altuni = sf->glyphs[gid]->altuni; altuni!=NULL; altuni=altuni->next ) {
		if ( altuni->unienc == unienc )
return( gid );
	    }
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

    if ( (map->enc->is_custom || map->enc->is_compact ||
	    map->enc->is_original) && unienc!=-1 ) {
	if ( unienc<map->enccount && map->map[unienc]!=-1 &&
		sf->glyphs[map->map[unienc]]!=NULL &&
		sf->glyphs[map->map[unienc]]->unicodeenc==unienc )
	    index = unienc;
	else for ( index = map->enccount-1; index>=0; --index ) {
	    if ( (pos = map->map[index])!=-1 && sf->glyphs[pos]!=NULL &&
		    sf->glyphs[pos]->unicodeenc==unienc )
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
			sf->glyphs[pos]->unicodeenc==unienc )
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
    int j,ret;
    struct cidmap *cidmap;

    if ( sf->cidmaster!=NULL || sf->subfontcnt!=0 ) {
	if ( sf->cidmaster!=NULL )
	    sf=sf->cidmaster;
	cidmap = FindCidMap(sf->cidregistry,sf->ordering,sf->supplement,sf);
	ret = NameUni2CID(cidmap,unienc,name);
	if ( ret!=-1 )
return( ret );
    }

    if ( sf->subfonts==NULL && sf->cidmaster==NULL )
return( SFFindGID(sf,unienc,name));

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
    int ind;
    int j;

    ind = SFCIDFindCID(sf,unienc,name);
    if ( ind==-1 )
return( NULL );

    if ( sf->subfonts==NULL && sf->cidmaster==NULL )
return( sf->glyphs[ind]);

    if ( sf->cidmaster!=NULL )
	sf=sf->cidmaster;

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
	sc = SplineCharCreate();
#ifdef FONTFORGE_CONFIG_TYPE3
	if ( sf->strokedfont ) {
	    sc->layers[ly_fore].dostroke = true;
	    sc->layers[ly_fore].dofill = false;
	}
#endif
	sc->unicodeenc = unienc;
	if ( name!=NULL )
	    sc->name = copy(name);
	else {
	    char buffer[40];
	    sprintf(buffer,"glyph%d", sf->glyphcnt);
	    sc->name = copy(buffer);
	}
	SFAddGlyphAndEncode(sf,sc,NULL,-1);
	SCLigDefault(sc);
    }
return( sc );
}

static int _SFFindExistingSlot(SplineFont *sf, int unienc, const char *name ) {
    int gid = -1;
    struct altuni *altuni;

    if ( unienc!=-1 ) {
	for ( gid=sf->glyphcnt-1; gid>=0; --gid ) if ( sf->glyphs[gid]!=NULL ) {
	    if ( sf->glyphs[gid]->unicodeenc==unienc )
	break;
	    for ( altuni=sf->glyphs[gid]->altuni ; altuni!=NULL && altuni->unienc!=unienc ;
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
    FontView *fvs;

    sc->orig_pos = i;
    sc->parent = sf;
 retry:
    for ( ref=sc->layers[ly_fore].refs; ref!=NULL; ref=ref->next ) {
	/* The sc in the ref is from the old font. It's got to be in the */
	/*  new font too (was either already there or just got copied) */
	ref->orig_pos =  SFFindExistingSlot(sf,ref->sc->unicodeenc,ref->sc->name);
	if ( ref->orig_pos==-1 ) {
	    IError("Bad reference, can't fix it up");
	    if ( ref==sc->layers[ly_fore].refs ) {
		sc->layers[ly_fore].refs = ref->next;
 goto retry;
	    } else {
		for ( prev=sc->layers[ly_fore].refs; prev->next!=ref; prev=prev->next );
		prev->next = ref->next;
		chunkfree(ref,sizeof(*ref));
		ref = prev;
	    }
	} else {
	    ref->sc = sf->glyphs[ref->orig_pos];
	    if ( ref->sc->orig_pos==-2 )
		MFixupSC(sf,ref->sc,ref->orig_pos);
	    SCReinstanciateRefChar(sc,ref);
	    SCMakeDependent(sc,ref->sc);
	}
    }
    /* I shan't automagically generate bitmaps for any bdf fonts */
    /*  but I have copied over the ones which match */
    for ( fvs=sf->fv; fvs!=NULL; fvs=fvs->nextsame ) if ( fvs->filled!=NULL )
	fvs->filled->glyphs[i] = SplineCharRasterize(sc,fvs->filled->pixelsize);
}

static void MergeFixupRefChars(SplineFont *sf) {
    int i;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL && sf->glyphs[i]->orig_pos==-2 ) {
	MFixupSC(sf,sf->glyphs[i], i);
    }
}

int SFHasChar(SplineFont *sf, int unienc, const char *name ) {
    SplineChar *sc = SFGetChar(sf,unienc,name);

return( SCWorthOutputting(sc) );
}

static void FVMergeRefigureMapSel(FontView *fv,SplineFont *into,SplineFont *o_sf,
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
		map->backmap = grealloc(map->backmap,(into->glyphcnt+cnt)*sizeof(int));
		map->backmax = into->glyphcnt+cnt;
	    }
	    memset(map->backmap+into->glyphcnt,-1,cnt*sizeof(int));
	    if ( map->enccount+extras>map->encmax ) {
		map->map = grealloc(map->map,(map->enccount+extras)*sizeof(int));
		map->encmax = map->enccount+extras;
	    }
	    memset(map->map+map->enccount,-1,extras*sizeof(int));
	    map->enccount += extras;
	}
    }
    if ( extras!=0 ) {
	fv->selected = grealloc(fv->selected,map->enccount);
	memset(fv->selected+map->enccount-extras,0,extras);
    }
}

static void _MergeFont(SplineFont *into,SplineFont *other) {
    int i,cnt, doit, emptypos, index, k;
    SplineFont *o_sf, *bitmap_into;
    BDFFont *bdf;
    FontView *fvs;
    int *mapping;

    emptypos = into->glyphcnt;
    mapping = galloc(other->glyphcnt*sizeof(int));
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
		    into->glyphs[index] = SplineCharCopy(o_sf->glyphs[i],into);
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
		    into->glyphs = grealloc(into->glyphs,(emptypos+cnt)*sizeof(SplineChar *));
		    memset(into->glyphs+emptypos,0,cnt*sizeof(SplineChar *));
		    for ( bdf = bitmap_into->bitmaps; bdf!=NULL; bdf=bdf->next )
			if ( emptypos+cnt > bdf->glyphcnt ) {
			    bdf->glyphs = grealloc(bdf->glyphs,(emptypos+cnt)*sizeof(BDFChar *));
			    memset(bdf->glyphs+emptypos,0,cnt*sizeof(BDFChar *));
			    bdf->glyphmax = bdf->glyphcnt = emptypos+cnt;
			}
		    for ( fvs = into->fv; fvs!=NULL; fvs=fvs->nextsame )
			if ( fvs->filled!=NULL ) {
			    fvs->filled->glyphs = grealloc(fvs->filled->glyphs,(emptypos+cnt)*sizeof(BDFChar *));
			    memset(fvs->filled->glyphs+emptypos,0,cnt*sizeof(BDFChar *));
			    fvs->filled->glyphmax = fvs->filled->glyphcnt = emptypos+cnt;
			}
		    for ( fvs = into->fv; fvs!=NULL; fvs=fvs->nextsame )
			if ( fvs->sf == into )
			    FVMergeRefigureMapSel(fvs,into,o_sf,mapping,emptypos,cnt);
		    into->glyphcnt = into->glyphmax = emptypos+cnt;
		}
	    }
	    ++k;
	} while ( k<other->subfontcnt );
    }
    for ( i=0; i<other->glyphcnt; ++i ) if ( (index=mapping[i])!=-1 )
	into->glyphs[index]->kerns = KernsCopy(other->glyphs[i]->kerns,mapping,into,other);
    free(mapping);
    GlyphHashFree(into);
    MergeFixupRefChars(into);
    if ( other->fv==NULL )
	SplineFontFree(other);
    into->changed = true;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    FontViewReformatAll(into);
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    GlyphHashFree(into);
}

static void __MergeFont(SplineFont *into,SplineFont *other) {

    AnchorClassesAdd(into,other);
    FPSTsAdd(into,other);
    ASMsAdd(into,other);
    KernClassesAdd(into,other);

    _MergeFont(into,other);
}

static void CIDMergeFont(SplineFont *into,SplineFont *other) {
    int i,j,k;
    SplineFont *i_sf, *o_sf;
    FontView *fvs;

    AnchorClassesAdd(into,other);
    FPSTsAdd(into,other);
    ASMsAdd(into,other);
    KernClassesAdd(into,other);

    k = 0;
    do {
	o_sf = other->subfonts[k];
	i_sf = into->subfonts[k];
	for ( i=o_sf->glyphcnt-1; i>=0 && o_sf->glyphs[i]==NULL; --i );
	if ( i>=i_sf->glyphcnt ) {
	    i_sf->glyphs = grealloc(i_sf->glyphs,(i+1)*sizeof(SplineChar *));
	    for ( j=i_sf->glyphcnt; j<=i; ++j )
		i_sf->glyphs[j] = NULL;
	    for ( fvs = i_sf->fv; fvs!=NULL; fvs=fvs->nextsame )
		if ( fvs->sf==i_sf ) {
		    fvs->selected = grealloc(fvs->selected,i+1);
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
		i_sf->glyphs[i] = SplineCharCopy(o_sf->glyphs[i],i_sf);
		if ( into->bitmaps!=NULL && other->bitmaps!=NULL )
		    BitmapsCopy(into,other,i,i);
	    }
	}
	MergeFixupRefChars(i_sf);
	++k;
    } while ( k<other->subfontcnt );
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    FontViewReformatAll(into);
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    into->changed = true;
    GlyphHashFree(into);
}

void MergeFont(FontView *fv,SplineFont *other) {

    if ( fv->sf==other ) {
#if defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("Merging Problem"),_("Merging a font with itself achieves nothing"));
#else
	gwwv_post_error(_("Merging Problem"),_("Merging a font with itself achieves nothing"));
#endif
return;
    }
    if ( fv->sf->cidmaster!=NULL && other->subfonts!=NULL &&
	    (strcmp(fv->sf->cidmaster->cidregistry,other->cidregistry)!=0 ||
	     strcmp(fv->sf->cidmaster->ordering,other->ordering)!=0 ||
	     fv->sf->cidmaster->supplement<other->supplement ||
	     fv->sf->cidmaster->subfontcnt<other->subfontcnt )) {
#if defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("Merging Problem"),_("When merging two CID keyed fonts, they must have the same Registry and Ordering, and the font being merged into (the mergee) must have a supplement which is at least as recent as the other's. Furthermore the mergee must have at least as many subfonts as the merger."));
#else
	gwwv_post_error(_("Merging Problem"),_("When merging two CID keyed fonts, they must have the same Registry and Ordering, and the font being merged into (the mergee) must have a supplement which is at least as recent as the other's. Furthermore the mergee must have at least as many subfonts as the merger."));
#endif
return;
    }
    /* Ok. when merging CID fonts... */
    /*  If fv is normal and other is CID then just flatten other and merge everything into fv */
    /*  If fv is CID and other is normal then merge other into the currently active font */
    /*  If both are CID then merge each subfont seperately */
    if ( fv->sf->cidmaster!=NULL && other->subfonts!=NULL )
	CIDMergeFont(fv->sf->cidmaster,other);
    else
	__MergeFont(fv->sf,other);
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static void MergeAskFilename(FontView *fv) {
    char *filename = GetPostscriptFontName(NULL,true);
    SplineFont *sf;
    char *eod, *fpt, *file, *full;

    if ( filename==NULL )
return;
    eod = strrchr(filename,'/');
    *eod = '\0';
    file = eod+1;
    do {
	fpt = strstr(file,"; ");
	if ( fpt!=NULL ) *fpt = '\0';
	full = galloc(strlen(filename)+1+strlen(file)+1);
	strcpy(full,filename); strcat(full,"/"); strcat(full,file);
	sf = LoadSplineFont(full,0);
	if ( sf!=NULL && sf->fv==NULL )
	    EncMapFree(sf->map);
	free(full);
	if ( sf==NULL )
	    /* Do Nothing */;
	else if ( sf->fv==fv )
	    gwwv_post_error(_("Merging Problem"),_("Merging a font with itself achieves nothing"));
	else
	    MergeFont(fv,sf);
	file = fpt+2;
    } while ( fpt!=NULL );
    free(filename);
}

GTextInfo *BuildFontList(FontView *except) {
    FontView *fv;
    int cnt=0;
    GTextInfo *tf;

    for ( fv=fv_list; fv!=NULL; fv = fv->next )
	++cnt;
    tf = gcalloc(cnt+3,sizeof(GTextInfo));
    for ( fv=fv_list, cnt=0; fv!=NULL; fv = fv->next ) if ( fv!=except ) {
	tf[cnt].fg = tf[cnt].bg = COLOR_DEFAULT;
	tf[cnt].text = (unichar_t *) fv->sf->fontname;
	tf[cnt].text_is_1byte = true;
	++cnt;
    }
    tf[cnt++].line = true;
    tf[cnt].fg = tf[cnt].bg = COLOR_DEFAULT;
    tf[cnt].text_is_1byte = true;
    tf[cnt++].text = (unichar_t *) _("Other ...");
return( tf );
}

void TFFree(GTextInfo *tf) {
/*
    int i;

    for ( i=0; tf[i].text!=NULL || tf[i].line ; ++i )
	if ( !tf[i].text_in_resource )
	    free( tf[i].text );
*/
    free(tf);
}

struct mf_data {
    int done;
    FontView *fv;
    GGadget *other;
    GGadget *amount;
};

static int MF_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	struct mf_data *d = GDrawGetUserData(gw);
	int i, index = GGadgetGetFirstListSelectedItem(d->other);
	FontView *fv;
	for ( i=0, fv=fv_list; fv!=NULL; fv=fv->next ) {
	    if ( fv==d->fv )
	continue;
	    if ( i==index )
	break;
	    ++i;
	}
	if ( fv==NULL )
	    MergeAskFilename(d->fv);
	else
	    MergeFont(d->fv,fv->sf);
	d->done = true;
    }
return( true );
}

static int MF_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	struct mf_data *d = GDrawGetUserData(gw);
	d->done = true;
    }
return( true );
}

static int mv_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct mf_data *d = GDrawGetUserData(gw);
	d->done = true;
    } else if ( event->type == et_char ) {
return( false );
    }
return( true );
}

void FVMergeFonts(FontView *fv) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[6];
    GTextInfo label[6];
    struct mf_data d;
    char buffer[80];

    /* If there's only one font loaded, then it's the current one, and there's*/
    /*  no point asking the user if s/he wants to merge any of the loaded */
    /*  fonts, go directly to searching the disk */
    if ( fv_list==fv && fv_list->next==NULL )
	MergeAskFilename(fv);
    else {
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title = _("Merge Fonts...");
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,150));
	pos.height = GDrawPointsToPixels(NULL,88);
	gw = GDrawCreateTopWindow(NULL,&pos,mv_e_h,&d,&wattrs);

	memset(&label,0,sizeof(label));
	memset(&gcd,0,sizeof(gcd));

	sprintf( buffer, _("Font to merge into %.20s"), fv->sf->fontname );
	label[0].text = (unichar_t *) buffer;
	label[0].text_is_1byte = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 12; gcd[0].gd.pos.y = 6; 
	gcd[0].gd.flags = gg_visible | gg_enabled;
	gcd[0].creator = GLabelCreate;

	gcd[1].gd.pos.x = 15; gcd[1].gd.pos.y = 21;
	gcd[1].gd.pos.width = 120;
	gcd[1].gd.flags = gg_visible | gg_enabled;
	gcd[1].gd.u.list = BuildFontList(fv);
	gcd[1].gd.label = &gcd[1].gd.u.list[0];
	gcd[1].gd.u.list[0].selected = true;
	gcd[1].creator = GListButtonCreate;

	gcd[2].gd.pos.x = 15-3; gcd[2].gd.pos.y = 55-3;
	gcd[2].gd.pos.width = -1; gcd[2].gd.pos.height = 0;
	gcd[2].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[2].text = (unichar_t *) _("_OK");
	label[2].text_is_1byte = true;
	label[2].text_in_resource = true;
	gcd[2].gd.mnemonic = 'O';
	gcd[2].gd.label = &label[2];
	gcd[2].gd.handle_controlevent = MF_OK;
	gcd[2].creator = GButtonCreate;

	gcd[3].gd.pos.x = -15; gcd[3].gd.pos.y = 55;
	gcd[3].gd.pos.width = -1; gcd[3].gd.pos.height = 0;
	gcd[3].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[3].text = (unichar_t *) _("_Cancel");
	label[3].text_is_1byte = true;
	label[3].text_in_resource = true;
	gcd[3].gd.label = &label[3];
	gcd[3].gd.mnemonic = 'C';
	gcd[3].gd.handle_controlevent = MF_Cancel;
	gcd[3].creator = GButtonCreate;

	GGadgetsCreate(gw,gcd);

	memset(&d,'\0',sizeof(d));
	d.other = gcd[1].ret;
	d.fv = fv;

	GWidgetHidePalettes();
	GDrawSetVisible(gw,true);
	while ( !d.done )
	    GDrawProcessOneEvent(NULL);
	GDrawDestroyWindow(gw);
	TFFree(gcd[1].gd.u.list);
    }
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

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
	    *cur = *base;
	    cur->orig_pos = cur->sc->orig_pos;
	    for ( i=0; i<6; ++i )
		cur->transform[i] = base->transform[i] + amount*(other->transform[i]-base->transform[i]);
	    cur->layers[0].splines = NULL;
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
    if ( cur->first==NULL )
	cur->first = p;
    else
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

static SplineSet *InterpSplineSets(SplineSet *base, SplineSet *other, real amount, SplineChar *sc) {
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

static KernPair *InterpKerns(KernPair *kp1, KernPair *kp2, real amount, SplineFont *new) {
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
	    nkp->sli = kp1->sli;
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

#ifdef FONTFORGE_CONFIG_TYPE3
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
	LogError( "Different settings on whether to fill in layer %d of %s\n", lc, sc->name );
    if ( base->fill_brush.col==COLOR_INHERITED && other->fill_brush.col==COLOR_INHERITED )
	to->fill_brush.col = COLOR_INHERITED;
    else if ( base->fill_brush.col!=COLOR_INHERITED && other->fill_brush.col!=COLOR_INHERITED )
	to->fill_brush.col = InterpColor( base->fill_brush.col,other->fill_brush.col, amount );
    else
	LogError( "Different settings on whether to inherit fill color in layer %d of %s\n", lc, sc->name );
    if ( base->fill_brush.opacity<0 && other->fill_brush.opacity<0 )
	to->fill_brush.opacity = WIDTH_INHERITED;
    else if ( base->fill_brush.opacity>=0 && other->fill_brush.opacity>=0 )
	to->fill_brush.opacity = base->fill_brush.opacity + amount*(other->fill_brush.opacity-base->fill_brush.opacity);
    else
	LogError( "Different settings on whether to inherit fill opacity in layer %d of %s\n", lc, sc->name );
    if ( base->stroke_pen.brush.col==COLOR_INHERITED && other->stroke_pen.brush.col==COLOR_INHERITED )
	to->stroke_pen.brush.col = COLOR_INHERITED;
    else if ( base->stroke_pen.brush.col!=COLOR_INHERITED && other->stroke_pen.brush.col!=COLOR_INHERITED )
	to->stroke_pen.brush.col = InterpColor( base->stroke_pen.brush.col,other->stroke_pen.brush.col, amount );
    else
	LogError( "Different settings on whether to inherit fill color in layer %d of %s\n", lc, sc->name );
    if ( base->stroke_pen.brush.opacity<0 && other->stroke_pen.brush.opacity<0 )
	to->stroke_pen.brush.opacity = WIDTH_INHERITED;
    else if ( base->stroke_pen.brush.opacity>=0 && other->stroke_pen.brush.opacity>=0 )
	to->stroke_pen.brush.opacity = base->stroke_pen.brush.opacity + amount*(other->stroke_pen.brush.opacity-base->stroke_pen.brush.opacity);
    else
	LogError( "Different settings on whether to inherit stroke opacity in layer %d of %s\n", lc, sc->name );
    if ( base->stroke_pen.width<0 && other->stroke_pen.width<0 )
	to->stroke_pen.width = WIDTH_INHERITED;
    else if ( base->stroke_pen.width>=0 && other->stroke_pen.width>=0 )
	to->stroke_pen.width = base->stroke_pen.width + amount*(other->stroke_pen.width-base->stroke_pen.width);
    else
	LogError( "Different settings on whether to inherit stroke width in layer %d of %s\n", lc, sc->name );
    if ( base->stroke_pen.linecap==other->stroke_pen.linecap )
	to->stroke_pen.linecap = base->stroke_pen.linecap;
    else
	LogError( "Different settings on stroke linecap in layer %d of %s\n", lc, sc->name );
    if ( base->stroke_pen.linejoin==other->stroke_pen.linejoin )
	to->stroke_pen.linejoin = base->stroke_pen.linejoin;
    else
	LogError( "Different settings on stroke linejoin in layer %d of %s\n", lc, sc->name );

    to->splines = InterpSplineSets(base->splines,other->splines,amount,sc);
    to->refs = InterpRefs(base->refs,other->refs,amount,sc);
    if ( base->images!=NULL || other->images!=NULL )
	LogError( "I can't even imagine how to attempt to interpolate images in layer %d of %s\n", lc, sc->name );
}
#endif

static void InterpolateChar(SplineFont *new, int orig_pos, SplineChar *base, SplineChar *other, real amount) {
    SplineChar *sc;

    if ( base==NULL || other==NULL )
return;
    sc = SplineCharCreate();
    sc->orig_pos = orig_pos;
    sc->unicodeenc = base->unicodeenc;
    new->glyphs[orig_pos] = sc;
    if ( orig_pos+1>new->glyphcnt )
	new->glyphcnt = orig_pos+1;
    sc->parent = new;
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
#ifdef FONTFORGE_CONFIG_TYPE3
    if ( base->parent->multilayer && other->parent->multilayer ) {
	int lc = base->layer_cnt,i;
	if ( lc!=other->layer_cnt ) {
	    LogError( "Different numbers of layers in %s\n", base->name );
	    if ( other->layer_cnt<lc ) lc = other->layer_cnt;
	}
	if ( lc>2 ) {
	    sc->layers = grealloc(sc->layers,lc*sizeof(Layer));
	    for ( i=ly_fore+1; i<lc; ++i )
		LayerDefault(&sc->layers[i]);
	}
	for ( i=ly_fore; i<lc; ++i )
	    LayerInterpolate(&sc->layers[i],&base->layers[i],&other->layers[i],amount,sc,i);
    } else
#endif
    {
	sc->layers[ly_fore].splines = InterpSplineSets(base->layers[ly_fore].splines,other->layers[ly_fore].splines,amount,sc);
	sc->layers[ly_fore].refs = InterpRefs(base->layers[ly_fore].refs,other->layers[ly_fore].refs,amount,sc);
    }
    sc->changedsincelasthinted = true;
    sc->widthset = base->widthset;
    sc->glyph_class = base->glyph_class;
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
	SCReinstanciateRefChar(sc,ref);
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
    int i, index;

    if ( base==other ) {
#if defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("Interpolating Problem"),_("Interpolating a font with itself achieves nothing"));
#else
	gwwv_post_error(_("Interpolating Problem"),_("Interpolating a font with itself achieves nothing"));
#endif
return( NULL );
    } else if ( base->order2!=other->order2 ) {
#if defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("Interpolating Problem"),_("Interpolating between fonts with different spline orders (i.e. between postscript and truetype)"));
#else
	gwwv_post_error(_("Interpolating Problem"),_("Interpolating between fonts with different spline orders (i.e. between postscript and truetype)"));
#endif
return( NULL );
    }
#ifdef FONTFORGE_CONFIG_TYPE3
    else if ( base->multilayer && other->multilayer ) {
# if defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("Interpolating Problem"),_("Interpolating between fonts with different editing types (ie. between type3 and type1)"));
# else
	gwwv_post_error(_("Interpolating Problem"),_("Interpolating between fonts with different editing types (ie. between type3 and type1)"));
# endif
return( NULL );
    }
#endif
    new = SplineFontBlank(base->glyphcnt);
    new->ascent = base->ascent + amount*(other->ascent-base->ascent);
    new->descent = base->descent + amount*(other->descent-base->descent);
    for ( i=0; i<base->glyphcnt; ++i ) if ( base->glyphs[i]!=NULL ) {
	index = SFFindExistingSlot(other,base->glyphs[i]->unicodeenc,base->glyphs[i]->name);
	if ( index!=-1 && other->glyphs[index]!=NULL ) {
	    InterpolateChar(new,i,base->glyphs[i],other->glyphs[index],amount);
	    if ( new->glyphs[i]!=NULL )
		new->glyphs[i]->kerns = InterpKerns(base->glyphs[i]->kerns,
			other->glyphs[index]->kerns,amount,new);
	}
    }
    InterpFixupRefChars(new);
    new->changed = true;
    new->map = EncMapFromEncoding(new,enc);
return( new );
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static void InterAskFilename(FontView *fv, real amount) {
    char *filename = GetPostscriptFontName(NULL,false);
    SplineFont *sf;

    if ( filename==NULL )
return;
    sf = LoadSplineFont(filename,0);
    if ( sf!=NULL && sf->fv==NULL )
	EncMapFree(sf->map);
    free(filename);
    if ( sf==NULL )
return;
    FontViewCreate(InterpolateFont(fv->sf,sf,amount,fv->map->enc));
}

#define CID_Amount	1000
static double last_amount=50;

static int IF_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	struct mf_data *d = GDrawGetUserData(gw);
	int i, index = GGadgetGetFirstListSelectedItem(d->other);
	FontView *fv;
	int err=false;
	real amount;
	
	amount = GetReal8(gw,CID_Amount, _("Amount"),&err);
	if ( err )
return( true );
	last_amount = amount;
	for ( i=0, fv=fv_list; fv!=NULL; fv=fv->next ) {
	    if ( fv==d->fv )
	continue;
	    if ( i==index )
	break;
	    ++i;
	}
	if ( fv==NULL )
	    InterAskFilename(d->fv,last_amount/100.0);
	else
	    FontViewCreate(InterpolateFont(d->fv->sf,fv->sf,last_amount/100.0,d->fv->map->enc));
	d->done = true;
    }
return( true );
}

void FVInterpolateFonts(FontView *fv) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[8];
    GTextInfo label[8];
    struct mf_data d;
    char buffer[80]; char buf2[30];

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Interpolate Fonts...");
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,200));
    pos.height = GDrawPointsToPixels(NULL,118);
    gw = GDrawCreateTopWindow(NULL,&pos,mv_e_h,&d,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    sprintf( buffer, _("Interpolating between %.20s and:"), fv->sf->fontname );
    label[0].text = (unichar_t *) buffer;
    label[0].text_is_1byte = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 12; gcd[0].gd.pos.y = 6; 
    gcd[0].gd.flags = gg_visible | gg_enabled;
    gcd[0].creator = GLabelCreate;

    gcd[1].gd.pos.x = 20; gcd[1].gd.pos.y = 21;
    gcd[1].gd.pos.width = 110;
    gcd[1].gd.flags = gg_visible | gg_enabled;
    gcd[1].gd.u.list = BuildFontList(fv);
    if ( gcd[1].gd.u.list[0].text!=NULL ) {
	gcd[1].gd.label = &gcd[1].gd.u.list[0];
	gcd[1].gd.u.list[0].selected = true;
    } else {
	gcd[1].gd.label = &gcd[1].gd.u.list[1];
	gcd[1].gd.u.list[1].selected = true;
	gcd[1].gd.flags = gg_visible;
    }
    gcd[1].creator = GListButtonCreate;

    sprintf( buf2, "%g", last_amount );
    label[2].text = (unichar_t *) buf2;
    label[2].text_is_1byte = true;
    gcd[2].gd.pos.x = 20; gcd[2].gd.pos.y = 51;
    gcd[2].gd.pos.width = 40;
    gcd[2].gd.flags = gg_visible | gg_enabled;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.cid = CID_Amount;
    gcd[2].creator = GTextFieldCreate;

    gcd[3].gd.pos.x = 5; gcd[3].gd.pos.y = 51+6;
    gcd[3].gd.flags = gg_visible | gg_enabled;
/* GT: The dialog looks like: */
/* GT:   Interpolating between <fontname> and: */
/* GT: <list of possible fonts> */
/* GT:   by  <50>% */
/* GT: So "by" means how much to interpolate. */
    label[3].text = (unichar_t *) _("by");
    label[3].text_is_1byte = true;
    gcd[3].gd.label = &label[3];
    gcd[3].creator = GLabelCreate;

    gcd[4].gd.pos.x = 20+40+3; gcd[4].gd.pos.y = 51+6;
    gcd[4].gd.flags = gg_visible | gg_enabled;
    label[4].text = (unichar_t *) "%";
    label[4].text_is_1byte = true;
    gcd[4].gd.label = &label[4];
    gcd[4].creator = GLabelCreate;

    gcd[5].gd.pos.x = 15-3; gcd[5].gd.pos.y = 85-3;
    gcd[5].gd.pos.width = -1; gcd[5].gd.pos.height = 0;
    gcd[5].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[5].text = (unichar_t *) _("_OK");
    label[5].text_is_1byte = true;
    label[5].text_in_resource = true;
    gcd[5].gd.mnemonic = 'O';
    gcd[5].gd.label = &label[5];
    gcd[5].gd.handle_controlevent = IF_OK;
    gcd[5].creator = GButtonCreate;

    gcd[6].gd.pos.x = -15; gcd[6].gd.pos.y = 85;
    gcd[6].gd.pos.width = -1; gcd[6].gd.pos.height = 0;
    gcd[6].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[6].text = (unichar_t *) _("_Cancel");
    label[6].text_is_1byte = true;
    label[6].text_in_resource = true;
    gcd[6].gd.label = &label[6];
    gcd[6].gd.mnemonic = 'C';
    gcd[6].gd.handle_controlevent = MF_Cancel;
    gcd[6].creator = GButtonCreate;

    GGadgetsCreate(gw,gcd);

    memset(&d,'\0',sizeof(d));
    d.other = gcd[1].ret;
    d.fv = fv;

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
    TFFree(gcd[1].gd.u.list);
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
