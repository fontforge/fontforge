/* Copyright (C) 2000-2008 by George Williams */
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
# include "fontforgegtk.h"
# include <gfile.h>
# include <fontforge/splinefont.h>

static void WindowSelect(GtkMenuItem *menuitem, gpointer user_data) {
    GtkWidget *top = GTK_WIDGET( user_data );

    gdk_window_show(top->window);
}


static void AddMI(GtkMenuShell *shell,GtkWidget *current, GtkWidget *cur,
	char *name, int changed, int is_fontview) {
    GtkWidget *mi;
    char buffer[28];

    sprintf( buffer, "%s%.20s %c", is_fontview ? "" : " ", name, changed ? '~' : ' ' );

    if ( current==cur ) {
	mi = gtk_check_menu_item_new_with_label(buffer);
	gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(mi), TRUE );
    } else
	mi = gtk_menu_item_new_with_label(buffer);
    gtk_widget_show (mi);
    gtk_container_add (GTK_CONTAINER (shell), mi);
    g_signal_connect ((gpointer) mi, "activate",
		    G_CALLBACK (WindowSelect),
		    (gpointer) cur);
}

void Menu_ActivateWindows(GtkMenuItem *menuitem, gpointer user_data) {
    GtkWidget *current = gtk_widget_get_toplevel( GTK_WIDGET( menuitem ));
    int i;
    GList *sub;
    GtkMenuShell *shell;
    FontView *fv;
    CharView *cv;
    BDFFont *bdf;
    MetricsView *mv;
    BitmapView *bv;

    if ( RecentFiles[0]==NULL )
return;
    shell = GTK_MENU_SHELL(gtk_menu_item_get_submenu( GTK_MENU_ITEM( menuitem )));

    sub = gtk_container_get_children( GTK_CONTAINER( shell ));
    while ( sub!=NULL ) {
	gtk_container_remove( GTK_CONTAINER(shell), GTK_WIDGET( sub->data ));
	sub = g_list_remove(sub, sub->data);
    }

    for ( fv = fv_list; fv!=NULL; fv = (FontView *) fv->b.next ) {
	AddMI(shell,current, fv->gw,fv->b.sf->fontname, fv->b.sf->changed,true);
	for ( i=0; i<fv->b.sf->glyphcnt; ++i ) if ( fv->b.sf->glyphs[i]!=NULL ) {
	    for ( cv = (CharView *) fv->b.sf->glyphs[i]->views; cv!=NULL; cv=(CharView *) cv->b.next )
		AddMI(shell,current,cv->gw,cv->b.sc->name,cv->b.sc->changed,false);
	}
	for ( bdf= fv->b.sf->bitmaps; bdf!=NULL; bdf = bdf->next ) {
	    for ( i=0; i<bdf->glyphcnt; ++i ) if ( bdf->glyphs[i]!=NULL ) {
		for ( bv = bdf->glyphs[i]->views; bv!=NULL; bv=bv->next )
		    AddMI(shell,current,bv->gw,fv->b.sf->glyphs[i]->name,
			    bv->bc->changed,false);
	    }
	}
	for ( mv=fv->b.sf->metrics; mv!=NULL; mv=mv->next )
	    AddMI(shell,current,mv->gw,"Metrics",false,false);
    }
}


static void RecentSelect(GtkMenuItem *menuitem, gpointer user_data) {
    ViewPostscriptFont((char *) (user_data),0);
}

void RecentMenuBuild(GtkMenuItem *menuitem, gpointer user_data) {
    int i;
    GList *sub;
    GtkWidget *shell;
    FontView *fv;

    if ( RecentFiles[0]==NULL )
return;
    shell = gtk_menu_item_get_submenu( GTK_MENU_ITEM( menuitem ));
    sub = gtk_container_get_children( GTK_CONTAINER( shell ));
    while ( sub!=NULL ) {
	gtk_container_remove( GTK_CONTAINER(shell), GTK_WIDGET( sub->data ));
	sub = g_list_remove(sub, sub->data);
    }

    for ( i=0; i<RECENT_MAX && RecentFiles[i]!=NULL; ++i ) {
	for ( fv=fv_list; fv!=NULL; fv=(FontView *) fv->b.next )
	    if ( fv->b.sf->filename!=NULL && strcmp(fv->b.sf->filename,RecentFiles[i])==0 )
	break;
	if ( fv==NULL ) {
	    GtkWidget *recent_mi = gtk_menu_item_new_with_label( GFileNameTail(RecentFiles[i]) );
	    gtk_widget_show (recent_mi);
	    gtk_container_add (GTK_CONTAINER (shell), recent_mi);
	    g_signal_connect ((gpointer) recent_mi, "activate",
			    G_CALLBACK (RecentSelect),
			    (gpointer) RecentFiles[i]);
	}
    }
}

int RecentFilesAny(void) {
    int i;
    FontViewBase *fvl;

    for ( i=0; i<RECENT_MAX && RecentFiles[i]!=NULL; ++i ) {
	for ( fvl=(FontViewBase *) fv_list; fvl!=NULL; fvl=fvl->next )
	    if ( fvl->sf->filename!=NULL && strcmp(fvl->sf->filename,RecentFiles[i])==0 )
	break;
	if ( fvl==NULL )
return( true );
    }
return( false );
}

static void ScriptSelect(GtkMenuItem *menuitem, gpointer user_data) {
    int index = (int) (user_data);
    FontViewBase *fv = (FontViewBase *) g_object_get_data( \
		G_OBJECT( gtk_widget_get_toplevel( GTK_WIDGET( menuitem ))),\
		"data" );

    /* the menu is not always up to date. If user changed prefs and then used */
    /*  Alt|Ctl|Digit s/he would not get a new menu built and the old one might*/
    /*  refer to something out of bounds. Hence the check */
    if ( index<0 || script_filenames[index]==NULL )
return;
    ExecuteScriptFile(fv,NULL,script_filenames[index]);
}

void ScriptMenuBuild(GtkMenuItem *menuitem, gpointer user_data) {
    int i;
    GList *sub;
    GtkMenuShell *shell;

    if ( script_menu_names[0]==NULL )
return;
    shell = GTK_MENU_SHELL(gtk_menu_item_get_submenu( GTK_MENU_ITEM( menuitem )));

    sub = gtk_container_get_children( GTK_CONTAINER( shell ));
    while ( sub!=NULL ) {
	gtk_container_remove( GTK_CONTAINER(shell), GTK_WIDGET( sub->data ));
	sub = g_list_remove(sub, sub->data);
    }

    for ( i=0; i<SCRIPT_MENU_MAX && script_menu_names[i]!=NULL; ++i ) {
	GtkWidget *script_mi =  gtk_menu_item_new_with_label( script_menu_names[i] );
	gtk_widget_show (script_mi);
	gtk_container_add (GTK_CONTAINER (shell), script_mi);
	g_signal_connect ((gpointer) script_mi, "activate",
			G_CALLBACK (ScriptSelect),
			(gpointer) i);
/* Not sure how to get the accelerator group of the menu */
/*	gtk_widget_add_accelerator (script_mi, "activate", accel_group,	*/
/*		i==9? '0' : '1'+i, GDK_CONTROL_MASK|GDK_MOD1_MASK,	*/
/*		GTK_ACCEL_VISIBLE);					*/
	
    }
}

/* Builds up a menu containing all the anchor classes */
void _aplistbuild(GtkMenuItem *menuitem,SplineFont *sf,
	void (*func)(GtkMenuItem *menuitem, gpointer user_data)) {
    AnchorClass *ac;
    GList *sub;
    GtkWidget *shell;
    GtkWidget *w;

    shell = gtk_menu_item_get_submenu( GTK_MENU_ITEM( menuitem ));

    sub = gtk_container_get_children( GTK_CONTAINER( shell ));
    while ( sub!=NULL ) {
	gtk_container_remove( GTK_CONTAINER(shell), GTK_WIDGET( sub->data ));
	sub = g_list_remove(sub, sub->data);
    }

    w =  gtk_menu_item_new_with_label( _("All") );
    gtk_widget_show (w);
    gtk_container_add (GTK_CONTAINER (shell), w);
    g_signal_connect ((gpointer) w, "activate",
		    G_CALLBACK (func),
		    (gpointer) (void *) (-1));
    if ( sf->anchor!=NULL ) {
	w = gtk_separator_menu_item_new( );
	gtk_widget_show (w);
	gtk_container_add (GTK_CONTAINER (shell), w);
	for ( ac=sf->anchor; ac!=NULL; ac = ac->next ) {
	    w =  gtk_menu_item_new_with_label( ac->name );
	    gtk_widget_show (w);
	    gtk_container_add (GTK_CONTAINER (shell), w);
	    g_signal_connect ((gpointer) w, "activate",
			    G_CALLBACK (func),
			    (gpointer) ac);
	}
    } else
	gtk_widget_set_sensitive(w,false);
}
