/* Copyright (C) 2000-2006 by George Williams */
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

/* Routines to handle bdf properties, and a dialog to set them */
#include "pfaeditui.h"
#include "splinefont.h"
#include <string.h>
#include <ustring.h>
#include <utype.h>
#include <math.h>
#include <gkeysym.h>

struct bdf_dlg_font {
    int old_prop_cnt;
    BDFProperties *old_props;
    BDFFont *bdf;
    int top_prop;	/* for the scrollbar */
    int sel_prop;
};

struct bdf_dlg {
    int fcnt;
    struct bdf_dlg_font *fonts;
    struct bdf_dlg_font *cur;
    EncMap *map;
    SplineFont *sf;
    GWindow gw, v;
    GGadget *vsb;
    GGadget *tf;
    GFont *font;
    int value_x;
    int vheight;
    int vwidth;
    int height,width;
    int fh, as;
    int done;
    int active;
    int press_pos;
};

struct {
    char *name;
    int type;
    int defaultable;
} StandardProps[] = {
	{ "FONT", prt_atom, true },
	{ "COMMENT", prt_string, false },
	{ "FOUNDRY", prt_string|prt_property, true },
	{ "FAMILY_NAME", prt_string|prt_property, true },
	{ "WEIGHT_NAME", prt_string|prt_property, true },
	{ "SLANT", prt_string|prt_property, true },
	{ "SETWIDTH_NAME", prt_string|prt_property, true },
	{ "ADD_STYLE_NAME", prt_string|prt_property, true },
	{ "PIXEL_SIZE", prt_int|prt_property, true },
	{ "POINT_SIZE", prt_int|prt_property, true },
	{ "RESOLUTION_X", prt_uint|prt_property, true },
	{ "RESOLUTION_Y", prt_uint|prt_property, true },
	{ "SPACING", prt_string|prt_property, true },
	{ "AVERAGE_WIDTH", prt_int|prt_property, true },
	{ "CHARSET_REGISTRY", prt_string|prt_property, true },
	{ "CHARSET_ENCODING", prt_string|prt_property, true },
	{ "FONTNAME_REGISTRY", prt_string|prt_property, true },
	{ "CHARSET_COLLECTIONS", prt_string|prt_property, true },
	{ "BITS_PER_PIXEL", prt_int|prt_property, true },
	{ "FONT_NAME", prt_string|prt_property, true },
	{ "FACE_NAME", prt_string|prt_property, true },
	{ "COPYRIGHT", prt_string|prt_property, true },
	{ "FONT_VERSION", prt_string|prt_property, true },
	{ "FONT_ASCENT", prt_int|prt_property, true },
	{ "FONT_DESCENT", prt_int|prt_property, true },
	{ "UNDERLINE_POSITION", prt_int|prt_property, true },
	{ "UNDERLINE_THICKNESS", prt_int|prt_property, true },
	{ "X_HEIGHT", prt_int|prt_property, true },
	{ "CAP_HEIGHT", prt_int|prt_property, true },
	{ "DEFAULT_CHAR", prt_uint|prt_property, true },
	{ "RAW_ASCENT", prt_int|prt_property, true },
	{ "RAW_DESCENT", prt_int|prt_property, true },
	{ "ITALIC_ANGLE", prt_int|prt_property, true },
	{ "NORM_SPACE", prt_int|prt_property, true },
	{ "QUAD_WIDTH", prt_int|prt_property, true },	/* Depreciated */
	{ "RESOLUTION", prt_uint|prt_property, true },	/* Depreciated */
	{ "RELATIVE_SETWIDTH", prt_uint|prt_property, true },
	{ "RELATIVE_WEIGHT", prt_uint|prt_property, true },
	{ "SUPERSCRIPT_X", prt_int|prt_property, true },
	{ "SUPERSCRIPT_Y", prt_int|prt_property, true },
	{ "SUPERSCRIPT_SIZE", prt_int|prt_property, true },
	{ "SUBSCRIPT_X", prt_int|prt_property, true },
	{ "SUBSCRIPT_Y", prt_int|prt_property, true },
	{ "SUBSCRIPT_SIZE", prt_int|prt_property, true },
	{ "FIGURE_WIDTH", prt_int|prt_property, true },
	{ "AVG_LOWERCASE_WIDTH", prt_int|prt_property, true },
	{ "AVG_UPPERCASE_WIDTH", prt_int|prt_property, true },
#ifdef FONTFORGE_CONFIG_BDF_GLYPH_RANGES
	{ "_XFREE86_GLYPH_RANGES", prt_string|prt_property, true },
#endif
	{ "WEIGHT", prt_uint|prt_property, false },
	{ "DESTINATION", prt_uint|prt_property, false },
	{ "MIN_SPACE", prt_int|prt_property, false },
	{ "MAX_SPACE", prt_int|prt_property, false },
	{ "END_SPACE", prt_int|prt_property, false },
	{ "SMALL_CAP_SIZE", prt_int|prt_property, false },
	{ "STRIKEOUT_ASCENT", prt_int|prt_property, false },
	{ "STRIKEOUT_DESCENT", prt_int|prt_property, false },
	{ "NOTICE", prt_string|prt_property, false },
	{ "FONT_TYPE", prt_string|prt_property, false },
	{ "RASTERIZER_NAME", prt_string|prt_property, false },
	{ "RASTERIZER_VERSION", prt_string|prt_property, false },
	{ "AXIS_NAMES", prt_string|prt_property, false },
	{ "AXIS_LIMITS", prt_string|prt_property, false },
	{ "AXIS_TYPES", prt_string|prt_property, false },
	NULL
};

int IsUnsignedBDFKey(char *key) {
    /* X says that some properties are signed and some are unsigned */
    /* neither bdf nor pcf supports this, but David (of freetype) */
    /* thought the sfnt BDF table should include it */
    /* Since these are X11 atom names, case is significant */
    int i;

    for ( i=0; StandardProps[i].name!=NULL; ++i )
	if ( strcmp(key,StandardProps[i].name)==0 )
return(( StandardProps[i].type&~prt_property)==prt_uint );

return( false );
}

static int NumericKey(char *key) {
    int i;

    for ( i=0; StandardProps[i].name!=NULL; ++i )
	if ( strcmp(key,StandardProps[i].name)==0 )
return(( StandardProps[i].type&~prt_property)==prt_uint || ( StandardProps[i].type&~prt_property)==prt_int );

return( false );
}

static int KnownProp(char *keyword) {	/* We can generate default values for these */
    int i;

    for ( i=0; StandardProps[i].name!=NULL; ++i )
	if ( strcmp(keyword,StandardProps[i].name)==0 )
return( StandardProps[i].defaultable );

return( false );
}

static int KeyType(char *keyword) {
    int i;

    for ( i=0; StandardProps[i].name!=NULL; ++i )
	if ( strcmp(keyword,StandardProps[i].name)==0 )
return( StandardProps[i].type );

return( -1 );
}

static int UnknownKey(char *keyword) {
    int i;

    for ( i=0; StandardProps[i].name!=NULL; ++i )
	if ( strcmp(keyword,StandardProps[i].name)==0 )
return( false );

return( true );
}

static char *BdfPropHasString(BDFFont *font,const char *key, char *def ) {
    int i;

    for ( i=0; i<font->prop_cnt; ++i ) if ( strcmp(font->props[i].name,key)==0 ) {
	switch ( font->props[i].type&~prt_property ) {
	  case prt_string:
	    if ( font->props[i].u.str!=NULL )		/* Can be NULL when creating & defaulting a prop */
return( font->props[i].u.str );
	  break;
	  case prt_atom:
	    if ( font->props[i].u.atom!=NULL )		/* Can be NULL when creating & defaulting a prop */
return( font->props[i].u.atom );
	  break;
	}
    }
return( def );
}

int BdfPropHasInt(BDFFont *font,const char *key, int def ) {
    int i;

    for ( i=0; i<font->prop_cnt; ++i ) if ( strcmp(font->props[i].name,key)==0 ) {
	switch ( font->props[i].type&~prt_property ) {
	  case prt_int: case prt_uint:
return( font->props[i].u.val );
	  break;
	}
    }
return( def );
}

static void BDFPropAddString(BDFFont *bdf,char *keyword,char *value,char *match_key) {
    int i;

    if ( match_key!=NULL && strcmp(keyword,match_key)!=0 )
return;

    for ( i=0; i<bdf->prop_cnt; ++i )
	if ( strcmp(keyword,bdf->props[i].name)==0 ) {
	    if ( (bdf->props[i].type&~prt_property)==prt_string ||
		    (bdf->props[i].type&~prt_property)==prt_atom )
		free( bdf->props[i].u.str );
    break;
	}
    if ( i==bdf->prop_cnt ) {
	if ( i>=bdf->prop_max )
	    bdf->props = grealloc(bdf->props,(bdf->prop_max+=10)*sizeof(BDFProperties));
	++bdf->prop_cnt;
	bdf->props[i].name = copy(keyword);
    }
    if ( strcmp(keyword,"FONT")==0 )
	bdf->props[i].type = prt_atom;
    else if ( strcmp(keyword,"COMMENT")==0 )
	bdf->props[i].type = prt_string;
    else
	bdf->props[i].type = prt_string | prt_property;
    bdf->props[i].u.str = copy(value);
}

static void BDFPropAddInt(BDFFont *bdf,char *keyword,int value,char *match_key) {
    int i;

    if ( match_key!=NULL && strcmp(keyword,match_key)!=0 )
return;

    for ( i=0; i<bdf->prop_cnt; ++i )
	if ( strcmp(keyword,bdf->props[i].name)==0 ) {
	    if ( (bdf->props[i].type&~prt_property)==prt_string ||
		    (bdf->props[i].type&~prt_property)==prt_atom )
		free( bdf->props[i].u.str );
    break;
	}
    if ( i==bdf->prop_cnt ) {
	if ( i>=bdf->prop_max )
	    bdf->props = grealloc(bdf->props,(bdf->prop_max+=10)*sizeof(BDFProperties));
	++bdf->prop_cnt;
	bdf->props[i].name = copy(keyword);
    }
    if ( IsUnsignedBDFKey(keyword) )
	bdf->props[i].type = prt_uint | prt_property;
    else
	bdf->props[i].type = prt_int | prt_property;
    bdf->props[i].u.val = value;
}

#ifdef FONTFORGE_CONFIG_BDF_GLYPH_RANGES
static void BDFPropClearKey(BDFFont *bdf,char *keyword) {
    int i, j;

    for ( i=j=0; i<bdf->prop_cnt; ++i ) {
	if ( strcmp(bdf->props[i].name,keyword)==0 ) {
	    free(bdf->props[i].name);
	    if ( (bdf->props[i].type&~prt_property)==prt_string ||
		    (bdf->props[i].type&~prt_property)==prt_atom )
		free(bdf->props[i].u.str );
	} else if ( i!=j ) {
	    bdf->props[j++] = bdf->props[i];
	} else
	    ++j;
    }
}
#endif		/* FONTFORGE_CONFIG_BDF_GLYPH_RANGES */

static void BDFPropAppendString(BDFFont *bdf,char *keyword,char *value) {
    int i = bdf->prop_cnt;

    if ( i>=bdf->prop_max )
	bdf->props = grealloc(bdf->props,(bdf->prop_max+=10)*sizeof(BDFProperties));
    ++bdf->prop_cnt;
    bdf->props[i].name = copy(keyword);
    if ( strcmp(keyword,"COMMENT")==0 )
	bdf->props[i].type = prt_string;
    else if ( strcmp(keyword,"FONT")==0 )
	bdf->props[i].type = prt_atom;
    else
	bdf->props[i].type = prt_string | prt_property;
    bdf->props[i].u.str = copy(value);
}

static int GenerateGlyphRanges(BDFFont *font) {
#ifdef FONTFORGE_CONFIG_BDF_GLYPH_RANGES
    char buffer[300], *pt, *end, add[30];
    int i, j, max, cnt=0;

    /* I gather that the _XFREE86_GLYPH_RANGES property has been dropped */
    /*  because it was felt that the metrics data would allow users to */
    /*  determine whether a character was in the font or not. I'm not in */
    /*  complete agreement with that argument because there are several */
    /*  zero-width spaces whose presence is not specified by the metrics */
    /*  but it is almost irrelevant whether they are present or not (though */
    /*  guessing wrong would present you with a "DEFAULT_CHAR" rather than */
    /*  nothing) */

    if ( map->enc->is_unicodebmp || map->enc->is_unicodefull )
	max = map->enc->char_cnt;
    else
return( 0 );
    BDFPropClearKey(font,"_XFREE86_GLYPH_RANGES");
    pt = buffer; end = pt+sizeof(buffer);
    for ( i=0; i<font->glyphcnt && i<max; ++i ) if ( !IsntBDFChar(font->glyphs[i]) ) {
	for ( j=i+1; j<font->glyphcnt && j<max && !IsntBDFChar(font->glyphs[j]); ++j );
	--j;
	if ( j==i )
	    sprintf( add, "%d ", i );
	else
	    sprintf( add, "%d_%d ", i, j );
	i = j;
	if ( pt+strlen(add) >= end ) {
	    pt[-1] = '\0';		/* was a space */
	    BDFPropAppendString(font,"_XFREE86_GLYPH_RANGES", buffer);
	    pt = buffer;
	    ++cnt;
	}
	strcpy(pt,add);
	pt += strlen(pt);
    }
    if ( pt!=buffer ) {
	pt[-1] = '\0';		/* was a space */
	BDFPropAppendString(font,"_XFREE86_GLYPH_RANGES", buffer);
	++cnt;
    }
return( cnt );
#else
return( 0 );
#endif		/* FONTFORGE_CONFIG_BDF_GLYPH_RANGES */
}

static char *AllSame(BDFFont *font,int *avg,int *cnt) {
    int c=0, a=0, common= -1;
    int i;
    BDFChar *bdfc;
    int cell = -1;

    /* Return 'P' if a proporitional font */
    /* Return 'C' if an X11 "cell" font (all glyphs within (0,width)x(ascent,desent) & monospace ) */
    /* Return 'M' if monospace (and not a cell) */
    for ( i=0; i<font->glyphcnt; ++i ) {
	if ( (bdfc = font->glyphs[i])!=NULL && !IsntBDFChar(bdfc)) {
	    ++c;
	    a += bdfc->width;
	    if ( common==-1 ) common = bdfc->width; else if ( common!=bdfc->width ) { common = -2; cell = 0; }
	    if ( cell ) {
		if ( bdfc->xmin<0 || bdfc->xmax>common || bdfc->ymax>font->ascent ||
			-bdfc->ymin>font->descent )
		    cell = 0;
		else
		    cell = 1;
	    }
	}
    }
    if ( c==0 ) *avg=0; else *avg = (10*a)/c;
    *cnt = c;
return(common==-2 ? "P" : cell ? "C" : "M" );
}

static void def_Charset_Col(SplineFont *sf,EncMap *map, char *buffer) {
    uint32 codepages[2];
    /* A buffer with 250 bytes should be more than big enough */

    OS2FigureCodePages(sf,codepages);
    buffer[0] = '\0';
    if ( codepages[1]&(1U<<31) )
	strcat(buffer , "ASCII " );
    if ( (codepages[1]&(1U<<30)) )
	strcat(buffer , "ISOLatin1Encoding " );
    if ( (codepages[0]&2) )
	strcat(buffer , "ISO8859-2 " );
    if ( (codepages[0]&4) )
	strcat(buffer , "ISO8859-5 " );
    if ( (codepages[0]&8) )
	strcat(buffer , "ISO8859-7 " );
    if ( (codepages[0]&0x10) )
	strcat(buffer , "ISO8859-9 " );
    if ( (codepages[0]&0x20) )
	strcat(buffer , "ISO8859-8 " );
    if ( (codepages[0]&0x40) )
	strcat(buffer , "ISO8859-6 " );
    if ( (codepages[0]&0x80) )
	strcat(buffer , "ISO8859-4 " );
    if ( (codepages[0]&0x10000)  )
	strcat(buffer , "ISO8859-11 " );
    if ( (codepages[0]&0x20000) && (map->enc->is_unicodebmp || map->enc->is_unicodefull ))
	strcat(buffer , "JISX0208.1997 " );
    if ( (codepages[0]&0x40000) && (map->enc->is_unicodebmp || map->enc->is_unicodefull ) )
	strcat(buffer , "GB2312.1980 " );
    if ( (codepages[0]&0x80000) && (map->enc->is_unicodebmp || map->enc->is_unicodefull ))
	strcat(buffer , "KSC5601.1992 " );
    if ( (codepages[0]&0x100000) && (map->enc->is_unicodebmp || map->enc->is_unicodefull ))
	strcat(buffer , "BIG5 " );
    if ( (codepages[0]&0x80000000) )
	strcat(buffer , "Symbol " );

    strcat(buffer , EncodingName(map->enc) );
}

static void def_Charset_Enc(EncMap *map,char *reg,char *enc) {
    char *pt;

    if ( map->enc->is_custom || map->enc->is_original ) {
	strcpy( reg, "FontSpecific" );
	strcpy( enc, "0");
    } else if ( (pt = strstr(map->enc->enc_name,"8859"))!=NULL ) {
	strcpy( reg, "ISO8859" );
	pt += 4;
	if ( !isdigit(*pt)) ++pt;
	strcpy(enc, pt );
    } else if ( map->enc->is_unicodebmp || map->enc->is_unicodefull ) {
	strcpy( reg, "ISO10646" );
	strcpy( enc, "1" );
    } else if ( strstr(map->enc->enc_name, "5601")!=NULL ) {
	strcpy( reg, "KSC5601.1992" );
	strcpy( enc, "3" );
    } else if ( strstr(map->enc->enc_name, "2312")!=NULL ) {
	strcpy( reg, "GB2312.1980" );
	strcpy( enc, "0" );
    } else if ( strstrmatch(map->enc->enc_name, "JISX0208")!=NULL ) {
	strcpy( reg, "JISX0208.1997" );
	strcpy( enc, "0" );
    } else {
	strcpy( reg, EncodingName(map->enc) );
	pt = strchr( reg,'-');
	if ( pt==NULL )
	    strcpy( enc, "0" );
	else {
	    strcpy( enc, pt+1 );
	    *pt = '\0';
	}
    }
}

static void decomposename(BDFFont *font, char *fontname, char *family_name, char *weight_name,
	char *slant, char *stylename, char *squeeze, char *sffamily, char *sfweight ) {
    char *ital, *bold, *style, *compress;
    char ich='\0', bch='\0', sch='\0', cch='\0';
    char *pt;

    if ( *fontname=='-' ) {
	sscanf(fontname,"-%*[^-]-%[^-]-%[^-]-%[^-]-%[^-]-%[^-]",
		family_name,weight_name,slant,squeeze,stylename);
    } else {
	/* Assume it's a fontographer type name */
	strcpy(slant,"R");
	strcpy(squeeze,"Normal");
/* Sigh. I don't use "Italic" and "Oblique" because urw truncates these to 4 */
/*  characters each. */
	if (( ital = strstrmatch(fontname,"Ital"))!=NULL )
	    strcpy(slant,"I");
	else if (( ital = strstrmatch(fontname,"Kurs"))!=NULL )
	    strcpy(slant,"I");
	else if (( ital = strstr(fontname,"Obli"))!=NULL )
	    strcpy(slant,"O");
	else if (( ital = strstr(fontname,"Slanted"))!=NULL )
	    strcpy(slant,"O");
/* The XLFD spec also supports RI and RO (reverse skewed italic/oblique) */
    
	if (( bold = strstr(fontname,"Bold"))==NULL &&
		( bold = strstr(fontname,"Ligh"))==NULL &&
		( bold = strstr(fontname,"Demi"))==NULL &&
		( bold = strstr(fontname,"Blac"))==NULL &&
		( bold = strstr(fontname,"Roma"))==NULL &&
		( bold = strstr(fontname,"Book"))==NULL &&
		( bold = strstr(fontname,"Regu"))==NULL &&
		( bold = strstr(fontname,"Medi"))==NULL );	/* Again, URW */
    
	if (( style = strstr(fontname,"Sans"))==NULL &&
		(compress = strstr(fontname,"SmallCaps"))==NULL );
	if ((compress = strstr(fontname,"Extended"))==NULL &&
		(compress = strstr(fontname,"Condensed"))==NULL );
    
	strcpy(weight_name,"Medium");
	*stylename='\0';
	if ( bold!=NULL ) { bch = *bold; *bold = '\0'; }
	if ( ital!=NULL ) { ich = *ital; *ital = '\0'; }
	if ( style!=NULL ) { sch = *style; *style = '\0'; }
	if ( compress!=NULL ) { cch = *compress; *compress = '\0'; }
	strcpy(family_name,fontname);
	if ( bold!=NULL ) {
	    *bold = bch; 
	    strcpy(weight_name,bold);
	    *bold = '\0';
	}
	if ( sfweight!=NULL && *sfweight!='\0' )
	    strcpy(weight_name,sfweight);
	if ( style!=NULL ) {
	    *style = sch;
	    strcpy(stylename,style);
	    *style = '\0';
	}
	if ( compress!=NULL ) {
	    *compress = cch;
	    strcpy(squeeze,compress);
	}
	if ( style!=NULL ) *style = sch;
	if ( bold!=NULL ) *bold = bch; 
	if ( ital!=NULL ) *ital = ich;
    }
    if ( sffamily!=NULL && *sffamily!='\0' )
	strcpy(family_name,sffamily);
    while ( (pt=strchr(family_name,'-'))!=NULL )
	strcpy(pt,pt+1);
}

static char *getcomponent(char *xlfd,char *pt,int maxlen) {
    char *end = pt+maxlen-1;

    while ( *xlfd!='-' && *xlfd!='\0' && pt<end )
	*pt++ = *xlfd++;
    while ( *xlfd!='-' && *xlfd!='\0' )
	++xlfd;
    *pt = '\0';
return( xlfd );
}

void XLFD_GetComponents(char *xlfd, struct xlfd_components *components) {
    /* can't use sscanf because it fails if %[^-] matches an empty string */

    memset(components,0,sizeof(*components));

    if ( *xlfd=='-' )
	xlfd = getcomponent(xlfd+1,components->foundry,sizeof(components->foundry));
    if ( *xlfd=='-' )
	xlfd = getcomponent(xlfd+1,components->family,sizeof(components->family));
    if ( *xlfd=='-' )
	xlfd = getcomponent(xlfd+1,components->weight,sizeof(components->weight));
    if ( *xlfd=='-' )
	xlfd = getcomponent(xlfd+1,components->slant,sizeof(components->slant));
    if ( *xlfd=='-' )
	xlfd = getcomponent(xlfd+1,components->setwidth,sizeof(components->setwidth));
    if ( *xlfd=='-' )
	xlfd = getcomponent(xlfd+1,components->add_style,sizeof(components->add_style));	
    if ( *xlfd=='-' )
	components->pixel_size = strtol(xlfd+1,&xlfd,10);
    if ( *xlfd=='-' )
	components->point_size = strtol(xlfd+1,&xlfd,10);
    if ( *xlfd=='-' )
	components->res_x = strtol(xlfd+1,&xlfd,10);
    if ( *xlfd=='-' )
	components->res_y = strtol(xlfd+1,&xlfd,10);
    if ( *xlfd=='-' )
	xlfd = getcomponent(xlfd+1,components->spacing,sizeof(components->spacing));	
    if ( *xlfd=='-' )
	components->avg_width = strtol(xlfd+1,&xlfd,10);
    if ( *xlfd=='-' )
	xlfd = getcomponent(xlfd+1,components->cs_reg,sizeof(components->cs_reg));	
    if ( *xlfd=='-' )
	xlfd = getcomponent(xlfd+1,components->cs_enc,sizeof(components->cs_enc));	
}

void XLFD_CreateComponents(BDFFont *font,EncMap *map, int res, struct xlfd_components *components) {
    int avg, cnt, pnt;
    char *mono;
    char family_name[80], weight_name[60], slant[10], stylename[40], squeeze[40];
    char *sffn = *font->sf->fontname ? font->sf->fontname : "Untitled";
    char reg[100], enc[40];
    int old_res;

    mono = AllSame(font,&avg,&cnt);
    old_res = BdfPropHasInt(font,"RESOLUTION_X",-1);
    if ( res!=-1 )
	/* Already set */;
    else if ( old_res>0 )
	res = old_res;
    else if ( font->res>0 )
	res = font->res;
    else if ( font->pixelsize==33 || font->pixelsize==28 || font->pixelsize==17 || font->pixelsize==14 )
	res = 100;
    else
	res = 75;

    pnt = ((font->pixelsize*72+res/2)/res)*10;
    if ( pnt==230 && res==75 )
	pnt = 240;

    decomposename(font, sffn, family_name, weight_name, slant,
	    stylename, squeeze, font->sf->familyname, font->sf->weight);
    def_Charset_Enc(map,reg,enc);
    strncpy(components->foundry,
	    BdfPropHasString(font,"FOUNDRY", font->foundry!=NULL?font->foundry:BDFFoundry==NULL?"FontForge":BDFFoundry),sizeof(components->foundry));
    strncpy(components->family,
	    BdfPropHasString(font,"FAMILY_NAME", family_name),sizeof(components->family));
    strncpy(components->weight,
	    BdfPropHasString(font,"WEIGHT_NAME", weight_name),sizeof(components->weight));
    strncpy(components->slant,
	    BdfPropHasString(font,"SLANT", slant),sizeof(components->slant));
    strncpy(components->setwidth,
	    BdfPropHasString(font,"SETWIDTH_NAME", squeeze),sizeof(components->setwidth));
    strncpy(components->add_style,
	    BdfPropHasString(font,"ADD_STYLE_NAME", stylename),sizeof(components->add_style));
    components->pixel_size = font->pixelsize;
    components->point_size = res==old_res ? BdfPropHasInt(font,"POINT_SIZE", pnt) : pnt;
    components->res_x = res;
    components->res_y = res;
    strncpy(components->spacing,
	    BdfPropHasString(font,"SPACING", mono),sizeof(components->spacing));
    components->avg_width = avg;
    strncpy(components->cs_reg,
	    BdfPropHasString(font,"CHARSET_REGISTRY", reg),sizeof(components->cs_reg));
    strncpy(components->cs_enc,
	    BdfPropHasString(font,"CHARSET_ENCODING", enc),sizeof(components->cs_enc));

    components->foundry[sizeof(components->foundry)-1] = '\0';
    components->family[sizeof(components->family)-1] = '\0';
    components->weight[sizeof(components->weight)-1] = '\0';
    components->slant[sizeof(components->slant)-1] = '\0';
    components->setwidth[sizeof(components->setwidth)-1] = '\0';
    components->add_style[sizeof(components->add_style)-1] = '\0';
    components->spacing[sizeof(components->spacing)-1] = '\0';
    components->cs_reg[sizeof(components->cs_reg)-1] = '\0';
    components->cs_enc[sizeof(components->cs_enc)-1] = '\0';

    components->char_cnt = cnt;
}
    
static void Default_XLFD(BDFFont *bdf,EncMap *map, int res) {
    char buffer[800];
    struct xlfd_components components;

    XLFD_CreateComponents(bdf,map, res, &components);
    snprintf( buffer, sizeof(buffer), "-%s-%s-%s-%s-%s-%s-%d-%d-%d-%d-%s-%d-%s-%s",
	    components.foundry,
	    components.family,
	    components.weight,
	    components.slant,
	    components.setwidth,
	    components.add_style,
	    components.pixel_size,
	    components.point_size,
	    components.res_x,
	    components.res_y,
	    components.spacing,
	    components.avg_width,
	    components.cs_reg,
	    components.cs_enc);
    BDFPropAddString(bdf,"FONT",buffer,NULL);
}

static int BDFPropReplace(BDFFont *bdf,const char *key, const char *value) {
    int i;
    char *pt;

    for ( i=0; i<bdf->prop_cnt; ++i ) if ( strcmp(bdf->props[i].name,key)==0 ) {
	switch ( bdf->props[i].type&~prt_property ) {
	  case prt_string:
	  case prt_atom:
	    free( bdf->props[i].u.atom );
	  break;
	}
	if ( (bdf->props[i].type&~prt_property)!=prt_atom )
	    bdf->props[i].type = (bdf->props[i].type&prt_property)|prt_string;
	pt = strchr(value,'\n');
	if ( pt==NULL )
	    bdf->props[i].u.str = copy(value);
	else
	    bdf->props[i].u.str = copyn(value,pt-value);
return( true );
    }
return( false );
}

void SFReplaceEncodingBDFProps(SplineFont *sf,EncMap *map) {
    BDFFont *bdf;
    char buffer[250];
    char reg[100], enc[40], *pt, *bpt;

    def_Charset_Col(sf,map, buffer);
    def_Charset_Enc(map,reg,enc);

    for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	BDFPropReplace(bdf,"CHARSET_REGISTRY", reg);
	BDFPropReplace(bdf,"CHARSET_ENCODING", enc);
	BDFPropReplace(bdf,"CHARSET_COLLECTIONS",buffer);
	if ( (bpt = BdfPropHasString(bdf,"FONT", NULL))!=NULL ) {
	    strncpy(buffer,bpt,sizeof(buffer)); buffer[sizeof(buffer)-1] = '\0';
	    pt = strrchr(buffer,'-');
	    if ( pt!=NULL ) for ( --pt; pt>buffer && *pt!='-'; --pt );
	    if ( pt>buffer ) {
		sprintf( pt+1, "%s-%s", reg, enc);
		BDFPropReplace(bdf,"FONT",buffer);
	    }
	}
    }
}

void SFReplaceFontnameBDFProps(SplineFont *sf) {
    BDFFont *bdf;
    char *bpt, *pt;
    char buffer2[300];

    for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	BDFPropReplace(bdf,"FONT_NAME", sf->fontname);
	BDFPropReplace(bdf,"FAMILY_NAME", sf->familyname);
	BDFPropReplace(bdf,"FULL_NAME", sf->fullname);
	BDFPropReplace(bdf,"COPYRIGHT", sf->copyright);
	if ( (bpt = BdfPropHasString(bdf,"FONT", NULL))!=NULL ) {
	    if ( bpt[0]=='-' ) {
		for ( pt = bpt+1; *pt && *pt!='-'; ++pt );
		if ( *pt=='-' ) {
		    *pt = '\0';
		    strcpy(buffer2,bpt);
		    strcat(buffer2,"-");
		    strcat(buffer2,sf->familyname);
		    for ( ++pt ; *pt && *pt!='-'; ++pt );
		    strcat(buffer2,pt);
		    BDFPropReplace(bdf,"FONT",buffer2);
		}
	    }
	}
    }
}

static void Default_Properties(BDFFont *bdf,EncMap *map,char *onlyme) {
    char *xlfd = BdfPropHasString(bdf,"FONT",NULL);
    struct xlfd_components components;
    int x_h= -1, cap_h= -1, def_ch=-1, gid;

    if ( (gid=SFFindExistingSlot(bdf->sf,'x',NULL))!=-1 && bdf->glyphs[gid]!=NULL ) {
	x_h = bdf->glyphs[gid]->ymax;
    }
    if ( 'X'<map->enccount && (gid=map->map['X'])!=-1 && bdf->glyphs[gid]!=NULL ) {
	cap_h = bdf->glyphs[gid]->ymax;
    }
    def_ch = SFFindNotdef(bdf->sf,-2);
    if ( def_ch!=-1 ) {
	def_ch = map->map[def_ch];		/* BDF works on the encoding, not on gids, so the default char must be encoded to be useful */
	if ( def_ch>=map->enc->char_cnt )
	    def_ch = -1;
    }

    if ( xlfd!=NULL )
	XLFD_GetComponents(xlfd,&components);
    else
	XLFD_CreateComponents(bdf,map, -1, &components);
    BDFPropAddString(bdf,"FOUNDRY",components.foundry,onlyme);
    BDFPropAddString(bdf,"FAMILY_NAME",components.family,onlyme);
    BDFPropAddString(bdf,"WEIGHT_NAME",components.weight,onlyme);
    BDFPropAddString(bdf,"SLANT",components.slant,onlyme);
    BDFPropAddString(bdf,"SETWIDTH_NAME",components.setwidth,onlyme);
    BDFPropAddString(bdf,"ADD_STYLE_NAME",components.add_style,onlyme);
    BDFPropAddInt(bdf,"PIXEL_SIZE",bdf->pixelsize,onlyme);
    BDFPropAddInt(bdf,"POINT_SIZE",components.point_size,onlyme);
    BDFPropAddInt(bdf,"RESOLUTION_X",components.res_x,onlyme);
    BDFPropAddInt(bdf,"RESOLUTION_Y",components.res_y,onlyme);
    BDFPropAddString(bdf,"SPACING",components.spacing,onlyme);
    BDFPropAddInt(bdf,"AVERAGE_WIDTH",components.avg_width,onlyme);
    BDFPropAddString(bdf,"CHARSET_REGISTRY",components.cs_reg,onlyme);
    BDFPropAddString(bdf,"CHARSET_ENCODING",components.cs_enc,onlyme);

    BDFPropAddString(bdf,"FONTNAME_REGISTRY","",onlyme);
    {
	char buffer[250];
	def_Charset_Col(bdf->sf,map, buffer);
	BDFPropAddString(bdf,"CHARSET_COLLECTIONS", buffer,onlyme );
    }

    if ( bdf->clut!=NULL )
	BDFPropAddInt( bdf, "BITS_PER_PIXEL", BDFDepth(bdf),onlyme);
    BDFPropAddString(bdf,"FONT_NAME",bdf->sf->fontname,onlyme);
    BDFPropAddString(bdf,"FACE_NAME",bdf->sf->fullname,onlyme);	/* Used to be FULL_NAME */
    if ( bdf->sf->copyright==NULL ) {
	char *pt = strchr(bdf->sf->copyright,'\n'), *new;
	if ( pt==NULL )
	    BDFPropAddString(bdf,"COPYRIGHT",bdf->sf->copyright,onlyme);
	else {
	    new = copyn(bdf->sf->copyright,pt-bdf->sf->copyright);
	    BDFPropAddString(bdf,"COPYRIGHT",new,onlyme);
	    free(new);
	}
    }
    if ( bdf->sf->version!=NULL )
	BDFPropAddString(bdf,"FONT_VERSION",bdf->sf->version,onlyme );
    BDFPropAddInt(bdf,"FONT_ASCENT",bdf->ascent,onlyme);
    BDFPropAddInt(bdf,"FONT_DESCENT",bdf->descent,onlyme);
    BDFPropAddInt(bdf,"UNDERLINE_POSITION",
	    (int) rint((bdf->sf->upos*bdf->pixelsize)/(bdf->sf->ascent+bdf->sf->descent)),onlyme);
    BDFPropAddInt(bdf,"UNDERLINE_THICKNESS",
	    (int) ceil((bdf->sf->uwidth*bdf->pixelsize)/(bdf->sf->ascent+bdf->sf->descent)),onlyme);
    
    if ( x_h!=-1 )
	BDFPropAddInt(bdf, "X_HEIGHT", x_h,onlyme );
    if ( cap_h!=-1 )
	BDFPropAddInt(bdf, "CAP_HEIGHT", cap_h,onlyme );
    if ( def_ch!=-1 )
	BDFPropAddInt(bdf, "DEFAULT_CHAR", def_ch,onlyme );
    BDFPropAddInt(bdf,"RAW_ASCENT",bdf->sf->ascent*1000/(bdf->sf->ascent+bdf->sf->descent),onlyme);
    BDFPropAddInt(bdf,"RAW_DESCENT",bdf->sf->descent*1000/(bdf->sf->ascent+bdf->sf->descent),onlyme);
    if ( bdf->sf->italicangle!=0 )
	BDFPropAddInt(bdf,"ITALIC_ANGLE",(int) ((90+bdf->sf->italicangle)*64),onlyme);

    if ( (gid=SFFindExistingSlot(bdf->sf,' ',NULL))!=-1 && bdf->glyphs[gid]!=NULL )
	BDFPropAddInt(bdf,"NORM_SPACE",bdf->glyphs[gid]->width,onlyme);

    if ( onlyme!=NULL ) {
	/* Only generate these oddities if they ask for them... */
	if ( strmatch(onlyme,"QUAD_WIDTH")==0 )		/* Depreciated */
	    BDFPropAddInt(bdf,"QUAD_WIDTH",bdf->pixelsize,onlyme);
	if ( components.res_x==components.res_y )	/* Depreciated */
	    /* This isn't dpi (why not???!), it is 1/100 pixels per point */
	    BDFPropAddInt(bdf,"RESOLUTION",7227/components.res_y,onlyme);
    }

    if ( bdf->sf->pfminfo.pfmset ) {
	/* Only generate these if the user has set them with font info */
	/* OS/2 uses values 0-900. XLFD uses 0-90 */
	BDFPropAddInt(bdf,"RELATIVE_WEIGHT",bdf->sf->pfminfo.weight/10,onlyme);
	BDFPropAddInt(bdf,"RELATIVE_SETWIDTH",bdf->sf->pfminfo.width*10,onlyme);
    }
    if ( bdf->sf->pfminfo.subsuper_set ) {
	BDFPropAddInt(bdf,"SUPERSCRIPT_X",
		bdf->sf->pfminfo.os2_supxoff*bdf->pixelsize/(bdf->sf->ascent+bdf->sf->descent),
		onlyme);
	BDFPropAddInt(bdf,"SUPERSCRIPT_Y",
		bdf->sf->pfminfo.os2_supyoff*bdf->pixelsize/(bdf->sf->ascent+bdf->sf->descent),
		onlyme);
	BDFPropAddInt(bdf,"SUPERSCRIPT_SIZE",
		bdf->sf->pfminfo.os2_supysize*bdf->pixelsize/(bdf->sf->ascent+bdf->sf->descent),
		onlyme);
	BDFPropAddInt(bdf,"SUBSCRIPT_X",
		bdf->sf->pfminfo.os2_subxoff*bdf->pixelsize/(bdf->sf->ascent+bdf->sf->descent),
		onlyme);
	BDFPropAddInt(bdf,"SUBSCRIPT_Y",
		bdf->sf->pfminfo.os2_subyoff*bdf->pixelsize/(bdf->sf->ascent+bdf->sf->descent),
		onlyme);
	BDFPropAddInt(bdf,"SUBSCRIPT_SIZE",
		bdf->sf->pfminfo.os2_subysize*bdf->pixelsize/(bdf->sf->ascent+bdf->sf->descent),
		onlyme);
    }

    {
	char *selection = "0123456789$";
	int width=-1;
	while ( *selection!='\0' ) {
	    if ( (gid=SFFindExistingSlot(bdf->sf,*selection,NULL))!=-1 &&
		    bdf->glyphs[gid]!=NULL ) {
		if ( width==-1 )
		    width = bdf->glyphs[gid]->width;
		else if ( width!=bdf->glyphs[gid]->width )
		    width = -2;
	    }
	    ++selection;
	}
	if ( width!=-2 )
	    BDFPropAddInt(bdf,"FIGURE_WIDTH",width,onlyme);
    }

    {
	int gid;
	int lc_cnt, lc_sum, uc_cnt, uc_sum;
	BDFChar *bdfc;

	lc_cnt = lc_sum = uc_cnt = uc_sum = 0;
	for ( gid = 0; gid<bdf->glyphcnt; ++gid ) if ( (bdfc=bdf->glyphs[gid])!=NULL ) {
	    SplineChar *sc = bdfc->sc;
	    if ( sc->unicodeenc!=-1 && sc->unicodeenc<0x10000 && islower(sc->unicodeenc ) ) {
		++lc_cnt;
		lc_sum += bdfc->width;
	    }
	    if ( sc->unicodeenc!=-1 && sc->unicodeenc<0x10000 && isupper(sc->unicodeenc ) ) {
		++uc_cnt;
		uc_sum += bdfc->width;
	    }
	}
	if ( lc_cnt!=0 )
	    BDFPropAddInt(bdf,"AVG_LOWERCASE_WIDTH",lc_sum*10/lc_cnt,onlyme);
	if ( uc_cnt!=0 )
	    BDFPropAddInt(bdf,"AVG_UPPERCASE_WIDTH",uc_sum*10/uc_cnt,onlyme);
    }

    /* Normally this does nothing, but fontforge may be configured to generate */
    /*  the obsolete _XFREE86_GLYPH_RANGES property, and if so this will */
    /*  generate them */
    GenerateGlyphRanges(bdf);

    /* MIN_SPACE, MAX_SPACE & END_SPACE are judgement calls and I shan't default them */
    /* also SMALL_CAP_SIZE, STRIKEOUT_ASCENT, STRIKEOUT_DESCENT, DESTINATION */
    /* WEIGHT requires a knowlege of stem sizes and I'm not going to do that */
    /*  much analysis */
    /* NOTICE is very similar to copyright. */
    /* FONT_TYPE, RASTERIZER_NAME, RASTERIZER_VERSION */
    /* RAW_* I only provide ASCENT & DESCENT */
    /* AXIS_NAMES, AXIS_LIMITS, AXIS_TYPES for mm fonts */
}

void BDFDefaultProps(BDFFont *bdf, EncMap *map, int res) {
    char *start, *end, *temp;

    bdf->prop_max = bdf->prop_cnt;

    Default_XLFD(bdf,map,res);

    if ( bdf->sf->copyright!=NULL ) {
	start = bdf->sf->copyright;
	while ( (end=strchr(start,'\n'))!=NULL ) {
	    temp = copyn(start,end-start );
	    BDFPropAppendString(bdf,"COMMENT", temp);
	    free( temp );
	    start = end+1;
	}
	if ( *start!='\0' )
	    BDFPropAppendString(bdf,"COMMENT", start );
    }
    Default_Properties(bdf,map,NULL);
}

static BDFProperties *BdfPropsCopy(BDFProperties *props, int cnt ) {
    BDFProperties *ret;
    int i;

    if ( cnt==0 )
return( NULL );
    ret = galloc(cnt*sizeof(BDFProperties));
    memcpy(ret,props,cnt*sizeof(BDFProperties));
    for ( i=0; i<cnt; ++i ) {
	ret[i].name = copy(ret[i].name);
	if ( (ret[i].type&~prt_property)==prt_string || (ret[i].type&~prt_property)==prt_atom )
	    ret[i].u.str = copy(ret[i].u.str);
    }
return( ret );
}

/* ************************************************************************** */
/* ************                    Dialog                    **************** */
/* ************************************************************************** */

#define CID_Delete	1001
#define CID_DefAll	1002
#define CID_DefCur	1003
#define CID_Up		1004
#define CID_Down	1005

#define CID_OK		1010
#define CID_Cancel	1011

static void BdfP_RefigureScrollbar(struct bdf_dlg *bd) {
    struct bdf_dlg_font *cur = bd->cur;
    int lines = bd->vheight/(bd->fh+1);

    /* allow for one extra line which will display "New" */
    GScrollBarSetBounds(bd->vsb,0,cur->bdf->prop_cnt+1,lines);
    if ( cur->top_prop+lines>cur->bdf->prop_cnt+1 )
	cur->top_prop = cur->bdf->prop_cnt+1-lines;
    if ( cur->top_prop < 0 )
	cur->top_prop = 0;
    GScrollBarSetPos(bd->vsb,cur->top_prop);
}

static void BdfP_EnableButtons(struct bdf_dlg *bd) {
    struct bdf_dlg_font *cur = bd->cur;

    if ( cur->sel_prop<0 || cur->sel_prop>=cur->bdf->prop_cnt ) {
	GGadgetSetEnabled(GWidgetGetControl(bd->gw,CID_Delete),false);
	GGadgetSetEnabled(GWidgetGetControl(bd->gw,CID_DefCur),false);
	GGadgetSetEnabled(GWidgetGetControl(bd->gw,CID_Up),false);
	GGadgetSetEnabled(GWidgetGetControl(bd->gw,CID_Down),false);
    } else {
	GGadgetSetEnabled(GWidgetGetControl(bd->gw,CID_Delete),true);
	GGadgetSetEnabled(GWidgetGetControl(bd->gw,CID_DefCur),KnownProp(cur->bdf->props[cur->sel_prop].name));
	GGadgetSetEnabled(GWidgetGetControl(bd->gw,CID_Up),cur->sel_prop>0);
	GGadgetSetEnabled(GWidgetGetControl(bd->gw,CID_Down),cur->sel_prop<cur->bdf->prop_cnt-1);
    }
}

static void BdfP_HideTextField(struct bdf_dlg *bd) {
    if ( bd->active ) {
	bd->active = false;
	GGadgetSetVisible(bd->tf,false);
    }
}

static int BdfP_FinishTextField(struct bdf_dlg *bd) {
    if ( bd->active ) {
	char *text = GGadgetGetTitle8(bd->tf);
	char *pt, *end;
	int val;
	struct bdf_dlg_font *cur = bd->cur;
	BDFFont *bdf = cur->bdf;

	for ( pt=text; *pt; ++pt )
	    if ( *pt&0x80 ) {
		gwwv_post_error(_("Not ASCII"),_("All characters in the value must be in ASCII"));
		free(text);
return( false );
	    }
	val = strtol(text,&end,10);
	if ( NumericKey(bdf->props[cur->sel_prop].name) ) {
	    if ( *end!='\0' ) {
		gwwv_post_error(_("Bad Number"),_("Must be a number"));
		free(text);
return( false );
	    }
	}
	if ( (bdf->props[cur->sel_prop].type&~prt_property)==prt_string ||
		(bdf->props[cur->sel_prop].type&~prt_property)==prt_atom )
	    free(bdf->props[cur->sel_prop].u.str);
	if ( UnknownKey(bdf->props[cur->sel_prop].name) ) {
	    if ( *end!='\0' ) {
		bdf->props[cur->sel_prop].type = prt_string | prt_property;
		bdf->props[cur->sel_prop].u.str = copy(text);
	    } else {
		if ( bdf->props[cur->sel_prop].type != (prt_uint | prt_property ))
		    bdf->props[cur->sel_prop].type = prt_int | prt_property;
		bdf->props[cur->sel_prop].u.val = val;
	    }
	} else if ( NumericKey(bdf->props[cur->sel_prop].name) ) {
	    bdf->props[cur->sel_prop].type = KeyType(bdf->props[cur->sel_prop].name);
	    bdf->props[cur->sel_prop].u.val = val;
	} else {
	    bdf->props[cur->sel_prop].type = KeyType(bdf->props[cur->sel_prop].name);
	    bdf->props[cur->sel_prop].u.str = copy(text);
	}
	free(text);	    
	bd->active = false;
	GGadgetSetVisible(bd->tf,false);
    }
return( true );
}

static int BdfP_DefaultAll(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct bdf_dlg *bd = GDrawGetUserData(GGadgetGetWindow(g));
	int res = BdfPropHasInt(bd->cur->bdf,"RESOLUTION_Y",-1);
	if ( res!=-1 )
	    bd->cur->bdf->res = res;
	BdfP_HideTextField(bd);
	BDFPropsFree(bd->cur->bdf);
	bd->cur->bdf->prop_cnt = bd->cur->bdf->prop_max = 0;
	bd->cur->bdf->props = NULL;
	BDFDefaultProps(bd->cur->bdf, bd->map, -1);
	bd->cur->top_prop = 0; bd->cur->sel_prop = -1;
	BdfP_RefigureScrollbar(bd);
	BdfP_EnableButtons(bd);
	GDrawRequestExpose(bd->v,NULL,false);
    }
return( true );
}

static void _BdfP_DefaultCurrent(struct bdf_dlg *bd) {
    struct bdf_dlg_font *cur = bd->cur;
    BDFFont *bdf = cur->bdf;
    if ( cur->sel_prop<0 || cur->sel_prop>=bdf->prop_cnt )
return;
    BdfP_HideTextField(bd);
    if ( strcmp(bdf->props[cur->sel_prop].name,"FONT")==0 ) {
	Default_XLFD(bdf,bd->map,-1);
    } else if ( strcmp(bdf->props[cur->sel_prop].name,"COMMENT")==0 )
return;
    else
	Default_Properties(bdf,bd->map,bdf->props[cur->sel_prop].name);
    GDrawRequestExpose(bd->v,NULL,false);
return;
}

static int BdfP_DefaultCurrent(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct bdf_dlg *bd = GDrawGetUserData(GGadgetGetWindow(g));
	_BdfP_DefaultCurrent(bd);
    }
return( true );
}

static int BdfP_DeleteCurrent(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct bdf_dlg *bd = GDrawGetUserData(GGadgetGetWindow(g));
	struct bdf_dlg_font *cur = bd->cur;
	BDFFont *bdf = cur->bdf;
	int i;
	if ( cur->sel_prop<0 || cur->sel_prop>=bdf->prop_cnt )
return( true );
	BdfP_HideTextField(bd);
	if ( (bdf->props[cur->sel_prop].type&~prt_property)==prt_string ||
		(bdf->props[cur->sel_prop].type&~prt_property)==prt_atom )
	    free(bdf->props[cur->sel_prop].u.str);
	free(bdf->props[cur->sel_prop].name);
	--bdf->prop_cnt;
	for ( i=cur->sel_prop; i<bdf->prop_cnt; ++i )
	    bdf->props[i] = bdf->props[i+1];
	if ( cur->sel_prop >= bdf->prop_cnt )
	    --cur->sel_prop;
	BdfP_RefigureScrollbar(bd);
	BdfP_EnableButtons(bd);
	GDrawRequestExpose(bd->v,NULL,false);
    }
return( true );
}

static void _BdfP_Up(struct bdf_dlg *bd) {
    struct bdf_dlg_font *cur = bd->cur;
    BDFFont *bdf = cur->bdf;
    GRect r;
    BDFProperties prop;
    if ( cur->sel_prop<1 || cur->sel_prop>=bdf->prop_cnt )
return;
    prop = bdf->props[cur->sel_prop];
    bdf->props[cur->sel_prop] = bdf->props[cur->sel_prop-1];
    bdf->props[cur->sel_prop-1] = prop;
    --cur->sel_prop;
    GGadgetGetSize(bd->tf,&r);
    GGadgetMove(bd->tf,r.x,r.y-(bd->fh+1));
    BdfP_EnableButtons(bd);
    GDrawRequestExpose(bd->v,NULL,false);
}

static int BdfP_Up(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	_BdfP_Up( GDrawGetUserData(GGadgetGetWindow(g)));
    }
return( true );
}

static void _BdfP_Down(struct bdf_dlg *bd) {
    struct bdf_dlg_font *cur = bd->cur;
    BDFFont *bdf = cur->bdf;
    GRect r;
    BDFProperties prop;
    if ( cur->sel_prop<0 || cur->sel_prop>=bdf->prop_cnt-1 )
return;
    prop = bdf->props[cur->sel_prop];
    bdf->props[cur->sel_prop] = bdf->props[cur->sel_prop+1];
    bdf->props[cur->sel_prop+1] = prop;
    ++cur->sel_prop;
    GGadgetGetSize(bd->tf,&r);
    GGadgetMove(bd->tf,r.x,r.y+(bd->fh+1));
    BdfP_EnableButtons(bd);
    GDrawRequestExpose(bd->v,NULL,false);
return;
}

static int BdfP_Down(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	_BdfP_Down((struct bdf_dlg *) GDrawGetUserData(GGadgetGetWindow(g)));
    }
return( true );
}

static int BdfP_ChangeBDF(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	struct bdf_dlg *bd = GDrawGetUserData(GGadgetGetWindow(g));
	int sel = GGadgetGetFirstListSelectedItem(g);
	if ( sel<0 || sel>=bd->fcnt )
return( true );
	if ( !BdfP_FinishTextField(bd)) {
	    sel = bd->cur-bd->fonts;
	    GGadgetSelectListItem(g,sel,true);
return( true );
	}
	bd->cur = &bd->fonts[sel];
	BdfP_RefigureScrollbar(bd);
	BdfP_EnableButtons(bd);
	GDrawRequestExpose(bd->v,NULL,false);
    }
return( true );
}

static void BdfP_DoCancel(struct bdf_dlg *bd) {
    int i;
    for ( i=0; i<bd->fcnt; ++i ) {
	BDFFont *bdf = bd->fonts[i].bdf;
	BDFPropsFree(bdf);
	bdf->props = bd->fonts[i].old_props;
	bdf->prop_cnt = bd->fonts[i].old_prop_cnt;
    }
    free(bd->fonts);
    bd->done = true;
}

static int BdfP_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct bdf_dlg *bd = GDrawGetUserData(GGadgetGetWindow(g));
	BdfP_DoCancel(bd);
    }
return( true );
}

static int BdfP_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct bdf_dlg *bd = GDrawGetUserData(GGadgetGetWindow(g));
	int i;
	struct xlfd_components components;
	char *xlfd;

	if ( !BdfP_FinishTextField(bd))
return( true );
	for ( i=0; i<bd->fcnt; ++i ) {
	    BDFFont *bdf = bd->fonts[i].bdf;
	    BDFProperties *temp = bdf->props;
	    int pc = bdf->prop_cnt;
	    bdf->props = bd->fonts[i].old_props;
	    bdf->prop_cnt = bd->fonts[i].old_prop_cnt;
	    BDFPropsFree(bdf);
	    bdf->props = temp;
	    bdf->prop_cnt = pc;

	    xlfd = BdfPropHasString(bdf,"FONT",NULL);
	    if ( xlfd!=NULL )
		XLFD_GetComponents(xlfd,&components);
	    else
		XLFD_CreateComponents(bdf,bd->map, -1, &components);
	    bdf->res = components.res_y;
	}
	free(bd->fonts);
	bd->sf->changed = true;
	bd->done = true;
    }
return( true );
}

static void BdfP_VScroll(struct bdf_dlg *bd,struct sbevent *sb) {
    int newpos = bd->cur->top_prop;
    int page = bd->vheight/(bd->fh+1);

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= 9*page/10;
      break;
      case et_sb_up:
        newpos--;
      break;
      case et_sb_down:
        newpos++;
      break;
      case et_sb_downpage:
        newpos += 9*page/10;
      break;
      case et_sb_bottom:
        newpos = bd->cur->bdf->prop_cnt+1;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos + page > bd->cur->bdf->prop_cnt+1 )
	newpos = bd->cur->bdf->prop_cnt+1 - page;
    if ( newpos<0 )
	newpos = 0;
    if ( newpos!=bd->cur->top_prop ) {
	int diff = (newpos-bd->cur->top_prop)*(bd->fh+1);
	GRect r;
	bd->cur->top_prop = newpos;
	GScrollBarSetPos(bd->vsb,newpos);
	GGadgetGetSize(bd->tf,&r);
	GGadgetMove(bd->tf,r.x,r.y+diff);
	r.x = 0; r.y = 0; r.width = bd->vwidth; r.height = (bd->vheight/(bd->fh+1))*(bd->fh+1);
	GDrawScroll(bd->v,&r,0,diff);
    }
}

static int _BdfP_VScroll(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_scrollbarchange ) {
	struct bdf_dlg *bd = (struct bdf_dlg *) GDrawGetUserData(GGadgetGetWindow(g));
	BdfP_VScroll(bd,&e->u.control.u.sb);
    }
return( true );
}

static void BdfP_Resize(struct bdf_dlg *bd) {
    extern int _GScrollBar_Width;
    int sbwidth = GDrawPointsToPixels(bd->gw,_GScrollBar_Width);
    GRect size, pos;
    static int cids[] = { CID_Delete, CID_DefAll, CID_DefCur, CID_Up,
	    CID_Down, CID_OK, CID_Cancel, 0 };
    int i;

    GDrawGetSize(bd->gw,&size);
    GDrawGetSize(bd->v,&pos);
    if ( size.width!=bd->width || size.height!=bd->height ) {
	int yoff = size.height-bd->height;
	int xoff = size.width-bd->width;
	bd->width = size.width; bd->height = size.height;
	bd->vwidth += xoff; bd->vheight += yoff;
	GDrawResize(bd->v,bd->vwidth,bd->vheight);
	
	GGadgetMove(bd->vsb,size.width-sbwidth,pos.y-1);
	GGadgetResize(bd->vsb,sbwidth,bd->vheight+2);

	GGadgetGetSize(bd->tf,&pos);
	GGadgetResize(bd->tf,pos.width+xoff,pos.height);

	for ( i=0; cids[i]!=0; ++i ) {
	    GGadgetGetSize(GWidgetGetControl(bd->gw,cids[i]),&pos);
	    GGadgetMove(GWidgetGetControl(bd->gw,cids[i]),pos.x,pos.y+yoff);
	}
    }
    BdfP_RefigureScrollbar(bd);
    GDrawRequestExpose(bd->v,NULL,false);
    GDrawRequestExpose(bd->gw,NULL,true);
}

static void BdfP_Expose(struct bdf_dlg *bd, GWindow pixmap) {
    struct bdf_dlg_font *cur = bd->cur;
    BDFFont *bdf = cur->bdf;
    int i;
    int page = bd->vheight/(bd->fh+1);
    GRect clip, old, r;
    char buffer[40];

    GDrawSetFont(pixmap,bd->font);
    clip.x = 4; clip.width = bd->value_x-4-2; clip.height = bd->fh;
    for ( i=0; i<page && i+cur->top_prop<bdf->prop_cnt; ++i ) {
	int sel = i+cur->top_prop==cur->sel_prop;
	clip.y = i*(bd->fh+1);
	if ( sel ) {
	    r.x = 0; r.width = bd->width;
	    r.y = clip.y; r.height = clip.height;
	    GDrawFillRect(pixmap,&r,0xffff00);
	}
	GDrawPushClip(pixmap,&clip,&old);
	GDrawDrawText8(pixmap,4,i*(bd->fh+1)+bd->as,bdf->props[i+cur->top_prop].name,-1,NULL,0x000000);
	GDrawPopClip(pixmap,&old);
	switch ( bdf->props[i+cur->top_prop].type&~prt_property ) {
	  case prt_string: case prt_atom:
	    GDrawDrawText8(pixmap,bd->value_x+2,i*(bd->fh+1)+bd->as,bdf->props[i+cur->top_prop].u.str,-1,NULL,0x000000);
	  break;
	  case prt_int:
	    sprintf( buffer, "%d", bdf->props[i+cur->top_prop].u.val );
	    GDrawDrawText8(pixmap,bd->value_x+2,i*(bd->fh+1)+bd->as,buffer,-1,NULL,0x000000);
	  break;
	  case prt_uint:
	    sprintf( buffer, "%u", bdf->props[i+cur->top_prop].u.val );
	    GDrawDrawText8(pixmap,bd->value_x+2,i*(bd->fh+1)+bd->as,buffer,-1,NULL,0x000000);
	  break;
	}
	GDrawDrawLine(pixmap,0,i*(bd->fh+1)+bd->fh,bd->vwidth,i*(bd->fh+1)+bd->fh,0x808080);
    }
    if ( i<page ) {
/* GT: I am told that the use of "|" to provide contextual information in a */
/* GT: gettext string is non-standard. However it is documented in section */
/* GT: 10.2.6 of http://www.gnu.org/software/gettext/manual/html_mono/gettext.html */
/* GT: */
/* GT: Anyway here the word "Property" is used to provide context for "New..." and */
/* GT: the translator should only translate "New...". This is necessary because in */
/* GT: French (or any language where adjectives agree in gender/number with their */
/* GT: nouns) there are several different forms of "New" and the one chose depends */
/* GT: on the noun in question. */
/* GT: A french translation might be either msgstr "Nouveau..." or msgstr "Nouvelle..." */
/* GT: */
/* GT: I expect there are more cases where one english word needs to be translated */
/* GT: by several different words in different languages (in Japanese a different */
/* GT: word is used for the latin script and the latin language) and that you, as */
/* GT: a translator may need to ask me to disambiguate more strings. Please do so: */
/* GT:      <pfaedit@users.sourceforge.net> */
	GDrawDrawText8(pixmap,4,i*(bd->fh+1)+bd->as,S_("Property|New..."),-1,NULL,0xff0000);
	GDrawDrawLine(pixmap,0,i*(bd->fh+1)+bd->fh,bd->vwidth,i*(bd->fh+1)+bd->fh,0x808080);
    }
    GDrawDrawLine(pixmap,bd->value_x,0,bd->value_x,bd->vheight,0x808080);
}

static void BdfP_Invoked(GWindow v, GMenuItem *mi, GEvent *e) {
    struct bdf_dlg *bd = (struct bdf_dlg *) GDrawGetUserData(v);
    BDFFont *bdf = bd->cur->bdf;
    char *prop_name = cu_copy(mi->ti.text);
    int sel = bd->cur->sel_prop;
    int i;

    if ( sel>=bdf->prop_cnt ) {
	/* Create a new one */
	if ( bdf->prop_cnt>=bdf->prop_max )
	    bdf->props = grealloc(bdf->props,(bdf->prop_max+=10)*sizeof(BDFProperties));
	sel = bd->cur->sel_prop = bdf->prop_cnt++;
	bdf->props[sel].name = prop_name;
	for ( i=0; StandardProps[i].name!=NULL; ++i )
	    if ( strcmp(prop_name,StandardProps[i].name)==0 )
	break;
	if ( StandardProps[i].name!=NULL ) {
	    bdf->props[sel].type = StandardProps[i].type;
	    if ( (bdf->props[sel].type&~prt_property)==prt_string ||
		    (bdf->props[sel].type&~prt_property)==prt_atom)
		bdf->props[sel].u.str = NULL;
	    else
		bdf->props[sel].u.val = 0;
	    if ( StandardProps[i].defaultable )
		_BdfP_DefaultCurrent(bd);
	    else if ( (bdf->props[sel].type&~prt_property)==prt_string ||
		    (bdf->props[sel].type&~prt_property)==prt_atom)
		bdf->props[sel].u.str = copy("");
	} else {
	    bdf->props[sel].type = prt_property|prt_string;
	    bdf->props[sel].u.str = copy("");
	}
    } else {
	free(bdf->props[sel].name);
	bdf->props[sel].name = prop_name;
    }
    GDrawRequestExpose(bd->v,NULL,false);
}

static void BdfP_PopupMenuProps(struct bdf_dlg *bd, GEvent *e) {
    GMenuItem *mi;
    int i;

    for ( i=0 ; StandardProps[i].name!=NULL; ++i );
    mi = gcalloc(i+3,sizeof(GMenuItem));
    mi[0].ti.text = (unichar_t *) _("No Change");
    mi[0].ti.text_is_1byte = true;
    mi[0].ti.fg = COLOR_DEFAULT;
    mi[0].ti.bg = COLOR_DEFAULT;

    mi[1].ti.line = true;
    mi[1].ti.fg = COLOR_DEFAULT;
    mi[1].ti.bg = COLOR_DEFAULT;
    
    for ( i=0 ; StandardProps[i].name!=NULL; ++i ) {
	mi[i+2].ti.text = (unichar_t *) StandardProps[i].name;	/* These do not translate!!! */
	mi[i+2].ti.text_is_1byte = true;
	mi[i+2].ti.fg = COLOR_DEFAULT;
	mi[i+2].ti.bg = COLOR_DEFAULT;
	mi[i+2].invoke = BdfP_Invoked;
    }

    GMenuCreatePopupMenu(bd->v,e,mi);
    free(mi);
}

static void BdfP_Mouse(struct bdf_dlg *bd, GEvent *e) {
    int line = e->u.mouse.y/(bd->fh+1) + bd->cur->top_prop;

    if ( line<0 || line>bd->cur->bdf->prop_cnt )
return;			/* "New" happens when line==bd->cur->bdf->prop_cnt */
    if ( e->type == et_mousedown ) {
	if ( !BdfP_FinishTextField(bd) ) {
	    bd->press_pos = -1;
return;
	}
	if ( e->u.mouse.x>=4 && e->u.mouse.x<= bd->value_x ) {
	    bd->cur->sel_prop = line;
	    BdfP_PopupMenuProps(bd,e);
	    BdfP_EnableButtons(bd);
	} else if ( line>=bd->cur->bdf->prop_cnt )
return;
	else {
	    bd->press_pos = line;
	    bd->cur->sel_prop = -1;
	    GDrawRequestExpose(bd->v,NULL,false );
	}
    } else if ( e->type == et_mouseup ) {
	int pos = bd->press_pos;
	bd->press_pos = -1;
	if ( line>=bd->cur->bdf->prop_cnt || line!=pos )
return;
	if ( bd->active )		/* Should never happen */
return;
	bd->cur->sel_prop = line;
	if ( e->u.mouse.x > bd->value_x ) {
	    BDFFont *bdf = bd->cur->bdf;
	    GGadgetMove(bd->tf,bd->value_x+2,(line-bd->cur->top_prop)*(bd->fh+1));
	    if ( (bdf->props[line].type&~prt_property) == prt_int ||
		    (bdf->props[line].type&~prt_property) == prt_uint ) {
		char buffer[40];
		sprintf( buffer,"%d",bdf->props[line].u.val );
		GGadgetSetTitle8(bd->tf,buffer);
	    } else
		GGadgetSetTitle8(bd->tf,bdf->props[line].u.str );
	    GGadgetSetVisible(bd->tf,true);
	    bd->active = true;
	}
	GDrawRequestExpose(bd->v,NULL,false );
    }
}

static int BdfP_Char(struct bdf_dlg *bd, GEvent *e) {
    if ( bd->active || bd->cur->sel_prop==-1 )
return( false );
    switch ( e->u.chr.keysym ) {
      case GK_Up: case GK_KP_Up:
	_BdfP_Up(bd);
return( true );
      case GK_Down: case GK_KP_Down:
	_BdfP_Down(bd);
return( true );
      default:
return( false );
    }
}

static int bdfpv_e_h(GWindow gw, GEvent *event) {
    struct bdf_dlg *bd = GDrawGetUserData(gw);
    if ( event->type==et_expose ) {
	BdfP_Expose(bd,gw);
    } else if ( event->type == et_char ) {
return( BdfP_Char(bd,event));
    } else if ( event->type == et_mousedown || event->type == et_mouseup || event->type==et_mousemove ) {
	BdfP_Mouse(bd,event);
	if ( event->type == et_mouseup )
	    BdfP_EnableButtons(bd);
    }
return( true );
}

static int bdfp_e_h(GWindow gw, GEvent *event) {
    struct bdf_dlg *bd = GDrawGetUserData(gw);
    if ( event->type==et_close ) {
	BdfP_DoCancel(bd);
    } else if ( event->type == et_expose ) {
	GRect r;
	GDrawGetSize(bd->v,&r);
	GDrawDrawLine(gw,0,r.y-1,bd->width,r.y-1,0x808080);
	GDrawDrawLine(gw,0,r.y+r.height,bd->width,r.y+r.height,0x808080);
    } else if ( event->type == et_char ) {
return( BdfP_Char(bd,event));
    } else if ( event->type == et_resize ) {
	BdfP_Resize(bd);
    }
return( true );
}

void SFBdfProperties(SplineFont *sf, EncMap *map, BDFFont *thisone) {
    struct bdf_dlg bd;
    int i;
    BDFFont *bdf;
    GTextInfo *ti;
    char buffer[40];
    char title[130];
    GRect pos, subpos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[10];
    GTextInfo label[9];
    FontRequest rq;
    extern int _GScrollBar_Width;
    int sbwidth;
    static unichar_t sans[] = { 'h','e','l','v','e','t','i','c','a',',','c','l','e','a','r','l','y','u',',','u','n','i','f','o','n','t',  '\0' };
    static GBox small = { 0 };
    GGadgetData gd;
    /* I don't use a MatrixEdit here because I want to be able to display */
    /*  non-standard properties. And a MatrixEdit can only disply things in */
    /*  its pull-down list */

    memset(&bd,0,sizeof(bd));
    bd.map = map;
    bd.sf = sf;
    for ( bdf = sf->bitmaps, i=0; bdf!=NULL; bdf=bdf->next, ++i );
    if ( i==0 )
return;
    bd.fcnt = i;
    bd.fonts = gcalloc(i,sizeof(struct bdf_dlg_font));
    bd.cur = &bd.fonts[0];
    for ( bdf = sf->bitmaps, i=0; bdf!=NULL; bdf=bdf->next, ++i ) {
	bd.fonts[i].bdf = bdf;
	bd.fonts[i].old_prop_cnt = bdf->prop_cnt;
	bd.fonts[i].old_props = BdfPropsCopy(bdf->props,bdf->prop_cnt);
	bd.fonts[i].sel_prop = -1;
	bdf->prop_max = bdf->prop_cnt;
	if ( bdf==thisone )
	    bd.cur = &bd.fonts[i];
    }

    ti = gcalloc((i+1),sizeof(GTextInfo));
    for ( bdf = sf->bitmaps, i=0; bdf!=NULL; bdf=bdf->next, ++i ) {
	if ( bdf->clut==NULL )
	    sprintf( buffer, "%d", bdf->pixelsize );
	else
	    sprintf( buffer, "%d@%d", bdf->pixelsize, BDFDepth(bdf));
	ti[i].text = (unichar_t *) copy(buffer);
	ti[i].text_is_1byte = true;
    }
    ti[bd.cur-bd.fonts].selected = true;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    snprintf(title,sizeof(title),_("Strike Information for %.90s"),
	    sf->fontname);
    wattrs.utf8_window_title = title;
    pos.x = pos.y = 0;
    pos.width = GDrawPointsToPixels(NULL,GGadgetScale(268));
    pos.height = GDrawPointsToPixels(NULL,375);
    bd.gw = gw = GDrawCreateTopWindow(NULL,&pos,bdfp_e_h,&bd,&wattrs);

    sbwidth = GDrawPointsToPixels(bd.gw,_GScrollBar_Width);
    subpos.x = 0; subpos.y =  GDrawPointsToPixels(NULL,28);
    subpos.width = pos.width-sbwidth;
    subpos.height = pos.height - subpos.y - GDrawPointsToPixels(NULL,70);
    wattrs.mask = wam_events;
    bd.v = GWidgetCreateSubWindow(gw,&subpos,bdfpv_e_h,&bd,&wattrs);
    bd.vwidth = subpos.width; bd.vheight = subpos.height;
    bd.width = pos.width; bd.height = pos.height;
    bd.value_x = GDrawPointsToPixels(bd.gw,135);

    memset(&rq,0,sizeof(rq));
    rq.family_name = sans;
    rq.point_size = 10;
    rq.weight = 400;
    bd.font = GDrawInstanciateFont(GDrawGetDisplayOfWindow(gw),&rq);
    {
	int as, ds, ld;
	GDrawFontMetrics(bd.font,&as,&ds,&ld);
	bd.as = as; bd.fh = as+ds;
    }

    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));

    i=0;
    gcd[i].gd.pos.x = 10; gcd[i].gd.pos.y = 3;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    gcd[i].gd.u.list = ti;
    gcd[i].gd.handle_controlevent = BdfP_ChangeBDF;
    gcd[i++].creator = GListButtonCreate;

    gcd[i].gd.pos.x = bd.vwidth; gcd[i].gd.pos.y = subpos.y-1;
    gcd[i].gd.pos.width = sbwidth; gcd[i].gd.pos.height = subpos.height+2;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_sb_vert | gg_pos_in_pixels;
    gcd[i].gd.handle_controlevent = _BdfP_VScroll;
    gcd[i++].creator = GScrollBarCreate;

    label[i].text = (unichar_t *) _("Delete");
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 4; gcd[i].gd.pos.y = GDrawPixelsToPoints(gw,subpos.y+subpos.height)+6;
    gcd[i].gd.flags = gg_visible | gg_enabled ;
    gcd[i].gd.handle_controlevent = BdfP_DeleteCurrent;
    gcd[i].gd.cid = CID_Delete;
    gcd[i++].creator = GButtonCreate;

    label[i].text = (unichar_t *) _("Default All");
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 80; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y;
    gcd[i].gd.flags = gg_visible | gg_enabled ;
    gcd[i].gd.handle_controlevent = BdfP_DefaultAll;
    gcd[i].gd.cid = CID_DefAll;
    gcd[i++].creator = GButtonCreate;

    label[i].text = (unichar_t *) _("Default This");
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.y = gcd[i-1].gd.pos.y;
    gcd[i].gd.flags = gg_visible | gg_enabled ;
    gcd[i].gd.handle_controlevent = BdfP_DefaultCurrent;
    gcd[i].gd.cid = CID_DefCur;
    gcd[i++].creator = GButtonCreate;

/* I want the 2 pronged arrow, but gdraw can't find a nice one */
/*  label[i].text = (unichar_t *) "⇑";	*//* Up Arrow */
    label[i].text = (unichar_t *) "↑";	/* Up Arrow */
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.y = gcd[i-1].gd.pos.y;
    gcd[i].gd.flags = gg_visible | gg_enabled ;
    gcd[i].gd.handle_controlevent = BdfP_Up;
    gcd[i].gd.cid = CID_Up;
    gcd[i++].creator = GButtonCreate;

/* I want the 2 pronged arrow, but gdraw can't find a nice one */
/*  label[i].text = (unichar_t *) "⇓";	*//* Down Arrow */
    label[i].text = (unichar_t *) "↓";	/* Down Arrow */
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.y = gcd[i-1].gd.pos.y;
    gcd[i].gd.flags = gg_visible | gg_enabled ;
    gcd[i].gd.handle_controlevent = BdfP_Down;
    gcd[i].gd.cid = CID_Down;
    gcd[i++].creator = GButtonCreate;


    gcd[i].gd.pos.x = 30-3; gcd[i].gd.pos.y = GDrawPixelsToPoints(NULL,pos.height)-32-3;
    gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[i].text = (unichar_t *) _("_OK");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.handle_controlevent = BdfP_OK;
    gcd[i].gd.cid = CID_OK;
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = -30; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+3;
    gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[i].text = (unichar_t *) _("_Cancel");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.handle_controlevent = BdfP_Cancel;
    gcd[i].gd.cid = CID_Cancel;
    gcd[i++].creator = GButtonCreate;
    
    GGadgetsCreate(gw,gcd);
    GTextInfoListFree(gcd[0].gd.u.list);
    bd.vsb = gcd[1].ret;

    small.main_background = small.main_foreground = COLOR_DEFAULT;
    small.main_foreground = 0x0000ff;
    memset(&gd,'\0',sizeof(gd));
    memset(&label[0],'\0',sizeof(label[0]));

    label[0].text = (unichar_t *) "\0";
    label[0].font = bd.font;
    gd.pos.height = bd.fh;
    gd.pos.width = bd.vwidth-bd.value_x;
    gd.label = &label[0];
    gd.box = &small;
    gd.flags = gg_enabled | gg_pos_in_pixels | gg_dontcopybox | gg_text_xim;
    bd.tf = GTextFieldCreate(bd.v,&gd,&bd);

    bd.press_pos = -1;
    BdfP_EnableButtons(&bd);
    BdfP_RefigureScrollbar(&bd);
    
    GDrawSetVisible(bd.v,true);
    GDrawSetVisible(gw,true);
    while ( !bd.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
}
