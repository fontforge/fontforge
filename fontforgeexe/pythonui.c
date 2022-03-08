/* Copyright (C) 2007-2012 by George Williams */
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
/*			   Python Interface to FontForge		      */

#include <fontforge-config.h>

#ifndef _NO_PYTHON

#include "cvundoes.h"
#include "ffglib.h"
#include "ffpython.h"
#include "fontforgeui.h"
#include "hotkeys.h"
#include "scriptfuncs.h"
#include "scripting.h"
#include "splinestroke.h"
#include "splineutil.h"
#include "ttf.h"
#include "ustring.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

/* The (messy) menu name situation:
 *
 * The updated interface encourages the caller to pass three names for each
 * menu: a localized name, an untranslated name, and an identifier string.
 * (With luck) the latter is for future use when menus are configurable, so
 * that a user can choose to put a registered menu item anywhere in the
 * hierarchy without worrying about the "registration name".
 *
 * The complexity with the other two names relates to mnemonics. The names
 * might come in any of the following forms in any combination:
 *
 * Cancel         Annuler
 * _Cancel        _Annuler
 * Cancel (_1)    キャンセル(_C)
 *
 * (A parenthetical in the English form is unlikely, but possible if a
 * developer has run out of other letters or hopes to claim a less likely
 * mnemonic in a Tools menu.)
 *
 * We want the following:
 *
 * 1. Below the Tools menus a "_" specified mnemonic is honored.
 * 2. On the Tools menu the specified mnemonic is treated as a suggestion.
 * 3. A user can assign a hotkey to a menu item, similar to other menus.
 *
 * So at the level of the tools menu we would ideally register two strings, one
 * to display in the menu with its assigned mnemonic and one to match assigned
 * hotkeys, which are (normally) specified in English without mnemonics.
 *
 * Take the second line above. If "A" is available we want these assignments
 * (ignoring casting):
 *
 * mmn[j].ti.text              = "Annuler";
 * mmn[j].ti.text_untranslated = "Cancel";
 * mmn[j].ti.mnemonic          = 'A';
 *
 * If "A" is not available but "u" is, we could do the same but switch the
 * mnemonic to 'u'. If none of the French letters are available we might
 * wind up with:
 *
 * mmn[j].ti.text              = "Annuler (4)";
 * mmn[j].ti.text_untranslated = "Cancel";
 * mmn[j].ti.mnemonic          = '4';
 *
 * If we only have one set of strings we don't know whether it is localized,
 * so we need to do:
 *
 * mmn[j].ti.text              = "Annuler (4)";
 * mmn[j].ti.text_untranslated = "Annuler";
 * mmn[j].ti.mnemonic          = '4';
 *
 * And, of course, if we *start* with "Annuler (_C)" or "キャンセル(_C)" and
 * "C" is not available we don't want to accidentally do
 *
 * mmn[j].ti.text              = "Annuler (C) (4)";
 * mmn[j].ti.text_untranslated = "Annuler (C)";
 * mmn[j].ti.mnemonic          = '4';
 *
 * so we need to strip off the parenthetical when reassigning (and hope that's
 * the convention the translator is using).
 *
 * ti.text is a (unichar_t *) but can be assigned a utf8 string when setting
 * ti.text_is_1byte to true. ti.text_untranslated is a (char *) that we just
 * have to hope can handle non-ASCII UTF-8 content, because that may be all we
 * have.
 */

static const char mn_order[] = NU_("}{:[]&=~%^;?+/,#$-@*!.7869054321QXZVJWFYGKUBHPCDMLTNSIROEA");

struct py_menu_item {
    PyObject *func;
    PyObject *check;		/* May be None (which I change to NULL) */
    PyObject *data;			/* May be None (left as None) */
};

typedef void (*ff_menu_callback)(GWindow gw, struct gmenuitem *mi, GEvent *e);

enum py_menu_type { pmt_font=0, pmt_char=1, pmt_size=2 };
enum py_menu_flag { pmf_font=1, pmf_char=2 };

static struct py_menu_data {
    struct py_menu_item *items;
    int cnt, max;
    GMenuItem2 *menu;
    ff_menu_callback moveto, invoke;
    void (*setmenu)(GMenuItem2 *menu);
    uint16_t mn_offset;
    unichar_t *mn_string;
    GHashTable *mn_avail;
    char *hotkey_prefix;
} *py_menus;

static unichar_t AllocateNextMnemonic(struct py_menu_data *pmd) {
    unichar_t uc = 0;
    while ( (uc=pmd->mn_string[pmd->mn_offset])!=0 &&
            !g_hash_table_contains(pmd->mn_avail, GUINT_TO_POINTER(uc)) )
	++pmd->mn_offset;

    if ( uc!=0 )
	g_hash_table_remove(pmd->mn_avail, GUINT_TO_POINTER(uc));

    return (unichar_t) uc;
}

static unichar_t FindMnemonic(unichar_t *menu_string, unichar_t alt,
                              struct py_menu_data *pmd) {
    unichar_t *cp;

    if ( g_hash_table_contains(pmd->mn_avail, GUINT_TO_POINTER(alt)) ) {
	g_hash_table_remove(pmd->mn_avail, GUINT_TO_POINTER(alt));
	return alt;
    }
    for ( cp = menu_string; *cp!=0; ++cp ) {
	alt = toupper(*cp);
	if ( g_hash_table_contains(pmd->mn_avail, GUINT_TO_POINTER(alt)) ) {
	    g_hash_table_remove(pmd->mn_avail, GUINT_TO_POINTER(alt));
	    return alt;
	}
    }
    return 0;
}

// Make sure there is no suffix when c==0 and exactly one suffix " (c)" when c!=0
static unichar_t *SetMnemonicSuffix(const unichar_t *menu_string, unichar_t e, unichar_t c) {
    int i;
    unichar_t *r;
    i = u_strlen(menu_string);
    r = malloc((i+5) * sizeof(unichar_t));
    memcpy(r, menu_string, sizeof(unichar_t)*(i+1));

    i--; // Make index of last character

    // Remove trailing spaces (if any)
    while ( i>=0 && r[i]==' ' )
	--i;
    r[i+1] = 0;
    // The following is intended to remove a parenthetical mnemonic
    // if one is already present in the string, or do nothing otherwise.
    // If the input is something less "standard" we just append the
    // new mnemonic character and hope for the best.
    if ( i>=0 && r[i]==')' ) {
	--i;
	if ( i>=0 && r[i]==e ) {
	    --i;
	    if ( i>=0 && r[i]=='(' ) {
		--i;
		while ( i>=0 && r[i]==' ' )
		    --i;
		r[i+1] = 0;
	    }
	}
    }

    // The calloc above ensures there is room for these characters
    if ( c!=0 ) {
	r[i+1] = ' ';
	r[i+2] = '(';
	r[i+3] = c;
	r[i+4] = ')';
	r[i+5] = 0;
    }
    return r;
}

static void py_tllistcheck(struct gmenuitem *mi, PyObject *owner, struct py_menu_data *pmd) {
    PyObject *arglist, *result;

    if ( mi == NULL )
	return;

    for ( mi = mi->sub; mi!=NULL && (mi->ti.text!=NULL || mi->ti.line); ++mi ) {
	if ( mi->mid==-1 )		/* Submenu */
	    continue;
	if ( mi->mid<0 || mi->mid>=pmd->cnt ) {
	    fprintf( stderr, "Bad Menu ID in python menu %d\n", mi->mid );
	    mi->ti.disabled = true;
	    continue;
	}
	if ( pmd->items[mi->mid].check==NULL ) {
	    mi->ti.disabled = false;
	    continue;
	}
	arglist = PyTuple_New(2);
	Py_XINCREF(pmd->items[mi->mid].data);
	Py_XINCREF(owner);
	PyTuple_SetItem(arglist,0,pmd->items[mi->mid].data);
	PyTuple_SetItem(arglist,1,owner);
	result = PyObject_CallObject(pmd->items[mi->mid].check, arglist);
	Py_DECREF(arglist);
	if ( result==NULL )
	    /* Oh. An error. How fun. See below */;
	else if ( !PyLong_Check(result)) {
	    char *menu_item_name = u2utf8_copy(mi->ti.text);
	    LogError(_("Return from enabling function for menu item %s must be boolean"), menu_item_name );
	    free( menu_item_name );
	    mi->ti.disabled = true;
	} else
	    mi->ti.disabled = PyLong_AsLong(result)==0;
	Py_XDECREF(result);
	if ( PyErr_Occurred()!=NULL )
	    PyErr_Print();
    }
}

static void py_menuactivate(struct gmenuitem *mi, PyObject *owner, struct py_menu_data *pmd) {
    PyObject *arglist, *result;

    if ( mi->mid==-1 )		/* Submenu */
	return;

    assert( py_menus!=NULL );

    if ( mi->mid<0 || mi->mid>=pmd->cnt ) {
	fprintf( stderr, "Bad Menu ID in python menu %d\n", mi->mid );
	return;
    }
    if ( pmd->items[mi->mid].func==NULL ) {
	return;
    }
    arglist = PyTuple_New(2);
    Py_XINCREF(pmd->items[mi->mid].data);
    Py_XINCREF(owner);
    PyTuple_SetItem(arglist,0,pmd->items[mi->mid].data);
    PyTuple_SetItem(arglist,1,owner);
    result = PyObject_CallObject(pmd->items[mi->mid].func, arglist);
    Py_DECREF(arglist);
    Py_XDECREF(result);
    if ( PyErr_Occurred()!=NULL )
	PyErr_Print();
}

void cvpy_tllistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    PyObject *pysc = PySC_From_SC(cv->b.sc);

    sc_active_in_ui = cv->b.sc;
    layer_active_in_ui = CVLayer((CharViewBase *) cv);
    PyFF_Glyph_Set_Layer(sc_active_in_ui,layer_active_in_ui);
    py_tllistcheck(mi,pysc,py_menus + pmt_char);
    sc_active_in_ui = NULL;
    layer_active_in_ui = ly_fore;
}

static void cvpy_menuactivate(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    PyObject *pysc = PySC_From_SC(cv->b.sc);

    sc_active_in_ui = cv->b.sc;
    layer_active_in_ui = CVLayer((CharViewBase *) cv);
    PyFF_Glyph_Set_Layer(sc_active_in_ui,layer_active_in_ui);
    py_menuactivate(mi,pysc,py_menus + pmt_char);
    sc_active_in_ui = NULL;
    layer_active_in_ui = ly_fore;
}

void fvpy_tllistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontViewBase *fv = (FontViewBase *) GDrawGetUserData(gw);
    PyObject *pyfv = PyFF_FontForFV(fv);

    fv_active_in_ui = fv;
    layer_active_in_ui = fv->active_layer;
    py_tllistcheck(mi,pyfv,py_menus + pmt_font);
    fv_active_in_ui = NULL;
}

static void fvpy_menuactivate(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontViewBase *fv = (FontViewBase *) GDrawGetUserData(gw);
    PyObject *pyfv = PyFF_FontForFV(fv);

    fv_active_in_ui = fv;
    layer_active_in_ui = fv->active_layer;
    py_menuactivate(mi,pyfv,py_menus + pmt_font);
    fv_active_in_ui = NULL;
}

static void PyMenuInit() {
    int t;
    unichar_t *mn_string, *cp;
    if ( py_menus!=NULL )
	return;

    py_menus = calloc(pmt_size, sizeof(struct py_menu_data));
    mn_string = utf82u_copy(U_(mn_order));

    for ( t=0; t<pmt_size; ++t ) {
	py_menus[t].mn_string = mn_string;
	py_menus[t].mn_avail = g_hash_table_new(g_direct_hash, g_direct_equal);
	for ( cp = mn_string; *cp!=0; ++cp )
	    g_hash_table_add(py_menus[t].mn_avail, GUINT_TO_POINTER(*cp));
    }

    py_menus[pmt_font].hotkey_prefix = "FontView.Menu.Tools.";
    py_menus[pmt_font].moveto = fvpy_tllistcheck;
    py_menus[pmt_font].invoke = fvpy_menuactivate;
    py_menus[pmt_font].setmenu = FVSetToolsSubmenu;

    py_menus[pmt_char].hotkey_prefix = "CharView.Menu.Tools.";
    py_menus[pmt_char].moveto = cvpy_tllistcheck;
    py_menus[pmt_char].invoke = cvpy_menuactivate;
    py_menus[pmt_char].setmenu = CVSetToolsSubmenu;
}

static struct flaglist menuviews[] = {
    { "Font", pmf_font },
    { "Glyph", pmf_char },
    { "Char", pmf_char },
    FLAGLIST_EMPTY
};

struct py_menu_text {
    const char *localized;
    const char *untranslated;
    const char *identifier;
};

struct py_menu_spec {
    int depth, divider;
    struct py_menu_text *levels;
    const char *shortcut_str;
    PyObject *func, *check, *data;
};

static int MenuDataAdd(struct py_menu_spec *spec, struct py_menu_data *pmd) {
    if ( pmd->cnt >= pmd->max )
	pmd->items = realloc(pmd->items,(pmd->max+=10)*sizeof(struct py_menu_item));

    pmd->items[pmd->cnt].func = spec->func;
    Py_XINCREF(spec->func);

    pmd->items[pmd->cnt].check= spec->check;
    Py_XINCREF(spec->check);

    pmd->items[pmd->cnt].data = spec->data;
    Py_XINCREF(spec->data);

    return pmd->cnt++;
}

static unichar_t *DoMnemonic(unichar_t *trans, unichar_t *alt,
		             struct py_menu_data *pmd) {
    unichar_t tmp_alt, *tmp_uni;

    tmp_alt = FindMnemonic(trans, *alt, pmd);
    if ( tmp_alt!=0 && tmp_alt!=*alt ) {
	// The original string might have had a suffix but we only need
	// to remove it if the alt code is replaced.
	tmp_uni = SetMnemonicSuffix(trans, *alt, 0);
	free(trans);
	trans = tmp_uni;
	*alt = tmp_alt;
    } else if ( tmp_alt==0 ) {
	*alt = AllocateNextMnemonic(pmd);
	tmp_uni = SetMnemonicSuffix(trans, 0, *alt);
	free(trans);
	trans = tmp_uni;
    }
    return trans;
}

static void InsertSubMenus(struct py_menu_spec *spec, struct py_menu_data *pmd) {
    int i, j;
    GMenuItem2 *mmn, *orig_menu, **mn;
    unichar_t alt;
    char *untrans, *action, *tmp_str;
    unichar_t *trans, *tmp_uni;

    mn = &pmd->menu;
    orig_menu = pmd->menu;

    for ( i=0; i<spec->depth; ++i ) {
	if ( i==spec->depth-1 && spec->divider ) {
	    untrans = "_____UNMATCH___ABLE______";
	    trans = NULL;
	    action = NULL;
	    alt = 0;
	} else {
	    // Lots of churn just to strip out a potentially unicode
	    // character from the untrans suffix, but oh well.
	    tmp_uni = utf82u_mncopy(spec->levels[i].untranslated, &alt);
	    untrans = u2utf8_copy(tmp_uni);
	    free(tmp_uni);
	    trans = utf82u_mncopy(spec->levels[i].localized, &alt);
	    if ( i==0 ) {
		action = strconcat(pmd->hotkey_prefix, untrans);
	    } else {
		tmp_str = strconcat3(action, ".", untrans);
		free(action);
		action = tmp_str;
	    }
	}
	j = 0;
	if ( *mn != NULL ) {
	    for ( j=0; (*mn)[j].ti.text!=NULL || (*mn)[j].ti.line; ++j ) {
		if ( (*mn)[j].ti.text==NULL )
	    continue;
		if ( strcmp((const char *) (*mn)[j].ti.text_untranslated,untrans)==0 )
	    break;
	    }
	}
	if ( *mn==NULL || (*mn)[j].ti.text==NULL ) {
	    *mn = realloc(*mn,(j+2)*sizeof(GMenuItem2));
	    memset(*mn+j,0,2*sizeof(GMenuItem2));
	}
	mmn = *mn;
	if ( mmn[j].ti.text==NULL ) {
	    mmn[j].ti.fg = mmn[j].ti.bg = COLOR_DEFAULT;
	    if ( i==0 && !spec->divider )
		trans = DoMnemonic(trans, &alt, pmd);
	    if ( i!=spec->depth-1 ) {
		mmn[j].ti.text = trans;
		mmn[j].ti.text_untranslated = untrans;
		mmn[j].ti.text_is_1byte = false;
		mmn[j].ti.mnemonic = alt;
		mmn[j].mid = -1;
		mmn[j].moveto = pmd->moveto;
		mn = &mmn[j].sub;
	    } else if ( spec->divider ) {
		mmn[j].ti.line = true;
	    } else {
		mmn[j].ti.text = trans;
		mmn[j].ti.text_untranslated = untrans;
		mmn[j].ti.text_is_1byte = false;
		mmn[j].ti.mnemonic = alt;
		mmn[j].invoke = pmd->invoke;
		mmn[j].mid = MenuDataAdd(spec,pmd);
	    }
	} else {
	    if ( mmn[j].sub != NULL ) {
		if ( i==0 && !spec->divider ) {
		    if ( mmn[j].ti.mnemonic != 0 )
			g_hash_table_add(pmd->mn_avail, GUINT_TO_POINTER(mmn[j].ti.mnemonic));
		    trans = DoMnemonic(trans, &alt, pmd);
		}
		free(mmn[j].ti.text);
		mmn[j].ti.text = trans;
		mmn[j].ti.mnemonic = alt;
	    }
	    if ( i!=spec->depth-1 )
		mn = &mmn[j].sub;
	    else {
		mmn[j].invoke = pmd->invoke;
		mmn[j].mid = MenuDataAdd(spec,pmd);
		fprintf( stderr, "Redefining menu item %s\n", untrans );
	    }
	    free(untrans);
	}
    }
    if ( spec->shortcut_str!=NULL )
	hotkeySetFull(action, spec->shortcut_str, false, hk_python);
    if ( orig_menu!=pmd->menu )
	(*pmd->setmenu)(pmd->menu);
}

static int DecodeMenuLevel(struct py_menu_text *text, PyObject *args) {
    if ( PyUnicode_Check(args) ) {
	text->localized = text->untranslated = PyUnicode_AsUTF8(args);
    } else {
	assert( PyTuple_Check(args) );
	for ( int i=0; i<PyTuple_Size(args); ++i ) {
	    if ( !PyUnicode_Check(PyTuple_GetItem(args, i)) ) {
		PyErr_Format(PyExc_ValueError, "All elements of a `name` or `submenu` tuple must be strings" );
		return false;
	    }
	}
	if ( PyTuple_Size(args)==2 ) {
	    text->localized = text->untranslated = PyUnicode_AsUTF8(PyTuple_GetItem(args, 0));
	    text->identifier = PyUnicode_AsUTF8(PyTuple_GetItem(args, 1));
	} else if ( PyTuple_Size(args)==3 ) {
	    text->localized = PyUnicode_AsUTF8(PyTuple_GetItem(args, 0));
	    text->untranslated = PyUnicode_AsUTF8(PyTuple_GetItem(args, 1));
	    text->identifier = PyUnicode_AsUTF8(PyTuple_GetItem(args, 2));
	} else {
	    PyErr_Format(PyExc_ValueError, "A `name` or `submenu` tuple must have either 2 or 3 elements" );
	    return false;
	}
    }
    return true;
}

static const char *rmi_keywords[] = { "callback", "enable", "data", "context", "hotkey", "name", "submenu", "keyword_only", NULL };
static const char *rmdiv_keywords[] = { "context", "divider", "submenu", NULL };

static PyObject *PyFF_registerMenuItem(PyObject *self, PyObject *args, PyObject *kwargs) {
    int i, flags, by_keyword = false, keyword_only = false;
    struct py_menu_spec spec;
    PyObject *context = Py_None, *name = Py_None, *submenu = Py_None;

    memset(&spec, 0, sizeof(spec));

    spec.func = spec.check = spec.data = Py_None;

    if ( no_windowing_ui ) // Just ignore non-ui calls
	Py_RETURN_NONE;

    if ( PyArg_ParseTupleAndKeywords(args, kwargs, "Op|$O", (char**) rmdiv_keywords,
                                     &context, &spec.divider, &submenu) ) {
	if ( !spec.divider ) {
	    PyErr_Format(PyExc_ValueError, "'divider' must be True when using this combination of keywords." );
	    return NULL;
	}
	by_keyword = true;
    } else {
	PyErr_Clear();
        if ( PyArg_ParseTupleAndKeywords(args, kwargs, "O|OOOsO$Op", (char**) rmi_keywords,
	                                 &spec.func, &spec.check, &spec.data, &context,
	                                 &spec.shortcut_str, &name, &submenu, &keyword_only) ) {
	    by_keyword = true;
	} // Leave error for keyword_only
    }

    if ( by_keyword ) {
	if ( PyList_Check(submenu) ) {
	    spec.depth = PyList_Size(submenu)+1;
	    spec.levels = calloc(spec.depth, sizeof(struct py_menu_text));
	    for (i=0; i<spec.depth-1; ++i) {
		if ( !DecodeMenuLevel(spec.levels+i, PyList_GetItem(submenu, i)) ) {
		    free(spec.levels);
		    return NULL;
		}
	    }
	} else if ( PyTuple_Check(submenu) || PyUnicode_Check(submenu) ) {
	    spec.depth = 2;
	    spec.levels = calloc(spec.depth, sizeof(struct py_menu_text));
	    if ( !DecodeMenuLevel(spec.levels, submenu) ) {
		free(spec.levels);
		return NULL;
	    }
	} else if ( submenu == Py_None ) {
	    spec.depth = 1;
	    spec.levels = calloc(spec.depth, sizeof(struct py_menu_text));
	} else {
	    PyErr_Format(PyExc_ValueError, "Unrecognized type for `submenu` argument" );
	    return NULL;
	}

	if ( PyTuple_Check(name) || PyUnicode_Check(name) ) {
	    if ( !DecodeMenuLevel(spec.levels+spec.depth-1, name) ) {
		free(spec.levels);
		return NULL;
	    }
	} else if ( !spec.divider ) {
	    PyErr_Format(PyExc_ValueError, "Unrecognized type for `name` argument" );
	    return NULL;
	}
    } else {
	if ( kwargs!=NULL && PyObject_IsTrue(PyDict_GetItemString(kwargs, "keyword_only"))==1 ) {
	    return NULL; // Return error set by ParseTupleAndKeywords
	}

	PyErr_Clear();

	// Legacy interface
	spec.depth = PyTuple_Size(args)-5;
	if ( spec.depth<1 ) {
	    PyErr_Format(PyExc_TypeError, "Positional interface: Too few arguments");
	    return NULL;
	}

	spec.func = PyTuple_GetItem(args, 0);
	spec.check = PyTuple_GetItem(args, 1);
	spec.data = PyTuple_GetItem(args, 2);
	context = PyTuple_GetItem(args, 3);

	if ( PyTuple_GetItem(args,4)!=Py_None ) {
	    spec.shortcut_str = PyUnicode_AsUTF8(PyTuple_GetItem(args, 4));
	    if (spec.shortcut_str == NULL) {
		PyErr_Format(PyExc_ValueError, "Positional interface: Shortcut argument is not string" );
		return NULL;
	    }
	}

	spec.levels = calloc(spec.depth, sizeof(struct py_menu_text));
	for ( i=0; i<spec.depth; ++i ) {
	    if ( !DecodeMenuLevel(spec.levels+i, PyTuple_GetItem(args, i+5)) ) {
		free(spec.levels);
		return NULL;
	    }
	}
    }

    if ( !spec.divider && !PyCallable_Check(spec.func) ) {
	PyErr_Format(PyExc_TypeError, "First ('callback') argument is not callable" );
	free(spec.levels);
	return NULL;
    }

    if (spec.check!=Py_None && !PyCallable_Check(spec.check)) {
	PyErr_Format(PyExc_TypeError, "Second ('enable') argument is not callable" );
	free(spec.levels);
	return NULL;
    } else if (spec.check==Py_None) {
	spec.check = NULL;
    }

    flags = FlagsFromTuple(context, menuviews, "menu window" );
    if ( flags==-1 ) {
	PyErr_Format(PyExc_ValueError, "Unknown context for menu" );
	free(spec.levels);
	return NULL;
    }

    if (spec.shortcut_str!=NULL && !HotkeyParse(NULL, spec.shortcut_str)) {
	PyErr_Format(PyExc_ValueError, "Cannot parse shortcut string" );
	free(spec.levels);
	return NULL;
    }

    PyMenuInit();

    if ( flags&pmf_font )
	InsertSubMenus(&spec, py_menus + pmt_font);
    if ( flags&pmf_char )
	InsertSubMenus(&spec, py_menus + pmt_char);

    Py_RETURN_NONE;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// This was used to add pycontrib/gdraw related interfaces to the fontforge
// module. It is left as a hook for possible future extension.
PyMethodDef module_fontforge_ui_methods[] = {

   // { "removeGtkWindowToMainEventLoopByFD", (PyCFunction) PyFFFont_removeGtkWindowToMainEventLoopByFD, METH_VARARGS, "fixme." },

   PYMETHODDEF_EMPTY /* Sentinel */
};


static PyMethodDef*
copyUIMethodsToBaseTable( PyMethodDef* ui, PyMethodDef* md )
{
    TRACE("copyUIMethodsToBaseTable()\n");
    // move md to the first target slot
    for( ; md->ml_name; )
    {
	md++;
    }
    for( ; ui->ml_name; ui++, md++ )
    {
	memcpy( md, ui, sizeof(PyMethodDef));
    }
    return md;
}

void PythonUI_Init(void) {
    TRACE("PythonUI_Init()\n");
    FfPy_Replace_MenuItemStub((PyCFunction)PyFF_registerMenuItem);

    copyUIMethodsToBaseTable( module_fontforge_ui_methods, module_fontforge_methods );
}
#endif

