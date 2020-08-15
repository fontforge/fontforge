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

#ifndef FONTFORGE_FFPYTHON_H
#define FONTFORGE_FFPYTHON_H

#include "flaglist.h"

#pragma push_macro("real")
#undef real
#define real py_real

#include <Python.h>
#include <structmember.h>

#undef real
#pragma pop_macro("real")

/*********** Common **********/
#ifndef Py_TYPE
#define Py_TYPE(ob) (((PyObject*)(ob))->ob_type)
#endif

#ifndef Py_TYPENAME
#define Py_TYPENAME(ob) (((PyObject*)(ob))->ob_type->tp_name)
#endif

#define Py_RETURN(self)		return( Py_INCREF((PyObject *) (self)), (PyObject *) (self) )

#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif

#ifndef PyVarObject_HEAD_INIT
    #define PyVarObject_HEAD_INIT(type, size) \
        PyObject_HEAD_INIT(type) size,
#endif

extern SplineChar *sc_active_in_ui;
extern FontViewBase *fv_active_in_ui;
extern int layer_active_in_ui;

extern void FfPy_Replace_MenuItemStub(PyObject *(*func)(PyObject *,PyObject *));
extern int PyFF_ConvexNibID(const char *);
extern PyObject *PySC_From_SC(SplineChar *sc);
extern PyObject *PyFV_From_FV(FontViewBase *fv);
extern int FlagsFromTuple(PyObject *tuple,struct flaglist *flags,const char *flagkind);
extern void PyFF_Glyph_Set_Layer(SplineChar *sc,int layer);


/********************************************************************************/
/** Allow both python.c and pythonui.c to access the python objects.          ***/
/********************************************************************************/

/* Other sentinel values for end-of-array initialization */
#define PYMETHODDEF_EMPTY  { NULL, NULL, 0, NULL }
#define PYGETSETDEF_EMPTY { NULL, NULL, NULL, NULL, NULL }

typedef struct ff_point {
    PyObject_HEAD
    /* Type-specific fields go here. */
    double x,y;
    uint8 on_curve;
    uint8 selected;
    uint8 type;
    uint8 interpolated;
    char *name;
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
extern PyTypeObject PyFF_ContourType;

typedef struct ff_layer {
    PyObject_HEAD
    /* Type-specific fields go here. */
    short cntr_cnt, cntr_max;
    struct ff_contour **contours;
    int is_quadratic;		/* bit flags, but access to int is faster */
} PyFF_Layer;
extern PyTypeObject PyFF_LayerType;

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
extern PyMethodDef module_fontforge_methods[];

PyObject* PyFF_FontForFV(FontViewBase *fv);
PyObject* PyFF_FontForFV_I(FontViewBase *fv);

#endif /* FONTFORGE_FFPYTHON_H */
