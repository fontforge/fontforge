/* Copyright (C) 2003-2006 by George Williams */
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
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
#include <gkeysym.h>
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

enum possub_type SFGTagUsed(struct gentagtype *gentags,uint32 tag) {
    int i;

    for ( i=0; i<gentags->tt_cur; ++i )
	if ( gentags->tagtype[i].tag==tag )
return( gentags->tagtype[i].type );

return( pst_null );
}

uint32 SFGenerateNewFeatureTag(struct gentagtype *gentags,enum possub_type type,
	uint32 suggested_tag) {
    char buf[8],tbuf[8];
    int i,j,k,l;

    if ( gentags->tt_cur >= gentags->tt_max ) {
	if ( gentags->tt_cur==0 ) {
	    gentags->tagtype = galloc((gentags->tt_max=32)*sizeof(struct tagtype));
	} else {
	    gentags->tt_max += 32;
	    gentags->tagtype = grealloc(gentags->tagtype,gentags->tt_max*sizeof(struct tagtype));
	}
    }
    for ( i=0; i<gentags->tt_cur && gentags->tagtype[i].type!=pst_null; ++i );
    if ( suggested_tag==0 ) {
	sprintf(buf, i<1000 ? "G%03d" : "%04d", i );
	gentags->tagtype[i].type = type;
	gentags->tagtype[i].tag = CHR(buf[0],buf[1],buf[2],buf[3]);
    } else {
	j = 0;
	tbuf[0] = suggested_tag>>24;
	tbuf[1] = (suggested_tag>>16)&0xff;
	tbuf[2] = (suggested_tag>>8)&0xff;
	tbuf[3] = suggested_tag&0xff;
	while ( SFGTagUsed(gentags,suggested_tag)!=pst_null ) {
	    sprintf(buf,"%d",j++);
	    k = strlen(buf);
	    for ( l=0; l<k; ++l )
		tbuf[3-l] = buf[k-l-1];
	    suggested_tag = CHR(tbuf[0],tbuf[1],tbuf[2],tbuf[3]);
	}
	gentags->tagtype[i].type = type;
	gentags->tagtype[i].tag = suggested_tag;
    }
    if ( i==gentags->tt_cur ) ++gentags->tt_cur;
return( gentags->tagtype[i].tag );
}

void SFFreeGenerateFeatureTag(struct gentagtype *gentags,uint32 tag) {
    int i;

    for ( i=0; i<gentags->tt_cur; ++i )
	if ( gentags->tagtype[i].tag==tag )
    break;
    if ( i==gentags->tt_cur-1 )
	--gentags->tt_cur;
    else if ( i<gentags->tt_cur ) {
	gentags->tagtype[i].type = pst_null;
	gentags->tagtype[i].tag = CHR(' ',' ',' ',' ');
    } else
	IError("Attempt to free an invalid generated tag" );
}

int SFRemoveThisFeatureTag(SplineFont *sf, uint32 tag, int sli, int flags) {
    /* if tag==0xffffffff treat as a wildcard (will match any tag) */
    /* if sli==SLI_UNKNOWN treat as a wildcard */
    /* if flags==-1 treat as a wildcard */
    int i,j,k;
    SplineChar *sc;
    PST *prev, *pst, *next;
    FPST *fprev, *fpst, *fnext;
    ASM *sprev, *sm, *snext;
    AnchorClass *ac, *aprev, *anext;
    KernPair *kp, *kprev, *knext;
    KernClass *kc, *cprev, *cnext;
    SplineFont *_sf;
    int any = false;

    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;
    _sf = sf;
    k = 0;
    do {
	sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[k];
	for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL ) {
	    for ( prev= NULL, pst = sc->possub; pst!=NULL; pst=next ) {
		next = pst->next;
		if ( ( tag==0xffffffff || tag==pst->tag ) &&
			( sli==SLI_UNKNOWN || sli==pst->script_lang_index ) &&
			( flags==-1 || flags==pst->flags )) {
		    if ( prev == NULL )
			sc->possub = next;
		    else
			prev->next = next;
		    pst->next = NULL;
		    PSTFree(pst);
		    any = true;
		} else
		    prev = pst;
	    }
	}
	++k;
    } while ( k<_sf->subfontcnt );

    for ( j=0; j<2; ++j ) {
	if ( tag==0xffffffff ||
		((tag==CHR('k','e','r','n') && j==0) ||
		 (tag==CHR('v','k','r','n') && j==1))) {
	    k = 0;
	    do {
		sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[k];
		for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL ) {
		    for ( kprev=NULL, kp = j==0 ? sc->kerns : sc->vkerns ; kp!=NULL; kp = knext ) {
			knext = kp->next;
			if ( ( sli==SLI_UNKNOWN || sli==kp->sli ) &&
				( flags==-1 || flags==kp->flags )) {
			    if ( kprev!=NULL )
				kprev->next = knext;
			    else if ( j==0 )
				sc->kerns = knext;
			    else
				sc->vkerns = knext;
			    kp->next = NULL;
			    KernPairsFree(kp);
			    any = true;
			} else
			    kprev = kp;
		    }
		}
		++k;
	    } while ( k<_sf->subfontcnt );
	}
	for ( cprev=NULL, kc = j==0 ? _sf->kerns : _sf->vkerns ; kc!=NULL; kc = cnext ) {
	    cnext = kc->next;
	    if ( ( sli==SLI_UNKNOWN || sli==kc->sli ) &&
		    ( flags==-1 || flags==kc->flags )) {
		if ( cprev!=NULL )
		    cprev->next = cnext;
		else if ( j==0 )
		    _sf->kerns = cnext;
		else
		    _sf->vkerns = cnext;
		kc->next = NULL;
		KernClassListFree(kc);
		any = true;
	    } else
		cprev = kc;
	}
    }

    for ( fprev = NULL, fpst = _sf->possub; fpst!=NULL; fpst=fnext ) {
	fnext = fpst->next;
	if ( ( tag==0xffffffff || tag==fpst->tag ) &&
		( sli==SLI_UNKNOWN || sli==fpst->script_lang_index ) &&
		( flags==-1 || flags==fpst->flags )) {
	    if ( fprev == NULL )
		sf->possub = fnext;
	    else
		fprev->next = fnext;
	    fpst->next = NULL;
	    FPSTFree(fpst);
	    any = true;
	} else
	    fprev = fpst;
    }

    for ( aprev=NULL, ac=sf->anchor; ac!=NULL; ac=anext ) {
	anext = ac->next;
	if ( ( tag==0xffffffff || tag==ac->feature_tag ) &&
		( sli==SLI_UNKNOWN || sli==ac->script_lang_index ) &&
		( flags==-1 || flags==ac->flags )) {
	    SFRemoveAnchorClass(sf,ac);
	    any = true;
	} else
	    aprev = ac;
    }

    if ( tag!=0xffffffff || (sli==SLI_UNKNOWN && flags==-1)) {
	int macflags;
	if ( flags==-1 ) macflags = -1;
	else if ( flags&pst_r2l ) macflags = asm_descending;
	else macflags = 0;
	for ( sprev = NULL, sm = _sf->sm; sm!=NULL; sm=snext ) {
	    snext = sm->next;
	    if ( ( tag==0xffffffff || tag==((sm->feature<<16)|sm->setting) ) &&
		    ( macflags==-1 || macflags==sm->flags )) {
		if ( sprev == NULL )
		    sf->sm = snext;
		else
		    sprev->next = snext;
		sm->next = NULL;
		ASMFree(sm);
		any = true;
	    } else
		sprev = sm;
	}
    }
    if ( any )
	_sf->changed = true;
return( any );
}

void RemoveGeneratedTagsAbove(SplineFont *sf, int old_top) {
    int k;

    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;
    for ( k=sf->gentags.tt_cur-1; k>=old_top; --k )
	SFRemoveThisFeatureTag(sf, sf->gentags.tagtype[k].tag, SLI_NESTED, -1);
    if ( old_top<sf->gentags.tt_cur )
	sf->gentags.tt_cur = old_top;
}

int SFRenameTheseFeatureTags(SplineFont *sf, uint32 tag, int sli, int flags,
	uint32 totag, int tosli, int toflags, int ismac) {
    /* if tag==0xffffffff treat as a wildcard (will match any tag) */
    /* if sli==SLI_UNKNOWN treat as a wildcard */
    /* if flags==-1 treat as a wildcard */
    int i,k;
    SplineChar *sc;
    PST *pst;
    SplineFont *_sf;
    FPST *fpst;
    AnchorClass *ac;
    ASM *sm;
    int any = false;

    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;
    _sf = sf;
    k = 0;
    do {
	sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[k];
	for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL ) {
	    for ( pst = sc->possub; pst!=NULL; pst=pst->next ) {
		if ( ( tag==0xffffffff || tag==pst->tag ) &&
			( sli==SLI_UNKNOWN || sli==pst->script_lang_index ) &&
			( flags==-1 || flags==pst->flags )) {
		    if ( totag!=0xffffffff ) {
			pst->tag = totag;
			pst->macfeature = ismac;
		    }
		    if ( tosli!=SLI_UNKNOWN )
			pst->script_lang_index = sli;
		    if ( toflags!=-1 )
			pst->flags = flags;
		    any = true;
		}
	    }
	}
	++k;
    } while ( k<_sf->subfontcnt );

    if ( !ismac ) {
	for ( fpst = _sf->possub; fpst!=NULL; fpst=fpst->next ) {
	    if ( ( tag==0xffffffff || tag==fpst->tag ) &&
		    ( sli==SLI_UNKNOWN || sli==fpst->script_lang_index ) &&
		    ( flags==-1 || flags==fpst->flags )) {
		if ( totag!=0xffffffff )
		    fpst->tag = totag;
		if ( tosli!=SLI_UNKNOWN )
		    fpst->script_lang_index = sli;
		if ( toflags!=-1 )
		    fpst->flags = toflags;
		any = true;
	    }
	}

	for ( ac=sf->anchor; ac!=NULL; ac=ac->next ) {
	    if ( ( tag==0xffffffff || tag==ac->feature_tag ) &&
		    ( sli==SLI_UNKNOWN || sli==ac->script_lang_index ) &&
		    ( flags==-1 || flags==ac->flags )) {
		if ( totag!=0xffffffff )
		    ac->feature_tag = totag;
		if ( tosli!=SLI_UNKNOWN )
		    ac->script_lang_index = sli;
		if ( toflags!=-1 )
		    ac->flags = toflags;
		any = true;
	    }
	}
    }

    if ( ismac && ( tag!=0xffffffff || (sli==SLI_UNKNOWN && flags==-1))) {
	int macflags;
	if ( flags==-1 ) macflags = -1;
	else if ( flags&pst_r2l ) macflags = asm_descending;
	else macflags = 0;
	for ( sm = _sf->sm; sm!=NULL; sm=sm->next ) {
	    if ( ( tag==0xffffffff || tag==((sm->feature<<16)|sm->setting) ) &&
		    ( macflags==-1 || macflags==sm->flags )) {
		if ( totag!=0xffffffff ) {
		    sm->feature = totag>>16;
		    sm->setting = totag&0xffff;
		}
		if ( toflags!=-1 ) {
		    if ( toflags&pst_r2l )
			sm->flags = asm_descending;
		    else
			sm->flags = 0;
		}
		any = true;
	    }
	}
    }
    if ( any )
	_sf->changed = true;
return( any );
}

struct copycontext {
    uint32 *nested_map;
    int *from_merge;
    int *to_merge;
    int mcur;
    EncMap *tomap;
};

int SFConvertSLI(SplineFont *fromsf,int sli,SplineFont *tosf,
	SplineChar *default_script) {
    int i,j,k;
    struct script_record *from_sr, *to_sr;
    FontView *fv;

    if ( sli==SLI_NESTED )
return( SLI_NESTED );

    if ( tosf==fromsf )
return( sli );		/* It's from us. Yeah! that's easy */


    for ( fv=fv_list; fv!=NULL && fv->sf!=fromsf; fv=fv->next );
    /* The font from which we did the copy is no longer loaded */
    /*  can't access it's sli data */
    if ( fv==NULL ) fromsf=NULL;

    if ( tosf->cidmaster ) tosf = tosf->cidmaster;
    else if ( tosf->mm!=NULL ) tosf = tosf->mm->normal;

    if ( fromsf==NULL ) {
	if ( tosf->script_lang==NULL )
	    sli = SFFindBiggestScriptLangIndex(tosf,SCScriptFromUnicode(default_script),DEFAULT_LANG);
	else {
	    for ( i=0; tosf->script_lang[i]!=NULL; ++i );
	    if ( sli>=i )
		sli = SFFindBiggestScriptLangIndex(tosf,SCScriptFromUnicode(default_script),DEFAULT_LANG);
	}
return( sli );
    }

    if ( fromsf->cidmaster ) fromsf = fromsf->cidmaster;
    else if ( fromsf->mm!=NULL ) fromsf = fromsf->mm->normal;

    from_sr = fromsf->script_lang[sli];
    if ( tosf->script_lang!=NULL ) {
	for ( i=0; tosf->script_lang[i]!=NULL; ++i ) {
	    struct script_record *to_sr = tosf->script_lang[i];
	    for ( j=0; to_sr[j].script!=0 && from_sr[j].script!=0 ; ++j ) {
		if ( to_sr[j].script!=from_sr[j].script )
	    break;
		for ( k=0; to_sr[j].langs[k]!=0 && from_sr[j].langs[k]!=0; ++k ) {
		    if ( to_sr[j].langs[k]!=from_sr[j].langs[k] )
		break;
		}
		if ( to_sr[j].langs[k]!=0 || from_sr[j].langs[k]!=0 )
	    break;
	    }
	    if ( to_sr[j].script==0 && from_sr[j].script==0 )
return( i );
	}
    }
    if ( tosf->script_lang==NULL ) {
	tosf->script_lang = gcalloc(2,sizeof(struct script_record *));
	i = 0;
    } else {
	tosf->script_lang = grealloc(tosf->script_lang,(i+2)*sizeof(struct script_record *));
	tosf->script_lang[i+1] = NULL;
    }
    for ( j=0; from_sr[j].script!=0 ; ++j );
    tosf->script_lang[i] = to_sr = gcalloc(j+1,sizeof(struct script_record));
    for ( j=0; from_sr[j].script!=0 ; ++j ) {
	to_sr[j].script = from_sr[j].script;
	for ( k=0; from_sr[j].langs[k]!=0; ++k );
	to_sr[j].langs = galloc((k+1)*sizeof(uint32));
	for ( k=0; from_sr[j].langs[k]!=0; ++k )
	    to_sr[j].langs[k] = from_sr[j].langs[k];
	to_sr[j].langs[k] = 0;
    }
return( i );
}

static uint32 ConvertNestedTag(SplineFont *tosf, SplineFont *fromsf,uint32 *nested_map, uint32 tag) {
    int i;

    for ( i=0; i<fromsf->gentags.tt_cur; ++i ) {
	if ( fromsf->gentags.tagtype[i].type!=pst_null && fromsf->gentags.tagtype[i].tag==tag )
    break;
    }
    if ( i==fromsf->gentags.tt_cur )		/* Can't happen */
return( tag );

    if ( nested_map[i]!=0 )
return( nested_map[i]);

    nested_map[i] = SFGenerateNewFeatureTag(&tosf->gentags,
	    fromsf->gentags.tagtype[i].type,
	    isdigit(tag&0xff)? 0 : tag );
    /* If the name ends in a digit, it is probably one of our generated tags */
    /*  in which case we should rename it to match the sequence of generated */
    /*  tags in the new font. If it doesn't end in a digit it is a user      */
    /*  supplied name and we should just pass it on */
return( nested_map[i] );
}

static char **ClassCopy(int class_cnt,char **classes) {
    char **newclasses;
    int i;

    if ( classes==NULL || class_cnt==0 )
return( NULL );
    newclasses = galloc(class_cnt*sizeof(char *));
    for ( i=0; i<class_cnt; ++i )
	newclasses[i] = copy(classes[i]);
return( newclasses );
}

static int _SFCopyTheseFeaturesToSF(SplineFont *_sf, uint32 tag, int sli, int flags,
	SplineFont *tosf, struct copycontext *cc);

static int SF_SCAddPST(SplineFont *tosf,SplineChar *sc,PST *pst,
	struct copycontext *cc, SplineFont *fromsf) {
    SplineChar *tosc;
    int to_index = SFFindSlot(tosf,cc->tomap,sc->unicodeenc,sc->name);
    PST *newpst;

    if ( to_index==-1 )
return( false );
    tosc = SFMakeChar(tosf,cc->tomap,to_index);
    if ( tosc==NULL )
return( false );

    newpst = chunkalloc(sizeof(PST));
    *newpst = *pst;
    if ( pst->script_lang_index == SLI_NESTED )
	newpst->tag = ConvertNestedTag(tosf, fromsf,cc->nested_map, pst->tag);
    newpst->script_lang_index = SFConvertSLI(fromsf,pst->script_lang_index,tosf,tosc);
    newpst->next = tosc->possub;
    tosc->possub = newpst;
    tosf->changed = true;

    switch( newpst->type ) {
      case pst_pair:
	newpst->u.pair.paired = copy(pst->u.pair.paired);
	newpst->u.pair.vr = chunkalloc(sizeof(struct vr [2]));
	memcpy(newpst->u.pair.vr,pst->u.pair.vr,sizeof(struct vr [2]));
      break;
      case pst_ligature:
	newpst->u.lig.lig = tosc;
	/* Fall through */
      case pst_substitution:
      case pst_alternate:
      case pst_multiple:
	newpst->u.subs.variant = copy(pst->u.subs.variant);
      break;
    }
return( true );
}

static void SF_SCAddAP(SplineFont *tosf,SplineChar *sc,AnchorPoint *ap,
	SplineFont *fromsf, AnchorClass *newac, struct copycontext *cc) {
    SplineChar *tosc;
    int to_index = SFFindSlot(tosf,cc->tomap,sc->unicodeenc,sc->name);
    AnchorPoint *newap;

    if ( to_index==-1 )
return;
    tosc = SFMakeChar(tosf,cc->tomap,to_index);
    if ( tosc==NULL )
return;

    newap = chunkalloc(sizeof(AnchorPoint));
    *newap = *ap;
    newap->anchor = newac;
    newap->next = tosc->anchor;
    tosc->anchor = newap;
}

static int SF_SCAddKP(SplineFont *tosf,SplineChar *sc,KernPair *kp,
	SplineFont *fromsf, int isvkern, struct copycontext *cc) {
    SplineChar *tosc, *tosecond;
    int to_index = SFFindSlot(tosf,cc->tomap,sc->unicodeenc,sc->name);
    int second_index = SFFindSlot(tosf,cc->tomap,kp->sc->unicodeenc,kp->sc->name);
    KernPair *newkp;

    if ( to_index==-1 || second_index==-1 )
return(false);
    tosc = SFMakeChar(tosf,cc->tomap,to_index);
    tosecond = SFMakeChar(tosf,cc->tomap,second_index);
    if ( tosc==NULL || tosecond==NULL )
return(false);

    newkp = chunkalloc(sizeof(AnchorPoint));
    *newkp = *kp;
    newkp->sc = tosecond;
    newkp->sli = SFConvertSLI(fromsf,kp->sli,tosf,tosc);
    if ( isvkern ) {
	newkp->next = tosc->vkerns;
	tosc->vkerns = newkp;
    } else {
	newkp->next = tosc->kerns;
	tosc->kerns = newkp;
    }
return(true);
}

static void SF_AddAnchor(SplineFont *tosf,AnchorClass *ac, struct copycontext *cc,
	SplineFont *fromsf) {
    AnchorClass *newac;
    int i, k;
    SplineFont *sf;
    SplineChar *sc;
    AnchorPoint *ap;

    for ( i=0; i<cc->mcur && cc->from_merge[i]!=ac->merge_with; ++i );
    if ( i==cc->mcur ) {
	AnchorClass *ac2;
	int max = 0;
	for ( ac2=tosf->anchor; ac2!=NULL; ac2=ac2->next )
	    if ( max<ac2->merge_with )
		max = ac2->merge_with;
	cc->from_merge[i] = ac->merge_with;
	cc->to_merge[i] = max+1;
	++cc->mcur;
    }

    newac = chunkalloc(sizeof(AnchorClass));
    *newac = *ac;
    if ( ac->script_lang_index == SLI_NESTED )
	newac->feature_tag = ConvertNestedTag(tosf, fromsf,cc->nested_map, ac->feature_tag);
    newac->name = copy(ac->name);
    newac->script_lang_index = SFConvertSLI(fromsf,ac->script_lang_index,tosf,NULL);
    tosf->changed = true;
    newac->next = tosf->anchor;
    tosf->anchor = newac;
    newac->merge_with = cc->to_merge[i];

    k = 0;
    do {
	sf = fromsf->subfonts==NULL ? fromsf : fromsf->subfonts[k];
	for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL ) {
	    for ( ap = sc->anchor; ap!=NULL; ap=ap->next ) {
		if ( ap->anchor==ac ) {
		    SF_SCAddAP(tosf,sc,ap, fromsf,newac,cc);
		}
	    }
	}
	++k;
    } while ( k<fromsf->subfontcnt );
}

static void SF_AddKernClass(SplineFont *tosf,KernClass *kc, struct copycontext *cc,
	SplineFont *fromsf, int isvkern) {
    KernClass *newkc;

    newkc = chunkalloc(sizeof(KernClass));
    *newkc = *kc;
    /* can't be nested */
    newkc->sli = SFConvertSLI(fromsf,kc->sli,tosf,NULL);
    tosf->changed = true;
    if ( isvkern ) {
	newkc->next = tosf->vkerns;
	tosf->vkerns = newkc;
    } else {
	newkc->next = tosf->kerns;
	tosf->kerns = newkc;
    }

    newkc->firsts = ClassCopy(newkc->first_cnt,newkc->firsts);
    newkc->seconds = ClassCopy(newkc->second_cnt,newkc->seconds);
    newkc->offsets = galloc(newkc->first_cnt*newkc->first_cnt*sizeof(int16));
    memcpy(newkc->offsets,kc->offsets,newkc->first_cnt*newkc->first_cnt*sizeof(int16));
}

static void SF_AddFPST(SplineFont *tosf,FPST *fpst, struct copycontext *cc, SplineFont *fromsf) {
    FPST *newfpst;
    int i, j, k, cur;
    uint32 *fromtags, *totags;

    newfpst = chunkalloc(sizeof(FPST));
    *newfpst = *fpst;
    if ( fpst->script_lang_index == SLI_NESTED )
	newfpst->tag = ConvertNestedTag(tosf, fromsf,cc->nested_map, fpst->tag);
    newfpst->script_lang_index = SFConvertSLI(fromsf,fpst->script_lang_index,tosf,NULL);
    tosf->changed = true;
    newfpst->next = tosf->possub;
    tosf->possub = newfpst;

    newfpst->nclass = ClassCopy(newfpst->nccnt,newfpst->nclass);
    newfpst->bclass = ClassCopy(newfpst->bccnt,newfpst->bclass);
    newfpst->fclass = ClassCopy(newfpst->fccnt,newfpst->fclass);

    newfpst->rules = galloc(newfpst->rule_cnt*sizeof(struct fpst_rule));
    memcpy(newfpst->rules,fpst->rules,newfpst->rule_cnt*sizeof(struct fpst_rule));

    fromtags = galloc(fromsf->gentags.tt_cur*sizeof(uint32));
    totags = galloc(fromsf->gentags.tt_cur*sizeof(uint32));
    cur = 0;
    for ( i=0; i<newfpst->rule_cnt; ++i ) {
	struct fpst_rule *r = &newfpst->rules[i], *oldr = &fpst->rules[i];

	r->lookups = galloc(r->lookup_cnt*sizeof(struct seqlookup));
	memcpy(r->lookups,oldr->lookups,r->lookup_cnt*sizeof(struct seqlookup));
	for ( k=0; k<r->lookup_cnt; ++k ) {
	    for ( j=0 ; j<cur; ++j )
		if ( fromtags[j] == r->lookups[k].lookup_tag )
	    break;
	    if ( j==cur ) {
		fromtags[cur] = r->lookups[k].lookup_tag;
		totags[cur++] = ConvertNestedTag(tosf, fromsf,cc->nested_map, r->lookups[k].lookup_tag);
	    }
	    r->lookups[k].lookup_tag = totags[j];
	}

	switch ( newfpst->format ) {
	  case pst_glyphs:
	    r->u.glyph.names = copy( r->u.glyph.names );
	    r->u.glyph.back = copy( r->u.glyph.back );
	    r->u.glyph.fore = copy( r->u.glyph.fore );
	  break;
	  case pst_class:
	    r->u.class.nclasses = galloc( r->u.class.ncnt*sizeof(uint16));
	    memcpy(r->u.class.nclasses,oldr->u.class.nclasses, r->u.class.ncnt*sizeof(uint16));
	    r->u.class.bclasses = galloc( r->u.class.bcnt*sizeof(uint16));
	    memcpy(r->u.class.bclasses,oldr->u.class.bclasses, r->u.class.ncnt*sizeof(uint16));
	    r->u.class.fclasses = galloc( r->u.class.fcnt*sizeof(uint16));
	    memcpy(r->u.class.fclasses,oldr->u.class.fclasses, r->u.class.fcnt*sizeof(uint16));
	  break;
	  case pst_coverage:
	    r->u.coverage.ncovers = ClassCopy( r->u.coverage.ncnt, r->u.coverage.ncovers );
	    r->u.coverage.bcovers = ClassCopy( r->u.coverage.bcnt, r->u.coverage.bcovers );
	    r->u.coverage.fcovers = ClassCopy( r->u.coverage.fcnt, r->u.coverage.fcovers );
	  break;
	  case pst_reversecoverage:
	    r->u.rcoverage.ncovers = ClassCopy( r->u.rcoverage.always1, r->u.rcoverage.ncovers );
	    r->u.rcoverage.bcovers = ClassCopy( r->u.rcoverage.bcnt, r->u.rcoverage.bcovers );
	    r->u.rcoverage.fcovers = ClassCopy( r->u.rcoverage.fcnt, r->u.rcoverage.fcovers );
	    r->u.rcoverage.replacements = copy( r->u.rcoverage.replacements );
	  break;
	}
    }
    for ( j=0; j<cur; ++j )
	_SFCopyTheseFeaturesToSF(fromsf,fromtags[j],SLI_NESTED,-1,tosf,cc);
    free(totags); free(fromtags);
}

static void SF_AddASM(SplineFont *tosf,ASM *sm, struct copycontext *cc, SplineFont *fromsf) {
    ASM *newsm;
    int i, j, k, cur;
    uint32 *fromtags, *totags;

    newsm = chunkalloc(sizeof(ASM));
    *newsm = *sm;
    newsm->next = tosf->sm;
    tosf->sm = newsm;
    tosf->changed = true;
    newsm->classes = ClassCopy(newsm->class_cnt, newsm->classes);
    newsm->state = galloc(newsm->class_cnt*newsm->state_cnt*sizeof(struct asm_state));
    memcpy(newsm->state,sm->state,
	    newsm->class_cnt*newsm->state_cnt*sizeof(struct asm_state));
    if ( newsm->type == asm_insert ) {
	for ( i=0; i<newsm->class_cnt*newsm->state_cnt; ++i ) {
	    struct asm_state *this = &newsm->state[i];
	    this->u.insert.mark_ins = copy(this->u.insert.mark_ins);
	    this->u.insert.cur_ins = copy(this->u.insert.cur_ins);
	}
    } else if ( newsm->type == asm_context ) {
	fromtags = gcalloc(newsm->class_cnt*newsm->state_cnt,sizeof(uint32));
	totags = gcalloc(newsm->class_cnt*newsm->state_cnt,sizeof(uint32));
	cur = 0;
	for ( i=0; i<newsm->class_cnt*newsm->state_cnt; ++i ) {
	    for ( k=0; k<2; ++k ) {
		uint32 *tagpt = &(&newsm->state[i].u.context.mark_tag)[k];
		if ( *tagpt==0 )
	    continue;
		for ( j=0 ; j<cur; ++j )
		    if ( fromtags[j] == *tagpt )
		break;
		if ( j==cur ) {
		    fromtags[cur] = *tagpt;
		    totags[cur++] = ConvertNestedTag(tosf, fromsf,cc->nested_map, *tagpt);
		}
		*tagpt = totags[j];
	    }
	}
	for ( j=0; j<cur; ++j )
	    _SFCopyTheseFeaturesToSF(fromsf,fromtags[j],SLI_NESTED,-1,tosf,cc);
	free(totags); free(fromtags);
    }
}

static int _SFCopyTheseFeaturesToSF(SplineFont *_sf, uint32 tag, int sli, int flags,
	SplineFont *tosf, struct copycontext *cc) {
    /* if tag==0xffffffff treat as a wildcard (will match any tag) */
    /* if sli==SLI_UNKNOWN treat as a wildcard */
    /* if flags==-1 treat as a wildcard */
    int i,j,k;
    SplineChar *sc;
    PST *pst;
    SplineFont *sf;
    FPST *fpst;
    ASM *sm;
    AnchorClass *ac;
    KernPair *kp;
    KernClass *kc;
    int any = false;

    k = 0;
    do {
	sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[k];
	for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL ) {
	    for ( pst = sc->possub; pst!=NULL; pst=pst->next ) {
		if ( ( tag==0xffffffff || tag==pst->tag ) &&
			( sli==SLI_UNKNOWN || sli==pst->script_lang_index ) &&
			( flags==-1 || flags==pst->flags )) {
		    if ( SF_SCAddPST(tosf,sc,pst, cc,_sf))
			any = true;
		}
	    }
	}
	++k;
    } while ( k<_sf->subfontcnt );

    for ( ac=sf->anchor; ac!=NULL; ac=ac->next ) {
	if ( ( tag==0xffffffff || tag==ac->feature_tag ) &&
		( sli==SLI_UNKNOWN || sli==ac->script_lang_index ) &&
		( flags==-1 || flags==ac->flags )) {
	    SF_AddAnchor(tosf,ac,cc,_sf);
	    any = true;
	}
    }

    for ( j=0; j<2; ++j ) {
	if ( tag==0xffffffff ||
		((tag==CHR('k','e','r','n') && j==0) ||
		 (tag==CHR('v','k','r','n') && j==1))) {
	    k = 0;
	    do {
		sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[k];
		for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL ) {
		    for ( kp = j==0 ? sc->kerns : sc->vkerns ; kp!=NULL; kp = kp->next ) {
			if ( ( sli==SLI_UNKNOWN || sli==kp->sli ) &&
				( flags==-1 || flags==kp->flags )) {
			    if ( SF_SCAddKP(tosf,sc,kp,_sf,j,cc) )
				any = true;
			}
		    }
		}
		++k;
	    } while ( k<_sf->subfontcnt );
	}
	for ( kc = j==0 ? _sf->kerns : _sf->vkerns ; kc!=NULL; kc = kc->next ) {
	    if ( ( sli==SLI_UNKNOWN || sli==kc->sli ) &&
		    ( flags==-1 || flags==kc->flags )) {
		SF_AddKernClass(tosf,kc,cc,_sf,j);
		any = true;
	    }
	}
    }

    for ( fpst = _sf->possub; fpst!=NULL; fpst=fpst->next ) {
	if ( ( tag==0xffffffff || tag==fpst->tag ) &&
		( sli==SLI_UNKNOWN || sli==fpst->script_lang_index ) &&
		( flags==-1 || flags==fpst->flags )) {
	    SF_AddFPST(tosf,fpst, cc,_sf);
	    any = true;
	}
    }

    if ( tag!=0xffffffff || (sli==SLI_UNKNOWN && flags==-1) ) {
	int macflags;
	if ( flags==-1 ) macflags = -1;
	else if ( flags&pst_r2l ) macflags = asm_descending;
	else macflags = 0;
	for ( sm = _sf->sm; sm!=NULL; sm=sm->next ) {
	    if ( ( tag==0xffffffff || tag==((sm->feature<<16)|sm->setting) ) &&
		    ( macflags==-1 || macflags==sm->flags )) {
		SF_AddASM(tosf,sm, cc,_sf);
		any = true;
	    }
	}
    }
return( any );
}

static int SFCopyTheseFeaturesToFV(SplineFont *sf, uint32 tag, int sli, int flags,
	FontView *tofv) {
    /* if tag==0xffffffff treat as a wildcard (will match any tag) */
    /* if sli==SLI_UNKNOWN treat as a wildcard */
    /* if flags==-1 treat as a wildcard */
    struct copycontext cc;
    int any;
    int i;
    AnchorClass *ac;
    SplineFont *tosf = tofv->sf;

    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;
    if ( tosf->cidmaster!=NULL ) tosf = tosf->cidmaster;
    for ( ac=sf->anchor, i=0; ac!=NULL; ++i, ac=ac->next );
    cc.nested_map = gcalloc(sf->gentags.tt_cur,sizeof(uint32));
    cc.from_merge = galloc(i*sizeof(int));
    cc.to_merge = galloc(i*sizeof(int));
    cc.tomap = tofv->map;
    cc.mcur = i;
    any = _SFCopyTheseFeaturesToSF(sf,tag,sli,flags,tosf,&cc);
    free(cc.nested_map); free(cc.from_merge); free(cc.to_merge);
    if ( any )
	tosf->changed = true;
return( any );
}

int SFRemoveUnusedNestedFeatures(SplineFont *sf) {
    uint8 *used;
    int i,j,k;
    FPST *fpst;
    ASM *sm;
    int any = false;

    if ( sf->cidmaster ) sf = sf->cidmaster;
    used = gcalloc(sf->gentags.tt_cur,sizeof(uint8));

    for ( fpst=sf->possub; fpst!=NULL; fpst=fpst->next ) {
	for ( i=0; i<fpst->rule_cnt; ++i ) {
	    struct fpst_rule *r = &fpst->rules[i];
	    for ( j=0; j<r->lookup_cnt; ++j ) {
		for ( k=0; k<sf->gentags.tt_cur; ++k ) {
		    if ( r->lookups[j].lookup_tag == sf->gentags.tagtype[k].tag ) {
			used[k] = true;
		break;
		    }
		}
	    }
	}
    }
    for ( sm = sf->sm; sm!=NULL; sm=sm->next ) if ( sm->type == asm_context ) {
	for ( i=0; i<sm->state_cnt*sm->class_cnt; ++i ) {
	    for ( j=0; j<2; ++j ) {
		uint32 tag = (&sm->state[i].u.context.mark_tag)[j];
		for ( k=0; k<sf->gentags.tt_cur; ++k ) {
		    if ( sf->gentags.tagtype[k].tag == tag ) {
			used[k] = true;
		break;
		    }
		}
	    }
	}
    }

    for ( i=0; i<sf->gentags.tt_cur; ++i ) if ( !used[i] && sf->gentags.tagtype[i].type!=pst_null ) {
	if ( SFRemoveThisFeatureTag(sf,sf->gentags.tagtype[i].tag,SLI_NESTED,-1))
	    any = true;
	sf->gentags.tagtype[i].type = pst_null;
    }
    free(used);
    if ( any )
	sf->changed = true;
return( any );
}

int SFHasNestedLookupWithTag(SplineFont *sf,uint32 tag,int ispos) {
    FPST *fpst;
    AnchorClass *ac;
    int i,type,start,end;
    PST *pst;

    for ( fpst=sf->possub; fpst!=NULL; fpst = fpst->next ) {
	if ((( ispos && (fpst->type==pst_contextpos || fpst->type==pst_chainpos)) ||
		(!ispos && (fpst->type==pst_contextsub || fpst->type==pst_chainsub || fpst->type==pst_reversesub ))) &&
		fpst->script_lang_index==SLI_NESTED && fpst->tag==tag )
return( true );
    }

    if ( ispos ) {
	for ( ac=sf->anchor; ac!=NULL; ac=ac->next )
	    if ( ac->feature_tag==tag && ac->script_lang_index==SLI_NESTED )
return( true );
    }

    if ( ispos ) {
	start = pst_position;
	end = pst_pair;
    } else {
	start = pst_substitution;
	end = pst_ligature;
    }
    for ( type=start; type<=end; ++type ) {
	for ( i=0; i<sf->glyphcnt; i++ ) if ( sf->glyphs[i]!=NULL ) {
	    for ( pst = sf->glyphs[i]->possub; pst!=NULL; pst=pst->next ) {
		if ( pst->type==type &&
			pst->script_lang_index == SLI_NESTED &&
			pst->tag == tag )
return( true );
	    }
	}
    }
return( false );
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static int typematch(int tagtype, int searchtype) {
return( tagtype==searchtype ||
		    (searchtype==fpst_max &&
			(tagtype==pst_position ||
			 tagtype==pst_pair ||
			 tagtype==pst_kerning ||
			 tagtype==pst_vkerning ||
			 tagtype==pst_anchors ||
			 tagtype==pst_contextpos ||
			 tagtype==pst_chainpos )) ||
		    (searchtype==fpst_max+1 &&
			(tagtype==pst_substitution ||
			 tagtype==pst_alternate ||
			 tagtype==pst_multiple ||
			 tagtype==pst_ligature ||
			 tagtype==pst_contextsub ||
			 tagtype==pst_chainsub ||
			 tagtype==pst_reversesub )) );
}

GTextInfo **SFGenTagListFromType(struct gentagtype *gentags,enum possub_type type) {
    int len, i;
    GTextInfo **ti;
    char buf[8];

    if ( type>=pst_lcaret && type<fpst_max && type!=pst_anchors )
	len=0;		/* Each FPST, etc. represents one lookup, can't duplicate it */
			/*  AnchorClasses can merge, so several make one lookup */
    else {
	len = 0;
	for ( i=0; i<gentags->tt_cur; ++i )
	    if ( typematch(gentags->tagtype[i].type,type))
		++len;
    }

    ti = galloc((len+1)*sizeof(GTextInfo *));
    ti[len] = gcalloc(1,sizeof(GTextInfo));
    buf[4] = 0;
    if ( type<pst_lcaret || type>=fpst_max ) {
	len = 0;
	for ( i=0; i<gentags->tt_cur; ++i )
	    if ( typematch(gentags->tagtype[i].type,type) ) {
		sprintf( buf, "%c%c%c%c",
			(int) (gentags->tagtype[i].tag>>24),
			(int) ((gentags->tagtype[i].tag>>16)&0xff),
			(int) ((gentags->tagtype[i].tag>>8)&0xff),
			(int) (gentags->tagtype[i].tag&0xff) );
		ti[len] = gcalloc(1,sizeof(GTextInfo));
		ti[len]->userdata = (void *) (intpt) gentags->tagtype[i].tag;
		ti[len]->text = uc_copy(buf);
		ti[len]->fg = ti[len]->bg = COLOR_DEFAULT;
		++len;
	    }
    }
return( ti );
}

/* ************************************************************************** */
/* ************************ Feature selection dialogs *********************** */
/* ************************************************************************** */

enum selectfeaturedlg_type { sfd_remove, sfd_retag, sfd_copyto };

struct sf_dlg {
    GWindow gw;
    GGadget *active_tf;		/* For menu routines */
    SplineFont *sf;
    GMenuItem *tagmenu;
    enum selectfeaturedlg_type which;
    int done, ok;
};

struct select_res {
    uint32 tag;
    int ismac, sli, flags;
};

#define CID_Tag		100
#define CID_SLI		101
#define CID_AnyFlags	102
#define CID_TheseFlags	103
#define CID_R2L		104
#define CID_IgnBase	105
#define CID_IgnLig	106
#define CID_IgnMark	107
#define CID_TagMenu	108
#define CID_FontList	109

static void SFD_OpenTypeTag(GWindow gw, GMenuItem *mi, GEvent *e) {
    struct sf_dlg *d = GDrawGetUserData(gw);
    unichar_t ubuf[6];
    uint32 tag = (uint32) (intpt) (mi->ti.userdata);

    ubuf[0] = tag>>24;
    ubuf[1] = (tag>>16)&0xff;
    ubuf[2] = (tag>>8)&0xff;
    ubuf[3] = tag&0xff;
    ubuf[4] = 0;
    GGadgetSetTitle(d->active_tf,ubuf);
}

static void SFD_MacTag(GWindow gw, GMenuItem *mi, GEvent *e) {
    struct sf_dlg *d = GDrawGetUserData(gw);
    unichar_t ubuf[20]; char buf[20];
    uint32 tag = (uint32) (intpt) (mi->ti.userdata);

    sprintf(buf,"<%d,%d>", (int) (tag>>16), (int) (tag&0xff));
    uc_strcpy(ubuf,buf);
    GGadgetSetTitle(d->active_tf,ubuf);
}

static void GMenuItemListFree(GMenuItem *mi) {
    int i;

    if ( mi==NULL )
return;

    for ( i=0; mi[i].ti.text || mi[i].ti.line || mi[i].ti.image ; ++i ) {
	if ( !mi[i].ti.text_in_resource )
	    free( mi[i].ti.text );
	GMenuItemListFree(mi[i].sub);
    }
    free(mi);
}

static GMenuItem *TIListToMI(GTextInfo *ti,void (*moveto)(struct gwindow *base,struct gmenuitem *mi,GEvent *)) {
    int i;
    GMenuItem *mi;

    for ( i=0; ti[i].text!=NULL || ti[i].line ; ++i );

    mi = gcalloc((i+1), sizeof(GMenuItem));
    for ( i=0; ti[i].text!=NULL || ti[i].line ; ++i ) {
	mi[i].ti = ti[i];
	if ( mi[i].ti.text_is_1byte )
	    mi[i].ti.text = (unichar_t *) copy((char *) mi[i].ti.text);
	else if ( !mi[i].ti.text_in_resource )
	    mi[i].ti.text = u_copy(mi[i].ti.text);
	mi[i].ti.bg = mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].moveto = moveto;
    }
    memset(&mi[i],0,sizeof(GMenuItem));
return( mi );
}

static GMenuItem *TagMenu(SplineFont *sf) {
    GMenuItem *top;
    extern GTextInfo *pst_tags[];
    static const char *names[] = { N_("Simple Substitution"), N_("Alternate Substitutions"), N_("Multiple Substitution"),
	    N_("_Ligatures"), N_("Context Sub"), N_("Chain Sub"), N_("Reverse Chain Sub"),
	    N_("Simple Position"), N_("Pairwise Position"), N_("Context Pos"), N_("Chain Pos"),
	    (char *) -1,
	    N_("Mac Features"), 0 };
    static const int indeces[] = { pst_substitution, pst_alternate, pst_multiple, pst_ligature,
	    pst_contextsub, pst_chainsub, pst_reversesub,
	    pst_position, pst_pair, pst_contextpos, pst_chainpos,
	    -1 };
    int i;
    GTextInfo *mac;

    for ( i=0; names[i]!=0; ++i );
    top = gcalloc((i+1),sizeof(GMenuItem));
    for ( i=0; names[i]!=(char *) -1; ++i ) {
	top[i].ti.text = (unichar_t *) _(names[i]);
	top[i].ti.text_is_1byte = true;
	top[i].ti.fg = top[i].ti.bg = COLOR_DEFAULT;
	top[i].sub = TIListToMI(pst_tags[indeces[i]-1],SFD_OpenTypeTag);
    }

    top[i].ti.fg = top[i].ti.bg = COLOR_DEFAULT;
    top[i++].ti.line = true;

    top[i].ti.text = (unichar_t *) names[i];
    top[i].ti.text_is_1byte = true;
    top[i].ti.fg = top[i].ti.bg = COLOR_DEFAULT;
    mac = AddMacFeatures(NULL,pst_max,sf);
    top[i].sub = TIListToMI(mac,SFD_MacTag);
    GTextInfoListFree(mac);
return( top );
}

static int SFD_TagMenu(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonpress ) {
	struct sf_dlg *d = GDrawGetUserData(GGadgetGetWindow(g));
	int off = GGadgetGetCid(g)-CID_TagMenu;
	GEvent dummy;
	GRect pos;
	d->active_tf = GWidgetGetControl(d->gw,CID_Tag+off);
	if ( d->tagmenu==NULL )
	    d->tagmenu = TagMenu(d->sf);

	GGadgetGetSize(g,&pos);
	memset(&dummy,0,sizeof(GEvent));
	dummy.type = et_mousedown;
	dummy.w = d->gw;
	dummy.u.mouse.x = pos.x;
	dummy.u.mouse.y = pos.y;
	GMenuCreatePopupMenu(d->gw, &dummy, d->tagmenu);
    }
return( true );
}

static int SFD_RadioChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	struct sf_dlg *d = GDrawGetUserData(GGadgetGetWindow(g));
	int off = GGadgetGetCid(g)-CID_AnyFlags;
	int enable;
	if ( off&1 ) --off;
	enable = GGadgetGetCid(g)-off == CID_TheseFlags;

	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_R2L+off),enable);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_IgnBase+off),enable);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_IgnLig+off),enable);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_IgnMark+off),enable);
    }
return( true );
}

static int SFD_ParseSelect(struct sf_dlg *d,int off,struct select_res *res) {
    const unichar_t *ret;
    unichar_t *end;
    int f,s;
    int i;
    int32 len;
    GTextInfo **ti;

    res->tag = 0xffffffff;
    res->ismac = 0;
    res->sli = SLI_UNKNOWN;
    res->flags = -1;

    ret = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Tag+off));
    if ( *ret=='\0' )
	res->tag = 0xffffffff;
    else if ( *ret=='<' && (f=u_strtol(ret+1,&end,10), *end==',') &&
	    (s=u_strtol(end+1,&end,10), *end=='>')) {
	res->tag = (f<<16)|s;
	res->ismac = true;
    } else if ( *ret=='\'' && u_strlen(ret)==6 && ret[5]=='\'' ) {
	res->tag = (ret[1]<<24) | (ret[2]<<16) | (ret[3]<<8) | ret[4];
    } else if ( u_strlen(ret)==4 ) {
	res->tag = (ret[0]<<24) | (ret[1]<<16) | (ret[2]<<8) | ret[3];
    } else {
#if defined(FONTFORGE_CONFIG_GDRAW)
	gwwv_post_error(_("Bad Tag"),_("Tag must be 4 characters long"));
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("Bad Tag"),_("Tag must be 4 characters long"));
#endif
return( false );
    }

    ti = GGadgetGetList(GWidgetGetControl(d->gw,CID_SLI+off),&len);
    for ( i=0; i<len; ++i ) {
	if ( ti[i]->selected ) {
	    res->sli = (intpt) ti[i]->userdata;
    break;
	}
    }

    if ( GGadgetIsChecked(GWidgetGetControl(d->gw,CID_TheseFlags+off)) ) {
	res->flags = 0;
	if ( GGadgetIsChecked(GWidgetGetControl(d->gw,off+CID_R2L)) ) res->flags |= pst_r2l;
	if ( GGadgetIsChecked(GWidgetGetControl(d->gw,off+CID_IgnBase)) ) res->flags |= pst_ignorebaseglyphs;
	if ( GGadgetIsChecked(GWidgetGetControl(d->gw,off+CID_IgnLig)) ) res->flags |= pst_ignoreligatures;
	if ( GGadgetIsChecked(GWidgetGetControl(d->gw,off+CID_IgnMark)) ) res->flags |= pst_ignorecombiningmarks;
    }
return( true );
}

static int AddSelect(GGadgetCreateData *gcd, GTextInfo *label, int k, int y,
	int off, SplineFont *sf ) {
    int j;

    label[k].text = (unichar_t *) _("_Tag:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = y+4; 
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k++].creator = GLabelCreate;

    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.cid = CID_Tag+off;
    gcd[k++].creator = GTextFieldCreate;

    label[k].image = &GIcon_menumark;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 115; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-1; 
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.cid = CID_TagMenu+off;
    gcd[k].gd.handle_controlevent = SFD_TagMenu;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("_Script & Languages:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+26; 
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k++].creator = GLabelCreate;

    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
    gcd[k].gd.pos.width = 140;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.u.list = SFLangList(sf,0xa,NULL);
    for ( j=0; gcd[k].gd.u.list[j].text!=NULL; ++j )
	gcd[k].gd.u.list[j].selected = ((intpt) gcd[k].gd.u.list[j].userdata==SLI_UNKNOWN );
    gcd[k].gd.cid = CID_SLI+off;
    gcd[k++].creator = GListButtonCreate;

    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+28;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_cb_on;
    label[k].text = (unichar_t *) _("Any Flags");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_AnyFlags+off;
    gcd[k].gd.handle_controlevent = SFD_RadioChanged;
    gcd[k++].creator = GRadioCreate;

    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+15;
    gcd[k].gd.flags = gg_visible | gg_enabled ;
    label[k].text = (unichar_t *) _("These Flags");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_TheseFlags+off;
    gcd[k].gd.handle_controlevent = SFD_RadioChanged;
    gcd[k++].creator = GRadioCreate;

    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+15;
    gcd[k].gd.flags = gg_visible;
    label[k].text = (unichar_t *) _("Right To Left");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_R2L+off;
    gcd[k++].creator = GCheckBoxCreate;

    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+15;
    gcd[k].gd.flags = gg_visible;
    label[k].text = (unichar_t *) _("Ignore Base Glyphs");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_IgnBase+off;
    gcd[k++].creator = GCheckBoxCreate;

    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+15;
    gcd[k].gd.flags = gg_visible;
    label[k].text = (unichar_t *) _("Ignore Ligatures");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_IgnLig+off;
    gcd[k++].creator = GCheckBoxCreate;

    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+15;
    gcd[k].gd.flags = gg_visible;
    label[k].text = (unichar_t *) _("Ignore Combining Marks");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_IgnMark+off;
    gcd[k++].creator = GCheckBoxCreate;

return( k );
}

static GTextInfo *SFD_FontList(SplineFont *notme) {
    FontView *fv;
    int i;
    GTextInfo *ti;

    for ( i=0, fv=fv_list; fv!=NULL; fv=fv->next ) if ( fv->sf!=notme )
	++i;
    ti = gcalloc(i+1,sizeof(GTextInfo));
    for ( i=0, fv=fv_list; fv!=NULL; fv=fv->next ) if ( fv->sf!=notme ) {
	ti[i].text = uc_copy(fv->sf->fontname);
	ti[i].fg = ti[i].bg = COLOR_DEFAULT;
	ti[i].userdata = fv;
	++i;
    }
return( ti );
}

static int SFD_FontSelected(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listdoubleclick ) {
	struct sf_dlg *d = GDrawGetUserData(GGadgetGetWindow(g));
	d->done = d->ok = true;
    }
return( true );
}

static int SFD_Ok(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct sf_dlg *d = GDrawGetUserData(GGadgetGetWindow(g));
	d->done = d->ok = true;
    }
return( true );
}

static int SFD_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct sf_dlg *d = GDrawGetUserData(GGadgetGetWindow(g));
	d->done = true;
    }
return( true );
}

static int sfd_e_h(GWindow gw, GEvent *event) {
    struct sf_dlg *d = GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_close:
	d->done = true;
      break;
      case et_char:
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("typofeat.html");
return( true );
	}
return( false );
    }
return( true );
}

static int SelectFeatureDlg(SplineFont *sf,enum selectfeaturedlg_type type) {
    struct sf_dlg d;
    static const char *titles[] = { N_("Remove Feature(s)..."),
	    N_("Retag Feature(s)..."),
	    N_("Copy Feature(s) To Font...") };
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[31];
    GTextInfo label[31];
    int i, k, y;
    struct select_res res1, res2;
    int ret;

    memset(&d,0,sizeof(d));
    d.sf = sf;
    d.which = type;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = true;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _(titles[type]);
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,GGadgetScale(200));
    pos.height = GDrawPointsToPixels(NULL,type==sfd_remove ? 225 :
	    type==sfd_retag ? 425 : 355 );
    d.gw = gw = GDrawCreateTopWindow(NULL,&pos,sfd_e_h,&d,&wattrs);

    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));
    k = 0;

    k = AddSelect(gcd, label, k, 5, 0, sf );
    if ( type==sfd_retag ) {
	gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+20;
	gcd[k].gd.pos.width = GDrawPixelsToPoints(NULL,pos.width)-20;
	gcd[k].gd.flags = gg_visible | gg_enabled;
	gcd[k++].creator = GLineCreate;

	label[k].text = (unichar_t *) _("Retag with...");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 20; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+5;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k++].creator = GLabelCreate;

	k = AddSelect(gcd, label, k, gcd[k-1].gd.pos.y+11, 100, sf );
    } else if ( type == sfd_copyto ) {
	gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+20;
	gcd[k].gd.pos.width = GDrawPixelsToPoints(NULL,pos.width)-20;
	gcd[k].gd.flags = gg_visible | gg_enabled;
	gcd[k++].creator = GLineCreate;

	label[k].text = (unichar_t *) _("To Font:");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+5;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k++].creator = GLabelCreate;

	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
	gcd[k].gd.pos.width = gcd[k-2].gd.pos.width+10;
	gcd[k].gd.pos.height = 8*12+10;
	gcd[k].gd.flags = gg_visible | gg_enabled | gg_list_alphabetic;
	gcd[k].gd.handle_controlevent = SFD_FontSelected;
	gcd[k].gd.cid = CID_FontList;
	gcd[k].gd.u.list = SFD_FontList(sf);
	gcd[k++].creator = GListCreate;
    }

    y = gcd[k-1].gd.pos.y+23;
    if ( type==sfd_copyto )
	y = gcd[k-1].gd.pos.y+gcd[k-1].gd.pos.height+7;

    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 30-3; gcd[k].gd.pos.y = y;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled | gg_but_default;
    gcd[k].gd.handle_controlevent = SFD_Ok;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = -30; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+3;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled | gg_but_cancel;
    gcd[k].gd.handle_controlevent = SFD_Cancel;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = 2; gcd[k].gd.pos.y = 2;
    gcd[k].gd.pos.width = pos.width-4;
    gcd[k].gd.pos.height = pos.height-4;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels;
    gcd[k++].creator = GGroupCreate;

    GGadgetsCreate(gw,gcd);
    for ( i=0; i<k; ++i )
	if ( gcd[i].gd.u.list!=NULL &&
		(gcd[i].creator==GListCreate || gcd[i].creator==GListButtonCreate))
	    GTextInfoListFree(gcd[i].gd.u.list);
    GDrawSetVisible(gw,true);
 retry:
    d.done = d.ok = false;
    while ( !d.done )
	GDrawProcessOneEvent(NULL);

    if ( !d.ok )
	ret = -1;
    else {
	if ( !SFD_ParseSelect(&d,0,&res1))
 goto retry;
	if ( type==sfd_remove )
	    ret = SFRemoveThisFeatureTag(sf,res1.tag,res1.sli,res1.flags);
	else if ( type==sfd_retag ) {
	    if ( !SFD_ParseSelect(&d,100,&res2))
 goto retry;
	    ret = SFRenameTheseFeatureTags(sf,res1.tag,res1.sli,res1.flags,
		    res2.tag,res2.sli,res2.flags,res2.ismac);
	} else {
	    GTextInfo *sel = GGadgetGetListItemSelected(GWidgetGetControl(d.gw,CID_FontList));
	    if ( sel==NULL ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
		gwwv_post_error(_("No Selected Font"),_("No Selected Font"));
#elif defined(FONTFORGE_CONFIG_GTK)
		gwwv_post_error(_("No Selected Font"),_("No Selected Font"));
#endif
 goto retry;
	    }
	    ret = SFCopyTheseFeaturesToFV(sf,res1.tag,res1.sli,res1.flags,
		    (FontView *) (sel->userdata));
	}
    }
    GDrawDestroyWindow(gw);
    GMenuItemListFree(d.tagmenu);
return( ret );
}

void SFRemoveFeatureDlg(SplineFont *sf) {
    if ( !SelectFeatureDlg(sf,sfd_remove))
#if defined(FONTFORGE_CONFIG_GDRAW)
	gwwv_post_error(_("No Features Removed"),_("No Features Removed"));
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("No Features Removed"),_("No Features Removed"));
#endif
}

void SFCopyFeatureToFontDlg(SplineFont *sf) {
    if ( !SelectFeatureDlg(sf,sfd_copyto))
#if defined(FONTFORGE_CONFIG_GDRAW)
	gwwv_post_error(_("No Features Copied"),_("No Features Copied"));
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("No Features Copied"),_("No Features Copied"));
#endif
}

void SFRetagFeatureDlg(SplineFont *sf) {
    if ( !SelectFeatureDlg(sf,sfd_retag))
#if defined(FONTFORGE_CONFIG_GDRAW)
	gwwv_post_error(_("No Features Retagged"),_("No Features Retagged"));
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("No Features Retagged"),_("No Features Retagged"));
#endif
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
