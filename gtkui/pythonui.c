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
/*			 Python Interface to FontForge GTK+		      */

#include <fontforge-config.h>

#ifndef _NO_PYTHON
#include "Python.h"
#include "structmember.h"

#include "fontforgegtk.h"
#include <fontforge/ttf.h>
#include <ustring.h>
#include <fontforge/scripting.h>
#include <fontforge/scriptfuncs.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdarg.h>
#include <fontforge/ffpython.h>
#include <gdk/gdkkeysyms.h>

struct python_menu_info {
    char *name;
    int line;
    char *shortcut_str;
    struct python_menu_shell *sub_menu;
    PyObject *func;
    PyObject *check_enabled;		/* May be None (which I change to NULL) */
    PyObject *data;			/* May be None (left as None) */
};

static struct python_menu_shell {
    struct python_menu_info *menu;
    int cnt, max;
} cvpy_menu_data = { NULL, 0, 0}, fvpy_menu_data = {NULL, 0, 0};

static void test_item(GtkWidget *mi, gpointer _owner) {
    PyObject *owner;
    PyObject *arglist, *result;
    struct python_menu_info *info;

    info = g_object_get_data(G_OBJECT(mi),"py_menu_data");
    if ( info->check_enabled==NULL )
return;

    arglist = PyTuple_New(2);
    Py_XINCREF(info->data);
    Py_XINCREF(owner);
    PyTuple_SetItem(arglist,0,info->data);
    PyTuple_SetItem(arglist,1,owner);
    result = PyEval_CallObject(info->check_enabled, arglist);
    Py_DECREF(arglist);
    if ( result==NULL )
	/* Oh. An error. How fun. See below */;
    else if ( !PyInt_Check(result)) {
	LogError( "Return from enabling function for menu item %s must be boolean", info->name );
	gtk_widget_set_sensitive( GTK_WIDGET(mi), false );
    } else
	gtk_widget_set_sensitive( GTK_WIDGET(mi), PyInt_AsLong(result)!=0 );
    Py_XDECREF(result);
    if ( PyErr_Occurred()!=NULL )
	PyErr_Print();
}

static void py_tllistcheck(GtkMenuItem *mi, PyObject *owner) {
    GtkWidget *submenu;

    if ( mi==NULL )
return;
    submenu = gtk_menu_item_get_submenu(mi);
    if ( submenu==NULL )
return;
    gtk_container_foreach( GTK_CONTAINER(submenu), test_item, owner);
}

static void py_menuactivate(struct python_menu_info *info,PyObject *owner) {
    PyObject *arglist, *result;

    if ( info->func==NULL ) {
return;
    }
    arglist = PyTuple_New(2);
    Py_XINCREF(info->data);
    Py_XINCREF(owner);
    PyTuple_SetItem(arglist,0,info->data);
    PyTuple_SetItem(arglist,1,owner);
    result = PyEval_CallObject(info->func, arglist);
    Py_DECREF(arglist);
    Py_XDECREF(result);
    if ( PyErr_Occurred()!=NULL )
	PyErr_Print();
}

static void cvpy_tllistcheck(GtkMenuItem *menuitem, gpointer *data) {
    CharViewBase *cv = (CharViewBase *) lookup_ffdata( GTK_WIDGET(menuitem));
    PyObject *pysc = PySC_From_SC(cv->sc);

    sc_active_in_ui = cv->sc;
    py_tllistcheck(menuitem,pysc);
    sc_active_in_ui = NULL;
}

static void cvpy_menuactivate(GtkMenuItem *menuitem, gpointer *data) {
    CharViewBase *cv = (CharViewBase *) lookup_ffdata( GTK_WIDGET(menuitem));
    PyObject *pysc = PySC_From_SC(cv->sc);
    struct python_menu_info *info = (struct python_menu_info *) data;

    if ( info==NULL )
return;

    sc_active_in_ui = cv->sc;
    py_menuactivate(info,pysc);
    sc_active_in_ui = NULL;
}

static void fvpy_tllistcheck(GtkMenuItem *menuitem, gpointer *data) {
    FontViewBase *fv = (FontViewBase *) lookup_ffdata( GTK_WIDGET(menuitem));
    PyObject *pyfv = PyFV_From_FV(fv);

    fv_active_in_ui = fv;
    py_tllistcheck(menuitem,pyfv);
    fv_active_in_ui = NULL;
}

static void fvpy_menuactivate(GtkMenuItem *menuitem, gpointer *data) {
    FontViewBase *fv = (FontViewBase *) lookup_ffdata( GTK_WIDGET(menuitem));
    PyObject *pyfv = PyFV_From_FV(fv);
    struct python_menu_info *info = (struct python_menu_info *) data;

    if ( info==NULL )
return;

    fv_active_in_ui = fv;
    py_menuactivate(info,pyfv);
    fv_active_in_ui = NULL;
}

enum whichmenu { menu_fv=1, menu_cv=2 };
static struct flaglist menuviews[] = {
    { "Font", menu_fv },
    { "Glyph", menu_cv },
    { "Char", menu_cv },
    NULL
};

static void InsertSubMenus(PyObject *args,struct python_menu_shell *mn, int is_cv) {
    int i, j, cnt;
    PyObject *func, *check, *data;
    char *shortcut_str;

    /* I've done type checking already */
    cnt = PyTuple_Size(args);
    func = PyTuple_GetItem(args,0);
    if ( (check = PyTuple_GetItem(args,1))==Py_None )
	check = NULL;
    data = PyTuple_GetItem(args,2);
    if ( PyTuple_GetItem(args,4)==Py_None )
	shortcut_str = NULL;
    else
	shortcut_str = PyString_AsString(PyTuple_GetItem(args,4));

    for ( i=5; i<cnt; ++i ) {
	PyObject *submenu_utf8 = PyString_AsEncodedObject(PyTuple_GetItem(args,i),
		"UTF-8",NULL);
	char *submenu = copy( PyString_AsString(submenu_utf8) );
	Py_DECREF(submenu_utf8);

	j = 0;
	if ( mn->menu!=NULL ) {
	    for ( j=0; mn->menu[j].name!=NULL || mn->menu[j].line; ++j ) {
		if ( mn->menu[j].name==NULL )
	    continue;
		if ( strcmp(mn->menu[j].name,submenu)==0 )
	    break;
	    }
	}
	if ( mn->menu==NULL || mn->menu[j].name==NULL ) {
	    if ( mn->cnt<=mn->max )
		mn->menu = realloc(mn->menu,((mn->max+=5)+1)*sizeof(struct python_menu_info));
	    memset(mn->menu+j,0,2*sizeof(struct python_menu_info));
	}
	if ( mn->menu[j].name==NULL )
	    mn->menu[j].name = submenu;
	else {
	    if ( mn->menu[j].func!=NULL ) {
		Py_DECREF(mn->menu[j].func);
		Py_DECREF(mn->menu[j].data);
	    }
	    if ( mn->menu[j].check_enabled!=NULL ) {
		Py_DECREF(mn->menu[j].check_enabled);
	    }
	    mn->menu[j].func = mn->menu[j].data = mn->menu[j].check_enabled = NULL;
	}
	if ( i!=cnt-1 ) {
	    if ( mn->menu[j].sub_menu==NULL )
		mn->menu[j].sub_menu = calloc(1,sizeof(struct python_menu_shell));
	    mn = mn->menu[j].sub_menu;
	} else {
	    Py_INCREF(func);
	    if ( check!=NULL )
		Py_INCREF(check);
	    Py_INCREF(data);

	    mn->menu[j].shortcut_str = copy(shortcut_str);
	    mn->menu[j].func = func;
	    mn->menu[j].check_enabled = check;
	    mn->menu[j].data = data;
	    mn->menu[j].sub_menu = NULL;
	}
    }
}

/* (function,check_enabled,data,(char/font),shortcut_str,{sub-menu,}menu-name) */
static PyObject *PyFF_registerMenuItem(PyObject *self, PyObject *args) {
    int i, cnt;
    int flags;
    PyObject *utf8_name;

    printf("PyFF_registerMenuItem(top)\n" );
    
    if ( !no_windowing_ui ) {
	cnt = PyTuple_Size(args);
	if ( cnt<6 ) {
	    PyErr_Format(PyExc_TypeError, "Too few arguments");
return( NULL );
	}
	if (!PyCallable_Check(PyTuple_GetItem(args,0))) {
	    PyErr_Format(PyExc_TypeError, "First argument is not callable" );
return( NULL );
	}
	if (PyTuple_GetItem(args,1)!=Py_None &&
		!PyCallable_Check(PyTuple_GetItem(args,1))) {
	    PyErr_Format(PyExc_TypeError, "First argument is not callable" );
return( NULL );
	}
	flags = FlagsFromTuple(PyTuple_GetItem(args,3), menuviews );
	if ( flags==-1 ) {
	    PyErr_Format(PyExc_ValueError, "Unknown window for menu" );
return( NULL );
	}
	if ( PyTuple_GetItem(args,4)!=Py_None ) {
	    char *shortcut_str = PyString_AsString(PyTuple_GetItem(args,4));
	    if ( shortcut_str==NULL )
return( NULL );
	}
	for ( i=5; i<cnt; ++i ) {
	    utf8_name = PyString_AsEncodedObject(PyTuple_GetItem(args,i),
			"UTF-8",NULL);
	    if ( utf8_name==NULL )
return( NULL );
        printf("utf8_name: %s\n", utf8_name );
	    Py_DECREF(utf8_name);
	}
	if ( flags&menu_fv )
	    InsertSubMenus(args,&fvpy_menu_data,false );
	if ( flags&menu_cv )
	    InsertSubMenus(args,&cvpy_menu_data,true );
    }

Py_RETURN_NONE;
}

static void AddAccelorator(GtkAccelGroup *accel,char *shortcut, GtkWidget *mi) {
    static struct { char *modifier; int mask; } modifiers[] = {
	{ "Ctl", GDK_CONTROL_MASK },
	{ "Control", GDK_CONTROL_MASK },
	{ "Shft", GDK_SHIFT_MASK },
	{ "Shift", GDK_SHIFT_MASK },
	{ "CapsLk", GDK_LOCK_MASK },
	{ "CapsLock", GDK_LOCK_MASK },
	{ "Meta", GDK_MOD1_MASK },
	{ "Alt", GDK_MOD1_MASK },
	{ "Flag0x01", 0x01 },
	{ "Flag0x02", 0x02 },
	{ "Flag0x04", 0x04 },
	{ "Flag0x08", 0x08 },
	{ "Flag0x10", 0x10 },
	{ "Flag0x20", 0x20 },
	{ "Flag0x40", 0x40 },
	{ "Flag0x80", 0x80 },
	{ "Opt", 0x2000 },
	{ "Option", 0x2000 },
	{ NULL }};
    char *pt;
    int mask, keysym, temp, i;

    mask = 0;
    keysym = '\0';

    pt = strchr(shortcut,'|');
    if ( pt!=NULL )
	shortcut = pt+1;
    if ( *shortcut=='\0' || strcmp(shortcut,"No Shortcut")==0 )
return;

    mask = 0;
    while ( (pt=strchr(shortcut,'+'))!=NULL && pt!=shortcut ) {	/* A '+' can also occur as the short cut char itself */
	for ( i=0; modifiers[i].modifier!=NULL; ++i ) {
	    if ( strncasecmp(shortcut,modifiers[i].modifier,pt-shortcut)==0 )
	break;
	}
	if ( modifiers[i].modifier!=NULL )
	    mask |= modifiers[i].mask;
	else if ( sscanf( shortcut, "0x%x", &temp)==1 )
	    mask |= temp;
	else {
	    fprintf( stderr, "Could not parse short cut: %s\n", shortcut );
return;
	}
	shortcut = pt+1;
    }
    keysym = gdk_keyval_from_name(shortcut);

    gtk_widget_add_accelerator (GTK_WIDGET(mi), "activate", accel,
			  keysym, mask,
			  GTK_ACCEL_VISIBLE);
}

static void PyFF_BuildToolsMenu(GtkWidget *top,GtkMenuItem *tools,
	struct python_menu_shell *menu_data,enum whichmenu which,
	GtkAccelGroup *accel ) {
    if ( menu_data->cnt==0 )
	gtk_widget_hide(GTK_WIDGET(tools));
    else {
	struct python_menu_info *info;
	GtkWidget *menu, *mi;
	menu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (tools), menu);
	g_signal_connect ((gpointer) tools, "activate",
			G_CALLBACK(which==menu_fv ? fvpy_tllistcheck : cvpy_tllistcheck),
			NULL);

	if ( accel==NULL ) {
	    accel = gtk_accel_group_new();
	    gtk_window_add_accel_group( GTK_WINDOW( top ), accel);
	}

	for ( info = menu_data->menu; info->name!=NULL; ++info ) {
	    mi = gtk_menu_item_new_with_mnemonic ( info->name );
	    gtk_widget_set_name (mi, info->name );
	    gtk_widget_show (mi);
	    gtk_container_add (GTK_CONTAINER (menu), mi);
	    if ( info->shortcut_str!=NULL )
		AddAccelorator(accel,info->shortcut_str,mi);
	    g_object_set_data( G_OBJECT(mi),"py_menu_data", info );
	    if ( info->sub_menu==NULL )
		g_signal_connect ((gpointer) mi, "activate",
				G_CALLBACK(which==menu_fv ? fvpy_menuactivate : cvpy_menuactivate),
				info);
	    else {
		PyFF_BuildToolsMenu(top,GTK_MENU_ITEM(mi),info->sub_menu,which,accel);
	    }
	}
    }
}

void PyFF_BuildFVToolsMenu(FontView *fv,GtkMenuItem *tools) {
    PyFF_BuildToolsMenu(fv->gw,tools,&fvpy_menu_data,menu_fv, NULL);
}

void PyFF_BuildCVToolsMenu(CharView *cv,GtkMenuItem *tools) {
    PyFF_BuildToolsMenu(cv->gw,tools,&cvpy_menu_data,menu_cv, NULL);
}

void PythonUI_Init(void) {
    FfPy_Replace_MenuItemStub(PyFF_registerMenuItem);
}
#endif
