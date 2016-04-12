/* Copyright (C) 2004-2008 by George Williams */
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

# include <gtk/gtk.h>
# include <gdk/gdkkeysyms.h>
# include <glib.h>
# include "gwwvask.h"
# include "support.h"
# include <stdarg.h>
# include <stdio.h>
# include <string.h>
# include <stdlib.h>
# include <ctype.h>
# include <sys/stat.h>
# include <sys/types.h>
# include <unistd.h>
# include <errno.h>
# include <libintl.h>
# include <stdint.h>
# define _(str)	gettext(str)

/* A set of extremely simple dlgs.
	Post a notice (which vanishes after a bit)
	Post an error message (which blocks until the user presses OK)
	Ask the user a question and return which button got pressed
	Ask the user for a string
	Ask the user to chose between a list of choices
 */

static void Finish( gpointer dlg ) {
    int timer_id;
    timer_id = (intptr_t) g_object_get_data(G_OBJECT(dlg),"timer");
    g_source_destroy(g_main_context_find_source_by_id(g_main_context_default(),
	    timer_id));
    gtk_widget_destroy(dlg);
}

static int TimerFinish( gpointer dlg ) {
    gtk_widget_destroy(dlg);
return( FALSE );			/* End timer */
}

void gwwv_post_notice(const char *title, const char *msg, ... ) {
    char buffer[400];
    va_list va;
    GtkWidget *dlg, *lab;
    int timer_id;

    va_start(va,msg);
    vsnprintf( buffer, sizeof(buffer), msg, va);
    va_end(va);

    dlg = gtk_dialog_new_with_buttons(title,NULL,0,
	    GTK_STOCK_OK,GTK_RESPONSE_ACCEPT,NULL);
    g_signal_connect_swapped(G_OBJECT(dlg),"key-press-event",
	    G_CALLBACK(Finish), G_OBJECT(dlg));
    g_signal_connect_swapped(G_OBJECT(dlg),"button-press-event",
	    G_CALLBACK(Finish), G_OBJECT(dlg));
    g_signal_connect_swapped(G_OBJECT(dlg),"response",
	    G_CALLBACK(Finish), G_OBJECT(dlg));
    timer_id = gtk_timeout_add(30*1000,TimerFinish,dlg);
    g_object_set_data(G_OBJECT(dlg),"timer",(void *) timer_id);

    lab = gtk_label_new_with_mnemonic(buffer);
    gtk_widget_show(lab);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dlg)->vbox),lab);

    gtk_widget_show(dlg);
    /* Don't wait for it */
}

void gwwv_post_error(const char *title, const char *msg, ... ) {
    char buffer[400];
    va_list va;
    GtkWidget *dlg, *lab;

    va_start(va,msg);
    vsnprintf( buffer, sizeof(buffer), msg, va);
    va_end(va);

    dlg = gtk_dialog_new_with_buttons(title,NULL,GTK_DIALOG_DESTROY_WITH_PARENT,
	    GTK_STOCK_OK,GTK_RESPONSE_ACCEPT,NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dlg),GTK_RESPONSE_ACCEPT);

    lab = gtk_label_new_with_mnemonic(buffer);
    gtk_widget_show(lab);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dlg)->vbox),lab);

    (void) gtk_dialog_run(GTK_DIALOG(dlg));
}

void gwwv_ierror(const char *msg, ... ) {
    char buffer[400];
    int len;
    va_list va;
    GtkWidget *dlg, *lab;

    va_start(va,msg);
    strcpy(buffer, _("Internal Error: "));
    len = strlen(buffer);
    vsnprintf( buffer+len, sizeof(buffer)-len, msg, va);
    va_end(va);

    dlg = gtk_dialog_new_with_buttons(_("Internal Error"),NULL,GTK_DIALOG_DESTROY_WITH_PARENT,
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
    vsnprintf( buffer, sizeof(buffer), msg, va);
    va_end(va);

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
    vsnprintf( buffer, sizeof(buffer), question, va);
    va_end(va);

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

static void select_me(gpointer *data,gpointer *_sel) {
    char *sel = (char *) _sel;
    sel[gtk_tree_path_get_indices((GtkTreePath *) data)[0]] = TRUE;
}

static int _gwwv_choose_with_buttons(const char *title,
	const char **choices, char *sel, int cnt, int def,
	const char *butnames[2], const char *msg, va_list va ) {
    char buffer[400];
    GtkWidget *dlg, *lab, *list;
    GtkListStore *model;
    int i, result, ret;
    GtkTreeIter iter;
    GtkTreeSelection *select;

    vsnprintf( buffer, sizeof(buffer), msg, va);
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
    if ( sel==NULL ) {
	gtk_tree_selection_set_mode( select, GTK_SELECTION_SINGLE);
	if ( def>=0 ) {
	    GtkTreePath *path = gtk_tree_path_new_from_indices(def,-1);
	    /* I don't understand this. If I use selection_single, I can't select */
	    /*  anything. If I use multiple it does what I expect single to do */
	    gtk_tree_selection_select_path(select,path);
	    gtk_tree_view_set_cursor(GTK_TREE_VIEW(list),path,NULL,FALSE);
	    gtk_tree_path_free(path);
	}
    } else {
	gtk_tree_selection_set_mode( select, GTK_SELECTION_MULTIPLE);
	for ( i=0; i<cnt; ++i ) if ( sel[i] ) {
	    GtkTreePath *path = gtk_tree_path_new_from_indices(i,-1);
	    gtk_tree_selection_select_path(select,path);
	    gtk_tree_view_set_cursor(GTK_TREE_VIEW(list),path,NULL,FALSE);
	    gtk_tree_path_free(path);
	}
    }
    gtk_widget_show(list);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dlg)->vbox),list);

    result = gtk_dialog_run(GTK_DIALOG(dlg));
    if ( result==GTK_RESPONSE_REJECT || result==GTK_RESPONSE_NONE || result==GTK_RESPONSE_DELETE_EVENT)
	ret = -1;
    else {
	GList *endsel = gtk_tree_selection_get_selected_rows(select,NULL);
	if ( endsel==NULL )
	    ret = -1;
	else {
	    if ( sel==NULL ) {
		ret = gtk_tree_path_get_indices(endsel->data)[0];
	    } else {
		for ( i=0; i<cnt; ++i ) sel[i] = FALSE;
		g_list_foreach( endsel, (GFunc) select_me, sel);
		ret = 0;
	    }
	    g_list_foreach( endsel, (GFunc) gtk_tree_path_free, NULL);
	    g_list_free( endsel );
	}
    }
    gtk_widget_destroy(dlg);

return( ret );
}

int gwwv_choose_with_buttons(const char *title,
	const char **choices, int cnt, int def,
	const char *butnames[2], const char *msg, ... ) {
    va_list va;
    int ret;

    va_start(va,msg);
    ret = _gwwv_choose_with_buttons(title,choices,NULL,cnt,def,butnames,msg,va);
    va_end(va);
return( ret );
}

int gwwv_choose(const char *title,
	const char **choices, int cnt, int def,
	const char *msg, ... ) {
    static const char *buts[] = { GTK_STOCK_OK, GTK_STOCK_CANCEL, NULL };
    va_list va;
    int ret;

    va_start(va,msg);
    ret = _gwwv_choose_with_buttons(title,choices,NULL,cnt,def,buts,msg,va);
    va_end(va);
return( ret );
}

int gwwv_choose_multiple(char *title,
	const char **choices, char *sel, int cnt, char **buts,
	const char *msg, ... ) {
    va_list va;
    int ret;

    va_start(va,msg);
    ret = _gwwv_choose_with_buttons(title,choices,sel,cnt,-1,(const char **) buts,msg,va);
    va_end(va);
return( ret );
}

/* *********************** FILE CHOOSER ROUTINES **************************** */

/* gtk's default filter function doesn't handle "{}" wildcards, so... */
/* this one handles *?{}[] wildcards */
int gwwv_wild_match(char *pattern, const char *name,int ignorecase) {
    char ch, *ppt, *ept;
    const char *npt;

    if ( pattern==NULL )
return( TRUE );

    while (( ch = *pattern)!='\0' ) {
	if ( ch=='*' ) {
	    for ( npt=name; ; ++npt ) {
		if ( gwwv_wild_match(pattern+1,npt,ignorecase))
return( TRUE );
		if ( *npt=='\0' )
return( FALSE );
	    }
	} else if ( ch=='?' ) {
	    if ( *name=='\0' )
return( FALSE );
	    ++name;
	} else if ( ch=='[' ) {
	    /* [<char>...] matches the chars
	    /* [<char>-<char>...] matches any char within the range (inclusive)
	    /* the above may be concattenated and the resultant pattern matches
	    /*		anything thing which matches any of them.
	    /* [^<char>...] matches any char which does not match the rest of
	    /*		the pattern
	    /* []...] as a special case a ']' immediately after the '[' matches
	    /*		itself and does not end the pattern */
	    int found = 0, not=0;
	    ++pattern;
	    if ( pattern[0]=='^' ) { not = 1; ++pattern; }
	    for ( ppt = pattern; (ppt!=pattern || *ppt!=']') && *ppt!='\0' ; ++ppt ) {
		ch = *ppt;
		if ( ppt[1]=='-' && ppt[2]!=']' && ppt[2]!='\0' ) {
		    char ch2 = ppt[2];
		    if ( (*name>=ch && *name<=ch2) ||
			    (ignorecase && islower(ch) && islower(ch2) &&
				    *name>=toupper(ch) && *name<=toupper(ch2)) ||
			    (ignorecase && isupper(ch) && isupper(ch2) &&
				    *name>=tolower(ch) && *name<=tolower(ch2))) {
			if ( !not ) {
			    found = 1;
	    break;
			}
		    } else {
			if ( not ) {
			    found = 1;
	    break;
			}
		    }
		    ppt += 2;
		} else if ( ch==*name || (ignorecase && tolower(ch)==tolower(*name)) ) {
		    if ( !not ) {
			found = 1;
	    break;
		    }
		} else {
		    if ( not ) {
			found = 1;
	    break;
		    }
		}
	    }
	    if ( !found )
return( FALSE );
	    while ( *ppt!=']' && *ppt!='\0' ) ++ppt;
	    pattern = ppt;
	    ++name;
	} else if ( ch=='{' ) {
	    /* matches any of a comma separated list of substrings */
	    for ( ppt = pattern+1; *ppt!='\0' ; ppt = ept ) {
		for ( ept=ppt; *ept!='}' && *ept!=',' && *ept!='\0'; ++ept );
		for ( npt = name; ppt<ept; ++npt, ++ppt ) {
		    if ( *ppt != *npt && (!ignorecase || tolower(*ppt)!=tolower(*npt)) )
		break;
		}
		if ( ppt==ept ) {
		    char *ecurly = ept;
		    while ( *ecurly!='}' && *ecurly!='\0' ) ++ecurly;
		    if ( gwwv_wild_match(ecurly+1,npt,ignorecase))
return( TRUE );
		}
		if ( *ept=='}' )
return( FALSE );
		if ( *ept==',' ) ++ept;
	    }
	} else if ( ch==*name ) {
	    ++name;
	} else if ( ignorecase && tolower(ch)==tolower(*name)) {
	    ++name;
	} else
return( FALSE );
	++pattern;
    }
    if ( *name=='\0' )
return( TRUE );

return( FALSE );
}

static gboolean gwwv_file_pattern_matcher(const GtkFileFilterInfo *info,
	gpointer data) {
    char *pattern = (char *) data;
return( gwwv_wild_match(pattern, info->filename, TRUE));
}

static void gwwv_file_def_filters(GtkWidget *dialog, const char *def_name,
	const struct gwwv_filter *filters ) {
    GtkFileFilter *filter, *standard;
    int i;

    g_object_set(G_OBJECT(dialog),"show-hidden",TRUE,NULL);
    if ( def_name!=NULL ) {
	gsize read, written;
	char *temp = g_filename_from_utf8(def_name,-1,&read,&written,NULL);
	char *pt = strrchr( temp,'/');
	if ( pt!=NULL ) {
	    *pt = '\0';
	    gtk_file_chooser_set_current_folder( GTK_FILE_CHOOSER( dialog ), temp );
	    gtk_file_chooser_set_current_name( GTK_FILE_CHOOSER( dialog ), pt+1 );
	} else
	    gtk_file_chooser_set_current_name( GTK_FILE_CHOOSER( dialog ), temp );
	free(temp);
    }
    if ( filters!=NULL ) {
	standard = NULL;
	for ( i=0; filters[i].name!=NULL; ++i ) {
	    filter = gtk_file_filter_new();
	    if ( filters[i].filtfunc!=NULL )
		gtk_file_filter_add_custom( filter,GTK_FILE_FILTER_FILENAME,
			filters[i].filtfunc, (gpointer) filters[i].wild, NULL);
	    else
		/* GTK's gtk_file_filter_add_pattern doesn't handled {} wildcards */
		/*  and I depend on those. So I use my own patern matcher */
		gtk_file_filter_add_custom( filter,GTK_FILE_FILTER_FILENAME,
			gwwv_file_pattern_matcher, (gpointer) filters[i].wild, NULL);
	    gtk_file_filter_set_name( filter,filters[i].name );
	    gtk_file_chooser_add_filter( GTK_FILE_CHOOSER( dialog ), filter );
	    if ( i==0 )
		standard = filter;
	}
	if ( standard!=NULL )
	    gtk_file_chooser_set_filter( GTK_FILE_CHOOSER( dialog ), standard );
    }
}

char *gwwv_open_filename_mult(const char *title, const char *def_name,
	const struct gwwv_filter *filters, int mult ) {
    GtkWidget *dialog;
    char *filename = NULL;
    gsize read, written;

    if ( mult )
	dialog = gtk_file_chooser_dialog_new (title,
					      NULL,
					      GTK_FILE_CHOOSER_ACTION_OPEN,
					      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					      GTK_STOCK_NEW, -100,
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      NULL);
    else
	dialog = gtk_file_chooser_dialog_new (title,
					      NULL,
					      GTK_FILE_CHOOSER_ACTION_OPEN,
					      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      NULL);
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER( dialog ), mult );
    gwwv_file_def_filters(dialog,def_name,filters);

    filename = NULL;
    if ( gtk_dialog_run(GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT ) {
	char *temp = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
	filename = g_filename_to_utf8(temp,-1,&read,&written,NULL);
	free(temp);
    }

    gtk_widget_destroy (dialog);
return( filename );
}

char *gwwv_save_filename_with_gadget(const char *title, const char *def_name,
	const struct gwwv_filter *filters, GtkWidget *extra ) {
    GtkWidget *dialog;
    char *filename = NULL;
    gsize read, written;
    dialog = gtk_file_chooser_dialog_new (title,
					  NULL,
					  GTK_FILE_CHOOSER_ACTION_SAVE,
					  GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					  NULL);
    gwwv_file_def_filters(dialog,def_name,filters);
    if ( extra != NULL )
	gtk_file_chooser_set_extra_widget( GTK_FILE_CHOOSER( dialog ), extra );

    filename = NULL;
    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
	char *temp = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
	filename = g_filename_to_utf8(temp,-1,&read,&written,NULL);
	free(temp);
    }

    gtk_widget_destroy (dialog);
return( filename );
}

char *gwwv_open_filename(const char *title, const char *def_name,
	const char *filter) {
    struct gwwv_filter filters[3];

    if ( filter==NULL )
return( gwwv_open_filename_mult(title,def_name,NULL,FALSE));
    memset(filters,0,sizeof(filters));
    filters[0].name = _("Default");
    filters[0].wild = (char *) filter;
    filters[1].name = _("All");
    filters[1].wild = "*";
return( gwwv_open_filename_mult(title,def_name,filters,FALSE));
}
    
char *gwwv_saveas_filename(const char *title, const char *def_name,
	const char *filter) {
    struct gwwv_filter filters[3];

    if ( filter==NULL )
return( gwwv_save_filename_with_gadget(title,def_name,NULL,FALSE));
    memset(filters,0,sizeof(filters));
    filters[0].name = _("Default");
    filters[0].wild = (char *) filter;
    filters[1].name = _("All");
    filters[1].wild = "*";
return( gwwv_save_filename_with_gadget(title,def_name,filters,FALSE));
}
