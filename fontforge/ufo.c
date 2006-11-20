/* Copyright (C) 2003-2006 by George Williams */
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
#include "pfaeditui.h"
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <locale.h>
#include <utype.h>
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
/* ****************************   GLIF Output    **************************** */
/* ************************************************************************** */
static int _GlifDump(FILE *glif,SplineChar *sc) {
    struct altuni *altuni;
    int isquad = sc->parent->order2;
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
	fprintf( glif, "  <unicode hex=\"%04x\"/>\n", altuni->unienc );

    if ( sc->layers[ly_fore].refs!=NULL || sc->layers[ly_fore].splines!=NULL ) {
	fprintf( glif, "  <outline>\n" );
	for ( ref = sc->layers[ly_fore].refs; ref!=NULL; ref=ref->next ) if ( SCWorthOutputting(ref->sc)) {
	    fprintf( glif, "    <component base=\"%s\"", ref->sc->name );
	    if ( ref->transform[0]!=1 )
		fprintf( glif, " xScale=\"%g\"", ref->transform[0] );
	    if ( ref->transform[3]!=1 )
		fprintf( glif, " yScale=\"%g\"", ref->transform[3] );
	    if ( ref->transform[1]!=0 )
		fprintf( glif, " xyScale=\"%g\"", ref->transform[1] );
	    if ( ref->transform[2]!=0 )
		fprintf( glif, " yxScale=\"%g\"", ref->transform[2] );
	    if ( ref->transform[4]!=0 )
		fprintf( glif, " xOffset=\"%g\"", ref->transform[4] );
	    if ( ref->transform[5]!=0 )
		fprintf( glif, " yOffset=\"%g\"", ref->transform[5] );
	    fprintf( glif, "/>\n" );
	}
	for ( spl=sc->layers[ly_fore].splines; spl!=NULL; spl=spl->next ) {
	    fprintf( glif, "    <contour>\n" );
	    for ( sp=spl->first; sp!=NULL; ) {
		fprintf( glif, "      <point x=\"%g\" y=\"%g\" type=\"%s\" smooth=\"%s\"/>\n",
			sp->me.x, sp->me.y,
			sp->prev==NULL        ? "move"   :
			sp->prev->knownlinear ? "line"   :
			isquad 		      ? "qcurve" :
						"curve",
			sp->pointtype!=pt_corner?"yes":"no" );
		if ( sp->next==NULL )
	    break;
		if ( !sp->next->knownlinear )
		    fprintf( glif, "      <point x=\"%g\" y=\"%g\"/>\n",
			    sp->nextcp.x, sp->nextcp.y );
		sp = sp->next->to;
		if ( !isquad && !sp->prev->knownlinear )
		    fprintf( glif, "      <point x=\"%g\" y=\"%g\"/>\n",
			    sp->prevcp.x, sp->prevcp.y );
		if ( sp==spl->first )
	    break;
	    }
	    fprintf( glif, "    </contour>\n" );
	}
	fprintf( glif, "  </outline>\n" );
    }
    fprintf( glif, "</glyph>\n" );
    err = ferror(glif);
    if ( fclose(glif))
	err = true;
return( !err );
}

static int GlifDump(char *glyphdir,char *gfname,SplineChar *sc) {
    char *gn = buildname(glyphdir,gfname);
    FILE *glif = fopen(gn,"w");
    int ret = _GlifDump(glif,sc);
    free(gn);
return( ret );
}

int _ExportGlif(FILE *glif,SplineChar *sc) {
return( _GlifDump(glif,sc));
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

static int UFOOutputFontInfo(char *basedir,SplineFont *sf) {
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
    PListOutputInteger(plist,"descender",sf->descent);
    PListOutputReal(plist,"italicAngle",sf->italicangle);
    PListOutputString(plist,"curveType",sf->order2 ? "Quadratic" : "Cubic");
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
    FILE *plist = PListCreate( basedir, "lib.plist" );

    if ( plist==NULL )
return( false );
    /* No idea what this is */
    /* Should I omit a file I don't use? Or leave it blank? */
return( PListOutputTrailer(plist));
}

int WriteUFOFont(char *basedir,SplineFont *sf,enum fontformat ff,int flags,
	EncMap *map) {
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
    err |= !UFOOutputFontInfo(basedir,sf);
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
	gfname = galloc(strlen(gfname)+20);
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
	err |= !GlifDump(glyphdir,gfname,sc);
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

static SplineChar *_UFOLoadGlyph(xmlDocPtr doc,char *glifname) {
    xmlNodePtr glyph, kids, contour, points;
    SplineChar *sc;
    xmlChar *format, *width, *height, *u;
    char *name;
    int uni;
    char *cpt;

    glyph = _xmlDocGetRootElement(doc);
    format = _xmlGetProp(glyph,"format");
    if ( _xmlStrcmp(glyph->name,(const xmlChar *) "glyph")!=0 ||
	    (format!=NULL && _xmlStrcmp(format,"1")!=0)) {
	LogError(_("Expected glyph file with format==1\n"));
	_xmlFreeDoc(doc);
	free(format);
return( NULL );
    }
    name = (char *) _xmlGetProp(glyph,"name");
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
    sc = SplineCharCreate();
    sc->name = name;

    for ( kids = glyph->children; kids!=NULL; kids=kids->next ) {
	if ( _xmlStrcmp(kids->name,(const xmlChar *) "advance")==0 ) {
	    width = _xmlGetProp(kids,"width");
	    height = _xmlGetProp(kids,"height");
	    if ( width!=NULL )
		sc->width = strtol((char *) width,NULL,10);
	    if ( height!=NULL )
		sc->vwidth = strtol((char *) height,NULL,10);
	    free(width); free(height);
	} else if ( _xmlStrcmp(kids->name,(const xmlChar *) "unicode")==0 ) {
	    u = _xmlGetProp(kids,"hex");
	    uni = strtol((char *) u,NULL,16);
	    if ( sc->unicodeenc == -1 )
		sc->unicodeenc = uni;
	    else
		AltUniAdd(sc,uni);
	} else if ( _xmlStrcmp(kids->name,(const xmlChar *) "outline")==0 ) {
	    for ( contour = kids->children; contour!=NULL; contour=contour->next ) {
		if ( _xmlStrcmp(contour->name,(const xmlChar *) "component")==0 ) {
		    char *base = (char *) _xmlGetProp(contour,"base"),
			*xs = (char *) _xmlGetProp(contour,"xScale"),
			*ys = (char *) _xmlGetProp(contour,"yScale"),
			*xys = (char *) _xmlGetProp(contour,"xyScale"),
			*yxs = (char *) _xmlGetProp(contour,"yxScale"),
			*xo = (char *) _xmlGetProp(contour,"xOffset"),
			*yo = (char *) _xmlGetProp(contour,"yOffset");
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
		    BasePoint pre[2], init[2];
		    int precnt=0, initcnt=0, open=0;

		    ss = chunkalloc(sizeof(SplineSet));
		    for ( points = contour->children; points!=NULL; points=points->next ) {
			char *xs, *ys, *type;
			int x,y;
			xs = (char *) _xmlGetProp(points,"x");
			ys = (char *) _xmlGetProp(points,"y");
			type = (char *) _xmlGetProp(points,"type");
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
			        memcpy(init,pre,sizeof(init));
			        initcnt = precnt;
			    } else if ( strcmp(type,"line")==0 ) {
				SplineMake(ss->last,sp,false);
			        ss->last = sp;
			    } else if ( strcmp(type,"curve")==0 ) {
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
				if ( precnt>=1 ) {
				    ss->last->nextcp = sp->prevcp = pre[0];
			            ss->last->nonextcp = sp->noprevcp = false;
				}
				SplineMake(ss->last,sp,true);
			        ss->last = sp;
			    }
			    precnt = 0;
			} else {
			    if ( precnt<2 ) {
				pre[precnt].x = x;
			        pre[precnt].y = y;
			        ++precnt;
			    }
			}
			free(xs); free(ys); free(type);
		    }
		    if ( !open ) {
			if ( initcnt==0 ) {
			    memcpy(init,pre,sizeof(init));
			    initcnt = precnt;
			}
			if ( initcnt==2 ) {
			    ss->last->nextcp = init[0];
			    ss->first->prevcp = init[1];
			    ss->last->nonextcp = ss->first->noprevcp = false;
			} else if ( initcnt == 1 ) {
			    ss->last->nextcp = ss->first->prevcp = init[0];
			    ss->last->nonextcp = ss->first->noprevcp = false;
			}
			SplineMake(ss->last,ss->first,initcnt==1);
			ss->last = ss->first;
		    }
		    ss->next = sc->layers[ly_fore].splines;
		    sc->layers[ly_fore].splines = ss;
		}
	    }
	}
    }
    _xmlFreeDoc(doc);
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
	    SCReinstanciateRefChar(sc,r);
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
#if !defined(FONTFORGE_CONFIG_NO_WINDOWING_UI)
    int tot;
#endif

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
#if !defined(FONTFORGE_CONFIG_NO_WINDOWING_UI)
    for ( tot=0, keys=dict->children; keys!=NULL; keys=keys->next ) {
	if ( _xmlStrcmp(keys->name,(const xmlChar *) "key")==0 )
	    ++tot;
    }
    gwwv_progress_change_total(tot);
#endif
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
#if !defined(FONTFORGE_CONFIG_NO_WINDOWING_UI)
	    gwwv_progress_next();
#endif
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
			kp->sli = SCDefaultSLI(sf,sc);
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
	    if ( _xmlStrcmp(keyname,"familyName")==0 )
		family = valname;
	    else if ( _xmlStrcmp(keyname,"fullName")==0 )
		fullname = valname;
	    else if ( _xmlStrcmp(keyname,"fontName")==0 )
		fontname = valname;
	    else if ( _xmlStrcmp(keyname,"weightName")==0 )
		weight = valname;
	    else if ( _xmlStrcmp(keyname,"copyright")==0 )
		copyright = valname;
	    else if ( _xmlStrcmp(keyname,"curveType")==0 )
		curve = valname;
	    else if ( _xmlStrcmp(keyname,"unitsPerEm")==0 ) {
		em = strtol(valname,&end,10);
		if ( *end!='\0' ) em = -1;
	    } else if ( _xmlStrcmp(keyname,"ascender")==0 ) {
		as = strtol(valname,&end,10);
		if ( *end!='\0' ) as = -1;
	    } else if ( _xmlStrcmp(keyname,"descender")==0 ) {
		ds = strtol(valname,&end,10);
		if ( *end!='\0' ) ds = -1;
	    } else if ( _xmlStrcmp(keyname,"italicAngle")==0 ) {
		ia = strtod(valname,&end);
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
    sf->order2 = false;
    sf->italicangle = ia;
    if ( strmatch(curve,"Quadratic")==0 )
	sf->order2 = true;
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

    setlocale(LC_NUMERIC,oldloc);

    sf->order2 = SFFindOrder(sf);
    SFSetOrder(sf,sf->order2);

    sf->map = EncMapFromEncoding(sf,FindOrMakeEncoding("Unicode"));
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
