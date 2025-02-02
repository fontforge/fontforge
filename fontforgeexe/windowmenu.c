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

#include "basics.h"
#include "fontforgeui.h"
#include "gfile.h"
#include "scripting.h"
#include "splinefont.h"
#include "ustring.h"

#include "gtk/c_context.h"

static void WindowSelect(GWindow base,struct gmenuitem *mi,GEvent *e) {
    GDrawRaise(mi->ti.userdata);
}

static void AddMI(GMenuItem *mi,GWindow gw,int changed, int top) {
    char* title = GDrawGetWindowTitle8(gw);
    mi->ti.userdata = gw;
    mi->ti.bg = GDrawGetDefaultBackground(GDrawGetDisplayOfWindow(gw));
    mi->invoke = WindowSelect;
    mi->ti.text = utf82u_copy(title);
    if(mi->ti.text == NULL)
	mi->ti.text = utf82u_copy("(null)");
    if ( u_strlen( mi->ti.text ) > 35 )
	mi->ti.text[35] = '\0';
    free(title);
}

/* Builds up a menu containing the titles of all the major windows */
void WindowMenuBuild(GWindow basew,struct gmenuitem *mi,GEvent *e) {
    int i, cnt, precnt;
    FontViewBase *fv;
    CharViewBase *cv;
    MetricsView *mv;
    BitmapView *bv;
    GMenuItem *sub;
    BDFFont *bdf;

    precnt = 6;
    cnt = precnt;

    for ( fv = (FontViewBase *) fv_list; fv!=NULL; fv = fv->next ) {
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
    sub = calloc(cnt+1,sizeof(GMenuItem));
    memcpy(sub,mi->sub,precnt*sizeof(struct gmenuitem));
    for ( i=0; i<precnt; ++i )
	mi->sub[i].ti.text = NULL;
    GMenuItemArrayFree(mi->sub);
    mi->sub = sub;

    for ( i=0; sub[i].ti.text!=NULL || sub[i].ti.line; ++i ) {
	if ( sub[i].ti.text_is_1byte ) {
	    sub[i].ti.text = utf82u_mncopy((char *) sub[i].ti.text,&sub[i].ti.mnemonic);
	} else
	    sub[i].ti.text = u_copy(sub[i].ti.text);
	sub[i].ti.text_is_1byte = sub[i].ti.text_in_resource = false;
    }
    cnt = precnt;
    for ( fv = (FontViewBase *) fv_list; fv!=NULL; fv = fv->next ) {
	if( !((FontView *) fv)->gw ) {
	    continue;
	}
	
	AddMI(&sub[cnt++],((FontView *) fv)->gw,fv->sf->changed,true);
	for ( i=0; i<fv->sf->glyphcnt; ++i ) if ( fv->sf->glyphs[i]!=NULL ) {
	    for ( cv = fv->sf->glyphs[i]->views; cv!=NULL; cv=cv->next )
		AddMI(&sub[cnt++],((CharView *) cv)->gw,cv->sc->changed,false);
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
    ViewPostScriptFont((char *) (mi->ti.userdata),0);
}

/* Builds up a menu containing the titles of all the unused recent files */
void MenuRecentBuild(GWindow base,struct gmenuitem *mi,GEvent *e) {
    char** recent_files_array = NULL;
    int i, n_recent = collect_recent_files(&recent_files_array);
    GMenuItem *sub;

    if ( mi->sub!=NULL ) {
	GMenuItemArrayFree(mi->sub);
	mi->sub = NULL;
    }

    sub = calloc(n_recent+1,sizeof(GMenuItem));
    for ( i=0; i<n_recent; ++i ) {
	GMenuItem *mi = &sub[i];
	mi->ti.userdata = recent_files_array[i];
	mi->ti.bg = mi->ti.fg = COLOR_DEFAULT;
	mi->invoke = RecentSelect;
	mi->ti.text = def2u_copy(GFileNameTail(recent_files_array[i]));
    }

    mi->sub = sub;
}

unsigned int collect_recent_files(char*** recent_files_array) {
    int i, n_recent, cnt1;
    FontViewBase *fv;

    n_recent = 0;
    for ( i=0; i<RECENT_MAX && RecentFiles[i]!=NULL; ++i ) {
	for ( fv=(FontViewBase *) fv_list; fv!=NULL; fv=fv->next )
	    if ( fv->sf->filename!=NULL && strcmp(fv->sf->filename,RecentFiles[i])==0 )
	break;
	if ( fv==NULL )
	    ++n_recent;
    }

    *recent_files_array = calloc(n_recent,sizeof(char*));
    cnt1 = 0;
    for ( i=0; i<RECENT_MAX && RecentFiles[i]!=NULL; ++i ) {
	for ( fv=(FontViewBase *) fv_list; fv!=NULL; fv=fv->next )
	    if ( fv->sf->filename!=NULL && strcmp(fv->sf->filename,RecentFiles[i])==0 )
	break;
	if ( fv==NULL ) {
	    (*recent_files_array)[cnt1++] = RecentFiles[i];
	}
    }
    if ( n_recent!=cnt1 )
	IError( "Bad counts in MenuRecentBuild");

    return n_recent;
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

#if !defined(_NO_FFSCRIPT) || !defined(_NO_PYTHON)
static void ScriptSelect(GWindow base,struct gmenuitem *mi,GEvent *e) {
    int index = (intptr_t) (mi->ti.userdata);
    FontView *fv = (FontView *) GDrawGetUserData(base);

    /* the menu is not always up to date. If user changed prefs and then used */
    /*  Alt|Ctl|Digit s/he would not get a new menu built and the old one might*/
    /*  refer to something out of bounds. Hence the check */
    if ( index<0 || script_filenames[index]==NULL )
return;
    ExecuteScriptFile((FontViewBase *) fv,NULL,script_filenames[index]);
}

void script_run(FontView *fv, int index) {
    /* the menu is not always up to date. If user changed prefs and then used */
    /*  Alt|Ctl|Digit s/he would not get a new menu built and the old one might*/
    /*  refer to something out of bounds. Hence the check */
    if ( index<0 || script_filenames[index]==NULL )
return;
    ExecuteScriptFile((FontViewBase *) fv,NULL,script_filenames[index]);
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
    sub = calloc(i+1,sizeof(GMenuItem));
    for ( i=0; i<SCRIPT_MENU_MAX && script_menu_names[i]!=NULL; ++i ) {
	GMenuItem *mi = &sub[i];
	mi->ti.userdata = (void *) (intptr_t) i;
	mi->ti.bg = mi->ti.fg = COLOR_DEFAULT;
	mi->invoke = ScriptSelect;
	mi->shortcut = i==9?'0':'1'+i;
	mi->short_mask = ksm_control|ksm_meta;
	mi->ti.text = u_copy(script_menu_names[i]);
    }
    mi->sub = sub;
}

unsigned int collect_script_names(char*** script_names_array) {
    int i, n_scripts;

    for ( i=0; i<SCRIPT_MENU_MAX && script_menu_names[i]!=NULL; ++i );
    n_scripts = i;

    *script_names_array = calloc(n_scripts,sizeof(char*));
    for ( i=0; i<n_scripts; ++i ) {
	(*script_names_array)[i] = u2utf8_copy(script_menu_names[i]);
    }

    return n_scripts;
}
#endif

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
    sub = calloc(cnt+1,sizeof(GMenuItem));
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

unsigned int collect_anchor_data(FontView *fv, AnchorMenuData** anchor_data_array) {
    unsigned int i, n_anchors = 0;
    SplineFont *sf = fv->b.sf;
    AnchorClass *ac;

    /* Cound the available anchors */
    for ( ac = sf->anchor; ac!=NULL; ++n_anchors, ac=ac->next );
        
    *anchor_data_array = calloc(n_anchors, sizeof(AnchorMenuData));
    for ( ac = sf->anchor, i = 0; ac!=NULL; ++i, ac=ac->next ) {
        (*anchor_data_array)[i].label = ac->name;
        (*anchor_data_array)[i].ac = ac;
    }

    return n_anchors;
}

void mbFreeGetText(GMenuItem *mb) {
    /* free gettext substitutions on this menu and all sub menus */
    int i;

    if ( mb==NULL )
return;
    for ( i=0; mb[i].ti.text!=NULL || mb[i].ti.line || mb[i].ti.image!=NULL; ++i ) {
        if (mb[i].ti.text_untranslated != NULL) { free(mb[i].ti.text_untranslated); mb[i].ti.text_untranslated = NULL; }
	if ( mb[i].ti.text!=NULL ) {
	    if ( mb[i].sub!=NULL )
		mbFreeGetText(mb[i].sub);
	}
    }
}

void mbDoGetText(GMenuItem *mb) {
    /* perform gettext substitutions on this menu and all sub menus */
    int i;

    if ( mb==NULL )
return;
    for ( i=0; mb[i].ti.text!=NULL || mb[i].ti.line || mb[i].ti.image!=NULL; ++i ) {
	if( mb[i].shortcut ) {
	    unichar_t tmp[2];
	    tmp[0] = mb[i].shortcut;
	    tmp[1] = (unichar_t)(0);
	    mb[i].ti.text_untranslated = cu_copy(tmp);
	} else
	    mb[i].ti.text_untranslated = copy((char*)mb[i].ti.text);
	if ( mb[i].ti.text!=NULL ) {
	    mb[i].ti.text = (unichar_t *) S_((char *) mb[i].ti.text);
	    if ( mb[i].sub!=NULL )
		mbDoGetText(mb[i].sub);
	}
    }
}

void mb2FreeGetText(GMenuItem2 *mb) {
    /* free gettext substitutions on this menu and all sub menus */
    int i;

    if ( mb==NULL )
return;
    for ( i=0; mb[i].ti.text!=NULL || mb[i].ti.line || mb[i].ti.image!=NULL; ++i ) {
	if (mb[i].ti.text_untranslated != NULL) { free(mb[i].ti.text_untranslated); mb[i].ti.text_untranslated = NULL; }
	if ( mb[i].ti.text!=NULL ) {
	    if ( mb[i].sub!=NULL )
		mb2FreeGetText(mb[i].sub);
	}
    }
}

void mb2DoGetText(GMenuItem2 *mb) {
    /* perform gettext substitutions on this menu and all sub menus */
    int i;

    if ( mb==NULL )
return;
    for ( i=0; mb[i].ti.text!=NULL || mb[i].ti.line || mb[i].ti.image!=NULL; ++i ) {
	if( mb[i].shortcut )
	    mb[i].ti.text_untranslated = copy(mb[i].shortcut);
	else
	    mb[i].ti.text_untranslated = copy((char*)mb[i].ti.text);
	if ( mb[i].ti.text!=NULL ) {
	    mb[i].ti.text = (unichar_t *) S_((char *) mb[i].ti.text);
	    if ( mb[i].sub!=NULL )
		mb2DoGetText(mb[i].sub);
	}
    }
}
