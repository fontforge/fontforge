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
#include <fontforge-version-extras.h>

#ifndef _NO_PYTHON

#include "autohint.h"
#include "autotrace.h"
#include "autowidth2.h"
#include "bitmapcontrol.h"
#include "cvexport.h"
#include "cvimages.h"
#include "cvundoes.h"
#include "dumppfa.h"
#include "encoding.h"
#include "featurefile.h"
#include "ffglib.h"
#include "ffpython.h"
#include "flaglist.h"
#include "fontforgevw.h"
#include "fvcomposite.h"
#include "fvfonts.h"
#include "fvimportbdf.h"
#include "glyphcomp.h"
#include "langfreq.h"
#include "lookups.h"
#include "mathconstants.h"
#include "mem.h"
#include "multidialog.h"
#include "namelist.h"
#include "nonlineartrans.h"
#include "othersubrs.h"
#include "plugin.h"
#include "print.h"
#include "psread.h"
#include "savefont.h"
#include "scriptfuncs.h"
#include "scripting.h"
#include "scstyles.h"
#include "search.h"
#include "sfd.h"
#include "spiro.h"
#include "splinefill.h"
#include "splineorder2.h"
#include "splineoverlap.h"
#include "splinesaveafm.h"
#include "splinestroke.h"
#include "splineutil.h"
#include "splineutil2.h"
#include "start.h"
#include "svg.h"
#include "tottf.h"
#include "tottfgpos.h"
#include "ttf.h"
#include "ttfinstrs.h"
#include "ustring.h"
#include "utanvec.h"
#include "utype.h"

#include <dirent.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <wchar.h>

extern int old_sfnt_flags;
extern int prefRevisionsToRetain;

/* ========== MODULE DEFINITIONS ========== */
/* The following types are used to define Python Modules.  For every
 * module, you must create a 'module_definition' structure describing
 * it.  Then toward the bottom of this file in the "MODULE REGISTRY"
 * section, make sure you add the module to the list of all modules.
 */

typedef int (*type_initializer)(PyTypeObject *);

/* Typedef 'python_type_info' -- Create an array of these, one for
 * each python class (or type) that your module defines.  End the
 * array with a sentinel using the TYPEINFO_EMPTY macro
 */
typedef struct {
    PyTypeObject *typeobj; /* The python type definition. */
    int add_to_module; /* Include this type's name in the module namespace
			* if true, or keep it hidden if false.
			*/
    type_initializer setup_function; /* Called to do any extra modifications or
				      * setup of the type definition prior to
				      * the finalizing PyTypeReady() call.
				      */
} python_type_info;
#define TYPEINFO_EMPTY { NULL, 0, NULL }  /* Sentinel */


/* Typedef 'module_definition' -- Create one of these structures for
 * each Python module that is being defined.
 *
 * Be sure to register the definition in the "MODULE REGISTRY" section.
 */
typedef struct {
    /* Set these members as initial constant values. */
    const char *module_name;
    const char *docstring;
    python_type_info *types; /* array of type definitions, or NULL */
    PyMethodDef *methods; /* array of module-level methods/functions, or NULL */
    int auto_import;  /* automatically import module in embedded mode? */

    void (*finalize_func)(PyObject* module); /* Called to do any remaining post-initialization fixups */

    /* These following members are set at runtime; initialize using
     * the MODULEDEF_RUNTIMEINFO_INIT macro.
     */
    struct {
	PyObject *module;
	PyObject* (*modinit_func)(void);
	PyModuleDef pymod_def;
    } runtime;
} module_definition;

#define MODULEDEF_RUNTIMEINFO_INIT { NULL, NULL, {PyModuleDef_HEAD_INIT,NULL,NULL,-1,NULL,NULL,NULL,NULL,NULL} }

/* ----------------------------------------------------- */

static PyTypeObject PyFF_FontType;
static PyTypeObject PyFF_GlyphType;

static PyObject *InitializePythonMainNamespace(void);


/* Takes an ASCII string and returns a newly-allocated C "wide" string
 * equivalent.
 */
static wchar_t *copy_to_wide_string(const char *s) {
    size_t n;
    wchar_t *ws;

    ws = NULL;
    n = mbstowcs(NULL, s, 0) + 1;
    if (n != (size_t) -1) {
        ws = calloc(n, sizeof(wchar_t));
        mbstowcs(ws, s, n);
    }
    return ws;
}

static struct flaglist sfnt_name_str_ids[];
static struct flaglist sfnt_name_mslangs[];

/*** These have been moved elsewhere ***/
/*FontViewBase *fv_active_in_ui = NULL;*/
/*SplineChar *sc_active_in_ui = NULL;*/
/*int layer_active_in_ui = ly_fore;*/

static PyObject *hook_dict;			/* Dictionary of python hook scripts (to be activated when certain fontforge events happen) */
static PyObject *pickler, *unpickler;		/* cPickle.dumps, cPickle.loads */
static PyObject *_new_point, *_new_contour, *_new_layer;	/* Python handles to c functions, needed for pickler */
static void PyFF_PickleTypesInit(void);

/* A contour is a list of points, some on curve, some off. */
/* A closed contour is a circularly linked list */
/* A contour may be either quadratic or cubic */
/* cubic contours have two off-curve points between on-curve points -- or none for lines */
/* quadratic contours have one off-curve points between on-curve -- or none for lines */
/*  quadratic contours may also have two adjacent off-curve points, in which case an on-curve point is interpolated between (as in truetype) */
/* A layer is a set of contours all to be drawn together */



static int SSSelectOnCurve(SplineSet *ss,int pos);
static SplineSet *_SSFromContour(PyFF_Contour *, int *start, int flags);
static PyFF_Contour *ContourFromSS(SplineSet *,PyFF_Contour *);
static SplineSet *SSFromContour(PyFF_Contour *contour, int *tt_start);
static SplineSet *_SSFromLayer(PyFF_Layer *, int flags);
static SplineSet *SSFromLayer(PyFF_Layer *layer);
static PyFF_Layer *LayerFromSS(SplineSet *ss,PyFF_Layer *layer);
static PyFF_Layer *LayerFromLayer(Layer *,PyFF_Layer *);


/* ************************************************************************** */
/* Find python objects corresponding to non-python structures */
/* ************************************************************************** */
PyObject* PyFF_FontForFV( FontViewBase *fv ) {
    if ( fv==NULL )
	return NULL;
    if ( fv->python_fv_object==NULL ) {
        fv->python_fv_object = PyFF_FontType.tp_alloc(&PyFF_FontType,0);
        ((PyFF_Font *) (fv->python_fv_object))->fv = fv;
        Py_INCREF( (PyObject *) (fv->python_fv_object) );
    }
    return fv->python_fv_object;
}
PyObject* PyFF_FontForFV_I( FontViewBase *fv ) {
    PyObject* pyfv = PyFF_FontForFV(fv);
    Py_XINCREF(pyfv);
    return pyfv;
}
static PyFF_Font* PyFF_FontForSF( SplineFont *sf ) {
    if ( sf==NULL )
	return NULL;
    return (PyFF_Font*)PyFF_FontForFV( sf->fv );
}
static PyFF_Font* PyFF_FontForSC( SplineChar *sc ) {
    if ( sc==NULL )
	return NULL;
    return PyFF_FontForSF( sc->parent );
}
static PyFF_Glyph* PyFF_GlyphForSC( SplineChar *sc ) {
    if ( sc==NULL )
	return NULL;
    return sc->python_sc_object;
}

/* ************************************************************************** */
/* Checks for closed fonts */
/* ************************************************************************** */

static int IsFontClosed(const PyFF_Font *self) {
    return( self==NULL || self->fv==NULL );
}

static int CheckIfFontClosed(const PyFF_Font *self) {
    if ( IsFontClosed(self) ) {
	PyErr_Format(PyExc_RuntimeError, "Operation is not allowed after font has been closed" );
return( -1 );
    }
    return 0;
}

/* ************************************************************************** */
/* Utilities */
/* ************************************************************************** */

static GPtrArray *default_pyinit_dirs(void);
static int dir_exists(const char* path);


static void FreeStringArray( int cnt, char **names ) {
    int i;
    if ( names==NULL )
	return;
    for ( i=1; i<cnt; ++i ) {
	if ( names[i] != NULL )
	    free(names[i]);
    }
    free(names);
}

typedef int (*cmpfunc)(PyObject*, PyObject*);

/* Wrap a cmpfunc in a richcmpfunc. */
static PyObject *enrichened_compare(cmpfunc compare, PyObject *a, PyObject *b, int op)
{
    PyObject *result = NULL;
    int cmp;

    cmp = compare(a, b);
    if (cmp == -1 && PyErr_Occurred()) {
        if (PyErr_ExceptionMatches(PyExc_TypeError))
            switch (op) {
                case Py_EQ:
                    PyErr_Clear();
                    result = Py_False;
                    break;
                case Py_NE:
                    PyErr_Clear();
                    result = Py_True;
                    break;
	        default:
		    break;
            }
    } else {
        switch (op) {
            case Py_LT:
                result = (cmp < 0) ? Py_True : Py_False;
                break;
            case Py_LE:
                result = (cmp <= 0) ? Py_True : Py_False;
                break;
            case Py_EQ:
                result = (cmp == 0) ? Py_True : Py_False;
                break;
            case Py_NE:
                result = (cmp != 0) ? Py_True : Py_False;
                break;
            case Py_GT:
                result = (cmp > 0) ? Py_True : Py_False;
                break;
            case Py_GE:
                result = (cmp >= 0) ? Py_True : Py_False;
                break;
            default:
                result = Py_NotImplemented;
        }
    }
    Py_XINCREF(result);
    return result;
}

static int FlagsFromString(const char *str, struct flaglist *flags, const char *flagkind) {
    int i;
    i = FindFlagByName( flags, str );
    if ( i == FLAG_UNKNOWN ) {
	if ( flagkind == NULL )
	    flagkind = "flag";

	PyErr_Format( PyExc_ValueError, "Unknown %s \"%s\"", flagkind, str );
    }
return( i );
}

/* FlagsFromTuple() - Converts a python sequence of strings into a flags integer.
 * On an error a python exception is raised and FLAG_UNKNOWN returned.
 * If flagkind is not NULL, then it is used in any error message to identify
 * the kind of flags being parsed.
 */
int FlagsFromTuple(PyObject *tuple,struct flaglist *flags, const char *flagkind) {
    int ret = 0,temp;
    int i;
    const char *str = NULL;
    PyObject *obj;

    if ( flagkind == NULL )
	flagkind = "flag";

    /* Might be omitted */
    if ( tuple == NULL )
return( 0 );
    /* Might just be one string, might be a tuple (or any sequence) of strings */
    if ( PyUnicode_Check(tuple)) {
        if ((str = PyUnicode_AsUTF8(tuple)) == NULL) {
            return FLAG_UNKNOWN;
        }
        return FlagsFromString(str,flags,flagkind);
    } else if ( PySequence_Check(tuple)) {
	ret = 0;
	for ( i=0; i<PySequence_Size(tuple); ++i ) {
	    obj = PySequence_GetItem(tuple,i);
	    if ( obj==Py_None )
	continue;
	    if ( !PyUnicode_Check(obj)) {
		PyErr_Format(PyExc_TypeError, "Bad %s list, must consist of strings only", flagkind);
return( FLAG_UNKNOWN );
	    } else if ((str = PyUnicode_AsUTF8(obj)) == NULL) {
		return FLAG_UNKNOWN;
	    }
            temp = FlagsFromString(str,flags,flagkind);
	    if ( temp==FLAG_UNKNOWN )
return( FLAG_UNKNOWN );
	    ret |= temp;
	}
return( ret );
    } else {
	PyErr_Format(PyExc_TypeError, "Bad %s list, must be a single string or a sequence (tuple/list) of strings", flagkind);
return( FLAG_UNKNOWN );
    }
}

static PyObject *PyFF_ValToObject(Val *val) {
    if ( val->type==v_int || val->type==v_unicode )
return( Py_BuildValue("i", val->u.ival ));
    else if ( val->type==v_str )
return( Py_BuildValue("s", val->u.sval ));
    else if ( val->type==v_real )
return( Py_BuildValue("d", val->u.fval ));
    else if ( val->type==v_arr || val->type==v_arrfree )
	PyErr_SetString(PyExc_NotImplementedError, "Array -> tuple conversion not yet implemented. I didn't think I needed to.");
return( NULL );
}

PyObject *PySC_From_SC(SplineChar *sc) {
    if ( sc->python_sc_object==NULL ) {
	sc->python_sc_object = PyFF_GlyphType.tp_alloc(&PyFF_GlyphType,0);
	((PyFF_Glyph *) (sc->python_sc_object))->sc = sc;
	((PyFF_Glyph *) (sc->python_sc_object))->layer = ly_fore;
	Py_INCREF( (PyObject *) (sc->python_sc_object) );	/* for the pointer in my fv */
    }
return( sc->python_sc_object );
}

static PyObject *PySC_From_SC_I(SplineChar *sc) {
    PyObject *s = PySC_From_SC(sc);
    Py_INCREF(s);
return( s );
}

void PyFF_Glyph_Set_Layer(SplineChar *sc,int layer) {
    PyObject *pysc = PySC_From_SC(sc);
    ((PyFF_Glyph *) pysc)->layer = layer;
}

#define BAD_TAG ((uint32_t)0xffffffff)
static uint32_t StrToTag(const char *tag_name, int *was_mac) {
    uint8_t foo[4];
    int feat, set;

    if ( tag_name==NULL ) {
	PyErr_Format(PyExc_TypeError, "OpenType tags must be represented as strings" );
return( BAD_TAG );
    }

    if ( was_mac!=NULL && sscanf(tag_name,"<%d,%d>", &feat, &set )==2 ) {
	if ( feat < 0 || set < 0 ) {
	    PyErr_Format(PyExc_ValueError, "OpenType tag feature or set number must not be negative: %s", tag_name);
return( BAD_TAG );
	}
	*was_mac = true;
return( ( ((uint32_t)feat)<<16) | set );
    }

    if ( was_mac ) *was_mac = false;
    foo[0] = foo[1] = foo[2] = foo[3] = ' ';
    if ( *tag_name!='\0' ) {
	foo[0] = tag_name[0];
	if ( tag_name[1]!='\0' ) {
	    foo[1] = tag_name[1];
	    if ( tag_name[2]!='\0' ) {
		foo[2] = tag_name[2];
		if ( tag_name[3]!='\0' ) {
		    foo[3] = tag_name[3];
		    if ( tag_name[4]!='\0' ) {
			PyErr_Format(PyExc_ValueError, "OpenType tags are limited to 4 characters: %s", tag_name);
return( BAD_TAG );
		    }
		}
	    }
	}
    }
return( (foo[0]<<24) | (foo[1]<<16) | (foo[2]<<8) | foo[3] );
}

static uint32_t StrObjToTag(PyObject *obj, int *was_mac) {
    const char *str = PyUnicode_AsUTF8(obj);
    return str ? StrToTag(str, was_mac) : BAD_TAG;
}

static PyObject *TagToPythonString(uint32_t tag,int ismac) {
    char foo[30];

    if ( ismac ) {
	sprintf( foo,"<%d,%d>", tag>>16, tag&0xffff );
    } else {
	foo[0] = tag>>24;
	foo[1] = tag>>16;
	foo[2] = tag>>8;
	foo[3] = tag;
	foo[4] = '\0';
    }
return( PyUnicode_FromString(foo));
}

/* ************************************************************************** */
/* Methods of module FontForge                                                */
/* ************************************************************************** */

static PyObject *PyFF_GetPrefs(PyObject *UNUSED(self), PyObject *args) {
    const char *prefname;
    Val val;

    /* Pref names are ascii so no need to worry about encoding */
    if ( !PyArg_ParseTuple(args,"s",&prefname) )
return( NULL );
    memset(&val,0,sizeof(val));

    if ( !GetPrefs((char *) prefname,&val)) {
	PyErr_Format(PyExc_NameError, "Unknown preference item in GetPrefs: %s", prefname );
return( NULL );
    }
return( PyFF_ValToObject(&val));
}

static PyObject *PyFF_SetPrefs(PyObject *UNUSED(self), PyObject *args) {
    const char *prefname;
    Val val;
    double d;

    memset(&val,0,sizeof(val));
    /* Pref names are ascii so no need to worry about encoding */
    if ( PyArg_ParseTuple(args,"si",&prefname,&val.u.ival) )
	val.type = v_int;
    else {
	PyErr_Clear();
	if ( PyArg_ParseTuple(args,"ss",&prefname, &val.u.sval) )
	    val.type = v_str;
	else {
	    PyErr_Clear();
	    if ( PyArg_ParseTuple(args,"sd",&d) ) {
		val.u.fval = d;
		val.type = v_real;
	    } else
return( NULL );
	}
    }

    bool succeeded = SetPrefs((char *) prefname,&val,NULL);
    if (val.type == v_str && val.u.sval) {
        val.u.sval = NULL;
    }

    if (!succeeded) {
	PyErr_Format(PyExc_NameError, "Unknown preference item in SetPrefs: %s", prefname );
return( NULL );
    }
Py_RETURN_NONE;
}

static PyObject *PyFF_SavePrefs(PyObject *UNUSED(self), PyObject *UNUSED(args)) {

    SavePrefs(false);
Py_RETURN_NONE;
}

static PyObject *PyFF_LoadPrefs(PyObject *UNUSED(self), PyObject *UNUSED(args)) {

    LoadPrefs();
Py_RETURN_NONE;
}

static PyObject *PyFF_DefaultOtherSubrs(PyObject *UNUSED(self), PyObject *UNUSED(args)) {

    DefaultOtherSubrs();
Py_RETURN_NONE;
}

static PyObject *PyFF_ReadOtherSubrsFile(PyObject *UNUSED(self), PyObject *args) {
    const char *filename;

    /* here we do want the default encoding */
    if ( !PyArg_ParseTuple(args,"s", &filename) )
return( NULL );

    if ( ReadOtherSubrsFile((char *) filename)<=0 ) {
        PyErr_Format(PyExc_ImportError, "Could not find OtherSubrs file %s",  filename);
return( NULL );
    }

Py_RETURN_NONE;
}

static PyObject *PyFF_LoadEncodingFile(PyObject *UNUSED(self), PyObject *args) {
    const char *filename;
    char * encodingname = NULL;

    /* here we do want the default encoding */
    if ( !PyArg_ParseTuple(args,"s|s", &filename, &encodingname) )
return( NULL );

return( Py_BuildValue("s", ParseEncodingFile((char *) filename, encodingname)) );
}

static PyObject *PyFF_LoadNamelist(PyObject *UNUSED(self), PyObject *args) {
    const char *filename;

    /* here we do want the default encoding */
    if ( !PyArg_ParseTuple(args,"s", &filename) )
return( NULL );

    LoadNamelist((char *) filename);

Py_RETURN_NONE;
}

static PyObject *PyFF_LoadNamelistDir(PyObject *UNUSED(self), PyObject *args) {
    const char *filename;

    /* here we do want the default encoding */
    if ( !PyArg_ParseTuple(args,"s", &filename) )
return( NULL );

    LoadNamelistDir((char *) filename);

Py_RETURN_NONE;
}

static PyObject *PyFF_PreloadCidmap(PyObject *UNUSED(self), PyObject *args) {
    const char *filename, *reg, *order;
    int supplement;

    /* here we do want the default encoding for the filename, the others should be ascii */
    if ( !PyArg_ParseTuple(args,"sssi", &filename, &reg, &order, &supplement) )
return( NULL );

    LoadMapFromFile((char *) filename, (char *) reg, (char *) order, supplement);

Py_RETURN_NONE;
}

static PyObject *PyFF_UnicodeFromName(PyObject *UNUSED(self), PyObject *args) {
    char *name;
    PyObject *ret;

    if ( !PyArg_ParseTuple(args,"s", &name ))
return( NULL );

    ret = Py_BuildValue("i", UniFromName((char *) name, ui_none,&custom));
return( ret );
}

static PyObject *PyFF_NameFromUnicode(PyObject *UNUSED(self), PyObject *args) {
    char buffer[400], *nlist = NULL;
    const char *name;
    int uniinterp, uni;
    NameList *for_new_glyphs;
    PyObject *ret = NULL;

    if ( !PyArg_ParseTuple(args, "i|s", &uni, &nlist) )
return( NULL );

    if ( nlist != NULL ) {
	uniinterp = ui_none;
	for_new_glyphs = NameListByName( nlist );
	if ( for_new_glyphs == NULL ) {
	    PyErr_Format(PyExc_EnvironmentError, "Unknown namelist: %s\n", nlist);
return( NULL );
	}
    } else if (fv_active_in_ui == NULL) {
	uniinterp = ui_none;
	for_new_glyphs = NameListByName("AGL with PUA");
    } else {
	uniinterp = fv_active_in_ui->sf->uni_interp;
	for_new_glyphs = fv_active_in_ui->sf->for_new_glyphs;
    }

    name = StdGlyphNameBoundsCheck(buffer,uni,uniinterp,for_new_glyphs);
    if ( name!=NULL )
	ret = Py_BuildValue("s", name);
    else
	PyErr_Format(PyExc_ValueError, "Value %d has no Unicode name.", uni);
    return( ret );
}

static PyObject *PyFF_UnicodeAnnotationFromLib(PyObject *UNUSED(self), PyObject *args) {
/* If the library is available, then get the official annotation for this unicode value */
/* This function may be used in conjunction with UnicodeNameFromLib(n) */
    PyObject *ret;
    char *temp;
    long val;

    if ( !PyArg_ParseTuple(args,"|l",&val) )
	return( NULL );

    if ( (temp=uniname_annotation(val, false))==NULL ) {
	return Py_BuildValue("s", "");
    }
    ret=Py_BuildValue("s",temp); free(temp);
    return( ret );
}

static PyObject *PyFF_UnicodeNameFromLib(PyObject *UNUSED(self), PyObject *args) {
/* If the library is available, then get the official name for this unicode value */
/* This function may be used in conjunction with UnicodeAnnotationFromLib(n) */
    PyObject *ret;
    char *temp;
    long val;

    if ( !PyArg_ParseTuple(args,"|l",&val) )
	return( NULL );

    if ( (temp=uniname_name(val))==NULL ) {
	return Py_BuildValue("s", "");
    }
    ret=Py_BuildValue("s",temp); free(temp);
    return( ret );
}

static PyObject *PyFF_UnicodeBlockCountFromLib(PyObject *UNUSED(self), PyObject *UNUSED(args)) {
/* If the library is available, then return the number of name blocks */
    int count;
    uniname_blocks(&count);
    return( Py_BuildValue("i", count) );
}

static PyObject *PyFF_UnicodeBlockStartFromLib(PyObject *UNUSED(self), PyObject *args) {
/* If the library is available, then get the official start for this unicode block */
/* Use this function with UnicodeBlockNameFromLib(n) & UnicodeBlockEndFromLib(n). */
    const struct unicode_range *blocks;
    int num_blocks;
    long val;

    if ( !PyArg_ParseTuple(args,"|l",&val) )
	return( NULL );

    blocks = uniname_blocks(&num_blocks);
    if (val < 0 || val >= num_blocks) {
        val = -1;
    } else {
        val = blocks[val].start;
    }

    return( Py_BuildValue("l", val) );
}

static PyObject *PyFF_UnicodeBlockEndFromLib(PyObject *UNUSED(self), PyObject *args) {
/* If the library is available, then get the official end for this unicode block. */
/* Use this function with UnicodeBlockStartFromLib(n), UnicodeBlockNameFromLib(n) */
    const struct unicode_range *blocks;
    int num_blocks;
    long val;

    if ( !PyArg_ParseTuple(args,"|l",&val) )
	return( NULL );

    blocks = uniname_blocks(&num_blocks);
    if (val < 0 || val >= num_blocks) {
        val = -1;
    } else {
        val = blocks[val].end;
    }

    return( Py_BuildValue("l", val) );
}

static PyObject *PyFF_UnicodeBlockNameFromLib(PyObject *UNUSED(self), PyObject *args) {
/* If the library is available, then get the official name for this unicode block */
/* Use this function with UnicodeBlockStartFromLib(n), UnicodeBlockEndFromLib(n). */
    const struct unicode_range *blocks;
    int num_blocks;
    long val;

    if ( !PyArg_ParseTuple(args,"|l",&val) )
	return( NULL );

    blocks = uniname_blocks(&num_blocks);
    if (val < 0 || val >= num_blocks) {
        return Py_BuildValue("s", "");
    }
    return Py_BuildValue("s", blocks[val].name);
}

static PyObject *PyFF_UnicodeNamesListVersion(PyObject *UNUSED(self), PyObject *UNUSED(args)) {
/* If the library is available, then return the Nameslist Version number */
    return Py_BuildValue("s", "NamesList-Version: " UNICODE_VERSION);
}

static PyObject *PyFF_UnicodeNames2FromLib(PyObject *UNUSED(self), PyObject *args) {
    long val;
    char *temp;

    if ( !PyArg_ParseTuple(args,"|l",&val) )
	return( NULL );
    if ( (temp=uniname_formal_alias(val))==NULL ) {
	return Py_BuildValue("s", "");
    }
    PyObject *ret=Py_BuildValue("s",temp); free(temp);
    return( ret );
}

static PyObject *PyFF_Version(PyObject *UNUSED(self), PyObject *UNUSED(args)) {
return( Py_BuildValue("s", FONTFORGE_VERSION ));
}


static PyObject *PyFF_RunInitScripts(PyObject *UNUSED(self), PyObject *UNUSED(args)) {
    InitializePythonMainNamespace();
    PyFF_ProcessInitFiles(true, false);
Py_RETURN_NONE;
}

static PyObject *PyFF_LoadPlugins(PyObject *UNUSED(self), PyObject *UNUSED(args)) {
    InitializePythonMainNamespace();
    PyFF_ProcessInitFiles(false, true);
Py_RETURN_NONE;
}

static PyObject *PyFF_GetScriptPath(PyObject *UNUSED(self), PyObject *UNUSED(args)) {
    PyObject *ret;
    GPtrArray *dpath;

    dpath = default_pyinit_dirs();
    ret = PyTuple_New((int)dpath->len);
    for (guint i = 0; i < dpath->len; ++i) {
        PyTuple_SET_ITEM(ret, (int)i, Py_BuildValue("s", dpath->pdata[i]));
    }

    g_ptr_array_free(dpath, true);

    return ret;
}

static PyObject *PyFF_GetUserConfigPath(PyObject *UNUSED(self), PyObject *UNUSED(args)) {
    PyObject *ret;
    char *userdir;

    userdir = getFontForgeUserDir(Config);
    ret=Py_BuildValue("s",userdir);
    free(userdir);

    return ret;
}

static PyObject *PyFF_FontTuple(PyObject *UNUSED(self), PyObject *UNUSED(args)) {
    FontViewBase *fv;
    int cnt;
    PyObject *tuple;

    for ( fv=FontViewFirst(), cnt=0; fv!=NULL; fv=fv->next, ++cnt );
    tuple = PyTuple_New(cnt);
    for ( fv=FontViewFirst(), cnt=0; fv!=NULL; fv=fv->next, ++cnt )
	PyTuple_SET_ITEM(tuple,cnt,PyFF_FontForFV_I(fv));

return( tuple );
}

static PyObject *PyFF_ActiveFont(PyObject *UNUSED(self), PyObject *UNUSED(args)) {

    if ( fv_active_in_ui==NULL )
Py_RETURN_NONE;

return( PyFF_FontForFV_I( fv_active_in_ui ));
}

static PyObject *PyFF_ActiveGlyph(PyObject *UNUSED(self), PyObject *UNUSED(args)) {

    if ( sc_active_in_ui==NULL )
Py_RETURN_NONE;

return( PySC_From_SC_I( sc_active_in_ui ));
}

static PyObject *PyFF_ActiveLayer(PyObject *UNUSED(self), PyObject *UNUSED(args)) {

return( Py_BuildValue("i", layer_active_in_ui ));
}

static FontViewBase *SFAdd(SplineFont *sf,int hide) {
    if ( sf->fv!=NULL )
	/* All done */;
    else if ( !no_windowing_ui )
	FontViewCreate(sf,hide);
    else
	FVAppend(_FontViewCreate(sf));
return( sf->fv );
}


struct flaglist openflaglist[] = {
    { "fstypepermitted", of_fstypepermitted },
    { "allglyphsinttc", of_all_glyphs_in_ttc },
    { "fontlint", of_fontlint },
    { "hidewindow", of_hidewindow },
    { "alltables", of_all_tables },
    FLAGLIST_EMPTY
};

static PyObject *PyFF_OpenFont(PyObject *UNUSED(self), PyObject *args) {
    char *filename, *locfilename;
    int openflags = 0;
    SplineFont *sf;
    PyObject *flagsobj = NULL;

    if ( !PyArg_ParseTuple(args,"s|O", &filename, &flagsobj ))
	return NULL;
    locfilename = utf82def_copy(filename);

    if ( flagsobj!=NULL && PyLong_Check(flagsobj) ) {
	openflags = PyLong_AsLong(flagsobj);
    } else if ( flagsobj!=NULL && PyTuple_Check(flagsobj) ) {
	openflags = FlagsFromTuple(flagsobj, openflaglist, "open flag");
    } else if ( flagsobj!=NULL ) {
	free(locfilename);
	PyErr_Format(PyExc_IndexError, "Flags must be specified as String Tuple or Int");
	return NULL;
    }
    /* The actual filename opened may be different from the one passed
     * to LoadSplineFont, so we can't report the filename on an
     * error.
     */
    sf = LoadSplineFont(locfilename,openflags);

    if ( sf==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "Open failed");
	free(locfilename);
return( NULL );
    }
    free(locfilename);
return( PyFF_FontForFV_I( SFAdd( sf, openflags&of_hidewindow )));
}

static PyObject *PyFF_FontsInFile(PyObject *UNUSED(self), PyObject *args) {
    char *filename;
    char *locfilename = NULL;
    PyObject *tuple;
    char **ret;
    int cnt, i;

    if ( !PyArg_ParseTuple(args,"s",&filename) )
return( NULL );
    locfilename = utf82def_copy(filename);
    ret = GetFontNames(locfilename, 1);
    free(locfilename);
    cnt = 0;
    if ( ret!=NULL ) for ( cnt=0; ret[cnt]!=NULL; ++cnt );

    tuple = PyTuple_New(cnt);
    for ( i=0; i<cnt; ++i )
	PyTuple_SetItem(tuple,i,Py_BuildValue("s",ret[i]));
return( tuple );
}

static void prterror(void *UNUSED(foo), char *msg, int UNUSED(pos)) {
    fprintf( stderr, "%s\n", msg );
}

static PyObject *PyFF_ParseTTFInstrs(PyObject *UNUSED(self), PyObject *args) {
    PyObject *binstr;
    char *instr_str;
    int icnt;
    uint8_t *instrs;

    if ( !PyArg_ParseTuple(args,"s",&instr_str) )
return( NULL );
    instrs = _IVParse(NULL,instr_str,&icnt,prterror,NULL);
    if ( instrs==NULL ) {
	PyErr_Format(PyExc_TypeError, "Failed to parse instructions" );
return( NULL );
    }
    binstr = PyBytes_FromStringAndSize((char *) instrs,icnt);
    free(instrs);

return( binstr );
}

static PyObject *PyFF_UnParseTTFInstrs(PyObject *UNUSED(self), PyObject *args) {
    PyObject *tuple, *ret;
    int icnt, i;
    uint8_t *instrs;
    char *as_str;

    if ( !PyArg_ParseTuple(args,"O",&tuple) )
return( NULL );
    if ( !PySequence_Check(tuple)) {
	PyErr_Format(PyExc_TypeError, "Argument must be a sequence" );
return( NULL );
    }
    if ( PyBytes_Check(tuple)) {
	char *space; Py_ssize_t len;
	PyBytes_AsStringAndSize(tuple,&space,&len);
	instrs = calloc(len,sizeof(uint8_t));
	icnt = len;
	memcpy(instrs,space,len);
    } else {
	icnt = PySequence_Size(tuple);
	instrs = malloc(icnt);
	for ( i=0; i<icnt; ++i ) {
	    instrs[i] = PyLong_AsLong(PySequence_GetItem(tuple,i));
	    if ( PyErr_Occurred()) {
		free(instrs);
return( NULL );
	    }
	}
    }
    as_str = _IVUnParseInstrs(instrs,icnt);
    ret = PyBytes_FromString(as_str);
    free(as_str); free(instrs);
return( ret );
}

static PyObject *PyFF_unitShape(PyObject *UNUSED(self), PyObject *args) {
    int n=0;
    SplineSet *ss;
    PyObject *ret;

    if ( !PyArg_ParseTuple(args,"|i",&n) )
return( NULL );
    ss = UnitShape(n);
    ret = (PyObject *) ContourFromSS(ss,NULL);
    SplinePointListFree(ss);
return( ret );
}

static struct flaglist printmethod[] = {
    { "lp", 0 },
    { "lpr", 1 },
    { "ghostview", 2 },
    { "ps-file", 3 },
    { "command", 4 },
    { "pdf-file", 5 },
    FLAGLIST_EMPTY /* Sentinel */
};

static PyObject *PyFF_printSetup(PyObject *UNUSED(self), PyObject *args) {
    char *ptype, *pcmd = NULL;
    int iptype;

    if ( !PyArg_ParseTuple(args,"s|sii", &ptype, &pcmd, &pagewidth, &pageheight ) )
return( NULL );
    iptype = FlagsFromString(ptype,printmethod,"printing method");
    if ( iptype==FLAG_UNKNOWN ) {
return( NULL );
    }

    printtype = iptype;
    if ( pcmd!=NULL && iptype==4 )
	printcommand = copy(pcmd);
    else if ( pcmd!=NULL && (iptype==0 || iptype==1) )
	printlazyprinter = copy(pcmd);
Py_RETURN_NONE;
}

/* ************************************************************************** */
/* ************************* Import/Export routines ************************* */
/* ************************************************************************** */

struct python_import_export *py_ie;
static int ie_cnt, ie_max;

void PyFF_SCImport(SplineChar *sc,int ie_index,char *filename,
	int layer, int clear) {
    int toback = layer!=ly_fore;
    PyObject *arglist, *result, *glyph = PySC_From_SC(sc);

    if ( ie_index>=ie_cnt )
return;
    SCPreserveLayer(sc,layer,false);
    if ( clear ) {
	SplinePointListsFree(sc->layers[layer].splines);
	sc->layers[layer].splines = NULL;
    }

    sc_active_in_ui = sc;
    layer_active_in_ui = layer;

    arglist = PyTuple_New(4);
    Py_XINCREF(py_ie[ie_index].data);
    Py_XINCREF(glyph);
    PyTuple_SetItem(arglist,0,py_ie[ie_index].data);
    PyTuple_SetItem(arglist,1,glyph);
    PyTuple_SetItem(arglist,2,PyUnicode_DecodeUTF8(filename,strlen(filename),NULL));
    PyTuple_SetItem(arglist,3,Py_BuildValue("i",toback));
    result = PyObject_CallObject(py_ie[ie_index].import, arglist);
    Py_DECREF(arglist);
    Py_XDECREF(result);
    if ( PyErr_Occurred()!=NULL )
	PyErr_Print();
}

void PyFF_SCExport(SplineChar *sc,int ie_index,char *filename,int layer) {
    PyObject *arglist, *result, *glyph = PySC_From_SC(sc);

    if ( ie_index>=ie_cnt )
return;

    sc_active_in_ui = sc;
    layer_active_in_ui = layer;

    arglist = PyTuple_New(3);
    Py_XINCREF(py_ie[ie_index].data);
    Py_XINCREF(glyph);
    PyTuple_SetItem(arglist,0,py_ie[ie_index].data);
    PyTuple_SetItem(arglist,1,glyph);
    PyTuple_SetItem(arglist,2,PyUnicode_DecodeUTF8(filename,strlen(filename),NULL));
    PyTuple_SetItem(arglist,2,PyUnicode_DecodeUTF8(filename,strlen(filename),NULL));
    result = PyObject_CallObject(py_ie[ie_index].export, arglist);
    Py_DECREF(arglist);
    Py_XDECREF(result);
    if ( PyErr_Occurred()!=NULL )
	PyErr_Print();
    sc_active_in_ui = NULL;
    layer_active_in_ui = ly_fore;
}

static PyObject *PyFF_registerImportExport(PyObject *UNUSED(self), PyObject *args) {
    PyObject *import, *export, *data;
    char *name, *exten, *exten_list=NULL;
    /* I'm assuming the extensions are in ASCII so no conversion will be needed*/

    if ( !PyArg_ParseTuple(args,"OOOss|s", &import, &export, &data,
	    &name, &exten, &exten_list ))
return( NULL );
    if ( import==Py_None && export==Py_None )
Py_RETURN_NONE;			/* Well, that was pointless */
    if ( import==Py_None )
	import=NULL;
    else if ( !PyCallable_Check(import) ) {
	PyErr_Format(PyExc_TypeError, "First argument is not callable" );
return( NULL );
    }
    if ( export==Py_None )
	export=NULL;
    else if ( !PyCallable_Check(export) ) {
	PyErr_Format(PyExc_TypeError, "Second argument is not callable" );
return( NULL );
    }

    Py_XINCREF(import);
    Py_XINCREF(export);
    Py_XINCREF(data);

    if ( ie_cnt>=ie_max )
	py_ie = realloc(py_ie,((ie_max += 10)+1)*sizeof(struct python_import_export));
    py_ie[ie_cnt].import = import;
    py_ie[ie_cnt].export = export;
    py_ie[ie_cnt].data = data;
    py_ie[ie_cnt].name = name;
    py_ie[ie_cnt].extension = copy(exten);
    py_ie[ie_cnt].all_extensions = copy(exten_list==NULL ? exten : exten_list);
    ++ie_cnt;
    py_ie[ie_cnt].name = NULL;		/* End list marker */

Py_RETURN_NONE;
}

static PyObject *PyFF_hasSpiro(PyObject *UNUSED(self), PyObject *UNUSED(args)) {
    PyObject *ret = hasspiro() ? Py_True : Py_False;

    Py_INCREF(ret);
return( ret );
}

static PyObject *PyFF_SpiroVersion(PyObject *UNUSED(self), PyObject *UNUSED(args)) {
    char *temp;

    temp=libspiro_version();	/* Return libspiro Version number */

    PyObject *ret=Py_BuildValue("s",temp); free(temp);
    return( ret );
}

GList_Glib* closingFunctionList = 0;

static PyObject *PyFF_onAppClosing(PyObject *self, PyObject *args) {
    int cnt;

    cnt = PyTuple_Size(args);
//    printf("PyFF_onAppClosing() cnt:%d\n", cnt );
    if ( cnt<1 ) {
	PyErr_Format(PyExc_TypeError, "Too few arguments");
	return( NULL );
    }
    if (!PyCallable_Check(PyTuple_GetItem(args,0))) {
	PyErr_Format(PyExc_TypeError, "First argument is not callable" );
	return( NULL );
    }
    PyObject *func = PyTuple_GetItem(args,0);
    Py_INCREF(func);
    closingFunctionList = g_list_prepend( closingFunctionList, func );

    PyObject *ret = Py_True;
    Py_INCREF(ret);
    return( ret );
}

static void python_call_onClosingFunctions_fe( gpointer data, gpointer udata )
{
    PyObject *func = (PyObject*)data;
//    printf("python_call_onClosingFunctions_fe() f:%p\n", func );
    if( func )
    {
	PyObject *arglist, *result;
	arglist = PyTuple_New(0);
	result = PyObject_CallObject( func, arglist);
	if ( !result )
	{ // error
	}
    }
}

void python_call_onClosingFunctions()
{
    g_list_foreach( closingFunctionList,
		    python_call_onClosingFunctions_fe, 0 );
}

/* ************************************************************************** */
/* ************************ User Interface routines ************************* */
/* ************************************************************************** */

static PyObject *PyFF_registerMenuItemStub(PyObject *UNUSED(self), PyObject *UNUSED(args), PyObject *UNUSED(kwargs)) {
//    printf("PyFF_registerMenuItemStub()\n");
    /* This is a stub which will be replaced when we've got a UI */
Py_RETURN_NONE;
}

static PyObject *PyFF_hasUserInterface(PyObject *UNUSED(self), PyObject *UNUSED(args)) {
    PyObject *ret = no_windowing_ui ? Py_False : Py_True;

    Py_INCREF(ret);
return( ret );
}

static PyObject *PyFF_scriptFromUnicode(PyObject *UNUSED(self), PyObject *args) {
    unsigned long u;
    if ( !PyArg_ParseTuple(args,"k",&u) )
	return( NULL );

    uint32_t script = ScriptFromUnicode(u, NULL);
    return TagToPythonString(script, false);
}

static PyObject *PyFF_logError(PyObject *UNUSED(self), PyObject *args) {
    char *msg;
    if ( !PyArg_ParseTuple(args,"s", &msg) )
return( NULL );
    LogError(msg);
Py_RETURN_NONE;
}

// This allows the init code to post only less aggressive warning
// messages during startup.
static bool showPythonErrors = 1;

static PyObject *PyFF_postError(PyObject *UNUSED(self), PyObject *args) {
    char *msg, *title;
    if ( !PyArg_ParseTuple(args,"ss", &title, &msg) )
return( NULL );
    if( showPythonErrors )
        ff_post_error(title,msg);		/* Prints to stderr if no ui */
Py_RETURN_NONE;
}

static PyObject *PyFF_postNotice(PyObject *UNUSED(self), PyObject *args) {
    char *msg, *title;
    if ( !PyArg_ParseTuple(args,"ss", &title, &msg) )
return( NULL );
    ff_post_notice(title,msg);		/* Prints to stderr if no ui */
Py_RETURN_NONE;
}

static PyObject *PyFF_openFilename(PyObject *UNUSED(self), PyObject *args) {
    char *title,*def=NULL,*filter=NULL;
    char *ret;
    PyObject *reto;

    if ( no_windowing_ui ) {
	PyErr_Format(PyExc_EnvironmentError, "No user interface");
return( NULL );
    }

    if ( !PyArg_ParseTuple(args,"s|ss", &title, &def, &filter) )
return( NULL );

    ret = ff_open_filename(title,def,filter);
    if ( ret==NULL )
Py_RETURN_NONE;
    reto = PyUnicode_DecodeUTF8(ret,strlen(ret),NULL);
    free(ret);
return( reto );
}

static PyObject *PyFF_saveFilename(PyObject *UNUSED(self), PyObject *args) {
    char *title,*def=NULL,*filter=NULL;
    char *ret;
    PyObject *reto;

    if ( no_windowing_ui ) {
	PyErr_Format(PyExc_EnvironmentError, "No user interface");
return( NULL );
    }

    if ( !PyArg_ParseTuple(args,"s|ss", &title, &def, &filter) )
return( NULL );

    ret = ff_save_filename(title,def,filter);
    if ( ret==NULL )
Py_RETURN_NONE;
    reto = PyUnicode_DecodeUTF8(ret,strlen(ret),NULL);
    free(ret);
return( reto );
}

int NibCheck(SplineSet *ss);

int PyFF_ConvexNibID(const char *tok) {
   int r = ConvexNibID(tok);
   if ( r==-1 )
	PyErr_Format(PyExc_TypeError, "Unrecognized convex nib context name" );
   return r;
}

static SplineSet *PyFF_ParseSetConvex(PyObject *args, int *tokp) {
    PyObject *obj;
    SplineSet *ss;
    char *tok;

    if ( !PyArg_ParseTuple(args,"O|s", &obj, &tok ) ) {
	return NULL;
    }
    *tokp = PyFF_ConvexNibID(tok);
    if ( *tokp==-1 )
	return NULL;

    if ( PyType_IsSubtype(&PyFF_LayerType, Py_TYPE(obj)) ) {
	ss = SSFromLayer( (PyFF_Layer *) obj);
    } else if ( PyType_IsSubtype(&PyFF_ContourType, Py_TYPE(obj)) ) {
	ss = SSFromContour( (PyFF_Contour *) obj, NULL);
    } else {
	PyErr_Format(PyExc_TypeError, "Argument must be a layer or a contour" );
	return( NULL );
    }

    if ( !NibCheck(ss) ) {
	SplinePointListFree(ss);
	return ( NULL );
    }

    return ss;
}

static PyObject *PyFF_setConvexNib(PyObject *UNUSED(self), PyObject *args) {
    SplineSet *ss;
    int toknum;

    ss = PyFF_ParseSetConvex(args, &toknum);
    if ( ss==NULL )
	return NULL;

    if ( toknum < 0 && no_windowing_ui ) {
	PyErr_Format(PyExc_TypeError, "No user interface" );
	return NULL;
    }
    if ( !StrokeSetConvex(ss, toknum) )
	return NULL;

    Py_RETURN_NONE;
}

static PyObject *PyFF_getConvexNib(PyObject *UNUSED(self), PyObject *args) {
    const char *tok;
    int toknum;
    SplineSet *ss;
    PyFF_Layer *l;

    if ((tok = PyUnicode_AsUTF8(args)) == NULL) {
        return NULL;
    }
    toknum = PyFF_ConvexNibID(tok);
    if ( toknum==-1 )
	return NULL;

    ss = StrokeGetConvex(toknum, false);
    if ( ss==NULL )
	Py_RETURN_NONE;
    l = LayerFromSS(ss, NULL);
    SplinePointListFree(ss);
    return (PyObject *) l;
}

static PyObject *PyFF_ask(PyObject *UNUSED(self), PyObject *args) {
    const char *title=NULL,*quest=NULL, **answers;
    int def=0, cancel=-1, cnt;
    PyObject *answero;
    int i, ret;

    if ( no_windowing_ui ) {
	PyErr_Format(PyExc_EnvironmentError, "No user interface");
return( NULL );
    }

    if ( !PyArg_ParseTuple(args,"ssO|ii", &title, &quest, &answero, &def, &cancel) )
return( NULL );
    if ( !PySequence_Check(answero) || PyUnicode_Check(answero)) {
	PyErr_Format(PyExc_TypeError, "Expected a tuple of strings for the third argument");
return( NULL );
    }
    cnt = PySequence_Size(answero);
    answers = calloc(cnt+1, sizeof(const char *));
    if ( cancel==-1 )
	cancel = cnt-1;
    if ( cancel<0 || cancel>=cnt || def<0 || def>=cnt ) {
	PyErr_Format(PyExc_ValueError, "Value out of bounds for 4th or 5th argument");
	free(answers);
return( NULL );
    }
    for ( i=0; i<cnt; ++i ) {
        PyObject *item = PySequence_GetItem(answero, i);
        answers[i] = PyUnicode_AsUTF8(item);
        Py_XDECREF(item);
        if (answers[i] == NULL) {
            free(answers);
            return NULL;
        }
    }
    answers[cnt] = NULL;

    ret = ff_ask(title,answers,def,cancel,quest);
    free(answers);
return( Py_BuildValue("i",ret));
}

static const char *askChoices_keywords[] = { "title", "question", "answers", "default", "multiple", NULL };

static PyObject *PyFF_askChoices(PyObject *UNUSED(self), PyObject *args, PyObject *kwargs) {
    const char *title=NULL, *quest=NULL;
    const char **answers; // receives answers written into `answero`
    PyObject* defo = NULL; // default answer index, or tuple of len cnt
    int def = 0; // default index, only used if not multianswer
    int cnt; // number of answers
    PyObject *multipleo = NULL; // expected to be boolean
    bool multiple = false;
    PyObject *answero = NULL; // expected to be tuple
    int ret;

    if ( no_windowing_ui ) {
        PyErr_Format(PyExc_EnvironmentError, "No user interface");
        return( NULL );
    }

    if ( !PyArg_ParseTupleAndKeywords(args, kwargs, "ssO|OO", (char**) askChoices_keywords,
                                      &title, &quest, &answero, &defo, &multipleo) ) {
        PyErr_Format(PyExc_EnvironmentError, "Failed to parse arguments");
        return( NULL );
    }

    multiple = multipleo != NULL && PyObject_IsTrue(multipleo);

    if ( !PySequence_Check(answero) || PyUnicode_Check(answero)) {
        PyErr_Format(PyExc_TypeError, "Expected a tuple of strings for the third argument");
        return( NULL );
    }

    cnt = PySequence_Size(answero);
    char* sel = calloc(cnt, sizeof(char));

    if (defo != NULL && defo != Py_None) {
        if ( PyLong_Check(defo) ) {
            def = (int)PyLong_AsLong(defo);
            if ( def<0 || def>=cnt ) {
                PyErr_Format(PyExc_ValueError, "Value out of bounds for 4th argument");
                free(sel);
                return( NULL );
            }
            if (multiple) {
                sel[def] = (char)1;
            }
        } else if ( PyTuple_Check(defo) ) {
            int dcnt = PySequence_Size(defo);
            if (dcnt != cnt) {
                PyErr_Format(PyExc_ValueError, "Expected tuple/list of %d items, got %d items", cnt, dcnt );
                free(sel);
                return( NULL );
            }
            bool def_set = false;
            for (int i = 0; i < cnt; i++) {
                PyObject* temp = PyTuple_GetItem(defo, i);
                sel[i] = PyObject_IsTrue(temp);
                if (!multiple && sel[i]) {
                    if (def_set) {
                        PyErr_Format(PyExc_ValueError, "`multiple` False, only expected 1 True in `default`" );
                        free(sel);
                        return( NULL );
                    }
                    def = i;
                    def_set = true;
                }
            }
        } else {
            PyErr_Format(PyExc_TypeError, "4th argument must be a tuple or integer" );
            free(sel);
            return( NULL );
        }
    }

    answers = calloc(cnt+1, sizeof(const char *));
    for ( int i=0; i<cnt; ++i ) {
        PyObject *temp = PySequence_GetItem(answero,i);
        answers[i] = PyUnicode_AsUTF8(temp);
        Py_XDECREF(temp);
        if (answers[i] == NULL) {
            free(answers);
            free(sel);
            return NULL;
        }
    }
    answers[cnt] = NULL;

    PyObject* reto;
    if (multiple) {
        char* buts[2] = {_("OK"), _("Cancel")};
        reto = PyTuple_New(cnt);
        ret = ff_choose_multiple(title, answers, sel, cnt, buts, quest);
        for (int i = 0; i < cnt; i++) {
            PyObject* o = (sel[i] == 1) ? Py_True : Py_False;
            PyTuple_SetItem(reto, i, o);
            Py_INCREF(o); // prevents crash on finalize
        }
    } else {
        ret = ff_choose(title, answers, cnt, def, quest);
    }

    free(sel);
    free(answers);

    if (multiple) {
        return ( reto );
    }
    return( Py_BuildValue("i", ret) );
}

static PyObject *PyFF_askString(PyObject *UNUSED(self), PyObject *args) {
    char *title,*quest, *def = NULL;
    char *ret;
    PyObject *reto;

    if ( no_windowing_ui ) {
	PyErr_Format(PyExc_EnvironmentError, "No user interface");
return( NULL );
    }

    if ( !PyArg_ParseTuple(args,"ss|s", &title, &quest, &def) )
return( NULL );

    ret = ff_ask_string(title,def,quest);
    if ( ret==NULL )
Py_RETURN_NONE;
    reto = Py_BuildValue("s",ret);
    free(ret);
return( reto );
}

struct flaglist multiqstntype[] = {
    { "savepath", mdq_savepath },
    { "openpath", mdq_openpath },
    { "string", mdq_string },
    { "choice", mdq_choice },
    FLAGLIST_EMPTY /* Sentinel */
};

void multiDlgFree(MultiDlgSpec *dlg, int do_top) {
    for (int i=0; i<dlg->len; ++i) {
	MultiDlgCategory *category = &dlg->categories[i];
	for (int j=0; j<category->len; ++j) {
	    MultiDlgQuestion *qstn = &category->questions[j];
	    for (int k=0; k<qstn->answer_len; ++k) {
		Py_DECREF((PyObject *) qstn->answers[k].tag);
		free(qstn->answers[k].name);
	    }
	    // elem->tag reference is held by tagdict;
	    free(qstn->answers);
	    free(qstn->label);
	    free(qstn->filter);
	    free(qstn->dflt);
	    free(qstn->str_answer);
	}
	free(category->questions);
	free(category->label);
    }
    free(dlg->categories);
    if (do_top)
	free(dlg);
}

void multiDlgPrint(MultiDlgSpec *dlg) {
    for (int i=0; i<dlg->len; ++i) {
	MultiDlgCategory *category = dlg->categories + i;
	if (dlg->len>1)
	    printf("Category: %s\n", category->label);
	for (int j=0; j<category->len; ++j) {
	    MultiDlgQuestion *qstn = category->questions + j;
            printf("  Question: tag='%p', label='%s', default='%s', filter='%s', multiple=%d, checks=%d, align=%d, str_answer='%s'\n", qstn->tag, qstn->label, qstn->dflt, qstn->filter, qstn->multiple, qstn->checks, qstn->align, qstn->str_answer);
	    for (int k=0; k<qstn->answer_len; ++k) {
		MultiDlgAnswer *answer = qstn->answers + k;
		printf("      Answer: tag='%p', name='%s', is_default='%d', is_checked=%d\n", answer->tag, answer->name, answer->is_default, answer->is_checked);
	    }
	}
    }
}

// These strings are copied (and the tag objects are ref-incremented) so
// that it is safe to call back out into python during the execution of
// the function (if desired).
static char *getDictItemStringString(PyObject *dict, const char *key) {
    if ( !PyDict_Check(dict) )
	return NULL;
    PyObject *t = PyDict_GetItemString(dict, key);
    if ( t==NULL || !PyUnicode_Check(t) )
	return NULL;
    return copy(PyUnicode_AsUTF8(t));
}

static int getDictItemStringBool(PyObject *dict, const char *key, int do_err) {
    if ( !PyDict_Check(dict) )
	return do_err ? -1 : false;
    PyObject *t = PyDict_GetItemString(dict, key);
    if ( t==NULL )
	return do_err ? -1 : false;
    int r = PyObject_IsTrue(t);
    return do_err && r==-1 ? -1 : r;
}

static int multiDlgDecodeQuestion(MultiDlgQuestion *qstn, PyObject *spec, PyObject *tagdict) {
    char *t;

    if ( (t = getDictItemStringString(spec, "type")) == NULL ) {
	PyErr_Format(PyExc_TypeError, "askMulti: Missing 'type' key in Element specification.");
	return false;
    }
    if ( ( qstn->type = FlagsFromString(t, multiqstntype, "askMulti Element type") ) == FLAG_UNKNOWN) {
        free(t);
	return false;
    }
    free(t);
    qstn->label = getDictItemStringString(spec, "question");
    PyObject *tag = PyDict_GetItemString(spec, "tag");
    if ( tag==NULL )
	    tag = PyDict_GetItemString(spec, "question");
    if ( tag==NULL ) {
	PyErr_Format(PyExc_TypeError, "askMulti: Question specification must include either `question` or `tag` key.");
	return false;
    }
    if ( PyObject_Hash(tag)==-1 ) {
	PyErr_Format(PyExc_ValueError, "askMulti: Tag for question '%s' is not hashable.", qstn->label );
	return false;
    }
    if ( PyDict_Contains(tagdict, tag) ) {
	PyErr_Format(PyExc_ValueError, "askMulti: tag for question '%s' already used for different question.", qstn->label );
	return false;
    }
    PyDict_SetItem(tagdict, tag, Py_True);
    qstn->tag = tag;
    qstn->filter = getDictItemStringString(spec, "filter");
    qstn->multiple = getDictItemStringBool(spec, "multiple", false);
    qstn->checks = getDictItemStringBool(spec, "checks", false);
    qstn->align = !getDictItemStringBool(spec, "noalign", false);
    if ( qstn->type==mdq_choice ) {
	PyObject *answers_spec = PyDict_GetItemString(spec, "answers");
	if ( answers_spec==NULL || !PySequence_Check(answers_spec) ) {
	    PyErr_Format(PyExc_TypeError, "askMulti: Answers for choice-type question '%s' are missing or not in a list.", qstn->label );
	    return false;
	}
	qstn->answer_len = PySequence_Size(answers_spec);
	qstn->answers = calloc(qstn->answer_len, sizeof(MultiDlgAnswer));
	int defcnt = 0;
	for (int i = 0; i<qstn->answer_len; ++i) {
	    MultiDlgAnswer *answer = qstn->answers + i;
	    answer->question = qstn;
	    PyObject *answer_spec = PySequence_GetItem(answers_spec, i);
	    if ( !PyDict_Check(answer_spec) ) {
		PyErr_Format(PyExc_TypeError, "askMulti: Answer for choice-type question '%s' not a dictionary", qstn->label );
		Py_DECREF(answer_spec);
		return false;
	    }
	    answer->is_default = getDictItemStringBool(answer_spec, "default", false);
	    if ( answer->is_default )
		defcnt++;
	    answer->name = getDictItemStringString(answer_spec, "name");
	    tag = PyDict_GetItemString(answer_spec, "tag");
	    if ( tag==NULL )
		tag = PyDict_GetItemString(answer_spec, "name");
	    if ( tag==NULL ) {
		PyErr_Format(PyExc_TypeError, "askMulti: Answer for choice-type question '%s' must include either `name` or `tag`.");
		Py_DECREF(answer_spec);
		return false;
	    }
	    Py_INCREF(tag);
	    answer->tag = tag;
	    Py_DECREF(answer_spec);
	}
	if ( !qstn->multiple && defcnt>1 ) {
	    PyErr_Format(PyExc_TypeError, "askMulti: Too many default answers for single-choice question '%s'.", qstn->label );
	    return false;
	}
    } else {
	qstn->dflt = getDictItemStringString(spec, "default");
    }
    return true;
}

static int multiDlgDecodeCategory(MultiDlgCategory *category, PyObject *spec, PyObject *tagdict) {
    if ( (category->label = getDictItemStringString(spec, "category")) == NULL) {
	PyErr_Format(PyExc_TypeError, "askMulti: Missing 'category' key in Category specification.");
	return false;
    }
    PyObject *qstns_spec = PyDict_GetItemString(spec, "questions");
    if ( qstns_spec==NULL || !PySequence_Check(qstns_spec) ) {
	PyErr_Format(PyExc_TypeError, "askMulti: Category 'questions' key for '%s' is missing or is not a list.", category->label);
	return false;
    }
    category->len = PySequence_Size(qstns_spec);
    category->questions = calloc(category->len, sizeof(MultiDlgQuestion));
    for (int i = 0; i<category->len; ++i) {
        PyObject *qstn_spec = PySequence_GetItem(qstns_spec, i);
        if ( !multiDlgDecodeQuestion(category->questions + i, qstn_spec, tagdict) ) {
            return false;
        }
        Py_DECREF(qstn_spec);
    }
    return true;
}

PyObject *multiDlgExtractAnswers(MultiDlgSpec *dspec) {
    PyObject *r = PyDict_New(), *k, *v, *vtuple;
    int c, q, a, ai;

    for ( c=0; c<dspec->len; ++c ) {
	MultiDlgCategory *cspec = dspec->categories + c;
	for ( q=0; q<cspec->len; ++q ) {
	    MultiDlgQuestion *qspec = cspec->questions + q;
	    if ( qspec->type==mdq_choice ) {
		int acnt = 0;
		for ( a=0; a<qspec->answer_len; ++a )
		    if ( qspec->answers[a].is_checked )
			acnt++;
		if ( qspec->multiple )
		    vtuple = PyTuple_New(acnt);
		else
		    assert( acnt<=1 );
		ai = 0;
		for ( a=0; a<qspec->answer_len; ++a )
		    if ( qspec->answers[a].is_checked ) {
			v = qspec->answers[a].tag;
			Py_INCREF(v);
			if ( qspec->multiple ) {
			    PyTuple_SetItem(vtuple, ai, v);
			    ai++;
			} else
			    break;
		    }
		if ( qspec->multiple )
		    v = vtuple;
	    } else {
		if ( qspec->str_answer==NULL ) {
		    v = Py_None;
		    Py_INCREF(Py_None);
		} else
		    v = PyUnicode_FromString(qspec->str_answer);
	    }
	    k = qspec->tag;
	    assert( !PyDict_Contains(r, k) );
	    PyDict_SetItem(r, k, v);
	    // XXX seems like v should have an extra reference at this point,
	    // (because _SetItem() doesn't steal) but DECREFing v causes crashes
	}
    }
    return r;
}
static const char *askMulti_keywords[] = { "title", "specification", NULL };

static PyObject *PyFF_askMulti(PyObject *UNUSED(self), PyObject *args, PyObject *kwargs) {
    char *title;
    PyObject *spec, *r = Py_None, *tagdict = PyDict_New();
    PyObject *catstring = PyUnicode_FromString("category");
    PyObject *qstnstring = PyUnicode_FromString("question");
    MultiDlgSpec dlg;

    if ( no_windowing_ui ) {
	PyErr_Format(PyExc_EnvironmentError, "No user interface");
	return NULL;
    }

    if ( !PyArg_ParseTupleAndKeywords(args, kwargs, "sO", (char**) askMulti_keywords,
                                      &title, &spec) ) {
        PyErr_Format(PyExc_TypeError, "askMulti: Failed to parse arguments");
	Py_DECREF(tagdict); Py_DECREF(catstring); Py_DECREF(qstnstring);
        return NULL;
    }
    if ( PySequence_Check(spec) ) {
	PyObject *test = PySequence_GetItem(spec, 0);
	if ( test==NULL || !PyDict_Check(test) ) {
	    PyErr_Format(PyExc_TypeError, "askMulti: Failed to parse arguments");
	    Py_DECREF(tagdict); Py_DECREF(catstring); Py_DECREF(qstnstring);
	    return NULL;
	}
	if ( PyDict_Contains(test, catstring) ) {
	    dlg.len = PySequence_Size(spec);
	    dlg.categories = calloc(dlg.len, sizeof(MultiDlgCategory));
	    for (int i = 0; i<dlg.len; ++i) {
		PyObject *cat_spec = PySequence_GetItem(spec, i);
		if ( !multiDlgDecodeCategory(dlg.categories + i, cat_spec, tagdict) ) {
		    multiDlgFree(&dlg, false);
		    Py_DECREF(tagdict); Py_DECREF(catstring); Py_DECREF(qstnstring);
		    return NULL;
		}
		Py_DECREF(cat_spec);
	    }
	} else if ( PyDict_Contains(test, qstnstring) ) {
	    dlg.len = 1;
	    dlg.categories = calloc(1, sizeof(MultiDlgCategory));
	    dlg.categories[0].len = PySequence_Size(spec);
	    dlg.categories[0].questions = calloc(dlg.categories[0].len, sizeof(MultiDlgQuestion));
	    for (int i = 0; i<dlg.categories[0].len; ++i) {
		PyObject *qstn_spec = PySequence_GetItem(spec, i);
		if ( !multiDlgDecodeQuestion(dlg.categories[0].questions + i, qstn_spec, tagdict) ) {
		    multiDlgFree(&dlg, false);
		    Py_DECREF(tagdict); Py_DECREF(catstring); Py_DECREF(qstnstring);
		    return NULL;
		}
		Py_DECREF(qstn_spec);
	    }
	} else {
	    PyErr_Format(PyExc_TypeError, "askMulti: Failed to parse arguments");
	    Py_DECREF(tagdict); Py_DECREF(catstring); Py_DECREF(qstnstring);
	    return NULL;
	}
	Py_DECREF(test);
    } else if ( PyDict_Check(spec) ) {
	if ( PyDict_Contains(spec, catstring) ) {
	    dlg.len = 1;
	    dlg.categories = calloc(1, sizeof(MultiDlgCategory));
	    if ( !multiDlgDecodeCategory(dlg.categories, spec, tagdict) ) {
		multiDlgFree(&dlg, false);
		Py_DECREF(tagdict); Py_DECREF(catstring); Py_DECREF(qstnstring);
		return NULL;
	    }
	} else if ( PyDict_Contains(spec, qstnstring) ) {
	    dlg.len = 1;
	    dlg.categories = calloc(1, sizeof(MultiDlgCategory));
	    dlg.categories[0].len = 1;
	    dlg.categories[0].questions = calloc(1, sizeof(MultiDlgQuestion));
	    if ( !multiDlgDecodeQuestion(dlg.categories[0].questions, spec, tagdict) ) {
		multiDlgFree(&dlg, false);
		Py_DECREF(tagdict); Py_DECREF(catstring); Py_DECREF(qstnstring);
		return NULL;
	    }
	} else {
	    PyErr_Format(PyExc_TypeError, "askMulti: Failed to parse arguments");
	    Py_DECREF(tagdict); Py_DECREF(catstring); Py_DECREF(qstnstring);
	    return NULL;
	}
    } else {
        PyErr_Format(PyExc_TypeError, "askMulti: Specification must be either a sequence or dictionary.");
	Py_DECREF(tagdict); Py_DECREF(catstring); Py_DECREF(qstnstring);
	return NULL;
    }
    if ( ff_ask_multi(title, &dlg) )
	r = multiDlgExtractAnswers(&dlg);

    multiDlgFree(&dlg, false);
    Py_DECREF(tagdict); Py_DECREF(catstring); Py_DECREF(qstnstring);

    return r;
}

/* ************************************************************************** */
/* Points */
/* ************************************************************************** */

static PyTypeObject PyFF_PointType;
static const char *py_point_types[] = { "splineCorner", "splineCurve", "splineHVCurve",
				     "splineTangent", NULL };
#define MAX_POINTTYPE_VAL 3

static enum pointtype PyFF_ConvertToPointType(int val) {
    switch(val) {
	case 0: return pt_corner;
	case 1: return pt_curve;
	case 2: return pt_hvcurve;
	case 3: return pt_tangent;
    }
    return pt_corner;
}

static int PyFF_ConvertFromPointType(enum pointtype pt) {
    switch(pt) {
	case pt_curve: return 1;
	case pt_corner: return 0;
	case pt_tangent: return 3;
	case pt_hvcurve: return 2;
    }
    return 0;
}

static void AddPointConstants( PyObject *module ) {
    int i;
    for ( i=0; py_point_types[i]!=NULL; ++i )
        PyModule_AddObject(module, py_point_types[i], Py_BuildValue("i",i));
}

static PyFF_Point *PyFFPoint_CNew(double x, double y, int on_curve, int sel, int type, char *name) {
    /* Convenience routine for creating a new point from C */
    PyFF_Point *self = (PyFF_Point *)PyFF_PointType.tp_alloc(&PyFF_PointType, 0);
    if ( self==NULL )
	return( NULL );
    self->x = x;
    self->y = y;
    self->on_curve = on_curve;
    self->selected = sel;
    self->type = type;
    self->name = copy(name);
    return( self );
}

static PyObject *PyFFPoint_dup(PyFF_Point *self) {
    PyFF_Point *ret = PyFFPoint_CNew(self->x, self->y, self->on_curve,
                                     self->selected, self->type, self->name);
    ret->interpolated = self->interpolated;
    return( (PyObject *) ret );
}

static PyFF_Point *PyFFPoint_Parse(PyObject *args, bool dup, bool always) {
    double x,y;
    PyFF_Point *p=NULL;
    int on, sel, type, interp;

    if ( args==NULL && !always )
	return NULL;

    x = y = 0.0;
    on = true;
    sel = false;
    interp = false;
    type = PyFF_ConvertFromPointType(pt_corner);
    if ( !PyArg_ParseTuple( args, "(ddi)|ii", &x, &y, &on, &type, &sel )) {
	PyErr_Clear();
	if ( !PyArg_ParseTuple( args, "(dd)|iii", &x, &y, &on, &type, &sel )) {
	    PyErr_Clear();
	    if ( !PyArg_ParseTuple( args, "dd|iiii", &x, &y, &on, &type, &sel, &interp )) {
		PyErr_Clear();
		if ( PyType_IsSubtype(&PyFF_PointType, Py_TYPE(args)) ) {
		    p = (PyFF_Point *) args;
		} else if ( !always ) {
		    return( NULL );
		}
	    }
	}
    }

    if ( type < 0 || type > MAX_POINTTYPE_VAL ) {
	PyErr_Format(PyExc_TypeError, "Unrecognized point type");
	return NULL;
    }
    if ( p==NULL ) {
	p = PyFFPoint_CNew(x,y,on,sel,type,NULL);
	if ( p==NULL )
	    return( NULL );
	if ( interp )
	    p->interpolated = true;
    } else if ( dup ) {
	p = (PyFF_Point *) PyFFPoint_dup(p);
    } else {
	Py_INCREF( p );
    }
    return p;
}

static void PyFF_TransformPoint(PyFF_Point *self, double transform[6]) {
    double x,y;

    x = transform[0]*(double)self->x + transform[2]*(double)self->y + transform[4];
    y = transform[1]*(double)self->x + transform[3]*(double)self->y + transform[5];
    self->x = rint(1024*x)/1024;
    self->y = rint(1024*y)/1024;
}

static PyObject *PyFFPoint_Transform(PyFF_Point *self, PyObject *args) {
    double m[6];

    if ( !PyArg_ParseTuple(args,"(dddddd)",&m[0], &m[1], &m[2], &m[3], &m[4], &m[5]) )
return( NULL );
    PyFF_TransformPoint(self,m);
    Py_INCREF((PyObject *) self);
Py_RETURN( self );
}

static PyObject *PyFFPoint_pickleReducer(PyFF_Point *self, PyObject *UNUSED(args)) {
    PyObject *reductionTuple, *argTuple;

    if ( _new_point==NULL )
	PyFF_PickleTypesInit();
    reductionTuple = PyTuple_New(2);
    Py_INCREF(_new_point);
    PyTuple_SetItem(reductionTuple,0,_new_point);
    argTuple = PyTuple_New(6);
    PyTuple_SetItem(reductionTuple,1,argTuple);
    PyTuple_SetItem(argTuple,0,Py_BuildValue("d", (double)self->x));
    PyTuple_SetItem(argTuple,1,Py_BuildValue("d", (double)self->y));
    PyTuple_SetItem(argTuple,2,Py_BuildValue("i", self->on_curve));
    PyTuple_SetItem(argTuple,3,Py_BuildValue("i", self->type));
    PyTuple_SetItem(argTuple,4,Py_BuildValue("i", self->selected));
    PyTuple_SetItem(argTuple,5,Py_BuildValue("i", self->interpolated));
return( reductionTuple );
}

static int PyFFPoint_compare(PyFF_Point *self,PyObject *other) {
    double x, y;

    if ( !PyArg_ParseTuple(other,"dd", &x, &y ) ) {
	PyErr_Clear();
	if ( PyType_IsSubtype(&PyFF_PointType, Py_TYPE(other)) ) {
	    x = ((PyFF_Point *) other)->x;
	    y = ((PyFF_Point *) other)->y;
	} else {
	    PyErr_Format(PyExc_TypeError, "Unexpected type");
	    return( -1 );
	}
    }

    if ( RealNear(self->x,x) ) {
	if ( RealNear(self->y,y))
return( 0 );
	else if ( (double)self->y>y )
return( 1 );
    } else if ( (double)self->x>x )
return( 1 );

return( -1 );
}

static PyObject *PyFFPoint_get_name(PyFF_Point *self, void *UNUSED(closure)) {
    if (self->name==NULL)
	Py_RETURN_NONE;
    return (Py_BuildValue("s", self->name));
}

static int PyFFPoint_set_name(PyFF_Point *self,PyObject *value, void *UNUSED(closure)) {
    if (value == Py_None) {
        free(self->name);
        self->name = NULL;
    } else {
        char *name = copy(PyUnicode_AsUTF8(value));
        if (name == NULL) {
            return -1;
        }
        free(self->name);
        self->name = name;
    }
    return 0;
}

static PyObject *PyFFPoint_richcompare(PyObject *a, PyObject *b, int op) {
    return enrichened_compare((cmpfunc) PyFFPoint_compare, a, b, op);
}

static PyObject *PyFFPoint_Repr(PyFF_Point *self) {
    char buffer[200];

    sprintf(buffer,"%s(%g,%g,%s)", Py_TYPE(self)->tp_name, (double)self->x, (double)self->y,
	    self->on_curve?"True":"False" );
return( PyUnicode_FromString(buffer));
}

static PyObject *PyFFPoint_Str(PyFF_Point *self) {
    char buffer[200];

    sprintf(buffer,"<FFPoint (%g,%g) %s>", (double)self->x, (double)self->y, self->on_curve?"on":"off" );
return( PyUnicode_FromString(buffer));
}

static PyMemberDef FFPoint_members[] = {
    {(char *)"x", T_DOUBLE, offsetof(PyFF_Point, x), 0,
     (char *)"x coordinate"},
    {(char *)"y", T_DOUBLE, offsetof(PyFF_Point, y), 0,
     (char *)"y coordinate"},
    {(char *)"on_curve", T_UBYTE, offsetof(PyFF_Point, on_curve), 0,
     (char *)"whether this point lies on the curve or is a control point"},
    {(char *)"selected", T_UBYTE, offsetof(PyFF_Point, selected), 0,
     (char *)"whether this point is selected"},
    {(char *)"type", T_UBYTE, offsetof(PyFF_Point, type), 0,
     (char *)"the FontForge spline point type"},
    {(char *)"interpolated", T_UBYTE, offsetof(PyFF_Point, interpolated), 0,
     (char *)"whether this (quadratic) point is interpolated"},
    {NULL, 0, 0, 0, NULL}  /* Sentinel */
};

static PyMethodDef FFPoint_methods[] = {
    {(char *)"dup", (PyCFunction)PyFFPoint_dup, METH_NOARGS,
     (char *)"Returns a copy of this point" },
    {(char *)"transform", (PyCFunction)PyFFPoint_Transform, METH_VARARGS,
     (char *)"Transforms the point by the transformation matrix (a 6 element tuple of reals)" },
    {(char *)"__reduce__", (PyCFunction)PyFFPoint_pickleReducer, METH_NOARGS,
     (char *)"cPickle calls this routine when it wants to pickle us" },
    PYMETHODDEF_EMPTY /* Sentinel */
};

static PyGetSetDef FFPoint_getset[] = {
        {(char *)"name",
     (getter)PyFFPoint_get_name, (setter)PyFFPoint_set_name,
     (char *)"Points may be named", NULL},
    PYGETSETDEF_EMPTY /* Sentinel */
};

static PyObject *PyFFPoint_New(PyTypeObject *UNUSED(type), PyObject *args, PyObject *UNUSED(kwds)) {
    PyFF_Point *self = PyFFPoint_Parse(args, true, true);
    return( (PyObject *)self );
}

static PyTypeObject PyFF_PointType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "fontforge.point",         /* tp_name */
    sizeof(PyFF_Point),        /* tp_basicsize */
    0,                         /* tp_itemsize */
    NULL,                      /* tp_dealloc */
    0,                         /* tp_vectorcall_offset */
    NULL,                      /* tp_getattr */
    NULL,                      /* tp_setattr */
    NULL,                      /* tp_reserved / tp_compare */
    (reprfunc) PyFFPoint_Repr, /* tp_repr */
    NULL,                      /* tp_as_number */
    NULL,                      /* tp_as_sequence */
    NULL,                      /* tp_as_mapping */
    NULL,                      /* tp_hash */
    NULL,                      /* tp_call */
    (reprfunc) PyFFPoint_Str,  /* tp_str */
    NULL,                      /* tp_getattro */
    NULL,                      /* tp_setattro */
    NULL,                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    "fontforge Point objects", /* tp_doc */
    NULL,                      /* tp_traverse */
    NULL,                      /* tp_clear */
    (richcmpfunc) PyFFPoint_richcompare, /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    NULL,                      /* tp_iter */
    NULL,                      /* tp_iternext */
    FFPoint_methods,           /* tp_methods */
    FFPoint_members,           /* tp_members */
    FFPoint_getset,            /* tp_getset */
    NULL,                      /* tp_base */
    NULL,                      /* tp_dict */
    NULL,                      /* tp_descr_get */
    NULL,                      /* tp_descr_set */
    0,                         /* tp_dictoffset */
    NULL,                      /* tp_init */
    NULL,                      /* tp_alloc */
    PyFFPoint_New,             /* tp_new */
    NULL,                      /* tp_free */
    NULL,                      /* tp_is_gc */
    NULL,                      /* tp_bases */
    NULL,                      /* tp_mro */
    NULL,                      /* tp_cache */
    NULL,                      /* tp_subclasses */
    NULL,                      /* tp_weaklist */
    NULL,                      /* tp_del */
    0,                         /* tp_version_tag */
};

/* ************************************************************************** */
/* Contour iterator type */
/* ************************************************************************** */

typedef struct {
    PyObject_HEAD
    int pos;
    PyFF_Contour *contour;
} contouriterobject;
static PyTypeObject PyFF_ContourIterType;

static PyObject *contouriter_new(PyObject *contour) {
    contouriterobject *ci;
    ci = PyObject_New(contouriterobject, &PyFF_ContourIterType);
    if (ci == NULL)
return NULL;
    ci->contour = ((PyFF_Contour *) contour);
    Py_INCREF(contour);
    ci->pos = 0;
return (PyObject *)ci;
}

static void contouriter_dealloc(contouriterobject *ci) {
    Py_XDECREF(ci->contour);
    PyObject_Del(ci);
}

static PyObject *contouriter_iternext(contouriterobject *ci) {
    PyFF_Contour *contour = ci->contour;
    PyObject *pt;

    if ( contour == NULL)
return NULL;

    if ( ci->pos<contour->pt_cnt ) {
	pt = (PyObject *) contour->points[ci->pos++];
	Py_INCREF(pt);
return( pt );
    }

return NULL;
}

static PyTypeObject PyFF_ContourIterType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "fontforge.contour_iterator", /* tp_name */
    sizeof(contouriterobject), /* tp_basicsize */
    0,                         /* tp_itemsize */
    /* methods */
    (destructor)contouriter_dealloc, /* tp_dealloc */
    0,                         /* tp_vectorcall_offset */
    NULL,                      /* tp_getattr */
    NULL,                      /* tp_setattr */
    NULL,                      /* tp_compare */
    NULL,                      /* tp_repr */
    NULL,                      /* tp_as_number */
    NULL,                      /* tp_as_sequence */
    NULL,                      /* tp_as_mapping */
    NULL,                      /* tp_hash */
    NULL,                      /* tp_call */
    NULL,                      /* tp_str */
    NULL,                      /* tp_getattro */
    NULL,                      /* tp_setattro */
    NULL,                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    NULL,                      /* tp_doc */
    NULL,                      /* tp_traverse */
    NULL,                      /* tp_clear */
    NULL,                      /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    PyObject_SelfIter,         /* tp_iter */
    (iternextfunc)contouriter_iternext,	/* tp_iternext */
    NULL,                      /* tp_methods */
    NULL,                      /* tp_members */
    NULL,                      /* tp_getset */
    NULL,                      /* tp_base */
    NULL,                      /* tp_dict */
    NULL,                      /* tp_descr_get */
    NULL,                      /* tp_descr_set */
    0,                         /* tp_dictoffset */
    NULL,                      /* tp_init */
    NULL,                      /* tp_alloc */
    NULL,                      /* tp_new */
    NULL,                      /* tp_free */
    NULL,                      /* tp_is_gc */
    NULL,                      /* tp_bases */
    NULL,                      /* tp_mro */
    NULL,                      /* tp_cache */
    NULL,                      /* tp_subclasses */
    NULL,                      /* tp_weaklist */
    NULL,                      /* tp_del */
    0,                         /* tp_version_tag */
};

/* ************************************************************************** */
/* Contours */
/* ************************************************************************** */

static int PyFFContour_clear(PyFF_Contour *self) {
    int i;

    for ( i=0; i<self->pt_cnt; ++i )
	Py_DECREF(self->points[i]);
    self->pt_cnt = 0;

return 0;
}

static void PyFFContour_dealloc(PyFF_Contour *self) {
    PyFFContour_clear(self);
    PyMem_Del(self->points);
    if ( self->spiro_cnt!=0 )
	PyMem_Del(self->spiros);
    free(self->name);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static void PyFFContour_ClearSpiros(PyFF_Contour *self) {
    if ( self->spiro_cnt!=0 )
	free(self->spiros);
    self->spiros = NULL;
    self->spiro_cnt = 0;
}

static PyObject *PyFFContour_new(PyTypeObject *type, PyObject *UNUSED(args), PyObject *UNUSED(kwds)) {
    PyFF_Contour *self;

    self = (PyFF_Contour *)type->tp_alloc(type, 0);
    if ( self!=NULL ) {
	self->points = NULL;
	self->pt_cnt = self->pt_max = 0;
	self->is_quadratic = false;
	self->closed = 0;
	self->spiro_cnt = 0;
	self->name = NULL;
    }
    return (PyObject *)self;
}

static int PyFFContour_init(PyFF_Contour *self, PyObject *args, PyObject *UNUSED(kwds)) {
    int quad=0;

    if ( args!=NULL && !PyArg_ParseTuple(args, "|i", &quad))
return -1;

    self->is_quadratic = (quad!=0);
return 0;
}

static PyObject *PyFFContour_Str(PyFF_Contour *self) {
    char *buffer, *pt;
    int i;
    PyObject *ret;

    buffer=pt=malloc(self->pt_cnt*30+30);
    strcpy(buffer, self->is_quadratic? "<Contour(quadratic)\n":"<Contour(cubic)\n");
    pt = buffer+strlen(buffer);
    for ( i=0; i<self->pt_cnt; ++i ) {
	sprintf( pt, "  (%g,%g) %s\n", (double)self->points[i]->x, (double)self->points[i]->y,
		self->points[i]->on_curve ? "on" : "off" );
	pt += strlen( pt );
    }
    strcpy(pt,">");
    ret = PyUnicode_FromString( buffer );
    free( buffer );
return( ret );
}

static int PyFFContour_docompare(PyFF_Contour *self,PyObject *other,
	double pt_err, double spline_err) {
    SplineSet *ss, *ss2;
    int ret;
    SplinePoint *badpoint;

    if ( !PyType_IsSubtype(&PyFF_ContourType, Py_TYPE(other)) ) {
	PyErr_Format(PyExc_TypeError, "Unexpected type");
return( -1 );
    }
    ss = SSFromContour(self,NULL);
    ss2 = SSFromContour((PyFF_Contour *) other,NULL);
    ret = SSsCompare(ss,ss2,pt_err,spline_err,&badpoint);
    SplinePointListFree(ss);
    SplinePointListFree(ss2);
return(ret);
}

static int PyFFContour_compare(PyFF_Contour *self,PyObject *other) {
    const double pt_err = .5, spline_err = 1;
    int i,ret;

    ret = PyFFContour_docompare(self,other,pt_err,spline_err);
    if ( !(ret&SS_NoMatch) )
return( 0 );
    /* There's no real ordering on these guys. Make up something that is */
    /*  at least consistent */
    if ( self->pt_cnt < ((PyFF_Contour *) other)->pt_cnt )
return( -1 );
    else if ( self->pt_cnt > ((PyFF_Contour *) other)->pt_cnt )
return( 1 );
    /* If there's a difference, then there must be at least one point to be */
    /*  different. And we only get here if there were a difference */
    for ( i=0; i<self->pt_cnt; ++i ) {
	ret = PyFFPoint_compare(self->points[i],(PyObject *) (((PyFF_Contour *) other)->points[i]) );
	if ( ret!=0 )
return( ret );
    }
return( -1 );		/* Arbitrary... but we can't get here=>all points same */
}

static PyObject *PyFFContour_richcompare(PyObject *a, PyObject *b, int op) {
    return enrichened_compare((cmpfunc) PyFFContour_compare, a, b, op);
}

/* ************************************************************************** */
/* Contour getters/setters */
/* ************************************************************************** */
static PyObject *PyFF_Contour_get_is_quadratic(PyFF_Contour *self, void *UNUSED(closure)) {
return( Py_BuildValue("i", self->is_quadratic ));
}

static int PyFF_Contour_set_is_quadratic(PyFF_Contour *self,PyObject *value, void *UNUSED(closure)) {
    int val;
    SplineSet *ss, *ss2;

    val = PyLong_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );

    val = (val!=0);
    if ( val == self->is_quadratic )
return( 0 );
    if ( self->pt_cnt!=0 ) {
	ss = SSFromContour(self,NULL);
	PyFFContour_clear(self);
        if ( ss==NULL ) {
	    if ( PyErr_Occurred() != NULL ) {
                return( -1 );
	    } else {
		// Empty contour
		self->is_quadratic = (val!=0);
		return ( 0 );
	    }
        }
	if ( val )
	    ss2 = SplineSetsTTFApprox(ss);
	else
	    ss2 = SplineSetsPSApprox(ss);
	SplinePointListFree(ss);
	ContourFromSS(ss2,self);
	SplinePointListFree(ss2);
    }
    self->is_quadratic = (val!=0);
    return( 0 );
}

static PyObject *PyFF_Contour_get_closed(PyFF_Contour *self, void *UNUSED(closure)) {
return( Py_BuildValue("i", self->closed ));
}

static int PyFF_Contour_set_closed(PyFF_Contour *self,PyObject *value, void *UNUSED(closure)) {
    int val;

    val = PyLong_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );

    val = (val!=0);
    if ( val == self->closed )
return( 0 );
    PyFFContour_ClearSpiros((PyFF_Contour *) self);
    if ( !val ) {
	self->closed = false;
	if ( self->pt_cnt>1 && self->points[0]->on_curve )
	    self->points[self->pt_cnt++] = PyFFPoint_CNew(self->points[0]->x,self->points[0]->y,true,false,0,NULL);
    } else {
	self->closed = true;
	if ( self->pt_cnt>1 && self->points[0]->on_curve &&
		self->points[self->pt_cnt-1]->on_curve &&
		self->points[0]->x == self->points[self->pt_cnt-1]->x &&
		self->points[0]->y == self->points[self->pt_cnt-1]->y ) {
	    --self->pt_cnt;
	    Py_DECREF(self->points[self->pt_cnt]);
	}
    }
return( 0 );
}

static PyObject *PyFF_Contour_get_spiros(PyFF_Contour *self, void *UNUSED(closure)) {
    PyObject *spirotuple;
    int i;

    if ( !hasspiro()) {
	PyErr_Format(PyExc_EnvironmentError, "Spiros not available. Please install libspiro before continuing" );
return( NULL );
    }
    if ( self->spiro_cnt==0 ) {
	SplineSet *ss;
	uint16_t cnt;
	ss = SSFromContour(self,NULL);
        if ( ss==NULL ) {
	    if ( PyErr_Occurred() == NULL )
		// XXX Should this really be an error?
		PyErr_SetString(PyExc_AttributeError, "Empty Contour");
            return( NULL );
        }
	self->spiros = SplineSet2SpiroCP(ss,&cnt);
	self->spiro_cnt = cnt;
    }
    spirotuple = PyTuple_New(self->spiro_cnt-1);
    for ( i=0; i<self->spiro_cnt-1; ++i ) {
	int ty = self->spiros[i].ty & 0x7f;
	PyTuple_SetItem(spirotuple,i,Py_BuildValue("ddii",
		self->spiros[i].x, self->spiros[i].y,
		ty==SPIRO_G4 ? 1 :
		ty==SPIRO_G2 ? 2 :
		ty==SPIRO_CORNER ? 3 :
		ty==SPIRO_LEFT ? 4 :
		ty==SPIRO_RIGHT ? 5 : 6,
		(self->spiros[i].ty&0x80)>>7 ));
    }
return( spirotuple );
}

static int PyFF_Contour_set_spiros(PyFF_Contour *self,PyObject *value, void *UNUSED(closure)) {
    PyObject *spirotuple = value;
    int i, cnt;
    spiro_cp *spiros;
    SplineSet *ss;

    if ( !hasspiro()) {
	PyErr_Format(PyExc_EnvironmentError, "Spiros not available. Please install libspiro before continuing" );
return( -1 );
    }
    if ( !PySequence_Check(spirotuple)) {
	PyErr_Format(PyExc_TypeError, "Please specify a tuple of spiro control points" );
return( -1 );
    }
    cnt = PySequence_Size(spirotuple);
    if ( cnt < 1 ) {
	PyErr_Format(PyExc_TypeError, "Please specify a tuple of spiro control points" );
        return( -1 );
    }
    PyFFContour_ClearSpiros((PyFF_Contour *) self);
    spiros = malloc((cnt+1)*sizeof(spiro_cp));
    spiros[cnt].x = spiros[cnt].y = 0;
    spiros[cnt].ty = SPIRO_END;
    for ( i=0; i<cnt; ++i ) {
	double x,y; int type,flags=0;
	if ( !PyArg_ParseTuple(PySequence_GetItem(spirotuple,i),"ddi|i",&x,&y,&type,&flags)) {
	    PyErr_Format(PyExc_TypeError, "Please specify a tuple of spiro control points" );
	    free(spiros);
return( -1 );
	}
	spiros[i].x = x;
	spiros[i].y = y;
	if ( type==1 )
	    spiros[i].ty = SPIRO_G4;
	else if ( type==2 )
	    spiros[i].ty = SPIRO_G2;
	else if ( type==3 )
	    spiros[i].ty = SPIRO_CORNER;
	else if ( type==4 )
	    spiros[i].ty = SPIRO_LEFT;
	else if ( type==5 )
	    spiros[i].ty = SPIRO_RIGHT;
	else if ( type==6 && i==0 )
	    spiros[i].ty = SPIRO_OPEN_CONTOUR;
	else {
	    PyErr_Format(PyExc_TypeError, "Unknown spiro control point type: %d", type );
	    free(spiros);
return( -1 );
	}
	if ( flags==1 )
	    SPIRO_SELECT(&spiros[i]);
	else if ( flags!=0 ) {
	    PyErr_Format(PyExc_TypeError, "Unexpected value for flags: %d", flags );
	    free(spiros);
return( -1 );
	}
    }
    ss = SpiroCP2SplineSet(spiros);
    ss->spiros = NULL; ss->spiro_cnt = ss->spiro_max = 0;
    ContourFromSS(ss,self);
    self->spiro_cnt = cnt+1;
    self->spiros = spiros;
    SplinePointListFree(ss);
return( 0 );
}

static PyObject *PyFF_Contour_get_name(PyFF_Contour *self, void *UNUSED(closure)) {
    if ( self->name==NULL )
Py_RETURN_NONE;

return( Py_BuildValue("s", self->name ));
}

static int PyFF_Contour_set_name(PyFF_Contour *self,PyObject *value, void *UNUSED(closure)) {
    if (value == Py_None) {
        free(self->name);
        self->name = NULL;
    } else {
        char *name = copy(PyUnicode_AsUTF8(value));
        if (name == NULL) {
            return -1;
        }
        free(self->name);
        self->name = name;
    }
    return 0;
}

static PyGetSetDef PyFFContour_getset[] = {
    {(char *)"is_quadratic",
     (getter)PyFF_Contour_get_is_quadratic, (setter)PyFF_Contour_set_is_quadratic,
     (char *)"Whether this is an quadratic or cubic contour", NULL},
    {(char *)"closed",
     (getter)PyFF_Contour_get_closed, (setter)PyFF_Contour_set_closed,
     (char *)"Whether this is a closed contour", NULL},
    {(char *)"spiros",
     (getter)PyFF_Contour_get_spiros, (setter)PyFF_Contour_set_spiros,
     (char *)"Alternate representation of the contour as a tuple of spiro control points", NULL},
/* Sigh. I misdocumented the above entry and called it "spiro" so now support both names */
    {(char *)"spiro",
     (getter)PyFF_Contour_get_spiros, (setter)PyFF_Contour_set_spiros,
     (char *)"Alternate representation of the contour as a tuple of spiro control points", NULL},
    {(char *)"name",
     (getter)PyFF_Contour_get_name, (setter)PyFF_Contour_set_name,
     (char *)"Contours may be named", NULL},
    PYGETSETDEF_EMPTY /* Sentinel */
};

/* ************************************************************************** */
/* Contour sequence */
/* ************************************************************************** */
static Py_ssize_t PyFFContour_Length( PyObject *self ) {
return( ((PyFF_Contour *) self)->pt_cnt );
}

static PyFF_Contour *PyFFPointList_Parse(PyObject *args);

static PyObject *PyFFContour_Concat( PyObject *_c1, PyObject *_c2 ) {
    PyFF_Contour *c1 = (PyFF_Contour *) _c1, *c2 = (PyFF_Contour *)_c2;
    PyFF_Contour *self;
    int i;
    PyFF_Contour dummy;
    PyFF_Point *dummies[1];

    if ( PyType_IsSubtype(&PyFF_ContourType, Py_TYPE(c1)) &&
         PyType_IsSubtype(&PyFF_ContourType, Py_TYPE(c2)) ) {
	if ( c1->is_quadratic != c2->is_quadratic ) {
	    PyErr_Format(PyExc_TypeError, "Both contours must be either cubic or quadratic.");
	    return( NULL );
	}
    } else if ( ( dummies[0] = PyFFPoint_Parse(_c2, false, false) ) != NULL ) {
	memset(&dummy,0,sizeof(dummy));
	dummy.pt_cnt = 1;
	dummy.points = dummies;
	c2 = &dummy;
    } else if ( ( c2 = PyFFPointList_Parse(_c2) ) == NULL ) {
	PyErr_Format(PyExc_TypeError, "Both arguments must be contours or encode a point or point list");
    }

    self = (PyFF_Contour *)PyFF_ContourType.tp_alloc(&PyFF_ContourType, 0);
    self->is_quadratic = c1->is_quadratic;
    self->closed = c1->closed;
    self->pt_max = self->pt_cnt = c1->pt_cnt + c2->pt_cnt;
    self->points = PyMem_New(PyFF_Point *,self->pt_max);
    for ( i=0; i<c1->pt_cnt; ++i ) {
	Py_INCREF(c1->points[i]);
	self->points[i] = c1->points[i];
    }
    for ( i=0; i<c2->pt_cnt; ++i ) {
	if ( c2!=&dummy )
	    Py_INCREF(c2->points[i]);
	self->points[c1->pt_cnt+i] = c2->points[i];
    }
    if ( ((PyObject *)c2)!=_c2 && c2!=&dummy )
	PyFFContour_dealloc(c2);
    Py_RETURN( (PyObject *) self );
}

static PyObject *PyFFContour_InPlaceConcat( PyObject *_self, PyObject *_c2 ) {
    PyFF_Contour *self = (PyFF_Contour *) _self, *c2 = (PyFF_Contour *) _c2;
    int i, old_cnt;
    PyFF_Contour dummy;
    PyFF_Point *dummies[1];

    if ( PyType_IsSubtype(&PyFF_ContourType, Py_TYPE(self)) &&
         PyType_IsSubtype(&PyFF_ContourType, Py_TYPE(c2)) ) {
	if ( self->is_quadratic != c2->is_quadratic ) {
	    PyErr_Format(PyExc_TypeError, "Both contours must be either cubic or quadratic.");
	    return( NULL );
	}
    } else if ( ( dummies[0] = PyFFPoint_Parse(_c2, false, false) ) != NULL ) {
	memset(&dummy,0,sizeof(dummy));
	dummy.pt_cnt = 1;
	dummy.points = dummies;
	c2 = &dummy;
    } else if ( ( c2 = PyFFPointList_Parse(_c2) ) == NULL ) {
	PyErr_Format(PyExc_TypeError, "Both arguments must be contours or encode a point or point list");
    }

    old_cnt = self->pt_cnt;
    self->pt_max = self->pt_cnt = self->pt_cnt + c2->pt_cnt;
    PyMem_Resize(self->points,PyFF_Point *,self->pt_max); /* Messes with self->points */
    for ( i=0; i<c2->pt_cnt; ++i ) {
	if ( c2!=&dummy )
	    Py_INCREF(c2->points[i]);
	self->points[old_cnt+i] = c2->points[i];
    }
    if ( ((PyObject *)c2)!=_c2 && c2!=&dummy )
	PyFFContour_dealloc(c2);
    PyFFContour_ClearSpiros((PyFF_Contour *) self);
    Py_RETURN( self );
}

static PyObject *PyFFContour_Index( PyObject *self, Py_ssize_t pos ) {
    PyFF_Contour *cont = (PyFF_Contour *) self;
    PyObject *ret;

    if ( pos<-cont->pt_cnt || pos>=cont->pt_cnt ) {
	PyErr_Format(PyExc_TypeError, "Index out of bounds");
	return( NULL );
    }

    if (pos < 0)
	pos += cont->pt_cnt;

    ret = (PyObject *) cont->points[pos];
    Py_INCREF(ret);
return( ret );
}

static int PyFFContour_IndexAssign( PyObject *self, Py_ssize_t pos, PyObject *val ) {
    PyFF_Contour *cont = (PyFF_Contour *) self;
    PyFF_Point *old, *vpoint = NULL;
    int i;

    if ( val!=NULL && ( vpoint = PyFFPoint_Parse(val, false, false) ) == NULL ) {
	PyErr_Format(PyExc_TypeError, "Unknown point format");
	return( -1 );
    }
    if ( pos<-cont->pt_cnt || pos>=cont->pt_cnt ) {
	PyErr_Format(PyExc_TypeError, "Index out of bounds");
	return( -1 );
    }

    if (pos < 0)
	pos += cont->pt_cnt;

    old = cont->points[pos];

    if ( val==NULL ) {
	for ( i=pos; i<cont->pt_cnt-1; ++i )
	    cont->points[i] = cont->points[i+1];
	cont->pt_cnt -= 1;
    } else {
	/* Be consistent about allowing the point array to be temporarily inconsistent
	if ( cont->points[pos]->on_curve != vpoint->on_curve && !cont->is_quadratic ) {
	    PyErr_Format(PyExc_TypeError, "Replacement point must have the same on_curve setting as original in a cubic contour");
	    return( -1 );
	}
	*/
	// PyFFPoint_Parse already incremented refcount
	cont->points[pos] = vpoint;
    }
    PyFFContour_ClearSpiros((PyFF_Contour *) self);
    Py_DECREF( old );
    return( 0 );
}


static int PyFFContour_Contains(PyObject *_self, PyObject *_pt) {
    PyFF_Contour *self = (PyFF_Contour *) _self;
    double x,y;
    int i;

    if ( PySequence_Check(_pt)) {
	if ( !PyArg_ParseTuple(_pt,"dd", &x, &y ))
return( -1 );
    } else if ( !PyType_IsSubtype(&PyFF_PointType, Py_TYPE(_pt)) ) {
	PyErr_Format(PyExc_TypeError, "Value must be a (FontForge) Point");
return( -1 );
    } else {
	x = ((PyFF_Point *) _pt)->x;
	y = ((PyFF_Point *) _pt)->y;
    }

    for ( i=0; i<self->pt_cnt; ++i )
	if ( self->points[i]->x == x && self->points[i]->y == y )
return( 1 );

return( 0 );
}

static PySequenceMethods PyFFContour_Sequence = {
    PyFFContour_Length,		/* length */
    PyFFContour_Concat,		/* concat */
    NULL,			/* repeat */
    PyFFContour_Index,		/* subscript */
    NULL,			/* slice */
    PyFFContour_IndexAssign,	/* subscript assign */
    NULL,			/* slice assign */
    PyFFContour_Contains,	/* contains */
    PyFFContour_InPlaceConcat,	/* inplace_concat */
    NULL			/* inplace repeat */
};

static PyObject *PyFFContour_Sub( PyObject *self, PyObject *key ) {
    PyFF_Contour *cont = (PyFF_Contour *) self;
    PyFF_Contour *ret;
    Py_ssize_t start, end, step, len;
    int i;

    if ( PyLong_Check(key)) {
        return PyFFContour_Index(self, PyNumber_AsSsize_t(key, PyExc_IndexError));
    }
    if ( !PySlice_Check(key) ) {
	PyErr_Format(PyExc_IndexError, "Contour indexed by integer only");
	return( NULL );
    }
#if PY_VERSION_HEX > 0x03060100
    if (PySlice_Unpack(key, &start, &end, &step) < 0) {
	return( NULL );
    }
    len = PySlice_AdjustIndices(cont->pt_cnt, &start, &end, step);
#elif PY_VERSION_HEX > 0x03020000
    if (PySlice_GetIndicesEx(key, cont->pt_cnt, &start, &end, &step, &len) < 0) {
	return( NULL );
    }
#else
    if (PySlice_GetIndicesEx((PySliceObject *) key, cont->pt_cnt, &start, &end, &step, &len) < 0) {
	return( NULL );
    }
#endif
    if ( !(step == 1 || step == -1) ) {
	PyErr_Format(PyExc_IndexError, "Only supported steps are 1 and -1");
	return( NULL );
    }

    ret = (PyFF_Contour *)PyFF_ContourType.tp_alloc(&PyFF_ContourType, 0);
    ret->is_quadratic = cont->is_quadratic;
    ret->closed = false;
    ret->pt_max = ret->pt_cnt = len;
    ret->points = PyMem_New(PyFF_Point *,ret->pt_max);

    for ( i = 0; i < len; i++ ) {
	ret->points[i] = cont->points[i*step + start];
	Py_INCREF(ret->points[i]);
    }
    return( (PyObject *) ret );
}

// Caller responsible for incrementing reference count
static void PyFFContour_CInsertPoint(PyFF_Contour *self, PyFF_Point *p, int pos) {
	int i;

	if ( pos<0 || pos>=self->pt_cnt-1 )
		pos = self->pt_cnt-1;
	if ( self->pt_cnt >= self->pt_max ) {
		/* Messes with self->points */
		PyMem_Resize(self->points,PyFF_Point *,self->pt_max += 10);
	}
	for ( i=self->pt_cnt-1; i>pos; --i )
	self->points[i+1] = self->points[i];
	self->points[pos+1] = p;
	PyFFContour_ClearSpiros(self);
	self->pt_cnt += 1;
}

// Uses a Contour object as a temporary home for the point list
static PyFF_Contour *PyFFPointList_Parse(PyObject *args) {
	PyFF_Contour *self = (PyFF_Contour *) PyFFContour_new(&PyFF_ContourType,NULL,NULL);
	PyFF_Point *p;
	int len, i;
	bool ok = true;

	if ( PyType_IsSubtype(&PyFF_ContourType, Py_TYPE(args) ) ) {
		PyFFContour_InPlaceConcat((PyObject *) self, args);
	} else if ( PySequence_Check(args) ) {
		len = PySequence_Size(args);
		for (i = 0; i < len; ++i) {
			p = PyFFPoint_Parse(PySequence_GetItem(args, i), false, false);
			if ( p!=NULL ) {
				PyFFContour_CInsertPoint(self, p, -1);
			} else {
				ok = false;
				break;
			}
		}
	} else {
		ok = false;
	}
	if ( ! ok ) {
		PyFFContour_dealloc(self);
		return NULL;
	}
	return self;
}

static int PyFFContour_SubAssign( PyObject *self, PyObject *key, PyObject *val ) {
    PyFF_Contour *cont = (PyFF_Contour *) self, *rpl;
    int i, diff;
    Py_ssize_t len, start, end, step;

    if ( PyLong_Check(key)) {
        return PyFFContour_IndexAssign(self, PyNumber_AsSsize_t(key, PyExc_IndexError), val);
    }
    rpl = PyFFPointList_Parse(val);
    if ( rpl==NULL ) {
	PyErr_Format(PyExc_TypeError, "Replacement must encode a point list");
	return( -1 );
    }
    if ( !PySlice_Check(key) ) {
	PyErr_Format(PyExc_IndexError, "Contour indexed by integer only");
	return( -1 );
    }
#if PY_VERSION_HEX > 0x03060100
    if (PySlice_Unpack(key, &start, &end, &step) < 0) {
	return( -1 );
    }
    len = PySlice_AdjustIndices(cont->pt_cnt, &start, &end, step);
#elif PY_VERSION_HEX > 0x03020000
    if (PySlice_GetIndicesEx(key, cont->pt_cnt, &start, &end, &step, &len) < 0) {
	return( -1 );
    }
#else
    if (PySlice_GetIndicesEx((PySliceObject *) key, cont->pt_cnt, &start, &end, &step, &len) < 0) {
	return( -1 );
    }
#endif
    if ( !(step == 1 || step == -1) ) {
	PyErr_Format(PyExc_IndexError, "Only supported steps are 1 and -1");
	return( -1 );
    }

    diff = ( rpl!=NULL ? rpl->pt_cnt : 0 ) - len;
    for ( i=start; i<end; ++i )
	Py_DECREF(cont->points[i]);
    if ( diff>0 ) {
	if ( cont->pt_cnt+diff >= cont->pt_max ) {
	    cont->pt_max = cont->pt_cnt + diff;
	    PyMem_Resize(cont->points,PyFF_Point *,cont->pt_max); /* Messes with cont->points */
	}
	for ( i=cont->pt_cnt-1; i>=end; --i )
	    cont->points[i+diff] = cont->points[i];
    } else if ( diff<0 ) {
	for ( i=end; i<cont->pt_cnt; ++i )
	    cont->points[i+diff] = cont->points[i];
    }
    cont->pt_cnt += diff;
    for ( i=0; i<rpl->pt_cnt; ++i ) {
	cont->points[i*step+start] = rpl->points[i];
	Py_INCREF(rpl->points[i]);
    }
    PyFFContour_ClearSpiros(cont);
    PyFFContour_dealloc(rpl);
    return( 0 );
}

static PyMappingMethods PyFFContour_Mapping = {
    PyFFContour_Length,			/* length */
    PyFFContour_Sub,			/* subscript */
    PyFFContour_SubAssign		/* subscript assign */
};

/* ************************************************************************** */
/* Contour methods */
/* ************************************************************************** */
static PyObject *PyFFContour_dup(PyFF_Contour *self) {
    /* Arg checking done elsewhere */
    int i;
    PyFF_Contour *ret = (PyFF_Contour *)PyFF_ContourType.tp_alloc(&PyFF_ContourType, 0);
    ret->pt_max = ret->pt_cnt = self->pt_cnt;
    ret->spiro_cnt = self->spiro_cnt;
    ret->name = copy(self->name);
    ret->is_quadratic = self->is_quadratic;
    ret->closed = self->closed;
    if ( ret->pt_cnt!=0 ) {
	ret->points = PyMem_New(PyFF_Point *,ret->pt_cnt);
	for ( i=0; i<ret->pt_cnt; ++i )
	    ret->points[i] = (PyFF_Point *) PyFFPoint_dup(self->points[i]);
    }
    if ( ret->spiro_cnt!=0 ) {
	ret->spiros = malloc(ret->pt_cnt*sizeof(spiro_cp));
	memcpy(ret->spiros,self->spiros,self->spiro_cnt*sizeof(spiro_cp));
    }
return( (PyObject *) ret );
}

static PyObject *PyFFContour_IsEmpty(PyFF_Contour *self) {
    /* Arg checking done elsewhere */
return( Py_BuildValue("i",self->pt_cnt==0 ) );
}

static PyObject *PyFFContour_Start(PyFF_Contour *self, PyObject *args) {
    double x,y;

    if ( self->pt_cnt!=0 ) {
	PyErr_SetString(PyExc_AttributeError, "Contour not empty");
return( NULL );
    }
    if ( !PyArg_ParseTuple( args, "dd", &x, &y ))
return( NULL );
    if ( 1>self->pt_max ) {
        /* Messes with self->points */
        PyMem_Resize(self->points,PyFF_Point *,self->pt_max += 10);
    }
    self->points[0] = PyFFPoint_CNew(x,y,true,false,0,NULL);
    self->pt_cnt = 1;
    PyFFContour_ClearSpiros((PyFF_Contour *) self);

Py_RETURN( self );
}

static PyObject *PyFFContour_LineTo(PyFF_Contour *self, PyObject *args) {
    double x,y;
    int pos = -1, i;

    if ( self->pt_cnt==0 ) {
	PyErr_SetString(PyExc_AttributeError, "Contour empty");
return( NULL );
    }
    if ( !PyArg_ParseTuple( args, "dd|i", &x, &y, &pos )) {
	PyErr_Clear();
	if ( !PyArg_ParseTuple( args, "(dd)|i", &x, &y, &pos ))
return( NULL );
    }
    if ( pos<0 || pos>=self->pt_cnt-1 )
	pos = self->pt_cnt-1;
    while ( pos>=0 && !self->points[pos]->on_curve )
	--pos;
    if ( pos<0 ) {
	PyErr_SetString(PyExc_AttributeError, "Contour contains no on-curve points");
return( NULL );
    }
    if ( self->pt_cnt >= self->pt_max ) {
        /* Messes with self->points */
        PyMem_Resize(self->points,PyFF_Point *,self->pt_max += 10);
    }
    for ( i=self->pt_cnt-1; i>pos; --i )
	self->points[i+1] = self->points[i];
    self->points[pos+1] = PyFFPoint_CNew(x,y,true,false,0,NULL);
    PyFFContour_ClearSpiros((PyFF_Contour *) self);
    ++self->pt_cnt;

Py_RETURN( self );
}

static PyObject *PyFFContour_CubicTo(PyFF_Contour *self, PyObject *args) {
    double x[3],y[3];
    PyFF_Point *np, *pp, *p;
    int pos=-1, i;

    if ( self->is_quadratic || self->pt_cnt==0 ) {
	PyErr_SetString(PyExc_AttributeError, "Contour quadratic, or empty");
return( NULL );
    }
    if ( !PyArg_ParseTuple( args, "(dd)(dd)(dd)|i", &x[0], &y[0], &x[1], &y[1], &x[2], &y[2], &pos )) {
	PyErr_Clear();
	if ( !PyArg_ParseTuple( args, "dddddd|i", &x[0], &y[0], &x[1], &y[1], &x[2], &y[2], &pos ))
return( NULL );
    }
    np = PyFFPoint_CNew(x[0],y[0],false,false,0,NULL);
    pp = PyFFPoint_CNew(x[1],y[1],false,false,0,NULL);
    p = PyFFPoint_CNew(x[2],y[2],true,false,0,NULL);
    if ( p==NULL ) {
	Py_XDECREF(pp);
	Py_XDECREF(np);
return( NULL );
    }

    if ( pos<0 || pos>=self->pt_cnt-1 )
	pos = self->pt_cnt-1;
    while ( pos>=0 && !self->points[pos]->on_curve )
	--pos;
    if ( pos<0 ) {
	PyErr_SetString(PyExc_AttributeError, "Contour contains no on-curve points");
return( NULL );
    }
    if ( self->pt_cnt+3 >= self->pt_max ) {
        /* Messes with self->points */
        PyMem_Resize(self->points,PyFF_Point *,self->pt_max += 10);
    }
    for ( i=self->pt_cnt-1; i>pos; --i )
	self->points[i+3] = self->points[i];
    self->points[pos+1] = np;
    self->points[pos+2] = pp;
    self->points[pos+3] = p;
    PyFFContour_ClearSpiros((PyFF_Contour *) self);
    self->pt_cnt += 3;
Py_RETURN( self );
}


static PyObject *PyFFContour_QuadraticTo(PyFF_Contour *self, PyObject *args) {
    double x[2],y[2];
    PyFF_Point *cp, *p;
    int pos=-1, i;

    if ( !self->is_quadratic || self->pt_cnt==0 ) {
	PyErr_SetString(PyExc_AttributeError, "Contour cubic, or empty");
return( NULL );
    }
    if ( !PyArg_ParseTuple( args, "(dd)(dd)|i", &x[0], &y[0], &x[1], &y[1], &pos )) {
	PyErr_Clear();
	if ( !PyArg_ParseTuple( args, "dddd|i", &x[0], &y[0], &x[1], &y[1], &pos ))
return( NULL );
    }
    cp = PyFFPoint_CNew(x[0],y[0],false,false,0,NULL);
    p = PyFFPoint_CNew(x[1],y[1],true,false,0,NULL);
    if ( p==NULL ) {
	Py_XDECREF(cp);
return( NULL );
    }

    if ( pos<0 || pos>=self->pt_cnt-1 )
	pos = self->pt_cnt-1;
    while ( pos>=0 && !self->points[pos]->on_curve )
	--pos;
    if ( pos<0 ) {
	PyErr_SetString(PyExc_AttributeError, "Contour contains no on-curve points");
return( NULL );
    }
    if ( self->pt_cnt+2 >= self->pt_max ) {
        /* Messes with self->points */
        PyMem_Resize(self->points,PyFF_Point *,self->pt_max += 10);
    }
    for ( i=self->pt_cnt-1; i>pos; --i )
	self->points[i+2] = self->points[i];
    self->points[pos+1] = cp;
    self->points[pos+2] = p;
    self->pt_cnt += 2;
    PyFFContour_ClearSpiros((PyFF_Contour *) self);
Py_RETURN( self );
}

static PyObject *PyFFContour_InsertPoint(PyFF_Contour *self, PyObject *args) {
    double x,y;
    PyFF_Point *p=NULL;
    int on, pos, sel, type;

    x = y = 0.0;
    pos = -1;
    on = true;
    sel = false;
    type = PyFF_ConvertFromPointType(pt_corner);
    if ( !PyArg_ParseTuple( args, "(ddiii)|i", &x, &y, &on, &type, &sel, &pos )) {
	PyErr_Clear();
    if ( !PyArg_ParseTuple( args, "(ddii)|i", &x, &y, &on, &type, &pos )) {
	PyErr_Clear();
	if ( !PyArg_ParseTuple( args, "(ddi)|i", &x, &y, &on, &pos )) {
	    PyErr_Clear();
	    if ( !PyArg_ParseTuple( args, "(dd)|i", &x, &y, &pos )) {
		PyErr_Clear();
		if ( !PyArg_ParseTuple( args, "O|i", &p, &pos ) ||
		     !PyType_IsSubtype(&PyFF_PointType, Py_TYPE(p)) ) {
		    return( NULL );
		}
	    }
	}
    }
    }

    if ( p==NULL ) {
	p = PyFFPoint_CNew(x,y,on,sel,type,NULL);
	if ( p==NULL )
	    return( NULL );
    } else {
	Py_INCREF( p );
    }

    PyFFContour_CInsertPoint(self, p, pos);
    Py_RETURN( self );
}

static PyObject *PyFFContour_MakeFirst(PyFF_Contour *self, PyObject *args) {
    int pos = -1, off;
    int i;
    PyFF_Point **temp, **old;

    if ( !PyArg_ParseTuple( args, "i", &pos ))
	return( NULL );
    if ( pos<0 || pos>=self->pt_cnt ) {
	PyErr_Format(PyExc_TypeError, "Position argument out of bounds");
	return( NULL );
    }
    if ( ! self->points[pos]->on_curve ) {
	PyErr_Format(PyExc_TypeError, "First point must be on curve");
	return( NULL );
    }

    temp = PyMem_New(PyFF_Point *,self->pt_max);
    old = self->points;
    for ( i=pos; i<self->pt_cnt; ++i )
	temp[i-pos] = old[i];
    off = i-pos;
    for ( i=0; i<pos; ++i )
	temp[i+off] = old[i];
    self->points = temp;
    PyMem_Del(old);
    PyFFContour_ClearSpiros((PyFF_Contour *) self);

Py_RETURN( self );
}

static PyObject *PyFFContour_ReverseDirection(PyFF_Contour *self, PyObject *UNUSED(args)) {
    int i, j;
    PyFF_Point **temp, **old;

    /* Arg checking done elsewhere */

    temp = PyMem_New(PyFF_Point *,self->pt_max);
    old = self->points;
    if ( self->closed ) {
	temp[0] = old[0];
	for ( i=self->pt_cnt-1, j=1; i>0; --i, ++j )
	    temp[j] = old[i];
    } else {
	for ( i=self->pt_cnt-1, j=0; i>=0; --i, ++j )
	    temp[j] = old[i];
    }
    self->points = temp;
    PyMem_Del(old);
    PyFFContour_ClearSpiros((PyFF_Contour *) self);

Py_RETURN( self );
}

static PyObject *PyFFContour_IsClockwise(PyFF_Contour *self, PyObject *UNUSED(args)) {
    SplineSet *ss;
    int ret;

    /* Arg checking done elsewhere */

    ss = SSFromContour(self,NULL);
    if ( ss==NULL ) {
	if ( PyErr_Occurred() == NULL )
	    PyErr_SetString(PyExc_AttributeError, "Empty Contour");
	return( NULL );
    }
    ret = SplinePointListIsClockwise(ss);
    SplinePointListFree(ss);
return( Py_BuildValue("i",ret ) );
}

static PyObject *PyFFContour_xBoundsAtY(PyFF_Contour *self, PyObject *args) {
    SplineSet *ss;
    double y1, y2=6.023e23;
    bigreal xmin, xmax;
    int ret;

    if ( !PyArg_ParseTuple(args,"d|d", &y1, &y2 ))
return( NULL );

    ss = SSFromContour(self,NULL);
    if ( ss==NULL )
Py_RETURN_NONE;

    if ( y2>1e23 )
	y2 = y1;
    ret = SSBoundsWithin(ss,y1,y2,&xmin,&xmax,1);
    SplinePointListFree(ss);
    if ( !ret )
Py_RETURN_NONE;

return( Py_BuildValue("(dd)",(double) xmin, (double) xmax ) );
}

static PyObject *PyFFContour_yBoundsAtX(PyFF_Contour *self, PyObject *args) {
    SplineSet *ss;
    double x1, x2=6.023e23;
    bigreal ymin, ymax;
    int ret;

    if ( !PyArg_ParseTuple(args,"d|d", &x1, &x2 ))
return( NULL );

    ss = SSFromContour(self,NULL);
    if ( ss==NULL )
Py_RETURN_NONE;

    if ( x2>1e23 )
	x2 = x1;
    ret = SSBoundsWithin(ss,x1,x2,&ymin,&ymax,0);
    SplinePointListFree(ss);
    if ( !ret )
Py_RETURN_NONE;

return( Py_BuildValue("(dd)",(double) ymin, (double) ymax ) );
}

static PyObject *PyFFContour_Merge(PyFF_Contour *self, PyObject *args) {
    SplineSet *ss;
    int i, pos;

    ss = SSFromContour(self,NULL);
    if ( ss==NULL ) {
	if ( PyErr_Occurred() == NULL )
	    PyErr_SetString(PyExc_AttributeError, "Empty Contour");
	return( NULL );
    }
    for ( i=0; i<PySequence_Size(args); ++i ) {
	pos = PyLong_AsLong(PySequence_GetItem(args,i));
	if ( PyErr_Occurred())
return( NULL );
	SSSelectOnCurve(ss,pos);
    }
    SplineCharMerge(NULL,&ss,1);
    if ( ss==NULL ) {
	for ( i=0; i<self->pt_cnt; ++i )
	    Py_DECREF(self->points[i]);
	self->pt_cnt = 0;
    } else {
	ContourFromSS(ss,self);
	SplinePointListFree(ss);
    }
    PyFFContour_ClearSpiros((PyFF_Contour *) self);
Py_RETURN( self );
}

/* Simplify flags: see 'enum simplify_flags' in splinefont.h */
struct flaglist simplifyflags[] = {
    { "cleanup", sf_cleanup },
    { "ignoreslopes", sf_ignoreslopes },
    { "ignoreextrema", sf_ignoreextremum },
    { "smoothcurves", sf_smoothcurves },
    { "choosehv", sf_choosehv },
    { "forcelines", sf_forcelines },
    { "nearlyhvlines", sf_nearlyhvlines },
    { "mergelines", sf_mergelines },
    { "setstarttoextremum", sf_setstart2extremum },
    { "setstarttoextrema",  sf_setstart2extremum }, /* Documentation error */
    { "removesingletonpoints", sf_rmsingletonpoints },
    FLAGLIST_EMPTY /* Sentinel */
};

static PyObject *PyFFContour_selfIntersects(PyFF_Contour *self, PyObject *UNUSED(args)) {
    SplineSet *ss;
    Spline *s, *s2;
    PyObject *ret;

    ss = SSFromContour(self,NULL);
    ret = SplineSetIntersect(ss,&s,&s2) ? Py_True : Py_False;
    SplinePointListFree(ss);
    Py_INCREF( ret );
return( ret );
}

static PyObject *PyFFContour_Simplify(PyFF_Contour *self, PyObject *args) {
    SplineSet *ss;
    static struct simplifyinfo smpl = { sf_normal, 0.75, 0.2, 10, 0, 0, 0 };
    int i;

    smpl.err = 1;
    smpl.linefixup = 2;
    smpl.linelenmax = 10;

    ss = SSFromContour(self,NULL);
    if ( ss==NULL ) {
	if ( PyErr_Occurred() != NULL )
	    return ( NULL );
	else
	    Py_RETURN( self );	/* As simple as it can be */
    }

    if ( PySequence_Size(args)>=1 )
	smpl.err = PyFloat_AsDouble(PySequence_GetItem(args,0));
    if ( !PyErr_Occurred() && PySequence_Size(args)>=2 )
	smpl.flags = FlagsFromTuple( PySequence_GetItem(args,1),simplifyflags,"simplify flag");
    if ( !PyErr_Occurred() && PySequence_Size(args)>=3 )
	smpl.tan_bounds = PyFloat_AsDouble( PySequence_GetItem(args,2));
    if ( !PyErr_Occurred() && PySequence_Size(args)>=4 )
	smpl.linefixup = PyFloat_AsDouble( PySequence_GetItem(args,3));
    if ( !PyErr_Occurred() && PySequence_Size(args)>=5 )
	smpl.linelenmax = PyFloat_AsDouble( PySequence_GetItem(args,4));
    if ( PyErr_Occurred() )
return( NULL );
    SplinePointListSimplify(NULL,ss,&smpl);
    if ( ss==NULL ) {
	for ( i=0; i<self->pt_cnt; ++i )
	    Py_DECREF(self->points[i]);
	self->pt_cnt = 0;
    } else {
	ContourFromSS(ss,self);
	SplinePointListFree(ss);
    }
Py_RETURN( self );
}

static PyObject *PyFFContour_Transform(PyFF_Contour *self, PyObject *args) {
    int i;
    double m[6];

    if ( !PyArg_ParseTuple(args,"(dddddd)",&m[0], &m[1], &m[2], &m[3], &m[4], &m[5]) )
return( NULL );
    for ( i=0; i<self->pt_cnt; ++i )
	PyFF_TransformPoint(self->points[i],m);
    PyFFContour_ClearSpiros((PyFF_Contour *) self);
Py_RETURN( self );
}

static PyObject *PyFFContour_similar(PyFF_Contour *self, PyObject *args) {
    double pt_err = -1, spline_err = -1;
    int ret;
    PyObject *other, *retO;

    if ( !PyArg_ParseTuple(args,"O|dd", &other, &pt_err, &spline_err) )
return( NULL );
    if ( pt_err==-1 ) {
	pt_err = .5;
	spline_err = pt_err;
    } else if ( spline_err==-1 )
	spline_err = pt_err;

    if ( !PyType_IsSubtype(&PyFF_ContourType, Py_TYPE(other)) ) {
	PyErr_Format(PyExc_TypeError, "Unexpected type");
return( NULL );
    }

    ret = PyFFContour_docompare(self,other,pt_err,spline_err);
    retO = (ret&SS_NoMatch) ? Py_False : Py_True;
    Py_INCREF( retO );
return( retO );
}

static PyObject *PyFFContour_pickleReducer(PyFF_Contour *self, PyObject *UNUSED(args)) {
    PyObject *reductionTuple, *argTuple;
    int i;

    if ( _new_contour==NULL )
	PyFF_PickleTypesInit();
    reductionTuple = PyTuple_New(2);
    Py_INCREF(_new_contour);
    PyTuple_SetItem(reductionTuple,0,_new_contour);
    argTuple = PyTuple_New(2+self->pt_cnt);
    PyTuple_SetItem(reductionTuple,1,argTuple);
    PyTuple_SetItem(argTuple,0,Py_BuildValue("i", self->is_quadratic));
    PyTuple_SetItem(argTuple,1,Py_BuildValue("i", self->closed));
    for ( i=0; i<self->pt_cnt; ++i ) {
	Py_INCREF((PyObject *)self->points[i]);
	PyTuple_SetItem(argTuple,2+i,(PyObject *) self->points[i]);
    }
return( reductionTuple );
}

static PyObject *PyFFContour_Round(PyFF_Contour *self, PyObject *args) {
    double factor=1;
    int i;

    if ( !PyArg_ParseTuple(args,"|d",&factor ) )
return( NULL );
    for ( i=0; i<self->pt_cnt; ++i ) {
	self->points[i]->x = rint( factor*(double)self->points[i]->x )/factor;
	self->points[i]->y = rint( factor*(double)self->points[i]->y )/factor;
    }
    PyFFContour_ClearSpiros((PyFF_Contour *) self);
Py_RETURN( self );
}

static PyObject *PyFFContour_Cluster(PyFF_Contour *self, PyObject *args) {
    double within = .1, max = .5;
    SplineChar sc;
    SplineSet *ss;
    Layer layers[2];

    if ( !PyArg_ParseTuple(args,"|dd", &within, &max ) )
return( NULL );

    ss = SSFromContour(self,NULL);
    if ( ss==NULL ) {
	if ( PyErr_Occurred() != NULL )
	    return ( NULL );
	else
	    Py_RETURN( self );	// no points=> no clusters
    }

    memset(&sc,0,sizeof(sc));
    memset(layers,0,sizeof(layers));
    sc.layers = layers;
    sc.layers[ly_fore].splines = ss;
    sc.name = copy("nameless");
    SCRoundToCluster( &sc,ly_fore,false,within,max);
    ContourFromSS(sc.layers[ly_fore].splines,self);
    SplinePointListFree(sc.layers[ly_fore].splines);
    PyFFContour_ClearSpiros((PyFF_Contour *) self);
Py_RETURN( self );
}

/* Add Extrema type: see 'enum ae_type' in splinefont.h */
struct flaglist addextremaflags[] = {
    { "all", ae_all },
    /*{ "", ae_between_selected },  -- Not needed for python */
    { "only_good", ae_only_good },
    { "only_good_rm", ae_only_good_rm_later },
    FLAGLIST_EMPTY /* Sentinel */
};

static PyObject *PyFFContour_AddExtrema(PyFF_Contour *self, PyObject *args) {
    int emsize = 1000;
    char *flag = NULL;
    int ae = ae_only_good;
    SplineSet *ss;

    if ( !PyArg_ParseTuple(args,"|si", &flag, &emsize ) )
return( NULL );
    if ( flag!=NULL ) {
	ae = FlagsFromString(flag,addextremaflags,"extrema flag");
	if ( ae==FLAG_UNKNOWN )
return( NULL );
    }

    ss = SSFromContour(self,NULL);
    if ( ss==NULL ) {
	if ( PyErr_Occurred() != NULL )
	    return ( NULL );
	else
	    Py_RETURN( self );	// no points=> nothing to do
    }
    SplineSetAddExtrema(NULL,ss,ae,emsize);
    ContourFromSS(ss,self);
    SplinePointListFree(ss);
Py_RETURN( self );
}

static PyObject *_PyFFContour_Action(PyFF_Contour *self, void (*func)(SplineChar*, SplineSet*, int)) { 
    SplineSet *ss;
    ss = SSFromContour(self,NULL);
    if ( ss==NULL ) {
	if ( PyErr_Occurred() != NULL )
	    return ( NULL );
	else
	    Py_RETURN( self );	// no points=> nothing to do
    }
    func(NULL,ss,false); 
    ContourFromSS(ss,self);
    SplinePointListFree(ss);
Py_RETURN( self );
}

static PyObject *PyFFContour_AddInflections(PyFF_Contour *self) {
    return _PyFFContour_Action(self,&SplineSetAddInflections);
}

static PyObject *PyFFContour_Balance(PyFF_Contour *self) {
    return _PyFFContour_Action(self,&SplineSetBalance);
}

static PyObject *PyFFContour_Harmonize(PyFF_Contour *self) {
    return _PyFFContour_Action(self,&SplineSetHarmonize);
}

static PyObject *PyFFContour_BoundingBox(PyFF_Contour *self, PyObject *UNUSED(args)) {
    double xmin, xmax, ymin, ymax;
    int i;

    if ( self->pt_cnt==0 )
        return( Py_BuildValue("(dddd)", 0.0, 0.0, 0.0, 0.0 ));

    xmin = xmax = self->points[0]->x;
    ymin = ymax = self->points[0]->y;
    for ( i=1; i<self->pt_cnt; ++i ) {
	if ( self->points[i]->x < xmin ) xmin = self->points[i]->x;
	if ( self->points[i]->x > xmax ) xmax = self->points[i]->x;
	if ( self->points[i]->y < ymin ) ymin = self->points[i]->y;
	if ( self->points[i]->y > ymax ) ymax = self->points[i]->y;
    }
return( Py_BuildValue("(dddd)", xmin, ymin, xmax, ymax ));
}

static PyObject *PyFFContour_GetSplineAfterPoint(PyFF_Contour *self, PyObject *args) {
    int pnum,prev;
    BasePoint start, ncp, pcp, end;
    double cx,cy, bx,by;

    if ( !PyArg_ParseTuple(args,"i", &pnum ) )
return( NULL );
    if ( pnum>=self->pt_cnt ) {
	PyErr_Format(PyExc_ValueError, "Point index out of range" );
return( NULL );
    }
    if ( self->is_quadratic ) {
	if ( self->points[pnum]->on_curve ) {
	    start.x = self->points[pnum]->x; start.y = self->points[pnum]->y;
	    if ( ++pnum>=self->pt_cnt )
		pnum = 0;
	    if ( self->points[pnum]->on_curve ) {
		end.x = self->points[pnum]->x; end.y = self->points[pnum]->y;
                return( Py_BuildValue("((dddd)(dddd))",
                                        0.0, 0.0,end.x-start.x,start.x,
                                        0.0, 0.0,end.y-start.y,start.y ));
	    } else {
		ncp.x = self->points[pnum]->x; ncp.y = self->points[pnum]->y;
		if ( ++pnum>=self->pt_cnt )
		    pnum = 0;
		if ( self->points[pnum]->on_curve ) {
		    end.x = self->points[pnum]->x; end.y = self->points[pnum]->y;
		} else {
		    end.x = ((double)self->points[pnum]->x+ncp.x)/2; end.y = ((double)self->points[pnum]->y+ncp.y)/2;
		}
	    }
	} else {
	    ncp.x = self->points[pnum]->x; ncp.y = self->points[pnum]->y;
	    if ( ( prev = pnum-1 )<0 ) prev = self->pt_cnt-1;
	    if ( self->points[prev]->on_curve ) {
		start.x = self->points[prev]->x; start.y = self->points[prev]->y;
	    } else {
		start.x = ((double)self->points[prev]->x+ncp.x)/2; start.y = ((double)self->points[prev]->y+ncp.y)/2;
	    }
	    if ( ++pnum>=self->pt_cnt )
		pnum = 0;
	    if ( self->points[pnum]->on_curve ) {
		end.x = self->points[pnum]->x; end.y = self->points[pnum]->y;
	    } else {
		end.x = ((double)self->points[pnum]->x+ncp.x)/2; end.y = ((double)self->points[pnum]->y+ncp.y)/2;
	    }
	}
	cx = 2*(ncp.x-start.x); cy = 2*(ncp.y-start.y);
        return( Py_BuildValue("((dddd)(dddd))", 0.0,end.x-start.x-cx,cx,start.x,
                                                0.0,end.y-start.y-cy,cy,start.y ));
    } else {
	if ( !self->points[pnum]->on_curve ) {
	    if ( ( --pnum )<0 ) pnum = self->pt_cnt-1;
	    if ( !self->points[pnum]->on_curve ) {
		if ( ( --pnum )<0 ) pnum = self->pt_cnt-1;
	    }
	}
	start.x = self->points[pnum]->x; start.y = self->points[pnum]->y;
	if ( ++pnum>=self->pt_cnt ) pnum = 0;
	if ( self->points[pnum]->on_curve ) {
	    end.x = self->points[pnum]->x; end.y = self->points[pnum]->y;
	    return( Py_BuildValue("((dddd)(dddd))", 0.0,0.0,end.x-start.x,start.x, 0.0,0.0,end.y-start.y,start.y ));
	}
	ncp.x = self->points[pnum]->x; ncp.y = self->points[pnum]->y;
	if ( ++pnum>=self->pt_cnt ) pnum = 0;
	pcp.x = self->points[pnum]->x; pcp.y = self->points[pnum]->y;
	if ( ++pnum>=self->pt_cnt ) pnum = 0;
	end.x = self->points[pnum]->x; end.y = self->points[pnum]->y;
	cx = 3*(ncp.x-start.x); cy = 3*(ncp.y-start.y);
	bx = 3*(pcp.x-ncp.x)-cx; by = 3*(pcp.y-ncp.y)-cy;
return( Py_BuildValue("((dddd)(dddd))", end.x-start.x-cx-bx,bx,cx,start.x,
					end.y-start.y-cy-by,by,cy,start.y ));
    }
}

static void do_pycall(PyObject *obj,const char *method,PyObject *args_tuple) {
    PyObject *func, *result;

    func = PyObject_GetAttrString(obj,method);	/* I hope this is right */
    if ( func==NULL ) {
        fprintf( stderr, "Failed to find %s in %s\n", method, Py_TYPENAME(obj) );
	Py_DECREF(args_tuple);
return;
    }
    if (!PyCallable_Check(func)) {
	PyErr_Format(PyExc_TypeError, "Method, %s, is not callable", method );
	Py_DECREF(args_tuple);
	Py_DECREF(func);
return;
    }
    result = PyObject_CallObject(func, args_tuple);
    Py_DECREF(args_tuple);
    Py_XDECREF(result);
    Py_DECREF(func);
    if ( PyErr_Occurred()!=NULL )
	PyErr_Print();
}

static PyObject *PointTuple(PyFF_Point *pt) {
    PyObject *pt_tuple = PyTuple_New(2);

    PyTuple_SetItem(pt_tuple,0,Py_BuildValue("d",(double)pt->x));
    PyTuple_SetItem(pt_tuple,1,Py_BuildValue("d",(double)pt->y));
return( pt_tuple );
}

static PyObject *PyFFContour_draw(PyFF_Contour *self, PyObject *args) {
    PyObject *pen, *tuple;
    PyFF_Point **points;
    int i, start, off, last, j;

    if ( !PyArg_ParseTuple(args,"O", &pen ) )
return( NULL );
    if ( self->pt_cnt<2 )
Py_RETURN( self );

    points = self->points;
    /* The pen protocol demands that we start with an on-curve point */
    /* this means we may need to rotate our point list. */
    /* The pen protocol allows a contour of entirely off curve quadratic points */
    /*  so make a special check for that (can't occur in cubics) */
    for ( start=0; start<self->pt_cnt; ++start )
	if ( points[start]->on_curve )
    break;
    if ( start==self->pt_cnt ) {
	if ( self->is_quadratic ) {
	    tuple = PyTuple_New(self->pt_cnt+1);
	    for ( i=0; i<self->pt_cnt; ++i ) {
		PyTuple_SetItem(tuple,i,PointTuple(points[i]));
	    }
	    PyTuple_SetItem(tuple,i,Py_None); Py_INCREF(Py_None);
	    do_pycall(pen,"qCurveTo",tuple);
	} else {
	    PyErr_Format(PyExc_TypeError, "A cubic contour must have at least one oncurve point to be drawn" );
return( NULL );
	}
    } else {
	if ( start!=0 ) {
	    points = malloc(self->pt_cnt*sizeof(PyFF_Point *));
	    for ( i=start; i<self->pt_cnt; ++i )
		points[i-start] = self->points[i];
	    off = self->pt_cnt - start;
	    for ( i=0; i<start; ++i )
		points[i+off] = self->points[i];
	}

	tuple = PyTuple_New(1);
	PyTuple_SetItem(tuple,0,PointTuple(points[0]));
	do_pycall(pen,"moveTo",tuple);
	if ( PyErr_Occurred()) {
            free(points);
return( NULL );
        }
	last = 0;
	for ( i=1; i<self->pt_cnt; ++i ) {
	    if ( !points[i]->on_curve )
	continue;
	    if ( i-last==1 ) {
		tuple = PyTuple_New(1);
		PyTuple_SetItem(tuple,0,PointTuple(points[i]));
		do_pycall(pen,"lineTo",tuple);
	    } else if ( i-last==3 && !self->is_quadratic ) {
		tuple = PyTuple_New(3);
		PyTuple_SetItem(tuple,0,PointTuple(points[i-2]));
		PyTuple_SetItem(tuple,1,PointTuple(points[i-1]));
		PyTuple_SetItem(tuple,2,PointTuple(points[i]));
		do_pycall(pen,"curveTo",tuple);
	    } else if ( self->is_quadratic ) {
		tuple = PyTuple_New(i-last);
		for ( j=last+1; j<=i; ++j )
		    PyTuple_SetItem(tuple,j-(last+1),PointTuple(points[j]));
		do_pycall(pen,"qCurveTo",tuple);
	    } else {
		PyErr_Format(PyExc_TypeError, "Wrong number of off-curve points on a cubic contour");
return( NULL );
	    }
	    if ( PyErr_Occurred())
return( NULL );
	    last = i;
	}
	if ( i-last==3 && !self->is_quadratic && self->closed ) {
	    tuple = PyTuple_New(3);
	    PyTuple_SetItem(tuple,0,PointTuple(points[i-2]));
	    PyTuple_SetItem(tuple,1,PointTuple(points[i-1]));
	    PyTuple_SetItem(tuple,2,PointTuple(points[0]));
	    do_pycall(pen,"curveTo",tuple);
	} else if ( i-last!=1 && self->is_quadratic && self->closed ) {
	    tuple = PyTuple_New(i-last);
	    for ( j=last+1; j<i; ++j )
		PyTuple_SetItem(tuple,j-(last+1),PointTuple(points[j]));
	    PyTuple_SetItem(tuple,j-(last+1),PointTuple(points[0]));
	    do_pycall(pen,"qCurveTo",tuple);
	}
    }

    if ( PyErr_Occurred())
return( NULL );

    tuple = PyTuple_New(0);
    if ( self->closed )
	do_pycall(pen,"closePath",tuple);
    else
	do_pycall(pen,"endPath",tuple);
    if ( PyErr_Occurred())
return( NULL );

Py_RETURN( self );
}

static PyMethodDef PyFFContour_methods[] = {
    {"dup", (PyCFunction)PyFFContour_dup, METH_NOARGS,
	     "Returns a deep copy of the contour." },
    {"isEmpty", (PyCFunction)PyFFContour_IsEmpty, METH_NOARGS,
	     "Returns whether a contour contains no points" },
    {"moveTo", (PyCFunction)PyFFContour_Start, METH_VARARGS,
	     "Place an initial point on an empty contour" },
    {"lineTo", (PyCFunction)PyFFContour_LineTo, METH_VARARGS,
	     "Append a line to a contour (optionally specifying position)" },
    {"cubicTo", (PyCFunction)PyFFContour_CubicTo, METH_VARARGS,
	     "Append a cubic curve to a (cubic) contour (optionally specifying position)" },
    {"quadraticTo", (PyCFunction)PyFFContour_QuadraticTo, METH_VARARGS,
	     "Append a quadratic curve to a (quadratic) contour (optionally specifying position)" },
    {"insertPoint", (PyCFunction)PyFFContour_InsertPoint, METH_VARARGS,
	     "Add a curve point to a contour (optionally specifying position)" },
    {"makeFirst", (PyCFunction)PyFFContour_MakeFirst, METH_VARARGS,
	     "Rotate a contour so that the specified point is first" },
    {"reverseDirection", (PyCFunction)PyFFContour_ReverseDirection, METH_NOARGS,
	     "Reverse a closed contour so that the second point on it is the penultimate and vice versa." },
    {"isClockwise", (PyCFunction)PyFFContour_IsClockwise, METH_NOARGS,
	     "Determine if a contour is oriented in a clockwise direction. If the contour intersects itself the results are indeterminate." },
    {"xBoundsAtY", (PyCFunction)PyFFContour_xBoundsAtY, METH_VARARGS,
	     "The minimum and maximum values of x attained for a given y, or returns None" },
    {"yBoundsAtX", (PyCFunction)PyFFContour_yBoundsAtX, METH_VARARGS,
	     "The minimum and maximum values of y attained for a given x, or returns None" },
    {"merge", (PyCFunction)PyFFContour_Merge, METH_VARARGS,
	     "Removes the specified on-curve point leaving the contour otherwise intact" },
    {"selfIntersects", (PyCFunction)PyFFContour_selfIntersects, METH_NOARGS,
	     "Returns whether this contour intersects itself" },
    {"similar", (PyCFunction)PyFFContour_similar, METH_VARARGS,
	     "Returns whether two contours are similar within certain error bounds." },
    {"simplify", (PyCFunction)PyFFContour_Simplify, METH_VARARGS,
	     "Smooths a contour" },
    {"transform", (PyCFunction)PyFFContour_Transform, METH_VARARGS,
	     "Transform a contour by a 6 element matrix." },
    {"addExtrema", (PyCFunction)PyFFContour_AddExtrema, METH_VARARGS,
	     "Add Extrema to a contour" },
	{"addInflections", (PyCFunction)PyFFContour_AddInflections, METH_NOARGS,
	     "Add points of inflection to a contour" },
	{"balance", (PyCFunction)PyFFContour_Balance, METH_NOARGS,
	     "Balance handles on a contour" },
	{"harmonize", (PyCFunction)PyFFContour_Harmonize, METH_NOARGS,
	     "Harmonize curvatures on a contour" },
    {"round", (PyCFunction)PyFFContour_Round, METH_VARARGS,
	     "Round points on a contour" },
    {"cluster", (PyCFunction)PyFFContour_Cluster, METH_VARARGS,
	     "Round points on a contour" },
    {"boundingBox", (PyCFunction)PyFFContour_BoundingBox, METH_NOARGS,
	     "Finds a bounding box for the contour (xmin,ymin,xmax,ymax)" },
    {"getSplineAfterPoint", (PyCFunction)PyFFContour_GetSplineAfterPoint, METH_VARARGS,
	     "Returns the coordinates of two cubic splines, one for x movement, one for y.\n"
	     "(Quadratic curves will have 0s for the first coordinates).\n"
	     "The spline will be either the on after the specified point for on-curve points.\n"
	     "or the one through the specified point for control points." },
    {"draw", (PyCFunction)PyFFContour_draw, METH_VARARGS,
	     "Support for the \"pen\" protocol (I hope)\nhttp://just.letterror.com/ltrwiki/PenProtocol" },
    {"__reduce__", (PyCFunction)PyFFContour_pickleReducer, METH_NOARGS,
	     "cPickle calls this routine when it wants to pickle us" },
    PYMETHODDEF_EMPTY  /* Sentinel */
};

PyTypeObject PyFF_ContourType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "fontforge.contour",       /*tp_name*/
    sizeof(PyFF_Contour),      /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)PyFFContour_dealloc, /*tp_dealloc*/
    0,                         /*tp_vectorcall_offset*/
    NULL,                      /*tp_getattr*/
    NULL,                      /*tp_setattr*/
    NULL,                      /*tp_reserved/tp_compare*/
    NULL,                      /*tp_repr*/
    NULL,                      /*tp_as_number*/
    &PyFFContour_Sequence,     /*tp_as_sequence*/
    &PyFFContour_Mapping,      /*tp_as_mapping*/
    NULL,                      /*tp_hash */
    NULL,                      /*tp_call*/
    (reprfunc)PyFFContour_Str, /*tp_str*/
    NULL,                      /*tp_getattro*/
    NULL,                      /*tp_setattro*/
    NULL,                      /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,/*tp_flags*/
    "fontforge Contour objects", /* tp_doc */
    NULL /*(traverseproc)FFContour_traverse*/,  /* tp_traverse */
    (inquiry)PyFFContour_clear,  /* tp_clear */
    (richcmpfunc)PyFFContour_richcompare, /*tp_richcompare*/
    0,                         /* tp_weaklistoffset */
    contouriter_new,           /* tp_iter */
    NULL,                      /* tp_iternext */
    PyFFContour_methods,       /* tp_methods */
    NULL,                      /* tp_members */
    PyFFContour_getset,        /* tp_getset */
    NULL,                      /* tp_base */
    NULL,                      /* tp_dict */
    NULL,                      /* tp_descr_get */
    NULL,                      /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)PyFFContour_init,/* tp_init */
    NULL,                      /* tp_alloc */
    PyFFContour_new,           /* tp_new */
    NULL,                      /* tp_free */
    NULL,                      /* tp_is_gc */
    NULL,                      /* tp_bases */
    NULL,                      /* tp_mro */
    NULL,                      /* tp_cache */
    NULL,                      /* tp_subclasses */
    NULL,                      /* tp_weaklist */
    NULL,                      /* tp_del */
    0,                         /* tp_version_tag */
};

/* ************************************************************************** */
/* Layer iterator type */
/* ************************************************************************** */

typedef struct {
    PyObject_HEAD
    int pos;
    PyFF_Layer *layer;
} layeriterobject;
static PyTypeObject PyFF_LayerIterType;

static PyObject *layeriter_new(PyObject *layer) {
    layeriterobject *li;
    li = PyObject_New(layeriterobject, &PyFF_LayerIterType);
    if (li == NULL)
return NULL;
    li->layer = ((PyFF_Layer *) layer);
    Py_INCREF(layer);
    li->pos = 0;
return (PyObject *)li;
}

static void layeriter_dealloc(layeriterobject *li) {
    Py_XDECREF(li->layer);
    PyObject_Del(li);
}

static PyObject *layeriter_iternext(layeriterobject *li) {
    PyFF_Layer *layer = li->layer;
    PyObject *c;

    if ( layer == NULL)
return NULL;

    if ( li->pos<layer->cntr_cnt ) {
	c = (PyObject *) layer->contours[li->pos++];
	Py_INCREF(c);
return( c );
    }

return NULL;
}

static PyTypeObject PyFF_LayerIterType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "fontforge.layer_iterator", /* tp_name */
    sizeof(layeriterobject),   /* tp_basicsize */
    0,                         /* tp_itemsize */
    /* methods */
    (destructor)layeriter_dealloc, /* tp_dealloc */
    0,                         /* tp_vectorcall_offset */
    NULL,                      /* tp_getattr */
    NULL,                      /* tp_setattr */
    NULL,                      /* tp_compare */
    NULL,                      /* tp_repr */
    NULL,                      /* tp_as_number */
    NULL,                      /* tp_as_sequence */
    NULL,                      /* tp_as_mapping */
    NULL,                      /* tp_hash */
    NULL,                      /* tp_call */
    NULL,                      /* tp_str */
    NULL,                      /* tp_getattro */
    NULL,                      /* tp_setattro */
    NULL,                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    NULL,                      /* tp_doc */
    NULL,                      /* tp_traverse */
    NULL,                      /* tp_clear */
    NULL,                      /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    PyObject_SelfIter,         /* tp_iter */
    (iternextfunc)layeriter_iternext, /* tp_iternext */
    NULL,                      /* tp_methods */
    NULL,                      /* tp_members */
    PyFFContour_getset,        /* tp_getset */
    NULL,                      /* tp_base */
    NULL,                      /* tp_dict */
    NULL,                      /* tp_descr_get */
    NULL,                      /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)PyFFContour_init,/* tp_init */
    NULL,                      /* tp_alloc */
    PyFFContour_new,           /* tp_new */
    NULL,                      /* tp_free */
    NULL,                      /* tp_is_gc */
    NULL,                      /* tp_bases */
    NULL,                      /* tp_mro */
    NULL,                      /* tp_cache */
    NULL,                      /* tp_subclasses */
    NULL,                      /* tp_weaklist */
    NULL,                      /* tp_del */
    0,                         /* tp_version_tag */
};

/* ************************************************************************** */
/* Layers */
/* ************************************************************************** */

static int PyFFLayer_clear(PyFF_Layer *self) {
    int i;

    for ( i=0; i<self->cntr_cnt; ++i )
	Py_DECREF(self->contours[i]);
    self->cntr_cnt = 0;

return 0;
}

static void PyFFLayer_dealloc(PyFF_Layer *self) {
    PyFFLayer_clear(self);
    PyMem_Del(self->contours);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *PyFFLayer_new(PyTypeObject *type, PyObject *UNUSED(args), PyObject *UNUSED(kwds)) {
    PyFF_Layer *self;

    self = (PyFF_Layer *)type->tp_alloc(type, 0);
    if ( self!=NULL ) {
	self->contours = NULL;
	self->cntr_cnt = self->cntr_max = 0;
	self->is_quadratic = 0;
    }

return (PyObject *)self;
}

static int PyFFLayer_init(PyFF_Layer *self, PyObject *args, PyObject *UNUSED(kwds)) {
    int quad=0;

    if ( args!=NULL && !PyArg_ParseTuple(args, "|i", &quad))
return -1;

    self->is_quadratic = (quad!=0);
return 0;
}
static PyObject *PyFFLayer_Str(PyFF_Layer *self) {
    char *buffer, *pt;
    int cnt, i,j;
    PyFF_Contour *contour;
    PyObject *ret;

    cnt = 0;
    for ( i=0; i<self->cntr_cnt; ++i )
	cnt += self->contours[i]->pt_cnt;
    buffer=pt=malloc(cnt*30+self->cntr_cnt*30+30);
    strcpy(buffer, self->is_quadratic? "<Layer(quadratic)\n":"<Layer(cubic)\n");
    pt = buffer+strlen(buffer);
    for ( i=0; i<self->cntr_cnt; ++i ) {
	contour = self->contours[i];
	strcpy(pt, " <Contour\n" );
	pt += strlen(pt);
	for ( j=0; j<contour->pt_cnt; ++j ) {
	    sprintf( pt, "  (%g,%g) %s\n", (double)contour->points[j]->x, (double)contour->points[j]->y,
		    contour->points[j]->on_curve ? "on" : "off" );
	    pt += strlen( pt );
	}
	strcpy(pt," >\n");
	pt += strlen(pt);
    }
    strcpy(pt,">");
    ret = PyUnicode_FromString( buffer );
    free( buffer );
return( ret );
}

static int PyFFLayer_docompare(PyFF_Layer *self,PyObject *other,
	double pt_err, double spline_err) {
    SplineSet *ss, *ss2;
    int ret;
    SplinePoint *badpoint;

    ss = SSFromLayer(self);
    if ( PyType_IsSubtype(&PyFF_ContourType, Py_TYPE(other)) ) {
	ss2 = SSFromContour((PyFF_Contour *) other,NULL);
    } else if ( PyType_IsSubtype(&PyFF_LayerType, Py_TYPE(other)) ) {
	ss2 = SSFromLayer((PyFF_Layer *) other);
    } else {
	PyErr_Format(PyExc_TypeError, "Unexpected type");
return( -1 );
    }
    ret = SSsCompare(ss,ss2,pt_err,spline_err,&badpoint);
    SplinePointListsFree(ss);
    SplinePointListsFree(ss2);
return(ret);
}

static int PyFFLayer_compare(PyFF_Layer *self,PyObject *other) {
    const double pt_err = .5, spline_err = 1;
    int i,j,ret;

    ret = PyFFLayer_docompare(self,other,pt_err,spline_err);
    if ( !(ret&SS_NoMatch) )
return( 0 );

    /* There's no real ordering on these guys. Make up something that is */
    /*  at least consistent */
    if ( PyType_IsSubtype(&PyFF_ContourType, Py_TYPE(other)) )
return( -1 );

    /* Ok, both are layers */
    if ( self->cntr_cnt < ((PyFF_Layer *) other)->cntr_cnt )
return( -1 );
    else if ( self->cntr_cnt > ((PyFF_Layer *) other)->cntr_cnt )
return( 1 );
    /* If there's a difference, then there must be at least one point to be */
    /*  different. And we only get here if there were a difference */
    for ( j=0; j<self->cntr_cnt; ++j ) {
	PyFF_Contour *scon = self->contours[j], *ocon = ((PyFF_Layer *) other)->contours[j];
	if ( scon->pt_cnt<ocon->pt_cnt )
return( -1 );
	else if ( scon->pt_cnt>ocon->pt_cnt )
return( 1 );
	for ( i=0; i<scon->pt_cnt; ++i ) {
	    ret = PyFFPoint_compare(scon->points[i],(PyObject *) ocon->points[i]);
	    if ( ret!=0 )
return( ret );
	}
    }

return( -1 );		/* Arbitrary... but we can't get here=>all points same */
}

static PyObject *PyFFLayer_richcompare(PyObject *a, PyObject *b, int op) {
    return enrichened_compare((cmpfunc) PyFFLayer_compare, a, b, op);
}

/* ************************************************************************** */
/* Layer getters/setters */
/* ************************************************************************** */
static PyObject *PyFF_Layer_get_is_quadratic(PyFF_Layer *self, void *UNUSED(closure)) {
return( Py_BuildValue("i", self->is_quadratic ));
}

static int PyFF_Layer_set_is_quadratic(PyFF_Layer *self,PyObject *value, void *UNUSED(closure)) {
    int val;
    SplineSet *ss, *ss2;

    val = PyLong_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );

    val = (val!=0);
    if ( val == self->is_quadratic )
return( 0 );
    ss = SSFromLayer(self);
    PyFFLayer_clear(self);
    if ( val )
	ss2 = SplineSetsTTFApprox(ss);
    else
	ss2 = SplineSetsPSApprox(ss);
    SplinePointListFree(ss);
    self->is_quadratic = (val!=0);
    LayerFromSS(ss2,self);
    SplinePointListFree(ss2);
return( 0 );
}

static PyGetSetDef PyFFLayer_getset[] = {
    {(char *)"is_quadratic",
     (getter)PyFF_Layer_get_is_quadratic, (setter)PyFF_Layer_set_is_quadratic,
     (char *)"Whether this is an quadratic or cubic layer", NULL},
    PYGETSETDEF_EMPTY /* Sentinel */
};

/* ************************************************************************** */
/* Layer sequence */
/* ************************************************************************** */
static Py_ssize_t PyFFLayer_Length( PyObject *self ) {
return( ((PyFF_Layer *) self)->cntr_cnt );
}

static PyObject *PyFFLayer_Concat( PyObject *_c1, PyObject *_c2 ) {
    PyFF_Layer *c1 = (PyFF_Layer *) _c1, *c2 = (PyFF_Layer *) _c2;
    PyFF_Layer *self;
    int i;
    PyFF_Layer dummy;
    PyFF_Contour *dummies[1];

    if ( PyType_IsSubtype(&PyFF_ContourType, Py_TYPE(c2)) &&
	    c1->is_quadratic == c2->is_quadratic ) {
	memset(&dummy,0,sizeof(dummy));
	dummy.cntr_cnt = 1;
	dummy.contours = dummies; dummies[0] = (PyFF_Contour *) _c2;
	c2 = &dummy;
    } else if ( !PyType_IsSubtype(&PyFF_LayerType, Py_TYPE(c1)) ||
                !PyType_IsSubtype(&PyFF_LayerType, Py_TYPE(c2)) ||
	    c1->is_quadratic != c2->is_quadratic ) {
	PyErr_Format(PyExc_TypeError, "Both arguments must be Layers of the same order");
return( NULL );
    }
    self = (PyFF_Layer *)PyFF_LayerType.tp_alloc(&PyFF_LayerType, 0);
    self->is_quadratic = c1->is_quadratic;
    self->cntr_max = self->cntr_cnt = c1->cntr_cnt + c2->cntr_cnt;
    self->contours = PyMem_New(PyFF_Contour *,self->cntr_max);
    for ( i=0; i<c1->cntr_cnt; ++i ) {
	Py_INCREF(c1->contours[i]);
	self->contours[i] = c1->contours[i];
    }
    for ( i=0; i<c2->cntr_cnt; ++i ) {
	Py_INCREF(c2->contours[i]);
	self->contours[c1->cntr_cnt+i] = c2->contours[i];
    }
Py_RETURN( self );
}

static PyObject *PyFFLayer_InPlaceConcat( PyObject *_self, PyObject *_c2 ) {
    PyFF_Layer *self = (PyFF_Layer *) _self, *c2 = (PyFF_Layer *) _c2;
    int i, old_cnt;
    PyFF_Layer dummy;
    PyFF_Contour *dummies[1];

    if ( PyType_IsSubtype(&PyFF_ContourType, Py_TYPE(c2)) &&
	    self->is_quadratic == ((PyFF_Contour *) c2)->is_quadratic ) {
	memset(&dummy,0,sizeof(dummy));
	dummy.cntr_cnt = 1;
	dummy.contours = dummies; dummies[0] = (PyFF_Contour *) _c2;
	c2 = &dummy;
    } else if ( !PyType_IsSubtype(&PyFF_LayerType, Py_TYPE(self)) ||
                !PyType_IsSubtype(&PyFF_LayerType, Py_TYPE(c2)) ||
	    self->is_quadratic != c2->is_quadratic ) {
	PyErr_Format(PyExc_TypeError, "Both arguments must be Layers of the same order");
return( NULL );
    }
    old_cnt = self->cntr_cnt;
    self->cntr_cnt += c2->cntr_cnt;
    if ( self->cntr_cnt >= self->cntr_max ) {
	self->cntr_max = self->cntr_cnt;
	PyMem_Resize(self->contours,PyFF_Contour *,self->cntr_max);  /* Messes with self->contours */
    }
    for ( i=0; i<c2->cntr_cnt; ++i ) {
	Py_INCREF(c2->contours[i]);
	self->contours[old_cnt+i] = c2->contours[i];
    }
Py_RETURN( self );
}

static PyObject *PyFFLayer_Index( PyObject *self, Py_ssize_t pos ) {
    PyFF_Layer *layer = (PyFF_Layer *) self;
    PyObject *ret;

    if ( pos<0 || pos>=layer->cntr_cnt ) {
	PyErr_Format(PyExc_TypeError, "Index out of bounds");
return( NULL );
    }
    ret = (PyObject *) layer->contours[pos];
    Py_INCREF(ret);
return( ret );
}

static int PyFFLayer_IndexAssign( PyObject *self, Py_ssize_t pos, PyObject *val ) {
    PyFF_Layer *layer = (PyFF_Layer *) self;
    PyFF_Contour *old, *contour;
    int i;

    if ( val!=NULL && !PyType_IsSubtype(&PyFF_ContourType, Py_TYPE(val)) ) {
	PyErr_Format(PyExc_TypeError, "Value must be a (FontForge) Contour");
	return( -1 );
    }
    if ( pos<0 || pos>=layer->cntr_cnt ) {
	PyErr_Format(PyExc_TypeError, "Index out of bounds");
	return( -1 );
    }

    old = layer->contours[pos];

    if ( val==NULL) {
	for ( i=pos; i<layer->cntr_cnt-1; ++i )
	    layer->contours[i] = layer->contours[i+1];
	layer->cntr_cnt -= 1;
    } else {
	contour = (PyFF_Contour *) val;
	if ( contour->is_quadratic!=layer->is_quadratic ) {
	    PyErr_Format(PyExc_TypeError, "Replacement contour must have the same order as the layer");
	    return( -1 );
	}
	layer->contours[pos] = contour;
	Py_INCREF( contour );
    }

    Py_DECREF( old );
    return( 0 );
}

static PySequenceMethods PyFFLayer_Sequence = {
    PyFFLayer_Length,		/* length */
    PyFFLayer_Concat,		/* concat */
    NULL,			/* repeat */
    PyFFLayer_Index,		/* subscript */
    NULL,			/* slice */
    PyFFLayer_IndexAssign,	/* subscript assign */
    NULL,			/* slice assign */
    NULL,			/* contains */
    PyFFLayer_InPlaceConcat,	/* inplace_concat */
    NULL			/* inplace repeat */
};

/* ************************************************************************** */
/* Layer methods */
/* ************************************************************************** */
static PyObject *PyFFLayer_dup(PyFF_Layer *self) {
    /* Arg checking done elsewhere */
    int i;
    PyFF_Layer *ret = (PyFF_Layer *)PyFF_LayerType.tp_alloc(&PyFF_LayerType, 0);

    ret->cntr_cnt = ret->cntr_max = self->cntr_cnt;
    ret->is_quadratic = self->is_quadratic;
    if ( ret->cntr_cnt!=0 ) {
	ret->contours = PyMem_New(PyFF_Contour *,ret->cntr_cnt);
	for ( i=0; i<ret->cntr_cnt; ++i )
	    ret->contours[i] = (PyFF_Contour *) PyFFContour_dup(self->contours[i]);
    }
return( (PyObject *) ret );
}

static PyObject *PyFFLayer_IsEmpty(PyFF_Layer *self) {
    /* Arg checking done elsewhere */
return( Py_BuildValue("i",self->cntr_cnt==0 ) );
}

static PyObject *PyFFLayer_selfIntersects(PyFF_Layer *self, PyObject *UNUSED(args)) {
    SplineSet *ss;
    Spline *s, *s2;
    PyObject *ret;

    ss = SSFromLayer(self);
    ret = SplineSetIntersect(ss,&s,&s2) ? Py_True : Py_False;
    SplinePointListFree(ss);
    Py_INCREF( ret );
return( ret );
}

static PyObject *PyFFLayer_Simplify(PyFF_Layer *self, PyObject *args) {
    SplineSet *ss;
    static struct simplifyinfo smpl = { sf_normal, 0.75, 0.2, 10, 0, 0, 0 };

    smpl.err = 1;
    smpl.linefixup = 2;
    smpl.linelenmax = 10;

    ss = SSFromLayer(self);
    if ( ss==NULL ) {
	if ( PyErr_Occurred() != NULL )
	    return ( NULL );
	else
	    Py_RETURN( self );	/* As simple as it can be */
    }

    if ( PySequence_Size(args)>=1 )
	smpl.err = PyFloat_AsDouble(PySequence_GetItem(args,0));
    if ( !PyErr_Occurred() && PySequence_Size(args)>=2 )
	smpl.flags = FlagsFromTuple( PySequence_GetItem(args,1),simplifyflags,"simplify flag");
    if ( !PyErr_Occurred() && PySequence_Size(args)>=3 )
	smpl.tan_bounds = PyFloat_AsDouble( PySequence_GetItem(args,2));
    if ( !PyErr_Occurred() && PySequence_Size(args)>=4 )
	smpl.linefixup = PyFloat_AsDouble( PySequence_GetItem(args,3));
    if ( !PyErr_Occurred() && PySequence_Size(args)>=5 )
	smpl.linelenmax = PyFloat_AsDouble( PySequence_GetItem(args,4));
    if ( PyErr_Occurred() )
return( NULL );
    ss = SplineCharSimplify(NULL,ss,&smpl);
    LayerFromSS(ss,self);
    SplinePointListsFree(ss);
Py_RETURN( self );
}

static PyObject *PyFFLayer_similar(PyFF_Layer *self, PyObject *args) {
    double pt_err = -1, spline_err = -1;
    int ret;
    PyObject *other, *retO;

    if ( !PyArg_ParseTuple(args,"O|dd", &other, &pt_err, &spline_err) )
return( NULL );
    if ( pt_err==-1 ) {
	pt_err = .5;
	spline_err = pt_err;
    } else if ( spline_err==-1 )
	spline_err = pt_err;

    if ( !PyType_IsSubtype(&PyFF_LayerType, Py_TYPE(other)) &&
	 !PyType_IsSubtype(&PyFF_ContourType, Py_TYPE(other)) ) {
	PyErr_Format(PyExc_TypeError, "Unexpected type");
return( NULL );
    }

    ret = PyFFLayer_docompare(self,other,pt_err,spline_err);
    retO = (ret&SS_NoMatch) ? Py_False : Py_True;
    Py_INCREF( retO );
return( retO );
}

static PyObject *PyFFLayer_Transform(PyFF_Layer *self, PyObject *args) {
    int i, j;
    double m[6];
    PyFF_Contour *cntr;

    if ( !PyArg_ParseTuple(args,"(dddddd)",&m[0], &m[1], &m[2], &m[3], &m[4], &m[5]) )
return( NULL );
    for ( i=0; i<self->cntr_cnt; ++i ) {
	cntr = self->contours[i];
	for ( j=0; j<cntr->pt_cnt; ++j )
	    PyFF_TransformPoint(cntr->points[j],m);
    }
Py_RETURN( self );
}

static PyObject *PyFFLayer_NLTransform(PyFF_Layer *self, PyObject *args) {
    char *xexpr, *yexpr;
    SplineSet *ss;

    if ( !PyArg_ParseTuple(args,"ss", &xexpr, &yexpr ) )
return( NULL );

    ss = SSFromLayer(self);
    if ( ss==NULL ) {
	if ( PyErr_Occurred() != NULL )
	    return ( NULL );
	else
	    Py_RETURN( self );
    }

    if ( !SSNLTrans(ss,xexpr,yexpr) ) {
	PyErr_Format(PyExc_TypeError, "Unparseable expression.");
	SplinePointListsFree(ss);
return( NULL );
    }

    LayerFromSS(ss,self);
    SplinePointListsFree(ss);
Py_RETURN( self );
}

static PyObject *PyFFLayer_pickleReducer(PyFF_Layer *self, PyObject *UNUSED(args)) {
    PyObject *reductionTuple, *argTuple;
    int i;

    if ( _new_layer==NULL )
	PyFF_PickleTypesInit();
    reductionTuple = PyTuple_New(2);
    Py_INCREF(_new_layer);
    PyTuple_SetItem(reductionTuple,0,_new_layer);
    argTuple = PyTuple_New(1+self->cntr_cnt);
    PyTuple_SetItem(reductionTuple,1,argTuple);
    PyTuple_SetItem(argTuple,0,Py_BuildValue("i", self->is_quadratic));
    for ( i=0; i<self->cntr_cnt; ++i ) {
	Py_INCREF((PyObject *)self->contours[i]);
	PyTuple_SetItem(argTuple,1+i,(PyObject *)self->contours[i]);
    }
return( reductionTuple );
}

static PyObject *PyFFLayer_Round(PyFF_Layer *self, PyObject *args) {
    double factor=1;
    int i,j;
    PyFF_Contour *cntr;

    if ( !PyArg_ParseTuple(args,"|d",&factor ) )
return( NULL );
    for ( i=0; i<self->cntr_cnt; ++i ) {
	cntr = self->contours[i];
	for ( j=0; j<cntr->pt_cnt; ++j ) {
	    cntr->points[j]->x = rint( factor*(double)cntr->points[j]->x )/factor;
	    cntr->points[j]->y = rint( factor*(double)cntr->points[j]->y )/factor;
	}
    }
Py_RETURN( self );
}

static PyObject *PyFFLayer_Cluster(PyFF_Layer *self, PyObject *args) {
    double within = .1, max = .5;
    SplineChar sc;
    Layer layers[2];
    SplineSet *ss;

    if ( !PyArg_ParseTuple(args,"|dd", &within, &max ) )
return( NULL );

    ss = SSFromLayer(self);
    if ( ss==NULL ) {
	if ( PyErr_Occurred() != NULL )
	    return ( NULL );
	else
	    Py_RETURN( self ); /* no contours=> no clusters */
    }

    memset(&sc,0,sizeof(sc));
    memset(layers,0,sizeof(layers));
    sc.layers = layers;
    sc.layers[ly_fore].splines = ss;
    sc.name = copy("nameless");
    SCRoundToCluster( &sc,ly_fore,false,within,max);
    LayerFromSS(sc.layers[ly_fore].splines,self);
    SplinePointListsFree(sc.layers[ly_fore].splines);
Py_RETURN( self );
}

static PyObject *PyFFLayer_AddExtrema(PyFF_Layer *self, PyObject *args) {
    int emsize = 1000;
    char *flag = NULL;
    int ae = ae_only_good;
    SplineSet *ss;

    if ( !PyArg_ParseTuple(args,"|si", &flag, &emsize ) )
return( NULL );
    if ( flag!=NULL ) {
	ae = FlagsFromString(flag,addextremaflags,"extrema flag");
        if ( ae==FLAG_UNKNOWN )
return( NULL );
    }

    ss = SSFromLayer(self);
    if ( ss==NULL ) {
	if ( PyErr_Occurred() != NULL )
	    return ( NULL );
	else
	    Py_RETURN( self ); // no contours=> nothing to do
    }

    SplineCharAddExtrema(NULL,ss,ae,emsize);
    LayerFromSS(ss,self);
    SplinePointListsFree(ss);
Py_RETURN( self );
}

static PyObject *_PyFFLayer_Action(PyFF_Layer *self, void (*func)(SplineChar*, SplineSet*, int)) { 
    SplineSet *ss;
    ss = SSFromLayer(self);
    if ( ss==NULL ) {
	if ( PyErr_Occurred() != NULL )
	    return ( NULL );
	else
	    Py_RETURN( self ); // no contours=> nothing to do
    }
    func(NULL,ss,false);
    LayerFromSS(ss,self);
    SplinePointListsFree(ss);
Py_RETURN( self );
}	

static PyObject *PyFFLayer_AddInflections(PyFF_Layer *self) {
    return _PyFFLayer_Action(self,&SplineCharAddInflections);
}

static PyObject *PyFFLayer_Balance(PyFF_Layer *self) {
    return _PyFFLayer_Action(self,&SplineCharBalance);
}

static PyObject *PyFFLayer_Harmonize(PyFF_Layer *self) {
    return _PyFFLayer_Action(self,&SplineCharHarmonize);
}

int NibCheck(SplineSet *nib) {
    enum ShapeType pt;
    SplineSet *ss;

    for ( ss=nib ; ss!=NULL; ss=ss->next ) {
	pt = NibIsValid(ss);
	if ( pt!=Shape_Convex ) {
	    PyErr_Format(PyExc_ValueError, NibShapeTypeMsg(pt));
	    return false;
	}
    }
    return( true );
}

/* Linecap type: see 'enum linecap' in splinefont.h */
struct flaglist linecap[] = {
    { "nib", lc_nib },
    { "butt", lc_butt },
    { "round", lc_round },
    { "square", lc_square },
    FLAGLIST_EMPTY /* Sentinel */
};

/* Linejoin type: see 'enum linejoin' in splinefont.h */
struct flaglist linejoin[] = {
    { "nib", lj_nib },
    { "miter", lj_miter },
    { "miterclip", lj_miterclip },
    { "round", lj_round },
    { "bevel", lj_bevel },
    { "arcs", lj_arcs },
    FLAGLIST_EMPTY /* Sentinel */
};

struct flaglist rmov[] = {
    { "layer", srmov_layer },
    { "contour", srmov_contour },
    { "none", srmov_none },
    FLAGLIST_EMPTY /* Sentinel */
};

struct flaglist sal[] = {
    { "svg2", sal_svg2 },
    { "ratio", sal_ratio },
    { "auto", sal_auto },
    FLAGLIST_EMPTY /* Sentinel */
};

struct flaglist strokeflags[] = {
    { "removeinternal", 1 },
    { "removeexternal", 2 },
    { "cleanup", 4 },
    FLAGLIST_EMPTY /* Sentinel */
};

#define STROKE_OPTKEYS "removeinternal", "removeexternal", "extrema", "simplify", "accuracy", "joinlimit", "extendcap", "jlrelative", "ecrelative", "removeoverlap", "arcsclip"
#define STROKE_OPTFORMAT "$ppppdddppss"
#define STROKE_OPTARGS &si->removeinternal, &si->removeexternal, &si->extrema, &si->simplify, &si->accuracy_target, &si->joinlimit, &si->extendcap, &si->jlrelative, &si->ecrelative, &rostring, &acstring

static char *strokekey_circ[]
    = { "type", "width", "cap", "join", "angle", STROKE_OPTKEYS, NULL };
static char *strokebkey_circ[]
    = { "type", "width", "cap", "join", "flags", NULL };
static char *strokekey_ellip[]
    = { "type", "width", "minorwidth", "angle", "cap", "join", STROKE_OPTKEYS, NULL };
static char *strokebkey_ellip[]
    = { "type", "width", "minorwidth", "angle", "cap", "join", "flags", NULL };
static char *strokekey_rect[]
    = { "type", "width", "height", "angle", "cap", "join", STROKE_OPTKEYS, NULL };
static char *strokebkey_rect[]
    = { "type", "width", "height", "angle", "flags", NULL };
static char *strokekey_conv[]
    = { "type", "contour", "angle", "cap", "join", STROKE_OPTKEYS, NULL };
static char *strokebkey_conv[]
    = { "type", "contour", "flags", NULL };

static int Stroke_Parse(StrokeInfo *si, PyObject *args, PyObject *keywds) {
    char *type, *cap="nib", *join="nib", *rostring="layer",  *acstring="auto";
    const char *str;
    int c, j, f, r, a, toknum;
    PyObject *flagtuple=NULL;
    PyObject *nib=NULL;

    if ( PyTuple_Size(args)==0 ) {
	PyErr_Format(PyExc_TypeError, "Expected a name of a pen type");
	return( -1 );
    }

    assert( si!=NULL );
    InitializeStrokeInfo(si);
    // Verify expected defaults
    assert( si->rmov==srmov_layer && si->cap==lc_nib && si->join==lj_nib );

    if ((str = PyUnicode_AsUTF8(PyTuple_GetItem(args, 0))) == NULL) {
        return -1;
    }
    if ( strcmp(str, "circular")==0 ) {
	si->stroke_type = si_round;
	if ( !PyArg_ParseTupleAndKeywords(args, keywds,
                "sd|ssd" STROKE_OPTFORMAT, strokekey_circ, &type, &si->width,
		&cap, &join, &si->penangle, STROKE_OPTARGS) ) {
	    PyErr_Clear();
	    if ( !PyArg_ParseTupleAndKeywords(args, keywds, "sd|ssO",
                strokebkey_circ, &type, &si->width,
		&cap, &join, &flagtuple) ) {
		PyErr_Format(PyExc_TypeError, "Wrong parameter set for "
		                              "'circular' nib type" );
		return( -1 );
	    }
	}
    } else if (    strcmp(str, "eliptical")==0
                || strcmp(str, "elliptical")==0 ) {
	si->stroke_type = si_round;
	if ( !PyArg_ParseTupleAndKeywords(args, keywds,
                "sdd|dss" STROKE_OPTFORMAT, strokekey_ellip, &type,
	        &si->width, &si->height, &si->penangle, &cap, &join,
	        STROKE_OPTARGS) ) {
	    PyErr_Clear();
	    if ( !PyArg_ParseTupleAndKeywords(args, keywds, "sddd|ssO",
                strokebkey_ellip, &type, &si->width, &si->height,
		&si->penangle, &cap, &join, &flagtuple) ) {
		PyErr_Format(PyExc_TypeError, "Wrong parameter set for "
		                              "'elliptical' nib type" );
		return( -1 );
	    }
	}
    } else if (    strcmp(str, "calligraphic")==0
                || strcmp(str, "rectangular")==0
                || strcmp(str, "caligraphic")==0
                || strcmp(str, "square")==0 ) {
	si->stroke_type = si_calligraphic;
	if ( !PyArg_ParseTupleAndKeywords(args, keywds,
                "sdd|dss" STROKE_OPTFORMAT, strokekey_rect, &type,
	        &si->width, &si->height, &si->penangle, &cap, &join,
	        STROKE_OPTARGS) ) {
	    PyErr_Clear();
	    if ( !PyArg_ParseTupleAndKeywords(args, keywds, "sddd|O",
                strokebkey_rect, &type, &si->width, &si->height,
		&si->penangle, &flagtuple) ) {
		PyErr_Format(PyExc_TypeError, "Wrong parameter set for "
		                              "'calligraphic' nib type" );
		return( -1 );
	    }
	}
    } else if (    strcmp(str, "convex")==0
                || strcmp(str, "poly")==0
                || strcmp(str, "polygonal")==0 ) {
	si->stroke_type = si_nib;
	if ( !PyArg_ParseTupleAndKeywords(args, keywds,
                "sO|dss" STROKE_OPTFORMAT, strokekey_conv, &type,
	        &nib, &si->penangle, &cap, &join, STROKE_OPTARGS) ) {
	    PyErr_Clear();
	    if ( !PyArg_ParseTupleAndKeywords(args, keywds, "sO|O",
                strokebkey_conv, &type, &nib, &flagtuple) ) {
		PyErr_Format(PyExc_TypeError, "Wrong parameter set for "
		                              "'convex' nib type" );
		return( -1 );
	    }
	}
    } else {
	PyErr_Format(PyExc_TypeError, "Unrecognized stroke type");
	return -1;
    }

    if ( si->stroke_type==si_nib ) {
	SplineSet *ss=NULL;
	int start=0, order2=0;

	if ( PyType_IsSubtype(&PyFF_ContourType, Py_TYPE(nib)) ) {
	    order2 = ((PyFF_Contour *) nib)->is_quadratic;
	    ss = SSFromContour((PyFF_Contour *) nib,&start);
            if ( ss==NULL ) {
		if ( PyErr_Occurred() != NULL )
                    PyErr_SetString(PyExc_AttributeError, "Empty Contour");
                return( -1 );
            }
	} else if ( PyType_IsSubtype(&PyFF_LayerType, Py_TYPE(nib)) ) {
	    order2 = ((PyFF_Layer *) nib)->is_quadratic;
	    ss = SSFromLayer((PyFF_Layer *) nib);
            if ( ss==NULL ) {
		if ( PyErr_Occurred() != NULL )
                    PyErr_SetString(PyExc_AttributeError, "Empty Layer");
                return( -1 );
            }
	} else if ( PyUnicode_Check(nib) ) {
	    if ((str = PyUnicode_AsUTF8(nib)) == NULL) {
		return -1;
	    }
	    toknum = PyFF_ConvexNibID(str);
	    if ( toknum==-1 )
		return -1;
	    ss = StrokeGetConvex(toknum, true);
	    if ( ss==NULL ) {
		PyErr_Format(PyExc_TypeError, "Nib context empty (never initialized?)");
		return -1;
	    }
	} else {
	    PyErr_Format(PyExc_TypeError, "Second argument must be a (fontforge) Contour or Layer or a nib context identifier string");
	    return( -1 );
	}
	if ( order2 ) {
	    SplineSet *temp = SSPSApprox(ss);
	    SplinePointListsFree(ss);
	    ss = temp;
	}
	if ( !NibCheck(ss) )
	    return( -1 );
	si->nib = ss;
    } else if ( si->width<=0 || si->height<0 ) {
        PyErr_Format(PyExc_ValueError, "Stroke dimensions must be positive" );
	return( -1 );
    }

    c = FlagsFromString(cap,linecap,"linecap type");
    if ( c==FLAG_UNKNOWN )
	return( -1 );
    if ( c==lc_square ) {
	c = lc_butt;
	if ( si->extendcap!=0 )
	    si->extendcap=0.5;
    }
    si->cap = c;
    j = FlagsFromString(join,linejoin,"linejoin type");
    if ( j==FLAG_UNKNOWN )
	return( -1 );
    si->join = j;
    r = FlagsFromString(rostring,rmov,"removeoverlap type");
    if ( r==FLAG_UNKNOWN )
	return( -1 );
    si->rmov = r;
    a = FlagsFromString(acstring,sal,"arcsclip type");
    if ( a==FLAG_UNKNOWN )
	return( -1 );
    si->al = a;

    if ( flagtuple!=NULL ) {
	f = FlagsFromTuple(flagtuple,strokeflags,"stroke flag");
	if ( f==FLAG_UNKNOWN )
	    return( -1 );
	si->removeinternal = ( (f&1)!=0 );
	si->removeexternal = ( (f&2)!=0 );
	si->simplify = ( (f&4)!=0 );
    }
    return( 0 );
}

static int LayerArgToLayer(SplineFont *sf, PyObject* layerp);

static char *layer_export_keywords[] = { "filename", "usetransform", "usesystem", "asksystem", NULL };

static PyObject *PyFFLayer_export(PyFF_Layer *self, PyObject *args,
                                  PyObject *keywds) {
    char *filename;
    char *locfilename = NULL;
    char *pt;
    FILE *file;
    SplineChar sc;
    Layer dummylayers[2];
    int use_system = false, ask_system = false;
    ExportParams ep, *epp;

    InitExportParams(&ep);

    if ( !PyArg_ParseTupleAndKeywords(args,keywds,"s|$ppp",
                                      layer_export_keywords,&filename,
                                      &ep.use_transform, &use_system,
                                      &ask_system) )
	return( NULL );
    locfilename = utf82def_copy(filename);

    if ( use_system || ask_system ) {
	epp = ExportParamsState();
	if ( ask_system )
	    ExportParamsDlg(epp);
    } else
	epp = &ep;

    pt = strrchr(locfilename,'.');
    if ( pt==NULL ) pt=locfilename;

    file = fopen( locfilename,"wb");
    if ( file==NULL ) {
	PyErr_SetFromErrnoWithFilename(PyExc_IOError,locfilename);
	free(locfilename);
return( NULL );
    }

    memset(&sc,0,sizeof(sc));
    memset(dummylayers,0,sizeof(dummylayers));
    sc.name = copy("<generic layer>");
    sc.layers = dummylayers;
    sc.layer_cnt = 2;
    dummylayers[ly_fore].splines = SSFromLayer(self);
    dummylayers[ly_fore].order2 = self->is_quadratic;

    if ( strcasecmp(pt,".eps")==0 || strcasecmp(pt,".ps")==0 || strcasecmp(pt,".art")==0 )
	_ExportEPS(file,&sc,ly_fore,true);
    else if ( strcasecmp(pt,".pdf")==0 )
	_ExportPDF(file,&sc,ly_fore);
    else if ( strcasecmp(pt,".svg")==0 )
	_ExportSVG(file,&sc,ly_fore,epp);
    else if ( strcasecmp(pt,".glif")==0 )
	_ExportGlif(file,&sc,ly_fore,3);
    else if ( strcasecmp(pt,".glif2")==0 )
	_ExportGlif(file,&sc,ly_fore,2);
    else if ( strcasecmp(pt,".glif3")==0 )
	_ExportGlif(file,&sc,ly_fore,3);
    else if ( strcasecmp(pt,".plate")==0 )
	_ExportPlate(file,&sc,ly_fore);
    /* else if ( strcasecmp(pt,".fig")==0 )*/
    else {
	PyErr_Format(PyExc_ValueError, "Unknown file name extension \"%s\" to export", pt );
	free( locfilename );
	fclose(file);
	SplinePointListsFree(dummylayers[ly_fore].splines);
return( NULL );
    }
    fclose(file);

    SplinePointListsFree(dummylayers[ly_fore].splines);
    free( locfilename );
Py_RETURN( self );
}

static PyObject *PyFFLayer_Stroke(PyFF_Layer *self, PyObject *args, PyObject *keywds) {
    SplineSet *ss, *newss;
    StrokeInfo si;

    if ( Stroke_Parse(&si, args, keywds)==-1 )
	return( NULL );

    ss = SSFromLayer(self);
    if ( ss==NULL ) {
	if ( PyErr_Occurred() != NULL )
	    return ( NULL );
	else
	    Py_RETURN( self ); // no contours=> nothing to do
    }
    newss = SplineSetStroke(ss,&si,self->is_quadratic);
    SplinePointListFree(ss);
    LayerFromSS(newss,self);
    SplinePointListsFree(newss);
    SplinePointListsFree(si.nib); si.nib = NULL;
    Py_RETURN( self );
}

static PyObject *PyFFLayer_Correct(PyFF_Layer *self, PyObject *UNUSED(args)) {
    SplineSet *ss, *newss;
    int changed = false;

    ss = SSFromLayer(self);
    if ( ss==NULL ) {
	if ( PyErr_Occurred() != NULL )
	    return ( NULL );
	else
	    Py_RETURN( self ); // no contours=> nothing to do
    }
    newss = SplineSetsCorrect(ss,&changed);
    /* same old splinesets */
    LayerFromSS(newss,self);
    SplinePointListsFree(newss);
Py_RETURN( self );
}

static PyObject *PyFFLayer_ReverseDirection(PyFF_Layer *self, PyObject *UNUSED(args)) {
    int i;

    for ( i=0; i<self->cntr_cnt; ++i )
	PyFFContour_ReverseDirection(self->contours[i],NULL);
Py_RETURN( self );
}

static PyObject *PyFFLayer_RemoveOverlap(PyFF_Layer *self, PyObject *UNUSED(args)) {
    SplineSet *ss, *newss;

    ss = SSFromLayer(self);
    if ( ss==NULL ) {
	if ( PyErr_Occurred() != NULL )
	    return ( NULL );
	else
	    Py_RETURN( self ); // no contours=> nothing to do
    }
    newss = SplineSetRemoveOverlap(NULL,ss,over_remove);
    /* Frees the old splinesets */
    LayerFromSS(newss,self);
    SplinePointListsFree(newss);
Py_RETURN( self );
}

static PyObject *PyFFLayer_Intersect(PyFF_Layer *self, PyObject *UNUSED(args)) {
    SplineSet *ss, *newss;

    ss = SSFromLayer(self);
    if ( ss==NULL ) {
	if ( PyErr_Occurred() != NULL )
	    return ( NULL );
	else
	    Py_RETURN( self ); // no contours=> nothing to do
    }
    newss = SplineSetRemoveOverlap(NULL,ss,over_intersect);
    /* Frees the old splinesets */
    LayerFromSS(newss,self);
    SplinePointListsFree(newss);
Py_RETURN( self );
}

static PyObject *PyFFLayer_Exclude(PyFF_Layer *self, PyObject *args) {
    SplineSet *ss, *newss, *excludes, *tail;
    PyObject *obj;

    if ( !PyArg_ParseTuple(args,"O", &obj ) )
return( NULL );
    if ( !PyType_IsSubtype(&PyFF_LayerType, Py_TYPE(obj)) ) {
	PyErr_Format(PyExc_TypeError, "Value must be a (FontForge) Layer");
return( NULL );
    }

    ss = SSFromLayer(self);
    if ( ss==NULL ) {
	if ( PyErr_Occurred() != NULL )
	    return ( NULL );
	else
	    Py_RETURN( self ); // no contours=> nothing to do
    }
    excludes = SSFromLayer((PyFF_Layer *) obj);
    for ( tail=ss; tail->next!=NULL; tail=tail->next );
    tail->next = excludes;
    while ( excludes!=NULL ) {
	excludes->first->selected = true;
	excludes = excludes->next;
    }
    newss = SplineSetRemoveOverlap(NULL,ss,over_exclude);
    /* Frees the old splinesets */
    LayerFromSS(newss,self);
    SplinePointListsFree(newss);
Py_RETURN( self );
}

static PyObject *PyFFLayer_interpolateNewLayer(PyFF_Layer *self, PyObject *args) {
    double amount;
    PyObject *obj;
    SplineSet *ss, *otherss, *newss;
    SplineChar dummy;

    if ( !PyArg_ParseTuple(args,"Od", &obj, &amount ) )
return( NULL );
    if ( !PyType_IsSubtype(&PyFF_LayerType, Py_TYPE(obj)) ) {
	PyErr_Format(PyExc_TypeError, "Value must be a (FontForge) Layer");
return( NULL );
    }

    memset(&dummy,0,sizeof(dummy));		/* Only used for error messages */
    dummy.name = _("<no glyph>");

    ss = SSFromLayer(self);
    otherss = SSFromLayer((PyFF_Layer *) obj);
    newss = SplineSetsInterpolate(ss,otherss,amount,&dummy);
    obj = (PyObject *) LayerFromSS(newss,NULL);
    SplinePointListsFree(ss);
    SplinePointListsFree(otherss);
    SplinePointListsFree(newss);
return( obj );
}

static PyObject *PyFFLayer_stemControl(PyObject *self, PyObject *args) {
    SplineSet *ss;
    double stemwidthscale, stemheightscale= -1, vscale= -1, hscale=1, xheight=-1;

    if ( !PyArg_ParseTuple(args,"d|dddd", &stemwidthscale, &hscale, &stemheightscale, &vscale, &xheight ) )
return( NULL );

    ss = SSFromLayer((PyFF_Layer *) self);
    if ( ss==NULL ) {
	if ( PyErr_Occurred() != NULL )
	    return ( NULL );
	else
	    Py_RETURN( self ); // no contours=> nothing to do
    }
    ss = SSControlStems(ss,stemwidthscale,stemheightscale,hscale,vscale,xheight);
    LayerFromSS(ss,(PyFF_Layer *) self);
    SplinePointListsFree(ss);
Py_RETURN( self );
}

static PyObject *PyFFLayer_BoundingBox(PyFF_Layer *self, PyObject *UNUSED(args)) {
    double xmin, xmax, ymin, ymax;
    int i,j,none;
    PyFF_Contour *cntr;

    none = true;
    for ( j=0; j<self->cntr_cnt; ++j ) {
	cntr = self->contours[j];
	for ( i=0; i<cntr->pt_cnt; ++i ) {
	    if ( none ) {
		xmin = xmax = cntr->points[0]->x;
		ymin = ymax = cntr->points[0]->y;
		none = false;
	    } else {
		if ( cntr->points[i]->x < xmin ) xmin = cntr->points[i]->x;
		if ( cntr->points[i]->x > xmax ) xmax = cntr->points[i]->x;
		if ( cntr->points[i]->y < ymin ) ymin = cntr->points[i]->y;
		if ( cntr->points[i]->y > ymax ) ymax = cntr->points[i]->y;
	    }
	}
    }
    if ( none )
return( Py_BuildValue("(dddd)", 0.0,0.0,0.0,0.0 ));

return( Py_BuildValue("(dddd)", xmin, ymin, xmax, ymax ));
}

static PyObject *PyFFLayer_xBoundsAtY(PyFF_Layer *self, PyObject *args) {
    SplineSet *ss;
    double y1, y2=6.023e23;
    int ret;
    bigreal xmin, xmax;

    if ( !PyArg_ParseTuple(args,"d|d", &y1, &y2 ))
return( NULL );

    ss = SSFromLayer((PyFF_Layer *) self);
    if ( ss==NULL )
Py_RETURN_NONE;

    if ( y2>1e23 )
	y2 = y1;
    ret = SSBoundsWithin(ss,y1,y2,&xmin,&xmax,1);
    SplinePointListsFree(ss);
    if ( !ret )
Py_RETURN_NONE;

return( Py_BuildValue("(dd)",(double) xmin, (double) xmax ) );
}

static PyObject *PyFFLayer_yBoundsAtX(PyFF_Layer *self, PyObject *args) {
    SplineSet *ss;
    double x1, x2=6.023e23;
    int ret;
    bigreal ymin, ymax;

    if ( !PyArg_ParseTuple(args,"d|d", &x1, &x2 ))
return( NULL );

    ss = SSFromLayer((PyFF_Layer *) self);
    if ( ss==NULL )
Py_RETURN_NONE;

    if ( x2>1e23 )
	x2 = x1;
    ret = SSBoundsWithin(ss,x1,x2,&ymin,&ymax,0);
    SplinePointListsFree(ss);
    if ( !ret )
Py_RETURN_NONE;

return( Py_BuildValue("(dd)",(double) ymin, (double) ymax ) );
}

static PyObject *PyFFLayer_draw(PyFF_Layer *self, PyObject *args) {
    int i;

    for ( i=0; i<self->cntr_cnt; ++i )
	PyFFContour_draw(self->contours[i],args);
Py_RETURN( self );
}

static PyMethodDef PyFFLayer_methods[] = {
    {"dup", (PyCFunction)PyFFLayer_dup, METH_NOARGS,
	     "Returns a deep copy of the layer" },
    {"isEmpty", (PyCFunction)PyFFLayer_IsEmpty, METH_NOARGS,
	     "Returns whether a layer contains no contours" },
    {"similar", (PyCFunction)PyFFLayer_similar, METH_VARARGS,
	     "compares two layers" },
    {"simplify", (PyCFunction)PyFFLayer_Simplify, METH_VARARGS,
	     "Smooths a layer" },
    {"selfIntersects", (PyCFunction)PyFFLayer_selfIntersects, METH_NOARGS,
	     "Returns whether this layer intersects itself" },
    {"transform", (PyCFunction)PyFFLayer_Transform, METH_VARARGS,
	     "Transform a layer by a 6 element matrix." },
    { "nltransform", (PyCFunction)PyFFLayer_NLTransform, METH_VARARGS,
	    "Transform a glyph by two non-linear expressions (one for x, one for y)." },
    {"addExtrema", (PyCFunction)PyFFLayer_AddExtrema, METH_VARARGS,
	     "Add Extrema to a layer" },
	{"addInflections", (PyCFunction)PyFFLayer_AddInflections, METH_NOARGS,
	     "Add points of inflection to a layer" },
	{"balance", (PyCFunction)PyFFLayer_Balance, METH_NOARGS,
	     "Balance handles on a layer" },
	{"harmonize", (PyCFunction)PyFFLayer_Harmonize, METH_NOARGS,
	     "Harmonize curvatures on a layer" },
    {"round", (PyCFunction)PyFFLayer_Round, METH_VARARGS,
	     "Round contours on a layer" },
    {"cluster", (PyCFunction)PyFFLayer_Cluster, METH_VARARGS,
	     "Round contours on a layer" },
    {"interpolateNewLayer", (PyCFunction)PyFFLayer_interpolateNewLayer, METH_VARARGS,
	     "Creates a new layer by interpolating between this one and the one in the first argument" },
    {"boundingBox", (PyCFunction)PyFFLayer_BoundingBox, METH_NOARGS,
	     "Finds a bounding box for the layer (xmin,ymin,xmax,ymax)" },
    {"xBoundsAtY", (PyCFunction)PyFFLayer_xBoundsAtY, METH_VARARGS,
	     "The minimum and maximum values of x attained for a given y, or returns None" },
    {"yBoundsAtX", (PyCFunction)PyFFLayer_yBoundsAtX, METH_VARARGS,
	     "The minimum and maximum values of y attained for a given x, or returns None" },
    {"correctDirection", (PyCFunction)PyFFLayer_Correct, METH_NOARGS,
	     "Orient a layer so that external contours are clockwise and internal counter clockwise." },
    {"reverseDirection", (PyCFunction)PyFFLayer_ReverseDirection, METH_NOARGS,
	     "Reverse the orientation of each contour in the layer." },
    {"export", (PyCFunction)PyFFLayer_export, METH_VARARGS | METH_KEYWORDS,
	     "Exports the layer to a file" },
    {"stroke", (PyCFunction)PyFFLayer_Stroke, METH_VARARGS | METH_KEYWORDS,
	     "Strokes the contours in a layer" },
    {"removeOverlap", (PyCFunction)PyFFLayer_RemoveOverlap, METH_NOARGS,
	     "Remove overlapping areas from a layer." },
    {"intersect", (PyCFunction)PyFFLayer_Intersect, METH_NOARGS,
	     "Leaves the areas where the contours of a layer overlap." },
    {"exclude", (PyCFunction)PyFFLayer_Exclude, METH_VARARGS,
	     "Exclude the area of the argument (also a layer) from the current layer" },
    {"stemControl", (PyCFunction)PyFFLayer_stemControl, METH_VARARGS,
	     "Allows you to scale stems and counters differently" },
    {"draw", (PyCFunction)PyFFLayer_draw, METH_VARARGS,
	     "Support for the \"pen\" protocol (I hope)\nhttp://just.letterror.com/ltrwiki/PenProtocol" },
    {"__reduce__", (PyCFunction)PyFFLayer_pickleReducer, METH_NOARGS,
	     "cPickle calls this routine when it wants to pickle us" },
    PYMETHODDEF_EMPTY /* Sentinel */
};

PyTypeObject PyFF_LayerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "fontforge.layer",         /* tp_name */
    sizeof(PyFF_Layer),        /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)PyFFLayer_dealloc, /* tp_dealloc */
    0,                         /* tp_vectorcall_offset */
    NULL,                      /* tp_getattr */
    NULL,                      /* tp_setattr */
    NULL,                      /* tp_reserved/tp_compare */
    NULL,                      /* tp_repr */
    NULL,                      /* tp_as_number */
    &PyFFLayer_Sequence,       /* tp_as_sequence */
    NULL,                      /* tp_as_mapping */
    NULL,                      /* tp_hash */
    NULL,                      /* tp_call */
    (reprfunc) PyFFLayer_Str,  /* tp_str */
    NULL,                      /* tp_getattro */
    NULL,                      /* tp_setattro */
    NULL,                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,/* tp_flags */
    "fontforge Layer objects", /* tp_doc */
    NULL /*(traverseproc)FFLayer_traverse*/,  /* tp_traverse */
    (inquiry)PyFFLayer_clear,  /* tp_clear */
    (richcmpfunc)PyFFLayer_richcompare, /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    layeriter_new,             /* tp_iter */
    NULL,                      /* tp_iternext */
    PyFFLayer_methods,         /* tp_methods */
    NULL,                      /* tp_members */
    PyFFLayer_getset,          /* tp_getset */
    NULL,                      /* tp_base */
    NULL,                      /* tp_dict */
    NULL,                      /* tp_descr_get */
    NULL,                      /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)PyFFLayer_init,  /* tp_init */
    NULL,                      /* tp_alloc */
    PyFFLayer_new,             /* tp_new */
    NULL,                      /* tp_free */
    NULL,                      /* tp_is_gc */
    NULL,                      /* tp_bases */
    NULL,                      /* tp_mro */
    NULL,                      /* tp_cache */
    NULL,                      /* tp_subclasses */
    NULL,                      /* tp_weaklist */
    NULL,                      /* tp_del */
    0,                         /* tp_version_tag */
};

/* ************************************************************************** */
/* Conversion routines between SplineSets and Contours & Layers */
/* ************************************************************************** */
static int SSSelectOnCurve(SplineSet *ss,int pos) {
    SplinePoint *sp;

    while ( ss!=NULL ) {
	for ( sp=ss->first ; ; ) {
	    if ( sp->ttfindex == pos ) {
		sp->selected = true;
return( true );
	    }
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp == ss->first )
	break;
	}
	ss = ss->next;
    }
return( 0 );
}

/* Point conversion flags: see 'enum pconvert_flags' in splinefont.h */
struct flaglist pconvertflags[] = {
    { "select_none", pconvert_flag_none },
    { "select_all", pconvert_flag_all },
    { "select_smooth", pconvert_flag_smooth },
    { "select_incompat", pconvert_flag_incompat },
    { "by_geom", pconvert_flag_by_geom },
    { "force", pconvert_flag_force_type },
    { "downgrade", pconvert_flag_downgrade },
    { "check", pconvert_flag_check_compat },
    { "hvcurve", pconvert_flag_hvcurve },
    FLAGLIST_EMPTY /* Sentinel */
};

static int CheckPConvertFlags(int flags, int defaults) {
	const int sels = pconvert_flag_none|pconvert_flag_all|pconvert_flag_smooth;
	const int modes = pconvert_flag_by_geom|pconvert_flag_force_type
	                  |pconvert_flag_downgrade|pconvert_flag_check_compat;
	int defsel = defaults&sels, defmode = defaults&modes, sel, mode;

	sel = flags&sels;
	if ( sel==0 )
		flags |= defsel;
	else if ( ( sel & (sel-1) ) != 0 ) {
		PyErr_Format(PyExc_ValueError, "At most one point selection flag is allowed.");
		return -1;
	}

	mode = flags&modes;
	if ( mode==0 )
		flags |= defmode; // Harmless if sel == pconvert_flag_none
	else if ( ( mode & (mode-1) ) != 0 ) {
		PyErr_Format(PyExc_ValueError, "At most one point conversion mode flag is allowed.");
		return -1;
	}
	// Don't worry about extraneous bits for now.
	return flags;
}

// Will return NULL on error or empty contour, can check python error
// status to tell which.
static SplineSet *_SSFromContour(PyFF_Contour *c,int *tt_start, int flags) {
    int start = 0, next;
    int i, skipped=0, index, ok;
    int nexti, previ;
    SplineSet *ss;
    SplinePoint *sp;

    if ( c->pt_cnt==0 ) {
	/*PyErr_Format(PyExc_TypeError, "Empty contour");*/
return( NULL );
    }

    if ( tt_start!=NULL )
	start = *tt_start;
    i = 0;
    next = start;

    ss = chunkalloc(sizeof(SplineSet));
    if ( ss==NULL )
	return( NULL );
    if ( c->spiro_cnt!=0 ) {
	ss->spiro_cnt = ss->spiro_max = c->spiro_cnt;
	ss->spiros = SpiroCPCopy(c->spiros,NULL);
    }
    ss->contour_name = copy(c->name);

    if ( c->is_quadratic ) {
	if ( !c->points[0]->on_curve ) {
	    if ( c->pt_cnt==1 ) {
		// XXX One off-curve point -- shouldn't this throw an error?
		ss->first = ss->last = SplinePointCreate(c->points[0]->x,c->points[0]->y);
		ss->start_offset = 0;
		ss->first->selected = c->points[0]->selected;
		ss->first->pointtype = PyFF_ConvertToPointType(c->points[0]->type);
		ss->first->name = copy(c->points[0]->name);
		ok = _SPLCategorizePoints(ss, flags);
		if ( !ok ) {
		    SplinePointListsFree(ss);
		    // assert ( flags&pconvert_flag_check );
		    PyErr_Format(PyExc_TypeError, "At least one point has a geometry incompatible with its type");
		    return( NULL );
		}
		return( ss );
	    }
	    ++i;
	    ++next;
	    skipped = true;
	}
	while ( i<c->pt_cnt ) {
	    if ( c->points[i]->on_curve ) {
		sp = SplinePointCreate(c->points[i]->x,c->points[i]->y);
		sp->pointtype = PyFF_ConvertToPointType(c->points[i]->type);
		sp->selected = c->points[i]->selected;
		sp->name = copy(c->points[i]->name);
		sp->ttfindex = next++;
		index = -1;
		if ( i>0 && !c->points[i-1]->on_curve )
		    index = i-1;
		else if ( i==0 && !c->points[c->pt_cnt-1]->on_curve )
		    index = c->pt_cnt-1;
		if ( index!=-1 ) {
		    sp->prevcp.x = c->points[index]->x;
		    sp->prevcp.y = c->points[index]->y;
		    sp->prevcpselected = c->points[index]->selected;
		}
		if ( ss->last==NULL ) {
		    ss->first = sp;
		    ss->start_offset = 0;
		} else
		    SplineMake2(ss->last,sp);
		ss->last = sp;
	    } else {
		if ( !c->points[i-1]->on_curve ) {
		    sp = SplinePointCreate((c->points[i]->x+c->points[i-1]->x)/2,(c->points[i]->y+c->points[i-1]->y)/2);
		    sp->pointtype = PyFF_ConvertToPointType(c->points[i]->type);
		    sp->ttfindex = -1;
		    sp->prevcp.x = c->points[i-1]->x;
		    sp->prevcp.y = c->points[i-1]->y;
		    sp->prevcpselected = c->points[i-1]->selected;
		    if ( ss->last==NULL ) {
			ss->first = sp;
			ss->start_offset = 0;
		    } else
			SplineMake2(ss->last,sp);
		    ss->last = sp;
		}
		ss->last->nextcp.x = c->points[i]->x;
		ss->last->nextcp.y = c->points[i]->y;
		ss->last->nextcpselected = c->points[i]->selected;
		ss->last->nextcpindex = next++;
	    }
	    ++i;
	}
	if ( skipped ) {
	    i = c->pt_cnt;
	    if ( !c->points[i-1]->on_curve ) {
		sp = SplinePointCreate((c->points[0]->x+c->points[i-1]->x)/2,(c->points[0]->y+c->points[i-1]->y)/2);
		sp->ttfindex = -1;
		sp->prevcp.x = c->points[i-1]->x;
		sp->prevcp.y = c->points[i-1]->y;
		sp->prevcpselected = c->points[i-1]->selected;
		if ( ss->last==NULL ) {
		    ss->first = sp;
		    ss->start_offset = 0;
		} else
		    SplineMake2(ss->last,sp);
		ss->last = sp;
	    }
	    ss->last->nextcp.x = c->points[0]->x;
	    ss->last->nextcp.y = c->points[0]->y;
	    ss->last->nextcpselected = c->points[0]->selected;
	    ss->last->nextcpindex = start;
	}
    } else {
	for ( i=0; i<c->pt_cnt; ++i ) {
	    if ( c->points[i]->on_curve )
	break;
	    ++next;
	}
	for ( i=0; i<c->pt_cnt; ++i ) {
	    if ( !c->points[i]->on_curve )
	continue;
	    sp = SplinePointCreate(c->points[i]->x,c->points[i]->y);
	    sp->pointtype = PyFF_ConvertToPointType(c->points[i]->type);
	    sp->selected = c->points[i]->selected;
	    sp->name = copy(c->points[i]->name);
	    sp->ttfindex = next++;
	    nexti = previ = -1;
	    if ( i==0 )
		previ = c->pt_cnt-1;
	    else
		previ = i-1;
	    if ( !c->points[previ]->on_curve ) {
		sp->prevcp.x = c->points[previ]->x;
		sp->prevcp.y = c->points[previ]->y;
		sp->prevcpselected = c->points[previ]->selected;
	    }
	    if ( i==c->pt_cnt-1 )
		nexti = 0;
	    else
		nexti = i+1;
	    if ( !c->points[nexti]->on_curve ) {
		sp->nextcp.x = c->points[nexti]->x;
		sp->nextcp.y = c->points[nexti]->y;
		sp->nextcpselected = c->points[nexti]->selected;
		next += 2;
		if ( nexti==c->pt_cnt-1 )
		    nexti = 0;
		else
		    ++nexti;
		if ( c->points[nexti]->on_curve ) {
		    SplinePointListsFree(ss);
                    free(sp);
		    PyErr_Format(PyExc_TypeError, "In cubic splines there must be exactly 2 control points between on curve points");
return( NULL );
		}
		if ( nexti==c->pt_cnt-1 )
		    nexti = 0;
		else
		    ++nexti;
		if ( !c->points[nexti]->on_curve ) {
		    SplinePointListsFree(ss);
                    free(sp);
		    PyErr_Format(PyExc_TypeError, "In cubic splines there must be exactly 2 control points between on curve points");
return( NULL );
		}
	    }
	    if ( ss->last==NULL ) {
		ss->first = sp;
		ss->start_offset = 0;
	    } else
		SplineMake3(ss->last,sp);
	    ss->last = sp;
	}
	if ( ss->last==NULL ) {
	    SplinePointListsFree(ss);
	    PyErr_Format(PyExc_TypeError, "Contour has points but none are on-curve");
return( NULL );
	}
    }
    if ( c->closed ) {

	SplineMake(ss->last,ss->first,c->is_quadratic);
	if ( c->is_quadratic && (ss->last->nextcpselected || ss->first->prevcpselected) ) {
            // The convention for tracking selection of quadratic control
	    // points is to use nextcpselected except at the tail of the
	    // list, where it's prevcpselected on the first point.
	     ss->last->nextcpselected = false;
	     ss->first->prevcpselected = true;
	}
	ss->last = ss->first;
    }
    if ( tt_start!=NULL )
	*tt_start = next;
    ok = _SPLCategorizePoints(ss, flags);
    if ( !ok ) {
	SplinePointListsFree(ss);
	// assert ( flags&pconvert_flag_check );
	PyErr_Format(PyExc_TypeError, "At least one point has a geometry incompatible with its type");
	return( NULL );
    }
    return( ss );
}

SplineSet *SSFromContour(PyFF_Contour *c,int *tt_start) {
	return _SSFromContour(c, tt_start, pconvert_flag_none);
}

static PyFF_Contour *ContourFromSS(SplineSet *ss,PyFF_Contour *ret) {
    int k, cnt;
    SplinePoint *sp;

    if ( ret==NULL )
	ret = (PyFF_Contour *) PyFFContour_new(&PyFF_ContourType,NULL,NULL);
    else
	PyFFContour_clear(ret);
    if ( ss->spiro_cnt!=0 ) {
	ret->spiro_cnt = ss->spiro_cnt;
	ret->spiros = SpiroCPCopy(ss->spiros,NULL);
    }
    ret->name = copy(ss->contour_name);
    ret->closed = ss->first->prev!=NULL;
    for ( k=0; k<2; ++k ) {
	if ( ss->first->next == NULL ) {
	    if ( k )
		ret->points[0] = PyFFPoint_CNew(ss->first->me.x,ss->first->me.y,true,
		                                ss->first->selected,PyFF_ConvertFromPointType(ss->first->pointtype),
						ss->first->name);
	    cnt = 1;
	} else if ( ss->first->next->order2 ) {
	    ret->is_quadratic = true;
	    cnt = 0;
	    for ( sp=ss->first; ; ) {
		if ( k ) {
		    ret->points[cnt] = PyFFPoint_CNew(sp->me.x,sp->me.y,true,sp->selected,
		                                      PyFF_ConvertFromPointType(sp->pointtype), sp->name);
		    ret->points[cnt]->interpolated = SPInterpolate(sp);
		}
		++cnt;
		if ( !sp->nonextcp ) {
		    if ( k )
			ret->points[cnt] = PyFFPoint_CNew(sp->nextcp.x,sp->nextcp.y,false,
			                                  sp->nextcpselected,0,sp->name);
		    ++cnt;
		}
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( sp==ss->first )
	    break;
	    }
	    if ( k && ss->first->prevcpselected && ! ret->points[cnt-1]->on_curve )
		ret->points[cnt-1]->selected = true;
	} else {
	    ret->is_quadratic = false;
	    for ( sp=ss->first, cnt=0; ; ) {
		if ( k )
		    ret->points[cnt] = PyFFPoint_CNew(sp->me.x,sp->me.y,true, sp->selected,
		                                      PyFF_ConvertFromPointType(sp->pointtype), sp->name);
		++cnt;			/* Sp itself */
		if ( sp->next==NULL )
	    break;
		if ( !sp->nonextcp || !sp->next->to->noprevcp ) {
		    if ( k ) {
			ret->points[cnt  ] = PyFFPoint_CNew(sp->nextcp.x,sp->nextcp.y,false,sp->nextcpselected,0,NULL);
			ret->points[cnt+1] = PyFFPoint_CNew(sp->next->to->prevcp.x,sp->next->to->prevcp.y,false,sp->next->to->prevcpselected,0,NULL);
		    }
		    cnt += 2;		/* not a line => 2 control points */
		}
		sp = sp->next->to;
		if ( sp==ss->first )
	    break;
	    }
	}
	if ( !k ) {
	    if ( cnt>=ret->pt_max ) {
		ret->pt_max = cnt;
		PyMem_Resize(ret->points,PyFF_Point *,cnt);  /* Messes with ret->points */
	    }
	    ret->pt_cnt = cnt;
	}
    }
return( ret );
}

static SplineSet *_SSFromLayer(PyFF_Layer *layer, int flags) {
    int start = 0;
    SplineSet *head=NULL, *tail, *cur;
    int i;

    for ( i=0; i<layer->cntr_cnt; ++i ) {
	cur = _SSFromContour( layer->contours[i], &start, flags );
	if ( cur!=NULL ) {
	    if ( head==NULL )
		head = cur;
	    else
		tail->next = cur;
	    tail = cur;
	} else if ( PyErr_Occurred() != NULL ) {
	    SplinePointListsFree(head);
	    return( NULL );
	}
	// Ignore empty contours
    }
    return( head );
}

static SplineSet *SSFromLayer(PyFF_Layer *layer) {
	return _SSFromLayer(layer, pconvert_flag_none);
}

static PyFF_Layer *LayerFromSS(SplineSet *ss,PyFF_Layer *layer) {
    SplineSet *cur;
    int cnt, i;

    if ( layer==NULL )
	layer = (PyFF_Layer *) PyFFLayer_new(&PyFF_LayerType,NULL,NULL);

    for ( cnt=0, cur=ss; cur!=NULL; cur=cur->next, ++cnt );
    if ( cnt>layer->cntr_max ) {
         /* Messes with layer->contours */
        PyMem_Resize(layer->contours,PyFF_Contour *,layer->cntr_max = cnt );
    }
    if ( cnt>layer->cntr_cnt ) {
	for ( i=layer->cntr_cnt; i<cnt; ++i )
	    layer->contours[i] = NULL;
    } else if ( cnt<layer->cntr_cnt ) {
	for ( i = cnt; i<layer->cntr_cnt; ++i )
	    Py_DECREF( (PyObject *) layer->contours[i] );
    }
    layer->cntr_cnt = cnt;
    for ( cnt=0, cur=ss; cur!=NULL; cur=cur->next, ++cnt ) {
	layer->contours[cnt] = ContourFromSS(cur,layer->contours[cnt]);
	layer->is_quadratic = layer->contours[cnt]->is_quadratic;
    }
return( layer );
}

static PyFF_Layer *LayerFromLayer(Layer *inlayer,PyFF_Layer *ret) {
    /* May want to copy fills and pens someday!! */
    PyFF_Layer *layer = LayerFromSS(inlayer->splines,ret);
    layer->is_quadratic = inlayer->order2;
return( layer );
}

/* ************************************************************************** */
/* GlyphPen Standard Methods */
/* ************************************************************************** */
static PyTypeObject PyFF_GlyphPenType;

static void PyFF_GlyphPen_dealloc(PyFF_GlyphPen *self) {
    if ( self->sc!=NULL ) {
	if ( self->changed )
	    SCCharChangedUpdate(self->sc,self->layer);
	self->sc = NULL;
    }
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *PyFFGlyphPen_Str(PyFF_GlyphPen *self) {
return( PyUnicode_FromFormat( "<GlyphPen for %s>", self->sc->name ));
}

/* ************************************************************************** */
/*  GlyphPen Methods  */
/* ************************************************************************** */
static void GlyphClear(PyObject *self) {
    SplineChar *sc = ((PyFF_GlyphPen *) self)->sc;
    int layer = ((PyFF_GlyphPen *) self)->layer;
    SCClearContents(sc,layer);
    ((PyFF_GlyphPen *) self)->replace = false;
}

static PyObject *PyFFGlyphPen_moveTo(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_GlyphPen *) self)->sc;
    int layer = ((PyFF_GlyphPen *) self)->layer;
    SplineSet *ss;
    double x,y;

    if ( !((PyFF_GlyphPen *) self)->ended ) {
	PyErr_Format(PyExc_EnvironmentError, "The moveTo operator may not be called while drawing a contour");
return( NULL );
    }
    if ( !PyArg_ParseTuple( args, "(dd)", &x, &y )) {
	PyErr_Clear();
	if ( !PyArg_ParseTuple( args, "dd", &x, &y ))
return( NULL );
    }
    if ( ((PyFF_GlyphPen *) self)->replace )
	GlyphClear(self);
    ss = chunkalloc(sizeof(SplineSet));
    ss->next = sc->layers[layer].splines;
    sc->layers[layer].splines = ss;
    ss->first = ss->last = SplinePointCreate(x,y);
    ss->start_offset = 0;

    ((PyFF_GlyphPen *) self)->ended = false;
    ((PyFF_GlyphPen *) self)->changed = true;
Py_RETURN( self );
}

static PyObject *PyFFGlyphPen_lineTo(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_GlyphPen *) self)->sc;
    int layer = ((PyFF_GlyphPen *) self)->layer;
    SplinePoint *sp;
    SplineSet *ss;
    double x,y;

    if ( ((PyFF_GlyphPen *) self)->ended ) {
	PyErr_Format(PyExc_EnvironmentError, "The lineTo operator must be preceded by a moveTo operator" );
return( NULL );
    }
    if ( !PyArg_ParseTuple( args, "(dd)", &x, &y )) {
	PyErr_Clear();
	if ( !PyArg_ParseTuple( args, "dd", &x, &y ))
return( NULL );
    }
    ss = sc->layers[layer].splines;
    sp = SplinePointCreate(x,y);
    SplineMake(ss->last,sp,sc->layers[layer].order2);
    ss->last = sp;

Py_RETURN( self );
}

static PyObject *PyFFGlyphPen_curveTo(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_GlyphPen *) self)->sc;
    int layer = ((PyFF_GlyphPen *) self)->layer;
    SplinePoint *sp;
    SplineSet *ss;
    double x[3],y[3];

    if ( ((PyFF_GlyphPen *) self)->ended ) {
	PyErr_Format(PyExc_EnvironmentError, "The curveTo operator must be preceded by a moveTo operator" );
return( NULL );
    }
    if ( sc->layers[layer].order2 ) {
	if ( !PyArg_ParseTuple( args, "(dd)(dd)", &x[1], &y[1], &x[2], &y[2] )) {
	    PyErr_Clear();
	    if ( !PyArg_ParseTuple( args, "dddd", &x[1], &y[1], &x[2], &y[2] ))
return( NULL );
	}
	x[0] = x[1]; y[0] = y[1];
    } else {
	if ( !PyArg_ParseTuple( args, "(dd)(dd)(dd)", &x[0], &y[0], &x[1], &y[1], &x[2], &y[2] )) {
	    PyErr_Clear();
	    if ( !PyArg_ParseTuple( args, "dddddd", &x[0], &y[0], &x[1], &y[1], &x[2], &y[2] ))
return( NULL );
	}
    }
    ss = sc->layers[layer].splines;
    sp = SplinePointCreate(x[2],y[2]);
    sp->prevcp.x = x[1]; sp->prevcp.y = y[1];
    ss->last->nextcp.x = x[0], ss->last->nextcp.y = y[0];
    SplineMake(ss->last,sp,sc->layers[layer].order2);
    ss->last = sp;

Py_RETURN( self );
}

static PyObject *PyFFGlyphPen_qCurveTo(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_GlyphPen *) self)->sc;
    int layer = ((PyFF_GlyphPen *) self)->layer;
    SplinePoint *sp;
    SplineSet *ss;
    double x0,y0, x1,y1, x2,y2;
    int len, i;
    PyObject *pt_tuple;

    if ( !sc->layers[layer].order2 ) {
	PyErr_Format(PyExc_EnvironmentError, "qCurveTo only applies to quadratic fonts" );
return( NULL );
    }

    len = PySequence_Size(args);
    if ( len < 2 ) {
	PyErr_Format(PyExc_EnvironmentError, "qCurveTo requires at least two arguments");
return( NULL );
    }
    else if ( PySequence_GetItem(args,len-1)== Py_None ) {
	--len;
	if ( !((PyFF_GlyphPen *) self)->ended ) {
	    PyErr_Format(PyExc_EnvironmentError, "qCurveTo must describe an entire contour if its last argument is None");
return( NULL );
	} else if ( len<2 ) {
	    PyErr_Format(PyExc_EnvironmentError, "qCurveTo must have at least two tuples");
return( NULL );
	}

	pt_tuple = PySequence_GetItem(args,0);
	if ( !PyArg_ParseTuple(pt_tuple,"dd", &x0, &y0 ))
return( NULL );

	ss = chunkalloc(sizeof(SplineSet));
	ss->next = sc->layers[layer].splines;
	sc->layers[layer].splines = ss;

	x1 = x0; y1 = y0;
	for ( i=1; i<len; ++i ) {
	    pt_tuple = PySequence_GetItem(args,i);
	    if ( !PyArg_ParseTuple(pt_tuple,"dd", &x2, &y2 ))
return( NULL );
	    sp = SplinePointCreate((x1+x2)/2,(y1+y2)/2);
	    sp->prevcp.x = x1; sp->prevcp.y = y1;
	    sp->nextcp.x = x2; sp->nextcp.y = y2;
	    if ( ss->first==NULL ) {
		ss->first = ss->last = sp;
		ss->start_offset = 0;
	    } else {
		SplineMake2(ss->last,sp);
		ss->last = sp;
	    }
	    x1=x2; y1=y2;
	}
	sp = SplinePointCreate((x0+x2)/2,(y0+y2)/2);
	sp->prevcp.x = x2; sp->prevcp.y = y2;
	sp->nextcp.x = x0; sp->nextcp.y = y0;
	SplineMake2(ss->last,sp);
	SplineMake2(sp,ss->first);
	ss->last = ss->first;

	/*((PyFF_GlyphPen *) self)->ended = true;*/
	((PyFF_GlyphPen *) self)->changed = true;
    } else {
	if ( ((PyFF_GlyphPen *) self)->ended ) {
	    PyErr_Format(PyExc_EnvironmentError, "The qCurveTo operator must be preceded by a moveTo operator" );
return( NULL );
	} else if ( len<2 ) {
	    PyErr_Format(PyExc_EnvironmentError, "qCurveTo must have at least two tuples");
return( NULL );
	}
	ss = sc->layers[layer].splines;
	pt_tuple = PySequence_GetItem(args,0);
	if ( !PyArg_ParseTuple(pt_tuple,"dd", &x1, &y1 ))
return( NULL );
	ss->last->nextcp.x = x1; ss->last->nextcp.y = y1;
	for ( i=1; i<len-1; ++i ) {
	    pt_tuple = PySequence_GetItem(args,i);
	    if ( !PyArg_ParseTuple(pt_tuple,"dd", &x2, &y2 ))
return( NULL );
	    sp = SplinePointCreate((x1+x2)/2,(y1+y2)/2);
	    sp->prevcp.x = x1; sp->prevcp.y = y1;
	    sp->nextcp.x = x2; sp->nextcp.y = y2;
	    SplineMake2(ss->last,sp);
	    ss->last = sp;
	    x1=x2; y1=y2;
	}
	pt_tuple = PySequence_GetItem(args,i);
	if ( !PyArg_ParseTuple(pt_tuple,"dd", &x2, &y2 ))
return( NULL );
	sp = SplinePointCreate(x2,y2);
	sp->prevcp.x = x1; sp->prevcp.y = y1;
	SplineMake2(ss->last,sp);
	ss->last = sp;
    }

Py_RETURN( self );
}

static PyObject *PyFFGlyphPen_closePath(PyObject *self, PyObject *UNUSED(args)) {
    SplineChar *sc = ((PyFF_GlyphPen *) self)->sc;
    int layer = ((PyFF_GlyphPen *) self)->layer;
    SplineSet *ss;

    if ( ((PyFF_GlyphPen *) self)->ended ) {
	PyErr_Format(PyExc_EnvironmentError, "The closePath operator requires and open path to close" );
return( NULL );
    }

    ss = sc->layers[layer].splines;
    if ( ss->first!=ss->last && RealNear(ss->first->me.x,ss->last->me.x) &&
	    RealNear(ss->first->me.y,ss->last->me.y)) {
	ss->first->prevcp = ss->last->prevcp;
	ss->first->noprevcp = ss->last->noprevcp;
	ss->last->prev->to = ss->first;
	ss->first->prev = ss->last->prev;
	SplinePointFree(ss->last);
    } else {
	SplineMake(ss->last,ss->first,sc->layers[layer].order2);
    }
    ss->last = ss->first;

    ((PyFF_GlyphPen *) self)->ended = true;
Py_RETURN( self );
}

static PyObject *PyFFGlyphPen_endPath(PyObject *self, PyObject *UNUSED(args)) {

    if ( ((PyFF_GlyphPen *) self)->ended ) {
	PyErr_Format(PyExc_EnvironmentError, "The endPath operator must be preceded path operations" );
return( NULL );
    }

    ((PyFF_GlyphPen *) self)->ended = true;
Py_RETURN( self );
}

static PyObject *PyFFGlyphPen_addComponent(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_GlyphPen *) self)->sc;
    int layer = ((PyFF_GlyphPen *) self)->layer;
    real transform[6];
    SplineChar *rsc;
    double m[6] = {1.0,0.0,0.0,1.0,0.0,0.0};
    char *str;
    int j, selected=false;

    if ( !((PyFF_GlyphPen *) self)->ended ) {
	PyErr_Format(PyExc_EnvironmentError, "The addComponent operator may not be called while drawing a contour");
return( NULL );
    }
    if ( ((PyFF_GlyphPen *) self)->replace )
	GlyphClear(self);

    if ( !PyArg_ParseTuple(args,"s|(dddddd)p",&str,
	    &m[0], &m[1], &m[2], &m[3], &m[4], &m[5], &selected) )
return( NULL );
    rsc = SFGetChar(sc->parent,-1,str);
    if ( rsc==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No glyph named %s", str);
return( NULL );
    }
    for ( j=0; j<6; ++j )
	transform[j] = m[j];
    _SCAddRef(sc,rsc,layer,transform,selected);
    SCCharChangedUpdate(sc,layer);

Py_RETURN( self );
}

static PyMethodDef PyFF_GlyphPen_methods[] = {
    { "moveTo", PyFFGlyphPen_moveTo, METH_VARARGS, "Start a new contour at a point (a two element tuple)" },
    { "lineTo", PyFFGlyphPen_lineTo, METH_VARARGS, "Draws a line from the current point to the argument (a two element tuple)" },
    { "curveTo", PyFFGlyphPen_curveTo, METH_VARARGS, "Draws a cubic or quadratic bezier curve from the current point to the last arg" },
    { "qCurveTo", PyFFGlyphPen_qCurveTo, METH_VARARGS, "Draws a series of quadratic bezier curves" },
    { "closePath", PyFFGlyphPen_closePath, METH_VARARGS, "Closes the current contour (and ends it)" },
    { "endPath", PyFFGlyphPen_endPath, METH_VARARGS, "Ends the current contour (without closing it)" },
    { "addComponent", PyFFGlyphPen_addComponent, METH_VARARGS, "Adds a reference into the glyph" },
    PYMETHODDEF_EMPTY /* Sentinel */
};
/* ************************************************************************** */
/*  GlyphPen Type  */
/* ************************************************************************** */

static PyTypeObject PyFF_GlyphPenType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "fontforge.glyphPen",      /* tp_name */
    sizeof(PyFF_GlyphPen),     /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor) PyFF_GlyphPen_dealloc, /* tp_dealloc */
    0,                         /* tp_vectorcall_offset */
    NULL,                      /* tp_getattr */
    NULL,                      /* tp_setattr */
    NULL,                      /* tp_compare */
    NULL,                      /* tp_repr */
    NULL,                      /* tp_as_number */
    NULL,                      /* tp_as_sequence */
    NULL,                      /* tp_as_mapping */
    NULL,                      /* tp_hash */
    NULL,                      /* tp_call */
    (reprfunc) PyFFGlyphPen_Str,/* tp_str */
    NULL,                      /* tp_getattro */
    NULL,                      /* tp_setattro */
    NULL,                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    "FontForge GlyphPen object",/* tp_doc */
    NULL,                      /* tp_traverse */
    NULL,                      /* tp_clear */
    NULL,                      /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    NULL,                      /* tp_iter */
    NULL,                      /* tp_iternext */
    PyFF_GlyphPen_methods,     /* tp_methods */
    NULL,                      /* tp_members */
    NULL,                      /* tp_getset */
    NULL,                      /* tp_base */
    NULL,                      /* tp_dict */
    NULL,                      /* tp_descr_get */
    NULL,                      /* tp_descr_set */
    0,                         /* tp_dictoffset */
    NULL,/*(initproc)PyFF_GlyphPen_init*/ /* tp_init */
    NULL,                      /* tp_alloc */
    NULL,/*PyFF_GlyphPen_new*/ /* tp_new */
    NULL,                      /* tp_free */
    NULL,                      /* tp_is_gc */
    NULL,                      /* tp_bases */
    NULL,                      /* tp_mro */
    NULL,                      /* tp_cache */
    NULL,                      /* tp_subclasses */
    NULL,                      /* tp_weaklist */
    NULL,                      /* tp_del */
    0,                         /* tp_version_tag */
};

/* ************************************************************************** */
/* Glyph Utilities */
/* ************************************************************************** */

static PyObject *PyFF_Glyph_get_layer_references(PyFF_Glyph *self, int layer) {
    RefChar *ref;
    int cnt;
    SplineChar *sc = self->sc;
    PyObject *tuple;

    for ( ref=sc->layers[layer].refs, cnt=0; ref!=NULL; ++cnt, ref=ref->next );
    tuple = PyTuple_New(cnt);
    for ( ref=sc->layers[layer].refs, cnt=0; ref!=NULL; ++cnt, ref=ref->next )
	PyTuple_SET_ITEM(tuple,cnt,Py_BuildValue("(s(dddddd)O)", ref->sc->name,
		ref->transform[0], ref->transform[1], ref->transform[2],
		ref->transform[3], ref->transform[4], ref->transform[5],
		ref->selected ? Py_True : Py_False));
return( tuple );
}

static int PyFF_Glyph_set_layer_references(PyFF_Glyph *self,PyObject *value,
	int layer) {
    int i, j, cnt, selected;
    double m[6];
    real transform[6];
    char *str;
    SplineChar *sc = self->sc, *rsc;
    SplineFont *sf = sc->parent;
    RefChar *refs, *next;

    if ( !PySequence_Check(value)) {
	PyErr_Format(PyExc_TypeError, "Value must be a tuple of references");
return( -1 );
    }
    cnt = PySequence_Size(value);
    for ( refs=sc->layers[layer].refs; refs!=NULL; refs = next ) {
	next = refs->next;
	SCRemoveDependent(sc,refs,layer);
    }
    sc->layers[layer].refs = NULL;
    for ( i=0; i<cnt; ++i ) {
	m[1] = m[2] = m[4] = m[5] = 0.0;
	m[0] = m[3] = 1.0;
	selected = false;
	if ( !PyArg_ParseTuple(PySequence_GetItem(value,i),"s|(dddddd)p",&str,
		&m[0], &m[1], &m[2], &m[3], &m[4], &m[5], &selected) )
return( -1 );
	rsc = SFGetChar(sf,-1,str);
	if ( rsc==NULL ) {
	    PyErr_Format(PyExc_EnvironmentError, "No glyph named %s", str);
return( -1 );
	}
	for ( j=0; j<6; ++j )
	    transform[j] = m[j];
	_SCAddRef(sc,rsc,layer,transform,selected);
    }
    SCCharChangedUpdate(sc,layer);
return( 0 );
}

static PyObject *PyFF_Glyph_get_a_layer(PyFF_Glyph *self,void *vli) {
    SplineChar *sc = self->sc;
    Layer *layer;
    PyFF_Layer *ly;
    int layeri = (int)(size_t)vli;

    if ( layeri<ly_grid || layeri>=sc->layer_cnt ) {
	PyErr_Format(PyExc_ValueError, "Layer is out of range" );
return( NULL );
    } else if ( layeri==ly_grid )
	layer = &sc->parent->grid;
    else
	layer = &sc->layers[layeri];
    ly = LayerFromLayer(layer,NULL);
return( (PyObject * ) ly );
}

static int PyFF_Glyph_CSetLayer(PyFF_Glyph *self, PyObject *value, int layeri, int flags) {
    SplineChar *sc = self->sc;
    Layer *layer;
    SplineSet *ss = NULL, *newss;
    int isquad;

    if ( layeri<ly_grid || layeri>=sc->layer_cnt ) {
	PyErr_Format(PyExc_ValueError, "Layer is out of range" );
	return( -1 );
    } else if ( layeri==ly_grid )
	layer = &sc->parent->grid;
    else
	layer = &sc->layers[layeri];
    if ( PyType_IsSubtype(&PyFF_LayerType, Py_TYPE(value)) ) {
	isquad = ((PyFF_Layer *) value)->is_quadratic;
	ss = _SSFromLayer( (PyFF_Layer *) value, flags);
    } else if ( PyType_IsSubtype(&PyFF_ContourType, Py_TYPE(value)) ) {
	isquad = ((PyFF_Contour *) value)->is_quadratic;
	ss = _SSFromContour( (PyFF_Contour *) value, NULL, flags);
    } else {
	PyErr_Format(PyExc_TypeError, "Argument must be a layer or a contour" );
	return( -1 );
    }

    if ( PyErr_Occurred() != NULL ) {
	return( -1 );
    }

    if ( layer->order2!=isquad ) {
	if ( layer->order2 )
	    newss = SplineSetsTTFApprox(ss);
	else
	    newss = SplineSetsPSApprox(ss);
	SplinePointListsFree(ss);
	ss = newss;
    }
    SplinePointListsFree(layer->splines);
    layer->splines = ss;

    SCCharChangedUpdate(sc,self->layer);
return( 0 );
}

static int PyFF_Glyph_set_a_layer(PyFF_Glyph *self,PyObject *value, void *vli) {
	return PyFF_Glyph_CSetLayer(self, value, (int)(size_t)vli, pconvert_flag_all|pconvert_flag_by_geom);
}

static int SFFindLayerIndexByName(SplineFont *sf,const char *name) {
    int l;

    if ( name!=NULL ) {
	for ( l=0; l<sf->layer_cnt; ++l ) {
	    if ( strcmp(sf->layers[l].name,name)==0 )
return( l );
	}
    }
    PyErr_Format(PyExc_ValueError, "Bad layer name: %s", name );
return( -1 );
}

/* ************************************************************************** */
/* Glyph helper types */
/* ************************************************************************** */

/* ************************************************************************** */
/* Layers dictionary iterator type */
/* ************************************************************************** */

static PyTypeObject PyFF_LayerArrayType;
typedef struct {
	PyObject_HEAD
	PyFF_LayerArray *layers;
	int pos;
} layersiterobject;
static PyTypeObject PyFF_LayerArrayIterType;

static PyObject *layersiter_new(PyObject *layers) {
    layersiterobject *di;
    di = PyObject_New(layersiterobject, &PyFF_LayerArrayIterType);
    if (di == NULL)
return NULL;
    Py_INCREF(layers);
    di->layers = (PyFF_LayerArray *) layers;
    di->pos = 0;
return (PyObject *)di;
}

static void layersiter_dealloc(layersiterobject *di) {
    Py_XDECREF(di->layers);
    PyObject_Del(di);
}

static PyObject *layersiter_iternextkey(layersiterobject *di) {
    PyFF_LayerArray *d = di->layers;
    SplineFont *sf;

    if (d == NULL )
return NULL;
    sf = d->sc->parent;

    if ( di->pos<sf->layer_cnt )
return( Py_BuildValue("s",sf->layers[di->pos++].name) );

return NULL;
}

static PyTypeObject PyFF_LayerArrayIterType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "fontforge.layer_array_iterator",  /* tp_name */
    sizeof(layersiterobject),  /* tp_basicsize */
    0,                         /* tp_itemsize */
    /* methods */
    (destructor)layersiter_dealloc, /* tp_dealloc */
    0,                         /* tp_vectorcall_offset */
    NULL,                      /* tp_getattr */
    NULL,                      /* tp_setattr */
    NULL,                      /* tp_compare */
    NULL,                      /* tp_repr */
    NULL,                      /* tp_as_number */
    NULL,                      /* tp_as_sequence */
    NULL,                      /* tp_as_mapping */
    NULL,                      /* tp_hash */
    NULL,                      /* tp_call */
    NULL,                      /* tp_str */
    NULL,                      /* tp_getattro */
    NULL,                      /* tp_setattro */
    NULL,                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    NULL,                      /* tp_doc */
    NULL,                      /* tp_traverse */
    NULL,                      /* tp_clear */
    NULL,                      /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    PyObject_SelfIter,         /* tp_iter */
    (iternextfunc)layersiter_iternextkey, /* tp_iternext */
    NULL,                      /* tp_methods */
    NULL,                      /* tp_members */
    NULL,                      /* tp_getset */
    NULL,                      /* tp_base */
    NULL,                      /* tp_dict */
    NULL,                      /* tp_descr_get */
    NULL,                      /* tp_descr_set */
    0,                         /* tp_dictoffset */
    NULL,/*(initproc)PyFF_GlyphPen_init*/ /* tp_init */
    NULL,                      /* tp_alloc */
    NULL,/*PyFF_GlyphPen_new*/ /* tp_new */
    NULL,                      /* tp_free */
    NULL,                      /* tp_is_gc */
    NULL,                      /* tp_bases */
    NULL,                      /* tp_mro */
    NULL,                      /* tp_cache */
    NULL,                      /* tp_subclasses */
    NULL,                      /* tp_weaklist */
    NULL,                      /* tp_del */
    0,                         /* tp_version_tag */
};

/* ************************************************************************** */
/* Layers Array Standard Methods */
/* ************************************************************************** */

static void PyFF_LayerArray_dealloc(PyFF_LayerArray *self) {
    self->sc = NULL;
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *PyFFLayerArray_Str(PyFF_LayerArray *self) {
return( PyUnicode_FromFormat( "<Layers Array for %s>", self->sc->name ));
}

static PyObject *PyFF_LayerArray_get_font(PyFF_LayerArray *self, void *UNUSED(closure)) {
    PyFF_Font *font = PyFF_FontForSC( self->sc );
    if ( font==NULL )
Py_RETURN_NONE;
    Py_INCREF(font);
    return (PyObject*)font;
}

static PyObject *PyFF_LayerArray_get_glyph(PyFF_LayerArray *self, void *UNUSED(closure)) {
    PyFF_Glyph *glyph = PyFF_GlyphForSC( self->sc );
    if ( glyph==NULL )
Py_RETURN_NONE;
    Py_INCREF(glyph);
    return (PyObject*)glyph;
}

/* ************************************************************************** */
/* ****************************** Layers Array ****************************** */
/* ************************************************************************** */

static Py_ssize_t PyFF_LayerArrayLength( PyObject *self ) {
    SplineChar *sc = ((PyFF_LayerArray *) self)->sc;
    if ( sc==NULL )
return( 0 );
    else
return( sc->layer_cnt );
}

static PyObject *LayerFromLayerIndex( SplineChar *sc, PyObject *index ) {
    int layer;

    if ( PyUnicode_Check(index)) {
	const char *name = PyUnicode_AsUTF8(index);
	if (name == NULL) {
	    return NULL;
	}
	layer = SFFindLayerIndexByName(sc->parent,name);
	if ( layer<0 )
return( NULL );
    } else if ( PyLong_Check(index)) {
	layer = PyLong_AsLong(index);
    } else {
	PyErr_Format(PyExc_TypeError, "Index must be a layer name or index" );
return( NULL );
    }
return( PyFF_Glyph_get_a_layer((PyFF_Glyph *) PySC_From_SC(sc),(void *)(size_t)layer));
}

static PyObject *PyFF_LayerArrayIndex( PyObject *self, PyObject *index ) {
    SplineChar *sc = ((PyFF_LayerArray *) self)->sc;
    return LayerFromLayerIndex(sc, index);
}

static int PyFF_LayerArrayIndexAssign( PyObject *self, PyObject *index, PyObject *value ) {
    SplineChar *sc = ((PyFF_LayerArray *) self)->sc;
    int layer;

    if ( PyUnicode_Check(index)) {
	const char *name = PyUnicode_AsUTF8(index);
	if (name == NULL) {
	    return -1;
	}
	layer = SFFindLayerIndexByName(sc->parent,name);
	if ( layer<0 )
return( -1 );
    } else if ( PyLong_Check(index)) {
	layer = PyLong_AsLong(index);
    } else {
	PyErr_Format(PyExc_TypeError, "Index must be a layer name or index" );
return( -1 );
    }
return( PyFF_Glyph_CSetLayer((PyFF_Glyph *) PySC_From_SC(sc),value,layer,pconvert_flag_all|pconvert_flag_by_geom));
}

static PyGetSetDef PyFF_LayerArray_getset[] = {
    {(char *)"font",
     (getter)PyFF_LayerArray_get_font, NULL,
     (char *)"returns the font to which this object belongs", NULL},
    {(char *)"glyph",
     (getter)PyFF_LayerArray_get_glyph, NULL,
     (char *)"returns the glyph to which this object belongs", NULL},
    PYGETSETDEF_EMPTY  /* Sentinel */
};

static PyMappingMethods PyFF_LayerArrayMapping = {
    PyFF_LayerArrayLength,		/* length */
    PyFF_LayerArrayIndex,		/* subscript */
    PyFF_LayerArrayIndexAssign	/* subscript assign */
};

/* ************************************************************************** */
/* ************************* initializer routines *************************** */
/* ************************************************************************** */

static PyTypeObject PyFF_LayerArrayType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "fontforge.layer_array", /* tp_name */
    sizeof(PyFF_LayerArray),   /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor) PyFF_LayerArray_dealloc, /* tp_dealloc */
    0,                         /* tp_vectorcall_offset */
    NULL,                      /* tp_getattr */
    NULL,                      /* tp_setattr */
    NULL,                      /* tp_compare */
    NULL,                      /* tp_repr */
    NULL,                      /* tp_as_number */
    NULL,                      /* tp_as_sequence */
    &PyFF_LayerArrayMapping,   /* tp_as_mapping */
    NULL,                      /* tp_hash */
    NULL,                      /* tp_call */
    (reprfunc) PyFFLayerArray_Str, /*tp_str */
    NULL,                      /* tp_getattro */
    NULL,                      /* tp_setattro */
    NULL,                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    "FontForge layers array",  /* tp_doc */
    NULL,                      /* tp_traverse */
    NULL,                      /* tp_clear */
    NULL,                      /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    layersiter_new,            /* tp_iter */
    NULL,                      /* tp_iternext */
    NULL,                      /* tp_methods */
    NULL,                      /* tp_members */
    PyFF_LayerArray_getset,    /* tp_getset */
    NULL,                      /* tp_base */
    NULL,                      /* tp_dict */
    NULL,                      /* tp_descr_get */
    NULL,                      /* tp_descr_set */
    0,                         /* tp_dictoffset */
    NULL,                      /* tp_init */
    NULL,                      /* tp_alloc */
    NULL,                      /* tp_new */
    NULL,                      /* tp_free */
    NULL,                      /* tp_is_gc */
    NULL,                      /* tp_bases */
    NULL,                      /* tp_mro */
    NULL,                      /* tp_cache */
    NULL,                      /* tp_subclasses */
    NULL,                      /* tp_weaklist */
    NULL,                      /* tp_del */
    0,                         /* tp_version_tag */
};

/* ************************************************************************** */
/* References Array Standard Methods */
/* ************************************************************************** */

static PyTypeObject PyFF_RefArrayType;
static void PyFF_RefArray_dealloc(PyFF_RefArray *self) {
    self->sc = NULL;
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *PyFFReferences_Str(PyFF_RefArray *self) {
return( PyUnicode_FromFormat( "<Layer References Array for %s>", self->sc->name ));
}

/* ************************************************************************** */
/* ****************************** References Array ****************************** */
/* ************************************************************************** */

static Py_ssize_t PyFF_RefArrayLength( PyObject *self ) {
    SplineChar *sc = ((PyFF_RefArray *) self)->sc;
    if ( sc==NULL )
return( 0 );
    else
return( sc->layer_cnt );
}

static PyObject *PyFF_RefArrayIndex( PyObject *self, PyObject *index ) {
    SplineChar *sc = ((PyFF_RefArray *) self)->sc;
    int layer;

    if ( PyUnicode_Check(index)) {
	const char *name = PyUnicode_AsUTF8(index);
	if (name == NULL) {
	    return NULL;
	}
	layer = SFFindLayerIndexByName(sc->parent,name);
	if ( layer<0 )
return( NULL );
    } else if ( PyLong_Check(index)) {
	layer = PyLong_AsLong(index);
    } else {
	PyErr_Format(PyExc_TypeError, "Index must be a layer name or index" );
return( NULL );
    }
return( PyFF_Glyph_get_layer_references((PyFF_Glyph *) PySC_From_SC(sc),layer));
}

static int PyFF_RefArrayIndexAssign( PyObject *self, PyObject *index, PyObject *value ) {
    SplineChar *sc = ((PyFF_RefArray *) self)->sc;
    int layer;

    if ( PyUnicode_Check(index)) {
	const char *name = PyUnicode_AsUTF8(index);
	if (name == NULL) {
	    return -1;
	}
	layer = SFFindLayerIndexByName(sc->parent,name);
	if ( layer<0 )
return( -1 );
    } else if ( PyLong_Check(index)) {
	layer = PyLong_AsLong(index);
    } else {
	PyErr_Format(PyExc_TypeError, "Index must be a layer name or index" );
return( -1 );
    }
return( PyFF_Glyph_set_layer_references((PyFF_Glyph *) PySC_From_SC(sc),value,layer));
}

static PyMappingMethods PyFF_RefArrayMapping = {
    PyFF_RefArrayLength,		/* length */
    PyFF_RefArrayIndex,		/* subscript */
    PyFF_RefArrayIndexAssign	/* subscript assign */
};

/* ************************************************************************** */
/* ************************* initializer routines *************************** */
/* ************************************************************************** */

static PyTypeObject PyFF_RefArrayType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "fontforge.references",    /* tp_name */
    sizeof(PyFF_RefArray),     /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor) PyFF_RefArray_dealloc, /* tp_dealloc */
    0,                         /* tp_vectorcall_offset */
    NULL,                      /* tp_getattr */
    NULL,                      /* tp_setattr */
    NULL,                      /* tp_compare */
    NULL,                      /* tp_repr */
    NULL,                      /* tp_as_number */
    NULL,                      /* tp_as_sequence */
    &PyFF_RefArrayMapping,     /* tp_as_mapping */
    NULL,                      /* tp_hash */
    NULL,                      /* tp_call */
    (reprfunc) PyFFReferences_Str, /* tp_str */
    NULL,                      /* tp_getattro */
    NULL,                      /* tp_setattro */
    NULL,                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    "FontForge layers references array",  /* tp_doc */
    NULL,                      /* tp_traverse */
    NULL,                      /* tp_clear */
    NULL,                      /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    layersiter_new,            /* tp_iter */
    NULL,                      /* tp_iternext */
    NULL,                      /* tp_methods */
    NULL,                      /* tp_members */
    NULL,                      /* tp_getset */
    NULL,                      /* tp_base */
    NULL,                      /* tp_dict */
    NULL,                      /* tp_descr_get */
    NULL,                      /* tp_descr_set */
    0,                         /* tp_dictoffset */
    NULL,                      /* tp_init */
    NULL,                      /* tp_alloc */
    NULL,                      /* tp_new */
    NULL,                      /* tp_free */
    NULL,                      /* tp_is_gc */
    NULL,                      /* tp_bases */
    NULL,                      /* tp_mro */
    NULL,                      /* tp_cache */
    NULL,                      /* tp_subclasses */
    NULL,                      /* tp_weaklist */
    NULL,                      /* tp_del */
    0,                         /* tp_version_tag */
};

/* ************************************************************************** */
/*  Glyph Math Kerning  */
/* ************************************************************************** */

static void PyFFMathKern_dealloc(PyFF_MathKern *self) {
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *PyFFMathKern_Str(PyFF_MathKern *self) {
return( PyUnicode_FromFormat( "<math kerning table for glyph %s>", self->sc->name ));
}

static PyObject *PyFF_MathKern_get_kerns(PyFF_MathKern *self, void *closure) {
    struct mathkernvertex *mkv;
    PyObject *tuple;
    int i;

    if ( self->sc->mathkern==NULL )
Py_RETURN_NONE;
    mkv = &self->sc->mathkern->top_right + (int) (intptr_t) closure;
    if ( mkv->cnt==0 )
Py_RETURN_NONE;

    tuple = PyTuple_New(mkv->cnt);
    for ( i=0; i<mkv->cnt; ++i ) {
	if ( i==mkv->cnt-1 )
	    PyTuple_SetItem(tuple,i,Py_BuildValue( "(ii)", mkv->mkd[i].kern,self->sc->parent->ascent));
	else
	    PyTuple_SetItem(tuple,i,Py_BuildValue( "(ii)", mkv->mkd[i].kern,mkv->mkd[i].height));
    }
return( tuple );
}

static int PyFF_MathKern_set_kerns(PyFF_MathKern *self, PyObject *value, void *closure) {
    struct mathkernvertex *mkv;
    struct mathkerndata *mkd;
    int i, cnt;

    if ( self->sc->mathkern==NULL ) {
	if ( value==Py_None )
return( 0 );
	self->sc->mathkern = chunkalloc(sizeof(struct mathkern));
    }
    mkv = &self->sc->mathkern->top_right + (int) (intptr_t) closure;
    if ( value==Py_None ) {
	MathKernVContentsFree(mkv);
	mkv->cnt = 0;
	mkv->mkd = NULL;
return( 0 );
    }
    if ( !PyTuple_Check(value) && !PyList_Check(value)) {
	PyErr_Format(PyExc_TypeError, "Value must be a tuple or a list" );
return( -1 );
    }
    cnt = PySequence_Size(value);
    mkd = calloc(cnt,sizeof(struct mathkerndata));
    for ( i=0; i<cnt; ++i ) {
	PyObject *obj = PySequence_GetItem(value,i);
	if ( i==cnt-1 && PyLong_Check(obj))
	    mkd[i].kern = PyLong_AsLong(obj);
	else if ( !PyArg_ParseTuple(obj, "hh", &mkd[i].kern, &mkd[i].height )) {
	    free(mkd);
return( -1 );
	}
    }
    MathKernVContentsFree(mkv);
    mkv->cnt = cnt;
    if ( cnt==0 ) {
	free(mkd);
	mkd=NULL;
    }
    mkv->mkd = mkd;
return( 0 );
}

static PyGetSetDef PyFFMathKern_members[] = {
    {(char *)"topRight",
     (getter)PyFF_MathKern_get_kerns, (setter)PyFF_MathKern_set_kerns,
     (char *)"Math Kerning information for the top right corner", (void *) (intptr_t) 0},
    {(char *)"topLeft",
     (getter)PyFF_MathKern_get_kerns, (setter)PyFF_MathKern_set_kerns,
     (char *)"Math Kerning information for the top left corner", (void *) (intptr_t) 1},
    {(char *)"bottomLeft",
     (getter)PyFF_MathKern_get_kerns, (setter)PyFF_MathKern_set_kerns,
     (char *)"Math Kerning information for the bottom left corner", (void *) (intptr_t) 3},
    {(char *)"bottomRight",
     (getter)PyFF_MathKern_get_kerns, (setter)PyFF_MathKern_set_kerns,
     (char *)"Math Kerning information for the bottom right corner", (void *) (intptr_t) 2},
    PYGETSETDEF_EMPTY /* Sentinel */
};

static PyTypeObject PyFF_MathKernType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "fontforge.mathKern",      /* tp_name */
    sizeof(PyFF_MathKern),     /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)PyFFMathKern_dealloc, /* tp_dealloc */
    0,                         /* tp_vectorcall_offset */
    NULL,                      /* tp_getattr */
    NULL,                      /* tp_setattr */
    NULL,                      /* tp_compare */
    NULL,                      /* tp_repr */
    NULL,                      /* tp_as_number */
    NULL,                      /* tp_as_sequence */
    NULL,                      /* tp_as_mapping */
    NULL,                      /* tp_hash */
    NULL,                      /* tp_call */
    (reprfunc) PyFFMathKern_Str, /* tp_str */
    NULL,                      /* tp_getattro */
    NULL,                      /* tp_setattro */
    NULL,                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    "fontforge per glyph math kerning objects", /* tp_doc */
    NULL,                      /* tp_traverse */
    NULL,                      /* tp_clear */
    NULL,                      /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    NULL,                      /* tp_iter */
    NULL,                      /* tp_iternext */
    NULL,                      /* tp_methods */
    NULL,                      /* tp_members */
    PyFFMathKern_members,      /* tp_getset */
    NULL,                      /* tp_base */
    NULL,                      /* tp_dict */
    NULL,                      /* tp_descr_get */
    NULL,                      /* tp_descr_set */
    0,                         /* tp_dictoffset */
    NULL,                      /* tp_init */
    NULL,                      /* tp_alloc */
    NULL,                      /* tp_new */
    NULL,                      /* tp_free */
    NULL,                      /* tp_is_gc */
    NULL,                      /* tp_bases */
    NULL,                      /* tp_mro */
    NULL,                      /* tp_cache */
    NULL,                      /* tp_subclasses */
    NULL,                      /* tp_weaklist */
    NULL,                      /* tp_del */
    0,                         /* tp_version_tag */
};

/* ************************************************************************** */
/* Glyph Standard Methods */
/* ************************************************************************** */

static void PyFF_Glyph_dealloc(PyFF_Glyph *self) {
    if ( self->sc!=NULL ) {
	if ( self->sc->python_sc_object == self )
	    self->sc->python_sc_object = NULL;
	self->sc = NULL;
    }
    Py_XDECREF(self->layers);
    Py_XDECREF(self->refs);
    Py_XDECREF(self->mk);
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *PyFFGlyph_Repr(PyFF_Glyph *self) {
    PyObject *ret;
    char buf[200];
    char *repr = buf;
    size_t space_needed;
    int at;
    const struct altuni *alt;

    /* Get space to hold string. For efficiency try to avoid malloc
     * except for rare cases where more is needed.
     */
    space_needed = 64;
    if ( self->sc!=NULL ) {
	space_needed += strlen(self->sc->name);
	/* Count the number of altuni where vs==-1, so we only get true alternates. */
	for ( alt=self->sc->altuni; alt!=NULL; alt=alt->next )
	    if ( alt->vs==-1 && alt->unienc>=0 )
		space_needed += 9;
    }
    if ( space_needed >= sizeof(buf) )
	repr = malloc( space_needed );

#ifdef DEBUG
    at = sprintf(repr, "<%s at 0x%p sc=0x%p",
		 Py_TYPENAME(self), self, self->sc);
#else
    at = sprintf(repr, "<%s at 0x%p", Py_TYPENAME(self), self);
#endif
    if ( self->sc==NULL ) {
	strcpy( &repr[at], " CLOSED>" );
    }
    else {
	if ( self->sc->unicodeenc >= 0 )
	    at += sprintf( &repr[at], " U+%04X", self->sc->unicodeenc);

	for ( alt=self->sc->altuni; alt!=NULL; alt=alt->next )
	    if ( alt->vs==-1 && alt->unienc>=0 )
		at += sprintf( &repr[at], " U+%04X", alt->unienc );

	at += sprintf( &repr[at], " \"%s\">", self->sc->name);
    }
    ret = PyUnicode_FromString(repr);
    if ( repr != buf )
	free(repr);
    return( ret );
}

static PyObject *PyFFGlyph_Str(PyFF_Glyph *self) {
    if ( self->sc==NULL || self->sc->parent==NULL )
return( PyUnicode_FromString("<Glyph from closed font>") );
return( PyUnicode_FromFormat( "<Glyph %s in font %s>", self->sc->name, self->sc->parent->fontname ));
}

static int PyFFGlyph_docompare(PyFF_Glyph *self,PyObject *other,
	double pt_err, double spline_err) {
    SplineSet *ss2;
    int ret;
    SplinePoint *badpoint;

    if ( PyType_IsSubtype(&PyFF_GlyphType, Py_TYPE(other)) ) {
	SplineChar *sc = self->sc;
	SplineChar *sc2 = ((PyFF_Glyph *) other)->sc;
	int olayer = ((PyFF_Glyph *) other)->layer;
	int ret;
	SplinePoint *dummy;

	ret = CompareLayer(NULL,
		sc->layers[self->layer].splines,sc2->layers[olayer].splines,
		sc->layers[self->layer].refs,sc2->layers[olayer].refs,
		pt_err,spline_err,sc->name,false,&dummy);
return( ret );
    } else if ( PyType_IsSubtype(&PyFF_ContourType, Py_TYPE(other)) ) {
	ss2 = SSFromContour((PyFF_Contour *) other,NULL);
    } else if ( PyType_IsSubtype(&PyFF_LayerType, Py_TYPE(other)) ) {
	ss2 = SSFromLayer((PyFF_Layer *) other);
    } else {
	PyErr_Format(PyExc_TypeError, "Unexpected type");
return( -1 );
    }
    if ( PyErr_Occurred() != NULL ) {
	return( -1 );
    }
    if ( self->sc->layers[self->layer].refs!=NULL )
return( SS_NoMatch | SS_RefMismatch );
    ret = SSsCompare(self->sc->layers[self->layer].splines,ss2,pt_err,spline_err,&badpoint);
    SplinePointListsFree(ss2);
return(ret);
}

static int PyFFGlyph_compare(PyFF_Glyph *self,PyObject *other) {
    const double pt_err = .5, spline_err = 1;
    int ret;
    SplineChar *sc1, *sc2;

    ret = PyFFGlyph_docompare(self,other,pt_err,spline_err);
    if ( !(ret&SS_NoMatch) )
return( 0 );

    /* There's no real ordering on these guys. Make up something that is */
    /*  at least consistent */
    if ( !PyType_IsSubtype(&PyFF_GlyphType, Py_TYPE(other)) )
return( -1 );
    /* Ok, both are glyphs */
    sc1 = self->sc; sc2 = ((PyFF_Glyph *) other)->sc;
return( sc1<sc2 ? -1 : 1 );
}

static PyObject *PyFFGlyph_richcompare(PyObject *a, PyObject *b, int op) {
    return enrichened_compare((cmpfunc) PyFFGlyph_compare, a, b, op);
}

/* ************************************************************************** */
/* Glyph getters/setters */
/* ************************************************************************** */

static PyObject *PyFF_Glyph_get_temporary(PyFF_Glyph *self, void *UNUSED(closure)) {
    if ( self->sc->python_temporary==NULL )
Py_RETURN_NONE;
    Py_INCREF( (PyObject *) (self->sc->python_temporary) );
return( self->sc->python_temporary );
}

static int PyFF_Glyph_set_temporary(PyFF_Glyph *self,PyObject *value, void *UNUSED(closure)) {
    PyObject *old = self->sc->python_temporary;

    /* I'd rather not store None, because C routines don't understand it */
    /*  and they occasionally need to know whether there is something real */
    /*  in this field. */
    if ( value==Py_None )
	value = NULL;
    Py_XINCREF(value);
    self->sc->python_temporary = value;
    Py_XDECREF(old);
return( 0 );
}

static PyObject *PyFF_Glyph_get_persistent(PyFF_Glyph *self, void *UNUSED(closure)) {
    if ( self->sc->layer_cnt <= ly_fore || self->sc->layers[ly_fore].python_persistent==NULL )
Py_RETURN_NONE;
    Py_INCREF( (PyObject *) (self->sc->layers[ly_fore].python_persistent) );
return( self->sc->layers[ly_fore].python_persistent );
}

static int PyFF_Glyph_set_persistent(PyFF_Glyph *self,PyObject *value, void *UNUSED(closure)) {
    if ( self->sc->layer_cnt <= ly_fore ) return -1;
    PyObject *old = self->sc->layers[ly_fore].python_persistent;

    /* I'd rather not store None, because C routines don't understand it */
    /*  and they occasionally need to know whether there is something real */
    /*  in this field. */
    if ( value==Py_None )
	value = NULL;
    Py_XINCREF(value);
    self->sc->layers[ly_fore].python_persistent = value;
    Py_XDECREF(old);
return( 0 );
}

static PyObject *PyFF_Glyph_get_activeLayer(PyFF_Glyph *self, void *UNUSED(closure)) {
return( Py_BuildValue("i", self->layer ));
}

static int PyFF_Glyph_set_activeLayer(PyFF_Glyph *self,PyObject *value, void *UNUSED(closure)) {
    int layer;

    if ( PyLong_Check(value) )
	layer = PyLong_AsLong(value);
    else if ( PyUnicode_Check(value)) {
	const char *name = PyUnicode_AsUTF8(value);
	if (name == NULL) {
	    return -1;
	}
	layer = SFFindLayerIndexByName(self->sc->parent,name);
	if ( layer<0 )
return( -1 );
    } else {
        return -1;
    }
    if ( layer<0 || layer>=self->sc->layer_cnt ) {
	PyErr_Format(PyExc_ValueError, "Layer is out of range" );
return( -1 );
    }
    self->layer = layer;
return( 0 );
}

static PyObject *PyFF_Glyph_get_glyphname(PyFF_Glyph *self, void *UNUSED(closure)) {

return( Py_BuildValue("s", self->sc->name ));
}

static int PyFF_Glyph_set_glyphname(PyFF_Glyph *self,PyObject *value, void *UNUSED(closure)) {
    FontViewBase *fvs;
    const char *str = PyUnicode_AsUTF8(value);
    if (str == NULL) {
        return -1;
    }

    SFGlyphRenameFixup(self->sc->parent,self->sc->name,str,false);
    self->sc->namechanged = self->sc->changed = true;
    free( self->sc->name );
    self->sc->name = copy(str);
    GlyphHashFree(self->sc->parent);
    SCRefreshTitles(self->sc);
    for ( fvs=self->sc->parent->fv; fvs!=NULL; fvs=fvs->nextsame ) {
	/* Postscript encodings are by name, others are by codepoint */
	if ( fvs->map->enc->psnames!=NULL && fvs->map->enc!=&custom ) {
	    fvs->map->enc = &custom;
	    FVSetTitle(fvs);
	}
    }
return( 0 );
}

static PyObject *PyFF_Glyph_get_encoding(PyFF_Glyph *self, void *UNUSED(closure)) {
    SplineChar *sc = self->sc;
    EncMap *map = sc->parent->fv->map;

return( Py_BuildValue("i", map->backmap[sc->orig_pos] ));
}

static PyObject *PyFF_Glyph_get_codepoint(PyFF_Glyph *self, void *UNUSED(closure)) {
    if ( self->sc==NULL || self->sc->unicodeenc < 0 )
Py_RETURN_NONE;
    return PyUnicode_FromFormat("U+%04X", self->sc->unicodeenc);
}

static PyObject *PyFF_Glyph_get_unicode(PyFF_Glyph *self, void *UNUSED(closure)) {

return( Py_BuildValue("i", self->sc->unicodeenc ));
}

static int PyFF_Glyph_set_unicode(PyFF_Glyph *self,PyObject *value, void *UNUSED(closure)) {
    FontViewBase *fvs;
    int uenc;

    uenc = PyLong_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );
    self->sc->unicodeenc = uenc;
    SCRefreshTitles(self->sc);
    for ( fvs=self->sc->parent->fv; fvs!=NULL; fvs=fvs->nextsame ) {
	/* Postscript encodings are by name, others are by codepoint */
	if ( fvs->map->enc->psnames==NULL && fvs->map->enc!=&custom ) {
	    fvs->map->enc = &custom;
	    FVSetTitle(fvs);
	}
    }
return( 0 );
}

static PyObject *PyFF_Glyph_get_altuni(PyFF_Glyph *self, void *UNUSED(closure)) {
    int cnt;
    struct altuni *au;
    PyObject *ret;

    for ( cnt=0, au = self->sc->altuni; au!=NULL; au=au->next, ++cnt );
    if ( cnt==0 )
Py_RETURN_NONE;

    ret = PyTuple_New(cnt);
    for ( cnt=0, au = self->sc->altuni; au!=NULL; au=au->next, ++cnt ) {
	PyTuple_SET_ITEM(ret,cnt,Py_BuildValue("(iii)", au->unienc,
		au->vs, au->fid));
    }
return( ret );
}

static int PyFF_Glyph_set_altuni(PyFF_Glyph *self,PyObject *value, void *UNUSED(closure)) {
    int cnt, i;
    struct altuni *head, *last=NULL, *cur;
    int uni, vs, fid;
    PyObject *obj;
    FontViewBase *fvs;

    if ( value == Py_None )
	head = NULL;
    else if ( !PySequence_Check(value)) {
	PyErr_Format(PyExc_TypeError, "Value must be a tuple of alternate unicode values");
return( -1 );
    } else {
	cnt = PySequence_Size(value);
	for ( i=0; i<cnt; ++i ) {
	    obj = PySequence_GetItem(value,i);
	    uni = 0; vs = -1; fid = 0;
	    if ( PyLong_Check(obj))
		uni = PyLong_AsLong(obj);
	    else if ( !PyArg_ParseTuple(obj,"i|ii", &uni, &vs, &fid))
return( -1 );
	    cur = chunkalloc(sizeof(struct altuni));
	    if ( vs==0 ) vs=-1;		/* convention used in charinfo */
	    cur->unienc = uni; cur->vs = vs; cur->fid = fid;
	    if ( last == NULL )
		head = cur;
	    else
		last->next = cur;
	    last = cur;
	}
    }

    AltUniFree(self->sc->altuni);
    self->sc->altuni = head;

    for ( fvs=self->sc->parent->fv; fvs!=NULL; fvs=fvs->nextsame ) {
	fvs->map->enc = &custom;
	FVSetTitle(fvs);
    }

    return( 0 );
}

static PyObject *PyFF_Glyph_get_changed(PyFF_Glyph *self, void *UNUSED(closure)) {

return( Py_BuildValue("i", self->sc->changed ));
}

static int PyFF_Glyph_set_changed(PyFF_Glyph *self,PyObject *value, void *UNUSED(closure)) {
    int uenc;

    uenc = PyLong_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );
    self->sc->changed = uenc;
return( 0 );
}

static PyObject *PyFF_Glyph_get_texheight(PyFF_Glyph *self, void *UNUSED(closure)) {

return( Py_BuildValue("i", self->sc->tex_height ));
}

static int PyFF_Glyph_set_texheight(PyFF_Glyph *self,PyObject *value, void *UNUSED(closure)) {
    int val;

    val = PyLong_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );
    self->sc->tex_height = val;
return( 0 );
}

static PyObject *PyFF_Glyph_get_texdepth(PyFF_Glyph *self, void *UNUSED(closure)) {

return( Py_BuildValue("i", self->sc->tex_depth ));
}

static int PyFF_Glyph_set_texdepth(PyFF_Glyph *self,PyObject *value, void *UNUSED(closure)) {
    int val;

    val = PyLong_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );
    self->sc->tex_depth = val;
return( 0 );
}

static PyObject *PyFF_Glyph_get_italiccorrection(PyFF_Glyph *self, void *UNUSED(closure)) {

return( Py_BuildValue("i", self->sc->italic_correction ));
}

static int PyFF_Glyph_set_italiccorrection(PyFF_Glyph *self,PyObject *value, void *UNUSED(closure)) {
    int val;

    val = PyLong_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );
    self->sc->italic_correction = val;
return( 0 );
}

static PyObject *PyFF_Glyph_get_topaccent(PyFF_Glyph *self, void *UNUSED(closure)) {

return( Py_BuildValue("i", self->sc->top_accent_horiz ));
}

static int PyFF_Glyph_set_topaccent(PyFF_Glyph *self,PyObject *value, void *UNUSED(closure)) {
    int val;

    val = PyLong_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );
    self->sc->top_accent_horiz = val;
return( 0 );
}

static PyObject *PyFF_Glyph_get_isextendedshape(PyFF_Glyph *self, void *UNUSED(closure)) {
    PyObject *ret;

    ret = self->sc->is_extended_shape ? Py_True : Py_False;
    Py_INCREF( ret );
return( ret );
}

static int PyFF_Glyph_set_isextendedshape(PyFF_Glyph *self,PyObject *value, void *UNUSED(closure)) {
    int val;

    val = PyLong_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );
    self->sc->is_extended_shape = val!=0;
return( 0 );
}


static PyObject *PyFF_Glyph_get_unlinkRmOvrlpSave(PyFF_Glyph *self, void *UNUSED(closure)) {

return( Py_BuildValue("i", self->sc->unlink_rm_ovrlp_save_undo ));
}

static int PyFF_Glyph_set_unlinkRmOvrlpSave(PyFF_Glyph *self,PyObject *value, void *UNUSED(closure)) {
    int val;

    val = PyLong_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );
    self->sc->unlink_rm_ovrlp_save_undo = val;
return( 0 );
}

static PyObject *PyFF_Glyph_get_originalgid(PyFF_Glyph *self, void *UNUSED(closure)) {

return( Py_BuildValue("i", self->sc->orig_pos ));
}

static PyObject *PyFF_Glyph_get_width(PyFF_Glyph *self, void *UNUSED(closure)) {

return( Py_BuildValue("i", self->sc->width ));
}

static int PyFF_Glyph_set_width(PyFF_Glyph *self,PyObject *value, void *UNUSED(closure)) {
    int val;

    val = PyLong_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );
    SCSynchronizeWidth(self->sc,val,self->sc->width,NULL);
    SCCharChangedUpdate(self->sc,ly_none);
return( 0 );
}

static PyObject *PyFF_Glyph_get_lsb(PyFF_Glyph *self, void *UNUSED(closure)) {
    DBounds b;

    SplineCharFindBounds(self->sc,&b);

return( Py_BuildValue("d", b.minx ));
}

static int PyFF_Glyph_set_lsb(PyFF_Glyph *self,PyObject *value, void *UNUSED(closure)) {
    int val;
    real trans[6];
    DBounds b;
    SplineChar *sc = self->sc;

    val = PyLong_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );
    SplineCharFindBounds(sc,&b);

    memset(trans,0,sizeof(trans));
    trans[0] = trans[3] = 1.0;
    trans[4] = val - b.minx;
    if ( trans[4]!=0 )
	FVTrans(sc->parent->fv,sc,trans,NULL,fvt_alllayers);
return( 0 );
}

static PyObject *PyFF_Glyph_get_rsb(PyFF_Glyph *self, void *UNUSED(closure)) {
    DBounds b;

    SplineCharFindBounds(self->sc,&b);

return( Py_BuildValue("d", self->sc->width - b.maxx ));
}

static int PyFF_Glyph_set_rsb(PyFF_Glyph *self,PyObject *value, void *UNUSED(closure)) {
    int val;
    DBounds b;

    val = PyLong_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );

    SplineCharFindBounds(self->sc,&b);
    SCSynchronizeWidth(self->sc,rint( val+b.maxx ),self->sc->width,NULL);
    SCCharChangedUpdate(self->sc,ly_none);
return( 0 );
}

static PyObject *PyFF_Glyph_get_vwidth(PyFF_Glyph *self, void *UNUSED(closure)) {

return( Py_BuildValue("i", self->sc->vwidth ));
}

static int PyFF_Glyph_set_vwidth(PyFF_Glyph *self,PyObject *value, void *UNUSED(closure)) {
    int val;

    val = PyLong_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );
    self->sc->vwidth = val;
return( 0 );
}

static PyObject *PyFF_Glyph_get_manualhints(PyFF_Glyph *self, void *UNUSED(closure)) {

return( Py_BuildValue("i", self->sc->manualhints ));
}

static PyObject *PyFF_Glyph_get_lcarets(PyFF_Glyph *self, void *UNUSED(closure)) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;
    int cnt=0, i;
    PST *pst, *lcar = NULL;
    PyObject *tuple;

    for ( pst = sc->possub; pst!=NULL; pst=pst->next ) {
       if ( pst->type==pst_lcaret ) {
           lcar = pst;
           cnt = lcar->u.lcaret.cnt;
    break;
       }
    }
    tuple = PyTuple_New(cnt);

    if ( lcar != NULL ) {
       for ( i=0; i<cnt; ++i ) {
           PyTuple_SetItem( tuple,i,Py_BuildValue("i",lcar->u.lcaret.carets[i]) );
       }
    }
return( tuple );
}

static int PyFF_Glyph_set_lcarets(PyFF_Glyph *self,PyObject *value, void *UNUSED(closure)) {
    SplineChar *sc = self->sc;
    int i, cnt, lig_comp_max = 0, lc;
    char *pt;
    int16_t *carets;
    PST *pst, *lcar = NULL;

    cnt = PySequence_Size(value);
    if ( cnt==-1 )
return( -1 );

    if ( cnt > 0 )
       carets = malloc( cnt*sizeof(int16_t) );
    for ( i=0; i<cnt; ++i ) {
       carets[i] = PyLong_AsLong( PySequence_GetItem(value,i) );
       if ( PyErr_Occurred()) {
           free(carets);
return( -1 );
       }
    }

    for ( pst = sc->possub; pst!=NULL; pst=pst->next ) {
       if ( pst->type==pst_lcaret ) {
           lcar = pst;
           free( lcar->u.lcaret.carets );
       } else if ( pst->type==pst_ligature ) {
           for ( lc=0, pt=pst->u.lig.components; *pt; ++pt )
               if ( *pt==' ' ) ++lc;
           if ( lc>lig_comp_max )
               lig_comp_max = lc;
       }
    }

    if ( lcar == NULL && cnt > 0 ) {
       lcar = chunkalloc(sizeof(PST));
       lcar->type = pst_lcaret;
       lcar->next = sc->possub;
       sc->possub = lcar;
    }
    if ( lcar != NULL ) {
       lcar->u.lcaret.cnt = cnt;
       lcar->u.lcaret.carets = cnt > 0 ? carets : NULL;
       sc->lig_caret_cnt_fixed = ( cnt != lig_comp_max ) ? true : false;
    }
return( 0 );
}

static int PyFF_Glyph_set_manualhints(PyFF_Glyph *self,PyObject *value, void *UNUSED(closure)) {
    int val;

    val = PyLong_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );
    self->sc->manualhints = (val!=0);
return( 0 );
}

static PyObject *PyFF_Glyph_get_font(PyFF_Glyph *self, void *UNUSED(closure)) {
    PyFF_Font *font = PyFF_FontForSC(self->sc);
    if ( font==NULL )
Py_RETURN_NONE;
    Py_INCREF(font);
    return (PyObject*)font;
}

static PyObject *PyFF_Glyph_get_references(PyFF_Glyph *self, void *closure) {
return( PyFF_Glyph_get_layer_references(self,self->layer));
}

static int PyFF_Glyph_set_references(PyFF_Glyph *self,PyObject *value, void *closure) {
return( PyFF_Glyph_set_layer_references(self,value,self->layer));
}

static PyObject *PyFF_Glyph_get_layerrefs(PyFF_Glyph *self, void *UNUSED(closure)) {
    PyFF_RefArray *layerrefs;

    if ( self->refs!=NULL )
Py_RETURN( self->refs );
    layerrefs = (PyFF_RefArray *) PyObject_New(PyFF_RefArray, &PyFF_RefArrayType);
    if (layerrefs == NULL)
return NULL;
    layerrefs->sc = self->sc;
    self->refs = layerrefs;
Py_RETURN( self->refs );
}

static PyObject *PyFF_Glyph_get_ttfinstrs(PyFF_Glyph *self, void *UNUSED(closure)) {
    SplineChar *sc = self->sc;
    PyObject *binstr;

    binstr = PyBytes_FromStringAndSize((char *) sc->ttf_instrs,sc->ttf_instrs_len);
return( binstr );
}

static int PyFF_Glyph_set_ttfinstrs(PyFF_Glyph *self,PyObject *value, void *UNUSED(closure)) {
    int i, cnt;
    SplineChar *sc = self->sc;

    if ( !PySequence_Check(value)) {
	PyErr_Format(PyExc_TypeError, "Value must be a sequence of integers");
return( -1 );
    }
    cnt = PySequence_Size(value);
    free(sc->ttf_instrs); sc->ttf_instrs = NULL; sc->ttf_instrs_len = cnt;
    SCNumberPoints(sc,self->layer);	/* If the point numbering is wrong then we'll just throw away the instructions when we notice it */
    sc->instructions_out_of_date = false;
    if ( cnt==0 )
return( 0 );
    if ( PyBytes_Check(value)) {
	char *space; Py_ssize_t len;
	PyBytes_AsStringAndSize(value,&space,&len);
	sc->ttf_instrs = calloc(len,sizeof(uint8_t));
	sc->ttf_instrs_len = len;
	memcpy(sc->ttf_instrs,space,len);
    } else {
	sc->ttf_instrs = calloc(cnt,sizeof(uint8_t));
	for ( i=0; i<cnt; ++i ) {
	    int val = PyLong_AsLong(PySequence_GetItem(value,i));
	    if ( PyErr_Occurred()!=NULL )
return( -1 );
	    sc->ttf_instrs[i] = val;
	}
    }
return( 0 );
}

struct flaglist glyphclasses[] = {
    { "automatic", 0 },
    { "noclass", 1 },
    { "baseglyph", 2 },
    { "baseligature", 3 },
    { "mark", 4 },
    { "component", 5 },
    FLAGLIST_EMPTY /* Sentinel */
};

static PyObject *PyFF_Glyph_get_glyphclass(PyFF_Glyph *self, void *UNUSED(closure)) {
return( Py_BuildValue("s", glyphclasses[self->sc->glyph_class].name ));
}

static int PyFF_Glyph_set_glyphclass(PyFF_Glyph *self,PyObject *value, void *UNUSED(closure)) {
    int gc;
    const char *glyphclassname = PyUnicode_AsUTF8(value);

    if (glyphclassname == NULL) {
        return -1;
    }
    gc = FlagsFromString(glyphclassname,glyphclasses,"glyph class");
    if ( gc==FLAG_UNKNOWN )
        return( -1 );
    self->sc->glyph_class = gc;
    return( 0 );
}

static PyObject *PyFF_Glyph_get_layers(PyFF_Glyph *self, void *UNUSED(closure)) {
    PyFF_LayerArray *layers;

    if ( self->layers!=NULL )
Py_RETURN( self->layers );
    layers = (PyFF_LayerArray *) PyObject_New(PyFF_LayerArray, &PyFF_LayerArrayType);
    if (layers == NULL)
return NULL;
    layers->sc = self->sc;
    self->layers = layers;
Py_RETURN( self->layers );
}

static PyObject *PyFF_Glyph_get_layer_cnt(PyFF_Glyph *self, void *UNUSED(closure)) {

return( Py_BuildValue("i", self->sc->layer_cnt ));
}

static PyObject *PyFF_Glyph_get_hints(StemInfo *head) {
    StemInfo *h;
    int cnt;
    PyObject *tuple;

    for ( h=head, cnt=0; h!=NULL; h=h->next, ++cnt );
    tuple = PyTuple_New(cnt);
    for ( h=head, cnt=0; h!=NULL; h=h->next, ++cnt ) {
	double start, width;
	start = h->start; width = h->width;
	if ( h->ghost && width>0 ) {
	    start += width;
	    width = -width;
	}
	PyTuple_SetItem(tuple,cnt,Py_BuildValue("(dd)", start, width ));
    }

return( tuple );
}

static int PyFF_Glyph_set_hints(PyFF_Glyph *self,int is_v,PyObject *value) {
    SplineChar *sc = self->sc;
    StemInfo *head=NULL, *tail=NULL, *cur;
    int i, cnt;
    double start, width;
    StemInfo **_head = is_v ? &sc->vstem : &sc->hstem;
    int layer;

    cnt = PySequence_Size(value);
    if ( cnt==-1 )
return( -1 );
    for ( i=0; i<cnt; ++i ) {
	if ( !PyArg_ParseTuple(PySequence_GetItem(value,i),"dd", &start, &width ))
return( -1 );
	cur = chunkalloc(sizeof(StemInfo));
	if ( width==-20 || width==-21 )
	    cur->ghost = true;
	if ( width<0 ) {
	    start += width;
	    width = -width;
	}
	cur->start = start;
	cur->width = width;
	if ( tail==NULL )
	    head = cur;
	else
	    tail->next = cur;
	tail = cur;
    }

    StemInfosFree(*_head);
    for ( layer=ly_fore; layer<sc->layer_cnt; ++layer )
	SCClearHintMasks(sc,layer,true);
    *_head = HintCleanup(head,true,1);
    if ( is_v ) {
        sc->vstem = head;
        SCGuessHintInstancesList( sc,self->layer,NULL,sc->vstem,NULL,false,false );
	sc->vconflicts = StemListAnyConflicts(sc->vstem);
    } else {
        sc->hstem = head;
        SCGuessHintInstancesList( sc,self->layer,sc->hstem,NULL,NULL,false,false );
	sc->hconflicts = StemListAnyConflicts(sc->hstem);
    }

    SCCharChangedUpdate(sc,ly_none);
return( 0 );
}

static PyObject *PyFF_Glyph_get_dhints(PyFF_Glyph *self, void *UNUSED(closure)) {
    DStemInfo *ds, *dn;
    int cnt;
    PyObject *tuple;

    ds = self->sc->dstem;
    for ( dn=ds, cnt=0; dn!=NULL; dn=dn->next, ++cnt );
    tuple = PyTuple_New(cnt);
    for ( dn=ds, cnt=0; dn!=NULL; dn=dn->next, ++cnt ) {
	BasePoint left, right, unit;
	left = dn->left; right = dn->right;
        unit = dn->unit;
	PyTuple_SetItem(tuple,cnt,Py_BuildValue("((dd)(dd)(dd))",
            left.x,left.y,right.x,right.y,unit.x,unit.y ));
    }

return( tuple );
}

static int PyFF_Glyph_set_dhints(PyFF_Glyph *self,PyObject *value, void *UNUSED(closure)) {
    SplineChar *sc = self->sc;
    DStemInfo *head=NULL, *cur;
    int i, cnt;
    double len, width;
    double lx, ly, rx, ry, ux, uy;
    DStemInfo **_head = &sc->dstem;

    cnt = PySequence_Size(value);
    if ( cnt==-1 )
return( -1 );
    for ( i=0; i<cnt; ++i ) {
	if ( !PyArg_ParseTuple(PySequence_GetItem(value,i),"(dd)(dd)(dd)",
            &lx,&ly,&rx,&ry,&ux,&uy ))
return( -1 );
        if ( ux == 0 && uy == 0 ) {
            LogError(_("Invalid unit vector has been specified. The hint is ignored.\n"));
    continue;
        } else if ( ux == 0 ) {
            LogError(_("Use the \'vhint\' property to specify a vertical hint.\n"));
    continue;
        } else if ( uy == 0 ) {
            LogError(_("Use the \'hhint\' property to specify a horizontal hint.\n"));
    continue;
        }
	cur = chunkalloc(sizeof(DStemInfo));
        len = sqrt( pow( ux,2 ) + pow( uy,2 ));
        ux /= len; uy /= len;
        if ( ux < 0 ) {
            cur->unit.x = -ux; cur->unit.y = -uy;
        } else {
            cur->unit.x = ux; cur->unit.y = uy;
        }
        width = ( rx - lx )*cur->unit.y - ( ry - ly )*cur->unit.x;
        if ( width < 0 ) {
	    cur->left.x = lx; cur->left.y = ly;
	    cur->right.x = rx; cur->right.y = ry;
        } else {
	    cur->left.x = rx; cur->left.y = ry;
	    cur->right.x = lx; cur->right.y = ly;
        }
        MergeDStemInfo( sc->parent,&head,cur );
    }

    DStemInfosFree(*_head);
    sc->dstem = head;
    SCGuessHintInstancesList( sc,self->layer,NULL,NULL,sc->dstem,false,true );
    SCCharChangedUpdate(sc,ly_none);
return( 0 );
}

static PyObject *PyFF_Glyph_get_hhints(PyFF_Glyph *self, void *UNUSED(closure)) {
return( PyFF_Glyph_get_hints(self->sc->hstem));
}

static int PyFF_Glyph_set_hhints(PyFF_Glyph *self,PyObject *value, void *UNUSED(closure)) {
return( PyFF_Glyph_set_hints(self,false,value));
}

static PyObject *PyFF_Glyph_get_vhints(PyFF_Glyph *self, void *UNUSED(closure)) {
return( PyFF_Glyph_get_hints(self->sc->vstem));
}

static int PyFF_Glyph_set_vhints(PyFF_Glyph *self,PyObject *value, void *UNUSED(closure)) {
return( PyFF_Glyph_set_hints(self,true,value));
}

static PyObject *PyFF_Glyph_get_user_decomp(PyFF_Glyph *self, void *UNUSED(closure)) {
    PyObject *ret;

    if ( self->sc->user_decomp==NULL ) {
        return( Py_BuildValue("s", "" ));
    }
    else {
        char* out = u2utf8_copy(self->sc->user_decomp);
        ret = PyUnicode_DecodeUTF8(out, strlen(out), NULL);
        free(out);
        return ret;
    }
}

static int PyFF_Glyph_set_user_decomp(PyFF_Glyph *self,PyObject *value, void *UNUSED(closure)) {
    const char *temp;
    unichar_t *udbuf;

    if (value == Py_None) {
        if (self->sc->user_decomp != NULL) { free(self->sc->user_decomp); }
        self->sc->user_decomp = NULL;
        return 0;
    } else if ((temp = PyUnicode_AsUTF8(value)) == NULL) {
        return -1;
    }

    udbuf = utf82u_copy(temp);

    if ( udbuf==NULL ) return -1;

    if (udbuf[0] == '\0') {
        if (self->sc->user_decomp != NULL) { free(self->sc->user_decomp); }
        self->sc->user_decomp = NULL;
        return 0;
    }

    if (self->sc->user_decomp != NULL) { free(self->sc->user_decomp); }
    self->sc->user_decomp = udbuf;

    return 0;
}

static PyObject *PyFF_Glyph_get_comment(PyFF_Glyph *self, void *UNUSED(closure)) {
    if ( self->sc->comment==NULL )
return( Py_BuildValue("s", "" ));
    else
return( PyUnicode_DecodeUTF8(self->sc->comment,strlen(self->sc->comment),NULL));
}

static int PyFF_Glyph_set_comment(PyFF_Glyph *self,PyObject *value, void *UNUSED(closure)) {
    char *newv = copy(PyUnicode_AsUTF8(value));
    if ( newv==NULL )
return( -1 );
    free(self->sc->comment);
    self->sc->comment = newv;
return( 0 );
}

/* Anchor type: see 'enum anchor_type' in splinefont.h */
static struct flaglist ap_types[] = {
    { "mark", at_mark },
    { "base", at_basechar },
    { "ligature", at_baselig },
    { "basemark", at_basemark },
    { "entry", at_centry },
    { "exit", at_cexit },
    { "baselig", at_baselig },
    FLAGLIST_EMPTY /* Sentinel */
};

static PyObject *_PyFF_Glyph_get_anchorPoints(PyFF_Glyph *self,int withsel) {
    SplineChar *sc = self->sc;
    AnchorPoint *ap;
    int cnt;
    PyObject *tuple;

    for ( ap=sc->anchor, cnt=0; ap!=NULL; ap=ap->next, ++cnt );
    tuple = PyTuple_New(cnt);
    for ( ap=sc->anchor, cnt=0; ap!=NULL; ap=ap->next, ++cnt ) {
	if ( !withsel ) {
	    if ( ap->type == at_baselig )
		PyTuple_SetItem(tuple,cnt,Py_BuildValue("(ssddi)", ap->anchor->name,
			ap_types[ap->type].name, ap->me.x, ap->me.y, ap->lig_index ));
	    else
		PyTuple_SetItem(tuple,cnt,Py_BuildValue("(ssdd)", ap->anchor->name,
			ap_types[ap->type].name, ap->me.x, ap->me.y ));
	} else {
	    if ( ap->type == at_baselig )
		PyTuple_SetItem(tuple,cnt,Py_BuildValue("(ssddOi)", ap->anchor->name,
			ap_types[ap->type].name, ap->me.x, ap->me.y,
			ap->selected?Py_True:Py_False, ap->lig_index ));
	    else
		PyTuple_SetItem(tuple,cnt,Py_BuildValue("(ssddO)", ap->anchor->name,
			ap_types[ap->type].name, ap->me.x, ap->me.y,
			ap->selected?Py_True:Py_False));
	}
    }

return( tuple );
}

static PyObject *PyFF_Glyph_get_anchorPoints(PyFF_Glyph *self, void *UNUSED(closure)) {
return( _PyFF_Glyph_get_anchorPoints(self,false));
}

static PyObject *PyFF_Glyph_get_anchorPointsWithSel(PyFF_Glyph *self, void *UNUSED(closure)) {
return( _PyFF_Glyph_get_anchorPoints(self,true));
}

static AnchorPoint *APFromTuple(SplineChar *sc,PyObject *tuple) {
    char *ac_name, *type;
    double x, y;
    int lig_index=-1, selected=false;
    AnchorPoint *ap;
    AnchorClass *ac;
    SplineFont *sf = sc->parent;
    int aptype;
    int len, hassel=false;

    len = PyTuple_Size(tuple);
    if ( len==5 ) {
	PyObject *o = PyTuple_GetItem(tuple,4);
	if ( PyBool_Check(o))
	    hassel = true;
    } else if ( len==6 )
	hassel = true;

    if ( hassel ) {
	if ( !PyArg_ParseTuple(tuple, "ssdd|ii", &ac_name, &type, &x, &y,
		&selected, &lig_index ))
return( NULL );
    } else {
	if ( !PyArg_ParseTuple(tuple, "ssdd|i", &ac_name, &type, &x, &y, &lig_index ))
return( NULL );
    }
    aptype = FlagsFromString(type,ap_types,"anchor type");
    if ( aptype==FLAG_UNKNOWN )
return( NULL );
    for ( ac=sf->anchor; ac!=NULL; ac=ac->next ) {
	if ( strcmp(ac->name,ac_name)==0 )
    break;
    }
    if ( ac==NULL ) {
        ac = chunkalloc(sizeof(AnchorClass));
        ac->name = copy( ac_name );
        ac->subtable = NULL;
        ac->type = act_unknown;
        ac->next = sf->anchor;
        sf->anchor = ac;
    }
    switch ( ac->type ) {
      case act_mark:
	if ( aptype!=at_mark && aptype!=at_basechar ) {
	    PyErr_Format(PyExc_TypeError, "You must specify either a mark or a base anchor type for this anchor class, %s.", ac_name );
return( NULL );
	}
      break;
      case act_mkmk:
	if ( aptype!=at_mark && aptype!=at_basemark ) {
	    PyErr_Format(PyExc_TypeError, "You must specify either a mark or a base mark anchor type for this anchor class, %s.", ac_name );
return( NULL );
	}
      break;
      case act_mklg:
	if ( aptype!=at_mark && aptype!=at_baselig ) {
	    PyErr_Format(PyExc_TypeError, "You must specify either a mark or a ligature anchor type for this anchor class, %s.", ac_name );
return( NULL );
	}
      break;
      case act_curs:
	if ( aptype!=at_centry && aptype!=at_cexit ) {
	    PyErr_Format(PyExc_TypeError, "You must specify either an entry or an exit anchor type for this anchor class, %s.", ac_name );
return( NULL );
	}
      default:
      break;
      /* leave act_unknown to allow anything until we resolve it by associating with a subtable */
    }
    if ( lig_index==-1 && aptype==at_baselig ) {
	PyErr_Format(PyExc_TypeError, "You must specify a ligature index for a ligature anchor point" );
return( NULL );
    } else if ( lig_index!=-1 && aptype!=at_baselig ) {
	PyErr_Format(PyExc_TypeError, "You may not specify a ligature index for a non-ligature anchor point" );
return( NULL );
    }

    ap = chunkalloc(sizeof(AnchorPoint));
    ap->anchor = ac;
    ap->type = aptype;
    ap->me.x = x;
    ap->me.y = y;
    ap->selected = selected;
    if ( aptype==at_baselig )
	ap->lig_index = lig_index;
return( ap );
}

static int PyFF_Glyph_set_anchorPoints(PyFF_Glyph *self,PyObject *value, void *UNUSED(closure)) {
    AnchorPoint *aphead=NULL, *aplast = NULL, *ap;
    int i;
    SplineChar *sc = self->sc;

    if ( !PySequence_Check(value)) {
	PyErr_Format(PyExc_TypeError, "Expected a tuple of anchor points" );
return( -1 );
    }

    for ( i=0; i<PySequence_Size(value); ++i ) {
	ap = APFromTuple(sc,PySequence_GetItem(value,i));
	if ( ap==NULL )
return( -1 );
	if ( aphead==NULL )
	    aphead = ap;
	else
	    aplast->next = ap;
	aplast = ap;
    }
    AnchorPointsFree(sc->anchor);
    sc->anchor = aphead;
    SCCharChangedUpdate(sc,ly_none);
return( 0 );
}

static PyObject *PyFF_Glyph_get_color(PyFF_Glyph *self, void *UNUSED(closure)) {
return( Py_BuildValue("i", self->sc->color ));
}

static int PyFF_Glyph_set_color(PyFF_Glyph *self,PyObject *value, void *UNUSED(closure)) {
    int val;
    SplineChar *sc = self->sc;

    val = PyLong_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );
    sc->color = val;
    SCCharChangedUpdate(sc,ly_none);
return( 0 );
}

static PyObject *PyFF_Glyph_get_script(PyFF_Glyph *self, void *UNUSED(closure)) {
    uint32_t script = SCScriptFromUnicode(self->sc);

return( TagToPythonString(script, false ));
}

static PyObject *PyFF_Glyph_get_validation_state(PyFF_Glyph *self, void *UNUSED(closure)) {
return( Py_BuildValue("i", self->sc->layers[self->layer].validation_state ));
}

static char *GlyphListToStr(PyObject *value) {
    char *str, *pt;
    int i,cnt,len;

    if ( !PySequence_Check(value)) {
	PyErr_Format(PyExc_TypeError, "Value must be a sequence" );
return( NULL );
    }
    if ( PyUnicode_Check(value) ) {
        str = copy(PyUnicode_AsUTF8(value));
    } else {
	cnt = PySequence_Size(value);
	len = 0;
	for ( i=0; i<cnt; ++i ) {
	    PyObject *obj = PySequence_GetItem(value,i);
	    if ( PyType_IsSubtype(&PyFF_GlyphType, Py_TYPE(obj)) ) {
		PyFF_Glyph *g = (PyFF_Glyph *) obj;
		len += strlen(g->sc->name) + 1;
	    } else {
		Py_XDECREF(obj);
		PyErr_Format(PyExc_TypeError, "Must be a sequence of glyphs" );
return( NULL );
	    }
	    Py_DECREF(obj);
	}
	pt = str = malloc(len+1);
	for ( i=0; i<cnt; ++i ) {
	    PyObject *obj = PySequence_GetItem(value,i);
	    PyFF_Glyph *g = (PyFF_Glyph *) obj;
	    strcpy(pt,g->sc->name);
	    pt += strlen(g->sc->name);
	    strcpy(pt," ");
	    pt += 1;
	    Py_DECREF(obj);
	}
	if ( pt>str ) pt[-1] = '\0';
    }
return( str );
}

static void FreeGVPartsList(struct gv_part* parts, int cnt) {
    if (parts == NULL) {
        return;
    }
    while (--cnt >= 0) {
        free(parts[cnt].component);
    }
    free(parts);
}

static void FreeGVParts(struct glyphvariants *gv) {
    if (gv != NULL && gv->part_cnt != 0) {
        FreeGVPartsList(gv->parts, gv->part_cnt);
        gv->part_cnt = 0;
        gv->parts = NULL;
    }
}

static PyObject *BuildComponentTuple(struct glyphvariants *gv) {
    PyObject *tuple;
    int i;

    if ( gv->part_cnt==0 )
Py_RETURN_NONE;
    tuple = PyTuple_New(gv->part_cnt);
    for ( i=0; i<gv->part_cnt; ++i ) {
	PyTuple_SetItem(tuple,i,Py_BuildValue("(siiii)",
		gv->parts[i].component,
		gv->parts[i].is_extender,
		gv->parts[i].startConnectorLength,
		gv->parts[i].endConnectorLength,
		gv->parts[i].fullAdvance));
    }
return( tuple );
}

static struct gv_part *ParseComponentTuple(PyObject *tuple,int *_cnt) {
    int i, cnt;
    struct gv_part *parts;
    PyObject *seq = PySequence_Fast(tuple, "Must be a tuple or a list");

    if (seq == NULL) {
        return NULL;
    }

    *_cnt = cnt = PySequence_Fast_GET_SIZE(seq);
    parts = calloc(cnt+1,sizeof(struct gv_part));
    for ( i=0; i<cnt; ++i ) {
	PyObject *obj = PySequence_Fast_GET_ITEM(seq, i);
	int extender=0, start=0, end=0, full=0;
	char *glyphName;
	if ( PyType_IsSubtype(&PyFF_GlyphType, Py_TYPE(obj)) ) {
	    parts[i].component = copy( ((PyFF_Glyph *) obj)->sc->name );
	} else if ( PyUnicode_Check(obj) ) {
	    char* temp = copy(PyUnicode_AsUTF8(obj));
	    if (temp == NULL) {
	        FreeGVPartsList(parts, i);
	        Py_DECREF(seq);
	        return NULL;
	    }
	    parts[i].component = temp;
	} else if ( PyTuple_Check(obj) && PyTuple_Size(obj)>0 &&
		    PyType_IsSubtype(&PyFF_GlyphType, Py_TYPE(PyTuple_GetItem(obj,0))) ) {
	    PyObject *g;
	    if ( !PyArg_ParseTuple(obj,"O|iiii", &g, &extender, &start, &end, &full )) {
		Py_DECREF(seq);
		FreeGVPartsList(parts, i);
return( NULL );
	    }
	    parts[i].component = copy(((PyFF_Glyph *) g)->sc->name);
	} else if ( PyArg_ParseTuple(obj,"s|iiii", &glyphName,
		&extender, &start, &end, &full )) {
	    parts[i].component = copy(glyphName);
	} else {
        Py_DECREF(seq);
        FreeGVPartsList(parts, i);
return( NULL );
        }
	parts[i].is_extender = extender;
	parts[i].startConnectorLength = start;
	parts[i].endConnectorLength = end;
	parts[i].fullAdvance = full;
    }
    Py_DECREF(seq);
return( parts );
}

static PyObject *PyFF_Glyph_get_horizontalCIC(PyFF_Glyph *self, void *UNUSED(closure)) {
    if ( self->sc->horiz_variants==NULL )
return( Py_BuildValue("i", 0));

return( Py_BuildValue("i", self->sc->horiz_variants->italic_correction ));
}

static int PyFF_Glyph_set_horizontalCIC(PyFF_Glyph *self,PyObject *value, void *UNUSED(closure)) {
    int val;

    val = PyLong_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );
    if ( self->sc->horiz_variants == NULL )
	self->sc->horiz_variants = chunkalloc(sizeof(struct glyphvariants));
    self->sc->horiz_variants->italic_correction = val;
return( 0 );
}

static PyObject *PyFF_Glyph_get_verticalCIC(PyFF_Glyph *self, void *UNUSED(closure)) {
    if ( self->sc->vert_variants==NULL )
return( Py_BuildValue("i", 0));

return( Py_BuildValue("i", self->sc->vert_variants->italic_correction ));
}

static int PyFF_Glyph_set_verticalCIC(PyFF_Glyph *self,PyObject *value, void *UNUSED(closure)) {
    int val;

    val = PyLong_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );
    if ( self->sc->vert_variants == NULL )
	self->sc->vert_variants = chunkalloc(sizeof(struct glyphvariants));
    self->sc->vert_variants->italic_correction = val;
return( 0 );
}

static PyObject *PyFF_Glyph_get_verticalVariants(PyFF_Glyph *self, void *UNUSED(closure)) {
    if ( self->sc->vert_variants==NULL || self->sc->vert_variants->variants==NULL )
Py_RETURN_NONE;

return( Py_BuildValue("s", self->sc->vert_variants->variants ));
}

static int PyFF_Glyph_set_verticalVariants(PyFF_Glyph *self,PyObject *value, void *UNUSED(closure)) {
    if ( value == Py_None ) {
	if ( self->sc->vert_variants!=NULL ) {
	    free(self->sc->vert_variants->variants);
	    self->sc->vert_variants->variants=NULL;
	}
    } else {
	char *str = GlyphListToStr(value);
	if ( str==NULL )
return( -1 );
	if ( self->sc->vert_variants == NULL )
	    self->sc->vert_variants = chunkalloc(sizeof(struct glyphvariants));
	self->sc->vert_variants->variants = str;
    }
return( 0 );
}

static PyObject *PyFF_Glyph_get_horizontalVariants(PyFF_Glyph *self, void *UNUSED(closure)) {
    if ( self->sc->horiz_variants==NULL || self->sc->horiz_variants->variants==NULL )
Py_RETURN_NONE;

return( Py_BuildValue("s", self->sc->horiz_variants->variants ));
}

static int PyFF_Glyph_set_horizontalVariants(PyFF_Glyph *self,PyObject *value, void *UNUSED(closure)) {
    if ( value == Py_None ) {
	if ( self->sc->horiz_variants!=NULL ) {
	    free(self->sc->horiz_variants->variants);
	    self->sc->horiz_variants->variants=NULL;
	}
    } else {
	char *str = GlyphListToStr(value);
	if ( str==NULL )
return( -1 );
	if ( self->sc->horiz_variants == NULL )
	    self->sc->horiz_variants = chunkalloc(sizeof(struct glyphvariants));
	self->sc->horiz_variants->variants = str;
    }
return( 0 );
}

static PyObject *PyFF_Glyph_get_horizontalComponents(PyFF_Glyph *self, void *UNUSED(closure)) {
    if ( self->sc->horiz_variants==0 || self->sc->horiz_variants->part_cnt==0 )
Py_RETURN_NONE;

return( BuildComponentTuple(self->sc->horiz_variants ));
}

static int PyFF_Glyph_set_horizontalComponents(PyFF_Glyph *self,PyObject *value, void *UNUSED(closure)) {
    int cnt;
    struct gv_part *parts;

    if ( value == Py_None ) {
	if ( self->sc->horiz_variants!=NULL ) {
	    FreeGVParts(self->sc->horiz_variants);
	}
    } else {
	parts = ParseComponentTuple(value,&cnt);
	if ( parts==NULL )
return( -1 );
	FreeGVParts(self->sc->horiz_variants);
	if ( self->sc->horiz_variants == NULL )
	    self->sc->horiz_variants = chunkalloc(sizeof(struct glyphvariants));
	self->sc->horiz_variants->part_cnt = cnt;
	self->sc->horiz_variants->parts = parts;
    }
return( 0 );
}

static PyObject *PyFF_Glyph_get_verticalComponents(PyFF_Glyph *self, void *UNUSED(closure)) {
    if ( self->sc->vert_variants==0 || self->sc->vert_variants->part_cnt==0 )
Py_RETURN_NONE;

return( BuildComponentTuple(self->sc->vert_variants ));
}

static int PyFF_Glyph_set_verticalComponents(PyFF_Glyph *self,PyObject *value, void *UNUSED(closure)) {
    int cnt;
    struct gv_part *parts;

    if ( value == Py_None ) {
	if ( self->sc->vert_variants!=NULL ) {
	    FreeGVParts(self->sc->vert_variants);
	}
    } else {
	parts = ParseComponentTuple(value,&cnt);
	if ( parts==NULL )
return( -1 );
	FreeGVParts(self->sc->vert_variants);
	if ( self->sc->vert_variants == NULL )
	    self->sc->vert_variants = chunkalloc(sizeof(struct glyphvariants));
	self->sc->vert_variants->part_cnt = cnt;
	self->sc->vert_variants->parts = parts;
    }
return( 0 );
}

static PyObject *PyFF_Glyph_get_mathKern(PyFF_Glyph *self, void *UNUSED(closure)) {
    PyFF_MathKern *mk;

    if ( self->mk!=NULL )
Py_RETURN( self->mk );
    mk = (PyFF_MathKern *) PyObject_New(PyFF_MathKern, &PyFF_MathKernType);
    if (mk == NULL)
return NULL;
    mk->sc = self->sc;
    self->mk = mk;
Py_RETURN( self->mk );
}

static PyGetSetDef PyFF_Glyph_getset[] = {
    {(char *)"userdata",
     (getter)PyFF_Glyph_get_temporary, (setter)PyFF_Glyph_set_temporary,
     (char *)"arbitrary (non persistent) user data (deprecated name for temporary)", NULL},
    {(char *)"temporary",
     (getter)PyFF_Glyph_get_temporary, (setter)PyFF_Glyph_set_temporary,
     (char *)"arbitrary (non persistent) user data", NULL},
    {(char *)"persistant",		/* I documented this member with the wrong spelling... so support it */
     (getter)PyFF_Glyph_get_persistent, (setter)PyFF_Glyph_set_persistent,
     (char *)"arbitrary persistent user data", NULL},
    {(char *)"persistent",
     (getter)PyFF_Glyph_get_persistent, (setter)PyFF_Glyph_set_persistent,
     (char *)"arbitrary persistent user data", NULL},
    {(char *)"activeLayer",
     (getter)PyFF_Glyph_get_activeLayer, (setter)PyFF_Glyph_set_activeLayer,
     (char *)"The layer in the glyph which is currently active", NULL},
    {(char *)"anchorPoints",
     (getter)PyFF_Glyph_get_anchorPoints, (setter)PyFF_Glyph_set_anchorPoints,
     (char *)"a tuple of all anchor points in the glyph", NULL},
/* There is no set_anchorPointsWithSel because we don't need it. We set the selection if we find it */
    {(char *)"anchorPointsWithSel",
     (getter)PyFF_Glyph_get_anchorPointsWithSel, (setter)PyFF_Glyph_set_anchorPoints,
     (char *)"a tuple of all anchor points in the glyph (with selection indication)", NULL},
    {(char *)"glyphname",
     (getter)PyFF_Glyph_get_glyphname, (setter)PyFF_Glyph_set_glyphname,
     (char *)"glyph name", NULL},
    {(char *)"codepoint",
     (getter)PyFF_Glyph_get_codepoint, NULL,
     (char *)"Unicode code point for this glyph in U+XXXX format, or None (readonly)", NULL},
    {(char *)"unicode",
     (getter)PyFF_Glyph_get_unicode, (setter)PyFF_Glyph_set_unicode,
     (char *)"Unicode code point for this glyph, or -1", NULL},
    {(char *)"altuni",
     (getter)PyFF_Glyph_get_altuni, (setter)PyFF_Glyph_set_altuni,
     (char *)"Alternate unicode encodings (and variation selectors) for this glyph", NULL},
    {(char *)"encoding",
     (getter)PyFF_Glyph_get_encoding, NULL,
     (char *)"Returns the glyph's encoding in the current font (readonly)", NULL},
    {(char *)"foreground",
     (getter)PyFF_Glyph_get_a_layer, (setter)PyFF_Glyph_set_a_layer,
     (char *)"Returns the foreground layer of the glyph", (void *)ly_fore},
    {(char *)"background",
     (getter)PyFF_Glyph_get_a_layer, (setter)PyFF_Glyph_set_a_layer,
     (char *)"Returns the background layer of the glyph", (void *)ly_back},
    {(char *)"layers",
     (getter)PyFF_Glyph_get_layers, NULL,
     (char *)"Returns an array of layers", NULL},
    {(char *)"references",
     (getter)PyFF_Glyph_get_references, (setter)PyFF_Glyph_set_references,
     (char *)"A tuple of all references in the glyph", NULL},
    {(char *)"layerrefs",
     (getter)PyFF_Glyph_get_layerrefs, NULL,
     (char *)"Returns an array of layer references", NULL},
    {(char *)"layer_cnt",
     (getter)PyFF_Glyph_get_layer_cnt, NULL,
     (char *)"Returns the number of layers in the glyph", NULL},
    {(char *)"color",
     (getter)PyFF_Glyph_get_color, (setter)PyFF_Glyph_set_color,
     (char *)"Glyph color", NULL},
    {(char *)"comment",
     (getter)PyFF_Glyph_get_comment, (setter)PyFF_Glyph_set_comment,
     (char *)"Glyph comment", NULL},
    {(char *)"user_decomp",
     (getter)PyFF_Glyph_get_user_decomp, (setter)PyFF_Glyph_set_user_decomp,
     (char *)"Glyph user decompositon", NULL},
    {(char *)"glyphclass",
     (getter)PyFF_Glyph_get_glyphclass, (setter)PyFF_Glyph_set_glyphclass,
     (char *)"glyph class", NULL},
    {(char *)"italicCorrection",
     (getter)PyFF_Glyph_get_italiccorrection, (setter)PyFF_Glyph_set_italiccorrection,
     (char *)"Math & TeX italic correction", NULL},
    {(char *)"isExtendedShape",
     (getter)PyFF_Glyph_get_isextendedshape, (setter)PyFF_Glyph_set_isextendedshape,
     (char *)"Math \"is extended shape\" field", NULL},
    {(char *)"script",
     (getter)PyFF_Glyph_get_script, NULL,
     (char *)"The OpenType script containing this glyph (readonly)", NULL},
    {(char *)"texheight",
     (getter)PyFF_Glyph_get_texheight, (setter)PyFF_Glyph_set_texheight,
     (char *)"TeX glyph height", NULL},
    {(char *)"texdepth",
     (getter)PyFF_Glyph_get_texdepth, (setter)PyFF_Glyph_set_texdepth,
     (char *)"TeX glyph depth", NULL},
    {(char *)"topaccent",
     (getter)PyFF_Glyph_get_topaccent, (setter)PyFF_Glyph_set_topaccent,
     (char *)"Math top accent horizontal position", NULL},
    {(char *)"ttinstrs",
     (getter)PyFF_Glyph_get_ttfinstrs, (setter)PyFF_Glyph_set_ttfinstrs,
     (char *)"TrueType Instructions for this glyph", NULL},
    {(char *)"unlinkRmOvrlpSave",
     (getter)PyFF_Glyph_get_unlinkRmOvrlpSave, (setter)PyFF_Glyph_set_unlinkRmOvrlpSave,
     (char *)"A flag which indicates that before the glyph is saved ff should unlink its references and run remove overlap on it.", NULL},
    {(char *)"changed",
     (getter)PyFF_Glyph_get_changed, (setter)PyFF_Glyph_set_changed,
     (char *)"Flag indicating whether this glyph has changed", NULL},
    {(char *)"originalgid",
     (getter)PyFF_Glyph_get_originalgid, NULL,
     (char *)"Original GID (readonly)", NULL},
    {(char *)"width",
     (getter)PyFF_Glyph_get_width, (setter)PyFF_Glyph_set_width,
     (char *)"Glyph's advance width", NULL},
    {(char *)"left_side_bearing",
     (getter)PyFF_Glyph_get_lsb, (setter)PyFF_Glyph_set_lsb,
     (char *)"Glyph's left side bearing", NULL},
    {(char *)"right_side_bearing",
     (getter)PyFF_Glyph_get_rsb, (setter)PyFF_Glyph_set_rsb,
     (char *)"Glyph's right side bearing", NULL},
    {(char *)"vwidth",
     (getter)PyFF_Glyph_get_vwidth, (setter)PyFF_Glyph_set_vwidth,
     (char *)"Glyph's vertical advance width", NULL},
    {(char *)"font",
     (getter)PyFF_Glyph_get_font, NULL,
     (char *)"Font containing the glyph (readonly)", NULL},
    {(char *)"hhints",
     (getter)PyFF_Glyph_get_hhints, (setter)PyFF_Glyph_set_hhints,
     (char *)"The horizontal hints of the glyph as a tuple, one entry per hint. Each hint is itself a tuple containing the start location and width of the hint", NULL},
    {(char *)"vhints",
     (getter)PyFF_Glyph_get_vhints, (setter)PyFF_Glyph_set_vhints,
     (char *)"The vertical hints of the glyph as a tuple, one entry per hint. Each hint is itself a tuple containing the start location and width of the hint", NULL},
    {(char *)"dhints",
     (getter)PyFF_Glyph_get_dhints, (setter)PyFF_Glyph_set_dhints,
     (char *)"The diagonal hints of the glyph as a tuple, one entry per hint. Each hint is itself a tuple containing three pairs of coordinates, specifying a point on the left edge, a point on the right edge and a unit vector for this hint.", NULL},
    {(char *)"manualHints",
     (getter)PyFF_Glyph_get_manualhints, (setter)PyFF_Glyph_set_manualhints,
     (char *)"The hints have been set manually, and the glyph should not be autohinted by default", NULL },
    {(char *)"lcarets",
     (getter)PyFF_Glyph_get_lcarets, (setter)PyFF_Glyph_set_lcarets,
     (char *)"The ligature caret locations, defined for this glyph, as a tuple.", NULL},
    {(char *)"validation_state",
     (getter)PyFF_Glyph_get_validation_state, NULL,
     (char *)"glyph's validation state (readonly)", NULL},
    {(char *)"horizontalVariants",
     (getter)PyFF_Glyph_get_horizontalVariants, (setter)PyFF_Glyph_set_horizontalVariants,
     (char *)"glyph's horizontal variants (for math typesetting) as a string of glyph names", NULL},
    {(char *)"verticalVariants",
     (getter)PyFF_Glyph_get_verticalVariants, (setter)PyFF_Glyph_set_verticalVariants,
     (char *)"glyph's vertical variants (for math typesetting) as a string of glyph names", NULL},
    {(char *)"horizontalComponents",
     (getter)PyFF_Glyph_get_horizontalComponents, (setter)PyFF_Glyph_set_horizontalComponents,
     (char *)"A way of build very large versions of the glyph (for math typesetting) out of lots of smaller glyphs.", NULL},
    {(char *)"verticalComponents",
     (getter)PyFF_Glyph_get_verticalComponents, (setter)PyFF_Glyph_set_verticalComponents,
     (char *)"A way of build very large versions of the glyph (for math typesetting) out of lots of smaller glyphs.", NULL},
    {(char *)"horizontalComponentItalicCorrection",
     (getter)PyFF_Glyph_get_horizontalCIC, (setter)PyFF_Glyph_set_horizontalCIC,
     (char *)"The italic correction for any composite glyph made with the horizontalComponents.", NULL},
    {(char *)"verticalComponentItalicCorrection",
     (getter)PyFF_Glyph_get_verticalCIC, (setter)PyFF_Glyph_set_verticalCIC,
     (char *)"The italic correction for any composite glyph made with the verticalComponents.", NULL},
    {(char *)"mathKern",
     (getter)PyFF_Glyph_get_mathKern, NULL,
     (char *)"math kerning information for the glyph.", NULL},
    PYGETSETDEF_EMPTY  /* Sentinel */
};

/* ************************************************************************** */
/*  Glyph Methods  */
/* ************************************************************************** */

static PyObject *PyFFGlyph_Build(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;
    int layer = ((PyFF_Glyph *) self)->layer;
    int accent_hint = false;
    PyObject *accent_hint_pyo = NULL;

    if ( PyArg_ParseTuple(args, "|O", &accent_hint_pyo) ) {
        if (accent_hint_pyo == Py_True) {
            accent_hint = true;
        }
    }

    if ( SFIsSomethingBuildable(sc->parent,sc,layer,false) )
	SCBuildComposit(sc->parent,sc,layer,NULL,true,accent_hint);

Py_RETURN( self );
}

static const char *appendaccent_keywords[] = { "name", "unicode", "pos", NULL };

static PyObject *PyFFGlyph_appendAccent(PyObject *self, PyObject *args, PyObject *keywds) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;
    int layer = ((PyFF_Glyph *) self)->layer;
    int pos = FF_UNICODE_NOPOSDATAGIVEN; /* unicode char pos info, see #define for (uint32_t)(utype2[]) */
    int uni=-1;				/* unicode char value */
    char *name = NULL;			/* unicode char name */
    int ret;

    if ( !PyArg_ParseTupleAndKeywords(args,keywds,"|sii",(char **)appendaccent_keywords,
	    &name, &uni, &pos))
return( NULL );
    if ( name==NULL && uni==-1 ) {
	PyErr_Format(PyExc_ValueError, "You must specify either a name of a unicode code point");
return( NULL );
    }
    ret = SCAppendAccent(sc,layer,name,uni,(uint32_t)(pos));
    if ( ret==1 ) {
	PyErr_Format(PyExc_ValueError, "No base character reference found");
return( NULL );
    } else if ( ret==2 ) {
	PyErr_Format(PyExc_ValueError, "Could not find that accent");
return( NULL );
    }
    SCCharChangedUpdate(sc,layer);

Py_RETURN( self );
}

static PyObject *PyFFGlyph_useRefsMetrics(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;
    int layer = ((PyFF_Glyph *) self)->layer;
    RefChar *ref, *itwilldo;
    char *name=NULL;
    int setting=true;

    if ( !PyArg_ParseTuple(args,"s|i", &name, &setting ))
return( NULL );
    for ( ref = sc->layers[layer].refs; ref!=NULL; ref=ref->next )
	ref->use_my_metrics = 0;
    itwilldo = NULL;
    for ( ref = sc->layers[layer].refs; ref!=NULL; ref=ref->next )
	if ( strcmp(ref->sc->name,name)==0 ) {
	    if ( ref->transform[0]==1 && ref->transform[3]==1 &&
		    ref->transform[1]==0 && ref->transform[2]==0 &&
		    ref->transform[4]==0 && ref->transform[5]==0 )
    break;
	    else
		itwilldo = ref;
	}
    if ( ref==NULL )
	ref = itwilldo;
    if ( ref==NULL ) {
	PyErr_Format(PyExc_ValueError, "Could not find a reference named %s", name );
return( NULL );
    }
    if ( setting ) {
	ref->use_my_metrics = true;
	sc->width = ref->sc->width;
    }

Py_RETURN( self );
}

static PyObject *PyFFGlyph_canonicalContours(PyObject *self, PyObject *UNUSED(args)) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;

    CanonicalContours(sc,((PyFF_Glyph *) self)->layer);

Py_RETURN( self );
}

static PyObject *PyFFGlyph_canonicalStart(PyObject *self, PyObject *UNUSED(args)) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;

    SPLsStartToLeftmost(sc,((PyFF_Glyph *) self)->layer);

Py_RETURN( self );
}

/* Char weight/Embolden types: see 'enum embolden_type' in baseviews.h */
static struct flaglist cw_types[] = {
    { "lcg", embolden_lcg },
    { "cjk", embolden_cjk },
    { "auto", embolden_auto },
    { "custom", embolden_custom },
    { "LCG", embolden_lcg },
    { "CJK", embolden_cjk },
    FLAGLIST_EMPTY /* Sentinel */
};

/* Counter types: see 'enum counter_type' in baseviews.h */
static struct flaglist co_types[] = {
    { "squish", ct_squish },
    { "retain", ct_retain },
    { "auto", ct_auto },
    FLAGLIST_EMPTY /* Sentinel */
};

static enum embolden_type CW_ParseArgs(SplineFont *sf, struct lcg_zones *zones, PyObject *args) {
    enum embolden_type type;
    const char *type_name="auto", *counter_name = "auto";
    PyObject *zoneO=NULL;
    int just_top;

    memset(zones,0,sizeof(*zones));
    zones->serif_height = -1;
    zones->serif_fuzz = .9;

    if ( !PyArg_ParseTuple(args,"d|sddsiO",
	    &zones->stroke_width, &type_name,
	    &zones->serif_height, &zones->serif_fuzz,
	    &counter_name,
	    &zones->removeoverlap,
	    &zoneO ))
return( embolden_error );
    type = FlagsFromString(type_name,cw_types,"embolden type");
    if ( type==(uint32_t)FLAG_UNKNOWN )
return( embolden_error );
    zones->counter_type = FlagsFromString(counter_name,co_types,"counter type");
    if ( zones->counter_type==(uint32_t)FLAG_UNKNOWN )
return( embolden_error );

    just_top = true;
    if ( zoneO==NULL )
	zones->top_bound = sf->ascent/2;
    else if ( PyLong_Check(zoneO))
	zones->top_bound = PyLong_AsLong(zoneO);
    else if ( PyTuple_Check(zoneO)) {
	if ( !PyArg_ParseTuple(zoneO,"iiii",
		&zones->top_bound,&zones->top_zone,&zones->bottom_zone,&zones->bottom_bound))
return( embolden_error );
	just_top = false;
    }
    if ( just_top ) {
	/* Bottom defaults to 0, and the other two are between */
	zones->top_zone = 3*zones->top_bound/4;
	zones->bottom_zone = zones->top_bound/4;
    }

return( type );
}

static PyObject *PyFFGlyph_changeWeight(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;
    enum embolden_type type;
    struct lcg_zones zones;

    type = CW_ParseArgs(sc->parent,&zones,args);
    if ( type == embolden_error )
return( NULL );
    ScriptSCEmbolden(sc,((PyFF_Glyph *) self)->layer,type,&zones);

Py_RETURN( self );
}

static PyObject *PyFFGlyph_condenseExtend(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;
    struct counterinfo ci;

    memset(&ci,0,sizeof(ci));
    ci.sb_factor = ci.sb_add = -10000;
    ci.correct_italic = true;
    ci.layer = ((PyFF_Glyph *) self)->layer;

    if ( !PyArg_ParseTuple(args,"dd|ddi",&ci.c_factor, &ci.c_add, &ci.sb_factor, &ci.sb_add,
	    &ci.correct_italic ))
return( NULL );
    ci.c_factor *= 100;			/* UI uses a percent */
    if ( ci.sb_factor == -10000 )
	ci.sb_factor = ci.c_factor;
    if ( ci.sb_add == -10000 )
	ci.sb_add = ci.c_add;
    CI_Init(&ci,sc->parent);
    ScriptSCCondenseExtend(sc,&ci);

Py_RETURN( self );
}

static PyObject *PyFFGlyph_AddReference(PyObject *self, PyObject *args) {
    double m[6] = {1.0,0.0,0.0,1.0,0.0,0.0};
    real transform[6];
    char *str;
    SplineChar *sc = ((PyFF_Glyph *) self)->sc, *rsc;
    SplineFont *sf = sc->parent;
    int j, selected=false;

    if ( !PyArg_ParseTuple(args,"s|(dddddd)p",&str,
	    &m[0], &m[1], &m[2], &m[3], &m[4], &m[5], &selected) )
return( NULL );
    rsc = SFGetChar(sf,-1,str);
    if ( rsc==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No glyph named %s", str);
return( NULL );
    }
    for ( j=0; j<6; ++j )
	transform[j] = m[j];
    _SCAddRef(sc,rsc,((PyFF_Glyph *) self)->layer,transform,selected);
    SCCharChangedUpdate(sc,((PyFF_Glyph *) self)->layer);

Py_RETURN( self );
}

static PyObject *PyFFGlyph_addAnchorPoint(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;
    AnchorPoint *ap = APFromTuple(sc,args), *ap2;
    int done = false;

    if ( ap==NULL )
return( NULL );

    for ( ap2=sc->anchor; ap2!=NULL; ap2=ap2->next ) {
        switch ( ap2->anchor->type ) {
            default:
                if ( ap2->anchor->name==ap->anchor->name && ap2->type==ap->type ) {
                    ap->next = ap2->next;
                    *ap2 = *ap;
                    done = true;
		}
		break;
            case act_mklg:
                if ( ap2->anchor->name==ap->anchor->name && ap2->lig_index==ap->lig_index ) {
                    ap->next = ap2->next;
                    *ap2 = *ap;
                    done = true;
		}
		break;
            case act_mark:
                if ( ap2->anchor->name==ap->anchor->name ) {
                    ap->next = ap2->next;
                    *ap2 = *ap;
                    done = true;
		}
		break;
	}
    }

    if ( !done ) {
       ap->next = sc->anchor;
       sc->anchor = ap;
    }

    SCCharChangedUpdate(sc,((PyFF_Glyph *) self)->layer);

Py_RETURN( self );
}

static const char *glyphpen_keywords[] = { "replace", NULL };
static PyObject *PyFFGlyph_GlyphPen(PyObject *self, PyObject *args, PyObject *keywds) {
    int replace = true;
    PyObject *gp;

    if ( !PyArg_ParseTupleAndKeywords(args, keywds, "|i", (char **)glyphpen_keywords,
	    &replace ))
return( NULL );
    gp = PyFF_GlyphPenType.tp_alloc(&PyFF_GlyphPenType,0);
    ((PyFF_GlyphPen *) gp)->sc = ((PyFF_Glyph *) self)->sc;
    ((PyFF_GlyphPen *) gp)->layer = ((PyFF_Glyph *) self)->layer;
    ((PyFF_GlyphPen *) gp)->replace = replace;
    ((PyFF_GlyphPen *) gp)->ended = true;
    ((PyFF_GlyphPen *) gp)->changed = false;
    /* tp_alloc increments the reference count for us */
return( gp );
}

static PyObject *PyFFGlyph_draw(PyObject *self, PyObject *args) {
    PyObject *layer, *result, *pen;
    RefChar *ref;
    PyObject *tuple;

    if ( !PyArg_ParseTuple(args,"O", &pen ) )
return( NULL );

    layer = PyFF_Glyph_get_a_layer((PyFF_Glyph *) self,(void *)(size_t)((PyFF_Glyph *) self)->layer);
    result = PyFFLayer_draw( (PyFF_Layer *) layer,args);
    Py_XDECREF(layer);

    for ( ref = ((PyFF_Glyph *) self)->sc->layers[((PyFF_Glyph *) self)->layer].refs; ref!=NULL; ref=ref->next ) {
	tuple = Py_BuildValue("s(dddddd)", ref->sc->name,
		ref->transform[0], ref->transform[1], ref->transform[2],
		ref->transform[3], ref->transform[4], ref->transform[5]);
	do_pycall(pen,"addComponent",tuple);
    }
return( result );
}

static bool PyFFParse_genericGlyphChange(PyObject *args, PyObject *keywds,
                                         struct genericchange *genchange);

static PyObject *PyFFGlyph_genericGlyphChange(PyFF_Glyph *self, PyObject *args,
                                              PyObject *keywds) {
    SplineChar *sc = self->sc;
    struct genericchange genchange;
    struct smallcaps small;

    if ( self->layer < 0 )
	Py_RETURN( self );

    if ( !PyFFParse_genericGlyphChange(args, keywds, &genchange) )
	return NULL;

    SmallCapsFindConstants(&small,sc->parent,self->layer);
    genchange.small = &small;
    genchange.italic_angle = small.italic_angle;
    genchange.tan_ia = small.tan_ia;

    genchange.g.cnt = genchange.m.cnt+2;
    genchange.g.maps = malloc(genchange.g.cnt*sizeof(struct position_maps));

    if ( sc->layers[self->layer].splines!=NULL )
	ChangeGlyph(sc,sc,self->layer,&genchange);

    free(genchange.g.maps);
    free(genchange.m.maps);

    Py_RETURN( self );
}

static PyObject *PyFFGlyph_addHint(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;
    int layer = ((PyFF_Glyph *) self)->layer;
    int is_v;
    double start, width;
    StemInfo *h;

    if ( !PyArg_ParseTuple(args,"idd", &is_v, &start, &width ) )
return( NULL );

    h = chunkalloc(sizeof(StemInfo));
    if ( width==-20 || width==-21 )
	h->ghost = true;
    if ( width<0 ) {
	start += width;
	width = -width;
    }
    h->start = start;
    h->width = width;
    if ( is_v ) {
	SCGuessVHintInstancesAndAdd(sc,layer,h,0x80000000,0x80000000);
	h->next = sc->vstem;
	sc->vstem = HintCleanup(h,true,1);
	sc->vconflicts = StemListAnyConflicts(sc->vstem);
    } else {
	SCGuessHHintInstancesAndAdd(sc,layer,h,0x80000000,0x80000000);
	h->next = sc->hstem;
	sc->hstem = HintCleanup(h,true,1);
	sc->hconflicts = StemListAnyConflicts(sc->hstem);
    }
Py_RETURN( self );
}

static PyObject *PyFFGlyph_autoHint(PyObject *self, PyObject *UNUSED(args)) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;
    int layer = ((PyFF_Glyph *) self)->layer;

    SplineCharAutoHint(sc,layer,NULL);
    SCUpdateAll(sc);
Py_RETURN( self );
}

static PyObject *PyFFGlyph_autoInstr(PyObject *self, PyObject *UNUSED(args)) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;

    GlobalInstrCt gic;
    InitGlobalInstrCt(&gic,sc->parent,((PyFF_Glyph *) self)->layer,NULL);
    NowakowskiSCAutoInstr(&gic,sc);
    FreeGlobalInstrCt(&gic);
Py_RETURN( self );
}

static PyObject *PyFFGlyph_autoTrace(PyObject *self, PyObject *UNUSED(args)) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;
    int layer = ((PyFF_Glyph *) self)->layer;
    char **at_args;

    at_args = AutoTraceArgs(false);
    if ( at_args==(char **) -1 ) {
	PyErr_Format(PyExc_EnvironmentError, "Bad autotrace args" );
return(NULL);
    }
    _SCAutoTrace(sc, layer, at_args);
Py_RETURN( self );
}

static char *glyph_import_keywords[] = { "filename", "correctdir",
    "simplify", "handle_clip", "handle_eraser", "scale", "accuracy",
    "default_joinlimit", "usesystem", "asksystem", "dimensions", NULL };

/* Legacy PostScript importing flags */
static struct flaglist import_ps_flags[] = {
    { "toobigwarn", 0 },			/* Obsolete */
    { "removeoverlap", 0 },			/* Obsolete */
    { "handle_eraser", 1 },
    { "correctdir",  2 },
    FLAGLIST_EMPTY /* Sentinel */
};

static PyObject *PyFFGlyph_import(PyObject *self, PyObject *args,
                                  PyObject *keywds) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;
    ImportParams ip, *ipp;
    char *filename;
    char *locfilename = NULL, *pt;
    PyObject *flags=NULL;
    int psflags, use_system = false, ask_system = false;
    bigreal jl_tmp = -1;

    InitImportParams(&ip);

    if ( !PyArg_ParseTupleAndKeywords(args, keywds,
                "s|$pppppddppp", glyph_import_keywords, &filename,
                &ip.correct_direction, &ip.simplify, &ip.clip, &ip.erasers,
                &ip.scale, &ip.accuracy_target, &jl_tmp, &use_system,
		&ask_system, &ip.dimensions) ) {
	PyErr_Clear();
	if ( !PyArg_ParseTuple(args,"s|O",&filename, &flags) )
	    return NULL;
	psflags = FlagsFromTuple(flags, import_ps_flags,
	                         "PostScript import flag");
	if ( psflags==FLAG_UNKNOWN )
	    return NULL;
	if ( psflags & 1 )
	    ip.erasers = true;
	if ( psflags & 2 )
	    ip.correct_direction = true;
    }
    locfilename = utf82def_copy(filename);

    /* Check if the file exists and is readable */
    if ( access(locfilename,R_OK)!=0 ) {
	PyErr_SetFromErrnoWithFilename(PyExc_IOError,locfilename);
	free(locfilename);
	return NULL;
    }

    if ( use_system || ask_system ) {
	ipp = ImportParamsState();
	if ( ask_system )
	    ImportParamsDlg(ipp);
    } else
	ipp = &ip;

    pt = strrchr(locfilename,'.');
    if ( pt==NULL ) pt=locfilename;

    if ( strcasecmp(pt,".eps")==0 || strcasecmp(pt,".ps")==0 || strcasecmp(pt,".art")==0 ) {
	SCImportPS(sc,((PyFF_Glyph *) self)->layer,locfilename,false,ipp);
    }
    else if ( strcasecmp(pt,".svg")==0 ) {
	SCImportSVG(sc,((PyFF_Glyph *) self)->layer,locfilename,NULL,0,false,ipp);
    }
    else if ( strcasecmp(pt,".glif")==0 ) {
	SCImportGlif(sc,((PyFF_Glyph *) self)->layer,locfilename,NULL,0,false,ipp);
    }
    else if ( strcasecmp(pt,".plate")==0 ) {
	FILE *plate = fopen(locfilename,"r");
	if ( plate==NULL ) {
	    PyErr_SetFromErrnoWithFilename(PyExc_IOError,locfilename);
	    free(locfilename);
return( NULL );
	}
	else {
	    SCImportPlateFile(sc,((PyFF_Glyph *) self)->layer,plate,false,ipp);
	    fclose(plate);
	}
    } /* else if ( strcasecmp(pt,".fig")==0 )*/
    else {
	GImage *image = GImageRead(locfilename);
	int ly = ((PyFF_Glyph *) self)->layer;
	if ( image==NULL ) {
	    PyErr_Format(PyExc_EnvironmentError, "Could not load image file \"%s\"", locfilename );
	    free(locfilename);
return(NULL);
	}
	if ( !sc->layers[ly].background )
	    ly = ly_back;
	SCAddScaleImage(sc,image,false,ly,ipp);
    }
    free( locfilename );
Py_RETURN( self );
}

static char *glyph_export_keywords[] = { "filename", "layer", "pixelsize",
    "bitdepth", "usetransform", "usesystem", "asksystem", NULL };

static PyObject *PyFFGlyph_export(PyObject *self, PyObject *args,
                                  PyObject *keywds) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;
    char *filename;
    char *locfilename = NULL;
    char *pt;
    int pixels=100, bits=8;
    int format=-1, use_system=false, ask_system=false;
    FILE *file;
    int layer = ((PyFF_Glyph *) self)->layer;
    PyObject *layerobj = NULL, *obj1=NULL, *obj2=NULL;
    ExportParams ep, *epp;

    InitExportParams(&ep);

    if ( !PyArg_ParseTupleAndKeywords(args, keywds, "s|$Oiippp",
                                      glyph_export_keywords, &filename,
                                      &layerobj, &pixels, &bits,
                                      &ep.use_transform, &use_system,
                                      &ask_system) ) {
	PyErr_Clear();
	if ( !PyArg_ParseTuple(args,"s|OO",&filename,&obj1,&obj2) )
	    return NULL;
    }
    locfilename = utf82def_copy(filename);

    if ( use_system || ask_system ) {
	epp = ExportParamsState();
	if ( ask_system )
	    ExportParamsDlg(epp);
    } else
	epp = &ep;

    pt = strrchr(locfilename,'.');
    if ( pt==NULL ) pt=locfilename;
    if ( strcasecmp(pt,".xbm")==0 ) {
	format=0; bits=1;
    } else if ( strcasecmp(pt,".bmp")==0 )
	format=1;
    else if ( strcasecmp(pt,".png")==0 )
	format=2;

    if ( format!=-1 ) {
	if ( obj1!=NULL ) {
	    if ( PyLong_Check(obj1) ) {
		pixels = PyLong_AsLong(obj1);
	    } else {
		free(locfilename);
		return NULL;
	    }
	}
	if ( obj2!=NULL ) {
	    if ( PyLong_Check(obj2) ) {
		bits = PyLong_AsLong(obj2);
	    } else {
		free(locfilename);
		return NULL;
	    }
	}
	if ( !ExportImage(locfilename,sc,layer,format,pixels,bits)) {
	    PyErr_Format(PyExc_EnvironmentError, "Could not create image file \"%s\"", locfilename );
	    free(locfilename);
return( NULL );
	}
    } else {
	file = fopen( locfilename,"wb");
	if ( file==NULL ) {
	    PyErr_SetFromErrnoWithFilename(PyExc_IOError,locfilename);
	    free(locfilename);
return( NULL );
	}

	if ( obj1!=NULL ) {
	    layer = LayerArgToLayer(sc->parent, obj1);
	    if ( layer==ly_none ) {
		free(locfilename);
		fclose(file);
		return NULL;
	    }
	}
	if ( layer<0 || layer>=sc->layer_cnt ) {
	    PyErr_Format(PyExc_ValueError, "Layer is out of range" );
	    free(locfilename);
	    fclose(file);
return( NULL );
	}

	if ( strcasecmp(pt,".eps")==0 || strcasecmp(pt,".ps")==0 || strcasecmp(pt,".art")==0 )
	    _ExportEPS(file,sc,layer,true);
	else if ( strcasecmp(pt,".pdf")==0 )
	    _ExportPDF(file,sc,layer);
	else if ( strcasecmp(pt,".svg")==0 )
	    _ExportSVG(file,sc,layer,epp);
	else if ( strcasecmp(pt,".glif")==0 )
	    _ExportGlif(file,sc,layer,3);
	else if ( strcasecmp(pt,".glif2")==0 )
	    _ExportGlif(file,sc,layer,2);
	else if ( strcasecmp(pt,".glif3")==0 )
	    _ExportGlif(file,sc,layer,3);
	else if ( strcasecmp(pt,".plate")==0 )
	    _ExportPlate(file,sc,layer);
	/* else if ( strcasecmp(pt,".fig")==0 )*/
	else {
	    PyErr_Format(PyExc_TypeError, "Unknown extension to export: %s", pt );
	    free( locfilename );
	    fclose(file);
return( NULL );
	}
	fclose(file);
    }
    free( locfilename );
Py_RETURN( self );
}

static PyObject *PyFFGlyph_unlinkRef(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;
    int layer = ((PyFF_Glyph *) self)->layer;
    char *refname=NULL;
    RefChar *ref;
    int any = false;

    if ( !PyArg_ParseTuple(args,"|s",&refname) )
return( NULL );
    for ( ref = sc->layers[layer].refs; ref!=NULL; ref=ref->next ) {
	if ( refname==NULL || strcmp(ref->sc->name,refname)==0 ) {
	    SCRefToSplines(sc,ref,layer);
	    any = true;
	}
    }

    if ( !any && refname!=NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No reference named %s found in glyph %s", refname, sc->name );
return( NULL );
    }
    if ( any )
	SCCharChangedUpdate(sc,((PyFF_Glyph *) self)->layer);
Py_RETURN( self );
}

static PyObject *PyFFGlyph_unlinkThisGlyph(PyObject *self, PyObject *UNUSED(args)) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;
    int layer = ((PyFF_Glyph *) self)->layer;

    UnlinkThisReference(NULL,sc,layer);
Py_RETURN( self );
}

static PyObject *TupleOfGlyphNames(char *str,int extras) {
    int cnt;
    char *pt, *start;
    PyObject *tuple;
    int ch;

    for ( pt=str; *pt==' '; ++pt );
    if ( *pt=='\0' )
return( PyTuple_New(extras));

    for ( cnt=1; *pt; ++pt ) {
	if ( *pt==' ' ) {
	    ++cnt;
	    while ( pt[1]==' ' ) ++pt;
	}
    }
    tuple = PyTuple_New(extras+cnt);
    for ( pt=str, cnt=0; *pt; ) {
	while ( *pt==' ' ) ++pt;
	if ( *pt=='\0' )
    break;
	start = pt;
	while ( *pt!=' ' && *pt!='\0' ) ++pt;
	ch = *pt; *pt = '\0';
	PyTuple_SetItem(tuple,extras+cnt,PyUnicode_FromString(start));
	*pt = ch;
	++cnt;
    }
return( tuple );
}

static char *GlyphNamesFromTuple(PyObject *glyphs) {
    int cnt, len, deltalen;
    const char *str;
    char *ret, *pt;
    int i;

    /* if called with a string, assume already in output format and return */
    if ( PyUnicode_Check(glyphs)) {
        if ((str = PyUnicode_AsUTF8(glyphs)) == NULL) {
            return NULL;
        } else if (str[0] == '\0') {
	    PyErr_Format(PyExc_TypeError,"Glyph name strings may not be empty");
            return( NULL );
        }
        return copy(str);
    }

    if ( !PyTuple_Check(glyphs) && !PyList_Check(glyphs)) {
        PyErr_Format(PyExc_TypeError,"Expected tuple of glyph names");
        return(NULL );
    }
    cnt = PySequence_Size(glyphs);
    len = 0;
    for ( i=0; i<cnt; ++i ) {
	PyObject *aglyph = PySequence_GetItem(glyphs,i);
	if ( PyType_IsSubtype(&PyFF_GlyphType, Py_TYPE(aglyph)) ) {
	    SplineChar *sc = ((PyFF_Glyph *) aglyph)->sc;
	    deltalen = strlen(sc->name);
            Py_DECREF(aglyph);
	} else if ( PyUnicode_Check(aglyph)) {
            str = PyUnicode_AsUTF8(aglyph);
            if (str == NULL) {
                Py_DECREF(aglyph);
                return NULL;
            }
            deltalen = strlen(str);
            Py_DECREF(aglyph);
        } else {
            Py_XDECREF(aglyph);
	    PyErr_Format(PyExc_TypeError,"Expected tuple of glyph names");
            return( NULL );
	}
        if ( deltalen==0 ) {
	    PyErr_Format(PyExc_TypeError,"Glyph name strings may not be empty");
            return( NULL );
        }
	len += deltalen+1;
    }

    ret = pt = malloc(len+1);
    for ( i=0; i<cnt; ++i ) {
	PyObject *aglyph = PySequence_GetItem(glyphs,i);
	if ( PyType_IsSubtype(&PyFF_GlyphType, Py_TYPE(aglyph)) ) {
	    SplineChar *sc = ((PyFF_Glyph *) aglyph)->sc;
	    str = sc->name;
	} else {
        str = PyUnicode_AsUTF8(aglyph);
    }
    if (str == NULL) {
        Py_DECREF(aglyph);
        free(ret);
        return NULL;
    }
	strcpy(pt,str);
	Py_DECREF(aglyph);
	pt += strlen(pt);
	*pt++ = ' ';
    }
    if ( pt!=ret )
	--pt;
    *pt = '\0';
    return( ret );
}

static char **GlyphNameArrayFromTuple(PyObject *glyphs) {
    int cnt;
    char *str, **ret;
    int i;

    if ( PyUnicode_Check(glyphs) || !PySequence_Check(glyphs) ) {
	PyErr_Format(PyExc_TypeError,"Expected tuple of glyph names");
return(NULL );
    }
    cnt = PySequence_Size(glyphs);
    ret = malloc((cnt+1)*sizeof(char *));
    for ( i=0; i<cnt; ++i ) {
	PyObject *aglyph = PySequence_GetItem(glyphs,i);
	if ( PyType_IsSubtype(&PyFF_GlyphType, Py_TYPE(aglyph)) ) {
	    SplineChar *sc = ((PyFF_Glyph *) aglyph)->sc;
	    str = sc->name;
	} else
	    str = PyBytes_AsString(aglyph);
	if ( str==NULL ) {
	    PyErr_Format(PyExc_TypeError,"Expected tuple of glyph names");
	    free(ret);
return( NULL );
	}
	ret[i] = copy(str);
    }
return( ret );
}

static SplineChar **GlyphsFromTuple(SplineFont *sf, PyObject *glyphs) {
    int cnt;
    char *str, *pt, *start, ch;
    int i;
    SplineChar **ret, *sc;

    if ( glyphs==NULL ) {
	PyErr_Format(PyExc_TypeError,"Unspecified argument." );
return( NULL );
    }
    if ( PyUnicode_Check(glyphs)) {
	/* A string of glyph names */
	if ((str = copy(PyUnicode_AsUTF8(glyphs))) == NULL) {
	    return NULL;
	}
	cnt = 0;
	for ( pt=str; *pt==' '; ++pt );
	while ( *pt!='\0' ) {
	    while ( *pt!=' ' && *pt!='\0' ) ++pt;
	    ++cnt;
	    while ( *pt==' ' ) ++pt;
	}
	if ( cnt==0 ) {
        free(str);
return( calloc(1,sizeof(SplineChar *)));
	}

	ret = malloc((cnt+1)*sizeof(SplineChar *));
	cnt = 0;
	for ( pt=str; *pt==' '; ++pt );
	while ( *pt!='\0' ) {
	    start = pt;
	    while ( *pt!=' ' && *pt!='\0' ) ++pt;
	    ch = *pt; *pt = '\0';
	    sc = SFGetChar(sf,-1,start);
	    if ( sc==NULL ) {
		PyErr_Format(PyExc_TypeError,"String, %s, is not the name of a glyph in the expected font.", start );
                free(str);
                free(ret);
return( NULL );
	    }
	    *pt = ch;
	    ret[cnt++] = sc;
	    while ( *pt==' ' ) ++pt;
	}
	ret[cnt] = NULL;
    free(str);
return( ret );
    }

    /* A tuple or list of glyphs of glyph names */
    if ( !PySequence_Check(glyphs) ) {
	PyErr_Format(PyExc_TypeError,"Expected tuple of glyph names");
return(NULL );
    }
    cnt = PySequence_Size(glyphs);
    ret = malloc((cnt+1)*sizeof(SplineChar *));
    for ( i=0; i<cnt; ++i ) {
	PyObject *aglyph = PySequence_GetItem(glyphs,i);
	if ( PyType_IsSubtype(&PyFF_GlyphType, Py_TYPE(aglyph)) ) {
	    ret[i] = ((PyFF_Glyph *) aglyph)->sc;
	    if ( ret[i]->parent!=sf ) {
		PyErr_Format(PyExc_TypeError,"Glyph object, %s, must belong to the expected font.", ret[i]->name);
                free(ret);
return( NULL );
	    }
	} else {
	    if ((str = (char*)PyUnicode_AsUTF8(aglyph)) == NULL ) { // FIXME: Free aglyph
		PyErr_Format(PyExc_TypeError,"Expected a name of a glyph in the expected font." );
                free(ret);
return( NULL );
	    }
	    sc = SFGetChar(sf,-1,str);
	    if ( sc==NULL ) {
		PyErr_Format(PyExc_TypeError,"String, %s, is not the name of a glyph in the expected font.", str );
                free(ret);
return( NULL );
	    }
	    ret[i] = sc;
	}
    }
    ret[i] = NULL;
return( ret );
}

static PyObject *PyFFGlyph_getPosSub(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;
    SplineFont *sf = sc->parent, *sf_sl = sf;
    int i, j, cnt;
    PyObject *ret, *temp;
    PST *pst;
    KernPair *kp;
    struct lookup_subtable *sub;
    char *subname;

    if ( sf_sl->cidmaster!=NULL ) sf_sl = sf_sl->cidmaster;
    else if ( sf_sl->mm!=NULL ) sf_sl = sf_sl->mm->normal;

    if ( !PyArg_ParseTuple(args,"s",&subname) ) {
        return( NULL );
    }
    if ( *subname=='*' )
	sub = NULL;
    else {
	sub = SFFindLookupSubtable(sf,subname);
	if ( sub==NULL ) {
	    PyErr_Format(PyExc_KeyError, "Unknown lookup subtable: '%s'",subname);
return( NULL );
	}
    }

    for ( i=0; i<2; ++i ) {
	cnt = 0;
	for ( pst = sc->possub; pst!=NULL; pst=pst->next ) {
	    if ( pst->type==pst_lcaret )
	continue;
	    if ( pst->subtable == sub || sub==NULL ) {
		if ( i ) {
		    switch ( pst->type ) {
		      default:
			Py_INCREF(Py_None);
			PyTuple_SetItem(ret,i,Py_None);
/* The important things here should not be translated. We hope the user will */
/*  never see this. Let's not translate it at all */
			LogError(_("Unexpected PST type in GetPosSub (%d).\n"), pst->type );
		      break;
		      case pst_position:
			PyTuple_SetItem(ret,cnt,Py_BuildValue("(ssiiii)",
				pst->subtable->subtable_name,"Position",
			        pst->u.pos.xoff,pst->u.pos.yoff,
			        pst->u.pos.h_adv_off, pst->u.pos.v_adv_off ));
		      break;
		      case pst_pair:
			PyTuple_SetItem(ret,cnt,Py_BuildValue("(sssiiiiiiii)",
				pst->subtable->subtable_name,"Pair",pst->u.pair.paired,
			        pst->u.pair.vr[0].xoff,pst->u.pair.vr[0].yoff,
			        pst->u.pair.vr[0].h_adv_off, pst->u.pair.vr[0].v_adv_off,
			        pst->u.pair.vr[1].xoff,pst->u.pair.vr[1].yoff,
			        pst->u.pair.vr[1].h_adv_off, pst->u.pair.vr[1].v_adv_off ));
		      break;
		      case pst_substitution:
			PyTuple_SetItem(ret,cnt,Py_BuildValue("(sss)",
				pst->subtable->subtable_name,"Substitution",pst->u.subs.variant));
		      break;
		      case pst_alternate:
		      case pst_multiple:
		      case pst_ligature:
			temp = TupleOfGlyphNames(pst->u.mult.components,2);
			PyTuple_SetItem(temp,0,PyUnicode_FromString(pst->subtable->subtable_name));
			PyTuple_SetItem(temp,1,PyUnicode_FromString(
				pst->type==pst_alternate?"AltSubs":
				pst->type==pst_multiple?"MultSubs":
			                    "Ligature"));
			PyTuple_SetItem(ret,cnt,temp);
		      break;
		    }
		}
		++cnt;
	    }
	}
	for ( j=0; j<2; ++j ) {
	    if ( sub==NULL || sub->lookup->lookup_type==gpos_pair ) {
		for ( kp= (j==0 ? sc->kerns : sc->vkerns); kp!=NULL; kp=kp->next ) {
		    if ( sub==NULL || sub==kp->subtable ) {
			if ( i ) {
			    int xadv1, yadv1, xadv2;
			    xadv1 = yadv1 = xadv2 = 0;
			    if ( j )
				yadv1 = kp->off;
			    else
				xadv1 = kp->off;
			    PyTuple_SetItem(ret,cnt,Py_BuildValue("(sssiiiiiiii)",
				    kp->subtable->subtable_name,"Pair",kp->sc->name,
			            0,0,xadv1,yadv1,
			            0,0,xadv2,0));
			}
			++cnt;
		    }
		}
	    }
	}
	if ( i==0 )
	    ret = PyTuple_New(cnt);
    }
return( ret );
}

static PyObject *PyFFGlyph_removePosSub(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;
    SplineFont *sf = sc->parent, *sf_sl = sf;
    int is_v;
    PST *pst, *next, *prev;
    KernPair *kp, *kpnext, *kpprev;
    struct lookup_subtable *sub;
    char *subname;

    if ( sf_sl->cidmaster!=NULL ) sf_sl = sf_sl->cidmaster;
    else if ( sf_sl->mm!=NULL ) sf_sl = sf_sl->mm->normal;

    if ( !PyArg_ParseTuple(args,"s",&subname) ) {
        return( NULL );
    }
    if ( *subname=='*' )
	sub = NULL;
    else {
	sub = SFFindLookupSubtable(sf,subname);
	if ( sub==NULL ) {
	    PyErr_Format(PyExc_KeyError, "Unknown lookup subtable: '%s'",subname);
return( NULL );
	}
    }

    for ( prev=NULL, pst = sc->possub; pst!=NULL; pst=next ) {
	next = pst->next;
	if ( pst->type==pst_lcaret ) {
	    prev = pst;
    continue;
	}
	if ( pst->subtable == sub || sub==NULL ) {
	    if ( prev==NULL )
		sc->possub = next;
	    else
		prev->next = next;
	    pst->next = NULL;
	    PSTFree(pst);
	} else
	    prev = pst;
    }
    for ( is_v=0; is_v<2; ++is_v ) {
	for ( kpprev=NULL, kp = is_v ? sc->vkerns: sc->kerns; kp!=NULL; kp=kpnext ) {
	    kpnext = kp->next;
	    if ( kp->subtable == sub || sub==NULL ) {
		if ( kpprev!=NULL )
		    kpprev->next = kpnext;
		else if ( is_v )
		    sc->vkerns = kpnext;
		else
		    sc->kerns = kpnext;
		kp->next = NULL;
		KernPairsFree(kp);
	    } else
		kpprev = kp;
	}
    }
Py_RETURN( self );
}

static PyObject *PyFFGlyph_addPosSub(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc, *osc;
    SplineFont *sf = sc->parent, *sf_sl = sf;
    PST temp, *pst, *old=NULL, *prev=NULL;
    struct lookup_subtable *sub;
    const char *subname, *other;
    KernPair *kp, *kpold=NULL, *kpprev=NULL;
    PyObject *others;

    memset(&temp,0,sizeof(temp));

    if ( sf_sl->cidmaster!=NULL ) sf_sl = sf_sl->cidmaster;
    else if ( sf_sl->mm!=NULL ) sf_sl = sf_sl->mm->normal;

    if ( PySequence_Size(args)==0 ) {
	PyErr_Format(PyExc_TypeError,"The first argument must be a subtable name");
return( NULL );
    }
    others = PySequence_GetItem(args, 0);
    subname = PyUnicode_AsUTF8(others);
    Py_XDECREF(others);
    if (subname == NULL) {
        return NULL;
    }
    sub = SFFindLookupSubtable(sf,subname);
    if ( sub==NULL ) {
	PyErr_Format(PyExc_KeyError, "Unknown lookup subtable: %s",subname);
	return( NULL );
    }

    temp.subtable = sub;

    for ( old = sc->possub; old!=NULL && old->subtable!=sub; prev=old, old=old->next );

    if ( sub->lookup->lookup_type==gpos_single ) {
	if ( !PyArg_ParseTuple(args,"shhhh", &subname,
		&temp.u.pos.xoff, &temp.u.pos.yoff,
		&temp.u.pos.h_adv_off, &temp.u.pos.v_adv_off))
return( NULL );
	temp.type = pst_position;
    } else if ( sub->lookup->lookup_type==gpos_pair ) {
	int off =0x7fffffff;
	temp.type = pst_pair;
	temp.u.pair.vr = chunkalloc(sizeof(struct vr [2]));
	if ( PyArg_ParseTuple(args,"ssi", &subname, &other, &off ))
	    /* Good */;
	else {
	    off = 0x7fffffff;
	    PyErr_Clear();
	    if ( !PyArg_ParseTuple(args,"sshhhhhhhh", &subname, &other,
		    &temp.u.pair.vr[0].xoff, &temp.u.pair.vr[0].yoff,
		    &temp.u.pair.vr[0].h_adv_off, &temp.u.pair.vr[0].v_adv_off,
		    &temp.u.pair.vr[1].xoff, &temp.u.pair.vr[1].yoff,
		    &temp.u.pair.vr[1].h_adv_off, &temp.u.pair.vr[1].v_adv_off))
return( NULL );
	    if ( temp.u.pair.vr[0].xoff==0 && temp.u.pair.vr[0].yoff==0 &&
		    temp.u.pair.vr[1].xoff==0 && temp.u.pair.vr[1].yoff==0 &&
		    temp.u.pair.vr[1].v_adv_off==0 ) {
		if ( temp.u.pair.vr[0].h_adv_off==0 && temp.u.pair.vr[1].h_adv_off==0 &&
			sub->vertical_kerning )
		    off = temp.u.pair.vr[0].v_adv_off;
		else if ( temp.u.pair.vr[0].h_adv_off==0 && temp.u.pair.vr[0].v_adv_off==0 &&
			SCRightToLeft(sc))
		    off = temp.u.pair.vr[1].h_adv_off;
		else if ( temp.u.pair.vr[0].v_adv_off==0 && temp.u.pair.vr[1].h_adv_off==0 )
		    off = temp.u.pair.vr[0].h_adv_off;
	    }
	}
	osc = SFGetChar(sf,-1,other);
	for ( old = sc->possub; old!=NULL &&
		(old->subtable!=sub || strcmp(old->u.pair.paired,other)!=0);
		prev=old, old=old->next );
	kpprev = NULL;
	for ( kpold = sub->vertical_kerning? sc->vkerns : sc->kerns;
		kpold!=NULL && (kpold->subtable!=sub || kpold->sc!=osc);
		kpprev = kpold, kpold = kpold->next );
	if ( off!=0x7fffffff && osc!=NULL ) {
	    if ( kpold!=NULL ) {
		kp = kpold;
	    } else {
		chunkfree(temp.u.pair.vr,sizeof(struct vr [2]));
		kp = chunkalloc(sizeof(KernPair));
		if ( sub->vertical_kerning ) {
		    kp->next = sc->vkerns;
		    sc->vkerns = kp;
		} else {
		    kp->next = sc->kerns;
		    sc->kerns = kp;
		}
	    }
	    kp->sc = osc;
	    kp->off = off;
	    kp->subtable = sub;
	    if ( old!=NULL ) {
		if ( prev==NULL )
		    sc->possub = old->next;
		else
		    prev->next = old->next;
		old->next = NULL;
		PSTFree(old);
	    }
Py_RETURN( self );
	}
	temp.u.pair.paired = copy(other);
	if ( kpold!=NULL ) {
	    if ( kpprev!=NULL )
		kpprev->next = kpold->next;
	    else if ( sub->vertical_kerning )
		sc->vkerns = kpold->next;
	    else
		sc->kerns = kpold->next;
	    kpold->next = NULL;
	    KernPairsFree(kpold);
	}
    } else if ( sub->lookup->lookup_type==gsub_single ) {
	if ( !PyArg_ParseTuple(args,"ss", &subname, &other))
return( NULL );
	temp.type = pst_substitution;
	temp.u.subs.variant = copy(other);
    } else {
	if ( !PyArg_ParseTuple(args,"sO", &subname, &others))
return( NULL );
	other = GlyphNamesFromTuple(others);
	if ( other==NULL )
return( NULL );
	if ( sub->lookup->lookup_type==gsub_alternate )
	    temp.type = pst_alternate;
	else if ( sub->lookup->lookup_type==gsub_multiple )
	    temp.type = pst_multiple;
	else if ( sub->lookup->lookup_type==gsub_ligature ) {
	    temp.type = pst_ligature;
	    old = NULL;
	} else {
	    PyErr_Format(PyExc_KeyError, "Unexpected lookup type: %s",sub->lookup->lookup_name);
return( NULL );
	}
	temp.u.subs.variant = (char*)other; // other holds rv from GlyphNamesFromTuple
    }
    if ( old!=NULL ) {
	switch ( sub->lookup->lookup_type ) {
	  case gpos_single:
	    old->u.pos = temp.u.pos;
	  break;
	  case gpos_pair:
	    chunkfree(old->u.pair.vr,sizeof(struct vr [2]));
	    free(old->u.pair.paired);
	    old->u.pair = temp.u.pair;
	  break;
	  default:
	    free(old->u.subs.variant);
	    old->u.subs.variant = temp.u.subs.variant;
	  break;
	}
    } else {
	pst = chunkalloc(sizeof(PST));
	*pst = temp;
	pst->next = sc->possub;
	sc->possub = pst;
    }
Py_RETURN( self );
}

static PyObject *PyFFGlyph_selfIntersects(PyObject *self, PyObject *UNUSED(args)) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;
    int layer = ((PyFF_Glyph *) self)->layer;
    Spline *s, *s2;
    PyObject *ret;
    SplineSet *ss;

    ss = LayerAllSplines(&sc->layers[layer]);
    ret = SplineSetIntersect(ss,&s,&s2) ? Py_True : Py_False;
    LayerUnAllSplines(&sc->layers[layer]);
    Py_INCREF( ret );
return( ret );
}

static PyObject *PyFFGlyph_validate(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;
    int force=false;
    int layer = ((PyFF_Glyph *) self)->layer;

    if ( !PyArg_ParseTuple(args,"|i",&force) )
return( NULL );
return( Py_BuildValue( "i", SCValidate(sc,layer,force)) );
}

/* Transformation flags: see 'enum fvtrans_flags' in baseviews.h */
static struct flaglist g_trans_flags[] = {
    { "partialRefs", fvt_partialreftrans },
    { "round", fvt_round_to_int },
    FLAGLIST_EMPTY /* Sentinel */
};

static PyObject *PyFFGlyph_Transform(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;
    int i;
    double m[6];
    real t[6];
    int flags;
    PyObject *flagO=NULL;

    if ( !PyArg_ParseTuple(args,"(dddddd)|O",&m[0], &m[1], &m[2], &m[3], &m[4], &m[5],
	    &flagO) )
return( NULL );
    flags = FlagsFromTuple(flagO,g_trans_flags,"transformation flag");
    if ( flags==FLAG_UNKNOWN )
return( NULL );
    flags |= fvt_alllayers;
    for ( i=0; i<6; ++i )
	t[i] = m[i];
    FVTrans(sc->parent->fv,sc,t,NULL,flags);
Py_RETURN( self );
}

static PyObject *PyFFGlyph_NLTransform(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;
    char *xexpr, *yexpr;

    if ( !PyArg_ParseTuple(args,"ss", &xexpr, &yexpr ) )
return( NULL );
    if ( !SCNLTrans(sc,((PyFF_Glyph *) self)->layer,xexpr,yexpr) ) {
	PyErr_Format(PyExc_TypeError, "Unparseable expression.");
return( NULL );
    }
Py_RETURN( self );
}

static PyObject *PyFFGlyph_Simplify(PyFF_Glyph *self, PyObject *args) {
    static struct simplifyinfo smpl = { sf_normal, 0.75, 0.2, 10, 0, 0, 0 };
    SplineChar *sc = self->sc;
    SplineFont *sf = sc->parent;
    int em = sf->ascent+sf->descent;

    smpl.err = em/1000.;
    smpl.linefixup = em/500.;
    smpl.linelenmax = em/100.;

    if ( PySequence_Size(args)>=1 )
	smpl.err = PyFloat_AsDouble(PySequence_GetItem(args,0));
    if ( !PyErr_Occurred() && PySequence_Size(args)>=2 )
	smpl.flags = FlagsFromTuple( PySequence_GetItem(args,1),simplifyflags,"simplify flag");
    if ( !PyErr_Occurred() && PySequence_Size(args)>=3 )
	smpl.tan_bounds = PyFloat_AsDouble( PySequence_GetItem(args,2));
    if ( !PyErr_Occurred() && PySequence_Size(args)>=4 )
	smpl.linefixup = PyFloat_AsDouble( PySequence_GetItem(args,3));
    if ( !PyErr_Occurred() && PySequence_Size(args)>=5 )
	smpl.linelenmax = PyFloat_AsDouble( PySequence_GetItem(args,4));
    if ( PyErr_Occurred() )
return( NULL );
    sc->layers[self->layer].splines = SplineCharSimplify(sc,sc->layers[self->layer].splines,&smpl);
    SCCharChangedUpdate(self->sc,self->layer);
Py_RETURN( self );
}

static PyObject *PyFFGlyph_Round(PyFF_Glyph *self, PyObject *args) {
    double factor=1;

    if ( !PyArg_ParseTuple(args,"|d",&factor ) )
return( NULL );
    SCRound2Int( self->sc,self->layer,factor);
    SCCharChangedUpdate(self->sc,self->layer);
Py_RETURN( self );
}

static PyObject *PyFFGlyph_Cluster(PyFF_Glyph *self, PyObject *args) {
    double within = .1, max = .5;

    if ( !PyArg_ParseTuple(args,"|dd", &within, &max ) )
return( NULL );

    SCRoundToCluster( self->sc,self->layer,false,within,max);
    SCCharChangedUpdate(self->sc,self->layer);
Py_RETURN( self );
}

static PyObject *PyFFGlyph_AddExtrema(PyFF_Glyph *self, PyObject *args) {
    int emsize = 1000;
    char *flag = NULL;
    int ae = ae_only_good;
    SplineChar *sc = self->sc;
    SplineFont *sf = sc->parent;

    if ( !PyArg_ParseTuple(args,"|si", &flag, &emsize ) )
return( NULL );
    if ( flag!=NULL ) {
	ae = FlagsFromString(flag,addextremaflags,"extrema flag");
	if ( ae == FLAG_UNKNOWN )
return( NULL );
    }
    SplineCharAddExtrema(sc,sc->layers[self->layer].splines,ae,sf->ascent+sf->descent);
    SCCharChangedUpdate(sc,self->layer);
Py_RETURN( self );
}

static PyObject *_PyFFGlyph_Action(PyFF_Glyph *self, void (*func)(SplineChar*, SplineSet*, int)) { 
    SplineChar *sc = self->sc;
    func(sc,sc->layers[self->layer].splines,false);
    SCCharChangedUpdate(sc,self->layer);
Py_RETURN( self );
}

static PyObject *PyFFGlyph_AddInflections(PyFF_Glyph *self) {
    return _PyFFGlyph_Action(self,&SplineCharAddInflections);
}

static PyObject *PyFFGlyph_Balance(PyFF_Glyph *self) {
    return _PyFFGlyph_Action(self,&SplineCharBalance);
}

static PyObject *PyFFGlyph_Harmonize(PyFF_Glyph *self) {
    return _PyFFGlyph_Action(self,&SplineCharHarmonize);
}

static PyObject *PyFFGlyph_Stroke(PyFF_Glyph *self, PyObject *args, PyObject *keywds) {
    StrokeInfo si;
    SplineSet *newss;

    if ( Stroke_Parse(&si, args, keywds)==-1 )
	return( NULL );

    newss = SplineSetStroke(self->sc->layers[self->layer].splines,&si,
	    self->sc->layers[self->layer].order2);
    SplinePointListFree(self->sc->layers[self->layer].splines);
    self->sc->layers[self->layer].splines = newss;
    SCCharChangedUpdate(self->sc,self->layer);
    SplinePointListsFree(si.nib); si.nib = NULL;
Py_RETURN( self );
}

static PyObject *PyFFGlyph_Correct(PyFF_Glyph *self, PyObject *UNUSED(args)) {
    int changed = false;

    self->sc->layers[self->layer].splines = SplineSetsCorrect(self->sc->layers[self->layer].splines,&changed);
    if ( changed )
	SCCharChangedUpdate(self->sc,self->layer);
Py_RETURN( self );
}

static PyObject *PyFFGlyph_RemoveOverlap(PyFF_Glyph *self, PyObject *UNUSED(args)) {

    self->sc->layers[self->layer].splines = SplineSetRemoveOverlap(self->sc,self->sc->layers[self->layer].splines,over_remove);
    SCCharChangedUpdate(self->sc,self->layer);
Py_RETURN( self );
}

static PyObject *PyFFGlyph_Intersect(PyFF_Glyph *self, PyObject *UNUSED(args)) {

    self->sc->layers[self->layer].splines = SplineSetRemoveOverlap(self->sc,self->sc->layers[self->layer].splines,over_intersect);
    SCCharChangedUpdate(self->sc,self->layer);
Py_RETURN( self );
}

static PyObject *PyFFGlyph_Exclude(PyFF_Glyph *self, PyObject *args) {
    SplineSet *ss, *excludes = NULL, *tail;
    PyObject *obj;

    if ( !PyArg_ParseTuple(args,"O", &obj ) )
return( NULL );
    if ( !PyType_IsSubtype(&PyFF_LayerType, Py_TYPE(obj)) ) {
	PyErr_Format(PyExc_TypeError, "Value must be a (FontForge) Layer");
return( NULL );
    }

    excludes = SSFromLayer((PyFF_Layer *) obj);
    if ( PyErr_Occurred() != NULL ) {
	return( NULL );
    }
    ss = self->sc->layers[self->layer].splines;
    for ( tail=ss; tail->next!=NULL; tail=tail->next );
    tail->next = excludes;
    while ( excludes!=NULL ) {
	excludes->first->selected = true;
	excludes = excludes->next;
    }
    self->sc->layers[self->layer].splines = SplineSetRemoveOverlap(NULL,ss,over_exclude);
    /* Frees the old splinesets */
    SCCharChangedUpdate(self->sc,self->layer);
Py_RETURN( self );
}

static PyObject *PyFFGlyph_preserveLayer(PyFF_Glyph *self, PyObject *args) {
    int layer = self->layer, dohints=false;
    PyObject *layerObj=NULL;
    SplineChar *sc = self->sc;

    if ( !PyArg_ParseTuple(args,"|Op", &layerObj, &dohints ) )
        return( NULL );
    if ( layerObj!=NULL ) {
	layer = LayerArgToLayer(sc->parent, layerObj);
	if ( layer == ly_none )
	    return NULL;
    }
    else if ( layer<0 || layer>=sc->layer_cnt ) {
        PyErr_Format(PyExc_ValueError, "Layer is out of range" );
        return( NULL );
    }
    _SCPreserveLayer(sc,layer,dohints);
    Py_RETURN( self );
}

static PyObject *PyFFGlyph_doUndo(PyFF_Glyph *self, PyObject *args) {
    int layer = self->layer, redo=false;
    PyObject *layerObj=NULL;
    SplineChar *sc = self->sc;

    if ( !PyArg_ParseTuple(args,"|Op", &layerObj, &redo ) )
        return( NULL );
    if ( layerObj!=NULL ) {
	layer = LayerArgToLayer(sc->parent, layerObj);
	if ( layer == ly_none )
	    return NULL;
    }
    else if ( layer<0 || layer>=sc->layer_cnt ) {
        PyErr_Format(PyExc_ValueError, "Layer is out of range" );
        return( NULL );
    }
    if ( redo )
	SCDoRedo(sc, layer);
    else
	SCDoUndo(sc, layer);
    Py_RETURN( self );
}

static int LayerArgToLayer(SplineFont *sf, PyObject* layerp) {
    int layeri;

    if ( PyUnicode_Check(layerp)) {
        const char *name = PyUnicode_AsUTF8(layerp);
        if (name == NULL) {
            return ly_none;
        }
        layeri = SFFindLayerIndexByName(sf, name);
        if ( layeri<0 ) {
            PyErr_Format(PyExc_ValueError, "Requested layer '%s' not found", name);
            return ly_none;
        }
    } else if (!PyLong_Check(layerp)) {
        PyErr_Format(PyExc_ValueError, "First argument must be string or layer index");
        return ly_none;
    } else {
        layeri = PyLong_AsLong(layerp);
    }
    return layeri;
}

static PyObject *PyFFGlyph_boundingBox(PyFF_Glyph *self, PyObject *UNUSED(args), PyObject *keywds) {
    DBounds bb;
    int layeri;

    PyObject* layerp = NULL;
    if (keywds != NULL) layerp = PyDict_GetItemString(keywds, "layer");

    if (layerp != NULL) {
        layeri = LayerArgToLayer(self->sc->parent, layerp);
        if (layeri == ly_none) return NULL;
        SplineCharLayerFindBounds(self->sc, layeri, &bb);
    } else {
        SplineCharFindBounds(self->sc, &bb);
    }

    return( Py_BuildValue("(dddd)", bb.minx,bb.miny, bb.maxx,bb.maxy ));
}

static PyObject* PyFF_Glyph_BoundsAt(PyCFunction bounds_func, PyFF_Glyph *self, PyObject *args, PyObject *keywds) {
    PyObject *temp = NULL;
    double nmin = 0, nmax = 0;
    double tnmin = 0, tnmax = 0;
    bool set = false;
    int layeri;

    PyFF_Contour* tempc = NULL;

    PyObject* layerp = NULL;
    if (keywds != NULL) layerp = PyDict_GetItemString(keywds, "layer");

    if (layerp != NULL) {
        layeri = LayerArgToLayer(self->sc->parent, layerp);
        if (layeri == ly_none) return NULL;
    }

    int layer_cnt = self->sc->layer_cnt;
    for (int i = 0; i < layer_cnt; i++) {
        if (layerp != NULL && layeri != i) continue;
        SplineSet* ss = LayerAllSplines(&self->sc->layers[i]);
        while (ss != NULL) {
            tempc = ContourFromSS(ss, NULL);
            temp = bounds_func((PyObject*)tempc, args);
            ss = ss->next;

            Py_DECREF(tempc);
            if (!PyTuple_Check(temp)) {
                Py_DECREF(temp);
                continue;
            } else if (!PyArg_ParseTuple(temp, "dd", &tnmin, &tnmax)) {
                IError("bounds_func returned an invalid tuple" );
                Py_DECREF(temp);
                return NULL;
            }
            Py_DECREF(temp);

            if (tnmin < nmin || !set) nmin = tnmin;
            if (tnmax > nmax || !set) nmax = tnmax;
            set = true;
        }
        LayerUnAllSplines(&self->sc->layers[i]);
    }

    if (set) {
        PyObject *bounds = Py_BuildValue("(dd)", nmin, nmax);
        return bounds;
    } else {
        Py_RETURN_NONE;
    }
}

static PyObject *PyFFGlyph_xBoundsAtY(PyFF_Glyph *self, PyObject *args, PyObject *keywds) {
    return PyFF_Glyph_BoundsAt((PyCFunction)PyFFContour_xBoundsAtY, self, args, keywds);
}

static PyObject *PyFFGlyph_yBoundsAtX(PyFF_Glyph *self, PyObject *args, PyObject *keywds) {
    return PyFF_Glyph_BoundsAt((PyCFunction)PyFFContour_yBoundsAtX, self, args, keywds);
}

static PyObject *PyFFGlyph_clear(PyFF_Glyph *self, PyObject *args) {
	int arglen = PySequence_Size(args);
    int layeri = self->layer;

    if (arglen <= 0) {
        SCClearContents(self->sc,layeri);
    } else if (arglen > 1) {
        PyErr_Format(PyExc_TypeError, "Too many arguments, only a layer index is allowed");
        return NULL;
    } else { /* arglen == 1 */
        PyObject *layerp = PySequence_GetItem(args,0);
        int layeri = LayerArgToLayer(self->sc->parent, layerp);
        if (layeri == ly_none) return NULL;

        // ly_grid not clearable with this function. In any event, it makes no
        // sense to put it here; clearing ly_grid should quite rightly go at
        // the font level since ly_grid is per-font not per-glyph so this is
        // not a bug
        if ( layeri<ly_back || layeri>=self->sc->layer_cnt ) {
            PyErr_Format(PyExc_ValueError, "Layer is out of range");
            return NULL;
        }
        SCClearLayer(self->sc, layeri);
    }

    SCCharChangedUpdate(self->sc,layeri);
    Py_RETURN(self);
}

static PyObject *PyFFGlyph_isWorthOutputting(PyFF_Glyph *self, PyObject *UNUSED(args)) {
    PyObject *ret;

    ret = SCWorthOutputting(self->sc) ? Py_True : Py_False;
    Py_INCREF( ret );
return( ret );
}

static PyObject *PyFFGlyph_setLayer(PyFF_Glyph *self, PyObject *args) {
	PyObject *layer, *index, *flagsobj = NULL;
	int layeri, flags = 0;

	if (!PyArg_ParseTuple(args, "OO|O", &layer, &index, &flagsobj)) {
		PyErr_Format(PyExc_ValueError, "Must be called with a layer, a layer identifier, and an optional flags tuple" );
		return NULL;
	} else if ( ! (PyType_IsSubtype(&PyFF_LayerType, Py_TYPE(layer)) || PyType_IsSubtype(&PyFF_ContourType, Py_TYPE(layer)) ) ) {
		PyErr_Format(PyExc_ValueError, "First argument must be a layer or contour" );
		return NULL;
	} else if ( PyUnicode_Check(index)) {
		const char *name = PyUnicode_AsUTF8(index);
		if (name == NULL) {
			return NULL;
		}
		layeri = SFFindLayerIndexByName(self->sc->parent,name);
		if ( layeri<0 ) {
			PyErr_Format(PyExc_ValueError, "Layer '%s' not found", name);
			return NULL;
		}
	} else if ( PyLong_Check(index)) {
		layeri = PyLong_AsLong(index);
	} else {
		PyErr_Format(PyExc_TypeError, "Index must be a layer name or index" );
		return NULL;
	}
	if ( flagsobj != NULL ) {
		flags = FlagsFromTuple(flagsobj,pconvertflags,"Point Conversion flags");
	}
	flags = CheckPConvertFlags(flags, pconvert_flag_all|pconvert_flag_by_geom);
	if ( flags < 0 )
		return NULL;

	if ( PyFF_Glyph_CSetLayer(self,layer,layeri,flags) == 0 )
		Py_RETURN (self);

	return NULL;
}

static PyMethodDef PyFF_Glyph_methods[] = {
    { "glyphPen", (PyCFunction) PyFFGlyph_GlyphPen, METH_VARARGS | METH_KEYWORDS, "Create a pen object which can draw into this glyph"},
    { "draw", (PyCFunction) PyFFGlyph_draw, METH_VARARGS , "Draw the glyph's outline to the pen argument"},
    { "addExtrema", (PyCFunction) PyFFGlyph_AddExtrema, METH_VARARGS, "Add extrema to the contours of the glyph"},
    { "addInflections", (PyCFunction) PyFFGlyph_AddInflections, METH_NOARGS, "Add points of inflection to the contours of the glyph"},
    { "balance", (PyCFunction) PyFFGlyph_Balance, METH_NOARGS, "Balance the handles on the contours of the glyph"},
    { "harmonize", (PyCFunction) PyFFGlyph_Harmonize, METH_NOARGS, "Harmonize the curvatures on the contours of the glyph"},
    { "addReference", PyFFGlyph_AddReference, METH_VARARGS, "Add a reference"},
    { "addAnchorPoint", PyFFGlyph_addAnchorPoint, METH_VARARGS, "Adds an anchor point"},
    { "addHint", PyFFGlyph_addHint, METH_VARARGS, "Add a postscript hint (is_vertical_hint,start_pos,width)"},
    { "addPosSub", PyFFGlyph_addPosSub, METH_VARARGS, "Adds position/substitution data to the glyph"},
    { "appendAccent", (PyCFunction) PyFFGlyph_appendAccent, METH_VARARGS | METH_KEYWORDS, "Append the named accent to the current glyph, positioning according to the rules buildAccent would use" },
    { "autoHint", PyFFGlyph_autoHint, METH_NOARGS, "Guess at postscript hints"},
    { "autoInstr", PyFFGlyph_autoInstr, METH_NOARGS, "Guess at truetype instructions"},
    { "autoTrace", PyFFGlyph_autoTrace, METH_NOARGS, "Autotrace any background images"},
    { "boundingBox", (PyCFunction) PyFFGlyph_boundingBox, METH_VARARGS | METH_KEYWORDS, "Finds the minimum bounding box for the glyph (xmin,ymin,xmax,ymax)" },
    { "build", PyFFGlyph_Build, METH_VARARGS, "If the current glyph is an accented character\nand all components are in the font\nthen build it out of references" },
    { "canonicalContours", (PyCFunction) PyFFGlyph_canonicalContours, METH_NOARGS, "Orders the contours in the current glyph by the x coordinate of their leftmost point. (This can reduce the size of the postscript charstring needed to describe the glyph(s)."},
    { "canonicalStart", (PyCFunction) PyFFGlyph_canonicalStart, METH_NOARGS, "Sets the start point of all the contours of the current glyph to be the leftmost point on the contour."},
    { "changeWeight", (PyCFunction) PyFFGlyph_changeWeight, METH_VARARGS, "Change the weight (thickness) of the stems of the glyph"},
    { "condenseExtend", (PyCFunction) PyFFGlyph_condenseExtend, METH_VARARGS, "Change the widths of the counters and side bearings of the glyph"},
    { "clear", (PyCFunction) PyFFGlyph_clear, METH_VARARGS, "Clears the contents of either a single layer of a glyph or of all the glyph's layers; makes it not worth outputting depending on its invocation" },
    { "cluster", (PyCFunction) PyFFGlyph_Cluster, METH_VARARGS, "Cluster the points of a glyph towards common values" },
    { "correctDirection", (PyCFunction) PyFFGlyph_Correct, METH_NOARGS, "Orient a layer so that external contours are clockwise and internal counter clockwise." },
    { "exclude", (PyCFunction) PyFFGlyph_Exclude, METH_VARARGS, "Exclude the area of the argument (a layer) from the current glyph"},
    { "export", (PyCFunction) PyFFGlyph_export, METH_VARARGS|METH_KEYWORDS, "Export the glyph, the format is determined by the extension. (provide the filename of the image file)" },
    { "genericGlyphChange", (PyCFunction) PyFFGlyph_genericGlyphChange, METH_VARARGS | METH_KEYWORDS, "Rather like changeWeight or condenseExtend, called 'Change Glyph' in UI"},
    { "getPosSub", PyFFGlyph_getPosSub, METH_VARARGS, "Gets position/substitution data from the glyph"},
    { "importOutlines", (PyCFunction) PyFFGlyph_import, METH_VARARGS|METH_KEYWORDS, "Import a background image or a foreground eps/svg/etc. (provide the filename of the image file)" },
    { "intersect", (PyCFunction) PyFFGlyph_Intersect, METH_NOARGS, "Leaves the areas where the contours of a glyph overlap."},
    { "isWorthOutputting", (PyCFunction) PyFFGlyph_isWorthOutputting, METH_NOARGS, "Returns whether the glyph is worth outputting" },
    { "removeOverlap", (PyCFunction) PyFFGlyph_RemoveOverlap, METH_NOARGS, "Remove overlapping areas from a glyph"},
    { "removePosSub", PyFFGlyph_removePosSub, METH_VARARGS, "Removes position/substitution data from the glyph"},
    { "round", (PyCFunction)PyFFGlyph_Round, METH_VARARGS, "Rounds point coordinates (and reference translations) to integers"},
    { "selfIntersects", (PyCFunction)PyFFGlyph_selfIntersects, METH_NOARGS, "Returns whether this glyph intersects itself" },
    { "setLayer", (PyCFunction)PyFFGlyph_setLayer, METH_VARARGS, "Replaces the content of the specified layer" },
    { "validate", (PyCFunction)PyFFGlyph_validate, METH_VARARGS, "Returns whether this glyph is valid for output (if not check validation_state" },
    { "simplify", (PyCFunction)PyFFGlyph_Simplify, METH_VARARGS, "Simplifies a glyph" },
    { "stroke", (PyCFunction)PyFFGlyph_Stroke, METH_VARARGS | METH_KEYWORDS, "Strokes the contours in a glyph"},
    { "transform", (PyCFunction)PyFFGlyph_Transform, METH_VARARGS, "Transform a glyph by a 6 element matrix." },
    { "nltransform", (PyCFunction)PyFFGlyph_NLTransform, METH_VARARGS, "Transform a glyph by two non-linear expressions (one for x, one for y)." },
    { "unlinkRef", PyFFGlyph_unlinkRef, METH_VARARGS, "Unlink a reference and turn it into outlines"},
    { "unlinkThisGlyph", PyFFGlyph_unlinkThisGlyph, METH_NOARGS, "Unlink all references to the current glyph in any other glyph in the font."},
    { "useRefsMetrics", PyFFGlyph_useRefsMetrics, METH_VARARGS, "Search the references of the current layer of the glyph and set the named reference's \"useMyMetrics\" flag."},
    { "xBoundsAtY", (PyCFunction)PyFFGlyph_xBoundsAtY, METH_VARARGS | METH_KEYWORDS, "The minimum and maximum values of x attained for a given y (range), or returns None"},
    { "yBoundsAtX", (PyCFunction)PyFFGlyph_yBoundsAtX, METH_VARARGS | METH_KEYWORDS, "The minimum and maximum values of y attained for a given x (range), or returns None"},
    { "preserveLayerAsUndo", (PyCFunction)PyFFGlyph_preserveLayer, METH_VARARGS, "Preserves the current layer -- as it now is -- in an undo"},
    { "doUndoLayer", (PyCFunction)PyFFGlyph_doUndo, METH_VARARGS, "Undo the last change to the layer or, if reverse is True, redo the last undo."},
    PYMETHODDEF_EMPTY /* Sentinel */
};
/* ************************************************************************** */
/*  Glyph Type  */
/* ************************************************************************** */

static PyTypeObject PyFF_GlyphType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "fontforge.glyph",         /* tp_name */
    sizeof(PyFF_Glyph),	       /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor) PyFF_Glyph_dealloc, /* tp_dealloc */
    0,                         /* tp_vectorcall_offset */
    NULL,                      /* tp_getattr */
    NULL,                      /* tp_setattr */
    NULL,                      /* tp_reserved/tp_compare */
    (reprfunc) PyFFGlyph_Repr, /* tp_repr */
    NULL,                      /* tp_as_number */
    NULL,                      /* tp_as_sequence */
    NULL,                      /* tp_as_mapping */
    NULL,                      /* tp_hash */
    NULL,                      /* tp_call */
    (reprfunc) PyFFGlyph_Str,  /* tp_str */
    NULL,                      /* tp_getattro */
    NULL,                      /* tp_setattro */
    NULL,                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    "FontForge Glyph object",  /* tp_doc */
    NULL,                      /* tp_traverse */
    NULL,                      /* tp_clear */
    (richcmpfunc)PyFFGlyph_richcompare, /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    NULL,                      /* tp_iter */
    NULL,                      /* tp_iternext */
    PyFF_Glyph_methods,        /* tp_methods */
    NULL,                      /* tp_members */
    PyFF_Glyph_getset,         /* tp_getset */
    NULL,                      /* tp_base */
    NULL,                      /* tp_dict */
    NULL,                      /* tp_descr_get */
    NULL,                      /* tp_descr_set */
    0,                         /* tp_dictoffset */
    NULL, /*(initproc)PyFF_Glyph_init*/ /* tp_init */
    NULL,                      /* tp_alloc */
    NULL, /*PyFF_Glyph_new*/   /* tp_new */
    NULL,                      /* tp_free */
    NULL,                      /* tp_is_gc */
    NULL,                      /* tp_bases */
    NULL,                      /* tp_mro */
    NULL,                      /* tp_cache */
    NULL,                      /* tp_subclasses */
    NULL,                      /* tp_weaklist */
    NULL,                      /* tp_del */
    0,                         /* tp_version_tag */
};

/* ************************************************************************** */
/* Cvt iterator type */
/* ************************************************************************** */

typedef struct {
    PyObject_HEAD
    size_t pos;
    PyFF_Cvt *cvt;
} cvtiterobject;
static PyTypeObject PyFF_CvtIterType;

static PyObject *cvtiter_new(PyObject *cvt) {
    cvtiterobject *ci;
    ci = PyObject_New(cvtiterobject, &PyFF_CvtIterType);
    if (ci == NULL)
return NULL;
    ci->cvt = ((PyFF_Cvt *) cvt);
    Py_INCREF(cvt);
    ci->pos = 0;
return (PyObject *)ci;
}

static void cvtiter_dealloc(cvtiterobject *ci) {
    Py_XDECREF(ci->cvt);
    PyObject_Del(ci);
}

static PyObject *cvtiter_iternext(cvtiterobject *ci) {
    PyFF_Cvt *cvt = ci->cvt;
    PyObject *entry;

    if ( cvt == NULL || cvt->cvt==NULL )
return NULL;

    if ( ci->pos<cvt->cvt->len/2 ) {
        entry = (Py_TYPE(cvt)->tp_as_sequence->sq_item)((PyObject *) cvt, ci->pos++);
return( entry );
    }

return NULL;
}

static PyTypeObject PyFF_CvtIterType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "fontforge.cvt_iterator",  /* tp_name */
    sizeof(cvtiterobject),     /* tp_basicsize */
    0,                         /* tp_itemsize */
    /* methods */
    (destructor)cvtiter_dealloc, /* tp_dealloc */
    0,                         /* tp_vectorcall_offset */
    NULL,                      /* tp_getattr */
    NULL,                      /* tp_setattr */
    NULL,                      /* tp_compare */
    NULL,                      /* tp_repr */
    NULL,                      /* tp_as_number */
    NULL,                      /* tp_as_sequence */
    NULL,                      /* tp_as_mapping */
    NULL,                      /* tp_hash */
    NULL,                      /* tp_call */
    NULL,                      /* tp_str */
    NULL,                      /* tp_getattro */
    NULL,                      /* tp_setattro */
    NULL,                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    NULL,                      /* tp_doc */
    NULL,                      /* tp_traverse */
    NULL,                      /* tp_clear */
    NULL,                      /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    PyObject_SelfIter,         /* tp_iter */
    (iternextfunc)cvtiter_iternext, /* tp_iternext */
    NULL,                      /* tp_methods */
    NULL,                      /* tp_members */
    PyFF_Glyph_getset,         /* tp_getset */
    NULL,                      /* tp_base */
    NULL,                      /* tp_dict */
    NULL,                      /* tp_descr_get */
    NULL,                      /* tp_descr_set */
    0,                         /* tp_dictoffset */
    NULL, /*(initproc)PyFF_Glyph_init*/ /* tp_init */
    NULL,                      /* tp_alloc */
    NULL, /*PyFF_Glyph_new*/   /* tp_new */
    NULL,                      /* tp_free */
    NULL,                      /* tp_is_gc */
    NULL,                      /* tp_bases */
    NULL,                      /* tp_mro */
    NULL,                      /* tp_cache */
    NULL,                      /* tp_subclasses */
    NULL,                      /* tp_weaklist */
    NULL,                      /* tp_del */
    0,                         /* tp_version_tag */
};

/* ************************************************************************** */
/* Cvt sequence object */
/* ************************************************************************** */

static PyTypeObject PyFF_CvtType;

static void PyFFCvt_dealloc(PyFF_Cvt *self) {
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *PyFFCvt_new(PyFF_Font *owner) {
    PyFF_Cvt *self;

    if ( CheckIfFontClosed(owner) )
return( NULL );

    if ( owner->cvt!=NULL )
Py_RETURN( owner->cvt );
    self = PyObject_New(PyFF_Cvt, &PyFF_CvtType);
    ((PyFF_Cvt *) self)->sf = owner->fv->sf;
    ((PyFF_Cvt *) self)->cvt = SFFindTable(self->sf,CHR('c','v','t',' '));
    owner->cvt = self;
Py_RETURN( self );
}

static PyObject *PyFFCvt_Str(PyFF_Cvt *self) {
return( PyUnicode_FromFormat( "<cvt table for font %s>", self->sf->fontname ));
}


static PyObject *PyFFCvt_get_font(PyFF_Cvt *self, void *UNUSED(closure)) {
    PyFF_Font *font = PyFF_FontForSF( self->sf );
    if ( font==NULL )
Py_RETURN_NONE;
    Py_INCREF(font);
    return (PyObject*)font;
}


/* ************************************************************************** */
/* Cvt sequence */
/* ************************************************************************** */

static Py_ssize_t PyFFCvt_Length( PyObject *self ) {
    if ( ((PyFF_Cvt *) self)->cvt==NULL )
return( 0 );
    else
return( ((PyFF_Cvt *) self)->cvt->len/2 );
}

static struct ttf_table *BuildCvt(SplineFont *sf,int initial_size) {
    struct ttf_table *cvt;

    cvt = chunkalloc(sizeof(struct ttf_table));
    cvt->next = sf->ttf_tables;
    sf->ttf_tables = cvt;
    cvt->tag = CHR('c','v','t',' ');
    cvt->data = malloc(initial_size);
    cvt->len = 0;
    cvt->maxlen = initial_size;
return( cvt );
}

static PyObject *PyFFCvt_Concat( PyObject *_c1, PyObject *_c2 ) {
    PyFF_Cvt *c1 = (PyFF_Cvt *) _c1, *c2 = (PyFF_Cvt *) _c2;
    PyObject *ret;
    Py_ssize_t len1, len2;
    int i;
    int is_cvt2;

    len1 = PyFFCvt_Length(_c1);
    if ( PyType_IsSubtype(&PyFF_CvtType, Py_TYPE(c2)) ) {
	len2 = PyFFCvt_Length(_c2);
	is_cvt2 = true;
    } else if ( PySequence_Check(_c2)) {
	is_cvt2 = false;
	len2 = PySequence_Size(_c2);
    } else {
	PyErr_Format(PyExc_TypeError, "The second argument must be either another cvt or a tuple of integers");
return( NULL );
    }
    ret = PyTuple_New(len1+len2);
    for ( i=0; i<len1; ++i )
	PyTuple_SetItem(ret,i,Py_BuildValue("i",memushort(c1->cvt->data,c1->cvt->len,i*sizeof(uint16_t))) );
    if ( is_cvt2 ) {
	for ( i=0; i<len2; ++i )
	    PyTuple_SetItem(ret,len1+i,Py_BuildValue("i",memushort(c2->cvt->data,c2->cvt->len,i*sizeof(uint16_t))) );
    } else {
	for ( i=0; i<len2; ++i )
	    PyTuple_SetItem(ret,len1+i,Py_BuildValue("i",PySequence_GetItem(_c2,i)));
    }
Py_RETURN( (PyObject *) ret );
}

static PyObject *PyFFCvt_InPlaceConcat( PyObject *_self, PyObject *_c2 ) {
    PyFF_Cvt *self = (PyFF_Cvt *) _self, *c2 = (PyFF_Cvt *) _c2;
    int i;
    int is_cvt2;
    Py_ssize_t len1, len2;
    struct ttf_table *cvt;

    len1 = PyFFCvt_Length(_self);
    if ( PyType_IsSubtype(&PyFF_CvtType, Py_TYPE(c2)) ) {
	len2 = PyFFCvt_Length(_c2);
	is_cvt2 = true;
    } else if ( PySequence_Check(_c2)) {
	is_cvt2 = false;
	len2 = PySequence_Size(_c2);
    } else {
	PyErr_Format(PyExc_TypeError, "The second argument must be either another cvt or a tuple of integers");
return( NULL );
    }

    if ( self->cvt==NULL )
	self->cvt = BuildCvt(self->sf,(len1+len2)*2);
    cvt = self->cvt;
    if ( (len1+len2)*2 >= cvt->maxlen )
	cvt->data = realloc(cvt->data,cvt->maxlen = 2*(len1+len2)+10 );
    if ( is_cvt2 ) {
	if ( len2!=0 )
	    memcpy(cvt->data+len1*sizeof(uint16_t),c2->cvt->data, 2*len2);
    } else {
	for ( i=0; i<len2; ++i ) {
	    int val = PyLong_AsLong(PySequence_GetItem(_c2,i));
	    if ( PyErr_Occurred())
return( NULL );
	    memputshort(cvt->data,sizeof(uint16_t)*(len1+i),val);
	}
    }
    cvt->len += 2*len2;
Py_RETURN( self );
}

static PyObject *PyFFCvt_Index( PyObject *self, Py_ssize_t pos ) {
    PyFF_Cvt *c = (PyFF_Cvt *) self;

    if ( c->cvt==NULL || pos<0 || pos>=c->cvt->len/2) {
	PyErr_Format(PyExc_TypeError, "Index out of bounds");
return( NULL );
    }
return( Py_BuildValue("i",(short)memushort(c->cvt->data,c->cvt->len,pos*sizeof(uint16_t))) );
}

static int PyFFCvt_IndexAssign( PyObject *self, Py_ssize_t pos, PyObject *value ) {
    PyFF_Cvt *c = (PyFF_Cvt *) self;
    struct ttf_table *cvt;
    int val;

    val = PyLong_AsLong(value);
    if ( PyErr_Occurred())
return( -1 );
    if ( c->cvt==NULL )
	c->cvt = BuildCvt(c->sf,2);
    cvt = c->cvt;
    if ( cvt==NULL || pos<0 || pos>cvt->len/2) {
	PyErr_Format(PyExc_TypeError, "Index out of bounds");
return( -1 );
    }
    if ( 2*pos>=cvt->maxlen )
	cvt->data = realloc(cvt->data,cvt->maxlen = sizeof(uint16_t)*pos+10 );
    if ( 2*pos>=cvt->len )
	cvt->len = sizeof(uint16_t)*pos;
    memputshort(cvt->data,sizeof(uint16_t)*pos,val);
return( 0 );
}

static PyObject *PyFFCvt_Slice( PyObject *self, Py_ssize_t start, Py_ssize_t end ) {
    PyFF_Cvt *c = (PyFF_Cvt *) self;
    struct ttf_table *cvt;
    int len, i;
    PyObject *ret;

    cvt = c->cvt;
    if ( cvt==NULL || end<start || end <0 || 2*start>=cvt->len ) {
	PyErr_Format(PyExc_ValueError, "Slice specification out of range" );
return( NULL );
    }

    len = end-start + 1;

    ret = PyTuple_New(len);
    for ( i=start; i<=end; ++i )
	PyTuple_SetItem(ret,i-start,Py_BuildValue("i",memushort(cvt->data,cvt->len,2*i)));

return( (PyObject *) ret );
}

static int PyFFCvt_SliceAssign( PyObject *_self, Py_ssize_t start, Py_ssize_t end, PyObject *rpl ) {
    PyFF_Cvt *c = (PyFF_Cvt *) _self;
    struct ttf_table *cvt;
    int len, i;

    cvt = c->cvt;
    if ( cvt==NULL || end<start || end <0 || 2*start>=cvt->len ) {
	PyErr_Format(PyExc_ValueError, "Slice specification out of range" );
return( -1 );
    }

    len = end-start + 1;

    if ( len!=PySequence_Size(rpl) ) {
	if ( !PyErr_Occurred())
	    PyErr_Format(PyExc_ValueError, "Replacement is different size than slice" );
return( -1 );
    }
    for ( i=start; i<=end; ++i ) {
	memputshort(cvt->data,sizeof(uint16_t)*i,
		PyLong_AsLong(PySequence_GetItem(rpl,i-start)));
	if ( PyErr_Occurred())
return( -1 );
    }
return( 0 );
}

static int PyFFCvt_Contains(PyObject *_self, PyObject *_val) {
    PyFF_Cvt *c = (PyFF_Cvt *) _self;
    struct ttf_table *cvt;
    size_t i;
    int val;

    val = PyLong_AsLong(_val);
    if ( PyErr_Occurred())
return( -1 );

    cvt = c->cvt;
    if ( cvt==NULL )
return( 0 );

    for ( i=0; i<cvt->len/2; ++i )
	if ( memushort(cvt->data,cvt->len,2*i)==val )
return( 1 );

return( 0 );
}

static PySequenceMethods PyFFCvt_Sequence = {
    PyFFCvt_Length,		/* length */
    PyFFCvt_Concat,		/* concat */
    NULL,			/* repeat */
    PyFFCvt_Index,		/* subscript */
    PyFFCvt_Slice,		/* slice */
    PyFFCvt_IndexAssign,	/* subscript assign */
    PyFFCvt_SliceAssign,	/* slice assign */
    PyFFCvt_Contains,		/* contains */
    PyFFCvt_InPlaceConcat,	/* inplace_concat */
    NULL			/* inplace repeat */
};

static PyObject *PyFFCvt_find(PyObject *self, PyObject *args) {
    PyFF_Cvt *c = (PyFF_Cvt *) self;
    struct ttf_table *cvt = c->cvt;
    int val, low=0;
    size_t high, i;

    if ( cvt==NULL )
return( Py_BuildValue("i", -1 ));

    high=cvt->len/2;

    if ( !PyArg_ParseTuple(args,"i|ii", &val, &low, &high ) )
return( NULL );
    if ( low<0 ) low=0;
    if ( high>cvt->len/2 ) high = cvt->len/2;
    for ( i=low; i<high; ++i )
	if ( (short) memushort(cvt->data,cvt->len,2*i)==val )
return( Py_BuildValue("i", i ));

return( Py_BuildValue("i", -1 ));
}

static PyGetSetDef PyFFCvt_getset[] = {
    {(char *)"font",
     (getter)PyFFCvt_get_font, NULL,
     (char *)"returns the font for this object", NULL},
    PYGETSETDEF_EMPTY /* Sentinel */
};

static PyMethodDef PyFFCvt_methods[] = {
    { (char *)"find", PyFFCvt_find, METH_VARARGS, (char *)"Finds the index in the cvt table of the specified value" },
    PYMETHODDEF_EMPTY /* Sentinel */
};

static PyTypeObject PyFF_CvtType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "fontforge.cvt",           /*tp_name*/
    sizeof(PyFF_Cvt),          /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)PyFFCvt_dealloc, /*tp_dealloc*/
    0,                         /*tp_vectorcall_offset*/
    NULL,                      /*tp_getattr*/
    NULL,                      /*tp_setattr*/
    NULL,                      /*tp_compare*/
    NULL,                      /*tp_repr*/
    NULL,                      /*tp_as_number*/
    &PyFFCvt_Sequence,         /*tp_as_sequence*/
    NULL,                      /*tp_as_mapping*/
    NULL,                      /*tp_hash */
    NULL,                      /*tp_call*/
    (reprfunc) PyFFCvt_Str,    /*tp_str*/
    NULL,                      /*tp_getattro*/
    NULL,                      /*tp_setattro*/
    NULL,                      /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    "fontforge cvt (control value table) objects", /* tp_doc */
    NULL,                      /* tp_traverse */
    NULL,                      /* tp_clear */
    NULL,                      /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    cvtiter_new,               /* tp_iter */
    NULL,                      /* tp_iternext */
    PyFFCvt_methods,           /* tp_methods */
    NULL,                      /* tp_members */
    PyFFCvt_getset,            /* tp_getset */
    NULL,                      /* tp_base */
    NULL,                      /* tp_dict */
    NULL,                      /* tp_descr_get */
    NULL,                      /* tp_descr_set */
    0,                         /* tp_dictoffset */
    NULL,                      /* tp_init */
    NULL,                      /* tp_alloc */
    NULL,                      /* tp_new */
    NULL,                      /* tp_free */
    NULL,                      /* tp_is_gc */
    NULL,                      /* tp_bases */
    NULL,                      /* tp_mro */
    NULL,                      /* tp_cache */
    NULL,                      /* tp_subclasses */
    NULL,                      /* tp_weaklist */
    NULL,                      /* tp_del */
    0,                         /* tp_version_tag */
};

/* ************************************************************************** */
/* Selection Standard Methods */
/* ************************************************************************** */

static PyTypeObject PyFF_SelectionType;

static void PyFFSelection_dealloc(PyFF_Selection *self) {
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *PyFFSelection_new(PyFF_Font *owner) {
    PyFF_Selection *self;

    if ( CheckIfFontClosed(owner) )
return(NULL);

    if ( owner->selection != NULL )
Py_RETURN( owner->selection );

    self = PyObject_New(PyFF_Selection, &PyFF_SelectionType);
    self->fv = owner->fv;
    owner->selection = self;
    self->by_glyphs = 0;
Py_RETURN( self );
}

static PyObject *PyFFSelection_Str(PyFF_Selection *self) {
return( PyUnicode_FromFormat( "<Selection for %s>", self->fv->sf->fontname ));
}

static PyObject *PyFFSelection_get_font(PyFF_Selection *self, void *UNUSED(closure)) {
    PyObject* font = PyFF_FontForFV_I( self->fv );
    if ( font==NULL )
Py_RETURN_NONE;
    return font;
}

static PyObject *PyFFSelection_ByGlyphs(PyFF_Selection *real_selection, void *UNUSED(closure)) {
    PyFF_Selection *self;

    self = PyObject_New(PyFF_Selection, &PyFF_SelectionType);
    self->fv = real_selection->fv;
    self->by_glyphs=1;
Py_RETURN( self );
}

/* ************************************************************************** */
/* Font Selection based methods */
/* ************************************************************************** */

enum { sel_more=1, sel_less=2,
	sel_unicode=4, sel_encoding=8,
	sel_singletons=16, sel_ranges=32 };
struct flaglist select_flags[] = {
    { "more", sel_more },
    { "less", sel_less },
    { "unicode", sel_unicode },
    { "encoding", sel_encoding },
    { "singletons", sel_singletons },
    { "ranges", sel_ranges },
    FLAGLIST_EMPTY /* Sentinel */
};

static PyObject *PyFFSelection_All(PyObject *self, PyObject *UNUSED(args)) {
    FontViewBase *fv = ((PyFF_Selection *) self)->fv;
    int i;

    for ( i=0; i<fv->map->enccount; ++i )
	fv->selected[i] = true;
Py_RETURN(self);
}

static PyObject *PyFFSelection_None(PyObject *self, PyObject *UNUSED(args)) {
    FontViewBase *fv = ((PyFF_Selection *) self)->fv;
    int i;

    for ( i=0; i<fv->map->enccount; ++i )
	fv->selected[i] = false;
Py_RETURN(self);
}

static PyObject *PyFFSelection_Changed(PyObject *self, PyObject *UNUSED(args)) {
    FontViewBase *fv = ((PyFF_Selection *) self)->fv;
    int i, gid;

    for ( i=0; i<fv->map->enccount; ++i ) {
	if ( (gid=fv->map->map[i])!=-1 && fv->sf->glyphs[gid]!=NULL )
	    fv->selected[i] = fv->sf->glyphs[gid]->changed;
	else
	    fv->selected[i] = false;
    }
Py_RETURN(self);
}

static PyObject *PyFFSelection_Invert(PyObject *self, PyObject *UNUSED(args)) {
    FontViewBase *fv = ((PyFF_Selection *) self)->fv;
    int i;

    for ( i=0; i<fv->map->enccount; ++i )
	fv->selected[i] = !fv->selected[i];
Py_RETURN(self);
}

static int SelIndex(PyObject *arg, FontViewBase *fv, int ints_as_unicode) {
    int enc;

    if ( PyUnicode_Check(arg)) {
	const char *name = PyUnicode_AsUTF8(arg);
	if (name == NULL) {
	    return -1;
	}
	enc = SFFindSlot(fv->sf, fv->map, -1, name );
    } else if ( PyLong_Check(arg)) {
	enc = PyLong_AsLong(arg);
	if ( ints_as_unicode )
	    enc = SFFindSlot(fv->sf, fv->map, enc, NULL );
    } else if ( PyType_IsSubtype(&PyFF_GlyphType, Py_TYPE(arg)) ) {
	SplineChar *sc = ((PyFF_Glyph *) arg)->sc;
	if ( sc->parent == fv->sf )
	    enc = fv->map->backmap[sc->orig_pos];
	else
	    enc = SFFindSlot(fv->sf, fv->map, sc->unicodeenc, sc->name );
    } else {
	PyErr_Format(PyExc_TypeError, "Unexpected argument type");
return( -1 );
    }
    if ( enc<0 || enc>=fv->map->enccount ) {
	PyErr_Format(PyExc_ValueError, "Encoding is out of range" );
return( -1 );
    }
return( enc );
}

static void FVBDeselectAll(FontViewBase *fv) {
    memset( fv->selected,0,fv->map->enccount);
}

static PyObject *PyFFSelection_select(PyObject *self, PyObject *args) {
    FontViewBase *fv = ((PyFF_Selection *) self)->fv;
    int flags = sel_encoding|sel_singletons;
    int i, j, cnt = PyTuple_Size(args);
    int range_started = false, range_first = -1;
    int enc;

    for ( i=0; i<cnt; ++i ) {
	PyObject *arg = PyTuple_GetItem(args,i);
	if ( !PyUnicode_Check(arg) && PySequence_Check(arg)) {
	    int newflags = FlagsFromTuple(arg,select_flags,"select flag");
	    if ( newflags==FLAG_UNKNOWN )
return( NULL );
	    if ( (newflags&(sel_more|sel_less)) == 0 )
		newflags |= (flags&(sel_more|sel_less));
	    if ( (newflags&(sel_unicode|sel_encoding)) == 0 )
		newflags |= (flags&(sel_unicode|sel_encoding));
	    if ( (newflags&(sel_singletons|sel_ranges)) == 0 )
		newflags |= (flags&(sel_singletons|sel_ranges));
	    flags = newflags;
	    if ( i==0 && (flags&(sel_more|sel_less)) == 0 )
		FVBDeselectAll(fv);
	    range_started = false;
	} else {
	    if ( i==0 )
		FVBDeselectAll(fv);
	    enc = SelIndex(arg,fv,flags&sel_unicode);
	    if ( enc==-1 )
return( NULL );
	    if ( flags&sel_less )
		fv->selected[enc] = 0;
	    else
		fv->selected[enc] = 1;
	    if ( flags&sel_ranges ) {
		if ( !range_started ) {
		    range_started = true;
		    range_first = enc;
		} else {
		    if ( range_first>enc ) {
			for ( j=enc; j<=range_first; ++j )
			    fv->selected[j] = (flags&sel_less)?0:1;
		    } else {
			for ( j=range_first; j<=enc; ++j )
			    fv->selected[j] = (flags&sel_less)?0:1;
		    }
		}
	    }
	}
    }

Py_RETURN(self);
}

static PyObject *fontiter_New(PyFF_Font *font, int bysel, struct searchdata *sv);

static PyObject *PySelection_iter(PyObject *object) {
    PyFF_Selection* self = (PyFF_Selection*)object;
    PyFF_Font* font = (PyFF_Font*)PyFF_FontForFV(self->fv);

    if ( CheckIfFontClosed(font) )
return (NULL);

    /* FontIter with either 1 => encodings of selected glyphs, or 2 => selected glyphs */
return( fontiter_New(font, self->by_glyphs + 1, NULL ));
}

static PyMethodDef PyFFSelection_methods[] = {
    { "select", PyFFSelection_select, METH_VARARGS, "Selects glyphs in the font" },
    { "all", PyFFSelection_All, METH_NOARGS, "Selects all glyphs in the font" },
    { "none", PyFFSelection_None, METH_NOARGS, "Deselects everything" },
    { "changed", PyFFSelection_Changed, METH_NOARGS, "Selects those glyphs which have changed" },
    { "invert", PyFFSelection_Invert, METH_NOARGS, "Inverts the selection" },
    PYMETHODDEF_EMPTY /* Sentinel */
};

static PyGetSetDef PyFFSelection_getset[] = {
    {(char *)"font",
     (getter)PyFFSelection_get_font, NULL,
     (char *)"returns the font for which this is a selection", NULL},
    {(char *)"byGlyphs",
     (getter)PyFFSelection_ByGlyphs, NULL,
     (char *)"returns a selection object whose iterator will return glyph objects (rather than encoding indices)", NULL},
    PYGETSETDEF_EMPTY  /* Sentinel */
};

/* ************************************************************************** */
/* Selection mapping */
/* ************************************************************************** */

static Py_ssize_t PyFFSelection_Length( PyObject *self ) {
return( ((PyFF_Selection *) self)->fv->map->enccount );
}

static PyObject *PyFFSelection_Index( PyObject *self, PyObject *index ) {
    PyFF_Selection *c = (PyFF_Selection *) self;
    PyObject *ret;
    int pos;

    pos = SelIndex(index,c->fv,false);
    if ( pos==-1 )
return( NULL );

    ret = c->fv->selected[pos] ? Py_True : Py_False;
    Py_INCREF( ret );
return( ret );
}

static int PyFFSelection_IndexAssign( PyObject *self, PyObject *index, PyObject *value ) {
    PyFF_Selection *c = (PyFF_Selection *) self;
    int val;
    int pos, cnt;

    if ( PySequence_Check(index)) {
	cnt = PySequence_Size(index);
	for ( pos=0; pos<cnt; ++pos )
	    if ( PyFFSelection_IndexAssign(self,PySequence_GetItem(index,pos),value)==-1 )
return( -1 );

return( 0 );
    }

    pos = SelIndex(index,c->fv,false);
    if ( pos==-1 )
return( -1 );

    if ( value==Py_True )
	val = 1;
    else if ( value==Py_False )
	val = 0;
    else {
	val = PyLong_AsLong(value);
	if ( PyErr_Occurred())
return( -1 );
    }
    c->fv->selected[pos] = val;
return( 0 );
}

static PyMappingMethods PyFFSelection_Mapping = {
    PyFFSelection_Length,		/* length */
    PyFFSelection_Index,		/* subscript */
    PyFFSelection_IndexAssign		/* subscript assign */
};

static PyTypeObject PyFF_SelectionType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "fontforge.selection",     /*tp_name*/
    sizeof(PyFF_Selection),    /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)PyFFSelection_dealloc, /*tp_dealloc*/
    0,                         /*tp_vectorcall_offset*/
    NULL,                      /*tp_getattr*/
    NULL,                      /*tp_setattr*/
    NULL,                      /*tp_compare*/
    NULL,                      /*tp_repr*/
    NULL,                      /*tp_as_number*/
    NULL,                      /*tp_as_sequence*/
    &PyFFSelection_Mapping,    /*tp_as_mapping*/
    NULL,                      /*tp_hash */
    NULL,                      /*tp_call*/
    (reprfunc)PyFFSelection_Str, /*tp_str*/
    NULL,                      /*tp_getattro*/
    NULL,                      /*tp_setattro*/
    NULL,                      /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    "fontforge selection objects", /* tp_doc */
    NULL,                      /* tp_traverse */
    NULL,                      /* tp_clear */
    NULL,                      /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    PySelection_iter,          /* tp_iter */
    NULL,                      /* tp_iternext */
    PyFFSelection_methods,     /* tp_methods */
    NULL,                      /* tp_members */
    PyFFSelection_getset,      /* tp_getset */
    NULL,                      /* tp_base */
    NULL,                      /* tp_dict */
    NULL,                      /* tp_descr_get */
    NULL,                      /* tp_descr_set */
    0,                         /* tp_dictoffset */
    NULL,                      /* tp_init */
    NULL,                      /* tp_alloc */
    NULL,                      /* tp_new */
    NULL,                      /* tp_free */
    NULL,                      /* tp_is_gc */
    NULL,                      /* tp_bases */
    NULL,                      /* tp_mro */
    NULL,                      /* tp_cache */
    NULL,                      /* tp_subclasses */
    NULL,                      /* tp_weaklist */
    NULL,                      /* tp_del */
    0,                         /* tp_version_tag */
};


/* ************************************************************************** */
/* Layers info array iterator type */
/* ************************************************************************** */

static PyTypeObject PyFF_LayerInfoType;
static PyTypeObject PyFF_LayerInfoArrayType;

typedef struct {
	PyObject_HEAD
	PyFF_LayerInfoArray *layers;
	int pos;
} layerinfoiterobject;
static PyTypeObject PyFF_LayerInfoArrayIterType;

static PyObject *layerinfoiter_new(PyObject *layers) {
    layerinfoiterobject *di;
    di = PyObject_New(layerinfoiterobject, &PyFF_LayerInfoArrayIterType);
    if (di == NULL)
return NULL;
    Py_INCREF(layers);
    di->layers = (PyFF_LayerInfoArray *) layers;
    di->pos = 0;
return (PyObject *)di;
}

static void layerinfoiter_dealloc(layerinfoiterobject *di) {
    Py_XDECREF(di->layers);
    PyObject_Del(di);
}

static PyObject *layerinfoiter_iternextkey(layerinfoiterobject *di) {
    PyFF_LayerInfoArray *d = di->layers;
    SplineFont *sf;

    if (d == NULL )
return NULL;
    sf = d->sf;

    if ( di->pos<sf->layer_cnt )
return( Py_BuildValue("s",sf->layers[di->pos++].name) );

return NULL;
}

static PyTypeObject PyFF_LayerInfoArrayIterType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "fontforge.layerinfo_array_iterator", /* tp_name */
    sizeof(layerinfoiterobject), /* tp_basicsize */
    0,                         /* tp_itemsize */
    /* methods */
    (destructor)layerinfoiter_dealloc, /* tp_dealloc */
    0,                         /* tp_vectorcall_offset */
    NULL,                      /* tp_getattr */
    NULL,                      /* tp_setattr */
    NULL,                      /* tp_compare */
    NULL,                      /* tp_repr */
    NULL,                      /* tp_as_number */
    NULL,                      /* tp_as_sequence */
    NULL,                      /* tp_as_mapping */
    NULL,                      /* tp_hash */
    NULL,                      /* tp_call */
    NULL,                      /* tp_str */
    NULL,                      /* tp_getattro */
    NULL,                      /* tp_setattro */
    NULL,                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    NULL,                      /* tp_doc */
    NULL,                      /* tp_traverse */
    NULL,                      /* tp_clear */
    NULL,                      /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    PyObject_SelfIter,         /* tp_iter */
    (iternextfunc)layerinfoiter_iternextkey, /* tp_iternext */
    NULL,                      /* tp_methods */
    NULL,                      /* members */
    PyFFSelection_getset,      /* tp_getset */
    NULL,                      /* tp_base */
    NULL,                      /* tp_dict */
    NULL,                      /* tp_descr_get */
    NULL,                      /* tp_descr_set */
    0,                         /* tp_dictoffset */
    NULL,                      /* tp_init */
    NULL,                      /* tp_alloc */
    NULL,                      /* tp_new */
    NULL,                      /* tp_free */
    NULL,                      /* tp_is_gc */
    NULL,                      /* tp_bases */
    NULL,                      /* tp_mro */
    NULL,                      /* tp_cache */
    NULL,                      /* tp_subclasses */
    NULL,                      /* tp_weaklist */
    NULL,                      /* tp_del */
    0,                         /* tp_version_tag */
};

/* ************************************************************************** */
/* Layer Info Standard Methods */
/* ************************************************************************** */

static void PyFF_LayerInfo_dealloc(PyFF_LayerInfo *self) {
    self->sf = NULL;
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *PyFFLayerInfo_Str(PyFF_LayerInfo *self) {
return( PyUnicode_FromFormat( "<LayerInfo %s,%d>",
	self->sf->layers[self->layer].name,
	self->sf->layers[self->layer].order2));
}

static PyObject *PyFF_LayerInfo_get_font(PyFF_LayerInfo *self, void *UNUSED(closure)) {
    PyFF_Font *font = PyFF_FontForSF( self->sf );
    if ( font==NULL )
Py_RETURN_NONE;
    Py_INCREF(font);
    return (PyObject*)font;
}

static PyObject *PyFF_LayerInfo_get_name(PyFF_LayerInfo *self, void *UNUSED(closure)) {
return( Py_BuildValue("s",self->sf->layers[self->layer].name));
}

static int PyFF_LayerInfo_set_name(PyFF_LayerInfo *self,PyObject *value, void *UNUSED(closure)) {
    char *name = copy(PyUnicode_AsUTF8(value));
    if (!name) {
        PyErr_Format(PyExc_TypeError,"Expected layer name");
        return -1;
    }
    free(self->sf->layers[self->layer].name);
    self->sf->layers[self->layer].name = name;
    return 0;
}

static PyObject *PyFF_LayerInfo_get_order2(PyFF_LayerInfo *self, void *UNUSED(closure)) {
return( Py_BuildValue("i",self->sf->layers[self->layer].order2));
}

static int PyFF_LayerInfo_set_order2(PyFF_LayerInfo *self,PyObject *value, void *UNUSED(closure)) {
    if ( PyLong_Check(value)) {
	int val = PyLong_AsLong(value)!=0;
	SplineFont *sf = self->sf;
	int layer = self->layer;
	if ( sf->layers[layer].order2!=val ) {
	    if ( val )
		SFConvertLayerToOrder2(sf,layer);
	    else
		SFConvertLayerToOrder3(sf,layer);
	}
return(0);
    }
    PyErr_Format(PyExc_TypeError,"Expected boolean value");
return( -1 );
}

static PyObject *PyFF_LayerInfo_get_background(PyFF_LayerInfo *self, void *UNUSED(closure)) {
return( Py_BuildValue("i",self->sf->layers[self->layer].background));
}

static int PyFF_LayerInfo_set_background(PyFF_LayerInfo *self,PyObject *value, void *UNUSED(closure)) {
    if ( PyLong_Check(value)) {
	int val = PyLong_AsLong(value)!=0;
	SplineFont *sf = self->sf;
	int layer = self->layer;
	if ( val!=sf->layers[layer].background )
	    SFLayerSetBackground(sf,layer,val);
return(0);
    }
    PyErr_Format(PyExc_TypeError,"Expected boolean value");
return( -1 );
}

static PyGetSetDef PyFF_LayerInfo_getset[] = {
    {(char *)"font",
     (getter)PyFF_LayerInfo_get_font, NULL,
     (char *)"returns the font to which this object belongs", NULL},
    {(char *)"name",
     (getter)PyFF_LayerInfo_get_name, (setter)PyFF_LayerInfo_set_name,
     (char *)"arbitrary (non-persistent) user data (deprecated name for temporary)", NULL},
    {(char *)"is_quadratic",
     (getter)PyFF_LayerInfo_get_order2, (setter)PyFF_LayerInfo_set_order2,
     (char *)"Does this layer contain quadratic or cubic splines (TrueType or PostScript)", NULL},
    {(char *)"is_background",
     (getter)PyFF_LayerInfo_get_background, (setter)PyFF_LayerInfo_set_background,
     (char *)"Is this a background layer or a foreground one?\nForeground layers may have fonts generated from them.\nBackground layers may contain background images", NULL},
    PYGETSETDEF_EMPTY /* Sentinel */
};

static PyTypeObject PyFF_LayerInfoType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "fontforge.layerinfo",     /* tp_name */
    sizeof(PyFF_LayerInfo),    /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor) PyFF_LayerInfo_dealloc, /* tp_dealloc */
    0,                         /* tp_vectorcall_offset */
    NULL,                      /* tp_getattr */
    NULL,                      /* tp_setattr */
    NULL,                      /* tp_compare */
    NULL,                      /* tp_repr */
    NULL,                      /* tp_as_number */
    NULL,                      /* tp_as_sequence */
    NULL,                      /* tp_as_mapping */
    NULL,                      /* tp_hash */
    NULL,                      /* tp_call */
    (reprfunc) PyFFLayerInfo_Str, /* tp_str */
    NULL,                      /* tp_getattro */
    NULL,                      /* tp_setattro */
    NULL,                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    "FontForge layer info",    /* tp_doc */
    NULL,                      /* tp_traverse */
    NULL,                      /* tp_clear */
    NULL,                      /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    NULL,                      /* tp_iter */
    NULL,                      /* tp_iternext */
    NULL,                      /* tp_methods */
    NULL,                      /* tp_members */
    PyFF_LayerInfo_getset,     /* tp_getset */
    NULL,                      /* tp_base */
    NULL,                      /* tp_dict */
    NULL,                      /* tp_descr_get */
    NULL,                      /* tp_descr_set */
    0,                         /* tp_dictoffset */
    NULL,                      /* tp_init */
    NULL,                      /* tp_alloc */
    NULL,                      /* tp_new */
    NULL,                      /* tp_free */
    NULL,                      /* tp_is_gc */
    NULL,                      /* tp_bases */
    NULL,                      /* tp_mro */
    NULL,                      /* tp_cache */
    NULL,                      /* tp_subclasses */
    NULL,                      /* tp_weaklist */
    NULL,                      /* tp_del */
    0,                         /* tp_version_tag */
};

/* ************************************************************************** */
/* Layers Info Array Standard Methods */
/* ************************************************************************** */

static void PyFF_LayerInfoArray_dealloc(PyFF_LayerInfoArray *self) {
    self->sf = NULL;
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *PyFFLayerInfoArray_Str(PyFF_LayerInfoArray *self) {
return( PyUnicode_FromFormat( "<Layer Info Array for %s>", self->sf->fontname ));
}

static PyObject *PyFFLayerInfoArray_get_font(PyFF_LayerInfoArray *self, void *UNUSED(closure)) {
    PyFF_Font *font = PyFF_FontForSF( self->sf );
    if ( font==NULL )
Py_RETURN_NONE;
    Py_INCREF(font);
    return (PyObject*)font;
}

/* ************************************************************************** */
/* ****************************** Layers Array ****************************** */
/* ************************************************************************** */

static Py_ssize_t PyFF_LayerInfoArrayLength( PyObject *self ) {
    SplineFont *sf = ((PyFF_LayerInfoArray *) self)->sf;
    if ( sf==NULL )
return( 0 );
    else
return( sf->layer_cnt );
}

static PyObject *PyFF_LayerInfoArrayIndex( PyObject *self, PyObject *index ) {
    SplineFont *sf = ((PyFF_LayerInfoArray *) self)->sf;
    int layer;
    PyFF_LayerInfo *li;

    if ( PyUnicode_Check(index)) {
	const char *name = PyUnicode_AsUTF8(index);
	if (name == NULL) {
	    return NULL;
	}
	layer = SFFindLayerIndexByName(sf,name);
	if ( layer<0 )
return( NULL );
    } else if ( PyLong_Check(index)) {
	layer = PyLong_AsLong(index);
    } else {
	PyErr_Format(PyExc_TypeError, "Index must be a layer name or index" );
return( NULL );
    }
    if ( layer<0 || layer>=sf->layer_cnt ) {
	PyErr_Format(PyExc_ValueError, "Layer is out of range" );
return( NULL );
    }
    li = PyObject_New(PyFF_LayerInfo, &PyFF_LayerInfoType);
    li->sf = sf;
    li->layer = layer;
return( (PyObject *) li );
}

static int PyFF_LayerInfoArrayIndexAssign( PyObject *self, PyObject *index, PyObject *value ) {
    SplineFont *sf = ((PyFF_LayerInfoArray *) self)->sf;
    int layer, order2;
    char *name;

    if ( PyUnicode_Check(index)) {
	const char *name = PyUnicode_AsUTF8(index);
	if (name == NULL) {
	    return -1;
	}
	layer = SFFindLayerIndexByName(sf,name);
	if ( layer<0 )
return( -1 );
    } else if ( PyLong_Check(index)) {
	layer = PyLong_AsLong(index);
    } else {
	PyErr_Format(PyExc_TypeError, "Index must be a layer name or index" );
return( -1 );
    }
    if ( layer<0 || layer>=sf->layer_cnt ) {
	PyErr_Format(PyExc_ValueError, "Layer is out of range" );
return( -1 );
    }
    if ( value==NULL ) {
	if ( layer>ly_fore )
	    SFRemoveLayer(sf,layer);
	else {
	    PyErr_Format(PyExc_ValueError, "You may not delete the background or foreground layers" );
return( -1 );
	}
return( 0 );
    } else if ( !PyArg_ParseTuple(value,"si", &name, &order2 ) )
return( -1 );
    free(sf->layers[layer].name);
    sf->layers[layer].name = copy(name);
    order2 = order2!=0;
    if ( sf->layers[layer].order2!=order2 ) {
	if ( sf->layers[layer].order2!=order2 ) {
	    if ( order2 )
		SFConvertLayerToOrder2(sf,layer);
	    else
		SFConvertLayerToOrder3(sf,layer);
	}
    }
return( 0 );
}

static PyMappingMethods PyFF_LayerInfoArrayMapping = {
    PyFF_LayerInfoArrayLength,		/* length */
    PyFF_LayerInfoArrayIndex,		/* subscript */
    PyFF_LayerInfoArrayIndexAssign	/* subscript assign */
};

static PyObject *PyFF_LayerInfoArray_add(PyObject *self, PyObject *args) {
    SplineFont *sf = ((PyFF_LayerInfoArray *) self)->sf;
    int order2, background=0;
    char *name;

    if ( !PyArg_ParseTuple(args,"si|i", &name, &order2, &background ) )
return( NULL );
    SFAddLayer(sf,name,order2,background);
	CVLayerPaletteCheck(sf);
Py_RETURN(self);
}

static PyGetSetDef PyFF_LayerInfoArray_getset[] = {
    {(char *)"font",
     (getter)PyFFLayerInfoArray_get_font, NULL,
     (char *)"returns the font for this object", NULL},
    PYGETSETDEF_EMPTY  /* Sentinel */
};

static PyMethodDef PyFF_LayerInfoArray_methods[] = {
    { "add", PyFF_LayerInfoArray_add, METH_VARARGS, "Adds a new layer to the font" },
    PYMETHODDEF_EMPTY /* Sentinel */
};

static PyTypeObject PyFF_LayerInfoArrayType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "fontforge.layerinfo_array",/*tp_name*/
    sizeof(PyFF_LayerInfoArray), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor) PyFF_LayerInfoArray_dealloc, /*tp_dealloc*/
    0,                         /*tp_vectorcall_offset*/
    NULL,                      /*tp_getattr*/
    NULL,                      /*tp_setattr*/
    NULL,                      /*tp_compare*/
    NULL,                      /*tp_repr*/
    NULL,                      /*tp_as_number*/
    NULL,                      /*tp_as_sequence*/
    &PyFF_LayerInfoArrayMapping, /*tp_as_mapping*/
    NULL,                      /*tp_hash */
    NULL,                      /*tp_call*/
    (reprfunc) PyFFLayerInfoArray_Str, /*tp_str*/
    NULL,                      /*tp_getattro*/
    NULL,                      /*tp_setattro*/
    NULL,                      /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    "FontForge layers array",  /* tp_doc */
    NULL,                      /* tp_traverse */
    NULL,                      /* tp_clear */
    NULL,                      /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    layerinfoiter_new,         /* tp_iter */
    NULL,                      /* tp_iternext */
    PyFF_LayerInfoArray_methods, /* tp_methods */
    NULL,                      /* tp_members */
    PyFF_LayerInfoArray_getset, /* tp_getset */
    NULL,                      /* tp_base */
    NULL,                      /* tp_dict */
    NULL,                      /* tp_descr_get */
    NULL,                      /* tp_descr_set */
    0,                         /* tp_dictoffset */
    NULL,                      /* tp_init */
    NULL,                      /* tp_alloc */
    NULL,                      /* tp_new */
    NULL,                      /* tp_free */
    NULL,                      /* tp_is_gc */
    NULL,                      /* tp_bases */
    NULL,                      /* tp_mro */
    NULL,                      /* tp_cache */
    NULL,                      /* tp_subclasses */
    NULL,                      /* tp_weaklist */
    NULL,                      /* tp_del */
    0,                         /* tp_version_tag */
};

/* ************************************************************************** */
/* Font math constants type */
/* ************************************************************************** */

static void PyFFMath_dealloc(PyFF_Math *self) {
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *PyFFMath_Str(PyFF_Math *self) {
return( PyUnicode_FromFormat( "<math table for font %s>", self->sf->fontname ));
}

static struct MATH *SFGetMathTable(SplineFont *sf) {
    if ( sf->cidmaster!=NULL )
	sf = sf->cidmaster;
    if ( sf->MATH==NULL )
	sf->MATH = MathTableNew(sf);
return(sf->MATH);
}

static PyObject *PyFFMath_get(PyFF_Math *self, void *closure) {
    int offset = (int) (intptr_t) closure;
    struct MATH *math;

    math = SFGetMathTable(self->sf);
	/* some entries are unsigned, but I don't know which here */
return( Py_BuildValue("i", *(int16_t *) (((char *) math) + offset) ));
}

static int PyFFMath_set(PyFF_Math *self, PyObject *value, void *closure) {
    int offset = (int) (intptr_t) closure;
    struct MATH *math;
    long val;

    math = SFGetMathTable(self->sf);
    val = PyLong_AsLong(value);
    if ( val==-1 && PyErr_Occurred())
return( -1 );
    if ( val<-32768 || val>65535 ) {
	PyErr_Format(PyExc_ValueError, "The math table constants must have 16 bit values, but this (%ld) is out of range", val);
return( -1 );
    }
    *(int16_t *) (((char *) math) + offset) = val;
return( 0 );
}

static PyObject *PyFFMath_clear(PyFF_Math *self, PyObject *UNUSED(args)) {
    SplineFont *sf = self->sf;
    if ( sf->cidmaster!=NULL )
	sf = sf->cidmaster;
    if ( sf->MATH!=NULL ) {
	MATHFree(sf->MATH);
	sf->MATH = NULL;
    }
Py_RETURN( self );
}

static PyObject *PyFFMath_exists(PyFF_Math *self, PyObject *UNUSED(args)) {
    PyObject *ret;
    SplineFont *sf = self->sf;
    if ( sf->cidmaster!=NULL )
	sf = sf->cidmaster;
    ret = self->sf->MATH!=NULL ? Py_True : Py_False;
    Py_INCREF( ret );
return( ret );
}

static PyMethodDef FFMath_methods[] = {
    {"clear", (PyCFunction)PyFFMath_clear, METH_NOARGS,
	     "Removes any (underlying) math table from this font. Referencing a member will create the table again." },
    {"exists", (PyCFunction)PyFFMath_exists, METH_NOARGS,
	     "Returns whether there is an underlying table associated with this font. Referencing a member will create the table if it does not exist." },
    PYMETHODDEF_EMPTY /* Sentinel */
};

static PyTypeObject PyFF_MathType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "fontforge.math",	       /* tp_name */
    sizeof(PyFF_Math),         /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)PyFFMath_dealloc, /*tp_dealloc */
    0,                         /* tp_vectorcall_offset */
    NULL,                      /* tp_getattr */
    NULL,                      /* tp_setattr */
    NULL,                      /* tp_compare */
    NULL,                      /* tp_repr */
    NULL,                      /* tp_as_number */
    NULL,                      /* tp_as_sequence */
    NULL,                      /* tp_as_mapping */
    NULL,                      /* tp_hash */
    NULL,                      /* tp_call */
    (reprfunc) PyFFMath_Str,   /* tp_str */
    NULL,                      /* tp_getattro */
    NULL,                      /* tp_setattro */
    NULL,                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    "fontforge math objects",  /* tp_doc */
    NULL,                      /* tp_traverse */
    NULL,                      /* tp_clear */
    NULL,                      /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    NULL,                      /* tp_iter */
    NULL,                      /* tp_iternext */
    FFMath_methods,            /* tp_methods */
    NULL,                      /* tp_members */
    NULL, /* Will build this when readying the type*/ /* tp_getset */
    NULL,                      /* tp_base */
    NULL,                      /* tp_dict */
    NULL,                      /* tp_descr_get */
    NULL,                      /* tp_descr_set */
    0,                         /* tp_dictoffset */
    NULL,                      /* tp_init */
    NULL,                      /* tp_alloc */
    NULL,                      /* tp_new */
    NULL,                      /* tp_free */
    NULL,                      /* tp_is_gc */
    NULL,                      /* tp_bases */
    NULL,                      /* tp_mro */
    NULL,                      /* tp_cache */
    NULL,                      /* tp_subclasses */
    NULL,                      /* tp_weaklist */
    NULL,                      /* tp_del */
    0,                         /* tp_version_tag */
};

/* Build a get/set table for the math type based on the math_constants_descriptor */
static int setup_math_type(PyTypeObject* mathtype) {
    int cnt;
    PyGetSetDef *getset;

    for ( cnt=0; math_constants_descriptor[cnt].script_name!=NULL; ++cnt );

    getset = calloc(cnt+1,sizeof(PyGetSetDef));
    for ( cnt=0; math_constants_descriptor[cnt].script_name!=NULL; ++cnt ) {
	getset[cnt].name = math_constants_descriptor[cnt].script_name;
	getset[cnt].get = (getter) PyFFMath_get;
	getset[cnt].set = (setter) PyFFMath_set;
	getset[cnt].doc = math_constants_descriptor[cnt].message;
	getset[cnt].closure = (void *) (intptr_t) math_constants_descriptor[cnt].offset;
    }
    mathtype->tp_getset = getset;
    return 0;
}

/* ************************************************************************** */
/* Private dictionary iterator type */
/* ************************************************************************** */

typedef struct {
	PyObject_HEAD
	PyFF_Private *private;
	int pos;
} privateiterobject;
static PyTypeObject PyFF_PrivateIterType;

static PyObject *privateiter_new(PyObject *private) {
    privateiterobject *di;
    di = PyObject_New(privateiterobject, &PyFF_PrivateIterType);
    if (di == NULL)
return NULL;
    Py_INCREF(private);
    di->private = (PyFF_Private *) private;
    di->pos = 0;
return (PyObject *)di;
}

static void privateiter_dealloc(privateiterobject *di) {
    Py_XDECREF(di->private);
    PyObject_Del(di);
}

static PyObject *privateiter_iternextkey(privateiterobject *di) {
    PyFF_Private *d = di->private;

    if (d == NULL || d->sf->private==NULL )
return NULL;

    if ( di->pos<d->sf->private->next )
return( Py_BuildValue("s",d->sf->private->keys[di->pos++]) );

return NULL;
}

static PyTypeObject PyFF_PrivateIterType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "fontforge.private_iterator",  /* tp_name */
    sizeof(privateiterobject), /* tp_basicsize */
    0,                         /* tp_itemsize */
    /* methods */
    (destructor)privateiter_dealloc, /* tp_dealloc */
    0,                         /* tp_vectorcall_offset */
    NULL,                      /* tp_getattr */
    NULL,                      /* tp_setattr */
    NULL,                      /* tp_compare */
    NULL,                      /* tp_repr */
    NULL,                      /* tp_as_number */
    NULL,                      /* tp_as_sequence */
    NULL,                      /* tp_as_mapping */
    NULL,                      /* tp_hash */
    NULL,                      /* tp_call */
    NULL,                      /* tp_str */
    NULL,                      /* tp_getattro */
    NULL,                      /* tp_setattro */
    NULL,                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    NULL,                      /* tp_doc */
    NULL,                      /* tp_traverse */
    NULL,                      /* tp_clear */
    NULL,                      /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    PyObject_SelfIter,         /* tp_iter */
    (iternextfunc)privateiter_iternextkey, /* tp_iternext */
    NULL,                      /* tp_methods */
    NULL,                      /* tp_members */
    NULL,                      /* tp_getset */
    NULL,                      /* tp_base */
    NULL,                      /* tp_dict */
    NULL,                      /* tp_descr_get */
    NULL,                      /* tp_descr_set */
    0,                         /* tp_dictoffset */
    NULL,                      /* tp_init */
    NULL,                      /* tp_alloc */
    NULL,                      /* tp_new */
    NULL,                      /* tp_free */
    NULL,                      /* tp_is_gc */
    NULL,                      /* tp_bases */
    NULL,                      /* tp_mro */
    NULL,                      /* tp_cache */
    NULL,                      /* tp_subclasses */
    NULL,                      /* tp_weaklist */
    NULL,                      /* tp_del */
    0,                         /* tp_version_tag */
};

/* ************************************************************************** */
/* Private Dict Standard Methods */
/* ************************************************************************** */

static void PyFF_Private_dealloc(PyFF_Private *self) {
    self->sf = NULL;
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *PyFFPrivate_Str(PyFF_Private *self) {
return( PyUnicode_FromFormat( "<Private Dictionary for %s>", self->sf->fontname ));
}

/* ************************************************************************** */
/* ************************** Private Dictionary **************************** */
/* ************************************************************************** */

static Py_ssize_t PyFF_PrivateLength( PyObject *self ) {
    struct psdict *private = ((PyFF_Private *) self)->sf->private;
    if ( private==NULL )
return( 0 );
    else
return( private->next );
}

static PyObject *PyFF_PrivateIndex( PyObject *self, PyObject *index ) {
    SplineFont *sf = ((PyFF_Private *) self)->sf;
    struct psdict *private = sf->private;
    char *value=NULL;
    char *pt, *end;
    double temp;
    PyObject *tuple;

    if ( PyUnicode_Check(index)) {
	const char *name = PyUnicode_AsUTF8(index);
	if (name == NULL) {
	    return NULL;
	}
	if ( private!=NULL )
	    value = PSDictHasEntry(private,name);
    } else {
	PyErr_Format(PyExc_TypeError, "Private dictionary index must be a string" );
return( NULL );
    }
    if ( value==NULL ) {
	PyErr_Format(PyExc_TypeError, "No such dictionary entry for specified index" );
return( NULL );
    }
    strtod(value,&end); while ( *end==' ' ) ++end;
    if ( *end=='\0' )
return( Py_BuildValue("d",strtod(value,NULL)) );

    if ( *value=='[' ) {
	int cnt = 0;
	pt = value+1;
	for (;;) {
	    strtod(pt,&end);
	    if ( pt==end )
	break;
	    ++cnt;
	    pt = end;
	}
	while ( *pt==' ' ) ++pt;
	if ( *pt==']' ) {
	    tuple = PyTuple_New(cnt);
	    cnt = 0;
	    pt = value+1;
	    for (;;) {
		temp = strtod(pt,&end);
		if ( pt==end )
	    break;
		PyTuple_SetItem(tuple,cnt++,Py_BuildValue("d",temp));
		pt = end;
	    }
return( tuple );
	}
    }
return( Py_BuildValue("s",value));
}

static int PyFF_PrivateIndexAssign( PyObject *self, PyObject *index, PyObject *value ) {
    SplineFont *sf = ((PyFF_Private *) self)->sf;
    struct psdict *private = sf->private;
    const char *string, *name;
    char *freeme = NULL;
    char buffer[40];

    if ( PyUnicode_Check(value)) {
        string = PyUnicode_AsUTF8(value);
        if (string == NULL) {
            return -1;
        }
    } else if ( PyFloat_Check(value)) {
        string = buffer;
        g_ascii_formatd(buffer, sizeof(buffer), "%g", PyFloat_AsDouble(value));
    } else if ( PyLong_Check(value)) {
        string = buffer;
        snprintf(buffer, sizeof(buffer), "%ld", PyLong_AsLong(value));
    } else if ( PySequence_Check(value)) {
	char *pt;
	int i, size = PySequence_Size(value);
	string = pt = freeme = malloc(size*21+4);
	*pt++ = '[';
	for ( i=0; i<size; ++i ) {
	    PyObject *obj = PySequence_GetItem(value, i);
	    g_ascii_formatd(pt, 21, "%g", PyFloat_AsDouble(obj));
	    Py_DECREF(obj);
	    pt += strlen(pt);
	    *pt++ = ' ';
	}
	if ( pt[-1]==' ' ) --pt;
	*pt++ = ']'; *pt = '\0';
    } else {
	PyErr_Format(PyExc_TypeError, "Assignment value must be string, float, integer or tuple" );
        return( -1 );
    }

    name = PyUnicode_AsUTF8(index);
    if (name == NULL) {
        PyErr_Format(PyExc_TypeError, "Private dictionary index must be a string" );
        free(freeme);
        return -1;
    } else if (private == NULL) {
        sf->private = private = calloc(1,sizeof(struct psdict));
    }

    PSDictChangeEntry(private,name,string);
    free(freeme);
    return( 0 );
}

static PyMappingMethods PyFF_PrivateMapping = {
    PyFF_PrivateLength,		/* length */
    PyFF_PrivateIndex,		/* subscript */
    PyFF_PrivateIndexAssign	/* subscript assign */
};

static PyObject *PyFFPrivate_Guess(PyFF_Private *self, PyObject *args) {
    SplineFont *sf = self->sf;
    char *name;

    if ( !PyArg_ParseTuple(args,"s", &name) )
return( NULL );
    if ( sf->private==NULL )
	sf->private = calloc(1,sizeof(struct psdict));

    SFPrivateGuess(sf,self->fv->active_layer,sf->private,name,true);
Py_RETURN( self );
}

static PyMethodDef FFPrivate_methods[] = {
    {"guess", (PyCFunction)PyFFPrivate_Guess, METH_VARARGS,
	     "Given the name of an entry, guesses a default value for it." },
    PYMETHODDEF_EMPTY /* Sentinel */
};

/* ************************************************************************** */
/* ************************* initializer routines *************************** */
/* ************************************************************************** */

static PyTypeObject PyFF_PrivateType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "fontforge.private",       /* tp_name */
    sizeof(PyFF_Private),      /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor) PyFF_Private_dealloc, /* tp_dealloc */
    0,                         /* tp_vectorcall_offset */
    NULL,                      /* tp_getattr */
    NULL,                      /* tp_setattr */
    NULL,                      /* tp_compare */
    NULL,                      /* tp_repr */
    NULL,                      /* tp_as_number */
    NULL,                      /* tp_as_sequence */
    &PyFF_PrivateMapping,      /* tp_as_mapping */
    NULL,                      /* tp_hash */
    NULL,                      /* tp_call */
    (reprfunc) PyFFPrivate_Str,/* tp_str */
    NULL,                      /* tp_getattro */
    NULL,                      /* tp_setattro */
    NULL,                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    "FontForge private dictionary", /* tp_doc */
    NULL,                      /* tp_traverse */
    NULL,                      /* tp_clear */
    NULL,                      /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    privateiter_new,           /* tp_iter */
    NULL,                      /* tp_iternext */
    FFPrivate_methods,         /* tp_methods */
    NULL,                      /* tp_members */
    NULL,                      /* tp_getset */
    NULL,                      /* tp_base */
    NULL,                      /* tp_dict */
    NULL,                      /* tp_descr_get */
    NULL,                      /* tp_descr_set */
    0,                         /* tp_dictoffset */
    NULL,                      /* tp_init */
    NULL,                      /* tp_alloc */
    NULL,                      /* tp_new */
    NULL,                      /* tp_free */
    NULL,                      /* tp_is_gc */
    NULL,                      /* tp_bases */
    NULL,                      /* tp_mro */
    NULL,                      /* tp_cache */
    NULL,                      /* tp_subclasses */
    NULL,                      /* tp_weaklist */
    NULL,                      /* tp_del */
    0,                         /* tp_version_tag */
};

/* ************************************************************************** */
/* Font iterator type */
/* ************************************************************************** */

typedef struct {
    PyObject_HEAD
    SplineFont *sf;
    int pos;
    int byselection;
    FontViewBase *fv;
    struct searchdata *sv;
} fontiterobject;
static PyTypeObject PyFF_FontIterType;

static PyObject *fontiter_New(PyFF_Font *self, int bysel, struct searchdata *sv) {
    fontiterobject *di;

    if ( CheckIfFontClosed(self) )
return( NULL );

    di = PyObject_New(fontiterobject, &PyFF_FontIterType);
    if (di == NULL)
return NULL;
    di->sf = self->fv->sf;
    di->fv = self->fv;
    di->pos = 0;
    di->byselection = bysel;
    di->sv = sv;
return (PyObject *)di;
}

static PyObject *fontiter_new_wholefont(PyObject *object) {
    PyFF_Font *self = (PyFF_Font*)object;
return( fontiter_New(self,false,NULL) );
}

static void fontiter_dealloc(fontiterobject *di) {
    PyObject_Del(di);
}

static PyObject *fontiter_iternextkey(fontiterobject *di) {
    if ( di->sv!=NULL ) {
	SplineChar *sc = SDFindNext(di->sv);
	if ( sc!=NULL ) {
	    const char *dictfmt = "{sKsKsK}";
	    PyObject *glyph, *tempdict;
	    PyObject *matched;
	    glyph = PySC_From_SC_I( sc );
	    /* Fill matched result into the glyph.temporary attribute. */
	    tempdict = PyFF_Glyph_get_temporary((PyFF_Glyph *)glyph, NULL);
	    if (tempdict == NULL || !PyDict_Check(tempdict)) {
		tempdict = PyDict_New();
		PyFF_Glyph_set_temporary((PyFF_Glyph *)glyph, tempdict,  NULL);
	    }

	    matched = Py_BuildValue((char *) dictfmt,
		    "findMatchedRefs", di->sv->matched_refs,
		    "findMatchedContours", di->sv->matched_ss,
		    "findMatchedContoursStart", di->sv->matched_ss_start
		    );
	    PyDict_Update(tempdict, matched);
	    Py_DECREF(tempdict);
	    Py_DECREF(matched);

return( glyph );
	}
    } else switch ( di->byselection ) {
      case 0: {		/* names of all glyphs in GID order */
	SplineFont *sf = di->sf;

	if (sf == NULL)
return NULL;

	while ( di->pos<sf->glyphcnt ) {
	    if ( sf->glyphs[di->pos]!=NULL )
return( Py_BuildValue("s",sf->glyphs[di->pos++]->name) );
	    ++di->pos;
	}
      break;}
      case 1: {		/* Encodings of selected glyphs (in encoding order) */
	FontViewBase *fv = di->fv;
	int enccount = fv->map->enccount;
	while ( di->pos < enccount ) {
	    if ( fv->selected[di->pos] )
return( Py_BuildValue("i",di->pos++ ) );
	    ++di->pos;
	}
      break;}
      case 2: {		/* Selected glyphs in encoding order */
	int gid;
	FontViewBase *fv = di->fv;
	int enccount = fv->map->enccount;
	while ( di->pos < enccount ) {
	    if ( fv->selected[di->pos] && (gid=fv->map->map[di->pos])!=-1 &&
		    SCWorthOutputting(fv->sf->glyphs[gid]) ) {
		++di->pos;
return( PySC_From_SC_I( fv->sf->glyphs[gid] ) );
	    }
	    ++di->pos;
	}
      break;}
      case 3: {		/* All glyphs in GID order */
	FontViewBase *fv = di->fv;
	int glyphcount = fv->sf->glyphcnt;
	while ( di->pos < glyphcount ) {
	    if ( SCWorthOutputting(fv->sf->glyphs[di->pos]) ) {
return( PySC_From_SC_I( fv->sf->glyphs[di->pos++] ) );
	    }
	    ++di->pos;
	}
      break;}
      case 4: {		/* All glyphs in encoding order */
	int gid;
	FontViewBase *fv = di->fv;
	int enccount = fv->map->enccount;
	while ( di->pos < enccount ) {
	    if ( (gid=fv->map->map[di->pos])!=-1 &&
		    SCWorthOutputting(fv->sf->glyphs[gid]) ) {
		++di->pos;
return( PySC_From_SC_I( fv->sf->glyphs[gid] ) );
	    }
	    ++di->pos;
	}
      break;}
      default:
      break;
    }

return NULL;
}

static PyTypeObject PyFF_FontIterType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "fontforge.font_iterator", /* tp_name */
    sizeof(fontiterobject),    /* tp_basicsize */
    0,                         /* tp_itemsize */
    /* methods */
    (destructor)fontiter_dealloc, /* tp_dealloc */
    0,                         /* tp_vectorcall_offset */
    NULL,                      /* tp_getattr */
    NULL,                      /* tp_setattr */
    NULL,                      /* tp_compare */
    NULL,                      /* tp_repr */
    NULL,                      /* tp_as_number */
    NULL,                      /* tp_as_sequence */
    NULL,                      /* tp_as_mapping */
    NULL,                      /* tp_hash */
    NULL,                      /* tp_call */
    NULL,                      /* tp_str */
    NULL,                      /* tp_getattro */
    NULL,                      /* tp_setattro */
    NULL,                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    NULL,                      /* tp_doc */
    NULL,                      /* tp_traverse */
    NULL,                      /* tp_clear */
    NULL,                      /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    PyObject_SelfIter,         /* tp_iter */
    (iternextfunc)fontiter_iternextkey, /* tp_iternext */
    NULL,                      /* tp_methods */
    NULL,                      /* tp_members */
    NULL,                      /* tp_getset */
    NULL,                      /* tp_base */
    NULL,                      /* tp_dict */
    NULL,                      /* tp_descr_get */
    NULL,                      /* tp_descr_set */
    0,                         /* tp_dictoffset */
    NULL,                      /* tp_init */
    NULL,                      /* tp_alloc */
    NULL,                      /* tp_new */
    NULL,                      /* tp_free */
    NULL,                      /* tp_is_gc */
    NULL,                      /* tp_bases */
    NULL,                      /* tp_mro */
    NULL,                      /* tp_cache */
    NULL,                      /* tp_subclasses */
    NULL,                      /* tp_weaklist */
    NULL,                      /* tp_del */
    0,                         /* tp_version_tag */
};

/* ************************************************************************** */
/* Font Standard Methods */
/* ************************************************************************** */

static void PyFF_Font_dealloc(PyFF_Font *self) {
    if ( self->fv!=NULL ) {
	if ( self->fv->python_fv_object == self )
	    self->fv->python_fv_object = NULL;
	self->fv = NULL;
    }
    Py_XDECREF(self->selection);
    Py_XDECREF(self->cvt);
    Py_XDECREF(self->layers);
    Py_XDECREF(self->private);
    Py_XDECREF(self->math);
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *PyFF_Font_new(PyTypeObject *type, PyObject *UNUSED(args), PyObject *UNUSED(kwds)) {
    PyFF_Font *self;

    self = (PyFF_Font *) (type->tp_alloc)(type,0);
    if ( self!=NULL ) {
	self->fv = SFAdd(SplineFontNew(),false);
	self->fv->python_fv_object = self;
    }
return( (PyObject *) self );
}

static PyObject *PyFFFont_close(PyFF_Font *self, PyObject *UNUSED(args)) {
    FontViewBase *fv;

    if( CheckIfFontClosed(self) )
return( NULL );
    fv = self->fv;

    if ( self->math!=NULL && self->math->sf == fv->sf )
	self->math->sf = NULL;

    if ( self->private!=NULL && self->private->fv == fv ) {
	self->private->fv = NULL;
	self->private->sf = NULL;
    }

    if ( self->layers!=NULL && self->layers->sf == fv->sf )
	self->layers->sf = NULL;

    if ( self->selection!=NULL && self->selection->fv == fv )
	self->selection->fv = NULL;

    if ( self->cvt!=NULL && self->cvt->sf == fv->sf ) {
	self->cvt->sf = NULL;
	self->cvt->cvt = NULL;
    }

    fv->python_fv_object = NULL;
    FontViewClose(fv);
    self->fv = NULL;
Py_RETURN_NONE;
}

static PyObject *PyFFFont_Repr(PyFF_Font *self) {
    PyObject *ret;
    char prefix[256];
#ifdef DEBUG
    snprintf(prefix,sizeof(prefix), "<%s at 0x%p fv=0x%p sf=0x%p",
	     Py_TYPENAME(self),
	     self,
	     self->fv,
	     (self->fv ? self->fv->sf : 0) );
#else
    snprintf(prefix,sizeof(prefix), "<%s at 0x%p",
	     Py_TYPENAME(self), self );
#endif
    if ( self->fv==NULL )
	ret = PyUnicode_FromFormat("%s CLOSED>",prefix);
    else
	ret = PyUnicode_FromFormat("%s \"%s\">",prefix,self->fv->sf->fontname);
    return( ret );
}

static PyObject *PyFFFont_Str(PyFF_Font *self) {
return( PyUnicode_FromFormat( "<Font: %s>",
			    IsFontClosed(self) ? "<closed>" : self->fv->sf->fontname ));
}

/* ************************************************************************** */
/* sfnt 'name' table stuff */
/* ************************************************************************** */


static PyObject *sfntnametuple(int lang,int strid,char *name) {
    PyObject *tuple;
    int i;

    tuple = PyTuple_New(3);

    PyTuple_SetItem(tuple,2,Py_BuildValue("s", name));

    for ( i=0; sfnt_name_mslangs[i].name!=NULL ; ++i )
	if ( sfnt_name_mslangs[i].flag == lang )
    break;
    if ( sfnt_name_mslangs[i].flag == lang )
	PyTuple_SetItem(tuple,0,Py_BuildValue("s", sfnt_name_mslangs[i].name));
    else
	PyTuple_SetItem(tuple,0,Py_BuildValue("i", lang));

    for ( i=0; sfnt_name_str_ids[i].name!=NULL ; ++i )
	if ( sfnt_name_str_ids[i].flag == strid )
    break;
    if ( sfnt_name_str_ids[i].flag == strid )
	PyTuple_SetItem(tuple,1,Py_BuildValue("s", sfnt_name_str_ids[i].name));
    else
	PyTuple_SetItem(tuple,1,Py_BuildValue("i", strid));
return( tuple );
}

static int SetSFNTName(SplineFont *sf,PyObject *tuple,struct ttflangname *english) {
    const char *lang_str, *strid_str, *string;
    int lang, strid;
    PyObject *val;
    struct ttflangname *names;

    if ( PySequence_Size(tuple)!=3 ) {
	PyErr_Format(PyExc_TypeError, "sfnt_name must be a tuples of three strings" );
return(0);
    }

    val = PySequence_GetItem(tuple,0);
    if ( PyUnicode_Check(val) ) {
        if ((lang_str = PyUnicode_AsUTF8(val)) != NULL) {
            lang = FlagsFromString(lang_str,sfnt_name_mslangs,"language");
        }
        if (lang_str == NULL || lang == FLAG_UNKNOWN) {
            Py_DECREF(val);
            return 0;
        }
    } else if ( PyLong_Check(val))
	lang = PyLong_AsLong(val);
    else {
	Py_XDECREF(val);
	PyErr_Format(PyExc_TypeError, "Language must be a string or an integer" );
return( 0 );
    }
    Py_DECREF(val);

    val = PySequence_GetItem(tuple,1);
    if ( PyUnicode_Check(val) ) {
        if ((strid_str = PyUnicode_AsUTF8(val)) != NULL) {
            strid = FlagsFromString(strid_str,sfnt_name_str_ids,"string id");
        }
        if (strid_str == NULL || strid == FLAG_UNKNOWN) {
            Py_DECREF(val);
            return 0;
        }
    } else if ( PyLong_Check(val))
	strid = PyLong_AsLong(val);
    else {
	Py_XDECREF(val);
	PyErr_Format(PyExc_TypeError, "String-id must be a string or an integer" );
return( 0 );
    }
    Py_DECREF(val);

    for ( names=sf->names; names!=NULL; names=names->next )
	if ( names->lang==lang )
    break;

    val = PySequence_GetItem(tuple, 2);
    if ( val==Py_None ) {
	if ( names!=NULL ) {
	    free( names->names[strid] );
	    names->names[strid] = NULL;
	}
	Py_DECREF(val);
return( 1 );
    }

    string = PyUnicode_AsUTF8(val);
    if ( string==NULL ) {
	Py_XDECREF(val);
return( 0 );
    }
    if ( lang==0x409 && english!=NULL && english->names[strid]!=NULL &&
         strcmp(string,english->names[strid])==0 ) {
	Py_DECREF(val);
return( 1 );	/* If they set it to the default, there's nothing to do */
    }

    if ( names==NULL ) {
	names = chunkalloc(sizeof( struct ttflangname ));
	names->lang = lang;
	names->next = sf->names;
	sf->names = names;
    }
    free(names->names[strid]);
    names->names[strid] = copy( string );
    Py_DECREF(val);
return( 1 );
}

/* ************************************************************************** */
/* Font getters/setters */
/* ************************************************************************** */

static PyObject *PyFF_Font_get_sfntnames(PyFF_Font *self, void *UNUSED(closure)) {
    struct ttflangname *names, *english;
    int cnt, i;
    PyObject *tuple;
    struct ttflangname dummy;
    SplineFont *sf;

    if ( CheckIfFontClosed(self) )
return( NULL );

    sf = self->fv->sf;
    memset(&dummy,0,sizeof(dummy));
    DefaultTTFEnglishNames(&dummy, sf);

    cnt = 0;
    for ( english = sf->names; english!=NULL; english=english->next )
	if ( english->lang==0x409 )
    break;
    for ( i=0; i<ttf_namemax; ++i ) {
	if ( (english!=NULL && english->names[i]!=NULL ) || dummy.names[i]!=NULL )
	    ++cnt;
    }
    for ( names = sf->names; names!=NULL; names=names->next ) if ( names!=english ) {
	for ( i=0; i<ttf_namemax; ++i ) {
	    if ( names->names[i]!=NULL )
		++cnt;
	}
    }
    tuple = PyTuple_New(cnt);
    cnt = 0;
    for ( i=0; i<ttf_namemax; ++i ) {
	char *nm = (english!=NULL && english->names[i]!=NULL ) ? english->names[i] : dummy.names[i];
	if ( nm!=NULL )
	    PyTuple_SetItem(tuple,cnt++,sfntnametuple(0x409,i,nm));
    }
    for ( names = sf->names; names!=NULL; names=names->next ) if ( names!=english ) {
	for ( i=0; i<ttf_namemax; ++i ) {
	    if ( names->names[i]!=NULL )
		PyTuple_SetItem(tuple,cnt++,sfntnametuple(names->lang,i,names->names[i]));
	}
    }

    for ( i=0; i<ttf_namemax; ++i )
	free( dummy.names[i]);

return( tuple );
}

static int PyFF_Font_set_sfntnames(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) {
    SplineFont *sf;
    struct ttflangname *names;
    struct ttflangname dummy;
    int i;

    if ( CheckIfFontClosed(self) )
return(-1);

    sf = self->fv->sf;
    if ( !PySequence_Check(value)) {
	PyErr_Format(PyExc_TypeError, "Value must be a tuple" );
return(-1);
    }

    memset(&dummy,0,sizeof(dummy));
    DefaultTTFEnglishNames(&dummy, sf);

    for ( names = sf->names; names!=NULL; names=names->next ) {
	for ( i=0; i<ttf_namemax; ++i ) {
	    free(names->names[i]);
	    names->names[i] = NULL;
	}
    }
    for ( i=PySequence_Size(value)-1; i>=0; --i )
	if ( !SetSFNTName(sf,PySequence_GetItem(value,i),&dummy) )
return( -1 );

    for ( i=0; i<ttf_namemax; ++i )
	free( dummy.names[i]);

return( 0 );
}

static PyObject *PyFF_Font_get_bitmapSizes(PyFF_Font *self, void *UNUSED(closure)) {
    PyObject *tuple;
    int cnt;
    SplineFont *sf;
    BDFFont *bdf;

    if ( CheckIfFontClosed(self) )
return( NULL );

    sf = self->fv->sf;
    for ( cnt=0, bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next, ++cnt );

    tuple = PyTuple_New(cnt);
    for ( cnt=0, bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next, ++cnt )
	PyTuple_SetItem(tuple,cnt,Py_BuildValue("i",
		bdf->clut==NULL ? bdf->pixelsize :
				(bdf->pixelsize | (BDFDepth(bdf)<<16)) ));

return( tuple );
}

static int bitmapper(PyFF_Font *self,PyObject *value,int isavail) {
    int cnt, i;
    int *sizes;

    if ( CheckIfFontClosed(self) )
return(-1);

    cnt = PyTuple_Size(value);
    if ( PyErr_Occurred())
return( -1 );
    sizes = malloc((cnt+1)*sizeof(int));
    for ( i=0; i<cnt; ++i ) {
	if ( !PyArg_ParseTuple(PyTuple_GetItem(value,i),"i", &sizes[i])) {
	    free(sizes);
return( -1 );
	}
	if ( (sizes[i]>>16)==0 )
	    sizes[i] |= 0x10000;
    }
    sizes[i] = 0;

    if ( !BitmapControl(self->fv,sizes,isavail,false) ) {
	free(sizes);
	PyErr_Format(PyExc_EnvironmentError, "Bitmap operation failed");
return( -1 );
    }
    free(sizes);
return( 0 );
}

static int PyFF_Font_set_bitmapSizes(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) {
return( bitmapper(self,value,true));
}

/* GASP (grid-fitting and scan procedure) flags */
static struct flaglist gaspflags[] = {
    { "gridfit",		1 },
    { "antialias",		2 },
    { "symmetric-smoothing",	8 },
    { "gridfit+smoothing",	4 },
    FLAGLIST_EMPTY /* Sentinel */
};

static PyObject *PyFF_Font_get_gasp(PyFF_Font *self, void *UNUSED(closure)) {
    PyObject *tuple, *flagstuple;
    int i, j, cnt;
    SplineFont *sf;

    if ( CheckIfFontClosed(self) )
return(NULL);

    sf = self->fv->sf;
    tuple = PyTuple_New(sf->gasp_cnt);

    for ( i=0; i<sf->gasp_cnt; ++i ) {
	for ( j=cnt=0; gaspflags[j].name!=NULL; ++j )
	    if ( sf->gasp[i].flags & gaspflags[j].flag )
		++cnt;
	flagstuple = PyTuple_New(cnt);
	for ( j=cnt=0; gaspflags[j].name!=NULL; ++j )
	    if ( sf->gasp[i].flags & gaspflags[j].flag )
		PyTuple_SetItem(flagstuple,cnt++,Py_BuildValue("s", gaspflags[j].name));
	PyTuple_SetItem(tuple,i,Py_BuildValue("iO",sf->gasp[i].ppem,flagstuple));
    }

return( tuple );
}

static int PyFF_Font_set_gasp(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) {
    SplineFont *sf;
    int cnt, i, flag;
    struct gasp *gasp;
    PyObject *flags;

    if ( CheckIfFontClosed(self) )
return(-1);

    sf = self->fv->sf;
    cnt = PyTuple_Size(value);
    if ( PyErr_Occurred())
return( -1 );
    if ( cnt==0 )
	gasp = NULL;
    else {
	gasp = malloc(cnt*sizeof(struct gasp));
	for ( i=0; i<cnt; ++i ) {
	    if ( !PyArg_ParseTuple(PyTuple_GetItem(value,i),"HO",
				   &gasp[i].ppem, &flags )) {
		free(gasp);
return( -1 );
	    }
	    flag = FlagsFromTuple(flags,gaspflags,"gasp flag");
	    if ( flag==FLAG_UNKNOWN ) {
		free(gasp);
return( -1 );
	    }
	    gasp[i].flags = flag;
	}
    }
    free(sf->gasp);
    sf->gasp = gasp;
    sf->gasp_cnt = cnt;
return( 0 );
}

static PyObject *PyFF_Font_get_lookups(PyFF_Font *self, void *UNUSED(closure), int isgpos) {
    PyObject *tuple;
    OTLookup *otl;
    int cnt;
    SplineFont *sf;

    if ( CheckIfFontClosed(self) )
return(NULL);

    sf = self->fv->sf;
    cnt = 0;
    for ( otl=isgpos ? sf->gpos_lookups : sf->gsub_lookups; otl!=NULL; otl=otl->next )
	++cnt;

    tuple = PyTuple_New(cnt);

    cnt = 0;
    for ( otl=isgpos ? sf->gpos_lookups : sf->gsub_lookups; otl!=NULL; otl=otl->next )
	PyTuple_SetItem(tuple,cnt++,Py_BuildValue("s",otl->lookup_name));

return( tuple );
}

static PyObject *PyFF_Font_get_gpos_lookups(PyFF_Font *self, void *closure) {
return( PyFF_Font_get_lookups(self,closure,true));
}

static PyObject *PyFF_Font_get_gsub_lookups(PyFF_Font *self, void *closure) {
return( PyFF_Font_get_lookups(self,closure,false));
}

static PyObject *PyFF_Font_get_math(PyFF_Font *self, void *UNUSED(closure)) {
    PyFF_Math *math;

    if ( CheckIfFontClosed(self) )
return(NULL);

    if ( self->math!=NULL )
Py_RETURN( self->math );

    math = (PyFF_Math *) PyObject_New(PyFF_Math, &PyFF_MathType);
    if (math == NULL)
return NULL;
    math->sf = self->fv->sf;
    self->math = math;
Py_RETURN( self->math );
}

static PyObject *PyFF_Font_get_private(PyFF_Font *self, void *UNUSED(closure)) {
    PyFF_Private *private;

    if ( CheckIfFontClosed(self) )
return(NULL);

    if ( self->private!=NULL )
Py_RETURN( self->private );
    private = (PyFF_Private *) PyObject_New(PyFF_Private, &PyFF_PrivateType);
    if (private == NULL)
return NULL;
    private->sf = self->fv->sf;
    private->fv = self->fv;
    self->private = private;
Py_RETURN( self->private );
}

static PyObject *PyFF_Font_get_layers(PyFF_Font *self, void *UNUSED(closure)) {
    PyFF_LayerInfoArray *layers;

    if ( CheckIfFontClosed(self) )
return(NULL);

    if ( self->layers!=NULL )
Py_RETURN( self->layers );

    layers = (PyFF_LayerInfoArray *) PyObject_New(PyFF_LayerInfoArray, &PyFF_LayerInfoArrayType);
    if (layers == NULL)
return NULL;
    layers->sf = self->fv->sf;
    self->layers = layers;
Py_RETURN( self->layers );
}

static PyObject *PyFF_Font_get_layer_cnt(PyFF_Font *self, void *UNUSED(closure)) {
    if ( CheckIfFontClosed(self) )
return(NULL);
return( Py_BuildValue("i", self->fv->sf->layer_cnt ));
}

static PyObject *PyFF_Font_get_activeLayer(PyFF_Font *self, void *UNUSED(closure)) {
    if ( CheckIfFontClosed(self) )
return(NULL);
return( Py_BuildValue("i", self->fv->active_layer ));
}

static int PyFF_Font_set_activeLayer(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) {
    int layer;

    if ( CheckIfFontClosed(self) )
return(-1);

    if ( PyLong_Check(value) )
	layer = PyLong_AsLong(value);
    else if ( PyUnicode_Check(value)) {
	const char *name = PyUnicode_AsUTF8(value);
	if (name == NULL) {
	    return -1;
	}
	layer = SFFindLayerIndexByName(self->fv->sf,name);
	if ( layer<0 )
return( -1 );
    } else {
        return -1;
    }
    if ( layer<0 || layer>=self->fv->sf->layer_cnt ) {
	PyErr_Format(PyExc_ValueError, "Layer is out of range" );
return( -1 );
    }
    self->fv->active_layer = layer;
return( 0 );
}

static PyObject *PyFF_Font_get_selection(PyFF_Font *self, void *UNUSED(closure)) {
return( PyFFSelection_new(self));
}

static int PyFF_Font_set_selection(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) {
    PyFF_Selection *sel = (PyFF_Selection *) value;
    int i, len2;
    int is_sel;
    FontViewBase *fv;

    if ( CheckIfFontClosed(self) )
return(-1);

    fv = self->fv;
    if ( PyType_IsSubtype(&PyFF_SelectionType, Py_TYPE(value)) ) {
	len2 = PyFFSelection_Length(value);
	is_sel = true;
    } else if ( PySequence_Check(value)) {
	is_sel = false;
	len2 = PySequence_Size(value);
    } else {
	PyErr_Format(PyExc_TypeError, "The value must be either another selection or a tuple of integers");
return( -1 );
    }

    if ( len2>=fv->map->enccount ) {
	PyErr_Format(PyExc_TypeError, "Too much data");
return( -1 );
    }
    if ( is_sel ) {
	if ( len2!=0 )
	    memcpy(fv->selected,sel->fv->selected,len2 );
    } else {
	for( i=0; i<len2; ++i ) {
	    int val;
	    PyObject *obj = PySequence_GetItem(value,i);
	    if ( obj==Py_True )
		val = 1;
	    else if ( obj==Py_False )
		val = 0;
	    else {
		val = PyLong_AsLong(obj);
		if ( PyErr_Occurred())
return( -1 );
	    }
	    fv->selected[i] = val;
	}
    }
return( 0 );
}

static PyObject *PyFF_Font_get_cvt(PyFF_Font *self, void *UNUSED(closure)) {
return( PyFFCvt_new(self));
}

static int PyFF_Font_set_cvt(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) {
    PyFF_Cvt *c2 = (PyFF_Cvt *) value;
    size_t i, len2;
    int is_cvt2;
    SplineFont *sf;
    struct ttf_table *cvt;

    if ( CheckIfFontClosed(self) )
return(-1);

    sf = self->fv->sf;
    if ( PyType_IsSubtype(&PyFF_CvtType, Py_TYPE(value)) ) {
	len2 = PyFFCvt_Length(value);
	is_cvt2 = true;
    } else if ( PySequence_Check(value)) {
	is_cvt2 = false;
	len2 = PySequence_Size(value);
    } else {
	PyErr_Format(PyExc_TypeError, "The value must be either another cvt or a tuple of integers");
return( -1 );
    }

    cvt = SFFindTable(sf,CHR('c','v','t',' '));
    if ( cvt==NULL )
	cvt = BuildCvt(sf,len2*2);
    if ( len2*2>=cvt->maxlen )
	cvt->data = realloc(cvt->data,cvt->maxlen = sizeof(uint16_t)*len2+10 );
    if ( is_cvt2 ) {
	if ( len2!=0 )
	    memcpy(cvt->data,c2->cvt->data,2*len2 );
    } else {
	for( i=0; i<len2; ++i ) {
	    memputshort(cvt->data,2*i,PyLong_AsLong(PySequence_GetItem(value,i)));
	    if ( PyErr_Occurred())
return( -1 );
	}
    }
    cvt->len = 2*len2;
return( 0 );
}

static PyObject *PyFF_Font_get_temporary(PyFF_Font *self, void *UNUSED(closure)) {
    SplineFont *sf;

    if ( CheckIfFontClosed(self) )
return(NULL);

    sf = self->fv->sf;
    if ( sf->python_temporary==NULL )
Py_RETURN_NONE;
    Py_INCREF( (PyObject *) (sf->python_temporary) );
return( sf->python_temporary );
}

static int PyFF_Font_set_temporary(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) {
    SplineFont *sf;
    PyObject *old;

    if ( CheckIfFontClosed(self) )
return(-1);

    sf = self->fv->sf;
    old = sf->python_temporary;
    /* I'd rather not store None, because C routines don't understand it */
    /*  and they occasionally need to know whether there is something real */
    /*  in this field. */
    if ( value==Py_None )
	value = NULL;
    Py_XINCREF(value);
    sf->python_temporary = value;
    Py_XDECREF(old);
return( 0 );
}

static PyObject *PyFF_Font_get_persistent(PyFF_Font *self, void *UNUSED(closure)) {
    SplineFont *sf;

    if ( CheckIfFontClosed(self) )
return(NULL);

    sf = self->fv->sf;
    if ( sf->python_persistent==NULL )
Py_RETURN_NONE;
    Py_INCREF( (PyObject *) (sf->python_persistent) );
return( sf->python_persistent );
}

static int PyFF_Font_set_persistent(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) {
    SplineFont *sf;
    PyObject *old;

    if ( CheckIfFontClosed(self) )
return(-1);

    sf = self->fv->sf;
    old = sf->python_persistent;
    /* I'd rather not store None, because C routines don't understand it */
    /*  and they occasionally need to know whether there is something real */
    /*  in this field. */
    if ( value==Py_None )
	value = NULL;
    Py_XINCREF(value);
    sf->python_persistent = value;
    Py_XDECREF(old);
return( 0 );
}

static int _PyFF_Font_set_str_null(SplineFont *sf,PyObject *value,
	const char *str,int offset) {
    char *newv, **oldpos;

    if ( value==NULL ) {
	PyErr_Format(PyExc_TypeError, "Cannot delete the %s", str);
return( -1 );
    }
    newv = (value == Py_None) ? NULL : copy(PyUnicode_AsUTF8(value));

    if ( newv==NULL && value!=Py_None )
return( -1 );
    oldpos = (char **) (((char *) sf) + offset );
    free( *oldpos );
    *oldpos = newv;
return( 0 );
}

static int PyFF_Font_set_str_null(PyFF_Font *self,PyObject *value,
				  const char *str,int offset) {
    if ( CheckIfFontClosed(self) )
return(-1);
return( _PyFF_Font_set_str_null(self->fv->sf,value,str,offset));
}

static int PyFF_Font_set_cidstr_null(PyFF_Font *self,PyObject *value,
				     const char *str,int offset) {
    if ( CheckIfFontClosed(self) )
return(-1);
    if ( self->fv->cidmaster==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "Not a cid-keyed font");
return( -1 );
    }
return( _PyFF_Font_set_str_null(self->fv->cidmaster,value,str,offset));
}

static int PyFF_Font_set_str(PyFF_Font *self,PyObject *value,
			     char *str,int offset) {
    char *newv, **oldpos;

    if ( CheckIfFontClosed(self) )
return(-1);
    if ( value==NULL ) {
	PyErr_Format(PyExc_TypeError, "Cannot delete the %s", str);
return( -1 );
    }

    newv = copy(PyUnicode_AsUTF8(value));

    if ( newv==NULL )
return( -1 );
    oldpos = (char **) (((char *) (self->fv->sf)) + offset );
    free( *oldpos );
    *oldpos = newv;
return( 0 );
}

static int _PyFF_Font_set_real(SplineFont *sf,PyObject *value,
			       const char *str,int offset) {
    double temp;

    if ( value==NULL ) {
	PyErr_Format(PyExc_TypeError, "Cannot delete the %s", str);
return( -1 );
    }
    temp = PyFloat_AsDouble(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );
    * (real *) (((char *) sf) + offset ) = temp;
return( 0 );
}

static int PyFF_Font_set_real(PyFF_Font *self,PyObject *value,
			      const char *str,int offset) {
    if ( CheckIfFontClosed(self) )
return(-1);
return( _PyFF_Font_set_real( self->fv->sf,value,str,offset));
}

static int PyFF_Font_set_cidreal(PyFF_Font *self,PyObject *value,
				 const char *str,int offset) {
    if ( CheckIfFontClosed(self) )
return(-1);
    if ( self->fv->cidmaster==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "Not a cid-keyed font");
return( -1 );
    }
return( _PyFF_Font_set_real( self->fv->cidmaster,value,str,offset));
}

static int _PyFF_Font_set_int(SplineFont *sf,PyObject *value,
	const char *str,int offset) {
    long temp;

    if ( value==NULL ) {
	PyErr_Format(PyExc_TypeError, "Cannot delete the %s", str);
return( -1 );
    }
    temp = PyLong_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );
    * (int *) (((char *) sf) + offset ) = temp;
return( 0 );
}

static int PyFF_Font_set_int(PyFF_Font *self,PyObject *value,
			     const char *str,int offset) {
    if ( CheckIfFontClosed(self) )
return(-1);
return( _PyFF_Font_set_int( self->fv->sf,value,str,offset));
}

static int PyFF_Font_set_cidint(PyFF_Font *self,PyObject *value,
				const char *str,int offset) {
    if ( CheckIfFontClosed(self) )
return(-1);
    if ( self->fv->cidmaster==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "Not a cid-keyed font");
return( -1 );
    }
return( _PyFF_Font_set_int( self->fv->cidmaster,value,str,offset));
}

static int PyFF_Font_set_int2(PyFF_Font *self,PyObject *value,
			      const char *str,int offset) {
    long temp;
    if ( CheckIfFontClosed(self) )
return(-1);
    if ( value==NULL ) {
	PyErr_Format(PyExc_TypeError, "Cannot delete the %s", str);
return( -1 );
    }
    temp = PyLong_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );
    * (int16_t *) (((char *) (self->fv->sf)) + offset ) = temp;
return( 0 );
}

static PyObject *PyFF_Font_get_texparams(PyFF_Font *self, void *UNUSED(closure)) {
    SplineFont *sf;
    int i, em;
    PyObject *tuple;
    double val;

    if ( CheckIfFontClosed(self) )
return(NULL);

    sf = self->fv->sf;
    em = sf->ascent+sf->descent;
    tuple = PyTuple_New(23);

    if ( sf->texdata.type==tex_text )
	PyTuple_SetItem(tuple,0,Py_BuildValue("s", "text"));
    else if ( sf->texdata.type==tex_math )
	PyTuple_SetItem(tuple,0,Py_BuildValue("s", "mathsym"));
    else if ( sf->texdata.type==tex_mathext )
	PyTuple_SetItem(tuple,0,Py_BuildValue("s", "mathext"));
    else if ( sf->texdata.type==tex_unset ) {
	PyTuple_SetItem(tuple,0,Py_BuildValue("s", "unset"));
	TeXDefaultParams(sf);
    }

    for ( i=1; i<23; i++ ) {
	val = rint( (double) sf->texdata.params[i-1] * em / (1<<20) );
	PyTuple_SetItem(tuple,i,Py_BuildValue( "d", val ));
    }

return( tuple );
}

/* *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  */
#define ff_gs_str(name) \
static PyObject *PyFF_Font_get_##name(PyFF_Font *self, void *UNUSED(closure)) { \
    if ( CheckIfFontClosed(self) ) return(NULL);	\
return( Py_BuildValue("s", self->fv->sf->name ));	\
}							\
							\
static int PyFF_Font_set_##name(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) {\
    if(CheckIfFontClosed(self)) return(-1);				\
return( PyFF_Font_set_str(self,value,(char *)#name,offsetof(SplineFont,name)) ); \
}

/* *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  */
#define ff_gs_strnull(name) \
static PyObject *PyFF_Font_get_##name(PyFF_Font *self, void *UNUSED(closure)) { \
    if ( CheckIfFontClosed(self) ) return(NULL);	\
    if ( self->fv->sf->name==NULL )			\
Py_RETURN_NONE;						\
    else						\
return( Py_BuildValue("s", self->fv->sf->name ));	\
}							\
							\
static int PyFF_Font_set_##name(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) {\
    if ( CheckIfFontClosed(self) ) return(-1);				     \
return( PyFF_Font_set_str_null(self,value,#name,offsetof(SplineFont,name)) );\
}

/* *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  */
#define ff_gs_cidstrnull(name) \
static PyObject *PyFF_Font_get_cid##name(PyFF_Font *self, void *UNUSED(closure)) { \
    if ( CheckIfFontClosed(self) ) return(NULL);	\
    if ( self->fv->cidmaster==NULL )			\
Py_RETURN_NONE;						\
    if ( self->fv->cidmaster->name==NULL )		\
Py_RETURN_NONE;						\
    else						\
return( Py_BuildValue("s", self->fv->cidmaster->name )); \
}							\
							\
static int PyFF_Font_set_cid##name(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) {\
    if ( CheckIfFontClosed(self) ) return(-1);					      \
return( PyFF_Font_set_cidstr_null(self,value,"cid" #name,offsetof(SplineFont,name)) );\
}

/* *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  */
#define ff_gs_real(name) \
static PyObject *PyFF_Font_get_##name(PyFF_Font *self, void *UNUSED(closure)) { \
    if ( CheckIfFontClosed(self) ) return(NULL);	\
return( Py_BuildValue("d", self->fv->sf->name ));	\
}							\
							\
static int PyFF_Font_set_##name(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) {\
    if ( CheckIfFontClosed(self) ) return(-1);				 \
return( PyFF_Font_set_real(self,value,#name,offsetof(SplineFont,name)) );\
}

/* *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  */
#define ff_gs_cidreal(name) \
static PyObject *PyFF_Font_get_##name(PyFF_Font *self, void *UNUSED(closure)) { \
    if ( CheckIfFontClosed(self) ) return (NULL);	\
    if ( self->fv->cidmaster==NULL )			\
Py_RETURN_NONE;						\
return( Py_BuildValue("d", (double)self->fv->cidmaster->name ));	\
}							\
							\
static int PyFF_Font_set_##name(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) {\
    if ( CheckIfFontClosed(self) ) return (-1);				    \
return( PyFF_Font_set_cidreal(self,value,#name,offsetof(SplineFont,name)) );\
}

/* *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  */
#define ff_gs_int(name) \
static PyObject *PyFF_Font_get_##name(PyFF_Font *self, void *UNUSED(closure)) { \
    if ( CheckIfFontClosed(self) ) return (NULL);	\
return( Py_BuildValue("i", self->fv->sf->name ));	\
}							\
							\
static int PyFF_Font_set_##name(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) {\
    if ( CheckIfFontClosed(self) ) return (-1);				\
return( PyFF_Font_set_int(self,value,#name,offsetof(SplineFont,name)) );\
}

/* *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  */
#define ff_gs_cidint(name) \
static PyObject *PyFF_Font_get_##name(PyFF_Font *self, void *UNUSED(closure)) { \
    if ( CheckIfFontClosed(self) ) return (NULL);		\
    if ( self->fv->cidmaster==NULL )				\
Py_RETURN_NONE;							\
return( Py_BuildValue("i", self->fv->cidmaster->name ));	\
}								\
								\
static int PyFF_Font_set_##name(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) {\
    if ( CheckIfFontClosed(self) ) return (-1);				   \
return( PyFF_Font_set_cidint(self,value,#name,offsetof(SplineFont,name)) );\
}

/* *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  */
#define ff_gs_int2(name) \
static PyObject *PyFF_Font_get_##name(PyFF_Font *self, void *UNUSED(closure)) { \
    if ( CheckIfFontClosed(self) ) return (NULL);	\
return( Py_BuildValue("i", self->fv->sf->name ));	\
}							\
							\
static int PyFF_Font_set_##name(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) {\
    if ( CheckIfFontClosed(self) ) return (-1);				 \
return( PyFF_Font_set_int2(self,value,#name,offsetof(SplineFont,name)) );\
}

/* *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  */
#define ff_gs_ro_bit(name) \
static PyObject *PyFF_Font_get_##name(PyFF_Font *self, void *UNUSED(closure)) { \
    if ( CheckIfFontClosed(self) ) return (NULL);	\
return( Py_BuildValue("i", self->fv->sf->name ));	\
}

/* *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  */
#define ff_gs_bit(name) \
static PyObject *PyFF_Font_get_##name(PyFF_Font *self, void *UNUSED(closure)) { \
    if ( CheckIfFontClosed(self) ) return (NULL);	\
return( Py_BuildValue("i", self->fv->sf->name ));	\
}							\
							\
static int PyFF_Font_set_##name(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) { \
    long temp;						\
							\
    if ( CheckIfFontClosed(self) ) return (-1);		\
    if ( value==NULL ) {				\
	PyErr_SetString(PyExc_TypeError, "Cannot delete the " #name ); \
return( -1 );						\
    }							\
    temp = PyLong_AsLong(value);				\
    if ( PyErr_Occurred()!=NULL )			\
return( -1 );						\
    self->fv->sf->name = temp;				\
return( 0 );						\
}
/* *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  */
static void SFDefaultOS2(SplineFont *sf) {
    if ( !sf->pfminfo.pfmset ) {
	SFDefaultOS2Info(&sf->pfminfo,sf,sf->fontname);
	sf->pfminfo.pfmset = sf->pfminfo.subsuper_set = sf->pfminfo.panose_set =
	    sf->pfminfo.hheadset = sf->pfminfo.vheadset = true;
    }
}

#define ff_gs_os2int2(name) \
static PyObject *PyFF_Font_get_OS2_##name(PyFF_Font *self, void *UNUSED(closure)) { \
    SplineFont *sf;					\
    if ( CheckIfFontClosed(self) ) return (NULL);	\
    sf = self->fv->sf;					\
    SFDefaultOS2(sf);					\
return( Py_BuildValue("i", sf->pfminfo.name ));		\
}							\
							\
static int PyFF_Font_set_OS2_##name(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) {\
    SplineFont *sf;					\
    if ( CheckIfFontClosed(self) ) return (-1);		\
    sf = self->fv->sf;					\
    SFDefaultOS2(sf);					\
return( PyFF_Font_set_int2(self,value,#name,offsetof(SplineFont,pfminfo)+offsetof(struct pfminfo,name)) );\
}
/* *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  */
#define ff_gs_os2bit(name) \
static PyObject *PyFF_Font_get_OS2_##name(PyFF_Font *self, void *UNUSED(closure)) { \
    SplineFont *sf;					\
    if ( CheckIfFontClosed(self) ) return (NULL);	\
    sf = self->fv->sf;					\
    SFDefaultOS2(sf);					\
return( Py_BuildValue("i", self->fv->sf->pfminfo.name ));	\
}							\
							\
static int PyFF_Font_set_OS2_##name(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) { \
    SplineFont *sf;					\
    long temp;						\
							\
    if ( CheckIfFontClosed(self) ) return (-1);		\
    sf = self->fv->sf;					\
    if ( value==NULL ) {				\
	PyErr_SetString(PyExc_TypeError, "Cannot delete the " #name ); \
return( -1 );						\
    }							\
    temp = PyLong_AsLong(value);				\
    if ( PyErr_Occurred()!=NULL )			\
return( -1 );						\
    SFDefaultOS2(sf);					\
    sf->pfminfo.name = temp;				\
return( 0 );						\
}

ff_gs_str(fontname)
ff_gs_str(fullname)
ff_gs_str(familyname)
ff_gs_str(weight)
ff_gs_str(version)

ff_gs_strnull(comments)
ff_gs_strnull(fontlog)
ff_gs_strnull(copyright)
ff_gs_strnull(xuid)
ff_gs_strnull(fondname)
ff_gs_strnull(woffMetadata)
ff_gs_strnull(defbasefilename)

ff_gs_cidstrnull(fontname)
ff_gs_cidstrnull(familyname)
ff_gs_cidstrnull(fullname)
ff_gs_cidstrnull(weight)
ff_gs_cidstrnull(copyright)
ff_gs_cidstrnull(cidregistry)
ff_gs_cidstrnull(ordering)

ff_gs_cidreal(cidversion)
ff_gs_cidint(supplement)

ff_gs_real(italicangle)
ff_gs_real(upos)
ff_gs_real(uwidth)
ff_gs_real(strokewidth)

ff_gs_int(ascent)
ff_gs_int(descent)
ff_gs_int(uniqueid)

ff_gs_int2(macstyle)
ff_gs_int2(os2_version)
ff_gs_int2(gasp_version)

ff_gs_os2int2(weight)
ff_gs_os2int2(width)
ff_gs_os2int2(stylemap)
ff_gs_os2int2(fstype)
ff_gs_os2int2(linegap)
ff_gs_os2int2(vlinegap)
ff_gs_os2int2(hhead_ascent)
ff_gs_os2int2(hhead_descent)
ff_gs_os2int2(os2_typoascent)
ff_gs_os2int2(os2_typodescent)
ff_gs_os2int2(os2_typolinegap)
ff_gs_os2int2(os2_winascent)
ff_gs_os2int2(os2_windescent)
ff_gs_os2int2(os2_subxsize)
ff_gs_os2int2(os2_subxoff)
ff_gs_os2int2(os2_subysize)
ff_gs_os2int2(os2_subyoff)
ff_gs_os2int2(os2_supxsize)
ff_gs_os2int2(os2_supxoff)
ff_gs_os2int2(os2_supysize)
ff_gs_os2int2(os2_supyoff)
ff_gs_os2int2(os2_strikeysize)
ff_gs_os2int2(os2_strikeypos)
ff_gs_os2int2(os2_capheight)
ff_gs_os2int2(os2_xheight)
ff_gs_os2int2(os2_family_class)

ff_gs_os2bit(winascent_add)
ff_gs_os2bit(windescent_add)
ff_gs_os2bit(hheadascent_add)
ff_gs_os2bit(hheaddescent_add)
ff_gs_os2bit(typoascent_add)
ff_gs_os2bit(typodescent_add)

ff_gs_bit(changed)
ff_gs_ro_bit(multilayer)
ff_gs_bit(strokedfont)
ff_gs_ro_bit(new)

ff_gs_bit(use_typo_metrics)
ff_gs_bit(weight_width_slope_only)
ff_gs_bit(onlybitmaps)
ff_gs_bit(hasvmetrics)
ff_gs_bit(head_optimized_for_cleartype)

static PyObject *PyFF_Font_get_creationtime(PyFF_Font *self, void *UNUSED(closure)) {
    if ( CheckIfFontClosed(self) )
return(NULL);

  SplineFont *sf = self->fv->sf;
  time_t t = sf->creationtime;
  const struct tm *tm = gmtime(&t);
  char creationtime[200];
  strftime(creationtime, sizeof(creationtime), "%Y/%m/%d %H:%M:%S", tm);
return Py_BuildValue("s", creationtime);
}

static PyObject *PyFF_Font_get_sfntRevision(PyFF_Font *self, void *UNUSED(closure)) {
    int version = self->fv->sf->sfntRevision;

    if ( CheckIfFontClosed(self) )
return(NULL);
    if ( version==sfntRevisionUnset )
Py_RETURN_NONE;

return( Py_BuildValue("d", version/65536.0 ));
}

static int PyFF_Font_set_sfntRevision(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) {
    SplineFont *sf;

    if ( CheckIfFontClosed(self) )
return(-1);
    sf = self->fv->sf;
    if ( value==Py_None )
	sf->sfntRevision = sfntRevisionUnset;
    else if ( PyFloat_Check(value)) {
	double temp = PyFloat_AsDouble(value);

	sf->sfntRevision = rint(65536*temp);
    } else if ( PyLong_Check(value)) {
	int val = PyLong_AsLong(value);
	/* if ( val<100 )
	    sf->sfntRevision = val<<16;
	else*/
	    sf->sfntRevision = val;
    } else {
	PyErr_Format(PyExc_TypeError, "Value must be a double, integer or None" );
return( -1 );
    }

return( 0 );
}

static PyObject *PyFF_Font_get_woffMajor(PyFF_Font *self, void *UNUSED(closure)) {
    int version;

    if ( CheckIfFontClosed(self) )
return (NULL);
    version = self->fv->sf->woffMajor;
    if ( version==woffUnset )
Py_RETURN_NONE;

return( Py_BuildValue("i", version ));
}

static int PyFF_Font_set_woffMajor(PyFF_Font *self, PyObject *value, void *UNUSED(closure)) {
    SplineFont *sf;

    if ( CheckIfFontClosed(self) )
return (-1);
    sf = self->fv->sf;
    if ( value==Py_None ) {
	sf->woffMajor = woffUnset;
	sf->woffMinor = woffUnset;
    } else if ( PyLong_Check(value)) {
	int val = PyLong_AsLong(value);
	sf->woffMajor = val;
	if ( sf->woffMinor==woffUnset )
	    sf->woffMinor = 0;
    } else {
	PyErr_Format(PyExc_TypeError, "Value must be an integer or None" );
return( -1 );
    }

return( 0 );
}

static PyObject *PyFF_Font_get_woffMinor(PyFF_Font *self, void *UNUSED(closure)) {
    SplineFont *sf;

    if ( CheckIfFontClosed(self) )
return (NULL);
    sf = self->fv->sf;
    if ( sf->woffMajor==woffUnset )
Py_RETURN_NONE;

return( Py_BuildValue("i", sf->woffMinor ));
}

static int PyFF_Font_set_woffMinor(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) {
    SplineFont *sf;

    if ( CheckIfFontClosed(self) )
return (-1);
    sf = self->fv->sf;
    if ( value==Py_None ) {
	sf->woffMajor = woffUnset;
	sf->woffMinor = woffUnset;
    } else if ( PyLong_Check(value)) {
	int val = PyLong_AsLong(value);
	sf->woffMinor = val;
	if ( sf->woffMajor==woffUnset )
	    sf->woffMajor = 0;
    } else {
	PyErr_Format(PyExc_TypeError, "Value must be an integer or None" );
return( -1 );
    }

return( 0 );
}

static PyObject *PyFF_Font_get_vertical_origin(PyFF_Font *self, void *UNUSED(closure)) {
    if ( CheckIfFontClosed(self) )
return (NULL);
return( Py_BuildValue("i", 0 )); /* No longer implemented, return 0 for backwards compatibility */
}

static int PyFF_Font_set_vertical_origin(PyFF_Font *UNUSED(self), PyObject *UNUSED(value), void *UNUSED(closure)) {
    PyErr_Format(PyExc_NotImplementedError, "No longer supported");
return( -1 );
}

static PyObject *PyFF_Font_get_os2codepages(PyFF_Font *self, void *UNUSED(closure)) {
    SplineFont *sf;

    if ( CheckIfFontClosed(self) )
return (NULL);
    sf = self->fv->sf;
    if ( !sf->pfminfo.hascodepages )
	OS2FigureCodePages(sf,sf->pfminfo.codepages);
	/* Don't mark it as having them though */
return( Py_BuildValue("(ii)", sf->pfminfo.codepages[0],sf->pfminfo.codepages[1]));
}

static int PyFF_Font_set_os2codepages(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) {
    SplineFont *sf;

    if ( CheckIfFontClosed(self) )
return (-1);
    sf = self->fv->sf;
    if ( value == NULL ) {
        sf->pfminfo.hascodepages = false;
return( 0 );
    }

    if ( !PyArg_ParseTuple(value,"ii", &sf->pfminfo.codepages[0], &sf->pfminfo.codepages[1]))
return(-1);
    sf->pfminfo.hascodepages = true;
return( 0 );
}

static PyObject *PyFF_Font_get_os2unicoderanges(PyFF_Font *self, void *UNUSED(closure)) {
    SplineFont *sf;

    if ( CheckIfFontClosed(self) )
return (NULL);
    sf = self->fv->sf;
    if ( !sf->pfminfo.hasunicoderanges )
	OS2FigureUnicodeRanges(sf,sf->pfminfo.unicoderanges);
	/* Don't mark it as having them though */
return( Py_BuildValue("(iiii)",
	sf->pfminfo.unicoderanges[0],sf->pfminfo.unicoderanges[1],
	sf->pfminfo.unicoderanges[2], sf->pfminfo.unicoderanges[3]));
}

static int PyFF_Font_set_os2unicoderanges(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) {
    SplineFont *sf;

    if ( CheckIfFontClosed(self) )
return (-1);
    sf = self->fv->sf;
    if ( value == NULL ) {
        sf->pfminfo.hasunicoderanges = false;
return( 0 );
    }

    if ( !PyArg_ParseTuple(value,"iiii",
	    &sf->pfminfo.unicoderanges[0], &sf->pfminfo.unicoderanges[1],
	    &sf->pfminfo.unicoderanges[2], &sf->pfminfo.unicoderanges[3]))
return(-1);
    sf->pfminfo.hasunicoderanges = true;
return( 0 );
}

static PyObject *PyFF_Font_get_loadvalidation_state(PyFF_Font *self, void *UNUSED(closure)) {
    if ( CheckIfFontClosed(self) )
return (NULL);
return( Py_BuildValue("i", self->fv->sf->loadvalidation_state));
}

static PyObject *PyFF_Font_get_privatevalidation_state(PyFF_Font *self, void *UNUSED(closure)) {
    if ( CheckIfFontClosed(self) )
return (NULL);
return( Py_BuildValue("i", ValidatePrivate(self->fv->sf)));
}

static PyObject *PyFF_Font_get_baseline(PyFF_Font *self, void *UNUSED(closure), struct Base *base) {
    PyObject *ret, *scripts, *langs, *features, *tags, *script, *poses, *lang, *feature;
    int cnt,i,j,k;
    struct basescript *bs;
    struct baselangextent *bl, *feat;

    if ( CheckIfFontClosed(self) )
return (NULL);
    if ( base==NULL )
Py_RETURN_NONE;

    ret  = PyTuple_New(2);
    tags = PyTuple_New(base->baseline_cnt);
    PyTuple_SetItem(ret,0,tags);
    for ( i=0; i<base->baseline_cnt; ++i )
	PyTuple_SetItem(tags,i,TagToPythonString(base->baseline_tags[i],false));

    for ( bs=base->scripts, cnt=0; bs!=NULL; bs=bs->next, ++cnt );
    scripts = PyTuple_New(cnt);
    PyTuple_SetItem(ret,1,scripts);
    for ( bs=base->scripts, i=0; bs!=NULL; bs=bs->next, ++i ) {
	script = PyTuple_New(4);
	PyTuple_SetItem(scripts,i,script);
	PyTuple_SetItem(script,0,TagToPythonString(bs->script,false));
	if ( base->baseline_cnt==0 ) {
	    Py_INCREF(Py_None); Py_INCREF(Py_None);
	    PyTuple_SetItem(script,1,Py_None);
	    PyTuple_SetItem(script,2,Py_None);
	} else {
	    PyTuple_SetItem(script,1,TagToPythonString(base->baseline_tags[bs->def_baseline],false));
	    poses = PyTuple_New(base->baseline_cnt);
	    for ( j=0; j<base->baseline_cnt; ++j )
		PyTuple_SetItem(poses,j,PyLong_FromLong(bs->baseline_pos[j]));
	    PyTuple_SetItem(script,2,poses);
	}
	for ( j=0, bl=bs->langs; bl!=NULL; bl=bl->next, ++j );
	langs = PyTuple_New(j);
	PyTuple_SetItem(script,3,langs);
	for ( j=0, bl=bs->langs; bl!=NULL; bl=bl->next, ++j ) {
	    lang = PyTuple_New(4);
	    PyTuple_SetItem(langs,j,lang);
	    PyTuple_SetItem(lang,0,TagToPythonString(bl->lang,false));
	    PyTuple_SetItem(lang,1,PyLong_FromLong(bl->descent));
	    PyTuple_SetItem(lang,2,PyLong_FromLong(bl->ascent));
	    for ( k=0, feat=bl->features; feat!=NULL; feat=feat->next, ++k );
	    features = PyTuple_New(k);
	    PyTuple_SetItem(lang,3,features);
	    for ( k=0, feat=bl->features; feat!=NULL; feat=feat->next, ++k ) {
		feature = PyTuple_New(3);
		PyTuple_SetItem(features,k,feature);
		PyTuple_SetItem(feature,0,TagToPythonString(feat->lang,false));
		PyTuple_SetItem(feature,1,PyLong_FromLong(feat->descent));
		PyTuple_SetItem(feature,2,PyLong_FromLong(feat->ascent));
	    }
	}
    }
return( ret );
}

static PyObject *PyFF_Font_get_horizontal_baseline(PyFF_Font *self, void *closure) {
    if ( CheckIfFontClosed(self) )
return (NULL);
return( PyFF_Font_get_baseline(self,closure,self->fv->sf->horiz_base));
}

static PyObject *PyFF_Font_get_vertical_baseline(PyFF_Font *self, void *closure) {
    if ( CheckIfFontClosed(self) )
return (NULL);
return( PyFF_Font_get_baseline(self,closure,self->fv->sf->vert_base));
}

static int PyFF_Font_set_baseline(PyFF_Font *self, PyObject *value, void *UNUSED(closure),struct Base **basep) {
    PyObject *basetags, *scripts;
    int basecnt,i;
    struct Base *base;
    int scriptcnt,langcnt,featcnt,j,k;
    struct basescript *bs = NULL, *lastbs;
    struct baselangextent *ln, *lastln;
    struct baselangextent *ft, *lastft;

    if ( CheckIfFontClosed(self) )
return (-1);
    if ( value==Py_None ) {
	BaseFree(*basep);
	*basep = NULL;
return( 0 );
    }
    if ( !PyArg_ParseTuple(value,"OO", &basetags, &scripts))
return( -1 );
    if ( basetags==Py_None )
	basecnt = 0;
    else {
	basecnt = PyTuple_Size(basetags);
	if ( basecnt<0 )
return( -1 );
    }
    base = chunkalloc(sizeof( struct Base));
    base->baseline_cnt = basecnt;
    base->baseline_tags = malloc(basecnt*sizeof(uint32_t));
    base->scripts = NULL;
    for ( i=0; i<basecnt; ++i ) {
	PyObject *str = PyTuple_GetItem(basetags,i);
	if ( !PyUnicode_Check(str) ) {
	    PyErr_Format(PyExc_TypeError, "Baseline tag must be a 4 character string" );
	    BaseFree(base);
return( -1 );
	}
	base->baseline_tags[i] = StrObjToTag(str,NULL);
	if ( base->baseline_tags[i]==BAD_TAG ) {
	    BaseFree(base);
return( -1 );
	}
    }

    lastbs = NULL;
    scriptcnt = PyTuple_Size(scripts);
    if ( scriptcnt<0 ) {
	BaseFree(base);
return( -1 );
    }
    for ( j=0; j<scriptcnt; ++j ) {
	PyObject *script = PyTuple_GetItem(scripts,j);
	char *scripttag, *def_baseln;
	PyObject *offsets, *langs;

	if ( !PyArg_ParseTuple(script,"szOO",&scripttag,&def_baseln,&offsets,&langs)) {
	    BaseFree(base);
return( -1 );
	}
	bs = chunkalloc(sizeof(struct basescript));
	bs->next = NULL;
	if ( lastbs==NULL )
	    base->scripts = bs;
	else
	    lastbs->next = bs;
	lastbs = bs;
	bs->script = StrToTag(scripttag,NULL);
	if ( bs->script == BAD_TAG ) {
	    BaseFree(base);
return( -1 );
	}
	if ( basecnt==0 && (def_baseln==NULL && offsets==NULL))
	    /* That's reasonable */;
	else if ( basecnt!=0 && (def_baseln!=NULL && offsets!=NULL)) {
	    /* Also reasonable */;
	    uint32_t tag = StrToTag(def_baseln,NULL);
	    if ( tag==BAD_TAG ) {
		BaseFree(base);
return( -1 );
	    }
	    if ( PyTuple_Size(offsets)!=basecnt ) {
		PyErr_Format(PyExc_TypeError, "There must be as many baseline positions as there are baslines, in a script" );
		BaseFree(base);
return( -1 );
	    }
	    for ( i=0; i<basecnt && base->baseline_tags[i]!=tag; ++i );
	    if ( i==basecnt ) {
		PyErr_Format(PyExc_TypeError, "A script's default baseline must be one of the baselines specified" );
		BaseFree(base);
return( -1 );
	    }
	    bs->def_baseline = i;
	    bs->baseline_pos = malloc(basecnt*sizeof(int16_t));
	    for ( i=0; i<basecnt; ++i ) {
		if ( !PyLong_Check(PyTuple_GetItem(offsets,i))) {
		    PyErr_Format(PyExc_TypeError, "Baseline positions must be integers");
		    BaseFree(base);
return( -1 );
		}
		bs->baseline_pos[i] = PyLong_AsLong(PyTuple_GetItem(offsets,i));
	    }
	} else {
	    BaseFree(base);
	    if ( basecnt==0 )
		PyErr_Format(PyExc_TypeError, "You did not specify any baselines, so you may not specify a default baseline, nor offsets in a script" );
	    else
		PyErr_Format(PyExc_TypeError, "You must specify a default baseline and offsets in a script" );
return( -1 );
	}

	if ( langs==Py_None )
    continue;			/* That's ok */
	if ( !PyTuple_Check(langs)) {
	    PyErr_Format(PyExc_TypeError, "The languages must be specified by a tuple");
	    BaseFree(base);
return( -1 );
	}
	lastln = NULL;
	langcnt = PyTuple_Size(langs);
	for ( i=0; i<langcnt; ++i ) {
	    PyObject *lang = PyTuple_GetItem(langs,i);
	    char *tag;
	    int min,max;
	    PyObject *features;

	    if ( !PyArg_ParseTuple(lang,"siiO",&tag,&min,&max,&features)) {
		BaseFree(base);
return( -1 );
	    }
	    ln = chunkalloc(sizeof(struct baselangextent));
	    if ( lastln==NULL )
		bs->langs = ln;
	    else
		lastln->next = ln;
	    lastln = ln;
	    ln->lang = StrToTag(tag,NULL);
	    if ( ln->lang == BAD_TAG ) {
		PyErr_Format(PyExc_TypeError, "A language tag must be a 4 character string");
		BaseFree(base);
return( -1 );
	    }
	    ln->descent = min;
	    ln->ascent  = max;

	    if ( features==Py_None )
	continue;			/* That's ok */
	    if ( !PyTuple_Check(features)) {
		PyErr_Format(PyExc_TypeError, "The features must be specified by a tuple");
		BaseFree(base);
return( -1 );
	    }
	    lastft = NULL;
	    featcnt = PyTuple_Size(features);
	    for ( k=0; k<featcnt; ++k ) {
		PyObject *feat = PyTuple_GetItem(features,k);
		char *tag;
		int min,max;

		if ( !PyArg_ParseTuple(feat,"sii",&tag,&min,&max)) {
		    BaseFree(base);
return( -1 );
		}
		ft = chunkalloc(sizeof(struct baselangextent));
		if ( lastft==NULL )
		    ln->features = ft;
		else
		    lastft->next = ft;
		lastft = ft;
		ft->lang = StrToTag(tag,NULL);
		if ( ft->lang == BAD_TAG ) {
		    BaseFree(base);
return( -1 );
		}
		ft->descent = min;
		ft->ascent  = max;
	    }
	}
    }
    BaseFree(*basep);
    *basep = base;
return( 0 );
}

static int PyFF_Font_set_horizontal_baseline(PyFF_Font *self,PyObject *value, void *closure) {
    if ( CheckIfFontClosed(self) )
return (-1);
return( PyFF_Font_set_baseline(self,value,closure,&self->fv->sf->horiz_base));
}

static int PyFF_Font_set_vertical_baseline(PyFF_Font *self,PyObject *value, void *closure) {
    if ( CheckIfFontClosed(self) )
return (-1);
return( PyFF_Font_set_baseline(self,value,closure,&self->fv->sf->vert_base));
}

static PyObject *PyFF_Font_get_path(PyFF_Font *self, void *UNUSED(closure)) {
    if ( CheckIfFontClosed(self) )
return (NULL);
    if ( self->fv->sf->origname==NULL )
Py_RETURN_NONE;
    else
return( Py_BuildValue("s", self->fv->sf->origname ));
}

static PyObject *PyFF_Font_get_sfd_path(PyFF_Font *self, void *UNUSED(closure)) {
    if ( CheckIfFontClosed(self) )
return (NULL);
    if ( self->fv->sf->filename==NULL )
Py_RETURN_NONE;
    else
return( Py_BuildValue("s", self->fv->sf->filename ));
}

static PyObject *PyFF_Font_get_OS2_panose(PyFF_Font *self, void *UNUSED(closure)) {
    int i;
    PyObject *tuple;
    SplineFont *sf;

    if ( CheckIfFontClosed(self) )
return (NULL);
    sf = self->fv->sf;
    if ( !sf->pfminfo.panose_set )
	SFDefaultOS2(sf);
    tuple = PyTuple_New(10);
    for ( i=0; i<10; ++i )
	PyTuple_SET_ITEM(tuple,i,Py_BuildValue("i",self->fv->sf->pfminfo.panose[i]));
return( tuple );
}

static int PyFF_Font_set_OS2_panose(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) {
    int panose[10], i;
    SplineFont *sf;

    if ( CheckIfFontClosed(self) )
return (-1);
    sf = self->fv->sf;
    if ( value==NULL ) {
	PyErr_Format(PyExc_TypeError, "Cannot delete panose");
return( -1 );
    }
    if ( !PyArg_ParseTuple(value,"iiiiiiiiii", &panose[0], &panose[1], &panose[2], &panose[3],
	    &panose[4], &panose[5], &panose[6], &panose[7], &panose[8], &panose[9] ))
return( -1 );

    if ( !sf->pfminfo.panose_set )
	SFDefaultOS2(sf);
    for ( i=0; i<10; ++i )
	sf->pfminfo.panose[i] = panose[i];
    sf->pfminfo.panose_set = true;
return( 0 );
}

static PyObject *PyFF_Font_get_OS2_vendor(PyFF_Font *self, void *UNUSED(closure)) {
    char buf[8];
    SplineFont *sf;

    if ( CheckIfFontClosed(self) )
return (NULL);
    sf = self->fv->sf;
    SFDefaultOS2(sf);
    buf[0] = sf->pfminfo.os2_vendor[0];
    buf[1] = sf->pfminfo.os2_vendor[1];
    buf[2] = sf->pfminfo.os2_vendor[2];
    buf[3] = sf->pfminfo.os2_vendor[3];
    buf[4] = '\0';
return( Py_BuildValue("s",buf));
}

static int PyFF_Font_set_OS2_vendor(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) {
    SplineFont *sf;
    const char *newv;

    if ( CheckIfFontClosed(self) )
return (-1);
    sf = self->fv->sf;
    if ( value==NULL ) {
	PyErr_Format(PyExc_TypeError, "Cannot delete vendor" );
return( -1 );
    } else if ((newv = PyUnicode_AsUTF8(value)) == NULL) {
        return -1;
    }

    if ( strlen( newv )>4 ) {
	PyErr_Format(PyExc_TypeError, "OS2 vendor is limited to 4 characters" );
return( -1 );
    }
    SFDefaultOS2(sf);
    strncpy(sf->pfminfo.os2_vendor, newv, 4);
    sf->pfminfo.panose_set = true;
return( 0 );
}

static PyObject *PyFF_Font_get_design_size(PyFF_Font *self, void *UNUSED(closure)) {
    /* Design size is expressed in tenths of points */
    if ( CheckIfFontClosed(self) )
return (NULL);
return( Py_BuildValue("d", self->fv->sf->design_size/10.0));
}

static int PyFF_Font_set_design_size(PyFF_Font *self, PyObject *value,
				     void *UNUSED(closure)) {
    double temp;

    if ( CheckIfFontClosed(self) )
return (-1);
    if ( value==NULL )
	self->fv->sf->design_size = 0;
    else if ( PyFloat_Check(value)) {
	temp = PyFloat_AsDouble(value);
	if ( PyErr_Occurred()!=NULL )
return( -1 );
	self->fv->sf->design_size = rint(10.0*temp);
    } else if ( PyLong_Check(value)) {
	int t = PyLong_AsLong(value);
	if ( PyErr_Occurred()!=NULL )
return( -1 );
	self->fv->sf->design_size = 10*t;
    }
return( 0 );
}

static PyObject *PyFF_Font_get_size_feature(PyFF_Font *self, void *UNUSED(closure)) {
    /* Size feature has two formats: Just a design size, or a design size */
    /*  and size bounds, id & name */
    /* First case means a tuple of one element, second a tuple of 5 */
    /* design size & bounds are measured in tenths of points */
    struct otfname *names;
    int i,cnt;
    PyObject *tuple;
    SplineFont *sf;

    if ( CheckIfFontClosed(self) )
        return (NULL);
    sf = self->fv->sf;
    if ( sf->design_size==0 )
        Py_RETURN_NONE;

    for ( names=sf->fontstyle_name, cnt=0; names!=NULL; names=names->next, ++cnt );
    if ( cnt==0 )
        return( Py_BuildValue("(d)", sf->design_size/10.0));
    tuple = PyTuple_New(cnt);
    for ( names=sf->fontstyle_name, cnt=0; names!=NULL; names=names->next, ++cnt ) {
	for ( i=0; sfnt_name_mslangs[i].name!=NULL ; ++i ) {
	    if ( sfnt_name_mslangs[i].flag == names->lang )
                break;
        }
	if ( sfnt_name_mslangs[i].flag == names->lang ) {
            PyTuple_SetItem(tuple,cnt,Py_BuildValue("ss", sfnt_name_mslangs[i].name, names->name));
        } else {
            PyTuple_SetItem(tuple,cnt,Py_BuildValue("is", names->lang, names->name));
        }
    }
    return( Py_BuildValue("(dddiO)", sf->design_size/10.0,
	sf->design_range_bottom/10.0, sf->design_range_top/10.,
	sf->fontstyle_id, tuple ));
}

static int PyFF_Font_set_size_feature(PyFF_Font *self,PyObject *value,
				      void *UNUSED(closure)) {
    double temp, top=0, bot=0;
    int id=0;
    PyObject *names=NULL;
    SplineFont *sf;
    int i;
    struct otfname *head, *last, *cur;
    const char *string;

    if ( CheckIfFontClosed(self) )
return (-1);
    sf = self->fv->sf;
    if ( value==NULL || value==Py_None ) {
	sf->design_size = 0;
	/* Setting the design size to zero means there will be no 'size' feature in the output */
return( 0 );
    }
    /* If they only specify a design size, we won't require that it be a tuple */
    /*  if it is in a tuple, remove it and treat as a single value */
    if ( PyTuple_Size(value)==1 )
	value = PyTuple_GetItem(value,0);
    if ( PyFloat_Check(value) || PyLong_Check(value)) {
	if ( PyFloat_Check(value))
	    temp = PyFloat_AsDouble(value);
	else
	    temp = PyLong_AsLong(value);
	if ( PyErr_Occurred()!=NULL )
return( -1 );
	sf->design_size = rint(10.0*temp);
	sf->design_range_bottom = 0;
	sf->design_range_top = 0;
	sf->fontstyle_id = 0;
	OtfNameListFree(sf->fontstyle_name);
	sf->fontstyle_name = NULL;
return( 0 );
    }

    if ( !PyArg_ParseTuple(value,"dddiO", &temp, &bot, &top, &id, &names )) {
        PyErr_Format(PyExc_TypeError,"Expecting 1 or 5 arguments");
return( -1 );
    }
    sf->design_size = rint(10.0*temp);
    sf->design_range_bottom = rint(10.0*bot);
    sf->design_range_top = rint(10.0*top);
    sf->fontstyle_id = id;

    if ( !PyTuple_Check(names) && !PyList_Check(names)) {
	PyErr_Format(PyExc_TypeError,"Final argument must be a tuple of tuples");
return( -1 );
    }
    head = last = NULL;
    for ( i=0; i<PySequence_Size(names); ++i ) {
	PyObject *subtuple = PySequence_GetItem(names,i);
	PyObject *val;
	int lang;

        if ( (!PyTuple_Check(subtuple) && !PyList_Check(subtuple)) ||
	         (PySequence_Size(subtuple) != 2) ) {
	    PyErr_Format(PyExc_TypeError, "Value must be a tuple of a language name and string" );
	    OtfNameListFree(head);
	    Py_XDECREF(subtuple);
return( -1 );
	}
	val = PySequence_GetItem(subtuple,0);
	if ( PyUnicode_Check(val) ) {
	    if ((string = PyUnicode_AsUTF8(val)) != NULL) {
		lang = FlagsFromString(string,sfnt_name_mslangs,"language");
	    }
	    if (string == NULL || lang == FLAG_UNKNOWN) {
		Py_DECREF(val);
		Py_DECREF(subtuple);
		OtfNameListFree(head);
		return -1;
	    }
	} else if ( PyLong_Check(val)) {
	    lang = PyLong_AsLong(val);
	} else {
	    Py_XDECREF(val);
	    Py_DECREF(subtuple);
	    PyErr_Format(PyExc_TypeError, "Language must be a string or an integer");
	    OtfNameListFree(head);
return( -1 );
	}
	Py_DECREF(val);
	val = PySequence_GetItem(subtuple,1);
	string = copy(PyUnicode_AsUTF8(val));
	Py_XDECREF(val);
	Py_DECREF(subtuple);
	if (string == NULL) {
	    OtfNameListFree(head);
	    PyErr_Format(PyExc_TypeError, "Name must be a string");
	    return -1;
	}
	cur = chunkalloc(sizeof( struct otfname ));
	cur->name = (char*)string; // last assignment to string was a copy
	cur->lang = lang;
	cur->next = NULL;
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;
    }
    OtfNameListFree(sf->fontstyle_name);
    sf->fontstyle_name = head;

return( 0 );
}

static PyObject *PyFF_Font_get_cidsubfontcnt(PyFF_Font *self, void *UNUSED(closure)) {
    if ( CheckIfFontClosed(self) )
return (NULL);
    if ( self->fv->cidmaster==NULL )
return( Py_BuildValue("i", 0));
return( Py_BuildValue("i", self->fv->cidmaster->subfontcnt));
}

static PyObject *PyFF_Font_get_cidsubfontnames(PyFF_Font *self, void *UNUSED(closure)) {
    PyObject *tuple;
    SplineFont *cidmaster;
    int i;

    if ( CheckIfFontClosed(self) )
return (NULL);
    cidmaster = self->fv->cidmaster;
    if ( cidmaster==NULL )
Py_RETURN_NONE;
    tuple = PyTuple_New(cidmaster->subfontcnt);
    for ( i=0; i<cidmaster->subfontcnt; ++i )
	PyTuple_SET_ITEM(tuple,i,Py_BuildValue("s",cidmaster->subfonts[i]->fontname));
return( tuple );
}

static PyObject *PyFF_Font_get_cidsubfont(PyFF_Font *self, void *UNUSED(closure)) {
    int i;
    SplineFont *cidmaster, *sf;

    if ( CheckIfFontClosed(self) )
return (NULL);
    cidmaster = self->fv->cidmaster;
    sf = self->fv->sf;
    if ( cidmaster==NULL )
return( Py_BuildValue("i", -1));
    for ( i=0; i<cidmaster->subfontcnt && sf!=cidmaster->subfonts[i]; ++i );
return( Py_BuildValue("i", i));
}

static int PyFF_Font_set_cidsubfont(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) {
    int i;
    SplineFont *cidmaster, *sf;
    EncMap *map;

    if ( CheckIfFontClosed(self) )
return (-1);
    cidmaster = self->fv->cidmaster;
    map = self->fv->map;
    if ( cidmaster==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "Not a cid-keyed font");
return( -1 );
    }
    if ( PyUnicode_Check(value)) {
	const char *name = PyUnicode_AsUTF8(value);
	if (name == NULL) {
	    return -1;
	}
	for ( i=cidmaster->subfontcnt-1; i>=0 && strcmp(name,cidmaster->subfonts[i]->fontname)!=0; ++i );
	if ( i<0 ) {
	    PyErr_Format(PyExc_EnvironmentError, "No subfont named %s", name);
return( -1 );
	}
    } else if ( PyLong_Check(value)) {
	i = PyLong_AsLong(value);
	if ( i<0 || i>=cidmaster->subfontcnt ) {
	    PyErr_Format(PyExc_EnvironmentError, "Subfont index %d out of bounds must be >=0 and <%d.", i, cidmaster->subfontcnt );
return( -1 );
	}
    } else {
	PyErr_Format(PyExc_TypeError, "Expected either a string (fontname) or an index when setting the subfont");
return( -1 );
    }

    sf = cidmaster->subfonts[i];

    MVDestroyAll(self->fv->sf);
    if ( sf->glyphcnt>self->fv->sf->glyphcnt ) {
	free(self->fv->selected);
	self->fv->selected = calloc(sf->glyphcnt,sizeof(char));
	if ( sf->glyphcnt>map->encmax )
	    map->map = realloc(map->map,(map->encmax = sf->glyphcnt)*sizeof(int));
	if ( sf->glyphcnt>map->backmax )
	    map->backmap = realloc(map->backmap,(map->backmax = sf->glyphcnt)*sizeof(int));
	for ( i=0; i<sf->glyphcnt; ++i )
	    map->map[i] = map->backmap[i] = i;
	map->enccount = sf->glyphcnt;
    }
    self->fv->sf = sf;
    if ( !no_windowing_ui ) {
	FVSetTitle(self->fv);
	FontViewReformatOne(self->fv);
    }
return( 0 );
}

static PyObject *PyFF_Font_get_is_cid(PyFF_Font *self, void *UNUSED(closure)) {
    SplineFont *cidmaster;

    if ( CheckIfFontClosed(self) )
return (NULL);
    cidmaster = self->fv->cidmaster;
    if ( cidmaster==NULL )
return( Py_BuildValue("i", 0));

return( Py_BuildValue("i", 1));
}

static PyObject *PyFF_Font_get_encoding(PyFF_Font *self, void *UNUSED(closure)) {
    if ( CheckIfFontClosed(self) )
return (NULL);
return( Py_BuildValue("s", self->fv->map->enc->enc_name));
}

static int PyFF_Font_set_encoding(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) {
    const char *encname;
    int ret;

    if ( CheckIfFontClosed(self) )
return (-1);
    if ( value==NULL ) {
	PyErr_Format(PyExc_TypeError, "Cannot delete encoding field" );
return( -1 );
    } else if ((encname = PyUnicode_AsUTF8(value)) == NULL) {
        return -1;
    }
    ret = SFReencode(self->fv->sf, encname, 0);
    if ( ret==-1 )
	PyErr_Format(PyExc_NameError, "Unknown encoding %s", encname);

return(ret);
}

/* Not really the right question now... but this is the closest we come */
static PyObject *PyFF_Font_get_is_quadratic(PyFF_Font *self, void *UNUSED(closure)) {
    if ( CheckIfFontClosed(self) )
return (NULL);
return( Py_BuildValue("i", self->fv->sf->layers[self->fv->active_layer].order2));
}

static int PyFF_Font_set_is_quadratic(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) {
    SplineFont *sf;
    int order2;

    if ( CheckIfFontClosed(self) )
return (-1);
    sf = self->fv->sf;
    if ( value==NULL ) {
	PyErr_Format(PyExc_TypeError, "Cannot delete is_quadratic field" );
return( -1 );
    }
    order2 = PyLong_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );
    if ( sf->layers[self->fv->active_layer].order2==order2 )
	/* Do Nothing */;
    else if ( order2 ) {
	SFCloseAllInstrs(sf);
	SFConvertToOrder2(sf);
    } else
	SFConvertToOrder3(sf);
return(0);
}

static PyObject *PyFF_Font_get_guide(PyFF_Font *self, void *UNUSED(closure)) {
    if ( CheckIfFontClosed(self) )
return (NULL);
return( (PyObject *) LayerFromLayer(&self->fv->sf->grid,NULL));
}

static int PyFF_Font_set_guide(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) {
    SplineSet *ss = NULL, *newss;
    int isquad = false;
    SplineFont *sf;
    Layer *guide;

    if ( CheckIfFontClosed(self) )
return (-1);
    if ( value==NULL ) {
	PyErr_Format(PyExc_TypeError, "Cannot delete guide field" );
return( -1 );
    }
    if ( PyType_IsSubtype(&PyFF_LayerType, Py_TYPE(value)) ) {
	isquad = ((PyFF_Layer *) value)->is_quadratic;
	ss = SSFromLayer( (PyFF_Layer *) value);
    } else if ( PyType_IsSubtype(&PyFF_ContourType, Py_TYPE(value)) ) {
	isquad = ((PyFF_Contour *) value)->is_quadratic;
	ss = SSFromContour( (PyFF_Contour *) value,NULL);
    } else {
	PyErr_Format( PyExc_TypeError, "Unexpected type" );
return( -1 );
    }
    if ( PyErr_Occurred() != NULL ) {
        SplinePointListsFree(ss);
	return( -1 );
    }
    sf = self->fv->sf;
    guide = &sf->grid;
    SplinePointListsFree(guide->splines);
    if ( sf->grid.order2!=isquad ) {
	if ( sf->grid.order2 )
	    newss = SplineSetsTTFApprox(ss);
	else
	    newss = SplineSetsPSApprox(ss);
	SplinePointListsFree(ss);
	ss = newss;
    }
    guide->splines = ss;
return( 0 );
}

static PyObject *PyFF_Font_get_mark_classes(PyFF_Font *self, void *UNUSED(closure)) {
    PyObject *tuple, *nametuple;
    SplineFont *sf;
    int i;

    if ( CheckIfFontClosed(self) )
return (NULL);
    sf = self->fv->sf;
    if ( sf->mark_class_cnt==0 )
Py_RETURN_NONE;

    tuple = PyTuple_New(sf->mark_class_cnt-1);
    for ( i=1; i<sf->mark_class_cnt; ++i ) {
	nametuple = TupleOfGlyphNames(sf->mark_classes[i],0);
	PyTuple_SetItem(tuple,i-1,Py_BuildValue("(sO)", sf->mark_class_names[i], nametuple));
    }
return( tuple );
}

static int PyFF_Font_set_mark_classes(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) {
    SplineFont *sf;
    int i, cnt;
    char **names, **classes;
    char *nm;
    PyObject *subtuple;

    if ( CheckIfFontClosed(self) )
        return (-1);
    sf = self->fv->sf;
    if ( value==NULL || value==Py_None ) {
        /* del font.markClasses  or  font.markClasses=None  removes old values */
        cnt = 0;
    } else if ( !PyTuple_Check(value) && !PyList_Check(value)) {
        PyErr_Format(PyExc_TypeError,"Expecting a tuple of tuples of names and glyphs");
        return( -1 );
    } else {
        /* we may get too many tuples, or too few (an empty tuple), or an error */
        cnt = PySequence_Size(value);
        if ( cnt==-1 )
            return( -1 );
        if ( cnt>=256 ) {
            PyErr_Format(PyExc_ValueError, "There may be at most 255 mark classes" );
            return( -1 );
        }
    }
    if ( cnt==0 ) {
        /* markClasses is being removed, delete any old, reset counts and pointers */
	MarkClassFree(sf->mark_class_cnt,sf->mark_classes,sf->mark_class_names);
	sf->mark_class_cnt = 0;
	sf->mark_classes = NULL;
	sf->mark_class_names = NULL;
        return( 0 );
    }

    names = malloc((cnt+1)*sizeof(char *));
    classes = malloc((cnt+1)*sizeof(char *));
    names[0] = classes[0] = NULL;
    /* Fill in names[] and classes[], starting at index 1 instead of 0 */
    for ( i=1; i<=cnt; ++i ) {
	names[i] = classes[i] = NULL;
        PyObject *seqItem = PySequence_GetItem(value,i-1);
	if ( !PyArg_ParseTuple(seqItem,"sO", &nm, &subtuple)) {
            PyErr_Format(PyExc_TypeError,"Expecting inner tuples to be name and glyphs");
            Py_DECREF(seqItem);
	    FreeStringArray( i, names );
	    FreeStringArray( i, classes );
            return( -1 );
	}
        Py_DECREF(seqItem);
        if ( strlen(nm)==0 ) {
            PyErr_Format(PyExc_TypeError,"Mark class name strings may not be empty");
            FreeStringArray( i, names );
            FreeStringArray( i, classes );
            return( -1 );
        }
	classes[i] = GlyphNamesFromTuple(subtuple);
	if ( classes[i]==NULL ) {
	    FreeStringArray( i, names );
	    FreeStringArray( i, classes );
            return( -1 );
	}
	names[i] = copy(nm);
    }

    MarkClassFree(sf->mark_class_cnt,sf->mark_classes,sf->mark_class_names);
    sf->mark_class_cnt = cnt+1; /* +1 because index 0 was skipped */
    sf->mark_classes = classes;
    sf->mark_class_names = names;

return( 0 );
}

static PyObject *PyFF_Font_get_em(PyFF_Font *self, void *UNUSED(closure)) {
    if ( CheckIfFontClosed(self) )
return (NULL);
return( Py_BuildValue("i", self->fv->sf->ascent + self->fv->sf->descent ));
}

static int PyFF_Font_set_em(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) {
    int newem, as, ds, oldem;
    SplineFont *sf;

    if ( CheckIfFontClosed(self) )
return (-1);
    if ( value==NULL ) {
	PyErr_Format(PyExc_TypeError, "Cannot delete em field" );
return( -1 );
    }
    if ( !PyLong_Check(value)) {
	PyErr_Format( PyExc_TypeError, "Unexpected type" );
return( -1 );
    }
    newem = PyLong_AsLong(value);
    if ( newem<10 || newem>=16*1024 ) {
	PyErr_Format(PyExc_ValueError, "Em size too big or too small" );
return( -1 );
    }
    sf = self->fv->sf;
    if ( (oldem = sf->ascent+sf->descent)<=0 ) oldem = 1;
    ds = rint( (double) newem * sf->descent / oldem );
    as = newem - ds;
    SFScaleToEm(sf,as,ds);
return( 0 );
}

static int PyFF_Font_SetMaxpValue(PyFF_Font *self,PyObject *value, const char *str) {
    SplineFont *sf;
    struct ttf_table *tab;
    int val;

    if ( CheckIfFontClosed(self) )
return (-1);
    sf = self->fv->sf;
    val = PyLong_AsLong(value);
    if ( PyErr_Occurred())
return( -1 );

    tab = SFFindTable(sf,CHR('m','a','x','p'));
    if ( tab==NULL ) {
	tab = chunkalloc(sizeof(struct ttf_table));
	tab->next = sf->ttf_tables;
	sf->ttf_tables = tab;
	tab->tag = CHR('m','a','x','p');
    }
    if ( tab->len<32 ) {
	tab->data = realloc(tab->data,32);
	memset(tab->data+tab->len,0,32-tab->len);
	if ( tab->len<16 )
	    tab->data[15] = 2;			/* Default zones to 2 */
	tab->len = tab->maxlen = 32;
    }
    if ( strmatch(str,"Zones")==0 )
	memputshort(tab->data,7*sizeof(uint16_t),val);
    else if ( strmatch(str,"TwilightPntCnt")==0 )
	memputshort(tab->data,8*sizeof(uint16_t),val);
    else if ( strmatch(str,"StorageCnt")==0 )
	memputshort(tab->data,9*sizeof(uint16_t),val);
    else if ( strmatch(str,"MaxStackDepth")==0 )
	memputshort(tab->data,12*sizeof(uint16_t),val);
    else if ( strmatch(str,"FDEFs")==0 )
	memputshort(tab->data,10*sizeof(uint16_t),val);
    else if ( strmatch(str,"IDEFs")==0 )
	memputshort(tab->data,11*sizeof(uint16_t),val);
return( 0 );
}

static PyObject *PyFF_Font_GetMaxpValue(PyFF_Font *self,const char *str) {
    SplineFont *sf;
    struct ttf_table *tab;
    uint8_t *data, dummy[32];
    int val;

    if ( CheckIfFontClosed(self) )
return (NULL);
    sf = self->fv->sf;
    memset(dummy,0,sizeof(dummy));
    dummy[15] = 2;
    tab = SFFindTable(sf,CHR('m','a','x','p'));
    if ( tab==NULL )
	data = dummy;
    else if ( tab->len<32 ) {
	memcpy(dummy,tab->data,tab->len);
	data = dummy;
    } else
	data = tab->data;

    if ( strmatch(str,"Zones")==0 )
	val = memushort(data,32,7*sizeof(uint16_t));
    else if ( strmatch(str,"TwilightPntCnt")==0 )
	val = memushort(data,32,8*sizeof(uint16_t));
    else if ( strmatch(str,"StorageCnt")==0 )
	val = memushort(data,32,9*sizeof(uint16_t));
    else if ( strmatch(str,"MaxStackDepth")==0 )
	val = memushort(data,32,12*sizeof(uint16_t));
    else if ( strmatch(str,"FDEFs")==0 )
	val = memushort(data,32,10*sizeof(uint16_t));
    else if ( strmatch(str,"IDEFs")==0 )
	val = memushort(data,32,11*sizeof(uint16_t));
    else
	val = -1;
return( Py_BuildValue("i",val));
}

static PyObject *PyFF_Font_get_maxp_IDEFs(PyFF_Font *self, void *UNUSED(closure)) {
    if ( CheckIfFontClosed(self) )
return (NULL);
return( PyFF_Font_GetMaxpValue(self,"IDEFs"));
}

static int PyFF_Font_set_maxp_IDEFs(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) {
    if ( CheckIfFontClosed(self) )
return (-1);
return( PyFF_Font_SetMaxpValue(self,value,"IDEFs"));
}

static PyObject *PyFF_Font_get_maxp_FDEFs(PyFF_Font *self, void *UNUSED(closure)) {
    if ( CheckIfFontClosed(self) )
return (NULL);
return( PyFF_Font_GetMaxpValue(self,"FDEFs"));
}

static int PyFF_Font_set_maxp_FDEFs(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) {
    if ( CheckIfFontClosed(self) )
return (-1);
return( PyFF_Font_SetMaxpValue(self,value,"FDEFs"));
}

static PyObject *PyFF_Font_get_maxp_maxStackDepth(PyFF_Font *self, void *UNUSED(closure)) {
    if ( CheckIfFontClosed(self) )
return (NULL);
return( PyFF_Font_GetMaxpValue(self,"MaxStackDepth"));
}

static int PyFF_Font_set_maxp_maxStackDepth(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) {
    if ( CheckIfFontClosed(self) )
return (-1);
return( PyFF_Font_SetMaxpValue(self,value,"MaxStackDepth"));
}

static PyObject *PyFF_Font_get_maxp_storageCnt(PyFF_Font *self, void *UNUSED(closure)) {
    if ( CheckIfFontClosed(self) )
return (NULL);
return( PyFF_Font_GetMaxpValue(self,"StorageCnt"));
}

static int PyFF_Font_set_maxp_storageCnt(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) {
    if ( CheckIfFontClosed(self) )
return (-1);
return( PyFF_Font_SetMaxpValue(self,value,"StorageCnt"));
}

static PyObject *PyFF_Font_get_maxp_twilightPtCnt(PyFF_Font *self, void *UNUSED(closure)) {
    if ( CheckIfFontClosed(self) )
return (NULL);
return( PyFF_Font_GetMaxpValue(self,"TwilightPntCnt"));
}

static int PyFF_Font_set_maxp_twilightPtCnt(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) {
    if ( CheckIfFontClosed(self) )
return (-1);
return( PyFF_Font_SetMaxpValue(self,value,"TwilightPntCnt"));
}

static PyObject *PyFF_Font_get_maxp_zones(PyFF_Font *self, void *UNUSED(closure)) {
    if ( CheckIfFontClosed(self) )
return (NULL);
return( PyFF_Font_GetMaxpValue(self,"Zones"));
}

static int PyFF_Font_set_maxp_zones(PyFF_Font *self,PyObject *value, void *UNUSED(closure)) {
    if ( CheckIfFontClosed(self) )
return (-1);
return( PyFF_Font_SetMaxpValue(self,value,"Zones"));
}

static PyObject *PyFF_Font_get_xHeight(PyFF_Font *self, void *UNUSED(closure)) {
    SplineFont *sf;
    double val;
    if ( CheckIfFontClosed(self) )
return (NULL);
    sf = self->fv->sf;
    val = SFXHeight(sf,self->fv->active_layer,true);
return( Py_BuildValue("d",val));
}

static PyObject *PyFF_Font_get_capHeight(PyFF_Font *self, void *UNUSED(closure)) {
    SplineFont *sf;
    double val;
    if ( CheckIfFontClosed(self) )
return (NULL);
    sf = self->fv->sf;
    val = SFCapHeight(sf,self->fv->active_layer,true);
return( Py_BuildValue("d",val));
}

static PyGetSetDef PyFF_Font_getset[] = {
    {(char *)"userdata",
     (getter)PyFF_Font_get_temporary, (setter)PyFF_Font_set_temporary,
     (char *)"arbitrary (non-persistent) user data (deprecated name for temporary)", NULL},
    {(char *)"temporary",
     (getter)PyFF_Font_get_temporary, (setter)PyFF_Font_set_temporary,
     (char *)"arbitrary (non-persistent) user data", NULL},
    {(char *)"persistant",		/* I documented this member with the wrong spelling... so support it */
     (getter)PyFF_Font_get_persistent, (setter)PyFF_Font_set_persistent,
     (char *)"arbitrary persistent user data", NULL},
    {(char *)"persistent",
     (getter)PyFF_Font_get_persistent, (setter)PyFF_Font_set_persistent,
     (char *)"arbitrary persistent user data", NULL},
    {(char *)"selection",
     (getter)PyFF_Font_get_selection, (setter)PyFF_Font_set_selection,
     (char *)"The font's selection array", NULL},
    {(char *)"activeLayer",
     (getter)PyFF_Font_get_activeLayer, (setter)PyFF_Font_set_activeLayer,
     (char *)"The currently active layer in the font", NULL},
    {(char *)"sfnt_names",
     (getter)PyFF_Font_get_sfntnames, (setter)PyFF_Font_set_sfntnames,
     (char *)"The sfnt 'name' table. A tuple of all ms names.\nEach name is itself a tuple of strings (language,strid,name)\nMac names will be automagically created from ms names", NULL},
    {(char *)"bitmapSizes",
     (getter)PyFF_Font_get_bitmapSizes, (setter)PyFF_Font_set_bitmapSizes,
     (char *)"A tuple of sizes of all bitmaps associated with the font", NULL},
    {(char *)"gpos_lookups",
     (getter)PyFF_Font_get_gpos_lookups, NULL,
     (char *)"The names of all lookups in the font's GPOS table (readonly)", NULL},
    {(char *)"gsub_lookups",
     (getter)PyFF_Font_get_gsub_lookups, NULL,
     (char *)"The names of all lookups in the font's GSUB table (readonly)", NULL},
    {(char *)"horizontalBaseline",
     (getter)PyFF_Font_get_horizontal_baseline, (setter)PyFF_Font_set_horizontal_baseline,
     (char *)"Horizontal baseline data, if any", NULL},
    {(char *)"verticalBaseline",
     (getter)PyFF_Font_get_vertical_baseline, (setter)PyFF_Font_set_vertical_baseline,
     (char *)"Vertical baseline data, if any", NULL},
    {(char *)"private",
     (getter)PyFF_Font_get_private, NULL,
     (char *)"The font's PostScript private dictionary (You may not set this field, but you may set things in it)", NULL},
    {(char *)"math",
     (getter)PyFF_Font_get_math, NULL,
     (char *)"The font's math constants (You may not set this field, but you may set things in it)", NULL},
    {(char *)"texparameters",
     (getter)PyFF_Font_get_texparams, NULL,
     (char *)"The font's TeX font parameters", NULL},
    {(char *)"cvt",
     (getter)PyFF_Font_get_cvt, (setter)PyFF_Font_set_cvt,
     (char *)"The font's TrueType cvt table", NULL},
    {(char *)"path",
     (getter)PyFF_Font_get_path, NULL,
     (char *)"filename of the original font file loaded (readonly)", NULL},
    {(char *)"sfd_path",
     (getter)PyFF_Font_get_sfd_path, NULL,
     (char *)"filename of the sfd file containing this font (if any) (readonly)", NULL},
    {(char *)"default_base_filename",
     (getter)PyFF_Font_get_defbasefilename, (setter)PyFF_Font_set_defbasefilename,
     (char *)"The default base for the filename when generating a font", NULL},
    {(char *)"fontname",
     (getter)PyFF_Font_get_fontname, (setter)PyFF_Font_set_fontname,
     (char *)"font name", NULL},
    {(char *)"fullname",
     (getter)PyFF_Font_get_fullname, (setter)PyFF_Font_set_fullname,
     (char *)"full name", NULL},
    {(char *)"familyname",
     (getter)PyFF_Font_get_familyname, (setter)PyFF_Font_set_familyname,
     (char *)"family name", NULL},
    {(char *)"weight",
     (getter)PyFF_Font_get_weight, (setter)PyFF_Font_set_weight,
     (char *)"weight (PS)", NULL},
    {(char *)"copyright",
     (getter)PyFF_Font_get_copyright, (setter)PyFF_Font_set_copyright,
     (char *)"copyright (PS)", NULL},
    {(char *)"version",
     (getter)PyFF_Font_get_version, (setter)PyFF_Font_set_version,
     (char *)"font version (PS)", NULL},
    {(char *)"comment",
     (getter)PyFF_Font_get_comments, (setter)PyFF_Font_set_comments,
     (char *)"A comment associated with the font. Can be anything", NULL},
    {(char *)"fontlog",
     (getter)PyFF_Font_get_fontlog, (setter)PyFF_Font_set_fontlog,
     (char *)"A comment associated with the font. Can be anything", NULL},
    {(char *)"xuid",
     (getter)PyFF_Font_get_xuid, (setter)PyFF_Font_set_xuid,
     (char *)"PostScript eXtended Unique ID", NULL},
    {(char *)"fondname",
     (getter)PyFF_Font_get_fondname, (setter)PyFF_Font_set_fondname,
     (char *)"Mac FOND resource name", NULL},
    {(char *)"cidregistry",
     (getter)PyFF_Font_get_cidcidregistry, (setter)PyFF_Font_set_cidcidregistry,
     (char *)"CID Registry", NULL},
    {(char *)"cidordering",
     (getter)PyFF_Font_get_cidordering, (setter)PyFF_Font_set_cidordering,
     (char *)"CID Ordering", NULL},
    {(char *)"cidsupplement",
     (getter)PyFF_Font_get_supplement, (setter)PyFF_Font_set_supplement,
     (char *)"CID Supplement", NULL},
    {(char *)"cidsubfont",
     (getter)PyFF_Font_get_cidsubfont, (setter)PyFF_Font_set_cidsubfont,
     (char *)"Returns the index of the current subfont of a cid-keyed font (or -1), and allows you to change the subfont.", NULL},
    {(char *)"cidsubfontcnt",
     (getter)PyFF_Font_get_cidsubfontcnt, NULL,
     (char *)"The number of sub fonts that make up a CID keyed font (readonly)", NULL},
    {(char *)"cidsubfontnames",
     (getter)PyFF_Font_get_cidsubfontnames, NULL,
     (char *)"The names of all the sub fonts that make up a CID keyed font (readonly)", NULL},
    {(char *)"cidfontname",
     (getter)PyFF_Font_get_cidfontname, (setter)PyFF_Font_set_cidfontname,
     (char *)"Font name of the cid-keyed font as a whole.", NULL},
    {(char *)"cidfamilyname",
     (getter)PyFF_Font_get_cidfamilyname, (setter)PyFF_Font_set_cidfamilyname,
     (char *)"Family name of the cid-keyed font as a whole.", NULL},
    {(char *)"cidfullname",
     (getter)PyFF_Font_get_cidfullname, (setter)PyFF_Font_set_cidfullname,
     (char *)"Full name of the cid-keyed font as a whole.", NULL},
    {(char *)"cidweight",
     (getter)PyFF_Font_get_cidweight, (setter)PyFF_Font_set_cidweight,
     (char *)"Weight of the cid-keyed font as a whole.", NULL},
    {(char *)"cidcopyright",
     (getter)PyFF_Font_get_cidcopyright, (setter)PyFF_Font_set_cidcopyright,
     (char *)"Copyright message of the cid-keyed font as a whole.", NULL},
    {(char *)"cidversion",
     (getter)PyFF_Font_get_cidversion, (setter)PyFF_Font_set_cidversion,
     (char *)"CID Version", NULL},
    {(char *)"iscid",		/* This should be is_cid, but due to an early misprint I now include both */
     (getter)PyFF_Font_get_is_cid, NULL,
     (char *)"Whether the font is a cid-keyed font. (readonly)", NULL},
    {(char *)"is_cid",
     (getter)PyFF_Font_get_is_cid, NULL,
     (char *)"Whether the font is a cid-keyed font. (readonly)", NULL},
    {(char *)"italicangle",
     (getter)PyFF_Font_get_italicangle, (setter)PyFF_Font_set_italicangle,
     (char *)"The Italic angle (skewedness) of the font", NULL},
    {(char *)"creationtime",
     (getter) PyFF_Font_get_creationtime, NULL,
     (char*)"Font creation time. (readonly)", NULL},
    {(char *)"upos",
     (getter)PyFF_Font_get_upos, (setter)PyFF_Font_set_upos,
     (char *)"Underline Position", NULL},
    {(char *)"uwidth",
     (getter)PyFF_Font_get_uwidth, (setter)PyFF_Font_set_uwidth,
     (char *)"Underline Width", NULL},
    {(char *)"strokewidth",
     (getter)PyFF_Font_get_strokewidth, (setter)PyFF_Font_set_strokewidth,
     (char *)"Stroke Width", NULL},
    {(char *)"ascent",
     (getter)PyFF_Font_get_ascent, (setter)PyFF_Font_set_ascent,
     (char *)"Font Ascent", NULL},
    {(char *)"descent",
     (getter)PyFF_Font_get_descent, (setter)PyFF_Font_set_descent,
     (char *)"Font Descent", NULL},
    {(char *)"xHeight",
     (getter)PyFF_Font_get_xHeight, NULL,
     (char *)"X Height of font (negative number means could not be computed (ie. no lowercase glyphs))", NULL},
    {(char *)"capHeight",
     (getter)PyFF_Font_get_capHeight, NULL,
     (char *)"Cap Height of font (negative number means could not be computed (ie. no uppercase glyphs))", NULL},
    {(char *)"em",
     (getter)PyFF_Font_get_em, (setter)PyFF_Font_set_em,
     (char *)"Em size", NULL},
    {(char *)"sfntRevision",
     (getter)PyFF_Font_get_sfntRevision, (setter)PyFF_Font_set_sfntRevision,
     (char *)"sfnt revision number", NULL},
    {(char *)"woffMajor",
     (getter)PyFF_Font_get_woffMajor, (setter)PyFF_Font_set_woffMajor,
     (char *)"woff major version number", NULL},
    {(char *)"woffMinor",
     (getter)PyFF_Font_get_woffMinor, (setter)PyFF_Font_set_woffMinor,
     (char *)"woff minor version number", NULL},
    {(char *)"woffMetadata",
     (getter)PyFF_Font_get_woffMetadata, (setter)PyFF_Font_set_woffMetadata,
     (char *)"woff meta data string (unparsed xml)", NULL},
    {(char *)"vertical_origin",
     (getter)PyFF_Font_get_vertical_origin, (setter)PyFF_Font_set_vertical_origin,
     (char *)"Vertical Origin (No longer supported, use BASE table instead)", NULL},
    {(char *)"uniqueid",
     (getter)PyFF_Font_get_uniqueid, (setter)PyFF_Font_set_uniqueid,
     (char *)"PostScript Unique ID", NULL},
    {(char *)"layer_cnt",
     (getter)PyFF_Font_get_layer_cnt, NULL,
     (char *)"Returns the number of layers in the font (readonly)", NULL},
    {(char *)"layers",
     (getter)PyFF_Font_get_layers, NULL,
     (char *)"Returns a dictionary like object with information on the layers of the font", NULL},
    {(char *)"loadState",
     (getter)PyFF_Font_get_loadvalidation_state, NULL,
     (char *)"A bitmask indicating non-fatal errors found when loading the font (readonly)", NULL},
    {(char *)"privateState",
     (getter)PyFF_Font_get_privatevalidation_state, NULL,
     (char *)"A bitmask indicating errors in the (PostScript) Private dictionary", NULL},
    {(char *)"macstyle",
     (getter)PyFF_Font_get_macstyle, (setter)PyFF_Font_set_macstyle,
     (char *)"Mac Style Bits", NULL},
    {(char *)"design_size",
     (getter)PyFF_Font_get_design_size, (setter)PyFF_Font_set_design_size,
     (char *)"Point size for which this font was designed", NULL},
    {(char *)"size_feature",
     (getter)PyFF_Font_get_size_feature, (setter)PyFF_Font_set_size_feature,
     (char *)"A tuple containing the info needed for the 'size' feature", NULL},
    {(char *)"gasp_version",
     (getter)PyFF_Font_get_gasp_version, (setter)PyFF_Font_set_gasp_version,
     (char *)"Gasp table version number", NULL},
    {(char *)"gasp",
     (getter)PyFF_Font_get_gasp, (setter)PyFF_Font_set_gasp,
     (char *)"Gasp table as a tuple", NULL},
    {(char *)"maxp_zones",
     (getter)PyFF_Font_get_maxp_zones, (setter)PyFF_Font_set_maxp_zones,
     (char *)"The number of zones used in the tt program", NULL},
    {(char *)"maxp_twilightPtCnt",
     (getter)PyFF_Font_get_maxp_twilightPtCnt, (setter)PyFF_Font_set_maxp_twilightPtCnt,
     (char *)"The number of points in the twilight zone of the tt program", NULL},
    {(char *)"maxp_storageCnt",
     (getter)PyFF_Font_get_maxp_storageCnt, (setter)PyFF_Font_set_maxp_storageCnt,
     (char *)"The number of storage locations used by the tt program", NULL},
    {(char *)"maxp_maxStackDepth",
     (getter)PyFF_Font_get_maxp_maxStackDepth, (setter)PyFF_Font_set_maxp_maxStackDepth,
     (char *)"The maximum stack depth used by the tt program", NULL},
    {(char *)"maxp_FDEFs",
     (getter)PyFF_Font_get_maxp_FDEFs, (setter)PyFF_Font_set_maxp_FDEFs,
     (char *)"The number of function definitions used by the tt program", NULL},
    {(char *)"maxp_IDEFs",
     (getter)PyFF_Font_get_maxp_IDEFs, (setter)PyFF_Font_set_maxp_IDEFs,
     (char *)"The number of instruction definitions used by the tt program", NULL},
    {(char *)"os2_version",
     (getter)PyFF_Font_get_os2_version, (setter)PyFF_Font_set_os2_version,
     (char *)"OS/2 table version number", NULL},
    {(char *)"os2_weight",
     (getter)PyFF_Font_get_OS2_weight, (setter)PyFF_Font_set_OS2_weight,
     (char *)"OS/2 weight", NULL},
    {(char *)"os2_width",
     (getter)PyFF_Font_get_OS2_width, (setter)PyFF_Font_set_OS2_width,
     (char *)"OS/2 width", NULL},
    {(char *)"os2_stylemap",
     (getter)PyFF_Font_get_OS2_stylemap, (setter)PyFF_Font_set_OS2_stylemap,
     (char *)"OS/2 stylemap", NULL},
    {(char *)"os2_fstype",
     (getter)PyFF_Font_get_OS2_fstype, (setter)PyFF_Font_set_OS2_fstype,
     (char *)"OS/2 fstype", NULL},
    {(char *)"head_optimized_for_cleartype",
     (getter)PyFF_Font_get_head_optimized_for_cleartype, (setter)PyFF_Font_set_head_optimized_for_cleartype,
     (char *)"Whether the font is optimized for cleartype", NULL},
    {(char *)"hhea_linegap",
     (getter)PyFF_Font_get_OS2_linegap, (setter)PyFF_Font_set_OS2_linegap,
     (char *)"hhea linegap", NULL},
    {(char *)"hhea_ascent",
     (getter)PyFF_Font_get_OS2_hhead_ascent, (setter)PyFF_Font_set_OS2_hhead_ascent,
     (char *)"hhea ascent", NULL},
    {(char *)"hhea_descent",
     (getter)PyFF_Font_get_OS2_hhead_descent, (setter)PyFF_Font_set_OS2_hhead_descent,
     (char *)"hhea descent", NULL},
    {(char *)"hhea_ascent_add",
     (getter)PyFF_Font_get_OS2_hheadascent_add, (setter)PyFF_Font_set_OS2_hheadascent_add,
     (char *)"Whether the hhea_ascent field is used as is, or as an offset applied to the value FontForge thinks appropriate", NULL},
    {(char *)"hhea_descent_add",
     (getter)PyFF_Font_get_OS2_hheaddescent_add, (setter)PyFF_Font_set_OS2_hheaddescent_add,
     (char *)"Whether the hhea_descent field is used as is, or as an offset applied to the value FontForge thinks appropriate", NULL},
    {(char *)"vhea_linegap",
     (getter)PyFF_Font_get_OS2_vlinegap, (setter)PyFF_Font_set_OS2_vlinegap,
     (char *)"vhea linegap", NULL},
    {(char *)"os2_typoascent",
     (getter)PyFF_Font_get_OS2_os2_typoascent, (setter)PyFF_Font_set_OS2_os2_typoascent,
     (char *)"OS/2 Typographic Ascent", NULL},
    {(char *)"os2_typodescent",
     (getter)PyFF_Font_get_OS2_os2_typodescent, (setter)PyFF_Font_set_OS2_os2_typodescent,
     (char *)"OS/2 Typographic Descent", NULL},
    {(char *)"os2_typolinegap",
     (getter)PyFF_Font_get_OS2_os2_typolinegap, (setter)PyFF_Font_set_OS2_os2_typolinegap,
     (char *)"OS/2 Typographic Linegap", NULL},
    {(char *)"os2_typoascent_add",
     (getter)PyFF_Font_get_OS2_typoascent_add, (setter)PyFF_Font_set_OS2_typoascent_add,
     (char *)"Whether the os2_typoascent field is used as is, or as an offset applied to the value FontForge thinks appropriate", NULL},
    {(char *)"os2_typodescent_add",
     (getter)PyFF_Font_get_OS2_typodescent_add, (setter)PyFF_Font_set_OS2_typodescent_add,
     (char *)"Whether the os2_typodescent field is used as is, or as an offset applied to the value FontForge thinks appropriate", NULL},
    {(char *)"os2_winascent",
     (getter)PyFF_Font_get_OS2_os2_winascent, (setter)PyFF_Font_set_OS2_os2_winascent,
     (char *)"OS/2 Windows Ascent", NULL},
    {(char *)"os2_windescent",
     (getter)PyFF_Font_get_OS2_os2_windescent, (setter)PyFF_Font_set_OS2_os2_windescent,
     (char *)"OS/2 Windows Descent", NULL},
    {(char *)"os2_winascent_add",
     (getter)PyFF_Font_get_OS2_winascent_add, (setter)PyFF_Font_set_OS2_winascent_add,
     (char *)"Whether the os2_winascent field is used as is, or as an offset applied to the value FontForge thinks appropriate", NULL},
    {(char *)"os2_windescent_add",
     (getter)PyFF_Font_get_OS2_windescent_add, (setter)PyFF_Font_set_OS2_windescent_add,
     (char *)"Whether the os2_windescent field is used as is, or as an offset applied to the value FontForge thinks appropriate", NULL},
    {(char *)"os2_subxsize",
     (getter)PyFF_Font_get_OS2_os2_subxsize, (setter)PyFF_Font_set_OS2_os2_subxsize,
     (char *)"OS/2 Subscript XSize", NULL},
    {(char *)"os2_subxoff",
     (getter)PyFF_Font_get_OS2_os2_subxoff, (setter)PyFF_Font_set_OS2_os2_subxoff,
     (char *)"OS/2 Subscript XOffset", NULL},
    {(char *)"os2_subysize",
     (getter)PyFF_Font_get_OS2_os2_subysize, (setter)PyFF_Font_set_OS2_os2_subysize,
     (char *)"OS/2 Subscript YSize", NULL},
    {(char *)"os2_subyoff",
     (getter)PyFF_Font_get_OS2_os2_subyoff, (setter)PyFF_Font_set_OS2_os2_subyoff,
     (char *)"OS/2 Subscript YOffset", NULL},
    {(char *)"os2_supxsize",
     (getter)PyFF_Font_get_OS2_os2_supxsize, (setter)PyFF_Font_set_OS2_os2_supxsize,
     (char *)"OS/2 Superscript XSize", NULL},
    {(char *)"os2_supxoff",
     (getter)PyFF_Font_get_OS2_os2_supxoff, (setter)PyFF_Font_set_OS2_os2_supxoff,
     (char *)"OS/2 Superscript XOffset", NULL},
    {(char *)"os2_supysize",
     (getter)PyFF_Font_get_OS2_os2_supysize, (setter)PyFF_Font_set_OS2_os2_supysize,
     (char *)"OS/2 Superscript YSize", NULL},
    {(char *)"os2_supyoff",
     (getter)PyFF_Font_get_OS2_os2_supyoff, (setter)PyFF_Font_set_OS2_os2_supyoff,
     (char *)"OS/2 Superscript YOffset", NULL},
    {(char *)"os2_strikeysize",
     (getter)PyFF_Font_get_OS2_os2_strikeysize, (setter)PyFF_Font_set_OS2_os2_strikeysize,
     (char *)"OS/2 Strikethrough YSize", NULL},
    {(char *)"os2_strikeypos",
     (getter)PyFF_Font_get_OS2_os2_strikeypos, (setter)PyFF_Font_set_OS2_os2_strikeypos,
     (char *)"OS/2 Strikethrough YPosition", NULL},
    {(char *)"os2_capheight",
     (getter)PyFF_Font_get_OS2_os2_capheight, (setter)PyFF_Font_set_OS2_os2_capheight,
     (char *)"OS/2 Capital Height", NULL},
    {(char *)"os2_xheight",
     (getter)PyFF_Font_get_OS2_os2_xheight, (setter)PyFF_Font_set_OS2_os2_xheight,
     (char *)"OS/2 x Height", NULL},
    {(char *)"os2_family_class",
     (getter)PyFF_Font_get_OS2_os2_family_class, (setter)PyFF_Font_set_OS2_os2_family_class,
     (char *)"OS/2 Family Class", NULL},
    {(char *)"os2_use_typo_metrics",
     (getter)PyFF_Font_get_use_typo_metrics, (setter)PyFF_Font_set_use_typo_metrics,
     (char *)"OS/2 Flag MS thinks is necessary to encourage people to follow the standard and use typographic metrics", NULL},
    {(char *)"os2_weight_width_slope_only",
     (getter)PyFF_Font_get_weight_width_slope_only, (setter)PyFF_Font_set_weight_width_slope_only,
     (char *)"OS/2 Flag MS thinks is necessary", NULL},
    {(char *)"os2_codepages",
     (getter)PyFF_Font_get_os2codepages, (setter)PyFF_Font_set_os2codepages,
     (char *)"The 2 element OS/2 codepage tuple", NULL},
    {(char *)"os2_unicoderanges",
     (getter)PyFF_Font_get_os2unicoderanges, (setter)PyFF_Font_set_os2unicoderanges,
     (char *)"The 4 element OS/2 unicode ranges tuple", NULL},
    {(char *)"os2_panose",
     (getter)PyFF_Font_get_OS2_panose, (setter)PyFF_Font_set_OS2_panose,
     (char *)"The 10 element OS/2 Panose tuple", NULL},
    {(char *)"os2_vendor",
     (getter)PyFF_Font_get_OS2_vendor, (setter)PyFF_Font_set_OS2_vendor,
     (char *)"The 4 character OS/2 vendor string", NULL},
    {(char *)"changed",
     (getter)PyFF_Font_get_changed, (setter)PyFF_Font_set_changed,
     (char *)"Flag indicating whether the font has been changed since it was loaded (read only)", NULL},
    {(char *)"isnew",
     (getter)PyFF_Font_get_new, NULL,
     (char *)"Flag indicating whether the font is new (read only)", NULL},
    {(char *)"hasvmetrics",
     (getter)PyFF_Font_get_hasvmetrics, (setter)PyFF_Font_set_hasvmetrics,
     (char *)"Flag indicating whether the font contains vertical metrics", NULL},
    {(char *)"onlybitmaps",
     (getter)PyFF_Font_get_onlybitmaps, (setter)PyFF_Font_set_onlybitmaps,
     (char *)"Flag indicating whether the font contains bitmap strikes but no outlines", NULL},
    {(char *)"encoding",
     (getter)PyFF_Font_get_encoding, (setter)PyFF_Font_set_encoding,
     (char *)"The encoding used for indexing the font", NULL },
    {(char *)"is_quadratic",
     (getter)PyFF_Font_get_is_quadratic, (setter)PyFF_Font_set_is_quadratic,
     (char *)"Flag indicating whether the font contains quadratic splines (truetype) or cubic (postscript)", NULL},
    {(char *)"multilayer",
     (getter)PyFF_Font_get_multilayer, NULL,
     (char *)"Flag indicating whether the font is multilayered (type3) or not (readonly)", NULL},
    {(char *)"strokedfont",
     (getter)PyFF_Font_get_strokedfont, (setter)PyFF_Font_set_strokedfont,
     (char *)"Flag indicating whether the font is a stroked font or not", NULL},
    {(char *)"guide",
     (getter)PyFF_Font_get_guide, (setter)PyFF_Font_set_guide,
     (char *)"The Contours that make up the guide layer of the font", NULL},
    {(char *)"markClasses",
     (getter)PyFF_Font_get_mark_classes, (setter)PyFF_Font_set_mark_classes,
     (char *)"A tuple each entry of which is itself a tuple containing a mark-class-name and a tuple of glyph-names", NULL},
    PYGETSETDEF_EMPTY  /* Sentinel */
};

/* ************************************************************************** */
/* Font Methods */
/* ************************************************************************** */

static PyObject *PyFFFont_GetTableData(PyFF_Font *self, PyObject *args) {
    char *table_name;
    uint32_t tag;
    struct ttf_table *tab;
    PyObject *binstr;

    if ( CheckIfFontClosed(self) )
return (NULL);
    if ( !PyArg_ParseTuple(args,"s",&table_name) )
return( NULL );
    tag = StrToTag(table_name,NULL);
    if ( tag==BAD_TAG )
return( NULL );

    for ( tab=self->fv->sf->ttf_tables; tab!=NULL && tab->tag!=tag; tab=tab->next );
    if ( tab==NULL )
	for ( tab=self->fv->sf->ttf_tab_saved; tab!=NULL && tab->tag!=tag; tab=tab->next );

    if ( tab==NULL )
Py_RETURN_NONE;

    binstr = PyBytes_FromStringAndSize((char *) tab->data,tab->len);
return( binstr );
}

static void TableAddInstrs(SplineFont *sf, uint32_t tag,int replace,
			   uint8_t *instrs,int icnt) {
    struct ttf_table *tab;

    for ( tab=sf->ttf_tables; tab!=NULL && tab->tag!=tag; tab=tab->next );
    if ( tab==NULL )
	for ( tab=sf->ttf_tab_saved; tab!=NULL && tab->tag!=tag; tab=tab->next );

    if ( replace && tab!=NULL ) {
	free(tab->data);
	tab->data = NULL;
	tab->len = tab->maxlen = 0;
    }
    if ( icnt==0 )
return;
    if ( tab==NULL ) {
	tab = chunkalloc(sizeof( struct ttf_table ));
	tab->tag = tag;
	if ( tag==CHR('p','r','e','p') || tag==CHR('f','p','g','m') ||
		tag==CHR('c','v','t',' ') || tag==CHR('m','a','x','p') ) {
	    tab->next = sf->ttf_tables;
	    sf->ttf_tables = tab;
	} else {
	    tab->next = sf->ttf_tab_saved;
	    sf->ttf_tab_saved = tab;
	}
    }
    if ( tab->data==NULL ) {
	tab->data = malloc(icnt);
	memcpy(tab->data,instrs,icnt);
	tab->len = icnt;
    } else {
	uint8_t *newi = malloc(icnt+tab->len);
	memcpy(newi,tab->data,tab->len);
	memcpy(newi+tab->len,instrs,icnt);
	free(tab->data);
	tab->data = newi;
	tab->len += icnt;
    }
    tab->maxlen = tab->len;
}

static PyObject *PyFFFont_SetTableData(PyFF_Font *self, PyObject *args) {
    char *table_name;
    uint32_t tag;
    PyObject *tuple;
    uint8_t *instrs;
    int icnt, i;

    if ( CheckIfFontClosed(self) )
return (NULL);
    if ( !PyArg_ParseTuple(args,"sO",&table_name,&tuple) )
return( NULL );
    tag = StrToTag(table_name,NULL);
    if ( tag==BAD_TAG )
return( NULL );

    if ( tuple==Py_None ) {
	SFRemoveSavedTable(self->fv->sf,tag);
Py_RETURN(self);
    }

    if ( !PySequence_Check(tuple)) {
	PyErr_Format(PyExc_TypeError, "Argument must be a tuple" );
return( NULL );
    }
    if ( PyBytes_Check(tuple)) {
	char *space; Py_ssize_t len;
	PyBytes_AsStringAndSize(tuple,&space,&len);
	instrs = calloc(len,sizeof(uint8_t));
	icnt = len;
	memcpy(instrs,space,len);
    } else {
	icnt = PySequence_Size(tuple);
	instrs = malloc(icnt);
	for ( i=0; i<icnt; ++i ) {
	    PyObject *val = PySequence_GetItem(tuple, i);
	    instrs[i] = PyLong_AsLong(val);
	    Py_XDECREF(val);
	    if ( PyErr_Occurred()) {
		free(instrs);
return( NULL );
	    }
	}
    }
    TableAddInstrs(self->fv->sf,tag,true,instrs,icnt);
    free(instrs);
Py_RETURN(self);
}

static PyObject *PyFFFont_regenBitmaps(PyFF_Font *self,PyObject *args) {
    if ( CheckIfFontClosed(self) )
return (NULL);
    if ( bitmapper(self,args,false)==-1 )
return( NULL );

Py_RETURN(self);
}

static PyObject *PyFFFont_importBitmaps(PyFF_Font *self,PyObject *args) {
    char *filename;
    char *locfilename = NULL, *ext;
    int to_background = -1, back=false;
    FontViewBase *fv;
    int ok, format;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    if ( !PyArg_ParseTuple(args,"s|i",&filename,
	    &to_background) )
return( NULL );
    locfilename = utf82def_copy(filename);

    ext = strrchr(locfilename,'.');
    if ( ext==NULL ) {
	int len = strlen(locfilename);
	ext = locfilename+len-2;
	if ( ext[0]!='p' || ext[1]!='k' ) {
	    PyErr_Format(PyExc_EnvironmentError, "No extension for bitmap font");
return( NULL );
	}
    }
    if ( strmatch(ext,".bdf")==0 || strmatch(ext-4,".bdf.gz")==0 )
	format = fv_bdf;
    else if ( strmatch(ext,".pcf")==0 || strmatch(ext-4,".pcf.gz")==0 )
	format = fv_pcf;
    else if ( strmatch(ext,".ttf")==0 || strmatch(ext,".otf")==0 || strmatch(ext,".otb")==0 )
	format = fv_ttf;
    else if ( strmatch(ext,"pk")==0 || strmatch(ext,".pk")==0 ) {
	format = fv_pk;
	back = true;
    } else {
	PyErr_Format(PyExc_EnvironmentError, "Bad extension for bitmap font");
return( NULL );
    }
    if ( to_background!=-1 )
	back = to_background;
    if ( format==fv_bdf )
	ok = FVImportBDF(fv,locfilename,false, back);
    else if ( format==fv_pcf )
	ok = FVImportBDF(fv,locfilename,2, back);
    else if ( format==fv_ttf )
	ok = FVImportMult(fv,locfilename, back, bf_ttf);
    else if ( format==fv_pk )
	ok = FVImportBDF(fv,locfilename,true, back);
    free(locfilename);
    if ( !ok ) {
	PyErr_Format(PyExc_EnvironmentError, "Could not load bitmap font");
return( NULL );
    }

Py_RETURN(self);
}

/* Font comparison flags */
static struct flaglist compflags[] = {
    { "outlines",		  1 },
    { "outlines-exactly",	  2 },
    { "warn-outlines-mismatch",	  4 },
    { "hints",			  8 },
    { "warn-refs-unlink",	  0x40 },
    { "strikes",		  0x80 },
    { "fontnames",		  0x100 },
    { "gpos",			  0x200 },
    { "gsub",			  0x400 },
    { "add-outlines",		  0x800 },
    { "create-glyphs",		  0x1000 },
    FLAGLIST_EMPTY /* Sentinel */
};

static PyObject *PyFFFont_compareFonts(PyFF_Font *self,PyObject *args) {
    /* Compare the current font against the named one	     */
    /* output to a file (used /dev/null if no output wanted) */
    /* flags control what tests are done		     */
    PyFF_Font *other;
    PyObject *flagstuple, *ret;
    FILE *diffs;
    int flags;
    char *filename, *locfilename;

    if ( CheckIfFontClosed(self) )
return (NULL);
    if ( !PyArg_ParseTuple(args,"OsO", &other, &filename, &flagstuple ))
return( NULL );
    locfilename = utf82def_copy(filename);

    if ( !PyType_IsSubtype(&PyFF_FontType, Py_TYPE(other)) ) {
	PyErr_Format(PyExc_TypeError,"First argument must be a fontforge font");
	free(locfilename);
return( NULL );
    }
    if ( CheckIfFontClosed(other) )
return (NULL);
    flags = FlagsFromTuple(flagstuple,compflags,"comparison flag");
    if ( flags==FLAG_UNKNOWN ) {
	free(locfilename);
return( NULL );
    }

    if ( strcmp(locfilename,"-")==0 )
	diffs = stdout;
    else {
	diffs = fopen(locfilename,"w");
	if ( diffs==NULL ) {
	    PyErr_SetFromErrnoWithFilename(PyExc_IOError,locfilename);
	    free(locfilename);
return( NULL );
	}
    }

    free( locfilename );

    ret = Py_BuildValue("i", CompareFonts(self->fv->sf, self->fv->map, other->fv->sf, diffs, flags ));
    if ( diffs!=stdout )
	fclose( diffs );
return( ret );
}

static PyObject *PyFFFont_appendSFNTName(PyFF_Font *self, PyObject *args) {
    SplineFont *sf;
    struct ttflangname dummy;
    int i;

    if ( CheckIfFontClosed(self) )
return (NULL);
    sf = self->fv->sf;
    memset(&dummy,0,sizeof(dummy));
    DefaultTTFEnglishNames(&dummy, sf);

    if ( !SetSFNTName(sf,args,&dummy) )
return( NULL );

    for ( i=0; i<ttf_namemax; ++i )
	free( dummy.names[i]);

Py_RETURN( self );
}

static PyObject *PyFFFont_cidConvertTo(PyFF_Font *self,PyObject *args) {
    FontViewBase *fv;
    SplineFont *sf;
    struct cidmap *map;
    char *registry, *ordering;
    int supplement;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    sf = fv->sf;
    if ( sf->cidmaster!=NULL ) {
	PyErr_Format(PyExc_EnvironmentError,"This font is already a CID keyed font." );
return( NULL );
    }
    if ( !PyArg_ParseTuple(args,"ssi", &registry, &ordering, &supplement ))
return( NULL );
    map = FindCidMap( registry, ordering, supplement, sf);
    if ( map == NULL ) {
	PyErr_Format(PyExc_EnvironmentError,"No cidmap matching given ROS (%s-%s-%d)",
		registry, ordering, supplement );
return( NULL );
    }
    MakeCIDMaster(sf, fv->map, false, NULL, map);
Py_RETURN( self );
}

static PyObject *PyFFFont_cidConvertByCmap(PyFF_Font *self,PyObject *args) {
    FontViewBase *fv;
    SplineFont *sf;
    char *locfilename;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    sf = fv->sf;
    if ( sf->cidmaster!=NULL ) {
	PyErr_Format(PyExc_EnvironmentError,"This font is already a CID keyed font." );
return( NULL );
    }
    if ( !PyArg_ParseTuple(args,"s", &locfilename ))
return( NULL );
    MakeCIDMaster(sf, fv->map, true, locfilename, NULL);
Py_RETURN( self );
}

static PyObject *PyFFFont_cidFlatten(PyFF_Font *self, PyObject *UNUSED(args)) {
    SplineFont *cidmaster;

    if ( CheckIfFontClosed(self) )
return (NULL);
    cidmaster = self->fv->cidmaster;
    if ( cidmaster==NULL ) {
	PyErr_Format(PyExc_EnvironmentError,"This font is not a CID keyed font." );
return( NULL );
    }

    SFFlatten(&cidmaster);
Py_RETURN( self );
}

static PyObject *PyFFFont_cidFlattenByCMap(PyFF_Font *self,PyObject *args) {
    SplineFont *cidmaster;
    char *locfilename;

    if ( CheckIfFontClosed(self) )
return (NULL);
    cidmaster = self->fv->cidmaster;
    if ( cidmaster==NULL ) {
	PyErr_Format(PyExc_EnvironmentError,"This font is not a CID keyed font." );
return( NULL );
    }

    if ( !PyArg_ParseTuple(args,"s", &locfilename ))
return( NULL );

    if ( !SFFlattenByCMap(&cidmaster,locfilename)) {
	PyErr_Format(PyExc_EnvironmentError,"Can't find (or can't parse) cmap file: %s", locfilename);
return( NULL );
    }
Py_RETURN( self );
}

static PyObject *PyFFFont_cidInsertBlankSubFont(PyFF_Font *self, PyObject *UNUSED(args)) {
    FontViewBase *fv;
    SplineFont *cidmaster, *sf;
    struct cidmap *map;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    cidmaster = fv->cidmaster;
    if ( cidmaster==NULL ) {
	PyErr_Format(PyExc_EnvironmentError,"This font is not a CID keyed font." );
return( NULL );
    }
    if ( cidmaster->subfontcnt>=255 ) {
	PyErr_Format(PyExc_EnvironmentError,"You may have at most 255 subfonts in a CID keyed font." );
return( NULL );
    }

    map = FindCidMap(cidmaster->cidregistry,cidmaster->ordering,cidmaster->supplement,cidmaster);
    sf = SplineFontBlank(MaxCID(map));
    sf->glyphcnt = sf->glyphmax;
    sf->cidmaster = cidmaster;
    sf->display_antialias = fv->sf->display_antialias;
    sf->display_bbsized = fv->sf->display_bbsized;
    sf->display_size = fv->sf->display_size;
    sf->private = calloc(1,sizeof(struct psdict));
    PSDictChangeEntry(sf->private,"lenIV","1");		/* It's 4 by default, in CIDs the convention seems to be 1 */
    FVInsertInCID(fv,sf);
Py_RETURN( self );
}

static PyObject *PyFFFont_cidRemoveSubFont(PyFF_Font *self, PyObject *UNUSED(args)) {
    FontViewBase *fv, *fvs;
    SplineFont *cidmaster, *sf, *replace;
    int i;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    sf = fv->sf;
    cidmaster = fv->cidmaster;
    if ( cidmaster==NULL ) {
	PyErr_Format(PyExc_EnvironmentError,"This font is not a CID keyed font." );
return( NULL );
    }
    if ( cidmaster->subfontcnt<=1 ) {
	PyErr_Format(PyExc_EnvironmentError,"You must have at least 1 subfont in a CID keyed font." );
return( NULL );
    }

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	SCCloseAllViews(sf->glyphs[i]);
    }
    MVDestroyAll(sf);

    for ( i=0; i<cidmaster->subfontcnt; ++i )
	if ( cidmaster->subfonts[i]==sf )
    break;
    replace = i==0?cidmaster->subfonts[1]:cidmaster->subfonts[i-1];
    while ( i<cidmaster->subfontcnt-1 ) {
	cidmaster->subfonts[i] = cidmaster->subfonts[i+1];
	++i;
    }
    --cidmaster->subfontcnt;

    for ( fvs=sf->fv; fvs!=NULL; fvs=fvs->nextsame ) {
	if ( fvs->sf==sf )
	    CIDSetEncMap( fvs,replace);
    }
    FontViewReformatAll(sf);
    SplineFontFree(sf);
Py_RETURN( self );
}

static struct lookup_subtable *addLookupSubtable(SplineFont *sf, char *lookup,
						 char *new_subtable, char *after_str) {
    OTLookup *otl;
    struct lookup_subtable *sub, *after=NULL;
    int is_v;

    otl = SFFindLookup(sf,lookup);
    if ( otl==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No lookup named %s", lookup );
return( NULL );
    }
    if ( after_str!=NULL ) {
	after = SFFindLookupSubtable(sf,after_str);
	if ( after==NULL ) {
	    PyErr_Format(PyExc_EnvironmentError, "No lookup subtable named %s", after_str );
return( NULL );
	} else if ( after->lookup!=otl ) {
	    PyErr_Format(PyExc_EnvironmentError, "Subtable, %s, is not in lookup %s.", after_str, lookup );
return( NULL );
	}
    }

    if ( sf->cidmaster ) sf = sf->cidmaster;

    if ( SFFindLookupSubtable(sf,new_subtable)!=NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "A lookup subtable named %s already exists", new_subtable);
return( NULL );
    }

    sub = chunkalloc(sizeof(struct lookup_subtable));
    sub->lookup = otl;
    sub->subtable_name = copy(new_subtable);
    if ( after!=NULL ) {
	sub->next = after->next;
	after->next = sub;
    } else {
	sub->next = otl->subtables;
	otl->subtables = sub;
    }

    switch ( otl->lookup_type ) {
      case gpos_cursive:
      case gpos_mark2base:
      case gpos_mark2ligature:
      case gpos_mark2mark:
        sub->anchor_classes = true;
      break;
      case gpos_pair:
        is_v = VerticalKernFeature(sf, otl, false);
        if ( is_v==-1 ) is_v = false;
        sub->vertical_kerning = is_v;
        sub->per_glyph_pst_or_kern = true;
      break;
      case gpos_single:
      case gsub_single:
      case gsub_multiple:
      case gsub_alternate:
      case gsub_ligature:
        sub->per_glyph_pst_or_kern = true;
      break;
      default:
      break;
    }

return( sub );
}

static PyObject *PyFFFont_buildOrReplaceAALTFeatures(PyFF_Font *self, PyObject *UNUSED(args)) {
    SplineFont *sf;

    if ( CheckIfFontClosed(self) )
return (NULL);
    sf = self->fv->sf;
    AddNewAALTFeatures(sf);

Py_RETURN( self );
}

static PyObject *PyFFFont_addAnchorClass(PyFF_Font *self, PyObject *args) {
    SplineFont *sf;
    char *subtable, *anchor_name, *typename;
    struct lookup_subtable *sub;
    AnchorClass *ac;
    int ac_type, aptype;

    if ( CheckIfFontClosed(self) )
return (NULL);
    sf = self->fv->sf;
    if ( !PyArg_ParseTuple(args,"ss|s", &subtable, &anchor_name, &typename ))
return( NULL );

    sub = SFFindLookupSubtable(sf,subtable);
    if ( sub==NULL && !typename ) {
	PyErr_Format(PyExc_EnvironmentError, "No subtable named %s", subtable );
return( NULL );
    }
    else if ( sub==NULL) {
        aptype = FlagsFromString(typename,ap_types,"anchor type");
        ac_type = aptype==at_basechar                           ? act_mark :
                        aptype==at_baselig                      ? act_mklg :
                        aptype==at_cexit || aptype==at_centry   ? act_curs :
                                                                  act_mkmk ;
    }
    else
        ac_type = sub->lookup->lookup_type==gpos_cursive                ? act_curs :
                        sub->lookup->lookup_type==gpos_mark2base        ? act_mark :
                        sub->lookup->lookup_type==gpos_mark2ligature    ? act_mklg :
                        sub->lookup->lookup_type==gpos_mark2mark        ? act_mkmk :
                                                                          act_unknown;
    if ( ac_type == act_unknown ) {
	PyErr_Format(PyExc_EnvironmentError, "Cannot add an anchor class to %s, it has the wrong lookup type", subtable );
return( NULL );
    }
    for ( ac=sf->anchor; ac!=NULL; ac=ac->next ) {
	if ( strcmp(ac->name,anchor_name)==0 )
    break;
    }
    if ( ac!=NULL && ac->subtable!=NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "An anchor class named %s already exists", anchor_name );
return( NULL );
    }
    else if ( ac!=NULL ) {  /* we are associating an existing implicit anchor class */
        ac->subtable = sub;
        ac->type = ac_type;
    }
    else {
        ac = chunkalloc(sizeof(AnchorClass));
        ac->name = copy( anchor_name );
        ac->subtable = sub;
        ac->type = ac_type;
        ac->next = sf->anchor;
        sf->anchor = ac;
    }

Py_RETURN( self );
}

static PyObject *PyFFFont_removeAnchorClass(PyFF_Font *self, PyObject *args) {
    SplineFont *sf;
    char *anchor_name;
    AnchorClass *ac;

    if ( CheckIfFontClosed(self) )
return (NULL);
    sf = self->fv->sf;
    if ( !PyArg_ParseTuple(args,"s", &anchor_name ))
return( NULL );

    for ( ac=sf->anchor; ac!=NULL; ac=ac->next ) {
	if ( strcmp(ac->name,anchor_name)==0 )
    break;
    }
    if ( ac==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No anchor class named %s exists", anchor_name );
return( NULL );
    }
    SFRemoveAnchorClass(sf,ac);
Py_RETURN( self );
}

static int ParseClassNames(PyObject *classes,char ***class_strs) {
    int cnt, i;
    char **cls;

    *class_strs = NULL;
    cnt = PySequence_Size(classes);
    if ( cnt==-1 )
return( -1 );
    *class_strs = cls = malloc(cnt*sizeof(char *));
    for ( i=0; i<cnt; ++i ) {
	PyObject *thingy = PySequence_GetItem(classes,i);
	if ( i==0 && thingy==Py_None ) {
	    cls[i] = NULL;
	    Py_DECREF(thingy);
	} else {
	    cls[i] = GlyphNamesFromTuple(thingy);
	    Py_XDECREF(thingy);
	    if ( cls[i]==NULL )
return( -1 );
	}
    }
return( cnt );
}

static PyObject *MakeClassNameTuple(int cnt, char**classes) {
    PyObject *tuple;
    int i;

    tuple = PyTuple_New(cnt);
    for ( i=0; i<cnt; ++i ) {
	if ( classes[i]==NULL ) {
	    PyTuple_SetItem(tuple,i,Py_None);
	    Py_INCREF(Py_None);
	} else
	    PyTuple_SetItem(tuple,i,TupleOfGlyphNames(classes[i],0));
    }
return( tuple );
}

static SplineChar **GlyphsFromSelection(FontViewBase *fv) {
    SplineFont *sf;
    EncMap *map;
    int selcnt;
    int enc,gid;
    SplineChar **glyphlist, *sc;

    map = fv->map;
    sf = fv->sf;
    selcnt=0;
    for ( enc=0; enc<map->enccount; ++enc ) {
	if ( fv->selected[enc] && (gid=map->map[enc])!=-1 &&
		SCWorthOutputting(sf->glyphs[gid]))
	    ++selcnt;
    }
    if ( selcnt<=1 ) {
	PyErr_Format(PyExc_EnvironmentError, "Please select some glyphs in the font view for FontForge to put into classes.");
return(NULL);
    }

    glyphlist = malloc((selcnt+1)*sizeof(SplineChar *));
    selcnt=0;
    for ( enc=0; enc<map->enccount; ++enc ) {
	if ( fv->selected[enc] && (gid=map->map[enc])!=-1 &&
		SCWorthOutputting(sc = sf->glyphs[gid]))
	    glyphlist[selcnt++] = sc;
    }
    glyphlist[selcnt] = NULL;
return( glyphlist );
}

static int pyBuildClasses(FontViewBase *fv,
	struct lookup_subtable *sub,real good_enough,
	PyObject *list1, PyObject *list2) {
    SplineChar **glyphlist, **first, **second;

    if ( list1==NULL ) {
	glyphlist = GlyphsFromSelection(fv);
	if ( glyphlist==NULL )
return( false );
	first = second = glyphlist;
    } else {
	first = GlyphsFromTuple(fv->sf,list1);
	second = GlyphsFromTuple(fv->sf,list2);
    }
    if ( first==NULL || second==NULL ) {
	free(second); free(first);
return( false );
    }
    AutoKern2BuildClasses(fv->sf,fv->active_layer,first,second,sub,
	    sub->separation,0,sub->kerning_by_touch, sub->onlyCloser,
	    !sub->dontautokern,
	    good_enough);
    free(first);
    if ( first!=second )
	free(second);
return( true );
}

static void pyAddOffsetAsIs(void *data,int left_index,int right_index, int kern) {
    struct lookup_subtable *sub = data;
    KernClass *kc = sub->kc;

    if ( !(sub->lookup->lookup_flags & pst_r2l) ) {
	kc->offsets[left_index*kc->second_cnt+right_index] = kern;
    } else {
	kc->offsets[right_index*kc->second_cnt+left_index] = kern;
    }
}

static void pyAutoKernAll(FontViewBase *fv,struct lookup_subtable *sub ) {
    char **lefts, **rights;
    int lcnt, rcnt;
    KernClass *kc = sub->kc;

    if ( !(sub->lookup->lookup_flags & pst_r2l) ) {
	lefts = kc->firsts; lcnt = kc->first_cnt;
	rights = kc->seconds; rcnt = kc->second_cnt;
    } else {
	lefts = kc->seconds; lcnt = kc->second_cnt;
	rights = kc->firsts; rcnt=kc->first_cnt;
    }
    AutoKern2NewClass(fv->sf,fv->active_layer, lefts, rights, lcnt, rcnt,
	    pyAddOffsetAsIs, sub, sub->separation, 0, sub->kerning_by_touch,
	    sub->onlyCloser, 0);
}

static PyObject *PyFFFont_addKerningClass(PyFF_Font *self, PyObject *args) {
    FontViewBase *fv;
    SplineFont *sf;

    if ( CheckIfFontClosed(self) )
return (NULL);

    fv = self->fv;
    sf = fv->sf;
    char *lookup, *subtable, *after_str=NULL;
    int i;
    struct lookup_subtable *sub;
    PyObject *class1s=NULL, *class2s=NULL, *offsets=NULL, *list1=NULL, *list2=NULL;
    PyObject *arg3, *arg4, *arg5;
    char **class1_strs, **class2_strs;
    int cnt1, cnt2, acnt;
    int16_t *offs=NULL;
    int separation= -1, touch=0, do_autokern=false, only_closer=0, autokern=true;
    double class_error_distance = -1;
    /* arguments:
     *  (char *lookupname, char *newsubtabname, char ***classes1, char ***classes2, int *offsets [,char *after_sub_name])
     *  (char *lookupname, char *newsubtabname, int separation, char ***classes1, char ***classes2 [, int only_closer, int autokern, char *after_sub_name])
     *  (char *lookupname, char *newsubtabname, int separation, double err, char **list1, char **list2 [, int only_closer, int autokern, char *after_sub_name])
     *  (char *lookupname, char *newsubtabname, int separation, double err [, int only_closer, int autokern, char *after_sub_name])
     *  Also support arguments where [,int autokern] is absent as we used not to
     *  allow the user to specify it
     * First is fully specified set of classes with offsets cnt=5/6
     * Second fully specified set of classes, to be autokerned cnt=5/7
     * Third two lists of glyphs to be turned into classes and then autokerned cnt=6/8
     * Fourth turns the selection into a list of glyphs, to be used both left and right for two sets of classes to be autokerned cnt=4/6
     */
    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    if ( (acnt = PyTuple_Size(args))<4 ) {
	PyErr_Format(PyExc_EnvironmentError, "Too few arguments.");
return( NULL );
    }
    arg3 = PyTuple_GetItem(args,2);
    arg4 = PyTuple_GetItem(args,3);
    do_autokern = true;
    if ( !PyLong_Check(arg3)) {
	if ( !PyArg_ParseTuple(args,"ssOOO|s", &lookup, &subtable, &class1s, &class2s,
		&offsets, &after_str ))
return( NULL );
	do_autokern = false;
    } else if ( !PyLong_Check(arg4) && !PyFloat_Check(arg4)) {
	PyObject *arg7 = acnt>=7 ? PyTuple_GetItem(args,6) : NULL;
	if ( arg7!=NULL && (PyLong_Check(arg7))) {
	    if ( !PyArg_ParseTuple(args,"ssiOO|iis", &lookup, &subtable,
		    &separation, &class1s, &class2s,
		    &only_closer, &autokern, &after_str ))
return( NULL );
	} else {
	    if ( !PyArg_ParseTuple(args,"ssiOO|is", &lookup, &subtable,
		    &separation, &class1s, &class2s,
		    &only_closer, &after_str ))
return( NULL );
	}
    } else if ( acnt>5 &&
	    (arg5=PyTuple_GetItem(args,4)) && PyTuple_Check(arg5) ) {
	PyObject *arg8 = acnt>=8 ? PyTuple_GetItem(args,7) : NULL;
	if ( arg8!=NULL && (PyLong_Check(arg8))) {
	    if ( !PyArg_ParseTuple(args,"ssidOO|iis", &lookup, &subtable,
		    &separation, &class_error_distance, &list1, &list2,
		    &only_closer, &autokern, &after_str ))
return( NULL );
	} else {
	    if ( !PyArg_ParseTuple(args,"ssidOO|is", &lookup, &subtable,
		    &separation, &class_error_distance, &list1, &list2,
		    &only_closer, &after_str ))
return( NULL );
	}
    } else {
	PyObject *arg6 = acnt>=6 ? PyTuple_GetItem(args,5) : NULL;
	if ( arg6!=NULL && (PyLong_Check(arg6))) {
	    if ( !PyArg_ParseTuple(args,"ssid|iis", &lookup, &subtable,
		    &separation, &class_error_distance,
		    &only_closer, &autokern, &after_str ))
return( NULL );
	} else {
	    if ( !PyArg_ParseTuple(args,"ssid|is", &lookup, &subtable,
		    &separation, &class_error_distance,
		    &only_closer, &after_str ))
return( NULL );
	}
    }
    if ( separation==0 )
	touch=1;

    if ( class1s!=NULL ) {
	cnt1 = ParseClassNames(class1s,&class1_strs);
	cnt2 = ParseClassNames(class2s,&class2_strs);
	if ( offsets!=NULL ) {
	    if ( cnt1*cnt2 != PySequence_Size(offsets) ) {
		PyErr_Format(PyExc_ValueError, "There aren't enough kerning offsets for the number of kerning classes. Should be %d", cnt1*cnt2 );
return( NULL );
	    }
	    offs = malloc(cnt1*cnt2*sizeof(int16_t));
	    for ( i=0 ; i<cnt1*cnt2; ++i ) {
		offs[i] = PyLong_AsLong(PySequence_GetItem(offsets,i));
		if ( PyErr_Occurred()) {
                    free(offs);
return( NULL );
}
	    }
	} else
	    offs = calloc(cnt1*cnt2,sizeof(int16_t));
    }

    sub = addLookupSubtable(sf, lookup, subtable, after_str);
    if ( sub==NULL ) {
        free(offs);
return( NULL );
    }
    if ( sub->lookup->lookup_type!=gpos_pair ) {
	PyErr_Format(PyExc_EnvironmentError, "Cannot add kerning data to %s, it has the wrong lookup type", lookup );
	free(offs);
return( NULL );
    }
    sub->per_glyph_pst_or_kern = false;
    if ( do_autokern ) {
	sub->separation = separation;
	sub->kerning_by_touch = touch;
	sub->onlyCloser = only_closer;
	sub->dontautokern = !autokern;
    }
    sub->kc = chunkalloc(sizeof(KernClass));
    sub->kc->subtable = sub;
    if ( class1s!=NULL ) {
	sub->kc->first_cnt = cnt1;
	sub->kc->second_cnt = cnt2;
	sub->kc->firsts = class1_strs;
	sub->kc->seconds = class2_strs;
	sub->kc->offsets = offs;
	sub->kc->adjusts = calloc(cnt1*cnt2,sizeof(DeviceTable));
	if ( offsets==NULL )
	    pyAutoKernAll(fv,sub);
    } else {
	if ( !pyBuildClasses(fv,sub,class_error_distance,list1,list2) ) {
            free(offs);
return( NULL );
        }
    }

    if ( sub->vertical_kerning ) {
	sub->kc->next = sf->vkerns;
	sf->vkerns = sub->kc;
    } else {
	sub->kc->next = sf->kerns;
	sf->kerns = sub->kc;
    }

Py_RETURN( self );
}

static PyObject *PyFFFont_alterKerningClass(PyFF_Font *self, PyObject *args) {
    SplineFont *sf;
    char *subtable;
    int i;
    struct lookup_subtable *sub;
    PyObject *class1s, *class2s, *offsets;
    char **class1_strs, **class2_strs;
    int cnt1, cnt2;
    int16_t *offs;

    if ( CheckIfFontClosed(self) )
return (NULL);
    sf = self->fv->sf;
    if ( !PyArg_ParseTuple(args,"sOOO", &subtable, &class1s, &class2s,
	    &offsets ))
return( NULL );

    sub = SFFindLookupSubtable(sf,subtable);
    if ( sub==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No subtable named %s", subtable );
return( NULL );
    }
    if ( sub->kc==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "This subtable, %s, does not contain not a kerning class", subtable );
return( NULL );
    }

    cnt1 = ParseClassNames(class1s,&class1_strs);
    cnt2 = ParseClassNames(class2s,&class2_strs);
    if ( cnt1*cnt2 != PySequence_Size(offsets) ) {
	PyErr_Format(PyExc_ValueError, "There aren't enough kerning offsets for the number of kerning classes. Should be %d", cnt1*cnt2 );
return( NULL );
    }
    offs = malloc(cnt1*cnt2*sizeof(int16_t));
    for ( i=0 ; i<cnt1*cnt2; ++i ) {
	offs[i] = PyLong_AsLong(PySequence_GetItem(offsets,i));
	if ( PyErr_Occurred()) {
	    free(offs); free(class2_strs); free(class1_strs);
return( NULL );
	}
    }

    KernClassFreeContents(sub->kc);
    sub->kc->first_cnt = cnt1;
    sub->kc->second_cnt = cnt2;
    sub->kc->firsts = class1_strs;
    sub->kc->seconds = class2_strs;
    sub->kc->offsets = offs;
    sub->kc->adjusts = calloc(cnt1*cnt2,sizeof(DeviceTable));

Py_RETURN( self );
}

static PyObject *PyFFFont_getKerningClass(PyFF_Font *self, PyObject *args) {
    SplineFont *sf;
    char *subtable;
    struct lookup_subtable *sub;
    PyObject *offsets;
    int i;

    if ( CheckIfFontClosed(self) )
return (NULL);
    sf = self->fv->sf;
    if ( !PyArg_ParseTuple(args,"s", &subtable ))
return( NULL );

    sub = SFFindLookupSubtable(sf,subtable);
    if ( sub==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No subtable named %s", subtable );
return( NULL );
    }
    if ( sub->kc==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "This subtable, %s, does not contain not a kerning class", subtable );
return( NULL );
    }
    offsets = PyTuple_New(sub->kc->first_cnt*sub->kc->second_cnt);
    for ( i=0; i<sub->kc->first_cnt*sub->kc->second_cnt; ++i )
	PyTuple_SetItem(offsets,i,PyLong_FromLong(sub->kc->offsets[i]));

return( Py_BuildValue("(OOO)",
	MakeClassNameTuple(sub->kc->first_cnt,sub->kc->firsts),
	MakeClassNameTuple(sub->kc->second_cnt,sub->kc->seconds),
	offsets));
}

static PyObject *PyFFFont_isKerningClass(PyFF_Font *self, PyObject *args) {
    SplineFont *sf;
    char *subtable;
    struct lookup_subtable *sub;
    PyObject *ret;

    if ( CheckIfFontClosed(self) )
return (NULL);
    sf = self->fv->sf;
    if ( !PyArg_ParseTuple(args,"s", &subtable ))
return( NULL );

    sub = SFFindLookupSubtable(sf,subtable);
    if ( sub==NULL || sub->kc==NULL )
	ret = Py_False;
    else
	ret = Py_True;
    Py_INCREF(ret);
return( ret );
}

static PyObject *PyFFFont_isVerticalKerning(PyFF_Font *self, PyObject *args) {
    SplineFont *sf;
    char *subtable;
    struct lookup_subtable *sub;
    PyObject *ret;

    if ( CheckIfFontClosed(self) )
return (NULL);
    sf = self->fv->sf;
    if ( !PyArg_ParseTuple(args,"s", &subtable ))
return( NULL );

    sub = SFFindLookupSubtable(sf,subtable);
    if ( sub==NULL || !sub->vertical_kerning )
	ret = Py_False;
    else
	ret = Py_True;
    Py_INCREF(ret);
return( ret );
}

static const char *ak_keywords1[] = { "subTableName", "separation", "minKern",
	"touch", "onlyCloser", "height", NULL };
static const char *ak_keywords2[] = { "subTableName", "separation", "list1", "list2",
	"minKern", "touch", "onlyCloser", "height", NULL };

static PyObject *PyFFFont_autoKern(PyFF_Font *self, PyObject *args, PyObject *keywds) {
    FontViewBase *fv;
    SplineFont *sf;
    char *subtablename;
    int separation;
    PyObject *list1=NULL, *list2=NULL;
    SplineChar **first, **second, **left, **right;
    struct lookup_subtable *sub;
    int minkern = 10, touch=0, height=0, onlyCloser=0;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    sf = fv->sf;
    if ( PyTuple_Size(args)==2 ) {
	if ( !PyArg_ParseTupleAndKeywords(args, keywds, "si|iiii", (char **)ak_keywords1,
		&subtablename, &separation, &minkern, &touch, &onlyCloser, &height))
return( NULL );
    } else {
	if ( !PyArg_ParseTupleAndKeywords(args, keywds, "siOO|iiii", (char **)ak_keywords2,
		&subtablename, &separation, &list1, &list2, &minkern, &touch,
		&onlyCloser, &height ))
return( NULL );
    }
    sub = SFFindLookupSubtable(sf,subtablename);
    if ( sub==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No subtable named %s exists", subtablename );
return( NULL );
    }
    if ( sub->lookup->lookup_type!=gpos_pair || sub->kc!=NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "%s is not a kerning pair subtable", subtablename );
return( NULL );
    }

    if ( list1!=NULL ) {
	first = GlyphsFromTuple(sf,list1);
	second = GlyphsFromTuple(sf,list2);
    } else {
	first = second = GlyphsFromSelection(fv);
    }
    if ( first==NULL || second==NULL ) {
	free(second); free(first);
return( NULL );
    }
    if ( sub->lookup->lookup_flags & pst_r2l ) {
	left = second;
	right = first;
    } else {
	left = first;
	right = second;
    }
    AutoKern2(sf, fv->active_layer,left,right, sub,
	separation, minkern, touch, onlyCloser, height,
	NULL,NULL);
    free(first);
    if ( first!=second )
	free(second);
Py_RETURN( self );
}

static PyObject *PyFFFont_removeLookup(PyFF_Font *self, PyObject *args) {
    SplineFont *sf;
    char *lookup;
    OTLookup *otl;
    int remove_acs = 0;

    if ( CheckIfFontClosed(self) )
return (NULL);
    sf = self->fv->sf;
    if ( !PyArg_ParseTuple(args,"s|i", &lookup, &remove_acs ))
return( NULL );

    otl = SFFindLookup(sf,lookup);
    if ( otl==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No lookup named %s exists", lookup );
return( NULL );
    }
    SFRemoveLookup(sf,otl,remove_acs);
Py_RETURN( self );
}

static PyObject *PyFFFont_mergeLookups(PyFF_Font *self, PyObject *args) {
    SplineFont *sf;
    char *lookup1, *lookup2;
    OTLookup *otl1, *otl2;
    struct lookup_subtable *sub;

    if ( CheckIfFontClosed(self) )
return (NULL);
    sf = self->fv->sf;
    if ( !PyArg_ParseTuple(args,"ss", &lookup1, &lookup2 ))
return( NULL );

    otl1 = SFFindLookup(sf,lookup1);
    if ( otl1==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No lookup named %s exists", lookup1 );
return( NULL );
    }
    otl2 = SFFindLookup(sf,lookup2);
    if ( otl2==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No lookup named %s exists", lookup2 );
return( NULL );
    }
    if ( otl1->lookup_type != otl2->lookup_type ) {
	PyErr_Format(PyExc_EnvironmentError, "When merging two lookups they must be of the same type, but %s and %s are not", lookup1, lookup2);
return( NULL );
    }
    FLMerge(otl1,otl2);

    for ( sub = otl2->subtables; sub!=NULL; sub=sub->next )
	sub->lookup = otl1;
    if ( otl1->subtables==NULL )
	otl1->subtables = otl2->subtables;
    else {
	for ( sub=otl1->subtables; sub->next!=NULL; sub=sub->next );
	sub->next = otl2->subtables;
    }
    otl2->subtables = NULL;
    SFRemoveLookup(sf,otl2,0);
Py_RETURN( self );
}

static PyObject *PyFFFont_removeLookupSubtable(PyFF_Font *self, PyObject *args) {
    SplineFont *sf;
    char *subtable;
    struct lookup_subtable *sub;
    int remove_acs = 0;

    if ( CheckIfFontClosed(self) )
return (NULL);
    sf = self->fv->sf;
    if ( !PyArg_ParseTuple(args,"s|i", &subtable, &remove_acs ))
return( NULL );

    sub = SFFindLookupSubtable(sf,subtable);
    if ( sub==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No subtable named %s exists", subtable );
return( NULL );
    }
    SFRemoveLookupSubTable(sf,sub,remove_acs);
Py_RETURN( self );
}

static PyObject *PyFFFont_mergeLookupSubtables(PyFF_Font *self, PyObject *args) {
    SplineFont *sf;
    char *subtable1, *subtable2;
    struct lookup_subtable *sub1, *sub2;

    if ( CheckIfFontClosed(self) )
return (NULL);
    sf = self->fv->sf;
    if ( !PyArg_ParseTuple(args,"ss", &subtable1, &subtable2 ))
return( NULL );

    sub1 = SFFindLookupSubtable(sf,subtable1);
    if ( sub1==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No subtable named %s exists", subtable1 );
return( NULL );
    }
    sub2 = SFFindLookupSubtable(sf,subtable2);
    if ( sub2==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No subtable named %s exists", subtable2 );
return( NULL );
    }
    if ( sub1->lookup!=sub2->lookup ) {
	PyErr_Format(PyExc_EnvironmentError, "When merging two lookup subtables they must be in the same lookup, but %s and %s are not", subtable1, subtable2);
return( NULL );
    }
    SFSubTablesMerge(sf,sub1,sub2);
    SFRemoveLookupSubTable(sf,sub2,0);
Py_RETURN( self );
}

/* OpenType lookup types: see 'enum otlookup_type' in splinefont.h */
static struct flaglist lookup_types[] = {
    { "gsub_single", gsub_single },
    { "gsub_multiple", gsub_multiple },
    { "gsub_alternate", gsub_alternate },
    { "gsub_ligature", gsub_ligature },
    { "gsub_context", gsub_context },
    { "gsub_contextchain", gsub_contextchain },
    { "gsub_reversecchain", gsub_reversecchain },
    { "morx_indic", morx_indic },
    { "morx_context", morx_context },
    { "morx_insert", morx_insert },
    { "gpos_single", gpos_single },
    { "gpos_pair", gpos_pair },
    { "gpos_cursive", gpos_cursive },
    { "gpos_mark2base", gpos_mark2base },
    { "gpos_mark2ligature", gpos_mark2ligature },
    { "gpos_mark2mark", gpos_mark2mark },
    { "gpos_context", gpos_context },
    { "gpos_contextchain", gpos_contextchain },
    { "kern_statemachine", kern_statemachine },
    FLAGLIST_EMPTY /* Sentinel */
};

/* OpenType lookup flags: see 'enum pst_flags' in splinefont.h */
static struct flaglist lookup_flags[] = {
    { "right_to_left", pst_r2l },
    { "ignore_bases", pst_ignorebaseglyphs },
    { "ignore_ligatures", pst_ignoreligatures },
    { "ignore_marks", pst_ignorecombiningmarks },
    /*{ "", pst_usemarkfilteringset },*/
    /*{ "", pst_markclass },*/
    /*{ "", pst_markset },*/
    { "right_2_left", pst_r2l },
    { "right2left", pst_r2l },
    FLAGLIST_EMPTY /* Sentinel */
};

static int ParseLookupFlagsItem(SplineFont *sf,PyObject *flagstr) {
    const char *str;
    int i;

    if ((str = PyUnicode_AsUTF8(flagstr)) == NULL) {
	return( -1 );
    }
    for ( i=0; lookup_flags[i].name!=NULL; ++i )
	if ( strcmp(lookup_flags[i].name,str)==0 ) {
	    return( lookup_flags[i].flag );
	}

    for ( i=1; i<sf->mark_class_cnt; ++i ) /* Start at 1 because class 0 is unused */
	if ( strcmp(sf->mark_class_names[i],str)==0 ) {
	    return( i<<8 );
	}

    for ( i=0; i<sf->mark_set_cnt; ++i )
	if ( strcmp(sf->mark_set_names[i],str)==0 ) {
	    return( (i<<16) | pst_usemarkfilteringset );
	}

    PyErr_Format(PyExc_ValueError, "Unknown lookup flag %s", str );
    return( -1 );
}

static int ParseLookupFlags(SplineFont *sf,PyObject *flagtuple) {
    int i, flags=0, cnt, temp;

    if ( PyLong_Check(flagtuple))
return( PyLong_AsLong(flagtuple));
    if ( PyUnicode_Check(flagtuple))
return( ParseLookupFlagsItem(sf,flagtuple));
    cnt = PySequence_Size(flagtuple);
    if ( cnt==-1 )
return( -1 );
    for ( i=0; i<cnt; ++i ) {
	PyObject *obj = PySequence_GetItem(flagtuple,i);
	temp = ParseLookupFlagsItem(sf,obj);
	Py_XDECREF(obj);
	if ( temp==-1 )
return( -1 );
	flags |= temp;
    }
return( flags );
}

#define BAD_FEATURE_LIST ((FeatureScriptLangList*)-1)

static FeatureScriptLangList *PyParseFeatureList(PyObject *tuple) {
    FeatureScriptLangList *flhead=NULL, *fltail, *fl;
    struct scriptlanglist *sltail, *sl;
    int f,s,l, cnt;
    int wasmac;
    PyObject *scripts, *langs;

    if ( !PySequence_Check(tuple)) {
	PyErr_Format(PyExc_TypeError, "A feature list is composed of a tuple of tuples" );
return( (FeatureScriptLangList *) -1 );
    }
    cnt = PySequence_Size(tuple);

    for ( f=0; f<cnt; ++f ) {
	PyObject *subs = PySequence_GetItem(tuple,f);
	if ( !PySequence_Check(subs)) {
	    PyErr_Format(PyExc_TypeError, "A feature list is composed of a tuple of tuples" );
	    FeatureScriptLangListFree(flhead);
return( BAD_FEATURE_LIST );
	} else if ( PySequence_Size(subs)!=2 ) {
	    PyErr_Format(PyExc_TypeError, "A feature list is composed of a tuple of tuples each containing two elements");
	    FeatureScriptLangListFree(flhead);
return( BAD_FEATURE_LIST );
	} else if ( !PyUnicode_Check(PySequence_GetItem(subs,0)) ||
		!PySequence_Check(PySequence_GetItem(subs,1))) {
	    PyErr_Format(PyExc_TypeError, "Bad type for argument");
	    FeatureScriptLangListFree(flhead);
return( BAD_FEATURE_LIST );
	}
	fl = chunkalloc(sizeof(FeatureScriptLangList));
	fl->featuretag = StrObjToTag(PySequence_GetItem(subs,0),&wasmac);
	if ( fl->featuretag == BAD_TAG ) {
	    free(fl);
	    FeatureScriptLangListFree(flhead);
return( BAD_FEATURE_LIST );
	}
	fl->ismac = wasmac;
	if ( flhead==NULL )
	    flhead = fl;
	else
	    fltail->next = fl;
	fltail = fl;
	scripts = PySequence_GetItem(subs,1);
	if ( !PySequence_Check(scripts)) {
	    PyErr_Format(PyExc_TypeError, "A script list is composed of a tuple of tuples" );
	    FeatureScriptLangListFree(flhead);
return( BAD_FEATURE_LIST );
	} else if ( PySequence_Size(scripts)==0 ) {
	    PyErr_Format(PyExc_TypeError, "No scripts specified for feature %s", PyBytes_AsString(PySequence_GetItem(subs,0)));
        FeatureScriptLangListFree(flhead);
return( BAD_FEATURE_LIST );
	}
	sltail = NULL;
	for ( s=0; s<PySequence_Size(scripts); ++s ) {
	    PyObject *scriptsubs = PySequence_GetItem(scripts,s);
	    if ( !PySequence_Check(scriptsubs)) {
		PyErr_Format(PyExc_TypeError, "A script list is composed of a tuple of tuples" );
		FeatureScriptLangListFree(flhead);
return( BAD_FEATURE_LIST );
	    } else if ( PySequence_Size(scriptsubs)!=2 ) {
		PyErr_Format(PyExc_TypeError, "A script list is composed of a tuple of tuples each containing two elements");
		FeatureScriptLangListFree(flhead);
return( BAD_FEATURE_LIST );
	    } else if ( !PyUnicode_Check(PySequence_GetItem(scriptsubs,0)) ||
		    !PySequence_Check(PySequence_GetItem(scriptsubs,1))) {
		PyErr_Format(PyExc_TypeError, "Bad type for argument");
		FeatureScriptLangListFree(flhead);
return( BAD_FEATURE_LIST );
	    }
	    sl = chunkalloc(sizeof(struct scriptlanglist));
	    sl->script = StrObjToTag(PySequence_GetItem(scriptsubs,0),NULL);
	    if ( sl->script==BAD_TAG ) {
		free(sl);
		FeatureScriptLangListFree(flhead);
return( BAD_FEATURE_LIST );
	    }
	    if ( sltail==NULL )
		fl->scripts = sl;
	    else
		sltail->next = sl;
	    sltail = sl;
	    langs = PySequence_GetItem(scriptsubs,1);
	    if ( PyUnicode_Check(langs) ) {
		uint32_t lang = StrObjToTag(langs,NULL);
		if ( lang==BAD_TAG ) {
		    FeatureScriptLangListFree(flhead);
return( BAD_FEATURE_LIST );
		}
		sl->lang_cnt = 1;
		sl->langs[0] = lang;
	    } else if ( !PySequence_Check(langs)) {
		PyErr_Format(PyExc_TypeError, "A language list is composed of a tuple of strings" );
		FeatureScriptLangListFree(flhead);
return( BAD_FEATURE_LIST );
	    } else if ( PySequence_Size(langs)==0 ) {
		sl->lang_cnt = 1;
		sl->langs[0] = DEFAULT_LANG;
	    } else {
		sl->lang_cnt = PySequence_Size(langs);
		if ( sl->lang_cnt>MAX_LANG )
		    sl->morelangs = malloc((sl->lang_cnt-MAX_LANG)*sizeof(uint32_t));
		for ( l=0; l<sl->lang_cnt; ++l ) {
		    uint32_t lang = StrObjToTag(PySequence_GetItem(langs,l),NULL);
		    if ( lang==BAD_TAG ) {
			FeatureScriptLangListFree(flhead);
return( BAD_FEATURE_LIST );
		    }
		    if ( l<MAX_LANG )
			sl->langs[l] = lang;
		    else
			sl->morelangs[l-MAX_LANG] = lang;
		}
	    }
	}
    }
return( flhead );
}

static PyObject *PyFFFont_addLookup(PyFF_Font *self, PyObject *args) {
    SplineFont *sf;
    OTLookup *otl, *after = NULL;
    int itype;
    char *lookup_str, *type, *after_str=NULL;
    PyObject *flagtuple=NULL, *featlist;
    int flags;
    FeatureScriptLangList *fl;

    if ( CheckIfFontClosed(self) )
return (NULL);
    sf = self->fv->sf;
    if ( !PyArg_ParseTuple(args,"ssOO|s", &lookup_str, &type, &flagtuple, &featlist, &after_str ))
return( NULL );

    otl = SFFindLookup(sf,lookup_str);
    if ( otl!=NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "A lookup named %s already exists", lookup_str );
return( NULL );
    }
    if ( after_str!=NULL ) {
	after = SFFindLookup(sf,after_str);
	if ( after==NULL ) {
	    PyErr_Format(PyExc_EnvironmentError, "No lookup named %s", after_str );
return( NULL );
	}
    }

    itype = FlagsFromString(type,lookup_types,"lookup type");
    if ( itype==FLAG_UNKNOWN )
return( NULL );

    if (flagtuple == Py_None || flagtuple == NULL)
    flags = 0;
    else
    flags = ParseLookupFlags(sf,flagtuple);
    if ( flags==-1 )
return( NULL );

    fl = PyParseFeatureList(featlist);
    if ( fl==BAD_FEATURE_LIST )
return( NULL );

    if ( after!=NULL && (after->lookup_type>=gpos_start)!=(itype>=gpos_start) ) {
	PyErr_Format(PyExc_EnvironmentError, "After lookup, %s, is in a different table", after_str );
	FeatureScriptLangListFree(fl);
return( NULL );
    }

    if ( sf->cidmaster ) sf = sf->cidmaster;

    otl = chunkalloc(sizeof(OTLookup));
    if ( after!=NULL ) {
	otl->next = after->next;
	after->next = otl;
    } else if ( itype>=gpos_start ) {
	otl->next = sf->gpos_lookups;
	sf->gpos_lookups = otl;
    } else {
	otl->next = sf->gsub_lookups;
	sf->gsub_lookups = otl;
    }
    otl->lookup_type = itype;
    otl->lookup_flags = flags;
    otl->lookup_name = copy(lookup_str);
    otl->features = fl;
    if ( fl!=NULL && (fl->featuretag==CHR('l','i','g','a') || fl->featuretag==CHR('r','l','i','g')))
	otl->store_in_afm = true;
Py_RETURN( self );
}

static PyObject *PyFFFont_importLookups(PyFF_Font *self, PyObject *args) {
    SplineFont *sf;
    SplineFont *othersf;
    PyObject *lookup_list;
    PyFF_Font *otherfont;
    const char *lookup_str, *before_str=NULL;
    OTLookup *otl, *before, **list;
    int i;

    if ( CheckIfFontClosed(self) )
return (NULL);
    sf = self->fv->sf;
    if ( !PyArg_ParseTuple(args,"OO|s", ((PyObject **)&otherfont), &lookup_list, &before_str))
return( NULL );
    if ( !PyType_IsSubtype(&PyFF_FontType, Py_TYPE(otherfont)) ) {
	PyErr_Format(PyExc_TypeError,"First argument must be a fontforge font");
return( NULL );
    }
    if ( CheckIfFontClosed(otherfont) )
return (NULL);

    othersf = otherfont->fv->sf;

    list = NULL;
    before = NULL;
    if ( before_str!=NULL )
	before = SFFindLookup(sf,before_str);
    if ( PyUnicode_Check(lookup_list)) {
	if ((lookup_str = PyUnicode_AsUTF8(lookup_list)) == NULL) {
	    return NULL;
	}
	otl = SFFindLookup(othersf,lookup_str);
	if ( otl==NULL ) {
	    PyErr_Format(PyExc_EnvironmentError, "No lookup named %s exists in %s.", lookup_str, othersf->fontname );
return( NULL );
	}
	list = calloc(2,sizeof(OTLookup *));
	list[0] = otl;
    } else if ( PySequence_Check(lookup_list)) {
	int subcnt = PySequence_Size(lookup_list);
	list = calloc(subcnt+1,sizeof(OTLookup *));
	for ( i=0; i<subcnt; ++i ) {
	    PyObject *str = PySequence_GetItem(lookup_list,i);
	    if ((lookup_str = PyUnicode_AsUTF8(str)) == NULL) {
		Py_DECREF(str);
		free(list);
		return NULL;
	    }
	    otl = SFFindLookup(othersf,lookup_str);
	    Py_DECREF(str);
	    if ( otl==NULL ) {
		PyErr_Format(PyExc_EnvironmentError, "No lookup named %s exists in %s.", lookup_str, othersf->fontname );
		free(list);
return( NULL );
	    }
	    list[i] = otl;
	}
    } else {
	PyErr_Format(PyExc_TypeError, "Unexpected type" );
        free(list);
return( NULL );
    }
    OTLookupsCopyInto(sf,othersf,list,before);
    free(list);
Py_RETURN( self );
}

static PyObject *PyFFFont_lookupSetFeatureList(PyFF_Font *self, PyObject *args) {
    SplineFont *sf;
    OTLookup *otl;
    char *lookup;
    PyObject *featlist;
    FeatureScriptLangList *fl;

    if ( CheckIfFontClosed(self) )
return (NULL);
    sf = self->fv->sf;
    if ( !PyArg_ParseTuple(args,"sO", &lookup, &featlist ))
return( NULL );

    otl = SFFindLookup(sf,lookup);
    if ( otl==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No lookup named %s", lookup );
return( NULL );
    }

    fl = PyParseFeatureList(featlist);
    if ( fl==BAD_FEATURE_LIST )
return( NULL );

    FeatureScriptLangListFree(otl->features);
    otl->features = fl;
Py_RETURN( self );
}

static PyObject *PyFFFont_lookupSetFlags(PyFF_Font *self, PyObject *args) {
    SplineFont *sf;
    OTLookup *otl;
    char *lookup;
    PyObject *flagtuple;
    int flags;

    if ( CheckIfFontClosed(self) )
return (NULL);
    sf = self->fv->sf;
    if ( !PyArg_ParseTuple(args,"sO", &lookup, &flagtuple ))
return( NULL );

    otl = SFFindLookup(sf,lookup);
    if ( otl==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No lookup named %s", lookup );
return( NULL );
    }

    flags = ParseLookupFlags(sf,flagtuple);
    if ( flags==-1 )
return( NULL );

    otl->lookup_flags = flags;
Py_RETURN( self );
}

static PyObject *PyFFFont_lookupSetStoreLigatureInAfm(PyFF_Font *self, PyObject *args) {
    SplineFont *sf;
    OTLookup *otl;
    char *lookup;
    int store_it;

    if ( CheckIfFontClosed(self) )
return (NULL);
    sf = self->fv->sf;
    if ( !PyArg_ParseTuple(args,"si", &lookup, &store_it ))
return( NULL );

    otl = SFFindLookup(sf,lookup);
    if ( otl==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No lookup named %s", lookup );
return( NULL );
    }
    otl->store_in_afm = store_it;
Py_RETURN( self );
}

static PyObject *PyFFFont_getLookupInfo(PyFF_Font *self, PyObject *args) {
    SplineFont *sf;
    OTLookup *otl;
    char *lookup;
    const char *type;
    int i, cnt;
    PyObject *flags_tuple;
    FeatureScriptLangList *fl;
    struct scriptlanglist *sl;
    int fcnt, scnt, l;
    PyObject *farray, *sarray, *larray;

    if ( CheckIfFontClosed(self) )
return (NULL);
    sf = self->fv->sf;
    if ( !PyArg_ParseTuple(args,"s", &lookup ))
return( NULL );

    otl = SFFindLookup(sf,lookup);
    if ( otl==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No lookup named %s", lookup );
return( NULL );
    }

    for ( i=0; lookup_types[i].name!=NULL ; ++i )
	if ( (uint32_t)lookup_types[i].flag == otl->lookup_type )
    break;
    type = lookup_types[i].name;

    cnt = ( otl->lookup_flags&0xff00 )!=0;
    for ( i=0; i<5; ++i )
	if ( otl->lookup_flags&(1<<i) )
	    ++cnt;
    flags_tuple = PyTuple_New(cnt);
    cnt = 0;
    if ( otl->lookup_flags&0xff00 )
	PyTuple_SetItem(flags_tuple,cnt++,Py_BuildValue("s",sf->mark_class_names[ (otl->lookup_flags&0xff00)>>8 ]));
    if ( otl->lookup_flags&pst_usemarkfilteringset )
	PyTuple_SetItem(flags_tuple,cnt++,Py_BuildValue("s",sf->mark_set_names[ (otl->lookup_flags>>16)&0xffff ]));
    for ( i=0; i<4; ++i )
	if ( otl->lookup_flags&(1<<i) )
	    PyTuple_SetItem(flags_tuple,cnt++,Py_BuildValue("s",lookup_flags[i].name));

    for ( fl=otl->features, fcnt=0; fl!=NULL; fl=fl->next, ++fcnt );
    farray = PyTuple_New(fcnt);
    for ( fl=otl->features, fcnt=0; fl!=NULL; fl=fl->next, ++fcnt ) {
	for ( sl=fl->scripts, scnt=0; sl!=NULL; sl=sl->next, ++scnt );
	sarray = PyTuple_New(scnt);
	for ( sl=fl->scripts, scnt=0; sl!=NULL; sl=sl->next, ++scnt ) {
	    larray = PyTuple_New(sl->lang_cnt);
	    for ( l=0; l<sl->lang_cnt; ++l )
		PyTuple_SetItem(larray,l,TagToPythonString(l<MAX_LANG?sl->langs[l]:sl->morelangs[l-MAX_LANG],false));
	    PyTuple_SetItem(sarray,scnt,Py_BuildValue("(OO)",
		    TagToPythonString(sl->script,false),larray));
	}
	PyTuple_SetItem(farray,fcnt,Py_BuildValue("(OO)",
		TagToPythonString(fl->featuretag,fl->ismac),sarray));
    }
return( Py_BuildValue("(sOO)",type,flags_tuple,farray) );
}

static PyObject *PyFFFont_addLookupSubtable(PyFF_Font *self, PyObject *args) {
    SplineFont *sf;
    char *lookup, *subtable, *after_str=NULL;
    OTLookup *otl;

    if ( CheckIfFontClosed(self) )
return (NULL);
    sf = self->fv->sf;
    if ( !PyArg_ParseTuple(args,"ss|s", &lookup, &subtable, &after_str ))
return( NULL );

    otl = SFFindLookup(sf,lookup);
    if ( otl!=NULL ) {
	if ( otl->lookup_type==gsub_context || otl->lookup_type==gsub_contextchain ||
		otl->lookup_type==gpos_context || otl->lookup_type==gpos_contextchain ||
		otl->lookup_type==gsub_reversecchain ) {
	    PyErr_Format(PyExc_TypeError, "Use addContextualSubtable to create a subtable in %s.", lookup );
return( NULL );
	}
    }

    if ( addLookupSubtable(sf, lookup, subtable, after_str)==NULL )
return( NULL );

Py_RETURN( self );
}

static const char *contextchain_keywords[] = {
	"lookup", "subtable", "type", "rule",
	"afterSubtable",
	"bclasses", "mclasses", "fclasses",
	"bclassnames", "mclassnames", "fclassnames", NULL };

static PyObject *PyFFFont_addContextualSubtable(PyFF_Font *self, PyObject *args, PyObject *keywds) {
    SplineFont *sf;
    char *lookup, *subtable, *after_str=NULL, *type, *rule;
    PyObject *bclasses=NULL, *mclasses=NULL, *fclasses=NULL;
    PyObject *bclassnames=NULL, *mclassnames=NULL, *fclassnames=NULL;
    struct lookup_subtable *new_subtable;
    FPST *fpst;
    OTLookup *otl;
    enum fpossub_format format;
    int bcnt=0, mcnt=0, fcnt=0;
    char **backclasses=NULL, **matchclasses=NULL, **forclasses=NULL;
    char **backclassnames=NULL, **matchclassnames=NULL, **forclassnames=NULL;
    int is_warning;
    char *msg;

    if ( CheckIfFontClosed(self) )
return (NULL);
    sf = self->fv->sf;
    if ( !PyArg_ParseTupleAndKeywords(args,keywds,"ssss|sOOOOOO", (char **)contextchain_keywords,
	    &lookup, &subtable, &type, &rule,
	    &after_str, &bclasses, &mclasses, &fclasses,
	    &bclassnames, &mclassnames, &fclassnames))
return( NULL );

    if ( strcasecmp(type,"glyph")==0 )
	format = pst_glyphs;
    else if ( strcasecmp(type,"class")==0 )
	format = pst_class;
    else if ( strcasecmp(type,"coverage")==0 )
	format = pst_coverage;
    else if ( strcasecmp(type,"reversecoverage")==0 )
	format = pst_reversecoverage;
    else {
	PyErr_Format(PyExc_TypeError, "Bad format, %s, for contextual lookup (must be one of \"glyph\", \"class\" or \"coverage\" (or, rarely, \"reversecoverage\"))", type );
return( NULL );
    }

    otl = SFFindLookup(sf,lookup);
    if ( otl!=NULL ) {
	if ( otl->lookup_type!=gsub_context && otl->lookup_type!=gsub_contextchain &&
		otl->lookup_type!=gpos_context && otl->lookup_type!=gpos_contextchain &&
		otl->lookup_type!=gsub_reversecchain ) {
	    PyErr_Format(PyExc_TypeError, "The lookup, %s, may not contain a contextual subtable.\nUse addLookupSubtable() instead", lookup );
return( NULL );
	}
	if ( otl->lookup_type == gsub_reversecchain && format!=pst_reversecoverage ) {
	    PyErr_Format(PyExc_TypeError, "Bad format, %s, for reverse context chaining lookup (must be \"reversecoverage\")", type );
return( NULL );
	}
	if ( otl->lookup_type != gsub_reversecchain && format==pst_reversecoverage ) {
	    PyErr_Format(PyExc_TypeError, "Bad format, %s, for this lookup (must be one of \"glyph\", \"class\" or \"coverage\")", type );
return( NULL );
	}
    }

    if ( format==pst_class && mclasses==NULL ) {
	PyErr_Format(PyExc_TypeError, "When using the class format, you must specify some classes" );
return( NULL );
    } else if ( format!=pst_class &&
	    (mclasses!=NULL || bclasses!=NULL || fclasses!=NULL ||
	     mclassnames!=NULL || bclassnames!=NULL || fclassnames!=NULL )) {
	PyErr_Format(PyExc_TypeError, "When not using the class format, you may not specify any classes" );
return( NULL );
    } else if ( otl!=NULL && ( otl->lookup_type==gsub_context || otl->lookup_type==gpos_context ) &&
	    ( bclasses!=NULL || fclasses!=NULL )) {
	PyErr_Format(PyExc_TypeError, "You may only specify backtracking or forward-looking classes when building a contextual chaining lookup (this one is merely contextual)." );
return( NULL );
    } else if ( (mclasses==NULL && mclassnames!=NULL) ||
		(bclasses==NULL && bclassnames!=NULL) ||
		(fclasses==NULL && fclassnames!=NULL) ) {
	PyErr_Format(PyExc_TypeError, "If you specify class names, you must also specify some classes..." );
return( NULL );
    } else if ( (mclasses!=NULL && mclassnames!=NULL && PySequence_Size(mclasses)!=PySequence_Size(mclassnames)) ||
		(bclasses!=NULL && bclassnames!=NULL && PySequence_Size(bclasses)!=PySequence_Size(bclassnames)) ||
		(fclasses!=NULL && fclassnames!=NULL && PySequence_Size(fclasses)!=PySequence_Size(fclassnames)) ) {
	PyErr_Format(PyExc_TypeError, "When you specify class names there must be as many names as there are classes" );
return( NULL );
    } else if ( format==pst_class ) {
	mcnt = ParseClassNames(mclasses,&matchclasses);
	if ( mcnt==-1 ) {
	    PyErr_Format(PyExc_TypeError, "Bad match class" );
return( NULL );
	}
	if ( mclassnames!=NULL ) {
	    if ((matchclassnames = GlyphNameArrayFromTuple(mclassnames))== NULL ) {
		PyErr_Format(PyExc_TypeError, "Bad set of class names for mclassname." );
return( NULL );
	    }
	}
	if ( bclasses!=NULL ) {
	    bcnt = ParseClassNames(bclasses,&backclasses);
	    if ( bcnt==-1 ) {
		PyErr_Format(PyExc_TypeError, "Bad backtrack class" );
return( NULL );
	    }
	    if ( bclassnames!=NULL ) {
		if ((backclassnames = GlyphNameArrayFromTuple(bclassnames))== NULL ) {
		    PyErr_Format(PyExc_TypeError, "Bad set of class names for bclassname." );
return( NULL );
		}
	    }
	}
	if ( fclasses!=NULL ) {
	    fcnt = ParseClassNames(fclasses,&forclasses);
	    if ( fcnt==-1 ) {
		PyErr_Format(PyExc_TypeError, "Bad forward class" );
return( NULL );
	    }
	    if ( fclassnames!=NULL ) {
		if ((forclassnames = GlyphNameArrayFromTuple(fclassnames))== NULL ) {
		    PyErr_Format(PyExc_TypeError, "Bad set of class names for fclassname." );
return( NULL );
		}
	    }
	}
    }

    new_subtable = addLookupSubtable(sf, lookup, subtable, after_str);
    if ( new_subtable==NULL ) {
	free(backclassnames); free(matchclasses); free(forclasses);
return( NULL );
    }
    fpst = chunkalloc(sizeof(FPST));
    fpst->subtable = new_subtable;
    new_subtable->fpst = fpst;
    fpst->format = format;
    fpst->type = otl->lookup_type==gsub_reversecchain ? pst_reversesub :
		otl->lookup_type==gsub_context ? pst_contextsub :
		otl->lookup_type==gsub_contextchain ? pst_chainsub :
		otl->lookup_type==gpos_context ? pst_contextpos :
		  pst_chainpos;
    fpst->next = sf->possub;
    sf->possub = fpst;
    fpst->bccnt = bcnt;
    fpst->bclass = backclasses;
    if ( backclasses!=NULL && backclassnames==NULL )
	fpst->bclassnames = calloc(bcnt,sizeof(char *));
    else
	fpst->bclassnames = backclassnames;
    fpst->nccnt = mcnt;
    fpst->nclass = matchclasses;
    if ( matchclasses!=NULL && matchclassnames==NULL )
	fpst->nclassnames = calloc(mcnt,sizeof(char *));
    else
	fpst->nclassnames = matchclassnames;
    fpst->fccnt = fcnt;
    fpst->fclass = forclasses;
    if ( forclasses!=NULL && forclassnames==NULL )
	fpst->fclassnames = calloc(fcnt,sizeof(char *));
    else
	fpst->fclassnames = forclassnames;
    fpst->rule_cnt = 1;
    fpst->rules = calloc(1,sizeof(struct fpst_rule));

    msg = FPSTRule_From_Str( sf,fpst,fpst->rules,rule,&is_warning);
    if ( is_warning ) {
	LogError("%s",msg);
	free(msg);
	msg = NULL;
    }
    if ( msg!=NULL ) {
	PyErr_Format(PyExc_TypeError, "%s", msg );
	free(msg);
return( NULL );
    }

Py_RETURN( self );
}

static PyObject *PyFFFont_getLookupSubtables(PyFF_Font *self, PyObject *args) {
    SplineFont *sf;
    char *lookup;
    OTLookup *otl;
    struct lookup_subtable *sub;
    int cnt;
    PyObject *tuple;

    if ( CheckIfFontClosed(self) )
return (NULL);
    sf = self->fv->sf;
    if ( !PyArg_ParseTuple(args,"s", &lookup ))
return( NULL );

    otl = SFFindLookup(sf,lookup);
    if ( otl==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No lookup named %s", lookup );
return( NULL );
    }
    for ( sub = otl->subtables, cnt=0; sub!=NULL; sub=sub->next, ++cnt );
    tuple = PyTuple_New(cnt);
    for ( sub = otl->subtables, cnt=0; sub!=NULL; sub=sub->next, ++cnt )
	PyTuple_SetItem(tuple,cnt,Py_BuildValue("s",sub->subtable_name));
return( tuple );
}

static PyObject *PyFFFont_getLookupSubtableAnchorClasses(PyFF_Font *self, PyObject *args) {
    SplineFont *sf;
    char *subtable;
    struct lookup_subtable *sub;
    AnchorClass *ac;
    int cnt;
    PyObject *tuple;

    if ( CheckIfFontClosed(self) )
return (NULL);
    sf = self->fv->sf;
    if ( !PyArg_ParseTuple(args,"s", &subtable ))
return( NULL );

    sub = SFFindLookupSubtable(sf,subtable);
    if ( sub==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No lookup subtable named %s", subtable );
return( NULL );
    }
    for ( ac = sf->anchor, cnt=0; ac!=NULL; ac=ac->next )
	if ( ac->subtable == sub )
	    ++cnt;
    tuple = PyTuple_New(cnt);
    for ( ac = sf->anchor, cnt=0; ac!=NULL; ac=ac->next )
	if ( ac->subtable == sub )
	    PyTuple_SetItem(tuple,cnt++,Py_BuildValue("s",ac->name));
return( tuple );
}

static PyObject *PyFFFont_getLookupOfSubtable(PyFF_Font *self, PyObject *args) {
    SplineFont *sf;
    char *subtable;
    struct lookup_subtable *sub;

    if ( CheckIfFontClosed(self) )
return (NULL);
    sf = self->fv->sf;
    if ( !PyArg_ParseTuple(args,"s", &subtable ))
return( NULL );

    sub = SFFindLookupSubtable(sf,subtable);
    if ( sub==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No lookup subtable named %s", subtable );
return( NULL );
    }
return( Py_BuildValue("s", sub->lookup->lookup_name ));
}

static PyObject *PyFFFont_getSubtableOfAnchor(PyFF_Font *self, PyObject *args) {
    SplineFont *sf;
    char *anchorclass;
    AnchorClass *ac;

    if ( CheckIfFontClosed(self) )
return (NULL);
    sf = self->fv->sf;
    if ( !PyArg_ParseTuple(args,"s", &anchorclass ))
return( NULL );

    for ( ac=sf->anchor; ac!=NULL; ac=ac->next )
	if ( strcmp(ac->name,anchorclass)==0 )
return( Py_BuildValue("s", ac->subtable ? ac->subtable->subtable_name : ""));

    PyErr_Format(PyExc_EnvironmentError, "No anchor class named %s", anchorclass );
return( NULL );
}

static PyObject *PyFFFont_saveNamelist(PyFF_Font *self, PyObject *args) {
    FontViewBase *fv;
    char *filename;
    FILE *file;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    if ( !PyArg_ParseTuple(args,"s", &filename ))
return( NULL );

    file = fopen(filename,"w");
    if ( file==NULL ) {
	PyErr_SetFromErrnoWithFilename(PyExc_IOError,filename);
return(NULL);
    }
    FVB_MakeNamelist(fv, file);
    fclose(file);
Py_RETURN_NONE;
}

static PyObject *PyFFFont_replaceAll(PyFF_Font *self, PyObject *args) {
    FontViewBase *fv;
    PyObject *srch, *rpl;
    SplineSet *srch_ss, *rpl_ss;
    double err = .01;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    if ( !PyArg_ParseTuple(args,"OO|d", &srch, &rpl, &err ))
return( NULL );

    if ( PyType_IsSubtype(&PyFF_LayerType, Py_TYPE(srch)) ) {
	srch_ss = SSFromLayer((PyFF_Layer *) srch);
    } else if ( PyType_IsSubtype(&PyFF_ContourType, Py_TYPE(srch)) ) {
	srch_ss = SSFromContour((PyFF_Contour *) srch, NULL);
    } else {
	PyErr_Format(PyExc_TypeError, "Expected a contour or layer");
return( NULL );
    }
    if ( PyErr_Occurred() != NULL ) {
	return( NULL );
    }

    if ( PyType_IsSubtype(&PyFF_LayerType, Py_TYPE(rpl)) ) {
	rpl_ss = SSFromLayer((PyFF_Layer *) rpl);
    } else if ( PyType_IsSubtype(&PyFF_ContourType, Py_TYPE(rpl)) ) {
	rpl_ss = SSFromContour((PyFF_Contour *) rpl, NULL);
    } else {
	PyErr_Format(PyExc_TypeError, "Expected a contour or layer");
return( NULL );
    }
    if ( PyErr_Occurred() != NULL ) {
	SplinePointListsFree(srch_ss);
	return( NULL );
    }

    /* srch_ss and rpl_ss will be freed by ReplaceAll */
return( Py_BuildValue( "i", FVReplaceAll(fv,srch_ss,rpl_ss,err,sv_reverse|sv_flips)));
}

/* Search flags: see 'enum search_flags' in baseviews.h */
struct flaglist find_flags[] = {
    {"reverse", sv_reverse},
    {"flips", sv_flips},
    {"rotate", sv_rotate},
    {"scale", sv_scale},
    {"endpoints", sv_endpoints},
    FLAGLIST_EMPTY /* Sentinel */
};

static PyObject *PyFFFont_find(PyFF_Font *self, PyObject *args) {
    FontViewBase *fv;
    PyObject *srch;
    SplineSet *srch_ss;
    PyObject *pyflags = NULL;
    int flags = 0;
    double err = .01;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    if ( !PyArg_ParseTuple(args,"O|dO", &srch, &err, &pyflags))
return( NULL );

    if ( PyType_IsSubtype(&PyFF_LayerType, Py_TYPE(srch)) ) {
	srch_ss = SSFromLayer((PyFF_Layer *) srch);
    } else if ( PyType_IsSubtype(&PyFF_ContourType, Py_TYPE(srch)) ) {
	srch_ss = SSFromContour((PyFF_Contour *) srch, NULL);
    } else {
	PyErr_Format(PyExc_TypeError, "Expected a contour or layer");
return( NULL );
    }
    if ( PyErr_Occurred() != NULL ) {
	return( NULL );
    }
    if (pyflags) {
	flags = FlagsFromTuple(pyflags, find_flags, "search flag");
	if ( flags==FLAG_UNKNOWN )
return( NULL );
    }
    else
	flags = sv_reverse|sv_flips;

    /* srch_ss will be freed when the iterator dies */
return( fontiter_New( self,false,SDFromContour(fv,srch_ss,err,flags)) );
}

static PyObject *PyFFFont_glyphs(PyFF_Font *self, PyObject *args) {
    const char *type = "GID";
    int index;

    if ( CheckIfFontClosed(self) )
return (NULL);
    if ( !PyArg_ParseTuple(args,"|s", &type ))
return( NULL );

    if ( strcasecmp(type,"GID")==0 )
	index = 3;
    else if ( strcasecmp(type,"encoding")==0 )
	index = 4;
    else {
	PyErr_Format(PyExc_TypeError, "Unexpected type");
return( NULL );
    }

return( fontiter_New( self,index,NULL) );
}


static PyObject *PyFFFont_Save(PyFF_Font *self, PyObject *args) {
    char *filename = NULL;
    int localRevisionsToRetain = -1;
    char *locfilename = NULL;
    char *pt;
    FontViewBase *fv;
    int s2d=false;

    if ( CheckIfFontClosed(self) )
	return(NULL);
    fv = self->fv;

    if ( !PyArg_ParseTuple(args,"|si", &filename, &localRevisionsToRetain ))
        return( NULL );

    if ( filename!=NULL )
    {
	/* Save As - Filename was provided */
	locfilename = utf82def_copy(filename);

	pt = strrchr(locfilename,'.');
	if ( pt!=NULL && strmatch(pt,".sfdir")==0 )
	    s2d = true;

	int rc = SFDWriteBakExtended( locfilename,
				      fv->sf,fv->map,fv->normal,s2d,
				      localRevisionsToRetain );
	if ( !rc )
	{
	    PyErr_Format(PyExc_EnvironmentError, "Save As \"%s\" failed",locfilename);
	    free(locfilename);
	    return( NULL );
	}
    }
    else
    {
	/* Save - No filename provided */
	if ( fv->cidmaster!=NULL && fv->cidmaster->filename!=NULL )
	    filename = fv->cidmaster->filename;
	else if ( fv->sf->mm!=NULL && fv->sf->mm->normal->filename!=NULL )
	    filename = fv->sf->mm->normal->filename;
	else if ( fv->sf->filename!=NULL )
	    filename = fv->sf->filename;
	else {
	    SplineFont *sf = fv->cidmaster?fv->cidmaster:
		    fv->sf->mm!=NULL?fv->sf->mm->normal:fv->sf;
	    char *fn = sf->defbasefilename ? sf->defbasefilename : sf->fontname;
	    locfilename = malloc((strlen(fn)+10));
	    strcpy(locfilename,fn);
	    if ( sf->defbasefilename!=NULL )
		/* Don't add a default suffix, they've already told us what name to use */;
	    else if ( fv->cidmaster!=NULL )
		strcat(locfilename,"CID");
	    else if ( sf->mm==NULL )
		;
	    else if ( sf->mm->apple )
		strcat(locfilename,"Var");
	    else
		strcat(locfilename,"MM");
	    strcat(locfilename,".sfd");
	}
	char* targetfilename = locfilename;
	if ( !targetfilename )
	    targetfilename = fv->sf->filename;

	/**
	 * If there are no existing backup files, don't start creating them here.
	 * Otherwise, save as many as the user wants.
	 */
	int rc = SFDWriteBakExtended( targetfilename,
				      fv->sf,fv->map,fv->normal,s2d,
				      localRevisionsToRetain );
	if ( !rc )
	{
	    PyErr_Format(PyExc_EnvironmentError, "Save failed");
        free(locfilename);
	    return( NULL );
	}
    }

    /* Save succeeded, do any post-save fixups.
     * Refer to _FVMenuSaveAs() in fontview.c
     */
    if ( locfilename!=NULL ) {
	SplineFont *sf = fv->cidmaster?fv->cidmaster:fv->sf->mm!=NULL?fv->sf->mm->normal:fv->sf;
	free(sf->filename);
	sf->filename = copy(locfilename);
	sf->save_to_dir = s2d;
	free(sf->origname);
	sf->origname = copy(locfilename);
	sf->new = false;
	if ( sf->mm!=NULL ) {
	    int i;
	    for ( i=0; i<sf->mm->instance_count; ++i ) {
		free(sf->mm->instances[i]->filename);
		sf->mm->instances[i]->filename = copy(locfilename);
		free(sf->mm->instances[i]->origname);
		sf->mm->instances[i]->origname = copy(locfilename);
		sf->mm->instances[i]->new = false;
	    }
	}
	SplineFontSetUnChanged(sf);

	free(locfilename);
    }

Py_RETURN( self );
}

static PyObject *PyFFFont_revert(PyFF_Font *self, PyObject *UNUSED(args)) {
    FontViewBase *fv;
    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    FVRevert(fv);
Py_RETURN( self );
}

static PyObject *PyFFFont_revertFromBackup(PyFF_Font *self, PyObject *UNUSED(args)) {
    FontViewBase *fv;
    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    FVRevertBackup(fv);
Py_RETURN( self );
}

/* filename, bitmaptype,flags,resolution,mult-sfd-file,namelist */
static const char *gen_keywords[] = { "filename", "bitmap_type", "flags", "bitmap_resolution",
	"subfont_directory", "namelist", "layer", NULL };
static const char *genttc_keywords[] = { "filename", "others", "bitmap_type", "flags",
	"ttcflags", "namelist", "layer", NULL };
struct flaglist gen_flags[] = {
    { "afm", fm_flag_afm },
    { "pfm", fm_flag_pfm },
    { "short-post", fm_flag_shortps },
    { "omit-instructions", fm_flag_nottfhints },
    { "apple", fm_flag_apple },
    { "opentype", fm_flag_opentype },
    { "PfEd-comments", fm_flag_pfed_comments },
    { "PfEd-colors", fm_flag_pfed_colors },
    { "PfEd-lookups", fm_flag_pfed_lookups },
    { "PfEd-guides", fm_flag_pfed_guides },
    { "PfEd-background", fm_flag_pfed_layers },
    { "winkern", fm_flag_winkern },
    { "glyph-map-file", fm_flag_glyphmap },
    { "TeX-table", fm_flag_TeXtable },
    { "ofm", fm_flag_ofm },
    { "old-kern", fm_flag_applemode },
    { "symbol", fm_flag_symbol },
    { "dummy-dsig", fm_flag_dummyDSIG },
    { "dummy-DSIG", fm_flag_dummyDSIG },
    { "no-FFTM-table", fm_flag_nofftm },
    { "tfm", fm_flag_tfm },
    { "no-flex", fm_flag_noflex },
    { "no-hints", fm_flag_nopshints },
    { "round", fm_flag_round },
    { "composites-in-afm", fm_flag_afmwithmarks },
    { "no-mac-names", fm_flag_nomacnames },
    FLAGLIST_EMPTY /* Sentinel */
};
/* Generate TrueType Collection flags: see 'enum ttc_flags' in splinefont.h */
struct flaglist genttc_flags[] = {
    { "merge", ttc_flag_trymerge },
    { "cff", ttc_flag_cff },
    FLAGLIST_EMPTY /* Sentinel */
};

static PyObject *PyFFFont_Generate(PyFF_Font *self, PyObject *args, PyObject *keywds) {
    char *filename;
    char *locfilename = NULL;
    FontViewBase *fv;
    PyObject *flags=NULL;
    int iflags = -1;
    int resolution = -1;
    const char *bitmaptype="";
    char *subfontdirectory=NULL, *namelist=NULL;
    NameList *rename_to = NULL;
    int layer;
    char *layer_str=NULL;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    layer = fv->active_layer;
    if ( !PyArg_ParseTupleAndKeywords(args, keywds, "s|sOissi", (char **)gen_keywords,
	    &filename, &bitmaptype, &flags, &resolution, &subfontdirectory,
	    &namelist, &layer) ) {
	PyErr_Clear();
	if ( !PyArg_ParseTupleAndKeywords(args, keywds, "s|sOisss", (char **)gen_keywords,
		&filename, &bitmaptype, &flags, &resolution, &subfontdirectory,
		&namelist, &layer_str) )
return( NULL );
	layer = SFFindLayerIndexByName(fv->sf,layer_str);
	if ( layer<0 )
return( NULL );
    }
    if ( layer<0 || layer>=fv->sf->layer_cnt ) {
	PyErr_Format(PyExc_ValueError, "Layer is out of range" );
return( NULL );
    }
    if ( flags!=NULL ) {
	iflags = FlagsFromTuple(flags,gen_flags,"generate flag");
	if ( iflags==FLAG_UNKNOWN ) {
return( NULL );
	}
	/* Legacy screw ups mean that opentype & apple bits don't mean what */
	/*  I want them to. Python users should not see that, but fix it up */
	/*  here */
	if ( (iflags&0x80) && (iflags&0x10) )	/* Both */
	    iflags &= ~0x10;
	else if ( (iflags&0x80) && !(iflags&0x10)) /* Just opentype */
	    iflags &= ~0x80;
	else if ( !(iflags&0x80) && (iflags&0x10)) /* Just apple */
	    /* This one's set already */;
	else
	    iflags |= 0x90;
    }
    if ( namelist!=NULL ) {
	rename_to = NameListByName(namelist);
	if ( rename_to==NULL ) {
	    PyErr_Format(PyExc_EnvironmentError, "Unknown namelist");
return( NULL );
	}
    }
    locfilename = utf82def_copy(filename);
    if ( !GenerateScript(fv->sf,locfilename,bitmaptype,iflags,resolution,subfontdirectory,
	    NULL,fv->normal==NULL?fv->map:fv->normal,rename_to,layer) ) {
	PyErr_Format(PyExc_EnvironmentError, "Font generation failed");
return( NULL );
    }
    free(locfilename);
Py_RETURN( self );
}


static void freesflist(struct sflist* list) {
    struct sflist *next;
    for( ; list != NULL; list=next ) {
	next = list->next;

        free(list->sizes);
	chunkfree(list, sizeof(struct sflist));
    }
    return;
}

static struct sflist *makesflist(PyFF_Font *font,enum bitmapformat bf) {
    struct sflist *ret;

    if ( CheckIfFontClosed(font) )
return(NULL);

    ret = chunkalloc(sizeof( struct sflist ));
    ret->sf  = font->fv->sf;
    ret->map = font->fv->map;

    if ( bf==bf_ttf ) {
	int cnt;
	BDFFont *bdf;
	for ( cnt=0, bdf=ret->sf->bitmaps; bdf!=NULL; bdf=bdf->next, ++cnt );
	if ( cnt!=0 ) {
	    ret->sizes = malloc((cnt+1)*sizeof(int32_t));
	    ret->sizes[cnt] = 0;
	    for ( cnt=0, bdf=ret->sf->bitmaps; bdf!=NULL; bdf=bdf->next, ++cnt ) {
		ret->sizes[cnt] = (BDFDepth(bdf)<<16) | bdf->pixelsize;
	    }
	}
    }
return( ret );
}

static PyObject *PyFFFont_GenerateTTC(PyFF_Font *self, PyObject *args, PyObject *keywds) {
    char *filename;
    char *locfilename = NULL;
    FontViewBase *fv;
    PyObject *flags=NULL, *ttcflags=NULL, *others=NULL;
    int iflags = old_sfnt_flags;
    int ittcflags = 0;
    const char *bitmaptype="", *namelist=NULL;
    NameList *rename_to = NULL;
    int layer;
    char *layer_str=NULL;
    struct sflist *head=NULL, *last, *cur;
    enum bitmapformat bf = bf_none;

    if ( CheckIfFontClosed(self) )
return(NULL);
    fv = self->fv;

    if ( !PyArg_ParseTupleAndKeywords(args, keywds, "sO|sOOsi", (char **)genttc_keywords,
	    &filename, &others, &bitmaptype, &flags, &ttcflags,
	    &namelist, &layer) ) {
	PyErr_Clear();
	if ( !PyArg_ParseTupleAndKeywords(args, keywds, "sO|sOOss", (char **)genttc_keywords,
		&filename, &others, &bitmaptype, &flags, &ttcflags,
		&namelist, &layer_str) )
return( NULL );
	layer = SFFindLayerIndexByName(fv->sf,layer_str);
	if ( layer<0 )
return( NULL );
    }
    if ( layer<0 || layer>=fv->sf->layer_cnt ) {
	PyErr_Format(PyExc_ValueError, "Layer is out of range" );
return( NULL );
    }
    if ( flags!=NULL ) {
	iflags = FlagsFromTuple(flags,gen_flags,"generate flag");
	if ( iflags==FLAG_UNKNOWN ) {
return( NULL );
	}
	/* Legacy screw ups mean that opentype & apple bits don't mean what */
	/*  I want them to. Python users should not see that, but fix it up */
	/*  here */
	if ( (iflags&0x80) && (iflags&0x10) )	/* Both */
	    iflags &= ~0x10;
	else if ( (iflags&0x80) && !(iflags&0x10)) /* Just opentype */
	    iflags &= ~0x80;
	else if ( !(iflags&0x80) && (iflags&0x10)) /* Just apple */
	    /* This one's set already */;
	else
	    iflags |= 0x90;
    }
    if ( ttcflags!=NULL ) {
	ittcflags = FlagsFromTuple(ttcflags,genttc_flags,"generate TTC flag");
	if ( ittcflags==FLAG_UNKNOWN ) {
return( NULL );
	}
    }
    if ( bitmaptype!=NULL && *bitmaptype!='\0' ) {
	if ( strcasecmp(bitmaptype,"ttf")==0 )
	    bf = bf_ttf;
	else {
	    PyErr_Format(PyExc_TypeError, "Unknown bitmap format");
return( NULL );
	}
    }
    if ( namelist!=NULL ) {
	rename_to = NameListByName(namelist);
	if ( rename_to==NULL ) {
	    PyErr_Format(PyExc_EnvironmentError, "Unknown namelist");
return( NULL );
	}
    }

    head = last = makesflist(self,bf);
    if ( others==Py_None )
	/* Silly to have a ttc with just one font, but ok */;
    else if ( PyType_IsSubtype(&PyFF_FontType, Py_TYPE(others)) ) {
	PyFF_Font* otherfont = (PyFF_Font*)others;
	if ( CheckIfFontClosed(otherfont) ) {
	    freesflist(head);
return(NULL);
	}
	cur = makesflist(otherfont,bf);
	last->next = cur;
	last = cur;
    } else if ( PySequence_Check(others) ) {
	int i, subcnt = PySequence_Size(others);
	for ( i=0; i<subcnt; ++i ) {
	    PyObject *item = PySequence_GetItem(others,i);
	    if ( PyType_IsSubtype(&PyFF_FontType, Py_TYPE(item)) ) {
		PyFF_Font* otherfont = (PyFF_Font*)item;
		if( CheckIfFontClosed(otherfont) ) {
		    freesflist(head);
return(NULL);
		}
		cur = makesflist(otherfont,bf);
		last->next = cur;
		last = cur;
	    } else {
		PyErr_Format(PyExc_TypeError, "Second argument must be either a font or a list of fonts");
return( NULL );
	    }
	}
    } else {
	PyErr_Format(PyExc_TypeError, "Second argument must be either a font or a list of fonts");
return( NULL );
    }

    locfilename = utf82def_copy(filename);

    if ( !WriteTTC(locfilename,head,ff_ttc,bf,iflags,layer,ittcflags)) {
	PyErr_Format(PyExc_EnvironmentError, "Font generation failed");
	/* Don't return here, will return after memory is freed below */
    }
    free(locfilename);
    freesflist(head);
    if ( PyErr_Occurred() )
return( NULL );
Py_RETURN( self );
}

static PyObject *PyFFFont_GenerateFeature(PyFF_Font *self, PyObject *args) {
    char *filename;
    char *locfilename = NULL;
    char *lookup_name = NULL;
    FontViewBase *fv;
    FILE *out;
    OTLookup *otl = NULL;
    int err;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    if ( !PyArg_ParseTuple(args,"s|s",&filename,&lookup_name) )
return( NULL );
    locfilename = utf82def_copy(filename);

    if ( lookup_name!=NULL ) {
	otl = SFFindLookup(fv->sf,lookup_name);
	if ( otl == NULL ) {
	    PyErr_Format(PyExc_EnvironmentError, "No lookup named %s", lookup_name );
return( NULL );
	}
    }
    out = fopen(locfilename,"w");
    if ( out==NULL ) {
	PyErr_SetFromErrnoWithFilename(PyExc_IOError,locfilename);
	free(locfilename);
return( NULL );
    }
    if ( otl!=NULL )
	FeatDumpOneLookup(out,fv->sf,otl);
    else
	FeatDumpFontLookups(out,fv->sf);
    err = ferror(out);
    if ( fclose(out)!=0 || err ) {
	PyErr_Format(PyExc_EnvironmentError, "IO error on file %s", locfilename);
	free(locfilename);
return( NULL );
    }
    free(locfilename);
Py_RETURN( self );
}

static PyObject *PyFFFont_MergeKern(PyFF_Font *self, PyObject *args) {
    char *filename;
    char *locfilename = NULL;
    int ignore_invalid_replacement = 0;
    FontViewBase *fv;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    if ( !PyArg_ParseTuple(args,"s|i",&filename,&ignore_invalid_replacement) )
return( NULL );
    locfilename = utf82def_copy(filename);

    if ( !LoadKerningDataFromMetricsFile(fv->sf,locfilename,fv->map,(bool)ignore_invalid_replacement)) {
	PyErr_Format(PyExc_EnvironmentError, "No metrics data found");
return( NULL );
    }
    free(locfilename);
Py_RETURN( self );
}

static PyObject *PyFFFont_MergeFonts(PyFF_Font *self, PyObject *args) {
    char *filename;
    char *locfilename = NULL;
    FontViewBase *fv;
    SplineFont *sf;
    int openflags=0;
    int preserveCrossFontKerning = 0;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    if ( !PyArg_ParseTuple(args,"s|ii",&filename,
	    &preserveCrossFontKerning, &openflags) ) {
	PyFF_Font *other;
	PyErr_Clear();
	if ( !PyArg_ParseTuple(args,"O!|i",&PyFF_FontType,&other,
		&preserveCrossFontKerning) || CheckIfFontClosed(other) )
return( NULL );
	sf = other->fv->sf;
    } else {
	locfilename = utf82def_copy(filename);
	sf = LoadSplineFont(locfilename,openflags);
	if ( sf==NULL ) {
	    PyErr_Format(PyExc_EnvironmentError, "No font found in file \"%s\"", locfilename);
	    free(locfilename);
return( NULL );
	}
    }
    free(locfilename);
    if ( sf->fv==NULL )
	EncMapFree(sf->map);
    MergeFont(fv,sf,preserveCrossFontKerning);
Py_RETURN( self );
}

static PyObject *PyFFFont_InterpolateFonts(PyFF_Font *self, PyObject *args) {
    char *filename;
    char *locfilename = NULL;
    FontViewBase *fv, *newfv;
    SplineFont *sf;
    int openflags=0;
    double fraction;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    if ( !PyArg_ParseTuple(args,"ds|i",&fraction,&filename, &openflags) )
return( NULL );
    locfilename = utf82def_copy(filename);
    sf = LoadSplineFont(locfilename,openflags);
    if ( sf==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No font found in file \"%s\"", locfilename);
	free(locfilename);
return( NULL );
    }
    free(locfilename);
    if ( sf->fv==NULL )
	EncMapFree(sf->map);
    newfv = SFAdd(InterpolateFont(fv->sf,sf,fraction, fv->map->enc ),false);
return( PyFF_FontForFV_I(newfv));
}

static PyObject *PyFFFont_CreateMappedChar(PyFF_Font *self, PyObject *args) {
    int enc;
    char *str;
    FontViewBase *fv;
    SplineChar *sc;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    if ( !PyArg_ParseTuple(args,"s", &str ) ) {
	PyErr_Clear();
	if ( !PyArg_ParseTuple(args,"i", &enc ) )
return( NULL );
	if ( enc<0 || enc>fv->map->enccount ) {
	    PyErr_Format(PyExc_ValueError, "Encoding is out of range" );
return( NULL );
	}
    } else {
	enc = SFFindSlot(fv->sf, fv->map, -1, str );
	if ( enc==-1 ) {
	    PyErr_Format(PyExc_ValueError, "Glyph name, %s, not in current encoding", str );
return( NULL );
	}
    }
    sc = SFMakeChar(fv->sf,fv->map,enc);
return( PySC_From_SC_I( sc ));
}

static PyObject *PyFFFont_CreateUnicodeChar(PyFF_Font *self, PyObject *args) {
    int uni, enc;
    char *name=NULL;
    FontViewBase *fv;
    SplineChar *sc;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    if ( !PyArg_ParseTuple(args,"i|s", &uni, &name ) )
return( NULL );
    if ( uni<-1 || uni >= (int)unicode4_size ) {
	PyErr_Format(PyExc_ValueError, "Unicode codepoint, %d, out of range, must be either -1 or between 0 and 0x10ffff", uni );
return( NULL );
    } else if ( uni==-1 && name==NULL ) {
	PyErr_Format(PyExc_ValueError, "If you do not specify a code point, you must specify a name.");
return( NULL );
    }

    enc = SFFindSlot(fv->sf, fv->map, uni, name );
    if ( enc!=-1 ) {
	sc = SFMakeChar(fv->sf,fv->map,enc);
	if ( name!=NULL ) {
	    free(sc->name);
	    sc->name = copy(name);
	    GlyphHashFree(fv->sf);
	}
    } else {
	sc = SFGetOrMakeChar(fv->sf,uni,name);
	/* does not add to current map. But since we didn't find a slot it's */
	/*  not in the encoding. We could add it in the unencoded area */
    }
return( PySC_From_SC_I( sc ));
}

static PyObject *PyFFFont_CreateInterpolatedGlyph(PyFF_Font *self, PyObject *args) {
    FontViewBase *fv;
    SplineFont *sf;
    PyObject *from, *to;
    double by;
    SplineChar *sc;
    int baseenc;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    sf = fv->sf;
    if ( !PyArg_ParseTuple(args,"OOd",&from,&to,&by) )
return( NULL );
    if ( !PyType_IsSubtype(&PyFF_GlyphType, Py_TYPE(from)) ||
         !PyType_IsSubtype(&PyFF_GlyphType, Py_TYPE(to))) {
	PyErr_Format(PyExc_TypeError, "Expected glyph objects");
return( NULL );
    }
    if ( SFGetChar(fv->sf,((PyFF_Glyph *) from)->sc->unicodeenc,((PyFF_Glyph *) from)->sc->name)!=NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "This glyph already exists in the font");
return( NULL );
    }

    sc = SplineCharInterpolate(((PyFF_Glyph *) from)->sc,((PyFF_Glyph *) to)->sc,by, sf);
    if ( sc==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "Interpolation failed");
return( NULL );
    }

    baseenc = EncFromUni(sc->unicodeenc,fv->map->enc);
    if ( baseenc==-1 )
	baseenc = fv->map->enccount+1;

    SFAddGlyphAndEncode(sf,sc,fv->map,baseenc);
return( PySC_From_SC_I( sc ));
}

static PyObject *PyFFFont_findEncodingSlot(PyFF_Font *self, PyObject *args) {
    int uni= -1;
    char *name=NULL;
    FontViewBase *fv;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    if ( !PyArg_ParseTuple(args,"s", &name ) ) {
	PyErr_Clear();
	if ( !PyArg_ParseTuple(args,"i", &uni ) )
return( NULL );
    }
    if ( uni<-1 || uni >= (int)unicode4_size ) {
	PyErr_Format(PyExc_ValueError, "Unicode codepoint, %d, out of range, must be either -1 or between 0 and 0x10ffff", uni );
return( NULL );
    }

return( Py_BuildValue("i",SFFindSlot(fv->sf, fv->map, uni, name )) );
}

static PyObject *PyFFFont_removeGlyph(PyFF_Font *self, PyObject *args) {
    int uni;
    char *name=NULL;
    FontViewBase *fv;
    SplineChar *sc;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    if ( PyTuple_Size(args)==1 && PyType_IsSubtype(&PyFF_GlyphType, Py_TYPE(PyTuple_GetItem(args,0))) ) {
	sc = ((PyFF_Glyph *) PyTuple_GetItem(args,0))->sc;
	if ( sc->parent!=fv->sf ) {
	    PyErr_Format(PyExc_ValueError, "This glyph is not in the font");
return( NULL );
	}
    } else {
	if ( PyTuple_Size(args)==1 && PyUnicode_Check(PyTuple_GetItem(args,0)) ) {
	    if ( !PyArg_ParseTuple(args,"s", &name ) )
return( NULL );
	    uni = -1;
	} else if ( !PyArg_ParseTuple(args,"i|s", &uni, &name ) )
return( NULL );
	if ( uni<-1 || uni >= (int)unicode4_size ) {
	    PyErr_Format(PyExc_ValueError, "Unicode codepoint, %d, out of range, must be either -1 or between 0 and 0x10ffff", uni );
return( NULL );
	} else if ( uni==-1 && name==NULL ) {
	    PyErr_Format(PyExc_ValueError, "If you do not specify a code point, you must specify a name.");
return( NULL );
	}
	sc = SFGetChar(fv->sf,uni,name);
	if ( sc==NULL ) {
	    PyErr_Format(PyExc_ValueError, "This glyph is not in the font");
return( NULL );
	}
    }
    SFRemoveGlyph(fv->sf,sc);
Py_RETURN( self );
}


/* Print sample flags */
static struct flaglist printflags[] = {
    { "fontdisplay", 0 },
    { "chars", 1 },
    { "waterfall", 2 },
    { "fontsample", 3 },
    { "fontsampleinfile", 4 },
    { "multisize", 2 },
    FLAGLIST_EMPTY /* Sentinel */
};

/* font.printSample(type, [pointsize, [sample, [output-filename]]])     */

static PyObject *PyFFFont_printSample(PyFF_Font *self, PyObject *args) {
    int type, i, inlinesample = true;
    char *typeArg, *sampleArg=NULL, *outputArg=NULL;
    int pointsizeArg=INT_MIN;
    PyObject *pointsizeTuple=NULL;
    PyObject *arg;
    int32_t *pointsizes=NULL;
    char *samplefile=NULL;
    unichar_t *sample=NULL;

    if ( CheckIfFontClosed(self) )
        return (NULL);

    /* Attempt parse assuming pointsize is a single integer */
    if ( !PyArg_ParseTuple(args,"s|iss",&typeArg,&pointsizeArg,&sampleArg,&outputArg) ) {
        PyErr_Clear();
        /* Attempt parse hoping pointsize is a tuple/list of integers */
        if ( !PyArg_ParseTuple(args,"s|Oss",&typeArg,&pointsizeTuple,&sampleArg,&outputArg) ) {
            PyErr_Format(PyExc_TypeError,"Expecting 1 to 4 args with type string as first arg");
            return( NULL );
        }
        if ( !PyTuple_Check(pointsizeTuple) && !PyList_Check(pointsizeTuple)) {
            PyErr_Format(PyExc_TypeError, "Second arg must be an integer, or a tuple or list of integers" );
            return( NULL );
        }
        if ( PySequence_Size(pointsizeTuple) < 1 ) {
            PyErr_Format(PyExc_TypeError, "Second arg must be an integer, or a tuple or list of integers" );
            return( NULL );
        }
    }

    type = FlagsFromString(typeArg,printflags,"print sample type");
    if ( type==FLAG_UNKNOWN ) {
        return( NULL );
    }
    if ( type==4 ) {
        type=3;
        inlinesample = false;
    }
    if ( pointsizeArg!=INT_MIN ) {
        if ( pointsizeArg>0 ) {
            pointsizes = calloc(2,sizeof(int32_t));
            pointsizes[0] = pointsizeArg;
        }
    } else if ( pointsizeTuple!=NULL ) {
        int subcnt = PySequence_Size(pointsizeTuple);
        pointsizes = malloc((subcnt+1)*sizeof(int32_t));
        for ( i=0; i<subcnt; ++i ) {
            arg = PySequence_GetItem(pointsizeTuple,i);
            pointsizes[i] = PyLong_AsLong(arg);
            Py_DECREF(arg);
            if ( PyErr_Occurred()) {
                free(pointsizes);
                return( NULL );
            }
        }
        pointsizes[i] = 0;
    }
    if ( sampleArg!=NULL ) {
        if ( inlinesample ) {
            sample = utf82u_copy(sampleArg);
            samplefile = NULL;
        } else {
            samplefile = utf82def_copy(sampleArg);
            sample = NULL;
        }
    }

    ScriptPrint(self->fv,type,pointsizes,samplefile,sample,outputArg);
    free(pointsizes);
    free(samplefile);
    /* ScriptPrint frees sample for us */
    Py_RETURN(self);
}

static PyObject *PyFFFont_randomText(PyFF_Font *self, PyObject *args) {
    FontViewBase *fv;
    char *script=NULL, *lang=NULL, *txt;
    uint32_t stag, ltag=0;
    PyObject *ret;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    if ( !PyArg_ParseTuple(args,"s|s",&script, &lang ))
return( NULL );
    stag = StrToTag(script,NULL);
    if ( lang!=NULL ) {
	ltag = StrToTag(lang,NULL);
	txt = RandomParaFromScriptLang(stag,ltag,fv->sf,NULL);
    } else
	txt = RandomParaFromScript(stag,&ltag,fv->sf);
    ret = Py_BuildValue("s",txt);
    free(txt);
return( ret );
}

static PyObject *PyFFFont_clear(PyFF_Font *self, PyObject *UNUSED(args)) {
    FontViewBase *fv;
    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    FVClear(fv);
Py_RETURN(self);
}

static PyObject *PyFFFont_cut(PyFF_Font *self, PyObject *UNUSED(args)) {
    FontViewBase *fv;
    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    FVCopy(fv,ct_fullcopy);
    FVClear(fv);
Py_RETURN(self);
}

static PyObject *PyFFFont_copy(PyFF_Font *self, PyObject *UNUSED(args)) {
    FontViewBase *fv;
    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    FVCopy(fv,ct_fullcopy);
Py_RETURN(self);
}

static PyObject *PyFFFont_copyReference(PyFF_Font *self, PyObject *UNUSED(args)) {
    FontViewBase *fv;
    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    FVCopy(fv,ct_reference);
Py_RETURN(self);
}

static PyObject *PyFFFont_paste(PyFF_Font *self, PyObject *UNUSED(args)) {
    FontViewBase *fv;
    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    PasteIntoFV(fv,false,NULL);
Py_RETURN(self);
}

static PyObject *PyFFFont_pasteInto(PyFF_Font *self, PyObject *UNUSED(args)) {
    FontViewBase *fv;
    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    PasteIntoFV(fv,true,NULL);
Py_RETURN(self);
}

static PyObject *PyFFFont_unlinkReferences(PyFF_Font *self, PyObject *UNUSED(args)) {
    FontViewBase *fv;
    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    FVUnlinkRef(fv);
Py_RETURN(self);
}


static PyObject *PyFFFont_Build(PyFF_Font *self, PyObject *UNUSED(args)) {
    FontViewBase *fv;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    FVBuildAccent(fv,false);

Py_RETURN( self );
}

static PyObject *PyFFFont_canonicalContours(PyFF_Font *self, PyObject *UNUSED(args)) {
    FontViewBase *fv;
    EncMap *map;
    SplineFont *sf;
    int i,gid;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    sf = fv->sf;
    map = fv->map;
    for ( i=0; i<map->enccount; ++i ) if ( (gid=map->map[i])!=-1 && sf->glyphs[gid]!=NULL && fv->selected[i] )
	CanonicalContours(sf->glyphs[gid],fv->active_layer);

Py_RETURN( self );
}

static PyObject *PyFFFont_canonicalStart(PyFF_Font *self, PyObject *UNUSED(args)) {
    FontViewBase *fv;
    EncMap *map;
    SplineFont *sf;
    int i,gid;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    sf = fv->sf;
    map = fv->map;
    for ( i=0; i<map->enccount; ++i ) if ( (gid=map->map[i])!=-1 && sf->glyphs[gid]!=NULL && fv->selected[i] )
	SPLsStartToLeftmost(sf->glyphs[gid],fv->active_layer);

Py_RETURN( self );
}

static PyObject *PyFFFont_changeWeight(PyFF_Font *self, PyObject *args) {
    FontViewBase *fv;
    enum embolden_type type;
    struct lcg_zones zones;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    type = CW_ParseArgs(fv->sf,&zones,args);
    if ( type == embolden_error )
return( NULL );
    FVEmbolden(fv,type,&zones);

Py_RETURN( self );
}

static PyObject *PyFFFont_condenseExtend(PyFF_Font *self, PyObject *args) {
    FontViewBase *fv;
    struct counterinfo ci;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    memset(&ci,0,sizeof(ci));
    ci.sb_factor = ci.sb_add = -10000;
    ci.correct_italic = true;

    if ( !PyArg_ParseTuple(args,"dd|ddi",&ci.c_factor, &ci.c_add, &ci.sb_factor, &ci.sb_add,
	    &ci.correct_italic ))
return( NULL );
    ci.c_factor *= 100;			/* UI uses a percent */
    if ( ci.sb_factor == -10000 )
	ci.sb_factor = ci.c_factor;
    if ( ci.sb_add == -10000 )
	ci.sb_add = ci.c_add;
    CI_Init(&ci,fv->sf);
    FVCondenseExtend(fv,&ci);

Py_RETURN( self );
}

static const char *italicize_keywords[] = {
    "italic_angle", "ia",
    "lc_condense", "lc",
    "uc_condense", "uc",
    "symbol_condense", "symbol",
    "deserif_flat", "deserif_slant", "deserif_pen",
    "baseline_serifs",
    "xheight_serifs",
    "ascent_serifs",
    "descent_serifs",
    "diagonal_serifs",
    "a",
    "f",
    "u0444",
    "u0438",
    "u043f",
    "u0442",
    "u0448",
    "u0452",
    "u045f",
    NULL
};

static ItalicInfo default_ii = {
    -13,                    /* Italic angle (in degrees) */
    .95,                    /* xheight percent */

    /* horizontal squash, lsb, stemsize, countersize, rsb */
    { .91, .89, .90, .91 }, /* For lower case */
    { .91, .93, .93, .91 }, /* For upper case */
    { .91, .93, .93, .91 }, /* For things which are neither upper nor lower case */
    srf_flat,               /* Secondary serifs (initial, medial on "m", descender on "p", "q" */
    true,                   /* Transform bottom serifs */
    true,                   /* Transform serifs at x-height */
    false,                  /* Transform serifs on ascenders */
    true,                   /* Transform serifs on diagonal stems at baseline and x-height */

    true,                   /* Change the shape of an "a" to look like a "d" without ascender */
    false,                  /* Change the shape of "f" so it descends below baseline (straight down no flag at end) */
    true,                   /* Change the shape of "f" so the bottom looks like the top */
    true,                   /* Remove serifs from the bottom of descenders */

    true,                   /* Make the cyrillic "phi" glyph have a top like an "f" */
    true,                   /* Make the cyrillic "i" glyph look like a latin "u" */
    true,                   /* Make the cyrillic "pi" glyph look like a latin "n" */
    true,                   /* Make the cyrillic "te" glyph look like a latin "m" */
    true,                   /* Make the cyrillic "sha" glyph look like a latin "m" rotated 180 */
    true,                   /* Make the cyrillic "dje" glyph look like a latin smallcaps T (not implemented) */
    true,                   /* Make the cyrillic "dzhe" glyph look like a latin "u" (same glyph used for cyrillic "i") */

    ITALICINFO_REMAINDER
};

static int SquashParse(struct hsquash *squash,PyObject *po) {
    if ( po==NULL )
return( true );

    if ( PyFloat_Check(po)) {
	squash->lsb_percent = squash->stem_percent = squash->counter_percent = squash->rsb_percent =
		PyFloat_AsDouble(po);
    } else if ( !PyArg_ParseTuple(po,"dddd",
	    squash->lsb_percent, squash->stem_percent,
	    squash->counter_percent, squash->rsb_percent ))
return( false );

return( true );
}

static int Parse_ItalicArgs(ItalicInfo *ii, PyObject *args, PyObject *keywds) {
    PyObject *lc=NULL, *uc=NULL, *symbols=NULL;
    int deserif_flat=false, deserif_slant=false, deserif_pen=false;
    int bottom_serif=default_ii.transform_bottom_serifs;
    int xh_serif = default_ii.transform_top_xh_serifs;
    int as_serif = default_ii.transform_top_as_serifs;
    int dg_serif = default_ii.transform_diagon_serifs;
    int ds_serif = default_ii.pq_deserif;
    int a = default_ii.a_from_d;
    int f = default_ii.f_long_tail ? 2 : default_ii.f_rotate_top ? 1 : 0;
    int u0444 = default_ii.cyrl_phi;
    int u0438 = default_ii.cyrl_i;
    int u043f = default_ii.cyrl_pi;
    int u0442 = default_ii.cyrl_te;
    int u0448 = default_ii.cyrl_sha;
    int u0452 = default_ii.cyrl_dje;
    int u045f = default_ii.cyrl_dzhe;

    *ii = default_ii;
    if ( !PyArg_ParseTupleAndKeywords(args,keywds,"|ddOOOOOOiiiiiiiiiiiiiiiii",(char **)italicize_keywords,
	    &ii->italic_angle, &ii->italic_angle,
	    &lc, &lc,
	    &uc, &uc,
	    &symbols, &symbols,
	    &deserif_flat, &deserif_slant, &deserif_pen,
	    &bottom_serif, &xh_serif, &as_serif, &dg_serif, &ds_serif,
	    &a, &f,
	    &u0444, &u0438, &u043f, &u0442, &u0448, &u0452, &u045f ))
return( false );
    if ( !SquashParse(&ii->lc,lc) || !SquashParse(&ii->uc,uc) || !SquashParse(&ii->neither,symbols))
return( false );
    if ( deserif_flat )
	ii->secondary_serif = srf_flat;
    else if ( deserif_slant )
	ii->secondary_serif = srf_simpleslant;
    else if ( deserif_pen )
	ii->secondary_serif = srf_complexslant;
    ii->transform_bottom_serifs = bottom_serif;
    ii->transform_top_xh_serifs = xh_serif;
    ii->transform_top_as_serifs = as_serif;
    ii->transform_diagon_serifs = dg_serif;
    ii->pq_deserif = ds_serif;

    ii->f_long_tail = ii->f_rotate_top = false;
    if ( f==2 )
	ii->f_long_tail = true;
    else if ( f==1 )
	ii->f_rotate_top = true;
    ii->a_from_d = a;

    ii->cyrl_phi  = u0444;
    ii->cyrl_i    = u0438;
    ii->cyrl_pi   = u043f;
    ii->cyrl_te   = u0442;
    ii->cyrl_sha  = u0448;
    ii->cyrl_dje  = u0452;
    ii->cyrl_dzhe = u045f;
return( true );
}

static PyObject *PyFFFont_italicize(PyFF_Font *self, PyObject *args, PyObject *keywds) {
    FontViewBase *fv;
    ItalicInfo ii;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    if ( !Parse_ItalicArgs(&ii,args,keywds))
return( NULL );
    MakeItalic(fv,NULL,&ii);

Py_RETURN( self );
}

static const char *smallcaps_keywords[] = { "scheight", "capheight", "lcstem", "ucstem",
	"symbols", "letter_extension", "symbol_extension", "stem_height_factor",
	"hscale", "vscale", NULL };

static PyObject *PyFFFont_addSmallCaps(PyFF_Font *self, PyObject *args, PyObject *keywds) {
    FontViewBase *fv;
    struct smallcaps small;
    struct genericchange genchange;
    double lc_width=0, uc_width=0, vstem_factor=0, hscale=0, vscale=0, scheight=0, capheight=0;
    int dosymbols=0;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    memset(&genchange,0,sizeof(genchange));
    SmallCapsFindConstants(&small,fv->sf,fv->active_layer);
    genchange.small = &small;
    genchange.gc = gc_smallcaps;
    genchange.extension_for_letters = "sc";
    genchange.extension_for_symbols = "taboldstyle";
    if ( !PyArg_ParseTupleAndKeywords(args,keywds,"|ddddissddd",(char **)smallcaps_keywords,
	    &scheight,&capheight,&lc_width,&uc_width,
	    &dosymbols, &genchange.extension_for_letters,
	    &genchange.extension_for_symbols,
	    &vstem_factor,
	    &hscale, &vscale))
return( NULL );
    if ( lc_width!=0 || uc_width!=0 ) {
	if ( lc_width!=0 )
	    small.lc_stem_width = lc_width;
	if ( uc_width!=0 )
	    small.uc_stem_width = uc_width;
	genchange.stem_width_scale = genchange.stem_height_scale =
		small.lc_stem_width / small.uc_stem_width;
    }
    genchange.do_smallcap_symbols = dosymbols;
    if ( vstem_factor!=0 )
	genchange.stem_height_scale = vstem_factor;
    if ( scheight>0 || capheight>0 ) {
	if ( scheight>0 )
	    small.scheight = scheight;
	if ( capheight>0 )
	    small.capheight = capheight;
    }
    if ( small.capheight>0 )
	genchange.v_scale = genchange.hcounter_scale = small.scheight/small.capheight;
    if ( hscale>0 )
	genchange.hcounter_scale = hscale;
    genchange.lsb_scale = genchange.rsb_scale = genchange.hcounter_scale;
    if ( vscale>0 )
	genchange.v_scale = vscale;

    FVAddSmallCaps(fv,&genchange);

Py_RETURN( self );
}

static const char *genchange_keywords[] = {
/* Stem info */
	"stemType",
	"thickThreshold",
	"stemScale", "stemAdd",
	"stemHeightScale", "thinStemScale",
	"stemHeightAdd", "thinStemAdd",
	"stemWidthScale", "thickStemScale",
	"stemWidthAdd", "thickStemAdd",
	"processDiagonalStems",
/* Horizontal counter info */
	"hCounterType",
	"hCounterScale", "hCounterAdd",
	"lsbScale", "lsbAdd",
	"rsbScale", "rsbAdd",
/* Vertical counter info */
	"vCounterType",
	"vCounterScale", "vCounterAdd",
	"vScale",
	"vMap",
	NULL};

static bool PyFFParse_genericGlyphChange(PyObject *args, PyObject *keywds,
                                         struct genericchange *genchange) {
    const char *stemtype = "uniform", *hcountertype="uniform";
    const char *vCounterType="mapped";
    double thickthreshold=0,
	stemscale=0, stemadd=0,
	stemheightscale=0, thinstemscale=0,
	stemheightadd=0, thinstemadd=0,
	stemwidthscale=0, thickstemscale=0,
	stemwidthadd=0, thickstemadd=0;
    int processdiagonalstems=1;
    double counterScale=0, counterAdd=0,
	lsbScale=0, lsbAdd=0,
	rsbScale=0, rsbAdd=0;
    double vCounterScale=0, vCounterAdd=0,
	vScale=1.0;
    PyObject *vMap=NULL;

    memset(genchange,0,sizeof(struct genericchange));

    genchange->gc = gc_generic;

    if ( !PyArg_ParseTupleAndKeywords(args,keywds,"|sdddddddddddisddddddsdddO",(char **)genchange_keywords,
/* Stem info */
	    &stemtype,
	    &thickthreshold,
	    &stemscale, &stemadd,
	    &stemheightscale,&thinstemscale,
	    &stemheightadd, &thinstemadd,
	    &stemwidthscale, &thickstemscale,
	    &stemwidthadd, &thickstemadd,
	    &processdiagonalstems,
/* Horizontal counter info */
	    &hcountertype,
	    &counterScale, &counterAdd,
	    &lsbScale, &lsbAdd,
	    &rsbScale, &rsbAdd,
/* Vertical counter info */
	    &vCounterType,
	    &vCounterScale, &vCounterAdd,
	    &vScale,
	    &vMap
	    ) )
	return false;

/* Stem info */
    if ( strcasecmp(stemtype,"uniform")==0 ) {
	if ( stemscale<=0 ) {
	    PyErr_Format(PyExc_TypeError, "Unexpected (or unspecified) value for stemScale: %g.", stemscale );
	    return false;
	}
	genchange->stem_width_scale = genchange->stem_height_scale = stemscale;
	genchange->stem_width_add   = genchange->stem_height_add = stemadd;
    } else if ( strcasecmp( stemtype, "thickThin" )==0 ) {
	if ( thinstemscale<=0 ) {
	    PyErr_Format(PyExc_TypeError, "Unexpected (or unspecified) value for thinStemScale: %g.", thinstemscale );
	    return false;
	}
	if ( thickstemscale<=0 ) {
	    PyErr_Format(PyExc_TypeError, "Unexpected (or unspecified) value for thickStemScale: %g.", thickstemscale );
	    return false;
	}
	if ( thickthreshold<=0 ) {
	    PyErr_Format(PyExc_TypeError, "Unexpected (or unspecified) value for thickThreshold: %g.", thickthreshold );
	    return false;
	}
	genchange->stem_width_scale  = thickstemscale;
	genchange->stem_width_add    = thickstemadd;
	genchange->stem_height_scale = thinstemscale;
	genchange->stem_height_add   = thinstemadd;
	genchange->stem_threshold    = thickthreshold;
    } else if ( strcasecmp( stemtype, "horizontalVertical" )==0 ) {
	if ( stemheightscale<=0 ) {
	    PyErr_Format(PyExc_TypeError, "Unexpected (or unspecified) value for stemHeightScale: %g.", stemheightscale );
	    return false;
	}
	if ( stemwidthscale<=0 ) {
	    PyErr_Format(PyExc_TypeError, "Unexpected (or unspecified) value for stemWidthScale: %g.", stemwidthscale );
	    return false;
	}
	genchange->stem_width_scale  = stemwidthscale;
	genchange->stem_width_add    = stemwidthadd;
	genchange->stem_height_scale = stemheightscale;
	genchange->stem_height_add   = stemheightadd;
    } else {
	PyErr_Format(PyExc_TypeError, "Unexpected value for stemType: %s\n (Try: 'uniform', 'thickThin', or 'horizontalVertical')", stemtype );
	return false;
    }
    genchange->dstem_control         = processdiagonalstems;
    if ( genchange->stem_height_add!=genchange->stem_width_add ) {
	if (( genchange->stem_height_add==0 && genchange->stem_width_add!=0 ) ||
		( genchange->stem_height_add!=0 && genchange->stem_width_add==0 )) {
	    PyErr_SetString(PyExc_TypeError, _("The horizontal and vertical stem add amounts must either both be zero, or neither may be 0"));
	    return false;
	}
	/* if width_add has a different sign than height_add that's also */
	/*  a problem, but this test will catch that too */
	if (( genchange->stem_height_add/genchange->stem_width_add>4 ) ||
		( genchange->stem_height_add/genchange->stem_width_add<.25 )) {
	     PyErr_SetString(PyExc_TypeError, _("The horizontal and vertical stem add amounts may not differ by more than a factor of 4"));
	     return false;
	}
    }

    /* Horizontal counter info */
    if ( strcasecmp(hcountertype,"uniform")==0 ) {
	if ( counterScale<=0 ) {
	    PyErr_Format(PyExc_TypeError, "Unexpected (or unspecified) value for hCounterScale: %g.", counterScale );
	    return false;
	}
	genchange->hcounter_scale = genchange->lsb_scale = genchange->rsb_scale = counterScale;
	genchange->hcounter_add   = genchange->lsb_add   = genchange->rsb_add   = counterAdd;
    } else if ( strcasecmp(hcountertype,"nonUniform")==0 ) {
	if ( counterScale<=0 ) {
	    PyErr_Format(PyExc_TypeError, "Unexpected (or unspecified) value for hCounterScale: %g.", counterScale );
	    return false;
	}
	if ( lsbScale<=0 ) {
	    PyErr_Format(PyExc_TypeError, "Unexpected (or unspecified) value for lsbScale: %g.", lsbScale );
	    return false;
	}
	if ( rsbScale<=0 ) {
	    PyErr_Format(PyExc_TypeError, "Unexpected (or unspecified) value for rsbScale: %g.", rsbScale );
	    return false;
	}
	genchange->hcounter_scale = counterScale;
	genchange->lsb_scale      = lsbScale;
	genchange->rsb_scale      = rsbScale;
	genchange->hcounter_add   = counterAdd;
	genchange->lsb_add        = lsbAdd;
	genchange->rsb_add        = rsbAdd;
    } else if ( strcasecmp(hcountertype,"center")==0 ) {
	genchange->center_in_hor_advance = 1;
    } else if ( strcasecmp(hcountertype,"retainScale")==0 ) {
	genchange->center_in_hor_advance = 2;
    } else {
	PyErr_Format(PyExc_TypeError, "Unexpected value for hCounterType: %s\n (Try: 'uniform', 'nonUniform', 'retain', or 'scale')", hcountertype );
	return false;
    }

    /* Vertical counter info */
    if ( strcasecmp(vCounterType,"scaled")==0 ) {
	if ( vCounterScale<=0 ) {
	    PyErr_Format(PyExc_TypeError, "Unexpected (or unspecified) value for vCounterScale: %g.", vCounterScale );
	    return false;
	}
	genchange->vcounter_scale = vCounterScale;
	genchange->vcounter_add   = vCounterAdd;
	genchange->use_vert_mapping = false;
    } else if ( strcasecmp(vCounterType,"mapped")==0 ) {
	int cnt,i;
	genchange->use_vert_mapping = true;
	if ( vScale<=0 ) {
	    PyErr_Format(PyExc_TypeError, "Unexpected (or unspecified) value for vScale: %g.", vScale );
	    return false;
	}
	genchange->v_scale = vScale;
	if ( vMap==NULL || !PySequence_Check(vMap) || PyUnicode_Check(vMap)) {
	    PyErr_Format(PyExc_TypeError, "vMap should be a tuple (or some other sequence type)." );
	    return false;
	}
	cnt = PySequence_Size(vMap);
	genchange->m.cnt = cnt;
	genchange->m.maps = malloc(cnt*sizeof(struct position_maps));
	for ( i=0; i<cnt; ++i ) {
	    PyObject *subTuple = PySequence_GetItem(vMap,i);
	    if ( subTuple==NULL || !PySequence_Check(subTuple) || PyUnicode_Check(subTuple) || PySequence_Size(subTuple)!=3 ) {
		PyErr_Format(PyExc_TypeError, "vMap should be a tuple of 3-tuples." );
		free(genchange->m.maps);
		return false;
	    }
	    if ( !PyArg_ParseTuple(subTuple,"ddd",
		    &genchange->m.maps[i].current,
		    &genchange->m.maps[i].desired,
		    &genchange->m.maps[i].cur_width) ) {
		free(genchange->m.maps);
		return false;
	    }
	}
    } else {
	PyErr_Format(PyExc_TypeError, "Unexpected value for vCounterType: %s\n (Try: 'scaled', or 'mapped')", vCounterType );
	return false;
    }
    return true;
}

static PyObject *PyFFFont_genericGlyphChange(PyFF_Font *self, PyObject *args,
                                             PyObject *keywds) {
    FontViewBase *fv;
    struct genericchange genchange;
    struct smallcaps small;

    if ( CheckIfFontClosed(self) )
	return NULL;

    fv = self->fv;

    if ( !PyFFParse_genericGlyphChange(args, keywds, &genchange) )
	return NULL;

    SmallCapsFindConstants(&small,fv->sf,fv->active_layer);
    genchange.small = &small;

    FVGenericChange( fv, &genchange );
    free(genchange.m.maps);

    Py_RETURN( self );
}

static PyObject *PyFFFont_autoHint(PyFF_Font *self, PyObject *UNUSED(args)) {
    FontViewBase *fv;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    FVAutoHint(fv);
Py_RETURN( self );
}

static PyObject *PyFFFont_autoInstr(PyFF_Font *self, PyObject *UNUSED(args)) {
    FontViewBase *fv;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    FVAutoInstr(fv);
Py_RETURN( self );
}

static const char *autowidth_keywords[] = { "separation", "minBearing", "maxBearing",
	"height", "loopCnt", NULL };

static PyObject *PyFFFont_autoWidth(PyFF_Font *self, PyObject *args, PyObject *keywds) {
    FontViewBase *fv;
    int space, min=10, max=-1, height=0, loop=1;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    if ( !PyArg_ParseTupleAndKeywords(args,keywds,"i|iiii",(char **)autowidth_keywords,
	    &space,&min,&max,&height,&loop))
return( NULL );
    AutoWidth2(fv,space,min,max, height, loop);
Py_RETURN( self );
}

static PyObject *PyFFFont_autoTrace(PyFF_Font *self, PyObject *UNUSED(args)) {
    FontViewBase *fv;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    FVAutoTrace(fv,false);
Py_RETURN( self );
}

/* Transformation flags: see 'enum fvtrans_flags' in baseviews.h */
static struct flaglist f_trans_flags[] = {
    { "activeLayer", fvt_alllayers },
    { "guide", fvt_dogrid },
    { "noWidth", fvt_dontmovewidth },
    { "round", fvt_round_to_int },
    { "simplePos", fvt_scalepstpos },
    { "kernClasses", fvt_scalekernclasses },
    FLAGLIST_EMPTY /* Sentinel */
};

static PyObject *PyFFFont_Transform(PyFF_Font *self, PyObject *args) {
    FontViewBase *fv;
    int i;
    double m[6];
    real t[6];
    BVTFunc bvts[1];
    int flags;
    PyObject *flagO=NULL;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    if ( !PyArg_ParseTuple(args,"(dddddd)|O",&m[0], &m[1], &m[2], &m[3], &m[4], &m[5], &flagO ) )
return( NULL );
    flags = FlagsFromTuple(flagO,f_trans_flags,"transformation flag");
    if ( flags==FLAG_UNKNOWN )
return( NULL );
    // Flip fvt_alllayers back
    if ( flags&fvt_alllayers )
	flags &= ~fvt_alllayers;
    else
	flags |= fvt_alllayers;
    for ( i=0; i<6; ++i )
	t[i] = m[i];
    bvts[0].func = bvt_none;
    FVTransFunc(fv,t,0,bvts,flags);
Py_RETURN( self );
}

static PyObject *PyFFFont_NLTransform(PyFF_Font *self, PyObject *args) {
    FontViewBase *fv;
    char *xexpr, *yexpr;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    if ( !PyArg_ParseTuple(args,"ss", &xexpr, &yexpr ) )
return( NULL );
    if ( !SFNLTrans(fv,xexpr,yexpr) ) {
	PyErr_Format(PyExc_TypeError, "Unparseable expression.");
return( NULL );
    }
Py_RETURN( self );
}

static PyObject *PyFFFont_Simplify(PyFF_Font *self, PyObject *args) {
    static struct simplifyinfo smpl = { sf_normal, 0.75, 0.2, 10, 0, 0, 0 };
    FontViewBase *fv;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    smpl.err = (fv->sf->ascent+fv->sf->descent)/1000.;
    smpl.linefixup = (fv->sf->ascent+fv->sf->descent)/500.;
    smpl.linelenmax = (fv->sf->ascent+fv->sf->descent)/100.;

    if ( PySequence_Size(args)>=1 )
	smpl.err = PyFloat_AsDouble(PySequence_GetItem(args,0));
    if ( !PyErr_Occurred() && PySequence_Size(args)>=2 )
	smpl.flags = FlagsFromTuple( PySequence_GetItem(args,1),simplifyflags,"simplify flag");
    if ( !PyErr_Occurred() && PySequence_Size(args)>=3 )
	smpl.tan_bounds = PyFloat_AsDouble( PySequence_GetItem(args,2));
    if ( !PyErr_Occurred() && PySequence_Size(args)>=4 )
	smpl.linefixup = PyFloat_AsDouble( PySequence_GetItem(args,3));
    if ( !PyErr_Occurred() && PySequence_Size(args)>=5 )
	smpl.linelenmax = PyFloat_AsDouble( PySequence_GetItem(args,4));
    if ( PyErr_Occurred() )
return( NULL );
    _FVSimplify(self->fv,&smpl);
Py_RETURN( self );
}

static PyObject *PyFFFont_Round(PyFF_Font *self, PyObject *args) {
    double factor=1;
    FontViewBase *fv;
    SplineFont *sf;
    EncMap *map;
    int i, gid;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    sf = fv->sf;
    map = fv->map;
    if ( !PyArg_ParseTuple(args,"|d",&factor ) )
return( NULL );
    for ( i=0; i<map->enccount; ++i ) if ( (gid=map->map[i])!=-1 && sf->glyphs[gid]!=NULL && fv->selected[i] ) {
	SplineChar *sc = sf->glyphs[gid];
	SCRound2Int( sc,fv->active_layer,factor);
    }
Py_RETURN( self );
}

static PyObject *PyFFFont_Cluster(PyFF_Font *self, PyObject *args) {
    double within = .1, max = .5;
    int i, gid;
    FontViewBase *fv;
    SplineFont *sf;
    EncMap *map;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    sf = fv->sf;
    map = fv->map;
    if ( !PyArg_ParseTuple(args,"|dd", &within, &max ) )
return( NULL );

    for ( i=0; i<map->enccount; ++i ) if ( (gid=map->map[i])!=-1 && sf->glyphs[gid]!=NULL && fv->selected[i] ) {
	SplineChar *sc = sf->glyphs[gid];
	SCRoundToCluster( sc,ly_all,false,within,max);
    }
Py_RETURN( self );
}

static PyObject *PyFFFont_AddExtrema(PyFF_Font *self, PyObject *UNUSED(args)) {
    FontViewBase *fv;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    FVAddExtrema(fv, false);
Py_RETURN( self );
}

static PyObject *PyFFFont_AddInflections(PyFF_Font *self) {
    FontViewBase *fv;
    
    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    FVAddInflections(fv, false);
Py_RETURN( self );
}

static PyObject *PyFFFont_Stroke(PyFF_Font *self, PyObject *args, PyObject *keywds) {
    StrokeInfo si;

    if ( CheckIfFontClosed(self) )
	return (NULL);
    if ( Stroke_Parse(&si, args, keywds)==-1 )
	return( NULL );

    FVStrokeItScript(self->fv, &si,false);
    SplinePointListsFree(si.nib); si.nib = NULL;
Py_RETURN( self );
}

static PyObject *PyFFFont_correctDirection(PyFF_Font *self, PyObject *UNUSED(args)) {
    int i, gid;
    FontViewBase *fv;
    SplineFont *sf;
    EncMap *map;
    int changed, refchanged;
    int checkrefs = true;
    RefChar *ref;
    SplineChar *sc;
    RefChar *next;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    sf = fv->sf;
    map = fv->map;
    for ( i=0; i<map->enccount; ++i ) if ( (gid=map->map[i])!=-1 && (sc=sf->glyphs[gid])!=NULL && fv->selected[i] ) {
	changed = refchanged = false;
	if ( checkrefs ) {
	    for ( ref=sc->layers[self->fv->active_layer].refs; ref!=NULL; ) {
		if ( ref->transform[0]*ref->transform[3]<0 ||
			(ref->transform[0]==0 && ref->transform[1]*ref->transform[2]>0)) {
		    if ( !refchanged ) {
			refchanged = true;
			SCPreserveLayer(sc,self->fv->active_layer,false);
		    }
		    next = ref->next;
		    SCRefToSplines(sc,ref,self->fv->active_layer);
		    ref = next;
		} else {
		    ref=ref->next;
		}
	    }
	}
	if ( !refchanged )
	    SCPreserveLayer(sc,self->fv->active_layer,false);
	sc->layers[self->fv->active_layer].splines = SplineSetsCorrect(sc->layers[self->fv->active_layer].splines,&changed);
	if ( changed || refchanged )
	    SCCharChangedUpdate(sc,self->fv->active_layer);
    }
Py_RETURN( self );
}

static PyObject *PyFFFont_RemoveOverlap(PyFF_Font *self, PyObject *UNUSED(args)) {
    if ( CheckIfFontClosed(self) )
return (NULL);
    FVOverlap(self->fv,over_remove);
Py_RETURN( self );
}

static PyObject *PyFFFont_Intersect(PyFF_Font *self, PyObject *UNUSED(args)) {
    if ( CheckIfFontClosed(self) )
return (NULL);
    FVOverlap(self->fv,over_intersect);
Py_RETURN( self );
}

static PyObject *PyFFFont_replaceWithReference(PyFF_Font *self, PyObject *args) {
    double fudge = .01;

    if ( CheckIfFontClosed(self) )
return (NULL);
    if ( !PyArg_ParseTuple(args,"|d",&fudge) )
return( NULL );

    FVBReplaceOutlineWithReference(self->fv,fudge);
Py_RETURN( self );
}

static PyObject *PyFFFont_correctReferences(PyFF_Font *self, PyObject *UNUSED(args)) {

    FVCorrectReferences(self->fv);
Py_RETURN( self );
}

static PyObject *PyFFFont_validate(PyFF_Font *self, PyObject *args) {
    FontViewBase *fv;
    SplineFont *sf;
    int force=false;

    if ( CheckIfFontClosed(self) )
return (NULL);
    fv = self->fv;
    sf = fv->sf;
    if ( !PyArg_ParseTuple(args,"|i",&force) )
return( NULL );
return( Py_BuildValue("i", SFValidate(sf,fv->active_layer,force)));
}

static PyObject *PyFFFont_reencode(PyFF_Font *self, PyObject *args) {
    int force=0;
    const char *encname;
    int ret;

    if ( CheckIfFontClosed(self) )
return ( NULL );
    if ( !PyArg_ParseTuple(args, "s|i", &encname, &force) )
return ( NULL );

    ret = SFReencode(self->fv->sf, encname, force);
    if ( ret==-1 ) {
	PyErr_Format(PyExc_NameError, "Unknown encoding %s", encname);
return ( NULL );
    }

Py_RETURN( self );
}

static PyObject *PyFFFont_clearSpecialData(PyFF_Font *self, PyObject *UNUSED(args)) {
    if ( CheckIfFontClosed(self) )
return (NULL);
    FVClearSpecialData(self->fv);
Py_RETURN( self );
}

PyMethodDef PyFF_Font_methods[] = {
    { "appendSFNTName", (PyCFunction) PyFFFont_appendSFNTName, METH_VARARGS, "Adds or replaces a name in the sfnt 'name' table. Takes three arguments, a language, a string id, and the string value" },
    { "close", (PyCFunction) PyFFFont_close, METH_NOARGS, "Frees up memory for the current font. Any python pointers to it will become invalid." },
    { "compareFonts", (PyCFunction) PyFFFont_compareFonts, METH_VARARGS, "Compares two fonts and stores the result into a file"},
    { "save", (PyCFunction) PyFFFont_Save, METH_VARARGS, "Save the current font to a sfd file" },
    { "generate", (PyCFunction) PyFFFont_Generate, METH_VARARGS | METH_KEYWORDS, "Save the current font to a standard font file" },
    { "generateTtc", (PyCFunction) PyFFFont_GenerateTTC, METH_VARARGS | METH_KEYWORDS, "Save the current font and some others into a truetype collection file" },
    { "generateFeatureFile", (PyCFunction) PyFFFont_GenerateFeature, METH_VARARGS, "Creates an adobe feature file containing all features and lookups" },
    { "mergeKern", (PyCFunction) PyFFFont_MergeKern, METH_VARARGS, "Merge feature data into the current font from an external file" },
    { "mergeFeature", (PyCFunction) PyFFFont_MergeKern, METH_VARARGS, "Merge feature data into the current font from an external file" },
    { "mergeFonts", (PyCFunction) PyFFFont_MergeFonts, METH_VARARGS, "Merge two fonts" },
    { "revert", (PyCFunction) PyFFFont_revert, METH_NOARGS, "Reloads the current font from the disk" },
    { "revertFromBackup", (PyCFunction) PyFFFont_revertFromBackup, METH_NOARGS, "Reloads the current font from a backup copy on the disk" },
    { "interpolateFonts", (PyCFunction) PyFFFont_InterpolateFonts, METH_VARARGS, "Interpolate between two fonts returning a new one" },
    { "createChar", (PyCFunction) PyFFFont_CreateUnicodeChar, METH_VARARGS, "Creates a (blank) glyph at the specified unicode codepoint" },
    { "createInterpolatedGlyph", (PyCFunction) PyFFFont_CreateInterpolatedGlyph, METH_VARARGS, "Creates (and returns) a new glyph interpolated between the two arguments." },
    { "createMappedChar", (PyCFunction) PyFFFont_CreateMappedChar, METH_VARARGS, "Creates a (blank) glyph at the specified encoding" },
    { "cidConvertTo", (PyCFunction) PyFFFont_cidConvertTo, METH_VARARGS, "Turns a normal font into a CID keyed font with one subfont." },
    { "cidConvertByCmap", (PyCFunction) PyFFFont_cidConvertByCmap, METH_VARARGS, "Turns a normal font into a CID keyed font with one subfont, using the CMAP file to specify the CID ordering." },
    { "cidFlatten", (PyCFunction) PyFFFont_cidFlatten, METH_NOARGS, "Turns a cid-keyed font into a normal font, using the CID ordering as an encoding" },
    { "cidFlattenByCMap", (PyCFunction) PyFFFont_cidFlattenByCMap, METH_VARARGS, "Turns a cid-keyed font into a normal font, using the mapping specified in the CMAP file." },
    { "cidInsertBlankSubFont", (PyCFunction) PyFFFont_cidInsertBlankSubFont, METH_NOARGS, "Adds a new subfont, a blank one, to the CID keyed font, and changes the current subfont to be the new one." },
    { "cidRemoveSubFont", (PyCFunction) PyFFFont_cidRemoveSubFont, METH_NOARGS, "Removes the current subfont from a CID keyed font." },
    { "getTableData", (PyCFunction) PyFFFont_GetTableData, METH_VARARGS, "Returns a tuple, one entry per byte (as unsigned integers) of the table"},
    { "setTableData", (PyCFunction) PyFFFont_SetTableData, METH_VARARGS, "Sets the table to a tuple of bytes"},
    { "addLookup", (PyCFunction) PyFFFont_addLookup, METH_VARARGS, "Add a new lookup"},
    { "addLookupSubtable", (PyCFunction) PyFFFont_addLookupSubtable, METH_VARARGS, "Add a new lookup-subtable (not for contextual lookups)"},
    { "addContextualSubtable", (PyCFunction) PyFFFont_addContextualSubtable, METH_VARARGS | METH_KEYWORDS, "Create a subtable in a contextual, contextual chaining or reverse contextual chaining lookup." },
    { "addAnchorClass", (PyCFunction) PyFFFont_addAnchorClass, METH_VARARGS, "Add a new anchor class to the subtable"},
    { "addKerningClass", (PyCFunction) PyFFFont_addKerningClass, METH_VARARGS, "Add a new subtable with a new kerning class to a lookup"},
    { "alterKerningClass", (PyCFunction) PyFFFont_alterKerningClass, METH_VARARGS, "Changes the existing kerning class in the named subtable"},
    { "autoKern", (PyCFunction) PyFFFont_autoKern, METH_VARARGS | METH_KEYWORDS, "Automatically generates kerning pairs between the specified sets of glyphs."},
    { "buildOrReplaceAALTFeatures", (PyCFunction) PyFFFont_buildOrReplaceAALTFeatures, METH_NOARGS, "Removes any existing 'aalt' features and builds new ones."},
    { "findEncodingSlot", (PyCFunction) PyFFFont_findEncodingSlot, METH_VARARGS, "Returns the encoding of a unicode code point or glyph name if they are in the current encoding. Else returns -1" },
    { "getKerningClass", (PyCFunction) PyFFFont_getKerningClass, METH_VARARGS, "Returns the contents of the kerning class in the named subtable"},
    { "getLookupInfo", (PyCFunction) PyFFFont_getLookupInfo, METH_VARARGS, "Get info about the named lookup" },
    { "getLookupSubtables", (PyCFunction) PyFFFont_getLookupSubtables, METH_VARARGS, "Get a tuple of subtable names in a lookup" },
    { "getLookupSubtableAnchorClasses", (PyCFunction) PyFFFont_getLookupSubtableAnchorClasses, METH_VARARGS, "Get a tuple of all anchor classes in a subtable" },
    { "getLookupOfSubtable", (PyCFunction) PyFFFont_getLookupOfSubtable, METH_VARARGS, "Returns the name of the lookup containing this subtable" },
    { "getSubtableOfAnchor", (PyCFunction) PyFFFont_getSubtableOfAnchor, METH_VARARGS, "Returns the name of the lookup subtable containing this anchor class" },
    { "importBitmaps", (PyCFunction) PyFFFont_importBitmaps, METH_VARARGS, "Imports bitmap strikes from a font file."},
    { "importLookups", (PyCFunction) PyFFFont_importLookups, METH_VARARGS, "Imports a tuple of named lookups from another font."},
    { "isKerningClass", (PyCFunction) PyFFFont_isKerningClass, METH_VARARGS, "Returns whether the named subtable contains a kerning class"},
    { "isVerticalKerning", (PyCFunction) PyFFFont_isVerticalKerning, METH_VARARGS, "Returns whether the named subtable contains vertical kerning data"},
    { "lookupSetFeatureList", (PyCFunction) PyFFFont_lookupSetFeatureList, METH_VARARGS, "Sets the feature, script, language list on a lookup" },
    { "lookupSetFlags", (PyCFunction) PyFFFont_lookupSetFlags, METH_VARARGS, "Sets the lookup flags on a lookup" },
    { "lookupSetStoreLigatureInAfm", (PyCFunction) PyFFFont_lookupSetStoreLigatureInAfm, METH_VARARGS, "Sets whether this ligature lookup contains data which should live in the afm file"},
    { "mergeLookups", (PyCFunction) PyFFFont_mergeLookups, METH_VARARGS, "Merges two lookups" },
    { "mergeLookupSubtables", (PyCFunction) PyFFFont_mergeLookupSubtables, METH_VARARGS, "Merges two lookup subtables" },
    { "printSample", (PyCFunction) PyFFFont_printSample, METH_VARARGS, "Produces a font sample printout" },
    { "randomText", (PyCFunction) PyFFFont_randomText, METH_VARARGS, "Produces a string with random text generated from the font using letter frequencies for the specified script and language"},
    { "regenBitmaps", (PyCFunction) PyFFFont_regenBitmaps, METH_VARARGS, "Rerasterize the bitmap fonts specified in the argument tuple" },
    { "removeAnchorClass", (PyCFunction) PyFFFont_removeAnchorClass, METH_VARARGS, "Removes the named anchor class" },
    { "removeGlyph", (PyCFunction) PyFFFont_removeGlyph, METH_VARARGS, "Removes the glyph from the font" },
    { "removeLookup", (PyCFunction) PyFFFont_removeLookup, METH_VARARGS, "Removes the named lookup" },
    { "removeLookupSubtable", (PyCFunction) PyFFFont_removeLookupSubtable, METH_VARARGS, "Removes the named lookup subtable" },
    { "saveNamelist", (PyCFunction) PyFFFont_saveNamelist, METH_VARARGS, "Saves the namelist of the current font." },
    { "replaceAll", (PyCFunction) PyFFFont_replaceAll, METH_VARARGS, "Searches for a pattern in the font and replaces it with another everywhere it was found" },
    { "find", (PyCFunction) PyFFFont_find, METH_VARARGS, "Searches for a pattern in the font and returns an iterator which produces glyphs with that pattern" },
    { "glyphs", (PyCFunction) PyFFFont_glyphs, METH_VARARGS, "Returns an iterator over all glyphs" },
/* Selection based */
    { "clear", (PyCFunction) PyFFFont_clear, METH_NOARGS, "Clears all selected glyphs" },
    { "cut", (PyCFunction) PyFFFont_cut, METH_NOARGS, "Cuts all selected glyphs" },
    { "copy", (PyCFunction) PyFFFont_copy, METH_NOARGS, "Copies all selected glyphs" },
    { "copyReference", (PyCFunction) PyFFFont_copyReference, METH_NOARGS, "Copies all selected glyphs as references" },
    { "paste", (PyCFunction) PyFFFont_paste, METH_NOARGS, "Pastes the clipboard into the selected glyphs (clearing them first)" },
    { "pasteInto", (PyCFunction) PyFFFont_pasteInto, METH_NOARGS, "Pastes the clipboard into the selected glyphs (merging with what's there)" },
    { "unlinkReferences", (PyCFunction) PyFFFont_unlinkReferences, METH_NOARGS, "Unlinks all references in the selected glyphs" },
    { "replaceWithReference", (PyCFunction) PyFFFont_replaceWithReference, METH_VARARGS, "Replaces any inline copies of any of the selected glyphs with a reference" },
    { "correctReferences", (PyCFunction) PyFFFont_correctReferences, METH_NOARGS, "Replaces any inline copies of any of the selected glyphs with a reference" },

    { "addExtrema", (PyCFunction) PyFFFont_AddExtrema, METH_NOARGS, "Add extrema to the contours of the glyph"},
    { "addInflections", (PyCFunction) PyFFFont_AddInflections, METH_NOARGS, "Add points of inflection to the contours of the glyph"},
    { "addSmallCaps", (PyCFunction) PyFFFont_addSmallCaps, METH_VARARGS | METH_KEYWORDS, "For selected upper/lower case (latin, greek, cyrillic) characters, add a small caps variant of that glyph"},
    { "autoHint", (PyCFunction) PyFFFont_autoHint, METH_NOARGS, "Guess at postscript hints"},
    { "autoInstr", (PyCFunction) PyFFFont_autoInstr, METH_NOARGS, "Guess at truetype instructions"},
    { "autoWidth", (PyCFunction) PyFFFont_autoWidth, METH_VARARGS | METH_KEYWORDS, "Guess horizontal advance widths for selected glyphs" },
    { "autoTrace", (PyCFunction) PyFFFont_autoTrace, METH_NOARGS, "Autotrace any background images"},
    { "build", (PyCFunction) PyFFFont_Build, METH_NOARGS, "If the current glyph is an accented character\nand all components are in the font\nthen build it out of references" },
    { "canonicalContours", (PyCFunction) PyFFFont_canonicalContours, METH_NOARGS, "Orders the contours in the current glyph by the x coordinate of their leftmost point. (This can reduce the size of the postscript charstring needed to describe the glyph(s)."},
    { "canonicalStart", (PyCFunction) PyFFFont_canonicalStart, METH_NOARGS, "Sets the start point of all the contours of the current glyph to be the leftmost point on the contour."},
    { "changeWeight", (PyCFunction) PyFFFont_changeWeight, METH_VARARGS, "Change the weight (thickness) of the stems of the selected glyphs"},
    { "condenseExtend", (PyCFunction) PyFFFont_condenseExtend, METH_VARARGS, "Change the widths of the counters and side bearings of the selected glyphs"},
    { "cluster", (PyCFunction) PyFFFont_Cluster, METH_VARARGS, "Cluster the points of a glyph towards common values" },
    /*{ "compareGlyphs", (PyCFunction) PyFFFont_compareGlyphs, METH_VARARGS, "Compares two sets of glyphs"},*/
    /* compareGlyphs assumes an old scripting context */
    { "correctDirection", (PyCFunction) PyFFFont_correctDirection, METH_NOARGS, "Orient a layer so that external contours are clockwise and internal counter clockwise." },
    { "genericGlyphChange", (PyCFunction) PyFFFont_genericGlyphChange, METH_VARARGS | METH_KEYWORDS, "Rather like changeWeight or condenseExtend, called 'Change Glyph' in UI"},
    { "italicize", (PyCFunction) PyFFFont_italicize, METH_VARARGS | METH_KEYWORDS, "Italicize the selected glyphs"},
    { "intersect", (PyCFunction) PyFFFont_Intersect, METH_NOARGS, "Leaves the areas where the contours of a glyph overlap."},
    { "removeOverlap", (PyCFunction) PyFFFont_RemoveOverlap, METH_NOARGS, "Remove overlapping areas from a glyph"},
    { "round", (PyCFunction)PyFFFont_Round, METH_VARARGS, "Rounds point coordinates (and reference translations) to integers"},
    { "simplify", (PyCFunction)PyFFFont_Simplify, METH_VARARGS, "Simplifies a glyph" },
    { "stroke", (PyCFunction)PyFFFont_Stroke, METH_VARARGS | METH_KEYWORDS, "Strokes the contours in a glyph"},
    { "transform", (PyCFunction)PyFFFont_Transform, METH_VARARGS, "Transform a font by a 6 element matrix." },
    { "nltransform", (PyCFunction)PyFFFont_NLTransform, METH_VARARGS, "Transform a font by non-linear expressions for x and y." },
    { "validate", (PyCFunction)PyFFFont_validate, METH_VARARGS, "Check whether a font is valid and return True if it is." },
    { "reencode", (PyCFunction)PyFFFont_reencode, METH_VARARGS, "Reencodes the current font into the given encoding." },
    { "clearSpecialData", (PyCFunction)PyFFFont_clearSpecialData, METH_NOARGS, "Clear special data not accessible in FontForge." },

    // Leave some sentinel slots here so that the UI
    // code can add it's methods to the end of the object declaration.
    PYMETHODDEF_EMPTY,
    PYMETHODDEF_EMPTY,
    PYMETHODDEF_EMPTY,
    PYMETHODDEF_EMPTY,
    PYMETHODDEF_EMPTY,
    PYMETHODDEF_EMPTY,
    PYMETHODDEF_EMPTY,
    PYMETHODDEF_EMPTY,
    PYMETHODDEF_EMPTY,
    PYMETHODDEF_EMPTY,
    PYMETHODDEF_EMPTY,
    PYMETHODDEF_EMPTY,
    PYMETHODDEF_EMPTY,
    PYMETHODDEF_EMPTY,
    PYMETHODDEF_EMPTY,
    PYMETHODDEF_EMPTY,
    PYMETHODDEF_EMPTY,
    PYMETHODDEF_EMPTY,
    PYMETHODDEF_EMPTY,

    PYMETHODDEF_EMPTY /* Sentinel */
};

/* ************************************************************************** */
/* *********************** Font as glyph dictionary ************************* */
/* ************************************************************************** */
static Py_ssize_t PyFF_FontLength( PyObject *object ) {
    PyFF_Font *self = (PyFF_Font*)object;
    if ( CheckIfFontClosed(self) )
return (-1);
return( self->fv->map->enccount );
}

static PyObject *PyFF_FontIndex( PyObject *object, PyObject *index ) {
    PyFF_Font *self = (PyFF_Font*)object;
    FontViewBase *fv;
    SplineFont *sf;
    SplineChar *sc = NULL;

    if ( CheckIfFontClosed(self) )
        return (NULL);
    fv = self->fv;
    sf = fv->sf;
    if ( PyUnicode_Check(index)) {
	const char *name = PyUnicode_AsUTF8(index);
	if (name == NULL) {
	    return NULL;
	}
	sc = SFGetChar(sf,-1,name);
    } else if ( PyLong_Check(index)) {
	int pos = PyLong_AsLong(index), gid;
	if ( pos<0 || pos>=fv->map->enccount ) {
	    PyErr_Format(PyExc_TypeError, "Index out of bounds");
            return( NULL );
	}
	gid = fv->map->map[pos];
	sc = gid==-1 ? NULL : sf->glyphs[gid];
    } else {
	PyErr_Format(PyExc_TypeError, "Index must be an integer or a string" );
        return( NULL );
    }
    if ( sc==NULL ) {
	PyErr_Format(PyExc_TypeError, "No such glyph" );
        return( NULL );
    }
    return( PySC_From_SC_I(sc));
}

static int PyFF_FontContains( PyObject *object, PyObject *index ) {
    PyFF_Font *self = (PyFF_Font*)object;
    FontViewBase *fv;
    SplineFont *sf;
    SplineChar *sc = NULL;

    if ( CheckIfFontClosed(self) )
        return (-1);
    fv = self->fv;
    sf = fv->sf;
    if ( PyUnicode_Check(index)) {
	const char *name = PyUnicode_AsUTF8(index);
	if (name == NULL) {
	    return 0;
	}
	sc = SFGetChar(sf,-1,name);
    } else if ( PyLong_Check(index)) {
	int pos = PyLong_AsLong(index), gid;
	if ( pos<0 || pos>=fv->map->enccount ) {
            return( 0 );
	}
	gid = fv->map->map[pos];
	sc = gid==-1 ? NULL : sf->glyphs[gid];
    } else {
	PyErr_Format(PyExc_TypeError, "Index must be an integer or a string" );
        return( -1 );
    }
    return( sc!=NULL );
}

static PySequenceMethods PyFF_FontSequence = {
    PyFF_FontLength,		/* length */
    NULL,			/* concat */
    NULL,			/* repeat */
    NULL,			/* subscript */
    NULL,			/* slice */
    NULL,			/* subscript assign */
    NULL,			/* slice assign */
    PyFF_FontContains,		/* contains */
    NULL,			/* inplace_concat */
    NULL			/* inplace repeat */
};

static PyMappingMethods PyFF_FontMapping = {
    PyFF_FontLength,		/* length */
    PyFF_FontIndex,		/* subscript */
    NULL			/* subscript assign */
};

static PyTypeObject PyFF_FontType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "fontforge.font",          /* tp_name */
    sizeof(PyFF_Font),         /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor) PyFF_Font_dealloc, /* tp_dealloc */
    0,                         /* tp_vectorcall_offset */
    NULL,                      /* tp_getattr */
    NULL,                      /* tp_setattr */
    NULL,                      /* tp_compare */
    (reprfunc) PyFFFont_Repr,   /* tp_repr */
    NULL,                      /* tp_as_number */
    &PyFF_FontSequence,        /* tp_as_sequence */
    &PyFF_FontMapping,         /* tp_as_mapping */
    NULL,                      /* tp_hash */
    NULL,                      /* tp_call */
    (reprfunc) PyFFFont_Str,   /* tp_str */
    NULL,                      /* tp_getattro */
    NULL,                      /* tp_setattro */
    NULL,                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    "FontForge Font object",   /* tp_doc */
    NULL,                      /* tp_traverse */
    NULL,                      /* tp_clear */
    NULL,                      /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    fontiter_new_wholefont,    /* tp_iter */
    NULL,                      /* tp_iternext */
    PyFF_Font_methods,         /* tp_methods */
    NULL,                      /* tp_members */
    PyFF_Font_getset,          /* tp_getset */
    NULL,                      /* tp_base */
    NULL,                      /* tp_dict */
    NULL,                      /* tp_descr_get */
    NULL,                      /* tp_descr_set */
    0,                         /* tp_dictoffset */
    /*(initproc)PyFF_Font_init*/0,  /* tp_init */
    0,                         /* tp_alloc */
    PyFF_Font_new,             /* tp_new */
    NULL,                      /* tp_free */
    NULL,                      /* tp_is_gc */
    NULL,                      /* tp_bases */
    NULL,                      /* tp_mro */
    NULL,                      /* tp_cache */
    NULL,                      /* tp_subclasses */
    NULL,                      /* tp_weaklist */
    NULL,                      /* tp_del */
    0,                         /* tp_version_tag */
};

/* ************************************************************************** */
/*		     Python Interface to FontForge Auto-Kerning		      */
/* ************************************************************************** */

/* To give the user the ability to create his own routine to calculate the */
/*  visual separation between two glyphs, we must provide a python type which */
/*  contains the same data as the C type: AW_Glyph */
/* Sigh. And another which is almost exactly the same for the left/right arrays*/

typedef struct {
    PyObject_HEAD
    AW_Glyph *base;
    PyObject *left, *right;
} PyFF_AWGlyph;

typedef struct {
    PyObject_HEAD
    AW_Glyph *base;
    int is_left;
} PyFF_AWGlyphI;

typedef struct {
    PyObject_HEAD
    AW_Data *base;
    PyObject *emSize;
    PyObject *layer;
    PyObject *regionHeight;
    PyObject *denom;
} PyFF_AWContext;
static PyTypeObject PyFF_AWGlyphType, PyFF_AWGlyphIndexType, PyFF_AWContextType;

static void PyFF_AWGlyphIndex_dealloc(PyFF_AWGlyphI *self) {
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static Py_ssize_t PyFF_AWGlyphLength( PyObject *self ) {
    AW_Glyph *aw = ((PyFF_AWGlyphI *) self)->base;
return( aw->imax_y - aw->imin_y + 1 );
}

static PyObject *PyFF_AWGlyphIndex( PyObject *self, PyObject *index ) {
    AW_Glyph *aw = ((PyFF_AWGlyphI *) self)->base;
    int pos;

    if ( !PyLong_Check(index)) {
	PyErr_Format(PyExc_TypeError, "Index must be an integer" );
return( NULL );
    }

    pos = PyLong_AsLong(index);
    if ( pos<aw->imin_y || pos>aw->imax_y ) {
	PyErr_Format(PyExc_TypeError, "Index out of bounds");
return( NULL );
    }
    if ( ((PyFF_AWGlyphI *) self)->is_left )
return( Py_BuildValue("i", aw->left[pos-aw->imin_y]) );
    else
return( Py_BuildValue("i", aw->right[pos-aw->imin_y]) );
}

static PyMappingMethods PyFF_AWGlyphMapping = {
    PyFF_AWGlyphLength,		/* length */
    PyFF_AWGlyphIndex,		/* subscript */
    NULL			/* subscript assign */
};

static PyTypeObject PyFF_AWGlyphIndexType = {
    PyVarObject_HEAD_INIT(NULL,0)
    "fontforge.awglyphIndex",  /* tp_name */
    sizeof(PyFF_AWGlyphI),     /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor) PyFF_AWGlyphIndex_dealloc, /* tp_dealloc */
    0,                         /* tp_vectorcall_offset */
    NULL,                      /* tp_getattr */
    NULL,                      /* tp_setattr */
    NULL,                      /* tp_compare */
    NULL,                      /* tp_repr */
    NULL,                      /* tp_as_number */
    NULL,                      /* tp_as_sequence */
    &PyFF_AWGlyphMapping,      /* tp_as_mapping */
    NULL,                      /* tp_hash */
    NULL,                      /* tp_call */
    NULL,                      /* tp_str */
    NULL,                      /* tp_getattro */
    NULL,                      /* tp_setattro */
    NULL,                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    "FontForge index aw",      /* tp_doc */
    NULL,                      /* tp_traverse */
    NULL,                      /* tp_clear */
    NULL,                      /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    NULL,                      /* tp_iter */
    NULL,                      /* tp_iternext */
    NULL,                      /* tp_methods */
    NULL,                      /* tp_members */
    NULL,                      /* tp_getset */
    NULL,                      /* tp_base */
    NULL,                      /* tp_dict */
    NULL,                      /* tp_descr_get */
    NULL,                      /* tp_descr_set */
    0,                         /* tp_dictoffset */
    NULL,                      /* tp_init */
    NULL,                      /* tp_alloc */
    NULL,                      /* tp_new */
    NULL,                      /* tp_free */
    NULL,                      /* tp_is_gc */
    NULL,                      /* tp_bases */
    NULL,                      /* tp_mro */
    NULL,                      /* tp_cache */
    NULL,                      /* tp_subclasses */
    NULL,                      /* tp_weaklist */
    NULL,                      /* tp_del */
    0,                         /* tp_version_tag */
};

static void PyFF_AWGlyph_dealloc(PyFF_AWGlyph *self) {
    Py_XDECREF(self->left);
    Py_XDECREF(self->right);
    if ( self->base!=NULL ) {
	if ( self->base->python_data == self )
	    self->base->python_data = NULL;
	self->base = NULL;
    }
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *GetPythonObjectForAWGlyph(AW_Glyph *aw) {
    if ( aw->python_data==NULL ) {
	aw->python_data = PyFF_AWGlyphType.tp_alloc(&PyFF_AWGlyphType,0);
	((PyFF_AWGlyph *) (aw->python_data))->base = aw;
	Py_INCREF( (PyObject *) (aw->python_data) );	/* for the pointer in the aw_glyph */
    }
    Py_INCREF( (PyFF_AWGlyph *) (aw->python_data) );
return( (PyObject *) (aw->python_data) );
}

static PyObject *PyFF_AWGlyph_getGlyph(PyFF_AWGlyph *self, void *UNUSED(closure)) {
    PyObject *ret;
    if ( self->base->sc==NULL )
	ret = Py_None;
    else
	ret = PySC_From_SC(self->base->sc);
    Py_INCREF( ret );
return( ret );
}

static PyObject *PyFF_AWGlyph_getBB(PyFF_AWGlyph *self, void *UNUSED(closure)) {
return( Py_BuildValue("(dddd)", self->base->bb.minx,self->base->bb.miny,
	    self->base->bb.maxx,self->base->bb.maxy ));
}

static PyObject *PyFF_AWGlyph_getIminY(PyFF_AWGlyph *self, void *UNUSED(closure)) {
return( Py_BuildValue("i", self->base->imin_y ));
}

static PyObject *PyFF_AWGlyph_getImaxY(PyFF_AWGlyph *self, void *UNUSED(closure)) {
return( Py_BuildValue("i", self->base->imax_y ));
}

static PyObject *PyFF_AWGlyph_getLeft(PyFF_AWGlyph *self, void *UNUSED(closure)) {
    if ( self->left==NULL ) {
	self->left = PyFF_AWGlyphIndexType.tp_alloc(&PyFF_AWGlyphIndexType,0);
	((PyFF_AWGlyphI *) (self->left))->base = self->base;
	((PyFF_AWGlyphI *) (self->left))->is_left = true;
	Py_INCREF( self->left );	/* for the pointer in the aw_glyph */
    }
    Py_INCREF( self->left );
return( self->left );
}

static PyObject *PyFF_AWGlyph_getRight(PyFF_AWGlyph *self, void *UNUSED(closure)) {
    if ( self->right==NULL ) {
	self->right = PyFF_AWGlyphIndexType.tp_alloc(&PyFF_AWGlyphIndexType,0);
	((PyFF_AWGlyphI *) (self->right))->base = self->base;
	((PyFF_AWGlyphI *) (self->right))->is_left = false;
	Py_INCREF( self->right );	/* for the pointer in the aw_glyph */
    }
    Py_INCREF( self->right );
return( self->right );
}

static PyGetSetDef PyFF_AWGlyph_getset[] = {
    {(char *)"glyph",
     (getter)PyFF_AWGlyph_getGlyph, NULL,
     (char *)"The underlying glyph which this object describes", NULL},
    {(char *)"boundingbox",
     (getter)PyFF_AWGlyph_getBB, NULL,
     (char *)"The bounding box of the underlying glyph", NULL},
    {(char *)"iminY",
     (getter)PyFF_AWGlyph_getIminY, NULL,
     (char *)"floor(bb.min_y/decimation_height)", NULL},
    {(char *)"imaxY",
     (getter)PyFF_AWGlyph_getImaxY, NULL,
     (char *)"ceil(bb.max_y/decimation_height)", NULL},
    {(char *)"left",
     (getter)PyFF_AWGlyph_getLeft, NULL,
     (char *)"array with left edge offsets from bounding box", NULL},
    {(char *)"right",
     (getter)PyFF_AWGlyph_getRight, NULL,
     (char *)"array with left edge offsets from bounding box", NULL},
    PYGETSETDEF_EMPTY /* Sentinel */
};

static PyTypeObject PyFF_AWGlyphType = {
    PyVarObject_HEAD_INIT(NULL,0)
    "fontforge.awglyph",       /* tp_name */
    sizeof(PyFF_AWGlyph),      /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor) PyFF_AWGlyph_dealloc, /* tp_dealloc */
    0,                         /* tp_vectorcall_offset */
    NULL,                      /* tp_getattr */
    NULL,                      /* tp_setattr */
    NULL,                      /* tp_compare */
    NULL,                      /* tp_repr */
    NULL,                      /* tp_as_number */
    NULL,                      /* tp_as_sequence */
    NULL,                      /* tp_as_mapping */    /* Need separate left/right version so needs a new type. Sigh. */
    NULL,                      /* tp_hash */
    NULL,                      /* tp_call */
    NULL,                      /* tp_str */
    NULL,                      /* tp_getattro */
    NULL,                      /* tp_setattro */
    NULL,                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    "FontForge Auto Width/Kern Glyph object",   /* tp_doc */
    NULL,                      /* tp_traverse */
    NULL,                      /* tp_clear */
    NULL,                      /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    NULL,                      /* tp_iter */
    NULL,                      /* tp_iternext */
    NULL,                      /* tp_methods */
    NULL,                      /* tp_members */
    PyFF_AWGlyph_getset,       /* tp_getset */
    NULL,                      /* tp_base */
    NULL,                      /* tp_dict */
    NULL,                      /* tp_descr_get */
    NULL,                      /* tp_descr_set */
    0,                         /* tp_dictoffset */
    NULL,                      /* tp_init */
    NULL,                      /* tp_alloc */
    NULL, /*PyFF_AWGlyph_new*/ /* tp_new */
    NULL,                      /* tp_free */
    NULL,                      /* tp_is_gc */
    NULL,                      /* tp_bases */
    NULL,                      /* tp_mro */
    NULL,                      /* tp_cache */
    NULL,                      /* tp_subclasses */
    NULL,                      /* tp_weaklist */
    NULL,                      /* tp_del */
    0,                         /* tp_version_tag */
};

static void PyFF_AWContext_dealloc(PyFF_AWContext *self) {
    Py_XDECREF(self->emSize);
    Py_XDECREF(self->layer);
    Py_XDECREF(self->regionHeight);
    Py_XDECREF(self->denom);
    if ( self->base!=NULL ) {
	self->base->python_data = NULL;
	self->base = NULL;
    }
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *GetPythonObjectForAWData(AW_Data *all) {
    if ( all->python_data==NULL ) {
	all->python_data = PyFF_AWContextType.tp_alloc(&PyFF_AWContextType,0);
	((PyFF_AWContext *) (all->python_data))->base = all;
	Py_INCREF( (PyObject *) (all->python_data) );	/* for the pointer in the aw_glyph */
    }
    Py_INCREF( (PyFF_AWContext *) (all->python_data) );
return( (PyObject *) (all->python_data) );
}

static PyObject *PyFF_AWContext_getFont(PyFF_AWContext *self, void *UNUSED(closure)) {
    return PyFF_FontForFV( self->base->fv );
}

static PyObject *PyFF_AWContext_getEmSize(PyFF_AWContext *self, void *UNUSED(closure)) {
    if ( self->emSize==NULL )
	self->emSize = PyLong_FromLong(self->base->sf->ascent+self->base->sf->descent);
    Py_INCREF( self->emSize );
return( self->emSize );
}

static PyObject *PyFF_AWContext_getLayer(PyFF_AWContext *self, void *UNUSED(closure)) {
    if ( self->layer==NULL )
	self->layer = PyLong_FromLong(self->base->layer);
    Py_INCREF( self->layer );
return( self->layer );
}

static PyObject *PyFF_AWContext_getRegionHeight(PyFF_AWContext *self, void *UNUSED(closure)) {
    if ( self->regionHeight==NULL )
	self->regionHeight = PyLong_FromLong(self->base->sub_height);
    Py_INCREF( self->regionHeight );
return( self->regionHeight );
}

static PyObject *PyFF_AWContext_getDenom(PyFF_AWContext *self, void *UNUSED(closure)) {
    if ( self->denom==NULL )
	self->denom = PyFloat_FromDouble(self->base->denom);
    Py_INCREF( self->denom );
return( self->denom );
}

static PyGetSetDef PyFF_AWContext_getset[] = {
    {(char *)"font",
     (getter)PyFF_AWContext_getFont, NULL,
     (char *)"The underlying font which this object describes", NULL},
    {(char *)"emSize",
     (getter)PyFF_AWContext_getEmSize, NULL,
     (char *)"Font's em-size", NULL},
    {(char *)"layer",
     (getter)PyFF_AWContext_getLayer, NULL,
     (char *)"active layer during the current operation", NULL},
    {(char *)"regionHeight",
     (getter)PyFF_AWContext_getRegionHeight, NULL,
     (char *)"The y coordinate line is subdivided into regions, and this is the height of each region", NULL},
    {(char *)"denom",
     (getter)PyFF_AWContext_getDenom, NULL,
     (char *)"A useful small number which varies with the emsize", NULL},
    PYGETSETDEF_EMPTY /* Sentinel */
};

static PyTypeObject PyFF_AWContextType = {
    PyVarObject_HEAD_INIT(NULL,0)
    "fontforge.awcontext",     /* tp_name */
    sizeof(PyFF_AWContext),    /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor) PyFF_AWContext_dealloc, /* tp_dealloc */
    0,                         /* tp_vectorcall_offset */
    NULL,                      /* tp_getattr */
    NULL,                      /* tp_setattr */
    NULL,                      /* tp_compare */
    NULL,                      /* tp_repr */
    NULL,                      /* tp_as_number */
    NULL,                      /* tp_as_sequence */
    NULL,                      /* tp_as_mapping */        /* Need separate left/right version so needs a new type. Sigh. */
    NULL,                      /* tp_hash */
    NULL,                      /* tp_call */
    NULL,                      /* tp_str */
    NULL,                      /* tp_getattro */
    NULL,                      /* tp_setattro */
    NULL,                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    "FontForge Auto Width/Kern Context object", /* tp_doc */
    NULL,                      /* tp_traverse */
    NULL,                      /* tp_clear */
    NULL,                      /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    NULL,                      /* tp_iter */
    NULL,                      /* tp_iternext */
    NULL,                      /* tp_methods */
    NULL,                      /* tp_members */
    PyFF_AWContext_getset,     /* tp_getset */
    NULL,                      /* tp_base */
    NULL,                      /* tp_dict */
    NULL,                      /* tp_descr_get */
    NULL,                      /* tp_descr_set */
    0,                         /* tp_dictoffset */
    NULL,                      /* tp_init */
    NULL,                      /* tp_alloc */
    NULL,                      /* tp_new */
    NULL,                      /* tp_free */
    NULL,                      /* tp_is_gc */
    NULL,                      /* tp_bases */
    NULL,                      /* tp_mro */
    NULL,                      /* tp_cache */
    NULL,                      /* tp_subclasses */
    NULL,                      /* tp_weaklist */
    NULL,                      /* tp_del */
    0,                         /* tp_version_tag */
};

/* User supplied python function which calculates the optical separation between */
/*  two glyphs when placed so that there bounding boxes are contiguous. I assume */
/*  that optical separation is linear (so if I move the bounding boxes by 3 em */
/*  then the optical separation also increases by 3 em) */
void *PyFF_GlyphSeparationHook = NULL;		/* Actually a python object */
static PyObject *PyFF_GlyphSeparationArg = NULL;

int PyFF_GlyphSeparation(AW_Glyph *g1,AW_Glyph *g2,AW_Data *all) {
    PyObject *arglist, *result;
    int ret;

    if ( PyFF_GlyphSeparationHook==NULL )
return( -1 );

    arglist = PyTuple_New(PyFF_GlyphSeparationArg!=NULL &&
			    PyFF_GlyphSeparationArg!=Py_None?4:3);
    Py_XINCREF((PyObject *) PyFF_GlyphSeparationHook);
    PyTuple_SetItem(arglist,0,GetPythonObjectForAWGlyph(g1));
    PyTuple_SetItem(arglist,1,GetPythonObjectForAWGlyph(g2));
    PyTuple_SetItem(arglist,2,GetPythonObjectForAWData(all));
    if ( PyFF_GlyphSeparationArg!=NULL &&
			    PyFF_GlyphSeparationArg!=Py_None ) {
	PyTuple_SetItem(arglist,3,PyFF_GlyphSeparationArg);
	Py_XINCREF(PyFF_GlyphSeparationArg);
    }
    result = PyObject_CallObject(PyFF_GlyphSeparationHook, arglist);
    Py_DECREF(arglist);
    if ( PyErr_Occurred()!=NULL ) {
	PyErr_Print();
	Py_XDECREF(result);
return( -1 );
    } else {
	ret = PyLong_AsLong(result);
	Py_XDECREF(result);
	if ( PyErr_Occurred()!=NULL ) {
	    PyErr_Print();
return( -1 );
	}
return( ret );
    }
}

static PyObject *PyFF_registerGlyphSeparationHook(PyObject *UNUSED(self), PyObject *args) {
    PyObject *name;	/* Ignored for now */
    PyObject *hook, *arg=NULL;

    if ( !PyArg_ParseTuple(args,"O|OO",&hook, &arg, &name ) )
return( NULL );
    if ( hook==Py_None ) {
	Py_XDECREF((PyObject *) PyFF_GlyphSeparationHook);
	Py_XDECREF( PyFF_GlyphSeparationArg);
	PyFF_GlyphSeparationHook = NULL;
	PyFF_GlyphSeparationArg = NULL;
    } else if (!PyCallable_Check((PyObject *) hook)) {
	PyErr_Format(PyExc_TypeError, "First argument is not callable" );
return( NULL );
    } else {
	Py_XDECREF((PyObject *) PyFF_GlyphSeparationHook);
	Py_XDECREF( PyFF_GlyphSeparationArg);
	PyFF_GlyphSeparationHook = hook;
	Py_XINCREF((PyObject *) PyFF_GlyphSeparationHook);
	if ( arg==Py_None )
	    arg = NULL;
	PyFF_GlyphSeparationArg = arg;
	Py_XINCREF(PyFF_GlyphSeparationArg);
    }

Py_RETURN_NONE;
}

void FFPy_AWGlyphFree(AW_Glyph *me) {
    Py_XDECREF((PyObject *) me->python_data);
}

void FFPy_AWDataFree(AW_Data *all) {
    Py_XDECREF((PyObject *) all->python_data);
}
/* ************************************************************************** */
/*			     FontForge Python Module			      */
/* ************************************************************************** */
PyMethodDef module_fontforge_methods[] = {
    { "getPrefs", PyFF_GetPrefs, METH_VARARGS, "Get FontForge preference items" },
    { "setPrefs", PyFF_SetPrefs, METH_VARARGS, "Set FontForge preference items" },
    { "savePrefs", PyFF_SavePrefs, METH_NOARGS, "Save FontForge preference items" },
    { "loadPrefs", PyFF_LoadPrefs, METH_NOARGS, "Load FontForge preference items" },
    { "hasSpiro", PyFF_hasSpiro, METH_NOARGS, "Returns whether this fontforge has access to Raph Levien's spiro package"},
    { "SpiroVersion", PyFF_SpiroVersion, METH_NOARGS, "Return Spiro Library Version" },
    { "onAppClosing", PyFF_onAppClosing, METH_VARARGS, "add a python function which is called when fontforge is closing down"},
    { "defaultOtherSubrs", PyFF_DefaultOtherSubrs, METH_NOARGS, "Use FontForge's default \"othersubrs\" functions for Type1 fonts" },
    { "readOtherSubrsFile", PyFF_ReadOtherSubrsFile, METH_VARARGS, "Read from a file, \"othersubrs\" functions for Type1 fonts" },
    { "loadEncodingFile", PyFF_LoadEncodingFile, METH_VARARGS, "Load an encoding file into the list of encodings" },
    { "loadNamelist", PyFF_LoadNamelist, METH_VARARGS, "Load a namelist into the list of namelists" },
    { "loadNamelistDir", PyFF_LoadNamelistDir, METH_VARARGS, "Load a directory of namelist files into the list of namelists" },
    { "preloadCidmap", PyFF_PreloadCidmap, METH_VARARGS, "Load a cidmap file" },
    { "unicodeFromName", PyFF_UnicodeFromName, METH_VARARGS, "Given a name, look it up in the namelists and find what unicode code point it maps to (returns -1 if not found)" },
    { "nameFromUnicode", PyFF_NameFromUnicode, METH_VARARGS, "Given a unicode code point and (optionally) a namelist, find the corresponding glyph name" },
    /* --start of names list functions------------------------ */
    { "UnicodeNameFromLib", PyFF_UnicodeNameFromLib, METH_VARARGS, "Return the www.unicode.org name for a given unicode character value" },
    { "UnicodeAnnotationFromLib", PyFF_UnicodeAnnotationFromLib, METH_VARARGS, "Return the www.unicode.org annotation(s) for a given unicode character value" },
    { "UnicodeBlockCountFromLib", PyFF_UnicodeBlockCountFromLib, METH_NOARGS, "Return the www.unicode.org block count" },
    { "UnicodeBlockStartFromLib", PyFF_UnicodeBlockStartFromLib, METH_VARARGS, "Return the www.unicode.org block start, for example block[0]={0..127} -> 0" },
    { "UnicodeBlockEndFromLib", PyFF_UnicodeBlockEndFromLib, METH_VARARGS, "Return the www.unicode.org block end, for example block[1]={128..255} -> 255" },
    { "UnicodeBlockNameFromLib", PyFF_UnicodeBlockNameFromLib, METH_VARARGS, "Return the www.unicode.org block name, for example block[2]={256..383} -> Latin Extended-A" },
    { "UnicodeNamesListVersion", PyFF_UnicodeNamesListVersion, METH_NOARGS, "Return the www.unicode.org NamesList version for this library" },
    { "UnicodeNames2FromLib", PyFF_UnicodeNames2FromLib, METH_VARARGS, "Return the www.unicode.org NamesList formal alias for this Unicode value if it exists for this library" },
    { "scriptFromUnicode", PyFF_scriptFromUnicode, METH_VARARGS, "Return the script tag for the given Unicode codepoint. So, 'Q' would return \"latn\"." },
    /* --end of names list functions-------------------------- */
    { "version", PyFF_Version, METH_NOARGS, "Returns a string containing the current version of FontForge, as 20061116" },
    { "runInitScripts", PyFF_RunInitScripts, METH_NOARGS, "Run the system and user initialization scripts, if not already run" },
    { "loadPlugins", PyFF_LoadPlugins, METH_NOARGS, "Load and initialize any active plugins not already initialized." },
    { "getPluginInfo", PyFF_GetPluginInfo, METH_NOARGS, "Returns an ordered list of tuples with configuration and other information about each discovered or recorded plugin." },
    { "configurePlugins", PyFF_ConfigurePlugins, METH_VARARGS, "Change the order of or enable/disable plugins." },
    { "scriptPath", PyFF_GetScriptPath, METH_NOARGS, "Returns a list of the directories searched for scripts"},
    { "userConfigPath", PyFF_GetUserConfigPath, METH_NOARGS, "Returns the path to the user's FontForge configuration directory, which should be writable."},
    { "fonts", PyFF_FontTuple, METH_NOARGS, "Returns a tuple of all loaded fonts" },
    { "fontsInFile", PyFF_FontsInFile, METH_VARARGS, "Returns a tuple containing the names of any fonts in an external file"},
    { "open", PyFF_OpenFont, METH_VARARGS, "Opens a font and returns it" },
    { "printSetup", PyFF_printSetup, METH_VARARGS, "Prepare to print a font sample (select default printer or file, page size, etc.)" },
    { "parseTTInstrs", PyFF_ParseTTFInstrs, METH_VARARGS, "Takes a string and parses it into a tuple of truetype instruction bytes"},
    { "unParseTTInstrs", PyFF_UnParseTTFInstrs, METH_VARARGS, "Takes a tuple of truetype instruction bytes and converts to a human readable string"},
    { "unitShape", PyFF_unitShape, METH_VARARGS, "Takes an integer argument an returns a regular n-gon with that many sides. If the argument is positive the n-gon is inscribed on the unit circle, negative it is circumscribed, 0 the result is the unit circle, 1 a square. Behavior undefined for 2, -1, -2."},
    { "activeFont", PyFF_ActiveFont, METH_NOARGS, "If invoked from the UI, this returns the currently active font. When not in UI this returns None"},
    /* Deprecated name for the above */
    { "activeFontInUI", PyFF_ActiveFont, METH_NOARGS, "If invoked from the UI, this returns the currently active font. When not in UI this returns None"},
    { "activeGlyph", PyFF_ActiveGlyph, METH_NOARGS, "If invoked from the UI, this returns the currently active glyph (or None)"},
    { "activeLayer", PyFF_ActiveLayer, METH_NOARGS, "If invoked from the UI, this returns the currently active layer"},
    { "registerGlyphSeparationHook", PyFF_registerGlyphSeparationHook, METH_VARARGS, "registers a python routine which finds the visual separation between two glyphs. Used in autowidth/kern and optical bound setting."},
    /* Access to the User Interface ... if any */
    { "hasUserInterface", PyFF_hasUserInterface, METH_NOARGS, "Returns whether this fontforge session has a user interface (True if it has opened windows) or is just running a script (False)"},
    { "registerImportExport", PyFF_registerImportExport, METH_VARARGS, "Adds an import/export spline conversion module"},
    { "registerMenuItem", (PyCFunction)PyFF_registerMenuItemStub, METH_VARARGS | METH_KEYWORDS, "Adds a menu item (which runs a python script) to the font or glyph (or both) windows -- in the Tools menu"},
    { "getConvexNib", PyFF_getConvexNib, METH_VARARGS, "Sets the specified 'Convex' to the layer/contour argument."},
    { "setConvexNib", PyFF_setConvexNib, METH_VARARGS, "Returns the specified 'Convex' nib as a layer."},
    { "logWarning", PyFF_logError, METH_VARARGS, "Adds a non-fatal message to the Warnings window"},
    { "postError", PyFF_postError, METH_VARARGS, "Pops up an error dialog box with the given title and message" },
    { "postNotice", PyFF_postNotice, METH_VARARGS, "Pops up an notice window with the given title and message" },
    { "openFilename", PyFF_openFilename, METH_VARARGS, "Pops up a file picker dialog asking the user for a filename to open" },
    { "saveFilename", PyFF_saveFilename, METH_VARARGS, "Pops up a file picker dialog asking the user for a filename to use for saving" },
    { "ask", PyFF_ask, METH_VARARGS, "Pops up a dialog asking the user a question and providing a set of buttons for the user to reply with" },
    { "askChoices", (PyCFunction)PyFF_askChoices, METH_VARARGS | METH_KEYWORDS, "Pops up a dialog asking the user a question and providing a scrolling list for the user to reply with" },
    { "askString", PyFF_askString, METH_VARARGS, "Pops up a dialog asking the user a question and providing a textfield for the user to reply with" },
    { "askMulti", (PyCFunction)PyFF_askMulti, METH_VARARGS | METH_KEYWORDS, "Pops up a dialog asking the user a question and providing a textfield for the user to reply with" },
    // Leave some sentinel slots here so that the UI
    // code can add it's methods to the end of the object declaration.
    PYMETHODDEF_EMPTY,
    PYMETHODDEF_EMPTY,
    PYMETHODDEF_EMPTY,
    PYMETHODDEF_EMPTY,
    PYMETHODDEF_EMPTY,
    PYMETHODDEF_EMPTY,
    PYMETHODDEF_EMPTY,
    PYMETHODDEF_EMPTY,
    PYMETHODDEF_EMPTY,
    PYMETHODDEF_EMPTY,
    PYMETHODDEF_EMPTY,
    PYMETHODDEF_EMPTY,
    PYMETHODDEF_EMPTY,
    PYMETHODDEF_EMPTY,
    PYMETHODDEF_EMPTY,
    PYMETHODDEF_EMPTY,
    PYMETHODDEF_EMPTY,
    PYMETHODDEF_EMPTY,
    PYMETHODDEF_EMPTY,

    PYMETHODDEF_EMPTY  /* Sentinel */
};

static python_type_info module_fontforge_types[] = {
    {&PyFF_AWContextType,	1, NULL},
    {&PyFF_AWGlyphIndexType,	0, NULL},
    {&PyFF_AWGlyphType,		1, NULL},
    {&PyFF_ContourIterType,	0, NULL},
    {&PyFF_ContourType,		1, NULL},
    {&PyFF_CvtIterType,		0, NULL},
    {&PyFF_CvtType,		1, NULL},
    {&PyFF_FontIterType,	0, NULL},
    {&PyFF_FontType,		1, NULL},
    {&PyFF_GlyphPenType,	1, NULL},
    {&PyFF_GlyphType,		1, NULL},
    {&PyFF_LayerArrayIterType,	0, NULL},
    {&PyFF_LayerArrayType,	1, NULL},
    {&PyFF_LayerInfoArrayIterType,	0, NULL},
    {&PyFF_LayerInfoArrayType,	1, NULL},
    {&PyFF_LayerInfoType,	1, NULL},
    {&PyFF_LayerIterType,	0, NULL},
    {&PyFF_LayerType,		1, NULL},
    {&PyFF_MathKernType,	1, NULL},
    {&PyFF_MathType,		1, setup_math_type},
    {&PyFF_PointType,		1, NULL},
    {&PyFF_PrivateIterType,	0, NULL},
    {&PyFF_PrivateType,		1, NULL},
    {&PyFF_RefArrayType,	1, NULL},
    {&PyFF_SelectionType,	1, NULL},
    TYPEINFO_EMPTY
};

static void AddHookDictionary( PyObject *module );
static void AddSpiroConstants( PyObject *module );
static void FinalizeFontforgeModule( PyObject* module ) {
    AddHookDictionary( module );
    AddPointConstants( module );
    AddSpiroConstants( module );
    PyModule_AddObject( module, "unspecifiedMathValue",
                        Py_BuildValue("i", TEX_UNDEF) );
}

static module_definition module_def_fontforge = {
    "fontforge",                           /* module_name */
    "FontForge font manipulation module.", /* docstring */
    module_fontforge_types,                /* types */
    module_fontforge_methods,              /* methods */
    true,                                  /* auto_import */
    FinalizeFontforgeModule,               /* finalize_func */
    MODULEDEF_RUNTIMEINFO_INIT
};

/* ************************************************************************** */
/* ************************* initializer routines *************************** */
/* ************************************************************************** */
void FfPy_Replace_MenuItemStub(PyObject *(*func)(PyObject *,PyObject *)) {
    int i;
    PyMethodDef *methods = module_fontforge_methods;
    for ( i=0; methods[i].ml_name!=NULL; ++i )
	if ( strcmp(methods[i].ml_name,"registerMenuItem")==0 ) {
	    methods[i].ml_meth = func;
return;
	}
}

static PyObject *PyPS_Identity(PyObject *UNUSED(noself), PyObject *UNUSED(args)) {
return( Py_BuildValue("(dddddd)",  1.0, 0.0, 0.0,  1.0, 0.0, 0.0));
}

static PyObject *PyPS_Translate(PyObject *UNUSED(noself), PyObject *args) {
    double x,y=0;

    if ( !PyArg_ParseTuple(args,"d|d",&x,&y) ) {
	PyErr_Clear();
	if ( !PyArg_ParseTuple(args,"(dd)",&x,&y) )
return( NULL );
    }

return( Py_BuildValue("(dddddd)",  1.0, 0.0, 0.0, 1.0,  x, y));
}

static PyObject *PyPS_Scale(PyObject *UNUSED(noself), PyObject *args) {
    double x,y=-99999;

    if ( !PyArg_ParseTuple(args,"d|d",&x,&y) )
return( NULL );
    if ( y== -99999 )
	y = x;

return( Py_BuildValue("(dddddd)",  x, 0.0, 0.0, y,  0.0, 0.0));
}

static PyObject *PyPS_Rotate(PyObject *UNUSED(noself), PyObject *args) {
    double theta, c, s;

    if ( !PyArg_ParseTuple(args,"d",&theta) )
return( NULL );
    c = cos(theta); s = sin(theta);

return( Py_BuildValue("(dddddd)",  c, s, -s, c,  0.0, 0.0));
}

static PyObject *PyPS_Skew(PyObject *UNUSED(noself), PyObject *args) {
    double theta, t;

    if ( !PyArg_ParseTuple(args,"d",&theta) )
return( NULL );
    t = tan(theta);

return( Py_BuildValue("(dddddd)",  1.0, 0.0, t, 1.0,  0.0, 0.0));
}

static PyObject *PyPS_Compose(PyObject *UNUSED(noself), PyObject *args) {
    double m1[6], m2[6];
    real r1[6], r2[6], r3[6];
    int i;
    PyObject *tuple;

    if ( !PyArg_ParseTuple(args,"(dddddd)(dddddd)",
	    &m1[0], &m1[1], &m1[2], &m1[3], &m1[4], &m1[5],
	    &m2[0], &m2[1], &m2[2], &m2[3], &m2[4], &m2[5] ))
return( NULL );
    for ( i=0; i<6; ++i ) {
	r1[i] = m1[i]; r2[i] = m2[i];
    }
    MatMultiply(r1,r2,r3);
    tuple = PyTuple_New(6);
    for ( i=0; i<6; ++i )
	PyTuple_SetItem(tuple,i,Py_BuildValue("d",(double) r3[i]));
return( tuple );
}

static PyObject *PyPS_Inverse(PyObject *UNUSED(noself), PyObject *args) {
    double m1[6];
    real r1[6], r3[6];
    int i;
    PyObject *tuple;

    if ( !PyArg_ParseTuple(args,"(dddddd)",
	    &m1[0], &m1[1], &m1[2], &m1[3], &m1[4], &m1[5] ))
return( NULL );
    for ( i=0; i<6; ++i )
	r1[i] = m1[i];
    MatInverse(r3,r1);
    tuple = PyTuple_New(6);
    for ( i=0; i<6; ++i )
	PyTuple_SetItem(tuple,i,Py_BuildValue("d",(double) r3[i]));
return( tuple );
}

static PyMethodDef module_psMat_methods[] = {
    { "identity", PyPS_Identity, METH_NOARGS, "Identity transformation" },
    { "translate", PyPS_Translate, METH_VARARGS, "Translation transformation" },
    { "rotate", PyPS_Rotate, METH_VARARGS, "Rotation transformation" },
    { "skew", PyPS_Skew, METH_VARARGS, "Skew transformation (for making a oblique font)" },
    { "scale", PyPS_Scale, METH_VARARGS, "Scale transformation" },
    { "compose", PyPS_Compose, METH_VARARGS, "Compose two transformations (matrix multiplication)" },
    { "inverse", PyPS_Inverse, METH_VARARGS, "Provide an inverse transformations (not always possible)" },
    PYMETHODDEF_EMPTY /* Sentinel */
};

static module_definition module_def_psMat = {
    "psMat",                               /* module_name */
    "PostScript Matric manipulation",      /* docstring */
    NULL,                                  /* types */
    module_psMat_methods,                  /* methods */
    true,                                  /* auto_import */
    NULL,                                  /* finalize_func */
    MODULEDEF_RUNTIMEINFO_INIT
};


static void PyFF_PicklerInit(void) {
    if ( pickler==NULL ) {
        FontForge_InitializeEmbeddedPython();
        PyRun_SimpleString("import pickle\nimport __FontForge_Internals___;\n__FontForge_Internals___.initPickles(pickle.dumps, pickle.loads);");
    }
}

static void PyFF_PickleTypesInit(void) {
    if ( _new_point==NULL )
	PyRun_SimpleString("import __FontForge_Internals___;\n__FontForge_Internals___.initPickleTypes(__FontForge_Internals___.newPoint,__FontForge_Internals___.newContour,__FontForge_Internals___.newLayer);");
}

char *PyFF_PickleMeToString(void *pydata) {
    PyObject *pyobj, *arglist, *result;
    char *ret = NULL;

    PyFF_PicklerInit();
    pyobj = pydata;
    arglist = PyTuple_New(2);
    Py_XINCREF(pyobj);
    PyTuple_SetItem(arglist,0,pyobj);
    PyTuple_SetItem(arglist,1,Py_BuildValue("i",0));	/* ASCII protocol */
    result = PyObject_CallObject(pickler, arglist);
    Py_DECREF(arglist);
    if ( result!=NULL )
	ret = copy(PyBytes_AsString(result));
    Py_XDECREF(result);
    if ( PyErr_Occurred()!=NULL ) {
	PyErr_Print();
	free(ret);
return( NULL );
    } else
return( ret );
}

void *PyFF_UnPickleMeToObjects(char *str) {
    PyObject *arglist, *result;

    PyFF_PicklerInit();
    arglist = PyTuple_New(1);
    PyTuple_SetItem(arglist,0,Py_BuildValue("y",str)); /* Bytes/String object */
    result = PyObject_CallObject(unpickler, arglist);
    Py_DECREF(arglist);
    if ( PyErr_Occurred()!=NULL ) {
	PyErr_Print();
return( NULL );
    } else
return( result );
}

static PyObject *PyFFi_initPickles(PyObject *UNUSED(noself), PyObject *args) {

    if ( !PyArg_ParseTuple(args,"OO",&pickler, &unpickler ))
return( NULL );
    Py_INCREF(pickler); Py_INCREF(unpickler);
Py_RETURN_NONE;
}

static PyObject *PyFFi_initPickleTypes(PyObject *UNUSED(noself), PyObject *args) {

    if ( !PyArg_ParseTuple(args,"OOO",&_new_point, &_new_contour, &_new_layer ))
return( NULL );
    Py_INCREF(_new_point); Py_INCREF(_new_contour); Py_INCREF(_new_layer);
Py_RETURN_NONE;
}

static PyObject *PyFFi_newPoint(PyObject *UNUSED(noself), PyObject *args) {
return( PyFFPoint_New(&PyFF_PointType,args,NULL));
}

static PyObject *PyFFi_newContour(PyObject *UNUSED(noself), PyObject *args) {
    PyFF_Contour *self = (PyFF_Contour *) PyFFContour_new(&PyFF_ContourType,NULL,NULL);
    int i, len;

    if ( self==NULL )
return( NULL );
    len = PyTuple_Size(args);
    if ( len<2 ) {
	PyErr_Format(PyExc_TypeError, "Too few arguments");
return( NULL );
    }
    self->is_quadratic = PyLong_AsLong(PyTuple_GetItem(args,0));
    if ( PyErr_Occurred()!=NULL )
return( NULL );
    self->closed = PyLong_AsLong(PyTuple_GetItem(args,1));
    if ( PyErr_Occurred()!=NULL )
return( NULL );
    self->pt_cnt = self->pt_max = len-2;
    self->points = PyMem_New(PyFF_Point *,self->pt_max);
    if ( self->points==NULL )
return( NULL );
    for ( i=0; i<len-2; ++i ) {
	PyObject *obj = PyTuple_GetItem(args,2+i);
	if ( !PyType_IsSubtype(&PyFF_PointType, Py_TYPE(obj)) ) {
	    PyErr_Format(PyExc_TypeError, "Expected FontForge points.");
return( NULL );
	}
	Py_INCREF(obj);
	self->points[i] = (PyFF_Point *) obj;
    }
return( (PyObject *) self );
}

static PyObject *PyFFi_newLayer(PyObject *UNUSED(noself), PyObject *args) {
    PyFF_Layer *self = (PyFF_Layer *) PyFFLayer_new(&PyFF_LayerType,NULL,NULL);
    int i, len;

    if ( self==NULL )
return( NULL );
    len = PyTuple_Size(args);
    if ( len<1 ) {
	PyErr_Format(PyExc_TypeError, "Too few arguments");
return( NULL );
    }
    self->is_quadratic = PyLong_AsLong(PyTuple_GetItem(args,0));
    if ( PyErr_Occurred()!=NULL )
return( NULL );
    self->cntr_cnt = self->cntr_max = len-1;
    self->contours = PyMem_New(PyFF_Contour *,self->cntr_max);
    if ( self->contours==NULL )
return( NULL );
    for ( i=0; i<len-1; ++i ) {
	PyObject *obj = PyTuple_GetItem(args,1+i);
	if ( !PyType_IsSubtype(&PyFF_ContourType, Py_TYPE(obj)) ) {
	    PyErr_Format(PyExc_TypeError, "Expected FontForge Contours.");
return( NULL );
	}
	Py_INCREF(obj);
	self->contours[i] = (PyFF_Contour *) obj;
    }
return( (PyObject *) self );
}

static PyMethodDef module_ff_internals_methods[] = {
    { "initPickles", PyFFi_initPickles, METH_VARARGS, "Set the pickle/unpickle globals so I can call them from C" },
    { "initPickleTypes", PyFFi_initPickleTypes, METH_VARARGS, "Set the some globals so I can call C functions from python" },
    { "newPoint", PyFFi_newPoint, METH_VARARGS, "Top level function to create a new point, needed (I think) for the pickler" },
    { "newContour", PyFFi_newContour, METH_VARARGS, "Top level function to create a new contour, needed (I think) for the pickler" },
    { "newLayer", PyFFi_newLayer, METH_VARARGS, "Top level function to create a new layer, needed (I think) for the pickler" },
    PYMETHODDEF_EMPTY /* Sentinel */
};

static module_definition module_def_ff_internals = {
    "__FontForge_Internals___",            /* module_name */
    "I use this to get access to certain python objects I need, and to hide some internal python functions. I don't expect users ever to care about it.",     /* docstring */
    NULL,                                  /* types */
    module_ff_internals_methods,           /* methods */
    false,                                 /* auto_import */
    NULL,                                  /* finalize_func */
    MODULEDEF_RUNTIMEINFO_INIT
};


void PyFF_ErrorString(const char *msg,const char *str) {
    char *cond = (char *) msg;
    if ( str!=NULL )
	cond = strconcat3(msg, " ", str);
    PyErr_SetString(PyExc_ValueError, cond );
    if ( cond!=msg )
	free(cond);
}

void PyFF_ErrorF3(const char *frmt, const char *str, int size, int depth) {
    PyErr_Format(PyExc_ValueError, frmt, str, size, depth );
}

/* ************************************************************************** */
/* PYTHON INITIALIZATION */
/* ************************************************************************** */

extern int no_windowing_ui, running_script;

static void RegisterAllPyModules(void);
static void CreateAllPyModules(void);

static PyObject * CreatePyModule( module_definition *moddef );
static void SetPythonProgramName( const char *progname /* in ASCII */ );
static void SetPythonModuleMetadata( PyObject *module );
static int FinalizePythonTypes( python_type_info* typelist );
static int AddPythonTypesToModule( PyObject *module, python_type_info* typelist );

#define ff_crmod(name) \
PyMODINIT_FUNC CreatePyModule_##name(void) {\
    return CreatePyModule(&module_def_##name);\
}

/* ===== MODULE REGISTRY -- LIST OF ALL PYTHON MODULES ===== */
static module_definition * all_modules[] = {
    &module_def_fontforge,
    &module_def_psMat,
    &module_def_ff_internals
};

ff_crmod(fontforge)
ff_crmod(psMat)
ff_crmod(ff_internals)

#define NUM_MODULES (sizeof(all_modules) / sizeof(module_definition *))

/* ===== END MODULE REGISTRY ===== */


static const char *spiro_names[] = { "spiroG4", "spiroG2", "spiroCorner",
				     "spiroLeft", "spiroRight", "spiroOpen", NULL };


static int FinalizePythonTypes(python_type_info* typelist) {
    int i=0;

    /* Build types first with custom initializers */
    for ( i=0; typelist[i].typeobj != NULL; ++i ) {
	PyTypeObject * typ = typelist[i].typeobj;
	type_initializer typ_init = typelist[i].setup_function;

	if ( typ != NULL && typ_init != NULL ) {
#ifdef DEBUG
	    fprintf(stderr,"Building type %s\n", typ->tp_name);
#endif
	    if ( typ_init( typ ) < 0 ) {
		fprintf(stderr,"Python initialization failed: setup of type %s failed\n",
			typ->tp_name);
		return -1;
	    }
	}
    }

    /* Let Python make the type ready */
    for ( i=0; typelist[i].typeobj != NULL; ++i ) {
	PyTypeObject * typ = typelist[i].typeobj;

#ifdef DEBUG
	fprintf(stdout,"PyTypeReady(%s)\n", typ->tp_name);
#endif
	if ( PyType_Ready(typ) < 0 ) {
	    fprintf(stderr,"Python initialization failed: PyTypeReady(%s) failed\n",
		    typ->tp_name);
	    return -1;
	}
    }
    return 0;
}

static int AddPythonTypesToModule( PyObject *module, python_type_info* typelist)
{
    int i;
    for ( i=0; typelist[i].typeobj != NULL; ++i ) {
	PyTypeObject * typ = typelist[i].typeobj;
	int do_add = typelist[i].add_to_module;
	const char *typnam;
	const char *dot;

	if ( ! do_add )
	    continue;

	/* Determine the type name, without the dotted module qualification */
	typnam = typ->tp_name;
	dot = strchr(typnam,'.');
	if ( dot!=NULL )
	    typnam = dot+1;

	/* Add the type to the module */
#ifdef DEBUG
	fprintf(stdout,"Adding type %s as %s\n", typ->tp_name, typnam);
#endif
	Py_INCREF( typ );
	PyModule_AddObject( module, typnam, (PyObject*)typ );
    }
    return 0;
}


static PyObject* CreatePyModule( module_definition *mdef ) {
    PyObject *module;

    if ( mdef->runtime.module != NULL )
	return mdef->runtime.module;

    if ( mdef->types != NULL && FinalizePythonTypes( mdef->types ) < 0 )
	return NULL;

    mdef->runtime.pymod_def.m_name = mdef->module_name;
    mdef->runtime.pymod_def.m_doc = mdef->docstring;
    mdef->runtime.pymod_def.m_methods = mdef->methods;
    mdef->runtime.pymod_def.m_size = -1;
#if PY_MAJOR_VERSION > 3 || PY_MINOR_VERSION >= 5
    mdef->runtime.pymod_def.m_slots = NULL;
#else
    mdef->runtime.pymod_def.m_reload = NULL;
#endif
    mdef->runtime.pymod_def.m_traverse = NULL;
    mdef->runtime.pymod_def.m_clear = NULL;
    mdef->runtime.pymod_def.m_free = NULL;
    module = PyModule_Create( &mdef->runtime.pymod_def );
    mdef->runtime.module = module;
    SetPythonModuleMetadata( module );
    if ( mdef->types != NULL )
	AddPythonTypesToModule( module, mdef->types );

    if ( mdef->finalize_func != NULL )
	(mdef->finalize_func)( module );

    return module;
}

static PyObject *InitializePythonMainNamespace(void) {
    static PyObject *module_main = NULL; /* Python's __main__ namespace module */
    unsigned i;

    if ( module_main != NULL )
	return module_main;

    module_main = PyImport_AddModule("__main__");

    /* Pre-import our modules */
    for ( i=0; i<NUM_MODULES; i++ ) {
	if ( all_modules[i]->auto_import ) {
	    const char *modname = all_modules[i]->module_name;
	    if ( ! PyObject_HasAttrString(module_main, modname) ) {
		PyObject *mod;
		mod = PyImport_ImportModule( modname );
		PyModule_AddObject( module_main, modname, mod );
	    }
	}
    }
    return module_main;
}

static void CreateAllPyModules(void) {
    for ( unsigned i=0; i<NUM_MODULES; i++ )
        CreatePyModule( all_modules[i] );
}


static void RegisterAllPyModules(void) {
    /* This adds all the modules to Python's 'builtin' module list.
     * It allows an 'import some_module' to always work, as python
     * knows where to find the module in already-loaded code without
     * having to search the filesystem for module files/libraries.
     * NOTE: This should only be called from the embedded python,
     *       when used from the pyhook, we can't (or shouldn't) do this
     */
    module_def_fontforge.runtime.modinit_func = CreatePyModule_fontforge;
    module_def_psMat.runtime.modinit_func = CreatePyModule_psMat;
    module_def_ff_internals.runtime.modinit_func = CreatePyModule_ff_internals;

    for ( unsigned i=0; i<NUM_MODULES; i++ ) {
	PyImport_AppendInittab( all_modules[i]->module_name,
				all_modules[i]->runtime.modinit_func );
    }
}

static void UnreferenceAllPyModules(void) {
    /* This clears references to the previously created Python modules
     * so that we avoid using them once they become invalid.
     */

    for ( unsigned i=0; i<NUM_MODULES; i++ ) {
	all_modules[i]->runtime.module = NULL;
    }
}

static int python_initialized = 0;

void FontForge_FinalizeEmbeddedPython(void) {
    // static int python_initialized is declared above.
    if ( !python_initialized )
	return;

    Py_Finalize();
    UnreferenceAllPyModules(); // We want to NULL old references.
    python_initialized = 0;
}

/* This is called to start up the embedded python interpreter */
void FontForge_InitializeEmbeddedPython(void) {
    // static int python_initialized is declared above.
    if ( python_initialized )
	return;

    SetPythonProgramName("fontforge");
    RegisterAllPyModules();
    Py_Initialize();
    python_initialized = 1;

    /* The embedded python interpreter is now functionally
     * "running". We can modify it to our needs.
     */
    CreateAllPyModules();
    InitializePythonMainNamespace();
}

static wchar_t ** copy_argv(char *arg0, int argc ,char **argv);

/* PyFF_Main() -- This is called to run a script as the main task, by
 * running the command:   fontforge -script somescript.py arg1 arg2 ...
 *
 * This function is passed the entire original command line.
 * Before passing it to Python, it must eliminate all the
 * options up to and including the '-script' option, but while
 * preserving argv[0].
 */
_Noreturn void PyFF_Main(int argc,char **argv,int start, int do_inits,
                         int do_plugins) {
    char *arg;
    wchar_t **newargv;
    int newargc;
    int exitcode;

    no_windowing_ui = running_script = true;

    FontForge_InitializeEmbeddedPython();
    PyFF_ProcessInitFiles(do_inits, do_plugins);

    /* Skip '-script' option */
    arg = argv[start];
    if ( *arg=='-' && arg[1]=='-' ) ++arg;
    if ( strcmp(arg,"-script")==0 )
	++start;

    /* Make new argv array */
    newargc = argc - start + 1;
    newargv = copy_argv(argv[0], newargc-1, &argv[start] );

    /* Run Python */
    exitcode = Py_Main( newargc, newargv );
    FontForge_FinalizeEmbeddedPython();
    exit(exitcode);
}


/* ************************************************************************** */
/* PYTHON INITIALIZATION */
/* ************************************************************************** */

static void SetPythonProgramName(const char *progname) {
    static wchar_t *saved_progname=NULL;
    if ( saved_progname )
	free(saved_progname);
    saved_progname = copy_to_wide_string(progname);
    Py_SetProgramName(saved_progname);
}

static wchar_t ** copy_argv(char *arg0, int argc ,char **argv) {
    int i;
    wchar_t **newargv;

    newargv= calloc(argc+2,sizeof(char *));
    newargv[0] = copy_to_wide_string(arg0);
    if (newargv[0] == NULL) {
        fprintf(stderr, "argv[0] is an invalid multibyte sequence in the current locale\n");
        exit(1);
    }

    for ( i=0; i<argc; ++i ) {
	newargv[i+1] = copy_to_wide_string(argv[i]);
	if (newargv[i+1] == NULL) {
	    fprintf(stderr, "argv[%d] is an invalid multibyte sequence in the current locale\n",i+1);
	    exit(1);
	}
    }
    newargv[argc+1] = NULL;
    return newargv;
}

/* ************************************************************************** */
/* Other python environment initializations */
/* ************************************************************************** */

static void SetPythonModuleMetadata( PyObject *module ) {
    PyObject* pyver;
    PyObject* pydate;
    time_t dt = FONTFORGE_MODTIME_RAW;
    struct tm* modtime = gmtime(&dt);

    /* Make __version__ string */
    pyver = PyUnicode_FromFormat("%s git:%s",
	     FONTFORGE_VERSION,
	     FONTFORGE_GIT_VERSION );
    PyModule_AddObject(module, "__version__", pyver);

    /* Make __date__ string */
    pydate = PyUnicode_FromFormat("%04d-%02d-%02d",
	     modtime->tm_year+1900, modtime->tm_mon+1, modtime->tm_mday );
    PyModule_AddObject(module, "__date__", pydate);
}

static void AddHookDictionary( PyObject *module ) {
    /* Add a dictionary in which the user may define hooks (scripts to
     * run) when certain events happen in fontforge, like loading a
     * file.
     */
    PyObject *hook_dict;
    hook_dict = PyDict_New();
    Py_INCREF(hook_dict);
    PyModule_AddObject(module, "hooks", hook_dict);
}

static void AddSpiroConstants( PyObject *module ) {
    int i;
    /* Add constant names for the spiro point types */
    for ( i=0; spiro_names[i]!=NULL; ++i )
        PyModule_AddObject(module, spiro_names[i], Py_BuildValue("i",i+1));
}


_Noreturn void PyFF_Stdin(int do_inits, int do_plugins) {
    no_windowing_ui = running_script = true;

    FontForge_InitializeEmbeddedPython();
    PyFF_ProcessInitFiles(do_inits, do_plugins);

    if ( isatty(fileno(stdin)))
	PyRun_InteractiveLoop(stdin,"<stdin>");
    else
	PyRun_SimpleFile(stdin,"<stdin>");

    FontForge_FinalizeEmbeddedPython();
    exit(0);
}


void PyFF_ScriptFile(FontViewBase *fv,SplineChar *sc, char *filename) {
    FILE *fp;
    int rc;

    fp = fopen(filename, "rb");
    if ( fp==NULL ) {
	fprintf(stderr, "Failed to open script \"%s\": %s\n", filename, strerror(errno));
	LogError(_("Can't open %s"), filename );
	return;
    }

    fv_active_in_ui = fv;		/* Make fv known to interpreter */
    sc_active_in_ui = sc;		/* Make sc known to interpreter */
    layer_active_in_ui = ly_fore;
    if ( fv!=NULL )
	layer_active_in_ui = fv->active_layer;

    rc = PyRun_SimpleFileEx(fp, filename, 1/*close fp*/);
    if ( rc != 0 ) {
	LogError(_("Execution of script %s failed"), filename );
    }
}

void PyFF_ScriptString(FontViewBase *fv,SplineChar *sc, int layer, char *str) {

    fv_active_in_ui = fv;		/* Make fv known to interpreter */
    sc_active_in_ui = sc;		/* Make sc known to interpreter */
    layer_active_in_ui = layer;
    if ( sc!=NULL )
	PyFF_Glyph_Set_Layer(sc,layer);
    PyRun_SimpleString(str);
}

void PyFF_FreeFV(FontViewBase *fv) {
    if ( fv->python_fv_object!=NULL ) {
	((PyFF_Font *) (fv->python_fv_object))->fv = NULL;
	Py_DECREF( (PyObject *) (fv->python_fv_object));
    }
}

void PyFF_FreeSF(SplineFont *sf) {
    Py_XDECREF( (PyObject *) (sf->python_persistent));
    Py_XDECREF( (PyObject *) (sf->python_temporary));
}

void PyFF_FreeSC(SplineChar *sc) {
    if ( sc->python_sc_object!=NULL ) {
	((PyFF_Glyph *) (sc->python_sc_object))->sc = NULL;
	Py_DECREF( (PyObject *) (sc->python_sc_object));
    }
#if 0
    // This is now layer-specific.
    Py_XDECREF( (PyObject *) (sc->python_persistent));
#endif // 0
    Py_XDECREF( (PyObject *) (sc->python_temporary));
}

void PyFF_FreeSCLayer(SplineChar *sc, int layer) {
    Py_XDECREF( (PyObject *) (sc->layers[layer].python_persistent));
}

extern void PyFF_FreePythonPersistent(void *python_persistent) {
    Py_XDECREF((PyObject *)python_persistent);
}

static gint GPtrArrayStrcmp(gconstpointer a, gconstpointer b) {
    return strcmp(*(const char**)a, *(const char**)b);
}

static void LoadFilesInPythonInitDir(char *dir) {
    DIR *diro;
    struct dirent *ent;
    GPtrArray *filelist;

    diro = opendir(dir);
    if ( diro==NULL )		/* It's ok not to have any python init scripts */
return;

    filelist = g_ptr_array_new_with_free_func(free);

    while ( (ent = readdir(diro))!=NULL ) {
	char *pt = strrchr(ent->d_name,'.');
	if ( pt==NULL )
    continue;
	if ( strcmp(pt,".py")==0 ) {
        g_ptr_array_add(filelist, smprintf("%s/%s", dir, ent->d_name));
	}
    }
    closedir(diro);

    g_ptr_array_sort(filelist, GPtrArrayStrcmp);

    showPythonErrors = 0;
    for (guint i = 0; i < filelist->len; ++i) {
	FILE *fp;
	char *pathname = (char*)filelist->pdata[i];
	fp = fopen( pathname, "rb" );
	if ( fp==NULL ) {
	    fprintf(stderr,"Failed to open script \"%s\": %s\n",pathname,strerror(errno));
	    continue;
	}
	PyRun_SimpleFileEx(fp, pathname, 1/*close fp*/);
    }
    showPythonErrors = 1;
    g_ptr_array_free(filelist, true);
}

static int dir_exists(const char* path) {
    struct stat st;
    if ( stat(path,&st)==0 && S_ISDIR(st.st_mode) )
	return 1;
    return 0;
}

static GPtrArray *default_pyinit_dirs(void) {
    GPtrArray *pathlist;
    const char *sharedir;
    const char *userdir;
    char subdir[16];
    char *buffer;

    pathlist = g_ptr_array_new_with_free_func(free);
    snprintf(subdir, sizeof(subdir), "python%d", PY_MAJOR_VERSION);

    sharedir = getFontForgeShareDir();
    userdir = getFontForgeUserDir(Config);

    if ( sharedir!=NULL ) {
        buffer = smprintf("%s/%s", sharedir, subdir);
        if ( dir_exists(buffer) ) {
            g_ptr_array_add(pathlist, buffer);
        }
        else { /* Fall back to version-less python */
            free(buffer);
            buffer = smprintf("%s/%s", sharedir, "python");
            if ( dir_exists(buffer) ) {
                g_ptr_array_add(pathlist, buffer);
            } else {
                free(buffer);
            }
        }
    }

    if ( userdir!=NULL ) {
        buffer = smprintf("%s/%s", userdir, subdir);
        if ( dir_exists(buffer) ) {
            g_ptr_array_add(pathlist, buffer);
        }
        else { /* Fall back to version-less python */
            free(buffer);
            buffer = smprintf("%s/%s", userdir, "python");
            if ( dir_exists(buffer) ) {
                g_ptr_array_add(pathlist, buffer);
            } else {
                free(buffer);
            }
        }
    }

    return pathlist;
}

void PyFF_ProcessInitFiles(int do_inits, int do_plugins) {
    static int done = false;
    GPtrArray *dpath;

    PyFF_ImportPlugins(do_plugins);

    if ( done || !do_inits ) // Idempotency, I presume
	return;

    dpath = default_pyinit_dirs();
    for (guint i = 0; i < dpath->len; ++i ) {
	LoadFilesInPythonInitDir( (char*)dpath->pdata[i] );
    }
    g_ptr_array_free(dpath, true);
    done = true;
}

void PyFF_CallDictFunc(PyObject *dict,const char *key,const char *argtypes, ... ) {
    PyObject *func, *arglist, *result;
    const char *pt;
    va_list ap;
    int i;

    if ( dict==NULL || !PyMapping_Check(dict) ||
	 !PyMapping_HasKeyString(dict,(char *)key) ||
	 (func = PyMapping_GetItemString(dict,(char *)key))==NULL )
return;
    if ( !PyCallable_Check(func)) {
	LogError(_("%s: Is not callable"), key );
	Py_DECREF(func);
return;
    }
    va_start(ap,argtypes);

    arglist = PyTuple_New(strlen(argtypes));
    for ( pt=argtypes, i=0; *pt; ++pt, ++i ) {
	PyObject *arg;
	if ( *pt=='f' )
	    arg = PyFF_FontForFV_I( va_arg(ap,FontViewBase *));
	else if ( *pt=='g' )
	    arg = PySC_From_SC_I( va_arg(ap,SplineChar *));
	else if ( *pt=='s' )
	    arg = Py_BuildValue("s", va_arg(ap, char *));
	else if ( *pt=='i' )
	    arg = Py_BuildValue("i", va_arg(ap, int));
	else if ( *pt=='n' ) {
	    arg = Py_None;
	    Py_INCREF(arg);
	} else {
	    IError("Unknown argument type in CallDictFunc" );
	    arg = Py_None;
	    Py_INCREF(arg);
	}
	PyTuple_SetItem(arglist,i,arg);
    }
    va_end(ap);
    result = PyObject_CallObject(func, arglist);
    Py_DECREF(arglist);
    Py_XDECREF(result);
    if ( PyErr_Occurred()!=NULL )
	PyErr_Print();
}

void PyFF_InitFontHook(FontViewBase *fv) {
    /* Ok we just created a new fontview, and attached it to a splinefont */
    /*  We have not added a window or menu to it yet */
    SplineFont *sf = fv->sf;
    PyObject *obj;

    if ( fv->nextsame!=NULL )		/* Duplicate window looking at previously loaded font */
return;

    fv_active_in_ui = fv;		/* Make fv known to interpreter */
    layer_active_in_ui = fv->active_layer;

    /* First check if it has a initScriptString in the persistent dictionary */
    /* (If we loaded from an sfd file) */
    obj = NULL;
    if ( sf->python_persistent!=NULL && PyMapping_Check(sf->python_persistent) &&
	 PyMapping_HasKeyString(sf->python_persistent,(char *)"initScriptString") &&
	 (obj = PyMapping_GetItemString(sf->python_persistent,(char *)"initScriptString"))!=NULL &&
	 PyUnicode_Check(obj)) {
	const char *str = PyUnicode_AsUTF8(obj);
	if (str == NULL) {
	    Py_DECREF(obj);
	    return;
	}
	PyRun_SimpleString(str);
    }
    Py_XDECREF(obj);

    if ( sf->new )
	PyFF_CallDictFunc(hook_dict,"newFontHook","f", fv );
    else
	PyFF_CallDictFunc(hook_dict,"loadFontHook","f", fv );
}


/* This function is called when fontforge is being imported into
** a python process.  Actually python first invokes the wrapper
** functions in the pyhook/ *.c files; and those then call this
** function.
*/
PyMODINIT_FUNC fontforge_python_init(const char* modulename) {
    static int initted = false;

    if (!initted) {
        doinitFontForgeMain();
        no_windowing_ui = running_script = true;

        CreateAllPyModules();

        // Register the internal module
        PyObject* modules = PySys_GetObject("modules");
        PyObject* ref = PyDict_GetItemString(modules, module_def_ff_internals.module_name);
        if (ref == NULL) {
            PyDict_SetItemString(modules, module_def_ff_internals.module_name, module_def_ff_internals.runtime.module);
        }

        initted = true;
    }

    for ( unsigned i=0; i<NUM_MODULES; i++ )
	if (strcmp(all_modules[i]->module_name, modulename)==0 )
	    return all_modules[i]->runtime.module;
    return NULL;
}

#else
#include "flaglist.h"
#include "fontforgevw.h"
#endif		/* _NO_PYTHON */

/* These don't get translated. They are a copy of a similar list in fontinfo.c */
static struct flaglist sfnt_name_str_ids[] = {
    { "SubFamily", 2},
    { "Copyright", 0},
    { "Family", 1},
    { "Fullname", 4},
    { "UniqueID", 3},
    { "Version", 5},
    { "PostScriptName", 6},
    { "Trademark", 7},
    { "Manufacturer", 8},
    { "Designer", 9},
    { "Descriptor", 10},
    { "Vendor URL", 11},
    { "Designer URL", 12},
    { "License", 13},
    { "License URL", 14},
/* slot 15 is reserved */
    { "Preferred Family", 16},
    { "Preferred Styles", 17},
    { "Compatible Full", 18},
    { "Sample Text", 19},
    { "CID findfont Name", 20},
    { "WWS Family", 21},
    { "WWS Subfamily", 22},
    FLAGLIST_EMPTY /* Sentinel */
};
/* These don't get translated. They are a copy of a similar list in fontinfo.c */
static struct flaglist sfnt_name_mslangs[] = {
    { "Afrikaans", 0x436},
    { "Albanian", 0x41c},
    { "Amharic", 0x45e},
    { "Arabic (Saudi Arabia)", 0x401},
    { "Arabic (Iraq)", 0x801},
    { "Arabic (Egypt)", 0xc01},
    { "Arabic (Libya)", 0x1001},
    { "Arabic (Algeria)", 0x1401},
    { "Arabic (Morocco)", 0x1801},
    { "Arabic (Tunisia)", 0x1C01},
    { "Arabic (Oman)", 0x2001},
    { "Arabic (Yemen)", 0x2401},
    { "Arabic (Syria)", 0x2801},
    { "Arabic (Jordan)", 0x2c01},
    { "Arabic (Lebanon)", 0x3001},
    { "Arabic (Kuwait)", 0x3401},
    { "Arabic (U.A.E.)", 0x3801},
    { "Arabic (Bahrain)", 0x3c01},
    { "Arabic (Qatar)", 0x4001},
    { "Armenian", 0x42b},
    { "Assamese", 0x44d},
    { "Azeri (Latin)", 0x42c},
    { "Azeri (Cyrillic)", 0x82c},
    { "Basque", 0x42d},
    { "Byelorussian", 0x423},
    { "Bengali", 0x445},
    { "Bengali Bangladesh", 0x845},
    { "Bulgarian", 0x402},
    { "Burmese", 0x455},
    { "Catalan", 0x403},
    { "Cambodian", 0x453},
    { "Cherokee", 0x45c},
    { "Chinese (Taiwan)", 0x404},
    { "Chinese (PRC)", 0x804},
    { "Chinese (Hong Kong)", 0xc04},
    { "Chinese (Singapore)", 0x1004},
    { "Chinese (Macau)", 0x1404},
    { "Croatian", 0x41a},
    { "Croatian Bosnia/Herzegovina", 0x101a},
    { "Czech", 0x405},
    { "Danish", 0x406},
    { "Divehi", 0x465},
    { "Dutch", 0x413},
    { "Flemish (Belgian Dutch)", 0x813},
    { "Edo", 0x466},
    { "English (British)", 0x809},
    { "English (US)", 0x409},
    { "English (Canada)", 0x1009},
    { "English (Australian)", 0xc09},
    { "English (New Zealand)", 0x1409},
    { "English (Irish)", 0x1809},
    { "English (South Africa)", 0x1c09},
    { "English (Jamaica)", 0x2009},
    { "English (Caribbean)", 0x2409},
    { "English (Belize)", 0x2809},
    { "English (Trinidad)", 0x2c09},
    { "English (Zimbabwe)", 0x3009},
    { "English (Philippines)", 0x3409},
    { "English (Indonesia)", 0x3809},
    { "English (Hong Kong)", 0x3c09},
    { "English (India)", 0x4009},
    { "English (Malaysia)", 0x4409},
    { "Estonian", 0x425},
    { "Faeroese", 0x438},
    { "Farsi", 0x429},
    { "Filipino", 0x464},
    { "Finnish", 0x40b},
    { "French French", 0x40c},
    { "French Belgium", 0x80c},
    { "French Canadian", 0xc0c},
    { "French Swiss", 0x100c},
    { "French Luxembourg", 0x140c},
    { "French Monaco", 0x180c},
    { "French West Indies", 0x1c0c},
    { "French Réunion", 0x200c},
    { "French D.R. Congo", 0x240c},
    { "French Senegal", 0x280c},
    { "French Camaroon", 0x2c0c},
    { "French Côte d'Ivoire", 0x300c},
    { "French Mali", 0x340c},
    { "French Morocco", 0x380c},
    { "French Haiti", 0x3c0c},
    { "French North Africa", 0xe40c},
    { "Frisian", 0x462},
    { "Fulfulde", 0x467},
    { "Gaelic (Scottish)", 0x43c},
    { "Gaelic (Irish)", 0x83c},
    { "Galician", 0x467},
    { "Georgian", 0x437},
    { "German German", 0x407},
    { "German Swiss", 0x807},
    { "German Austrian", 0xc07},
    { "German Luxembourg", 0x1007},
    { "German Liechtenstein", 0x1407},
    { "Greek", 0x408},
    { "Guarani", 0x474},
    { "Gujarati", 0x447},
    { "Hausa", 0x468},
    { "Hawaiian", 0x475},
    { "Hebrew", 0x40d},
    { "Hindi", 0x439},
    { "Hungarian", 0x40e},
    { "Ibibio", 0x469},
    { "Icelandic", 0x40f},
    { "Igbo", 0x470},
    { "Indonesian", 0x421},
    { "Inuktitut", 0x45d},
    { "Italian", 0x410},
    { "Italian Swiss", 0x810},
    { "Japanese", 0x411},
    { "Kannada", 0x44b},
    { "Kanuri", 0x471},
    { "Kashmiri (India)", 0x860},
    { "Kazakh", 0x43f},
    { "Khmer", 0x453},
    { "Kirghiz", 0x440},
    { "Konkani", 0x457},
    { "Korean", 0x412},
    { "Korean (Johab)", 0x812},
    { "Lao", 0x454},
    { "Latvian", 0x426},
    { "Latin", 0x476},
    { "Lithuanian", 0x427},
    { "Lithuanian (Classic)", 0x827},
    { "Macedonian", 0x42f},
    { "Malay", 0x43e},
    { "Malay (Brunei)", 0x83e},
    { "Malayalam", 0x44c},
    { "Maltese", 0x43a},
    { "Manipuri", 0x458},
    { "Maori", 0x481},
    { "Marathi", 0x44e},
    { "Mongolian (Cyrillic)", 0x450},
    { "Mongolian (Mongolian)", 0x850},
    { "Nepali", 0x461},
    { "Nepali (India)", 0x861},
    { "Norwegian (Bokmal)", 0x414},
    { "Norwegian (Nynorsk)", 0x814},
    { "Oriya", 0x448},
    { "Oromo", 0x472},
    { "Papiamentu", 0x479},
    { "Pashto", 0x463},
    { "Polish", 0x415},
    { "Portuguese (Portugal)", 0x416},
    { "Portuguese (Brasil)", 0x816},
    { "Punjabi (India)", 0x446},
    { "Punjabi (Pakistan)", 0x846},
    { "Quecha (Bolivia)", 0x46b},
    { "Quecha (Ecuador)", 0x86b},
    { "Quecha (Peru)", 0xc6b},
    { "Rhaeto-Romanic", 0x417},
    { "Romanian", 0x418},
    { "Romanian (Moldova)", 0x818},
    { "Russian", 0x419},
    { "Russian (Moldova)", 0x819},
    { "Sami (Lappish)", 0x43b},
    { "Sanskrit", 0x44f},
    { "Sepedi", 0x46c},
    { "Serbian (Cyrillic)", 0xc1a},
    { "Serbian (Latin)", 0x81a},
    { "Sindhi India", 0x459},
    { "Sindhi Pakistan", 0x859},
    { "Sinhalese", 0x45b},
    { "Slovak", 0x41b},
    { "Slovenian", 0x424},
    { "Sorbian", 0x42e},
    { "Spanish (Traditional)", 0x40a},
    { "Spanish Mexico", 0x80a},
    { "Spanish (Modern)", 0xc0a},
    { "Spanish (Guatemala)", 0x100a},
    { "Spanish (Costa Rica)", 0x140a},
    { "Spanish (Panama)", 0x180a},
    { "Spanish (Dominican Republic)", 0x1c0a},
    { "Spanish (Venezuela)", 0x200a},
    { "Spanish (Colombia)", 0x240a},
    { "Spanish (Peru)", 0x280a},
    { "Spanish (Argentina)", 0x2c0a},
    { "Spanish (Ecuador)", 0x300a},
    { "Spanish (Chile)", 0x340a},
    { "Spanish (Uruguay)", 0x380a},
    { "Spanish (Paraguay)", 0x3c0a},
    { "Spanish (Bolivia)", 0x400a},
    { "Spanish (El Salvador)", 0x440a},
    { "Spanish (Honduras)", 0x480a},
    { "Spanish (Nicaragua)", 0x4c0a},
    { "Spanish (Puerto Rico)", 0x500a},
    { "Spanish (United States)", 0x540a},
    { "Spanish (Latin America)", 0xe40a},
    { "Sutu", 0x430},
    { "Swahili (Kenyan)", 0x441},
    { "Swedish (Sweden)", 0x41d},
    { "Swedish (Finland)", 0x81d},
    { "Syriac", 0x45a},
    { "Tagalog", 0x464},
    { "Tajik", 0x428},
    { "Tamazight (Arabic)", 0x45f},
    { "Tamazight (Latin)", 0x85f},
    { "Tamil", 0x449},
    { "Tatar (Tatarstan)", 0x444},
    { "Telugu", 0x44a},
    { "Thai", 0x41e},
    { "Tibetan (PRC)", 0x451},
    { "Tibetan Bhutan", 0x851},
    { "Tigrinya Ethiopia", 0x473},
    { "Tigrinyan Eritrea", 0x873},
    { "Tsonga", 0x431},
    { "Tswana", 0x432},
    { "Turkish", 0x41f},
    { "Turkmen", 0x442},
    { "Uighur", 0x480},
    { "Ukrainian", 0x422},
    { "Urdu (Pakistan)", 0x420},
    { "Urdu (India)", 0x820},
    { "Uzbek (Latin)", 0x443},
    { "Uzbek (Cyrillic)", 0x843},
    { "Venda", 0x433},
    { "Vietnamese", 0x42a},
    { "Welsh", 0x452},
    { "Xhosa", 0x434},
    { "Yi", 0x478},
    { "Yiddish", 0x43d},
    { "Yoruba", 0x46a},
    { "Zulu", 0x435},
    FLAGLIST_EMPTY /* Sentinel */
};

const char *NOUI_TTFNameIds(int id) {
    int i;

    for ( i=0; sfnt_name_str_ids[i].name!=NULL; ++i )
	if ( sfnt_name_str_ids[i].flag == id )
return( (char *) sfnt_name_str_ids[i].name );

return( _("Unknown") );
}

const char *NOUI_MSLangString(int language) {
    int i;

    for ( i=0; sfnt_name_mslangs[i].name!=NULL; ++i )
	if ( sfnt_name_mslangs[i].flag == language )
return( (char *) sfnt_name_mslangs[i].name );

    language &= 0xff;
    for ( i=0; sfnt_name_mslangs[i].name!=NULL; ++i )
	if ( sfnt_name_mslangs[i].flag == language )
return( (char *) sfnt_name_mslangs[i].name );

return( _("Unknown") );
}
