/* Copyright (C) 2000-2012 by George Williams */
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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "ustring.h"
#include <ffglib.h>
#include <glib/gprintf.h>
#include "gdraw.h"
#include "gwidget.h"
#include "ggadget.h"
#include "ggadgetP.h"

static GWindow last;
static const char *last_title;

struct dlg_info {
    int done;
    int ret;
    int multi;
    int exposed;
    int size_diff;
    int bcnt;
};

static int d_e_h(GWindow gw, GEvent *event) {
    struct dlg_info *d = GDrawGetUserData(gw);

    if ( event->type==et_close ) {
	d->done = true;
	/* d->ret is initialized to cancel */
    } else if ( event->type==et_controlevent &&
	    event->u.control.subtype == et_buttonactivate ) {
	d->done = true;
	d->ret = GGadgetGetCid(event->u.control.g);
    } else if ( event->type == et_expose ) {
	d->exposed = true;
    } else if ( event->type == et_char ) {
return( false );
    } else if ( event->type == et_resize && !d->exposed ) {
	GRect pos,rootsize;
	GDrawGetSize(gw,&pos);
	GDrawGetSize(GDrawGetRoot(NULL),&rootsize);
	if ( pos.x+pos.width>=rootsize.width || pos.y+pos.height>=rootsize.height ) {
	    if ( pos.x+pos.width>=rootsize.width )
		if (( pos.x = rootsize.width - pos.width )<0 ) pos.x = 0;
	    if ( pos.y+pos.height>=rootsize.height )
		if (( pos.y = rootsize.height - pos.height )<0 ) pos.y = 0;
	    GDrawMove(gw,pos.x,pos.y);
	}
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    }
return( true );
}

static int w_e_h(GWindow gw, GEvent *event) {

    if ( event->type==et_close ||
	    (event->type==et_controlevent &&
	     event->u.control.subtype == et_buttonactivate ) ||
	    event->type==et_timer )
	GDrawDestroyWindow(gw);
    else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    } else if ( event->type == et_char ) {
return( false );
    } else if ( event->type == et_destroy ) {
	if ( last==gw ) {
	    last = NULL;
	    last_title = NULL;
	}
    }
return( true );
}

#define GLINE_MAX	20

static int FindLineBreaks(const unichar_t *question, GTextInfo linebreaks[GLINE_MAX+1]) {
    int lb, i;
    const unichar_t *pt, *last;

    /* Find good places to break the question into bits */
    last = pt = question;
    linebreaks[0].text = (unichar_t *) question;
    lb=0;
    while ( *pt!='\0' && lb<GLINE_MAX-1 ) {
	last = pt;
	/* break lines around 60 characters (or if there are no spaces at 90 chars) */
	while ( pt-linebreaks[lb].text<60 ||
		(pt-linebreaks[lb].text<90 && last==linebreaks[lb].text)) {
	    if ( *pt=='\n' || *pt=='\0' ) {
		last = pt;
	break;
	    }
	    if ( *pt==' ' )
		last = pt;
	    ++pt;
	}
	if ( last==linebreaks[lb].text )
	    last = pt;
	if ( *last==' ' || *last=='\n' ) ++last;
	linebreaks[++lb].text = (unichar_t *) last;
	pt = last;
    }
    if ( *pt!='\0' )
	linebreaks[++lb].text = (unichar_t *) pt+u_strlen(pt);
    for ( i=0; i<lb; ++i ) {
	int temp = linebreaks[i+1].text - linebreaks[i].text;
	if ( linebreaks[i+1].text[-1]==' ' || linebreaks[i+1].text[-1]=='\n' )
	    --temp;
	linebreaks[i].text = u_copyn(linebreaks[i].text,temp);
    }

    if ( question[u_strlen(question)-1]=='\n' )
	--lb;
return( lb );
}

static GWindow DlgCreate(const unichar_t *title,const unichar_t *question,va_list ap,
	const unichar_t **answers, const unichar_t *mn, int def, int cancel,
	struct dlg_info *d, int add_text, int restrict_input, int center) {
    GTextInfo qlabels[GLINE_MAX+1], *blabels;
    GGadgetCreateData *gcd;
    int lb, bcnt=0;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    extern FontInstance *_ggadget_default_font;
    int as, ds, ld, fh;
    int w, maxw, bw, bspace;
    int i, y;
    gchar *buf, *qbuf;
    unichar_t *ubuf;
    extern GBox _GGadget_defaultbutton_box;

    if ( d!=NULL )
	memset(d,0,sizeof(*d));
    GGadgetInit();
    /* u_vsnprintf(ubuf,sizeof(ubuf)/sizeof(ubuf[0]),question,ap);*/
    /* u_vsnprintf() also assumes UCS4 string is null terminated */
    qbuf = g_ucs4_to_utf8( (const gunichar *)question, -1, NULL, NULL, NULL );
    if( !qbuf ) {
	fprintf( stderr, "Failed to convert question string in DlgCreate()\n" );
	return( NULL );
    }
    g_vasprintf(&buf, qbuf, ap);
    g_free( qbuf );
    ubuf = (unichar_t *) g_utf8_to_ucs4( buf, -1, NULL, NULL, NULL );
    g_free( buf );
    if( !ubuf ) {
	fprintf( stderr, "Failed to convert formatted string in DlgCreate()\n" );
	return( NULL );
    }
    if ( screen_display==NULL ) {
	char *temp = u2def_copy(ubuf);
	fprintf(stderr, "%s\n", temp);
	free(temp);
	if ( d!=NULL ) d->done = true;
return( NULL );
    }

    GProgressPauseTimer();
    memset(qlabels,'\0',sizeof(qlabels));
    lb = FindLineBreaks(ubuf,qlabels);
    for ( bcnt=0; answers[bcnt]!=NULL; ++bcnt);
    blabels = calloc(bcnt+1,sizeof(GTextInfo));
    for ( bcnt=0; answers[bcnt]!=NULL; ++bcnt)
	blabels[bcnt].text = (unichar_t *) answers[bcnt];

    memset(&wattrs,0,sizeof(wattrs));
    /* If we have many questions in quick succession the dlg will jump around*/
    /*  as it tracks the cursor (which moves to the buttons). That's not good*/
    /*  So I don't do undercursor here */
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_isdlg;
    if ( restrict_input )
	wattrs.mask |= wam_restrict;
    else 
	wattrs.mask |= wam_notrestricted;
    if ( center )
	wattrs.mask |= wam_centered;
    else
	wattrs.mask |= wam_undercursor;
    wattrs.is_dlg = true;
    wattrs.not_restricted = true;
    wattrs.restrict_input_to_me = 1;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.undercursor = 1;
    wattrs.centered = 2;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = (unichar_t *) title;
    pos.x = pos.y = 0;
    pos.width = 200; pos.height = 60;		/* We'll figure size later */
		/* but if we get the size too small, the cursor isn't in dlg */
    gw = GDrawCreateTopWindow(NULL,&pos,restrict_input?d_e_h:w_e_h,d,&wattrs);

    GGadgetInit();
    GDrawSetFont(gw,_ggadget_default_font);
    GDrawWindowFontMetrics(gw,_ggadget_default_font,&as,&ds,&ld);
    fh = as+ds;
    maxw = 0;
    for ( i=0; i<lb; ++i ) {
	w = GDrawGetTextWidth(gw,qlabels[i].text,-1);
	if ( w>maxw ) maxw = w;
    }
    bw = 0;
    for ( i=0; i<bcnt; ++i ) {
	w = GDrawGetTextWidth(gw,answers[i],-1);
	if ( w>bw ) bw = w;
    }
    bw += GDrawPointsToPixels(gw,20);
    bspace = GDrawPointsToPixels(gw,6);
    if ( (bw+bspace) * bcnt > maxw )
	maxw = (bw+bspace)*bcnt;
    if ( bcnt!=1 )
	bspace = (maxw-bcnt*bw)/(bcnt-1);
    maxw += GDrawPointsToPixels(gw,16);

    gcd = calloc(lb+bcnt+2,sizeof(GGadgetCreateData));
    if ( lb==1 ) {
	gcd[0].gd.pos.width = GDrawGetTextWidth(gw,qlabels[0].text,-1);
	gcd[0].gd.pos.x = (maxw-gcd[0].gd.pos.width)/2;
	gcd[0].gd.pos.y = GDrawPointsToPixels(gw,6);
	gcd[0].gd.pos.height = fh;
	gcd[0].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels | gg_pos_use0;
	gcd[0].gd.label = &qlabels[0];
	gcd[0].creator = GLabelCreate;
    } else for ( i=0; i<lb; ++i ) {
	gcd[i].gd.pos.x = GDrawPointsToPixels(gw,8);
	gcd[i].gd.pos.y = GDrawPointsToPixels(gw,6)+i*fh;
	gcd[i].gd.pos.width = GDrawGetTextWidth(gw,qlabels[i].text,-1);
	gcd[i].gd.pos.height = fh;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels | gg_pos_use0;
	gcd[i].gd.label = &qlabels[i];
	gcd[i].creator = GLabelCreate;
    }
    y = GDrawPointsToPixels(gw,12)+lb*fh;
    if ( add_text ) {
	gcd[bcnt+lb].gd.pos.x = GDrawPointsToPixels(gw,8);
	gcd[bcnt+lb].gd.pos.y = y;
	gcd[bcnt+lb].gd.pos.width = maxw-2*GDrawPointsToPixels(gw,6);
	gcd[bcnt+lb].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels | gg_pos_use0 | gg_text_xim;
	gcd[bcnt+lb].gd.cid = bcnt;
	gcd[bcnt+lb].creator = GTextFieldCreate;
	y += fh + GDrawPointsToPixels(gw,6) + GDrawPointsToPixels(gw,10);
    }
    y += GDrawPointsToPixels(gw,2);
    for ( i=0; i<bcnt; ++i ) {
	gcd[i+lb].gd.pos.x = GDrawPointsToPixels(gw,8) + i*(bw+bspace);
	gcd[i+lb].gd.pos.y = y;
	gcd[i+lb].gd.pos.width = bw;
	gcd[i+lb].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels | gg_pos_use0;
	if ( i==def ) {
	    gcd[i+lb].gd.flags |= gg_but_default;
	    if ( _GGadget_defaultbutton_box.flags&box_draw_default ) {
		gcd[i+lb].gd.pos.x -= GDrawPointsToPixels(gw,3);
		gcd[i+lb].gd.pos.y -= GDrawPointsToPixels(gw,3);
		gcd[i+lb].gd.pos.width += 2*GDrawPointsToPixels(gw,3);
	    }
	}
	if ( i==cancel )
	    gcd[i+lb].gd.flags |= gg_but_cancel;
	gcd[i+lb].gd.cid = i;
	gcd[i+lb].gd.label = &blabels[i];
	if ( mn!=NULL ) {
	    gcd[i+lb].gd.mnemonic = mn[i];
	    if ( mn[i]=='\0' ) mn = NULL;
	}
	gcd[i+lb].creator = GButtonCreate;
    }
    if ( bcnt==1 )
	gcd[lb].gd.pos.x = (maxw-bw)/2;
    GGadgetsCreate(gw,gcd);
    pos.width = maxw;
    pos.height = (lb+1)*fh + GDrawPointsToPixels(gw,34);
    if ( add_text )
	pos.height += fh + GDrawPointsToPixels(gw,16);
    GDrawResize(gw,pos.width,pos.height);
    GWidgetHidePalettes();
    if ( d!=NULL ) {
	d->ret  = cancel;
	d->bcnt = bcnt;
    }
    GDrawSetVisible(gw,true);
    free(blabels);
    free(gcd);
    for ( i=0; i<lb; ++i )
	free(qlabels[i].text);
    GProgressResumeTimer();
return( gw );
}

int GWidgetAsk(const unichar_t *title,
	const unichar_t **answers, const unichar_t *mn, int def, int cancel,
	const unichar_t *question,...) {
    struct dlg_info d;
    GWindow gw;
    va_list ap;

    if ( screen_display==NULL )
return( def );

    va_start(ap,question);
    gw = DlgCreate(title,question,ap,answers,mn,def,cancel,&d,false,true,false);
    va_end(ap);

    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
return(d.ret);
}

unichar_t *GWidgetAskStringR(int title,const unichar_t *def,int question,...) {
    struct dlg_info d;
    GWindow gw;
    unichar_t *ret = NULL;
    const unichar_t *ocb[3]; unichar_t ocmn[2];
    va_list ap;

    if ( screen_display==NULL )
return( u_copy(def ));

    ocb[2]=NULL;
    ocb[0] = GStringGetResource( _STR_OK, &ocmn[0]);
    ocb[1] = GStringGetResource( _STR_Cancel, &ocmn[1]);
    va_start(ap,question);
    gw = DlgCreate(GStringGetResource( title,NULL),GStringGetResource( question,NULL),ap,
	    ocb,ocmn,0,1,&d,true,true,false);
    va_end(ap);
    if ( def!=NULL && *def!='\0' )
	GGadgetSetTitle(GWidgetGetControl(gw,2),def);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    if ( d.ret==0 )
	ret = GGadgetGetTitle(GWidgetGetControl(gw,2));
    GDrawDestroyWindow(gw);
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
return(ret);
}

void GWidgetError(const unichar_t *title,const unichar_t *statement, ...) {
    struct dlg_info d;
    GWindow gw;
    const unichar_t *ob[2]; unichar_t omn[1];
    va_list ap;

    ob[1]=NULL;
    ob[0] = GStringGetResource( _STR_OK, &omn[0]);
    va_start(ap,statement);
    gw = DlgCreate(title,statement,ap,ob,omn,0,0,&d,false,true,true);
    va_end(ap);
    if ( gw!=NULL ) {
	while ( !d.done )
	    GDrawProcessOneEvent(NULL);
	GDrawDestroyWindow(gw);
    }
}

/* ************************************************************************** */

#define CID_Cancel	0
#define CID_OK		1
#define CID_List	2
#define CID_SelectAll	3
#define CID_SelectNone	4

static int GCD_Select(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	GGadget *list = GWidgetGetControl(gw,CID_List);

	GGadgetSelectListItem(list,-1,GGadgetGetCid(g)==CID_SelectAll?1:0 );
    }
return( true );
}

static int c_e_h(GWindow gw, GEvent *event) {
    struct dlg_info *d = GDrawGetUserData(gw);

    if ( event->type==et_close ) {
	d->done = true;
	d->ret = -1;
    } else if ( event->type==et_resize ) {
	GDrawRequestExpose(gw,NULL,false);
    } else if ( event->type==et_controlevent &&
	    (event->u.control.subtype == et_buttonactivate ||
	     event->u.control.subtype == et_listdoubleclick )) {
	d->done = true;
	if ( GGadgetGetCid(event->u.control.g)==CID_Cancel )
	    d->ret = -1;
	else
	    d->ret = GGadgetGetFirstListSelectedItem(GWidgetGetControl(gw,CID_List));
    } else if ( event->type == et_char ) {
return( false );
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    }
return( true );
}

/* ************** Parallel routines using utf8 arguments ******************** */

static GWindow DlgCreate8(const char *title,const char *question,va_list ap,
	const char **answers, int def, int cancel,
	struct dlg_info *d, int add_text, const char *defstr,
	int restrict_input, int center) {
    GTextInfo qlabels[GLINE_MAX+1], *blabels;
    GGadgetCreateData *gcd, *array[2*(GLINE_MAX+5)+1], boxes[5], **barray, *labarray[4];
    int lb, bcnt=0, l;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    extern FontInstance *_ggadget_default_font;
    int as, ds, ld, fh;
    int w, maxw, bw, bspace;
    int i, y;
    gchar *buf;
    unichar_t *ubuf;
    extern GBox _GGadget_defaultbutton_box;

    if ( d!=NULL )
	memset(d,0,sizeof(*d));
    /*vsnprintf(buf,sizeof(buf)/sizeof(buf[0]),question,ap);*/
    g_vasprintf( &buf, (const gchar *) question, ap );
    if ( screen_display==NULL ) {
	fprintf(stderr, "%s\n", buf );
	if ( d!=NULL ) d->done = true;
return( NULL );
    }
    /*ubuf = utf82u_copy(buf);*/
    ubuf = (unichar_t *) g_utf8_to_ucs4( (const gchar *) buf, -1, NULL, NULL, NULL);
    g_free( buf );
    if( !ubuf ) {
	fprintf( stderr, "Failed to convert question string in DlgCreate8()\n" );
	return( NULL );
    }

    GGadgetInit();
    GProgressPauseTimer();
    memset(qlabels,'\0',sizeof(qlabels));
    lb = FindLineBreaks(ubuf,qlabels);
    for ( bcnt=0; answers[bcnt]!=NULL; ++bcnt);
    blabels = calloc(bcnt+1,sizeof(GTextInfo));
    barray = calloc(2*bcnt+3,sizeof(GGadgetCreateData *));
    for ( bcnt=0; answers[bcnt]!=NULL; ++bcnt) {
	blabels[bcnt].text = (unichar_t *) answers[bcnt];
	blabels[bcnt].text_is_1byte = true;
	blabels[bcnt].text_in_resource = true;	/* Look for mnemonics in the utf8 string (preceded by _) */
    }

    memset(&wattrs,0,sizeof(wattrs));
    /* If we have many questions in quick succession the dlg will jump around*/
    /*  as it tracks the cursor (which moves to the buttons). That's not good*/
    /*  So I don't do undercursor here */
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_isdlg;
    if ( restrict_input )
	wattrs.mask |= wam_restrict;
    else 
	wattrs.mask |= wam_notrestricted;
    if ( center )
	wattrs.mask |= wam_centered;
    else
	wattrs.mask |= wam_undercursor;
    wattrs.not_restricted = true;
    wattrs.restrict_input_to_me = 1;
    wattrs.is_dlg = true;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.undercursor = 1;
    wattrs.centered = 2;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = (char *) title;
    pos.x = pos.y = 0;
    pos.width = 200; pos.height = 60;		/* We'll figure size later */
		/* but if we get the size too small, the cursor isn't in dlg */
    gw = GDrawCreateTopWindow(NULL,&pos,restrict_input?d_e_h:w_e_h,d,&wattrs);

    GGadgetInit();
    GDrawSetFont(gw,_ggadget_default_font);
    GDrawWindowFontMetrics(gw,_ggadget_default_font,&as,&ds,&ld);
    fh = as+ds;
    maxw = 0;
    for ( i=0; i<lb; ++i ) {
	w = GDrawGetTextWidth(gw,qlabels[i].text,-1);
	if ( w>maxw ) maxw = w;
    }
    if ( add_text && defstr!=NULL ) {
	extern GFont *_gtextfield_font;
	if ( _gtextfield_font!=NULL ) {
	    GDrawSetFont(gw,_gtextfield_font);
	    w = GDrawGetText8Width(gw,defstr,-1);
	    GDrawSetFont(gw,_ggadget_default_font);
	} else
	    w = 8*GDrawGetText8Width(gw,defstr,-1)/5;
	w += GDrawPointsToPixels(gw,40);
	if ( w >1000 ) w = 1000;
	if ( w>maxw ) maxw = w;
    }
    bw = 0;
    for ( i=0; i<bcnt; ++i ) {
	w = GDrawGetText8Width(gw,answers[i],-1);
	if ( w>bw ) bw = w;
    }
    bw += GDrawPointsToPixels(gw,20);
    bspace = GDrawPointsToPixels(gw,6);
    if ( (bw+bspace) * bcnt > maxw )
	maxw = (bw+bspace)*bcnt;
    if ( bcnt!=1 )
	bspace = (maxw-bcnt*bw)/(bcnt-1);
    maxw += GDrawPointsToPixels(gw,16);

    gcd = calloc(lb+bcnt+2,sizeof(GGadgetCreateData));
    memset(boxes,0,sizeof(boxes));
    l = 0;
    if ( lb==1 ) {
	gcd[0].gd.pos.width = GDrawGetTextWidth(gw,qlabels[0].text,-1);
	gcd[0].gd.pos.x = (maxw-gcd[0].gd.pos.width)/2;
	gcd[0].gd.pos.y = GDrawPointsToPixels(gw,6);
	gcd[0].gd.pos.height = fh;
	gcd[0].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels | gg_pos_use0;
	gcd[0].gd.label = &qlabels[0];
	gcd[0].creator = GLabelCreate;
	labarray[0] = GCD_Glue; labarray[1] = &gcd[0]; labarray[2] = GCD_Glue; labarray[3] = NULL;
	boxes[2].gd.flags = gg_visible|gg_enabled;
	boxes[2].gd.u.boxelements = labarray;
	boxes[2].creator = GHBoxCreate;
	array[0] = &boxes[2]; array[1] = NULL;
	l = 1;
    } else for ( i=0; i<lb; ++i ) {
	gcd[i].gd.pos.x = GDrawPointsToPixels(gw,8);
	gcd[i].gd.pos.y = GDrawPointsToPixels(gw,6)+i*fh;
	gcd[i].gd.pos.width = GDrawGetTextWidth(gw,qlabels[i].text,-1);
	gcd[i].gd.pos.height = fh;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels | gg_pos_use0;
	gcd[i].gd.label = &qlabels[i];
	gcd[i].creator = GLabelCreate;
	array[2*l] = &gcd[i]; array[2*l++ +1] = NULL;
    }
    y = GDrawPointsToPixels(gw,12)+lb*fh;
    if ( add_text ) {
	gcd[bcnt+lb].gd.pos.x = GDrawPointsToPixels(gw,8);
	gcd[bcnt+lb].gd.pos.y = y;
	gcd[bcnt+lb].gd.pos.width = maxw-2*GDrawPointsToPixels(gw,6);
	gcd[bcnt+lb].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels | gg_pos_use0 | gg_text_xim;
	gcd[bcnt+lb].gd.cid = bcnt;
	gcd[bcnt+lb].creator = GTextFieldCreate;
	if ( add_text==2 )
	    gcd[bcnt+lb].creator = GPasswordCreate;
	y += fh + GDrawPointsToPixels(gw,6) + GDrawPointsToPixels(gw,10);
	array[2*l] = &gcd[bcnt+lb]; array[2*l++ +1] = NULL;
    }
    y += GDrawPointsToPixels(gw,2);
    for ( i=0; i<bcnt; ++i ) {
	gcd[i+lb].gd.pos.x = GDrawPointsToPixels(gw,8) + i*(bw+bspace);
	gcd[i+lb].gd.pos.y = y;
	gcd[i+lb].gd.pos.width = bw;
	gcd[i+lb].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels | gg_pos_use0;
	if ( i==def ) {
	    gcd[i+lb].gd.flags |= gg_but_default;
	    if ( _GGadget_defaultbutton_box.flags&box_draw_default ) {
		gcd[i+lb].gd.pos.x -= GDrawPointsToPixels(gw,3);
		gcd[i+lb].gd.pos.y -= GDrawPointsToPixels(gw,3);
		gcd[i+lb].gd.pos.width += 2*GDrawPointsToPixels(gw,3);
	    }
	}
	if ( i==cancel )
	    gcd[i+lb].gd.flags |= gg_but_cancel;
	gcd[i+lb].gd.cid = i;
	gcd[i+lb].gd.label = &blabels[i];
	gcd[i+lb].creator = GButtonCreate;
	barray[2*i] = GCD_Glue; barray[2*i+1] = &gcd[i+lb];
    }
    barray[2*i] = GCD_Glue; barray[2*i+1] = NULL;
    boxes[3].gd.flags = gg_visible|gg_enabled;
    boxes[3].gd.u.boxelements = barray;
    boxes[3].creator = GHBoxCreate;
    array[2*l] = &boxes[3]; array[2*l++ +1] = NULL;
    array[2*l] = NULL;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_visible|gg_enabled;
    boxes[0].gd.u.boxelements = array;
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(gw,boxes);
    GHVBoxSetExpandableCol(boxes[3].ret,gb_expandgluesame);
    if ( boxes[2].ret!=NULL )
	GHVBoxSetExpandableCol(boxes[2].ret,gb_expandglue);
    GHVBoxFitWindow(boxes[0].ret);

    GWidgetHidePalettes();
    if ( d!=NULL ) {
	d->ret  = cancel;
	d->bcnt = bcnt;
    }
    GDrawSetVisible(gw,true);
    free(blabels);
    free(gcd);
    free(barray);
    for ( i=0; i<lb; ++i )
	free(qlabels[i].text);
    free(ubuf);
    GProgressResumeTimer();
return( gw );
}

int GWidgetAsk8(const char *title,
	const char **answers, int def, int cancel,
	const char *question,...) {
    struct dlg_info d;
    GWindow gw;
    va_list ap;

    if ( screen_display==NULL )
return( def );

    va_start(ap,question);
    gw = DlgCreate8(title,question,ap,answers,def,cancel,&d,false,NULL,true,false);
    va_end(ap);

    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
return(d.ret);
}

int GWidgetAskCentered8(const char *title,
	const char **answers, int def, int cancel,
	const char *question, ... ) {
    struct dlg_info d;
    GWindow gw;
    va_list ap;

    if ( screen_display==NULL )
return( def );

    va_start(ap,question);
    gw = DlgCreate8(title,question,ap,answers,def,cancel,&d,false,NULL,true,true);
    va_end(ap);

    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
return(d.ret);
}

char *GWidgetAskString8(const char *title,const char *def,
	const char *question,...) {
    struct dlg_info d;
    GWindow gw;
    char *ret = NULL;
    char *ocb[3];
    va_list ap;

    if ( screen_display==NULL )
return( copy(def ));

    ocb[2]=NULL;
    if ( _ggadget_use_gettext ) {
	ocb[0] = _("_OK");
	ocb[1] = _("_Cancel");
    } else {
	ocb[0] = u2utf8_copy(GStringGetResource( _STR_OK, NULL));
	ocb[1] = u2utf8_copy(GStringGetResource( _STR_Cancel, NULL));
    }
    va_start(ap,question);
    gw = DlgCreate8(title,question,ap,(const char **) ocb,0,1,&d,true,def,true,false);
    va_end(ap);
    if ( def!=NULL && *def!='\0' )
	GGadgetSetTitle8(GWidgetGetControl(gw,2),def);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    if ( d.ret==0 )
	ret = GGadgetGetTitle8(GWidgetGetControl(gw,2));
    GDrawDestroyWindow(gw);
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
    if ( !_ggadget_use_gettext ) {
	free(ocb[0]); free(ocb[1]);
    }
return(ret);
}

char *GWidgetAskPassword8(const char *title,const char *def,
	const char *question,...) {
    struct dlg_info d;
    GWindow gw;
    char *ret = NULL;
    char *ocb[3];
    va_list ap;

    if ( screen_display==NULL )
return( copy(def ));

    ocb[2]=NULL;
    if ( _ggadget_use_gettext ) {
	ocb[0] = _("_OK");
	ocb[1] = _("_Cancel");
    } else {
	ocb[0] = u2utf8_copy(GStringGetResource( _STR_OK, NULL));
	ocb[1] = u2utf8_copy(GStringGetResource( _STR_Cancel, NULL));
    }
    va_start(ap,question);
    gw = DlgCreate8(title,question,ap,(const char **) ocb,0,1,&d,2,def,true,false);
    va_end(ap);
    if ( def!=NULL && *def!='\0' )
	GGadgetSetTitle8(GWidgetGetControl(gw,2),def);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    if ( d.ret==0 )
	ret = GGadgetGetTitle8(GWidgetGetControl(gw,2));
    GDrawDestroyWindow(gw);
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
    if ( !_ggadget_use_gettext ) {
	free(ocb[0]); free(ocb[1]);
    }
return(ret);
}

void _GWidgetPostNotice8(const char *title,const char *statement,va_list ap, int timeout) {
    GWindow gw;
    char *ob[2];

    /* Force an old notice to disappear */
    if ( title==NULL ) {
	if ( last!=NULL )
	    GDrawDestroyWindow(last);
return;
    }

    ob[1]=NULL;
    if ( _ggadget_use_gettext )
	ob[0] = _("_OK");
    else
	ob[0] = u2utf8_copy(GStringGetResource( _STR_OK, NULL));
    gw = DlgCreate8(title,statement,ap,(const char **) ob,0,0,NULL,false,NULL,false,true);
    if ( gw!=NULL && timeout>0 ) 
	GDrawRequestTimer(gw,timeout*1000,0,NULL);
    /* Continue merrily on our way. Window will destroy itself in 40 secs */
    /*  or when user kills it. We can ignore it */
    if ( !_ggadget_use_gettext )
	free(ob[0]);
    last = gw;
    last_title = title;
}

void GWidgetPostNotice8(const char *title,const char *statement,...) {
    va_list ap;

    /* Force an old notice to disappear */
    if ( title==NULL ) {
	if ( last!=NULL )
	    GDrawDestroyWindow(last);
return;
    }

    va_start(ap,statement);
    _GWidgetPostNotice8(title,statement,ap,40);
    va_end(ap);
}

void GWidgetPostNoticeTimeout8(int timeout,const char *title,const char *statement,...) {
    va_list ap;

    /* Force an old notice to disappear */
    if ( title==NULL ) {
	if ( last!=NULL )
	    GDrawDestroyWindow(last);
return;
    }

    va_start(ap,statement);
    _GWidgetPostNotice8(title,statement,ap,timeout);
    va_end(ap);
}

int GWidgetPostNoticeActive8(const char *title) {
return( last!=NULL && last_title == title );
}

void GWidgetError8(const char *title,const char *statement, ...) {
    struct dlg_info d;
    GWindow gw;
    char *ob[2];
    va_list ap;

    ob[1]=NULL;
    if ( _ggadget_use_gettext )
	ob[0] = _("_OK");
    else
	ob[0] = u2utf8_copy(GStringGetResource( _STR_OK, NULL));
    va_start(ap,statement);
    gw = DlgCreate8(title,statement,ap,(const char **) ob,0,0,&d,false,NULL,true,true);
    va_end(ap);
    if ( gw!=NULL ) {
	while ( !d.done )
	    GDrawProcessOneEvent(NULL);
	GDrawDestroyWindow(gw);
    }
    if ( !_ggadget_use_gettext )
	free(ob[0]);
}

static GWindow ChoiceDlgCreate8(struct dlg_info *d,const char *title,
	const char *question, va_list ap,
	const char **choices, int cnt, char *multisel,
	char *buts[2], int def,
	int restrict_input, int center) {
    GTextInfo qlabels[GLINE_MAX+1], *llabels, blabel[4];
    GGadgetCreateData *gcd, **array, boxes[5], *labarray[4], *barray[10], *barray2[8];
    int lb,l;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    extern FontInstance *_ggadget_default_font;
    int as, ds, ld, fh;
    int w, maxw;
    int i, y, listi;
    char buf[600];
    unichar_t *ubuf;
    extern GBox _GGadget_defaultbutton_box;

    memset(d,0,sizeof(*d));
    GProgressPauseTimer();
    vsnprintf(buf,sizeof(buf)/sizeof(buf[0]),question,ap);
    ubuf = utf82u_copy(buf);
    memset(qlabels,'\0',sizeof(qlabels));
    lb = FindLineBreaks(ubuf,qlabels);
    llabels = calloc(cnt+1,sizeof(GTextInfo));
    for ( i=0; i<cnt; ++i) {
	if ( choices[i][0]=='-' && choices[i][1]=='\0' )
	    llabels[i].line = true;
	else {
	    llabels[i].text = (unichar_t *) choices[i];
	    llabels[i].text_is_1byte = true;
	}
	if ( multisel )
	    llabels[i].selected = multisel[i];
	else
	    llabels[i].selected = (i==def);
    }

    memset(&wattrs,0,sizeof(wattrs));
    /* If we have many questions in quick succession the dlg will jump around*/
    /*  as it tracks the cursor (which moves to the buttons). That's not good*/
    /*  So I don't do undercursor here */
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_isdlg;
    if ( restrict_input )
	wattrs.mask |= wam_restrict;
    else 
	wattrs.mask |= wam_notrestricted;
    if ( center )
	wattrs.mask |= wam_centered;
    else
	wattrs.mask |= wam_undercursor;
    wattrs.not_restricted = true;
    wattrs.restrict_input_to_me = 1;
    wattrs.is_dlg = true;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.undercursor = 1;
    wattrs.centered = 2;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = (char *) title;
    pos.x = pos.y = 0;
    pos.width = 220; pos.height = 60;		/* We'll figure size later */
		/* but if we get the size too small, the cursor isn't in dlg */
    gw = GDrawCreateTopWindow(NULL,&pos,c_e_h,d,&wattrs);

    GGadgetInit();
    GDrawSetFont(gw,_ggadget_default_font);
    GDrawWindowFontMetrics(gw,_ggadget_default_font,&as,&ds,&ld);
    fh = as+ds;
    maxw = 220;
    for ( i=0; i<cnt; ++i) {
	if ( llabels[i].text!=NULL ) {		/* lines */
	    w = GDrawGetText8Width(gw,(char *) llabels[i].text,-1);
	    if ( w>900 ) w = 900;
	    if ( w>maxw ) maxw = w;
	}
    }
    for ( i=0; i<lb; ++i ) {
	w = GDrawGetTextWidth(gw,qlabels[i].text,-1);
	if ( w>maxw ) maxw = w;
    }
    maxw += GDrawPointsToPixels(gw,20);

    gcd = calloc(lb+1+2+2+2,sizeof(GGadgetCreateData));
    array = calloc(2*(lb+1+2+2+2+1),sizeof(GGadgetCreateData *));
    memset(boxes,0,sizeof(boxes));
    l=0;
    if ( lb==1 ) {
	gcd[0].gd.pos.width = GDrawGetTextWidth(gw,qlabels[0].text,-1);
	gcd[0].gd.pos.x = (maxw-gcd[0].gd.pos.width)/2;
	gcd[0].gd.pos.y = GDrawPointsToPixels(gw,6);
	gcd[0].gd.pos.height = fh;
	gcd[0].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels | gg_pos_use0;
	gcd[0].gd.label = &qlabels[0];
	gcd[0].creator = GLabelCreate;
	labarray[0] = GCD_Glue; labarray[1] = &gcd[0]; labarray[2] = GCD_Glue; labarray[3] = NULL;
	boxes[2].gd.flags = gg_visible|gg_enabled;
	boxes[2].gd.u.boxelements = labarray;
	boxes[2].creator = GHBoxCreate;
	array[0] = &boxes[2]; array[1] = NULL;
	i = l = 1;
    } else for ( i=0; i<lb; ++i ) {
	gcd[i].gd.pos.x = GDrawPointsToPixels(gw,8);
	gcd[i].gd.pos.y = GDrawPointsToPixels(gw,6)+i*fh;
	gcd[i].gd.pos.width = GDrawGetTextWidth(gw,qlabels[i].text,-1);
	gcd[i].gd.pos.height = fh;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels | gg_pos_use0;
	gcd[i].gd.label = &qlabels[i];
	gcd[i].creator = GLabelCreate;
	array[2*l] = &gcd[i]; array[2*l++ +1] = NULL;
    }

    y = GDrawPointsToPixels(gw,12)+lb*fh;
    gcd[i].gd.pos.x = GDrawPointsToPixels(gw,8); gcd[i].gd.pos.y = y;
    gcd[i].gd.pos.width = maxw - 2*GDrawPointsToPixels(gw,8);
    gcd[i].gd.pos.height = (cnt<4?4:cnt<8?cnt:8)*fh + 2*GDrawPointsToPixels(gw,6);
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels | gg_pos_use0;
    if ( multisel )
	gcd[i].gd.flags |= gg_list_multiplesel;
    else
	gcd[i].gd.flags |= gg_list_exactlyone;
    gcd[i].gd.u.list = llabels;
    gcd[i].gd.cid = CID_List;
    listi = i;
    gcd[i++].creator = GListCreate;
    y += gcd[i-1].gd.pos.height + GDrawPointsToPixels(gw,10);
    array[2*l] = &gcd[i-1]; array[2*l++ +1] = NULL;

    memset(blabel,'\0',sizeof(blabel));
    if ( multisel ) {
	y -= GDrawPointsToPixels(gw,5);
	gcd[i].gd.pos.x = GDrawPointsToPixels(gw,15); gcd[i].gd.pos.y = y;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels | gg_pos_use0;
	gcd[i].gd.label = &blabel[2];
	if ( _ggadget_use_gettext ) {
	    blabel[2].text = (unichar_t *) _("Select _All");
	    blabel[2].text_is_1byte = true;
	} else
	    blabel[2].text = (unichar_t *) _STR_SelectAll;
	blabel[2].text_in_resource = true;
	gcd[i].gd.cid = CID_SelectAll;
	gcd[i].gd.handle_controlevent = GCD_Select;
	gcd[i++].creator = GButtonCreate;
	barray2[0] = GCD_Glue; barray2[1] = &gcd[i-1];

	gcd[i].gd.pos.x = maxw-GDrawPointsToPixels(gw,15)-
		GDrawPointsToPixels(gw,GIntGetResource(_NUM_Buttonsize));
	gcd[i].gd.pos.y = y;
	gcd[i].gd.pos.width = -1;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels | gg_pos_use0 ;
	gcd[i].gd.label = &blabel[3];
	if ( _ggadget_use_gettext ) {
	    blabel[3].text = (unichar_t *) _("_None");
	    blabel[3].text_is_1byte = true;
	} else
	    blabel[3].text = (unichar_t *) _STR_None;
	blabel[3].text_in_resource = true;
	gcd[i].gd.cid = CID_SelectNone;
	gcd[i].gd.handle_controlevent = GCD_Select;
	gcd[i++].creator = GButtonCreate;
	y += GDrawPointsToPixels(gw,30);
	barray2[2] = GCD_Glue; barray2[3] = &gcd[i-1]; barray2[4] = GCD_Glue; barray2[5] = NULL;
	boxes[3].gd.flags = gg_visible|gg_enabled;
	boxes[3].gd.u.boxelements = barray2;
	boxes[3].creator = GHBoxCreate;
	array[2*l] = &boxes[3]; array[2*l++ +1] = NULL;
    }

    if ( _GGadget_defaultbutton_box.flags&box_draw_default ) {
	gcd[i].gd.pos.x = GDrawPointsToPixels(gw,15)-3; gcd[i].gd.pos.y = y-3;
    } else {
	gcd[i].gd.pos.x = GDrawPointsToPixels(gw,15); gcd[i].gd.pos.y = y;
    }
    gcd[i].gd.pos.width = -1;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels |gg_but_default | gg_pos_use0;
    gcd[i].gd.label = &blabel[0];
    blabel[0].text = (unichar_t *) buts[0];
    blabel[0].text_is_1byte = true;
    blabel[0].text_in_resource = true;
    gcd[i].gd.cid = CID_OK;
    gcd[i++].creator = GButtonCreate;
    barray[0] = GCD_Glue; barray[1] = &gcd[i-1]; barray[2] = GCD_Glue; barray[3] = GCD_Glue;

    gcd[i].gd.pos.x = maxw-GDrawPointsToPixels(gw,15)-
	    GDrawPointsToPixels(gw,GIntGetResource(_NUM_Buttonsize));
    gcd[i].gd.pos.y = y;
    gcd[i].gd.pos.width = -1;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels |gg_but_cancel | gg_pos_use0;
    gcd[i].gd.label = &blabel[1];
    blabel[1].text = (unichar_t *) buts[1];
    blabel[1].text_is_1byte = true;
    blabel[1].text_in_resource = true;
    gcd[i].gd.cid = CID_Cancel;
    gcd[i++].creator = GButtonCreate;
    barray[4] = barray[5] = barray[6] = GCD_Glue; barray[7] = &gcd[i-1]; barray[8] = GCD_Glue; barray[9] = NULL;

    boxes[4].gd.flags = gg_visible|gg_enabled;
    boxes[4].gd.u.boxelements = barray;
    boxes[4].creator = GHBoxCreate;
    array[2*l] = &boxes[4]; array[2*l++ +1] = NULL;
    array[2*l] = NULL;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_visible|gg_enabled;
    boxes[0].gd.u.boxelements = array;
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(gw,boxes);
    GHVBoxSetExpandableCol(boxes[4].ret,gb_expandgluesame);
    if ( boxes[3].ret!=NULL )
	GHVBoxSetExpandableCol(boxes[3].ret,gb_expandgluesame);
    if ( boxes[2].ret!=NULL )
	GHVBoxSetExpandableCol(boxes[2].ret,gb_expandglue);
    GHVBoxFitWindow(boxes[0].ret);
    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    d->ret = -1;
    d->size_diff = pos.height - gcd[listi].gd.pos.height;
    free(llabels);
    free(array);
    free(gcd);
    for ( i=0; i<lb; ++i )
	free(qlabels[i].text);
    free(ubuf);
    GProgressResumeTimer();
return( gw );
}

int GWidgetChoices8(const char *title, const char **choices,int cnt, int def,
	const char *question,...) {
    struct dlg_info d;
    GWindow gw;
    va_list ap;
    char *buts[3];

    if ( screen_display==NULL )
return( -2 );

    va_start(ap,question);
    if ( _ggadget_use_gettext ) {
	buts[0] = _("_OK");
	buts[1] = _("_Cancel");
    } else {
	buts[0] = u2utf8_copy(GStringGetResource(_STR_OK,NULL));
	buts[1] = u2utf8_copy(GStringGetResource(_STR_Cancel,NULL));
    }
    buts[2] = NULL;
    gw = ChoiceDlgCreate8(&d,title,question,ap,
	    choices,cnt,NULL, buts,def,true,false);
    va_end(ap);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
    if ( !_ggadget_use_gettext ) {
	free(buts[0]); free(buts[1]);
    }
return(d.ret);
}

int GWidgetChoicesB8(char *title, const char **choices, int cnt, int def,
	char *buts[2], const char *question,...) {
    struct dlg_info d;
    GWindow gw;
    va_list ap;

    if ( screen_display==NULL )
return( -2 );

    va_start(ap,question);
    gw = ChoiceDlgCreate8(&d,title,question,ap,
	    choices,cnt,NULL,buts,def,true,false);
    va_end(ap);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
return(d.ret);
}

int GWidgetChoicesBM8(char *title, const char **choices,char *sel,
	int cnt, char *buts[2], const char *question,...) {
    struct dlg_info d;
    GWindow gw;
    va_list ap;
    GGadget *list;
    GTextInfo **lsel;
    int i; int32 len;
    char *buttons[3];

    if ( screen_display==NULL )
return( -2 );

    if ( buts==NULL ) {
	buts = buttons;
	buttons[2] = NULL;
	if ( _ggadget_use_gettext ) {
	    buts[0] = _("_OK");
	    buts[1] = _("_Cancel");
	} else {
	    buts[0] = u2utf8_copy(GStringGetResource(_STR_OK,NULL));
	    buts[1] = u2utf8_copy(GStringGetResource(_STR_Cancel,NULL));
	}
    }
    va_start(ap,question);
    gw = ChoiceDlgCreate8(&d,title,question,ap,
	    choices,cnt,sel,buts,-1,true,false);
    va_end(ap);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    if ( d.ret==-1 ) {
	for ( i=0; i<cnt; ++i )
	    sel[i] = 0;
    } else {
	list = GWidgetGetControl(gw,CID_List);
	lsel = GGadgetGetList(list,&len);
	for ( i=0; i<len; ++i )
	    sel[i] = lsel[i]->selected;
    }
    GDrawDestroyWindow(gw);
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
    if ( !_ggadget_use_gettext ) {
	free(buts[0]); free(buts[1]);
    }
return(d.ret);
}
