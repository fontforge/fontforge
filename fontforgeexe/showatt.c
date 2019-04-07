/* -*- coding: utf-8 -*- */
/* Copyright (C) 2003-2012 by George Williams */
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
#include "fontforgeui.h"
#include "fvfonts.h"
#include <gfile.h>
#include "glyphcomp.h"
#include "lookups.h"
#include "splinefill.h"
#include "splinesaveafm.h"
#include "splineutil.h"
#include "tottfaat.h"
#include "tottfgpos.h"
#include <utype.h>
#include <ustring.h>
#include <gkeysym.h>

#include "ttf.h"

extern int _GScrollBar_Width;

/* This file contains routines to build a dialog showing GPOS/GSUB/morx */
/*  tables and their contents */

struct att_dlg;
struct node {
    unsigned int open: 1;
    unsigned int children_checked: 1;
    unsigned int used: 1;
    unsigned int macfeat: 1;
    unsigned int monospace: 1;
    unsigned int horizontal: 1;
    uint16 cnt;
    struct node *children, *parent;
    void (*build)(struct node *,struct att_dlg *);
    char *label;		/* utf8 */
    uint32 tag;
    union sak {
	SplineChar *sc;
	int index;
	OTLookup *otl;
	struct lookup_subtable *sub;
	struct baselangextent *langs;
	Justify *jscript;
	struct jstf_lang *jlang;
	OTLookup **otll;
    } u;
    int lpos;
};

enum dlg_type { dt_show_att, dt_font_comp };

struct att_dlg {
    unsigned int done: 1;
    struct node *tables;
    int open_cnt, lines_page, off_top, off_left, page_width, bmargin;
    int maxl;
    SplineFont *sf;
    int def_layer;
    GWindow gw,v;
    GGadget *vsb, *hsb, *cancel;
    int fh, as;
    GFont *font, *monofont;
    struct node *current;
    enum dlg_type dlg_type;
    FontView *fv1, *fv2;
    struct node *popup_node;
};

static void BuildGSUBlookups(struct node *node,struct att_dlg *att);

static void nodesfree(struct node *node) {
    int i;

    if ( node==NULL )
return;
    for ( i=0; node[i].label!=NULL; ++i ) {
	nodesfree(node[i].children);
	free(node[i].label);
    }
    free(node);
}

static int node_alphabetize(const void *_n1, const void *_n2) {
    const struct node *n1 = _n1, *n2 = _n2;
    int ret;

    ret = strcasecmp(n1->label,n2->label);
    if ( ret!=0 )
return( ret );

return( strcmp(n1->label,n2->label));
}

static void BuildMarkedLigatures(struct node *node,struct att_dlg *att) {
    SplineChar *sc = node->u.sc;
    struct lookup_subtable *sub = node->parent->parent->u.sub;
    AnchorClass *ac;
    int classcnt, j, max, k;
    AnchorPoint *ap;
    char buf[90];
    SplineFont *sf = att->sf;

    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;

    classcnt = 0;
    for ( ap=sc->anchor; ap!=NULL; ap=ap->next )
	if ( ap->anchor->subtable == sub )
	    ++classcnt;
    max=0;
    for ( ap=sc->anchor; ap!=NULL ; ap=ap->next )
	if ( ap->lig_index>max )
	    max = ap->lig_index;
    node->children = calloc(classcnt+1,sizeof(struct node));
    for ( k=j=0; k<=max; ++k ) {
	for ( ac=sf->anchor; ac!=NULL; ac=ac->next ) if ( ac->subtable==sub ) {
	    for ( ap=sc->anchor; ap!=NULL && (ap->type!=at_baselig || ap->anchor!=ac || ap->lig_index!=k); ap=ap->next );
	    if ( ap!=NULL ) {
		sprintf(buf,_("Component %d %.30s (%d,%d)"),
			k, ac->name, (int) ap->me.x, (int) ap->me.y );
		node->children[j].label = copy(buf);
		node->children[j++].parent = node;
	    }
	}
    }
    node->cnt = j;
}

static void BuildMarkedChars(struct node *node,struct att_dlg *att) {
    SplineChar *sc = node->u.sc;
    struct lookup_subtable *sub = node->parent->parent->u.sub;
    AnchorClass *ac;
    int classcnt, j;
    AnchorPoint *ap;
    char buf[80];
    SplineFont *sf = att->sf;

    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;

    classcnt = 0;
    for ( ap=sc->anchor; ap!=NULL; ap=ap->next )
	if ( ap->anchor->subtable == sub )
	    ++classcnt;
    node->children = calloc(classcnt+1,sizeof(struct node));
    for ( j=0, ac=sf->anchor; ac!=NULL; ac=ac->next ) if ( ac->subtable==sub ) {
	for ( ap=sc->anchor; ap!=NULL && (!(ap->type==at_basechar || ap->type==at_basemark) || ap->anchor!=ac); ap=ap->next );
	if ( ap!=NULL ) {
	    sprintf(buf,"%.30s (%d,%d)", ac->name,
		    (int) ap->me.x, (int) ap->me.y );
	    node->children[j].label = copy(buf);
	    node->children[j++].parent = node;
	}
    }
    qsort(node->children,j,sizeof(struct node), node_alphabetize);
    node->cnt = j;
}

static void BuildBase(struct node *node,SplineChar **bases,enum anchor_type at, struct node *parent) {
    int i;

    node->parent = parent;
    node->label = copy(at==at_basechar?_("Base Glyphs"):
			at==at_baselig?_("Base Ligatures"):
			_("Base Marks"));
    for ( i=0; bases[i]!=NULL; ++i );
    if ( i==0 ) {
	node->cnt = 1;
	node->children = calloc(2,sizeof(struct node));
	node->children[0].label = copy(_("Empty"));
	node->children[0].parent = node;
    } else {
	node->cnt = i;
	node->children = calloc(i+1,sizeof(struct node));
	for ( i=0; bases[i]!=NULL; ++i ) {
	    node->children[i].label = copy(bases[i]->name);
	    node->children[i].parent = node;
	    node->children[i].u.sc = bases[i];
	    node->children[i].build = at==at_baselig?BuildMarkedLigatures:BuildMarkedChars;
	}
	qsort(node->children,node->cnt,sizeof(struct node), node_alphabetize);
    }
}

static void BuildMark(struct node *node,SplineChar **marks,AnchorClass *ac, struct node *parent) {
    int i;
    char buf[80];
    AnchorPoint *ap;

    node->parent = parent;
    sprintf(buf,_("Mark Class %.20s"),ac->name);
    node->label = copy(buf);
    for ( i=0; marks[i]!=NULL; ++i );
    if ( i==0 ) {
	node->cnt = 1;
	node->children = calloc(2,sizeof(struct node));
	node->children[0].label = copy(_("Empty"));
	node->children[0].parent = node;
    } else {
	node->cnt = i;
	node->children = calloc(i+1,sizeof(struct node));
	for ( i=0; marks[i]!=NULL; ++i ) {
	    for ( ap=marks[i]->anchor; ap!=NULL && (ap->type!=at_mark || ap->anchor!=ac); ap=ap->next );
	    sprintf(buf,_("%.30s (%d,%d)"), marks[i]->name,
		    (int) ap->me.x, (int) ap->me.y );
	    node->children[i].label = copy(buf);
	    node->children[i].parent = node;
	}
	qsort(node->children,node->cnt,sizeof(struct node), node_alphabetize);
    }
}

static void BuildAnchorLists(struct node *node,struct att_dlg *att) {
    struct lookup_subtable *sub = node->u.sub;
    AnchorClass *ac, *ac2;
    int cnt, i, j, classcnt;
    AnchorPoint *ap, *ent, *ext;
    SplineChar **base, **lig, **mkmk, **entryexit;
    SplineChar ***marks;
    int *subcnts;
    SplineFont *sf = att->sf;
    char buf[80];

    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;

    if ( sub->lookup->lookup_type==gpos_cursive ) {
	for ( ac = sf->anchor; ac!=NULL && ac->subtable!=sub; ac = ac->next );
	entryexit = ac==NULL ? NULL : EntryExitDecompose(sf,ac,NULL);
	if ( entryexit==NULL ) {
	    node->children = calloc(2,sizeof(struct node));
	    node->cnt = 1;
	    node->children[0].label = copy(_("Empty"));
	    node->children[0].parent = node;
	} else {
	    for ( cnt=0; entryexit[cnt]!=NULL; ++cnt );
	    node->children = calloc(cnt+1,sizeof(struct node));
	    node->cnt = cnt;
	    for ( cnt=0; entryexit[cnt]!=NULL; ++cnt ) {
		node->children[cnt].u.sc = entryexit[cnt];
		node->children[cnt].label = copy(entryexit[cnt]->name);
		node->children[cnt].parent = node;
	    }
	    qsort(node->children,node->cnt,sizeof(struct node), node_alphabetize);
	    for ( cnt=0; entryexit[cnt]!=NULL; ++cnt ) {
		for ( ent=ext=NULL, ap = node->children[cnt].u.sc->anchor; ap!=NULL; ap=ap->next ) {
		    if ( ap->anchor==ac ) {
			if ( ap->type == at_centry )
			    ent = ap;
			else if ( ap->type == at_cexit )
			    ent = ap;
		    }
		}
		node->children[cnt].cnt = (ent!=NULL)+(ext!=NULL);
		node->children[cnt].children = calloc((1+(ent!=NULL)+(ext!=NULL)),sizeof(struct node));
		i = 0;
		if ( ent!=NULL ) {
		    snprintf(buf,sizeof(buf), _("Entry (%d,%d)"),
			    (int) ent->me.x, (int) ent->me.y);
		    node->children[cnt].children[i].label = copy(buf);
		    node->children[cnt].children[i].parent = &node->children[cnt];
		    ++i;
		}
		if ( ext!=NULL ) {
		    snprintf(buf,sizeof(buf), _("Exit (%d,%d)"),
			    (int) ext->me.x, (int) ext->me.y);
		    node->children[cnt].children[i].label = copy(buf);
		    node->children[cnt].children[i].parent = &node->children[cnt];
		    ++i;
		}
	    }
	}
	free(entryexit);
    } else {
	classcnt = 0;
	ac = NULL;
	for ( ac2=sf->anchor; ac2!=NULL; ac2=ac2->next ) {
	    if ( ac2->subtable == sub ) {
		if ( ac==NULL ) ac=ac2;
		ac2->matches = true;
		++classcnt;
	    } else
		ac2->matches = false;
	}
	if ( classcnt==0 )
return;

	marks = malloc(classcnt*sizeof(SplineChar **));
	subcnts = malloc(classcnt*sizeof(int));
	AnchorClassDecompose(sf,ac,classcnt,subcnts,marks,&base,&lig,&mkmk,NULL);
	node->cnt = classcnt+(base!=NULL)+(lig!=NULL)+(mkmk!=NULL);
	node->children = calloc(node->cnt+1,sizeof(struct node));
	i=0;
	if ( base!=NULL )
	    BuildBase(&node->children[i++],base,at_basechar,node);
	if ( lig!=NULL )
	    BuildBase(&node->children[i++],lig,at_baselig,node);
	if ( mkmk!=NULL )
	    BuildBase(&node->children[i++],mkmk,at_basemark,node);
	for ( j=0, ac2=ac; j<classcnt; ac2=ac2->next ) if ( ac2->matches ) {
	    if ( marks[j]!=NULL )
		BuildMark(&node->children[i++],marks[j],ac2,node);
	    ++j;
	}
	node->cnt = i;
	for ( i=0; i<classcnt; ++i )
	    free(marks[i]);
	free(marks);
	free(subcnts);
	free(base);
	free(lig);
	free(mkmk);
    }
}

static void BuildKC2(struct node *node,struct att_dlg *att) {
    KernClass *kc = node->parent->u.sub->kc;
    struct node *seconds;
    int index=node->u.index,i,cnt, len;
    char buf[32];
    char *name;

    for ( i=1,cnt=0; i<kc->second_cnt; ++i )
	if ( kc->offsets[index*kc->second_cnt+i]!=0 && strlen(kc->seconds[i])!=0 )
	    ++cnt;

    node->children = seconds = calloc(cnt+1,sizeof(struct node));
    node->cnt = cnt;
    cnt = 0;
    for ( i=1; i<kc->second_cnt; ++i ) if ( kc->offsets[index*kc->second_cnt+i]!=0 && strlen(kc->seconds[i])!=0 ) {
	sprintf( buf, "%d ", kc->offsets[index*kc->second_cnt+i]);
	len = strlen(buf)+strlen(kc->seconds[i])+1;
	name = malloc(len);
	strcpy(name,buf);
	strcat(name,kc->seconds[i]);
	seconds[cnt].label = name;
	seconds[cnt].parent = node;
	seconds[cnt].build = NULL;
	seconds[cnt++].u.index = i;
    }
}

static void BuildKC(struct node *node,struct att_dlg *att) {
    KernClass *kc = node->u.sub->kc;
    struct node *firsts;
    int i,j,cnt,cnt2;

    for ( i=1,cnt=0; i<kc->first_cnt; ++i ) {
	for ( j=1,cnt2=0 ; j<kc->second_cnt; ++j ) {
	    if ( kc->offsets[i*kc->second_cnt+j]!=0 )
		++cnt2;
	}
	if ( cnt2 && strlen(kc->firsts[i])>0 )
	    ++cnt;
    }

    node->children = firsts = calloc(cnt+1,sizeof(struct node));
    node->cnt = cnt;
    for ( i=1,cnt=0; i<kc->first_cnt; ++i ) {
	for ( j=1,cnt2=0 ; j<kc->second_cnt; ++j ) {
	    if ( kc->offsets[i*kc->second_cnt+j]!=0 )
		++cnt2;
	}
	if ( cnt2==0 || strlen(kc->firsts[i])==0 )
    continue;
	firsts[cnt].label = copy(kc->firsts[i]);
	firsts[cnt].parent = node;
	firsts[cnt].build = BuildKC2;
	firsts[cnt++].u.index = i;
    }
}

static int PSTAllComponentsExist(SplineFont *sf,char *glyphnames ) {
    char *start, *end, ch;
    int ret;

    if ( glyphnames==NULL )
return( false );
    for ( start=glyphnames; *start; start = end ) {
	while ( *start==' ' ) ++start;
	for ( end = start; *end!=' ' && *end!='\0'; ++end );
	if ( end==start )
    break;
	ch = *end; *end = '\0';
	ret = SCWorthOutputting(SFGetChar(sf,-1,start));
	*end = ch;
	if ( !ret )
return( false );
    }
return( true );
}

static void BuildFPSTRule(struct node *node,struct att_dlg *att) {
    FPST *fpst = node->parent->u.sub->fpst;
    int index = node->u.index;
    struct fpst_rule *r = &fpst->rules[index];
    int len, i, j;
    struct node *lines;
    char buf[200], *pt, *start, *spt;
    char *upt;
    GrowBuf gb;

    memset(&gb,0,sizeof(gb));

    for ( i=0; i<2; ++i ) {
	len = 0;

	switch ( fpst->format ) {
	  case pst_glyphs:
	    if ( r->u.glyph.back!=NULL && *r->u.glyph.back!='\0' ) {
		if ( i ) {
		    strcpy(buf, _("Backtrack Match: ") );
		    lines[len].label = malloc((strlen(buf)+strlen(r->u.glyph.back)+1));
		    strcpy(lines[len].label,buf);
		    upt = lines[len].label+strlen(lines[len].label);
		    for ( pt=r->u.glyph.back+strlen(r->u.glyph.back); pt>r->u.glyph.back; pt=start ) {
			for ( start = pt-1; start>=r->u.glyph.back&& *start!=' '; --start );
			for ( spt=start+1; spt<pt; )
			    *upt++ = *spt++;
			*upt++ = ' ';
		    }
		    upt[-1] = '\0';
		    lines[len].parent = node;
		}
		++len;
	    }
	    if ( i ) {
		strcpy(buf, _("Match: ") );
		lines[len].label = malloc((strlen(buf)+strlen(r->u.glyph.names)+1));
		strcpy(lines[len].label,buf);
		strcat(lines[len].label,r->u.glyph.names);
		lines[len].parent = node;
	    }
	    ++len;
	    if ( r->u.glyph.fore!=NULL && *r->u.glyph.fore!='\0' ) {
		if ( i ) {
		    strcpy(buf, _("Lookahead Match: ") );
		    lines[len].label = malloc((strlen(buf)+strlen(r->u.glyph.fore)+1));
		    strcpy(lines[len].label,buf);
		    strcat(lines[len].label,r->u.glyph.fore);
		    lines[len].parent = node;
		}
		++len;
	    }
	  break;
	  case pst_class:
	    if ( r->u.class.bcnt!=0 ) {
		if ( i ) {
		    gb.pt = gb.base;
		    GrowBufferAddStr(&gb,P_("Backtrack class: ","Backtrack classes: ",r->u.class.bcnt));
		    for ( j=r->u.class.bcnt-1; j>=0; --j ) {
			if ( fpst->bclassnames==NULL || fpst->bclassnames[r->u.class.bclasses[j]]==NULL ) {
			    sprintf( buf, "%d ", r->u.class.bclasses[j] );
			    GrowBufferAddStr(&gb,buf);
			} else
			    GrowBufferAddStr(&gb,fpst->bclassnames[r->u.class.bclasses[j]]);
		    }
		    lines[len].label = copy(gb.base);
		    lines[len].parent = node;
		}
		++len;
	    }
	    if ( i ) {
		gb.pt = gb.base;
		GrowBufferAddStr(&gb, P_("Class","Classes",r->u.class.ncnt));
		for ( j=0; j<r->u.class.ncnt; ++j ) {
		    if ( fpst->nclassnames==NULL || fpst->nclassnames[r->u.class.nclasses[j]]==NULL ) {
			sprintf( buf, "%d ", r->u.class.nclasses[j] );
			GrowBufferAddStr(&gb,buf);
		    } else
			GrowBufferAddStr(&gb,fpst->nclassnames[r->u.class.nclasses[j]]);
		}
		lines[len].label = copy(gb.base);
		lines[len].parent = node;
	    }
	    ++len;
	    if ( r->u.class.fcnt!=0 ) {
		if ( i ) {
		    gb.pt = gb.base;
		    GrowBufferAddStr(&gb, P_("Lookahead Class","Lookahead Classes",r->u.class.fcnt));
		    for ( j=0; j<r->u.class.fcnt; ++j ) {
			if ( fpst->fclassnames==NULL || fpst->fclassnames[r->u.class.fclasses[j]]==NULL ) {
			    sprintf( buf, "%d ", r->u.class.fclasses[j] );
			    GrowBufferAddStr(&gb,buf);
			} else
			    GrowBufferAddStr(&gb,fpst->fclassnames[r->u.class.fclasses[j]]);
		    }
		    lines[len].label = copy(gb.base);
		    lines[len].parent = node;
		}
		++len;
	    }
	  break;
	  case pst_coverage:
	  case pst_reversecoverage:
	    for ( j=r->u.coverage.bcnt-1; j>=0; --j ) {
		if ( i ) {
		    sprintf(buf, _("Back coverage %d: "), -j-1);
		    lines[len].label = malloc((strlen(buf)+strlen(r->u.coverage.bcovers[j])+1));
		    strcpy(lines[len].label,buf);
		    strcat(lines[len].label,r->u.coverage.bcovers[j]);
		    lines[len].parent = node;
		}
		++len;
	    }
	    for ( j=0; j<r->u.coverage.ncnt; ++j ) {
		if ( i ) {
		    sprintf(buf, _("Coverage %d: "), j);
		    lines[len].label = malloc((strlen(buf)+strlen(r->u.coverage.ncovers[j])+1));
		    strcpy(lines[len].label,buf);
		    strcat(lines[len].label,r->u.coverage.ncovers[j]);
		    lines[len].parent = node;
		}
		++len;
	    }
	    for ( j=0; j<r->u.coverage.fcnt; ++j ) {
		if ( i ) {
		    sprintf(buf, _("Lookahead coverage %d: "), j+r->u.coverage.ncnt);
		    lines[len].label = malloc((strlen(buf)+strlen(r->u.coverage.fcovers[j])+1));
		    strcpy(lines[len].label,buf);
		    strcat(lines[len].label,r->u.coverage.fcovers[j]);
		    lines[len].parent = node;
		}
		++len;
	    }
	  break;
	}
	switch ( fpst->format ) {
	  case pst_glyphs:
	  case pst_class:
	  case pst_coverage:
	    for ( j=0; j<r->lookup_cnt; ++j ) {
		if ( i ) {
		    sprintf(buf, _("Apply at %d %.80s"), r->lookups[j].seq,
			    r->lookups[j].lookup->lookup_name );
		    lines[len].label = copy(buf);
		    lines[len].parent = node;
		    lines[len].u.otl = r->lookups[j].lookup;
		    lines[len].build = BuildGSUBlookups;
		}
		++len;
	    }
	  break;
	  case pst_reversecoverage:
	    if ( i ) {
		strcpy(buf, _("Replacement: ") );
		lines[len].label = malloc((strlen(buf)+strlen(r->u.rcoverage.replacements)+1));
		strcpy(lines[len].label,buf);
		strcat(lines[len].label,r->u.rcoverage.replacements);
		lines[len].parent = node;
	    }
	    ++len;
	  break;
	}
	if ( i==0 ) {
	    node->children = lines = calloc(len+1,sizeof(struct node));
	    node->cnt = len;
	}
    }
    free(gb.base);
}

static void BuildFPST(struct node *node,struct att_dlg *att) {
    FPST *fpst = node->u.sub->fpst;
    int len, i, j;
    struct node *lines;
    char buf[200];
    static char *type[] = { N_("Contextual Positioning"), N_("Contextual Substitution"),
	    N_("Chaining Positioning"), N_("Chaining Substitution"),
	    N_("Reverse Chaining Subs") };
    static char *format[] = { N_("glyphs"), N_("classes"), N_("coverage"), N_("coverage") };

    lines = NULL;
    for ( i=0; i<2; ++i ) {
	len = 0;

	if ( i ) {
/* GT: There are various broad classes of lookups here and the first string */
/* GT: describes those: "Contextual Positioning", Contextual Substitution", etc. */
/* GT: Each of those may be formated in 3 different ways: by (or perhaps using */
/* GT: would be a better word) glyphs, classes or coverage tables. */
/* GT: So this might look like: */
/* GT:  Contextual Positioning by classes */
	    sprintf(buf, _("%s by %s"), _(type[fpst->type-pst_contextpos]),
		    _(format[fpst->format]));
	    lines[len].label = copy(buf);
	    lines[len].parent = node;
	}
	++len;
	if ( fpst->format==pst_class ) {
	    for ( j=1; j<fpst->bccnt ; ++j ) {
		if ( i ) {
		    sprintf(buf, _("Backtrack class %d: "), j);
		    lines[len].label = malloc((strlen(buf)+strlen(fpst->bclass[j])+1));
		    strcpy(lines[len].label,buf);
		    strcat(lines[len].label,fpst->bclass[j]);
		    lines[len].parent = node;
		}
		++len;
	    }
	    for ( j=1; j<fpst->nccnt ; ++j ) {
		if ( i ) {
		    sprintf(buf, _("Class %d: "), j);
		    lines[len].label = malloc((strlen(buf)+strlen(fpst->nclass[j])+1));
		    strcpy(lines[len].label,buf);
		    strcat(lines[len].label,fpst->nclass[j]);
		    lines[len].parent = node;
		}
		++len;
	    }
	    for ( j=1; j<fpst->fccnt ; ++j ) {
		if ( i ) {
		    sprintf(buf, _("Lookahead class %d: "), j);
		    lines[len].label = malloc((strlen(buf)+strlen(fpst->fclass[j])+1));
		    strcpy(lines[len].label,buf);
		    strcat(lines[len].label,fpst->fclass[j]);
		    lines[len].parent = node;
		}
		++len;
	    }
	}
	for ( j=0; j<fpst->rule_cnt; ++j ) {
	    if ( i ) {
		sprintf(buf, _("Rule %d"), j);
		lines[len].label = copy(buf);
		lines[len].parent = node;
		lines[len].u.index = j;
		lines[len].build = BuildFPSTRule;
	    }
	    ++len;
	}
	if ( i==0 ) {
	    node->children = lines = calloc(len+1,sizeof(struct node));
	    node->cnt = len;
	}
    }
}

static void BuildASM(struct node *node,struct att_dlg *att) {
    ASM *sm = node->u.sub->sm;
    int len, i, j, k, scnt = 0;
    struct node *lines;
    char buf[200], *space;
    static char *type[] = { N_("Indic Reordering"), N_("Contextual Substitution"),
	    N_("Ligatures"), N_("<undefined>"), N_("Simple Substitution"),
	    N_("Glyph Insertion"), N_("<undefined>"),  N_("<undefined>"), N_("<undefined>"),
	    N_("<undefined>"), N_("<undefined>"), N_("<undefined>"),N_("<undefined>"),
	    N_("<undefined>"), N_("<undefined>"), N_("<undefined>"), N_("<undefined>"),
	    N_("Kern by State") };
    OTLookup **used;

    if ( sm->type == asm_context ) {
	used = malloc(sm->class_cnt*sm->state_cnt*2*sizeof(OTLookup *));
	for ( i=scnt=0; i<sm->class_cnt*sm->state_cnt; ++i ) {
	    OTLookup *otl;
	    otl = sm->state[i].u.context.mark_lookup;
	    if ( otl!=NULL ) {
		for ( k=0; k<scnt && used[k]!=otl; ++k );
		if ( k==scnt ) used[scnt++] = otl;
	    }
	    otl = sm->state[i].u.context.cur_lookup;
	    if ( otl!=NULL ) {
		for ( k=0; k<scnt && used[k]!=otl; ++k );
		if ( k==scnt ) used[scnt++] = otl;
	    }
	}
    }

    lines = NULL;
    space = malloc( 81*sm->class_cnt+40 );
    for ( i=0; i<2; ++i ) {
	len = 0;

	if ( i ) {
	    lines[len].label = copy(_(type[sm->type]));
	    lines[len].parent = node;
	}
	++len;
	for ( j=4; j<sm->class_cnt ; ++j ) {
	    if ( i ) {
		sprintf(buf, _("Class %d: "), j);
		lines[len].label = malloc((strlen(buf)+strlen(sm->classes[j])+1));
		strcpy(lines[len].label,buf);
		strcat(lines[len].label,sm->classes[j]);
		lines[len].parent = node;
	    }
	    ++len;
	}
	for ( j=0; j<sm->state_cnt; ++j ) {
	    if ( i ) {
/* GT: You're in a state machine, and this is describing the %4d'th state of */
/* GT: that machine. From the state the next state will be a list of */
/* GT: state-numbers which are appended to this string. */
		sprintf(space, _("State %4d Next: "), j );
		for ( k=0; k<sm->class_cnt; ++k )
		    sprintf( space+strlen(space), "%5d", sm->state[j*sm->class_cnt+k].next_state );
		lines[len].label = copy(space);
		lines[len].parent = node;
		lines[len].monospace = true;
	    }
	    ++len;
	    if ( i ) {
		sprintf(space, _("State %4d Flags:"), j );
		for ( k=0; k<sm->class_cnt; ++k )
		    sprintf( space+strlen(space), " %04x", sm->state[j*sm->class_cnt+k].flags );
		lines[len].label = copy(space);
		lines[len].parent = node;
		lines[len].monospace = true;
	    }
	    ++len;
	    if ( sm->type==asm_context ) {
		if ( i ) {
		    sprintf(space, _("State %4d Mark: "), j );
		    for ( k=0; k<sm->class_cnt; ++k )
			if ( sm->state[j*sm->class_cnt+k].u.context.mark_lookup==NULL )
			    strcat(space,"     ");
			else
			    sprintf( space+strlen(space), " %.80s", sm->state[j*sm->class_cnt+k].u.context.mark_lookup->lookup_name );
		    lines[len].label = copy(space);
		    lines[len].parent = node;
		    lines[len].monospace = true;
		}
		++len;
		if ( i ) {
		    sprintf(space, _("State %4d Cur:  "), j );
		    for ( k=0; k<sm->class_cnt; ++k )
			if ( sm->state[j*sm->class_cnt+k].u.context.cur_lookup==NULL )
			    strcat(space,"     ");
			else
			    sprintf( space+strlen(space), " %.80s", sm->state[j*sm->class_cnt+k].u.context.cur_lookup->lookup_name );
		    lines[len].label = copy(space);
		    lines[len].parent = node;
		    lines[len].monospace = true;
		}
		++len;
	    }
	}
	for ( j=0; j<scnt; ++j ) {
	    if ( i ) {
		sprintf(buf, _("Nested Substitution %.80s"), used[j]->lookup_name );
		lines[len].label = copy(buf);
		lines[len].parent = node;
		lines[len].u.otl = used[j];
		lines[len].build = BuildGSUBlookups;
	    }
	    ++len;
	}
	if ( i==0 ) {
	    node->children = lines = calloc(len+1,sizeof(struct node));
	    node->cnt = len;
	}
    }
    free(space);
    free(used);
}

static void BuildKern2(struct node *node,struct att_dlg *att) {
    struct lookup_subtable *sub = node->parent->u.sub;
    SplineChar *base = node->u.sc;
    SplineFont *_sf = att->sf;
    int doit, cnt;
    int isv;
    PST *pst;
    KernPair *kp;
    char buffer[200];
    struct node *lines;

    for ( doit = 0; doit<2; ++doit ) {
	cnt = 0;
	for ( pst=base->possub; pst!=NULL; pst = pst->next ) {
	    if ( pst->subtable==sub && pst->type==pst_pair &&
		    PSTAllComponentsExist(_sf,pst->u.pair.paired)) {
		if ( doit ) {
		    sprintf(buffer, "%.80s ", pst->u.pair.paired );
		    if ( pst->u.pair.vr[0].xoff!=0 ||
			    /* If everything is 0, we'll want to display something */
			    /* might as well be this */
			    ( pst->u.pair.vr[0].yoff == 0 &&
			      pst->u.pair.vr[0].h_adv_off ==0 &&
			      pst->u.pair.vr[0].v_adv_off ==0 &&
			      pst->u.pair.vr[1].xoff ==0 &&
			      pst->u.pair.vr[1].yoff ==0 &&
			      pst->u.pair.vr[1].h_adv_off ==0 &&
			      pst->u.pair.vr[1].v_adv_off == 0 ))
			sprintf( buffer+strlen(buffer), " ∆x¹=%d", pst->u.pair.vr[0].xoff );
		    if ( pst->u.pair.vr[0].yoff!=0 )
			sprintf( buffer+strlen(buffer), " ∆y¹=%d", pst->u.pair.vr[0].yoff );
		    if ( pst->u.pair.vr[0].h_adv_off!=0 )
			sprintf( buffer+strlen(buffer), " ∆x_adv¹=%d", pst->u.pair.vr[0].h_adv_off );
		    if ( pst->u.pair.vr[0].v_adv_off!=0 )
			sprintf( buffer+strlen(buffer), " ∆y_adv¹=%d", pst->u.pair.vr[0].v_adv_off );
		    if ( pst->u.pair.vr[1].xoff!=0 )
			sprintf( buffer+strlen(buffer), " ∆x²=%d", pst->u.pair.vr[1].xoff );
		    if ( pst->u.pair.vr[1].yoff!=0 )
			sprintf( buffer+strlen(buffer), " ∆y²=%d", pst->u.pair.vr[1].yoff );
		    if ( pst->u.pair.vr[1].h_adv_off!=0 )
			sprintf( buffer+strlen(buffer), " ∆x_adv²=%d", pst->u.pair.vr[1].h_adv_off );
		    if ( pst->u.pair.vr[1].v_adv_off!=0 )
			sprintf( buffer+strlen(buffer), " ∆y_adv²=%d", pst->u.pair.vr[1].v_adv_off );
		    lines[cnt].label = copy(buffer);
		    lines[cnt].parent = node;
		}
		++cnt;
	    }
	}
	for ( isv=0; isv<2 ; ++isv ) {
	    for ( kp= isv ? base->vkerns : base->kerns ; kp!=NULL; kp=kp->next ) {
		if ( kp->subtable==sub ) {
		    if ( doit ) {
			if ( isv )
			    sprintf( buffer, "%.80s  ∆y_adv¹=%d", kp->sc->name, kp->off );
			else if ( sub->lookup->lookup_flags&pst_r2l )
			    sprintf( buffer, "%.80s  ∆x_adv²=%d", kp->sc->name, kp->off );
			else
			    sprintf( buffer, "%.80s  ∆x_adv¹=%d", kp->sc->name, kp->off );
			lines[cnt].label = copy(buffer);
			lines[cnt].parent = node;
		    }
		    ++cnt;
		}
	    }
	}
	if ( !doit ) {
	    node->children = lines = calloc(cnt+1,sizeof(struct node));
	    node->cnt = cnt;
	} else
	    qsort(lines,cnt,sizeof(struct node), node_alphabetize);
    }
}

static void BuildKern(struct node *node,struct att_dlg *att) {
    struct lookup_subtable *sub = node->u.sub;
    SplineFont *_sf = att->sf, *sf;
    int k, gid, doit, cnt, maxc, isv;
    SplineChar *sc;
    PST *pst;
    KernPair *kp;
    struct node *lines;

    if ( _sf->cidmaster!=NULL ) _sf = _sf->cidmaster;

    k=maxc=0;
    do {
	sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[k];
	if ( sf->glyphcnt>maxc ) maxc = sf->glyphcnt;
	++k;
    } while ( k<_sf->subfontcnt );

    for ( doit = 0; doit<2; ++doit ) {
	cnt = 0;
	for ( gid=0; gid<maxc; ++gid ) {
	    k=0;
	    sc = NULL;
	    do {
		sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[k];
		if ( gid<sf->glyphcnt && sf->glyphs[gid]!=NULL ) {
		    sc = sf->glyphs[gid];
	    break;
		}
		++k;
	    } while ( k<_sf->subfontcnt );
	    if ( sc!=NULL ) {
		int found = false;
		for ( pst=sc->possub; pst!=NULL; pst = pst->next ) {
		    if ( pst->subtable==sub ) {
			found = true;
		break;
		    }
		}
		for ( isv=0; isv<2 && !found; ++isv ) {
		    for ( kp= isv ? sc->vkerns : sc->kerns ; kp!=NULL; kp=kp->next ) {
			if ( kp->subtable==sub ) {
			    found = true;
		    break;
			}
		    }
		}
		if ( found ) {
		    if ( doit ) {
			lines[cnt].label = copy(sc->name);
			lines[cnt].parent = node;
			lines[cnt].build = BuildKern2;
			lines[cnt].u.sc = sc;
		    }
		    ++cnt;
		}
	    }
	}
	if ( !doit ) {
	    node->children = lines = calloc(cnt+1,sizeof(struct node));
	    node->cnt = cnt;
	} else
	    qsort(lines,cnt,sizeof(struct node), node_alphabetize);
    }
}

static void BuildPST(struct node *node,struct att_dlg *att) {
    struct lookup_subtable *sub = node->u.sub;
    SplineFont *_sf = att->sf, *sf;
    int k, gid, doit, cnt, maxl, len, maxc;
    SplineChar *sc;
    PST *pst;
    struct node *lines;
    char *lbuf=NULL;

    if ( _sf->cidmaster!=NULL ) _sf = _sf->cidmaster;

    k=maxc=0;
    do {
	sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[k];
	if ( sf->glyphcnt>maxc ) maxc = sf->glyphcnt;
	++k;
    } while ( k<_sf->subfontcnt );

    for ( doit = 0; doit<2; ++doit ) {
	cnt = maxl = 0;
	for ( gid=0; gid<maxc; ++gid ) {
	    k=0;
	    sc = NULL;
	    do {
		sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[k];
		if ( gid<sf->glyphcnt && sf->glyphs[gid]!=NULL ) {
		    sc = sf->glyphs[gid];
	    break;
		}
		++k;
	    } while ( k<_sf->subfontcnt );
	    if ( sc!=NULL ) {
		for ( pst=sc->possub; pst!=NULL; pst = pst->next ) {
		    if ( pst->subtable==sub ) {
			if ( doit ) {
			    if ( pst->type==pst_position )
				sprintf(lbuf,"%s ∆x=%d ∆y=%d ∆x_adv=%d ∆y_adv=%d",
					sc->name,
					pst->u.pos.xoff, pst->u.pos.yoff,
					pst->u.pos.h_adv_off, pst->u.pos.v_adv_off );
			    else
				sprintf(lbuf, "%s %s %s", sc->name,
					pst->type==pst_ligature ? "<=" : "=>",
					pst->u.subs.variant );
			    lines[cnt].label = copy(lbuf);
			    lines[cnt].parent = node;
			} else {
			    if ( pst->type==pst_position )
				len = strlen(sc->name)+40;
			    else
				len = strlen(sc->name)+strlen(pst->u.subs.variant)+8;
			    if ( len>maxl ) maxl = len;
			}
			++cnt;
		    }
		}
	    }
	}
	if ( !doit ) {
	    lbuf = malloc(maxl*sizeof(unichar_t));
	    node->children = lines = calloc(cnt+1,sizeof(struct node));
	    node->cnt = cnt;
	} else
	    qsort(lines,cnt,sizeof(struct node), node_alphabetize);
    }
    free(lbuf);
}

static void BuildSubtableDispatch(struct node *node,struct att_dlg *att) {
    struct lookup_subtable *sub = node->u.sub;
    int lookup_type = sub->lookup->lookup_type;

    switch ( lookup_type ) {
      case gpos_context: case gpos_contextchain:
      case gsub_context: case gsub_contextchain: case gsub_reversecchain:
	BuildFPST(node,att);
return;
      case gpos_cursive: case gpos_mark2base: case gpos_mark2ligature: case gpos_mark2mark:
	BuildAnchorLists(node,att);
return;
      case gpos_pair:
	if ( sub->kc!=NULL )
	    BuildKC(node,att);
	else
	    BuildKern(node,att);
return;
      case gpos_single:
      case gsub_single: case gsub_multiple: case gsub_alternate: case gsub_ligature:
	BuildPST(node,att);
return;
      case morx_indic: case morx_context: case morx_insert:
      case kern_statemachine:
	BuildASM(node,att);
return;
    }
    IError( "Unknown lookup type in BuildDispatch");
}

static void BuildGSUBlookups(struct node *node,struct att_dlg *att) {
    OTLookup *otl = node->u.otl;
    struct lookup_subtable *sub;
    struct node *subslist;
    int cnt;

    for ( sub = otl->subtables, cnt=0; sub!=NULL; sub=sub->next, ++cnt );
    subslist = calloc(cnt+1,sizeof(struct node));
    for ( sub = otl->subtables, cnt=0; sub!=NULL; sub=sub->next, ++cnt ) {
	subslist[cnt].parent = node;
	subslist[cnt].u.sub = sub;
	subslist[cnt].build = BuildSubtableDispatch;
	subslist[cnt].label = copy(sub->subtable_name);
    }

    node->children = subslist;
    node->cnt = cnt;
}

static void BuildGSUBfeatures(struct node *node,struct att_dlg *att) {
    int isgsub = node->parent->parent->parent->tag==CHR('G','S','U','B');
    uint32 script = node->parent->parent->tag, lang = node->parent->tag, feat=node->tag;
    OTLookup *otl;
    SplineFont *sf = att->sf;
    int match;
    FeatureScriptLangList *fl;
    struct scriptlanglist *sl;
    int cnt, j,l;
    struct node *lookups = NULL;

    for ( j=0; j<2; ++j ) {
	cnt = 0;
	for ( otl = isgsub ? sf->gsub_lookups : sf->gpos_lookups ; otl!=NULL ; otl=otl->next ) {
	    match = false;
	    for ( fl = otl->features; fl!=NULL && !match; fl=fl->next ) {
		if ( fl->featuretag == feat ) {
		    for ( sl = fl->scripts; sl!=NULL && !match; sl=sl->next ) {
			if ( sl->script == script ) {
			    for ( l=0; l<sl->lang_cnt; ++l ) {
				uint32 _lang = l<MAX_LANG ? sl->langs[l] : sl->morelangs[l-MAX_LANG];
				if ( _lang == lang ) {
				    match = true;
			    break;
				}
			    }
			}
		    }
		}
	    }
	    if ( match ) {
		if ( lookups ) {
		    lookups[cnt].parent = node;
		    lookups[cnt].build = BuildGSUBlookups;
		    lookups[cnt].label = copy(otl->lookup_name);
		    lookups[cnt].u.otl = otl;
		}
		++cnt;
	    }
	}
	if ( lookups == NULL )
	    lookups = calloc(cnt+1,sizeof(struct node));
    }

    node->children = lookups;
    node->cnt = cnt;
}

static void BuildGSUBlang(struct node *node,struct att_dlg *att) {
    int isgsub = node->parent->parent->tag==CHR('G','S','U','B');
    uint32 script = node->parent->tag, lang = node->tag;
    int i,j;
    SplineFont *_sf = att->sf;
    struct node *featnodes;
    uint32 *featlist;

    /* Build up the list of features in this lang entry of this script in GSUB/GPOS */

    featlist = SFFeaturesInScriptLang(_sf,!isgsub,script,lang);
    for ( j=0; featlist[j]!=0; ++j );
    featnodes = calloc(j+1,sizeof(struct node));
    for ( i=0; featlist[i]!=0; ++i ) {
	featnodes[i].tag = featlist[i];
	featnodes[i].parent = node;
	featnodes[i].build = BuildGSUBfeatures;
	featnodes[i].label = TagFullName(_sf,featnodes[i].tag,false,false);
    }
    free( featlist );

    node->children = featnodes;
    node->cnt = j;
}

static void BuildGSUBscript(struct node *node,struct att_dlg *att) {
    SplineFont *sf = att->sf;
    int lang_max;
    int i,j;
    struct node *langnodes;
    uint32 *langlist;
    extern GTextInfo languages[];
    char buf[100];
    int isgpos = node->parent->tag == CHR('G','P','O','S');

    /* Build the list of languages that are used in this script */
    /* Don't bother to check whether they actually get used */

    langlist = SFLangsInScript(sf,isgpos,node->tag);
    for ( j=0; langlist[j]!=0; ++j );
    lang_max = j;
    langnodes = calloc(lang_max+1,sizeof(struct node));
    for ( i=0; langlist[i]!=0; ++i )
	langnodes[i].tag = langlist[i];
    free( langlist );

    for ( i=0; i<lang_max; ++i ) {
	for ( j=0; languages[j].text!=NULL && langnodes[i].tag!=(uint32) (intpt) languages[j].userdata; ++j );
	buf[0] = '\'';
	buf[1] = langnodes[i].tag>>24;
	buf[2] = (langnodes[i].tag>>16)&0xff;
	buf[3] = (langnodes[i].tag>>8)&0xff;
	buf[4] = langnodes[i].tag&0xff;
	buf[5] = '\'';
	buf[6] = ' ';
	if ( languages[j].text!=NULL ) {
	    strcpy(buf+7,S_((char *) languages[j].text));
	    strcat(buf," ");
	} else
	    buf[7]='\0';
	strcat(buf,_("Language"));
	langnodes[i].label = copy(buf);
	langnodes[i].build = BuildGSUBlang;
	langnodes[i].parent = node;
    }
    node->children = langnodes;
    node->cnt = i;
}

static void BuildLookupList(struct node *node,struct att_dlg *att) {
    OTLookup **otll = node->u.otll;
    int i;
    struct node *lookupnodes;

    for ( i=0; otll[i]!=NULL; ++i );
    lookupnodes = calloc(i+1,sizeof(struct node));
    for ( i=0; otll[i]!=NULL; ++i ) {
	lookupnodes[i].label = copy(otll[i]->lookup_name);
	lookupnodes[i].parent = node;
	lookupnodes[i].build = BuildGSUBlookups;
	lookupnodes[i].u.otl = otll[i];
    }

    node->children = lookupnodes;
    node->cnt = i;
}

static void BuildJSTFPrio(struct node *node, struct node *parent, OTLookup **otll,
	char *label_if_some, char *label_if_none ) {

    node->parent = parent;
    if ( otll==NULL || otll[0]==NULL ) {
	node->label = copy(label_if_none);
	node->children_checked = true;
	node->cnt = 0;
    } else {
	node->label = copy(label_if_some);
	node->build = BuildLookupList;
	node->u.otll = otll;
    }
}

static void BuildJSTFlang(struct node *node,struct att_dlg *att) {
    struct jstf_lang *jlang = node->u.jlang;
    int i;
    struct node *prionodes, *kids;
    char buf[120];

    prionodes = calloc(jlang->cnt+1,sizeof(struct node));
    for ( i=0; i<jlang->cnt; ++i ) {
	kids = calloc(7,sizeof(struct node));
	BuildJSTFPrio(&kids[0],&prionodes[i],jlang->prios[i].enableExtend,_("Lookups Enabled for Expansion"), _("No Lookups Enabled for Expansion"));
	BuildJSTFPrio(&kids[1],&prionodes[i],jlang->prios[i].disableExtend,_("Lookups Disabled for Expansion"), _("No Lookups Disabled for Expansion"));
	BuildJSTFPrio(&kids[2],&prionodes[i],jlang->prios[i].maxExtend,_("Lookups Limiting Expansion"), _("No Lookups Limiting Expansion"));
	BuildJSTFPrio(&kids[3],&prionodes[i],jlang->prios[i].enableShrink,_("Lookups Enabled for Shrinkage"), _("No Lookups Enabled for Shrinkage"));
	BuildJSTFPrio(&kids[4],&prionodes[i],jlang->prios[i].disableShrink,_("Lookups Disabled for Shrinkage"), _("No Lookups Disabled for Shrinkage"));
	BuildJSTFPrio(&kids[5],&prionodes[i],jlang->prios[i].maxShrink,_("Lookups Limiting Shrinkage"), _("No Lookups Limiting Shrinkage"));
	sprintf( buf, _("Priority: %d"), i );
	prionodes[i].label = copy(buf);
	prionodes[i].parent = node;
	prionodes[i].children_checked = true;
	prionodes[i].children = kids;
	prionodes[i].cnt = 6;
    }

    node->children = prionodes;
    node->cnt = i;
}

static void BuildJSTFscript(struct node *node,struct att_dlg *att) {
    SplineFont *sf = att->sf;
    Justify *jscript = node->u.jscript;
    int lang_max;
    int i,j, ch;
    struct node *langnodes, *extenders=NULL;
    extern GTextInfo languages[];
    char buf[100];
    struct jstf_lang *jlang;
    char *start, *end;
    int gc;
    SplineChar *sc;

    for ( jlang=jscript->langs, lang_max=0; jlang!=NULL; jlang=jlang->next, ++lang_max );
    langnodes = calloc(lang_max+2,sizeof(struct node));
    gc =0;
    if ( jscript->extenders!=NULL ) {
	for ( start= jscript->extenders; ; ) {
	    for ( ; *start==',' || *start==' '; ++start );
	    for ( end=start ; *end!='\0' && *end!=',' && *end!=' '; ++end );
	    if ( start==end )
	break;
	    ++gc;
	    if ( *end=='\0' )
	break;
	    start = end;
	}
	extenders = calloc(gc+1,sizeof(struct node));
	gc=0;
	for ( start= jscript->extenders; ; ) {
	    for ( ; *start==',' || *start==' '; ++start );
	    for ( end=start ; *end!='\0' && *end!=',' && *end!=' '; ++end );
	    if ( start==end )
	break;
	    ch = *end; *end='\0';
	    sc = SFGetChar(sf,-1,start);
	    *end = ch;
	    if ( sc!=NULL ) {
		extenders[gc].label = copy(sc->name);
		extenders[gc].parent = &langnodes[0];
		extenders[gc].children_checked = true;
		extenders[gc].u.sc = sc;
		++gc;
	    }
	    if ( *end=='\0' )
	break;
	    start = end;
	}
    }
    if ( gc==0 ) {
	free(extenders);
	extenders=NULL;
	langnodes[0].label = copy(_("No Extender Glyphs"));
	langnodes[0].parent = node;
	langnodes[0].children_checked = true;
    } else {
	langnodes[0].label = copy(_("Extender Glyphs"));
	langnodes[0].parent = node;
	langnodes[0].children_checked = true;
	langnodes[0].children = extenders;
	langnodes[0].cnt = gc;
    }
	    
    for ( jlang=jscript->langs, i=1; jlang!=NULL; jlang=jlang->next, ++i ) {
	langnodes[i].tag = jlang->lang;
	for ( j=0; languages[j].text!=NULL && langnodes[i].tag!=(uint32) (intpt) languages[j].userdata; ++j );
	buf[0] = '\'';
	buf[1] = langnodes[i].tag>>24;
	buf[2] = (langnodes[i].tag>>16)&0xff;
	buf[3] = (langnodes[i].tag>>8)&0xff;
	buf[4] = langnodes[i].tag&0xff;
	buf[5] = '\'';
	buf[6] = ' ';
	if ( languages[j].text!=NULL ) {
	    strcpy(buf+7,S_((char *) languages[j].text));
	    strcat(buf," ");
	} else
	    buf[7]='\0';
	strcat(buf,_("Language"));
	langnodes[i].label = copy(buf);
	langnodes[i].build = BuildJSTFlang;
	langnodes[i].parent = node;
	langnodes[i].u.jlang = jlang;
    }
    node->children = langnodes;
    node->cnt = i;
}

static void BuildMClass(struct node *node,struct att_dlg *att) {
    SplineFont *_sf = att->sf;
    struct node *glyphs;
    int i;
    char *temp;

    node->children = glyphs = calloc(_sf->mark_class_cnt,sizeof(struct node));
    node->cnt = _sf->mark_class_cnt-1;
    for ( i=1; i<_sf->mark_class_cnt; ++i ) {
	glyphs[i-1].parent = node;
	temp = malloc((strlen(_sf->mark_classes[i]) + strlen(_sf->mark_class_names[i]) + 4));
	strcpy(temp,_sf->mark_class_names[i]);
	strcat(temp,": ");
	strcat(temp,_sf->mark_classes[i]);
	glyphs[i-1].label = temp;
    }
}

static void BuildLCarets(struct node *node,struct att_dlg *att) {
    SplineChar *sc = node->u.sc;
    PST *pst;
    int i,j;
    char buffer[20];
    struct node *lcars;

    j = -1;
    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) if ( pst->type==pst_lcaret ) {
	for ( j=pst->u.lcaret.cnt-1; j>=0; --j )
	    if ( pst->u.lcaret.carets[j]!=0 )
    goto break2;
    }
  break2:
    if ( j==-1 )
return;
    ++j;
    node->children = lcars = calloc(j+1,sizeof(struct node));
    node->cnt = j;
    for ( j=i=0; j<pst->u.lcaret.cnt; ++j ) {
	if ( pst->u.lcaret.carets[j]!=0 ) {
	    sprintf( buffer,"%d", pst->u.lcaret.carets[j] );
	    lcars[i].parent = node;
	    lcars[i++].label = copy(buffer);
	}
    }
}

static void BuildLcar(struct node *node,struct att_dlg *att) {
    SplineFont *sf, *_sf = att->sf;
    struct node *glyphs;
    int i,j,k,l, lcnt;
    PST *pst;

    glyphs = NULL;
    for ( k=0; k<2; ++k ) {
	lcnt = 0;
	l = 0;
	do {
	    sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[l];
	    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL && sf->glyphs[i]->ttf_glyph!=-1 ) {
		for ( pst=sf->glyphs[i]->possub; pst!=NULL; pst=pst->next ) {
		    if ( pst->type == pst_lcaret ) {
			for ( j=pst->u.lcaret.cnt-1; j>=0; --j )
			    if ( pst->u.lcaret.carets[j]!=0 )
			break;
			if ( j!=-1 )
		break;
		    }
		}
		if ( pst!=NULL ) {
		    if ( glyphs!=NULL ) {
			glyphs[lcnt].parent = node;
			glyphs[lcnt].build = BuildLCarets;
			glyphs[lcnt].u.sc = sf->glyphs[i];
			glyphs[lcnt].label = copy(sf->glyphs[i]->name);
		    }
		    ++lcnt;
		}
	    }
	    ++l;
	} while ( l<_sf->subfontcnt );
	if ( lcnt==0 )
    break;
	if ( glyphs!=NULL )
    break;
	node->children = glyphs = calloc(lcnt+1,sizeof(struct node));
	node->cnt = lcnt;
    }
    if ( glyphs!=NULL )
	qsort(glyphs,lcnt,sizeof(struct node), node_alphabetize);
}

static void BuildGdefs(struct node *node,struct att_dlg *att) {
    SplineFont *sf, *_sf = att->sf;
    int i, cmax, l,j, ccnt;
    SplineChar *sc;
    struct node *chars;
    char buffer[100];

    cmax = 0;
    l = 0;
    do {
	sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[l];
	if ( cmax<sf->glyphcnt ) cmax = sf->glyphcnt;
	++l;
    } while ( l<_sf->subfontcnt );

    chars = NULL;
    for ( j=0; j<2; ++j ) {
	ccnt = 0;
	for ( i=0; i<cmax; ++i ) {
	    l = 0;
	    sc = NULL;
	    do {
		sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[l];
		if ( l<sf->glyphcnt && sf->glyphs[i]!=NULL ) {
		    sc = sf->glyphs[i];
	    break;
		}
		++l;
	    } while ( l<_sf->subfontcnt );
	    if ( sc!=NULL && SCWorthOutputting(sc) ) {
		if ( chars!=NULL ) {
		    int gdefc = gdefclass(sc);
		    sprintf(buffer,"%.70s %s", sc->name,
			gdefc==0 ? _("Not classified") :
			gdefc==1 ? _("Base") :
			gdefc==2 ? _("Ligature") :
			gdefc==3 ? _("Mark") :
			    _("Component") );
		    chars[ccnt].parent = node;
		    chars[ccnt].label = copy(buffer);;
		}
		++ccnt;
	    }
	}
	if ( ccnt==0 )
    break;
	if ( chars==NULL ) {
	    node->cnt = ccnt;
	    node->children = chars = calloc(ccnt+1,sizeof(struct node));
	}
    }
}

static void BuildGDEF(struct node *node,struct att_dlg *att) {
    SplineFont *sf, *_sf = att->sf;
    AnchorClass *ac;
    PST *pst;
    int l,j,i;
    int gdef, lcar, mclass;

    for ( ac = _sf->anchor; ac!=NULL; ac=ac->next ) {
	if ( ac->type==act_curs )
    break;
    }
    gdef = lcar = 0;
    if ( ac!=NULL )
	gdef = 1;
    l = 0;
    pst = NULL;
    do {
	sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[l];
	for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL && sf->glyphs[i]->ttf_glyph!=-1 ) {
	    for ( pst=sf->glyphs[i]->possub; pst!=NULL; pst=pst->next ) {
		if ( pst->type == pst_lcaret ) {
		    for ( j=pst->u.lcaret.cnt-1; j>=0; --j )
			if ( pst->u.lcaret.carets[j]!=0 ) {
			    lcar = 1;
		    break;
			}
		    if ( j!=-1 )
	    break;
		}
	    }
	    if ( sf->glyphs[i]->glyph_class!=0 )
		gdef = 1;
	}
	++l;
    } while ( l<_sf->subfontcnt );

    mclass = _sf->mark_class_cnt!=0;

    if ( gdef+lcar+mclass!=0 ) {
	node->children = calloc(gdef+lcar+mclass+1,sizeof(struct node));
	node->cnt = gdef+lcar+mclass;
	if ( gdef ) {
	    node->children[0].label = copy(_("Glyph Definition Sub-Table"));
	    node->children[0].build = BuildGdefs;
	    node->children[0].parent = node;
	}
	if ( lcar ) {
/* GT: Here caret means where to place the cursor inside a ligature. So OpenType */
/* GT: allows there to be a typing cursor inside a ligature (for instance you */
/* GT: can have a cursor between f and i in the "fi" ligature) */
	    node->children[gdef].label = copy(_("Ligature Caret Sub-Table"));
	    node->children[gdef].build = BuildLcar;
	    node->children[gdef].parent = node;
	}
	if ( mclass ) {
	    node->children[gdef+lcar].label = copy(_("Mark Attachment Classes"));
	    node->children[gdef+lcar].build = BuildMClass;
	    node->children[gdef+lcar].parent = node;
	}
    }
}

static void BuildBaseLangs(struct node *node,struct att_dlg *att) {
    struct baselangextent *bl = node->u.langs, *lf;
    int cnt;
    struct node *langs;
    char buffer[300];

    for ( lf = bl, cnt=0; lf!=NULL; lf=lf->next, ++cnt );

    node->children = langs = calloc(cnt+1,sizeof(struct node));
    node->cnt = cnt;

    for ( lf = bl, cnt=0; lf!=NULL; lf=lf->next, ++cnt ) {
	sprintf( buffer, _("%c%c%c%c  Min Extent=%d, Max Extent=%d"),
		lf->lang>>24, lf->lang>>16, lf->lang>>8, lf->lang,
		lf->descent, lf->ascent );
	langs[cnt].label = copy(buffer);
	langs[cnt].parent = node;
	if ( lf->features!=NULL ) {
	    langs[cnt].build = BuildBaseLangs;
	    langs[cnt].u.langs = lf->features;
	}
    }
}

static void BuildBASE(struct node *node,struct att_dlg *att) {
    SplineFont *_sf = att->sf;
    struct Base *base = node->horizontal ? _sf->horiz_base : _sf->vert_base;
    struct basescript *bs;
    int cnt, i;
    struct node *scripts;
    char buffer[300];

    for ( bs=base->scripts, cnt=0; bs!=NULL; bs=bs->next, ++cnt );

    node->children = scripts = calloc(cnt+1,sizeof(struct node));
    node->cnt = cnt;

    for ( bs=base->scripts, cnt=0; bs!=NULL; bs=bs->next, ++cnt ) {
	if ( base->baseline_cnt!=0 ) {
	    i = bs->def_baseline;
	    sprintf( buffer, _("Script '%c%c%c%c' on %c%c%c%c "),
		    bs->script>>24, bs->script>>16, bs->script>>8, bs->script,
		    base->baseline_tags[i]>>24, base->baseline_tags[i]>>16,
		    base->baseline_tags[i]>>8, base->baseline_tags[i] );
	    for ( i=0; i<base->baseline_cnt; ++i )
		sprintf(buffer+strlen(buffer), " %c%c%c%c: %d ",
			base->baseline_tags[i]>>24, base->baseline_tags[i]>>16,
			base->baseline_tags[i]>>8, base->baseline_tags[i],
			bs->baseline_pos[i]);
	} else
	    sprintf( buffer, _("Script '%c%c%c%c' "),
		    bs->script>>24, bs->script>>16, bs->script>>8, bs->script );
	scripts[cnt].label = copy(buffer);
	scripts[cnt].parent = node;
	if ( bs->langs!=NULL ) {
	    scripts[cnt].build = BuildBaseLangs;
	    scripts[cnt].u.langs = bs->langs;
	}
    }
}

static void BuildBsLnTable(struct node *node,struct att_dlg *att) {
    SplineFont *_sf = att->sf;
    int def_baseline;
    int offsets[32];
    int16 *baselines;
    char buffer[300];
    struct node *glyphs;
    int gid,i;
    SplineChar *sc;

    baselines = PerGlyphDefBaseline(_sf,&def_baseline);
    FigureBaseOffsets(_sf,def_baseline&0x1f,offsets);

    node->children = calloc(3+1,sizeof(struct node));
    node->cnt = 3;

    sprintf( buffer, _("Default Baseline: '%s'"),
	    (def_baseline&0x1f)==0 ? "romn" :
	    (def_baseline&0x1f)==1 ? "idcn" :
	    (def_baseline&0x1f)==2 ? "ideo" :
	    (def_baseline&0x1f)==3 ? "hang" :
	    (def_baseline&0x1f)==4 ? "math" : "????" );
    node->children[0].label = copy(buffer);
    node->children[0].parent = node;
    sprintf( buffer, _("Offsets from def. baseline:  romn: %d  idcn: %d  ideo: %d  hang: %d  math: %d"),
	    offsets[0], offsets[1], offsets[2], offsets[3], offsets[4] );
    node->children[1].label = copy(buffer);
    node->children[1].parent = node;
    if ( def_baseline&0x100 ) {
	node->children[2].label = copy(_("All glyphs have the same baseline"));
	node->children[2].parent = node;
    } else {
	node->children[2].label = copy(_("Per glyph baseline data"));
	node->children[2].parent = node;
	node->children[2].children_checked = true;
	node->children[2].children = glyphs = calloc(_sf->glyphcnt+1,sizeof(struct node));
	for ( gid=i=0; gid<_sf->glyphcnt; ++gid ) if ( (sc=_sf->glyphs[gid])!=NULL ) {
	    sprintf( buffer, "%s: %s", sc->name,
		    (baselines[gid])==0 ? "romn" :
		    (baselines[gid])==1 ? "idcn" :
		    (baselines[gid])==2 ? "ideo" :
		    (baselines[gid])==3 ? "hang" :
		    (baselines[gid])==4 ? "math" : "????" );
	    glyphs[i].label = copy(buffer);
	    glyphs[i++].parent = &node->children[2];
	}
	node->children[2].cnt = i;
    }
    free(baselines);
}

static void BuildOpticalBounds(struct node *node,struct att_dlg *att) {
    SplineFont *sf, *_sf = att->sf;
    int i, cmax, l,j, ccnt;
    SplineChar *sc;
    struct node *chars;
    char buffer[200];
    PST *left, *right;

    cmax = 0;
    l = 0;
    do {
	sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[l];
	if ( cmax<sf->glyphcnt ) cmax = sf->glyphcnt;
	++l;
    } while ( l<_sf->subfontcnt );

    chars = NULL;
    for ( j=0; j<2; ++j ) {
	ccnt = 0;
	for ( i=0; i<cmax; ++i ) {
	    l = 0;
	    sc = NULL;
	    do {
		sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[l];
		if ( i<sf->glyphcnt && sf->glyphs[i]!=NULL ) {
		    sc = sf->glyphs[i];
	    break;
		}
		++l;
	    } while ( l<_sf->subfontcnt );
	    if ( sc!=NULL && SCWorthOutputting(sc) &&
		    haslrbounds(sc,&left,&right)) {
		if ( chars!=NULL ) {
		    strncpy(buffer,sc->name,70);
		    if ( left!=NULL )
			sprintf(buffer+strlen(buffer), _("  Left Bound=%d"),
				left->u.pos.xoff );
		    if ( right!=NULL )
			sprintf(buffer+strlen(buffer), _("  Right Bound=%d"),
				-right->u.pos.h_adv_off );
		    chars[ccnt].parent = node;
		    chars[ccnt].label = copy(buffer);
		}
		++ccnt;
	    }
	}
	if ( ccnt==0 )
return;
	if ( chars==NULL ) {
	    node->children = chars = calloc(ccnt+1,sizeof(struct node));
	    node->cnt = ccnt;
	}
    }
}

static void BuildProperties(struct node *node,struct att_dlg *att) {
    SplineFont *sf, *_sf = att->sf;
    int i, cmax, l,j,k, ccnt;
    SplineChar *sc;
    struct node *chars;
    uint16 *props;
    char buffer[200];

    cmax = 0;
    l = 0;
    do {
	sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[l];
	if ( cmax<sf->glyphcnt ) cmax = sf->glyphcnt;
	++l;
    } while ( l<_sf->subfontcnt );

    chars = NULL; props = NULL;
    for ( j=0; j<2; ++j ) {
	ccnt = 0;
	for ( i=0; i<cmax; ++i ) {
	    l = 0;
	    sc = NULL;
	    do {
		sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[l];
		if ( i<sf->glyphcnt && sf->glyphs[i]!=NULL ) {
		    sc = sf->glyphs[i];
	    break;
		}
		++l;
	    } while ( l<_sf->subfontcnt );
	    if ( sc!=NULL ) {
		if ( chars==NULL ) {
		    if ( SCWorthOutputting(sc))
			sc->ttf_glyph = ccnt++;
		    else
			sc->ttf_glyph = -1;
		} else if ( sc->ttf_glyph!=-1 ) {
		    int prop = props[sc->ttf_glyph], offset;
		    sprintf( buffer, "%.70s  dir=%s", sc->name,
			(prop&0x7f)==0 ? _("Strong Left to Right"):
			(prop&0x7f)==1 ? _("Strong Right to Left"):
			(prop&0x7f)==2 ? _("Arabic Right to Left"):
			(prop&0x7f)==3 ? _("European Number"):
			(prop&0x7f)==4 ? _("European Number Separator"):
			(prop&0x7f)==5 ? _("European Number Terminator"):
			(prop&0x7f)==6 ? _("Arabic Number"):
			(prop&0x7f)==7 ? _("Common Number Separator"):
			(prop&0x7f)==8 ? _("Block Separator"):
			(prop&0x7f)==9 ? _("Segment Separator"):
			(prop&0x7f)==10 ? _("White Space"):
			(prop&0x7f)==11 ? _("Neutral"):
			    _("<Unknown direction>") );
		    if ( prop&0x8000 )
			strcat(buffer,_("  Floating accent"));
		    if ( prop&0x4000 )
			strcat(buffer,_("  Hang left"));
		    if ( prop&0x2000 )
			strcat(buffer,_("  Hang right"));
		    if ( prop&0x80 )
			strcat(buffer,_("  Attach right"));
		    if ( prop&0x1000 ) {
			offset = (prop&0xf00)>>8;
			if ( offset&0x8 )
			    offset |= 0xfffffff0;
			if ( offset>0 ) {
			    for ( k=i+offset; k<sf->glyphcnt; ++k )
				if ( sf->glyphs[k]!=NULL && sf->glyphs[k]->ttf_glyph==sc->ttf_glyph+offset ) {
				    sprintf( buffer+strlen(buffer), _("  Mirror=%.30s"), sf->glyphs[k]->name );
			    break;
				}
			} else {
			    for ( k=i+offset; k>=0; --k )
				if ( sf->glyphs[k]!=NULL && sf->glyphs[k]->ttf_glyph==sc->ttf_glyph+offset ) {
				    sprintf( buffer+strlen(buffer), _("  Mirror=%.30s"), sf->glyphs[k]->name );
			    break;
				}
			}
		    }
		    chars[ccnt].parent = node;
		    chars[ccnt++].label = copy(buffer);
		}
	    }
	}
	if ( chars==NULL ) {
	    struct glyphinfo gi;
	    memset(&gi,0,sizeof(gi)); gi.gcnt = _sf->glyphcnt;
	    props = props_array(_sf,&gi);
	    if ( props==NULL )
return;
	    node->children = chars = calloc(ccnt+1,sizeof(struct node));
	}
	node->cnt = ccnt;
    }
    free(props);
}

static void BuildKernTable(struct node *node,struct att_dlg *att) {
    SplineFont *_sf = att->sf;
    OTLookup *otl;
    FeatureScriptLangList *fl;
    int doit, cnt;
    struct node *kerns = NULL;

    if ( _sf->cidmaster ) _sf = _sf->cidmaster;

    for ( doit=0; doit<2; ++doit ) {
	cnt = 0;
	for ( otl = _sf->gpos_lookups; otl!=NULL ; otl=otl->next ) {
	    for ( fl=otl->features; fl!=NULL; fl=fl->next ) {
		if ( (fl->featuretag==CHR('k','e','r','n') || fl->featuretag==CHR('v','k','r','n')) &&
			scriptsHaveDefault(fl->scripts))
	    break;
	    }
	    if ( otl->lookup_type == gpos_pair && fl!=NULL ) {
		if ( doit ) {
		    kerns[cnt].parent = node;
		    kerns[cnt].build = BuildGSUBlookups;
		    kerns[cnt].label = copy(otl->lookup_name);
		    kerns[cnt].u.otl = otl;
		}
		++cnt;
	    }
	}
	if ( !doit ) {
	    node->children = kerns = calloc(cnt+1,sizeof(struct node));
	    node->cnt = cnt;
	}
    }
}

static void BuildMorxTable(struct node *node,struct att_dlg *att) {
    SplineFont *_sf = att->sf;
    OTLookup *otl;
    FeatureScriptLangList *fl;
    int doit, cnt;
    struct node *lookups = NULL;
    int feat, set;

    if ( _sf->cidmaster ) _sf = _sf->cidmaster;

    for ( doit=0; doit<2; ++doit ) {
	cnt = 0;
	for ( otl = _sf->gsub_lookups; otl!=NULL ; otl=otl->next ) {
	    for ( fl=otl->features; fl!=NULL; fl=fl->next ) {
		if ( fl->ismac ||
			(OTTagToMacFeature(fl->featuretag,&feat,&set) &&
			 scriptsHaveDefault(fl->scripts) &&
			 (otl->lookup_type==gsub_single || otl->lookup_type==gsub_ligature)))
		break;
	    }
	    if ( fl!=NULL ) {
		if ( doit ) {
		    lookups[cnt].parent = node;
		    lookups[cnt].build = BuildGSUBlookups;
		    lookups[cnt].label = copy(otl->lookup_name);
		    lookups[cnt].u.otl = otl;
		}
		++cnt;
	    }
	}
	if ( !doit ) {
	    node->children = lookups = calloc(cnt+1,sizeof(struct node));
	    node->cnt = cnt;
	}
    }
}

static void BuildTable(struct node *node,struct att_dlg *att) {
    SplineFont *_sf = att->sf;
    int script_max;
    int i,j;
    uint32 *scriptlist;
    struct node *scriptnodes;
    extern GTextInfo scripts[];
    int isgsub = node->tag==CHR('G','S','U','B');
    char buf[120];

    /* Build the list of scripts that are mentioned in the font */
    scriptlist = SFScriptsInLookups(_sf,!isgsub);
    if ( scriptlist==NULL )
return;
    for ( i=0; scriptlist[i]!=0; ++i );
    script_max = i;
    scriptnodes = calloc(script_max+1,sizeof(struct node));
    for ( i=0; scriptlist[i]!=0; ++i )
	scriptnodes[i].tag = scriptlist[i];
    free( scriptlist );

    for ( i=0; i<script_max; ++i ) {
	for ( j=0; scripts[j].text!=NULL && scriptnodes[i].tag!=(uint32) (intpt) scripts[j].userdata; ++j );
	buf[0] = '\'';
	buf[1] = scriptnodes[i].tag>>24;
	buf[2] = (scriptnodes[i].tag>>16)&0xff;
	buf[3] = (scriptnodes[i].tag>>8)&0xff;
	buf[4] = scriptnodes[i].tag&0xff;
	buf[5] = '\'';
	buf[6] = ' ';
	if ( scripts[j].text!=NULL ) {
	    strcpy(buf+7,S_((char*) scripts[j].text));
	    strcat(buf," ");
	} else
	    buf[7]='\0';
/* GT: See the long comment at "Property|New" */
/* GT: The msgstr should contain a translation of "Script", ignore "writing system|" */
/* GT: English uses "script" to mean a general writing style (latin, greek, kanji) */
/* GT: and the cursive handwriting style. Here we mean the general writing system. */
	strcat(buf,S_("writing system|Script"));
	scriptnodes[i].label = copy(buf);
	scriptnodes[i].build = BuildGSUBscript;
	scriptnodes[i].parent = node;
    }
    node->children = scriptnodes;
    node->cnt = i;
}

static void BuildJSTFTable(struct node *node,struct att_dlg *att) {
    SplineFont *_sf = att->sf;
    int sub_cnt,i,j;
    Justify *jscript;
    struct node *scriptnodes;
    char buf[120];
    extern GTextInfo scripts[];

    for ( sub_cnt=0, jscript=_sf->justify; jscript!=NULL; jscript=jscript->next, ++sub_cnt );
    scriptnodes = calloc(sub_cnt+1,sizeof(struct node));
    for ( i=0, jscript=_sf->justify; jscript!=NULL; jscript=jscript->next, ++i ) {
	scriptnodes[i].tag = jscript->script;
	for ( j=0; scripts[j].text!=NULL && scriptnodes[i].tag!=(uint32) (intpt) scripts[j].userdata; ++j );
	buf[0] = '\'';
	buf[1] = scriptnodes[i].tag>>24;
	buf[2] = (scriptnodes[i].tag>>16)&0xff;
	buf[3] = (scriptnodes[i].tag>>8)&0xff;
	buf[4] = scriptnodes[i].tag&0xff;
	buf[5] = '\'';
	buf[6] = ' ';
	if ( scripts[j].text!=NULL ) {
	    strcpy(buf+7,S_((char*) scripts[j].text));
	    strcat(buf," ");
	} else
	    buf[7]='\0';
/* GT: See the long comment at "Property|New" */
/* GT: The msgstr should contain a translation of "Script", ignore "writing system|" */
/* GT: English uses "script" to me a general writing style (latin, greek, kanji) */
/* GT: and the cursive handwriting style. Here we mean the general writing system. */
	strcat(buf,S_("writing system|Script"));
	scriptnodes[i].label = copy(buf);
	scriptnodes[i].build = BuildJSTFscript;
	scriptnodes[i].parent = node;
	scriptnodes[i].u.jscript = jscript;
    }
    node->children = scriptnodes;
    node->cnt = i;
}

static void BuildTop(struct att_dlg *att) {
    SplineFont *sf, *_sf = att->sf;
    int hasgsub=0, hasgpos=0, hasgdef=0, hasbase=0, hasjstf=0;
    int hasmorx=0, haskern=0, hasvkern=0, haslcar=0, hasprop=0, hasopbd=0, hasbsln=0;
    int haskc=0, hasvkc=0;
    int feat, set;
    struct node *tables;
    PST *pst;
    SplineChar *sc;
    int i,k,j;
    AnchorClass *ac;
    OTLookup *otl;
    FeatureScriptLangList *fl;
    char buffer[200];

    SFFindClearUnusedLookupBits(_sf);

    for ( otl=_sf->gsub_lookups; otl!=NULL; otl=otl->next ) {
	for ( fl = otl->features; fl!=NULL ; fl=fl->next ) {
	    if ( !fl->ismac )
		hasgsub = true;
	    if ( fl->ismac ||
		    (OTTagToMacFeature(fl->featuretag,&feat,&set) &&
		     scriptsHaveDefault(fl->scripts) &&
		     (otl->lookup_type==gsub_single || otl->lookup_type==gsub_ligature)))
		hasmorx = true;
	}
    }
    for ( otl=_sf->gpos_lookups; otl!=NULL; otl=otl->next ) {
	if ( otl->lookup_type==kern_statemachine )
	    haskern = true;
	else
	    hasgpos = true;
	if ( otl->lookup_type == gpos_single )
	    for ( fl = otl->features; fl!=NULL ; fl=fl->next ) {
		if ( fl->featuretag==CHR('l','f','b','d') || fl->featuretag==CHR('r','t','b','d') )
		    hasopbd = true;
	    }
    }
	    
    k=0;
    do {
	sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[k];
	for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL ) {
	    if (( sc->unicodeenc>=0x10800 && sc->unicodeenc<=0x10fff ) ||
		    ( sc->unicodeenc!=-1 && sc->unicodeenc<0x10fff &&
			isrighttoleft(sc->unicodeenc)) ||
			ScriptIsRightToLeft(SCScriptFromUnicode(sc)) ) {
		hasprop = true;
	    }
	    if ( sc->glyph_class!=0 )
		hasgdef = true;
	    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
		if ( pst->type == pst_lcaret ) {
		    for ( j=pst->u.lcaret.cnt-1; j>=0; --j )
			if ( pst->u.lcaret.carets[j]!=0 )
		    break;
		    if ( j!=-1 )
			hasgdef = haslcar = true;
		}
	    }
	    if ( sc->kerns!=NULL || sc->vkerns!=NULL )
		haskern = hasgpos = true;
	}
	++k;
    } while ( k<_sf->subfontcnt );
    if ( _sf->vkerns!=NULL || _sf->kerns!=NULL )
	haskern = hasgpos = true;
    if ( _sf->anchor!=NULL )
	hasgpos = true;
    for ( ac = sf->anchor; ac!=NULL; ac=ac->next ) {
	if ( ac->type==act_curs )
    break;
    }
    if ( ac!=NULL )
	hasgdef = true;
    hasbase = ( _sf->horiz_base!=NULL || _sf->vert_base!=NULL );
    hasbsln = ( _sf->horiz_base!=NULL && _sf->horiz_base->baseline_cnt!=0 );
    hasjstf = ( _sf->justify!=NULL );

    if ( hasgsub+hasgpos+hasgdef+hasmorx+haskern+haslcar+hasopbd+hasprop+hasbase+hasjstf==0 ) {
	tables = calloc(2,sizeof(struct node));
	tables[0].label = copy(_("No Advanced Typography"));
    } else {
	tables = calloc((hasgsub||hasgpos||hasgdef||hasbase||hasjstf)+
	    (hasmorx||haskern||haslcar||hasopbd||hasprop||hasbsln)+1,sizeof(struct node));
	i=0;
	if ( hasgsub || hasgpos || hasgdef || hasbase || hasjstf ) {
	    tables[i].label = copy(_("OpenType Tables"));
	    tables[i].children_checked = true;
	    tables[i].children = calloc(hasgsub+hasgpos+hasgdef+hasbase+hasjstf+1,sizeof(struct node));
	    tables[i].cnt = hasgsub + hasgpos + hasgdef + hasbase + hasjstf;
	    if ( hasbase ) {
		int sub_cnt= (sf->horiz_base!=NULL) + (sf->vert_base!=NULL), j=0;
		tables[i].children[0].label = copy(_("'BASE' Baseline Table"));
		tables[i].children[0].tag = CHR('B','A','S','E');
		tables[i].children[0].parent = &tables[i];
		tables[i].children[0].children_checked = true;
		tables[i].children[0].children = calloc(sub_cnt+1,sizeof(struct node));
		tables[i].children[0].cnt = sub_cnt;
		if ( _sf->horiz_base!=NULL ) {
		    snprintf(buffer,sizeof(buffer),
			    P_("Horizontal: %d baseline","Horizontal: %d baselines",_sf->horiz_base->baseline_cnt),
			    _sf->horiz_base->baseline_cnt );
		    tables[i].children[0].children[j].label = copy(buffer);
		    tables[i].children[0].children[j].horizontal = true;
		    tables[i].children[0].children[j].parent = &tables[i].children[0];
		    tables[i].children[0].children[j].build = BuildBASE;
		    ++j;
		}
		if ( _sf->vert_base!=NULL ) {
		    snprintf(buffer,sizeof(buffer),
			    P_("Vertical: %d baseline","Vertical: %d baselines",_sf->vert_base->baseline_cnt),
			    _sf->vert_base->baseline_cnt );
		    tables[i].children[0].children[j].label = copy(buffer);
		    tables[i].children[0].children[j].horizontal = false;
		    tables[i].children[0].children[j].parent = &tables[i].children[0];
		    tables[i].children[0].children[j].build = BuildBASE;
		    ++j;
		}
	    }
	    if ( hasgdef ) {
		tables[i].children[hasbase].label = copy(_("'GDEF' Glyph Definition Table"));
		tables[i].children[hasbase].tag = CHR('G','D','E','F');
		tables[i].children[hasbase].build = BuildGDEF;
		tables[i].children[hasbase].parent = &tables[i];
	    }
	    if ( hasgpos ) {
		tables[i].children[hasgdef+hasbase].label = copy(_("'GPOS' Glyph Positioning Table"));
		tables[i].children[hasgdef+hasbase].tag = CHR('G','P','O','S');
		tables[i].children[hasgdef+hasbase].build = BuildTable;
		tables[i].children[hasgdef+hasbase].parent = &tables[i];
	    }
	    if ( hasgsub ) {
		tables[i].children[hasgdef+hasgpos+hasbase].label = copy(_("'GSUB' Glyph Substitution Table"));
		tables[i].children[hasgdef+hasgpos+hasbase].tag = CHR('G','S','U','B');
		tables[i].children[hasgdef+hasgpos+hasbase].build = BuildTable;
		tables[i].children[hasgdef+hasgpos+hasbase].parent = &tables[i];
	    }
	    if ( hasjstf ) {
		int k = hasgdef+hasgpos+hasbase+hasgsub;
		tables[i].children[k].label = copy(_("'JSTF' Justification Table"));
		tables[i].children[k].tag = CHR('J','S','T','F');
		tables[i].children[k].parent = &tables[i];
		tables[i].children[k].build = BuildJSTFTable;
	    }
	    ++i;
	}
	if ( hasmorx || haskern || haslcar || hasopbd || hasprop || hasbsln ) {
	    int j = 0;
	    tables[i].label = copy(_("Apple Advanced Typography"));
	    tables[i].children_checked = true;
	    tables[i].children = calloc(hasmorx+haskern+haslcar+hasopbd+hasprop+hasvkern+haskc+hasvkc+hasbsln+1,sizeof(struct node));
	    tables[i].cnt = hasmorx+haskern+hasopbd+hasprop+haslcar+hasvkern+haskc+hasvkc+hasbsln;
	    if ( hasbsln ) {
		tables[i].children[j].label = copy(_("'bsln' Horizontal Baseline Table"));
		tables[i].children[j].tag = CHR('b','s','l','n');
		tables[i].children[j].build = BuildBsLnTable;
		tables[i].children[j++].parent = &tables[i];
	    }
	    if ( haskern ) {
		tables[i].children[j].label = copy(_("'kern' Horizontal Kerning Table"));
		tables[i].children[j].tag = CHR('k','e','r','n');
		tables[i].children[j].build = BuildKernTable;
		tables[i].children[j++].parent = &tables[i];
	    }
	    if ( haslcar ) {
		tables[i].children[j].label = copy(_("'lcar' Ligature Caret Table"));
		tables[i].children[j].tag = CHR('l','c','a','r');
		tables[i].children[j].build = BuildLcar;
		tables[i].children[j++].parent = &tables[i];
	    }
	    if ( hasmorx ) {
		tables[i].children[j].label = copy(_("'morx' Glyph Extended Metamorphosis Table"));
		tables[i].children[j].tag = CHR('m','o','r','x');
		tables[i].children[j].build = BuildMorxTable;
		tables[i].children[j++].parent = &tables[i];
	    }
	    if ( hasopbd ) {
		tables[i].children[j].label = copy(_("'opbd' Optical Bounds Table"));
		tables[i].children[j].tag = CHR('o','p','b','d');
		tables[i].children[j].build = BuildOpticalBounds;
		tables[i].children[j++].parent = &tables[i];
	    }
	    if ( hasprop ) {
		tables[i].children[j].label = copy(_("'prop' Glyph Properties Table"));
		tables[i].children[j].tag = CHR('p','r','o','p');
		tables[i].children[j].build = BuildProperties;
		tables[i].children[j++].parent = &tables[i];
	    }
	    ++i;
	}
    }

    att->tables = tables;
    att->current = tables;
}

static int _SizeCnt(struct att_dlg *att,struct node *node, int lpos,int depth) {
    int i, len;

    if ( node->monospace )
	GDrawSetFont(att->v,att->monofont);
    node->lpos = lpos++;
    len = 5+8*depth+ att->as + 5 + GDrawGetText8Width(att->v,node->label,-1);
    if ( len>att->maxl ) att->maxl = len;
    if ( node->monospace )
	GDrawSetFont(att->v,att->font);

    if ( node->open ) {
	if ( !node->children_checked && node->build!=NULL ) {
	    (node->build)(node,att);
	    node->children_checked = true;
	}
	for ( i=0; i<node->cnt; ++i )
	    lpos = _SizeCnt(att,&node->children[i],lpos,depth+1);
    }
return( lpos );
}

static int SizeCnt(struct att_dlg *att,struct node *node, int lpos) {
    att->maxl = 0;
    GDrawSetFont(att->v,att->font);
    while ( node->label ) {
	lpos = _SizeCnt(att,node,lpos,0);
	++node;
    }

    GScrollBarSetBounds(att->vsb,0,lpos,att->lines_page);
    GScrollBarSetBounds(att->hsb,0,att->maxl,att->page_width);
    att->open_cnt = lpos;
return( lpos );
}

static struct node *NodeFindLPos(struct node *node,int lpos,int *depth) {
    for (;;) {
	if ( node->lpos==lpos )
return( node );
	if ( node[1].label!=NULL && node[1].lpos<=lpos )
	    ++node;
	else if ( node->children==NULL || !node->open )
return( NULL );
	else {
	    node = node->children;
	    ++*depth;
	}
    }
}

static struct node *NodeNext(struct node *node,int *depth) {
    if ( node->open && node->children && node->children[0].label ) {
	++*depth;
return( node->children );
    }
    for (;;) {
	if ( node[1].label )
return( node+1 );
	node = node->parent;
	--*depth;
	if ( node==NULL )
return( NULL );
    }
}

static struct node *NodePrev(struct att_dlg *att, struct node *node,int *depth) {
    while ( node->parent!=NULL && node==node->parent->children ) {
	node = node->parent;
	--*depth;
    }
    if ( node->parent==NULL && node==att->tables )
return( NULL );
    --node;
    while ( node->open ) {
	node = &node->children[node->cnt-1];
	++*depth;
    }
return( node );
}

static char *findstartquote(char *str) {
    static int quotes[] = { '"', 0x00ab, 0x2018, 0x201b, 0x201c, 0x201e, 0 };
    char *last, *cur;
    int i, ch;

    for ( cur=str; *cur!='\0'; ) {
	last = cur;
	ch = utf8_ildb((const char **) &cur);
	for ( i=0; quotes[i]!=0; ++i )
	    if ( ch==quotes[i] )
return( last );
    }
return( NULL );
}

static char *findendquote(char *str) {
    static int endquotes[] = { '"', 0x00bb, 0x2019, 0x201b, 0x201d, 0x201e, 0 };
    char *last, *cur;
    int i, ch;

    /* startquote =*/ utf8_ildb((const char **) &str);
    for ( cur=str; *cur!='\0'; ) {
	last = cur;
	ch = utf8_ildb((const char **) &cur);
	if ( ch==' ' )
return( NULL );
	for ( i=0; endquotes[i]!=0; ++i )
	    if ( ch==endquotes[i] )
return( last );
    }
return( NULL );
}

static void AttExpose(struct att_dlg *att,GWindow pixmap,GRect *rect) {
    int depth, y;
    struct node *node;
    GRect r;
    Color fg;
    char *spt, *ept;

    GDrawFillRect(pixmap,rect,GDrawGetDefaultBackground(NULL));
    GDrawSetLineWidth(pixmap,0);

    r.height = r.width = att->as;
    y = (rect->y/att->fh) * att->fh + att->as;
    depth=0;
    node = NodeFindLPos(att->tables,rect->y/att->fh+att->off_top,&depth);
    GDrawSetFont(pixmap,att->font);
    while ( node!=NULL ) {
	r.y = y-att->as+1;
	r.x = 5+8*depth - att->off_left;
	fg = node==att->current ? 0xff0000 : 0x000000;
	if ( node->build || node->children ) {
	    GDrawDrawRect(pixmap,&r,fg);
	    GDrawDrawLine(pixmap,r.x+2,r.y+att->as/2,r.x+att->as-2,r.y+att->as/2,
		    fg);
	    if ( !node->open )
		GDrawDrawLine(pixmap,r.x+att->as/2,r.y+2,r.x+att->as/2,r.y+att->as-2,
			fg);
	}
	if ( node->monospace )
	    GDrawSetFont(pixmap,att->monofont);
	ept = NULL;
	if ( att->dlg_type==dt_font_comp ) {
	    if ( (spt = findstartquote(node->label))!=NULL )
		ept = findendquote(spt);
	}
	if ( ept==NULL )
	    GDrawDrawText8(pixmap,r.x+r.width+5,y,node->label,-1,fg);
	else {
	    int len;
	    len = GDrawDrawText8(pixmap,r.x+r.width+5,y,node->label,spt-node->label,fg);
	    len += GDrawDrawText8(pixmap,r.x+r.width+5+len,y,spt,ept-spt,0x0000ff);
	    GDrawDrawText8(pixmap,r.x+r.width+5+len,y,ept,-1,fg);
	}
	if ( node->monospace )
	    GDrawSetFont(pixmap,att->font);
	node = NodeNext(node,&depth);
	y += att->fh;
	if ( y-att->fh>rect->y+rect->height )
    break;
    }
}

static void ATTChangeCurrent(struct att_dlg *att,struct node *node) {
    int oldl = att->current->lpos, newl;
    GRect r;
    if ( node==NULL )
return;
    newl = node->lpos;
    att->current = node;
    r.x =0; r.width = 3000;
    if ( newl<att->off_top || newl>=att->off_top+att->lines_page ) {
	att->off_top = newl-att->lines_page/3;
	if ( att->off_top<0 ) att->off_top = 0;
	GScrollBarSetPos(att->vsb,att->off_top);
	GDrawRequestExpose(att->v,NULL,false);
    } else if ( newl==oldl+1 ) {
	r.y = (oldl-att->off_top)*att->fh; r.height = 2*att->fh;
	GDrawRequestExpose(att->v,&r,false);
    } else if ( newl==oldl-1 ) {
	r.y = (newl-att->off_top)*att->fh; r.height = 2*att->fh;
	GDrawRequestExpose(att->v,&r,false);
    } else {
	r.y = (newl-att->off_top)*att->fh; r.height = att->fh;
	GDrawRequestExpose(att->v,&r,false);
	r.y = (oldl-att->off_top)*att->fh; r.height = att->fh;
	GDrawRequestExpose(att->v,&r,false);
    }
}

static void pututf8(uint32 ch,FILE *file) {
    if ( ch<0x80 )
	putc(ch,file);
    else if ( ch<0x800 ) {
	putc(0xc0 | (ch>>6), file);
	putc(0x80 | (ch&0x3f), file);
    } else {
	putc( 0xe0 | (ch>>12),file );
	putc( 0x80 | ((ch>>6)&0x3f),file );
	putc( 0x80 | (ch&0x3f),file );
    }
}

static void AttSave(struct att_dlg *att) {
    char *ret = gwwv_save_filename(_("Save"),NULL,
	    "*.txt");
    char *cret;
    FILE *file;
    char *pt;
    struct node *node;
    int depth=0, d;

    if ( ret==NULL )
return;
    cret = utf82def_copy(ret);
    file = fopen(cret,"w");
    free(cret);
    if ( file==NULL ) {
	ff_post_error(_("Save Failed"),_("Save Failed"),ret);
	free(ret);
return;
    }
    free(ret);

    pututf8(0xfeff,file);	/* Zero width something or other. Marks this as unicode, utf8 */
    node = NodeFindLPos(att->tables,0,&depth);
    while ( node!=NULL ) {
	d = depth;
	while ( d>=4 ) {
	    pututf8('\t',file);
	    d -= 4;
	}
	while ( d>0 ) {
	    pututf8(' ',file);
	    pututf8(' ',file);
	    --d;
	}
	if ( !node->build && !node->children )
	    pututf8(' ',file);
	else if ( node->open )
	    pututf8('-',file);
	else
	    pututf8('+',file);
	for ( pt=node->label; *pt; ++pt )
	    putc(*pt,file);
	pututf8('\n',file);
	node = NodeNext(node,&depth);
    }
    fclose(file);
}

static void AttSaveM(GWindow gw, GMenuItem *mi,GEvent *e) {
    struct att_dlg *att = (struct att_dlg *) GDrawGetUserData(gw);
    AttSave(att);
}

static GMenuItem att_popuplist[] = {
    { { (unichar_t *) N_("Save"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, 'S' }, 'S', ksm_control, NULL, NULL, AttSaveM, 0 },
    GMENUITEM_EMPTY
};

static FontView *FVVerify(FontView *fv) {
    FontView *test;

    for ( test= fv_list; test!=NULL && test!=fv; test=(FontView *) test->b.next );
return( test );
}

static void FontCompActivate(struct att_dlg *att,struct node *node) {
    char *pt, *pt2; int ch;
    SplineChar *sc1=NULL, *sc2=NULL;
    int size=0, depth=0;
    BDFFont *bdf1, *bdf2;

    att->fv1 = FVVerify(att->fv1);
    att->fv2 = FVVerify(att->fv2);

    pt = findstartquote(node->label);
    if ( pt==NULL )
return;
    pt2 = findendquote(pt);
    if ( pt2==NULL )
return;
    utf8_ildb((const char **) &pt);
    ch = *pt2; *pt2 = '\0';
    if ( att->fv1!=NULL )
	sc1 = SFGetChar(att->fv1->b.sf,-1,pt);
    if ( att->fv2!=NULL )
	sc2 = SFGetChar(att->fv2->b.sf,-1,pt);
    *pt2 = ch;

    pt = strchr(node->label,'@');
    if ( pt!=NULL ) {
	for (pt2 = pt-1; pt2>=node->label && isdigit(*pt2); --pt2 );
	size = strtol(pt2+1,NULL,10);
	depth = strtol(pt+1,NULL,10);
    }
    if ( size!=0 && depth!=0 ) {
	for ( bdf1 = att->fv1->b.sf->bitmaps; bdf1!=NULL &&
		(bdf1->pixelsize!=size || BDFDepth(bdf1)!=depth); bdf1=bdf1->next );
	for ( bdf2 = att->fv2->b.sf->bitmaps; bdf2!=NULL &&
		(bdf2->pixelsize!=size || BDFDepth(bdf2)!=depth); bdf2=bdf2->next );
	if ( bdf1!=NULL && sc1!=NULL && sc1->orig_pos<bdf1->glyphcnt &&
		bdf1->glyphs[sc1->orig_pos]!=NULL )
	    BitmapViewCreate(bdf1->glyphs[sc1->orig_pos],bdf1,att->fv1,
		    att->fv1->b.map->backmap[sc1->orig_pos]);
	if ( bdf2!=NULL && sc2!=NULL && sc2->orig_pos<bdf2->glyphcnt &&
		bdf2->glyphs[sc2->orig_pos]!=NULL )
	    BitmapViewCreate(bdf2->glyphs[sc2->orig_pos],bdf2,att->fv2,
		    att->fv2->b.map->backmap[sc2->orig_pos]);
    } else {
	if ( sc1!=NULL )
	    CharViewCreate(sc1,att->fv1,att->fv1->b.map->backmap[sc1->orig_pos]);
	if ( sc2!=NULL )
	    CharViewCreate(sc2,att->fv2,att->fv2->b.map->backmap[sc2->orig_pos]);
    }
}

static void _ATT_FreeImage(const void *_ci, GImage *img) {
    GImageDestroy(img);
}

static GImage *_ATT_PopupImage(const void *_att) {
    const struct att_dlg *att = _att;
    char *start, *pt;
    int ch;
    SplineChar *sc;
    int isliga;

    if ( att->popup_node==NULL || att->popup_node->label==NULL )
return( NULL );
    for ( start=att->popup_node->label; *start==' ' || isdigit(*start); ++start );
    for ( pt=start; *pt!='\0' && *pt!=' '; ++pt );
    ch = *pt; *pt = '\0';
    sc = SFGetChar(att->sf,-1,start);
    *pt = ch;
    if ( sc==NULL )
return( NULL );

    isliga = -1;
    while ( *pt==' ' || *pt=='=' || *pt=='>' || *pt=='<' ) {
	if ( *pt=='<' ) isliga = true;
	if ( *pt=='>' ) isliga = false;
	++pt;
    }
    if ( !isalpha(*pt))		/* If alphabetic, then show the glyph names that follow too. Otherwise show nothing for gpos lookups */
	pt = "";
return( NameList_GetImage(att->sf,sc,att->def_layer,pt,isliga));
}

static void ATTStartPopup(struct att_dlg *att,struct node *node) {
    att->popup_node = node;
    GGadgetPreparePopupImage(att->v,NULL,att,_ATT_PopupImage,_ATT_FreeImage);
}

static void AttMouse(struct att_dlg *att,GEvent *event) {
    int l, depth;
    struct node *node;
    GRect r;

    if ( event->type==et_mousedown ) {
	GGadgetEndPopup();
	if ( event->u.mouse.button==3 ) {
	    static int done=false;
	    if ( !done ) {
		done = true;
		att_popuplist[0].ti.text = (unichar_t *) _( (char *)att_popuplist[0].ti.text);
	    }
	    GMenuCreatePopupMenu(att->v,event, att_popuplist);
	}
return;
    }

    l = (event->u.mouse.y/att->fh);
    depth=0;
    node = NodeFindLPos(att->tables,l+att->off_top,&depth);
    if ( event->type==et_mouseup ) {
	ATTChangeCurrent(att,node);
	if ( event->u.mouse.y > l*att->fh+att->as ||
		event->u.mouse.x<5+8*depth ||
		event->u.mouse.x>=5+8*depth+att->as || node==NULL ) {
		/* Not in +/- rectangle */
	    if ( event->u.mouse.x > 5+8*depth+att->as && node!=NULL &&
		    att->dlg_type == dt_font_comp && event->u.mouse.clicks>1 )
		FontCompActivate(att,node);
return;
	}
	node->open = !node->open;

	SizeCnt(att,att->tables,0);

	r.x = 0; r.width = 3000;
	r.y = l*att->fh; r.height = 3000;
	GDrawRequestExpose(att->v,&r,false);
    } else if ( event->type == et_mousemove ) {
	GGadgetEndPopup();
        ATTStartPopup(att,node);
    }
}

static void AttScroll(struct att_dlg *att,struct sbevent *sb) {
    int newpos = att->off_top;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= att->lines_page;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
        newpos += att->lines_page;
      break;
      case et_sb_bottom:
        newpos = att->open_cnt-att->lines_page;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos>att->open_cnt-att->lines_page )
        newpos = att->open_cnt-att->lines_page;
    if ( newpos<0 ) newpos =0;
    if ( newpos!=att->off_top ) {
	int diff = newpos-att->off_top;
	att->off_top = newpos;
	GScrollBarSetPos(att->vsb,att->off_top);
	GDrawScroll(att->v,NULL,0,diff*att->fh);
    }
}


static void AttHScroll(struct att_dlg *att,struct sbevent *sb) {
    int newpos = att->off_left;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= att->page_width;
      break;
  /* I'd like to scroll by one character. .6*em is right for courier. */
      case et_sb_up:
        newpos -= (6*att->fh)/10;
      break;
      case et_sb_down:
        newpos += (6*att->fh)/10;
      break;
      case et_sb_downpage:
        newpos += att->page_width;
      break;
      case et_sb_bottom:
        newpos = att->maxl-att->page_width;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos>att->maxl-att->page_width )
        newpos = att->maxl-att->page_width;
    if ( newpos<0 ) newpos =0;
    if ( newpos!=att->off_left ) {
	int diff = newpos-att->off_left;
	att->off_left = newpos;
	GScrollBarSetPos(att->hsb,att->off_left);
	GDrawScroll(att->v,NULL,-diff,0);
    }
}

static void AttResize(struct att_dlg *att,GEvent *event) {
    GRect size, wsize;
    int lcnt;
    int sbsize = GDrawPointsToPixels(att->gw,_GScrollBar_Width);

    GDrawGetSize(att->gw,&size);
    lcnt = (size.height-att->bmargin)/att->fh;
    GGadgetResize(att->vsb,sbsize,lcnt*att->fh);
    GGadgetMove(att->vsb,size.width-sbsize,0);
    GGadgetResize(att->hsb,size.width-sbsize,sbsize);
    GGadgetMove(att->hsb,0,lcnt*att->fh);
    GDrawResize(att->v,size.width-sbsize,lcnt*att->fh);
    att->page_width = size.width-sbsize;
    att->lines_page = lcnt;
    GScrollBarSetBounds(att->vsb,0,att->open_cnt,att->lines_page);
    GScrollBarSetBounds(att->hsb,0,att->maxl,att->page_width);

    GGadgetGetSize(att->cancel,&wsize);
    GGadgetMove(att->cancel,(size.width-wsize.width)/2,lcnt*att->fh+sbsize+5);
    GDrawRequestExpose(att->v,NULL,true);
    GDrawRequestExpose(att->gw,NULL,true);
}

static int AttChar(struct att_dlg *att,GEvent *event) {
    int depth = 0;
    int pos;

    switch (event->u.chr.keysym) {
      case GK_F1: case GK_Help:
	help("showatt.html");
return( true );
      case GK_Return: case GK_KP_Enter:
	att->current->open = !att->current->open;

	att->open_cnt = SizeCnt(att,att->tables,0);
	GScrollBarSetBounds(att->vsb,0,att->open_cnt,att->lines_page);

	GDrawRequestExpose(att->v,NULL,false);
return( true );
      case GK_Page_Down: case GK_KP_Page_Down:
	pos = att->off_top+(att->lines_page<=1?1:att->lines_page-1);
	if ( pos >= att->open_cnt-att->lines_page )
	    pos = att->open_cnt-att->lines_page;
	if ( pos<0 ) pos = 0;
	att->off_top = pos;
	GScrollBarSetPos(att->vsb,pos);
	GDrawRequestExpose(att->v,NULL,false);
return( true );
      case GK_Down: case GK_KP_Down:
	ATTChangeCurrent(att,NodeNext(att->current,&depth));
return( true );
      case GK_Up: case GK_KP_Up:
	ATTChangeCurrent(att,NodePrev(att,att->current,&depth));
return( true );
      case GK_Page_Up: case GK_KP_Page_Up:
	pos = att->off_top-(att->lines_page<=1?1:att->lines_page-1);
	if ( pos<0 ) pos = 0;
	att->off_top = pos;
	GScrollBarSetPos(att->vsb,pos);
	GDrawRequestExpose(att->v,NULL,false);
return( true );
      case GK_Left: case GK_KP_Left:
	ATTChangeCurrent(att,att->current->parent);
return( true );
      case GK_Right: case GK_KP_Right:
	if ( !att->current->open ) {
	    att->current->open = !att->current->open;

	    att->open_cnt = SizeCnt(att,att->tables,0);
	    GScrollBarSetBounds(att->vsb,0,att->open_cnt,att->lines_page);
	    if ( att->current->children!=NULL )
		att->current = att->current->children;

	    GDrawRequestExpose(att->v,NULL,false);
	} else
	    ATTChangeCurrent(att,att->current->children);
return( true );
      case GK_Home: case GK_KP_Home:
	ATTChangeCurrent(att,att->tables);
return( true );
      case GK_End: case GK_KP_End:
	ATTChangeCurrent(att,NodeFindLPos(att->tables,att->open_cnt-1,&depth));
return( true );
      case 'S': case 's':
	if ( event->u.mouse.state&ksm_control )
	    AttSave(att);
return( true );
    }
return( false );
}

static int attv_e_h(GWindow gw, GEvent *event) {
    struct att_dlg *att = (struct att_dlg *) GDrawGetUserData(gw);

    if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button>=4 && event->u.mouse.button<=7) ) {
return( GGadgetDispatchEvent(att->vsb,event));
    }

    switch ( event->type ) {
      case et_expose:
	AttExpose(att,gw,&event->u.expose.rect);
      break;
      case et_char:
return( AttChar(att,event));
      case et_mousedown:
      case et_mousemove:
      case et_mouseup:
	AttMouse(att,event);
      break;
    }
return( true );
}

static int att_e_h(GWindow gw, GEvent *event) {
    struct att_dlg *att = (struct att_dlg *) GDrawGetUserData(gw);

    if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button>=4 && event->u.mouse.button<=7) ) {
return( GGadgetDispatchEvent(att->vsb,event));
    }

    switch ( event->type ) {
      case et_expose:
      break;
      case et_char:
return( AttChar(att,event));
      break;
      case et_resize:
	if ( event->u.resize.sized )
	    AttResize(att,event);
      break;
      case et_controlevent:
	switch ( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    if ( event->u.control.g == att->vsb )
		AttScroll(att,&event->u.control.u.sb);
	    else
		AttHScroll(att,&event->u.control.u.sb);
	  break;
	  case et_buttonactivate:
	    att->done = true;
	    if ( att->dlg_type==dt_font_comp )
		GDrawDestroyWindow(gw);
	  break;
	}
      break;
      case et_close:
	att->done = true;
	if ( att->dlg_type==dt_font_comp )
	    GDrawDestroyWindow(gw);
      break;
      case et_destroy:
	if ( att!=NULL ) {
	    nodesfree(att->tables);
	    free(att);
	}
    }
return( true );
}

static void ShowAttCreateDlg(struct att_dlg *att, SplineFont *sf, int which,
	char *win_title) {
    GRect pos;
    GWindowAttrs wattrs;
    FontRequest rq;
    int as, ds, ld;
    GGadgetCreateData gcd[5];
    GTextInfo label[4];
    int sbsize = GDrawPointsToPixels(NULL,_GScrollBar_Width);
    static GFont *monofont=NULL, *propfont=NULL;

    if ( sf->cidmaster ) sf = sf->cidmaster;

    memset( att,0,sizeof(*att));
    att->sf = sf;
    att->dlg_type = which;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = which==dt_show_att;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = win_title;
    pos.x = pos.y = 0;
    pos.width = GDrawPointsToPixels(NULL,GGadgetScale(which==0?200:450));
    pos.height = GDrawPointsToPixels(NULL,300);
    att->gw = GDrawCreateTopWindow(NULL,&pos,att_e_h,att,&wattrs);

    if ( propfont==NULL ) {
	memset(&rq,'\0',sizeof(rq));
	rq.utf8_family_name = SANS_UI_FAMILIES;
	rq.point_size = 12;
	rq.weight = 400;
	propfont = GDrawInstanciateFont(att->gw,&rq);
	propfont = GResourceFindFont("ShowATT.Font",propfont);

	GDrawDecomposeFont(propfont, &rq);
	rq.utf8_family_name = MONO_UI_FAMILIES;	/* I want to show tabluar data sometimes */
	monofont = GDrawInstanciateFont(att->gw,&rq);
	monofont = GResourceFindFont("ShowATT.MonoFont",monofont);
    }
    att->font = propfont;
    att->monofont = monofont;
    GDrawWindowFontMetrics(att->gw,att->font,&as,&ds,&ld);
    att->fh = as+ds; att->as = as;

    att->bmargin = GDrawPointsToPixels(NULL,32)+sbsize;

    att->lines_page = (pos.height-att->bmargin)/att->fh;
    att->page_width = pos.width-sbsize;
    wattrs.mask = wam_events|wam_cursor/*|wam_bordwidth|wam_bordcol*/;
    wattrs.border_width = 1;
    wattrs.border_color = 0x000000;
    pos.x = 0; pos.y = 0;
    pos.width -= sbsize; pos.height = att->lines_page*att->fh;
    att->v = GWidgetCreateSubWindow(att->gw,&pos,attv_e_h,att,&wattrs);
    GDrawSetVisible(att->v,true);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    gcd[0].gd.pos.x = pos.width; gcd[0].gd.pos.y = 0;
    gcd[0].gd.pos.width = sbsize;
    gcd[0].gd.pos.height = pos.height;
    gcd[0].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels | gg_sb_vert;
    gcd[0].creator = GScrollBarCreate;

    gcd[1].gd.pos.x = 0; gcd[1].gd.pos.y = pos.height;
    gcd[1].gd.pos.height = sbsize;
    gcd[1].gd.pos.width = pos.width;
    gcd[1].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels;
    gcd[1].creator = GScrollBarCreate;

    gcd[2].gd.pos.width = -1;
    gcd[2].gd.pos.x = (pos.width-sbsize-GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor))/2;
    gcd[2].gd.pos.y = pos.height+sbsize+5;
    gcd[2].gd.flags = gg_visible | gg_enabled | gg_but_default | gg_but_cancel | gg_pos_in_pixels;
    label[2].text = (unichar_t *) _("_OK");
    label[2].text_is_1byte = true;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].creator = GButtonCreate;

    GGadgetsCreate(att->gw,gcd);
    att->vsb = gcd[0].ret;
    att->hsb = gcd[1].ret;
    att->cancel = gcd[2].ret;
}

void ShowAtt(SplineFont *sf,int def_layer) {
    struct att_dlg att;

    ShowAttCreateDlg(&att, sf, 0, _("Show ATT"));
    att.def_layer = def_layer;

    BuildTop(&att);
    att.open_cnt = SizeCnt(&att,att.tables,0);
    GScrollBarSetBounds(att.vsb,0,att.open_cnt,att.lines_page);

    GDrawSetVisible(att.gw,true);

    while ( !att.done )
	GDrawProcessOneEvent(NULL);
    GDrawSetUserData(att.gw,NULL);
    nodesfree(att.tables);
    GDrawDestroyWindow(att.gw);
}

/* ************************************************************************** */
/* ****************************** Font Compare ****************************** */
/* ************************ (reuse the show att dlg) ************************ */
/* ************************************************************************** */
struct nested_file {
    FILE *file;
    char *linebuf;
    int linemax;
    int read_nest;
};

static int ReadNestedLine(struct nested_file *nf) {
    char *pt, *end;
    int ch;
    int nest = 0;

    pt = nf->linebuf; end = pt + nf->linemax-1;
    while ( (ch=getc(nf->file))==' ' )
	++nest;
    if ( ch==EOF ) {
	nf->read_nest = -1;
return( -1 );
    }
    while ( ch!=EOF && ch!='\n' ) {
	if ( pt>=end ) {
	    char *old = nf->linebuf;
	    nf->linemax += 200;
	    nf->linebuf = realloc(nf->linebuf, nf->linemax);
	    pt = nf->linebuf + (pt-old);
	    end = nf->linebuf + nf->linemax - 1;
	}
	*pt++ = ch;
	ch = getc(nf->file);
    }
    *pt = '\0';
    nf->read_nest = nest;
return( nest );
}

static void ReadKids(struct nested_file *nf,int desired_nest,struct node *parent) {
    int i=0, max=0, j, k;

    ReadNestedLine(nf);
    for (;;) {
	if ( nf->read_nest < desired_nest )
    break;
	if ( i>=max-1 ) {
	    parent->children = realloc(parent->children,(max+=10)*sizeof( struct node ));
	    memset(parent->children+i,0,(max-i)*sizeof(struct node));
	}
	parent->children[i].label = copy(nf->linebuf);
	parent->children[i].parent = parent;
	ReadKids(nf,desired_nest+1,&parent->children[i]);
	++i;
    }
    if ( i!=0 ) {
	if ( i<max-1 )
	    parent->children = realloc(parent->children,(i+1)*sizeof(struct node));
	/* The reallocs can invalidate the parent field */
	for ( j=0; j<i; ++j )
	    for ( k=0; k<parent->children[j].cnt; ++k )
		parent->children[j].children[k].parent = &parent->children[j];
	parent->cnt = i;
    }
}
    
static void BuildFCmpNodes(struct att_dlg *att, SplineFont *sf1, SplineFont *sf2,int flags) {
    FILE *tmp = GFileTmpfile();
    struct node *tables;
    struct nested_file nf;

    att->tables = tables = calloc(2,sizeof(struct node));
    att->current = tables;
    if ( !CompareFonts(sf1,att->fv1->b.map,sf2,tmp,flags) && ftell(tmp)==0 ) {
	tables[0].label = copy(_("No differences found"));
    } else {
	tables[0].label = copy(_("Differences..."));
	rewind(tmp);
	memset(&nf,0,sizeof(nf));
	nf.file = tmp;
	nf.linebuf = malloc( nf.linemax = 300 );
	ReadKids(&nf,0,&tables[0]);
	free(nf.linebuf);
    }
    fclose(tmp);
}

static void FontCmpDlg(FontView *fv1, FontView *fv2,int flags) {
    struct att_dlg *att;
    char buffer[300];
    SplineFont *sf1 = fv1->b.sf, *sf2=fv2->b.sf;

    if ( strcmp(sf1->fontname,sf2->fontname)!=0 )
	snprintf( buffer, sizeof(buffer), _("Compare %s to %s"), sf1->fontname, sf2->fontname);
    else if ( sf1->version!=NULL && sf2->version!=NULL &&
	    strcmp(sf1->version,sf2->version)!=0 )
	snprintf( buffer, sizeof(buffer), _("Compare version %s of %s to %s"),
		sf1->version, sf1->fontname, sf2->version);
    else
	strcpy( buffer, _("Font Compare")); 

    att = malloc(sizeof(struct att_dlg));
    ShowAttCreateDlg(att, sf1, dt_font_comp, buffer);
    att->fv1 = fv1; att->fv2 = fv2;

    GDrawSetCursor(fv1->v,ct_watch);
    GDrawSetCursor(fv2->v,ct_watch);
    BuildFCmpNodes(att,sf1,sf2,flags);
    GDrawSetCursor(fv1->v,ct_pointer);
    GDrawSetCursor(fv2->v,ct_pointer);
    
    att->open_cnt = SizeCnt(att,att->tables,0);
    GScrollBarSetBounds(att->vsb,0,att->open_cnt,att->lines_page);

    GDrawSetVisible(att->gw,true);
}

static void FCAskFilename(FontView *fv,int flags) {
    char *filename = GetPostScriptFontName(NULL,false);
    FontView *otherfv;

    if ( filename==NULL )
return;
    otherfv = (FontView *) ViewPostScriptFont(filename,0);
    free(filename); filename = NULL;
    if ( otherfv==NULL )
return;
    FontCmpDlg(fv,otherfv,flags);
}

struct mf_data {
    int done;
    FontView *fv;
    GGadget *other;
    GGadget *amount;
};
#define CID_Outlines	1
#define	CID_Exact	2
#define CID_Warn	3
#define CID_Fuzzy	4
#define CID_Hinting	5
#define CID_Bitmaps	6
#define CID_Names	7
#define CID_GPos	8
#define CID_GSub	9
#define CID_HintMasks	10
#define CID_HintMasksWConflicts	11
#define CID_NoHintMasks	12
#define CID_RefContourWarn	13
#define CID_AddDiffs	14
#define CID_AddMissing	15

static int last_flags = fcf_outlines|fcf_hinting|fcf_bitmaps|fcf_names|fcf_gpos|fcf_gsub|fcf_adddiff2sf1;

static int FC_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	struct mf_data *d = GDrawGetUserData(gw);
	int i, index = GGadgetGetFirstListSelectedItem(d->other);
	FontView *fv;
	int flags = 0;
	for ( i=0, fv=fv_list; fv!=NULL; fv=(FontView *) (fv->b.next) ) {
	    if ( fv==d->fv )
	continue;
	    if ( i==index )
	break;
	    ++i;
	}

	if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_Outlines)) )
	    flags |= fcf_outlines;
	if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_Exact)) )
	    flags |= fcf_exact;
	else if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_Warn)) )
	    flags |= fcf_warn_not_exact;
	if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_RefContourWarn)) )
	    flags |= fcf_warn_not_ref_exact;
	if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_Hinting)) )
	    flags |= fcf_hinting;
	if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_HintMasks)) )
	    flags |= fcf_hintmasks;
	if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_HintMasksWConflicts)) )
	    flags |= fcf_hmonlywithconflicts|fcf_hintmasks;
	if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_Bitmaps)) )
	    flags |= fcf_bitmaps;
	if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_Names)) )
	    flags |= fcf_names;
	if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_GPos)) )
	    flags |= fcf_gpos;
	if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_GSub)) )
	    flags |= fcf_gsub;
	if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_AddDiffs)) )
	    flags |= fcf_adddiff2sf1;
	if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_AddMissing)) )
	    flags |= fcf_addmissing;
	last_flags = flags;

	GDrawDestroyWindow(gw);
	if ( fv==NULL )
	    FCAskFilename(d->fv,flags);
	else
	    FontCmpDlg(d->fv,fv,flags);
	d->done = true;
    }
return( true );
}

static int FC_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	struct mf_data *d = GDrawGetUserData(gw);
	d->done = true;
	GDrawDestroyWindow(gw);
    }
return( true );
}

static int fc_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct mf_data *d = GDrawGetUserData(gw);
	d->done = true;
	GDrawDestroyWindow(gw);
    } else if ( event->type == et_char ) {
return( false );
    }
return( true );
}

void FontCompareDlg(FontView *fv) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[25];
    GTextInfo label[25];
    struct mf_data d;
    char buffer[80];
    int k;

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict|wam_isdlg;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.is_dlg = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title = _("Font Compare");
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,180));
	pos.height = GDrawPointsToPixels(NULL,88+15*14+2);
	gw = GDrawCreateTopWindow(NULL,&pos,fc_e_h,&d,&wattrs);

	memset(&label,0,sizeof(label));
	memset(&gcd,0,sizeof(gcd));

	k=0;
	sprintf( buffer, _("Font to compare with %.20s"), fv->b.sf->fontname );
	label[k].text = (unichar_t *) buffer;
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 12; gcd[k].gd.pos.y = 6; 
	gcd[k].gd.flags = gg_visible | gg_enabled;
	gcd[k++].creator = GLabelCreate;

	gcd[k].gd.pos.x = 15; gcd[k].gd.pos.y = 21;
	gcd[k].gd.pos.width = 145;
	gcd[k].gd.flags = gg_visible | gg_enabled;
	gcd[k].gd.u.list = BuildFontList(fv);
	gcd[k].gd.label = &gcd[k].gd.u.list[0];
	gcd[k].gd.u.list[0].selected = true;
	gcd[k++].creator = GListButtonCreate;

	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+24;
	if ( fv->b.sf->onlybitmaps )
	    gcd[k].gd.flags = gg_visible;
	else
	    gcd[k].gd.flags = gg_visible | gg_enabled | ((last_flags&fcf_outlines)?gg_cb_on:0);
	label[k].text = (unichar_t *) _("Compare _Outlines");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.cid = CID_Outlines;
	gcd[k++].creator = GCheckBoxCreate;

	gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
	if ( fv->b.sf->onlybitmaps )
	    gcd[k].gd.flags = gg_visible | gg_utf8_popup;
	else
	    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup | ((last_flags&fcf_exact)?gg_cb_on:0);
	label[k].text = (unichar_t *) _("_Exact");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.cid = CID_Exact;
	gcd[k].gd.popup_msg = (unichar_t *) _("Accept outlines which exactly match the original");
	gcd[k++].creator = GRadioCreate;

	gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
	if ( fv->b.sf->onlybitmaps )
	    gcd[k].gd.flags = gg_visible | gg_utf8_popup;
	else
	    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup | ((last_flags&fcf_exact)?0:gg_cb_on);
	label[k].text = (unichar_t *) _("_Accept inexact");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.cid = CID_Fuzzy;
	gcd[k].gd.popup_msg = (unichar_t *) _("Accept an outline which is a close approximation to the original.\nIt may be off by an em-unit, or have a reference which matches a contour.");
	gcd[k++].creator = GRadioCreate;

	gcd[k].gd.pos.x = 15; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
	if ( fv->b.sf->onlybitmaps )
	    gcd[k].gd.flags = gg_visible | gg_utf8_popup;
	else
	    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup | ((last_flags&fcf_warn_not_exact)?gg_cb_on:0);
	label[k].text = (unichar_t *) _("_Warn if inexact");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.cid = CID_Warn;
	gcd[k].gd.popup_msg = (unichar_t *) _("Warn if the outlines are close but not exactly the same");
	gcd[k++].creator = GCheckBoxCreate;

	gcd[k].gd.pos.x = 15; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
	if ( fv->b.sf->onlybitmaps )
	    gcd[k].gd.flags = gg_visible | gg_utf8_popup;
	else
	    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup | ((last_flags&fcf_warn_not_ref_exact)?gg_cb_on:0);
	label[k].text = (unichar_t *) _("Warn if _unlinked references");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.cid = CID_RefContourWarn;
	gcd[k].gd.popup_msg = (unichar_t *) _("Warn if one glyph contains an outline while the other contains a reference (but the reference describes the same outline)");
	gcd[k++].creator = GCheckBoxCreate;

	gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
	if ( fv->b.sf->onlybitmaps )
	    gcd[k].gd.flags = gg_visible | gg_utf8_popup;
	else
	    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup | ((last_flags&fcf_hinting)?gg_cb_on:0);
	label[k].text = (unichar_t *) _("Compare _Hints");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.cid = CID_Hinting;
	gcd[k].gd.popup_msg = (unichar_t *) _("Compare postscript hints and hintmasks and truetype instructions");
	gcd[k++].creator = GCheckBoxCreate;

	gcd[k].gd.pos.x = 15; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
	if ( fv->b.sf->onlybitmaps )
	    gcd[k].gd.flags = gg_visible | gg_utf8_popup;
	else
	    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup | (((last_flags&fcf_hintmasks) && !(last_flags&fcf_hmonlywithconflicts))?gg_cb_on:0);
	label[k].text = (unichar_t *) _("Compare Hint_Masks");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.cid = CID_HintMasks;
	gcd[k].gd.popup_msg = (unichar_t *) _("Compare hintmasks");
	gcd[k++].creator = GRadioCreate;

	gcd[k].gd.pos.x = 15; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
	if ( fv->b.sf->onlybitmaps )
	    gcd[k].gd.flags = gg_visible | gg_utf8_popup;
	else
	    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup | ((last_flags&fcf_hmonlywithconflicts)?gg_cb_on:0);
	label[k].text = (unichar_t *) _("HintMasks only if conflicts");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.cid = CID_HintMasksWConflicts;
	gcd[k].gd.popup_msg = (unichar_t *) _("Don't compare hintmasks if the glyph has no hint conflicts");
	gcd[k++].creator = GRadioCreate;

	gcd[k].gd.pos.x = 15; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
	if ( fv->b.sf->onlybitmaps )
	    gcd[k].gd.flags = gg_visible ;
	else
	    gcd[k].gd.flags = gg_visible | gg_enabled | ((last_flags&fcf_hintmasks)?0:gg_cb_on);
	label[k].text = (unichar_t *) _("Don't Compare HintMasks");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.cid = CID_NoHintMasks;
	gcd[k++].creator = GRadioCreate;

	gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
	if ( fv->b.sf->onlybitmaps )
	    gcd[k].gd.flags = gg_visible | gg_utf8_popup;
	else
	    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup | ((last_flags&fcf_adddiff2sf1)?gg_cb_on:0);
	label[k].text = (unichar_t *) _("_Add Diff Outlines to Background");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.cid = CID_AddDiffs;
	gcd[k].gd.popup_msg = (unichar_t *) _("If two glyphs differ, then add the outlines of the second glyph\nto the background layer of the first (So when opening the first\nthe differences will be visible).");
	gcd[k++].creator = GCheckBoxCreate;

	gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
	if ( fv->b.sf->onlybitmaps )
	    gcd[k].gd.flags = gg_visible | gg_utf8_popup;
	else
	    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup | ((last_flags&fcf_addmissing)?gg_cb_on:0);
	label[k].text = (unichar_t *) _("Add _Missing Glyphs");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.cid = CID_AddMissing;
	gcd[k].gd.popup_msg = (unichar_t *) _("If a glyph in the second font is missing from the first, then\nadd it to the first with the outlines of the second font in\nthe background");
	gcd[k++].creator = GCheckBoxCreate;

	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+16;
	if ( fv->b.sf->bitmaps==NULL )
	    gcd[k].gd.flags = gg_visible;
	else
	    gcd[k].gd.flags = gg_visible | gg_enabled | ((last_flags&fcf_bitmaps)?gg_cb_on:0);
	label[k].text = (unichar_t *) _("Compare _Bitmaps");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.cid = CID_Bitmaps;
	gcd[k++].creator = GCheckBoxCreate;

	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
	gcd[k].gd.flags = gg_visible | gg_enabled | ((last_flags&fcf_names)?gg_cb_on:0);
	label[k].text = (unichar_t *) _("Compare _Names");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.cid = CID_Names;
	gcd[k++].creator = GCheckBoxCreate;

	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
	gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup | ((last_flags&fcf_gpos)?gg_cb_on:0);
	label[k].text = (unichar_t *) _("Compare Glyph _Positioning");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.cid = CID_GPos;
	gcd[k].gd.popup_msg = (unichar_t *) _("Kerning & such");
	gcd[k++].creator = GCheckBoxCreate;

	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
	gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup | ((last_flags&fcf_gsub)?gg_cb_on:0);
	label[k].text = (unichar_t *) _("Compare Glyph _Substitution");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.cid = CID_GSub;
	gcd[k].gd.popup_msg = (unichar_t *) _("Ligatures & such");
	gcd[k++].creator = GCheckBoxCreate;

	gcd[k].gd.pos.x = 15-3; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+23-3;
	gcd[k].gd.pos.width = -1; gcd[k].gd.pos.height = 0;
	gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[k].text = (unichar_t *) _("_OK");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.handle_controlevent = FC_OK;
	gcd[k++].creator = GButtonCreate;

	gcd[k].gd.pos.x = -15; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+3;
	gcd[k].gd.pos.width = -1; gcd[k].gd.pos.height = 0;
	gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[k].text = (unichar_t *) _("_Cancel");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.handle_controlevent = FC_Cancel;
	gcd[k++].creator = GButtonCreate;

	gcd[k].gd.pos.x = 2; gcd[k].gd.pos.y = 2;
	gcd[k].gd.pos.width = pos.width-4; gcd[k].gd.pos.height = pos.height-2;
	gcd[k].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
	gcd[k++].creator = GGroupCreate;

	GGadgetsCreate(gw,gcd);

	memset(&d,'\0',sizeof(d));
	d.other = gcd[1].ret;
	d.fv = fv;

	GWidgetHidePalettes();
	GDrawSetVisible(gw,true);
	while ( !d.done )
	    GDrawProcessOneEvent(NULL);
	TFFree(gcd[1].gd.u.list);
}
