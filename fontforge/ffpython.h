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

#if PY_MAJOR_VERSION >= 3

#define PyInt_Check    PyLong_Check
#define PyInt_AsLong   PyLong_AsLong
#define PyInt_FromLong PyLong_FromLong

#define STRING_CHECK   PyUnicode_Check
#define STRING_TO_PY   PyUnicode_FromString
#define DECODE_UTF8(s, size, errors) PyUnicode_DecodeUTF8(s, size, errors)
#define PYBYTES_UTF8(str)            PyUnicode_AsUTF8String(str)
#define STRING_FROM_FORMAT           PyUnicode_FromFormat

#define PICKLE "pickle"

#else /* PY_MAJOR_VERSION >= 3 */

#define STRING_CHECK   PyBytes_Check
#define STRING_TO_PY   PyBytes_FromString
#define DECODE_UTF8(s, size, errors) PyString_Decode(s, size, "UTF-8", errors)
#define PYBYTES_UTF8(str)            PyString_AsEncodedObject(str, "UTF-8", NULL)
#define STRING_FROM_FORMAT           PyBytes_FromFormat

#define PICKLE "cPickle"

#endif /* PY_MAJOR_VERSION >= 3 */


#if !defined( Py_RETURN_NONE )
/* Not defined before 2.4 */
# define Py_RETURN_NONE		return( Py_INCREF(Py_None), Py_None )
#endif
#define Py_RETURN(self)		return( Py_INCREF((PyObject *) (self)), (PyObject *) (self) )

#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif

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

struct flaglist { char *name; int flag; };

extern SplineChar *sc_active_in_ui;
extern FontViewBase *fv_active_in_ui;
extern int layer_active_in_ui;

extern void FfPy_Replace_MenuItemStub(PyObject *(*func)(PyObject *,PyObject *));
extern PyObject *PySC_From_SC(SplineChar *sc);
extern PyObject *PyFV_From_FV(FontViewBase *fv);
extern int FlagsFromTuple(PyObject *tuple,struct flaglist *flags);
extern void PyFF_Glyph_Set_Layer(SplineChar *sc,int layer);
