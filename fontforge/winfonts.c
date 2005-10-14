/* Copyright (C) 2002-2005 by George Williams */
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
#include <stdio.h>
#include <math.h>
#include "splinefont.h"
#include <ustring.h>
#include <string.h>
#include <utype.h>

/* Look for 
Source: Microsoft Windows 2.0 SDK Programmer's Refrence, pages 639 through 645
        Microsoft Windows Device Driver Kit, Device Driver Adaptaion Guide, pages 13-1-13-15

This is quoted directly from "The PC Programmer's Sourcebook, 2nd Edition", by
Thom Hogan, pages 6-18 through 6-19.  You need this book.  ISBN 1-55615-321-X.
*/

/* The information herein is derived from Windows 3 Developer's Notes summary */
/* Spec for v3 */
/*   http://support.microsoft.com/default.aspx?scid=KB;en-us;q65123	      */
/*    http://www.csdn.net/Dev/Format/text/font.htm			      */
/* Spec for v2 */
/*   http://www.technoir.nu/hplx/hplx-l/9708/msg00404.html		      */
/* Spec for FontDirEntry */
/*   http://www.sxlist.com/techref/os/win/api/win32/struc/src/str08_9.htm     */
/* Spec for ?FON? file? */
/*   http://www.csn.ul.ie/~caolan/publink/winresdump/winresdump/doc/resfmt.txt*/
/*  and from the freetype source code (particularly:			      */
/*  ~freetype/src/winfonts/winfnt.c and ~freetype/include/freetype/internal/fntypes.h */

/* Windows FNT header. A FON file may contain several FNTs */
struct fntheader {
    uint16	version;		/* Either 0x200 or 0x300 */
    uint32	filesize;
    char	copyright[60+1];
    uint16	type;
#define FNT_TYPE_VECTOR	0x0001		/* If set a vector FNT, else raster (we only parse rasters) */
/* not used, mbz	0x0002 */
#define FNT_TYPE_MEMORY	0x0004		/* If set font is in ROM */
/* not used, mbz	0x0078 */
#define FNT_TYPE_DEVICE	0x0080		/* If set font is "realized by a device" whatever that means */
/* reserved for device	0xff00 */
    uint16	pointsize;		/* design pointsize */
    uint16	vertres;		/* Vertical resolution of font */
    uint16	hortres;		/* Horizontal resolution of font */
    uint16	ascent;
    uint16	internal_leading;
    uint16	external_leading;
    uint8	italic;			/* set to 1 for italic fonts */
    uint8	underline;		/* set to 1 for underlined fonts */
    uint8	strikeout;		/* set to 1 for struckout fonts */
    uint16	weight;			/* 1-1000 windows weight value */
    uint8	charset;		/* ??? */
    uint16	width;			/* non-0 => fixed width font, width of all chars */
    uint16	height;			/* height of font bounding box */
    uint8	pitchfamily;
#define FNT_PITCH_VARIABLE	0x01	/* Variable width font */
#define FNT_FAMILY_MASK		0xf0
#define  FNT_FAMILY_DONTCARE	0x00
#define  FNT_FAMILY_SERIF	0x10
#define  FNT_FAMILY_SANSSERIF	0x20
#define  FNT_FAMILY_FIXED	0x30
#define  FNT_FAMILY_SCRIPT	0x40
#define  FNT_FAMILY_DECORATIVE	0x50
    uint16	avgwidth;		/* Width of "X" */
    uint16	maxwidth;
    uint8	firstchar;
    uint8	lastchar;
    uint8	defchar;		/* ?-firstchar */
    uint8	breakchar;		/* 32-firstchar */
    uint16	widthbytes;		/* Number of bytes in a row */
    uint32	deviceoffset;		/* set to 0 */
    uint32	faceoffset;		/* Offset from start of file to face name (C string) */
    uint32	bitspointer;		/* set to 0 */
    uint32	bitsoffset;		/* Offset from start of file to start of bitmap info */
    uint8	mbz1;
/* These fields are not present in 2.0 and are not meaningful in 3.0 */
/*  they are there for future expansion */
    uint32	flags;
#define FNT_FLAGS_FIXED		0x0001
#define FNT_FLAGS_PROPORTIONAL	0x0002
#define FNT_FLAGS_ABCFIXED	0x0004
#define FNT_FLAGS_ABCPROP	0x0008
#define FNT_FLAGS_1COLOR	0x0010
#define FNT_FLAGS_16COLOR	0x0020
#define FNT_FLAGS_256COLOR	0x0040
#define FNT_FLAGS_RGBCOLOR	0x0080
    uint16	aspace;
    uint16	bspace;
    uint16	cspace;
    uint32	coloroffset;		/* Offset to color table */
    uint8	mbz2[16];		/* Freetype says 4. Online docs say 16 & earlier versions were wrong... */
#if 0		/* Font data */
    union {
	/* Width below is width of bitmap, and advance width */
	/* so no chars can extend before 0 or after advance */
	struct v2chars { uint16 width; uint16 offset; } v2;
	struct v3chars { uint16 width; uint32 offset; } v3;
    } chartable[/*lastchar-firstchar+2*/258];
    /* facename */
    /* devicename */
    /* bitmaps */
	/* Each character is stored in column-major order with one byte for each row */
	/*  then the second byte for each row, ... */
#endif
};


struct v2chars { uint16 width; uint16 offset; };
    /* In v2 I get the impression that characters are stored as they are on the */
    /*  ie. one huge bitmap. The offset gives the location in each row */
struct v3chars { uint16 width; uint32 offset; };
    /* The offset gives the offset to the entire character (stored in a weird */
    /*  format but basically contiguously) from the bitsoffset */

struct winmz_header {
    uint16 magic;
#define FON_MZ_MAGIC	0x5A4D
    uint16 skip[29];
    uint32 lfanew;
};

struct winne_header {
    uint16 magic;
#define FON_NE_MAGIC	0x454E
    uint8 skip[34];
    uint16 resource_tab_offset;
    uint16 rname_tab_offset;
};

/* Little-endian routines. I hate eggs anyway. */
static int lgetushort(FILE *f) {
    int ch1, ch2;
    ch1 = getc(f);
    ch2 = getc(f);
return( (ch2<<8)|ch1 );
}

static int lgetlong(FILE *f) {
    int ch1, ch2, ch3, ch4;
    ch1 = getc(f);
    ch2 = getc(f);
    ch3 = getc(f);
    ch4 = getc(f);
return( (ch4<<24)|(ch3<<16)|(ch2<<8)|ch1 );
}

static void lputshort(FILE *f,int val) {
    putc(val&0xff,f);
    putc((val>>8)&0xff,f);
}

static void lputlong(FILE *f,int val) {
    putc(val&0xff,f);
    putc((val>>8),f);
    putc((val>>16),f);
    putc((val>>24)&0xff,f);
}

static int FNT_Load(FILE *fnt,SplineFont *sf) {
    struct fntheader fntheader;
    struct v3chars charinfo[258];	/* Max size */
    int i, j, k, ch;
    uint32 base = ftell(fnt);
    char *pt, *spt, *temp;
    BDFFont *bdf;
    BDFChar *bdfc;

    memset(&fntheader,0,sizeof(fntheader));
    fntheader.version = lgetushort(fnt);
    if ( fntheader.version != 0x200 && fntheader.version != 0x300 )
return( false );
    fntheader.filesize = lgetlong(fnt);
    for ( i=0; i<60; ++i )
	fntheader.copyright[i] = getc(fnt);
    fntheader.copyright[i] = '\0';
    for ( --i; i>=0 && fntheader.copyright[i]==' '; --i )
	fntheader.copyright[i] = '\0';
    fntheader.type = lgetushort(fnt);
    if ( fntheader.type & (FNT_TYPE_VECTOR|FNT_TYPE_MEMORY|FNT_TYPE_DEVICE))
return( false );
    fntheader.pointsize = lgetushort(fnt);
    fntheader.vertres = lgetushort(fnt);
    fntheader.hortres = lgetushort(fnt);
    fntheader.ascent = lgetushort(fnt);
    fntheader.internal_leading = lgetushort(fnt);
    fntheader.external_leading = lgetushort(fnt);
    fntheader.italic = getc(fnt);
    fntheader.underline = getc(fnt);
    fntheader.strikeout = getc(fnt);
    fntheader.weight = lgetushort(fnt);
    fntheader.charset = getc(fnt);
    fntheader.width = lgetushort(fnt);
    fntheader.height = lgetushort(fnt);
    fntheader.pitchfamily = getc(fnt);
    fntheader.avgwidth = lgetushort(fnt);
    fntheader.maxwidth = lgetushort(fnt);
    fntheader.firstchar = getc(fnt);
    fntheader.lastchar = getc(fnt);
    fntheader.defchar = getc(fnt);
    fntheader.breakchar = getc(fnt);
    fntheader.widthbytes = lgetushort(fnt);
    fntheader.deviceoffset = lgetlong(fnt);
    fntheader.faceoffset = lgetlong(fnt);
    fntheader.bitspointer = lgetlong(fnt);
    fntheader.bitsoffset = lgetlong(fnt);
    (void) getc(fnt);		/* Not documented in the v2 spec but seems to be present */
    if ( fntheader.version == 0x300 ) {
	fntheader.flags = lgetlong(fnt);
	if ( fntheader.flags & (FNT_FLAGS_ABCFIXED|FNT_FLAGS_ABCPROP|FNT_FLAGS_16COLOR|FNT_FLAGS_256COLOR|FNT_FLAGS_RGBCOLOR))
return( false );
	fntheader.aspace = lgetushort(fnt);
	fntheader.bspace = lgetushort(fnt);
	fntheader.cspace = lgetushort(fnt);
	fntheader.coloroffset = lgetlong(fnt);
	for ( i=0; i<16; ++i )		/* Freetype thinks this is 4 */
	    (void) getc(fnt);
#if 0
/* A font dir entry looks like this. It does not read bitspointer, bitsoffset */
    } else {
	fntheader.deviceoffset = lgetlong(fnt);
	fntheader.faceoffset = lgetlong(fnt);
	(void) lgetlong(fnt);
	while ( (ch=getc(fnt))!=EOF && ch!='\0' );
	fntheader.faceoffset = ftell(fnt)-base;
	while ( (ch=getc(fnt))!=EOF && ch!='\0' );
#endif
    }

    memset(charinfo,0,sizeof(charinfo));
    for ( i=fntheader.firstchar; i<=fntheader.lastchar+2; ++i ) {
	charinfo[i].width = lgetushort(fnt);
	if ( fntheader.version==0x200 )
	    charinfo[i].offset = lgetushort(fnt);
	else
	    charinfo[i].offset = lgetlong(fnt);
    }

    /* Set the font names and the pfminfo structure */
    sf->pfminfo.pfmset = true;
    if ( fntheader.copyright[0]!='\0' ) {
	free(sf->copyright);
	sf->copyright = copy(fntheader.copyright);
    }
    free(sf->weight);
    sf->weight = copy( fntheader.weight<=100 ? "Thin" :
			fntheader.weight<=200 ? "Extralight" :
			fntheader.weight<=300 ? "Light" :
			fntheader.weight<=400 ? "Normal" :
			fntheader.weight<=500 ? "Medium" :
			fntheader.weight<=600 ? "Demibold" :
			fntheader.weight<=700 ? "Bold" :
			fntheader.weight<=800 ? "Heavy" :
			fntheader.weight<=900 ? "Black" : "Nord" );
    sf->pfminfo.weight = fntheader.weight;
    sf->pfminfo.panose[2] = fntheader.weight/100 + 1;
    fseek(fnt,base+fntheader.faceoffset,SEEK_SET);
    for ( i=0; (ch=getc(fnt))!=EOF && ch!=0; ++i );
    free(sf->familyname);
    sf->familyname = galloc(i+2);
    fseek(fnt,base+fntheader.faceoffset,SEEK_SET);
    for ( i=0; (ch=getc(fnt))!=EOF && ch!=0; ++i )
	sf->familyname[i] = ch;
    sf->familyname[i] = '\0';
    temp = galloc(i+50);
    strcpy(temp,sf->familyname);
    if ( fntheader.weight<=300 && fntheader.weight>500 ) {
	strcat(temp," ");
	strcat(temp,sf->weight);
    }
    if ( fntheader.italic )
	strcat(temp," Italic");
    free(sf->fullname);
    sf->fullname = temp;
    free(sf->fontname);
    sf->fontname = copy(sf->fullname);
    for ( pt=spt=sf->fontname; *spt; ++spt )
	if ( *spt!=' ' )
	    *pt++ = *spt;
    *pt = '\0';
    sf->pfminfo.pfmfamily = fntheader.pitchfamily;
    sf->pfminfo.panose[0] = 2;
    if ( (fntheader.pitchfamily&FNT_FAMILY_MASK)==FNT_FAMILY_SCRIPT )
	sf->pfminfo.panose[0] = 3;
    sf->pfminfo.width = 5;		/* No info about condensed/extended */
    sf->pfminfo.panose[3] = 3;
    if ( !(fntheader.pitchfamily&FNT_PITCH_VARIABLE ) )
	sf->pfminfo.panose[3] = 9;
    sf->pfminfo.linegap = (sf->ascent+sf->descent)*fntheader.external_leading/fntheader.height;
    if ( fntheader.italic )
	sf->italicangle = 11.25;

    bdf = gcalloc(1,sizeof(BDFFont));
    bdf->sf = sf;
    bdf->glyphcnt = sf->glyphcnt;
    bdf->glyphmax = sf->glyphmax;
    bdf->res = fntheader.vertres;
    bdf->pixelsize = rint(fntheader.pointsize*fntheader.vertres/72.27);
    bdf->glyphs = gcalloc(sf->glyphmax,sizeof(BDFChar *));
    bdf->ascent = rint(.8*bdf->pixelsize);		/* shouldn't use typographical ascent */
    bdf->descent = bdf->pixelsize-bdf->ascent;
    for ( i=fntheader.firstchar; i<=fntheader.lastchar; ++i ) if ( charinfo[i].width!=0 ) {
	int gid = SFMakeChar(sf,sf->map,i)->orig_pos;

	if ( gid>=bdf->glyphcnt ) {
	    if ( gid>=bdf->glyphmax )
		bdf->glyphs = grealloc(bdf->glyphs,(bdf->glyphmax=sf->glyphmax)*sizeof(BDFChar *));
	    memset(bdf->glyphs+bdf->glyphcnt,0,(sf->glyphcnt-bdf->glyphcnt)*sizeof(BDFChar *));
	    bdf->glyphcnt = sf->glyphcnt;
	}

	bdf->glyphs[gid] = bdfc = chunkalloc(sizeof(BDFChar));
	bdfc->xmin = 0;
	bdfc->xmax = charinfo[i].width-1;
	bdfc->ymin = fntheader.ascent-fntheader.height;
	bdfc->ymax = fntheader.ascent-1;
	bdfc->width = charinfo[i].width;
	bdfc->bytes_per_line = (bdfc->xmax>>3)+1;
	bdfc->bitmap = gcalloc(bdfc->bytes_per_line*fntheader.height,sizeof(uint8));
	bdfc->orig_pos = gid;
	bdfc->sc = sf->glyphs[gid];
	bdfc->sc->widthset = true;

	fseek(fnt,base+charinfo[i].offset,SEEK_SET);
	for ( j=0; j<bdfc->bytes_per_line; ++j ) {
	    for ( k=0; k<fntheader.height; ++k )
		bdfc->bitmap[k*bdfc->bytes_per_line+j] = getc(fnt);
	}
	BCCompressBitmap(bdfc);

	if ( feof(fnt) ) {
	    BDFFontFree(bdf);
return( false );
	}
    }

    bdf->next = sf->bitmaps;
    sf->bitmaps = bdf;
return( true );
}

SplineFont *SFReadWinFON(char *filename,int toback) {
    FILE *fon;
    int magic, i, shift_size;
    SplineFont *sf;
    uint32 neoffset, recoffset, recend;
    int font_count;
    BDFFont *bdf, *next;

    fon = fopen(filename,"rb");
    if ( fon==NULL )
return( NULL );
    magic = lgetushort(fon);
    fseek(fon,0,SEEK_SET);
    if ( magic!=0x200 && magic!=0x300 && magic!=FON_MZ_MAGIC ) {
	fclose(fon);
return( NULL );
    }
    sf = SplineFontBlank(256);
    sf->map = EncMapNew(256,256,FindOrMakeEncoding("win"));

    if ( magic == 0x200 || magic==0x300 )
	FNT_Load(fon,sf);
    else {
	/* Ok, we know it begins with a mz header (whatever that is) */
	/* all we care about is the offset to the ne header (whatever that is) */
	fseek(fon,30*sizeof(uint16),SEEK_SET);
	neoffset = lgetlong(fon);
	fseek(fon,neoffset,SEEK_SET);
	if ( lgetushort(fon)!=FON_NE_MAGIC ) {
	    EncMapFree(sf->map);
	    SplineFontFree(sf);
	    fclose(fon);
return( NULL );
	}
	for ( i=0; i<34; ++i ) getc(fon);
	recoffset = neoffset + lgetushort(fon);
	recend = neoffset + lgetushort(fon);

	fseek(fon,recoffset,SEEK_SET);
	shift_size = lgetushort(fon);
	font_count = 0;
	while ( ftell(fon)<recend ) {
	    int id, count;
	    id = lgetushort(fon);
	    if ( id==0 )
	break;
	    count = lgetushort(fon);
	    if ( id==0x8008 ) {
		/* id==0x8007 seems to point to a Font Dir Entry which is almost */
		/*  a copy of the fntheader */
		font_count = count;
		lgetlong(fon);
	break;
	    }
	    fseek(fon,4+count*12,SEEK_CUR);
	}
	for ( i=0; i<font_count; ++i ) {
	    uint32 here = ftell(fon);
	    uint32 offset = lgetushort(fon)<<shift_size;
	    fseek(fon,offset,SEEK_SET);		/* FontDirEntries need a +4 here */
	    FNT_Load(fon,sf);
	    fseek(fon,here+12,SEEK_SET);
	}
    }
    fclose(fon);
    if ( sf->bitmaps==NULL ) {
	EncMapFree(sf->map);
	SplineFontFree(sf);
return( NULL );
    }

    SFOrderBitmapList(sf);
    if ( sf->bitmaps->next!=NULL && toback ) {
	for ( bdf = sf->bitmaps; bdf->next!=NULL; bdf = next ) {
	    next = bdf->next;
	    BDFFontFree(bdf);
	}
	sf->bitmaps = bdf;
    }
    /* Find the biggest font, and adjust the metrics to match */
    for ( bdf = sf->bitmaps; bdf->next!=NULL; bdf = bdf->next );
    for ( i=0; i<sf->glyphcnt ; ++i ) if ( sf->glyphs[i]!=NULL && bdf->glyphs[i]!=NULL )
	sf->glyphs[i]->width = rint(bdf->glyphs[i]->width*1000.0/bdf->pixelsize);
    sf->onlybitmaps = true;
return( sf );
}

static int _FONFontDump(FILE *file,BDFFont *font, EncMap *map, int res) {
    uint32 startpos, endpos, namelocpos, datapos, namepos;
    int i, j, k, l;
    int ch;
    int cnt, badch, defch;
    int first, last, avgwid, maxwid, samewid, maxy, miny, widbytes, spacesize;
    struct pfminfo pfminfo;
    int complained=false;
    int gid;

    if ( font->clut!=NULL )
return( false );

    avgwid = widbytes = maxwid = maxy = last = cnt = 0;
    miny = first = 999999;
    samewid = -1;
    badch = -1;
    defch = -1;
    for ( i=0; i<map->enccount && i<256; ++i ) if ( (gid=map->map[i])!=-1 && font->glyphs[gid]!=NULL && font->glyphs[gid]->width>0 ) {
	if ( i==0 || (i==0x80 && defch!=0) ) defch=i;
	else if ( i!=' ' ) defch = i;
	if ( i<first ) first = i;
	last = i;
	++cnt;
	avgwid += font->glyphs[gid]->width;
	widbytes += (font->glyphs[gid]->width+7)>>3;
	if ( font->glyphs[gid]->ymax>maxy ) maxy = font->glyphs[gid]->ymax;
	if ( font->glyphs[gid]->ymin<miny ) miny = font->glyphs[gid]->ymin;
	if ( font->glyphs[gid]->width>maxy ) maxwid = font->glyphs[gid]->width;
	if ( font->glyphs[gid]->width<font->glyphs[gid]->xmax || font->glyphs[gid]->xmin<0 )
	    badch = gid;
	if ( samewid==-1 ) samewid = font->glyphs[gid]->width;
	else if ( samewid!=font->glyphs[gid]->width ) samewid = -2;
    }
    if (( spacesize = font->pixelsize/4 )==0 ) spacesize=1;
    gid = map->map[' '];
    if ( gid!=-1 && font->glyphs[gid]!=NULL && font->glyphs[gid]->sc!=NULL &&
	    font->glyphs[gid]->sc->unicodeenc == ' ' )
	spacesize = font->glyphs[gid]->width;
    if ( badch!=-1 )
	LogError( _("At pixelsize %d the character %s either starts before the origin or extends beyond the advance width.\n"),
		font->pixelsize, font->glyphs[badch]->sc->name );
    SFDefaultOS2Info(&pfminfo,font->sf,font->sf->fontname);
    widbytes = avgwid+spacesize;
    if ( cnt!=0 ) avgwid = rint(avgwid/(double) cnt);
    gid = map->map['X'];
    if ( font->glyphs[gid]!=NULL && font->glyphs[gid]->sc!=NULL &&
	    font->glyphs[gid]->sc->unicodeenc == 'X' )
	avgwid = font->glyphs[gid]->width;

    if ( res<0 ) {
	switch ( font->pixelsize ) {
	  case 13: case 16: case 32:
	    res = 96;
	  default:
	    res = 120;
	  break;
	}
    }

    startpos = ftell(file);
    lputshort(file,0x200);
    lputlong(file,0);		/* Fix file size up later */
    i = 0;
    if ( font->sf->copyright ) {
	for ( i=0; i<59 && font->sf->copyright[i]!='\0'; ++i )
	    putc(font->sf->copyright[i],file);
    }
    while ( i<60 ) {
	putc('\0',file);
	++i;
    }
    lputshort(file,0);					/* flags */
    lputshort(file,rint(font->pixelsize*72.0/res));	/* pointsize */
    lputshort(file,res);				/* vertical res */
    lputshort(file,res);				/* horizontal res */
    lputshort(file,maxy+1);				/* ascent */
    lputshort(file,0);					/* internal_leading */
    lputshort(file,					/* external_leading */
		    rint(pfminfo.linegap*font->pixelsize/(double)(font->sf->ascent+font->sf->descent)) );
    if ( font->sf->italicangle!=0 ||
	    strstrmatch(font->sf->fontname,"ital")!=NULL ||
	    strstrmatch(font->sf->fontname,"kurs")!=NULL ||
	    strstrmatch(font->sf->fontname,"slanted")!=NULL ||
	    strstrmatch(font->sf->fontname,"obli")!=NULL )
	putc('\1',file);
    else
	putc('\0',file);				/* italic */
    putc('\0',file);					/* underline */
    putc('\0',file);					/* strikeout */
    lputshort(file,pfminfo.weight);			/* weight */
    putc('\0',file);					/* charset */ /* ??? */
    lputshort(file,samewid>0?samewid:0);		/* fixed width */
    lputshort(file,maxy-miny+1);			/* bounding box height */
    putc(pfminfo.pfmfamily,file);			/* pitchfamily */
    lputshort(file,avgwid);				/* average width */
    lputshort(file,maxwid);				/* max width */
    putc(first,file);
    putc(last,file);
    putc(defch-first,file);
    putc(' '-first,file);
    lputshort(file,widbytes);				/* bytes per row */
    lputlong(file,0);					/* device name */
    namelocpos = ftell(file);
    lputlong(file,0);					/* face name, fill in later */
    lputlong(file,0);					/* location in ROM */
    datapos = 118+(last-first+3)*4;
    lputlong(file,datapos);				/* bitmap data pointer */
    putc('\0',file);
/* End of FNT header */

    widbytes = 0;
    for ( i=first; i<=last; ++i ) {
	if ( (gid=map->map[i])!=-1 && font->glyphs[gid]!=NULL && font->glyphs[gid]->width>0 )
	    lputshort(file,font->glyphs[gid]->width);
	else
	    lputshort(file,0);
	lputshort(file,datapos+widbytes);
	if ( gid!=-1 && font->glyphs[gid]!=NULL && font->glyphs[gid]->width>0 )
	    widbytes += ((font->glyphs[gid]->width+7)>>3) * (maxy-miny+1);
    }
    /* And a space */
    lputshort(file,spacesize );
    lputshort(file,widbytes);
    widbytes += spacesize;
    /* And an end marker */
    lputshort(file,0);
    lputshort(file,widbytes);

    if ( ftell(file)-startpos != datapos ) {
	LogError( _("Internal error in creating FNT. file offset wrong\n") );
	complained = true;
    }

    /* And finally the character data */
    widbytes = 0;
    for ( i=first; i<=last; ++i ) {
	int gid = map->map[i];
	BDFChar *bdfc = gid==-1 ? NULL : font->glyphs[gid];
	if ( bdfc!=NULL && bdfc->width>0 ) {
	    widbytes += ((bdfc->width+7)>>3) * (maxy-miny+1);
	    for ( k=0 ; k< bdfc->width; k+= 8 ) {
		for ( j=maxy; j>=miny; --j ) {
		    if ( j>bdfc->ymax || j<bdfc->ymin )
			putc('\0',file);
		    else {
			ch = 0;
			for ( l=0; l<8; ++l ) {
			    if ( k+l>=bdfc->xmin && k+l<=bdfc->xmax &&
				    (bdfc->bitmap[(bdfc->ymax-j)*bdfc->bytes_per_line+((k+l-bdfc->xmin)>>3)]
					&(0x80>>((k+l-bdfc->xmin)&7))) )
				ch |= (0x80>>l);
			}
			putc(ch,file);
		    }
		}
	    }
	    if ( ftell(file)-startpos != datapos+widbytes && !complained ) {
		LogError( _("Internal error in creating FNT. file offset wrong in bitmap data\n") );
		complained = true;
	    }
	}
    }
    /* And the space character */
    spacesize = (spacesize+7)>>3;
    for ( i=0; i<spacesize*(maxy-miny+1); ++i )
	putc('\0',file);

    /* Now the face name */
    namepos = ftell(file);
    fwrite(font->sf->familyname,1,strlen(font->sf->familyname)+1,file);

    /* And now fixup the file size field */
    endpos = ftell(file);
    fseek(file,startpos+2,SEEK_SET);
    lputlong(file,endpos-startpos);
    fseek(file,namelocpos,SEEK_SET);
    lputlong(file,namepos);
    fseek(file,endpos,SEEK_SET);
return( true );
}

int FONFontDump(char *filename,BDFFont *font, EncMap *map, int res) {
    FILE *file;
    int ret;

    file = fopen(filename,"wb");
    if ( file==NULL ) {
	LogError( _("Can't open %s\n"), filename );
return( 0 );
    }
    ret = _FONFontDump(file,font,map,res);
    if ( ferror(file))
	ret = 0;
    if ( fclose(file)!=0 )
	ret = 0;
return( ret );
}
