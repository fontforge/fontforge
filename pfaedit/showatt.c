/* Copyright (C) 2000-2003 by George Williams */
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
    SplineChar *sc;
    int lpos;
};

struct att_dlg {
    unsigned int done: 1;
    struct node *tables;
    int open_cnt, lines_page, off_top, bmargin;
    SplineFont *sf;
    GWindow gw,v;
    GGadget *vsb, *cancel;
    int fh, as;
    GFont *font;
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

static void BuildAnchorLists(struct node *node,struct att_dlg *att,uint32 script, AnchorClass *ac) {
}

static void BuildKerns2(struct node *node,struct att_dlg *att) {
    KernPair *kp = node->sc->kerns;
    int i;
    struct node *chars;
    char buf[80];

    for ( kp=node->sc->kerns, i=0; kp!=NULL; kp=kp->next ) ++i;
    node->children = chars = gcalloc(i+1,sizeof(struct node));
    node->cnt = i;
    for ( kp=node->sc->kerns, i=0; kp!=NULL; kp=kp->next, ++i ) {
	sprintf( buf, "%.40s %d", kp->sc->name, kp->off );
	chars[i].label = uc_copy(buf);
	chars[i].parent = node;
    }
}

static void BuildKerns(struct node *node,struct att_dlg *att,uint32 script) {
    int i,k,j, tot;
    SplineFont *_sf = att->sf, *sf;
    struct node *chars;

    for ( j=0; j<2; ++j ) {
	tot = 0;
	k=0;
	do {
	    sf = _sf->subfontcnt==0 ? _sf : _sf->subfonts[k++];
	    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL && sf->chars[i]->kerns!=NULL &&
		    SCScriptFromUnicode(sf->chars[i])==script ) {
		if ( j ) {
		    chars[tot].sc = sf->chars[i];
		    chars[tot].label = uc_copy(sf->chars[i]->name);
		    chars[tot].build = BuildKerns2;
		    chars[tot].parent = node;
		}
		++tot;
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
    BuildKerns(node,att,node->tag);
}

static void BuildFeatures(struct node *node,struct att_dlg *att,
	uint32 script, uint32 lang, uint32 tag) {
    int i,j,k,l,m, maxc, tot, maxl, len;
    SplineFont *_sf = att->sf, *sf;
    SplineChar *sc;
    unichar_t *ubuf;
    char buf[60];
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
		for ( pst=sc->possub; pst!=NULL; pst=pst->next ) if ( pst->tag==tag && pst->type!=pst_lcaret ) {
		    int sli = pst->script_lang_index;
		    struct script_record *sr = _sf->script_lang[sli];
		    for ( l=0; sr[l].script!=0 && sr[l].script!=script; ++l );
		    if ( sr[l].script!=0 ) {
			for ( m=0; sr[l].langs[m]!=0 && sr[l].langs[m]!=lang; ++m );
			if ( sr[l].langs[m]!=0 ) {
			    if ( chars ) {
				uc_strcpy(ubuf,sc->name);
			        if ( pst->type==pst_position ) {
				    sprintf(buf,"dx=%d dy=%d dx_adv=%d dy_adv=%d",
					    pst->u.pos.xoff, pst->u.pos.yoff,
					    pst->u.pos.h_adv_off, pst->u.pos.v_adv_off );
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
    BuildFeatures(node,att, script,lang,feat);
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
	    BuildKerns(node,att,script);
return;
	}
	for ( ac = att->sf->anchor; ac!=NULL; ac=ac->next ) {
	    if ( feat==ac->feature_tag ) {
		BuildAnchorLists(node,att,script,ac);
return;
	    }
	}
    }

    BuildFeatures(node,att, script,lang,feat);
}

static void BuildGSUBlang(struct node *node,struct att_dlg *att) {
    int isgsub = node->parent->parent->tag==CHR('G','S','U','B');
    uint32 script = node->parent->tag, lang = node->tag;
    int i,j,k,l,m, tot, max;
    SplineFont *_sf = att->sf, *sf;
    uint32 *feats;
    struct node *featnodes;
    extern GTextInfo *pst_tags[];
    int haskerns=false;
    AnchorClass *ac;
    SplineChar *sc;
    PST *pst;
    unichar_t ubuf[80];

    /* Build up the list of features in this lang entry of this script in GSUB/GPOS */
    k=tot=0;
    max=30;
    feats = galloc(max*sizeof(uint32));
    do {
	sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[k];
	for ( i=0; i<sf->charcnt; ++i ) if ( (sc=sf->chars[i])!=NULL ) {
	    for ( pst=sc->possub; pst!=NULL; pst=pst->next )
		    if ( (isgsub && pst->type!=pst_position && pst->type!=pst_lcaret ) ||
			    (!isgsub && pst->type==pst_position)) {
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
	    if ( !isgsub && lang==CHR('d','f','l','t') && !haskerns &&
		    SCScriptFromUnicode(sc)==script ) {
		haskerns = true;
		if ( tot>=max )
		    feats = grealloc(feats,(max+=30)*sizeof(uint32));
		feats[tot++] = CHR('k','e','r','n');
	    }
	}
	++k;
    } while ( k<_sf->subfontcnt );
    if ( !isgsub ) {
	for ( ac=_sf->anchor; ac!=NULL; ac=ac->next ) {
	    int sli = ac->script_lang_index;
	    struct script_record *sr = _sf->script_lang[sli];
	    for ( l=0; sr[l].script!=0 && sr[l].script!=script; ++l );
	    if ( sr[l].script!=0 ) {
		for ( m=0; sr[l].langs[m]!=0 && sr[l].langs[m]!=lang; ++m );
		if ( sr[l].langs[m]!=0 ) {
		    for ( l=0; l<tot && feats[l]!=ac->feature_tag; ++l );
		    if ( l>=tot ) {
			if ( tot>=max )
			    feats = grealloc(feats,(max+=30)*sizeof(uint32));
			feats[tot++] = ac->feature_tag;
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
	featnodes[i].label = u_copy(ubuf);
    }

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
	if ( languages[j].text!=NULL )
	    u_strcpy(ubuf+7,GStringGetResource((uint32) languages[j].text,NULL));
	else
	    ubuf[7]='\0';
	langnodes[i].label = u_copy(ubuf);
	langnodes[i].build = BuildGSUBlang;
	langnodes[i].parent = node;
    }
    node->children = langnodes;
    node->cnt = i;
}

static void BuildTable(struct node *node,struct att_dlg *att) {
    SplineFont *sf, *_sf = att->sf;
    int script_max;
    int i,j,k,l;
    struct node *scriptnodes;
    extern GTextInfo scripts[];
    int isgsub = node->tag==CHR('G','S','U','B'), isgpos = node->tag==CHR('G','P','O','S');
    int ismorx = node->tag==CHR('m','o','r','x'), iskern = node->tag==CHR('k','e','r','n');
    int feat, set;
    SplineChar *sc;
    PST *pst;
    unichar_t ubuf[120];

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
		if ( pst->type == pst_position )
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
	    if ( sc->kerns!=NULL && (iskern || isgpos)) {
		uint32 script = SCScriptFromUnicode(sc);
		for ( j=0; j<script_max && scriptnodes[j].tag!=script; ++j );
		if ( j<script_max )
		    scriptnodes[j].used = true;
	    }
	}
	++k;
    } while ( k<_sf->subfontcnt );
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
	if ( scripts[j].text!=NULL )
	    u_strcpy(ubuf+7,GStringGetResource((uint32) scripts[j].text,NULL));
	else
	    ubuf[7]='\0';
	scriptnodes[i].label = u_copy(ubuf);
	scriptnodes[i].build = iskern ? BuildKernScript :
				ismorx ? BuildMorxScript :
			        BuildGSUBscript;
	scriptnodes[i].parent = node;
    }
    node->children = scriptnodes;
    node->cnt = i;
}

static void BuildTop(struct att_dlg *att) {
    SplineFont *sf, *_sf = att->sf;
    int hasgsub=0, hasgpos=0, hasmorx=0, haskern=0;
    int feat, set;
    struct node *tables;
    PST *pst;
    SplineChar *sc;
    int i,k;

    k=0;
    do {
	sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[k];
	for ( i=0; i<sf->charcnt; ++i ) if ( (sc=sf->chars[i])!=NULL ) {
	    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) if ( pst->type!=pst_lcaret ) {
		if ( pst->type == pst_position )
		    hasgpos = true;
		else {
		    hasgsub = true;
		    if ( OTTagToMacFeature(pst->type,&feat,&set))
			hasmorx = true;
		}
	    }
	    if ( sc->kerns!=NULL && SCScriptFromUnicode(sc)!=0 ) {
		haskern = hasgpos = true;
		SFAddScriptLangIndex(sf,SCScriptFromUnicode(sc),DEFAULT_LANG);
	    }
	}
	++k;
    } while ( k<_sf->subfontcnt );

    if ( hasgsub+hasgpos+hasmorx+haskern==0 ) {
	tables = gcalloc(2,sizeof(struct node));
	tables[0].label = u_copy(GStringGetResource(_STR_NoAdvancedTypography,NULL));
    } else {
	tables = gcalloc((hasgsub||hasgpos)+(hasmorx||haskern)+1,sizeof(struct node));
	i=0;
	if ( hasgsub || hasgpos ) {
	    tables[i].label = uc_copy("OTF");
	    tables[i].children_checked = true;
	    tables[i].children = gcalloc(hasgsub+hasgpos+1,sizeof(struct node));
	    tables[i].cnt = hasgsub + hasgpos;
	    if ( hasgsub ) {
		tables[i].children[0].label = uc_copy("GSUB");
		tables[i].children[0].tag = CHR('G','S','U','B');
		tables[i].children[0].build = BuildTable;
		tables[i].children[0].parent = &tables[i];
	    }
	    if ( hasgpos ) {
		tables[i].children[hasgsub].label = uc_copy("GPOS");
		tables[i].children[hasgsub].tag = CHR('G','P','O','S');
		tables[i].children[hasgsub].build = BuildTable;
		tables[i].children[hasgsub].parent = &tables[i];
	    }
	    ++i;
	}
	if ( hasmorx || haskern ) {
	    tables[i].label = u_copy(GStringGetResource(_STR_AppleAdvancedTypography,NULL));
	    tables[i].children_checked = true;
	    tables[i].children = gcalloc(hasmorx+haskern+1,sizeof(struct node));
	    tables[i].cnt = hasmorx+haskern;
	    if ( hasmorx ) {
		tables[i].children[0].label = uc_copy("GSUB");
		tables[i].children[0].tag = CHR('m','o','r','x');
		tables[i].children[0].build = BuildTable;
		tables[i].children[0].parent = &tables[i];
	    }
	    if ( haskern ) {
		tables[i].children[hasmorx].label = uc_copy("kern");
		tables[i].children[hasmorx].tag = CHR('k','e','r','n');
		tables[i].children[hasmorx].build = BuildTable;
		tables[i].children[hasmorx].parent = &tables[i];
	    }
	    ++i;
	}
    }

    att->tables = tables;
}

static int _SizeCnt(struct att_dlg *att,struct node *node, int lpos) {
    int i;

    node->lpos = lpos++;
    if ( node->open ) {
	if ( !node->children_checked && node->build!=NULL ) {
	    (node->build)(node,att);
	    node->children_checked = true;
	}
	for ( i=0; i<node->cnt; ++i )
	    lpos = _SizeCnt(att,&node->children[i],lpos);
    }
return( lpos );
}

static int SizeCnt(struct att_dlg *att,struct node *node, int lpos) {
    while ( node->label ) {
	lpos = _SizeCnt(att,node,lpos);
	++node;
    }
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

static void AttExpose(struct att_dlg *att,GWindow pixmap,GRect *rect) {
    int depth, y;
    struct node *node;
    GRect r;

    GDrawFillRect(pixmap,rect,GDrawGetDefaultBackground(NULL));

    r.height = r.width = att->as;
    y = (rect->y/att->fh) * att->fh + att->as;
    depth=0;
    node = NodeFindLPos(att->tables,rect->y/att->fh+att->off_top,&depth);
    GDrawSetFont(pixmap,att->font);
    while ( node!=NULL ) {
	r.y = y-att->as+1;
	r.x = 5+8*depth;
	if ( node->build || node->children ) {
	    GDrawDrawRect(pixmap,&r,0x000000);
	    GDrawDrawLine(pixmap,r.x+2,r.y+att->as/2,r.x+att->as-2,r.y+att->as/2,
		    0x000000);
	    if ( !node->open )
		GDrawDrawLine(pixmap,r.x+att->as/2,r.y+2,r.x+att->as/2,r.y+att->as-2,
			0x000000);
	}
	GDrawDrawText(pixmap,r.x+r.width+5,y,node->label,-1,NULL,0x000000);
	node = NodeNext(node,&depth);
	y += att->fh;
    }
}

static void AttMouse(struct att_dlg *att,GEvent *event) {
    int l, depth, cnt;
    struct node *node;
    GRect r;

    if ( event->type!=et_mouseup )
return;
    l = (event->u.mouse.y/att->fh);
    if ( event->u.mouse.y > l*att->fh+att->as )
return;			/* Not in +/- rectangle */
    depth=0;
    node = NodeFindLPos(att->tables,l+att->off_top,&depth);
    if ( event->u.mouse.x<5+8*depth || event->u.mouse.x>=5+8*depth+att->as || node==NULL )
return;
    node->open = !node->open;

    cnt = SizeCnt(att,att->tables,0);
    GScrollBarSetBounds(att->vsb,0,cnt,att->lines_page);
    att->open_cnt = cnt;

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

static void AttResize(struct att_dlg *att,GEvent *event) {
    GRect size, wsize;
    int lcnt;
    int sbsize = GDrawPointsToPixels(att->gw,_GScrollBar_Width);

    GDrawGetSize(att->gw,&size);
    lcnt = (size.height-att->bmargin)/att->fh;
    GGadgetResize(att->vsb,sbsize,lcnt*att->fh);
    GGadgetMove(att->vsb,size.width-sbsize,0);
    GDrawResize(att->v,size.width-sbsize,lcnt*att->fh);
    att->lines_page = lcnt;
    GScrollBarSetBounds(att->vsb,0,att->open_cnt,att->lines_page);

    GGadgetGetSize(att->cancel,&wsize);
    GGadgetMove(att->cancel,(size.width-wsize.width)/2,size.height-att->bmargin+5);
    GDrawRequestExpose(att->v,NULL,true);
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
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("showatt.html");
return( true );
	}
return( false );
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
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("showatt.html");
return( true );
	}
return( false );
      break;
      case et_resize:
	if ( event->u.resize.sized )
	    AttResize(att,event);
      break;
      case et_controlevent:
	switch ( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    AttScroll(att,&event->u.control.u.sb);
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

    att.bmargin = GDrawPointsToPixels(NULL,30);

    att.lines_page = (pos.height-att.bmargin)/att.fh;
    wattrs.mask = wam_events|wam_cursor|wam_bordwidth;
    wattrs.border_width = 1;
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

    gcd[1].gd.pos.width = GIntGetResource(_NUM_Buttonsize);
    gcd[1].gd.pos.x = (pos.width+sbsize-gcd[1].gd.pos.width)/2; gcd[1].gd.pos.y = pos.height+7;
    gcd[1].gd.flags = gg_visible | gg_enabled | gg_but_default | gg_but_cancel | gg_pos_in_pixels;
    label[1].text = (unichar_t *) _STR_OK;
    label[1].text_in_resource = true;
    gcd[1].gd.label = &label[1];
    gcd[1].creator = GButtonCreate;

    GGadgetsCreate(att.gw,gcd);
    att.vsb = gcd[0].ret;
    att.cancel = gcd[1].ret;

    BuildTop(&att);
    att.open_cnt = SizeCnt(&att,att.tables,0);
    GScrollBarSetBounds(att.vsb,0,att.open_cnt,att.lines_page);

    GDrawSetVisible(att.gw,true);

    while ( !att.done )
	GDrawProcessOneEvent(NULL);
    nodesfree(att.tables);
    GDrawDestroyWindow(att.gw);
}
