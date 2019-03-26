/* Copyright (C) 2005-2012 by George Williams */
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

#include "palmfonts.h"

#include "bvedit.h"
#include "encoding.h"
#include "fontforgevw.h"
#include "macbinary.h"
#include "mem.h"
#include "splinefill.h"
#include "splinesaveafm.h"
#include "splineutil.h"
#include "splineutil2.h"
#include "tottf.h"
#include <stdio.h>
#include <math.h>
#include "splinefont.h"
#include <string.h>
#include <ustring.h>

/* Palm bitmap fonts are a bastardized version of the mac 'FONT' resource
 (which is the same format as the newer but still obsolete 'NFNT' resource).
  http://www.palmos.com/dev/support/docs/palmos/PalmOSReference/Font.html
They are stored in a resource database (pdb file)
  http://www.palmos.com/dev/support/docs/fileformats/Introl.html#970318
*/

/* Palm resource dbs have no magic numbers, nor much of anything I can use
    to validate that I have opened a real db file. Their records don't have
    much in the way of identification either, so I can't tell if a record
    is supposed to be a font of an image or a twemlo. Fonts have no names
    (usually. FontBucket has their own non-standard format with names), so
    I can't provide the user with a list of fonts in the file.
    So do the best we can to guess.
*/

struct font {
    int ascent;
    int leading;
    int frectheight;
    int rowwords;
    int first, last;
    struct chars {
	uint16 start;
	int16 width;
    } chars[258];
};

static SplineFont *MakeContainer(struct font *fn, char *family, const char *style) {
    SplineFont *sf;
    int em;
    int i;
    EncMap *map;
    SplineChar *sc;

    sf = SplineFontBlank(256);
    free(sf->familyname); free(sf->fontname); free(sf->fullname);
    sf->familyname = copy(family);
    sf->fontname = malloc(strlen(family)+strlen(style)+2);
    strcpy(sf->fontname,family);
    if ( *style!='\0' ) {
	strcat(sf->fontname,"-");
	strcat(sf->fontname,style);
    }
    sf->fullname = copy(sf->fontname);

    free(sf->copyright); sf->copyright = NULL;

    sf->map = map = EncMapNew(257,257,FindOrMakeEncoding("win"));

    em = sf->ascent+sf->descent;
    sf->ascent = fn->ascent*em/fn->frectheight;
    sf->descent = em - sf->ascent;
    sf->onlybitmaps = true;

    for ( i=fn->first; i<=fn->last; ++i ) if ( fn->chars[i].width!=-1 ) {
	sc = SFMakeChar(sf,map,i);
	sc->width = fn->chars[i].width*em/fn->frectheight;
	sc->widthset = true;
    }
    sc = SFMakeChar(sf,map,256);
    free(sc->name); sc->name = copy(".notdef");
    sc->width = fn->chars[i].width*em/fn->frectheight;
    sc->widthset = true;
return( sf );
}

/* scale all metric info by density/72. Not clear what happens for non-integral results */
static void PalmReadBitmaps(SplineFont *sf,FILE *file,int imagepos,
	struct font *fn,int density) {
    int pixelsize = density*fn->frectheight/72;
    BDFFont *bdf;
    uint16 *fontImage;
    int imagesize, index, i;
    int gid;
    EncMap *map = sf->map;

    for ( bdf = sf->bitmaps; bdf!=NULL && bdf->pixelsize!=pixelsize; bdf=bdf->next );
    if ( bdf!=NULL )
return;

    imagesize = (density*fn->rowwords/72)*(density*fn->frectheight/72);
    fontImage = malloc(2*imagesize);
    fseek(file,imagepos,SEEK_SET);
    for ( i=0; i<imagesize; ++i )
	fontImage[i] = getushort(file);
    if ( feof(file) ) {
	free(fontImage);
return;
    }

    bdf = calloc(1,sizeof(BDFFont));
    bdf->sf = sf;
    bdf->next = sf->bitmaps;
    sf->bitmaps = bdf;
    bdf->glyphcnt = sf->glyphcnt;
    bdf->glyphmax = sf->glyphmax;
    bdf->pixelsize = pixelsize;
    bdf->glyphs = calloc(sf->glyphmax,sizeof(BDFChar *));
    bdf->ascent = density*fn->ascent/72;
    bdf->descent = pixelsize - bdf->ascent;
    bdf->res = 72;

    for ( index=fn->first; index<=fn->last+1; ++index ) {
	int enc = index==fn->last+1 ? 256 : index;
	if ( (gid=map->map[enc])!=-1 && fn->chars[index].width!=-1 ) {
	    BDFChar *bdfc;
	    int i,j, bits, bite, bit;

	    bdfc = chunkalloc(sizeof(BDFChar));
	    memset( bdfc,'\0',sizeof( BDFChar ));
	    bdfc->xmin = 0;
	    bdfc->xmax = density*(fn->chars[index+1].start-fn->chars[index].start)/72-1;
	    bdfc->ymin = -bdf->descent;
	    bdfc->ymax = bdf->ascent-1;
	    bdfc->width = density*fn->chars[index].width/72;
	    bdfc->vwidth = pixelsize;
	    bdfc->bytes_per_line = ((bdfc->xmax-bdfc->xmin)>>3) + 1;
	    bdfc->bitmap = calloc(bdfc->bytes_per_line*(density*fn->frectheight)/72,sizeof(uint8));
	    bdfc->orig_pos = gid;
	    bdfc->sc = sf->glyphs[gid];
	    bdf->glyphs[gid] = bdfc;

	    bits = density*fn->chars[index].start/72; bite = density*fn->chars[index+1].start/72;
	    for ( i=0; i<density*fn->frectheight/72; ++i ) {
		uint16 *test = fontImage + i*density*fn->rowwords/72;
		uint8 *bpt = bdfc->bitmap + i*bdfc->bytes_per_line;
		for ( bit=bits, j=0; bit<bite; ++bit, ++j ) {
		    if ( test[bit>>4]&(0x8000>>(bit&0xf)) )
			bpt[j>>3] |= (0x80>>(j&7));
		}
	    }
	    BCCompressBitmap(bdfc);
	}
    }
    free(fontImage);
}

static SplineFont *PalmTestFont(FILE *file,int end, char *family,const char *style) {
    int type;
    int owtloc;
    int pos = ftell(file);
    struct density {
	int density;
	int offset;
    } density[10];
    int dencount, i;
    struct font fn;
    int imagepos=0;
    SplineFont *sf;
    int maxbit;

    type = getushort(file);
    if ( type==0x0090 || type==0x0092 ) {
	LogError( _("Warning: Byte swapped font mark in palm font.\n") );
	type = type<<8;
    }
    if ( (type&0x9000)!=0x9000 )
return( NULL );
    memset(&fn,0,sizeof(fn));
    fn.first = getushort(file);
    fn.last = getushort(file);
    /* maxWidth = */ (void) getushort(file);
    /* kernmax = */ (void) getushort(file);
    /* ndescent = */ (void) getushort(file);
    /* frectwidth = */ (void) getushort(file);
    fn.frectheight = getushort(file);
    owtloc = ftell(file);
    owtloc += 2*getushort(file);
    fn.ascent = getushort(file);
    /* descent = */ (void) getushort(file);
    fn.leading = getushort(file);
    fn.rowwords = getushort(file);
    if ( feof(file) || ftell(file)>=end || fn.first>fn.last || fn.last>255 ||
	    pos+(fn.last-fn.first+2)*2+2*fn.rowwords*fn.frectheight>end ||
	    owtloc+2*(fn.last-fn.first+2)>end )
return( NULL );
    dencount = 0;
    if ( type&0x200 ) {
	if ( getushort(file)!=1 )	/* Extended data version number */
return(NULL);
	dencount = getushort(file);
	if ( dencount>6 )		/* only a few sizes allowed */
return( NULL );
	for ( i=0; i<dencount; ++i ) {
	    density[i].density = getushort(file);
	    density[i].offset = getlong(file);
	    if ( ftell(file)>end ||
		    (density[i].density!=72 &&
		     density[i].density!=108 &&
		     density[i].density!=144 &&
		     density[i].density!=216 &&	/*Documented, but not supported*/
		     density[i].density!=288 ))	/*Documented, but not supported*/
return( NULL );
	}
    } else {
	imagepos = ftell(file);
	fseek(file,2*fn.rowwords*fn.frectheight,SEEK_CUR);
    }

    /* Bitmap location table */
    /*  two extra entries. One gives loc of .notdef glyph, one points just after it */
    maxbit = fn.rowwords*16;
    for ( i=fn.first; i<=fn.last+2; ++i ) {
	fn.chars[i].start = getushort(file);
	if ( fn.chars[i].start>maxbit || (i!=0 && fn.chars[i].start<fn.chars[i-1].start))
return( NULL );
    }

    fseek(file,owtloc,SEEK_SET);
    for ( i=fn.first; i<=fn.last+1; ++i ) {
	int offset, width;
	offset = (int8) getc(file);
	width = (int8) getc(file);
	if ( offset==-1 && width==-1 )
	    /* Skipped glyph */;
	else if ( offset!=0 )
return(NULL);
	fn.chars[i].width = width;
    }
    if ( feof(file) || ftell(file)>end )
return( NULL );

    sf = MakeContainer(&fn,family,style);
    if ( type&0x200 ) {
	for ( i=0; i<dencount; ++i )
	    PalmReadBitmaps(sf,file,pos+density[i].offset,&fn,density[i].density);
    } else {
	PalmReadBitmaps(sf,file,imagepos,&fn,72);
    }
return( sf );
}

static char *palmreadstring(FILE *file) {
    int pos = ftell(file);
    int i, ch;
    char *str, *pt;

    for ( i=0; (ch=getc(file))!=0 && ch!=EOF; ++i);
    str = pt = malloc(i+1);
    fseek(file,pos,SEEK_SET);
    while ( (ch=getc(file))!=0 && ch!=EOF )
	*pt++ = ch;
    *pt = '\0';
return( str );
}

static SplineFont *PalmTestRecord(FILE *file,int start, int end, char *name) {
    int here = ftell(file);
    int type, size, pos;
    SplineFont *sf = NULL;
    char *family=NULL, *style=NULL;
    int version;

    if ( end<=start )
return( NULL );

    fseek(file,start,SEEK_SET);
    type = getushort(file);
    if ( feof(file))
  goto ret;
    fseek(file,start,SEEK_SET);
    if ( (type&0x9000)==0x9000 || type==0x0090 || type==0x0092 ) {
	sf = PalmTestFont(file,end,name,"");
	if ( sf!=NULL )
  goto ret;
    }
    /* Now test for a font bucket structure */
    version = getc(file); /* version number of font bucket format. currently 0 */
    if ( version==4 ) {
	LogError( _("Warning: Font Bucket version 4 treated as 0.\n") );
	version=0;
    }
    if ( version!=0 )
  goto ret;
    if ( getc(file)!=0 )	/* not interested in system fonts */
  goto ret;
    (void) getushort(file);	/* Skip the pixel height */
    (void) getushort(file);	/* blank bits */
    size = getlong(file);
    pos = ftell(file);		/* Potential start of font data */
    if ( pos+size > end )
  goto ret;
    fseek(file,size,SEEK_CUR);
    family = palmreadstring(file);
    style = palmreadstring(file);
    if ( feof(file) || ftell(file)>end )
  goto ret;
    fseek(file,pos,SEEK_SET);
    sf = PalmTestFont(file,pos+size,family,style);

  ret:
    free(family); free(style);
    fseek(file,here,SEEK_SET);
return( sf );
}

SplineFont *SFReadPalmPdb(char *filename) {
    char name[33];
    FILE *file;
    int num_records, i, file_end;
    int offset, next_offset;
    SplineFont *sf;

    file = fopen(filename,"rb");
    if ( file==NULL )
return( NULL );

    fseek(file,0,SEEK_END);
    file_end = ftell(file);
    fseek(file,0,SEEK_SET);

    fread(name,1,32,file);
    if ( ferror(file) )
  goto fail;
    name[32]=0;
    fseek(file,0x2c,SEEK_CUR);		/* Find start of record list */
    num_records = getushort(file);
    if ( num_records<=0 )
  goto fail;
    offset = getlong(file);		/* offset to data */
    (void) getlong(file);		/* random junk */
    if ( offset>= file_end )
  goto fail;
    for ( i=1; i<num_records; ++i ) {
	next_offset = getlong(file);
	(void) getlong(file);
	if ( feof(file) || next_offset<offset || next_offset>file_end )
  goto fail;
	sf = PalmTestRecord(file,offset,next_offset,name);
	if ( sf!=NULL ) {
	    fclose(file);
return( sf );
	}
	offset=next_offset;
    }
    sf = PalmTestRecord(file,offset,file_end,name);
    if ( sf!=NULL ) {
	fclose(file);
return( sf );
    }

  fail:
    fclose(file);
return( NULL );			/* failed */
}

static FILE *MakeFewRecordPdb(const char *filename,int cnt) {
    FILE *file;
    char *fn = malloc(strlen(filename)+8), *pt1, *pt2;
    long now;
    int i;

    strcpy(fn,filename);
    pt1 = strrchr(fn,'/');
    if ( pt1==NULL ) pt1 = fn; else ++pt1;
    pt2 = strrchr(fn,'.');
    if ( pt2==NULL || pt2<pt1 )
	pt2 = fn+strlen(fn);
    strcpy(pt2,".pdb");
    file = fopen(fn,"wb");
    if ( file==NULL ) {
	ff_post_error(_("Couldn't open file"),_("Couldn't open file %.200s"),fn);
	free(fn);
return( NULL );
    }

    *pt2 = '\0';
    for ( i=0; i<31 && *pt1; ++i, ++pt1 )
	putc(*pt1,file);
    while ( i<32 ) {
	putc('\0',file);
	++i;
    }
    putshort(file,0);				/* attributes */
    putshort(file,0);				/* version */
    now = mactime();
    putlong(file,now);
    putlong(file,now);
    putlong(file,now);
    putlong(file,0);				/* modification number */
    putlong(file,0);				/* app info */
    putlong(file,0);				/* sort info */
    putlong(file,CHR('F','o','n','t'));		/* type */
    putlong(file,CHR('F','t','F','g'));		/* creator */
    putlong(file,0);				/* uniqueIDseed */

    /* Record list */
    putlong(file,0);				/* next record id */
    putshort(file,cnt);				/* numRecords */

    putlong(file,ftell(file)+8*cnt);		/* offset to data */
    putlong(file,0);

    for ( i=1; i<cnt; ++i ) {
	putlong(file,0);
	putlong(file,0);
    }
free(fn);
return(file);
}

static BDFFont *getbdfsize(SplineFont *sf, int32 size) {
    BDFFont *bdf;

    for ( bdf=sf->bitmaps; bdf!=NULL && (bdf->pixelsize!=(size&0xffff) || BDFDepth(bdf)!=(size>>16)); bdf=bdf->next );
return( bdf );
}

struct FontTag {
  uint16 fontType;
  int16 firstChar;
  int16 lastChar;
  int16 maxWidth;
  int16 kernMax;
  int16 nDescent;
  int16 fRectWidth;
  int16 fRectHeight;
  int16 owTLoc;
  int16 ascent;
  int16 descent;
  int16 leading;
  int16 rowWords;
};

static int ValidMetrics(BDFFont *test,BDFFont *base,EncMap *map,int den) {
    /* All glyphs must fit within 0,advance-width */
    /* All advance widths must be den*base->advance_width */
    int i, gid;
    int warned=false, wwarned = false;
    IBounds ib;

    if ( test==NULL )
return( true );

    if ( test==base && map->enc->char_cnt>=256 )
	ff_post_notice(_("Bad Metrics"),_("Only the first 256 glyphs in the encoding will be used"));

    for ( i=0; i<map->enccount && i<256; ++i ) if ( (gid=map->map[i])!=-1 && (test->glyphs[gid]!=NULL || base->glyphs[gid]!=NULL )) {
	if ( base->glyphs[gid]==NULL || test->glyphs[gid]==NULL ) {
	    ff_post_error(_("Bad Metrics"),_("One of the fonts %1$d,%2$d is missing glyph %3$d"),
		    test->pixelsize,base->pixelsize, i);
return( false );
	}
	/* Can't just use the glyph's max/min properties at this point, as some glyphs may contain */
	/* references, in which case those properties would be mostly irrelevant */
	BDFCharQuickBounds( test->glyphs[gid],&ib,0,0,false,true );
	if ( !warned &&
		(ib.minx<0 || ib.maxx>test->glyphs[gid]->width ||
		 ib.maxy>=test->ascent || ib.miny<-test->descent)) {
	    ff_post_notice(_("Bad Metrics"),_("In font %1$d the glyph %2$.30s either starts before 0, or extends after the advance width or is above the ascent or below the descent"),
		    test->pixelsize,test->glyphs[gid]->sc->name);
	    warned = true;
	}
	if ( !wwarned && test->glyphs[gid]->width!=den*base->glyphs[gid]->width ) {
	    ff_post_notice(_("Bad Metrics"),_("In font %1$d the advance width of glyph %2$.30s does not scale the base advance width properly, it shall be forced to the proper value"),
		    test->pixelsize,test->glyphs[gid]->sc->name);
	    wwarned = true;
	}
	if ( base->glyphs[gid]->width>127 ) {
	    ff_post_error(_("Bad Metrics"),_("Advance width of glyph %.30s must be less than 127"),
		    test->pixelsize,test->glyphs[gid]->sc->name);
return( false );
	}
    }
return( true );
}

static void PalmAddChar(uint16 *image,int rw,int rbits,
	BDFFont *bdf,BDFChar *bc, int width) {
    int i,j;

    for ( i=0; i<bdf->pixelsize; ++i ) {
	int y = bdf->ascent-1-i;
	if ( y<=bc->ymax && y>=bc->ymin ) {
	    int bi = bc->ymax-y;
	    int ipos = i*rw;
	    int bipos = bi*bc->bytes_per_line;
	    for ( j=bc->xmin<=0?0:bc->xmin; j<width && j<=bc->xmax; ++j )
		if ( bc->bitmap[bipos+((j-bc->xmin)>>3)]&(0x80>>((j-bc->xmin)&7)) )
		    image[ipos+((rbits+j)>>4)] |= (0x8000>>((rbits+j)&0xf));
	}
    }
}

static uint16 *BDF2Image(struct FontTag *fn, BDFFont *bdf, int **offsets,
	int **widths, int16 *rowWords, BDFFont *base,EncMap *map,int notdefpos) {
    int rbits, rw;
    int i,j, gid;
    uint16 *image;
    int den;
    BDFChar *bdfc;

    if ( bdf==NULL )
return( NULL );
    for ( i=0; i<map->enccount; i++ ) if (( gid=map->map[i])!=-1 && ( bdfc = bdf->glyphs[gid] ) != NULL )
	BCPrepareForOutput( bdfc,true );

    den = bdf->pixelsize/fn->fRectHeight;

    rbits = 0;
    for ( i=fn->firstChar; i<=fn->lastChar; ++i )
	if ( (gid=map->map[i])!=-1 && gid!=notdefpos && base->glyphs[gid]!=NULL )
	    rbits += base->glyphs[gid]->width;
    if ( notdefpos!=-1 )
	rbits += base->glyphs[notdefpos]->width;
    else
	rbits += (fn->fRectHeight/2)+1;
    rw = den * ((rbits+15)/16);

    if ( rowWords!=NULL ) {
	*rowWords = rw;
	*offsets = malloc((fn->lastChar-fn->firstChar+3)*sizeof(int));
	*widths = malloc((fn->lastChar-fn->firstChar+3)*sizeof(int));
    }
    image = calloc(bdf->pixelsize*rw,sizeof(uint16));
    rbits = 0;
    for ( i=fn->firstChar; i<=fn->lastChar; ++i ) {
	if ( offsets!=NULL )
	    (*offsets)[i-fn->firstChar] = rbits;
	if ( (gid=map->map[i])!=-1 && gid!=notdefpos && base->glyphs[gid]!=NULL ) {
	    if ( widths!=NULL )
		(*widths)[i-fn->firstChar] = den*base->glyphs[gid]->width;
	    PalmAddChar(image,rw,rbits,bdf,bdf->glyphs[gid],den*base->glyphs[gid]->width);
	    rbits += den*base->glyphs[gid]->width;
	} else if ( widths!=NULL )
	    (*widths)[i-fn->firstChar] = -1;
    }
    if ( offsets!=NULL )
	(*offsets)[i-fn->firstChar] = rbits;
    if ( notdefpos!=-1 ) {
	PalmAddChar(image,rw,rbits,bdf,bdf->glyphs[notdefpos],den*base->glyphs[notdefpos]->width);
	if ( widths!=NULL )
	    (*widths)[i-fn->firstChar] = den*base->glyphs[notdefpos]->width;
	rbits += bdf->glyphs[notdefpos]->width;
    } else {
	int wid, down, height;
	wid = (fn->fRectHeight/2)*(bdf->pixelsize/fn->fRectHeight)-1;
	if ( widths!=NULL )
	    (*widths)[i-fn->firstChar] = wid+1;
	height = 2*bdf->ascent/3;
	if ( height<3 ) height = bdf->ascent;
	down = bdf->ascent-height;
	for ( j=down; j<down+height; ++j ) {
	    image[j*rw+(rbits>>4)] |= (0x8000>>(rbits&0xf));
	    image[j*rw+((rbits+wid-1)>>4)] |= (0x8000>>((rbits+wid-1)&0xf));
	}
	for ( j=rbits; j<rbits+wid; ++j ) {
	    image[down*rw+(j>>4)] |= (0x8000>>(j&0xf));
	    image[(down+height-1)*rw+(j>>4)] |= (0x8000>>(j&0xf));
	}
	rbits += wid+1;
    }
    if ( offsets!=NULL )
	(*offsets)[i+1-fn->firstChar] = rbits;
    for ( i=0; i<map->enccount; i++ ) if (( gid=map->map[i])!=-1 && ( bdfc = bdf->glyphs[gid] ) != NULL )
	BCRestoreAfterOutput( bdfc );
return( image );
}

int WritePalmBitmaps(const char *filename,SplineFont *sf, int32 *sizes,EncMap *map) {
    BDFFont *base=NULL, *temp;
    BDFFont *densities[4];	/* Be prepared for up to quad density */
    				/* Ignore 1.5 density. No docs on how odd metrics get rounded */
    int i, j, k, f, den, dencnt, gid;
    struct FontTag fn;
    uint16 *images[4];
    int *offsets, *widths;
    int owbase, owpos, font_start, density_starts;
    FILE *file;
    int fontcnt, fonttype;
    int notdefpos = -1;

    if ( sizes==NULL || sizes[0]==0 )
return(false);
    for ( i=0; sizes[i]!=0; ++i ) {
	temp = getbdfsize(sf,sizes[i]);
	if ( temp==NULL || BDFDepth(temp)!=1 )
return( false );
	if ( base==NULL || base->pixelsize>temp->pixelsize )
	    base = temp;
    }
    memset(densities,0,sizeof(densities));
    for ( i=0; sizes[i]!=0; ++i ) {
	temp = getbdfsize(sf,sizes[i]);
	den = temp->pixelsize/base->pixelsize;
	if ( temp->pixelsize!=base->pixelsize*den || den>4 ) {
	    ff_post_error(_("Unexpected density"),_("One of the bitmap fonts specified, %1$d, is not an integral scale up of the smallest font, %2$d (or is too large a factor)"),
		    temp->pixelsize,base->pixelsize);
return( false );
	}
	densities[den-1] = temp;
    }

    dencnt = 0;
    for ( i=0; i<4; ++i ) {
	if ( !ValidMetrics(densities[i],base,map,i+1))
return( false );
	if ( densities[i]) ++dencnt;
    }

    fontcnt = 1;
    if ( no_windowing_ui && dencnt>1 )
	fonttype = 3;
    else if ( dencnt>1 ) {
	char *choices[5];
	choices[0] = _("Multiple-Density Font");
	choices[1] = _("High-Density Font");
	choices[2] = _("Single and Multi-Density Fonts");
	choices[3] = _("Single and High-Density Fonts");
	choices[4] = NULL;
	fonttype = ff_choose(_("Choose a file format..."),(const char **) choices,4,3,_("What type(s) of palm font records do you want?"));
	if ( fonttype==-1 )
return( false );
	if ( fonttype>=2 )
	    fontcnt = 2;
    } else
	fonttype = 4;

    memset(&fn,0,sizeof(fn));
    fn.fontType = 0x9000;
    fn.firstChar = -1;
    fn.fRectHeight = base->pixelsize;
    fn.ascent = base->ascent;
    fn.descent = base->descent;
    for ( i=0; i<map->enccount && i<256; ++i ) {
	gid = map->map[i];
	if ( gid!=-1 && base->glyphs[gid]!=NULL ) {
	    if ( strcmp(sf->glyphs[gid]->name,".notdef")!=0 ) {
		if ( fn.firstChar==-1 ) fn.firstChar = i;
		fn.lastChar = i;
	    }
	    if ( base->glyphs[gid]->width > fn.maxWidth )
		fn.maxWidth = fn.fRectWidth = base->glyphs[gid]->width;
	}
    }
    notdefpos = SFFindNotdef(sf,-2);
    if ( notdefpos > base->glyphcnt || base->glyphs[notdefpos] == NULL )
	notdefpos = -1;

    file = MakeFewRecordPdb(filename,fontcnt);
    if ( file==NULL )
return( false );

    images[0] = BDF2Image(&fn,base,&offsets,&widths,&fn.rowWords,base,map,notdefpos);
    for ( i=1; i<4; ++i )
	images[i] = BDF2Image(&fn,densities[i],NULL,NULL,NULL,base,map,notdefpos);

    for ( f=0; f<2; ++f ) if (( f==0 && fonttype>=2 ) || (f==1 && fonttype!=4)) {
	font_start = ftell(file);

	fn.fontType = f==0 ? 0x9000 : 0x9200;
	putshort(file,fn.fontType);
	putshort(file,fn.firstChar);
	putshort(file,fn.lastChar);
	putshort(file,fn.maxWidth);
	putshort(file,fn.kernMax);
	putshort(file,fn.nDescent);
	putshort(file,fn.fRectWidth);
	putshort(file,fn.fRectHeight);
	owbase = ftell(file);
	putshort(file,fn.owTLoc);
	putshort(file,fn.ascent);
	putshort(file,fn.descent);
	putshort(file,fn.leading);
	putshort(file,fn.rowWords);

	if ( f==0 ) {
	    for ( i=0; i<fn.fRectHeight*fn.rowWords; ++i )
		putshort(file,images[0][i]);
	} else {
	    putshort(file,1);		/* Extended version field */
	    putshort(file,fonttype==0 || fonttype==2 ? dencnt : dencnt-1 );
	    density_starts = ftell(file);
	    for ( i=0; i<4; ++i ) {
		if ( densities[i]!=NULL && (i!=0 || fonttype==0 || fonttype==2)) {
		    putshort(file,(i+1)*72);
		    putlong(file,0);
		}
	    }
	}
	for ( i=fn.firstChar; i<=fn.lastChar+2; ++i )
	    putshort(file,offsets[i-fn.firstChar]);
	owpos = ftell(file);
	fseek(file,owbase,SEEK_SET);
	putshort(file,(owpos-owbase)/2);
	fseek(file,owpos,SEEK_SET);
	for ( i=fn.firstChar; i<=fn.lastChar; ++i ) {
	    if ( (gid=map->map[i])==-1 || base->glyphs[gid]==NULL ) {
		putc(-1,file);
		putc(-1,file);
	    } else {
		putc(0,file);
		putc(base->glyphs[gid]->width,file);
	    }
	}
	putc(0,file);		/* offset/width for notdef */
	putc(widths[i-fn.firstChar],file);

	if ( f==1 ) {
	    for ( i=j=0; i<4; ++i ) {
		if ( densities[i]!=NULL && (i!=0 || fonttype==0 || fonttype==2)) {
		    int here = ftell(file);
		    fseek(file,density_starts+j*6+2,SEEK_SET);
		    putlong(file,here-font_start);
		    fseek(file,here,SEEK_SET);
		    for ( k=0; k<(i+1)*(i+1)*fn.fRectHeight*fn.rowWords; ++k )
			putshort(file,images[i][k]);
		    ++j;
		}
	    }
	}
	if ( f==0 && fontcnt==2 ) {
	    int here = ftell(file);
	    fseek(file,font_start-8,SEEK_SET);
	    putlong(file,here);
	    fseek(file,here,SEEK_SET);
	}
    }
    fclose(file);
    free(offsets);
    free(widths);
    for ( i=0; i<4; ++i )
	free(images[i]);
return( true );
}
