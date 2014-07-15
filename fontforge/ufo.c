/* Copyright (C) 2003-2012 by George Williams */
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

#ifndef _NO_PYTHON
# include "Python.h"
# include "structmember.h"
#else
# include <utype.h>
#endif

#include "fontforgevw.h"
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <locale.h>
#include <chardata.h>
#include <gfile.h>
#include <ustring.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#ifndef _NO_PYTHON
# include "ffpython.h"
#endif

#include <stdarg.h>
#include <string.h>

#undef extended			/* used in xlink.h */
#include <libxml/tree.h>

/* The UFO (Unified Font Object) format ( http://unifiedfontobject.org/ ) */
/* is a directory containing a bunch of (mac style) property lists and another*/
/* directory containing glif files (and contents.plist). */

/* Property lists contain one <dict> element which contains <key> elements */
/*  each followed by an <integer>, <real>, <true/>, <false/> or <string> element, */
/*  or another <dict> */

/* UFO format 2.0 includes an adobe feature file "features.fea" and slightly */
/*  different/more tags in fontinfo.plist */

static char *buildname(const char *basedir, const char *sub) {
    char *fname = malloc(strlen(basedir)+strlen(sub)+2);

    strcpy(fname, basedir);
    if ( fname[strlen(fname)-1]!='/' )
	strcat(fname,"/");
    strcat(fname,sub);
return( fname );
}

static xmlNodePtr xmlNewChildInteger(xmlNodePtr parent, xmlNsPtr ns, const xmlChar * name, long int value) {
  char * valtmp = NULL;
  // Textify the value to be enclosed.
  if (asprintf(&valtmp, "%ld", value) >= 0) {
    xmlNodePtr childtmp = xmlNewChild(parent, NULL, BAD_CAST name, BAD_CAST valtmp); // Make a text node for the value.
    free(valtmp); valtmp = NULL; // Free the temporary text store.
    return childtmp;
  }
  return NULL;
}
static xmlNodePtr xmlNewNodeInteger(xmlNsPtr ns, const xmlChar * name, long int value) {
  char * valtmp = NULL;
  xmlNodePtr childtmp = xmlNewNode(NULL, BAD_CAST name); // Create a named node.
  // Textify the value to be enclosed.
  if (asprintf(&valtmp, "%ld", value) >= 0) {
    xmlNodePtr valtmpxml = xmlNewText(BAD_CAST valtmp); // Make a text node for the value.
    xmlAddChild(childtmp, valtmpxml); // Attach the text node as content of the named node.
    free(valtmp); valtmp = NULL; // Free the temporary text store.
  } else {
    xmlFreeNode(childtmp); childtmp = NULL;
  }
  return childtmp;
}
static xmlNodePtr xmlNewChildFloat(xmlNodePtr parent, xmlNsPtr ns, const xmlChar * name, double value) {
  char * valtmp = NULL;
  // Textify the value to be enclosed.
  if (asprintf(&valtmp, "%g", value) >= 0) {
    xmlNodePtr childtmp = xmlNewChild(parent, NULL, BAD_CAST name, BAD_CAST valtmp); // Make a text node for the value.
    free(valtmp); valtmp = NULL; // Free the temporary text store.
    return childtmp;
  }
  return NULL;
}
static xmlNodePtr xmlNewNodeFloat(xmlNsPtr ns, const xmlChar * name, double value) {
  char * valtmp = NULL;
  xmlNodePtr childtmp = xmlNewNode(NULL, BAD_CAST name); // Create a named node.
  // Textify the value to be enclosed.
  if (asprintf(&valtmp, "%g", value) >= 0) {
    xmlNodePtr valtmpxml = xmlNewText(BAD_CAST valtmp); // Make a text node for the value.
    xmlAddChild(childtmp, valtmpxml); // Attach the text node as content of the named node.
    free(valtmp); valtmp = NULL; // Free the temporary text store.
  } else {
    xmlFreeNode(childtmp); childtmp = NULL;
  }
  return NULL;
}
static xmlNodePtr xmlNewChildString(xmlNodePtr parent, xmlNsPtr ns, const xmlChar * name, char * value) {
  xmlNodePtr childtmp = xmlNewChild(parent, NULL, BAD_CAST name, BAD_CAST value); // Make a text node for the value.
  return childtmp;
}
static xmlNodePtr xmlNewNodeString(xmlNsPtr ns, const xmlChar * name, char * value) {
  xmlNodePtr childtmp = xmlNewNode(NULL, BAD_CAST name); // Create a named node.
  xmlNodePtr valtmpxml = xmlNewText(BAD_CAST value); // Make a text node for the value.
  xmlAddChild(childtmp, valtmpxml); // Attach the text node as content of the named node.
  return childtmp;
}
static xmlNodePtr xmlNewNodeVPrintf(xmlNsPtr ns, const xmlChar * name, char * format, va_list arguments) {
  char * valtmp = NULL;
  if (vasprintf(&valtmp, format, arguments) < 0) {
    return NULL;
  }
  xmlNodePtr childtmp = xmlNewNode(NULL, BAD_CAST name); // Create a named node.
  xmlNodePtr valtmpxml = xmlNewText(BAD_CAST valtmp); // Make a text node for the value.
  xmlAddChild(childtmp, valtmpxml); // Attach the text node as content of the named node.
  free(valtmp); valtmp = NULL; // Free the temporary text store.
  return childtmp;
}
static xmlNodePtr xmlNewNodePrintf(xmlNsPtr ns, const xmlChar * name, char * format, ...) {
  va_list arguments;
  va_start(arguments, format);
  xmlNodePtr output = xmlNewNodeVPrintf(ns, name, format, arguments);
  va_end(arguments);
  return output;
}
static xmlNodePtr xmlNewChildVPrintf(xmlNodePtr parent, xmlNsPtr ns, const xmlChar * name, char * format, va_list arguments) {
  xmlNodePtr output = xmlNewNodeVPrintf(ns, name, format, arguments);
  xmlAddChild(parent, output);
  return output;
}
static xmlNodePtr xmlNewChildPrintf(xmlNodePtr parent, xmlNsPtr ns, const xmlChar * name, char * format, ...) {
  va_list arguments;
  va_start(arguments, format);
  xmlNodePtr output = xmlNewChildVPrintf(parent, ns, name, format, arguments);
  va_end(arguments);
  return output;
}
static void xmlSetPropVPrintf(xmlNodePtr target, const xmlChar * name, char * format, va_list arguments) {
  char * valtmp = NULL;
  // Generate the value.
  if (vasprintf(&valtmp, format, arguments) < 0) {
    return;
  }
  xmlSetProp(target, name, valtmp); // Set the property.
  free(valtmp); valtmp = NULL; // Free the temporary text store.
  return;
}
static void xmlSetPropPrintf(xmlNodePtr target, const xmlChar * name, char * format, ...) {
  va_list arguments;
  va_start(arguments, format);
  xmlSetPropVPrintf(target, name, format, arguments);
  va_end(arguments);
  return;
}

/* ************************************************************************** */
/* *************************   Python lib Output    ************************* */
/* ************************************************************************** */
#ifndef _NO_PYTHON
static int PyObjDumpable(PyObject *value);
xmlNodePtr PyObjectToXML( PyObject *value );
#endif

xmlNodePtr PythonLibToXML(void *python_persistent,SplineChar *sc) {
    int has_hints = (sc!=NULL && (sc->hstem!=NULL || sc->vstem!=NULL ));
    xmlNodePtr retval = NULL, dictnode = NULL, keynode = NULL, valnode = NULL;
    // retval = xmlNewNode(NULL, BAD_CAST "lib"); //     "<lib>"
    dictnode = xmlNewNode(NULL, BAD_CAST "dict"); //     "  <dict>"
    if ( has_hints 
#ifndef _NO_PYTHON
         || (python_persistent!=NULL && PyMapping_Check((PyObject *)python_persistent) && sc!=NULL)
#endif
       ) {

        xmlAddChild(retval, dictnode);
        /* Not officially part of the UFO/glif spec, but used by robofab */
	if ( has_hints ) {
            // Remember that the value of the plist key is in the node that follows it in the dict (not an x.m.l. child).
            xmlNewChild(dictnode, NULL, BAD_CAST "key", BAD_CAST "com.fontlab.hintData"); // Label the hint data block.
	    //                                           "    <key>com.fontlab.hintData</key>\n"
	    //                                           "    <dict>"
            xmlNodePtr hintdict = xmlNewChild(dictnode, NULL, BAD_CAST "dict", NULL);
	    if ( sc->hstem!=NULL ) {
                StemInfo *h;
                xmlNewChild(hintdict, NULL, BAD_CAST "key", BAD_CAST "hhints");
		//                                       "      <key>hhints</key>"
		//                                       "      <array>"
                xmlNodePtr hintarray = xmlNewChild(hintdict, NULL, BAD_CAST "array", NULL);
		for ( h = sc->hstem; h!=NULL; h=h->next ) {
                    char * valtmp = NULL;
                    xmlNodePtr stemdict = xmlNewChild(hintarray, NULL, BAD_CAST "dict", NULL);
		    //                                   "        <dict>"
                    xmlNewChild(stemdict, NULL, BAD_CAST "key", "position");
		    //                                   "          <key>position</key>"
                    xmlNewChildInteger(stemdict, NULL, BAD_CAST "integer", (int) rint(h->start));
		    //                                   "          <integer>%d</integer>\n" ((int) rint(h->start))
                    xmlNewChild(stemdict, NULL, BAD_CAST "key", "width");
		    //                                   "          <key>width</key>"
                    xmlNewChildInteger(stemdict, NULL, BAD_CAST "integer", (int) rint(h->width));
		    //                                   "          <integer>%d</integer>\n" ((int) rint(h->width))
		    //                                   "        </dict>\n"
		}
		//                                       "      </array>\n"
	    }
	    if ( sc->vstem!=NULL ) {
                StemInfo *h;
                xmlNewChild(hintdict, NULL, BAD_CAST "key", BAD_CAST "vhints");
		//                                       "      <key>vhints</key>"
		//                                       "      <array>"
                xmlNodePtr hintarray = xmlNewChild(hintdict, NULL, BAD_CAST "array", NULL);
		for ( h = sc->vstem; h!=NULL; h=h->next ) {
                    char * valtmp = NULL;
                    xmlNodePtr stemdict = xmlNewChild(hintarray, NULL, BAD_CAST "dict", NULL);
		    //                                   "        <dict>"
                    xmlNewChild(stemdict, NULL, BAD_CAST "key", "position");
		    //                                   "          <key>position</key>"
                    xmlNewChildInteger(stemdict, NULL, BAD_CAST "integer", (int) rint(h->start));
		    //                                   "          <integer>%d</integer>\n" ((int) rint(h->start))
                    xmlNewChild(stemdict, NULL, BAD_CAST "key", "width");
		    //                                   "          <key>width</key>"
                    xmlNewChildInteger(stemdict, NULL, BAD_CAST "integer", (int) rint(h->width));
		    //                                   "          <integer>%d</integer>\n" ((int) rint(h->width))
		    //                                   "        </dict>\n"
		}
		//                                       "      </array>\n"
	    }
	    //                                           "    </dict>"
	}
#ifndef _NO_PYTHON
        PyObject *dict = python_persistent, *items, *key, *value;
	/* Ok, look at the persistent data and output it (all except for a */
	/*  hint entry -- we've already handled that with the real hints, */
	/*  no point in retaining out of date hints too */
	if ( dict != NULL ) {
            int i, len;
            char *str;
	    items = PyMapping_Items(dict);
	    len = PySequence_Size(items);
	    for ( i=0; i<len; ++i ) {
			PyObject *item = PySequence_GetItem(items,i);
			key = PyTuple_GetItem(item,0);
			if ( !PyBytes_Check(key))		/* Keys need not be strings */
			continue;
			str = PyBytes_AsString(key);
			if ( !str || (strcmp(str,"com.fontlab.hintData")==0 && sc!=NULL) )	/* Already done */
			continue;
			value = PyTuple_GetItem(item,1);
			if ( !value || !PyObjDumpable(value))
			continue;
			// "<key>%s</key>" str
      xmlNewChild(dictnode, NULL, BAD_CAST "key", str);
      xmlNodePtr tmpNode = PyObjectToXML(value);
      xmlAddChild(dictnode, tmpNode);
			// "<...>...</...>"
	    }
	}
#endif
    }
    //                                                 "  </dict>"
    // //                                                 "</lib>"
    return dictnode;
}

#ifndef _NO_PYTHON
static int PyObjDumpable(PyObject *value) {
    if ( PyInt_Check(value))
return( true );
    if ( PyFloat_Check(value))
return( true );
	if ( PyDict_Check(value))
return( true );
    if ( PySequence_Check(value))		/* Catches strings and tuples */
return( true );
    if ( PyMapping_Check(value))
return( true );
    if ( PyBool_Check(value))
return( true );
    if ( value == Py_None )
return( true );

return( false );
}

xmlNodePtr PyObjectToXML( PyObject *value ) {
    xmlNodePtr childtmp = NULL;
    xmlNodePtr valtmpxml = NULL;
    char * valtmp = NULL;
    if (PyDict_Check(value)) {
      childtmp = PythonLibToXML(value,NULL);
    } else if ( PyMapping_Check(value)) {
      childtmp = PythonLibToXML(value,NULL);
    } else if ( PyBytes_Check(value)) {		/* Must precede the sequence check */
      char *str = PyBytes_AsString(value);
      if (str != NULL) {
        childtmp = xmlNewNode(NULL, BAD_CAST "integer"); // Create a string node.
          // "<string>%s</string>" str
      }
    } else if ( value==Py_True )
	childtmp = xmlNewNode(NULL, BAD_CAST "true"); // "<true/>"
    else if ( value==Py_False )
        childtmp = xmlNewNode(NULL, BAD_CAST "false"); // "<false/>"
    else if ( value==Py_None )
        childtmp = xmlNewNode(NULL, BAD_CAST "none");  // "<none/>"
    else if (PyInt_Check(value)) {
        childtmp = xmlNewNodeInteger(NULL, BAD_CAST "integer", PyInt_AsLong(value)); // Create an integer node.
        // "<integer>%ld</integer>"
    } else if (PyFloat_Check(value)) {
        childtmp = xmlNewNode(NULL, BAD_CAST "real");
        if (asprintf(&valtmp, "%g", PyFloat_AsDouble(value)) < 0) {
          valtmpxml = xmlNewText(BAD_CAST valtmp);
          xmlAddChild(childtmp, valtmpxml);
          free(valtmp); valtmp = NULL;
        } else {
          xmlFreeNode(childtmp); childtmp = NULL;
        }
        // "<real>%g</real>"
    } else if (PySequence_Check(value)) {
	int i, len = PySequence_Size(value);
        xmlNodePtr itemtmp = NULL;
        childtmp = xmlNewNode(NULL, BAD_CAST "array");
        // "<array>"
	for ( i=0; i<len; ++i ) {
	    PyObject *obj = PySequence_GetItem(value,i);
	    if ( PyObjDumpable(obj)) {
		itemtmp = PyObjectToXML(obj);
                xmlAddChild(childtmp, itemtmp);
	    }
	}
        // "</array>"
    }
    return childtmp;
}
#endif

/* ************************************************************************** */
/* ****************************   GLIF Output    **************************** */
/* ************************************************************************** */

static int refcomp(const void *_r1, const void *_r2) {
    const RefChar *ref1 = *(RefChar * const *)_r1;
    const RefChar *ref2 = *(RefChar * const *)_r2;
return( strcmp( ref1->sc->name, ref2->sc->name) );
}

xmlNodePtr _GlifToXML(SplineChar *sc,int layer) {
    struct altuni *altuni;
    int isquad = sc->layers[layer].order2;
    SplineSet *spl;
    SplinePoint *sp;
    AnchorPoint *ap;
    RefChar *ref;
    int err;
    char * stringtmp = NULL;
    char numstring[32];
    memset(numstring, 0, sizeof(numstring));

    // "<?xml version=\"1.0\" encoding=\"UTF-8\""
    /* No DTD for these guys??? */
    // Is there a DTD for glif data? (asks Frank)
    // Perhaps we need to make one.

    xmlNodePtr topglyphxml = xmlNewNode(NULL, BAD_CAST "glyph"); // Create the glyph node.
    xmlSetProp(topglyphxml, "name", sc->name); // Set the name for the glyph.
    xmlSetProp(topglyphxml, "format", "1"); // Set the format of the glyph.
    // "<glyph name=\"%s\" format=\"1\">" sc->name

    xmlNodePtr tmpxml2 = xmlNewChild(topglyphxml, NULL, BAD_CAST "advance", NULL); // Create the advance node.
    xmlSetPropPrintf(tmpxml2, BAD_CAST "width", "%d", sc->width);
    if ( sc->parent->hasvmetrics ) {
      if (asprintf(&stringtmp, "%d", sc->width) >= 0) {
        xmlSetProp(tmpxml2, BAD_CAST "height", stringtmp);
        free(stringtmp); stringtmp = NULL;
      }
    }
    // "<advance width=\"%d\" height=\"%d\"/>" sc->width sc->vwidth

    if ( sc->unicodeenc!=-1 ) {
      xmlNodePtr unicodexml = xmlNewChild(topglyphxml, NULL, BAD_CAST "unicode", NULL);
      xmlSetPropPrintf(unicodexml, BAD_CAST "hex", "%04X", sc->unicodeenc);
    }
    // "<unicode hex=\"%04X\"/>\n" sc->unicodeenc

    for ( altuni = sc->altuni; altuni!=NULL; altuni = altuni->next )
	if ( altuni->vs==-1 && altuni->fid==0 ) {
          xmlNodePtr unicodexml = xmlNewChild(topglyphxml, NULL, BAD_CAST "unicode", NULL);
          xmlSetPropPrintf(unicodexml, BAD_CAST "hex", "%04X", altuni->unienc);
        }
        // "<unicode hex=\"%04X\"/>" altuni->unienc

    if ( sc->layers[layer].refs!=NULL || sc->layers[layer].splines!=NULL ) {
      xmlNodePtr outlinexml = xmlNewChild(topglyphxml, NULL, BAD_CAST "outline", NULL);
	// "<outline>"
	/* RoboFab outputs components in alphabetic (case sensitive) order */
	/*  I've been asked to do that too */
	if ( sc->layers[layer].refs!=NULL ) {
	    RefChar **refs;
	    int i, cnt;
	    for ( cnt=0, ref = sc->layers[layer].refs; ref!=NULL; ref=ref->next ) if ( SCWorthOutputting(ref->sc))
		++cnt;
	    refs = malloc(cnt*sizeof(RefChar *));
	    for ( cnt=0, ref = sc->layers[layer].refs; ref!=NULL; ref=ref->next ) if ( SCWorthOutputting(ref->sc))
		refs[cnt++] = ref;
	    if ( cnt>1 )
		qsort(refs,cnt,sizeof(RefChar *),refcomp);
	    for ( i=0; i<cnt; ++i ) {
		ref = refs[i];
    xmlNodePtr componentxml = xmlNewChild(outlinexml, NULL, BAD_CAST "component", NULL);
    xmlSetPropPrintf(componentxml, BAD_CAST "base", "%s", ref->sc->name);
		// "<component base=\"%s\"" ref->sc->name
    char *floattmp = NULL;
		if ( ref->transform[0]!=1 ) {
                  xmlSetPropPrintf(componentxml, BAD_CAST "xScale", "%g", (double) ref->transform[0]);
		    // "xScale=\"%g\"" (double)ref->transform[0]
                }
		if ( ref->transform[3]!=1 ) {
                  xmlSetPropPrintf(componentxml, BAD_CAST "yScale", "%g", (double) ref->transform[3]);
		    // "yScale=\"%g\"" (double)ref->transform[3]
                }
		if ( ref->transform[1]!=0 ) {
                  xmlSetPropPrintf(componentxml, BAD_CAST "xyScale", "%g", (double) ref->transform[1]);
		    // "xyScale=\"%g\"" (double)ref->transform[1]
                }
		if ( ref->transform[2]!=0 ) {
                  xmlSetPropPrintf(componentxml, BAD_CAST "yxScale", "%g", (double) ref->transform[2]);
		    // "yxScale=\"%g\"" (double)ref->transform[2]
                }
		if ( ref->transform[4]!=0 ) {
                  xmlSetPropPrintf(componentxml, BAD_CAST "xOffset", "%g", (double) ref->transform[4]);
		    // "xOffset=\"%g\"" (double)ref->transform[4]
                }
		if ( ref->transform[5]!=0 ) {
                  xmlSetPropPrintf(componentxml, BAD_CAST "yOffset", "%g", (double) ref->transform[5]);
		    // "yOffset=\"%g\"" (double)ref->transform[5]
                }
		// "/>"
	    }
	    free(refs);
	}
        for ( ap=sc->anchor; ap!=NULL; ap=ap->next ) {
            int ismark = (ap->type==at_mark || ap->type==at_centry);
            xmlNodePtr contourxml = xmlNewChild(outlinexml, NULL, BAD_CAST "contour", NULL);
            // "<contour>"
            xmlNodePtr pointxml = xmlNewChild(contourxml, NULL, BAD_CAST "point", NULL);
            xmlSetPropPrintf(pointxml, BAD_CAST "x", "%g", ap->me.x);
            xmlSetPropPrintf(pointxml, BAD_CAST "y", "%g", ap->me.y);
            xmlSetPropPrintf(pointxml, BAD_CAST "type", "move");
            xmlSetPropPrintf(pointxml, BAD_CAST "name", "%s%s", ismark ? "_" : "", ap->anchor->name);
            // "<point x=\"%g\" y=\"%g\" type=\"move\" name=\"%s%s\"/>" ap->me.x ap->me.y (ismark ? "_" : "") ap->anchor->name
            // "</contour>"
        }
	for ( spl=sc->layers[layer].splines; spl!=NULL; spl=spl->next ) {
            xmlNodePtr contourxml = xmlNewChild(outlinexml, NULL, BAD_CAST "contour", NULL);
	    // "<contour>"
	    for ( sp=spl->first; sp!=NULL; ) {
		/* Undocumented fact: If a contour contains a series of off-curve points with no on-curve then treat as quadratic even if no qcurve */
		// We write the next on-curve point.
		if (!isquad || sp->ttfindex != 0xffff || !SPInterpolate(sp) || sp->pointtype!=pt_curve || sp->name != NULL) {
		  xmlNodePtr pointxml = xmlNewChild(contourxml, NULL, BAD_CAST "point", NULL);
		  xmlSetPropPrintf(pointxml, BAD_CAST "x", "%g", (double)sp->me.x);
		  xmlSetPropPrintf(pointxml, BAD_CAST "y", "%g", (double)sp->me.y);
		  xmlSetPropPrintf(pointxml, BAD_CAST "type", BAD_CAST (
		  sp->prev==NULL        ? "move"   :
					sp->prev->knownlinear ? "line"   :
					isquad 		      ? "qcurve" :
					"curve"));
		  if (sp->pointtype != pt_corner) xmlSetProp(pointxml, BAD_CAST "smooth", BAD_CAST "yes");
		  if (sp->name !=NULL) xmlSetProp(pointxml, BAD_CAST "name", BAD_CAST sp->name);
                  // "<point x=\"%g\" y=\"%g\" type=\"%s\"%s%s%s%s/>\n" (double)sp->me.x (double)sp->me.y
		  // (sp->prev==NULL ? "move" : sp->prev->knownlinear ? "line" : isquad ? "qcurve" : "curve")
		  // (sp->pointtype!=pt_corner?" smooth=\"yes\"":"")
		  // (sp->name?" name=\"":"") (sp->name?sp->name:"") (sp->name?"\"":"")
		}
		if ( sp->next==NULL )
	    	  break;
		// We write control points.
		if ( !sp->next->knownlinear ) {
                          xmlNodePtr pointxml = xmlNewChild(contourxml, NULL, BAD_CAST "point", NULL);
                          xmlSetPropPrintf(pointxml, BAD_CAST "x", "%g", (double)sp->nextcp.x);
                          xmlSetPropPrintf(pointxml, BAD_CAST "y", "%g", (double)sp->nextcp.y);
		    	  // "<point x=\"%g\" y=\"%g\"/>\n" (double)sp->nextcp.x (double)sp->nextcp.y
		}
		sp = sp->next->to;
		if ( !isquad && !sp->prev->knownlinear ) {
                          xmlNodePtr pointxml = xmlNewChild(contourxml, NULL, BAD_CAST "point", NULL);
                          xmlSetPropPrintf(pointxml, BAD_CAST "x", "%g", (double)sp->prevcp.x);
                          xmlSetPropPrintf(pointxml, BAD_CAST "y", "%g", (double)sp->prevcp.y);
                          // "<point x=\"%g\" y=\"%g\"/>\n" (double)sp->prevcp.x (double)sp->prevcp.y
		}
		if ( sp==spl->first )
	    		break;
	    }
	    // "</contour>"
	}
	// "</outline>"
    }
    xmlNodePtr libxml = xmlNewChild(topglyphxml, NULL, BAD_CAST "lib", NULL);
    xmlNodePtr pythonblob = PythonLibToXML(sc->python_persistent, sc);
    xmlAddChild(libxml, pythonblob);
    return topglyphxml;
}

static int GlifDump(const char *glyphdir, const char *gfname, const SplineChar *sc, int layer) {
    char *gn = buildname(glyphdir,gfname);
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    if (doc == NULL) return 0;
    xmlNodePtr root_node = _GlifToXML(sc, layer);
    if (root_node == NULL) {xmlFreeDoc(doc); doc = NULL; return 0;}
    xmlDocSetRootElement(doc, root_node);
    int ret = (xmlSaveFormatFileEnc(gn, doc, "UTF-8", 1) != -1);
    xmlFreeDoc(doc); doc = NULL;
    free(gn);
    return ret;
}

int _ExportGlif(FILE *glif,SplineChar *sc,int layer) {
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr root_node = _GlifToXML(sc, layer);
    xmlDocSetRootElement(doc, root_node);
    xmlDocFormatDump(glif, doc, 1);
    xmlFreeDoc(doc); doc = NULL;
    return ( doc != NULL );
}

/* ************************************************************************** */
/* ****************************    UFO Output    **************************** */
/* ************************************************************************** */

void clear_cached_ufo_paths(SplineFont * sf) {
  // We cache the glif names and the layer paths.
  // This is helpful for preserving the structure of a U. F. O. to be edited.
  // But it may be desirable to purge that data on final output for consistency.
  // This function does that.
  int i;
  // First we clear the glif names.
  for (i = 0; i < sf->glyphcnt; i++) {
    struct splinechar * sc = sf->glyphs[i];
    if (sc->glif_name != NULL) { free(sc->glif_name); sc->glif_name = NULL; }
  }
  // Then we clear the layer names.
  for (i = 0; i < sf->layer_cnt; i++) {
    struct layerinfo * ly = &(sf->layers[i]);
    if (ly->ufo_path != NULL) { free(ly->ufo_path); ly->ufo_path = NULL; }
  }
}

xmlDocPtr PlistInit() {
    // Some of this code is pasted from libxml2 samples.
    xmlDocPtr doc = NULL;
    xmlNodePtr root_node = NULL, dict_node = NULL;
    xmlDtdPtr dtd = NULL;
    
    char buff[256];
    int i, j;

    LIBXML_TEST_VERSION;

    doc = xmlNewDoc(BAD_CAST "1.0");
    dtd = xmlCreateIntSubset(doc, BAD_CAST "plist", BAD_CAST "-//Apple Computer//DTD PLIST 1.0//EN", BAD_CAST "http://www.apple.com/DTDs/PropertyList-1.0.dtd");
    root_node = xmlNewNode(NULL, BAD_CAST "plist");
    xmlDocSetRootElement(doc, root_node);
    return doc;
}

static void PListAddInteger(xmlNodePtr parent, const char *key, int value) {
    xmlNewChild(parent, NULL, BAD_CAST "key", BAD_CAST key);
    xmlNewChildPrintf(parent, NULL, BAD_CAST "integer", "%d", value);
}

static void PListAddReal(xmlNodePtr parent, const char *key, double value) {
    xmlNewChild(parent, NULL, BAD_CAST "key", BAD_CAST key);
    xmlNewChildPrintf(parent, NULL, BAD_CAST "real", "%g", value);
}

static void PListAddBoolean(xmlNodePtr parent, const char *key, int value) {
    xmlNewChild(parent, NULL, BAD_CAST "key", BAD_CAST key);
    xmlNewChild(parent, NULL, BAD_CAST (value ? "true": "false"), NULL);
}

static void PListAddDate(xmlNodePtr parent, const char *key, time_t timestamp) {
/* openTypeHeadCreated = string format as \"YYYY/MM/DD HH:MM:SS\".	*/
/* \"YYYY/MM/DD\" is year/month/day. The month is in the range 1-12 and	*/
/* the day is in the range 1-end of month.				*/
/*  \"HH:MM:SS\" is hour:minute:second. The hour is in the range 0:23.	*/
/* Minutes and seconds are in the range 0-59.				*/
    struct tm *tm = gmtime(&timestamp);
    xmlNewChild(parent, NULL, BAD_CAST "key", BAD_CAST key);
    xmlNewChildPrintf(parent, NULL, BAD_CAST "string",
            "%4d/%02d/%02d %02d:%02d:%02d", tm->tm_year+1900, tm->tm_mon+1,
	    tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
}

int count_occurrence(const char* big, const char* little) {
    const char * tmp = big;
    int output = 0;
    while (tmp = strstr(tmp, little)) { output ++; tmp ++; }
    return output;
}

void PListAddString(xmlNodePtr parent, const char *key, const char *value) {
    if ( value==NULL ) value = "";
    xmlNodePtr keynode = xmlNewChild(parent, NULL, BAD_CAST "key", BAD_CAST key); // "<key>%s</key>" key
#ifdef ESCAPE_LIBXML_STRINGS
    size_t main_size = strlen(value) +
                       (count_occurrence(value, "<") * (strlen("&lt")-strlen("<"))) +
                       (count_occurrence(value, ">") * (strlen("&gt")-strlen("<"))) +
                       (count_occurrence(value, "&") * (strlen("&amp")-strlen("<")));
    char *tmpstring = malloc(main_size + 1); tmpstring[0] = '\0';
    off_t pos1 = 0;
    while ( *value ) {
	if ( *value=='<' ) {
	    strcat(tmpstring, "&lt;");
            pos1 += strlen("&lt;");
	} else if ( *value=='>' ) {
	    strcat(tmpstring, "&gt;");
            pos1 += strlen("&gt;");
	} else if ( *value == '&' ) {
	    strcat(tmpstring, "&amp;");
	    pos1 += strlen("&amp;");
	} else {
	    tmpstring[pos1++] = *value;
            tmpstring[pos1] = '\0';
        }
	++value;
    }
    xmlNodePtr valnode = xmlNewChild(parent, NULL, BAD_CAST "string", tmpstring); // "<string>%s</string>" tmpstring
#else
    xmlNodePtr valnode = xmlNewChild(parent, NULL, BAD_CAST "string", value); // "<string>%s</string>" tmpstring
#endif
}

static void PListAddNameString(xmlNodePtr parent, const char *key, const SplineFont *sf, int strid) {
    char *value=NULL, *nonenglish=NULL, *freeme=NULL;
    struct ttflangname *nm;

    for ( nm=sf->names; nm!=NULL; nm=nm->next ) {
	if ( nm->names[strid]!=NULL ) {
	    nonenglish = nm->names[strid];
	    if ( nm->lang == 0x409 ) {
		value = nm->names[strid];
    break;
	    }
	}
    }
    if ( value==NULL && strid==ttf_version && sf->version!=NULL )
	value = freeme = strconcat("Version ",sf->version);
    if ( value==NULL )
	value=nonenglish;
    if ( value!=NULL ) {
	PListAddString(parent,key,value);
    }
    free(freeme);
}

static void PListAddIntArray(xmlNodePtr parent, const char *key, const char *entries, int len) {
    int i;
    xmlNewChild(parent, NULL, BAD_CAST "key", BAD_CAST key);
    xmlNodePtr arrayxml = xmlNewChild(parent, NULL, BAD_CAST "array", NULL);
    for ( i=0; i<len; ++i ) {
      xmlNewChildInteger(arrayxml, NULL, BAD_CAST "integer", entries[i]);
    }
}

static void PListAddPrivateArray(xmlNodePtr parent, const char *key, struct psdict *private) {
    char *value;
    if ( private==NULL )
return;
    value = PSDictHasEntry(private,key);
    if ( value==NULL )
return;
    xmlNewChildPrintf(parent, NULL, BAD_CAST "key", "postscript%s", key); // "<key>postscript%s</key>" key
    xmlNodePtr arrayxml = xmlNewChild(parent, NULL, BAD_CAST "array", NULL); // "<array>"
    while ( *value==' ' || *value=='[' ) ++value;
    for (;;) {
	int skipping=0;
        size_t tmpsize = 8;
        char * tmp = malloc(tmpsize);
        off_t tmppos = 0;
	while ( *value!=']' && *value!='\0' && *value!=' ' && tmp!=NULL) {
            if (*value=='.') skipping = true;
	    if (skipping)
		++value;
	    else
                tmp[tmppos++] = *value++;
            if (tmppos == tmpsize) { tmpsize *= 2; tmp = realloc(tmp, tmpsize); }
	}
        tmp[tmppos] = '\0';
        if (tmp != NULL) {
          xmlNewChildString(arrayxml, NULL, BAD_CAST "integer", BAD_CAST tmp); // "<integer>%s</integer>" tmp
          free(tmp); tmp = NULL;
        }
	while ( *value==' ' ) ++value;
	if ( *value==']' || *value=='\0' ) break;
    }
    // "</array>"
}

static void PListAddPrivateThing(xmlNodePtr parent, const char *key, struct psdict *private, char *type) {
    char *value;

    if ( private==NULL ) return;
    value = PSDictHasEntry(private,key);
    if ( value==NULL ) return;

    while ( *value==' ' || *value=='[' ) ++value;

    xmlNewChildPrintf(parent, NULL, BAD_CAST "key", "postscript%s", key); // "<key>postscript%s</key>" key
    while ( *value==' ' || *value=='[' ) ++value;
    {
	int skipping=0;
        size_t tmpsize = 8;
        char * tmp = malloc(tmpsize);
        off_t tmppos = 0;
	while ( *value!=']' && *value!='\0' && *value!=' ' && tmp!=NULL) {
            if (*value=='.') skipping = true;
	    if (skipping)
		++value;
	    else
                tmp[tmppos++] = *value++;
            if (tmppos == tmpsize) { tmpsize *= 2; tmp = realloc(tmp, tmpsize); }
	}
        if (tmp != NULL) {
          xmlNewChildString(parent, NULL, BAD_CAST "integer", BAD_CAST tmp); // "<integer>%s</integer>" tmp
          free(tmp); tmp = NULL;
        }
	while ( *value==' ' ) ++value;
    }
}

static int UFOOutputMetaInfo(const char *basedir,SplineFont *sf) {
    xmlDocPtr plistdoc = PlistInit(); if (plistdoc == NULL) return false; // Make the document.
    xmlNodePtr rootnode = xmlDocGetRootElement(plistdoc); if (rootnode == NULL) { xmlFreeDoc(plistdoc); return false; } // Find the root node.
    xmlNodePtr dictnode = xmlNewChild(rootnode, NULL, BAD_CAST "dict", NULL); if (rootnode == NULL) { xmlFreeDoc(plistdoc); return false; } // Add the dict.
    PListAddString(dictnode,"creator","net.GitHub.FontForge");
    PListAddInteger(dictnode,"formatVersion",2);
    char *fname = buildname(basedir, "metainfo.plist"); // Build the file name.
    xmlSaveFormatFileEnc(fname, plistdoc, "UTF-8", 1); // Store the document.
    free(fname); fname = NULL;
    xmlFreeDoc(plistdoc); // Free the memory.
    xmlCleanupParser();
    return true;
}

static int UFOOutputFontInfo(const char *basedir, SplineFont *sf, int layer) {
    xmlDocPtr plistdoc = PlistInit(); if (plistdoc == NULL) return false; // Make the document.
    xmlNodePtr rootnode = xmlDocGetRootElement(plistdoc); if (rootnode == NULL) { xmlFreeDoc(plistdoc); return false; } // Find the root node.
    xmlNodePtr dictnode = xmlNewChild(rootnode, NULL, BAD_CAST "dict", NULL); if (rootnode == NULL) { xmlFreeDoc(plistdoc); return false; } // Add the dict.

    DBounds bb;
    double test;

/* Same keys in both formats */
    PListAddString(dictnode,"familyName",sf->familyname_with_timestamp ? sf->familyname_with_timestamp : sf->familyname);
    PListAddString(dictnode,"styleName",SFGetModifiers(sf));
    PListAddString(dictnode,"copyright",sf->copyright);
    PListAddNameString(dictnode,"trademark",sf,ttf_trademark);
    PListAddInteger(dictnode,"unitsPerEm",sf->ascent+sf->descent);
    test = SFXHeight(sf,layer,true);
    if ( test>0 )
	PListAddInteger(dictnode,"xHeight",(int) rint(test));
    test = SFCapHeight(sf,layer,true);
    if ( test>0 )
	PListAddInteger(dictnode,"capHeight",(int) rint(test));
    if ( sf->ufo_ascent==0 )
	PListAddInteger(dictnode,"ascender",sf->ascent);
    else if ( sf->ufo_ascent==floor(sf->ufo_ascent))
	PListAddInteger(dictnode,"ascender",sf->ufo_ascent);
    else
	PListAddReal(dictnode,"ascender",sf->ufo_ascent);
    if ( sf->ufo_descent==0 )
	PListAddInteger(dictnode,"descender",-sf->descent);
    else if ( sf->ufo_descent==floor(sf->ufo_descent))
	PListAddInteger(dictnode,"descender",sf->ufo_descent);
    else
	PListAddReal(dictnode,"descender",sf->ufo_descent);
    PListAddReal(dictnode,"italicAngle",sf->italicangle);
    PListAddString(dictnode,"note",sf->comments);
    PListAddDate(dictnode,"openTypeHeadCreated",sf->creationtime);
    SplineFontFindBounds(sf,&bb);

    if ( sf->pfminfo.hheadascent_add )
	PListAddInteger(dictnode,"openTypeHheaAscender",bb.maxy+sf->pfminfo.hhead_ascent);
    else
	PListAddInteger(dictnode,"openTypeHheaAscender",sf->pfminfo.hhead_ascent);
    if ( sf->pfminfo.hheaddescent_add )
	PListAddInteger(dictnode,"openTypeHheaDescender",bb.miny+sf->pfminfo.hhead_descent);
    else
	PListAddInteger(dictnode,"openTypeHheaDescender",sf->pfminfo.hhead_descent);
    PListAddInteger(dictnode,"openTypeHheaLineGap",sf->pfminfo.linegap);

    PListAddNameString(dictnode,"openTypeNameDesigner",sf,ttf_designer);
    PListAddNameString(dictnode,"openTypeNameDesignerURL",sf,ttf_designerurl);
    PListAddNameString(dictnode,"openTypeNameManufacturer",sf,ttf_manufacturer);
    PListAddNameString(dictnode,"openTypeNameManufacturerURL",sf,ttf_venderurl);
    PListAddNameString(dictnode,"openTypeNameLicense",sf,ttf_license);
    PListAddNameString(dictnode,"openTypeNameLicenseURL",sf,ttf_licenseurl);
    PListAddNameString(dictnode,"openTypeNameVersion",sf,ttf_version);
    PListAddNameString(dictnode,"openTypeNameUniqueID",sf,ttf_uniqueid);
    PListAddNameString(dictnode,"openTypeNameDescription",sf,ttf_descriptor);
    PListAddNameString(dictnode,"openTypeNamePreferedFamilyName",sf,ttf_preffamilyname);
    PListAddNameString(dictnode,"openTypeNamePreferedSubfamilyName",sf,ttf_prefmodifiers);
    PListAddNameString(dictnode,"openTypeNameCompatibleFullName",sf,ttf_compatfull);
    PListAddNameString(dictnode,"openTypeNameSampleText",sf,ttf_sampletext);
    PListAddNameString(dictnode,"openTypeWWSFamilyName",sf,ttf_wwsfamily);
    PListAddNameString(dictnode,"openTypeWWSSubfamilyName",sf,ttf_wwssubfamily);
    if ( sf->pfminfo.panose_set )
	PListAddIntArray(dictnode,"openTypeOS2Panose",sf->pfminfo.panose,10);
    if ( sf->pfminfo.pfmset ) {
	char vendor[8], fc[2];
	PListAddInteger(dictnode,"openTypeOS2WidthClass",sf->pfminfo.width);
	PListAddInteger(dictnode,"openTypeOS2WeightClass",sf->pfminfo.weight);
	memcpy(vendor,sf->pfminfo.os2_vendor,4);
	vendor[4] = 0;
	PListAddString(dictnode,"openTypeOS2VendorID",vendor);
	fc[0] = sf->pfminfo.os2_family_class>>8; fc[1] = sf->pfminfo.os2_family_class&0xff;
	PListAddIntArray(dictnode,"openTypeOS2FamilyClass",fc,2);
	if ( sf->pfminfo.fstype!=-1 ) {
	    int fscnt,i;
	    char fstype[16];
	    for ( i=fscnt=0; i<16; ++i )
		if ( sf->pfminfo.fstype&(1<<i) )
		    fstype[fscnt++] = i;
	    if ( fscnt!=0 )
		PListAddIntArray(dictnode,"openTypeOS2Type",fstype,fscnt);
	}
	if ( sf->pfminfo.typoascent_add )
	    PListAddInteger(dictnode,"openTypeOS2TypoAscender",sf->ascent+sf->pfminfo.os2_typoascent);
	else
	    PListAddInteger(dictnode,"openTypeOS2TypoAscender",sf->pfminfo.os2_typoascent);
	if ( sf->pfminfo.typodescent_add )
	    PListAddInteger(dictnode,"openTypeOS2TypoDescender",sf->descent+sf->pfminfo.os2_typodescent);
	else
	    PListAddInteger(dictnode,"openTypeOS2TypoDescender",sf->pfminfo.os2_typodescent);
	PListAddInteger(dictnode,"openTypeOS2TypoLineGap",sf->pfminfo.os2_typolinegap);
	if ( sf->pfminfo.winascent_add )
	    PListAddInteger(dictnode,"openTypeOS2WinAscent",bb.maxy+sf->pfminfo.os2_winascent);
	else
	    PListAddInteger(dictnode,"openTypeOS2WinAscent",sf->pfminfo.os2_winascent);
	if ( sf->pfminfo.windescent_add )
	    PListAddInteger(dictnode,"openTypeOS2WinDescent",bb.miny+sf->pfminfo.os2_windescent);
	else
	    PListAddInteger(dictnode,"openTypeOS2WinDescent",sf->pfminfo.os2_windescent);
    }
    if ( sf->pfminfo.subsuper_set ) {
	PListAddInteger(dictnode,"openTypeOS2SubscriptXSize",sf->pfminfo.os2_subxsize);
	PListAddInteger(dictnode,"openTypeOS2SubscriptYSize",sf->pfminfo.os2_subysize);
	PListAddInteger(dictnode,"openTypeOS2SubscriptXOffset",sf->pfminfo.os2_subxoff);
	PListAddInteger(dictnode,"openTypeOS2SubscriptYOffset",sf->pfminfo.os2_subyoff);
	PListAddInteger(dictnode,"openTypeOS2SuperscriptXSize",sf->pfminfo.os2_supxsize);
	PListAddInteger(dictnode,"openTypeOS2SuperscriptYSize",sf->pfminfo.os2_supysize);
	PListAddInteger(dictnode,"openTypeOS2SuperscriptXOffset",sf->pfminfo.os2_supxoff);
	PListAddInteger(dictnode,"openTypeOS2SuperscriptYOffset",sf->pfminfo.os2_supyoff);
	PListAddInteger(dictnode,"openTypeOS2StrikeoutSize",sf->pfminfo.os2_strikeysize);
	PListAddInteger(dictnode,"openTypeOS2StrikeoutPosition",sf->pfminfo.os2_strikeypos);
    }
    if ( sf->pfminfo.vheadset )
	PListAddInteger(dictnode,"openTypeVheaTypoLineGap",sf->pfminfo.vlinegap);
    if ( sf->pfminfo.hasunicoderanges ) {
	char ranges[128];
	int i, j, c = 0;

	for ( i = 0; i<4; i++ )
	    for ( j = 0; j<32; j++ )
		if ( sf->pfminfo.unicoderanges[i] & (1 << j) )
		    ranges[c++] = i*32+j;
	if ( c!=0 )
	    PListAddIntArray(dictnode,"openTypeOS2UnicodeRanges",ranges,c);
    }
    if ( sf->pfminfo.hascodepages ) {
	char pages[64];
	int i, j, c = 0;

	for ( i = 0; i<2; i++)
	    for ( j=0; j<32; j++ )
		if ( sf->pfminfo.codepages[i] & (1 << j) )
		    pages[c++] = i*32+j;
	if ( c!=0 )
	    PListAddIntArray(dictnode,"openTypeOS2CodePageRanges",pages,c);
    }
    PListAddString(dictnode,"postscriptFontName",sf->fontname);
    PListAddString(dictnode,"postscriptFullName",sf->fullname);
    PListAddString(dictnode,"postscriptWeightName",sf->weight);
    /* Spec defines a "postscriptSlantAngle" but I don't think postscript does*/
    /* PS does define an italicAngle, but presumably that's the general italic*/
    /* angle we output earlier */
    /* UniqueID is obsolete */
    PListAddInteger(dictnode,"postscriptUnderlineThickness",sf->uwidth);
    PListAddInteger(dictnode,"postscriptUnderlinePosition",sf->upos);
    if ( sf->private!=NULL ) {
	char *pt;
	PListAddPrivateArray(dictnode, "BlueValues", sf->private);
	PListAddPrivateArray(dictnode, "OtherBlues", sf->private);
	PListAddPrivateArray(dictnode, "FamilyBlues", sf->private);
	PListAddPrivateArray(dictnode, "FamilyOtherBlues", sf->private);
	PListAddPrivateArray(dictnode, "StemSnapH", sf->private);
	PListAddPrivateArray(dictnode, "StemSnapV", sf->private);
	PListAddPrivateThing(dictnode, "BlueFuzz", sf->private, "integer");
	PListAddPrivateThing(dictnode, "BlueShift", sf->private, "integer");
	PListAddPrivateThing(dictnode, "BlueScale", sf->private, "real");
	if ( (pt=PSDictHasEntry(sf->private,"ForceBold"))!=NULL &&
		strstr(pt,"true")!=NULL )
	    PListAddBoolean(dictnode, "postscriptForceBold", true );
    }
    if ( sf->fondname!=NULL )
    PListAddString(dictnode,"macintoshFONDName",sf->fondname);
    // TODO: Output unrecognized data.
    char *fname = buildname(basedir, "fontinfo.plist"); // Build the file name.
    xmlSaveFormatFileEnc(fname, plistdoc, "UTF-8", 1); // Store the document.
    free(fname); fname = NULL;
    xmlFreeDoc(plistdoc); // Free the memory.
    xmlCleanupParser();
    return true;
return true;
}

static int UFOOutputGroups(const char *basedir, const SplineFont *sf) {
    /* These don't act like fontforge's groups. There are comments that this */
    /*  could be used for defining classes (kerning classes, etc.) but no */
    /*  resolution saying that the actually are. */
    /* Should I omit a file I don't use? Or leave it blank? */
    xmlDocPtr plistdoc = PlistInit(); if (plistdoc == NULL) return false; // Make the document.
    xmlNodePtr rootnode = xmlDocGetRootElement(plistdoc); if (rootnode == NULL) { xmlFreeDoc(plistdoc); return false; } // Find the root node.
    xmlNodePtr dictnode = xmlNewChild(rootnode, NULL, BAD_CAST "dict", NULL); if (rootnode == NULL) { xmlFreeDoc(plistdoc); return false; } // Add the dict.
    // TODO: Maybe output things.
    char *fname = buildname(basedir, "groups.plist"); // Build the file name.
    xmlSaveFormatFileEnc(fname, plistdoc, "UTF-8", 1); // Store the document.
    free(fname); fname = NULL;
    xmlFreeDoc(plistdoc); // Free the memory.
    xmlCleanupParser();
    return true;
}

static void KerningPListAddGlyph(xmlNodePtr parent, const char *key, const KernPair *kp) {
    xmlNewChild(parent, NULL, BAD_CAST "key", BAD_CAST key); // "<key>%s</key>" key
    xmlNodePtr dictxml = xmlNewChild(parent, NULL, BAD_CAST "dict", NULL); // "<dict>"
    while ( kp!=NULL ) {
      xmlNewChildInteger(dictxml, NULL, BAD_CAST kp->sc->name, kp->off); // "<key>%s</key><integer>%d</integer>" kp->sc->name kp->off
      kp = kp->next;
    }
}

static int UFOOutputKerning(const char *basedir, const SplineFont *sf) {
    SplineChar *sc;
    int i;

    xmlDocPtr plistdoc = PlistInit(); if (plistdoc == NULL) return false; // Make the document.
    xmlNodePtr rootnode = xmlDocGetRootElement(plistdoc); if (rootnode == NULL) { xmlFreeDoc(plistdoc); return false; } // Find the root node.
    xmlNodePtr dictnode = xmlNewChild(rootnode, NULL, BAD_CAST "dict", NULL); if (rootnode == NULL) { xmlFreeDoc(plistdoc); return false; } // Add the dict.

    for ( i=0; i<sf->glyphcnt; ++i ) if ( SCWorthOutputting(sc=sf->glyphs[i]) && sc->kerns!=NULL )
	KerningPListAddGlyph(dictnode,sc->name,sc->kerns);

    char *fname = buildname(basedir, "kerning.plist"); // Build the file name.
    xmlSaveFormatFileEnc(fname, plistdoc, "UTF-8", 1); // Store the document.
    free(fname); fname = NULL;
    xmlFreeDoc(plistdoc); // Free the memory.
    xmlCleanupParser();
    return true;
}

static int UFOOutputVKerning(const char *basedir, const SplineFont *sf) {
    SplineChar *sc;
    int i;

    xmlDocPtr plistdoc = PlistInit(); if (plistdoc == NULL) return false; // Make the document.
    xmlNodePtr rootnode = xmlDocGetRootElement(plistdoc); if (rootnode == NULL) { xmlFreeDoc(plistdoc); return false; } // Find the root node.
    xmlNodePtr dictnode = xmlNewChild(rootnode, NULL, BAD_CAST "dict", NULL); if (rootnode == NULL) { xmlFreeDoc(plistdoc); return false; } // Add the dict.

    for ( i=sf->glyphcnt-1; i>=0; --i ) if ( SCWorthOutputting(sc=sf->glyphs[i]) && sc->vkerns!=NULL ) break;
    if ( i<0 ) return( true );
    for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL && sc->vkerns!=NULL )
	KerningPListAddGlyph(dictnode,sc->name,sc->vkerns);

    char *fname = buildname(basedir, "vkerning.plist"); // Build the file name.
    xmlSaveFormatFileEnc(fname, plistdoc, "UTF-8", 1); // Store the document.
    free(fname); fname = NULL;
    xmlFreeDoc(plistdoc); // Free the memory.
    xmlCleanupParser();
    return true;
}

static int UFOOutputLib(const char *basedir, const SplineFont *sf) {
#ifndef _NO_PYTHON
    if ( sf->python_persistent==NULL || PyMapping_Check(sf->python_persistent) == 0) return true;

    xmlDocPtr plistdoc = PlistInit(); if (plistdoc == NULL) return false; // Make the document.
    xmlNodePtr rootnode = xmlDocGetRootElement(plistdoc); if (rootnode == NULL) return false; // Find the root node.

    xmlNodePtr dictnode = PythonLibToXML(sf->python_persistent,NULL);
    xmlAddChild(rootnode, dictnode);

    char *fname = buildname(basedir, "lib.plist"); // Build the file name.
    xmlSaveFormatFileEnc(fname, plistdoc, "UTF-8", 1); // Store the document.
    free(fname); fname = NULL;
    xmlFreeDoc(plistdoc); // Free the memory.
    xmlCleanupParser();
#endif
return( true );
}

static int UFOOutputFeatures(const char *basedir, SplineFont *sf) {
    char *fname = buildname(basedir,"features.fea");
    FILE *feats = fopen( fname, "w" );
    int err;

    free(fname);
    if ( feats==NULL )
return( false );
    FeatDumpFontLookups(feats,sf);
    err = ferror(feats);
    fclose(feats);
return( !err );
}

static size_t count_caps(const char * input) {
  size_t count = 0;
  for (int i = 0; input[i] != '\0'; i++) {
    if ((input[i] >= 'A') && (input[i] <= 'Z')) count ++;
  }
  return count;
}

static char * upper_case(const char * input) {
  size_t output_length = strlen(input);
  char * output = malloc(output_length + 1);
  off_t pos = 0;
  if (output == NULL) return NULL;
  while (pos < output_length) {
    if ((input[pos] >= 'a') && (input[pos] <= 'z')) {
      output[pos] = (char)(((unsigned char) input[pos]) - 0x20U);
    } else {
      output[pos] = input[pos];
    }
    pos++;
  }
  output[pos] = '\0';
  return output;
}

static char * same_case(const char * input) {
  size_t output_length = strlen(input);
  char * output = malloc(output_length + 1);
  off_t pos = 0;
  if (output == NULL) return NULL;
  while (pos < output_length) {
    output[pos] = input[pos];
    pos++;
  }
  output[pos] = '\0';
  return output;
}

static char * delimit_null(const char * input, char delimiter) {
  size_t output_length = strlen(input);
  char * output = malloc(output_length + 1);
  if (output == NULL) return NULL;
  off_t pos = 0;
  while (pos < output_length) {
    if (input[pos] == delimiter) {
      output[pos] = '\0';
    } else {
      output[pos] = input[pos];
    }
    pos++;
  }
  return output;
}

const char * DOS_reserved[12] = {"CON", "PRN", "AUX", "CLOCK$", "NUL", "COM1", "COM2", "COM3", "COM4", "LPT1", "LPT2", "LPT3"};
const int DOS_reserved_count = 12;

int polyMatch(const char * input, int reference_count, const char ** references) {
  for (off_t pos = 0; pos < reference_count; pos++) {
    if (strcmp(references[pos], input) == 0) return 1;
  }
  return 0;
}

int is_DOS_drive(char * input) {
  if ((input != NULL) &&
     (strlen(input) == 2) &&
     (((input[0] >= 'A') && (input[0] <= 'Z')) || ((input[0] >= 'a') && (input[0] <= 'z'))) &&
     (input[1] == ':'))
       return 1;
  return 0;
}

// Because Windows does not offer strtok_r but does offer strtok_s, which is almost identical,
// we add a simple compatibility layer.
// It might be nice to move support functions into a dedicated file at some point.
static char * strtok_r_ff_ufo(char *s1, const char *s2, char **lasts) {
#ifdef __MINGW32__
      return strtok_s(s1, s2, lasts);
#else
      return strtok_r(s1, s2, lasts);
#endif
}

char * ufo_name_mangle(const char * input, const char * prefix, const char * suffix, int flags) {
  // This does not append the prefix or the suffix.
  // flags & 1 determines whether to post-pad caps (something implemented in the standard).
  // flags & 2 determines whether to replace a leading '.' (standard).
  // flags & 4 determines whether to restrict DOS names (standard).
  // flags & 8 determines whether to implement additional character restrictions.
  // The specification lists '"' '*' '+' '/' ':' '<' '>' '?' '[' '\\' ']' '|'
  // and also anything in the range 0x00-0x1F and 0x7F.
  // Our additional restriction list includes '\'' '&' '%' '$' '#' '`' '=' '!' ';'
  // Standard behavior comes from passing a flags value of 7.
  const char * standard_restrict = "\"*+/:<>?[]\\]|";
  const char * extended_restrict = "\'&%$#`=!;";
  size_t prefix_length = strlen(prefix);
  size_t max_length = 255 - prefix_length - strlen(suffix);
  size_t input_length = strlen(input);
  size_t stop_pos = ((max_length < input_length) ? max_length : input_length); // Minimum.
  size_t output_length_1 = input_length;
  if (flags & 1) output_length_1 += count_caps(input); // Add space for underscore pads on caps.
  off_t output_pos = 0;
  char * output = malloc(output_length_1 + 1);
  for (int i = 0; i < input_length; i++) {
    if (strchr(standard_restrict, input[i]) || (input[i] <= 0x1F) || (input[i] >= 0x7F)) {
      output[output_pos++] = '_'; // If the character is restricted, place an underscore.
    } else if ((flags & 8) && strchr(extended_restrict, input[i])) {
      output[output_pos++] = '_'; // If the extended restriction list is enabled and matches, ....
    } else if ((flags & 1) && (input[i] >= 'A') && (input[i] <= 'Z')) {
      output[output_pos++] = input[i];
      output[output_pos++] = '_'; // If we have a capital letter, we post-pad if desired.
    } else if ((flags & 2) && (i == 0) && (prefix_length == 0) && (input[i] == '.')) {
      output[output_pos++] = '_'; // If we have a leading '.', we convert to an underscore.
    } else {
      output[output_pos++] = input[i];
    }
  }
  output[output_pos] = '\0';
  if (output_pos > max_length) {
    output[max_length] = '\0';
  }
  char * output2 = NULL;
  off_t output2_pos = 0;
  {
    char * disposable = malloc(output_length_1 + 1);
    strcpy(disposable, output); // strtok rewrites the input string, so we make a copy.
    output2 = malloc((2 * output_length_1) + 1); // It's easier to pad than to calculate.
    output2_pos = 0;
    char * saveptr = NULL;
    char * current = strtok_r_ff_ufo(disposable, ".", &saveptr); // We get the first name part.
    while (current != NULL) {
      char * uppered = upper_case(output);
      if (polyMatch(uppered, DOS_reserved_count, DOS_reserved) || is_DOS_drive(uppered)) {
        output2[output2_pos++] = '_'; // Prefix an underscore if it's a reserved name.
      }
      free(uppered); uppered = NULL;
      for (off_t parti = 0; current[parti] != '\0'; parti++) {
        output2[output2_pos++] = current[parti];
      }
      current = strtok_r_ff_ufo(NULL, ".", &saveptr);
      if (current != NULL) output2[output2_pos++] = '.';
    }
    output2[output2_pos] = '\0';
    output2 = realloc(output2, output2_pos + 1);
    free(disposable); disposable = NULL;
  }
  free(output); output = NULL;
  return output2;
}

#ifdef FF_UTHASH_GLIF_NAMES
#include "glif_name_hash.h"
#endif

char * ufo_name_number(
#ifdef FF_UTHASH_GLIF_NAMES
struct glif_name_index * glif_name_hash,
#else
void * glif_name_hash,
#endif
int index, const char * input, const char * prefix, const char * suffix, int flags) {
        // This does not append the prefix or the suffix.
        // The specification deals with name collisions by appending a 15-digit decimal number to the name.
        // But the name length cannot exceed 255 characters, so it is necessary to crop the base name if it is too long.
        // Name exclusions are case insensitive, so we uppercase.
        char * name_numbered = upper_case(input);
        char * full_name_base = same_case(input); // This is in case we do not need a number added.
        if (strlen(input) > (255 - strlen(prefix) - strlen(suffix))) {
          // If the numbered name base is too long, we crop it, even if we are not numbering.
          full_name_base[(255 - strlen(suffix))] = '\0';
          full_name_base = realloc(full_name_base, ((255 - strlen(prefix) - strlen(suffix)) + 1));
        }
        char * name_base = same_case(input); // This is in case we need a number added.
        long int name_number = 0;
#ifdef FF_UTHASH_GLIF_NAMES
        if (glif_name_hash != NULL) {
          if (strlen(input) > (255 - 15 - strlen(prefix) - strlen(suffix))) {
            // If the numbered name base is too long, we crop it.
            name_base[(255 - 15 - strlen(suffix))] = '\0';
            name_base = realloc(name_base, ((255 - 15 - strlen(prefix) - strlen(suffix)) + 1));
          }
          // Check the resulting name against a hash table of names.
          if (glif_name_search_glif_name(glif_name_hash, name_numbered) != NULL) {
            // If the name is taken, we must make space for a 15-digit number.
            char * name_base_upper = upper_case(name_base);
            while (glif_name_search_glif_name(glif_name_hash, name_numbered) != NULL) {
              name_number++; // Remangle the name until we have no more matches.
              free(name_numbered); name_numbered = NULL;
              asprintf(&name_numbered, "%s%15ld", name_base_upper, name_number);
            }
            free(name_base_upper); name_base_upper = NULL;
          }
          // Insert the result into the hash table.
          glif_name_track_new(glif_name_hash, index, name_numbered);
        }
#endif
        // Now we want the correct capitalization.
        free(name_numbered); name_numbered = NULL;
        if (name_number > 0) {
          asprintf(&name_numbered, "%s%15ld", name_base, name_number);
        } else {
          asprintf(&name_numbered, "%s", full_name_base);
        }
        free(name_base); name_base = NULL;
        free(full_name_base); full_name_base = NULL;
        return name_numbered;
}

int WriteUFOLayer(const char * glyphdir, SplineFont * sf, int layer) {
    xmlDocPtr plistdoc = PlistInit(); if (plistdoc == NULL) return false; // Make the document.
    xmlNodePtr rootnode = xmlDocGetRootElement(plistdoc); if (rootnode == NULL) { xmlFreeDoc(plistdoc); return false; } // Find the root node.
    xmlNodePtr dictnode = xmlNewChild(rootnode, NULL, BAD_CAST "dict", NULL); if (rootnode == NULL) { xmlFreeDoc(plistdoc); return false; } // Make the dict.

    GFileMkDir( glyphdir );
    int i;
    SplineChar * sc;
    int err;
    for ( i=0; i<sf->glyphcnt; ++i ) if ( SCWorthOutputting(sc=sf->glyphs[i]) ) {
	PListAddString(dictnode,sc->name,sc->glif_name); // Add the glyph to the table of contents.
        // TODO: Optionally skip rewriting an untouched glyph.
        // Do we track modified glyphs carefully enough for this?
	err |= !GlifDump(glyphdir,sc->glif_name,sc,layer);
    }

    char *fname = buildname(glyphdir, "contents.plist"); // Build the file name for the contents.
    xmlSaveFormatFileEnc(fname, plistdoc, "UTF-8", 1); // Store the document.
    free(fname); fname = NULL;
    xmlFreeDoc(plistdoc); // Free the memory.
    xmlCleanupParser();
    return err;
}

int WriteUFOFontFlex(const char *basedir, SplineFont *sf, enum fontformat ff,int flags,
	const EncMap *map,int layer, int all_layers) {
    char *foo = NULL, *glyphdir, *gfname;
    int err;
    FILE *plist;
    int i;
    SplineChar *sc;

    /* Clean it out, if it exists */
    if (asprintf(&foo, "rm -rf %s", basedir) >= 0) {
      if (system( foo ) == -1) fprintf(stderr, "Error clearing %s.\n", basedir);
      free( foo ); foo = NULL;
    }

    /* Create it */
    GFileMkDir( basedir );

    err  = !UFOOutputMetaInfo(basedir,sf);
    err |= !UFOOutputFontInfo(basedir,sf,layer);
    err |= !UFOOutputGroups(basedir,sf);
    err |= !UFOOutputKerning(basedir,sf);
    err |= !UFOOutputVKerning(basedir,sf);
    err |= !UFOOutputLib(basedir,sf);
    err |= !UFOOutputFeatures(basedir,sf);

    if ( err )
return( false );

#ifdef FF_UTHASH_GLIF_NAMES
    struct glif_name_index _glif_name_hash;
    struct glif_name_index * glif_name_hash = &_glif_name_hash; // Open the hash table.
    memset(glif_name_hash, 0, sizeof(struct glif_name_index));
#else
    void * glif_name_hash = NULL;
#endif
    // First we generate glif names.
    for ( i=0; i<sf->glyphcnt; ++i ) if ( SCWorthOutputting(sc=sf->glyphs[i]) ) {
        char * startname = NULL;
        if (sc->glif_name != NULL)
          startname = strdup(sc->glif_name); // If the splinechar has a glif name, try to use that.
        else
          startname = ufo_name_mangle(sc->name, "", ".glif", 7); // If not, call the mangler.
        // Number the name (as the specification requires) if there is a collision.
        // And add it to the hash table with its index.
        char * numberedname = ufo_name_number(glif_name_hash, i, startname, "", ".glif", 7);
        free(startname); startname = NULL;
        char * final_name = NULL;
        asprintf(&final_name, "%s%s%s", "", numberedname, ".glif"); // Generate the final name with prefix and suffix.
        free(numberedname); numberedname = NULL;
        // We update the saved glif_name only if it is different (so as to minimize churn).
        if ((sc->glif_name != NULL) && (strcmp(sc->glif_name, final_name) != 0))
          free(sc->glif_name); sc->glif_name = NULL;
        if (sc->glif_name == NULL)
          sc->glif_name = final_name;
        else
	  free(final_name); final_name = NULL;
    }
#ifdef FF_UTHASH_GLIF_NAMES
    glif_name_hash_destroy(glif_name_hash); // Close the hash table.
#endif
    
    if (all_layers) {
#ifdef FF_UTHASH_GLIF_NAMES
      struct glif_name_index _layer_name_hash;
      struct glif_name_index * layer_name_hash = &_layer_name_hash; // Open the hash table.
      memset(layer_name_hash, 0, sizeof(struct glif_name_index));
      struct glif_name_index _layer_path_hash;
      struct glif_name_index * layer_path_hash = &_layer_path_hash; // Open the hash table.
      memset(layer_path_hash, 0, sizeof(struct glif_name_index));
#else
      void * layer_name_hash = NULL;
#endif
      xmlDocPtr plistdoc = PlistInit(); if (plistdoc == NULL) return false; // Make the document.
      xmlNodePtr rootnode = xmlDocGetRootElement(plistdoc); if (rootnode == NULL) { xmlFreeDoc(plistdoc); return false; } // Find the root node.
      xmlNodePtr arraynode = xmlNewChild(rootnode, NULL, BAD_CAST "array", NULL); if (rootnode == NULL) { xmlFreeDoc(plistdoc); return false; } // Make the dict.
	
      int layer_pos;
      for (layer_pos = 0; layer_pos < sf->layer_cnt; layer_pos++) {
        glyphdir = buildname(basedir,"glyphs");
        xmlNodePtr layernode = xmlNewChild(arraynode, NULL, BAD_CAST "array", NULL);
        // We make the layer name.
        char * layer_name_start = NULL;
        if (layer_pos == ly_fore) layer_name_start = "public.default";
        else if (layer_pos == ly_back) layer_name_start = "public.background";
        else layer_name_start = sf->layers[layer_pos].name;
        if (layer_name_start == NULL) layer_name_start = "unnamed"; // The remangle step adds any needed numbers.
        char * numberedlayername = ufo_name_number(layer_name_hash, layer_pos, layer_name_start, "", "", 7);
        // We make the layer path.
        char * layer_path_start = NULL;
        char * numberedlayerpath = NULL;
        if (layer_pos == ly_fore) {
          numberedlayerpath = strdup("glyphs");
        } else if (sf->layers[layer_pos].ufo_path != NULL) {
          layer_path_start = strdup(sf->layers[layer_pos].ufo_path);
          numberedlayerpath = ufo_name_number(layer_path_hash, layer_pos, layer_path_start, "glyphs.", "", 7);
        } else {
          layer_path_start = ufo_name_mangle(sf->layers[layer_pos].name, "glyphs.", "", 7);
          numberedlayerpath = ufo_name_number(layer_path_hash, layer_pos, layer_path_start, "glyphs.", "", 7);
        }
        if (layer_path_start != NULL) { free(layer_path_start); layer_path_start = NULL; }
        // We write to the layer contents.
        xmlNewChild(layernode, NULL, BAD_CAST "string", numberedlayername);
        xmlNewChild(layernode, NULL, BAD_CAST "string", numberedlayerpath);
        // We write the glyph directory.
        err |= WriteUFOLayer(glyphdir, sf, layer_pos);
        free(numberedlayername); numberedlayername = NULL;
        free(numberedlayerpath); numberedlayerpath = NULL;
      }
      char *fname = buildname(basedir, "layercontents.plist"); // Build the file name for the contents.
      xmlSaveFormatFileEnc(fname, plistdoc, "UTF-8", 1); // Store the document.
      free(fname); fname = NULL;
      xmlFreeDoc(plistdoc); // Free the memory.
      xmlCleanupParser();
#ifdef FF_UTHASH_GLIF_NAMES
      glif_name_hash_destroy(layer_name_hash); // Close the hash table.
      glif_name_hash_destroy(layer_path_hash); // Close the hash table.
#endif
    } else {
        glyphdir = buildname(basedir,"glyphs");
        WriteUFOLayer(glyphdir, sf, layer);
    }

    free( glyphdir );
return( !err );
}

int WriteUFOFont(const char *basedir, SplineFont *sf, enum fontformat ff, int flags,
	const EncMap *map, int layer) {
  return WriteUFOFontFlex(basedir, sf, ff, flags, map, layer, 0);
}

/* ************************************************************************** */
/* *****************************    UFO Input    **************************** */
/* ************************************************************************** */

static char *get_thingy(FILE *file,char *buffer,char *tag) {
    int ch;
    char *pt;

    for (;;) {
	while ( (ch=getc(file))!='<' && ch!=EOF );
	if ( ch==EOF )
return( NULL );
	while ( (ch=getc(file))!=EOF && isspace(ch) );
	pt = tag;
	while ( ch==*pt || tolower(ch)==*pt ) {
	    ++pt;
	    ch = getc(file);
	}
	if ( *pt=='\0' )
    continue;
	if ( ch==EOF )
return( NULL );
	while ( isspace(ch)) ch=getc(file);
	if ( ch!='>' )
    continue;
	pt = buffer;
	while ( (ch=getc(file))!='<' && ch!=EOF && pt<buffer+1000)
	    *pt++ = ch;
	*pt = '\0';
return( buffer );
    }
}

char **NamesReadUFO(char *filename) {
    char *fn = buildname(filename,"fontinfo.plist");
    FILE *info = fopen(fn,"r");
    char buffer[1024];
    char **ret;

    free(fn);
    if ( info==NULL )
return( NULL );
    while ( get_thingy(info,buffer,"key")!=NULL ) {
	if ( strcmp(buffer,"fontName")!=0 ) {
	    if ( get_thingy(info,buffer,"string")!=NULL ) {
		ret = calloc(2,sizeof(char *));
		ret[0] = copy(buffer);
		fclose(info);
return( ret );
	    }
	    fclose(info);
return( NULL );
	}
    }
    fclose(info);
return( NULL );
}

#include <libxml/parser.h>

static int libxml_init_base() {
return( true );
}

static xmlNodePtr FindNode(xmlNodePtr kids,char *name) {
    while ( kids!=NULL ) {
	if ( xmlStrcmp(kids->name,(const xmlChar *) name)== 0 )
return( kids );
	kids = kids->next;
    }
return( NULL );
}

#ifndef _NO_PYTHON
static PyObject *XMLEntryToPython(xmlDocPtr doc,xmlNodePtr entry);

static PyObject *LibToPython(xmlDocPtr doc,xmlNodePtr dict) {
	// This function is responsible for parsing keys in dicts.
    PyObject *pydict = PyDict_New();
    PyObject *item = NULL;
    xmlNodePtr keys, temp;

	// Get the first item, then iterate through all items in the dict.
    for ( keys=dict->children; keys!=NULL; keys=keys->next ) {
		// See that the item is in fact a key.
		if ( xmlStrcmp(keys->name,(const xmlChar *) "key")== 0 ) {
			// Fetch the name, which, according to the libxml specification, is the first child.
			char *keyname = (char *) xmlNodeListGetString(doc,keys->children,true);
			// In a property list, the value is a sibling of the key name.
			// Iterate through the following siblings (including keys (!)) until we find a text entry.
			for ( temp=keys->next; temp!=NULL; temp=temp->next ) {
				if ( xmlStrcmp(temp->name,(const xmlChar *) "text")!=0 ) break;
			}
			// Convert the X.M.L. entry into a Python object.
			item = NULL;
			if ( temp!=NULL) item = XMLEntryToPython(doc,temp);
			if ( item!=NULL ) PyDict_SetItemString(pydict, keyname, item );
			if ( temp==NULL ) break;
			else if ( xmlStrcmp(temp->name,(const xmlChar *) "key")!=0 ) keys = temp->next;
			// If and only if the parsing succeeds, jump over any entries we read when searching for a text block.
			free(keyname);
		}
    }
return( pydict );
}

static PyObject *XMLEntryToPython(xmlDocPtr doc,xmlNodePtr entry) {
    char *contents;

    if ( xmlStrcmp(entry->name,(const xmlChar *) "true")==0 ) {
	Py_INCREF(Py_True);
return( Py_True );
    }
    if ( xmlStrcmp(entry->name,(const xmlChar *) "false")==0 ) {
	Py_INCREF(Py_False);
return( Py_False );
    }
    if ( xmlStrcmp(entry->name,(const xmlChar *) "none")==0 ) {
	Py_INCREF(Py_None);
return( Py_None );
    }

    if ( xmlStrcmp(entry->name,(const xmlChar *) "dict")==0 )
return( LibToPython(doc,entry));
    if ( xmlStrcmp(entry->name,(const xmlChar *) "array")==0 ) {
	xmlNodePtr sub;
	int cnt;
	PyObject *ret, *item;
	/* I'm translating "Arrays" as tuples... not sure how to deal with */
	/*  actual python arrays. But these look more like tuples than arrays*/
	/*  since each item gets its own type */

	for ( cnt=0, sub=entry->children; sub!=NULL; sub=sub->next ) {
	    if ( xmlStrcmp(sub->name,(const xmlChar *) "text")==0 )
	continue;
	    ++cnt;
	}
	ret = PyTuple_New(cnt);
	for ( cnt=0, sub=entry->children; sub!=NULL; sub=sub->next ) {
	    if ( xmlStrcmp(sub->name,(const xmlChar *) "text")==0 )
	continue;
	    item = XMLEntryToPython(doc,sub);
	    if ( item==NULL ) {
		item = Py_None;
		Py_INCREF(item);
	    }
	    PyTuple_SetItem(ret,cnt,item);
	    ++cnt;
	}
return( ret );
    }

    contents = (char *) xmlNodeListGetString(doc,entry->children,true);
    if ( xmlStrcmp(entry->name,(const xmlChar *) "integer")==0 ) {
	long val = strtol(contents,NULL,0);
	free(contents);
return( Py_BuildValue("i",val));
    }
    if ( xmlStrcmp(entry->name,(const xmlChar *) "real")==0 ) {
	double val = strtod(contents,NULL);
	free(contents);
return( Py_BuildValue("d",val));
    }
    if ( xmlStrcmp(entry->name,(const xmlChar *) "string")==0 ) {
	PyObject *ret = Py_BuildValue("s",contents);
	free(contents);
return( ret );
    }
    LogError(_("Unknown python type <%s> when reading UFO/GLIF lib data."), (char *) entry->name);
    free( contents );
return( NULL );
}
#endif

static StemInfo *GlifParseHints(xmlDocPtr doc,xmlNodePtr dict,char *hinttype) {
    StemInfo *head=NULL, *last=NULL, *h;
    xmlNodePtr keys, array, kids, poswidth,temp;
    double pos, width;

    for ( keys=dict->children; keys!=NULL; keys=keys->next ) {
	if ( xmlStrcmp(keys->name,(const xmlChar *) "key")== 0 ) {
	    char *keyname = (char *) xmlNodeListGetString(doc,keys->children,true);
	    int found = strcmp(keyname,hinttype)==0;
	    free(keyname);
	    if ( found ) {
		for ( array=keys->next; array!=NULL; array=array->next ) {
		    if ( xmlStrcmp(array->name,(const xmlChar *) "array")==0 )
		break;
		}
		if ( array!=NULL ) {
		    for ( kids = array->children; kids!=NULL; kids=kids->next ) {
			if ( xmlStrcmp(kids->name,(const xmlChar *) "dict")==0 ) {
			    pos = -88888888; width = 0;
			    for ( poswidth=kids->children; poswidth!=NULL; poswidth=poswidth->next ) {
				if ( xmlStrcmp(poswidth->name,(const xmlChar *) "key")==0 ) {
				    char *keyname2 = (char *) xmlNodeListGetString(doc,poswidth->children,true);
				    int ispos = strcmp(keyname2,"position")==0, iswidth = strcmp(keyname2,"width")==0;
				    double value;
				    free(keyname2);
				    for ( temp=poswidth->next; temp!=NULL; temp=temp->next ) {
					if ( xmlStrcmp(temp->name,(const xmlChar *) "text")!=0 )
				    break;
				    }
				    if ( temp!=NULL ) {
					char *valname = (char *) xmlNodeListGetString(doc,temp->children,true);
					if ( xmlStrcmp(temp->name,(const xmlChar *) "integer")==0 )
					    value = strtol(valname,NULL,10);
					else if ( xmlStrcmp(temp->name,(const xmlChar *) "real")==0 )
					    value = strtod(valname,NULL);
					else
					    ispos = iswidth = false;
					free(valname);
					if ( ispos )
					    pos = value;
					else if ( iswidth )
					    width = value;
					poswidth = temp;
				    }
				}
			    }
			    if ( pos!=-88888888 && width!=0 ) {
				h = chunkalloc(sizeof(StemInfo));
			        h->start = pos;
			        h->width = width;
			        if ( width==-20 || width==-21 )
				    h->ghost = true;
				if ( head==NULL )
				    head = last = h;
				else {
				    last->next = h;
			            last = h;
				}
			    }
			}
		    }
		}
	    }
	}
    }
return( head );
}

static SplineChar *_UFOLoadGlyph(SplineFont *sf, xmlDocPtr doc, char *glifname, char* glyphname, SplineChar* existingglyph, int layerdest) {
    xmlNodePtr glyph, kids, contour, points;
    SplineChar *sc;
    xmlChar *format, *width, *height, *u;
    char *name, *tmpname;
    int uni;
    char *cpt;
    SplineSet *last = NULL;
	int newsc = 0;

    glyph = xmlDocGetRootElement(doc);
    format = xmlGetProp(glyph,(xmlChar *) "format");
    if ( xmlStrcmp(glyph->name,(const xmlChar *) "glyph")!=0 ||
	    (format!=NULL && xmlStrcmp(format,(xmlChar *) "1")!=0)) {
		LogError(_("Expected glyph file with format==1"));
		xmlFreeDoc(doc);
		free(format);
		return( NULL );
    }
	free(format);
	tmpname = (char *) xmlGetProp(glyph,(xmlChar *) "name");
	if (glyphname != NULL) {
		// We use the provided name from the glyph listing since the specification says to trust that one more.
		name = copy(glyphname);
		// But we still fetch the internally listed name for verification and fail on a mismatch.
		if ((name == NULL) || ((name != NULL) && (tmpname != NULL) && (strcmp(glyphname, name) != 0))) {
			LogError(_("Bad glyph name."));
			if ( tmpname != NULL ) { free(tmpname); tmpname = NULL; }
			if ( name != NULL ) { free(name); name = NULL; }
			xmlFreeDoc(doc);
			return NULL;
		}
		if ( tmpname != NULL ) { free(tmpname); tmpname = NULL; }
	} else {
		name = tmpname;
	}
    if ( name==NULL && glifname!=NULL ) {
		char *pt = strrchr(glifname,'/');
		name = copy(pt+1);
		for ( pt=cpt=name; *cpt!='\0'; ++cpt ) {
			if ( *cpt!='_' )
			*pt++ = *cpt;
			else if ( islower(*name))
			*name = toupper(*name);
		}
		*pt = '\0';
    } else if ( name==NULL )
		name = copy("nameless");
	// We assign a placeholder name if no name exists.
	// We create a new SplineChar 
	if (existingglyph != NULL) {
		sc = existingglyph;
		free(name); name = NULL;
	} else {
    	sc = SplineCharCreate(2);
    	sc->name = name;
		newsc = 1;
	}
	if (sc == NULL) {
		xmlFreeDoc(doc);
		return NULL;
	}
    last = NULL;
	// Check layer availability here.
	if ( layerdest>=sc->layer_cnt ) {
		sc->layers = realloc(sc->layers,(layerdest+1)*sizeof(Layer));
		memset(sc->layers+sc->layer_cnt,0,(layerdest+1-sc->layer_cnt)*sizeof(Layer));
		sc->layer_cnt = layerdest + 1;
	}
	if (sc->layers == NULL) {
		if ((newsc == 1) && (sc != NULL)) {
			SplineCharFree(sc);
		}
		xmlFreeDoc(doc);
		return NULL;
	}
    for ( kids = glyph->children; kids!=NULL; kids=kids->next ) {
	if ( xmlStrcmp(kids->name,(const xmlChar *) "advance")==0 ) {
		if ((layerdest == ly_fore) || newsc) {
			width = xmlGetProp(kids,(xmlChar *) "width");
			height = xmlGetProp(kids,(xmlChar *) "height");
			if ( width!=NULL )
			sc->width = strtol((char *) width,NULL,10);
			if ( height!=NULL )
			sc->vwidth = strtol((char *) height,NULL,10);
			sc->widthset = true;
			free(width); free(height);
		}
	} else if ( xmlStrcmp(kids->name,(const xmlChar *) "unicode")==0 ) {
		if ((layerdest == ly_fore) || newsc) {
			u = xmlGetProp(kids,(xmlChar *) "hex");
			uni = strtol((char *) u,NULL,16);
			if ( sc->unicodeenc == -1 )
			sc->unicodeenc = uni;
			else
			AltUniAdd(sc,uni);
			free(u);
		}
	} else if ( xmlStrcmp(kids->name,(const xmlChar *) "outline")==0 ) {
	    for ( contour = kids->children; contour!=NULL; contour=contour->next ) {
		if ( xmlStrcmp(contour->name,(const xmlChar *) "component")==0 ) {
		    char *base = (char *) xmlGetProp(contour,(xmlChar *) "base"),
			*xs = (char *) xmlGetProp(contour,(xmlChar *) "xScale"),
			*ys = (char *) xmlGetProp(contour,(xmlChar *) "yScale"),
			*xys = (char *) xmlGetProp(contour,(xmlChar *) "xyScale"),
			*yxs = (char *) xmlGetProp(contour,(xmlChar *) "yxScale"),
			*xo = (char *) xmlGetProp(contour,(xmlChar *) "xOffset"),
			*yo = (char *) xmlGetProp(contour,(xmlChar *) "yOffset");
		    RefChar *r;
		    if ( base==NULL )
				LogError(_("component with no base glyph"));
		    else {
				r = RefCharCreate();
				r->sc = SplineCharCreate(0);
				r->sc->name = base;
				r->transform[0] = r->transform[3] = 1;
				if ( xs!=NULL )
					r->transform[0] = strtod(xs,NULL);
				if ( ys!=NULL )
					r->transform[3] = strtod(ys,NULL);
				if ( xys!=NULL )
					r->transform[1] = strtod(xys,NULL);
				if ( yxs!=NULL )
					r->transform[2] = strtod(yxs,NULL);
				if ( xo!=NULL )
					r->transform[4] = strtod(xo,NULL);
				if ( yo!=NULL )
					r->transform[5] = strtod(yo,NULL);
				r->next = sc->layers[layerdest].refs;
				sc->layers[layerdest].refs = r;
		    }
		    free(xs); free(ys); free(xys); free(yxs); free(xo); free(yo);
		} else if ( xmlStrcmp(contour->name,(const xmlChar *) "contour")==0 ) {
		    xmlNodePtr npoints;
		    SplineSet *ss;
		    SplinePoint *sp;
			SplinePoint *sp2;
		    BasePoint pre[2], init[4];
		    int precnt=0, initcnt=0, open=0;
			// precnt seems to count control points leading into the next on-curve point. pre stores those points.
			// initcnt counts the control points that appear before the first on-curve point. This can get updated at the beginning and/or the end of the list.
			// This is important for determining the order of the closing curve.
			// A further improvement would be to prefetch the entire list so as to know the declared order of a curve before processing the point.

			// We now look for anchor points.
            char *sname;

            for ( points=contour->children; points!=NULL; points=points->next )
                if ( xmlStrcmp(points->name,(const xmlChar *) "point")==0 )
            break;
            for ( npoints=points->next; npoints!=NULL; npoints=npoints->next )
                if ( xmlStrcmp(npoints->name,(const xmlChar *) "point")==0 )
            break;
			// If the contour has a single point without another point after it, we assume it to be an anchor point.
            if ( points!=NULL && npoints==NULL ) {
                sname = (char *) xmlGetProp(points, (xmlChar *) "name");
                if ( sname!=NULL) {
                    /* make an AP and if necessary an AC */
                    AnchorPoint *ap = chunkalloc(sizeof(AnchorPoint));
                    AnchorClass *ac;
                    char *namep = *sname=='_' ? sname + 1 : sname;
                    char *xs = (char *) xmlGetProp(points, (xmlChar *) "x");
                    char *ys = (char *) xmlGetProp(points, (xmlChar *) "y");
                    ap->me.x = strtod(xs,NULL);
                    ap->me.y = strtod(ys,NULL);

                    ac = SFFindOrAddAnchorClass(sf,namep,NULL);
                    if (*sname=='_')
                        ap->type = ac->type==act_curs ? at_centry : at_mark;
                    else
                        ap->type = ac->type==act_mkmk   ? at_basemark :
                                    ac->type==act_curs  ? at_cexit :
                                    ac->type==act_mklg  ? at_baselig :
                                                          at_basechar;
                    ap->anchor = ac;
                    ap->next = sc->anchor;
                    sc->anchor = ap;
                    free(xs); free(ys); free(sname);
        			continue; // We stop processing the contour at this point.
                }
            }

			// If we have not identified the contour as holding an anchor point, we continue processing it as a rendered shape.
			int wasquad = -1; // This tracks whether we identified the previous curve as quadratic. (-1 means undefined.)
			int firstpointsaidquad = -1; // This tracks the declared order of the curve leading into the first on-curve point.

		    ss = chunkalloc(sizeof(SplineSet));
			ss->first = NULL;

			for ( points = contour->children; points!=NULL; points=points->next ) {
			char *xs, *ys, *type, *pname, *smooths;
			double x,y;
			int smooth = 0;
			// We discard any entities in the splineset that are not points.
			if ( xmlStrcmp(points->name,(const xmlChar *) "point")!=0 )
		    continue;
			// Read as strings from xml.
			xs = (char *) xmlGetProp(points,(xmlChar *) "x");
			ys = (char *) xmlGetProp(points,(xmlChar *) "y");
			type = (char *) xmlGetProp(points,(xmlChar *) "type");
			pname = (char *) xmlGetProp(points,(xmlChar *) "name");
			smooths = (char *) xmlGetProp(points,(xmlChar *) "smooth");
			if (smooths != NULL) {
				if (strcmp(smooths,"yes") == 0) smooth = 1;
				free(smooths); smooths=NULL;
			}
			if ( xs==NULL || ys == NULL ) {
				if (xs != NULL) { free(xs); xs = NULL; }
				if (ys != NULL) { free(ys); ys = NULL; }
				if (type != NULL) { free(type); type = NULL; }
				if (pname != NULL) { free(pname); pname = NULL; }
		    	continue;
			}
			x = strtod(xs,NULL); y = strtod(ys,NULL);
			if ( type!=NULL && (strcmp(type,"move")==0 ||
					    strcmp(type,"line")==0 ||
					    strcmp(type,"curve")==0 ||
					    strcmp(type,"qcurve")==0 )) {
				// This handles only actual points.
				// We create and label the point.
			    sp = SplinePointCreate(x,y);
				sp->dontinterpolate = 1;
				if (pname != NULL) {
					sp->name = copy(pname);
				}
				if (smooth == 1) sp->pointtype = pt_curve;
				else sp->pointtype = pt_corner;
			    if ( strcmp(type,"move")==0 ) {
					open = true;
			        ss->first = ss->last = sp;
			    } else if ( ss->first==NULL ) {
					ss->first = ss->last = sp;
			        memcpy(init,pre,sizeof(pre));
			        initcnt = precnt;
			        if ( strcmp(type,"qcurve")==0 )
				    wasquad = true;
					if ( strcmp(type,"curve")==0 )
					wasquad = false;
					firstpointsaidquad = wasquad;
			    } else if ( strcmp(type,"line")==0 ) {
				SplineMake(ss->last,sp,false);
			        ss->last = sp;
			    } else if ( strcmp(type,"curve")==0 ) {
				wasquad = false;
				if ( precnt==2 ) {
				    ss->last->nextcp = pre[0];
			        ss->last->nonextcp = false;
			        sp->prevcp = pre[1];
			        sp->noprevcp = false;
				} else if ( precnt==1 ) {
				    ss->last->nextcp = sp->prevcp = pre[0];
			        ss->last->nonextcp = sp->noprevcp = false;
				}
				SplineMake(ss->last,sp,false);
			        ss->last = sp;
			    } else if ( strcmp(type,"qcurve")==0 ) {
					wasquad = true;
					if ( precnt>0 && precnt<=2 ) {
						if ( precnt==2 ) {
							// If we have two cached control points and the end point is quadratic, we need an implied point between the two control points.
							sp2 = SplinePointCreate((pre[1].x+pre[0].x)/2,(pre[1].y+pre[0].y)/2);
							sp2->prevcp = ss->last->nextcp = pre[0];
							sp2->noprevcp = ss->last->nonextcp = false;
							sp2->ttfindex = 0xffff;
							SplineMake(ss->last,sp2,true);
							ss->last = sp2;
						}
						// Now we connect the real point.
						sp->prevcp = ss->last->nextcp = pre[precnt-1];
						sp->noprevcp = ss->last->nonextcp = false;
					}
					SplineMake(ss->last,sp,true);
					ss->last = sp;
			    }
			    precnt = 0;
			} else {
				// This handles non-end-points (control points).
			    if ( wasquad==-1 && precnt==2 ) {
				// We don't know whether the current curve is quadratic or cubic, but, if we're hitting three off-curve points in a row, something is off.
				// As mentioned below, we assume in this case that we're dealing with a quadratic TrueType curve that needs implied points.
				// We create those points since they are adjustable in Fontforge.
				// There is not a valid case as far as Frank knows in which a cubic curve would have implied points.
				/* Undocumented fact: If there are no on-curve points (and therefore no indication of quadratic/cubic), assume truetype implied points */
					memcpy(init,pre,sizeof(pre));
					initcnt = 1;
					// We make the point between the two already cached control points.
					sp = SplinePointCreate((pre[1].x+pre[0].x)/2,(pre[1].y+pre[0].y)/2);
					sp->ttfindex = 0xffff;
					if (pname != NULL) {
						sp->name = copy(pname);
					}
			        sp->nextcp = pre[1];
			        sp->nonextcp = false;
			        if ( ss->first==NULL ) {
						// This is indeed possible if the first three points are control points.
				    	ss->first = sp;
						memcpy(init,pre,sizeof(pre));
			            initcnt = 1;
					} else {
				    	ss->last->nextcp = sp->prevcp = pre[0];
			            ss->last->nonextcp = sp->noprevcp = false;
			            initcnt = 0;
			            SplineMake(ss->last,sp,true);
                    }
			        ss->last = sp;
					// We make the point between the previously cached control point and the new control point.
					sp = SplinePointCreate((x+pre[1].x)/2,(y+pre[1].y)/2);
			        sp->prevcp = pre[1];
			        sp->noprevcp = false;
					sp->ttfindex = 0xffff;
			        SplineMake(ss->last,sp,true);
			        ss->last = sp;
			        pre[0].x = x; pre[0].y = y;
			        precnt = 1;
					wasquad = true;
			    } else if ( wasquad==true && precnt==1 && 0 ) {
					// Frank thinks that this might generate false positives for qcurves.
					// The only case in which this would create a qcurve missed by the previous condition block
					// and the point type reader would, it seems, be a cubic curve trailing a quadratic curve.
					// This seems not to be the best way to handle it.
					sp = SplinePointCreate((x+pre[0].x)/2,(y+pre[0].y)/2);
					if (pname != NULL) {
						sp->name = copy(pname);
					}
			        sp->prevcp = pre[0];
			        sp->noprevcp = false;
					sp->ttfindex = 0xffff;
			        if ( ss->last==NULL ) {
				    	ss->first = sp;
			            memcpy(init,pre,sizeof(pre));
			            initcnt = 1;
					} else {
					    ss->last->nextcp = sp->prevcp;
			            ss->last->nonextcp = false;
				    	SplineMake(ss->last,sp,true);
					}
					ss->last = sp;
			        pre[0].x = x; pre[0].y = y;
			    } else if ( precnt<2 ) {
					pre[precnt].x = x;
			        pre[precnt].y = y;
			        ++precnt;
			    }
			}
                        if (xs != NULL) { free(xs); xs = NULL; }
                        if (ys != NULL) { free(ys); ys = NULL; }
                        if (type != NULL) { free(type); type = NULL; }
                        if (pname != NULL) { free(pname); pname = NULL; }
		    }
			// We are finished looping, so it's time to close the curve if it is to be closed.
		    if ( !open ) {
			// init has a list of control points leading into the first point. pre has a list of control points trailing the last processed on-curve point.
			// We merge pre into init and use init as the list of control points between the last processed on-curve point and the first on-curve point.
			if ( precnt!=0 ) {
			    BasePoint temp[2];
			    memcpy(temp,init,sizeof(temp));
			    memcpy(init,pre,sizeof(pre));
			    memcpy(init+precnt,temp,sizeof(temp));
			    initcnt += precnt;
			}
			if ( (firstpointsaidquad==true && initcnt>0) || initcnt==1 ) {
				// If the final curve is declared quadratic or is assumed to be by control point count, we proceed accordingly.
			    int i;
			    for ( i=0; i<initcnt-1; ++i ) {
					// If the final curve is declared quadratic but has more than one control point, we add implied points.
					sp = SplinePointCreate((init[i+1].x+init[i].x)/2,(init[i+1].y+init[i].y)/2);
			        sp->prevcp = ss->last->nextcp = init[i];
			        sp->noprevcp = ss->last->nonextcp = false;
					sp->ttfindex = 0xffff;
			        SplineMake(ss->last,sp,true);
			        ss->last = sp;
			    }
			    ss->last->nextcp = ss->first->prevcp = init[initcnt-1];
			    ss->last->nonextcp = ss->first->noprevcp = false;
			    wasquad = true;
			} else if ( initcnt==2 ) {
			    ss->last->nextcp = init[0];
			    ss->first->prevcp = init[1];
			    ss->last->nonextcp = ss->first->noprevcp = false;
				wasquad = false;
			}
			SplineMake(ss->last,ss->first,firstpointsaidquad);
			ss->last = ss->first;
		    }
		    if ( last==NULL ) {
				// FTODO
				// Deal with existing splines somehow.
				sc->layers[layerdest].splines = ss;
		    } else
				last->next = ss;
				last = ss;
		    }
	    }
	} else if ( xmlStrcmp(kids->name,(const xmlChar *) "lib")==0 ) {
	    xmlNodePtr keys, temp, dict = FindNode(kids->children,"dict");
	    if ( dict!=NULL ) {
		for ( keys=dict->children; keys!=NULL; keys=keys->next ) {
		    if ( xmlStrcmp(keys->name,(const xmlChar *) "key")== 0 ) {
				char *keyname = (char *) xmlNodeListGetString(doc,keys->children,true);
				if ( strcmp(keyname,"com.fontlab.hintData")==0 ) {
			    	for ( temp=keys->next; temp!=NULL; temp=temp->next ) {
						if ( xmlStrcmp(temp->name,(const xmlChar *) "dict")==0 )
						    break;
			    	}
			    	if ( temp!=NULL ) {
						if (layerdest == ly_fore) {
							if (sc->hstem == NULL) {
								sc->hstem = GlifParseHints(doc,temp,"hhints");
								SCGuessHHintInstancesList(sc,ly_fore);
							}
							if (sc->vstem == NULL) {
								sc->vstem = GlifParseHints(doc,temp,"vhints");
			        			SCGuessVHintInstancesList(sc,ly_fore);
			        		}
						}
			    	}
					break;
				}
				free(keyname);
		    }
		}
#ifndef _NO_PYTHON
		sc->python_persistent = LibToPython(doc,dict);
#endif
	    }
	}
    }
    xmlFreeDoc(doc);
    SPLCategorizePoints(sc->layers[layerdest].splines);
return( sc );
}

static SplineChar *UFOLoadGlyph(SplineFont *sf,char *glifname, char* glyphname, SplineChar* existingglyph, int layerdest) {
    xmlDocPtr doc;

    doc = xmlParseFile(glifname);
    if ( doc==NULL ) {
	LogError(_("Bad glif file %s"), glifname);
return( NULL );
    }
return( _UFOLoadGlyph(sf,doc,glifname,glyphname,existingglyph,layerdest));
}


static void UFORefFixup(SplineFont *sf, SplineChar *sc ) {
    RefChar *r, *prev;
    SplineChar *rsc;

    if ( sc==NULL || sc->ticked )
		return;
    sc->ticked = true;
    prev = NULL;
	// For each reference, attempt to locate the real splinechar matching the name stored in the fake splinechar.
	// Free the fake splinechar afterwards.
    for ( r=sc->layers[ly_fore].refs; r!=NULL; r=r->next ) {
		rsc = SFGetChar(sf,-1, r->sc->name);
		if ( rsc==NULL ) {
			LogError(_("Failed to find glyph %s when fixing up references"), r->sc->name);
			if ( prev==NULL )
			sc->layers[ly_fore].refs = r->next;
			else
			prev->next = r->next;
			SplineCharFree(r->sc);
			/* Memory leak. We loose r */
		} else {
			UFORefFixup(sf,rsc);
			SplineCharFree(r->sc);
			r->sc = rsc;
			prev = r;
			SCReinstanciateRefChar(sc,r,ly_fore);
		}
    }
}

static void UFOLoadGlyphs(SplineFont *sf,char *glyphdir, int layerdest) {
    char *glyphlist = buildname(glyphdir,"contents.plist");
    xmlDocPtr doc;
    xmlNodePtr plist, dict, keys, value;
    char *valname, *glyphfname;
    int i;
    SplineChar *sc;
    int tot;

    doc = xmlParseFile(glyphlist);
    free(glyphlist);
    if ( doc==NULL ) {
	LogError(_("Bad contents.plist"));
return;
    }
    plist = xmlDocGetRootElement(doc);
    dict = FindNode(plist->children,"dict");
    if ( xmlStrcmp(plist->name,(const xmlChar *) "plist")!=0 || dict == NULL ) {
	LogError(_("Expected property list file"));
	xmlFreeDoc(doc);
return;
    }
	// Count glyphs for the benefit of measuring progress.
    for ( tot=0, keys=dict->children; keys!=NULL; keys=keys->next ) {
		if ( xmlStrcmp(keys->name,(const xmlChar *) "key")==0 )
		    ++tot;
    }
    ff_progress_change_total(tot);
	// Start reading in glyph name to file name mappings.
    for ( keys=dict->children; keys!=NULL; keys=keys->next ) {
		for ( value = keys->next; value!=NULL && xmlStrcmp(value->name,(const xmlChar *) "text")==0;
			value = value->next );
		if ( value==NULL )
			break;
		if ( xmlStrcmp(keys->name,(const xmlChar *) "key")==0 ) {
			char * glyphname = (char *) xmlNodeListGetString(doc,keys->children,true);
			int newsc = 0;
			SplineChar* existingglyph = NULL;
			if (glyphname != NULL) {
				existingglyph = SFGetChar(sf,-1,glyphname);
				if (existingglyph == NULL) newsc = 1;
				valname = (char *) xmlNodeListGetString(doc,value->children,true);
				glyphfname = buildname(glyphdir,valname);
				free(valname);
				sc = UFOLoadGlyph(sf, glyphfname, glyphname, existingglyph, layerdest);
				if ( ( sc!=NULL ) && newsc ) {
					sc->parent = sf;
					if ( sf->glyphcnt>=sf->glyphmax )
						sf->glyphs = realloc(sf->glyphs,(sf->glyphmax+=100)*sizeof(SplineChar *));
					sc->orig_pos = sf->glyphcnt;
					sf->glyphs[sf->glyphcnt++] = sc;
				}
			}
			keys = value;
			ff_progress_next();
		}
    }
    xmlFreeDoc(doc);

    GlyphHashFree(sf);
    for ( i=0; i<sf->glyphcnt; ++i )
	UFORefFixup(sf,sf->glyphs[i]);
}

static void UFOHandleKern(SplineFont *sf,char *basedir,int isv) {
    char *fname = buildname(basedir,isv ? "vkerning.plist" : "kerning.plist");
    xmlDocPtr doc=NULL;
    xmlNodePtr plist,dict,keys,value,subkeys;
    char *keyname, *valname;
    int offset;
    SplineChar *sc, *ssc;
    KernPair *kp;
    char *end;
    uint32 script;

    if ( GFileExists(fname))
	doc = xmlParseFile(fname);
    free(fname);
    if ( doc==NULL )
return;

    plist = xmlDocGetRootElement(doc);
    dict = FindNode(plist->children,"dict");
    if ( xmlStrcmp(plist->name,(const xmlChar *) "plist")!=0 || dict==NULL ) {
	LogError(_("Expected property list file"));
	xmlFreeDoc(doc);
return;
    }
    for ( keys=dict->children; keys!=NULL; keys=keys->next ) {
	for ( value = keys->next; value!=NULL && xmlStrcmp(value->name,(const xmlChar *) "text")==0;
		value = value->next );
	if ( value==NULL )
    break;
	if ( xmlStrcmp(keys->name,(const xmlChar *) "key")==0 ) {
	    keyname = (char *) xmlNodeListGetString(doc,keys->children,true);
	    sc = SFGetChar(sf,-1,keyname);
	    free(keyname);
	    if ( sc==NULL )
	continue;
	    keys = value;
	    for ( subkeys = value->children; subkeys!=NULL; subkeys = subkeys->next ) {
		for ( value = subkeys->next; value!=NULL && xmlStrcmp(value->name,(const xmlChar *) "text")==0;
			value = value->next );
		if ( value==NULL )
	    break;
		if ( xmlStrcmp(subkeys->name,(const xmlChar *) "key")==0 ) {
		    keyname = (char *) xmlNodeListGetString(doc,subkeys->children,true);
		    ssc = SFGetChar(sf,-1,keyname);
		    free(keyname);
		    if ( ssc==NULL )
		continue;
		    for ( kp=isv?sc->vkerns:sc->kerns; kp!=NULL && kp->sc!=ssc; kp=kp->next );
		    if ( kp!=NULL )
		continue;
		    subkeys = value;
		    valname = (char *) xmlNodeListGetString(doc,value->children,true);
		    offset = strtol(valname,&end,10);
		    if ( *end=='\0' ) {
			kp = chunkalloc(sizeof(KernPair));
			kp->off = offset;
			kp->sc = ssc;
			if ( isv ) {
			    kp->next = sc->vkerns;
			    sc->vkerns = kp;
			} else {
			    kp->next = sc->kerns;
			    sc->kerns = kp;
			}
			script = SCScriptFromUnicode(sc);
			if ( script==DEFAULT_SCRIPT )
			    script = SCScriptFromUnicode(ssc);
			kp->subtable = SFSubTableFindOrMake(sf,
				isv?CHR('v','k','r','n'):CHR('k','e','r','n'),
				script, gpos_pair);
		    }
		    free(valname);
		}
	    }
	}
    }
    xmlFreeDoc(doc);
}

static void UFOAddName(SplineFont *sf,char *value,int strid) {
    /* We simply assume that the entries in the name table are in English */
    /* The format doesn't say -- which bothers me */
    struct ttflangname *names;

    for ( names=sf->names; names!=NULL && names->lang!=0x409; names=names->next );
    if ( names==NULL ) {
	names = chunkalloc(sizeof(struct ttflangname));
	names->next = sf->names;
	names->lang = 0x409;
	sf->names = names;
    }
    names->names[strid] = value;
}

static void UFOAddPrivate(SplineFont *sf,char *key,char *value) {
    char *pt;

    if ( sf->private==NULL )
	sf->private = chunkalloc(sizeof(struct psdict));
    for ( pt=value; *pt!='\0'; ++pt ) {	/* Value might contain white space. turn into spaces */
	if ( *pt=='\n' || *pt=='\r' || *pt=='\t' )
	    *pt = ' ';
    }
    PSDictChangeEntry(sf->private, key, value);
}

static void UFOAddPrivateArray(SplineFont *sf,char *key,xmlDocPtr doc,xmlNodePtr value) {
    char space[400], *pt, *end;
    xmlNodePtr kid;

    if ( xmlStrcmp(value->name,(const xmlChar *) "array")!=0 )
return;
    pt = space; end = pt+sizeof(space)-10;
    *pt++ = '[';
    for ( kid = value->children; kid!=NULL; kid=kid->next ) {
	if ( xmlStrcmp(kid->name,(const xmlChar *) "integer")==0 ||
		xmlStrcmp(kid->name,(const xmlChar *) "real")==0 ) {
	    char *valName = (char *) xmlNodeListGetString(doc,kid->children,true);
	    if ( pt+1+strlen(valName)<end ) {
		if ( pt!=space+1 )
		    *pt++=' ';
		strcpy(pt,valName);
		pt += strlen(pt);
	    }
	    free(valName);
	}
    }
    if ( pt!=space+1 ) {
	*pt++ = ']';
	*pt++ = '\0';
	UFOAddPrivate(sf,key,space);
    }
}

static void UFOGetByteArray(char *array,int cnt,xmlDocPtr doc,xmlNodePtr value) {
    xmlNodePtr kid;
    int i;

    memset(array,0,cnt);

    if ( xmlStrcmp(value->name,(const xmlChar *) "array")!=0 )
return;
    i=0;
    for ( kid = value->children; kid!=NULL; kid=kid->next ) {
	if ( xmlStrcmp(kid->name,(const xmlChar *) "integer")==0 ) {
	    char *valName = (char *) xmlNodeListGetString(doc,kid->children,true);
	    if ( i<cnt )
		array[i++] = strtol(valName,NULL,10);
	    free(valName);
	}
    }
}

static long UFOGetBits(xmlDocPtr doc,xmlNodePtr value) {
    xmlNodePtr kid;
    long mask=0;

    if ( xmlStrcmp(value->name,(const xmlChar *) "array")!=0 )
return( 0 );
    for ( kid = value->children; kid!=NULL; kid=kid->next ) {
	if ( xmlStrcmp(kid->name,(const xmlChar *) "integer")==0 ) {
	    char *valName = (char *) xmlNodeListGetString(doc,kid->children,true);
	    mask |= 1<<strtol(valName,NULL,10);
	    free(valName);
	}
    }
return( mask );
}

static void UFOGetBitArray(xmlDocPtr doc,xmlNodePtr value,uint32 *res,int len) {
    xmlNodePtr kid;
    int index;

    if ( xmlStrcmp(value->name,(const xmlChar *) "array")!=0 )
return;
    for ( kid = value->children; kid!=NULL; kid=kid->next ) {
	if ( xmlStrcmp(kid->name,(const xmlChar *) "integer")==0 ) {
	    char *valName = (char *) xmlNodeListGetString(doc,kid->children,true);
	    index = strtol(valName,NULL,10);
	    if ( index < len<<5 )
		res[index>>5] |= 1<<(index&31);
	    free(valName);
	}
    }
}

SplineFont *SFReadUFO(char *basedir, int flags) {
    xmlNodePtr plist, dict, keys, value;
    xmlDocPtr doc;
    SplineFont *sf;
    xmlChar *keyname, *valname;
    char *stylename=NULL;
    char *temp, *glyphlist, *glyphdir;
    char oldloc[25], *end;
    int as = -1, ds= -1, em= -1;

    if ( !libxml_init_base()) {
	LogError(_("Can't find libxml2."));
return( NULL );
    }

    temp = buildname(basedir,"fontinfo.plist");
    doc = xmlParseFile(temp);
    free(temp);
    if ( doc==NULL ) {
	/* Can I get an error message from libxml? */
return( NULL );
    }
    plist = xmlDocGetRootElement(doc);
    dict = FindNode(plist->children,"dict");
    if ( xmlStrcmp(plist->name,(const xmlChar *) "plist")!=0 || dict==NULL ) {
	LogError(_("Expected property list file"));
	xmlFreeDoc(doc);
return( NULL );
    }

    sf = SplineFontEmpty();
    strncpy( oldloc,setlocale(LC_NUMERIC,NULL),24 );
    oldloc[24]=0;
    setlocale(LC_NUMERIC,"C");
    for ( keys=dict->children; keys!=NULL; keys=keys->next ) {
	for ( value = keys->next; value!=NULL && xmlStrcmp(value->name,(const xmlChar *) "text")==0;
		value = value->next );
	if ( value==NULL )
    break;
	if ( xmlStrcmp(keys->name,(const xmlChar *) "key")==0 ) {
	    keyname = xmlNodeListGetString(doc,keys->children,true);
	    valname = xmlNodeListGetString(doc,value->children,true);
	    keys = value;
	    if ( xmlStrcmp(keyname,(xmlChar *) "familyName")==0 )
		sf->familyname = (char *) valname;
	    else if ( xmlStrcmp(keyname,(xmlChar *) "styleName")==0 )
		stylename = (char *) valname;
	    else if ( xmlStrcmp(keyname,(xmlChar *) "fullName")==0 ||
		    xmlStrcmp(keyname,(xmlChar *) "postscriptFullName")==0 )
		sf->fullname = (char *) valname;
	    else if ( xmlStrcmp(keyname,(xmlChar *) "fontName")==0 ||
		    xmlStrcmp(keyname,(xmlChar *) "postscriptFontName")==0 )
		sf->fontname = (char *) valname;
	    else if ( xmlStrcmp(keyname,(xmlChar *) "weightName")==0 ||
		    xmlStrcmp(keyname,(xmlChar *) "postscriptWeightName")==0 )
		sf->weight = (char *) valname;
	    else if ( xmlStrcmp(keyname,(xmlChar *) "note")==0 )
		sf->comments = (char *) valname;
	    else if ( xmlStrcmp(keyname,(xmlChar *) "copyright")==0 )
		sf->copyright = (char *) valname;
	    else if ( xmlStrcmp(keyname,(xmlChar *) "trademark")==0 )
		UFOAddName(sf,(char *) valname,ttf_trademark);
	    else if ( strncmp((char *) keyname,"openTypeName",12)==0 ) {
		if ( xmlStrcmp(keyname+12,(xmlChar *) "Designer")==0 )
		    UFOAddName(sf,(char *) valname,ttf_designer);
		else if ( xmlStrcmp(keyname+12,(xmlChar *) "DesignerURL")==0 )
		    UFOAddName(sf,(char *) valname,ttf_designerurl);
		else if ( xmlStrcmp(keyname+12,(xmlChar *) "Manufacturer")==0 )
		    UFOAddName(sf,(char *) valname,ttf_manufacturer);
		else if ( xmlStrcmp(keyname+12,(xmlChar *) "ManufacturerURL")==0 )
		    UFOAddName(sf,(char *) valname,ttf_venderurl);
		else if ( xmlStrcmp(keyname+12,(xmlChar *) "License")==0 )
		    UFOAddName(sf,(char *) valname,ttf_license);
		else if ( xmlStrcmp(keyname+12,(xmlChar *) "LicenseURL")==0 )
		    UFOAddName(sf,(char *) valname,ttf_licenseurl);
		else if ( xmlStrcmp(keyname+12,(xmlChar *) "Version")==0 )
		    UFOAddName(sf,(char *) valname,ttf_version);
		else if ( xmlStrcmp(keyname+12,(xmlChar *) "UniqueID")==0 )
		    UFOAddName(sf,(char *) valname,ttf_uniqueid);
		else if ( xmlStrcmp(keyname+12,(xmlChar *) "Description")==0 )
		    UFOAddName(sf,(char *) valname,ttf_descriptor);
		else if ( xmlStrcmp(keyname+12,(xmlChar *) "PreferedFamilyName")==0 )
		    UFOAddName(sf,(char *) valname,ttf_preffamilyname);
		else if ( xmlStrcmp(keyname+12,(xmlChar *) "PreferedSubfamilyName")==0 )
		    UFOAddName(sf,(char *) valname,ttf_prefmodifiers);
		else if ( xmlStrcmp(keyname+12,(xmlChar *) "CompatibleFullName")==0 )
		    UFOAddName(sf,(char *) valname,ttf_compatfull);
		else if ( xmlStrcmp(keyname+12,(xmlChar *) "SampleText")==0 )
		    UFOAddName(sf,(char *) valname,ttf_sampletext);
		else if ( xmlStrcmp(keyname+12,(xmlChar *) "WWSFamilyName")==0 )
		    UFOAddName(sf,(char *) valname,ttf_wwsfamily);
		else if ( xmlStrcmp(keyname+12,(xmlChar *) "WWSSubfamilyName")==0 )
		    UFOAddName(sf,(char *) valname,ttf_wwssubfamily);
		else
		    free(valname);
	    } else if ( strncmp((char *) keyname, "openTypeHhea",12)==0 ) {
		if ( xmlStrcmp(keyname+12,(xmlChar *) "Ascender")==0 ) {
		    sf->pfminfo.hhead_ascent = strtol((char *) valname,&end,10);
		    sf->pfminfo.hheadascent_add = false;
		} else if ( xmlStrcmp(keyname+12,(xmlChar *) "Descender")==0 ) {
		    sf->pfminfo.hhead_descent = strtol((char *) valname,&end,10);
		    sf->pfminfo.hheaddescent_add = false;
		} else if ( xmlStrcmp(keyname+12,(xmlChar *) "LineGap")==0 )
		    sf->pfminfo.linegap = strtol((char *) valname,&end,10);
		free(valname);
		sf->pfminfo.hheadset = true;
	    } else if ( strncmp((char *) keyname,"openTypeVhea",12)==0 ) {
		if ( xmlStrcmp(keyname+12,(xmlChar *) "LineGap")==0 )
		    sf->pfminfo.vlinegap = strtol((char *) valname,&end,10);
		sf->pfminfo.vheadset = true;
		free(valname);
	    } else if ( strncmp((char *) keyname,"openTypeOS2",11)==0 ) {
		sf->pfminfo.pfmset = true;
		if ( xmlStrcmp(keyname+11,(xmlChar *) "Panose")==0 ) {
		    UFOGetByteArray(sf->pfminfo.panose,sizeof(sf->pfminfo.panose),doc,value);
		    sf->pfminfo.panose_set = true;
		} else if ( xmlStrcmp(keyname+11,(xmlChar *) "Type")==0 ) {
		    sf->pfminfo.fstype = UFOGetBits(doc,value);
		    if ( sf->pfminfo.fstype<0 ) {
			/* all bits are set, but this is wrong, OpenType spec says */
			/* bits 0, 4-7 and 10-15 must be unset, go see		   */
			/* http://www.microsoft.com/typography/otspec/os2.htm#fst  */
			LogError(_("Bad openTypeOS2type key: all bits are set. It will be ignored"));
			sf->pfminfo.fstype = 0;
		    }
		} else if ( xmlStrcmp(keyname+11,(xmlChar *) "FamilyClass")==0 ) {
		    char fc[2];
		    UFOGetByteArray(fc,sizeof(fc),doc,value);
		    sf->pfminfo.os2_family_class = (fc[0]<<8)|fc[1];
		} else if ( xmlStrcmp(keyname+11,(xmlChar *) "WidthClass")==0 )
		    sf->pfminfo.width = strtol((char *) valname,&end,10);
		else if ( xmlStrcmp(keyname+11,(xmlChar *) "WeightClass")==0 )
		    sf->pfminfo.weight = strtol((char *) valname,&end,10);
		else if ( xmlStrcmp(keyname+11,(xmlChar *) "VendorID")==0 )
		{
		    const int os2_vendor_sz = sizeof(sf->pfminfo.os2_vendor);
		    const int valname_len = c_strlen(valname);

		    if( valname && valname_len <= os2_vendor_sz )
			strncpy(sf->pfminfo.os2_vendor,valname,valname_len);

		    char *temp = sf->pfminfo.os2_vendor + os2_vendor_sz - 1;
		    while ( *temp == 0 && temp >= sf->pfminfo.os2_vendor )
			*temp-- = ' ';
		}
		else if ( xmlStrcmp(keyname+11,(xmlChar *) "TypoAscender")==0 ) {
		    sf->pfminfo.typoascent_add = false;
		    sf->pfminfo.os2_typoascent = strtol((char *) valname,&end,10);
		} else if ( xmlStrcmp(keyname+11,(xmlChar *) "TypoDescender")==0 ) {
		    sf->pfminfo.typodescent_add = false;
		    sf->pfminfo.os2_typodescent = strtol((char *) valname,&end,10);
		} else if ( xmlStrcmp(keyname+11,(xmlChar *) "TypoLineGap")==0 )
		    sf->pfminfo.os2_typolinegap = strtol((char *) valname,&end,10);
		else if ( xmlStrcmp(keyname+11,(xmlChar *) "WinAscent")==0 ) {
		    sf->pfminfo.winascent_add = false;
		    sf->pfminfo.os2_winascent = strtol((char *) valname,&end,10);
		} else if ( xmlStrcmp(keyname+11,(xmlChar *) "WinDescent")==0 ) {
		    sf->pfminfo.windescent_add = false;
		    sf->pfminfo.os2_windescent = strtol((char *) valname,&end,10);
		} else if ( strncmp((char *) keyname+11,"Subscript",9)==0 ) {
		    sf->pfminfo.subsuper_set = true;
		    if ( xmlStrcmp(keyname+20,(xmlChar *) "XSize")==0 )
			sf->pfminfo.os2_subxsize = strtol((char *) valname,&end,10);
		    else if ( xmlStrcmp(keyname+20,(xmlChar *) "YSize")==0 )
			sf->pfminfo.os2_subysize = strtol((char *) valname,&end,10);
		    else if ( xmlStrcmp(keyname+20,(xmlChar *) "XOffset")==0 )
			sf->pfminfo.os2_subxoff = strtol((char *) valname,&end,10);
		    else if ( xmlStrcmp(keyname+20,(xmlChar *) "YOffset")==0 )
			sf->pfminfo.os2_subyoff = strtol((char *) valname,&end,10);
		} else if ( strncmp((char *) keyname+11, "Superscript",11)==0 ) {
		    sf->pfminfo.subsuper_set = true;
		    if ( xmlStrcmp(keyname+22,(xmlChar *) "XSize")==0 )
			sf->pfminfo.os2_supxsize = strtol((char *) valname,&end,10);
		    else if ( xmlStrcmp(keyname+22,(xmlChar *) "YSize")==0 )
			sf->pfminfo.os2_supysize = strtol((char *) valname,&end,10);
		    else if ( xmlStrcmp(keyname+22,(xmlChar *) "XOffset")==0 )
			sf->pfminfo.os2_supxoff = strtol((char *) valname,&end,10);
		    else if ( xmlStrcmp(keyname+22,(xmlChar *) "YOffset")==0 )
			sf->pfminfo.os2_supyoff = strtol((char *) valname,&end,10);
		} else if ( strncmp((char *) keyname+11, "Strikeout",9)==0 ) {
		    sf->pfminfo.subsuper_set = true;
		    if ( xmlStrcmp(keyname+20,(xmlChar *) "Size")==0 )
			sf->pfminfo.os2_strikeysize = strtol((char *) valname,&end,10);
		    else if ( xmlStrcmp(keyname+20,(xmlChar *) "Position")==0 )
			sf->pfminfo.os2_strikeypos = strtol((char *) valname,&end,10);
		} else if ( strncmp((char *) keyname+11, "CodePageRanges",14)==0 ) {
		    UFOGetBitArray(doc,value,sf->pfminfo.codepages,2);
		    sf->pfminfo.hascodepages = true;
		} else if ( strncmp((char *) keyname+11, "UnicodeRanges",13)==0 ) {
		    UFOGetBitArray(doc,value,sf->pfminfo.unicoderanges,4);
		    sf->pfminfo.hasunicoderanges = true;
		}
		free(valname);
	    } else if ( strncmp((char *) keyname, "postscript",10)==0 ) {
		if ( xmlStrcmp(keyname+10,(xmlChar *) "UnderlineThickness")==0 )
		    sf->uwidth = strtol((char *) valname,&end,10);
		else if ( xmlStrcmp(keyname+10,(xmlChar *) "UnderlinePosition")==0 )
		    sf->upos = strtol((char *) valname,&end,10);
		else if ( xmlStrcmp(keyname+10,(xmlChar *) "BlueFuzz")==0 )
		    UFOAddPrivate(sf,"BlueFuzz",(char *) valname);
		else if ( xmlStrcmp(keyname+10,(xmlChar *) "BlueScale")==0 )
		    UFOAddPrivate(sf,"BlueScale",(char *) valname);
		else if ( xmlStrcmp(keyname+10,(xmlChar *) "BlueShift")==0 )
		    UFOAddPrivate(sf,"BlueShift",(char *) valname);
		else if ( xmlStrcmp(keyname+10,(xmlChar *) "BlueValues")==0 )
		    UFOAddPrivateArray(sf,"BlueValues",doc,value);
		else if ( xmlStrcmp(keyname+10,(xmlChar *) "OtherBlues")==0 )
		    UFOAddPrivateArray(sf,"OtherBlues",doc,value);
		else if ( xmlStrcmp(keyname+10,(xmlChar *) "FamilyBlues")==0 )
		    UFOAddPrivateArray(sf,"FamilyBlues",doc,value);
		else if ( xmlStrcmp(keyname+10,(xmlChar *) "FamilyOtherBlues")==0 )
		    UFOAddPrivateArray(sf,"FamilyOtherBlues",doc,value);
		else if ( xmlStrcmp(keyname+10,(xmlChar *) "StemSnapH")==0 )
		    UFOAddPrivateArray(sf,"StemSnapH",doc,value);
		else if ( xmlStrcmp(keyname+10,(xmlChar *) "StemSnapV")==0 )
		    UFOAddPrivateArray(sf,"StemSnapV",doc,value);
		else if ( xmlStrcmp(keyname+10,(xmlChar *) "ForceBold")==0 )
		    UFOAddPrivate(sf,"ForceBold",(char *) value->name);
			/* value->name is either true or false */
		free(valname);
	    } else if ( strncmp((char *)keyname,"macintosh",9)==0 ) {
		if ( xmlStrcmp(keyname+9,(xmlChar *) "FONDName")==0 )
		    sf->fondname = (char *) valname;
		else
		    free(valname);
	    } else if ( xmlStrcmp(keyname,(xmlChar *) "unitsPerEm")==0 ) {
		em = strtol((char *) valname,&end,10);
		if ( *end!='\0' ) em = -1;
		free(valname);
	    } else if ( xmlStrcmp(keyname,(xmlChar *) "ascender")==0 ) {
		as = strtod((char *) valname,&end);
		if ( *end!='\0' ) as = -1;
		else sf->ufo_ascent = as;
		free(valname);
	    } else if ( xmlStrcmp(keyname,(xmlChar *) "descender")==0 ) {
		ds = -strtod((char *) valname,&end);
		if ( *end!='\0' ) ds = -1;
		else sf->ufo_descent = -ds;
		free(valname);
	    } else if ( xmlStrcmp(keyname,(xmlChar *) "italicAngle")==0 ||
		    xmlStrcmp(keyname,(xmlChar *) "postscriptSlantAngle")==0 ) {
		sf->italicangle = strtod((char *) valname,&end);
		if ( *end!='\0' ) sf->italicangle = 0;
		free(valname);
	    } else if ( xmlStrcmp(keyname,(xmlChar *) "descender")==0 ) {
		ds = -strtol((char *) valname,&end,10);
		if ( *end!='\0' ) ds = -1;
		free(valname);
	    } else
		free(valname);
	    free(keyname);
	}
    }
    if ( em==-1 && as!=-1 && ds!=-1 )
	em = as + ds;
    if ( em==as+ds )
	/* Yay! They follow my conventions */;
    else if ( em!=-1 ) {
	as = 800*em/1000;
	ds = em-as;
    }
    if ( em==-1 ) {
	LogError(_("This font does not specify unitsPerEm"));
	xmlFreeDoc(doc);
	setlocale(LC_NUMERIC,oldloc);
	SplineFontFree(sf);
return( NULL );
    }
    sf->ascent = as; sf->descent = ds;
    if ( sf->fontname==NULL ) {
	if ( stylename!=NULL && sf->familyname!=NULL )
	    sf->fontname = strconcat3(sf->familyname,"-",stylename);
	else
	    sf->fontname = copy("Untitled");
    }
    if ( sf->fullname==NULL ) {
	if ( stylename!=NULL && sf->familyname!=NULL )
	    sf->fullname = strconcat3(sf->familyname," ",stylename);
	else
	    sf->fullname = copy(sf->fontname);
    }
    if ( sf->familyname==NULL )
	sf->familyname = copy(sf->fontname);
    free(stylename);
    if ( sf->weight==NULL )
	sf->weight = copy("Regular");
    if ( sf->version==NULL && sf->names!=NULL &&
	    sf->names->names[ttf_version]!=NULL &&
	    strncmp(sf->names->names[ttf_version],"Version ",8)==0 )
	sf->version = copy(sf->names->names[ttf_version]+8);
    xmlFreeDoc(doc);

	char * layercontentsname = buildname(basedir,"layercontents.plist");
	char ** layernames = NULL;
	if (layercontentsname == NULL) {
		return( NULL );
	} else if ( GFileExists(layercontentsname)) {
		xmlDocPtr layercontentsdoc = NULL;
		xmlNodePtr layercontentsplist = NULL;
		xmlNodePtr layercontentsdict = NULL;
		xmlNodePtr layercontentslayer = NULL;
		xmlNodePtr layercontentsvalue = NULL;
		int layercontentslayercount = 0;
		int layernamesbuffersize = 0;
		int layercontentsvaluecount = 0;
		if ( (layercontentsdoc = xmlParseFile(layercontentsname)) ) {
			// The layercontents plist contains an array of double-element arrays. There is no top-level dict. Note that the indices in the layercontents array may not match those in the Fontforge layers array due to reserved spaces.
			if ( ( layercontentsplist = xmlDocGetRootElement(layercontentsdoc) ) && ( layercontentsdict = FindNode(layercontentsplist->children,"array") ) ) {
				layercontentslayercount = 0;
				layernamesbuffersize = 2;
				layernames = malloc(2*sizeof(char*)*layernamesbuffersize);
				// Look through the children of the top-level array. Stop if one of them is not an array. (Ignore text objects since these probably just have whitespace.)
				for ( layercontentslayer = layercontentsdict->children ;
				( layercontentslayer != NULL ) && ( ( xmlStrcmp(layercontentslayer->name,(const xmlChar *) "array")==0 ) || ( xmlStrcmp(layercontentslayer->name,(const xmlChar *) "text")==0 ) ) ;
				layercontentslayer = layercontentslayer->next ) {
					if ( xmlStrcmp(layercontentslayer->name,(const xmlChar *) "array")==0 ) {
						xmlChar * layerlabel = NULL;
						xmlChar * layerglyphdirname = NULL;
						layercontentsvaluecount = 0;
						// Look through the children (effectively columns) of the layer array (the row). Reject non-string values.
						for ( layercontentsvalue = layercontentslayer->children ;
						( layercontentsvalue != NULL ) && ( ( xmlStrcmp(layercontentsvalue->name,(const xmlChar *) "string")==0 ) || ( xmlStrcmp(layercontentsvalue->name,(const xmlChar *) "text")==0 ) ) ;
						layercontentsvalue = layercontentsvalue->next ) {
							if ( xmlStrcmp(layercontentsvalue->name,(const xmlChar *) "string")==0 ) {
								if (layercontentsvaluecount == 0) layerlabel = xmlNodeListGetString(layercontentsdoc, layercontentsvalue->xmlChildrenNode, true);
								if (layercontentsvaluecount == 1) layerglyphdirname = xmlNodeListGetString(layercontentsdoc, layercontentsvalue->xmlChildrenNode, true);
								layercontentsvaluecount++;
								}
						}
						// We need two values (as noted above) per layer entry and ignore any layer lacking those.
						if ((layercontentsvaluecount > 1) && (layernamesbuffersize < INT_MAX/2)) {
							// Resize the layer names array as necessary.
							if (layercontentslayercount >= layernamesbuffersize) {
								layernamesbuffersize *= 2;
								layernames = realloc(layernames, 2*sizeof(char*)*layernamesbuffersize);
							}
							// Fail silently on allocation failure; it's highly unlikely.
							if (layernames != NULL) {
								layernames[2*layercontentslayercount] = copy((char*)(layerlabel));
								if (layernames[2*layercontentslayercount]) {
									layernames[(2*layercontentslayercount)+1] = copy((char*)(layerglyphdirname));
									if (layernames[(2*layercontentslayercount)+1])
										layercontentslayercount++; // We increment only if both pointers are valid so as to avoid read problems later.
									else
										free(layernames[2*layercontentslayercount]);
								}
							}
						}
						if (layerlabel != NULL) { xmlFree(layerlabel); layerlabel = NULL; }
						if (layerglyphdirname != NULL) { xmlFree(layerglyphdirname); layerglyphdirname = NULL; }
					}
				}
				if (layernames != NULL) {
					int lcount = 0;
					int auxpos = 2;
					int layerdest = 0;
					int bg = 1;
					if (layercontentslayercount > 0) {
						// Start reading layers.
						for (lcount = 0; lcount < layercontentslayercount; lcount++) {
							// We refuse to load a layer with an incorrect prefix.
                                                	if (
							(((strcmp(layernames[2*lcount],"public.default")==0) &&
							(strcmp(layernames[2*lcount+1],"glyphs") == 0)) ||
							(strstr(layernames[2*lcount+1],"glyphs.") == layernames[2*lcount+1])) &&
							(glyphdir = buildname(basedir,layernames[2*lcount+1]))) {
                                                        	if ((glyphlist = buildname(glyphdir,"contents.plist"))) {
									if ( !GFileExists(glyphlist)) {
										LogError(_("No glyphs directory or no contents file"));
									} else {
										// Only public.default gets mapped as a foreground layer.
										bg = 1;
										// public.default and public.background have fixed mappings. Other layers start at 2.
										if (strcmp(layernames[2*lcount],"public.default")==0) {
											layerdest = ly_fore;
											bg = 0;
										} else if (strcmp(layernames[2*lcount],"public.background")==0) {
											layerdest = ly_back;
										} else {
											layerdest = auxpos++;
										}

										// We ensure that the splinefont layer list has sufficient space.
										if ( layerdest+1>sf->layer_cnt ) {
 										    sf->layers = realloc(sf->layers,(layerdest+1)*sizeof(LayerInfo));
										    memset(sf->layers+sf->layer_cnt,0,((layerdest+1)-sf->layer_cnt)*sizeof(LayerInfo));
										}
										sf->layer_cnt = layerdest+1;

										// The check is redundant, but it allows us to copy from sfd.c.
										if (( layerdest<sf->layer_cnt ) && sf->layers) {
											if (sf->layers[layerdest].name)
												free(sf->layers[layerdest].name);
											sf->layers[layerdest].name = strdup(layernames[2*lcount]);
											if (sf->layers[layerdest].ufo_path)
												free(sf->layers[layerdest].ufo_path);
											sf->layers[layerdest].ufo_path = strdup(layernames[2*lcount+1]);
											sf->layers[layerdest].background = bg;
											// Fetch glyphs.
											UFOLoadGlyphs(sf,glyphdir,layerdest);
											// Determine layer spline order.
											sf->layers[layerdest].order2 = SFLFindOrder(sf,layerdest);
											// Conform layer spline order (reworking control points if necessary).
											SFLSetOrder(sf,layerdest,sf->layers[layerdest].order2);
											// Set the grid order to the foreground order if appropriate.
											if (layerdest == ly_fore) sf->grid.order2 = sf->layers[layerdest].order2;
										}
									}
									free(glyphlist);
								}
								free(glyphdir);
							}
						}
					} else {
						LogError(_("layercontents.plist lists no valid layers."));
					}
					// Free layer names.
					for (lcount = 0; lcount < layercontentslayercount; lcount++) {
						if (layernames[2*lcount]) free(layernames[2*lcount]);
						if (layernames[2*lcount+1]) free(layernames[2*lcount+1]);
					}
					free(layernames);
				}
			}
			xmlFreeDoc(layercontentsdoc);
		}
	} else {
		glyphdir = buildname(basedir,"glyphs");
    	glyphlist = buildname(glyphdir,"contents.plist");
    	if ( !GFileExists(glyphlist)) {
			LogError(_("No glyphs directory or no contents file"));
    	} else {
			UFOLoadGlyphs(sf,glyphdir,ly_fore);
			sf->layers[ly_fore].order2 = sf->layers[ly_back].order2 = sf->grid.order2 =
		    SFFindOrder(sf);
   	    	SFSetOrder(sf,sf->layers[ly_fore].order2);
		}
	    free(glyphlist);
		free(glyphdir);
	}
	free(layercontentsname);

    sf->map = EncMapFromEncoding(sf,FindOrMakeEncoding("Unicode"));

    /* Might as well check for feature files even if version 1 */
    temp = buildname(basedir,"features.fea");
    if ( GFileExists(temp))
	SFApplyFeatureFilename(sf,temp);
    free(temp);

    UFOHandleKern(sf,basedir,0);
    UFOHandleKern(sf,basedir,1);

#ifndef _NO_PYTHON
    temp = buildname(basedir,"lib.plist");
    doc = NULL;
    if ( GFileExists(temp))
	doc = xmlParseFile(temp);
    free(temp);
    if ( doc!=NULL ) {
		plist = xmlDocGetRootElement(doc);
		dict = NULL;
		if ( plist!=NULL )
			dict = FindNode(plist->children,"dict");
		if ( plist==NULL ||
			xmlStrcmp(plist->name,(const xmlChar *) "plist")!=0 ||
			dict==NULL ) {
			LogError(_("Expected property list file"));
		} else {
			sf->python_persistent = LibToPython(doc,dict);
		}
		xmlFreeDoc(doc);
    }
#endif
    setlocale(LC_NUMERIC,oldloc);
return( sf );
}

SplineSet *SplinePointListInterpretGlif(SplineFont *sf,char *filename,char *memory, int memlen,
	int em_size,int ascent,int is_stroked) {
    xmlDocPtr doc;
    char oldloc[25];
    SplineChar *sc;
    SplineSet *ss;

    if ( !libxml_init_base()) {
	LogError(_("Can't find libxml2."));
return( NULL );
    }
    if ( filename!=NULL )
	doc = xmlParseFile(filename);
    else
	doc = xmlParseMemory(memory,memlen);
    if ( doc==NULL )
return( NULL );

    strncpy( oldloc,setlocale(LC_NUMERIC,NULL),24 );
    oldloc[24]=0;
    setlocale(LC_NUMERIC,"C");
    sc = _UFOLoadGlyph(sf,doc,filename,NULL,NULL,ly_fore);
    setlocale(LC_NUMERIC,oldloc);

    if ( sc==NULL )
return( NULL );

    ss = sc->layers[ly_fore].splines;
    sc->layers[ly_fore].splines = NULL;
    SplineCharFree(sc);
return( ss );
}

int HasUFO(void) {
return( libxml_init_base());
}
