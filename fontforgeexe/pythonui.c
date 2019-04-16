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

#include <ffglib.h>

#include <fontforge-config.h>

#ifndef _NO_PYTHON
#include "Python.h"
#include "structmember.h"

#include "cvundoes.h"
#include "fontforgeui.h"
#include "ttf.h"
#include "ustring.h"
#include "scripting.h"
#include "scriptfuncs.h"
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <fcntl.h>
#include "ffpython.h"

static struct python_menu_info {
    PyObject *func;
    PyObject *check_enabled;		/* May be None (which I change to NULL) */
    PyObject *data;			/* May be None (left as None) */
} *cvpy_menu_data = NULL, *fvpy_menu_data = NULL;
static int cvpy_menu_cnt=0, cvpy_menu_max = 0;
static int fvpy_menu_cnt=0, fvpy_menu_max = 0;

GMenuItem2 *cvpy_menu, *fvpy_menu;

static void py_tllistcheck(struct gmenuitem *mi,PyObject *owner,
	struct python_menu_info *menu_data, int menu_cnt) {
    PyObject *arglist, *result;

    if ( menu_data==NULL || mi == NULL )
return;

    for ( mi = mi->sub; mi !=NULL && (mi->ti.text!=NULL || mi->ti.line); ++mi ) {
	if ( mi->mid==-1 )		/* Submenu */
    continue;
	if ( mi->mid<0 || mi->mid>=menu_cnt ) {
	    fprintf( stderr, "Bad Menu ID in python menu %d\n", mi->mid );
	    mi->ti.disabled = true;
    continue;
	}
	if ( menu_data[mi->mid].check_enabled==NULL ) {
	    mi->ti.disabled = false;
    continue;
	}
	arglist = PyTuple_New(2);
	Py_XINCREF(menu_data[mi->mid].data);
	Py_XINCREF(owner);
	PyTuple_SetItem(arglist,0,menu_data[mi->mid].data);
	PyTuple_SetItem(arglist,1,owner);
	result = PyEval_CallObject(menu_data[mi->mid].check_enabled, arglist);
	Py_DECREF(arglist);
	if ( result==NULL )
	    /* Oh. An error. How fun. See below */;
	else if ( !PyInt_Check(result)) {
	    char *menu_item_name = u2utf8_copy(mi->ti.text);
	    LogError(_("Return from enabling function for menu item %s must be boolean"), menu_item_name );
	    free( menu_item_name );
	    mi->ti.disabled = true;
	} else
	    mi->ti.disabled = PyInt_AsLong(result)==0;
	Py_XDECREF(result);
	if ( PyErr_Occurred()!=NULL )
	    PyErr_Print();
    }
}

static void py_menuactivate(struct gmenuitem *mi,PyObject *owner,
	struct python_menu_info *menu_data, int menu_cnt) {
    PyObject *arglist, *result;

    if ( mi->mid==-1 )		/* Submenu */
return;
    if ( mi->mid<0 || mi->mid>=menu_cnt ) {
	fprintf( stderr, "Bad Menu ID in python menu %d\n", mi->mid );
return;
    }
    if ( menu_data[mi->mid].func==NULL ) {
return;
    }
    arglist = PyTuple_New(2);
    Py_XINCREF(menu_data[mi->mid].data);
    Py_XINCREF(owner);
    PyTuple_SetItem(arglist,0,menu_data[mi->mid].data);
    PyTuple_SetItem(arglist,1,owner);
    result = PyEval_CallObject(menu_data[mi->mid].func, arglist);
    Py_DECREF(arglist);
    Py_XDECREF(result);
    if ( PyErr_Occurred()!=NULL )
	PyErr_Print();
}

void cvpy_tllistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    PyObject *pysc = PySC_From_SC(cv->b.sc);

    if ( cvpy_menu_data==NULL )
return;

    sc_active_in_ui = cv->b.sc;
    layer_active_in_ui = CVLayer((CharViewBase *) cv);
    PyFF_Glyph_Set_Layer(sc_active_in_ui,layer_active_in_ui);
    py_tllistcheck(mi,pysc,cvpy_menu_data,cvpy_menu_cnt);
    sc_active_in_ui = NULL;
    layer_active_in_ui = ly_fore;
}

static void cvpy_menuactivate(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    PyObject *pysc = PySC_From_SC(cv->b.sc);

    if ( cvpy_menu_data==NULL )
return;

    sc_active_in_ui = cv->b.sc;
    layer_active_in_ui = CVLayer((CharViewBase *) cv);
    PyFF_Glyph_Set_Layer(sc_active_in_ui,layer_active_in_ui);
    py_menuactivate(mi,pysc,cvpy_menu_data,cvpy_menu_cnt);
    sc_active_in_ui = NULL;
    layer_active_in_ui = ly_fore;
}

void fvpy_tllistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontViewBase *fv = (FontViewBase *) GDrawGetUserData(gw);
    PyObject *pyfv = PyFV_From_FV(fv);

    if ( fvpy_menu_data==NULL )
return;

    fv_active_in_ui = fv;
    layer_active_in_ui = fv->active_layer;
    py_tllistcheck(mi,pyfv,fvpy_menu_data,fvpy_menu_cnt);
    fv_active_in_ui = NULL;
}

static void fvpy_menuactivate(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontViewBase *fv = (FontViewBase *) GDrawGetUserData(gw);
    PyObject *pyfv = PyFV_From_FV(fv);

    if ( fvpy_menu_data==NULL )
return;

    fv_active_in_ui = fv;
    layer_active_in_ui = fv->active_layer;
    py_menuactivate(mi,pyfv,fvpy_menu_data,fvpy_menu_cnt);
    fv_active_in_ui = NULL;
}

enum { menu_fv=1, menu_cv=2 };
static struct flaglist menuviews[] = {
    { "Font", menu_fv },
    { "Glyph", menu_cv },
    { "Char", menu_cv },
    FLAGLIST_EMPTY
};

static int MenuDataAdd(PyObject *func,PyObject *check,PyObject *data,int is_cv) {
    Py_INCREF(func);
    if ( check!=NULL )
	Py_INCREF(check);
    Py_INCREF(data);

    if ( is_cv ) {
	if ( cvpy_menu_cnt >= cvpy_menu_max )
	    cvpy_menu_data = realloc(cvpy_menu_data,(cvpy_menu_max+=10)*sizeof(struct python_menu_info));
	cvpy_menu_data[cvpy_menu_cnt].func = func;
	cvpy_menu_data[cvpy_menu_cnt].check_enabled = check;
	cvpy_menu_data[cvpy_menu_cnt].data = data;
return( cvpy_menu_cnt++ );
    } else {
	if ( fvpy_menu_cnt >= fvpy_menu_max )
	    fvpy_menu_data = realloc(fvpy_menu_data,(fvpy_menu_max+=10)*sizeof(struct python_menu_info));
	fvpy_menu_data[fvpy_menu_cnt].func = func;
	fvpy_menu_data[fvpy_menu_cnt].check_enabled = check;
	fvpy_menu_data[fvpy_menu_cnt].data = data;
return( fvpy_menu_cnt++ );
    }
}

static void InsertSubMenus(PyObject *args,GMenuItem2 **mn, int is_cv) {
    int i, j, cnt;
    PyObject *func, *check, *data;
    char *shortcut_str;
    GMenuItem2 *mmn;

    /* I've done type checking already */
    cnt = PyTuple_Size(args);
    func = PyTuple_GetItem(args,0);
    if ( (check = PyTuple_GetItem(args,1))==Py_None )
	check = NULL;
    data = PyTuple_GetItem(args,2);
    if ( PyTuple_GetItem(args,4)==Py_None )
	shortcut_str = NULL;
    else {
#if PY_MAJOR_VERSION >= 3
        PyObject *obj = PyUnicode_AsUTF8String(PyTuple_GetItem(args,4));
        shortcut_str = PyBytes_AsString(obj);
#else /* PY_MAJOR_VERSION >= 3 */
        shortcut_str = PyBytes_AsString(PyTuple_GetItem(args,4));
#endif /* PY_MAJOR_VERSION >= 3 */
    }

    for ( i=5; i<cnt; ++i ) {
        PyObject *submenu_utf8 = PYBYTES_UTF8(PyTuple_GetItem(args,i));
	unichar_t *submenuu = utf82u_copy( PyBytes_AsString(submenu_utf8) );
	Py_DECREF(submenu_utf8);

	j = 0;
	if ( *mn != NULL ) {
	    for ( j=0; (*mn)[j].ti.text!=NULL || (*mn)[j].ti.line; ++j ) {
		if ( (*mn)[j].ti.text==NULL )
	    continue;
		if ( u_strcmp((*mn)[j].ti.text,submenuu)==0 )
	    break;
	    }
	}
	if ( *mn==NULL || (*mn)[j].ti.text==NULL ) {
	    *mn = realloc(*mn,(j+2)*sizeof(GMenuItem2));
	    memset(*mn+j,0,2*sizeof(GMenuItem2));
	}
	mmn = *mn;
	if ( mmn[j].ti.text==NULL ) {
	    mmn[j].ti.text = submenuu;
	    mmn[j].ti.fg = mmn[j].ti.bg = COLOR_DEFAULT;
	    if ( i!=cnt-1 ) {
		mmn[j].mid = -1;
		mmn[j].moveto = is_cv ? cvpy_tllistcheck : fvpy_tllistcheck;
		mn = &mmn[j].sub;
	    } else {
		mmn[j].shortcut = shortcut_str;
		mmn[j].invoke = is_cv ? cvpy_menuactivate : fvpy_menuactivate;
		mmn[j].mid = MenuDataAdd(func,check,data,is_cv);
	    }
	} else {
	    if ( i!=cnt-1 )
		mn = &mmn[j].sub;
	    else {
		char *temp = u2utf8_copy(submenuu);
		mmn[j].shortcut = shortcut_str;
		mmn[j].invoke = is_cv ? cvpy_menuactivate : fvpy_menuactivate;
		mmn[j].mid = MenuDataAdd(func,check,data,is_cv);
		fprintf( stderr, "Redefining menu item %s\n", temp );
		free(temp);
		free(submenuu);
	    }
	}
    }
}

/* (function,check_enabled,data,(char/font),shortcut_str,{sub-menu,}menu-name) */
static PyObject *PyFF_registerMenuItem(PyObject *self, PyObject *args) {
    int i, cnt;
    int flags;
    PyObject *utf8_name;

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
	    PyErr_Format(PyExc_TypeError, "Second argument is not callable" );
return( NULL );
	}
	flags = FlagsFromTuple(PyTuple_GetItem(args,3), menuviews, "menu window" );
	if ( flags==-1 ) {
	    PyErr_Format(PyExc_ValueError, "Unknown window for menu" );
return( NULL );
	}
	if ( PyTuple_GetItem(args,4)!=Py_None ) {
#if PY_MAJOR_VERSION >= 3
        PyObject *obj = PyUnicode_AsUTF8String(PyTuple_GetItem(args,4));
        if ( obj==NULL )
return( NULL );
        char *shortcut_str = PyBytes_AsString(obj);
        if ( shortcut_str==NULL ) {
            Py_DECREF( obj );
return( NULL );
        }
        Py_DECREF( obj );
#else /* PY_MAJOR_VERSION >= 3 */
	    char *shortcut_str = PyBytes_AsString(PyTuple_GetItem(args,4));
	    if ( shortcut_str==NULL )
return( NULL );
#endif /* PY_MAJOR_VERSION >= 3 */
	}
	for ( i=5; i<cnt; ++i ) {
        utf8_name = PYBYTES_UTF8(PyTuple_GetItem(args,i));
	    if ( utf8_name==NULL )
return( NULL );
	    Py_DECREF(utf8_name);
	}
	if ( flags&menu_fv )
	    InsertSubMenus(args,&fvpy_menu,false );
	if ( flags&menu_cv )
	    InsertSubMenus(args,&cvpy_menu,true );
    }

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
    FfPy_Replace_MenuItemStub(PyFF_registerMenuItem);

    copyUIMethodsToBaseTable( module_fontforge_ui_methods, module_fontforge_methods );
}
#endif

