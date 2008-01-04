/* Copyright (C) 2006 by George Williams */
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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <libintl.h>
#define _(str)	gettext(str)

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "basics.h"
#include "support.h"
#include "gwwvprogress.h"

#define GLADE_HOOKUP_OBJECT(component,widget,name) \
  g_object_set_data_full (G_OBJECT (component), name, \
    gtk_widget_ref (widget), (GDestroyNotify) gtk_widget_unref)

#define GLADE_HOOKUP_OBJECT_NO_REF(component,widget,name) \
  g_object_set_data (G_OBJECT (component), name, widget)


typedef struct gprogress {
    struct timeval start_time;	/* Don't pop up unless we're after this */
    struct timeval pause_time;
    int sofar;
    int tot;
    gint16 stage, stages;
    unsigned int aborted: 1;
    unsigned int superceded: 1;
    unsigned int visible: 1;
    unsigned int paused: 1;
    unsigned int sawmap: 1;
    unsigned int death_pending: 1;
    GtkWidget *progress;
    struct gprogress *prev;
} GProgress;

static GProgress *current;


static void gwwv_progress__end_indicator(void) {
    GProgress *old = current;

    current=old->prev;
    gtk_widget_destroy(old->progress);
    free(old);
    if ( current!=NULL ) {
	current->superceded = false;
	if ( current->visible )
	    gtk_widget_show(current->progress);
    }
}

static void gwwv_progress__map(GtkWidget *widget, gpointer user_data) {
    if ( current!=NULL ) {
	current->sawmap = true;
	if ( current->death_pending )
	    gwwv_progress__end_indicator();
    }
}


static void gwwv_progress__stop(GtkButton *button, gpointer user_data) {
    if ( current!=NULL )
	current->aborted = true;
}

static GtkWidget* create_Progress(void) {
  GtkWidget *Progress;
  GtkWidget *vbox1;
  GtkWidget *line1;
  GtkWidget *line2;
  GtkWidget *progressbar;
  GtkWidget *hbox1;
  GtkWidget *stop;
  GtkWidget *alignment1;
  GtkWidget *hbox2;
  GtkWidget *image1;
  GtkWidget *label3;

  Progress = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (Progress), _("Progress"));
  gtk_window_set_modal (GTK_WINDOW (Progress), TRUE);

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (Progress), vbox1);

  line1 = gtk_label_new (_("line1"));
  gtk_widget_show (line1);
  gtk_box_pack_start (GTK_BOX (vbox1), line1, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (line1), GTK_JUSTIFY_CENTER);

  line2 = gtk_label_new (_("line2"));
  gtk_widget_show (line2);
  gtk_box_pack_start (GTK_BOX (vbox1), line2, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (line2), GTK_JUSTIFY_CENTER);

  progressbar = gtk_progress_bar_new ();
  gtk_widget_show (progressbar);
  gtk_box_pack_start (GTK_BOX (vbox1), progressbar, FALSE, FALSE, 0);

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox1);
  gtk_box_pack_start (GTK_BOX (vbox1), hbox1, TRUE, TRUE, 0);

  stop = gtk_button_new ();
  gtk_widget_show (stop);
  gtk_box_pack_start (GTK_BOX (hbox1), stop, FALSE, FALSE, 0);

  alignment1 = gtk_alignment_new (0.5, 0.5, 0, 0);
  gtk_widget_show (alignment1);
  gtk_container_add (GTK_CONTAINER (stop), alignment1);

  hbox2 = gtk_hbox_new (FALSE, 2);
  gtk_widget_show (hbox2);
  gtk_container_add (GTK_CONTAINER (alignment1), hbox2);

  image1 = gtk_image_new_from_stock ("gtk-stop", GTK_ICON_SIZE_BUTTON);
  gtk_widget_show (image1);
  gtk_box_pack_start (GTK_BOX (hbox2), image1, FALSE, FALSE, 0);

  label3 = gtk_label_new_with_mnemonic (_("Stop"));
  gtk_widget_show (label3);
  gtk_box_pack_start (GTK_BOX (hbox2), label3, FALSE, FALSE, 0);

  g_signal_connect ((gpointer) Progress, "map",
                    G_CALLBACK (gwwv_progress__map),
                    NULL);
  g_signal_connect ((gpointer) stop, "activate",
                    G_CALLBACK (gwwv_progress__stop),
                    NULL);

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF (Progress, Progress, "Progress");
  GLADE_HOOKUP_OBJECT (Progress, vbox1, "vbox1");
  GLADE_HOOKUP_OBJECT (Progress, line1, "line1");
  GLADE_HOOKUP_OBJECT (Progress, line2, "line2");
  GLADE_HOOKUP_OBJECT (Progress, progressbar, "progressbar");
  GLADE_HOOKUP_OBJECT (Progress, hbox1, "hbox1");
  GLADE_HOOKUP_OBJECT (Progress, stop, "stop");
  GLADE_HOOKUP_OBJECT (Progress, alignment1, "alignment1");
  GLADE_HOOKUP_OBJECT (Progress, hbox2, "hbox2");
  GLADE_HOOKUP_OBJECT (Progress, image1, "image1");
  GLADE_HOOKUP_OBJECT (Progress, label3, "label3");

  return Progress;
}

static void gwwv_progress__display(void) {
    if ( current==NULL )
return;
    gtk_widget_show(current->progress);
    current->visible = true;
    if ( current->prev!=NULL && current->prev->visible ) {
	gtk_widget_hide(current->prev->progress);
	current->superceded = true;
    }
}

static void gwwv_progress__time_check(void) {
    struct timeval tv;

    if ( current==NULL || current->visible || current->death_pending || current->paused )
return;
    gettimeofday(&tv,NULL);
    if ( tv.tv_sec>current->start_time.tv_sec ||
	    (tv.tv_sec==current->start_time.tv_sec && tv.tv_usec>current->start_time.tv_usec )) {
	if ( current->tot>0 &&
		current->sofar+current->stage*current->tot>(9*current->stages*current->tot)/10 )
return;		/* If it's almost done, no point in making it visible */
	gwwv_progress__display();
    }
}

static int gwwv_progress__process(void) {
    double frac;
    GtkWidget *bar;

    if ( current==NULL || current->paused )
return( true );
    if ( !current->visible )
	gwwv_progress__time_check();
    if ( !current->visible )
return( true );

    if ( current->tot==0 || current->stages==0 )
	frac = 0;
    else
	frac = (current->stage*current->tot + current->sofar)/
		(double) (current->stages*current->tot);
    bar = lookup_widget(current->progress,"progressbar");
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(bar),frac);

    g_main_context_iteration(NULL,FALSE);
return( !current->aborted );
}


/* ************************************************************************** */
/* *************************** External Interface *************************** */
/* ************************************************************************** */
void gwwv_progress_start_indicator(int delay, const char *title, const char *line1,
	const char *line2, int tot, int stages) {
    GProgress *new;
    struct timeval tv;
    GtkWidget *line;

    new = gcalloc(1,sizeof(GProgress));
    new->progress = create_Progress();
    gtk_window_set_title(GTK_WINDOW(new->progress),title);
    line = lookup_widget(new->progress,"line1");
    gtk_label_set_text(GTK_LABEL(line),line1);
    line = lookup_widget(new->progress,"line2");
    gtk_label_set_text(GTK_LABEL(line),line2);

    new->tot = tot;
    new->stages = stages;
    new->prev = current;

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
    gwwv_progress__time_check();
}

void gwwv_progress_end_indicator(void) {

    if ( current==NULL )
return;
    if ( !current->sawmap )
	current->death_pending = true;
    else
	gwwv_progress__end_indicator();
}

void gwwv_progress_show(void) {

    if ( current==NULL || current->visible || current->death_pending )
return;
    gwwv_progress__display();
}

int gwwv_progress_next(void) {

    if ( current==NULL )
return(true);
    ++current->sofar;
    if ( current->sofar>=current->tot )
	current->sofar = current->tot-1;
return( gwwv_progress__process());
}

int gwwv_progress_increment(int cnt) {

    if ( current==NULL )
return(true);
    current->sofar += cnt;
    if ( current->sofar>=current->tot )
	current->sofar = current->tot-1;
return( gwwv_progress__process());
}

int gwwv_progress_next_stage(void) {

    if ( current==NULL )
return(true);
    ++current->stage;
    current->sofar = 0;
    if ( current->stage>=current->stages )
	current->stage = current->stages-1;
return( gwwv_progress__process());
}

void gwwv_progress_change_line1(const char *line1) {
    GtkWidget *line;
    line = lookup_widget(current->progress,"line1");
    gtk_label_set_text(GTK_LABEL(line),line1);
}

void gwwv_progress_change_line2(const char *line2) {
    GtkWidget *line;
    line = lookup_widget(current->progress,"line2");
    gtk_label_set_text(GTK_LABEL(line),line2);
}

void gwwv_progress_enable_stop(int enabled) {
    GtkWidget *stop;
    stop = lookup_widget(current->progress,"stop");
    gtk_widget_set_sensitive(stop,enabled);
}

void gwwv_progress_change_total(int tot) {
    if ( current==NULL || tot<=0 )
return;
    current->tot = tot;
    if ( current->sofar>current->tot )
	current->sofar = current->tot;
}
 
void gwwv_progress_change_stages(int stages) {
    if ( current==NULL )
return;
    if ( stages<=0 )
	stages = 1;
    current->stages = stages;
    if ( current->stage>=stages )
	current->stage = stages-1;
}

void gwwv_progress_pause_timer(void) {
    if ( current==NULL || current->visible || current->death_pending || current->paused )
return;
    gettimeofday(&current->pause_time,NULL);
    current->paused = true;
}

void gwwv_progress_resume_timer(void) {
    struct timeval tv, res;

    if ( current==NULL || current->visible || current->death_pending || !current->paused )
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

int gwwv_progress_reset(void) {

    if ( current==NULL )
return(true);
    current->sofar = 0;
return( gwwv_progress__process());
}
