/* Copyright (C) 2000-2012 by George Williams */
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

#include "fontforge-config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#ifndef X_DISPLAY_MISSING
# include <X11/Xatom.h>
#else
# define XA_X_HEIGHT	1
# define XA_CAP_HEIGHT	2
#endif

#include "fontP.h"
#include "gpsdrawP.h"
#include "ustring.h"
#include "charset.h"
#include "utype.h"
#include "gresource.h"
#include "fileutil.h"

/* standard 35 lazy printer afm files may be found at ftp://ftp.adobe.com/pub/adobe/type/win/all/afmfiles/base35 */

struct temp_font {
    XCharStruct *per_char;
    struct kern_info **kerns;
    int max_alloc;
    int font_as, font_ds;
    float llx, lly, urx, ury;
    int cap_h, x_h;
    int ch_width, ch_height;	/* fixed width fonts may not have the width in the char metrics */
    XCharStruct max_b;
    XCharStruct min_b;
    unsigned int is_8859_1:1;
    unsigned int is_adobe:1;
    unsigned int any_kerns: 1;
    unsigned int prop: 1;
    char *names[256];
    int min_ch, max_ch;
    int min_ch2, max_ch2;
    char family_name[200];
};

static char *iso_8859_1_names[256] = {
  "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "", "", "",
  "space", "exclam", "quotedbl", "numbersign", "dollar", "percent", "ampersand",
  "quoteright", "parenleft", "parenright", "asterisk", "plus", "comma", "minus",
  "period", "slash", "zero", "one", "two", "three", "four", "five", "six", "seven",
  "eight", "nine", "colon", "semicolon", "less", "equal", "greater", "question",
  "at", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N",
  "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "bracketleft",
  "backslash", "bracketright", "asciicircum", "underscore", "quoteleft",
  "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o",
  "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "braceleft", "bar",
  "braceright", "asciitilde", "", "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "nbspace", "exclamdown", "cent", "sterling", "currency", "yen",
  "brokenbar", "section", "eresis", "copyright", "ordfeminine", "guillemotleft",
  "logicalnot", "hyphen", "registered", "cron", "degree", "plusminus",
  "twosuperior", "threesuperior", "ute", "mu", "paragraph", "periodcentered",
  "dilla", "onesuperior", "ordmasculine", "guillemotright", "onequarter",
  "onehalf", "threequarters", "questiondown", "Agrave", "Aacute", "Acircumflex",
  "Atilde", "Adieresis", "Aring", "AE", "Ccedilla", "Egrave", "Eacute",
  "Ecircumflex", "Edieresis", "Igrave", "Iacute", "Icircumflex", "Idieresis",
  "Eth", "Ntilde", "Ograve", "Oacute", "Ocircumflex", "Otilde", "Odieresis",
  "multiply", "Oslash", "Ugrave", "Uacute", "Ucircumflex", "Udieresis",
  "Yacute", "Thorn", "germandbls", "agrave", "aacute", "acircumflex",
  "atilde", "adieresis", "aring", "ae", "ccedilla", "egrave", "eacute",
  "ecircumflex", "edieresis", "igrave", "iacute", "icircumflex", "idieresis",
  "eth", "ntilde", "ograve", "oacute", "ocircumflex", "otilde", "odieresis",
  "divide", "oslash", "ugrave", "uacute", "ucircumflex", "udieresis", "yacute",
  "thorn", "ydieresis"
};

static int find_char(struct temp_font *tf,char *name) {
    int i;
    if ( tf->is_adobe ) {
	for ( i=0; i<256; ++i )
	    if ( iso_8859_1_names[i]!=NULL && strcmp(name,iso_8859_1_names[i])==0 )
return( i );
    } else {
	for ( i=0; i<256; ++i )
	    if ( tf->names[i]!=NULL && strcmp(name,tf->names[i])==0 )
return( i );
    }

return( -1 );
}

static int name_char(struct temp_font *tf,int ch, char *name) {

    if ( tf->is_adobe )
return( find_char(tf,name));
    if ( ch>=tf->max_alloc ) {
	if ( tf->max_alloc==256 ) tf->max_alloc = 128*256; else tf->max_alloc = 65536;
	tf->per_char = realloc(tf->per_char,tf->max_alloc*sizeof(XCharStruct));
	tf->kerns = realloc(tf->kerns,tf->max_alloc*sizeof(struct kern_info *));
    }
    if ( ch!=-1 && ch<256 && name[0]!='\0')
	tf->names[ch] = copy(name);
return( ch );
}

static void parse_CharMetric_line(struct temp_font *tf, char *line) {
    int ch, ch2, wid; float a1, a2, a3, a4;
    char name[201], *pt;

    while ( isspace(*line)) ++line;
    if ( *line=='\0' )
return;
    wid = tf->ch_width; ch= -1; name[0]='\0';
    for ( pt=line; pt && *pt ; ) {
	if ( isspace(*pt) || *pt==';' )
	    ++pt;
	else {
	    if ( pt[0]=='C' && isspace(pt[1]))
		sscanf( pt, "C %d", &ch );
	    else if ( pt[0]=='C' && pt[1]=='H' && isspace(pt[2]))
		sscanf( pt, "CH <%x>", (unsigned *) &ch );
	    else if ( pt[0]=='W' && pt[1]=='X' && isspace(pt[2]))
		sscanf( pt, "WX %d", &wid );
	    else if ( pt[0]=='W' && pt[1]=='0' && pt[2]=='X' && isspace(pt[3]))
		sscanf( pt, "W0X %d", &wid );
	    else if ( pt[0]=='N' && isspace(pt[1]))
		sscanf( pt, "N %200s", name );
	    else if ( pt[0]=='B' && isspace(pt[1]))
		sscanf( pt, "B %g %g %g %g", &a1, &a2, &a3, &a4 );
	    pt = strchr(pt,';');
	}
    }

    if ( (ch2 = name_char(tf,ch,name))== -1 ) {
	if ( tf->is_8859_1 )
	    fprintf( stderr, "Unknown character name <%s>\n", name );
return;
    }
    if ( ch2>=tf->max_ch ) tf->max_ch = ch2;
    if ( ch2<=tf->min_ch ) tf->min_ch = ch2;
    if ( (ch2&0xff)>=tf->max_ch2 ) tf->max_ch2 = (ch2&0xff);
    if ( (ch2&0xff)<=tf->min_ch2 ) tf->min_ch2 = (ch2&0xff);
    a2 = -a2;		/* a descent of 1 == a bbox of -1 */
    tf->per_char[ch2].width = wid;
    tf->per_char[ch2].lbearing = a1+.5;
    tf->per_char[ch2].rbearing = a3+.5;
    tf->per_char[ch2].ascent = a4+.5;
    tf->per_char[ch2].descent = a2+.5;
    tf->per_char[ch2].attributes |= AFM_EXISTS;
    if ( tf->max_b.lbearing<a1 ) tf->max_b.lbearing = a1;
    if ( tf->max_b.rbearing<a1 ) tf->max_b.rbearing = a3;
    if ( tf->max_b.ascent<a1 ) tf->max_b.ascent = a2;
    if ( tf->max_b.descent<a1 ) tf->max_b.descent = a4;
    if ( tf->max_b.width<wid ) tf->max_b.width = wid;
    if ( tf->min_b.lbearing>a1 ) tf->min_b.lbearing = a1;
    if ( tf->min_b.rbearing>a1 ) tf->min_b.rbearing = a3;
    if ( tf->min_b.ascent>a1 ) tf->min_b.ascent = a2;
    if ( tf->min_b.descent>a1 ) tf->min_b.descent = a4;
    if ( tf->min_b.width>wid ) tf->min_b.width = wid;
}

static char *skipwhite(char *pt) {
    while ( isspace(*pt)) ++pt;
return( pt );
}

static void parse_KernData_line(struct temp_font *tf, char *line) {
    int ch, ch2, kern;
    char name[200], name2[200];
    struct kern_info *ki;

    while ( isspace(*line)) ++line;
    if ( *line=='\0' )
return;
    if ( sscanf(line,"KPX %s %s %d", name, name2, &kern )!= 3 ) {
	if ( sscanf(line,"KP %s %s %d", name, name2, &kern )!= 3 ) {
	    fprintf( stderr, "Bad afm kern line <%s>\n", line );
return;
	}
    }
    if ( (ch = find_char(tf,name))== -1 ) {
	if ( tf->is_8859_1 )
	    fprintf( stderr, "Unknown character name <%s>\n", name );
return;
    }
    if ( (ch2 = find_char(tf,name2))== -1 ) {
	if ( tf->is_8859_1 )
	    fprintf( stderr, "Unknown character name <%s>\n", name2 );
return;
    }
    tf->per_char[ch].attributes |= AFM_KERN;
    ki = malloc(sizeof(struct kern_info));
    ki->next = tf->kerns[ch];
    tf->kerns[ch] = ki;
    ki->following = ch2;
    ki->kern = kern;
}

static void ParseCharMetrics(FILE *file,struct temp_font *tf, char *line, char *pt) {
    int cnt,i;

    cnt = strtol(skipwhite(pt),NULL,10);
    for ( i=0; i<cnt && fgets(line,400,file)!=NULL; ++i ) {
	int len = strlen(line);
	if ( line[len-1]=='\n' ) line[--len]='\0';
	if ( line[len-1]=='\r' ) line[--len]='\0';
	parse_CharMetric_line(tf,line);
    }
}

static void ParseKernData(FILE *file,struct temp_font *tf, char *line, char *pt) {
    int cnt,i;

    while ( fgets(line,400,file)!=NULL ) {
	if ( strstartmatch("EndKernData",line))
return;
	else if (( pt = strstartmatch("StartKernPairs",line))!=NULL )
    break;
    }
    cnt = strtol(skipwhite(pt),NULL,10);
    for ( i=0; i<cnt && fgets(line,400,file)!=NULL; ++i ) {
	int len = strlen(line);
	if ( line[len-1]=='\n' ) line[--len]='\0';
	if ( line[len-1]=='\r' ) line[--len]='\0';
	parse_KernData_line(tf,line);
    }
}

static void buildXFont(struct temp_font *tf,struct font_data *fd) {
    XFontStruct *info;
    int i;
    
    fd->info = info = malloc(sizeof(XFontStruct));
    /* Where do I get the ascent and descent out of a postscript font???? */
    if ( tf->font_as!=0 && tf->font_ds!=0 ) {
	info->ascent = tf->font_as + (1000-tf->font_as+tf->font_ds)/2;
    } else
	info->ascent = tf->ury + (1000-tf->ury+tf->lly)/2;
    info->descent = 1000-info->ascent;
    info->min_bounds = tf->min_b;
    info->max_bounds = tf->max_b;
    if ( tf->x_h!=0 || tf->cap_h!=0 ) {
	info->n_properties = (tf->x_h!=0)+(tf->cap_h!=0);
	info->properties = malloc( info->n_properties*sizeof( XFontProp));
	i=0;
	if ( tf->x_h!=0 ) {
	    info->properties[i].name = XA_X_HEIGHT;
	    info->properties[i++].card32 = tf->x_h;
	}
	if ( tf->cap_h!=0 ) {
	    info->properties[i].name = XA_CAP_HEIGHT;
	    info->properties[i++].card32 = tf->cap_h;
	}
    }
    if ( tf->max_ch>=256 ) {
	int i,row;
	info->min_byte1 = tf->min_ch>>8;
	info->max_byte1 = tf->max_ch>>8;
	info->min_char_or_byte2 = tf->min_ch2;
	info->max_char_or_byte2 = tf->max_ch2;
	row = (info->max_char_or_byte2-info->min_char_or_byte2+1);
 	info->per_char = malloc((info->max_byte1-info->min_byte1+1)*
 		row*sizeof(XCharStruct));
 	for ( i=info->min_byte1; i<info->max_byte1; ++i )
	    memcpy(info->per_char+(i-info->min_byte1)*row,
		    tf->per_char+i*256+info->min_char_or_byte2,
		    row*sizeof(XCharStruct));
	/* Don't support kerns here */
   } else {
	info->min_char_or_byte2 = tf->min_ch;
	info->max_char_or_byte2 = tf->max_ch;
	info->per_char = malloc((tf->max_ch-tf->min_ch+1)*sizeof(XCharStruct));
	memcpy(info->per_char,tf->per_char+tf->min_ch,(tf->max_ch-tf->min_ch+1)*sizeof(XCharStruct));
	if ( tf->any_kerns ) {
	    fd->kerns = malloc((tf->max_ch-tf->min_ch+1)*sizeof(struct kern_info *));
	    memcpy(fd->kerns,tf->per_char+tf->min_ch,(tf->max_ch-tf->min_ch+1)*sizeof(struct kern_info *));
	}
    }
}

/* if fd is NULL, then don't read the font metrics stuff */
/*  but create the fd and hash it into the fontstate */
static void parse_afm(FState *fonts, char *filename, struct font_data *fd) {
    FILE *file;
    char line[400];
    struct temp_font ti;
    char *pt;
    int full = (fd!=NULL);
    struct font_name *fn;
    unichar_t ubuf[300];
    
    if (( file=fopen(filename,"r"))==NULL ) {
	GDrawIError( "Can't open afm file %s\n", filename);
return;
    }
    memset(&ti,'\0',sizeof(ti));
    ti.min_ch = 65535;
    ti.prop = true;
    if ( fd==NULL )
	fd = calloc(1,sizeof(*fd));

    while ( fgets(line,sizeof(line),file)!=NULL ) {
	int len = strlen(line);
	if ( line[len-1]=='\n' ) line[--len]='\0';
	if ( line[len-1]=='\r' ) line[--len]='\0';
    	if (( pt = strstartmatch("FontName", line))!=NULL ) {
	    if ( !full ) {
		fd->localname = copy(skipwhite(pt));
		fd->weight = _GDraw_FontFigureWeights(def2u_strncpy(ubuf,fd->localname,sizeof(ubuf)/sizeof(ubuf[0])));
		if ( strstrmatch(fd->localname,"Italic") || strstrmatch(fd->localname,"Oblique"))
		    fd->style = fs_italic;
		if ( strstrmatch(fd->localname,"SmallCaps") )
		    fd->style = fs_smallcaps;
		if ( strstrmatch(fd->localname,"condensed") )
		    fd->style = fs_condensed;
		if ( strstrmatch(fd->localname,"extended") )
		    fd->style = fs_extended;
	    }
    	} else if (( pt = strstartmatch("FamilyName", line))!=NULL )
    	    strcpy(ti.family_name,skipwhite(pt));
    	else if (( pt = strstartmatch("Weight", line))!=NULL )
    	    /*fd->weight = 3*/ /* !!!! */;
    	else if (( pt = strstartmatch("EncodingScheme", line))!=NULL ) {
	    if ( full )
		/* Don't bother, we've done it before */;
    	    else if ( strcmp(skipwhite(pt),"AdobeStandardEncoding")==0 ) {
    		fd->map = em_iso8859_1;
		fd->needsremap = true;
    		ti.is_adobe = 1;
    	    } else if ( strcmp(skipwhite(pt),"SymbolEncoding")==0 ) {
    		fd->map = em_symbol;
    	    } else if ( strcmp(skipwhite(pt),"FontSpecific")==0 &&
		    fd->localname!=NULL && strcmp(fd->localname,"Symbol")==0 ) {
    		fd->map = em_symbol;
    	    } else if ( strcmp(skipwhite(pt),"DingbatsEncoding")==0 ) {
   		fd->map = em_zapfding;
    	    } else if ( strcmp(skipwhite(pt),"FontSpecific")==0 &&
		    fd->localname!=NULL && strcmp(fd->localname,"ZapfDingbats")==0 ) {
    		fd->map = em_zapfding;
    	    } else {
    		fd->map = _GDraw_ParseMapping(def2u_strncpy(ubuf,skipwhite(pt),sizeof(ubuf)/sizeof(ubuf[0])));
    		if ( fd->map==em_none ) {
		    if ( !full )
			fd->charmap_name = uc_copy(skipwhite(pt));
		    fd->map = em_max;
		    /* Might turn out to be a user specified mapping later */
    		}
    		ti.is_8859_1 = (fd->map == em_iso8859_1);
    	    }
    	} else if (( pt = strstartmatch("Ascender", line))!=NULL )
    	    sscanf(pt, "%d", &ti.font_as );
    	else if (( pt = strstartmatch("Descender", line))!=NULL )
    	    sscanf(pt, "%d", &ti.font_ds );
    	else if (( pt = strstartmatch("CapHeight", line))!=NULL ) {
    	    sscanf(pt, "%d", &ti.cap_h );
	    fd->cap_height = ti.cap_h;
    	} else if (( pt = strstartmatch("XHeight", line))!=NULL ) {
    	    sscanf(pt, "%d", &ti.x_h );
	    fd->cap_height = ti.x_h;
    	} else if (( pt = strstartmatch("FontBBox", line))!=NULL )
    	    sscanf(pt, "%g %g %g %g", &ti.llx, &ti.lly, &ti.urx, &ti.ury );
    	else if (( pt = strstartmatch("CharWidth", line))!=NULL ) {
    	    ti.prop = false;
    	    sscanf(pt, "%d %d", &ti.ch_width, &ti.ch_height );
    	} else if (( pt = strstartmatch("IsFixedPitch", line))!=NULL )
    	    ti.prop = strstrmatch(pt,"true")==NULL;
    	else if (( pt = strstartmatch("MetricSets", line))!=NULL ) {
    	    int val;
    	    sscanf(pt, "%d", &val );
    	    if ( val==1 ) {			/* Can't handle vertical printing */
    		fclose(file);
    		free(fd);
return;
	    }
    	} else if (( pt = strstartmatch("MappingScheme", line))!=NULL ) {
    	    int val;
    	    sscanf(pt, "%d", &val );
    	    if ( val!=2 ) {			/* Can only handle 8/8 mapping (ie. 2 byte chars) */
    		fclose(file);
    		free(fd);
return;
	    }
    	} else if (( pt = strstartmatch("IsCIDFont", line))!=NULL ) {
    	    if ( strstrmatch(pt,"true")!=NULL ) {	/* I need a mapping! */
    		fclose(file);
    		free(fd);
return;
	    }
    	} else if (( pt = strstartmatch("StartCharMetrics", line))!=NULL ) {
	    if ( !full )
	break;
	    ti.max_alloc = 256;
	    ti.per_char = malloc(256*sizeof(XCharStruct));
	    ti.kerns = malloc(256*sizeof(struct kern_info *));
    	    ParseCharMetrics(file,&ti,line,pt);
    	} else if (( pt = strstartmatch("StartKernData", line))!=NULL ) {
	    if ( !full )
	break;
    	    ParseKernData(file,&ti,line,pt);
	}
    }
    fclose(file);

    if ( !full ) {
	static char *extensions[] = { ".pfa", ".PFA", ".pfb", ".PFB", ".ps", ".PS", NULL };
	int i;
	strcpy(line,filename);
	fd->metricsfile = copy(filename);
	/* some directory trees put the afm files in their own sub-directory */
	if (( pt = strstr(line,"/afm/"))!=NULL )
	    strcpy(pt,pt+4);
	pt = strrchr(line,'.');
	if ( pt!=NULL ) {
	    for ( i=0; extensions[i]!=NULL; ++i ) {
		strcpy(pt,extensions[i]);
		if ( GFileExists(line)) {
		    fd->fontfile = copy(line);
	    break;
		}
	    }
	}
	fn = _GDraw_HashFontFamily(fonts,
		def2u_strncpy(ubuf,ti.family_name,sizeof(ubuf)/sizeof(ubuf[0])),
		ti.prop);
	fd->next = fn->data[fd->map];
	fn->data[fd->map] = fd;
	fd->parent = fn;
	fd->is_scalable = true;
    } else
	buildXFont(&ti,fd);
    free(ti.kerns); free(ti.per_char);
}

int _GPSDraw_InitFonts(FState *fonts) {
    char *pt, *path, *pp, *epp;
    char dirname[1025], filename[1200];
    DIR *file;
    struct dirent *rec;
    int offset;

    if ( fonts->names_loaded!=0 )
return( true );

    path = GResourceFindString("PSFontPath");
    if ( path==NULL )
	path = copy(getenv("PSFONTPATH"));
    if ( path==NULL ) {
	path = copy(GFileBuildName(GResourceProgramDir,"print",filename,sizeof(filename)));
	/* append /print to program directory */
    }

    for ( pp = path; *pp; pp = epp ) {
	if (( epp = strchr(pp,':'))==NULL )
	    epp = pp+strlen(pp);
	strncpy(dirname,pp,epp-pp);
	GFileBuildName(dirname,"afm",dirname,sizeof(dirname));
	if ( !GFileExists(dirname))
	    dirname[epp-pp] = '\0';
	file = opendir(dirname);
	if ( file!=NULL ) {
	    if (( rec = readdir(file))==NULL ) {		/* Must have "." */
		closedir(file);
    continue;
	    }
	    /* There's a bug in some solaris readdir libs, so that the structure */
	    /*  returned doesn't match the include file alignment */
	    offset = 0;
	    if ( strcmp(rec->d_name,".")!=0 && strcmp(rec->d_name-2,".")==0 )
		offset = -2;
	    while ( (rec = readdir(file))!=NULL ) {
		char *name = rec->d_name+offset;
		if ( (pt = strstrmatch(name,".afm"))!=NULL && pt[4]=='\0' ) {	/* don't find "*.afm~" */
		    GFileBuildName(dirname,name,filename,sizeof(filename));
		    parse_afm(fonts,filename,NULL);
		/* I don't understand acfm files, but I think I can skip 'em */
		}
	    }
	    closedir(file);
	}
    }
    fonts->names_loaded = true;
return( fonts->names_loaded );
}

void _GPSDraw_ResetFonts(FState *fonts) {
    int j;
    struct font_name *fn;
    struct font_data *fd, *prev, *next;
    int map;

    /* After we are done printing we need to reset the font state (if we start another*/
    /*  print job and we leave the old state hanging around we will think fonts have */
    /*  been downloaded that are long forgotten by the printer */
    for ( j=0; j<26; ++j ) {
	for ( fn = fonts->font_names[j]; fn!=NULL; fn=fn->next ) {
	    for ( map=0; map<em_max; ++map ) {
		prev = NULL;
		for ( fd=fn->data[map]; fd!=NULL; fd=next ) {
		    next = fd->next;
		    if ( fd->point_size!=0 ) {
			_GDraw_FreeFD(fd);
			if ( prev==NULL )
			    fn->data[map] = next;
			else
			    prev->next = next;
		    } else {
			prev = fd;
			fd->copiedtoprinter = false;
			fd->includenoted = false;
			fd->remapped = false;
		    }
		}
	    }
	}
    }
}


void _GPSDraw_ListNeededFonts(GPSWindow ps) {
    FState *fonts = ps->display->fontstate;
    int j;
    struct font_name *fn;
    struct font_data *fd, *prev, *next;
    int map;
    int first = true;

    /* After we are done printing we need to tell postscript what fonts we expect */
    /*  to be on the printer */
    for ( j=0; j<26; ++j ) {
	for ( fn = fonts->font_names[j]; fn!=NULL; fn=fn->next ) {
	    for ( map=0; map<em_max; ++map ) {
		prev = NULL;
		for ( fd=fn->data[map]; fd!=NULL; fd=next ) {
		    next = fd->next;
		    if ( fd->point_size==0 && fd->includenoted ) {
			if ( first )
			    fprintf(ps->output_file, "%%%%DocumentNeededResources: font %s\n",
				    fd->localname );
			else
			    fprintf(ps->output_file, "%%%%+ font %s\n",
				    fd->localname );
			first = false;
		    }
		}
	    }
	}
    }
    if ( first ) {
	/* not sure how to say we don't need anything, but the PS Ref Man */
	/*  says there must be a line at end if there's an (atend) at start */
	fprintf(ps->output_file, "%%%%DocumentNeededResources:\n" );
    }

    first = true;
    for ( j=0; j<26; ++j ) {
	for ( fn = fonts->font_names[j]; fn!=NULL; fn=fn->next ) {
	    for ( map=0; map<em_max; ++map ) {
		prev = NULL;
		for ( fd=fn->data[map]; fd!=NULL; fd=next ) {
		    next = fd->next;
		    if ( fd->point_size==0 && fd->copiedtoprinter ) {
			if ( first )
			    fprintf(ps->output_file, "%%%%DocumentSuppliedResources: font %s\n",
				    fd->localname );
			else
			    fprintf(ps->output_file, "%%%%+ font %s\n",
				    fd->localname );
			first = false;
		    }
		}
	    }
	}
    }
    if ( first ) {
	/* not sure how to say we don't use anything, but the PS Ref Man */
	/*  says there must be a line at end if there's an (atend) at start */
	fprintf(ps->output_file, "%%%%DocumentSuppliedResources:\n" );
    }
}

void _GPSDraw_CopyFile(FILE *to, FILE *from) {
    char buffer[8*1024], *buf;
    int len, ch, flag, i;

    ch = getc(from);
    if ( ch== (unsigned char) '\200' ) {
	/* Is it a postscript binary font file? */
	/* strip out the binary headers and de-binary it */
	while ( ch== (unsigned char) '\200' ) {
	    flag = getc(from);
	    if ( flag==3 )		/* eof mark */
	break;
	    len = getc(from);
	    len |= getc(from)<<8;
	    len |= getc(from)<<16;
	    len |= getc(from)<<24;
	    buf = malloc(len);
	    if ( flag==1 ) {
		/* ascii section */
		len = fread(buf,sizeof(char),len,from);
		fwrite(buf,sizeof(char),len,to);
	    } else if ( flag==2 ) {
		/* binary section */
		len = fread(buf,sizeof(char),len,from);
		for ( i=0; i<len; ++i ) {
		    ch = buf[i];
		    if ( ((ch>>4)&0xf) <= 9 )
			putc('0'+((ch>>4)&0xf),to);
		    else
			putc('A'-10+((ch>>4)&0xf),to);
		    if ( (ch&0xf) <= 9 )
			putc('0'+(ch&0xf),to);
		    else
			putc('A'-10+(ch&0xf),to);
		    if ( (i&0x1f)==0x1f )
			putc('\n',to);
		}
	    }
	    free(buf);
	    ch = getc(from);
	}
    } else {
	ungetc(ch,from);
	while ((len=fread(buffer,sizeof(char),sizeof(buffer),from))>0 )
	    fwrite(buffer,sizeof(char),len,to);
    }
    fputc('\n',to);
}

void _GPSDraw_ProcessFont(GPSWindow ps, struct font_data *fd) {
    char buffer[100];
    struct font_data *base = fd->base;
    FILE *output_file = ps->init_file;
    double skew = 0.0, factor = 1.0;
    int style = fd->style;
    int point = fd->point_size;

    if ( base->base!=NULL && base->needsprocessing )
	_GPSDraw_ProcessFont(ps,base);
    else if ( base->base==NULL ) {
	if ( base->fontfile!=NULL && !base->copiedtoprinter ) {
	    FILE *ff = fopen( base->fontfile,"rb");
	    if ( ff==NULL ) GDrawIError("Can't download font: %s", base->localname );
	    else {
		fprintf( output_file, "%%%%BeginResource: font %s\n", base->localname );
		_GPSDraw_CopyFile(output_file,ff);
		fclose(ff);
		fprintf( output_file, "%%%%EndResource\n" );
	    }
	    base->copiedtoprinter = true;
	} else if ( base->fontfile==NULL && !base->includenoted ) {
	    fprintf( output_file, "%%%%IncludeResource: font %s\n", base->localname );
	    base->includenoted = true;
	}

	if ( base->needsremap && !base->remapped ) {
	    fprintf( output_file, "/%s-ISO-8859-1 /%s findfont ISOLatin1Encoding g_font_remap definefont\n",
		    base->localname, base->localname );
	    base->remapped = true;
	}
    }

    if ( ((style&fs_italic) && !(base->style&fs_italic)) ||
	    ((style&fs_extended) && !(base->style&fs_extended)) ||
	    ((style&fs_condensed) && !(base->style&fs_condensed)) ) {
	/* Let's do some skewing.  It's not real italic, but it's the best we gots */
	if ( base->base==NULL )
	    sprintf( buffer, "%s__%d_%s%s%s", base->localname, point,
		    ((style&fs_italic) && !(base->style&fs_italic))? "O":"",
		    ((style&fs_extended) && !(base->style&fs_extended))? "E":"",
		    ((style&fs_condensed) && !(base->style&fs_condensed))? "C":"");
	else
	    sprintf( buffer, "%s_%s%s%s", base->localname, 
		    ((style&fs_italic) && !(base->style&fs_italic))? "O":"",
		    ((style&fs_extended) && !(base->style&fs_extended))? "E":"",
		    ((style&fs_condensed) && !(base->style&fs_condensed))? "C":"");
	if ( (style&fs_italic) && !(base->style&fs_italic) )
	    skew = point/10.0;
	if ( (style&fs_extended) && !(base->style&fs_extended) )
	    factor = 1.1;
	if ( (style&fs_condensed) && !(base->style&fs_condensed) )
	    factor = .9;
	fprintf( output_file, "MyFontDict /%s /%s%s findfont [%g 0 %g %d 0 0] makefont put\n",
		buffer, base->localname, base->remapped?"-ISO-8859-1":"",
		factor*point, skew, point );
    } else {
	sprintf( buffer, "%s__%d", base->localname, point );
	fprintf( output_file, "MyFontDict /%s /%s%s findfont %d scalefont put\n",
		buffer, base->localname, base->remapped?"-ISO-8859-1":"", point );
    }

    fd->needsprocessing = false;
}
