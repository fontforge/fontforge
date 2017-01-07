/* Copyright (C) 2003-2007 by George Williams */
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

#include "asmfpst.h"

#include "fontforgevw.h"
#include "fvfonts.h"
#include "ttf.h"
#include "splineutil.h"
#include "tottfaat.h"
#include "tottfgpos.h"
#include <chardata.h>
#include <utype.h>
#include <ustring.h>

/* ************************************************************************** */
/* *************** Routines to test conversion from OpenType **************** */
/* ************************************************************************** */

int ClassesMatch(int cnt1,char **classes1,int cnt2,char **classes2) {
    int i;

    if ( cnt1!=cnt2 )
return( false );
    for ( i=1; i<cnt2; ++i )
	if ( strcmp(classes1[i],classes2[i])!=0 )
return( false );

return( true );
}

static char **classcopy(char **names,int nextclass) {
    char **ret;
    int i;

    if ( nextclass <= 1 )
return( NULL );

    ret = malloc(nextclass*sizeof(char *));
    ret[0] = NULL;
    for ( i=1; i<nextclass; ++i )
	ret[i] = copy(names[i]);
return( ret );
}

FPST *FPSTGlyphToClass(FPST *fpst) {
    FPST *new;
    int nextclass=0, i,j,k, max, cnt, ch;
    char *pt, *end;
    char **names;

    if ( fpst->format!=pst_glyphs )
return( NULL );

    new = chunkalloc(sizeof(FPST));
    new->type = fpst->type;
    new->format = pst_class;
    new->subtable = fpst->subtable;
    new->rule_cnt = fpst->rule_cnt;
    new->rules = calloc(fpst->rule_cnt,sizeof(struct fpst_rule));

    max = 100; nextclass=1;
    names = malloc(max*sizeof(char *));
    names[0] = NULL;
    for ( i=0; i<fpst->rule_cnt; ++i ) {
	for ( j=0; j<3; ++j ) {
	    cnt = 0;
	    if ( (&fpst->rules[i].u.glyph.names)[j]!=NULL && *(&fpst->rules[i].u.glyph.names)[j]!='\0' ) {
		for ( pt=(&fpst->rules[i].u.glyph.names)[j]; *pt; ) {
		    while ( *pt==' ' ) ++pt;
		    if ( *pt=='\0' )
		break;
		    while ( *pt!=' ' && *pt!='\0' ) ++pt;
		    ++cnt;
		}
	    }
	    (&new->rules[i].u.class.ncnt)[j] = cnt;
	    if ( cnt!=0 ) {
		(&new->rules[i].u.class.nclasses)[j] = malloc(cnt*sizeof(uint16));
		cnt = 0;
		for ( pt=(&fpst->rules[i].u.glyph.names)[j]; *pt; pt=end ) {
		    while ( *pt==' ' ) ++pt;
		    if ( *pt=='\0' )
		break;
		    for ( end=pt ; *end!=' ' && *end!='\0'; ++end );
		    ch = *end; *end='\0';
		    for ( k=1; k<nextclass; ++k )
			if ( strcmp(pt,names[k])==0 )
		    break;
		    if ( k==nextclass ) {
			if ( nextclass>=max )
			    names = realloc(names,(max+=100)*sizeof(char *));
			names[nextclass++] = copy(pt);
		    }
		    *end = ch;
		    (&new->rules[i].u.class.nclasses)[j][cnt++] = k;
		}
	    }
	}
	new->rules[i].lookup_cnt = fpst->rules[i].lookup_cnt;
	new->rules[i].lookups = malloc(fpst->rules[i].lookup_cnt*sizeof(struct seqlookup));
	memcpy(new->rules[i].lookups,fpst->rules[i].lookups,
		fpst->rules[i].lookup_cnt*sizeof(struct seqlookup));
    }
    new->nccnt = nextclass;
    new->nclass = names;
    new->nclassnames = calloc(nextclass,sizeof(char *));	/* Leave as NULL */
    if ( fpst->type==pst_chainpos || fpst->type==pst_chainsub ) {
	/* our class set has one "class" for each glyph used anywhere */
	/*  all three class sets are the same */
	new->bccnt = new->fccnt = nextclass;
	new->bclass = classcopy(names,nextclass);
	new->fclass = classcopy(names,nextclass);
	new->bclassnames = calloc(nextclass,sizeof(char *));	/* Leave as NULL */
	new->fclassnames = calloc(nextclass,sizeof(char *));	/* Leave as NULL */
    }
return( new );
}

static int ValidSubs(OTLookup *otl ) {
return( otl->lookup_type == gsub_single );
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

static OTLookup *RuleHasSubsHere(struct fpst_rule *rule,int depth) {
    int i,j;

    if ( depth<rule->u.class.bcnt )
return( NULL );
    depth -= rule->u.class.bcnt;
    if ( depth>=rule->u.class.ncnt )
return( NULL );
    for ( i=0; i<rule->lookup_cnt; ++i ) {
	if ( rule->lookups[i].seq==depth ) {
	    /* It is possible to have two substitutions applied at the same */
	    /*  location. I can't deal with that here */
	    for ( j=i+1; j<rule->lookup_cnt; ++j ) {
		if ( rule->lookups[j].seq==depth )
return( (OTLookup *) 0xffffffff );
	    }
return( rule->lookups[i].lookup );
	}
    }

return( 0 );
}

static OTLookup *RulesAllSameSubsAt(struct contexttree *me,int pos) {
    int i;
    OTLookup *tag=(OTLookup *) 0x01, *newtag;	/* Can't use 0 as an "unused" flag because it is perfectly valid for there to be no substititution. But then all rules must have no subs */

    for ( i=0; i<me->rule_cnt; ++i ) {
	newtag = RuleHasSubsHere(me->rules[i].rule,pos);
	if ( tag==(OTLookup *) 0x01 )
	    tag=newtag;
	else if ( newtag!=tag )
return( (OTLookup *) 0xffffffff );
    }
    if ( tag==(OTLookup *) 0x01 )
return( NULL );		/* Should never happen */

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
	    if ( me->applymarkedsubs==(OTLookup *) 0xffffffff )
return( false );
	    if ( !ValidSubs(me->applymarkedsubs))
return( false );
	}
	me->applycursubs = RulesAllSameSubsAt(me,me->depth);
	if ( me->applycursubs==(OTLookup *) 0xffffffff )
return( false );
	if ( me->applycursubs!=NULL && !ValidSubs(me->applycursubs))
return( false );
	for ( i=0; i<me->branch_cnt; ++i ) {
	    if ( !TreeFollowBranches(sf,me->branches[i].branch,-1))
return( false );
	}
    } else {
	for ( i=0; i<me->branch_cnt; ++i ) {
	    for ( j=0; j<me->rule_cnt; ++j )
		if ( me->rules[j].branch==me->branches[i].branch &&
			RuleHasSubsHere(me->rules[j].rule,me->depth)!=NULL )
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
	me->rules = calloc(me->rule_cnt,sizeof(struct ct_subs));
	for ( i=0; i<me->rule_cnt; ++i )
	    me->rules[i].rule = &fpst->rules[i];
	me->parent = NULL;
    } else {
	me->depth = parent->depth+1;
	for ( i=rcnt=0; i<parent->rule_cnt; ++i )
	    if ( parent->rules[i].rule->u.class.allclasses[me->depth] == class )
		++rcnt;
	me->rule_cnt = rcnt;
	me->rules = calloc(me->rule_cnt,sizeof(struct ct_subs));
	for ( i=rcnt=0; i<parent->rule_cnt; ++i )
	    if ( parent->rules[i].rule->u.class.allclasses[me->depth] == class )
		me->rules[rcnt++].rule = parent->rules[i].rule;
	me->parent = parent;
    }
    classes = malloc(me->rule_cnt*sizeof(uint16));
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
    me->branches = calloc(ccnt,sizeof(struct ct_branch));
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
	fpst->rules[i].u.class.allclasses = malloc((fpst->rules[i].u.class.bcnt+
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

    if ( ret!=NULL )
	TreeLabelState(ret,1);	/* actually, it's states 0&1, but this will do */

return( ret );
}

static struct contexttree *TreeNext(struct contexttree *cur) {
    struct contexttree *p;
    int i;

    if ( cur->branch_cnt!=0 )
return( cur->branches[0].branch );
    else {
	for (;;) {
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

int FPSTisMacable(SplineFont *sf, FPST *fpst) {
    int i;
    int featureType, featureSetting;
    struct contexttree *ret;
    FeatureScriptLangList *fl;

    if ( fpst->type!=pst_contextsub && fpst->type!=pst_chainsub )
return( false );
    for ( fl = fpst->subtable->lookup->features; fl!=NULL; fl=fl->next ) {
	if ( OTTagToMacFeature(fl->featuretag,&featureType,&featureSetting) &&
		scriptsHaveDefault(fl->scripts) )
    break;
    }
    if ( fl==NULL )
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
	    if ( !ValidSubs(fpst->rules[i].lookups[1].lookup) )
return( false );
		
	} else if ( fpst->rules[i].lookup_cnt!=1 )
return( false );
	if ( !ValidSubs(fpst->rules[i].lookups[0].lookup) )
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
    ret = pt = malloc(len+1);
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
    markglyphs = malloc(sf->glyphcnt*sizeof(SplineChar *));
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
    ret = pt = malloc(len+1);
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

static int FindFormLookupsForScript(SplineFont *sf,uint32 script,OTLookup *lookups[4]) {
    OTLookup *otl;
    FeatureScriptLangList *fl;
    struct scriptlanglist *sl;
    int which;

    memset(lookups,0,4*sizeof(OTLookup *));
    for ( otl=sf->gsub_lookups; otl!=NULL; otl=otl->next ) if ( !otl->unused && otl->lookup_type == gsub_single ) {
	for ( fl=otl->features; fl!=NULL; fl=fl->next ) {
	    if ( fl->featuretag== CHR('i','n','i','t') ) which = 0;
	    else if ( fl->featuretag== CHR('m','e','d','i') ) which = 1;
	    else if ( fl->featuretag== CHR('f','i','n','a') ) which = 2;
	    else if ( fl->featuretag== CHR('i','s','o','l') ) which = 3;
	    else
	continue;
	    if ( lookups[which]!=NULL )
	continue;
	    for ( sl=fl->scripts; sl!=NULL && sl->script!=script; sl=sl->next );
	    if ( sl==NULL )
	continue;
	    lookups[which] = otl;
	break;
	}
    }
    if ( lookups[0]!=NULL || lookups[1]!=NULL || lookups[2]!=NULL || lookups[3]!=NULL )
return( true );

return( false );
}

ASM *ASMFromOpenTypeForms(SplineFont *sf,uint32 script) {
    int i, which, cg, mg;
    SplineChar *sc, *rsc, **classglyphs, **markglyphs;
    PST *pst;
    OTLookup *lookups[4];
    ASM *sm;
    int flags;

    if ( !FindFormLookupsForScript(sf,script,lookups))
return( NULL );
    flags = (lookups[0]!=NULL ? lookups[0]->lookup_flags
	    :lookups[1]!=NULL ? lookups[1]->lookup_flags
	    :lookups[2]!=NULL ? lookups[2]->lookup_flags
	    :                   lookups[3]->lookup_flags);
    classglyphs = calloc((sf->glyphcnt+1),sizeof(SplineChar *));
    markglyphs = malloc((sf->glyphcnt+1)*sizeof(SplineChar *));

    mg = 0;
    for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL ) {
	if ( (flags&pst_ignorecombiningmarks) && IsMarkChar(sc)) {
	    markglyphs[mg++] = sc;
	} else if ( SCScriptFromUnicode(sc)==script ) {
	    classglyphs[sc->orig_pos] = sc;
	    for ( pst = sc->possub; pst!=NULL; pst=pst->next ) if ( pst->subtable!=NULL ) {
		OTLookup *otl = pst->subtable->lookup;
		for ( which=3; which>=0; --which ) {
		    if ( otl==lookups[which])
		break;
		}
		if ( which==-1 )
	    continue;
		rsc = SFGetChar(sf,-1,pst->u.subs.variant);
		if ( rsc!=NULL )
		    classglyphs[rsc->orig_pos] = rsc;
	    }
	}
    }
    markglyphs[mg] = NULL;

    cg = 0;
    for ( i=0; i<sf->glyphcnt; ++i )
	if ( classglyphs[i]!=NULL )
	    classglyphs[cg++] = classglyphs[i];
    classglyphs[cg] = NULL;

    sm = chunkalloc(sizeof(ASM));
    sm->type = asm_context;
    sm->flags = (flags&pst_r2l) ? asm_descending : 0;
	/* This is a temporary value. It should be replaced if we will retain */
	/*  this state machine */
    sm->subtable = (lookups[3]!=NULL ? lookups[3] : lookups[0]!=NULL ? lookups[0] : lookups[1]!=NULL ? lookups[1] : lookups[2])->subtables;
    /* Only one (or two) classes of any importance: Letter in this script */
    /* might already be formed. Might be a lig. Might be normal */
    /* Oh, if ignoremarks is true, then combining marks merit a class of their own */
    sm->class_cnt = (flags&pst_ignorecombiningmarks) ? 6 : 5;
    sm->classes = calloc(sm->class_cnt,sizeof(char *));

    sm->classes[4] = GlyphListToNames(classglyphs);
    if ( flags&pst_ignorecombiningmarks )
	sm->classes[5] = GlyphListToNames(markglyphs);
    free(classglyphs); free(markglyphs);


    /* State 0,1 are start states */
    /* State 2 means we have found one interesting letter, transformed current to 'init' and marked it (in case we need to make it isolated) */
    /* State 3 means we have found two interesting letters, transformed current to 'medi' and marked (in case we need to make it final) */
    sm->state_cnt = 4;
    sm->state = calloc(sm->state_cnt*sm->class_cnt,sizeof(struct asm_state));

    /* State 0,1 (start), Class 4 (char in script) takes us to state 2 */
    sm->state[4].next_state = 2;
    sm->state[4].flags = 0x8000;

    sm->state[sm->class_cnt+4] = sm->state[4];

    for ( i=0; i<4; ++i ) {
	sm->state[2*sm->class_cnt+i].next_state = 0;
	sm->state[2*sm->class_cnt+i].u.context.mark_lookup = lookups[3];/* Isolated */
    }

    sm->state[2*sm->class_cnt+4].next_state = 3;
    sm->state[2*sm->class_cnt+4].flags = 0x8000;
    sm->state[2*sm->class_cnt+4].u.context.mark_lookup = lookups[0];	/* Initial */

    for ( i=0; i<4; ++i ) {
	sm->state[3*sm->class_cnt+i].next_state = 0;
	sm->state[3*sm->class_cnt+i].u.context.mark_lookup = lookups[2];/* Final */
    }

    sm->state[3*sm->class_cnt+4].next_state = 3;
    sm->state[3*sm->class_cnt+4].flags = 0x8000;
    sm->state[3*sm->class_cnt+4].u.context.mark_lookup = lookups[1];	/* Medial */

    /* Deleted glyph retains same state, just eats the glyph */
    for ( i=0; i<sm->state_cnt; ++i ) {
	int pos = i*sm->class_cnt+2, mpos = i*sm->class_cnt+5;
	sm->state[pos].next_state = i;
	sm->state[pos].flags = 0;
	sm->state[pos].u.context.cur_lookup = NULL;
	sm->state[pos].u.context.mark_lookup = NULL;
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
    next = calloc(1<<match_len,sizeof(int));
    temp = malloc((1<<match_len)*sizeof(SplineChar **));

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
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
		temp[mask] = malloc(max*sizeof(SplineChar *));
	    temp[mask][next[mask]++] = sc;
	    sc->ticked = true;
	}
    }

    gall = calloc(gtot+1,sizeof(SplineChar *));
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
    if ( fpst->subtable->lookup->lookup_flags & pst_ignorecombiningmarks ) {
	for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL && sf->glyphs[i]->ttf_glyph!=-1 ) {
	    if ( sf->glyphs[i]->lsidebearing==0 && IsMarkChar(sf->glyphs[i])) {
		sf->glyphs[i]->lsidebearing = class_cnt;
		++gcnt;
	    }
	}
	++class_cnt;			/* Add a class for the marks so we can ignore them */
    }
    *cc = class_cnt+4;
    glyphs = malloc((gcnt+1)*sizeof(SplineChar *));
    map = malloc((gcnt+1)*sizeof(uint16));
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

    nc = calloc(match_len,sizeof(int));
    *classes = malloc((match_len+1)*sizeof(int *));
    for ( i=0; i<match_len; ++i )
	(*classes)[i] = malloc((class_cnt+1)*sizeof(int));
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
    int subspos = r->u.coverage.bcnt+r->lookups[0].seq;
    OTLookup *substag = r->lookups[0].lookup, *finaltag=NULL;
    uint16 *map;
    ASM *sm;

    /* In one very specific case we can support two substitutions */
    if ( r->lookup_cnt==2 ) {
	if ( r->lookups[0].seq==r->u.coverage.ncnt-1 ) {
	    finaltag = substag;
	    subspos = r->u.coverage.bcnt+r->lookups[1].seq;
	    substag = r->lookups[1].lookup;
	} else
	    finaltag = r->lookups[1].lookup;
    }

    tables = malloc((r->u.coverage.ncnt+r->u.coverage.bcnt+r->u.coverage.fcnt+1)*sizeof(SplineChar **));
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
    sm->flags = (fpst->subtable->lookup->lookup_flags&pst_r2l) ? asm_descending : 0;
    sm->class_cnt = class_cnt;
    sm->classes = malloc(class_cnt*sizeof(char *));
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
    sm->state = calloc(sm->state_cnt*sm->class_cnt,sizeof(struct asm_state));
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
			sm->state[off+i].u.context.cur_lookup = substag;
		    else {
			sm->state[off+i].u.context.mark_lookup = substag;
			sm->state[off+i].u.context.cur_lookup = finaltag;
		    }
		} else if ( subspos==j )
		    sm->state[off+i].flags = 0x8000;
	    } else if ( i==2 || ((fpst->subtable->lookup->lookup_flags&pst_ignorecombiningmarks) && i==class_cnt-1 ) )
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
	    trans->u.context.mark_lookup = cur->branches[i].branch->applymarkedsubs;
	    trans->u.context.cur_lookup = cur->branches[i].branch->applycursubs;
return;
	}
    }

    if ( cur->ends_here!=NULL ) {
	trans->next_state = 0;
	trans->flags = 0x4000;
	trans->u.context.mark_lookup = cur->applymarkedsubs;
	trans->u.context.cur_lookup = cur->applycursubs;
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
	    trans->u.context.mark_lookup==NULL &&
	    trans->u.context.cur_lookup==NULL );
}

static ASM *ASMFromClassFPST(SplineFont *sf,FPST *fpst, struct contexttree *tree) {
    ASM *sm;
    struct contexttree *cur;
    int i;

    sm = chunkalloc(sizeof(ASM));
    sm->type = asm_context;
    sm->flags = (fpst->subtable->lookup->lookup_flags&pst_r2l) ? asm_descending : 0;
    /* mac class sets have four magic classes, opentype sets only have one */
    sm->class_cnt = (fpst->subtable->lookup->lookup_flags&pst_ignorecombiningmarks) ? fpst->nccnt+4 : fpst->nccnt+3;
    sm->classes = malloc(sm->class_cnt*sizeof(char *));
    sm->classes[0] = sm->classes[1] = sm->classes[2] = sm->classes[3] = NULL;
    for ( i=1; i<fpst->nccnt; ++i )
	sm->classes[i+3] = copy(fpst->nclass[i]);
    if ( fpst->subtable->lookup->lookup_flags&pst_ignorecombiningmarks )
	sm->classes[sm->class_cnt-1] = BuildMarkClass(sf);

    /* Now build the state machine */
    sm->state_cnt = tree->next_state;
    sm->state = calloc(sm->state_cnt*sm->class_cnt,sizeof(struct asm_state));
    for ( cur=tree; cur!=NULL; cur = TreeNext(cur)) if ( cur->state!=0 ) {
	int off = cur->state*sm->class_cnt;

	SMSetState(&sm->state[off+1],cur,0);		/* Out of bounds state */
	sm->state[off+2].next_state = cur->state;	/* Deleted glyph gets eaten and ignored */
	if ( fpst->subtable->lookup->lookup_flags&pst_ignorecombiningmarks )
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
	sm = ASMFromCoverageFPST(sf,fpst,ordered);
    else {
	tree = FPST2Tree(sf, tempfpst);
	if ( tree!=NULL ) {
	    sm = ASMFromClassFPST(sf,tempfpst,tree);
	    TreeFree(tree);
	} else
	    sm = NULL;
    }

    if ( tempfpst!=fpst )
	FPSTFree(tempfpst);
	/* This is a temporary value. It should be replaced if we plan to */
	/*  retain this state machine */
    if ( sm!=NULL )
	sm->subtable = fpst->subtable;
return( sm );
}
