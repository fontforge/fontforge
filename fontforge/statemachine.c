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
#include "ttf.h"
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
#include <chardata.h>
#include <utype.h>
#include <ustring.h>
#include <gkeysym.h>

#define CID_New		300
#define CID_Edit	301
#define CID_Delete	302
#define CID_Classes	305

#define CID_FeatSet	306

#define CID_Ok		307
#define CID_Cancel	308

#define CID_Line1	309
#define CID_Line2	310
#define CID_Group	311
#define CID_Group2	312

#define CID_Set		313
#define CID_Select	314
#define CID_GlyphList	315
#define CID_Next	316
#define CID_Prev	317

#define CID_RightToLeft	318
#define CID_VertOnly	319

#define SMD_WIDTH	350
#define SMD_HEIGHT	400
#define SMD_CANCELDROP	32
#define SMD_DIRDROP	88

#define CID_NextState	400
#define CID_Flag4000	401
#define CID_Flag8000	402
#define CID_Flag2000	403
#define CID_Flag1000	404
#define CID_Flag0800	405
#define CID_Flag0400	406
#define CID_IndicVerb	407
#define CID_InsCur	408
#define CID_InsMark	409
#define CID_TagCur	410
#define CID_TagMark	411
#define CID_Kerns	412
#define CID_StateClass	413

#define CID_Up		420
#define CID_Down	421
#define CID_Left	422
#define CID_Right	423

#define SMDE_WIDTH	200
#define SMDE_HEIGHT	(SMD_DIRDROP+200)

extern int _GScrollBar_Width;

typedef struct statemachinedlg {
    GWindow gw, cw, editgw;
    int state_cnt, class_cnt, index;
    struct asm_state *states;
    GGadget *hsb, *vsb;
    ASM *sm;
    SplineFont *sf;
    struct gfi_data *d;
    int isnew;
    GFont *font;
    int fh, as;
    int stateh, statew;
    int xstart, ystart;		/* This is where the headers start */
    int xstart2, ystart2;	/* This is where the data start */
    int width, height;
    int offleft, offtop;
    int canceldrop, sbdrop;
    GTextInfo *mactags;
    int isedit;
    int st_pos;
    int edit_done, edit_ok;
} SMD;

static int  SMD_SBReset(SMD *smd);
static void SMD_HShow(SMD *smd,int pos);

static char *indicverbs[2][16] = {
    { "", "Ax", "xD", "AxD", "ABx", "ABx", "xCD", "xCD",
	"AxCD",  "AxCD", "ABxD", "ABxD", "ABxCD", "ABxCD", "ABxCD", "ABxCD" },
    { "", "xA", "Dx", "DxA", "xAB", "xBA", "CDx", "DCx",
	"CDxA",  "DCxA", "DxAB", "DxBA", "CDxAB", "CDxBA", "DCxAB", "DCxBA" }};

static void StatesFree(struct asm_state *old,int old_class_cnt,int old_state_cnt,
	enum asm_type type) {
    int i, j;

    if ( type==asm_insert ) {
	for ( i=0; i<old_state_cnt ; ++i ) {
	    for ( j=0; j<old_class_cnt; ++j ) {
		struct asm_state *this = &old[i*old_class_cnt+j];
		free(this->u.insert.mark_ins);
		free(this->u.insert.cur_ins);
	    }
	}
    } else if ( type==asm_kern ) {
	for ( i=0; i<old_state_cnt ; ++i ) {
	    for ( j=0; j<old_class_cnt; ++j ) {
		struct asm_state *this = &old[i*old_class_cnt+j];
		free(this->u.kern.kerns);
	    }
	}
    }
    free(old);
}

static struct asm_state *StateCopy(struct asm_state *old,int old_class_cnt,int old_state_cnt,
	int new_class_cnt,int new_state_cnt, enum asm_type type,int freeold) {
    struct asm_state *new = gcalloc(new_class_cnt*new_state_cnt,sizeof(struct asm_state));
    int i,j;
    int minclass = new_class_cnt<old_class_cnt ? new_class_cnt : old_class_cnt;

    for ( i=0; i<old_state_cnt && i<new_state_cnt; ++i ) {
	memcpy(new+i*new_class_cnt, old+i*old_class_cnt, minclass*sizeof(struct asm_state));
	if ( type==asm_insert ) {
	    for ( j=0; j<minclass; ++j ) {
		struct asm_state *this = &new[i*new_class_cnt+j];
		this->u.insert.mark_ins = copy(this->u.insert.mark_ins);
		this->u.insert.cur_ins = copy(this->u.insert.cur_ins);
	    }
	} else if ( type==asm_kern ) {
	    for ( j=0; j<minclass; ++j ) {
		struct asm_state *this = &new[i*new_class_cnt+j];
		int16 *temp;
		temp = galloc(this->u.kern.kcnt*sizeof(int16));
		memcpy(temp,this->u.kern.kerns,this->u.kern.kcnt*sizeof(int16));
		this->u.kern.kerns = temp;
	    }
	}
    }
    for ( ; i<new_state_cnt; ++i )
	new[i*new_class_cnt+2].next_state = i;		/* Deleted glyphs should be treated as noops */

    if ( freeold )
	StatesFree(old,old_class_cnt,old_state_cnt,type);

return( new );
}

static void StateRemoveClasses(SMD *smd, GTextInfo **removethese ) {
    struct asm_state *new;
    int i,j,k, remove_cnt;

    for ( remove_cnt=i=0; i<smd->class_cnt; ++i )
	if ( removethese[i]->selected )
	    ++remove_cnt;
    if ( remove_cnt==0 )
return;

    new = gcalloc((smd->class_cnt-remove_cnt)*smd->state_cnt,sizeof(struct asm_state));
    for ( i=0; i<smd->state_cnt ; ++i ) {
	for ( j=k=0; j<smd->class_cnt; ++j ) {
	    if ( !removethese[j]->selected )
		new[i*(smd->class_cnt-remove_cnt)+k++] = smd->states[i*smd->class_cnt+j];
	    else if ( smd->sm->type==asm_insert ) {
		free(smd->states[i*smd->class_cnt+j].u.insert.mark_ins);
		free(smd->states[i*smd->class_cnt+j].u.insert.cur_ins);
	    } else if ( smd->sm->type==asm_kern ) {
		free(smd->states[i*smd->class_cnt+j].u.kern.kerns);
	    }
	}
    }

    free(smd->states);
    smd->states = new;
    smd->class_cnt -= remove_cnt;
}

static int FindMaxReachableStateCnt(SMD *smd) {
    int max_reachable = 1;
    int i, j, ns;

    for ( i=0; i<=max_reachable && i<smd->state_cnt; ++i ) {
	for ( j=0; j<smd->class_cnt; ++j ) {
	    ns = smd->states[i*smd->class_cnt+j].next_state;
	    if ( ns>max_reachable )
		max_reachable = ns;
	}
    }
return( max_reachable+1 );		/* The count is one more than the max */
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

/* ************************************************************************** */
/* *************** Routines to test conversion from OpenType **************** */
/* ************************************************************************** */

static int ValidSubs(SplineFont *sf,uint32 tag ) {
    int i, any=false;
    PST *pst;

    for ( i = 0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	for ( pst=sf->glyphs[i]->possub; pst!=NULL && pst->tag!=tag; pst=pst->next );
	if ( pst!=NULL && pst->script_lang_index==SLI_NESTED ) {
	    if ( pst->type==pst_substitution )
		any = true;
	    else
return( false );
	}
    }
return( any );
}

static void TreeFree(struct contexttree *tree) {
    int i;

    if ( tree==NULL )
return;

    for ( i=0; i<tree->branch_cnt; ++i )
	TreeFree(tree->branches[i].branch);

    free( tree->branches );
    free( tree->rules );
    chunkfree( tree,sizeof(*tree) );
}

static int TreeLabelState(struct contexttree *tree, int snum) {
    int i;

    if ( tree->branch_cnt==0 && tree->ends_here!=NULL ) {
	tree->state = 0;
return( snum );
    }

    tree->state = snum++;
    for ( i=0; i<tree->branch_cnt; ++i )
	snum = TreeLabelState(tree->branches[i].branch,snum);
    tree->next_state = snum;

return( snum );
}

static uint32 RuleHasSubsHere(struct fpst_rule *rule,int depth) {
    int i,j;

    if ( depth<rule->u.class.bcnt )
return( 0 );
    depth -= rule->u.class.bcnt;
    if ( depth>=rule->u.class.ncnt )
return( 0 );
    for ( i=0; i<rule->lookup_cnt; ++i ) {
	if ( rule->lookups[i].seq==depth ) {
	    /* It is possible to have two substitutions applied at the same */
	    /*  location. I can't deal with that here */
	    for ( j=i+1; j<rule->lookup_cnt; ++j ) {
		if ( rule->lookups[j].seq==depth )
return( 0xffffffff );
	    }
return( rule->lookups[i].lookup_tag );
	}
    }

return( 0 );
}

static uint32 RulesAllSameSubsAt(struct contexttree *me,int pos) {
    int i;
    uint32 tag=0x01, newtag;	/* Can't use 0 as an "unused" flag because it is perfectly valid for there to be no substititution. But then all rules must have no subs */

    for ( i=0; i<me->rule_cnt; ++i ) {
	newtag = RuleHasSubsHere(me->rules[i].rule,pos);
	if ( tag==0x01 )
	    tag=newtag;
	else if ( newtag!=tag )
return( 0xffffffff );
    }
return( tag );
}

static int TreeFollowBranches(SplineFont *sf,struct contexttree *me,int pending_pos) {
    int i, j;

    me->pending_pos = pending_pos;
    if ( me->ends_here!=NULL ) {
	/* If any rule ends here then we have to be able to apply all current */
	/*  and pending substitutions */
	if ( pending_pos!=-1 ) {
	    me->applymarkedsubs = RulesAllSameSubsAt(me,pending_pos);
	    if ( me->applymarkedsubs==0xffffffff )
return( false );
	    if ( !ValidSubs(sf,me->applymarkedsubs))
return( false );
	}
	me->applycursubs = RulesAllSameSubsAt(me,me->depth);
	if ( me->applycursubs==0xffffffff )
return( false );
	if ( me->applycursubs!=0 && !ValidSubs(sf,me->applycursubs))
return( false );
	for ( i=0; i<me->branch_cnt; ++i ) {
	    if ( !TreeFollowBranches(sf,me->branches[i].branch,-1))
return( false );
	}
    } else {
	for ( i=0; i<me->branch_cnt; ++i ) {
	    for ( j=0; j<me->rule_cnt; ++j )
		if ( me->rules[j].branch==me->branches[i].branch &&
			RuleHasSubsHere(me->rules[j].rule,me->depth))
	    break;
	    if ( j<me->rule_cnt ) {
		if ( pending_pos==-1 ) {
		    pending_pos = me->pending_pos = me->depth;
		    me->markme = true;
		} else
return( false );
	    }
	    if ( !TreeFollowBranches(sf,me->branches[i].branch,pending_pos))
return( false );
	}
    }

return( true );
}

static struct contexttree *_FPST2Tree(FPST *fpst,struct contexttree *parent,int class) {
    struct contexttree *me = chunkalloc(sizeof(struct contexttree));
    int i, rcnt, ccnt, k, thisclass;
    uint16 *classes;

    if ( fpst!=NULL ) {
	me->depth = -1;
	me->rule_cnt = fpst->rule_cnt;
	me->rules = gcalloc(me->rule_cnt,sizeof(struct ct_subs));
	for ( i=0; i<me->rule_cnt; ++i )
	    me->rules[i].rule = &fpst->rules[i];
	me->parent = NULL;
    } else {
	me->depth = parent->depth+1;
	for ( i=rcnt=0; i<parent->rule_cnt; ++i )
	    if ( parent->rules[i].rule->u.class.allclasses[me->depth] == class )
		++rcnt;
	me->rule_cnt = rcnt;
	me->rules = gcalloc(me->rule_cnt,sizeof(struct ct_subs));
	for ( i=rcnt=0; i<parent->rule_cnt; ++i )
	    if ( parent->rules[i].rule->u.class.allclasses[me->depth] == class )
		me->rules[rcnt++].rule = parent->rules[i].rule;
	me->parent = parent;
    }
    classes = galloc(me->rule_cnt*sizeof(uint16));
    for ( i=ccnt=0; i<me->rule_cnt; ++i ) {
	thisclass = me->rules[i].thisclassnum = me->rules[i].rule->u.class.allclasses[me->depth+1];
	if ( thisclass==0xffff ) {
	    if ( me->ends_here==NULL )
		me->ends_here = me->rules[i].rule;
	} else {
	    for ( k=0; k<ccnt; ++k )
		if ( classes[k] == thisclass )
	    break;
	    if ( k==ccnt )
		classes[ccnt++] = thisclass;
	}
    }
    me->branch_cnt = ccnt;
    me->branches = gcalloc(ccnt,sizeof(struct ct_branch));
    for ( i=0; i<ccnt; ++i )
	me->branches[i].classnum = classes[i];
    for ( i=0; i<ccnt; ++i ) {
	me->branches[i].branch = _FPST2Tree(NULL,me,classes[i]);
	for ( k=0; k<me->rule_cnt; ++k )
	    if ( classes[i]==me->rules[k].thisclassnum )
		me->rules[k].branch = me->branches[i].branch;
    }
    free(classes );
return( me );
}

static void FPSTBuildAllClasses(FPST *fpst) {
    int i, off,j;

    for ( i=0; i<fpst->rule_cnt; ++i ) {
	fpst->rules[i].u.class.allclasses = galloc((fpst->rules[i].u.class.bcnt+
						    fpst->rules[i].u.class.ncnt+
			                            fpst->rules[i].u.class.fcnt+
			                            1)*sizeof(uint16));
	off = fpst->rules[i].u.class.bcnt;
	for ( j=0; j<off; ++j )
	    fpst->rules[i].u.class.allclasses[j] = fpst->rules[i].u.class.bclasses[off-1-j];
	for ( j=0; j<fpst->rules[i].u.class.ncnt; ++j )
	    fpst->rules[i].u.class.allclasses[off+j] = fpst->rules[i].u.class.nclasses[j];
	off += j;
	for ( j=0; j<fpst->rules[i].u.class.fcnt; ++j )
	    fpst->rules[i].u.class.allclasses[off+j] = fpst->rules[i].u.class.fclasses[j];
	fpst->rules[i].u.class.allclasses[off+j] = 0xffff;	/* End of rule marker */
    }
}

static void FPSTFreeAllClasses(FPST *fpst) {
    int i;

    for ( i=0; i<fpst->rule_cnt; ++i ) {
	free( fpst->rules[i].u.class.allclasses );
	fpst->rules[i].u.class.allclasses = NULL;
    }
}

static struct contexttree *FPST2Tree(SplineFont *sf,FPST *fpst) {
    struct contexttree *ret;

    if ( fpst->format != pst_class )
return( NULL );

    /* I could check for subclasses rather than ClassesMatch, but then I'd have */
    /* to make sure that class 0 was used (if at all) consistently */
    if ( (fpst->bccnt!=0 && !ClassesMatch(fpst->bccnt,fpst->bclass,fpst->nccnt,fpst->nclass)) ||
	    (fpst->fccnt!=0 && !ClassesMatch(fpst->fccnt,fpst->fclass,fpst->nccnt,fpst->nclass)))
return( NULL );

    FPSTBuildAllClasses(fpst);
	
    ret = _FPST2Tree(fpst,NULL,0);

    if ( !TreeFollowBranches(sf,ret,-1) ) {
	TreeFree(ret);
	ret = NULL;
    }

    FPSTFreeAllClasses(fpst);

    TreeLabelState(ret,1);	/* actually, it's states 0&1, but this will do */

return( ret );
}

static struct contexttree *TreeNext(struct contexttree *cur) {
    struct contexttree *p;
    int i;

    if ( cur->branch_cnt!=0 )
return( cur->branches[0].branch );
    else {
	forever {
	    p = cur->parent;
	    if ( p==NULL )
return( NULL );
	    for ( i=0; i<p->branch_cnt; ++i ) {
		if ( p->branches[i].branch==cur ) {
		    ++i;
	    break;
		}
	    }
	    if ( i<p->branch_cnt )
return( p->branches[i].branch );
	    cur = p;
	}
    }
}

int FPSTisMacable(SplineFont *sf, FPST *fpst, int checktag) {
    int i;
    int featureType, featureSetting;
    struct contexttree *ret;

    if ( fpst->type!=pst_contextsub && fpst->type!=pst_chainsub )
return( false );
    if ( !SLIHasDefault(sf,fpst->script_lang_index))
return( false );
    if ( checktag && !OTTagToMacFeature(fpst->tag,&featureType,&featureSetting) )
return( false );

    if ( fpst->format == pst_glyphs ) {
	FPST *tempfpst = FPSTGlyphToClass(fpst);
	ret = FPST2Tree(sf, tempfpst);
	FPSTFree(tempfpst);
	TreeFree(ret);
return( ret!=NULL );
    } else if ( fpst->format == pst_class ) {
	ret = FPST2Tree(sf, fpst);
	TreeFree(ret);
return( ret!=NULL );
    } else if ( fpst->format != pst_coverage )
return( false );

    for ( i=0; i<fpst->rule_cnt; ++i ) {
	if ( fpst->rules[i].u.coverage.ncnt+
		fpst->rules[i].u.coverage.bcnt+
		fpst->rules[i].u.coverage.fcnt>=10 )
return( false );			/* Let's not make a state machine this complicated */

	if ( fpst->rules[i].lookup_cnt==2 ) {
	    switch ( fpst->format ) {
	      case pst_coverage:
		/* Second substitution must be on the final glyph */
		if ( fpst->rules[i].u.coverage.fcnt!=0 ||
			fpst->rules[i].lookups[0].seq==fpst->rules[i].lookups[1].seq ||
			(fpst->rules[i].lookups[0].seq!=fpst->rules[i].u.coverage.ncnt-1 &&
			 fpst->rules[i].lookups[1].seq!=fpst->rules[i].u.coverage.ncnt-1) )
return( false );
	      break;
	      default:
return( false );
	    }
	    if ( !ValidSubs(sf,fpst->rules[i].lookups[1].lookup_tag) )
return( false );
		
	} else if ( fpst->rules[i].lookup_cnt!=1 )
return( false );
	if ( !ValidSubs(sf,fpst->rules[i].lookups[0].lookup_tag) )
return( false );
    }

return( fpst->rule_cnt>0 );
}

/* ************************************************************************** */
/* *************** Conversion from OpenType Context/Chaining **************** */
/* ************************************************************************** */

	/* ********************** From Forms ********************** */
static int IsMarkChar( SplineChar *sc ) {
    AnchorPoint *ap;

    ap=sc->anchor;
    while ( ap!=NULL && (ap->type==at_centry || ap->type==at_cexit) )
	ap = ap->next;
    if ( ap!=NULL && (ap->type==at_mark || ap->type==at_basemark) )
return( true );

return( false );
}

static char *GlyphListToNames(SplineChar **classglyphs) {
    int i, len;
    char *ret, *pt;

    for ( i=len=0; classglyphs[i]!=NULL; ++i )
	len += strlen(classglyphs[i]->name)+1;
    ret = pt = galloc(len+1);
    for ( i=0; classglyphs[i]!=NULL; ++i ) {
	strcpy(pt,classglyphs[i]->name);
	pt += strlen(pt);
	*pt++ = ' ';
    }
    if ( pt>ret )
	pt[-1] = '\0';
    else
	*ret = '\0';
return( ret );
}

static char *BuildMarkClass(SplineFont *sf) {
    SplineChar *sc, **markglyphs;
    int i, mg;
    char *ret;

    mg = 0;
    markglyphs = galloc(sf->glyphcnt*sizeof(SplineChar *));
    for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL ) {
	if ( IsMarkChar(sc)) {
	    markglyphs[mg++] = sc;
	}
    }
    markglyphs[mg] = NULL;
    ret = GlyphListToNames(markglyphs);
    free(markglyphs);
return(ret);
}

static char *BuildClassNames(SplineChar **glyphs,uint16 *map, int classnum) {
    int i, len;
    char *ret, *pt;

    for ( i=len=0; glyphs[i]!=NULL; ++i ) {
	if ( map[i]==classnum )
	    len += strlen(glyphs[i]->name)+1;
    }
    ret = pt = galloc(len+1);
    for ( i=len=0; glyphs[i]!=NULL; ++i ) {
	if ( map[i]==classnum ) {
	    strcpy(pt,glyphs[i]->name);
	    pt += strlen(pt);
	    *pt++ = ' ';
	}
    }
    if ( pt>ret )
	pt[-1] = '\0';
    else
	*ret = '\0';
return( ret );
}

static void BuildNestedFormSubs(SplineFont *sf,SplineChar **glyphs,
	SplineChar **forms[4], uint32 tags[4], int flags) {
    int any[4];
    int i,j;

    memset(any,0,sizeof(any));
    for ( i=0; glyphs[i]!=NULL; ++i ) {
	for ( j=0; j<4; ++j )
	    if ( forms[j][i])
		any[j] = true;
    }

    memset(tags,0,4*sizeof(uint32));
    for ( j=0; j<4; ++j ) if ( any[j] )
	tags[j] = SFGenerateNewFeatureTag(&sf->gentags,pst_substitution,0);

    for ( i=0; glyphs[i]!=NULL; ++i ) {
	for ( j=0; j<4; ++j ) {
	    if ( forms[j][i]!=NULL ) {
		if ( glyphs[i]!=forms[j][i] )
		    glyphs[i]->possub = AddSubs(glyphs[i]->possub,tags[j],forms[j][i]->name,
			    flags,SLI_NESTED,NULL);
		if ( j==2 && forms[1][i]!=NULL )	/* Final must be prepared to convert both base chars and medial chars */
		    forms[1][i]->possub = AddSubs(forms[1][i]->possub,tags[j],forms[j][i]->name,flags,
			    SLI_NESTED,NULL);
		else if ( j==3 && forms[0][i]!=NULL )	/* Isolated must convert both base and initial chars */
		    forms[0][i]->possub = AddSubs(forms[0][i]->possub,tags[j],forms[j][i]->name,flags,
			    SLI_NESTED,NULL);
	    }
	}
    }
}

ASM *ASMFromOpenTypeForms(SplineFont *sf,int sli,int flags) {
    int i, gcnt, f, any, which, cg, mg, ng;
    SplineChar *sc, *rsc, **glyphs, **classglyphs, **markglyphs;
    SplineChar **forms[4];
    PST *pst;
    uint32 script;
    uint32 tags[4];
    int found;
    ASM *sm;

    glyphs = galloc(sf->glyphcnt*sizeof(SplineChar *));
    classglyphs = galloc(sf->glyphcnt*sizeof(SplineChar *));
    markglyphs = galloc(sf->glyphcnt*sizeof(SplineChar *));
    for ( i=0; i<4; ++i )
	forms[i] = gcalloc(sf->glyphcnt,sizeof(SplineChar *));

    gcnt = 0;
    for ( f=0; f<4; ++f) forms[f][gcnt] = NULL;
    found = 0;
    for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL ) {
	any = false;
	for ( pst = sc->possub; pst!=NULL; pst=pst->next ) if ( pst->script_lang_index==sli && pst->flags==flags ) {
	    if ( pst->tag==CHR('i','n','i','t')) which = 0;
	    else if ( pst->tag==CHR('m','e','d','i')) which = 1;
	    else if ( pst->tag==CHR('f','i','n','a')) which = 2;
	    else if ( pst->tag==CHR('i','s','o','l')) which = 3;
	    else
	continue;
	    found |= 1<<which;
	    rsc = SFGetChar(sf,-1,pst->u.subs.variant);
	    forms[which][gcnt] = rsc;
	    any = true;
	}
	if ( any ) {
	    /* Currently I transform all isolated chars to initial first */
	    /*  so if the isolated char, is just the char unchanged, I have to change it back */
	    /* Similarly for medial & final */
	    if ( forms[0][gcnt]!=NULL && forms[3][gcnt]==NULL ) forms[3][gcnt] = sc;
	    if ( forms[1][gcnt]!=NULL && forms[2][gcnt]==NULL ) forms[2][gcnt] = sc;
	    glyphs[gcnt++] = sc;
	    for ( f=0; f<4; ++f) forms[f][gcnt] = NULL;
	}
    }
    glyphs[gcnt] = NULL;
    if ( gcnt==0 ) {
	for ( f=0; f<4; ++f) free(forms[f]); free(glyphs);
return( NULL );
    }
    script =SCScriptFromUnicode(glyphs[0]);

    sm = chunkalloc(sizeof(ASM));
    sm->type = asm_context;
    sm->flags = (flags&pst_r2l) ? asm_descending : 0;
    sm->opentype_tag =  (found&1) ? CHR('i','n','i','t') :
			(found&2) ? CHR('m','e','d','i') :
			(found&4) ? CHR('f','i','n','a') :
					CHR('i','s','o','l');
    /* Only one (or two) classes of any importance: Letter in this script */
    /* might already be formed. Might be a lig. Might be normal */
    /* Oh, if ignoremarks is true, then combining marks merit a class of their own */
    sm->class_cnt = (flags&pst_ignorecombiningmarks) ? 6 : 5;
    sm->classes = gcalloc(sm->class_cnt,sizeof(char *));

    cg = mg = ng = 0;
    for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL ) {
	if ( (flags&pst_ignorecombiningmarks) && IsMarkChar(sc)) {
	    markglyphs[mg++] = sc;
	    if ( sc==glyphs[ng]) ++ng;
	} else if ( sc==glyphs[ng] ) {
	    classglyphs[cg++] = sc;
	    ++ng;
	} else if ( SCScriptFromUnicode(sc)==script )
	    classglyphs[cg++] = sc;
    }
    classglyphs[cg] = NULL;
    sm->classes[4] = GlyphListToNames(classglyphs);
    if ( flags&pst_ignorecombiningmarks ) {
	markglyphs[mg] = NULL;
	sm->classes[5] = GlyphListToNames(markglyphs);
    }
    free(classglyphs); free(markglyphs);

    BuildNestedFormSubs(sf,glyphs,forms,tags,flags);
    free(glyphs);
    for ( i=0; i<4; ++i ) free(forms[i]);

    /* State 0,1 are start states */
    /* State 2 means we have found one interesting letter, transformed current to 'init' and marked it (in case we need to make it isolated) */
    /* State 3 means we have found two interesting letters, transformed current to 'medi' and marked (in case we need to make it final) */
    sm->state_cnt = 4;
    sm->state = gcalloc(sm->state_cnt*sm->class_cnt,sizeof(struct asm_state));

    sm->state[4].next_state = 2;
    sm->state[4].flags = 0x8000;
    sm->state[4].u.context.cur_tag = tags[0];			  /* Initial */

    sm->state[sm->class_cnt+4] = sm->state[4];

    for ( i=0; i<4; ++i ) {
	sm->state[2*sm->class_cnt+i].next_state = 0;
	sm->state[2*sm->class_cnt+i].u.context.mark_tag = tags[3];/* Isolated */
    }

    sm->state[2*sm->class_cnt+4].next_state = 3;
    sm->state[2*sm->class_cnt+4].flags = 0x8000;
    sm->state[2*sm->class_cnt+4].u.context.cur_tag = tags[1];	  /* Medial */

    for ( i=0; i<4; ++i ) {
	sm->state[3*sm->class_cnt+i].next_state = 0;
	sm->state[3*sm->class_cnt+i].u.context.mark_tag = tags[2];/* Final */
    }

    sm->state[3*sm->class_cnt+4] = sm->state[2*sm->class_cnt+4];

    /* Deleted glyph retains same state, just eats the glyph */
    for ( i=0; i<sm->state_cnt; ++i ) {
	int pos = i*sm->class_cnt+2, mpos = i*sm->class_cnt+5;
	sm->state[pos].next_state = i;
	sm->state[pos].flags = 0;
	sm->state[pos].u.context.cur_tag = 0;
	sm->state[pos].u.context.mark_tag = 0;
	/* same for ignored marks */
	if ( flags&pst_ignorecombiningmarks )
	    sm->state[mpos].next_state = i;
    }

return( sm );
}

	/* ********************** From Coverage FPST ********************** */
static SplineChar **morx_cg_FigureClasses(SplineChar ***tables,int match_len,
	int ***classes, int *cc, uint16 **mp, int *gc,
	FPST *fpst,SplineFont *sf,int ordered) {
    int i,j,k, mask, max, class_cnt, gcnt, gtot;
    SplineChar ***temp, *sc, **glyphs, **gall;
    uint16 *map;
    int *nc;
    int *next;
    /* For each glyph used, figure out what coverage tables it gets used in */
    /*  then all the glyphs which get used in the same set of coverage tables */
    /*  can form one class */

    if ( match_len>10 )		/* would need too much space to figure out */
return( NULL );

    gtot = 0;
    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	sf->glyphs[i]->lsidebearing = 1;
	if ( !ordered )
	    sf->glyphs[i]->ttf_glyph = gtot++;
	else if ( sf->glyphs[i]->ttf_glyph+1>gtot )
	    gtot = sf->glyphs[i]->ttf_glyph+1;
    }

    max=0;
    for ( i=0; i<match_len; ++i ) {
	for ( k=0; tables[i][k]!=NULL; ++k );
	if ( k>max ) max=k;
    }
    next = gcalloc(1<<match_len,sizeof(int));
    temp = galloc((1<<match_len)*sizeof(SplineChar **));

    for ( i=0; i<gtot; ++i ) if ( sf->glyphs[i]!=NULL ) {
	sf->glyphs[i]->lsidebearing = 0;
	sf->glyphs[i]->ticked = false;
    }
    for ( i=0; i<match_len; ++i ) {
	for ( j=0; tables[i][j]!=NULL ; ++j )
	    tables[i][j]->lsidebearing |= 1<<i;
    }

    for ( i=0; i<match_len; ++i ) {
	for ( j=0; (sc=tables[i][j])!=NULL ; ++j ) if ( !sc->ticked ) {
	    mask = sc->lsidebearing;
	    if ( next[mask]==0 )
		temp[mask] = galloc(max*sizeof(SplineChar *));
	    temp[mask][next[mask]++] = sc;
	    sc->ticked = true;
	}
    }

    gall = gcalloc(gtot+1,sizeof(SplineChar *));
    class_cnt = gcnt = 0;
    for ( i=0; i<(1<<match_len); ++i ) {
	if ( next[i]!=0 ) {
	    for ( k=0; k<next[i]; ++k ) {
		gall[temp[i][k]->ttf_glyph] = temp[i][k];
		temp[i][k]->lsidebearing = class_cnt;
	    }
	    ++class_cnt;
	    gcnt += next[i];
	    free(temp[i]);
	}
    }
    if ( fpst->flags & pst_ignorecombiningmarks ) {
	for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL && sf->glyphs[i]->ttf_glyph!=-1 ) {
	    if ( sf->glyphs[i]->lsidebearing==0 && IsMarkChar(sf->glyphs[i])) {
		sf->glyphs[i]->lsidebearing = class_cnt;
		++gcnt;
	    }
	}
	++class_cnt;			/* Add a class for the marks so we can ignore them */
    }
    *cc = class_cnt+4;
    glyphs = galloc((gcnt+1)*sizeof(SplineChar *));
    map = galloc((gcnt+1)*sizeof(uint16));
    gcnt = 0;
    for ( i=0; i<gtot; ++i ) if ( gall[i]!=NULL ) {
	glyphs[gcnt] = gall[i];
	map[gcnt++] = gall[i]->lsidebearing+4;	/* there are 4 standard classes, so our first class starts at 4 */
    }
    glyphs[gcnt] = NULL;
    free(gall);
    free(temp);
    *gc = gcnt;
    *mp = map;

    nc = gcalloc(match_len,sizeof(int));
    *classes = galloc((match_len+1)*sizeof(int *));
    for ( i=0; i<match_len; ++i )
	(*classes)[i] = galloc((class_cnt+1)*sizeof(int));
    (*classes)[i] = NULL;

    class_cnt = 0;
    for ( i=0; i<(1<<match_len); ++i ) {
	if ( next[i]!=0 ) {
	    for ( j=0; j<match_len; ++j ) if ( i&(1<<j)) {
		(*classes)[j][nc[j]++] = class_cnt+4;	/* there are 4 standard classes, so our first class starts at 4 */
	    }
	    ++class_cnt;
	}
    }
    for ( j=0; j<match_len; ++j )
	(*classes)[j][nc[j]] = 0xffff;		/* End marker */

    free(next);
    free(nc);
return( glyphs );
}

static ASM *ASMFromCoverageFPST(SplineFont *sf,FPST *fpst,int ordered) {
    SplineChar ***tables, **glyphs;
    int **classes, class_cnt, gcnt;
    int i, j, k, match_len;
    struct fpst_rule *r = &fpst->rules[0];
    int subspos = r->u.coverage.bcnt+r->lookups[0].seq, hasfinal = false;
    int substag = r->lookups[0].lookup_tag, finaltag=-1;
    uint16 *map;
    ASM *sm;

    /* In one very specific case we can support two substitutions */
    if ( r->lookup_cnt==2 ) {
	hasfinal = true;
	if ( r->lookups[0].seq==r->u.coverage.ncnt-1 ) {
	    finaltag = substag;
	    subspos = r->u.coverage.bcnt+r->lookups[1].seq;
	    substag = r->lookups[1].lookup_tag;
	} else
	    finaltag = r->lookups[1].lookup_tag;
    }

    tables = galloc((r->u.coverage.ncnt+r->u.coverage.bcnt+r->u.coverage.fcnt+1)*sizeof(SplineChar **));
    for ( j=0, i=r->u.coverage.bcnt-1; i>=0; --i, ++j )
	tables[j] = SFGlyphsFromNames(sf,r->u.coverage.bcovers[i]);
    for ( i=0; i<r->u.coverage.ncnt; ++i, ++j )
	tables[j] = SFGlyphsFromNames(sf,r->u.coverage.ncovers[i]);
    for ( i=0; i<r->u.coverage.fcnt; ++i, ++j )
	tables[j] = SFGlyphsFromNames(sf,r->u.coverage.fcovers[i]);
    tables[j] = NULL;
    match_len = j;

    for ( i=0; i<match_len; ++i )
	if ( tables[i]==NULL || tables[i][0]==NULL )
return( NULL );

    glyphs = morx_cg_FigureClasses(tables,match_len,
	    &classes,&class_cnt,&map,&gcnt,fpst,sf,ordered);
    if ( glyphs==NULL )
return( NULL );

    for ( i=0; i<match_len; ++i )
	free(tables[i]);
    free(tables);

    sm = chunkalloc(sizeof(ASM));
    sm->type = asm_context;
    sm->flags = (fpst->flags&pst_r2l) ? asm_descending : 0;
    sm->class_cnt = class_cnt;
    sm->classes = galloc(class_cnt*sizeof(char *));
    sm->classes[0] = sm->classes[1] = sm->classes[2] = sm->classes[3] = NULL;
    for ( i=4; i<class_cnt; ++i )
	sm->classes[i] = BuildClassNames(glyphs,map,i);
    free(glyphs); free(map);

    /* Now build the state machine */
    /* we have match_len+1 states (there are 2 initial states) */
    /*  we transition from the initial state to our first state when we get */
    /*  any class which makes up the first coverage table. From the first */
    /*  to the second on any class which makes up the second ... */
    sm->state_cnt = match_len+1;
    sm->state = gcalloc(sm->state_cnt*sm->class_cnt,sizeof(struct asm_state));
    for ( j=0; j<match_len; ++j ) {
	int off = (j+1)*sm->class_cnt;
	for ( i=0; i<class_cnt; ++i ) {
	    for ( k=0; classes[j][k]!=0xffff && classes[j][k]!=i; ++k );
	    if ( classes[j][k]==i ) {
		sm->state[off+i].next_state = j+2;
		if ( j==match_len-1 ) {
		    sm->state[off+i].next_state = 0;
		    sm->state[off+i].flags = 0x4000;
		    if ( subspos==j )
			sm->state[off+i].u.context.cur_tag = substag;
		    else {
			sm->state[off+i].u.context.mark_tag = substag;
			sm->state[off+i].u.context.cur_tag = finaltag;
		    }
		} else if ( subspos==j )
		    sm->state[off+i].flags = 0x8000;
	    } else if ( i==2 || ((fpst->flags&pst_ignorecombiningmarks) && i==class_cnt-1 ) )
		sm->state[off+i].next_state = j+1;	/* Deleted glyph is a noop */
	    else if ( j!=0 )
		sm->state[off+i].flags = 0x4000;	/* Don't eat the current glyph, go back to state 0 and see if it will start the sequence over again */
	}
    }
    /* Class 0 and class 1 should be the same. We only filled in class 1 above*/
    memcpy(sm->state,sm->state+sm->class_cnt,sm->class_cnt*sizeof(struct asm_state));
    for ( j=0; j<match_len; ++j )
	free(classes[j]);
    free(classes);
return( sm );
}

	/* ********************** From Class FPST ********************** */
static void SMSetState(struct asm_state *trans,struct contexttree *cur,int class) {
    int i;

    for ( i=0; i<cur->branch_cnt; ++i ) {
	if ( cur->branches[i].classnum==class ) {
	    trans->next_state = cur->branches[i].branch->state;
	    /* If we go back to state 0, it means we want to start from */
	    /*  the begining again, and we should check against the */
	    /*  current glyph (which failed for us, but might be useful */
	    /*  to start a new operation).  Even if we did not fail we */
	    /*  should still do this (so don't advance the glyph) */
	    trans->flags = cur->branches[i].branch->state!=0
		    ? cur->branches[i].branch->markme?0x8000:0x0000
		    : cur->branches[i].branch->markme?0xc000:0x4000;
	    trans->u.context.mark_tag = cur->branches[i].branch->applymarkedsubs;
	    trans->u.context.cur_tag = cur->branches[i].branch->applycursubs;
return;
	}
    }

    if ( cur->ends_here!=NULL ) {
	trans->next_state = 0;
	trans->flags = 0x4000;
	trans->u.context.mark_tag = cur->applymarkedsubs;
	trans->u.context.cur_tag = cur->applycursubs;
    } else
	trans->next_state = 0;
}

static struct asm_state *AnyActiveSubstrings(struct contexttree *tree,
	struct contexttree *cur,int class, struct asm_state *trans, int classcnt) {
    struct fpc *any = &cur->rules[0].rule->u.class;
    int i,rc,j, b;

    for ( i=1; i<=cur->depth; ++i ) {
	for ( rc=0; rc<tree->rule_cnt; ++rc ) {
	    struct fpc *r = &tree->rules[rc].rule->u.class;
	    int ok = true;
	    for ( j=0; j<=cur->depth-i; ++j ) {
		if ( any->allclasses[j+i]!=r->allclasses[j] ) {
		    ok = false;
	    break;
		}
	    }
	    if ( ok && r->allclasses[j]==class ) {
		struct contexttree *sub = tree;
		for ( j=0; j<=cur->depth-i; ++j ) {
		    for ( b=0; b<sub->branch_cnt; ++b ) {
			if ( sub->branches[b].classnum==r->allclasses[j] ) {
			    sub = sub->branches[b].branch;
		    break;
			}
		    }
		}
		if ( trans[sub->state*classcnt+class+3].next_state!=0 &&
			(sub->pending_pos+i == cur->pending_pos ||
			 sub->pending_pos == -1 ))
return( &trans[sub->state*classcnt+class+3] );
	    }
	}
    }
return( NULL );
}

static int FailureTrans( struct asm_state *trans ) {
return( trans->next_state==0 &&
	    trans->u.context.mark_tag==0 &&
	    trans->u.context.cur_tag==0 );
}

static ASM *ASMFromClassFPST(SplineFont *sf,FPST *fpst, struct contexttree *tree) {
    ASM *sm;
    struct contexttree *cur;
    int i;

    sm = chunkalloc(sizeof(ASM));
    sm->type = asm_context;
    sm->flags = (fpst->flags&pst_r2l) ? asm_descending : 0;
    /* mac class sets have four magic classes, opentype sets only have one */
    sm->class_cnt = (fpst->flags&pst_ignorecombiningmarks) ? fpst->nccnt+4 : fpst->nccnt+3;
    sm->classes = galloc(sm->class_cnt*sizeof(char *));
    sm->classes[0] = sm->classes[1] = sm->classes[2] = sm->classes[3] = NULL;
    for ( i=1; i<fpst->nccnt; ++i )
	sm->classes[i+3] = copy(fpst->nclass[i]);
    if ( fpst->flags&pst_ignorecombiningmarks )
	sm->classes[sm->class_cnt-1] = BuildMarkClass(sf);

    /* Now build the state machine */
    sm->state_cnt = tree->next_state;
    sm->state = gcalloc(sm->state_cnt*sm->class_cnt,sizeof(struct asm_state));
    for ( cur=tree; cur!=NULL; cur = TreeNext(cur)) if ( cur->state!=0 ) {
	int off = cur->state*sm->class_cnt;

	SMSetState(&sm->state[off+1],cur,0);		/* Out of bounds state */
	sm->state[off+2].next_state = cur->state;	/* Deleted glyph gets eaten and ignored */
	if ( fpst->flags&pst_ignorecombiningmarks )
	    sm->state[off+sm->class_cnt-1].next_state = cur->state;	/* As do ignored marks */
	for ( i=1; i<fpst->nccnt; ++i )
	    SMSetState(&sm->state[off+i+3],cur,i);
    }
    /* Class 0 and class 1 should be the same. We only filled in class 1 above*/
    memcpy(sm->state,sm->state+sm->class_cnt,sm->class_cnt*sizeof(struct asm_state));
    /* Do a sort of transitive closure on states, so if we are looking for */
    /*  either "abcd" or "bce", don't lose the "bce" inside "abce" */
    FPSTBuildAllClasses(fpst);
    for ( cur = tree; cur!=NULL; cur = TreeNext(cur)) if ( cur->state>1 ) {
	int off = cur->state*sm->class_cnt;
	for ( i=1; i<fpst->nccnt; ++i ) if ( FailureTrans(&sm->state[off+3+i]) ) {
	    struct asm_state *trans =
		    AnyActiveSubstrings(tree,cur,i, sm->state,sm->class_cnt);
	    if ( trans!=NULL )
		sm->state[off+3+i] = *trans;
	}
    }
    FPSTFreeAllClasses(fpst);
return( sm );
}

ASM *ASMFromFPST(SplineFont *sf,FPST *fpst,int ordered) {
    FPST *tempfpst=fpst;
    struct contexttree *tree=NULL;
    ASM *sm;

    if ( fpst->format==pst_glyphs )
	tempfpst = FPSTGlyphToClass( fpst );
    if ( tempfpst->format==pst_coverage )
return( ASMFromCoverageFPST(sf,fpst,ordered));

    tree = FPST2Tree(sf, tempfpst);
    if ( tree!=NULL ) {
	sm = ASMFromClassFPST(sf,tempfpst,tree);
	TreeFree(tree);
    } else
	sm = NULL;
    if ( tempfpst!=fpst )
	FPSTFree(tempfpst);
    sm->opentype_tag = fpst->tag;
return( sm );
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
/* ************************************************************************** */
/* ************************* Opentype conversion dlg ************************ */
/* ************************************************************************** */
struct cvt_dlg {
    int done;
    ASM *ret;
    GGadget *list;
    SplineFont *sf;
};

#define CID_Convert	100

struct fs_dlg {
    int done, ok;
    int feature, setting;
    GTextInfo *mactags;
};

/*#define CID_FeatSet	306*/
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

struct sliflag *SFGetFormsList(SplineFont *sf,int test_dflt) {
    struct sliflag *sliflags;
    int cur, max;
    int i,j,k;
    SplineChar *sc;
    PST *pst;
    SplineFont *_sf;

    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;
    _sf = sf;
    cur = 0; max = 10;
    sliflags = galloc(11*sizeof(struct sliflag));

    k = 0;
    do {
	sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[k];
	for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL ) {
	    for ( pst = sc->possub; pst!=NULL; pst=pst->next ) {
		if ( pst->type == pst_lcaret )
	    continue;
		if ( pst->script_lang_index==SLI_NESTED )
	    continue;
		if ( test_dflt && !SLIHasDefault(sf,pst->script_lang_index))
	    continue;
		if ( pst->tag==CHR('i','n','i','t') ||
			pst->tag==CHR('m','e','d','i') ||
			pst->tag==CHR('f','i','n','a') ||
			pst->tag==CHR('i','s','o','l') ) {
		    for ( j=0; j<cur; ++j )
			if ( sliflags[j].sli==pst->script_lang_index &&
				sliflags[j].flags==pst->flags )
		    break;
		    if ( j>=cur ) {
			if ( cur>=max )
			    sliflags = grealloc(sliflags,((max+=10)+1)*sizeof(struct sliflag));
			sliflags[cur].sli = pst->script_lang_index;
			sliflags[cur++].flags = pst->flags;
		    }
		}
	    }
	}
	++k;
    } while ( k<_sf->subfontcnt );
    if ( cur==0 ) {
	free(sliflags);
return( NULL );
    }
    sliflags[cur].sli = 0xffff; sliflags[cur].flags = 0xffff;
return( sliflags );
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
/* A quick check to see if there is any opentype thing which is LIKELY to be */
/*  convertable to an apple contextual glyph substitution state machine */
int SFAnyConvertableSM(SplineFont *sf) {
    int i,k;
    SplineChar *sc;
    PST *pst;
    SplineFont *_sf;
    FPST *fpst;

    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;
    _sf = sf;

    for ( fpst=sf->possub; fpst!=NULL; fpst = fpst->next )
	if ( fpst->type==pst_contextsub || fpst->type==pst_chainsub )
return( true );

    k = 0;
    do {
	sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[k];
	for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL ) {
	    for ( pst = sc->possub; pst!=NULL; pst=pst->next ) {
		if ( pst->script_lang_index==SLI_NESTED )
	    continue;
		if ( pst->tag==CHR('i','n','i','t') ||
			pst->tag==CHR('m','e','d','i') ||
			pst->tag==CHR('f','i','n','a') ||
			pst->tag==CHR('i','s','o','l') )
return( true );
	    }
	}
	++k;
    } while ( k<_sf->subfontcnt );
return( false );
}

static GTextInfo *ConvertableItems(SplineFont *sf) {
    int max=10, cur=0;
    GTextInfo *ret = gcalloc((max+1),sizeof(GTextInfo));
    struct sliflag *sliflags = SFGetFormsList(sf,false);
    FPST *fpst;
    int i;
    static int types[] = { pst_contextsub, pst_chainsub };
    char buffer[100];

    if ( sliflags!=NULL ) {
	for ( i=0; sliflags[i].sli!=0xffff; ++i ) {
	    sprintf( buffer, "Forms (init,medi,fina,isol) %c%c%c%c %d",
		    sliflags[i].flags&pst_r2l ? 'r':' ',
		    sliflags[i].flags&pst_ignorebaseglyphs ? 'b':' ',
		    sliflags[i].flags&pst_ignoreligatures ? 'l':' ',
		    sliflags[i].flags&pst_ignorecombiningmarks ? 'm':' ',
    		    sliflags[i].sli );
	    if ( cur>=max ) {
		ret = grealloc(ret,((max+=10)+1)*sizeof(GTextInfo));
		memset(ret+max,0,11*sizeof(GTextInfo));
	    }
	    ret[cur].text = uc_copy(buffer);
	    ret[cur].checked = true;
	    ret[cur++].userdata = (void *) (intpt) ((sliflags[i].sli<<16)|sliflags[i].flags);
	}
	free(sliflags);
    }

    for ( i=0; i<2; ++i ) {
	if ( cur>=max ) {
	    ret = grealloc(ret,((max+=10)+1)*sizeof(GTextInfo));
	    memset(ret+max,0,11*sizeof(GTextInfo));
	}
	ret[cur].disabled = true;
	ret[cur++].line = true;

	for ( fpst=sf->possub; fpst!=NULL; fpst = fpst->next ) if ( fpst->type==types[i] ) {
	    if ( cur>=max ) {
		ret = grealloc(ret,((max+=10)+1)*sizeof(GTextInfo));
		memset(ret+max,0,11*sizeof(GTextInfo));
	    }
	    ret[cur].text = ClassName("",fpst->tag,fpst->flags,
		fpst->script_lang_index, -1, -1,false,sf);
	    ret[cur].checked = false;
	    ret[cur].disabled = !FPSTisMacable(sf,fpst,false);
	    ret[cur++].userdata = (void *) fpst;
	}
    }
    if ( cur==0 ) {
	free(ret);
return( NULL );
    }
return( ret );
}

static int fs_e_h(GWindow gw, GEvent *e) {
    if ( e->type==et_close ) {
	struct fs_dlg *d = GDrawGetUserData(gw);
	d->done = true;
    } else if ( e->type == et_char ) {
return( false );
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct fs_dlg *d = GDrawGetUserData(gw);
	if ( (d->ok = GGadgetGetCid(e->u.control.g)) ) {
	    unichar_t *end;
	    const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(gw,CID_FeatSet));
	    if ( *ret=='<' ) ++ret;
	    d->feature = u_strtol(ret,&end,10);
	    for ( ; isspace(*end); ++end );
	    d->setting = u_strtol(end+1,&end,10);
	    for ( ; isspace(*end); ++end );
	    if ( *end=='>' ) ++end;
	    for ( ; isspace(*end); ++end );
	    if ( *end!='\0' ) {
		gwwv_post_error(_("Bad Feature Setting"),_("Bad Feature Setting"));
return( true );
	    }
	}
	d->done = true;
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	if ( e->u.control.u.tf_changed.from_pulldown!=-1 ) {
	    struct fs_dlg *d = GDrawGetUserData(gw);
	    uint32 tag = (uint32) (intpt) d->mactags[e->u.control.u.tf_changed.from_pulldown].userdata;
	    unichar_t ubuf[20];
	    char buf[20];
	    /* If they select something from the pulldown, don't show the human */
	    /*  readable form, instead show the numeric feature/setting */
	    sprintf( buf,"<%d,%d>", (int) (tag>>16), (int) (tag&0xffff) );
	    uc_strcpy(ubuf,buf);
	    GGadgetSetTitle(e->u.control.g,ubuf);
	}
    }
return( true );
}

static int GetFeatureSetting(SplineFont *sf, unichar_t *text,
	int *feature,int *setting) {
    struct fs_dlg d;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[8];
    GTextInfo label[8];
    int k;

    memset(&d,0,sizeof(d));

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Convert from OpenType...");
    pos.x = pos.y = 0;
    pos.width = GDrawPointsToPixels(NULL,200);
    pos.height = GDrawPointsToPixels(NULL,96);
    gw = GDrawCreateTopWindow(NULL,&pos,fs_e_h,&d,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    k=0;

    label[k].text = (unichar_t *) _("Feature/Setting for:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = 5;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;

    label[k].text = text;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;

    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.cid = CID_FeatSet;
    gcd[k].gd.u.list = d.mactags = AddMacFeatures(NULL,pst_max,sf);
    gcd[k++].creator = GListFieldCreate;

    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 30; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+26;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
    gcd[k].gd.cid = true;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = -30; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+3;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    gcd[k].gd.cid = false;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = gcd[k].gd.pos.y = 2;
    gcd[k].gd.pos.width = pos.width - 4;
    gcd[k].gd.pos.height = pos.height - 4;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels;
    gcd[k++].creator = GGroupCreate;

    GGadgetsCreate(gw,gcd);
    GDrawSetVisible(gw,true);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
    *feature = d.feature;
    *setting = d.setting;
return( d.ok );
}

static void _SMCVT_Convert(struct cvt_dlg *d) {
    ASM *sm, *last;
    int32 len,i;
    GTextInfo **ti = GGadgetGetList(d->list,&len);
    int feature,setting;

    for ( i=0; i<len; ++i ) if ( ti[i]->selected && !ti[i]->disabled ) {
	if ( !GetFeatureSetting(d->sf,ti[i]->text,&feature,&setting) )
    continue;
	if ( ti[i]->checked ) {
	    uint32 val = (intpt) ti[i]->userdata;
	    sm = ASMFromOpenTypeForms(d->sf,val>>16,val&0xffff);
	} else {
	    sm = ASMFromFPST(d->sf,(FPST *) (ti[i]->userdata),false);
	}
	sm->feature = feature; sm->setting = setting;
	if ( d->ret==NULL )
	    d->ret = sm;
	else
	    last->next = sm;
	last = sm;
    }
    d->done = true;
}

static int SMCVT_Convert(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct cvt_dlg *d = GDrawGetUserData(GGadgetGetWindow(g));
	_SMCVT_Convert(d);
    }
return( true );
}

static int SMCVT_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct cvt_dlg *d = GDrawGetUserData(GGadgetGetWindow(g));
	d->done = true;
    }
return( true );
}

static int SMCVT_Selected(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_Convert),
		GGadgetGetFirstListSelectedItem(g)>=0);
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_listdoubleclick ) {
	struct cvt_dlg *d = GDrawGetUserData(GGadgetGetWindow(g));
	_SMCVT_Convert(d);
    }
return( true );
}

static int cvt_e_h(GWindow gw, GEvent *e) {
    if ( e->type==et_close ) {
	struct cvt_dlg *d = GDrawGetUserData(gw);
	d->done = true;
    } else if ( e->type == et_char ) {
return( false );
    }
return( true );
}

ASM *SMConvertDlg(SplineFont *sf) {
    GTextInfo *cvts = ConvertableItems(sf);
    struct cvt_dlg d;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[8];
    GTextInfo label[8];
    int k;

    if ( cvts==NULL ) {
	gwwv_post_error(_("Nothing to Convert"),_("Nothing to Convert"));
return( NULL );
    }

    memset(&d,0,sizeof(d));
    d.sf = sf;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Convert from OpenType...");
    pos.x = pos.y = 0;
    pos.width = GDrawPointsToPixels(NULL,180);
    pos.height = GDrawPointsToPixels(NULL,200);
    gw = GDrawCreateTopWindow(NULL,&pos,cvt_e_h,&d,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    k=0;

    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = 5;
    gcd[k].gd.pos.width = 170;
    gcd[k].gd.pos.height = 150;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_list_multiplesel;
    gcd[k].gd.handle_controlevent = SMCVT_Selected;
    gcd[k].gd.u.list = cvts;
    gcd[k++].creator = GListCreate;

    label[k].text = (unichar_t *) _("Convert...");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 30; gcd[k].gd.pos.y = 168-3;
    gcd[k].gd.flags = gg_visible /*| gg_enabled*/ | gg_but_default;
    gcd[k].gd.handle_controlevent = SMCVT_Convert;
    gcd[k].gd.cid = CID_Convert;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = -30; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+3;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    gcd[k].gd.handle_controlevent = SMCVT_Cancel;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = gcd[k].gd.pos.y = 2;
    gcd[k].gd.pos.width = pos.width - 4;
    gcd[k].gd.pos.height = pos.height - 4;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels;
    gcd[k++].creator = GGroupCreate;

    GGadgetsCreate(gw,gcd);
    d.list = gcd[0].ret;
    GDrawSetVisible(gw,true);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
return( d.ret );
}


/* ************************************************************************** */
/* ****************************** Edit a State ****************************** */
/* ************************************************************************** */
GTextInfo indicverbs_list[] = {
    { (unichar_t *) N_("No Change"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Ax => xA"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("xD => Dx"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("AxD => DxA"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("ABx => xAB"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("ABx => xBA"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("xCD => CDx"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("xCD => DCx"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("AxCD => CDxA"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("AxCD => DCxA"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("ABxD => DxAB"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("ABxD => DxBA"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("ABxCD => CDxAB"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("ABxCD => CDxBA"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("ABxCD => DCxAB"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("ABxCD => DCxBA"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }
};

static char *copy_count(GWindow gw,int cid,int *cnt) {
    const unichar_t *u = _GGadgetGetTitle(GWidgetGetControl(gw,cid)), *upt;
    char *ret, *pt;
    int c;

    while ( *u==' ' ) ++u;
    if ( *u=='\0' ) {
	*cnt = 0;
return( NULL );
    }

    ret = pt = galloc(u_strlen(u)+1);
    c = 0;
    for ( upt=u; *upt; ) {
	if ( *upt==' ' ) {
	    while ( *upt==' ' ) ++upt;
	    if ( *upt!='\0' ) {
		++c;
		*pt++ = ' ';
	    }
	} else
	    *pt++ = *upt++;
    }
    *pt = '\0';
    *cnt = c+1;
return( ret );
}

static void SMD_Fillup(SMD *smd) {
    int state = smd->st_pos/smd->class_cnt;
    int class = smd->st_pos%smd->class_cnt;
    struct asm_state *this = &smd->states[smd->st_pos];
    char buffer[100], *temp;
    char buf[100];
    int j;
    GGadget *list = GWidgetGetControl( smd->gw, CID_Classes );
    GTextInfo *ti = GGadgetGetListItem(list,class);

    snprintf(buffer,sizeof(buffer)/sizeof(buffer[0]),
	    _("State %d,  %.40s"), state, (temp = u2utf8_copy(ti->text)) );
    free(temp);
    GGadgetSetTitle8(GWidgetGetControl(smd->editgw,CID_StateClass),buffer);
    sprintf(buf,"%d", this->next_state );
    GGadgetSetTitle8(GWidgetGetControl(smd->editgw,CID_NextState),buf);

    GGadgetSetChecked(GWidgetGetControl(smd->editgw,CID_Flag4000),
	    this->flags&0x4000?0:1);
    GGadgetSetChecked(GWidgetGetControl(smd->editgw,CID_Flag8000),
	    this->flags&0x8000?1:0);
    if ( smd->sm->type==asm_indic ) {
	GGadgetSetChecked(GWidgetGetControl(smd->editgw,CID_Flag2000),
		this->flags&0x2000?1:0);
	GGadgetSelectOneListItem(GWidgetGetControl(smd->editgw,CID_IndicVerb),
		this->flags&0xf);
    } else if ( smd->sm->type==asm_insert ) {
	GGadgetSetChecked(GWidgetGetControl(smd->editgw,CID_Flag2000),
		this->flags&0x2000?1:0);
	GGadgetSetChecked(GWidgetGetControl(smd->editgw,CID_Flag1000),
		this->flags&0x1000?1:0);
	GGadgetSetChecked(GWidgetGetControl(smd->editgw,CID_Flag0800),
		this->flags&0x0800?1:0);
	GGadgetSetChecked(GWidgetGetControl(smd->editgw,CID_Flag0400),
		this->flags&0x0400?1:0);
	temp = this->u.insert.mark_ins;
	buffer[0]='\0';
	GGadgetSetTitle8(GWidgetGetControl(smd->editgw,CID_InsMark),temp==NULL?buffer:temp);
	temp = this->u.insert.cur_ins;
	GGadgetSetTitle8(GWidgetGetControl(smd->editgw,CID_InsCur),temp==NULL?buffer:temp);
    } else if ( smd->sm->type==asm_kern ) {
	buf[0] = '\0';
	for ( j=0; j<this->u.kern.kcnt; ++j )
	    sprintf( buf+strlen(buf), "%d ", this->u.kern.kerns[j]);
	if ( buf[0]!='\0' && buf[strlen(buf)-1]==' ' )
	    buf[strlen(buf)-1] = '\0';
	GGadgetSetTitle8(GWidgetGetControl(smd->editgw,CID_Kerns),buf);
    } else {
	buffer[0] = this->u.context.mark_tag>>24;
	buffer[1] = (this->u.context.mark_tag>>16)&0xff;
	buffer[2] = (this->u.context.mark_tag>>8)&0xff;
	buffer[3] = (this->u.context.mark_tag)&0xff;
	buffer[4] = 0;
	GGadgetSetTitle8(GWidgetGetControl(smd->editgw,CID_TagMark),buffer);
	buffer[0] = this->u.context.cur_tag>>24;
	buffer[1] = (this->u.context.cur_tag>>16)&0xff;
	buffer[2] = (this->u.context.cur_tag>>8)&0xff;
	buffer[3] = (this->u.context.cur_tag)&0xff;
	buffer[4] = 0;
	GGadgetSetTitle8(GWidgetGetControl(smd->editgw,CID_TagCur),buffer);
    }

    GGadgetSetEnabled(GWidgetGetControl(smd->editgw,CID_Up), state!=0 );
    GGadgetSetEnabled(GWidgetGetControl(smd->editgw,CID_Left), class!=0 );
    GGadgetSetEnabled(GWidgetGetControl(smd->editgw,CID_Right), class<smd->class_cnt-1 );
    GGadgetSetEnabled(GWidgetGetControl(smd->editgw,CID_Down), state<smd->state_cnt-1 );
}

static int SMD_DoChange(SMD *smd) {
    struct asm_state *this = &smd->states[smd->st_pos];
    int err=false, ns, flags, cnt;
    char *mins, *cins;
    uint32 mtag, ctag;
    const unichar_t *ret;
    unichar_t *end;
    int16 kbuf[9];
    int kerns;
    int oddcomplain=false;

    ns = GetInt8(smd->editgw,CID_NextState,_("Next State:"),&err);
    if ( err )
return( false );
    flags = 0;
    if ( !GGadgetIsChecked(GWidgetGetControl(smd->editgw,CID_Flag4000)) ) flags |= 0x4000;
    if ( GGadgetIsChecked(GWidgetGetControl(smd->editgw,CID_Flag8000)) ) flags |= 0x8000;
    if ( smd->sm->type==asm_indic ) {
	if ( GGadgetIsChecked(GWidgetGetControl(smd->editgw,CID_Flag2000)) ) flags |= 0x2000;
	flags |= GGadgetGetFirstListSelectedItem(GWidgetGetControl(smd->editgw,CID_IndicVerb));
	this->next_state = ns;
	this->flags = flags;
    } else if ( smd->sm->type==asm_kern ) {
	ret = _GGadgetGetTitle(GWidgetGetControl(smd->editgw,CID_Kerns));
	kerns=0;
	while ( *ret!='\0' ) {
	    while ( *ret==' ' ) ++ret;
	    if ( *ret=='\0' )
	break;
	    kbuf[kerns] = u_strtol(ret,&end,10);
	    if ( end==ret ) {
		Protest8(_("Kern Values:"));
return( false );
	    } else if ( kerns>=8 ) {
		gwwv_post_error(_("Too Many Kerns"),_("At most 8 kerning values may be specified here"));
return( false );
	    } else if ( kbuf[kerns]&1 ) {
		kbuf[kerns] &= ~1;
		if ( !oddcomplain )
		    gwwv_post_notice(_("Kerning values must be even"),_("Kerning values must be even"));
		oddcomplain = true;
	    }
	    ++kerns;
	    ret = end;
	}
	this->next_state = ns;
	this->flags = flags;
	free(this->u.kern.kerns);
	this->u.kern.kcnt = kerns;
	if ( kerns==0 )
	    this->u.kern.kerns = NULL;
	else {
	    this->u.kern.kerns = galloc(kerns*sizeof(int16));
	    memcpy(this->u.kern.kerns,kbuf,kerns*sizeof(int16));
	}
    } else if ( smd->sm->type==asm_context ) {
	mtag = ctag = 0;
	ret = _GGadgetGetTitle(GWidgetGetControl(smd->editgw,CID_TagMark));
	if ( *ret=='\0' )
	    /* That's ok */;
	else if ( u_strlen(ret)!=4 ) {
	    gwwv_post_error(_("Tag must be 4 characters long"),_("Tag must be 4 characters long"));
return( false );
	} else
	    mtag = (ret[0]<<24)|(ret[1]<<16)|(ret[2]<<8)|ret[3];
	ret = _GGadgetGetTitle(GWidgetGetControl(smd->editgw,CID_TagCur));
	if ( *ret=='\0' )
	    /* That's ok */;
	else if ( u_strlen(ret)!=4 ) {
	    gwwv_post_error(_("Tag must be 4 characters long"),_("Tag must be 4 characters long"));
return( false );
	} else
	    ctag = (ret[0]<<24)|(ret[1]<<16)|(ret[2]<<8)|ret[3];
	this->next_state = ns;
	this->flags = flags;
	this->u.context.mark_tag = mtag;
	this->u.context.cur_tag = ctag;
    } else {
	char *foo;

	if ( GGadgetIsChecked(GWidgetGetControl(smd->editgw,CID_Flag2000)) ) flags |= 0x2000;
	if ( GGadgetIsChecked(GWidgetGetControl(smd->editgw,CID_Flag1000)) ) flags |= 0x1000;
	if ( GGadgetIsChecked(GWidgetGetControl(smd->editgw,CID_Flag0800)) ) flags |= 0x0800;
	if ( GGadgetIsChecked(GWidgetGetControl(smd->editgw,CID_Flag0400)) ) flags |= 0x0400;

	foo = GGadgetGetTitle8(GWidgetGetControl(smd->editgw,CID_InsMark));
	if ( !CCD_NameListCheck(smd->sf,foo,false,_("Missing Glyph Name"))) {
	    free(foo);
return( false );
	}
	free(foo);

	mins = copy_count(smd->editgw,CID_InsMark,&cnt);
	if ( cnt>31 ) {
	    gwwv_post_error(_("Too Many Glyphs"),_("At most 31 glyphs may be specified in an insert list"));
	    free(mins);
return( false );
	}
	flags |= cnt<<5;

	foo = GGadgetGetTitle8(GWidgetGetControl(smd->editgw,CID_InsCur));
	if ( !CCD_NameListCheck(smd->sf,foo,false,_("Missing Glyph Name"))) {
	    free(foo);
return( false );
	}
	free(foo);
	cins = copy_count(smd->editgw,CID_InsCur,&cnt);
	if ( cnt>31 ) {
	    gwwv_post_error(_("Too Many Glyphs"),_("At most 31 glyphs may be specified in an insert list"));
	    free(mins);
	    free(cins);
return( false );
	}
	flags |= cnt;
	this->next_state = ns;
	this->flags = flags;
	free(this->u.insert.mark_ins);
	free(this->u.insert.cur_ins);
	this->u.insert.mark_ins = mins;
	this->u.insert.cur_ins = cins;
    }

    /* Show changes in main window */
    GDrawRequestExpose(smd->gw,NULL,false);
return( true );
}

static int SMDE_Arrow(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	SMD *smd = GDrawGetUserData(GGadgetGetWindow(g));
	int state = smd->st_pos/smd->class_cnt;
	int class = smd->st_pos%smd->class_cnt;
	switch( GGadgetGetCid(g)) {
	  case CID_Up:
	    if ( state!=0 ) --state;
	  break;
	  case CID_Left:
	    if ( class!=0 ) --class;
	  break;
	  case CID_Right:
	    if ( class<smd->class_cnt-1 ) ++class;
	  break;
	  case CID_Down:
	    if ( state<smd->state_cnt-1 ) ++state;
	  break;
	}
	if ( state!=smd->st_pos/smd->class_cnt || class!=smd->st_pos%smd->class_cnt ) {
	    if ( SMD_DoChange(smd)) {
		smd->st_pos = state*smd->class_cnt+class;
		SMD_Fillup(smd);
	    }
	}
    }
return( true );
}

static int smdedit_e_h(GWindow gw, GEvent *event) {
    SMD *smd = GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_close:
	smd->edit_done = true;
	smd->edit_ok = false;
      break;
      case et_char:
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("statemachine.html#EditTransition");
return( true );
	} else if ( event->u.chr.keysym == GK_Escape ) {
	    smd->edit_done = true;
return( true );
	} else if ( event->u.chr.chars[0] == '\r' ) {
	    smd->edit_done = SMD_DoChange(smd);
return( true );
	}
return( false );
      case et_controlevent:
	switch( event->u.control.subtype ) {
	  case et_buttonactivate:
	    if ( GGadgetGetCid(event->u.control.g)==CID_Ok )
		smd->edit_done = SMD_DoChange(smd);
	    else
		smd->edit_done = true;
	  break;
	}
      break;
    }
return( true );
}

static void SMD_EditState(SMD *smd) {
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[23];
    GTextInfo label[23];
    GRect pos;
    int k, listk, new_cnt;
    char stateclass[100];
    static int indicv_done = false;

    smd->edit_done = false;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = true;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Edit State Transition");
    pos.x = pos.y = 0;
    pos.width = GDrawPointsToPixels(NULL,GGadgetScale(SMDE_WIDTH));
    pos.height = GDrawPointsToPixels(NULL,SMDE_HEIGHT);
    smd->editgw = gw = GDrawCreateTopWindow(NULL,&pos,smdedit_e_h,smd,&wattrs);

    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));
    k = 0; listk = -1;

    snprintf(stateclass,sizeof(stateclass), _("State %d,  %.40s"),
	    999, _("Class 1: {Everything Else}" ));
    label[k].text = (unichar_t *) stateclass;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = 5;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.cid = CID_StateClass;
    gcd[k++].creator = GLabelCreate;

    label[k].text = (unichar_t *) _("Next State:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+13+4;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k++].creator = GLabelCreate;

    gcd[k].gd.pos.x = 80; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.cid = CID_NextState;
    gcd[k++].creator = GTextFieldCreate;

    label[k].text = (unichar_t *) _("Advance To Next Glyph");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+24;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.cid = CID_Flag4000;
    gcd[k++].creator = GCheckBoxCreate;

    label[k].text = (unichar_t *) (smd->sm->type==asm_kern?_("Push Current Glyph"):
				   smd->sm->type!=asm_indic?_("Mark Current Glyph"):
					    _("Mark Current Glyph As First"));
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+16;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.cid = CID_Flag8000;
    gcd[k++].creator = GCheckBoxCreate;

    if ( smd->sm->type==asm_indic ) {
	if ( !indicv_done ) {
	    int i;
	    for ( i=0; indicverbs_list[i].text!=NULL; ++i )
		indicverbs_list[i].text = (unichar_t *) _((char *) indicverbs_list[i].text );
	    indicv_done = true;
	}
	label[k].text = (unichar_t *) _("Mark Current Glyph As Last");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+16;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_Flag2000;
	gcd[k++].creator = GCheckBoxCreate;

	gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+24;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.u.list = indicverbs_list;
	gcd[k].gd.cid = CID_IndicVerb;
	gcd[k++].creator = GListButtonCreate;
    } else if ( smd->sm->type==asm_insert ) {
	label[k].text = (unichar_t *) _("Current Glyph Is Kashida Like");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+16;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_Flag2000;
	gcd[k++].creator = GCheckBoxCreate;

	label[k].text = (unichar_t *) _("Marked Glyph Is Kashida Like");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+16;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_Flag1000;
	gcd[k++].creator = GCheckBoxCreate;

	label[k].text = (unichar_t *) _("Insert Before Current Glyph");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+16;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_Flag0800;
	gcd[k++].creator = GCheckBoxCreate;

	label[k].text = (unichar_t *) _("Insert Before Marked Glyph");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+16;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_Flag0400;
	gcd[k++].creator = GCheckBoxCreate;

	label[k].text = (unichar_t *) _("Mark Insert:");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+26;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k++].creator = GLabelCreate;

	gcd[k].gd.pos.x = gcd[2].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_InsMark;
	gcd[k++].creator = GTextFieldCreate;

	label[k].text = (unichar_t *) _("Current Insert:");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+30;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k++].creator = GLabelCreate;

	gcd[k].gd.pos.x = gcd[2].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_InsCur;
	gcd[k++].creator = GTextFieldCreate;
    } else if ( smd->sm->type==asm_kern ) {
	label[k].text = (unichar_t *) _("Kern Values:");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+26;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k++].creator = GLabelCreate;

	gcd[k].gd.pos.x = gcd[2].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_Kerns;
	gcd[k++].creator = GTextFieldCreate;
    } else {
	label[k].text = (unichar_t *) _("Mark Subs:");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+26;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k++].creator = GLabelCreate;

	listk = k;
	gcd[k].gd.pos.x = gcd[2].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_TagMark;
	gcd[k++].creator = GListFieldCreate;

	label[k].text = (unichar_t *) _("Current Subs:");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+30;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k++].creator = GLabelCreate;

	gcd[k].gd.pos.x = gcd[2].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_TagCur;
	gcd[k].gd.u.list = gcd[k-2].gd.u.list;
	gcd[k++].creator = GListFieldCreate;
    }

    label[k].text = (unichar_t *) U_("_Up↑");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = (SMDE_WIDTH-GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor))/2;
    gcd[k].gd.pos.y = SMDE_HEIGHT-SMD_DIRDROP;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled;
    gcd[k].gd.cid = CID_Up;
    gcd[k].gd.handle_controlevent = SMDE_Arrow;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) U_("←_Left");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = SMDE_HEIGHT-SMD_DIRDROP+13;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled;
    gcd[k].gd.cid = CID_Left;
    gcd[k].gd.handle_controlevent = SMDE_Arrow;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) U_("_Right→");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = -10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled;
    gcd[k].gd.cid = CID_Right;
    gcd[k].gd.handle_controlevent = SMDE_Arrow;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) U_("↓_Down");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-3].gd.pos.x; gcd[k].gd.pos.y = SMDE_HEIGHT-SMD_DIRDROP+26;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled;
    gcd[k].gd.cid = CID_Down;
    gcd[k].gd.handle_controlevent = SMDE_Arrow;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 30-3; gcd[k].gd.pos.y = SMDE_HEIGHT-SMD_CANCELDROP-3;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled | gg_but_default;
    gcd[k].gd.cid = CID_Ok;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = -30; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+3;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled | gg_but_cancel;
    gcd[k].gd.cid = CID_Cancel;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = 2; gcd[k].gd.pos.y = 2;
    gcd[k].gd.pos.width = pos.width-4;
    gcd[k].gd.pos.height = pos.height-4;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels;
    gcd[k++].creator = GGroupCreate;

    GGadgetsCreate(gw,gcd);

    if ( listk!=-1 ) {
	GGadgetSetList(gcd[listk].ret,
		SFGenTagListFromType(&smd->sf->gentags,pst_substitution),false);
	GGadgetSetList(gcd[listk+2].ret,
		SFGenTagListFromType(&smd->sf->gentags,pst_substitution),false);
    }

    SMD_Fillup(smd);

    GDrawSetVisible(gw,true);

    smd->edit_done = false;
    while ( !smd->edit_done )
	GDrawProcessOneEvent(NULL);

    new_cnt = FindMaxReachableStateCnt(smd);
    if ( new_cnt!=smd->state_cnt ) {
	smd->states = StateCopy(smd->states,smd->class_cnt,smd->state_cnt,
		smd->class_cnt,new_cnt,
		smd->sm->type,true);
	smd->state_cnt = new_cnt;
	SMD_SBReset(smd);
	GDrawRequestExpose(smd->gw,NULL,false);
    }
    smd->st_pos = -1;
    GDrawDestroyWindow(gw);
return;
}

/* ************************************************************************** */
/* **************************** Edit/Add a Class **************************** */
/* ************************************************************************** */

static int SMD_ToSelection(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	SMD *smd = GDrawGetUserData(GGadgetGetWindow(g));
	const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(smd->gw,CID_GlyphList));
	SplineFont *sf = smd->sf;
	FontView *fv = sf->fv;
	const unichar_t *end;
	int pos, found=-1;
	char *nm;

	GDrawSetVisible(fv->gw,true);
	GDrawRaise(fv->gw);
	memset(fv->selected,0,fv->map->enccount);
	while ( *ret ) {
	    end = u_strchr(ret,' ');
	    if ( end==NULL ) end = ret+u_strlen(ret);
	    nm = cu_copybetween(ret,end);
	    for ( ret = end; isspace(*ret); ++ret);
	    if (( pos = SFFindSlot(sf,fv->map,-1,nm))!=-1 ) {
		if ( found==-1 ) found = pos;
		fv->selected[pos] = true;
	    }
	    free(nm);
	}

	if ( found!=-1 )
	    FVScrollToChar(fv,found);
	GDrawRequestExpose(fv->v,NULL,false);
    }
return( true );
}

static int SMD_FromSelection(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	SMD *smd = GDrawGetUserData(GGadgetGetWindow(g));
	SplineFont *sf = smd->sf;
	FontView *fv = sf->fv;
	unichar_t *vals, *pt;
	int i, len, max;
	SplineChar *sc;
    
	for ( i=len=max=0; i<fv->map->enccount; ++i ) if ( fv->selected[i]) {
	    sc = SFMakeChar(sf,fv->map,i);
	    len += strlen(sc->name)+1;
	    if ( fv->selected[i]>max ) max = fv->selected[i];
	}
	pt = vals = galloc((len+1)*sizeof(unichar_t));
	*pt = '\0';
	/* in a class the order of selection is irrelevant */
	for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] ) {
	    int gid = fv->map->map[i];
	    if ( gid!=-1 ) {
		uc_strcpy(pt,sf->glyphs[gid]->name);
		pt += u_strlen(pt);
		*pt++ = ' ';
	    }
	}
	if ( pt>vals ) pt[-1]='\0';
    
	GGadgetSetTitle(GWidgetGetControl(smd->gw,CID_GlyphList),vals);
    }
return( true );
}

static int SMD_Prev(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	SMD *smd = GDrawGetUserData(GGadgetGetWindow(g));
	GDrawSetVisible(smd->cw,false);
    }
return( true );
}

static unichar_t *AddClass(int class_num,char *text,int freetext) {
    char buf[80];
    unichar_t *ret;

    snprintf(buf,sizeof(buf)/sizeof(buf[0]), _("Class %d"),class_num);
    ret = galloc((strlen(buf)+strlen(text)+4)*sizeof(unichar_t));
    utf82u_strcpy(ret,buf);
    uc_strcat(ret,": ");
    utf82u_strcpy(ret+u_strlen(ret),text);
    if ( freetext )
	free((unichar_t *) text);
return( ret );
}


static unichar_t *UAddClass(int class_num,const unichar_t *text,int freetext) {
    char buf[80];
    unichar_t *ret;

    snprintf(buf,sizeof(buf)/sizeof(buf[0]), _("Class %d"),class_num);
    ret = galloc((strlen(buf)+u_strlen(text)+4)*sizeof(unichar_t));
    utf82u_strcpy(ret,buf);
    uc_strcat(ret,": ");
    u_strcat(ret,text);
    if ( freetext )
	free((unichar_t *) text);
return( ret );
}
static int SMD_Next(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	SMD *smd = GDrawGetUserData(GGadgetGetWindow(g));
	char *ret = GGadgetGetTitle8(GWidgetGetControl(smd->gw,CID_GlyphList));
	unichar_t *temp;
	GGadget *list = GWidgetGetControl( smd->gw, CID_Classes );
	int32 len;

	if ( !CCD_NameListCheck(smd->sf,ret,true,_("Bad Class")) ||
		CCD_InvalidClassList(ret,list,smd->isedit)) {
	    free(ret);
return( true );
	}

	if ( smd->isedit ) {
	    int cn = GGadgetGetFirstListSelectedItem(list);
	    temp = AddClass(cn,ret,false);
	    GListChangeLine(list,cn,temp);
	} else {
	    GGadgetGetList(list,&len);
	    temp = AddClass(len,ret,false);
	    GListAppendLine(list,temp,false);
	    smd->states = StateCopy(smd->states,smd->class_cnt,smd->state_cnt,
		    smd->class_cnt+1,smd->state_cnt,
		    smd->sm->type,true);
	    ++smd->class_cnt;
	    SMD_SBReset(smd);
	}
	GDrawSetVisible(smd->cw,false);		/* This will give us an expose so we needed ask for one */
	free(temp);
    }
return( true );
}

static void _SMD_DoEditNew(SMD *smd,int isedit) {
    static unichar_t nullstr[] = { 0 };
    unichar_t *upt;

    smd->isedit = isedit;
    if ( isedit ) {
	GTextInfo *selected = GGadgetGetListItemSelected(GWidgetGetControl(
		smd->gw, CID_Classes));
	if ( selected==NULL )
return;
	upt = uc_strstr(selected->text,": ");
	if ( upt==NULL ) upt = selected->text;
	else upt += 2;
	GGadgetSetTitle(GWidgetGetControl(smd->cw,CID_GlyphList),upt);
    } else {
	GGadgetSetTitle(GWidgetGetControl(smd->cw,CID_GlyphList),nullstr);
    }
    GDrawSetVisible(smd->cw,true);
}

/* ************************************************************************** */
/* ****************************** Main Dialog ******************************* */
/* ************************************************************************** */

static void _SMD_EnableButtons(SMD *smd) {
    GGadget *list = GWidgetGetControl(smd->gw,CID_Classes);
    int32 i, len, j;
    GTextInfo **ti;

    ti = GGadgetGetList(list,&len);
    i = GGadgetGetFirstListSelectedItem(list);
    GGadgetSetEnabled(GWidgetGetControl(smd->gw,CID_Delete),i>=4);
    for ( j=i+1; j<len; ++j )
	if ( ti[j]->selected )
    break;
    GGadgetSetEnabled(GWidgetGetControl(smd->gw,CID_Edit),i>=4 && j==len);
}

static void SMD_GListDelSelected(GGadget *list) {
    int32 len; int i,j;
    GTextInfo **old, **new;
    unichar_t *upt;

    old = GGadgetGetList(list,&len);
    new = gcalloc(len+1,sizeof(GTextInfo *));
    for ( i=j=0; i<len; ++i ) if ( !old[i]->selected ) {
	new[j] = galloc(sizeof(GTextInfo));
	*new[j] = *old[i];
	upt = uc_strstr(new[j]->text,": ");
	if ( upt==NULL ) upt = new[j]->text; else upt += 2;
	new[j]->text = UAddClass(j,upt,false);
	++j;
    }
    new[j] = gcalloc(1,sizeof(GTextInfo));
    GGadgetSetList(list,new,false);
}

static int SMD_Delete(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	SMD *smd = GDrawGetUserData(GGadgetGetWindow(g));
	GGadget *list = GWidgetGetControl(smd->gw,CID_Classes);
	int32 len;

	StateRemoveClasses(smd,GGadgetGetList(list,&len));
	SMD_GListDelSelected(list);
	_SMD_EnableButtons(smd);
	SMD_SBReset(smd);
	GDrawRequestExpose(smd->gw,NULL,false);
    }
return( true );
}

static int SMD_Edit(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	SMD *smd = GDrawGetUserData(GGadgetGetWindow(g));
	if ( GGadgetGetFirstListSelectedItem(GWidgetGetControl(smd->gw,CID_Classes))>0 )
	    _SMD_DoEditNew(smd,true);
    }
return( true );
}

static int SMD_New(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	SMD *smd = GDrawGetUserData(GGadgetGetWindow(g));
	_SMD_DoEditNew(smd,false);
    }
return( true );
}

static int SMD_ClassSelected(GGadget *g, GEvent *e) {
    SMD *smd = GDrawGetUserData(GGadgetGetWindow(g));
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	_SMD_EnableButtons(smd);
	SMD_HShow(smd,GGadgetGetFirstListSelectedItem(g));
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_listdoubleclick ) {
	if ( GGadgetGetFirstListSelectedItem(g)>0 )
	    _SMD_DoEditNew(smd,true);
    }
return( true );
}

static void _SMD_Finish(SMD *smd, int success) {

    GDrawDestroyWindow(smd->gw);

    GFI_FinishSMNew(smd->d,smd->sm,success,smd->isnew);
    GFI_SMDEnd(smd->d);

    GTextInfoListFree(smd->mactags);
    free(smd);
}

static void _SMD_Cancel(SMD *smd) {

    StatesFree(smd->states,smd->state_cnt,smd->class_cnt,smd->sm->type);
    _SMD_Finish(smd,false);
}

static int SMD_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	SMD *smd = GDrawGetUserData(GGadgetGetWindow(g));

	if ( GDrawIsVisible(smd->cw))
return( SMD_Prev(g,e));

	_SMD_Cancel(smd);
    }
return( true );
}

static int SMD_Ok(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	SMD *smd = GDrawGetUserData(GGadgetGetWindow(g));
	const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(smd->gw,CID_FeatSet));
	unichar_t *end;
	int feat, set, i;
	int32 len;
	GTextInfo **ti = GGadgetGetList(GWidgetGetControl(smd->gw,CID_Classes),&len);
	ASM *sm = smd->sm;
	unichar_t *upt;

	if ( GDrawIsVisible(smd->cw))
return( SMD_Next(g,e));

	if ( *ret=='<' )
	    ++ret;
	feat = u_strtol(ret,&end,10);
	if ( *end==',' && ( set = u_strtol(end+1,&end,10), *end=='>' || *end=='\0')) {
	    sm->feature = feat;
	    sm->setting = set;
	} else if ( sm->type==asm_kern ) {	/* Kerns don't get feature/settings */
	    sm->feature = sm->setting = 0xffff;
	} else {
	    gwwv_post_error(_("Bad Feature Setting"),_("Bad Feature Setting"));
return( true );
	}
	
	for ( i=4; i<sm->class_cnt; ++i )
	    free(sm->classes[i]);
	free(sm->classes);
	sm->classes = galloc(smd->class_cnt*sizeof(char *));
	sm->classes[0] = sm->classes[1] = sm->classes[2] = sm->classes[3] = NULL;
	sm->class_cnt = smd->class_cnt;
	for ( i=4; i<sm->class_cnt; ++i ) {
	    upt = uc_strstr(ti[i]->text,": ");
	    if ( upt==NULL ) upt = ti[i]->text; else upt += 2;
	    sm->classes[i] = cu_copy(upt);
	}

	StatesFree(sm->state,sm->state_cnt,sm->class_cnt,
		sm->type);
	sm->state_cnt = smd->state_cnt;
	sm->state = smd->states;
	sm->flags = (sm->flags & ~0xc000) |
		(GGadgetIsChecked(GWidgetGetControl(smd->gw,CID_RightToLeft))?0x4000:0) |
		(GGadgetIsChecked(GWidgetGetControl(smd->gw,CID_VertOnly))?0x8000:0);
	_SMD_Finish(smd,true);
    }
return( true );
}

static void SMD_Mouse(SMD *smd,GEvent *event) {
    static unichar_t space[100];
    unichar_t *upt;
    char buf[30];
    int32 len;
    GTextInfo **ti;
    int pos = ((event->u.mouse.y-smd->ystart2)/smd->stateh+smd->offtop) * smd->class_cnt +
	    (event->u.mouse.x-smd->xstart2)/smd->statew + smd->offleft;

    GGadgetEndPopup();

    if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button==4 || event->u.mouse.button==5) ) {
	GGadgetDispatchEvent(smd->vsb,event);
return;
    }
    
    if ( event->u.mouse.x<smd->xstart || event->u.mouse.x>smd->xstart2+smd->width ||
	    event->u.mouse.y<smd->ystart || event->u.mouse.y>smd->ystart2+smd->height )
return;

    if ( event->type==et_mousemove ) {
	int c = (event->u.mouse.x - smd->xstart2)/smd->statew + smd->offleft;
	int s = (event->u.mouse.y - smd->ystart2)/smd->stateh + smd->offtop;
	space[0] = '\0';
	if ( event->u.mouse.y>=smd->ystart2 && s<smd->state_cnt ) {
	    sprintf( buf, "State %d\n", s );
	    uc_strcpy(space,buf);
	}
	if ( event->u.mouse.x>=smd->xstart2 && c<smd->class_cnt ) {
	    sprintf( buf, "Class %d\n", c );
	    uc_strcat(space,buf);
	    ti = GGadgetGetList(GWidgetGetControl(smd->gw,CID_Classes),&len);
	    len = u_strlen(space);
	    upt = uc_strstr(ti[c]->text,": ");
	    if ( upt==NULL ) upt = ti[c]->text;
	    else upt += 2;
	    u_strncpy(space+len,upt,(sizeof(space)/sizeof(space[0]))-1 - len);
	} else if ( event->u.mouse.x<smd->xstart2 ) {
	    if ( s==0 )
		utf82u_strcat(space,_("{Start of Input}"));
	    else if ( s==1 )
		utf82u_strcat(space,_("{Start of Line}"));
	}
	if ( space[0]=='\0' )
return;
	if ( space[u_strlen(space)-1]=='\n' )
	    space[u_strlen(space)-1]='\0';
	GGadgetPreparePopup(smd->gw,space);
    } else if ( event->u.mouse.x<smd->xstart2 || event->u.mouse.y<smd->ystart2 )
return;
    else if ( event->type==et_mousedown )
	smd->st_pos = pos;
    else if ( event->type==et_mouseup ) {
	if ( pos==smd->st_pos )
	    SMD_EditState(smd);
    }
}

static void SMD_Expose(SMD *smd,GWindow pixmap,GEvent *event) {
    GRect *area = &event->u.expose.rect;
    GRect rect;
    GRect clip,old1,old2;
    int len, off, i, j, x, y, kddd=false;
    unichar_t ubuf[8];
    char buf[100];

    if ( area->y+area->height<smd->ystart )
return;
    if ( area->y>smd->ystart2+smd->height )
return;

    GDrawPushClip(pixmap,area,&old1);
    GDrawSetFont(pixmap,smd->font);
    GDrawSetLineWidth(pixmap,0);
    rect.x = smd->xstart; rect.y = smd->ystart;
    rect.width = smd->width+(smd->xstart2-smd->xstart);
    rect.height = smd->height+(smd->ystart2-smd->ystart);
    clip = rect;
    GDrawPushClip(pixmap,&clip,&old2);
    for ( i=0 ; smd->offtop+i<=smd->state_cnt && (i-1)*smd->stateh<smd->height; ++i ) {
	GDrawDrawLine(pixmap,smd->xstart,smd->ystart2+i*smd->stateh,smd->xstart+rect.width,smd->ystart2+i*smd->stateh,
		0x808080);
	if ( i+smd->offtop<smd->state_cnt ) {
	    sprintf( buf, i+smd->offtop<100 ? "St%d" : "%d", i+smd->offtop );
	    len = strlen( buf );
	    off = (smd->stateh-len*smd->fh)/2;
	    for ( j=0; j<len; ++j ) {
		ubuf[0] = buf[j];
		GDrawDrawText(pixmap,smd->xstart+3,smd->ystart2+i*smd->stateh+off+j*smd->fh+smd->as,
		    ubuf,1,NULL,0xff0000);
	    }
	}
    }
    for ( i=0 ; smd->offleft+i<=smd->class_cnt && (i-1)*smd->statew<smd->width; ++i ) {
	GDrawDrawLine(pixmap,smd->xstart2+i*smd->statew,smd->ystart,smd->xstart2+i*smd->statew,smd->ystart+rect.height,
		0x808080);
	if ( i+smd->offleft<smd->class_cnt ) {
	    uc_strcpy(ubuf,"Class");
	    GDrawDrawText(pixmap,smd->xstart2+i*smd->statew+1,smd->ystart+smd->as+1,
		ubuf,-1,NULL,0xff0000);
	    sprintf( buf, "%d", i+smd->offleft );
	    uc_strcpy(ubuf,buf);
	    len = GDrawGetTextWidth(pixmap,ubuf,-1,NULL);
	    GDrawDrawText(pixmap,smd->xstart2+i*smd->statew+(smd->statew-len)/2,smd->ystart+smd->fh+smd->as+1,
		ubuf,-1,NULL,0xff0000);
	}
    }

    for ( i=0 ; smd->offtop+i<smd->state_cnt && (i-1)*smd->stateh<smd->height; ++i ) {
	y = smd->ystart2+i*smd->stateh;
	if ( y>area->y+area->height )
    break;
	if ( y+smd->stateh<area->y )
    continue;
	for ( j=0 ; smd->offleft+j<smd->class_cnt && (j-1)*smd->statew<smd->width; ++j ) {
	    struct asm_state *this = &smd->states[(i+smd->offtop)*smd->class_cnt+j+smd->offleft];
	    x = smd->xstart2+j*smd->statew;
	    if ( x>area->x+area->width )
	break;
	    if ( x+smd->statew<area->x )
	continue;

	    sprintf( buf, "%d", this->next_state );
	    uc_strcpy(ubuf,buf);
	    len = GDrawGetTextWidth(pixmap,ubuf,-1,NULL);
	    GDrawDrawText(pixmap,x+(smd->statew-len)/2,y+smd->as+1,
		ubuf,-1,NULL,0x000000);

	    ubuf[0] = (this->flags&0x8000)? 'M' : ' ';
	    if ( smd->sm->type==asm_kern && (this->flags&0x8000))
		ubuf[0] = 'P';
	    ubuf[1] = (this->flags&0x4000)? ' ' : 'A';
	    ubuf[2] = '\0';
	    if ( smd->sm->type==asm_indic ) {
		ubuf[2] = (this->flags&0x2000) ? 'L' : ' ';
		ubuf[3] = '\0';
	    }
	    len = GDrawGetTextWidth(pixmap,ubuf,-1,NULL);
	    GDrawDrawText(pixmap,x+(smd->statew-len)/2,y+smd->fh+smd->as+1,
		ubuf,-1,NULL,0x000000);

	    ubuf[0]='\0';
	    if ( smd->sm->type==asm_indic ) {
		uc_strcpy(ubuf,indicverbs[0][this->flags&0xf]);
	    } else if ( smd->sm->type==asm_context ) {
		ubuf[0] = this->u.context.mark_tag>>24;
		ubuf[1] = (this->u.context.mark_tag>>16)&0xff;
		ubuf[2] = (this->u.context.mark_tag>>8)&0xff;
		ubuf[3] = this->u.context.mark_tag&0xff;
		ubuf[4] = 0;
	    } else if ( smd->sm->type==asm_insert ) {
		ubuf[0] = '\0';
		if ( this->u.insert.mark_ins!=NULL )
		    uc_strncpy(ubuf,this->u.insert.mark_ins,5);
	    } else { /* kern */
		if ( this->u.kern.kerns!=NULL ) {
		    int j;
		    buf[0] = '\0';
		    for ( j=0; j<this->u.kern.kcnt; ++j )
			sprintf(buf+strlen(buf),"%d ", this->u.kern.kerns[j]);
		    kddd = ( strlen(buf)>5 );
		    buf[5] = '\0';
		    uc_strcpy(ubuf,buf);
		} else
		    kddd = false;
	    }
	    len = GDrawGetTextWidth(pixmap,ubuf,-1,NULL);
	    GDrawDrawText(pixmap,x+(smd->statew-len)/2,y+2*smd->fh+smd->as+1,
		ubuf,-1,NULL,0x000000);

	    if ( smd->sm->type==asm_indic ) {
		uc_strcpy(ubuf,indicverbs[1][this->flags&0xf]);
	    } else if ( smd->sm->type==asm_context ) {
		ubuf[0] = this->u.context.cur_tag>>24;
		ubuf[1] = (this->u.context.cur_tag>>16)&0xff;
		ubuf[2] = (this->u.context.cur_tag>>8)&0xff;
		ubuf[3] = this->u.context.cur_tag&0xff;
		ubuf[4] = 0;
	    } else if ( smd->sm->type==asm_insert ) {
		ubuf[0] = '\0';
		if ( this->u.insert.cur_ins!=NULL )
		    uc_strncpy(ubuf,this->u.insert.cur_ins,5);
	    } else { /* kern */
		if ( kddd ) uc_strcpy(ubuf,"...");
		else ubuf[0] = '\0';
	    }
	    len = GDrawGetTextWidth(pixmap,ubuf,-1,NULL);
	    GDrawDrawText(pixmap,x+(smd->statew-len)/2,y+3*smd->fh+smd->as+1,
		ubuf,-1,NULL,0x000000);
	}
    }

    GDrawDrawLine(pixmap,smd->xstart,smd->ystart2,smd->xstart+rect.width,smd->ystart2,
	    0x000000);
    GDrawDrawLine(pixmap,smd->xstart2,smd->ystart,smd->xstart2,smd->ystart+rect.height,
	    0x000000);
    GDrawPopClip(pixmap,&old2);
    GDrawPopClip(pixmap,&old1);
    GDrawDrawRect(pixmap,&rect,0x000000);
    rect.y += rect.height;
    rect.x += rect.width;
    LogoExpose(pixmap,event,&rect,dm_fore);
}

static int SMD_SBReset(SMD *smd) {
    int oldtop = smd->offtop, oldleft = smd->offleft;

    GScrollBarSetBounds(smd->vsb,0,smd->state_cnt, smd->height/smd->stateh);
    GScrollBarSetBounds(smd->hsb,0,smd->class_cnt, smd->width/smd->statew);
    if ( smd->offtop + (smd->height/smd->stateh) >= smd->state_cnt )
	smd->offtop = smd->state_cnt - (smd->height/smd->stateh);
    if ( smd->offtop < 0 ) smd->offtop = 0;
    if ( smd->offleft + (smd->width/smd->statew) >= smd->class_cnt )
	smd->offleft = smd->class_cnt - (smd->width/smd->statew);
    if ( smd->offleft < 0 ) smd->offleft = 0;
    GScrollBarSetPos(smd->vsb,smd->offtop);
    GScrollBarSetPos(smd->hsb,smd->offleft);

return( oldtop!=smd->offtop || oldleft!=smd->offleft );
}

static void SMD_HShow(SMD *smd, int pos) {
    if ( pos<0 || pos>=smd->class_cnt )
return;
#if 0
    if ( pos>=smd->offleft && pos<smd->offleft+(smd->width/smd->statew) )
return;		/* Already visible */
#endif
    --pos;	/* One line of context */
    if ( pos + (smd->width/smd->statew) >= smd->class_cnt )
	pos = smd->class_cnt - (smd->width/smd->statew);
    if ( pos < 0 ) pos = 0;
    smd->offleft = pos;
    GScrollBarSetPos(smd->hsb,pos);
    GDrawRequestExpose(smd->gw,NULL,false);
}

static void SMD_HScroll(SMD *smd,struct sbevent *sb) {
    int newpos = smd->offleft;
    GRect rect;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
	if ( smd->width/smd->statew == 1 )
	    --newpos;
	else
	    newpos -= smd->width/smd->statew - 1;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
	if ( smd->width/smd->statew == 1 )
	    ++newpos;
	else
	    newpos += smd->width/smd->statew - 1;
      break;
      case et_sb_bottom:
        newpos = smd->class_cnt - (smd->width/smd->statew);
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos + (smd->width/smd->statew) >= smd->class_cnt )
	newpos = smd->class_cnt - (smd->width/smd->statew);
    if ( newpos < 0 ) newpos = 0;
    if ( newpos!=smd->offleft ) {
	int diff = newpos-smd->offleft;
	smd->offleft = newpos;
	GScrollBarSetPos(smd->hsb,newpos);
	rect.x = smd->xstart2+1; rect.y = smd->ystart;
	rect.width = smd->width-1;
	rect.height = smd->height+(smd->ystart2-smd->ystart);
	GDrawScroll(smd->gw,&rect,-diff*smd->statew,0);
    }
}

static void SMD_VScroll(SMD *smd,struct sbevent *sb) {
    int newpos = smd->offtop;
    GRect rect;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
	if ( smd->height/smd->stateh == 1 )
	    --newpos;
	else
	    newpos -= smd->height/smd->stateh - 1;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
	if ( smd->height/smd->stateh == 1 )
	    ++newpos;
	else
	    newpos += smd->height/smd->stateh - 1;
      break;
      case et_sb_bottom:
        newpos = smd->state_cnt - (smd->height/smd->stateh);
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos + (smd->height/smd->stateh) >= smd->state_cnt )
	newpos = smd->state_cnt - (smd->height/smd->stateh);
    if ( newpos < 0 ) newpos = 0;
    if ( newpos!=smd->offtop ) {
	int diff = newpos-smd->offtop;
	smd->offtop = newpos;
	GScrollBarSetPos(smd->vsb,newpos);
	rect.x = smd->xstart; rect.y = smd->ystart2+1;
	rect.width = smd->width+(smd->xstart2-smd->xstart);
	rect.height = smd->height-1;
	GDrawScroll(smd->gw,&rect,0,diff*smd->stateh);
    }
}

static int subsmd_e_h(GWindow gw, GEvent *event) {
    SMD *smd = GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_char:
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("statemachine.html#EditClass");
return( true );
	} else if ( event->u.chr.keysym=='q' && (event->u.chr.state&ksm_control)) {
	    if ( event->u.chr.state&ksm_shift )
		_SMD_Cancel(smd);
	    else
		MenuExit(NULL,NULL,NULL);
return( true );
	}
return( false );
    }
return( true );
}

static int smd_e_h(GWindow gw, GEvent *event) {
    SMD *smd = GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_close:
	_SMD_Cancel(smd);
      break;
      case et_char:
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("statemachine.html");
return( true );
	} else if ( event->u.chr.keysym=='q' && (event->u.chr.state&ksm_control)) {
	    if ( event->u.chr.state&ksm_shift )
		_SMD_Cancel(smd);
	    else
		MenuExit(NULL,NULL,NULL);
return( true );
	}
return( false );
      case et_expose:
	SMD_Expose(smd,gw,event);
      break;
      case et_resize: {
	int blen = GDrawPointsToPixels(NULL,GIntGetResource(_NUM_Buttonsize));
	GRect wsize, csize;

	GDrawGetSize(smd->gw,&wsize);
	GDrawResize(smd->cw,wsize.width,wsize.height);
	GGadgetResize(GWidgetGetControl(smd->gw,CID_Group),wsize.width-4,wsize.height-4);
	GGadgetResize(GWidgetGetControl(smd->gw,CID_Group2),wsize.width-4,wsize.height-4);
	GGadgetResize(GWidgetGetControl(smd->gw,CID_Line1),wsize.width-20,1);
	GGadgetResize(GWidgetGetControl(smd->gw,CID_Line2),wsize.width-20,1);

	GGadgetGetSize(GWidgetGetControl(smd->gw,CID_Ok),&csize);
	GGadgetMove(GWidgetGetControl(smd->gw,CID_Ok),csize.x,wsize.height-smd->canceldrop-3);
	GGadgetMove(GWidgetGetControl(smd->cw,CID_Prev),csize.x+3,wsize.height-smd->canceldrop);
	GGadgetGetSize(GWidgetGetControl(smd->gw,CID_Cancel),&csize);
	GGadgetMove(GWidgetGetControl(smd->gw,CID_Cancel),wsize.width-blen-30,wsize.height-smd->canceldrop);
	GGadgetMove(GWidgetGetControl(smd->gw,CID_Next),wsize.width-blen-33,wsize.height-smd->canceldrop-3);

	GGadgetGetSize(GWidgetGetControl(smd->gw,CID_Classes),&csize);
	GGadgetResize(GWidgetGetControl(smd->gw,CID_Classes),wsize.width-GDrawPointsToPixels(NULL,10),csize.height);
	GGadgetResize(GWidgetGetControl(smd->gw,CID_GlyphList),wsize.width-GDrawPointsToPixels(NULL,10),csize.height);

	GGadgetGetSize(smd->hsb,&csize);
	smd->width = wsize.width-csize.height-smd->xstart2-5;
	GGadgetResize(smd->hsb,smd->width,csize.height);
	GGadgetMove(smd->hsb,smd->xstart2,wsize.height-smd->sbdrop-csize.height);
	GGadgetGetSize(smd->vsb,&csize);
	smd->height = wsize.height-smd->sbdrop-smd->ystart2-csize.width;
	GGadgetResize(smd->vsb,csize.width,wsize.height-smd->sbdrop-smd->ystart2-csize.width);
	GGadgetMove(smd->vsb,wsize.width-csize.width-5,smd->ystart2);
	SMD_SBReset(smd);

	GDrawRequestExpose(smd->gw,NULL,false);
      } break;
      case et_mouseup: case et_mousemove: case et_mousedown:
	SMD_Mouse(smd,event);
      break;
      case et_controlevent:
	switch( event->u.control.subtype ) {
	  case et_textchanged:
	    if ( event->u.control.u.tf_changed.from_pulldown!=-1 ) {
		uint32 tag = (uint32) (intpt) smd->mactags[event->u.control.u.tf_changed.from_pulldown].userdata;
		unichar_t ubuf[20];
		char buf[20];
		/* If they select something from the pulldown, don't show the human */
		/*  readable form, instead show the numeric feature/setting */
		sprintf( buf,"<%d,%d>", (int) (tag>>16), (int) (tag&0xffff) );
		uc_strcpy(ubuf,buf);
		GGadgetSetTitle(event->u.control.g,ubuf);
	    }
	  break;
	  case et_scrollbarchange:
	    if ( event->u.control.g == smd->hsb )
		SMD_HScroll(smd,&event->u.control.u.sb);
	    else
		SMD_VScroll(smd,&event->u.control.u.sb);
	  break;
	}
      break;
#if 0
      case et_drop:
	smd_Drop(GDrawGetUserData(gw),event);
      break;
#endif
    }
return( true );
}

SMD *StateMachineEdit(SplineFont *sf,ASM *sm,struct gfi_data *d) {
    static char *titles[2][3] = {
	{ N_("Edit Indic Rearrangement"), N_("Edit Contextual Substitution"), N_("Edit Contextual Glyph Insertion") },
	{ N_("New Indic Rearrangement"), N_("New Contextual Substitution"), N_("New Contextual Glyph Insertion") }};
    SMD *smd = gcalloc(1,sizeof(SMD));
    GRect pos, subpos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[20];
    GTextInfo label[20];
    int k, vk;
    int blen = GIntGetResource(_NUM_Buttonsize);
    const int space = 7;
    static unichar_t courier[] = { 'c', 'o', 'u', 'r', 'i', 'e', 'r', ',', 'm','o','n','o','s','p','a','c','e',',','c','l','e','a','r','l','y','u',',', 'u','n','i','f','o','n','t', '\0' };
    int as, ds, ld, sbsize;
    FontRequest rq;
    static unichar_t statew[] = { '1', '2', '3', '4', '5', 0 };
    char buf[20];
    unichar_t ubuf[20];

    smd->sf = sf;
    smd->sm = sm;
    smd->d = d;
    smd->isnew = (sm->class_cnt==0);
    if ( smd->isnew ) {
	smd->class_cnt = 4;			/* 4 built in classes */
	smd->state_cnt = 2;			/* 2 built in states */
	smd->states = gcalloc(smd->class_cnt*smd->state_cnt,sizeof(struct asm_state));
	smd->states[1*4+2].next_state = 1;	/* deleted glyph is a noop */
    } else {
	smd->class_cnt = sm->class_cnt;
	smd->state_cnt = sm->state_cnt;
	smd->states = StateCopy(sm->state,sm->class_cnt,sm->state_cnt,
		smd->class_cnt,smd->state_cnt,sm->type,false);
    }
    smd->index = sm->type==asm_indic ? 0 : sm->type==asm_context ? 1 : 2;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = false;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _(titles[smd->isnew][smd->index]);
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,GGadgetScale(SMD_WIDTH));
    pos.height = GDrawPointsToPixels(NULL,SMD_HEIGHT);
    smd->gw = gw = GDrawCreateTopWindow(NULL,&pos,smd_e_h,smd,&wattrs);

    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));
    k = 0;

    label[k].text = (unichar_t *) _("Feature, Setting:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = 5;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k++].creator = GLabelCreate;

    if ( !smd->isnew ) {
	sprintf(buf,"<%d,%d>", sm->feature, sm->setting );
	uc_strcpy(ubuf,buf);
	label[k].text = ubuf;
	gcd[k].gd.label = &label[k];
    }
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.u.list = smd->mactags = AddMacFeatures(NULL,pst_max,sf);
    gcd[k].gd.cid = CID_FeatSet;
    gcd[k++].creator = GListFieldCreate;

    label[k].text = (unichar_t *) _("Right To Left");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 150; gcd[k].gd.pos.y = 5;
    gcd[k].gd.flags = gg_enabled|gg_visible | (sm->flags&0x4000?gg_cb_on:0);
    gcd[k].gd.cid = CID_RightToLeft;
    gcd[k++].creator = GCheckBoxCreate;
    if ( smd->sm->type == asm_kern ) {
	gcd[k-1].gd.flags = gg_enabled;		/* I'm not sure why kerning doesn't have an r2l bit */
	gcd[k-2].gd.flags = gg_enabled;
	gcd[k-3].gd.flags = gg_enabled;
    }

    label[k].text = (unichar_t *) _("Vertical Only");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 150; gcd[k].gd.pos.y = 5+16;
    gcd[k].gd.flags = gg_enabled|gg_visible | (sm->flags&0x8000?gg_cb_on:0);
    gcd[k].gd.cid = CID_VertOnly;
    gcd[k++].creator = GCheckBoxCreate;

    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = GDrawPointsToPixels(gw,gcd[k-3].gd.pos.y+27);
    gcd[k].gd.pos.width = pos.width-20;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels;
    gcd[k].gd.cid = CID_Line1;
    gcd[k++].creator = GLineCreate;

    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = GDrawPixelsToPoints(gw,gcd[k-1].gd.pos.y)+5;
    gcd[k].gd.pos.width = SMD_WIDTH-10;
    gcd[k].gd.pos.height = 8*12+10;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_list_multiplesel;
    gcd[k].gd.handle_controlevent = SMD_ClassSelected;
    gcd[k].gd.cid = CID_Classes;
    gcd[k++].creator = GListCreate;

    label[k].text = (unichar_t *) _("_New");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+gcd[k-1].gd.pos.height+10;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.handle_controlevent = SMD_New;
    gcd[k].gd.cid = CID_New;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("_Edit");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5+blen+space; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible;
    gcd[k].gd.handle_controlevent = SMD_Edit;
    gcd[k].gd.cid = CID_Edit;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("_Delete");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x+blen+space; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible;
    gcd[k].gd.handle_controlevent = SMD_Delete;
    gcd[k].gd.cid = CID_Delete;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = GDrawPointsToPixels(gw,gcd[k-1].gd.pos.y+28);
    gcd[k].gd.pos.width = pos.width-20;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels;
    gcd[k].gd.cid = CID_Line2;
    gcd[k++].creator = GLineCreate;

    smd->canceldrop = GDrawPointsToPixels(gw,SMD_CANCELDROP);
    smd->sbdrop = smd->canceldrop+GDrawPointsToPixels(gw,7);

    vk = k;
    gcd[k].gd.pos.width = sbsize = GDrawPointsToPixels(gw,_GScrollBar_Width);
    gcd[k].gd.pos.x = pos.width-sbsize;
    gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+8;
    gcd[k].gd.pos.height = pos.height-gcd[k].gd.pos.y-sbsize-smd->sbdrop;
    gcd[k].gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_sb_vert;
    gcd[k++].creator = GScrollBarCreate;
    smd->height = gcd[k-1].gd.pos.height;
    smd->ystart = gcd[k-1].gd.pos.y;

    gcd[k].gd.pos.height = sbsize;
    gcd[k].gd.pos.y = pos.height-sbsize-8;
    gcd[k].gd.pos.x = 4;
    gcd[k].gd.pos.width = pos.width-sbsize;
    gcd[k].gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
    gcd[k++].creator = GScrollBarCreate;
    smd->width = gcd[k-1].gd.pos.width;
    smd->xstart = 5;

    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 30-3; gcd[k].gd.pos.y = SMD_HEIGHT-SMD_CANCELDROP-3;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled | gg_but_default;
    gcd[k].gd.handle_controlevent = SMD_Ok;
    gcd[k].gd.cid = CID_Ok;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = -30; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+3;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled | gg_but_cancel;
    gcd[k].gd.handle_controlevent = SMD_Cancel;
    gcd[k].gd.cid = CID_Cancel;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = 2; gcd[k].gd.pos.y = 2;
    gcd[k].gd.pos.width = pos.width-4;
    gcd[k].gd.pos.height = pos.height-4;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels;
    gcd[k].gd.cid = CID_Group;
    gcd[k++].creator = GGroupCreate;

    GGadgetsCreate(gw,gcd);
    smd->vsb = gcd[vk].ret;
    smd->hsb = gcd[vk+1].ret;

    {
	GGadget *list = GWidgetGetControl(smd->gw,CID_Classes);
	GListAppendLine8(list,_("Class 0: {End of Text}"),false);
	GListAppendLine8(list,_("Class 1: {Everything Else}"),false);
	GListAppendLine8(list,_("Class 2: {Deleted Glyph}"),false);
	GListAppendLine8(list,_("Class 3: {End of Line}"),false);
	for ( k=4; k<sm->class_cnt; ++k ) {
	    unichar_t *temp = AddClass(k,sm->classes[k],true);
	    GListAppendLine(list,temp,false);
	    free(temp);
	}
    }

    wattrs.mask = wam_events;
    subpos = pos; subpos.x = subpos.y = 0;
    smd->cw = GWidgetCreateSubWindow(smd->gw,&subpos,subsmd_e_h,smd,&wattrs);

    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));
    k = 0;

    label[k].text = (unichar_t *) _("Set");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = 5;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.popup_msg = (unichar_t *) _("Set this glyph list to be the characters selected in the fontview");
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[k].gd.handle_controlevent = SMD_FromSelection;
    gcd[k].gd.cid = CID_Set;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("Select");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 70; gcd[k].gd.pos.y = 5;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.popup_msg = (unichar_t *) _("Set the fontview's selection to be the characters named here");
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[k].gd.handle_controlevent = SMD_ToSelection;
    gcd[k].gd.cid = CID_Select;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = 30;
    gcd[k].gd.pos.width = SMD_WIDTH-25; gcd[k].gd.pos.height = 8*13+4;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_textarea_wrap;
    gcd[k].gd.cid = CID_GlyphList;
    gcd[k++].creator = GTextAreaCreate;

    label[k].text = (unichar_t *) _("< _Prev");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 30; gcd[k].gd.pos.y = SMD_HEIGHT-SMD_CANCELDROP;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled /*| gg_but_cancel*/;
    gcd[k].gd.handle_controlevent = SMD_Prev;
    gcd[k].gd.cid = CID_Prev;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("_Next >");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = -30+3; gcd[k].gd.pos.y = SMD_HEIGHT-SMD_CANCELDROP;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled /*| gg_but_default*/;
    gcd[k].gd.handle_controlevent = SMD_Next;
    gcd[k].gd.cid = CID_Next;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = 2; gcd[k].gd.pos.y = 2;
    gcd[k].gd.pos.width = pos.width-4;
    gcd[k].gd.pos.height = pos.height-4;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels;
    gcd[k].gd.cid = CID_Group2;
    gcd[k++].creator = GGroupCreate;

    GGadgetsCreate(smd->cw,gcd);


    memset(&rq,'\0',sizeof(rq));
    rq.point_size = 12;
    rq.weight = 400;
    rq.family_name = courier;
    smd->font = GDrawInstanciateFont(GDrawGetDisplayOfWindow(gw),&rq);
    GDrawFontMetrics(smd->font,&as,&ds,&ld);
    smd->fh = as+ds; smd->as = as;
    GDrawSetFont(gw,smd->font);

    smd->stateh = 4*smd->fh+3;
    smd->statew = GDrawGetTextWidth(gw,statew,-1,NULL)+3;
    smd->xstart2 = smd->xstart+smd->statew/2;
    smd->ystart2 = smd->ystart+2*smd->fh+1;

    GDrawSetVisible(gw,true);

return( smd );
}

void SMD_Close(SMD *smd) {
    _SMD_Cancel(smd);
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
