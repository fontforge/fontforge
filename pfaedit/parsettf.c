/* Copyright (C) 2000-2003 by George Williams */
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
/* It grows on you though... now that I understand it better it seems better designed */
/*  but the docs remain in conflict. Sometimes badly so */

/* !!!I don't currently parse instructions to get hints, but I do save them */

int prefer_cjk_encodings=false;

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
    } else if ( enc==em_big5 || enc==em_big5hkscs ) {
	str = pt = galloc(2*len+2);	/* Probably more space than we need, but it should be enough */
	for ( i=0; i<len; ++i ) {
	    ch = getc(ttf);
	    if ( ch==0 ) {
		/* Interesting. I've never seen this described, but empirically */
		/*  it appears that a leading 0 byte means an ascii byte follows */
		/*  I thought we got mixed 8/16 encoding, not straight 16... */
		*pt++ = getc(ttf);
		++i;
	    } else if ( ch<0x81 )
		*pt++ = ch;
	    else {
		ch = ((ch<<8)|getc(ttf));
		*pt++ = unicode_from_big5hkscs[ch-0x8100];
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
	    enc = em_big5hkscs;		/* Or should we just guess big5? Both are wrong sometimes */
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
	    enc = em_big5hkscs;
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
return( 0x40e );
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
    char *pt, *lparen;

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
    pt = strrchr(filename,'/');
    if ( pt==NULL ) pt = filename;
    if ( (lparen = strchr(pt,'('))!=NULL && strchr(lparen,')')!=NULL ) {
	char *find = copy(lparen+1);
	pt = strchr(find,')');
	if ( pt!=NULL ) *pt='\0';
	for ( choice=cnt-1; choice>=0; --choice )
	    if ( uc_strcmp(names[choice],find)==0 )
	break;
	if ( choice==-1 ) {
	    char *fn = copy(filename);
	    fn[lparen-filename] = '\0';
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

    /* Apple says that 'typ1' is a valid code for a type1 font wrapped up in */
    /*  a truetype table structure, but gives no docs on what tables get used */
    /*  or how */
    if ( version==CHR('t','y','p','1')) {
	if ( filename==NULL ) filename = "it";
	fprintf( stderr, "Interesting, I've never seen an example of this type of font, could you\nsend me a copy of %s?\nThanks\n  gww@silcom.com\n", filename );
    }
    if ( version!=0x00010000 && version!=CHR('t','r','u','e') &&
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
	    info->cff_length = length;
	  break;
	  case CHR('c','m','a','p'):
	    info->encoding_start = offset;
	  break;
	  case CHR('g','l','y','f'):
	    info->glyph_start = offset;
	    info->glyph_length = length;
	  break;
	  case CHR('G','D','E','F'):
	    info->gdef_start = offset;
	  break;
	  case CHR('G','P','O','S'):
	    info->gpos_start = offset;
	  break;
	  case CHR('G','S','U','B'):
	    info->gsub_start = offset;
	  break;
	  case CHR('b','d','a','t'):		/* Apple/MS use a different tag, but the same format. Great. */
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
	    info->maxp_len = length;
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
	      /* Apple stuff */
	  case CHR('a','c','n','t'):
	    info->acnt_start = offset;
	  break;
	  case CHR('f','e','a','t'):
	    info->feat_start = offset;
	  break;
	  case CHR('l','c','a','r'):
	    info->lcar_start = offset;
	  break;
	  case CHR('m','o','r','t'):
	    info->mort_start = offset;
	  break;
	  case CHR('m','o','r','x'):
	    info->morx_start = offset;
	  break;
	  case CHR('o','p','b','d'):
	    info->opbd_start = offset;
	  break;
	  case CHR('p','r','o','p'):
	    info->prop_start = offset;
	  break;
	      /* to make sense of instrs */
	  case CHR('c','v','t',' '):
	    info->cvt_start = offset;
	    info->cvt_len = length;
	  break;
	  case CHR('p','r','e','p'):
	    info->prep_start = offset;
	    info->prep_len = length;
	  break;
	  case CHR('f','p','g','m'):
	    info->fpgm_start = offset;
	    info->fpgm_len = length;
	  break;
	  case CHR('P','f','E','d'):
	    info->pfed_start = offset;
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
    /*info->pfminfo.hhead_ascent =*/ getushort(ttf);
    /*info->pfminfo.hhead_descent = (short)*/ getushort(ttf);
    info->pfminfo.linegap = getushort(ttf);
    info->ascent = info->pfminfo.hhead_ascent;

    /* fontographer puts the max ascender/min descender here instead. idiots */
    if (( info->ascent==0 && info->descent==0 ) || info->ascent>info->emsize )
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

static void FigureControls(SplinePoint *from, SplinePoint *to, BasePoint *cp,
	int is_order2) {
    /* What are the control points for 2 cp bezier which will provide the same*/
    /*  curve as that for the 1 cp bezier specified above */
    real b, c, d;

    if ( is_order2 ) {
	from->nextcp = to->prevcp = *cp;
    } else {
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
    }

    if ( from->me.x!=from->nextcp.x || from->me.y!=from->nextcp.y )
	from->nonextcp = false;
    if ( to->me.x!=to->prevcp.x || to->me.y!=to->prevcp.y )
	to->noprevcp = false;
    if ( is_order2 && (to->noprevcp || from->nonextcp)) {
	to->noprevcp = from->nonextcp = true;
	from->nextcp = from->me;
	to->prevcp = to->me;
    }
}

static SplineSet *ttfbuildcontours(int path_cnt,uint16 *endpt, char *flags,
	BasePoint *pts, int is_order2) {
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
		sp->ttfindex = i;
		if ( last_off && cur->last!=NULL )
		    FigureControls(cur->last,sp,&pts[i-1],is_order2);
		last_off = false;
	    } else if ( last_off ) {
		/* two off curve points get a third on curve point created */
		/* half-way between them. Now isn't that special */
		sp = chunkalloc(sizeof(SplinePoint));
		sp->me.x = (pts[i].x+pts[i-1].x)/2;
		sp->me.y = (pts[i].y+pts[i-1].y)/2;
		sp->nextcp = sp->prevcp = sp->me;
		sp->nonextcp = true;
		sp->ttfindex = 0xffff;
		if ( last_off && cur->last!=NULL )
		    FigureControls(cur->last,sp,&pts[i-1],is_order2);
		/* last_off continues to be true */
	    } else {
		last_off = true;
		sp = NULL;
	    }
	    if ( sp!=NULL ) {
		if ( cur->first==NULL )
		    cur->first = sp;
		else
		    SplineMake(cur->last,sp,is_order2);
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
	    sp->ttfindex = i-1;
	    cur->first = cur->last = sp;
	} else if ( !(flags[start]&_On_Curve) && !(flags[i-1]&_On_Curve) ) {
	    sp = chunkalloc(sizeof(SplinePoint));
	    sp->me.x = (pts[start].x+pts[i-1].x)/2;
	    sp->me.y = (pts[start].y+pts[i-1].y)/2;
	    sp->nextcp = sp->prevcp = sp->me;
	    sp->nonextcp = true;
	    sp->ttfindex = 0xffff;
	    FigureControls(cur->last,sp,&pts[i-1],is_order2);
	    SplineMake(cur->last,sp,is_order2);
	    cur->last = sp;
	    FigureControls(sp,cur->first,&pts[start],is_order2);
	} else if ( !(flags[i-1]&_On_Curve))
	    FigureControls(cur->last,cur->first,&pts[i-1],is_order2);
	else if ( !(flags[start]&_On_Curve) )
	    FigureControls(cur->last,cur->first,&pts[start],is_order2);
	if ( cur->last!=cur->first ) {
	    SplineMake(cur->last,cur->first,is_order2);
	    cur->last = cur->first;
	}
    }
return( head );
}

static void readttfsimpleglyph(FILE *ttf,struct ttfinfo *info,SplineChar *sc, int path_cnt) {
    uint16 *endpt = galloc((path_cnt+1)*sizeof(uint16));
    uint8 *instructions;
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

    sc->splines = ttfbuildcontours(path_cnt,endpt,flags,pts,info->to_order2);
    if ( info->to_order2 && len!=0 ) {
	sc->ttf_instrs_len = len;
	sc->ttf_instrs = instructions;
    } else
	free(instructions);
    SCCatagorizePoints(sc);
    free(endpt);
    free(flags);
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
	cur->transform[4] = arg1;
	cur->transform[5] = arg2;
	if ( flags & _ARGS_ARE_XY ) {
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
	    /* Ah. It turns out that even Apple does not do what Apple's docs */
	    /*  claim it does. I think I've worked it out (see below), but... */
	    /*  Bleah! */
	} else {
	    /* Somehow we can get offsets by looking at the points in the */
	    /*  points so far generated and comparing them to the points in */
	    /*  the current componant */
	    /* How exactly is not described on any of the Apple, MS, Adobe */
	    /* freetype looks up arg1 in the set of points we've got so far */
	    /*  looks up arg2 in the new component (before renumbering) */
	    /*  offset.x = arg1.x - arg2.x; offset.y = arg1.y - arg2.y; */
	    /* This fixup needs to be done later though (after all glyphs */
	    /*  have been loaded) */
	    cur->point_match = true;
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
	if ( flags & _ARGS_ARE_XY ) {	/* Only muck with these guys if they are real offsets and not point matching */
#ifdef __Mac
	/* On mac assume scaled offsets unless told unscaled explicitly */
	if ( !(flags&_UNSCALED_OFFSETS) &&
#else
	/* everywhere else assume unscaled offsets unless told scaled explicitly */
	if ( (flags & _SCALED_OFFSETS) &&
#endif
		(flags & _ARGS_ARE_XY) && (flags&(_SCALE|_XY_SCALE|_MATRIX))) {
	    /*static int asked = 0;*/
	    /* This is not what Apple documents on their website. But it is */
	    /*  what appears to match the behavior of their rasterizer */
	    cur->transform[4] *= sqrt(cur->transform[0]*cur->transform[0]+
		    cur->transform[1]*cur->transform[1]);
	    cur->transform[5] *= sqrt(cur->transform[2]*cur->transform[2]+
		    cur->transform[3]*cur->transform[3]);
#if 0
	    /* Apple's Chicago is an example */
	    if ( info->fontname!=NULL && strcmp(info->fontname,"CompositeMac")!=0 && !asked ) {
		/* Not interested in the test font I generated myself */
		asked = true;
		fprintf( stderr, "Neat! You've got a font that actually uses Apple's scaled composite offsets.\n" );
		fprintf( stderr, "  I've never seen one, could you send me a copy of %s?\n", info->fontname );
		fprintf( stderr, "  Thanks.  gww@silcom.com\n" );
	    }
#endif
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
    if ( (flags & _INSTR ) && info->to_order2 ) {
	sc->ttf_instrs_len = getushort(ttf);
	if ( sc->ttf_instrs_len != 0 ) {
	    uint8 *instructions = galloc(sc->ttf_instrs_len);
	    int i;
	    for ( i=0; i<sc->ttf_instrs_len; ++i )
		instructions[i] = getc(ttf);
	    sc->ttf_instrs = instructions;
	}
    }
    /* I'm ignoring many things that I don't understand here */
    /* USE_MY_METRICS, what happens if ARGS AREN'T XY */
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

    if ( end>info->glyph_length ) {
	if ( !info->complainedbeyondglyfend )
	    fprintf(stderr, "Bad glyph (%d), its definition extends beyond the end of the glyf table\n", enc );
	info->complainedbeyondglyfend = true;
	SplineCharFree(sc);
return( NULL );
    }
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
    if ( ftell(ttf)>info->glyph_start+end )
	fprintf(stderr, "Bad glyph (%d), its definition extends beyond the space allowed for it\n", enc );
    sc->enc = 0;
return( sc );
}

static void readttfencodings(FILE *ttf,struct ttfinfo *info, int justinuse);
static void readttfgsubUsed(FILE *ttf,struct ttfinfo *info);

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
	if ( info->gsub_start!=0 )		/* Some glyphs may appear in substitutions and not in the encoding... */
	    readttfgsubUsed(ttf,info);
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
    for ( i=0; i<info->glyph_cnt ; ++i )
	if ( info->chars[i]!=NULL )
	    info->chars[i]->orig_pos = i;
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
    free(dict);
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
	if ( sp==0 && oval!=6 && oval!=7 && oval!=8 && oval!=9 && oval !=(12<<8)+12 && oval !=(12<<8)+13)
	    fprintf( stderr, "No argument to operator %d in private dict\n", oval );
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
    SplineFont *sf = SplineFontEmpty();
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
    sf->uniqueid = subdict->uniqueid;

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
    if ( dict->copyright!=-1 || dict->notice!=-1 )
	free( info->copyright );
    if ( dict->copyright!=-1 )
	info->copyright = copy(getsid(dict->copyright,strings));
    else if ( dict->notice!=-1 )
	info->copyright = copy(getsid(dict->notice,strings));
    if ( dict->familyname!=-1 ) {
	free(info->familyname);
	info->familyname = copy(getsid(dict->familyname,strings));
    }
    if ( dict->fullname!=-1 ) {
	free(info->fullname);
	info->fullname = copy(getsid(dict->fullname,strings));
    }
    if ( dict->weight!=-1 ) {
	free(info->weight);
	info->weight = copy(getsid(dict->weight,strings));
    }
    if ( dict->version!=-1 ) {
	free(info->version);
	info->version = copy(getsid(dict->version,strings));
    }
    if ( dict->fontname!=NULL ) {
	free(info->fontname);
	info->fontname = copy(dict->fontname);
    }
    info->italicAngle = dict->italicangle;
    info->upos = dict->underlinepos;
    info->uwidth = dict->underlinewidth;
    info->xuid = intarray2str(dict->xuid,sizeof(dict->xuid)/sizeof(dict->xuid[0]));
    info->uniqueid = dict->uniqueid;

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

    if ( info->to_order2 ) {
	for ( i=0; i<info->glyph_cnt; ++i )
	    SCConvertToOrder2(info->chars[i]);
    }

    for ( i=0; fontnames[i]!=NULL && i<1; ++i ) {
	free(fontnames[i]);
	TopDictFree(dicts[i]);
    }
    free(fontnames); free(dicts);
    if ( strings!=NULL ) {
	for ( i=0; strings[i]!=NULL; ++i )
	    free(strings[i]);
	free(strings);
    }
    for ( i=0; i<gsubs.cnt; ++i )
	free(gsubs.values[i]);
    free(gsubs.values); free(gsubs.lens);

    for ( i=0; i<info->glyph_cnt ; ++i )
	if ( info->chars[i]!=NULL )
	    info->chars[i]->orig_pos = i;

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
	if ( enc>0x8100 )
	    enc = unicode_from_big5hkscs[enc-0x8100];
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
    uint8 *used;
    int badencwarned=false;

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
	    info->platform = platform; info->specific = specific;
	} else if ( (enc!=em_unicode4 || (!prefer_cjk_encodings ||
		(enc!=em_sjis && enc!=em_wansung && enc!=em_big5 &&
		 enc!=em_big5hkscs && enc!=em_johab))) &&
		(( platform==3 && specific==1 ) || /* MS Unicode */
/* Well I should only deal with apple unicode specific==0 (default) and 3 (U2.0 semantics) */
/*  but apple ships dfonts with specific==1 (Unicode 1.1 semantics) */
/*  which is stupid of them */
		( platform==0 /*&& (specific==0 || specific==3)*/ && enc!=em_symbol ))) {	/* Apple Unicode */
	    enc = em_unicode;
	    encoff = offset;
	    info->platform = platform; info->specific = specific;
	    mod = 0;
	/* choose ms symbol over apple unicode. If there's an ms uncode it will */
	/*  come after ms symbol in the list and we'll get that */
	} else if ( platform==3 && specific==0 && (enc==em_none||enc==-2||enc==em_mac||enc==em_unicode) ) {
	    /* Only select symbol if we don't have something better */
	    enc = em_symbol;
	    encoff = offset;
	    info->platform = platform; info->specific = specific;
	    /* Now I had assumed this would be a 1 byte encoding, but it turns*/
	    /*  out to map into the unicode private use area at U+f000-U+F0FF */
	    /*  so it's a 2 byte enc */
/* Mac platform specific encodings are script numbers. 0=>roman, 1=>jap, 2=>big5, 3=>korean, 4=>arab, 5=>hebrew, 6=>greek, 7=>cyrillic, ... 25=>simplified chinese */
	} else if ( platform==1 && specific==0 && (enc==em_none||enc==-2) ) {
	    info->platform = platform; info->specific = specific;
	    enc = em_mac;
	    encoff = offset;
	    trans = unicode_from_mac;
	} else if ( platform==1 && (specific==2 ||specific==1) &&
		(prefer_cjk_encodings || enc!=em_unicode) ) {
	    /* I've seen an example of a big5 encoding so I know this is right*/
	    /*  Japanese appears to be sjis */
	    enc = specific==1?em_sjis:specific==2?em_big5hkscs:specific==3?em_wansung:em_gb2312;
	    mod = specific==1?2:specific==2?4:specific==3?5:3;		/* convert to ms specific */
	    info->platform = platform; info->specific = specific;
	    encoff = offset;
	} else if ( platform==3 && (specific==2 || specific==4 || specific==5 || specific==6 ) &&
		(prefer_cjk_encodings || enc!=em_unicode) ) {
	    /* Old ms docs say that specific==3 => big 5, new docs say specific==4 => big5 */
	    /*  Ain't that jus' great? */
	    enc = specific==2? em_sjis : specific==5 ? em_wansung : specific==4? em_big5hkscs : em_johab;
	    info->platform = platform; info->specific = specific;
	    mod = specific;
	    encoff = offset;
	} else if ( enc==em_none ) {
	    enc = -2;
	    mod = -1;
	    encoff = offset;
	    info->platform = platform; info->specific = specific;
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
	    used = gcalloc(65536,sizeof(uint8));
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
			if ( justinuse && (uint16) (j+delta[i])<info->glyph_cnt )
			    info->inuse[(uint16) (j+delta[i])] = true;
			else if ( (uint16) (j+delta[i])>=info->glyph_cnt || info->chars[(uint16) (j+delta[i])]==NULL )
			    fprintf( stderr, "Attempt to encode missing glyph %d to %d (0x%x)\n",
				    (uint16) (j+delta[i]), modenc(j,mod), modenc(j,mod));
			else if ( info->chars[(uint16) (j+delta[i])]->unicodeenc==-1 ) {
			    int uenc = umodenc(j,mod);
			    if ( uenc!=-1 && used[uenc] ) {
				if ( !badencwarned ) {
				    fprintf( stderr, "Multiple glyphs map to the same unicode encoding U+%04X, only one will be used\n", uenc );
			            badencwarned = true;
				}
			    } else {
				if ( uenc!=-1 ) used[uenc] = true;
				info->chars[(uint16) (j+delta[i])]->unicodeenc = uenc;
				info->chars[(uint16) (j+delta[i])]->enc = modenc(j,mod);
			    }
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
			        /*  program says it is treated as 0 */
				fprintf( stderr, "Attempt to encode missing glyph %d to %d (0x%x)\n",
					index, modenc(j,mod), modenc(j,mod));
			    else if ( justinuse )
				info->inuse[index] = 1;
			    else if ( info->chars[index]==NULL )
				fprintf( stderr, "Attempt to encode missing glyph %d to %d (0x%x)\n",
					index, modenc(j,mod), modenc(j,mod));
			    else if ( info->chars[index]->unicodeenc==-1 ) {
				int uenc = umodenc(j,mod);
				if ( uenc!=-1 && used[uenc] ) {
				    if ( !badencwarned ) {
					fprintf( stderr, "Multiple glyphs map to the same unicode encoding U+%04X, only one will be used\n", uenc );
					badencwarned = true;
				    }
				} else {
				    if ( uenc!=-1 ) used[uenc] = true;
				    info->chars[index]->unicodeenc = uenc;
				    info->chars[index]->enc = modenc(j,mod);
				}
			    } else
				info->dups = makedup(info->chars[index],umodenc(j,mod),modenc(j,mod),info->dups);
			}
		    }
		} else
		    fprintf( stderr, "Use of a range offset of 0xffff to mean a missing glyph in cmap table\n" );
	    }
	    free(glyphs);
	    free(rangeOffset);
	    free(delta);
	    free(startchars);
	    free(endchars);
	    free(used);
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
	    int max_sub_head_key = 0, cnt, max_pos= -1;
	    struct subhead *subheads;
	    
	    for ( i=0; i<256; ++i ) {
		table[i] = getushort(ttf)/8;	/* Sub-header keys */
		if ( table[i]>max_sub_head_key ) {
		    max_sub_head_key = table[i];	/* The entry is a byte pointer, I want a pointer in units of struct subheader */
		    max_pos = i;
		}
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
		    /* Special case, single byte encoding entry, look it up in */
		    /*  subhead */
		    /* In the one example I've got of this encoding (wcl-02.ttf) the chars */
		    /* 0xfd, 0xfe, 0xff are said to exist but there is no mapping */
		    /* for them. */
		    if ( i>=max_pos )
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
		    /*if ( last==-1 ) last = i;*/
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
    i = UniFromName(name);
    if ( i==-1 && strlen(name)==4 ) {
	/* MS says use this kind of name, Adobe says use the one above */
	char *end;
	i = strtol(name,&end,16);
	if ( i>=0 && i<=0xffff && *end=='\0' )
return( i );
    }
return( i );
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
    /* unicoderange[] */ getlong(ttf);
    /* unicoderange[] */ getlong(ttf);
    /* unicoderange[] */ getlong(ttf);
    /* unicoderange[] */ getlong(ttf);
    /* vendor */ getlong(ttf);
    /* fsselection */ getushort(ttf);
    /* firstchar */ getushort(ttf);
    /* lastchar */ getushort(ttf);
    info->pfminfo.os2_typoascent = getushort(ttf);
    info->pfminfo.os2_typodescent = (short) getushort(ttf);
    if ( info->pfminfo.linegap==0 ) {
	info->pfminfo.linegap = getushort(ttf);
    } else
	/* typographic linegap = */ getushort(ttf);
    info->pfminfo.os2_winascent = getushort(ttf);
    info->pfminfo.os2_windescent = getushort(ttf);
    info->pfminfo.pfmset = true;
}

static void GuessNamesFromGSUB(FILE *ttf,struct ttfinfo *info);

static void readttfpostnames(FILE *ttf,struct ttfinfo *info) {
    int i,j;
    int format, len, gc, gcbig, val;
    const char *name;
    char buffer[30];
    uint16 *indexes;
    extern const char *ttfstandardnames[];
    int notdefwarned = false;

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

    /* where no names are given, but we've got a unicode encoding use */
    /*  that to guess at them */
    for ( i=0; i<info->glyph_cnt; ++i ) {
	/* info->chars[i] can be null in some TTC files */
	if ( info->chars[i]==NULL || info->chars[i]->name!=NULL )
    continue;
	if ( i==0 )
	    name = ".notdef";
	else if ( info->chars[i]->unicodeenc==-1 ) {
	    /* Do this later */;
	    name = NULL;
	} else if ( info->chars[i]->unicodeenc<psunicodenames_cnt &&
		psunicodenames[info->chars[i]->unicodeenc]!=NULL )
	    name = psunicodenames[info->chars[i]->unicodeenc];
	else {
	    if ( info->chars[i]->unicodeenc<0x10000 )
		sprintf( buffer, "uni%04X", info->chars[i]->unicodeenc );
	    else
		sprintf( buffer, "u%04X", info->chars[i]->unicodeenc );
	    name = buffer;
	}
	GProgressNext();
	info->chars[i]->name = copy(name);
    }

    /* If we have a GSUB table we can give some unencoded glyphs name */
    /*  for example if we have a vrt2 substitution of A to <unencoded> */
    /*  we could name the unencoded "A.vrt2" (though in this case we might */
    /*  try A.vert instead */ /* Werner suggested this */
    /* We could try this from morx too, except that apple features don't */
    /*  meaningful ids. That is A.15,3 isn't very readable */
    for ( i=info->glyph_cnt-1; i>=0 ; --i )
	if ( info->chars[i]!=NULL && info->chars[i]->name==NULL )
    break;
    if ( i>=0 && info->gsub_start!=0 )
	GuessNamesFromGSUB(ttf,info);

    for ( i=0; i<info->glyph_cnt; ++i ) {
	/* info->chars[i] can be null in some TTC files */
	if ( info->chars[i]==NULL )
    continue;
	if ( i!=0 && info->chars[i]->name!=NULL &&
		strcmp(info->chars[i]->name,".notdef")==0 ) {
	    /* for some reason MS puts out fonts where several characters */
	    /* are called .notdef (and only one is a real notdef). So if we */
	    /* find a glyph other than 0 called ".notdef" then pretend it had */
	    /* no name */
	    if ( !notdefwarned ) {
		notdefwarned = true;
		fprintf( stderr, "Glyph %d is called \".notdef\", a singularly inept choice of name (only glyph 0\n may be called .notdef)\nPfaEdit will rename it.\n", i );
	    }
	    free(info->chars[i]->name);
	    info->chars[i]->name = NULL;
	}
	if ( info->chars[i]->name!=NULL )
    continue;
	if ( info->ordering!=NULL )
	    sprintf(buffer, "%.20s-%d", info->ordering, i );
	else if ( info->chars[i]->enc!=0 )
	    sprintf(buffer, "nounicode-%d-%d-%x", info->platform, info->specific,
		    info->chars[i]->enc );
	else
	    sprintf( buffer, "glyph%d", i );
	info->chars[i]->name = copy(buffer);
	GProgressNext();
    }
    GProgressNextStage();
}

static int SLIFromInfo(struct ttfinfo *info,SplineChar *sc,uint32 lang) {
    uint32 script = SCScriptFromUnicode(sc);
    int j;

    if ( script==0 ) script = CHR('l','a','t','n');
    if ( info->script_lang==NULL ) {
	info->script_lang = galloc(2*sizeof(struct script_record *));
	j = 0;
    } else {
	for ( j=0; info->script_lang[j]!=NULL; ++j ) {
	    if ( info->script_lang[j][0].script==script &&
		    info->script_lang[j][1].script == 0 &&
		    info->script_lang[j][0].langs[0] == lang &&
		    info->script_lang[j][0].langs[1] == 0 )
return( j );
	}
    }
    info->script_lang = grealloc(info->script_lang,(j+2)*sizeof(struct script_record *));
    info->script_lang[j+1] = NULL;
    info->script_lang[j]= gcalloc(2,sizeof(struct script_record));
    info->script_lang[j][0].script = script;
    info->script_lang[j][0].langs = gcalloc(2,sizeof(uint32));
    info->script_lang[j][0].langs[0] = lang;
return( j );
}

static uint16 *getAppleClassTable(FILE *ttf, int classdef_offset, int cnt, int sub, int div) {
    uint16 *class = gcalloc(cnt,sizeof(uint16));
    int first, i, n;
    /* Apple stores its class tables as containing offsets. I find it hard to */
    /*  think that way and convert them to indeces (by subtracting off a base */
    /*  offset and dividing by the item's size) before doing anything else */

    fseek(ttf,classdef_offset,SEEK_SET);
    first = getushort(ttf);
    n = getushort(ttf);
    if ( first+n+1>=cnt )
	fprintf( stderr, "Bad Apple Kern Class\n" );
    for ( i=0; i<=n && i+first<cnt; ++i )
	class[first+i] = (getushort(ttf)-sub)/div;
return( class );
}

static char **ClassToNames(struct ttfinfo *info,int class_cnt,uint16 *class,int glyph_cnt) {
    char **ret = galloc(class_cnt*sizeof(char *));
    int *lens = gcalloc(class_cnt,sizeof(int));
    int i;

    ret[0] = NULL;
    for ( i=0 ; i<glyph_cnt; ++i ) if ( class[i]!=0 && info->chars[i]!=NULL && class[i]<class_cnt )
	lens[class[i]] += strlen(info->chars[i]->name)+1;
    for ( i=1; i<class_cnt ; ++i )
	ret[i] = galloc(lens[i]+1);
    memset(lens,0,class_cnt*sizeof(int));
    for ( i=0 ; i<glyph_cnt; ++i ) if ( class[i]!=0 && info->chars[i]!=NULL ) {
	if ( class[i]<class_cnt ) {
	    strcpy(ret[class[i]]+lens[class[i]], info->chars[i]->name );
	    lens[class[i]] += strlen(info->chars[i]->name)+1;
	    ret[class[i]][lens[class[i]]-1] = ' ';
	} else
	    fprintf( stderr, "Class index out of range in kerning class\n" );
    }
    for ( i=1; i<class_cnt ; ++i )
	if ( lens[i]==0 )
	    ret[i][0] = '\0';
	else
	    ret[i][lens[i]-1] = '\0';
    free(lens);
return( ret );
}

/* Apple's docs imply that kerning info is always provided left to right, even*/
/*  for right to left scripts. If that be so then we need code in here to reverse */
/*  the order of the characters for right to left since pfaedit's convention */
/*  is to follow writing order rather than to go left to right */
static void readttfkerns(FILE *ttf,struct ttfinfo *info) {
    int tabcnt, len, coverage,i,j, npairs, version, format, flags_good;
    int left, right, offset, array, rowWidth;
    int header_size;
    KernPair *kp;
    KernClass *kc;
    uint32 begin_table;
    uint16 *class1, *class2;
    int isv;

    fseek(ttf,info->kern_start,SEEK_SET);
    version = getushort(ttf);
    tabcnt = getushort(ttf);
    if ( version!=0 ) {
	fseek(ttf,info->kern_start,SEEK_SET);
	version = getlong(ttf);
	tabcnt = getlong(ttf);
    }
    for ( i=0; i<tabcnt; ++i ) {
	begin_table = ftell(ttf);
	if ( version==0 ) {
	    /* version = */ getushort(ttf);
	    len = getushort(ttf);
	    coverage = getushort(ttf);
	    format = coverage>>8;
	    flags_good = ((coverage&7)<=1);
	    isv = !(coverage&1);
	    header_size = 6;
	} else {
	    len = getlong(ttf);
	    coverage = getushort(ttf);
	    /* Apple has reordered the bits */
	    format = (coverage&0xff);
	    flags_good = ((coverage&0xff00)==0 || (coverage&0xff00)==0x8000);
	    isv = coverage&0x8000? 1 : 0;
	    /* tupleIndex = */ getushort(ttf);
	    header_size = 8;
	}
	if ( flags_good && format==0 ) {
	    /* format 0, horizontal kerning data (as pairs) not perpendicular */
	    npairs = getushort(ttf);
	    /* searchRange = */ getushort(ttf);
	    /* entrySelector = */ getushort(ttf);
	    /* rangeShift = */ getushort(ttf);
	    for ( j=0; j<npairs; ++j ) {
		left = getushort(ttf);
		right = getushort(ttf);
		offset = (short) getushort(ttf);
		if ( left<info->glyph_cnt && right<info->glyph_cnt ) {
		    kp = chunkalloc(sizeof(KernPair));
		    kp->sc = info->chars[right];
		    kp->off = offset;
		    kp->sli = SLIFromInfo(info,info->chars[left],DEFAULT_LANG);
		    if ( isv ) {
			kp->next = info->chars[left]->vkerns;
			info->chars[left]->vkerns = kp;
		    } else {
			kp->next = info->chars[left]->kerns;
			info->chars[left]->kerns = kp;
		    }
		} else
		    fprintf( stderr, "Bad kern pair glyphs %d & %d must be less than %d\n",
			    left, right, info->glyph_cnt );
	    }
	} else if ( flags_good && format==1 ) {
	    /* format 1 is an apple state machine which can handle weird cases */
	    /*  OpenType's spec doesn't document this */
	    /*  I shan't support it */
	    fseek(ttf,len-header_size,SEEK_CUR);
	    fprintf( stderr, "This font has a format 1 kerning table (a state machine).\nPfaEdit doesn't parse these\nSorry.\n" );
	} else if ( flags_good && format==2 ) {
	    /* format 2, horizontal kerning data (as classes) not perpendicular */
	    /*  OpenType's spec documents this, but says windows won't support it */
	    /*  OpenType's spec also contradicts Apple's as to the data stored */
	    /*  OTF says class indeces are stored, Apple says byte offsets into array */
	    /*  Apple says offsets are stored in uint8, otf says indeces are in uint16 */
	    /* Bleah!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
	    /*  Ah. Apple's docs are incorrect. the values stored are uint16 offsets */
	    rowWidth = getushort(ttf);
	    left = getushort(ttf);
	    right = getushort(ttf);
	    array = getushort(ttf);
	    if ( isv ) {
		if ( info->khead==NULL )
		    info->vkhead = kc = chunkalloc(sizeof(KernClass));
		else
		    kc = info->vklast->next = chunkalloc(sizeof(KernClass));
		info->vklast = kc;
	    } else {
		if ( info->khead==NULL )
		    info->khead = kc = chunkalloc(sizeof(KernClass));
		else
		    kc = info->klast->next = chunkalloc(sizeof(KernClass));
		info->klast = kc;
	    }
	    kc->second_cnt = rowWidth/sizeof(uint16);
	    class1 = getAppleClassTable(ttf, begin_table+left, info->glyph_cnt, array, rowWidth );
	    class2 = getAppleClassTable(ttf, begin_table+right, info->glyph_cnt, 0, sizeof(uint16) );
	    for ( i=0; i<info->glyph_cnt; ++i ) {
		if ( class1[i]!=0 ) {
		    kc->sli = SLIFromInfo(info,info->chars[i],DEFAULT_LANG);
	    break;
		}
	    }
	    for ( i=0; i<info->glyph_cnt; ++i )
		if ( class1[i]>kc->first_cnt )
		    kc->first_cnt = class1[i];
	    ++ kc->first_cnt;
	    kc->offsets = galloc(kc->first_cnt*kc->second_cnt*sizeof(int16));
	    kc->firsts = ClassToNames(info,kc->first_cnt,class1,info->glyph_cnt);
	    kc->seconds = ClassToNames(info,kc->second_cnt,class2,info->glyph_cnt);
	    fseek(ttf,begin_table+array,SEEK_SET);
	    for ( i=0; i<kc->first_cnt*kc->second_cnt; ++i )
		kc->offsets[i] = getushort(ttf);
	    free(class1); free(class2);
	    fseek(ttf,begin_table+len,SEEK_SET);
	} else if ( flags_good && format==3 ) {
	    /* format 3, horizontal kerning data (as classes limited to 256 entries) not perpendicular */
	    /*  OpenType's spec doesn't document this */
	    fseek(ttf,len-header_size,SEEK_CUR);
    fprintf( stderr, "This font has a format 3 kerning table. I've never seen that and don't know\nhow to parse it. Could you send a copy of %s to gww@silcom.com?\nThanks!\n",
	info->fontname );
	} else {
	    fseek(ttf,len-header_size,SEEK_CUR);
	}
    }
}

#define MAX_LANG 		20		/* Don't support more than 20 languages per feature (only remember first 20) */
struct scriptlist {
    uint32 script;
    uint32 langs[MAX_LANG];
    int lang_cnt;
    struct scriptlist *next;
};

struct feature {
    uint32 offset;
    uint32 tag;
    struct scriptlist *sl, *reqsl;
    /*int script_lang_index;*/
    int lcnt;
    uint16 *lookups;
};
struct lookup {
    uint32 tag;
    struct scriptlist *sl;
    int script_lang_index;
    uint16 flags;
    uint16 lookup;
    struct lookup *alttags;
};

static uint16 *getCoverageTable(FILE *ttf, int coverage_offset, struct ttfinfo *info) {
    int format, cnt, i,j;
    uint16 *glyphs=NULL;
    int start, end, ind, max;

    fseek(ttf,coverage_offset,SEEK_SET);
    format = getushort(ttf);
    if ( format==1 ) {
	cnt = getushort(ttf);
	glyphs = galloc((cnt+1)*sizeof(uint16));
	for ( i=0; i<cnt; ++i ) {
	    glyphs[i] = getushort(ttf);
	    if ( glyphs[i]>=info->glyph_cnt ) {
		fprintf( stderr, "Bad coverage table. Glyph %d out of range [0,%d)\n", glyphs[i], info->glyph_cnt );
		glyphs[i] = 0;
	    }
	}
    } else if ( format==2 ) {
	glyphs = gcalloc((max=256),sizeof(uint16));
	cnt = getushort(ttf);
	for ( i=0; i<cnt; ++i ) {
	    start = getushort(ttf);
	    end = getushort(ttf);
	    ind = getushort(ttf);
	    if ( start>end || end>=info->glyph_cnt ) {
		fprintf( stderr, "Bad coverage table. Glyph range %d-%d out of range [0,%d)\n", start, end, info->glyph_cnt );
		start = end = 0;
	    }
	    if ( ind+end-start+2 >= max ) {
		int oldmax = max;
		max = ind+end-start+2;
		glyphs = grealloc(glyphs,max*sizeof(uint16));
		memset(glyphs+oldmax,0,(max-oldmax)*sizeof(uint16));
	    }
	    for ( j=start; j<=end; ++j ) {
		glyphs[j-start+ind] = j;
		if ( j>=info->glyph_cnt )
		    glyphs[j-start+ind] = 0;
	    }
	}
	if ( cnt!=0 )
	    cnt = ind+end-start+1;
    } else {
	fprintf( stderr, "Bad format for coverage table %d\n", format );
return( NULL );
    }
    glyphs[cnt] = 0xffff;
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
	if ( start+(int) glyphcnt>cnt ) {
	    fprintf( stderr, "Bad class def table. start=%d cnt=%d, max glyph=%d\n", start, glyphcnt, cnt );
	    glyphcnt = cnt-start;
	}
	for ( i=0; i<glyphcnt; ++i )
	    glist[start+i] = getushort(ttf);
    } else if ( format==2 ) {
	rangecnt = getushort(ttf);
	for ( i=0; i<rangecnt; ++i ) {
	    start = getushort(ttf);
	    end = getushort(ttf);
	    if ( start>end || end>=cnt )
		fprintf( stderr, "Bad class def table. Glyph range %d-%d out of range [0,%d)\n", start, end, cnt );
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

static void addPairPos(struct ttfinfo *info, int glyph1, int glyph2,
	struct lookup *lookup,struct valuerecord *vr1,struct valuerecord *vr2) {
    
    if ( glyph1<info->glyph_cnt && glyph2<info->glyph_cnt ) {
	PST *pos = chunkalloc(sizeof(PST));
	pos->type = pst_pair;
	pos->tag = lookup->tag;
	pos->script_lang_index = lookup->script_lang_index;
	pos->flags = lookup->flags;
	pos->next = info->chars[glyph1]->possub;
	info->chars[glyph1]->possub = pos;
	pos->u.pair.vr = chunkalloc(sizeof(struct vr [2]));
	pos->u.pair.paired = copy(info->chars[glyph2]->name);
	pos->u.pair.vr[0].xoff = vr1->xplacement;
	pos->u.pair.vr[0].yoff = vr1->yplacement;
	pos->u.pair.vr[0].h_adv_off = vr1->xadvance;
	pos->u.pair.vr[0].v_adv_off = vr1->yadvance;
	pos->u.pair.vr[1].xoff = vr2->xplacement;
	pos->u.pair.vr[1].yoff = vr2->yplacement;
	pos->u.pair.vr[1].h_adv_off = vr2->xadvance;
	pos->u.pair.vr[1].v_adv_off = vr2->yadvance;
    } else
	fprintf( stderr, "Bad pair position: glyphs %d & %d should have been < %d\n",
		glyph1, glyph2, info->glyph_cnt );
}

static int addKernPair(struct ttfinfo *info, int glyph1, int glyph2,
	int16 offset, uint16 sli, uint16 flags,int isv) {
    KernPair *kp;
    if ( glyph1<info->glyph_cnt && glyph2<info->glyph_cnt ) {
	for ( kp=isv ? info->chars[glyph1]->vkerns : info->chars[glyph1]->kerns;
		kp!=NULL; kp=kp->next ) {
	    if ( kp->sc == info->chars[glyph2] )
	break;
	}
	if ( kp==NULL ) {
	    kp = chunkalloc(sizeof(KernPair));
	    kp->sc = info->chars[glyph2];
	    kp->off = offset;
	    kp->sli = sli;
	    kp->flags = flags;
	    if ( isv ) {
		kp->next = info->chars[glyph1]->vkerns;
		info->chars[glyph1]->vkerns = kp;
	    } else {
		kp->next = info->chars[glyph1]->kerns;
		info->chars[glyph1]->kerns = kp;
	    }
	} else if ( kp->sli!=sli || kp->flags!=flags )
return( true );
    } else
	fprintf( stderr, "Bad kern pair: glyphs %d & %d should have been < %d\n",
		glyph1, glyph2, info->glyph_cnt );
return( false );
}

static void gposKernSubTable(FILE *ttf, int stoffset, struct ttfinfo *info, struct lookup *lookup,int isv) {
    int coverage, cnt, i, j, pair_cnt, vf1, vf2, glyph2;
    int cd1, cd2, c1_cnt, c2_cnt;
    uint16 format;
    uint16 *ps_offsets;
    uint16 *glyphs, *class1, *class2;
    struct valuerecord vr1, vr2;
    long foffset;
    KernClass *kc;

    format=getushort(ttf);
    if ( format!=1 && format!=2 )	/* Unknown subtable format */
return;
    coverage = getushort(ttf);
    vf1 = getushort(ttf);
    vf2 = getushort(ttf);
    if ( isv==1 ) {
	if ( vf1&0xff77 )
	    isv = 2;
	if ( vf2&0xff77 )
	    isv = 2;
    } else if ( isv==0 ) {
	if ( vf1&0xffbb)	/* can't represent things that deal with y advance/placement nor with x placement as kerning */
	    isv = 2;
	if ( vf2&0xffaa )
	    isv = 2;
    }
    if ( format==1 ) {
	cnt = getushort(ttf);
	ps_offsets = galloc(cnt*sizeof(uint16));
	for ( i=0; i<cnt; ++i )
	    ps_offsets[i]=getushort(ttf);
	glyphs = getCoverageTable(ttf,stoffset+coverage,info);
	if ( glyphs==NULL )
return;
	for ( i=0; i<cnt; ++i ) if ( glyphs[i]<info->glyph_cnt ) {
	    fseek(ttf,stoffset+ps_offsets[i],SEEK_SET);
	    pair_cnt = getushort(ttf);
	    for ( j=0; j<pair_cnt; ++j ) {
		glyph2 = getushort(ttf);
		readvaluerecord(&vr1,vf1,ttf);
		readvaluerecord(&vr2,vf2,ttf);
		if ( isv==2 )
		    addPairPos(info, glyphs[i], glyph2,lookup,&vr1,&vr2);
		else if ( isv ) {
		    if ( addKernPair(info, glyphs[i], glyph2, vr1.yadvance,lookup->script_lang_index,lookup->flags,isv))
			addPairPos(info, glyphs[i], glyph2,lookup,&vr1,&vr2);
			/* If we've already got kern data for this pair of */
			/*  glyphs, then we can't make it be a true KernPair */
			/*  but we can save the info as a pst_pair */
		} else if ( lookup->flags&1 ) {	/* R2L */
		    if ( addKernPair(info, glyphs[i], glyph2, vr2.xadvance,lookup->script_lang_index,lookup->flags,isv))
			addPairPos(info, glyphs[i], glyph2,lookup,&vr1,&vr2);
		} else {
		    if ( addKernPair(info, glyphs[i], glyph2, vr1.xadvance,lookup->script_lang_index,lookup->flags,isv))
			addPairPos(info, glyphs[i], glyph2,lookup,&vr1,&vr2);
		}
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
	if ( isv!=2 ) {
	    if ( isv ) {
		if ( info->vkhead==NULL )
		    info->vkhead = kc = chunkalloc(sizeof(KernClass));
		else
		    kc = info->vklast->next = chunkalloc(sizeof(KernClass));
		info->vklast = kc;
	    } else {
		if ( info->khead==NULL )
		    info->khead = kc = chunkalloc(sizeof(KernClass));
		else
		    kc = info->klast->next = chunkalloc(sizeof(KernClass));
		info->klast = kc;
	    }
	    kc->first_cnt = c1_cnt; kc->second_cnt = c2_cnt;
	    kc->sli = lookup->script_lang_index;
	    kc->flags = lookup->flags;
	    kc->offsets = galloc(c1_cnt*c2_cnt*sizeof(int16));
	    kc->firsts = ClassToNames(info,c1_cnt,class1,info->glyph_cnt);
	    kc->seconds = ClassToNames(info,c2_cnt,class2,info->glyph_cnt);
	    for ( i=0; i<c1_cnt; ++i) {
		for ( j=0; j<c2_cnt; ++j) {
		    readvaluerecord(&vr1,vf1,ttf);
		    readvaluerecord(&vr2,vf2,ttf);
		    if ( isv )
			kc->offsets[i*c2_cnt+j] = vr1.yadvance;
		    else if ( lookup->flags&1 )	/* R2L */
			kc->offsets[i*c2_cnt+j] = vr2.xadvance;
		    else
			kc->offsets[i*c2_cnt+j] = vr1.xadvance;
		}
	    }
	} else {
	    int k,l;
	    for ( i=0; i<c1_cnt; ++i) {
		for ( j=0; j<c2_cnt; ++j) {
		    readvaluerecord(&vr1,vf1,ttf);
		    readvaluerecord(&vr2,vf2,ttf);
		    if ( vr1.xadvance!=0 || vr1.xplacement!=0 || vr1.yadvance!=0 || vr1.yplacement!=0 ||
			    vr2.xadvance!=0 || vr2.xplacement!=0 || vr2.yadvance!=0 || vr2.yplacement!=0 )
			for ( k=0; k<info->glyph_cnt; ++k )
			    if ( class1[k]==i )
				for ( l=0; l<info->glyph_cnt; ++l )
				    if ( class2[l]==j )
					addPairPos(info, k,l,lookup,&vr1,&vr2);
		}
	    }
	}
	free(class1); free(class2);
    }
}

static void gposCursiveSubTable(FILE *ttf, int stoffset, struct ttfinfo *info,struct lookup *lookup) {
    int coverage, cnt, format, i;
    struct ee_offsets { int entry, exit; } *offsets;
    uint16 *glyphs;
    AnchorClass *class;
    AnchorPoint *ap;
    SplineChar *sc;

    format=getushort(ttf);
    if ( format!=1 )	/* Unknown subtable format */
return;
    coverage = getushort(ttf);
    cnt = getushort(ttf);
    if ( cnt==0 )
return;
    offsets = galloc(cnt*sizeof(struct ee_offsets));
    for ( i=0; i<cnt; ++i ) {
	offsets[i].entry = getushort(ttf);
	offsets[i].exit  = getushort(ttf);
    }
    glyphs = getCoverageTable(ttf,stoffset+coverage,info);

    class = chunkalloc(sizeof(AnchorClass));
    class->name = uc_copy("Cursive");
    class->feature_tag = lookup->tag;
    class->script_lang_index = lookup->script_lang_index;
    class->type = act_curs;
    if ( info->ahead==NULL )
	info->ahead = class;
    else
	info->alast->next = class;
    info->alast = class;

    for ( i=0; i<cnt; ++i ) {
	sc = info->chars[glyphs[i]];
	if ( offsets[i].entry!=0 ) {
	    ap = chunkalloc(sizeof(AnchorPoint));
	    ap->anchor = class;
	    fseek(ttf,stoffset+offsets[i].entry,SEEK_SET);
	    /* All anchor types have the same initial 3 entries, and I only care */
	    /*  about two of them, so I can ignore all the complexities of the */
	    /*  format type */
	    /* format = */ getushort(ttf);
	    ap->me.x = (int16) getushort(ttf);
	    ap->me.y = (int16) getushort(ttf);
	    ap->type = at_centry;
	    ap->next = sc->anchor;
	    sc->anchor = ap;
	}
	if ( offsets[i].exit!=0 ) {
	    ap = chunkalloc(sizeof(AnchorPoint));
	    ap->anchor = class;
	    fseek(ttf,stoffset+offsets[i].exit,SEEK_SET);
	    /* format = */ getushort(ttf);
	    ap->me.x = (int16) getushort(ttf);
	    ap->me.y = (int16) getushort(ttf);
	    ap->type = at_cexit;
	    ap->next = sc->anchor;
	    sc->anchor = ap;
	}
    }
    free(offsets);
    free(glyphs);
}

static AnchorClass **MarkGlyphsProcessMarks(FILE *ttf,int markoffset,
	struct ttfinfo *info,struct lookup *lookup,uint16 *markglyphs,
	int classcnt,int lu_type) {
    AnchorClass **classes = gcalloc(classcnt,sizeof(AnchorClass *)), *ac;
    unichar_t ubuf[50];
    int i, cnt;
    struct mr { uint16 class, offset; } *at_offsets;
    AnchorPoint *ap;
    SplineChar *sc;

    for ( i=0; i<classcnt; ++i ) {
	u_snprintf(ubuf,sizeof(ubuf)/sizeof(ubuf[0]),GStringGetResource(_STR_UntitledAnchor_n,NULL),
		info->anchor_class_cnt+i );
	classes[i] = ac = chunkalloc(sizeof(AnchorClass));
	ac->name = u_copy(ubuf);
	ac->feature_tag = lookup->tag;
	ac->script_lang_index = lookup->script_lang_index;
	ac->flags = lookup->flags;
	ac->merge_with = info->anchor_merge_cnt+1;
	ac->type = lu_type==6 ? act_mkmk : act_mark;
	    /* I don't distinguish between mark to base and mark to lig */
	if ( info->ahead==NULL )
	    info->ahead = ac;
	else
	    info->alast->next = ac;
	info->alast = ac;
    }

    fseek(ttf,markoffset,SEEK_SET);
    cnt = getushort(ttf);
    at_offsets = galloc(cnt*sizeof(struct mr));
    for ( i=0; i<cnt; ++i ) {
	at_offsets[i].class = getushort(ttf);
	at_offsets[i].offset = getushort(ttf);
	if ( at_offsets[i].class>=classcnt ) {
	    at_offsets[i].class = 0;
	    fprintf( stderr, "Class out of bounds in GPOS mark sub-table\n" );
	}
    }
    for ( i=0; i<cnt; ++i ) {
	sc = info->chars[markglyphs[i]];
	if ( markglyphs[i]>=info->glyph_cnt || sc==NULL || at_offsets[i].offset==0 )
    continue;
	ap = chunkalloc(sizeof(AnchorPoint));
	ap->anchor = classes[at_offsets[i].class];
	fseek(ttf,markoffset+at_offsets[i].offset,SEEK_SET);
	/* All anchor types have the same initial 3 entries, and I only care */
	/*  about two of them, so I can ignore all the complexities of the */
	/*  format type */
	/* format = */ getushort(ttf);
	ap->me.x = (int16) getushort(ttf);
	ap->me.y = (int16) getushort(ttf);
	ap->type = at_mark;
	ap->next = sc->anchor;
	sc->anchor = ap;
    }
    free(at_offsets);
return( classes );
}

static void MarkGlyphsProcessBases(FILE *ttf,int baseoffset,
	struct ttfinfo *info,struct lookup *lookup,uint16 *baseglyphs,int classcnt,
	AnchorClass **classes,enum anchor_type at) {
    int basecnt,i, j, ibase;
    uint16 *offsets;
    SplineChar *sc;
    AnchorPoint *ap;

    fseek(ttf,baseoffset,SEEK_SET);
    basecnt = getushort(ttf);
    offsets = galloc(basecnt*classcnt*sizeof(uint16));
    for ( i=0; i<basecnt*classcnt; ++i )
	offsets[i] = getushort(ttf);
    for ( i=ibase=0; i<basecnt; ++i, ibase+= classcnt ) {
	sc = info->chars[baseglyphs[i]];
	if ( baseglyphs[i]>=info->glyph_cnt || sc==NULL )
    continue;
	for ( j=0; j<classcnt; ++j ) if ( offsets[ibase+j]!=0 ) {
	    fseek(ttf,baseoffset+offsets[ibase+j],SEEK_SET);
	    ap = chunkalloc(sizeof(AnchorPoint));
	    ap->anchor = classes[j];
	    /* All anchor types have the same initial 3 entries, and I only care */
	    /*  about two of them, so I can ignore all the complexities of the */
	    /*  format type */
	    /* format = */ getushort(ttf);
	    ap->me.x = (int16) getushort(ttf);
	    ap->me.y = (int16) getushort(ttf);
	    ap->type = at;
	    ap->next = sc->anchor;
	    sc->anchor = ap;
	}
    }
}

static void MarkGlyphsProcessLigs(FILE *ttf,int baseoffset,
	struct ttfinfo *info,struct lookup *lookup,uint16 *baseglyphs,int classcnt,
	AnchorClass **classes) {
    int basecnt,compcnt, i, j, k, kbase;
    uint16 *loffsets, *aoffsets;
    SplineChar *sc;
    AnchorPoint *ap;

    fseek(ttf,baseoffset,SEEK_SET);
    basecnt = getushort(ttf);
    loffsets = galloc(basecnt*sizeof(uint16));
    for ( i=0; i<basecnt; ++i )
	loffsets[i] = getushort(ttf);
    for ( i=0; i<basecnt; ++i ) {
	sc = info->chars[baseglyphs[i]];
	if ( baseglyphs[i]>=info->glyph_cnt || sc==NULL )
    continue;
	fseek(ttf,baseoffset+loffsets[i],SEEK_SET);
	compcnt = getushort(ttf);
	aoffsets = galloc(compcnt*classcnt*sizeof(uint16));
	for ( k=0; k<compcnt*classcnt; ++k )
	    aoffsets[k] = getushort(ttf);
	for ( k=kbase=0; k<compcnt; ++k, kbase+=classcnt ) {
	    for ( j=0; j<classcnt; ++j ) if ( aoffsets[kbase+j]!=0 ) {
		fseek(ttf,baseoffset+loffsets[i]+aoffsets[kbase+j],SEEK_SET);
		ap = chunkalloc(sizeof(AnchorPoint));
		ap->anchor = classes[j];
		/* All anchor types have the same initial 3 entries, and I only care */
		/*  about two of them, so I can ignore all the complexities of the */
		/*  format type */
		/* format = */ getushort(ttf);
		ap->me.x = (int16) getushort(ttf);
		ap->me.y = (int16) getushort(ttf);
		ap->type = at_baselig;
		ap->lig_index = k;
		ap->next = sc->anchor;
		sc->anchor = ap;
	    }
	}
    }
}

static void gposMarkSubTable(FILE *ttf, uint32 stoffset,
	struct ttfinfo *info, struct lookup *lookup,int lu_type) {
    int markcoverage, basecoverage, classcnt, markoffset, baseoffset;
    uint16 *markglyphs, *baseglyphs;
    AnchorClass **classes;

	/* The header for the three different mark tables is the same */
    /* Type = */ getushort(ttf);
    markcoverage = getushort(ttf);
    basecoverage = getushort(ttf);
    classcnt = getushort(ttf);
    markoffset = getushort(ttf);
    baseoffset = getushort(ttf);
    markglyphs = getCoverageTable(ttf,stoffset+markcoverage,info);
    baseglyphs = getCoverageTable(ttf,stoffset+basecoverage,info);
    if ( baseglyphs==NULL || markglyphs==NULL ) {
	free(baseglyphs); free(markglyphs);
return;
    }
	/* as is the (first) mark table */
    classes = MarkGlyphsProcessMarks(ttf,stoffset+markoffset,
	    info,lookup,markglyphs,classcnt,lu_type);
    switch ( lu_type ) {
      case 4:			/* Mark to Base */
      case 6:			/* Mark to Mark */
	  MarkGlyphsProcessBases(ttf,stoffset+baseoffset,
	    info,lookup,baseglyphs,classcnt,classes,
	    lu_type==4?at_basechar:at_basemark);
      break;
      case 5:			/* Mark to Ligature */
	  MarkGlyphsProcessLigs(ttf,stoffset+baseoffset,
	    info,lookup,baseglyphs,classcnt,classes);
      break;
    }
    info->anchor_class_cnt += classcnt;
    ++ info->anchor_merge_cnt;
    free(markglyphs); free(baseglyphs);
    free(classes);
}

static void gposSimplePos(FILE *ttf, int stoffset, struct ttfinfo *info, struct lookup *lookup) {
    int coverage, cnt, i, vf;
    uint16 format;
    uint16 *glyphs;
    struct valuerecord *vr=NULL, _vr, *which;

    format=getushort(ttf);
    if ( format!=1 && format!=2 )	/* Unknown subtable format */
return;
    coverage = getushort(ttf);
    vf = getushort(ttf);
    if ( (vf&0xf)==0 )	/* Not interested in things whose data lives in device tables */
return;
    if ( format==1 ) {
	memset(&_vr,0,sizeof(_vr));
	readvaluerecord(&_vr,vf,ttf);
    } else {
	cnt = getushort(ttf);
	vr = gcalloc(cnt,sizeof(struct valuerecord));
	for ( i=0; i<cnt; ++i )
	    readvaluerecord(&vr[i],vf,ttf);
    }
    glyphs = getCoverageTable(ttf,stoffset+coverage,info);
    if ( glyphs==NULL ) {
	free(vr);
return;
    }
    for ( i=0; glyphs[i]!=0xffff; ++i ) if ( glyphs[i]<info->glyph_cnt ) {
	PST *pos = chunkalloc(sizeof(PST));
	pos->type = pst_position;
	pos->tag = lookup->tag;
	pos->script_lang_index = lookup->script_lang_index;
	pos->flags = lookup->flags;
	pos->next = info->chars[glyphs[i]]->possub;
	info->chars[glyphs[i]]->possub = pos;
	which = format==1 ? &_vr : &vr[i];
	pos->u.pos.xoff = which->xplacement;
	pos->u.pos.yoff = which->yplacement;
	pos->u.pos.h_adv_off = which->xadvance;
	pos->u.pos.v_adv_off = which->yadvance;
    }
    free(vr);
    free(glyphs);
}

static void gposExtensionSubTable(FILE *ttf, int stoffset,
	struct ttfinfo *info, struct lookup *lookup) {
    uint32 base = ftell(ttf), st, offset;
    int lu_type;

    /* Format = */ getushort(ttf);
    lu_type = getushort(ttf);
    offset = getlong(ttf);

    fseek(ttf,st = base+offset,SEEK_SET);
    switch ( lu_type ) {
      case 1:
	gposSimplePos(ttf,st,info,lookup);
      break;  
      case 2:
	if ( lookup->tag==CHR('k','e','r','n') )
	    gposKernSubTable(ttf,st,info,lookup,false);
	else if ( lookup->tag==CHR('v','k','r','n') )
	    gposKernSubTable(ttf,st,info,lookup,true);
	else
	    gposKernSubTable(ttf,st,info,lookup,2);
      break;  
      case 3:
	if ( lookup->tag==CHR('c','u','r','s') )
	    gposCursiveSubTable(ttf,st,info,lookup);
      break;
      case 4: case 5: case 6:
	gposMarkSubTable(ttf,st,info,lookup,lu_type);
      break;
/* Any cases added here also need to go in the readttfgpossub */
      case 9:
	fprintf( stderr, "This font is erroneous it has a GPOS extension subtable that points to\nanother extension sub-table.\n" );
      break;
    }

    if ( !info->extensionrequested ) {
	info->extensionrequested = true;
	fprintf( stderr, "Hi. This font uses GPOS extension sub-tables, and I've never seen one that\ndoes. I'd like to be able to look at it to test my code. Would you\nsend a copy of %s to\n   gww@silcom.com\nThanks!\n",
		info->fontname );
    }
}

enum gsub_inusetype { git_normal, git_justinuse, git_findnames };

static struct { uint32 tag; char *str; } tagstr[] = {
    { CHR('v','r','t','2'), "vert" },
    { CHR('s','m','c','p'), "sc" },
    { CHR('s','m','c','p'), "small" },
    { CHR('o','n','u','m'), "oldstyle" },
    { CHR('s','u','p','s'), "superior" },
    { CHR('s','u','b','s'), "inferior" },
    { CHR('s','w','s','h'), "swash" },
    { 0, NULL }
};

static void gsubSimpleSubTable(FILE *ttf, int stoffset, struct ttfinfo *info,
	struct lookup *lookup, int justinuse) {
    int coverage, cnt, i, j, which;
    uint16 format;
    uint16 *glyphs, *glyph2s=NULL;
    int delta=0;

    format=getushort(ttf);
    if ( format!=1 && format!=2 )	/* Unknown subtable format */
return;
    coverage = getushort(ttf);
    if ( format==1 ) {
	delta = getushort(ttf);
    } else {
	cnt = getushort(ttf);
	glyph2s = galloc(cnt*sizeof(uint16));
	for ( i=0; i<cnt; ++i )
	    glyph2s[i] = getushort(ttf);
	    /* in range check comes later */
    }
    glyphs = getCoverageTable(ttf,stoffset+coverage,info);
    if ( glyphs==NULL ) {
	free(glyph2s);
return;
    }
    if ( justinuse==git_findnames ) {
	/* Unnamed glyphs get a name built of the base name and the lookup tag */
	for ( i=0; glyphs[i]!=0xffff; ++i ) if ( glyphs[i]<info->glyph_cnt ) {
	    if ( info->chars[glyphs[i]]->name!=NULL ) {
		which = format==1 ? (uint16) (glyphs[i]+delta) : glyph2s[i];
		if ( info->chars[which]->name==NULL ) {
		    char *basename = info->chars[glyphs[i]]->name;
		    char *str = galloc(strlen(basename)+6);
		    char tag[5], *pt=tag;
		    for ( j=0; tagstr[j].tag!=0 && tagstr[j].tag!=lookup->tag; ++j );
		    if ( tagstr[j].tag!=0 )
			pt = tagstr[j].str;
		    else {
			tag[0] = lookup->tag>>24;
			if ( (tag[1] = (lookup->tag>>16)&0xff)==' ' ) tag[1] = '\0';
			if ( (tag[2] = (lookup->tag>>8)&0xff)==' ' ) tag[2] = '\0';
			if ( (tag[3] = (lookup->tag)&0xff)==' ' ) tag[3] = '\0';
			tag[4] = '\0';
			pt = tag;
		    }
		    str = galloc(strlen(basename)+strlen(pt)+2);
		    sprintf(str,"%s.%s", basename, pt );
		    info->chars[which]->name = str;
		}
	    }
	}
    } else if ( justinuse==git_justinuse ) {
	for ( i=0; glyphs[i]!=0xffff; ++i ) if ( glyphs[i]<info->glyph_cnt ) {
	    info->inuse[glyphs[i]]= true;
	    which = format==1 ? (uint16) (glyphs[i]+delta) : glyph2s[i];
	    info->inuse[which]= true;
	}
    } else if ( justinuse==git_normal ) {
	for ( i=0; glyphs[i]!=0xffff; ++i ) if ( glyphs[i]<info->glyph_cnt && info->chars[glyphs[i]]!=NULL ) {
	    which = format==1 ? (uint16) (glyphs[i]+delta) : glyph2s[i];
	    if ( which>=info->glyph_cnt ) {
		fprintf( stderr, "Bad substitution glyph: %d not less than %d\n",
			which, info->glyph_cnt);
		which = 0;
	    }
	    if ( info->chars[which]!=NULL ) {	/* Might be in a ttc file */
		PST *pos = chunkalloc(sizeof(PST));
		pos->type = pst_substitution;
		pos->tag = lookup->tag;
		pos->script_lang_index = lookup->script_lang_index;
		pos->flags = lookup->flags;
		pos->next = info->chars[glyphs[i]]->possub;
		info->chars[glyphs[i]]->possub = pos;
		pos->u.subs.variant = copy(info->chars[which]->name);
	    }
	}
    }
    free(glyph2s);
    free(glyphs);
}

/* Multiple and alternate substitution lookups have the same format */
static void gsubMultipleSubTable(FILE *ttf, int stoffset, struct ttfinfo *info,
	struct lookup *lookup, int lu_type, int justinuse) {
    int coverage, cnt, i, j, len, max;
    uint16 format;
    uint16 *offsets;
    uint16 *glyphs, *glyph2s;
    char *pt;
    int bad;

    if ( justinuse==git_findnames )
return;

    format=getushort(ttf);
    if ( format!=1 )	/* Unknown subtable format */
return;
    coverage = getushort(ttf);
    cnt = getushort(ttf);
    offsets = galloc(cnt*sizeof(uint16));
    for ( i=0; i<cnt; ++i )
	offsets[i] = getushort(ttf);
    glyphs = getCoverageTable(ttf,stoffset+coverage,info);
    if ( glyphs==NULL ) {
	free(offsets);
return;
    }
    max = 20;
    glyph2s = galloc(max*sizeof(uint16));
    for ( i=0; glyphs[i]!=0xffff; ++i ) {
	PST *sub;
	fseek(ttf,stoffset+offsets[i],SEEK_SET);
	cnt = getushort(ttf);
	if ( cnt>max ) {
	    max = cnt+30;
	    glyph2s = grealloc(glyph2s,max*sizeof(uint16));
	}
	len = 0; bad = false;
	for ( j=0; j<cnt; ++j ) {
	    glyph2s[j] = getushort(ttf);
	    if ( glyph2s[j]>=info->glyph_cnt ) {
		if ( !justinuse )
		    fprintf( stderr, "Bad Multiple/Alternate substitution glyph %d not less than %d\n",
			    glyph2s[j], info->glyph_cnt );
		glyph2s[j] = 0;
	    }
	    if ( justinuse==git_justinuse )
		/* Do Nothing */;
	    else if ( info->chars[glyph2s[j]]==NULL )
		bad = true;
	    else
		len += strlen( info->chars[glyph2s[j]]->name) +1;
	}
	if ( justinuse==git_justinuse ) {
	    info->inuse[glyphs[i]] = 1;
	    for ( j=0; j<cnt; ++j )
		info->inuse[glyph2s[j]] = 1;
	} else if ( info->chars[glyphs[i]]!=NULL && !bad ) {
	    sub = chunkalloc(sizeof(PST));
	    sub->type = lu_type==2?pst_multiple:pst_alternate;
	    sub->tag = lookup->tag;
	    sub->script_lang_index = lookup->script_lang_index;
	    sub->flags = lookup->flags;
	    sub->next = info->chars[glyphs[i]]->possub;
	    info->chars[glyphs[i]]->possub = sub;
	    pt = sub->u.subs.variant = galloc(len+1);
	    *pt = '\0';
	    for ( j=0; j<cnt; ++j ) {
		strcat(pt,info->chars[glyph2s[j]]->name);
		strcat(pt," ");
	    }
	}
    }
    free(glyphs);
    free(glyph2s);
    free(offsets);
}

static void gsubLigatureSubTable(FILE *ttf, int stoffset,
	struct ttfinfo *info, struct lookup *lookup, int justinuse) {
    int coverage, cnt, i, j, k, lig_cnt, cc, len;
    uint16 *ls_offsets, *lig_offsets;
    uint16 *glyphs, *lig_glyphs, lig;
    char *pt;
    PST *liga;

    /* Format = */ getushort(ttf);
    coverage = getushort(ttf);
    cnt = getushort(ttf);
    ls_offsets = galloc(cnt*sizeof(uint16));
    for ( i=0; i<cnt; ++i )
	ls_offsets[i]=getushort(ttf);
    glyphs = getCoverageTable(ttf,stoffset+coverage,info);
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
	    if ( lig>=info->glyph_cnt ) {
		fprintf( stderr, "Bad ligature glyph %d not less than %d\n",
			lig, info->glyph_cnt );
		lig = 0;
	    }
	    cc = getushort(ttf);
	    if ( cc>100 ) {
		fprintf( stderr, "Unlikely count of ligature components (%d), I suspect this ligature sub-\n table is garbage, I'm giving up on it.\n", cc );
		free(glyphs); free(lig_offsets);
return;
	    }
	    lig_glyphs = galloc(cc*sizeof(uint16));
	    lig_glyphs[0] = glyphs[i];
	    for ( k=1; k<cc; ++k ) {
		lig_glyphs[k] = getushort(ttf);
		if ( lig_glyphs[k]>=info->glyph_cnt ) {
		    if ( justinuse==git_normal )
			fprintf( stderr, "Bad ligature component glyph %d not less than %d (in ligature %d)\n",
				lig_glyphs[k], info->glyph_cnt, lig );
		    lig_glyphs[k] = 0;
		}
	    }
	    if ( justinuse==git_justinuse ) {
		info->inuse[lig] = 1;
		for ( k=0; k<cc; ++k )
		    info->inuse[lig_glyphs[k]] = 1;
	    } else if ( justinuse==git_findnames ) {
		/* If our ligature glyph has no name (and its components do) */
		/*  give it a name by concatenating components with underscores */
		/*  between them, and appending the tag */
		if ( info->chars[lig]!=NULL && info->chars[lig]->name==NULL ) {
		    int len=0;
		    for ( k=0; k<cc; ++k ) {
			if ( info->chars[lig_glyphs[k]]==NULL || info->chars[lig_glyphs[k]]->name==NULL )
		    break;
			len += strlen(info->chars[lig_glyphs[k]]->name)+1;
		    }
		    if ( k==cc ) {
			char *str = galloc(len+6), *pt;
			char tag[5];
			tag[0] = lookup->tag>>24;
			if ( (tag[1] = (lookup->tag>>16)&0xff)==' ' ) tag[1] = '\0';
			if ( (tag[2] = (lookup->tag>>8)&0xff)==' ' ) tag[2] = '\0';
			if ( (tag[3] = (lookup->tag)&0xff)==' ' ) tag[3] = '\0';
			tag[4] = '\0';
			*str='\0';
			for ( k=0; k<cc; ++k ) {
			    strcat(str,info->chars[lig_glyphs[k]]->name);
			    strcat(str,"_");
			}
			pt = str+strlen(str);
			pt[-1] = '.';
			strcpy(pt,tag);
			info->chars[lig]->name = str;
		    }
		}
	    } else if ( info->chars[lig]!=NULL ) {
		for ( k=len=0; k<cc; ++k )
		    len += strlen(info->chars[lig_glyphs[k]]->name)+1;
		liga = chunkalloc(sizeof(PST));
		liga->type = pst_ligature;
		liga->tag = lookup->tag;
		liga->script_lang_index = lookup->script_lang_index;
		liga->flags = lookup->flags;
		liga->next = info->chars[lig]->possub;
		info->chars[lig]->possub = liga;
		liga->u.lig.lig = info->chars[lig];
		liga->u.lig.components = pt = galloc(len);
		for ( k=0; k<cc; ++k ) {
		    strcpy(pt,info->chars[lig_glyphs[k]]->name);
		    pt += strlen(pt);
		    *pt++ = ' ';
		}
		pt[-1] = '\0';
		free(lig_glyphs);
	    }
	}
	free(lig_offsets);
    }
    free(ls_offsets); free(glyphs);
}

static void gsubExtensionSubTable(FILE *ttf, int stoffset,
	struct ttfinfo *info, struct lookup *lookup, int justinuse) {
    uint32 base = ftell(ttf), st, offset;
    int lu_type;

    /* Format = */ getushort(ttf);
    lu_type = getushort(ttf);
    offset = getlong(ttf);

    fseek(ttf,st = base+offset,SEEK_SET);
    switch ( lu_type ) {
      case 1:
	gsubSimpleSubTable(ttf,st,info,lookup,justinuse);
      break;
      case 2: case 3:	/* Multiple and alternate have same format, different semantics */
	gsubMultipleSubTable(ttf,st,info,lookup,lu_type,justinuse);
      break;
      case 4:
	gsubLigatureSubTable(ttf,st,info,lookup,justinuse);
      break;
/* Any cases added here also need to go in the readttfgpossub and readttfgsubUsed */
      case 7:
	fprintf( stderr, "This font is erroneous it has a GSUB extension subtable that points to\nanother extension sub-table.\n" );
      break;
    }
#if 0
    if ( !info->extensionrequested ) {
	info->extensionrequested = true;
	fprintf( stderr, "Hi. This font uses GSUB extension sub-tables, and I've never seen one that\ndoes. I'd like to be able to look at it to test my code. Would you\nsend a copy of %s to\n   gww@silcom.com\nThanks!\n",
		info->fontname );
    }
#endif
}

static struct feature *readttffeatures(FILE *ttf,int32 pos,int isgpos, struct ttfinfo *info) {
    /* read the features table returning an array containing all interesting */
    /*  features */
    int cnt;
    uint32 tag;
    int i,j;
    struct feature *features;

    fseek(ttf,pos,SEEK_SET);
    cnt = getushort(ttf);
    if ( cnt<=0 )
return( NULL );
    features = gcalloc(cnt+1,sizeof(struct feature));
    info->feats[isgpos] = galloc((cnt+1)*sizeof(uint32));
    info->feats[isgpos][0] = 0;
    for ( i=0; i<cnt; ++i ) {
	features[i].tag = tag = getlong(ttf);
	features[i].offset = getushort(ttf);
    }

    for ( i=0; i<cnt; ++i ) {
	fseek(ttf,pos+features[i].offset,SEEK_SET);
	/* feature parameters = */ getushort(ttf);
	features[i].lcnt = getushort(ttf);
	features[i].lookups = galloc(features[i].lcnt*sizeof(uint16));
	for ( j=0; j<features[i].lcnt; ++j )
	    features[i].lookups[j] = getushort(ttf);
    }

return( features );
}

static struct scriptlist *ScriptListCopy(const struct scriptlist *sl) {
    struct scriptlist *head=NULL, *last=NULL, *new;

    while ( sl ) {
	new = galloc(sizeof(struct scriptlist));
	*new = *sl;
	if ( head==NULL )
	    head = new;
	else
	    last->next = new;
	last = new;
	sl = sl->next;
    }
return( head );
}

static struct scriptlist *ScriptListMerge(struct scriptlist *into,const struct scriptlist *from) {
    struct scriptlist *sl, *new;
    int i,j;

    while ( from ) {
	for ( sl=into; sl!=NULL && sl->script!=from->script; sl=sl->next );
	if ( sl==NULL ) {
	    new = galloc(sizeof(struct scriptlist));
	    *new = *from;
	    new->next = into;
	    into = new;
	} else {
	    for ( i=0; i<from->lang_cnt; ++i ) {
		if ( sl->lang_cnt<MAX_LANG ) {
		    for ( j=0; j<sl->lang_cnt && sl->langs[j]!=from->langs[i]; ++j );
		    if ( j>=sl->lang_cnt )
			sl->langs[sl->lang_cnt++] = from->langs[i];
		}
	    }
	}
	from = from->next;
    }
return( into );
}

static struct lookup *compactttflookups(struct feature *features,uint32 *feats) {
    /* go through the feature table we read, and return an array containing */
    /*  all lookup indeces which match the feature tag */
    int cnt, extras=0;
    int i,j,k,l;
    struct lookup *lookups, *cur, *alt;

    cnt = -1;
    for ( i=0; features[i].tag!=0; ++i )
	for ( k=0; k<features[i].lcnt; ++k )
	    if ( cnt<features[i].lookups[k] ) cnt = features[i].lookups[k];
    ++cnt;

    lookups = gcalloc(cnt+1,sizeof(struct lookup));
    for ( i=0; features[i].tag!=0; ++i ) {
	for ( k=0; k<features[i].lcnt; ++k ) {
	    j = features[i].lookups[k];
	    cur = &lookups[j];
	    while ( cur!=NULL && cur->tag!=0 && cur->tag!=features[i].tag )
		cur = cur->alttags;
	    if ( cur==NULL ) {
		cur = gcalloc(1,sizeof(struct lookup));
		cur->alttags = lookups[j].alttags;
		lookups[j].alttags = cur;
		++extras;
	    }
	    if ( cur->tag==0 ) {
		cur->tag = features[i].tag;
		cur->sl = ScriptListCopy(features[i].sl);
		cur->lookup = j;
	    } else {
		cur->sl = ScriptListMerge(cur->sl,features[i].sl);
	    }
	}
	free( features[i].lookups );
    }
    /* Some lookups may not be "interesting" so remove holes */
    for ( i=j=0; i<cnt; ++i ) {
	if ( lookups[i].tag!=0 )
	    lookups[j++] = lookups[i];
    }
    if ( extras!=0 ) {
	lookups = grealloc(lookups,(j+extras+1)*sizeof(struct lookup));
	for ( i=0; i<j; ++i ) {
	    for ( cur = lookups[i].alttags; cur!=NULL; cur = alt ) {
		alt = cur->alttags;
		lookups[j] = *cur;
		lookups[j++].alttags = NULL;
		free(cur);
	    }
	}
    }

    free( features );
    if ( j==0 ) {
	free(lookups);
return( NULL );
    }

    lookups[j].lookup = -1;
    lookups[j].tag = 0;
    lookups[j].script_lang_index = 0xffff;

    for ( i=0; i<j ; ++i ) for ( k=0; k<j; ++k ) if ( lookups[k].lookup==i ) {
	for ( l=0; feats[l]!=0 && feats[l]!=lookups[k].tag; ++l );
	if ( feats[l]==0 ) {
	    feats[l] = lookups[k].tag;
	    feats[l+1] = 0;
	}
    }
return( lookups );
}

static void LSysAddToFeature(struct feature *feature,uint32 script,uint32 lang,
	int is_required) {
    int j;
    struct scriptlist *sl;

    for ( sl=is_required ? feature->reqsl : feature->sl; sl!=NULL && sl->script!=script; sl=sl->next );
    if ( sl==NULL ) {
	sl = gcalloc(1,sizeof(struct scriptlist));
	if ( is_required ) {
	    sl->next = feature->reqsl;
	    feature->reqsl = sl;
	} else {
	    sl->next = feature->sl;
	    feature->sl = sl;
	}
	sl->script = script;
    }
    if ( sl->lang_cnt<MAX_LANG ) {
	for ( j=sl->lang_cnt-1; j>=0 ; --j )
	    if ( sl->langs[j]==lang )
	break;
	if ( j<0 )
	    sl->langs[sl->lang_cnt++] = lang;
    }
}

static void ProcessLangSys(FILE *ttf,uint32 langsysoff,
	struct feature *features, uint32 script, uint32 lang) {
    int cnt, feature, i;

    fseek(ttf,langsysoff,SEEK_SET);
    /* lookuporder = */ getushort(ttf);	/* not meaningful yet */
    feature = getushort(ttf); /* required feature */
    if ( feature==0xffff )
	/* No required feature */;
    else {
	LSysAddToFeature(&features[feature],script,lang,true);
    }
    cnt = getushort(ttf);
    for ( i=0; i<cnt; ++i ) {
	feature = getushort(ttf);
	LSysAddToFeature(&features[feature],script,lang,false);
    }
}

static struct feature *RequiredFeatureFixup( struct feature *features) {
    int i, extra=0;

    for ( i=0; features[i].tag!=0; ++i ) {
	if ( features[i].reqsl==NULL )
	    /* Nothing to be done */;
	else if ( features[i].sl==NULL ) {
	    features[i].tag = REQUIRED_FEATURE;
	    features[i].sl = features[i].reqsl;
	    features[i].reqsl = NULL;
	} else
	    ++extra;
    }
    if ( extra == 0 )
return( features );
    features = grealloc(features,(i+extra+1)*sizeof(struct feature));
    memset(features+i,0,(extra+1)*sizeof(struct feature));
    extra = i;
    for ( i=0; features[i].tag!=0; ++i ) {
	if ( features[i].reqsl!=NULL && features[i].sl!=NULL ) {
	    features[extra] = features[i];
	    features[extra].tag = REQUIRED_FEATURE;
	    features[extra].sl = features[i].reqsl;
	    features[i].reqsl = NULL; features[extra].reqsl = NULL;
	    ++extra;
	}
    }
return( features );
}

static struct feature *tagTtfFeaturesWithScript(FILE *ttf,uint32 script_pos,
	struct feature *features) {
    int cnt, i, j;
    struct scriptrec {
	uint32 script;
	uint32 offset;
    } *scripts;
    struct stab {
	uint16 deflangsys;
	int langcnt, maxcnt;
	uint16 *langsys;
	uint32 *lang;
    } stab;

    fseek(ttf,script_pos,SEEK_SET);
    cnt = getushort(ttf);
    scripts = galloc(cnt*sizeof(struct scriptrec));
    for ( i=0; i<cnt; ++i ) {
	scripts[i].script = getlong(ttf);
	scripts[i].offset = script_pos+getushort(ttf);
    }

    memset(&stab,0,sizeof(stab));
    stab.maxcnt = 30;
    stab.langsys = galloc(30*sizeof(uint16));
    stab.lang = galloc(30*sizeof(uint32));
    for ( i=0; i<cnt; ++i ) {
	fseek(ttf,scripts[i].offset,SEEK_SET);
	stab.deflangsys = getushort(ttf);
	stab.langcnt = getushort(ttf);
	if ( stab.langcnt>=stab.maxcnt ) {
	    stab.maxcnt = stab.langcnt+30;
	    stab.langsys = grealloc(stab.langsys,stab.maxcnt*sizeof(uint16));
	    stab.lang = grealloc(stab.langsys,stab.maxcnt*sizeof(uint32));
	}
	for ( j=0; j<stab.langcnt; ++j ) {
	    stab.lang[j] = getlong(ttf);
	    stab.langsys[j] = getushort(ttf);
	}
	if ( stab.deflangsys!=0 )
	    ProcessLangSys(ttf,scripts[i].offset+stab.deflangsys,features,scripts[i].script,DEFAULT_LANG);
	for ( j=0; j<stab.langcnt; ++j )
	    ProcessLangSys(ttf,scripts[i].offset+stab.langsys[j],features,scripts[i].script,stab.lang[j]);
    }
    free(stab.langsys);
    free(stab.lang);
    free(scripts);

return( RequiredFeatureFixup(features));
}

static int SLMatch(struct script_record *sr,struct scriptlist *sl) {
    int i,j;
    struct scriptlist *slt;

    for ( j=0, slt=sl; slt!=NULL; slt=slt->next, ++j );
    for ( i=0; sr[i].script!=0; ++i );
    if ( i!=j )
return( false );

    for ( slt=sl; slt!=NULL; slt=slt->next ) {
	for ( i=0; sr[i].script!=0 && sr[i].script!=slt->script; ++i );
	if ( sr[i].script==0 )
return( false );
	for ( j=0 ; sr[i].langs[j]!=0; ++j );
	if ( j!=slt->lang_cnt )
return( false );
	for ( j=0 ; sr[i].langs[j]!=0; ++j )
	    if ( sr[i].langs[j]!=slt->langs[j] )
return( false );
    }
return( true );
}

static void FigureScriptIndeces(struct ttfinfo *info,struct lookup *lookups) {
    int i,j,k;
    struct scriptlist *sl, *snext;

    for ( i=0; lookups[i].tag!=0; ++i );
    if ( info->script_lang==NULL ) {
	info->script_lang = gcalloc(i+1,sizeof(struct script_record *));
    } else {
	for ( j=0; info->script_lang[j]!=NULL; ++j );
	info->script_lang = grealloc(info->script_lang,(i+j+1)*sizeof(struct script_record *));
    }
    for ( i=0; lookups[i].tag!=0; ++i ) {
	for ( j=0; info->script_lang[j]!=NULL; ++j )
	    if ( SLMatch(info->script_lang[j],lookups[i].sl))
	break;
	lookups[i].script_lang_index = j;
	if ( info->script_lang[j]==NULL ) {
	    for ( k=0, sl=lookups[i].sl; sl!=NULL; sl=sl->next, ++k );
	    info->script_lang[j] = galloc((k+1)*sizeof(struct script_record));
	    for ( k=0, sl=lookups[i].sl; sl!=NULL; sl=sl->next, ++k ) {
		info->script_lang[j][k].script = sl->script;
		info->script_lang[j][k].langs = galloc((sl->lang_cnt+1)*sizeof(uint32));
		memcpy(info->script_lang[j][k].langs,sl->langs,sl->lang_cnt*sizeof(uint32));
		info->script_lang[j][k].langs[sl->lang_cnt]=0;
	    }
	    info->script_lang[j][k].script = 0;
	    info->script_lang[j+1] = NULL;
	}
	for ( sl=lookups[i].sl; sl!=NULL; sl=snext ) {
	    snext = sl->next;
	    free( sl );
	}
    }
}

static void ProcessGPOSGSUB(FILE *ttf,struct ttfinfo *info,int gpos,int inusetype) {
    int i, j, k, lu_cnt, lu_type, cnt, st;
    uint16 *lu_offsets, *st_offsets;
    int32 base, lookup_start;
    int32 script_off, feature_off;
    struct feature *features;
    struct lookup *lookups;

    base = gpos?info->gpos_start:info->gsub_start;
    fseek(ttf,base,SEEK_SET);
    /* version = */ getlong(ttf);
    script_off = getushort(ttf);
    feature_off = getushort(ttf);
    lookup_start = base+getushort(ttf);
    features = readttffeatures(ttf,base+feature_off,gpos,info);
    if ( features==NULL )		/* None of the data we care about */
return;
    features = tagTtfFeaturesWithScript(ttf,base+script_off,features);
    lookups = compactttflookups( features,info->feats[gpos] );
    if ( lookups==NULL )
return;
    FigureScriptIndeces(info,lookups);
    
    fseek(ttf,lookup_start,SEEK_SET);

    lu_cnt = getushort(ttf);
    lu_offsets = galloc(lu_cnt*sizeof(uint16));
    for ( i=0; i<lu_cnt; ++i )
	lu_offsets[i] = getushort(ttf);

    for ( k=0; lookups[k].lookup!=(uint16) -1; ++k ) {
	i = lookups[k].lookup;
	if ( i>=lu_cnt )
    continue;
	fseek(ttf,lookup_start+lu_offsets[i],SEEK_SET);
	lu_type = getushort(ttf);
	lookups[k].flags = getushort(ttf);
	cnt = getushort(ttf);
	st_offsets = galloc(cnt*sizeof(uint16));
	for ( j=0; j<cnt; ++j )
	    st_offsets[j] = getushort(ttf);
	for ( j=0; j<cnt; ++j ) {
	    fseek(ttf,st = lookup_start+lu_offsets[i]+st_offsets[j],SEEK_SET);
	    if ( gpos ) {
		switch ( lu_type ) {
		  case 1:
		    gposSimplePos(ttf,st,info,&lookups[k]);
		  break;  
		  case 2:
		    if ( lookups[k].tag==CHR('k','e','r','n') )
			gposKernSubTable(ttf,st,info,&lookups[k],false);
		    else if ( lookups[k].tag==CHR('v','k','r','n') )
			gposKernSubTable(ttf,st,info,&lookups[k],true);
		    else
			gposKernSubTable(ttf,st,info,&lookups[k],2);
		  break;  
		  case 3:
		    if ( lookups[k].tag==CHR('c','u','r','s') )
			gposCursiveSubTable(ttf,st,info,&lookups[k]);
		  break;
		  case 4: case 5: case 6:
		    gposMarkSubTable(ttf,st,info,&lookups[k],lu_type);
		  break;
/* Any cases added here also need to go in the gposExtensionSubTable */
		  case 9:
		    gposExtensionSubTable(ttf,st,info,&lookups[k]);
		  break;
		}
	    } else {
		switch ( lu_type ) {
		  case 1:
		    gsubSimpleSubTable(ttf,st,info,&lookups[k],inusetype);
		  break;
		  case 2: case 3:	/* Multiple and alternate have same format, different semantics */
		    gsubMultipleSubTable(ttf,st,info,&lookups[k],lu_type,inusetype);
		  break;
		  case 4:
		    gsubLigatureSubTable(ttf,st,info,&lookups[k],inusetype);
		  break;
/* Any cases added here also need to go in the gsubExtensionSubTable and readttfgsubUsed */
		  case 7:
		    gsubExtensionSubTable(ttf,st,info,&lookups[k],inusetype);
		  break;
		}
	    }
	}
	free(st_offsets);
    }
    free(lu_offsets);
    free(lookups);
}

static void readttfgsubUsed(FILE *ttf,struct ttfinfo *info) {
    ProcessGPOSGSUB(ttf,info,false,git_justinuse);
}

static void GuessNamesFromGSUB(FILE *ttf,struct ttfinfo *info) {
    ProcessGPOSGSUB(ttf,info,false,git_findnames);
}

static void readttfgpossub(FILE *ttf,struct ttfinfo *info,int gpos) {
    ProcessGPOSGSUB(ttf,info,gpos,git_normal);
}

static void readttfgdef(FILE *ttf,struct ttfinfo *info) {
    int lclo;
    int coverage, cnt, i,j, format;
    uint16 *glyphs, *lc_offsets, *offsets;
    uint32 caret_base;
    PST *pst;
    SplineChar *sc;

    fseek(ttf,info->gdef_start,SEEK_SET);
    if ( getlong(ttf)!=0x00010000 )
return;
    /* glyph class def = */ getushort(ttf);
    /* attach list = */ getushort(ttf);
    lclo = getushort(ttf);		/* ligature caret list */
    /* mark attach class = */ getushort(ttf);
    if ( lclo==0 )
return;

    lclo += info->gdef_start;
    fseek(ttf,lclo,SEEK_SET);
    coverage = getushort(ttf);
    cnt = getushort(ttf);
    if ( cnt==0 )
return;
    lc_offsets = galloc(cnt*sizeof(uint16));
    for ( i=0; i<cnt; ++i )
	lc_offsets[i]=getushort(ttf);
    glyphs = getCoverageTable(ttf,lclo+coverage,info);
    for ( i=0; i<cnt; ++i ) if ( glyphs[i]<info->glyph_cnt ) {
	fseek(ttf,lclo+lc_offsets[i],SEEK_SET);
	sc = info->chars[glyphs[i]];
	for ( pst=sc->possub; pst!=NULL && pst->type!=pst_lcaret; pst=pst->next );
	if ( pst==NULL ) {
	    pst = chunkalloc(sizeof(PST));
	    pst->next = sc->possub;
	    sc->possub = pst;
	    pst->type = pst_lcaret;
	}
	caret_base = ftell(ttf);
	pst->u.lcaret.cnt = getushort(ttf);
	if ( pst->u.lcaret.carets!=NULL ) free(pst->u.lcaret.carets);
	offsets = galloc(pst->u.lcaret.cnt*sizeof(uint16));
	for ( j=0; j<pst->u.lcaret.cnt; ++j )
	    offsets[j] = getushort(ttf);
	pst->u.lcaret.carets = galloc(pst->u.lcaret.cnt*sizeof(int16));
	for ( j=0; j<pst->u.lcaret.cnt; ++j ) {
	    fseek(ttf,caret_base+offsets[j],SEEK_SET);
	    format=getushort(ttf);
	    if ( format==1 ) {
		pst->u.lcaret.carets[j] = getushort(ttf);
	    } else if ( format==2 ) {
		pst->u.lcaret.carets[j] = 0;
		/* point = */ getushort(ttf);
	    } else if ( format==3 ) {
		pst->u.lcaret.carets[j] = getushort(ttf);
		/* in device table = */ getushort(ttf);
	    } else {
		fprintf( stderr, "!!!! Unknown caret format %d !!!!\n", format );
	    }
	}
	free(offsets);
    }
    free(lc_offsets);
    free(glyphs);
}

static void readttf_applelookup(FILE *ttf,struct ttfinfo *info,
	void (*apply_values)(struct ttfinfo *info, int gfirst, int glast,FILE *ttf),
	void (*apply_value)(struct ttfinfo *info, int gfirst, int glast,FILE *ttf),
	void (*apply_default)(struct ttfinfo *info, int gfirst, int glast,void *def),
	void *def) {
    int format, i, first, last, data_off, cnt, prev;
    uint32 here;
    uint32 base = ftell(ttf);

    switch ( format = getushort(ttf)) {
      case 0:	/* Simple array */
	apply_values(info,0,info->glyph_cnt-1,ttf);
      break;
      case 2:	/* Segment Single */
	/* Entry size  */ getushort(ttf);
	cnt = getushort(ttf);
	/* search range */ getushort(ttf);
	/* log2(cnt)    */ getushort(ttf);
	/* range shift  */ getushort(ttf);
	prev = 0;
	for ( i=0; i<cnt; ++i ) {
	    last = getushort(ttf);
	    first = getushort(ttf);
	    if ( apply_default!=NULL )
		apply_default(info,prev,first-1,def);
	    apply_value(info,first,last,ttf);
	    prev = last+1;
	}
      break;
      case 4:	/* Segment multiple */
	/* Entry size  */ getushort(ttf);
	cnt = getushort(ttf);
	/* search range */ getushort(ttf);
	/* log2(cnt)    */ getushort(ttf);
	/* range shift  */ getushort(ttf);
	prev = 0;
	for ( i=0; i<cnt; ++i ) {
	    last = getushort(ttf);
	    first = getushort(ttf);
	    data_off = getushort(ttf);
	    here = ftell(ttf);
	    if ( apply_default!=NULL )
		apply_default(info,prev,first-1,def);
	    fseek(ttf,base+data_off,SEEK_SET);
	    apply_values(info,first,last,ttf);
	    fseek(ttf,here,SEEK_SET);
	    prev = last+1;
	}
      break;
      case 6:	/* Single table */
	/* Entry size  */ getushort(ttf);
	cnt = getushort(ttf);
	/* search range */ getushort(ttf);
	/* log2(cnt)    */ getushort(ttf);
	/* range shift  */ getushort(ttf);
	prev = 0;
	for ( i=0; i<cnt; ++i ) {
	    first = getushort(ttf);
	    if ( apply_default!=NULL )
		apply_default(info,prev,first-1,def);
	    apply_value(info,first,first,ttf);
	    prev = first+1;
	}
      break;
      case 8:	/* Simple array */
	first = getushort(ttf);
	cnt = getushort(ttf);
	if ( apply_default!=NULL ) {
	    apply_default(info,0,first-1,def);
	    apply_default(info,first+cnt,info->glyph_cnt-1,def);
	}
	apply_values(info,first,first+cnt-1,ttf);
      break;
      default:
	fprintf( stderr, "Invalid lookup table format. %d\n", format );
      break;
    }
}

static void TTF_SetProp(struct ttfinfo *info,int gnum, int prop) {
    int offset;
    PST *pst;

    if ( gnum<0 || gnum>=info->glyph_cnt ) {
	fprintf( stderr, "Glyph out of bounds in 'prop' table %d\n", gnum );
return;
    }

    if ( prop&0x1000 ) {	/* Mirror */
	offset = (prop<<20)>>28;
	if ( gnum+offset>=0 && gnum+offset<info->glyph_cnt &&
		info->chars[gnum+offset]->name!=NULL ) {
	    pst = chunkalloc(sizeof(PST));
	    pst->type = pst_substitution;
	    pst->tag = CHR('r','t','l','a');
	    pst->script_lang_index = SLIFromInfo(info,info->chars[gnum],DEFAULT_LANG);
	    pst->next = info->chars[gnum]->possub;
	    info->chars[gnum]->possub = pst;
	    pst->u.subs.variant = copy(info->chars[gnum+offset]->name);
	}
    }
}

static void prop_apply_values(struct ttfinfo *info, int gfirst, int glast,FILE *ttf) {
    int i;

    for ( i=gfirst; i<=glast; ++i )
	TTF_SetProp(info,i, getushort(ttf));
}

static void prop_apply_value(struct ttfinfo *info, int gfirst, int glast,FILE *ttf) {
    int i;
    int prop;

    prop = getushort(ttf);
    for ( i=gfirst; i<=glast; ++i )
	TTF_SetProp(info,i, prop);
}

static void prop_apply_default(struct ttfinfo *info, int gfirst, int glast,void *def) {
    int def_prop, i;

    def_prop = (int) def;
    for ( i=gfirst; i<=glast; ++i )
	TTF_SetProp(info,i, def_prop);
}

static void readttfprop(FILE *ttf,struct ttfinfo *info) {
    int def;

    fseek(ttf,info->prop_start,SEEK_SET);
    /* The one example that I've got has a wierd version, so I don't check it */
    /*  the three versions that I know about are all pretty much the same, just a few extra flags */
    /* version = */ getlong(ttf);
    /* format = */ getushort(ttf);
    def = getushort(ttf);
    readttf_applelookup(ttf,info,
	    prop_apply_values,prop_apply_value,
	    prop_apply_default,(void *) def);
}

static void TTF_SetLcaret(struct ttfinfo *info, int gnum, int offset, FILE *ttf) {
    uint32 here = ftell(ttf);
    PST *pst;
    SplineChar *sc;
    int cnt, i;

    if ( gnum<0 || gnum>=info->glyph_cnt ) {
	fprintf( stderr, "Glyph out of bounds in 'lcar' table %d\n", gnum );
return;
    } else if ( (sc=info->chars[gnum])==NULL )
return;

    fseek(ttf,info->lcar_start+offset,SEEK_SET);
    cnt = getushort(ttf);
    pst = chunkalloc(sizeof(PST));
    pst->type = pst_lcaret;
    pst->tag = CHR(' ',' ',' ',' ');
    pst->next = sc->possub;
    sc->possub = pst;
    pst->u.lcaret.cnt = cnt;
    pst->u.lcaret.carets = galloc(cnt*sizeof(uint16));
    for ( i=0; i<cnt; ++i )
	pst->u.lcaret.carets[i] = getushort(ttf);
    fseek(ttf,here,SEEK_SET);
}

static void lcar_apply_values(struct ttfinfo *info, int gfirst, int glast,FILE *ttf) {
    int i;

    for ( i=gfirst; i<=glast; ++i )
	TTF_SetLcaret(info,i, getushort(ttf), ttf);
}

static void lcar_apply_value(struct ttfinfo *info, int gfirst, int glast,FILE *ttf) {
    int i;
    int offset;

    offset = getushort(ttf);
    for ( i=gfirst; i<=glast; ++i )
	TTF_SetLcaret(info,i, offset, ttf);
}
    
static void readttflcar(FILE *ttf,struct ttfinfo *info) {

    fseek(ttf,info->lcar_start,SEEK_SET);
    /* version = */ getlong(ttf);
    if ( getushort(ttf)!=0 )	/* A format type of 1 has the caret locations */
return;				/*  indicated by points */
    readttf_applelookup(ttf,info,
	    lcar_apply_values,lcar_apply_value,NULL,NULL);
}

static void TTF_SetOpticalBounds(struct ttfinfo *info, int gnum, int left, int right) {
    PST *pst;
    SplineChar *sc;

    if ( left==0 && right==0 )
return;

    if ( gnum<0 || gnum>=info->glyph_cnt ) {
	fprintf( stderr, "Glyph out of bounds in 'opbd' table %d\n", gnum );
return;
    } else if ( (sc=info->chars[gnum])==NULL )
return;

    if ( left!=0 ) {
	pst = chunkalloc(sizeof(PST));
	pst->type = pst_position;
	pst->tag = CHR('l','f','b','d');
	pst->script_lang_index = SLIFromInfo(info,sc,DEFAULT_LANG);
	pst->next = sc->possub;
	sc->possub = pst;
	pst->u.pos.xoff = left;
	pst->u.pos.h_adv_off = left;
    }
    if ( right!=0 ) {
	pst = chunkalloc(sizeof(PST));
	pst->type = pst_position;
	pst->tag = CHR('r','t','b','d');
	pst->script_lang_index = SLIFromInfo(info,sc,DEFAULT_LANG);
	pst->next = sc->possub;
	sc->possub = pst;
	pst->u.pos.h_adv_off = -right;
    }
}

static void opbd_apply_values(struct ttfinfo *info, int gfirst, int glast,FILE *ttf) {
    int i, left, right, offset;
    uint32 here;
    /* the apple docs say that the bounds live in the lookup table */
    /* the picture in the docs shows the bounds pointed to */
    /* the example shows the bounds pointed to */
    /* We conclude the original authors were bad writers */

    for ( i=gfirst; i<=glast; ++i ) {
	offset = getushort(ttf);
	here = ftell(ttf);
	fseek(ttf,info->opbd_start+offset,SEEK_SET);
	left = (int16) getushort(ttf);
	/* top = (int16) */ getushort(ttf);
	right = (int16) getushort(ttf);
	/* bottom = (int16) */ getushort(ttf);
	fseek(ttf,here,SEEK_SET);
	TTF_SetOpticalBounds(info,i, left, right);
    }
}

static void opbd_apply_value(struct ttfinfo *info, int gfirst, int glast,FILE *ttf) {
    int i, left, right, offset;
    uint32 here;

    offset = getushort(ttf);
    here = ftell(ttf);
    fseek(ttf,info->opbd_start+offset,SEEK_SET);
    left = (int16) getushort(ttf);
    /* top = (int16) */ getushort(ttf);
    right = (int16) getushort(ttf);
    /* bottom = (int16) */ getushort(ttf);
    fseek(ttf,here,SEEK_SET);

    for ( i=gfirst; i<=glast; ++i )
	TTF_SetOpticalBounds(info,i, left, right);
}
    
static void readttfopbd(FILE *ttf,struct ttfinfo *info) {

    fseek(ttf,info->opbd_start,SEEK_SET);
    /* version = */ getlong(ttf);
    if ( getushort(ttf)!=0 )	/* A format type of 1 has the bounds */
return;				/*  indicated by points */
    readttf_applelookup(ttf,info,
	    opbd_apply_values,opbd_apply_value,NULL,NULL);
}

static void TTF_SetMortSubs(struct ttfinfo *info, int gnum, int gsubs) {
    PST *pst;
    SplineChar *sc, *ssc;

    if ( gsubs==0 )
return;

    if ( gnum<0 || gnum>=info->glyph_cnt ) {
	fprintf( stderr, "Glyph out of bounds in 'mort' table %d\n", gnum );
return;
    } else if ( gsubs<0 || gsubs>=info->glyph_cnt ) {
	fprintf( stderr, "Substitute glyph out of bounds in 'mort' table %d\n", gsubs );
return;
    } else if ( (sc=info->chars[gnum])==NULL || (ssc=info->chars[gsubs])==NULL )
return;

    pst = chunkalloc(sizeof(PST));
    pst->type = pst_substitution;
    pst->tag = info->mort_subs_tag;
    pst->flags = info->mort_r2l ? pst_r2l : 0;
    pst->script_lang_index = SLIFromInfo(info,sc,DEFAULT_LANG);
    pst->next = sc->possub;
    sc->possub = pst;
    pst->u.subs.variant = copy(ssc->name);
}

static void mort_apply_values(struct ttfinfo *info, int gfirst, int glast,FILE *ttf) {
    uint16 gnum;
    int i;

    for ( i=gfirst; i<=glast; ++i ) {
	gnum = getushort(ttf);
	TTF_SetMortSubs(info,i, gnum);
    }
}

static void mort_apply_value(struct ttfinfo *info, int gfirst, int glast,FILE *ttf) {
    uint16 gnum;
    int i;

    gnum = getushort(ttf);

    for ( i=gfirst; i<=glast; ++i )
	TTF_SetMortSubs(info,i, gnum );
}

static void mortclass_apply_values(struct ttfinfo *info, int gfirst, int glast,FILE *ttf) {
    int i;

    for ( i=gfirst; i<=glast; ++i )
	info->morx_classes[i] = getushort(ttf);
}

static void mortclass_apply_value(struct ttfinfo *info, int gfirst, int glast,FILE *ttf) {
    uint16 class;
    int i;

    class = getushort(ttf);

    for ( i=gfirst; i<=glast; ++i )
	info->morx_classes[i] = class;
}

int32 memlong(uint8 *data,int offset) {
    int ch1 = data[offset], ch2 = data[offset+1], ch3 = data[offset+2], ch4 = data[offset+3];
return( (ch1<<24)|(ch2<<16)|(ch3<<8)|ch4 );
}

int memushort(uint8 *data,int offset) {
    int ch1 = data[offset], ch2 = data[offset+1];
return( (ch1<<8)|ch2 );
}

void memputshort(uint8 *data,int offset,uint16 val) {
    data[offset] = (val>>8);
    data[offset+1] = val&0xff;
}

#define MAX_LIG_COMP	16
struct statemachine {
    uint8 *data;
    int length;
    uint32 nClasses;
    uint32 classOffset, stateOffset, entryOffset, ligActOff, compOff, ligOff;
    uint16 *classes;
    uint16 lig_comp_classes[MAX_LIG_COMP];
    uint16 lig_comp_glyphs[MAX_LIG_COMP];
    int lcp;
    uint8 *states_in_use;
    int smax;
    struct ttfinfo *info;
    int cnt;
};

static void mort_figure_ligatures(struct statemachine *sm, int lcp, int off, int32 lig_offset) {
    uint32 lig;
    int i, j, lig_glyph;
    PST *pst;
    int len;

    if ( lcp<0 || off+3>sm->length )
return;

    lig = memlong(sm->data,off);
    off += sizeof(long);

    for ( i=0; i<sm->info->glyph_cnt; ++i ) if ( sm->classes[i]==sm->lig_comp_classes[lcp] ) {
	sm->lig_comp_glyphs[lcp] = i;
	lig_offset += memushort(sm->data,2*( ((((int32) lig)<<2)>>2) + i ) );
	if ( lig&0xc0000000 ) {
	    if ( lig_offset+1 > sm->length ) {
		fprintf( stderr, "Invalid ligature offset\n" );
    break;
	    }
	    lig_glyph = memushort(sm->data,lig_offset);
	    if ( lig_glyph>=sm->info->glyph_cnt ) {
		fprintf(stderr, "Attempt to make a ligature for glyph %d out of " );
		for ( j=lcp; j<sm->lcp; ++j )
		    fprintf(stderr,"%d ",sm->lig_comp_glyphs[j]);
		fprintf(stderr,"\n");
	    } else {
		char *comp;
		for ( len=0, j=lcp; j<sm->lcp; ++j )
		    len += strlen(sm->info->chars[sm->lig_comp_glyphs[j]]->name)+1;
		comp = galloc(len);
		*comp = '\0';
		for ( j=lcp; j<sm->lcp; ++j ) {
		    strcat(comp,sm->info->chars[sm->lig_comp_glyphs[j]]->name);
		    if ( j!=sm->lcp-1 )
			strcat(comp," ");
		}
		for ( pst=sm->info->chars[lig_glyph]->possub; pst!=NULL; pst=pst->next )
		    if ( pst->type==pst_ligature && pst->tag==sm->info->mort_subs_tag &&
			    strcmp(comp,pst->u.lig.components)==0 )
		break;
		/* There are cases where there will be multiple entries for */
		/*  the same lig. ie. if we have "ff" and "ffl" then there */
		/*  will be multiple entries for "ff" */
		if ( pst == NULL ) {
		    pst = chunkalloc(sizeof(PST));
		    pst->type = pst_ligature;
		    pst->tag = sm->info->mort_subs_tag;
		    pst->flags = sm->info->mort_r2l ? (pst_r2l|pst_ignorecombiningmarks) : pst_ignorecombiningmarks;
		    pst->script_lang_index = SLIFromInfo(sm->info,sm->info->chars[lig_glyph],DEFAULT_LANG);
		    pst->u.lig.components = comp;
		    pst->u.lig.lig = sm->info->chars[lig_glyph];
		    pst->next = sm->info->chars[lig_glyph]->possub;
		    sm->info->chars[lig_glyph]->possub = pst;
		} else
		    free(comp);
	    }
	} else
	    mort_figure_ligatures(sm,lcp-1,off,lig_offset);
	lig_offset -= memushort(sm->data,2*( ((((int32) lig)<<2)>>2) + i ) );
    }
}

static void follow_mort_state(struct statemachine *sm,int offset,int class) {
    int state = (offset-sm->stateOffset)/sm->nClasses;
    int class_top, class_bottom;

    if ( state<0 || state>=sm->smax || sm->states_in_use[state] || sm->lcp>=MAX_LIG_COMP )
return;
    ++ sm->cnt;
    if ( sm->cnt>=10000 ) {
	if ( sm->cnt==10000 )
	    fprintf(stderr,"In an attempt to process the ligatures of this font, I've concluded\nthat the state machine in Apple's mort/morx table is\n(like the learned constable) too cunning to be understood.\nI shall give up on it. Your ligatures may be incomplete.\n" );
return;
    }
    sm->states_in_use[state] = true;

    if ( class==-1 ) { class_bottom = 0; class_top = sm->nClasses; }
    else { class_bottom = class; class_top = class+1; }
    for ( class=class_bottom; class<class_top; ++class ) {
	int ent = sm->data[offset+class];
	int newState = memushort(sm->data,sm->entryOffset+4*ent);
	int flags = memushort(sm->data,sm->entryOffset+4*ent+2);
	if ( flags&0x8000 )	/* Set component */
	    sm->lig_comp_classes[sm->lcp++] = class;
	if ( flags&0x3fff ) {
	    mort_figure_ligatures(sm, sm->lcp-1, flags & 0x3fff, 0);
	} else if ( flags&0x8000 )
	    follow_mort_state(sm,newState,(flags&0x4000)?class:-1);
	if ( flags&0x8000 )
	    --sm->lcp;
    }
    sm->states_in_use[state] = false;
}

static void morx_figure_ligatures(struct statemachine *sm, int lcp, int ligindex, int32 lig_offset) {
    uint32 lig;
    int i, j, lig_glyph;
    PST *pst;
    int len;

    if ( lcp<0 || sm->ligActOff+4*ligindex+3>sm->length )
return;

    lig = memlong(sm->data,sm->ligActOff+4*ligindex);
    ++ligindex;

    for ( i=0; i<sm->info->glyph_cnt; ++i ) if ( sm->classes[i]==sm->lig_comp_classes[lcp] ) {
	sm->lig_comp_glyphs[lcp] = i;
	lig_offset += memushort(sm->data,sm->compOff + 2*( ((((int32) lig)<<2)>>2) + i ) );
	if ( lig&0xc0000000 ) {
	    if ( sm->ligOff+2*lig_offset+1 > sm->length ) {
		fprintf( stderr, "Invalid ligature offset\n" );
    break;
	    }
	    lig_glyph = memushort(sm->data,sm->ligOff+2*lig_offset);
	    if ( lig_glyph>=sm->info->glyph_cnt ) {
		fprintf(stderr, "Attempt to make a ligature for glyph %d out of " );
		for ( j=lcp; j<sm->lcp; ++j )
		    fprintf(stderr,"%d ",sm->lig_comp_glyphs[j]);
		fprintf(stderr,"\n");
	    } else {
		char *comp;
		for ( len=0, j=lcp; j<sm->lcp; ++j )
		    len += strlen(sm->info->chars[sm->lig_comp_glyphs[j]]->name)+1;
		comp = galloc(len);
		*comp = '\0';
		for ( j=lcp; j<sm->lcp; ++j ) {
		    strcat(comp,sm->info->chars[sm->lig_comp_glyphs[j]]->name);
		    if ( j!=sm->lcp-1 )
			strcat(comp," ");
		}
		for ( pst=sm->info->chars[lig_glyph]->possub; pst!=NULL; pst=pst->next )
		    if ( pst->type==pst_ligature && pst->tag==sm->info->mort_subs_tag &&
			    strcmp(comp,pst->u.lig.components)==0 )
		break;
		/* There are cases where there will be multiple entries for */
		/*  the same lig. ie. if we have "ff" and "ffl" then there */
		/*  will be multiple entries for "ff" */
		if ( pst == NULL ) {
		    pst = chunkalloc(sizeof(PST));
		    pst->type = pst_ligature;
		    pst->flags = sm->info->mort_r2l ? (pst_r2l|pst_ignorecombiningmarks) : pst_ignorecombiningmarks;
		    pst->tag = sm->info->mort_subs_tag;
		    pst->script_lang_index = SLIFromInfo(sm->info,sm->info->chars[lig_glyph],DEFAULT_LANG);
		    pst->u.lig.components = comp;
		    pst->u.lig.lig = sm->info->chars[lig_glyph];
		    pst->next = sm->info->chars[lig_glyph]->possub;
		    sm->info->chars[lig_glyph]->possub = pst;
		}
	    }
	} else
	    morx_figure_ligatures(sm,lcp-1,ligindex,lig_offset);
	lig_offset -= memushort(sm->data,sm->compOff + 2*( ((((int32) lig)<<2)>>2) + i ) );
    }
}

static void follow_morx_state(struct statemachine *sm,int state,int class) {
    int class_top, class_bottom;

    if ( state<0 || state>=sm->smax || sm->states_in_use[state] || sm->lcp>=MAX_LIG_COMP )
return;
    sm->states_in_use[state] = true;

    if ( class==-1 ) { class_bottom = 0; class_top = sm->nClasses; }
    else { class_bottom = class; class_top = class+1; }
    for ( class=class_bottom; class<class_top; ++class ) {
	int ent = memushort(sm->data, sm->stateOffset + 2*(state*sm->nClasses+class) );
	int newState = memushort(sm->data,sm->entryOffset+6*ent);
	int flags = memushort(sm->data,sm->entryOffset+6*ent+2);
	int ligindex = memushort(sm->data,sm->entryOffset+6*ent+4);
	if ( flags&0x8000 )	/* Set component */
	    sm->lig_comp_classes[sm->lcp++] = class;
	if ( flags&0x2000 ) {
	    morx_figure_ligatures(sm, sm->lcp-1, ligindex, 0);
	} else if ( flags&0x8000 )
	    follow_morx_state(sm,newState,(flags&0x4000)?class:-1);
	if ( flags&0x8000 )
	    --sm->lcp;
    }
    sm->states_in_use[state] = false;
}

static void readttf_mortx_lig(FILE *ttf,struct ttfinfo *info,int ismorx,uint32 base,uint32 length) {
    uint32 here;
    struct statemachine sm;
    int first, cnt, i;

    memset(&sm,0,sizeof(sm));
    sm.info = info;
    here = ftell(ttf);
    length -= here-base;
    sm.data = galloc(length);
    sm.length = length;
    if ( fread(sm.data,1,length,ttf)!=length ) {
	free(sm.data);
	fprintf( stderr, "Bad mort ligature table. Not long enough\n");
return;
    }
    fseek(ttf,here,SEEK_SET);
    if ( ismorx ) {
	sm.nClasses = memlong(sm.data,0);
	sm.classOffset = memlong(sm.data,sizeof(long));
	sm.stateOffset = memlong(sm.data,2*sizeof(long));
	sm.entryOffset = memlong(sm.data,3*sizeof(long));
	sm.ligActOff = memlong(sm.data,4*sizeof(long));
	sm.compOff = memlong(sm.data,5*sizeof(long));
	sm.ligOff = memlong(sm.data,6*sizeof(long));
	fseek(ttf,here+sm.classOffset,SEEK_SET);
	sm.classes = info->morx_classes = galloc(info->glyph_cnt*sizeof(uint16));
	for ( i=0; i<info->glyph_cnt; ++i )
	    sm.classes[i] = 1;			/* Out of bounds */
	readttf_applelookup(ttf,info,
		mortclass_apply_values,mortclass_apply_value,NULL,NULL);
	sm.smax = length/(2*sm.nClasses);
	sm.states_in_use = gcalloc(sm.smax,sizeof(uint8));
	follow_morx_state(&sm,0,-1);
    } else {
	sm.nClasses = memushort(sm.data,0);
	sm.classOffset = memushort(sm.data,sizeof(uint16));
	sm.stateOffset = memushort(sm.data,2*sizeof(uint16));
	sm.entryOffset = memushort(sm.data,3*sizeof(uint16));
	sm.ligActOff = memushort(sm.data,4*sizeof(uint16));
	sm.compOff = memushort(sm.data,5*sizeof(uint16));
	sm.ligOff = memushort(sm.data,6*sizeof(uint16));
	sm.classes = galloc(info->glyph_cnt*sizeof(uint16));
	for ( i=0; i<info->glyph_cnt; ++i )
	    sm.classes[i] = 1;			/* Out of bounds */
	first = memushort(sm.data,sm.classOffset);
	cnt = memushort(sm.data,sm.classOffset+sizeof(uint16));
	for ( i=0; i<cnt; ++i )
	    sm.classes[first+i] = sm.data[sm.classOffset+2*sizeof(uint16)+i];
	sm.smax = length/sm.nClasses;
	sm.states_in_use = gcalloc(sm.smax,sizeof(uint8));
	follow_mort_state(&sm,sm.stateOffset,-1);
    }
    free(sm.data);
    free(sm.states_in_use);
    free(sm.classes);
}

static uint32 readmortchain(FILE *ttf,struct ttfinfo *info, uint32 base, int ismorx) {
    uint32 chain_len, nfeatures, nsubtables;
    uint32 enable_flags, disable_flags, flags;
    int featureType, featureSetting;
    int i,j,k,l;
    uint32 length, coverage;
    uint32 here;
    uint32 tag;
    struct tagmaskfeature { uint32 tag, enable_flags; } tmf[32];
    int r2l;

    /* default flags = */ getlong(ttf);
    chain_len = getlong(ttf);
    if ( ismorx ) {
	nfeatures = getlong(ttf);
	nsubtables = getlong(ttf);
    } else {
	nfeatures = getushort(ttf);
	nsubtables = getushort(ttf);
    }

    k = 0;
    for ( i=0; i<nfeatures; ++i ) {
	featureType = getushort(ttf);
	featureSetting = getushort(ttf);
	enable_flags = getlong(ttf);
	disable_flags = getlong(ttf);
	tag = MacFeatureToOTTag(featureType,featureSetting);
	if ( enable_flags!=0 && tag!=0 && k<32 ) {
	    tmf[k].tag = tag;
	    tmf[k++].enable_flags = enable_flags;
	}
    }
    if ( k==0 )
return( chain_len );

    for ( i=0; i<nsubtables; ++i ) {
	here = ftell(ttf);
	if ( ismorx ) {
	    length = getlong(ttf);
	    coverage = getlong(ttf);
	} else {
	    length = getushort(ttf);
	    coverage = getushort(ttf);
	    coverage = ((coverage&0xe000)<<16) | (coverage&7);	/* convert to morx format */
	}
	r2l = (coverage & 0x40000000)? 1 : 0;
	flags = getlong(ttf);
	for ( j=k-1; j>=0 && !(flags&tmf[j].enable_flags); --j );
	if ( j>=0 ) {
	    info->mort_subs_tag = tmf[j].tag;
	    info->mort_r2l = r2l;
	    for ( l=0; info->feats[0][l]!=0; ++l )
		if ( info->feats[0][l]==tmf[j].tag )
	    break;
	    if ( info->feats[0][l]==0 && l<info->mort_max ) {
		info->feats[0][l] = tmf[j].tag;
		info->feats[0][l+1] = 0;
	    }
	    switch( coverage&0xff ) {
	      case 0:
		/* Indic rearangement */
	      break;
	      case 1:
		/* contextual glyph substitution */
	      break;
	      case 2:	/* ligature substitution */
		readttf_mortx_lig(ttf,info,ismorx,here,length);
	      break;
	      case 4:	/* non-contextual glyph substitutions */
		/* Another case of that isn't specified in the docs */
		/* It seems unlikely that (in morx at least!) the base for */
		/*  offsets in the lookup table should be the start of the */
		/*  mor[tx] table, it would make more sense for it to be the*/
		/*  start of the lookup table instead (for format 4 lookups) */
		readttf_applelookup(ttf,info,
			mort_apply_values,mort_apply_value,NULL,NULL);
	      break;
	      case 5:
		/* contextual glyph insertion */
	      break;
	    }
	}
	fseek(ttf, here+length, SEEK_SET );
    }

return( chain_len );
}

static void readttfmort(FILE *ttf,struct ttfinfo *info) {
    uint32 base = info->morx_start!=0 ? info->morx_start : info->mort_start;
    uint32 here, len;
    int ismorx;
    int32 version;
    int i, nchains;

    fseek(ttf,base,SEEK_SET);
    version = getlong(ttf);
    ismorx = version == 0x00020000;
    if ( version!=0x00010000 && version != 0x00020000 )
return;
    nchains = getlong(ttf);
    info->mort_max = nchains*33;		/* Maximum of one feature per bit ? */
    info->feats[0] = galloc((info->mort_max+1)*sizeof(uint32));
    info->feats[0][0] = 0;
    for ( i=0; i<nchains; ++i ) {
	here = ftell(ttf);
	len = readmortchain(ttf,info,base,ismorx);
	fseek(ttf,here+len,SEEK_SET);
    }
}

static void UnfigureControls(Spline *spline,BasePoint *pos) {
    pos->x = rint( (spline->splines[0].c+2*spline->splines[0].d)/2 );
    pos->y = rint( (spline->splines[1].c+2*spline->splines[1].d)/2 );
}

static int ttfFindPointInSC(SplineChar *sc,int pnum,BasePoint *pos) {
    SplineSet *ss;
    SplinePoint *sp;
    int last=0, ret;
    RefChar *refs;

    for ( ss = sc->splines; ss!=NULL; ss=ss->next ) {
	for ( sp=ss->first; ; ) {
	    if ( sp->ttfindex==pnum ) {
		*pos = sp->me;
return(-1);
	    } else if ( !sp->nonextcp && last+1==pnum ) {
		/* fix this up to be 2 degree bezier control point */
		UnfigureControls(sp->next,pos);
return( -1 );
	    }
	    if ( sp->ttfindex==0xffff )
		++last;
	    else if ( sp->nonextcp )
		last = sp->ttfindex;
	    else
		last = sp->ttfindex+1;
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
    }
    for ( refs=sc->refs; refs!=NULL; refs=refs->next ) {
	ret = ttfFindPointInSC(refs->sc,pnum-last,pos);
	if ( ret==-1 )
return( -1 );
	last += ret;
    }
return( last );		/* Count of number of points in the character */
}

static void ttfPointMatch(SplineChar *sc,RefChar *rf) {
    BasePoint sofar, inref;

    if ( ttfFindPointInSC(sc,rf->transform[4],&sofar)!=-1 ||
	    ttfFindPointInSC(rf->sc,rf->transform[5],&inref)!=-1 ) {
	fprintf( stderr, "Could not match points in composite glyph (%g to %g) when adding %s to %s\n",
		rf->transform[4], rf->transform[5], rf->sc->name, sc->name);
return;
    }
    rf->transform[4] = sofar.x-inref.x;
    rf->transform[5] = sofar.y-inref.y;
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
	    if ( ref->point_match )
		ttfPointMatch(info->chars[i],ref);
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

static void TtfCopyTableBlindly(struct ttfinfo *info,FILE *ttf,
	uint32 start,uint32 len,uint32 tag) {
    struct ttf_table *tab;

    if ( start==0 || len==0 )
return;
    tab = chunkalloc(sizeof(struct ttf_table));
    tab->tag = tag;
    tab->len = len;
    tab->data = galloc(len);
    fseek(ttf,start,SEEK_SET);
    fread(tab->data,1,len,ttf);
    tab->next = info->tabs;
    info->tabs = tab;
}

static int readttf(FILE *ttf, struct ttfinfo *info, char *filename) {
    char *oldloc;

    GProgressChangeStages(3);
    if ( !readttfheader(ttf,info,filename)) {
return( 0 );
    }
    oldloc = setlocale(LC_NUMERIC,"C");		/* TrueType doesn't need this but opentype dictionaries do */
    readttfpreglyph(ttf,info);
    GProgressChangeTotal(info->glyph_cnt);

    /* If font only contains bitmaps, then only read bitmaps */
    if ( (info->glyphlocations_start==0 || info->glyph_length==0) &&
	    info->cff_start==0 &&
	    info->bitmapdata_start!=0 && info->bitmaploc_start!=0 )
	info->onlystrikes = true;

    if ( !info->onlystrikes &&
	    info->glyphlocations_start!=0 && info->glyph_start!=0 &&
	    info->cff_start!=0 ) {
	static int buts[] = { _STR_TTFGlyf, _STR_OTFCFF, _STR_Cancel, 0 };
	int choice = GWidgetAskR(_STR_PickFont,buts,0,2,_STR_GlyfAndCFF);
	if ( choice==2 )
return( 0 );
	else if ( choice==0 )
	    info->cff_start=0;
	else
	    info->glyph_start = info->glyphlocations_start = 0;
    }

    if ( info->onlystrikes ) {
	info->chars = gcalloc(info->glyph_cnt+1,sizeof(SplineChar *));
	info->to_order2 = new_fonts_are_order2;
    } else if ( info->glyphlocations_start!=0 && info->glyph_start!=0 ) {
	info->to_order2 = (!loaded_fonts_same_as_new ||
		(loaded_fonts_same_as_new && new_fonts_are_order2));
	readttfglyphs(ttf,info);
    } else if ( info->cff_start!=0 ) {
	info->to_order2 = (loaded_fonts_same_as_new && new_fonts_are_order2);
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
    if ( info->onlystrikes && info->bitmaps==NULL ) {
	free(info->chars);
return( 0 );
    }
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
    if ( info->gdef_start!=0 )		/* ligature caret positioning info */
	readttfgdef(ttf,info);
    else {
	if ( info->prop_start!=0 )
	    readttfprop(ttf,info);
	if ( info->lcar_start!=0 )
	    readttflcar(ttf,info);
    }
    if ( info->gpos_start!=0 )		/* kerning info may live in the gpos table too */
	readttfgpossub(ttf,info,true);
    else {
	if ( info->kern_start!=0 )
	    readttfkerns(ttf,info);
	if ( info->opbd_start!=0 )
	    readttfopbd(ttf,info);
    }
    if ( info->gsub_start!=0 )
	readttfgpossub(ttf,info,false);
    else {
	/* We will default the gsub table later... */;
	if ( info->morx_start!=0 || info->mort_start!=0 )
	    readttfmort(ttf,info);
    }
    if ( info->to_order2 ) {
	    /* Yes, even though we've looked at maxp already, let's make a blind */
	    /*  copy too for those fields we can't compute on our own */
	    /* Like number of instructions, etc. */
	TtfCopyTableBlindly(info,ttf,info->maxp_start,info->maxp_len,CHR('m','a','x','p'));
	TtfCopyTableBlindly(info,ttf,info->cvt_start,info->cvt_len,CHR('c','v','t',' '));
	TtfCopyTableBlindly(info,ttf,info->fpgm_start,info->fpgm_len,CHR('f','p','g','m'));
	TtfCopyTableBlindly(info,ttf,info->prep_start,info->prep_len,CHR('p','r','e','p'));
    }
    if ( info->pfed_start!=0 )
	pfed_read(ttf,info);
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
	uenc = UniFromName(sc->name);
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
    SplineFont *sf, *_sf;
    int i,k;
    BDFFont *bdf;
    struct table_ordering *ord;

    sf = SplineFontEmpty();
    sf->display_size = -default_fv_font_size;
    sf->display_antialias = default_fv_antialias;
    sf->fontname = info->fontname;
    sf->fullname = info->fullname;
    sf->familyname = info->familyname;
    sf->onlybitmaps = info->onlystrikes;
    sf->order2 = info->to_order2;
    sf->comments = info->fontcomments;
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
    sf->uniqueid = info->uniqueid;
    sf->pfminfo = info->pfminfo;
    sf->names = info->names;
    sf->anchor = info->ahead;
    sf->kerns = info->khead;
    sf->vkerns = info->vkhead;
    sf->script_lang = info->script_lang;
    sf->ttf_tables = info->tabs;
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

    for ( i=0; i<info->glyph_cnt; ++i ) if ( info->chars[i]!=NULL ) {
	SCOrderAP(info->chars[i]);
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
    if ( info->gsub_start==0 && info->mort_start==0 && info->morx_start==0 ) {
	/* Get default ligature values, etc. */
	k=0;
	do {
	    _sf = k<sf->subfontcnt?sf->subfonts[k]:sf;
	    for ( i=0; i<sf->charcnt; ++i ) {
		if ( _sf->chars[i]!=NULL )		/* Might be null in ttc files */
		    SCLigDefault(_sf->chars[i]);
	    }
	    ++k;
	} while ( k<sf->subfontcnt );
    }

    if ( info->feats[0]!=NULL ) {
	ord = chunkalloc(sizeof(struct table_ordering));
	ord->table_tag = CHR('G','S','U','B');		/* or mort/morx */
	ord->ordered_features = info->feats[0];
	sf->orders = ord;
    }
    if ( info->feats[1]!=NULL ) {
	ord = chunkalloc(sizeof(struct table_ordering));
	ord->table_tag = CHR('G','P','O','S');
	ord->ordered_features = info->feats[1];
	ord->next = sf->orders;
	sf->orders = ord;
    }

    otf_orderlangs(sf);		/* I thought these had to be ordered, but it seems I was wrong. But I depend on the order, so I enforce it here */

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
    char *temp=filename, *pt, *lparen;

    pt = strrchr(filename,'/');
    if ( pt==NULL ) pt = filename;
    if ( (lparen=strchr(pt,'('))!=NULL && strchr(lparen,')')!=NULL ) {
	temp = copy(filename);
	pt = temp + (lparen-filename);
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
