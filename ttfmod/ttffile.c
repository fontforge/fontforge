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
#include <chardata.h>
#include <charset.h>
#include <gfile.h>
#include <gwidget.h>
#include <math.h>

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

/* In table version numbers, the high order nibble of mantissa is in bcd, not hex */
/* I've no idea whether the lower order nibbles should be bcd or hex */
/* But let's assume some consistancy... */
real getvfixed(FILE *ttf) {
    int32 val = getlong(ttf);
    int mant = val&0xffff;
    mant = ((mant&0xf000)>>12)*1000 + ((mant&0xf00)>>8)*100 + ((mant&0xf0)>>4)*10 + (mant&0xf);
return( (real) (val>>16) + (mant/10000.0) );
}

real get2dot14(FILE *ttf) {
    int32 val = getushort(ttf);
    int mant = val&0x3fff;
    /* This oddity may be needed to deal with the first 2 bits being signed */
    /*  and the low-order bits unsigned */
return( (real) ((val<<16)>>(16+14)) + (mant/16384.0) );
}

Table *TableFind(TtfFont *tfont, int name) {
    int i;

    for ( i=0; i<tfont->tbl_cnt; ++i ) {
	if ( tfont->tbls[i]->name==name)
return( tfont->tbls[i] );
    }
return( NULL );
}

void TableFillup(Table *tbl) {
    int i;

    if ( tbl==NULL )
return;
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

/* In table version numbers, the high order nibble of mantissa is in bcd, not hex */
/* I've no idea whether the lower order nibbles should be bcd or hex */
/* But let's assume some consistancy... */
real tgetvfixed(Table *tab,int pos) {
    int32 val = tgetlong(tab,pos);
    int mant = val&0xffff;
    mant = ((mant&0xf000)>>12)*1000 + ((mant&0xf00)>>8)*100 + ((mant&0xf0)>>4)*10 + (mant&0xf);
return( (real) (val>>16) + (mant/10000.0) );
}

real tget2dot14(Table *tab,int pos) {
    int32 val = tgetushort(tab,pos);
    int mant = val&0x3fff;
    /* This oddity may be needed to deal with the first 2 bits being signed */
    /*  and the low-order bits unsigned */
return( (real) ((val<<16)>>(16+14)) + (mant/16384.0) );
}

int ptgetushort(uint8 *data) {
    int ch1 = data[0];
    int ch2 = data[1];
return( (ch1<<8)|ch2 );
}

int32 ptgetlong(uint8 *data) {
    int ch1 = data[0];
    int ch2 = data[1];
    int ch3 = data[2];
    int ch4 = data[3];
return( (ch1<<24)|(ch2<<16)|(ch3<<8)|ch4 );
}

real ptgetfixed(uint8 *data) {
    int32 val = ptgetlong(data);
    int mant = val&0xffff;
    /* This oddity may be needed to deal with the first 16 bits being signed */
    /*  and the low-order bits unsigned */
return( (real) (val>>16) + (mant/65536.0) );
}

real ptgetvfixed(uint8 *data) {
    int32 val = ptgetlong(data);
    int mant = val&0xffff;
    mant = ((mant&0xf000)>>12)*1000 + ((mant&0xf00)>>8)*100 + ((mant&0xf0)>>4)*10 + (mant&0xf);
return( (real) (val>>16) + (mant/10000.0) );
}

void putushort(FILE *file,uint16 val) {
    putc((val>>8),file);
    putc((val&0xff),file);
}

void putshort(FILE *file,uint16 val) {
    putc((val>>8),file);
    putc((val&0xff),file);
}

void putlong(FILE *file,uint32 val) {
    putc((val>>24)&0xff,file);
    putc((val>>16)&0xff,file);
    putc((val>>8)&0xff,file);
    putc((val&0xff),file);
}

void ptputushort(uint8 *data, uint16 val) {
    data[0] = (val>>8);
    data[1] = val&0xff;
}

void ptputlong(uint8 *data, uint32 val) {
    data[0] = (val>>24);
    data[1] = (val>>16)&0xff;
    data[2] = (val>>8)&0xff;
    data[3] = val&0xff;
}

void ptputfixed(uint8 *data,real val) {
    int ints = floor(val);
    int mant = (val-ints)*65536;
    int ival = (ints<<16) | mant;
    ptputlong(data,ival);
}

void ptputvfixed(uint8 *data,real val) {
    int ints = floor(val);
    int mant = (val-ints)*10000;
    int ival = ints<<16;

    ival |= (mant/1000)<<12;
    ival |= (mant/100%10)<<8;
    ival |= (mant/10%10)<<4;
    ival |= (mant%10);
    
    ptputlong(data,ival);
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
	if ( tf->tbls[i]->name == CHR('m','a','x','p'))
    break;
    if ( i==tf->tbl_cnt )
return( 0 );

    fseek(ttf,tf->tbls[i]->start+4,SEEK_SET);
    val = getushort(ttf);
    fseek(ttf,pos,SEEK_SET);
return(val);
}

static int checkfstype(FILE *ttf,TtfFont *tf) {
    long pos = ftell(ttf);
    int i, val;

    for ( i=0; i<tf->tbl_cnt; ++i )
	if ( tf->tbls[i]->name == CHR('O','S','/','2'))
    break;
    if ( i==tf->tbl_cnt )
return( true );

    fseek(ttf,tf->tbls[i]->start+8,SEEK_SET);
    val = getushort(ttf);
    fseek(ttf,pos,SEEK_SET);
    if ( (val&0xff)==0x0002 ) {
	static int buts[] = { _STR_Yes, _STR_No, 0 };
	if ( GWidgetAskR(_STR_RestrictedFont,buts,1,1,_STR_RestrictedRightsFont)==1 ) {
return( false );
	}
    }
return(true);
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

static void free_enctabledata(void *_data) {
    struct enctab *enc, *next;

    for ( enc=_data; enc!=NULL; enc = next ) {
	next = enc->next;
	free(enc->enc);
	if ( enc->uenc!=enc->enc )	/* Unicode fonts can share enc and uenc */
	    free(enc->uenc);
	free(enc);
    }
}

static void write_enctabledata(FILE *tottf,Table *cmap) {
    /* !!!! */
}

void readttfencodings(FILE *ttf,struct ttffont *font) {
    int i,j;
    int nencs, version;
    int len;
    uint16 table[256];
    int segCount;
    uint16 *endchars, *startchars, *delta, *rangeOffset, *glyphs;
    int index;
    Table *tab;
    const unichar_t *trans=NULL;
    struct enctab *enc, *last=NULL, *best=NULL;
    int bestval=0;

/* find the cmap (encoding) table */
    for ( i=0; i<font->tbl_cnt; ++i )
	if ( font->tbls[i]->name == CHR('c','m','a','p'))
    break;
    if ( i==font->tbl_cnt )
return;
    tab = font->tbls[i];

  if ( tab->table_data == NULL ) {
/* Find out what encodings are defined and where they live */
    fseek(ttf,tab->start,SEEK_SET);
    version = getushort(ttf);
    nencs = getushort(ttf);
    for ( i=0; i<nencs; ++i ) {
	enc = gcalloc(1,sizeof(struct enctab));
	enc->platform = getushort(ttf);
	enc->specific = getushort(ttf);
	enc->offset = getlong(ttf);
	if ( last==NULL )
	    tab->table_data = enc;
	else
	    last->next = enc;
	last = enc;
    }
    tab->free_tabledata = free_enctabledata;
    tab->write_tabledata = write_enctabledata;

/* read in each encoding table (presuming we understand it) */
    for ( enc = tab->table_data; enc!=NULL; enc=enc->next ) {
	enc->cnt = font->glyph_cnt;
	enc->enc = galloc(enc->cnt*sizeof(unichar_t));
	memset(enc->enc,'\377',enc->cnt*sizeof(unichar_t));
	fseek(ttf,tab->start+enc->offset,SEEK_SET);
	enc->format = getushort(ttf);
	enc->len = getushort(ttf);
	enc->language = getushort(ttf);		/* or version for ms */
	if ( enc->format==0 ) {
	    for ( i=0; i<len-6; ++i )
		table[i] = getc(ttf);
	    for ( i=0; i<256 && table[i]<enc->cnt && i<enc->len-6; ++i )
		enc->enc[table[i]] = i;
	} else if ( enc->format==4 ) {
	    segCount = getushort(ttf)/2;
	    /* searchRange = */ getushort(ttf);
	    /* entrySelector = */ getushort(ttf);
	    /* rangeShift = */ getushort(ttf);
	    endchars = galloc(segCount*sizeof(uint16));
	    for ( i=0; i<segCount; ++i )
		endchars[i] = getushort(ttf);
	    if ( getushort(ttf)!=0 )
		GDrawIError("Expected 0 in true type font");
	    startchars = galloc(segCount*sizeof(uint16));
	    for ( i=0; i<segCount; ++i )
		startchars[i] = getushort(ttf);
	    delta = galloc(segCount*sizeof(uint16));
	    for ( i=0; i<segCount; ++i )
		delta[i] = getushort(ttf);
	    rangeOffset = galloc(segCount*sizeof(uint16));
	    for ( i=0; i<segCount; ++i )
		rangeOffset[i] = getushort(ttf);
	    len = enc->len- 8*sizeof(uint16) +
		    4*segCount*sizeof(uint16);
	    /* that's the amount of space left in the subtable and it must */
	    /*  be filled with glyphIDs */
	    glyphs = galloc(len);
	    for ( i=0; i<len/2; ++i )
		glyphs[i] = getushort(ttf);
	    for ( i=0; i<segCount; ++i ) {
		if ( rangeOffset[i]==0 && startchars[i]==0xffff )
		    /* Done */;
		else if ( rangeOffset[i]==0 ) {
		    for ( j=startchars[i]; j<=endchars[i]; ++j ) {
			if ( enc->enc[(uint16) (j+delta[i])]==0xffff )
			    enc->enc[(uint16) (j+delta[i])] = j;	/* Only use first enc, may be several */
		    }
		} else if ( rangeOffset[i]!=0xffff ) {
		    /* It isn't explicitly mentioned by a rangeOffset of 0xffff*/
		    /*  means no glyph */
		    for ( j=startchars[i]; j<=endchars[i]; ++j ) {
			index = glyphs[ (i-segCount+rangeOffset[i]/2) +
					    j-startchars[i] ];
			if ( index!=0 ) {
			    index = (unsigned short) (index+delta[i]);
			    if ( index>=enc->cnt ) GDrawIError( "Bad index" );
			    else if ( enc->enc[index]==0xffff ) enc->enc[index] = j;
			}
		    }
		}
	    }
	    free(glyphs);
	    free(rangeOffset);
	    free(delta);
	    free(startchars);
	    free(endchars);
	} else if ( enc->format==6 ) {
	    /* Apple's unicode format */
	    int first, count;
	    first = getushort(ttf);
	    count = getushort(ttf);
	    for ( i=0; i<count; ++i ) {
		j = getushort(ttf);
		enc->enc[j] = first+i;
	    }
	} else {
	    free(enc->enc); enc->enc=NULL;
	    if ( enc->format==2 ) {
		fprintf(stderr,"I don't support mixed 8/16 bit characters (no shit jis for me)");
	    } else if ( enc->format==8 ) {
		fprintf(stderr,"I don't support mixed 16/32 bit characters (no unicode surogates)");
	    } else if ( enc->format==10 || enc->format==12 ) {
		fprintf(stderr,"I don't support 32 bit characters");
	    } else {
		fprintf(stderr,"I don't understand this format type at all in a cmap table %d\n", enc->format );
	    }
	}
    }

/* Convert each table to unicode (if we can) */
    for ( enc = tab->table_data; enc!=NULL; enc=enc->next ) if ( enc->enc!=NULL ) {
	enum charset type = em_none;
	switch ( enc->platform ) {
	  case 0: /* Unicode */
	  case 2: /* Obsolete ISO 10646 */
	    type = em_unicode;
	    /* the various specific values say what version of unicode. I'm not */
	    /* keeping track of that (no mapping table of unicode1->3) */
	    /* except for CJK it's mostly just extensions */
	  break;
	  case 1:
	    switch ( enc->specific ) {
	      case 0: type = em_mac; break;	/* Mac Roman */
	      /* 1, Japanese */
	      /* 2, Trad Chinese */
	      /* 3, Korean */
	      /* 4, Arabic */
	      /* 5, Hebrew */
	      /* 6, Greek */
	      /* 7, Russian */
	      /* 8, RSymbol */
	      /* 9, Devanagari */
	      /* 10, Gurmukhi */
	      /* 11, Gujarati */
	      /* 12, Oriya */
	      /* 13, Bengali */
	      /* 14, Tamil */
	      /* 15, Telugu */
	      /* 16, Kannada */
	      /* 17, Malayalam */
	      /* 18, Sinhalese */
	      /* 19, Burmese */
	      /* 20, Khmer */
	      /* 21, Thai */
	      /* 22, Laotian */
	      /* 23, Georgian */
	      /* 24, Armenian */
	      /* 25, Simplified Chinese */
	      /* 26, Tibetan */
	      /* 27, Mongolian */
	      /* 28, Geez */
	      /* 29, Slavic */
	      /* 30, Vietnamese */
	      /* 31, Sindhi */
	    }
	  break;
	  case 3:		/* MS */
	    switch ( enc->specific ) {
	      case 0: type = em_symbol; break;
	      case 1: type = em_unicode; break;
	      case 2: type = em_jis208; break;
	      case 3: type = em_big5; break;
	      /* 4, PRC */
	      case 5: type = em_ksc5601; break;
	      /* 6, Johab */
	      case 10: type = em_unicode; break;	/* 4byte iso10646 */
	    }
	  break;
	}
	if ( type==em_unicode )
	    enc->uenc = enc->enc;
	else if ( type!=em_none ) {
	    enc->uenc = galloc(enc->cnt*sizeof(unichar_t));
	    memset(enc->uenc,'\377',enc->cnt*sizeof(unichar_t));
	    trans = NULL;
	    if ( type==em_mac )
		trans = unicode_from_mac;
	    else if ( type==em_symbol )
		trans = unicode_from_MacSymbol;
	    for ( i=0; i<enc->cnt; ++i ) if ( enc->enc[i]!=0xffff ) {
		if ( trans!=NULL )
		    enc->uenc[i] = trans[enc->enc[i]];
		else if ( type==em_big5 ) {
		    if ( enc->enc[i]>0xa100 )
			enc->uenc[i] = unicode_from_big5[enc->enc[i]-0xa100];
		    else if ( enc->enc[i]>0x100 )
			enc->uenc[i] = 0xffff;
		    else
			enc->uenc[i] = enc->enc[i];
		} else if ( type == em_ksc5601 ) {
		    int val = enc->enc[i];
		    if ( val>0xa1a1 ) {
			val -= 0xa1a1;
			val = (val>>8)*94 + (val&0xff);
			val = unicode_from_ksc5601[val];
			if ( val==0 ) val = -1;
		    } else if ( val>0x100 )
			val = -1;
		    enc->uenc[i] = val;
		} else if ( type==em_jis208 ) {
		    int val = enc->enc[i];
		    if ( val<=127 ) {
			/* Latin */
			if ( val=='\\' ) val = 0xa5;	/* Yen */
		    } else if ( val>=161 && val<=223 ) {
			/* Katakana */
			val = unicode_from_jis201[val];
		    } else {
			int ch1 = val>>8, ch2 = val&0xff;
			if ( ch1 >= 129 && ch1<= 159 )
			    ch1 -= 112;
			else
			    ch1 -= 176;
			ch1 <<= 1;
			if ( ch2>=159 )
			    ch2-= 126;
			else if ( ch2>127 ) {
			    --ch1;
			    ch2 -= 32;
			} else {
			    --ch1;
			    ch2 -= 31;
			}
			val = unicode_from_jis208[(ch1-0x21)*94+(ch2-0x21)];
		    }
		    enc->uenc[i] = val;
		} else {
		    GDrawIError("Eh? Unsupported encoding %d", type );
		}
	    }
	}
    }
  }

/* Find the best table we can */
    for ( enc = tab->table_data; enc!=NULL; enc=enc->next ) {
	if ( enc->uenc==NULL )
	    /* Unparseable, unuseable */;
	else if (( enc->platform==3 && enc->specific==1 ) ||	/* MS Unicode */
		(enc->platform==0 && (enc->specific==0 || enc->specific==3))) {	/* Apple unicode */
	    bestval = 3;
	    best = enc;
	} else if ( enc->platform==3 && enc->specific==0 && best==NULL ) {
	    /* Only select symbol if we don't have something better */
	    best = enc;
	    bestval = 1;
	} else if ( ((enc->platform==1 && enc->specific==0 ) || enc->platform==3 ) &&
		bestval!=3 ) {
	    /* Mac 8bit/CJK if no unicode */
	    best = enc;
	    bestval = 2;
	}
    }
    font->enc = best;
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
	    if ( table->start==offset && table->len==length ) {
		if ( table->name==name )
return( table );
		/* EBDT/bdat, EBLC/bloc use the same structure and could share tables */
		for ( i=0; i<sizeof(table->othernames)/sizeof(table->othernames[0]); ++i ) {
		    if ( table->othernames[i]==name || table->othernames[i]==0 ) {
			table->othernames[i] = name;
return( table );
		    }
		}
	    }
	}
    }

    table = gcalloc(1,sizeof(Table));
    table->name = name;
    if ( name==CHR('g','l','y','f') || name==CHR('l','o','c','a') )
	table->special = true;
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
    if ( tf->fontname==NULL )
	fprintf(stderr, "This font has no name. That will cause problems\n" );
    tf->glyph_cnt = TTFGetGlyphCnt(ttf,tf);
    readttfencodings(ttf,tf);
return( tf );
}

static TtfFile *readttfheader(FILE *ttf,char *filename) {
    TtfFile *f = gcalloc(1,sizeof(TtfFile));

    f->filename = copy(filename);
    f->file = ttf;
    f->font_cnt = 1;
    f->fonts = gcalloc(1,sizeof(TtfFont *));
    f->fonts[0] = _readttfheader(ttf,f);
    if ( !checkfstype(ttf,f->fonts[0]))
return( NULL );

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
    if ( !checkfstype(ttf,f->fonts[0]))
return( NULL );

return( f );
}

TtfFile *ReadTtfFont(char *filename) {
    FILE *ttf = fopen(filename,"r");
    int32 version;
    char absolute[1025];

    if ( ttf==NULL )
return( NULL );
    if ( *filename!='/' )
	filename = GFileGetAbsoluteName(filename,absolute,sizeof(absolute));
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

void TTFFileFreeData(TtfFile *ttf) {
    int i,j;
    TtfFont *font;
    Table *tab;

    for ( i=0; i<ttf->font_cnt; ++i ) {
	font = ttf->fonts[i];
	for ( j=0; j<font->tbl_cnt; ++j ) {
	    tab = font->tbls[j];
	    if ( tab->table_data ) {
		(tab->free_tabledata)( tab->table_data );
		tab->table_data = NULL;
	    }
	    if ( tab->data ) {
		free( tab->data );
		tab->data = NULL;
	    }
	    tab->changed = tab->td_changed = false;
	}
    }
    ttf->changed = ttf->gcchanged = false;
}

void TTFFileFree(TtfFile *ttf) {
    int i,j,cnt;
    Table **tabs;
    TtfFont *font;

    TTFFileFreeData(ttf);

    for ( i=cnt=0; i<ttf->font_cnt; ++i )
	cnt += ttf->fonts[i]->tbl_cnt;
    tabs = galloc((cnt+1)*sizeof(Table *));
    for ( i=cnt=0; i<ttf->font_cnt; ++i ) {
	font = ttf->fonts[i];
	for ( j=0; j<font->tbl_cnt; ++j ) {
	    if ( !font->tbls[j]->freeing ) {
		font->tbls[j]->freeing = true;
		tabs[cnt++] = font->tbls[j];
	    }
	}
	free(font->tbls);
	free(font);
    }
    for ( i=0; i<cnt; ++i )
	free(tabs[i]);
    free(tabs);

    free(ttf->fonts);
    free(ttf);
}
