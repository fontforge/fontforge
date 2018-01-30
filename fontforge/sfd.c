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

#include "sfd.h"

#include "autohint.h"
#include "bvedit.h"
#include "cvimages.h"
#include "cvundoes.h"
#include "encoding.h"
#include "fontforge.h"
#include "fvfonts.h"
#include "http.h"
#include "lookups.h"
#include "mem.h"
#include "namelist.h"
#include "parsettf.h"
#include "psread.h"
#include "splinefont.h"
#include "splinefill.h"
#include "splinesaveafm.h"
#include "splineutil.h"
#include "splineutil2.h"
#include "tottfgpos.h"
#include "ttfinstrs.h"
#include "baseviews.h"
#include "views.h"
#include <gdraw.h>
#include <gutils.h>
#include <ustring.h>
#include <math.h>
#include <utype.h>
#include <unistd.h>
#include <locale.h>
#include <gfile.h>
#include <gwidget.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>		/* For NAME_MAX or _POSIX_NAME_MAX */

#include <ffglib.h>


#ifndef NAME_MAX
# ifndef  _POSIX_NAME_MAX
#  define _POSIX_NAME_MAX 512
# endif
# define NAME_MAX _POSIX_NAME_MAX
#endif

int UndoRedoLimitToSave = 0;
int UndoRedoLimitToLoad = 0;

static const char *joins[] = { "miter", "round", "bevel", "inher", NULL };
static const char *caps[] = { "butt", "round", "square", "inher", NULL };
static const char *spreads[] = { "pad", "reflect", "repeat", NULL };

int prefRevisionsToRetain = 32;


#define SFD_PTFLAG_TYPE_MASK          0x3
#define SFD_PTFLAG_IS_SELECTED        0x4
#define SFD_PTFLAG_NEXTCP_IS_DEFAULT  0x8
#define SFD_PTFLAG_PREVCP_IS_DEFAULT  0x10
#define SFD_PTFLAG_ROUND_IN_X         0x20
#define SFD_PTFLAG_ROUND_IN_Y         0x40
#define SFD_PTFLAG_INTERPOLATE        0x80
#define SFD_PTFLAG_INTERPOLATE_NEVER  0x100
#define SFD_PTFLAG_PREV_EXTREMA_MARKED_ACCEPTABLE  0x200
#define SFD_PTFLAG_FORCE_OPEN_PATH    0x400




/* I will retain this list in case there are still some really old sfd files */
/*  including numeric encodings.  This table maps them to string encodings */
static const char *charset_names[] = {
    "custom",
    "iso8859-1", "iso8859-2", "iso8859-3", "iso8859-4", "iso8859-5",
    "iso8859-6", "iso8859-7", "iso8859-8", "iso8859-9", "iso8859-10",
    "iso8859-11", "iso8859-13", "iso8859-14", "iso8859-15", "iso8859-16",
    "koi8-r",
    "jis201",
    "win", "mac", "symbol", "zapfding", "adobestandard",
    "jis208", "jis212", "ksc5601", "gb2312", "big5", "big5hkscs", "johab",
    "unicode", "unicode4", "sjis", "wansung", "gb2312pk", NULL};

static const char *unicode_interp_names[] = { "none", "adobe", "greek",
    "japanese", "tradchinese", "simpchinese", "korean", "ams", NULL };

/* sfdir files and extensions */
#define FONT_PROPS	"font.props"
#define STRIKE_PROPS	"strike.props"
#define EXT_CHAR	'.'
#define GLYPH_EXT	".glyph"
#define BITMAP_EXT	".bitmap"
#define STRIKE_EXT	".strike"
#define SUBFONT_EXT	".subfont"
#define INSTANCE_EXT	".instance"

signed char inbase64[256] = {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
        -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
        -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};
static char base64[64] = {
 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

static const char *end_tt_instrs = "EndTTInstrs";
static void SFDDumpRefs(FILE *sfd,RefChar *refs, int *newgids);
static RefChar *SFDGetRef(FILE *sfd, int was_enc);
static void SFDDumpImage(FILE *sfd,ImageList *img);
static AnchorPoint *SFDReadAnchorPoints(FILE *sfd,SplineChar *sc,AnchorPoint** alist, AnchorPoint *lastap);
static void SFDDumpTtfInstrs(FILE *sfd,SplineChar *sc);
static void SFDDumpTtfInstrsExplicit(FILE *sfd,uint8 *ttf_instrs, int16 ttf_instrs_len );
static void SFDDumpHintList(FILE *sfd,const char *key, StemInfo *h);
static void SFDDumpDHintList( FILE *sfd,const char *key, DStemInfo *d );
static StemInfo *SFDReadHints(FILE *sfd);
static DStemInfo *SFDReadDHints( SplineFont *sf,FILE *sfd,int old );

static int PeekMatch(FILE *stream, const char * target) {
  // This returns 1 if target matches the next characters in the stream.
  int pos1 = 0;
  int lastread = getc(stream);
  while (target[pos1] != '\0' && lastread != EOF && lastread == target[pos1]) {
    pos1 ++; lastread = getc(stream);
  }
  
  int rewind_amount = pos1 + ((lastread == EOF) ? 0 : 1);
  fseek(stream, -rewind_amount, SEEK_CUR);
  return (target[pos1] == '\0');
}

static void utf7_encode(FILE *sfd,long ch) {

    putc(base64[(ch>>18)&0x3f],sfd);
    putc(base64[(ch>>12)&0x3f],sfd);
    putc(base64[(ch>>6)&0x3f],sfd);
    putc(base64[ch&0x3f],sfd);
}

static char *base64_encode(char *ostr, long ch) {

    *ostr++ = base64[(ch>>18)&0x3f];
    *ostr++ = base64[(ch>>12)&0x3f];
    *ostr++ = base64[(ch>> 6)&0x3f];
    *ostr++ = base64[(ch    )&0x3f];
return( ostr );
}

static void SFDDumpUTF7Str(FILE *sfd, const char *_str) {
    int ch, prev_cnt=0, prev=0, in=0;
    const unsigned char *str = (const unsigned char *) _str;

    putc('"',sfd);
    if ( str!=NULL ) while ( (ch = *str++)!='\0' ) {
	/* Convert from utf8 to ucs4 */
	if ( ch<=127 )
	    /* Done */;
	else if ( ch<=0xdf && *str!='\0' ) {
	    ch = ((ch&0x1f)<<6) | (*str++&0x3f);
	} else if ( ch<=0xef && *str!='\0' && str[1]!='\0' ) {
	    ch = ((ch&0xf)<<12) | ((str[0]&0x3f)<<6) | (str[1]&0x3f);
	    str += 2;
	} else if ( *str!='\0' && str[1]!='\0' && str[2]!='\0' ) {
	    int w = ( ((ch&0x7)<<2) | ((str[0]&0x30)>>4) )-1;
	    int s1, s2;
	    s1 = (w<<6) | ((str[0]&0xf)<<2) | ((str[1]&0x30)>>4);
	    s2 = ((str[1]&0xf)<<6) | (str[2]&0x3f);
	    ch = (s1*0x400)+s2 + 0x10000;
	    str += 3;
	} else {
	    /* illegal */
	}
	if ( ch<127 && ch!='\n' && ch!='\r' && ch!='\\' && ch!='~' &&
		ch!='+' && ch!='=' && ch!='"' ) {
	    if ( prev_cnt!=0 ) {
		prev<<= (prev_cnt==1?16:8);
		utf7_encode(sfd,prev);
		prev_cnt=prev=0;
	    }
	    if ( in ) {
		if ( inbase64[ch]!=-1 || ch=='-' )
		    putc('-',sfd);
		in = 0;
	    }
	    putc(ch,sfd);
	} else if ( ch=='+' && !in ) {
	    putc('+',sfd);
	    putc('-',sfd);
	} else if ( prev_cnt== 0 ) {
	    if ( !in ) {
		putc('+',sfd);
		in = 1;
	    }
	    prev = ch;
	    prev_cnt = 2;		/* 2 bytes */
	} else if ( prev_cnt==2 ) {
	    prev<<=8;
	    prev += (ch>>8)&0xff;
	    utf7_encode(sfd,prev);
	    prev = (ch&0xff);
	    prev_cnt=1;
	} else {
	    prev<<=16;
	    prev |= ch;
	    utf7_encode(sfd,prev);
	    prev_cnt = prev = 0;
	}
    }
    if ( prev_cnt==2 ) {
	prev<<=8;
	utf7_encode(sfd,prev);
    } else if ( prev_cnt==1 ) {
	prev<<=16;
	utf7_encode(sfd,prev);
    }
    putc('"',sfd);
}


char *utf8toutf7_copy(const char *_str) {
    int ch, prev_cnt=0, prev=0, in=0;
    const unsigned char *str = (const unsigned char *) _str;
    int i, len;
    char *ret=NULL, *ostr=NULL;

    if ( str==NULL )
return( NULL );
    for ( i=0; i<2; ++i ) {
	str = (const unsigned char *) _str;
	len= prev_cnt= prev= in=0;
	while ( (ch = *str++)!='\0' ) {
	    /* Convert from utf8 to ucs2 */
	    if ( ch<=127 )
		/* Done */;
	    else if ( ch<=0xdf && *str!='\0' ) {
		ch = ((ch&0x1f)<<6) | (*str++&0x3f);
	    } else if ( ch<=0xef && *str!='\0' && str[1]!='\0' ) {
		ch = ((ch&0xf)<<12) | ((str[0]&0x3f)<<6) | (str[1]&0x3f);
		str += 2;
	    } else if ( *str!='\0' && str[1]!='\0' && str[2]!='\0' ) {
		int w = ( ((ch&0x7)<<2) | ((str[0]&0x30)>>4) )-1;
		int s1, s2;
		s1 = (w<<6) | ((str[0]&0xf)<<2) | ((str[1]&0x30)>>4);
		s2 = ((str[1]&0xf)<<6) | (str[2]&0x3f);
		ch = (s1*0x400)+s2 + 0x10000;
		str += 3;
	    } else {
		/* illegal */
	    }
	    if ( ch<127 && ch!='\n' && ch!='\r' && ch!='\\' && ch!='~' &&
		    ch!='+' && ch!='=' && ch!='"' ) {
		if ( prev_cnt!=0 ) {
		    if ( i ) {
			prev<<= (prev_cnt==1?16:8);
			ostr = base64_encode(ostr,prev);
			prev_cnt=prev=0;
		    } else
			len += 4;
		}
		if ( in ) {
		    if ( inbase64[ch]!=-1 || ch=='-' ) {
			if ( i )
			    *ostr++ = '-';
			else
			    ++len;
		    }
		    in = 0;
		}
		if ( i )
		    *ostr++ = ch;
		else
		    ++len;
	    } else if ( ch=='+' && !in ) {
		if ( i ) {
		    *ostr++ = '+';
		    *ostr++ = '-';
		} else
		    len += 2;
	    } else if ( prev_cnt== 0 ) {
		if ( !in ) {
		    if ( i )
			*ostr++ = '+';
		    else
			++len;
		    in = 1;
		}
		prev = ch;
		prev_cnt = 2;		/* 2 bytes */
	    } else if ( prev_cnt==2 ) {
		prev<<=8;
		prev += (ch>>8)&0xff;
		if ( i ) {
		    ostr = base64_encode(ostr,prev);
		    prev_cnt=prev=0;
		} else
		    len += 4;
		prev = (ch&0xff);
		prev_cnt=1;
	    } else {
		prev<<=16;
		prev |= ch;
		if ( i ) {
		    ostr = base64_encode(ostr,prev);
		    prev_cnt=prev=0;
		} else
		    len += 4;
		prev_cnt = prev = 0;
	    }
	}
	if ( prev_cnt==2 ) {
	    prev<<=8;
	    if ( i ) {
		ostr = base64_encode(ostr,prev);
		prev_cnt=prev=0;
	    } else
		len += 4;
	} else if ( prev_cnt==1 ) {
	    prev<<=16;
	    if ( i ) {
		ostr = base64_encode(ostr,prev);
		prev_cnt=prev=0;
	    } else
		len += 4;
	}
	if ( in ) {
	    if ( i )
		*ostr++ = '-';
	    else
		++len;
	}
	if ( i==0 )
	    ostr = ret = malloc(len+1);
    }
    *ostr = '\0';
return( ret );
}

/* Long lines can be broken by inserting \\\n (backslash newline) */
/*  into the line. I don't think this is ever ambiguous as I don't */
/*  think a line can end with backslash */
/* UPDATE: it can... that's handled in getquotedeol() below. */
static int nlgetc(FILE *sfd) {
    int ch, ch2;

    ch=getc(sfd);
    if ( ch!='\\' )
return( ch );
    ch2 = getc(sfd);
    if ( ch2=='\n' )
return( nlgetc(sfd));
    ungetc(ch2,sfd);
return( ch );
}

static char *SFDReadUTF7Str(FILE *sfd) {
    char *buffer = NULL, *pt, *end = NULL;
    int ch1, ch2, ch3, ch4, done, c;
    int prev_cnt=0, prev=0, in=0;

    ch1 = nlgetc(sfd);
    while ( isspace(ch1) && ch1!='\n' && ch1!='\r') ch1 = nlgetc(sfd);
    if ( ch1=='\n' || ch1=='\r' )
	ungetc(ch1,sfd);
    if ( ch1!='"' )
return( NULL );
    pt = 0;
    while ( (ch1=nlgetc(sfd))!=EOF && ch1!='"' ) {
	done = 0;
	if ( !done && !in ) {
	    if ( ch1=='+' ) {
		ch1 = nlgetc(sfd);
		if ( ch1=='-' ) {
		    ch1 = '+';
		    done = true;
		} else {
		    in = true;
		    prev_cnt = 0;
		}
	    } else
		done = true;
	}
	if ( !done ) {
	    if ( ch1=='-' ) {
		in = false;
	    } else if ( inbase64[ch1]==-1 ) {
		in = false;
		done = true;
	    } else {
		ch1 = inbase64[ch1];
		ch2 = inbase64[c = nlgetc(sfd)];
		if ( ch2==-1 ) {
		    ungetc(c, sfd);
		    ch2 = ch3 = ch4 = 0;
		} else {
		    ch3 = inbase64[c = nlgetc(sfd)];
		    if ( ch3==-1 ) {
			ungetc(c, sfd);
			ch3 = ch4 = 0;
		    } else {
			ch4 = inbase64[c = nlgetc(sfd)];
			if ( ch4==-1 ) {
			    ungetc(c, sfd);
			    ch4 = 0;
			}
		    }
		}
		ch1 = (ch1<<18) | (ch2<<12) | (ch3<<6) | ch4;
		if ( prev_cnt==0 ) {
		    prev = ch1&0xff;
		    ch1 >>= 8;
		    prev_cnt = 1;
		} else /* if ( prev_cnt == 1 ) */ {
		    ch1 |= (prev<<24);
		    prev = (ch1&0xffff);
		    ch1 = (ch1>>16)&0xffff;
		    prev_cnt = 2;
		}
		done = true;
	    }
	}
	if ( pt+10>=end ) {
	    if ( buffer==NULL ) {
		pt = buffer = malloc(400);
		end = buffer+400;
	    } else if (pt) {
		char *temp = realloc(buffer,end-buffer+400);
		pt = temp+(pt-buffer);
		end = temp+(end-buffer+400);
		buffer = temp;
	    }
	}
	if ( pt && done )
	    pt = utf8_idpb(pt,ch1,0);
	if ( prev_cnt==2 ) {
	    prev_cnt = 0;
	    if ( pt && prev!=0 )
		pt = utf8_idpb(pt,prev,0);
	}
	if ( pt==0 ) {
	    free(buffer);
	    return( NULL );
	}
    }
    if ( buffer==NULL )
return( NULL );
    *pt = '\0';
    pt = copy(buffer);
    free(buffer );
return( pt );
}

char *utf7toutf8_copy(const char *_str) {
    char *buffer = NULL, *pt, *end = NULL;
    int ch1, ch2, ch3, ch4, done;
    int prev_cnt=0, prev=0, in=0;
    const char *str = _str;

    if ( str==NULL )
return( NULL );
    buffer = pt = malloc(400);
    end = pt+400;
    while ( (ch1=*str++)!='\0' ) {
	done = 0;
	if ( !done && !in ) {
	    if ( ch1=='+' ) {
		ch1=*str++;
		if ( ch1=='-' ) {
		    ch1 = '+';
		    done = true;
		} else {
		    in = true;
		    prev_cnt = 0;
		}
	    } else
		done = true;
	}
	if ( !done ) {
	    if ( ch1=='-' ) {
		in = false;
	    } else if ( inbase64[ch1]==-1 ) {
		in = false;
		done = true;
	    } else {
		ch1 = inbase64[ch1];
		ch2 = inbase64[(unsigned char) *str++];
		if ( ch2==1 ) {
		    --str;
		    ch2 = ch3 = ch4 = 0;
		} else {
		    ch3 = inbase64[(unsigned char) *str++];
		    if ( ch3==-1 ) {
			--str;
			ch3 = ch4 = 0;
		    } else {
			ch4 = inbase64[(unsigned char) *str++];
			if ( ch4==-1 ) {
			    --str;
			    ch4 = 0;
			}
		    }
		}
		ch1 = (ch1<<18) | (ch2<<12) | (ch3<<6) | ch4;
		if ( prev_cnt==0 ) {
		    prev = ch1&0xff;
		    ch1 >>= 8;
		    prev_cnt = 1;
		} else /* if ( prev_cnt == 1 ) */ {
		    ch1 |= (prev<<24);
		    prev = (ch1&0xffff);
		    ch1 = (ch1>>16)&0xffff;
		    prev_cnt = 2;
		}
		done = true;
	    }
	}
	if ( pt+10>=end ) {
	    char *temp = realloc(buffer,end-buffer+400);
	    pt = temp+(pt-buffer);
	    end = temp+(end-buffer+400);
	    buffer = temp;
	}
	if ( pt && done )
	    pt = utf8_idpb(pt,ch1,0);
	if ( prev_cnt==2 ) {
	    prev_cnt = 0;
	    if ( pt && prev!=0 )
		pt = utf8_idpb(pt,prev,0);
	}
	if ( pt==0 ) {
	    free(buffer);
	    return( NULL );
	}
    }
    *pt = '\0';
    pt = copy(buffer);
    free(buffer );
return( pt );
}

struct enc85 {
    FILE *sfd;
    unsigned char sofar[4];
    int pos;
    int ccnt;
};

static void SFDEnc85(struct enc85 *enc,int ch) {
    enc->sofar[enc->pos++] = ch;
    if ( enc->pos==4 ) {
	unsigned int val = (enc->sofar[0]<<24)|(enc->sofar[1]<<16)|(enc->sofar[2]<<8)|enc->sofar[3];
	if ( val==0 ) {
	    fputc('z',enc->sfd);
	    ++enc->ccnt;
	} else {
	    int ch2, ch3, ch4, ch5;
	    ch5 = val%85;
	    val /= 85;
	    ch4 = val%85;
	    val /= 85;
	    ch3 = val%85;
	    val /= 85;
	    ch2 = val%85;
	    val /= 85;
	    fputc('!'+val,enc->sfd);
	    fputc('!'+ch2,enc->sfd);
	    fputc('!'+ch3,enc->sfd);
	    fputc('!'+ch4,enc->sfd);
	    fputc('!'+ch5,enc->sfd);
	    enc->ccnt += 5;
	    if ( enc->ccnt > 70 ) { fputc('\n',enc->sfd); enc->ccnt=0; }
	}
	enc->pos = 0;
    }
}

static void SFDEnc85EndEnc(struct enc85 *enc) {
    int i;
    int ch2, ch3, ch4, ch5;
    unsigned val;
    if ( enc->pos==0 )
return;
    for ( i=enc->pos; i<4; ++i )
	enc->sofar[i] = 0;
    val = (enc->sofar[0]<<24)|(enc->sofar[1]<<16)|(enc->sofar[2]<<8)|enc->sofar[3];
    if ( val==0 ) {
	fputc('z',enc->sfd);
    } else {
	ch5 = val%85;
	val /= 85;
	ch4 = val%85;
	val /= 85;
	ch3 = val%85;
	val /= 85;
	ch2 = val%85;
	val /= 85;
	fputc('!'+val,enc->sfd);
	fputc('!'+ch2,enc->sfd);
	fputc('!'+ch3,enc->sfd);
	fputc('!'+ch4,enc->sfd);
	fputc('!'+ch5,enc->sfd);
    }
    enc->pos = 0;
}

static void SFDDumpHintMask(FILE *sfd,HintMask *hintmask) {
    unsigned i, j;

    for ( i=HntMax/8-1; i>0; --i )
	if ( (*hintmask)[i]!=0 )
    break;
    for ( j=0; /* j <= i, but that might never be true, so we test j == i at end of loop */ ; ++j ) {
	if ( ((*hintmask)[j]>>4)<10 )
	    putc('0'+((*hintmask)[j]>>4),sfd);
	else
	    putc('a'-10+((*hintmask)[j]>>4),sfd);
	if ( ((*hintmask)[j]&0xf)<10 )
	    putc('0'+((*hintmask)[j]&0xf),sfd);
	else
	    putc('a'-10+((*hintmask)[j]&0xf),sfd);
        if (j == i) break;
    }
}

static void SFDDumpSplineSet(FILE *sfd,SplineSet *spl) {
    SplinePoint *first, *sp;
    int order2 = spl->first->next==NULL || spl->first->next->order2;

    for ( ; spl!=NULL; spl=spl->next ) {
	first = NULL;
	for ( sp = spl->first; ; sp=sp->next->to ) {
#ifndef FONTFORGE_CONFIG_USE_DOUBLE
	    if ( first==NULL )
		fprintf( sfd, "%g %g m ", (double) sp->me.x, (double) sp->me.y );
	    else if ( sp->prev->islinear && sp->noprevcp )		/* Don't use known linear here. save control points if there are any */
		fprintf( sfd, " %g %g l ", (double) sp->me.x, (double) sp->me.y );
	    else
		fprintf( sfd, " %g %g %g %g %g %g c ",
			(double) sp->prev->from->nextcp.x, (double) sp->prev->from->nextcp.y,
			(double) sp->prevcp.x, (double) sp->prevcp.y,
			(double) sp->me.x, (double) sp->me.y );
#else
	    if ( first==NULL )
		fprintf( sfd, "%.12g %.12g m ", (double) sp->me.x, (double) sp->me.y );
	    else if ( sp->prev->islinear && sp->noprevcp )		/* Don't use known linear here. save control points if there are any */
		fprintf( sfd, " %.12g %.12g l ", (double) sp->me.x, (double) sp->me.y );
	    else
		fprintf( sfd, " %.12g %.12g %.12g %.12g %.12g %.12g c ",
			(double) sp->prev->from->nextcp.x, (double) sp->prev->from->nextcp.y,
			(double) sp->prevcp.x, (double) sp->prevcp.y,
			(double) sp->me.x, (double) sp->me.y );
#endif
	    int ptflags = 0;
	    ptflags = sp->pointtype|(sp->selected<<2)|
		(sp->nextcpdef<<3)|(sp->prevcpdef<<4)|
		(sp->roundx<<5)|(sp->roundy<<6)|
		(sp->ttfindex==0xffff?(1<<7):0)|
		(sp->dontinterpolate<<8)|
		((sp->prev && sp->prev->acceptableextrema)<<9);

	    // Last point in the splineset, and we are an open path.
	    if( !sp->next
		&& spl->first && !spl->first->prev )
	    {
		ptflags |= SFD_PTFLAG_FORCE_OPEN_PATH;
	    }


	    fprintf(sfd, "%d", ptflags );
	    if ( order2 ) {
		if ( sp->ttfindex!=0xfffe && sp->nextcpindex!=0xfffe ) {
		    putc(',',sfd);
		    if ( sp->ttfindex==0xffff )
			fprintf(sfd,"-1");
		    else if ( sp->ttfindex!=0xfffe )
			fprintf(sfd,"%d",sp->ttfindex);
		    if ( sp->nextcpindex==0xffff )
			fprintf(sfd,",-1");
		    else if ( sp->nextcpindex!=0xfffe )
			fprintf(sfd,",%d",sp->nextcpindex);
		}
	    } else {
		if ( sp->hintmask!=NULL ) {
		    putc('x',sfd);
		    SFDDumpHintMask(sfd, sp->hintmask);
		}
	    }
	    putc('\n',sfd);
	    if (sp->name != NULL) {
		fputs("NamedP: ", sfd);
		SFDDumpUTF7Str(sfd, sp->name);
		putc('\n', sfd);
	    }
	    if ( sp==first )
	break;
	    if ( first==NULL ) first = sp;
	    if ( sp->next==NULL )
	break;
	}
	if ( spl->spiro_cnt!=0 ) {
	    int i;
	    fprintf( sfd, "  Spiro\n" );
	    for ( i=0; i<spl->spiro_cnt; ++i ) {
		fprintf( sfd, "    %g %g %c\n", spl->spiros[i].x, spl->spiros[i].y,
			    spl->spiros[i].ty&0x7f);
	    }
	    fprintf( sfd, "  EndSpiro\n" );
	}
	if ( spl->contour_name!=NULL ) {
	    fprintf( sfd, "  Named: " );
	    SFDDumpUTF7Str(sfd,spl->contour_name);
	    putc('\n',sfd);
	}
	if ( spl->is_clip_path ) {
	    fprintf( sfd, "  PathFlags: %d\n", spl->is_clip_path );
	}
	if ( spl->start_offset ) {
	    fprintf( sfd, "  PathStart: %d\n", spl->start_offset );
	}
    }
    fprintf( sfd, "EndSplineSet\n" );
}

static void SFDDumpDeviceTable(FILE *sfd,DeviceTable *adjust) {
    int i;

    if ( adjust==NULL )
return;
    fprintf( sfd, "{" );
    if ( adjust->corrections!=NULL ) {
	fprintf( sfd, "%d-%d ", adjust->first_pixel_size, adjust->last_pixel_size );
	for ( i=0; i<=adjust->last_pixel_size-adjust->first_pixel_size; ++i )
	    fprintf( sfd, "%s%d", i==0?"":",", adjust->corrections[i]);
    }
    fprintf( sfd, "}" );
}

static void SFDDumpValDevTab(FILE *sfd,ValDevTab *adjust) {
    if ( adjust==NULL )
return;
    fprintf( sfd, " [ddx=" ); SFDDumpDeviceTable(sfd,&adjust->xadjust);
    fprintf( sfd, " ddy=" ); SFDDumpDeviceTable(sfd,&adjust->yadjust);
    fprintf( sfd, " ddh=" ); SFDDumpDeviceTable(sfd,&adjust->xadv);
    fprintf( sfd, " ddv=" ); SFDDumpDeviceTable(sfd,&adjust->yadv);
    putc(']',sfd);
}

static void SFDDumpAnchorPoints(FILE *sfd,AnchorPoint *ap) {
    if (ap==NULL) {
	return;
    }

    for ( ; ap!=NULL; ap=ap->next )
    {
	fprintf( sfd, "AnchorPoint: " );
	SFDDumpUTF7Str(sfd,ap->anchor->name);
	putc(' ',sfd);
	fprintf( sfd, "%g %g %s %d",
		(double) ap->me.x, (double) ap->me.y,
		ap->type==at_centry ? "entry" :
		ap->type==at_cexit ? "exit" :
		ap->type==at_mark ? "mark" :
		ap->type==at_basechar ? "basechar" :
		ap->type==at_baselig ? "baselig" : "basemark",
		ap->lig_index );
	if ( ap->xadjust.corrections!=NULL || ap->yadjust.corrections!=NULL ) {
	    putc(' ',sfd);
	    SFDDumpDeviceTable(sfd,&ap->xadjust);
	    putc(' ',sfd);
	    SFDDumpDeviceTable(sfd,&ap->yadjust);
	} else
	if ( ap->has_ttf_pt )
	    fprintf( sfd, " %d", ap->ttf_pt_index );
	putc('\n',sfd);
    }
}

/* Run length encoding */
/* We always start with a background pixel(1), each line is a series of counts */
/*  we alternate background/foreground. If we can't represent an entire run */
/*  as one count, then we can split it up into several smaller runs and put */
/*  0 counts in between */
/* counts 0-254 mean 0-254 pixels of the current color */
/* count 255 means that the next two bytes (bigendian) provide a two byte count */
/* count 255 0 n (n<255) means that the previous line should be repeated n+1 times */
/* count 255 0 255 means 255 pixels of the current color */
static uint8 *image2rle(struct _GImage *img, int *len) {
    int max = img->height*img->bytes_per_line;
    uint8 *rle, *pt, *end;
    int cnt, set;
    int i,j,k;

    *len = 0;
    if ( img->image_type!=it_mono || img->bytes_per_line<5 )
return( NULL );
    rle = calloc(max,sizeof(uint8)), pt = rle, end=rle+max-3;

    for ( i=0; i<img->height; ++i ) {
	if ( i!=0 ) {
	    if ( memcmp(img->data+i*img->bytes_per_line,
			img->data+(i-1)*img->bytes_per_line, img->bytes_per_line)== 0 ) {
		for ( k=1; k<img->height-i; ++k ) {
		    if ( memcmp(img->data+(i+k)*img->bytes_per_line,
				img->data+i*img->bytes_per_line, img->bytes_per_line)!= 0 )
		break;
		}
		i+=k;
		while ( k>0 ) {
		    if ( pt>end ) {
			free(rle);
return( NULL );
		    }
		    *pt++ = 255;
		    *pt++ = 0;
		    *pt++ = k>254 ? 254 : k;
		    k -= 254;
		}
		if ( i>=img->height )
    break;
	    }
	}

	set=1; cnt=0; j=0;
	while ( j<img->width ) {
	    for ( k=j; k<img->width; ++k ) {
		if (( set && !(img->data[i*img->bytes_per_line+(k>>3)]&(0x80>>(k&7))) ) ||
		    ( !set && (img->data[i*img->bytes_per_line+(k>>3)]&(0x80>>(k&7))) ))
	    break;
	    }
	    cnt = k-j;
	    j=k;
	    do {
		if ( pt>=end ) {
		    free(rle);
return( NULL );
		}
		if ( cnt<=254 )
		    *pt++ = cnt;
		else {
		    *pt++ = 255;
		    if ( cnt>65535 ) {
			*pt++ = 255;
			*pt++ = 255;
			*pt++ = 0;		/* nothing of the other color, we've still got more of this one */
		    } else {
			*pt++ = cnt>>8;
			*pt++ = cnt&0xff;
		    }
		}
		cnt -= 65535;
	    } while ( cnt>0 );
	    set = 1-set;
	}
    }
    *len = pt-rle;
return( rle );
}

void SFDDumpUndo(FILE *sfd,SplineChar *sc,Undoes *u, const char* keyPrefix, int idx ) {
    fprintf(sfd, "%sOperation\n",      keyPrefix );
    fprintf(sfd, "Index: %d\n",        idx );
    fprintf(sfd, "Type: %d\n",         u->undotype );
    fprintf(sfd, "WasModified: %d\n",  u->was_modified );
    fprintf(sfd, "WasOrder2: %d\n",    u->was_order2 );
    if( u->layer != UNDO_LAYER_UNKNOWN )
    {
	fprintf(sfd, "Layer: %d\n",    u->layer );
    }

    switch( u->undotype )
    {
        case ut_tstate:
        case ut_state:
            fprintf(sfd, "Width: %d\n",           u->u.state.width );
            fprintf(sfd, "VWidth: %d\n",          u->u.state.vwidth );
            fprintf(sfd, "LBearingChange: %d\n",  u->u.state.lbearingchange );
            fprintf(sfd, "UnicodeEnc: %d\n",      u->u.state.unicodeenc );
            if( u->u.state.charname )
                fprintf(sfd, "Charname: \"%s\"\n", u->u.state.charname );
            if( u->u.state.comment )
                fprintf(sfd, "Comment: \"%s\"\n", u->u.state.comment );
            if( u->u.state.refs ) {
                SFDDumpRefs( sfd, u->u.state.refs, 0 );
            }
	    if( u->u.state.images ) {
                SFDDumpImage( sfd, u->u.state.images );
            }
            fprintf(sfd, "InstructionsLength: %d\n", u->u.state.instrs_len );
            if( u->u.state.anchor ) {
                SFDDumpAnchorPoints( sfd, u->u.state.anchor );
            }
	    if( u->u.state.splines ) {
                fprintf(sfd, "SplineSet\n" );
                SFDDumpSplineSet( sfd, u->u.state.splines );
            }
            break;

        case ut_statehint:
        {
            SplineChar* tsc = 0;
            tsc = SplineCharCopy( sc, 0, 0 );
            ExtractHints( tsc, u->u.state.hints, 1 );
            SFDDumpHintList(  sfd, "HStem: ",  tsc->hstem);
            SFDDumpHintList(  sfd, "VStem: ",  tsc->vstem);
            SFDDumpDHintList( sfd, "DStem2: ", tsc->dstem);
            SplineCharFree( tsc );

	    if( u->u.state.instrs_len )
                SFDDumpTtfInstrsExplicit( sfd, u->u.state.instrs, u->u.state.instrs_len );
            break;
        }

        case ut_hints:
        {
            SplineChar* tsc = 0;
            tsc = SplineCharCopy( sc, 0, 0 );
            tsc->ttf_instrs = 0;
            ExtractHints( tsc, u->u.state.hints, 1 );
            SFDDumpHintList(  sfd, "HStem: ",  tsc->hstem);
            SFDDumpHintList(  sfd, "VStem: ",  tsc->vstem);
            SFDDumpDHintList( sfd, "DStem2: ", tsc->dstem);
            SplineCharFree( tsc );

            if( u->u.state.instrs_len )
                SFDDumpTtfInstrsExplicit( sfd, u->u.state.instrs, u->u.state.instrs_len );
            if( u->copied_from && u->copied_from->fullname )
                fprintf(sfd, "CopiedFrom: %s\n", u->copied_from->fullname );
            break;
        }

        case ut_width:
        case ut_vwidth:
        {
            fprintf(sfd, "Width: %d\n", u->u.width );
            break;
        }

        default:
        break;
    }

    fprintf(sfd, "End%sOperation\n", keyPrefix );
}

static void SFDDumpImage(FILE *sfd,ImageList *img) {
    GImage *image = img->image;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    struct enc85 enc;
    int rlelen;
    uint8 *rle;
    int i;

    rle = image2rle(base,&rlelen);
    fprintf(sfd, "Image: %d %d %d %d %d %x %g %g %g %g %d\n",
	    (int) base->width, (int) base->height, base->image_type,
	    (int) (base->image_type==it_true?3*base->width:base->bytes_per_line),
	    base->clut==NULL?0:base->clut->clut_len,(int) base->trans,
	    (double) img->xoff, (double) img->yoff, (double) img->xscale, (double) img->yscale, rlelen );
    memset(&enc,'\0',sizeof(enc));
    enc.sfd = sfd;
    if ( base->clut!=NULL ) {
	for ( i=0; i<base->clut->clut_len; ++i ) {
	    SFDEnc85(&enc,base->clut->clut[i]>>16);
	    SFDEnc85(&enc,(base->clut->clut[i]>>8)&0xff);
	    SFDEnc85(&enc,base->clut->clut[i]&0xff);
	}
    }
    if ( rle!=NULL ) {
	uint8 *pt=rle, *end=rle+rlelen;
	while ( pt<end )
	    SFDEnc85(&enc,*pt++);
	free( rle );
    } else {
	for ( i=0; i<base->height; ++i ) {
	    if ( base->image_type==it_rgba ) {
		uint32 *ipt = (uint32 *) (base->data + i*base->bytes_per_line);
		uint32 *iend = (uint32 *) (base->data + (i+1)*base->bytes_per_line);
		while ( ipt<iend ) {
		    SFDEnc85(&enc,*ipt>>24);
		    SFDEnc85(&enc,(*ipt>>16)&0xff);
		    SFDEnc85(&enc,(*ipt>>8)&0xff);
		    SFDEnc85(&enc,*ipt&0xff);
		    ++ipt;
		}
	    } else if ( base->image_type==it_true ) {
		int *ipt = (int *) (base->data + i*base->bytes_per_line);
		int *iend = (int *) (base->data + (i+1)*base->bytes_per_line);
		while ( ipt<iend ) {
		    SFDEnc85(&enc,*ipt>>16);
		    SFDEnc85(&enc,(*ipt>>8)&0xff);
		    SFDEnc85(&enc,*ipt&0xff);
		    ++ipt;
		}
	    } else {
		uint8 *pt = (uint8 *) (base->data + i*base->bytes_per_line);
		uint8 *end = (uint8 *) (base->data + (i+1)*base->bytes_per_line);
		while ( pt<end ) {
		    SFDEnc85(&enc,*pt);
		    ++pt;
		}
	    }
	}
    }
    SFDEnc85EndEnc(&enc);
    fprintf(sfd,"\nEndImage\n" );
}

static void SFDDumpHintList(FILE *sfd,const char *key, StemInfo *h) {
    HintInstance *hi;

    if ( h==NULL )
return;
    fprintf(sfd, "%s", key );
    for ( ; h!=NULL; h=h->next ) {
	fprintf(sfd, "%g %g", (double) h->start,(double) h->width );
	if ( h->ghost ) putc('G',sfd);
	if ( h->where!=NULL ) {
	    putc('<',sfd);
	    for ( hi=h->where; hi!=NULL; hi=hi->next )
		fprintf(sfd, "%g %g%c", (double) hi->begin, (double) hi->end, hi->next?' ':'>');
	}
	putc(h->next?' ':'\n',sfd);
    }
}

static void SFDDumpDHintList( FILE *sfd,const char *key, DStemInfo *d ) {
    HintInstance *hi;

    if ( d==NULL )
return;
    fprintf(sfd, "%s", key );
    for ( ; d!=NULL; d=d->next ) {
	fprintf(sfd, "%g %g %g %g %g %g",
		(double) d->left.x, (double) d->left.y,
		(double) d->right.x, (double) d->right.y,
		(double) d->unit.x, (double) d->unit.y );
	if ( d->where!=NULL ) {
	    putc('<',sfd);
	    for ( hi=d->where; hi!=NULL; hi=hi->next )
		fprintf(sfd, "%g %g%c", (double) hi->begin, (double) hi->end, hi->next?' ':'>');
	}
	putc(d->next?' ':'\n',sfd);
    }
}

static void SFDDumpTtfInstrsExplicit(FILE *sfd,uint8 *ttf_instrs, int16 ttf_instrs_len )
{
    char *instrs = _IVUnParseInstrs( ttf_instrs, ttf_instrs_len );
    char *pt;
    fprintf( sfd, "TtInstrs:\n" );
    for ( pt=instrs; *pt!='\0'; ++pt )
	putc(*pt,sfd);
    if ( pt[-1]!='\n' )
	putc('\n',sfd);
    free(instrs);
    fprintf( sfd, "%s\n", end_tt_instrs );
}
static void SFDDumpTtfInstrs(FILE *sfd,SplineChar *sc)
{
    SFDDumpTtfInstrsExplicit( sfd, sc->ttf_instrs,sc->ttf_instrs_len );
}

static void SFDDumpTtfTable(FILE *sfd,struct ttf_table *tab,SplineFont *sf) {
    if ( tab->tag == CHR('p','r','e','p') || tab->tag == CHR('f','p','g','m') ) {
	/* These are tables of instructions and should be dumped as such */
	char *instrs;
	char *pt;
	fprintf( sfd, "TtTable: %c%c%c%c\n",
		(int) (tab->tag>>24), (int) ((tab->tag>>16)&0xff), (int) ((tab->tag>>8)&0xff), (int) (tab->tag&0xff) );
	instrs = _IVUnParseInstrs( tab->data,tab->len );
	for ( pt=instrs; *pt!='\0'; ++pt )
	    putc(*pt,sfd);
	if ( pt[-1]!='\n' )
	    putc('\n',sfd);
	free(instrs);
	fprintf( sfd, "%s\n", end_tt_instrs );
    } else if ( (tab->tag == CHR('c','v','t',' ') || tab->tag == CHR('m','a','x','p')) &&
	    (tab->len&1)==0 ) {
	int i, ended;
	uint8 *pt;
	fprintf( sfd, "ShortTable: %c%c%c%c %d\n",
		(int) (tab->tag>>24), (int) ((tab->tag>>16)&0xff), (int) ((tab->tag>>8)&0xff), (int) (tab->tag&0xff),
		(int) (tab->len>>1) );
	pt = (uint8*) tab->data;
	ended = tab->tag!=CHR('c','v','t',' ') || sf->cvt_names==NULL;
	for ( i=0; i<(tab->len>>1); ++i ) {
	    int num = (int16) ((pt[0]<<8) | pt[1]);
	    fprintf( sfd, "  %d", num );
	    if ( !ended ) {
		if ( sf->cvt_names[i]==END_CVT_NAMES )
		    ended=true;
		else if ( sf->cvt_names[i]!=NULL ) {
		    putc(' ',sfd);
		    SFDDumpUTF7Str(sfd,sf->cvt_names[i]);
		    putc(' ',sfd);
		}
	    }
	    putc('\n',sfd);
	    pt += 2;
	}
	fprintf( sfd, "EndShort\n");
    } else {
	/* maxp, who knows what. Dump 'em as binary for now */
	struct enc85 enc;
	int i;

	memset(&enc,'\0',sizeof(enc));
	enc.sfd = sfd;

	fprintf( sfd, "TtfTable: %c%c%c%c %d\n",
		(int) (tab->tag>>24), (int) ((tab->tag>>16)&0xff), (int) ((tab->tag>>8)&0xff), (int) (tab->tag&0xff),
		(int) tab->len );
	for ( i=0; i<tab->len; ++i )
	    SFDEnc85(&enc,tab->data[i]);
	SFDEnc85EndEnc(&enc);
	fprintf(sfd,"\nEndTtf\n" );
    }
}

static int SFDOmit(SplineChar *sc) {
    int layer;
    BDFFont *bdf;

    if ( sc==NULL )
return( true );
    if ( sc->comment!=NULL || sc->color!=COLOR_DEFAULT )
return( false );
    for ( layer = ly_back; layer<sc->layer_cnt; ++layer ) {
	if ( sc->layers[layer].splines!=NULL ||
		sc->layers[layer].refs!=NULL ||
		sc->layers[layer].images!=NULL )
return( false );
    }
    if ( sc->parent->onlybitmaps ) {
	for ( bdf = sc->parent->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	    if ( sc->orig_pos<bdf->glyphcnt && bdf->glyphs[sc->orig_pos]!=NULL )
return( false );
	}
    }
    if ( !sc->widthset )
return(true);

return( false );
}

static void SFDDumpRefs(FILE *sfd,RefChar *refs, int *newgids) {
    RefChar *ref;

    for ( ref=refs; ref!=NULL; ref=ref->next ) if ( ref->sc!=NULL ) {
	fprintf(sfd, "Refer: %d %d %c %g %g %g %g %g %g %d",
		    newgids!=NULL ? newgids[ref->sc->orig_pos]:ref->sc->orig_pos,
		    ref->sc->unicodeenc,
		    ref->selected?'S':'N',
		    (double) ref->transform[0], (double) ref->transform[1], (double) ref->transform[2],
		    (double) ref->transform[3], (double) ref->transform[4], (double) ref->transform[5],
		    ref->use_my_metrics|(ref->round_translation_to_grid<<1)|
		     (ref->point_match<<2));
	if ( ref->point_match ) {
	    fprintf(sfd, " %d %d", ref->match_pt_base, ref->match_pt_ref );
	    if ( ref->point_match_out_of_date )
		fprintf( sfd, " O" );
	}
	putc('\n',sfd);
    }
}

static void SFDDumpMathVertex(FILE *sfd,struct mathkernvertex *vert,const char *name) {
    int i;

    if ( vert==NULL || vert->cnt==0 )
return;

    fprintf( sfd, "%s %d ", name, vert->cnt );
    for ( i=0; i<vert->cnt; ++i ) {
	fprintf( sfd, " %d", vert->mkd[i].height );
	SFDDumpDeviceTable(sfd,vert->mkd[i].height_adjusts );
	fprintf( sfd, ",%d", vert->mkd[i].kern );
	SFDDumpDeviceTable(sfd,vert->mkd[i].kern_adjusts );
    }
    putc('\n',sfd );
}

static void SFDDumpGlyphVariants(FILE *sfd,struct glyphvariants *gv,const char *name) {
    int i;

    if ( gv==NULL )
return;

    if ( gv->variants!=NULL )
	fprintf( sfd, "GlyphVariants%s: %s\n", name, gv->variants );
    if ( gv->part_cnt!=0 ) {
	if ( gv->italic_correction!=0 ) {
	    fprintf( sfd, "GlyphComposition%sIC: %d", name, gv->italic_correction );
	    if ( gv->italic_adjusts!=NULL ) {
		putc(' ',sfd);
		SFDDumpDeviceTable(sfd,gv->italic_adjusts);
	    }
	    putc('\n',sfd);
	}
	fprintf( sfd, "GlyphComposition%s: %d ", name, gv->part_cnt );
	for ( i=0; i<gv->part_cnt; ++i ) {
	    fprintf( sfd, " %s%%%d,%d,%d,%d", gv->parts[i].component,
		    gv->parts[i].is_extender,
		    gv->parts[i].startConnectorLength,
		    gv->parts[i].endConnectorLength,
		    gv->parts[i].fullAdvance);
	}
	putc('\n',sfd);
    }
}

static void SFDDumpCharMath(FILE *sfd,SplineChar *sc) {
    if ( sc->italic_correction!=TEX_UNDEF && sc->italic_correction!=0 ) {
	fprintf( sfd, "ItalicCorrection: %d", sc->italic_correction );
	if ( sc->italic_adjusts!=NULL ) {
	    putc(' ',sfd);
	    SFDDumpDeviceTable(sfd,sc->italic_adjusts);
	}
	putc('\n',sfd);
    }
    if ( sc->top_accent_horiz!=TEX_UNDEF ) {
	fprintf( sfd, "TopAccentHorizontal: %d", sc->top_accent_horiz );
	if ( sc->top_accent_adjusts!=NULL ) {
	    putc(' ',sfd);
	    SFDDumpDeviceTable(sfd,sc->top_accent_adjusts);
	}
	putc('\n',sfd);
    }
    if ( sc->is_extended_shape )
	fprintf( sfd, "IsExtendedShape: %d\n", sc->is_extended_shape );
    SFDDumpGlyphVariants(sfd,sc->vert_variants,"Vertical");
    SFDDumpGlyphVariants(sfd,sc->horiz_variants,"Horizontal");
    if ( sc->mathkern!=NULL ) {
	SFDDumpMathVertex(sfd,&sc->mathkern->top_right,"TopRightVertex:");
	SFDDumpMathVertex(sfd,&sc->mathkern->top_left,"TopLeftVertex:");
	SFDDumpMathVertex(sfd,&sc->mathkern->bottom_right,"BottomRightVertex:");
	SFDDumpMathVertex(sfd,&sc->mathkern->bottom_left,"BottomLeftVertex:");
    }
}

static void SFDPickleMe(FILE *sfd,void *python_data, int python_data_has_lists) {
    char *string, *pt;

#ifdef _NO_PYTHON
    string = (char *) python_data;
#else
    string = PyFF_PickleMeToString(python_data);
#endif
    if ( string==NULL )
return;
    if (python_data_has_lists)
    fprintf( sfd, "PickledDataWithLists: \"" );
    else
    fprintf( sfd, "PickledData: \"" );
    for ( pt=string; *pt; ++pt ) {
	if ( *pt=='\\' || *pt=='"' )
	    putc('\\',sfd);
	putc(*pt,sfd);
    }
    fprintf( sfd, "\"\n");
#ifndef _NO_PYTHON
    free(string);
#endif
}

static void *SFDUnPickle(FILE *sfd, int python_data_has_lists) {
    int ch, quoted;
    static int max = 0;
    static char *buf = NULL;
    char *pt, *end;
    int cnt;

    pt = buf; end = buf+max;
    while ( (ch=nlgetc(sfd))!='"' && ch!='\n' && ch!=EOF );
    if ( ch!='"' )
return( NULL );

    quoted = false;
    while ( ((ch=nlgetc(sfd))!='"' || quoted) && ch!=EOF ) {
	if ( !quoted && ch=='\\' )
	    quoted = true;
	else {
	    if ( pt>=end ) {
		cnt = pt-buf;
		buf = realloc(buf,(max+=200)+1);
		pt = buf+cnt;
		end = buf+max;
	    }
	    *pt++ = ch;
	    quoted = false;
	}
    }
    if ( pt==buf )
return( NULL );
    *pt='\0';
#ifdef _NO_PYTHON
return( copy(buf));
#else
return( PyFF_UnPickleMeToObjects(buf));
#endif
    /* buf is a static buffer, I don't free it, I'll reuse it next time */
}


static void SFDDumpGradient(FILE *sfd, const char *keyword, struct gradient *gradient) {
    int i;

    /* Use ";" as a coord separator because we treat "," as a potential decimal point */
    fprintf( sfd, "%s %g;%g %g;%g %g %s %d ", keyword,
	    (double) gradient->start.x, (double) gradient->start.y,
	    (double) gradient->stop.x, (double) gradient->stop.y,
	    (double) gradient->radius,
	    spreads[gradient->sm],
	    gradient->stop_cnt );
    for ( i=0 ; i<gradient->stop_cnt; ++i ) {
	fprintf( sfd, "{%g #%06x %g} ", (double) gradient->grad_stops[i].offset,
		gradient->grad_stops[i].col, (double) gradient->grad_stops[i].opacity );
    }
    putc('\n',sfd);
}

static void SFDDumpPattern(FILE *sfd, const char *keyword, struct pattern *pattern) {

    fprintf( sfd, "%s %s %g;%g [%g %g %g %g %g %g]\n", keyword,
	    pattern->pattern,
	    (double) pattern->width, (double) pattern->height,
	    (double) pattern->transform[0], (double) pattern->transform[1],
	    (double) pattern->transform[2], (double) pattern->transform[3],
	    (double) pattern->transform[4], (double) pattern->transform[5] );
}

void SFD_DumpPST( FILE *sfd, SplineChar *sc ) {
    PST *pst;

    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
	if (( pst->subtable==NULL && pst->type!=pst_lcaret) || pst->type==pst_null )
	    /* Skip it */;
	else {
	    static const char *keywords[] = { "Null:", "Position2:", "PairPos2:",
		    "Substitution2:",
		    "AlternateSubs2:", "MultipleSubs2:", "Ligature2:",
		    "LCarets2:", NULL };
	    fprintf( sfd, "%s ", keywords[pst->type] );
	    if ( pst->subtable!=NULL ) {
		SFDDumpUTF7Str(sfd,pst->subtable->subtable_name);
		putc(' ',sfd);
	    }
	    if ( pst->type==pst_position ) {
		fprintf( sfd, "dx=%d dy=%d dh=%d dv=%d",
			pst->u.pos.xoff, pst->u.pos.yoff,
			pst->u.pos.h_adv_off, pst->u.pos.v_adv_off);
		SFDDumpValDevTab(sfd,pst->u.pos.adjust);
		putc('\n',sfd);
	    } else if ( pst->type==pst_pair ) {
		fprintf( sfd, "%s dx=%d dy=%d dh=%d dv=%d",
			pst->u.pair.paired,
			pst->u.pair.vr[0].xoff, pst->u.pair.vr[0].yoff,
			pst->u.pair.vr[0].h_adv_off, pst->u.pair.vr[0].v_adv_off );
		SFDDumpValDevTab(sfd,pst->u.pair.vr[0].adjust);
		fprintf( sfd, " dx=%d dy=%d dh=%d dv=%d",
			pst->u.pair.vr[1].xoff, pst->u.pair.vr[1].yoff,
			pst->u.pair.vr[1].h_adv_off, pst->u.pair.vr[1].v_adv_off);
		SFDDumpValDevTab(sfd,pst->u.pair.vr[1].adjust);
		putc('\n',sfd);
	    } else if ( pst->type==pst_lcaret ) {
		int i;
		fprintf( sfd, "%d ", pst->u.lcaret.cnt );
		for ( i=0; i<pst->u.lcaret.cnt; ++i ) {
		    fprintf( sfd, "%d", pst->u.lcaret.carets[i] );
                    if ( i<pst->u.lcaret.cnt-1 ) putc(' ',sfd);
                }
		fprintf( sfd, "\n" );
	    } else
		fprintf( sfd, "%s\n", pst->u.lig.components );
	}
    }
}



void SFD_DumpKerns( FILE *sfd, SplineChar *sc, int *newgids ) {
    KernPair *kp;
    int v;

    for ( v=0; v<2; ++v ) {
	kp = v ? sc->vkerns : sc->kerns;
	if ( kp!=NULL ) {
	    fprintf( sfd, v ? "VKerns2:" : "Kerns2:" );
	    for ( ; kp!=NULL; kp=kp->next )
		if ( !SFDOmit(kp->sc)) {
		    fprintf( sfd, " %d %d ",
			    newgids!=NULL?newgids[kp->sc->orig_pos]:kp->sc->orig_pos,
			    kp->off );
		    SFDDumpUTF7Str(sfd,kp->subtable->subtable_name);
		    if ( kp->adjust!=NULL ) putc(' ',sfd);
		    SFDDumpDeviceTable(sfd,kp->adjust);
		}
	    fprintf(sfd, "\n" );
	}
    }
}

void SFDDumpCharStartingMarker(FILE *sfd,SplineChar *sc) {
    if ( AllAscii(sc->name))
	fprintf(sfd, "StartChar: %s\n", sc->name );
    else {
	fprintf(sfd, "StartChar: " );
	SFDDumpUTF7Str(sfd,sc->name);
	putc('\n',sfd);
    }
}


static void SFDDumpChar(FILE *sfd,SplineChar *sc,EncMap *map,int *newgids,int todir,int saveUndoes) {
    // TODO: Output the U. F. O. glif name.
    ImageList *img;
    KernPair *kp;
    PST *pst;
    int i, v, enc;
    struct altuni *altuni;

    if (!todir)
	putc('\n',sfd);

    SFDDumpCharStartingMarker( sfd, sc );
    if ( (enc = map->backmap[sc->orig_pos])>=map->enccount ) {
	if ( sc->parent->cidmaster==NULL )
	    IError("Bad reverse encoding");
	enc = -1;
    }
    if ( sc->unicodeenc!=-1 &&
	    ((map->enc->is_unicodebmp && sc->unicodeenc<0x10000) ||
	     (map->enc->is_unicodefull && sc->unicodeenc < (int)unicode4_size)) )
	/* If we have altunis, then the backmap may not give the primary */
	/*  unicode code point, which is what we need here */
	fprintf(sfd, "Encoding: %d %d %d\n", sc->unicodeenc, sc->unicodeenc,
		newgids!=NULL?newgids[sc->orig_pos]:sc->orig_pos);
    else
	fprintf(sfd, "Encoding: %d %d %d\n", enc, sc->unicodeenc,
		newgids!=NULL?newgids[sc->orig_pos]:sc->orig_pos);
    if ( sc->altuni ) {
	fprintf( sfd, "AltUni2:" );
	for ( altuni = sc->altuni; altuni!=NULL; altuni=altuni->next )
	    fprintf( sfd, " %06x.%06x.%x", altuni->unienc, altuni->vs, altuni->fid );
	putc( '\n', sfd);
    }
    if ( sc->glif_name ) {
        fprintf(sfd, "GlifName: ");
        if ( AllAscii(sc->glif_name))
	    fprintf(sfd, "%s", sc->glif_name );
        else {
	    SFDDumpUTF7Str(sfd,sc->glif_name);
        }
        putc('\n',sfd);
    }
    fprintf(sfd, "Width: %d\n", sc->width );
    if ( sc->vwidth!=sc->parent->ascent+sc->parent->descent )
	fprintf(sfd, "VWidth: %d\n", sc->vwidth );
    if ( sc->glyph_class!=0 )
	fprintf(sfd, "GlyphClass: %d\n", sc->glyph_class );
    if ( sc->unlink_rm_ovrlp_save_undo )
	fprintf(sfd, "UnlinkRmOvrlpSave: %d\n", sc->unlink_rm_ovrlp_save_undo );
    if ( sc->inspiro )
	fprintf(sfd, "InSpiro: %d\n", sc->inspiro );
    if ( sc->lig_caret_cnt_fixed )
	fprintf(sfd, "LigCaretCntFixed: %d\n", sc->lig_caret_cnt_fixed );
    if ( sc->changedsincelasthinted|| sc->manualhints || sc->widthset )
	fprintf(sfd, "Flags: %s%s%s%s%s\n",
		sc->changedsincelasthinted?"H":"",
		sc->manualhints?"M":"",
		sc->widthset?"W":"",
		sc->views!=NULL?"O":"",
		sc->instructions_out_of_date?"I":"");
    if ( sc->tex_height!=TEX_UNDEF || sc->tex_depth!=TEX_UNDEF )
	fprintf( sfd, "TeX: %d %d\n", sc->tex_height, sc->tex_depth );
    if ( sc->is_extended_shape || sc->italic_correction!=TEX_UNDEF ||
	    sc->top_accent_horiz!=TEX_UNDEF || sc->vert_variants!=NULL ||
	    sc->horiz_variants!=NULL || sc->mathkern!=NULL )
	SFDDumpCharMath(sfd,sc);
#if 0
    // This is now layer-specific.
    if ( sc->python_persistent!=NULL )
	SFDPickleMe(sfd,sc->python_persistent,sc->python_persistent_has_lists);
#endif // 0
#if HANYANG
    if ( sc->compositionunit )
	fprintf( sfd, "CompositionUnit: %d %d\n", sc->jamo, sc->varient );
#endif
    SFDDumpHintList(sfd,"HStem: ", sc->hstem);
    SFDDumpHintList(sfd,"VStem: ", sc->vstem);
    SFDDumpDHintList(sfd,"DStem2: ", sc->dstem);
    if ( sc->countermask_cnt!=0 ) {
	fprintf( sfd, "CounterMasks: %d", sc->countermask_cnt );
	for ( i=0; i<sc->countermask_cnt; ++i ) {
	    putc(' ',sfd);
	    SFDDumpHintMask(sfd,&sc->countermasks[i]);
	}
	putc('\n',sfd);
    }
    if ( sc->ttf_instrs_len!=0 )
	SFDDumpTtfInstrs(sfd,sc);
    SFDDumpAnchorPoints(sfd,sc->anchor);
    fprintf( sfd, "LayerCount: %d\n", sc->layer_cnt );
    for ( i=0; i<sc->layer_cnt; ++i ) {
        if( saveUndoes && UndoRedoLimitToSave > 0) {
            if( sc->layers[i].undoes || sc->layers[i].redoes ) {
                fprintf(sfd, "UndoRedoHistory\n" );
                fprintf(sfd, "Layer: %d\n", i );
                Undoes *undo = 0;
                int idx   = 0;
                int limit = 0;

                fprintf(sfd, "Undoes\n" );
                idx = 0;
                undo = sc->layers[i].undoes;
                for( limit = UndoRedoLimitToSave;
                     undo && (limit==-1 || limit>0);
                     undo = undo->next, idx++ ) {
                    SFDDumpUndo( sfd, sc, undo, "Undo", idx );
                    if( limit > 0 )
                        limit--;
                }
                fprintf(sfd, "EndUndoes\n" );

		fprintf(sfd, "Redoes\n" );
                idx = 0;
                limit = UndoRedoLimitToSave;
                undo = sc->layers[i].redoes;
                for( limit = UndoRedoLimitToSave;
                     undo && (limit==-1 || limit>0);
                     undo = undo->next, idx++ ) {
                    SFDDumpUndo( sfd, sc, undo, "Redo", idx );
                    if( limit > 0 )
                        limit--;
                }
                fprintf(sfd, "EndRedoes\n" );
                fprintf(sfd, "EndUndoRedoHistory\n" );
            }
        }

	if ( sc->parent->multilayer ) {
	    fprintf(sfd, "Layer: %d  %d %d %d  #%06x %g  #%06x %g %g %s %s [%g %g %g %g] [",
		    i, sc->layers[i].dofill, sc->layers[i].dostroke, sc->layers[i].fillfirst,
		    sc->layers[i].fill_brush.col, (double) sc->layers[i].fill_brush.opacity,
		    sc->layers[i].stroke_pen.brush.col, (double) sc->layers[i].stroke_pen.brush.opacity,
		    (double) sc->layers[i].stroke_pen.width, joins[sc->layers[i].stroke_pen.linejoin], caps[sc->layers[i].stroke_pen.linecap],
		    (double) sc->layers[i].stroke_pen.trans[0], (double) sc->layers[i].stroke_pen.trans[1],
		    (double) sc->layers[i].stroke_pen.trans[2], (double) sc->layers[i].stroke_pen.trans[3] );
	    if ( sc->layers[i].stroke_pen.dashes[0]==0 && sc->layers[i].stroke_pen.dashes[1]==DASH_INHERITED )
		fprintf(sfd,"0 %d]\n", DASH_INHERITED);
	    else { int j;
		for ( j=0; j<DASH_MAX && sc->layers[i].stroke_pen.dashes[j]!=0; ++j )
		    fprintf( sfd,"%d ", sc->layers[i].stroke_pen.dashes[j]);
		fprintf(sfd,"]\n");
	    }
	    if ( sc->layers[i].fill_brush.gradient!=NULL )
		SFDDumpGradient(sfd,"FillGradient:", sc->layers[i].fill_brush.gradient );
	    else if ( sc->layers[i].fill_brush.pattern!=NULL )
		SFDDumpPattern(sfd,"FillPattern:", sc->layers[i].fill_brush.pattern );
	    if ( sc->layers[i].stroke_pen.brush.gradient!=NULL )
		SFDDumpGradient(sfd,"StrokeGradient:", sc->layers[i].stroke_pen.brush.gradient );
	    else if ( sc->layers[i].stroke_pen.brush.pattern!=NULL )
		SFDDumpPattern(sfd,"StrokePattern:", sc->layers[i].stroke_pen.brush.pattern );
	} else {
	    if ( sc->layers[i].images==NULL && sc->layers[i].splines==NULL &&
		    sc->layers[i].refs==NULL && (sc->layers[i].validation_state&vs_known) == 0 &&
		    sc->layers[i].python_persistent == NULL)
    continue;
	    if ( i==ly_back )
		fprintf( sfd, "Back\n" );
	    else if ( i==ly_fore )
		fprintf( sfd, "Fore\n" );
	    else
		fprintf(sfd, "Layer: %d\n", i );
	}
	for ( img=sc->layers[i].images; img!=NULL; img=img->next )
	    SFDDumpImage(sfd,img);
	if ( sc->layers[i].splines!=NULL ) {
	    fprintf(sfd, "SplineSet\n" );
	    SFDDumpSplineSet(sfd,sc->layers[i].splines);
	}
	SFDDumpRefs(sfd,sc->layers[i].refs,newgids);
	if ( sc->layers[i].validation_state&vs_known )
	    fprintf( sfd, "Validated: %d\n", sc->layers[i].validation_state );
        if ( sc->layers[i].python_persistent!=NULL )
	  SFDPickleMe(sfd,sc->layers[i].python_persistent,sc->layers[i].python_persistent_has_lists);
    }
    for ( v=0; v<2; ++v ) {
	kp = v ? sc->vkerns : sc->kerns;
	if ( kp!=NULL ) {
	    fprintf( sfd, v ? "VKerns2:" : "Kerns2:" );
	    for ( ; kp!=NULL; kp=kp->next ) {
            if ( !SFDOmit(kp->sc)) {
                fprintf( sfd, " %d %d ",
                         newgids!=NULL?newgids[kp->sc->orig_pos]:kp->sc->orig_pos,
                         kp->off );
                SFDDumpUTF7Str(sfd,kp->subtable->subtable_name);
                if ( kp->adjust!=NULL ) putc(' ',sfd);
                SFDDumpDeviceTable(sfd,kp->adjust);
            }
        }
	    fprintf(sfd, "\n" );
	}
    }
    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
	if (( pst->subtable==NULL && pst->type!=pst_lcaret) || pst->type==pst_null )
	    /* Skip it */;
	else {
	    static const char *keywords[] = { "Null:", "Position2:", "PairPos2:",
		    "Substitution2:",
		    "AlternateSubs2:", "MultipleSubs2:", "Ligature2:",
		    "LCarets2:", NULL };
	    fprintf( sfd, "%s ", keywords[pst->type] );
	    if ( pst->subtable!=NULL ) {
		SFDDumpUTF7Str(sfd,pst->subtable->subtable_name);
		putc(' ',sfd);
	    }
	    if ( pst->type==pst_position ) {
		fprintf( sfd, "dx=%d dy=%d dh=%d dv=%d",
			pst->u.pos.xoff, pst->u.pos.yoff,
			pst->u.pos.h_adv_off, pst->u.pos.v_adv_off);
		SFDDumpValDevTab(sfd,pst->u.pos.adjust);
		putc('\n',sfd);
	    } else if ( pst->type==pst_pair ) {
		fprintf( sfd, "%s dx=%d dy=%d dh=%d dv=%d",
			pst->u.pair.paired,
			pst->u.pair.vr[0].xoff, pst->u.pair.vr[0].yoff,
			pst->u.pair.vr[0].h_adv_off, pst->u.pair.vr[0].v_adv_off );
		SFDDumpValDevTab(sfd,pst->u.pair.vr[0].adjust);
		fprintf( sfd, " dx=%d dy=%d dh=%d dv=%d",
			pst->u.pair.vr[1].xoff, pst->u.pair.vr[1].yoff,
			pst->u.pair.vr[1].h_adv_off, pst->u.pair.vr[1].v_adv_off);
		SFDDumpValDevTab(sfd,pst->u.pair.vr[1].adjust);
		putc('\n',sfd);
	    } else if ( pst->type==pst_lcaret ) {
		int i;
		fprintf( sfd, "%d ", pst->u.lcaret.cnt );
		for ( i=0; i<pst->u.lcaret.cnt; ++i ) {
		    fprintf( sfd, "%d", pst->u.lcaret.carets[i] );
                    if ( i<pst->u.lcaret.cnt-1 ) putc(' ',sfd);
                }
		fprintf( sfd, "\n" );
	    } else
		fprintf( sfd, "%s\n", pst->u.lig.components );
	}
    }
    if ( sc->comment!=NULL ) {
	fprintf( sfd, "Comment: " );
	SFDDumpUTF7Str(sfd,sc->comment);
	putc('\n',sfd);
    }
    if ( sc->color!=COLOR_DEFAULT )
	fprintf( sfd, "Colour: %x\n", (int) sc->color );
    if ( sc->parent->multilayer ) {
	if ( sc->tile_margin!=0 )
	    fprintf( sfd, "TileMargin: %g\n", (double) sc->tile_margin );
	else if ( sc->tile_bounds.minx!=0 || sc->tile_bounds.maxx!=0 )
	    fprintf( sfd, "TileBounds: %g %g %g %g\n", (double) sc->tile_bounds.minx, (double) sc->tile_bounds.miny, (double) sc->tile_bounds.maxx, (double) sc->tile_bounds.maxy );
    }
    fprintf(sfd,"EndChar\n" );
}

static void SFDDumpBitmapChar(FILE *sfd,BDFChar *bfc, int enc,int *newgids) {
    struct enc85 encrypt;
    int i;

    fprintf(sfd, "BDFChar: %d %d %d %d %d %d %d",
	    newgids!=NULL ? newgids[bfc->orig_pos] : bfc->orig_pos, enc,
	    bfc->width, bfc->xmin, bfc->xmax, bfc->ymin, bfc->ymax );
    if ( bfc->sc->parent->hasvmetrics )
	fprintf(sfd, " %d", bfc->vwidth);
    putc('\n',sfd);
    memset(&encrypt,'\0',sizeof(encrypt));
    encrypt.sfd = sfd;
    for ( i=0; i<=bfc->ymax-bfc->ymin; ++i ) {
	uint8 *pt = (uint8 *) (bfc->bitmap + i*bfc->bytes_per_line);
	uint8 *end = pt + bfc->bytes_per_line;
	while ( pt<end ) {
	    SFDEnc85(&encrypt,*pt);
	    ++pt;
	}
    }
    SFDEnc85EndEnc(&encrypt);
    fputc('\n',sfd);
}

static void appendnames(char *dest,char *dir,const char *dir_char,char *name,const char *ext ) {
    strcpy(dest,dir);
    dest += strlen(dest);
    strcpy(dest,dir_char);
    dest += strlen(dest);
    /* Some file systems are case-insensitive, so we can't just */
    /* copy the glyph name blindly (else "A" and "a" would map to the same file */
    for (;;) {
	if ( strncmp(name,"uni",3)==0 && ishexdigit(name[3]) && ishexdigit(name[4]) &&
		ishexdigit(name[5]) && ishexdigit(name[6])) {
	    /* but in a name like uni00AD case is irrelevant. Even under unix its */
	    /*  the same as uni00ad -- and it looks ugly */
	    strncpy(dest,name,7);
	    dest += 7; name += 7;
	    while ( ishexdigit(name[0]) && ishexdigit(name[1]) &&
		    ishexdigit(name[2]) && ishexdigit(name[3]) ) {
		strncpy(dest,name,4);
		dest += 4; name += 4;
	    }
	} else if ( name[0]=='u' && ishexdigit(name[1]) && ishexdigit(name[2]) &&
		ishexdigit(name[3]) && ishexdigit(name[4]) &&
		ishexdigit(name[5]) ) {
	    strncpy(dest,name,5);
	    dest += 5; name += 5;
	} else
    break;
	if ( *name!='_' )
    break;
	*dest++ = '_';
	++name;
    }
    while ( *name ) {
	if ( isupper(*name)) {
	    *dest++ = '_';
	    *dest++ = *name;
	} else
	    *dest++ = *name;
	++name;
    }
    strcpy(dest,ext);
}

static int SFDDumpBitmapFont(FILE *sfd,BDFFont *bdf,EncMap *encm,int *newgids,
	int todir, char *dirname) {
    int i;
    int err = false;
    BDFChar *bc;
    BDFRefChar *ref;

    ff_progress_next_stage();
    if (bdf->foundry)
        fprintf( sfd, "BitmapFont: %d %d %d %d %d %s\n", bdf->pixelsize, bdf->glyphcnt,
                 bdf->ascent, bdf->descent, BDFDepth(bdf), bdf->foundry );
    else
        fprintf( sfd, "BitmapFont: %d %d %d %d %d\n", bdf->pixelsize, bdf->glyphcnt,
                 bdf->ascent, bdf->descent, BDFDepth(bdf) );
    if ( bdf->prop_cnt>0 ) {
	fprintf( sfd, "BDFStartProperties: %d\n", bdf->prop_cnt );
	for ( i=0; i<bdf->prop_cnt; ++i ) {
	    fprintf(sfd,"%s %d ", bdf->props[i].name, bdf->props[i].type );
	    switch ( bdf->props[i].type&~prt_property ) {
	      case prt_int: case prt_uint:
		fprintf(sfd, "%d\n", bdf->props[i].u.val );
	      break;
	      case prt_string: case prt_atom:
		fprintf(sfd, "\"%s\"\n", bdf->props[i].u.str );
	      break;
	      default:
	      break;
	    }
	}
	fprintf( sfd, "BDFEndProperties\n" );
    }
    if ( bdf->res>20 )
	fprintf( sfd, "Resolution: %d\n", bdf->res );
    for ( i=0; i<bdf->glyphcnt; ++i ) {
	if ( bdf->glyphs[i]!=NULL ) {
	    if ( todir ) {
		char *glyphfile = malloc(strlen(dirname)+2*strlen(bdf->glyphs[i]->sc->name)+20);
		FILE *gsfd;
		appendnames(glyphfile,dirname,"/",bdf->glyphs[i]->sc->name,BITMAP_EXT );
		gsfd = fopen(glyphfile,"w");
		if ( gsfd!=NULL ) {
		    SFDDumpBitmapChar(gsfd,bdf->glyphs[i],encm->backmap[i],newgids);
		    if ( ferror(gsfd)) err = true;
		    if ( fclose(gsfd)) err = true;
		} else
		    err = true;
		free(glyphfile);
	    } else
		SFDDumpBitmapChar(sfd,bdf->glyphs[i],encm->backmap[i],newgids);
	}
	ff_progress_next();
    }
    for ( i=0; i<bdf->glyphcnt; ++i ) if (( bc = bdf->glyphs[i] ) != NULL ) {
    	for ( ref=bc->refs; ref!=NULL; ref=ref->next )
	    fprintf(sfd, "BDFRefChar: %d %d %d %d %c\n",
		newgids!=NULL ? newgids[bc->orig_pos] : bc->orig_pos,
		newgids!=NULL ? newgids[ref->bdfc->orig_pos] : ref->bdfc->orig_pos,
		ref->xoff,ref->yoff,ref->selected?'S':'N' );
    }
    fprintf( sfd, "EndBitmapFont\n" );
return( err );
}

static void SFDDumpPrivate(FILE *sfd,struct psdict *private) {
    int i;
    char *pt;
    /* These guys should all be ascii text */
    fprintf( sfd, "BeginPrivate: %d\n", private->next );
    for ( i=0; i<private->next ; ++i ) {
      fprintf( sfd, "%s %d ", private->keys[i],
	       (int)strlen(private->values[i]));
	for ( pt = private->values[i]; *pt; ++pt )
	    putc(*pt,sfd);
	putc('\n',sfd);
    }
    fprintf( sfd, "EndPrivate\n" );
}

static void SFDDumpLangName(FILE *sfd, struct ttflangname *ln) {
    int i, end;
    fprintf( sfd, "LangName: %d", ln->lang );
    for ( end = ttf_namemax; end>0 && ln->names[end-1]==NULL; --end );
    for ( i=0; i<end; ++i ) {
        putc(' ',sfd);
        SFDDumpUTF7Str(sfd,ln->names[i]);
    }
    putc('\n',sfd);
}

static void SFDDumpGasp(FILE *sfd, SplineFont *sf) {
    int i;

    if ( sf->gasp_cnt==0 )
return;

    fprintf( sfd, "GaspTable: %d", sf->gasp_cnt );
    for ( i=0; i<sf->gasp_cnt; ++i )
	fprintf( sfd, " %d %d", sf->gasp[i].ppem, sf->gasp[i].flags );
    fprintf( sfd, " %d", sf->gasp_version);
    putc('\n',sfd);
}

static void SFDDumpDesignSize(FILE *sfd, SplineFont *sf) {
    struct otfname *on;

    if ( sf->design_size==0 )
return;

    fprintf( sfd, "DesignSize: %d", sf->design_size );
    if ( sf->fontstyle_id!=0 || sf->fontstyle_name!=NULL ||
	    sf->design_range_bottom!=0 || sf->design_range_top!=0 ) {
	fprintf( sfd, " %d-%d %d ",
		sf->design_range_bottom, sf->design_range_top,
		sf->fontstyle_id );
	for ( on=sf->fontstyle_name; on!=NULL; on=on->next ) {
	    fprintf( sfd, "%d ", on->lang );
	    SFDDumpUTF7Str(sfd, on->name);
	    if ( on->next!=NULL ) putc(' ',sfd);
	}
    }
    putc('\n',sfd);
}

static void SFDDumpOtfFeatNames(FILE *sfd, SplineFont *sf) {
    struct otffeatname *fn;
    struct otfname *on;

    for ( fn=sf->feat_names; fn!=NULL; fn=fn->next ) {
	fprintf( sfd, "OtfFeatName: '%c%c%c%c' ",
		fn->tag>>24, fn->tag>>16, fn->tag>>8, fn->tag );
	for ( on=fn->names; on!=NULL; on=on->next ) {
	    fprintf( sfd, "%d ", on->lang );
	    SFDDumpUTF7Str(sfd, on->name);
	    if ( on->next!=NULL ) putc(' ',sfd);
	}
	putc('\n',sfd);
    }
}

static void putstring(FILE *sfd, const char *header, char *body) {
    fprintf( sfd, "%s", header );
    while ( *body ) {
	if ( *body=='\n' || *body == '\\' || *body=='\r' ) {
	    putc('\\',sfd);
	    if ( *body=='\\' )
		putc('\\',sfd);
	    else {
		putc('n',sfd);
		if ( *body=='\r' && body[1]=='\n' )
		    ++body;
	    }
	} else
	    putc(*body,sfd);
	++body;
    }
    putc('\n',sfd);
}

const char *EncName(Encoding *encname) {
return( encname->enc_name );
}

static void SFDDumpEncoding(FILE *sfd,Encoding *encname,const char *keyword) {
    fprintf(sfd, "%s: %s\n", keyword, encname->enc_name );
}

static void SFDDumpMacName(FILE *sfd,struct macname *mn) {
    char *pt;

    while ( mn!=NULL ) {
      fprintf( sfd, "MacName: %d %d %d \"", mn->enc, mn->lang,
	       (int)strlen(mn->name) );
	for ( pt=mn->name; *pt; ++pt ) {
	    if ( *pt<' ' || *pt>=0x7f || *pt=='\\' || *pt=='"' )
		fprintf( sfd, "\\%03o", *(uint8 *) pt );
	    else
		putc(*pt,sfd);
	}
	fprintf( sfd, "\"\n" );
	mn = mn->next;
    }
}

void SFDDumpMacFeat(FILE *sfd,MacFeat *mf) {
    struct macsetting *ms;

    if ( mf==NULL )
return;

    while ( mf!=NULL ) {
	if ( mf->featname!=NULL ) {
	    fprintf( sfd, "MacFeat: %d %d %d\n", mf->feature, mf->ismutex, mf->default_setting );
	    SFDDumpMacName(sfd,mf->featname);
	    for ( ms=mf->settings; ms!=NULL; ms=ms->next ) {
		if ( ms->setname!=NULL ) {
		    fprintf( sfd, "MacSetting: %d\n", ms->setting );
		    SFDDumpMacName(sfd,ms->setname);
		}
	    }
	}
	mf = mf->next;
    }
    fprintf( sfd,"EndMacFeatures\n" );
}

static void SFDDumpBaseLang(FILE *sfd,struct baselangextent *bl) {

    if ( bl->lang==0 )
	fprintf( sfd, " { %d %d", bl->descent, bl->ascent );
    else
	fprintf( sfd, " { '%c%c%c%c' %d %d",
		bl->lang>>24, bl->lang>>16, bl->lang>>8, bl->lang,
		bl->descent, bl->ascent );
    for ( bl=bl->features; bl!=NULL; bl=bl->next )
	SFDDumpBaseLang(sfd,bl);
    putc('}',sfd);
}

static void SFDDumpBase(FILE *sfd,const char *keyword,struct Base *base) {
    int i;
    struct basescript *bs;
    struct baselangextent *bl;

    fprintf( sfd, "%s %d", keyword, base->baseline_cnt );
    for ( i=0; i<base->baseline_cnt; ++i ) {
	fprintf( sfd, " '%c%c%c%c'",
		base->baseline_tags[i]>>24,
		base->baseline_tags[i]>>16,
		base->baseline_tags[i]>>8,
		base->baseline_tags[i]);
    }
    putc('\n',sfd);

    for ( bs=base->scripts; bs!=NULL; bs=bs->next ) {
	fprintf( sfd, "BaseScript: '%c%c%c%c' %d ",
		bs->script>>24, bs->script>>16, bs->script>>8, bs->script,
		bs->def_baseline );
	for ( i=0; i<base->baseline_cnt; ++i )
	    fprintf( sfd, " %d", bs->baseline_pos[i]);
	for ( bl=bs->langs; bl!=NULL; bl=bl->next )
	    SFDDumpBaseLang(sfd,bl);
	putc('\n',sfd);
    }
}

static void SFDDumpJSTFLookups(FILE *sfd,const char *keyword, OTLookup **list ) {
    int i;

    if ( list==NULL || list[0]==NULL )
return;

    fprintf( sfd, "%s ", keyword );
    for ( i=0; list[i]!=NULL; ++i ) {
	SFDDumpUTF7Str(sfd,list[i]->lookup_name);
	if ( list[i+1]!=NULL ) putc(' ',sfd);
    }
    putc('\n',sfd);
}

static void SFDDumpJustify(FILE *sfd,SplineFont *sf) {
    Justify *jscript;
    struct jstf_lang *jlang;
    int i;

    for ( jscript = sf->justify; jscript!=NULL; jscript=jscript->next ) {
	fprintf( sfd, "Justify: '%c%c%c%c'\n",
		jscript->script>>24,
		jscript->script>>16,
		jscript->script>>8,
		jscript->script);
	if ( jscript->extenders!=NULL )
	    fprintf( sfd, "JstfExtender: %s\n", jscript->extenders );
	for ( jlang = jscript->langs; jlang!=NULL; jlang = jlang->next ) {
	    fprintf( sfd, "JstfLang: '%c%c%c%c' %d\n",
		jlang->lang>>24,
		jlang->lang>>16,
		jlang->lang>>8,
		jlang->lang, jlang->cnt );
	    for ( i=0; i<jlang->cnt; ++i ) {
		fprintf( sfd, "JstfPrio:\n" );
		SFDDumpJSTFLookups(sfd,"JstfEnableShrink:", jlang->prios[i].enableShrink );
		SFDDumpJSTFLookups(sfd,"JstfDisableShrink:", jlang->prios[i].disableShrink );
		SFDDumpJSTFLookups(sfd,"JstfMaxShrink:", jlang->prios[i].maxShrink );
		SFDDumpJSTFLookups(sfd,"JstfEnableExtend:", jlang->prios[i].enableExtend );
		SFDDumpJSTFLookups(sfd,"JstfDisableExtend:", jlang->prios[i].disableExtend );
		SFDDumpJSTFLookups(sfd,"JstfMaxExtend:", jlang->prios[i].maxExtend );
	    }
	}
    }
    if ( sf->justify!=NULL )
	fprintf( sfd, "EndJustify\n" );
}

static void SFDFpstClassNamesOut(FILE *sfd,int class_cnt,char **classnames,const char *keyword) {
    char buffer[20];
    int i;

    if ( class_cnt>0 && classnames!=NULL ) {
	fprintf( sfd, "  %s: ", keyword );
	for ( i=0; i<class_cnt; ++i ) {
	    if ( classnames[i]==NULL ) {
		sprintf( buffer,"%d", i );
		SFDDumpUTF7Str(sfd,buffer);
	    } else
		SFDDumpUTF7Str(sfd,classnames[i]);
	    if ( i<class_cnt-1 ) putc(' ',sfd);
	}
	putc('\n',sfd);
    }
}

FILE* MakeTemporaryFile(void) {
    FILE *ret = NULL;

#ifndef __MINGW32__
    gchar *loc;
    int fd = g_file_open_tmp("fontforge-XXXXXX", &loc, NULL);

    if (fd != -1) {
        ret = fdopen(fd, "w+");
        g_unlink(loc);
        g_free(loc);
    }
#else
    HANDLE hFile = INVALID_HANDLE_VALUE;
    for (int retries = 0; hFile == INVALID_HANDLE_VALUE && retries < 10; retries++) {
        wchar_t *temp = _wtempnam(NULL, L"FontForge");
        hFile = CreateFileW(temp, GENERIC_READ|GENERIC_WRITE, 0, NULL,
            CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE, NULL);
        free(temp);
    }
    ret = _fdopen(_open_osfhandle((intptr_t)hFile, 0), "wb+");
    if (ret == NULL && hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }
#endif
    return ret;
}


/**
 * Read an entire file from the given open file handle and return that data
 * as an allocated string that the caller must free.
 * If any read or memory error occurs, then free string and return 0.
 * FIXME: Use a better method than fseek() and ftell() since this does not
 * play well with stdin, streaming input type files, or files with NULLs
 */
char* FileToAllocatedString( FILE *f ) {
    char *ret, *buf;
    long fsize = 0;
    size_t bread = 0;

    /* get approximate file size, and allocate some memory */
    if ( fseek(f,0,SEEK_END)==0 && \
	 (fsize=ftell(f))!=-1   && \
	 fseek(f,0,SEEK_SET)==0 && \
	 (buf=calloc(fsize+30001,1))!=NULL ) {
	/* fread in file, size=non-exact, then resize memory smaller */
	bread=fread(buf,1,fsize+30000,f);
	if ( bread<=0 || bread >=(size_t)fsize+30000 || (ret=realloc(buf,bread+1))==NULL ) {
	    free( buf );
	} else {
	    ret[bread] = '\0';
	    return( ret );
	}
    }

    /* error occurred reading in file */
    fprintf(stderr,_("Failed to read a file. Bytes read:%ld file size:%ld\n"),(long)(bread),fsize );
    return( 0 );
}


void SFD_DumpLookup( FILE *sfd, SplineFont *sf ) {
    int isgpos;
    OTLookup *otl;
    struct lookup_subtable *sub;
    FeatureScriptLangList *fl;
    struct scriptlanglist *sl;
    int i;

    for ( isgpos=0; isgpos<2; ++isgpos ) {
	for ( otl = isgpos ? sf->gpos_lookups : sf->gsub_lookups; otl!=NULL; otl = otl->next ) {
	    fprintf( sfd, "Lookup: %d %d %d ", otl->lookup_type, otl->lookup_flags, otl->store_in_afm );
	    SFDDumpUTF7Str(sfd,otl->lookup_name);
	    fprintf( sfd, " { " );
	    for ( sub=otl->subtables; sub!=NULL; sub=sub->next ) {
		SFDDumpUTF7Str(sfd,sub->subtable_name);
		putc(' ',sfd);
		if ( otl->lookup_type==gsub_single && sub->suffix!=NULL ) {
		    putc('(',sfd);
		    SFDDumpUTF7Str(sfd,sub->suffix);
		    putc(')',sfd);
		} else if ( otl->lookup_type==gpos_pair && sub->vertical_kerning )
		    fprintf(sfd,"(1)");
		if ( otl->lookup_type==gpos_pair && (sub->separation!=0 || sub->kerning_by_touch))
		    fprintf(sfd,"[%d,%d,%d]", sub->separation, sub->minkern, sub->kerning_by_touch+2*sub->onlyCloser+4*sub->dontautokern );
		putc(' ',sfd);
	    }
	    fprintf( sfd, "} [" );
	    for ( fl=otl->features; fl!=NULL; fl=fl->next ) {
		if ( fl->ismac )
		    fprintf( sfd, "<%d,%d> (",
			    (int) (fl->featuretag>>16),
			    (int) (fl->featuretag&0xffff));
		else
		    fprintf( sfd, "'%c%c%c%c' (",
			    (int) (fl->featuretag>>24), (int) ((fl->featuretag>>16)&0xff),
			    (int) ((fl->featuretag>>8)&0xff), (int) (fl->featuretag&0xff) );
		for ( sl= fl->scripts; sl!=NULL; sl = sl->next ) {
		    fprintf( sfd, "'%c%c%c%c' <",
			    (int) (sl->script>>24), (int) ((sl->script>>16)&0xff),
			    (int) ((sl->script>>8)&0xff), (int) (sl->script&0xff) );
		    for ( i=0; i<sl->lang_cnt; ++i ) {
			uint32 lang = i<MAX_LANG ? sl->langs[i] : sl->morelangs[i-MAX_LANG];
			fprintf( sfd, "'%c%c%c%c' ",
				(int) (lang>>24), (int) ((lang>>16)&0xff),
				(int) ((lang>>8)&0xff), (int) (lang&0xff) );
		    }
		    fprintf( sfd, "> " );
		}
		fprintf( sfd, ") " );
	    }
	    fprintf( sfd, "]\n" );
	}
    }
}

int SFD_DumpSplineFontMetadata( FILE *sfd, SplineFont *sf )
{
    int i, j;
    struct ttflangname *ln;
    struct ttf_table *tab;
    KernClass *kc;
    FPST *fpst;
    ASM *sm;
    int isv;
    int err = false;
    int isgpos;
    OTLookup *otl;
    struct lookup_subtable *sub;
    FeatureScriptLangList *fl;
    struct scriptlanglist *sl;

    fprintf(sfd, "FontName: %s\n", sf->fontname );
    if ( sf->fullname!=NULL )
	fprintf(sfd, "FullName: %s\n", sf->fullname );
    if ( sf->familyname!=NULL )
	fprintf(sfd, "FamilyName: %s\n", sf->familyname );
    if ( sf->weight!=NULL )
	fprintf(sfd, "Weight: %s\n", sf->weight );
    if ( sf->copyright!=NULL )
	putstring(sfd, "Copyright: ", sf->copyright );
    if ( sf->comments!=NULL ) {
	fprintf( sfd, "UComments: " );
	SFDDumpUTF7Str(sfd,sf->comments);
	putc('\n',sfd);
    }
    if ( sf->fontlog!=NULL ) {
	fprintf( sfd, "FontLog: " );
	SFDDumpUTF7Str(sfd,sf->fontlog);
	putc('\n',sfd);
    }

    if ( sf->version!=NULL )
	fprintf(sfd, "Version: %s\n", sf->version );
    if ( sf->styleMapFamilyName!=NULL )
	fprintf(sfd, "StyleMapFamilyName: %s\n", sf->styleMapFamilyName );
    if ( sf->fondname!=NULL )
	fprintf(sfd, "FONDName: %s\n", sf->fondname );
    if ( sf->defbasefilename!=NULL )
	fprintf(sfd, "DefaultBaseFilename: %s\n", sf->defbasefilename );
    if ( sf->strokewidth!=0 )
	fprintf(sfd, "StrokeWidth: %g\n", (double) sf->strokewidth );
    fprintf(sfd, "ItalicAngle: %g\n", (double) sf->italicangle );
    fprintf(sfd, "UnderlinePosition: %g\n", (double) sf->upos );
    fprintf(sfd, "UnderlineWidth: %g\n", (double) sf->uwidth );
    fprintf(sfd, "Ascent: %d\n", sf->ascent );
    fprintf(sfd, "Descent: %d\n", sf->descent );
    fprintf(sfd, "InvalidEm: %d\n", sf->invalidem );
    if ( sf->sfntRevision!=sfntRevisionUnset )
	fprintf(sfd, "sfntRevision: 0x%08x\n", sf->sfntRevision );
    if ( sf->woffMajor!=woffUnset ) {
	fprintf(sfd, "woffMajor: %d\n", sf->woffMajor );
	fprintf(sfd, "woffMinor: %d\n", sf->woffMinor );
    }
    if ( sf->woffMetadata!=NULL ) {
	fprintf( sfd, "woffMetadata: " );
	SFDDumpUTF7Str(sfd,sf->woffMetadata);
	putc('\n',sfd);
    }
    if ( sf->ufo_ascent!=0 )
	fprintf(sfd, "UFOAscent: %g\n", (double) sf->ufo_ascent );
    if ( sf->ufo_descent!=0 )
	fprintf(sfd, "UFODescent: %g\n", (double) sf->ufo_descent );
    fprintf(sfd, "LayerCount: %d\n", sf->layer_cnt );
    for ( i=0; i<sf->layer_cnt; ++i ) {
	fprintf( sfd, "Layer: %d %d ", i, sf->layers[i].order2/*, sf->layers[i].background*/ );
	SFDDumpUTF7Str(sfd,sf->layers[i].name);
	fprintf( sfd, " %d", sf->layers[i].background );
	if (sf->layers[i].ufo_path != NULL) { putc(' ',sfd); SFDDumpUTF7Str(sfd,sf->layers[i].ufo_path); }
	putc('\n',sfd);
    }
    // TODO: Output U. F. O. layer path.
    if (sf->preferred_kerning != 0) fprintf(sfd, "PreferredKerning: %d\n", sf->preferred_kerning);
    if ( sf->strokedfont )
	fprintf(sfd, "StrokedFont: %d\n", sf->strokedfont );
    else if ( sf->multilayer )
	fprintf(sfd, "MultiLayer: %d\n", sf->multilayer );
    if ( sf->hasvmetrics )
	fprintf(sfd, "HasVMetrics: %d\n", sf->hasvmetrics );
    if ( sf->use_xuid && sf->changed_since_xuidchanged )
	fprintf(sfd, "NeedsXUIDChange: 1\n" );
    if ( sf->xuid!=NULL )
	fprintf(sfd, "XUID: %s\n", sf->xuid );
    if ( sf->uniqueid!=0 )
	fprintf(sfd, "UniqueID: %d\n", sf->uniqueid );
    if ( sf->use_xuid )
	fprintf(sfd, "UseXUID: 1\n" );
    if ( sf->use_uniqueid )
	fprintf(sfd, "UseUniqueID: 1\n" );
    if ( sf->horiz_base!=NULL )
	SFDDumpBase(sfd,"BaseHoriz:",sf->horiz_base);
    if ( sf->vert_base!=NULL )
	SFDDumpBase(sfd,"BaseVert:",sf->vert_base);
    if ( sf->pfminfo.stylemap!=-1 )
	fprintf(sfd, "StyleMap: 0x%04x\n", sf->pfminfo.stylemap );
    if ( sf->pfminfo.fstype!=-1 )
	fprintf(sfd, "FSType: %d\n", sf->pfminfo.fstype );
    fprintf(sfd, "OS2Version: %d\n", sf->os2_version );
    fprintf(sfd, "OS2_WeightWidthSlopeOnly: %d\n", sf->weight_width_slope_only );
    fprintf(sfd, "OS2_UseTypoMetrics: %d\n", sf->use_typo_metrics );
    fprintf(sfd, "CreationTime: %lld\n", sf->creationtime );
    fprintf(sfd, "ModificationTime: %lld\n", sf->modificationtime );
    if ( sf->pfminfo.pfmset ) {
	fprintf(sfd, "PfmFamily: %d\n", sf->pfminfo.pfmfamily );
	fprintf(sfd, "TTFWeight: %d\n", sf->pfminfo.weight );
	fprintf(sfd, "TTFWidth: %d\n", sf->pfminfo.width );
	fprintf(sfd, "LineGap: %d\n", sf->pfminfo.linegap );
	fprintf(sfd, "VLineGap: %d\n", sf->pfminfo.vlinegap );
	/*putc('\n',sfd);*/
    }
    if ( sf->pfminfo.panose_set ) {
	fprintf(sfd, "Panose:" );
	for ( i=0; i<10; ++i )
	    fprintf( sfd, " %d", sf->pfminfo.panose[i]);
	putc('\n',sfd);
    }
    fprintf(sfd, "OS2TypoAscent: %d\n", sf->pfminfo.os2_typoascent );
    fprintf(sfd, "OS2TypoAOffset: %d\n", sf->pfminfo.typoascent_add );
    fprintf(sfd, "OS2TypoDescent: %d\n", sf->pfminfo.os2_typodescent );
    fprintf(sfd, "OS2TypoDOffset: %d\n", sf->pfminfo.typodescent_add );
    fprintf(sfd, "OS2TypoLinegap: %d\n", sf->pfminfo.os2_typolinegap );
    fprintf(sfd, "OS2WinAscent: %d\n", sf->pfminfo.os2_winascent );
    fprintf(sfd, "OS2WinAOffset: %d\n", sf->pfminfo.winascent_add );
    fprintf(sfd, "OS2WinDescent: %d\n", sf->pfminfo.os2_windescent );
    fprintf(sfd, "OS2WinDOffset: %d\n", sf->pfminfo.windescent_add );
    fprintf(sfd, "HheadAscent: %d\n", sf->pfminfo.hhead_ascent );
    fprintf(sfd, "HheadAOffset: %d\n", sf->pfminfo.hheadascent_add );
    fprintf(sfd, "HheadDescent: %d\n", sf->pfminfo.hhead_descent );
    fprintf(sfd, "HheadDOffset: %d\n", sf->pfminfo.hheaddescent_add );
    if ( sf->pfminfo.subsuper_set ) {
	fprintf(sfd, "OS2SubXSize: %d\n", sf->pfminfo.os2_subxsize );
	fprintf(sfd, "OS2SubYSize: %d\n", sf->pfminfo.os2_subysize );
	fprintf(sfd, "OS2SubXOff: %d\n", sf->pfminfo.os2_subxoff );
	fprintf(sfd, "OS2SubYOff: %d\n", sf->pfminfo.os2_subyoff );
	fprintf(sfd, "OS2SupXSize: %d\n", sf->pfminfo.os2_supxsize );
	fprintf(sfd, "OS2SupYSize: %d\n", sf->pfminfo.os2_supysize );
	fprintf(sfd, "OS2SupXOff: %d\n", sf->pfminfo.os2_supxoff );
	fprintf(sfd, "OS2SupYOff: %d\n", sf->pfminfo.os2_supyoff );
	fprintf(sfd, "OS2StrikeYSize: %d\n", sf->pfminfo.os2_strikeysize );
	fprintf(sfd, "OS2StrikeYPos: %d\n", sf->pfminfo.os2_strikeypos );
    }
    if ( sf->pfminfo.os2_capheight!=0 )
    fprintf(sfd, "OS2CapHeight: %d\n", sf->pfminfo.os2_capheight );
    if ( sf->pfminfo.os2_xheight!=0 )
    fprintf(sfd, "OS2XHeight: %d\n", sf->pfminfo.os2_xheight );
    if ( sf->pfminfo.os2_family_class!=0 )
	fprintf(sfd, "OS2FamilyClass: %d\n", sf->pfminfo.os2_family_class );
    if ( sf->pfminfo.os2_vendor[0]!='\0' ) {
	fprintf(sfd, "OS2Vendor: '%c%c%c%c'\n",
		sf->pfminfo.os2_vendor[0], sf->pfminfo.os2_vendor[1],
		sf->pfminfo.os2_vendor[2], sf->pfminfo.os2_vendor[3] );
    }
    if ( sf->pfminfo.hascodepages )
	fprintf(sfd, "OS2CodePages: %08x.%08x\n", sf->pfminfo.codepages[0], sf->pfminfo.codepages[1]);
    if ( sf->pfminfo.hasunicoderanges )
	fprintf(sfd, "OS2UnicodeRanges: %08x.%08x.%08x.%08x\n",
		sf->pfminfo.unicoderanges[0], sf->pfminfo.unicoderanges[1],
		sf->pfminfo.unicoderanges[2], sf->pfminfo.unicoderanges[3] );
    if ( sf->macstyle!=-1 )
	fprintf(sfd, "MacStyle: %d\n", sf->macstyle );
    /* Must come before any kerning classes, anchor classes, conditional psts */
    /* state machines, psts, kerning pairs, etc. */
    for ( isgpos=0; isgpos<2; ++isgpos ) {
	for ( otl = isgpos ? sf->gpos_lookups : sf->gsub_lookups; otl!=NULL; otl = otl->next ) {
	    fprintf( sfd, "Lookup: %d %d %d ", otl->lookup_type, otl->lookup_flags, otl->store_in_afm );
	    SFDDumpUTF7Str(sfd,otl->lookup_name);
	    fprintf( sfd, " { " );
	    for ( sub=otl->subtables; sub!=NULL; sub=sub->next ) {
		SFDDumpUTF7Str(sfd,sub->subtable_name);
		putc(' ',sfd);
		if ( otl->lookup_type==gsub_single && sub->suffix!=NULL ) {
		    putc('(',sfd);
		    SFDDumpUTF7Str(sfd,sub->suffix);
		    putc(')',sfd);
		} else if ( otl->lookup_type==gpos_pair && sub->vertical_kerning )
		    fprintf(sfd,"(1)");
		if ( otl->lookup_type==gpos_pair && (sub->separation!=0 || sub->kerning_by_touch))
		    fprintf(sfd,"[%d,%d,%d]", sub->separation, sub->minkern, sub->kerning_by_touch+2*sub->onlyCloser+4*sub->dontautokern );
		putc(' ',sfd);
	    }
	    fprintf( sfd, "} [" );
	    for ( fl=otl->features; fl!=NULL; fl=fl->next ) {
		if ( fl->ismac )
		    fprintf( sfd, "<%d,%d> (",
			    (int) (fl->featuretag>>16),
			    (int) (fl->featuretag&0xffff));
		else
		    fprintf( sfd, "'%c%c%c%c' (",
			    (int) (fl->featuretag>>24), (int) ((fl->featuretag>>16)&0xff),
			    (int) ((fl->featuretag>>8)&0xff), (int) (fl->featuretag&0xff) );
		for ( sl= fl->scripts; sl!=NULL; sl = sl->next ) {
		    fprintf( sfd, "'%c%c%c%c' <",
			    (int) (sl->script>>24), (int) ((sl->script>>16)&0xff),
			    (int) ((sl->script>>8)&0xff), (int) (sl->script&0xff) );
		    for ( i=0; i<sl->lang_cnt; ++i ) {
			uint32 lang = i<MAX_LANG ? sl->langs[i] : sl->morelangs[i-MAX_LANG];
			fprintf( sfd, "'%c%c%c%c' ",
				(int) (lang>>24), (int) ((lang>>16)&0xff),
				(int) ((lang>>8)&0xff), (int) (lang&0xff) );
		    }
		    fprintf( sfd, "> " );
		}
		fprintf( sfd, ") " );
	    }
	    fprintf( sfd, "]\n" );
	}
    }

    if ( sf->mark_class_cnt!=0 ) {
	fprintf( sfd, "MarkAttachClasses: %d\n", sf->mark_class_cnt );
	for ( i=1; i<sf->mark_class_cnt; ++i ) {	/* Class 0 is unused */
	    SFDDumpUTF7Str(sfd, sf->mark_class_names[i]);
	    putc(' ',sfd);
	    if ( sf->mark_classes[i]!=NULL )
		fprintf( sfd, "%d %s\n", (int) strlen(sf->mark_classes[i]),
			sf->mark_classes[i] );
	    else
		fprintf( sfd, "0 \n" );
	}
    }
    if ( sf->mark_set_cnt!=0 ) {
	fprintf( sfd, "MarkAttachSets: %d\n", sf->mark_set_cnt );
	for ( i=0; i<sf->mark_set_cnt; ++i ) {	/* Set 0 is used */
	    SFDDumpUTF7Str(sfd, sf->mark_set_names[i]);
	    putc(' ',sfd);
	    if ( sf->mark_sets[i]!=NULL )
		fprintf( sfd, "%d %s\n", (int) strlen(sf->mark_sets[i]),
			sf->mark_sets[i] );
	    else
		fprintf( sfd, "0 \n" );
	}
    }

    fprintf( sfd, "DEI: 91125\n" );
    for ( isv=0; isv<2; ++isv ) {
	for ( kc=isv ? sf->vkerns : sf->kerns; kc!=NULL; kc = kc->next ) {
	  if (kc->firsts_names == NULL && kc->seconds_names == NULL && kc->firsts_flags == NULL && kc->seconds_flags == NULL) {
	    fprintf( sfd, "%s: %d%s %d ", isv ? "VKernClass2" : "KernClass2",
		    kc->first_cnt, kc->firsts[0]!=NULL?"+":"",
		    kc->second_cnt );
	    SFDDumpUTF7Str(sfd,kc->subtable->subtable_name);
	    putc('\n',sfd);
	    if ( kc->firsts[0]!=NULL )
	      fprintf( sfd, " %d %s\n", (int)strlen(kc->firsts[0]),
		       kc->firsts[0]);
	    for ( i=1; i<kc->first_cnt; ++i )
	      fprintf( sfd, " %d %s\n", (int)strlen(kc->firsts[i]),
		       kc->firsts[i]);
	    for ( i=1; i<kc->second_cnt; ++i )
	      fprintf( sfd, " %d %s\n", (int)strlen(kc->seconds[i]),
		       kc->seconds[i]);
	    for ( i=0; i<kc->first_cnt*kc->second_cnt; ++i ) {
		fprintf( sfd, " %d", kc->offsets[i]);
		putc(' ',sfd);
		SFDDumpDeviceTable(sfd,&kc->adjusts[i]);
	    }
	    fprintf( sfd, "\n" );
	  } else {
	    fprintf( sfd, "%s: %d%s %d ", isv ? "VKernClass3" : "KernClass3",
		    kc->first_cnt, kc->firsts[0]!=NULL?"+":"",
		    kc->second_cnt );
	    SFDDumpUTF7Str(sfd,kc->subtable->subtable_name);
	    putc('\n',sfd);
	    if ( kc->firsts[0]!=NULL ) {
	      fprintf( sfd, " %d ", ((kc->firsts_flags && kc->firsts_flags[0]) ? kc->firsts_flags[0] : 0));
	      SFDDumpUTF7Str(sfd, ((kc->firsts_names && kc->firsts_names[0]) ? kc->firsts_names[0] : ""));
	      fprintf( sfd, " " );
	      SFDDumpUTF7Str(sfd,kc->firsts[0]);
	      fprintf( sfd, "\n" );
	    }
	    for ( i=1; i<kc->first_cnt; ++i ) {
	      fprintf( sfd, " %d ", ((kc->firsts_flags && kc->firsts_flags[i]) ? kc->firsts_flags[i] : 0));
	      SFDDumpUTF7Str(sfd, ((kc->firsts_names && kc->firsts_names[i]) ? kc->firsts_names[i] : ""));
	      fprintf( sfd, " " );
	      SFDDumpUTF7Str(sfd,kc->firsts[i]);
	      fprintf( sfd, "\n" );
	    }
	    for ( i=1; i<kc->second_cnt; ++i ) {
	      fprintf( sfd, " %d ", ((kc->seconds_flags && kc->seconds_flags[i]) ? kc->seconds_flags[i] : 0));
	      SFDDumpUTF7Str(sfd, ((kc->seconds_names && kc->seconds_names[i]) ? kc->seconds_names[i] : ""));
	      fprintf( sfd, " " );
	      SFDDumpUTF7Str(sfd,kc->seconds[i]);
	      fprintf( sfd, "\n" );
	    }
	    for ( i=0; i<kc->first_cnt*kc->second_cnt; ++i ) {
		fprintf( sfd, " %d %d", ((kc->offsets_flags && kc->offsets_flags[i]) ? kc->offsets_flags[i] : 0), kc->offsets[i]);
		putc(' ',sfd);
		SFDDumpDeviceTable(sfd,&kc->adjusts[i]);
	    }
	    fprintf( sfd, "\n" );
	  }
	}
    }
    for ( fpst=sf->possub; fpst!=NULL; fpst=fpst->next ) {
	static const char *keywords[] = { "ContextPos2:", "ContextSub2:", "ChainPos2:", "ChainSub2:", "ReverseChain2:", NULL };
	static const char *formatkeys[] = { "glyph", "class", "coverage", "revcov", NULL };
	fprintf( sfd, "%s %s ", keywords[fpst->type-pst_contextpos],
		formatkeys[fpst->format] );
	SFDDumpUTF7Str(sfd,fpst->subtable->subtable_name);
	fprintf( sfd, " %d %d %d %d\n",
		fpst->nccnt, fpst->bccnt, fpst->fccnt, fpst->rule_cnt );
	if ( fpst->nccnt>0 && fpst->nclass[0]!=NULL )
	  fprintf( sfd, "  Class0: %d %s\n", (int)strlen(fpst->nclass[0]),
		   fpst->nclass[0]);
	for ( i=1; i<fpst->nccnt; ++i )
	  fprintf( sfd, "  Class: %d %s\n", (int)strlen(fpst->nclass[i]),
		   fpst->nclass[i]);
	for ( i=1; i<fpst->bccnt; ++i )
	  fprintf( sfd, "  BClass: %d %s\n", (int)strlen(fpst->bclass[i]),
		   fpst->bclass[i]);
	for ( i=1; i<fpst->fccnt; ++i )
	  fprintf( sfd, "  FClass: %d %s\n", (int)strlen(fpst->fclass[i]),
		   fpst->fclass[i]);
	for ( i=0; i<fpst->rule_cnt; ++i ) {
	    switch ( fpst->format ) {
	      case pst_glyphs:
		fprintf( sfd, " String: %d %s\n",
			 (int)strlen(fpst->rules[i].u.glyph.names),
			 fpst->rules[i].u.glyph.names);
		if ( fpst->rules[i].u.glyph.back!=NULL )
		  fprintf( sfd, " BString: %d %s\n",
			   (int)strlen(fpst->rules[i].u.glyph.back),
			   fpst->rules[i].u.glyph.back);
		else
		    fprintf( sfd, " BString: 0\n");
		if ( fpst->rules[i].u.glyph.fore!=NULL )
		  fprintf( sfd, " FString: %d %s\n",
			   (int)strlen(fpst->rules[i].u.glyph.fore),
			   fpst->rules[i].u.glyph.fore);
		else
		    fprintf( sfd, " FString: 0\n");
	      break;
	      case pst_class:
		fprintf( sfd, " %d %d %d\n  ClsList:", fpst->rules[i].u.class.ncnt, fpst->rules[i].u.class.bcnt, fpst->rules[i].u.class.fcnt );
		for ( j=0; j<fpst->rules[i].u.class.ncnt; ++j )
		    fprintf( sfd, " %d", fpst->rules[i].u.class.nclasses[j]);
		fprintf( sfd, "\n  BClsList:" );
		for ( j=0; j<fpst->rules[i].u.class.bcnt; ++j )
		    fprintf( sfd, " %d", fpst->rules[i].u.class.bclasses[j]);
		fprintf( sfd, "\n  FClsList:" );
		for ( j=0; j<fpst->rules[i].u.class.fcnt; ++j )
		    fprintf( sfd, " %d", fpst->rules[i].u.class.fclasses[j]);
		fprintf( sfd, "\n" );
	      break;
	      case pst_coverage:
	      case pst_reversecoverage:
		fprintf( sfd, " %d %d %d\n", fpst->rules[i].u.coverage.ncnt, fpst->rules[i].u.coverage.bcnt, fpst->rules[i].u.coverage.fcnt );
		for ( j=0; j<fpst->rules[i].u.coverage.ncnt; ++j )
		  fprintf( sfd, "  Coverage: %d %s\n",
			   (int)strlen(fpst->rules[i].u.coverage.ncovers[j]),
			   fpst->rules[i].u.coverage.ncovers[j]);
		for ( j=0; j<fpst->rules[i].u.coverage.bcnt; ++j )
		  fprintf( sfd, "  BCoverage: %d %s\n",
			   (int)strlen(fpst->rules[i].u.coverage.bcovers[j]),
			   fpst->rules[i].u.coverage.bcovers[j]);
		for ( j=0; j<fpst->rules[i].u.coverage.fcnt; ++j )
		  fprintf( sfd, "  FCoverage: %d %s\n",
			   (int)strlen(fpst->rules[i].u.coverage.fcovers[j]),
			   fpst->rules[i].u.coverage.fcovers[j]);
	      break;
	      default:
	      break;
	    }
	    switch ( fpst->format ) {
	      case pst_glyphs:
	      case pst_class:
	      case pst_coverage:
		fprintf( sfd, " %d\n", fpst->rules[i].lookup_cnt );
		for ( j=0; j<fpst->rules[i].lookup_cnt; ++j ) {
		    fprintf( sfd, "  SeqLookup: %d ",
			    fpst->rules[i].lookups[j].seq );
		    SFDDumpUTF7Str(sfd,fpst->rules[i].lookups[j].lookup->lookup_name);
		    putc('\n',sfd);
		}
	      break;
	      case pst_reversecoverage:
		fprintf( sfd, "  Replace: %d %s\n",
			 (int)strlen(fpst->rules[i].u.rcoverage.replacements),
			 fpst->rules[i].u.rcoverage.replacements);
	      break;
	      default:
	      break;
	    }
	}
	/* It would make more sense to output these up near the classes */
	/*  but that would break backwards compatibility (old parsers will */
	/*  ignore these entries if they are at the end, new parsers will */
	/*  read them */
	SFDFpstClassNamesOut(sfd,fpst->nccnt,fpst->nclassnames,"ClassNames");
	SFDFpstClassNamesOut(sfd,fpst->bccnt,fpst->bclassnames,"BClassNames");
	SFDFpstClassNamesOut(sfd,fpst->fccnt,fpst->fclassnames,"FClassNames");
	fprintf( sfd, "EndFPST\n" );
    }
    struct ff_glyphclasses *grouptmp;
    for ( grouptmp = sf->groups; grouptmp != NULL; grouptmp = grouptmp->next ) {
      fprintf(sfd, "Group: ");
      SFDDumpUTF7Str(sfd, grouptmp->classname); fprintf(sfd, " ");
      SFDDumpUTF7Str(sfd, grouptmp->glyphs); fprintf(sfd, "\n");
    }
    struct ff_rawoffsets *groupkerntmp;
    for ( groupkerntmp = sf->groupkerns; groupkerntmp != NULL; groupkerntmp = groupkerntmp->next ) {
      fprintf(sfd, "GroupKern: ");
      SFDDumpUTF7Str(sfd, groupkerntmp->left); fprintf(sfd, " ");
      SFDDumpUTF7Str(sfd, groupkerntmp->right); fprintf(sfd, " ");
      fprintf(sfd, "%d\n", groupkerntmp->offset);
    }
    for ( groupkerntmp = sf->groupvkerns; groupkerntmp != NULL; groupkerntmp = groupkerntmp->next ) {
      fprintf(sfd, "GroupVKern: ");
      SFDDumpUTF7Str(sfd, groupkerntmp->left); fprintf(sfd, " ");
      SFDDumpUTF7Str(sfd, groupkerntmp->right); fprintf(sfd, " ");
      fprintf(sfd, "%d\n", groupkerntmp->offset);
    }
    for ( sm=sf->sm; sm!=NULL; sm=sm->next ) {
	static const char *keywords[] = { "MacIndic2:", "MacContext2:", "MacLigature2:", "unused", "MacSimple2:", "MacInsert2:",
	    "unused", "unused", "unused", "unused", "unused", "unused",
	    "unused", "unused", "unused", "unused", "unused", "MacKern2:",
	    NULL };
	fprintf( sfd, "%s ", keywords[sm->type-asm_indic] );
	SFDDumpUTF7Str(sfd,sm->subtable->subtable_name);
	fprintf( sfd, " %d %d %d\n", sm->flags, sm->class_cnt, sm->state_cnt );
	for ( i=4; i<sm->class_cnt; ++i )
	  fprintf( sfd, "  Class: %d %s\n", (int)strlen(sm->classes[i]),
		   sm->classes[i]);
	for ( i=0; i<sm->class_cnt*sm->state_cnt; ++i ) {
	    fprintf( sfd, " %d %d ", sm->state[i].next_state, sm->state[i].flags );
	    if ( sm->type==asm_context ) {
		if ( sm->state[i].u.context.mark_lookup==NULL )
		    putc('~',sfd);
		else
		    SFDDumpUTF7Str(sfd,sm->state[i].u.context.mark_lookup->lookup_name);
		putc(' ',sfd);
		if ( sm->state[i].u.context.cur_lookup==0 )
		    putc('~',sfd);
		else
		    SFDDumpUTF7Str(sfd,sm->state[i].u.context.cur_lookup->lookup_name);
		putc(' ',sfd);
	    } else if ( sm->type == asm_insert ) {
		if ( sm->state[i].u.insert.mark_ins==NULL )
		    fprintf( sfd, "0 ");
		else
		  fprintf( sfd, "%d %s ",
			   (int)strlen(sm->state[i].u.insert.mark_ins),
			   sm->state[i].u.insert.mark_ins );
		if ( sm->state[i].u.insert.cur_ins==NULL )
		    fprintf( sfd, "0 ");
		else
		  fprintf( sfd, "%d %s ",
			   (int)strlen(sm->state[i].u.insert.cur_ins),
			   sm->state[i].u.insert.cur_ins );
	    } else if ( sm->type == asm_kern ) {
		fprintf( sfd, "%d ", sm->state[i].u.kern.kcnt );
		for ( j=0; j<sm->state[i].u.kern.kcnt; ++j )
		    fprintf( sfd, "%d ", sm->state[i].u.kern.kerns[j]);
	    }
	    putc('\n',sfd);
	}
	fprintf( sfd, "EndASM\n" );
    }
    SFDDumpMacFeat(sfd,sf->features);
    SFDDumpJustify(sfd,sf);
    for ( tab = sf->ttf_tables; tab!=NULL ; tab = tab->next )
	SFDDumpTtfTable(sfd,tab,sf);
    for ( tab = sf->ttf_tab_saved; tab!=NULL ; tab = tab->next )
	SFDDumpTtfTable(sfd,tab,sf);
    for ( ln = sf->names; ln!=NULL; ln=ln->next )
	SFDDumpLangName(sfd,ln);
    if ( sf->gasp_cnt!=0 )
	SFDDumpGasp(sfd,sf);
    if ( sf->design_size!=0 )
	SFDDumpDesignSize(sfd,sf);
    if ( sf->feat_names!=NULL )
	SFDDumpOtfFeatNames(sfd,sf);

return( err );
}


static int SFD_Dump( FILE *sfd, SplineFont *sf, EncMap *map, EncMap *normal,
		     int todir, char *dirname)
{
    int i, realcnt;
    BDFFont *bdf;
    int *newgids = NULL;
    int err = false;

    if ( normal!=NULL )
	map = normal;

    SFD_DumpSplineFontMetadata( sfd, sf ); //, map, normal, todir, dirname );

    if ( sf->MATH!=NULL ) {
	struct MATH *math = sf->MATH;
	for ( i=0; math_constants_descriptor[i].script_name!=NULL; ++i ) {
	    fprintf( sfd, "MATH:%s: %d", math_constants_descriptor[i].script_name,
		    *((int16 *) (((char *) (math)) + math_constants_descriptor[i].offset)) );
	    if ( math_constants_descriptor[i].devtab_offset>=0 ) {
		DeviceTable **devtab = (DeviceTable **) (((char *) (math)) + math_constants_descriptor[i].devtab_offset );
		putc(' ',sfd);
		SFDDumpDeviceTable(sfd,*devtab);
	    }
	    putc('\n',sfd);
	}
    }
    if ( sf->python_persistent!=NULL )
	SFDPickleMe(sfd,sf->python_persistent, sf->python_persistent_has_lists);
    if ( sf->subfontcnt!=0 ) {
	/* CID fonts have no encodings, they have registry info instead */
	fprintf(sfd, "Registry: %s\n", sf->cidregistry );
	fprintf(sfd, "Ordering: %s\n", sf->ordering );
	fprintf(sfd, "Supplement: %d\n", sf->supplement );
	fprintf(sfd, "CIDVersion: %g\n", (double) sf->cidversion );	/* This is a number whereas "version" is a string */
    } else
	SFDDumpEncoding(sfd,map->enc,"Encoding");
    if ( normal!=NULL )
	fprintf(sfd, "Compacted: 1\n" );
    fprintf( sfd, "UnicodeInterp: %s\n", unicode_interp_names[sf->uni_interp]);
    fprintf( sfd, "NameList: %s\n", sf->for_new_glyphs->title );

    if ( map->remap!=NULL ) {
	struct remap *remap;
	int n;
	for ( n=0,remap = map->remap; remap->infont!=-1; ++n, ++remap );
	fprintf( sfd, "RemapN: %d\n", n );
	for ( remap = map->remap; remap->infont!=-1; ++remap )
	    fprintf(sfd, "Remap: %x %x %d\n", (int) remap->firstenc, (int) remap->lastenc, (int) remap->infont );
    }
    if ( sf->display_size!=0 )
	fprintf( sfd, "DisplaySize: %d\n", sf->display_size );
    if ( sf->display_layer!=ly_fore )
	fprintf( sfd, "DisplayLayer: %d\n", sf->display_layer );
    fprintf( sfd, "AntiAlias: %d\n", sf->display_antialias );
    fprintf( sfd, "FitToEm: %d\n", sf->display_bbsized );
    if ( sf->extrema_bound!=0 )
	fprintf( sfd, "ExtremaBound: %d\n", sf->extrema_bound );
    if ( sf->width_separation!=0 )
	fprintf( sfd, "WidthSeparation: %d\n", sf->width_separation );
    {
	int rc, cc, te;
	if ( (te = FVWinInfo(sf->fv,&cc,&rc))!= -1 )
	    fprintf( sfd, "WinInfo: %d %d %d\n", te, cc, rc );
	else if ( sf->top_enc!=-1 )
	    fprintf( sfd, "WinInfo: %d %d %d\n", sf->top_enc, sf->desired_col_cnt,
		sf->desired_row_cnt);
    }
    if ( sf->onlybitmaps!=0 )
	fprintf( sfd, "OnlyBitmaps: %d\n", sf->onlybitmaps );
    if ( sf->private!=NULL )
	SFDDumpPrivate(sfd,sf->private);
#if HANYANG
    if ( sf->rules!=NULL )
	SFDDumpCompositionRules(sfd,sf->rules);
#endif
    if ( sf->grid.splines!=NULL ) {
	if ( sf->grid.order2 )
	    fprintf(sfd, "GridOrder2: %d\n", sf->grid.order2 );
	fprintf(sfd, "Grid\n" );
	SFDDumpSplineSet(sfd,sf->grid.splines);
    }
    if ( sf->texdata.type!=tex_unset ) {
	fprintf(sfd, "TeXData: %d %d", (int) sf->texdata.type, (int) ((sf->design_size<<19)+2)/5 );
	for ( i=0; i<22; ++i )
	    fprintf(sfd, " %d", (int) sf->texdata.params[i]);
	putc('\n',sfd);
    }
    if ( sf->anchor!=NULL ) {
	AnchorClass *an;
	fprintf(sfd, "AnchorClass2:");
	for ( an=sf->anchor; an!=NULL; an=an->next ) {
	    putc(' ',sfd);
	    SFDDumpUTF7Str(sfd,an->name);
            if ( an->subtable!=NULL ) {
	        putc(' ',sfd);
	        SFDDumpUTF7Str(sfd,an->subtable->subtable_name);
            }
            else
                fprintf(sfd, "\"\" ");
	}
	putc('\n',sfd);
    }

    if ( sf->subfontcnt!=0 ) {
	if ( todir ) {
	    for ( i=0; i<sf->subfontcnt; ++i ) {
		char *subfont = malloc(strlen(dirname)+1+strlen(sf->subfonts[i]->fontname)+20);
		char *fontprops;
		FILE *ssfd;
		sprintf( subfont,"%s/%s" SUBFONT_EXT, dirname, sf->subfonts[i]->fontname );
		GFileMkDir(subfont);
		fontprops = malloc(strlen(subfont)+strlen("/" FONT_PROPS)+1);
		strcpy(fontprops,subfont); strcat(fontprops,"/" FONT_PROPS);
		ssfd = fopen( fontprops,"w");
		if ( ssfd!=NULL ) {
		    err |= SFD_Dump(ssfd,sf->subfonts[i],map,NULL,todir,subfont);
		    if ( ferror(ssfd) ) err = true;
		    if ( fclose(ssfd)) err = true;
		} else
		    err = true;
		free(fontprops);
		free(subfont);
	    }
	} else {
	    int max;
	    for ( i=max=0; i<sf->subfontcnt; ++i )
		if ( max<sf->subfonts[i]->glyphcnt )
		    max = sf->subfonts[i]->glyphcnt;
	    fprintf(sfd, "BeginSubFonts: %d %d\n", sf->subfontcnt, max );
	    for ( i=0; i<sf->subfontcnt; ++i )
		SFD_Dump(sfd,sf->subfonts[i],map,NULL,false, NULL);
	    fprintf(sfd, "EndSubFonts\n" );
	}
    } else {
	int enccount = map->enccount;
	if ( sf->cidmaster!=NULL ) {
	    realcnt = -1;
	    enccount = sf->glyphcnt;
	} else {
	    realcnt = 0;
	    for ( i=0; i<sf->glyphcnt; ++i ) if ( !SFDOmit(sf->glyphs[i]) )
		++realcnt;
	    if ( realcnt!=sf->glyphcnt ) {
		newgids = malloc(sf->glyphcnt*sizeof(int));
		realcnt = 0;
		for ( i=0; i<sf->glyphcnt; ++i )
		    if ( SFDOmit(sf->glyphs[i]) )
			newgids[i] = -1;
		    else
			newgids[i] = realcnt++;
	    }
	}
	if ( !todir )
	    fprintf(sfd, "BeginChars: %d %d\n",
	        enccount<map->enc->char_cnt? map->enc->char_cnt : enccount,
	        realcnt );
	for ( i=0; i<sf->glyphcnt; ++i ) {
	    if ( !SFDOmit(sf->glyphs[i]) ) {
		if ( !todir )
		SFDDumpChar(sfd,sf->glyphs[i],map,newgids,todir,1);
		else {
		    char *glyphfile = malloc(strlen(dirname)+2*strlen(sf->glyphs[i]->name)+20);
		    FILE *gsfd;
		    appendnames(glyphfile,dirname,"/",sf->glyphs[i]->name,GLYPH_EXT );
		    gsfd = fopen(glyphfile,"w");
		    if ( gsfd!=NULL ) {
			SFDDumpChar(gsfd,sf->glyphs[i],map,newgids,todir,1);
			if ( ferror(gsfd)) err = true;
			if ( fclose(gsfd)) err = true;
		    } else
			err = true;
		    free(glyphfile);
		}
	    }
	    ff_progress_next();
	}
	if ( !todir )
	    fprintf(sfd, "EndChars\n" );
    }

    if ( sf->bitmaps!=NULL )
	ff_progress_change_line2(_("Saving Bitmaps"));
    for ( bdf = sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	if ( todir ) {
	    char *strike = malloc(strlen(dirname)+1+20+20);
	    char *strikeprops;
	    FILE *ssfd;
	    sprintf( strike,"%s/%d" STRIKE_EXT, dirname, bdf->pixelsize );
	    GFileMkDir(strike);
	    strikeprops = malloc(strlen(strike)+strlen("/" STRIKE_PROPS)+1);
	    strcpy(strikeprops,strike); strcat(strikeprops,"/" STRIKE_PROPS);
	    ssfd = fopen( strikeprops,"w");
	    if ( ssfd!=NULL ) {
		err |= SFDDumpBitmapFont(ssfd,bdf,map,newgids,todir,strike);
		if ( ferror(ssfd) ) err = true;
		if ( fclose(ssfd)) err = true;
	    } else
		err = true;
	    free(strikeprops);
	    free(strike);
	} else
	    SFDDumpBitmapFont(sfd,bdf,map,newgids,todir,dirname);
    }
    fprintf(sfd, sf->cidmaster==NULL?"EndSplineFont\n":"EndSubSplineFont\n" );
    free(newgids);
return( err );
}

static int SFD_MIDump(SplineFont *sf,EncMap *map,char *dirname,	int mm_pos) {
    char *instance = malloc(strlen(dirname)+1+10+20);
    char *fontprops;
    FILE *ssfd;
    int err = false;

    /* I'd like to use the font name, but the order of the instances is */
    /*  crucial and I must enforce an ordering on them */
    sprintf( instance,"%s/mm%d" INSTANCE_EXT, dirname, mm_pos );
    GFileMkDir(instance);
    fontprops = malloc(strlen(instance)+strlen("/" FONT_PROPS)+1);
    strcpy(fontprops,instance); strcat(fontprops,"/" FONT_PROPS);
    ssfd = fopen( fontprops,"w");
    if ( ssfd!=NULL ) {
	err |= SFD_Dump(ssfd,sf,map,NULL,true,instance);
	if ( ferror(ssfd) ) err = true;
	if ( fclose(ssfd)) err = true;
    } else
	err = true;
    free(fontprops);
    free(instance);
return( err );
}

static int SFD_MMDump(FILE *sfd,SplineFont *sf,EncMap *map,EncMap *normal,
	int todir, char *dirname) {
    MMSet *mm = sf->mm;
    int max, i, j;
    int err = false;

    fprintf( sfd, "MMCounts: %d %d %d %d\n", mm->instance_count, mm->axis_count,
	    mm->apple, mm->named_instance_count );
    fprintf( sfd, "MMAxis:" );
    for ( i=0; i<mm->axis_count; ++i )
	fprintf( sfd, " %s", mm->axes[i]);
    putc('\n',sfd);
    fprintf( sfd, "MMPositions:" );
    for ( i=0; i<mm->axis_count*mm->instance_count; ++i )
	fprintf( sfd, " %g", (double) mm->positions[i]);
    putc('\n',sfd);
    fprintf( sfd, "MMWeights:" );
    for ( i=0; i<mm->instance_count; ++i )
	fprintf( sfd, " %g", (double) mm->defweights[i]);
    putc('\n',sfd);
    for ( i=0; i<mm->axis_count; ++i ) {
	fprintf( sfd, "MMAxisMap: %d %d", i, mm->axismaps[i].points );
	for ( j=0; j<mm->axismaps[i].points; ++j )
	    fprintf( sfd, " %g=>%g", (double) mm->axismaps[i].blends[j], (double) mm->axismaps[i].designs[j]);
	fputc('\n',sfd);
	SFDDumpMacName(sfd,mm->axismaps[i].axisnames);
    }
    if ( mm->cdv!=NULL ) {
	fprintf( sfd, "MMCDV:\n" );
	fputs(mm->cdv,sfd);
	fprintf( sfd, "\nEndMMSubroutine\n" );
    }
    if ( mm->ndv!=NULL ) {
	fprintf( sfd, "MMNDV:\n" );
	fputs(mm->ndv,sfd);
	fprintf( sfd, "\nEndMMSubroutine\n" );
    }
    for ( i=0; i<mm->named_instance_count; ++i ) {
	fprintf( sfd, "MMNamedInstance: %d ", i );
	for ( j=0; j<mm->axis_count; ++j )
	    fprintf( sfd, " %g", (double) mm->named_instances[i].coords[j]);
	fputc('\n',sfd);
	SFDDumpMacName(sfd,mm->named_instances[i].names);
    }

    if ( todir ) {
	for ( i=0; i<mm->instance_count; ++i )
	    err |= SFD_MIDump(mm->instances[i],map,dirname,i+1);
	err |= SFD_MIDump(mm->normal,map,dirname,0);
    } else {
	for ( i=max=0; i<mm->instance_count; ++i )
	    if ( max<mm->instances[i]->glyphcnt )
		max = mm->instances[i]->glyphcnt;
	fprintf(sfd, "BeginMMFonts: %d %d\n", mm->instance_count+1, max );
	for ( i=0; i<mm->instance_count; ++i )
	    SFD_Dump(sfd,mm->instances[i],map,normal,todir,dirname);
	SFD_Dump(sfd,mm->normal,map,normal,todir,dirname);
    }
    fprintf(sfd, "EndMMFonts\n" );
return( err );
}

static int SFDDump(FILE *sfd,SplineFont *sf,EncMap *map,EncMap *normal,
	int todir, char *dirname) {
    int i, realcnt;
    BDFFont *bdf;
    int err = false;

    realcnt = sf->glyphcnt;
    if ( sf->subfontcnt!=0 ) {
	for ( i=0; i<sf->subfontcnt; ++i )
	    if ( realcnt<sf->subfonts[i]->glyphcnt )
		realcnt = sf->subfonts[i]->glyphcnt;
    }
    for ( i=0, bdf = sf->bitmaps; bdf!=NULL; bdf=bdf->next, ++i );
    ff_progress_start_indicator(10,_("Saving..."),_("Saving Spline Font Database"),_("Saving Outlines"),
	    realcnt,i+1);
    ff_progress_enable_stop(false);
    double version = 3.1;
    if( !UndoRedoLimitToSave )
        version = 3.0;
    fprintf(sfd, "SplineFontDB: %.1f\n", version );
    if ( sf->mm != NULL )
	err = SFD_MMDump(sfd,sf->mm->normal,map,normal,todir,dirname);
    else
	err = SFD_Dump(sfd,sf,map,normal,todir,dirname);
    ff_progress_end_indicator();
return( err );
}

static void SFDirClean(char *filename) {
    DIR *dir;
    struct dirent *ent;
    char *buffer, *pt;

    unlink(filename);		/* Just in case it's a normal file, it shouldn't be, but just in case... */
    dir = opendir(filename);
    if ( dir==NULL )
return;
    buffer = malloc(strlen(filename)+1+NAME_MAX+1);
    while ( (ent = readdir(dir))!=NULL ) {
	if ( strcmp(ent->d_name,".")==0 || strcmp(ent->d_name,"..")==0 )
    continue;
	pt = strrchr(ent->d_name,EXT_CHAR);
	if ( pt==NULL )
    continue;
	sprintf( buffer,"%s/%s", filename, ent->d_name );
	if ( strcmp(pt,".props")==0 ||
		strcmp(pt,GLYPH_EXT)==0 ||
		strcmp(pt,BITMAP_EXT)==0 )
	    unlink( buffer );
	else if ( strcmp(pt,STRIKE_EXT)==0 ||
		strcmp(pt,SUBFONT_EXT)==0 ||
		strcmp(pt,INSTANCE_EXT)==0 )
	    SFDirClean(buffer);
	/* If there are filenames we don't recognize, leave them. They might contain version control info */
    }
    free(buffer);
    closedir(dir);
}

static void SFFinalDirClean(char *filename) {
    DIR *dir;
    struct dirent *ent;
    char *buffer, *markerfile, *pt;

    /* we did not unlink sub-directories in case they contained version control */
    /*  files. We did remove all our files from them, however.  If the user */
    /*  removed a bitmap strike or a cid-subfont those sub-dirs will now be */
    /*  empty. If the user didn't remove them then they will contain our marker */
    /*  files. So if we find a subdir with no marker files in it, remove it */
    dir = opendir(filename);
    if ( dir==NULL )
return;
    buffer = malloc(strlen(filename)+1+NAME_MAX+1);
    markerfile = malloc(strlen(filename)+2+2*NAME_MAX+1);
    while ( (ent = readdir(dir))!=NULL ) {
	if ( strcmp(ent->d_name,".")==0 || strcmp(ent->d_name,"..")==0 )
    continue;
	pt = strrchr(ent->d_name,EXT_CHAR);
	if ( pt==NULL )
    continue;
	sprintf( buffer,"%s/%s", filename, ent->d_name );
	if ( strcmp(pt,".strike")==0 ||
		strcmp(pt,SUBFONT_EXT)==0 ||
		strcmp(pt,INSTANCE_EXT)==0 ) {
	    if ( strcmp(pt,".strike")==0 )
		sprintf( markerfile,"%s/" STRIKE_PROPS, buffer );
	    else
		sprintf( markerfile,"%s/" FONT_PROPS, buffer );
	    if ( !GFileExists(markerfile)) {
		GFileRemove(buffer, false);
	    }
	}
    }
    free(buffer);
    free(markerfile);
    closedir(dir);
}

int SFDWrite(char *filename,SplineFont *sf,EncMap *map,EncMap *normal,int todir) {
    FILE *sfd;
    int i, gc;
    char *tempfilename = filename;
    int err = false;

    if ( todir ) {
	SFDirClean(filename);
	GFileMkDir(filename);		/* this will fail if directory already exists. That's ok */
	tempfilename = malloc(strlen(filename)+strlen("/" FONT_PROPS)+1);
	strcpy(tempfilename,filename); strcat(tempfilename,"/" FONT_PROPS);
    }

    if ( !todir && strstr(filename,"://")!=NULL )
	sfd = tmpfile();
    else
	sfd = fopen(tempfilename,"w");
    if ( tempfilename!=filename ) free(tempfilename);
    if ( sfd==NULL )
return( 0 );

    locale_t tmplocale; locale_t oldlocale; // Declare temporary locale storage.
    switch_to_c_locale(&tmplocale, &oldlocale); // Switch to the C locale temporarily and cache the old locale.
    if ( sf->cidmaster!=NULL ) {
	sf=sf->cidmaster;
	gc = 1;
	for ( i=0; i<sf->subfontcnt; ++i )
	    if ( sf->subfonts[i]->glyphcnt > gc )
		gc = sf->subfonts[i]->glyphcnt;
	map = EncMap1to1(gc);
	err = SFDDump(sfd,sf,map,NULL,todir,filename);
	EncMapFree(map);
    } else
	err = SFDDump(sfd,sf,map,normal,todir,filename);
    switch_to_old_locale(&tmplocale, &oldlocale); // Switch to the cached locale.
    if ( ferror(sfd) ) err = true;
    if ( !err && !todir && strstr(filename,"://")!=NULL )
	err = !URLFromFile(filename,sfd);
    if ( fclose(sfd) ) err = true;
    if ( todir )
	SFFinalDirClean(filename);
return( !err );
}

int SFDDoesAnyBackupExist(char* filename)
{
    char path[PATH_MAX];
    int idx = 1;

    snprintf( path, PATH_MAX, "%s-%02d", filename, idx );
    return GFileExists(path);
}

/**
 * Handle creation of potential implicit revisions when saving.
 *
 * If s2d is set then we are saving to an sfdir and no revisions are
 * created.
 *
 * If localRevisionsToRetain == 0 then no revisions are made.
 *
 * If localRevisionsToRetain > 0 then it is taken as an explict number
 * of revisions to make, and revisions are made
 *
 * If localRevisionsToRetain == -1 then it is "not set".
 * In that case, revisions are only made if there are already revisions
 * for the locfilename.
 *
 */
int SFDWriteBakExtended(char* locfilename,
			SplineFont *sf,EncMap *map,EncMap *normal,
			int s2d,
			int localRevisionsToRetain )
{
    int rc = 0;

    if( s2d )
    {
	rc = SFDWrite(locfilename,sf,map,normal,s2d);
	return rc;
    }


    int cacheRevisionsToRetain = prefRevisionsToRetain;

    sf->save_to_dir = s2d;

    if( localRevisionsToRetain < 0 )
    {
	// If there are no backups, then don't start creating any
	if( !SFDDoesAnyBackupExist(sf->filename))
	    prefRevisionsToRetain = 0;
    }
    else
    {
	prefRevisionsToRetain = localRevisionsToRetain;
    }

    rc = SFDWriteBak( locfilename, sf, map, normal );

    prefRevisionsToRetain = cacheRevisionsToRetain;

    return rc;
}


int SFDWriteBak(char *filename,SplineFont *sf,EncMap *map,EncMap *normal) {
    char *buf=0, *buf2=NULL;
    int ret;

    if ( sf->save_to_dir )
    {
	ret = SFDWrite(filename,sf,map,normal,true);
	return(ret);
    }

    if ( sf->cidmaster!=NULL )
	sf=sf->cidmaster;
    buf = malloc(strlen(filename)+10);
    if ( sf->compression!=0 )
    {
	buf2 = malloc(strlen(filename)+10);
	strcpy(buf2,filename);
	strcat(buf2,compressors[sf->compression-1].ext);
	strcpy(buf,buf2);
	strcat(buf,"~");
	if ( rename(buf2,buf)==0 )
	    sf->backedup = bs_backedup;
    }
    else
    {
	sf->backedup = bs_dontknow;

	if( prefRevisionsToRetain )
	{
	    char path[PATH_MAX];
	    char pathnew[PATH_MAX];
	    int idx = 0;

	    snprintf( path,    PATH_MAX, "%s", filename );
	    snprintf( pathnew, PATH_MAX, "%s-%02d", filename, idx );
	    (void)rename( path, pathnew );

	    for( idx=prefRevisionsToRetain; idx > 0; idx-- )
	    {
		snprintf( path, PATH_MAX, "%s-%02d", filename, idx-1 );
		snprintf( pathnew, PATH_MAX, "%s-%02d", filename, idx );

		int rc = rename( path, pathnew );
		if( !idx && !rc )
		    sf->backedup = bs_backedup;
	    }
	    idx = prefRevisionsToRetain+1;
	    snprintf( path, PATH_MAX, "%s-%02d", filename, idx );
	    unlink(path);
	}

    }
    free(buf);

    ret = SFDWrite(filename,sf,map,normal,false);
    if ( ret && sf->compression!=0 ) {
	unlink(buf2);
	buf = malloc(strlen(filename)+40);
	sprintf( buf, "%s %s", compressors[sf->compression-1].recomp, filename );
	if ( system( buf )!=0 )
	    sf->compression = 0;
	free(buf);
    }
    free(buf2);
    return( ret );
}

/* ********************************* INPUT ********************************** */
#include "sfd1.h"

char *getquotedeol(FILE *sfd) {
    char *pt, *str, *end;
    int ch;

    pt = str = malloc(101); end = str+100;
    while ( isspace(ch = nlgetc(sfd)) && ch!='\r' && ch!='\n' );
    while ( ch!='\n' && ch!='\r' && ch!=EOF ) {
	if ( ch=='\\' ) {
	    /* We can't use nlgetc() here, because it would misinterpret */
	    /* double backslash at the end of line. Multiline strings,   */
	    /* broken with backslash + newline, are just handled above.  */
	    ch = getc(sfd);
	    if ( ch=='n' ) ch='\n';
	    /* else if ( ch=='\\' ) ch=='\\'; */ /* second backslash of '\\' */

	    /* FontForge doesn't write other escape sequences in this context. */
	    /* So any other value of ch is assumed impossible. */
	}
	if ( pt>=end ) {
	    pt = realloc(str,end-str+101);
	    end = pt+(end-str)+100;
	    str = pt;
	    pt = end-100;
	}
	*pt++ = ch;
	ch = nlgetc(sfd);
    }
    *pt='\0';
    /* these strings should be in utf8 now, but some old sfd files might have */
    /* latin1. Not a severe problems because they SHOULD be in ASCII. So any */
    /* non-ascii strings are erroneous anyway */
    if ( !utf8_valid(str) ) {
	pt = latin1_2_utf8_copy(str);
	free(str);
	str = pt;
    }
return( str );
}

static int geteol(FILE *sfd, char *tokbuf) {
    char *pt=tokbuf, *end = tokbuf+2000-2; int ch;

    while ( isspace(ch = nlgetc(sfd)) && ch!='\r' && ch!='\n' );
    while ( ch!='\n' && ch!='\r' && ch!=EOF ) {
	if ( pt<end ) *pt++ = ch;
	ch = nlgetc(sfd);
    }
    *pt='\0';
return( pt!=tokbuf?1:ch==EOF?-1: 0 );
}
void visitSFDFragment( FILE *sfd, SplineFont *sf,
		       visitSFDFragmentFunc ufunc, void* udata )
{
    int eof;
    char tok[2000];
    while ( 1 ) {
	if ( (eof = getname(sfd,tok))!=1 ) {
	    if ( eof==-1 )
		break;
	    geteol(sfd,tok);
	    continue;
	}

	ufunc( sfd, tok, sf, udata );
    }
}


static int getprotectedname(FILE *sfd, char *tokbuf) {
    char *pt=tokbuf, *end = tokbuf+100-2; int ch;

    while ( (ch = nlgetc(sfd))==' ' || ch=='\t' );
    while ( ch!=EOF && !isspace(ch) && ch!='[' && ch!=']' && ch!='{' && ch!='}' && ch!='<' && ch!='%' ) {
	if ( pt<end ) *pt++ = ch;
	ch = nlgetc(sfd);
    }
    if ( pt==tokbuf && ch!=EOF )
	*pt++ = ch;
    else
	ungetc(ch,sfd);
    *pt='\0';
return( pt!=tokbuf?1:ch==EOF?-1: 0 );
}

int getname(FILE *sfd, char *tokbuf) {
    int ch;

    while ( isspace(ch = nlgetc(sfd)));
    ungetc(ch,sfd);
return( getprotectedname(sfd,tokbuf));
}

static uint32 gettag(FILE *sfd) {
    int ch, quoted;
    uint32 tag;

    while ( (ch=nlgetc(sfd))==' ' );
    if ( (quoted = (ch=='\'')) ) ch = nlgetc(sfd);
    tag = (ch<<24)|(nlgetc(sfd)<<16);
    tag |= nlgetc(sfd)<<8;
    tag |= nlgetc(sfd);
    if ( quoted ) (void) nlgetc(sfd);
return( tag );
}

static int getint(FILE *sfd, int *val) {
    char tokbuf[100]; int ch;
    char *pt=tokbuf, *end = tokbuf+100-2;

    while ( isspace(ch = nlgetc(sfd)));
    if ( ch=='-' || ch=='+' ) {
	*pt++ = ch;
	ch = nlgetc(sfd);
    }
    while ( isdigit(ch)) {
	if ( pt<end ) *pt++ = ch;
	ch = nlgetc(sfd);
    }
    *pt='\0';
    ungetc(ch,sfd);
    *val = strtol(tokbuf,NULL,10);
return( pt!=tokbuf?1:ch==EOF?-1: 0 );
}

static int getlonglong(FILE *sfd, long long *val) {
    char tokbuf[100]; int ch;
    char *pt=tokbuf, *end = tokbuf+100-2;

    while ( isspace(ch = nlgetc(sfd)));
    if ( ch=='-' || ch=='+' ) {
	*pt++ = ch;
	ch = nlgetc(sfd);
    }
    while ( isdigit(ch)) {
	if ( pt<end ) *pt++ = ch;
	ch = nlgetc(sfd);
    }
    *pt='\0';
    ungetc(ch,sfd);
    *val = strtoll(tokbuf,NULL,10);
return( pt!=tokbuf?1:ch==EOF?-1: 0 );
}

static int gethex(FILE *sfd, uint32 *val) {
    char tokbuf[100]; int ch;
    char *pt=tokbuf, *end = tokbuf+100-2;

    while ( isspace(ch = nlgetc(sfd)));
    if ( ch=='#' )
	ch = nlgetc(sfd);
    if ( ch=='-' || ch=='+' ) {
	*pt++ = ch;
	ch = nlgetc(sfd);
    }
    if ( ch=='0' ) {
	ch = nlgetc(sfd);
	if ( ch=='x' || ch=='X' )
	    ch = nlgetc(sfd);
	else {
	    ungetc(ch,sfd);
	    ch = '0';
	}
    }
    while ( isdigit(ch) || (ch>='a' && ch<='f') || (ch>='A' && ch<='F')) {
	if ( pt<end ) *pt++ = ch;
	ch = nlgetc(sfd);
    }
    *pt='\0';
    ungetc(ch,sfd);
    *val = strtoul(tokbuf,NULL,16);
return( pt!=tokbuf?1:ch==EOF?-1: 0 );
}

static int gethexints(FILE *sfd, uint32 *val, int cnt) {
    int i, ch;

    for ( i=0; i<cnt; ++i ) {
	if ( i!=0 ) {
	    ch = nlgetc(sfd);
	    if ( ch!='.' ) ungetc(ch,sfd);
	}
	if ( !gethex(sfd,&val[i]))
return( false );
    }
return( true );
}

static int getsint(FILE *sfd, int16 *val) {
    int val2;
    int ret = getint(sfd,&val2);
    *val = val2;
return( ret );
}

static int getusint(FILE *sfd, uint16 *val) {
    int val2;
    int ret = getint(sfd,&val2);
    *val = val2;
return( ret );
}

static int getreal(FILE *sfd, real *val) {
    char tokbuf[100];
    int ch;
    char *pt=tokbuf, *end = tokbuf+100-2, *nend;

    while ( isspace(ch = nlgetc(sfd)));
    if ( ch!='e' && ch!='E' )		/* real's can't begin with exponants */
	while ( isdigit(ch) || ch=='-' || ch=='+' || ch=='e' || ch=='E' || ch=='.' || ch==',' ) {
	    if ( pt<end ) *pt++ = ch;
	    ch = nlgetc(sfd);
	}
    *pt='\0';
    ungetc(ch,sfd);
    *val = strtod(tokbuf,&nend);
    /* Beware of different locals! */
    if ( *nend!='\0' ) {
	if ( *nend=='.' )
	    *nend = ',';
	else if ( *nend==',' )
	    *nend = '.';
	*val = strtod(tokbuf,&nend);
    }
return( pt!=tokbuf && *nend=='\0'?1:ch==EOF?-1: 0 );
}

/* Don't use nlgetc here. We carefully control newlines when dumping in 85 */
/*  but backslashes can occur at end of line. */
static int Dec85(struct enc85 *dec) {
    int ch1, ch2, ch3, ch4, ch5;
    unsigned int val;

    if ( dec->pos<0 ) {
	while ( isspace(ch1=getc(dec->sfd)));
	if ( ch1=='z' ) {
	    dec->sofar[0] = dec->sofar[1] = dec->sofar[2] = dec->sofar[3] = 0;
	    dec->pos = 3;
	} else {
	    while ( isspace(ch2=getc(dec->sfd)));
	    while ( isspace(ch3=getc(dec->sfd)));
	    while ( isspace(ch4=getc(dec->sfd)));
	    while ( isspace(ch5=getc(dec->sfd)));
	    val = ((((ch1-'!')*85+ ch2-'!')*85 + ch3-'!')*85 + ch4-'!')*85 + ch5-'!';
	    dec->sofar[3] = val>>24;
	    dec->sofar[2] = val>>16;
	    dec->sofar[1] = val>>8;
	    dec->sofar[0] = val;
	    dec->pos = 3;
	}
    }
return( dec->sofar[dec->pos--] );
}

static void rle2image(struct enc85 *dec,int rlelen,struct _GImage *base) {
    uint8 *pt, *end;
    int r,c,set, cnt, ch, ch2;
    int i;

    r = c = 0; set = 1; pt = base->data; end = pt + base->bytes_per_line*base->height;
    memset(base->data,0xff,end-pt);
    while ( rlelen>0 ) {
	if ( pt>=end ) {
	    IError( "RLE failure\n" );
	    while ( rlelen>0 ) { Dec85(dec); --rlelen; }
    break;
	}
	ch = Dec85(dec);
	--rlelen;
	if ( ch==255 ) {
	    ch2 = Dec85(dec);
	    cnt = (ch2<<8) + Dec85(dec);
	    rlelen -= 2;
	} else
	    cnt = ch;
	if ( ch==255 && ch2==0 && cnt<255 ) {
	    /* Line duplication */
	    for ( i=0; i<cnt && pt<end; ++i ) {
		memcpy(pt,base->data+(r-1)*base->bytes_per_line,base->bytes_per_line);
		++r;
		pt += base->bytes_per_line;
	    }
	    set = 1;
	} else {
	    if ( pt + ((c+cnt)>>3) > end ) {
		IError( "Run length encoded image has been corrupted.\n" );
    break;
	    }
	    if ( !set ) {
		for ( i=0; i<cnt; ++i )
		    pt[(c+i)>>3] &= ((~0x80)>>((c+i)&7));
	    }
	    c += cnt;
	    set = 1-set;
	    if ( c>=base->width ) {
		++r;
		pt += base->bytes_per_line;
		c = 0; set = 1;
	    }
	}
    }
}

static ImageList *SFDGetImage(FILE *sfd) {
    /* We've read the image token */
    int width, height, image_type, bpl, clutlen, rlelen;
    uint32 trans;
    struct _GImage *base;
    GImage *image;
    ImageList *img;
    struct enc85 dec;
    int i, ch;

    memset(&dec,'\0', sizeof(dec)); dec.pos = -1;
    dec.sfd = sfd;

    getint(sfd,&width);
    getint(sfd,&height);
    getint(sfd,&image_type);
    getint(sfd,&bpl);
    getint(sfd,&clutlen);
    gethex(sfd,&trans);
    image = GImageCreate(image_type,width,height);
    base = image->list_len==0?image->u.image:image->u.images[0];
    img = calloc(1,sizeof(ImageList));
    img->image = image;
    getreal(sfd,&img->xoff);
    getreal(sfd,&img->yoff);
    getreal(sfd,&img->xscale);
    getreal(sfd,&img->yscale);
    while ( (ch=nlgetc(sfd))==' ' || ch=='\t' );
    ungetc(ch,sfd);
    rlelen = 0;
    if ( isdigit(ch))
	getint(sfd,&rlelen);
    base->trans = trans;
    if ( clutlen!=0 ) {
	if ( base->clut==NULL )
	    base->clut = calloc(1,sizeof(GClut));
	base->clut->clut_len = clutlen;
	base->clut->trans_index = trans;
	for ( i=0;i<clutlen; ++i ) {
	    int r,g,b;
	    r = Dec85(&dec);
	    g = Dec85(&dec);
	    b = Dec85(&dec);
	    base->clut->clut[i] = (r<<16)|(g<<8)|b;
	}
    }
    if ( rlelen!=0 ) {
	rle2image(&dec,rlelen,base);
    } else {
	for ( i=0; i<height; ++i ) {
	    if ( image_type==it_rgba ) {
		uint32 *ipt = (uint32 *) (base->data + i*base->bytes_per_line);
		uint32 *iend = (uint32 *) (base->data + (i+1)*base->bytes_per_line);
		int r,g,b, a;
		while ( ipt<iend ) {
		    a = Dec85(&dec);
		    r = Dec85(&dec);
		    g = Dec85(&dec);
		    b = Dec85(&dec);
		    *ipt++ = (a<<24)|(r<<16)|(g<<8)|b;
		}
	    } else if ( image_type==it_true ) {
		int *ipt = (int *) (base->data + i*base->bytes_per_line);
		int *iend = (int *) (base->data + (i+1)*base->bytes_per_line);
		int r,g,b;
		while ( ipt<iend ) {
		    r = Dec85(&dec);
		    g = Dec85(&dec);
		    b = Dec85(&dec);
		    *ipt++ = (r<<16)|(g<<8)|b;
		}
	    } else {
		uint8 *pt = (uint8 *) (base->data + i*base->bytes_per_line);
		uint8 *end = (uint8 *) (base->data + (i+1)*base->bytes_per_line);
		while ( pt<end ) {
		    *pt++ = Dec85(&dec);
		}
	    }
	}
    }
    img->bb.minx = img->xoff; img->bb.maxy = img->yoff;
    img->bb.maxx = img->xoff + GImageGetWidth(img->image)*img->xscale;
    img->bb.miny = img->yoff - GImageGetHeight(img->image)*img->yscale;
    /* In old sfd files I failed to recognize bitmap pngs as bitmap, so put */
    /*  in a little check here that converts things which should be bitmap to */
    /*  bitmap */ /* Eventually it can be removed as all old sfd files get */
    /*  converted. 22/10/2002 */
    if ( base->image_type==it_index && base->clut!=NULL && base->clut->clut_len==2 )
	img->image = ImageAlterClut(img->image);
return( img );
}

static void SFDGetType1(FILE *sfd) {
    /* We've read the OrigType1 token (this is now obselete, but parse it in case there are any old sfds) */
    int len;
    struct enc85 dec;

    memset(&dec,'\0', sizeof(dec)); dec.pos = -1;
    dec.sfd = sfd;

    getint(sfd,&len);
    while ( --len >= 0 )
	Dec85(&dec);
}

static void SFDGetTtfInstrs(FILE *sfd, SplineChar *sc) {
    /* We've read the TtfInstr token, it is followed by a byte count */
    /* and then the instructions in enc85 format */
    int i,len;
    struct enc85 dec;

    memset(&dec,'\0', sizeof(dec)); dec.pos = -1;
    dec.sfd = sfd;

    getint(sfd,&len);
    sc->ttf_instrs = malloc(len);
    sc->ttf_instrs_len = len;
    for ( i=0; i<len; ++i )
	sc->ttf_instrs[i] = Dec85(&dec);
}

static void tterr(void *UNUSED(rubbish), char *message, int UNUSED(pos)) {
    LogError(_("When loading tt instrs from sfd: %s\n"), message );
}

static void SFDGetTtInstrs(FILE *sfd, SplineChar *sc) {
    /* We've read the TtInstr token, it is followed by text versions of */
    /*  the instructions, slurp it all into a big buffer, and then parse that */
    char *buf=NULL, *pt=buf, *end=buf;
    int ch;
    int backlen = strlen(end_tt_instrs);
    int instr_len;

    while ( (ch=nlgetc(sfd))!=EOF ) {
	if ( pt>=end ) {
	    char *newbuf = realloc(buf,(end-buf+200));
	    pt = newbuf+(pt-buf);
	    end = newbuf+(end+200-buf);
	    buf = newbuf;
	}
	*pt++ = ch;
	if ( pt-buf>backlen && strncmp(pt-backlen,end_tt_instrs,backlen)==0 ) {
	    pt -= backlen;
    break;
	}
    }
    *pt = '\0';

    sc->ttf_instrs = _IVParse(sc->parent,buf,&instr_len,tterr,NULL);
    sc->ttf_instrs_len = instr_len;

    free(buf);
}

static struct ttf_table *SFDGetTtfTable(FILE *sfd, SplineFont *sf,struct ttf_table *lasttab[2]) {
    /* We've read the TtfTable token, it is followed by a tag and a byte count */
    /* and then the instructions in enc85 format */
    int i,len;
    int which;
    struct enc85 dec;
    struct ttf_table *tab = chunkalloc(sizeof(struct ttf_table));

    memset(&dec,'\0', sizeof(dec)); dec.pos = -1;
    dec.sfd = sfd;

    tab->tag = gettag(sfd);

    if ( tab->tag==CHR('f','p','g','m') || tab->tag==CHR('p','r','e','p') ||
	    tab->tag==CHR('c','v','t',' ') || tab->tag==CHR('m','a','x','p'))
	which = 0;
    else
	which = 1;

    getint(sfd,&len);
    tab->data = malloc(len);
    tab->len = len;
    for ( i=0; i<len; ++i )
	tab->data[i] = Dec85(&dec);

    if ( lasttab[which]!=NULL )
	lasttab[which]->next = tab;
    else if ( which==0 )
	sf->ttf_tables = tab;
    else
	sf->ttf_tab_saved = tab;
    lasttab[which] = tab;
return( tab );
}

static struct ttf_table *SFDGetShortTable(FILE *sfd, SplineFont *sf,struct ttf_table *lasttab[2]) {
    /* We've read the ShortTable token, it is followed by a tag and a word count */
    /* and then the (text) values of the words that make up the cvt table */
    int i,len, ch;
    uint8 *pt;
    int which, iscvt, started;
    struct ttf_table *tab = chunkalloc(sizeof(struct ttf_table));

    tab->tag = gettag(sfd);

    if ( tab->tag==CHR('f','p','g','m') || tab->tag==CHR('p','r','e','p') ||
	    tab->tag==CHR('c','v','t',' ') || tab->tag==CHR('m','a','x','p'))
	which = 0;
    else
	which = 1;
    iscvt = tab->tag==CHR('c','v','t',' ');

    getint(sfd,&len);
    pt = tab->data = malloc(2*len);
    tab->len = 2*len;
    started = false;
    for ( i=0; i<len; ++i ) {
	int num;
	getint(sfd,&num);
	*pt++ = num>>8;
	*pt++ = num&0xff;
	if ( iscvt ) {
	    ch = nlgetc(sfd);
	    if ( ch==' ' ) {
		if ( !started ) {
		    sf->cvt_names = calloc(len+1,sizeof(char *));
		    sf->cvt_names[len] = END_CVT_NAMES;
		    started = true;
		}
		sf->cvt_names[i] = SFDReadUTF7Str(sfd);
	    } else
		ungetc(ch,sfd);
	}
    }

    if ( lasttab[which]!=NULL )
	lasttab[which]->next = tab;
    else if ( which==0 )
	sf->ttf_tables = tab;
    else
	sf->ttf_tab_saved = tab;
    lasttab[which] = tab;
return( tab );
}

static struct ttf_table *SFDGetTtTable(FILE *sfd, SplineFont *sf,struct ttf_table *lasttab[2]) {
    /* We've read the TtTable token, it is followed by a tag */
    /* and then the instructions in text format */
    int ch;
    int which;
    struct ttf_table *tab = chunkalloc(sizeof(struct ttf_table));
    char *buf=NULL, *pt=buf, *end=buf;
    int backlen = strlen(end_tt_instrs);

    tab->tag = gettag(sfd);

    if ( tab->tag==CHR('f','p','g','m') || tab->tag==CHR('p','r','e','p') ||
	    tab->tag==CHR('c','v','t',' ') || tab->tag==CHR('m','a','x','p'))
	which = 0;
    else
	which = 1;

    while ( (ch=nlgetc(sfd))!=EOF ) {
	if ( pt>=end ) {
	    char *newbuf = realloc(buf,(end-buf+200));
	    pt = newbuf+(pt-buf);
	    end = newbuf+(end+200-buf);
	    buf = newbuf;
	}
	*pt++ = ch;
	if ( pt-buf>backlen && strncmp(pt-backlen,end_tt_instrs,backlen)==0 ) {
	    pt -= backlen;
    break;
	}
    }
    *pt = '\0';
    tab->data = _IVParse(sf,buf,&tab->len,tterr,NULL);
    free(buf);

    if ( lasttab[which]!=NULL )
	lasttab[which]->next = tab;
    else if ( which==0 )
	sf->ttf_tables = tab;
    else
	sf->ttf_tab_saved = tab;
    lasttab[which] = tab;
return( tab );
}

static int SFDCloseCheck(SplinePointList *spl,int order2) {
    if ( spl->first!=spl->last &&
	    RealNear(spl->first->me.x,spl->last->me.x) &&
	    RealNear(spl->first->me.y,spl->last->me.y)) {
	SplinePoint *oldlast = spl->last;
	spl->first->prevcp = oldlast->prevcp;
	spl->first->noprevcp = oldlast->noprevcp;
	oldlast->prev->from->next = NULL;
	spl->last = oldlast->prev->from;
	chunkfree(oldlast->prev,sizeof(*oldlast));
	chunkfree(oldlast->hintmask,sizeof(HintMask));
	chunkfree(oldlast,sizeof(*oldlast));
	SplineMake(spl->last,spl->first,order2);
	spl->last = spl->first;
return( true );
    }
return( false );
}

static void SFDGetHintMask(FILE *sfd,HintMask *hintmask) {
    int nibble = 0, ch;

    memset(hintmask,0,sizeof(HintMask));
    for (;;) {
	ch = nlgetc(sfd);
	if ( isdigit(ch))
	    ch -= '0';
	else if ( ch>='a' && ch<='f' )
	    ch -= 'a'-10;
	else if ( ch>='A' && ch<='F' )
	    ch -= 'A'-10;
	else {
	    ungetc(ch,sfd);
    break;
	}
	if ( nibble<2*HntMax/8 )
	    (*hintmask)[nibble>>1] |= ch<<(4*(1-(nibble&1)));
	++nibble;
    }
}

static void SFDGetSpiros(FILE *sfd,SplineSet *cur) {
    int ch;
    spiro_cp cp;

    ch = nlgetc(sfd);		/* S */
    ch = nlgetc(sfd);		/* p */
    ch = nlgetc(sfd);		/* i */
    ch = nlgetc(sfd);		/* r */
    ch = nlgetc(sfd);		/* o */
    while ( fscanf(sfd,"%lg %lg %c", &cp.x, &cp.y, &cp.ty )==3 ) {
	if ( cur!=NULL ) {
	    if ( cur->spiro_cnt>=cur->spiro_max )
		cur->spiros = realloc(cur->spiros,(cur->spiro_max+=10)*sizeof(spiro_cp));
	    cur->spiros[cur->spiro_cnt++] = cp;
	}
    }
    if ( cur!=NULL && (cur->spiros[cur->spiro_cnt-1].ty&0x7f)!=SPIRO_END ) {
	if ( cur->spiro_cnt>=cur->spiro_max )
	    cur->spiros = realloc(cur->spiros,(cur->spiro_max+=1)*sizeof(spiro_cp));
	memset(&cur->spiros[cur->spiro_cnt],0,sizeof(spiro_cp));
	cur->spiros[cur->spiro_cnt++].ty = SPIRO_END;
    }
    ch = nlgetc(sfd);
    if ( ch=='E' ) {
	ch = nlgetc(sfd);		/* n */
	ch = nlgetc(sfd);		/* d */
	ch = nlgetc(sfd);		/* S */
	ch = nlgetc(sfd);		/* p */
	ch = nlgetc(sfd);		/* i */
	ch = nlgetc(sfd);		/* r */
	ch = nlgetc(sfd);		/* o */
    } else
	ungetc(ch,sfd);
}

static SplineSet *SFDGetSplineSet(FILE *sfd,int order2) {
    SplinePointList *cur=NULL, *head=NULL;
    BasePoint current;
    real stack[100];
    int sp=0;
    SplinePoint *pt = NULL;
    int ch;
	int ch2;
    char tok[100];
    int ttfindex = 0;
    int lastacceptable;

    current.x = current.y = 0;
    lastacceptable = 0;
    while ( 1 ) {
	int have_read_val = 0;
	int val = 0;

	while ( getreal(sfd,&stack[sp])==1 )
	    if ( sp<99 )
		++sp;
	while ( isspace(ch=nlgetc(sfd)));
	if ( ch=='E' || ch=='e' || ch==EOF )
    break;
	if ( ch=='S' ) {
	    ungetc(ch,sfd);
	    SFDGetSpiros(sfd,cur);
    continue;
	} else if (( ch=='N' ) &&
	    nlgetc(sfd)=='a' &&	/* a */
	    nlgetc(sfd)=='m' &&	/* m */
	    nlgetc(sfd)=='e' &&	/* e */
	    nlgetc(sfd)=='d' ) /* d */ {
	    ch2 = nlgetc(sfd);		/* : */
		// We are either fetching a splineset name (Named:) or a point name (NamedP:).
		if (ch2=='P') { if ((nlgetc(sfd)==':') && (pt!=NULL)) { if (pt->name!=NULL) {free(pt->name);} pt->name = SFDReadUTF7Str(sfd); } }
		else if (ch2==':') { if (cur != NULL) cur->contour_name = SFDReadUTF7Str(sfd); else { char * freetmp = SFDReadUTF7Str(sfd); free(freetmp); freetmp = NULL; } }
        continue;
	} else if ( ch=='P' && PeekMatch(sfd,"ath") ) {
	    int flags;
	    nlgetc(sfd);		/* a */
	    nlgetc(sfd);		/* t */
	    nlgetc(sfd);		/* h */
	    if (PeekMatch(sfd,"Flags:")) {
	      nlgetc(sfd);		/* F */
	      nlgetc(sfd);		/* l */
	      nlgetc(sfd);		/* a */
	      nlgetc(sfd);		/* g */
	      nlgetc(sfd);		/* s */
	      nlgetc(sfd);		/* : */
	      getint(sfd,&flags);
	      if (cur != NULL) cur->is_clip_path = flags&1;
	    } else if (PeekMatch(sfd,"Start:")) {
	      nlgetc(sfd);		/* S */
	      nlgetc(sfd);		/* t */
	      nlgetc(sfd);		/* a */
	      nlgetc(sfd);		/* r */
	      nlgetc(sfd);		/* t */
	      nlgetc(sfd);		/* : */
	      getint(sfd,&flags);
	      if (cur != NULL) cur->start_offset = flags;
	    }
	}
	pt = NULL;
	if ( ch=='l' || ch=='m' ) {
	    if ( sp>=2 ) {
		current.x = stack[sp-2];
		current.y = stack[sp-1];
		sp -= 2;
		pt = chunkalloc(sizeof(SplinePoint));
		pt->me = current;
		pt->noprevcp = true; pt->nonextcp = true;
		if ( ch=='m' ) {
		    SplinePointList *spl = chunkalloc(sizeof(SplinePointList));
		    spl->first = spl->last = pt;
		    spl->start_offset = 0;
		    if ( cur!=NULL ) {
			if ( SFDCloseCheck(cur,order2))
			    --ttfindex;
			cur->next = spl;
		    } else
			head = spl;
		    cur = spl;
		} else {
		    if ( cur!=NULL && cur->first!=NULL && (cur->first!=cur->last || cur->first->next==NULL) ) {
			if ( cur->last->nextcpindex==0xfffe )
			    cur->last->nextcpindex = 0xffff;
			SplineMake(cur->last,pt,order2);
			cur->last->nonextcp = 1;
			pt->noprevcp = 1;
			cur->last = pt;
		    }
		}
	    } else
		sp = 0;
	} else if ( ch=='c' ) {
	    if ( sp>=6 ) {
		getint(sfd,&val);
		have_read_val = 1;


		current.x = stack[sp-2];
		current.y = stack[sp-1];
		real original_current_x = current.x;
		if( val & SFD_PTFLAG_FORCE_OPEN_PATH )
		{
		    // Find somewhere vacant to put the point.x for now
		    // we need to do this check in case we choose a point that is already
		    // on the spline and this connect back to that point instead of creating
		    // an open path
		    while( 1 )
		    {
			real offset = 0.1;
			current.x += offset;
			if( !cur || !SplinePointListContainsPointAtX( cur, current.x ))
			{
			    break;
			}
		    }
		}

		if ( cur!=NULL && cur->first!=NULL && (cur->first!=cur->last || cur->first->next==NULL) ) {
		    cur->last->nextcp.x = stack[sp-6];
		    cur->last->nextcp.y = stack[sp-5];
		    cur->last->nonextcp = false;
		    pt = chunkalloc(sizeof(SplinePoint));
		    pt->prevcp.x = stack[sp-4];
		    pt->prevcp.y = stack[sp-3];
		    pt->me = current;
		    pt->nonextcp = true;
		    if ( cur->last->nextcpindex==0xfffe )
			cur->last->nextcpindex = ttfindex++;
		    else if ( cur->last->nextcpindex!=0xffff )
			ttfindex = cur->last->nextcpindex+1;
		    SplineMake(cur->last,pt,order2);
		    cur->last = pt;
		    // pt->me is a copy of 'current' so we should now move
		    // the x coord of pt->me back to where it should be.
		    // The whole aim here is that this spline remains an open path
		    // when PTFLAG_FORCE_OPEN_PATH is set.
		    pt->me.x = original_current_x;
		}

		// Move the point back to the same location it was
		// but do not connect it back to the point that is
		// already there.
		if( val & SFD_PTFLAG_FORCE_OPEN_PATH )
		{
		    current.x = original_current_x;
		}
		
		sp -= 6;
	    } else
		sp = 0;
	}
	if ( pt!=NULL ) {
	    if( !have_read_val )
		getint(sfd,&val);

	    pt->pointtype = (val & SFD_PTFLAG_TYPE_MASK);
	    pt->selected  = (val & SFD_PTFLAG_IS_SELECTED) > 0;
	    pt->nextcpdef = (val & SFD_PTFLAG_NEXTCP_IS_DEFAULT) > 0;
	    pt->prevcpdef = (val & SFD_PTFLAG_PREVCP_IS_DEFAULT) > 0;
	    pt->roundx    = (val & SFD_PTFLAG_ROUND_IN_X) > 0;
	    pt->roundy    = (val & SFD_PTFLAG_ROUND_IN_Y) > 0;
	    pt->dontinterpolate = (val & SFD_PTFLAG_INTERPOLATE_NEVER) > 0;
	    if ( pt->prev!=NULL )
		pt->prev->acceptableextrema = (val & SFD_PTFLAG_PREV_EXTREMA_MARKED_ACCEPTABLE) > 0;
	    else
		lastacceptable = (val & SFD_PTFLAG_PREV_EXTREMA_MARKED_ACCEPTABLE) > 0;
	    if ( val&0x80 )
		pt->ttfindex = 0xffff;
	    else
		pt->ttfindex = ttfindex++;
	    pt->nextcpindex = 0xfffe;
	    ch = nlgetc(sfd);
	    if ( ch=='x' ) {
		pt->hintmask = chunkalloc(sizeof(HintMask));
		SFDGetHintMask(sfd,pt->hintmask);
	    } else if ( ch!=',' )
		ungetc(ch,sfd);
	    else {
		ch = nlgetc(sfd);
		if ( ch==',' )
		    pt->ttfindex = 0xfffe;
		else {
		    ungetc(ch,sfd);
		    getint(sfd,&val);
		    pt->ttfindex = val;
		    nlgetc(sfd);	/* skip comma */
		    if ( val!=-1 )
			ttfindex = val+1;
		}
		ch = nlgetc(sfd);
		if ( ch=='\r' || ch=='\n' )
		    ungetc(ch,sfd);
		else {
		    ungetc(ch,sfd);
		    getint(sfd,&val);
		    pt->nextcpindex = val;
		    if ( val!=-1 )
			ttfindex = val+1;
		}
	    }
	}
    }
    if ( cur!=NULL )
	SFDCloseCheck(cur,order2);
    if ( lastacceptable && cur->last->prev!=NULL )
	cur->last->prev->acceptableextrema = true;
    getname(sfd,tok);
return( head );
}

Undoes *SFDGetUndo( FILE *sfd, SplineChar *sc,
		    const char* startTag,
		    int current_layer )
{
    Undoes *u = 0;
    char tok[2000];
    int i;
    RefChar *lastr=NULL;
    ImageList *lasti=NULL;
    AnchorPoint *lastap = NULL;
    SplineChar* tsc = 0;

    if ( getname(sfd,tok)!=1 )
        return( NULL );
    if ( strcmp(tok, startTag) )
        return( NULL );

    u = chunkalloc(sizeof(Undoes));
    u->undotype = ut_state;
    u->layer = UNDO_LAYER_UNKNOWN;

    while ( 1 )
    {
        if ( getname(sfd,tok)!=1 ) {
            chunkfree(u,sizeof(Undoes));
            return( NULL );
        }

        if ( !strmatch(tok,"EndUndoOperation")
            || !strmatch(tok,"EndRedoOperation"))
        {
            if( u->undotype == ut_hints ) {
                if( tsc ) {
                    u->u.state.hints = UHintCopy(tsc,1);
                    SplineCharFree( tsc );
                }
            }

            return u;
        }
	if ( !strmatch(tok,"Index:")) {
            getint(sfd,&i);
        }
	if ( !strmatch(tok,"Type:")) {
            getint(sfd,&i);
            u->undotype = i;
            if( u->undotype == ut_hints ) {
                tsc = SplineCharCopy( sc, 0, 0 );
                tsc->hstem = 0;
                tsc->vstem = 0;
                tsc->dstem = 0;
            }
        }
        if ( !strmatch(tok,"WasModified:")) {
            getint(sfd,&i);
            u->was_modified = i;
        }
        if ( !strmatch(tok,"WasOrder2:")) {
            getint(sfd,&i);
            u->was_order2 = i;
        }
        if ( !strmatch(tok,"Layer:")) {
            getint(sfd,&i);
            u->layer = i;
        }

        switch( u->undotype )
        {
	case ut_tstate:
	case ut_state:
	    if ( !strmatch(tok,"Width:"))          { getint(sfd,&i); u->u.state.width = i; }
	    if ( !strmatch(tok,"VWidth:"))         { getint(sfd,&i); u->u.state.vwidth = i; }
	    if ( !strmatch(tok,"LBearingChange:")) { getint(sfd,&i); u->u.state.lbearingchange = i; }
	    if ( !strmatch(tok,"UnicodeEnc:"))     { getint(sfd,&i); u->u.state.unicodeenc = i; }
	    if ( !strmatch(tok,"Charname:"))       { u->u.state.charname = getquotedeol(sfd); }
	    if ( !strmatch(tok,"Comment:"))        { u->u.state.comment  = getquotedeol(sfd); }

	    if( !strmatch(tok,"Refer:"))
	    {
		RefChar *ref = SFDGetRef(sfd,strmatch(tok,"Ref:")==0);
		int i=0;
		for( i=0; i< ref->layer_cnt; i++ ) {
		    ref->layers[i].splines = 0;
		}
		if ( !u->u.state.refs )
		    u->u.state.refs = ref;
		else
		    lastr->next = ref;
		lastr = ref;
	    }

	    if( !strmatch(tok,"Image:"))
	    {
		ImageList *img = SFDGetImage(sfd);
		if ( !u->u.state.images )
		    u->u.state.images = img;
		else
		    lasti->next = img;
		lasti = img;
	    }

	    if( !strmatch(tok,"Comment:")) {
		u->u.state.comment  = getquotedeol(sfd);
	    }
	    if( !strmatch(tok,"InstructionsLength:")) {
		getint(sfd,&i); u->u.state.instrs_len = i;
	    }
	    if( !strmatch(tok,"AnchorPoint:") ) {
		lastap = SFDReadAnchorPoints( sfd, sc, &(u->u.state.anchor), lastap );
	    }
	    if ( !strmatch(tok,"SplineSet")) {
		u->u.state.splines = SFDGetSplineSet(sfd,sc->layers[current_layer].order2);
	    }
	    break;
	case ut_hints:
	{
	    if ( !strmatch(tok,"HStem:") ) {
		tsc->hstem = SFDReadHints(sfd);
		tsc->hconflicts = StemListAnyConflicts(tsc->hstem);
	    }
	    else if ( !strmatch(tok,"VStem:") ) {
		tsc->vstem = SFDReadHints(sfd);
		tsc->vconflicts = StemListAnyConflicts(tsc->vstem);
	    }
	    else if( !strmatch(tok,"DStem2:"))
		tsc->dstem = SFDReadDHints( sc->parent,sfd,false );
	    else if( !strmatch(tok,"TtInstrs:")) {
		SFDGetTtInstrs(sfd,tsc);
		u->u.state.instrs = tsc->ttf_instrs;
		u->u.state.instrs_len = tsc->ttf_instrs_len;
		tsc->ttf_instrs = 0;
		tsc->ttf_instrs_len = 0;
	    }
	    break;
	}

	case ut_width:
	case ut_vwidth:
	    if( !strmatch(tok,"Width:")) {
		getint(sfd,&i); u->u.width = i;
	    }
	    break;
	default:
	break;
        }
    }

    return u;
}

static void SFDGetMinimumDistances(FILE *sfd, SplineChar *sc) {
    SplineSet *ss;
    SplinePoint *sp;
    int pt,i, val, err;
    int ch;
    SplinePoint **mapping=NULL;
    MinimumDistance *last, *md, *mdhead=NULL;

    for ( i=0; i<2; ++i ) {
	pt = 0;
	for ( ss = sc->layers[ly_fore].splines; ss!=NULL; ss=ss->next ) {
	    for ( sp=ss->first; ; ) {
		if ( mapping!=NULL ) mapping[pt] = sp;
		pt++;
		if ( sp->next == NULL )
	    break;
		sp = sp->next->to;
		if ( sp==ss->first )
	    break;
	    }
	}
	if ( mapping==NULL )
	    mapping = calloc(pt,sizeof(SplinePoint *));
    }

    last = NULL;
    for ( ch=nlgetc(sfd); ch!=EOF && ch!='\n'; ch=nlgetc(sfd)) {
	err = false;
	while ( isspace(ch) && ch!='\n' ) ch=nlgetc(sfd);
	if ( ch=='\n' )
    break;
	md = chunkalloc(sizeof(MinimumDistance));
	if ( ch=='x' ) md->x = true;
	getint(sfd,&val);
	if ( val<-1 || val>=pt ) {
	    IError( "Minimum Distance specifies bad point (%d) in sfd file\n", val );
	    err = true;
	} else if ( val!=-1 ) {
	    md->sp1 = mapping[val];
	    md->sp1->dontinterpolate = true;
	}
	ch = nlgetc(sfd);
	if ( ch!=',' ) {
	    IError( "Minimum Distance lacks a comma where expected\n" );
	    err = true;
	}
	getint(sfd,&val);
	if ( val<-1 || val>=pt ) {
	    IError( "Minimum Distance specifies bad point (%d) in sfd file\n", val );
	    err = true;
	} else if ( val!=-1 ) {
	    md->sp2 = mapping[val];
	    md->sp2->dontinterpolate = true;
	}
	if ( !err ) {
	    if ( last!=NULL )
		last->next = md;
	    last = md;
	} else
	    chunkfree(md,sizeof(MinimumDistance));
    }
    free(mapping);

    /* Obsolete concept */
    MinimumDistancesFree(mdhead);
}

static HintInstance *SFDReadHintInstances(FILE *sfd, StemInfo *stem) {
    HintInstance *head=NULL, *last=NULL, *cur;
    real begin, end;
    int ch;

    while ( (ch=nlgetc(sfd))==' ' || ch=='\t' );
    if ( ch=='G' && stem != NULL ) {
	stem->ghost = true;
	while ( (ch=nlgetc(sfd))==' ' || ch=='\t' );
    }
    if ( ch!='<' ) {
	ungetc(ch,sfd);
return(NULL);
    }
    while ( getreal(sfd,&begin)==1 && getreal(sfd,&end)) {
	cur = chunkalloc(sizeof(HintInstance));
	cur->begin = begin;
	cur->end = end;
	if ( head == NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;
    }
    while ( (ch=nlgetc(sfd))==' ' || ch=='\t' );
    if ( ch!='>' )
	ungetc(ch,sfd);
return( head );
}

static StemInfo *SFDReadHints(FILE *sfd) {
    StemInfo *head=NULL, *last=NULL, *cur;
    real start, width;

    while ( getreal(sfd,&start)==1 && getreal(sfd,&width)) {
	cur = chunkalloc(sizeof(StemInfo));
	cur->start = start;
	cur->width = width;
	cur->where = SFDReadHintInstances(sfd,cur);
	if ( head == NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;
    }
return( head );
}

static DStemInfo *SFDReadDHints( SplineFont *sf,FILE *sfd,int old ) {
    DStemInfo *head=NULL, *last=NULL, *cur;
    int i;
    BasePoint bp[4], *bpref[4], left, right, unit;
    double rstartoff, rendoff, lendoff;

    if ( old ) {
        for ( i=0 ; i<4 ; i++ ) bpref[i] = &bp[i];

        while ( getreal( sfd,&bp[0].x ) && getreal( sfd,&bp[0].y ) &&
	        getreal( sfd,&bp[1].x ) && getreal( sfd,&bp[1].y ) &&
	        getreal( sfd,&bp[2].x ) && getreal( sfd,&bp[2].y ) &&
	        getreal( sfd,&bp[3].x ) && getreal( sfd,&bp[3].y )) {

            /* Ensure point coordinates specified in the sfd file do */
            /* form a diagonal line */
            if ( PointsDiagonalable( sf,bpref,&unit )) {
	        cur = chunkalloc( sizeof( DStemInfo ));
	        cur->left = *bpref[0];
	        cur->right = *bpref[1];
                cur->unit = unit;
                /* Generate a temporary hint instance, so that the hint can */
                /* be visible in charview even if subsequent rebuilding instances */
                /* fails (e. g. for composite characters) */
                cur->where = chunkalloc( sizeof( HintInstance ));
                rstartoff = ( cur->right.x - cur->left.x ) * cur->unit.x +
                            ( cur->right.y - cur->left.y ) * cur->unit.y;
                rendoff =   ( bpref[2]->x - cur->left.x ) * cur->unit.x +
                            ( bpref[2]->y - cur->left.y ) * cur->unit.y;
                lendoff =   ( bpref[3]->x - cur->left.x ) * cur->unit.x +
                            ( bpref[3]->y - cur->left.y ) * cur->unit.y;
                cur->where->begin = ( rstartoff > 0 ) ? rstartoff : 0;
                cur->where->end   = ( rendoff > lendoff ) ? lendoff : rendoff;
                MergeDStemInfo( sf,&head,cur );
            }
        }
    } else {
        while ( getreal( sfd,&left.x ) && getreal( sfd,&left.y ) &&
                getreal( sfd,&right.x ) && getreal( sfd,&right.y ) &&
                getreal( sfd,&unit.x ) && getreal( sfd,&unit.y )) {
	    cur = chunkalloc( sizeof( DStemInfo ));
	    cur->left = left;
	    cur->right = right;
            cur->unit = unit;
	    cur->where = SFDReadHintInstances( sfd,NULL );
	    if ( head == NULL )
	        head = cur;
	    else
	        last->next = cur;
	    last = cur;
        }
    }
return( head );
}

static DeviceTable *SFDReadDeviceTable(FILE *sfd,DeviceTable *adjust) {
    int i, junk, first, last, ch, len;

    while ( (ch=nlgetc(sfd))==' ' );
    if ( ch=='{' ) {
	while ( (ch=nlgetc(sfd))==' ' );
	if ( ch=='}' )
return(NULL);
	else
	    ungetc(ch,sfd);
	getint(sfd,&first);
	ch = nlgetc(sfd);		/* Should be '-' */
	getint(sfd,&last);
	len = last-first+1;
	if ( len<=0 ) {
	    IError( "Bad device table, invalid length.\n" );
return(NULL);
	}
	if ( adjust==NULL )
	    adjust = chunkalloc(sizeof(DeviceTable));
	adjust->first_pixel_size = first;
	adjust->last_pixel_size = last;
	adjust->corrections = malloc(len);
	for ( i=0; i<len; ++i ) {
	    while ( (ch=nlgetc(sfd))==' ' );
	    if ( ch!=',' ) ungetc(ch,sfd);
	    getint(sfd,&junk);
	    adjust->corrections[i] = junk;
	}
	while ( (ch=nlgetc(sfd))==' ' );
	if ( ch!='}' ) ungetc(ch,sfd);
    } else
	ungetc(ch,sfd);
return( adjust );
}

static ValDevTab *SFDReadValDevTab(FILE *sfd) {
    int i, j, ch;
    ValDevTab vdt;
    char buf[4];

    memset(&vdt,0,sizeof(vdt));
    buf[3] = '\0';
    while ( (ch=nlgetc(sfd))==' ' );
    if ( ch=='[' ) {
	for ( i=0; i<4; ++i ) {
	    while ( (ch=nlgetc(sfd))==' ' );
	    if ( ch==']' )
	break;
	    buf[0]=ch;
	    for ( j=1; j<3; ++j ) buf[j]=nlgetc(sfd);
	    while ( (ch=nlgetc(sfd))==' ' );
	    if ( ch!='=' ) ungetc(ch,sfd);
	    SFDReadDeviceTable(sfd,
		    strcmp(buf,"ddx")==0 ? &vdt.xadjust :
		    strcmp(buf,"ddy")==0 ? &vdt.yadjust :
		    strcmp(buf,"ddh")==0 ? &vdt.xadv :
		    strcmp(buf,"ddv")==0 ? &vdt.yadv :
			(&vdt.xadjust) + i );
	    while ( (ch=nlgetc(sfd))==' ' );
	    if ( ch!=']' ) ungetc(ch,sfd);
	    else
	break;
	}
	if ( vdt.xadjust.corrections!=NULL || vdt.yadjust.corrections!=NULL ||
		vdt.xadv.corrections!=NULL || vdt.yadv.corrections!=NULL ) {
	    ValDevTab *v = chunkalloc(sizeof(ValDevTab));
	    *v = vdt;
return( v );
	}
    } else
	ungetc(ch,sfd);
return( NULL );
}

static AnchorPoint *SFDReadAnchorPoints(FILE *sfd,SplineChar *sc,AnchorPoint** alist, AnchorPoint *lastap)
{
    AnchorPoint *ap = chunkalloc(sizeof(AnchorPoint));
    AnchorClass *an;
    char *name;
    char tok[200];
    int ch;

    name = SFDReadUTF7Str(sfd);
    if ( name==NULL ) {
	LogError(_("Anchor Point with no class name: %s"), sc->name );
	AnchorPointsFree(ap);
return( lastap );
    }
    for ( an=sc->parent->anchor; an!=NULL && strcmp(an->name,name)!=0; an=an->next );
    free(name);
    ap->anchor = an;
    getreal(sfd,&ap->me.x);
    getreal(sfd,&ap->me.y);
    ap->type = -1;
    if ( getname(sfd,tok)==1 ) {
	if ( strcmp(tok,"mark")==0 )
	    ap->type = at_mark;
	else if ( strcmp(tok,"basechar")==0 )
	    ap->type = at_basechar;
	else if ( strcmp(tok,"baselig")==0 )
	    ap->type = at_baselig;
	else if ( strcmp(tok,"basemark")==0 )
	    ap->type = at_basemark;
	else if ( strcmp(tok,"entry")==0 )
	    ap->type = at_centry;
	else if ( strcmp(tok,"exit")==0 )
	    ap->type = at_cexit;
    }
    getsint(sfd,&ap->lig_index);
    ch = nlgetc(sfd);
    ungetc(ch,sfd);
    if ( ch==' ' ) {
	SFDReadDeviceTable(sfd,&ap->xadjust);
	SFDReadDeviceTable(sfd,&ap->yadjust);
	ch = nlgetc(sfd);
	ungetc(ch,sfd);
	if ( isdigit(ch)) {
	    getsint(sfd,(int16 *) &ap->ttf_pt_index);
	    ap->has_ttf_pt = true;
	}
    }
    if ( ap->anchor==NULL || ap->type==-1 ) {
	LogError(_("Bad Anchor Point: %s"), sc->name );
	AnchorPointsFree(ap);
return( lastap );
    }
    if ( lastap==NULL )
	(*alist) = ap;
    else
	lastap->next = ap;

    return( ap );
}

static RefChar *SFDGetRef(FILE *sfd, int was_enc) {
    RefChar *rf;
    int temp=0, ch;

    rf = RefCharCreate();
    getint(sfd,&rf->orig_pos);
    rf->encoded = was_enc;
    if ( getint(sfd,&temp))
	rf->unicode_enc = temp;
    while ( isspace(ch=nlgetc(sfd)));
    if ( ch=='S' ) rf->selected = true;
    getreal(sfd,&rf->transform[0]);
    getreal(sfd,&rf->transform[1]);
    getreal(sfd,&rf->transform[2]);
    getreal(sfd,&rf->transform[3]);
    getreal(sfd,&rf->transform[4]);
    getreal(sfd,&rf->transform[5]);
    while ( (ch=nlgetc(sfd))==' ');
    ungetc(ch,sfd);
    if ( isdigit(ch) ) {
	getint(sfd,&temp);
	rf->use_my_metrics = temp&1;
	rf->round_translation_to_grid = (temp&2)?1:0;
	rf->point_match = (temp&4)?1:0;
	if ( rf->point_match ) {
	    getsint(sfd,(int16 *) &rf->match_pt_base);
	    getsint(sfd,(int16 *) &rf->match_pt_ref);
	    while ( (ch=nlgetc(sfd))==' ');
	    if ( ch=='O' )
		rf->point_match_out_of_date = true;
	    else
		ungetc(ch,sfd);
	}
    }
return( rf );
}

/* I used to create multiple ligatures by putting ";" between them */
/* that is the component string for "ffi" was "ff i ; f f i" */
/* Now I want to have separate ligature structures for each */
static PST1 *LigaCreateFromOldStyleMultiple(PST1 *liga) {
    char *pt;
    PST1 *new, *last=liga;
    while ( (pt = strrchr(liga->pst.u.lig.components,';'))!=NULL ) {
	new = chunkalloc(sizeof( PST1 ));
	*new = *liga;
	new->pst.u.lig.components = copy(pt+1);
	last->pst.next = (PST *) new;
	last = new;
	*pt = '\0';
    }
return( last );
}

#ifdef FONTFORGE_CONFIG_CVT_OLD_MAC_FEATURES
static struct { int feature, setting; uint32 tag; } formertags[] = {
    { 1, 6, CHR('M','L','O','G') },
    { 1, 8, CHR('M','R','E','B') },
    { 1, 10, CHR('M','D','L','G') },
    { 1, 12, CHR('M','S','L','G') },
    { 1, 14, CHR('M','A','L','G') },
    { 8, 0, CHR('M','S','W','I') },
    { 8, 2, CHR('M','S','W','F') },
    { 8, 4, CHR('M','S','L','I') },
    { 8, 6, CHR('M','S','L','F') },
    { 8, 8, CHR('M','S','N','F') },
    { 22, 1, CHR('M','W','I','D') },
    { 27, 1, CHR('M','U','C','M') },
    { 103, 2, CHR('M','W','I','D') },
    { -1, -1, 0xffffffff },
};

static void CvtOldMacFeature(PST1 *pst) {
    int i;

    if ( pst->macfeature )
return;
    for ( i=0; formertags[i].feature!=-1 ; ++i ) {
	if ( pst->tag == formertags[i].tag ) {
	    pst->macfeature = true;
	    pst->tag = (formertags[i].feature<<16) | formertags[i].setting;
return;
	}
    }
}
#endif

static void SFDSetEncMap(SplineFont *sf,int orig_pos,int enc) {
    EncMap *map = sf->map;

    if ( map==NULL )
return;

    if ( orig_pos>=map->backmax ) {
	int old = map->backmax;
	map->backmax = orig_pos+10;
	map->backmap = realloc(map->backmap,map->backmax*sizeof(int));
	memset(map->backmap+old,-1,(map->backmax-old)*sizeof(int));
    }
    if ( map->backmap[orig_pos] == -1 )		/* backmap will not be unique if multiple encodings come from same glyph */
	map->backmap[orig_pos] = enc;
    if ( enc>=map->encmax ) {
	int old = map->encmax;
	map->encmax = enc+10;
	map->map = realloc(map->map,map->encmax*sizeof(int));
	memset(map->map+old,-1,(map->encmax-old)*sizeof(int));
    }
    if ( enc>=map->enccount )
	map->enccount = enc+1;
    if ( enc!=-1 )
	map->map[enc] = orig_pos;
}

static void SCDefaultInterpolation(SplineChar *sc) {
    SplineSet *cur;
    SplinePoint *sp;
    /* We used not to store the dontinterpolate bit. We used to use the */
    /* presence or absence of instructions as that flag */

    if ( sc->ttf_instrs_len!=0 ) {
	for ( cur=sc->layers[ly_fore].splines; cur!=NULL; cur=cur->next ) {
	    for ( sp=cur->first; ; ) {
		if ( sp->ttfindex!=0xffff && SPInterpolate(sp))
		    sp->dontinterpolate = true;
		if ( sp->next==NULL )
	    break;
		sp=sp->next->to;
		if ( sp==cur->first )
	    break;
	    }
	}
    }
}

static void SFDParseMathValueRecord(FILE *sfd,int16 *value,DeviceTable **devtab) {
    getsint(sfd,value);
    *devtab = SFDReadDeviceTable(sfd,NULL);
}

static struct glyphvariants *SFDParseGlyphComposition(FILE *sfd,
	struct glyphvariants *gv, char *tok) {
    int i;

    if ( gv==NULL )
	gv = chunkalloc(sizeof(struct glyphvariants));
    getint(sfd,&gv->part_cnt);
    gv->parts = calloc(gv->part_cnt,sizeof(struct gv_part));
    for ( i=0; i<gv->part_cnt; ++i ) {
	int temp, ch;
	getname(sfd,tok);
	gv->parts[i].component = copy(tok);
	while ( (ch=nlgetc(sfd))==' ' );
	if ( ch!='%' ) ungetc(ch,sfd);
	getint(sfd,&temp);
	gv->parts[i].is_extender = temp;
	while ( (ch=nlgetc(sfd))==' ' );
	if ( ch!=',' ) ungetc(ch,sfd);
	getint(sfd,&temp);
	gv->parts[i].startConnectorLength=temp;
	while ( (ch=nlgetc(sfd))==' ' );
	if ( ch!=',' ) ungetc(ch,sfd);
	getint(sfd,&temp);
	gv->parts[i].endConnectorLength = temp;
	while ( (ch=nlgetc(sfd))==' ' );
	if ( ch!=',' ) ungetc(ch,sfd);
	getint(sfd,&temp);
	gv->parts[i].fullAdvance = temp;
    }
return( gv );
}

static void SFDParseVertexKern(FILE *sfd, struct mathkernvertex *vertex) {
    int i,ch;

    getint(sfd,&vertex->cnt);
    vertex->mkd = calloc(vertex->cnt,sizeof(struct mathkerndata));
    for ( i=0; i<vertex->cnt; ++i ) {
	SFDParseMathValueRecord(sfd,&vertex->mkd[i].height,&vertex->mkd[i].height_adjusts);
	while ( (ch=nlgetc(sfd))==' ' );
	if ( ch!=EOF && ch!=',' )
	    ungetc(ch,sfd);
	SFDParseMathValueRecord(sfd,&vertex->mkd[i].kern,&vertex->mkd[i].kern_adjusts);
    }
}

static struct gradient *SFDParseGradient(FILE *sfd,char *tok) {
    struct gradient *grad = chunkalloc(sizeof(struct gradient));
    int ch, i;

    getreal(sfd,&grad->start.x);
    while ( isspace(ch=nlgetc(sfd)));
    if ( ch!=';' ) ungetc(ch,sfd);
    getreal(sfd,&grad->start.y);

    getreal(sfd,&grad->stop.x);
    while ( isspace(ch=nlgetc(sfd)));
    if ( ch!=';' ) ungetc(ch,sfd);
    getreal(sfd,&grad->stop.y);

    getreal(sfd,&grad->radius);

    getname(sfd,tok);
    for ( i=0; spreads[i]!=NULL; ++i )
	if ( strmatch(spreads[i],tok)==0 )
    break;
    if ( spreads[i]==NULL ) i=0;
    grad->sm = i;

    getint(sfd,&grad->stop_cnt);
    grad->grad_stops = calloc(grad->stop_cnt,sizeof(struct grad_stops));
    for ( i=0; i<grad->stop_cnt; ++i ) {
	while ( isspace(ch=nlgetc(sfd)));
	if ( ch!='{' ) ungetc(ch,sfd);
	getreal( sfd, &grad->grad_stops[i].offset );
	gethex( sfd, &grad->grad_stops[i].col );
	getreal( sfd, &grad->grad_stops[i].opacity );
	while ( isspace(ch=nlgetc(sfd)));
	if ( ch!='}' ) ungetc(ch,sfd);
    }
return( grad );
}

static struct pattern *SFDParsePattern(FILE *sfd,char *tok) {
    struct pattern *pat = chunkalloc(sizeof(struct pattern));
    int ch;

    getname(sfd,tok);
    pat->pattern = copy(tok);

    getreal(sfd,&pat->width);
    while ( isspace(ch=nlgetc(sfd)));
    if ( ch!=';' ) ungetc(ch,sfd);
    getreal(sfd,&pat->height);

    while ( isspace(ch=nlgetc(sfd)));
    if ( ch!='[' ) ungetc(ch,sfd);
    getreal(sfd,&pat->transform[0]);
    getreal(sfd,&pat->transform[1]);
    getreal(sfd,&pat->transform[2]);
    getreal(sfd,&pat->transform[3]);
    getreal(sfd,&pat->transform[4]);
    getreal(sfd,&pat->transform[5]);
    while ( isspace(ch=nlgetc(sfd)));
    if ( ch!=']' ) ungetc(ch,sfd);
return( pat );
}


static void SFDConsumeUntil( FILE *sfd, const char** terminators ) {

    char* line = 0;
    while((line = getquotedeol( sfd ))) {
        const char** tp = terminators;
        for( ; tp && *tp; ++tp ) {
            if( !strnmatch( line, *tp, strlen( *tp ))) {
                free(line);
                return;
            }
        }
        free(line);
    }
}

static int orig_pos;

void SFDGetKerns( FILE *sfd, SplineChar *sc, char* ttok ) {
    struct splinefont * sf = sc->parent;
    char tok[2001], ch;
    uint32 script = 0;
    SplineFont *sli_sf = sf->cidmaster ? sf->cidmaster : sf;

    strncpy(tok,ttok,sizeof(tok)-1);
    tok[2000]=0;

    if( strmatch(tok,"Kerns2:")==0 ||
	strmatch(tok,"VKerns2:")==0 ) {
	    KernPair *kp, *last=NULL;
	    int isv = *tok=='V';
	    int off, index;
	    struct lookup_subtable *sub;
	    int kernCount = 0;
	    if ( sf->sfd_version<2 )
		LogError(_("Found an new style kerning pair inside a version 1 (or lower) sfd file.\n") );
	    while ( fscanf(sfd,"%d %d", &index, &off )==2 ) {
		sub = SFFindLookupSubtableAndFreeName(sf,SFDReadUTF7Str(sfd));
		if ( sub==NULL ) {
		    LogError(_("KernPair with no subtable name.\n"));
	    	    break;
		}
		kernCount++;
		kp = chunkalloc(sizeof(KernPair1));
		kp->sc = (SplineChar *) (intpt) index;
		kp->kcid = true;
		kp->off = off;
		kp->subtable = sub;
		kp->next = NULL;
		while ( (ch=nlgetc(sfd))==' ' );
		ungetc(ch,sfd);
		if ( ch=='{' ) {
		    kp->adjust = SFDReadDeviceTable(sfd, NULL);
		}
		if ( last != NULL )
		    last->next = kp;
		else if ( isv )
		    sc->vkerns = kp;
		else
		    sc->kerns = kp;
		last = kp;
	    }
	    if( !kernCount ) {
//		printf("SFDGetKerns() have a BLANK KERN\n");
		sc->kerns = 0;
	    }
    } else if ( strmatch(tok,"Kerns:")==0 ||
		strmatch(tok,"KernsSLI:")==0 ||
		strmatch(tok,"KernsSLIF:")==0 ||
		strmatch(tok,"VKernsSLIF:")==0 ||
		strmatch(tok,"KernsSLIFO:")==0 ||
		strmatch(tok,"VKernsSLIFO:")==0 ) {
	    KernPair1 *kp, *last=NULL;
	    int index, off, sli, flags=0;
	    int hassli = (strmatch(tok,"KernsSLI:")==0);
	    int isv = *tok=='V';
	    int has_orig = strstr(tok,"SLIFO:")!=NULL;
	    if ( sf->sfd_version>=2 ) {
		IError( "Found an old style kerning pair inside a version 2 (or higher) sfd file." );
exit(1);
	    }
	    if ( strmatch(tok,"KernsSLIF:")==0 || strmatch(tok,"KernsSLIFO:")==0 ||
		    strmatch(tok,"VKernsSLIF:")==0 || strmatch(tok,"VKernsSLIFO:")==0 )
		hassli=2;
	    while ( (hassli==1 && fscanf(sfd,"%d %d %d", &index, &off, &sli )==3) ||
		    (hassli==2 && fscanf(sfd,"%d %d %d %d", &index, &off, &sli, &flags )==4) ||
		    (hassli==0 && fscanf(sfd,"%d %d", &index, &off )==2) ) {
		if ( !hassli )
		    sli = SFFindBiggestScriptLangIndex(sli_sf,
			    script!=0?script:SCScriptFromUnicode(sc),DEFAULT_LANG);
		if ( sli>=((SplineFont1 *) sli_sf)->sli_cnt && sli!=SLI_NESTED) {
		    static int complained=false;
		    if ( !complained )
			IError("'%s' in %s has a script index out of bounds: %d",
				isv ? "vkrn" : "kern",
				sc->name, sli );
		    sli = SFFindBiggestScriptLangIndex(sli_sf,
			    SCScriptFromUnicode(sc),DEFAULT_LANG);
		    complained = true;
		}
		kp = chunkalloc(sizeof(KernPair1));
		kp->kp.sc = (SplineChar *) (intpt) index;
		kp->kp.kcid = has_orig;
		kp->kp.off = off;
		kp->sli = sli;
		kp->flags = flags;
		kp->kp.next = NULL;
		while ( (ch=nlgetc(sfd))==' ' );
		ungetc(ch,sfd);
		if ( ch=='{' ) {
		    kp->kp.adjust = SFDReadDeviceTable(sfd, NULL);
		}
		if ( last != NULL )
		    last->kp.next = (KernPair *) kp;
		else if ( isv )
		    sc->vkerns = (KernPair *) kp;
		else
		    sc->kerns = (KernPair *) kp;
		last = kp;
	    }
    } else {
	return;
    }

    // we matched something, grab the next top level token to ttok
    getname( sfd, ttok );
}


void SFDGetPSTs( FILE *sfd, SplineChar *sc, char* ttok ) {
    struct splinefont * sf = sc->parent;
    char tok[2001], ch;
    int isliga = 0, ispos, issubs, ismult, islcar, ispair, temp;
    PST *last = NULL;
    uint32 script = 0;
    SplineFont *sli_sf = sf->cidmaster ? sf->cidmaster : sf;

    strncpy(tok,ttok,sizeof(tok)-1);

    if ( strmatch(tok,"Script:")==0 ) {
	/* Obsolete. But still used for parsing obsolete ligature/subs tags */
	while ( (ch=nlgetc(sfd))==' ' || ch=='\t' );
	if ( ch=='\n' || ch=='\r' )
	    script = 0;
	else {
	    ungetc(ch,sfd);
	    script = gettag(sfd);
	}
    } else if ( (ispos = (strmatch(tok,"Position:")==0)) ||
		( ispos  = (strmatch(tok,"Position2:")==0)) ||
		( ispair = (strmatch(tok,"PairPos:")==0)) ||
		( ispair = (strmatch(tok,"PairPos2:")==0)) ||
		( islcar = (strmatch(tok,"LCarets:")==0)) ||
		( islcar = (strmatch(tok,"LCarets2:")==0)) ||
		( isliga = (strmatch(tok,"Ligature:")==0)) ||
		( isliga = (strmatch(tok,"Ligature2:")==0)) ||
		( issubs = (strmatch(tok,"Substitution:")==0)) ||
		( issubs = (strmatch(tok,"Substitution2:")==0)) ||
		( ismult = (strmatch(tok,"MultipleSubs:")==0)) ||
		( ismult = (strmatch(tok,"MultipleSubs2:")==0)) ||
		strmatch(tok,"AlternateSubs:")==0 ||
		strmatch(tok,"AlternateSubs2:")==0 ) {
	    PST *pst;
	    int old, type;
	    type = ispos ? pst_position :
			 ispair ? pst_pair :
			 islcar ? pst_lcaret :
			 isliga ? pst_ligature :
			 issubs ? pst_substitution :
			 ismult ? pst_multiple :
			 pst_alternate;
	    if ( strchr(tok,'2')!=NULL ) {
		old = false;
		pst = chunkalloc(sizeof(PST));
		if ( type!=pst_lcaret )
		    pst->subtable = SFFindLookupSubtableAndFreeName(sf,SFDReadUTF7Str(sfd));
	    } else {
		old = true;
		pst = chunkalloc(sizeof(PST1));
		((PST1 *) pst)->tag = CHR('l','i','g','a');
		((PST1 *) pst)->script_lang_index = 0xffff;
		while ( (ch=nlgetc(sfd))==' ' || ch=='\t' );
		if ( isdigit(ch)) {
		    int temp;
		    ungetc(ch,sfd);
		    getint(sfd,&temp);
		    ((PST1 *) pst)->flags = temp;
		    while ( (ch=nlgetc(sfd))==' ' || ch=='\t' );
		} else
		    ((PST1 *) pst)->flags = 0 /*PSTDefaultFlags(type,sc)*/;
		if ( isdigit(ch)) {
		    ungetc(ch,sfd);
		    getusint(sfd,&((PST1 *) pst)->script_lang_index);
		    while ( (ch=nlgetc(sfd))==' ' || ch=='\t' );
		} else
		    ((PST1 *) pst)->script_lang_index = SFFindBiggestScriptLangIndex(sf,
			    script!=0?script:SCScriptFromUnicode(sc),DEFAULT_LANG);
		if ( ch=='\'' ) {
		    ungetc(ch,sfd);
		    ((PST1 *) pst)->tag = gettag(sfd);
		} else if ( ch=='<' ) {
		    getint(sfd,&temp);
		    ((PST1 *) pst)->tag = temp<<16;
		    nlgetc(sfd);	/* comma */
		    getint(sfd,&temp);
		    ((PST1 *) pst)->tag |= temp;
		    nlgetc(sfd);	/* close '>' */
		    ((PST1 *) pst)->macfeature = true;
		} else
		    ungetc(ch,sfd);
		if ( type==pst_lcaret ) {
		/* These are meaningless for lcarets, set them to innocuous values */
		    ((PST1 *) pst)->script_lang_index = SLI_UNKNOWN;
		    ((PST1 *) pst)->tag = CHR(' ',' ',' ',' ');
		} else if ( ((PST1 *) pst)->script_lang_index>=((SplineFont1 *) sli_sf)->sli_cnt && ((PST1 *) pst)->script_lang_index!=SLI_NESTED ) {
		    static int complained=false;
		    if ( !complained )
			IError("'%c%c%c%c' in %s has a script index out of bounds: %d",
				(((PST1 *) pst)->tag>>24), (((PST1 *) pst)->tag>>16)&0xff, (((PST1 *) pst)->tag>>8)&0xff, ((PST1 *) pst)->tag&0xff,
				sc->name, ((PST1 *) pst)->script_lang_index );
		    else
			IError( "'%c%c%c%c' in %s has a script index out of bounds: %d\n",
				(((PST1 *) pst)->tag>>24), (((PST1 *) pst)->tag>>16)&0xff, (((PST1 *) pst)->tag>>8)&0xff, ((PST1 *) pst)->tag&0xff,
				sc->name, ((PST1 *) pst)->script_lang_index );
		    ((PST1 *) pst)->script_lang_index = SFFindBiggestScriptLangIndex(sli_sf,
			    SCScriptFromUnicode(sc),DEFAULT_LANG);
		    complained = true;
		}
	    }
	    if ( (sf->sfd_version<2)!=old ) {
		IError( "Version mixup in PST of sfd file." );
exit(1);
	    }
	    if ( last==NULL )
		sc->possub = pst;
	    else
		last->next = pst;
	    last = pst;
	    pst->type = type;
	    if ( pst->type==pst_position ) {
		fscanf( sfd, " dx=%hd dy=%hd dh=%hd dv=%hd",
			&pst->u.pos.xoff, &pst->u.pos.yoff,
			&pst->u.pos.h_adv_off, &pst->u.pos.v_adv_off);
		pst->u.pos.adjust = SFDReadValDevTab(sfd);
		ch = nlgetc(sfd);		/* Eat new line */
	    } else if ( pst->type==pst_pair ) {
		getname(sfd,tok);
		pst->u.pair.paired = copy(tok);
		pst->u.pair.vr = chunkalloc(sizeof(struct vr [2]));
		fscanf( sfd, " dx=%hd dy=%hd dh=%hd dv=%hd",
			&pst->u.pair.vr[0].xoff, &pst->u.pair.vr[0].yoff,
			&pst->u.pair.vr[0].h_adv_off, &pst->u.pair.vr[0].v_adv_off);
		pst->u.pair.vr[0].adjust = SFDReadValDevTab(sfd);
		fscanf( sfd, " dx=%hd dy=%hd dh=%hd dv=%hd",
			&pst->u.pair.vr[1].xoff, &pst->u.pair.vr[1].yoff,
			&pst->u.pair.vr[1].h_adv_off, &pst->u.pair.vr[1].v_adv_off);
		pst->u.pair.vr[0].adjust = SFDReadValDevTab(sfd);
		ch = nlgetc(sfd);
	    } else if ( pst->type==pst_lcaret ) {
		int i;
		fscanf( sfd, " %d", &pst->u.lcaret.cnt );
		pst->u.lcaret.carets = malloc(pst->u.lcaret.cnt*sizeof(int16));
		for ( i=0; i<pst->u.lcaret.cnt; ++i )
		    fscanf( sfd, " %hd", &pst->u.lcaret.carets[i]);
		geteol(sfd,tok);
	    } else {
		geteol(sfd,tok);
		pst->u.lig.components = copy(tok);	/* it's in the same place for all formats */
		if ( isliga ) {
		    pst->u.lig.lig = sc;
		    if ( old )
			last = (PST *) LigaCreateFromOldStyleMultiple((PST1 *) pst);
		}
	    }
#ifdef FONTFORGE_CONFIG_CVT_OLD_MAC_FEATURES
	    if ( old )
		CvtOldMacFeature((PST1 *) pst);
#endif
    } else {
	return;
    }

    // we matched something, grab the next top level token to ttok
    getname( sfd, ttok );
}


char* SFDMoveToNextStartChar( FILE* sfd ) {
    char ret[2000];

    memset( ret, '\0', 2000 );
    char* line = 0;
    while((line = getquotedeol( sfd ))) {
	if( !strnmatch( line, "StartChar:", strlen( "StartChar:" ))) {
	    // FIXME: use the getname()/SFDReadUTF7Str() combo
	    // from SFDGetChar
	    int len = strlen("StartChar:");
	    while( line[len] && line[len] == ' ' )
		len++;
	    strcpy( ret, line+len );
	    free(line);
	    return copy(ret);
	}
	free(line);
	if(feof( sfd ))
	    break;

    }
    return 0;
}

static SplineChar *SFDGetChar(FILE *sfd,SplineFont *sf, int had_sf_layer_cnt) {
    SplineChar *sc;
    char tok[2000], ch;
    RefChar *lastr=NULL, *ref;
    ImageList *lasti=NULL, *img;
    AnchorPoint *lastap = NULL;
    int isliga = 0, ispos, issubs, ismult, islcar, ispair, temp, i;
    PST *last = NULL;
    uint32 script = 0;
    int current_layer = ly_fore;
    int multilayer = sf->multilayer;
    int had_old_dstems = false;
    SplineFont *sli_sf = sf->cidmaster ? sf->cidmaster : sf;
    struct altuni *altuni;
    int oldback = false;

    if ( getname(sfd,tok)!=1 )
return( NULL );
    if ( strcmp(tok,"StartChar:")!=0 )
return( NULL );
    while ( isspace(ch=nlgetc(sfd)));
    ungetc(ch,sfd);
    sc = SFSplineCharCreate(sf);
    if ( ch!='"' ) {
	if ( getname(sfd,tok)!=1 ) {
	    SplineCharFree(sc);
return( NULL );
	}
	sc->name = copy(tok);
    } else {
	sc->name = SFDReadUTF7Str(sfd);
	if ( sc->name==NULL ) {
	    SplineCharFree(sc);
return( NULL );
	}
    }
    sc->vwidth = sf->ascent+sf->descent;
    sc->parent = sf;
    while ( 1 ) {
	if ( getname(sfd,tok)!=1 ) {
	    SplineCharFree(sc);
return( NULL );
	}
	if ( strmatch(tok,"Encoding:")==0 ) {
	    int enc;
	    getint(sfd,&enc);
	    getint(sfd,&sc->unicodeenc);
	    while ( (ch=nlgetc(sfd))==' ' || ch=='\t' );
	    ungetc(ch,sfd);
	    if ( ch!='\n' && ch!='\r' ) {
		getint(sfd,&sc->orig_pos);
		if ( sc->orig_pos==65535 )
		    sc->orig_pos = orig_pos++;
		    /* An old mark meaning: "I don't know" */
		if ( sc->orig_pos<sf->glyphcnt && sf->glyphs[sc->orig_pos]!=NULL )
		    sc->orig_pos = sf->glyphcnt;
		if ( sc->orig_pos>=sf->glyphcnt ) {
		    if ( sc->orig_pos>=sf->glyphmax )
			sf->glyphs = realloc(sf->glyphs,(sf->glyphmax = sc->orig_pos+10)*sizeof(SplineChar *));
		    memset(sf->glyphs+sf->glyphcnt,0,(sc->orig_pos+1-sf->glyphcnt)*sizeof(SplineChar *));
		    sf->glyphcnt = sc->orig_pos+1;
		}
		if ( sc->orig_pos+1 > orig_pos )
		    orig_pos = sc->orig_pos+1;
	    } else if ( sf->cidmaster!=NULL ) {		/* In cid fonts the orig_pos is just the cid */
		sc->orig_pos = enc;
	    } else {
		sc->orig_pos = orig_pos++;
	    }
	    SFDSetEncMap(sf,sc->orig_pos,enc);
	} else if ( strmatch(tok,"AltUni:")==0 ) {
	    int uni;
	    while ( getint(sfd,&uni)==1 ) {
		altuni = chunkalloc(sizeof(struct altuni));
		altuni->unienc = uni;
		altuni->vs = -1;
		altuni->fid = 0;
		altuni->next = sc->altuni;
		sc->altuni = altuni;
	    }
	} else if ( strmatch(tok,"AltUni2:")==0 ) {
	    uint32 uni[3];
	    while ( gethexints(sfd,uni,3) ) {
		altuni = chunkalloc(sizeof(struct altuni));
		altuni->unienc = uni[0];
		altuni->vs = uni[1];
		altuni->fid = uni[2];
		altuni->next = sc->altuni;
		sc->altuni = altuni;
	    }
	} else if ( strmatch(tok,"OldEncoding:")==0 ) {
	    int old_enc;		/* Obsolete info */
	    getint(sfd,&old_enc);
        } else if ( strmatch(tok,"Script:")==0 ) {
	    /* Obsolete. But still used for parsing obsolete ligature/subs tags */
            while ( (ch=nlgetc(sfd))==' ' || ch=='\t' );
            if ( ch=='\n' || ch=='\r' )
                script = 0;
            else {
		ungetc(ch,sfd);
		script = gettag(sfd);
            }
	} else if ( strmatch(tok,"GlifName:")==0 ) {
            while ( isspace(ch=nlgetc(sfd)));
            ungetc(ch,sfd);
            if ( ch!='"' ) {
              if ( getname(sfd,tok)!=1 ) {
                LogError(_("Invalid glif name.\n"));
              }
	      sc->glif_name = copy(tok);
            } else {
	      sc->glif_name = SFDReadUTF7Str(sfd);
	      if ( sc->glif_name==NULL ) {
                LogError(_("Invalid glif name.\n"));
	      }
            }
	} else if ( strmatch(tok,"Width:")==0 ) {
	    getsint(sfd,&sc->width);
	} else if ( strmatch(tok,"VWidth:")==0 ) {
	    getsint(sfd,&sc->vwidth);
	} else if ( strmatch(tok,"GlyphClass:")==0 ) {
	    getint(sfd,&temp);
	    sc->glyph_class = temp;
	} else if ( strmatch(tok,"UnlinkRmOvrlpSave:")==0 ) {
	    getint(sfd,&temp);
	    sc->unlink_rm_ovrlp_save_undo = temp;
	} else if ( strmatch(tok,"InSpiro:")==0 ) {
	    getint(sfd,&temp);
	    sc->inspiro = temp;
	} else if ( strmatch(tok,"LigCaretCntFixed:")==0 ) {
	    getint(sfd,&temp);
	    sc->lig_caret_cnt_fixed = temp;
	} else if ( strmatch(tok,"Flags:")==0 ) {
	    while ( isspace(ch=nlgetc(sfd)) && ch!='\n' && ch!='\r');
	    while ( ch!='\n' && ch!='\r' ) {
		if ( ch=='H' ) sc->changedsincelasthinted=true;
		else if ( ch=='M' ) sc->manualhints = true;
		else if ( ch=='W' ) sc->widthset = true;
		else if ( ch=='O' ) sc->wasopen = true;
		else if ( ch=='I' ) sc->instructions_out_of_date = true;
		ch = nlgetc(sfd);
	    }
	    if ( sf->multilayer || sf->onlybitmaps || sf->strokedfont || sc->layers[ly_fore].order2 )
		sc->changedsincelasthinted = false;
	} else if ( strmatch(tok,"TeX:")==0 ) {
	    getsint(sfd,&sc->tex_height);
	    getsint(sfd,&sc->tex_depth);
	    while ( isspace(ch=nlgetc(sfd)) && ch!='\n' && ch!='\r');
	    ungetc(ch,sfd);
	    if ( ch!='\n' && ch!='\r' ) {
		int16 old_tex;
		/* Used to store two extra values here */
		getsint(sfd,&old_tex);
		getsint(sfd,&old_tex);
		if ( sc->tex_height==0 && sc->tex_depth==0 )		/* Fixup old bug */
		    sc->tex_height = sc->tex_depth = TEX_UNDEF;
	    }
	} else if ( strmatch(tok,"ItalicCorrection:")==0 ) {
	    SFDParseMathValueRecord(sfd,&sc->italic_correction,&sc->italic_adjusts);
	} else if ( strmatch(tok,"TopAccentHorizontal:")==0 ) {
	    SFDParseMathValueRecord(sfd,&sc->top_accent_horiz,&sc->top_accent_adjusts);
	} else if ( strmatch(tok,"GlyphCompositionVerticalIC:")==0 ) {
	    if ( sc->vert_variants==NULL )
		sc->vert_variants = chunkalloc(sizeof(struct glyphvariants));
	    SFDParseMathValueRecord(sfd,&sc->vert_variants->italic_correction,&sc->vert_variants->italic_adjusts);
	} else if ( strmatch(tok,"GlyphCompositionHorizontalIC:")==0 ) {
	    if ( sc->horiz_variants==NULL )
		sc->horiz_variants = chunkalloc(sizeof(struct glyphvariants));
	    SFDParseMathValueRecord(sfd,&sc->horiz_variants->italic_correction,&sc->horiz_variants->italic_adjusts);
	} else if ( strmatch(tok,"IsExtendedShape:")==0 ) {
	    int temp;
	    getint(sfd,&temp);
	    sc->is_extended_shape = temp;
	} else if ( strmatch(tok,"GlyphVariantsVertical:")==0 ) {
	    if ( sc->vert_variants==NULL )
		sc->vert_variants = chunkalloc(sizeof(struct glyphvariants));
	    geteol(sfd,tok);
	    sc->vert_variants->variants = copy(tok);
	} else if ( strmatch(tok,"GlyphVariantsHorizontal:")==0 ) {
	    if ( sc->horiz_variants==NULL )
		sc->horiz_variants = chunkalloc(sizeof(struct glyphvariants));
	    geteol(sfd,tok);
	    sc->horiz_variants->variants = copy(tok);
	} else if ( strmatch(tok,"GlyphCompositionVertical:")==0 ) {
	    sc->vert_variants = SFDParseGlyphComposition(sfd, sc->vert_variants,tok);
	} else if ( strmatch(tok,"GlyphCompositionHorizontal:")==0 ) {
	    sc->horiz_variants = SFDParseGlyphComposition(sfd, sc->horiz_variants,tok);
	} else if ( strmatch(tok,"TopRightVertex:")==0 ) {
	    if ( sc->mathkern==NULL )
		sc->mathkern = chunkalloc(sizeof(struct mathkern));
	    SFDParseVertexKern(sfd, &sc->mathkern->top_right);
	} else if ( strmatch(tok,"TopLeftVertex:")==0 ) {
	    if ( sc->mathkern==NULL )
		sc->mathkern = chunkalloc(sizeof(struct mathkern));
	    SFDParseVertexKern(sfd, &sc->mathkern->top_left);
	} else if ( strmatch(tok,"BottomRightVertex:")==0 ) {
	    if ( sc->mathkern==NULL )
		sc->mathkern = chunkalloc(sizeof(struct mathkern));
	    SFDParseVertexKern(sfd, &sc->mathkern->bottom_right);
	} else if ( strmatch(tok,"BottomLeftVertex:")==0 ) {
	    if ( sc->mathkern==NULL )
		sc->mathkern = chunkalloc(sizeof(struct mathkern));
	    SFDParseVertexKern(sfd, &sc->mathkern->bottom_left);
#if HANYANG
	} else if ( strmatch(tok,"CompositionUnit:")==0 ) {
	    getsint(sfd,&sc->jamo);
	    getsint(sfd,&sc->varient);
	    sc->compositionunit = true;
#endif
	} else if ( strmatch(tok,"HStem:")==0 ) {
	    sc->hstem = SFDReadHints(sfd);
	    sc->hconflicts = StemListAnyConflicts(sc->hstem);
	} else if ( strmatch(tok,"VStem:")==0 ) {
	    sc->vstem = SFDReadHints(sfd);
	    sc->vconflicts = StemListAnyConflicts(sc->vstem);
	} else if ( strmatch(tok,"DStem:")==0 ) {
	    sc->dstem = SFDReadDHints( sc->parent,sfd,true );
            had_old_dstems = true;
	} else if ( strmatch(tok,"DStem2:")==0 ) {
	    sc->dstem = SFDReadDHints( sc->parent,sfd,false );
	} else if ( strmatch(tok,"CounterMasks:")==0 ) {
	    getsint(sfd,&sc->countermask_cnt);
	    sc->countermasks = calloc(sc->countermask_cnt,sizeof(HintMask));
	    for ( i=0; i<sc->countermask_cnt; ++i ) {
		int ch;
		while ( (ch=nlgetc(sfd))==' ' );
		ungetc(ch,sfd);
		SFDGetHintMask(sfd,&sc->countermasks[i]);
	    }
	} else if ( strmatch(tok,"AnchorPoint:")==0 ) {
	    lastap = SFDReadAnchorPoints(sfd,sc,&sc->anchor,lastap);
	} else if ( strmatch(tok,"Fore")==0 ) {
	    while ( isspace(ch = nlgetc(sfd)));
	    ungetc(ch,sfd);
	    if ( ch!='I' && ch!='R' && ch!='S' && ch!='V' && ch!=' ' && ch!='\n' && 
	         !PeekMatch(sfd, "Pickled") && !PeekMatch(sfd, "EndChar") &&
	         !PeekMatch(sfd, "Fore") && !PeekMatch(sfd, "Back") && !PeekMatch(sfd, "Layer") ) {
		/* Old format, without a SplineSet token */
		sc->layers[ly_fore].splines = SFDGetSplineSet(sfd,sc->layers[ly_fore].order2);
	    }
	    current_layer = ly_fore;
	} else if ( strmatch(tok,"MinimumDistance:")==0 ) {
	    SFDGetMinimumDistances(sfd,sc);
	} else if ( strmatch(tok,"Validated:")==0 ) {
	    getsint(sfd,(int16 *) &sc->layers[current_layer].validation_state);
	} else if ( strmatch(tok,"Back")==0 ) {
	    while ( isspace(ch=nlgetc(sfd)));
	    ungetc(ch,sfd);
	    if ( ch!='I' && ch!='R' && ch!='S' && ch!='V' && ch!=' ' && ch!='\n' &&
	         !PeekMatch(sfd, "Pickled") && !PeekMatch(sfd, "EndChar") &&
	         !PeekMatch(sfd, "Fore") && !PeekMatch(sfd, "Back") && !PeekMatch(sfd, "Layer") ) {
		/* Old format, without a SplineSet token */
		sc->layers[ly_back].splines = SFDGetSplineSet(sfd,sc->layers[ly_back].order2);
		oldback = true;
	    }
	    current_layer = ly_back;
	} else if ( strmatch(tok,"LayerCount:")==0 ) {
	    getint(sfd,&temp);
	    if ( temp>sc->layer_cnt ) {
		sc->layers = realloc(sc->layers,temp*sizeof(Layer));
		memset(sc->layers+sc->layer_cnt,0,(temp-sc->layer_cnt)*sizeof(Layer));
	    }
	    sc->layer_cnt = temp;
	    current_layer = ly_fore;
	} else if ( strmatch(tok,"Layer:")==0 ) {
	    int layer;
	    int dofill, dostroke, fillfirst, linejoin, linecap;
	    uint32 fillcol, strokecol;
	    real fillopacity, strokeopacity, strokewidth, trans[4];
	    DashType dashes[DASH_MAX];
	    int i;
	    getint(sfd,&layer);
	    if ( layer>=sc->layer_cnt ) {
		sc->layers = realloc(sc->layers,(layer+1)*sizeof(Layer));
		memset(sc->layers+sc->layer_cnt,0,(layer+1-sc->layer_cnt)*sizeof(Layer));
	    }
	    if ( sc->parent->multilayer ) {
		getint(sfd,&dofill);
		getint(sfd,&dostroke);
		getint(sfd,&fillfirst);
		gethex(sfd,&fillcol);
		getreal(sfd,&fillopacity);
		gethex(sfd,&strokecol);
		getreal(sfd,&strokeopacity);
		getreal(sfd,&strokewidth);
		getname(sfd,tok);
		for ( i=0; joins[i]!=NULL; ++i )
		    if ( strmatch(joins[i],tok)==0 )
		break;
		if ( joins[i]==NULL ) --i;
		linejoin = i;
		getname(sfd,tok);
		for ( i=0; caps[i]!=NULL; ++i )
		    if ( strmatch(caps[i],tok)==0 )
		break;
		if ( caps[i]==NULL ) --i;
		linecap = i;
		while ( (ch=nlgetc(sfd))==' ' || ch=='[' );
		ungetc(ch,sfd);
		getreal(sfd,&trans[0]);
		getreal(sfd,&trans[1]);
		getreal(sfd,&trans[2]);
		getreal(sfd,&trans[3]);
		while ( (ch=nlgetc(sfd))==' ' || ch==']' );
		if ( ch=='[' ) {
		    for ( i=0;; ++i ) { int temp;
			if ( !getint(sfd,&temp) )
		    break;
			else if ( i<DASH_MAX )
			    dashes[i] = temp;
		    }
		    if ( i<DASH_MAX )
			dashes[i] = 0;
		} else {
		    ungetc(ch,sfd);
		    memset(dashes,0,sizeof(dashes));
		}
		sc->layers[layer].dofill = dofill;
		sc->layers[layer].dostroke = dostroke;
		sc->layers[layer].fillfirst = fillfirst;
		sc->layers[layer].fill_brush.col = fillcol;
		sc->layers[layer].fill_brush.opacity = fillopacity;
		sc->layers[layer].stroke_pen.brush.col = strokecol;
		sc->layers[layer].stroke_pen.brush.opacity = strokeopacity;
		sc->layers[layer].stroke_pen.width = strokewidth;
		sc->layers[layer].stroke_pen.linejoin = linejoin;
		sc->layers[layer].stroke_pen.linecap = linecap;
		memcpy(sc->layers[layer].stroke_pen.dashes,dashes,sizeof(dashes));
		memcpy(sc->layers[layer].stroke_pen.trans,trans,sizeof(trans));
	    }
	    current_layer = layer;
	    lasti = NULL;
	    lastr = NULL;
	} else if ( strmatch(tok,"FillGradient:")==0 ) {
	    sc->layers[current_layer].fill_brush.gradient = SFDParseGradient(sfd,tok);
	} else if ( strmatch(tok,"FillPattern:")==0 ) {
	    sc->layers[current_layer].fill_brush.pattern = SFDParsePattern(sfd,tok);
	} else if ( strmatch(tok,"StrokeGradient:")==0 ) {
	    sc->layers[current_layer].stroke_pen.brush.gradient = SFDParseGradient(sfd,tok);
	} else if ( strmatch(tok,"StrokePattern:")==0 ) {
	    sc->layers[current_layer].stroke_pen.brush.pattern = SFDParsePattern(sfd,tok);
	} else if ( strmatch(tok,"UndoRedoHistory")==0 ) {

	    getname(sfd,tok);
	    if ( !strmatch(tok,"Layer:") ) {
		int layer;
		getint(sfd,&layer);
	    }

	    int limit;
	    Undoes *undo = 0;
	    struct undoes *last = 0;

	    getname(sfd,tok);
	    if ( !strmatch(tok,"Undoes") ) {
		undo = 0;
		limit = UndoRedoLimitToLoad;
		last = sc->layers[current_layer].undoes;
		while((undo = SFDGetUndo( sfd, sc, "UndoOperation", current_layer )))
		{
		    // push to back
		    if( last ) last->next = undo;
		    else       sc->layers[current_layer].undoes = undo;
		    last = undo;

		    if( limit != -1 ) {
			limit--;
			if( limit <= 0 ) {
			    // we have hit our load limit, so lets just chuck everything away
			    // until we hit the EndUndoes/EndRedoes magic line and then start
			    // actually processing again.
			    const char* terminators[] = { "EndUndoes", "EndRedoes", 0 };
			    SFDConsumeUntil( sfd, terminators );
			}
		    }
		}
	    }
	    getname(sfd,tok);
	    if ( !strmatch(tok,"Redoes") ) {
		undo = 0;
		limit = UndoRedoLimitToLoad;
		last = sc->layers[current_layer].redoes;
		while((undo = SFDGetUndo( sfd, sc, "RedoOperation", current_layer )))
		{
		    // push to back
		    if( last ) last->next = undo;
		    else       sc->layers[current_layer].redoes = undo;
		    last = undo;

		    if( limit != -1 ) {
			limit--;
			if( limit <= 0 ) {
			    // we have hit our load limit, so lets just chuck everything away
			    // until we hit the EndUndoes/EndRedoes magic line and then start
			    // actually processing again.
			    const char* terminators[] = { "EndUndoes", "EndRedoes", 0 };
			    SFDConsumeUntil( sfd, terminators );
			}
		    }
		}
	    }
	} else if ( strmatch(tok,"SplineSet")==0 ) {
	    sc->layers[current_layer].splines = SFDGetSplineSet(sfd,sc->layers[current_layer].order2);
	} else if ( strmatch(tok,"Ref:")==0 || strmatch(tok,"Refer:")==0 ) {
	    /* I should be depending on the version number here, but I made */
	    /*  a mistake and bumped the version too late. So the version is */
	    /*  not an accurate mark, but the presence of a LayerCount keyword*/
	    /*  in the font is an good mark. Before the LayerCount was added */
	    /*  (version 2) only the foreground layer could have references */
	    /*  after that (eventually version 3) any layer could. */
	    if ( oldback || !had_sf_layer_cnt ) current_layer = ly_fore;
	    ref = SFDGetRef(sfd,strmatch(tok,"Ref:")==0);
	    if ( sc->layers[current_layer].refs==NULL )
		sc->layers[current_layer].refs = ref;
	    else
		lastr->next = ref;
	    lastr = ref;
	} else if ( strmatch(tok,"Image:")==0 ) {
	    int ly = current_layer;
	    if ( !multilayer && !sc->layers[ly].background ) ly = ly_back;
	    img = SFDGetImage(sfd);
	    if ( sc->layers[ly].images==NULL )
		sc->layers[ly].images = img;
	    else
		lasti->next = img;
	    lasti = img;
	} else if ( strmatch(tok,"PickledData:")==0 ) {
	    if (current_layer < sc->layer_cnt) {
	      sc->layers[current_layer].python_persistent = SFDUnPickle(sfd, 0);
	      sc->layers[current_layer].python_persistent_has_lists = 0;
	    }
	} else if ( strmatch(tok,"PickledDataWithLists:")==0 ) {
	    if (current_layer < sc->layer_cnt) {
	      sc->layers[current_layer].python_persistent = SFDUnPickle(sfd, 1);
	      sc->layers[current_layer].python_persistent_has_lists = 1;
	    }
	} else if ( strmatch(tok,"OrigType1:")==0 ) {	/* Accept, slurp, ignore contents */
	    SFDGetType1(sfd);
	} else if ( strmatch(tok,"TtfInstrs:")==0 ) {	/* Binary format */
	    SFDGetTtfInstrs(sfd,sc);
	} else if ( strmatch(tok,"TtInstrs:")==0 ) {	/* ASCII format */
	    SFDGetTtInstrs(sfd,sc);
	} else if ( strmatch(tok,"Kerns2:")==0 ||
		strmatch(tok,"VKerns2:")==0 ) {
	    KernPair *kp, *last=NULL;
	    int isv = *tok=='V';
	    int off, index;
	    struct lookup_subtable *sub;

	    if ( sf->sfd_version<2 )
		LogError(_("Found an new style kerning pair inside a version 1 (or lower) sfd file.\n") );
	    while ( fscanf(sfd,"%d %d", &index, &off )==2 ) {
		sub = SFFindLookupSubtableAndFreeName(sf,SFDReadUTF7Str(sfd));
		if ( sub==NULL ) {
		    LogError(_("KernPair with no subtable name.\n"));
	    break;
		}
		kp = chunkalloc(sizeof(KernPair1));
		kp->sc = (SplineChar *) (intpt) index;
		kp->kcid = true;
		kp->off = off;
		kp->subtable = sub;
		kp->next = NULL;
		while ( (ch=nlgetc(sfd))==' ' );
		ungetc(ch,sfd);
		if ( ch=='{' ) {
		    kp->adjust = SFDReadDeviceTable(sfd, NULL);
		}
		if ( last != NULL )
		    last->next = kp;
		else if ( isv )
		    sc->vkerns = kp;
		else
		    sc->kerns = kp;
		last = kp;
	    }
	} else if ( strmatch(tok,"Kerns:")==0 ||
		strmatch(tok,"KernsSLI:")==0 ||
		strmatch(tok,"KernsSLIF:")==0 ||
		strmatch(tok,"VKernsSLIF:")==0 ||
		strmatch(tok,"KernsSLIFO:")==0 ||
		strmatch(tok,"VKernsSLIFO:")==0 ) {
	    KernPair1 *kp, *last=NULL;
	    int index, off, sli, flags=0;
	    int hassli = (strmatch(tok,"KernsSLI:")==0);
	    int isv = *tok=='V';
	    int has_orig = strstr(tok,"SLIFO:")!=NULL;
	    if ( sf->sfd_version>=2 ) {
		IError( "Found an old style kerning pair inside a version 2 (or higher) sfd file." );
exit(1);
	    }
	    if ( strmatch(tok,"KernsSLIF:")==0 || strmatch(tok,"KernsSLIFO:")==0 ||
		    strmatch(tok,"VKernsSLIF:")==0 || strmatch(tok,"VKernsSLIFO:")==0 )
		hassli=2;
	    while ( (hassli==1 && fscanf(sfd,"%d %d %d", &index, &off, &sli )==3) ||
		    (hassli==2 && fscanf(sfd,"%d %d %d %d", &index, &off, &sli, &flags )==4) ||
		    (hassli==0 && fscanf(sfd,"%d %d", &index, &off )==2) ) {
		if ( !hassli )
		    sli = SFFindBiggestScriptLangIndex(sli_sf,
			    script!=0?script:SCScriptFromUnicode(sc),DEFAULT_LANG);
		if ( sli>=((SplineFont1 *) sli_sf)->sli_cnt && sli!=SLI_NESTED) {
		    static int complained=false;
		    if ( !complained )
			IError("'%s' in %s has a script index out of bounds: %d",
				isv ? "vkrn" : "kern",
				sc->name, sli );
		    sli = SFFindBiggestScriptLangIndex(sli_sf,
			    SCScriptFromUnicode(sc),DEFAULT_LANG);
		    complained = true;
		}
		kp = chunkalloc(sizeof(KernPair1));
		kp->kp.sc = (SplineChar *) (intpt) index;
		kp->kp.kcid = has_orig;
		kp->kp.off = off;
		kp->sli = sli;
		kp->flags = flags;
		kp->kp.next = NULL;
		while ( (ch=nlgetc(sfd))==' ' );
		ungetc(ch,sfd);
		if ( ch=='{' ) {
		    kp->kp.adjust = SFDReadDeviceTable(sfd, NULL);
		}
		if ( last != NULL )
		    last->kp.next = (KernPair *) kp;
		else if ( isv )
		    sc->vkerns = (KernPair *) kp;
		else
		    sc->kerns = (KernPair *) kp;
		last = kp;
	    }
	} else if ( (ispos = (strmatch(tok,"Position:")==0)) ||
		( ispos  = (strmatch(tok,"Position2:")==0)) ||
		( ispair = (strmatch(tok,"PairPos:")==0)) ||
		( ispair = (strmatch(tok,"PairPos2:")==0)) ||
		( islcar = (strmatch(tok,"LCarets:")==0)) ||
		( islcar = (strmatch(tok,"LCarets2:")==0)) ||
		( isliga = (strmatch(tok,"Ligature:")==0)) ||
		( isliga = (strmatch(tok,"Ligature2:")==0)) ||
		( issubs = (strmatch(tok,"Substitution:")==0)) ||
		( issubs = (strmatch(tok,"Substitution2:")==0)) ||
		( ismult = (strmatch(tok,"MultipleSubs:")==0)) ||
		( ismult = (strmatch(tok,"MultipleSubs2:")==0)) ||
		strmatch(tok,"AlternateSubs:")==0 ||
		strmatch(tok,"AlternateSubs2:")==0 ) {
	    PST *pst;
	    int old, type;
	    type = ispos ? pst_position :
			 ispair ? pst_pair :
			 islcar ? pst_lcaret :
			 isliga ? pst_ligature :
			 issubs ? pst_substitution :
			 ismult ? pst_multiple :
			 pst_alternate;
	    if ( strchr(tok,'2')!=NULL ) {
		old = false;
		pst = chunkalloc(sizeof(PST));
		if ( type!=pst_lcaret )
		    pst->subtable = SFFindLookupSubtableAndFreeName(sf,SFDReadUTF7Str(sfd));
	    } else {
		old = true;
		pst = chunkalloc(sizeof(PST1));
		((PST1 *) pst)->tag = CHR('l','i','g','a');
		((PST1 *) pst)->script_lang_index = 0xffff;
		while ( (ch=nlgetc(sfd))==' ' || ch=='\t' );
		if ( isdigit(ch)) {
		    int temp;
		    ungetc(ch,sfd);
		    getint(sfd,&temp);
		    ((PST1 *) pst)->flags = temp;
		    while ( (ch=nlgetc(sfd))==' ' || ch=='\t' );
		} else
		    ((PST1 *) pst)->flags = 0 /*PSTDefaultFlags(type,sc)*/;
		if ( isdigit(ch)) {
		    ungetc(ch,sfd);
		    getusint(sfd,&((PST1 *) pst)->script_lang_index);
		    while ( (ch=nlgetc(sfd))==' ' || ch=='\t' );
		} else
		    ((PST1 *) pst)->script_lang_index = SFFindBiggestScriptLangIndex(sf,
			    script!=0?script:SCScriptFromUnicode(sc),DEFAULT_LANG);
		if ( ch=='\'' ) {
		    ungetc(ch,sfd);
		    ((PST1 *) pst)->tag = gettag(sfd);
		} else if ( ch=='<' ) {
		    getint(sfd,&temp);
		    ((PST1 *) pst)->tag = temp<<16;
		    nlgetc(sfd);	/* comma */
		    getint(sfd,&temp);
		    ((PST1 *) pst)->tag |= temp;
		    nlgetc(sfd);	/* close '>' */
		    ((PST1 *) pst)->macfeature = true;
		} else
		    ungetc(ch,sfd);
		if ( type==pst_lcaret ) {
		/* These are meaningless for lcarets, set them to innocuous values */
		    ((PST1 *) pst)->script_lang_index = SLI_UNKNOWN;
		    ((PST1 *) pst)->tag = CHR(' ',' ',' ',' ');
		} else if ( ((PST1 *) pst)->script_lang_index>=((SplineFont1 *) sli_sf)->sli_cnt && ((PST1 *) pst)->script_lang_index!=SLI_NESTED ) {
		    static int complained=false;
		    if ( !complained )
			IError("'%c%c%c%c' in %s has a script index out of bounds: %d",
				(((PST1 *) pst)->tag>>24), (((PST1 *) pst)->tag>>16)&0xff, (((PST1 *) pst)->tag>>8)&0xff, ((PST1 *) pst)->tag&0xff,
				sc->name, ((PST1 *) pst)->script_lang_index );
		    else
			IError( "'%c%c%c%c' in %s has a script index out of bounds: %d\n",
				(((PST1 *) pst)->tag>>24), (((PST1 *) pst)->tag>>16)&0xff, (((PST1 *) pst)->tag>>8)&0xff, ((PST1 *) pst)->tag&0xff,
				sc->name, ((PST1 *) pst)->script_lang_index );
		    ((PST1 *) pst)->script_lang_index = SFFindBiggestScriptLangIndex(sli_sf,
			    SCScriptFromUnicode(sc),DEFAULT_LANG);
		    complained = true;
		}
	    }
	    if ( (sf->sfd_version<2)!=old ) {
		IError( "Version mixup in PST of sfd file." );
exit(1);
	    }
	    if ( last==NULL )
		sc->possub = pst;
	    else
		last->next = pst;
	    last = pst;
	    pst->type = type;
	    if ( pst->type==pst_position ) {
		fscanf( sfd, " dx=%hd dy=%hd dh=%hd dv=%hd",
			&pst->u.pos.xoff, &pst->u.pos.yoff,
			&pst->u.pos.h_adv_off, &pst->u.pos.v_adv_off);
		pst->u.pos.adjust = SFDReadValDevTab(sfd);
		ch = nlgetc(sfd);		/* Eat new line */
	    } else if ( pst->type==pst_pair ) {
		getname(sfd,tok);
		pst->u.pair.paired = copy(tok);
		pst->u.pair.vr = chunkalloc(sizeof(struct vr [2]));
		fscanf( sfd, " dx=%hd dy=%hd dh=%hd dv=%hd",
			&pst->u.pair.vr[0].xoff, &pst->u.pair.vr[0].yoff,
			&pst->u.pair.vr[0].h_adv_off, &pst->u.pair.vr[0].v_adv_off);
		pst->u.pair.vr[0].adjust = SFDReadValDevTab(sfd);
		fscanf( sfd, " dx=%hd dy=%hd dh=%hd dv=%hd",
			&pst->u.pair.vr[1].xoff, &pst->u.pair.vr[1].yoff,
			&pst->u.pair.vr[1].h_adv_off, &pst->u.pair.vr[1].v_adv_off);
		pst->u.pair.vr[0].adjust = SFDReadValDevTab(sfd);
		ch = nlgetc(sfd);
	    } else if ( pst->type==pst_lcaret ) {
		int i;
		fscanf( sfd, " %d", &pst->u.lcaret.cnt );
		pst->u.lcaret.carets = malloc(pst->u.lcaret.cnt*sizeof(int16));
		for ( i=0; i<pst->u.lcaret.cnt; ++i )
		    fscanf( sfd, " %hd", &pst->u.lcaret.carets[i]);
		geteol(sfd,tok);
	    } else {
		geteol(sfd,tok);
		pst->u.lig.components = copy(tok);	/* it's in the same place for all formats */
		if ( isliga ) {
		    pst->u.lig.lig = sc;
		    if ( old )
			last = (PST *) LigaCreateFromOldStyleMultiple((PST1 *) pst);
		}
	    }
#ifdef FONTFORGE_CONFIG_CVT_OLD_MAC_FEATURES
	    if ( old )
		CvtOldMacFeature((PST1 *) pst);
#endif
	} else if ( strmatch(tok,"Colour:")==0 ) {
	    uint32 temp;
	    gethex(sfd,&temp);
	    sc->color = temp;
	} else if ( strmatch(tok,"Comment:")==0 ) {
	    sc->comment = SFDReadUTF7Str(sfd);
	} else if ( strmatch(tok,"TileMargin:")==0 ) {
	    getreal(sfd,&sc->tile_margin);
	} else if ( strmatch(tok,"TileBounds:")==0 ) {
	    getreal(sfd,&sc->tile_bounds.minx);
	    getreal(sfd,&sc->tile_bounds.miny);
	    getreal(sfd,&sc->tile_bounds.maxx);
	    getreal(sfd,&sc->tile_bounds.maxy);
	} else if ( strmatch(tok,"EndChar")==0 ) {
	    if ( sc->orig_pos<sf->glyphcnt )
		sf->glyphs[sc->orig_pos] = sc;
            /* Recalculating hint active zones may be needed for old .sfd files. */
            /* Do this when we have finished with other glyph components, */
            /* so that splines are already available */
	    if ( sf->sfd_version<2 )
                SCGuessHintInstancesList( sc,ly_fore,sc->hstem,sc->vstem,sc->dstem,false,false );
            else if ( had_old_dstems && sc->layers[ly_fore].splines != NULL )
                SCGuessHintInstancesList( sc,ly_fore,NULL,NULL,sc->dstem,false,true );
	    if ( sc->layers[ly_fore].order2 )
		SCDefaultInterpolation(sc);
return( sc );
	} else {
	    geteol(sfd,tok);
	}
    }
}

static int SFDGetBitmapProps(FILE *sfd,BDFFont *bdf,char *tok) {
    int pcnt;
    int i;

    if ( getint(sfd,&pcnt)!=1 || pcnt<=0 )
return( 0 );
    bdf->prop_cnt = pcnt;
    bdf->props = malloc(pcnt*sizeof(BDFProperties));
    for ( i=0; i<pcnt; ++i ) {
	if ( getname(sfd,tok)!=1 )
    break;
	if ( strcmp(tok,"BDFEndProperties")==0 )
    break;
	bdf->props[i].name = copy(tok);
	getint(sfd,&bdf->props[i].type);
	switch ( bdf->props[i].type&~prt_property ) {
	  case prt_int: case prt_uint:
	    getint(sfd,&bdf->props[i].u.val);
	  break;
	  case prt_string: case prt_atom:
	    geteol(sfd,tok);
	    if ( tok[strlen(tok)-1]=='"' ) tok[strlen(tok)-1] = '\0';
	    bdf->props[i].u.str = copy(tok[0]=='"'?tok+1:tok);
	  break;
	  default:
	  break;
	}
    }
    bdf->prop_cnt = i;
return( 1 );
}

static int SFDGetBitmapChar(FILE *sfd,BDFFont *bdf) {
    BDFChar *bfc;
    struct enc85 dec;
    int i, enc, orig;
    int width,xmax,xmin,ymax,ymin, vwidth=-1;
    EncMap *map;
    int ch;

    map = bdf->sf->map;

    if ( getint(sfd,&orig)!=1 || orig<0 )
return( 0 );
    if ( getint(sfd,&enc)!=1 )
return( 0 );
    if ( getint(sfd,&width)!=1 )
return( 0 );
    if ( getint(sfd,&xmin)!=1 )
return( 0 );
    if ( getint(sfd,&xmax)!=1 )
return( 0 );
    if ( getint(sfd,&ymin)!=1 )
return( 0 );
    while ( (ch=nlgetc(sfd))==' ');
    ungetc(ch,sfd);
    if ( ch=='\n' || ch=='\r' || getint(sfd,&ymax)!=1 ) {
	/* Old style format, no orig_pos given, shift everything by 1 */
	ymax = ymin;
	ymin = xmax;
	xmax = xmin;
	xmin = width;
	width = enc;
	enc = orig;
	orig = map->map[enc];
    } else {
	while ( (ch=nlgetc(sfd))==' ');
	ungetc(ch,sfd);
	if ( ch!='\n' && ch!='\r' )
	    getint(sfd,&vwidth);
    }
    if ( enc<0 ||xmax<xmin || ymax<ymin )
return( 0 );

    bfc = chunkalloc(sizeof(BDFChar));
    if (bfc == NULL)
        return 0;

    if ( orig==-1 ) {
	bfc->sc = SFMakeChar(bdf->sf,map,enc);
	orig = bfc->sc->orig_pos;
    }

    bfc->orig_pos = orig;
    bfc->width = width;
    bfc->ymax = ymax; bfc->ymin = ymin;
    bfc->xmax = xmax; bfc->xmin = xmin;
    bdf->glyphs[orig] = bfc;
    bfc->sc = bdf->sf->glyphs[orig];
    bfc->vwidth = vwidth!=-1 ? vwidth :
	    rint(bfc->sc->vwidth*bdf->pixelsize / (real) (bdf->sf->ascent+bdf->sf->descent));
    if ( bdf->clut==NULL ) {
	bfc->bytes_per_line = (bfc->xmax-bfc->xmin)/8 +1;
	bfc->depth = 1;
    } else {
	bfc->bytes_per_line = bfc->xmax-bfc->xmin +1;
	bfc->byte_data = true;
	bfc->depth = bdf->clut->clut_len==4 ? 2 : bdf->clut->clut_len==16 ? 4 : 8;
    }
    bfc->bitmap = calloc((bfc->ymax-bfc->ymin+1)*bfc->bytes_per_line,sizeof(char));

    memset(&dec,'\0', sizeof(dec)); dec.pos = -1;
    dec.sfd = sfd;
    for ( i=0; i<=bfc->ymax-bfc->ymin; ++i ) {
	uint8 *pt = (uint8 *) (bfc->bitmap + i*bfc->bytes_per_line);
	uint8 *end = (uint8 *) (bfc->bitmap + (i+1)*bfc->bytes_per_line);
	while ( pt<end ) {
	    *pt++ = Dec85(&dec);
	}
    }
    if ( bfc->sc==NULL ) {
	bdf->glyphs[bfc->orig_pos] = NULL;
	BDFCharFree(bfc);
    }
/* This fixes a bug: We didn't set "widthset" on splinechars when reading in */
/*  winfonts. We should set it now on any bitmaps worth outputting to make up*/
/*  for that. Eventually we should have good sfd files and can remove this */
    else if ( bfc->sc->width!=bdf->sf->ascent + bdf->sf->descent )
	bfc->sc->widthset = true;
return( 1 );
}

static int SFDGetBitmapReference(FILE *sfd,BDFFont *bdf) {
    BDFChar *bc;
    BDFRefChar *ref, *head;
    int gid, rgid, xoff, yoff;
    char ch;

    /* 'BDFRefChar:' elements should not occur in the file before the corresponding */
    /* 'BDFChar:'. However it is possible that the glyphs they refer to are not yet */
    /* available. So we will find them later */
    if ( getint(sfd,&gid)!=1 || gid<=0 || gid >= bdf->glyphcnt || ( bc = bdf->glyphs[gid] ) == NULL )
return( 0 );
    if ( getint(sfd,&rgid)!=1 || rgid<0 )
return( 0 );
    if ( getint(sfd,&xoff)!=1 )
return( 0 );
    if ( getint(sfd,&yoff)!=1 )
return( 0 );
    while ( isspace( ch=nlgetc( sfd )) && ch!='\r' && ch!='\n' );

    ref = calloc( 1,sizeof( BDFRefChar ));
    ref->gid = rgid; ref->xoff = xoff, ref->yoff = yoff;
    if ( ch == 'S' ) ref->selected = true;
    for ( head = bc->refs; head != NULL && head->next!=NULL; head = head->next );
    if ( head == NULL ) bc->refs = ref;
    else head->next = ref;
return( 1 );
}

static void SFDFixupBitmapRefs( BDFFont *bdf ) {
    BDFChar *bc, *rbc;
    BDFRefChar *head, *next, *prev;
    int i;

    for ( i=0; i<bdf->glyphcnt; i++ ) if (( bc = bdf->glyphs[i] ) != NULL ) {
	prev = NULL;
	for ( head = bc->refs; head != NULL; head = next ) {
	    next = head->next;
	    if (( rbc = bdf->glyphs[head->gid] ) != NULL ) {
		head->bdfc = rbc;
		BCMakeDependent( bc,rbc );
		prev = head;
	    } else {
		LogError(_("Glyph %d in bitmap strike %d pixels refers to a missing glyph (%d)"),
		    bc->orig_pos, bdf->pixelsize, head->gid );
		if ( prev == NULL ) bc->refs = next;
		else prev->next = next;
	    }
	}
    }

}

static int SFDGetBitmapFont(FILE *sfd,SplineFont *sf,int fromdir,char *dirname) {
    BDFFont *bdf, *prev;
    char tok[200];
    int pixelsize, ascent, descent, depth=1;
    int ch, enccount;

    if ( getint(sfd,&pixelsize)!=1 || pixelsize<=0 )
return( 0 );
    if ( getint(sfd,&enccount)!=1 || enccount<0 )
return( 0 );
    if ( getint(sfd,&ascent)!=1 || ascent<0 )
return( 0 );
    if ( getint(sfd,&descent)!=1 || descent<0 )
return( 0 );
    if ( getint(sfd,&depth)!=1 )
	depth = 1;	/* old sfds don't have a depth here */
    else if ( depth!=1 && depth!=2 && depth!=4 && depth!=8 )
return( 0 );
    while ( (ch = nlgetc(sfd))==' ' );
    ungetc(ch,sfd);		/* old sfds don't have a foundry */

    bdf = calloc(1,sizeof(BDFFont));
    if (bdf == NULL)
        return 0;

    if ( ch!='\n' && ch!='\r' ) {
	getname(sfd,tok);
	bdf->foundry = copy(tok);
    }
    bdf->pixelsize = pixelsize;
    bdf->ascent = ascent;
    bdf->descent = descent;
    if ( depth!=1 )
	BDFClut(bdf,(1<<(depth/2)));

    if ( sf->bitmaps==NULL )
	sf->bitmaps = bdf;
    else {
	for ( prev=sf->bitmaps; prev->next!=NULL; prev=prev->next );
	prev->next = bdf;
    }
    bdf->sf = sf;
    bdf->glyphcnt = bdf->glyphmax = sf->glyphcnt;
    bdf->glyphs = calloc(bdf->glyphcnt,sizeof(BDFChar *));

    while ( getname(sfd,tok)==1 ) {
	if ( strcmp(tok,"BDFStartProperties:")==0 )
	    SFDGetBitmapProps(sfd,bdf,tok);
	else if ( strcmp(tok,"BDFEndProperties")==0 )
	    /* Do Nothing */;
	else if ( strcmp(tok,"Resolution:")==0 )
	    getint(sfd,&bdf->res);
	else if ( strcmp(tok,"BDFChar:")==0 )
	    SFDGetBitmapChar(sfd,bdf);
	else if ( strcmp(tok,"BDFRefChar:")==0 )
	    SFDGetBitmapReference(sfd,bdf);
	else if ( strcmp(tok,"EndBitmapFont")==0 )
    break;
    }
    if ( fromdir ) {
	DIR *dir;
	struct dirent *ent;
	char *name;

	dir = opendir(dirname);
	if ( dir==NULL )
return( 0 );
	name = malloc(strlen(dirname)+NAME_MAX+3);

	while ( (ent=readdir(dir))!=NULL ) {
	    char *pt = strrchr(ent->d_name,EXT_CHAR);
	    if ( pt==NULL )
		/* Nothing interesting */;
	    else if ( strcmp(pt,BITMAP_EXT)==0 ) {
		FILE *gsfd;
		sprintf(name,"%s/%s", dirname, ent->d_name);
		gsfd = fopen(name,"r");
		if ( gsfd!=NULL ) {
		    if ( getname(gsfd,tok) && strcmp(tok,"BDFChar:")==0)
			SFDGetBitmapChar(gsfd,bdf);
		    fclose(gsfd);
		    ff_progress_next();
		}
	    }
	}
	free(name);
	closedir(dir);
    }
    SFDFixupBitmapRefs( bdf );
return( 1 );
}

static void SFDFixupRef(SplineChar *sc,RefChar *ref,int layer) {
    RefChar *rf;
    int ly;

    if ( sc->parent->multilayer ) {
	for ( ly=ly_fore; ly<ref->sc->layer_cnt; ++ly ) {
	    for ( rf = ref->sc->layers[ly].refs; rf!=NULL; rf=rf->next ) {
		if ( rf->sc==sc ) {	/* Huh? */
		    ref->sc->layers[ly].refs = NULL;
	    break;
		}
		if ( rf->layers[0].splines==NULL )
		    SFDFixupRef(ref->sc,rf,layer);
	    }
	}
    } else {
	for ( rf = ref->sc->layers[layer].refs; rf!=NULL; rf=rf->next ) {
	    if ( rf->sc==sc ) {	/* Huh? */
		ref->sc->layers[layer].refs = NULL;
	break;
	    }
	    if ( rf->layers[0].splines==NULL )
		SFDFixupRef(ref->sc,rf,layer);
	}
    }
    SCReinstanciateRefChar(sc,ref,layer);
    SCMakeDependent(sc,ref->sc);
}

/* Look for character duplicates, such as might be generated by having the same */
/*  glyph at two encoding slots */
/* This is an obsolete convention, supported now only in sfd files */
/* I think it is ok if something depends on this character, because the */
/*  code that handles references will automatically unwrap it down to be base */
static SplineChar *SCDuplicate(SplineChar *sc) {
    SplineChar *matched = sc;

    if ( sc==NULL || sc->parent==NULL || sc->parent->cidmaster!=NULL )
return( sc );		/* Can't do this in CID keyed fonts */

    if ( sc->layer_cnt!=2 )
return( sc );

    while ( sc->layers[ly_fore].refs!=NULL &&
	    sc->layers[ly_fore].refs->sc!=NULL &&	/* Can happen if we are called during font loading before references are fixed up */
	    sc->layers[ly_fore].refs->next==NULL &&
	    sc->layers[ly_fore].refs->transform[0]==1 && sc->layers[ly_fore].refs->transform[1]==0 &&
	    sc->layers[ly_fore].refs->transform[2]==0 && sc->layers[ly_fore].refs->transform[3]==1 &&
	    sc->layers[ly_fore].refs->transform[4]==0 && sc->layers[ly_fore].refs->transform[5]==0 ) {
	char *basename = sc->layers[ly_fore].refs->sc->name;
	if ( strcmp(sc->name,basename)!=0 )
    break;
	matched = sc->layers[ly_fore].refs->sc;
	sc = sc->layers[ly_fore].refs->sc;
    }
return( matched );
}

static void SFDFixupUndoRefsUndoList(SplineFont *sf,Undoes *undo) {
    for( ; undo; undo = undo->next ) {
	if( undo->undotype == ut_state && undo->u.state.refs ) {
	    RefChar *ref=NULL;
	    for ( ref=undo->u.state.refs; ref!=NULL; ref=ref->next ) {

		ref->sc = sf->glyphs[ ref->orig_pos ];
	    }
	}
    }
}

static void SFDFixupUndoRefs(SplineFont *sf) {
    int i=0;
    Undoes *undo = 0;

    for ( i=0; i<sf->glyphcnt; ++i ) {
	if ( sf->glyphs[i]!=NULL ) {
	    SplineChar *sc = sf->glyphs[i];
	    int layer = 0;
	    for ( layer=0; layer<sc->layer_cnt; ++layer ) {
		if( sc->layers[layer].undoes ) {
		    undo = sc->layers[layer].undoes;
		    SFDFixupUndoRefsUndoList( sf, undo );
		}
		if( sc->layers[layer].redoes ) {
		    undo = sc->layers[layer].redoes;
		    SFDFixupUndoRefsUndoList( sf, undo );
		}
	    }
	}
    }


}


void SFDFixupRefs(SplineFont *sf) {
    int i, isv;
    RefChar *refs, *rnext, *rprev;
    /*int isautorecovery = sf->changed;*/
    KernPair *kp, *prev, *next;
    EncMap *map = sf->map;
    int layer;
    int k,l;
    SplineFont *cidmaster = sf, *ksf;

    k = 1;
    if ( sf->subfontcnt!=0 )
	sf = sf->subfonts[0];

    ff_progress_change_line2(_("Interpreting Glyphs"));
    for (;;) {
	for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	    SplineChar *sc = sf->glyphs[i];
	    /* A changed character is one that has just been recovered */
	    /*  unchanged characters will already have been fixed up */
	    /* Er... maybe not. If the character being recovered is refered to */
	    /*  by another character then we need to fix up that other char too*/
	    /*if ( isautorecovery && !sc->changed )*/
	/*continue;*/
	    for ( layer = 0; layer<sc->layer_cnt; ++layer ) {
		rprev = NULL;
		for ( refs = sc->layers[layer].refs; refs!=NULL; refs=rnext ) {
		    rnext = refs->next;
		    if ( refs->encoded ) {		/* Old sfd format */
			if ( refs->orig_pos<map->encmax && map->map[refs->orig_pos]!=-1 )
			    refs->orig_pos = map->map[refs->orig_pos];
			else
			    refs->orig_pos = sf->glyphcnt;
			refs->encoded = false;
		    }
		    if ( refs->orig_pos<sf->glyphcnt && refs->orig_pos>=0 )
			refs->sc = sf->glyphs[refs->orig_pos];
		    if ( refs->sc!=NULL ) {
			refs->unicode_enc = refs->sc->unicodeenc;
			refs->adobe_enc = getAdobeEnc(refs->sc->name);
			rprev = refs;
			if ( refs->use_my_metrics ) {
			    if ( sc->width != refs->sc->width ) {
				LogError(_("Bad sfd file. Glyph %s has width %d even though it should be\n  bound to the width of %s which is %d.\n"),
					sc->name, sc->width, refs->sc->name, refs->sc->width );
				sc->width = refs->sc->width;
			    }
			}
		    } else {
			RefCharFree(refs);
			if ( rprev!=NULL )
			    rprev->next = rnext;
			else
			    sc->layers[layer].refs = rnext;
		    }
		}
	    }
	    /* In old sfd files we used a peculiar idiom to represent a multiply */
	    /*  encoded glyph. Fix it up now. Remove the fake glyph and adjust the*/
	    /*  map */
	    /*if ( isautorecovery && !sc->changed )*/
	/*continue;*/
	    for ( isv=0; isv<2; ++isv ) {
		for ( prev = NULL, kp=isv?sc->vkerns : sc->kerns; kp!=NULL; kp=next ) {
		    int index = (intpt) (kp->sc);

		    next = kp->next;
		    // be impotent if the reference is already to the correct location
                    if ( !kp->kcid ) {	/* It's encoded (old sfds), else orig */
                        if ( index>=map->encmax || map->map[index]==-1 )
                            index = sf->glyphcnt;
                        else
                            index = map->map[index];
                    }
                    kp->kcid = false;
                    ksf = sf;
                    if ( cidmaster!=sf ) {
                        for ( l=0; l<cidmaster->subfontcnt; ++l ) {
                            ksf = cidmaster->subfonts[l];
                            if ( index<ksf->glyphcnt && ksf->glyphs[index]!=NULL )
                                break;
                        }
                    }
                    if ( index>=ksf->glyphcnt || ksf->glyphs[index]==NULL ) {
                        IError( "Bad kerning information in glyph %s\n", sc->name );
                        kp->sc = NULL;
                    } else {
                        kp->sc = ksf->glyphs[index];
                    }

		    if ( kp->sc!=NULL )
			prev = kp;
		    else{
			if ( prev!=NULL )
			    prev->next = next;
			else if ( isv )
			    sc->vkerns = next;
			else
			    sc->kerns = next;
			chunkfree(kp,sizeof(KernPair));
		    }
		}
	    }
	    if ( SCDuplicate(sc)!=sc ) {
		SplineChar *base = SCDuplicate(sc);
		int orig = sc->orig_pos, enc = sf->map->backmap[orig], uni = sc->unicodeenc;
		SplineCharFree(sc);
		sf->glyphs[i]=NULL;
		sf->map->backmap[orig] = -1;
		sf->map->map[enc] = base->orig_pos;
		AltUniAdd(base,uni);
	    }
	}
	for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	    SplineChar *sc = sf->glyphs[i];
	    for ( layer=0; layer<sc->layer_cnt; ++layer ) {
		for ( refs = sf->glyphs[i]->layers[layer].refs; refs!=NULL; refs=refs->next ) {
		    SFDFixupRef(sf->glyphs[i],refs,layer);
		}
	    }
	    ff_progress_next();
	}
	if ( sf->cidmaster==NULL )
	    for ( i=sf->glyphcnt-1; i>=0 && sf->glyphs[i]==NULL; --i )
		sf->glyphcnt = i;
	if ( k>=cidmaster->subfontcnt )
    break;
	sf = cidmaster->subfonts[k++];
    }
}

/* When we recover from an autosaved file we must be careful. If that file */
/*  contains a character that is refered to by another character then the */
/*  dependent list will contain a dead pointer without this routine. Similarly*/
/*  for kerning */
/* We might have needed to do something for references except they've already */
/*  got a orig_pos field and passing through SFDFixupRefs will munch their*/
/*  SplineChar pointer */
static void SFRemoveDependencies(SplineFont *sf) {
    int i;
    struct splinecharlist *dlist, *dnext;
    KernPair *kp;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	for ( dlist = sf->glyphs[i]->dependents; dlist!=NULL; dlist = dnext ) {
	    dnext = dlist->next;
	    chunkfree(dlist,sizeof(*dlist));
	}
	sf->glyphs[i]->dependents = NULL;
	for ( kp=sf->glyphs[i]->kerns; kp!=NULL; kp=kp->next ) {
	    kp->sc = (SplineChar *) (intpt) (kp->sc->orig_pos);
	    kp->kcid = true;		/* flag */
	}
	for ( kp=sf->glyphs[i]->vkerns; kp!=NULL; kp=kp->next ) {
	    kp->sc = (SplineChar *) (intpt) (kp->sc->orig_pos);
	    kp->kcid = true;
	}
    }
}

static void SFDGetPrivate(FILE *sfd,SplineFont *sf) {
    int i, cnt, len;
    char name[200];
    char *pt, *end;

    sf->private = calloc(1,sizeof(struct psdict));
    getint(sfd,&cnt);
    sf->private->next = sf->private->cnt = cnt;
    sf->private->values = calloc(cnt,sizeof(char *));
    sf->private->keys = calloc(cnt,sizeof(char *));
    for ( i=0; i<cnt; ++i ) {
	getname(sfd,name);
	sf->private->keys[i] = copy(name);
	getint(sfd,&len);
	nlgetc(sfd);	/* skip space */
	pt = sf->private->values[i] = malloc(len+1);
	for ( end = pt+len; pt<end; ++pt )
	    *pt = nlgetc(sfd);
	*pt='\0';
    }
}

static void SFDGetSubrs(FILE *sfd) {
    /* Obselete, parse it in case there are any old sfds */
    int i, cnt, tot, len;
    struct enc85 dec;

    getint(sfd,&cnt);
    tot = 0;
    for ( i=0; i<cnt; ++i ) {
	getint(sfd,&len);
	tot += len;
    }

    memset(&dec,'\0', sizeof(dec)); dec.pos = -1;
    dec.sfd = sfd;
    for ( i=0; i<tot; ++i )
	Dec85(&dec);
}

static struct ttflangname *SFDGetLangName(FILE *sfd,struct ttflangname *old) {
    struct ttflangname *cur = chunkalloc(sizeof(struct ttflangname)), *prev;
    int i;

    getint(sfd,&cur->lang);
    for ( i=0; i<ttf_namemax; ++i )
	cur->names[i] = SFDReadUTF7Str(sfd);
    if ( old==NULL )
return( cur );
    for ( prev = old; prev->next !=NULL; prev = prev->next );
    prev->next = cur;
return( old );
}

static void SFDGetGasp(FILE *sfd,SplineFont *sf) {
    int i;

    getsint(sfd,(int16 *) &sf->gasp_cnt);
    sf->gasp = malloc(sf->gasp_cnt*sizeof(struct gasp));
    for ( i=0; i<sf->gasp_cnt; ++i ) {
	getsint(sfd,(int16 *) &sf->gasp[i].ppem);
	getsint(sfd,(int16 *) &sf->gasp[i].flags);
    }
    getsint(sfd,(int16 *) &sf->gasp_version);
}

static void SFDGetDesignSize(FILE *sfd,SplineFont *sf) {
    int ch;
    struct otfname *cur;

    getsint(sfd,(int16 *) &sf->design_size);
    while ( (ch=nlgetc(sfd))==' ' );
    ungetc(ch,sfd);
    if ( isdigit(ch)) {
	getsint(sfd,(int16 *) &sf->design_range_bottom);
	while ( (ch=nlgetc(sfd))==' ' );
	if ( ch!='-' )
	    ungetc(ch,sfd);
	getsint(sfd,(int16 *) &sf->design_range_top);
	getsint(sfd,(int16 *) &sf->fontstyle_id);
	for (;;) {
	    while ( (ch=nlgetc(sfd))==' ' );
	    ungetc(ch,sfd);
	    if ( !isdigit(ch))
	break;
	    cur = chunkalloc(sizeof(struct otfname));
	    cur->next = sf->fontstyle_name;
	    sf->fontstyle_name = cur;
	    getsint(sfd,(int16 *) &cur->lang);
	    cur->name = SFDReadUTF7Str(sfd);
	}
    }
}

static void SFDGetOtfFeatName(FILE *sfd,SplineFont *sf) {
    int ch;
    struct otfname *cur;
    struct otffeatname *fn;

    fn = chunkalloc(sizeof(struct otffeatname));
    fn->tag = gettag(sfd);
    for (;;) {
	while ( (ch=nlgetc(sfd))==' ' );
	ungetc(ch,sfd);
	if ( !isdigit(ch))
    break;
	cur = chunkalloc(sizeof(struct otfname));
	cur->next = fn->names;
	fn->names = cur;
	getsint(sfd,(int16 *) &cur->lang);
	cur->name = SFDReadUTF7Str(sfd);
    }
    fn->next = sf->feat_names;
    sf->feat_names = fn;
}

static Encoding *SFDGetEncoding(FILE *sfd, char *tok) {
    Encoding *enc = NULL;
    int encname;

    if ( getint(sfd,&encname) ) {
	if ( encname<(int)(sizeof(charset_names)/sizeof(charset_names[0])-1) )
	    enc = FindOrMakeEncoding(charset_names[encname]);
    } else {
	geteol(sfd,tok);
	enc = FindOrMakeEncoding(tok);
    }
    if ( enc==NULL )
	enc = &custom;
return( enc );
}

static enum uni_interp SFDGetUniInterp(FILE *sfd, char *tok, SplineFont *sf) {
    int uniinterp = ui_none;
    int i;

    geteol(sfd,tok);
    for ( i=0; unicode_interp_names[i]!=NULL; ++i )
	if ( strcmp(tok,unicode_interp_names[i])==0 ) {
	    uniinterp = i;
    break;
	}
    /* These values are now handled by namelists */
    if ( uniinterp == ui_adobe ) {
	sf->for_new_glyphs = NameListByName("AGL with PUA");
	uniinterp = ui_none;
    } else if ( uniinterp == ui_greek ) {
	sf->for_new_glyphs = NameListByName("Greek small caps");
	uniinterp = ui_none;
    } else if ( uniinterp == ui_ams ) {
	sf->for_new_glyphs = NameListByName("AMS Names");
	uniinterp = ui_none;
    }

return( uniinterp );
}

static void SFDGetNameList(FILE *sfd, char *tok, SplineFont *sf) {
    NameList *nl;

    geteol(sfd,tok);
    nl = NameListByName(tok);
    if ( nl==NULL )
	LogError(_("Failed to find NameList: %s"), tok);
    else
	sf->for_new_glyphs = nl;
}


static OTLookup *SFD_ParseNestedLookup(FILE *sfd, SplineFont *sf, int old) {
    uint32 tag;
    int ch, isgpos;
    OTLookup *otl;
    char *name;

    while ( (ch=nlgetc(sfd))==' ' );
    if ( ch=='~' )
return( NULL );
    else if ( old ) {
	if ( ch!='\'' )
return( NULL );

	ungetc(ch,sfd);
	tag = gettag(sfd);
return( (OTLookup *) (intpt) tag );
    } else {
	ungetc(ch,sfd);
	name = SFDReadUTF7Str(sfd);
	if ( name==NULL )
return( NULL );
	for ( isgpos=0; isgpos<2; ++isgpos ) {
	    for ( otl=isgpos ? sf->gpos_lookups : sf->gsub_lookups; otl!=NULL; otl=otl->next ) {
		if ( strcmp(name,otl->lookup_name )==0 )
	goto break2;
	    }
	}
	break2:
	free(name);
return( otl );
    }
}

static void SFDParseChainContext(FILE *sfd,SplineFont *sf,FPST *fpst, char *tok, int old) {
    int ch, i, j, k, temp;
    SplineFont *sli_sf = sf->cidmaster ? sf->cidmaster : sf;

    fpst->type = strnmatch(tok,"ContextPos",10)==0 ? pst_contextpos :
		strnmatch(tok,"ContextSub",10)==0 ? pst_contextsub :
		strnmatch(tok,"ChainPos",8)==0 ? pst_chainpos :
		strnmatch(tok,"ChainSub",8)==0 ? pst_chainsub : pst_reversesub;
    getname(sfd,tok);
    fpst->format = strmatch(tok,"glyph")==0 ? pst_glyphs :
		    strmatch(tok,"class")==0 ? pst_class :
		    strmatch(tok,"coverage")==0 ? pst_coverage : pst_reversecoverage;
    if ( old ) {
	fscanf(sfd, "%hu %hu", &((FPST1 *) fpst)->flags, &((FPST1 *) fpst)->script_lang_index );
	if ( ((FPST1 *) fpst)->script_lang_index>=((SplineFont1 *) sli_sf)->sli_cnt && ((FPST1 *) fpst)->script_lang_index!=SLI_NESTED ) {
	    static int complained=false;
	    if ( ((SplineFont1 *) sli_sf)->sli_cnt==0 )
		IError("'%c%c%c%c' has a script index out of bounds: %d\nYou MUST fix this manually",
			(((FPST1 *) fpst)->tag>>24), (((FPST1 *) fpst)->tag>>16)&0xff, (((FPST1 *) fpst)->tag>>8)&0xff, ((FPST1 *) fpst)->tag&0xff,
			((FPST1 *) fpst)->script_lang_index );
	    else if ( !complained )
		IError("'%c%c%c%c' has a script index out of bounds: %d",
			(((FPST1 *) fpst)->tag>>24), (((FPST1 *) fpst)->tag>>16)&0xff, (((FPST1 *) fpst)->tag>>8)&0xff, ((FPST1 *) fpst)->tag&0xff,
			((FPST1 *) fpst)->script_lang_index );
	    else
		IError("'%c%c%c%c' has a script index out of bounds: %d\n",
			(((FPST1 *) fpst)->tag>>24), (((FPST1 *) fpst)->tag>>16)&0xff, (((FPST1 *) fpst)->tag>>8)&0xff, ((FPST1 *) fpst)->tag&0xff,
			((FPST1 *) fpst)->script_lang_index );
	    if ( ((SplineFont1 *) sli_sf)->sli_cnt!=0 )
		((FPST1 *) fpst)->script_lang_index = ((SplineFont1 *) sli_sf)->sli_cnt-1;
	    complained = true;
	}
	while ( (ch=nlgetc(sfd))==' ' || ch=='\t' );
	if ( ch=='\'' ) {
	    ungetc(ch,sfd);
	    ((FPST1 *) fpst)->tag = gettag(sfd);
	} else
	    ungetc(ch,sfd);
    } else {
	fpst->subtable = SFFindLookupSubtableAndFreeName(sf,SFDReadUTF7Str(sfd));
        if ( !fpst->subtable )
            LogError(_("Missing Subtable definition found in chained context"));
        else
	    fpst->subtable->fpst = fpst;
    }
    fscanf(sfd, "%hu %hu %hu %hu", &fpst->nccnt, &fpst->bccnt, &fpst->fccnt, &fpst->rule_cnt );
    if ( fpst->nccnt!=0 || fpst->bccnt!=0 || fpst->fccnt!=0 ) {
	fpst->nclass = malloc(fpst->nccnt*sizeof(char *));
	fpst->nclassnames = calloc(fpst->nccnt,sizeof(char *));
	if ( fpst->nccnt!=0 ) fpst->nclass[0] = NULL;
	if ( fpst->bccnt!=0 || fpst->fccnt!=0 ) {
	    fpst->bclass = malloc(fpst->bccnt*sizeof(char *));
	    fpst->bclassnames = calloc(fpst->bccnt,sizeof(char *));
	    if (fpst->bccnt!=0 ) fpst->bclass[0] = NULL;
	    fpst->fclass = malloc(fpst->fccnt*sizeof(char *));
	    fpst->fclassnames = calloc(fpst->fccnt,sizeof(char *));
	    if (fpst->fccnt!=0 ) fpst->fclass[0] = NULL;
	}
    }

    for ( j=0; j<3; ++j ) {
	for ( i=1; i<(&fpst->nccnt)[j]; ++i ) {
	    getname(sfd,tok);
	    if ( i==1 && j==0 && strcmp(tok,"Class0:")==0 )
		i=0;
	    getint(sfd,&temp);
	    (&fpst->nclass)[j][i] = malloc(temp+1); (&fpst->nclass)[j][i][temp] = '\0';
	    nlgetc(sfd);	/* skip space */
	    fread((&fpst->nclass)[j][i],1,temp,sfd);
	}
    }

    fpst->rules = calloc(fpst->rule_cnt,sizeof(struct fpst_rule));
    for ( i=0; i<fpst->rule_cnt; ++i ) {
	switch ( fpst->format ) {
	  case pst_glyphs:
	    for ( j=0; j<3; ++j ) {
		getname(sfd,tok);
		getint(sfd,&temp);
		(&fpst->rules[i].u.glyph.names)[j] = malloc(temp+1);
		(&fpst->rules[i].u.glyph.names)[j][temp] = '\0';
		nlgetc(sfd);	/* skip space */
		fread((&fpst->rules[i].u.glyph.names)[j],1,temp,sfd);
	    }
	  break;
	  case pst_class:
	    fscanf( sfd, "%d %d %d", &fpst->rules[i].u.class.ncnt, &fpst->rules[i].u.class.bcnt, &fpst->rules[i].u.class.fcnt );
	    for ( j=0; j<3; ++j ) {
		getname(sfd,tok);
		(&fpst->rules[i].u.class.nclasses)[j] = malloc((&fpst->rules[i].u.class.ncnt)[j]*sizeof(uint16));
		for ( k=0; k<(&fpst->rules[i].u.class.ncnt)[j]; ++k ) {
		    getusint(sfd,&(&fpst->rules[i].u.class.nclasses)[j][k]);
		}
	    }
	  break;
	  case pst_coverage:
	  case pst_reversecoverage:
	    fscanf( sfd, "%d %d %d", &fpst->rules[i].u.coverage.ncnt, &fpst->rules[i].u.coverage.bcnt, &fpst->rules[i].u.coverage.fcnt );
	    for ( j=0; j<3; ++j ) {
		(&fpst->rules[i].u.coverage.ncovers)[j] = malloc((&fpst->rules[i].u.coverage.ncnt)[j]*sizeof(char *));
		for ( k=0; k<(&fpst->rules[i].u.coverage.ncnt)[j]; ++k ) {
		    getname(sfd,tok);
		    getint(sfd,&temp);
		    (&fpst->rules[i].u.coverage.ncovers)[j][k] = malloc(temp+1);
		    (&fpst->rules[i].u.coverage.ncovers)[j][k][temp] = '\0';
		    nlgetc(sfd);	/* skip space */
		    fread((&fpst->rules[i].u.coverage.ncovers)[j][k],1,temp,sfd);
		}
	    }
	  break;
	  default:
	  break;
	}
	switch ( fpst->format ) {
	  case pst_glyphs:
	  case pst_class:
	  case pst_coverage:
	    getint(sfd,&fpst->rules[i].lookup_cnt);
	    fpst->rules[i].lookups = malloc(fpst->rules[i].lookup_cnt*sizeof(struct seqlookup));
	    for ( j=k=0; j<fpst->rules[i].lookup_cnt; ++j ) {
		getname(sfd,tok);
		getint(sfd,&fpst->rules[i].lookups[j].seq);
		fpst->rules[i].lookups[k].lookup = SFD_ParseNestedLookup(sfd,sf,old);
		if ( fpst->rules[i].lookups[k].lookup!=NULL )
		    ++k;
	    }
	    fpst->rules[i].lookup_cnt = k;
	  break;
	  case pst_reversecoverage:
	    getname(sfd,tok);
	    getint(sfd,&temp);
	    fpst->rules[i].u.rcoverage.replacements = malloc(temp+1);
	    fpst->rules[i].u.rcoverage.replacements[temp] = '\0';
	    nlgetc(sfd);	/* skip space */
	    fread(fpst->rules[i].u.rcoverage.replacements,1,temp,sfd);
	  break;
	  default:
	  break;
	}
    }
    getname(sfd,tok);	/* EndFPST, or one of the ClassName tokens (in newer sfds) */
    while ( strcmp(tok,"ClassNames:")==0 || strcmp(tok,"BClassNames:")==0 ||
	    strcmp(tok,"FClassNames:")==0 ) {
	int which = strcmp(tok,"ClassNames:")==0 ? 0 :
		    strcmp(tok,"BClassNames:")==0 ? 1 : 2;
	int cnt = (&fpst->nccnt)[which];
	char **classnames = (&fpst->nclassnames)[which];
	int i;

	for ( i=0; i<cnt; ++i )
	    classnames[i] = SFDReadUTF7Str(sfd);
	getname(sfd,tok);	/* EndFPST, or one of the ClassName tokens (in newer sfds) */
    }

}

static void SFDParseStateMachine(FILE *sfd,SplineFont *sf,ASM *sm, char *tok,int old) {
    int i, temp;

    sm->type = strnmatch(tok,"MacIndic",8)==0 ? asm_indic :
		strnmatch(tok,"MacContext",10)==0 ? asm_context :
		strnmatch(tok,"MacLigature",11)==0 ? asm_lig :
		strnmatch(tok,"MacSimple",9)==0 ? asm_simple :
		strnmatch(tok,"MacKern",7)==0 ? asm_kern : asm_insert;
    if ( old ) {
	getusint(sfd,&((ASM1 *) sm)->feature);
	nlgetc(sfd);		/* Skip comma */
	getusint(sfd,&((ASM1 *) sm)->setting);
    } else {
	sm->subtable = SFFindLookupSubtableAndFreeName(sf,SFDReadUTF7Str(sfd));
	sm->subtable->sm = sm;
    }
    getusint(sfd,&sm->flags);
    getusint(sfd,&sm->class_cnt);
    getusint(sfd,&sm->state_cnt);

    sm->classes = malloc(sm->class_cnt*sizeof(char *));
    sm->classes[0] = sm->classes[1] = sm->classes[2] = sm->classes[3] = NULL;
    for ( i=4; i<sm->class_cnt; ++i ) {
	getname(sfd,tok);
	getint(sfd,&temp);
	sm->classes[i] = malloc(temp+1); sm->classes[i][temp] = '\0';
	nlgetc(sfd);	/* skip space */
	fread(sm->classes[i],1,temp,sfd);
    }

    sm->state = malloc(sm->class_cnt*sm->state_cnt*sizeof(struct asm_state));
    for ( i=0; i<sm->class_cnt*sm->state_cnt; ++i ) {
	getusint(sfd,&sm->state[i].next_state);
	getusint(sfd,&sm->state[i].flags);
	if ( sm->type == asm_context ) {
	    sm->state[i].u.context.mark_lookup = SFD_ParseNestedLookup(sfd,sf,old);
	    sm->state[i].u.context.cur_lookup = SFD_ParseNestedLookup(sfd,sf,old);
	} else if ( sm->type == asm_insert ) {
	    getint(sfd,&temp);
	    if ( temp==0 )
		sm->state[i].u.insert.mark_ins = NULL;
	    else {
		sm->state[i].u.insert.mark_ins = malloc(temp+1); sm->state[i].u.insert.mark_ins[temp] = '\0';
		nlgetc(sfd);	/* skip space */
		fread(sm->state[i].u.insert.mark_ins,1,temp,sfd);
	    }
	    getint(sfd,&temp);
	    if ( temp==0 )
		sm->state[i].u.insert.cur_ins = NULL;
	    else {
		sm->state[i].u.insert.cur_ins = malloc(temp+1); sm->state[i].u.insert.cur_ins[temp] = '\0';
		nlgetc(sfd);	/* skip space */
		fread(sm->state[i].u.insert.cur_ins,1,temp,sfd);
	    }
	} else if ( sm->type == asm_kern ) {
	    int j;
	    getint(sfd,&sm->state[i].u.kern.kcnt);
	    if ( sm->state[i].u.kern.kcnt!=0 )
		sm->state[i].u.kern.kerns = malloc(sm->state[i].u.kern.kcnt*sizeof(int16));
	    for ( j=0; j<sm->state[i].u.kern.kcnt; ++j ) {
		getint(sfd,&temp);
		sm->state[i].u.kern.kerns[j] = temp;
	    }
	}
    }
    getname(sfd,tok);			/* EndASM */
}

static struct macname *SFDParseMacNames(FILE *sfd, char *tok) {
    struct macname *head=NULL, *last=NULL, *cur;
    int enc, lang, len;
    char *pt;
    int ch;

    while ( strcmp(tok,"MacName:")==0 ) {
	cur = chunkalloc(sizeof(struct macname));
	if ( last==NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;

	getint(sfd,&enc);
	getint(sfd,&lang);
	getint(sfd,&len);
	cur->enc = enc;
	cur->lang = lang;
	cur->name = pt = malloc(len+1);

	while ( (ch=nlgetc(sfd))==' ');
	if ( ch=='"' )
	    ch = nlgetc(sfd);
	while ( ch!='"' && ch!=EOF && pt<cur->name+len ) {
	    if ( ch=='\\' ) {
		*pt  = (nlgetc(sfd)-'0')<<6;
		*pt |= (nlgetc(sfd)-'0')<<3;
		*pt |= (nlgetc(sfd)-'0');
	    } else
		*pt++ = ch;
	    ch = nlgetc(sfd);
	}
	*pt = '\0';
	getname(sfd,tok);
    }
return( head );
}

MacFeat *SFDParseMacFeatures(FILE *sfd, char *tok) {
    MacFeat *cur, *head=NULL, *last=NULL;
    struct macsetting *slast, *scur;
    int feat, ism, def, set;

    while ( strcmp(tok,"MacFeat:")==0 ) {
	cur = chunkalloc(sizeof(MacFeat));
	if ( last==NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;

	getint(sfd,&feat); getint(sfd,&ism); getint(sfd, &def);
	cur->feature = feat; cur->ismutex = ism; cur->default_setting = def;
	getname(sfd,tok);
	cur->featname = SFDParseMacNames(sfd,tok);
	slast = NULL;
	while ( strcmp(tok,"MacSetting:")==0 ) {
	    scur = chunkalloc(sizeof(struct macsetting));
	    if ( slast==NULL )
		cur->settings = scur;
	    else
		slast->next = scur;
	    slast = scur;

	    getint(sfd,&set);
	    scur->setting = set;
	    getname(sfd,tok);
	    scur->setname = SFDParseMacNames(sfd,tok);
	}
    }
return( head );
}

static char *SFDParseMMSubroutine(FILE *sfd) {
    char buffer[400], *sofar=calloc(1,1);
    const char *endtok = "EndMMSubroutine";
    int len = 0, blen, first=true;

    while ( fgets(buffer,sizeof(buffer),sfd)!=NULL ) {
	if ( strncmp(buffer,endtok,strlen(endtok))==0 )
    break;
	if ( first ) {
	    first = false;
	    if ( strcmp(buffer,"\n")==0 )
    continue;
	}
	blen = strlen(buffer);
	sofar = realloc(sofar,len+blen+1);
	strcpy(sofar+len,buffer);
	len += blen;
    }
    if ( len>0 && sofar[len-1]=='\n' )
	sofar[len-1] = '\0';
return( sofar );
}

static void MMInferStuff(MMSet *mm) {
    int i,j;

    if ( mm==NULL )
return;
    if ( mm->apple ) {
	for ( i=0; i<mm->axis_count; ++i ) {
	    for ( j=0; j<mm->axismaps[i].points; ++j ) {
		real val = mm->axismaps[i].blends[j];
		if ( val == -1. )
		    mm->axismaps[i].min = mm->axismaps[i].designs[j];
		else if ( val==0 )
		    mm->axismaps[i].def = mm->axismaps[i].designs[j];
		else if ( val==1 )
		    mm->axismaps[i].max = mm->axismaps[i].designs[j];
	    }
	}
    }
}

static void SFDSizeMap(EncMap *map,int glyphcnt,int enccnt) {
    if ( glyphcnt>map->backmax ) {
	map->backmap = realloc(map->backmap,glyphcnt*sizeof(int));
	memset(map->backmap+map->backmax,-1,(glyphcnt-map->backmax)*sizeof(int));
	map->backmax = glyphcnt;
    }
    if ( enccnt>map->encmax ) {
	map->map = realloc(map->map,enccnt*sizeof(int));
	memset(map->map+map->backmax,-1,(enccnt-map->encmax)*sizeof(int));
	map->encmax = map->enccount = enccnt;
    }
}

static SplineFont *SFD_GetFont(FILE *sfd,SplineFont *cidmaster,char *tok,
	int fromdir, char *dirname, float sfdversion);

static SplineFont *SFD_FigureDirType(SplineFont *sf,char *tok, char *dirname,
	Encoding *enc, struct remap *remap,int had_layer_cnt) {
    /* In a sfdir a directory will either contain glyph files */
    /*                                            subfont dirs */
    /*                                            instance dirs */
    /* (or bitmap files, but we don't care about them here */
    /* It will not contain some glyph and some subfont nor instance files */
    int gc=0, sc=0, ic=0, bc=0;
    DIR *dir;
    struct dirent *ent;
    char *name, *props, *pt;

    dir = opendir(dirname);
    if ( dir==NULL )
return( sf );
    sf->save_to_dir = true;
    while ( (ent=readdir(dir))!=NULL ) {
	pt = strrchr(ent->d_name,EXT_CHAR);
	if ( pt==NULL )
	    /* Nothing interesting */;
	else if ( strcmp(pt,GLYPH_EXT)==0 )
	    ++gc;
	else if ( strcmp(pt,SUBFONT_EXT)==0 )
	    ++sc;
	else if ( strcmp(pt,INSTANCE_EXT)==0 )
	    ++ic;
	else if ( strcmp(pt,STRIKE_EXT)==0 )
	    ++bc;
    }
    rewinddir(dir);
    name = malloc(strlen(dirname)+NAME_MAX+3);
    props = malloc(strlen(dirname)+2*NAME_MAX+4);
    if ( gc!=0 ) {
	sf->glyphcnt = 0;
	sf->glyphmax = gc;
	sf->glyphs = calloc(gc,sizeof(SplineChar *));
	ff_progress_change_total(gc);
	if ( sf->cidmaster!=NULL ) {
	    sf->map = sf->cidmaster->map;
	} else {
	    sf->map = EncMapNew(enc->char_cnt>gc?enc->char_cnt:gc,gc,enc);
	    sf->map->remap = remap;
	}
	SFDSizeMap(sf->map,sf->glyphcnt,enc->char_cnt>gc?enc->char_cnt:gc);

	while ( (ent=readdir(dir))!=NULL ) {
	    pt = strrchr(ent->d_name,EXT_CHAR);
	    if ( pt==NULL )
		/* Nothing interesting */;
	    else if ( strcmp(pt,GLYPH_EXT)==0 ) {
		FILE *gsfd;
		sprintf(name,"%s/%s", dirname, ent->d_name);
		gsfd = fopen(name,"r");
		if ( gsfd!=NULL ) {
		    SFDGetChar(gsfd,sf,had_layer_cnt);
		    ff_progress_next();
		    fclose(gsfd);
		}
	    }
	}
	ff_progress_next_stage();
    } else if ( sc!=0 ) {
	int i=0;
	sf->subfontcnt = sc;
	sf->subfonts = calloc(sf->subfontcnt,sizeof(SplineFont *));
	sf->map = EncMap1to1(1000);
	ff_progress_change_stages(2*sc);

	while ( (ent=readdir(dir))!=NULL ) {
	    pt = strrchr(ent->d_name,EXT_CHAR);
	    if ( pt==NULL )
		/* Nothing interesting */;
	    else if ( strcmp(pt,SUBFONT_EXT)==0 && i<sc ) {
		FILE *ssfd;
		sprintf(name,"%s/%s", dirname, ent->d_name);
		sprintf(props,"%s/" FONT_PROPS, name);
		ssfd = fopen(props,"r");
		if ( ssfd!=NULL ) {
		    if ( i!=0 )
			ff_progress_next_stage();
		    sf->subfonts[i++] = SFD_GetFont(ssfd,sf,tok,true,name,sf->sfd_version);
		    fclose(ssfd);
		}
	    }
	}
    } else if ( ic!=0 ) {
	MMSet *mm = sf->mm;
	int ipos, i=0;

	MMInferStuff(sf->mm);
	ff_progress_change_stages(2*(mm->instance_count+1));
	while ( (ent=readdir(dir))!=NULL ) {
	    pt = strrchr(ent->d_name,EXT_CHAR);
	    if ( pt==NULL )
		/* Nothing interesting */;
	    else if ( strcmp(pt,INSTANCE_EXT)==0 && sscanf( ent->d_name, "mm%d", &ipos)==1 ) {
		FILE *ssfd;
		if ( i!=0 )
		    ff_progress_next_stage();
		sprintf(name,"%s/%s", dirname, ent->d_name);
		sprintf(props,"%s/" FONT_PROPS, name);
		ssfd = fopen(props,"r");
		if ( ssfd!=NULL ) {
		    SplineFont *mmsf;
		    mmsf = SFD_GetFont(ssfd,NULL,tok,true,name,sf->sfd_version);
		    if ( ipos!=0 ) {
			EncMapFree(mmsf->map);
			mmsf->map=NULL;
		    }
		    mmsf->mm = mm;
		    if ( ipos == 0 )
			mm->normal = mmsf;
		    else
			mm->instances[ipos-1] = mmsf;
		    fclose(ssfd);
		}
	    }
	}
	ff_progress_next_stage();
	sf->mm = NULL;
	SplineFontFree(sf);
	sf = mm->normal;
	if ( sf->map->enc!=&custom ) {
	    EncMap *map;
	    MMMatchGlyphs(mm);		/* sfd files from before the encoding change can have mismatched orig pos */
	    map = EncMapFromEncoding(sf,sf->map->enc);
	    EncMapFree(sf->map);
	    sf->map = map;
	}
    }

    if ( bc!=0 ) {
	rewinddir(dir);
	while ( (ent=readdir(dir))!=NULL ) {
	    pt = strrchr(ent->d_name,EXT_CHAR);
	    if ( pt==NULL )
		/* Nothing interesting */;
	    else if ( strcmp(pt,STRIKE_EXT)==0 ) {
		FILE *ssfd;
		sprintf(name,"%s/%s", dirname, ent->d_name);
		sprintf(props,"%s/" STRIKE_PROPS, name);
		ssfd = fopen(props,"r");
		if ( ssfd!=NULL ) {
		    if ( getname(ssfd,tok)==1 && strcmp(tok,"BitmapFont:")==0 )
			SFDGetBitmapFont(ssfd,sf,true,name);
		    fclose(ssfd);
		}
	    }
	}
	SFOrderBitmapList(sf);
    }
    closedir(dir);
    free(name);
    free(props);
return( sf );
}

static void SFD_DoAltUnis(SplineFont *sf) {
    int i;
    struct altuni *alt;
    SplineChar *sc;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc = sf->glyphs[i])!=NULL ) {
	for ( alt = sc->altuni; alt!=NULL; alt = alt->next ) {
	    if ( alt->vs==-1 && alt->fid==0 ) {
		int enc = EncFromUni(alt->unienc,sf->map->enc);
		if ( enc!=-1 )
		    SFDSetEncMap(sf,sc->orig_pos,enc);
	    }
	}
    }
}

static void SFDParseLookup(FILE *sfd,OTLookup *otl) {
    int ch;
    struct lookup_subtable *sub, *lastsub;
    FeatureScriptLangList *fl, *lastfl;
    struct scriptlanglist *sl, *lastsl;
    int i, lcnt, lmax=0;
    uint32 *langs=NULL;
    char *subname;

    while ( (ch=nlgetc(sfd))==' ' );
    if ( ch=='{' ) {
	lastsub = NULL;
	while ( (subname = SFDReadUTF7Str(sfd))!=NULL ) {
	    while ( (ch=nlgetc(sfd))==' ' );
	    ungetc(ch,sfd);
	    sub = chunkalloc(sizeof(struct lookup_subtable));
	    sub->subtable_name = subname;
	    sub->lookup = otl;
	    switch ( otl->lookup_type ) {
	      case gsub_single:
		while ( (ch=nlgetc(sfd))==' ' );
		if ( ch=='(' ) {
		    sub->suffix = SFDReadUTF7Str(sfd);
		    while ( (ch=nlgetc(sfd))==' ' );
			/* slurp final paren */
		} else
		    ungetc(ch,sfd);
		sub->per_glyph_pst_or_kern = true;
	      break;
	      case gsub_multiple: case gsub_alternate: case gsub_ligature:
	      case gpos_single:
		sub->per_glyph_pst_or_kern = true;
	      break;
	      case gpos_pair:
		if ( (ch=nlgetc(sfd))=='(' ) {
		    ch = nlgetc(sfd);
		    sub->vertical_kerning = (ch=='1');
		    nlgetc(sfd);	/* slurp final paren */
		    ch=nlgetc(sfd);
		}
		if ( ch=='[' ) {
		    getsint(sfd,&sub->separation);
		    nlgetc(sfd);	/* slurp comma */
		    getsint(sfd,&sub->minkern);
		    nlgetc(sfd);	/* slurp comma */
		    ch = nlgetc(sfd);
		    sub->kerning_by_touch = ((ch-'0')&1)?1:0;
		    sub->onlyCloser       = ((ch-'0')&2)?1:0;
		    sub->dontautokern     = ((ch-'0')&4)?1:0;
		    nlgetc(sfd);	/* slurp final bracket */
		} else {
		    ungetc(ch,sfd);
		}
		sub->per_glyph_pst_or_kern = true;
	      break;
	      case gpos_cursive: case gpos_mark2base: case gpos_mark2ligature: case gpos_mark2mark:
		sub->anchor_classes = true;
	      break;
	      default:
	      break;
	    }
	    if ( lastsub==NULL )
		otl->subtables = sub;
	    else
		lastsub->next = sub;
	    lastsub = sub;
	}
	while ( (ch=nlgetc(sfd))==' ' );
	if ( ch=='}' )
	    ch = nlgetc(sfd);
    }
    while ( ch==' ' )
	ch = nlgetc(sfd);
    if ( ch=='[' ) {
	lastfl = NULL;
	for (;;) {
	    while ( (ch=nlgetc(sfd))==' ' );
	    if ( ch==']' )
	break;
	    fl = chunkalloc(sizeof(FeatureScriptLangList));
	    if ( lastfl==NULL )
		otl->features = fl;
	    else
		lastfl->next = fl;
	    lastfl = fl;
	    if ( ch=='<' ) {
		int ft=0,fs=0;
		fscanf(sfd,"%d,%d>", &ft, &fs );
		fl->ismac = true;
		fl->featuretag = (ft<<16) | fs;
	    } else if ( ch=='\'' ) {
		ungetc(ch,sfd);
		fl->featuretag = gettag(sfd);
	    }
	    while ( (ch=nlgetc(sfd))==' ' );
	    if ( ch=='(' ) {
		lastsl = NULL;
		for (;;) {
		    while ( (ch=nlgetc(sfd))==' ' );
		    if ( ch==')' )
		break;
		    sl = chunkalloc(sizeof(struct scriptlanglist));
		    if ( lastsl==NULL )
			fl->scripts = sl;
		    else
			lastsl->next = sl;
		    lastsl = sl;
		    if ( ch=='\'' ) {
			ungetc(ch,sfd);
			sl->script = gettag(sfd);
		    }
		    while ( (ch=nlgetc(sfd))==' ' );
		    if ( ch=='<' ) {
			lcnt = 0;
			for (;;) {
			    while ( (ch=nlgetc(sfd))==' ' );
			    if ( ch=='>' )
			break;
			    if ( ch=='\'' ) {
				ungetc(ch,sfd);
			        if ( lcnt>=lmax )
				    langs = realloc(langs,(lmax+=10)*sizeof(uint32));
				langs[lcnt++] = gettag(sfd);
			    }
			}
			sl->lang_cnt = lcnt;
			if ( lcnt>MAX_LANG )
			    sl->morelangs = malloc((lcnt-MAX_LANG)*sizeof(uint32));
			for ( i=0; i<lcnt; ++i ) {
			    if ( i<MAX_LANG )
				sl->langs[i] = langs[i];
			    else
				sl->morelangs[i-MAX_LANG] = langs[i];
			}
		    }
		}
	    }
	}
    }
    free(langs);
}

static void SFDParseMathItem(FILE *sfd,SplineFont *sf,char *tok) {
    /* The first five characters of a math item's keyword will be "MATH:" */
    /*  the rest will be one of the entries in math_constants_descriptor */
    int i;
    struct MATH *math;

    if ( (math = sf->MATH) == NULL )
	math = sf->MATH = calloc(1,sizeof(struct MATH));
    for ( i=0; math_constants_descriptor[i].script_name!=NULL; ++i ) {
	char *name = math_constants_descriptor[i].script_name;
	int len = strlen( name );
	if ( strncmp(tok+5,name,len)==0 && tok[5+len] == ':' && tok[6+len]=='\0' ) {
	    int16 *pos = (int16 *) (((char *) (math)) + math_constants_descriptor[i].offset );
	    getsint(sfd,pos);
	    if ( math_constants_descriptor[i].devtab_offset != -1 ) {
		DeviceTable **devtab = (DeviceTable **) (((char *) (math)) + math_constants_descriptor[i].devtab_offset );
		*devtab = SFDReadDeviceTable(sfd,*devtab);
    break;
	    }
	}
    }
}

static struct baselangextent *ParseBaseLang(FILE *sfd) {
    struct baselangextent *bl;
    struct baselangextent *cur, *last;
    int ch;

    while ( (ch=nlgetc(sfd))==' ' );
    if ( ch=='{' ) {
	bl = chunkalloc(sizeof(struct baselangextent));
	while ( (ch=nlgetc(sfd))==' ' );
	ungetc(ch,sfd);
	if ( ch=='\'' )
	    bl->lang = gettag(sfd);		/* Lang or Feature tag, or nothing */
	getsint(sfd,&bl->descent);
	getsint(sfd,&bl->ascent);
	last = NULL;
	while ( (ch=nlgetc(sfd))==' ' );
	while ( ch=='{' ) {
	    ungetc(ch,sfd);
	    cur = ParseBaseLang(sfd);
	    if ( last==NULL )
		bl->features = cur;
	    else
		last->next = cur;
	    last = cur;
	    while ( (ch=nlgetc(sfd))==' ' );
	}
	if ( ch!='}' ) ungetc(ch,sfd);
return( bl );
    }
return( NULL );
}

static struct basescript *SFDParseBaseScript(FILE *sfd,struct Base *base) {
    struct basescript *bs;
    int i, ch;
    struct baselangextent *last, *cur;

    if ( base==NULL )
return(NULL);

    bs = chunkalloc(sizeof(struct basescript));

    bs->script = gettag(sfd);
    getint(sfd,&bs->def_baseline);
    if ( base->baseline_cnt!=0 ) {
	bs->baseline_pos = calloc(base->baseline_cnt,sizeof(int16));
	for ( i=0; i<base->baseline_cnt; ++i )
	    getsint(sfd, &bs->baseline_pos[i]);
    }
    while ( (ch=nlgetc(sfd))==' ' );
    last = NULL;
    while ( ch=='{' ) {
	ungetc(ch,sfd);
	cur = ParseBaseLang(sfd);
	if ( last==NULL )
	    bs->langs = cur;
	else
	    last->next = cur;
	last = cur;
	while ( (ch=nlgetc(sfd))==' ' );
    }
return( bs );
}

static struct Base *SFDParseBase(FILE *sfd) {
    struct Base *base = chunkalloc(sizeof(struct Base));
    int i;

    getint(sfd,&base->baseline_cnt);
    if ( base->baseline_cnt!=0 ) {
	base->baseline_tags = malloc(base->baseline_cnt*sizeof(uint32));
	for ( i=0; i<base->baseline_cnt; ++i )
	    base->baseline_tags[i] = gettag(sfd);
    }
return( base );
}

static OTLookup **SFDLookupList(FILE *sfd,SplineFont *sf) {
    int ch;
    OTLookup *space[100], **buf=space, *otl, **ret;
    int lcnt=0, lmax=100;
    char *name;

    for (;;) {
	while ( (ch=nlgetc(sfd))==' ' );
	if ( ch=='\n' || ch==EOF )
    break;
	ungetc(ch,sfd);
	name = SFDReadUTF7Str(sfd);
	otl = SFFindLookup(sf,name);
	free(name);
	if ( otl!=NULL ) {
	    if ( lcnt>lmax ) {
		if ( buf==space ) {
		    buf = malloc((lmax=lcnt+50)*sizeof(OTLookup *));
		    memcpy(buf,space,sizeof(space));
		} else
		    buf = realloc(buf,(lmax+=50)*sizeof(OTLookup *));
	    }
	    buf[lcnt++] = otl;
	}
    }
    if ( lcnt==0 )
return( NULL );

    ret = malloc((lcnt+1)*sizeof(OTLookup *));
    memcpy(ret,buf,lcnt*sizeof(OTLookup *));
    ret[lcnt] = NULL;
return( ret );
}

static void SFDParseJustify(FILE *sfd, SplineFont *sf, char *tok) {
    Justify *last=NULL, *cur;
    struct jstf_lang *jlang, *llast;
    int p = 0,ch;

    while ( strcmp(tok,"Justify:")==0 ) {
	cur = chunkalloc(sizeof(Justify));
	if ( last==NULL )
	    sf->justify = cur;
	else
	    last->next = cur;
	last = cur;
	llast = jlang = NULL;
	cur->script = gettag(sfd);
	while ( getname(sfd,tok)>0 ) {
	    if ( strcmp(tok,"Justify:")==0 || strcmp(tok,"EndJustify")==0 )
	break;
	    if ( strcmp(tok,"JstfExtender:")==0 ) {
		while ( (ch=nlgetc(sfd))==' ' );
		ungetc(ch,sfd);
		geteol(sfd,tok);
		cur->extenders = copy(tok);
	    } else if ( strcmp(tok,"JstfLang:")==0 ) {
		jlang = chunkalloc(sizeof(struct jstf_lang));
		if ( llast==NULL )
		    cur->langs = jlang;
		else
		    llast->next = jlang;
		llast = jlang;
		jlang->lang = gettag(sfd);
		p = -1;
		getint(sfd,&jlang->cnt);
		if ( jlang->cnt!=0 )
		    jlang->prios = calloc(jlang->cnt,sizeof(struct jstf_prio));
	    } else if ( strcmp(tok,"JstfPrio:")==0 ) {
		if ( jlang!=NULL ) {
		    ++p;
		    if ( p>= jlang->cnt ) {
			jlang->prios = realloc(jlang->prios,(p+1)*sizeof(struct jstf_prio));
			memset(jlang->prios+jlang->cnt,0,(p+1-jlang->cnt)*sizeof(struct jstf_prio));
			jlang->cnt = p+1;
		    }
		}
	    } else if ( strcmp(tok,"JstfEnableShrink:" )==0 ) {
		if ( p<0 ) p=0;
		if ( jlang!=NULL && p<jlang->cnt )
		    jlang->prios[p].enableShrink = SFDLookupList(sfd,sf);
	    } else if ( strcmp(tok,"JstfDisableShrink:" )==0 ) {
		if ( p<0 ) p=0;
		if ( jlang!=NULL && p<jlang->cnt )
		    jlang->prios[p].disableShrink = SFDLookupList(sfd,sf);
	    } else if ( strcmp(tok,"JstfMaxShrink:" )==0 ) {
		if ( p<0 ) p=0;
		if ( jlang!=NULL && p<jlang->cnt )
		    jlang->prios[p].maxShrink = SFDLookupList(sfd,sf);
	    } else if ( strcmp(tok,"JstfEnableExtend:" )==0 ) {
		if ( p<0 ) p=0;
		if ( jlang!=NULL && p<jlang->cnt )
		    jlang->prios[p].enableExtend = SFDLookupList(sfd,sf);
	    } else if ( strcmp(tok,"JstfDisableExtend:" )==0 ) {
		if ( p<0 ) p=0;
		if ( jlang!=NULL && p<jlang->cnt )
		    jlang->prios[p].disableExtend = SFDLookupList(sfd,sf);
	    } else if ( strcmp(tok,"JstfMaxExtend:" )==0 ) {
		if ( p<0 ) p=0;
		if ( jlang!=NULL && p<jlang->cnt )
		    jlang->prios[p].maxExtend = SFDLookupList(sfd,sf);
	    } else
		geteol(sfd,tok);
	}
    }
}



void SFD_GetFontMetaDataData_Init( SFD_GetFontMetaDataData* d )
{
    memset( d, 0, sizeof(SFD_GetFontMetaDataData));
}

/**
 *
 * @return true if the function matched the current token. If true
 *         is returned the caller should avoid further processing of 'tok'
 *         a return of false means that the caller might try
 *         to handle the token with another function or drop it.
 */
bool SFD_GetFontMetaData( FILE *sfd,
			  char *tok,
			  SplineFont *sf,
			  SFD_GetFontMetaDataData* d )
{
    int ch;
    int i;
    KernClass* kc = 0;
    int old;
    char val[2000];

    // This allows us to assume we can dereference d
    // at all times
    static SFD_GetFontMetaDataData my_static_d;
    static int my_static_d_is_virgin = 1;
    if( !d )
    {
	if( my_static_d_is_virgin )
	{
	    my_static_d_is_virgin = 0;
	    SFD_GetFontMetaDataData_Init( &my_static_d );
	}
	d = &my_static_d;
    }

    if ( strmatch(tok,"FontName:")==0 )
    {
	geteol(sfd,val);
	sf->fontname = copy(val);
    }
    else if ( strmatch(tok,"FullName:")==0 )
    {
	geteol(sfd,val);
	sf->fullname = copy(val);
    }
    else if ( strmatch(tok,"FamilyName:")==0 )
    {
	geteol(sfd,val);
	sf->familyname = copy(val);
    }
    else if ( strmatch(tok,"DefaultBaseFilename:")==0 )
    {
	geteol(sfd,val);
	sf->defbasefilename = copy(val);
    }
    else if ( strmatch(tok,"Weight:")==0 )
    {
	getprotectedname(sfd,val);
	sf->weight = copy(val);
    }
    else if ( strmatch(tok,"Copyright:")==0 )
    {
	sf->copyright = getquotedeol(sfd);
    }
    else if ( strmatch(tok,"Comments:")==0 )
    {
	char *temp = getquotedeol(sfd);
	sf->comments = latin1_2_utf8_copy(temp);
	free(temp);
    }
    else if ( strmatch(tok,"UComments:")==0 )
    {
	sf->comments = SFDReadUTF7Str(sfd);
    }
    else if ( strmatch(tok,"FontLog:")==0 )
    {
	sf->fontlog = SFDReadUTF7Str(sfd);
    }
    else if ( strmatch(tok,"Version:")==0 )
    {
	geteol(sfd,val);
	sf->version = copy(val);
    }
    else if ( strmatch(tok,"StyleMapFamilyName:")==0 )
    {
    sf->styleMapFamilyName = SFDReadUTF7Str(sfd);
    }
    /* Legacy attribute for StyleMapFamilyName. Deprecated. */
    else if ( strmatch(tok,"OS2FamilyName:")==0 )
    {
    if (sf->styleMapFamilyName == NULL)
        sf->styleMapFamilyName = SFDReadUTF7Str(sfd);
    }
    else if ( strmatch(tok,"FONDName:")==0 )
    {
	geteol(sfd,val);
	sf->fondname = copy(val);
    }
    else if ( strmatch(tok,"ItalicAngle:")==0 )
    {
	getreal(sfd,&sf->italicangle);
    }
    else if ( strmatch(tok,"StrokeWidth:")==0 )
    {
	getreal(sfd,&sf->strokewidth);
    }
    else if ( strmatch(tok,"UnderlinePosition:")==0 )
    {
	getreal(sfd,&sf->upos);
    }
    else if ( strmatch(tok,"UnderlineWidth:")==0 )
    {
	getreal(sfd,&sf->uwidth);
    }
    else if ( strmatch(tok,"ModificationTime:")==0 )
    {
	getlonglong(sfd,&sf->modificationtime);
    }
    else if ( strmatch(tok,"CreationTime:")==0 )
    {
	getlonglong(sfd,&sf->creationtime);
	d->hadtimes = true;
    }
    else if ( strmatch(tok,"PfmFamily:")==0 )
    {
	int temp;
	getint(sfd,&temp);
	sf->pfminfo.pfmfamily = temp;
	sf->pfminfo.pfmset = true;
    }
    else if ( strmatch(tok,"LangName:")==0 )
    {
	sf->names = SFDGetLangName(sfd,sf->names);
    }
    else if ( strmatch(tok,"GaspTable:")==0 )
    {
	SFDGetGasp(sfd,sf);
    }
    else if ( strmatch(tok,"DesignSize:")==0 )
    {
	SFDGetDesignSize(sfd,sf);
    }
    else if ( strmatch(tok,"OtfFeatName:")==0 )
    {
	SFDGetOtfFeatName(sfd,sf);
    }
    else if ( strmatch(tok,"PfmWeight:")==0 || strmatch(tok,"TTFWeight:")==0 )
    {
	getsint(sfd,&sf->pfminfo.weight);
	sf->pfminfo.pfmset = true;
    }
    else if ( strmatch(tok,"TTFWidth:")==0 )
    {
	getsint(sfd,&sf->pfminfo.width);
	sf->pfminfo.pfmset = true;
    }
    else if ( strmatch(tok,"Panose:")==0 )
    {
	int temp,i;
	for ( i=0; i<10; ++i )
	{
	    getint(sfd,&temp);
	    sf->pfminfo.panose[i] = temp;
	}
	sf->pfminfo.panose_set = true;
    }
    else if ( strmatch(tok,"LineGap:")==0 )
    {
	getsint(sfd,&sf->pfminfo.linegap);
	sf->pfminfo.pfmset = true;
    }
    else if ( strmatch(tok,"VLineGap:")==0 )
    {
	getsint(sfd,&sf->pfminfo.vlinegap);
	sf->pfminfo.pfmset = true;
    }
    else if ( strmatch(tok,"HheadAscent:")==0 )
    {
	getsint(sfd,&sf->pfminfo.hhead_ascent);
    }
    else if ( strmatch(tok,"HheadAOffset:")==0 )
    {
	int temp;
	getint(sfd,&temp); sf->pfminfo.hheadascent_add = temp;
    }
    else if ( strmatch(tok,"HheadDescent:")==0 )
    {
	getsint(sfd,&sf->pfminfo.hhead_descent);
    }
    else if ( strmatch(tok,"HheadDOffset:")==0 )
    {
	int temp;
	getint(sfd,&temp); sf->pfminfo.hheaddescent_add = temp;
    }
    else if ( strmatch(tok,"OS2TypoLinegap:")==0 )
    {
	getsint(sfd,&sf->pfminfo.os2_typolinegap);
    }
    else if ( strmatch(tok,"OS2TypoAscent:")==0 )
    {
	getsint(sfd,&sf->pfminfo.os2_typoascent);
    }
    else if ( strmatch(tok,"OS2TypoAOffset:")==0 )
    {
	int temp;
	getint(sfd,&temp); sf->pfminfo.typoascent_add = temp;
    }
    else if ( strmatch(tok,"OS2TypoDescent:")==0 )
    {
	getsint(sfd,&sf->pfminfo.os2_typodescent);
    }
    else if ( strmatch(tok,"OS2TypoDOffset:")==0 )
    {
	int temp;
	getint(sfd,&temp); sf->pfminfo.typodescent_add = temp;
    }
    else if ( strmatch(tok,"OS2WinAscent:")==0 )
    {
	getsint(sfd,&sf->pfminfo.os2_winascent);
    }
    else if ( strmatch(tok,"OS2WinDescent:")==0 )
    {
	getsint(sfd,&sf->pfminfo.os2_windescent);
    }
    else if ( strmatch(tok,"OS2WinAOffset:")==0 )
    {
	int temp;
	getint(sfd,&temp); sf->pfminfo.winascent_add = temp;
    }
    else if ( strmatch(tok,"OS2WinDOffset:")==0 )
    {
	int temp;
	getint(sfd,&temp); sf->pfminfo.windescent_add = temp;
    }
    else if ( strmatch(tok,"HHeadAscent:")==0 )
    {
	// DUPLICATE OF ABOVE
	getsint(sfd,&sf->pfminfo.hhead_ascent);
    }
    else if ( strmatch(tok,"HHeadDescent:")==0 )
    {
	// DUPLICATE OF ABOVE
	getsint(sfd,&sf->pfminfo.hhead_descent);
    }

    else if ( strmatch(tok,"HHeadAOffset:")==0 )
    {
	// DUPLICATE OF ABOVE
	int temp;
	getint(sfd,&temp); sf->pfminfo.hheadascent_add = temp;
    }
    else if ( strmatch(tok,"HHeadDOffset:")==0 )
    {
	// DUPLICATE OF ABOVE
	int temp;
	getint(sfd,&temp); sf->pfminfo.hheaddescent_add = temp;
    }
    else if ( strmatch(tok,"MacStyle:")==0 )
    {
	getsint(sfd,&sf->macstyle);
    }
    else if ( strmatch(tok,"OS2SubXSize:")==0 )
    {
	getsint(sfd,&sf->pfminfo.os2_subxsize);
	sf->pfminfo.subsuper_set = true;
    }
    else if ( strmatch(tok,"OS2SubYSize:")==0 )
    {
	getsint(sfd,&sf->pfminfo.os2_subysize);
    }
    else if ( strmatch(tok,"OS2SubXOff:")==0 )
    {
	getsint(sfd,&sf->pfminfo.os2_subxoff);
    }
    else if ( strmatch(tok,"OS2SubYOff:")==0 )
    {
	getsint(sfd,&sf->pfminfo.os2_subyoff);
    }
    else if ( strmatch(tok,"OS2SupXSize:")==0 )
    {
	getsint(sfd,&sf->pfminfo.os2_supxsize);
    }
    else if ( strmatch(tok,"OS2SupYSize:")==0 )
    {
	getsint(sfd,&sf->pfminfo.os2_supysize);
    }
    else if ( strmatch(tok,"OS2SupXOff:")==0 )
    {
	getsint(sfd,&sf->pfminfo.os2_supxoff);
    }
    else if ( strmatch(tok,"OS2SupYOff:")==0 )
    {
	getsint(sfd,&sf->pfminfo.os2_supyoff);
    }
    else if ( strmatch(tok,"OS2StrikeYSize:")==0 )
    {
	getsint(sfd,&sf->pfminfo.os2_strikeysize);
    }
    else if ( strmatch(tok,"OS2StrikeYPos:")==0 )
    {
	getsint(sfd,&sf->pfminfo.os2_strikeypos);
    }
    else if ( strmatch(tok,"OS2CapHeight:")==0 )
    {
	getsint(sfd,&sf->pfminfo.os2_capheight);
    }
    else if ( strmatch(tok,"OS2XHeight:")==0 )
    {
	getsint(sfd,&sf->pfminfo.os2_xheight);
    }
    else if ( strmatch(tok,"OS2FamilyClass:")==0 )
    {
	getsint(sfd,&sf->pfminfo.os2_family_class);
    }
    else if ( strmatch(tok,"OS2Vendor:")==0 )
    {
	while ( isspace(nlgetc(sfd)));
	sf->pfminfo.os2_vendor[0] = nlgetc(sfd);
	sf->pfminfo.os2_vendor[1] = nlgetc(sfd);
	sf->pfminfo.os2_vendor[2] = nlgetc(sfd);
	sf->pfminfo.os2_vendor[3] = nlgetc(sfd);
	(void) nlgetc(sfd);
    }
    else if ( strmatch(tok,"OS2CodePages:")==0 )
    {
	gethexints(sfd,sf->pfminfo.codepages,2);
	sf->pfminfo.hascodepages = true;
    }
    else if ( strmatch(tok,"OS2UnicodeRanges:")==0 )
    {
	gethexints(sfd,sf->pfminfo.unicoderanges,4);
	sf->pfminfo.hasunicoderanges = true;
    }
    else if ( strmatch(tok,"TopEncoding:")==0 )
    {
	/* Obsolete */
	getint(sfd,&sf->top_enc);
    }
    else if ( strmatch(tok,"Ascent:")==0 )
    {
	getint(sfd,&sf->ascent);
    }
    else if ( strmatch(tok,"Descent:")==0 )
    {
	getint(sfd,&sf->descent);
    }
    else if ( strmatch(tok,"InvalidEm:")==0 )
    {
	getint(sfd,&sf->invalidem);
    }
    else if ( strmatch(tok,"woffMajor:")==0 )
    {
	getint(sfd,&sf->woffMajor);
    }
    else if ( strmatch(tok,"woffMinor:")==0 )
    {
	getint(sfd,&sf->woffMinor);
    }
    else if ( strmatch(tok,"woffMetadata:")==0 )
    {
	sf->woffMetadata = SFDReadUTF7Str(sfd);
    }
    else if ( strmatch(tok,"UFOAscent:")==0 )
    {
	    getreal(sfd,&sf->ufo_ascent);
    }
    else if ( strmatch(tok,"UFODescent:")==0 )
    {
	getreal(sfd,&sf->ufo_descent);
    }
    else if ( strmatch(tok,"sfntRevision:")==0 )
    {
	gethex(sfd,(uint32 *)&sf->sfntRevision);
    }
    else if ( strmatch(tok,"LayerCount:")==0 )
    {
	d->had_layer_cnt = true;
	getint(sfd,&sf->layer_cnt);
	if ( sf->layer_cnt>2 ) {
	    sf->layers = realloc(sf->layers,sf->layer_cnt*sizeof(LayerInfo));
	    memset(sf->layers+2,0,(sf->layer_cnt-2)*sizeof(LayerInfo));
	}
    }
    else if ( strmatch(tok,"Layer:")==0 )
    {
        // TODO: Read the U. F. O. path.
	int layer, o2, bk;
	getint(sfd,&layer);
	if ( layer>=sf->layer_cnt ) {
	    sf->layers = realloc(sf->layers,(layer+1)*sizeof(LayerInfo));
	    memset(sf->layers+sf->layer_cnt,0,((layer+1)-sf->layer_cnt)*sizeof(LayerInfo));
	    sf->layer_cnt = layer+1;
	}
	getint(sfd,&o2);
	sf->layers[layer].order2 = o2;
	sf->layers[layer].background = layer==ly_back;
	/* Used briefly, now background is after layer name */
	while ( (ch=nlgetc(sfd))==' ' );
	ungetc(ch,sfd);
	if ( ch!='"' ) {
	    getint(sfd,&bk);
	    sf->layers[layer].background = bk;
	}
	/* end of section for obsolete format */
	sf->layers[layer].name = SFDReadUTF7Str(sfd);
	while ( (ch=nlgetc(sfd))==' ' );
	ungetc(ch,sfd);
	if ( ch!='\n' ) {
	    getint(sfd,&bk);
	    sf->layers[layer].background = bk;
	}
	while ( (ch=nlgetc(sfd))==' ' );
	ungetc(ch,sfd);
	if ( ch!='\n' ) { sf->layers[layer].ufo_path = SFDReadUTF7Str(sfd); }
    }
    else if ( strmatch(tok,"PreferredKerning:")==0 )
    {
	int temp;
	getint(sfd,&temp);
	sf->preferred_kerning = temp;
    }
    else if ( strmatch(tok,"StrokedFont:")==0 )
    {
	int temp;
	getint(sfd,&temp);
	sf->strokedfont = temp;
    }
    else if ( strmatch(tok,"MultiLayer:")==0 )
    {
	int temp;
	getint(sfd,&temp);
	sf->multilayer = temp;
    }
    else if ( strmatch(tok,"NeedsXUIDChange:")==0 )
    {
	int temp;
	getint(sfd,&temp);
	sf->changed_since_xuidchanged = temp;
    }
    else if ( strmatch(tok,"VerticalOrigin:")==0 )
    {
	// this doesn't seem to be written ever.
	int temp;
	getint(sfd,&temp);
	sf->hasvmetrics = true;
    }
    else if ( strmatch(tok,"HasVMetrics:")==0 )
    {
	int temp;
	getint(sfd,&temp);
	sf->hasvmetrics = temp;
    }
    else if ( strmatch(tok,"Justify:")==0 )
    {
	SFDParseJustify(sfd,sf,tok);
    }
    else if ( strmatch(tok,"BaseHoriz:")==0 )
    {
	sf->horiz_base = SFDParseBase(sfd);
	d->last_base = sf->horiz_base;
	d->last_base_script = NULL;
    }
    else if ( strmatch(tok,"BaseVert:")==0 )
    {
	sf->vert_base = SFDParseBase(sfd);
	d->last_base = sf->vert_base;
	d->last_base_script = NULL;
    }
    else if ( strmatch(tok,"BaseScript:")==0 )
    {
	struct basescript *bs = SFDParseBaseScript(sfd,d->last_base);
	if ( d->last_base==NULL )
	{
	    BaseScriptFree(bs);
	    bs = NULL;
	}
	else if ( d->last_base_script!=NULL )
	    d->last_base_script->next = bs;
	else
	    d->last_base->scripts = bs;
	d->last_base_script = bs;
    }
    else if ( strmatch(tok,"StyleMap:")==0 )
    {
    gethex(sfd,(uint32 *)&sf->pfminfo.stylemap);
    }
    /* Legacy attribute for StyleMap. Deprecated. */
    else if ( strmatch(tok,"OS2StyleName:")==0 )
    {
    char* sname = SFDReadUTF7Str(sfd);
    if (sf->pfminfo.stylemap == -1) {
        if (strcmp(sname,"bold italic")==0) sf->pfminfo.stylemap = 0x21;
        else if (strcmp(sname,"bold")==0) sf->pfminfo.stylemap = 0x20;
        else if (strcmp(sname,"italic")==0) sf->pfminfo.stylemap = 0x01;
        else if (strcmp(sname,"regular")==0) sf->pfminfo.stylemap = 0x40;
    }
    free(sname);
    }
    else if ( strmatch(tok,"FSType:")==0 )
    {
	getsint(sfd,&sf->pfminfo.fstype);
    }
    else if ( strmatch(tok,"OS2Version:")==0 )
    {
	getsint(sfd,&sf->os2_version);
    }
    else if ( strmatch(tok,"OS2_WeightWidthSlopeOnly:")==0 )
    {
	int temp;
	getint(sfd,&temp);
	sf->weight_width_slope_only = temp;
    }
    else if ( strmatch(tok,"OS2_UseTypoMetrics:")==0 )
    {
	int temp;
	getint(sfd,&temp);
	sf->use_typo_metrics = temp;
    }
    else if ( strmatch(tok,"UseUniqueID:")==0 )
    {
	int temp;
	getint(sfd,&temp);
	sf->use_uniqueid = temp;
    }
    else if ( strmatch(tok,"UseXUID:")==0 )
    {
	int temp;
	getint(sfd,&temp);
	sf->use_xuid = temp;
    }
    else if ( strmatch(tok,"UniqueID:")==0 )
    {
	getint(sfd,&sf->uniqueid);
    }
    else if ( strmatch(tok,"XUID:")==0 )
    {
	geteol(sfd,tok);
	sf->xuid = copy(tok);
    }
    else if ( strmatch(tok,"Lookup:")==0 )
    {
	OTLookup *otl;
	int temp;
	if ( sf->sfd_version<2 ) {
	    IError( "Lookups should not happen in version 1 sfd files." );
	    exit(1);
	}
	otl = chunkalloc(sizeof(OTLookup));
	getint(sfd,&temp); otl->lookup_type = temp;
	getint(sfd,&temp); otl->lookup_flags = temp;
	getint(sfd,&temp); otl->store_in_afm = temp;
	otl->lookup_name = SFDReadUTF7Str(sfd);
	if ( otl->lookup_type<gpos_single ) {
	    if ( d->lastsotl==NULL )
		sf->gsub_lookups = otl;
	    else
		d->lastsotl->next = otl;
	    d->lastsotl = otl;
	} else {
	    if ( d->lastpotl==NULL )
		sf->gpos_lookups = otl;
	    else
		d->lastpotl->next = otl;
	    d->lastpotl = otl;
	}
	SFDParseLookup(sfd,otl);
    }
    else if ( strmatch(tok,"MarkAttachClasses:")==0 )
    {
	getint(sfd,&sf->mark_class_cnt);
	sf->mark_classes = malloc(sf->mark_class_cnt*sizeof(char *));
	sf->mark_class_names = malloc(sf->mark_class_cnt*sizeof(char *));
	sf->mark_classes[0] = NULL; sf->mark_class_names[0] = NULL;
	for ( i=1; i<sf->mark_class_cnt; ++i )
	{
	    /* Class 0 is unused */
	    int temp;
	    while ( (temp=nlgetc(sfd))=='\n' || temp=='\r' ); ungetc(temp,sfd);
	    sf->mark_class_names[i] = SFDReadUTF7Str(sfd);
	    getint(sfd,&temp);
	    sf->mark_classes[i] = malloc(temp+1); sf->mark_classes[i][temp] = '\0';
	    nlgetc(sfd);	/* skip space */
	    fread(sf->mark_classes[i],1,temp,sfd);
	}
    }
    else if ( strmatch(tok,"MarkAttachSets:")==0 )
    {
	getint(sfd,&sf->mark_set_cnt);
	sf->mark_sets = malloc(sf->mark_set_cnt*sizeof(char *));
	sf->mark_set_names = malloc(sf->mark_set_cnt*sizeof(char *));
	for ( i=0; i<sf->mark_set_cnt; ++i )
	{
	    /* Set 0 is used */
	    int temp;
	    while ( (temp=nlgetc(sfd))=='\n' || temp=='\r' ); ungetc(temp,sfd);
	    sf->mark_set_names[i] = SFDReadUTF7Str(sfd);
	    getint(sfd,&temp);
	    sf->mark_sets[i] = malloc(temp+1); sf->mark_sets[i][temp] = '\0';
	    nlgetc(sfd);	/* skip space */
	    fread(sf->mark_sets[i],1,temp,sfd);
	}
    }
    else if ( strmatch(tok,"KernClass2:")==0 || strmatch(tok,"VKernClass2:")==0 ||
	      strmatch(tok,"KernClass:")==0 || strmatch(tok,"VKernClass:")==0 ||
	      strmatch(tok,"KernClass3:")==0 || strmatch(tok,"VKernClass3:")==0 )
    {
	int kernclassversion = 0;
	int isv = tok[0]=='V';
	int kcvoffset = (isv ? 10 : 9); //Offset to read kerning class version
	if (isdigit(tok[kcvoffset])) kernclassversion = tok[kcvoffset] - '0';
	int temp, classstart=1;
	int old = (kernclassversion == 0);

	if ( (sf->sfd_version<2)!=old ) {
	    IError( "Version mixup in Kerning Classes of sfd file." );
	    exit(1);
	}
	kc = chunkalloc(old ? sizeof(KernClass1) : sizeof(KernClass));
	getint(sfd,&kc->first_cnt);
	ch=nlgetc(sfd);
	if ( ch=='+' )
	    classstart = 0;
	else
	    ungetc(ch,sfd);
	getint(sfd,&kc->second_cnt);
	if ( old ) {
	    getint(sfd,&temp); ((KernClass1 *) kc)->sli = temp;
	    getint(sfd,&temp); ((KernClass1 *) kc)->flags = temp;
	} else {
	    kc->subtable = SFFindLookupSubtableAndFreeName(sf,SFDReadUTF7Str(sfd));
	    if ( kc->subtable!=NULL && kc->subtable->kc==NULL )
		kc->subtable->kc = kc;
	    else {
		if ( kc->subtable==NULL )
		    LogError(_("Bad SFD file, missing subtable in kernclass defn.\n") );
		else
		    LogError(_("Bad SFD file, two kerning classes assigned to the same subtable: %s\n"), kc->subtable->subtable_name );
		kc->subtable = NULL;
	    }
	}
	kc->firsts = calloc(kc->first_cnt,sizeof(char *));
	kc->seconds = calloc(kc->second_cnt,sizeof(char *));
	kc->offsets = calloc(kc->first_cnt*kc->second_cnt,sizeof(int16));
	kc->adjusts = calloc(kc->first_cnt*kc->second_cnt,sizeof(DeviceTable));
	if (kernclassversion >= 3) {
	  kc->firsts_flags = calloc(kc->first_cnt, sizeof(int));
	  kc->seconds_flags = calloc(kc->second_cnt, sizeof(int));
	  kc->offsets_flags = calloc(kc->first_cnt*kc->second_cnt, sizeof(int));
	  kc->firsts_names = calloc(kc->first_cnt, sizeof(char*));
	  kc->seconds_names = calloc(kc->second_cnt, sizeof(char*));
	}
	kc->firsts[0] = NULL;
	for ( i=classstart; i<kc->first_cnt; ++i ) {
	  if (kernclassversion < 3) {
	    getint(sfd,&temp);
	    kc->firsts[i] = malloc(temp+1); kc->firsts[i][temp] = '\0';
	    nlgetc(sfd);	/* skip space */
	    fread(kc->firsts[i],1,temp,sfd);
	  } else {
	    getint(sfd,&kc->firsts_flags[i]);
	    while ((ch=nlgetc(sfd)) == ' '); ungetc(ch, sfd); if (ch == '\n' || ch == EOF) continue;
	    kc->firsts_names[i] = SFDReadUTF7Str(sfd);
	    while ((ch=nlgetc(sfd)) == ' '); ungetc(ch, sfd); if (ch == '\n' || ch == EOF) continue;
	    kc->firsts[i] = SFDReadUTF7Str(sfd);
            if (kc->firsts[i] == NULL) kc->firsts[i] = copy(""); // In certain places, this must be defined.
	    while ((ch=nlgetc(sfd)) == ' ' || ch == '\n'); ungetc(ch, sfd);
	  }
	}
	kc->seconds[0] = NULL;
	for ( i=1; i<kc->second_cnt; ++i ) {
	  if (kernclassversion < 3) {
	    getint(sfd,&temp);
	    kc->seconds[i] = malloc(temp+1); kc->seconds[i][temp] = '\0';
	    nlgetc(sfd);	/* skip space */
	    fread(kc->seconds[i],1,temp,sfd);
	  } else {
	    getint(sfd,&temp);
	    kc->seconds_flags[i] = temp;
	    while ((ch=nlgetc(sfd)) == ' '); ungetc(ch, sfd); if (ch == '\n' || ch == EOF) continue;
	    kc->seconds_names[i] = SFDReadUTF7Str(sfd);
	    while ((ch=nlgetc(sfd)) == ' '); ungetc(ch, sfd); if (ch == '\n' || ch == EOF) continue;
	    kc->seconds[i] = SFDReadUTF7Str(sfd);
            if (kc->seconds[i] == NULL) kc->seconds[i] = copy(""); // In certain places, this must be defined.
	    while ((ch=nlgetc(sfd)) == ' ' || ch == '\n'); ungetc(ch, sfd);
	  }
	}
	for ( i=0; i<kc->first_cnt*kc->second_cnt; ++i ) {
	  if (kernclassversion >= 3) {
	    getint(sfd,&temp);
	    kc->offsets_flags[i] = temp;
	  }
	    getint(sfd,&temp);
	    kc->offsets[i] = temp;
	    SFDReadDeviceTable(sfd,&kc->adjusts[i]);
	}
	if ( !old && kc->subtable == NULL ) {
	    /* Error. Ignore it. Free it. Whatever */;
	} else if ( !isv ) {
	    if ( d->lastkc==NULL )
		sf->kerns = kc;
	    else
		d->lastkc->next = kc;
	    d->lastkc = kc;
	} else {
	    if ( d->lastvkc==NULL )
		sf->vkerns = kc;
	    else
		d->lastvkc->next = kc;
	    d->lastvkc = kc;
	}
    }
    else if ( strmatch(tok,"ContextPos2:")==0 || strmatch(tok,"ContextSub2:")==0 ||
	      strmatch(tok,"ChainPos2:")==0 || strmatch(tok,"ChainSub2:")==0 ||
	      strmatch(tok,"ReverseChain2:")==0 ||
	      strmatch(tok,"ContextPos:")==0 || strmatch(tok,"ContextSub:")==0 ||
	      strmatch(tok,"ChainPos:")==0 || strmatch(tok,"ChainSub:")==0 ||
	      strmatch(tok,"ReverseChain:")==0 )
    {
	FPST *fpst;
	int old;
	if ( strchr(tok,'2')!=NULL ) {
	    old = false;
	    fpst = chunkalloc(sizeof(FPST));
	} else {
	    old = true;
	    fpst = chunkalloc(sizeof(FPST1));
	}
	if ( (sf->sfd_version<2)!=old ) {
	    IError( "Version mixup in FPST of sfd file." );
	    exit(1);
	}
	if ( d->lastfp==NULL )
	    sf->possub = fpst;
	else
	    d->lastfp->next = fpst;
	d->lastfp = fpst;
	SFDParseChainContext(sfd,sf,fpst,tok,old);
    }
    else if ( strmatch(tok,"Group:")==0 ) {
        struct ff_glyphclasses *grouptmp = calloc(1, sizeof(struct ff_glyphclasses));
        while ((ch=nlgetc(sfd)) == ' '); ungetc(ch, sfd);
        grouptmp->classname = SFDReadUTF7Str(sfd);
        while ((ch=nlgetc(sfd)) == ' '); ungetc(ch, sfd);
        grouptmp->glyphs = SFDReadUTF7Str(sfd);
        while ((ch=nlgetc(sfd)) == ' ' || ch == '\n'); ungetc(ch, sfd);
        if (d->lastgroup != NULL) d->lastgroup->next = grouptmp; else sf->groups = grouptmp;
        d->lastgroup = grouptmp;
    }
    else if ( strmatch(tok,"GroupKern:")==0 ) {
        int temp = 0;
        struct ff_rawoffsets *kerntmp = calloc(1, sizeof(struct ff_rawoffsets));
        while ((ch=nlgetc(sfd)) == ' '); ungetc(ch, sfd);
        kerntmp->left = SFDReadUTF7Str(sfd);
        while ((ch=nlgetc(sfd)) == ' '); ungetc(ch, sfd);
        kerntmp->right = SFDReadUTF7Str(sfd);
        while ((ch=nlgetc(sfd)) == ' '); ungetc(ch, sfd);
        getint(sfd,&temp);
        kerntmp->offset = temp;
        while ((ch=nlgetc(sfd)) == ' ' || ch == '\n'); ungetc(ch, sfd);
        if (d->lastgroupkern != NULL) d->lastgroupkern->next = kerntmp; else sf->groupkerns = kerntmp;
        d->lastgroupkern = kerntmp;
    }
    else if ( strmatch(tok,"GroupVKern:")==0 ) {
        int temp = 0;
        struct ff_rawoffsets *kerntmp = calloc(1, sizeof(struct ff_rawoffsets));
        while ((ch=nlgetc(sfd)) == ' '); ungetc(ch, sfd);
        kerntmp->left = SFDReadUTF7Str(sfd);
        while ((ch=nlgetc(sfd)) == ' '); ungetc(ch, sfd);
        kerntmp->right = SFDReadUTF7Str(sfd);
        while ((ch=nlgetc(sfd)) == ' '); ungetc(ch, sfd);
        getint(sfd,&temp);
        kerntmp->offset = temp;
        while ((ch=nlgetc(sfd)) == ' ' || ch == '\n'); ungetc(ch, sfd);
        if (d->lastgroupvkern != NULL) d->lastgroupvkern->next = kerntmp; else sf->groupvkerns = kerntmp;
        d->lastgroupvkern = kerntmp;
    }
    else if ( strmatch(tok,"MacIndic2:")==0 || strmatch(tok,"MacContext2:")==0 ||
	      strmatch(tok,"MacLigature2:")==0 || strmatch(tok,"MacSimple2:")==0 ||
	      strmatch(tok,"MacKern2:")==0 || strmatch(tok,"MacInsert2:")==0 ||
	      strmatch(tok,"MacIndic:")==0 || strmatch(tok,"MacContext:")==0 ||
	      strmatch(tok,"MacLigature:")==0 || strmatch(tok,"MacSimple:")==0 ||
	      strmatch(tok,"MacKern:")==0 || strmatch(tok,"MacInsert:")==0 )
    {
	ASM *sm;
	if ( strchr(tok,'2')!=NULL ) {
	    old = false;
	    sm = chunkalloc(sizeof(ASM));
	} else {
	    old = true;
	    sm = chunkalloc(sizeof(ASM1));
	}
	if ( (sf->sfd_version<2)!=old ) {
	    IError( "Version mixup in state machine of sfd file." );
	    exit(1);
	}
	if ( d->lastsm==NULL )
	    sf->sm = sm;
	else
	    d->lastsm->next = sm;
	d->lastsm = sm;
	SFDParseStateMachine(sfd,sf,sm,tok,old);
    }
    else if ( strmatch(tok,"MacFeat:")==0 )
    {
	sf->features = SFDParseMacFeatures(sfd,tok);
    }
    else if ( strmatch(tok,"TtfTable:")==0 )
    {
	/* Old, binary format */
	/* still used for maxp and unknown tables */
	SFDGetTtfTable(sfd,sf,d->lastttf);
    }
    else if ( strmatch(tok,"TtTable:")==0 )
    {
	/* text instruction format */
	SFDGetTtTable(sfd,sf,d->lastttf);
    }


    ///////////////////

    else if ( strmatch(tok,"ShortTable:")==0 )
    {
	// only read, not written.
	/* text number format */
	SFDGetShortTable(sfd,sf,d->lastttf);
    }
    else
    {
        //
        // We didn't have a match ourselves.
        //
        return false;
    }
    return true;
}

void SFD_GetFontMetaDataVoid( FILE *sfd,
			  char *tok,
			  SplineFont *sf,
			  void* d ) {
  SFD_GetFontMetaData(sfd, tok, sf, d);
}

static SplineFont *SFD_GetFont( FILE *sfd,SplineFont *cidmaster,char *tok,
				int fromdir, char *dirname, float sfdversion )
{
    SplineFont *sf;
    int realcnt, i, eof, mappos=-1, ch;
    struct table_ordering *lastord = NULL;
    struct axismap *lastaxismap = NULL;
    struct named_instance *lastnamedinstance = NULL;
    int pushedbacktok = false;
    Encoding *enc = &custom;
    struct remap *remap = NULL;
    int haddupenc;
    int old_style_order2 = false;
    int had_layer_cnt=false;

    orig_pos = 0;		/* Only used for compatibility with extremely old sfd files */

    sf = SplineFontEmpty();
    if ( sfdversion>0 && sfdversion<2 ) {
	/* If it's an old style sfd file with old style features we need some */
	/*  extra data space to do the conversion from old to new */
	sf = realloc(sf,sizeof(SplineFont1));
	memset(((uint8 *) sf) + sizeof(SplineFont),0,sizeof(SplineFont1)-sizeof(SplineFont));
    }
    sf->sfd_version = sfdversion;
    sf->cidmaster = cidmaster;
    sf->uni_interp = ui_unset;
	SFD_GetFontMetaDataData d;
	SFD_GetFontMetaDataData_Init( &d );
    while ( 1 ) {
	if ( pushedbacktok )
	    pushedbacktok = false;
	else if ( (eof = getname(sfd,tok))!=1 ) {
	    if ( eof==-1 )
    break;
	    geteol(sfd,tok);
    continue;
	}


	bool wasMetadata = SFD_GetFontMetaData( sfd, tok, sf, &d );
	had_layer_cnt = d.had_layer_cnt;
        if( wasMetadata )
        {
            // we have handled the token entirely
            // inside SFD_GetFontMetaData() move to next token.
            continue;
        }
        
        
	if ( strmatch(tok,"DisplaySize:")==0 )
	{
	    getint(sfd,&sf->display_size);
	}
	else if ( strmatch(tok,"DisplayLayer:")==0 )
	{
	    getint(sfd,&sf->display_layer);
	}
	else if ( strmatch(tok,"ExtremaBound:")==0 )
	{
	    getint(sfd,&sf->extrema_bound);
	}
	else if ( strmatch(tok,"WidthSeparation:")==0 )
	{
	    getint(sfd,&sf->width_separation);
	}
	else if ( strmatch(tok,"WinInfo:")==0 )
	{
	    int temp1, temp2;
	    getint(sfd,&sf->top_enc);
	    getint(sfd,&temp1);
	    getint(sfd,&temp2);
	    if ( sf->top_enc<=0 ) sf->top_enc=-1;
	    if ( temp1<=0 ) temp1 = 16;
	    if ( temp2<=0 ) temp2 = 4;
	    sf->desired_col_cnt = temp1;
	    sf->desired_row_cnt = temp2;
	}
	else if ( strmatch(tok,"AntiAlias:")==0 )
	{
	    int temp;
	    getint(sfd,&temp);
	    sf->display_antialias = temp;
	}
	else if ( strmatch(tok,"FitToEm:")==0 )
	{
	    int temp;
	    getint(sfd,&temp);
	    sf->display_bbsized = temp;
	}
	else if ( strmatch(tok,"OnlyBitmaps:")==0 )
	{
	    int temp;
	    getint(sfd,&temp);
	    sf->onlybitmaps = temp;
	}
	else if ( strmatch(tok,"Order2:")==0 )
	{
	    getint(sfd,&old_style_order2);
	    sf->grid.order2 = old_style_order2;
	    sf->layers[ly_back].order2 = old_style_order2;
	    sf->layers[ly_fore].order2 = old_style_order2;
	}
	else if ( strmatch(tok,"GridOrder2:")==0 )
	{
	    int o2;
	    getint(sfd,&o2);
	    sf->grid.order2 = o2;
	}
	else if ( strmatch(tok,"Encoding:")==0 )
	{
	    enc = SFDGetEncoding(sfd,tok);
	    if ( sf->map!=NULL ) sf->map->enc = enc;
	}
	else if ( strmatch(tok,"OldEncoding:")==0 )
	{
	    /* old_encname =*/ (void) SFDGetEncoding(sfd,tok);
	}
	else if ( strmatch(tok,"UnicodeInterp:")==0 )
	{
	    sf->uni_interp = SFDGetUniInterp(sfd,tok,sf);
	}
	else if ( strmatch(tok,"NameList:")==0 )
	{
	    SFDGetNameList(sfd,tok,sf);
	}
	else if ( strmatch(tok,"Compacted:")==0 )
	{
	    int temp;
	    getint(sfd,&temp);
	    sf->compacted = temp;
	}
	else if ( strmatch(tok,"Registry:")==0 )
	{
	    geteol(sfd,tok);
	    sf->cidregistry = copy(tok);
	}


	//////////


	else if ( strmatch(tok,"Ordering:")==0 ) {
	    geteol(sfd,tok);
	    sf->ordering = copy(tok);
	} else if ( strmatch(tok,"Supplement:")==0 ) {
	    getint(sfd,&sf->supplement);
	} else if ( strmatch(tok,"RemapN:")==0 ) {
	    int n;
	    getint(sfd,&n);
	    remap = calloc(n+1,sizeof(struct remap));
	    remap[n].infont = -1;
	    mappos = 0;
	    if ( sf->map!=NULL ) sf->map->remap = remap;
	} else if ( strmatch(tok,"Remap:")==0 ) {
	    uint32 f, l; int p;
	    gethex(sfd,&f);
	    gethex(sfd,&l);
	    getint(sfd,&p);
	    if ( remap!=NULL && remap[mappos].infont!=-1 ) {
		remap[mappos].firstenc = f;
		remap[mappos].lastenc = l;
		remap[mappos].infont = p;
		mappos++;
	    }
	} else if ( strmatch(tok,"CIDVersion:")==0 ) {
	    real temp;
	    getreal(sfd,&temp);
	    sf->cidversion = temp;
	} else if ( strmatch(tok,"Grid")==0 ) {
	    sf->grid.splines = SFDGetSplineSet(sfd,sf->grid.order2);
	} else if ( strmatch(tok,"ScriptLang:")==0 ) {
	    int i,j,k;
	    int imax, jmax, kmax;
	    if ( sf->sfd_version==0 || sf->sfd_version>=2 ) {
		IError( "Script lang lists should not happen in version 2 sfd files." );
                SplineFontFree(sf);
                return NULL;
	    }
	    getint(sfd,&imax);
	    ((SplineFont1 *) sf)->sli_cnt = imax;
	    ((SplineFont1 *) sf)->script_lang = malloc((imax+1)*sizeof(struct script_record *));
	    ((SplineFont1 *) sf)->script_lang[imax] = NULL;
	    for ( i=0; i<imax; ++i ) {
		getint(sfd,&jmax);
		((SplineFont1 *) sf)->script_lang[i] = malloc((jmax+1)*sizeof(struct script_record));
		((SplineFont1 *) sf)->script_lang[i][jmax].script = 0;
		for ( j=0; j<jmax; ++j ) {
		    ((SplineFont1 *) sf)->script_lang[i][j].script = gettag(sfd);
		    getint(sfd,&kmax);
		    ((SplineFont1 *) sf)->script_lang[i][j].langs = malloc((kmax+1)*sizeof(uint32));
		    ((SplineFont1 *) sf)->script_lang[i][j].langs[kmax] = 0;
		    for ( k=0; k<kmax; ++k ) {
			((SplineFont1 *) sf)->script_lang[i][j].langs[k] = gettag(sfd);
		    }
		}
	    }
	} else if ( strmatch(tok,"TeXData:")==0 ) {
	    int temp;
	    getint(sfd,&temp);
	    sf->texdata.type = temp;
	    getint(sfd, &temp);
	    if ( sf->design_size==0 ) {
	    	sf->design_size = (5*temp+(1<<18))>>19;
	    }
	    for ( i=0; i<22; ++i ) {
		int foo;
		getint(sfd,&foo);
		sf->texdata.params[i]=foo;
	    }
	} else if ( strnmatch(tok,"AnchorClass",11)==0 ) {
	    char *name;
	    AnchorClass *lastan = NULL, *an;
	    int old = strchr(tok,'2')==NULL;
	    while ( (name=SFDReadUTF7Str(sfd))!=NULL ) {
		an = chunkalloc(old ? sizeof(AnchorClass1) : sizeof(AnchorClass));
		an->name = name;
		if ( old ) {
		    getname(sfd,tok);
		    if ( tok[0]=='0' && tok[1]=='\0' )
			((AnchorClass1 *) an)->feature_tag = 0;
		    else {
			if ( tok[1]=='\0' ) { tok[1]=' '; tok[2] = 0; }
			if ( tok[2]=='\0' ) { tok[2]=' '; tok[3] = 0; }
			if ( tok[3]=='\0' ) { tok[3]=' '; tok[4] = 0; }
			((AnchorClass1 *) an)->feature_tag = (tok[0]<<24) | (tok[1]<<16) | (tok[2]<<8) | tok[3];
		    }
		    while ( (ch=nlgetc(sfd))==' ' || ch=='\t' );
		    ungetc(ch,sfd);
		    if ( isdigit(ch)) {
			int temp;
			getint(sfd,&temp);
			((AnchorClass1 *) an)->flags = temp;
		    }
		    while ( (ch=nlgetc(sfd))==' ' || ch=='\t' );
		    ungetc(ch,sfd);
		    if ( isdigit(ch)) {
			int temp;
			getint(sfd,&temp);
			((AnchorClass1 *) an)->script_lang_index = temp;
		    } else
			((AnchorClass1 *) an)->script_lang_index = 0xffff;		/* Will be fixed up later */
		    while ( (ch=nlgetc(sfd))==' ' || ch=='\t' );
		    ungetc(ch,sfd);
		    if ( isdigit(ch)) {
			int temp;
			getint(sfd,&temp);
			((AnchorClass1 *) an)->merge_with = temp;
		    } else
			((AnchorClass1 *) an)->merge_with = 0xffff;			/* Will be fixed up later */
		} else {
                    char *subtable_name = SFDReadUTF7Str(sfd);
                    if ( subtable_name!=NULL)                                           /* subtable is optional */
		        an->subtable = SFFindLookupSubtableAndFreeName(sf,subtable_name);
                }
		while ( (ch=nlgetc(sfd))==' ' || ch=='\t' );
		ungetc(ch,sfd);
		if ( isdigit(ch) ) {
		    /* Early versions of SfdFormat 2 had a number here */
		    int temp;
		    getint(sfd,&temp);
		    an->type = temp;
		} else if ( old ) {
		    if ( ((AnchorClass1 *) an)->feature_tag==CHR('c','u','r','s'))
			an->type = act_curs;
		    else if ( ((AnchorClass1 *) an)->feature_tag==CHR('m','k','m','k'))
			an->type = act_mkmk;
		    else
			an->type = act_mark;
		} else {
		    an->type = act_mark;
		    if( an->subtable && an->subtable->lookup )
		    {
			switch ( an->subtable->lookup->lookup_type )
			{
			case gpos_cursive:
			    an->type = act_curs;
			    break;
			case gpos_mark2base:
			    an->type = act_mark;
			    break;
			case gpos_mark2ligature:
			    an->type = act_mklg;
			    break;
			case gpos_mark2mark:
			    an->type = act_mkmk;
			    break;
			default:
			    an->type = act_mark;
			    break;
			}
		    }
		}
		if ( lastan==NULL )
		    sf->anchor = an;
		else
		    lastan->next = an;
		lastan = an;
	    }
	} else if ( strncmp(tok,"MATH:",5)==0 ) {
	    SFDParseMathItem(sfd,sf,tok);
	} else if ( strmatch(tok,"TableOrder:")==0 ) {
	    int temp;
	    struct table_ordering *ord;
	    if ( sfdversion==0 || sfdversion>=2 ) {
		IError("Table ordering specified in version 2 sfd file.\n" );
                SplineFontFree(sf);
                return NULL;
	    }
	    ord = chunkalloc(sizeof(struct table_ordering));
	    ord->table_tag = gettag(sfd);
	    getint(sfd,&temp);
	    ord->ordered_features = malloc((temp+1)*sizeof(uint32));
	    ord->ordered_features[temp] = 0;
	    for ( i=0; i<temp; ++i ) {
		while ( isspace((ch=nlgetc(sfd))) );
		if ( ch=='\'' ) {
		    ungetc(ch,sfd);
		    ord->ordered_features[i] = gettag(sfd);
		} else if ( ch=='<' ) {
		    int f,s;
		    fscanf(sfd,"%d,%d>", &f, &s );
		    ord->ordered_features[i] = (f<<16)|s;
		}
	    }
	    if ( lastord==NULL )
		((SplineFont1 *) sf)->orders = ord;
	    else
		lastord->next = ord;
	    lastord = ord;
	} else if ( strmatch(tok,"BeginPrivate:")==0 ) {
	    SFDGetPrivate(sfd,sf);
	} else if ( strmatch(tok,"BeginSubrs:")==0 ) {	/* leave in so we don't croak on old sfd files */
	    SFDGetSubrs(sfd);
	} else if ( strmatch(tok,"PickledData:")==0 ) {
	    if (sf->python_persistent != NULL) {
#if defined(_NO_PYTHON)
	      free( sf->python_persistent );	/* It's a string of pickled data which we leave as a string */
#else
	      PyFF_FreePythonPersistent(sf->python_persistent);
#endif
	      sf->python_persistent = NULL;
	    }
	    sf->python_persistent = SFDUnPickle(sfd, 0);
	    sf->python_persistent_has_lists = 0;
	} else if ( strmatch(tok,"PickledDataWithLists:")==0 ) {
	    if (sf->python_persistent != NULL) {
#if defined(_NO_PYTHON)
	      free( sf->python_persistent );	/* It's a string of pickled data which we leave as a string */
#else
	      PyFF_FreePythonPersistent(sf->python_persistent);
#endif
	      sf->python_persistent = NULL;
	    }
	    sf->python_persistent = SFDUnPickle(sfd, 1);
	    sf->python_persistent_has_lists = 1;
	} else if ( strmatch(tok,"MMCounts:")==0 ) {
	    MMSet *mm = sf->mm = chunkalloc(sizeof(MMSet));
	    getint(sfd,&mm->instance_count);
	    getint(sfd,&mm->axis_count);
	    ch = nlgetc(sfd);
	    if ( ch!=' ' )
		ungetc(ch,sfd);
	    else { int temp;
		getint(sfd,&temp);
		mm->apple = temp;
		getint(sfd,&mm->named_instance_count);
	    }
	    mm->instances = calloc(mm->instance_count,sizeof(SplineFont *));
	    mm->positions = malloc(mm->instance_count*mm->axis_count*sizeof(real));
	    mm->defweights = malloc(mm->instance_count*sizeof(real));
	    mm->axismaps = calloc(mm->axis_count,sizeof(struct axismap));
	    if ( mm->named_instance_count!=0 )
		mm->named_instances = calloc(mm->named_instance_count,sizeof(struct named_instance));
	} else if ( strmatch(tok,"MMAxis:")==0 ) {
	    MMSet *mm = sf->mm;
	    if ( mm!=NULL ) {
		for ( i=0; i<mm->axis_count; ++i ) {
		    getname(sfd,tok);
		    mm->axes[i] = copy(tok);
		}
	    }
	} else if ( strmatch(tok,"MMPositions:")==0 ) {
	    MMSet *mm = sf->mm;
	    if ( mm!=NULL ) {
		for ( i=0; i<mm->axis_count*mm->instance_count; ++i )
		    getreal(sfd,&mm->positions[i]);
	    }
	} else if ( strmatch(tok,"MMWeights:")==0 ) {
	    MMSet *mm = sf->mm;
	    if ( mm!=NULL ) {
		for ( i=0; i<mm->instance_count; ++i )
		    getreal(sfd,&mm->defweights[i]);
	    }
	} else if ( strmatch(tok,"MMAxisMap:")==0 ) {
	    MMSet *mm = sf->mm;
	    if ( mm!=NULL ) {
		int index, points;
		getint(sfd,&index); getint(sfd,&points);
		mm->axismaps[index].points = points;
		mm->axismaps[index].blends = malloc(points*sizeof(real));
		mm->axismaps[index].designs = malloc(points*sizeof(real));
		for ( i=0; i<points; ++i ) {
		    getreal(sfd,&mm->axismaps[index].blends[i]);
		    while ( (ch=nlgetc(sfd))!=EOF && isspace(ch));
		    ungetc(ch,sfd);
		    if ( (ch=nlgetc(sfd))!='=' )
			ungetc(ch,sfd);
		    else if ( (ch=nlgetc(sfd))!='>' )
			ungetc(ch,sfd);
		    getreal(sfd,&mm->axismaps[index].designs[i]);
		}
		lastaxismap = &mm->axismaps[index];
		lastnamedinstance = NULL;
	    }
	} else if ( strmatch(tok,"MMNamedInstance:")==0 ) {
	    MMSet *mm = sf->mm;
	    if ( mm!=NULL ) {
		int index;
		getint(sfd,&index);
		mm->named_instances[index].coords = malloc(mm->axis_count*sizeof(real));
		for ( i=0; i<mm->axis_count; ++i )
		    getreal(sfd,&mm->named_instances[index].coords[i]);
		lastnamedinstance = &mm->named_instances[index];
		lastaxismap = NULL;
	    }
	} else if ( strmatch(tok,"MacName:")==0 ) {
	    struct macname *names = SFDParseMacNames(sfd,tok);
	    if ( lastaxismap!=NULL )
		lastaxismap->axisnames = names;
	    else if ( lastnamedinstance !=NULL )
		lastnamedinstance->names = names;
	    pushedbacktok = true;
	} else if ( strmatch(tok,"MMCDV:")==0 ) {
	    MMSet *mm = sf->mm;
	    if ( mm!=NULL )
		mm->cdv = SFDParseMMSubroutine(sfd);
	} else if ( strmatch(tok,"MMNDV:")==0 ) {
	    MMSet *mm = sf->mm;
	    if ( mm!=NULL )
		mm->ndv = SFDParseMMSubroutine(sfd);
	} else if ( strmatch(tok,"BeginMMFonts:")==0 ) {
	    int cnt;
	    getint(sfd,&cnt);
	    getint(sfd,&realcnt);
	    ff_progress_change_stages(cnt);
	    ff_progress_change_total(realcnt);
	    MMInferStuff(sf->mm);
    break;
	} else if ( strmatch(tok,"BeginSubFonts:")==0 ) {
	    getint(sfd,&sf->subfontcnt);
	    sf->subfonts = calloc(sf->subfontcnt,sizeof(SplineFont *));
	    getint(sfd,&realcnt);
	    sf->map = EncMap1to1(realcnt);
	    ff_progress_change_stages(2);
	    ff_progress_change_total(realcnt);
    break;
	} else if ( strmatch(tok,"BeginChars:")==0 ) {
	    int charcnt;
	    getint(sfd,&charcnt);
	    if (charcnt<enc->char_cnt) {
		IError("SFD file specifies too few slots for its encoding.\n" );
exit( 1 );
	    }
	    if ( getint(sfd,&realcnt)!=1 || realcnt==-1 )
		realcnt = charcnt;
	    else
		++realcnt;		/* value saved is max glyph, not glyph cnt */
	    ff_progress_change_total(realcnt);
	    sf->glyphcnt = sf->glyphmax = realcnt;
	    sf->glyphs = calloc(realcnt,sizeof(SplineChar *));
	    if ( cidmaster!=NULL ) {
		sf->map = cidmaster->map;
	    } else {
		sf->map = EncMapNew(charcnt,realcnt,enc);
		sf->map->remap = remap;
	    }
	    SFDSizeMap(sf->map,sf->glyphcnt,charcnt);
    break;
#if HANYANG
	} else if ( strmatch(tok,"BeginCompositionRules")==0 ) {
	    sf->rules = SFDReadCompositionRules(sfd);
#endif
	} else {
	    /* If we don't understand it, skip it */
	    geteol(sfd,tok);
	}
    }

    if ( fromdir )
	sf = SFD_FigureDirType(sf,tok,dirname,enc,remap,had_layer_cnt);
    else if ( sf->subfontcnt!=0 ) {
	ff_progress_change_stages(2*sf->subfontcnt);
	for ( i=0; i<sf->subfontcnt; ++i ) {
	    if ( i!=0 )
		ff_progress_next_stage();
	    sf->subfonts[i] = SFD_GetFont(sfd,sf,tok,fromdir,dirname,sfdversion);
	}
    } else if ( sf->mm!=NULL ) {
	MMSet *mm = sf->mm;
	ff_progress_change_stages(2*(mm->instance_count+1));
	for ( i=0; i<mm->instance_count; ++i ) {
	    if ( i!=0 )
		ff_progress_next_stage();
	    mm->instances[i] = SFD_GetFont(sfd,NULL,tok,fromdir,dirname,sfdversion);
	    EncMapFree(mm->instances[i]->map); mm->instances[i]->map=NULL;
	    mm->instances[i]->mm = mm;
	}
	ff_progress_next_stage();
	mm->normal = SFD_GetFont(sfd,NULL,tok,fromdir,dirname,sfdversion);
	mm->normal->mm = mm;
	sf->mm = NULL;
	SplineFontFree(sf);
	sf = mm->normal;
	if ( sf->map->enc!=&custom ) {
	    EncMap *map;
	    MMMatchGlyphs(mm);		/* sfd files from before the encoding change can have mismatched orig pos */
	    map = EncMapFromEncoding(sf,sf->map->enc);
	    EncMapFree(sf->map);
	    sf->map = map;
	}
    } else {
	while ( SFDGetChar(sfd,sf,had_layer_cnt)!=NULL ) {
	    ff_progress_next();
	}
	ff_progress_next_stage();
    }
    haddupenc = false;
    while ( getname(sfd,tok)==1 ) {
	if ( strcmp(tok,"EndSplineFont")==0 || strcmp(tok,"EndSubSplineFont")==0 )
    break;
	else if ( strcmp(tok,"BitmapFont:")==0 )
	    SFDGetBitmapFont(sfd,sf,false,NULL);
	else if ( strmatch(tok,"DupEnc:")==0 ) {
	    int enc, orig;
	    haddupenc = true;
	    if ( getint(sfd,&enc) && getint(sfd,&orig) && sf->map!=NULL ) {
		SFDSetEncMap(sf,orig,enc);
	    }
	}
    }
    if ( sf->cidmaster==NULL )
	SFDFixupRefs(sf);

    if ( !haddupenc )
	SFD_DoAltUnis(sf);
    else
	AltUniFigure(sf,sf->map,true);
    if ( sf->sfd_version<2 )
	SFD_AssignLookups((SplineFont1 *) sf);
    if ( !d.hadtimes )
	SFTimesFromFile(sf,sfd);
    // Make a blank encoding if there are no characters so as to avoid crashes later.
    if (sf->map == NULL) sf->map = EncMapNew(sf->glyphcnt,sf->glyphcnt,&custom);

    SFDFixupUndoRefs(sf);
return( sf );
}

void SFTimesFromFile(SplineFont *sf,FILE *file) {
    struct stat b;
    if ( fstat(fileno(file),&b)!=-1 ) {
	sf->modificationtime = GetST_MTime(b);
	sf->creationtime = GetST_MTime(b);
    }
}

static double SFDStartsCorrectly(FILE *sfd,char *tok) {
    real dval;
    int ch;

    if ( getname(sfd,tok)!=1 )
return( -1 );
    if ( strcmp(tok,"SplineFontDB:")!=0 )
return( -1 );
    if ( getreal(sfd,&dval)!=1 )
return( -1 );
    /* We don't yet generate version 4 of sfd. It will contain backslash */
    /*  newline in the middle of very long lines. I've put in code to parse */
    /*  this sequence, but I don't yet generate it. I want the parser to */
    /*  perculate through to users before I introduce the new format so there */
    /*  will be fewer complaints when it happens */
    // MIQ: getreal() can give some funky rounding errors it seems
    if ( dval!=0 && dval!=1 && dval!=2.0 && dval!=3.0
         && !(dval > 3.09 && dval <= 3.11)
         && dval!=4.0 )
    {
        LogError("Bad SFD Version number %.1f", dval );
return( -1 );
    }
    ch = nlgetc(sfd); ungetc(ch,sfd);
    if ( ch!='\r' && ch!='\n' )
return( -1 );

return( dval );
}

static SplineFont *SFD_Read(char *filename,FILE *sfd, int fromdir) {
    SplineFont *sf=NULL;
    char tok[2000];
    double version;

    if ( sfd==NULL ) {
	if ( fromdir ) {
	    snprintf(tok,sizeof(tok),"%s/" FONT_PROPS, filename );
	    sfd = fopen(tok,"r");
	} else
	    sfd = fopen(filename,"r");
    }
    if ( sfd==NULL )
return( NULL );
    locale_t tmplocale; locale_t oldlocale; // Declare temporary locale storage.
    switch_to_c_locale(&tmplocale, &oldlocale); // Switch to the C locale temporarily and cache the old locale.
    ff_progress_change_stages(2);
    if ( (version = SFDStartsCorrectly(sfd,tok))!=-1 )
	sf = SFD_GetFont(sfd,NULL,tok,fromdir,filename,version);
    switch_to_old_locale(&tmplocale, &oldlocale); // Switch to the cached locale.
    if ( sf!=NULL ) {
	sf->filename = copy(filename);
	if ( sf->mm!=NULL ) {
	    int i;
	    for ( i=0; i<sf->mm->instance_count; ++i )
		sf->mm->instances[i]->filename = copy(filename);
	} else if ( !sf->onlybitmaps ) {
/* Jonathyn Bet'nct points out that once you edit in an outline window, even */
/*  if by mistake, your onlybitmaps status is gone for good */
/* Regenerate it if the font has no splines, refs, etc. */
	    int i;
	    SplineChar *sc;
	    for ( i=sf->glyphcnt-1; i>=0; --i )
		if ( (sc = sf->glyphs[i])!=NULL &&
			(sc->layer_cnt!=2 ||
			 sc->layers[ly_fore].splines!=NULL ||
			 sc->layers[ly_fore].refs!=NULL ))
	     break;
	     if ( i==-1 )
		 sf->onlybitmaps = true;
	}
    }
    fclose(sfd);
return( sf );
}

SplineFont *SFDRead(char *filename) {
return( SFD_Read(filename,NULL,false));
}

SplineFont *_SFDRead(char *filename,FILE *sfd) {
return( SFD_Read(filename,sfd,false));
}

SplineFont *SFDirRead(char *filename) {
return( SFD_Read(filename,NULL,true));
}

SplineChar *SFDReadOneChar(SplineFont *cur_sf,const char *name) {
    FILE *sfd;
    SplineChar *sc=NULL;
    char oldloc[25], tok[2000];
    uint32 pos;
    SplineFont sf;
    LayerInfo layers[2];
    double version;
    int had_layer_cnt=false;
    int chars_seen = false;

    if ( cur_sf->save_to_dir ) {
	snprintf(tok,sizeof(tok),"%s/" FONT_PROPS,cur_sf->filename);
	sfd = fopen(tok,"r");
    } else
	sfd = fopen(cur_sf->filename,"r");
    if ( sfd==NULL )
return( NULL );
    locale_t tmplocale; locale_t oldlocale; // Declare temporary locale storage.
    switch_to_c_locale(&tmplocale, &oldlocale); // Switch to the C locale temporarily and cache the old locale.

    memset(&sf,0,sizeof(sf));
    memset(&layers,0,sizeof(layers));
    sf.layer_cnt = 2;
    sf.layers = layers;
    sf.ascent = 800; sf.descent = 200;
    if ( cur_sf->cidmaster ) cur_sf = cur_sf->cidmaster;
    if ( (version = SFDStartsCorrectly(sfd,tok))>=2 ) {
	sf.sfd_version = version;
	sf.gpos_lookups = cur_sf->gpos_lookups;
	sf.gsub_lookups = cur_sf->gsub_lookups;
	sf.anchor = cur_sf->anchor;
	pos = ftell(sfd);
	while ( getname(sfd,tok)!=-1 ) {
	    if ( strcmp(tok,"StartChar:")==0 ) {
		if ( getname(sfd,tok)==1 && strcmp(tok,name)==0 ) {
		    fseek(sfd,pos,SEEK_SET);
		    sc = SFDGetChar(sfd,&sf,had_layer_cnt);
	break;
		}
	    } else if ( strmatch(tok,"BeginChars:")==0 ) {
		chars_seen = true;
	    } else if ( chars_seen ) {
		/* Don't try to look for things in the file header any more */
		/* The "Layer" keyword has a different meaning in this context */
	    } else if ( strmatch(tok,"Order2:")==0 ) {
		int order2;
		getint(sfd,&order2);
		sf.grid.order2 = order2;
		sf.layers[ly_back].order2 = order2;
		sf.layers[ly_fore].order2 = order2;
	    } else if ( strmatch(tok,"LayerCount:")==0 ) {
		had_layer_cnt = true;
		getint(sfd,&sf.layer_cnt);
		if ( sf.layer_cnt>2 ) {
		    sf.layers = calloc(sf.layer_cnt,sizeof(LayerInfo));
		}
	    } else if ( strmatch(tok,"Layer:")==0 ) {
		int layer, o2;
		getint(sfd,&layer);
		getint(sfd,&o2);
		if ( layer<sf.layer_cnt )
		    sf.layers[layer].order2 = o2;
		free( SFDReadUTF7Str(sfd));
	    } else if ( strmatch(tok,"MultiLayer:")==0 ) {
		int ml;
		getint(sfd,&ml);
		sf.multilayer = ml;
	    } else if ( strmatch(tok,"StrokedFont:")==0 ) {
		int stk;
		getint(sfd,&stk);
		sf.strokedfont = stk;
	    } else if ( strmatch(tok,"Ascent:")==0 ) {
		getint(sfd,&sf.ascent);
	    } else if ( strmatch(tok,"Descent:")==0 ) {
		getint(sfd,&sf.descent);
	    } else if ( strmatch(tok,"InvalidEm:")==0 ) {
		getint(sfd,&sf.invalidem);
	    }
	    pos = ftell(sfd);
	}
    }
    fclose(sfd);
    if ( cur_sf->save_to_dir ) {
	if ( sc!=NULL ) IError("Read a glyph from font.props");
	/* Doesn't work for CID keyed, nor for mm */
	snprintf(tok,sizeof(tok),"%s/%s" GLYPH_EXT,cur_sf->filename,name);
	sfd = fopen(tok,"r");
	if ( sfd!=NULL ) {
	    sc = SFDGetChar(sfd,&sf,had_layer_cnt);
	    fclose(sfd);
	}
    }

    if ( sf.layers!=layers )
	free(sf.layers);
    switch_to_old_locale(&tmplocale, &oldlocale); // Switch to the cached locale.
return( sc );
}

static int ModSF(FILE *asfd,SplineFont *sf) {
    Encoding *newmap;
    int cnt;
    int multilayer=0;
    char tok[200];
    int i,k;
    SplineChar *sc;
    SplineFont *ssf;
    SplineFont temp;
    int layercnt;

    memset(&temp,0,sizeof(temp));
    temp.layers = sf->layers;
    temp.layer_cnt = sf->layer_cnt;
    temp.layers[ly_back].order2 = sf->layers[ly_back].order2;
    temp.layers[ly_fore].order2 = sf->layers[ly_fore].order2;
    temp.ascent = sf->ascent; temp.descent = sf->descent;
    temp.multilayer = sf->multilayer;
    temp.gpos_lookups = sf->gpos_lookups;
    temp.gsub_lookups = sf->gsub_lookups;
    temp.anchor = sf->anchor;
    temp.sfd_version = 2;

    if ( getname(asfd,tok)!=1 || strcmp(tok,"Encoding:")!=0 )
return(false);
    newmap = SFDGetEncoding(asfd,tok);
    if ( getname(asfd,tok)!=1 )
return( false );
    if ( strcmp(tok,"UnicodeInterp:")==0 ) {
	sf->uni_interp = SFDGetUniInterp(asfd,tok,sf);
	if ( getname(asfd,tok)!=1 )
return( false );
    }
    if ( sf->map!=NULL && sf->map->enc!=newmap ) {
	EncMap *map = EncMapFromEncoding(sf,newmap);
	EncMapFree(sf->map);
	sf->map = map;
    }
    temp.map = sf->map;
    if ( strcmp(tok,"LayerCount:")==0 ) {
	getint(asfd,&layercnt);
	if ( layercnt>sf->layer_cnt ) {
	    sf->layers = realloc(sf->layers,layercnt*sizeof(LayerInfo));
	    memset(sf->layers+sf->layer_cnt,0,(layercnt-sf->layer_cnt)*sizeof(LayerInfo));
	}
	sf->layer_cnt = layercnt;
	if ( getname(asfd,tok)!=1 )
return( false );
    }
    while ( strcmp(tok,"Layer:")==0 ) {
	int layer, o2;
	getint(asfd,&layer);
	getint(asfd,&o2);
	if ( layer<sf->layer_cnt ) {
	    sf->layers[layer].order2 = o2;
		if (sf->layers[layer].name)
		    free(sf->layers[layer].name);
	    sf->layers[layer].name = SFDReadUTF7Str(asfd);
	}
	if ( getname(asfd,tok)!=1 )
return( false );
    }
    if ( strcmp(tok,"MultiLayer:")==0 ) {
	getint(asfd,&multilayer);
	if ( getname(asfd,tok)!=1 )
return( false );
    }
    if ( multilayer!=sf->multilayer ) {
	if ( !multilayer )
	    SFSplinesFromLayers(sf,false);
	sf->multilayer = multilayer;
	/* SFLayerChange(sf);*/		/* Shouldn't have any open windows, should not be needed */
    }
    if ( strcmp(tok,"BeginChars:")!=0 )
return(false);
    SFRemoveDependencies(sf);

    getint(asfd,&cnt);
    if ( cnt>sf->glyphcnt ) {
	sf->glyphs = realloc(sf->glyphs,cnt*sizeof(SplineChar *));
	for ( i=sf->glyphcnt; i<cnt; ++i )
	    sf->glyphs[i] = NULL;
	sf->glyphcnt = sf->glyphmax = cnt;
    }
    while ( (sc = SFDGetChar(asfd,&temp,true))!=NULL ) {
	ssf = sf;
	for ( k=0; k<sf->subfontcnt; ++k ) {
	    if ( sc->orig_pos<sf->subfonts[k]->glyphcnt ) {
		ssf = sf->subfonts[k];
		if ( SCWorthOutputting(ssf->glyphs[sc->orig_pos]))
	break;
	    }
	}
	if ( sc->orig_pos<ssf->glyphcnt ) {
	    if ( ssf->glyphs[sc->orig_pos]!=NULL )
		SplineCharFree(ssf->glyphs[sc->orig_pos]);
	    ssf->glyphs[sc->orig_pos] = sc;
	    sc->parent = ssf;
	    sc->changed = true;
	}
    }
    sf->changed = true;
    SFDFixupRefs(sf);
return(true);
}

static SplineFont *SlurpRecovery(FILE *asfd,char *tok,int sizetok) {
    char *pt; int ch;
    SplineFont *sf;

    ch=nlgetc(asfd);
    ungetc(ch,asfd);
    if ( ch=='B' ) {
	if ( getname(asfd,tok)!=1 )
return(NULL);
	if ( strcmp(tok,"Base:")!=0 )
return(NULL);
	while ( isspace(ch=nlgetc(asfd)) && ch!=EOF && ch!='\n' );
	for ( pt=tok; ch!=EOF && ch!='\n'; ch = nlgetc(asfd) )
	    if ( pt<tok+sizetok-2 )
		*pt++ = ch;
	*pt = '\0';
	sf = LoadSplineFont(tok,0);
    } else {
	sf = SplineFontNew();
	sf->onlybitmaps = false;
	strcpy(tok,"<New File>");
    }
    if ( sf==NULL )
return( NULL );

    if ( !ModSF(asfd,sf)) {
	SplineFontFree(sf);
return( NULL );
    }
return( sf );
}

/**
 * Asks the user whether or not to recover, skip or delete an autosaved file.
 * If requested by the user, this function will attempt to delete the file.
 * @param [in] filename The path to the autosaved file.
 * @param [in,out] state The current state.
 *                 state&1: Recover all. state&2: Forget all.
 * @param [out] asfd Location to store the file pointer to the autosaved file.
 * @return true iff the file is to be recovered. If true, asfd will hold the
 *         corresponding file pointer, which must be closed by the caller. If
 *         false, asfd will hold NULL.
 */
static int ask_about_file(char *filename, int *state, FILE **asfd) {
    int ret;
    char *buts[6];
    char buffer[800], *pt;

    if ((*asfd = fopen(filename, "r")) == NULL) {
        return false;
    } else if (*state&1) { //Recover all
        return true;
    } else if (*state&2) { //Forget all
        fclose(*asfd);
        *asfd = NULL;
        unlink(filename);
        return false;
    }

    fgets(buffer,sizeof(buffer),*asfd);
    rewind(*asfd);
    if (strncmp(buffer,"Base: ",6) != 0) {
        strcpy(buffer+6, "<New File>");
    }
    pt = buffer+6;
    if (strlen(buffer+6) > 70) {
        pt = strrchr(buffer+6,'/');
        if (pt == NULL)
            pt = buffer+6;
    }

    buts[0] = _("Yes"); buts[1] = _("Yes to _All");
    buts[2] = _("_Skip for now");
    buts[3] = _("Forget _to All"); buts[4] = _("_Forget about it");
    buts[5] = NULL;
    ret = ff_ask(_("Recover old edit"),(const char **) buts,0,3,_("You appear to have an old editing session on %s.\nWould you like to recover it?"), pt);
    switch (ret) {
        case 1: //Recover all
            *state = 1;
            break;
        case 2: //Skip one
            fclose(*asfd);
            *asfd = NULL;
            return false;
        case 3: //Forget all
            *state = 2;
            /* Fall through */
        case 4: //Forget one
            fclose(*asfd);
            *asfd = NULL;
            unlink(filename);
            return false;
        default: //Recover one
            break;
    }
    return true;
}

SplineFont *SFRecoverFile(char *autosavename,int inquire,int *state) {
    FILE *asfd;
    SplineFont *ret;
    char tok[1025];

    if (!inquire) {
        *state = 1; //Default to recover all
    }
    if (!ask_about_file(autosavename, state, &asfd)) {
return( NULL );
    }
    locale_t tmplocale; locale_t oldlocale; // Declare temporary locale storage.
    switch_to_c_locale(&tmplocale, &oldlocale); // Switch to the C locale temporarily and cache the old locale.
    ret = SlurpRecovery(asfd,tok,sizeof(tok));
    if ( ret==NULL ) {
	const char *buts[3];
	buts[0] = "_Forget It"; buts[1] = "_Try Again"; buts[2] = NULL;
	if ( ff_ask(_("Recovery Failed"),(const char **) buts,0,1,_("Automagic recovery of changes to %.80s failed.\nShould FontForge try again to recover next time you start it?"),tok)==0 )
	    unlink(autosavename);
    }
    switch_to_old_locale(&tmplocale, &oldlocale); // Switch to the cached locale.
    fclose(asfd);
    if ( ret )
	ret->autosavename = copy(autosavename);
return( ret );
}

void SFAutoSave(SplineFont *sf,EncMap *map) {
    int i, k, max;
    FILE *asfd;
    SplineFont *ssf;

    if ( no_windowing_ui )		/* No autosaves when just scripting */
return;

    if ( sf->cidmaster!=NULL ) sf=sf->cidmaster;
    asfd = fopen(sf->autosavename,"w");
    if ( asfd==NULL )
return;

    max = sf->glyphcnt;
    for ( i=0; i<sf->subfontcnt; ++i )
	if ( sf->subfonts[i]->glyphcnt>max ) max = sf->subfonts[i]->glyphcnt;

    locale_t tmplocale; locale_t oldlocale; // Declare temporary locale storage.
    switch_to_c_locale(&tmplocale, &oldlocale); // Switch to the C locale temporarily and cache the old locale.
    if ( !sf->new && sf->origname!=NULL )	/* might be a new file */
	fprintf( asfd, "Base: %s%s\n", sf->origname,
		sf->compression==0?"":compressors[sf->compression-1].ext );
    fprintf( asfd, "Encoding: %s\n", map->enc->enc_name );
    fprintf( asfd, "UnicodeInterp: %s\n", unicode_interp_names[sf->uni_interp]);
    fprintf( asfd, "LayerCount: %d\n", sf->layer_cnt );
    for ( i=0; i<sf->layer_cnt; ++i ) {
	fprintf( asfd, "Layer: %d %d ", i, sf->layers[i].order2 );
	SFDDumpUTF7Str(asfd,sf->layers[i].name);
	putc('\n',asfd);
    }
    if ( sf->multilayer )
	fprintf( asfd, "MultiLayer: %d\n", sf->multilayer );
    fprintf( asfd, "BeginChars: %d\n", max );
    for ( i=0; i<max; ++i ) {
	ssf = sf;
	for ( k=0; k<sf->subfontcnt; ++k ) {
	    if ( i<sf->subfonts[k]->glyphcnt ) {
		ssf = sf->subfonts[k];
		if ( SCWorthOutputting(ssf->glyphs[i]))
	break;
	    }
	}
	if ( ssf->glyphs[i]!=NULL && ssf->glyphs[i]->changed )
	    SFDDumpChar( asfd,ssf->glyphs[i],map,NULL,false,1);
    }
    fprintf( asfd, "EndChars\n" );
    fprintf( asfd, "EndSplineFont\n" );
    fclose(asfd);
    switch_to_old_locale(&tmplocale, &oldlocale); // Switch to the cached locale.
    sf->changed_since_autosave = false;
}

void SFClearAutoSave(SplineFont *sf) {
    int i;
    SplineFont *ssf;

    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;
    sf->changed_since_autosave = false;
    for ( i=0; i<sf->subfontcnt; ++i ) {
	ssf = sf->subfonts[i];
	ssf->changed_since_autosave = false;
	if ( ssf->autosavename!=NULL ) {
	    unlink( ssf->autosavename );
	    free( ssf->autosavename );
	    ssf->autosavename = NULL;
	}
    }
    if ( sf->autosavename==NULL )
return;
    unlink(sf->autosavename);
    free(sf->autosavename);
    sf->autosavename = NULL;
}

char **NamesReadSFD(char *filename) {
    FILE *sfd = fopen(filename,"r");
    char tok[2000];
    char **ret = NULL;
    int eof;

    if ( sfd==NULL )
return( NULL );
    locale_t tmplocale; locale_t oldlocale; // Declare temporary locale storage.
    switch_to_c_locale(&tmplocale, &oldlocale); // Switch to the C locale temporarily and cache the old locale.
    if ( SFDStartsCorrectly(sfd,tok)!=-1 ) {
	while ( !feof(sfd)) {
	    if ( (eof = getname(sfd,tok))!=1 ) {
		if ( eof==-1 )
	break;
		geteol(sfd,tok);
	continue;
	    }
	    if ( strmatch(tok,"FontName:")==0 ) {
		getname(sfd,tok);
		ret = malloc(2*sizeof(char*));
		ret[0] = copy(tok);
		ret[1] = NULL;
	break;
	    }
	}
    }
    switch_to_old_locale(&tmplocale, &oldlocale); // Switch to the cached locale.
    fclose(sfd);
return( ret );
}
