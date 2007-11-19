/* Copyright (C) 2000-2007 by George Williams */
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
#include <string.h>
#include <ustring.h>
#include <utype.h>

char *BdfPropHasString(BDFFont *font,const char *key, char *def ) {
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

BDFChar *BDFMakeGID(BDFFont *bdf,int gid) {
    SplineFont *sf=bdf->sf;
    SplineChar *sc;
    BDFChar *bc;
    int i;
    extern int use_freetype_to_rasterize_fv;

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
	    bdf->glyphs = grealloc(bdf->glyphs,(bdf->glyphmax=sf->glyphmax)*sizeof(BDFChar *));
	for ( i=bdf->glyphcnt; i<=gid; ++i )
	    bdf->glyphs[i] = NULL;
	bdf->glyphcnt = sf->glyphcnt;
    }
    if ( (bc = bdf->glyphs[gid])==NULL ) {
	if ( use_freetype_to_rasterize_fv ) {
	    void *freetype_context = FreeTypeFontContext(sf,sc,NULL);
	    if ( freetype_context != NULL ) {
		bc = SplineCharFreeTypeRasterize(freetype_context,
			sc->orig_pos,bdf->pixelsize,bdf->clut?8:1);
		FreeTypeFreeContext(freetype_context);
	    }
	}
	if ( bc!=NULL )
	    /* Done */;
	else if ( bdf->clut==NULL )
	    bc = SplineCharRasterize(sc,bdf->pixelsize);
	else
	    bc = SplineCharAntiAlias(sc,bdf->pixelsize,BDFDepth(bdf));
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
