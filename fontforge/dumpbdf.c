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

#include "dumpbdf.h"

#include "bitmapchar.h"
#include "bvedit.h"
#include "encoding.h"
#include "fontforge.h"
#include "splinefont.h"
#include "splinefill.h"
#include "splinesaveafm.h"
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

static void calculate_bounding_box(BDFFont *font,
	int *fbb_width,int *fbb_height,int *fbb_lbearing, int *fbb_descent) {
    int minx=0,maxx=0,miny=0,maxy=0;
    BDFChar *bdfc;
    int i;

    for ( i=0; i<font->glyphcnt; ++i ) {
	if ( (bdfc = font->glyphs[i])!=NULL ) {
	    if ( minx==0 && maxx==0 ) {
		minx = bdfc->xmin;
		maxx = bdfc->xmax;
	    } else {
		if ( minx>bdfc->xmin ) minx=bdfc->xmin;
		if ( maxx<bdfc->xmax ) maxx = bdfc->xmax;
	    }
	    if ( miny==0 && maxy==0 ) {
		miny = bdfc->ymin;
		maxy = bdfc->ymax;
	    } else {
		if ( miny>bdfc->ymin ) miny=bdfc->ymin;
		if ( maxy<bdfc->ymax ) maxy = bdfc->ymax;
	    }
	}
    }
    *fbb_height = maxy-miny+1;
    *fbb_width = maxx-minx+1;
    *fbb_descent = miny;
    *fbb_lbearing = minx;
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
    ff_progress_next();
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
	  default:
	  break;
	}
return( true );
    }
return( false );
}

static void BPSet(BDFFont *font,const char *key,int *val,double scale,
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

static void BDFDumpHeader(FILE *file,BDFFont *font,EncMap *map,
	int res, struct metric_defaults *defs) {
    char temp[200];
    int fbb_height, fbb_width, fbb_descent, fbb_lbearing;
    int pcnt;
    int em = font->sf->ascent + font->sf->descent;
    int i;
    struct xlfd_components components;
    int old_prop_cnt = font->prop_cnt;
    int resolution_mismatch = false;

    if ( old_prop_cnt==0 )
	BDFDefaultProps(font,map,res);

    XLFD_CreateComponents(font,map, res, &components);
    if ( BdfPropHasKey(font,"RESOLUTION_Y",temp,sizeof(temp)) ) {
	if ( components.res_y!=strtol(temp,NULL,0) )
	    resolution_mismatch = true;
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
    if ( !resolution_mismatch && BdfPropHasKey(font,"FONT",temp,sizeof(temp)) )
	fprintf( file, "FONT %s\n", temp );
    else {
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
    }
#if !OLD_GREYMAP_FORMAT
    if ( BdfPropHasKey(font,"SIZE",temp,sizeof(temp)) )
	fprintf( file, "SIZE %s\n", temp );
    else if ( font->clut==NULL )
	fprintf( file, "SIZE %d %d %d\n", components.point_size/10, components.res_x, components.res_y );
    else
	fprintf( file, "SIZE %d %d %d  %d\n", components.point_size/10, components.res_x, components.res_y,
		font->clut->clut_len==256 ? 8 :
		font->clut->clut_len==16 ? 4 : 2);
#else
    fprintf( file, "SIZE %d %d %d\n", components.point_size/10, components.res_x, components.res_y );
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

    for ( i=pcnt=0; i<font->prop_cnt; ++i ) {
	if ( font->props[i].type&prt_property )
	    ++pcnt;
    }
    if ( pcnt!=0 ) {
	fprintf( file, "STARTPROPERTIES %d\n", pcnt );
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
		    if ( resolution_mismatch && (strcmp(font->props[i].name,"RESOLUTION_Y")==0 ||
			    strcmp(font->props[i].name,"RESOLUTION_X")==0 ))
			fprintf( file, "%d\n", components.res_y );
		    else if ( resolution_mismatch && strcmp(font->props[i].name,"POINT_SIZE")==0 )
			fprintf( file, "%d\n", ((font->pixelsize*72+components.res_y/2)/components.res_y)*10 );
		    else
			fprintf( file, "%d\n", font->props[i].u.val );
		  break;
		  default:
		  break;
		}
	    }
	}
    } else {
	fprintf( file, "STARTPROPERTIES %d\n", 15+ (font->clut!=NULL));
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
	fprintf( file, "SPACING \"%s\"\n", components.spacing );
	fprintf( file, "AVERAGE_WIDTH %d\n", components.avg_width );
	if ( font->clut!=NULL )
	    fprintf( file, "BITS_PER_PIXEL %d\n", BDFDepth(font));
	fprintf( file, "CHARSET_REGISTRY \"%s\"\n", components.cs_reg );
	fprintf( file, "CHARSET_ENCODING \"%s\"\n", components.cs_enc );
    }
    fprintf( file, "ENDPROPERTIES\n" );
    { /* AllSame tells us how many glyphs in the font. Which is great for */
      /* many things, but BDF files care about how many encoding slots are */
      /* filled. Usually these are the same, but not if a glyph is used in */
      /* two encodings */
	int i, cnt = 0;
	for ( i=0; i<map->enccount; ++i ) {
	    int gid = map->map[i];
	    if ( gid!=-1 && !IsntBDFChar(font->glyphs[gid]))
		++cnt;
	}
	components.char_cnt = cnt;
    }
    fprintf( file, "CHARS %d\n", components.char_cnt );

    if ( old_prop_cnt==0 ) {
	BDFPropsFree(font);
	font->prop_cnt = 0;
	font->props = NULL;
    }
}

int BDFFontDump(char *filename,BDFFont *font, EncMap *map, int res) {
    char buffer[300];
    FILE *file;
    int i, enc, gid;
    int ret = 0;
    const char *encodingname = EncodingName(map->enc);
    int dups=0;
    struct metric_defaults defs;
    BDFChar *bdfc;

    for ( i=0; i<map->enccount; i++ ) if (( gid=map->map[i])!=-1 && ( bdfc = font->glyphs[gid] ) != NULL )
	BCPrepareForOutput( bdfc,true );
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
	    gid = map->map[i];
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
    for ( i=0; i<map->enccount; i++ ) if (( gid=map->map[i])!=-1 && ( bdfc = font->glyphs[gid] ) != NULL )
	BCRestoreAfterOutput( bdfc );
return( ret );
}
