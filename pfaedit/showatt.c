/* Copyright (C) 2003 by George Williams */
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
    uint16 cnt;
    struct node *children, *parent;
    void (*build)(struct node *,struct att_dlg *);
    unichar_t *label;
    uint32 tag;
    union sak {
	SplineChar *sc;
	AnchorClass *ac;
	KernClass *kc;
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
    GFont *font;
    struct node *current;
};

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
		u_sprintf(ubuf,GStringGetResource(_STR_MarkLigComponentNamePos,NULL),
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
	    u_sprintf(ubuf,GStringGetResource(_STR_MarkAnchorNamePos,NULL), ac2->name,
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
    node->label = u_copy(GStringGetResource(at==at_basechar?_STR_BaseCharacters:
					    at==at_baselig?_STR_BaseLigatures:
			                    _STR_BaseMarks,NULL));
    for ( i=0; bases[i]!=NULL; ++i );
    if ( i==0 ) {
	node->cnt = 1;
	node->children = gcalloc(2,sizeof(struct node));
	node->children[0].label = u_copy(GStringGetResource(_STR_Empty,NULL));
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
    u_sprintf(ubuf,GStringGetResource(_STR_MarkClassS,NULL),ac->name);
    node->label = u_copy(ubuf);
    for ( i=0; marks[i]!=NULL; ++i );
    if ( i==0 ) {
	node->cnt = 1;
	node->children = gcalloc(2,sizeof(struct node));
	node->children[0].label = u_copy(GStringGetResource(_STR_Empty,NULL));
	node->children[0].parent = node;
    } else {
	node->cnt = i;
	node->children = gcalloc(i+1,sizeof(struct node));
	for ( i=0; marks[i]!=NULL; ++i ) {
	    for ( ap=marks[i]->anchor; ap!=NULL && (ap->type!=at_mark || ap->anchor!=ac); ap=ap->next );
	    u_sprintf(ubuf,GStringGetResource(_STR_MarkCharNamePos,NULL), marks[i]->name,
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
	    node->children[0].label = u_copy(GStringGetResource(_STR_Empty,NULL));
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
			    GStringGetResource(_STR_Entry,NULL),
			    ent->me.x, ent->me.y);
		    node->children[cnt].children[i].label = u_copy(ubuf);
		    node->children[cnt].children[i].parent = &node->children[cnt];
		    ++i;
		}
		if ( ext!=NULL ) {
		    u_snprintf(ubuf,sizeof(ubuf)/sizeof(ubuf[0]),
			    GStringGetResource(_STR_Exit,NULL),
			    ext->me.x, ext->me.y);
		    node->children[cnt].children[i].label = u_copy(ubuf);
		    node->children[cnt].children[i].parent = &node->children[cnt];
		    ++i;
		}
	    }
	}
	free(entryexit);
    } else {
	for ( ac2=ac, classcnt=0; ac2!=NULL &&
		ac2->feature_tag==ac->feature_tag &&
		ac2->script_lang_index == ac->script_lang_index &&
		ac2->merge_with == ac->merge_with ; ac2 = ac2->next )
	    ++classcnt;
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

static void BuildKerns2(struct node *node,struct att_dlg *att,
	uint32 script, uint32 lang,int isv) {
    KernPair *kp = node->u.sc->kerns;
    int i,j,l,m;
    struct node *chars;
    char buf[80];
    SplineFont *_sf = att->sf;

    for ( j=0; j<2; ++j ) {
	for ( kp=isv ? node->u.sc->vkerns:node->u.sc->kerns, i=0; kp!=NULL; kp=kp->next ) {
	    int sli = kp->sli;
	    struct script_record *sr = _sf->script_lang[sli];
	    for ( l=0; sr[l].script!=0 && sr[l].script!=script; ++l );
	    if ( sr[l].script!=0 ) {
		for ( m=0; sr[l].langs[m]!=0 && sr[l].langs[m]!=lang; ++m );
		if ( sr[l].langs[m]!=0 ) {
		    if ( j ) {
			sprintf( buf, "%.40s %d", kp->sc->name, kp->off );
			chars[i].label = uc_copy(buf);
			chars[i].parent = node;
		    }
		    ++i;
		}
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
    int i,k,j,l,m, tot;
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
		    int sli = kp->sli;
		    struct script_record *sr = _sf->script_lang[sli];
		    for ( l=0; sr[l].script!=0 && sr[l].script!=script; ++l );
		    if ( sr[l].script!=0 ) {
			for ( m=0; sr[l].langs[m]!=0 && sr[l].langs[m]!=lang; ++m );
			if ( sr[l].langs[m]!=0 ) {
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

static void BuildFeatures(struct node *node,struct att_dlg *att,
	uint32 script, uint32 lang, uint32 tag, int ispos) {
    int i,j,k,l,m, maxc, tot, maxl, len;
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
		if ( sf->chars[i]!=NULL ) {
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
		    int sli = pst->script_lang_index;
		    struct script_record *sr = _sf->script_lang[sli];
		    for ( l=0; sr[l].script!=0 && sr[l].script!=script; ++l );
		    if ( sr[l].script!=0 ) {
			for ( m=0; sr[l].langs[m]!=0 && sr[l].langs[m]!=lang; ++m );
			if ( sr[l].langs[m]!=0 ) {
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

static void BuildMorxScript(struct node *node,struct att_dlg *att) {
    uint32 script = node->tag, lang = DEFAULT_LANG;
    int i,j,k,l,m, tot, max;
    SplineFont *_sf = att->sf, *sf;
    uint32 *feats;
    struct node *featnodes;
    extern GTextInfo *pst_tags[];
    int feat, set;
    SplineChar *sc;
    PST *pst;
    char buf[20];
    unichar_t ubuf[120];

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
			    OTTagToMacFeature(pst->tag,&feat,&set)) {
		int sli = pst->script_lang_index;
		struct script_record *sr = _sf->script_lang[sli];
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

    featnodes = gcalloc(tot+1,sizeof(struct node));
    for ( i=0; i<tot; ++i ) {
	/* featnodes[i].tag = feats[i]; */
	featnodes[i].parent = node;
	featnodes[i].build = BuildMorxFeatures;
	for ( k=1; pst_tags[k]!=NULL; ++k ) {
	    for ( j=0; pst_tags[k][j].text!=NULL && featnodes[i].tag!=(uint32) pst_tags[k][j].userdata; ++j );
	    if ( pst_tags[k][j].text!=NULL )
	break;
	}
	OTTagToMacFeature(pst->tag,&feat,&set);
	featnodes[i].tag = (feat<<16)|set;
	sprintf( buf, "%d,%d ", feat,set );
	uc_strcpy(ubuf,buf);
	if ( pst_tags[k]!=NULL )
	    u_strcat(ubuf,GStringGetResource((uint32) pst_tags[k][j].text,NULL));
	featnodes[i].label = u_copy(ubuf);
    }

    qsort(featnodes,tot,sizeof(struct node),compare_tag);
    for ( i=0; i<tot; ++i )
	featnodes[i].tag = MacFeatureToOTTag(featnodes[i].tag>>16,featnodes[i].tag&0xffff);
    node->children = featnodes;
    node->cnt = tot;
}

static void BuildGSUBfeatures(struct node *node,struct att_dlg *att) {
    int isgsub = node->parent->parent->parent->tag==CHR('G','S','U','B');
    uint32 script = node->parent->parent->tag, lang = node->parent->tag, feat=node->tag;
    AnchorClass *ac;

    if ( !isgsub ) {
	if ( feat==CHR('k','e','r','n')) {
	    if ( node->u.kc == (KernClass *) -1 ) {
		BuildKerns(node,att,script,lang,BuildKerns2G,false);
return;
	    } else if ( node->u.kc != NULL ) {
		BuildKC(node,att);
return;
	    }
	}
	if ( feat==CHR('v','k','r','n')) {
	    if ( node->u.kc == (KernClass *) -1 ) {
		BuildKerns(node,att,script,lang,BuildKerns2GV,true);
return;
	    } else if ( node->u.kc != NULL ) {
		BuildKC(node,att);
return;
	    }
	}
	for ( ac = att->sf->anchor; ac!=NULL; ac=ac->next ) {
	    if ( feat==ac->feature_tag ) {
		BuildAnchorLists(node,att,script);
return;
	    }
	}
    }

    BuildFeatures(node,att, script,lang,feat,!isgsub);
}

static void BuildGSUBlang(struct node *node,struct att_dlg *att) {
    int isgsub = node->parent->parent->tag==CHR('G','S','U','B');
    uint32 script = node->parent->tag, lang = node->tag;
    int i,j,k,l,m, tot, max;
    SplineFont *_sf = att->sf, *sf;
    uint32 *feats;
    struct node *featnodes;
    extern GTextInfo *pst_tags[];
    int haskerns=false, hasvkerns=false;
    AnchorClass *ac, *ac2;
    union sak *acs;
    SplineChar *sc;
    PST *pst;
    KernPair *kp;
    unichar_t ubuf[80];
    int isv;
    KernClass *kc;

    /* Build up the list of features in this lang entry of this script in GSUB/GPOS */
    k=tot=0;
    max=30;
    feats = galloc(max*sizeof(uint32));
    do {
	sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[k];
	for ( i=0; i<sf->charcnt; ++i ) if ( (sc=sf->chars[i])!=NULL ) {
	    for ( pst=sc->possub; pst!=NULL; pst=pst->next )
		    if ( (isgsub && pst->type!=pst_position && pst->type!=pst_lcaret &&
			    pst->type!=pst_pair) ||
			    (!isgsub && (pst->type==pst_position || pst->type==pst_pair))) {
		int sli = pst->script_lang_index;
		struct script_record *sr = _sf->script_lang[sli];
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
	    if ( !isgsub ) {
		for ( isv=0; isv<2; ++isv ) {
		    if ( isv && hasvkerns )
		continue;
		    if ( !isv && haskerns )
		continue;
		    for ( kp=isv ? sc->vkerns : sc->kerns; kp!=NULL; kp=kp->next ) {
			int sli = kp->sli;
			struct script_record *sr = _sf->script_lang[sli];
			for ( l=0; sr[l].script!=0 && sr[l].script!=script; ++l );
			if ( sr[l].script!=0 ) {
			    for ( m=0; sr[l].langs[m]!=0 && sr[l].langs[m]!=lang; ++m );
			    if ( sr[l].langs[m]!=0 ) {
				if ( tot>=max )
				    feats = grealloc(feats,(max+=30)*sizeof(uint32));
				if ( isv ) {
				    feats[tot++] = CHR('v','k','r','n');
				    hasvkerns = tot;	/* Yes, I know tot has been incremented. I need it to be non-zero */
				} else {
				    feats[tot++] = CHR('k','e','r','n');
				    haskerns = tot;	/* Yes, I know tot has been incremented. I need it to be non-zero */
				}
		    break;
			    }
			}
		    }
		}
	    }
	}
	++k;
    } while ( k<_sf->subfontcnt );
    acs = NULL;
    if ( !isgsub ) {
	acs = gcalloc(max,sizeof(union sak));
	AnchorClassOrder(_sf);
	for ( ac=_sf->anchor; ac!=NULL; ac=ac->next )
	    ac->processed = false;
	for ( ac=_sf->anchor; ac!=NULL; ac = ac->next ) if ( !ac->processed ) {
	    int sli = ac->script_lang_index;
	    struct script_record *sr = _sf->script_lang[sli];
	    for ( l=0; sr[l].script!=0 && sr[l].script!=script; ++l );
	    if ( sr[l].script!=0 ) {
		for ( m=0; sr[l].langs[m]!=0 && sr[l].langs[m]!=lang; ++m );
		if ( sr[l].langs[m]!=0 ) {
		    /* We expect to get multiple features with the same tag here */
		    if ( tot>=max ) {
			feats = grealloc(feats,(max+=30)*sizeof(uint32));
			acs = grealloc(acs,max*sizeof(union sak));
		    }
		    acs[tot].ac = ac;
		    feats[tot++] = ac->feature_tag;
		}
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
		struct script_record *sr = _sf->script_lang[kc->sli];
		for ( l=0; sr[l].script!=0 && sr[l].script!=script; ++l );
		if ( sr[l].script!=0 ) {
		    for ( m=0; sr[l].langs[m]!=0 && sr[l].langs[m]!=lang; ++m );
		    if ( sr[l].langs[m]!=0 ) {
			/* We expect to get multiple features with the same tag here */
			if ( tot>=max ) {
			    feats = grealloc(feats,(max+=30)*sizeof(uint32));
			    acs = grealloc(acs,max*sizeof(union sak));
			}
			acs[tot].kc = kc;
			feats[tot++] = isv ? CHR('v','k','r','n') : CHR('k','e','r','n');
		    }
		}
	    }
	}
    }

    featnodes = gcalloc(tot+1,sizeof(struct node));
    for ( i=0; i<tot; ++i ) {
	featnodes[i].tag = feats[i];
	featnodes[i].parent = node;
	featnodes[i].build = BuildGSUBfeatures;
	if ( feats[i]==REQUIRED_FEATURE ) {
	    u_strcpy(ubuf,GStringGetResource(_STR_RequiredFeature,NULL));
	} else {
	    for ( k=1; pst_tags[k]!=NULL; ++k ) {
		for ( j=0; pst_tags[k][j].text!=NULL && featnodes[i].tag!=(uint32) pst_tags[k][j].userdata; ++j );
		if ( pst_tags[k][j].text!=NULL )
	    break;
	    }
	    ubuf[0] = '\'';
	    ubuf[1] = featnodes[i].tag>>24;
	    ubuf[2] = (featnodes[i].tag>>16)&0xff;
	    ubuf[3] = (featnodes[i].tag>>8)&0xff;
	    ubuf[4] = featnodes[i].tag&0xff;
	    ubuf[5] = '\'';
	    ubuf[6] = ' ';
	    if ( pst_tags[k]!=NULL )
		u_strcpy(ubuf+7,GStringGetResource((uint32) pst_tags[k][j].text,NULL));
	    else
		ubuf[7]='\0';
	}
	if ( acs!=NULL )
	    featnodes[i].u = acs[i];
	featnodes[i].label = u_copy(ubuf);
    }

    /* We need to distinguish between kerns from our own kern pairs, and a */
    /*  real pair positioning feature the user added itself named 'kern' */
    if ( haskerns!=0 )
	featnodes[haskerns-1].u.kc = (KernClass *) -1;
    if ( hasvkerns!=0 )
	featnodes[hasvkerns-1].u.kc = (KernClass *) -1;

    qsort(featnodes,tot,sizeof(struct node),compare_tag);
    node->children = featnodes;
    node->cnt = tot;
}

static void BuildGSUBscript(struct node *node,struct att_dlg *att) {
    SplineFont *sf = att->sf;
    int lang_max;
    int i,j,k,l;
    struct node *langnodes;
    extern GTextInfo languages[];
    unichar_t ubuf[100];

    /* Build the list of languages that are used in this script */
    /* Don't bother to check whether they actually get used */
    for ( j=0; j<2; ++j ) {
	lang_max = 0;
	if ( sf->script_lang!=NULL )
	for ( i=0; sf->script_lang[i]!=NULL ; ++i ) {
	    for ( k=0; sf->script_lang[i][k].script!=0; ++k ) {
		if ( sf->script_lang[i][k].script == node->tag ) {
		    for ( l=0; sf->script_lang[i][k].langs[l]!=0; ++l ) {
			if ( j )
			    langnodes[lang_max].tag = sf->script_lang[i][k].langs[l];
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
	u_strcat(ubuf,GStringGetResource(_STR_OTFLanguage,NULL));
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
		if ( l<sf->charcnt && sf->chars[l]!=NULL ) {
		    sc = sf->chars[i];
	    break;
		}
		++l;
	    } while ( l<_sf->subfontcnt );
	    if ( sc!=NULL && SCWorthOutputting(sc) ) {
		if ( chars!=NULL ) {
		    int gdefc = gdefclass(sc);
		    sprintf(buffer,"%.70s %s", sc->name,
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
    l = 0;
    pst = NULL;
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
	    if ( pst!=NULL )
	break;
	}
	if ( pst!=NULL )
    break;
	++l;
    } while ( l<_sf->subfontcnt );

    gdef = lcar = 0;
    if ( ac!=NULL )
	gdef = 1;
    if ( pst!=NULL )
	lcar = 1;
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
    SplineFont *sf, *_sf = att->sf;
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

    /* Build the list of scripts that are mentioned in the font */
    for ( j=0; j<2; ++j ) {
	script_max = 0;
	if ( _sf->script_lang!=NULL )
	for ( i=0; _sf->script_lang[i]!=NULL ; ++i ) {
	    for ( k=0; _sf->script_lang[i][k].script!=0; ++k ) {
		if ( j )
		    scriptnodes[script_max].tag = _sf->script_lang[i][k].script;
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
		else
		    relevant = isgsub || (ismorx && OTTagToMacFeature(pst->tag,&feat,&set));
		if ( relevant ) {
		    int sli = pst->script_lang_index;
		    struct script_record *sr = _sf->script_lang[sli];
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
			struct script_record *sr = _sf->script_lang[sli];
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
	    for ( kc = isv ? _sf->vkerns : _sf->kerns; kc!=NULL; kc=kc->next ) {
		int sli = kc->sli;
		struct script_record *sr = _sf->script_lang[sli];
		for ( l=0; sr[l].script!=0 ; ++l ) {
		    for ( j=0; j<script_max && scriptnodes[j].tag!=sr[l].script; ++j );
		    if ( j<script_max )
			scriptnodes[j].used = true;
		}
	    }
	}
    }
		
    if ( isgpos && _sf->anchor!=NULL ) {
	for ( ac=_sf->anchor; ac!=NULL; ac=ac->next ) {
	    int sli = ac->script_lang_index;
	    struct script_record *sr = _sf->script_lang[sli];
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
	u_strcat(ubuf,GStringGetResource(_STR_OTFScript,NULL));
	scriptnodes[i].label = u_copy(ubuf);
	scriptnodes[i].build = iskern ? BuildKernScript :
				isvkern ? BuildVKernScript :
				ismorx ? BuildMorxScript :
			        BuildGSUBscript;
	scriptnodes[i].parent = node;
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
		    hasgsub = true;
		    if ( OTTagToMacFeature(pst->type,&feat,&set))
			hasmorx = true;
		}
	    }
	    if ( sc->kerns!=NULL && SCScriptFromUnicode(sc)!=0 ) {
		haskern = hasgpos = true;
		SFAddScriptLangIndex(sf,SCScriptFromUnicode(sc),DEFAULT_LANG);
	    }
	    if ( sc->vkerns!=NULL && SCScriptFromUnicode(sc)!=0 ) {
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

    if ( hasgsub+hasgpos+hasgdef+hasmorx+haskern+haslcar+hasopbd+hasprop==0 ) {
	tables = gcalloc(2,sizeof(struct node));
	tables[0].label = u_copy(GStringGetResource(_STR_NoAdvancedTypography,NULL));
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
	    tables[i].label = u_copy(GStringGetResource(_STR_AppleAdvancedTypography,NULL));
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
		tables[i].children[j].tag = CHR('o','p','b','d');
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

    node->lpos = lpos++;
    len = 5+8*depth+ att->as + 5 + GDrawGetTextWidth(att->v,node->label,-1,NULL);
    if ( len>att->maxl ) att->maxl = len;

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
	GDrawDrawText(pixmap,r.x+r.width+5,y,node->label,-1,NULL,fg);
	node = NodeNext(node,&depth);
	y += att->fh;
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
    unichar_t *ret = GWidgetSaveAsFile(GStringGetResource(_STR_Save,NULL),NULL,
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
	GWidgetErrorR(_STR_SaveFailed,_STR_SaveFailed,cret);
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
    GScrollBarSetBounds(att->vsb,0,att->maxl,att->page_width);

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
    wattrs.window_title = GStringGetResource(_STR_ShowAtt,NULL);
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,GGadgetScale(200));
    pos.height = GDrawPointsToPixels(NULL,300);
    att.gw = GDrawCreateTopWindow(NULL,&pos,att_e_h,&att,&wattrs);

    memset(&rq,'\0',sizeof(rq));
    rq.family_name = helv;
    rq.point_size = 12;
    rq.weight = 400;
    att.font = GDrawInstanciateFont(GDrawGetDisplayOfWindow(att.gw),&rq);
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

    gcd[2].gd.pos.width = GIntGetResource(_NUM_Buttonsize);
    gcd[2].gd.pos.x = (pos.width+sbsize-gcd[2].gd.pos.width)/2;
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
