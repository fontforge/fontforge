/* Copyright (C) 2000-2002 by George Williams */
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
#include <chardata.h>
#include <utype.h>
#include <ustring.h>
#include <math.h>
#include <locale.h>
#include <gwidget.h>
#include "ttf.h"

/* True Type is a really icky format. Nothing is together. It's badly described */
/*  much of the description is misleading */
/* Apple's version: */
/*  http://fonts.apple.com/TTRefMan/index.html */
/* MS's version: */
/*  http://www.microsoft.com/typography/tt/tt.htm */
/* An helpful but incomplete description is given at */
/*  http://www.truetype.demon.co.uk/ttoutln.htm */
/* For some things I looked at freetype's code to see how they did it */
/*  (I think only for what happens if !ARGS_ARE_XY) */
/*  http://freetype.sourceforge.net/ */

/* !!!I don't currently parse instructions to get hints */

int getushort(FILE *ttf) {
    int ch1 = getc(ttf);
    int ch2 = getc(ttf);
    if ( ch2==EOF )
return( EOF );
return( (ch1<<8)|ch2 );
}

int get3byte(FILE *ttf) {
    int ch1 = getc(ttf);
    int ch2 = getc(ttf);
    int ch3 = getc(ttf);
    if ( ch3==EOF )
return( EOF );
return( (ch1<<16)|(ch2<<8)|ch3 );
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

static int32 getoffset(FILE *ttf, int offsize) {
    if ( offsize==1 )
return( getc(ttf));
    else if ( offsize==2 )
return( getushort(ttf));
    else if ( offsize==3 )
return( get3byte(ttf));
    else
return( getlong(ttf));
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

static unichar_t *_readencstring(FILE *ttf,int offset,int len,int enc) {
    long pos = ftell(ttf);
    unichar_t *str, *pt, empty[1];
    int i, ch;

    fseek(ttf,offset,SEEK_SET);
    if ( enc==em_mac ) {
	str = pt = galloc(2*len+2);
	for ( i=0; i<len; ++i )
	    *pt++ = unicode_from_mac[getc(ttf)];
    } else if ( enc==em_adobestandard ) {
	str = pt = galloc(2*len+2);
	for ( i=0; i<len; ++i )
	    *pt++ = unicode_from_adobestd[getc(ttf)];
    } else if ( enc==em_unicode ) {
	str = pt = galloc(len+2);
	for ( i=0; i<len/2; ++i ) {
	    ch = getc(ttf)<<8;
	    *pt++ = ch | getc(ttf);
	}
    } else if ( enc==em_big5 ) {
	str = pt = galloc(2*len+2);	/* Probably more space than we need, but it should be enough */
	for ( i=0; i<len; ++i ) {
	    ch = getc(ttf);
	    if ( ch==0 ) {
		/* Interesting. I've never seen this described, but empirically */
		/*  it appears that a leading 0 byte means an ascii byte follows */
		/*  I thought we got mixed 8/16 encoding, not straight 16... */
		*pt++ = getc(ttf);
		++i;
	    } else if ( ch<0xa1 )
		*pt++ = ch;
	    else {
		ch = ((ch<<8)|getc(ttf));
		*pt++ = unicode_from_big5[ch-0xa100];
		++i;
	    }
	}
    } else if ( enc==em_johab ) {
	str = pt = galloc(2*len+2);	/* Probably more space than we need, but it should be enough */
	for ( i=0; i<len; ++i ) {
	    ch = getc(ttf);
	    if ( ch<0x84 )
		*pt++ = ch;
	    else {
		ch = ((ch<<8)|getc(ttf));
		*pt++ = unicode_from_johab[ch-0x8400];
		++i;
	    }
	}
    } else if ( enc==em_wansung ) {
	str = pt = galloc(2*len+2);	/* Probably more space than we need, but it should be enough */
	for ( i=0; i<len; ++i ) {
	    ch = getc(ttf);
	    if ( ch<0xa1 )
		*pt++ = ch;
	    else {
		ch = ((ch<<8)|getc(ttf)) - 0xa1a1;
		ch = (ch>>8)*94 + (ch&0xff);
		*pt++ = unicode_from_ksc5601[ch];
		++i;
	    }
	}
    } else if ( enc==em_sjis ) {
	str = pt = galloc(2*len+2);	/* Probably more space than we need, but it should be enough */
	for ( i=0; i<len; ++i ) {
	    ch = getc(ttf);
	    if ( ch<127 ) {
		if ( ch=='\\' ) ch= 0xa5;		/* Yen */
		*pt++ = ch;
	    } else if ( ch>=161 && ch<=223 ) {
		*pt++ = unicode_from_jis201[ch];	/* Katakana */
	    } else {
		int ch2 = getc(ttf);
		if ( ch >= 129 && ch<= 159 )
		    ch -= 112;
		else
		    ch -= 176;
		ch <<= 1;
		if ( ch2>=159 )
		    ch2-= 126;
		else if ( ch2>127 ) {
		    --ch;
		    ch2 -= 32;
		} else {
		    --ch;
		    ch2 -= 31;
		}
		*pt++ = unicode_from_jis208[(ch-0x21)*94+(ch2-0x21)];
		++i;
	    }
	}
    } else {
	fprintf( stderr, "Unexpected encoding %d\n", enc );
	str = NULL; pt = empty;
    }
    *pt = '\0';
    fseek(ttf,pos,SEEK_SET);
return( str );
}

static int enc_from_platspec(int platform,int specific) {
    int enc;

    enc = em_none;
    if ( platform==0 )
	enc = em_unicode;
    else if ( platform==1 ) {
	if ( specific==0 )
	    enc = em_mac;
	else if ( specific==1 )
	    enc = em_sjis;
	else if ( specific==2 )
	    enc = em_big5;
	else if ( specific==3 )
	    enc = em_wansung;
    } else if ( platform==2 ) {		/* obselete */
	if ( specific==0 )
	    enc = em_iso8859_1;		/* Actually just ASCII */
	else if ( specific==1 )
	    enc = em_unicode;
	else if ( specific==2 )
	    enc = em_iso8859_1;
    } else if ( platform==3 ) {
	if ( specific==1 || specific==0 )	/* symbol (sp=0) is just unicode */
	    enc = em_unicode;
	else if ( specific==2 )
	    enc = em_sjis;
	else if ( specific==5 )
	    enc = em_wansung;
	else if ( specific==4 )
	    enc = em_big5;
	else if ( specific==6 )
	    enc = em_johab;
    } else if ( platform==7 ) {		/* Used internally in freetype, but */
	if ( specific==0 )		/*  there's no harm in looking for it */
	    enc = em_adobestandard;	/*  even if it never happens */
	else if ( specific==1 )
	    /* adobe_expert */;
	else if ( specific==2 )
	    /* adobe_custom */;
    }
return( enc );
}

static int AppleLang_to_MS(int language) {
    switch ( language ) {
      case 0:		/* English */
return( 0x409 );
      case 36:		/* Albanian */
return( 0x41c );
      case 12:		/* Arabic */
return( 0x401 );
      case 129:		/* Basque */
return( 0x42d );
      case 46:		/* Byelorussian */
return( 0x423 );
      case 44:		/* Bulgarian */
return( 0x402 );
      case 130:		/* Catalan */
return( 0x403 );
      case 19:		/* Traditional Chinese */
return( 0x404 );
      case 33:		/* Simplified Chinese */
return( 0x04 );			/* Not sure about this MS value... */
      case 18:		/* Croatian */
return( 0x41a );
      case 38:		/* Czech */
return( 0x405 );
      case 7:		/* Danish */
return( 0x406 );
      case 4:		/* Dutch */
return( 0x413 );
      case 34:		/* Flemish */
return( 0x813 );
      case 27:		/* Estonian */
return( 0x425 );
      case 13:		/* Finnish */
return( 0x40b );
      case 1:		/* French */
return( 0x40c );
      case 2:		/* German */
return( 0x407 );
      case 14: case 148:/* Greek */
return( 0x408 );
      case 10:		/* Hebrew */
return( 0x40d );
      case 26:		/* Hungarian */
return( 0x402 );
      case 15:		/* Icelandic */
return( 0x40f );
      case 3:		/* Italian */
return( 0x410 );
      case 11:		/* Japanese */
return( 0x411 );
      case 28:		/* Latvian */
return( 0x426 );
      case 24:		/* Lithuanian */
return( 0x427 );
      case 9:		/* Norwegian */
return( 0x414 );
      case 25:		/* Polish */
return( 0x415 );
      case 8:		/* Portuguese */
return( 0x416 );
      case 37:		/* Romanian */
return( 0x418 );
      case 32:		/* Russian */
return( 0x419 );
      case 39:		/* Slovak */
return( 0x41b );
      case 40:		/* Slovenian */
return( 0x424 );
      case 6:		/* Spanish */
return( 0x40a );
      case 5:		/* Swedish */
return( 0x41d );
      case 17:		/* Turkish */
return( 0x41f );
      case 45:		/* Ukrainian */
return( 0x422 );
      default:
return( 0 );
    }
}

unichar_t *TTFGetFontName(FILE *ttf,int32 offset,int32 off2) {
    int i,num;
    int32 tag, nameoffset, length, stringoffset;
    int plat, spec, lang, name, len, off, val;
    int fullval, fullstr, fulllen, famval, famstr, famlen;
    int enc, fullenc, famenc;

    fseek(ttf,offset,SEEK_SET);
    /* version = */ getlong(ttf);
    num = getushort(ttf);
    /* srange = */ getushort(ttf);
    /* esel = */ getushort(ttf);
    /* rshift = */ getushort(ttf);
    for ( i=0; i<num; ++i ) {
	tag = getlong(ttf);
	/* checksum = */ getlong(ttf);
	nameoffset = off2+getlong(ttf);
	length = getlong(ttf);
	if ( tag==CHR('n','a','m','e'))
    break;
    }
    if ( i==num )
return( NULL );

    fseek(ttf,nameoffset,SEEK_SET);
    /* format = */ getushort(ttf);
    num = getushort(ttf);
    stringoffset = nameoffset+getushort(ttf);
    fullval = famval = 0; fullenc = famenc = em_none;
    for ( i=0; i<num; ++i ) {
	plat = getushort(ttf);
	spec = getushort(ttf);
	lang = getushort(ttf);
	name = getushort(ttf);
	len = getushort(ttf);
	off = getushort(ttf);
	enc = enc_from_platspec(plat,spec);
	val = 0;
    /* I really want an english name */
	if ( (plat==0 || plat==1) && enc!=em_none && lang==0 )
	    val = 11;
	else if ( plat==3 && enc!=em_none && (lang&0xff)==0x09 )
	    val = 12;
    /* failing that I'll take what I can get */
	else if ( enc!=em_none )
	    val = 1;
	if ( name==4 && val>fullval ) {
	    fullval = val;
	    fullstr = off;
	    fulllen = len;
	    fullenc = enc;
	    if ( val==12 )
    break;
	} else if ( name==1 && val>famval ) {
	    famval = val;
	    famstr = off;
	    famlen = len;
	    famenc = enc;
	}
    }
    if ( fullval==0 ) {
	if ( famval==0 )
return( NULL );
	fullstr = famstr;
	fulllen = famlen;
	fullenc = famenc;
    }
return( _readencstring(ttf,stringoffset+fullstr,fulllen,fullenc));
}

static int PickTTFFont(FILE *ttf,char *filename) {
    int32 *offsets, cnt, i, choice, j;
    unichar_t **names;
    char *pt;

    /* TTCF version = */ getlong(ttf);
    cnt = getlong(ttf);
    if ( cnt==1 ) {
	/* This is easy, don't bother to ask the user, there's no choice */
	int32 offset = getlong(ttf);
	fseek(ttf,offset,SEEK_SET);
return( true );
    }
    offsets = galloc(cnt*sizeof(int32));
    for ( i=0; i<cnt; ++i )
	offsets[i] = getlong(ttf);
    names = galloc(cnt*sizeof(unichar_t *));
    for ( i=j=0; i<cnt; ++i ) {
	names[j] = TTFGetFontName(ttf,offsets[i],0);
	if ( names[j]!=NULL ) ++j;
    }
    if ( (pt = strchr(filename,'('))!=NULL ) {
	char *find = copy(pt+1);
	pt = strchr(find,')');
	if ( pt!=NULL ) *pt='\0';
	for ( choice=cnt-1; choice>=0; --choice )
	    if ( uc_strcmp(names[choice],find)==0 )
	break;
	if ( choice==-1 ) {
	    char *fn = copy(filename);
	    *strchr(fn,'(') = '\0';
	    GWidgetErrorR(_STR_NotInCollection,_STR_FontNotInCollection,find,fn);
	    free(fn);
	}
	free(find);
    } else if ( screen_display==NULL )
	choice = 0;
    else
	choice = GWidgetChoicesR(_STR_PickFont,(const unichar_t **) names,j,0,_STR_MultipleFontsPick);
    if ( choice!=-1 )
	fseek(ttf,offsets[choice],SEEK_SET);
    for ( i=0; i<j; ++i )
	free(names[i]);
    free(names);
    free(offsets);
return( choice!=-1);
}

static int PickCFFFont(char **fontnames) {
    unichar_t **names;
    int cnt, i, choice;

    for ( cnt=0; fontnames[cnt]!=NULL; ++cnt);
    names = gcalloc(cnt+1,sizeof(unichar_t *));
    for ( i=0; i<cnt; ++i )
	names[i] = uc_copy(fontnames[i]);
    choice = GWidgetChoicesR(_STR_PickFont,
	    (const unichar_t **) names,cnt,0,_STR_MultipleFontsPick);
    for ( i=0; i<cnt; ++i )
	free(names[i]);
    free(names);
return( choice );
}

static int readttfheader(FILE *ttf, struct ttfinfo *info,char *filename) {
    int i;
    int tag, checksum, offset, length, version;

    version=getlong(ttf);
    if ( version==CHR('t','t','c','f')) {
	/* TrueType font collection */
	info->is_ttc = true;
	if ( !PickTTFFont(ttf,filename))
return( 0 );
	/* If they picked a font, then we should be left pointing at the */
	/*  start of the Table Directory for that font */
	info->one_of_many = true;
	version = getlong(ttf);
    }

    if ( (version)!=0x00010000 && version!=CHR('t','r','u','e') &&
	    version!=CHR('O','T','T','O'))
return( 0 );			/* Not version 1 of true type, nor Open Type */
    info->numtables = getushort(ttf);
    /* searchRange = */ getushort(ttf);
    /* entrySelector = */ getushort(ttf);
    /* rangeshift = */ getushort(ttf);

    for ( i=0; i<info->numtables; ++i ) {
	tag = getlong(ttf);
	checksum = getlong(ttf);
	offset = getlong(ttf);
	length = getlong(ttf);
#ifdef DEBUG
 printf( "%c%c%c%c\n", tag>>24, (tag>>16)&0xff, (tag>>8)&0xff, tag&0xff );
#endif
	switch ( tag ) {
	  case CHR('C','F','F',' '):
	    info->cff_start = offset;
	    info->cff_length = offset;
	  break;
	  case CHR('c','m','a','p'):
	    info->encoding_start = offset;
	  break;
	  case CHR('g','l','y','f'):
	    info->glyph_start = offset;
	  break;
	  case CHR('G','P','O','S'):
	    info->gpos_start = offset;
	  break;
	  case CHR('G','S','U','B'):
	    info->gsub_start = offset;
	  break;
	  case CHR('b','d','a','t'):		/* Apple/MS use a different tag. Great. */
	  case CHR('E','B','D','T'):
	    info->bitmapdata_start = offset;
	  break;
	  case CHR('b','l','o','c'):		/* Apple/MS use a different tag. Great. */
	  case CHR('E','B','L','C'):
	    info->bitmaploc_start = offset;
	  break;
	  case CHR('b','h','e','d'):		/* Apple uses bhed for fonts with only bitmaps */
	  case CHR('h','e','a','d'):
	    info->head_start = offset;
	  break;
	  case CHR('h','h','e','a'):
	    info->hhea_start = offset;
	  break;
	  case CHR('h','m','t','x'):
	    info->hmetrics_start = offset;
	  break;
	  case CHR('k','e','r','n'):
	    info->kern_start = offset;
	  break;
	  case CHR('l','o','c','a'):
	    info->glyphlocations_start = offset;
	    info->loca_length = length;
	    info->glyph_cnt = length/2-1;	/* the minus one is because there is one extra entry to give the length of the last glyph */
	  break;
	  case CHR('m','a','x','p'):
	    info->maxp_start = offset;
	  break;
	  case CHR('n','a','m','e'):
	    info->copyright_start = offset;
	  break;
	  case CHR('p','o','s','t'):
	    info->postscript_start = offset;
	  break;
	  case CHR('O','S','/','2'):
	    info->os2_start = offset;
	  break;
	  case CHR('v','h','e','a'):
	    info->vhea_start = offset;
	  break;
	  case CHR('v','m','t','x'):
	    info->vmetrics_start = offset;
	  break;
	  case CHR('V','O','R','G'):
	    info->vorg_start = offset;
	  break;
	}
    }
return( true );
}

static void readttfhead(FILE *ttf,struct ttfinfo *info) {
    /* Here I want units per em, and size of loca entries */
    int i;
    fseek(ttf,info->head_start+4*4+2,SEEK_SET);		/* skip over the version number and a bunch of junk */
    info->emsize = getushort(ttf);
    for ( i=0; i<15; ++i )
	getushort(ttf);
    info->index_to_loc_is_long = getushort(ttf);
    if ( info->index_to_loc_is_long )
	info->glyph_cnt = info->loca_length/4-1;
}

static void readttfhhea(FILE *ttf,struct ttfinfo *info) {
    /* Here I want ascent, descent and the number of horizontal metrics */
    int i;

    fseek(ttf,info->hhea_start+4,SEEK_SET);		/* skip over the version number */
    info->ascent = getushort(ttf);
    info->descent = -(short) getushort(ttf);
    info->pfminfo.linegap = getushort(ttf);

    /* fontographer puts the max ascender/min descender here instead. idiots */
    if ( info->ascent==0 && info->descent==0 )
	info->ascent = .8*info->emsize;
#if 0
    else if ( info->ascent+info->descent!=info->emsize )
	info->ascent = info->ascent * ((real) info->emsize)/(info->ascent+info->descent);
#endif
    info->descent = info->emsize-info->ascent;

    for ( i=0; i<12; ++i )
	getushort(ttf);
    info->width_cnt = getushort(ttf);
}

static void readttfmaxp(FILE *ttf,struct ttfinfo *info) {
    /* All I want here is the number of glyphs */
    int cnt;
    fseek(ttf,info->maxp_start+4,SEEK_SET);		/* skip over the version number */
    cnt = getushort(ttf);
    if ( cnt!=info->glyph_cnt && info->glyph_cnt!=0 ) {
	GDrawError("TTF Font has bad glyph count field. maxp says: %d sizeof(loca)=>%d", cnt, info->glyph_cnt);
	if ( cnt>info->glyph_cnt )
	    cnt = info->glyph_cnt;		/* Use the smaller of the two values */
    }
    /* Open Type fonts have no loca table, so we can't calculate the glyph */
    /*  count from it */
    info->glyph_cnt = cnt;
}

static char *stripspaces(char *str) {
    char *str2 = str, *base = str;

    if ( str==NULL )
return( NULL );

    while ( *str ) {
	if ( *str==' ' )
	    ++str;
	else
	    *str2++ = *str++;
    }
    *str2 = '\0';
return( base );
}

static void TTFAddLangStr(FILE *ttf, struct ttfinfo *info, int language, int id,
	int strlen, int stroff,int enc) {
    struct ttflangname *cur, *prev;
    unichar_t *str;

    if ( id<0 || id>=ttf_namemax )
return;
    if ( (language&0xff00)==0 ) language |= 0x400;

    str = _readencstring(ttf,stroff,strlen,enc);
    if ( str==NULL )		/* we didn't understand the encoding */
return;

    for ( prev=NULL, cur=info->names; cur!=NULL && cur->lang!=language; prev = cur, cur=cur->next );
    if ( cur==NULL ) {
	cur=gcalloc(1,sizeof(struct ttflangname));
	cur->lang = language;
	if ( prev==NULL )
	    info->names = cur;
	else
	    prev->next = cur;
    }
    if ( cur->names[id]!=NULL ) free(cur->names[id]);
    cur->names[id] = str;
}

static int is_ascii(unichar_t *str) {	/* isascii is in ctype */
    if ( str==NULL )
return( false );
    while ( *str && *str<127 && *str>=' ' )
	++str;
return( *str=='\0' );
}

static int is_latin1(unichar_t *str) {
    if ( str==NULL )
return( false );
    while ( *str && *str<256 && *str>=' ' )
	++str;
return( *str=='\0' );
}

static char *removeaccents(char *_str) {
    /* Postscript really wants ASCII, so if we can make it ascii without too */
    /*  much loss, let's do so */
    unsigned char *str = (unsigned char *) _str, *pt = str;
    while ( *pt ) {
	if ( *pt>=0xc0 && *pt<=0xc5 )
	    *pt = 'A';
	else if ( *pt>=0xe0 && *pt<=0xe5 )
	    *pt = 'a';
	else if ( *pt==0xc6 || *pt==0xe6 || *pt==0xdf ) {
	    unsigned char *temp = galloc(strlen((char *) str)+2);
	    strcpy((char *) temp,(char *) str);
	    if ( *pt==0xc6 ) {
		temp[pt-str] = 'A';
		temp[pt-str+1] = 'E';
	    } else if ( *pt==0e6 ) {
		temp[pt-str] = 'a';
		temp[pt-str+1] = 'e';
	    } else {
		temp[pt-str] = 's';
		temp[pt-str+1] = 's';
	    }
	    strcpy((char *) temp+(pt-str)+2,(char *) pt+1);
	    pt = temp+(pt-str)+1;
	    free(str);
	    str = temp;
	} else if ( *pt==0xc7 )
	    *pt = 'C';
	else if ( *pt==0xe7 )
	    *pt = 'c';
	else if ( *pt>=0xc8 && *pt<=0xcb )
	    *pt = 'E';
	else if ( *pt>=0xe8 && *pt<=0xeb )
	    *pt = 'e';
	else if ( *pt>=0xcc && *pt<=0xcf )
	    *pt = 'I';
	else if ( *pt>=0xec && *pt<=0xef )
	    *pt = 'i';
	else if ( *pt==0xd1 )
	    *pt = 'N';
	else if ( *pt==0xf1 )
	    *pt = 'n';
	else if ( (*pt>=0xd2 && *pt<=0xd6 ) || *pt==0xd8 )
	    *pt = 'O';
	else if ( (*pt>=0xf2 && *pt<=0xf6 ) || *pt==0xf8 )
	    *pt = 'o';
	else if ( *pt>=0xd9 && *pt<=0xdc )
	    *pt = 'U';
	else if ( *pt>=0xf9 && *pt<=0xfc )
	    *pt = 'u';
	else if ( *pt>=0xdd )
	    *pt = 'Y';
	else if ( *pt>=0xfd && *pt<=0xff )
	    *pt = 'y';
	++pt;
    }
return( (char *) str );
}

static char *FindLangEntry(struct ttfinfo *info, int id ) {
    /* Look for an entry with string id */
    /* we prefer english, if we can't find english look for something in ascii */
    /* if not english let's try for latin1, if that fails we are doomed */
    struct ttflangname *cur;
    char *ret;

    for ( cur=info->names; cur!=NULL && cur->lang!=0x409; cur=cur->next );
    if ( cur!=NULL && cur->names[id]==NULL ) cur = NULL;
    if ( cur==NULL )
	for ( cur=info->names; cur!=NULL && (cur->lang&0xf)!=0x09; cur=cur->next );
    if ( cur!=NULL && cur->names[id]==NULL ) cur = NULL;
    if ( cur==NULL )
	for ( cur=info->names; cur!=NULL && !is_ascii(cur->names[id]); cur=cur->next );
    if ( cur==NULL )
	for ( cur=info->names; cur!=NULL && !is_latin1(cur->names[id]); cur=cur->next );
    if ( cur==NULL )
return( NULL );
    ret = cu_copy(cur->names[id]);
    ret = removeaccents(ret);
return( ret );
}

static void readttfcopyrights(FILE *ttf,struct ttfinfo *info) {
    int i, cnt, tableoff;
    int platform, specific, language, name, str_len, stroff;
    int enc;

    /* All I want here are the names of the font and its copyright */
    fseek(ttf,info->copyright_start,SEEK_SET);
    /* format selector = */ getushort(ttf);
    cnt = getushort(ttf);
    tableoff = info->copyright_start+getushort(ttf);
    for ( i=0; i<cnt; ++i ) {
	platform = getushort(ttf);
	specific = getushort(ttf);
	language = getushort(ttf);
	name = getushort(ttf);
	str_len = getushort(ttf);
	stroff = getushort(ttf);
	enc = enc_from_platspec(platform,specific);
	if ( enc!=em_none ) {
	    if ( platform==0 || platform==1 )
		language = AppleLang_to_MS(language);
	    TTFAddLangStr(ttf,info,language,name,str_len,tableoff+stroff,enc);
	}
    }

    if ( info->copyright==NULL )
	info->copyright = FindLangEntry(info,ttf_copyright);
    if ( info->familyname==NULL )
	info->familyname = FindLangEntry(info,ttf_family);
    if ( info->fullname==NULL )
	info->fullname = FindLangEntry(info,ttf_fullname);
    if ( info->version==NULL )
	info->version = FindLangEntry(info,ttf_version);
    if ( info->fontname==NULL )
	info->fontname = FindLangEntry(info,ttf_postscriptname);

    /* OpenType spec says the version string should begin with "Version " and */
    /*  end with a space and have a number in between */
    if ( info->version==NULL ) info->version = copy("1.0");
    else if ( strnmatch(info->version,"Version ",8)==0 ) {
	char *temp = copy(info->version+8);
	if ( temp[strlen(temp)-1]==' ' )
	    temp[strlen(temp)-1] = '\0';
	free(info->version);
	info->version = temp;
    }
    if ( info->fontname==NULL && info->fullname!=NULL )
	info->fontname = stripspaces(copy(info->fullname));
}

static void readttfpreglyph(FILE *ttf,struct ttfinfo *info) {
    readttfhead(ttf,info);
    readttfhhea(ttf,info);
    readttfmaxp(ttf,info);
    readttfcopyrights(ttf,info);
}

#define _On_Curve	1
#define _X_Short	2
#define _Y_Short	4
#define _Repeat		8
#define _X_Same		0x10
#define _Y_Same		0x20

static void FigureControls(SplinePoint *from, SplinePoint *to, BasePoint *cp) {
    /* What are the control points for 2 cp bezier which will provide the same*/
    /*  curve as that for the 1 cp bezier specified above */
    real b, c, d;

    d = from->me.x;
    c = 2*cp->x - 2*from->me.x;
    b = to->me.x+from->me.x-2*cp->x;
    from->nextcp.x = d+c/3;
    to->prevcp.x = from->nextcp.x + (c+b)/3;

    d = from->me.y;
    c = 2*cp->y - 2*from->me.y;
    b = to->me.y+from->me.y-2*cp->y;
    from->nextcp.y = d+c/3;
    to->prevcp.y = from->nextcp.y + (c+b)/3;

    if ( from->me.x!=from->nextcp.x || from->me.y!=from->nextcp.y )
	from->nonextcp = false;
    if ( to->me.x!=to->prevcp.x || to->me.y!=to->prevcp.y )
	to->noprevcp = false;
}

static SplineSet *ttfbuildcontours(int path_cnt,uint16 *endpt, char *flags,
	BasePoint *pts) {
    SplineSet *head=NULL, *last=NULL, *cur;
    int i, path, start, last_off;
    SplinePoint *sp;

    for ( path=i=0; path<path_cnt; ++path ) {
	if ( endpt[path]<i )	/* Sigh. Yes there are fonts with bad endpt info */
    continue;
	cur = chunkalloc(sizeof(SplineSet));
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;
	last_off = false;
	start = i;
	while ( i<=endpt[path] ) {
	    if ( flags[i]&_On_Curve ) {
		sp = chunkalloc(sizeof(SplinePoint));
		sp->me = sp->nextcp = sp->prevcp = pts[i];
		sp->nonextcp = sp->noprevcp = true;
		if ( last_off && cur->last!=NULL )
		    FigureControls(cur->last,sp,&pts[i-1]);
		last_off = false;
	    } else if ( last_off ) {
		/* two off curve points get a third on curve point created */
		/* half-way between them. Now isn't that special */
		sp = chunkalloc(sizeof(SplinePoint));
		sp->me.x = (pts[i].x+pts[i-1].x)/2;
		sp->me.y = (pts[i].y+pts[i-1].y)/2;
		sp->nextcp = sp->prevcp = sp->me;
		sp->nonextcp = true;
		if ( last_off && cur->last!=NULL )
		    FigureControls(cur->last,sp,&pts[i-1]);
		/* last_off continues to be true */
	    } else {
		last_off = true;
		sp = NULL;
	    }
	    if ( sp!=NULL ) {
		if ( cur->first==NULL )
		    cur->first = sp;
		else
		    SplineMake(cur->last,sp);
		cur->last = sp;
	    }
	    ++i;
	}
	if ( start==i-1 ) {
	    /* MS chinese fonts have contours consisting of a single off curve*/
	    /*  point. What on earth do they think that means? */
	    sp = chunkalloc(sizeof(SplinePoint));
	    sp->me.x = pts[start].x;
	    sp->me.y = pts[start].y;
	    sp->nextcp = sp->prevcp = sp->me;
	    sp->nonextcp = sp->noprevcp = true;
	    cur->first = cur->last = sp;
	} else if ( !(flags[start]&_On_Curve) && !(flags[i-1]&_On_Curve) ) {
	    sp = chunkalloc(sizeof(SplinePoint));
	    sp->me.x = (pts[start].x+pts[i-1].x)/2;
	    sp->me.y = (pts[start].y+pts[i-1].y)/2;
	    sp->nextcp = sp->prevcp = sp->me;
	    sp->nonextcp = true;
	    FigureControls(cur->last,sp,&pts[i-1]);
	    SplineMake(cur->last,sp);
	    cur->last = sp;
	    FigureControls(sp,cur->first,&pts[start]);
	} else if ( !(flags[i-1]&_On_Curve))
	    FigureControls(cur->last,cur->first,&pts[i-1]);
	else if ( !(flags[start]&_On_Curve) )
	    FigureControls(cur->last,cur->first,&pts[start]);
	if ( cur->last!=cur->first ) {
	    SplineMake(cur->last,cur->first);
	    cur->last = cur->first;
	}
    }
return( head );
}

static void ttfstemhints(SplineChar *sc,char *instructions,int len) {
    /* !!!!! I assume these instructions have some hints ????? */
}

static void readttfsimpleglyph(FILE *ttf,struct ttfinfo *info,SplineChar *sc, int path_cnt) {
    uint16 *endpt = galloc((path_cnt+1)*sizeof(uint16));
    char *instructions;
    char *flags;
    BasePoint *pts;
    int i, j, tot, len;
    int last_pos;

    for ( i=0; i<path_cnt; ++i )
	endpt[i] = getushort(ttf);
    if ( path_cnt==0 ) {
	tot = 0;
	pts = galloc(sizeof(BasePoint));
    } else {
	tot = endpt[path_cnt-1]+1;
	pts = galloc(tot*sizeof(BasePoint));
    }

    len = getushort(ttf);
    instructions = galloc(len);
    for ( i=0; i<len; ++i )
	instructions[i] = getc(ttf);

    flags = galloc(tot);
    for ( i=0; i<tot; ++i ) {
	flags[i] = getc(ttf);
	if ( flags[i]&_Repeat ) {
	    int cnt = getc(ttf);
	    if ( i+cnt>=tot ) {
		GDrawIError("Flag count is wrong (or total is): %d %d", i+cnt, tot );
		cnt = tot-i-1;
	    }
	    for ( j=0; j<cnt; ++j )
		flags[i+j+1] = flags[i];
	    i += cnt;
	}
    }
    if ( i!=tot )
	GDrawIError("Flag count is wrong (or total is): %d %d", i, tot );

    last_pos = 0;
    for ( i=0; i<tot; ++i ) {
	if ( flags[i]&_X_Short ) {
	    int off = getc(ttf);
	    if ( !(flags[i]&_X_Same ) )
		off = -off;
	    pts[i].x = last_pos + off;
	} else if ( flags[i]&_X_Same )
	    pts[i].x = last_pos;
	else
	    pts[i].x = last_pos + (short) getushort(ttf);
	last_pos = pts[i].x;
    }

    last_pos = 0;
    for ( i=0; i<tot; ++i ) {
	if ( flags[i]&_Y_Short ) {
	    int off = getc(ttf);
	    if ( !(flags[i]&_Y_Same ) )
		off = -off;
	    pts[i].y = last_pos + off;
	} else if ( flags[i]&_Y_Same )
	    pts[i].y = last_pos;
	else
	    pts[i].y = last_pos + (short) getushort(ttf);
	last_pos = pts[i].y;
    }

    sc->splines = ttfbuildcontours(path_cnt,endpt,flags,pts);
    ttfstemhints(sc,instructions,len);
    SCCatagorizePoints(sc);
    free(endpt);
    free(flags);
    free(instructions);
    free(pts);
    if ( feof(ttf))
	fprintf( stderr, "Reached end of file when reading simple glyph\n" );
}

#define _ARGS_ARE_WORDS	1
#define _ARGS_ARE_XY	2
#define _ROUND		4		/* round offsets so componant is on grid */
#define _SCALE		8
/* 0x10 is reserved */
#define _MORE		0x20
#define _XY_SCALE	0x40
#define _MATRIX		0x80
#define _INSTR		0x100
#define _MY_METRICS	0x200
#define _OVERLAP_COMPOUND	0x400	/* Used in Apple GX fonts */
	    /* Means the components overlap (which? this one and what other?) */
/* Described in OpenType specs, not by Apple */
/* amusingly, Apple supports but MS does not */
#define _SCALED_OFFSETS		0x800	/* Use Apple definition of offset interpretation */
#define _UNSCALED_OFFSETS	0x1000	/* Use MS definition */

static void readttfcompositglyph(FILE *ttf,struct ttfinfo *info,SplineChar *sc, int32 end) {
    RefChar *head=NULL, *last=NULL, *cur;
    int flags, arg1, arg2;

    do {
	if ( ftell(ttf)>=end ) {
	    fprintf( stderr, "Bad flags value, implied MORE components at end of glyph\n" );
    break;
	}
	cur = chunkalloc(sizeof(RefChar));
	flags = getushort(ttf);
	cur->local_enc = getushort(ttf);
	if ( info->inuse!=NULL )
	    info->inuse[cur->local_enc] = true;
	if ( flags&_ARGS_ARE_WORDS ) {
	    arg1 = (short) getushort(ttf);
	    arg2 = (short) getushort(ttf);
	} else {
	    arg1 = (signed char) getc(ttf);
	    arg2 = (signed char) getc(ttf);
	}
	if ( flags & _ARGS_ARE_XY ) {
	    cur->transform[4] = arg1;
	    cur->transform[5] = arg2;
	    /* There is some very strange stuff (half-)documented on the apple*/
	    /*  site about how these should be interpretted when there are */
	    /*  scale factors, or rotations */
	    /* It isn't well enough described to be comprehensible */
	    /*  http://fonts.apple.com/TTRefMan/RM06/Chap6glyf.html */
	    /* Microsoft says nothing about this */
	    /* Adobe implies this is a difference between MS and Apple */
	    /*  MS doesn't do this, Apple does (GRRRGH!!!!) */
	    /* Adobe says that setting bit 12 means that this will not happen */
	    /* Adobe says that setting bit 11 means that this will happen */
	    /*  So if either bit is set we know when this happens, if neither */
	    /*  we guess... But I still don't know how to interpret the */
	    /*  apple mode under rotation... */
	    /* I notice that FreeType does nothing about rotation nor does it */
	    /*  interpret bits 11&12 */
	} else {
	    /* Somehow we can get offsets by looking at the points in the */
	    /*  points so far generated and comparing them to the points in */
	    /*  the current componant */
	    /* How exactly is not described on any of the Apple, MS, Adobe */
	    /* freetype looks up arg1 in the set of points we've got so far */
	    /*  looks up arg2 in the new component (before renumbering) */
	    /*  offset.x = arg1.x - arg2.x; offset.y = arg1.y - arg2.y; */
	    /* Sadly I don't retain the information I need to deal with this */
	    /*  I lose the point numbers and the control points... */
	    /* I have implemented it in ttfmod where that info is retained */
	    cur->point_match = true;
	    fprintf( stderr, "TTF Warning: I mayn't understand matching points. Please send me a copy\n" );
	    fprintf( stderr, "  of whatever ttf file you are loading. Thanks.  gww@silcom.com\n" );
	}
	cur->transform[0] = cur->transform[3] = 1.0;
	if ( flags & _SCALE )
	    cur->transform[0] = cur->transform[3] = get2dot14(ttf);
	else if ( flags & _XY_SCALE ) {
	    cur->transform[0] = get2dot14(ttf);
	    cur->transform[3] = get2dot14(ttf);
	} else if ( flags & _MATRIX ) {
	    cur->transform[0] = get2dot14(ttf);
	    cur->transform[1] = get2dot14(ttf);
	    cur->transform[2] = get2dot14(ttf);
	    cur->transform[3] = get2dot14(ttf);
	}
#ifdef __Mac
	/* On mac assume scaled offsets unless told unscaled explicitly */
	if ( !(flags&_UNSCALED_OFFSETS) &&
#else
	/* everywhere else assume unscaled offsets unless told scaled explicitly */
	if ( (flags & _SCALED_OFFSETS) &&
#endif
		(flags & _ARGS_ARE_XY) && (flags&(_SCALE|_XY_SCALE|_MATRIX))) {
	    static int asked = 0;
	    /* This is not what Apple documents on their website. But it is */
	    /*  what appears to match the behavior of their rasterizer */
	    cur->transform[4] *= sqrt(cur->transform[0]*cur->transform[0]+
		    cur->transform[1]*cur->transform[1]);
	    cur->transform[5] *= sqrt(cur->transform[2]*cur->transform[2]+
		    cur->transform[3]*cur->transform[3]);
	    if ( info->fontname!=NULL && strcmp(info->fontname,"CompositeMac")!=0 && !asked ) {
		/* Not interested in the test font I generated myself */
		asked = true;
		fprintf( stderr, "Neat! You've got a font that actually uses Apple's scaled composite offsets.\n" );
		fprintf( stderr, "  I've never seen one, could you send me a copy of %s?\n", info->fontname );
		fprintf( stderr, "  Thanks.  gww@silcom.com\n" );
	    }
	}
	if ( cur->local_enc>=info->glyph_cnt ) {
	    fprintf(stderr, "Glyph %d attempts to reference glyph %d which is outside the font\n", sc->enc, cur->local_enc );
	    chunkfree(cur,sizeof(*cur));
	} else {
	    if ( head==NULL )
		head = cur;
	    else
		last->next = cur;
	    last = cur;
	}
	if ( feof(ttf)) {
	    fprintf(stderr, "Reached end of file when reading composit glyph\n" );
    break;
	}
    } while ( flags&_MORE );
    /* I'm ignoring many things that I don't understand here */
    /* Instructions, USE_MY_METRICS, what happens if ARGS AREN'T XY */
    /* ROUND means that offsets should rounded to the grid before being added */
    /*  (it's irrelevant for my purposes) */
    sc->refs = head;
}

static SplineChar *readttfglyph(FILE *ttf,struct ttfinfo *info,int start, int end,int enc) {
    int path_cnt;
    SplineChar *sc = SplineCharCreate();

    sc->unicodeenc = -1;
    sc->vwidth = info->emsize;
    /* sc->manualhints = 1; */ /* But only when I know how to read them in!!!! */

    if ( start==end ) {
	/* This isn't mentioned, but we seem to get some glyphs with no size,*/
	/*  not even a path cnt. They appear to be empty glyphs */
return( sc );
    }
    fseek(ttf,info->glyph_start+start,SEEK_SET);
    path_cnt = (short) getushort(ttf);
    /* xmin = */ getushort(ttf);
    /* ymin = */ getushort(ttf);
    /* xmax = */ getushort(ttf);
    /* ymax = */ sc->lsidebearing = getushort(ttf);
    sc->enc = enc;
    if ( path_cnt>=0 )
	readttfsimpleglyph(ttf,info,sc,path_cnt);
    else
	readttfcompositglyph(ttf,info,sc,info->glyph_start+end);
    sc->enc = 0;
return( sc );
}

static void readttfencodings(FILE *ttf,struct ttfinfo *info, int justinuse);

static void readttfglyphs(FILE *ttf,struct ttfinfo *info) {
    int i, anyread;
    uint32 *goffsets = galloc((info->glyph_cnt+1)*sizeof(uint32));

    /* First we read all the locations. This might not be needed, they may */
    /*  just follow one another, but nothing I've noticed says that so let's */
    /*  be careful */
    fseek(ttf,info->glyphlocations_start,SEEK_SET);
    if ( info->index_to_loc_is_long ) {
	for ( i=0; i<=info->glyph_cnt ; ++i )
	    goffsets[i] = getlong(ttf);
    } else {
	for ( i=0; i<=info->glyph_cnt ; ++i )
	    goffsets[i] = 2*getushort(ttf);
    }

    info->chars = gcalloc(info->glyph_cnt,sizeof(SplineChar *));
    if ( !info->is_ttc ) {
	/* read all the glyphs */
	for ( i=0; i<info->glyph_cnt ; ++i ) {
	    info->chars[i] = readttfglyph(ttf,info,goffsets[i],goffsets[i+1],i);
	    GProgressNext();
	}
    } else {
	/* only read the glyphs we actually use in this font */
	/* this is complicated by references, we can't just rely on the encoding */
	/*  to tell us what is used */
	info->inuse = gcalloc(info->glyph_cnt,sizeof(char));
	readttfencodings(ttf,info,true);
	anyread = true;
	while ( anyread ) {
	    anyread = false;
	    for ( i=0; i<info->glyph_cnt ; ++i ) {
		if ( info->inuse[i] && info->chars[i]==NULL ) {
		    info->chars[i] = readttfglyph(ttf,info,goffsets[i],goffsets[i+1],i);
		    GProgressNext();
		    anyread = info->chars[i]!=NULL;
		}
	    }
	}
    }
    free(goffsets);
/* Debug */
    for ( i=0; i<info->glyph_cnt ; ++i )
	if ( info->chars[i]!=NULL )
	    info->chars[i]->ttf_glyph = i;
/* End Debug */
    GProgressNextStage();
}

/* Standard names for cff */
const char *cffnames[] = {
 ".notdef",
 "space",
 "exclam",
 "quotedbl",
 "numbersign",
 "dollar",
 "percent",
 "ampersand",
 "quoteright",
 "parenleft",
 "parenright",
 "asterisk",
 "plus",
 "comma",
 "hyphen",
 "period",
 "slash",
 "zero",
 "one",
 "two",
 "three",
 "four",
 "five",
 "six",
 "seven",
 "eight",
 "nine",
 "colon",
 "semicolon",
 "less",
 "equal",
 "greater",
 "question",
 "at",
 "A",
 "B",
 "C",
 "D",
 "E",
 "F",
 "G",
 "H",
 "I",
 "J",
 "K",
 "L",
 "M",
 "N",
 "O",
 "P",
 "Q",
 "R",
 "S",
 "T",
 "U",
 "V",
 "W",
 "X",
 "Y",
 "Z",
 "bracketleft",
 "backslash",
 "bracketright",
 "asciicircum",
 "underscore",
 "quoteleft",
 "a",
 "b",
 "c",
 "d",
 "e",
 "f",
 "g",
 "h",
 "i",
 "j",
 "k",
 "l",
 "m",
 "n",
 "o",
 "p",
 "q",
 "r",
 "s",
 "t",
 "u",
 "v",
 "w",
 "x",
 "y",
 "z",
 "braceleft",
 "bar",
 "braceright",
 "asciitilde",
 "exclamdown",
 "cent",
 "sterling",
 "fraction",
 "yen",
 "florin",
 "section",
 "currency",
 "quotesingle",
 "quotedblleft",
 "guillemotleft",
 "guilsinglleft",
 "guilsinglright",
 "fi",
 "fl",
 "endash",
 "dagger",
 "daggerdbl",
 "periodcentered",
 "paragraph",
 "bullet",
 "quotesinglbase",
 "quotedblbase",
 "quotedblright",
 "guillemotright",
 "ellipsis",
 "perthousand",
 "questiondown",
 "grave",
 "acute",
 "circumflex",
 "tilde",
 "macron",
 "breve",
 "dotaccent",
 "dieresis",
 "ring",
 "cedilla",
 "hungarumlaut",
 "ogonek",
 "caron",
 "emdash",
 "AE",
 "ordfeminine",
 "Lslash",
 "Oslash",
 "OE",
 "ordmasculine",
 "ae",
 "dotlessi",
 "lslash",
 "oslash",
 "oe",
 "germandbls",
 "onesuperior",
 "logicalnot",
 "mu",
 "trademark",
 "Eth",
 "onehalf",
 "plusminus",
 "Thorn",
 "onequarter",
 "divide",
 "brokenbar",
 "degree",
 "thorn",
 "threequarters",
 "twosuperior",
 "registered",
 "minus",
 "eth",
 "multiply",
 "threesuperior",
 "copyright",
 "Aacute",
 "Acircumflex",
 "Adieresis",
 "Agrave",
 "Aring",
 "Atilde",
 "Ccedilla",
 "Eacute",
 "Ecircumflex",
 "Edieresis",
 "Egrave",
 "Iacute",
 "Icircumflex",
 "Idieresis",
 "Igrave",
 "Ntilde",
 "Oacute",
 "Ocircumflex",
 "Odieresis",
 "Ograve",
 "Otilde",
 "Scaron",
 "Uacute",
 "Ucircumflex",
 "Udieresis",
 "Ugrave",
 "Yacute",
 "Ydieresis",
 "Zcaron",
 "aacute",
 "acircumflex",
 "adieresis",
 "agrave",
 "aring",
 "atilde",
 "ccedilla",
 "eacute",
 "ecircumflex",
 "edieresis",
 "egrave",
 "iacute",
 "icircumflex",
 "idieresis",
 "igrave",
 "ntilde",
 "oacute",
 "ocircumflex",
 "odieresis",
 "ograve",
 "otilde",
 "scaron",
 "uacute",
 "ucircumflex",
 "udieresis",
 "ugrave",
 "yacute",
 "ydieresis",
 "zcaron",
 "exclamsmall",
 "Hungarumlautsmall",
 "dollaroldstyle",
 "dollarsuperior",
 "ampersandsmall",
 "Acutesmall",
 "parenleftsuperior",
 "parenrightsuperior",
 "twodotenleader",
 "onedotenleader",
 "zerooldstyle",
 "oneoldstyle",
 "twooldstyle",
 "threeoldstyle",
 "fouroldstyle",
 "fiveoldstyle",
 "sixoldstyle",
 "sevenoldstyle",
 "eightoldstyle",
 "nineoldstyle",
 "commasuperior",
 "threequartersemdash",
 "periodsuperior",
 "questionsmall",
 "asuperior",
 "bsuperior",
 "centsuperior",
 "dsuperior",
 "esuperior",
 "isuperior",
 "lsuperior",
 "msuperior",
 "nsuperior",
 "osuperior",
 "rsuperior",
 "ssuperior",
 "tsuperior",
 "ff",
 "ffi",
 "ffl",
 "parenleftinferior",
 "parenrightinferior",
 "Circumflexsmall",
 "hyphensuperior",
 "Gravesmall",
 "Asmall",
 "Bsmall",
 "Csmall",
 "Dsmall",
 "Esmall",
 "Fsmall",
 "Gsmall",
 "Hsmall",
 "Ismall",
 "Jsmall",
 "Ksmall",
 "Lsmall",
 "Msmall",
 "Nsmall",
 "Osmall",
 "Psmall",
 "Qsmall",
 "Rsmall",
 "Ssmall",
 "Tsmall",
 "Usmall",
 "Vsmall",
 "Wsmall",
 "Xsmall",
 "Ysmall",
 "Zsmall",
 "colonmonetary",
 "onefitted",
 "rupiah",
 "Tildesmall",
 "exclamdownsmall",
 "centoldstyle",
 "Lslashsmall",
 "Scaronsmall",
 "Zcaronsmall",
 "Dieresissmall",
 "Brevesmall",
 "Caronsmall",
 "Dotaccentsmall",
 "Macronsmall",
 "figuredash",
 "hypheninferior",
 "Ogoneksmall",
 "Ringsmall",
 "Cedillasmall",
 "questiondownsmall",
 "oneeighth",
 "threeeighths",
 "fiveeighths",
 "seveneighths",
 "onethird",
 "twothirds",
 "zerosuperior",
 "foursuperior",
 "fivesuperior",
 "sixsuperior",
 "sevensuperior",
 "eightsuperior",
 "ninesuperior",
 "zeroinferior",
 "oneinferior",
 "twoinferior",
 "threeinferior",
 "fourinferior",
 "fiveinferior",
 "sixinferior",
 "seveninferior",
 "eightinferior",
 "nineinferior",
 "centinferior",
 "dollarinferior",
 "periodinferior",
 "commainferior",
 "Agravesmall",
 "Aacutesmall",
 "Acircumflexsmall",
 "Atildesmall",
 "Adieresissmall",
 "Aringsmall",
 "AEsmall",
 "Ccedillasmall",
 "Egravesmall",
 "Eacutesmall",
 "Ecircumflexsmall",
 "Edieresissmall",
 "Igravesmall",
 "Iacutesmall",
 "Icircumflexsmall",
 "Idieresissmall",
 "Ethsmall",
 "Ntildesmall",
 "Ogravesmall",
 "Oacutesmall",
 "Ocircumflexsmall",
 "Otildesmall",
 "Odieresissmall",
 "OEsmall",
 "Oslashsmall",
 "Ugravesmall",
 "Uacutesmall",
 "Ucircumflexsmall",
 "Udieresissmall",
 "Yacutesmall",
 "Thornsmall",
 "Ydieresissmall",
 "001.000",
 "001.001",
 "001.002",
 "001.003",
 "Black",
 "Bold",
 "Book",
 "Light",
 "Medium",
 "Regular",
 "Roman",
 "Semibold",
 NULL
};
const int nStdStrings = sizeof(cffnames)/sizeof(cffnames[0])-1;

static char **readcfffontnames(FILE *ttf) {
    uint16 count = getushort(ttf);
    int offsize;
    uint32 *offsets;
    char **names;
    int i,j;

    if ( count==0 )
return( NULL );
    offsets = galloc((count+1)*sizeof(uint32));
    offsize = getc(ttf);
    for ( i=0; i<=count; ++i )
	offsets[i] = getoffset(ttf,offsize);
    names = galloc((count+1)*sizeof(char *));
    for ( i=0; i<count; ++i ) {
	names[i] = galloc(offsets[i+1]-offsets[i]+1);
	for ( j=0; j<offsets[i+1]-offsets[i]; ++j )
	    names[i][j] = getc(ttf);
	names[i][j] = '\0';
    }
    names[i] = NULL;
    free(offsets);
return( names );
}

static char *addnibble(char *pt, int nib) {
    if ( nib<=9 )
	*pt++ = nib+'0';
    else if ( nib==10 )
	*pt++ = '.';
    else if ( nib==11 )
	*pt++ = 'E';
    else if ( nib==12 ) {
	*pt++ = 'E';
	*pt++ = '-';
    } else if ( nib==14 )
	*pt++ = '-';
    else if ( nib==15 )
	*pt++ = '\0';
return( pt );
}

static int readcffthing(FILE *ttf,int *_ival,real *dval,int *operand) {
    char buffer[50], *pt;
    int ch, ival;

    ch = getc(ttf);
    if ( ch==12 ) {
	*operand = (12<<8) | getc(ttf);
return( 3 );
    } else if ( ch<=21 ) {
	*operand = ch;
return( 3 );
    } else if ( ch==30 ) {
	/* fixed format doesn't exist in dict data but does in type2 strings */
	pt = buffer;
	do {
	    ch = getc(ttf);
	    pt = addnibble(pt,ch>>4);
	    pt = addnibble(pt,ch&0xf);
	} while ( pt[-1]!='\0' );
	*dval = strtod(buffer,NULL);
return( 2 );
    } else if ( ch>=32 && ch<=246 ) {
	*_ival = ch-139;
return( 1 );
    } else if ( ch>=247 && ch<=250 ) {
	*_ival = ((ch-247)<<8) + getc(ttf)+108;
return( 1 );
    } else if ( ch>=251 && ch<=254 ) {
	*_ival = -((ch-251)<<8) - getc(ttf)-108;
return( 1 );
    } else if ( ch==28 ) {
	ival = getc(ttf)<<8;
	*_ival = (short) (ival | getc(ttf));
return( 1 );
    } else if ( ch==29 ) {
	/* 4 byte integers exist in dict data but not in type2 strings */
	ival = getc(ttf)<<24;
	ival = ival | getc(ttf)<<16;
	ival = ival | getc(ttf)<<8;
	*_ival = (int) (ival | getc(ttf));
return( 1 );
    }
    fprintf(stderr, "Unexpected value in dictionary %d\n", ch );
    *_ival = 0;
return( 0 );
}

struct topdicts {
    int32 cff_start;

    char *fontname;	/* From Name Index */

    int version;	/* SID */
    int notice;		/* SID */
    int copyright;	/* SID */
    int fullname;	/* SID */
    int familyname;	/* SID */
    int weight;		/* SID */
    int isfixedpitch;
    real italicangle;
    real underlinepos;
    real underlinewidth;
    int painttype;
    int charstringtype;
    real fontmatrix[6];
    int uniqueid;
    real fontbb[4];
    real strokewidth;
    int xuid[20];
    int charsetoff;	/* from start of file */
    int encodingoff;	/* from start of file */
    int charstringsoff;	/* from start of file */
    int private_size;
    int private_offset;	/* from start of file */
    int synthetic_base;	/* font index */
    int postscript_code;	/* SID */
 /* synthetic fonts only */
    int basefontname;		/* SID */
    int basefontblend[16];	/* delta */
 /* CID fonts only */
    int ros_registry;		/* SID */
    int ros_ordering;		/* SID */
    int ros_supplement;
    real cidfontversion;
    int cidfontrevision;
    int cidfonttype;
    int cidcount;
    int uidbase;
    int fdarrayoff;	/* from start of file */
    int fdselectoff;	/* from start of file */
    int sid_fontname;	/* SID */
/* Private stuff */
    real bluevalues[14];
    real otherblues[10];
    real familyblues[14];
    real familyotherblues[10];
    real bluescale;
    real blueshift;
    real bluefuzz;
    int stdhw;
    int stdvw;
    real stemsnaph[10];
    real stemsnapv[10];
    int forcebold;
    int languagegroup;
    real expansionfactor;
    int initialRandomSeed;
    int subrsoff;	/* from start of this private table */
    int defaultwidthx;
    int nominalwidthx;

    struct pschars glyphs;
    struct pschars local_subrs;
    uint16 *charset;
};

static void TopDictFree(struct topdicts *dict) {
    int i;

    free(dict->charset);
    for ( i=0; i<dict->glyphs.cnt; ++i )
	free(dict->glyphs.values[i]);
    free(dict->glyphs.values);
    free(dict->glyphs.lens);
    for ( i=0; i<dict->local_subrs.cnt; ++i )
	free(dict->local_subrs.values[i]);
    free(dict->local_subrs.values);
    free(dict->local_subrs.lens);
}

static void readcffsubrs(FILE *ttf, struct pschars *subs) {
    uint16 count = getushort(ttf);
    int offsize;
    uint32 *offsets;
    int i,j;

    memset(subs,'\0',sizeof(struct pschars));
    if ( count==0 )
return;
    subs->cnt = count;
    subs->lens = galloc(count*sizeof(int));
    subs->values = galloc(count*sizeof(uint8 *));
    offsets = galloc((count+1)*sizeof(uint32));
    offsize = getc(ttf);
    for ( i=0; i<=count; ++i )
	offsets[i] = getoffset(ttf,offsize);
    for ( i=0; i<count; ++i ) {
	subs->lens[i] = offsets[i+1]-offsets[i];
	subs->values[i] = galloc(offsets[i+1]-offsets[i]+1);
	for ( j=0; j<offsets[i+1]-offsets[i]; ++j )
	    subs->values[i][j] = getc(ttf);
	subs->values[i][j] = '\0';
    }
    free(offsets);
}

static struct topdicts *readcfftopdict(FILE *ttf, char *fontname, int len) {
    struct topdicts *td = gcalloc(1,sizeof(struct topdicts));
    long base = ftell(ttf);
    int ival, oval, sp, ret, i;
    real stack[50];

    td->fontname = fontname;
    td->underlinepos = -100;
    td->underlinewidth = 50;
    td->charstringtype = 2;
    td->fontmatrix[0] = td->fontmatrix[3] = .001;

    td->notice = td->copyright = td->fullname = td->familyname = td->weight = td->version = -1;
    td->postscript_code = td->basefontname = -1;
    td->synthetic_base = td->ros_registry = -1;
    td->fdarrayoff = td->fdselectoff = td->sid_fontname = -1;

    while ( ftell(ttf)<base+len ) {
	sp = 0;
	while ( (ret=readcffthing(ttf,&ival,&stack[sp],&oval))!=3 && ftell(ttf)<base+len ) {
	    if ( ret==1 )
		stack[sp]=ival;
	    if ( ret!=0 && sp<45 )
		++sp;
	}
	if ( sp==0 )
	    fprintf( stderr, "No argument to operator\n" );
	else if ( ret==3 ) switch( oval ) {
	  case 0:
	    td->version = stack[sp-1];
	  break;
	  case 1:
	    td->notice = stack[sp-1];
	  break;
	  case (12<<8)+0:
	    td->copyright = stack[sp-1];
	  break;
	  case 2:
	    td->fullname = stack[sp-1];
	  break;
	  case 3:
	    td->familyname = stack[sp-1];
	  break;
	  case 4:
	    td->weight = stack[sp-1];
	  break;
	  case (12<<8)+1:
	    td->isfixedpitch = stack[sp-1];
	  break;
	  case (12<<8)+2:
	    td->italicangle = stack[sp-1];
	  break;
	  case (12<<8)+3:
	    td->underlinepos = stack[sp-1];
	  break;
	  case (12<<8)+4:
	    td->underlinewidth = stack[sp-1];
	  break;
	  case (12<<8)+5:
	    td->painttype = stack[sp-1];
	  break;
	  case (12<<8)+6:
	    td->charstringtype = stack[sp-1];
	  break;
	  case (12<<8)+7:
	    memcpy(td->fontmatrix,stack,(sp>=6?6:sp)*sizeof(real));
	  break;
	  case 13:
	    td->uniqueid = stack[sp-1];
	  break;
	  case 5:
	    memcpy(td->fontbb,stack,(sp>=4?4:sp)*sizeof(real));
	  break;
	  case (12<<8)+8:
	    td->strokewidth = stack[sp-1];
	  break;
	  case 14:
	    for ( i=0; i<sp && i<20; ++i )
		td->xuid[i] = stack[i];
	  break;
	  case 15:
	    td->charsetoff = stack[sp-1];
	  break;
	  case 16:
	    td->encodingoff = stack[sp-1];
	  break;
	  case 17:
	    td->charstringsoff = stack[sp-1];
	  break;
	  case 18:
	    td->private_size = stack[0];
	    td->private_offset = stack[1];
	  break;
	  case (12<<8)+20:
	    td->synthetic_base = stack[sp-1];
	  break;
	  case (12<<8)+21:
	    td->postscript_code = stack[sp-1];
	  break;
	  case (12<<8)+22:
	    td->basefontname = stack[sp-1];
	  break;
	  case (12<<8)+23:
	    for ( i=0; i<sp && i<16; ++i )
		td->basefontblend[i] = stack[i];
	  break;
	  case (12<<8)+30:
	    td->ros_registry = stack[0];
	    td->ros_ordering = stack[1];
	    td->ros_supplement = stack[2];
	  break;
	  case (12<<8)+31:
	    td->cidfontversion = stack[sp-1];
	  break;
	  case (12<<8)+32:
	    td->cidfontrevision = stack[sp-1];
	  break;
	  case (12<<8)+33:
	    td->cidfonttype = stack[sp-1];
	  break;
	  case (12<<8)+34:
	    td->cidcount = stack[sp-1];
	  break;
	  case (12<<8)+35:
	    td->uidbase = stack[sp-1];
	  break;
	  case (12<<8)+36:
	    td->fdarrayoff = stack[sp-1];
	  break;
	  case (12<<8)+37:
	    td->fdselectoff = stack[sp-1];
	  break;
	  case (12<<8)+38:
	    td->sid_fontname = stack[sp-1];
	  break;
	  default:
	    fprintf(stderr,"Unknown operator in %s: %x\n", fontname, oval );
	  break;
	}
    }
return( td );
}

static void readcffprivate(FILE *ttf, struct topdicts *td) {
    int ival, oval, sp, ret, i;
    real stack[50];
    int32 end = td->cff_start+td->private_offset+td->private_size;

    fseek(ttf,td->cff_start+td->private_offset,SEEK_SET);

    td->subrsoff = -1;
    td->expansionfactor = .06;
    td->bluefuzz = 1;
    td->blueshift = 7;
    td->bluescale = .039625;

    while ( ftell(ttf)<end ) {
	sp = 0;
	while ( (ret=readcffthing(ttf,&ival,&stack[sp],&oval))!=3 && ftell(ttf)<end ) {
	    if ( ret==1 )
		stack[sp]=ival;
	    if ( ret!=0 && sp<45 )
		++sp;
	}
	if ( sp==0 )
	    fprintf( stderr, "No argument to operator\n" );
	else if ( ret==3 ) switch( oval ) {
	  case 6:
	    for ( i=0; i<sp && i<14; ++i ) {
		td->bluevalues[i] = stack[i];
		if ( i!=0 )
		    td->bluevalues[i] += td->bluevalues[i-1];
	    }
	  break;
	  case 7:
	    for ( i=0; i<sp && i<10; ++i ) {
		td->otherblues[i] = stack[i];
		if ( i!=0 )
		    td->otherblues[i] += td->otherblues[i-1];
	    }
	  break;
	  case 8:
	    for ( i=0; i<sp && i<14; ++i ) {
		td->familyblues[i] = stack[i];
		if ( i!=0 )
		    td->familyblues[i] += td->familyblues[i-1];
	    }
	  break;
	  case 9:
	    for ( i=0; i<sp && i<10; ++i ) {
		td->familyotherblues[i] = stack[i];
		if ( i!=0 )
		    td->familyotherblues[i] += td->familyotherblues[i-1];
	    }
	  break;
	  case (12<<8)+9:
	    td->bluescale = stack[sp-1];
	  break;
	  case (12<<8)+10:
	    td->blueshift = stack[sp-1];
	  break;
	  case (12<<8)+11:
	    td->bluefuzz = stack[sp-1];
	  break;
	  case 10:
	    td->stdhw = stack[sp-1];
	  break;
	  case 11:
	    td->stdvw = stack[sp-1];
	  break;
	  case (12<<8)+12:
	    for ( i=0; i<sp && i<10; ++i ) {
		td->stemsnaph[i] = stack[i];
		if ( i!=0 )
		    td->stemsnaph[i] += td->stemsnaph[i-1];
	    }
	  break;
	  case (12<<8)+13:
	    for ( i=0; i<sp && i<10; ++i ) {
		td->stemsnapv[i] = stack[i];
		if ( i!=0 )
		    td->stemsnapv[i] += td->stemsnapv[i-1];
	    }
	  break;
	  case (12<<8)+14:
	    td->forcebold = stack[sp-1];
	  break;
	  case (12<<8)+17:
	    td->languagegroup = stack[sp-1];
	  break;
	  case (12<<8)+18:
	    td->expansionfactor = stack[sp-1];
	  break;
	  case (12<<8)+19:
	    td->initialRandomSeed = stack[sp-1];
	  break;
	  case 19:
	    td->subrsoff = stack[sp-1];
	  break;
	  case 20:
	    td->defaultwidthx = stack[sp-1];
	  break;
	  case 21:
	    td->nominalwidthx = stack[sp-1];
	  break;
	  default:
	    fprintf(stderr,"Unknown operator in %s: %x\n", td->fontname, oval );
	  break;
	}
    }

    if ( td->subrsoff!=-1 ) {
	fseek(ttf,td->cff_start+td->private_offset+td->subrsoff,SEEK_SET);
	readcffsubrs(ttf,&td->local_subrs);
    }
}

static struct topdicts **readcfftopdicts(FILE *ttf, char **fontnames, int32 cff_start) {
    uint16 count = getushort(ttf);
    int offsize;
    uint32 *offsets;
    struct topdicts **dicts;
    int i;

    if ( count==0 )
return( NULL );
    offsets = galloc((count+1)*sizeof(uint32));
    offsize = getc(ttf);
    for ( i=0; i<=count; ++i )
	offsets[i] = getoffset(ttf,offsize);
    dicts = galloc((count+1)*sizeof(struct topdicts *));
    for ( i=0; i<count; ++i ) {
	dicts[i] = readcfftopdict(ttf,fontnames!=NULL?fontnames[i]:NULL,
		offsets[i+1]-offsets[i]);
	dicts[i]->cff_start = cff_start;
    }
    dicts[i] = NULL;
    free(offsets);
return( dicts );
}

/* I really expect to deal with encodings in ttf cmap, but just in case */
/*  I need to do more someday, this is the outline... */
static void readcffenc(FILE *ttf,struct topdicts *dict,char **strings) {
    int format, cnt, i;

    if ( dict->encodingoff==0 ) {
	/* Standard Encoding */;
return;
    } else if ( dict->encodingoff==1 ) {
	/* Expert Encoding */;
return;
    }
    fseek(ttf,dict->cff_start+dict->encodingoff,SEEK_SET);
    format = getc(ttf);
    if ( (format&0x7f)==0 ) {
	cnt = getc(ttf);
	for ( i=0; i<cnt; ++i )
	    getc(ttf);
    } else if ( (format&0x7f)==1 ) {
	cnt = getc(ttf);
	for ( i=0; i<cnt; ++i ) {
	    /* First= */ getc(ttf);
	    /* nLeft= */ getc(ttf);
	}
    }
    if ( format&0x80 ) {
	cnt = getc(ttf);
	for ( i=0; i<cnt; ++i ) {
	    /* Supplement  Encoding */ getc(ttf);
	    /* To SID */ getushort(ttf);
	}
    }
}

static void readcffset(FILE *ttf,struct topdicts *dict) {
    int len = dict->glyphs.cnt;
    int i;
    int format, cnt, j, first;

    if ( dict->charsetoff==0 ) {
	/* ISO Adobe charset */
	dict->charset = galloc(len*sizeof(uint16));
	for ( i=0; i<len && i<=228; ++i )
	    dict->charset[i] = i;
    } else if ( dict->charsetoff==1 ) {
	/* Expert charset */
	dict->charset = galloc((len<162?162:len)*sizeof(uint16));
	dict->charset[0] = 0;		/* .notdef */
	dict->charset[1] = 1;
	for ( i=2; i<len && i<=238-227; ++i )
	    dict->charset[i] = i+227;
	dict->charset[12] = 13;
	dict->charset[13] = 14;
	dict->charset[14] = 15;
	dict->charset[15] = 99;
	for ( i=16; i<len && i<=248-223; ++i )
	    dict->charset[i] = i+223;
	dict->charset[25] = 27;
	dict->charset[26] = 28;
	for ( i=27; i<len && i<=266-222; ++i )
	    dict->charset[i] = i+222;
	dict->charset[44] = 109;
	dict->charset[45] = 110;
	for ( i=46; i<len && i<=318-221; ++i )
	    dict->charset[i] = i+221;
	dict->charset[96] = 158;
	dict->charset[97] = 155;
	dict->charset[98] = 163;
	for ( i=99; i<len && i<=326-220; ++i )
	    dict->charset[i] = i+220;
	dict->charset[107] = 150;
	dict->charset[108] = 164;
	dict->charset[109] = 169;
	for ( i=110; i<len && i<=378-217; ++i )
	    dict->charset[i] = i+217;
    } else if ( dict->charsetoff==2 ) {
	/* Expert subset charset */
	dict->charset = galloc((len<130?130:len)*sizeof(uint16));
	dict->charset[0] = 0;		/* .notdef */
	dict->charset[1] = 1;
	for ( i=2; i<len && i<=238-227; ++i )
	    dict->charset[i] = i+227;
	dict->charset[12] = 13;
	dict->charset[13] = 14;
	dict->charset[14] = 15;
	dict->charset[15] = 99;
	for ( i=16; i<len && i<=248-223; ++i )
	    dict->charset[i] = i+223;
	dict->charset[25] = 27;
	dict->charset[26] = 28;
	for ( i=27; i<len && i<=266-222; ++i )
	    dict->charset[i] = i+222;
	dict->charset[44] = 109;
	dict->charset[45] = 110;
	for ( i=46; i<len && i<=272-221; ++i )
	    dict->charset[i] = i+221;
	dict->charset[51] = 300;
	dict->charset[52] = 301;
	dict->charset[53] = 302;
	dict->charset[54] = 305;
	dict->charset[55] = 314;
	dict->charset[56] = 315;
	dict->charset[57] = 158;
	dict->charset[58] = 155;
	dict->charset[59] = 163;
	for ( i=60; i<len && i<=326-260; ++i )
	    dict->charset[i] = i+260;
	dict->charset[67] = 150;
	dict->charset[68] = 164;
	dict->charset[69] = 169;
	for ( i=110; i<len && i<=346-217; ++i )
	    dict->charset[i] = i+217;
    } else {
	dict->charset = galloc(len*sizeof(uint16));
	dict->charset[0] = 0;		/* .notdef */
	fseek(ttf,dict->cff_start+dict->charsetoff,SEEK_SET);
	format = getc(ttf);
	if ( format==0 ) {
	    for ( i=1; i<len; ++i )
		dict->charset[i] = getushort(ttf);
	} else if ( format==1 ) {
	    for ( i = 1; i<len; ) {
		first = dict->charset[i++] = getushort(ttf);
		cnt = getc(ttf);
		for ( j=0; j<cnt; ++j )
		    dict->charset[i++] = ++first;
	    }
	} else if ( format==2 ) {
	    for ( i = 1; i<len; ) {
		first = dict->charset[i++] = getushort(ttf);
		cnt = getushort(ttf);
		for ( j=0; j<cnt; ++j )
		    dict->charset[i++] = ++first;
	    }
	}
    }
    while ( i<len ) dict->charset[i++] = 0;
}

static uint8 *readfdselect(FILE *ttf,int numglyphs) {
    uint8 *fdselect = gcalloc(numglyphs,sizeof(uint8));
    int i, j, format, nr, first, end, fd;

    format = getc(ttf);
    if ( format==0 ) {
	for ( i=0; i<numglyphs; ++i )
	    fdselect[i] = getc(ttf);
    } else if ( format==3 ) {
	nr = getushort(ttf);
	first = getushort(ttf);
	for ( i=0; i<nr; ++i ) {
	    fd = getc(ttf);
	    end = getushort(ttf);
	    for ( j=first; j<end; ++j ) {
		if ( j>=numglyphs )
		    fprintf( stderr, "Bad fdselect\n" );
		else
		    fdselect[j] = fd;
	    }
	    first = end;
	}
    } else {
	fprintf( stderr, "Didn't understand format for fdselect %d\n", format );
    }
return( fdselect );
}


static const char *getsid(int sid,char **strings) {
    if ( sid==-1 )
return( NULL );
    else if ( sid<nStdStrings )
return( cffnames[sid] );
    else
return( strings[sid-nStdStrings]);
}

static char *intarray2str(int *array, int size) {
    int i,j;
    char *pt, *ret;

    for ( i=size-1; i>=0 && array[i]==0; --i );
    if ( i==-1 )
return( NULL );
    ret = pt = galloc((i+1)*12+12);
    *pt++ = '[';
    for ( j=0; j<=i; ++j ) {
	sprintf( pt, "%d ", array[j]);
	pt += strlen(pt);
    }
    pt[-1]=']';
return( ret );
}

static char *realarray2str(real *array, int size) {
    int i,j;
    char *pt, *ret;

    for ( i=size-1; i>=0 && array[i]==0; --i );
    if ( i==-1 )
return( NULL );
    ret = pt = galloc((i+1)*20+12);
    *pt++ = '[';
    for ( j=0; j<=i; ++j ) {
	sprintf( pt, "%g ", array[j]);
	pt += strlen(pt);
    }
    pt[-1]=']';
return( ret );
}

static void privateadd(struct psdict *private,char *key,char *value) {
    if ( value==NULL )
return;
    private->keys[private->next] = copy(key);
    private->values[private->next++] = value;
}

static void privateaddint(struct psdict *private,char *key,int val) {
    char buf[10];
    if ( val==0 )
return;
    sprintf( buf,"%d", val );
    privateadd(private,key,copy(buf));
}

static void privateaddintarray(struct psdict *private,char *key,int val) {
    char buf[10];
    if ( val==0 )
return;
    sprintf( buf,"[%d]", val );
    privateadd(private,key,copy(buf));
}

static void privateaddreal(struct psdict *private,char *key,real val) {
    char buf[10];
    if ( val==0 )
return;
    sprintf( buf,"%g", val );
    privateadd(private,key,copy(buf));
}

static void cffprivatefillup(struct psdict *private, struct topdicts *dict) {
    private->cnt = 14;
    private->keys = galloc(14*sizeof(char *));
    private->values = galloc(14*sizeof(char *));
    privateadd(private,"BlueValues",
	    realarray2str(dict->bluevalues,sizeof(dict->bluevalues)/sizeof(dict->bluevalues[0])));
    privateadd(private,"OtherBlues",
	    realarray2str(dict->otherblues,sizeof(dict->otherblues)/sizeof(dict->otherblues[0])));
    privateadd(private,"FamilyBlues",
	    realarray2str(dict->familyblues,sizeof(dict->familyblues)/sizeof(dict->familyblues[0])));
    privateadd(private,"FamilyOtherBlues",
	    realarray2str(dict->familyotherblues,sizeof(dict->familyotherblues)/sizeof(dict->familyotherblues[0])));
    privateaddreal(private,"BlueScale",dict->bluescale);
    privateaddreal(private,"BlueShift",dict->blueshift);
    privateaddreal(private,"BlueFuzz",dict->bluefuzz);
    privateaddintarray(private,"StdHW",dict->stdhw);
    privateaddintarray(private,"StdVW",dict->stdvw);
    privateadd(private,"SnapStemH",
	    realarray2str(dict->stemsnaph,sizeof(dict->stemsnaph)/sizeof(dict->stemsnaph[0])));
    privateadd(private,"SnapStemV",
	    realarray2str(dict->stemsnapv,sizeof(dict->stemsnapv)/sizeof(dict->stemsnapv[0])));
    if ( dict->forcebold )
	privateadd(private,"ForceBold","true");
    privateaddint(private,"LanguageGroup",dict->languagegroup);
    privateaddreal(private,"ExpansionFactor",dict->expansionfactor);
}

static SplineFont *cffsffillup(struct topdicts *subdict, char **strings ) {
    SplineFont *sf = gcalloc(1,sizeof(SplineFont));
    int emsize;
    static int nameless;

    sf->fontname = copy(getsid(subdict->sid_fontname,strings));
    if ( sf->fontname==NULL ) {
	char buffer[40];
	sprintf(buffer,"UntitledSubFont_%d", ++nameless );
	sf->fontname = copy(buffer);
    }
    sf->encoding_name = em_none;

    if ( subdict->fontmatrix[0]==0 )
	emsize = 1000;
    else
	emsize = rint( 1/subdict->fontmatrix[0] );
    sf->ascent = .8*emsize;
    sf->descent = emsize - sf->ascent;
    if ( subdict->copyright!=-1 )
	sf->copyright = copy(getsid(subdict->copyright,strings));
    else
	sf->copyright = copy(getsid(subdict->notice,strings));
    sf->familyname = copy(getsid(subdict->familyname,strings));
    sf->fullname = copy(getsid(subdict->fullname,strings));
    sf->weight = copy(getsid(subdict->weight,strings));
    sf->version = copy(getsid(subdict->version,strings));
    sf->italicangle = subdict->italicangle;
    sf->upos = subdict->underlinepos;
    sf->uwidth = subdict->underlinewidth;
    sf->xuid = intarray2str(subdict->xuid,sizeof(subdict->xuid)/sizeof(subdict->xuid[0]));

    if ( subdict->private_size>0 ) {
	sf->private = gcalloc(1,sizeof(struct psdict));
	cffprivatefillup(sf->private,subdict);
    }
return( sf );
}

static void cffinfofillup(struct ttfinfo *info, struct topdicts *dict,
	char **strings ) {

    info->glyph_cnt = dict->glyphs.cnt;

    if ( dict->fontmatrix[0]==0 )
	info->emsize = 1000;
    else
	info->emsize = rint( 1/dict->fontmatrix[0] );
#if 1
    info->ascent = .8*info->emsize;
#else
    info->ascent = dict->fontbb[3]*info->emsize/(dict->fontbb[3]-dict->fontbb[1]);
#endif
    info->descent = info->emsize - info->ascent;
    if ( dict->copyright!=-1 )
	info->copyright = copy(getsid(dict->copyright,strings));
    else
	info->copyright = copy(getsid(dict->notice,strings));
    info->familyname = copy(getsid(dict->familyname,strings));
    info->fullname = copy(getsid(dict->fullname,strings));
    info->weight = copy(getsid(dict->weight,strings));
    info->version = copy(getsid(dict->version,strings));
    info->fontname = copy(dict->fontname);
    info->italicAngle = dict->italicangle;
    info->upos = dict->underlinepos;
    info->uwidth = dict->underlinewidth;
    info->xuid = intarray2str(dict->xuid,sizeof(dict->xuid)/sizeof(dict->xuid[0]));

    if ( dict->private_size>0 ) {
	info->private = gcalloc(1,sizeof(struct psdict));
	cffprivatefillup(info->private,dict);
    }
    if ( dict->ros_registry!=-1 ) {
	info->cidregistry = copy(getsid(dict->ros_registry,strings));
	info->ordering = copy(getsid(dict->ros_ordering,strings));
	info->supplement = dict->ros_supplement;
	info->cidfontversion = dict->cidfontversion;
    }
}

static void cfffigure(struct ttfinfo *info, struct topdicts *dict,
	char **strings, struct pschars *gsubrs) {
    int i, cstype;
    struct pschars *subrs;

    cffinfofillup(info, dict, strings );

/* The format allows for some dicts that are type1 strings and others that */
/*  are type2s. Which means that the global subrs will have a different bias */
/*  as we flip from font to font. So we can't set the bias when we read in */
/*  the subrs but must wait until we know which font we're working on. */
    cstype = dict->charstringtype;
    gsubrs->bias = cstype==1 ? 0 :
	    gsubrs->cnt < 1240 ? 107 :
	    gsubrs->cnt <33900 ? 1131 : 32768;
    subrs = &dict->local_subrs;
    subrs->bias = cstype==1 ? 0 :
	    subrs->cnt < 1240 ? 107 :
	    subrs->cnt <33900 ? 1131 : 32768;

    info->chars = gcalloc(info->glyph_cnt,sizeof(SplineChar *));
    for ( i=0; i<info->glyph_cnt; ++i ) {
	info->chars[i] = PSCharStringToSplines(
		dict->glyphs.values[i], dict->glyphs.lens[i],cstype-1,
		subrs,gsubrs,getsid(dict->charset[i],strings));
	info->chars[i]->vwidth = info->emsize;
	if ( cstype==2 ) {
	    if ( info->chars[i]->width == (int16) 0x8000 )
		info->chars[i]->width = dict->defaultwidthx;
	    else
		info->chars[i]->width += dict->defaultwidthx;
	}
    }
    /* Need to do a reference fixup here !!!!! just in case some idiot */
    /*  used type1 char strings */
}

static void cidfigure(struct ttfinfo *info, struct topdicts *dict,
	char **strings, struct pschars *gsubrs, struct topdicts **subdicts,
	uint8 *fdselect) {
    int i, j, cstype, uni, cid;
    struct pschars *subrs;
    SplineFont *sf;
    struct cidmap *map;
    char buffer[100];

    cffinfofillup(info, dict, strings );

    for ( j=0; subdicts[j]!=NULL; ++j );
    info->subfontcnt = j;
    info->subfonts = gcalloc(j+1,sizeof(SplineFont *));
    for ( j=0; subdicts[j]!=NULL; ++j ) 
	info->subfonts[j] = cffsffillup(subdicts[j],strings);
    for ( i=0; i<info->glyph_cnt; ++i ) {
	sf = info->subfonts[ fdselect[i] ];
	cid = dict->charset[i];
	if ( cid>=sf->charcnt ) sf->charcnt = cid+1;
    }
    for ( j=0; subdicts[j]!=NULL; ++j )
	info->subfonts[j]->chars = gcalloc(info->subfonts[j]->charcnt,sizeof(SplineChar *));

    info->chars = gcalloc(info->glyph_cnt,sizeof(SplineChar *));

    /* info->chars provides access to the chars ordered by glyph, which the */
    /*  ttf routines care about */
    /* sf->chars provides access to the chars ordered by CID. Not sure what */
    /*  would happen to a kern from one font to another... */

    map = FindCidMap(info->cidregistry,info->ordering,info->supplement,NULL);

    for ( i=0; i<info->glyph_cnt; ++i ) {
	j = fdselect[i];
	sf = info->subfonts[ j ];
/* The format allows for some dicts that are type1 strings and others that */
/*  are type2s. Which means that the global subrs will have a different bias */
/*  as we flip from font to font. So we can't set the bias when we read in */
/*  the subrs but must wait until we know which font we're working on. */
	cstype = subdicts[j]->charstringtype;
	gsubrs->bias = cstype==1 ? 0 :
		gsubrs->cnt < 1240 ? 107 :
		gsubrs->cnt <33900 ? 1131 : 32768;
	subrs = &subdicts[j]->local_subrs;
	subrs->bias = cstype==1 ? 0 :
		subrs->cnt < 1240 ? 107 :
		subrs->cnt <33900 ? 1131 : 32768;

	cid = dict->charset[i];
	uni = CID2NameEnc(map,cid,buffer,sizeof(buffer));
	info->chars[i] = PSCharStringToSplines(
		dict->glyphs.values[i], dict->glyphs.lens[i],cstype-1,
		subrs,gsubrs,buffer);
	info->chars[i]->vwidth = sf->ascent+sf->descent;
	info->chars[i]->unicodeenc = uni;
	sf->chars[cid] = info->chars[i];
	sf->chars[cid]->parent = sf;
	sf->chars[cid]->enc = cid;
	if ( sf->chars[cid]->refs!=NULL )
	    GDrawIError( "Reference found in CID font. Can't fix it up");
	if ( cstype==2 ) {
	    if ( sf->chars[cid]->width == (int16) 0x8000 )
		sf->chars[cid]->width = dict->defaultwidthx;
	    else
		sf->chars[cid]->width += dict->defaultwidthx;
	}
	GProgressNext();
    }
    /* No need to do a reference fixup here-- the chars aren't associated */
    /*  with any encoding as is required for seac */
}

static int readcffglyphs(FILE *ttf,struct ttfinfo *info) {
    int offsize;
    int hdrsize;
    char **fontnames, **strings;
    struct topdicts **dicts, **subdicts;
    int i, j, which;
    struct pschars gsubs;
    uint8 *fdselect;

    fseek(ttf,info->cff_start,SEEK_SET);
    if ( getc(ttf)!='\1' ) {		/* Major version */
	fprintf( stderr, "CFF version mismatch\n" );
return( 0 );
    }
    getc(ttf);				/* Minor version */
    hdrsize = getc(ttf);
    offsize = getc(ttf);
    if ( hdrsize!=4 )
	fseek(ttf,info->cff_start+hdrsize,SEEK_SET);
    fontnames = readcfffontnames(ttf);
    which = 0;
    if ( fontnames[1]!=NULL ) {		/* More than one? Can that even happen in OpenType? */
	which = PickCFFFont(fontnames);
	if ( which==-1 ) {
	    for ( i=0; fontnames[i]!=NULL; ++i )
		free(fontnames[i]);
	    free(fontnames);
return( 0 );
	}
    }
    dicts = readcfftopdicts(ttf,fontnames,info->cff_start);
	/* String index is just the same as fontname index */
    strings = readcfffontnames(ttf);
    readcffsubrs(ttf,&gsubs );
    /* Can be many fonts here. Only decompose the one */
	if ( dicts[which]->charstringsoff!=-1 ) {
	    fseek(ttf,info->cff_start+dicts[which]->charstringsoff,SEEK_SET);
	    readcffsubrs(ttf,&dicts[which]->glyphs);
	}
	if ( dicts[which]->private_offset!=-1 )
	    readcffprivate(ttf,dicts[which]);
	if ( dicts[which]->charsetoff!=-1 )
	    readcffset(ttf,dicts[which]);
	if ( dicts[which]->encodingoff!=-1 )
	    readcffenc(ttf,dicts[which],strings);
	if ( dicts[which]->fdarrayoff==-1 )
	    cfffigure(info,dicts[which],strings,&gsubs);
	else {
	    fseek(ttf,info->cff_start+dicts[which]->fdarrayoff,SEEK_SET);
	    subdicts = readcfftopdicts(ttf,NULL,info->cff_start);
	    fseek(ttf,info->cff_start+dicts[which]->fdselectoff,SEEK_SET);
	    fdselect = readfdselect(ttf,dicts[which]->glyphs.cnt);
	    for ( j=0; subdicts[j]!=NULL; ++j ) {
		if ( subdicts[j]->private_offset!=-1 )
		    readcffprivate(ttf,subdicts[j]);
		if ( subdicts[j]->charsetoff!=-1 )
		    readcffset(ttf,subdicts[j]);
	    }
	    cidfigure(info,dicts[which],strings,&gsubs,subdicts,fdselect);
	    for ( j=0; subdicts[j]!=NULL; ++j )
		TopDictFree(subdicts[j]);
	    free(subdicts); free(fdselect);
	}

    for ( i=0; fontnames[i]!=NULL && i<1; ++i ) {
	free(fontnames[i]);
	TopDictFree(dicts[i]);
    }
    free(fontnames); free(dicts);
    if ( strings!=NULL ) {
	for ( i=0; strings[i]!=NULL && i<1; ++i )
	    free(strings[i]);
	free(strings);
    }
    for ( i=0; i<gsubs.cnt; ++i )
	free(gsubs.values[i]);
    free(gsubs.values); free(gsubs.lens);
return( 1 );
}

static void readttfwidths(FILE *ttf,struct ttfinfo *info) {
    int i,j;
    int lastwidth = info->emsize;
    /* I'm not interested in the lsb, I'm not sure what it means if it differs*/
    /*  from that is specified in the outline. Do we move the outline? */

    fseek(ttf,info->hmetrics_start,SEEK_SET);
    for ( i=0; i<info->width_cnt && i<info->glyph_cnt; ++i ) {
	if ( info->chars[i]!=NULL ) {		/* can happen in ttc files */
	    info->chars[i]->width = lastwidth = getushort(ttf);
	    info->chars[i]->widthset = true;
	} else
	    /* unused width = */ lastwidth = getushort(ttf);
	/* lsb = */ getushort(ttf);
    }
    if ( i==0 )
	fprintf( stderr, "Invalid ttf hmtx table (or hhea), numOfLongVerMetrics is 0\n" );
	
    for ( j=i; j<info->glyph_cnt; ++j ) {
	if ( info->chars[j]!=NULL ) {		/* In a ttc file we may skip some */
	    info->chars[j]->width = lastwidth;
	    info->chars[j]->widthset = true;
	}
    }
}

static void readttfvwidths(FILE *ttf,struct ttfinfo *info) {
    int i,j;
    int lastvwidth = info->emsize, vwidth_cnt, tsb, cnt=0;
    int32 voff=0;

    fseek(ttf,info->vhea_start+4+4,SEEK_SET);		/* skip over the version number & typo right/left */
    info->pfminfo.vlinegap = getushort(ttf);

    for ( i=0; i<12; ++i )
	getushort(ttf);
    vwidth_cnt = getushort(ttf);

    fseek(ttf,info->vmetrics_start,SEEK_SET);
    for ( i=0; i<vwidth_cnt && i<info->glyph_cnt; ++i ) {
	lastvwidth = getushort(ttf);
	tsb = getushort(ttf);
	if ( info->chars[i]!=NULL ) {		/* can happen in ttc files */
	    info->chars[i]->vwidth = lastvwidth;
	    if ( info->cff_start==0 ) {
		voff += tsb + info->chars[i]->lsidebearing /* actually maxy */;
		++cnt;
	    }
	}
    }
    if ( i==0 )
	fprintf( stderr, "Invalid ttf vmtx table (or vhea), numOfLongVerMetrics is 0\n" );
	
    for ( j=i; j<info->glyph_cnt; ++j ) {
	if ( info->chars[j]!=NULL )		/* In a ttc file we may skip some */
	    info->chars[j]->vwidth = lastvwidth;
    }

    /* for truetype fonts the vertical offset is found by adding the ymax of a */
    /*  character to the top side bearing. I set the font wide value to the */
    /*  average of them all */
    /* But opentype doesn't give us the ymax easily, and rather than compute */
    /*  the bounding box I'll just punt and pick a reasonable value */
    /* Of course I hope it will be over riden by the VORG table */
    if ( cnt!=0 )
	info->vertical_origin = (voff+cnt/2)/cnt;
    if ( info->vertical_origin==0 )
	info->vertical_origin = info->ascent;
    if ( info->vorg_start!=0 ) {
	fseek(ttf,info->vorg_start+4,SEEK_SET);
	info->vertical_origin = (short) getushort(ttf);
    }
}

static struct dup *makedup(SplineChar *sc, int uni, int enc, struct dup *prev) {
    struct dup *d = gcalloc(1,sizeof(struct dup));

    d->sc = sc;
    d->uni = uni;
    d->enc = enc;
    d->prev = prev;
return( d );
}

static void dupfree(struct dup *dups) {
    struct dup *next;

    while ( dups!=NULL ) {
	next = dups->prev;
	free(dups);
	dups = next;
    }
}

static int modenc(int enc,int modtype) {
#if 0		/* to convert to jis208 (or later ksc5601) */
    if ( modtype==2 /* SJIS */ ) {
	if ( enc<=127 ) {
	    /* Latin */
	    enc += 94*96;
	} else if ( enc>=161 && enc<=223 ) {
	    /* Katakana */
	    enc += 96*96-160;
	} else {
	    int ch1 = enc>>8, ch2 = enc&0xff;
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
	    enc = (ch1-0x21)*96+(ch2-0x20);
	}
    } else if ( modtype==5 /* Wansung == KSC 5601-1987, I hope */ ) {
	if ( enc>0xa1a1 ) {
	    enc -= 0xa1a1;
	    enc = (enc>>8)*96 + (enc&0xff)+1;
	} else if ( enc<0x100 )
	    enc += 96*94;
    }
#endif
return( enc );
}

static int umodenc(int enc,int modtype) {
    if ( modtype==-1 )
return( -1 );
    if ( modtype<=1 /* Unicode */ ) {
	/* No conversion */;
    } else if ( modtype==2 /* SJIS */ ) {
	if ( enc<=127 ) {
	    /* Latin */
	    if ( enc=='\\' ) enc = 0xa5;	/* Yen */
	} else if ( enc>=161 && enc<=223 ) {
	    /* Katakana */
	    enc = unicode_from_jis201[enc];
	} else {
	    int ch1 = enc>>8, ch2 = enc&0xff;
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
	    enc = unicode_from_jis208[(ch1-0x21)*94+(ch2-0x21)];
	}
    } else if ( modtype==4 /* BIG5 */ ) {	/* old ms docs say big5 is modtype==3, but new ones say 4 */
	if ( enc>0xa100 )
	    enc = unicode_from_big5[enc-0xa100];
	else if ( enc>0x100 )
	    enc = -1;
    } else if ( modtype==5 /* Wansung == KSC 5601-1987, I hope */ ) {
	if ( enc>0xa1a1 ) {
	    enc -= 0xa1a1;
	    enc = (enc>>8)*94 + (enc&0xff);
	    enc = unicode_from_ksc5601[enc];
	    if ( enc==0 ) enc = -1;
	} else if ( enc>0x100 )
	    enc = -1;
    } else if ( modtype==6 /* Johab */ ) {
	if ( enc>0x8400 )
	    enc = unicode_from_johab[enc-0x8400];
	else if ( enc>0x100 )
	    enc = -1;
    }
    if ( enc==0 )
	enc = -1;
return( enc );
}

static void readttfencodings(FILE *ttf,struct ttfinfo *info, int justinuse) {
    int i,j;
    int nencs, version;
    enum charset enc = em_none;
    int platform, specific;
    int offset, encoff=0;
    int format, len;
    uint16 table[256];
    int segCount;
    uint16 *endchars, *startchars, *delta, *rangeOffset, *glyphs;
    int index, last;
    int mod = 0;
    const unichar_t *trans=NULL;
    SplineChar *sc;

    fseek(ttf,info->encoding_start,SEEK_SET);
    version = getushort(ttf);
    nencs = getushort(ttf);
    if ( version!=0 && nencs==0 )
	nencs = version;		/* Sometimes they are backwards */
    for ( i=0; i<nencs; ++i ) {
	platform = getushort(ttf);
	specific = getushort(ttf);
	offset = getlong(ttf);
	if ( platform==3 && specific==10 ) { /* MS Unicode 4 byte */
	    enc = em_unicode4;
	    encoff = offset;
	    mod = 0;
	} else if (( platform==3 && specific==1 && enc!=em_unicode4 ) || /* MS Unicode */
/* Well I should only deal with apple unicode specific==0 (default) and 3 (U2.0 semantics) */
/*  but apple ships dfonts with specific==1 (Unicode 1.1 semantics) */
/*  which is stupid of them */
		( platform==0 /*&& (specific==0 || specific==3)*/ && enc!=em_unicode4 && enc!=em_symbol )) {	/* Apple Unicode */
	    enc = em_unicode;
	    encoff = offset;
	    mod = 0;
	/* choose ms symbol over apple unicode. If there's an ms uncode it will */
	/*  come after ms symbol in the list and we'll get that */
	} else if ( platform==3 && specific==0 && (enc==em_none||enc==-2||enc==em_mac||enc==em_unicode) ) {
	    /* Only select symbol if we don't have something better */
	    enc = em_symbol;
	    encoff = offset;
	    /* Now I had assumed this would be a 1 byte encoding, but it turns*/
	    /*  out to map into the unicode private use area at U+f000-U+F0FF */
	    /*  so it's a 2 byte enc */
/* Mac platform specific encodings are script numbers. 0=>roman, 1=>jap, 2=>big5, 3=>korean, 4=>arab, 5=>hebrew, 6=>greek, 7=>cyrillic, ... 25=>simplified chinese */
	} else if ( platform==1 && specific==0 && (enc==em_none||enc==-2) ) {
	    enc = em_mac;
	    encoff = offset;
	    trans = unicode_from_mac;
	} else if ( platform==1 && (specific==2 ||specific==1) /*&& enc!=em_unicode*/ ) {
	    /* I've seen an example of a big5 encoding so I know this is right*/
	    /*  Japanese appears to be sjis */
	    enc = specific==1?em_sjis:specific==2?em_big5:specific==3?em_wansung:em_gb2312;
	    mod = specific==1?2:specific==2?4:specific==3?5:3;		/* convert to ms specific */
	    encoff = offset;
	} else if ( platform==3 && (specific==2 || specific==4 || specific==5 || specific==6 ) /*&&
		enc!=em_unicode*/ ) {
	    /* Old ms docs say that specific==3 => big 5, new docs say specific==4 => big5 */
	    /*  Ain't that jus' great? */
	    enc = specific==2? em_sjis : specific==5 ? em_wansung : specific==4? em_big5 : em_johab;
	    mod = specific;
	    encoff = offset;
	} else if ( enc==em_none ) {
	    enc = -2;
	    mod = -1;
	    encoff = offset;
	}
    }
    if ( enc!=em_none ) {
	fseek(ttf,info->encoding_start+encoff,SEEK_SET);
	format = getushort(ttf);
	if ( format!=12 && format!=10 && format!=8 ) {
	    len = getushort(ttf);
	    /* version/language = */ getushort(ttf);
	} else {
	    /* padding */ getushort(ttf);
	    len = getlong(ttf);
	    /* language = */ getlong(ttf);
	}
	if ( format==0 ) {
	    for ( i=0; i<len-6; ++i )
		table[i] = getc(ttf);
	    if ( enc==em_symbol && trans==NULL )
		trans = unicode_from_MacSymbol;
	    for ( i=0; i<256 && i<info->glyph_cnt && i<len-6; ++i )
		if ( !justinuse ) {
		    info->chars[table[i]]->enc = i;
		    if ( trans!=NULL )
			info->chars[table[i]]->unicodeenc = trans[i];
		} else
		    info->inuse[table[i]] = 1;
	} else if ( format==4 ) {
	    if ( enc==em_symbol ) { enc=em_unicode; info->twobytesymbol=true;}
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
	    len -= 8*sizeof(uint16) +
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
			if ( justinuse )
			    info->inuse[(uint16) (j+delta[i])] = true;
			else if ( info->chars[(uint16) (j+delta[i])]==NULL )
			    /* Do Nothing */;
			else if ( info->chars[(uint16) (j+delta[i])]->unicodeenc==-1 ) {
			    info->chars[(uint16) (j+delta[i])]->unicodeenc = umodenc(j,mod);
			    info->chars[(uint16) (j+delta[i])]->enc = modenc(j,mod);
			} else
			    info->dups = makedup(info->chars[(uint16) (j+delta[i])],umodenc(j,mod),modenc(j,mod),info->dups);
		    }
		} else if ( rangeOffset[i]!=0xffff ) {
		    /* It isn't explicitly mentioned by a rangeOffset of 0xffff*/
		    /*  means no glyph */
		    for ( j=startchars[i]; j<=endchars[i]; ++j ) {
			index = glyphs[ (i-segCount+rangeOffset[i]/2) +
					    j-startchars[i] ];
			if ( index!=0 ) {
			    index = (unsigned short) (index+delta[i]);
			    if ( index>=info->glyph_cnt )
				/* This isn't mentioned either, but in some */
			        /*  MS Chinese fonts (kaiu.ttf) the index */
			        /*  goes out of bounds. and MS's ttf dump */
			        /*  program says it is treated as 0 */;
			    else if ( justinuse )
				info->inuse[index] = 1;
			    else if ( info->chars[index]==NULL )
				/* Do Nothing */;
			    else if ( info->chars[index]->unicodeenc==-1 ) {
				info->chars[index]->unicodeenc = umodenc(j,mod);
				info->chars[index]->enc = modenc(j,mod);
			    } else
				info->dups = makedup(info->chars[index],umodenc(j,mod),modenc(j,mod),info->dups);
			}
		    }
		}
	    }
	    free(glyphs);
	    free(rangeOffset);
	    free(delta);
	    free(startchars);
	    free(endchars);
	} else if ( format==6 ) {
	    /* array unicode format */
	    int first, count;
	    if ( enc!=em_unicode && enc!=em_unicode4 )
		GDrawIError("I don't support truncated array encoding (format=6) except for unicode" );
	    first = getushort(ttf);
	    count = getushort(ttf);
	    if ( justinuse )
		for ( i=0; i<count; ++i )
		    info->inuse[getushort(ttf)]= 1;
	    else
		for ( i=0; i<count; ++i )
		    info->chars[getushort(ttf)]->unicodeenc = first+i;
	} else if ( format==2 ) {
	    int max_sub_head_key = 0, cnt;
	    struct subhead *subheads;

	    for ( i=0; i<256; ++i ) {
		table[i] = getushort(ttf)/8;	/* Sub-header keys */
		if ( table[i]>max_sub_head_key )
		    max_sub_head_key = table[i];	/* The entry is a byte pointer, I want a pointer in units of struct subheader */
	    }
	    subheads = galloc((max_sub_head_key+1)*sizeof(struct subhead));
	    for ( i=0; i<=max_sub_head_key; ++i ) {
		subheads[i].first = getushort(ttf);
		subheads[i].cnt = getushort(ttf);
		subheads[i].delta = getushort(ttf);
		subheads[i].rangeoff = (getushort(ttf)-
				(max_sub_head_key-i)*sizeof(struct subhead)-
				sizeof(short))/sizeof(short);
	    }
	    cnt = (len-(ftell(ttf)-(info->encoding_start+encoff)))/sizeof(short);
	    /* The count is the number of glyph indexes to read. it is the */
	    /*  length of the entire subtable minus that bit we've read so far */
	    glyphs = galloc(cnt*sizeof(short));
	    for ( i=0; i<cnt; ++i )
		glyphs[i] = getushort(ttf);
	    last = -1;
	    for ( i=0; i<256; ++i ) {
		if ( table[i]==0 ) {
		    /* Special case, single byte encoding entry, look i up in */
		    /*  subhead */
		    /* In the one example I've got of this encoding (wcl-02.ttf) the chars */
		    /* 0xfd, 0xfe, 0xff are said to exist but there is no mapping */
		    /* for them. */
		    if ( last!=-1 )
			index = 0;	/* the subhead says there are 256 entries, but in fact there are only 193, so attempting to find these guys should give an error */
		    else if ( i<subheads[0].first || i>=subheads[0].first+subheads[0].cnt ||
			    subheads[0].rangeoff+(i-subheads[0].first)>=cnt )
			index = 0;
		    else if ( (index = glyphs[subheads[0].rangeoff+(i-subheads[0].first)])!= 0 )
			index = (uint32) (index+subheads[0].delta);
		    /* I assume the single byte codes are just ascii or latin1*/
		    if ( index!=0 && index<info->glyph_cnt ) {
			if ( justinuse )
			    info->inuse[index] = 1;
			else if ( info->chars[index]==NULL )
			    /* Do Nothing */;
			else if ( info->chars[index]->unicodeenc==-1 ) {
			    info->chars[index]->unicodeenc = i;
			    info->chars[index]->enc = modenc(i,mod);
			} else
			    info->dups = makedup(info->chars[index],i,i,info->dups);
		    }
		} else {
		    int k = table[i];
		    for ( j=0; j<subheads[k].cnt; ++j ) {
			int enc;
			if ( subheads[k].rangeoff+j>=cnt )
			    index = 0;
			else if ( (index = glyphs[subheads[k].rangeoff+j])!= 0 )
			    index = (uint16) (index+subheads[k].delta);
			if ( index!=0 && index<info->glyph_cnt ) {
			    enc = (i<<8)|(j+subheads[k].first);
			    if ( justinuse )
				info->inuse[index] = 1;
			    else if ( info->chars[index]==NULL )
				/* Do Nothing */;
			    else if ( info->chars[index]->unicodeenc==-1 ) {
				info->chars[index]->unicodeenc = umodenc(enc,mod);
				info->chars[index]->enc = modenc(enc,mod);
			    } else
				info->dups = makedup(info->chars[index],umodenc(enc,mod),modenc(enc,mod),info->dups);
			}
		    }
		    if ( last==-1 ) last = i;
		}
	    }
	    free(subheads);
	    free(glyphs);
	} else if ( format==8 ) {
	    uint32 ngroups, start, end, startglyph;
	    if ( enc!=em_unicode4 )
		GDrawIError("I don't support 32 bit characters except for the UCS-4 (MS platform, specific=10)" );
	    /* I'm now assuming unicode surrogate encoding, so I just ignore */
	    /*  the is32 table (it will be set for the surrogates and not for */
	    /*  anything else */
	    fseek(ttf,8192,SEEK_CUR);
	    ngroups = getlong(ttf);
	    for ( j=0; j<ngroups; ++j ) {
		start = getlong(ttf);
		end = getlong(ttf);
		startglyph = getlong(ttf);
		if ( justinuse )
		    for ( i=start; i<=end; ++i )
			info->inuse[startglyph+i-start]= 1;
		else
		    for ( i=start; i<=end; ++i ) {
			(sc = info->chars[startglyph+i-start])->unicodeenc =
				((i>>16)-0xd800)*0x400 + (i&0xffff)-0xdc00 + 0x10000;
			sc->enc = sc->unicodeenc;
		    }
	    }
	} else if ( format==10 ) {
	    /* same as format 6, except for 4byte chars */
	    int first, count;
	    if ( enc!=em_unicode4 )
		GDrawIError("I don't support 32 bit characters except for the UCS-4 (MS platform, specific=10)" );
	    first = getlong(ttf);
	    count = getlong(ttf);
	    if ( justinuse )
		for ( i=0; i<count; ++i )
		    info->inuse[getushort(ttf)]= 1;
	    else
		for ( i=0; i<count; ++i ) {
		    (sc = info->chars[getushort(ttf)])->unicodeenc = first+i;
		    sc->enc = first+i;
		}
	} else if ( format==12 ) {
	    uint32 ngroups, start, end, startglyph;
	    if ( enc!=em_unicode4 )
		GDrawIError("I don't support 32 bit characters except for the UCS-4 (MS platform, specific=10)" );
	    ngroups = getlong(ttf);
	    for ( j=0; j<ngroups; ++j ) {
		start = getlong(ttf);
		end = getlong(ttf);
		startglyph = getlong(ttf);
		if ( justinuse )
		    for ( i=start; i<=end; ++i )
			info->inuse[startglyph+i-start]= 1;
		else
		    for ( i=start; i<=end; ++i ) {
			(sc = info->chars[startglyph+i-start])->unicodeenc = i;
			sc->enc = i;
		    }
	    }
	}
    }
    if ( info->chars!=NULL && info->chars[0]!=NULL && info->chars[0]->unicodeenc==0xffff &&
	    info->chars[0]->name!=NULL && strcmp(info->chars[0]->name,".notdef")==0 )
	info->chars[0]->unicodeenc = -1;
    info->encoding_name = enc;
}

static int EncFromName(const char *name) {
    int i;
    if ( name[0]=='u' && name[1]=='n' && name[2]=='i' ) {
	char *end;
	i = strtol(name+3,&end,16);
	if ( i>=0 && i<=0xffff && *end=='\0' )
return( i );
    } else if ( strlen(name)==4 ) {
	/* MS says use this kind of name, Adobe says use the one above */
	char *end;
	i = strtol(name,&end,16);
	if ( i>=0 && i<=0xffff && *end=='\0' )
return( i );
    }
	
    for ( i=0; i<psunicodenames_cnt; ++i ) if ( psunicodenames[i]!=NULL ) {
	if ( strcmp(psunicodenames[i],name)==0 )
return( i );
    }
return( -1 );
}

static void readttfos2metrics(FILE *ttf,struct ttfinfo *info) {
    int i;

    fseek(ttf,info->os2_start,SEEK_SET);
    /* version */ getushort(ttf);
    /* avgWidth */ getushort(ttf);
    info->pfminfo.weight = getushort(ttf);
    info->pfminfo.width = getushort(ttf);
    info->pfminfo.fstype = getushort(ttf);
    /* sub xsize */ getushort(ttf);
    /* sub ysize */ getushort(ttf);
    /* sub xoff */ getushort(ttf);
    /* sub yoff */ getushort(ttf);
    /* sup xsize */ getushort(ttf);
    /* sup ysize */ getushort(ttf);
    /* sup xoff */ getushort(ttf);
    /* sup yoff */ getushort(ttf);
    /* strike ysize */ getushort(ttf);
    /* strike ypos */ getushort(ttf);
    /* Family Class */ getushort(ttf);
    for ( i=0; i<10; ++i )
	info->pfminfo.panose[i] = getc(ttf);
    info->pfminfo.pfmfamily = info->pfminfo.panose[0]==2 ? 0x11 :	/* might be 0x21 */ /* Text & Display maps to either serif 0x11 or sans 0x21 or monospace 0x31 */
		      info->pfminfo.panose[0]==3 ? 0x41 :	/* Script */
		      info->pfminfo.panose[0]==4 ? 0x51 :	/* Decorative */
		      0x51;					/* And pictorial doesn't fit into pfm */
    if ( info->pfminfo.linegap==0 ) {
	/* unicoderange[] */ getlong(ttf);
	/* unicoderange[] */ getlong(ttf);
	/* unicoderange[] */ getlong(ttf);
	/* unicoderange[] */ getlong(ttf);
	/* vendor */ getlong(ttf);
	/* fsselection */ getushort(ttf);
	/* firstchar */ getushort(ttf);
	/* lastchar */ getushort(ttf);
	/* typoascender */ getushort(ttf);
	/* typodescender */ getushort(ttf);
	info->pfminfo.linegap = getushort(ttf);
    }
    info->pfminfo.pfmset = true;
}

static void readttfpostnames(FILE *ttf,struct ttfinfo *info) {
    int i,j;
    int format, len, gc, gcbig, val;
    const char *name;
    char buffer[30];
    uint16 *indexes;
    extern const char *ttfstandardnames[];

    GProgressChangeLine2R(_STR_ReadingNames);
    fseek(ttf,info->postscript_start,SEEK_SET);
    format = getlong(ttf);
    info->italicAngle = getfixed(ttf);
    info->upos = (short) getushort(ttf);
    info->uwidth = (short) getushort(ttf);
    /* fixedpitch = */ getlong(ttf);
    /* mem1 = */ getlong(ttf);
    /* mem2 = */ getlong(ttf);
    /* mem3 = */ getlong(ttf);
    /* mem4 = */ getlong(ttf);
    if ( format==0x00020000 ) {
	gc = getushort(ttf);
	indexes = gcalloc(65536,sizeof(uint16));
	/* the index table is backwards from the way I want to use it */
	gcbig = 0;
	for ( i=0; i<gc; ++i ) {
	    indexes[val = getushort(ttf)] = i;
	    if ( val>=258 ) ++gcbig;
	}

	/* if we are only loading bitmaps, we can get holes in our data */
	for ( i=0; i<258; ++i ) if ( indexes[i]!=0 || i==0 ) if ( info->chars[indexes[i]]!=NULL ) {
	    info->chars[indexes[i]]->name = copy(ttfstandardnames[i]);
	    if ( info->chars[indexes[i]]->unicodeenc==-1 )
		info->chars[indexes[i]]->unicodeenc = EncFromName(ttfstandardnames[i]);
	}
	gcbig += 258;
	for ( i=258; i<gcbig; ++i ) {
	    char *nm;
	    len = getc(ttf);
	    nm = galloc(len+1);
	    for ( j=0; j<len; ++j )
		nm[j] = getc(ttf);
	    nm[j] = '\0';
	    if ( info->chars[indexes[i]]!=NULL ) {
		info->chars[indexes[i]]->name = nm;
		if ( info->chars[indexes[i]]->unicodeenc==-1 )
		    info->chars[indexes[i]]->unicodeenc = EncFromName(nm);
	    }
	}
	free(indexes);
    }

    /* where no names are given, use the encoding to guess at them */
    /*  (or if the names default to the macintosh standard) */
    for ( i=0; i<info->glyph_cnt; ++i ) {
	/* info->chars[i] can be null in some TTC files */
	if ( info->chars[i]==NULL || info->chars[i]->name!=NULL )
    continue;
	if ( i==0 )
	    name = ".notdef";
	else if ( info->chars[i]->unicodeenc==-1 ) {
	    if ( info->chars[i]->enc!=0 )
		sprintf(buffer, "nounicode_%x", info->chars[i]->enc );
	    else
		sprintf( buffer, "unencoded_%d", i );
	    name = buffer;
	} else if ( info->chars[i]->unicodeenc<65536 &&
		psunicodenames[info->chars[i]->unicodeenc]!=NULL )
	    name = psunicodenames[info->chars[i]->unicodeenc];
	else {
	    sprintf( buffer, "uni%04X", info->chars[i]->unicodeenc );
	    name = buffer;
	}
	info->chars[i]->name = copy(name);
	GProgressNext();
    }
    GProgressNextStage();
}

static void readttfkerns(FILE *ttf,struct ttfinfo *info) {
    int tabcnt, len, coverage,i,j, npairs;
    int left, right, offset;
    KernPair *kp;

    fseek(ttf,info->kern_start,SEEK_SET);
    /* version = */ getushort(ttf);
    tabcnt = getushort(ttf);
    for ( i=0; i<tabcnt; ++i ) {
	/* version = */ getushort(ttf);
	len = getushort(ttf);
	coverage = getushort(ttf);
	if ( (coverage&7)==0x1 && (coverage&0xff00)==0 ) {
	    /* format 0, horizontal kerning data not perpendicular */
	    npairs = getushort(ttf);
	    /* searchRange = */ getushort(ttf);
	    /* entrySelector = */ getushort(ttf);
	    /* rangeShift = */ getushort(ttf);
	    for ( j=0; j<npairs; ++j ) {
		left = getushort(ttf);
		right = getushort(ttf);
		offset = (short) getushort(ttf);
		if ( left<info->glyph_cnt && right<info->glyph_cnt ) {
		    kp = gcalloc(1,sizeof(KernPair));
		    kp->sc = info->chars[right];
		    kp->off = offset;
		    kp->next = info->chars[left]->kerns;
		    info->chars[left]->kerns = kp;
		}
	    }
	} else {
	    fseek(ttf,len-6,SEEK_CUR);
	}
    }
}

static uint16 *getCoverageTable(FILE *ttf, int coverage_offset) {
    int format, cnt, i,j;
    uint16 *glyphs=NULL;
    int start, end, ind, max;

    fseek(ttf,coverage_offset,SEEK_SET);
    format = getushort(ttf);
    if ( format==1 ) {
	cnt = getushort(ttf);
	glyphs = galloc(cnt*sizeof(uint16));
	for ( i=0; i<cnt; ++i )
	    glyphs[i] = getushort(ttf);
    } else if ( format==2 ) {
	glyphs = galloc((max=256)*sizeof(uint16));
	cnt = getushort(ttf);
	for ( i=0; i<cnt; ++i ) {
	    start = getushort(ttf);
	    end = getushort(ttf);
	    ind = getushort(ttf);
	    if ( ind+end-start+1 >= max ) {
		max = ind+end-start+1;
		glyphs = grealloc(glyphs,max*sizeof(uint16));
	    }
	    for ( j=start; j<=end; ++j )
		glyphs[j-start+ind] = j;
	}
    }
return( glyphs );
}

struct valuerecord {
    int16 xplacement, yplacement;
    int16 xadvance, yadvance;
    uint16 offXplaceDev, offYplaceDev;
    uint16 offXadvanceDev, offYadvanceDev;
};

static uint16 *getClassDefTable(FILE *ttf, int classdef_offset, int cnt) {
    int format, i, j;
    uint16 start, glyphcnt, rangecnt, end, class;
    uint16 *glist=NULL;

    fseek(ttf, classdef_offset, SEEK_SET);
    glist = galloc(cnt*sizeof(uint16));
    for ( i=0; i<cnt; ++i )
	glist[i] = 0;	/* Class 0 is default */
    format = getushort(ttf);
    if ( format==1 ) {
	start = getushort(ttf);
	glyphcnt = getushort(ttf);
	for ( i=0; i<glyphcnt; ++i )
	    glist[start+i] = getushort(ttf);
    } else if ( format==2 ) {
	rangecnt = getushort(ttf);
	for ( i=0; i<rangecnt; ++i ) {
	    start = getushort(ttf);
	    end = getushort(ttf);
	    class = getushort(ttf);
	    for ( j=start; j<=end; ++j )
		glist[j] = class;
	}
    }
return glist;
}

static void readvaluerecord(struct valuerecord *vr,int vf,FILE *ttf) {
    memset(vr,'\0',sizeof(struct valuerecord));
    if ( vf&1 )
	vr->xplacement = getushort(ttf);
    if ( vf&2 )
	vr->yplacement = getushort(ttf);
    if ( vf&4 )
	vr->xadvance = getushort(ttf);
    if ( vf&8 )
	vr->yadvance = getushort(ttf);
    if ( vf&0x10 )
	vr->offXplaceDev = getushort(ttf);
    if ( vf&0x20 )
	vr->offYplaceDev = getushort(ttf);
    if ( vf&0x40 )
	vr->offXadvanceDev = getushort(ttf);
    if ( vf&0x80 )
	vr->offYadvanceDev = getushort(ttf);
}

static void addKernPair(struct ttfinfo *info, int glyph1, int glyph2, int16 offset) {
    KernPair *kp;
    if ( glyph1<info->glyph_cnt && glyph2<info->glyph_cnt ) {
	for ( kp=info->chars[glyph1]->kerns; kp!=NULL; kp=kp->next )
	    if ( kp->sc == info->chars[glyph2] )
	break;
	if ( kp==NULL ) {
	    kp = gcalloc(1,sizeof(KernPair));
	    kp->sc = info->chars[glyph2];
	    kp->off = offset;
	    kp->next = info->chars[glyph1]->kerns;
	    info->chars[glyph1]->kerns = kp;
	}
    }
}

static void gposKernSubTable(FILE *ttf, int which, int stoffset, struct ttfinfo *info) {
    int coverage, cnt, i, j, pair_cnt, vf1, vf2, glyph2;
    int cd1, cd2, c1_cnt, c2_cnt, k, l;
    int16 offset;
    uint16 format;
    uint16 *ps_offsets;
    uint16 *glyphs, *class1, *class2;
    struct valuerecord vr1, vr2;
    long foffset;

    format=getushort(ttf);
    if ( format!=1 && format!=2 )	/* Unknown subtable format */
return;
    coverage = getushort(ttf);
    vf1 = getushort(ttf);
    if ( vf1&0xffbb)	/* Not interested in things that deal with y advance/placement nor with x placement */
return;
    vf2 = getushort(ttf);
    if ( vf2&0xffaa)	/* Not interested in things that deal with y advance/placement */
return;
    if ( format==1 ) {
	cnt = getushort(ttf);
	ps_offsets = galloc(cnt*sizeof(uint16));
	for ( i=0; i<cnt; ++i )
	    ps_offsets[i]=getushort(ttf);
	glyphs = getCoverageTable(ttf,stoffset+coverage);
	if ( glyphs==NULL )
return;
	for ( i=0; i<cnt; ++i ) {
	    fseek(ttf,stoffset+ps_offsets[i],SEEK_SET);
	    pair_cnt = getushort(ttf);
	    for ( j=0; j<pair_cnt; ++j ) {
		glyph2 = getushort(ttf);
		readvaluerecord(&vr1,vf1,ttf);
		readvaluerecord(&vr2,vf2,ttf);
		addKernPair(info, glyphs[i], glyph2, vr1.xadvance+vr2.xplacement);
	    }
	}
	free(ps_offsets); free(glyphs);
    } else if ( format==2 ) {	/* Class-based kerning */
	cd1 = getushort(ttf);
	cd2 = getushort(ttf);
	foffset = ftell(ttf);
	class1 = getClassDefTable(ttf, stoffset+cd1, info->glyph_cnt);
	class2 = getClassDefTable(ttf, stoffset+cd2, info->glyph_cnt);
	fseek(ttf, foffset, SEEK_SET);	/* come back */
	c1_cnt = getushort(ttf);
	c2_cnt = getushort(ttf);
	for ( i=0; i<c1_cnt; ++i) {
	    for ( j=0; j<c2_cnt; ++j) {
		readvaluerecord(&vr1,vf1,ttf);
		readvaluerecord(&vr2,vf2,ttf);
		offset = vr1.xadvance+vr2.xplacement;
		if( offset!=0 )
		    for ( k=0; k<info->glyph_cnt; ++k )
			if ( class1[k]==i )
			    for ( l=0; l<info->glyph_cnt; ++l )
				if ( class2[l]==j )
				    addKernPair(info, k, l, offset);
	    }
	}
	free(class1); free(class2);
    }
}

static void gsubLigatureSubTable(FILE *ttf, int which, int stoffset,struct ttfinfo *info) {
    int coverage, cnt, i, j, k, lig_cnt, cc, len;
    uint16 *ls_offsets, *lig_offsets;
    uint16 *glyphs, *lig_glyphs, lig;
    char *pt;

    /* Type = */ getushort(ttf);
    coverage = getushort(ttf);
    cnt = getushort(ttf);
    ls_offsets = galloc(cnt*sizeof(uint16));
    for ( i=0; i<cnt; ++i )
	ls_offsets[i]=getushort(ttf);
    glyphs = getCoverageTable(ttf,stoffset+coverage);
    if ( glyphs==NULL )
return;
    for ( i=0; i<cnt; ++i ) {
	fseek(ttf,stoffset+ls_offsets[i],SEEK_SET);
	lig_cnt = getushort(ttf);
	lig_offsets = galloc(lig_cnt*sizeof(uint16));
	for ( j=0; j<lig_cnt; ++j )
	    lig_offsets[j] = getushort(ttf);
	for ( j=0; j<lig_cnt; ++j ) {
	    fseek(ttf,stoffset+ls_offsets[i]+lig_offsets[j],SEEK_SET);
	    lig = getushort(ttf);
	    cc = getushort(ttf);
	    lig_glyphs = galloc(cc*sizeof(uint16));
	    lig_glyphs[0] = glyphs[i];
	    for ( k=1; k<cc; ++k )
		lig_glyphs[k] = getushort(ttf);
	    for ( k=len=0; k<cc; ++k )
		len += strlen(info->chars[lig_glyphs[k]]->name)+1;
	    if ( info->chars[lig]->lig!=NULL ) {
		len += strlen( info->chars[lig]->lig->components)+8;
		info->chars[lig]->lig->components = pt = grealloc(info->chars[lig]->lig->components,len);
		pt += strlen(pt);
		*pt++ = ';';
	    } else {
		info->chars[lig]->lig = galloc(sizeof(Ligature));
		info->chars[lig]->lig->lig = info->chars[lig];
		info->chars[lig]->lig->components = pt = galloc(len);
	    }
	    for ( k=0; k<cc; ++k ) {
		strcpy(pt,info->chars[lig_glyphs[k]]->name);
		pt += strlen(pt);
		*pt++ = ' ';
	    }
	    pt[-1] = '\0';
	}
	free(lig_offsets);
    }
    free(ls_offsets); free(glyphs);
}

static int *readttffeatures(FILE *ttf,int32 pos,int tag) {
    /* read the features table returning an array containing all features */
    /*  lookup indeces which match the feature tag */
    int cnt, next, lcnt;
    int i,j,k,max;
    int32 here = ftell(ttf);
    int *lookups, *ft_offsets;

    fseek(ttf,pos,SEEK_SET);
    cnt = getushort(ttf);
    if ( cnt<=0 ) {
	fseek(ttf,here,SEEK_SET);
return( NULL );
    }
    ft_offsets = galloc(cnt*sizeof(int));
    for ( i=j=0; i<cnt; ++i ) {
	int ftag = getlong(ttf);
	int offset = getushort(ttf);
	if ( ftag==tag )
	    ft_offsets[j++] = offset;
    }
    cnt = j;
    if ( cnt==0 ) {
	fseek(ttf,here,SEEK_SET);
	free(ft_offsets);
return( NULL );
    }

    max = cnt;
    lookups = galloc((cnt+1)*sizeof(int));
    next = 0;
    for ( i=0; i<cnt; ++i ) {
	fseek(ttf,pos+ft_offsets[i],SEEK_SET);
	/* feature parameters = */ getushort(ttf);
	lcnt = getushort(ttf);
	if ( next+lcnt>cnt ) {
	    max = next+lcnt+(cnt-i)*2;
	    lookups = grealloc(lookups,(max+1)*sizeof(int));
	}
	for ( j=0; j<lcnt; ++j )
	    lookups[next++] = getushort(ttf);
    }

    free( ft_offsets );
    fseek(ttf,here,SEEK_SET);
    if ( next==0 ) {
	free(lookups);
return( NULL );
    }

    /* The same lookup may appear in several features, so remove duplicates */
    for ( i=0; i<next; ++i ) {
	for ( j=i+1; j<next; ++j ) {
	    while ( j<next && lookups[i]==lookups[j] ) {
		--next;
		for ( k=j; k<next; ++k )
		    lookups[k] = lookups[k+1];
	    }
	}
    }
    lookups[next] = -1;
return( lookups );
}

static void readttfgpossub(FILE *ttf,struct ttfinfo *info,int gpos) {
    int i, j, k, lu_cnt, lu_type, flags, cnt, st;
    uint16 *lu_offsets, *st_offsets;
    int32 base, lookup_start;
    int *lookups;

    base = gpos?info->gpos_start:info->gsub_start;
    fseek(ttf,base,SEEK_SET);
    /* version = */ getlong(ttf);
    /* Script list offset = */ getushort(ttf);
    lookups = readttffeatures(ttf,base+getushort(ttf),gpos?CHR('k','e','r','n'):CHR('l','i','g','a'));
    if ( lookups==NULL )		/* None of the data we care about */
return;
    lookup_start = base+getushort(ttf);
    fseek(ttf,lookup_start,SEEK_SET);

    lu_cnt = getushort(ttf);
    lu_offsets = galloc(lu_cnt*sizeof(uint16));
    for ( i=0; i<lu_cnt; ++i )
	lu_offsets[i] = getushort(ttf);
    for ( k=0; lookups[k]!=-1; ++k ) {
	i = lookups[k];
	if ( i>=lu_cnt )
    continue;
	fseek(ttf,lookup_start+lu_offsets[i],SEEK_SET);
	lu_type = getushort(ttf);
	flags = getushort(ttf);
	cnt = getushort(ttf);
	st_offsets = galloc(cnt*sizeof(uint16));
	for ( j=0; j<cnt; ++j )
	    st_offsets[j] = getushort(ttf);
	for ( j=0; j<cnt; ++j ) {
	    fseek(ttf,st = lookup_start+lu_offsets[i]+st_offsets[j],SEEK_SET);
	    switch ( lu_type ) {
	      case 2:
		if ( gpos )
		    gposKernSubTable(ttf,j,st,info);
	      break;
	      case 4:
		if ( !gpos )
		    gsubLigatureSubTable(ttf,j,st,info);
	      break;
	    }
	}
	free(st_offsets);
    }
    free(lu_offsets);
    free(lookups);
}

static int ttfFixupRef(struct ttfinfo *info,int i) {
    RefChar *ref, *prev, *next;

    if ( info->chars[i]==NULL )		/* Can happen in ttc files */
return( false );
    if ( info->chars[i]->ticked )
return( false );
    info->chars[i]->ticked = true;
    prev = NULL;
    for ( ref=info->chars[i]->refs; ref!=NULL; ref=next ) {
	if ( ref->sc!=NULL )
    break;				/* Already done */
	next = ref->next;
	if ( !ttfFixupRef(info,ref->local_enc)) {
	    if ( prev==NULL )
		info->chars[i]->refs = next;
	    else
		prev->next = next;
	    free(ref);
	} else {
	    ref->sc = info->chars[ref->local_enc];
	    ref->adobe_enc = getAdobeEnc(ref->sc->name);
	    SCReinstanciateRefChar(info->chars[i],ref);
	    SCMakeDependent(info->chars[i],ref->sc);
	    prev = ref;
	}
    }
    info->chars[i]->ticked = false;
return( true );
}

static void ttfFixupReferences(struct ttfinfo *info) {
    int i;

    GProgressChangeLine2R(_STR_FixingupReferences);
    for ( i=0; i<info->glyph_cnt; ++i ) {
	ttfFixupRef(info,i);
	GProgressNext();
    }
    GProgressNextStage();
}

static int readttf(FILE *ttf, struct ttfinfo *info, char *filename) {
    int i;
    char *oldloc;

    GProgressChangeStages(3);
    if ( !readttfheader(ttf,info,filename)) {
return( 0 );
    }
    oldloc = setlocale(LC_NUMERIC,"C");		/* TrueType doesn't need this but opentype dictionaries do */
    readttfpreglyph(ttf,info);
    GProgressChangeTotal(info->glyph_cnt);

    if ( info->onlystrikes )
	info->chars = gcalloc(info->glyph_cnt+1,sizeof(SplineChar *));
    else if ( info->glyphlocations_start!=0 && info->glyph_start!=0 )
	readttfglyphs(ttf,info);
    else if ( info->cff_start!=0 ) {
	if ( !readcffglyphs(ttf,info) ) {
return( 0 );
	}
    } else {
return( 0 );
    }
    if ( info->bitmapdata_start!=0 && info->bitmaploc_start!=0 )
	TTFLoadBitmaps(ttf,info,info->onlyonestrike);
    else if ( info->onlystrikes )
	GWidgetErrorR( _STR_NoBitmaps, _STR_NoBitmapsInTTF, filename==NULL ? "<unknown>" : filename );
    if ( info->onlystrikes && info->bitmaps==NULL )
return( 0 );
    if ( info->hmetrics_start!=0 )
	readttfwidths(ttf,info);
    if ( info->vmetrics_start!=0 && info->vhea_start!=0 )
	readttfvwidths(ttf,info);
    if ( info->cidregistry==NULL )
	readttfencodings(ttf,info,false);
    if ( info->os2_start!=0 )
	readttfos2metrics(ttf,info);
    if ( info->postscript_start!=0 )
	readttfpostnames(ttf,info);
    if ( info->kern_start!=0 )
	readttfkerns(ttf,info);
    if ( info->gpos_start!=0 )		/* kerning info may live in the gpos table too */
	readttfgpossub(ttf,info,true);
    if ( info->gsub_start!=0 )
	readttfgpossub(ttf,info,false);
    else
	for ( i=0; i<info->glyph_cnt; ++i )
	    if ( info->chars[i]!=NULL )		/* Might be null in ttc files */
		info->chars[i]->lig = SCLigDefault(info->chars[i]);
    setlocale(LC_NUMERIC,oldloc);
    ttfFixupReferences(info);
return( true );
}

static SplineChar *SFMakeDupRef(SplineFont *sf, int local_enc, struct dup *dup) {
    SplineChar *sc;

    sc = MakeDupRef(dup->sc,local_enc,dup->uni);
    sc->parent = sf;
return( sc );
}

static void SymbolFixup(struct ttfinfo *info) {
    SplineChar *lo[256], *hi[256], *sc, **chars;
    int extras=0, i, uenc;
    struct dup *dup;

    memset(lo,0,sizeof(lo));
    memset(hi,0,sizeof(hi));
    if ( info->chars[0]!=NULL && info->chars[0]->enc==0 )
	lo[0] = info->chars[0];
    for ( i=0; i<info->glyph_cnt; ++i ) if ( (sc = info->chars[i])!=NULL ) {
	if ( sc->enc>0 && sc->enc<=0xff )
	    lo[sc->enc] = sc;
	else if ( sc->enc>=0xf000 && sc->enc<=0xf0ff )
	    hi[sc->enc-0xf000] = sc;
	else if ( sc->enc!=0 )
return;		/* Leave it as it is, it isn't a real symbol encoding */
    }
    for ( dup=info->dups; dup!=NULL; dup=dup->prev ) {
	if ( !((dup->enc>0 && dup->enc<=0xff ) || ( dup->enc>=0xf000 && dup->enc<=0xf0ff )) )
return;
    }
    extras = 0;
    for ( i=1; i<info->glyph_cnt; ++i ) if ( (sc = info->chars[i])!=NULL ) {
	if ( sc->enc==0 )
	    sc->enc = 256 + extras++;
    }
    for ( dup=info->dups; dup!=NULL; dup=dup->prev ) {
	if ( dup->enc>0 && dup->enc<=0xff )
	    lo[dup->enc] = MakeDupRef(dup->sc,dup->enc,dup->uni);
	else
	    hi[dup->enc-0xf000] = MakeDupRef(dup->sc,dup->enc,dup->uni);
    }
    for ( i=0; i<256; ++i ) {
	if ( hi[i]!=NULL && lo[i]!=NULL ) {
	    lo[i]->enc = 256+extras++;
	    hi[i]->enc -= 0xf000;
	} else if ( hi[i]!=NULL )
	    hi[i]->enc -= 0xf000;
    }
    chars = gcalloc(256+extras,sizeof(SplineChar *));
    for ( i=0; i<info->glyph_cnt; ++i ) if ( (sc = info->chars[i])!=NULL )
	chars[sc->enc] = sc;
    for ( i=0; i<256; ++i ) if ( (sc = lo[i])!=NULL )
	chars[sc->enc] = sc;
    for ( i=0; i<256; ++i ) if ( (sc = hi[i])!=NULL )
	chars[sc->enc] = sc;
    for ( i=0; i<256+extras; ++i ) if ( (sc=chars[i])!=NULL ) {
	uenc = UnicodeNameLookup(sc->name);
	if ( uenc!=-1 )
	    sc->unicodeenc = uenc;
    }
    if ( chars[0x41]!=NULL && chars[0x41]->name!=NULL && strcmp(chars[0x41]->name,"Alpha")==0 )
	info->encoding_name = em_symbol;
    else
	info->encoding_name = em_none;
    free(info->chars);
    info->chars = chars;
    info->glyph_cnt = 256+extras;
    info->is_onebyte = true;

    dupfree(info->dups);
    info->dups = NULL;
}

static void CheckEncoding(struct ttfinfo *info) {
    const uint16 *table;
    static const uint16 *choices[] = { unicode_from_MacSymbol, unicode_from_mac, 
	unicode_from_win, unicode_from_i8859_1, unicode_from_adobestd,
	unicode_from_ZapfDingbats, NULL };
    static int encs[] = { em_symbol, em_mac, em_win, em_iso8859_1,
	em_adobestandard, em_zapfding };
    int i,j, extras, errs;
    SplineChar **chars;

    extras = 0;
    for ( j=0; j<info->glyph_cnt; ++j )
	if ( info->chars[j]->enc==0 && j!=0 )
	    ++extras;
    chars = gcalloc(256+extras,sizeof(SplineChar *));
    extras = 0;
    for ( j=0; j<info->glyph_cnt; ++j ) {
	if ( info->chars[j]->enc==0 && j!=0 ) {
	    chars[256+extras] = info->chars[j];
	    info->chars[j]->enc = 256+extras++;
	} else
	    chars[info->chars[j]->enc] = info->chars[j];
    }

    info->encoding_name = em_none;
    for ( i=0; choices[i]!=NULL; ++i ) {
	table = choices[i];
	errs = 0;
	for ( j=0; j<256 ; ++j ) if ( chars[j]!=NULL ) {
	    if ( table[j]!=chars[j]->unicodeenc && table[j]!=0 )
		++errs;
	}
	if ( j==256 && errs<11 ) {
	    info->encoding_name = encs[i];
    break;
	}
    }
    free(info->chars);
    info->chars = chars;
    info->glyph_cnt = 256+extras;
    info->is_onebyte = true;
}

static void UseGivenEncoding(SplineFont *sf,struct ttfinfo *info) {
    int istwobyte = false, i, oldcnt = info->glyph_cnt, extras=0, epos, max=96*94;
    SplineChar **oldchars = info->chars, **newchars;
    struct dup *dup;
    BDFFont *bdf;
    BDFChar **obc;
    RefChar *rf;
    int newcharcnt;

    if ( info->is_onebyte ) {
	/* We did most of this in CheckEncoding */
	max = 256;
	for ( i=0; i<oldcnt; ++i ) if ( oldchars[i]!=NULL ) {
	    if ( oldchars[i]->enc>=256 )
		++extras;
	    oldchars[i]->parent = sf;
	}
    } else {
	istwobyte = true;
	for ( i=0; i<oldcnt; ++i ) if ( oldchars[i]!=NULL ) {
	    if ( oldchars[i]->enc>=256 ) {
		istwobyte = true;
		if ( oldchars[i]->enc>max ) max = oldchars[i]->enc;
	    } else if ( oldchars[i]->enc==0 && i!=0 )
		++extras;
	    oldchars[i]->parent = sf;
	}
    }
    for ( dup=info->dups; dup!=NULL && !istwobyte; dup=dup->prev )
	if ( dup->enc>=256 ) {
	    istwobyte = true;
	    if ( dup->enc>max ) max = dup->enc;
	}

    if ( info->encoding_name>=em_first2byte && info->encoding_name<=em_last94x94 )
	epos = ((max+95)/96)*96;
    else if ( info->encoding_name==em_unicode4 )
	epos = (max>=unicode4_size) ? max+1 : unicode4_size;
    else
	epos = istwobyte?65536:256;
    newcharcnt = epos+extras;
    newchars = gcalloc(newcharcnt,sizeof(SplineChar *));
    for ( i=0; i<oldcnt; ++i ) if ( oldchars[i]!=NULL ) {
	if ( oldchars[i]->enc!=0 || i==0 )
	    newchars[oldchars[i]->enc] = oldchars[i];
	else {
	    oldchars[i]->enc = epos;
	    newchars[epos++] = oldchars[i];
	}
    }

    for ( i=0; i<newcharcnt; ++i ) if ( newchars[i]!=NULL ) {
	for ( rf = newchars[i]->refs; rf!=NULL; rf = rf->next ) {
	    rf->local_enc = rf->sc->enc;
	    rf->unicode_enc = rf->sc->unicodeenc;
	}
    }

    sf->chars = oldchars;
    for ( dup=info->dups; dup!=NULL; dup=dup->prev )
	if ( newchars[dup->enc]==NULL )
	    newchars[dup->enc] = SFMakeDupRef(sf,dup->enc,dup);
    sf->chars = newchars; sf->charcnt = newcharcnt;
    free(oldchars);

    sf->encoding_name = info->encoding_name==-2? em_none : info->encoding_name;

    for ( bdf=sf->bitmaps; bdf!=NULL; bdf = bdf->next ) {
	bdf->encoding_name = sf->encoding_name;
	obc = bdf->chars;
	bdf->chars = gcalloc(sf->charcnt,sizeof(BDFChar *));
	for ( i=0; i<bdf->charcnt; ++i ) if ( obc[i]!=NULL ) {
	    bdf->chars[obc[i]->sc->enc] = obc[i];
	    obc[i]->enc = obc[i]->sc->enc;
	}
	bdf->charcnt = sf->charcnt;
	free(obc);
    }

    dupfree(info->dups);
    info->dups = NULL;
}

static SplineFont *SFFillFromTTF(struct ttfinfo *info) {
    SplineFont *sf;
    int i;
    BDFFont *bdf;

    sf = SplineFontEmpty();
    sf->display_size = -default_fv_font_size;
    sf->display_antialias = default_fv_antialias;
    sf->fontname = info->fontname;
    sf->fullname = info->fullname;
    sf->familyname = info->familyname;
    if ( sf->fontname==NULL ) {
	sf->fontname = copy(sf->fullname);
	if ( sf->fontname==NULL )
	    sf->fontname = copy(sf->familyname);
	if ( sf->fontname==NULL ) sf->fontname = copy("UntitledTTF");
    }
    if ( sf->fullname==NULL ) sf->fullname = copy( sf->fontname );
    if ( sf->familyname==NULL ) sf->familyname = copy( sf->fontname );
    sf->weight = info->weight ? info->weight : 
		copy( info->pfminfo.weight <= 100 ? "Thin" :
			info->pfminfo.weight <= 200 ? "Extra-Light" :
			info->pfminfo.weight <= 300 ? "Light" :
			info->pfminfo.weight <= 400 ? "Book" :
			info->pfminfo.weight <= 500 ? "Medium" :
			info->pfminfo.weight <= 600 ? "Demi" :
			info->pfminfo.weight <= 700 ? "Bold" :
			info->pfminfo.weight <= 800 ? "Heavy" :
			    "Black" );
    sf->copyright = info->copyright;
    sf->version = info->version;
    sf->italicangle = info->italicAngle;
    sf->upos = info->upos;
    sf->uwidth = info->uwidth;
    sf->ascent = info->ascent;
    sf->vertical_origin = info->vertical_origin;
    if ( info->vhea_start!=0 && info->vmetrics_start!=0 )
	sf->hasvmetrics = true;
    sf->descent = info->descent;
    sf->private = info->private;
    sf->xuid = info->xuid;
    sf->pfminfo = info->pfminfo;
    sf->names = info->names;
    if ( info->encoding_name == em_symbol || info->encoding_name == em_mac )
	/* Don't trust those encodings */
	CheckEncoding(info);
    if ( info->twobytesymbol )
	/* rework ms symbol encodings */
	SymbolFixup(info);
    sf->encoding_name = em_none;
    sf->cidregistry = info->cidregistry;
    sf->ordering = info->ordering;
    sf->supplement = info->supplement;
    sf->cidversion = info->cidfontversion;
    sf->bitmaps = info->bitmaps;
    for ( bdf = info->bitmaps; bdf!=NULL; bdf = bdf->next ) {
	bdf->encoding_name = info->encoding_name;
	bdf->sf = sf;
    }

    if ( info->subfontcnt == 0 ) {
	UseGivenEncoding(sf,info);
    } else {
	sf->subfontcnt = info->subfontcnt;
	sf->subfonts = info->subfonts;
	free(info->chars);		/* This is the GID->char index, don't need it now */
	for ( i=0; i<sf->subfontcnt; ++i ) {
	    sf->subfonts[i]->cidmaster = sf;
	    sf->subfonts[i]->vertical_origin = sf->vertical_origin;
	    sf->subfonts[i]->hasvmetrics = sf->hasvmetrics;
	}
    }
    TTF_PSDupsDefault(sf);
return( sf );
}

SplineFont *_SFReadTTF(FILE *ttf, int flags,char *filename) {
    struct ttfinfo info;
    int ret;

    memset(&info,'\0',sizeof(struct ttfinfo));
    info.onlystrikes = (flags&ttf_onlystrikes)?1:0;
    info.onlyonestrike = (flags&ttf_onlyonestrike)?1:0;
    ret = readttf(ttf,&info,filename);
    if ( !ret )
return( NULL );
return( SFFillFromTTF(&info));
}

SplineFont *SFReadTTF(char *filename, int flags) {
    FILE *ttf;
    SplineFont *sf;
    char *temp=filename, *pt;

    if ( strchr(filename,'(')!=NULL ) {
	temp = copy(filename);
	pt = strchr(temp,'(');
	*pt = '\0';
    }
    ttf = fopen(temp,"r");
    if ( temp!=filename ) free(temp);
    if ( ttf==NULL )
return( NULL );

    sf = _SFReadTTF(ttf,flags,filename);
    fclose(ttf);
return( sf );
}

/* This routine is for reading any bare CFF data in a stand alone file */
/*  I doubt it will ever happen, but it's fairly easy... */
SplineFont *CFFParse(FILE *temp,int len, char *fontsetname) {
    struct ttfinfo info;

    memset(&info,'\0',sizeof(info));
    info.cff_start = 0;
    info.cff_length = len;
    if ( !readcffglyphs(temp,&info) )
return( NULL );
return( SFFillFromTTF(&info));
}
