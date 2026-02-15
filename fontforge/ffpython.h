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
#include "splinefont.h"
#include "baseviews.h"

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

/* These variables are defined in activeinui.c (compiled as C), so they need
 * extern "C" linkage when referenced from C++ code like python.c */
#ifdef __cplusplus
extern "C" {
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
#ifdef __cplusplus
}
#endif


/********************************************************************************/
/** Allow both python.c and pythonui.c to access the python objects.          ***/
/********************************************************************************/

/* Other sentinel values for end-of-array initialization */
#define PYMETHODDEF_EMPTY  { NULL, NULL, 0, NULL }
#define PYGETSETDEF_EMPTY { NULL, NULL, NULL, NULL, NULL }

typedef struct ff_glyph PyFF_Glyph;
typedef struct ff_font PyFF_Font;

typedef struct ff_point {
    PyObject_HEAD
    /* Type-specific fields go here. */
    double x,y;
    uint8_t on_curve;
    uint8_t selected;
    uint8_t type;
    uint8_t interpolated;
    char *name;
} PyFF_Point;

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
    PyFF_Glyph *glyph;
    uint8_t replace;
    uint8_t ended;
    uint8_t changed;
    int layer;
} PyFF_GlyphPen;

typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    PyFF_Glyph *glyph;
} PyFF_LayerArray;

typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    PyFF_Glyph *glyph;
} PyFF_RefArray;

typedef struct glyphmathkernobject {
    PyObject_HEAD
    PyFF_Glyph *glyph;
} PyFF_MathKern;

typedef struct ff_glyph {
    PyObject_HEAD
    /* Type-specific fields go here. */
    void *sc_opaque; // Use PyFF_Glyph_GetSC() to access this pointer, never use it directly
    PyFF_LayerArray *layers;
    PyFF_RefArray *refs;
    PyFF_MathKern *mk;
    int layer;
} PyFF_Glyph;

typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    PyFF_Font *font;
    int layer;
} PyFF_LayerInfo;

typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    PyFF_Font *font;
} PyFF_LayerInfoArray;

typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    PyFF_Font *font;
} PyFF_Private;

typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    PyFF_Font *font;
    int by_glyphs;
} PyFF_Selection;

typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    PyFF_Font *font;
    struct ttf_table *cvt;
} PyFF_Cvt;

typedef struct fontmathobject {
    PyObject_HEAD
    PyFF_Font *font;
} PyFF_Math;

/* This Python object is a view into SplineFont::MATH DeviceTable objects.
   It can't exist separately and will probably crash if accessed after the
   font has been closed or deleted. */
typedef struct fontmathdevicetableobject {
    PyObject_HEAD
    PyFF_Font *font;
    int devtab_offset;
} PyFF_MathDeviceTable;

typedef struct ff_font {
    PyObject_HEAD
    /* Type-specific fields go here. */
    FontViewBase *fv;
    PyFF_LayerInfoArray *layers;
    PyFF_Private *priv;
    PyFF_Cvt *cvt;
    PyFF_Selection *selection;
    PyFF_Math *math;
} PyFF_Font;

extern PyMethodDef PyFF_Font_methods[];
extern PyMethodDef module_fontforge_methods[];

#ifdef __cplusplus
extern "C" {
#endif
PyObject* PyFF_FontForFV(FontViewBase *fv);
PyObject* PyFF_FontForFV_I(FontViewBase *fv);
#ifdef __cplusplus
}
#endif

#endif /* FONTFORGE_FFPYTHON_H */
