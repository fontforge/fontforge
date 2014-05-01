/* Copyright (C) 2004 by George Williams */
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

/* This program reads a pcl file (for an HP printer) and extracts any truetype*/
/*  fonts nested within it */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define true	1
#define false	0
#define CHR(ch1,ch2,ch3,ch4) (((ch1)<<24)|((ch2)<<16)|((ch3)<<8)|(ch4))

/* ************************************************************************** */
/* ************************** Parsing of PCL files ************************** */
/* ************************************************************************** */

static int escape_char = '\33';

struct ttf_header {
    int header_size;
    int fd_size;
    int header_format;
    int fonttype;
    int style;
    int baseline_pos;
    int cell_width;
    int cell_height;
    int orientation;
    int spacing;
    int symbol_set;
    int pitch;
    int height;
    int xheight;
    int widthtype;
    int strokeweight;
    int typeface;
    int serifstyle;
    int quality;
    int placement;
    int upos, uthick;
    int textheight;
    int textwidth;
    int firstcode;
    int num_chars;
    int actual_num;		/* Not all glyphs are present. tt fonts seem to be subset */
				/*  there also seems some confusion between glyphs and encodings (two entries for glyph 3, space and nbspace) */
    int pitchx, heightx;
    int capheight;
    int fontnumber;
    char fontname[17];
    int xres, yres;		/* Bitmap fonts only */
    int scale_factor;
    int mupos, muthick;
    int fontscaletech;
    int variety;
    struct subtables {
	int len;
	int offset, newoff;
	int checksum;
	int tag;
	FILE *file;
	int closeme;
    } *subtables;
    int subtable_cnt;
    struct glyph {
	int len;
	int offset;
	int unicode;		/* 0xffff means unencoded glyph */
	int glyph;
	struct glyph *continuation;
    } *glyphs;
    /* the glyph count is the num_chars field above */
    char *copyright;
    unsigned char panose[10];
    unsigned char charcompl[8];
};

static unsigned short getshort(FILE *pcl) {
    int ch1, ch2;
    ch1 = getc(pcl);
    ch2 = getc(pcl);
return( (ch1<<8)|ch2 );
}

static unsigned int getlong(FILE *pcl) {
    int ch1, ch2, ch3, ch4;
    ch1 = getc(pcl);
    ch2 = getc(pcl);
    ch3 = getc(pcl);
    ch4 = getc(pcl);
return( (ch1<<24)|(ch2<<16)|(ch3<<8)|ch4 );
}

static int skip2thingamy(FILE *pcl,char *thingamy) {
    int ch;

    for (;;) {
	while ( (ch=getc(pcl))!=EOF && ch!=escape_char );
	if ( ch==EOF )
return( false );
	while ( (ch=getc(pcl))==escape_char );
	if ( ch!=thingamy[0] )
    continue;
	ch = getc(pcl);
	if ( ch==thingamy[1] )
return( true );
	if ( ch==escape_char )
	    ungetc(ch,pcl);
    }
}

static int skip2charcode(FILE *pcl) {
return( skip2thingamy(pcl,"*c"));
}

static int skip2glyph(FILE *pcl) {
return( skip2thingamy(pcl,"(s"));
}

static int slurpglyph(FILE *pcl,struct ttf_header *hdr, int slot) {
    int unicode, size, ch;
    int desc_size, i;
    int cd_size;
    int glyph_start;
    int restart = ftell(pcl);

    if ( !skip2charcode(pcl)) {
	fprintf( stderr, "Fewer glyphs in font than expected\n" );
	fseek(pcl,restart,SEEK_SET);
return( false );
    }
    fscanf(pcl,"%d", &unicode );
    ch = getc(pcl);
    if ( ch!='E' ) {
	fprintf( stderr, "Fewer glyphs in font than expected\n" );
	fseek(pcl,restart,SEEK_SET);
return( false );
    }

    if ( !skip2glyph(pcl))
return( false );
    fscanf(pcl,"%d", &size );
    ch = getc(pcl);
    if ( ch!='W' ) {
	fprintf( stderr, "Bad font, glyph size does not end in \"W\"\n" );
return( false );
    }
    glyph_start = ftell(pcl);

    if ( getc(pcl)!=15 ) {
	fprintf( stderr, "Bad font, glyph format must be 15\n" );
return( false );
    }
    if ( getc(pcl)!=0 ) {
	fprintf( stderr, "Bad font, continuation may not be specified here\n" );
return( false );
    }
    desc_size = getc(pcl);
    if ( getc(pcl)!=15 ) {
	fprintf( stderr, "Bad font, glyph class must be 15\n" );
return( false );
    }
    for ( i=2; i<desc_size; ++i )
	getc(pcl);
    cd_size = getshort(pcl);
    hdr->glyphs[slot].glyph = getshort(pcl);
    hdr->glyphs[slot].unicode = unicode;
    hdr->glyphs[slot].len = cd_size-4;
    hdr->glyphs[slot].offset = ftell(pcl);
    hdr->glyphs[slot].continuation = NULL;

    fseek(pcl,glyph_start+size,SEEK_SET);
    if ( size<cd_size+4+desc_size ) {
	/* The docs only allow for one continuation glyph */
	if ( !skip2glyph(pcl))
return( false );
	fscanf(pcl,"%d", &size );
	ch = getc(pcl);
	if ( ch!='W' ) {
	    fprintf( stderr, "Bad font, glyph size does not end in \"W\"\n" );
return( false );
	}
	if ( getc(pcl)!=15 ) {
	    fprintf( stderr, "Bad font, glyph format must be 15\n" );
return( false );
	}
	if ( getc(pcl)!=1 ) {
	    fprintf( stderr, "Bad font, continuation must be specified here\n" );
return( false );
	}
	hdr->glyphs[slot].continuation = calloc(1,sizeof(struct glyph));
	hdr->glyphs[slot].continuation->offset = ftell(pcl);
	hdr->glyphs[slot].continuation->len = size-4;
	fseek(pcl,size-2,SEEK_CUR);
    }
return( true );
}

static int slurpglyphs(FILE *pcl,struct ttf_header *hdr) {
    int i;

    hdr->glyphs = calloc(hdr->num_chars,sizeof(struct glyph));

    for ( i=0; i<hdr->num_chars; ++i )
	if ( !slurpglyph(pcl,hdr,i)) {
	    hdr->actual_num = i;
return( false );
	}
    hdr->actual_num = i;

return( true );
}

static int skip2fontheader(FILE *pcl) {
return( skip2thingamy(pcl,")s"));
}

static void readttftables(FILE *pcl,struct ttf_header *hdr,int seg_size) {
    int header_base = ftell(pcl);
    int end_at = header_base+seg_size;
    int num, i,j;

    /* version = */ getlong(pcl);
    num = getshort(pcl);
    /* srange = */ getshort(pcl);
    /* esel = */ getshort(pcl);
    /* rshift = */ getshort(pcl);

    hdr->subtable_cnt = num;
    hdr->subtables = malloc(num*sizeof(struct subtables));
    for ( i=j=0; i<num; ++i ) {
	hdr->subtables[j].tag = getlong(pcl);
	/* checksum = */ getlong(pcl);
	hdr->subtables[j].offset = header_base+getlong(pcl);
	hdr->subtables[j].len = getlong(pcl);
	hdr->subtables[j].file = pcl;
	hdr->subtables[j].closeme = false;
	if ( hdr->subtables[j].tag != CHR('g','d','i','r'))
	    ++j;
	else
	    --hdr->subtable_cnt;
    }
    /* head, hhea, hmtx and maxp required to be present */
    /* We must synthesize: OS/2, name, cmap, glyf, loca, post */
    /* We should remove 'gdir' (not part of truetype) */
    fseek(pcl,end_at,SEEK_SET);
}

static struct subtables *gettable(struct ttf_header *hdr,int tag) {
    int j;
    for ( j=0; j<hdr->subtable_cnt && tag!=hdr->subtables[j].tag; ++j );
    if ( j==hdr->subtable_cnt )
return( NULL );

return(&hdr->subtables[j]);
}

static int hasRequiredTables(struct ttf_header *hdr) {
    static struct { int name, present; } tables[] = {
	{ CHR('h','e','a','d') },
	{ CHR('h','h','e','a') },
	{ CHR('h','m','t','x') },
	{ CHR('m','a','x','p') },
	/* They can leave out 'gdir' if they want, I don't need it */
	/* (and I'll remove it anyway) */
	{ 0 }};
    int i;

    for ( i=0; tables[i].name!=0; ++i ) {
	if ( gettable(hdr,tables[i].name)==NULL )
return( false );
    }
return( true );
}

static int readheaderformat(FILE *pcl,struct ttf_header *hdr) {
    int ch, i;

    memset(hdr,0,sizeof(*hdr));

    fscanf(pcl,"%d", &hdr->header_size );
    ch = getc(pcl);
    if ( ch!='W' ) {
	fprintf( stderr, "Bad font, font header command doesn't end in \"W\"\n" );
return( false );
    }
    hdr->fd_size = getshort(pcl);
    hdr->header_format = getc(pcl);
    hdr->fonttype = getc(pcl);
    hdr->style = getc(pcl)<<8;
    (void) getc(pcl);
    hdr->baseline_pos = getshort(pcl);
    hdr->cell_width = getshort(pcl);
    hdr->cell_height = getshort(pcl);
    hdr->orientation = getc(pcl);
    hdr->spacing = getc(pcl);
    hdr->symbol_set = getshort(pcl);
    hdr->pitch = getshort(pcl);
    hdr->height = getshort(pcl);
    hdr->xheight = getshort(pcl);
    hdr->widthtype = getc(pcl);
    hdr->style |= getc(pcl);
    hdr->strokeweight = getc(pcl);
    hdr->typeface = getc(pcl);
    hdr->typeface |= getc(pcl)<<8;
    hdr->serifstyle = getc(pcl);
    hdr->quality = getc(pcl);
    hdr->placement = getc(pcl);
    hdr->upos = getc(pcl);
    hdr->uthick = getc(pcl);
    hdr->textheight = getshort(pcl);
    hdr->textwidth = getshort(pcl);
    hdr->firstcode = getshort(pcl);
    hdr->num_chars = getshort(pcl);
    hdr->pitchx = getc(pcl);
    hdr->heightx = getc(pcl);
    hdr->capheight = getshort(pcl);
    hdr->fontnumber = getlong(pcl);
    for ( i=0; i<16; ++i )
	hdr->fontname[i] = getc(pcl);
    while ( i>1 && hdr->fontname[i-1]==' ' ) --i;
    hdr->fontname[i] = '\0';
return( false );
}

static int readheaderttf(FILE *pcl,struct ttf_header *hdr) {
    int i;
    int seg_id, seg_size;

    hdr->scale_factor = getshort(pcl);
    hdr->mupos = (short) getshort(pcl);
    hdr->muthick = getshort(pcl);
    hdr->fontscaletech = getc(pcl);
    hdr->variety = getc(pcl);

    if ( hdr->fd_size<72 || hdr->header_format!=15 ) {
	fprintf( stderr, "%s is not a PCL truetype font\n", hdr->fontname );
return( false );
    }
    /* Manual says there font descriptor size is at least 72, but no indication */
    /*  given about what these variable data might contain. The examples I've */
    /*  been given have the fd_size==72 */
    for ( i=0 ; i<hdr->fd_size-72; ++i )
	getc(pcl);

    for (;;) {
	seg_id = getshort(pcl);
	seg_size = getshort(pcl);
	if ( seg_id == (('C'<<8)|'P') ) {
	    hdr->copyright = malloc(seg_size+1);
	    fread(hdr->copyright,1,seg_size,pcl);
	    hdr->copyright[seg_size] = '\0';
	} else if ( seg_id == (('C'<<8)|'C') ) {
	    if ( seg_size==8 )
		fread(hdr->charcompl,1,8,pcl);
	    else
		fseek(pcl,seg_size,SEEK_CUR);
	} else if ( seg_id == (('P'<<8)|'A') ) {
	    if ( seg_size==10 )
		fread(hdr->panose,1,10,pcl);
	    else
		fseek(pcl,seg_size,SEEK_CUR);
	} else if ( seg_id == (('G'<<8)|'T') ) {
	    readttftables(pcl,hdr,seg_size);
	} else if ( seg_id == 0xffff || seg_id==EOF ) {
	    getshort(pcl);
    break;
	} else
	    fseek(pcl,seg_size,SEEK_CUR);
    }
    if ( !hasRequiredTables(hdr)) {
	fprintf( stderr, "%s is missing a required ttf table\n", hdr->fontname );
return( false );
    }

return( slurpglyphs(pcl,hdr));
}

/* ************************************************************************** */
/* *********************** Generate Missing ttf tables ********************** */
/* ************************************************************************** */

static void putshort(FILE *file,int sval) {
    putc((sval>>8)&0xff,file);
    putc(sval&0xff,file);
}

static void putlong(FILE *file,int val) {
    putc((val>>24)&0xff,file);
    putc((val>>16)&0xff,file);
    putc((val>>8)&0xff,file);
    putc(val&0xff,file);
}

static unsigned short gettableshort(struct ttf_header *hdr, int tag, int offset) {
    /* Get a short at the indicated offset of the indicated table */
    struct subtables *tbl = gettable(hdr,tag);
    if ( tbl==NULL ) {
	fprintf( stderr, "Missing required table: '%c%c%c%c'\n", tag>>24, tag>>16, tag>>8, tag );
return( -1 );
    }
    if ( offset+1>=tbl->len ) {
	fprintf( stderr, "Attempt to read beyond the end of a table\n" );
return( -1 );
    }
    fseek(tbl->file,tbl->offset+offset,SEEK_SET);
return(getshort(tbl->file));
}

static void copydata(FILE *to,FILE *from,int offset,int len) {
    int i, ch;

    fseek( from,offset,SEEK_SET );
    for ( i=0; i<len; ++i ) {
	ch = getc(from);
	putc(ch,to);
    }
}

static void AddTable(struct ttf_header *hdr,struct subtables *table) {
    struct subtables *new = malloc((hdr->subtable_cnt+1)*sizeof(struct subtables));
    int i;

    for ( i=0; i<hdr->subtable_cnt && hdr->subtables[i].tag<table->tag; ++i ) {
	new[i] = hdr->subtables[i];
    }
    new[i] = *table;
    for ( ; i<hdr->subtable_cnt ; ++i ) {
	new[i+1] = hdr->subtables[i];
    }
    free(hdr->subtables);
    hdr->subtables = new;
    ++hdr->subtable_cnt;
}

static void regen_head(struct ttf_header *hdr, int forcelongloca) {
    /* Just on the off chance the 'head' table says that we can use a short */
    /*  loca format, when we can't, we might need to redo head */
    /* Oops, no. We always need to redo head to set checksumAdj to 0 */
    struct subtables *head_tab = gettable(hdr,CHR('h','e','a','d'));
    FILE *head;

    if ( head_tab==NULL )
return;
    if ( head_tab->len<54 )
return;

    head = tmpfile();
    copydata(head,head_tab->file,head_tab->offset,head_tab->len);
    fseek(head,8,SEEK_SET);
    putlong(head,0);
    if ( forcelongloca ) {
	fseek(head,50,SEEK_SET);
	putshort(head,true);
	fseek(head,0,SEEK_END);
    }
    head_tab->file = head;
    head_tab->offset = 0;
    head_tab->closeme = true;
}

static void create_cmap(struct ttf_header *hdr,struct glyph *glyphs,int numGlyphs) {
    FILE *cmap = tmpfile();
    struct subtables table;
    /* We are given data for a unicode table */
    /* I'm not going to try to figure out a mac roman table */
    /* I suppose I might as well add a mac unicode table */
    /* taken pretty much directly from pfaedit */

    {
	int i,j,l, mspos;
	int segcnt, delta, rpos;
	struct cmapseg { unsigned short start, end; unsigned short delta; unsigned short rangeoff; } *cmapseg;
	unsigned short *ranges;
	unsigned int *avail = malloc(65536*sizeof(unsigned int));
	memset(avail,0xff,65536*sizeof(unsigned int));
	for ( i=0; i<numGlyphs; ++i ) {
	    if ( glyphs[i].unicode<0xffff )
		avail[glyphs[i].unicode] = glyphs[i].glyph;
	}

	j = -1;
	for ( i=segcnt=0; i<65536; ++i ) {
	    if ( avail[i]!=0xffffffff && j==-1 ) {
		j=i;
		++segcnt;
	    } else if ( j!=-1 && avail[i]==0xffffffff )
		j = -1;
	}
	cmapseg = calloc(segcnt+1,sizeof(struct cmapseg));
	ranges = malloc(hdr->num_chars*sizeof(short));
	j = -1;
	for ( i=segcnt=0; i<65536; ++i ) {
	    if ( avail[i]!=0xffffffff && j==-1 ) {
		j=i;
		cmapseg[segcnt].start = j;
		++segcnt;
	    } else if ( j!=-1 && avail[i]==0xffffffff ) {
		cmapseg[segcnt-1].end = i-1;
		j = -1;
	    }
	}
	if ( j!=-1 )
	    cmapseg[segcnt-1].end = i-1;
	/* create a dummy segment to mark the end of the table */
	cmapseg[segcnt].start = cmapseg[segcnt].end = 0xffff;
	cmapseg[segcnt++].delta = 1;
	rpos = 0;
	for ( i=0; i<segcnt-1; ++i ) {
	    l = avail[cmapseg[i].start];
	    delta = l-cmapseg[i].start;
	    for ( j=cmapseg[i].start; j<=cmapseg[i].end; ++j ) {
		l = avail[j];
		if ( delta != l-j )
	    break;
	    }
	    if ( j>cmapseg[i].end )
		cmapseg[i].delta = delta;
	    else {
		cmapseg[i].rangeoff = (rpos + (segcnt-i)) * sizeof(short);
		for ( j=cmapseg[i].start; j<=cmapseg[i].end; ++j ) {
		    l = avail[j];
		    ranges[rpos++] = l;
		}
	    }
	}
	free(avail);

	/* Two encoding table pointers, one for ms, one for mac unicode */
	/*  (same as ms) */
	putshort(cmap,0);		/* version */
	putshort(cmap,2);		/* num tables */

	mspos = 2*sizeof(unsigned short)+2*(2*sizeof(unsigned short)+sizeof(unsigned int));
	/* big mac table, just a copy of the ms table */
	putshort(cmap,0);	/* mac unicode platform */
	putshort(cmap,3);	/* Unicode 2.0 */
	putlong(cmap,mspos);

	putshort(cmap,3);		/* ms platform */
	putshort(cmap,1);		/* plat specific enc, Unicode */
	putlong(cmap,mspos);	/* offset from tab start to sub tab start */

	if ( ftell(cmap)!=mspos )
	    fprintf( stderr, "Internal error, encoding table not where expected\n" );

	putshort(cmap,4);		/* format */
	putshort(cmap,(8+4*segcnt+rpos)*sizeof(short));
	putshort(cmap,0);		/* language/version */
	putshort(cmap,2*segcnt);	/* segcnt */
	for ( j=0,i=1; i<=segcnt; i<<=1, ++j );
	putshort(cmap,i);		/* 2*2^floor(log2(segcnt)) */
	putshort(cmap,j-1);
	putshort(cmap,2*segcnt-i);
	for ( i=0; i<segcnt; ++i )
	    putshort(cmap,cmapseg[i].end);
	putshort(cmap,0);
	for ( i=0; i<segcnt; ++i )
	    putshort(cmap,cmapseg[i].start);
	for ( i=0; i<segcnt; ++i )
	    putshort(cmap,cmapseg[i].delta);
	for ( i=0; i<segcnt; ++i )
	    putshort(cmap,cmapseg[i].rangeoff);
	for ( i=0; i<rpos; ++i )
	    putshort(cmap,ranges[i]);
	free(ranges);
	free(cmapseg);
    }

    table.len = ftell(cmap);
    table.offset = 0;
    table.tag = CHR('c','m','a','p');
    table.file = cmap;
    table.closeme = true;
    AddTable(hdr,&table);
}

/* this table taken from PfaEdit */
static int uniranges[][3] = {
    { 0x20, 0x7e, 0 },		/* Basic Latin */
    { 0xa0, 0xff, 1 },		/* Latin-1 Supplement */
    { 0x100, 0x17f, 2 },	/* Latin Extended-A */
    { 0x180, 0x24f, 3 },	/* Latin Extended-B */
    { 0x250, 0x2af, 4 },	/* IPA Extensions */
    { 0x2b0, 0x2ff, 5 },	/* Spacing Modifier Letters */
    { 0x300, 0x36f, 6 },	/* Combining Diacritical Marks */
    { 0x370, 0x3ff, 7 },	/* Greek */
    { 0x400, 0x52f, 9 },	/* Cyrillic / Cyrillic Supplement */
    { 0x530, 0x58f, 10 },	/* Armenian */
    { 0x590, 0x5ff, 11 },	/* Hebrew */
    { 0x600, 0x6ff, 13 },	/* Arabic */
    { 0x700, 0x74f, 71 },	/* Syriac */
    { 0x780, 0x7bf, 72 },	/* Thaana */
    { 0x900, 0x97f, 15 },	/* Devanagari */
    { 0x980, 0x9ff, 16 },	/* Bengali */
    { 0xa00, 0xa7f, 17 },	/* Gurmukhi */
    { 0xa80, 0xaff, 18 },	/* Gujarati */
    { 0xb00, 0xb7f, 19 },	/* Oriya */
    { 0xb80, 0xbff, 20 },	/* Tamil */
    { 0xc00, 0xc7f, 21 },	/* Telugu */
    { 0xc80, 0xcff, 22 },	/* Kannada */
    { 0xd00, 0xd7f, 23 },	/* Malayalam */
    { 0xd80, 0xdff, 73 },	/* Sinhala */
    { 0xe00, 0xe7f, 24 },	/* Thai */
    { 0xe80, 0xeff, 25 },	/* Lao */
    { 0xf00, 0xfbf, 70 },	/* Tibetan */
    { 0x1000, 0x109f, 74 },	/* Myanmar */
    { 0x10a0, 0x10ff, 26 },	/* Georgian */
    { 0x1100, 0x11ff, 28 },	/* Hangul Jamo */
    { 0x1200, 0x137f, 75 },	/* Ethiopic */
    { 0x13a0, 0x13ff, 76 },	/* Cherokee */
    { 0x1400, 0x167f, 77 },	/* Unified Canadian Aboriginal Symbols */
    { 0x1680, 0x169f, 78 },	/* Ogham */
    { 0x16a0, 0x16ff, 79 },	/* Runic */
    { 0x1700, 0x177f, 84 },	/* Tagalog / Harunoo / Buhid / Tagbanwa */
    { 0x1780, 0x17ff, 80 },	/* Khmer */
    { 0x1800, 0x18af, 81 },	/* Mongolian */
    /* { 0x1900, 0x194f, },	   Limbu */
    /* { 0x1950, 0x197f, },	   Tai le */
    /* { 0x19e0, 0x19ff, },	   Khmer Symbols */
    /* { 0x1d00, 0x1d7f, },	   Phonetic Extensions */
    { 0x1e00, 0x1eff, 29 },	/* Latin Extended Additional */
    { 0x1f00, 0x1fff, 30 },	/* Greek Extended */
    { 0x2000, 0x206f, 31 },	/* General Punctuation */
    { 0x2070, 0x209f, 32 },	/* Superscripts and Subscripts */
    { 0x20a0, 0x20cf, 33 },	/* Currency Symbols */
    { 0x20d0, 0x20ff, 34 },	/* Combining Marks for Symbols */
    { 0x2100, 0x214f, 35 },	/* Letterlike Symbols */
    { 0x2150, 0x218f, 36 },	/* Number Forms */
    { 0x2190, 0x21ff, 37 },	/* Arrows */
    { 0x2200, 0x22ff, 38 },	/* Mathematical Operators */
    { 0x2300, 0x237f, 39 },	/* Miscellaneous Technical */
    { 0x2400, 0x243f, 40 },	/* Control Pictures */
    { 0x2440, 0x245f, 41 },	/* Optical Character Recognition */
    { 0x2460, 0x24ff, 42 },	/* Enclosed Alphanumerics */
    { 0x2500, 0x257f, 43 },	/* Box Drawing */
    { 0x2580, 0x259f, 44 },	/* Block Elements */
    { 0x25a0, 0x25ff, 45 },	/* Geometric Shapes */
    { 0x2600, 0x267f, 46 },	/* Miscellaneous Symbols */
    { 0x2700, 0x27bf, 47 },	/* Dingbats */
    { 0x27c0, 0x27ef, 38 },	/* Miscellaneous Mathematical Symbols-A */
    { 0x27f0, 0x27ff, 37 },	/* Supplementary Arrows-A */
    { 0x2800, 0x28ff, 82 },	/* Braille Patterns */
    { 0x2900, 0x297f, 37 },	/* Supplementary Arrows-B */
    { 0x2980, 0x2aff, 38 },	/* Miscellaneous Mathematical Symbols-B /
				   Supplemental Mathematical Operators */
    { 0x2e80, 0x2fff, 59 },	/* CJK Radicals Supplement / Kangxi Radicals /
				   Ideographic Description Characters */
    { 0x3000, 0x303f, 48 },	/* CJK Symbols and Punctuation */
    { 0x3040, 0x309f, 49 },	/* Hiragana */
    { 0x30a0, 0x30ff, 50 },	/* Katakana */
    { 0x3100, 0x312f, 51 },	/* Bopomofo */
    { 0x3130, 0x318f, 52 },	/* Hangul Compatibility Jamo */
    { 0x3190, 0x319f, 59 },	/* Kanbun */
    { 0x31a0, 0x31bf, 51 },	/* Bopomofo Extended */
    { 0x31f0, 0x31ff, 50 },	/* Katakana Phonetic Extensions */
    { 0x3200, 0x32ff, 54 },	/* Enclosed CJK Letters and Months */
    { 0x3300, 0x33ff, 55 },	/* CJK compatability */
    { 0x3400, 0x4dbf, 59 },	/* CJK Unified Ideographs Extension A */
    /* { 0x4dc0, 0x4dff, },	   Yijing Hexagram Symbols */
    { 0x4e00, 0x9fff, 59 },	/* CJK Unified Ideographs */
    { 0xa000, 0xa4cf, 81 },	/* Yi Syllables / Yi Radicals */
    { 0xac00, 0xd7af, 56 },	/* Hangul */
    { 0xe000, 0xf8ff, 60 },	/* Private Use Area */

    { 0xf900, 0xfaff, 61 },	/* CJK Compatibility Ideographs */
    /* 12 ideographs in The IBM 32 Compatibility Additions are CJK unified
       ideographs despite their names: see The Unicode Standard 4.0, p.475 */
    { 0xfa0e, 0xfa0f, 59 },
    { 0xfa10, 0xfa10, 61 },
    { 0xfa11, 0xfa11, 59 },
    { 0xfa12, 0xfa12, 61 },
    { 0xfa13, 0xfa14, 59 },
    { 0xfa15, 0xfa1e, 61 },
    { 0xfa1f, 0xfa1f, 59 },
    { 0xfa20, 0xfa20, 61 },
    { 0xfa21, 0xfa21, 59 },
    { 0xfa22, 0xfa22, 61 },
    { 0xfa23, 0xfa24, 59 },
    { 0xfa25, 0xfa26, 61 },
    { 0xfa27, 0xfa29, 59 },
    { 0xfa2a, 0xfaff, 61 },	/* CJK Compatibility Ideographs */

    { 0xfb00, 0xfb4f, 62 },	/* Alphabetic Presentation Forms */
    { 0xfb50, 0xfdff, 63 },	/* Arabic Presentation Forms-A */
    { 0xfe00, 0xfe0f, 91 },	/* Variation Selectors */
    { 0xfe20, 0xfe2f, 64 },	/* Combining Half Marks */
    { 0xfe30, 0xfe4f, 65 },	/* CJK Compatibility Forms */
    { 0xfe50, 0xfe6f, 66 },	/* Small Form Variants */
    { 0xfe70, 0xfeef, 67 },	/* Arabic Presentation Forms-B */
    { 0xff00, 0xffef, 68 },	/* Halfwidth and Fullwidth Forms */
    { 0xfff0, 0xffff, 69 },	/* Specials */

    /* { 0x10000, 0x1007f, },   Linear B Syllabary */
    /* { 0x10080, 0x100ff, },   Linear B Ideograms */
    /* { 0x10100, 0x1013f, },   Aegean Numbers */
    { 0x10300, 0x1032f, 85 },	/* Old Italic */
    { 0x10330, 0x1034f, 86 },	/* Gothic */
    { 0x10400, 0x1044f, 87 },	/* Deseret */
    /* { 0x10450, 0x1047f, },   Shavian */
    /* { 0x10480, 0x104af, },   Osmanya */
    /* { 0x10800, 0x1083f, },   Cypriot Syllabary */
    { 0x1d000, 0x1d1ff, 88 },	/* Byzantine Musical Symbols / Musical Symbols */
    /* { 0x1d300, 0x1d35f, },   Tai Xuan Jing Symbols */
    { 0x1d400, 0x1d7ff, 89 },	/* Mathematical Alphanumeric Symbols */
    { 0x20000, 0x2a6df, 59 },	/* CJK Unified Ideographs Extension B */
    { 0x2f800, 0x2fa1f, 61 },	/* CJK Compatibility Ideographs Supplement */
    { 0xe0000, 0xe007f, 92 },	/* Tags */
    { 0xe0100, 0xe01ef, 91 },	/* Variation Selectors Supplement */
    { 0xf0000, 0xffffd, 90 },	/* Supplementary Private Use Area-A */
    { 0x100000, 0x10fffd, 90 },	/* Supplementary Private Use Area-B */
};

/* this routine taken from PfaEdit */
static void SetRanges(int urange[4],int CodePage[2],int unicode) {
    int j;
    
    for ( j=0; j<sizeof(uniranges)/sizeof(uniranges[0]); ++j )
	if ( unicode>=uniranges[j][0] && unicode<=uniranges[j][1] ) {
	    int bit = uniranges[j][2];
	    urange[bit>>5] |= (1<<(bit&31));
    break;
	}

    if ( unicode=='A' )
	CodePage[1] |= 1<<31;		/* US (Ascii I assume) */
    else if ( unicode==0xde ) {
	CodePage[0] |= 1<<0;		/* latin1 */
	CodePage[1] |= 1<<30;		/* WE/Latin1 */
    } else if ( unicode==0xc3 )
	CodePage[1] |= 1<<18;		/* MS-DOS Nordic */
    else if ( unicode==0xe9 )
	CodePage[1] |= 1<<20;		/* MS-DOS Canadian French */
    else if ( unicode==0xf5 )
	CodePage[1] |= 1<<23;		/* MS-DOS Portuguese */
    else if ( unicode==0xfe )
	CodePage[1] |= 1<<22;		/* MS-DOS Icelandic */
    else if ( unicode==0x13d ) {
	CodePage[0] |= 1<<1;		/* latin2 */
	CodePage[1] |= 1<<26;		/* latin2 */
    } else if ( unicode==0x411 ) {
	CodePage[0] |= 1<<2;		/* cyrillic */
	CodePage[1] |= 1<<17;		/* MS DOS Russian */
	CodePage[1] |= 1<<25;		/* IBM Cyrillic */
    } else if ( unicode==0x386 ) {
	CodePage[0] |= 1<<3;		/* greek */
	CodePage[1] |= 1<<16;		/* IBM Greek */
	CodePage[1] |= 1<<28;		/* Greek, former 437 G */
    } else if ( unicode==0x130 ) {
	CodePage[0] |= 1<<4;		/* turkish */
	CodePage[1] |= 1<<24;		/* IBM turkish */
    } else if ( unicode==0x5d0 ) {
	CodePage[0] |= 1<<5;		/* hebrew */
	CodePage[1] |= 1<<21;		/* hebrew */
    } else if ( unicode==0x631 ) {
	CodePage[0] |= 1<<6;		/* arabic */
	CodePage[1] |= 1<<19;		/* arabic */
	CodePage[1] |= 1<<29;		/* arabic; ASMO 708 */
    } else if ( unicode==0x157 ) {
	CodePage[0] |= 1<<7;		/* baltic */
	CodePage[1] |= 1<<27;		/* baltic */
    } else if ( unicode==0xe45 )
	CodePage[0] |= 1<<16;		/* thai */
    else if ( unicode==0x30a8 )
	CodePage[0] |= 1<<17;		/* japanese */
    else if ( unicode==0x3105 )
	CodePage[0] |= 1<<18;		/* simplified chinese */
    else if ( unicode==0x3131 )
	CodePage[0] |= 1<<19;		/* korean wansung */
    else if ( unicode==0xacf4 )
	CodePage[0] |= 1<<21;		/* korean Johab */
    else if ( unicode==0x21d4 )
	CodePage[0] |= 1<<31;		/* symbol */
}

static void create_OS2(struct ttf_header *hdr,struct glyph **orderedglyphs,int numGlyphs) {
    FILE *os2 = tmpfile();
    struct subtables table;
    int i;
    int uranges[4], codeRanges[2];
    int minu, maxu;
    int fsstyle=0;

    if ( strstr(hdr->fontname,"Bd")!=NULL )
	fsstyle |= (1<<5);
    if ( strstr(hdr->fontname,"It")!=NULL )
	fsstyle |= 1;

    putshort(os2,0x0002);		/* version 2 */
    putshort(os2,hdr->textwidth);	/* supposed to be average character width, but average lower case is close */
    putshort(os2,hdr->strokeweight==0?400:
		 hdr->strokeweight==1?500:
		 hdr->strokeweight==2?600:
		 hdr->strokeweight==3?700:
		 hdr->strokeweight==4?800:
		 hdr->strokeweight==5?900:
		 hdr->strokeweight==6?950:
		 hdr->strokeweight>=7?990:
		 hdr->strokeweight>=-3?300:
		 hdr->strokeweight==-4?200:
		 hdr->strokeweight==-5?100:
		 hdr->strokeweight==-6? 50: 10);	/* Weight class */
    putshort(os2,hdr->widthtype+5);			/* Width class */
    putshort(os2,0x0000);				/* Installable embedding (fstype) */
    putshort(os2,hdr->scale_factor*2/3);		/* ySubscript XSize */
    putshort(os2,hdr->scale_factor*2/3);		/* ySubscript YSize */
    putshort(os2,0);					/* ySubscript XOffset */
    putshort(os2,hdr->scale_factor*1/3);		/* ySubscript YOffset */
    putshort(os2,hdr->scale_factor*2/3);		/* ySuperscript XSize */
    putshort(os2,hdr->scale_factor*2/3);		/* ySuperscript YSize */
    putshort(os2,0);					/* ySuperscript XOffset */
    putshort(os2,-hdr->scale_factor*1/3);		/* ySuperscript YOffset */
    putshort(os2,hdr->scale_factor*102/2048);		/* strikeout width */
    putshort(os2,hdr->scale_factor*530/2048);		/* strikeout pos */
    putshort(os2,0);					/* no idea about ibm families */
    /* if we have panose data, use it, else the field is initted to 0 which */
    /*  is the default (or unknown, or something like that) */
    for ( i=0; i<10; ++i )
	putc(hdr->panose[i],os2);

    memset(uranges,0,sizeof(uranges)); memset(codeRanges,0,sizeof(codeRanges));
    minu = 0xffff;
    maxu = 0x0000;
    for ( i=0; i<hdr->actual_num; ++i ) if ( hdr->glyphs[i].unicode<0xffff ) {
	SetRanges(uranges,codeRanges,hdr->glyphs[i].unicode);
	if ( hdr->glyphs[i].unicode<minu )
	    minu = hdr->glyphs[i].unicode;
	if ( hdr->glyphs[i].unicode>maxu && hdr->glyphs[i].unicode<0xffff )
	    maxu = hdr->glyphs[i].unicode;
    }
	
    putlong(os2,uranges[0]);
    putlong(os2,uranges[1]);
    putlong(os2,uranges[2]);
    putlong(os2,uranges[3]);
    putc(' ',os2);putc(' ',os2);putc(' ',os2);putc(' ',os2); /* give up on vendor id */
    putshort(os2,fsstyle);
    putshort(os2,minu);					/* glyph index of minimum unicode */
    putshort(os2,maxu);					/* glyph index of maximum unicode */
    putshort(os2,gettableshort(hdr,CHR('h','h','e','a'),4));	/* Typo ascender, approx with apple's ascender (even though that's wrong) */
    putshort(os2,gettableshort(hdr,CHR('h','h','e','a'),6));	/* Typo descender, approx with apple's descender (even though that's wrong) */
    putshort(os2,gettableshort(hdr,CHR('h','h','e','a'),8));	/* Typo linegap, approx with apple's linegap (even though that's wrong) */
    putshort(os2,gettableshort(hdr,CHR('h','e','a','d'),42));	/* win ascent, approx with apple's ascender (even though that's wrong) */
    putshort(os2,-gettableshort(hdr,CHR('h','e','a','d'),38));	/* win descent, approx with apple's descender (even though that's wrong) */
    putlong(os2,codeRanges[0]);
    putlong(os2,codeRanges[1]);
    putshort(os2,hdr->heightx);					/* xheight */
    putshort(os2,hdr->capheight);				/* cap height */
    putshort(os2,0);						/* default char */
    putshort(os2,' ');						/* break enc */
    putshort(os2,1);						/* maxcontext */

    table.len = ftell(os2);
    table.offset = 0;
    table.tag = CHR('O','S','/','2');
    table.file = os2;
    table.closeme = true;
    AddTable(hdr,&table);
}

static void create_glyp_loca(FILE *pcl,struct ttf_header *hdr) {
    int numGlyphs = gettableshort(hdr,CHR('m','a','x','p'),4);	/* num glyph offset in maxp */
    int loca_is_big = gettableshort(hdr,CHR('h','e','a','d'),50);
    int *locas = malloc((numGlyphs+1)*sizeof(int));
    struct glyph **orderedglyphs = calloc(numGlyphs,sizeof(struct glyph *));
    int i;
    FILE *glyf = tmpfile(), *loca = tmpfile();
    struct subtables table;

    for ( i=0; i<hdr->actual_num; ++i ) {
	if ( hdr->glyphs[i].glyph>=numGlyphs ) {
	    fprintf( stderr, "Glyph outside of range specified by maxp.\n" );
	} else
	    orderedglyphs[hdr->glyphs[i].glyph] = &hdr->glyphs[i];
    }

    for ( i=0; i<numGlyphs; ++i ) {
	locas[i] = ftell(glyf);
	if ( orderedglyphs[i]!=NULL ) {
	    copydata(glyf,pcl,orderedglyphs[i]->offset,orderedglyphs[i]->len);
	    /* At most one continuation... */
	    if ( orderedglyphs[i]->continuation != NULL )
		copydata(glyf,pcl,orderedglyphs[i]->continuation->offset,orderedglyphs[i]->continuation->len);
	}
	if ( ftell(glyf)&1 )	/* short loca tables need everything word aligned */
	    putc('\0',glyf);
    }

    table.len = ftell(glyf);
    table.offset = 0;
    table.tag = CHR('g','l','y','f');
    table.file = glyf;
    table.closeme = true;
    AddTable(hdr,&table);

    locas[i] = ftell(glyf);
    regen_head(hdr,!loca_is_big && locas[i]>2*65535);
    if ( locas[i]>2*65535 )
	loca_is_big = true;
    if ( loca_is_big ) {
	for ( i=0; i<=numGlyphs; ++i )
	    putlong(loca,locas[i]);
    } else {
	for ( i=0; i<=numGlyphs; ++i )
	    putshort(loca,locas[i]/2);
    }

    table.len = ftell(loca);
    table.offset = 0;
    table.tag = CHR('l','o','c','a');
    table.file = loca;
    table.closeme = true;
    AddTable(hdr,&table);

    create_cmap(hdr,hdr->glyphs,hdr->actual_num);
    create_OS2(hdr,orderedglyphs,numGlyphs);
    free(orderedglyphs);
    free(locas);
}

static void create_post(struct ttf_header *hdr) {
    FILE *post = tmpfile();
    struct subtables table;

    putlong(post,0x00030000);		/* version 3, no glyph name data */
    putlong(post,0x00000000);		/* assume italic angle zero */
    putshort(post,hdr->mupos);
    putshort(post,hdr->muthick);
    putlong(post,!hdr->spacing);	/* monospace = !proportional*/
    putlong(post,0);			/* no idea about vm requirements */
    putlong(post,0);			/* no idea about vm requirements */
    putlong(post,0);			/* no idea about vm requirements */
    putlong(post,0);			/* no idea about vm requirements */

    table.len = ftell(post);
    table.offset = 0;
    table.tag = CHR('p','o','s','t');
    table.file = post;
    table.closeme = true;
    AddTable(hdr,&table);
}

static void create_name(struct ttf_header *hdr) {
    FILE *name = tmpfile();
    struct subtables table;
    /* Tiny name table with one entry containing the font's name (that's all we know) */
    /* oops, we might have a copyright */
    int cnt = 5+(hdr->copyright!=NULL);
    int off = 0;
    char *pt, *pt2;
    char family[20], style[20], *version="Version 1.0", unique[32];

    /* Guess at font family, style */
    strcpy(family,hdr->fontname);
    for ( pt=pt2=family; *pt!='\0'; ++pt )
	if ( *pt>' ' && *pt<0x7f && *pt!='(' && *pt!=')' && *pt!='{' && *pt!='}' &&
		*pt!='[' && *pt!=']' && *pt!='<' && *pt!='>' && *pt!='%' &&
		*pt!='/' )
	    *pt2++ = *pt;
    *pt2 = '\0';
    if ( (pt=strstr(family,"BdIt"))!=NULL )
	strcpy(style,"Bold Italic");
    else if ( (pt=strstr(family,"Bd"))!=NULL )
	strcpy(style,"Bold");
    else if ( (pt=strstr(family,"It"))!=NULL )
	strcpy(style,"Italic");
    else
	strcpy(style,"Regular");
    if ( pt!=NULL ) *pt='\0';
    sprintf( unique, "pcl2ttf: %s", hdr->fontname );

    putshort(name,0);			/* format 0 */
    putshort(name,cnt);			/* only one name */
    putshort(name,6+cnt*12);		/* offset to strings. 6 bytes of header, 12bytes for name records */

    if ( hdr->copyright!=NULL ) {
	putshort(name,3);		/* Platform = MS */
	putshort(name,1);		/* MS Unicode */
	putshort(name,0x409);		/* US English */
	putshort(name,0);		/* Copyright string */
	putshort(name,2*strlen(hdr->copyright));	/* It's unicode so 2 bytes/char */
	putshort(name,0);		/* start of string table */
	off = 2*strlen(hdr->copyright);
    }
    putshort(name,3);			/* Platform = MS */
    putshort(name,1);			/* MS Unicode */
    putshort(name,0x409);		/* US English */
    putshort(name,1);			/* family */
    putshort(name,2*strlen(family));	/* It's unicode so 2 bytes/char */
    putshort(name,off);			/* offset */
    off += 2*strlen(family);
    putshort(name,3);			/* Platform = MS */
    putshort(name,1);			/* MS Unicode */
    putshort(name,0x409);		/* US English */
    putshort(name,2);			/* style */
    putshort(name,2*strlen(style));	/* It's unicode so 2 bytes/char */
    putshort(name,off);			/* offset */
    off += 2*strlen(style);
    putshort(name,3);			/* Platform = MS */
    putshort(name,1);			/* MS Unicode */
    putshort(name,0x409);		/* US English */
    putshort(name,3);			/* style */
    putshort(name,2*strlen(unique));	/* It's unicode so 2 bytes/char */
    putshort(name,off);			/* offset */
    off += 2*strlen(unique);
    putshort(name,3);			/* Platform = MS */
    putshort(name,1);			/* MS Unicode */
    putshort(name,0x409);		/* US English */
    putshort(name,4);			/* full fontname */
    putshort(name,2*strlen(hdr->fontname));	/* It's unicode so 2 bytes/char */
    putshort(name,off);			/* offset */
    off += 2*strlen(hdr->fontname);
    putshort(name,3);			/* Platform = MS */
    putshort(name,1);			/* MS Unicode */
    putshort(name,0x409);		/* US English */
    putshort(name,5);			/* style */
    putshort(name,2*strlen(version));	/* It's unicode so 2 bytes/char */
    putshort(name,off);			/* offset */
    off += 2*strlen(version);


    if ( hdr->copyright!=NULL ) {
	for ( pt=hdr->copyright; *pt; ++pt ) {
	    putc('\0',name);
	    putc(*pt,name);
	}
    }
    for ( pt=family; *pt; ++pt ) {
	putc('\0',name);
	putc(*pt,name);
    }
    for ( pt=style; *pt; ++pt ) {
	putc('\0',name);
	putc(*pt,name);
    }
    for ( pt=unique; *pt; ++pt ) {
	putc('\0',name);
	putc(*pt,name);
    }
    for ( pt=hdr->fontname; *pt; ++pt ) {
	putc('\0',name);
	putc(*pt,name);
    }
    for ( pt=version; *pt; ++pt ) {
	putc('\0',name);
	putc(*pt,name);
    }

    table.len = ftell(name);
    table.offset = 0;
    table.tag = CHR('n','a','m','e');
    table.file = name;
    table.closeme = true;
    AddTable(hdr,&table);
}

static void create_missingtables(FILE *pcl,struct ttf_header *hdr) {
    create_glyp_loca(pcl,hdr);	/* and cmap and os2 */
    create_post(hdr);
    create_name(hdr);
}

/* ************************************************************************** */
/* ************************** Output final ttf file ************************* */
/* ************************************************************************** */

static int filecheck(FILE *file, int off, int len) {
    unsigned int sum = 0, chunk;
    int i, len4 = len/4;

    fseek(file,off,SEEK_SET);
    for ( i=0; i<len4; ++i ) {
	chunk = getlong(file);
	if ( feof(file))
    break;
	sum += chunk;
    }
    if ( (len&3)==1 )
	chunk = getc(file)<<24;
    else if ( (len&3)==2 )
	chunk = getshort(file)<<16;
    else if ( (len&3)==3 ) {
	chunk = getshort(file)<<16;
	chunk |= getc(file)<<8;
    } else
	chunk = 0;
    sum += chunk;
return( sum );
}

static void dumpfont(struct ttf_header *hdr) {
    int offset, i,n=hdr->subtable_cnt, sr, head_index;
    FILE *ttf;
    char buffer[32], *pt, *fpt;
    struct stat statb;

    fpt = hdr->fontname;
    for ( pt=buffer; *fpt; ++fpt )
	if ( *fpt!=' ' && *fpt!='/' )
	    *pt++ = *fpt;
    strcpy(pt,".ttf");
    ttf = fopen(buffer,"wb+");
    if ( ttf==NULL ) {
	fprintf( stderr, "Failed to open %s\n", buffer );
return;
    } else
	printf( "Created %s\n", buffer );
    for ( i=0; i<n; ++i )
	hdr->subtables[i].checksum = filecheck(hdr->subtables[i].file,
	    hdr->subtables[i].offset,hdr->subtables[i].len);
    offset = 12+16*n;
    for ( i=0; i<n; ++i ) {
	hdr->subtables[i].newoff = offset;
	offset += (hdr->subtables[i].len+3)&~3;
    }

    /* Dump truetype table header */
    sr = (n<16?8:n<32?16:n<64?32:64)*16;
    putlong(ttf,0x00010000);		/* Version */
    putshort(ttf,n);			/* table count */
    putshort(ttf,sr);			/* Search range */
    putshort(ttf,(n<16?3:n<32?4:n<64?5:6));
    putshort(ttf,n*16-sr);

    /* Dump table of contents */
    head_index = -1;
    for ( i=0; i<n; ++i ) {
	if ( hdr->subtables[i].tag==CHR('h','e','a','d') )
	    head_index = i;
	putlong(ttf,hdr->subtables[i].tag);
	putlong(ttf,hdr->subtables[i].checksum);
	putlong(ttf,hdr->subtables[i].newoff);
	putlong(ttf,hdr->subtables[i].len);
    }

    /* Dump each table */
    for ( i=0; i<n; ++i ) {
	if ( ftell(ttf)!=hdr->subtables[i].newoff )
	    fprintf( stderr, "Internal error, file offset wrong in final table dump\n" );

	copydata(ttf,hdr->subtables[i].file,
	    hdr->subtables[i].offset,hdr->subtables[i].len);
	if ( ftell(ttf)&1 )
	    putc('\0',ttf);
	if ( ftell(ttf)&2 )
	    putshort(ttf,0);

	if ( hdr->subtables[i].closeme )
	    fclose(hdr->subtables[i].file);
    }

    if ( head_index==-1 )
	fprintf(stderr, "Missing 'head' table\n" );
    else {
	int checksum = 0xb1b0afba - filecheck(ttf,0,ftell(ttf));
	fseek(ttf,hdr->subtables[head_index].newoff+8,SEEK_SET);
	putlong(ttf,checksum);
    }
    if ( ferror(ttf) )
	fprintf( stderr, "Error writing ttf file.\n" );
    fclose(ttf);
    stat(buffer,&statb);
    chmod(buffer,statb.st_mode|S_IXUSR|S_IXGRP|S_IXOTH);	/* Set the execute bits (in case it's windows) */
}

/* ************************************************************************** */
/* ******************************* bitmap font ****************************** */
/* ************************************************************************** */

static int readheaderbitmap(FILE *pcl,struct ttf_header *hdr) {
    int i, base;

    if ( hdr->header_format==20 ) {
	hdr->xres = getshort(pcl);
	hdr->yres = getshort(pcl);
    } else
	hdr->xres = hdr->yres = 300;

    if ( (hdr->header_format==0 && hdr->fd_size<64) ||
	    (hdr->header_format==20 && hdr->fd_size<68) ||
	    (hdr->header_format!=0 && hdr->header_format!=20)) {
	fprintf( stderr, "%s is not a PCL bitmap font\n", hdr->fontname );
return( false );
    }
    /* Variable data contain the copyright */
    base = hdr->header_format==0 ? 64 : 68;
    if ( hdr->fd_size>base ) {
	char *pt;
	hdr->copyright = pt = malloc(hdr->fd_size-base+1);
	for ( i=0; i<hdr->fd_size-base; ++i )
	    *pt++ = getc(pcl);
	*pt = '\0';
    }
return( true );
}

static int slurpbdf_glyph(FILE *pcl,struct ttf_header *hdr, FILE *bdf,
	int fbbox[4], int *cnt) {
    int encoding, size, ch;
    int desc_size, i;
    int class;
    int glyph_start;
    int restart = ftell(pcl);
    int left_off, top_off, width, height, deltax, bpl;

    if ( !skip2charcode(pcl)) {
	fprintf( stderr, "Fewer glyphs in font than expected\n" );
	fseek(pcl,restart,SEEK_SET);
return( false );
    }
    fscanf(pcl,"%d", &encoding );
    ch = getc(pcl);
    if ( ch!='E' ) {
	fprintf( stderr, "Fewer glyphs in font than expected\n" );
	fseek(pcl,restart,SEEK_SET);
return( false );
    }

    if ( !skip2glyph(pcl))
return( false );
    fscanf(pcl,"%d", &size );
    ch = getc(pcl);
    if ( ch!='W' ) {
	fprintf( stderr, "Bad font, glyph size does not end in \"W\"\n" );
return( false );
    }
    glyph_start = ftell(pcl);

    if ( getc(pcl)!=4 ) {
	fprintf( stderr, "Bad font, glyph format must be 4\n" );
return( false );
    }
    if ( getc(pcl)!=0 ) {
	fprintf( stderr, "Bad font, I don't support continuations\n" );
return( false );
    }
    desc_size = getc(pcl);
    if ( desc_size!=14 ) {
	fprintf( stderr, "Bad font, glyph descriptor size must be 14\n" );
return( false );
    }
    class = getc(pcl);
    if ( class!=1 && class!=2 ) {
	fprintf( stderr, "Bad font, glyph class must be 1 or 2\n" );
return( false );
    }
    getc(pcl);		/* orientation */
    getc(pcl);		/* reserved */
    left_off = getshort(pcl);
    top_off = getshort(pcl);
    width = getshort(pcl);
    height = getshort(pcl);
    deltax = getshort(pcl);

    if ( left_off<fbbox[2]) fbbox[2] = left_off;
    if ( left_off+width > fbbox[0] ) fbbox[0] = left_off+width;
    if ( top_off-height < fbbox[3] ) fbbox[3] = top_off-height;
    if ( top_off > fbbox[1] ) fbbox[1] = top_off;

    fprintf( bdf, "STARTCHAR glyph%d\n", *cnt++ );
    fprintf( bdf, "ENCODING %d\n", encoding );
    fprintf( bdf, "DWIDTH %d 0\n", deltax );
    fprintf( bdf, "BBX %d %d %d %d\n", width, height, left_off, top_off-height );
    fprintf( bdf, "BITMAP\n" );
    bpl = (width+7)/8;
    if ( class==2 ) {
	fprintf( stderr, "Warning: I don't support run length encoded glyphs\n" );
	i = 0;
    } else {
	for ( i=0; i<size-14 && i<height*bpl ; ++i ) {
	    int ch = getc(pcl);
	    if ( (ch>>4) >= 10 )
		putc((ch>>4)-10+'A',bdf);
	    else
		putc((ch>>4)+'0',bdf);
	    if ( (ch&0xf) >= 10 )
		putc((ch&0xf)-10+'A',bdf);
	    else
		putc((ch&0xf)+'0',bdf);
	    if ( i % bpl == bpl-1 )
		putc('\n',bdf);
	}
    }
    for ( ; i<height*bpl; ++i ) {
	putc('0',bdf);
	putc('0',bdf);
	if ( i % bpl == bpl-1 )
	    putc('\n',bdf);
    }

return( true );
}

static int slurpbdf_glyphs(FILE *pcl,struct ttf_header *hdr, FILE *bdf,
	int fbbox[4], int *cnt) {
    int i;

    for ( i=0; i<hdr->num_chars; ++i )
	if ( !slurpbdf_glyph(pcl,hdr,bdf,fbbox,cnt)) {
	    hdr->actual_num = i;
return( false );
	}
    hdr->actual_num = i;

return( true );
}

static int dumpbdffont(FILE *pcl,struct ttf_header *hdr) {
    FILE *bdf;
    char buffer[32], *pt, *fpt;
    char *weight, *expansion;
    int fbboxpos;
    int fbbox[4];	/* xmax, ymax, xmin, ymin */
    int cntpos;
    int cnt;

    fpt = hdr->fontname;
    for ( pt=buffer; *fpt; ++fpt )
	if ( *fpt!=' ' && *fpt!='/' )
	    *pt++ = *fpt;
    strcpy(pt,".bdf");
    bdf = fopen(buffer,"wb+");
    if ( bdf==NULL ) {
	fprintf( stderr, "Failed to open %s\n", buffer );
return( false );
    } else
	printf( "Created %s\n", buffer );

    fprintf( bdf, "STARTFONT 2.1\n" );
    *pt = '\0';
    weight = hdr->strokeweight<=-7 ? "UltraThin" :
	     hdr->strokeweight==-6 ? "ExtraThin" :
	     hdr->strokeweight==-5 ? "Thin" :
	     hdr->strokeweight==-4 ? "ExtraLight" :
	     hdr->strokeweight==-3 ? "Light" :
	     hdr->strokeweight==-2 ? "DemiLight" :
	     hdr->strokeweight==-1 ? "SemiLight" :
	     hdr->strokeweight==0 ? "Medium" :
	     hdr->strokeweight==1 ? "SemiBold" :
	     hdr->strokeweight==2 ? "DemiBold" :
	     hdr->strokeweight==3 ? "Bold" :
	     hdr->strokeweight==4 ? "ExtraBold" :
	     hdr->strokeweight==5 ? "Black" :
	     hdr->strokeweight==6 ? "ExtraBlack" : "UltraBlack";
    expansion = hdr->widthtype<=-5 ? "UltraCompressed" :
	     hdr->widthtype==-4 ? "ExtraCompressed" :
	     hdr->widthtype==-3 ? "Compressed" :
	     hdr->strokeweight<=-1 ? "Condensed" :
	     hdr->strokeweight==0 ? "Normal" :
	     hdr->strokeweight<=2 ? "Expanded" :
		 "ExtraExpanded";
    fprintf( bdf, "FONT -pcl2ttf-%s-%s-%c-%s--%d-%d-%d-%d-%c-%d-FontSpecific-1\n",
	    buffer,
	    weight,
	    (hdr->style&3)==1 || (hdr->style&3)==2 ? 'I' : 'R',
	    expansion,
	    hdr->height/4,		/* pixelsize in dots*/
	    (int) rint(720.0*hdr->height/hdr->yres/4),	/* pointsize * 10 */
	    hdr->xres,
	    hdr->yres,
	    hdr->spacing ? 'M' : 'P',	/* Monospace, proportional */
	    hdr->height/8		/* Guess. average char width */
	    );
    fprintf( bdf, "SIZE %d %d %d\n", (int) rint(72.0*hdr->height/hdr->yres/4),
	    hdr->xres, hdr->yres);
    fbboxpos = ftell(bdf);
    fprintf( bdf, "FONTBOUNDINGBOX %5d %5d %5d %5d\n", 0, 0, 0, 0 );
    if ( hdr->copyright!=NULL )
	fprintf( bdf, "COMMENT \"%s\"\n", hdr->copyright );
    fprintf( bdf, "STARTPROPERTIES %d\n", 15+( hdr->xres==hdr->yres )+( hdr->copyright!=NULL ) );
    fprintf( bdf, "FONT_NAME \"%s\"\n", buffer );
    fprintf( bdf, "FAMILY_NAME \"%s\"\n", buffer );
    fprintf( bdf, "FULL_NAME \"%s\"\n", buffer );
    fprintf( bdf, "FONT_ASCENT %d\n", hdr->baseline_pos );
    fprintf( bdf, "FONT_DESCENT %d\n", hdr->height/4-hdr->baseline_pos );
    fprintf( bdf, "UNDERLINE_POSITION %d\n", hdr->upos );
    fprintf( bdf, "UNDERLINE_THICKNESS %d\n", hdr->uthick );
    fprintf( bdf, "FOUNDRY \"pcl2ttf\"\n" );
    fprintf( bdf, "WEIGHT_NAME \"%s\"\n", weight );
    fprintf( bdf, "SETWIDTH_NAME \"%s\"\n", expansion );
    fprintf( bdf, "SLANT \"%c\"\n", (hdr->style&3)==1 || (hdr->style&3)==2 ? 'I' : 'R');
    fprintf( bdf, "PIXEL_SIZE %d\n", hdr->height/4);
    fprintf( bdf, "POINT_SIZE %d\n", (int) rint(720.0*hdr->height/hdr->yres/4));
    fprintf( bdf, "RESOLUTION_X %d\n", hdr->xres );
    fprintf( bdf, "RESOLUTION_Y %d\n", hdr->yres );
    if ( hdr->xres==hdr->yres )
	fprintf( bdf, "RESOLUTION %d\n", hdr->xres );
    fprintf( bdf, "SPACING \"%c\"\n", hdr->spacing ? 'M' : 'P');
    if ( hdr->copyright!=NULL )
	fprintf( bdf, "COPYRIGHT \"%s\"\n", hdr->copyright );
    fprintf( bdf, "ENDPROPERTIES\n" );

    fbbox[0] = fbbox[1] = -10000;
    fbbox[2] = fbbox[3] = 10000;

    cntpos = ftell(bdf);
    fprintf( bdf, "CHARS %3d\n", 0 );
    cnt = 0;

    slurpbdf_glyphs(pcl,hdr,bdf,fbbox, &cnt);

    fprintf( bdf, "ENDFONT\n" );
    
    if ( fbbox[0]==-10000 )
	fbbox[0] = fbbox[1] = fbbox[2] = fbbox[0] = 0;
    fseek(bdf,fbboxpos,SEEK_SET);
    fprintf( bdf, "FONTBOUNDINGBOX %5d %5d %5d %5d\n", fbbox[0], fbbox[1], fbbox[2], fbbox[3] );
    fseek(bdf,cntpos,SEEK_SET );
    fprintf( bdf, "CHARS %3d\n", cnt );
    fclose(bdf);
return( true );
}

/* ************************************************************************** */
/* **************************** ui, such as it is *************************** */
/* ************************************************************************** */

static void freedata(struct ttf_header *hdr) {
    int i;

    free(hdr->subtables);
    if ( hdr->glyphs!=NULL ) {
	for ( i=0; i<hdr->num_chars; ++i )
	    free( hdr->glyphs[i].continuation );
    }
    free(hdr->glyphs);
    free(hdr->copyright);
}

static void parsepclfile(char *filename) {
    FILE *pcl = fopen(filename,"rb");
    struct ttf_header hdr;

    if ( pcl==NULL ) {
	fprintf( stderr, "Can't open %s\n", filename );
return;
    }
    while ( skip2fontheader(pcl)) {
	readheaderformat(pcl,&hdr);
	if ( hdr.header_format==15 ) {		/* TrueType */
	    if ( readheaderttf(pcl,&hdr) ) {
		create_missingtables(pcl,&hdr);
		dumpfont(&hdr);
	    }
	} else if ( hdr.header_format==0 || hdr.header_format==20 ) {	/* Bitmap */
	    if ( readheaderbitmap(pcl,&hdr))
		dumpbdffont(pcl,&hdr);
	} else if ( hdr.header_format==10 || hdr.header_format==11 ) {
	    fprintf( stderr, "This file contains an Intellifont Scalable font.\nI have no docs on that format and can't help you.\n" );
	} else {
	    fprintf( stderr, "I don't recognize this font format. It isn't mentioned in my documentation.\nSorry.\n" );
	}
	freedata(&hdr);
    }
    fclose(pcl);
}

int main( int argc, char *argv[]) {
    int i;

    if ( argc==1 ) {
	fprintf( stderr, "Usage: %s file.pcl\n", argv[0] );
exit(1);
    }
    for ( i=1; i<argc; ++i )
	parsepclfile(argv[i]);
return( 0 );
}
