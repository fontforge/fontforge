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
#include <time.h>
#ifndef _NO_PYTHON
# include "ffpython.h"
#endif

/* The UFO (Unified Font Object) format, http://unifiedfontobject.org/ */
/* Obsolete: http://just.letterror.com/ltrwiki/UnifiedFontObject */
/* is a directory containing a bunch of (mac style) property lists and another*/
/* directory containing glif files (and contents.plist). */

/* Property lists contain one <dict> element which contains <key> elements */
/*  each followed by an <integer>, <real>, <true/>, <false/> or <string> element, */
/*  or another <dict> */

/* UFO format 2.0 includes an adobe feature file "feature.fea" and slightly */
/*  different/more tags in fontinfo.plist */

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
		if ( !PyBytes_Check(key))		/* Keys need not be strings */
	    continue;
		str = PyBytes_AsString(key);
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
    else if ( PyBytes_Check(value)) {		/* Must precede the sequence check */
	char *str = PyBytes_AsString(value);
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
static int refcomp(const void *_r1, const void *_r2) {
    const RefChar *ref1 = *(RefChar * const *)_r1;
    const RefChar *ref2 = *(RefChar * const *)_r2;
return( strcmp( ref1->sc->name, ref2->sc->name) );
}

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
	fprintf( glif, "  <unicode hex=\"%04X\"/>\n", sc->unicodeenc );
    for ( altuni = sc->altuni; altuni!=NULL; altuni = altuni->next )
	if ( altuni->vs==-1 && altuni->fid==0 )
	    fprintf( glif, "  <unicode hex=\"%04x\"/>\n", altuni->unienc );

    if ( sc->layers[layer].refs!=NULL || sc->layers[layer].splines!=NULL ) {
	fprintf( glif, "  <outline>\n" );
	/* RoboFab outputs components in alphabetic (case sensitive) order */
	/*  I've been asked to do that too */
	if ( sc->layers[layer].refs!=NULL ) {
	    RefChar **refs;
	    int i, cnt;
	    for ( cnt=0, ref = sc->layers[layer].refs; ref!=NULL; ref=ref->next ) if ( SCWorthOutputting(ref->sc))
		++cnt;
	    refs = galloc(cnt*sizeof(RefChar *));
	    for ( cnt=0, ref = sc->layers[layer].refs; ref!=NULL; ref=ref->next ) if ( SCWorthOutputting(ref->sc))
		refs[cnt++] = ref;
	    if ( cnt>1 )
		qsort(refs,cnt,sizeof(RefChar *),refcomp);
	    for ( i=0; i<cnt; ++i ) {
		ref = refs[i];
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
	    free(refs);
	}
	for ( spl=sc->layers[layer].splines; spl!=NULL; spl=spl->next ) {
	    fprintf( glif, "    <contour>\n" );
	    for ( sp=spl->first; sp!=NULL; ) {
		/* Undocumented fact: If a contour contains a series of off-curve points with no on-curve then treat as quadratic even if no qcurve */
		if ( !isquad || /*sp==spl->first ||*/ !SPInterpolate(sp) )
		    fprintf( glif, "      <point x=\"%g\" y=\"%g\" type=\"%s\"%s/>\n",
			    (double) sp->me.x, (double) sp->me.y,
			    sp->prev==NULL        ? "move"   :
			    sp->prev->knownlinear ? "line"   :
			    isquad 		      ? "qcurve" :
						    "curve",
			    sp->pointtype!=pt_corner?" smooth=\"yes\"":"" );
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

static void PListOutputInteger(FILE *plist, char *key, int value) {
    fprintf( plist, "\t<key>%s</key>\n", key );
    fprintf( plist, "\t<integer>%d</integer>\n", value );
}

static void PListOutputReal(FILE *plist, char *key, double value) {
    fprintf( plist, "\t<key>%s</key>\n", key );
    fprintf( plist, "\t<real>%g</real>\n", value );
}

static void PListOutputBoolean(FILE *plist, char *key, int value) {
    fprintf( plist, "\t<key>%s</key>\n", key );
    fprintf( plist, value ? "\t<true/>\n" : "\t<false/>\n" );
}

static void PListOutputDate(FILE *plist, char *key, time_t timestamp) {
    struct tm *tm = gmtime(&timestamp);

    fprintf( plist, "\t<key>%s</key>\n", key );
    fprintf( plist, "\t<string>%4d/%02d/%02d %02d:%02d:%02d</string>\n",
	    tm->tm_year+1900, tm->tm_mon,
	    tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec );
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

static void PListOutputNameString(FILE *plist, char *key, SplineFont *sf, int strid) {
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
    if ( value!=NULL )
	PListOutputString(plist,key,value);
    free(freeme);
}

static void PListOutputIntArray(FILE *plist, char *key, char *entries, int len) {
    int i;

    fprintf( plist, "\t<key>%s</key>\n", key );
    fprintf( plist, "\t<array>\n" );
    for ( i=0; i<len; ++i )
	fprintf( plist, "\t\t<integer>%d</integer>\n", entries[i] );
    fprintf( plist, "\t</array>\n" );
}

static void PListOutputPrivateArray(FILE *plist, char *key, struct psdict *private) {
    char *value;
    int skipping;

    if ( private==NULL )
return;
    value = PSDictHasEntry(private,key);
    if ( value==NULL )
return;

    while ( *value==' ' || *value=='[' ) ++value;

    fprintf( plist, "\t<key>postscript%s</key>\n", key );
    fprintf( plist, "\t<array>\n" );
    forever {
	fprintf( plist, "\t\t<integer>" );
	skipping=0;
	while ( *value!=']' && *value!='\0' && *value!=' ' ) {
	    if ( *value=='.' || skipping ) {
		skipping=true;
		++value;
	    } else
		fputc(*value++,plist);
	}
	fprintf( plist, "</integer>\n" );
	while ( *value==' ' ) ++value;
	if ( *value==']' || *value=='\0' )
    break;
    }
    fprintf( plist, "\t</array>\n" );
}

static void PListOutputPrivateThing(FILE *plist, char *key, struct psdict *private, char *type) {
    char *value;

    if ( private==NULL )
return;
    value = PSDictHasEntry(private,key);
    if ( value==NULL )
return;

    while ( *value==' ' || *value=='[' ) ++value;

    fprintf( plist, "\t<key>postscript%s</key>\n", key );
    fprintf( plist, "\t<%s>%s</%s>\n", type, value, type );
}

static int UFOOutputMetaInfo(char *basedir,SplineFont *sf) {
    FILE *plist = PListCreate( basedir, "metainfo.plist" );

    if ( plist==NULL )
return( false );
    PListOutputString(plist,"creator","net.SourceForge.FontForge");
#ifdef Version_1
    PListOutputInteger(plist,"formatVersion",1);
#else
    PListOutputInteger(plist,"formatVersion",2);
#endif
return( PListOutputTrailer(plist));
}

static int UFOOutputFontInfo(char *basedir,SplineFont *sf, int layer) {
    FILE *plist = PListCreate( basedir, "fontinfo.plist" );
    DBounds bb;
    double test;

    if ( plist==NULL )
return( false );
/* Same keys in both formats */
    PListOutputString(plist,"familyName",sf->familyname);
    PListOutputString(plist,"styleName",SFGetModifiers(sf));
    PListOutputString(plist,"copyright",sf->copyright);
    PListOutputNameString(plist,"trademark",sf,ttf_trademark);
    PListOutputInteger(plist,"unitsPerEm",sf->ascent+sf->descent);
    test = SFXHeight(sf,layer,true);
    if ( test>0 )
	PListOutputInteger(plist,"xHeight",(int) rint(test));
    test = SFCapHeight(sf,layer,true);
    if ( test>0 )
	PListOutputInteger(plist,"capHeight",(int) rint(test));
    if ( sf->ufo_ascent==0 )
	PListOutputInteger(plist,"ascender",sf->ascent);
    else if ( sf->ufo_ascent==floor(sf->ufo_ascent))
	PListOutputInteger(plist,"ascender",sf->ufo_ascent);
    else
	PListOutputReal(plist,"ascender",sf->ufo_ascent);
    if ( sf->ufo_descent==0 )
	PListOutputInteger(plist,"descender",-sf->descent);
    else if ( sf->ufo_descent==floor(sf->ufo_descent))
	PListOutputInteger(plist,"descender",sf->ufo_descent);
    else
	PListOutputReal(plist,"descender",sf->ufo_descent);
    PListOutputReal(plist,"italicAngle",sf->italicangle);
#ifdef Version_1
    PListOutputString(plist,"fullName",sf->fullname);
    PListOutputString(plist,"fontName",sf->fontname);
    /* FontForge does not maintain a menuname except possibly in the ttfnames section where there are many different languages of it */
    PListOutputString(plist,"weightName",sf->weight);
    /* No longer in the spec. Was it ever? Did I get this wrong? */
    /* PListOutputString(plist,"curveType",sf->layers[layer].order2 ? "Quadratic" : "Cubic");*/
#else
    PListOutputString(plist,"note",sf->comments);
    PListOutputDate(plist,"openTypeHeadCreated",sf->creationtime);
    SplineFontFindBounds(sf,&bb);
    if ( sf->pfminfo.hheadset ) {
	if ( sf->pfminfo.hheadascent_add )
	    PListOutputInteger(plist,"openTypeHheaAscender",bb.maxy+sf->pfminfo.hhead_ascent);
	else
	    PListOutputInteger(plist,"openTypeHheaAscender",sf->pfminfo.hhead_ascent);
	if ( sf->pfminfo.hheaddescent_add )
	    PListOutputInteger(plist,"openTypeHheaDescender",bb.miny+sf->pfminfo.hhead_descent);
	else
	    PListOutputInteger(plist,"openTypeHheaDescender",sf->pfminfo.hhead_descent);
	PListOutputInteger(plist,"openTypeHheaLineGap",sf->pfminfo.linegap);
    }
    PListOutputNameString(plist,"openTypeNameDesigner",sf,ttf_designer);
    PListOutputNameString(plist,"openTypeNameDesignerURL",sf,ttf_designerurl);
    PListOutputNameString(plist,"openTypeNameManufacturer",sf,ttf_manufacturer);
    PListOutputNameString(plist,"openTypeNameManufacturerURL",sf,ttf_venderurl);
    PListOutputNameString(plist,"openTypeNameLicense",sf,ttf_license);
    PListOutputNameString(plist,"openTypeNameLicenseURL",sf,ttf_licenseurl);
    PListOutputNameString(plist,"openTypeNameVersion",sf,ttf_version);
    PListOutputNameString(plist,"openTypeNameUniqueID",sf,ttf_uniqueid);
    PListOutputNameString(plist,"openTypeNameDescription",sf,ttf_descriptor);
    PListOutputNameString(plist,"openTypeNamePreferedFamilyName",sf,ttf_preffamilyname);
    PListOutputNameString(plist,"openTypeNamePreferedSubfamilyName",sf,ttf_prefmodifiers);
    PListOutputNameString(plist,"openTypeNameCompatibleFullName",sf,ttf_compatfull);
    PListOutputNameString(plist,"openTypeNameSampleText",sf,ttf_sampletext);
    PListOutputNameString(plist,"openTypeWWSFamilyName",sf,ttf_wwsfamily);
    PListOutputNameString(plist,"openTypeWWSSubfamilyName",sf,ttf_wwssubfamily);
    if ( sf->pfminfo.panose_set )
	PListOutputIntArray(plist,"openTypeOS2Panose",sf->pfminfo.panose,10);
    if ( sf->pfminfo.pfmset ) {
	char vendor[8], fc[2];
	PListOutputInteger(plist,"openTypeOS2WidthClass",sf->pfminfo.width);
	PListOutputInteger(plist,"openTypeOS2WeightClass",sf->pfminfo.weight);
	memcpy(vendor,sf->pfminfo.os2_vendor,4);
	vendor[4] = 0;
	PListOutputString(plist,"openTypeOS2VendorID",vendor);
	fc[0] = sf->pfminfo.os2_family_class>>8; fc[1] = sf->pfminfo.os2_family_class&0xff;
	PListOutputIntArray(plist,"openTypeOS2FamilyClass",fc,2);
	{
	    int fscnt,i;
	    char fstype[16];
	    for ( i=fscnt=0; i<16; ++i )
		if ( sf->pfminfo.fstype&(1<<i) )
		    fstype[fscnt++] = i;
	    if ( fscnt!=0 )
		PListOutputIntArray(plist,"openTypeOS2Type",fstype,fscnt);
	}
	if ( sf->pfminfo.typoascent_add )
	    PListOutputInteger(plist,"openTypeOS2TypoAscender",sf->ascent+sf->pfminfo.os2_typoascent);
	else
	    PListOutputInteger(plist,"openTypeOS2TypoAscender",sf->pfminfo.os2_typoascent);
	if ( sf->pfminfo.typodescent_add )
	    PListOutputInteger(plist,"openTypeOS2TypoDescender",sf->descent+sf->pfminfo.os2_typodescent);
	else
	    PListOutputInteger(plist,"openTypeOS2TypoDescender",sf->pfminfo.os2_typodescent);
	PListOutputInteger(plist,"openTypeOS2TypoLineGap",sf->pfminfo.os2_typolinegap);
	if ( sf->pfminfo.winascent_add )
	    PListOutputInteger(plist,"openTypeOS2WinAscent",bb.maxy+sf->pfminfo.os2_winascent);
	else
	    PListOutputInteger(plist,"openTypeOS2WinAscent",sf->pfminfo.os2_winascent);
	if ( sf->pfminfo.windescent_add )
	    PListOutputInteger(plist,"openTypeOS2WinDescent",bb.miny+sf->pfminfo.os2_windescent);
	else
	    PListOutputInteger(plist,"openTypeOS2WinDescent",sf->pfminfo.os2_windescent);
    }
    if ( sf->pfminfo.subsuper_set ) {
	PListOutputInteger(plist,"openTypeOS2SubscriptXSize",sf->pfminfo.os2_subxsize);
	PListOutputInteger(plist,"openTypeOS2SubscriptYSize",sf->pfminfo.os2_subysize);
	PListOutputInteger(plist,"openTypeOS2SubscriptXOffset",sf->pfminfo.os2_subxoff);
	PListOutputInteger(plist,"openTypeOS2SubscriptYOffset",sf->pfminfo.os2_subyoff);
	PListOutputInteger(plist,"openTypeOS2SuperscriptXSize",sf->pfminfo.os2_supxsize);
	PListOutputInteger(plist,"openTypeOS2SuperscriptYSize",sf->pfminfo.os2_supysize);
	PListOutputInteger(plist,"openTypeOS2SuperscriptXOffset",sf->pfminfo.os2_supxoff);
	PListOutputInteger(plist,"openTypeOS2SuperscriptYOffset",sf->pfminfo.os2_supyoff);
	PListOutputInteger(plist,"openTypeOS2StrikeoutSize",sf->pfminfo.os2_strikeysize);
	PListOutputInteger(plist,"openTypeOS2StrikeoutPosition",sf->pfminfo.os2_strikeypos);
    }
    if ( sf->pfminfo.vheadset )
	PListOutputInteger(plist,"openTypeVheaTypoLineGap",sf->pfminfo.vlinegap);
    if ( sf->pfminfo.hasunicoderanges ) {
	char ranges[128];
	int i, j, c = 0;

	for ( i = 0; i<4; i++ )
	    for ( j = 0; j<32; j++ )
		if ( sf->pfminfo.unicoderanges[i] & (1 << j) )
		    ranges[c++] = i*32+j;
	if ( c!=0 )
	    PListOutputIntArray(plist,"openTypeOS2UnicodeRanges",ranges,c);
    }
    if ( sf->pfminfo.hascodepages ) {
	char pages[64];
	int i, j, c = 0;

	for ( i = 0; i<2; i++)
	    for ( j=0; j<32; j++ )
		if ( sf->pfminfo.codepages[i] & (1 << j) )
		    pages[c++] = i*32+j;
	if ( c!=0 )
	    PListOutputIntArray(plist,"openTypeOS2CodePageRanges",pages,c);
    }
    PListOutputString(plist,"postscriptFontName",sf->fontname);
    PListOutputString(plist,"postscriptFullName",sf->fullname);
    PListOutputString(plist,"postscriptWeightName",sf->weight);
    /* Spec defines a "postscriptSlantAngle" but I don't think postscript does*/
    /* PS does define an italicAngle, but presumably that's the general italic*/
    /* angle we output earlier */
    /* UniqueID is obsolete */
    PListOutputInteger(plist,"postscriptUnderlineThickness",sf->uwidth);
    PListOutputInteger(plist,"postscriptUnderlinePosition",sf->upos);
    if ( sf->private!=NULL ) {
	char *pt;
	PListOutputPrivateArray(plist, "BlueValues", sf->private);
	PListOutputPrivateArray(plist, "OtherBlues", sf->private);
	PListOutputPrivateArray(plist, "FamilyBlues", sf->private);
	PListOutputPrivateArray(plist, "FamilyOtherBlues", sf->private);
	PListOutputPrivateArray(plist, "StemSnapH", sf->private);
	PListOutputPrivateArray(plist, "StemSnapV", sf->private);
	PListOutputPrivateThing(plist, "BlueFuzz", sf->private, "integer");
	PListOutputPrivateThing(plist, "BlueShift", sf->private, "integer");
	PListOutputPrivateThing(plist, "BlueScale", sf->private, "real");
	if ( (pt=PSDictHasEntry(sf->private,"ForceBold"))!=NULL &&
		strstr(pt,"true")!=NULL )
	    PListOutputBoolean(plist, "postscriptForceBold", true );
    }
    if ( sf->fondname!=NULL )
    PListOutputString(plist,"macintoshFONDName",sf->fondname);
#endif
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

#ifndef Version_1
static int UFOOutputFeatures(char *basedir,SplineFont *sf) {
    char *fname = buildname(basedir,"feature.fea");
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
#endif

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
    GFileMkDir( basedir );

    err  = !UFOOutputMetaInfo(basedir,sf);
    err |= !UFOOutputFontInfo(basedir,sf,layer);
    err |= !UFOOutputGroups(basedir,sf);
    err |= !UFOOutputKerning(basedir,sf);
    err |= !UFOOutputVKerning(basedir,sf);
    err |= !UFOOutputLib(basedir,sf);
#ifndef Version_1
    err |= !UFOOutputFeatures(basedir,sf);
#endif

    if ( err )
return( false );

    glyphdir = buildname(basedir,"glyphs");
    GFileMkDir( glyphdir );

    plist = PListCreate(glyphdir,"contents.plist");
    if ( plist==NULL ) {
	free(glyphdir);
return( false );
    }

    for ( i=0; i<sf->glyphcnt; ++i ) if ( SCWorthOutputting(sc=sf->glyphs[i]) ) {
	char *start, *gstart;
	gstart = gfname = galloc(2*strlen(sc->name)+20);
	start = sc->name;
	if ( *start=='.' ) {
	    *gstart++ = '_';
	    ++start;
	}
	while ( *start ) {
	    /* Now the spec has a very complicated algorithm for producing a */
	    /*  filename, dividing the glyph name into chunks at every period*/
	    /*  and then again at every underscore, and then adding an under-*/
	    /*  score at the end of a chunk if the chunk begins with a capital*/
	    /* BUT... */
	    /* That's not what RoboFAB does. It simply adds an underscore after*/
	    /*  every capital letter. Much easier. And since people have */
	    /*  complained that I follow the spec, let's not. */
	    *gstart++ = *start;
	    if ( isupper( *start++ ))
	        *gstart++ = '_';
	}
#ifdef __VMS
	*gstart ='\0';
	for ( gstart=gfname; *gstart; ++gstart ) {
	    if ( *gstart=='.' )
		*gstart = '@';		/* VMS only allows one "." in a filename */
	}
#endif
	strcpy(gstart,".glif");
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

#ifdef _NO_LIBXML
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
	    if ( *cpt=='@' )		/* VMS doesn't let me have two "." in a filename so I use @ instead when a "." is called for */
		*cpt = '.';
	    if ( *cpt!='_' )
		*pt++ = *cpt;
	    else if ( islower(*name))
		*name = toupper(*name);
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

    if ( _xmlStrcmp(value->name,(const xmlChar *) "array")!=0 )
return;
    pt = space; end = pt+sizeof(space)-10;
    *pt++ = '[';
    for ( kid = value->children; kid!=NULL; kid=kid->next ) {
	if ( _xmlStrcmp(kid->name,(const xmlChar *) "integer")==0 ||
		_xmlStrcmp(kid->name,(const xmlChar *) "real")==0 ) {
	    char *valName = (char *) _xmlNodeListGetString(doc,kid->children,true);
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

    if ( _xmlStrcmp(value->name,(const xmlChar *) "array")!=0 )
return;
    i=0;
    for ( kid = value->children; kid!=NULL; kid=kid->next ) {
	if ( _xmlStrcmp(kid->name,(const xmlChar *) "integer")==0 ) {
	    char *valName = (char *) _xmlNodeListGetString(doc,kid->children,true);
	    if ( i<cnt )
		array[i++] = strtol(valName,NULL,10);
	    free(valName);
	}
    }
}

static long UFOGetBits(xmlDocPtr doc,xmlNodePtr value) {
    xmlNodePtr kid;
    long mask=0;

    if ( _xmlStrcmp(value->name,(const xmlChar *) "array")!=0 )
return( 0 );
    for ( kid = value->children; kid!=NULL; kid=kid->next ) {
	if ( _xmlStrcmp(kid->name,(const xmlChar *) "integer")==0 ) {
	    char *valName = (char *) _xmlNodeListGetString(doc,kid->children,true);
	    mask |= 1<<strtol(valName,NULL,10);
	    free(valName);
	}
    }
return( mask );
}

static void UFOGetBitArray(xmlDocPtr doc,xmlNodePtr value,uint32 *res,int len) {
    xmlNodePtr kid;
    int index;

    if ( _xmlStrcmp(value->name,(const xmlChar *) "array")!=0 )
return;
    for ( kid = value->children; kid!=NULL; kid=kid->next ) {
	if ( _xmlStrcmp(kid->name,(const xmlChar *) "integer")==0 ) {
	    char *valName = (char *) _xmlNodeListGetString(doc,kid->children,true);
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
    char oldloc[24], *end;
    int as = -1, ds= -1, em= -1;

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

    sf = SplineFontEmpty();
    strcpy( oldloc,setlocale(LC_NUMERIC,NULL) );
    setlocale(LC_NUMERIC,"C");
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
		sf->familyname = (char *) valname;
	    else if ( _xmlStrcmp(keyname,(xmlChar *) "styleName")==0 )
		stylename = (char *) valname;
	    else if ( _xmlStrcmp(keyname,(xmlChar *) "fullName")==0 ||
		    _xmlStrcmp(keyname,(xmlChar *) "postscriptFullName")==0 )
		sf->fullname = (char *) valname;
	    else if ( _xmlStrcmp(keyname,(xmlChar *) "fontName")==0 ||
		    _xmlStrcmp(keyname,(xmlChar *) "postscriptFontName")==0 )
		sf->fontname = (char *) valname;
	    else if ( _xmlStrcmp(keyname,(xmlChar *) "weightName")==0 ||
		    _xmlStrcmp(keyname,(xmlChar *) "postscriptWeightName")==0 )
		sf->weight = (char *) valname;
	    else if ( _xmlStrcmp(keyname,(xmlChar *) "note")==0 )
		sf->comments = (char *) valname;
	    else if ( _xmlStrcmp(keyname,(xmlChar *) "copyright")==0 )
		sf->copyright = (char *) valname;
	    else if ( _xmlStrcmp(keyname,(xmlChar *) "trademark")==0 )
		UFOAddName(sf,(char *) valname,ttf_trademark);
	    else if ( strncmp((char *) keyname,"openTypeName",12)==0 ) {
		if ( _xmlStrcmp(keyname+12,(xmlChar *) "Designer")==0 )
		    UFOAddName(sf,(char *) valname,ttf_designer);
		else if ( _xmlStrcmp(keyname+12,(xmlChar *) "DesignerURL")==0 )
		    UFOAddName(sf,(char *) valname,ttf_designerurl);
		else if ( _xmlStrcmp(keyname+12,(xmlChar *) "Manufacturer")==0 )
		    UFOAddName(sf,(char *) valname,ttf_manufacturer);
		else if ( _xmlStrcmp(keyname+12,(xmlChar *) "ManufacturerURL")==0 )
		    UFOAddName(sf,(char *) valname,ttf_venderurl);
		else if ( _xmlStrcmp(keyname+12,(xmlChar *) "License")==0 )
		    UFOAddName(sf,(char *) valname,ttf_license);
		else if ( _xmlStrcmp(keyname+12,(xmlChar *) "LicenseURL")==0 )
		    UFOAddName(sf,(char *) valname,ttf_licenseurl);
		else if ( _xmlStrcmp(keyname+12,(xmlChar *) "Version")==0 )
		    UFOAddName(sf,(char *) valname,ttf_version);
		else if ( _xmlStrcmp(keyname+12,(xmlChar *) "UniqueID")==0 )
		    UFOAddName(sf,(char *) valname,ttf_uniqueid);
		else if ( _xmlStrcmp(keyname+12,(xmlChar *) "Description")==0 )
		    UFOAddName(sf,(char *) valname,ttf_descriptor);
		else if ( _xmlStrcmp(keyname+12,(xmlChar *) "PreferedFamilyName")==0 )
		    UFOAddName(sf,(char *) valname,ttf_preffamilyname);
		else if ( _xmlStrcmp(keyname+12,(xmlChar *) "PreferedSubfamilyName")==0 )
		    UFOAddName(sf,(char *) valname,ttf_prefmodifiers);
		else if ( _xmlStrcmp(keyname+12,(xmlChar *) "CompatibleFullName")==0 )
		    UFOAddName(sf,(char *) valname,ttf_compatfull);
		else if ( _xmlStrcmp(keyname+12,(xmlChar *) "SampleText")==0 )
		    UFOAddName(sf,(char *) valname,ttf_sampletext);
		else if ( _xmlStrcmp(keyname+12,(xmlChar *) "WWSFamilyName")==0 )
		    UFOAddName(sf,(char *) valname,ttf_wwsfamily);
		else if ( _xmlStrcmp(keyname+12,(xmlChar *) "WWSSubfamilyName")==0 )
		    UFOAddName(sf,(char *) valname,ttf_wwssubfamily);
		else
		    free(valname);
	    } else if ( strncmp((char *) keyname, "openTypeHhea",12)==0 ) {
		if ( _xmlStrcmp(keyname+12,(xmlChar *) "Ascender")==0 ) {
		    sf->pfminfo.hhead_ascent = strtol((char *) valname,&end,10);
		    sf->pfminfo.hheadascent_add = false;
		} else if ( _xmlStrcmp(keyname+12,(xmlChar *) "Descender")==0 ) {
		    sf->pfminfo.hhead_descent = strtol((char *) valname,&end,10);
		    sf->pfminfo.hheaddescent_add = false;
		} else if ( _xmlStrcmp(keyname+12,(xmlChar *) "LineGap")==0 )
		    sf->pfminfo.linegap = strtol((char *) valname,&end,10);
		free(valname);
		sf->pfminfo.hheadset = true;
	    } else if ( strncmp((char *) keyname,"openTypeVhea",12)==0 ) {
		if ( _xmlStrcmp(keyname+12,(xmlChar *) "LineGap")==0 )
		    sf->pfminfo.vlinegap = strtol((char *) valname,&end,10);
		sf->pfminfo.vheadset = true;
		free(valname);
	    } else if ( strncmp((char *) keyname,"openTypeOS2",11)==0 ) {
		sf->pfminfo.pfmset = true;
		if ( _xmlStrcmp(keyname+11,(xmlChar *) "Panose")==0 ) {
		    UFOGetByteArray(sf->pfminfo.panose,sizeof(sf->pfminfo.panose),doc,value);
		    sf->pfminfo.panose_set = true;
		} else if ( _xmlStrcmp(keyname+11,(xmlChar *) "Type")==0 )
		    sf->pfminfo.fstype = UFOGetBits(doc,value);
		else if ( _xmlStrcmp(keyname+11,(xmlChar *) "FamilyClass")==0 ) {
		    char fc[2];
		    UFOGetByteArray(fc,sizeof(fc),doc,value);
		    sf->pfminfo.os2_family_class = (fc[0]<<8)|fc[1];
		} else if ( _xmlStrcmp(keyname+11,(xmlChar *) "WidthClass")==0 )
		    sf->pfminfo.width = strtol((char *) valname,&end,10);
		else if ( _xmlStrcmp(keyname+11,(xmlChar *) "WeightClass")==0 )
		    sf->pfminfo.weight = strtol((char *) valname,&end,10);
		else if ( _xmlStrcmp(keyname+11,(xmlChar *) "VendorID")==0 ) {
		    char *temp = sf->pfminfo.os2_vendor + 3;
		    strncpy(sf->pfminfo.os2_vendor,valname,4);
		    while ( *temp == 0 && temp >= sf->pfminfo.os2_vendor ) *temp-- = ' ';
		} else if ( _xmlStrcmp(keyname+11,(xmlChar *) "TypoAscender")==0 ) {
		    sf->pfminfo.typoascent_add = false;
		    sf->pfminfo.os2_typoascent = strtol((char *) valname,&end,10);
		} else if ( _xmlStrcmp(keyname+11,(xmlChar *) "TypoDescender")==0 ) {
		    sf->pfminfo.typodescent_add = false;
		    sf->pfminfo.os2_typodescent = strtol((char *) valname,&end,10);
		} else if ( _xmlStrcmp(keyname+11,(xmlChar *) "TypoLineGap")==0 )
		    sf->pfminfo.os2_typolinegap = strtol((char *) valname,&end,10);
		else if ( _xmlStrcmp(keyname+11,(xmlChar *) "WinAscent")==0 ) {
		    sf->pfminfo.winascent_add = false;
		    sf->pfminfo.os2_winascent = strtol((char *) valname,&end,10);
		} else if ( _xmlStrcmp(keyname+11,(xmlChar *) "WinDescent")==0 ) {
		    sf->pfminfo.windescent_add = false;
		    sf->pfminfo.os2_windescent = strtol((char *) valname,&end,10);
		} else if ( strncmp((char *) keyname+11,"Subscript",9)==0 ) {
		    sf->pfminfo.subsuper_set = true;
		    if ( _xmlStrcmp(keyname+20,(xmlChar *) "XSize")==0 )
			sf->pfminfo.os2_subxsize = strtol((char *) valname,&end,10);
		    else if ( _xmlStrcmp(keyname+20,(xmlChar *) "YSize")==0 )
			sf->pfminfo.os2_subysize = strtol((char *) valname,&end,10);
		    else if ( _xmlStrcmp(keyname+20,(xmlChar *) "XOffset")==0 )
			sf->pfminfo.os2_subxoff = strtol((char *) valname,&end,10);
		    else if ( _xmlStrcmp(keyname+20,(xmlChar *) "YOffset")==0 )
			sf->pfminfo.os2_subyoff = strtol((char *) valname,&end,10);
		} else if ( strncmp((char *) keyname+11, "Superscript",11)==0 ) {
		    sf->pfminfo.subsuper_set = true;
		    if ( _xmlStrcmp(keyname+22,(xmlChar *) "XSize")==0 )
			sf->pfminfo.os2_supxsize = strtol((char *) valname,&end,10);
		    else if ( _xmlStrcmp(keyname+22,(xmlChar *) "YSize")==0 )
			sf->pfminfo.os2_supysize = strtol((char *) valname,&end,10);
		    else if ( _xmlStrcmp(keyname+22,(xmlChar *) "XOffset")==0 )
			sf->pfminfo.os2_supxoff = strtol((char *) valname,&end,10);
		    else if ( _xmlStrcmp(keyname+22,(xmlChar *) "YOffset")==0 )
			sf->pfminfo.os2_supyoff = strtol((char *) valname,&end,10);
		} else if ( strncmp((char *) keyname+11, "Strikeout",9)==0 ) {
		    sf->pfminfo.subsuper_set = true;
		    if ( _xmlStrcmp(keyname+20,(xmlChar *) "Size")==0 )
			sf->pfminfo.os2_strikeysize = strtol((char *) valname,&end,10);
		    else if ( _xmlStrcmp(keyname+20,(xmlChar *) "Position")==0 )
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
		if ( _xmlStrcmp(keyname+10,(xmlChar *) "UnderlineThickness")==0 )
		    sf->uwidth = strtol((char *) valname,&end,10);
		else if ( _xmlStrcmp(keyname+10,(xmlChar *) "UnderlinePosition")==0 )
		    sf->upos = strtol((char *) valname,&end,10);
		else if ( _xmlStrcmp(keyname+10,(xmlChar *) "BlueFuzz")==0 )
		    UFOAddPrivate(sf,"BlueFuzz",(char *) valname);
		else if ( _xmlStrcmp(keyname+10,(xmlChar *) "BlueScale")==0 )
		    UFOAddPrivate(sf,"BlueScale",(char *) valname);
		else if ( _xmlStrcmp(keyname+10,(xmlChar *) "BlueShift")==0 )
		    UFOAddPrivate(sf,"BlueShift",(char *) valname);
		else if ( _xmlStrcmp(keyname+10,(xmlChar *) "BlueValues")==0 )
		    UFOAddPrivateArray(sf,"BlueValues",doc,value);
		else if ( _xmlStrcmp(keyname+10,(xmlChar *) "OtherBlues")==0 )
		    UFOAddPrivateArray(sf,"OtherBlues",doc,value);
		else if ( _xmlStrcmp(keyname+10,(xmlChar *) "FamilyBlues")==0 )
		    UFOAddPrivateArray(sf,"FamilyBlues",doc,value);
		else if ( _xmlStrcmp(keyname+10,(xmlChar *) "FamilyOtherBlues")==0 )
		    UFOAddPrivateArray(sf,"FamilyOtherBlues",doc,value);
		else if ( _xmlStrcmp(keyname+10,(xmlChar *) "StemSnapH")==0 )
		    UFOAddPrivateArray(sf,"StemSnapH",doc,value);
		else if ( _xmlStrcmp(keyname+10,(xmlChar *) "StemSnapV")==0 )
		    UFOAddPrivateArray(sf,"StemSnapV",doc,value);
		else if ( _xmlStrcmp(keyname+10,(xmlChar *) "ForceBold")==0 )
		    UFOAddPrivate(sf,"ForceBold",(char *) value->name);
			/* value->name is either true or false */
		free(valname);
	    } else if ( strncmp((char *)keyname,"macintosh",9)==0 ) {
		if ( _xmlStrcmp(keyname+9,(xmlChar *) "FONDName")==0 )
		    sf->fondname = (char *) valname;
		else
		    free(valname);
	    } else if ( _xmlStrcmp(keyname,(xmlChar *) "unitsPerEm")==0 ) {
		em = strtol((char *) valname,&end,10);
		if ( *end!='\0' ) em = -1;
		free(valname);
	    } else if ( _xmlStrcmp(keyname,(xmlChar *) "ascender")==0 ) {
		as = strtod((char *) valname,&end);
		if ( *end!='\0' ) as = -1;
		else sf->ufo_ascent = as;
		free(valname);
	    } else if ( _xmlStrcmp(keyname,(xmlChar *) "descender")==0 ) {
		ds = -strtod((char *) valname,&end);
		if ( *end!='\0' ) ds = -1;
		else sf->ufo_descent = -ds;
		free(valname);
	    } else if ( _xmlStrcmp(keyname,(xmlChar *) "italicAngle")==0 ||
		    _xmlStrcmp(keyname,(xmlChar *) "postscriptSlantAngle")==0 ) {
		sf->italicangle = strtod((char *) valname,&end);
		if ( *end!='\0' ) sf->italicangle = 0;
		free(valname);
	    } else if ( _xmlStrcmp(keyname,(xmlChar *) "descender")==0 ) {
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
	LogError( _("This font does not specify unitsPerEm\n") );
	_xmlFreeDoc(doc);
	setlocale(LC_NUMERIC,oldloc);
	SplineFontFree(sf);
return( NULL );
    }
    sf->ascent = as; sf->descent = ds;
    if ( sf->fontname==NULL ) {
	if ( stylename!=NULL && sf->familyname!=NULL )
	    sf->fullname = strconcat3(sf->familyname,"-",stylename);
	else
	    sf->fontname = "Untitled";
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
	sf->weight = copy("Medium");
    if ( sf->version==NULL && sf->names!=NULL &&
	    sf->names->names[ttf_version]!=NULL &&
	    strncmp(sf->names->names[ttf_version],"Version ",8)==0 )
	sf->version = copy(sf->names->names[ttf_version]+8);
    _xmlFreeDoc(doc);

    UFOLoadGlyphs(sf,glyphdir);

    UFOHandleKern(sf,basedir,0);
    UFOHandleKern(sf,basedir,1);

    sf->layers[ly_fore].order2 = sf->layers[ly_back].order2 = sf->grid.order2 =
	    SFFindOrder(sf);
    SFSetOrder(sf,sf->layers[ly_fore].order2);

    sf->map = EncMapFromEncoding(sf,FindOrMakeEncoding("Unicode"));

    /* Might as well check for feature files even if version 1 */
    temp = buildname(basedir,"feature.fea");
    if ( GFileExists(temp))
	SFApplyFeatureFilename(sf,temp);
    free(temp);

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
    char oldloc[24];
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

    strcpy( oldloc,setlocale(LC_NUMERIC,NULL) );
    setlocale(LC_NUMERIC,"C");
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
