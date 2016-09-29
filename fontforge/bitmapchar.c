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

#include "bitmapchar.h"

#include "bvedit.h"
#include "fontforgevw.h"
#include "cvundoes.h"
#include <string.h>
#include <ustring.h>
#include <utype.h>
#include <math.h>

int use_freetype_to_rasterize_fv = 1;
char *BDFFoundry=NULL;

struct std_bdf_props StandardProps[] = {
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
	STD_BDF_PROPS_EMPTY
};

int IsUnsignedBDFKey(const char *key) {
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

const char *BdfPropHasString(BDFFont *font,const char *key, const char *def ) {
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
	  default:
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
	  default:
	  break;
	}
    }
return( def );
}

static void BDFPropAddString(BDFFont *bdf,const char *keyword,const char *value,char *match_key) {
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
	    bdf->props = realloc(bdf->props,(bdf->prop_max+=10)*sizeof(BDFProperties));
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

static void BDFPropAddInt(BDFFont *bdf,const char *keyword,int value,char *match_key) {
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
	    bdf->props = realloc(bdf->props,(bdf->prop_max+=10)*sizeof(BDFProperties));
	++bdf->prop_cnt;
	bdf->props[i].name = copy(keyword);
    }
    if ( IsUnsignedBDFKey(keyword) )
	bdf->props[i].type = prt_uint | prt_property;
    else
	bdf->props[i].type = prt_int | prt_property;
    bdf->props[i].u.val = value;
}

static const char *AllSame(BDFFont *font,int *avg,int *cnt) {
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

static void BDFPropAppendString(BDFFont *bdf,const char *keyword,char *value) {
    int i = bdf->prop_cnt;

    if ( i>=bdf->prop_max )
	bdf->props = realloc(bdf->props,(bdf->prop_max+=10)*sizeof(BDFProperties));
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

static int BDFPropReplace(BDFFont *bdf,const char *key, const char *value) {
    int i;
    char *pt;

    for ( i=0; i<bdf->prop_cnt; ++i ) if ( strcmp(bdf->props[i].name,key)==0 ) {
	switch ( bdf->props[i].type&~prt_property ) {
	  case prt_string:
	  case prt_atom:
	    free( bdf->props[i].u.atom );
	  break;
	  default:
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

void SFReplaceEncodingBDFProps(SplineFont *sf,EncMap *map) {
    BDFFont *bdf;
    char buffer[250];
    char reg[100], enc[40], *pt;
    const char *bpt;

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
	if ( (bpt = copy(BdfPropHasString(bdf,"FONT", NULL)))!=NULL ) {
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

BDFProperties *BdfPropsCopy(BDFProperties *props, int cnt ) {
    BDFProperties *ret;
    int i;

    if ( cnt==0 )
return( NULL );
    ret = malloc(cnt*sizeof(BDFProperties));
    memcpy(ret,props,cnt*sizeof(BDFProperties));
    for ( i=0; i<cnt; ++i ) {
	ret[i].name = copy(ret[i].name);
	if ( (ret[i].type&~prt_property)==prt_string || (ret[i].type&~prt_property)==prt_atom )
	    ret[i].u.str = copy(ret[i].u.str);
    }
return( ret );
}

void def_Charset_Enc(EncMap *map,char *reg,char *enc) {
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

static void decomposename(const char *fontname, char *family_name, char *weight_name,
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
            ( bold = strstr(fontname,"Medi"))==NULL )
        {
            /* Again, URW */
            ;
        }
    
	if (( style = strstr(fontname,"Sans"))==NULL &&
            (compress = strstr(fontname,"SmallCaps"))==NULL )
        {
	    ;
	}
	if ((compress = strstr(fontname,"Extended"))==NULL &&
            (compress = strstr(fontname,"Condensed"))==NULL )
        {
            ;
        }
    
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

static const char *getcomponent(const char *xlfd,char *pt,int maxlen) {
    char *end = pt+maxlen-1;

    while ( *xlfd!='-' && *xlfd!='\0' && pt<end )
	*pt++ = *xlfd++;
    while ( *xlfd!='-' && *xlfd!='\0' )
	++xlfd;
    *pt = '\0';
return( xlfd );
}

void XLFD_GetComponents(const char *xlfd, struct xlfd_components *components) {
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
	components->pixel_size = strtol(xlfd+1,(char **)&xlfd,10);
    if ( *xlfd=='-' )
	components->point_size = strtol(xlfd+1,(char **)&xlfd,10);
    if ( *xlfd=='-' )
	components->res_x = strtol(xlfd+1,(char **)&xlfd,10);
    if ( *xlfd=='-' )
	components->res_y = strtol(xlfd+1,(char **)&xlfd,10);
    if ( *xlfd=='-' )
	xlfd = getcomponent(xlfd+1,components->spacing,sizeof(components->spacing));	
    if ( *xlfd=='-' )
	components->avg_width = strtol(xlfd+1,(char **)&xlfd,10);
    if ( *xlfd=='-' )
	xlfd = getcomponent(xlfd+1,components->cs_reg,sizeof(components->cs_reg));	
    if ( *xlfd=='-' )
	xlfd = getcomponent(xlfd+1,components->cs_enc,sizeof(components->cs_enc));	
}

void XLFD_CreateComponents(BDFFont *font,EncMap *map, int res, struct xlfd_components *components) {
    int avg, cnt, pnt;
    const char *mono;
    char family_name[80], weight_name[60], slant[10], stylename[40], squeeze[40];
    const char *sffn = *font->sf->fontname ? font->sf->fontname : "Untitled";
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

    decomposename(sffn, family_name, weight_name, slant,
	    stylename, squeeze, font->sf->familyname, font->sf->weight);
    def_Charset_Enc(map,reg,enc);
    strncpy(components->foundry,
	    BdfPropHasString(font,"FOUNDRY", font->foundry!=NULL?font->foundry:BDFFoundry==NULL?copy("FontForge"):BDFFoundry),sizeof(components->foundry));
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

void Default_XLFD(BDFFont *bdf,EncMap *map, int res) {
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

void Default_Properties(BDFFont *bdf,EncMap *map,char *onlyme) {
    const char *xlfd = BdfPropHasString(bdf,"FONT",NULL);
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
    if ( bdf->sf->copyright!=NULL ) {
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
	const char *selection = "0123456789$";
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

BDFChar *BDFMakeGID(BDFFont *bdf,int gid) {
    SplineFont *sf=bdf->sf;
    SplineChar *sc;
    BDFChar *bc;
    int i;

    if ( gid==-1 )
return( NULL );

    if ( sf->cidmaster!=NULL || sf->subfonts!=NULL ) {
	int j = SFHasCID(sf,gid);
	if ( sf->cidmaster ) sf = sf->cidmaster;
	if ( j==-1 ) {
	    for ( j=0; j<sf->subfontcnt; ++j )
		if ( gid<sf->subfonts[j]->glyphcnt )
	    break;
	    if ( j==sf->subfontcnt )
return( NULL );
	}
	sf = sf->subfonts[j];
    }
    if ( (sc = sf->glyphs[gid])==NULL )
return( NULL );

    if ( gid>=bdf->glyphcnt ) {
	if ( gid>=bdf->glyphmax )
	    bdf->glyphs = realloc(bdf->glyphs,(bdf->glyphmax=sf->glyphmax)*sizeof(BDFChar *));
	for ( i=bdf->glyphcnt; i<=gid; ++i )
	    bdf->glyphs[i] = NULL;
	bdf->glyphcnt = sf->glyphcnt;
    }
    if ( (bc = bdf->glyphs[gid])==NULL ) {
	if ( use_freetype_to_rasterize_fv ) {
	    void *freetype_context = FreeTypeFontContext(sf,sc,NULL,ly_fore);
	    if ( freetype_context != NULL ) {
		bc = SplineCharFreeTypeRasterize(freetype_context,
			sc->orig_pos,bdf->pixelsize,72,bdf->clut?8:1);
		FreeTypeFreeContext(freetype_context);
	    }
	}
	if ( bc!=NULL )
	    /* Done */;
	else if ( bdf->clut==NULL )
	    bc = SplineCharRasterize(sc,ly_fore,bdf->pixelsize);
	else
	    bc = SplineCharAntiAlias(sc,ly_fore,bdf->pixelsize,BDFDepth(bdf));
	bdf->glyphs[gid] = bc;
	bc->orig_pos = gid;
	BCCharChangedUpdate(bc);
    }
return( bc );
}

BDFChar *BDFMakeChar(BDFFont *bdf,EncMap *map,int enc) {
    SplineFont *sf=bdf->sf;

    if ( enc==-1 )
return( NULL );

    if ( sf->cidmaster!=NULL ) {
	int j = SFHasCID(sf,enc);
	sf = sf->cidmaster;
	if ( j==-1 ) {
	    for ( j=0; j<sf->subfontcnt; ++j )
		if ( enc<sf->subfonts[j]->glyphcnt )
	    break;
	    if ( j==sf->subfontcnt )
return( NULL );
	}
	sf = sf->subfonts[j];
    }
    SFMakeChar(sf,map,enc);
return( BDFMakeGID(bdf,map->map[enc]));
}

void BCClearAll(BDFChar *bc) {
    BDFRefChar *head,*cur;
    
    if ( bc==NULL )
return;
    for ( head = bc->refs; head != NULL; ) {
	cur = head; head = head->next; free( cur );
    }
    bc->refs = NULL;
    
    BCPreserveState(bc);
    BCFlattenFloat(bc);
    memset(bc->bitmap,'\0',bc->bytes_per_line*(bc->ymax-bc->ymin+1));
    BCCompressBitmap(bc);
    bc->xmin = 0; bc->xmax = 0;
    bc->ymin = 0; bc->ymax = 0;
    BCCharChangedUpdate(bc);
}

static void BCChngNoUpdate(BDFChar *bc) {
    bc->changed = true;
    bc->sc->parent->changed = true;
}

static void BCNoUpdate(BDFChar *UNUSED(bc)) {
}

static void BCNothingDestroyed(BDFChar *UNUSED(bc)) {
}

static struct bc_interface noui_bc = {
    BCChngNoUpdate,
    BCNoUpdate,
    BCNothingDestroyed
};

struct bc_interface *bc_interface = &noui_bc;

void FF_SetBCInterface(struct bc_interface *bci) {
    bc_interface = bci;
}
