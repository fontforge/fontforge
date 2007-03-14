/* Copyright (C) 2000-2007 by George Williams */
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
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
# include "pfaeditui.h"
# include <gfile.h>
# include "splinefont.h"
# ifdef FONTFORGE_CONFIG_GDRAW
#  include "ustring.h"

extern void GMenuItemArrayFree(struct gmenuitem *mi);

static void WindowSelect(GWindow base,struct gmenuitem *mi,GEvent *e) {
    GDrawRaise(mi->ti.userdata);
}

static void AddMI(GMenuItem *mi,GWindow gw,int changed, int top) {
    mi->ti.userdata = gw;
    mi->ti.bg = changed?0xffffff:GDrawGetDefaultBackground(GDrawGetDisplayOfWindow(gw));
    if ( top ) mi->ti.fg = 0x008000;
    mi->invoke = WindowSelect;
    mi->ti.text = GDrawGetWindowTitle(gw);
    if ( u_strlen( mi->ti.text ) > 20 )
	mi->ti.text[20] = '\0';
}

/* Builds up a menu containing the titles of all the major windows */
void WindowMenuBuild(GWindow basew,struct gmenuitem *mi,GEvent *e) {
    int i, cnt, precnt;
    FontView *fv;
    CharView *cv;
    MetricsView *mv;
    BitmapView *bv;
    GMenuItem *sub;
    BDFFont *bdf;
    extern void GMenuItemArrayFree(GMenuItem *mi);

    precnt = 6;
    cnt = precnt;

    for ( fv = fv_list; fv!=NULL; fv = fv->next ) {
	++cnt;		/* for the font */
	for ( i=0; i<fv->sf->glyphcnt; ++i ) if ( fv->sf->glyphs[i]!=NULL ) {
	    for ( cv = fv->sf->glyphs[i]->views; cv!=NULL; cv=cv->next )
		++cnt;		/* for each char view in the font */
	}
	for ( bdf= fv->sf->bitmaps; bdf!=NULL; bdf = bdf->next ) {
	    for ( i=0; i<bdf->glyphcnt; ++i ) if ( bdf->glyphs[i]!=NULL ) {
		for ( bv = bdf->glyphs[i]->views; bv!=NULL; bv=bv->next )
		    ++cnt;
	    }
	}
	for ( mv=fv->sf->metrics; mv!=NULL; mv=mv->next )
	    ++cnt;
    }
    if ( cnt==0 ) {
	/* This can't happen */
return;
    }
    sub = gcalloc(cnt+1,sizeof(GMenuItem));
    memcpy(sub,mi->sub,precnt*sizeof(struct gmenuitem));
    for ( i=0; i<precnt; ++i )
	mi->sub[i].ti.text = NULL;
    GMenuItemArrayFree(mi->sub);
    mi->sub = sub;

    for ( i=0; sub[i].ti.text!=NULL || sub[i].ti.line; ++i ) {
	if ( sub[i].ti.text_is_1byte && sub[i].ti.text_in_resource) {
	    sub[i].ti.text = utf82u_mncopy((char *) sub[i].ti.text,&sub[i].ti.mnemonic);
	    sub[i].ti.text_is_1byte = sub[i].ti.text_in_resource = false;
	} else if ( sub[i].ti.text_is_1byte ) {
	    sub[i].ti.text = utf82u_copy((char *) sub[i].ti.text);
	    sub[i].ti.text_is_1byte = false;
	} else if ( sub[i].ti.text_in_resource ) {
	    sub[i].ti.text = u_copy(GStringGetResource((intpt) sub[i].ti.text,NULL));
	    sub[i].ti.text_in_resource = false;
	} else
	    sub[i].ti.text = u_copy(sub[i].ti.text);
    }
    cnt = precnt;
    for ( fv = fv_list; fv!=NULL; fv = fv->next ) {
	AddMI(&sub[cnt++],fv->gw,fv->sf->changed,true);
	for ( i=0; i<fv->sf->glyphcnt; ++i ) if ( fv->sf->glyphs[i]!=NULL ) {
	    for ( cv = fv->sf->glyphs[i]->views; cv!=NULL; cv=cv->next )
		AddMI(&sub[cnt++],cv->gw,cv->sc->changed,false);
	}
	for ( bdf= fv->sf->bitmaps; bdf!=NULL; bdf = bdf->next ) {
	    for ( i=0; i<bdf->glyphcnt; ++i ) if ( bdf->glyphs[i]!=NULL ) {
		for ( bv = bdf->glyphs[i]->views; bv!=NULL; bv=bv->next )
		    AddMI(&sub[cnt++],bv->gw,bv->bc->changed,false);
	    }
	}
	for ( mv=fv->sf->metrics; mv!=NULL; mv=mv->next )
	    AddMI(&sub[cnt++],mv->gw,false,false);
    }
}

static void RecentSelect(GWindow base,struct gmenuitem *mi,GEvent *e) {
    ViewPostscriptFont((char *) (mi->ti.userdata));
}

/* Builds up a menu containing the titles of all the unused recent files */
void MenuRecentBuild(GWindow base,struct gmenuitem *mi,GEvent *e) {
    int i, cnt, cnt1;
    FontView *fv;
    extern void GMenuItemArrayFree(struct gmenuitem *mi);
    GMenuItem *sub;

    if ( mi->sub!=NULL ) {
	GMenuItemArrayFree(mi->sub);
	mi->sub = NULL;
    }

    cnt = 0;
    for ( i=0; i<RECENT_MAX && RecentFiles[i]!=NULL; ++i ) {
	for ( fv=fv_list; fv!=NULL; fv=fv->next )
	    if ( fv->sf->filename!=NULL && strcmp(fv->sf->filename,RecentFiles[i])==0 )
	break;
	if ( fv==NULL )
	    ++cnt;
    }
    if ( cnt==0 ) {
	/* This can't happen */
return;
    }
    sub = gcalloc(cnt+1,sizeof(GMenuItem));
    cnt1 = 0;
    for ( i=0; i<RECENT_MAX && RecentFiles[i]!=NULL; ++i ) {
	for ( fv=fv_list; fv!=NULL; fv=fv->next )
	    if ( fv->sf->filename!=NULL && strcmp(fv->sf->filename,RecentFiles[i])==0 )
	break;
	if ( fv==NULL ) {
	    GMenuItem *mi = &sub[cnt1++];
	    mi->ti.userdata = RecentFiles[i];
	    mi->ti.bg = mi->ti.fg = COLOR_DEFAULT;
	    mi->invoke = RecentSelect;
	    mi->ti.text = def2u_copy(GFileNameTail(RecentFiles[i]));
	}
    }
    if ( cnt!=cnt1 )
	IError( "Bad counts in MenuRecentBuild");
    mi->sub = sub;
}

int RecentFilesAny(void) {
    int i;
    FontView *fvl;

    for ( i=0; i<RECENT_MAX && RecentFiles[i]!=NULL; ++i ) {
	for ( fvl=fv_list; fvl!=NULL; fvl=fvl->next )
	    if ( fvl->sf->filename!=NULL && strcmp(fvl->sf->filename,RecentFiles[i])==0 )
	break;
	if ( fvl==NULL )
return( true );
    }
return( false );
}

static void ScriptSelect(GWindow base,struct gmenuitem *mi,GEvent *e) {
    int index = (intpt) (mi->ti.userdata);
    FontView *fv = (FontView *) GDrawGetUserData(base);

    /* the menu is not always up to date. If user changed prefs and then used */
    /*  Alt|Ctl|Digit s/he would not get a new menu built and the old one might*/
    /*  refer to something out of bounds. Hence the check */
    if ( index<0 || script_filenames[index]==NULL )
return;
    ExecuteScriptFile(fv,script_filenames[index]);
}

/* Builds up a menu containing any user defined scripts */
void MenuScriptsBuild(GWindow base,struct gmenuitem *mi,GEvent *e) {
    int i;
    GMenuItem *sub;

    if ( mi->sub!=NULL ) {
	GMenuItemArrayFree(mi->sub);
	mi->sub = NULL;
    }

    for ( i=0; i<SCRIPT_MENU_MAX && script_menu_names[i]!=NULL; ++i );
    if ( i==0 ) {
	/* This can't happen */
return;
    }
    sub = gcalloc(i+1,sizeof(GMenuItem));
    for ( i=0; i<SCRIPT_MENU_MAX && script_menu_names[i]!=NULL; ++i ) {
	GMenuItem *mi = &sub[i];
	mi->ti.userdata = (void *) (intpt) i;
	mi->ti.bg = mi->ti.fg = COLOR_DEFAULT;
	mi->invoke = ScriptSelect;
	mi->shortcut = i==9?'0':'1'+i;
	mi->short_mask = ksm_control|ksm_meta;
	mi->ti.text = u_copy(script_menu_names[i]);
    }
    mi->sub = sub;
}

/* Builds up a menu containing all the anchor classes */
void _aplistbuild(struct gmenuitem *top,SplineFont *sf,
	void (*func)(GWindow,struct gmenuitem *,GEvent *)) {
    int cnt;
    GMenuItem *mi, *sub;
    AnchorClass *ac;

    if ( top->sub!=NULL ) {
	GMenuItemArrayFree(top->sub);
	top->sub = NULL;
    }

    cnt = 0;
    for ( ac = sf->anchor; ac!=NULL; ac=ac->next ) ++cnt;
    if ( cnt==0 )
	cnt = 1;
    else
	cnt += 2;
    sub = gcalloc(cnt+1,sizeof(GMenuItem));
    mi = &sub[0];
    mi->ti.userdata = (void *) (-1);
    mi->ti.bg = mi->ti.fg = COLOR_DEFAULT;
    mi->invoke = func;
    mi->ti.text = utf82u_copy(_("All"));
    if ( cnt==1 )
	mi->ti.disabled = true;
    else {
	++mi;
	mi->ti.bg = mi->ti.fg = COLOR_DEFAULT;
	mi->ti.line = true;
	++mi;
    }
    for ( ac=sf->anchor; ac!=NULL; ac = ac->next, ++mi ) {
	mi->ti.userdata = (void *) ac;
	mi->ti.bg = mi->ti.fg = COLOR_DEFAULT;
	mi->invoke = func;
	mi->ti.text = utf82u_copy(ac->name);
    }
    top->sub = sub;
}
# elif defined(FONTFORGE_CONFIG_GTK)
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
	gtk_check_menu_item_set_active( GTK_CHECKMENUITEM(mi), TRUE );
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
    GtkWidget *w;
    FontView *fv;
    CharView *cv;
    MetricsView *mv;
    BitmapView *bv;

    if ( RecentFiles[0]==NULL )
return;
    shell = gtk_menu_item_get_submenu( GTK_MENU_ITEM( menuitem ));

    sub = gtk_container_get_children( GTK_CONTAINER( shell ));
    while ( sub!=NULL ) {
	gtk_container_remove( GTK_CONTAINER(shell), GTK_WIDGET( sub->data ));
	sub = g_list_remove(sub, sub->data);
    }

    for ( fv = fv_list; fv!=NULL; fv = fv->next ) {
	AddMI(shell,current, fv->gw,fv->sf->fontname, fv->sf->changed,true);
	for ( i=0; i<fv->sf->glyphcnt; ++i ) if ( fv->sf->glyphs[i]!=NULL ) {
	    for ( cv = fv->sf->glyphs[i]->views; cv!=NULL; cv=cv->next )
		AddMI(shell,current,cv->gw,cv->sc->name,cv->sc->changed,false);
	}
	for ( bdf= fv->sf->bitmaps; bdf!=NULL; bdf = bdf->next ) {
	    for ( i=0; i<bdf->glyphcnt; ++i ) if ( bdf->glyphs[i]!=NULL ) {
		for ( bv = bdf->glyphs[i]->views; bv!=NULL; bv=bv->next )
		    AddMI(shell,current,bv->gw,fv->sf->glyphs[i]->name,
			    bv->bc->changed,false);
	    }
	}
	for ( mv=fv->sf->metrics; mv!=NULL; mv=mv->next )
	    AddMI(shell,current,mv->gw,"Metrics",false,false);
    }
}


static void RecentSelect(GtkMenuItem *menuitem, gpointer user_data) {
    ViewPostscriptFont((char *) (userdata));
}

void RecentMenuBuild(GtkMenuItem *menuitem, gpointer user_data) {
    int i;
    GList *sub;
    GtkMenuShell *shell;
    GtkWidget *w;
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
	for ( fv=fv_list; fv!=NULL; fv=fv->next )
	    if ( fv->sf->filename!=NULL && strcmp(fv->sf->filename,RecentFiles[i])==0 )
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


static void ScriptSelect(GtkMenuItem *menuitem, gpointer user_data) {
    int index = (int) (userdata);
    FontView *fv = (FontView *) g_object_get_data( \
		G_OBJECT( gtk_widget_get_toplevel( GTK_WIDGET( menuitem ))),\
		"data" );

    /* the menu is not always up to date. If user changed prefs and then used */
    /*  Alt|Ctl|Digit s/he would not get a new menu built and the old one might*/
    /*  refer to something out of bounds. Hence the check */
    if ( index<0 || script_filenames[index]==NULL )
return;
    ExecuteScriptFile(fv,script_filenames[index]);
}

void ScriptMenuBuild(GtkMenuItem *menuitem, gpointer user_data) {
    int i;
    GList *sub;
    GtkMenuShell *shell;
    GtkWidget *w;

    if ( script_menu_names[0]==NULL )
return;
    shell = gtk_menu_item_get_submenu( GTK_MENU_ITEM( menuitem ));

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
    int cnt;
    GMenuItem *mi, *sub;
    AnchorClass *ac;
    GList *sub;
    GtkMenuShell *shell;
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
	for ( ac = sf->anchor; ac!=NULL; ac=ac->next ) {
    }
    for ( ac=sf->anchor; ac!=NULL; ac = ac->next, ++mi ) {
	w =  gtk_menu_item_new_with_label( ac->name );
	gtk_widget_show (w);
	gtk_container_add (GTK_CONTAINER (shell), w);
	g_signal_connect ((gpointer) w, "activate",
			G_CALLBACK (func),
			(gpointer) ac);
    }
}
# endif

void mbDoGetText(GMenuItem *mb) {
    /* perform gettext substitutions on this menu and all sub menus */
    int i;

    if ( mb==NULL )
return;
    for ( i=0; mb[i].ti.text!=NULL || mb[i].ti.line || mb[i].ti.image!=NULL; ++i ) {
	if ( mb[i].ti.text!=NULL ) {
	    mb[i].ti.text = (unichar_t *) S_((char *) mb[i].ti.text);
	    if ( mb[i].sub!=NULL )
		mbDoGetText(mb[i].sub);
	}
    }
}

void mb2DoGetText(GMenuItem2 *mb) {
    /* perform gettext substitutions on this menu and all sub menus */
    int i;

    if ( mb==NULL )
return;
    for ( i=0; mb[i].ti.text!=NULL || mb[i].ti.line || mb[i].ti.image!=NULL; ++i ) {
	if ( mb[i].ti.text!=NULL ) {
	    mb[i].ti.text = (unichar_t *) S_((char *) mb[i].ti.text);
	    if ( mb[i].sub!=NULL )
		mb2DoGetText(mb[i].sub);
	}
    }
}
#endif
