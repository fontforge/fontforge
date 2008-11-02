/* Copyright (C) 2003-2008 by George Williams */
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

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

/* The UFO (Unified Font Object) format, http://just.letterror.com/ltrwiki/UnifiedFontObject */
/* is a directory containing a bunch of (mac style) property lists and another*/
/* directory containing glif files (and contents.plist). */

/* Property lists contain one <dict> element which contains <key> elements */
/*  each followed by an <integer>, <real>, <true/>, <false/> or <string> element, */
/*  or another <dict> */

static char *buildname(char *basedir,char *sub) {
    char *fname = galloc(strlen(basedir)+strlen(sub)+2);

    strcpy(fname, basedir);
    if ( fname[strlen(fname)-1]!='/' )
	strcat(fname,"/");
    strcat(fname,sub);
return( fname );
}

/* ************************************************************************** */
/* *************************   Python lib Output    ************************* */
/* ************************************************************************** */
#ifndef _NO_PYTHON
static int PyObjDumpable(PyObject *value);
static void DumpPyObject( FILE *file, PyObject *value );
#endif

static void DumpPythonLib(FILE *file,void *python_persistent,SplineChar *sc) {
    StemInfo *h;
    int has_hints = (sc!=NULL && (sc->hstem!=NULL || sc->vstem!=NULL ));
	
#ifdef _NO_PYTHON
    if ( has_hints ) {
	/* Not officially part of the UFO/glif spec, but used by robofab */
	fprintf( file, "  <lib>\n" );
	fprintf( file, "    <dict>\n" );
#else
    PyObject *dict = python_persistent, *items, *key, *value;
    int i, len;
    char *str;

    if ( has_hints || (dict!=NULL && PyMapping_Check(dict)) ) {
	if ( sc!=NULL ) {
	    fprintf( file, "  <lib>\n" );
	    fprintf( file, "    <dict>\n" );
	}
	if ( has_hints ) {
#endif	
	    fprintf( file, "      <key>com.fontlab.hintData</key>\n" );
	    fprintf( file, "      <dict>\n" );
	    if ( sc->hstem!=NULL ) {
		fprintf( file, "\t<key>hhints</key>\n" );
		fprintf( file, "\t<array>\n" );
		for ( h = sc->hstem; h!=NULL; h=h->next ) {
		    fprintf( file, "\t  <dict>\n" );
		    fprintf( file, "\t    <key>position</key>" );
		    fprintf( file, "\t    <integer>%d</integer>\n", (int) rint(h->start));
		    fprintf( file, "\t    <key>width</key>" );
		    fprintf( file, "\t    <integer>%d</integer>\n", (int) rint(h->width));
		    fprintf( file, "\t  </dict>\n" );
		}
		fprintf( file, "\t</array>\n" );
	    }
	    if ( sc->vstem!=NULL ) {
		fprintf( file, "\t<key>vhints</key>\n" );
		fprintf( file, "\t<array>\n" );
		for ( h = sc->vstem; h!=NULL; h=h->next ) {
		    fprintf( file, "\t  <dict>\n" );
		    fprintf( file, "\t    <key>position</key>\n" );
		    fprintf( file, "\t    <integer>%d</integer>\n", (int) rint(h->start));
		    fprintf( file, "\t    <key>width</key>\n" );
		    fprintf( file, "\t    <integer>%d</integer>\n", (int) rint(h->width));
		    fprintf( file, "\t  </dict>\n" );
		}
		fprintf( file, "\t</array>\n" );
	    }
	    fprintf( file, "      </dict>\n" );
#ifndef _NO_PYTHON
	}
	/* Ok, look at the persistent data and output it (all except for a */
	/*  hint entry -- we've already handled that with the real hints, */
	/*  no point in retaining out of date hints too */
	if ( dict != NULL ) {
	    items = PyMapping_Items(dict);
	    len = PySequence_Size(items);
	    for ( i=0; i<len; ++i ) {
		PyObject *item = PySequence_GetItem(items,i);
		key = PyTuple_GetItem(item,0);
		if ( !PyString_Check(key))		/* Keys need not be strings */
	    continue;
		str = PyString_AsString(key);
		if ( strcmp(str,"com.fontlab.hintData")==0 && sc!=NULL )	/* Already done */
	    continue;
		value = PyTuple_GetItem(item,1);
		if ( !PyObjDumpable(value))
	    continue;
		fprintf( file, "    <key>%s</key>\n", str );
		DumpPyObject( file, value );
	    }
	}
#endif
	if ( sc!=NULL ) {
	    fprintf( file, "    </dict>\n" );
	    fprintf( file, "  </lib>\n" );
	}
    }
}

#ifndef _NO_PYTHON
static int PyObjDumpable(PyObject *value) {
    if ( PyInt_Check(value))
return( true );
    if ( PyFloat_Check(value))
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

static void DumpPyObject( FILE *file, PyObject *value ) {
    if ( PyMapping_Check(value))
	DumpPythonLib(file,value,NULL);
    else if ( PyString_Check(value)) {		/* Must precede the sequence check */
	char *str = PyString_AsString(value);
	fprintf( file, "      <string>%s</string>\n", str );
    } else if ( value==Py_True )
	fprintf( file, "      <true/>\n" );
    else if ( value==Py_False )
	fprintf( file, "      <false/>\n" );
    else if ( value==Py_None )
	fprintf( file, "      <none/>\n" );
    else if (PyInt_Check(value))
	fprintf( file, "      <integer>%ld</integer>\n", PyInt_AsLong(value) );
    else if (PyInt_Check(value))
	fprintf( file, "      <real>%g</real>\n", PyFloat_AsDouble(value) );
    else if (PySequence_Check(value)) {
	int i, len = PySequence_Size(value);

	fprintf( file, "      <array>\n" );
	for ( i=0; i<len; ++i ) {
	    PyObject *obj = PySequence_GetItem(value,i);
	    if ( PyObjDumpable(obj)) {
		fprintf( file, "  ");
		DumpPyObject(file,obj);
	    }
	}
	fprintf( file, "      </array>\n" );
    }
}
#endif

/* ************************************************************************** */
/* ****************************   GLIF Output    **************************** */
/* ************************************************************************** */
static int _GlifDump(FILE *glif,SplineChar *sc,int layer) {
    struct altuni *altuni;
    int isquad = sc->layers[layer].order2;
    SplineSet *spl;
    SplinePoint *sp;
    RefChar *ref;
    int err;

    if ( glif==NULL )
return( false );

    fprintf( glif, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" );
    /* No DTD for these guys??? */
    fprintf( glif, "<glyph name=\"%s\" format=\"1\">\n", sc->name );
    if ( sc->parent->hasvmetrics )
	fprintf( glif, "  <advance width=\"%d\" height=\"%d\"/>\n", sc->width, sc->vwidth );
    else
	fprintf( glif, "  <advance width=\"%d\"/>\n", sc->width );
    if ( sc->unicodeenc!=-1 )
	fprintf( glif, "  <unicode hex=\"%04x\"/>\n", sc->unicodeenc );
    for ( altuni = sc->altuni; altuni!=NULL; altuni = altuni->next )
	if ( altuni->vs==-1 && altuni->fid==0 )
	    fprintf( glif, "  <unicode hex=\"%04x\"/>\n", altuni->unienc );

    if ( sc->layers[layer].refs!=NULL || sc->layers[layer].splines!=NULL ) {
	fprintf( glif, "  <outline>\n" );
	for ( ref = sc->layers[layer].refs; ref!=NULL; ref=ref->next ) if ( SCWorthOutputting(ref->sc)) {
	    fprintf( glif, "    <component base=\"%s\"", ref->sc->name );
	    if ( ref->transform[0]!=1 )
		fprintf( glif, " xScale=\"%g\"", (double) ref->transform[0] );
	    if ( ref->transform[3]!=1 )
		fprintf( glif, " yScale=\"%g\"", (double) ref->transform[3] );
	    if ( ref->transform[1]!=0 )
		fprintf( glif, " xyScale=\"%g\"", (double) ref->transform[1] );
	    if ( ref->transform[2]!=0 )
		fprintf( glif, " yxScale=\"%g\"", (double) ref->transform[2] );
	    if ( ref->transform[4]!=0 )
		fprintf( glif, " xOffset=\"%g\"", (double) ref->transform[4] );
	    if ( ref->transform[5]!=0 )
		fprintf( glif, " yOffset=\"%g\"", (double) ref->transform[5] );
	    fprintf( glif, "/>\n" );
	}
	for ( spl=sc->layers[layer].splines; spl!=NULL; spl=spl->next ) {
	    fprintf( glif, "    <contour>\n" );
	    for ( sp=spl->first; sp!=NULL; ) {
		/* Undocumented fact: If a contour contains a series of off-curve points with no on-curve then treat as quadratic even if no qcurve */
		if ( !isquad || /*sp==spl->first ||*/ !SPInterpolate(sp) )
		    fprintf( glif, "      <point x=\"%g\" y=\"%g\" type=\"%s\" smooth=\"%s\"/>\n",
			    (double) sp->me.x, (double) sp->me.y,
			    sp->prev==NULL        ? "move"   :
			    sp->prev->knownlinear ? "line"   :
			    isquad 		      ? "qcurve" :
						    "curve",
			    sp->pointtype!=pt_corner?"yes":"no" );
		if ( sp->next==NULL )
	    break;
		if ( !sp->next->knownlinear )
		    fprintf( glif, "      <point x=\"%g\" y=\"%g\"/>\n",
			    (double) sp->nextcp.x, (double) sp->nextcp.y );
		sp = sp->next->to;
		if ( !isquad && !sp->prev->knownlinear )
		    fprintf( glif, "      <point x=\"%g\" y=\"%g\"/>\n",
			    (double) sp->prevcp.x, (double) sp->prevcp.y );
		if ( sp==spl->first )
	    break;
	    }
	    fprintf( glif, "    </contour>\n" );
	}
	fprintf( glif, "  </outline>\n" );
    }
    DumpPythonLib(glif,sc->python_persistent,sc);
    fprintf( glif, "</glyph>\n" );
    err = ferror(glif);
    if ( fclose(glif))
	err = true;
return( !err );
}

static int GlifDump(char *glyphdir,char *gfname,SplineChar *sc,int layer) {
    char *gn = buildname(glyphdir,gfname);
    FILE *glif = fopen(gn,"w");
    int ret = _GlifDump(glif,sc,layer);
    free(gn);
return( ret );
}

int _ExportGlif(FILE *glif,SplineChar *sc,int layer) {
return( _GlifDump(glif,sc,layer));
}

/* ************************************************************************** */
/* ****************************    UFO Output    **************************** */
/* ************************************************************************** */

static void PListOutputHeader(FILE *plist) {
    fprintf( plist, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" );
    fprintf( plist, "<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n" );
    fprintf( plist, "<plist version=\"1.0\">\n" );
    fprintf( plist, "    <dict>\n" );
}

static FILE *PListCreate(char *basedir,char *sub) {
    char *fname = buildname(basedir,sub);
    FILE *plist = fopen( fname, "w" );

    free(fname);
    if ( plist==NULL )
return( NULL );
    PListOutputHeader(plist);
return( plist );
}

static int PListOutputTrailer(FILE *plist) {
    int ret = true;
    fprintf( plist, "    </dict>\n" );
    fprintf( plist, "</plist>\n" );
    if ( ferror(plist))
	ret = false;
    if ( fclose(plist) )
	ret = false;
return( ret );
}

static void PListOutputString(FILE *plist, char *key, char *value) {
    if ( value==NULL ) value = "";
    fprintf( plist, "\t<key>%s</key>\n", key );
    fprintf( plist, "\t<string>" );
    while ( *value ) {
	if ( *value=='<' )
	    fprintf( plist,"&lt;");
	else if ( *value == '&' )
	    fprintf( plist,"&amp;");
	else
	    putc(*value,plist);
	++value;
    }
    fprintf(plist, "</string>\n" );
}

static void PListOutputInteger(FILE *plist, char *key, int value) {
    fprintf( plist, "\t<key>%s</key>\n", key );
    fprintf( plist, "\t<integer>%d</integer>\n", value );
}

static void PListOutputReal(FILE *plist, char *key, double value) {
    fprintf( plist, "\t<key>%s</key>\n", key );
    fprintf( plist, "\t<real>%g</real>\n", value );
}

#if 0
static void PListOutputBoolean(FILE *plist, char *key, int value) {
    fprintf( plist, "\t<key>%s</key>\n", key );
    fprintf( plist, value ? "\t<true/>\n" : "\t<false/>\n" );
}
#endif

static int UFOOutputMetaInfo(char *basedir,SplineFont *sf) {
    FILE *plist = PListCreate( basedir, "metainfo.plist" );

    if ( plist==NULL )
return( false );
    PListOutputString(plist,"creator","FontForge");
    PListOutputInteger(plist,"formatVersion",1);
return( PListOutputTrailer(plist));
}

static int UFOOutputFontInfo(char *basedir,SplineFont *sf, int layer) {
    FILE *plist = PListCreate( basedir, "fontinfo.plist" );

    if ( plist==NULL )
return( false );
    PListOutputString(plist,"familyName",sf->familyname);
    /* FontForge does not maintain a stylename except possibly in the ttfnames section where there are many different languages of it */
    PListOutputString(plist,"fullName",sf->fullname);
    PListOutputString(plist,"fontName",sf->fontname);
    /* FontForge does not maintain a menuname except possibly in the ttfnames section where there are many different languages of it */
    PListOutputString(plist,"weightName",sf->weight);
    PListOutputString(plist,"copyright",sf->copyright);
    PListOutputInteger(plist,"unitsPerEm",sf->ascent+sf->descent);
    PListOutputInteger(plist,"ascender",sf->ascent);
    PListOutputInteger(plist,"descender",-sf->descent);
    PListOutputReal(plist,"italicAngle",sf->italicangle);
    PListOutputString(plist,"curveType",sf->layers[layer].order2 ? "Quadratic" : "Cubic");
return( PListOutputTrailer(plist));
}

static int UFOOutputGroups(char *basedir,SplineFont *sf) {
    FILE *plist = PListCreate( basedir, "groups.plist" );

    if ( plist==NULL )
return( false );
    /* These don't act like fontforge's groups. There are comments that this */
    /*  could be used for defining classes (kerning classes, etc.) but no */
    /*  resolution saying that the actually are. */
    /* Should I omit a file I don't use? Or leave it blank? */
return( PListOutputTrailer(plist));
}

static void KerningPListOutputGlyph(FILE *plist, char *key,KernPair *kp) {
    fprintf( plist, "\t<key>%s</key>\n", key );
    fprintf( plist, "\t<dict>\n" );
    while ( kp!=NULL ) {
	if ( kp->off!=0 && SCWorthOutputting(kp->sc)) {
	    fprintf( plist, "\t    <key>%s</key>\n", kp->sc->name );
	    fprintf( plist, "\t    <integer>%d</integer>\n", kp->off );
	}
	kp = kp->next;
    }
    fprintf( plist, "\t</dict>\n" );
}

static int UFOOutputKerning(char *basedir,SplineFont *sf) {
    FILE *plist = PListCreate( basedir, "kerning.plist" );
    SplineChar *sc;
    int i;

    if ( plist==NULL )
return( false );
    /* There is some muttering about how to do kerning by classes, but no */
    /*  resolution to those thoughts. So I ignore the issue */
    for ( i=0; i<sf->glyphcnt; ++i ) if ( SCWorthOutputting(sc=sf->glyphs[i]) && sc->kerns!=NULL )
	KerningPListOutputGlyph(plist,sc->name,sc->kerns);
return( PListOutputTrailer(plist));
}

static int UFOOutputVKerning(char *basedir,SplineFont *sf) {
    FILE *plist;
    SplineChar *sc;
    int i;

    for ( i=sf->glyphcnt-1; i>=0; --i ) if ( SCWorthOutputting(sc=sf->glyphs[i]) && sc->vkerns!=NULL )
    break;
    if ( i<0 )
return( true );

    plist = PListCreate( basedir, "vkerning.plist" );
    if ( plist==NULL )
return( false );
    for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL && sc->vkerns!=NULL )
	KerningPListOutputGlyph(plist,sc->name,sc->vkerns);
return( PListOutputTrailer(plist));
}

static int UFOOutputLib(char *basedir,SplineFont *sf) {
#ifndef _NO_PYTHON
    if ( sf->python_persistent!=NULL && PyMapping_Check(sf->python_persistent) ) {
	FILE *plist = PListCreate( basedir, "lib.plist" );

	if ( plist==NULL )
return( false );
	DumpPythonLib(plist,sf->python_persistent,NULL);
return( PListOutputTrailer(plist));
    }
#endif
return( true );
}

int WriteUFOFont(char *basedir,SplineFont *sf,enum fontformat ff,int flags,
	EncMap *map,int layer) {
    char *foo = galloc( strlen(basedir) +20 ), *glyphdir, *gfname;
    int err;
    FILE *plist;
    int i;
    SplineChar *sc;

    /* Clean it out, if it exists */
    sprintf( foo, "rm -rf %s", basedir );
    system( foo );
    free( foo );

    /* Create it */
    mkdir( basedir, 0755 );

    err  = !UFOOutputMetaInfo(basedir,sf);
    err |= !UFOOutputFontInfo(basedir,sf,layer);
    err |= !UFOOutputGroups(basedir,sf);
    err |= !UFOOutputKerning(basedir,sf);
    err |= !UFOOutputVKerning(basedir,sf);
    err |= !UFOOutputLib(basedir,sf);

    if ( err )
return( false );

    glyphdir = buildname(basedir,"glyphs");
    mkdir( glyphdir, 0755 );

    plist = PListCreate(glyphdir,"contents.plist");
    if ( plist==NULL ) {
	free(glyphdir);
return( false );
    }

    for ( i=0; i<sf->glyphcnt; ++i ) if ( SCWorthOutputting(sc=sf->glyphs[i]) ) {
	gfname = galloc(strlen(sc->name)+20);
	if ( isupper(sc->name[0])) {
	    char *pt;
	    pt = strchr(sc->name,'.');
	    if ( pt==NULL ) {
		strcpy(gfname,sc->name);
		strcat(gfname,"_");
	    } else {
		strncpy(gfname,sc->name,pt-sc->name);
		gfname[pt-sc->name] = '-';
		strcpy(gfname + (pt-sc->name) + 1,pt);
	    }
	} else
	    strcpy(gfname,sc->name);
	strcat(gfname,".glif");
	PListOutputString(plist,sc->name,gfname);
	err |= !GlifDump(glyphdir,gfname,sc,layer);
	free(gfname);
    }
    free( glyphdir );
    err |= !PListOutputTrailer(plist);
return( !err );
}

/* ************************************************************************** */
/* *****************************    UFO Input    **************************** */
/* ************************************************************************** */

static char *get_thingy(FILE *file,char *buffer,char *tag) {
    int ch;
    char *pt;

    forever {
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
		ret = gcalloc(2,sizeof(char *));
		ret[0] = copy(buffer);
return( ret );
	    }
return( NULL );
	}
    }
return( NULL );
}

#if _NO_LIBXML
int HasUFO(void) {
return( false );
}

SplineFont *SFReadUFO(char *filename, int flags) {
return( NULL );
}

SplineSet *SplinePointListInterpretGlif(char *filename,char *memory, int memlen,
	int em_size,int ascent,int is_stroked) {
return( NULL );
}
#else

#ifndef HAVE_ICONV_H
# undef iconv
# undef iconv_t
# undef iconv_open
# undef iconv_close
#endif

#undef extended			/* used in xlink.h */
#include <libxml/parser.h>

# if defined(_STATIC_LIBXML) || defined(NODYNAMIC)

#define _xmlParseMemory		xmlParseMemory
#define _xmlParseFile		xmlParseFile
#define _xmlDocGetRootElement	xmlDocGetRootElement
#define _xmlFreeDoc		xmlFreeDoc
#ifdef __CygWin
# define _xmlFree		free
/* Nasty kludge, but xmlFree doesn't work on cygwin (or I can't get it to) */
#else
# define _xmlFree		xmlFree
#endif
#define _xmlStrcmp		xmlStrcmp
#define _xmlGetProp		xmlGetProp
#define _xmlNodeListGetString		xmlNodeListGetString

static int libxml_init_base() {
return( true );
}
# else
#  include <dynamic.h>

static DL_CONST void *libxml;
static xmlDocPtr (*_xmlParseMemory)(const char *memory,int memsize);
static xmlDocPtr (*_xmlParseFile)(const char *filename);
static xmlNodePtr (*_xmlDocGetRootElement)(xmlDocPtr doc);
static void (*_xmlFreeDoc)(xmlDocPtr doc);
static void (*_xmlFree)(void *);
static int (*_xmlStrcmp)(const xmlChar *,const xmlChar *);
static xmlChar *(*_xmlGetProp)(xmlNodePtr,const xmlChar *);
static xmlChar *(*_xmlNodeListGetString)(xmlDocPtr,xmlNodePtr,int);

static int libxml_init_base() {
    static int xmltested = false;

    if ( xmltested )
return( libxml!=NULL );

    dlopen("libz" SO_EXT,RTLD_GLOBAL|RTLD_LAZY);

    libxml = dlopen( "libxml2" SO_EXT,RTLD_LAZY);
# ifdef SO_2_EXT
    if ( libxml==NULL )
	libxml = dlopen("libxml2" SO_2_EXT,RTLD_LAZY);
# endif
    xmltested = true;
    if ( libxml==NULL )
return( false );

    _xmlParseMemory = (xmlDocPtr (*)(const char *,int)) dlsym(libxml,"xmlParseMemory");
    _xmlParseFile = (xmlDocPtr (*)(const char *)) dlsym(libxml,"xmlParseFile");
    _xmlDocGetRootElement = (xmlNodePtr (*)(xmlDocPtr )) dlsym(libxml,"xmlDocGetRootElement");
    _xmlFreeDoc = (void (*)(xmlDocPtr)) dlsym(libxml,"xmlFreeDoc");
    /* xmlFree is done differently for threaded and non-threaded libraries. */
    /*  I hope this gets both right... */
    if ( dlsym(libxml,"__xmlFree")) {
	xmlFreeFunc *(*foo)(void) = (xmlFreeFunc *(*)(void)) dlsym(libxml,"__xmlFree");
	_xmlFree = *(*foo)();
    } else {
	xmlFreeFunc *foo = dlsym(libxml,"xmlFree");
	_xmlFree = *foo;
    }
    _xmlStrcmp = (int (*)(const xmlChar *,const xmlChar *)) dlsym(libxml,"xmlStrcmp");
    _xmlGetProp = (xmlChar *(*)(xmlNodePtr,const xmlChar *)) dlsym(libxml,"xmlGetProp");
    _xmlNodeListGetString = (xmlChar *(*)(xmlDocPtr,xmlNodePtr,int)) dlsym(libxml,"xmlNodeListGetString");
    if ( _xmlParseFile==NULL || _xmlDocGetRootElement==NULL || _xmlFree==NULL ) {
	libxml = NULL;
return( false );
    }

return( true );
}
# endif

static xmlNodePtr FindNode(xmlNodePtr kids,char *name) {
    while ( kids!=NULL ) {
	if ( _xmlStrcmp(kids->name,(const xmlChar *) name)== 0 )
return( kids );
	kids = kids->next;
    }
return( NULL );
}

#ifndef _NO_PYTHON
static PyObject *XMLEntryToPython(xmlDocPtr doc,xmlNodePtr entry);

static PyObject *LibToPython(xmlDocPtr doc,xmlNodePtr dict) {
    PyObject *pydict = PyDict_New();
    PyObject *item;
    xmlNodePtr keys, temp;

    for ( keys=dict->children; keys!=NULL; keys=keys->next ) {
	if ( _xmlStrcmp(keys->name,(const xmlChar *) "key")== 0 ) {
	    char *keyname = (char *) _xmlNodeListGetString(doc,keys->children,true);
	    for ( temp=keys->next; temp!=NULL; temp=temp->next ) {
		if ( _xmlStrcmp(temp->name,(const xmlChar *) "text")!=0 )
	    break;
	    }
	    item = XMLEntryToPython(doc,temp);
	    if ( item!=NULL )
		PyDict_SetItemString(pydict, keyname, item );
	    if ( temp==NULL )
	break;
	    else if ( _xmlStrcmp(temp->name,(const xmlChar *) "key")!=0 )
		keys = temp->next;
	    free(keyname);
	}
    }
return( pydict );
}

static PyObject *XMLEntryToPython(xmlDocPtr doc,xmlNodePtr entry) {
    char *contents;

    if ( _xmlStrcmp(entry->name,(const xmlChar *) "true")==0 ) {
	Py_INCREF(Py_True);
return( Py_True );
    }
    if ( _xmlStrcmp(entry->name,(const xmlChar *) "false")==0 ) {
	Py_INCREF(Py_False);
return( Py_False );
    }
    if ( _xmlStrcmp(entry->name,(const xmlChar *) "none")==0 ) {
	Py_INCREF(Py_None);
return( Py_None );
    }

    if ( _xmlStrcmp(entry->name,(const xmlChar *) "dict")==0 )
return( LibToPython(doc,entry));
    if ( _xmlStrcmp(entry->name,(const xmlChar *) "array")==0 ) {
	xmlNodePtr sub;
	int cnt;
	PyObject *ret, *item;
	/* I'm translating "Arrays" as tuples... not sure how to deal with */
	/*  actual python arrays. But these look more like tuples than arrays*/
	/*  since each item gets its own type */

	for ( cnt=0, sub=entry->children; sub!=NULL; sub=sub->next ) {
	    if ( _xmlStrcmp(sub->name,(const xmlChar *) "text")==0 )
	continue;
	    ++cnt;
	}
	ret = PyTuple_New(cnt);
	for ( cnt=0, sub=entry->children; sub!=NULL; sub=sub->next ) {
	    if ( _xmlStrcmp(sub->name,(const xmlChar *) "text")==0 )
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

    contents = (char *) _xmlNodeListGetString(doc,entry->children,true);
    if ( _xmlStrcmp(entry->name,(const xmlChar *) "integer")==0 ) {
	long val = strtol(contents,NULL,0);
	free(contents);
return( Py_BuildValue("i",val));
    }
    if ( _xmlStrcmp(entry->name,(const xmlChar *) "real")==0 ) {
	double val = strtod(contents,NULL);
	free(contents);
return( Py_BuildValue("d",val));
    }
    if ( _xmlStrcmp(entry->name,(const xmlChar *) "string")==0 ) {
	PyObject *ret = Py_BuildValue("s",contents);
	free(contents);
return( ret );
    }
    LogError("Unknown python type <%s> when reading UFO/GLIF lib data.", (char *) entry->name);
    free( contents );
return( NULL );
}
#endif

static StemInfo *GlifParseHints(xmlDocPtr doc,xmlNodePtr dict,char *hinttype) {
    StemInfo *head=NULL, *last=NULL, *h;
    xmlNodePtr keys, array, kids, poswidth,temp;
    double pos, width;

    for ( keys=dict->children; keys!=NULL; keys=keys->next ) {
	if ( _xmlStrcmp(keys->name,(const xmlChar *) "key")== 0 ) {
	    char *keyname = (char *) _xmlNodeListGetString(doc,keys->children,true);
	    int found = strcmp(keyname,hinttype)==0;
	    free(keyname);
	    if ( found ) {
		for ( array=keys->next; array!=NULL; array=array->next ) {
		    if ( _xmlStrcmp(array->name,(const xmlChar *) "array")==0 )
		break;
		}
		if ( array!=NULL ) {
		    for ( kids = array->children; kids!=NULL; kids=kids->next ) {
			if ( _xmlStrcmp(kids->name,(const xmlChar *) "dict")==0 ) {
			    pos = -88888888; width = 0;
			    for ( poswidth=kids->children; poswidth!=NULL; poswidth=poswidth->next ) {
				if ( _xmlStrcmp(poswidth->name,(const xmlChar *) "key")==0 ) {
				    char *keyname2 = (char *) _xmlNodeListGetString(doc,poswidth->children,true);
				    int ispos = strcmp(keyname2,"position")==0, iswidth = strcmp(keyname2,"width")==0;
				    double value;
				    free(keyname2);
				    for ( temp=poswidth->next; temp!=NULL; temp=temp->next ) {
					if ( _xmlStrcmp(temp->name,(const xmlChar *) "text")!=0 )
				    break;
				    }
				    if ( temp!=NULL ) {
					char *valname = (char *) _xmlNodeListGetString(doc,temp->children,true);
					if ( _xmlStrcmp(temp->name,(const xmlChar *) "integer")==0 )
					    value = strtol(valname,NULL,10);
					else if ( _xmlStrcmp(temp->name,(const xmlChar *) "real")==0 )
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

static SplineChar *_UFOLoadGlyph(xmlDocPtr doc,char *glifname) {
    xmlNodePtr glyph, kids, contour, points;
    SplineChar *sc;
    xmlChar *format, *width, *height, *u;
    char *name;
    int uni;
    char *cpt;
    int wasquad = -1;	/* Unspecified */
    SplineSet *last = NULL;

    glyph = _xmlDocGetRootElement(doc);
    format = _xmlGetProp(glyph,(xmlChar *) "format");
    if ( _xmlStrcmp(glyph->name,(const xmlChar *) "glyph")!=0 ||
	    (format!=NULL && _xmlStrcmp(format,(xmlChar *) "1")!=0)) {
	LogError(_("Expected glyph file with format==1\n"));
	_xmlFreeDoc(doc);
	free(format);
return( NULL );
    }
    name = (char *) _xmlGetProp(glyph,(xmlChar *) "name");
    if ( name==NULL && glifname!=NULL ) {
	char *pt = strrchr(glifname,'/');
	name = copy(pt+1);
	for ( pt=cpt=name; *cpt!='\0'; ++cpt ) {
	    if ( *cpt!='_' )
		*pt++ = *cpt;
	}
	*pt = '\0';
    } else if ( name==NULL )
	name = copy("nameless");
    sc = SplineCharCreate(2);
    sc->name = name;
    last = NULL;

    for ( kids = glyph->children; kids!=NULL; kids=kids->next ) {
	if ( _xmlStrcmp(kids->name,(const xmlChar *) "advance")==0 ) {
	    width = _xmlGetProp(kids,(xmlChar *) "width");
	    height = _xmlGetProp(kids,(xmlChar *) "height");
	    if ( width!=NULL )
		sc->width = strtol((char *) width,NULL,10);
	    if ( height!=NULL )
		sc->vwidth = strtol((char *) height,NULL,10);
	    sc->widthset = true;
	    free(width); free(height);
	} else if ( _xmlStrcmp(kids->name,(const xmlChar *) "unicode")==0 ) {
	    u = _xmlGetProp(kids,(xmlChar *) "hex");
	    uni = strtol((char *) u,NULL,16);
	    if ( sc->unicodeenc == -1 )
		sc->unicodeenc = uni;
	    else
		AltUniAdd(sc,uni);
	} else if ( _xmlStrcmp(kids->name,(const xmlChar *) "outline")==0 ) {
	    for ( contour = kids->children; contour!=NULL; contour=contour->next ) {
		if ( _xmlStrcmp(contour->name,(const xmlChar *) "component")==0 ) {
		    char *base = (char *) _xmlGetProp(contour,(xmlChar *) "base"),
			*xs = (char *) _xmlGetProp(contour,(xmlChar *) "xScale"),
			*ys = (char *) _xmlGetProp(contour,(xmlChar *) "yScale"),
			*xys = (char *) _xmlGetProp(contour,(xmlChar *) "xyScale"),
			*yxs = (char *) _xmlGetProp(contour,(xmlChar *) "yxScale"),
			*xo = (char *) _xmlGetProp(contour,(xmlChar *) "xOffset"),
			*yo = (char *) _xmlGetProp(contour,(xmlChar *) "yOffset");
		    RefChar *r;
		    if ( base==NULL )
			LogError(_("component with no base glyph"));
		    else {
			r = RefCharCreate();
			r->sc = (SplineChar *) base;
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
			r->next = sc->layers[ly_fore].refs;
			sc->layers[ly_fore].refs = r;
		    }
		    free(xs); free(ys); free(xys); free(yxs); free(xo); free(yo);
		} else if ( _xmlStrcmp(contour->name,(const xmlChar *) "contour")==0 ) {
		    SplineSet *ss;
		    SplinePoint *sp;
		    BasePoint pre[2], init[4];
		    int precnt=0, initcnt=0, open=0;

		    ss = chunkalloc(sizeof(SplineSet));
		    for ( points = contour->children; points!=NULL; points=points->next ) {
			char *xs, *ys, *type;
			double x,y;
			if ( _xmlStrcmp(points->name,(const xmlChar *) "point")!=0 )
		    continue;
			xs = (char *) _xmlGetProp(points,(xmlChar *) "x");
			ys = (char *) _xmlGetProp(points,(xmlChar *) "y");
			type = (char *) _xmlGetProp(points,(xmlChar *) "type");
			if ( xs==NULL || ys == NULL )
		    continue;
			x = strtod(xs,NULL); y = strtod(ys,NULL);
			if ( type!=NULL && (strcmp(type,"move")==0 ||
					    strcmp(type,"line")==0 ||
					    strcmp(type,"curve")==0 ||
					    strcmp(type,"qcurve")==0 )) {
			    sp = SplinePointCreate(x,y);
			    if ( strcmp(type,"move")==0 ) {
				open = true;
			        ss->first = ss->last = sp;
			    } else if ( ss->first==NULL ) {
				ss->first = ss->last = sp;
			        memcpy(init,pre,sizeof(pre));
			        initcnt = precnt;
			        if ( strcmp(type,"qcurve")==0 )
				    wasquad = true;
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
				if ( precnt==2 ) {
				    SplinePoint *sp = SplinePointCreate((pre[1].x+pre[0].x)/2,(pre[1].y+pre[0].y)/2);
				    sp->prevcp = ss->last->nextcp = pre[0];
				    sp->noprevcp = ss->last->nonextcp = false;
				    SplineMake(ss->last,sp,true);
				    ss->last = sp;
				}
			        if ( precnt>=1 ) {
				    ss->last->nextcp = sp->prevcp = pre[precnt-1];
			            ss->last->nonextcp = sp->noprevcp = false;
				}
				SplineMake(ss->last,sp,true);
			        ss->last = sp;
			    }
			    precnt = 0;
			} else {
			    if ( wasquad==-1 && precnt==2 ) {
				/* Undocumented fact: If there are no on-curve points (and therefore no indication of quadratic/cubic), assume truetype implied points */
				memcpy(init,pre,sizeof(pre));
				initcnt = 1;
				sp = SplinePointCreate((pre[1].x+pre[0].x)/2,(pre[1].y+pre[0].y)/2);
			        sp->nextcp = pre[1];
			        sp->nonextcp = false;
			        if ( ss->first==NULL )
				    ss->first = sp;
				else {
				    ss->last->nextcp = sp->prevcp = pre[0];
			            ss->last->nonextcp = sp->noprevcp = false;
			            initcnt = 0;
			            SplineMake(ss->last,sp,true);
				}
			        ss->last = sp;
				sp = SplinePointCreate((x+pre[1].x)/2,(y+pre[1].y)/2);
			        sp->prevcp = pre[1];
			        sp->noprevcp = false;
			        SplineMake(ss->last,sp,true);
			        ss->last = sp;
			        pre[0].x = x; pre[0].y = y;
			        precnt = 1;
				wasquad = true;
			    } else if ( wasquad==true && precnt==1 ) {
				sp = SplinePointCreate((x+pre[0].x)/2,(y+pre[0].y)/2);
			        sp->prevcp = pre[0];
			        sp->noprevcp = false;
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
			free(xs); free(ys); free(type);
		    }
		    if ( !open ) {
			if ( precnt!=0 ) {
			    BasePoint temp[2];
			    memcpy(temp,init,sizeof(temp));
			    memcpy(init,pre,sizeof(pre));
			    memcpy(init+precnt,temp,sizeof(temp));
			    initcnt += precnt;
			}
			if ( (wasquad==true && initcnt>0) || initcnt==1 ) {
			    int i;
			    for ( i=0; i<initcnt-1; ++i ) {
				sp = SplinePointCreate((init[i+1].x+init[i].x)/2,(init[i+1].y+init[i].y)/2);
			        sp->prevcp = ss->last->nextcp = init[i];
			        sp->noprevcp = ss->last->nonextcp = false;
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
			}
			SplineMake(ss->last,ss->first,wasquad);
			ss->last = ss->first;
		    }
		    if ( last==NULL )
			sc->layers[ly_fore].splines = ss;
		    else
			last->next = ss;
		    last = ss;
		}
	    }
	} else if ( _xmlStrcmp(kids->name,(const xmlChar *) "lib")==0 ) {
	    xmlNodePtr keys, temp, dict = FindNode(kids->children,"dict");
	    if ( dict!=NULL ) {
		for ( keys=dict->children; keys!=NULL; keys=keys->next ) {
		    if ( _xmlStrcmp(keys->name,(const xmlChar *) "key")== 0 ) {
			char *keyname = (char *) _xmlNodeListGetString(doc,keys->children,true);
			int found = strcmp(keyname,"com.fontlab.hintData")==0;
			free(keyname);
			if ( found ) {
			    for ( temp=keys->next; temp!=NULL; temp=temp->next ) {
				if ( _xmlStrcmp(temp->name,(const xmlChar *) "dict")==0 )
			    break;
			    }
			    if ( temp!=NULL ) {
				sc->hstem = GlifParseHints(doc,temp,"hhints");
				sc->vstem = GlifParseHints(doc,temp,"vhints");
			        SCGuessHHintInstancesList(sc,ly_fore);
			        SCGuessVHintInstancesList(sc,ly_fore);
			    }
		break;
			}
		    }
		}
#ifndef _NO_PYTHON
		sc->python_persistent = LibToPython(doc,dict);
#endif
	    }
	}
    }
    _xmlFreeDoc(doc);
    SPLCatagorizePoints(sc->layers[ly_fore].splines);
return( sc );
}

static SplineChar *UFOLoadGlyph(char *glifname) {
    xmlDocPtr doc;

    doc = _xmlParseFile(glifname);
    if ( doc==NULL ) {
	LogError( _("Bad glif file %s\n" ), glifname);
return( NULL );
    }
return( _UFOLoadGlyph(doc,glifname));
}


static void UFORefFixup(SplineFont *sf, SplineChar *sc ) {
    RefChar *r, *prev;
    SplineChar *rsc;

    if ( sc==NULL || sc->ticked )
return;
    sc->ticked = true;
    prev = NULL;
    for ( r=sc->layers[ly_fore].refs; r!=NULL; r=r->next ) {
	rsc = SFGetChar(sf,-1,(char *) (r->sc));
	if ( rsc==NULL ) {
	    LogError( _("Failed to find glyph %s when fixing up references\n"), (char *) r->sc );
	    if ( prev==NULL )
		sc->layers[ly_fore].refs = r->next;
	    else
		prev->next = r->next;
	    free((char *) r->sc);
	    /* Memory leak. We loose r */
	} else {
	    UFORefFixup(sf,rsc);
	    free((char *) r->sc);
	    r->sc = rsc;
	    prev = r;
	    SCReinstanciateRefChar(sc,r,ly_fore);
	}
    }
}    

static void UFOLoadGlyphs(SplineFont *sf,char *glyphdir) {
    char *glyphlist = buildname(glyphdir,"contents.plist");
    xmlDocPtr doc;
    xmlNodePtr plist, dict, keys, value;
    char *valname, *glyphfname;
    int i;
    SplineChar *sc;
    int tot;

    doc = _xmlParseFile(glyphlist);
    free(glyphlist);
    if ( doc==NULL ) {
	LogError( _("Bad contents.plist\n" ));
return;
    }
    plist = _xmlDocGetRootElement(doc);
    dict = FindNode(plist->children,"dict");
    if ( _xmlStrcmp(plist->name,(const xmlChar *) "plist")!=0 || dict == NULL ) {
	LogError(_("Expected property list file"));
	_xmlFreeDoc(doc);
return;
    }
    for ( tot=0, keys=dict->children; keys!=NULL; keys=keys->next ) {
	if ( _xmlStrcmp(keys->name,(const xmlChar *) "key")==0 )
	    ++tot;
    }
    ff_progress_change_total(tot);
    for ( keys=dict->children; keys!=NULL; keys=keys->next ) {
	for ( value = keys->next; value!=NULL && _xmlStrcmp(value->name,(const xmlChar *) "text")==0;
		value = value->next );
	if ( value==NULL )
    break;
	if ( _xmlStrcmp(keys->name,(const xmlChar *) "key")==0 ) {
	    valname = (char *) _xmlNodeListGetString(doc,value->children,true);
	    glyphfname = buildname(glyphdir,valname);
	    free(valname);
	    sc = UFOLoadGlyph(glyphfname);
	    if ( sc!=NULL ) {
		sc->parent = sf;
		if ( sf->glyphcnt>=sf->glyphmax )
		    sf->glyphs = grealloc(sf->glyphs,(sf->glyphmax+=100)*sizeof(SplineChar *));
		sc->orig_pos = sf->glyphcnt;
		sf->glyphs[sf->glyphcnt++] = sc;
	    }
	    keys = value;
	    ff_progress_next();
	}
    }
    _xmlFreeDoc(doc);

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
	doc = _xmlParseFile(fname);
    free(fname);
    if ( doc==NULL )
return;

    plist = _xmlDocGetRootElement(doc);
    dict = FindNode(plist->children,"dict");
    if ( _xmlStrcmp(plist->name,(const xmlChar *) "plist")!=0 || dict==NULL ) {
	LogError(_("Expected property list file"));
	_xmlFreeDoc(doc);
return;
    }
    for ( keys=dict->children; keys!=NULL; keys=keys->next ) {
	for ( value = keys->next; value!=NULL && _xmlStrcmp(value->name,(const xmlChar *) "text")==0;
		value = value->next );
	if ( value==NULL )
    break;
	if ( _xmlStrcmp(keys->name,(const xmlChar *) "key")==0 ) {
	    keyname = (char *) _xmlNodeListGetString(doc,keys->children,true);
	    sc = SFGetChar(sf,-1,keyname);
	    free(keyname);
	    if ( sc==NULL )
	continue;
	    keys = value;
	    for ( subkeys = value->children; subkeys!=NULL; subkeys = subkeys->next ) {
		for ( value = subkeys->next; value!=NULL && _xmlStrcmp(value->name,(const xmlChar *) "text")==0;
			value = value->next );
		if ( value==NULL )
	    break;
		if ( _xmlStrcmp(subkeys->name,(const xmlChar *) "key")==0 ) {
		    keyname = (char *) _xmlNodeListGetString(doc,subkeys->children,true);
		    ssc = SFGetChar(sf,-1,keyname);
		    free(keyname);
		    if ( ssc==NULL )
		continue;
		    subkeys = value;
		    valname = (char *) _xmlNodeListGetString(doc,value->children,true);
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
    _xmlFreeDoc(doc);
}

SplineFont *SFReadUFO(char *basedir, int flags) {
    xmlNodePtr plist, dict, keys, value;
    xmlDocPtr doc;
    SplineFont *sf;
    xmlChar *keyname, *valname;
    char *temp, *glyphlist, *glyphdir;
    char *oldloc, *end;
    int as = -1, ds= -1, em= -1;
    double ia = 0;
    char *family=NULL,*fontname=NULL,*fullname=NULL,*weight=NULL,*copyright=NULL,*curve=NULL;

    if ( !libxml_init_base()) {
	LogError( _("Can't find libxml2.\n") );
return( NULL );
    }

    glyphdir = buildname(basedir,"glyphs");
    glyphlist = buildname(glyphdir,"contents.plist");
    if ( !GFileExists(glyphlist)) {
	LogError( "No glyphs directory or no contents file\n" );
	free(glyphlist);
return( NULL );
    }
    free(glyphlist);

    temp = buildname(basedir,"fontinfo.plist");
    doc = _xmlParseFile(temp);
    free(temp);
    if ( doc==NULL ) {
	/* Can I get an error message from libxml? */
return( NULL );
    }
    plist = _xmlDocGetRootElement(doc);
    dict = FindNode(plist->children,"dict");
    if ( _xmlStrcmp(plist->name,(const xmlChar *) "plist")!=0 || dict==NULL ) {
	LogError(_("Expected property list file"));
	_xmlFreeDoc(doc);
return( NULL );
    }

    oldloc = setlocale(LC_NUMERIC,"C");
    for ( keys=dict->children; keys!=NULL; keys=keys->next ) {
	for ( value = keys->next; value!=NULL && _xmlStrcmp(value->name,(const xmlChar *) "text")==0;
		value = value->next );
	if ( value==NULL )
    break;
	if ( _xmlStrcmp(keys->name,(const xmlChar *) "key")==0 ) {
	    keyname = _xmlNodeListGetString(doc,keys->children,true);
	    valname = _xmlNodeListGetString(doc,value->children,true);
	    keys = value;
	    if ( _xmlStrcmp(keyname,(xmlChar *) "familyName")==0 )
		family = (char *) valname;
	    else if ( _xmlStrcmp(keyname,(xmlChar *) "fullName")==0 )
		fullname = (char *) valname;
	    else if ( _xmlStrcmp(keyname,(xmlChar *) "fontName")==0 )
		fontname = (char *) valname;
	    else if ( _xmlStrcmp(keyname,(xmlChar *) "weightName")==0 )
		weight = (char *) valname;
	    else if ( _xmlStrcmp(keyname,(xmlChar *) "copyright")==0 )
		copyright = (char *) valname;
	    else if ( _xmlStrcmp(keyname,(xmlChar *) "curveType")==0 )
		curve = (char *) valname;
	    else if ( _xmlStrcmp(keyname,(xmlChar *) "unitsPerEm")==0 ) {
		em = strtol((char *) valname,&end,10);
		if ( *end!='\0' ) em = -1;
	    } else if ( _xmlStrcmp(keyname,(xmlChar *) "ascender")==0 ) {
		as = strtol((char *) valname,&end,10);
		if ( *end!='\0' ) as = -1;
	    } else if ( _xmlStrcmp(keyname,(xmlChar *) "descender")==0 ) {
		ds = -strtol((char *) valname,&end,10);
		if ( *end!='\0' ) ds = -1;
	    } else if ( _xmlStrcmp(keyname,(xmlChar *) "italicAngle")==0 ) {
		ia = strtod((char *) valname,&end);
		if ( *end!='\0' ) ia = 0;
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
	LogError( _("This font does not specify unitsPerEm\n") );
	_xmlFreeDoc(doc);
	setlocale(LC_NUMERIC,oldloc);
return( NULL );
    }
    sf = SplineFontEmpty();
    sf->familyname = family;
    sf->fontname = fontname;
    sf->fullname = fullname;
    sf->weight = weight;
    sf->copyright = copyright;
    sf->ascent = as; sf->descent = ds;
    sf->layers[ly_fore].order2 = false;
    sf->italicangle = ia;
    if ( curve!=NULL && strmatch(curve,"Quadratic")==0 )
	sf->layers[ly_fore].order2 = sf->layers[ly_back].order2 = sf->grid.order2 = true;
    if ( sf->fontname==NULL )
	sf->fontname = "Untitled";
    if ( sf->familyname==NULL )
	sf->familyname = copy(sf->fontname);
    if ( sf->fullname==NULL )
	sf->fullname = copy(sf->fontname);
    if ( sf->weight==NULL )
	sf->weight = copy("Medium");
    _xmlFreeDoc(doc);

    UFOLoadGlyphs(sf,glyphdir);

    UFOHandleKern(sf,basedir,0);
    UFOHandleKern(sf,basedir,1);

    sf->layers[ly_fore].order2 = sf->layers[ly_back].order2 = sf->grid.order2 =
	    SFFindOrder(sf);
    SFSetOrder(sf,sf->layers[ly_fore].order2);

    sf->map = EncMapFromEncoding(sf,FindOrMakeEncoding("Unicode"));

#ifndef _NO_PYTHON
    temp = buildname(basedir,"lib.plist");
    doc = NULL;
    if ( GFileExists(temp))
	doc = _xmlParseFile(temp);
    free(temp);
    if ( doc!=NULL ) {
	plist = _xmlDocGetRootElement(doc);
	dict = NULL;
	if ( plist!=NULL )
	    dict = FindNode(plist->children,"dict");
	if ( plist==NULL ||
		_xmlStrcmp(plist->name,(const xmlChar *) "plist")!=0 ||
		dict==NULL ) {
	    LogError(_("Expected property list file"));
	} else {
	    sf->python_persistent = LibToPython(doc,dict);
	}
	_xmlFreeDoc(doc);
    }
#endif
    setlocale(LC_NUMERIC,oldloc);
return( sf );
}

SplineSet *SplinePointListInterpretGlif(char *filename,char *memory, int memlen,
	int em_size,int ascent,int is_stroked) {
    xmlDocPtr doc;
    char *oldloc;
    SplineChar *sc;
    SplineSet *ss;

    if ( !libxml_init_base()) {
	LogError( _("Can't find libxml2.\n") );
return( NULL );
    }
    if ( filename!=NULL )
	doc = _xmlParseFile(filename);
    else
	doc = _xmlParseMemory(memory,memlen);
    if ( doc==NULL )
return( NULL );

    oldloc = setlocale(LC_NUMERIC,"C");
    sc = _UFOLoadGlyph(doc,filename);
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
#endif
