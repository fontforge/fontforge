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
    GWindow formats, coverage, glyphs, classes;
    enum activewindow { aw_formats, aw_coverage, aw_glyphs,
	    aw_classes } aw;
/* Wizard panes:
  formats    -- gives user a choice between by glyph/class/coverage table
  glyphs     -- user picked glyph format, shows a matrix edit of glyph lists
		  and lookups.
  classes    -- user picked class format. shows a matrix edit of class lists
		  with lookups. Also the three sets of classes.
  coverage   -- user picked coverage table format. shows a matrix edit with
		  a bunch of coverage tables and lookups/replacements.
		  [coverage format only accepts 1 rule]
*/
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

#define CID_GList	2000
#define CID_GNewSection	2001
#define CID_GAddLookup	2002

#define CID_CList	(CID_GList+100)
#define CID_CNewSection	(CID_GNewSection+100)
#define CID_CAddLookup	(CID_GAddLookup+100)

#define CID_MatchClasses	(CID_GList+300+0*20)
#define CID_BackClasses		(CID_MatchClasses+1*20)
#define CID_ForeClasses		(CID_MatchClasses+2*20)
#define CID_SameAsClasses	(CID_MatchClasses+1)		/* Not actually used itself */
#define CID_BackClassesSameAs	(CID_SameAsClasses+1*20)
#define CID_ForeClassesSameAs	(CID_SameAsClasses+2*20)

#define CID_ClassMatchType	2500

#define CID_Covers	2600

char *cu_copybetween(const unichar_t *start, const unichar_t *end) {
    char *ret = galloc(end-start+1);
    cu_strncpy(ret,start,end-start);
    ret[end-start] = '\0';
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

    rpt = ret = galloc(strlen(src)+found_cnt*(strlen(rpl)-flen)+1);
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

static void CCD_EnableNextPrev(struct contextchaindlg *ccd) {
    /* Cancel is always enabled */
    switch ( ccd->aw ) {
      case aw_formats:
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_Prev),false);
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_Next),true);
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_OK),false);
      break;
      case aw_glyphs:
      case aw_classes:
      case aw_coverage:
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_Prev),ccd->fpst->type!=pst_reversesub);
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_Next),false);
	GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_OK),true);
      break;
      default:
	IError("Can't get here");
      break;
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

static int CCD_SameAsClasses(GGadget *g, GEvent *e) {
    int ison = GGadgetIsChecked(g);
    int off = GGadgetGetCid(g)-CID_SameAsClasses;
    struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));

    GGadgetSetEnabled(GWidgetGetControl(ccd->gw,CID_CList+off),!ison);
return( true );
}

static void CCD_Close(struct contextchaindlg *ccd) {
    if ( ccd->isnew )
	GFI_FinishContextNew(ccd->gfi,ccd->fpst,false);
    ccd->done = true;
}

static void _CCD_Ok(struct contextchaindlg *ccd) {
    FPST *fpst = ccd->fpst, *dummyfpst;
    int len, i, k, had_class0;
    struct matrix_data *old, *classes;
    struct fpst_rule dummy;
    char *msg;
    int ans, cnt, cols, first, last, forward_start;
    int iswarning;
    char *buts[3];
    buts[0] = _("_Yes"); buts[1] = _("_No"); buts[2] = NULL;

    switch ( ccd->aw ) {
      case aw_classes:
      case aw_glyphs: {
	old = GMatrixEditGet(GWidgetGetControl(ccd->gw,ccd->aw==aw_glyphs?CID_GList:CID_CList),&len);
	if ( len==0 ) {
	    ff_post_error(_("Missing rules"),_(" There must be at least one contextual rule"));
return;
	}
	dummyfpst = chunkalloc(sizeof(FPST));
	*dummyfpst = *fpst;
	dummyfpst->format = pst_glyphs;
	if ( ccd->aw==aw_classes ) {
	    dummyfpst->format = pst_class;
	    for ( i=0; i<3; ++i ) { int clen;
		if ( i==0 || !GGadgetIsChecked(GWidgetGetControl(ccd->gw,CID_SameAsClasses+i*20)) )
		    classes = GMatrixEditGet(GWidgetGetControl(ccd->gw,CID_MatchClasses+i*20),&clen);
		else	/* If SameAs the reparse the match classes */
		    classes = GMatrixEditGet(GWidgetGetControl(ccd->gw,CID_MatchClasses),&clen);
		(&dummyfpst->nccnt)[i] = clen;
		(&dummyfpst->nclass)[i] = galloc(clen*sizeof(char*));
		(&dummyfpst->nclassnames)[i] = galloc(clen*sizeof(char*));
		(&dummyfpst->nclass)[i][0] = NULL;
		(&dummyfpst->nclassnames)[i][0] = NULL;
		had_class0 = i==0 && !isEverythingElse(classes[3*0+1].u.md_str);
		if ( !had_class0 )
		    (&dummyfpst->nclassnames)[i][0] = copy(classes[3*0+0].u.md_str);
		for ( k=had_class0 ? 0 : 1 ; k<clen; ++k ) {
		    (&dummyfpst->nclassnames)[i][k] = copy(classes[3*k+0].u.md_str);
		    (&dummyfpst->nclass)[i][k] = GlyphNameListDeUnicode(classes[3*k+1].u.md_str);
		}
	    }
	} else {
	    dummyfpst->nccnt = dummyfpst->bccnt = dummyfpst->fccnt = 0;
	    dummyfpst->nclass = dummyfpst->bclass = dummyfpst->fclass = NULL;
	    dummyfpst->nclassnames = dummyfpst->bclassnames = dummyfpst->fclassnames = NULL;
	}
	dummyfpst->rule_cnt = len;
	dummyfpst->rules = gcalloc(len,sizeof(struct fpst_rule));
	for ( i=0; i<len; ++i ) {
	    msg = FPSTRule_From_Str(ccd->sf,dummyfpst,&dummyfpst->rules[i],old[i].u.md_str,&iswarning);
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
      case aw_coverage:
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
		if ( old[cols*i+1].u.md_str!=NULL ) {
		    if ( first==-1 )
			first = last = i;
		    else {
			ff_post_error(_("Bad Coverage Table"),_("In a Reverse Chaining Substitution there must be exactly one coverage table with replacements"));
return;
		    }
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
		dummy.lookups = gcalloc(cnt,sizeof(struct seqlookup));
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
	dummy.u.coverage.ncovers = galloc(dummy.u.coverage.ncnt*sizeof(char *));
	if ( dummy.u.coverage.bcnt!=0 )
	    dummy.u.coverage.bcovers = galloc(dummy.u.coverage.bcnt*sizeof(char *));
	if ( dummy.u.coverage.fcnt!=0 )
	    dummy.u.coverage.fcovers = galloc(dummy.u.coverage.fcnt*sizeof(char *));
	for ( i=0; i<first; ++i )
	    dummy.u.coverage.bcovers[first-1-i] = copy( old[cols*i+0].u.md_str );
	for ( i=0; i<dummy.u.coverage.ncnt; ++i )
	    dummy.u.coverage.ncovers[i] = copy( old[cols*(first+1)+0].u.md_str );
	for ( i=forward_start; i<len; ++i )
	    dummy.u.coverage.fcovers[i-forward_start] = copy( old[cols*i+0].u.md_str );
	FPSTClassesFree(fpst);
	FPSTRulesFree(fpst->rules,fpst->format,fpst->rule_cnt);
	fpst->format = fpst->type==pst_reversesub ? pst_reversecoverage : pst_coverage;
	fpst->rule_cnt = 1;
	fpst->rules = gcalloc(1,sizeof(struct fpst_rule));
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
	    if ( GGadgetIsChecked(GWidgetGetControl(ccd->gw,CID_ByGlyph)) ) {
		ccd->aw = aw_glyphs;
		GDrawSetVisible(ccd->glyphs,true);
	    } else if ( GGadgetIsChecked(GWidgetGetControl(ccd->gw,CID_ByClass)) ) {
		ccd->aw = aw_classes;
		GDrawSetVisible(ccd->classes,true);
	    } else {
		ccd->aw = aw_coverage;
		GDrawSetVisible(ccd->coverage,true);
	    }
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
	  case aw_coverage:
	  case aw_classes:
	  case aw_glyphs:
	    ccd->aw = aw_formats;
	    GDrawSetVisible(ccd->coverage,false);
	    GDrawSetVisible(ccd->classes,false);
	    GDrawSetVisible(ccd->glyphs,false);
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
    if ( ccd->aw==aw_formats ) {
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
	GGadget *gme = GWidgetGetControl(GGadgetGetWindow(g),CID_GList+off);
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
	GGadget *gme = GWidgetGetControl(GGadgetGetWindow(g),CID_GList+off);
	GGadget *tf = _GMatrixEditGetActiveTextField(gme);
	GTextInfo *lookup_ti = GGadgetGetListItemSelected(g);
	OTLookup *otl;

	if ( tf!=NULL && lookup_ti!=NULL && lookup_ti->userdata!=NULL) {
	    char *space;
	    unichar_t *temp;
	    otl = lookup_ti->userdata;
	    space = galloc(strlen(otl->lookup_name)+8);
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
	GDrawResize(ccd->glyphs,wsize.width,wsize.height);
	GDrawResize(ccd->classes,wsize.width,wsize.height);

	GDrawRequestExpose(ccd->gw,NULL,false);
	GDrawRequestExpose(ccd->aw==aw_formats ? ccd->formats :
			    ccd->aw==aw_coverage ? ccd->coverage :
			    ccd->aw==aw_classes ? ccd->classes :
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

static unichar_t **CCD_ClassListCompletion(GGadget *t,int from_tab) {
    struct contextchaindlg *ccd = GDrawGetUserData(GDrawGetParentWindow(GGadgetGetWindow(t)));
    unichar_t *pt, *spt, *basept, *wild; unichar_t **ret;
    int i, cnt, doit, match_len;
    int do_wildcards;
    int section;
    int namecnt;
    int cid;
    struct matrix_data *classnames;

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
    if ( section==0 && !GGadgetIsChecked(GWidgetGetControl(ccd->gw,CID_BackClassesSameAs)) )
	cid = CID_BackClasses;
    else if ( section==2 && !GGadgetIsChecked(GWidgetGetControl(ccd->gw,CID_ForeClassesSameAs)) )
	cid = CID_ForeClasses;
    else
	cid = CID_MatchClasses;
    classnames = GMatrixEditGet(GWidgetGetControl(ccd->gw,cid),&namecnt);
    if ( namecnt==0 )		/* No classes specified yet, can't help */
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
	wild = galloc((u_strlen(spt)+2)*sizeof(unichar_t));
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
			unichar_t *temp = galloc((spt-basept+strlen(str)+4)*sizeof(unichar_t));
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
	    ret = galloc((cnt+1)*sizeof(unichar_t *));
    }
    free(wild);
return( ret );
}

static void CCD_FinishCoverageEdit(GGadget *g,int r, int c, int wasnew) {
    struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));
    int cols = GMatrixEditGetColCnt(g);
    int first, last, i;
    int rows;
    struct matrix_data *covers = _GMatrixEditGet(g,&rows);


    if ( c==0 )
	ME_SetCheckUnique(g, r, c, ccd->sf);
    else if ( c==1 && cols==2 ) {	/* The replacement list in a reverse coverage */
	ME_ListCheck(g, r, c, ccd->sf);
    } else if ( c==1 && cols==4 ) {	/* The lookup in a coverage line (not reverse coverage) */
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
	if ( GGadgetIsChecked(GWidgetGetControl(ccd->classes,CID_SameAsClasses+20)))
	    sections |= 0x1;
	if ( GGadgetIsChecked(GWidgetGetControl(ccd->classes,CID_SameAsClasses+40)))
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
    GGadget *gclassrules = GWidgetGetControl(ccd->classes,CID_CList);
    struct matrix_data *classrules = GMatrixEditGet(gclassrules,&rows);
    int cols = GMatrixEditGetColCnt(gclassrules);
    char *end_back=NULL, *end_match=NULL, *pt, *last_name, *temp;
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
	struct matrix_data *classes = _GMatrixEditGet(g,&rows);
	char buffer[40], *end, *pt;

	if ( strchr(classes[3*r+0].u.md_str,' ')!=NULL ) {
	    for ( pt=classes[3*r+0].u.md_str; *pt; ++pt) {
		if ( isspace(*pt))
		    *pt = '_';
	    }
	    GGadgetRedraw(g);
	    ff_post_error(_("Bad class name"),_("No spaces allowed in class names."));
	}
	i = strtol(classes[3*r+0].u.md_str,&end,10);
	if ( *end=='\0' && i!=r ) {
	    sprintf( buffer,"%d",r );
	    free( classes[3*r+0].u.md_str );
	    classes[3*r+0].u.md_str = copy(buffer);
	    GGadgetRedraw(g);
	    ff_post_error(_("Bad class name"),_("If a class name is a number, it must be the index of the class in the array of classes."));
	}
	for ( i=0; i<rows; ++i ) if ( i!=r ) {
	    if ( strcmp(classes[3*i+0].u.md_str,classes[3*r+0].u.md_str) == 0 ) {
		sprintf( buffer,"%d",r );
		free( classes[3*r+0].u.md_str );
		classes[3*r+0].u.md_str = copy(buffer);
		GGadgetRedraw(g);
		ff_post_error(_("Bad class name"),_("The class name, %s, is already in use."), classes[3*i+0].u.md_str);
	    }
	}
	if ( strcmp(classes[3*r+0].u.md_str,classes[3*r+2].u.md_str)!=0 ) {
	    RenameClass(ccd,classes[3*r+2].u.md_str,classes[3*r+0].u.md_str,WhichSections(ccd,g));
	    free(classes[3*r+2].u.md_str);
	    classes[3*r+2].u.md_str = copy(classes[3*r+0].u.md_str);
	}
    }
}

static void CCD_ClassGoing(GGadget *g,int r) {
    /* We are about to delete this row in the class dlg, so fix up all numeric */
    /*  classes after it */
    struct contextchaindlg *ccd = GDrawGetUserData(GGadgetGetWindow(g));
    int sections = WhichSections(ccd,g);
    int rows, i;
    struct matrix_data *classes = _GMatrixEditGet(g,&rows);
    char buffer[20], *end;

    for ( i=r+1; i<rows; ++i ) {
	strtol(classes[3*i+0].u.md_str,&end,10);
	if ( *end=='\0' ) {
	    sprintf(buffer,"%d",i-1);
	    free(classes[3*i+0].u.md_str);
	    classes[3*i+0].u.md_str = copy(buffer);
	    RenameClass(ccd,classes[3*i+2].u.md_str,buffer,sections);
	    free(classes[3*i+2].u.md_str);
	    classes[3*i+2].u.md_str = copy(buffer);
	}
    }
}

static void CCD_InitClassRow(GGadget *g,int r) {
    char buffer[20];
    int rows;
    struct matrix_data *classes = _GMatrixEditGet(g,&rows);

    sprintf( buffer, "%d", r );
    classes[3*r+0].u.md_str = copy(buffer);
}

static int CCD_EnableDeleteClass(GGadget *g,int whichclass) {
return( whichclass>0 );
}

static GTextInfo section[] = {
    { (unichar_t *) N_("Section|Continue"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 1, 0, 1},
    { (unichar_t *) N_("Section|Start"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static struct col_init class_ci[] = {
    { me_string, NULL, NULL, NULL, N_("Class|Name") },
    { me_funcedit, CCD_PickGlyphsForClass, NULL, NULL, N_("Glyphs in the class") },
    { me_string, NULL, NULL, NULL, "Old Name (hidden)" },
    };
static struct col_init coverage_ci[] = {
    { me_funcedit, CCD_PickGlyphsForClass, NULL, NULL, N_("Glyphs in the coverage tables") },
    { me_enum, NULL, NULL, NULL, N_("Apply lookup") },
    { me_enum, NULL, section, NULL, N_("Section")},
    { me_addr, NULL, section, NULL, "Old lookup, internal use only"},
    };
static struct col_init rcoverage_ci[] = {
    { me_funcedit, CCD_PickGlyphsForClass, NULL, NULL, N_("Glyphs in the coverage tables") },
    { me_funcedit, CCD_PickGlyphsForClass, NULL, NULL, N_("Replacement glyphs") },
    };
static struct col_init glyph_ci[] = {
    { me_string, NULL, NULL, NULL, N_("Matching rules based on a list glyphs") },
    };
static struct col_init classrules_ci[] = {
    { me_string, NULL, NULL, NULL, N_("Matching rules based on a list of classes") },
    };

/* This routine has several problems:
  1. The order in which lookups are to be applied is lost, they will be
      applied in sequence order
  2. In coverage classes only one lookup may be applied for any given
      coverage set.
  Adobe's feature files have these restrictions and more, so my hope is that
  they will not matter.
*/
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
    GTabInfo faspects[5];
    GWindowAttrs wattrs;
    GGadgetCreateData glgcd[8], fgcd[3], cgcd[3][18],
	clgcd[10], subboxes[3][5], topbox[3], classbox[2], extrabuttonsgcd[3];
    GTextInfo gllabel[8], flabel[3], clabel[3][18],
	cllabel[10],extrabuttonslab[3];
    GGadgetCreateData bgcd[6], rgcd[15], boxes[3], *barray[10], *hvarray[4][2];
    GGadgetCreateData *subvarray[3][10], *topvarray[8],
	    *classv[4];
    GTextInfo blabel[5], rlabel[15];
    FPST *tempfpst;
    struct matrixinit coverage_mi, classrules_mi, glyphs_mi, class_mi[3];
    static int inited=false;
    int isgpos = ( fpst->type==pst_contextpos || fpst->type==pst_chainpos );
    struct matrix_data *md;
    GTextInfo *lookup_list, *addlookup_list, *addrmlookup_list;

    lookup_list = SFLookupArrayFromType(sf,isgpos?gpos_start:gsub_start);
    for ( i=0; lookup_list[i].text!=NULL; ++i ) {
	if (fpst!=NULL && lookup_list[i].userdata == fpst->subtable->lookup )
	    lookup_list[i].disabled = true;
	lookup_list[i].selected = false;
    }
    addlookup_list = gcalloc(i+3,sizeof(GTextInfo));
    memcpy(addlookup_list+1,lookup_list,(i+1)*sizeof(GTextInfo));
    addrmlookup_list = gcalloc(i+3,sizeof(GTextInfo));
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
    coverage_ci[1].enum_vals = addrmlookup_list;

    if ( !inited ) {
	inited = true;
	section[0].text = (unichar_t *) S_( (char *) section[0].text);
	section[1].text = (unichar_t *) S_( (char *) section[1].text);
	coverage_ci[0].title = S_(coverage_ci[0].title);
	coverage_ci[1].title = S_(coverage_ci[1].title);
	coverage_ci[2].title = S_(coverage_ci[2].title);
	rcoverage_ci[0].title = S_(rcoverage_ci[0].title);
	rcoverage_ci[1].title = S_(rcoverage_ci[1].title);
	glyph_ci[0].title = S_(glyph_ci[0].title);
	class_ci[0].title = S_(class_ci[0].title);
	class_ci[1].title = S_(class_ci[1].title);
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
	ccd->aw = aw_classes;
    } else {
	bgcd[2].gd.flags = gg_visible | gg_enabled;
	bgcd[3].gd.flags = gg_visible;
	ccd->aw = aw_glyphs;
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
    ccd->glyphs = GWidgetCreateSubWindow(ccd->gw,&subpos,subccd_e_h,ccd,&wattrs);
    memset(&gllabel,0,sizeof(gllabel));
    memset(&glgcd,0,sizeof(glgcd));

    k = 0;
    memset(&glyphs_mi,0,sizeof(glyphs_mi));
    glyphs_mi.col_cnt = 1;
    glyphs_mi.col_init = glyph_ci;
    if ( fpst->format==pst_glyphs && fpst->rule_cnt>0 ) {
	glyphs_mi.initial_row_cnt = fpst->rule_cnt;
	md = gcalloc(fpst->rule_cnt,sizeof(struct matrix_data));
	for ( j=0; j<fpst->rule_cnt; ++j ) {
	    md[j+0].u.md_str = FPSTRule_To_Str(sf,fpst,&fpst->rules[j]);;
	}
	glyphs_mi.matrix_data = md;
    } else {
	glyphs_mi.initial_row_cnt = 0;
	glyphs_mi.matrix_data = NULL;
    }
    glgcd[k].gd.flags = gg_visible | gg_enabled;
    glgcd[k].gd.cid = CID_GList;
    glgcd[k].gd.u.matrix = &glyphs_mi;
    glgcd[k++].creator = GMatrixEditCreate;
    /* No need for a box, this widget fills the pane */

    GGadgetsCreate(ccd->glyphs,glgcd);
    GMatrixEditSetUpDownVisible(GWidgetGetControl(ccd->glyphs,CID_GList),
	    true);
    GMatrixEditSetColumnCompletion(GWidgetGetControl(ccd->glyphs,CID_GList),0,
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
	GDrawSetFont(ccd->glyphs,_ggadget_default_font);
    }
    extrabuttonsgcd[i].gd.pos.width = GDrawPixelsToPoints(ccd->glyphs,
	    GDrawGetText8Width(ccd->glyphs,(char *)extrabuttonslab[i].text,-1,NULL))+50;
    extrabuttonsgcd[i].gd.cid = CID_GAddLookup;
    extrabuttonsgcd[i].gd.handle_controlevent = CCD_AddLookup;
    extrabuttonsgcd[i++].creator = GListButtonCreate;
    GMatrixEditAddButtons(GWidgetGetControl(ccd->glyphs,CID_GList),extrabuttonsgcd);
    

	/* Pane showing the single rule in a coverage table */
    ccd->coverage = GWidgetCreateSubWindow(ccd->gw,&subpos,subccd_e_h,ccd,&wattrs);
    memset(&clabel,0,sizeof(clabel));
    memset(&cgcd,0,sizeof(cgcd));
    memset(&subboxes,0,sizeof(subboxes));
    memset(&flabel,0,sizeof(flabel));
    memset(&fgcd,0,sizeof(fgcd));
    memset(&topbox,0,sizeof(topbox));

    memset(&coverage_mi,0,sizeof(coverage_mi));
    coverage_mi.col_cnt = 4;
    coverage_mi.col_init = coverage_ci;
    if ( fpst->format==pst_reversecoverage ) {
	coverage_mi.col_cnt = 2;
	coverage_mi.col_init = rcoverage_ci;
    }
    if (( fpst->format==pst_coverage || fpst->format==pst_reversecoverage )
	    && fpst->rules!=NULL ) {
	int cnt = fpst->rules[0].u.coverage.ncnt+fpst->rules[0].u.coverage.bcnt+fpst->rules[0].u.coverage.fcnt;
	coverage_mi.initial_row_cnt = cnt;
	md = gcalloc(cnt*coverage_mi.col_cnt,sizeof(struct matrix_data));
	for ( i=0,j=fpst->rules[0].u.coverage.bcnt-1; j>=0; --j, ++i )
	    md[i*coverage_mi.col_cnt].u.md_str = SFNameList2NameUni(sf,fpst->rules[0].u.coverage.bcovers[j]);
	for ( j=0; j<fpst->rules[0].u.coverage.ncnt; ++j, ++i ) {
	    md[coverage_mi.col_cnt*i+0].u.md_str = SFNameList2NameUni(sf,fpst->rules[0].u.coverage.ncovers[j]);
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
	    md[coverage_mi.col_cnt*i].u.md_str = SFNameList2NameUni(sf,fpst->rules[0].u.coverage.fcovers[j]);
	    if ( fpst->format==pst_coverage )
		md[4*i+2].u.md_ival = j==0;
	}
	coverage_mi.matrix_data = md;
    } else {
	coverage_mi.initial_row_cnt = 0;
	coverage_mi.matrix_data = NULL;
    }
    coverage_mi.finishedit = CCD_FinishCoverageEdit;

    flabel[0].text = (unichar_t *) _("A list of coverage tables:");
    flabel[0].text_is_1byte = true;
    fgcd[0].gd.label = &flabel[0];
    fgcd[0].gd.flags = gg_visible | gg_enabled;
    fgcd[0].creator = GLabelCreate;
    topvarray[0] = &fgcd[0];

    fgcd[1].gd.flags = gg_visible | gg_enabled;
    fgcd[1].gd.cid = CID_Covers;
    fgcd[1].gd.u.matrix = &coverage_mi;
    fgcd[1].creator = GMatrixEditCreate;
    topvarray[1] = &fgcd[1];
    topvarray[2] = NULL;
    topbox[0].gd.flags = gg_enabled|gg_visible;
    topbox[0].gd.u.boxelements = topvarray;
    topbox[0].creator = GVBoxCreate;
    GGadgetsCreate(ccd->coverage,topbox);
    GMatrixEditSetUpDownVisible(GWidgetGetControl(ccd->coverage,CID_Covers),
	    true);
    GMatrixEditSetColumnCompletion(GWidgetGetControl(ccd->coverage,CID_Covers),0,
	    CCD_GlyphListCompletion);
    if ( fpst->type==pst_reversesub ) {
	GMatrixEditSetColumnCompletion(GWidgetGetControl(ccd->coverage,CID_Covers),1,
		CCD_GlyphListCompletion);
    }
    GHVBoxSetExpandableRow(topbox[0].ret,1);
    if ( fpst->type == pst_contextpos || fpst->type == pst_contextsub )
	GMatrixEditShowColumn(GWidgetGetControl(ccd->coverage,CID_Covers),2,false);
    if ( fpst->format!=pst_reversecoverage )
	GMatrixEditShowColumn(GWidgetGetControl(ccd->coverage,CID_Covers),3,false);

	/* This pane displays a set of class rules, and below the three sets of classes for those rules */
    ccd->classes = GWidgetCreateSubWindow(ccd->gw,&subpos,subccd_e_h,ccd,&wattrs);
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

    memset(&classrules_mi,0,sizeof(classrules_mi));
    classrules_mi.col_cnt = 1;
    classrules_mi.col_init = classrules_ci;
    if ( tempfpst->format==pst_class && tempfpst->rule_cnt>0 ) {
	classrules_mi.initial_row_cnt = tempfpst->rule_cnt;
	md = gcalloc(tempfpst->rule_cnt,sizeof(struct matrix_data));
	for ( j=0; j<tempfpst->rule_cnt; ++j ) {
	    md[j+0].u.md_str = FPSTRule_To_Str(sf,tempfpst,&tempfpst->rules[j]);;
	}
	classrules_mi.matrix_data = md;
    } else {
	classrules_mi.initial_row_cnt = 0;
	classrules_mi.matrix_data = NULL;
    }
    clgcd[k].gd.flags = gg_visible | gg_enabled;
    clgcd[k].gd.cid = CID_CList;
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
	if ( tempfpst->format==pst_class ) {
	    char **classes = (&tempfpst->nclass)[i];
	    char **classnames = (&tempfpst->nclassnames)[i];
	    if ( classes==NULL )
		cc=1;
	    else
		cc = (&tempfpst->nccnt)[i];
	    class_mi[i].initial_row_cnt = cc;
	    md = gcalloc(3*cc,sizeof(struct matrix_data));
/* GT: This is the default class name for the class containing any glyphs */
/* GT: which aren't specified in other classes. The class name may NOT */
/* GT: contain spaces. Use an underscore or something similar instead */
	    md[0+0].u.md_str = copy(classnames==NULL || classnames[0]==NULL?S_("Glyphs|All_Others"):classnames[0]); 
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
	} else {
	    class_mi[i].initial_row_cnt = 0;
	    class_mi[i].matrix_data = NULL;
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
    GGadgetsCreate(ccd->classes,topbox);

    /* Top box should give it's size equally to its two components */
    GHVBoxSetExpandableRow(classbox[0].ret,0);
    GHVBoxSetExpandableRow(subboxes[0][0].ret,0);
    GHVBoxSetExpandableRow(subboxes[1][0].ret,1);
    GHVBoxSetExpandableRow(subboxes[2][0].ret,1);
    GMatrixEditSetColumnCompletion(GWidgetGetControl(ccd->classes,CID_MatchClasses),1,CCD_GlyphListCompletion);
    GMatrixEditSetColumnCompletion(GWidgetGetControl(ccd->classes,CID_BackClasses),1,CCD_GlyphListCompletion);
    GMatrixEditSetColumnCompletion(GWidgetGetControl(ccd->classes,CID_ForeClasses),1,CCD_GlyphListCompletion);
    GMatrixEditSetColumnCompletion(GWidgetGetControl(ccd->classes,CID_CList),0,
	    CCD_ClassListCompletion);
    if ( fpst->type==pst_chainpos || fpst->type==pst_chainsub ) {
	extrabuttonsgcd[0].gd.cid = CID_CNewSection;
	extrabuttonsgcd[1].gd.cid = CID_CAddLookup;
    } else
	extrabuttonsgcd[0].gd.cid = CID_CAddLookup;
    GMatrixEditAddButtons(GWidgetGetControl(ccd->classes,CID_CList),extrabuttonsgcd);
    for ( i=0; i<3; ++i ) {
	GMatrixEditShowColumn(GWidgetGetControl(ccd->classes,CID_MatchClasses+i*20),2,false);
	GMatrixEditSetBeforeDelete(GWidgetGetControl(ccd->classes,CID_MatchClasses+i*20),CCD_ClassGoing);
    }
    if ( tempfpst!=fpst )
	FPSTFree(tempfpst);


    if ( ccd->aw == aw_formats )
	GDrawSetVisible(ccd->formats,true);
    else if ( ccd->aw == aw_glyphs )
	GDrawSetVisible(ccd->glyphs,true);
    else if ( ccd->aw == aw_classes )
	GDrawSetVisible(ccd->classes,true);
    else
	GDrawSetVisible(ccd->coverage,true);

    GDrawSetVisible(gw,true);
    while ( !ccd->done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(ccd->gw);

    free(addlookup_list);	/* This doesn't need its contents freed, it's a shallow copy of lookup_list */
    free(addrmlookup_list);	/* ditto */
    GTextInfoListFree(lookup_list);

    /* ccd is freed by the event handler when we get a et_destroy event */

return;
}
