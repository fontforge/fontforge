/* Copyright (C) 2003-2005 by George Williams */
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
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
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
    uint16 cnt;
    struct node *children, *parent;
    void (*build)(struct node *,struct att_dlg *);
    unichar_t *label;
    uint32 tag;
    union sak {
	SplineChar *sc;
	AnchorClass *ac;
	KernClass *kc;
	FPST *fpst;
	ASM *sm;
	int index;
    } u;
    int lpos;
};

struct att_dlg {
    unsigned int done: 1;
    struct node *tables;
    int open_cnt, lines_page, off_top, off_left, page_width, bmargin;
    int maxl;
    SplineFont *sf;
    GWindow gw,v;
    GGadget *vsb, *hsb, *cancel;
    int fh, as;
    GFont *font, *monofont;
    struct node *current;
};

static void BuildFPST(struct node *node,struct att_dlg *att);
static void BuildAnchorLists_(struct node *node,struct att_dlg *att);

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

static int compare_tag(const void *_n1, const void *_n2) {
    const struct node *n1 = _n1, *n2 = _n2;
return( n1->tag-n2->tag );
}

static void BuildMarkedLigatures(struct node *node,struct att_dlg *att) {
    SplineChar *sc = node->u.sc;
    AnchorClass *ac = node->parent->parent->u.ac, *ac2;
    int classcnt, i, j, max, k;
    AnchorPoint *ap;
    unichar_t ubuf[60];

    for ( ac2=ac, classcnt=0; ac2!=NULL &&
	    ac2->feature_tag==ac->feature_tag &&
	    ac2->script_lang_index == ac->script_lang_index &&
	    ac2->merge_with == ac->merge_with ; ac2 = ac2->next )
	++classcnt;
    max=0;
    for ( ap=sc->anchor; ap!=NULL ; ap=ap->next )
	if ( ap->lig_index>max )
	    max = ap->lig_index;
    node->children = gcalloc((classcnt*(max+1))+1,sizeof(struct node));
    for ( k=j=0; k<=max; ++k ) {
	for ( i=0, ac2=ac; i<classcnt; ++i, ac2=ac2->next ) {
	    for ( ap=sc->anchor; ap!=NULL && (ap->type!=at_baselig || ap->anchor!=ac2 || ap->lig_index!=k); ap=ap->next );
	    if ( ap!=NULL ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
		u_sprintf(ubuf,GStringGetResource(_STR_MarkLigComponentNamePos,NULL),
#elif defined(FONTFORGE_CONFIG_GTK)
		u_sprintf(ubuf,_("Component %d %.30s (%d,%d)"),
#endif
			k, ac2->name, (int) ap->me.x, (int) ap->me.y );
		node->children[j].label = u_copy(ubuf);
		node->children[j++].parent = node;
	    }
	}
    }
    node->cnt = j;
}

static void BuildMarkedChars(struct node *node,struct att_dlg *att) {
    SplineChar *sc = node->u.sc;
    AnchorClass *ac = node->parent->parent->u.ac, *ac2;
    int classcnt, i, j;
    AnchorPoint *ap;
    unichar_t ubuf[60];

    for ( ac2=ac, classcnt=0; ac2!=NULL &&
	    ac2->feature_tag==ac->feature_tag &&
	    ac2->script_lang_index == ac->script_lang_index &&
	    ac2->merge_with == ac->merge_with ; ac2 = ac2->next )
	++classcnt;
    node->children = gcalloc(classcnt+1,sizeof(struct node));
    for ( i=j=0, ac2=ac; i<classcnt; ++i, ac2=ac2->next ) {
	for ( ap=sc->anchor; ap!=NULL && (!(ap->type==at_basechar || ap->type==at_basemark) || ap->anchor!=ac2); ap=ap->next );
	if ( ap!=NULL ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	    u_sprintf(ubuf,GStringGetResource(_STR_MarkAnchorNamePos,NULL), ac2->name,
#elif defined(FONTFORGE_CONFIG_GTK)
	    u_sprintf(ubuf,_("%.30s (%d,%d)"), ac2->name,
#endif
		    (int) ap->me.x, (int) ap->me.y );
	    node->children[j].label = u_copy(ubuf);
	    node->children[j++].parent = node;
	}
    }
    node->cnt = j;
}

static void BuildBase(struct node *node,SplineChar **bases,enum anchor_type at, struct node *parent) {
    int i;

    node->parent = parent;
#if defined(FONTFORGE_CONFIG_GDRAW)
    node->label = u_copy(GStringGetResource(at==at_basechar?_STR_BaseGlyphs:
#elif defined(FONTFORGE_CONFIG_GTK)
    node->label = u_copy(at==at_basechar?_("Base Glyphs")
#endif
					    at==at_baselig?_STR_BaseLigatures:
			                    _STR_BaseMarks,NULL));
    for ( i=0; bases[i]!=NULL; ++i );
    if ( i==0 ) {
	node->cnt = 1;
	node->children = gcalloc(2,sizeof(struct node));
#if defined(FONTFORGE_CONFIG_GDRAW)
	node->children[0].label = u_copy(GStringGetResource(_STR_Empty,NULL));
#elif defined(FONTFORGE_CONFIG_GTK)
	node->children[0].label = u_copy(_("Empty"));
#endif
	node->children[0].parent = node;
    } else {
	node->cnt = i;
	node->children = gcalloc(i+1,sizeof(struct node));
	for ( i=0; bases[i]!=NULL; ++i ) {
	    node->children[i].label = uc_copy(bases[i]->name);
	    node->children[i].parent = node;
	    node->children[i].u.sc = bases[i];
	    node->children[i].build = at==at_baselig?BuildMarkedLigatures:BuildMarkedChars;
	}
    }
}

static void BuildMark(struct node *node,SplineChar **marks,AnchorClass *ac, struct node *parent) {
    int i;
    unichar_t ubuf[60];
    AnchorPoint *ap;

    node->parent = parent;
#if defined(FONTFORGE_CONFIG_GDRAW)
    u_sprintf(ubuf,GStringGetResource(_STR_MarkClassS,NULL),ac->name);
#elif defined(FONTFORGE_CONFIG_GTK)
    u_sprintf(ubuf,_("Mark Class %.20s"),ac->name);
#endif
    node->label = u_copy(ubuf);
    for ( i=0; marks[i]!=NULL; ++i );
    if ( i==0 ) {
	node->cnt = 1;
	node->children = gcalloc(2,sizeof(struct node));
#if defined(FONTFORGE_CONFIG_GDRAW)
	node->children[0].label = u_copy(GStringGetResource(_STR_Empty,NULL));
#elif defined(FONTFORGE_CONFIG_GTK)
	node->children[0].label = u_copy(_("Empty"));
#endif
	node->children[0].parent = node;
    } else {
	node->cnt = i;
	node->children = gcalloc(i+1,sizeof(struct node));
	for ( i=0; marks[i]!=NULL; ++i ) {
	    for ( ap=marks[i]->anchor; ap!=NULL && (ap->type!=at_mark || ap->anchor!=ac); ap=ap->next );
#if defined(FONTFORGE_CONFIG_GDRAW)
	    u_sprintf(ubuf,GStringGetResource(_STR_MarkCharNamePos,NULL), marks[i]->name,
#elif defined(FONTFORGE_CONFIG_GTK)
	    u_sprintf(ubuf,_("%.30s (%d,%d)"), marks[i]->name,
#endif
		    (int) ap->me.x, (int) ap->me.y );
	    node->children[i].label = u_copy(ubuf);
	    node->children[i].parent = node;
	}
    }
}

static void BuildAnchorLists(struct node *node,struct att_dlg *att,uint32 script) {
    AnchorClass *ac = node->u.ac, *ac2;
    int cnt, i, j, classcnt;
    AnchorPoint *ap, *ent, *ext;
    SplineChar **base, **lig, **mkmk, **entryexit;
    SplineChar ***marks;
    int *subcnts;
    SplineFont *sf = att->sf;
    unichar_t ubuf[60];

    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;

    if ( ac->type==act_curs ) {
	entryexit = EntryExitDecompose(sf,ac);
	if ( entryexit==NULL ) {
	    node->children = gcalloc(2,sizeof(struct node));
	    node->cnt = 1;
#if defined(FONTFORGE_CONFIG_GDRAW)
	    node->children[0].label = u_copy(GStringGetResource(_STR_Empty,NULL));
#elif defined(FONTFORGE_CONFIG_GTK)
	    node->children[0].label = u_copy(_("Empty"));
#endif
	    node->children[0].parent = node;
	} else {
	    for ( cnt=0; entryexit[cnt]!=NULL; ++cnt );
	    node->children = gcalloc(cnt+1,sizeof(struct node));
	    for ( cnt=0; entryexit[cnt]!=NULL; ++cnt ) {
		node->children[cnt].u.sc = entryexit[cnt];
		node->children[cnt].label = uc_copy(entryexit[cnt]->name);
		node->children[cnt].parent = node;
		for ( ent=ext=NULL, ap=entryexit[cnt]->anchor; ap!=NULL; ap=ap->next ) {
		    if ( ap->anchor==ac ) {
			if ( ap->type == at_centry )
			    ent = ap;
			else if ( ap->type == at_cexit )
			    ent = ap;
		    }
		}
		node->children[cnt].cnt = 1+(ent!=NULL)+(ext!=NULL);
		node->children[cnt].children = gcalloc((1+(ent!=NULL)+(ext!=NULL)),sizeof(struct node));
		i = 0;
		if ( ent!=NULL ) {
		    u_snprintf(ubuf,sizeof(ubuf)/sizeof(ubuf[0]),
#if defined(FONTFORGE_CONFIG_GDRAW)
			    GStringGetResource(_STR_Entry,NULL),
#elif defined(FONTFORGE_CONFIG_GTK)
			    _("Entry (%d,%d)"),
#endif
			    ent->me.x, ent->me.y);
		    node->children[cnt].children[i].label = u_copy(ubuf);
		    node->children[cnt].children[i].parent = &node->children[cnt];
		    ++i;
		}
		if ( ext!=NULL ) {
		    u_snprintf(ubuf,sizeof(ubuf)/sizeof(ubuf[0]),
#if defined(FONTFORGE_CONFIG_GDRAW)
			    GStringGetResource(_STR_Exit,NULL),
#elif defined(FONTFORGE_CONFIG_GTK)
			    _("Exit (%d,%d)"),
#endif
			    ext->me.x, ext->me.y);
		    node->children[cnt].children[i].label = u_copy(ubuf);
		    node->children[cnt].children[i].parent = &node->children[cnt];
		    ++i;
		}
	    }
	}
	free(entryexit);
    } else {
	for ( ac2=ac, classcnt=0; ac2!=NULL ; ac2=ac2->next ) {
	    if ( ac2->feature_tag==ac->feature_tag &&
		    ac2->script_lang_index == ac->script_lang_index &&
		    ac2->merge_with == ac->merge_with ) {
		++classcnt;
		ac2->matches = true;
	    } else
		ac2->matches = false;
	}

	marks = galloc(classcnt*sizeof(SplineChar **));
	subcnts = galloc(classcnt*sizeof(int));
	AnchorClassDecompose(sf,ac,classcnt,subcnts,marks,&base,&lig,&mkmk);
	node->cnt = classcnt+(base!=NULL)+(lig!=NULL)+(mkmk!=NULL);
	node->children = gcalloc(node->cnt+1,sizeof(struct node));
	i=0;
	if ( base!=NULL )
	    BuildBase(&node->children[i++],base,at_basechar,node);
	if ( lig!=NULL )
	    BuildBase(&node->children[i++],lig,at_baselig,node);
	if ( mkmk!=NULL )
	    BuildBase(&node->children[i++],mkmk,at_basemark,node);
	for ( j=0, ac2=ac; j<classcnt; ++j, ac2=ac2->next ) if ( marks[j]!=NULL )
	    BuildMark(&node->children[i++],marks[j],ac2,node);
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
    KernClass *kc = node->parent->u.kc;
    struct node *seconds;
    int index=node->u.index,i,cnt, len;
    char buf[32];
    unichar_t *name;

    for ( i=1,cnt=0; i<kc->second_cnt; ++i )
	if ( kc->offsets[index*kc->second_cnt+i]!=0 && strlen(kc->seconds[i])!=0 )
	    ++cnt;

    node->children = seconds = gcalloc(cnt+1,sizeof(struct node));
    node->cnt = cnt;
    cnt = 0;
    for ( i=1; i<kc->second_cnt; ++i ) if ( kc->offsets[index*kc->second_cnt+i]!=0 && strlen(kc->seconds[i])!=0 ) {
	sprintf( buf, "%d ", kc->offsets[index*kc->second_cnt+i]);
	len = strlen(buf)+strlen(kc->seconds[i])+1;
	name = galloc(len*sizeof(unichar_t));
	uc_strcpy(name,buf);
	uc_strcat(name,kc->seconds[i]);
	seconds[cnt].label = name;
	seconds[cnt].parent = node;
	seconds[cnt].build = NULL;
	seconds[cnt++].u.index = i;
    }
}

static void BuildKC(struct node *node,struct att_dlg *att) {
    KernClass *kc = node->u.kc;
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

    node->children = firsts = gcalloc(cnt+1,sizeof(struct node));
    node->cnt = cnt;
    for ( i=1,cnt=0; i<kc->first_cnt; ++i ) {
	for ( j=1,cnt2=0 ; j<kc->second_cnt; ++j ) {
	    if ( kc->offsets[i*kc->second_cnt+j]!=0 )
		++cnt2;
	}
	if ( cnt2==0 || strlen(kc->firsts[i])==0 )
    continue;
	firsts[cnt].label = uc_copy(kc->firsts[i]);
	firsts[cnt].parent = node;
	firsts[cnt].build = BuildKC2;
	firsts[cnt++].u.index = i;
    }
}

static int SLIMatches(SplineFont *sf, int sli, uint32 script, uint32 lang) {
    struct script_record *sr;
    int l,m;

    if ( sli==SLI_UNKNOWN )
return( false );
    if ( sli==SLI_NESTED )
return( script=='*' );
    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;
    else if ( sf->mm!=NULL ) sf = sf->mm->normal;

    sr = sf->script_lang[sli];
    for ( l=0; sr[l].script!=0 && sr[l].script!=script; ++l );
    if ( sr[l].script!=0 ) {
	for ( m=0; sr[l].langs[m]!=0 && sr[l].langs[m]!=lang; ++m );
	if ( sr[l].langs[m]!=0 )
return( true );
    }
return( false );
}

static void BuildKerns2(struct node *node,struct att_dlg *att,
	uint32 script, uint32 lang,int isv) {
    KernPair *kp = node->u.sc->kerns;
    int i,j;
    struct node *chars;
    char buf[80];
    SplineFont *_sf = att->sf;

    for ( j=0; j<2; ++j ) {
	for ( kp=isv ? node->u.sc->vkerns:node->u.sc->kerns, i=0; kp!=NULL; kp=kp->next ) {
	    if ( SLIMatches(_sf,kp->sli,script,lang) ) {
		if ( j ) {
		    sprintf( buf, "%.40s %d", kp->sc->name, kp->off );
		    chars[i].label = uc_copy(buf);
		    chars[i].parent = node;
		}
		++i;
	    }
	}
	if ( j==0 ) {
	    node->children = chars = gcalloc(i+1,sizeof(struct node));
	    node->cnt = i;
	}
    }
}

static void BuildKerns2G(struct node *node,struct att_dlg *att) {
    uint32 script = node->parent->parent->parent->tag, lang = node->parent->parent->tag;
    BuildKerns2(node,att,script,lang,false);
}

static void BuildKerns2GV(struct node *node,struct att_dlg *att) {
    uint32 script = node->parent->parent->parent->tag, lang = node->parent->parent->tag;
    BuildKerns2(node,att,script,lang,true);
}

static void BuildKerns2M(struct node *node,struct att_dlg *att) {
    uint32 script = node->parent->tag, lang = DEFAULT_LANG;
    BuildKerns2(node,att,script,lang,false);
}

static void BuildKerns2MV(struct node *node,struct att_dlg *att) {
    uint32 script = node->parent->tag, lang = DEFAULT_LANG;
    BuildKerns2(node,att,script,lang,true);
}

static void BuildKerns(struct node *node,struct att_dlg *att,uint32 script,uint32 lang,
	void (*build)(struct node *,struct att_dlg *),int isv) {
    int i,k,j, tot;
    SplineFont *_sf = att->sf, *sf;
    struct node *chars;
    KernPair *kp;

    for ( j=0; j<2; ++j ) {
	tot = 0;
	k=0;
	do {
	    sf = _sf->subfontcnt==0 ? _sf : _sf->subfonts[k++];
	    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
		for ( kp=isv ? sf->chars[i]->vkerns:sf->chars[i]->kerns; kp!=NULL; kp=kp->next ) {
		    if ( SLIMatches(_sf,kp->sli,script,lang) ) {
			if ( j ) {
			    chars[tot].u.sc = sf->chars[i];
			    chars[tot].label = uc_copy(sf->chars[i]->name);
			    chars[tot].build = build;
			    chars[tot].parent = node;
			}
			++tot;
	    break;
		    }
		}
	    }
	    ++k;
	} while ( k<_sf->subfontcnt );
	if ( tot==0 )
return;
	if ( !j ) {
	    node->children = chars = gcalloc(tot+1,sizeof(struct node));
	    node->cnt = tot;
	}
    }
}

static void BuildKernScript(struct node *node,struct att_dlg *att) {
    BuildKerns(node,att,node->tag,DEFAULT_LANG,BuildKerns2M,false);
}

static void BuildVKernScript(struct node *node,struct att_dlg *att) {
    BuildKerns(node,att,node->tag,DEFAULT_LANG,BuildKerns2MV,true);
}

static int PSTAllComponentsExist(SplineFont *sf,char *glyphnames ) {
    char *start, *end, ch;
    int ret;

    if ( glyphnames==NULL )
return( false );
    for ( start=glyphnames; *start; start = end ) {
	while ( *start==' ' ) ++start;
	for ( end = start; *end!=' ' && *end!='\0'; ++end );
	ch = *end; *end = '\0';
	ret = SCWorthOutputting(SFGetCharDup(sf,-1,start));
	*end = ch;
	if ( !ret )
return( false );
    }
return( true );
}

static void BuildFeatures(struct node *node,struct att_dlg *att,
	uint32 script, uint32 lang, uint32 tag, int ispos) {
    int i,j,k, maxc, tot, maxl, len;
    SplineFont *_sf = att->sf, *sf;
    SplineChar *sc;
    unichar_t *ubuf;
    char buf[200];
    struct node *chars;
    PST *pst;

    /* Build up the list of characters which use this GSUB/GPOS/morx feature */
    k=maxc=0;
    do {
	sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[k];
	if ( sf->charcnt>maxc ) maxc = sf->charcnt;
	++k;
    } while ( k<_sf->subfontcnt );

    chars = NULL; maxl = 0;
    for ( j=0; j<2; ++j ) {
	tot = 0;
	for ( i=0; i<maxc; ++i ) {
	    k=0;
	    sc = NULL;
	    do {
		sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[k];
		if ( i<sf->charcnt && sf->chars[i]!=NULL ) {
		    sc = sf->chars[i];
	    break;
		}
		++k;
	    } while ( k<_sf->subfontcnt );
	    if ( sc!=NULL ) {
		for ( pst=sc->possub; pst!=NULL; pst=pst->next )
			if ( pst->tag==tag && pst->type!=pst_lcaret &&
			 (( ispos && (pst->type==pst_position || pst->type==pst_pair)) ||
			  (!ispos && (pst->type!=pst_position && pst->type!=pst_pair))) ) {
		    if ( SLIMatches(sf,pst->script_lang_index,script,lang)) {
			if ( chars ) {
			    uc_strcpy(ubuf,sc->name);
			    if ( pst->type==pst_position ) {
				sprintf(buf," dx=%d dy=%d dx_adv=%d dy_adv=%d",
					pst->u.pos.xoff, pst->u.pos.yoff,
					pst->u.pos.h_adv_off, pst->u.pos.v_adv_off );
				uc_strcat(ubuf,buf);
			    } else if ( pst->type==pst_pair ) {
				sprintf(buf," %.50s dx=%d dy=%d dx_adv=%d dy_adv=%d | dx=%d dy=%d dx_adv=%d dy_adv=%d",
					pst->u.pair.paired,
					pst->u.pair.vr[0].xoff, pst->u.pair.vr[0].yoff,
					pst->u.pair.vr[0].h_adv_off, pst->u.pair.vr[0].v_adv_off,
					pst->u.pair.vr[1].xoff, pst->u.pair.vr[1].yoff,
					pst->u.pair.vr[1].h_adv_off, pst->u.pair.vr[1].v_adv_off );
				uc_strcat(ubuf,buf);
			    } else {
				if ( !PSTAllComponentsExist(_sf,pst->u.subs.variant ))
		continue;
				if ( pst->type==pst_ligature )
				    uc_strcat(ubuf, " <= " );
				else
				    uc_strcat(ubuf, " => " );
				uc_strcat(ubuf,pst->u.subs.variant);
			    }
			    chars[tot].label = u_copy(ubuf);
			    chars[tot].parent = node;
			} else {
			    if ( pst->type==pst_position )
				len = strlen(sc->name)+40;
			    else if ( pst->type==pst_pair )
				len = strlen(sc->name)+strlen(pst->u.pair.paired)+
				    100;
			    else
				len = strlen(sc->name)+strlen(pst->u.subs.variant)+8;
			    if ( len>maxl ) maxl = len;
			}
			++tot;
		    }
		}
	    }
	}
	if ( tot==0 )
return;
	if ( chars==NULL ) {
	    ubuf = galloc(maxl*sizeof(unichar_t));
	    chars = gcalloc(tot+1,sizeof(struct node));
	}
    }
    node->children = chars;
    node->cnt = tot;
    free(ubuf);
}

static void BuildMorxFeatures(struct node *node,struct att_dlg *att) {
    uint32 script = node->parent->tag, lang = DEFAULT_LANG, feat=node->tag;
    BuildFeatures(node,att, script,lang,feat,false);
}

#if defined(FONTFORGE_CONFIG_GDRAW)
unichar_t *TagFullName(SplineFont *sf,uint32 tag, int ismac) {
    unichar_t ubuf[100], *end = ubuf+sizeof(ubuf)/sizeof(ubuf[0]), *setname;
    int j,k;
    extern GTextInfo *pst_tags[];

    if ( ismac==-1 )
	/* Guess */
	ismac = (tag>>24)<' ' || (tag>>24)>0x7e;

    if ( ismac ) {
	char buf[30];
	sprintf( buf, "<%d,%d> ", tag>>16,tag&0xffff );
	uc_strcpy(ubuf, buf );
	if ( (setname = PickNameFromMacName(FindMacSettingName(sf,tag>>16,tag&0xffff)))!=NULL ) {
	    u_strcat( ubuf, setname );
	    free( setname );
	}
    } else {
	if ( tag==REQUIRED_FEATURE ) {
	    u_strcpy(ubuf,GStringGetResource(_STR_RequiredFeature,NULL));
	} else {
	    for ( k=0; pst_tags[k]!=NULL; ++k ) {
		for ( j=0; pst_tags[k][j].text!=NULL && tag!=(uint32) pst_tags[k][j].userdata; ++j );
		if ( pst_tags[k][j].text!=NULL )
	    break;
	    }
	    ubuf[0] = '\'';
	    ubuf[1] = tag>>24;
	    ubuf[2] = (tag>>16)&0xff;
	    ubuf[3] = (tag>>8)&0xff;
	    ubuf[4] = tag&0xff;
	    ubuf[5] = '\'';
	    ubuf[6] = ' ';
	    if ( pst_tags[k]!=NULL )
		u_strncpy(ubuf+7, GStringGetResource((uint32) pst_tags[k][j].text,NULL),end-ubuf-7);
	    else
		ubuf[7]='\0';
	}
    }
return( u_copy( ubuf ));
}
#elif defined(FONTFORGE_CONFIG_GTK)
#endif

static void BuildMorxScript(struct node *node,struct att_dlg *att) {
    uint32 script = node->tag, lang = DEFAULT_LANG;
    int i,k,l,m, tot, max;
    SplineFont *_sf = att->sf, *sf, *sf_sl;
    uint32 *feats;
    FPST *fpst, **fpsts;
    struct node *featnodes;
    int feat, set;
    SplineChar *sc;
    PST *pst;

    sf_sl = _sf;
    if ( sf_sl->cidmaster ) sf_sl = sf_sl->cidmaster;
    else if ( sf_sl->mm!=NULL ) sf_sl = sf_sl->mm->normal;

    /* Build up the list of features in this "script" entry in morx */
    k=tot=0;
    max=30;
    feats = galloc(max*sizeof(uint32));
    do {
	sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[k];
	for ( i=0; i<sf->charcnt; ++i ) if ( (sc=sf->chars[i])!=NULL ) {
	    for ( pst=sc->possub; pst!=NULL; pst=pst->next )
		    if ( pst->type!=pst_position && pst->type!=pst_lcaret &&
			    pst->type!=pst_pair &&
			    pst->script_lang_index!=SLI_NESTED &&
			    (pst->macfeature ||
			     OTTagToMacFeature(pst->tag,&feat,&set))) {
		int sli = pst->script_lang_index;
		struct script_record *sr = sf_sl->script_lang[sli];
		for ( l=0; sr[l].script!=0 && sr[l].script!=script; ++l );
		if ( sr[l].script!=0 ) {
		    for ( m=0; sr[l].langs[m]!=0 && sr[l].langs[m]!=lang; ++m );
		    if ( sr[l].langs[m]!=0 ) {
			for ( l=0; l<tot && feats[l]!=pst->tag; ++l );
			if ( l>=tot ) {
			    if ( tot>=max )
				feats = grealloc(feats,(max+=30)*sizeof(uint32));
			    feats[tot++] = pst->tag;
			}
		    }
		}
	    }
	}
	++k;
    } while ( k<_sf->subfontcnt );

    fpsts = NULL;
    for ( fpst = _sf->possub; fpst!=NULL; fpst=fpst->next ) {
	if ( _sf->sm==NULL && FPSTisMacable(_sf,fpst,true)) {
	    int sli = fpst->script_lang_index;
	    struct script_record *sr = sf_sl->script_lang[sli];
	    for ( l=0; sr[l].script!=0 && sr[l].script!=script; ++l );
	    if ( sr[l].script!=0 ) {
		for ( m=0; sr[l].langs[m]!=0 && sr[l].langs[m]!=lang; ++m );
		if ( sr[l].langs[m]!=0 ) {
		    if ( fpsts==NULL )
			fpsts = gcalloc(max,sizeof(FPST *));
		    if ( tot>=max ) {
			feats = grealloc(feats,(max+=30)*sizeof(uint32));
			fpsts = grealloc(fpsts,max*sizeof(FPST *));
		    }
		    fpsts[tot] = fpst;
		    feats[tot++] = fpst->tag;
		}
	    }
	}
    }

    featnodes = gcalloc(tot+1,sizeof(struct node));
    for ( i=0; i<tot; ++i ) {
	/* featnodes[i].tag = feats[i]; */
	featnodes[i].parent = node;
	featnodes[i].build = BuildMorxFeatures;
	if ( fpsts!=NULL && fpsts[i]!=NULL ) {
	    featnodes[i].build = BuildFPST;
	    featnodes[i].u.fpst = fpsts[i];
	}
	if ( !OTTagToMacFeature(feats[i],&feat,&set) ) {
	    feat = feats[i]>>16;
	    set = feats[i]&0xffff;
	    featnodes[i].macfeat = true;
	}
	featnodes[i].label = TagFullName(sf,(feat<<16)|set,true);
    }

    qsort(featnodes,tot,sizeof(struct node),compare_tag);
    for ( i=0; i<tot; ++i )
	if ( !featnodes[i].macfeat )
	    featnodes[i].tag = MacFeatureToOTTag(featnodes[i].tag>>16,featnodes[i].tag&0xffff);
    node->children = featnodes;
    node->cnt = tot;
}

static void BuildKerns_(struct node *node,struct att_dlg *att) {
    uint32 script = node->parent->parent->tag, lang = node->parent->tag;
    BuildKerns(node,att,script,lang,BuildKerns2G,false);
}

static void BuildKernsV_(struct node *node,struct att_dlg *att) {
    uint32 script = node->parent->parent->tag, lang = node->parent->tag;
    BuildKerns(node,att,script,lang,BuildKerns2GV,true);
}

static void BuildAnchorLists_(struct node *node,struct att_dlg *att) {
    uint32 script = node->parent->parent->tag;
    BuildAnchorLists(node,att,script);
}

static void BuildFeature_(struct node *node,struct att_dlg *att) {
    uint32 feat=node->tag;
    FPST *fpst = node->parent->parent->u.fpst;
    int isgpos = (fpst->type == pst_contextpos || fpst->type == pst_chainpos);
    BuildFeatures(node,att, '*','*',feat,isgpos);
}

static void BuildFeature__(struct node *node,struct att_dlg *att) {
    uint32 feat=node->tag;
    BuildFeatures(node,att, '*','*',feat,false);
}

static void FigureBuild(struct node *node,uint32 tag,SplineFont *sf) {
    FPST *fpst;
    AnchorClass *ac;

    for ( fpst=sf->possub; fpst!=NULL && fpst->tag!=tag; fpst=fpst->next );
    if ( fpst!=NULL ) {
	node->u.fpst = fpst;
	node->build = BuildFPST;
return;
    }
    for ( ac=sf->anchor; ac!=NULL && ac->feature_tag!=tag; ac=ac->next );
    if ( ac!=NULL ) {
	node->u.ac = ac;
	node->build = BuildAnchorLists_;
return;
    }
    node->build = BuildFeature_;
}

static void BuildFPSTRule(struct node *node,struct att_dlg *att) {
    FPST *fpst = node->parent->u.fpst;
    int index = node->u.index;
    struct fpst_rule *r = &fpst->rules[index];
    int len, i, j;
    struct node *lines;
    char buf[200], *space, *pt, *start, *spt;
    unichar_t *upt;
    SplineFont *sf = att->sf;

    for ( i=0; i<2; ++i ) {
	len = 0;

	switch ( fpst->format ) {
	  case pst_glyphs:
	    if ( r->u.glyph.back!=NULL && *r->u.glyph.back!='\0' ) {
		if ( i ) {
		    sprintf(buf, "Backtrack Match: " );
		    lines[len].label = galloc((strlen(buf)+strlen(r->u.glyph.back)+1)*sizeof(unichar_t));
		    uc_strcpy(lines[len].label,buf);
		    upt = lines[len].label+u_strlen(lines[len].label);
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
		sprintf(buf, "Match: " );
		lines[len].label = galloc((strlen(buf)+strlen(r->u.glyph.names)+1)*sizeof(unichar_t));
		uc_strcpy(lines[len].label,buf);
		uc_strcat(lines[len].label,r->u.glyph.names);
		lines[len].parent = node;
	    }
	    ++len;
	    if ( r->u.glyph.fore!=NULL && *r->u.glyph.fore!='\0' ) {
		if ( i ) {
		    sprintf(buf, "Lookahead Match: " );
		    lines[len].label = galloc((strlen(buf)+strlen(r->u.glyph.fore)+1)*sizeof(unichar_t));
		    uc_strcpy(lines[len].label,buf);
		    uc_strcat(lines[len].label,r->u.glyph.fore);
		    lines[len].parent = node;
		}
		++len;
	    }
	  break;
	  case pst_class:
	    if ( r->u.class.bcnt!=0 ) {
		if ( i ) {
		    space = pt = galloc(100+7*r->u.class.bcnt);
		    strcpy(space, r->u.class.bcnt==1 ? "Backtrack class: " : "Backtrack classes: " );
		    pt += strlen(space);
		    for ( j=r->u.class.bcnt-1; j>=0; --j ) {
			sprintf( pt, "%d ", r->u.class.bclasses[j] );
			pt += strlen( pt );
			lines[len].label = uc_copy(space);
			lines[len].parent = node;
		    }
		    free(space);
		}
		++len;
	    }
	    if ( i ) {
		space = pt = galloc(100+7*r->u.class.ncnt);
		strcpy(space, r->u.class.ncnt==1 ? "Class: " : "Classes: " );
		pt += strlen(space);
		for ( j=0; j<r->u.class.ncnt; ++j ) {
		    sprintf( pt, "%d ", r->u.class.nclasses[j] );
		    pt += strlen( pt );
		    lines[len].label = uc_copy(space);
		    lines[len].parent = node;
		}
		free(space);
	    }
	    ++len;
	    if ( r->u.class.fcnt!=0 ) {
		if ( i ) {
		    space = pt = galloc(100+7*r->u.class.fcnt);
		    strcpy(space, r->u.class.fcnt==1 ? "Lookahead class: " : "Lookahead classes: " );
		    pt += strlen(space);
		    for ( j=0; j<r->u.class.fcnt; ++j ) {
			sprintf( pt, "%d ", r->u.class.fclasses[j] );
			pt += strlen( pt );
			lines[len].label = uc_copy(space);
			lines[len].parent = node;
		    }
		    free(space);
		}
		++len;
	    }
	  break;
	  case pst_coverage:
	  case pst_reversecoverage:
	    for ( j=r->u.coverage.bcnt-1; j>=0; --j ) {
		if ( i ) {
		    sprintf(buf, "Back coverage %d: ", -j-1);
		    lines[len].label = galloc((strlen(buf)+strlen(r->u.coverage.bcovers[j])+1)*sizeof(unichar_t));
		    uc_strcpy(lines[len].label,buf);
		    uc_strcat(lines[len].label,r->u.coverage.bcovers[j]);
		    lines[len].parent = node;
		}
		++len;
	    }
	    for ( j=0; j<r->u.coverage.ncnt; ++j ) {
		if ( i ) {
		    sprintf(buf, "Coverage %d: ", j);
		    lines[len].label = galloc((strlen(buf)+strlen(r->u.coverage.ncovers[j])+1)*sizeof(unichar_t));
		    uc_strcpy(lines[len].label,buf);
		    uc_strcat(lines[len].label,r->u.coverage.ncovers[j]);
		    lines[len].parent = node;
		}
		++len;
	    }
	    for ( j=0; j<r->u.coverage.fcnt; ++j ) {
		if ( i ) {
		    sprintf(buf, "Lookahead coverage %d: ", j+r->u.coverage.ncnt);
		    lines[len].label = galloc((strlen(buf)+strlen(r->u.coverage.fcovers[j])+1)*sizeof(unichar_t));
		    uc_strcpy(lines[len].label,buf);
		    uc_strcat(lines[len].label,r->u.coverage.fcovers[j]);
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
		    sprintf(buf, "Apply at %d '%c%c%c%c'", r->lookups[j].seq,
			    r->lookups[j].lookup_tag>>24, (r->lookups[j].lookup_tag>>16)&0xff,
			    (r->lookups[j].lookup_tag>>8)&0xff, r->lookups[j].lookup_tag&0xff );
		    lines[len].label = uc_copy(buf);
		    lines[len].parent = node;
		    lines[len].tag = r->lookups[j].lookup_tag;
		    FigureBuild(&lines[len],r->lookups[j].lookup_tag,sf);
		}
		++len;
	    }
	  break;
	  case pst_reversecoverage:
	    if ( i ) {
		sprintf(buf, "Replacement: " );
		lines[len].label = galloc((strlen(buf)+strlen(r->u.rcoverage.replacements)+1)*sizeof(unichar_t));
		uc_strcpy(lines[len].label,buf);
		uc_strcat(lines[len].label,r->u.rcoverage.replacements);
		lines[len].parent = node;
	    }
	    ++len;
	  break;
	}
	if ( i==0 ) {
	    node->children = lines = gcalloc(len+1,sizeof(struct node));
	    node->cnt = len;
	}
    }
}

static void BuildFPST(struct node *node,struct att_dlg *att) {
    FPST *fpst = node->u.fpst;
    int len, i, j;
    struct node *lines;
    char buf[200];
    static char *type[] = { "Contextual Positioning", "Contextual Substitution",
	    "Chaining Positioning", "Chaining Substitution",
	    "Reverse Chaining Subs" };
    static char *format[] = { "glyphs", "classes", "coverage", "coverage" };

    lines = NULL;
    for ( i=0; i<2; ++i ) {
	len = 0;

	if ( i ) {
	    sprintf(buf, "%s by %s", type[fpst->type-pst_contextpos], format[fpst->format]);
	    lines[len].label = uc_copy(buf);
	    lines[len].parent = node;
	}
	++len;
	if ( fpst->format==pst_class ) {
	    for ( j=1; j<fpst->bccnt ; ++j ) {
		if ( i ) {
		    sprintf(buf, "Backtrack class %d: ", j);
		    lines[len].label = galloc((strlen(buf)+strlen(fpst->bclass[j])+1)*sizeof(unichar_t));
		    uc_strcpy(lines[len].label,buf);
		    uc_strcat(lines[len].label,fpst->bclass[j]);
		    lines[len].parent = node;
		}
		++len;
	    }
	    for ( j=1; j<fpst->nccnt ; ++j ) {
		if ( i ) {
		    sprintf(buf, "Class %d: ", j);
		    lines[len].label = galloc((strlen(buf)+strlen(fpst->nclass[j])+1)*sizeof(unichar_t));
		    uc_strcpy(lines[len].label,buf);
		    uc_strcat(lines[len].label,fpst->nclass[j]);
		    lines[len].parent = node;
		}
		++len;
	    }
	    for ( j=1; j<fpst->fccnt ; ++j ) {
		if ( i ) {
		    sprintf(buf, "Lookahead class %d: ", j);
		    lines[len].label = galloc((strlen(buf)+strlen(fpst->fclass[j])+1)*sizeof(unichar_t));
		    uc_strcpy(lines[len].label,buf);
		    uc_strcat(lines[len].label,fpst->fclass[j]);
		    lines[len].parent = node;
		}
		++len;
	    }
	}
	for ( j=0; j<fpst->rule_cnt; ++j ) {
	    if ( i ) {
		sprintf(buf, "Rule %d", j);
		lines[len].label = uc_copy(buf);
		lines[len].parent = node;
		lines[len].u.index = j;
		lines[len].build = BuildFPSTRule;
	    }
	    ++len;
	}
	if ( i==0 ) {
	    node->children = lines = gcalloc(len+1,sizeof(struct node));
	    node->cnt = len;
	}
    }
}

static void BuildASM(struct node *node,struct att_dlg *att) {
    ASM *sm = node->u.sm;
    int len, i, j, k, scnt = 0;
    struct node *lines;
    char buf[200], *space;
    static char *type[] = { "Indic Reordering", "Contextual Substitution",
	    "Ligatures", "<undefined>", "Simple Substitution",
	    "Glyph Insertion", "<undefined>",  "<undefined>", "<undefined>",
	    "<undefined>", "<undefined>", "<undefined>","<undefined>",
	    "<undefined>", "<undefined>", "<undefined>", "<undefined>",
	    "Kern by State" };
    uint32 *subs=NULL;

    if ( sm->type == asm_context ) {
	subs = galloc(sm->class_cnt*sm->state_cnt*2*sizeof(uint32));
	for ( i=scnt=0; i<sm->class_cnt*sm->state_cnt; ++i ) {
	    int tag;
	    tag = sm->state[i].u.context.mark_tag;
	    if ( tag!=0 ) {
		for ( k=0; k<scnt && subs[k]!=tag; ++k );
		if ( k==scnt ) subs[scnt++] = tag;
	    }
	    tag = sm->state[i].u.context.cur_tag;
	    if ( tag!=0 ) {
		for ( k=0; k<scnt && subs[k]!=tag; ++k );
		if ( k==scnt ) subs[scnt++] = tag;
	    }
	}
    }

    lines = NULL;
    space = galloc( 5*sm->class_cnt+40 );
    for ( i=0; i<2; ++i ) {
	len = 0;

	if ( i ) {
	    lines[len].label = uc_copy(type[sm->type]);
	    lines[len].parent = node;
	}
	++len;
	for ( j=4; j<sm->class_cnt ; ++j ) {
	    if ( i ) {
		sprintf(buf, "Class %d: ", j);
		lines[len].label = galloc((strlen(buf)+strlen(sm->classes[j])+1)*sizeof(unichar_t));
		uc_strcpy(lines[len].label,buf);
		uc_strcat(lines[len].label,sm->classes[j]);
		lines[len].parent = node;
	    }
	    ++len;
	}
	for ( j=0; j<sm->state_cnt; ++j ) {
	    if ( i ) {
		sprintf(space, "State %4d Next: ", j );
		for ( k=0; k<sm->class_cnt; ++k )
		    sprintf( space+strlen(space), "%5d", sm->state[j*sm->class_cnt+k].next_state );
		lines[len].label = uc_copy(space);
		lines[len].parent = node;
		lines[len].monospace = true;
	    }
	    ++len;
	    if ( i ) {
		sprintf(space, "State %4d Flags:", j );
		for ( k=0; k<sm->class_cnt; ++k )
		    sprintf( space+strlen(space), " %04x", sm->state[j*sm->class_cnt+k].flags );
		lines[len].label = uc_copy(space);
		lines[len].parent = node;
		lines[len].monospace = true;
	    }
	    ++len;
	    if ( sm->type==asm_context ) {
		if ( i ) {
		    sprintf(space, "State %4d Mark: ", j );
		    for ( k=0; k<sm->class_cnt; ++k )
			if ( sm->state[j*sm->class_cnt+k].u.context.mark_tag==0 )
			    strcat(space,"     ");
			else
			    sprintf( space+strlen(space), " %c%c%c%c",
				    sm->state[j*sm->class_cnt+k].u.context.mark_tag>>24,
				    (sm->state[j*sm->class_cnt+k].u.context.mark_tag>>16)&0xff,
				    (sm->state[j*sm->class_cnt+k].u.context.mark_tag>>8)&0xff,
				    sm->state[j*sm->class_cnt+k].u.context.mark_tag&0xff );
		    lines[len].label = uc_copy(space);
		    lines[len].parent = node;
		    lines[len].monospace = true;
		}
		++len;
		if ( i ) {
		    sprintf(space, "State %4d Cur:  ", j );
		    for ( k=0; k<sm->class_cnt; ++k )
			if ( sm->state[j*sm->class_cnt+k].u.context.cur_tag==0 )
			    strcat(space,"     ");
			else
			    sprintf( space+strlen(space), " %c%c%c%c",
				    sm->state[j*sm->class_cnt+k].u.context.cur_tag>>24,
				    (sm->state[j*sm->class_cnt+k].u.context.cur_tag>>16)&0xff,
				    (sm->state[j*sm->class_cnt+k].u.context.cur_tag>>8)&0xff,
				    sm->state[j*sm->class_cnt+k].u.context.cur_tag&0xff );
		    lines[len].label = uc_copy(space);
		    lines[len].parent = node;
		    lines[len].monospace = true;
		}
		++len;
	    }
	}
	for ( j=0; j<scnt; ++j ) {
	    if ( i ) {
		sprintf(buf, "Nested Substitution '%c%c%c%c'",
			subs[j]>>24, (subs[j]>>16)&0xff,
			(subs[j]>>8)&0xff, subs[j]&0xff );
		lines[len].label = uc_copy(buf);
		lines[len].parent = node;
		lines[len].tag = subs[j];
		lines[len].build = BuildFeature__;
	    }
	    ++len;
	}
	if ( i==0 ) {
	    node->children = lines = gcalloc(len+1,sizeof(struct node));
	    node->cnt = len;
	}
    }
    free(space);
    free(subs);
}

static void BuildGSUBfeatures(struct node *node,struct att_dlg *att) {
    int isgsub = node->parent->parent->parent->tag==CHR('G','S','U','B');
    uint32 script = node->parent->parent->tag, lang = node->parent->tag, feat=node->tag;

    BuildFeatures(node,att, script,lang,feat,!isgsub);
}

static void BuildGSUBlang(struct node *node,struct att_dlg *att) {
    int isgsub = node->parent->parent->tag==CHR('G','S','U','B');
    uint32 script = node->parent->tag, lang = node->tag;
    int i,k,l, tot, max;
    SplineFont *_sf = att->sf, *sf;
    struct f {
	uint32 feats;
	union sak acs;
	void (*build)(struct node *,struct att_dlg *);
    } *f;
    struct node *featnodes;
    int haskerns=false, hasvkerns=false;
    AnchorClass *ac, *ac2;
    SplineChar *sc;
    PST *pst;
    KernPair *kp;
    int isv;
    KernClass *kc;
    FPST *fpst;

    /* Build up the list of features in this lang entry of this script in GSUB/GPOS */
    k=tot=0;
    max=30;
    f = gcalloc(max,sizeof(struct f));
    do {
	sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[k];
	for ( i=0; i<sf->charcnt; ++i ) if ( (sc=sf->chars[i])!=NULL ) {
	    for ( pst=sc->possub; pst!=NULL; pst=pst->next )
		    if ((isgsub && pst->type!=pst_position && pst->type!=pst_lcaret &&
			    pst->type!=pst_pair) ||
			    (!isgsub && (pst->type==pst_position || pst->type==pst_pair)) ) {
		if ( SLIMatches(_sf,pst->script_lang_index,script,lang) &&
			!pst->macfeature ) {
		    for ( l=0; l<tot && f[l].feats!=pst->tag; ++l );
		    if ( l>=tot ) {
			if ( tot>=max ) {
			    f = grealloc(f,(max+=30)*sizeof(struct f));
			    memset(f+(max-30),0,30*sizeof(struct f));
			}
			f[tot++].feats = pst->tag;
		    }
		}
	    }
	    if ( !isgsub ) {
		for ( isv=0; isv<2; ++isv ) {
		    if ( isv && hasvkerns )
		continue;
		    if ( !isv && haskerns )
		continue;
		    for ( kp=isv ? sc->vkerns : sc->kerns; kp!=NULL; kp=kp->next ) {
			if ( SLIMatches(_sf,kp->sli,script,lang)) {
			    if ( tot>=max ) {
				f = grealloc(f,(max+=30)*sizeof(struct f));
				memset(f+(max-30),0,30*sizeof(struct f));
			    }
			    if ( isv ) {
				f[tot].feats = CHR('v','k','r','n');
				f[tot++].build = BuildKernsV_;
				hasvkerns = true;
			    } else {
				f[tot].feats = CHR('k','e','r','n');
				f[tot++].build = BuildKerns_;
				haskerns = true;
			    }
		    break;
			}
		    }
		}
	    }
	}
	++k;
    } while ( k<_sf->subfontcnt );
    if ( !isgsub ) {
	AnchorClassOrder(_sf);
	for ( ac=_sf->anchor; ac!=NULL; ac=ac->next )
	    ac->processed = false;
	for ( ac=_sf->anchor; ac!=NULL; ac = ac->next ) if ( !ac->processed ) {
	    if ( SLIMatches(_sf,ac->script_lang_index,script,lang) ) {
		/* We expect to get multiple features with the same tag here */
		if ( tot>=max ) {
		    f = grealloc(f,(max+=30)*sizeof(struct f));
		    memset(f+(max-30),0,30*sizeof(struct f));
		}
		f[tot].acs.ac = ac;
		f[tot].build = BuildAnchorLists_;
		f[tot++].feats = ac->feature_tag;
	    }
	    /* These all get merged together into one feature */
	    for ( ac2 = ac->next; ac2!=NULL ; ac2 = ac2->next ) {
		if ( ac2->feature_tag==ac->feature_tag &&
			ac2->script_lang_index == ac->script_lang_index &&
			ac2->merge_with == ac->merge_with )
		    ac2->processed = true;
	    }
	}
	for ( isv = 0; isv<2; ++isv ) {
	    for ( kc=isv ? _sf->vkerns : _sf->kerns; kc!=NULL; kc=kc->next ) {
		if ( SLIMatches(_sf,kc->sli,script,lang) ) {
		    /* We expect to get multiple features with the same tag here */
		    if ( tot>=max ) {
			f = grealloc(f,(max+=30)*sizeof(struct f));
			memset(f+(max-30),0,30*sizeof(struct f));
		    }
		    f[tot].acs.kc = kc;
		    f[tot].build = BuildKC;
		    f[tot++].feats = isv ? CHR('v','k','r','n') : CHR('k','e','r','n');
		}
	    }
	}
    }

    for ( fpst = _sf->possub; fpst!=NULL; fpst=fpst->next )
	if (( !isgsub && (fpst->type==pst_contextpos || fpst->type==pst_chainpos)) ||
		( isgsub && (fpst->type==pst_contextsub || fpst->type==pst_chainsub || fpst->type==pst_reversesub ))) {
	    if ( SLIMatches(_sf,fpst->script_lang_index,script,lang)) {
		/* We expect to get multiple features with the same tag here */
		if ( tot>=max ) {
		    f = grealloc(f,(max+=30)*sizeof(struct f));
		    memset(f+(max-30),0,30*sizeof(struct f));
		}
		f[tot].acs.fpst = fpst;
		f[tot].build = BuildFPST;
		f[tot++].feats = fpst->tag;
	    }
	}

    featnodes = gcalloc(tot+1,sizeof(struct node));
    for ( i=0; i<tot; ++i ) {
	featnodes[i].tag = f[i].feats;
	featnodes[i].parent = node;
	featnodes[i].build = f[i].build==NULL ? BuildGSUBfeatures : f[i].build;
	featnodes[i].u = f[i].acs;
	featnodes[i].label = TagFullName(sf,featnodes[i].tag,false);
    }

    qsort(featnodes,tot,sizeof(struct node),compare_tag);
    node->children = featnodes;
    node->cnt = tot;
}

static void BuildGSUBscript(struct node *node,struct att_dlg *att) {
    SplineFont *sf = att->sf, *sf_sl = sf;
    int lang_max;
    int i,j,k,l;
    struct node *langnodes;
    extern GTextInfo languages[];
    unichar_t ubuf[100];

    /* Build the list of languages that are used in this script */
    /* Don't bother to check whether they actually get used */
    if ( sf_sl->cidmaster!=NULL ) sf_sl = sf_sl->cidmaster;
    else if ( sf_sl->mm!=NULL ) sf_sl = sf_sl->mm->normal;

    for ( j=0; j<2; ++j ) {
	lang_max = 0;
	if ( sf_sl->script_lang!=NULL )
	for ( i=0; sf_sl->script_lang[i]!=NULL ; ++i ) {
	    for ( k=0; sf_sl->script_lang[i][k].script!=0; ++k ) {
		if ( sf_sl->script_lang[i][k].script == node->tag ) {
		    for ( l=0; sf_sl->script_lang[i][k].langs[l]!=0; ++l ) {
			if ( j )
			    langnodes[lang_max].tag = sf_sl->script_lang[i][k].langs[l];
			++lang_max;
		    }
		}
	    }
	}
	if ( lang_max==0 )
return;
	if ( j==0 )
	    langnodes = gcalloc(lang_max+1,sizeof(struct node));
    }

    qsort(langnodes,lang_max,sizeof(struct node),compare_tag);

    /* Remove duplicates */
    for ( i=j=0; i<lang_max; ++j, ++i ) {
	langnodes[j].tag = langnodes[i].tag;
	while ( langnodes[i].tag==langnodes[i+1].tag )
	    ++i;
    }
    langnodes[j].tag = 0;

    lang_max = j;
    for ( i=0; i<lang_max; ++i ) {
	for ( j=0; languages[j].text!=NULL && langnodes[i].tag!=(uint32) languages[j].userdata; ++j );
	ubuf[0] = '\'';
	ubuf[1] = langnodes[i].tag>>24;
	ubuf[2] = (langnodes[i].tag>>16)&0xff;
	ubuf[3] = (langnodes[i].tag>>8)&0xff;
	ubuf[4] = langnodes[i].tag&0xff;
	ubuf[5] = '\'';
	ubuf[6] = ' ';
	if ( languages[j].text!=NULL ) {
	    u_strcpy(ubuf+7,GStringGetResource((uint32) languages[j].text,NULL));
	    uc_strcat(ubuf," ");
	} else
	    ubuf[7]='\0';
#if defined(FONTFORGE_CONFIG_GDRAW)
	u_strcat(ubuf,GStringGetResource(_STR_OTFLanguage,NULL));
#elif defined(FONTFORGE_CONFIG_GTK)
	u_strcat(ubuf,_("Language"));
#endif
	langnodes[i].label = u_copy(ubuf);
	langnodes[i].build = BuildGSUBlang;
	langnodes[i].parent = node;
    }
    node->children = langnodes;
    node->cnt = i;
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
    node->children = lcars = gcalloc(j+1,sizeof(struct node));
    node->cnt = j;
    for ( j=i=0; j<pst->u.lcaret.cnt; ++j ) {
	if ( pst->u.lcaret.carets[j]!=0 ) {
	    sprintf( buffer,"%d", pst->u.lcaret.carets[j] );
	    lcars[i].parent = node;
	    lcars[i++].label = uc_copy(buffer);
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
	    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL && sf->chars[i]->ttf_glyph!=-1 ) {
		for ( pst=sf->chars[i]->possub; pst!=NULL; pst=pst->next ) {
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
			glyphs[lcnt].u.sc = sf->chars[i];
			glyphs[lcnt].label = uc_copy(sf->chars[i]->name);
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
	node->children = glyphs = gcalloc(lcnt+1,sizeof(struct node));
	node->cnt = lcnt;
    }
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
	if ( cmax<sf->charcnt ) cmax = sf->charcnt;
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
		if ( l<sf->charcnt && sf->chars[i]!=NULL ) {
		    sc = sf->chars[i];
	    break;
		}
		++l;
	    } while ( l<_sf->subfontcnt );
	    if ( sc!=NULL && SCWorthOutputting(sc) ) {
		if ( chars!=NULL ) {
		    int gdefc = gdefclass(sc);
		    sprintf(buffer,"%.70s %s", sc->name,
			gdefc==0 ? "Not classified" :
			gdefc==1 ? "Base" :
			gdefc==2 ? "Ligature" :
			gdefc==3 ? "Mark" :
			    "Componant" );
		    chars[ccnt].parent = node;
		    chars[ccnt].label = uc_copy(buffer);;
		}
		++ccnt;
	    }
	}
	if ( ccnt==0 )
    break;
	if ( chars==NULL ) {
	    node->cnt = ccnt;
	    node->children = chars = gcalloc(ccnt+1,sizeof(struct node));
	}
    }
}

static void BuildGDEF(struct node *node,struct att_dlg *att) {
    SplineFont *sf, *_sf = att->sf;
    AnchorClass *ac;
    PST *pst;
    int l,j,i;
    int gdef, lcar;

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
	for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL && sf->chars[i]->ttf_glyph!=-1 ) {
	    for ( pst=sf->chars[i]->possub; pst!=NULL; pst=pst->next ) {
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
	    if ( sf->chars[i]->glyph_class!=0 )
		gdef = 1;
	}
	++l;
    } while ( l<_sf->subfontcnt );

    if ( gdef+lcar!=0 ) {
	node->children = gcalloc(gdef+lcar+1,sizeof(struct node));
	node->cnt = gdef+lcar;
	if ( gdef ) {
	    node->children[0].label = uc_copy("Glyph Definition Sub-Table");
	    node->children[0].build = BuildGdefs;
	    node->children[0].parent = node;
	}
	if ( lcar ) {
	    node->children[gdef].label = uc_copy("Ligature Caret Sub-Table");
	    node->children[gdef].build = BuildLcar;
	    node->children[gdef].parent = node;
	}
    }
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
	if ( cmax<sf->charcnt ) cmax = sf->charcnt;
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
		if ( i<sf->charcnt && sf->chars[i]!=NULL ) {
		    sc = sf->chars[i];
	    break;
		}
		++l;
	    } while ( l<_sf->subfontcnt );
	    if ( sc!=NULL && SCWorthOutputting(sc) &&
		    haslrbounds(sc,&left,&right)) {
		if ( chars!=NULL ) {
		    strncpy(buffer,sc->name,70);
		    if ( left!=NULL )
			sprintf(buffer+strlen(buffer), "  Left Bound=%d",
				left->u.pos.xoff );
		    if ( right!=NULL )
			sprintf(buffer+strlen(buffer), "  Right Bound=%d",
				-right->u.pos.h_adv_off );
		    chars[ccnt].parent = node;
		    chars[ccnt].label = uc_copy(buffer);
		}
		++ccnt;
	    }
	}
	if ( ccnt==0 )
return;
	if ( chars==NULL ) {
	    node->children = chars = gcalloc(ccnt+1,sizeof(struct node));
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
	if ( cmax<sf->charcnt ) cmax = sf->charcnt;
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
		if ( i<sf->charcnt && sf->chars[i]!=NULL ) {
		    sc = sf->chars[i];
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
			(prop&0x7f)==0 ? "Strong Left to Right":
			(prop&0x7f)==1 ? "Strong Right to Left":
			(prop&0x7f)==2 ? "Arabic Right to Left":
			(prop&0x7f)==3 ? "European Number":
			(prop&0x7f)==4 ? "European Number Seperator":
			(prop&0x7f)==5 ? "European Number Terminator":
			(prop&0x7f)==6 ? "Arabic Number":
			(prop&0x7f)==7 ? "Common Number Seperator":
			(prop&0x7f)==8 ? "Block Seperator":
			(prop&0x7f)==9 ? "Segment Seperator":
			(prop&0x7f)==10 ? "White Space":
			(prop&0x7f)==11 ? "Neutral":
			    "<Unknown direction>" );
		    if ( prop&0x8000 )
			strcat(buffer,"  Floating accent");
		    if ( prop&0x4000 )
			strcat(buffer,"  Hang left");
		    if ( prop&0x2000 )
			strcat(buffer,"  Hang right");
		    if ( prop&0x80 )
			strcat(buffer,"  Attach right");
		    if ( prop&0x1000 ) {
			offset = (prop&0xf00)>>8;
			if ( offset&0x8 )
			    offset |= 0xfffffff0;
			if ( offset>0 ) {
			    for ( k=i+offset; k<sf->charcnt; ++k )
				if ( sf->chars[k]!=NULL && sf->chars[k]->ttf_glyph==sc->ttf_glyph+offset ) {
				    sprintf( buffer+strlen(buffer), "  Mirror=%.30s", sf->chars[k]->name );
			    break;
				}
			} else {
			    for ( k=i+offset; k>=0; --k )
				if ( sf->chars[k]!=NULL && sf->chars[k]->ttf_glyph==sc->ttf_glyph+offset ) {
				    sprintf( buffer+strlen(buffer), "  Mirror=%.30s", sf->chars[k]->name );
			    break;
				}
			}
		    }
		    chars[ccnt].parent = node;
		    chars[ccnt++].label = uc_copy(buffer);
		}
	    }
	}
	if ( chars==NULL ) {
	    props = props_array(_sf,ccnt);
	    if ( props==NULL )
return;
	    node->children = chars = gcalloc(ccnt+1,sizeof(struct node));
	}
	node->cnt = ccnt;
    }
    free(props);
}

static void BuildTable(struct node *node,struct att_dlg *att) {
    SplineFont *sf, *_sf = att->sf, *sf_sl = _sf;
    int script_max;
    int i,j,k,l;
    struct node *scriptnodes;
    extern GTextInfo scripts[];
    int isgsub = node->tag==CHR('G','S','U','B'), isgpos = node->tag==CHR('G','P','O','S');
    int ismorx = node->tag==CHR('m','o','r','x'), iskern = node->tag==CHR('k','e','r','n');
    int isvkern = node->tag==CHR('v','k','r','n');
    int feat, set;
    SplineChar *sc;
    PST *pst;
    KernPair *kp;
    unichar_t ubuf[120];
    AnchorClass *ac;
    int isv;
    FPST *fpst;
    ASM *sm;

    if ( sf_sl->cidmaster != NULL ) sf_sl = sf_sl->cidmaster;
    else if ( sf_sl->mm!=NULL ) sf_sl = sf_sl->mm->normal;

    /* Build the list of scripts that are mentioned in the font */
    for ( j=0; j<2; ++j ) {
	script_max = 0;
	if ( sf_sl->script_lang!=NULL )
	for ( i=0; sf_sl->script_lang[i]!=NULL ; ++i ) {
	    for ( k=0; sf_sl->script_lang[i][k].script!=0; ++k ) {
		if ( j )
		    scriptnodes[script_max].tag = sf_sl->script_lang[i][k].script;
		++script_max;
	    }
	}
	if ( script_max==0 )
return;
	if ( j==0 )
	    scriptnodes = gcalloc(script_max+1,sizeof(struct node));
    }

    qsort(scriptnodes,script_max,sizeof(struct node),compare_tag);

    /* Remove duplicates */
    for ( i=j=0; i<script_max; ++j, ++i ) {
	scriptnodes[j].tag = scriptnodes[i].tag;
	while ( scriptnodes[i].tag==scriptnodes[i+1].tag )
	    ++i;
    }
    scriptnodes[j].tag = 0;
    script_max = j;

    /* Now check which scripts are used... */
    k=0;
    do {
	sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[k];
	for ( i=0; i<sf->charcnt; ++i ) if ( (sc=sf->chars[i])!=NULL ) {
	    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) if ( pst->type!=pst_lcaret ) {
		int relevant = false;
		if ( pst->type == pst_position || pst->type==pst_pair )
		    relevant = isgpos;
		else if ( isgsub )
		    relevant = !pst->macfeature;
		else if ( ismorx )
		    relevant = pst->macfeature || OTTagToMacFeature(pst->tag,&feat,&set);
		if ( pst->script_lang_index==SLI_NESTED || pst->script_lang_index==SLI_UNKNOWN ) relevant = false;
		if ( relevant ) {
		    int sli = pst->script_lang_index;
		    struct script_record *sr = sf_sl->script_lang[sli];
		    for ( l=0; sr[l].script!=0 ; ++l ) {
			for ( j=0; j<script_max && scriptnodes[j].tag!=sr[l].script; ++j );
			if ( j<script_max )
			    scriptnodes[j].used = true;
		    }
		}
	    }
	    for ( isv=0; isv<2; ++isv ) {
		KernPair *head = isv ? sc->vkerns : sc->kerns;
		if ( head!=NULL && ((iskern && !isv) || (isvkern && isv) || isgpos)) {
		    for ( kp = head; kp!=NULL; kp=kp->next ) {
			int sli = kp->sli;
			struct script_record *sr = sf_sl->script_lang[sli];
			for ( l=0; sr[l].script!=0 ; ++l ) {
			    for ( j=0; j<script_max && scriptnodes[j].tag!=sr[l].script; ++j );
			    if ( j<script_max )
				scriptnodes[j].used = true;
			}
		    }
		}
	    }
	}
	++k;
    } while ( k<_sf->subfontcnt );

    if ( isgpos ) {
	for ( isv=0; isv<2; ++isv ) {
	    KernClass *kc;
	    for ( kc = isv ? _sf->vkerns : _sf->kerns; kc!=NULL; kc=kc->next ) if ( kc->sli!=0xffff && kc->sli!=0xff ) {
		int sli = kc->sli;
		struct script_record *sr = sf_sl->script_lang[sli];
		for ( l=0; sr[l].script!=0 ; ++l ) {
		    for ( j=0; j<script_max && scriptnodes[j].tag!=sr[l].script; ++j );
		    if ( j<script_max )
			scriptnodes[j].used = true;
		}
	    }
	}
    }
		
    if ( isgpos && _sf->anchor!=NULL ) {
	for ( ac=_sf->anchor; ac!=NULL; ac=ac->next ) if ( ac->script_lang_index!=SLI_NESTED && ac->script_lang_index!=SLI_UNKNOWN ) {
	    int sli = ac->script_lang_index;
	    struct script_record *sr = sf_sl->script_lang[sli];
	    for ( l=0; sr[l].script!=0 ; ++l ) {
		for ( j=0; j<script_max && scriptnodes[j].tag!=sr[l].script; ++j );
		if ( j<script_max )
		    scriptnodes[j].used = true;
	    }
	}
    }

    if ( isgpos || isgsub || ismorx ) {
	for ( fpst = _sf->possub; fpst!=NULL; fpst=fpst->next )
	    if ((( isgpos && (fpst->type==pst_contextpos || fpst->type==pst_chainpos)) ||
		    ( isgsub && (fpst->type==pst_contextsub || fpst->type==pst_chainsub || fpst->type==pst_reversesub )) ||
		    ( ismorx && sf->sm==NULL && FPSTisMacable(sf,fpst,true))) &&
		    fpst->script_lang_index!=SLI_NESTED && fpst->script_lang_index!=SLI_UNKNOWN ) {
		int sli = fpst->script_lang_index;
		struct script_record *sr = sf_sl->script_lang[sli];
		for ( l=0; sr[l].script!=0 ; ++l ) {
		    for ( j=0; j<script_max && scriptnodes[j].tag!=sr[l].script; ++j );
		    if ( j<script_max )
			scriptnodes[j].used = true;
		}
	    }
    }

    /* And remove any that aren't */
    for ( i=j=0; i<script_max; ++i ) {
	while ( i<script_max && !scriptnodes[i].used ) ++i;
	if ( i<script_max )
	    scriptnodes[j++].tag = scriptnodes[i].tag;
    }
    scriptnodes[j].tag = 0;
    script_max = j;

    for ( i=0; i<script_max; ++i ) {
	for ( j=0; scripts[j].text!=NULL && scriptnodes[i].tag!=(uint32) scripts[j].userdata; ++j );
	ubuf[0] = '\'';
	ubuf[1] = scriptnodes[i].tag>>24;
	ubuf[2] = (scriptnodes[i].tag>>16)&0xff;
	ubuf[3] = (scriptnodes[i].tag>>8)&0xff;
	ubuf[4] = scriptnodes[i].tag&0xff;
	ubuf[5] = '\'';
	ubuf[6] = ' ';
	if ( scripts[j].text!=NULL ) {
	    u_strcpy(ubuf+7,GStringGetResource((uint32) scripts[j].text,NULL));
	    uc_strcat(ubuf," ");
	} else
	    ubuf[7]='\0';
#if defined(FONTFORGE_CONFIG_GDRAW)
	u_strcat(ubuf,GStringGetResource(_STR_OTFScript,NULL));
#elif defined(FONTFORGE_CONFIG_GTK)
	u_strcat(ubuf,_("Script"));
#endif
	scriptnodes[i].label = u_copy(ubuf);
	scriptnodes[i].build = iskern ? BuildKernScript :
				isvkern ? BuildVKernScript :
				ismorx ? BuildMorxScript :
			        BuildGSUBscript;
	scriptnodes[i].parent = node;
    }

    /* Sadly I have no script for the morx state machines */
    if ( ismorx && _sf->sm!=NULL ) {
	for ( j=0, sm=_sf->sm; sm!=NULL; sm=sm->next ) if ( sm->type!=asm_kern ) ++j;
	scriptnodes = grealloc(scriptnodes,(i+j+1)*sizeof(scriptnodes[0]));
	memset(scriptnodes+i,0,(j+1)*sizeof(scriptnodes[0]));
	for ( j=0, sm=_sf->sm; sm!=NULL; sm=sm->next ) if ( sm->type!=asm_kern ) {
	    scriptnodes[i+j].macfeat = true;
	    scriptnodes[i+j].parent = node;
	    scriptnodes[i+j].build = BuildASM;
	    scriptnodes[i+j].tag = (sm->feature<<16)|sm->setting;
	    scriptnodes[i+j].u.sm = sm;
	    scriptnodes[i+j].label = TagFullName(sf,(sm->feature<<16)|sm->setting,true);
	    ++j;
	}
	qsort(scriptnodes+i,j,sizeof(struct node),compare_tag);
	i += j;
    }

    /* Nor have I a script for the kern state machines */
    if ( iskern && _sf->sm!=NULL ) {
	for ( j=0, sm=_sf->sm; sm!=NULL; sm=sm->next, ++j );
	scriptnodes = grealloc(scriptnodes,(i+j+1)*sizeof(scriptnodes[0]));
	memset(scriptnodes+i,0,(j+1)*sizeof(scriptnodes[0]));
	for ( j=0, sm=_sf->sm; sm!=NULL; sm=sm->next ) if ( sm->type==asm_kern ) {
	    scriptnodes[i+j].parent = node;
	    scriptnodes[i+j].build = BuildASM;
	    scriptnodes[i+j].u.sm = sm;
	    scriptnodes[i+j].label = uc_copy("Kern by State Machine");
	    ++j;
	}
	qsort(scriptnodes+i,j,sizeof(struct node),compare_tag);
	i += j;
    }
    node->children = scriptnodes;
    node->cnt = i;
}

static void BuildTop(struct att_dlg *att) {
    SplineFont *sf, *_sf = att->sf;
    int hasgsub=0, hasgpos=0, hasgdef=0;
    int hasmorx=0, haskern=0, hasvkern=0, haslcar=0, hasprop=0, hasopbd=0;
    int haskc=0, hasvkc=0;
    int feat, set;
    struct node *tables;
    PST *pst;
    SplineChar *sc;
    int i,k,j;
    AnchorClass *ac;
    KernClass *kc;
    FPST *fpst;
    ASM *sm;

    k=0;
    do {
	sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[k];
	for ( i=0; i<sf->charcnt; ++i ) if ( (sc=sf->chars[i])!=NULL ) {
	    if (( sc->unicodeenc>=0x10800 && sc->unicodeenc<=0x103ff ) ||
		    ( sc->unicodeenc!=-1 && sc->unicodeenc<0x10fff &&
			isrighttoleft(sc->unicodeenc)) ||
			ScriptIsRightToLeft(SCScriptFromUnicode(sc)) ) {
		hasprop = true;
	    }
	    if ( sc->glyph_class!=0 )
		hasgdef = true;
	    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
		if ( pst->type == pst_position ) {
		    hasgpos = true;
		    if ( pst->tag==CHR('l','f','b','d') || pst->tag==CHR('r','t','b','d') )
			hasopbd = true;
		} else if ( pst->type == pst_pair ) {
		    hasgpos = true;
		} else if ( pst->type == pst_lcaret ) {
		    for ( j=pst->u.lcaret.cnt-1; j>=0; --j )
			if ( pst->u.lcaret.carets[j]!=0 )
		    break;
		    if ( j!=-1 )
			hasgdef = haslcar = true;
		} else {
		    if ( !pst->macfeature )
			hasgsub = true;
		    if ( SLIHasDefault(sf,pst->script_lang_index) &&
			    (pst->macfeature ||
			     OTTagToMacFeature(pst->tag,&feat,&set)))
			hasmorx = true;
		}
	    }
	    if ( sc->kerns!=NULL && SCScriptFromUnicode(sc)!=DEFAULT_SCRIPT ) {
		haskern = hasgpos = true;
		SFAddScriptLangIndex(sf,SCScriptFromUnicode(sc),DEFAULT_LANG);
	    }
	    if ( sc->vkerns!=NULL && SCScriptFromUnicode(sc)!=DEFAULT_SCRIPT ) {
		hasvkern = hasgpos = true;
		SFAddScriptLangIndex(sf,SCScriptFromUnicode(sc),DEFAULT_LANG);
	    }
	}
	++k;
    } while ( k<_sf->subfontcnt );
    for ( kc = _sf->kerns; kc!=NULL; kc=kc->next )
	++hasvkc;
    for ( kc = _sf->vkerns; kc!=NULL; kc=kc->next )
	++haskc;
    if ( haskc || hasvkc )
	hasgpos = true;
    if ( _sf->anchor!=NULL )
	hasgpos = true;
    for ( ac = sf->anchor; ac!=NULL; ac=ac->next ) {
	if ( ac->type==act_curs )
    break;
    }
    if ( ac!=NULL )
	hasgdef = true;
    for ( fpst = sf->possub; fpst!=NULL; fpst=fpst->next ) {
	if ( fpst->type == pst_contextpos || fpst->type == pst_chainpos )
	    hasgpos = true;
	else
	    hasgsub = true;
	if ( sf->sm==NULL && FPSTisMacable(sf,fpst,true))
	    hasmorx = true;
    }
    for ( sm=sf->sm; sm!=NULL; sm=sm->next ) {
	if ( sm->type == asm_kern )
	    haskern = true;
	else
	    hasmorx = true;
    }

    if ( hasgsub+hasgpos+hasgdef+hasmorx+haskern+haslcar+hasopbd+hasprop==0 ) {
	tables = gcalloc(2,sizeof(struct node));
#if defined(FONTFORGE_CONFIG_GDRAW)
	tables[0].label = u_copy(GStringGetResource(_STR_NoAdvancedTypography,NULL));
#elif defined(FONTFORGE_CONFIG_GTK)
	tables[0].label = u_copy(_("No Advanced Typography"));
#endif
    } else {
	tables = gcalloc((hasgsub||hasgpos||hasgdef)+
	    (hasmorx||haskern||haslcar||hasopbd||hasprop||hasvkern||haskc||hasvkc)+1,sizeof(struct node));
	i=0;
	if ( hasgsub || hasgpos || hasgdef) {
	    tables[i].label = uc_copy("OpenType Tables");
	    tables[i].children_checked = true;
	    tables[i].children = gcalloc(hasgsub+hasgpos+hasgdef+1,sizeof(struct node));
	    tables[i].cnt = hasgsub + hasgpos + hasgdef;
	    if ( hasgdef ) {
		tables[i].children[0].label = uc_copy("'GDEF' Glyph Definition Table");
		tables[i].children[0].tag = CHR('G','D','E','F');
		tables[i].children[0].build = BuildGDEF;
		tables[i].children[0].parent = &tables[i];
	    }
	    if ( hasgpos ) {
		tables[i].children[hasgdef].label = uc_copy("'GPOS' Glyph Positioning Table");
		tables[i].children[hasgdef].tag = CHR('G','P','O','S');
		tables[i].children[hasgdef].build = BuildTable;
		tables[i].children[hasgdef].parent = &tables[i];
	    }
	    if ( hasgsub ) {
		tables[i].children[hasgdef+hasgpos].label = uc_copy("'GSUB' Glyph Substitution Table");
		tables[i].children[hasgdef+hasgpos].tag = CHR('G','S','U','B');
		tables[i].children[hasgdef+hasgpos].build = BuildTable;
		tables[i].children[hasgdef+hasgpos].parent = &tables[i];
	    }
	    ++i;
	}
	if ( hasmorx || haskern || hasvkern || haslcar || hasopbd || hasprop ) {
	    int j = 0;
#if defined(FONTFORGE_CONFIG_GDRAW)
	    tables[i].label = u_copy(GStringGetResource(_STR_AppleAdvancedTypography,NULL));
#elif defined(FONTFORGE_CONFIG_GTK)
	    tables[i].label = u_copy(_("Apple Advanced Typography"));
#endif
	    tables[i].children_checked = true;
	    tables[i].children = gcalloc(hasmorx+haskern+haslcar+hasopbd+hasprop+hasvkern+haskc+hasvkc+1,sizeof(struct node));
	    tables[i].cnt = hasmorx+haskern+hasopbd+hasprop+haslcar+hasvkern+haskc+hasvkc;
	    if ( haskern ) {
		tables[i].children[j].label = hasvkern||haskc||hasvkc ?
				uc_copy("'kern' Horizontal Kerning Sub-Table") :
				uc_copy("'kern' Horizontal Kerning Table");
		tables[i].children[j].tag = CHR('k','e','r','n');
		tables[i].children[j].build = BuildTable;
		tables[i].children[j++].parent = &tables[i];
	    }
	    for ( kc = _sf->kerns; kc!=NULL; kc=kc->next ) {
		tables[i].children[j].label = uc_copy("'kern' Horizontal Kerning Class");
		tables[i].children[j].tag = CHR('k','e','r','n');
		tables[i].children[j].u.kc = kc;
		tables[i].children[j].build = BuildKC;
		tables[i].children[j++].parent = &tables[i];
	    }
	    if ( hasvkern ) {
		tables[i].children[j].label = haskern ? uc_copy("'kern' Vertical Kerning Sub-Table") :
				uc_copy("'kern' Vertical Kerning Table");
		tables[i].children[j].tag = CHR('v','k','r','n');
		tables[i].children[j].build = BuildTable;
		tables[i].children[j++].parent = &tables[i];
	    }
	    for ( kc = _sf->vkerns; kc!=NULL; kc=kc->next ) {
		tables[i].children[j].label = uc_copy("'kern' Vertical Kerning Class");
		tables[i].children[j].tag = CHR('k','e','r','n');
		tables[i].children[j].u.kc = kc;
		tables[i].children[j].build = BuildKC;
		tables[i].children[j++].parent = &tables[i];
	    }
	    if ( haslcar ) {
		tables[i].children[j].label = uc_copy("'lcar' Ligature Caret Table");
		tables[i].children[j].tag = CHR('l','c','a','r');
		tables[i].children[j].build = BuildLcar;
		tables[i].children[j++].parent = &tables[i];
	    }
	    if ( hasmorx ) {
		tables[i].children[j].label = uc_copy("'morx' Glyph Extended Metamorphasis Table");
		tables[i].children[j].tag = CHR('m','o','r','x');
		tables[i].children[j].build = BuildTable;
		tables[i].children[j++].parent = &tables[i];
	    }
	    if ( hasopbd ) {
		tables[i].children[j].label = uc_copy("'opbd' Optical Bounds Table");
		tables[i].children[j].tag = CHR('o','p','b','d');
		tables[i].children[j].build = BuildOpticalBounds;
		tables[i].children[j++].parent = &tables[i];
	    }
	    if ( hasprop ) {
		tables[i].children[j].label = uc_copy("'prop' Glyph Properties Table");
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
    len = 5+8*depth+ att->as + 5 + GDrawGetTextWidth(att->v,node->label,-1,NULL);
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
    forever {
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
    forever {
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

static void AttExpose(struct att_dlg *att,GWindow pixmap,GRect *rect) {
    int depth, y;
    struct node *node;
    GRect r;
    Color fg;

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
	GDrawDrawText(pixmap,r.x+r.width+5,y,node->label,-1,NULL,fg);
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

static unichar_t txt[] = { '*','.','t','x','t',  '\0' };
static void AttSave(struct att_dlg *att) {
#if defined(FONTFORGE_CONFIG_GDRAW)
    unichar_t *ret = GWidgetSaveAsFile(GStringGetResource(_STR_Save,NULL),NULL,
#elif defined(FONTFORGE_CONFIG_GTK)
    unichar_t *ret = GWidgetSaveAsFile(_("Save"),NULL,
#endif
	    txt,NULL,NULL);
    char *cret;
    FILE *file;
    unichar_t *pt;
    struct node *node;
    int depth=0, d;

    if ( ret==NULL )
return;
    cret = u2def_copy(ret);
    free(ret);
    file = fopen(cret,"w");
    if ( file==NULL ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	GWidgetErrorR(_STR_SaveFailed,_STR_SaveFailed,cret);
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("Save Failed"),_("Save Failed"),cret);
#endif
	free(cret);
return;
    }
    free(cret);

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
	    pututf8(*pt,file);
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
    { { (unichar_t *) _STR_Save, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'S' }, 'S', ksm_control, NULL, NULL, AttSaveM },
    { NULL }
};

static void AttMouse(struct att_dlg *att,GEvent *event) {
    int l, depth, cnt;
    struct node *node;
    GRect r;

    if ( event->type!=et_mouseup )
return;

    l = (event->u.mouse.y/att->fh);
    depth=0;
    node = NodeFindLPos(att->tables,l+att->off_top,&depth);
    ATTChangeCurrent(att,node);
    if ( event->u.mouse.y > l*att->fh+att->as ||
	    event->u.mouse.x<5+8*depth ||
	    event->u.mouse.x>=5+8*depth+att->as || node==NULL )
return;			/* Not in +/- rectangle */
    node->open = !node->open;

    cnt = SizeCnt(att,att->tables,0);

    r.x = 0; r.width = 3000;
    r.y = l*att->fh; r.height = 3000;
    GDrawRequestExpose(att->v,&r,false);
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
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
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
      case GK_Down: case GK_KP_Down:
	ATTChangeCurrent(att,NodeNext(att->current,&depth));
return( true );
      case GK_Up: case GK_KP_Up:
	ATTChangeCurrent(att,NodePrev(att,att->current,&depth));
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
	    (event->u.mouse.button==4 || event->u.mouse.button==5) ) {
return( GGadgetDispatchEvent(att->vsb,event));
    }

    switch ( event->type ) {
      case et_expose:
	AttExpose(att,gw,&event->u.expose.rect);
      break;
      case et_char:
return( AttChar(att,event));
      case et_mousedown:
	if ( event->u.mouse.button==3 )
	    GMenuCreatePopupMenu(gw,event, att_popuplist);
      break;
      case et_mouseup:
	AttMouse(att,event);
      break;
    }
return( true );
}

static int att_e_h(GWindow gw, GEvent *event) {
    struct att_dlg *att = (struct att_dlg *) GDrawGetUserData(gw);

    if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button==4 || event->u.mouse.button==5) ) {
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
	  break;
	}
      break;
      case et_close:
	att->done = true;
      break;
    }
return( true );
}

void ShowAtt(SplineFont *sf) {
    struct att_dlg att;
    GRect pos;
    GWindowAttrs wattrs;
    FontRequest rq;
    static unichar_t helv[] = { 'h', 'e', 'l', 'v', 'e', 't', 'i', 'c', 'a',',','c','a','l','i','b','a','n',',','c','l','e','a','r','l','y','u',',','u','n','i','f','o','n','t',  '\0' };
    static unichar_t courier[] = { 'c', 'o', 'u', 'r', 'i', 'e', 'r', ',', 'm','o','n','o','s','p','a','c','e',',','c','l','e','a','r','l','y','u',',', 'u','n','i','f','o','n','t', '\0' };
    int as, ds, ld;
    GGadgetCreateData gcd[5];
    GTextInfo label[4];
    int sbsize = GDrawPointsToPixels(NULL,_GScrollBar_Width);

    if ( sf->cidmaster ) sf = sf->cidmaster;

    memset( &att,0,sizeof(att));
    att.sf = sf;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
#if defined(FONTFORGE_CONFIG_GDRAW)
    wattrs.window_title = GStringGetResource(_STR_ShowAtt,NULL);
#elif defined(FONTFORGE_CONFIG_GTK)
    wattrs.window_title = _("Show ATT");
#endif
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,GGadgetScale(200));
    pos.height = GDrawPointsToPixels(NULL,300);
    att.gw = GDrawCreateTopWindow(NULL,&pos,att_e_h,&att,&wattrs);

    memset(&rq,'\0',sizeof(rq));
    rq.family_name = helv;
    rq.point_size = 12;
    rq.weight = 400;
    att.font = GDrawInstanciateFont(GDrawGetDisplayOfWindow(att.gw),&rq);
    rq.family_name = courier;		/* I want to show tabluar data sometimes */
    att.monofont = GDrawInstanciateFont(GDrawGetDisplayOfWindow(att.gw),&rq);
    GDrawFontMetrics(att.font,&as,&ds,&ld);
    att.fh = as+ds; att.as = as;

    att.bmargin = GDrawPointsToPixels(NULL,32)+sbsize;

    att.lines_page = (pos.height-att.bmargin)/att.fh;
    att.page_width = pos.width-sbsize;
    wattrs.mask = wam_events|wam_cursor/*|wam_bordwidth|wam_bordcol*/;
    wattrs.border_width = 1;
    wattrs.border_color = 0x000000;
    pos.x = 0; pos.y = 0;
    pos.width -= sbsize; pos.height = att.lines_page*att.fh;
    att.v = GWidgetCreateSubWindow(att.gw,&pos,attv_e_h,&att,&wattrs);
    GDrawSetVisible(att.v,true);

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
    label[2].text = (unichar_t *) _STR_OK;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].creator = GButtonCreate;

    GGadgetsCreate(att.gw,gcd);
    att.vsb = gcd[0].ret;
    att.hsb = gcd[1].ret;
    att.cancel = gcd[2].ret;

    BuildTop(&att);
    att.open_cnt = SizeCnt(&att,att.tables,0);
    GScrollBarSetBounds(att.vsb,0,att.open_cnt,att.lines_page);

    GDrawSetVisible(att.gw,true);

    while ( !att.done )
	GDrawProcessOneEvent(NULL);
    nodesfree(att.tables);
    GDrawDestroyWindow(att.gw);
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
