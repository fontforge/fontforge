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
#include "pfaedit.h"
#include "splinefont.h"
#include <gdraw.h>			/* for the defn of GClut for greymaps */
#include <string.h>
#include <ustring.h>
#include <utype.h>
#include <math.h>

#define MAX_WIDTH	200

int IsntBDFChar(BDFChar *bdfc) {
    if ( bdfc==NULL )
return( true );

return( !SCWorthOutputting(bdfc->sc));
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
/* The XFLD spec also supports RI and RO (reverse skewed italic/oblique) */
    
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

static int GenerateGlyphRanges(BDFFont *font, FILE *file) {
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
	    if ( file!=NULL )
		fprintf(file, "_XFREE86_GLYPH_RANGES \"%s\"\n", buffer );
	    pt = buffer;
	    ++cnt;
	}
	strcpy(pt,add);
	pt += strlen(pt);
    }
    if ( pt!=buffer ) {
	pt[-1] = '\0';		/* was a space */
	if ( file!=NULL )
	    fprintf(file, "_XFREE86_GLYPH_RANGES \"%s\"\n", buffer );
	++cnt;
    }
return( cnt );
#else
return( 0 );
#endif		/* FONTFORGE_CONFIG_BDF_GLYPH_RANGES */
}

static void calculate_bounding_box(BDFFont *font,
	int *fbb_width,int *fbb_height,int *fbb_lbearing, int *fbb_descent) {
    int height=0, width=0, descent=900, lbearing=900; 
    BDFChar *bdfc;
    int i;

    for ( i=0; i<font->glyphcnt; ++i ) {
	if ( (bdfc = font->glyphs[i])!=NULL ) {
	    if ( bdfc->ymax-bdfc->ymin+1>height ) height = bdfc->ymax-bdfc->ymin+1;
	    if ( bdfc->xmax-bdfc->xmin+1>width ) width = bdfc->xmax-bdfc->xmin+1;
	    if ( bdfc->ymin<descent ) descent = bdfc->ymin;
	    if ( bdfc->xmin<lbearing ) lbearing = bdfc->xmin;
	}
    }
    *fbb_height = height;
    *fbb_width = width;
    *fbb_descent = descent;
    *fbb_lbearing = lbearing;
}

#define MS_Hor	1
#define MS_Vert	2
struct metric_defaults {
    int metricssets;	/* 0=>none, 1=>hor, 2=>vert, 3=>hv */
    int swidth;
    int dwidth;
    int swidth1;
    int dwidth1;
};

static void BDFDumpChar(FILE *file,BDFFont *font,BDFChar *bdfc,int enc,
	EncMap *map, int *dups, struct metric_defaults *defs) {
    int r,c;
    int bpl;
    int em = ( font->sf->ascent+font->sf->descent );	/* Just in case em isn't 1000, be prepared to normalize */
    int isdup = false;

    BCCompressBitmap(bdfc);
    if ( bdfc->sc->altuni!=NULL ) {
	int uni = UniFromEnc(enc,map->enc);
	isdup = uni!=bdfc->sc->unicodeenc;
    }
    if ( !isdup )
	fprintf( file, "STARTCHAR %s\n", bdfc->sc->name );
    else
	fprintf( file, "STARTCHAR %s.dup%d\n", bdfc->sc->name, ++*dups );

    fprintf( file, "ENCODING %d\n", enc );
    if ( !(defs->metricssets&MS_Hor) || bdfc->sc->width!=defs->swidth )
	fprintf( file, "SWIDTH %d 0\n", bdfc->sc->width*1000/em );
    if ( !(defs->metricssets&MS_Hor) || bdfc->width!=defs->dwidth )
	fprintf( file, "DWIDTH %d 0\n", bdfc->width );
    if ( font->sf->hasvmetrics ) {
	if ( !(defs->metricssets&MS_Vert) || bdfc->sc->vwidth!=defs->swidth1 )
	    fprintf( file, "SWIDTH1 %d 0\n", bdfc->sc->vwidth*1000/em );
	if ( !(defs->metricssets&MS_Vert) || bdfc->vwidth!=defs->dwidth1 )
	    fprintf( file, "DWIDTH1 %d 0\n", bdfc->vwidth );
	if ( 	/* For CID fonts */
		font->sf->vertical_origin!=bdfc->sc->parent->vertical_origin ) {
		fprintf( file, "VVECTOR %d,%d\n", 500, 1000*bdfc->sc->parent->vertical_origin/
			(bdfc->sc->parent->ascent+bdfc->sc->parent->descent)  );
	}
    }
    fprintf( file, "BBX %d %d %d %d\n", bdfc->xmax-bdfc->xmin+1, bdfc->ymax-bdfc->ymin+1,
	    bdfc->xmin, bdfc->ymin );
    fprintf( file, "BITMAP\n" );
    bpl = bdfc->bytes_per_line;
    for ( r = 0; r<bdfc->ymax-bdfc->ymin+1; ++r ) {
	for ( c=0; c<bpl; ++c ) {
	    if ( font->clut==NULL || font->clut->clut_len==256 ) {
		int n1 = bdfc->bitmap[r*bdfc->bytes_per_line+c]>>4;
		int n2 = bdfc->bitmap[r*bdfc->bytes_per_line+c]&0xf;
		if ( n1>=10 )
		    putc(n1-10+'A',file);
		else
		    putc(n1+'0',file);
		if ( n2>=10 )
		    putc(n2-10+'A',file);
		else
		    putc(n2+'0',file);
	    } else if ( font->clut->clut_len==16 ) {
		int n1 = bdfc->bitmap[r*bdfc->bytes_per_line+c];
		if ( n1>=10 )
		    putc(n1-10+'A',file);
		else
		    putc(n1+'0',file);
	    } else {
		int n1 = bdfc->bitmap[r*bdfc->bytes_per_line+c]<<2;
		if ( c<bpl-1 )
		    n1 += bdfc->bitmap[r*bdfc->bytes_per_line+ ++c];
		if ( n1>=10 )
		    putc(n1-10+'A',file);
		else
		    putc(n1+'0',file);
	    }
	}
	if ( font->clut!=NULL )
	    if ( ( font->clut->clut_len==16 && (bpl&1)) ||
		    (font->clut->clut_len==4 && ((bpl&3)==1 || (bpl&3)==2)) )
		putc('0',file);
	putc('\n',file);
    }
    fprintf( file, "ENDCHAR\n" );
#if defined(FONTFORGE_CONFIG_GDRAW)
    gwwv_progress_next();
#elif defined(FONTFORGE_CONFIG_GTK)
    gwwv_progress_next();
#endif
}

static void figureDefMetrics(BDFFont *font,struct metric_defaults *defs) {
    int i, maxi;
    int width[256], vwidth[256];
    BDFChar *wsc[256], *vsc[256], *bdfc;

    defs->metricssets = MS_Hor|MS_Vert;
    memset(width,0,sizeof(width));
    memset(vwidth,0,sizeof(vwidth));
    for ( i=0; i<font->glyphcnt; ++i ) if ( (bdfc=font->glyphs[i])!=NULL ) {
	if ( bdfc->width<256 ) {
	    ++width[bdfc->width];
	    wsc[bdfc->width] = bdfc;
	}
	if ( bdfc->vwidth<256 ) {
	    ++vwidth[bdfc->vwidth];
	    vsc[bdfc->vwidth] = bdfc;
	}
    }

    maxi=-1;
    for ( i=0; i<256; ++i ) {
	if ( maxi==-1 || width[i]>width[maxi] )
	    maxi = i;
    }
    if ( maxi!=-1 ) {
	defs->metricssets = MS_Hor;
	defs->dwidth = maxi;
	defs->swidth = rint( wsc[maxi]->sc->width * 1000.0 / (font->sf->ascent+font->sf->descent));
    }

    maxi=-1;
    for ( i=0; i<256; ++i ) {
	if ( maxi==-1 || vwidth[i]>vwidth[maxi] )
	    maxi = i;
    }
    if ( maxi!=-1 ) {
	defs->metricssets |= MS_Vert;
	defs->dwidth1 = maxi;
	defs->swidth1 = rint(vsc[maxi]->sc->vwidth * 1000.0 / (font->sf->ascent+font->sf->descent));
    }
}

static int BdfPropHasKey(BDFFont *font,const char *key,char *buffer, int len ) {
    int i;

    for ( i=0; i<font->prop_cnt; ++i ) if ( strcmp(font->props[i].name,key)==0 ) {
	switch ( font->props[i].type&~prt_property ) {
	  case prt_string:
	    snprintf( buffer, len, "\"%s\"", font->props[i].u.str );
	  break;
	  case prt_atom:
	    snprintf( buffer, len, "%s", font->props[i].u.atom );
	  break;
	  case prt_int: case prt_uint:
	    snprintf( buffer, len, "%d", font->props[i].u.val );
	  break;
	}
return( true );
    }
return( false );
}

static char *BdfPropHasString(BDFFont *font,const char *key, char *def ) {
    int i;

    for ( i=0; i<font->prop_cnt; ++i ) if ( strcmp(font->props[i].name,key)==0 ) {
	switch ( font->props[i].type&~prt_property ) {
	  case prt_string:
return( font->props[i].u.str );
	  break;
	  case prt_atom:
return( font->props[i].u.atom );
	  break;
	}
    }
return( def );
}

static int BdfPropHasInt(BDFFont *font,const char *key, int def ) {
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

static void BPSet(BDFFont *font,char *key,int *val,double scale,
	int *metricssets,int flag) {
    int i,value;

    for ( i=0; i<font->prop_cnt; ++i ) if ( strcmp(font->props[i].name,key)==0 ) {
	switch ( font->props[i].type&~prt_property ) {
	  case prt_atom:
	    value = strtol(font->props[i].u.atom,NULL,10);
	  break;
	  case prt_int: case prt_uint:
	    value = font->props[i].u.val;
	  break;
	  default:
return;
	}
	*val = rint( value*scale );
	*metricssets |= flag;
return;
    }
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

void XLFD_GetComponents(char *xlfd, struct xlfd_components *components) {
    sscanf( xlfd, "-%80[^-]-%100[^-]-%80[^-]-%40[^-]-%50[^-]-%50[^-]-%d-%d-%d-%d-%40[^-]-%d-%80[^-]-%80[^-]",
	    components->foundry,
	    components->family,
	    components->weight,
	    components->slant,
	    components->setwidth,
	    components->add_style,
	    &components->pixel_size,
	    &components->point_size,
	    &components->res_x,
	    &components->res_y,
	    components->spacing,
	    &components->avg_width,
	    components->cs_reg,
	    components->cs_enc);
}

void XLFD_CreateComponents(BDFFont *font,EncMap *map, int res, struct xlfd_components *components) {
    int avg, cnt, pnt;
    char *mono;
    char family_name[80], weight_name[60], slant[10], stylename[40], squeeze[40];
    char temp[200];
    char *sffn = *font->sf->fontname ? font->sf->fontname : "Untitled";
    char reg[100], enc[40];
    int old_res;

    mono = AllSame(font,&avg,&cnt);
    old_res = -1;
    if ( BdfPropHasKey(font,"RESOLUTION_X",temp,sizeof(temp)) )
	old_res = strtol(temp,NULL,10);
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

static void BDFDumpHeader(FILE *file,BDFFont *font,EncMap *map,
	int res, struct metric_defaults *defs) {
    char temp[200];
    int x_h= -1, cap_h= -1, def_ch=-1, gid;
    int fbb_height, fbb_width, fbb_descent, fbb_lbearing;
    int glyph_at_zero;
    int pcnt;
    int em = font->sf->ascent + font->sf->descent;
    int i;
    int old_pcnt = font->prop_cnt;
    int has_reg, has_sets;
    struct xlfd_components components;

    XLFD_CreateComponents(font,map, res, &components);

    if ( BdfPropHasKey(font,"RESOLUTION_X",temp,sizeof(temp)) ) {
	if ( components.res_y!=strtol(temp,NULL,0) )
	    font->prop_cnt = 0;		/* Many properties are meaningless if you change the resolution */
    }

    memset(defs,-1,sizeof(*defs));
    defs->metricssets = 0;
    BPSet(font,"SWIDTH",&defs->swidth,1000.0/em,&defs->metricssets,MS_Hor);
    BPSet(font,"DWIDTH",&defs->dwidth,1.0,&defs->metricssets,MS_Hor);
    BPSet(font,"SWIDTH1",&defs->swidth1,1000.0/em,&defs->metricssets,MS_Vert);
    BPSet(font,"DWIDTH1",&defs->dwidth1,1.0,&defs->metricssets,MS_Vert);
    if ( font->sf->hasvmetrics && defs->metricssets==0 )
	figureDefMetrics(font,defs);

  /* Vertical metrics & metrics specified at top level are 2.2 features */
    fprintf( file, font->clut!=NULL ? "STARTFONT 2.3\n" :
		   font->sf->hasvmetrics || defs->metricssets!=0? "STARTFONT 2.2\n" :
		   "STARTFONT 2.1\n" );
    if ( BdfPropHasKey(font,"FONT",temp,sizeof(temp)) )
	fprintf( file, "FONT %s\n", temp );
    else
	fprintf( file, "FONT -%s-%s-%s-%s-%s-%s-%d-%d-%d-%d-%s-%d-%s-%s\n",
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
#if !OLD_GREYMAP_FORMAT
    if ( BdfPropHasKey(font,"SIZE",temp,sizeof(temp)) )
	fprintf( file, "SIZE %s\n", temp );
    else if ( font->clut==NULL )
	fprintf( file, "SIZE %d %d %d\n", components.point_size/10, res, res );
    else
	fprintf( file, "SIZE %d %d %d  %d\n", components.point_size/10, res, res,
		font->clut->clut_len==256 ? 8 :
		font->clut->clut_len==16 ? 4 : 2);
#else
    fprintf( file, "SIZE %d %d %d\n", components.point_size/10, res, res );
#endif
    calculate_bounding_box(font,&fbb_width,&fbb_height,&fbb_lbearing,&fbb_descent);
    fprintf( file, "FONTBOUNDINGBOX %d %d %d %d\n", fbb_width, fbb_height, fbb_lbearing, fbb_descent);

    if ( defs->metricssets!=0 ) {
	if ( (defs->metricssets&MS_Vert) || font->sf->hasvmetrics )
	    fprintf( file, "METRICSSET 2\n" );	/* Both horizontal and vertical metrics */
	if ( defs->swidth!=-1 )
	    fprintf( file, "SWIDTH %d 0\n", defs->swidth );	/* Default advance width value (afm) */
	if ( defs->dwidth!=-1 )
	    fprintf( file, "DWIDTH %d 0\n", defs->dwidth );	/* Default advance width value (pixels) */
	if ( defs->swidth1!=-1 )
	    fprintf( file, "SWIDTH1 %d 0\n", defs->swidth1 );	/* Default advance vwidth value (afm) */
	if ( defs->dwidth1!=-1 )
	    fprintf( file, "DWIDTH1 %d 0\n", defs->dwidth1 );	/* Default advance vwidth value (pixels) */
	if ( font->sf->hasvmetrics || (defs->metricssets&MS_Vert) )
	    fprintf( file, "VVECTOR %d,%d\n", font->pixelsize/2, font->ascent  );
		/* Spec doesn't say if vvector is in afm(S) or pixel(D) units */
		/*  but there is an implication that it is in pixel units */
		/*  offset from horizontal origin to vertical orig */
	if ( defs->swidth!=-1 )
	    defs->swidth = rint( defs->swidth*em/1000.0 );
	if ( defs->swidth1!=-1 )
	    defs->swidth1 = rint( defs->swidth1*em/1000.0 );
    }
    /* the 2.2 spec says we can omit SWIDTH/DWIDTH from character metrics if we */
    /* specify it here. That would make monospaced fonts a lot smaller, but */
    /* that's not in the 2.1 and I worry that some parsers won't be able to */
    /* handle it (mine didn't until just now), so I shan't do that */

    fprintf(file, "COMMENT \"Generated by fontforge, http://fontforge.sourceforge.net\"\n" );
    for ( i=0; i<font->prop_cnt; ++i ) {
	if ( strcmp(font->props[i].name,"COMMENT")==0 &&
		(font->props[i].type==prt_string || font->props[i].type==prt_atom))
	    if ( strstr(font->props[i].u.str,"Generated by fontforge")==NULL )
		fprintf( file, "COMMENT \"%s\"\n", font->props[i].u.str );
    }

    has_reg = has_sets = 0;
    for ( i=pcnt=0; i<font->prop_cnt; ++i ) {
	if ( font->props[i].type&prt_property ) {
	    ++pcnt;
	    if ( strcmp(font->props[i].name,"CHARSET_REGISTRY")==0 )
		has_reg = true;
	    if ( strcmp(font->props[i].name,"CHARSET_COLLECTIONS")==0 )
		has_sets = true;
	}
    }
    if ( pcnt!=0 ) {
	fprintf( file, "STARTPROPERTIES %d\n", pcnt+(!has_reg?2:0)+(!has_sets) );
	for ( i=pcnt=0; i<font->prop_cnt; ++i ) {
	    if ( font->props[i].type&prt_property ) {
		fprintf( file, "%s ", font->props[i].name );
		switch ( font->props[i].type&~prt_property ) {
		  case prt_string:
		    fprintf( file, "\"%s\"\n", font->props[i].u.str );
		  break;
		  case prt_atom:
		    fprintf( file, "%s\n", font->props[i].u.atom );
		  break;
		  case prt_int: case prt_uint:
		    fprintf( file, "%d\n", font->props[i].u.val );
		  break;
		}
	    }
	}
    } else {
	if ( (gid=SFFindExistingSlot(font->sf,'x',NULL))!=-1 && font->glyphs[gid]!=NULL ) {
	    x_h = font->glyphs[gid]->ymax;
	}
	if ( (gid=SFFindExistingSlot(font->sf,'X',NULL))!=-1 && font->glyphs[gid]!=NULL ) {
	    cap_h = font->glyphs[gid]->ymax;
	}
	def_ch = SFFindNotdef(font->sf,-2);
	if ( def_ch!=-1 ) {
	    def_ch = map->map[def_ch];		/* BDF works on the encoding, not on gids, so the default char must be encoded to be useful */
	    if ( def_ch>=map->enc->char_cnt )
		def_ch = -1;
	}
	glyph_at_zero = false;	/* bdftopcf will make the glyph encoded at 0 */
	if ( map->map[0]!=-1 && !IsntBDFChar(font->glyphs[map->map[0]]) )
	    glyph_at_zero = true;	/* be the default glyph if no explicit default*/
		    /* char is given. A default char of -1 means no default */

	fprintf( file, "STARTPROPERTIES %d\n", 24+(x_h!=-1)+(cap_h!=-1)+
		GenerateGlyphRanges(font,NULL)+
		( font->sf->italicangle!=0 ) +
		(def_ch!=-1 || glyph_at_zero)+
		(font->clut!=NULL));
	fprintf( file, "FONT_NAME \"%s\"\n", font->sf->fontname );
	fprintf( file, "FACE_NAME \"%s\"\n", font->sf->fullname );	/* Used to be FULL_NAME */
	fprintf( file, "FONT_ASCENT %d\n", font->ascent );
	fprintf( file, "FONT_DESCENT %d\n", font->descent );
	fprintf( file, "UNDERLINE_POSITION %d\n", 
		(int) ((font->sf->upos*font->pixelsize)/(font->sf->ascent+font->sf->descent)));
	fprintf( file, "UNDERLINE_THICKNESS %d\n", 
	    (int) ((font->sf->uwidth*font->pixelsize)/(font->sf->ascent+font->sf->descent)));
	/* fprintf( file, "QUAD_WIDTH %d\n", font->pixelsize ); */	/* Depreciated */
	if ( x_h!=-1 )
	    fprintf( file, "X_HEIGHT %d\n", x_h );
	if ( cap_h!=-1 )
	    fprintf( file, "CAP_HEIGHT %d\n", cap_h );
	if ( def_ch!=-1 || glyph_at_zero )
	    fprintf( file, "DEFAULT_CHAR %d\n", def_ch );
	if ( font->sf->italicangle!=0 )
	    fprintf( file, "ITALIC_ANGLE %d\n",(int) ((90+font->sf->italicangle)*64));
	fprintf( file, "FONTNAME_REGISTRY \"\"\n" );
	fprintf( file, "FOUNDRY \"%s\"\n", components.foundry );
	fprintf( file, "FAMILY_NAME \"%s\"\n", components.family );
	fprintf( file, "WEIGHT_NAME \"%s\"\n", components.weight );
	fprintf( file, "SLANT \"%s\"\n", components.slant );
	fprintf( file, "SETWIDTH_NAME \"%s\"\n", components.setwidth );
	fprintf( file, "ADD_STYLE_NAME \"%s\"\n", components.add_style );
	fprintf( file, "PIXEL_SIZE %d\n", font->pixelsize );
	fprintf( file, "POINT_SIZE %d\n", components.point_size );
	fprintf( file, "RESOLUTION_X %d\n",components.res_x );
	fprintf( file, "RESOLUTION_Y %d\n",components.res_y );
	fprintf( file, "RESOLUTION %d\n",components.res_y );
	fprintf( file, "SPACING \"%s\"\n", components.spacing );
	fprintf( file, "AVERAGE_WIDTH %d\n", components.avg_width );
	if ( font->clut!=NULL )
	    fprintf( file, "BITS_PER_PIXEL %d\n", BDFDepth(font));

	if ( font->sf->copyright==NULL )
	    fprintf( file, "COPYRIGHT \"\"\n" );
	else {
	    char *pt = strchr(font->sf->copyright,'\n'), *end;
	    if ( pt==NULL )
		fprintf( file, "COPYRIGHT \"%s\"\n", font->sf->copyright );
	    else {
		fprintf( file, "COPYRIGHT \"%.*s\"\n",
			 (int)(pt - font->sf->copyright),
			 font->sf->copyright );
		forever {
		    ++pt;
		    end = strchr(pt,'\n');
		    if ( end==NULL ) {
			fprintf( file, "COMMENT %s\n", pt );
		break;
		    } else
		      fprintf( file, "COMMENT %.*s\n", (int)(end-pt), pt );
		    pt = end;
		}
	    }
	}
	/* Normally this does nothing, but fontforge may be configured to generate */
	/*  the obsolete _XFREE86_GLYPH_RANGES property, and if so this will */
	/*  generate them */
	GenerateGlyphRanges(font,file);
    }
    if ( !has_reg ) {
	fprintf( file, "CHARSET_REGISTRY \"%s\"\n", components.cs_reg );
	fprintf( file, "CHARSET_ENCODING \"%s\"\n", components.cs_enc );
    }

    if ( !has_sets ) {
	char buffer[250];
	def_Charset_Col(font->sf,map, buffer);
	fprintf( file, "CHARSET_COLLECTIONS \"%s\"\n", buffer );
    }
    fprintf( file, "ENDPROPERTIES\n" );
    fprintf( file, "CHARS %d\n", components.char_cnt );

    font->prop_cnt = old_pcnt;
}

int BDFFontDump(char *filename,BDFFont *font, EncMap *map, int res) {
    char buffer[300];
    FILE *file;
    int i, enc;
    int ret = 0;
    char *encodingname = EncodingName(map->enc);
    int dups=0;
    struct metric_defaults defs;

    if ( filename==NULL ) {
	sprintf(buffer,"%s-%s.%d.bdf", font->sf->fontname, encodingname, font->pixelsize );
	filename = buffer;
    }
    file = fopen(filename,"w" );
    if ( file==NULL )
	LogError( _("Can't open %s\n"), filename );
    else {
	BDFDumpHeader(file,font,map,res,&defs);
	for ( i=0; i<map->enccount; ++i ) {
	    int gid = map->map[i];
	    if ( gid!=-1 && !IsntBDFChar(font->glyphs[gid])) {
		enc = i;
		if ( i>=map->enc->char_cnt )	/* The map's enccount may contain "unencoded" glyphs */
		    enc = -1;
		BDFDumpChar(file,font,font->glyphs[gid],enc,map,&dups,&defs);
	    }
	}
	fprintf( file, "ENDFONT\n" );
	if ( ferror(file))
	    LogError( _("Failed to write %s\n"), filename );
	else
	    ret = 1;
	fclose(file);
    }
return( ret );
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
    char reg[100], enc[40], *pt;

    def_Charset_Col(sf,map, buffer);
    def_Charset_Enc(map,reg,enc);

    for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	BDFPropReplace(bdf,"CHARSET_REGISTRY", reg);
	BDFPropReplace(bdf,"CHARSET_ENCODING", enc);
	BDFPropReplace(bdf,"CHARSET_COLLECTIONS",buffer);
	if ( BdfPropHasKey(bdf,"FONT",buffer,sizeof(buffer)) ) {
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
    char buffer[250], *pt;
    char buffer2[300];

    for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	BDFPropReplace(bdf,"FONT_NAME", sf->fontname);
	BDFPropReplace(bdf,"FAMILY_NAME", sf->familyname);
	BDFPropReplace(bdf,"FULL_NAME", sf->fullname);
	BDFPropReplace(bdf,"COPYRIGHT", sf->copyright);
	if ( BdfPropHasKey(bdf,"FONT",buffer,sizeof(buffer)) ) {
	    if ( buffer[0]=='-' ) {
		for ( pt = buffer+1; *pt && *pt!='-'; ++pt );
		if ( *pt=='-' ) {
		    *pt = '\0';
		    strcpy(buffer2,buffer);
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
