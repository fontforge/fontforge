/* Copyright (C) 2004 by George Williams */
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

#if defined( FONTFORGE_CONFIG_GTK ) || 1
# include <gtk/gtk.h>
# include <gdk/gdkkeysyms.h>
# include "gwwvask.h"
# include <stdarg.h>
# include <stdio.h>
# include <string.h>

/* A set of extremely simple dlgs.
	Post a notice (which vanishes after a bit)
	Post an error message (which blocks until the user presses OK)
	Ask the user a question and return which button got pressed
	Ask the user for a string
	Ask the user to chose between a list of choices
 */

static int Finish( gpointer dlg ) {
    gtk_widget_destroy(dlg);
return( FALSE );			/* End timer */
}

void gwwv_post_notice(const char *title, const char *msg, ... ) {
    char buffer[400];
    va_list va;
    GtkWidget *dlg, *lab;

    va_start(va,msg);
#if defined( _NO_SNPRINTF ) || defined( __VMS )
    vsprintf( buffer, msg, va);
#else
    vsnprintf( buffer, sizeof(buffer), msg, va);
#endif

    dlg = gtk_dialog_new_with_buttons(title,NULL,0,
	    GTK_STOCK_OK,GTK_RESPONSE_ACCEPT,NULL);
    g_signal_connect_swapped(G_OBJECT(dlg),"key-press-event",
	    G_CALLBACK(Finish), G_OBJECT(dlg));
    g_signal_connect_swapped(G_OBJECT(dlg),"button-press-event",
	    G_CALLBACK(Finish), G_OBJECT(dlg));
    g_signal_connect_swapped(G_OBJECT(dlg),"response",
	    G_CALLBACK(Finish), G_OBJECT(dlg));
    gtk_timeout_add(30*1000,Finish,dlg);

    lab = gtk_label_new_with_mnemonic(buffer);
    gtk_widget_show(lab);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dlg)->vbox),lab);

    gtk_widget_show(dlg);
    /* Don't wait for it */
}

void gwwv_error(const char *title, const char *msg, ... ) {
    char buffer[400];
    va_list va;
    GtkWidget *dlg, *lab;

    va_start(va,msg);
#if defined( _NO_SNPRINTF ) || defined( __VMS )
    vsprintf( buffer, msg, va);
#else
    vsnprintf( buffer, sizeof(buffer), msg, va);
#endif

    dlg = gtk_dialog_new_with_buttons(title,NULL,GTK_DIALOG_DESTROY_WITH_PARENT,
	    GTK_STOCK_OK,GTK_RESPONSE_ACCEPT,NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dlg),GTK_RESPONSE_ACCEPT);

    lab = gtk_label_new_with_mnemonic(buffer);
    gtk_widget_show(lab);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dlg)->vbox),lab);

    (void) gtk_dialog_run(GTK_DIALOG(dlg));
}

struct ask_data {
    int def;
    int cancel;
};

static int askdefcancel(GtkWidget *widget, GdkEventKey *key ) {
    if ( key->keyval == GDK_Escape || key->keyval == GDK_Return ) {
	struct ask_data *ad = g_object_get_data(G_OBJECT(widget),"data");
	gtk_dialog_response( GTK_DIALOG(widget),
		( key->keyval==GDK_Escape )
		    ? ad->cancel
		    : ad->def );
return( TRUE );
    }
return( FALSE );
}
    
int gwwv_ask(const char *title,const char **butnames, int def,int cancel,
	const char *msg, ... ) {
    char buffer[400];
    va_list va;
    GtkWidget *dlg, *lab;
    struct ask_data ad;
    int i, result;

    va_start(va,msg);
#if defined( _NO_SNPRINTF ) || defined( __VMS )
    vsprintf( buffer, msg, va);
#else
    vsnprintf( buffer, sizeof(buffer), msg, va);
#endif

    ad.cancel = cancel;
    ad.def = def;

    dlg = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dlg),title);
    g_object_set_data(G_OBJECT(dlg),"data", &ad);
    g_signal_connect(G_OBJECT(dlg),"key-press-event",
	    G_CALLBACK(askdefcancel), NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dlg),def);

    for ( i=0; butnames[i]!=NULL; ++i )
	gtk_dialog_add_button(GTK_DIALOG(dlg),butnames[i],i);

    lab = gtk_label_new_with_mnemonic(buffer);
    gtk_widget_show(lab);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dlg)->vbox),lab);

    result = gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);

    if ( result<0 )
return( cancel );

return( result );
}

static void ok(GtkWidget *widget, gpointer data ) {
    gtk_dialog_response( GTK_DIALOG(widget),GTK_RESPONSE_ACCEPT);
}

static int askstrdefcancel(GtkWidget *widget, GdkEventKey *key ) {
    if ( key->keyval == GDK_Escape || key->keyval == GDK_Return ) {
	gtk_dialog_response( GTK_DIALOG(widget),
		( key->keyval==GDK_Escape )
		    ? GTK_RESPONSE_REJECT
		    : GTK_RESPONSE_ACCEPT );
return( TRUE );
    }
return( FALSE );
}

char *gwwv_ask_string(const char *title,const char *def,
	const char *question,...) {
    char buffer[400];
    va_list va;
    GtkWidget *dlg, *lab, *text;
    int result;
    char *ret;

    va_start(va,question);
#if defined( _NO_SNPRINTF ) || defined( __VMS )
    vsprintf( buffer, question, va);
#else
    vsnprintf( buffer, sizeof(buffer), question, va);
#endif

    dlg = gtk_dialog_new_with_buttons(title,NULL,GTK_DIALOG_DESTROY_WITH_PARENT,
	    GTK_STOCK_OK,GTK_RESPONSE_ACCEPT,
	    GTK_STOCK_CANCEL,GTK_RESPONSE_REJECT,
	    NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dlg),GTK_RESPONSE_ACCEPT);
    g_signal_connect(G_OBJECT(dlg),"key-press-event",
	    G_CALLBACK(askstrdefcancel), NULL);

    lab = gtk_label_new_with_mnemonic(buffer);
    gtk_widget_show(lab);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dlg)->vbox),lab);

    text = gtk_entry_new();
    if ( def!=NULL )
	gtk_entry_set_text(GTK_ENTRY(text),def);
    gtk_widget_show(text);
    g_signal_connect(G_OBJECT(text),"activate",
	    G_CALLBACK(ok), dlg);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dlg)->vbox),text);

    result = gtk_dialog_run(GTK_DIALOG(dlg));
    ret = (result==GTK_RESPONSE_ACCEPT) ? strdup( gtk_entry_get_text(GTK_ENTRY(text))): NULL;
    gtk_widget_destroy(dlg);
return( ret );
}
    
static int _gwwv_choose_with_buttons(const char *title,
	const char **choices, int cnt, int def,
	const char *butnames[2], const char *msg, va_list va ) {
    char buffer[400];
    GtkWidget *dlg, *lab, *list;
    GtkListStore *model;
    int i, result, ret;
    GtkTreeIter iter;
    GtkTreeSelection *select;

#if defined( _NO_SNPRINTF ) || defined( __VMS )
    vsprintf( buffer, msg, va);
#else
    vsnprintf( buffer, sizeof(buffer), msg, va);
#endif

    dlg = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dlg),title);
    g_signal_connect(G_OBJECT(dlg),"key-press-event",
	    G_CALLBACK(askstrdefcancel), NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dlg),GTK_RESPONSE_ACCEPT);

    for ( i=0; butnames[i]!=NULL; ++i )
	gtk_dialog_add_button(GTK_DIALOG(dlg),butnames[i],
		i==0?GTK_RESPONSE_ACCEPT :
		butnames[i+1]==NULL ? GTK_RESPONSE_REJECT : i);

    lab = gtk_label_new_with_mnemonic(buffer);
    gtk_widget_show(lab);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dlg)->vbox),lab);

    model = gtk_list_store_new(1,G_TYPE_STRING);
    for ( i=0; choices[i]!=NULL; ++i ) {
	gtk_list_store_append( model, &iter);
	gtk_list_store_set( model, &iter, 0, choices[i], -1);
    }
    list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));
    gtk_tree_view_append_column( GTK_TREE_VIEW(list),
	    gtk_tree_view_column_new_with_attributes(
		    NULL,
		    gtk_cell_renderer_text_new(),
		    "text", 0,
		    NULL));
    gtk_tree_view_set_headers_visible( GTK_TREE_VIEW(list),FALSE );
    select = gtk_tree_view_get_selection( GTK_TREE_VIEW( list ));
    if ( def>=0 ) {
	GtkTreePath *path = gtk_tree_path_new_from_indices(def,-1);
	/* I don't understand this. If I use selection_single, I can't select */
	/*  anything. If I use multiple it does what I expect single to do */
	gtk_tree_selection_set_mode( select, GTK_SELECTION_MULTIPLE);
	gtk_tree_selection_select_path(select,path);
 printf( "Path=%d\n", gtk_tree_path_get_indices(path)[0] );
	gtk_tree_path_free(path);
    }
    gtk_widget_show(list);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dlg)->vbox),list);
    gtk_tree_selection_set_mode( select, GTK_SELECTION_SINGLE);

    result = gtk_dialog_run(GTK_DIALOG(dlg));
    if ( result==GTK_RESPONSE_REJECT || result==GTK_RESPONSE_NONE || result==GTK_RESPONSE_DELETE_EVENT)
	ret = -1;
    else {
	GList *sel = gtk_tree_selection_get_selected_rows(select,NULL);
	if ( sel==NULL )
	    ret = -1;
	else {
	    ret = gtk_tree_path_get_indices(sel->data)[0];
	    g_list_foreach( sel, (GFunc) gtk_tree_path_free, NULL);
	    g_list_free( sel );
	}
    }
    gtk_widget_destroy(dlg);

return( ret );
}

int gwwv_choose_with_buttons(const char *title,
	const char **choices, int cnt, int def,
	const char *butnames[2], const char *msg, ... ) {
    va_list va;

    va_start(va,msg);
return( _gwwv_choose_with_buttons(title,choices,cnt,def,butnames,msg,va));
}

int gwwv_choose(const char *title,
	const char **choices, int cnt, int def,
	const char *msg, ... ) {
    static const char *buts[] = { GTK_STOCK_OK, GTK_STOCK_CANCEL, NULL };
    va_list va;

    va_start(va,msg);
return( _gwwv_choose_with_buttons(title,choices,cnt,def,buts,msg,va));
}
#endif
