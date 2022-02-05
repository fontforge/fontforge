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

#include <fontforge-config.h>

#include "ggadget.h"
#include "ggadgetP.h"		/* For the font family names */
#include "gprogress.h"
#include "gwidget.h"
#include "ustring.h"

#include <sys/time.h>

typedef struct gprogress {
    struct timeval start_time;	/* Don't pop up unless we're after this */
    struct timeval pause_time;
    unichar_t *line1;
    unichar_t *line2;
    int sofar;
    int tot;
    int16_t stage, stages;
    int16_t width;
    int16_t l1width, l2width;
    int16_t l1y, l2y, boxy;
    int16_t last_amount;
    unsigned int aborted: 1;
    unsigned int visible: 1;
    unsigned int dying: 1;
    unsigned int paused: 1;
    unsigned int sawmap: 1;
    GWindow gw;
    GFont *font;
    struct gprogress *prev;
} GProgress;

static Color progress_background = 0xffffff, progress_foreground;
static Color progress_fillcol = 0xc0c0ff;
static GResFont progress_font = GRESFONT_INIT("400 12pt " MONO_UI_FAMILIES);

static GProgress *current;

static void GProgressDisplay(void) {
    GDrawSetVisible(current->gw,true);
    current->visible = true;
    if ( current->prev!=NULL && current->prev->visible ) {
	GDrawSetVisible(current->prev->gw,false);
	current->prev->visible = false;
    }
}

static void GProgressTimeCheck() {
    struct timeval tv;

    if ( current==NULL || current->visible || current->dying || current->paused )
return;
    gettimeofday(&tv,NULL);
    if ( tv.tv_sec>current->start_time.tv_sec ||
	    (tv.tv_sec==current->start_time.tv_sec && tv.tv_usec>current->start_time.tv_usec )) {
	if ( current->tot>0 &&
		current->sofar+current->stage*current->tot>(9*current->stages*current->tot)/10 )
return;		/* If it's almost done, no point in making it visible */
	GProgressDisplay();
    }
}

static void GProgressDraw(GProgress *p,GWindow pixmap,GRect *rect) {
    GRect r, old;
    int width, amount;

    GDrawPushClip(pixmap,rect,&old);
    GDrawSetFont(pixmap,p->font);
    if ( p->line1!=NULL )
	GDrawDrawText(pixmap, (p->width-p->l1width)/2, p->l1y, p->line1, -1,
		progress_foreground );
    if ( p->line2!=NULL )
	GDrawDrawText(pixmap, (p->width-p->l2width)/2, p->l2y, p->line2, -1,
		progress_foreground );

    r.x = GDrawPointsToPixels(pixmap,10);
    r.y = p->boxy;
    r.height = r.x;
    width = p->width-2*r.x;
    
    if ( p->tot==0 )
	amount = 0;
    else
	amount = width * (p->stage*p->tot + p->sofar)/(p->stages*p->tot);
    if ( amount>0 ) {
	r.width = amount;
	GDrawFillRect(pixmap,&r,progress_fillcol);
    } else if ( p->tot==0 ) {
	r.width = width;
	GDrawSetStippled(pixmap,1,0,0);
	GDrawFillRect(pixmap,&r,progress_fillcol);
	GDrawSetStippled(pixmap,0,0,0);
    }
    r.width = width;
    GDrawDrawRect(pixmap,&r,progress_foreground);
    GDrawPopClip(pixmap,&old);
}

static int GProgressProcess(GProgress *p) {
    int width, amount;
    int tenpt;

    if ( !p->visible )
	GProgressTimeCheck();

    tenpt = GDrawPointsToPixels(p->gw,10);
    width = p->width-2*tenpt;
    if ( p->tot==0 )
	amount = 0;
    else
	amount = width * (p->stage*p->tot + p->sofar)/(p->stages*p->tot);
    if ( amount!=p->last_amount ) {
	if ( amount<p->last_amount || p->last_amount==0 )
	    GDrawRequestExpose(p->gw,NULL,false);
	else {
	    GRect r;
	    r.height = tenpt-1;
	    r.width = width;
	    r.x = tenpt;
	    r.y = p->boxy+1;
	    GDrawRequestExpose(p->gw,&r,false);
	}
	p->last_amount = amount;
    }
    GDrawProcessPendingEvents(NULL);
return( !p->aborted );
}

static int progress_eh(GWindow gw, GEvent *event) {
    GProgress *p = GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_destroy:
	free(p->line1);
	free(p->line2);
	free(p);
      break;
      case et_close:
	p->aborted = true;
	GDrawSetVisible(gw,false);
      break;
      case et_expose:
	GProgressDraw(p,gw,&event->u.expose.rect);
      break;
      case et_controlevent:
	if ( event->u.control.subtype == et_buttonactivate )
	    p->aborted = true;
      break;
      case et_char:
	if ( (event->u.chr.state&ksm_control) && event->u.chr.chars[0]=='.' )
	    p->aborted = true;
      break;
      case et_map:
	p->sawmap = true;
      break;
      default: break;
    }
return( true );
}

/* ************************************************************************** */
static struct resed progress_re[] = {
    {N_("Color|Foreground"), "Foreground", rt_color, &progress_foreground, N_("Text color for progress windows"), NULL, { 0 }, 0, 0 },
    {N_("Color|FillColor"), "FillColor", rt_color, &progress_fillcol, N_("Color used to draw the progress bar"), NULL, { 0 }, 0, 0 },
    {N_("Color|Background"), "Background", rt_color, &progress_background, N_("Background color for progress windows"), NULL, { 0 }, 0, 0 },
    RESED_EMPTY
};
extern GResInfo ggadget_ri;
GResInfo gprogress_ri = {
    &ggadget_ri, NULL, NULL,NULL,
    NULL,	/* No box */
    &progress_font,
    NULL,
    progress_re,
    N_("Progress"),
    N_("Progress Bars"),
    "GProgress",
    "Gdraw",
    false,
    false,
    0,
    GBOX_EMPTY,
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};

void GProgressStartIndicator(
    int delay,			/* in tenths of seconds */
    const unichar_t *win_title,	/* for the window decoration */
    const unichar_t *line1,	/* First line of description */
    const unichar_t *line2,	/* Second line */
    int tot,			/* Number of sub-entities in the operation */
    int stages			/* Number of stages, each processing tot sub-entities */
) {
    GProgress *new;
    GWindowAttrs wattrs;
    GWindow root;
    GGadgetData gd;
    int ld, as, ds;
    GRect pos;
    struct timeval tv;
    GTextInfo label;

    if ( screen_display==NULL )
return;

    GResEditDoInit(&gprogress_ri);
    new = calloc(1,sizeof(GProgress));
    new->line1 = u_copy(line1);
    new->line2 = u_copy(line2);
    new->tot = tot;
    new->stages = stages;
    new->prev = current;

    root = GDrawGetRoot(NULL);
    GDrawSetFont(root, progress_font.fi);
    GDrawWindowFontMetrics(root,new->font = progress_font.fi,&as,&ds,&ld);

    if ( new->line1!=NULL )
	new->l1width = GDrawGetTextWidth(root,new->line1,-1);
    if ( new->line2!=NULL )
	new->l2width = GDrawGetTextWidth(root,new->line2,-1);
    new->l1y = GDrawPointsToPixels(root,5) + as;
    new->l2y = new->l1y + as+ds;
    new->boxy = new->l2y + as+ds;
    pos.width = (new->l1width>new->l2width)?new->l1width:new->l2width;
    if ( pos.width<GDrawPointsToPixels(root,100) )
	pos.width = GDrawPointsToPixels(root,100);
    pos.width += 2 * GDrawPointsToPixels(root,10);
    pos.height = new->boxy + GDrawPointsToPixels(root,44);
    new->width = pos.width;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|(win_title!=NULL?wam_wtitle:0)|
	    wam_centered|wam_restrict|wam_redirect|wam_isdlg|wam_backcol;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.cursor = ct_watch;
    wattrs.window_title = u_copy(win_title);
    wattrs.centered = true;
    wattrs.restrict_input_to_me = true;
    wattrs.redirect_chars_to_me = true;
    wattrs.is_dlg = true;
    wattrs.redirect_from = NULL;
    wattrs.background_color = progress_background;
    pos.x = pos.y = 0;
    new->gw = GDrawCreateTopWindow(NULL,&pos,progress_eh,new,&wattrs);
    free((void *) wattrs.window_title);

    memset(&gd,'\0',sizeof(gd)); memset(&label,'\0',sizeof(label));
    gd.pos.width = GDrawPointsToPixels(new->gw,50);
    gd.pos.x = pos.width-gd.pos.width-10;
    gd.pos.y = pos.height-GDrawPointsToPixels(new->gw,29);
    gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels | gg_pos_use0;
    gd.mnemonic = 'S';
    label.text = (unichar_t *) _("_Stop");
    label.text_is_1byte = true;
    label.text_in_resource = true;
    gd.label = &label;
    GButtonCreate( new->gw, &gd, NULL);

    /* If there's another progress indicator up, it will not move and ours */
    /*  won't be visible if we have a delay, so force delay to 0 here */
    if ( current!=NULL ) delay = 0;
    gettimeofday(&tv,NULL);
    new->start_time = tv;
    new->start_time.tv_usec += (delay%10)*100000;
    new->start_time.tv_sec += delay/10;
    if ( new->start_time.tv_usec >= 1000000 ) {
	++new->start_time.tv_sec;
	new->start_time.tv_usec -= 1000000;
    }

    current = new;
    GProgressTimeCheck();
}

void GProgressEndIndicator(void) {
    GProgress *old=current;

    if ( old==NULL )
return;
    current = old->prev;
    
    old->dying = true;
    /* the X server sometimes crashes if we: */
	    /* Create a window */
	    /* map it */
	    /* but destroy it before the server can send us the map notify event */
	    /* next three lines seem to deal with it */
    /* unmapping the window also causes a crash */
    if ( old->visible ) while ( !old->sawmap ) {
	GDrawSync(NULL);
	GDrawProcessPendingEvents(NULL);
    }
    GDrawDestroyWindow(old->gw);
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
}

void GProgressChangeLine1(const unichar_t *line1) {
    if ( current==NULL )
return;
    free( current->line1 );
    current->line1 = u_copy(line1);
    if ( current->line1!=NULL ) {
	GDrawSetFont(current->gw,current->font);
	current->l1width = GDrawGetTextWidth(current->gw,current->line1,-1);
    }
    if ( current->visible )
	GDrawRequestExpose(current->gw,NULL,false);
}

void GProgressChangeLine2(const unichar_t *line2) {
    if ( current==NULL )
return;
    free( current->line2 );
    current->line2 = u_copy(line2);
    if ( current->line2!=NULL ) {
	GDrawSetFont(current->gw,current->font);
	current->l2width = GDrawGetTextWidth(current->gw,current->line2,-1);
    }
    if ( current->visible )
	GDrawRequestExpose(current->gw,NULL,false);
}

void GProgressChangeTotal(int tot) {
    if ( current==NULL )
return;
    current->tot = tot;
}

void GProgressChangeStages(int stages) {
    if ( current==NULL )
return;
    if ( stages<=0 )
	stages = 1;
    current->stages = stages;
    if ( current->stage>=stages )
	current->stage = stages-1;
}

void GProgressEnableStop(int enabled) {
    if ( current==NULL )
return;
    GGadgetSetEnabled(GWidgetGetControl(current->gw,0),enabled);
}

int GProgressNextStage(void) {

    if ( current==NULL )
return(true);
    ++current->stage;
    current->sofar = 0;
    if ( current->stage>=current->stages )
	current->stage = current->stages-1;
return( GProgressProcess(current));
}

int GProgressNext(void) {

    if ( current==NULL )
return(true);
    ++current->sofar;
    if ( current->sofar>=current->tot )
	current->sofar = current->tot-1;
return( GProgressProcess(current));
}

int GProgressIncrementBy(int cnt) {

    if ( current==NULL )
return(true);
    current->sofar += cnt;
    if ( current->sofar>=current->tot )
	current->sofar = current->tot-1;
return( GProgressProcess(current));
}

int GProgressReset(void) {

    if ( current==NULL )
return(true);
    current->sofar = 0;
return( GProgressProcess(current));
}

void GProgressPauseTimer(void) {
    if ( current==NULL || current->visible || current->dying || current->paused )
return;
    gettimeofday(&current->pause_time,NULL);
    current->paused = true;
}

void GProgressResumeTimer(void) {
    struct timeval tv, res;

    if ( current==NULL || current->visible || current->dying || !current->paused )
return;
    current->paused = false;
    gettimeofday(&tv,NULL);
    res.tv_sec = tv.tv_sec - current->pause_time.tv_sec;
    if ( (res.tv_usec = tv.tv_usec - current->pause_time.tv_usec)<0 ) {
	--res.tv_sec;
	res.tv_usec += 1000000;
    }
    current->start_time.tv_sec += res.tv_sec;
    if ( (current->start_time.tv_usec += res.tv_usec)>= 1000000 ) {
	++current->start_time.tv_sec;
	current->start_time.tv_usec -= 1000000;
    }
}

void GProgressShow(void) {

    if ( current==NULL || current->visible || current->dying )
return;

    GProgressDisplay();
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
}

void GProgressStartIndicator8(int delay, const char *title, const char *line1,
	const char *line2, int tot, int stages) {
    unichar_t *tit = utf82u_copy(title),
		*l1 = utf82u_copy(line1),
		*l2 = utf82u_copy(line2);
    GProgressStartIndicator(delay,
	tit,l1,l2,
	tot,stages);
    free(l1); free(l2); free(tit);
}

void GProgressChangeLine1_8(const char *line1) {
    unichar_t *l1 = utf82u_copy(line1);
    GProgressChangeLine1(l1);
    free(l1);
}

void GProgressChangeLine2_8(const char *line2) {
    unichar_t *l2 = utf82u_copy(line2);
    GProgressChangeLine2(l2);
    free(l2);
}
