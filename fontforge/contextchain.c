/* Copyright (C) 2003-2011 by George Williams */
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
/* Removed: cselect, classbuild */
    GWindow formats, coverage, grules, glyphs, classrules, classnumber;
    enum activewindow { aw_formats, aw_coverage, aw_grules, aw_glyphs,
	    aw_classrules, aw_classnumber } aw;
/* Wizard panes:
  formats    -- gives user a choice between by glyph/class/coverage table
  grules     -- user picked glyph format, shows a list of glyph lists and with
		  lookups. Editing a glyph list takes us to the glyphs pane
  classrules -- user picked class format. shows a list of class lists with
		  lookups. Also a tabbed widget showing the three sets of
		  classes.
  coverage   -- user picked coverage table format. shows a tabbed widget with
		  three lists of coverage tables. (for match also shows a
		  set of sequence #/lookup pairs.
		  [There is no coveragerules pane because coverage format
		   only accepts 1 rule]
  glyphs     -- from grules, user asked for a new rule. Shows tabbed widget
		  with three lists of glyphs (for match, also shows a set of
		  sequence #/lookups
  classnumber - from class rules, tabbed pane describing one class based rule
*/
    int row_being_edited;
    uint8 foredefault;
    int subheightdiff, canceldrop;
    int done;
    int layer;
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
/* The CIDs given here are for glyph lists (aw_glyphs & aw_grules) */
/*  similar controls refering to coverage tables have 100 added to them */
/* The CIDs here are also for the "match" aspect of the tabsets */
/*  those in backtrack get 20 added, those in lookahead get 40 */
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

#define CID_ClassNumbers	2000
#define CID_ClassList		2001
#define CID_ClassType		2002
#define CID_SameAsClasses	2003

char *cu_copybetween(const unichar_t *start, const unichar_t *end) {
    char *ret = galloc(end-start+1);
    cu_strncpy(ret,start,end-start);
    ret[end-start] = '\0';
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

static char *classnumbers(int cnt,uint16 *classes) {
    char buf[20];
    int i, len;
    char *pt, *ret;

    len = 0;
    for ( i=0; i<cnt; ++i ) {
	sprintf( buf, "%d ", classes[i]);
	len += strlen(buf);
    }
    ret = pt = galloc(len+3);
    *pt = '\0';		/* In case it is empty */

    for ( i=0; i<cnt; ++i ) {
	sprintf( pt, "%d ", classes[i]);
	pt += strlen(pt);
    }
return( ret );
}

static char *rclassnumbers(int cnt,uint16 *classes) {
    char buf[20];
    int i, len;
    char *pt, *ret;

    len = 0;
    for ( i=0; i<cnt; ++i ) {
	sprintf( buf, "%d ", classes[i]);
	len += strlen(buf);
    }
    ret = pt = galloc(len+3);
    *pt = '\0';
    for ( i=cnt-1; i>=0; --i ) {
	sprintf( pt, "%d ", classes[i]);
	pt += strlen(pt);
    }
return( ret );
}

static int CCD_GlyphNameCnt(const char *pt) {
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

    len += 4;		/* for the arrow, takes 3 bytes in utf8 */
    for ( i=0; i<r->lookup_cnt; ++i ) {
	sprintf( buf," %d \"\",", r->lookups[i].seq );
	len += strlen(buf) + strlen( r->lookups[i].lookup->lookup_name );
    }
return( len );
}

static char *addseqlookups(char *pt, struct fpst_rule *r) {
    int i;

    pt = utf8_idpb(pt, 0x21d2);
    for ( i=0; i<r->lookup_cnt; ++i ) {
	sprintf( pt," %d \"%s\",", r->lookups[i].seq, r->lookups[i].lookup->lookup_name);
	pt += strlen(pt);
    }
    if ( pt[-1]==',' ) --pt;
    *pt = '\0';
return( pt );
}

static void parseseqlookups(SplineFont *sf, const char *solooks, struct fpst_rule *r) {
    int cnt;
    const char *pt;

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
	char *end;
	r->lookups[cnt].seq = strtol(pt,&end,10);
	pt = end+1;
	if ( *pt=='"' ) {
	    const char *eoname; char *temp;
	    ++pt;
	    for ( eoname = pt; *eoname!='\0' && *eoname!='"'; ++eoname );
	    temp = copyn(pt,eoname-pt);
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

static char *gruleitem(struct fpst_rule *r) {
    char *ret, *pt;
    int len;

    len = (r->u.glyph.back==NULL ? 0 : strlen(r->u.glyph.back)) +
	    strlen(r->u.glyph.names) +
	    (r->u.glyph.fore==0 ? 0 : strlen(r->u.glyph.fore)) +
	    seqlookuplen(r);

    ret = pt = galloc(len+8);
    if ( r->u.glyph.back!=NULL && *r->u.glyph.back!='\0' ) {
	char *temp = reversenames(r->u.glyph.back);
	strcpy(pt,temp);
	pt += strlen(temp);
	free(temp);
	*pt++ = ' ';
    }
    *pt++ = '[';
    *pt++ = ' ';
    strcpy(pt,r->u.glyph.names);
    pt += strlen(r->u.glyph.names);
    *pt++ = ' ';
    *pt++ = ']';
    *pt++ = ' ';
    if ( r->u.glyph.fore!=NULL  && *r->u.glyph.fore!='\0' ) {
	strcpy(pt,r->u.glyph.fore);
	pt += strlen(r->u.glyph.fore);
	*pt++ = ' ';
    }
    pt = addseqlookups(pt, r);
return( ret );
}

static void gruleitem2rule(SplineFont *sf, const char *ruletext,struct fpst_rule *r) {
    const char *pt, *pt2;
    char *temp, *freeme;
    int ch;

    if ( ruletext==NULL )
return;
    for ( pt=ruletext; *pt!='\0' && *pt!='['; ++pt );
    if ( *pt=='\0' )
return;
    if ( pt>ruletext ) {
	temp = GlyphNameListDeUnicode(freeme = copyn(ruletext,pt-1-ruletext));
	r->u.glyph.back = reversenames(temp);
	free(temp); free(freeme);
    }
    ruletext = pt+2;
    for ( pt=ruletext; *pt!='\0' && *pt!=']'; ++pt );
    if ( *pt=='\0' )
return;
    r->u.glyph.names = GlyphNameListDeUnicode(freeme = copyn(ruletext,pt-1-ruletext));
    free(freeme);
    ruletext = pt+2;
    for ( pt2=ruletext; (ch=utf8_ildb((const char **) &pt2))!='\0' && ch!=0x21d2; );
    if ( ch=='\0' )
return;
    if ( pt2!=ruletext ) {
	r->u.glyph.fore = GlyphNameListDeUnicode(freeme = copyn(ruletext,pt2-3-ruletext));
	free(freeme);
    }
    parseseqlookups(sf,pt2+2,r);
}

static char *classruleitem(struct fpst_rule *r) {
    char *ret, *pt;
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
	sprintf( pt, "%d ", r->u.class.bclasses[k]);
	pt += strlen(pt);
    }
    *pt++ = '[';
    for ( k=0; k<r->u.class.ncnt; ++k ) {
	sprintf( pt, "%d ", r->u.class.nclasses[k]);
	pt += strlen(pt);
    }
    if ( pt[-1]==' ' ) --pt;
    *pt++ = ']';
    for ( k=0; k<r->u.class.fcnt; ++k ) {
	sprintf( pt, " %d", r->u.class.fclasses[k]);
	pt += strlen(pt);
    }

    *pt++ = ' ';
    pt = addseqlookups(pt, r);
return( ret );
}

static void classruleitem2rule(SplineFont *sf,const char *ruletext,struct fpst_rule *r) {
    const char *pt; char *end;
    int len, i, ch;

    memset(r,0,sizeof(*r));

    len = 0; i=1;
    for ( pt=ruletext; *pt; ) {
	while ( (ch = utf8_ildb((const char **) &pt))!='[' && ch!=']' && ch!=0x21d2 && ch!='\0' ) {
	    if ( isdigit( ch )) {
		++len;
		while ( isdigit(*pt)) ++pt;
	    }
	}
	(&r->u.class.ncnt)[i] = len;
	(&r->u.class.nclasses)[i] = galloc(len*sizeof(uint16));
	len = 0;
	if ( ch=='\0' || ch==0x21d2 )
    break;
	if ( ch=='[' ) i=0;
	else i=2;
    }
    len = 0; i=1;
    for ( pt=ruletext; *pt; ) {
	while ( (ch = utf8_ildb((const char **) &pt))!='[' && ch!=']' && ch!=0x21d2 && ch!='\0' ) {
	    if ( isdigit( ch )) {
		if ( len>=(&r->u.class.ncnt)[i] )
		    IError( "classruleitem2rule bad memory");
		(&r->u.class.nclasses)[i][len++] = strtol(pt-1,&end,10);
		pt = end;
	    }
	}
	len = 0;
	if ( ch=='[' ) i=0;
	else i=2;
	if ( ch=='\0' || ch==0x21d2 )
    break;
    }
    /* reverse the backtrack */
    for ( i=0; i<r->u.class.bcnt/2; ++i ) {
	int temp = r->u.class.bclasses[i];
	r->u.class.bclasses[i] = r->u.class.bclasses[r->u.class.bcnt-1-i];
	r->u.class.bclasses[r->u.class.bcnt-1-i] = temp;
    }

    if ( ch=='\0' || *pt=='\0' )
return;
    parseseqlookups(sf,pt,r);
}

static void CCD_ParseLookupList(SplineFont *sf, struct fpst_rule *r,GGadget *list) {
    int len, i;
    struct matrix_data *classes = GMatrixEditGet(list,&len);

    r->lookup_cnt = len;
    r->lookups = galloc(len*sizeof(struct seqlookup));
    for ( i=0; i<len; ++i ) {
	r->lookups[i].seq = classes[2*i+1].u.md_ival;
	r->lookups[i].lookup = (OTLookup *) classes[2*i+0].u.md_ival;
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
      case aw_classnumber:
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_Prev),true);
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_Next),true);
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_OK),false);
      break;
      case aw_coverage:
      case aw_grules:
      case aw_classrules:
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_Prev),ccd->fpst->format!=pst_reversecoverage);
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_Next),false);
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_OK),true);
      break;
      default:
	IError("Can't get here");
      break;
    }
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

static void CCD_FinishRule(struct contextchaindlg *ccd) {
    struct fpst_rule dummy;
    GGadget *list;
    int i,tot;
    char *buts[3];
    buts[0] = _("_Yes"); buts[1] = _("_No"); buts[2] = NULL;
    struct matrix_data *md;
    int len;

    if ( ccd->aw==aw_classnumber ) {
	char *ret;
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
	ret = classruleitem(&dummy);
	FPSTRuleContentsFree(&dummy,pst_class);
	list = GWidgetGetControl(ccd->gw,CID_GList+200);
	md = GMatrixEditGet(list,&len);
	free(md[ccd->row_being_edited].u.md_str);
	md[ccd->row_being_edited].u.md_str = ret;
	GMatrixEditSet(list,md,len,false);
	ccd->aw = aw_classrules;
	GDrawSetVisible(ccd->classnumber,false);
	GDrawSetVisible(ccd->classrules,true);
    } else if ( ccd->aw==aw_glyphs ) {			/* It's from glyph list */
	char *ret, *temp, *freeme;

	memset(&dummy,0,sizeof(dummy));
	CCD_ParseLookupList(ccd->sf,&dummy,GWidgetGetControl(ccd->gw,CID_LookupList));
	if ( dummy.lookup_cnt==0 ) {
	    int ans = gwwv_ask(_("No Sequence/Lookups"),
		    (const char **) buts,0,1,
		    _("There are no entries in the Sequence/Lookup List, was this intentional?"));
	    if ( ans==1 )
return;
	}
	temp = GGadgetGetTitle8(GWidgetGetControl(ccd->gw,CID_GlyphList));
	dummy.u.glyph.names = GlyphNameListDeUnicode(temp);
	free(temp);
	tot = CCD_GlyphNameCnt( temp = GGadgetGetTitle8(GWidgetGetControl(ccd->gw,CID_GlyphList)));
	free(temp);
	for ( i=0; i<dummy.lookup_cnt; ++i ) {
	    if ( dummy.lookups[i].seq >= tot ) {
		ff_post_error(_("Bad Sequence/Lookup List"),_("Sequence number out of bounds, must be less than %d (number of glyphs, classes or coverage tables)"),  tot );
return;
	    }
	}
	temp = GGadgetGetTitle8(GWidgetGetControl(ccd->gw,CID_GlyphList+20));
	freeme = GlyphNameListDeUnicode(temp);
	dummy.u.glyph.back = reversenames(freeme);
	free(temp); free(freeme);
	temp = GGadgetGetTitle8(GWidgetGetControl(ccd->gw,CID_GlyphList+40));
	dummy.u.glyph.fore = GlyphNameListDeUnicode(temp);
	free(temp);
	ret = gruleitem(&dummy);
	FPSTRuleContentsFree(&dummy,pst_glyphs);
	list = GWidgetGetControl(ccd->gw,CID_GList+0);
	md = GMatrixEditGet(list,&len);
	free(md[ccd->row_being_edited].u.md_str);
	md[ccd->row_being_edited].u.md_str = ret;
	GMatrixEditSet(list,md,len,false);
	ccd->aw = aw_grules;
	GDrawSetVisible(ccd->glyphs,false);
	GDrawSetVisible(ccd->grules,true);
    } else {
	IError("Bad format in CCD_FinishRule");
    }
}

static int isEverythingElse(char *text) {
    /* GT: The string "{Everything Else}" is used in the context of a list */
    /* GT: of classes (a set of kerning classes) where class 0 designates the */
    /* GT: default class containing all glyphs not specified in the other classes */
    int ret = strcmp(text,_("{Everything Else}"));
return( ret==0 );
}

static char *CCD_PickGlyphsForClass(GGadget *g,int r, int c) {
    struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));
    int rows, cols = GMatrixEditGetColCnt(g);
    struct matrix_data *classes = _GMatrixEditGet(g,&rows);
    char *new = GlyphSetFromSelection(ccd->sf,ccd->layer,classes[r*cols+c].u.md_str);
return( new );
}

static void _CCD_FromSelection(struct contextchaindlg *ccd,int cid ) {
    char *curval = GGadgetGetTitle8(GWidgetGetControl(ccd->gw,cid));
    char *new = GlyphSetFromSelection(ccd->sf,ccd->layer,curval);

    free(curval);
    if ( new==NULL )
return;
    GGadgetSetTitle8(GWidgetGetControl(ccd->gw,cid),new);
    free(new);
}

static int CCD_FromSelection(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));
	int cid = (intpt) GGadgetGetUserData(g);

	_CCD_FromSelection(ccd,cid);
    }
return( true );
}

static GTextInfo **MD2TI(struct matrix_data *md,int len) {
    GTextInfo **ti = gcalloc(len+1,sizeof(GTextInfo *));
    int i;

    for ( i=0; i<len; ++i ) {
	ti[i] = gcalloc(1,sizeof(GTextInfo));
	ti[i]->text = utf82u_copy(md[i].u.md_str);
	ti[i]->fg = ti[i]->bg = COLOR_DEFAULT;
    }
    ti[i] = gcalloc(1,sizeof(GTextInfo));
return( ti );
}

static char *CCD_NewGlyphRule(GGadget *glyphrules,int r,int c) {
    struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(glyphrules));
    int rows;
    struct matrix_data *rulelist = GMatrixEditGet(glyphrules,&rows);
    struct fpst_rule dummy;
    GGadget *lookuplist = GWidgetGetControl(ccd->glyphs,CID_LookupList+0);
    struct matrix_data *md;
    char *temp2;
    int j;

    memset(&dummy,0,sizeof(dummy));
    gruleitem2rule(ccd->sf,rulelist[r].u.md_str,&dummy);
    GGadgetSetTitle8(GWidgetGetControl(ccd->gw,CID_GlyphList),dummy.u.glyph.names==NULL?"":dummy.u.glyph.names);
    GGadgetSetTitle8(GWidgetGetControl(ccd->gw,CID_GlyphList+20),(temp2 = reversenames(dummy.u.glyph.back))==NULL ? "" : temp2);
    free(temp2);
    GGadgetSetTitle8(GWidgetGetControl(ccd->gw,CID_GlyphList+40),dummy.u.glyph.fore!=NULL?dummy.u.glyph.fore:"");

    md = gcalloc(2*dummy.lookup_cnt,sizeof(struct matrix_data));
    for ( j=0; j<dummy.lookup_cnt; ++j ) {
	md[2*j+0].u.md_ival = (intpt) (void *) dummy.lookups[j].lookup;
	md[2*j+1].u.md_ival = (intpt) dummy.lookups[j].seq;
    }
    GMatrixEditSet(lookuplist,md,dummy.lookup_cnt,false);
    ccd->aw = aw_glyphs;
    ccd->row_being_edited = r;
    GDrawSetVisible(ccd->grules,false);
    GDrawSetVisible(ccd->glyphs,true);
    CCD_EnableNextPrev(ccd);
return( copy( rulelist[r].u.md_str ) );		/* We change the display to get the new value, but we must return something... */
}

static char *CCD_NewClassRule(GGadget *classrules,int r,int c) {
    struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(classrules));
    int rows;
    struct matrix_data *rulelist = GMatrixEditGet(classrules,&rows);
    struct fpst_rule dummy;
    GGadget *lookuplist = GWidgetGetControl(ccd->classnumber,CID_LookupList+500);
    struct matrix_data *md;
    int len;
    int i,j;
    char *temp;

    memset(&dummy,0,sizeof(dummy));
    classruleitem2rule(ccd->sf,rulelist[r].u.md_str,&dummy);
    GGadgetSetTitle8(GWidgetGetControl(ccd->gw,CID_ClassNumbers),
	    (temp=classnumbers(dummy.u.class.ncnt,dummy.u.class.nclasses)));
    free(temp);
    GGadgetSetTitle8(GWidgetGetControl(ccd->gw,CID_ClassNumbers+20),
	    (temp=rclassnumbers(dummy.u.class.bcnt,dummy.u.class.bclasses)));
    free(temp);
    GGadgetSetTitle8(GWidgetGetControl(ccd->gw,CID_ClassNumbers+40),
	    (temp=classnumbers(dummy.u.class.fcnt,dummy.u.class.fclasses)));
    free(temp);

    md = gcalloc(2*dummy.lookup_cnt,sizeof(struct matrix_data));
    for ( j=0; j<dummy.lookup_cnt; ++j ) {
	md[2*j+0].u.md_ival = (intpt) (void *) dummy.lookups[j].lookup;
	md[2*j+1].u.md_ival = (intpt) dummy.lookups[j].seq;
    }
    GMatrixEditSet(lookuplist,md,dummy.lookup_cnt,false);
    FPSTRuleContentsFree(&dummy,pst_class);
    GWidgetIndicateFocusGadget(GWidgetGetControl(ccd->gw,CID_ClassNumbers));
    for ( i=0; i<3; ++i ) {
	if ( i!=0 && GGadgetIsChecked(GWidgetGetControl(ccd->gw,CID_SameAsClasses+i*20)) )
	    md = GMatrixEditGet(GWidgetGetControl(ccd->gw,CID_GList+300),&len);
	else
	    md = GMatrixEditGet(GWidgetGetControl(ccd->gw,CID_GList+300+i*20),&len);
	GGadgetSetList(GWidgetGetControl(ccd->gw,CID_ClassList+i*20),MD2TI(md,len),false);
    }
    ccd->aw = aw_classnumber;
    ccd->row_being_edited = r;
    GDrawSetVisible(ccd->classrules,false);
    GDrawSetVisible(ccd->classnumber,true);
    CCD_EnableNextPrev(ccd);
return( copy( rulelist[r].u.md_str ) );		/* We change the display to get the new value, but we must return something... */
}

static int CCD_SameAsClasses(GGadget *g, GEvent *e) {
    int ison = GGadgetIsChecked(g);
    int off = GGadgetGetCid(g)-CID_SameAsClasses;
    struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));

    GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_GList+300+off),!ison);
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

static void CCD_Close(struct contextchaindlg *ccd) {
    if ( ccd->isnew )
	GFI_FinishContextNew(ccd->gfi,ccd->fpst,false);
    ccd->done = true;
}

static char **CCD_ParseCoverageList(struct contextchaindlg *ccd,int cid,int *cnt) {
    int i;
    int _cnt;
    struct matrix_data *covers = GMatrixEditGet(GWidgetGetControl(ccd->gw,cid),&_cnt);
    char **ret;

    *cnt = _cnt;
    if ( _cnt==0 )
return( NULL );
    ret = galloc(*cnt*sizeof(char *));
    for ( i=0; i<_cnt; ++i )
	ret[i] = GlyphNameListDeUnicode(covers[i].u.md_str);
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
    int len, i, k, had_class0;
    struct matrix_data *old, *classes;
    struct fpst_rule dummy;
    int has[3];
    char *buts[3];
    buts[0] = _("_Yes"); buts[1] = _("_No"); buts[2] = NULL;
    char *temp;

    switch ( ccd->aw ) {
      case aw_grules: {
	old = GMatrixEditGet(GWidgetGetControl(ccd->gw,CID_GList),&len);
	if ( len==0 ) {
	    ff_post_error(_("Missing rules"),_(" There must be at least one contextual rule"));
return;
	}
	FPSTRulesFree(fpst->rules,fpst->format,fpst->rule_cnt);
	fpst->format = pst_glyphs;
	fpst->rule_cnt = len;
	fpst->rules = gcalloc(len,sizeof(struct fpst_rule));
	for ( i=0; i<len; ++i )
	    gruleitem2rule(ccd->sf,old[i].u.md_str,&fpst->rules[i]);
      } break;
      case aw_classrules: {
	old = GMatrixEditGet(GWidgetGetControl(ccd->gw,CID_GList+200),&len);
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
	    classruleitem2rule(ccd->sf,old[i].u.md_str,&fpst->rules[i]);
	    if ( fpst->rules[i].u.class.bcnt!=0 ) has[1] = true;
	    if ( fpst->rules[i].u.class.fcnt!=0 ) has[2] = true;
	}
	for ( i=0; i<3; ++i ) {
	    if ( i==0 || !GGadgetIsChecked(GWidgetGetControl(ccd->gw,CID_SameAsClasses+i*20)) )
		classes = GMatrixEditGet(GWidgetGetControl(ccd->gw,CID_GList+300+i*20),&len);
	    else if ( has[i] )
		classes = GMatrixEditGet(GWidgetGetControl(ccd->gw,CID_GList+300),&len);
	    else
	continue;
	    (&fpst->nccnt)[i] = len;
	    (&fpst->nclass)[i] = galloc(len*sizeof(char*));
	    (&fpst->nclass)[i][0] = NULL;
	    had_class0 = i==0 && !isEverythingElse(classes[0].u.md_str);
	    for ( k=had_class0 ? 0 : 1 ; k<len; ++k )
		(&fpst->nclass)[i][k] = GlyphNameListDeUnicode(classes[k].u.md_str);
	}
      } break;
      case aw_coverage:
	old = GMatrixEditGet(GWidgetGetControl(ccd->gw,CID_GList+100),&len);
	if ( len==0 ) {
	    ff_post_error(_("Bad Coverage Table"),_("There must be at least one match coverage table"));
return;
	}
	if ( fpst->format==pst_reversecoverage ) {
	    if ( len!=1 ) {
		ff_post_error(_("Bad Coverage Table"),_("In a Reverse Chaining Substitution there must be exactly one coverage table to match"));
return;
	    }
	    if ( CCD_GlyphNameCnt(old[0].u.md_str)!=CCD_GlyphNameCnt(
		    temp=GGadgetGetTitle8(GWidgetGetControl(ccd->gw,CID_RplList+100))) ) {
		free(temp);
		ff_post_error(_("Replacement mismatch"),_("In a Reverse Chaining Substitution there must be exactly as many replacements as there are glyph names in the match coverage table"));
return;
	    }
	    dummy.u.rcoverage.replacements = GlyphNameListDeUnicode(temp);
	    free(temp);
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
    ccd->done = true;
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
		ccd->aw = aw_grules;
		GDrawSetVisible(ccd->grules,true);
	    } else if ( GGadgetIsChecked(GWidgetGetControl(ccd->gw,CID_ByClass)) ) {
		ccd->aw = aw_classrules;
		GDrawSetVisible(ccd->classrules,true);
	    } else {
		ccd->aw = aw_coverage;
		GDrawSetVisible(ccd->coverage,true);
	    }
	  break;
	  case aw_glyphs:
	  case aw_classnumber:
	    CCD_FinishRule(ccd);
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
	    ccd->aw = aw_grules;
	    GDrawSetVisible(ccd->glyphs,false);
	    GDrawSetVisible(ccd->grules,true);
	  break;
	  case aw_classnumber:
	    ccd->aw = aw_classrules;
	    GDrawSetVisible(ccd->classnumber,false);
	    GDrawSetVisible(ccd->classrules,true);
	  break;
	  case aw_coverage:
	  case aw_classrules:
	  case aw_grules:
	    ccd->aw = aw_formats;
	    GDrawSetVisible(ccd->coverage,false);
	    GDrawSetVisible(ccd->classrules,false);
	    GDrawSetVisible(ccd->grules,false);
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

    /* figure out what the default action should be */
    memset(&ev,0,sizeof(ev));
    ev.type = et_controlevent;
    ev.u.control.subtype = et_buttonactivate;
    if ( ccd->aw==aw_formats || ccd->aw == aw_glyphs ) {
	ev.u.control.g = GWidgetGetControl(ccd->gw,CID_Next);
	CCD_Next(ev.u.control.g,&ev);
/* For the glyph list and the coverage list, the [Next] button is disabled */
/* I think [OK] is probably inappropriate, so let's do either [New] or [Edit] */
/*  (which are what [Next] would logically do) depending on which of the two */
/*  is more appropriate */
/*	ev.u.control.g = GWidgetGetControl(ccd->gw,CID_OK); */
/*	CCD_OK(ev.u.control.g,&ev);			    */
    } else if ( ccd->aw==aw_grules ) {
    } else {
    }
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
	GRect wsize;
	struct contextchaindlg *ccd = GDrawGetUserData(gw);

	GGadgetGetSize(GWidgetGetControl(ccd->gw,CID_SubSize),&wsize);

	GDrawResize(ccd->formats,wsize.width,wsize.height);
	GDrawResize(ccd->coverage,wsize.width,wsize.height);
	GDrawResize(ccd->grules,wsize.width,wsize.height);
	GDrawResize(ccd->glyphs,wsize.width,wsize.height);
	GDrawResize(ccd->classrules,wsize.width,wsize.height);
	GDrawResize(ccd->classnumber,wsize.width,wsize.height);

	GDrawRequestExpose(ccd->gw,NULL,false);
	GDrawRequestExpose(ccd->aw==aw_formats ? ccd->formats :
			    ccd->aw==aw_coverage ? ccd->coverage :
			    ccd->aw==aw_grules ? ccd->grules :
					ccd->glyphs, NULL,false);
    }
return( true );
}

#define CCD_WIDTH	340
#define CCD_HEIGHT	340

static unichar_t **CCD_GlyphListCompletion(GGadget *t,int from_tab) {
    struct contextchaindlg *ccd = GDrawGetUserData(GDrawGetParentWindow(GGadgetGetWindow(t)));
    SplineFont *sf = ccd->sf;

return( SFGlyphNameCompletion(sf,t,from_tab,true));
}

static void CCD_FinishCoverageEdit(GGadget *g,int r, int c, int wasnew) {
    struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));

    ME_SetCheckUnique(g, r, c, ccd->sf);
}


static void CCD_FinishClassEdit(GGadget *g,int r, int c, int wasnew) {
    struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));

    ME_ClassCheckUnique(g, r, c, ccd->sf);
}

static int CCD_EnableDeleteClass(GGadget *g,int whichclass) {
return( whichclass>0 );
}

static struct col_init class_ci[] = {
    { me_funcedit, CCD_PickGlyphsForClass, NULL, NULL, N_("Glyphs in the classes") },
    };
static struct col_init coverage_ci[] = {
    { me_funcedit, CCD_PickGlyphsForClass, NULL, NULL, N_("Glyphs in the coverage tables") },
    };
static struct col_init seqlookup_ci[] = {
    { me_enum, NULL, NULL, NULL, N_("Apply lookup") },
    { me_int, NULL, NULL, NULL, N_("at position") },
    };
static struct col_init glyphrules_ci[] = {
    { me_onlyfuncedit, CCD_NewGlyphRule, NULL, NULL, N_("Matching rules based on a list glyphs") },
    };
static struct col_init classrules_ci[] = {
    { me_onlyfuncedit, CCD_NewClassRule, NULL, NULL, N_("Matching rules based on a list of classes") },
    };

void ContextChainEdit(SplineFont *sf,FPST *fpst,
	struct gfi_data *gfi, unichar_t *newname,int layer) {
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
    GGadgetCreateData glgcd[8], fgcd[3], ggcd[3][14], cgcd[3][18],
	clgcd[10], subboxes[3][5], topbox[3], classbox[2];
    GTextInfo gllabel[8], flabel[3], glabel[3][14], clabel[3][18],
	cllabel[10];
    GGadgetCreateData bgcd[6], rgcd[15], boxes[3], *barray[10], *hvarray[4][2];
    GGadgetCreateData *subvarray[3][10], *subvarray2[4], *subvarray3[4], *topvarray[8],
	    *classv[4], *subvarray4[4];
    GTextInfo blabel[5], rlabel[15];
    struct fpst_rule *r=NULL;
    FPST *tempfpst;
    struct matrixinit coverage_mi[3], class_mi[3], gl_seqlookup_mi, cl_seqlookup_mi,
	co_seqlookup_mi, classrules_mi, glyphrules_mi;
    static int inited=false;
    int isgpos = ( fpst->type==pst_contextpos || fpst->type==pst_chainpos );
    struct matrix_data *md;
    GTextInfo *lookup_list;

    lookup_list = SFLookupArrayFromType(sf,isgpos?gpos_start:gsub_start);
    for ( i=0; lookup_list[i].text!=NULL; ++i ) {
	if (fpst!=NULL && lookup_list[i].userdata == fpst->subtable->lookup )
	    lookup_list[i].disabled = true;
    }
    seqlookup_ci[0].enum_vals = lookup_list;

    if ( !inited ) {
	inited = true;
	class_ci[0].title = S_(class_ci[0].title);
	coverage_ci[0].title = S_(coverage_ci[0].title);
	seqlookup_ci[0].title = S_(seqlookup_ci[0].title);
	seqlookup_ci[1].title = S_(seqlookup_ci[1].title);
	glyphrules_ci[0].title = S_(glyphrules_ci[0].title);
	classrules_ci[0].title = S_(classrules_ci[0].title);
    }
	
    ccd = chunkalloc(sizeof(struct contextchaindlg));
    ccd->gfi = gfi;
    ccd->sf = sf;
    ccd->fpst = fpst;
    ccd->newname = newname;
    ccd->isnew = newname!=NULL;
    ccd->layer = layer;

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
	ccd->aw = aw_classrules;
    } else {
	bgcd[2].gd.flags = gg_visible | gg_enabled;
	bgcd[3].gd.flags = gg_visible;
	ccd->aw = aw_grules;
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

	/* Pane showing all rules from a glyph format (not classes or coverage tables) */
    ccd->grules = GWidgetCreateSubWindow(ccd->gw,&subpos,subccd_e_h,ccd,&wattrs);
    memset(&gllabel,0,sizeof(gllabel));
    memset(&glgcd,0,sizeof(glgcd));

    k = 0;
    memset(&glyphrules_mi,0,sizeof(glyphrules_mi));
    glyphrules_mi.col_cnt = 1;
    glyphrules_mi.col_init = glyphrules_ci;
    if ( fpst->format==pst_glyphs && fpst->rule_cnt>0 ) {
	glyphrules_mi.initial_row_cnt = fpst->rule_cnt;
	md = gcalloc(fpst->rule_cnt,sizeof(struct matrix_data));
	for ( j=0; j<fpst->rule_cnt; ++j ) {
	    md[j+0].u.md_str = gruleitem(&fpst->rules[j]);
	}
	glyphrules_mi.matrix_data = md;
    } else {
	glyphrules_mi.initial_row_cnt = 0;
	glyphrules_mi.matrix_data = NULL;
    }
    glgcd[k].gd.flags = gg_visible | gg_enabled;
    glgcd[k].gd.cid = CID_GList+0;
    glgcd[k].gd.u.matrix = &glyphrules_mi;
    glgcd[k++].creator = GMatrixEditCreate;
    /* No need for a box, this widget fills the pane */

    GGadgetsCreate(ccd->grules,glgcd);
    GMatrixEditSetUpDownVisible(GWidgetGetControl(ccd->grules,CID_GList+0),
	    true);

	/* Pane showing a single rule from a glyph format (not classes or coverage tables) */
    ccd->glyphs = GWidgetCreateSubWindow(ccd->gw,&subpos,subccd_e_h,ccd,&wattrs);
    memset(&ggcd,0,sizeof(ggcd));
    memset(&fgcd,0,sizeof(fgcd));
    memset(&glabel,0,sizeof(glabel));
    memset(&flabel,0,sizeof(flabel));
    memset(&faspects,0,sizeof(faspects));
    memset(&subboxes,0,sizeof(subboxes));
    memset(&topbox,0,sizeof(topbox));
    for ( i=0; i<3; ++i ) {	/* Match, background foreground tabs */
	k=0;
	glabel[i][k].text = (unichar_t *) _("Set From Selection");
	glabel[i][k].text_is_1byte = true;
	ggcd[i][k].gd.label = &glabel[i][k];
	ggcd[i][k].gd.popup_msg = (unichar_t *) _("Set this glyph list from a selection.");
	ggcd[i][k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	ggcd[i][k].gd.handle_controlevent = CCD_FromSelection;
	ggcd[i][k].data = (void *) (intpt) CID_GlyphList+(0*100+i*20);
	ggcd[i][k++].creator = GButtonCreate;
	subvarray[i][0] = &ggcd[i][k-1];

	ggcd[i][k].gd.flags = gg_visible | gg_enabled | gg_textarea_wrap;
	ggcd[i][k].gd.cid = CID_GlyphList+(0*100+i*20);
	ggcd[i][k].gd.u.completion = CCD_GlyphListCompletion;
	ggcd[i][k++].creator = GTextCompletionCreate;
	subvarray[i][1] = &ggcd[i][k-1];

	if ( i==0 ) {
	    glabel[i][k].text = (unichar_t *) _("An ordered list of lookups and positions");
	    glabel[i][k].text_is_1byte = true;
	    ggcd[i][k].gd.label = &glabel[i][k];
	    ggcd[i][k].gd.flags = gg_visible | gg_enabled;
	    ggcd[i][k++].creator = GLabelCreate;
	    subvarray[i][2] = &ggcd[i][k-1];

	    memset(&gl_seqlookup_mi,0,sizeof(gl_seqlookup_mi));
	    gl_seqlookup_mi.col_cnt = 2;
	    gl_seqlookup_mi.col_init = seqlookup_ci;
	    gl_seqlookup_mi.matrix_data = NULL;
	    gl_seqlookup_mi.initial_row_cnt = 0;

	    ggcd[i][k].gd.flags = gg_visible | gg_enabled;
	    ggcd[i][k].gd.cid = CID_LookupList+0;
	    ggcd[i][k].gd.u.matrix = &gl_seqlookup_mi;
	    ggcd[i][k++].creator = GMatrixEditCreate;
	    subvarray[i][3] = &ggcd[i][k-1];
	} else
	    subvarray[i][2] = subvarray[i][3] = GCD_Glue;
	subvarray[i][4] = NULL;

	subboxes[i][0].gd.flags = gg_enabled|gg_visible;
	subboxes[i][0].gd.u.boxelements = subvarray[i];
	subboxes[i][0].creator = GVBoxCreate;
    }

    j = 0;

    faspects[j].text = (unichar_t *) _("Match");
    faspects[j].text_is_1byte = true;
    faspects[j].selected = true;
    faspects[j].gcd = subboxes[j]; ++j;

    faspects[j].text = (unichar_t *) _("Backtrack");
    faspects[j].text_is_1byte = true;
    faspects[j].disabled = fpst->type==pst_contextpos || fpst->type==pst_contextsub;
    faspects[j].gcd = subboxes[j]; ++j;

    faspects[j].text = (unichar_t *) _("Lookahead");
    faspects[j].text_is_1byte = true;
    faspects[j].disabled = fpst->type==pst_contextpos || fpst->type==pst_contextsub;
    faspects[j].gcd = subboxes[j]; ++j;

    flabel[0].text = (unichar_t *) _("A list of glyphs:");
    flabel[0].text_is_1byte = true;
    fgcd[0].gd.label = &flabel[0];
    fgcd[0].gd.flags = gg_visible | gg_enabled;
    fgcd[0].creator = GLabelCreate;
    topvarray[0] = &fgcd[0];

    fgcd[1].gd.u.tabs = faspects;
    fgcd[1].gd.flags = gg_visible | gg_enabled;
    fgcd[1].gd.cid = CID_MatchType;
    fgcd[1].creator = GTabSetCreate;
    topvarray[1] = &fgcd[1];
    topvarray[2] = NULL;
    topbox[0].gd.flags = gg_enabled|gg_visible;
    topbox[0].gd.u.boxelements = topvarray;
    topbox[0].creator = GVBoxCreate;
    GGadgetsCreate(ccd->glyphs,topbox);

    GMatrixEditSetUpDownVisible(GWidgetGetControl(ccd->glyphs,CID_LookupList+0),
	    true);
    GHVBoxSetExpandableRow(topbox[0].ret,1);
    GHVBoxSetExpandableRow(subboxes[0][0].ret,3);
    GHVBoxSetExpandableRow(subboxes[1][0].ret,gb_expandglue);
    GHVBoxSetExpandableRow(subboxes[2][0].ret,gb_expandglue);

	/* Pane showing the single rule in a coverage table */
    ccd->coverage = GWidgetCreateSubWindow(ccd->gw,&subpos,subccd_e_h,ccd,&wattrs);
    memset(&clabel,0,sizeof(clabel));
    memset(&cgcd,0,sizeof(cgcd));
    memset(&subboxes,0,sizeof(subboxes));
    memset(&topbox,0,sizeof(topbox));

    for ( i=0; i<3; ++i ) {
	k = 0;
	memset(&coverage_mi[i],0,sizeof(coverage_mi[i]));
	coverage_mi[i].col_cnt = 1;
	coverage_mi[i].col_init = coverage_ci;
	if (( fpst->format==pst_coverage || fpst->format==pst_reversecoverage )
		&& fpst->rules!=NULL ) {
	    int cnt = (&fpst->rules[0].u.coverage.ncnt)[i];
	    char **names = (&fpst->rules[0].u.coverage.ncovers)[i];
	    coverage_mi[i].initial_row_cnt = cnt;
	    md = gcalloc(cnt,sizeof(struct matrix_data));
	    for ( j=0; j<cnt; ++j ) {
		md[j].u.md_str = SFNameList2NameUni(sf,names[j]);
	    }
	    coverage_mi[i].matrix_data = md;
	} else {
	    coverage_mi[i].initial_row_cnt = 0;
	    coverage_mi[i].matrix_data = NULL;
	}
	coverage_mi[i].finishedit = CCD_FinishCoverageEdit;
	cgcd[i][k].gd.flags = gg_visible | gg_enabled;
	cgcd[i][k].gd.cid = CID_GList+100+i*20;
	cgcd[i][k].gd.u.matrix = &coverage_mi[i];
	cgcd[i][k++].creator = GMatrixEditCreate;

	faspects[i].gcd = &cgcd[i][k-1];

	if ( i==0 ) {
	    if ( ccd->fpst->type!=pst_reversesub ) {
		clabel[i][k].text = (unichar_t *) _("An ordered list of lookups and positions");
		clabel[i][k].text_is_1byte = true;
		cgcd[i][k].gd.label = &clabel[i][k];
		cgcd[i][k].gd.flags = gg_visible | gg_enabled;
		cgcd[i][k++].creator = GLabelCreate;
		subvarray2[0] = &cgcd[i][k-1];

		memset(&co_seqlookup_mi,0,sizeof(co_seqlookup_mi));
		co_seqlookup_mi.col_cnt = 2;
		co_seqlookup_mi.col_init = seqlookup_ci;
		if ( fpst->format==pst_coverage ) {
		    r = &fpst->rules[0];
		    co_seqlookup_mi.initial_row_cnt = r->lookup_cnt;
		    md = gcalloc(2*r->lookup_cnt,sizeof(struct matrix_data));
		    for ( j=0; j<r->lookup_cnt; ++j ) {
			md[2*j+0].u.md_ival = (intpt) (void *) r->lookups[j].lookup;
			md[2*j+1].u.md_ival = (intpt) r->lookups[j].seq;
		    }
		    co_seqlookup_mi.matrix_data = md;
		} else {
		    co_seqlookup_mi.initial_row_cnt = 0;
		    co_seqlookup_mi.matrix_data = NULL;
		}
		cgcd[i][k].gd.flags = gg_visible | gg_enabled;
		cgcd[i][k].gd.cid = CID_LookupList+100+(i*20);
		cgcd[i][k].gd.u.matrix = &co_seqlookup_mi;
		cgcd[i][k++].creator = GMatrixEditCreate;
		subvarray2[1] = &cgcd[i][k-1];
		subvarray2[2] = NULL;

		subboxes[i][1].gd.flags = gg_enabled|gg_visible;
		subboxes[i][1].gd.u.boxelements = subvarray2;
		subboxes[i][1].creator = GVBoxCreate;

		subvarray3[0] = &cgcd[i][0];
		subvarray3[1] = &subboxes[i][1];
		subvarray3[2] = NULL;

		subboxes[i][2].gd.flags = gg_enabled|gg_visible;
		subboxes[i][2].gd.u.boxelements = subvarray3;
		subboxes[i][2].creator = GVBoxCreate;
		faspects[i].gcd = subboxes[i]+2;
	    } else {
		r = NULL;
		if ( fpst->rule_cnt==1 ) r = &fpst->rules[0];

		clabel[i][k].text = (unichar_t *) _("Replacements");
		clabel[i][k].text_is_1byte = true;
		cgcd[i][k].gd.label = &clabel[i][k];
		cgcd[i][k].gd.flags = gg_visible | gg_enabled;
		cgcd[i][k++].creator = GLabelCreate;
		subvarray2[0] = &cgcd[i][k-1];

		clabel[i][k].text = (unichar_t *) _("Set From Selection");
		clabel[i][k].text_is_1byte = true;
		cgcd[i][k].gd.label = &clabel[i][k];
		cgcd[i][k].gd.popup_msg = (unichar_t *) _("Set this glyph list from a selection.");
		cgcd[i][k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
		cgcd[i][k].gd.handle_controlevent = CCD_FromSelection;
		cgcd[i][k].data = (void *) (intpt) (CID_RplList+100);
		cgcd[i][k++].creator = GButtonCreate;
		subvarray2[1] = &cgcd[i][k-1];
		subvarray2[2] = GCD_Glue;
		subvarray2[3] = NULL;

		subboxes[i][1].gd.flags = gg_enabled|gg_visible;
		subboxes[i][1].gd.u.boxelements = subvarray2;
		subboxes[i][1].creator = GHBoxCreate;
		subvarray3[0] = &subboxes[i][1];

		if ( r!=NULL && r->u.rcoverage.replacements!=NULL ) {
		    clabel[i][k].text = (unichar_t *) SFNameList2NameUni(sf,r->u.rcoverage.replacements);
		    clabel[i][k].text_is_1byte = true;
		    cgcd[i][k].gd.label = &clabel[i][k];
		}
		cgcd[i][k].gd.flags = gg_visible | gg_enabled;
		cgcd[i][k].gd.cid = CID_RplList+100;
		cgcd[i][k].gd.u.completion = CCD_GlyphListCompletion;
		cgcd[i][k++].creator = GTextCompletionCreate;
		subvarray3[1] = &cgcd[i][k-1];
		subvarray3[2] = GCD_Glue;
		subvarray3[3] = NULL;

		subboxes[i][2].gd.flags = gg_enabled|gg_visible;
		subboxes[i][2].gd.u.boxelements = subvarray3;
		subboxes[i][2].creator = GVBoxCreate;

		subvarray4[0] = &cgcd[i][0];
		subvarray4[1] = &subboxes[i][2];
		subvarray4[2] = NULL;

		subboxes[i][3].gd.flags = gg_enabled|gg_visible;
		subboxes[i][3].gd.u.boxelements = subvarray4;
		subboxes[i][3].creator = GVBoxCreate;
		faspects[i].gcd = subboxes[i]+3;
	    }
	}
    }

    flabel[0].text = (unichar_t *) _("A coverage table:");
    flabel[0].text_is_1byte = true;
    fgcd[0].gd.label = &flabel[0];
    fgcd[0].gd.flags = gg_visible | gg_enabled;
    fgcd[0].creator = GLabelCreate;
    topvarray[0] = &fgcd[0];

    fgcd[1].gd.u.tabs = faspects;
    fgcd[1].gd.flags = gg_visible | gg_enabled;
    fgcd[1].gd.cid = CID_MatchType+100;
    fgcd[1].creator = GTabSetCreate;
    topvarray[1] = &fgcd[1];
    topvarray[2] = NULL;
    topbox[0].gd.flags = gg_enabled|gg_visible;
    topbox[0].gd.u.boxelements = topvarray;
    topbox[0].creator = GVBoxCreate;
    GGadgetsCreate(ccd->coverage,topbox);
    for ( i=0; i<3; ++i ) {
	GMatrixEditSetUpDownVisible(GWidgetGetControl(ccd->coverage,CID_GList+100+i*20),
		true);
	GMatrixEditSetColumnCompletion(GWidgetGetControl(ccd->coverage,CID_GList+100+i*20),0,
		CCD_GlyphListCompletion);
    }
    if ( ccd->fpst->type!=pst_reversesub ) {
	GMatrixEditSetUpDownVisible(GWidgetGetControl(ccd->coverage,CID_LookupList+100+0),
		true);
	GHVBoxSetExpandableRow(subboxes[0][1].ret,1);
    } else {
	GHVBoxSetExpandableCol(subboxes[0][1].ret,gb_expandglue);
	GHVBoxSetExpandableRow(subboxes[0][2].ret,gb_expandglue);
    }
    GHVBoxSetExpandableRow(topbox[0].ret,1);


	/* This pane displays a set of class rules, and below the three sets of classes for those rules */
    ccd->classrules = GWidgetCreateSubWindow(ccd->gw,&subpos,subccd_e_h,ccd,&wattrs);
    memset(&cllabel,0,sizeof(cllabel));
    memset(&clgcd,0,sizeof(clgcd));
    memset(&clabel,0,sizeof(clabel));
    memset(&cgcd,0,sizeof(cgcd));
    memset(&subboxes,0,sizeof(subboxes));
    memset(&topbox,0,sizeof(topbox));
    memset(&classbox,0,sizeof(classbox));

    tempfpst = fpst;
    if ( fpst->format==pst_glyphs && fpst->rule_cnt>0 )
	tempfpst = FPSTGlyphToClass(fpst);

    k=0;

    memset(&classrules_mi,0,sizeof(classrules_mi));
    classrules_mi.col_cnt = 1;
    classrules_mi.col_init = classrules_ci;
    if ( tempfpst->format==pst_class && tempfpst->rule_cnt>0 ) {
	classrules_mi.initial_row_cnt = tempfpst->rule_cnt;
	md = gcalloc(tempfpst->rule_cnt,sizeof(struct matrix_data));
	for ( j=0; j<tempfpst->rule_cnt; ++j ) {
	    md[j+0].u.md_str = classruleitem(&tempfpst->rules[j]);
	}
	classrules_mi.matrix_data = md;
    } else {
	classrules_mi.initial_row_cnt = 0;
	classrules_mi.matrix_data = NULL;
    }
    clgcd[k].gd.flags = gg_visible | gg_enabled;
    clgcd[k].gd.cid = CID_GList+200;
    clgcd[k].gd.u.matrix = &classrules_mi;
    clgcd[k++].creator = GMatrixEditCreate;
    classv[k-1] = &clgcd[k-1];

    clgcd[k].gd.pos.width = CCD_WIDTH-23;
    clgcd[k].gd.flags = gg_visible | gg_enabled;
    clgcd[k++].creator = GLineCreate;
    classv[k-1] = &clgcd[k-1];
    classv[k] = NULL;

    classbox[0].gd.flags = gg_enabled|gg_visible;
    classbox[0].gd.u.boxelements = classv;
    classbox[0].creator = GVBoxCreate;
    topvarray[0] = &classbox[0];

    for ( i=0; i<3; ++i ) {
	int l=0, cc, j, sameas=0;

	if ( i!=0 ) {
	    if ( (&tempfpst->nccnt)[i] == tempfpst->nccnt ) {
		sameas = gg_cb_on;
		for ( j=1; j<(&tempfpst->nccnt)[i]; ++j ) {
		    if ( strcmp((&tempfpst->nclass)[i][j],tempfpst->nclass[j])!=0 ) {
			sameas = 0;
		break;
		    }
		}
	    }
	    clabel[i][l].text = (unichar_t *) _("Same as Match Classes");
	    clabel[i][l].text_is_1byte = true;
	    cgcd[i][l].gd.label = &clabel[i][l];
	    cgcd[i][l].gd.handle_controlevent = CCD_SameAsClasses;
	    cgcd[i][l].gd.flags = gg_visible | gg_enabled | sameas;
	    cgcd[i][l].gd.cid = CID_SameAsClasses + i*20;
	    cgcd[i][l++].creator = GCheckBoxCreate;
	    subvarray[i][l-1] = &cgcd[i][l-1];
	}

	memset(&class_mi[i],0,sizeof(class_mi[0]));
	class_mi[i].col_cnt = 1;
	class_mi[i].col_init = class_ci;
	if ( tempfpst->format==pst_class ) {
	    char **classes = (&tempfpst->nclass)[i];
	    if ( classes==NULL )
		cc=1;
	    else
		cc = (&tempfpst->nccnt)[i];
	    class_mi[i].initial_row_cnt = cc;
	    md = gcalloc(cc,sizeof(struct matrix_data));
	    md[0].u.md_str = copy(_("{Everything Else}"));
	    md[0].frozen = true;
	    for ( j=1; j<cc; ++j ) {
		md[j+0].u.md_str = SFNameList2NameUni(sf,classes[j]);
	    }
	    class_mi[i].matrix_data = md;
	} else {
	    class_mi[i].initial_row_cnt = 0;
	    class_mi[i].matrix_data = NULL;
	}
	class_mi[i].finishedit = CCD_FinishClassEdit;
	class_mi[i].candelete = CCD_EnableDeleteClass;

	if ( sameas )
	    cgcd[i][l].gd.flags = gg_visible;
	else
	    cgcd[i][l].gd.flags = gg_visible | gg_enabled;
	cgcd[i][l].gd.cid = CID_GList+300+i*20;
	cgcd[i][l].gd.u.matrix = &class_mi[i];
	cgcd[i][l++].creator = GMatrixEditCreate;
	subvarray[i][l-1] = &cgcd[i][l-1];
	subvarray[i][l] = NULL;

	subboxes[i][0].gd.flags = gg_enabled|gg_visible;
	subboxes[i][0].gd.u.boxelements = subvarray[i];
	subboxes[i][0].creator = GVBoxCreate;

	faspects[i].gcd = subboxes[i];
    }
    j=0;
    faspects[j++].text = (unichar_t *) _("Match Classes");
    faspects[j++].text = (unichar_t *) _("Back Classes");
    faspects[j++].text = (unichar_t *) _("Ahead Classes");

    clgcd[k].gd.u.tabs = faspects;
    clgcd[k].gd.flags = gg_visible | gg_enabled;
    clgcd[k].gd.cid = CID_MatchType+300;
    clgcd[k].creator = GTabSetCreate;
    topvarray[1] = &clgcd[k]; topvarray[2] = NULL;
    topbox[0].gd.flags = gg_enabled|gg_visible;
    topbox[0].gd.u.boxelements = topvarray;
    topbox[0].creator = GVBoxCreate;
    GGadgetsCreate(ccd->classrules,topbox);

    /* Top box should give it's size equally to its two components */
    GHVBoxSetExpandableRow(classbox[0].ret,0);
    GHVBoxSetExpandableRow(subboxes[0][0].ret,0);
    GHVBoxSetExpandableRow(subboxes[1][0].ret,1);
    GHVBoxSetExpandableRow(subboxes[2][0].ret,1);
    GMatrixEditSetColumnCompletion(GWidgetGetControl(ccd->classrules,CID_GList+300+0*20),0,CCD_GlyphListCompletion);
    GMatrixEditSetColumnCompletion(GWidgetGetControl(ccd->classrules,CID_GList+300+1*20),0,CCD_GlyphListCompletion);
    GMatrixEditSetColumnCompletion(GWidgetGetControl(ccd->classrules,CID_GList+300+2*20),0,CCD_GlyphListCompletion);

    if ( tempfpst!=fpst )
	FPSTFree(tempfpst);

	/* This pane displays a tabbed set describing one rule in class format */
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
	cgcd[i][k].gd.flags = gg_visible | gg_enabled;
	cgcd[i][k++].creator = GLabelCreate;
	subvarray[i][k-1] = &cgcd[i][k-1];

	cgcd[i][k].gd.flags = gg_visible | gg_enabled | gg_textarea_wrap;
	cgcd[i][k].gd.cid = CID_ClassNumbers+i*20;
	cgcd[i][k++].creator = GTextAreaCreate;
	subvarray[i][k-1] = &cgcd[i][k-1];

	clabel[i][k].text = (unichar_t *) _("Classes");
	clabel[i][k].text_is_1byte = true;
	cgcd[i][k].gd.label = &clabel[i][k];
	cgcd[i][k].gd.flags = gg_visible | gg_enabled;
	cgcd[i][k++].creator = GLabelCreate;
	subvarray[i][k-1] = &cgcd[i][k-1];

	cgcd[i][k].gd.flags = gg_visible | gg_enabled | gg_textarea_wrap;
	cgcd[i][k].gd.handle_controlevent = CCD_ClassSelected;
	cgcd[i][k].gd.cid = CID_ClassList+i*20;
	cgcd[i][k++].creator = GListCreate;
	subvarray[i][k-1] = &cgcd[i][k-1];
	subvarray[i][k] = NULL;

	subboxes[i][0].gd.flags = gg_enabled|gg_visible;
	subboxes[i][0].gd.u.boxelements = subvarray[i];
	subboxes[i][0].creator = GVBoxCreate;

	faspects[i].gcd = subboxes[i];

	if ( i==0 ) {
	    clabel[i][k].text = (unichar_t *) _("An ordered list of lookups and positions");
	    clabel[i][k].text_is_1byte = true;
	    ggcd[i][k].gd.label = &clabel[i][k];
	    ggcd[i][k].gd.flags = gg_visible | gg_enabled;
	    ggcd[i][k++].creator = GLabelCreate;
	    subvarray2[0] = &ggcd[i][k-1];

	    memset(&cl_seqlookup_mi,0,sizeof(cl_seqlookup_mi));
	    cl_seqlookup_mi.col_cnt = 2;
	    cl_seqlookup_mi.col_init = seqlookup_ci;
	    cl_seqlookup_mi.matrix_data = NULL;
	    cl_seqlookup_mi.initial_row_cnt = 0;

	    ggcd[i][k].gd.flags = gg_visible | gg_enabled;
	    ggcd[i][k].gd.cid = CID_LookupList+500;
	    ggcd[i][k].gd.u.matrix = &cl_seqlookup_mi;
	    ggcd[i][k++].creator = GMatrixEditCreate;
	    subvarray2[1] = &ggcd[i][k-1];
	    subvarray2[2] = NULL;

	    subboxes[i][1].gd.flags = gg_enabled|gg_visible;
	    subboxes[i][1].gd.u.boxelements = subvarray2;
	    subboxes[i][1].creator = GVBoxCreate;

	    subvarray3[0] = &subboxes[i][0];
	    subvarray3[1] = &subboxes[i][1];
	    subvarray3[2] = NULL;

	    subboxes[i][2].gd.flags = gg_enabled|gg_visible;
	    subboxes[i][2].gd.u.boxelements = subvarray3;
	    subboxes[i][2].creator = GVBoxCreate;
	    faspects[i].gcd = subboxes[i]+2;
	}
    }

    j=0;
    faspects[j++].text = (unichar_t *) _("Match");
    faspects[j++].text = (unichar_t *) _("Backtrack");
    faspects[j++].text = (unichar_t *) _("Lookahead");

    k=0;
    clgcd[k].gd.u.tabs = faspects;
    clgcd[k].gd.flags = gg_visible | gg_enabled;
    clgcd[k].gd.cid = CID_ClassType;
    clgcd[k].creator = GTabSetCreate;
    GGadgetsCreate(ccd->classnumber,clgcd);
    GHVBoxSetExpandableRow(subboxes[0][0].ret,3);
    GHVBoxSetExpandableRow(subboxes[1][0].ret,3);
    GHVBoxSetExpandableRow(subboxes[2][0].ret,3);
    GHVBoxSetExpandableRow(subboxes[0][1].ret,1);


    if ( ccd->aw == aw_formats )
	GDrawSetVisible(ccd->formats,true);
    else if ( ccd->aw == aw_grules )
	GDrawSetVisible(ccd->grules,true);
    else if ( ccd->aw == aw_classrules )
	GDrawSetVisible(ccd->classrules,true);
    else
	GDrawSetVisible(ccd->coverage,true);

    GDrawSetVisible(gw,true);
    while ( !ccd->done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(ccd->gw);

    GTextInfoListFree(lookup_list);
    seqlookup_ci[1].enum_vals = NULL;

    /* ccd is freed by the event handler when we get a et_destroy event */

return;
}
