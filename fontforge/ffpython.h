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

#include "flaglist.h"

/*********** PYTHON 3 **********/
#if PY_MAJOR_VERSION >= 3

#define PyInt_Check    PyLong_Check
#define PyInt_AsLong   PyLong_AsLong
#define PyInt_FromLong PyLong_FromLong

#define ANYSTRING_CHECK(obj) (PyUnicode_Check(obj))
#define STRING_CHECK   PyUnicode_Check
#define STRING_TO_PY   PyUnicode_FromString
#define DECODE_UTF8(s, size, errors) PyUnicode_DecodeUTF8(s, size, errors)
#define PYBYTES_UTF8(str)            PyUnicode_AsUTF8String(str)
#define STRING_FROM_FORMAT           PyUnicode_FromFormat

#define PICKLE "pickle"

#else /* PY_MAJOR_VERSION >= 3 */
/*********** PYTHON 2 **********/

#define ANYSTRING_CHECK(obj) ( PyUnicode_Check(obj) || PyString_Check(obj) )
#define STRING_CHECK   PyBytes_Check
#define STRING_TO_PY   PyBytes_FromString
#define DECODE_UTF8(s, size, errors) PyString_Decode(s, size, "UTF-8", errors)
#define PYBYTES_UTF8(str)            PyString_AsEncodedObject(str, "UTF-8", NULL)
#define STRING_FROM_FORMAT           PyBytes_FromFormat

#define PICKLE "cPickle"

#endif /* PY_MAJOR_VERSION >= 3 */

/*********** Common **********/
#ifndef Py_TYPE
#define Py_TYPE(ob) (((PyObject*)(ob))->ob_type)
#endif

#ifndef Py_TYPENAME
#define Py_TYPENAME(ob) (((PyObject*)(ob))->ob_type->tp_name)
#endif

#if !defined( Py_RETURN_NONE )
/* Not defined before 2.4 */
# define Py_RETURN_NONE		return( Py_INCREF(Py_None), Py_None )
#endif
#define Py_RETURN(self)		return( Py_INCREF((PyObject *) (self)), (PyObject *) (self) )

#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif

#ifndef PyVarObject_HEAD_INIT
    #define PyVarObject_HEAD_INIT(type, size) \
        PyObject_HEAD_INIT(type) size,
#endif


extern char* AnyPyString_to_UTF8( PyObject* );

#if PY_MAJOR_VERSION < 2 || (PY_MAJOR_VERSION == 2 && PY_MINOR_VERSION <= 7)
#define PyBytesObject PyStringObject
#define PyBytes_Type PyString_Type

#define PyBytes_Check PyString_Check
#define PyBytes_CheckExact PyString_CheckExact 
#define PyBytes_CHECK_INTERNED PyString_CHECK_INTERNED
#define PyBytes_AS_STRING PyString_AS_STRING
#define PyBytes_GET_SIZE PyString_GET_SIZE
#define Py_TPFLAGS_BYTES_SUBCLASS Py_TPFLAGS_STRING_SUBCLASS

#define PyBytes_FromStringAndSize PyString_FromStringAndSize
#define PyBytes_FromString PyString_FromString
#define PyBytes_FromFormatV PyString_FromFormatV
#define PyBytes_FromFormat PyString_FromFormat
#define PyBytes_Size PyString_Size
#define PyBytes_AsString PyString_AsString
#define PyBytes_Repr PyString_Repr
#define PyBytes_Concat PyString_Concat
#define PyBytes_ConcatAndDel PyString_ConcatAndDel
#define _PyBytes_Resize _PyString_Resize
#define _PyBytes_Eq _PyString_Eq
#define PyBytes_Format PyString_Format
#define _PyBytes_FormatLong _PyString_FormatLong
#define PyBytes_DecodeEscape PyString_DecodeEscape
#define _PyBytes_Join _PyString_Join
#define PyBytes_Decode PyString_Decode
#define PyBytes_Encode PyString_Encode
#define PyBytes_AsEncodedObject PyString_AsEncodedObject
#define PyBytes_AsEncodedString PyString_AsEncodedString
#define PyBytes_AsDecodedObject PyString_AsDecodedObject
#define PyBytes_AsDecodedString PyString_AsDecodedString
#define PyBytes_AsStringAndSize PyString_AsStringAndSize
#define _PyBytes_InsertThousandsGrouping _PyString_InsertThousandsGrouping
#endif

#if PY_MAJOR_VERSION < 2 || (PY_MAJOR_VERSION == 2 && PY_MINOR_VERSION <= 4)
typedef int Py_ssize_t;
#endif

extern SplineChar *sc_active_in_ui;
extern FontViewBase *fv_active_in_ui;
extern int layer_active_in_ui;

extern void FfPy_Replace_MenuItemStub(PyObject *(*func)(PyObject *,PyObject *));
extern PyObject *PySC_From_SC(SplineChar *sc);
extern PyObject *PyFV_From_FV(FontViewBase *fv);
extern int FlagsFromTuple(PyObject *tuple,struct flaglist *flags,const char *flagkind);
extern void PyFF_Glyph_Set_Layer(SplineChar *sc,int layer);


/********************************************************************************/
/** Allow both python.c and pythonui.c to aceess the python objects.          ***/
/********************************************************************************/

/* Other sentinel values for end-of-array initialization */
#define PYMETHODDEF_EMPTY  { NULL, NULL, 0, NULL }
#define PYGETSETDEF_EMPTY { NULL, NULL, NULL, NULL, NULL }

typedef struct ff_point {
    PyObject_HEAD
    /* Type-specific fields go here. */
    float x,y;
    uint8 on_curve;
    uint8 selected;
} PyFF_Point;
static PyTypeObject PyFF_PointType;

typedef struct ff_contour {
    PyObject_HEAD
    /* Type-specific fields go here. */
    int pt_cnt, pt_max;
    struct ff_point **points;
    short is_quadratic, closed;		/* bit flags, but access to short is faster */
    spiro_cp *spiros;
    int spiro_cnt;
    char *name;
} PyFF_Contour;
static PyTypeObject PyFF_ContourType;

typedef struct ff_layer {
    PyObject_HEAD
    /* Type-specific fields go here. */
    short cntr_cnt, cntr_max;
    struct ff_contour **contours;
    int is_quadratic;		/* bit flags, but access to int is faster */
} PyFF_Layer;
static PyTypeObject PyFF_LayerType;

typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    SplineChar *sc;
    uint8 replace;
    uint8 ended;
    uint8 changed;
    int layer;
} PyFF_GlyphPen;
static PyTypeObject PyFF_GlyphPenType;

typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    SplineChar *sc;
} PyFF_LayerArray;
static PyTypeObject PyFF_LayerArrayType;

typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    SplineChar *sc;
} PyFF_RefArray;
static PyTypeObject PyFF_RefArrayType;

typedef struct glyphmathkernobject {
    PyObject_HEAD
    SplineChar *sc;
} PyFF_MathKern;
static PyTypeObject PyFF_MathKernType;

typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    SplineChar *sc;
    PyFF_LayerArray *layers;
    PyFF_RefArray *refs;
    PyFF_MathKern *mk;
    int layer;
} PyFF_Glyph;
static PyTypeObject PyFF_GlyphType;

typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    SplineFont *sf;
    int layer;
} PyFF_LayerInfo;
static PyTypeObject PyFF_LayerInfoType;

typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    SplineFont *sf;
} PyFF_LayerInfoArray;
static PyTypeObject PyFF_LayerInfoArrayType;

typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    SplineFont *sf;
    FontViewBase *fv;
} PyFF_Private;
static PyTypeObject PyFF_PrivateType;

typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    FontViewBase *fv;
    int by_glyphs;
} PyFF_Selection;
static PyTypeObject PyFF_SelectionType;

typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    SplineFont *sf;
    struct ttf_table *cvt;
} PyFF_Cvt;
static PyTypeObject PyFF_CvtType;

typedef struct fontmathobject {
    PyObject_HEAD
    SplineFont *sf;
} PyFF_Math;
static PyTypeObject PyFF_MathType;

typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    FontViewBase *fv;
    PyFF_LayerInfoArray *layers;
    PyFF_Private *private;
    PyFF_Cvt *cvt;
    PyFF_Selection *selection;
    PyFF_Math *math;
} PyFF_Font;
static PyTypeObject PyFF_FontType;

extern PyMethodDef PyFF_Font_methods[];
PyObject *PyFV_From_FV_I(FontViewBase *fv);

// return is really a CharView*
typedef void* (*pyFF_maybeCallCVPreserveState_Func_t)( PyFF_Glyph *self );
pyFF_maybeCallCVPreserveState_Func_t get_pyFF_maybeCallCVPreserveState_Func( void );
void set_pyFF_maybeCallCVPreserveState_Func( pyFF_maybeCallCVPreserveState_Func_t f );

typedef void (*pyFF_sendRedoIfInSession_Func_t)( void* cv );
pyFF_sendRedoIfInSession_Func_t get_pyFF_sendRedoIfInSession_Func( void );
void set_pyFF_sendRedoIfInSession_Func( pyFF_sendRedoIfInSession_Func_t f );

