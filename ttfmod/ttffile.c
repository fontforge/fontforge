/* Copyright (C) 2001 by George Williams */
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
#include "ttfmod.h"
#include <ustring.h>

int getushort(FILE *ttf) {
    int ch1 = getc(ttf);
    int ch2 = getc(ttf);
    if ( ch2==EOF )
return( EOF );
return( (ch1<<8)|ch2 );
}

int32 getlong(FILE *ttf) {
    int ch1 = getc(ttf);
    int ch2 = getc(ttf);
    int ch3 = getc(ttf);
    int ch4 = getc(ttf);
    if ( ch4==EOF )
return( EOF );
return( (ch1<<24)|(ch2<<16)|(ch3<<8)|ch4 );
}

real getfixed(FILE *ttf) {
    int32 val = getlong(ttf);
    int mant = val&0xffff;
    /* This oddity may be needed to deal with the first 16 bits being signed */
    /*  and the low-order bits unsigned */
return( (real) (val>>16) + (mant/65536.0) );
}

real get2dot14(FILE *ttf) {
    int32 val = getushort(ttf);
    int mant = val&0x3fff;
    /* This oddity may be needed to deal with the first 2 bits being signed */
    /*  and the low-order bits unsigned */
return( (real) ((val<<16)>>(16+14)) + (mant/16384.0) );
}

void TableFillup(Table *tbl) {
    int i;

    if ( tbl->data!=NULL )
return;
    tbl->data = galloc( ((tbl->len+3)/4)*4 );
    fseek(tbl->container->file,tbl->start,SEEK_SET);
    fread(tbl->data,1,tbl->len,tbl->container->file);
    for ( i=tbl->len; i<((tbl->len+3)/4)*4; ++i )
	tbl->data[i] = 0;
}

int tgetushort(Table *tab,int pos) {
    int ch1 = tab->data[pos];
    int ch2 = tab->data[pos+1];
    if ( pos+1>=tab->newlen )
return( EOF );
return( (ch1<<8)|ch2 );
}

int32 tgetlong(Table *tab,int pos) {
    int ch1 = tab->data[pos];
    int ch2 = tab->data[pos+1];
    int ch3 = tab->data[pos+2];
    int ch4 = tab->data[pos+3];
    if ( pos+3>=tab->newlen )
return( EOF );
return( (ch1<<24)|(ch2<<16)|(ch3<<8)|ch4 );
}

real tgetfixed(Table *tab,int pos) {
    int32 val = tgetlong(tab,pos);
    int mant = val&0xffff;
    /* This oddity may be needed to deal with the first 16 bits being signed */
    /*  and the low-order bits unsigned */
return( (real) (val>>16) + (mant/65536.0) );
}

real tget2dot14(Table *tab,int pos) {
    int32 val = tgetushort(tab,pos);
    int mant = val&0x3fff;
    /* This oddity may be needed to deal with the first 2 bits being signed */
    /*  and the low-order bits unsigned */
return( (real) ((val<<16)>>(16+14)) + (mant/16384.0) );
}

static unichar_t *_readustring(FILE *ttf,int offset,int len) {
    long pos = ftell(ttf);
    unichar_t *str, *pt;
    int i, ch;

    fseek(ttf,offset,SEEK_SET);
    str = pt = galloc(len+2);
    for ( i=0; i<len/2; ++i ) {
	ch = getc(ttf)<<8;
	*pt++ = ch | getc(ttf);
    }
    *pt = '\0';
    fseek(ttf,pos,SEEK_SET);
return( str );
}

static int TTFGetGlyphCnt(FILE *ttf,TtfFont *tf) {
    long pos = ftell(ttf);
    int i, val;

    for ( i=0; i<tf->tbl_cnt; ++i )
	if ( tf->tbls[i]->name == CHR('n','a','m','e'))
    break;
    if ( i==tf->tbl_cnt )
return( 0 );

    fseek(ttf,tf->tbls[i]->start+4,SEEK_SET);
    val = getushort(ttf);
    fseek(ttf,pos,SEEK_SET);
return(pos);
}

static unichar_t *TTFGetFontName(FILE *ttf,TtfFont *tf) {
    int i,num;
    int32 nameoffset, stringoffset;
    int plat, spec, lang, name, len, off, val;
    int fullval, fullstr, fulllen, famval, famstr, famlen;

    for ( i=0; i<tf->tbl_cnt; ++i )
	if ( tf->tbls[i]->name == CHR('n','a','m','e'))
    break;
    if ( i==tf->tbl_cnt )
return( uc_copy("<nameless>"));

    nameoffset = tf->tbls[i]->start;
    fseek(ttf,nameoffset,SEEK_SET);
    /* format = */ getushort(ttf);
    num = getushort(ttf);
    stringoffset = nameoffset+getushort(ttf);
    fullval = famval = 0;
    for ( i=0; i<num; ++i ) {
	plat = getushort(ttf);
	spec = getushort(ttf);
	lang = getushort(ttf);
	name = getushort(ttf);
	len = getushort(ttf);
	off = getushort(ttf);
	val = 0;
	if ( plat==0 && /* any unicode semantics will do && */ lang==0 )
	    val = 1;
	else if ( plat==3 && spec==1 && lang==0x409 )
	    val = 2;
	if ( name==4 && val>fullval ) {
	    fullval = val;
	    fullstr = off;
	    fulllen = len;
	    if ( val==2 )
    break;
	} else if ( name==1 && val>famval ) {
	    famval = val;
	    famstr = off;
	    famlen = len;
	}
    }
    if ( fullval==0 ) {
	if ( famval==0 )
return( NULL );
	fullstr = famstr;
	fulllen = famlen;
    }
return( _readustring(ttf,stringoffset+fullstr,fulllen));
}

static Table *readtablehead(FILE *ttf,TtfFile *f) {
    int32 name = getlong(ttf);
    int32 checksum = getlong(ttf);
    int32 offset = getlong(ttf);
    int32 length = getlong(ttf);
    Table *table;
    int i,j;

    /* In a TTC file some tables may be shared, check through previous fonts */
    /*  in the file to see if we've got this already */
    for ( i=0; i<f->font_cnt && f->fonts[i]!=NULL; ++i ) {
	for ( j=0; j<f->fonts[i]->tbl_cnt; ++j ) {
	    table = f->fonts[i]->tbls[j];
	    if ( table->name==name && table->start==offset && table->len==length )
return( table );
	}
    }

    table = gcalloc(1,sizeof(Table));
    table->name = name;
    table->oldchecksum = checksum;
    table->start = offset;
    table->len = table->newlen = length;
    table->container = f;
return( table );
}

static TtfFont *_readttfheader(FILE *ttf,TtfFile *f) {
    TtfFont *tf = gcalloc(1,sizeof(TtfFont));
    int i;

    tf->version = getlong(ttf);
    tf->tbl_cnt = tf->tbl_max = getushort(ttf);
    tf->container = f;
    /* searchRange = */ getushort(ttf);
    /* entrySelector = */ getushort(ttf);
    /* rangeshift = */ getushort(ttf);
    tf->tbls = galloc(tf->tbl_cnt*sizeof(Table *));
    for ( i=0; i<tf->tbl_cnt; ++i )
	tf->tbls[i] = readtablehead(ttf,f);
    tf->fontname = TTFGetFontName(ttf,tf);
    tf->glyph_cnt = TTFGetGlyphCnt(ttf,tf);
return( tf );
}

static TtfFile *readttfheader(FILE *ttf,char *filename) {
    TtfFile *f = gcalloc(1,sizeof(TtfFile));

    f->filename = copy(filename);
    f->file = ttf;
    f->font_cnt = 1;
    f->fonts = gcalloc(1,sizeof(TtfFont *));
    f->fonts[0] = _readttfheader(ttf,f);
return( f );
}

static TtfFile *readttcfheader(FILE *ttf,char *filename) {
    TtfFile *f = gcalloc(1,sizeof(TtfFile));
    int i;
    int32 *offsets;

    /* TTCF version = */ getlong(ttf);
    f->filename = copy(filename);
    f->file = ttf;
    f->is_ttc = true;
    f->font_cnt = getlong(ttf);
    f->fonts = gcalloc(f->font_cnt,sizeof(TtfFont *));
    offsets = galloc(f->font_cnt*sizeof(int32));
    for ( i=0; i<f->font_cnt; ++i )
	offsets[i] = getlong(ttf);
    for ( i = 0; i<f->font_cnt; ++i ) {
	fseek(ttf,offsets[i],SEEK_SET);
	f->fonts[i] = _readttfheader(ttf,f);
    }
    free(offsets);
return( f );
}

TtfFile *ReadTtfFont(char *filename) {
    FILE *ttf = fopen(filename,"r");
    int32 version;

    if ( ttf==NULL )
return( NULL );
    version=getlong(ttf);
    if ( version==CHR('t','t','c','f'))
return( readttcfheader(ttf,filename));
    if ( version==0x00010000 || version == CHR('O','T','T','O')) {
	fseek(ttf,0,SEEK_SET);
return( readttfheader(ttf,filename));
    }
    /* We leave the file open for further reads, unless it's not a ttf file */
    fclose(ttf);
return( NULL );
}
