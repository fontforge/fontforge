/* Copyright (C) 2000-2005 by George Williams */
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

#define MAX_WIDTH	200

int IsntBDFChar(BDFChar *bdfc) {
    if ( bdfc==NULL )
return( true );

    if ( bdfc->sc->parent->onlybitmaps )
return( bdfc->enc!=0 && strcmp(bdfc->sc->name,".notdef")==0 );

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
    
	if (( bold = strstr(fontname,"Bold"))==NULL &&
		( bold = strstr(fontname,"Ligh"))==NULL &&
		( bold = strstr(fontname,"Demi"))==NULL &&
		( bold = strstr(fontname,"Blac"))==NULL &&
		( bold = strstr(fontname,"Roma"))==NULL &&
		( bold = strstr(fontname,"Book"))==NULL &&
		( bold = strstr(fontname,"Regu"))==NULL &&
		( bold = strstr(fontname,"Medi"))==NULL );	/* Again, URW */
    
	if (( style = strstr(fontname,"Sans"))==NULL );
	if ((compress = strstr(fontname,"Extended"))==NULL &&
		(compress = strstr(fontname,"Condensed"))==NULL &&
		(compress = strstr(fontname,"SmallCaps"))==NULL );
    
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

static int AllSame(BDFFont *font,int *avg,int *cnt) {
    int c=0, a=0, common= -1;
    int i;
    BDFChar *bdfc;

    for ( i=0; i<font->charcnt; ++i ) {
	if ( (bdfc = font->chars[i])!=NULL && !IsntBDFChar(bdfc)) {
	    ++c;
	    a += bdfc->width;
	    if ( common==-1 ) common = bdfc->width; else if ( common!=bdfc->width ) common = -2;
	}
    }
    if ( c==0 ) *avg=0; else *avg = (10*a)/c;
    *cnt = c;
return(common!=-2);
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

    if ( font->encoding_name->is_unicodebmp || font->encoding_name->is_unicodefull )
	max = font->encoding_name->char_cnt;
    else
return( 0 );
    pt = buffer; end = pt+sizeof(buffer);
    for ( i=0; i<font->charcnt && i<max; ++i ) if ( !IsntBDFChar(font->chars[i]) ) {
	for ( j=i+1; j<font->charcnt && j<max && !IsntBDFChar(font->chars[j]); ++j );
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

    for ( i=0; i<font->charcnt; ++i ) {
	if ( (bdfc = font->chars[i])!=NULL ) {
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

static void BDFDumpChar(FILE *file,BDFFont *font,BDFChar *bdfc,int enc) {
    int r,c;
    int bpl;
    int em = ( font->sf->ascent+font->sf->descent );	/* Just in case em isn't 1000, be prepared to normalize */

    BCCompressBitmap(bdfc);
    if ( bdfc->enc==0xa0 && strcmp(bdfc->sc->name,"space")==0 )
	fprintf( file, "STARTCHAR %s\n", "nonbreakingspace" );
    else if ( bdfc->enc==0xad && strcmp(bdfc->sc->name,"hyphen")==0 )
	fprintf( file, "STARTCHAR %s\n", "softhyphen" );
    else
	fprintf( file, "STARTCHAR %s\n", bdfc->sc->name );
    fprintf( file, "ENCODING %d\n", enc );
    if ( !font->sf->hasvmetrics || bdfc->sc->width!=em ) {
	fprintf( file, "SWIDTH %d 0\n", bdfc->sc->width*1000/em );
	fprintf( file, "DWIDTH %d 0\n", bdfc->width );
    }
    if ( font->sf->hasvmetrics && bdfc->sc->vwidth!=em ) {
	fprintf( file, "SWIDTH1 %d 0\n", bdfc->sc->vwidth*1000/em );
	fprintf( file, "DWIDTH1 %d 0\n", (bdfc->sc->vwidth*font->pixelsize+em/2)/em );
	if ( font->sf->vertical_origin!=bdfc->sc->parent->vertical_origin )	/* For CID fonts */
	    fprintf( file, "VVECTOR %d,%d\n", 500, 1000*bdfc->sc->parent->vertical_origin/
		    (bdfc->sc->parent->ascent+bdfc->sc->parent->descent)  );
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
    GProgressNext();
#elif defined(FONTFORGE_CONFIG_GTK)
    gwwv_progress_next();
#endif
}

static void BDFDumpHeader(FILE *file,BDFFont *font,char *encoding, int res) {
    int avg, cnt, pnt;
    char *mono;
    char family_name[80], weight_name[60], slant[10], stylename[40], squeeze[40];
    char buffer[400];
    int x_h= -1, cap_h= -1, def_ch=-1;
    char fontname[300];
    int fbb_height, fbb_width, fbb_descent, fbb_lbearing;
    char *pt;
    uint32 codepages[2];
    int i;
    char *sffn = *font->sf->fontname ? font->sf->fontname : "Untitled";

    if ( AllSame(font,&avg,&cnt))
	mono="M";
    else
	mono="P";
    if ( res!=-1 )
	/* Already set */;
    else if ( font->pixelsize==33 || font->pixelsize==28 || font->pixelsize==17 || font->pixelsize==14 )
	res = 100;
    else
	res = 75;

    pnt = ((font->pixelsize*72+res/2)/res)*10;
    if ( pnt==230 && res==75 )
	pnt = 240;

    if ( strcmp(encoding,"ISOLatin1Encoding")==0 )
	encoding = "ISO8859-1";

    decomposename(font, sffn, family_name, weight_name, slant,
	    stylename, squeeze, font->sf->familyname, font->sf->weight);
    if ( *font->sf->fontname=='-' ) {
	strcpy(buffer,font->sf->fontname);
	sprintf(fontname,"%s%s%s%s%s", family_name, stylename,
		strcmp(weight_name,"Medium")==0?"":weight_name,
		*slant=='I'?"Italic":*slant=='O'?"Oblique":"",
		squeeze);
    } else {
	sprintf( buffer, "-%s-%s-%s-%s-%s-%s-%d-%d-%d-%d-%s-%d-%s",
	    font->foundry!=NULL?font->foundry:BDFFoundry==NULL?"FontForge":BDFFoundry,
	    family_name, weight_name, slant, squeeze, stylename,
	    font->pixelsize, pnt, res, res, mono, avg, encoding );
	if ( strchr(encoding,'-')==NULL )
	    strcat(buffer,"-0");
	strcpy(fontname,font->sf->fontname);
    }

  /* Vertical metrics & metrics specified at top level are 2.2 features */
    fprintf( file, font->clut!=NULL ? "STARTFONT 2.3\n" :
		   font->sf->hasvmetrics ? "STARTFONT 2.2\n" :
		   "STARTFONT 2.1\n" );
    fprintf( file, "FONT %s\n", buffer );	/* FONT ... */
#if !OLD_GREYMAP_FORMAT
    if ( font->clut==NULL )
	fprintf( file, "SIZE %d %d %d\n", pnt/10, res, res );
    else
	fprintf( file, "SIZE %d %d %d  %d\n", pnt/10, res, res,
		font->clut->clut_len==256 ? 8 :
		font->clut->clut_len==16 ? 4 : 2);
#else
    fprintf( file, "SIZE %d %d %d\n", pnt/10, res, res );
#endif
    calculate_bounding_box(font,&fbb_width,&fbb_height,&fbb_lbearing,&fbb_descent);
    fprintf( file, "FONTBOUNDINGBOX %d %d %d %d\n", fbb_width, fbb_height, fbb_lbearing, fbb_descent);
    if ( font->sf->hasvmetrics ) {
	fprintf( file, "METRICSSET 2\n" );	/* Both horizontal and vertical metrics */
	fprintf( file, "SWIDTH 1000\n" );	/* Default advance width value */
	fprintf( file, "DWIDTH %d\n", font->pixelsize ); /* Default advance width value */
	fprintf( file, "SWIDTH1 1000\n" );	/* Default advance width value */
	fprintf( file, "DWIDTH1 %d\n", font->pixelsize ); /* Default advance width value */
	fprintf( file, "VVECTOR %d,%d\n", 500, 1000*font->sf->vertical_origin/
		(font->sf->ascent+font->sf->descent)  );
		/* Spec doesn't say if vvector is in scaled or unscaled units */
		/*  offset from horizontal origin to vertical orig */
    }
    /* the 2.2 spec says we can omit SWIDTH/DWIDTH from character metrics if we */
    /* specify it here. That would make monospaced fonts a lot smaller, but */
    /* that's not in the 2.1 and I worry that some parsers won't be able to */
    /* handle it (mine didn't until just now), so I shan't do that */

    fprintf(file, "COMMENT \"Generated by fontforge, http://fontforge.sourceforge.net\"\n" );

    if ( 'x'<font->charcnt && font->chars['x']!=NULL ) {
	x_h = font->chars['x']->ymax;
    }
    if ( 'X'<font->charcnt && font->chars['X']!=NULL ) {
	cap_h = font->chars['X']->ymax;
    }
    if ( (font->sf->chars[0]!=NULL &&
	    (font->sf->chars[0]->layers[ly_fore].splines!=NULL || (font->sf->chars[0]->widthset && *mono!='M')) &&
	     font->sf->chars[0]->layers[ly_fore].refs==NULL && strcmp(font->sf->chars[0]->name,".notdef")==0 ) )
	def_ch = 0;

    fprintf( file, "STARTPROPERTIES %d\n", 25+(x_h!=-1)+(cap_h!=-1)+
	    GenerateGlyphRanges(font,NULL)+
	    (def_ch!=-1)+(font->clut!=NULL));
    fprintf( file, "FONT_NAME \"%s\"\n", font->sf->fontname );
    fprintf( file, "FONT_ASCENT %d\n", font->ascent );
    fprintf( file, "FONT_DESCENT %d\n", font->descent );
    fprintf( file, "UNDERLINE_POSITION %d\n", (int) font->sf->upos );
    fprintf( file, "UNDERLINE_THICKNESS %d\n", (int) font->sf->uwidth );
    fprintf( file, "QUAD_WIDTH %d\n", font->pixelsize );
    if ( x_h!=-1 )
	fprintf( file, "X_HEIGHT %d\n", x_h );
    if ( cap_h!=-1 )
	fprintf( file, "CAP_HEIGHT %d\n", cap_h );
    if ( def_ch!=-1 )
	fprintf( file, "DEFAULT_CHAR %d\n", def_ch );
    fprintf( file, "FONTNAME_REGISTRY \"\"\n" );
    fprintf( file, "FAMILY_NAME \"%s\"\n", family_name );
    fprintf( file, "FOUNDRY \"%s\"\n", font->foundry!=NULL?font->foundry:BDFFoundry==NULL?"FontForge":BDFFoundry );
    fprintf( file, "WEIGHT_NAME \"%s\"\n", weight_name );
    fprintf( file, "SETWIDTH_NAME \"%s\"\n", squeeze );
    fprintf( file, "SLANT \"%s\"\n", slant );
    fprintf( file, "ADD_STYLE_NAME \"%s\"\n", stylename );
    fprintf( file, "PIXEL_SIZE %d\n", font->pixelsize );
    fprintf( file, "POINT_SIZE %d\n", pnt );
    fprintf( file, "RESOLUTION_X %d\n",res );
    fprintf( file, "RESOLUTION_Y %d\n",res );
    fprintf( file, "RESOLUTION %d\n",res );
    fprintf( file, "SPACING \"%s\"\n", mono );
    fprintf( file, "AVERAGE_WIDTH %d\n", avg );
    if ( font->clut!=NULL )
	fprintf( file, "BITS_PER_PIXEL %d\n", BDFDepth(font));
    if ( font->encoding_name->is_custom || font->encoding_name->is_original )
	fprintf( file, "CHARSET_REGISTRY \"FontSpecific\"\n" );
    else if ( strstr(font->encoding_name->enc_name,"8859")!=NULL )
	fprintf( file, "CHARSET_REGISTRY \"ISO8859\"\n" );
    else if ( font->encoding_name->is_unicodebmp ||
	    font->encoding_name->is_unicodefull )
	fprintf( file, "CHARSET_REGISTRY \"ISO10646\"\n" );
    else
	fprintf( file, "CHARSET_REGISTRY \"%s\"\n", encoding );
    if (( pt = strrchr(encoding,'-'))!=NULL )
	/* DoNothing */;
    else if ( (pt = strstr(encoding,"8859"))!=NULL && isdigit(pt[4]))
	pt += 4;
    else
	pt = "0";
    fprintf( file, "CHARSET_ENCODING \"%s\"\n", pt );

    OS2FigureCodePages(font->sf,codepages);
    fprintf( file, "CHARSET_COLLECTIONS \"" );
    if ( codepages[0]&0xff )
	fprintf( file, "ASCII " );
    else {
	for ( i=32; i<127 && i<font->sf->charcnt; ++i )
	    /* I'll accept a missing glyph, but not a badly encoded one */
	    if ( font->sf->chars[i]!=NULL && font->sf->chars[i]->unicodeenc!=i )
	break;
	if ( i==127 )
	    fprintf( file, "ASCII ");
    }
    if ( (codepages[0]&1) )
	fprintf( file, "ISOLatin1Encoding " );
    if ( (codepages[0]&2) )
	fprintf( file, "ISO8859-2 " );
    if ( (codepages[0]&4) )
	fprintf( file, "ISO8859-5 " );
    if ( (codepages[0]&8) )
	fprintf( file, "ISO8859-7 " );
    if ( (codepages[0]&0x10) )
	fprintf( file, "ISO8859-9 " );
    if ( (codepages[0]&0x20) )
	fprintf( file, "ISO8859-8 " );
    if ( (codepages[0]&0x40) )
	fprintf( file, "ISO8859-6 " );
    if ( (codepages[0]&0x80) )
	fprintf( file, "ISO8859-4 " );
    if ( (codepages[0]&0x10000)  )
	fprintf( file, "ISO8859-11 " );
    if ( (codepages[0]&0x20000) && (font->encoding_name->is_unicodebmp || font->encoding_name->is_unicodefull ))
	fprintf( file, "JISX0208.1997 " );
    if ( (codepages[0]&0x40000) && (font->encoding_name->is_unicodebmp || font->encoding_name->is_unicodefull ) )
	fprintf( file, "GB2312.1980 " );
    if ( (codepages[0]&0x80000) && (font->encoding_name->is_unicodebmp || font->encoding_name->is_unicodefull ))
	fprintf( file, "KSC5601.1992 " );
    if ( (codepages[0]&0x100000) && (font->encoding_name->is_unicodebmp || font->encoding_name->is_unicodefull ))
	fprintf( file, "BIG5 " );
    if ( (codepages[0]&0x80000000) )
	fprintf( file, "Symbol " );

    fprintf( file, "%s\"\n", encoding );

    fprintf( file, "FULL_NAME \"%s\"\n", font->sf->fullname );

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
    fprintf( file, "ENDPROPERTIES\n" );
    fprintf( file, "CHARS %d\n", cnt );
}

int BDFFontDump(char *filename,BDFFont *font, char *encodingname, int res) {
    char buffer[300];
    FILE *file;
    int i, enc;
    int ret = 0;

    if ( filename==NULL ) {
	sprintf(buffer,"%s-%s.%d.bdf", font->sf->fontname, encodingname, font->pixelsize );
	filename = buffer;
    }
    file = fopen(filename,"w" );
    if ( file==NULL )
	fprintf( stderr, "Can't open %s\n", filename );
    else {
	BDFDumpHeader(file,font,encodingname,res);
	for ( i=0; i<font->charcnt; ++i ) if ( !IsntBDFChar(font->chars[i])) {
	    enc = i;
	    if ( i>=font->sf->encoding_name->char_cnt )
		enc = -1;
	    BDFDumpChar(file,font,font->chars[i],enc);
	}
	fprintf( file, "ENDFONT\n" );
	if ( ferror(file))
	    fprintf( stderr, "Failed to write %s\n", filename );
	else
	    ret = 1;
	fclose(file);
    }
return( ret );
}
