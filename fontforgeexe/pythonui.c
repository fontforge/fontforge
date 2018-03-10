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

// to get asprintf() defined from stdio.h on GNU platforms
#define _GNU_SOURCE 1

#include <ffglib.h>

#include <fontforge-config.h>

#ifndef _NO_PYTHON
#include "Python.h"
#include "structmember.h"

#include "cvundoes.h"
#include "fontforgeui.h"
#include "ttf.h"
#include "plugins.h"
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

#include "gnetwork.h"
#ifdef BUILD_COLLAB
#include "collab/zmq_kvmsg.h"
#endif
#include "collabclientui.h"

/**
 * Use this to track if the script has joined a collab session.
 * if not then we get to very quickly avoid the collab code path :)
 */
static int inPythonStartedCollabSession = 0;

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
		mmn[j].shortcut = shortcut_str;
		mmn[j].invoke = is_cv ? cvpy_menuactivate : fvpy_menuactivate;
		mmn[j].mid = MenuDataAdd(func,check,data,is_cv);
		fprintf( stderr, "Redefining menu item %s\n", u2utf8_copy(submenuu) );
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


static PyObject *PyFFFont_CollabSessionStart(PyFF_Font *self, PyObject *args)
{
#ifdef BUILD_COLLAB
    int port_default = collabclient_getDefaultBasePort();
    int port = port_default;
    char address[IPADDRESS_STRING_LENGTH_T];
    if( !getNetworkAddress( address ))
    {
	snprintf( address, IPADDRESS_STRING_LENGTH_T-1,
		  "%s", HostPortPack( "127.0.0.1", port ));
    }
    else
    {
	snprintf( address, IPADDRESS_STRING_LENGTH_T-1,
		  "%s", HostPortPack( address, port ));
    }

    if ( PySequence_Size(args) == 1 )
    {
	char* uaddr = 0;
	if ( !PyArg_ParseTuple(args,"es","UTF-8",&uaddr) )
	    return( NULL );

	strcpy( address, uaddr );
    }
    FontViewBase *fv = self->fv;


    HostPortUnpack( address, &port, port_default );

    TRACE("address:%s\n", address );
    TRACE("port:%d\n", port );

    void* cc = collabclient_new( address, port );
    fv->collabClient = cc;
    collabclient_sessionStart( cc, (FontView*)fv );
    TRACE("connecting to server...sent the sfd for session start.\n");
    inPythonStartedCollabSession = 1;

#endif
    Py_RETURN( self );
}

static void* pyFF_maybeCallCVPreserveState( PyFF_Glyph *self ) {
#ifndef BUILD_COLLAB
    return 0;
#else
    if( !inPythonStartedCollabSession )
	return 0;

    CharView* cv = 0;
    static GHashTable* ht = 0;
    if( !ht )
    {
	ht = g_hash_table_new( g_direct_hash, g_direct_equal );
    }
    fprintf(stderr,"hash size:%d\n", g_hash_table_size(ht));

    gpointer cache = g_hash_table_lookup( ht, self->sc );
    if( cache )
    {
	return cache;
    }

    SplineFont *sf = self->sc->parent;
    FontView* fv = (FontView*)FontViewFind( FontViewFind_bySplineFont, sf );
    if( !fv )
    {
	fprintf(stderr,"Collab error: can not find fontview for the SplineFont of the active char\n");
    }
    else
    {
	int old_no_windowing_ui = no_windowing_ui;
	no_windowing_ui = 0;

	// FIXME: need to find the existing cv if available!
	cv = CharViewCreate( self->sc, fv, -1 );
	g_hash_table_insert( ht, self->sc, cv );
        fprintf(stderr,"added... hash size:%d\n", g_hash_table_size(ht));
	CVPreserveState( &cv->b );
	collabclient_CVPreserveStateCalled( &cv->b );

	no_windowing_ui = old_no_windowing_ui;
//	printf("called CVPreserveState()\n");
    }

    return cv;
#endif
}

static void pyFF_sendRedoIfInSession_Func_Real( void* cvv )
{
#ifdef BUILD_COLLAB
    CharView* cv = (CharView*)cvv;
    if( inPythonStartedCollabSession && cv )
    {
	collabclient_sendRedo( &cv->b );
//	printf("collabclient_sendRedo()...\n");
    }
#endif
}


static PyObject *CollabSessionSetUpdatedCallback = NULL;

static PyObject *PyFFFont_CollabSessionJoin(PyFF_Font *self, PyObject *args)
{
#ifdef BUILD_COLLAB

    char* address = collabclient_makeAddressString(
	"localhost", collabclient_getDefaultBasePort());

    if ( PySequence_Size(args) == 1 )
    {
	char* uaddr = 0;
	if ( !PyArg_ParseTuple(args,"es","UTF-8",&uaddr) )
	    return( NULL );

	address = uaddr;
    }
    FontViewBase *fv = self->fv;

    TRACE("PyFFFont_CollabSessionJoin() address:%s fv:%p\n", address, self->fv );
    void* cc = collabclient_newFromPackedAddress( address );
    TRACE("PyFFFont_CollabSessionJoin() address:%s cc1:%p\n", address, cc );
    fv->collabClient = cc;
    TRACE("PyFFFont_CollabSessionJoin() address:%s cc2:%p\n", address, fv->collabClient );
    FontViewBase* newfv = collabclient_sessionJoin( cc, (FontView*)fv );
    // here fv->collabClient is 0 and there is a new fontview.
    TRACE("PyFFFont_CollabSessionJoin() address:%s cc3:%p\n", address, fv->collabClient );
    TRACE("PyFFFont_CollabSessionJoin() address:%s cc4:%p\n", address, newfv->collabClient );

    inPythonStartedCollabSession = 1;
    PyObject* ret = PyFV_From_FV_I( newfv );
    Py_RETURN( ret );
#endif

    Py_RETURN( self );
}


#ifdef BUILD_COLLAB
static void InvokeCollabSessionSetUpdatedCallback(PyFF_Font *self) {
    if( CollabSessionSetUpdatedCallback )
    {
	PyObject *arglist;
	PyObject *result;

	arglist = Py_BuildValue("(O)", self);
	result = PyObject_CallObject(CollabSessionSetUpdatedCallback, arglist);
	Py_DECREF(arglist);
    }
}
#endif

static PyObject *PyFFFont_CollabSessionRunMainLoop(PyFF_Font *self, PyObject *args)
{
#ifdef BUILD_COLLAB
    int timeoutMS = 1000;
    int iterationTime = 50;
    int64_t originalSeq = collabclient_getCurrentSequenceNumber( self->fv->collabClient );

    TRACE("PyFFFont_CollabSessionRunMainLoop() called fv:%p\n", self->fv );
    TRACE("PyFFFont_CollabSessionRunMainLoop() called cc:%p\n", self->fv->collabClient );
    for( ; timeoutMS > 0; timeoutMS -= iterationTime )
    {
	g_usleep( iterationTime * 1000 );
	MacServiceReadFDs();
    }

    TRACE("originalSeq:%ld\n",(long int)(originalSeq));
    TRACE("     newSeq:%ld\n",(long int)(collabclient_getCurrentSequenceNumber( self->fv->collabClient )));

    if( originalSeq < collabclient_getCurrentSequenceNumber( self->fv->collabClient ))
    {
//	printf("***********************\n");
//	printf("*********************** calling python updated function!!\n");
//	printf("***********************\n");
//	printf("***********************\n");
	InvokeCollabSessionSetUpdatedCallback( self );
    }
#endif

    Py_RETURN( self );
}


static PyObject *PyFFFont_CollabSessionSetUpdatedCallback(PyFF_Font *self, PyObject *args)
{
    PyObject *result = NULL;
    PyObject *temp;

    if (PyArg_ParseTuple(args, "O:set_callback", &temp)) {
        if (!PyCallable_Check(temp)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be callable");
            return NULL;
        }
        Py_XINCREF(temp);
        Py_XDECREF(CollabSessionSetUpdatedCallback);
        CollabSessionSetUpdatedCallback = temp;
        /* Boilerplate to return "None" */
        Py_INCREF(Py_None);
        result = Py_None;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#ifdef FONTFORGE_CAN_USE_GTK

#define GMenuItem GMenuItem_Glib
#define GTimer GTimer_GTK
#define GList  GList_Glib
#include <gdk/gdkx.h>
#undef GTimer
#undef GList
#undef GMenuItem


static void GtkWindowToMainEventLoop_fd_callback( int fd, void* datas )
{
//    printf("GtkWindowToMainEventLoop_fd_callback()\n");
    gboolean may_block = false;
    g_main_context_iteration( g_main_context_default(), may_block );
}



static PyObject *PyFFFont_addGtkWindowToMainEventLoop(PyFF_Font *self, PyObject *args)
{
    PyObject *result = NULL;
    PyObject *temp;
    int v = 0;

    if ( !PyArg_ParseTuple( args, "i", &v ))
        return( NULL );

    gpointer gdkwindow = gdk_xid_table_lookup( v );

    if( gdkwindow )
    {
        Display* d = GDK_WINDOW_XDISPLAY(gdkwindow);
        int fd = XConnectionNumber(d);
        if( fd )
        {
            gpointer udata = 0;
            GDrawAddReadFD( 0, fd, udata, GtkWindowToMainEventLoop_fd_callback );
        }
    }
    
    /* Boilerplate to return "None" */
    Py_INCREF(Py_None);
    result = Py_None;
    return result;
}

static PyObject *PyFFFont_getGtkWindowMainEventLoopFD(PyFF_Font *self, PyObject *args)
{
    PyObject *result = NULL;
    PyObject *temp;
    int v = 0;

    if ( !PyArg_ParseTuple( args, "i", &v ))
        return( NULL );

    gpointer gdkwindow = gdk_xid_table_lookup( v );

    if( gdkwindow )
    {
        Display* d = GDK_WINDOW_XDISPLAY(gdkwindow);
        int fd = XConnectionNumber(d);
        if( fd )
        {
	    return( Py_BuildValue("i", fd ));
        }
    }
    
    /* Boilerplate to return "None" */
    Py_INCREF(Py_None);
    result = Py_None;
    return result;
}

static PyObject *PyFFFont_removeGtkWindowToMainEventLoop(PyFF_Font *self, PyObject *args)
{
    PyObject *result = NULL;
    PyObject *temp;
    int v = 0;

    if ( !PyArg_ParseTuple( args, "i", &v ))
        return( NULL );

    gpointer gdkwindow = gdk_xid_table_lookup( v );

    if( gdkwindow )
    {
        Display* d = GDK_WINDOW_XDISPLAY(gdkwindow);
        int fd = XConnectionNumber(d);
        if( fd )
        {
            gpointer udata = 0;
            GDrawRemoveReadFD( 0, fd, udata );
        }
    }
    
    /* Boilerplate to return "None" */
    Py_INCREF(Py_None);
    result = Py_None;
    return result;
}

static PyObject *PyFFFont_removeGtkWindowToMainEventLoopByFD(PyFF_Font *self, PyObject *args)
{
    PyObject *result = NULL;
    PyObject *temp;
    int v = 0;

    if ( !PyArg_ParseTuple( args, "i", &v ))
        return( NULL );

    int fd = v;
    gpointer udata = 0;
    GDrawRemoveReadFD( 0, fd, udata );
    
    /* Boilerplate to return "None" */
    Py_INCREF(Py_None);
    result = Py_None;
    return result;
}

#else

#define EMPTY_METHOD				\
    {						\
    PyObject *result = NULL;			\
    /* Boilerplate to return "None" */		\
    Py_INCREF(Py_None);				\
    result = Py_None;				\
    return result;				\
}
    									\
static PyObject *PyFFFont_addGtkWindowToMainEventLoop(PyFF_Font *self, PyObject *args)
{ EMPTY_METHOD; }
static PyObject *PyFFFont_getGtkWindowMainEventLoopFD(PyFF_Font *self, PyObject *args)
{ EMPTY_METHOD; }
static PyObject *PyFFFont_removeGtkWindowToMainEventLoop(PyFF_Font *self, PyObject *args)
{ EMPTY_METHOD; }
static PyObject *PyFFFont_removeGtkWindowToMainEventLoopByFD(PyFF_Font *self, PyObject *args)
{ EMPTY_METHOD; }

#endif

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////



static PyObject *PyFF_getLastChangedName(PyObject *UNUSED(self), PyObject *args) {
    PyObject *ret;
    ret = Py_BuildValue("s", Collab_getLastChangedName() );
    return( ret );
}
static PyObject *PyFF_getLastChangedPos(PyObject *UNUSED(self), PyObject *args) {
    PyObject *ret;
    ret = Py_BuildValue("i", Collab_getLastChangedPos() );
    return( ret );
}
static PyObject *PyFF_getLastChangedCodePoint(PyObject *UNUSED(self), PyObject *args) {
    PyObject *ret;
    ret = Py_BuildValue("i", Collab_getLastChangedCodePoint() );
    return( ret );
}
static PyObject *PyFF_getLastSeq(PyFF_Font *self, PyObject *args)
{
    int64_t seq = collabclient_getCurrentSequenceNumber( self->fv->collabClient );
    PyObject *ret;
    ret = Py_BuildValue("i", seq );
    return( ret );
}



/******************************/
/******************************/
/******************************/

PyMethodDef PyFF_FontUI_methods[] = {
   { "CollabSessionStart", (PyCFunction) PyFFFont_CollabSessionStart, METH_VARARGS, "Start a collab session at the given address (or the public IP address by default)" },

   { "CollabSessionJoin", (PyCFunction) PyFFFont_CollabSessionJoin, METH_VARARGS, "Join a collab session at the given address (or localhost by default)" },
   { "CollabSessionRunMainLoop", (PyCFunction) PyFFFont_CollabSessionRunMainLoop, METH_VARARGS, "Run the main loop, checking for and reacting to Collab messages for the given number of milliseconds (or 1 second by default)" },
   { "CollabSessionSetUpdatedCallback", (PyCFunction) PyFFFont_CollabSessionSetUpdatedCallback, METH_VARARGS, "Python function to call after a new collab update has been applied" },

   { "CollabLastChangedName", (PyCFunction) PyFF_getLastChangedName, METH_VARARGS, "" },
   { "CollabLastChangedPos", (PyCFunction) PyFF_getLastChangedPos, METH_VARARGS, "" },
   { "CollabLastChangedCodePoint", (PyCFunction) PyFF_getLastChangedCodePoint, METH_VARARGS, "" },
   { "CollabLastSeq", (PyCFunction) PyFF_getLastSeq, METH_VARARGS, "" },

   
   PYMETHODDEF_EMPTY /* Sentinel */
};

PyMethodDef module_fontforge_ui_methods[] = {

   // allow python code to expose it's gtk mainloop to fontforge
   { "addGtkWindowToMainEventLoop", (PyCFunction) PyFFFont_addGtkWindowToMainEventLoop, METH_VARARGS, "fixme." },
   { "getGtkWindowMainEventLoopFD", (PyCFunction) PyFFFont_getGtkWindowMainEventLoopFD, METH_VARARGS, "fixme." },
   { "removeGtkWindowToMainEventLoop", (PyCFunction) PyFFFont_removeGtkWindowToMainEventLoop, METH_VARARGS, "fixme." },
   { "removeGtkWindowToMainEventLoopByFD", (PyCFunction) PyFFFont_removeGtkWindowToMainEventLoopByFD, METH_VARARGS, "fixme." },

   
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

static void python_ui_fd_callback( int fd, void* udata );
static void python_ui_setup_callback( bool makefifo )
{
#ifndef __MINGW32__
    int fd = 0;
    int err = 0;
    char *userCacheDir, *sockPath;

    userCacheDir = getFontForgeUserDir(Cache);
    if ( userCacheDir==NULL ) {
        LogError("PythonUISetup: failed to discover user cache dir path");
        return;
    }

    asprintf(&sockPath, "%s/python-socket", userCacheDir);
    free(userCacheDir);

    if( makefifo ) {
        err = mkfifo( sockPath, 0600 );
        if ( err==-1  &&  errno!=EEXIST) {
            LogError("PythonUISetup: unable to mkfifo('%s'): errno %d\n", sockPath, errno);
            free(sockPath);
            return;
        }
    }

    fd = open( sockPath, O_RDONLY | O_NDELAY );
    if ( fd==-1) {
        LogError("PythonUISetup: unable to open socket '%s': errno %d\n", sockPath, errno);
        free(sockPath);
        return;
    }
    free(sockPath);

    void* udata = 0;
    GDrawAddReadFD( 0, fd, udata, python_ui_fd_callback );
    return;
#endif   
}

static void python_ui_fd_callback( int fd, void* udata )
{
#ifndef __MINGW32__
    char data[ 1024*100 + 1 ];
    memset(data, '\0', 1024*100 );
//    sleep( 1 );
    int sz = read( fd, data, 1024*100 );
//    fprintf( stderr, "python_ui_fd_callback() sz:%d d:%s\n", sz, data );

    CharView* cv = CharViewFindActive();
    if( cv )
    {
	int layer = 0;
	PyFF_ScriptString( cv->b.fv, cv->b.sc, layer, data );
    }
    
    GDrawRemoveReadFD( 0, fd, udata );
    python_ui_setup_callback( 0 );
#endif    
}

void PythonUI_namedpipe_Init(void) {
    python_ui_setup_callback( 1 );
    
}

void PythonUI_Init(void) {
    TRACE("PythonUI_Init()\n"); 
    FfPy_Replace_MenuItemStub(PyFF_registerMenuItem);
    set_pyFF_maybeCallCVPreserveState_Func( pyFF_maybeCallCVPreserveState );
    set_pyFF_sendRedoIfInSession_Func( pyFF_sendRedoIfInSession_Func_Real );

    copyUIMethodsToBaseTable( PyFF_FontUI_methods,         PyFF_Font_methods );
    copyUIMethodsToBaseTable( module_fontforge_ui_methods, module_fontforge_methods );

    
}
#endif

