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

#include <fontforge-config.h>

#include "asmfpst.h"
#include "chardata.h"
#include "fontforgeui.h"
#include "gkeysym.h"
#include "lookups.h"
#include "splineutil.h"
#include "ustring.h"
#include "utype.h"

/* Currently uses class numbers rather than names!!!!!! */

/* ************************************************************************** */
/* ************************ Context/Chaining Dialog ************************* */
/* ************************************************************************** */
enum activewindow { aw_formats, aw_coverage, aw_grules, aw_glyphs,
		    aw_classrules, aw_classnumber,
		    aw_coverage_simple, aw_glyphs_simple, aw_classes_simple };
struct contextchaindlg {
    struct gfi_data *gfi;
    SplineFont *sf;
    FPST *fpst;
    unichar_t *newname;
    int isnew;
    GWindow gw;
    GWindow formats, coverage, grules, glyphs, classrules, classnumber;
    GWindow coverage_simple, glyphs_simple, classes_simple;
    enum activewindow aw;
/* Wizard panes:
  formats         -- gives user a choice between by glyph/class/coverage table
Simple version:
  glyphs_simple   -- user picked glyph format, shows a matrix edit of glyph lists
		     and lookups.
  classes_simple  -- user picked class format. shows a matrix edit of class lists
		     with lookups. Also the three sets of classes_simple.
  coverage_simple -- user picked coverage table format. shows a matrix edit with
		     a bunch of coverage tables and lookups/replacements.
		     [coverage_simple format only accepts 1 rule]
Complex version:
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

#define CID_Simple	205
#define CID_Complex	206

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

/* And for the simple dlg... */

#define CID_GList_Simple	4000
#define CID_GNewSection		4001
#define CID_GAddLookup		4002

#define CID_CList_Simple	(CID_GList_Simple+100)
#define CID_CNewSection		(CID_GNewSection+100)
#define CID_CAddLookup		(CID_GAddLookup+100)

#define CID_MatchClasses	(CID_GList_Simple+300+0*20)
#define CID_BackClasses		(CID_MatchClasses+1*20)
#define CID_ForeClasses		(CID_MatchClasses+2*20)
#define CID_SameAsClasses_S	(CID_MatchClasses+1)		/* Not actually used itself */
#define CID_BackClassesSameAs_S	(CID_SameAsClasses_S+1*20)
#define CID_ForeClassesSameAs_S	(CID_SameAsClasses_S+2*20)

#define CID_ClassMatchType	4500

#define CID_Covers		4600

char *cu_copybetween(const unichar_t *start, const unichar_t *end) {
    char *ret = malloc(end-start+1);
    cu_strncpy(ret,start,end-start);
    ret[end-start] = '\0';
return( ret );
}

static char *reversenames(char *str) {
    char *ret;
    char *rpt, *pt, *start, *spt;

    if ( str==NULL )
return( NULL );

    rpt = ret = malloc(strlen(str)+1);
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

static char *rpl(const char *src, const char *find, const char *rpl) {
    const char *pt, *start;
    char *ret, *rpt;
    int found_cnt=0;
    int flen = strlen(find);

    for ( pt=src; *pt; ) {
	while ( isspace(*pt)) ++pt;
	if ( *pt=='\0' )
    break;
	for ( start=pt; !isspace(*pt) && *pt!='\0'; ++pt );
	if ( pt-start==flen && strncmp(find,start,flen)==0 )
	    ++found_cnt;
    }
    if ( found_cnt==0 )
return( copy(src));

    rpt = ret = malloc(strlen(src)+found_cnt*(strlen(rpl)-flen)+1);
    for ( pt=src; *pt; ) {
	while ( isspace(*pt))
	    *rpt++ = *pt++;
	if ( *pt=='\0' )
    break;
	for ( start=pt; !isspace(*pt) && *pt!='\0'; ++pt );
	if ( pt-start==flen && strncmp(find,start,flen)==0 ) {
	    strcpy(rpt,rpl);
	    rpt += strlen(rpt);
	} else {
	    strncpy(rpt,start,pt-start);
	    rpt += (pt-start);
	}
    }
    *rpt = '\0';
return( ret );
}

static char *classnumbers(int cnt,uint16 *classes, struct matrix_data *classnames, int rows, int cols) {
    char buf[20];
    int i, len;
    char *pt, *ret;

    len = 0;
    for ( i=0; i<cnt; ++i ) {
	if ( classnames[cols*classes[i]+0].u.md_str==NULL ) {
	    sprintf( buf, "%d ", classes[i]);
	    len += strlen(buf);
	} else {
	    len += strlen(classnames[cols*classes[i]+0].u.md_str)+1;
	}
    }
    ret = pt = malloc(len+3);
    *pt = '\0';		/* In case it is empty */

    for ( i=0; i<cnt; ++i ) {
	if ( classnames[cols*classes[i]+0].u.md_str==NULL ) {
	    sprintf( pt, "%d ", classes[i]);
	    pt += strlen(pt);
	} else {
	    strcpy(pt, classnames[cols*classes[i]+0].u.md_str);
	    pt += strlen(pt);
	    *pt++ = ' ';
	}
    }
    if ( pt>ret && pt[-1]==' ' )
	pt[-1] = '\0';
return( ret );
}

static char *rclassnumbers(int cnt,uint16 *classes, struct matrix_data *classnames, int rows, int cols) {
    char buf[20];
    int i, len;
    char *pt, *ret;

    len = 0;
    for ( i=0; i<cnt; ++i ) {
	if ( classnames[cols*classes[i]+0].u.md_str==NULL ) {
	    sprintf( buf, "%d ", classes[i]);
	    len += strlen(buf);
	} else {
	    len += strlen(classnames[cols*classes[i]+0].u.md_str)+1;
	}
    }
    ret = pt = malloc(len+3);
    *pt = '\0';
    for ( i=cnt-1; i>=0; --i ) {
	if ( classnames[cols*classes[i]+0].u.md_str==NULL ) {
	    sprintf( pt, "%d ", classes[i]);
	    pt += strlen(pt);
	} else {
	    strcpy(pt, classnames[cols*classes[i]+0].u.md_str);
	    pt += strlen(pt);
	    *pt++ = ' ';
	}
    }
    if ( pt>ret && pt[-1]==' ' )
	pt[-1] = '\0';
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

    pt = utf8_idpb(pt, 0x21d2,0);
    for ( i=0; i<r->lookup_cnt; ++i ) {
	sprintf( pt," %d <%s>,", r->lookups[i].seq, r->lookups[i].lookup->lookup_name);
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
	while ( *pt!='<' && *pt!='\0' ) ++pt;
	if ( *pt=='<' ) {
	    ++pt;
	    while ( *pt!='>' && *pt!='\0' ) ++pt;
	    if ( *pt=='>' ) ++pt;
	}
	if ( *pt==',' ) ++pt;
    }
    r->lookup_cnt = cnt;
    r->lookups = calloc(cnt,sizeof(struct seqlookup));
    cnt = 0;
    pt = solooks;
    for (;;) {
	char *end;
	r->lookups[cnt].seq = strtol(pt,&end,10);
	for ( pt = end+1; isspace( *pt ); ++pt );
	if ( *pt=='<' ) {
	    const char *eoname; char *temp;
	    ++pt;
	    for ( eoname = pt; *eoname!='\0' && *eoname!='>'; ++eoname );
	    temp = copyn(pt,eoname-pt);
	    r->lookups[cnt].lookup = SFFindLookup(sf,temp);
	    if ( r->lookups[cnt].lookup==NULL )
		IError("No lookup in parseseqlookups");
	    free(temp);
	    pt = eoname;
	    if ( *pt=='>' ) ++pt;
	} else
	    IError("No lookup in parseseqlookups");
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

    ret = pt = malloc(len+8);
    if ( r->u.glyph.back!=NULL && *r->u.glyph.back!='\0' ) {
	char *temp = reversenames(r->u.glyph.back);
	strcpy(pt,temp);
	pt += strlen(temp);
	free(temp);
	*pt++ = ' ';
    }
    *pt++ = '|';
    *pt++ = ' ';
    strcpy(pt,r->u.glyph.names);
    pt += strlen(r->u.glyph.names);
    *pt++ = ' ';
    *pt++ = '|';
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
    for ( pt=ruletext; *pt!='\0' && *pt!='|'; ++pt );
    if ( *pt=='\0' )
return;
    if ( pt>ruletext ) {
	temp = GlyphNameListDeUnicode(freeme = copyn(ruletext,pt-1-ruletext));
	r->u.glyph.back = reversenames(temp);
	free(temp); free(freeme);
    }
    ruletext = pt+2;
    for ( pt=ruletext; *pt!='\0' && *pt!='|'; ++pt );
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

static char *classruleitem(struct fpst_rule *r,struct matrix_data **classes, int clen[3], int cols) {
    char *ret, *pt;
    int len, i, k;
    char buf[20];

    len = 0;
    for ( i=0; i<3; ++i ) {
	for ( k=0; k<(&r->u.class.ncnt)[i]; ++k ) {
	    int c = (&r->u.class.nclasses)[i][k];
	    if ( classes[i][cols*c+0].u.md_str!=NULL && *classes[i][cols*c+0].u.md_str!='\0' )
		len += strlen(classes[i][cols*c+0].u.md_str)+1;
	    else {
		sprintf( buf, "%d ", c);
		len += strlen(buf);
	    }
	}
    }

    ret = pt = malloc((len+8+seqlookuplen(r)) * sizeof(unichar_t));
    for ( k=r->u.class.bcnt-1; k>=0; --k ) {
	int c = r->u.class.bclasses[k];
	if ( classes[1][cols*c+0].u.md_str!=NULL && *classes[1][cols*c+0].u.md_str!='\0' ) {
	    strcpy(pt,classes[1][cols*c+0].u.md_str);
	    pt += strlen( pt );
	    *pt++ = ' ';
	} else {
	    sprintf( pt, "%d ", c);
	    pt += strlen(pt);
	}
    }
    *pt++ = '|';
    for ( k=0; k<r->u.class.ncnt; ++k ) {
	int c = r->u.class.nclasses[k];
	if ( classes[0][cols*c+0].u.md_str!=NULL && *classes[0][cols*c+0].u.md_str!='\0' ) {
	    strcpy(pt,classes[0][cols*c+0].u.md_str);
	    pt += strlen( pt );
	    *pt++ = ' ';
	} else {
	    sprintf( pt, "%d ", c);
	    pt += strlen(pt);
	}
    }
    if ( pt[-1]==' ' ) --pt;
    *pt++ = '|';
    for ( k=0; k<r->u.class.fcnt; ++k ) {
	int c = r->u.class.fclasses[k];
	if ( classes[2][cols*c+0].u.md_str!=NULL && *classes[2][cols*c+0].u.md_str!='\0' ) {
	    strcpy(pt,classes[2][cols*c+0].u.md_str);
	    pt += strlen( pt );
	    *pt++ = ' ';
	} else {
	    sprintf( pt, "%d ", c);
	    pt += strlen(pt);
	}
    }

    *pt++ = ' ';
    pt = addseqlookups(pt, r);
return( ret );
}

static void classruleitem2rule(SplineFont *sf,const char *ruletext,struct fpst_rule *r,
	struct matrix_data **classes, int clen[3], int cols) {
    const char *pt, *start, *nstart; char *end;
    int len, i, ch, k;

    memset(r,0,sizeof(*r));

    len = 0; i=1;
    for ( pt=ruletext; *pt; ) {
	ch = utf8_ildb((const char **) &pt);
	while ( ch!='|' && ch!=0x21d2 && ch!='\0' ) {
	    while ( isspace(ch))
		ch = utf8_ildb((const char **) &pt);
	    if ( ch=='|' || ch== 0x21d2 || ch=='\0' )
	break;
	    ++len;
	    while ( !isspace(ch) && ch!='|' && ch!=0x21d2 && ch!='\0' )
		ch = utf8_ildb((const char **) &pt);
	}
	(&r->u.class.ncnt)[i] = len;
	(&r->u.class.nclasses)[i] = malloc(len*sizeof(uint16));
	len = 0;
	if ( ch=='\0' || ch==0x21d2 )
    break;
	if ( ch=='|' ) {
	    if ( i==1 ) i=0;
	    else if ( i==0 ) i=2;
	}
    }
    len = 0; i=1;
    for ( pt=ruletext; *pt; ) {
	start = pt;
	ch = utf8_ildb((const char **) &pt);
	while ( ch!='|' && ch!=0x21d2 && ch!='\0' ) {
	    while ( isspace(ch)) {
		start = pt;
		ch = utf8_ildb((const char **) &pt);
	    }
	    if ( ch=='|' || ch== 0x21d2 || ch=='\0' )
	break;
	    nstart = start;
	    while ( !isspace(ch) && ch!='|' && ch!=0x21d2 && ch!='\0' ) {
		nstart = pt;
		ch = utf8_ildb((const char **) &pt);
	    }
	    (&r->u.class.nclasses)[i][len] = strtol(start,&end,10);
	    if ( end<nstart ) {		/* Not a number, must be a class name */
		for ( k=0; k<clen[i]; ++k ) {
		    if ( classes[i][cols*k+0].u.md_str==NULL )
		continue;
		    if ( strlen(classes[i][cols*k+0].u.md_str)==nstart-start &&
			    strncmp(classes[i][cols*k+0].u.md_str,start,nstart-start)==0 ) {
			(&r->u.class.nclasses)[i][len] = k;
		break;
		    }
		}
	    }
	    start = nstart;
	    ++len;
	}
	len = 0;
	if ( ch=='\0' || ch==0x21d2 )
    break;
	if ( ch=='|' ) {
	    if ( i==1 ) i=0;
	    else if ( i==0 ) i=2;
	}
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
    r->lookups = malloc(len*sizeof(struct seqlookup));
    for ( i=0; i<len; ++i ) {
	r->lookups[i].seq = classes[2*i+1].u.md_ival;
	r->lookups[i].lookup = (OTLookup *) classes[2*i+0].u.md_ival;
    }
}

static void CCD_EnableNextPrev(struct contextchaindlg *ccd) {
    /* Cancel is always enabled, Not so for OK or Next or Prev */
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
      case aw_glyphs_simple:
      case aw_classes_simple:
      case aw_coverage_simple:
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_Prev),ccd->fpst->type!=pst_reversesub);
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_Next),false);
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_OK),true);
      break;
      default:
	IError("Can't get here");
      break;
    }
}

static int CCD_ReasonableClassNum(const unichar_t *umatch,GGadget *list,
	struct fpst_rule *r, int which ) {
    int cols, len;
    struct matrix_data *classes;
    char *match = u2utf8_copy(umatch);
    char *pt, *start, *end;
    int doit;
    int any,val;

    cols = GMatrixEditGetColCnt(list);
    classes = GMatrixEditGet(list,&len);

    for ( doit = 0; doit<2; ++doit ) {
	any = 0;
	for ( pt=match;; ) {
	    while ( *pt==' ' ) ++pt;
	    if ( *pt=='\0' )
	break;
	    for ( start=pt; *pt!=' ' && *pt!='\0'; ++pt );
	    val = strtol(start,&end,10);
	    if ( *end!='\0' ) {
		for ( val=len-1; val>=0; --val )
		    if ( classes[cols*val+0].u.md_str!=NULL &&
			    strlen(classes[cols*val+0].u.md_str)==pt-start &&
			    strncasecmp(start,classes[cols*val+0].u.md_str,pt-start)==0 )
		break;
	    }
	    if ( val<0 || val>=len ) {
		ff_post_error(_("Bad Class"),_("%.*s is not a valid class name (or number)"), pt-start, start);
return( false );
	    }
	    if ( doit )
		(&r->u.class.nclasses)[which][any] = val;
	    ++any;
	}
	if ( !doit ) {
	    (&r->u.class.ncnt)[which] = any;
	    (&r->u.class.nclasses)[which] = malloc(any*sizeof(uint16));
	}
    }
return( true );
}

static int ClassNamePrep(struct contextchaindlg *ccd,struct matrix_data **classes,int clen[3]) {
    int i;

    for ( i=0; i<3; ++i ) {
	if ( i==0 || !GGadgetIsChecked(GWidgetGetControl(ccd->gw,CID_SameAsClasses+i*20)) )
	    classes[i] = GMatrixEditGet(GWidgetGetControl(ccd->gw,CID_GList+300+i*20),&clen[i]);
	else
	    classes[i] = GMatrixEditGet(GWidgetGetControl(ccd->gw,CID_GList+300),&clen[i]);
    }
return( GMatrixEditGetColCnt(GWidgetGetControl(ccd->gw,CID_GList+300+0*20)) );
}

static void CCD_FinishRule(struct contextchaindlg *ccd) {
    struct fpst_rule dummy;
    GGadget *list;
    int i,tot;
    char *buts[3];
    buts[0] = _("_Yes"); buts[1] = _("_No"); buts[2] = NULL;
    struct matrix_data *md;
    int len;
    int clen[3], ccols;
    struct matrix_data *classes[3];

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
	ccols = ClassNamePrep(ccd,classes,clen);
	if ( !CCD_ReasonableClassNum(
		_GGadgetGetTitle(GWidgetGetControl(ccd->gw,CID_ClassNumbers)),
		    GWidgetGetControl(ccd->gw,CID_ClassList),
		    &dummy,0) ||
	     !CCD_ReasonableClassNum(
		_GGadgetGetTitle(GWidgetGetControl(ccd->gw,CID_ClassNumbers+20)),
		    GWidgetGetControl(ccd->gw,CID_ClassList+20),
		    &dummy,1) ||
	     !CCD_ReasonableClassNum(
		_GGadgetGetTitle(GWidgetGetControl(ccd->gw,CID_ClassNumbers+40)),
		    GWidgetGetControl(ccd->gw,CID_ClassList+40),
		    &dummy,2)) {
	    FPSTRuleContentsFree(&dummy,pst_class);
return;
	}
	/* reverse the backtrack */
	for ( i=0; i<dummy.u.class.bcnt/2; ++i ) {
	    int temp = dummy.u.class.bclasses[i];
	    dummy.u.class.bclasses[i] = dummy.u.class.bclasses[dummy.u.class.bcnt-1-i];
	    dummy.u.class.bclasses[dummy.u.class.bcnt-1-i] = temp;
	}
	for ( i=0; i<dummy.lookup_cnt; ++i ) {
	    if ( dummy.lookups[i].seq >= dummy.u.class.ncnt ) {
		ff_post_error(_("Bad Sequence/Lookup List"),_("Sequence number out of bounds, must be less than %d (number of classes in list above)"), dummy.u.class.ncnt );
return;
	    }
	}
	ret = classruleitem(&dummy,classes,clen,ccols);
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

static struct matrix_data *MD2MD(struct matrix_data *md,int len) {
    struct matrix_data *newmd = calloc(2*len,sizeof(struct matrix_data));
    int i;

    for ( i=0; i<len; ++i ) {
	newmd[2*i+0].u.md_str = copy(md[3*i+0].u.md_str);
	newmd[2*i+1].u.md_str = copy(md[3*i+1].u.md_str);
    }
return( newmd );
}

static void CCD_NewGlyphRule(GGadget *glyphrules,int r,int c) {
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

    md = calloc(2*dummy.lookup_cnt,sizeof(struct matrix_data));
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
}

static char *_CCD_NewGlyphRule(GGadget *glyphrules,int r,int c) {
    int rows;
    struct matrix_data *rulelist = GMatrixEditGet(glyphrules,&rows);
    CCD_NewGlyphRule(glyphrules,r,c);
return( copy( rulelist[r].u.md_str ) );		/* We change the display to get the new value, but we must return something... */
}

static void CCD_NewClassRule(GGadget *classrules,int r,int c) {
    struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(classrules));
    int rows;
    struct matrix_data *rulelist = GMatrixEditGet(classrules,&rows);
    struct fpst_rule dummy;
    GGadget *lookuplist = GWidgetGetControl(ccd->classnumber,CID_LookupList+500);
    struct matrix_data *md, *classes[3];
    int len, clen[3], ccols;
    int i,j;
    char *temp;

    ccols = ClassNamePrep(ccd,classes,clen);
    memset(&dummy,0,sizeof(dummy));
    classruleitem2rule(ccd->sf,rulelist[r].u.md_str,&dummy,classes,clen,ccols);
    GGadgetSetTitle8(GWidgetGetControl(ccd->gw,CID_ClassNumbers),
	    (temp=classnumbers(dummy.u.class.ncnt,dummy.u.class.nclasses,classes[0],clen[0],ccols)));
    free(temp);
    GGadgetSetTitle8(GWidgetGetControl(ccd->gw,CID_ClassNumbers+20),
	    (temp=rclassnumbers(dummy.u.class.bcnt,dummy.u.class.bclasses,classes[1],clen[1],ccols)));
    free(temp);
    GGadgetSetTitle8(GWidgetGetControl(ccd->gw,CID_ClassNumbers+40),
	    (temp=classnumbers(dummy.u.class.fcnt,dummy.u.class.fclasses,classes[2],clen[2],ccols)));
    free(temp);

    md = calloc(2*dummy.lookup_cnt,sizeof(struct matrix_data));
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
	GMatrixEditSet(GWidgetGetControl(ccd->gw,CID_ClassList+i*20),MD2MD(md,len),len,false);
    }
    ccd->aw = aw_classnumber;
    ccd->row_being_edited = r;
    GDrawSetVisible(ccd->classrules,false);
    GDrawSetVisible(ccd->classnumber,true);
    CCD_EnableNextPrev(ccd);
}

static char *_CCD_NewClassRule(GGadget *classrules,int r,int c) {
    int rows;
    struct matrix_data *rulelist = GMatrixEditGet(classrules,&rows);
    CCD_NewClassRule(classrules,r,c);
return( copy( rulelist[r].u.md_str ) );		/* We change the display to get the new value, but we must return something... */
}

static int CCD_SameAsClasses(GGadget *g, GEvent *e) {
    int ison = GGadgetIsChecked(g);
    int cid = GGadgetGetCid(g);
    struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));

    if ( cid < CID_SameAsClasses_S ) {
	int off = cid-CID_SameAsClasses;
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_GList+300+off),!ison);
return( true );
    } else {
	int off = cid-CID_SameAsClasses_S;
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_CList_Simple+off),!ison);
return( true );
    }
}

static void CCD_ClassSelected(GGadget *g, int r, int c) {
    struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));
    int off = GGadgetGetCid(g)-CID_ClassList;
    int rows, cols = GMatrixEditGetColCnt(g);
    struct matrix_data *classes = GMatrixEditGet(g,&rows);
    GGadget *tf = GWidgetGetControl(ccd->gw,CID_ClassNumbers+off);
    char buf[20];
    unichar_t ubuf[80];

    if ( r<0 || r>=rows )
return;
    if ( classes[cols*r+0].u.md_str == NULL || classes[cols*r+0].u.md_str[0]=='\0' ) {
	sprintf( buf, " %d ", r );
	uc_strcpy(ubuf,buf);
    } else {
	ubuf[0]=' ';
	utf82u_strncpy(ubuf+1,classes[cols*r+0].u.md_str,sizeof(ubuf)/sizeof(ubuf[0])-2 );
	ubuf[sizeof(ubuf)/sizeof(ubuf[0])-2] = '\0';
	uc_strcat(ubuf," ");
    }
    GTextFieldReplace(tf,ubuf);
return;
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
    ret = malloc(*cnt*sizeof(char *));
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
    FPST *fpst = ccd->fpst, *dummyfpst;
    int len, i, k, had_class0, clen[3], ccols;
    struct matrix_data *old, *classes[3], *classes_simple;
    struct fpst_rule dummy;
    char *msg;
    int ans, cnt, cols, first, last, forward_start;
    int iswarning;
    char *temp;
    int has[3];
    char *buts[3];

    buts[0] = _("_Yes"); buts[1] = _("_No"); buts[2] = NULL;

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
	fpst->rules = calloc(len,sizeof(struct fpst_rule));
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
	fpst->rules = calloc(len,sizeof(struct fpst_rule));
	fpst->nccnt = fpst->bccnt = fpst->fccnt = 0;
	fpst->nclass = fpst->bclass = fpst->fclass = NULL;
	has[1] = has[2] = false; has[0] = true;
	ccols = ClassNamePrep(ccd,classes,clen);
	for ( i=0; i<len; ++i ) {
	    classruleitem2rule(ccd->sf,old[i].u.md_str,&fpst->rules[i],classes,clen,ccols);
	    if ( fpst->rules[i].u.class.bcnt!=0 ) has[1] = true;
	    if ( fpst->rules[i].u.class.fcnt!=0 ) has[2] = true;
	}
	for ( i=0; i<3; ++i ) {
	    if ( i!=0 && !has[i] )
	continue;
	    (&fpst->nccnt)[i] = clen[i];
	    (&fpst->nclass)[i] = malloc(clen[i]*sizeof(char*));
	    (&fpst->nclass)[i][0] = NULL;
	    had_class0 = i==0 && !isEverythingElse(classes[i][0].u.md_str);
	    for ( k=had_class0 ? 0 : 1 ; k<clen[i]; ++k )
		(&fpst->nclass)[i][k] = GlyphNameListDeUnicode(classes[i][ccols*k+1].u.md_str);
	    for ( k=0; k<clen[i]; ++k )
		(&fpst->nclassnames)[i][k] = copy(classes[i][ccols*k+0].u.md_str);
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
	fpst->rules = calloc(1,sizeof(struct fpst_rule));
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
      case aw_classes_simple:
      case aw_glyphs_simple: {
	old = GMatrixEditGet(GWidgetGetControl(ccd->gw,ccd->aw==aw_glyphs_simple?CID_GList_Simple:CID_CList_Simple),&len);
	if ( len==0 ) {
	    ff_post_error(_("Missing rules"),_(" There must be at least one contextual rule"));
return;
	}
	dummyfpst = chunkalloc(sizeof(FPST));
	*dummyfpst = *fpst;
	dummyfpst->format = pst_glyphs;
	if ( ccd->aw==aw_classes_simple ) {
	    dummyfpst->format = pst_class;
	    for ( i=0; i<3; ++i ) { int clen;
		if ( i==0 || !GGadgetIsChecked(GWidgetGetControl(ccd->gw,CID_SameAsClasses_S+i*20)) )
		    classes_simple = GMatrixEditGet(GWidgetGetControl(ccd->gw,CID_MatchClasses+i*20),&clen);
		else	/* If SameAs the reparse the match classes_simple */
		    classes_simple = GMatrixEditGet(GWidgetGetControl(ccd->gw,CID_MatchClasses),&clen);
		(&dummyfpst->nccnt)[i] = clen;
		if ( clen!=0 ) {
		    (&dummyfpst->nclass)[i] = malloc(clen*sizeof(char*));
		    (&dummyfpst->nclassnames)[i] = malloc(clen*sizeof(char*));
		    (&dummyfpst->nclass)[i][0] = NULL;
		    (&dummyfpst->nclassnames)[i][0] = NULL;
		    had_class0 = i==0 && !isEverythingElse(classes_simple[3*0+1].u.md_str);
		    if ( !had_class0 )
			(&dummyfpst->nclassnames)[i][0] = copy(classes_simple[3*0+0].u.md_str);
		    for ( k=had_class0 ? 0 : 1 ; k<clen; ++k ) {
			(&dummyfpst->nclassnames)[i][k] = copy(classes_simple[3*k+0].u.md_str);
			(&dummyfpst->nclass)[i][k] = GlyphNameListDeUnicode(classes_simple[3*k+1].u.md_str);
		    }
		}
	    }
	} else {
	    dummyfpst->nccnt = dummyfpst->bccnt = dummyfpst->fccnt = 0;
	    dummyfpst->nclass = dummyfpst->bclass = dummyfpst->fclass = NULL;
	    dummyfpst->nclassnames = dummyfpst->bclassnames = dummyfpst->fclassnames = NULL;
	}
	dummyfpst->rule_cnt = len;
	dummyfpst->rules = calloc(len,sizeof(struct fpst_rule));
	for ( i=0; i<len; ++i ) {
	    char *temp = old[i].u.md_str;
	    if ( ccd->aw==aw_glyphs_simple )
		temp = GlyphNameListDeUnicode(temp);
	    msg = FPSTRule_From_Str(ccd->sf,dummyfpst,&dummyfpst->rules[i],temp,&iswarning);
	    if ( temp!=old[i].u.md_str )
		free(temp);
	    if ( msg!=NULL ) {
		if ( !iswarning ) {
		    ff_post_error(_("Bad rule"), "%s", msg );
		    free(msg);
		    FPSTClassesFree(dummyfpst);
		    FPSTRulesFree(dummyfpst->rules,dummyfpst->format,dummyfpst->rule_cnt);
		    chunkfree(dummyfpst,sizeof(FPST));
return;
		} else {
		    ans = gwwv_ask(_("Warning"),
			    (const char **) buts,0,1,
			    _("%s\nProceed anyway?"), msg);
		    free(msg);
		    if ( ans==1 ) {
			FPSTClassesFree(dummyfpst);
			FPSTRulesFree(dummyfpst->rules,dummyfpst->format,dummyfpst->rule_cnt);
			chunkfree(dummyfpst,sizeof(FPST));
return;
		    }
		}
	    }
	}
	FPSTRulesFree(fpst->rules,fpst->format,fpst->rule_cnt);
	fpst->format = dummyfpst->format;
	fpst->rule_cnt = dummyfpst->rule_cnt;
	fpst->rules = dummyfpst->rules;
	if ( fpst->format==pst_class ) {
	    FPSTClassesFree(fpst);
	    fpst->nccnt = dummyfpst->nccnt; fpst->bccnt = dummyfpst->bccnt; fpst->fccnt = dummyfpst->fccnt;
	    fpst->nclass = dummyfpst->nclass; fpst->bclass = dummyfpst->bclass; fpst->fclass = dummyfpst->fclass;
	    fpst->nclassnames = dummyfpst->nclassnames; fpst->bclassnames = dummyfpst->bclassnames; fpst->fclassnames = dummyfpst->fclassnames;
	}
	chunkfree(dummyfpst,sizeof(FPST));
      } break;
      case aw_coverage_simple:
	old = GMatrixEditGet(GWidgetGetControl(ccd->gw,CID_Covers),&len);
	cols = GMatrixEditGetColCnt(GWidgetGetControl(ccd->gw,CID_Covers));
	if ( len==0 ) {
	    ff_post_error(_("Bad Coverage Table"),_("There must be at least one match coverage table"));
return;
	}
	memset(&dummy,0,sizeof(dummy));
	if ( fpst->format==pst_reversecoverage ) {
	    first = last = -1;
	    for ( i=0; i<len; ++i ) {
		if ( old[cols*i+1].u.md_str!=NULL && *old[cols*i+1].u.md_str!='\0' ) {
		    if ( first==-1 )
			first = last = i;
		    else {
			ff_post_error(_("Bad Coverage Table"),_("In a Reverse Chaining Substitution there must be exactly one coverage table with replacements"));
return;
		    }
		    /* There is only this one line of replacements to add. */
		    dummy.u.rcoverage.replacements = GlyphNameListDeUnicode( old[cols*i+1].u.md_str );
		}
	    }
	    if ( first==-1 ) {
		ff_post_error(_("Bad Coverage Table"),_("In a Reverse Chaining Substitution there must be exactly one coverage table with replacements"));
return;
	    }
	    if ( GlyphNameCnt(old[cols*first+0].u.md_str)!=GlyphNameCnt(old[cols*first+1].u.md_str) ) {
		ff_post_error(_("Replacement mismatch"),_("In a Reverse Chaining Substitution there must be exactly as many replacements as there are glyph names in the match coverage table"));
return;
	    }
	    forward_start = last+1;
	} else {
	    first = last = forward_start = -1;
	    if ( fpst->type == pst_contextpos || fpst->type == pst_contextsub ) {
		first = 0;
		forward_start = len;
		/* No check here to see if any lookups invoked. It's only a warning */
		/*  Do we need to add it???? */
	    } else {
		for ( i=0; i<len; ++i ) {
		    if ( old[cols*i+2].u.md_ival ) {
			if ( first==-1 )
			    first = last = i;
			else if ( forward_start==-1 ) {
			    if ( old[cols*i+1].u.md_addr!=NULL ) {
				ff_post_error(_("Bad Sections"),_("The sections specified do not make sense. All lookups must lie in the middle section."));
return;
			    }
			    forward_start = i;
			} else {
			    ff_post_error(_("Bad Sections"),_("The sections specified do not make sense. All lookups must lie in the middle section."));
return;
			}
		    } else if ( old[cols*i+1].u.md_addr!=NULL ) {
			if ( first==-1 )
			    first = last = i;
			else if ( forward_start!=-1 ) {
			    ff_post_error(_("Bad Sections"),_("The sections specified do not make sense. All lookups must lie in the middle section."));
return;
			}
		    }
		}
		if ( forward_start==-1 && last!=-1 )
		    forward_start = last+1;
		if ( first==-1 ) {
		    ans = gwwv_ask(_("Warning"),
			    (const char **) buts,0,1,
			    _("This rule activates no lookups.\nProceed anyway?"));
		    if ( ans==1 )
return;
		    first = 0;
		    forward_start = len;
		}
	    }
	    for ( i=first, cnt=0; i<forward_start; ++i )
		if ( old[cols*i+1].u.md_addr!=NULL )
		    ++cnt;
	    dummy.lookup_cnt = cnt;
	    if ( cnt!=0 ) {
		dummy.lookups = calloc(cnt,sizeof(struct seqlookup));
		for ( i=first, cnt=0; i<forward_start; ++i ) {
		    if ( old[cols*i+1].u.md_addr!=NULL ) {
			dummy.lookups[cnt].lookup = old[cols*i+1].u.md_addr;
			dummy.lookups[cnt].seq = i-first;
			++cnt;
		    }
		}
	    }
	}
	dummy.u.coverage.bcnt = first;
	dummy.u.coverage.ncnt = forward_start-first;
	dummy.u.coverage.fcnt = len-forward_start;
	dummy.u.coverage.ncovers = malloc(dummy.u.coverage.ncnt*sizeof(char *));
	if ( dummy.u.coverage.bcnt!=0 )
	    dummy.u.coverage.bcovers = malloc(dummy.u.coverage.bcnt*sizeof(char *));
	if ( dummy.u.coverage.fcnt!=0 )
	    dummy.u.coverage.fcovers = malloc(dummy.u.coverage.fcnt*sizeof(char *));
	for ( i=0; i<first; ++i )
	    dummy.u.coverage.bcovers[first-1-i] = GlyphNameListDeUnicode( old[cols*i+0].u.md_str );
	for ( i=0; i<dummy.u.coverage.ncnt; ++i )
	    dummy.u.coverage.ncovers[i] = GlyphNameListDeUnicode( old[cols*(first+i)+0].u.md_str );
	for ( i=forward_start; i<len; ++i )
	    dummy.u.coverage.fcovers[i-forward_start] = GlyphNameListDeUnicode( old[cols*i+0].u.md_str );
	FPSTClassesFree(fpst);
	FPSTRulesFree(fpst->rules,fpst->format,fpst->rule_cnt);
	fpst->format = fpst->type==pst_reversesub ? pst_reversecoverage : pst_coverage;
	fpst->rule_cnt = 1;
	fpst->rules = calloc(1,sizeof(struct fpst_rule));
	fpst->rules[0] = dummy;
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
	    if ( GGadgetIsChecked(GWidgetGetControl(ccd->gw,CID_Simple)) ) {
		if ( GGadgetIsChecked(GWidgetGetControl(ccd->gw,CID_ByGlyph)) ) {
		    ccd->aw = aw_glyphs_simple;
		    GDrawSetVisible(ccd->glyphs_simple,true);
		} else if ( GGadgetIsChecked(GWidgetGetControl(ccd->gw,CID_ByClass)) ) {
		    ccd->aw = aw_classes_simple;
		    GDrawSetVisible(ccd->classes_simple,true);
		} else {
		    ccd->aw = aw_coverage_simple;
		    GDrawSetVisible(ccd->coverage_simple,true);
		}
	    } else {
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
	  case aw_coverage_simple:
	  case aw_classes_simple:
	  case aw_glyphs_simple:
	    ccd->aw = aw_formats;
	    GDrawSetVisible(ccd->coverage_simple,false);
	    GDrawSetVisible(ccd->classes_simple,false);
	    GDrawSetVisible(ccd->glyphs_simple,false);
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
    } else {
    }
}

static int CCD_NewSection(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	int off = GGadgetGetCid(g)-CID_GNewSection;
	GGadget *gme = GWidgetGetControl(GGadgetGetWindow(g),CID_GList_Simple+off);
	GGadget *tf = _GMatrixEditGetActiveTextField(gme);
	static const unichar_t section_mark[] = { ' ', '|', ' ', '\0' };

	if ( tf!=NULL )
	    GTextFieldReplace(tf,section_mark);
    }
return( true );
}

static int CCD_AddLookup(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	int off = GGadgetGetCid(g)-CID_GAddLookup;
	GGadget *gme = GWidgetGetControl(GGadgetGetWindow(g),CID_GList_Simple+off);
	GGadget *tf = _GMatrixEditGetActiveTextField(gme);
	GTextInfo *lookup_ti = GGadgetGetListItemSelected(g);
	OTLookup *otl;

	if ( tf!=NULL && lookup_ti!=NULL && lookup_ti->userdata!=NULL) {
	    char *space;
	    unichar_t *temp;
	    otl = lookup_ti->userdata;
	    space = malloc(strlen(otl->lookup_name)+8);
	    sprintf( space, " @<%s> ", otl->lookup_name );
	    temp = utf82u_copy(space);
	    GTextFieldReplace(tf,temp);
	    free(space);
	    free(temp);
	}
	GGadgetSelectOneListItem(g,0);
    }
return( true );
}

static int subccd_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("ui/dialogs/contextchain.html", NULL);
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
	    help("ui/dialogs/contextchain.html", NULL);
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
	GDrawResize(ccd->coverage_simple,wsize.width,wsize.height);
	GDrawResize(ccd->glyphs_simple,wsize.width,wsize.height);
	GDrawResize(ccd->classes_simple,wsize.width,wsize.height);

	GDrawRequestExpose(ccd->gw,NULL,false);
	GDrawRequestExpose(ccd->aw==aw_formats ? ccd->formats :
			    ccd->aw==aw_coverage ? ccd->coverage :
			    ccd->aw==aw_grules ? ccd->grules :
			    ccd->aw==aw_glyphs ? ccd->glyphs :
			    ccd->aw==aw_classrules ? ccd->classrules :
			    ccd->aw==aw_classnumber ? ccd->classnumber :
			    ccd->aw==aw_coverage_simple ? ccd->coverage_simple :
			    ccd->aw==aw_classes_simple ? ccd->classes_simple :
					ccd->glyphs_simple, NULL,false);
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

static unichar_t **CCD_ClassListCompletion(GGadget *t,int from_tab) {
    struct contextchaindlg *ccd = GDrawGetUserData(GDrawGetParentWindow(GGadgetGetWindow(t)));
    unichar_t *pt, *spt, *basept, *wild; unichar_t **ret;
    int i, cnt, doit, match_len;
    int do_wildcards;
    int section;
    int namecnt;
    int cid;
    struct matrix_data *classnames;

    section = 0;

    pt = spt = basept = (unichar_t *) _GGadgetGetTitle(t);
    if ( pt==NULL || *pt=='\0' )
return( NULL );

    if ( ccd->fpst->type==pst_contextsub || ccd->fpst->type==pst_contextpos )
	section=1;
    else {
	for ( pt=basept; *pt ; ++pt )
	    if ( *pt=='|' )
		++section;
	if ( section>2 )
	    section=2;
    }
    if ( section==0 && !GGadgetIsChecked(GWidgetGetControl(ccd->gw,CID_BackClassesSameAs_S)) )
	cid = CID_BackClasses;
    else if ( section==2 && !GGadgetIsChecked(GWidgetGetControl(ccd->gw,CID_ForeClassesSameAs_S)) )
	cid = CID_ForeClasses;
    else
	cid = CID_MatchClasses;
    classnames = GMatrixEditGet(GWidgetGetControl(ccd->gw,cid),&namecnt);
    if ( namecnt==0 )		/* No classes_simple specified yet, can't help */
return( NULL );

    if (( spt = u_strrchr(spt,' '))== NULL )
	spt = basept;
    else {
	pt = ++spt;
	if ( *pt=='\0' )
return( NULL );
    }
    while ( *pt && *pt!='*' && *pt!='?' && *pt!='[' && *pt!='{' )
	++pt;
    do_wildcards = *pt!='\0';

    if ( do_wildcards && !from_tab )
return( NULL );

    wild = NULL;
    if ( do_wildcards ) {
	pt = spt;
	wild = malloc((u_strlen(spt)+2)*sizeof(unichar_t));
	u_strcpy(wild,pt);
	uc_strcat(wild,"*");
    }

    match_len = u_strlen(spt);
    ret = NULL;
    for ( doit=0; doit<2; ++doit ) {
	cnt=0;
	for ( i=0; i<namecnt; ++i ) if ( classnames[3*i+0].u.md_str!=NULL ) {
	    int matched;
	    char *str = classnames[3*i+0].u.md_str;
	    if ( do_wildcards ) {
		unichar_t *temp = utf82u_copy(str);
		matched = GGadgetWildMatch((unichar_t *) wild,temp,false);
		free(temp);
	    } else
		matched = uc_strncmp(spt,str,match_len)==0;
	    if ( matched ) {
		if ( doit ) {
		    if ( spt==basept ) {
			ret[cnt] = utf82u_copy(str);
		    } else {
			unichar_t *temp = malloc((spt-basept+strlen(str)+4)*sizeof(unichar_t));
			int len;
			u_strncpy(temp,basept,spt-basept);
			utf82u_strcpy(temp+(spt-basept),str);
			len = u_strlen(temp);
			ret[cnt] = temp;
		    }
		}
		++cnt;
	    }
	}
	if ( doit )
	    ret[cnt] = NULL;
	else if ( cnt==0 )
    break;
	else
	    ret = malloc((cnt+1)*sizeof(unichar_t *));
    }
    free(wild);
return( ret );
}

static void CCD_FinishCoverageEdit(GGadget *g,int r, int c, int wasnew) {
    struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));

    ME_SetCheckUnique(g, r, c, ccd->sf);
}

static void CCD_FinishCoverageSimpleEdit(GGadget *g,int r, int c, int wasnew) {
    struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));
    int cols = GMatrixEditGetColCnt(g);
    int first, last, i;
    int rows;
    struct matrix_data *covers = _GMatrixEditGet(g,&rows);


    if ( c==0 )
	ME_SetCheckUnique(g, r, c, ccd->sf);
    else if ( c==1 && cols==2 ) {	/* The replacement list in a reverse coverage_simple */
	ME_ListCheck(g, r, c, ccd->sf);
    } else if ( c==1 && cols==4 ) {	/* The lookup in a coverage_simple line (not reverse coverage_simple) */
	if ( covers[cols*r+c].u.md_addr== (void *) (intpt) -1 ) {
	    /* They asked to remove the lookup that was there */
	    covers[cols*r+c].u.md_addr = NULL;
	} else if ( covers[cols*r+c].u.md_addr==NULL ) {
	    /* The selected the "Add Lookup" item which should be a no-op */
	    /*  but actually sets the thing to 0 */
	    covers[cols*r+c].u.md_addr = covers[cols*r+3].u.md_addr;
	}
	/* Save the new value somewhere safe */
	covers[cols*r+3].u.md_addr = covers[cols*r+c].u.md_addr;
	first = last = -1;
	for ( i=0; i<rows; ++i ) {
	    if ( covers[cols*i+c].u.md_addr!=NULL ) {
		if ( first==-1 )
		    first = i;
		last = i;
	    }
	}
	if ( first!=-1 ) {
	    /* there may not be a section break between first and last */
	    for ( i=first; i<=last; ++i )
		covers[cols*i+2].u.md_ival = false;
	    for ( i=0; i<first; ++i )
		if ( covers[cols*i+2].u.md_ival )
	    break;
	    if ( i==first )
		covers[cols*first+2].u.md_ival = true;
	    for ( i=rows-1; i>last; --i )
		if ( covers[cols*i+2].u.md_ival )
	    break;
	    if ( i<=last && last+1<rows )
		covers[cols*(last+1)+2].u.md_ival = true;
	}
	GGadgetRedraw(g);
    } else if ( c==2 ) {
	first = last = -1;
	for ( i=0; i<rows; ++i ) {
	    if ( covers[cols*i+1].u.md_addr!=NULL ) {
		if ( first==-1 )
		    first = i;
		last = i;
	    }
	}
	if ( covers[cols*i+2].u.md_ival ) {
	    if ( r<=first ) {
		for ( i=0; i<=first; ++i ) if ( i!=r )
		    covers[cols*i+2].u.md_ival = false;
	    } else if ( r<=last ) {
		covers[cols*i+2].u.md_ival = false;
		GDrawBeep(NULL);
	    } else {
		for ( i=last+1; i<=rows; ++i ) if ( i!=r )
		    covers[cols*i+2].u.md_ival = false;
	    }
	} else {
	    for ( i=first; i>=0; --i )
		if ( covers[cols*i+2].u.md_ival )
	    break;
	    if ( i==0 )
		covers[cols*0+2].u.md_ival = true;
	}
	GGadgetRedraw(g);
    }
}

static int WhichSections(struct contextchaindlg *ccd, GGadget *g) {
    int off = GGadgetGetCid(g)-CID_MatchClasses;
    int sections = 0;

    if ( ccd->fpst->type==pst_contextpos || ccd->fpst->type==pst_contextsub )
	sections = 0x7;		/* All sections */
    else if ( off==0 ) {
	sections = 0x2;		/* Match */
	if ( GGadgetIsChecked(GWidgetGetControl(ccd->classes_simple,CID_SameAsClasses_S+20)))
	    sections |= 0x1;
	if ( GGadgetIsChecked(GWidgetGetControl(ccd->classes_simple,CID_SameAsClasses_S+40)))
	    sections |= 0x4;
    } else if ( off==20 )
	sections = 0x1;		/* Backtracking */
    else
	sections = 0x4;		/* Foreward */
return( sections );
}

static void RenameClass(struct contextchaindlg *ccd,char *old,char *new,int sections) {
    /* A class has been renamed. We should fix up all class rules that use */
    /*  the old name to use the new one. The name may only be valid in one */
    /*  section of the rule (backtrack, match, or forward), or in all, or */
    /*  in match and one of the other two */
    int rows, i;
    GGadget *gclassrules = GWidgetGetControl(ccd->classes_simple,CID_CList_Simple);
    struct matrix_data *classrules = GMatrixEditGet(gclassrules,&rows);
    int cols = GMatrixEditGetColCnt(gclassrules);
    char *pt, *last_name, *temp;
    int ch;

    if ( sections==0x7 ) {
	/* name change applies through out rule, no need to search for sections */
	for ( i=0; i<rows; ++i ) {
	    char *oldrule = classrules[cols*i+0].u.md_str;
	    char *newrule = rpl(oldrule,old,new);
	    free(oldrule);
	    classrules[cols*i+0].u.md_str = newrule;
	}
    } else {
	for ( i=0; i<rows; ++i ) {
	    char *end_back = NULL;
	    char *end_match = NULL;
	    char *oldrule = classrules[cols*i+0].u.md_str;
	    char *newrule;
	    for ( pt=last_name=oldrule; *pt; ) {
		while ( isspace(*pt)) ++pt;
		if ( *pt=='|' ) {
		    if ( end_back == NULL )
			end_back = pt;
		    else if ( end_match==NULL )
			end_match = pt;
		} else if ( end_back==NULL && ( *pt=='<' || *pt=='@' )) {
		    end_back = last_name;
		} else
		    last_name = pt;
		if ( *pt=='<' || *pt=='@' ) {
		    while ( *pt!='>' && *pt!='\0' ) ++pt;
		    if ( *pt=='>' ) ++pt;
		} else {
		    while ( !isspace( *pt ) && *pt!='\0' )
			++pt;
		}
	    }
	    if ( (sections==0x3 && end_match==NULL) ||
		    (sections==0x6 && end_back==NULL) ||
		    (sections==0x2 && end_match==NULL && end_back==NULL)) {
		newrule = rpl(oldrule,old,new);
	    } else if ( sections==0x3 ) {
		ch = *end_match; *end_match='\0';
		temp = rpl(oldrule,old,new);
		*end_match = ch;
		newrule = strconcat(temp,end_match);
		free(temp);
	    } else if ( sections==0x6 ) {
		temp = rpl(end_back,old,new);
		ch = *end_back; *end_back = '\0';
		newrule = strconcat(oldrule,temp);
		*end_back = ch;
		free(temp);
	    } else if ( sections==0x1 ) {
		if ( end_back==NULL )
		    newrule=NULL;
		else {
		    ch = *end_back; *end_back = '\0';
		    temp = rpl(oldrule,old,new);
		    *end_back = ch;
		    newrule = strconcat(temp,end_back);
		    free(temp);
		}
	    } else if ( sections==0x2 ) {
		if ( end_match==NULL ) {
		    temp = rpl(end_back,old,new);	/* end_back is not NULL, checked above */
		    ch = *end_back; *end_back = '\0';
		    newrule = strconcat(oldrule,temp);
		    *end_back = ch;
		    free(temp);
		} else if ( end_back==NULL ) {
		    ch = *end_match; *end_match = '\0';
		    temp = rpl(oldrule,old,new);
		    *end_match = ch;
		    newrule = strconcat(temp,end_match);
		    free(temp);
		} else {
		    ch = *end_match; *end_match = '\0';
		    temp = rpl(end_back,old,new);
		    *end_match = ch;
		    ch = *end_back; *end_back = '\0';
		    newrule = strconcat3(oldrule,temp,end_match);
		    *end_back = ch;
		    free(temp);
		}
	    } else if ( sections==0x4 ) {
		if ( end_match==NULL )
		    newrule=NULL;
		else {
		    temp = rpl(end_match,old,new);
		    ch = *end_match; *end_match = '\0';
		    newrule = strconcat(oldrule,temp);
		    *end_match = ch;
		    free(temp);
		}
	    } else
		newrule = NULL;
	    if ( newrule!=NULL ) {
		free(oldrule);
		classrules[cols*i+0].u.md_str = newrule;
	    }
	}
    }
    GGadgetRedraw(gclassrules);
}

static void CCD_FinishClassEdit(GGadget *g,int r, int c, int wasnew) {
    struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));

    if ( c==1 )
	ME_ClassCheckUnique(g, r, c, ccd->sf);
    else if ( c==0 ) {
	int rows, i;
	struct matrix_data *classes_simple = _GMatrixEditGet(g,&rows);
	char buffer[40], *end, *pt;

	if ( strchr(classes_simple[3*r+0].u.md_str,' ')!=NULL ) {
	    for ( pt=classes_simple[3*r+0].u.md_str; *pt; ++pt) {
		if ( isspace(*pt))
		    *pt = '_';
	    }
	    GGadgetRedraw(g);
	    ff_post_error(_("Bad class name"),_("No spaces allowed in class names."));
	}
	i = strtol(classes_simple[3*r+0].u.md_str,&end,10);
	if ( *end=='\0' && i!=r ) {
	    sprintf( buffer,"%d",r );
	    free( classes_simple[3*r+0].u.md_str );
	    classes_simple[3*r+0].u.md_str = copy(buffer);
	    GGadgetRedraw(g);
	    ff_post_error(_("Bad class name"),_("If a class name is a number, it must be the index of the class in the array of classes_simple."));
	}
	for ( i=0; i<rows; ++i ) if ( i!=r ) {
	    if ( strcmp(classes_simple[3*i+0].u.md_str,classes_simple[3*r+0].u.md_str) == 0 ) {
		sprintf( buffer,"%d",r );
		free( classes_simple[3*r+0].u.md_str );
		classes_simple[3*r+0].u.md_str = copy(buffer);
		GGadgetRedraw(g);
		ff_post_error(_("Bad class name"),_("The class name, %s, is already in use."), classes_simple[3*i+0].u.md_str);
	    }
	}
	if ( strcmp(classes_simple[3*r+0].u.md_str,classes_simple[3*r+2].u.md_str)!=0 ) {
	    RenameClass(ccd,classes_simple[3*r+2].u.md_str,classes_simple[3*r+0].u.md_str,WhichSections(ccd,g));
	    free(classes_simple[3*r+2].u.md_str);
	    classes_simple[3*r+2].u.md_str = copy(classes_simple[3*r+0].u.md_str);
	}
    }
}

static void CCD_ClassGoing(GGadget *g,int r) {
    /* We are about to delete this row in the class dlg, so fix up all numeric */
    /*  classes_simple after it */
    struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));
    int sections = WhichSections(ccd,g);
    int rows, i;
    struct matrix_data *classes_simple = _GMatrixEditGet(g,&rows);
    char buffer[20], *end;

    for ( i=r+1; i<rows; ++i ) {
	strtol(classes_simple[3*i+0].u.md_str,&end,10);
	if ( *end=='\0' ) {
	    sprintf(buffer,"%d",i-1);
	    free(classes_simple[3*i+0].u.md_str);
	    classes_simple[3*i+0].u.md_str = copy(buffer);
	    RenameClass(ccd,classes_simple[3*i+2].u.md_str,buffer,sections);
	    free(classes_simple[3*i+2].u.md_str);
	    classes_simple[3*i+2].u.md_str = copy(buffer);
	}
    }
}

static void CCD_InitClassRow(GGadget *g,int r) {
    char buffer[20];
    int rows;
    struct matrix_data *classes_simple = _GMatrixEditGet(g,&rows);

    sprintf( buffer, "%d", r );
    classes_simple[3*r+0].u.md_str = copy(buffer);
}

static int CCD_EnableDeleteClass(GGadget *g,int whichclass) {
return( whichclass>0 );
}

static enum gme_updown CCD_EnableUpDown(GGadget *g,int r) {
    int rows;
    enum gme_updown ret = 0;

    (void) GMatrixEditGet(g,&rows);
    if ( r>=2 )
	ret = ud_up_enabled;
    if ( r>=1 && r<rows-1 )
	ret |= ud_down_enabled;
return( ret );
}

static GTextInfo section[] = {
    { (unichar_t *) N_("Section|Continue"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Section|Start"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static struct col_init class_ci[] = {
    { me_string, NULL, NULL, NULL, N_("Class|Name") },
    { me_funcedit, CCD_PickGlyphsForClass, NULL, NULL, N_("Glyphs in the class") },
    { me_string, NULL, NULL, NULL, "Old Name (hidden)" },
    };
static struct col_init classlist_ci[] = {
    { me_string, NULL, NULL, NULL, N_("Class|Name") },
    { me_string, NULL, NULL, NULL, N_("Glyphs in the class") },
    };
static struct col_init coverage_ci[] = {
    { me_funcedit, CCD_PickGlyphsForClass, NULL, NULL, N_("Glyphs in the coverage tables") },
    };
static struct col_init seqlookup_ci[] = {
    { me_enum, NULL, NULL, NULL, N_("Apply lookup") },
    { me_int, NULL, NULL, NULL, N_("at position") },
    };
static struct col_init glyphrules_ci[] = {
    { me_onlyfuncedit, _CCD_NewGlyphRule, NULL, NULL, N_("Matching rules based on a list of glyphs") },
    };
static struct col_init classrules_ci[] = {
    { me_onlyfuncedit, _CCD_NewClassRule, NULL, NULL, N_("Matching rules based on a list of classes") },
    };
static struct col_init coveragesimple_ci[] = {
    { me_funcedit, CCD_PickGlyphsForClass, NULL, NULL, N_("Glyphs in the coverage tables") },
    { me_enum, NULL, NULL, NULL, N_("Apply lookup") },
    { me_enum, NULL, section, NULL, N_("Section")},
    { me_addr, NULL, section, NULL, "Old lookup, internal use only"},
    };
static struct col_init rcoveragesimple_ci[] = {
    { me_funcedit, CCD_PickGlyphsForClass, NULL, NULL, N_("Glyphs in the coverage tables") },
    { me_funcedit, CCD_PickGlyphsForClass, NULL, NULL, N_("Replacement glyphs") },
    };
static struct col_init glyph_ci[] = {
    { me_string, NULL, NULL, NULL, N_("Matching rules based on a list of glyphs") },
    };
static struct col_init classsimple_ci[] = {
    { me_string, NULL, NULL, NULL, N_("Matching rules based on a list of classes") },
    };

/* The "simple" dialog has several problems:
  1. The order in which lookups are to be applied is lost, they will be
      applied in sequence order
  2. In coverage_simple classes_simple only one lookup may be applied for any given
      coverage_simple set.
  Adobe's feature files have these restrictions and more, so my hope is that
   they will not matter.
  If they do matter, then we fall back on the old, more complex (I think more
   complex anyway) format.
*/
static int CanBeSimple(FPST *fpst) {
    int i, s, last_seq;
    for ( i=0; i<fpst->rule_cnt; ++i ) {
	struct fpst_rule *r = &fpst->rules[i];

	last_seq = -1;
	for ( s=0; s<r->lookup_cnt; ++s ) {
	    if ( last_seq>r->lookups[s].seq )
return( false );
	    else if ( last_seq==r->lookups[s].seq &&
		    (fpst->format==pst_coverage || fpst->format==pst_reversecoverage))
return( false );
	    last_seq = r->lookups[s].seq;
	}
    }
return( true );
}

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
	clgcd[10], subboxes[3][5], topbox[3], classbox[2], extrabuttonsgcd[3];
    GTextInfo gllabel[8], flabel[3], glabel[3][14], clabel[3][18],
	cllabel[10], extrabuttonslab[3];
    GGadgetCreateData bgcd[6], rgcd[15], boxes[4], *barray[10], *hvarray[7][2];
    GGadgetCreateData *subvarray[3][10], *subvarray2[4], *subvarray3[4], *topvarray[8],
	    *classv[4], *subvarray4[4], *barray2[6];
    GTextInfo blabel[5], rlabel[15];
    struct fpst_rule *r=NULL;
    FPST *tempfpst;
    struct matrixinit coverage_mi[3], class_mi[3], classlist_mi[3],
	gl_seqlookup_mi, cl_seqlookup_mi,
	co_seqlookup_mi, classrules_mi, glyphrules_mi,
	coveragesimple_mi, classsimple_mi, glyphs_mi;
    static int inited=false;
    int isgpos = ( fpst->type==pst_contextpos || fpst->type==pst_chainpos );
    struct matrix_data *md;
    GTextInfo *lookup_list, *addlookup_list, *addrmlookup_list;
    int use_simple = CanBeSimple(fpst);
    struct matrix_data *_classes[3]; int clen[3];

    lookup_list = SFLookupArrayFromType(sf,isgpos?gpos_start:gsub_start);
    for ( i=0; lookup_list[i].text!=NULL; ++i ) {
	if (fpst!=NULL && lookup_list[i].userdata == fpst->subtable->lookup )
	    lookup_list[i].disabled = true;
	lookup_list[i].selected = false;
    }
    addlookup_list = calloc(i+3,sizeof(GTextInfo));
    memcpy(addlookup_list+1,lookup_list,(i+1)*sizeof(GTextInfo));
    addrmlookup_list = calloc(i+3,sizeof(GTextInfo));
    memcpy(addrmlookup_list+2,lookup_list,(i+1)*sizeof(GTextInfo));
    addlookup_list[0].text = (unichar_t *) S_("Add Lookup");
    addlookup_list[0].text_is_1byte = true;
    addlookup_list[0].selected = true;
    addrmlookup_list[0].text = (unichar_t *) S_("Add Lookup");
    addrmlookup_list[0].text_is_1byte = true;
    addrmlookup_list[0].selected = true;
    addrmlookup_list[1].text = (unichar_t *) S_("Remove Lookup");
    addrmlookup_list[1].text_is_1byte = true;
    addrmlookup_list[1].selected = false;
    addrmlookup_list[1].userdata = (void *) (intpt) -1;
    coveragesimple_ci[1].enum_vals = addrmlookup_list;
    seqlookup_ci[0].enum_vals = lookup_list;

    if ( !inited ) {
	inited = true;
	section[0].text = (unichar_t *) S_( (char *) section[0].text);
	section[1].text = (unichar_t *) S_( (char *) section[1].text);
	classlist_ci[0].title = class_ci[0].title = S_(class_ci[0].title);
	classlist_ci[1].title = class_ci[1].title = S_(class_ci[1].title);
	coverage_ci[0].title = S_(coverage_ci[0].title);
	seqlookup_ci[0].title = S_(seqlookup_ci[0].title);
	seqlookup_ci[1].title = S_(seqlookup_ci[1].title);
	glyphrules_ci[0].title = S_(glyphrules_ci[0].title);
	classrules_ci[0].title = S_(classrules_ci[0].title);
	coveragesimple_ci[0].title = S_(coveragesimple_ci[0].title);
	coveragesimple_ci[1].title = S_(coveragesimple_ci[1].title);
	coveragesimple_ci[2].title = S_(coveragesimple_ci[2].title);
	rcoveragesimple_ci[0].title = S_(rcoveragesimple_ci[0].title);
	rcoveragesimple_ci[1].title = S_(rcoveragesimple_ci[1].title);
	glyph_ci[0].title = S_(glyph_ci[0].title);
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
	ccd->aw = use_simple ? aw_coverage_simple : aw_coverage;
    } else if ( ccd->isnew || fpst->rule_cnt==0) {
	fpst->format = pst_class;
	bgcd[1].gd.flags = gg_visible;
	bgcd[3].gd.flags = gg_visible | gg_enabled;
	ccd->aw = aw_formats;
    } else if ( fpst->format==pst_coverage ) {
	bgcd[2].gd.flags = gg_visible | gg_enabled;
	bgcd[3].gd.flags = gg_visible;
	ccd->aw = use_simple ? aw_coverage_simple : aw_coverage;
	    /* different buttens are enabled from those of reversesub above */
    } else if ( fpst->format==pst_class ) {
	bgcd[2].gd.flags = gg_visible | gg_enabled;
	bgcd[3].gd.flags = gg_visible;
	ccd->aw = use_simple ? aw_classes_simple : aw_classrules;
    } else {
	bgcd[2].gd.flags = gg_visible | gg_enabled;
	bgcd[3].gd.flags = gg_visible;
	ccd->aw = use_simple ? aw_glyphs_simple : aw_grules;
    }

    GGadgetsCreate(gw,boxes);
    GHVBoxSetExpandableRow(boxes[0].ret,0);
    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandgluesame);
    /*GHVBoxFitWindow(boxes[0].ret);*/

    wattrs.mask = wam_events;
    ccd->formats = GWidgetCreateSubWindow(ccd->gw,&subpos,subccd_e_h,ccd,&wattrs);
    memset(&rlabel,0,sizeof(rlabel));
    memset(&rgcd,0,sizeof(rgcd));
    memset(boxes,0,sizeof(boxes));

    i = 0;
    rgcd[i].gd.pos.x = 5; rgcd[i].gd.pos.y = 5+i*13;
    rgcd[i].gd.flags = gg_visible | gg_enabled;
    rlabel[i].text = (unichar_t *) _(
	    "OpenType Contextual or Chaining subtables may be in one\n"
	    " of three formats. The context may be specified either\n"
	    " as a string of specific glyphs, a string of glyph classes\n"
	    " or a string of coverage tables\n"
	    "In the first format you must specify a string of glyph-names\n"
	    " In the second format you must specify a string of class names\n"
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
    rgcd[i++].creator = GRadioCreate;
    barray[3] = &rgcd[i-1]; barray[4] = GCD_Glue; barray[5] = NULL;

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = barray;
    boxes[2].creator = GHBoxCreate;
    hvarray[1][0] = &boxes[2]; hvarray[1][1] = NULL;

    rgcd[i].gd.flags = gg_visible | gg_enabled;
    rlabel[i].text = (unichar_t *) _(
	    "This dialog has two formats. A simpler one which\n"
	    " hides some of the complexities of these rules,\n"
	    " or a more complex form which gives you full control.");
    rlabel[i].text_is_1byte = true;
    rgcd[i].gd.label = &rlabel[i];
    rgcd[i++].creator = GLabelCreate;
    hvarray[2][0] = GCD_HPad10; hvarray[2][1] = NULL;
    hvarray[3][0] = &rgcd[i-1]; hvarray[3][1] = NULL;

    rgcd[i].gd.pos.x = 5; rgcd[i].gd.pos.y = 5+i*13;
    rgcd[i].gd.flags = gg_visible | gg_enabled;
    rlabel[i].text = (unichar_t *) _( "Dialog Type:");
    rlabel[i].text_is_1byte = true;
    rgcd[i].gd.label = &rlabel[i];
    rgcd[i++].creator = GLabelCreate;
    barray2[0] = GCD_HPad10; barray2[1] = &rgcd[i-1];

    rgcd[i].gd.pos.x = rgcd[i-1].gd.pos.x; rgcd[i].gd.pos.y = rgcd[i-1].gd.pos.y+16;
    rgcd[i].gd.flags = gg_visible | gg_enabled | (use_simple ? gg_cb_on : 0 );
    rlabel[i].text = (unichar_t *) _("Simple");
    rlabel[i].text_is_1byte = true;
    rgcd[i].gd.label = &rlabel[i];
    rgcd[i].gd.cid = CID_Simple;
    rgcd[i++].creator = GRadioCreate;
    barray2[2] = &rgcd[i-1];

    rgcd[i].gd.pos.x = rgcd[i-1].gd.pos.x; rgcd[i].gd.pos.y = rgcd[i-1].gd.pos.y+16;
    rgcd[i].gd.flags = gg_visible | gg_enabled | (!use_simple ? gg_cb_on : 0 );
    rlabel[i].text = (unichar_t *) _("Complex");
    rlabel[i].text_is_1byte = true;
    rgcd[i].gd.label = &rlabel[i];
    rgcd[i].gd.cid = CID_Complex;
    rgcd[i++].creator = GRadioCreate;
    barray2[3] = &rgcd[i-1]; barray2[4] = GCD_Glue; barray2[5] = NULL;

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = barray2;
    boxes[3].creator = GHBoxCreate;
    hvarray[4][0] = &boxes[3]; hvarray[4][1] = NULL;
    hvarray[5][0] = GCD_Glue; hvarray[5][1] = NULL; hvarray[6][0] = NULL;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = hvarray[0];
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(ccd->formats,boxes);
    GHVBoxSetExpandableRow(boxes[0].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandgluesame);
    GHVBoxSetExpandableCol(boxes[3].ret,gb_expandglue);

	/* Pane showing all rules from a glyph format, simple format */
    ccd->grules = GWidgetCreateSubWindow(ccd->gw,&subpos,subccd_e_h,ccd,&wattrs);
    memset(&gllabel,0,sizeof(gllabel));
    memset(&glgcd,0,sizeof(glgcd));

    k = 0;
    memset(&glyphrules_mi,0,sizeof(glyphrules_mi));
    glyphrules_mi.col_cnt = 1;
    glyphrules_mi.col_init = glyphrules_ci;
    if ( fpst->format==pst_glyphs && fpst->rule_cnt>0 ) {
	glyphrules_mi.initial_row_cnt = fpst->rule_cnt;
	md = calloc(fpst->rule_cnt,sizeof(struct matrix_data));
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
    GMatrixEditSetOtherButtonEnable(GWidgetGetControl(ccd->grules,CID_GList+0),CCD_NewGlyphRule);

    ccd->glyphs_simple = GWidgetCreateSubWindow(ccd->gw,&subpos,subccd_e_h,ccd,&wattrs);
    memset(&gllabel,0,sizeof(gllabel));
    memset(&glgcd,0,sizeof(glgcd));

    k = 0;
    memset(&glyphs_mi,0,sizeof(glyphs_mi));
    glyphs_mi.col_cnt = 1;
    glyphs_mi.col_init = glyph_ci;
    if ( fpst->format==pst_glyphs && fpst->rule_cnt>0 ) {
	glyphs_mi.initial_row_cnt = fpst->rule_cnt;
	md = calloc(fpst->rule_cnt,sizeof(struct matrix_data));
	for ( j=0; j<fpst->rule_cnt; ++j ) {
	    md[j+0].u.md_str = FPSTRule_To_Str(sf,fpst,&fpst->rules[j]);;
	}
	glyphs_mi.matrix_data = md;
    } else {
	glyphs_mi.initial_row_cnt = 0;
	glyphs_mi.matrix_data = NULL;
    }
    glgcd[k].gd.flags = gg_visible | gg_enabled;
    glgcd[k].gd.cid = CID_GList_Simple;
    glgcd[k].gd.u.matrix = &glyphs_mi;
    glgcd[k++].creator = GMatrixEditCreate;
    /* No need for a box, this widget fills the pane */

    GGadgetsCreate(ccd->glyphs_simple,glgcd);
    GMatrixEditSetUpDownVisible(GWidgetGetControl(ccd->glyphs_simple,CID_GList_Simple),
	    true);
    GMatrixEditSetColumnCompletion(GWidgetGetControl(ccd->glyphs_simple,CID_GList_Simple),0,
	    CCD_GlyphListCompletion);

    memset(&extrabuttonslab,0,sizeof(extrabuttonslab));
    memset(&extrabuttonsgcd,0,sizeof(extrabuttonsgcd));

    i=0;
    if ( fpst->type==pst_chainpos || fpst->type==pst_chainsub ) {
	extrabuttonsgcd[i].gd.flags = gg_visible | gg_enabled;
	extrabuttonslab[i].text = (unichar_t *) _("New Section");
	extrabuttonslab[i].text_is_1byte = true;
	extrabuttonsgcd[i].gd.label = &extrabuttonslab[i];
	extrabuttonsgcd[i].gd.cid = CID_GNewSection;
	extrabuttonsgcd[i].gd.handle_controlevent = CCD_NewSection;
	extrabuttonsgcd[i++].creator = GButtonCreate;
    }
    extrabuttonsgcd[i].gd.flags = gg_visible | gg_enabled;
    extrabuttonslab[i].text = (unichar_t *) _("Add Lookup");
    extrabuttonslab[i].text_is_1byte = true;
    extrabuttonsgcd[i].gd.u.list = addlookup_list;
    extrabuttonsgcd[i].gd.label = &extrabuttonslab[i];
    {
	extern FontInstance *_ggadget_default_font;
	GDrawSetFont(ccd->glyphs_simple,_ggadget_default_font);
    }
    extrabuttonsgcd[i].gd.pos.width = GDrawPixelsToPoints(ccd->glyphs_simple,
	    GDrawGetText8Width(ccd->glyphs_simple,(char *)extrabuttonslab[i].text,-1))+50;
    extrabuttonsgcd[i].gd.cid = CID_GAddLookup;
    extrabuttonsgcd[i].gd.handle_controlevent = CCD_AddLookup;
    extrabuttonsgcd[i++].creator = GListButtonCreate;
    GMatrixEditAddButtons(GWidgetGetControl(ccd->glyphs_simple,CID_GList_Simple),extrabuttonsgcd);

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
	ggcd[i][k].gd.popup_msg = _("Set this glyph list from a selection.");
	ggcd[i][k].gd.flags = gg_visible | gg_enabled;
	ggcd[i][k].gd.handle_controlevent = CCD_FromSelection;
	ggcd[i][k].data=(void *)((intpt)CID_GlyphList+(0*100+i*20));
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

	/* Pane showing the single rule in a coverage table (complex) */
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
	    md = calloc(cnt,sizeof(struct matrix_data));
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
		    md = calloc(2*r->lookup_cnt,sizeof(struct matrix_data));
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
		cgcd[i][k].gd.popup_msg = _("Set this glyph list from a selection.");
		cgcd[i][k].gd.flags = gg_visible | gg_enabled;
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

	/* Pane showing the single rule in a coverage table (simple) */
    ccd->coverage_simple = GWidgetCreateSubWindow(ccd->gw,&subpos,subccd_e_h,ccd,&wattrs);
    memset(&clabel,0,sizeof(clabel));
    memset(&cgcd,0,sizeof(cgcd));
    memset(&subboxes,0,sizeof(subboxes));
    memset(&flabel,0,sizeof(flabel));
    memset(&fgcd,0,sizeof(fgcd));
    memset(&topbox,0,sizeof(topbox));

    memset(&coveragesimple_mi,0,sizeof(coveragesimple_mi));
    coveragesimple_mi.col_cnt = 4;
    coveragesimple_mi.col_init = coveragesimple_ci;
    if ( fpst->format==pst_reversecoverage ) {
	coveragesimple_mi.col_cnt = 2;
	coveragesimple_mi.col_init = rcoveragesimple_ci;
    }
    if (( fpst->format==pst_coverage || fpst->format==pst_reversecoverage )
	    && fpst->rules!=NULL ) {
	int cnt = fpst->rules[0].u.coverage.ncnt+fpst->rules[0].u.coverage.bcnt+fpst->rules[0].u.coverage.fcnt;
	coveragesimple_mi.initial_row_cnt = cnt;
	md = calloc(cnt*coveragesimple_mi.col_cnt,sizeof(struct matrix_data));
	for ( i=0,j=fpst->rules[0].u.coverage.bcnt-1; j>=0; --j, ++i )
	    md[i*coveragesimple_mi.col_cnt].u.md_str = SFNameList2NameUni(sf,fpst->rules[0].u.coverage.bcovers[j]);
	for ( j=0; j<fpst->rules[0].u.coverage.ncnt; ++j, ++i ) {
	    md[coveragesimple_mi.col_cnt*i+0].u.md_str = SFNameList2NameUni(sf,fpst->rules[0].u.coverage.ncovers[j]);
	    if ( fpst->format==pst_reversecoverage )
		md[i*2+1].u.md_str = SFNameList2NameUni(sf,fpst->rules[0].u.rcoverage.replacements);
	    else {
		md[4*i+2].u.md_ival = j==0;
		for ( k=0; k<fpst->rules[0].lookup_cnt; ++k ) {
		    if ( fpst->rules[0].lookups[k].seq==j ) {
			md[4*i+1].u.md_addr = md[4*i+3].u.md_addr = fpst->rules[0].lookups[k].lookup;
		break;
		    }
		}
	    }
	}
	for ( j=0; j<fpst->rules[0].u.coverage.fcnt; ++j, ++i ) {
	    md[coveragesimple_mi.col_cnt*i].u.md_str = SFNameList2NameUni(sf,fpst->rules[0].u.coverage.fcovers[j]);
	    if ( fpst->format==pst_coverage )
		md[4*i+2].u.md_ival = j==0;
	}
	coveragesimple_mi.matrix_data = md;
    } else {
	coveragesimple_mi.initial_row_cnt = 0;
	coveragesimple_mi.matrix_data = NULL;
    }
    coveragesimple_mi.finishedit = CCD_FinishCoverageSimpleEdit;

    flabel[0].text = (unichar_t *) _("A list of coverage tables:");
    flabel[0].text_is_1byte = true;
    fgcd[0].gd.label = &flabel[0];
    fgcd[0].gd.flags = gg_visible | gg_enabled;
    fgcd[0].creator = GLabelCreate;
    topvarray[0] = &fgcd[0];

    fgcd[1].gd.flags = gg_visible | gg_enabled;
    fgcd[1].gd.cid = CID_Covers;
    fgcd[1].gd.u.matrix = &coveragesimple_mi;
    fgcd[1].creator = GMatrixEditCreate;
    topvarray[1] = &fgcd[1];
    topvarray[2] = NULL;
    topbox[0].gd.flags = gg_enabled|gg_visible;
    topbox[0].gd.u.boxelements = topvarray;
    topbox[0].creator = GVBoxCreate;
    GGadgetsCreate(ccd->coverage_simple,topbox);
    GMatrixEditSetUpDownVisible(GWidgetGetControl(ccd->coverage_simple,CID_Covers),
	    true);
    GMatrixEditSetColumnCompletion(GWidgetGetControl(ccd->coverage_simple,CID_Covers),0,
	    CCD_GlyphListCompletion);
    if ( fpst->type==pst_reversesub ) {
	GMatrixEditSetColumnCompletion(GWidgetGetControl(ccd->coverage_simple,CID_Covers),1,
		CCD_GlyphListCompletion);
    }
    GHVBoxSetExpandableRow(topbox[0].ret,1);
    if ( fpst->type == pst_contextpos || fpst->type == pst_contextsub )
	GMatrixEditShowColumn(GWidgetGetControl(ccd->coverage_simple,CID_Covers),2,false);
    if ( fpst->format!=pst_reversecoverage )
	GMatrixEditShowColumn(GWidgetGetControl(ccd->coverage_simple,CID_Covers),3,false);


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
    /* Further inits happen after we've set up the classnames */
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
	class_mi[i].col_cnt = 3;
	class_mi[i].col_init = class_ci;
	{
	    char **classes, **classnames;
	    if ( tempfpst->format==pst_class ) {
		classes = (&tempfpst->nclass)[i];
		classnames = (&tempfpst->nclassnames)[i];
	    } else {
		classes = NULL;
		classnames = NULL;
	    }
	    if ( classes==NULL )
		/* Make sure the class 0 is always present */
		cc=1;
	    else
		cc = (&tempfpst->nccnt)[i];
	    class_mi[i].initial_row_cnt = cc;
	    md = calloc(3*cc+3,sizeof(struct matrix_data));
	    md[0+0].u.md_str = copy(classnames==NULL || cc==0 || classnames[0]==NULL?S_("Glyphs|All_Others"):classnames[0]);
	    md[3*0+1].u.md_str = copy(_("{Everything Else}"));
	    md[3*0+1].frozen = true;
	    md[0+2].u.md_str = copy(md[0+0].u.md_str);
	    for ( j=1; j<cc; ++j ) {
		if ( classnames==NULL || classnames[j]==NULL ) {
		    char buffer[12];
		    sprintf( buffer,"%d",j );
		    md[3*j+0].u.md_str = copy(buffer);
		} else
		    md[3*j+0].u.md_str = copy(classnames[j]);
		md[3*j+1].u.md_str = SFNameList2NameUni(sf,classes[j]);
		md[3*j+2].u.md_str = copy(md[3*j+0].u.md_str);
	    }
	    class_mi[i].matrix_data = md;

	    clen[i] = cc;
	    _classes[i] = md;
	}
	class_mi[i].initrow = CCD_InitClassRow;
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

    /* Finish initializing the list of class rules */
    if ( tempfpst->format==pst_class && tempfpst->rule_cnt>0 ) {
	classrules_mi.initial_row_cnt = tempfpst->rule_cnt;
	md = calloc(tempfpst->rule_cnt,sizeof(struct matrix_data));
	for ( j=0; j<tempfpst->rule_cnt; ++j ) {
	    md[j+0].u.md_str = classruleitem(&tempfpst->rules[j],_classes,clen,3);
	}
	classrules_mi.matrix_data = md;
    } else {
	classrules_mi.initial_row_cnt = 0;
	classrules_mi.matrix_data = NULL;
    }

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
    GMatrixEditSetUpDownVisible(GWidgetGetControl(ccd->classrules,CID_GList+200), true);
    GMatrixEditSetOtherButtonEnable(GWidgetGetControl(ccd->classrules,CID_GList+200),CCD_NewClassRule);
    for ( i=0; i<3; ++i ) {
	GGadget *list = GWidgetGetControl(ccd->classrules,CID_GList+300+i*20);
	GMatrixEditShowColumn(list,2,false);
	GMatrixEditSetUpDownVisible(list, true);
	GMatrixEditSetCanUpDown(list, CCD_EnableUpDown);
    }

	/* This pane displays a set of class rules, and below the three sets of classes_simple for those rules */
    ccd->classes_simple = GWidgetCreateSubWindow(ccd->gw,&subpos,subccd_e_h,ccd,&wattrs);
    memset(&cllabel,0,sizeof(cllabel));
    memset(&clgcd,0,sizeof(clgcd));
    memset(&clabel,0,sizeof(clabel));
    memset(&cgcd,0,sizeof(cgcd));
    memset(&subboxes,0,sizeof(subboxes));
    memset(&topbox,0,sizeof(topbox));
    memset(&classbox,0,sizeof(classbox));
    memset(&faspects,0,sizeof(faspects));

    tempfpst = fpst;
    if ( fpst->format==pst_glyphs && fpst->rule_cnt>0 )
	tempfpst = FPSTGlyphToClass(fpst);

    k=0;

    memset(&classsimple_mi,0,sizeof(classsimple_mi));
    classsimple_mi.col_cnt = 1;
    classsimple_mi.col_init = classsimple_ci;
    if ( tempfpst->format==pst_class && tempfpst->rule_cnt>0 ) {
	classsimple_mi.initial_row_cnt = tempfpst->rule_cnt;
	md = calloc(tempfpst->rule_cnt,sizeof(struct matrix_data));
	for ( j=0; j<tempfpst->rule_cnt; ++j ) {
	    md[j+0].u.md_str = FPSTRule_To_Str(sf,tempfpst,&tempfpst->rules[j]);;
	}
	classsimple_mi.matrix_data = md;
    } else {
	classsimple_mi.initial_row_cnt = 0;
	classsimple_mi.matrix_data = NULL;
    }
    clgcd[k].gd.flags = gg_visible | gg_enabled;
    clgcd[k].gd.cid = CID_CList_Simple;
    clgcd[k].gd.u.matrix = &classsimple_mi;
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
	    cgcd[i][l].gd.cid = CID_SameAsClasses_S + i*20;
	    cgcd[i][l++].creator = GCheckBoxCreate;
	    subvarray[i][l-1] = &cgcd[i][l-1];
	}

	memset(&class_mi[i],0,sizeof(class_mi[0]));
	class_mi[i].col_cnt = 3;
	class_mi[i].col_init = class_ci;
	{
	    char **classes, **classnames;
	    if ( tempfpst->format==pst_class ) {
		classes = (&tempfpst->nclass)[i];
		classnames = (&tempfpst->nclassnames)[i];
	    } else {
		classes = NULL;
		classnames = NULL;
	    }
	    if ( classes==NULL )
		/* Make sure the class 0 is always present */
		cc=1;
	    else
		cc = (&tempfpst->nccnt)[i];
	    class_mi[i].initial_row_cnt = cc;
	    md = calloc(3*cc+3,sizeof(struct matrix_data));
/* GT: This is the default class name for the class containing any glyphs_simple */
/* GT: which aren't specified in other classes_simple. The class name may NOT */
/* GT: contain spaces. Use an underscore or something similar instead */
	    md[0+0].u.md_str = copy(classnames==NULL || cc==0 || classnames[0]==NULL?S_("Glyphs|All_Others"):classnames[0]);
	    md[0+1].u.md_str = copy(_("{Everything Else}"));
	    md[0+1].frozen = true;
	    md[0+2].u.md_str = copy(md[0+0].u.md_str);
	    for ( j=1; j<cc; ++j ) {
		if ( classnames==NULL || classnames[j]==NULL ) {
		    char buffer[12];
		    sprintf( buffer,"%d",j );
		    md[3*j+0].u.md_str = copy(buffer);
		} else
		    md[3*j+0].u.md_str = copy(classnames[j]);
		md[3*j+1].u.md_str = SFNameList2NameUni(sf,classes[j]);
		md[3*j+2].u.md_str = copy(md[3*j+0].u.md_str);
	    }
	    class_mi[i].matrix_data = md;
	}
	class_mi[i].initrow = CCD_InitClassRow;
	class_mi[i].finishedit = CCD_FinishClassEdit;
	class_mi[i].candelete = CCD_EnableDeleteClass;

	if ( sameas )
	    cgcd[i][l].gd.flags = gg_visible;
	else
	    cgcd[i][l].gd.flags = gg_visible | gg_enabled;
	cgcd[i][l].gd.cid = CID_MatchClasses+i*20;
	cgcd[i][l].gd.u.matrix = &class_mi[i];
	cgcd[i][l++].creator = GMatrixEditCreate;
	subvarray[i][l-1] = &cgcd[i][l-1];
	subvarray[i][l] = NULL;

	subboxes[i][0].gd.flags = gg_enabled|gg_visible;
	subboxes[i][0].gd.u.boxelements = subvarray[i];
	subboxes[i][0].creator = GVBoxCreate;

	faspects[i].gcd = subboxes[i];
	faspects[i].text_is_1byte = true;
    }
    j=0;
    faspects[j++].text = (unichar_t *) _("Match Classes");
    faspects[j++].text = (unichar_t *) _("Back Classes");
    faspects[j++].text = (unichar_t *) _("Ahead Classes");
    if ( fpst->type == pst_contextpos || fpst->type == pst_contextsub )
	faspects[1].disabled = faspects[2].disabled = true;

    clgcd[k].gd.u.tabs = faspects;
    clgcd[k].gd.flags = gg_visible | gg_enabled;
    clgcd[k].gd.cid = CID_ClassMatchType;
    clgcd[k].creator = GTabSetCreate;
    topvarray[1] = &clgcd[k]; topvarray[2] = NULL;
    topbox[0].gd.flags = gg_enabled|gg_visible;
    topbox[0].gd.u.boxelements = topvarray;
    topbox[0].creator = GVBoxCreate;
    GGadgetsCreate(ccd->classes_simple,topbox);

    /* Top box should give it's size equally to its two components */
    GHVBoxSetExpandableRow(classbox[0].ret,0);
    GHVBoxSetExpandableRow(subboxes[0][0].ret,0);
    GHVBoxSetExpandableRow(subboxes[1][0].ret,1);
    GHVBoxSetExpandableRow(subboxes[2][0].ret,1);
    GMatrixEditSetColumnCompletion(GWidgetGetControl(ccd->classes_simple,CID_MatchClasses),1,CCD_GlyphListCompletion);
    GMatrixEditSetColumnCompletion(GWidgetGetControl(ccd->classes_simple,CID_BackClasses),1,CCD_GlyphListCompletion);
    GMatrixEditSetColumnCompletion(GWidgetGetControl(ccd->classes_simple,CID_ForeClasses),1,CCD_GlyphListCompletion);
    GMatrixEditSetColumnCompletion(GWidgetGetControl(ccd->classes_simple,CID_CList_Simple),0,
	    CCD_ClassListCompletion);
    if ( fpst->type==pst_chainpos || fpst->type==pst_chainsub ) {
	extrabuttonsgcd[0].gd.cid = CID_CNewSection;
	extrabuttonsgcd[1].gd.cid = CID_CAddLookup;
    } else
	extrabuttonsgcd[0].gd.cid = CID_CAddLookup;
    GMatrixEditSetUpDownVisible(GWidgetGetControl(ccd->classes_simple,CID_CList_Simple), true);
    GMatrixEditAddButtons(GWidgetGetControl(ccd->classes_simple,CID_CList_Simple),extrabuttonsgcd);
    for ( i=0; i<3; ++i ) {
	GGadget *list = GWidgetGetControl(ccd->classes_simple,CID_MatchClasses+i*20);
	GMatrixEditShowColumn(list,2,false);
	GMatrixEditSetUpDownVisible(list, true);
	GMatrixEditSetCanUpDown(list, CCD_EnableUpDown);
	GMatrixEditSetBeforeDelete(list,CCD_ClassGoing);
    }
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

	clabel[i][k].text = (unichar_t *) _("List of class names");
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

	memset(&classlist_mi[i],0,sizeof(classlist_mi[0]));
	classlist_mi[i].col_cnt = 2;
	classlist_mi[i].col_init = classlist_ci;

	cgcd[i][k].gd.flags = gg_visible | gg_enabled | gg_textarea_wrap;
	cgcd[i][k].gd.cid = CID_ClassList+i*20;
	cgcd[i][k].gd.u.matrix = &classlist_mi[i];
	cgcd[i][k++].creator = GMatrixEditCreate;
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

    GMatrixEditSetEditable(GWidgetGetControl(ccd->classnumber,CID_ClassList+0*20),false);
    GMatrixEditSetEditable(GWidgetGetControl(ccd->classnumber,CID_ClassList+1*20),false);
    GMatrixEditSetEditable(GWidgetGetControl(ccd->classnumber,CID_ClassList+2*20),false);
    GMatrixEditSetOtherButtonEnable(GWidgetGetControl(ccd->classnumber,CID_ClassList+0*20),CCD_ClassSelected);
    GMatrixEditSetOtherButtonEnable(GWidgetGetControl(ccd->classnumber,CID_ClassList+1*20),CCD_ClassSelected);
    GMatrixEditSetOtherButtonEnable(GWidgetGetControl(ccd->classnumber,CID_ClassList+2*20),CCD_ClassSelected);


    if ( ccd->aw == aw_formats )
	GDrawSetVisible(ccd->formats,true);
    else if ( use_simple ) {
	if ( ccd->aw == aw_glyphs_simple )
	    GDrawSetVisible(ccd->glyphs_simple,true);
	else if ( ccd->aw == aw_classes_simple )
	    GDrawSetVisible(ccd->classes_simple,true);
	else {
	    GDrawSetVisible(ccd->coverage_simple,true);
	    GDrawResize(ccd->gw,
		    fpst->format==pst_reversecoverage?3*pos.width/2:2*pos.width,
		    pos.height);
	}
    } else {
	if ( ccd->aw == aw_grules )
	    GDrawSetVisible(ccd->grules,true);
	else if ( ccd->aw == aw_classrules )
	    GDrawSetVisible(ccd->classrules,true);
	else
	    GDrawSetVisible(ccd->coverage,true);
    }

    GDrawSetVisible(gw,true);
    while ( !ccd->done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(ccd->gw);

    GTextInfoListFree(lookup_list);
    seqlookup_ci[1].enum_vals = NULL;

    /* ccd is freed by the event handler when we get a et_destroy event */

return;
}
