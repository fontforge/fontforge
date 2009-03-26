/* Copyright (C) 2003-2009 by George Williams */
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
#include <gkeysym.h>

/* ************************************************************************** */
/* ************************ Context/Chaining Dialog ************************* */
/* ************************************************************************** */
struct contextchaindlg {
    struct gfi_data *gfi;
    SplineFont *sf;
    FPST *fpst;
    unichar_t *newname;
    int isnew;
    GWindow gw;
    GWindow formats, coverage, glist, glyphs, cselect, class, classbuild, classnumber;
    enum activewindow { aw_formats, aw_coverage, aw_glist, aw_glyphs, aw_cselect,
	    aw_class, aw_classbuild, aw_classnumber } aw;
    uint16 wasedit;
    uint16 wasoffset;
    uint8 foredefault;
    int subheightdiff, canceldrop;
};

#define CID_OK		100
#define CID_Cancel	101
#define CID_Next	102
#define CID_Prev	103
#define CID_SubSize	104

#define CID_ByGlyph	200
#define CID_ByClass	201
#define CID_ByCoverage	202

/* There are more CIDs used in this file than those listed here */
/* The CIDs given here are for glyph lists (aw_glyphs & aw_glist) */
/*  similar controls refering to coverage tables have 100 added to them */
/* The CIDs here are also for the "match" aspect of the tabsets */
/*  those in backtrack get 20 added, those in lookahead get 40 */
#define CID_New		300
#define CID_Edit	301
#define CID_Delete	302
#define CID_Up		303
#define CID_Down	304
#define CID_GList	305

#define CID_MatchType	1003
#define CID_Set		1004
#define CID_Select	1005
#define CID_GlyphList	1006
#define CID_Set2	1007
#define CID_Select2	1008
#define CID_RplList	1009
#define CID_LookupList	1010
#define CID_LNew	1011
#define CID_LEdit	1012
#define CID_LDelete	1013
#define CID_LUp		1014
#define CID_LDown	1015

#define CID_StdClasses	1016

#define CID_ClassNumbers	2000
#define CID_ClassList		2001
#define CID_ClassType		2002
#define CID_SameAsClasses	2003

GTextInfo stdclasses[] = {
    { (unichar_t *) N_("None"), NULL, 0, 0, "None", NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("0-9"), NULL, 0, 0, "0-9", NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Letters in Script(s)"), NULL, 0, 0, "Letters", NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lower Case"), NULL, 0, 0, "LC", NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Upper Case"), NULL, 0, 0, "UC", NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Glyphs in Script(s)"), NULL, 0, 0, "Glyphs", NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Added"), NULL, 0, 0, "None", NULL, 1, 0, 0, 0, 0, 0, 1},
    { NULL }
};

char *cu_copybetween(const unichar_t *start, const unichar_t *end) {
    char *ret = galloc(end-start+1);
    cu_strncpy(ret,start,end-start);
    ret[end-start] = '\0';
return( ret );
}

static char *ccd_cu_copy(const unichar_t *start) {
    char *ret, *pt;

    while ( isspace(*start)) ++start;
    if ( *start=='\0' )
return( NULL );
    ret = pt = galloc(u_strlen(start)+1);
    while ( *start ) {
	while ( !isspace(*start) && *start!='\0' )
	    *pt++ = *start++;
	while ( isspace(*start)) ++start;
	if ( *start!='\0' ) *pt++ = ' ';
    }
    *pt = '\0';
return( ret );
}

static char *reversenames(char *str) {
    char *ret;
    char *rpt, *pt, *start, *spt;

    if ( str==NULL )
return( NULL );

    rpt = ret = galloc(strlen(str)+1);
    *ret = '\0';
    for ( pt=str+strlen(str); pt>str; pt=start ) {
	for ( start = pt-1; start>=str && *start!=' '; --start );
	for ( spt=start+1; spt<pt; )
	    *rpt++ = *spt++;
	*rpt++ = ' ';
    }
    if ( rpt>ret )
	rpt[-1] = '\0';
return( ret );
}

static unichar_t *classnumbers(int cnt,uint16 *classes) {
    char buf[20];
    int i, len;
    unichar_t *pt, *ret;

    len = 0;
    for ( i=0; i<cnt; ++i ) {
	sprintf( buf, "%d ", classes[i]);
	len += strlen(buf);
    }
    ret = pt = galloc((len+3)*sizeof(unichar_t));
    *pt = '\0';		/* In case it is empty */

    for ( i=0; i<cnt; ++i ) {
	sprintf( buf, "%d ", classes[i]);
	uc_strcpy(pt,buf);
	pt += strlen(buf);
    }
return( ret );
}

static unichar_t *rclassnumbers(int cnt,uint16 *classes) {
    char buf[20];
    int i, len;
    unichar_t *pt, *ret;

    len = 0;
    for ( i=0; i<cnt; ++i ) {
	sprintf( buf, "%d ", classes[i]);
	len += strlen(buf);
    }
    ret = pt = galloc((len+3)*sizeof(unichar_t));
    *pt = '\0';
    for ( i=cnt-1; i>=0; --i ) {
	sprintf( buf, "%d ", classes[i]);
	uc_strcpy(pt,buf);
	pt += strlen(buf);
    }
return( ret );
}

static int CCD_GlyphNameCnt(const unichar_t *pt) {
    int cnt = 0;

    while ( *pt ) {
	while ( isspace( *pt )) ++pt;
	if ( *pt=='\0' )
return( cnt );
	++cnt;
	while ( !isspace(*pt) && *pt!='\0' ) ++pt;
    }
return( cnt );
}

static int seqlookuplen(struct fpst_rule *r) {
    int i, len=0;
    char buf[20];

    ++len;
    for ( i=0; i<r->lookup_cnt; ++i ) {
	sprintf( buf," %d \"\",", r->lookups[i].seq );
	len += strlen(buf) + utf8_strlen( r->lookups[i].lookup->lookup_name );
    }
return( len );
}

static unichar_t *addseqlookups(unichar_t *pt, struct fpst_rule *r) {
    int i;
    char buf[20];

    *pt++ = 0x21d2;
    for ( i=0; i<r->lookup_cnt; ++i ) {
	unichar_t *temp;
	sprintf( buf," %d \"", r->lookups[i].seq );
	uc_strcpy(pt, buf);
	pt += u_strlen(pt);
	temp = utf82u_copy(r->lookups[i].lookup->lookup_name );
	u_strcpy(pt,temp);
	pt += u_strlen(pt);
	*pt++ = '"';
	*pt++ = ',';
    }
    if ( pt[-1]==',' ) pt[-1] = '\0';
    *pt = '\0';
return( pt );
}

static void parseseqlookups(SplineFont *sf, const unichar_t *solooks, struct fpst_rule *r) {
    int cnt;
    const unichar_t *pt;

    for ( pt = solooks, cnt=0; *pt!='\0'; ) {
	++cnt;
	while ( *pt!='"' && *pt!='\0' ) ++pt;
	if ( *pt=='"' ) {
	    ++pt;
	    while ( *pt!='"' && *pt!='\0' ) ++pt;
	    if ( *pt=='"' ) ++pt;
	}
	if ( *pt==',' ) ++pt;
    }
    r->lookup_cnt = cnt;
    r->lookups = gcalloc(cnt,sizeof(struct seqlookup));
    cnt = 0;
    pt = solooks;
    forever {
	unichar_t *end;
	r->lookups[cnt].seq = u_strtol(pt,&end,10);
	pt = end+1;
	if ( *pt=='"' ) {
	    const unichar_t *eoname; char *temp;
	    ++pt;
	    for ( eoname = pt; *eoname!='\0' && *eoname!='"'; ++eoname );
	    temp = u2utf8_copyn(pt,eoname-pt);
	    r->lookups[cnt].lookup = SFFindLookup(sf,temp);
	    if ( r->lookups[cnt].lookup==NULL )
		IError("No lookup in parseseqlookups");
	    free(temp);
	    pt = eoname;
	    if ( *pt=='"' ) ++pt;
	}
	++cnt;
	if ( *pt!=',' )
    break;
	++pt;
    }
}

static unichar_t *glistitem(struct fpst_rule *r) {
    unichar_t *ret, *pt;
    int len;

    len = (r->u.glyph.back==NULL ? 0 : strlen(r->u.glyph.back)) +
	    strlen(r->u.glyph.names) +
	    (r->u.glyph.fore==0 ? 0 : strlen(r->u.glyph.fore)) +
	    seqlookuplen(r);

    ret = pt = galloc((len+8) * sizeof(unichar_t));
    if ( r->u.glyph.back!=NULL && *r->u.glyph.back!='\0' ) {
	char *temp = reversenames(r->u.glyph.back);
	uc_strcpy(pt,temp);
	pt += strlen(temp);
	free(temp);
	*pt++ = ' ';
    }
    *pt++ = '[';
    *pt++ = ' ';
    uc_strcpy(pt,r->u.glyph.names);
    pt += strlen(r->u.glyph.names);
    *pt++ = ' ';
    *pt++ = ']';
    *pt++ = ' ';
    if ( r->u.glyph.fore!=NULL  && *r->u.glyph.fore!='\0' ) {
	uc_strcpy(pt,r->u.glyph.fore);
	pt += strlen(r->u.glyph.fore);
	*pt++ = ' ';
    }
    pt = addseqlookups(pt, r);
return( ret );
}

static void glistitem2rule(SplineFont *sf, const unichar_t *ret,struct fpst_rule *r) {
    const unichar_t *pt;
    char *temp;

    for ( pt=ret; *pt!='\0' && *pt!='['; ++pt );
    if ( *pt=='\0' )
return;
    if ( pt>ret ) {
	temp = cu_copybetween(ret,pt-1);
	r->u.glyph.back = reversenames(temp);
	free(temp);
    }
    ret = pt+2;
    for ( pt=ret; *pt!='\0' && *pt!=']'; ++pt );
    if ( *pt=='\0' )
return;
    r->u.glyph.names = cu_copybetween(ret,pt-1);
    ret = pt+2;
    for ( pt=ret; *pt!='\0' && *pt!=0x21d2; ++pt );
    if ( *pt=='\0' )
return;
    if ( pt!=ret )
	r->u.glyph.fore = cu_copybetween(ret,pt-1);
    parseseqlookups(sf,pt+2,r);
}

static GTextInfo **glistlist(FPST *fpst) {
    GTextInfo **ti;
    int i;

    if ( fpst->format!=pst_glyphs || fpst->rule_cnt==0 )
return(NULL);
    ti = galloc((fpst->rule_cnt+1)*sizeof(GTextInfo *));
    ti[fpst->rule_cnt] = gcalloc(1,sizeof(GTextInfo));
    for ( i=0; i<fpst->rule_cnt; ++i ) {
	ti[i] = gcalloc(1,sizeof(GTextInfo));
	ti[i]->text = glistitem(&fpst->rules[i]);
	ti[i]->fg = ti[i]->bg = COLOR_DEFAULT;
    }
return( ti );
}

static unichar_t *clslistitem(struct fpst_rule *r) {
    unichar_t *ret, *pt;
    int len, i, k;
    char buf[20];

    len = 0;
    for ( i=0; i<3; ++i ) {
	for ( k=0; k<(&r->u.class.ncnt)[i]; ++k ) {
	    sprintf( buf, "%d ", (&r->u.class.nclasses)[i][k]);
	    len += strlen(buf);
	}
    }

    ret = pt = galloc((len+8+seqlookuplen(r)) * sizeof(unichar_t));
    for ( k=r->u.class.bcnt-1; k>=0; --k ) {
	sprintf( buf, "%d ", r->u.class.bclasses[k]);
	uc_strcpy(pt,buf);
	pt += strlen(buf);
    }
    *pt++ = '[';
    for ( k=0; k<r->u.class.ncnt; ++k ) {
	sprintf( buf, "%d ", r->u.class.nclasses[k]);
	uc_strcpy(pt,buf);
	pt += strlen(buf);
    }
    if ( pt[-1]==' ' ) --pt;
    *pt++ = ']';
    for ( k=0; k<r->u.class.fcnt; ++k ) {
	sprintf( buf, " %d", r->u.class.fclasses[k]);
	uc_strcpy(pt,buf);
	pt += strlen(buf);
    }

    *pt++ = ' ';
    pt = addseqlookups(pt, r);
return( ret );
}

static void clslistitem2rule(SplineFont *sf,const unichar_t *ret,struct fpst_rule *r) {
    const unichar_t *pt; unichar_t *end;
    int len, i;

    memset(r,0,sizeof(*r));

    len = 0; i=1;
    for ( pt=ret; *pt; ++pt ) {
	while ( *pt!='[' && *pt!=']' && *pt!=0x21d2 && *pt!='\0' ) {
	    if ( isdigit( *pt )) {
		++len;
		while ( isdigit(*pt)) ++pt;
	    } else if ( *pt!=' ' && *pt!='[' && *pt!=']' && *pt!=0x21d2 && *pt!='\0' )
		++pt;
	    while ( *pt==' ' ) ++pt;
	}
	(&r->u.class.ncnt)[i] = len;
	(&r->u.class.nclasses)[i] = galloc(len*sizeof(uint16));
	len = 0;
	if ( *pt=='\0' || *pt==0x21d2 )
    break;
	if ( *pt=='[' ) i=0;
	else i=2;
    }
    len = 0; i=1;
    for ( pt=ret; *pt; ++pt ) {
	while ( *pt!='[' && *pt!=']' && *pt!=0x21d2 && *pt!='\0' ) {
	    if ( isdigit( *pt )) {
		if ( len>=(&r->u.class.ncnt)[i] )
		    IError( "clslistitem2rule bad memory");
		(&r->u.class.nclasses)[i][len++] = u_strtol(pt,&end,10);
		pt = end;
	    } else if ( *pt!=' ' && *pt!='[' && *pt!=']' && *pt!=0x21d2 && *pt!='\0' )
		++pt;
	    while ( *pt==' ' ) ++pt;
	}
	len = 0;
	if ( *pt=='[' ) i=0;
	else i=2;
	if ( *pt=='\0' || *pt==0x21d2 )
    break;
    }
    /* reverse the backtrack */
    for ( i=0; i<r->u.class.bcnt/2; ++i ) {
	int temp = r->u.class.bclasses[i];
	r->u.class.bclasses[i] = r->u.class.bclasses[r->u.class.bcnt-1-i];
	r->u.class.bclasses[r->u.class.bcnt-1-i] = temp;
    }

    if ( *pt=='\0' || pt[1]=='\0' )
return;
    parseseqlookups(sf,pt+2,r);
}

static GTextInfo **clslistlist(FPST *fpst) {
    GTextInfo **ti;
    int i;

    if ( fpst->format!=pst_class || fpst->rule_cnt==0 )
return(NULL);
    ti = galloc((fpst->rule_cnt+1)*sizeof(GTextInfo *));
    ti[fpst->rule_cnt] = gcalloc(1,sizeof(GTextInfo));
    for ( i=0; i<fpst->rule_cnt; ++i ) {
	ti[i] = gcalloc(1,sizeof(GTextInfo));
	ti[i]->text = clslistitem(&fpst->rules[i]);
	ti[i]->fg = ti[i]->bg = COLOR_DEFAULT;
    }
return( ti );
}

static GTextInfo **clistlist(FPST *fpst,int which) {
    GTextInfo **ti;
    int cnt, i;

    if ( (fpst->format!=pst_coverage && fpst->format!=pst_reversecoverage) ||
	    fpst->rule_cnt!=1 )
return(NULL);
    cnt = (&fpst->rules[0].u.coverage.ncnt)[which];
    if ( cnt == 0 )
return( NULL );
    ti = galloc((cnt+1)*sizeof(GTextInfo *));
    ti[cnt] = gcalloc(1,sizeof(GTextInfo));
    if ( which != 1 ) {
	for ( i=0; i<cnt; ++i ) {
	    ti[i] = gcalloc(1,sizeof(GTextInfo));
	    ti[i]->text = uc_copy((&fpst->rules[0].u.coverage.ncovers)[which][i]);
	    ti[i]->fg = ti[i]->bg = COLOR_DEFAULT;
	}
    } else {
	for ( i=0; i<cnt; ++i ) {
	    ti[i] = gcalloc(1,sizeof(GTextInfo));
	    ti[i]->text = uc_copy(fpst->rules[0].u.coverage.bcovers[cnt-i-1]);
	    ti[i]->fg = ti[i]->bg = COLOR_DEFAULT;
	}
    }
return( ti );
}

static GTextInfo **slistlist(struct fpst_rule *r) {
    GTextInfo **ti;
    char buf[20];
    int i;

    if ( r->lookup_cnt==0 )
return(NULL);
    ti = galloc((r->lookup_cnt+1)*sizeof(GTextInfo *));
    ti[r->lookup_cnt] = gcalloc(1,sizeof(GTextInfo));
    for ( i=0; i<r->lookup_cnt; ++i ) {
	unichar_t *space;
	ti[i] = gcalloc(1,sizeof(GTextInfo));
	sprintf( buf,"%d ", r->lookups[i].seq );
	ti[i]->text = space = galloc((strlen(buf)+utf8_strlen(r->lookups[i].lookup->lookup_name)+2)*sizeof(unichar_t));
	uc_strcpy(space,buf);
	utf82u_strcat(space,r->lookups[i].lookup->lookup_name);
	ti[i]->fg = ti[i]->bg = COLOR_DEFAULT;
    }
return( ti );
}

static void CCD_ParseLookupList(SplineFont *sf, struct fpst_rule *r,GGadget *list) {
    int32 len, i;
    GTextInfo **ti = GGadgetGetList(list,&len);
    unichar_t *end;
    char *name;

    r->lookup_cnt = len;
    r->lookups = galloc(len*sizeof(struct seqlookup));
    for ( i=0; i<len; ++i ) {
	r->lookups[i].seq = u_strtol(ti[i]->text,&end,10);
	while ( *end==' ') ++end;
	if ( *end=='"' ) ++end;
	name = u2utf8_copy(end);
	if ( name[strlen(name)-1] == '"' )
	    name[strlen(name)-1] = '\0';
	r->lookups[i].lookup = SFFindLookup(sf,name);
	if ( r->lookups[i].lookup==NULL )
	    IError("Failed to find lookup in CCD_ParseLookupList");
	free(name);
    }
}

static void CCD_EnableNextPrev(struct contextchaindlg *ccd) {
    /* Cancel is always enabled */
    switch ( ccd->aw ) {
      case aw_formats:
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_Prev),false);
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_Next),true);
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_OK),false);
      break;
      case aw_glyphs:
      case aw_classbuild:
      case aw_classnumber:
      case aw_cselect:
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_Prev),true);
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_Next),true);
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_OK),false);
      break;
      case aw_coverage:
      case aw_glist:
      case aw_class:
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_Prev),ccd->fpst->format!=pst_reversecoverage);
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_Next),false);
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_OK),true);
      break;
      default:
	IError("Can't get here");
      break;
    }
}

static int CCD_ValidNameList(const char *ret,int empty_bad) {
    int first;
    extern int allow_utf8_glyphnames;

    while ( isspace(*ret)) ++ret;
    if ( *ret=='\0' )
return( !empty_bad );

    first = true;
    while ( *ret ) {
	if ( (*(unsigned char *) ret>=0x7f && !allow_utf8_glyphnames) || *ret<' ' )
return( false );
	if ( first && isdigit(*ret) )
return( false );
	first = isspace(*ret);
	if ( *ret=='(' || *ret=='[' || *ret=='{' || *ret=='<' ||
		*ret==')' || *ret==']' || *ret=='}' || *ret=='>' ||
		*ret=='%' || *ret=='/' )
return( false );
	++ret;
    }
return( true );
}

static char *CCD_NonExistantName(SplineFont *sf,const char *ret) {
    const char *pt, *end; char *name;

    for ( pt = ret; *pt; pt = end ) {
	while ( *pt==' ' ) ++pt;
	if ( *pt=='\0' )
    break;
	end = strchr(pt,' ');
	if ( end==NULL ) end = pt+strlen(pt);
	name = copyn(pt,end-pt);
	if ( SFGetChar(sf,-1,name)==NULL )
return( name );
	free(name);
    }
return( NULL );
}

int CCD_NameListCheck(SplineFont *sf,const char *ret,int empty_bad,char *title) {
    char *missingname;
    int ans;

    if ( !CCD_ValidNameList(ret,empty_bad) ) {
	ff_post_error(title,
		strcmp(title,_("Bad Class"))==0? _("A class must contain at least one glyph name, glyph names must be valid postscript names, and no glyphs may appear in any other class") :
		strcmp(title,_("Bad Coverage"))==0? _("A coverage table must contain at least one glyph name, and glyph names must be valid postscript names") :
		    _("A glyph list must contain at least one glyph name, and glyph names must be valid postscript names") );
return(false);
    }
    if ( (missingname=CCD_NonExistantName(sf,ret))!=NULL ) {
	char *buts[3];
	buts[0] = _("_Yes"); buts[1] = _("_No"); buts[2] = NULL;
	ans = gwwv_ask(title,(const char **) buts,0,1,
		strcmp(title,_("Bad Class"))==0? _("The class member \"%.20s\" is not in this font.\nIs that what you want?") :
		strcmp(title,_("Bad Coverage"))==0? _("The coverage table member \"%.20s\" is not in this font.\nIs that what you want?") :
		    _("There is no glyph named \"%.20hs\" in this font.\nIs that what you want?"), missingname );
	free(missingname);
return(!ans);
    }
return( true );
}

int CCD_InvalidClassList(char *ret,GGadget *list,int wasedit) {
    int32 len;
    GTextInfo **ti = GGadgetGetList(list,&len);
    char *pt, *end; unichar_t *tpt, *tend;
    int i;

    for ( pt = ret; *pt; pt = end ) {
	while ( *pt==' ' ) ++pt;
	if ( *pt=='\0' )
    break;
	end = strchr(pt,' ');
	if ( end==NULL ) end = pt+strlen(pt);
	for ( i=0; i<len; ++i ) {
	    if ( wasedit && ti[i]->selected )
	continue;
	    for ( tpt=ti[i]->text; *tpt; tpt = tend ) {
		while ( *tpt==' ' ) ++tpt;
		tend = u_strchr(tpt,' ');
		if ( tend==NULL ) tend = tpt+u_strlen(tpt);
		/* We've already checked for ASCII, so ucs2 & utf8 should */
		/* have the same length for an ASCII string */
		if ( tend-tpt==end-pt && uc_strncmp(tpt,pt,end-pt)==0 ) {
		    char *dupname = copyn(pt,end-pt);
		    ff_post_error(_("Bad Class"),_("No glyphs from another class may appear here, but %.30s does"), dupname);
		    free(dupname);
return( true );
		}
	    }
	}
    }
return( false );
}

static int CCD_ReasonableClassNum(const unichar_t *match,GGadget *mlist,
	const unichar_t *back, GGadget *blist,
	const unichar_t *fore, GGadget *flist,
	struct fpst_rule *r ) {
    int32 mlen, blen, flen;
    GTextInfo **ti;
    const unichar_t *pt; unichar_t *end;
    int any;
    int val;

    ti = GGadgetGetList(mlist,&mlen);
    ti = GGadgetGetList(blist,&blen);
    ti = GGadgetGetList(flist,&flen);

    any = 0;
    for ( pt=match;; pt = end) {
	while ( *pt==' ' ) ++pt;
	if ( *pt=='\0' )
    break;
	if ( isdigit( *pt )) ++any;
	val = u_strtol(pt,&end,10);
	if ( *end!=' ' && *end!='\0' ) {
	    ff_post_error(_("Bad Match Class Number"),_("The list of class numbers should only contain digits and spaces"));
return( false );
	}
	if ( val>=mlen || val<0) {
	    ff_post_error(_("Bad Match Class Number"),_("There are only %d classes in this class set, yet you are trying to use %d."),mlen,val);
return( false );
	}
    }
    r->u.class.ncnt = any;
    if ( any==0 ) {
	ff_post_error(_("Bad Match Class Number"),_("There are no class numbers listed to be matched. There must be at least one."));
return( false );
    }

    any = 0;
    for ( pt=back;; pt = end) {
	while ( *pt==' ' ) ++pt;
	if ( *pt=='\0' )
    break;
	if ( isdigit( *pt )) ++any;
	val = u_strtol(pt,&end,10);
	if ( *end!=' ' && *end!='\0' ) {
	    ff_post_error(_("Bad Backtrack Class Number"),_("The list of class numbers should only contain digits and spaces"));
return( false );
	}
	if ( val>=blen || val<0) {
	    ff_post_error(_("Bad Backtrack Class Number"),_("There are only %d classes in this class set, yet you are trying to use %d."),blen,val);
return( false );
	}
    }
    r->u.class.bcnt = any;

    any = 0;
    for ( pt=fore;; pt = end) {
	while ( *pt==' ' ) ++pt;
	if ( *pt=='\0' )
    break;
	if ( isdigit( *pt )) ++any;
	val = u_strtol(pt,&end,10);
	if ( *end!=' ' && *end!='\0' ) {
	    ff_post_error(_("Bad Lookahead Class Number"),_("The list of class numbers should only contain digits and spaces"));
return( false );
	}
	if ( val>=flen || val<0) {
	    ff_post_error(_("Bad Lookahead Class Number"),_("There are only %d classes in this class set, yet you are trying to use %d."),flen,val);
return( false );
	}
    }
    r->u.class.fcnt = any;

    r->u.class.nclasses = galloc(r->u.class.ncnt*sizeof(uint16));
    r->u.class.bclasses = galloc(r->u.class.bcnt*sizeof(uint16));
    r->u.class.fclasses = galloc(r->u.class.fcnt*sizeof(uint16));

    any = 0;
    for ( pt=match;; pt = end) {
	while ( *pt==' ' ) ++pt;
	if ( *pt=='\0' )
    break;
	if ( any>=r->u.class.ncnt ) IError("ReasonableClassNumber unreasonable");
	r->u.class.nclasses[any++] = u_strtol(pt,&end,10);
    }

    any = 0;
    for ( pt=back;; pt = end) {
	while ( *pt==' ' ) ++pt;
	if ( *pt=='\0' )
    break;
	if (r->u.class.bcnt-1-any<0 || r->u.class.bcnt-1-any>=r->u.class.bcnt ) IError("ReasonableClassNumber unreasonable");
	r->u.class.bclasses[r->u.class.bcnt-1-any++] = u_strtol(pt,&end,10);
    }

    any = 0;
    for ( pt=fore;; pt = end) {
	while ( *pt==' ' ) ++pt;
	if ( *pt=='\0' )
    break;
	if ( any>=r->u.class.fcnt ) IError("ReasonableClassNumber unreasonable");
	r->u.class.fclasses[any++] = u_strtol(pt,&end,10);
    }
return( true );
}

static void CCD_FinishEditNew(struct contextchaindlg *ccd) {
    char *temp;
    struct fpst_rule dummy;
    GGadget *list = GWidgetGetControl(ccd->gw,CID_GList+ccd->wasoffset);
    int i,tot;
    char *buts[3];
    buts[0] = _("_Yes"); buts[1] = _("_No"); buts[2] = NULL;

    if ( ccd->wasoffset>=300 ) {		/* It's a class */
	char *ret = GGadgetGetTitle8(GWidgetGetControl(ccd->gw,CID_GlyphList+300));
	if ( !CCD_NameListCheck(ccd->sf,ret,true,_("Bad Class")) ||
		CCD_InvalidClassList(ret,list,ccd->wasedit) ) {
	    free(ret);
return;
	}
	if ( ccd->wasedit ) {
	    GListChangeLine8(list,GGadgetGetFirstListSelectedItem(list),ret);
	} else {
	    GListAppendLine8(list,ret,false);
	}
	ccd->aw = aw_class;
	GDrawSetVisible(ccd->classbuild,false);
	GDrawSetVisible(ccd->class,true);
	free(ret);
    } else if ( ccd->wasoffset>=200 ) {		/* It's class numbers */
	unichar_t *ret;
	memset(&dummy,0,sizeof(dummy));
	CCD_ParseLookupList(ccd->sf,&dummy,GWidgetGetControl(ccd->gw,CID_LookupList+500));
	if ( dummy.lookup_cnt==0 ) {
	    int ans = gwwv_ask(_("No Sequence/Lookups"),
		    (const char **) buts,0,1,
		    _("There are no entries in the Sequence/Lookup List, was this intentional?"));
	    if ( ans==1 )
return;
	}
	if ( !CCD_ReasonableClassNum(
		_GGadgetGetTitle(GWidgetGetControl(ccd->gw,CID_ClassNumbers)),
		    GWidgetGetControl(ccd->gw,CID_ClassList),
		_GGadgetGetTitle(GWidgetGetControl(ccd->gw,CID_ClassNumbers+20)),
		    GWidgetGetControl(ccd->gw,CID_ClassList+20),
		_GGadgetGetTitle(GWidgetGetControl(ccd->gw,CID_ClassNumbers+40)),
		    GWidgetGetControl(ccd->gw,CID_ClassList+40), &dummy )) {
	    FPSTRuleContentsFree(&dummy,pst_class);
return;
	}
	for ( i=0; i<dummy.lookup_cnt; ++i ) {
	    if ( dummy.lookups[i].seq >= dummy.u.class.ncnt ) {
		ff_post_error(_("Bad Sequence/Lookup List"),_("Sequence number out of bounds, must be less than %d (number of classes in list above)"), dummy.u.class.ncnt );
return;
	    }
	}
	ret = clslistitem(&dummy);
	FPSTRuleContentsFree(&dummy,pst_class);
	if ( ccd->wasedit ) {
	    GListChangeLine(list,GGadgetGetFirstListSelectedItem(list),ret);
	} else {
	    GListAppendLine(list,ret,false);
	}
	ccd->aw = aw_class;
	GDrawSetVisible(ccd->classnumber,false);
	GDrawSetVisible(ccd->class,true);
    } else if ( ccd->wasoffset>=100 ) {		/* It's from coverage */
	char *ret = GGadgetGetTitle8(GWidgetGetControl(ccd->gw,CID_GlyphList+100));
	if ( !CCD_NameListCheck(ccd->sf,ret,true,_("Bad Coverage")) ) {
	    free(ret);
return;
	}
	if ( ccd->wasedit ) {
	    GListChangeLine8(list,GGadgetGetFirstListSelectedItem(list),ret);
	} else {
	    GListAppendLine8(list,ret,false);
	}
	ccd->aw = aw_coverage;
	GDrawSetVisible(ccd->cselect,false);
	GDrawSetVisible(ccd->coverage,true);
	free(ret);
    } else {			/* It's from glyph list */
	unichar_t *ret; char *val;

	memset(&dummy,0,sizeof(dummy));
	val = GGadgetGetTitle8(GWidgetGetControl(ccd->gw,CID_GlyphList));
	if ( !CCD_NameListCheck(ccd->sf,val,true,_("Bad Match Glyph List")) ) {
	    free(val);
return;
	}
	free(val);
	val = GGadgetGetTitle8(GWidgetGetControl(ccd->gw,CID_GlyphList+20));
	if ( !CCD_NameListCheck(ccd->sf,val,false,_("Bad Backtrack Glyph List")) ) {
	    free(val);
return;
	}
	free(val);
	val = GGadgetGetTitle8(GWidgetGetControl(ccd->gw,CID_GlyphList+40));
	if ( !CCD_NameListCheck(ccd->sf,val,false,_("Bad Lookahead Glyph List")) ) {
	    free(val);
return;
	}
	free(val);
	CCD_ParseLookupList(ccd->sf,&dummy,GWidgetGetControl(ccd->gw,CID_LookupList));
	if ( dummy.lookup_cnt==0 ) {
	    int ans = gwwv_ask(_("No Sequence/Lookups"),
		    (const char **) buts,0,1,
		    _("There are no entries in the Sequence/Lookup List, was this intentional?"));
	    if ( ans==1 )
return;
	}
	dummy.u.glyph.names = ccd_cu_copy(_GGadgetGetTitle(GWidgetGetControl(ccd->gw,CID_GlyphList)));
	tot = CCD_GlyphNameCnt(_GGadgetGetTitle(GWidgetGetControl(ccd->gw,CID_GlyphList)));
	for ( i=0; i<dummy.lookup_cnt; ++i ) {
	    if ( dummy.lookups[i].seq >= tot ) {
		ff_post_error(_("Bad Sequence/Lookup List"),_("Sequence number out of bounds, must be less than %d (number of glyphs, classes or coverage tables)"),  tot );
return;
	    }
	}
	temp = ccd_cu_copy(_GGadgetGetTitle(GWidgetGetControl(ccd->gw,CID_GlyphList+20)));
	dummy.u.glyph.back = reversenames(temp);
	free(temp);
	dummy.u.glyph.fore = ccd_cu_copy(_GGadgetGetTitle(GWidgetGetControl(ccd->gw,CID_GlyphList+40)));
	ret = glistitem(&dummy);
	FPSTRuleContentsFree(&dummy,pst_glyphs);
	if ( ccd->wasedit ) {
	    GListChangeLine(list,GGadgetGetFirstListSelectedItem(list),ret);
	} else {
	    GListAppendLine(list,ret,false);
	}
	ccd->aw = aw_glist;
	GDrawSetVisible(ccd->glyphs,false);
	GDrawSetVisible(ccd->glist,true);
    }
}

static int isEverythingElse(unichar_t *text) {
    /* GT: The string "{Everything Else}" is used in the context of a list */
    /* GT: of classes (a set of kerning classes) where class 0 designates the */
    /* GT: default class containing all glyphs not specified in the other classes */
    unichar_t *everything_else = utf82u_copy( _("{Everything Else}") );
    int ret = u_strcmp(text,everything_else);
    free(everything_else);
return( ret==0 );
}

static void _CCD_DoEditNew(struct contextchaindlg *ccd,int off,int isedit) {
    static unichar_t nulstr[] = { 0 };
    int i;

    if ( off>=200 && off<300) {	/* It's a list of class numbers from the class window */
	int32 i, len;
	GTextInfo **ti;
	if ( isedit ) {
	    GGadget *list = GWidgetGetControl(ccd->gw,CID_GList+off);
	    struct fpst_rule dummy;
	    unichar_t *temp;
	    int32 len;
	    GTextInfo **old = GGadgetGetList(list,&len);
	    i = GGadgetGetFirstListSelectedItem(list);
	    if ( i==-1 )
return;
	    memset(&dummy,0,sizeof(dummy));
	    clslistitem2rule(ccd->sf,old[i]->text,&dummy);
	    GGadgetSetTitle(GWidgetGetControl(ccd->gw,CID_ClassNumbers),
		    (temp=classnumbers(dummy.u.class.ncnt,dummy.u.class.nclasses)));
	    free(temp);
	    GGadgetSetTitle(GWidgetGetControl(ccd->gw,CID_ClassNumbers+20),
		    (temp=rclassnumbers(dummy.u.class.bcnt,dummy.u.class.bclasses)));
	    free(temp);
	    GGadgetSetTitle(GWidgetGetControl(ccd->gw,CID_ClassNumbers+40),
		    (temp=classnumbers(dummy.u.class.fcnt,dummy.u.class.fclasses)));
	    free(temp);
	    ti = slistlist(&dummy);
	    GGadgetSetList(GWidgetGetControl(ccd->gw,CID_LookupList+500),ti,false);
	    FPSTRuleContentsFree(&dummy,pst_class);
	} else {
	    for ( i=0; i<3; ++i )
		GGadgetSetTitle(GWidgetGetControl(ccd->gw,CID_ClassNumbers+i*20),nulstr);
	    GGadgetClearList(GWidgetGetControl(ccd->gw,CID_LookupList+500));
	}
	GWidgetIndicateFocusGadget(GWidgetGetControl(ccd->gw,CID_ClassNumbers));
	for ( i=0; i<3; ++i ) {
	    if ( i!=0 && GGadgetIsChecked(GWidgetGetControl(ccd->gw,CID_SameAsClasses+i*20)) )
		ti = GGadgetGetList(GWidgetGetControl(ccd->gw,CID_GList+300),&len);
	    else
		ti = GGadgetGetList(GWidgetGetControl(ccd->gw,CID_GList+300+i*20),&len);
	    GGadgetSetList(GWidgetGetControl(ccd->gw,CID_ClassList+i*20),ti,true);
	}
	ccd->aw = aw_classnumber;
	GDrawSetVisible(ccd->class,false);
	GDrawSetVisible(ccd->classnumber,true);
    } else if ( off>=100 ) {	/* It's from coverage or class */
	int to_off = off>=300 ? 300 : 100;
	if ( isedit ) {
	    GGadget *list = GWidgetGetControl(ccd->gw,CID_GList+off);
	    int32 len;
	    GTextInfo **old = GGadgetGetList(list,&len);
	    i = GGadgetGetFirstListSelectedItem(list);
	    if ( i==-1 )
return;
	    if ( off==300 && i==0 && GTabSetGetSel(GWidgetGetControl(ccd->gw,CID_MatchType+300))==0 &&
		    isEverythingElse(old[i]->text))
		GGadgetSetTitle8(GWidgetGetControl(ccd->gw,CID_GlyphList+to_off),"");
	    else
		GGadgetSetTitle(GWidgetGetControl(ccd->gw,CID_GlyphList+to_off),old[i]->text);
	} else {
	    GGadgetSetTitle(GWidgetGetControl(ccd->gw,CID_GlyphList+to_off),nulstr);
	}
	GWidgetIndicateFocusGadget(GWidgetGetControl(ccd->gw,CID_GlyphList+to_off));
	if ( off>=300 ) {		/* It's a class from the class window */
	    ccd->aw = aw_classbuild;
	    GDrawSetVisible(ccd->class,false);
	    GDrawSetVisible(ccd->classbuild,true);
	} else {
	    ccd->aw = aw_cselect;
	    GDrawSetVisible(ccd->coverage,false);
	    GDrawSetVisible(ccd->cselect,true);
	}
    } else {			/* It's from glyph list */
	unichar_t *temp; char *temp2;
	if ( isedit ) {
	    struct fpst_rule dummy;
	    GGadget *list = GWidgetGetControl(ccd->gw,CID_GList+off);
	    int32 len; int i;
	    GTextInfo **old = GGadgetGetList(list,&len), **ti;
	    i = GGadgetGetFirstListSelectedItem(list);
	    if ( i==-1 )
return;
	    memset(&dummy,0,sizeof(dummy));
	    glistitem2rule(ccd->sf,old[i]->text,&dummy);
	    GGadgetSetTitle(GWidgetGetControl(ccd->gw,CID_GlyphList),(temp=uc_copy(dummy.u.glyph.names)));
	    free(temp);
	    GGadgetSetTitle(GWidgetGetControl(ccd->gw,CID_GlyphList+20),(temp=uc_copy((temp2 = reversenames(dummy.u.glyph.back))==NULL ? "" : temp2)));
	    free(temp); free(temp2);
	    GGadgetSetTitle(GWidgetGetControl(ccd->gw,CID_GlyphList+40),(temp=uc_copy(dummy.u.glyph.fore?dummy.u.glyph.fore:"")));
	    free(temp);
	    ti = slistlist(&dummy);
	    GGadgetSetList(GWidgetGetControl(ccd->gw,CID_LookupList),ti,false);
	    FPSTRuleContentsFree(&dummy,pst_glyphs);
	} else {
	    for ( i=0; i<3; ++i )
		GGadgetSetTitle(GWidgetGetControl(ccd->gw,CID_GlyphList+i*20),nulstr);
	    GGadgetClearList(GWidgetGetControl(ccd->gw,CID_LookupList));
	}
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_LEdit),false);
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_LDelete),false);
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_LUp),false);
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_LDown),false);
	ccd->aw = aw_glyphs;
	GDrawSetVisible(ccd->glist,false);
	GDrawSetVisible(ccd->glyphs,true);
    }
    CCD_EnableNextPrev(ccd);
    ccd->wasedit = isedit;
    ccd->wasoffset = off;
}

static void _CCD_ToSelection(struct contextchaindlg *ccd,int cid,int order_matters ) {
    const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(ccd->gw,cid));
    SplineFont *sf = ccd->sf;
    FontView *fv = (FontView *) sf->fv;
    const unichar_t *end;
    int i,pos, found=-1;
    char *nm;

    GDrawRaise(fv->gw);
    GDrawSetVisible(fv->gw,true);
    memset(fv->b.selected,0,fv->b.map->enccount);
    i=1;
    while ( *ret ) {
	end = u_strchr(ret,' ');
	if ( end==NULL ) end = ret+u_strlen(ret);
	nm = cu_copybetween(ret,end);
	for ( ret = end; isspace(*ret); ++ret);
	if (( pos = SFFindSlot(sf,fv->b.map,-1,nm))!=-1 ) {
	    if ( found==-1 ) found = pos;
	    fv->b.selected[pos] = i;
	    if ( order_matters && i<255 ) ++i;
	}
	free(nm);
    }

    if ( found!=-1 )
	FVScrollToChar(fv,found);
    GDrawRequestExpose(fv->v,NULL,false);
}

static int CCD_ToSelection(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));
	int off = GGadgetGetCid(g)-CID_Select;
	int isglyph = ccd->aw==aw_glyphs;

	_CCD_ToSelection(ccd,CID_GlyphList+off,isglyph);
    }
return( true );
}

static int CCD_ToSelection2(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));
	int off = GGadgetGetCid(g)-CID_Select2;

	_CCD_ToSelection(ccd,CID_RplList+off,true);
    }
return( true );
}

static int BaseEnc(SplineChar *sc) {
    int uni = sc->unicodeenc;

    if ( uni==-1 || (uni>=0xe000 && uni<=0xf8ff)) {	/* Private use or undefined */
	char *pt = strchr(sc->name,'.');
	if ( pt!=NULL ) {
	    *pt = '\0';
	    uni = UniFromName(sc->name,ui_none,NULL);
	    *pt = '.';
	}
    }
return( uni );
}

static int GlyphInClass(char *name,char *glyphs) {
    char *start, *pt;
    int len = strlen(name);

    for ( start = glyphs; *start ; ) {
	pt = strchr(start,' ');
	if ( pt==NULL ) {
return( strcmp(name,start)==0 );
	}
	if ( pt-start==len && strncmp(name,start,len)==0 )
return( true );
	start = pt;
	while ( *start==' ' ) ++start;
    }
return( false );
}

static char *addglyph(char *results,char *name) {
    int len = strlen( results );
    results = grealloc(results,len+strlen(name)+2);
    if ( len!=0 ) {
	results[len] = ' ';
	strcpy( results+len+1,name );
    } else
	strcpy(results,name);
return( results );
}

static int CCD_StdClass(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));
	int off = GGadgetGetCid(g)-CID_StdClasses;
	GTextInfo *ti = GGadgetGetListItemSelected(g);
	char *glyphs, *results;
	int any, gid, check_script, baseenc;
	FeatureScriptLangList *fl = ccd->fpst->subtable->lookup->features;
	SplineChar *sc;

	if ( ti==NULL )
return( true );
	if ( strcmp((char *) ti->userdata,"None")==0 )
return( true );

	check_script= strcmp((char *) ti->userdata,"0-9")!=0;
	if ( check_script && fl==NULL ) {
	    GGadgetSelectOneListItem(g,0);
return( true );
	}
	glyphs = GGadgetGetTitle8(GWidgetGetControl(ccd->gw,CID_GlyphList+off));
	results = copy(glyphs);
	any = 0;

	for ( gid=0; gid<ccd->sf->glyphcnt; ++gid ) if ( (sc=ccd->sf->glyphs[gid])!=NULL ) {
	    if ( check_script && !ScriptInFeatureScriptList(SCScriptFromUnicode(sc),fl))
	continue;
	    baseenc = BaseEnc(sc);
	    if ( strcmp(ti->userdata,"Glyphs")==0 ) {
		/* ok, we've checked the script, nothing else to do, it matches*/
	    } else if ( baseenc>=0x10000 || baseenc==-1 ) {
		/* I don't have any tables outside bmp */
	continue;
	    } else if ( strcmp(ti->userdata,"0-9")==0 ) {
		if ( baseenc<'0' || baseenc>'9' )
	continue;
	    } else if ( strcmp(ti->userdata,"Letters")==0 ) {
		if ( !isalpha(baseenc))
	continue;
	    } else if ( strcmp(ti->userdata,"LC")==0 ) {
		if ( !islower(baseenc))
	continue;
	    } else if ( strcmp(ti->userdata,"UC")==0 ) {
		if ( !isupper(baseenc))
	continue;
	    }
	    /* If we get here the glyph should be in the class */
	    if ( GlyphInClass(sc->name,glyphs))
	continue;		/* It's already there nothing for us to do */
	    results = addglyph(results,sc->name);
	    any = true;
	}
	if ( any ) {
	    GGadgetSelectOneListItem(g,sizeof(stdclasses)/sizeof(stdclasses[0])-2);
	    GGadgetSetTitle8(GWidgetGetControl(ccd->gw,CID_GlyphList+off),results);
	} else
	    GGadgetSelectOneListItem(g,0);
	free(glyphs); free(results);
    }
return( true );
}

static void _CCD_FromSelection(struct contextchaindlg *ccd,int cid,int order_matters ) {
    SplineFont *sf = ccd->sf;
    FontView *fv = (FontView *) sf->fv;
    unichar_t *vals, *pt;
    int i, j, len, max;
    SplineChar *sc;

    for ( i=len=max=0; i<fv->b.map->enccount; ++i ) if ( fv->b.selected[i]) {
	sc = SFMakeChar(sf,fv->b.map,i);
	len += strlen(sc->name)+1;
	if ( fv->b.selected[i]>max ) max = fv->b.selected[i];
    }
    pt = vals = galloc((len+1)*sizeof(unichar_t));
    *pt = '\0';
    if ( order_matters ) {
	/* in a glyph string the order of selection is important */
	/* also in replacement list */
	for ( j=1; j<=max; ++j ) {
	    for ( i=0; i<fv->b.map->enccount; ++i ) if ( fv->b.selected[i]==j ) {
		int gid = fv->b.map->map[i];
		if ( gid!=-1 && sf->glyphs[gid]!=NULL ) {
		    uc_strcpy(pt,sf->glyphs[gid]->name);
		    pt += u_strlen(pt);
		    *pt++ = ' ';
		}
	    }
	}
    } else {
	/* in a coverage table (or class) the order of selection is irrelevant */
	for ( i=0; i<fv->b.map->enccount; ++i ) if ( fv->b.selected[i] ) {
	    int gid = fv->b.map->map[i];
	    if ( gid!=-1 && sf->glyphs[gid]!=NULL ) {
		uc_strcpy(pt,sf->glyphs[gid]->name);
		pt += u_strlen(pt);
		*pt++ = ' ';
	    }
	}
    }
    if ( pt>vals ) pt[-1]='\0';

    GGadgetSetTitle(GWidgetGetControl(ccd->gw,cid),vals);
    free(vals);
}

static int CCD_FromSelection(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));
	int off = GGadgetGetCid(g)-CID_Set;
	int isglyph = ccd->aw==aw_glyphs;

	_CCD_FromSelection(ccd,CID_GlyphList+off,isglyph);
    }
return( true );
}

static int CCD_FromSelection2(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));
	int off = GGadgetGetCid(g)-CID_Set2;

	_CCD_FromSelection(ccd,CID_RplList+off,true);
    }
return( true );
}

static int CCD_Delete(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));
	int off = GGadgetGetCid(g)-CID_Delete;
	GListDelSelected(GWidgetGetControl(ccd->gw,CID_GList+off));
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_Delete+off),false);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_Edit+off),false);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_Up+off),false);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_Down+off),false);
    }
return( true );
}

static int CCD_Up(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));
	int off = GGadgetGetCid(g)-CID_Up;
	GGadget *list = GWidgetGetControl(ccd->gw,CID_GList+off);
	int32 len; int i;

	(void) GGadgetGetList(list,&len);
	GListMoveSelected(list,-1);
	i = GGadgetGetFirstListSelectedItem(list);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_Up+off),i!=0);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_Down+off),i!=len-1);
    }
return( true );
}

static int CCD_Down(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));
	int off = GGadgetGetCid(g)-CID_Down;
	GGadget *list = GWidgetGetControl(ccd->gw,CID_GList+off);
	int32 len; int i;

	(void) GGadgetGetList(list,&len);
	GListMoveSelected(list,1);
	i = GGadgetGetFirstListSelectedItem(list);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_Up+off),i!=0);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_Down+off),i!=len-1);
    }
return( true );
}

static int CCD_Edit(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));
	int off = GGadgetGetCid(g)-CID_Edit;
	_CCD_DoEditNew(ccd,off,true);
    }
return( true );
}

static int CCD_New(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));
	int off = GGadgetGetCid(g)-CID_New;
	_CCD_DoEditNew(ccd,off,false);
    }
return( true );
}

static int CCD_GlyphSelected(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));
	int off = GGadgetGetCid(g)-CID_GList;
	int32 len; int i;

	(void) GGadgetGetList(g,&len);
	i = GGadgetGetFirstListSelectedItem(g);
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_Up+off),i>0 &&
		(i>1 || off<300));
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_Down+off),i!=len-1 &&
		i!=-1 && (i!=0 || off<300));
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_Delete+off),i!=-1 &&
		(i!=0 || off<300));
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_Edit+off),i!=-1 &&
		(i!=0 || off<300 ||
		 (off>=300 && GTabSetGetSel(GWidgetGetControl(ccd->gw,CID_MatchType+300))==0 )));
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_listdoubleclick ) {
	struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));
	int off = GGadgetGetCid(g)-CID_GList;
	int i = GGadgetGetFirstListSelectedItem(g);
	if ( i!=0 || off<300 )
	    _CCD_DoEditNew(ccd,off,true);
    }
return( true );
}

static int CCD_SameAsClasses(GGadget *g, GEvent *e) {
    int ison = GGadgetIsChecked(g);
    int off = GGadgetGetCid(g)-CID_SameAsClasses;
    struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));

    GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_GList+300+off),!ison);
    GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_New+300+off),!ison);
    if ( ison ) {
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_Edit+300+off),false);
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_Delete+300+off),false);
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_Up+300+off),false);
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_Down+300+off),false);
    } else {
	GEvent ev;
	memset(&ev,0,sizeof(ev));
	ev.type =et_controlevent;
	ev.u.control.subtype = et_listselected;
	ev.u.control.g = GWidgetGetControl(ccd->gw,CID_GList+300+off);
	CCD_GlyphSelected(ev.u.control.g,&ev);
    }
return( true );
}

static int CCD_ClassSelected(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));
	int off = GGadgetGetCid(g)-CID_ClassList;
	int i;
	GGadget *tf = GWidgetGetControl(ccd->gw,CID_ClassNumbers+off);
	char buf[20];
	unichar_t ubuf[20];

	i = GGadgetGetFirstListSelectedItem(g);
	if ( i==-1 )
return( true );
	sprintf( buf, " %d ", i );
	uc_strcpy(ubuf,buf);
	GTextFieldReplace(tf,ubuf);
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_listdoubleclick ) {
    }
return( true );
}

static int CCD_LDelete(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));
	int off = GGadgetGetCid(g)-CID_LDelete;
	GListDelSelected(GWidgetGetControl(ccd->gw,CID_LookupList+off));
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_LDelete+off),false);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_LEdit+off),false);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_LUp+off),false);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_LDown+off),false);
    }
return( true );
}

static int CCD_LUp(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));
	int off = GGadgetGetCid(g)-CID_LUp;
	GGadget *list = GWidgetGetControl(ccd->gw,CID_LookupList+off);
	int32 len; int i;

	(void) GGadgetGetList(list,&len);
	GListMoveSelected(list,-1);
	i = GGadgetGetFirstListSelectedItem(list);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_LUp+off),i!=0);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_LDown+off),i!=len-1);
    }
return( true );
}

static int CCD_LDown(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));
	int off = GGadgetGetCid(g)-CID_LDown;
	GGadget *list = GWidgetGetControl(ccd->gw,CID_LookupList+off);
	int32 len; int i;

	(void) GGadgetGetList(list,&len);
	GListMoveSelected(list,1);
	i = GGadgetGetFirstListSelectedItem(list);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_LUp+off),i!=0);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_LDown+off),i!=len-1);
    }
return( true );
}

struct seqlook_data {
    int done;
    int ok;
};

static int seqlook_e_h(GWindow gw, GEvent *e) {
    if ( e->type==et_close ) {
	struct seqlook_data *d = GDrawGetUserData(gw);
	d->done = true;
    } else if ( e->type == et_char ) {
return( false );
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct seqlook_data *d = GDrawGetUserData(gw);
	d->done = true;
	d->ok = GGadgetGetCid(e->u.control.g);
    }
return( true );
}

static void _CCD_DoLEditNew(struct contextchaindlg *ccd,int off,int isedit) {
    int seq, i;
    char cbuf[20];
    unichar_t nullstr[1], *end, *lstr = nullstr;
    GGadget *list = GWidgetGetControl(ccd->gw,CID_LookupList+off);
    struct seqlook_data d;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[8], boxes[3], *hvarray[7][3], *barray[8];
    GTextInfo label[8], **ti;
    const unichar_t *pt;
    unichar_t *line;
    int selpos= -1;
    int isgpos;

    nullstr[0] = 0; seq = 0;
    if ( isedit ) {
	int32 len;
	GTextInfo **ti = GGadgetGetList(list,&len);
	selpos = GGadgetGetFirstListSelectedItem(list);
	seq = u_strtol(ti[selpos]->text,&end,10);
	if ( *end==' ' ) ++end;
	lstr = end;
    }


    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict|wam_isdlg;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.is_dlg = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Sequence/Lookup");
    pos.x = pos.y = 0;
    pos.width = GDrawPointsToPixels(NULL,180);
    pos.height = GDrawPointsToPixels(NULL,130);
    gw = GDrawCreateTopWindow(NULL,&pos,seqlook_e_h,&d,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    memset(&boxes,0,sizeof(boxes));

    label[0].text = (unichar_t *) _("Sequence Number:");
    label[0].text_is_1byte = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 6;
    gcd[0].gd.flags = gg_visible | gg_enabled;
    gcd[0].creator = GLabelCreate;
    hvarray[0][0] = &gcd[0]; hvarray[0][1] = GCD_ColSpan; hvarray[0][2] = NULL;

    sprintf(cbuf,"%d",seq);
    label[1].text = (unichar_t *) cbuf;
    label[1].text_is_1byte = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.pos.x = 10; gcd[1].gd.pos.y = 18;
    gcd[1].gd.flags = gg_visible | gg_enabled;
    gcd[1].gd.cid = 1000;
    gcd[1].creator = GTextFieldCreate;
    hvarray[1][0] = GCD_HPad10; hvarray[1][1] = &gcd[1]; hvarray[1][2] = NULL;

    label[2].text = (unichar_t *) _("Lookup:");
    label[2].text_is_1byte = true;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = 5; gcd[2].gd.pos.y = gcd[1].gd.pos.y+25;
    gcd[2].gd.flags = gg_visible | gg_enabled;
    gcd[2].creator = GLabelCreate;
    hvarray[2][0] = &gcd[2]; hvarray[2][1] = GCD_ColSpan; hvarray[2][2] = NULL;

    gcd[3].gd.pos.x = 10; gcd[3].gd.pos.y = gcd[2].gd.pos.y+13;
    gcd[3].gd.pos.width = 140;
    gcd[3].gd.flags = gg_visible | gg_enabled;
    gcd[3].creator = GListButtonCreate;
    hvarray[3][0] = GCD_HPad10; hvarray[3][1] = &gcd[3]; hvarray[3][2] = NULL;

    gcd[4].gd.pos.width = 10; gcd[4].gd.pos.height = 10;
    gcd[4].gd.flags = gg_visible | gg_enabled;
    gcd[4].creator = GSpacerCreate;
    hvarray[4][0] = &gcd[4]; hvarray[4][1] = GCD_ColSpan; hvarray[4][2] = NULL;

    gcd[5].gd.pos.x = 20-3; gcd[5].gd.pos.y = 130-35-3;
    gcd[5].gd.pos.width = -1; gcd[5].gd.pos.height = 0;
    gcd[5].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[5].text = (unichar_t *) _("_OK");
    label[5].text_is_1byte = true;
    label[5].text_in_resource = true;
    gcd[5].gd.label = &label[5];
    gcd[5].gd.cid = true;
    gcd[5].creator = GButtonCreate;
    barray[0] = GCD_Glue; barray[1] = &gcd[5]; barray[2] = GCD_Glue;

    gcd[6].gd.pos.width = -1; gcd[6].gd.pos.height = 0;
    gcd[6].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[6].text = (unichar_t *) _("_Cancel");
    label[6].text_is_1byte = true;
    label[6].text_in_resource = true;
    gcd[6].gd.label = &label[6];
    gcd[6].creator = GButtonCreate;
    barray[3] = GCD_Glue; barray[4] = &gcd[6]; barray[5] = GCD_Glue;
    barray[6] = NULL;

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = barray;
    boxes[2].creator = GHBoxCreate;

    hvarray[5][0] = &boxes[2]; hvarray[5][1] = GCD_ColSpan; hvarray[5][2] = NULL;
    hvarray[6][0] = NULL;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = hvarray[0];
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(gw,boxes);
    isgpos = ( ccd->fpst->type==pst_contextpos || ccd->fpst->type==pst_chainpos );
    GGadgetSetList(gcd[3].ret, ti = SFLookupListFromType(ccd->sf,isgpos?gpos_start:gsub_start),false);
    for ( i=0; ti[i]->text!=NULL; ++i ) {
	if ( u_strcmp(lstr,ti[i]->text)==0 )
	    GGadgetSelectOneListItem(gcd[3].ret,i);
	if ( ccd->fpst!=NULL && ti[i]->userdata == ccd->fpst->subtable->lookup )
	    ti[i]->disabled = true;
    }
    GHVBoxFitWindow(boxes[0].ret);
    GDrawSetVisible(gw,true);

  retry:
    memset(&d,'\0',sizeof(d));
    while ( !d.done )
	GDrawProcessOneEvent(NULL);

    if ( d.ok ) {
	int err = false;
	seq = GetInt8(gw,1000,_("Sequence Position:"),&err);
	if ( err )
  goto retry;
	pt = _GGadgetGetTitle(gcd[3].ret);
	if ( pt==NULL || *pt=='\0' ) {
	    ff_post_error("Please select a lookup", "Please select a lookup");
  goto retry;
	}
	sprintf(cbuf,"%d ",seq);
	line = galloc((strlen(cbuf)+u_strlen(pt)+4)*sizeof(unichar_t));
	uc_strcpy(line,cbuf);
	u_strcat(line,pt);
	if ( isedit )
	    GListChangeLine(list,selpos,line);
	else
	    GListAppendLine(list,line,false);
	free(line);
    }
    GDrawDestroyWindow(gw);
}

static int CCD_LEdit(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));
	int off = GGadgetGetCid(g)-CID_LEdit;
	_CCD_DoLEditNew(ccd,off,true);
    }
return( true );
}

static int CCD_LNew(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));
	int off = GGadgetGetCid(g)-CID_LNew;
	_CCD_DoLEditNew(ccd,off,false);
    }
return( true );
}

static int CCD_LookupSelected(GGadget *g, GEvent *e) {
    struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));
    int off = GGadgetGetCid(g)-CID_LookupList;
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	int32 len; int i;

	(void) GGadgetGetList(g,&len);
	i = GGadgetGetFirstListSelectedItem(g);
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_LUp+off),i>0);
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_LDown+off),i!=len-1 && i!=-1);
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_LDelete+off),i!=-1);
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_LEdit+off),i!=-1);
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_listdoubleclick ) {
	_CCD_DoLEditNew(ccd,off,true);
    }
return( true );
}

void CCD_Close(struct contextchaindlg *ccd) {
    if ( ccd->isnew )
	GFI_FinishContextNew(ccd->gfi,ccd->fpst,false);
    GFI_CCDEnd(ccd->gfi);
    GDrawDestroyWindow(ccd->gw);
}

static char **CCD_ParseCoverageList(struct contextchaindlg *ccd,int cid,int *cnt) {
    int i;
    int32 _cnt;
    GTextInfo **ti = GGadgetGetList(GWidgetGetControl(ccd->gw,cid),&_cnt);
    char **ret;

    *cnt = _cnt;
    if ( _cnt==0 )
return( NULL );
    ret = galloc(*cnt*sizeof(char *));
    for ( i=0; i<_cnt; ++i )
	ret[i] = ccd_cu_copy(ti[i]->text);
return( ret );
}

static void CoverageReverse(char **bcovers,int bcnt) {
    int i;
    char *temp;

    for ( i=0; i<bcnt/2; ++i ) {
	temp = bcovers[i];
	bcovers[i] = bcovers[bcnt-i-1];
	bcovers[bcnt-i-1] = temp;
    }
}

static void _CCD_Ok(struct contextchaindlg *ccd) {
    FPST *fpst = ccd->fpst;
    int32 len, i, k, had_class0;
    GTextInfo **old, **classes;
    struct fpst_rule dummy;
    int has[3];
    char *buts[3];
    buts[0] = _("_Yes"); buts[1] = _("_No"); buts[2] = NULL;

    switch ( ccd->aw ) {
      case aw_glist: {
	old = GGadgetGetList(GWidgetGetControl(ccd->gw,CID_GList),&len);
	if ( len==0 ) {
	    ff_post_error(_("Missing rules"),_(" There must be at least one contextual rule"));
return;
	}
	FPSTRulesFree(fpst->rules,fpst->format,fpst->rule_cnt);
	fpst->format = pst_glyphs;
	fpst->rule_cnt = len;
	fpst->rules = gcalloc(len,sizeof(struct fpst_rule));
	for ( i=0; i<len; ++i )
	    glistitem2rule(ccd->sf,old[i]->text,&fpst->rules[i]);
      } break;
      case aw_class: {
	old = GGadgetGetList(GWidgetGetControl(ccd->gw,CID_GList+200),&len);
	if ( len==0 ) {
	    ff_post_error(_("Missing rules"),_(" There must be at least one contextual rule"));
return;
	}
	FPSTRulesFree(fpst->rules,fpst->format,fpst->rule_cnt);
	fpst->format = pst_class;
	fpst->rule_cnt = len;
	fpst->rules = gcalloc(len,sizeof(struct fpst_rule));
	fpst->nccnt = fpst->bccnt = fpst->fccnt = 0;
	fpst->nclass = fpst->bclass = fpst->fclass = NULL;
	has[1] = has[2] = false; has[0] = true;
	for ( i=0; i<len; ++i ) {
	    clslistitem2rule(ccd->sf,old[i]->text,&fpst->rules[i]);
	    if ( fpst->rules[i].u.class.bcnt!=0 ) has[1] = true;
	    if ( fpst->rules[i].u.class.fcnt!=0 ) has[2] = true;
	}
	for ( i=0; i<3; ++i ) {
	    if ( i==0 || !GGadgetIsChecked(GWidgetGetControl(ccd->gw,CID_SameAsClasses+i*20)) )
		classes = GGadgetGetList(GWidgetGetControl(ccd->gw,CID_GList+300+i*20),&len);
	    else if ( has[i] )
		classes = GGadgetGetList(GWidgetGetControl(ccd->gw,CID_GList+300),&len);
	    else
	continue;
	    (&fpst->nccnt)[i] = len;
	    (&fpst->nclass)[i] = galloc(len*sizeof(char*));
	    (&fpst->nclass)[i][0] = NULL;
	    had_class0 = i==0 && !isEverythingElse(classes[0]->text);
	    for ( k=had_class0 ? 0 : 1 ; k<len; ++k )
		(&fpst->nclass)[i][k] = cu_copy(classes[k]->text);
	}
      } break;
      case aw_coverage:
	old = GGadgetGetList(GWidgetGetControl(ccd->gw,CID_GList+100),&len);
	if ( len==0 ) {
	    ff_post_error(_("Bad Coverage Table"),_("There must be at least one match coverage table"));
return;
	}
	if ( fpst->format==pst_reversecoverage ) {
	    if ( len!=1 ) {
		ff_post_error(_("Bad Coverage Table"),_("In a Reverse Chaining Substitution there must be exactly one coverage table to match"));
return;
	    }
	    if ( CCD_GlyphNameCnt(old[0]->text)!=CCD_GlyphNameCnt(
		    _GGadgetGetTitle(GWidgetGetControl(ccd->gw,CID_RplList+100))) ) {
		ff_post_error(_("Replacement mismatch"),_("In a Reverse Chaining Substitution there must be exactly as many replacements as there are glyph names in the match coverage table"));
return;
	    }
	    dummy.u.rcoverage.replacements = ccd_cu_copy(
		    _GGadgetGetTitle(GWidgetGetControl(ccd->gw,CID_RplList+100)));
	} else {
	    CCD_ParseLookupList(ccd->sf,&dummy,GWidgetGetControl(ccd->gw,CID_LookupList+100));
	    if ( dummy.lookup_cnt==0 ) {
		int ans = gwwv_ask(_("No Sequence/Lookups"),
			(const char **) buts,0,1,
			_("There are no entries in the Sequence/Lookup List, was this intentional?"));
		if ( ans==1 )
return;
	    }
	}
	FPSTRulesFree(fpst->rules,fpst->format,fpst->rule_cnt);
	fpst->format = fpst->type==pst_reversesub ? pst_reversecoverage : pst_coverage;
	fpst->rule_cnt = 1;
	fpst->rules = gcalloc(1,sizeof(struct fpst_rule));
	fpst->rules[0].u.coverage.ncovers = CCD_ParseCoverageList(ccd,CID_GList+100,&fpst->rules[0].u.coverage.ncnt);
	fpst->rules[0].u.coverage.bcovers = CCD_ParseCoverageList(ccd,CID_GList+100+20,&fpst->rules[0].u.coverage.bcnt);
	CoverageReverse(fpst->rules[0].u.coverage.bcovers,fpst->rules[0].u.coverage.bcnt);
	fpst->rules[0].u.coverage.fcovers = CCD_ParseCoverageList(ccd,CID_GList+100+40,&fpst->rules[0].u.coverage.fcnt);
	if ( fpst->format==pst_reversecoverage )
	    fpst->rules[0].u.rcoverage.replacements = dummy.u.rcoverage.replacements;
	else {
	    fpst->rules[0].lookup_cnt = dummy.lookup_cnt;
	    fpst->rules[0].lookups = dummy.lookups;
	    for ( i=0; i<dummy.lookup_cnt; ++i ) {
		if ( dummy.lookups[i].seq >= fpst->rules[0].u.coverage.ncnt ) {
		    ff_post_error(_("Bad Sequence/Lookup List"),
			    _("Sequence number out of bounds, must be less than %d (number of classes in list above)"),
			    fpst->rules[0].u.coverage.ncnt );
return;
		}
	    }
	}
      break;
      default:
	IError("The OK button should not be enabled here");
return;
    }
    if ( ccd->isnew )
	GFI_FinishContextNew(ccd->gfi,ccd->fpst,true);
    GFI_CCDEnd(ccd->gfi);
    GDrawDestroyWindow(ccd->gw);
}

static int CCD_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));
	_CCD_Ok(ccd);
    }
return( true );
}

static int CCD_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));
	CCD_Close(ccd);
    }
return( true );
}

static int CCD_Next(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));
	switch ( ccd->aw ) {
	  case aw_formats:
	    GDrawSetVisible(ccd->formats,false);
	    if ( GGadgetIsChecked(GWidgetGetControl(ccd->gw,CID_ByGlyph)) ) {
		ccd->aw = aw_glist;
		GDrawSetVisible(ccd->glist,true);
	    } else if ( GGadgetIsChecked(GWidgetGetControl(ccd->gw,CID_ByClass)) ) {
		ccd->aw = aw_class;
		GDrawSetVisible(ccd->class,true);
	    } else {
		ccd->aw = aw_coverage;
		GDrawSetVisible(ccd->coverage,true);
	    }
	  break;
	  case aw_glyphs:
	  case aw_classnumber:
	  case aw_classbuild:
	  case aw_cselect:
	    CCD_FinishEditNew(ccd);
	  break;
	  default:
	    IError("The next button should not be enabled here");
	  break;
	}
	CCD_EnableNextPrev(ccd);
    }
return( true );
}

static int CCD_Prev(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));
	switch ( ccd->aw ) {
	  case aw_formats:
	    CCD_Cancel(g,e);
	  break;
	  case aw_glyphs:
	    ccd->aw = aw_glist;
	    GDrawSetVisible(ccd->glyphs,false);
	    GDrawSetVisible(ccd->glist,true);
	  break;
	  case aw_cselect:
	    ccd->aw = aw_coverage;
	    GDrawSetVisible(ccd->cselect,false);
	    GDrawSetVisible(ccd->coverage,true);
	  break;
	  case aw_classnumber:
	  case aw_classbuild:
	    ccd->aw = aw_class;
	    GDrawSetVisible(ccd->classbuild,false);
	    GDrawSetVisible(ccd->classnumber,false);
	    GDrawSetVisible(ccd->class,true);
	  break;
	  case aw_coverage:
	  case aw_class:
	  case aw_glist:
	    ccd->aw = aw_formats;
	    GDrawSetVisible(ccd->coverage,false);
	    GDrawSetVisible(ccd->class,false);
	    GDrawSetVisible(ccd->glist,false);
	    GDrawSetVisible(ccd->formats,true);
	  break;
	  default:
	    IError("Can't get here");
	  break;
	}
	CCD_EnableNextPrev(ccd);
    }
return( true );
}

static void CCD_SimulateDefaultButton( struct contextchaindlg *ccd ) {
    GEvent ev;
    int i,j;

    /* figure out what the default action should be */
    memset(&ev,0,sizeof(ev));
    ev.type = et_controlevent;
    ev.u.control.subtype = et_buttonactivate;
    if ( ccd->aw==aw_formats || ccd->aw == aw_glyphs || ccd->aw == aw_cselect ) {
	ev.u.control.g = GWidgetGetControl(ccd->gw,CID_Next);
	CCD_Next(ev.u.control.g,&ev);
/* For the glyph list and the coverage list, the [Next] button is disabled */
/* I think [OK] is probably inappropriate, so let's do either [New] or [Edit] */
/*  (which are what [Next] would logically do) depending on which of the two */
/*  is more appropriate */
/*	ev.u.control.g = GWidgetGetControl(ccd->gw,CID_OK); */
/*	CCD_OK(ev.u.control.g,&ev);			    */
    } else if ( ccd->aw==aw_glist ) {
	i = GGadgetGetFirstListSelectedItem( GWidgetGetControl(ccd->gw,CID_GList));
	if ( i==-1 ) {
	    ev.u.control.g = GWidgetGetControl(ccd->gw,CID_New);
	    CCD_New(ev.u.control.g,&ev);
	} else {
	    ev.u.control.g = GWidgetGetControl(ccd->gw,CID_Edit);
	    CCD_Edit(ev.u.control.g,&ev);
	}
    } else {
	i = GTabSetGetSel( GWidgetGetControl(ccd->gw,CID_MatchType+100));
	j = GGadgetGetFirstListSelectedItem( GWidgetGetControl(ccd->gw,CID_GList+100+i*20));
	if ( j==-1 ) {
	    ev.u.control.g = GWidgetGetControl(ccd->gw,CID_New+100+i*20);
	    CCD_New(ev.u.control.g,&ev);
	} else {
	    ev.u.control.g = GWidgetGetControl(ccd->gw,CID_Edit+100+i*20);
	    CCD_Edit(ev.u.control.g,&ev);
	}
    }
}

static void CCD_Drop(struct contextchaindlg *ccd,GEvent *event) {
    char *cnames;
    unichar_t *unames, *pt;
    const unichar_t *ret;
    GGadget *g;
    int32 len;

    if ( ccd->aw != aw_glyphs && ccd->aw != aw_cselect &&
	    (ccd->aw != aw_coverage || ccd->fpst->format!=pst_reversecoverage)) {
	GDrawBeep(NULL);
return;
    }

    if ( ccd->aw==aw_cselect )
	g = GWidgetGetControl(ccd->gw,CID_GlyphList+100);
    else if ( ccd->aw==aw_glyphs ) {
	int which = GTabSetGetSel(GWidgetGetControl(ccd->glyphs,CID_MatchType));
	g = GWidgetGetControl(ccd->gw,CID_GlyphList+20*which);
    } else {
	int which = GTabSetGetSel(GWidgetGetControl(ccd->coverage,CID_MatchType+100));
	if ( which!=0 ) {
	    GDrawBeep(NULL);
return;
	}
	g = GWidgetGetControl(ccd->gw,CID_RplList+100);
    }

    if ( !GDrawSelectionHasType(ccd->gw,sn_drag_and_drop,"STRING"))
return;
    cnames = GDrawRequestSelection(ccd->gw,sn_drag_and_drop,"STRING",&len);
    if ( cnames==NULL )
return;

    ret = _GGadgetGetTitle(g);
    unames = galloc((strlen(cnames)+u_strlen(ret)+8)*sizeof(unichar_t));
    u_strcpy(unames,ret);
    pt = unames+u_strlen(unames);
    if ( pt>unames && pt[-1]!=' ' )
	uc_strcpy(pt," ");
    uc_strcat(pt,cnames);
    free(cnames);

    GGadgetSetTitle(g,unames);
}

static int subccd_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("contextchain.html");
return( true );
	} else if ( GMenuIsCommand(event,H_("Quit|Ctl+Q") )) {
	    MenuExit(NULL,NULL,NULL);
return( true );
	} else if ( GMenuIsCommand(event,H_("Close|Ctl+Shft+Q") )) {
		CCD_Close(GDrawGetUserData(gw));
return( true );
	} else if ( event->u.chr.chars[0]=='\r' ) {
	    CCD_SimulateDefaultButton( (struct contextchaindlg *) GDrawGetUserData(gw));
return( true );
	}
return( false );
    } else if ( event->type == et_drop ) {
	CCD_Drop(GDrawGetUserData(gw),event);
    }
return( true );
}

static int ccd_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct contextchaindlg *ccd = GDrawGetUserData(gw);
	CCD_Close(ccd);
    } else if ( event->type==et_destroy ) {
	struct contextchaindlg *ccd = GDrawGetUserData(gw);
	chunkfree(ccd,sizeof(struct contextchaindlg));
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("contextchain.html");
return( true );
	} else if ( GMenuIsCommand(event,H_("Quit|Ctl+Q") )) {
	    MenuExit(NULL,NULL,NULL);
return( true );
	} else if ( GMenuIsCommand(event,H_("Close|Ctl+Shft+Q") )) {
	    CCD_Close(GDrawGetUserData(gw));
return( true );
	} else if ( event->u.chr.chars[0]=='\r' ) {
	    CCD_SimulateDefaultButton( (struct contextchaindlg *) GDrawGetUserData(gw));
return( true );
	}
return( false );
    } else if ( event->type==et_resize ) {
	/* int blen = GDrawPointsToPixels(NULL,GIntGetResource(_NUM_Buttonsize)),*/
	int i;
	GRect wsize, csize;
	struct contextchaindlg *ccd = GDrawGetUserData(gw);
	GGadget *g;

	GGadgetGetSize(GWidgetGetControl(ccd->gw,CID_SubSize),&wsize);

	GDrawResize(ccd->formats,wsize.width,wsize.height);
	GDrawResize(ccd->coverage,wsize.width,wsize.height);
	GDrawResize(ccd->glist,wsize.width,wsize.height);
	GDrawResize(ccd->glyphs,wsize.width,wsize.height);
	GDrawResize(ccd->cselect,wsize.width,wsize.height);
	GDrawResize(ccd->class,wsize.width,wsize.height);

	GDrawGetSize(ccd->gw,&wsize);

	GGadgetGetSize(GWidgetGetControl(ccd->gw,CID_MatchType),&csize);
	GGadgetResize(GWidgetGetControl(ccd->gw,CID_MatchType),wsize.width-GDrawPointsToPixels(NULL,10),csize.height);
	GGadgetGetSize(GWidgetGetControl(ccd->gw,CID_MatchType+100),&csize);
	GGadgetResize(GWidgetGetControl(ccd->gw,CID_MatchType+100),wsize.width-GDrawPointsToPixels(NULL,10),csize.height);
	GGadgetGetSize(GWidgetGetControl(ccd->gw,CID_LookupList),&csize);
	GGadgetResize(GWidgetGetControl(ccd->gw,CID_LookupList),wsize.width-GDrawPointsToPixels(NULL,20),csize.height);
	GGadgetGetSize(GWidgetGetControl(ccd->gw,CID_GList),&csize);
	GGadgetResize(GWidgetGetControl(ccd->gw,CID_GList),wsize.width-GDrawPointsToPixels(NULL,20),csize.height);
	if ( (g = GWidgetGetControl(ccd->gw,CID_LookupList+100))!=NULL ) {
	    GGadgetGetSize(g,&csize);
	    GGadgetResize(g,wsize.width-GDrawPointsToPixels(NULL,20),csize.height);
	} else {
	    GGadgetGetSize(GWidgetGetControl(ccd->gw,CID_RplList+100),&csize);
	    GGadgetResize(GWidgetGetControl(ccd->gw,CID_RplList+100),wsize.width-GDrawPointsToPixels(NULL,25),csize.height);
	}
	for ( i=0; i<3; ++i ) {
	    if ( (g=GWidgetGetControl(ccd->gw,CID_GlyphList+i*20))!=NULL ) {
		GGadgetGetSize(g,&csize);
		GGadgetResize(g,wsize.width-GDrawPointsToPixels(NULL,25),csize.height);
	    }
	    if ( (g=GWidgetGetControl(ccd->gw,CID_GList+100+i*20))!=NULL ) {
		GGadgetGetSize(g,&csize);
		GGadgetResize(g,wsize.width-GDrawPointsToPixels(NULL,20),csize.height);
	    }
	}
	if ( (g=GWidgetGetControl(ccd->gw,CID_GlyphList+100))!=NULL ) {
	    GGadgetGetSize(g,&csize);
	    GGadgetResize(g,wsize.width-GDrawPointsToPixels(NULL,25),csize.height);
	}

	GDrawRequestExpose(ccd->gw,NULL,false);
	GDrawRequestExpose(ccd->aw==aw_formats ? ccd->formats :
			    ccd->aw==aw_coverage ? ccd->coverage :
			    ccd->aw==aw_glist ? ccd->glist :
			    ccd->aw==aw_glyphs ? ccd->glyphs :
				ccd->cselect, NULL,false);
    } else if ( event->type == et_drop ) {
	CCD_Drop(GDrawGetUserData(gw),event);
    }
return( true );
}

#define CCD_WIDTH	340
#define CCD_HEIGHT	340

static int CCD_AddGList(GGadgetCreateData *gcd, GTextInfo *label,int off,
	int y, int width,int height) {
    int k = 0, space;
    int blen = GIntGetResource(_NUM_Buttonsize);

    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = y;
    gcd[k].gd.pos.width = width;
    gcd[k].gd.pos.height = height;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.handle_controlevent = CCD_GlyphSelected;
    gcd[k].gd.cid = CID_GList+off;
    gcd[k++].creator = GListCreate;

    space = width==CCD_WIDTH-30 ? 7 : 10;
    label[k].text = (unichar_t *) S_("Class|_New");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+gcd[k-1].gd.pos.height+10;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.handle_controlevent = CCD_New;
    gcd[k].gd.cid = CID_New+off;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("_Edit");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5+blen+space; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible;
    gcd[k].gd.handle_controlevent = CCD_Edit;
    gcd[k].gd.cid = CID_Edit+off;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("_Delete");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x+blen+space; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible;
    gcd[k].gd.handle_controlevent = CCD_Delete;
    gcd[k].gd.cid = CID_Delete+off;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("_Up");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x+blen+space+5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible;
    gcd[k].gd.handle_controlevent = CCD_Up;
    gcd[k].gd.cid = CID_Up+off;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("_Down");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x+blen+space; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible;
    gcd[k].gd.handle_controlevent = CCD_Down;
    gcd[k].gd.cid = CID_Down+off;
    gcd[k++].creator = GButtonCreate;

return( k );
}

static int CCD_AddGlyphList(GGadgetCreateData *gcd, GTextInfo *label,int off,
	int y, int height) {
    int k;
    static int inited = 0;

    if ( !inited ) {
	for ( k=0; stdclasses[k].text!=NULL; ++k )
	    stdclasses[k].text = (unichar_t *) _((char *) stdclasses[k].text);
	inited = true;
    }

    k=0;
    label[k].text = (unichar_t *) _("Set From Font");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = y;
    gcd[k].gd.popup_msg = (unichar_t *) _("Set this glyph list to be the characters selected in the fontview");
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[k].gd.handle_controlevent = CCD_FromSelection;
    gcd[k].gd.cid = CID_Set+off;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("Select In Font");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 110; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;
    gcd[k].gd.popup_msg = (unichar_t *) _("Set the fontview's selection to be the characters named here");
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[k].gd.handle_controlevent = CCD_ToSelection;
    gcd[k].gd.cid = CID_Select+off;
    gcd[k++].creator = GButtonCreate;

    if ( off>100 ) {	/* Don't add for glyph lists */
	label[k].text = (unichar_t *) _("Add Standard Class:");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = y+24+6;
	gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	gcd[k++].creator = GLabelCreate;

	gcd[k].gd.pos.x = 110; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-6;
	gcd[k].gd.popup_msg = (unichar_t *) _("Add one of these standard classes of glyphs to the current class");
	gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	gcd[k].gd.u.list = stdclasses;
	gcd[k].gd.handle_controlevent = CCD_StdClass;
	gcd[k].gd.cid = CID_StdClasses+off;
	gcd[k++].creator = GListButtonCreate;

	y+=24;
    }

    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = y+24;
    gcd[k].gd.pos.width = CCD_WIDTH-25; gcd[k].gd.pos.height = 8*13+4;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_textarea_wrap;
    gcd[k].gd.cid = CID_GlyphList+off;
    gcd[k++].creator = GTextAreaCreate;
return( k );
}

static void CCD_AddSeqLookup(GGadgetCreateData *gcd, GTextInfo *label,int off,
	int y, int width, int height) {
    int k, space;
    int blen = GIntGetResource(_NUM_Buttonsize);

    k = 0;
    space = width==CCD_WIDTH-30 ? 7 : 10;
    label[k].text = (unichar_t *) _("An ordered list of sequence positions and lookup tags");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = y;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;

    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+13;
    gcd[k].gd.pos.width = width;
    gcd[k].gd.pos.height = height;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.handle_controlevent = CCD_LookupSelected;
    gcd[k].gd.cid = CID_LookupList+off;
    gcd[k++].creator = GListCreate;

    label[k].text = (unichar_t *) S_("Lookup|_New");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+gcd[k-1].gd.pos.height+10;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.handle_controlevent = CCD_LNew;
    gcd[k].gd.cid = CID_LNew+off;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("_Edit");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x+blen+space; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible;
    gcd[k].gd.handle_controlevent = CCD_LEdit;
    gcd[k].gd.cid = CID_LEdit+off;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("_Delete");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x+blen+space; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible;
    gcd[k].gd.handle_controlevent = CCD_LDelete;
    gcd[k].gd.cid = CID_LDelete+off;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("_Up");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x+blen+space+5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible;
    gcd[k].gd.handle_controlevent = CCD_LUp;
    gcd[k].gd.cid = CID_LUp+off;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("_Down");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x+blen+space; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible;
    gcd[k].gd.handle_controlevent = CCD_LDown;
    gcd[k].gd.cid = CID_LDown+off;
    gcd[k++].creator = GButtonCreate;
}

static void CCD_AddReplacements(GGadgetCreateData *gcd, GTextInfo *label,
	struct fpst_rule *r,int off, int y) {
    int k;

    k=0;
    label[k].text = (unichar_t *) _("Replacements");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = y;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;

    label[k].text = (unichar_t *) _("Set From Font");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+13;
    gcd[k].gd.popup_msg = (unichar_t *) _("Set this glyph list to be the characters selected in the fontview");
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[k].gd.handle_controlevent = CCD_FromSelection2;
    gcd[k].gd.cid = CID_Set2+off;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("Select In Font");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 110; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;
    gcd[k].gd.popup_msg = (unichar_t *) _("Set the fontview's selection to be the characters named here");
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[k].gd.handle_controlevent = CCD_ToSelection2;
    gcd[k].gd.cid = CID_Select2+off;
    gcd[k++].creator = GButtonCreate;

    if ( r!=NULL && r->u.rcoverage.replacements!=NULL ) {
	label[k].text = (unichar_t *) r->u.rcoverage.replacements;
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
    }
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+25;
    gcd[k].gd.pos.width = CCD_WIDTH-25; gcd[k].gd.pos.height = 4*13+4;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_textarea_wrap;
    gcd[k].gd.cid = CID_RplList+off;
    gcd[k++].creator = GTextAreaCreate;
}

struct contextchaindlg *ContextChainEdit(SplineFont *sf,FPST *fpst,
	struct gfi_data *gfi, unichar_t *newname) {
    struct contextchaindlg *ccd;
    int i,j,k;
    static char *titles[2][5] = {
	{ N_("Edit Contextual Position"), N_("Edit Contextual Substitution"),
	    N_("Edit Chaining Position"), N_("Edit Chaining Substitution"),
	    N_("Edit Reverse Chaining Substitution"),
	},
	{ N_("New Contextual Position"), N_("New Contextual Substitution"),
	    N_("New Chaining Position"), N_("New Chaining Substitution"),
	    N_("New Reverse Chaining Substitution")
	}};
    GRect pos, subpos;
    GWindow gw;
    GWindowAttrs wattrs;
    GTabInfo faspects[5];
    GGadgetCreateData glgcd[8], fgcd[3], ggcd[3][14], cgcd[3][18], csgcd[6],
	clgcd[10];
    GTextInfo gllabel[8], flabel[3], glabel[3][14], clabel[3][18], cslabel[6],
	cllabel[10];
    GGadgetCreateData bgcd[6], rgcd[15], boxes[3], *barray[10], *hvarray[4][2];
    GTextInfo blabel[5], rlabel[15];
    struct fpst_rule *r=NULL;
    FPST *tempfpst;

    ccd = chunkalloc(sizeof(struct contextchaindlg));
    ccd->gfi = gfi;
    ccd->sf = sf;
    ccd->fpst = fpst;
    ccd->newname = newname;
    ccd->isnew = newname!=NULL;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = false;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _(titles[ccd->isnew][fpst->type-pst_contextpos]);
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,GGadgetScale(CCD_WIDTH));
    pos.height = GDrawPointsToPixels(NULL,CCD_HEIGHT);
    ccd->gw = gw = GDrawCreateTopWindow(NULL,&pos,ccd_e_h,ccd,&wattrs);

    subpos = pos;
    subpos.y = subpos.x = 4;
    subpos.width -= 8;
    ccd->subheightdiff = GDrawPointsToPixels(NULL,44)-8;
    subpos.height -= ccd->subheightdiff;


    memset(&blabel,0,sizeof(blabel));
    memset(&bgcd,0,sizeof(bgcd));
    memset(&boxes,0,sizeof(boxes));

    ccd->canceldrop = GDrawPointsToPixels(NULL,30);
    bgcd[0].gd.pos = subpos;
    bgcd[0].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels;
    bgcd[0].gd.cid = CID_SubSize;
    bgcd[0].creator = GSpacerCreate;
    hvarray[0][0] = &bgcd[0]; hvarray[0][1] = NULL;

    bgcd[1].gd.flags = gg_visible | gg_enabled;
    blabel[1].text = (unichar_t *) _("_OK");
    blabel[1].text_is_1byte = true;
    blabel[1].text_in_resource = true;
    bgcd[1].gd.label = &blabel[1];
    bgcd[1].gd.cid = CID_OK;
    bgcd[1].gd.handle_controlevent = CCD_OK;
    bgcd[1].creator = GButtonCreate;
    barray[0] = GCD_Glue; barray[1] = &bgcd[1];

    bgcd[2].gd.flags = gg_visible;
    blabel[2].text = (unichar_t *) _("< _Prev");
    blabel[2].text_is_1byte = true;
    blabel[2].text_in_resource = true;
    bgcd[2].gd.label = &blabel[2];
    bgcd[2].gd.handle_controlevent = CCD_Prev;
    bgcd[2].gd.cid = CID_Prev;
    bgcd[2].creator = GButtonCreate;
    barray[2] = GCD_Glue; barray[3] = &bgcd[2];

    bgcd[3].gd.flags = gg_visible;
    blabel[3].text = (unichar_t *) _("_Next >");
    blabel[3].text_is_1byte = true;
    blabel[3].text_in_resource = true;
    bgcd[3].gd.label = &blabel[3];
    bgcd[3].gd.handle_controlevent = CCD_Next;
    bgcd[3].gd.cid = CID_Next;
    bgcd[3].creator = GButtonCreate;
    barray[4] = GCD_Glue; barray[5] = &bgcd[3];

    bgcd[4].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    blabel[4].text = (unichar_t *) _("_Cancel");
    blabel[4].text_is_1byte = true;
    blabel[4].text_in_resource = true;
    bgcd[4].gd.label = &blabel[4];
    bgcd[4].gd.handle_controlevent = CCD_Cancel;
    bgcd[4].gd.cid = CID_Cancel;
    bgcd[4].creator = GButtonCreate;
    barray[6] = GCD_Glue; barray[7] = &bgcd[4]; barray[8] = GCD_Glue; barray[9] = NULL;

    boxes[2].gd.flags = gg_enabled | gg_visible;
    boxes[2].gd.u.boxelements = barray;
    boxes[2].creator = GHBoxCreate;
    hvarray[1][0] = &boxes[2]; hvarray[1][1] = hvarray[2][0] = NULL; 

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled | gg_visible;
    boxes[0].gd.u.boxelements = hvarray[0];
    boxes[0].creator = GHVGroupCreate;

    if ( fpst->type == pst_reversesub ) {
	bgcd[2].gd.flags = bgcd[3].gd.flags = gg_visible;
	ccd->aw = aw_coverage;
    } else if ( ccd->isnew || fpst->rule_cnt==0) {
	fpst->format = pst_class;
	bgcd[1].gd.flags = gg_visible;
	bgcd[3].gd.flags = gg_visible | gg_enabled;
	ccd->aw = aw_formats;
    } else if ( fpst->format==pst_coverage ) {
	bgcd[2].gd.flags = gg_visible | gg_enabled;
	bgcd[3].gd.flags = gg_visible;
	ccd->aw = aw_coverage;	/* flags are different from those of reversesub above */
    } else if ( fpst->format==pst_class ) {
	bgcd[2].gd.flags = gg_visible | gg_enabled;
	bgcd[3].gd.flags = gg_visible;
	ccd->aw = aw_class;
    } else {
	bgcd[2].gd.flags = gg_visible | gg_enabled;
	bgcd[3].gd.flags = gg_visible;
	ccd->aw = aw_glist;
    }

    GGadgetsCreate(gw,boxes);
    GHVBoxSetExpandableRow(boxes[0].ret,0);
    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandgluesame);
    /*GHVBoxFitWindow(boxes[0].ret);*/

    wattrs.mask = wam_events;
    ccd->formats = GWidgetCreateSubWindow(ccd->gw,&subpos,subccd_e_h,ccd,&wattrs);
    memset(&rlabel,0,sizeof(rlabel));
    memset(&rgcd,0,sizeof(rgcd));

    i = 0;
    rgcd[i].gd.pos.x = 5; rgcd[i].gd.pos.y = 5+i*13;
    rgcd[i].gd.flags = gg_visible | gg_enabled;
    rlabel[i].text = (unichar_t *) _(
	    "OpenType Contextual or Chaining subtables may be in one\n"
	    " of three formats. The context may be specified either\n"
	    " as a string of specific glyphs, a string of glyph classes\n"
	    " or a string of coverage tables\n"
	    "In the first format you must specify a string of glyph-names\n"
	    " In the second format you must specify a string of class numbers\n"
	    " In the third format you must specify a string each element\n"
	    "  of which may contain several glyph-names\n"
	    "For chaining subtables you may also specify backtrack and\n"
	    " lookahead lists.");
    rlabel[i].text_is_1byte = true;
    rgcd[i].gd.label = &rlabel[i];
    rgcd[i++].creator = GLabelCreate;
    hvarray[0][0] = &rgcd[i-1]; hvarray[0][1] = NULL;

    rgcd[i].gd.flags = gg_visible | gg_enabled | (fpst->format==pst_glyphs ? gg_cb_on : 0 );
    rlabel[i].text = (unichar_t *) _("By Glyphs");
    rlabel[i].text_is_1byte = true;
    rgcd[i].gd.label = &rlabel[i];
    rgcd[i].gd.cid = CID_ByGlyph;
    rgcd[i++].creator = GRadioCreate;
    barray[0] = GCD_HPad10; barray[1] = &rgcd[i-1];

    rgcd[i].gd.pos.x = rgcd[i-1].gd.pos.x; rgcd[i].gd.pos.y = rgcd[i-1].gd.pos.y+16;
    rgcd[i].gd.flags = gg_visible | gg_enabled | (fpst->format==pst_class ? gg_cb_on : 0 );
    rlabel[i].text = (unichar_t *) _("By Classes");
    rlabel[i].text_is_1byte = true;
    rgcd[i].gd.label = &rlabel[i];
    rgcd[i].gd.cid = CID_ByClass;
    rgcd[i++].creator = GRadioCreate;
    barray[2] = &rgcd[i-1];

    rgcd[i].gd.pos.x = rgcd[i-1].gd.pos.x; rgcd[i].gd.pos.y = rgcd[i-1].gd.pos.y+16;
    rgcd[i].gd.flags = gg_visible | gg_enabled | (fpst->format!=pst_glyphs && fpst->format!=pst_class ? gg_cb_on : 0 );
    rlabel[i].text = (unichar_t *) _("By Coverage");
    rlabel[i].text_is_1byte = true;
    rgcd[i].gd.label = &rlabel[i];
    rgcd[i].gd.cid = CID_ByCoverage;
    rgcd[i].creator = GRadioCreate;
    barray[3] = &rgcd[i]; barray[4] = GCD_Glue; barray[5] = NULL;

    memset(boxes,0,sizeof(boxes));

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = barray;
    boxes[2].creator = GHBoxCreate;
    hvarray[1][0] = &boxes[2]; hvarray[1][1] = NULL;
    hvarray[2][0] = GCD_Glue; hvarray[2][1] = NULL; hvarray[3][0] = NULL;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = hvarray[0];
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(ccd->formats,boxes);
    GHVBoxSetExpandableRow(boxes[0].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandgluesame);


    ccd->glyphs = GWidgetCreateSubWindow(ccd->gw,&subpos,subccd_e_h,ccd,&wattrs);
    memset(&ggcd,0,sizeof(ggcd));
    memset(&fgcd,0,sizeof(fgcd));
    memset(&glabel,0,sizeof(glabel));
    memset(&flabel,0,sizeof(flabel));
    memset(&faspects,0,sizeof(faspects));
    for ( i=0; i<3; ++i ) {
	k = CCD_AddGlyphList(&ggcd[i][0],&glabel[i][0],(0*100+i*20), 5, 8*13+4 );

	if ( i==0 )
	    CCD_AddSeqLookup(&ggcd[i][k],&glabel[i][k],(i*20),ggcd[0][k-1].gd.pos.y+ggcd[0][k-1].gd.pos.height+10,
		    CCD_WIDTH-20,4*13+10);
    }

    j = 0;

    faspects[j].text = (unichar_t *) _("Match");
    faspects[j].text_is_1byte = true;
    faspects[j].selected = true;
    faspects[j].gcd = ggcd[j]; ++j;

    faspects[j].text = (unichar_t *) _("Backtrack");
    faspects[j].text_is_1byte = true;
    faspects[j].disabled = fpst->type==pst_contextpos || fpst->type==pst_contextsub;
    faspects[j].gcd = ggcd[j]; ++j;

    faspects[j].text = (unichar_t *) _("Lookahead");
    faspects[j].text_is_1byte = true;
    faspects[j].disabled = fpst->type==pst_contextpos || fpst->type==pst_contextsub;
    faspects[j].gcd = ggcd[j]; ++j;

    flabel[0].text = (unichar_t *) _("A list of glyphs:");
    flabel[0].text_is_1byte = true;
    fgcd[0].gd.label = &flabel[0];
    fgcd[0].gd.pos.x = 5; fgcd[0].gd.pos.y = 5;
    fgcd[0].gd.flags = gg_visible | gg_enabled;
    fgcd[0].creator = GLabelCreate;

    fgcd[1].gd.pos.x = 3; fgcd[1].gd.pos.y = 18;
    fgcd[1].gd.pos.width = CCD_WIDTH-10;
    fgcd[1].gd.pos.height = CCD_HEIGHT-60;
    fgcd[1].gd.u.tabs = faspects;
    fgcd[1].gd.flags = gg_visible | gg_enabled;
    fgcd[1].gd.cid = CID_MatchType;
    fgcd[1].creator = GTabSetCreate;
    GGadgetsCreate(ccd->glyphs,fgcd);


    ccd->coverage = GWidgetCreateSubWindow(ccd->gw,&subpos,subccd_e_h,ccd,&wattrs);
    memset(&clabel,0,sizeof(clabel));
    memset(&cgcd,0,sizeof(cgcd));

    for ( i=0; i<3; ++i ) {
	k = CCD_AddGList(&cgcd[i][0],&clabel[i][0],100+i*20,5,
		CCD_WIDTH-20,i==0 ? CCD_HEIGHT-250 : CCD_HEIGHT-150);

	if ( i==0 ) {
	    if ( ccd->fpst->type!=pst_reversesub )
		CCD_AddSeqLookup(&cgcd[i][k],&clabel[i][k],100,cgcd[0][k-1].gd.pos.y+40,
			CCD_WIDTH-20,4*13+10);
	    else {
		r = NULL;
		if ( fpst->rule_cnt==1 ) r = &fpst->rules[0];
		CCD_AddReplacements(&cgcd[i][k],&clabel[i][k],r,100,cgcd[0][k-1].gd.pos.y+40);
	    }
	}
	faspects[i].gcd = cgcd[i];
    }
    flabel[0].text = (unichar_t *) _("A coverage table:");
    fgcd[1].gd.cid = CID_MatchType+100;
    GGadgetsCreate(ccd->coverage,fgcd);
    if (( fpst->format==pst_coverage || fpst->format==pst_reversecoverage ) &&
	    fpst->rule_cnt==1 ) {
	for ( i=0; i<3; ++i ) {
	    if ( (&fpst->rules[0].u.coverage.ncnt)[i]!=0 )
		GGadgetSetList(cgcd[i][0].ret,clistlist(fpst,i),false);
	}
	if ( fpst->format==pst_coverage ) {
	    GTextInfo **ti = slistlist(&fpst->rules[0]);
	    GGadgetSetList(GWidgetGetControl(ccd->gw,CID_LookupList+100),ti,false);
	}
    }

    ccd->glist = GWidgetCreateSubWindow(ccd->gw,&subpos,subccd_e_h,ccd,&wattrs);
    memset(&gllabel,0,sizeof(gllabel));
    memset(&glgcd,0,sizeof(glgcd));

    k = 0;
    gllabel[k].text = (unichar_t *) _("A list of glyph lists:");
    gllabel[k].text_is_1byte = true;
    glgcd[k].gd.label = &gllabel[k];
    glgcd[k].gd.pos.x = 5; glgcd[k].gd.pos.y = 5;
    glgcd[k].gd.flags = gg_visible | gg_enabled;
    glgcd[k++].creator = GLabelCreate;

    k += CCD_AddGList(&glgcd[k],&gllabel[k],0,19,
	    CCD_WIDTH-20,CCD_HEIGHT-100);

    GGadgetsCreate(ccd->glist,glgcd);
    if ( fpst->format==pst_glyphs && fpst->rule_cnt>0 )
	GGadgetSetList(glgcd[1].ret,glistlist(fpst),false);


    ccd->cselect = GWidgetCreateSubWindow(ccd->gw,&subpos,subccd_e_h,ccd,&wattrs);
    memset(&cslabel,0,sizeof(cslabel));
    memset(&csgcd,0,sizeof(csgcd));

    CCD_AddGlyphList(csgcd,cslabel,100,5,8*13+4);
    GGadgetsCreate(ccd->cselect,csgcd);

    ccd->class = GWidgetCreateSubWindow(ccd->gw,&subpos,subccd_e_h,ccd,&wattrs);
    memset(&cllabel,0,sizeof(cllabel));
    memset(&clgcd,0,sizeof(clgcd));
    memset(&clabel,0,sizeof(clabel));
    memset(&cgcd,0,sizeof(cgcd));

    k=0;
    cllabel[k].text = (unichar_t *) _("List of lists of class numbers");
    cllabel[k].text_is_1byte = true;
    clgcd[k].gd.label = &cllabel[k];
    clgcd[k].gd.pos.x = 5; clgcd[k].gd.pos.y = 5;
    clgcd[k].gd.flags = gg_visible | gg_enabled;
    clgcd[k++].creator = GLabelCreate;

    k += CCD_AddGList(&clgcd[k],&cllabel[k],200,19,
	    CCD_WIDTH-20,CCD_HEIGHT-270);

    clgcd[k].gd.pos.x = 2; clgcd[k].gd.pos.y = clgcd[k-1].gd.pos.y+30;
    clgcd[k].gd.pos.width = CCD_WIDTH-23;
    clgcd[k].gd.flags = gg_visible | gg_enabled;
    clgcd[k++].creator = GLineCreate;

    for ( i=0; i<3; ++i ) {
	int l=0;
	clabel[i][l].text = (unichar_t *) _("Classes (Lists of lists of glyph names)");
	clabel[i][l].text_is_1byte = true;
	cgcd[i][l].gd.label = &clabel[i][l];
	cgcd[i][l].gd.pos.x = 5; cgcd[i][l].gd.pos.y = 5;
	cgcd[i][l].gd.flags = gg_visible | gg_enabled;
	cgcd[i][l++].creator = GLabelCreate;

	if ( i!=0 ) {
	    clabel[i][l].text = (unichar_t *) _("Same as Match Classes");
	    clabel[i][l].text_is_1byte = true;
	    cgcd[i][l].gd.label = &clabel[i][l];
	    cgcd[i][l].gd.pos.x = 190; cgcd[i][l].gd.pos.y = 7;
	    cgcd[i][l].gd.handle_controlevent = CCD_SameAsClasses;
	    cgcd[i][l].gd.flags = gg_visible | gg_enabled;
	    cgcd[i][l].gd.cid = CID_SameAsClasses + i*20;
	    cgcd[i][l++].creator = GCheckBoxCreate;
	}

	CCD_AddGList(&cgcd[i][l],&clabel[i][l],300+i*20,cgcd[i][0].gd.pos.y+15,
		CCD_WIDTH-20,CCD_HEIGHT-270);
	faspects[i].gcd = cgcd[i];
    }
    j=0;
    faspects[j++].text = (unichar_t *) _("Match Classes");
    faspects[j++].text = (unichar_t *) _("Back Classes");
    faspects[j++].text = (unichar_t *) _("Ahead Classes");

    clgcd[k].gd.pos.x = 3; clgcd[k].gd.pos.y = clgcd[k-1].gd.pos.y+5;
    clgcd[k].gd.pos.width = CCD_WIDTH-10;
    clgcd[k].gd.pos.height = CCD_HEIGHT-185;
    clgcd[k].gd.u.tabs = faspects;
    clgcd[k].gd.flags = gg_visible | gg_enabled;
    clgcd[k].gd.cid = CID_MatchType+300;
    clgcd[k].creator = GTabSetCreate;
    GGadgetsCreate(ccd->class,clgcd);

    tempfpst = fpst;
    if ( fpst->format==pst_glyphs && fpst->rule_cnt>0 )
	tempfpst = FPSTGlyphToClass(fpst);
    if ( tempfpst->format==pst_class && tempfpst->rule_cnt>0 ) {
	GGadgetSetList(GWidgetGetControl(ccd->class,CID_GList+200),clslistlist(tempfpst),false);
	for ( i=0; i<3; ++i ) {
	    GGadget *list = GWidgetGetControl(ccd->class,CID_GList+300+i*20);
	    if ( i!=0 || tempfpst->nclass[0]==NULL )
		GListAppendLine8(list,_("{Everything Else}"),false);
	    else
		GListAppendLine8(list,(&tempfpst->nclass)[i][0],false);
	    for ( k=1; k<(&tempfpst->nccnt)[i]; ++k ) {
		GListAppendLine8(list,(&tempfpst->nclass)[i][k],false);
	    }
	    if ( i!=0 && ClassesMatch((&tempfpst->nccnt)[i],(&tempfpst->nclass)[i],tempfpst->nccnt,tempfpst->nclass)) {
		GGadgetSetChecked(GWidgetGetControl(ccd->class,CID_SameAsClasses+i*20),true);
		CCD_SameAsClasses(GWidgetGetControl(ccd->class,CID_SameAsClasses+i*20),NULL);
	    }
	}
	if ( tempfpst!=fpst )
	    FPSTFree(tempfpst);
    } else {
	for ( i=0; i<3; ++i )
	    GListAppendLine8(GWidgetGetControl(ccd->class,CID_GList+300+i*20),
		    _("{Everything Else}"),false);
	for ( i=1; i<3; ++i ) {
	    GGadgetSetChecked(GWidgetGetControl(ccd->class,CID_SameAsClasses+i*20),true);
	    CCD_SameAsClasses(GWidgetGetControl(ccd->class,CID_SameAsClasses+i*20),NULL);
	}
    }

    ccd->classbuild = GWidgetCreateSubWindow(ccd->gw,&subpos,subccd_e_h,ccd,&wattrs);
    memset(&cslabel,0,sizeof(cslabel));
    memset(&csgcd,0,sizeof(csgcd));

    CCD_AddGlyphList(csgcd,cslabel,300,5,8*13+4);
    GGadgetsCreate(ccd->classbuild,csgcd);

    ccd->classnumber = GWidgetCreateSubWindow(ccd->gw,&subpos,subccd_e_h,ccd,&wattrs);
    memset(&clabel,0,sizeof(clabel));
    memset(&cgcd,0,sizeof(cgcd));
    memset(&cllabel,0,sizeof(cllabel));
    memset(&clgcd,0,sizeof(clgcd));

    for ( i=0; i<3; ++i ) {
	k=0;

	clabel[i][k].text = (unichar_t *) _("List of class numbers");
	clabel[i][k].text_is_1byte = true;
	cgcd[i][k].gd.label = &clabel[i][k];
	cgcd[i][k].gd.pos.x = 5; cgcd[i][k].gd.pos.y = 5;
	cgcd[i][k].gd.flags = gg_visible | gg_enabled;
	cgcd[i][k++].creator = GLabelCreate;

	cgcd[i][k].gd.pos.x = 5; cgcd[i][k].gd.pos.y = cgcd[i][k-1].gd.pos.y+14;
	cgcd[i][k].gd.pos.width = CCD_WIDTH-25; cgcd[i][k].gd.pos.height = 4*13+4;
	cgcd[i][k].gd.flags = gg_visible | gg_enabled | gg_textarea_wrap;
	cgcd[i][k].gd.cid = CID_ClassNumbers+i*20;
	cgcd[i][k++].creator = GTextAreaCreate;

	clabel[i][k].text = (unichar_t *) _("Classes");
	clabel[i][k].text_is_1byte = true;
	cgcd[i][k].gd.label = &clabel[i][k];
	cgcd[i][k].gd.pos.x = 5; cgcd[i][k].gd.pos.y = cgcd[i][k-1].gd.pos.y+cgcd[i][k-1].gd.pos.height+5;
	cgcd[i][k].gd.flags = gg_visible | gg_enabled;
	cgcd[i][k++].creator = GLabelCreate;

	cgcd[i][k].gd.pos.x = 5; cgcd[i][k].gd.pos.y = cgcd[i][k-1].gd.pos.y+14;
	cgcd[i][k].gd.pos.width = CCD_WIDTH-25; cgcd[i][k].gd.pos.height = 4*13+10;
	cgcd[i][k].gd.flags = gg_visible | gg_enabled | gg_textarea_wrap;
	cgcd[i][k].gd.handle_controlevent = CCD_ClassSelected;
	cgcd[i][k].gd.cid = CID_ClassList+i*20;
	cgcd[i][k++].creator = GListCreate;

	if ( i==0 )
	    CCD_AddSeqLookup(&cgcd[i][k],&clabel[i][k],500,cgcd[0][k-1].gd.pos.y+cgcd[0][k-1].gd.pos.height+5,
		    CCD_WIDTH-20,4*13+10);
	faspects[i].gcd = cgcd[i];
    }

    j=0;
    faspects[j++].text = (unichar_t *) _("Match");
    faspects[j++].text = (unichar_t *) _("Backtrack");
    faspects[j++].text = (unichar_t *) _("Lookahead");

    k=0;
    clgcd[k].gd.pos.x = 3; clgcd[k].gd.pos.y = 5;
    clgcd[k].gd.pos.width = CCD_WIDTH-10;
    clgcd[k].gd.pos.height = CCD_HEIGHT-45;
    clgcd[k].gd.u.tabs = faspects;
    clgcd[k].gd.flags = gg_visible | gg_enabled;
    clgcd[k].gd.cid = CID_ClassType;
    clgcd[k].creator = GTabSetCreate;
    GGadgetsCreate(ccd->classnumber,clgcd);


    if ( ccd->aw == aw_formats )
	GDrawSetVisible(ccd->formats,true);
    else if ( ccd->aw == aw_glist )
	GDrawSetVisible(ccd->glist,true);
    else if ( ccd->aw == aw_class )
	GDrawSetVisible(ccd->class,true);
    else
	GDrawSetVisible(ccd->coverage,true);

    GDrawSetVisible(gw,true);

return( ccd );
}
