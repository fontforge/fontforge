/* Copyright (C) 2008 by George Williams */
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
#include "fontforgegtk.h"
# include <gfile.h>
# include <ustring.h>
# include <gtk/gtk.h>
# include <gdk/gdkkeysyms.h>
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

extern NameList *force_names_when_opening;
int default_font_filter_index=0;
struct gwwv_filter *user_font_filters = NULL;

struct gwwv_filter def_font_filters[] = {
	{ N_("All Fonts"), "*.{"
	   "pfa,"
	   "pfb,"
	   "pt3,"
	   "t42,"
	   "sfd,"
	   "ttf,"
	   "bdf,"
	   "otf,"
	   "otb,"
	   "cff,"
	   "cef,"
	   "gai,"
#ifndef _NO_LIBXML
	   "svg,"
	   "ufo,"
#endif
	   "pf3,"
	   "ttc,"
	   "gsf,"
	   "cid,"
	   "bin,"
	   "hqx,"
	   "dfont,"
	   "mf,"
	   "ik,"
	   "fon,"
	   "fnt,"
	   "pcf,"
	   "pmf,"
	   "*pk,"
	   "*gf,"
	   "pdb"
	   "}"
/* With any of these methods of compression */
	     "{.gz,.Z,.bz2,}"},
	{ N_("Outline Fonts"), "*.{"
	   "pfa,"
	   "pfb,"
	   "pt3,"
	   "t42,"
	   "sfd,"
	   "ttf,"
	   "otf,"
	   "cff,"
	   "cef,"
	   "gai,"
#ifndef _NO_LIBXML
	   "svg,"
	   "ufo,"
#endif
	   "pf3,"
	   "ttc,"
	   "gsf,"
	   "cid,"
	   "bin,"
	   "hqx,"
	   "dfont,"
	   "mf,"
	   "ik"
	   "}"
	     "{.gz,.Z,.bz2,}"},
	{ N_("Bitmap Fonts"), "*.{"
	   "bdf,"
	   "otb,"
	   "bin,"
	   "hqx,"
	   "fon,"
	   "fnt,"
	   "pcf,"
	   "pmf,"
	   "*pk,"
	   "*gf,"
	   "pdb"
	   "}"
	     "{.gz,.Z,.bz2,}"},
	{NU_("ΤεΧ Bitmap Fonts"), "*{pk,gf}"},
	{N_("PostScript"), "*.{pfa,pfb,t42,otf,cef,cff,gai,pf3,pt3,gsf,cid}{.gz,.Z,.bz2,}"},
	{N_("TrueType"), "*.{ttf,t42,ttc}{.gz,.Z,.bz2,}"},
	{N_("OpenType"), "*.{ttf,otf}{.gz,.Z,.bz2,}"},
	{N_("Type1"), "*.{pfa,pfb,gsf,cid}{.gz,.Z,.bz2,}"},
	{N_("Type2"), "*.{otf,cef,cff,gai}{.gz,.Z,.bz2,}"},
	{N_("Type3"), "*.{pf3,pt3}{.gz,.Z,.bz2,}"},
#ifndef _NO_LIBXML
	{N_("SVG"), "*.svg{.gz,.Z,.bz2,}"},
#endif
	{N_("FontForge's SFD"), "*.sfd{.gz,.Z,.bz2,}"},
	{N_("Backup SFD"), "*.sfd~"},
	{N_("Extract from PDF"), "*.pdf{.gz,.Z,.bz2,}"},
	{"  ----  ", ""},
	{N_("All Files"), "*"},
	 NULL };

static gboolean gwwv_file_pattern_matcher(const GtkFileFilterInfo *info,
	gpointer data) {
    char *pattern = (char *) data;
return( gwwv_wild_match(pattern, info->filename, TRUE));
}

static void find_fonts_callback(GtkFileChooser *dialog, gpointer tooltip) {
    GtkTooltips *tooltips = (GtkTooltips *) tooltip;
    GSList *files = gtk_file_chooser_get_filenames(dialog);
    GSList *test;
    int len, cnt, i;
    char ***fonts, *pt, *text;

    if ( files==NULL || (cnt = g_slist_length(files))==0 )
	gtk_tooltips_set_tip(tooltips,GTK_WIDGET(dialog),"","");
    else {
	fonts = gcalloc(cnt,sizeof(char **));
	cnt = len = 0;
	for ( test=files; test!=NULL; test=test->next, ++cnt) {
	    fonts[cnt] = GetFontNames((char *) (test->data));
	    if ( fonts[cnt]!=NULL ) {
		len += 4*strlen((char *) (test->data))+1;	/* allow space for utf8 conversion */
		for ( i=0; fonts[cnt][i]!=NULL; ++i )
		    len += strlen( fonts[cnt][i])+2;
	    }
	}
	pt = text = galloc(len+10);
	cnt = 0;
	for ( test=files; test!=NULL; test=test->next, ++cnt) {
	    if ( fonts[cnt]!=NULL ) {
		/* If there is only one file selected, don't bother to name it */
		if ( cnt!=0 || test->next!=NULL ) {
		    gsize junk;
		    char *temp = g_filename_to_utf8(GFileNameTail( (char *) (test->data) ),
			    -1, &junk, &junk, NULL);
		    strcpy(pt,temp);
		    g_free(temp);
		    pt += strlen(pt);
		    *pt++ = '\n';
		}
		for ( i=0; fonts[cnt][i]!=NULL; ++i ) {
		    *pt++ = ' ';
		    strcpy(pt,fonts[cnt][i]);
		    free(fonts[cnt][i]);
		    pt += strlen(pt);
		    *pt ++ = '\n';
		}
		free(fonts[cnt]);
	    }
	}
	if ( pt>text && pt[-1]=='\n' )
	    pt[-1]='\0';
	else
	    *pt = '\0';
	free(fonts);
	if ( *text=='\0' )
	    gtk_tooltips_set_tip( tooltips, GTK_WIDGET(dialog), "???", NULL );
	else {
	    gtk_tooltips_set_tip( tooltips, GTK_WIDGET(dialog), text, NULL );
	}
	free(text);
    }

    for ( test=files; test!=NULL; test=test->next)
	g_free(test->data);
    g_slist_free(files);
}

static void RememberCurrentFilter(GtkFileFilter *filter) {
    int i, k;
    const char *name;

    if ( filter==NULL )
return;
    name = gtk_file_filter_get_name(filter);
    if ( name==NULL )
return;

    for ( i=0; def_font_filters[i].name!=NULL; ++i ) {
	if ( strcmp(name,_(def_font_filters[i].name))== 0 ) {
	    if ( *def_font_filters[i].wild!='\0' )
		default_font_filter_index = i;
return;
	}
    }
    if ( user_font_filters!=NULL ) {
	for ( k=0; user_font_filters[k].name!=NULL; ++i, ++k ) {
	    if ( strcmp(name,_(user_font_filters[k].name))== 0 ) {
		default_font_filter_index = k;
return;
	    }
	}
    }
}

char *FVOpenFont(const char *title, const char *def_name, int mult ) {
    GtkWidget *dialog;
    char *filename = NULL;
    GtkFileFilter *filter, *standard, *first;
    int ans, i,k;
    GtkTooltips *tips;

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
    g_object_set(G_OBJECT(dialog),"show-hidden",TRUE,NULL);
    if ( def_name!=NULL ) {
	char *pt = strrchr( def_name,'/');
	if ( pt!=NULL ) {
	    char *dir = copyn(def_name,pt-def_name);
	    gtk_file_chooser_set_current_folder( GTK_FILE_CHOOSER( dialog ), dir );
	    gtk_file_chooser_set_current_name( GTK_FILE_CHOOSER( dialog ), pt+1 );
	    free(dir);
	} else
	    gtk_file_chooser_set_current_name( GTK_FILE_CHOOSER( dialog ), def_name );
    }
    standard = first = NULL;
    for ( i=0; def_font_filters[i].name!=NULL; ++i ) {
	filter = gtk_file_filter_new();
	/* GTK's gtk_file_filter_add_pattern doesn't handled {} wildcards */
	/*  and I depend on those. So I use my own patern matcher */
	gtk_file_filter_add_custom( filter,GTK_FILE_FILTER_FILENAME,
		gwwv_file_pattern_matcher, (gpointer) def_font_filters[i].wild, NULL);
	gtk_file_filter_set_name( filter,_(def_font_filters[i].name) );
	gtk_file_chooser_add_filter( GTK_FILE_CHOOSER( dialog ), filter );
	if ( i==default_font_filter_index )
	    standard = filter;
	if ( first==NULL )
	    first = filter;
    }
    if ( user_font_filters!=NULL ) {
	for ( k=0; user_font_filters[k].name!=NULL; ++i, ++k ) {
	    filter = gtk_file_filter_new();
	    gtk_file_filter_add_custom( filter,GTK_FILE_FILTER_FILENAME,
		    gwwv_file_pattern_matcher, (gpointer) user_font_filters[k].wild, NULL);
	    gtk_file_filter_set_name( filter,_(user_font_filters[k].name) );
	    gtk_file_chooser_add_filter( GTK_FILE_CHOOSER( dialog ), filter );
	    if ( i==default_font_filter_index )
		standard = filter;
	}
    }
    if ( standard==NULL )
	standard = first;
    if ( standard!=NULL )
	gtk_file_chooser_set_filter( GTK_FILE_CHOOSER( dialog ), standard );

    /* New Filter button, Recent files list */

    if ( RecentFiles[0]!=NULL ) {
    }

    g_signal_connect (G_OBJECT(dialog), "selection-changed",
                    G_CALLBACK (find_fonts_callback),
                    (tips = gtk_tooltips_new()) );

    filename = NULL;
    ans = gtk_dialog_run (GTK_DIALOG (dialog));
    if ( ans == GTK_RESPONSE_ACCEPT) {
	filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
	RememberCurrentFilter(gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(dialog)));
    } else if ( ans == -100 )
	FontNew();		/* And return NULL */

    gtk_widget_destroy (dialog);
    g_object_unref( G_OBJECT(tips));
return( filename );
}
