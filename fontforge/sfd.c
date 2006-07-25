/* Copyright (C) 2000-2006 by George Williams */
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
#include "gdraw.h"
#include "ustring.h"
#include <math.h>
#include <utype.h>
#include <unistd.h>
#include <locale.h>
#include <gwidget.h>

/* I will retain this list in case there are still some really old sfd files */
/*  including numeric encodings.  This table maps them to string encodings */
static const char *charset_names[] = {
    "custom",
    "iso8859-1", "iso8859-2", "iso8859-3", "iso8859-4", "iso8859-5",
    "iso8859-6", "iso8859-7", "iso8859-8", "iso8859-9", "iso8859-10",
    "iso8859-11", "iso8859-13", "iso8859-14", "iso8859-15",
    "koi8-r",
    "jis201",
    "win", "mac", "symbol", "zapfding", "adobestandard",
    "jis208", "jis212", "ksc5601", "gb2312", "big5", "big5hkscs", "johab",
    "unicode", "unicode4", "sjis", "wansung", "gb2312pk", NULL};

static const char *unicode_interp_names[] = { "none", "adobe", "greek",
    "japanese", "tradchinese", "simpchinese", "korean", "ams", NULL };

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
    putc(' ',sfd);
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
	    ostr = ret = galloc(len+1);
    }
    *ostr = '\0';
return( ret );
}

static char *SFDReadUTF7Str(FILE *sfd) {
    char *buffer = NULL, *pt, *end = NULL;
    int ch1, ch2, ch3, ch4, done, c;
    int prev_cnt=0, prev=0, in=0;

    ch1 = getc(sfd);
    while ( isspace(ch1) && ch1!='\n' && ch1!='\r') ch1 = getc(sfd);
    if ( ch1=='\n' || ch1=='\r' )
	ungetc(ch1,sfd);
    if ( ch1!='"' )
return( NULL );
    pt = NULL;
    while ( (ch1=getc(sfd))!=EOF && ch1!='"' ) {
	done = 0;
	if ( !done && !in ) {
	    if ( ch1=='+' ) {
		ch1 = getc(sfd);
		if ( ch1=='-' ) {
		    if ( pt<end ) *pt++ = '+';
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
		ch2 = inbase64[c = getc(sfd)];
		if ( ch2==-1 ) {
		    ungetc(c, sfd);
		    ch2 = ch3 = ch4 = 0;
		} else {
		    ch3 = inbase64[c = getc(sfd)];
		    if ( ch3==-1 ) {
			ungetc(c, sfd);
			ch3 = ch4 = 0;
		    } else {
			ch4 = inbase64[c = getc(sfd)];
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
		pt = buffer = galloc(400);
		end = buffer+400;
	    } else {
		char *temp = grealloc(buffer,end-buffer+400);
		pt = temp+(pt-buffer);
		end = temp+(end-buffer+400);
		buffer = temp;
	    }
	}
	if ( done )
	    pt = utf8_idpb(pt,ch1);
	if ( prev_cnt==2 ) {
	    prev_cnt = 0;
	    if ( prev!=0 )
		pt = utf8_idpb(pt,prev);
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
    int ch1, ch2, ch3, ch4, done, c;
    int prev_cnt=0, prev=0, in=0;
    const char *str = _str;

    if ( str==NULL )
return( NULL );
    buffer = pt = galloc(400);
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
		ch2 = inbase64[c = *str++];
		if ( ch2==1 ) {
		    --str;
		    ch2 = ch3 = ch4 = 0;
		} else {
		    ch3 = inbase64[c = *str++];
		    if ( ch3==-1 ) {
			--str;
			ch3 = ch4 = 0;
		    } else {
			ch4 = inbase64[c = *str++];
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
	    char *temp = grealloc(buffer,end-buffer+400);
	    pt = temp+(pt-buffer);
	    end = temp+(end-buffer+400);
	    buffer = temp;
	}
	if ( done )
	    pt = utf8_idpb(pt,ch1);
	if ( prev_cnt==2 ) {
	    prev_cnt = 0;
	    if ( prev!=0 )
		pt = utf8_idpb(pt,prev);
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
    int i, j;

    for ( i=HntMax/8-1; i>0; --i )
	if ( (*hintmask)[i]!=0 )
    break;
    for ( j=0; j<=i ; ++j ) {
	if ( ((*hintmask)[j]>>4)<10 )
	    putc('0'+((*hintmask)[j]>>4),sfd);
	else
	    putc('a'-10+((*hintmask)[j]>>4),sfd);
	if ( ((*hintmask)[j]&0xf)<10 )
	    putc('0'+((*hintmask)[j]&0xf),sfd);
	else
	    putc('a'-10+((*hintmask)[j]&0xf),sfd);
    }
}

static void SFDDumpSplineSet(FILE *sfd,SplineSet *spl) {
    SplinePoint *first, *sp;
    int order2 = spl->first->next==NULL || spl->first->next->order2;

    for ( ; spl!=NULL; spl=spl->next ) {
	first = NULL;
	for ( sp = spl->first; ; sp=sp->next->to ) {
	    if ( first==NULL )
		fprintf( sfd, "%g %g m ", (double) sp->me.x, (double) sp->me.y );
	    else if ( sp->prev->islinear )		/* Don't use known linear here. save control points if there are any */
		fprintf( sfd, " %g %g l ", (double) sp->me.x, (double) sp->me.y );
	    else
		fprintf( sfd, " %g %g %g %g %g %g c ",
			(double) sp->prev->from->nextcp.x, (double) sp->prev->from->nextcp.y,
			(double) sp->prevcp.x, (double) sp->prevcp.y,
			(double) sp->me.x, (double) sp->me.y );
	    fprintf(sfd, "%d", sp->pointtype|(sp->selected<<2)|
			(sp->nextcpdef<<3)|(sp->prevcpdef<<4)|
			(sp->roundx<<5)|(sp->roundy<<6)|
			(sp->ttfindex==0xffff?(1<<7):0)|
			(sp->dontinterpolate<<8) );
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
	    if ( sp==first )
	break;
	    if ( first==NULL ) first = sp;
	    if ( sp->next==NULL )
	break;
	}
    }
    fprintf( sfd, "EndSplineSet\n" );
}

static void SFDDumpMinimumDistances(FILE *sfd,SplineChar *sc) {
    MinimumDistance *md = sc->md;
    SplineSet *ss;
    SplinePoint *sp;
    int pt=0;

    if ( md==NULL )
return;
    for ( ss = sc->layers[ly_fore].splines; ss!=NULL; ss=ss->next ) {
	for ( sp=ss->first; ; ) {
	    sp->ptindex = pt++;
	    if ( sp->next == NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
    }
    fprintf( sfd, "MinimumDistance: " );
    while ( md!=NULL ) {
	fprintf( sfd, "%c%d,%d ", md->x?'x':'y', md->sp1?md->sp1->ptindex:-1,
		md->sp2?md->sp2->ptindex:-1 );
	md = md->next;
    }
    fprintf( sfd, "\n" );
}

#ifdef FONTFORGE_CONFIG_DEVICETABLES
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
#endif

static void SFDDumpAnchorPoints(FILE *sfd,SplineChar *sc) {
    AnchorPoint *ap;

    for ( ap = sc->anchor; ap!=NULL; ap=ap->next ) {
	fprintf( sfd, "AnchorPoint: " );
	SFDDumpUTF7Str(sfd,ap->anchor->name);
	fprintf( sfd, "%g %g %s %d",
		(double) ap->me.x, (double) ap->me.y,
		ap->type==at_centry ? "entry" :
		ap->type==at_cexit ? "exit" :
		ap->type==at_mark ? "mark" :
		ap->type==at_basechar ? "basechar" :
		ap->type==at_baselig ? "baselig" : "basemark",
		ap->lig_index );
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	if ( ap->xadjust.corrections!=NULL || ap->yadjust.corrections!=NULL ) {
	    putc(' ',sfd);
	    SFDDumpDeviceTable(sfd,&ap->xadjust);
	    putc(' ',sfd);
	    SFDDumpDeviceTable(sfd,&ap->yadjust);
	} else
#endif
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
    rle = gcalloc(max,sizeof(uint8)), pt = rle, end=rle+max-3;

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
	    if ( base->image_type==it_true ) {
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

static void SFDDumpHintList(FILE *sfd,char *key, StemInfo *h) {
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

static void SFDDumpDHintList(FILE *sfd,char *key, DStemInfo *h) {

    if ( h==NULL )
return;
    fprintf(sfd, "%s", key );
    for ( ; h!=NULL; h=h->next ) {
	fprintf(sfd, "%g %g %g %g %g %g %g %g",
		(double) h->leftedgetop.x, (double) h->leftedgetop.y,
		(double) h->rightedgetop.x, (double) h->rightedgetop.y,
		(double) h->leftedgebottom.x, (double) h->leftedgebottom.y,
		(double) h->rightedgebottom.x, (double) h->rightedgebottom.y );
	putc(h->next?' ':'\n',sfd);
    }
}

static void SFDDumpTtfInstrs(FILE *sfd,SplineChar *sc) {
    struct enc85 enc;
    int i;

    memset(&enc,'\0',sizeof(enc));
    enc.sfd = sfd;

    fprintf( sfd, "TtfInstrs: %d\n", sc->ttf_instrs_len );
    for ( i=0; i<sc->ttf_instrs_len; ++i )
	SFDEnc85(&enc,sc->ttf_instrs[i]);
    SFDEnc85EndEnc(&enc);
    fprintf(sfd,"\nEndTtf\n" );
}

static void SFDDumpTtfTable(FILE *sfd,struct ttf_table *tab) {
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

static void SFDDumpRefs(FILE *sfd,RefChar *refs, char *name,EncMap *map, int *newgids) {
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

#ifdef FONTFORGE_CONFIG_TYPE3
static char *joins[] = { "miter", "round", "bevel", "inher", NULL };
static char *caps[] = { "butt", "round", "square", "inher", NULL };
#endif

static void SFDDumpChar(FILE *sfd,SplineChar *sc,EncMap *map,int *newgids) {
    ImageList *img;
    KernPair *kp;
    PST *liga;
    int i;
    struct altuni *altuni;

    fprintf(sfd, "StartChar: %s\n", sc->name );
    if ( map->backmap[sc->orig_pos]>=map->enccount ) {
	IError("Bad reverse encoding");
	map->backmap[sc->orig_pos] = -1;
    }
    fprintf(sfd, "Encoding: %d %d %d\n", (int) map->backmap[sc->orig_pos], sc->unicodeenc,
	    newgids!=NULL?newgids[sc->orig_pos]:sc->orig_pos);
    if ( sc->altuni ) {
	fprintf( sfd, "AltUni:" );
	for ( altuni = sc->altuni; altuni!=NULL; altuni=altuni->next )
	    fprintf( sfd, " %d", altuni->unienc );
	putc( '\n', sfd);
    }
    fprintf(sfd, "Width: %d\n", sc->width );
    if ( sc->vwidth!=sc->parent->ascent+sc->parent->descent )
	fprintf(sfd, "VWidth: %d\n", sc->vwidth );
    if ( sc->glyph_class!=0 )
	fprintf(sfd, "GlyphClass: %d\n", sc->glyph_class );
    if ( sc->changedsincelasthinted|| sc->manualhints || sc->widthset )
	fprintf(sfd, "Flags: %s%s%s%s%s\n",
		sc->changedsincelasthinted?"H":"",
		sc->manualhints?"M":"",
		sc->widthset?"W":"",
		sc->views!=NULL?"O":"",
		sc->instructions_out_of_date?"I":"");
    if ( sc->tex_height!=TEX_UNDEF || sc->tex_depth!=TEX_UNDEF ||
	    sc->tex_sub_pos!=TEX_UNDEF || sc->tex_super_pos!=TEX_UNDEF )
	fprintf( sfd, "TeX: %d %d %d %d\n", sc->tex_height, sc->tex_depth,
		sc->tex_sub_pos, sc->tex_super_pos );
#if HANYANG
    if ( sc->compositionunit )
	fprintf( sfd, "CompositionUnit: %d %d\n", sc->jamo, sc->varient );
#endif
    SFDDumpHintList(sfd,"HStem: ", sc->hstem);
    SFDDumpHintList(sfd,"VStem: ", sc->vstem);
    SFDDumpDHintList(sfd,"DStem: ", sc->dstem);
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
    SFDDumpAnchorPoints(sfd,sc);
    if ( sc->layers[ly_back].splines!=NULL ) {
	fprintf(sfd, "Back\n" );
	SFDDumpSplineSet(sfd,sc->layers[ly_back].splines);
    } else if ( sc->layers[ly_back].images!=NULL )
	fprintf(sfd, "Back\n" );
    for ( img=sc->layers[ly_back].images; img!=NULL; img=img->next )
	SFDDumpImage(sfd,img);
#ifdef FONTFORGE_CONFIG_TYPE3
    if ( sc->parent->multilayer ) {
	fprintf( sfd, "LayerCount: %d\n", sc->layer_cnt );
	for ( i=ly_fore; i<sc->layer_cnt; ++i ) {
	    fprintf(sfd, "Layer: %d  %d %d %d  #%06x %g  #%06x %g %g %s %s [%g %g %g %g] [",
		    i, sc->layers[i].dofill, sc->layers[i].dostroke, sc->layers[i].fillfirst,
		    sc->layers[i].fill_brush.col, sc->layers[i].fill_brush.opacity,
		    sc->layers[i].stroke_pen.brush.col, sc->layers[i].stroke_pen.brush.opacity,
		     sc->layers[i].stroke_pen.width, joins[sc->layers[i].stroke_pen.linejoin], caps[sc->layers[i].stroke_pen.linecap],
		    sc->layers[i].stroke_pen.trans[0], sc->layers[i].stroke_pen.trans[1],
		    sc->layers[i].stroke_pen.trans[2], sc->layers[i].stroke_pen.trans[3] );
	    if ( sc->layers[i].stroke_pen.dashes[0]==0 && sc->layers[i].stroke_pen.dashes[1]==DASH_INHERITED )
		fprintf(sfd,"0 %d]\n", DASH_INHERITED);
	    else { int j;
		for ( j=0; j<DASH_MAX && sc->layers[i].stroke_pen.dashes[j]!=0; ++j )
		    fprintf( sfd,"%d ", sc->layers[i].stroke_pen.dashes[j]);
		fprintf(sfd,"]\n");
	    }
	    for ( img=sc->layers[i].images; img!=NULL; img=img->next )
		SFDDumpImage(sfd,img);
	    if ( sc->layers[i].splines!=NULL ) {
		fprintf(sfd, "SplineSet\n" );
		SFDDumpSplineSet(sfd,sc->layers[i].splines);
	    }
	    SFDDumpRefs(sfd,sc->layers[i].refs,sc->name,map,newgids);
	}
    } else
#endif
    {
	if ( sc->layers[ly_fore].splines!=NULL ) {
	    fprintf(sfd, "Fore\n" );
	    SFDDumpSplineSet(sfd,sc->layers[ly_fore].splines);
	    SFDDumpMinimumDistances(sfd,sc);
	}
	SFDDumpRefs(sfd,sc->layers[ly_fore].refs,sc->name,map,newgids);
    }
    if ( sc->kerns!=NULL ) {
	fprintf( sfd, "KernsSLIFO:" );
	for ( kp = sc->kerns; kp!=NULL; kp=kp->next )
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    if ( (kp->off!=0 || kp->adjust!=NULL) && !SFDOmit(kp->sc)) {
		fprintf( sfd, " %d %d %d %d",
			newgids!=NULL?newgids[kp->sc->orig_pos]:kp->sc->orig_pos,
			kp->off, kp->sli, kp->flags );
		if ( kp->adjust!=NULL ) putc(' ',sfd);
		SFDDumpDeviceTable(sfd,kp->adjust);
	    }
#else
	    if ( kp->off!=0 && !SFDOmit(kp->sc))
		fprintf( sfd, " %d %d %d %d",
			newgids!=NULL?newgids[kp->sc->orig_pos]:kp->sc->orig_pos,
			kp->off, kp->sli, kp->flags );
#endif
	fprintf(sfd, "\n" );
    }
    if ( sc->vkerns!=NULL ) {
	fprintf( sfd, "VKernsSLIFO:" );
	for ( kp = sc->vkerns; kp!=NULL; kp=kp->next )
	    if ( kp->off!=0 && !SFDOmit(kp->sc))
		fprintf( sfd, " %d %d %d %d",
			newgids!=NULL?newgids[kp->sc->orig_pos]:kp->sc->orig_pos,
			kp->off, kp->sli, kp->flags );
	fprintf(sfd, "\n" );
    }
    for ( liga=sc->possub; liga!=NULL; liga=liga->next ) {
	if (( liga->tag==0 && liga->type!=pst_lcaret) || liga->type==pst_null )
	    /* Skip it */;
	else {
	    static char *keywords[] = { "Null:", "Position:", "PairPos:",
		    "Substitution:",
		    "AlternateSubs:", "MultipleSubs:", "Ligature:",
		    "LCarets:", NULL };
	    if ( liga->tag==0 ) liga->tag = CHR(' ',' ',' ',' ');
	    fprintf( sfd, "%s %d %d ",
		    keywords[liga->type], liga->flags,
		    liga->script_lang_index );
	    if ( liga->macfeature )
		fprintf( sfd, "<%d,%d> ",
			(int) (liga->tag>>16),
			(int) (liga->tag&0xffff));
	    else
		fprintf( sfd, "'%c%c%c%c' ",
			(int) (liga->tag>>24), (int) ((liga->tag>>16)&0xff),
			(int) ((liga->tag>>8)&0xff), (int) (liga->tag&0xff) );
	    if ( liga->type==pst_position ) {
		fprintf( sfd, "dx=%d dy=%d dh=%d dv=%d",
			liga->u.pos.xoff, liga->u.pos.yoff,
			liga->u.pos.h_adv_off, liga->u.pos.v_adv_off);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		SFDDumpValDevTab(sfd,liga->u.pos.adjust);
#endif
		putc('\n',sfd);
	    } else if ( liga->type==pst_pair ) {
		fprintf( sfd, "%s dx=%d dy=%d dh=%d dv=%d",
			liga->u.pair.paired,
			liga->u.pair.vr[0].xoff, liga->u.pair.vr[0].yoff,
			liga->u.pair.vr[0].h_adv_off, liga->u.pair.vr[0].v_adv_off );
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		SFDDumpValDevTab(sfd,liga->u.pair.vr[0].adjust);
#endif
		fprintf( sfd, " dx=%d dy=%d dh=%d dv=%d",
			liga->u.pair.vr[1].xoff, liga->u.pair.vr[1].yoff,
			liga->u.pair.vr[1].h_adv_off, liga->u.pair.vr[1].v_adv_off);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		SFDDumpValDevTab(sfd,liga->u.pair.vr[1].adjust);
#endif
		putc('\n',sfd);
	    } else if ( liga->type==pst_lcaret ) {
		int i;
		fprintf( sfd, "%d ", liga->u.lcaret.cnt );
		for ( i=0; i<liga->u.lcaret.cnt; ++i )
		    fprintf( sfd, "%d ", liga->u.lcaret.carets[i] );
		fprintf( sfd, "\n" );
	    } else
		fprintf( sfd, "%s\n", liga->u.lig.components );
	}
    }
    if ( sc->comment!=NULL ) {
	fprintf( sfd, "Comment: " );
	SFDDumpUTF7Str(sfd,sc->comment);
	putc('\n',sfd);
    }
    if ( sc->color!=COLOR_DEFAULT )
	fprintf( sfd, "Colour: %x\n", (int) sc->color );
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
#if 0
    fprintf(sfd,"\nEndBitmapChar\n" );
#else
    fputc('\n',sfd);
#endif
}

static void SFDDumpBitmapFont(FILE *sfd,BDFFont *bdf,EncMap *encm,int *newgids) {
    int i;

    gwwv_progress_next_stage();
    fprintf( sfd, "BitmapFont: %d %d %d %d %d %s\n", bdf->pixelsize, bdf->glyphcnt,
	    bdf->ascent, bdf->descent, BDFDepth(bdf), bdf->foundry?bdf->foundry:"" );
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
	    }
	}
	fprintf( sfd, "BDFEndProperties\n" );
    }
    if ( bdf->res>20 )
	fprintf( sfd, "Resolution: %d\n", bdf->res );
    for ( i=0; i<bdf->glyphcnt; ++i ) {
	if ( bdf->glyphs[i]!=NULL )
	    SFDDumpBitmapChar(sfd,bdf->glyphs[i],encm->backmap[i],newgids);
	gwwv_progress_next();
    }
    fprintf( sfd, "EndBitmapFont\n" );
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
    fprintf( sfd, "LangName: %d ", ln->lang );
    for ( end = ttf_namemax; end>0 && ln->names[end-1]==NULL; --end );
    for ( i=0; i<end; ++i )
	SFDDumpUTF7Str(sfd,ln->names[i]);
    putc('\n',sfd);
}

static void SFDDumpDesignSize(FILE *sfd, SplineFont *sf) {
    struct otfname *on;

    if ( sf->design_size==0 )
return;

    fprintf( sfd, "DesignSize: %d", sf->design_size );
    if ( sf->fontstyle_id!=0 || sf->fontstyle_name!=NULL ||
	    sf->design_range_bottom!=0 || sf->design_range_top!=0 ) {
	fprintf( sfd, " %d-%d %d",
		sf->design_range_bottom, sf->design_range_top,
		sf->fontstyle_id );
	for ( on=sf->fontstyle_name; on!=NULL; on=on->next ) {
	    fprintf( sfd, " %d ", on->lang );
	    SFDDumpUTF7Str(sfd, on->name);
	}
    }
    putc('\n',sfd);
}

static void putstring(FILE *sfd, char *header, char *body) {
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

static void SFDDumpEncoding(FILE *sfd,Encoding *encname,char *keyword) {
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

static void SFD_Dump(FILE *sfd,SplineFont *sf,EncMap *map,EncMap *normal) {
    int i, j, realcnt;
    BDFFont *bdf;
    struct ttflangname *ln;
    struct table_ordering *ord;
    struct ttf_table *tab;
    KernClass *kc;
    FPST *fpst;
    ASM *sm;
    int isv;
    int *newgids = NULL;

    if ( normal!=NULL )
	map = normal;

    fprintf(sfd, "FontName: %s\n", sf->fontname );
    if ( sf->fullname!=NULL )
	fprintf(sfd, "FullName: %s\n", sf->fullname );
    if ( sf->familyname!=NULL )
	fprintf(sfd, "FamilyName: %s\n", sf->familyname );
    if ( sf->weight!=NULL )
	fprintf(sfd, "Weight: %s\n", sf->weight );
    if ( sf->copyright!=NULL )
	putstring(sfd, "Copyright: ", sf->copyright );
    if ( sf->comments!=NULL )
	putstring(sfd, "Comments: ", sf->comments );
    if ( sf->version!=NULL )
	fprintf(sfd, "Version: %s\n", sf->version );
    if ( sf->fondname!=NULL )
	fprintf(sfd, "FONDName: %s\n", sf->fondname );
    if ( sf->strokewidth!=0 )
	fprintf(sfd, "StrokeWidth: %g\n", (double) sf->strokewidth );
    fprintf(sfd, "ItalicAngle: %g\n", (double) sf->italicangle );
    fprintf(sfd, "UnderlinePosition: %g\n", (double) sf->upos );
    fprintf(sfd, "UnderlineWidth: %g\n", (double) sf->uwidth );
    fprintf(sfd, "Ascent: %d\n", sf->ascent );
    fprintf(sfd, "Descent: %d\n", sf->descent );
    if ( sf->order2 )
	fprintf(sfd, "Order2: %d\n", sf->order2 );
    if ( sf->strokedfont )
	fprintf(sfd, "StrokedFont: %d\n", sf->strokedfont );
    else if ( sf->multilayer )
	fprintf(sfd, "MultiLayer: %d\n", sf->multilayer );
    if ( sf->hasvmetrics )
	fprintf(sfd, "VerticalOrigin: %d\n", sf->vertical_origin );
    if ( sf->changed_since_xuidchanged )
	fprintf(sfd, "NeedsXUIDChange: 1\n" );
    if ( sf->xuid!=NULL )
	fprintf(sfd, "XUID: %s\n", sf->xuid );
    if ( sf->uniqueid!=0 )
	fprintf(sfd, "UniqueID: %d\n", sf->uniqueid );
    if ( sf->pfminfo.fstype!=-1 )
	fprintf(sfd, "FSType: %d\n", sf->pfminfo.fstype );
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
    if ( sf->pfminfo.os2_family_class!=0 )
	fprintf(sfd, "OS2FamilyClass: %d\n", sf->pfminfo.os2_family_class );
    if ( sf->pfminfo.os2_vendor[0]!='\0' ) {
	fprintf(sfd, "OS2Vendor: '%c%c%c%c'\n",
		sf->pfminfo.os2_vendor[0], sf->pfminfo.os2_vendor[1],
		sf->pfminfo.os2_vendor[2], sf->pfminfo.os2_vendor[3] );
    }
    if ( sf->macstyle!=-1 )
	fprintf(sfd, "MacStyle: %d\n", sf->macstyle );
    if ( sf->script_lang ) {
	int i,j,k;
	for ( i=0; sf->script_lang[i]!=NULL; ++i );
	fprintf(sfd, "ScriptLang: %d\n", i );
	for ( i=0; sf->script_lang[i]!=NULL; ++i ) {
	    for ( j=0; sf->script_lang[i][j].script!=0; ++j );
	    fprintf( sfd, " %d ", j );		/* Script cnt */
	    for ( j=0; sf->script_lang[i][j].script!=0; ++j ) {
		for ( k=0; sf->script_lang[i][j].langs[k]!=0 ; ++k );
		fprintf( sfd, "%c%c%c%c %d ",
			(int) (sf->script_lang[i][j].script>>24),
			(int) ((sf->script_lang[i][j].script>>16)&0xff),
			(int) ((sf->script_lang[i][j].script>>8)&0xff),
			(int) (sf->script_lang[i][j].script&0xff),
			k );
		for ( k=0; sf->script_lang[i][j].langs[k]!=0 ; ++k )
		    fprintf( sfd, "%c%c%c%c ",
			    (int) (sf->script_lang[i][j].langs[k]>>24),
			    (int) ((sf->script_lang[i][j].langs[k]>>16)&0xff),
			    (int) ((sf->script_lang[i][j].langs[k]>>8)&0xff),
			    (int) (sf->script_lang[i][j].langs[k]&0xff) );
	    }
	    fprintf( sfd,"\n");
	}
    }
    if ( sf->mark_class_cnt!=0 ) {
	fprintf( sfd, "MarkAttachClasses: %d\n", sf->mark_class_cnt );
	for ( i=1; i<sf->mark_class_cnt; ++i ) {	/* Class 0 is unused */
	    SFDDumpUTF7Str(sfd, sf->mark_class_names[i]);
	    if ( sf->mark_classes[i]!=NULL )
		fprintf( sfd, "%d %s\n", (int) strlen(sf->mark_classes[i]),
			sf->mark_classes[i] );
	    else
		fprintf( sfd, "0 \n" );
	}
    }
    for ( isv=0; isv<2; ++isv ) {
	for ( kc=isv ? sf->vkerns : sf->kerns; kc!=NULL; kc = kc->next ) {
	    fprintf( sfd, "%s: %d%s %d %d %d\n", isv ? "VKernClass" : "KernClass",
		    kc->first_cnt, kc->firsts[0]!=NULL?"+":"",
		    kc->second_cnt, kc->sli, kc->flags );
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
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		putc(' ',sfd);
		SFDDumpDeviceTable(sfd,&kc->adjusts[i]);
#endif
	    }
	    fprintf( sfd, "\n" );
	}
    }
    for ( fpst=sf->possub; fpst!=NULL; fpst=fpst->next ) {
	static char *keywords[] = { "ContextPos:", "ContextSub:", "ChainPos:", "ChainSub:", "ReverseChain:", NULL };
	static char *formatkeys[] = { "glyph", "class", "coverage", "revcov", NULL };
	fprintf( sfd, "%s %s %d %d '%c%c%c%c' %d %d %d %d\n",
		keywords[fpst->type-pst_contextpos],
		formatkeys[fpst->format],
		fpst->flags,
		fpst->script_lang_index,
		(int) (fpst->tag>>24), (int) ((fpst->tag>>16)&0xff),
		(int) ((fpst->tag>>8)&0xff), (int) (fpst->tag&0xff),
		fpst->nccnt, fpst->bccnt, fpst->fccnt, fpst->rule_cnt );
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
	    }
	    switch ( fpst->format ) {
	      case pst_glyphs:
	      case pst_class:
	      case pst_coverage:
		fprintf( sfd, " %d\n", fpst->rules[i].lookup_cnt );
		for ( j=0; j<fpst->rules[i].lookup_cnt; ++j )
		    fprintf( sfd, "  SeqLookup: %d '%c%c%c%c'\n",
			    fpst->rules[i].lookups[j].seq,
			    (int) (fpst->rules[i].lookups[j].lookup_tag>>24),
			    (int) ((fpst->rules[i].lookups[j].lookup_tag>>16)&0xff),
			    (int) ((fpst->rules[i].lookups[j].lookup_tag>>8)&0xff),
			    (int) ((fpst->rules[i].lookups[j].lookup_tag)&0xff));
	      break;
	      case pst_reversecoverage:
		fprintf( sfd, "  Replace: %d %s\n",
			 (int)strlen(fpst->rules[i].u.rcoverage.replacements),
			 fpst->rules[i].u.rcoverage.replacements);
	      break;
	    }
	}
	fprintf( sfd, "EndFPST\n" );
    }
    for ( sm=sf->sm; sm!=NULL; sm=sm->next ) {
	static char *keywords[] = { "MacIndic:", "MacContext:", "MacLigature:", "unused", "MacSimple:", "MacInsert:",
	    "unused", "unused", "unused", "unused", "unused", "unused",
	    "unused", "unused", "unused", "unused", "unused", "MacKern:",
	    NULL };
	fprintf( sfd, "%s %d,%d %d %d %d\n",
		keywords[sm->type-asm_indic],
		sm->feature, sm->setting,
		sm->flags,
		sm->class_cnt, sm->state_cnt );
	for ( i=4; i<sm->class_cnt; ++i )
	  fprintf( sfd, "  Class: %d %s\n", (int)strlen(sm->classes[i]),
		   sm->classes[i]);
	for ( i=0; i<sm->class_cnt*sm->state_cnt; ++i ) {
	    fprintf( sfd, " %d %d ", sm->state[i].next_state, sm->state[i].flags );
	    if ( sm->type==asm_context ) {
		if ( sm->state[i].u.context.mark_tag==0 )
		    fprintf(sfd,"~ ");
		else
		    fprintf(sfd,"'%c%c%c%c' ",
			(int) (sm->state[i].u.context.mark_tag>>24),
			(int) ((sm->state[i].u.context.mark_tag>>16)&0xff),
			(int) ((sm->state[i].u.context.mark_tag>>8)&0xff),
			(int) (sm->state[i].u.context.mark_tag&0xff));
		if ( sm->state[i].u.context.cur_tag==0 )
		    fprintf(sfd,"~ ");
		else
		    fprintf(sfd,"'%c%c%c%c' ",
			(int) (sm->state[i].u.context.cur_tag>>24),
			(int) ((sm->state[i].u.context.cur_tag>>16)&0xff),
			(int) ((sm->state[i].u.context.cur_tag>>8)&0xff),
			(int) (sm->state[i].u.context.cur_tag&0xff));
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
    if ( sf->gentags.tt_cur>0 ) {
	fprintf( sfd, "GenTags: %d", sf->gentags.tt_cur );
	for ( i=0; i<sf->gentags.tt_cur; ++i ) {
	    switch ( sf->gentags.tagtype[i].type ) {
	      case pst_position:	fprintf(sfd," ps"); break;
	      case pst_pair:		fprintf(sfd," pr"); break;
	      case pst_substitution:	fprintf(sfd," sb"); break;
	      case pst_alternate:	fprintf(sfd," as"); break;
	      case pst_multiple:	fprintf(sfd," ms"); break;
	      case pst_ligature:	fprintf(sfd," lg"); break;
	      case pst_anchors:		fprintf(sfd," an"); break;
	      case pst_contextpos:	fprintf(sfd," cp"); break;
	      case pst_contextsub:	fprintf(sfd," cs"); break;
	      case pst_chainpos:	fprintf(sfd," pc"); break;
	      case pst_chainsub:	fprintf(sfd," sc"); break;
	      case pst_reversesub:	fprintf(sfd," rs"); break;
	      case pst_null:		fprintf(sfd," nl"); break;
	    }
	    fprintf(sfd,"'%c%c%c%c'",
		    (int) (sf->gentags.tagtype[i].tag>>24),
		    (int) ((sf->gentags.tagtype[i].tag>>16)&0xff),
		    (int) ((sf->gentags.tagtype[i].tag>>8)&0xff),
		    (int) (sf->gentags.tagtype[i].tag&0xff) );
	}
	putc('\n',sfd);
    }
    for ( ord = sf->orders; ord!=NULL ; ord = ord->next ) {
	for ( i=0; ord->ordered_features[i]!=0; ++i );
	fprintf( sfd, "TableOrder: %c%c%c%c %d\n",
		(int) (ord->table_tag>>24), (int) ((ord->table_tag>>16)&0xff), (int) ((ord->table_tag>>8)&0xff), (int) (ord->table_tag&0xff),
		i );
	for ( i=0; ord->ordered_features[i]!=0; ++i )
	    if ( (ord->ordered_features[i]>>24)<' ' || (ord->ordered_features[i]>>24)>=0x7f )
		fprintf( sfd, "\t<%d,%d>\n", (int) (ord->ordered_features[i]>>16), (int) (ord->ordered_features[i]&0xffff) );
	    else
		fprintf( sfd, "\t'%c%c%c%c'\n",
			(int) (ord->ordered_features[i]>>24), (int) ((ord->ordered_features[i]>>16)&0xff), (int) ((ord->ordered_features[i]>>8)&0xff), (int) (ord->ordered_features[i]&0xff) );
    }
    for ( tab = sf->ttf_tables; tab!=NULL ; tab = tab->next )
	SFDDumpTtfTable(sfd,tab);
    for ( tab = sf->ttf_tab_saved; tab!=NULL ; tab = tab->next )
	SFDDumpTtfTable(sfd,tab);
    for ( ln = sf->names; ln!=NULL; ln=ln->next )
	SFDDumpLangName(sfd,ln);
    if ( sf->design_size!=0 )
	SFDDumpDesignSize(sfd,sf);
    if ( sf->subfontcnt!=0 ) {
	/* CID fonts have no encodings, they have registry info instead */
	fprintf(sfd, "Registry: %s\n", sf->cidregistry );
	fprintf(sfd, "Ordering: %s\n", sf->ordering );
	fprintf(sfd, "Supplement: %d\n", sf->supplement );
	fprintf(sfd, "CIDVersion: %g\n", sf->cidversion );	/* This is a number whereas "version" is a string */
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
    fprintf( sfd, "AntiAlias: %d\n", sf->display_antialias );
    fprintf( sfd, "FitToEm: %d\n", sf->display_bbsized );
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
	fprintf(sfd, "Grid\n" );
	SFDDumpSplineSet(sfd,sf->grid.splines);
    }
    if ( sf->texdata.type!=tex_unset ) {
	fprintf(sfd, "TeXData: %d %d", sf->texdata.type, (int) ((sf->design_size<<19)+2)/5 );
	for ( i=0; i<22; ++i )
	    fprintf(sfd, " %d", (int) sf->texdata.params[i]);
	putc('\n',sfd);
    }
    if ( sf->anchor!=NULL ) {
	AnchorClass *an;
	fprintf(sfd, "AnchorClass: ");
	for ( an=sf->anchor; an!=NULL; an=an->next ) {
	    SFDDumpUTF7Str(sfd,an->name);
	    if ( an->feature_tag==0 )
		fprintf( sfd, "0 ");
	    else
		fprintf( sfd, "%c%c%c%c ",
			(int) (an->feature_tag>>24), (int) ((an->feature_tag>>16)&0xff),
			(int) ((an->feature_tag>>8)&0xff), (int) (an->feature_tag&0xff) );
	    fprintf( sfd, "%d %d %d %d ", an->flags, an->script_lang_index,
		    an->merge_with, an->type );
	}
	putc('\n',sfd);
    }

    if ( sf->subfontcnt!=0 ) {
	int max;
	for ( i=max=0; i<sf->subfontcnt; ++i )
	    if ( max<sf->subfonts[i]->glyphcnt )
		max = sf->subfonts[i]->glyphcnt;
	fprintf(sfd, "BeginSubFonts: %d %d\n", sf->subfontcnt, max );
	for ( i=0; i<sf->subfontcnt; ++i )
	    SFD_Dump(sfd,sf->subfonts[i],map,NULL);
	fprintf(sfd, "EndSubFonts\n" );
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
		newgids = galloc(sf->glyphcnt*sizeof(int));
		realcnt = 0;
		for ( i=0; i<sf->glyphcnt; ++i )
		    if ( SFDOmit(sf->glyphs[i]) )
			newgids[i] = -1;
		    else
			newgids[i] = realcnt++;
	    }
	}
	fprintf(sfd, "BeginChars: %d %d\n", enccount, realcnt );
	for ( i=0; i<sf->glyphcnt; ++i ) {
	    if ( !SFDOmit(sf->glyphs[i]) )
		SFDDumpChar(sfd,sf->glyphs[i],map,newgids);
#if defined(FONTFORGE_CONFIG_GDRAW)
	    gwwv_progress_next();
#elif defined(FONTFORGE_CONFIG_GTK)
	    gwwv_progress_next();
#endif
	}
	fprintf(sfd, "EndChars\n" );
	for ( i=0; i<map->enccount; ++i ) {
	    if ( map->map[i]!=-1 && map->backmap[map->map[i]]!=i &&
		    !SFDOmit(sf->glyphs[map->map[i]]) )
		fprintf( sfd, "DupEnc: %d %d\n", i,
			(int) (newgids!=NULL?newgids[map->map[i]]: map->map[i]));
	}
    }

    if ( sf->bitmaps!=NULL )
#if defined(FONTFORGE_CONFIG_GDRAW)
	gwwv_progress_change_line2(_("Saving Bitmaps"));
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_progress_change_line2(_("Saving Bitmaps"));
#endif
    for ( bdf = sf->bitmaps; bdf!=NULL; bdf=bdf->next )
	SFDDumpBitmapFont(sfd,bdf,map,newgids);
    fprintf(sfd, sf->cidmaster==NULL?"EndSplineFont\n":"EndSubSplineFont\n" );
    if ( newgids!=NULL )
	free(newgids);
}

static void SFD_MMDump(FILE *sfd,SplineFont *sf,EncMap *map,EncMap *normal) {
    MMSet *mm = sf->mm;
    int max, i, j;

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

    for ( i=max=0; i<mm->instance_count; ++i )
	if ( max<mm->instances[i]->glyphcnt )
	    max = mm->instances[i]->glyphcnt;
    fprintf(sfd, "BeginMMFonts: %d %d\n", mm->instance_count+1, max );
    for ( i=0; i<mm->instance_count; ++i )
	SFD_Dump(sfd,mm->instances[i],map,normal);
    SFD_Dump(sfd,mm->normal,map,normal);
    fprintf(sfd, "EndMMFonts\n" );
}

static void SFDDump(FILE *sfd,SplineFont *sf,EncMap *map,EncMap *normal) {
    int i, realcnt;
    BDFFont *bdf;

    realcnt = sf->glyphcnt;
    if ( sf->subfontcnt!=0 ) {
	for ( i=0; i<sf->subfontcnt; ++i )
	    if ( realcnt<sf->subfonts[i]->glyphcnt )
		realcnt = sf->subfonts[i]->glyphcnt;
    }
    for ( i=0, bdf = sf->bitmaps; bdf!=NULL; bdf=bdf->next, ++i );
#if defined(FONTFORGE_CONFIG_GDRAW)
    gwwv_progress_start_indicator(10,_("Saving..."),_("Saving Spline Font Database"),_("Saving Outlines"),
	    realcnt,i+1);
    GProgressEnableStop(false);
#elif defined(FONTFORGE_CONFIG_GTK)
    gwwv_progress_start_indicator(10,_("Saving..."),_("Saving Spline Font Database"),_("Saving Outlines"),
	    realcnt,i+1);
    gwwv_progress_enable_stop(false);
#endif
    fprintf(sfd, "SplineFontDB: %.1f\n", 1.0 );
    if ( sf->mm != NULL )
	SFD_MMDump(sfd,sf->mm->normal,map,normal);
    else
	SFD_Dump(sfd,sf,map,normal);
#if defined(FONTFORGE_CONFIG_GDRAW)
    gwwv_progress_end_indicator();
#elif defined(FONTFORGE_CONFIG_GTK)
    gwwv_progress_end_indicator();
#endif
}

int SFDWrite(char *filename,SplineFont *sf,EncMap *map,EncMap *normal) {
    FILE *sfd = fopen(filename,"w");
    char *oldloc;

    if ( sfd==NULL )
return( 0 );
    oldloc = setlocale(LC_NUMERIC,"C");
    if ( sf->cidmaster!=NULL )
	sf=sf->cidmaster;
    SFDDump(sfd,sf,map,normal);
    setlocale(LC_NUMERIC,oldloc);
return( !ferror(sfd) && fclose(sfd)==0 );
}

int SFDWriteBak(SplineFont *sf,EncMap *map,EncMap *normal) {
    char *buf/*, *pt, *bpt*/;

    if ( sf->cidmaster!=NULL )
	sf=sf->cidmaster;
    buf = galloc(strlen(sf->filename)+10);
#if 1
    strcpy(buf,sf->filename);
    strcat(buf,"~");
#else
    pt = strrchr(sf->filename,'.');
    if ( pt==NULL || pt<strrchr(sf->filename,'/'))
	pt = sf->filename+strlen(sf->filename);
    strcpy(buf,sf->filename);
    bpt = buf + (pt-sf->filename);
    *bpt++ = '~';
    strcpy(bpt,pt);
#endif
    if ( rename(sf->filename,buf)==0 )
	sf->backedup = bs_backedup;
    free(buf);

return( SFDWrite(sf->filename,sf,map,normal));
}

/* ********************************* INPUT ********************************** */

static char *getquotedeol(FILE *sfd) {
    char *pt, *str, *end;
    int ch;

    pt = str = galloc(101); end = str+100;
    while ( isspace(ch = getc(sfd)) && ch!='\r' && ch!='\n' );
    while ( ch!='\n' && ch!='\r' && ch!=EOF ) {
	if ( ch=='\\' ) {
	    ch = getc(sfd);
	    if ( ch=='n' ) ch='\n';
	}
	if ( pt>=end ) {
	    pt = grealloc(str,end-str+101);
	    end = pt+(end-str)+100;
	    str = pt;
	    pt = end-100;
	}
	*pt++ = ch;
	ch = getc(sfd);
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

    while ( isspace(ch = getc(sfd)) && ch!='\r' && ch!='\n' );
    while ( ch!='\n' && ch!='\r' && ch!=EOF ) {
	if ( pt<end ) *pt++ = ch;
	ch = getc(sfd);
    }
    *pt='\0';
return( pt!=tokbuf?1:ch==EOF?-1: 0 );
}

static int getprotectedname(FILE *sfd, char *tokbuf) {
    char *pt=tokbuf, *end = tokbuf+100-2; int ch;

    while ( (ch = getc(sfd))==' ' || ch=='\t' );
    while ( ch!=EOF && !isspace(ch) && ch!='[' && ch!=']' && ch!='{' && ch!='}' && ch!='<' && ch!='%' ) {
	if ( pt<end ) *pt++ = ch;
	ch = getc(sfd);
    }
    if ( pt==tokbuf && ch!=EOF )
	*pt++ = ch;
    else
	ungetc(ch,sfd);
    *pt='\0';
return( pt!=tokbuf?1:ch==EOF?-1: 0 );
}

static int getname(FILE *sfd, char *tokbuf) {
    int ch;

    while ( isspace(ch = getc(sfd)));
    ungetc(ch,sfd);
return( getprotectedname(sfd,tokbuf));
}

static int getint(FILE *sfd, int *val) {
    char tokbuf[100]; int ch;
    char *pt=tokbuf, *end = tokbuf+100-2;

    while ( isspace(ch = getc(sfd)));
    if ( ch=='-' || ch=='+' ) {
	*pt++ = ch;
	ch = getc(sfd);
    }
    while ( isdigit(ch)) {
	if ( pt<end ) *pt++ = ch;
	ch = getc(sfd);
    }
    *pt='\0';
    ungetc(ch,sfd);
    *val = strtol(tokbuf,NULL,10);
return( pt!=tokbuf?1:ch==EOF?-1: 0 );
}

static int gethex(FILE *sfd, int *val) {
    char tokbuf[100]; int ch;
    char *pt=tokbuf, *end = tokbuf+100-2;

    while ( isspace(ch = getc(sfd)));
    if ( ch=='#' )
	ch = getc(sfd);
    if ( ch=='-' || ch=='+' ) {
	*pt++ = ch;
	ch = getc(sfd);
    }
    while ( isdigit(ch) || (ch>='a' && ch<='f') || (ch>='A' && ch<='F')) {
	if ( pt<end ) *pt++ = ch;
	ch = getc(sfd);
    }
    *pt='\0';
    ungetc(ch,sfd);
    *val = strtoul(tokbuf,NULL,16);
return( pt!=tokbuf?1:ch==EOF?-1: 0 );
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

    while ( isspace(ch = getc(sfd)));
    if ( ch!='e' && ch!='E' )		/* real's can't begin with exponants */
	while ( isdigit(ch) || ch=='-' || ch=='+' || ch=='e' || ch=='E' || ch=='.' || ch==',' ) {
	    if ( pt<end ) *pt++ = ch;
	    ch = getc(sfd);
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
    int width, height, image_type, bpl, clutlen, trans, rlelen;
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
    img = gcalloc(1,sizeof(ImageList));
    img->image = image;
    getreal(sfd,&img->xoff);
    getreal(sfd,&img->yoff);
    getreal(sfd,&img->xscale);
    getreal(sfd,&img->yscale);
    while ( (ch=getc(sfd))==' ' || ch=='\t' );
    ungetc(ch,sfd);
    rlelen = 0;
    if ( isdigit(ch))
	getint(sfd,&rlelen);
    base->trans = trans;
    if ( clutlen!=0 ) {
	if ( base->clut==NULL )
	    base->clut = gcalloc(1,sizeof(GClut));
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
	    if ( image_type==it_true ) {
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

static void SFDGetType1(FILE *sfd, SplineChar *sc) {
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
    sc->ttf_instrs = galloc(len);
    sc->ttf_instrs_len = len;
    for ( i=0; i<len; ++i )
	sc->ttf_instrs[i] = Dec85(&dec);
}

static struct ttf_table *SFDGetTtfTable(FILE *sfd, SplineFont *sf,struct ttf_table *lasttab[2]) {
    /* We've read the TtfTable token, it is followed by a tag and a byte count */
    /* and then the instructions in enc85 format */
    int i,len, ch;
    int which;
    struct enc85 dec;
    struct ttf_table *tab = chunkalloc(sizeof(struct ttf_table));

    memset(&dec,'\0', sizeof(dec)); dec.pos = -1;
    dec.sfd = sfd;

    while ( (ch=getc(sfd))==' ' );
    tab->tag = (ch<<24)|(getc(sfd)<<16);
    tab->tag |= getc(sfd)<<8;
    tab->tag |= getc(sfd);

    if ( tab->tag==CHR('f','p','g','m') || tab->tag==CHR('p','r','e','p') ||
	    tab->tag==CHR('c','v','t',' ') || tab->tag==CHR('m','a','x','p'))
	which = 0;
    else
	which = 1;

    getint(sfd,&len);
    tab->data = galloc(len);
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

static int SFDCloseCheck(SplinePointList *spl,int order2) {
    if ( spl->first!=spl->last &&
	    RealNear(spl->first->me.x,spl->last->me.x) &&
	    RealNear(spl->first->me.y,spl->last->me.y)) {
	SplinePoint *oldlast = spl->last;
	spl->first->prevcp = oldlast->prevcp;
	spl->first->noprevcp = false;
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
    forever {
	ch = getc(sfd);
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

static SplineSet *SFDGetSplineSet(SplineFont *sf,FILE *sfd) {
    SplinePointList *cur=NULL, *head=NULL;
    BasePoint current;
    real stack[100];
    int sp=0;
    SplinePoint *pt;
    int ch;
    char tok[100];
    int ttfindex = 0;

    current.x = current.y = 0;
    while ( 1 ) {
	while ( getreal(sfd,&stack[sp])==1 )
	    if ( sp<99 )
		++sp;
	while ( isspace(ch=getc(sfd)));
	if ( ch=='E' || ch=='e' || ch==EOF )
    break;
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
		    if ( cur!=NULL ) {
			if ( SFDCloseCheck(cur,sf->order2))
			    --ttfindex;
			cur->next = spl;
		    } else
			head = spl;
		    cur = spl;
		} else {
		    if ( cur!=NULL && cur->first!=NULL && (cur->first!=cur->last || cur->first->next==NULL) ) {
			if ( cur->last->nextcpindex==0xfffe )
			    cur->last->nextcpindex = 0xffff;
			SplineMake(cur->last,pt,sf->order2);
			cur->last = pt;
		    }
		}
	    } else
		sp = 0;
	} else if ( ch=='c' ) {
	    if ( sp>=6 ) {
		current.x = stack[sp-2];
		current.y = stack[sp-1];
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
		    SplineMake(cur->last,pt,sf->order2);
		    cur->last = pt;
		}
		sp -= 6;
	    } else
		sp = 0;
	}
	if ( pt!=NULL ) {
	    int val;
	    getint(sfd,&val);
	    pt->pointtype = (val&3);
	    pt->selected = val&4?1:0;
	    pt->nextcpdef = val&8?1:0;
	    pt->prevcpdef = val&0x10?1:0;
	    pt->roundx = val&0x20?1:0;
	    pt->roundy = val&0x40?1:0;
	    pt->dontinterpolate = val&0x100?1:0;
	    if ( val&0x80 )
		pt->ttfindex = 0xffff;
	    else
		pt->ttfindex = ttfindex++;
	    pt->nextcpindex = 0xfffe;
	    ch = getc(sfd);
	    if ( ch=='x' ) {
		pt->hintmask = chunkalloc(sizeof(HintMask));
		SFDGetHintMask(sfd,pt->hintmask);
	    } else if ( ch!=',' )
		ungetc(ch,sfd);
	    else {
		ch = getc(sfd);
		if ( ch==',' )
		    pt->ttfindex = 0xfffe;
		else {
		    ungetc(ch,sfd);
		    getint(sfd,&val);
		    pt->ttfindex = val;
		    getc(sfd);	/* skip comma */
		    if ( val!=-1 )
			ttfindex = val+1;
		}
		ch = getc(sfd);
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
	SFDCloseCheck(cur,sf->order2);
    getname(sfd,tok);
return( head );
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
	    mapping = gcalloc(pt,sizeof(SplinePoint *));
    }

    last = NULL;
    for ( ch=getc(sfd); ch!=EOF && ch!='\n'; ch=getc(sfd)) {
	err = false;
	while ( isspace(ch) && ch!='\n' ) ch=getc(sfd);
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
	ch = getc(sfd);
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
	    if ( last==NULL )
		mdhead = md;
	    else
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

    while ( (ch=getc(sfd))==' ' || ch=='\t' );
    if ( ch=='G' ) {
	stem->ghost = true;
	while ( (ch=getc(sfd))==' ' || ch=='\t' );
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
    while ( (ch=getc(sfd))==' ' || ch=='\t' );
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

static DStemInfo *SFDReadDHints(FILE *sfd) {
    DStemInfo *head=NULL, *last=NULL, *cur;
    DStemInfo d;

    memset(&d,'\0',sizeof(d));
    while ( getreal(sfd,&d.leftedgetop.x)==1 && getreal(sfd,&d.leftedgetop.y) &&
	    getreal(sfd,&d.rightedgetop.x)==1 && getreal(sfd,&d.rightedgetop.y) &&
	    getreal(sfd,&d.leftedgebottom.x)==1 && getreal(sfd,&d.leftedgebottom.y) &&
	    getreal(sfd,&d.rightedgebottom.x)==1 && getreal(sfd,&d.rightedgebottom.y) ) {
	cur = chunkalloc(sizeof(DStemInfo));
	*cur = d;
	if ( head == NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;
    }
    DStemInfosFree(head);
return( NULL );
}

#ifdef FONTFORGE_CONFIG_DEVICETABLES
static void SFDReadDeviceTable(FILE *sfd,DeviceTable *adjust) {
    int i, junk, first, last, ch, len;

    while ( (ch=getc(sfd))==' ' );
    if ( ch=='{' ) {
	while ( (ch=getc(sfd))==' ' );
	if ( ch=='}' )
return;
	else
	    ungetc(ch,sfd);
	getint(sfd,&first);
	ch = getc(sfd);		/* Should be '-' */
	getint(sfd,&last);
	len = last-first+1;
	if ( len<=0 ) {
	    IError( "Bad device table, invalid length.\n" );
return;
	}
	adjust->first_pixel_size = first;
	adjust->last_pixel_size = last;
	adjust->corrections = galloc(len);
	for ( i=0; i<len; ++i ) {
	    while ( (ch=getc(sfd))==' ' );
	    if ( ch!=',' ) ungetc(ch,sfd);
	    getint(sfd,&junk);
	    adjust->corrections[i] = junk;
	}
	while ( (ch=getc(sfd))==' ' );
	if ( ch!='}' ) ungetc(ch,sfd);
    } else
	ungetc(ch,sfd);
}

static ValDevTab *SFDReadValDevTab(FILE *sfd) {
    int i, j, ch;
    ValDevTab vdt;
    char buf[4];

    memset(&vdt,0,sizeof(vdt));
    buf[3] = '\0';
    while ( (ch=getc(sfd))==' ' );
    if ( ch=='[' ) {
	for ( i=0; i<4; ++i ) {
	    while ( (ch=getc(sfd))==' ' );
	    if ( ch==']' )
	break;
	    buf[0]=ch;
	    for ( j=1; j<3; ++j ) buf[j]=getc(sfd);
	    while ( (ch=getc(sfd))==' ' );
	    if ( ch!='=' ) ungetc(ch,sfd);
	    SFDReadDeviceTable(sfd,
		    strcmp(buf,"ddx")==0 ? &vdt.xadjust :
		    strcmp(buf,"ddy")==0 ? &vdt.yadjust :
		    strcmp(buf,"ddh")==0 ? &vdt.xadv :
		    strcmp(buf,"ddv")==0 ? &vdt.yadv :
			(&vdt.xadjust) + i );
	    while ( (ch=getc(sfd))==' ' );
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
#else
static void SFDSkipDeviceTable(FILE *sfd) {
    int i, junk, first, last, ch;

    while ( (ch=getc(sfd))==' ' );
    if ( ch=='{' ) {
	while ( (ch=getc(sfd))==' ' );
	if ( ch=='}' )
return;
	else
	    ungetc(ch,sfd);
	getint(sfd,&first);
	ch = getc(sfd);		/* Should be '-' */
	getint(sfd,&last);
	for ( i=0; i<=last-first; ++i ) {
	    while ( (ch=getc(sfd))==' ' );
	    if ( ch!=',' ) ungetc(ch,sfd);
	    getint(sfd,&junk);
	}
	while ( (ch=getc(sfd))==' ' );
	if ( ch!='}' ) ungetc(ch,sfd);
    } else
	ungetc(ch,sfd);
}

static void SFDSkipValDevTab(FILE *sfd) {
    int i, j, ch;

    while ( (ch=getc(sfd))==' ' );
    if ( ch=='[' ) {
	for ( i=0; i<4; ++i ) {
	    while ( (ch=getc(sfd))==' ' );
	    if ( ch==']' )
	break;
	    for ( j=0; j<3; ++j ) ch=getc(sfd);
	    SFDSkipDeviceTable(sfd);
	    while ( (ch=getc(sfd))==' ' );
	    if ( ch!=']' ) ungetc(ch,sfd);
	    else
	break;
	}
    } else
	ungetc(ch,sfd);
}
#endif

static AnchorPoint *SFDReadAnchorPoints(FILE *sfd,SplineChar *sc,AnchorPoint *lastap) {
    AnchorPoint *ap = chunkalloc(sizeof(AnchorPoint));
    AnchorClass *an;
    char *name;
    char tok[200];
    int ch;

    name = SFDReadUTF7Str(sfd);
    if ( name==NULL ) {
	LogError( "Anchor Point with no class name" );
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
    ch = getc(sfd);
    ungetc(ch,sfd);
    if ( ch==' ' ) {
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	SFDReadDeviceTable(sfd,&ap->xadjust);
	SFDReadDeviceTable(sfd,&ap->yadjust);
#else
	SFDSkipDeviceTable(sfd);
	SFDSkipDeviceTable(sfd);
#endif
	ch = getc(sfd);
	ungetc(ch,sfd);
	if ( isdigit(ch)) {
	    getsint(sfd,(int16 *) &ap->ttf_pt_index);
	    ap->has_ttf_pt = true;
	}
    }
    if ( ap->anchor==NULL || ap->type==-1 ) {
	LogError( "Bad Anchor Point" );
	AnchorPointsFree(ap);
return( lastap );
    }
    if ( lastap==NULL )
	sc->anchor = ap;
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
    while ( isspace(ch=getc(sfd)));
    if ( ch=='S' ) rf->selected = true;
    getreal(sfd,&rf->transform[0]);
    getreal(sfd,&rf->transform[1]);
    getreal(sfd,&rf->transform[2]);
    getreal(sfd,&rf->transform[3]);
    getreal(sfd,&rf->transform[4]);
    getreal(sfd,&rf->transform[5]);
    while ( (ch=getc(sfd))==' ');
    ungetc(ch,sfd);
    if ( isdigit(ch) ) {
	getint(sfd,&temp);
	rf->use_my_metrics = temp&1;
	rf->round_translation_to_grid = (temp&2)?1:0;
	rf->point_match = (temp&4)?1:0;
	if ( rf->point_match ) {
	    getsint(sfd,(int16 *) &rf->match_pt_base);
	    getsint(sfd,(int16 *) &rf->match_pt_ref);
	    while ( (ch=getc(sfd))==' ');
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
/* Now I want to have seperate ligature structures for each */
static PST *LigaCreateFromOldStyleMultiple(PST *liga) {
    char *pt;
    PST *new, *last=liga;
    while ( (pt = strrchr(liga->u.lig.components,';'))!=NULL ) {
	new = chunkalloc(sizeof( PST ));
	*new = *liga;
	new->u.lig.components = copy(pt+1);
	last->next = new;
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

static void CvtOldMacFeature(PST *pst) {
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
	map->backmap = grealloc(map->backmap,map->backmax*sizeof(int));
	memset(map->backmap+old,-1,(map->backmax-old)*sizeof(int));
    }
    if ( map->backmap[orig_pos] == -1 )		/* backmap will not be unique if multiple encodings come from same glyph */
	map->backmap[orig_pos] = enc;
    if ( enc>=map->encmax ) {
	int old = map->encmax;
	map->encmax = enc+10;
	map->map = grealloc(map->map,map->encmax*sizeof(int));
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

static int orig_pos;

static SplineChar *SFDGetChar(FILE *sfd,SplineFont *sf) {
    SplineChar *sc;
    char tok[2000], ch;
    RefChar *lastr=NULL, *ref;
    ImageList *lasti=NULL, *img;
    AnchorPoint *lastap = NULL;
    int isliga, ispos, issubs, ismult, islcar, ispair, temp, i;
    PST *last = NULL;
    uint32 script = 0;
    int current_layer = ly_fore;
    int multilayer = sf->multilayer;
    SplineFont *sli_sf = sf->cidmaster ? sf->cidmaster : sf;
    struct altuni *altuni;

    if ( getname(sfd,tok)!=1 )
return( NULL );
    if ( strcmp(tok,"StartChar:")!=0 )
return( NULL );
    if ( getname(sfd,tok)!=1 )
return( NULL );

    sc = SplineCharCreate();
    sc->name = copy(tok);
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
	    while ( (ch=getc(sfd))==' ' || ch=='\t' );
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
			sf->glyphs = grealloc(sf->glyphs,(sf->glyphmax = sc->orig_pos+10)*sizeof(SplineChar *));
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
		altuni->next = sc->altuni;
		sc->altuni = altuni;
	    }
	} else if ( strmatch(tok,"OldEncoding:")==0 ) {
	    int old_enc;		/* Obsolete info */
	    getint(sfd,&old_enc);
        } else if ( strmatch(tok,"Script:")==0 ) {
	    /* Obsolete. But still used for parsing obsolete ligature/subs tags */
            while ( (ch=getc(sfd))==' ' || ch=='\t' );
            if ( ch=='\n' || ch=='\r' )
                script = 0;
            else {
                script = ch<<24;
                script |= (getc(sfd)<<16);
                script |= (getc(sfd)<<8);
                script |= getc(sfd);
            }
	} else if ( strmatch(tok,"Width:")==0 ) {
	    getsint(sfd,&sc->width);
	} else if ( strmatch(tok,"VWidth:")==0 ) {
	    getsint(sfd,&sc->vwidth);
	} else if ( strmatch(tok,"GlyphClass:")==0 ) {
	    getint(sfd,&temp);
	    sc->glyph_class = temp;
	} else if ( strmatch(tok,"Flags:")==0 ) {
	    while ( isspace(ch=getc(sfd)) && ch!='\n' && ch!='\r');
	    while ( ch!='\n' && ch!='\r' ) {
		if ( ch=='H' ) sc->changedsincelasthinted=true;
		else if ( ch=='M' ) sc->manualhints = true;
		else if ( ch=='W' ) sc->widthset = true;
		else if ( ch=='O' ) sc->wasopen = true;
		else if ( ch=='I' ) sc->instructions_out_of_date = true;
		ch = getc(sfd);
	    }
	    if ( sf->multilayer || sf->onlybitmaps || sf->strokedfont || sf->order2 )
		sc->changedsincelasthinted = false;
	} else if ( strmatch(tok,"TeX:")==0 ) {
	    getsint(sfd,&sc->tex_height);
	    getsint(sfd,&sc->tex_depth);
	    getsint(sfd,&sc->tex_sub_pos);
	    getsint(sfd,&sc->tex_super_pos);
#if HANYANG
	} else if ( strmatch(tok,"CompositionUnit:")==0 ) {
	    getsint(sfd,&sc->jamo);
	    getsint(sfd,&sc->varient);
	    sc->compositionunit = true;
#endif
	} else if ( strmatch(tok,"HStem:")==0 ) {
	    sc->hstem = SFDReadHints(sfd);
	    SCGuessHHintInstancesList(sc);		/* For reading in old .sfd files with no HintInstance data */
	    sc->hconflicts = StemListAnyConflicts(sc->hstem);
	} else if ( strmatch(tok,"VStem:")==0 ) {
	    sc->vstem = SFDReadHints(sfd);
	    SCGuessVHintInstancesList(sc);		/* For reading in old .sfd files */
	    sc->vconflicts = StemListAnyConflicts(sc->vstem);
	} else if ( strmatch(tok,"DStem:")==0 ) {
	    sc->dstem = SFDReadDHints(sfd);
	} else if ( strmatch(tok,"CounterMasks:")==0 ) {
	    getsint(sfd,&sc->countermask_cnt);
	    sc->countermasks = gcalloc(sc->countermask_cnt,sizeof(HintMask));
	    for ( i=0; i<sc->countermask_cnt; ++i )
		SFDGetHintMask(sfd,&sc->countermasks[i]);
	} else if ( strmatch(tok,"AnchorPoint:")==0 ) {
	    lastap = SFDReadAnchorPoints(sfd,sc,lastap);
	} else if ( strmatch(tok,"Fore")==0 ) {
	    sc->layers[ly_fore].splines = SFDGetSplineSet(sf,sfd);
	    current_layer = ly_fore;
	} else if ( strmatch(tok,"MinimumDistance:")==0 ) {
	    SFDGetMinimumDistances(sfd,sc);
	} else if ( strmatch(tok,"Back")==0 ) {
	    while ( isspace(ch=getc(sfd)));
	    ungetc(ch,sfd);
	    if ( ch!='I' )
		sc->layers[ly_back].splines = SFDGetSplineSet(sf,sfd);
	    current_layer = ly_back;
#ifdef FONTFORGE_CONFIG_TYPE3
	} else if ( strmatch(tok,"LayerCount:")==0 ) {
	    getint(sfd,&temp);
	    if ( temp>sc->layer_cnt ) {
		sc->layers = grealloc(sc->layers,temp*sizeof(Layer));
		memset(sc->layers+sc->layer_cnt,0,(temp-sc->layer_cnt)*sizeof(Layer));
	    }
	    sc->layer_cnt = temp;
	    current_layer = ly_fore;
	} else if ( strmatch(tok,"Layer:")==0 ) {
	    int layer, dofill, dostroke, fillfirst, fillcol, strokecol, linejoin, linecap;
	    real fillopacity, strokeopacity, strokewidth, trans[4];
	    DashType dashes[DASH_MAX];
	    int i;
	    getint(sfd,&layer);
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
	    if ( layer>=sc->layer_cnt ) {
		sc->layers = grealloc(sc->layers,(layer+1)*sizeof(Layer));
		memset(sc->layers+sc->layer_cnt,0,(layer+1-sc->layer_cnt)*sizeof(Layer));
	    }
	    while ( (ch=getc(sfd))==' ' || ch=='[' );
	    ungetc(ch,sfd);
	    getreal(sfd,&trans[0]);
	    getreal(sfd,&trans[1]);
	    getreal(sfd,&trans[2]);
	    getreal(sfd,&trans[3]);
	    while ( (ch=getc(sfd))==' ' || ch==']' );
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
	    current_layer = layer;
	    sc->layers[current_layer].dofill = dofill;
	    sc->layers[current_layer].dostroke = dostroke;
	    sc->layers[current_layer].fillfirst = fillfirst;
	    sc->layers[current_layer].fill_brush.col = fillcol;
	    sc->layers[current_layer].fill_brush.opacity = fillopacity;
	    sc->layers[current_layer].stroke_pen.brush.col = strokecol;
	    sc->layers[current_layer].stroke_pen.brush.opacity = strokeopacity;
	    sc->layers[current_layer].stroke_pen.width = strokewidth;
	    sc->layers[current_layer].stroke_pen.linejoin = linejoin;
	    sc->layers[current_layer].stroke_pen.linecap = linecap;
	    memcpy(sc->layers[current_layer].stroke_pen.dashes,dashes,sizeof(dashes));
	    memcpy(sc->layers[current_layer].stroke_pen.trans,trans,sizeof(trans));
	    lasti = NULL;
	    lastr = NULL;
	} else if ( strmatch(tok,"SplineSet")==0 ) {
	    sc->layers[current_layer].splines = SFDGetSplineSet(sf,sfd);
#endif
	} else if ( strmatch(tok,"Ref:")==0 || strmatch(tok,"Refer:")==0 ) {
	    if ( !multilayer ) current_layer = ly_fore;
	    ref = SFDGetRef(sfd,strmatch(tok,"Ref:")==0);
	    if ( sc->layers[current_layer].refs==NULL )
		sc->layers[current_layer].refs = ref;
	    else
		lastr->next = ref;
	    lastr = ref;
	} else if ( strmatch(tok,"Image:")==0 ) {
	    if ( !multilayer ) current_layer = ly_back;
	    img = SFDGetImage(sfd);
	    if ( sc->layers[current_layer].images==NULL )
		sc->layers[current_layer].images = img;
	    else
		lasti->next = img;
	    lasti = img;
	} else if ( strmatch(tok,"OrigType1:")==0 ) {	/* Accept, slurp, ignore contents */
	    SFDGetType1(sfd,sc);
	} else if ( strmatch(tok,"TtfInstrs:")==0 ) {
	    SFDGetTtfInstrs(sfd,sc);
	} else if ( strmatch(tok,"Kerns:")==0 ||
		strmatch(tok,"KernsSLI:")==0 ||
		strmatch(tok,"KernsSLIF:")==0 ||
		strmatch(tok,"VKernsSLIF:")==0 ||
		strmatch(tok,"KernsSLIFO:")==0 ||
		strmatch(tok,"VKernsSLIFO:")==0 ) {
	    KernPair *kp, *last=NULL;
	    int index, off, sli, flags=0;
	    int hassli = (strmatch(tok,"KernsSLI:")==0);
	    int isv = *tok=='V';
	    int has_orig = strstr(tok,"SLIFO:")!=NULL;
	    if ( strmatch(tok,"KernsSLIF:")==0 || strmatch(tok,"KernsSLIFO:")==0 ||
		    strmatch(tok,"VKernsSLIF:")==0 || strmatch(tok,"VKernsSLIFO:")==0 )
		hassli=2;
	    while ( (hassli==1 && fscanf(sfd,"%d %d %d", &index, &off, &sli )==3) ||
		    (hassli==2 && fscanf(sfd,"%d %d %d %d", &index, &off, &sli, &flags )==4) ||
		    (hassli==0 && fscanf(sfd,"%d %d", &index, &off )==2) ) {
		if ( !hassli )
		    sli = SFFindBiggestScriptLangIndex(sf,
			    script!=0?script:SCScriptFromUnicode(sc),DEFAULT_LANG);
		if ( sli>=sli_sf->sli_cnt && sli!=SLI_NESTED) {
		    static int complained=false;
		    if ( !complained )
			IError("'%s' in %s has a script index out of bounds: %d",
				isv ? "vkrn" : "kern",
				sc->name, sli );
		    else
			IError( "'%s' in %s has a script index out of bounds: %d",
				isv ? "vkrn" : "kern",
				sc->name, sli );
		    sli = SFFindBiggestScriptLangIndex(sli_sf,
			    SCScriptFromUnicode(sc),DEFAULT_LANG);
		    complained = true;
		}
		kp = chunkalloc(sizeof(KernPair));
		kp->sc = (SplineChar *) (intpt) index;
		kp->kcid = has_orig;
		kp->off = off;
		kp->sli = sli;
		kp->flags = flags;
		kp->next = NULL;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		while ( (ch=getc(sfd))==' ' );
		ungetc(ch,sfd);
		if ( ch=='{' ) {
		    kp->adjust=chunkalloc(sizeof(DeviceTable));
		    SFDReadDeviceTable(sfd,kp->adjust);
		}
#else
		SFDSkipDeviceTable(sfd);
#endif
		if ( last != NULL )
		    last->next = kp;
		else if ( isv )
		    sc->vkerns = kp;
		else
		    sc->kerns = kp;
		last = kp;
	    }
	} else if ( (ispos = (strmatch(tok,"Position:")==0)) ||
		( ispair = (strmatch(tok,"PairPos:")==0)) ||
		( islcar = (strmatch(tok,"LCarets:")==0)) ||
		( isliga = (strmatch(tok,"Ligature:")==0)) ||
		( issubs = (strmatch(tok,"Substitution:")==0)) ||
		( ismult = (strmatch(tok,"MultipleSubs:")==0)) ||
		strmatch(tok,"AlternateSubs:")==0 ) {
	    PST *liga = chunkalloc(sizeof(PST));
	    if ( last==NULL )
		sc->possub = liga;
	    else
		last->next = liga;
	    last = liga;
	    liga->type = ispos ? pst_position :
			 ispair ? pst_pair :
			 islcar ? pst_lcaret :
			 isliga ? pst_ligature :
			 issubs ? pst_substitution :
			 ismult ? pst_multiple :
			 pst_alternate;
	    liga->tag = CHR('l','i','g','a');
	    liga->script_lang_index = 0xffff;
	    while ( (ch=getc(sfd))==' ' || ch=='\t' );
	    if ( isdigit(ch)) {
		int temp;
		ungetc(ch,sfd);
		getint(sfd,&temp);
		liga->flags = temp;
		while ( (ch=getc(sfd))==' ' || ch=='\t' );
	    } else
		liga->flags = PSTDefaultFlags(liga->type,sc);
	    if ( isdigit(ch)) {
		ungetc(ch,sfd);
		getusint(sfd,&liga->script_lang_index);
		while ( (ch=getc(sfd))==' ' || ch=='\t' );
	    } else
		liga->script_lang_index = SFFindBiggestScriptLangIndex(sf,
			script!=0?script:SCScriptFromUnicode(sc),DEFAULT_LANG);
	    if ( ch=='\'' ) {
		liga->tag = getc(sfd)<<24;
		liga->tag |= getc(sfd)<<16;
		liga->tag |= getc(sfd)<<8;
		liga->tag |= getc(sfd);
		getc(sfd);	/* Final quote */
	    } else if ( ch=='<' ) {
		getint(sfd,&temp);
		liga->tag = temp<<16;
		getc(sfd);	/* comma */
		getint(sfd,&temp);
		liga->tag |= temp;
		getc(sfd);	/* close '>' */
		liga->macfeature = true;
	    } else
		ungetc(ch,sfd);
	    if ( liga->type == pst_lcaret ) {
		/* These are meaningless for lcarets, set them to innocuous values */
		liga->script_lang_index = SLI_UNKNOWN;
		liga->tag = CHR(' ',' ',' ',' ');
	    } else if ( liga->script_lang_index>=sli_sf->sli_cnt && liga->script_lang_index!=SLI_NESTED ) {
		static int complained=false;
		if ( !complained )
		    IError("'%c%c%c%c' in %s has a script index out of bounds: %d",
			    (liga->tag>>24), (liga->tag>>16)&0xff, (liga->tag>>8)&0xff, liga->tag&0xff,
			    sc->name, liga->script_lang_index );
		else
		    IError( "'%c%c%c%c' in %s has a script index out of bounds: %d\n",
			    (liga->tag>>24), (liga->tag>>16)&0xff, (liga->tag>>8)&0xff, liga->tag&0xff,
			    sc->name, liga->script_lang_index );
		liga->script_lang_index = SFFindBiggestScriptLangIndex(sli_sf,
			SCScriptFromUnicode(sc),DEFAULT_LANG);
		complained = true;
	    }
	    if ( liga->type==pst_position ) {
		fscanf( sfd, " dx=%hd dy=%hd dh=%hd dv=%hd",
			&liga->u.pos.xoff, &liga->u.pos.yoff,
			&liga->u.pos.h_adv_off, &liga->u.pos.v_adv_off);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		liga->u.pos.adjust = SFDReadValDevTab(sfd);
#else
		SFDSkipValDevTab(sfd);
#endif
		ch = getc(sfd);		/* Eat new line */
	    } else if ( liga->type==pst_pair ) {
		getname(sfd,tok);
		liga->u.pair.paired = copy(tok);
		liga->u.pair.vr = chunkalloc(sizeof(struct vr [2]));
		fscanf( sfd, " dx=%hd dy=%hd dh=%hd dv=%hd",
			&liga->u.pair.vr[0].xoff, &liga->u.pair.vr[0].yoff,
			&liga->u.pair.vr[0].h_adv_off, &liga->u.pair.vr[0].v_adv_off);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		liga->u.pair.vr[0].adjust = SFDReadValDevTab(sfd);
#else
		SFDSkipValDevTab(sfd);
#endif
		fscanf( sfd, " dx=%hd dy=%hd dh=%hd dv=%hd",
			&liga->u.pair.vr[1].xoff, &liga->u.pair.vr[1].yoff,
			&liga->u.pair.vr[1].h_adv_off, &liga->u.pair.vr[1].v_adv_off);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		liga->u.pair.vr[0].adjust = SFDReadValDevTab(sfd);
#else
		SFDSkipValDevTab(sfd);
#endif
		ch = getc(sfd);
	    } else if ( liga->type==pst_lcaret ) {
		int i;
		fscanf( sfd, " %d", &liga->u.lcaret.cnt );
		liga->u.lcaret.carets = galloc(liga->u.lcaret.cnt*sizeof(int16));
		for ( i=0; i<liga->u.lcaret.cnt; ++i )
		    fscanf( sfd, " %hd", &liga->u.lcaret.carets[i]);
		geteol(sfd,tok);
	    } else {
		geteol(sfd,tok);
		liga->u.lig.components = copy(tok);	/* it's in the same place for all formats */
		if ( isliga ) {
		    liga->u.lig.lig = sc;
		    last = LigaCreateFromOldStyleMultiple(liga);
		}
	    }
#ifdef FONTFORGE_CONFIG_CVT_OLD_MAC_FEATURES
	    CvtOldMacFeature(liga);
#endif
	} else if ( strmatch(tok,"Colour:")==0 ) {
	    int temp;
	    gethex(sfd,&temp);
	    sc->color = temp;
	} else if ( strmatch(tok,"Comment:")==0 ) {
	    sc->comment = SFDReadUTF7Str(sfd);
	} else if ( strmatch(tok,"EndChar")==0 ) {
	    if ( sc->orig_pos<sf->glyphcnt )
		sf->glyphs[sc->orig_pos] = sc;
#if 0		/* Auto recovery fails if we do this */
	    else {
		SplineCharFree(sc);
		sc = NULL;
	    }
#endif
	    if ( sf->order2 )
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
    bdf->props = galloc(pcnt*sizeof(BDFProperties));
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

    bfc = chunkalloc(sizeof(BDFChar));
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
    while ( (ch=getc(sfd))==' ');
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
	while ( (ch=getc(sfd))==' ');
	ungetc(ch,sfd);
	if ( ch!='\n' && ch!='\r' )
	    getint(sfd,&vwidth);
    }
    if ( enc<0 ||xmax<xmin || ymax<ymin )
return( 0 );
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
    bfc->bitmap = gcalloc((bfc->ymax-bfc->ymin+1)*bfc->bytes_per_line,sizeof(char));

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

static int SFDGetBitmapFont(FILE *sfd,SplineFont *sf) {
    BDFFont *bdf, *prev;
    char tok[200];
    int pixelsize, ascent, descent, depth=1;
    int ch, enccount;

    bdf = gcalloc(1,sizeof(BDFFont));

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
    while ( (ch = getc(sfd))==' ' );
    ungetc(ch,sfd);		/* old sfds don't have a foundry */
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
    bdf->glyphs = gcalloc(bdf->glyphcnt,sizeof(BDFChar *));

    while ( getname(sfd,tok)==1 ) {
	if ( strcmp(tok,"BDFStartProperties:")==0 )
	    SFDGetBitmapProps(sfd,bdf,tok);
	else if ( strcmp(tok,"BDFEndProperties")==0 )
	    /* Do Nothing */;
	else if ( strcmp(tok,"Resolution:")==0 )
	    getint(sfd,&bdf->res);
	else if ( strcmp(tok,"BDFChar:")==0 )
	    SFDGetBitmapChar(sfd,bdf);
	else if ( strcmp(tok,"EndBitmapFont")==0 )
    break;
    }
return( 1 );
}

static void SFDFixupRef(SplineChar *sc,RefChar *ref) {
    RefChar *rf;

    for ( rf = ref->sc->layers[ly_fore].refs; rf!=NULL; rf=rf->next ) {
	if ( rf->sc==sc ) {	/* Huh? */
	    ref->sc->layers[ly_fore].refs = NULL;
    break;
	}
	if ( rf->layers[0].splines==NULL )
	    SFDFixupRef(ref->sc,rf);
    }
    SCReinstanciateRefChar(sc,ref);
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
	int len = strlen(basename);
	if ( strncmp(sc->name,basename,len)==0 &&
		(sc->name[len]=='.' || sc->name[len]=='\0'))
	    matched = sc->layers[ly_fore].refs->sc;
	sc = sc->layers[ly_fore].refs->sc;
    }
return( matched );
}

static void SFDFixupRefs(SplineFont *sf) {
    int i, isv;
    RefChar *refs, *rnext, *rprev;
    /*int isautorecovery = sf->changed;*/
    KernPair *kp, *prev, *next;
    EncMap *map = sf->map;
    int layer;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	SplineChar *sc = sf->glyphs[i];
	/* A changed character is one that has just been recovered */
	/*  unchanged characters will already have been fixed up */
	/* Er... maybe not. If the character being recovered is refered to */
	/*  by another character then we need to fix up that other char too*/
	/*if ( isautorecovery && !sc->changed )*/
    /*continue;*/
	for ( layer = ly_fore; layer<sc->layer_cnt; ++layer ) {
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
		if ( !kp->kcid ) {	/* It's encoded (old sfds), else orig */
		    if ( ((intpt) (kp->sc))>=map->encmax || map->map[(intpt) (kp->sc)]==-1 )
			index = sf->glyphcnt;
		    else
			index = map->map[(intpt) (kp->sc)];
		}
		if ( index>=sf->glyphcnt ) {
		    IError( "Bad kerning information in glyph %s\n", sc->name );
		    kp->sc = NULL;
		} else
		    kp->sc = sf->glyphs[index];
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
	for ( refs = sf->glyphs[i]->layers[ly_fore].refs; refs!=NULL; refs=refs->next ) {
	    SFDFixupRef(sf->glyphs[i],refs);
	}
#if defined(FONTFORGE_CONFIG_GDRAW)
	gwwv_progress_next();
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_progress_next();
#endif
    }
    if ( sf->cidmaster==NULL )
	for ( i=sf->glyphcnt-1; i>=0 && sf->glyphs[i]==NULL; --i )
	    sf->glyphcnt = i;
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

    sf->private = gcalloc(1,sizeof(struct psdict));
    getint(sfd,&cnt);
    sf->private->next = sf->private->cnt = cnt;
    sf->private->values = gcalloc(cnt,sizeof(char *));
    sf->private->keys = gcalloc(cnt,sizeof(char *));
    for ( i=0; i<cnt; ++i ) {
	getname(sfd,name);
	sf->private->keys[i] = copy(name);
	getint(sfd,&len);
	getc(sfd);	/* skip space */
	pt = sf->private->values[i] = galloc(len+1);
	for ( end = pt+len; pt<end; ++pt )
	    *pt = getc(sfd);
	*pt='\0';
    }
}

static void SFDGetSubrs(FILE *sfd,SplineFont *sf) {
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

static void SFDGetDesignSize(FILE *sfd,SplineFont *sf) {
    int ch;
    struct otfname *cur;

    getsint(sfd,(int16 *) &sf->design_size);
    while ( (ch=getc(sfd))==' ' );
    ungetc(ch,sfd);
    if ( isdigit(ch)) {
	getsint(sfd,(int16 *) &sf->design_range_bottom);
	while ( (ch=getc(sfd))==' ' );
	if ( ch!='-' )
	    ungetc(ch,sfd);
	getsint(sfd,(int16 *) &sf->design_range_top);
	getsint(sfd,(int16 *) &sf->fontstyle_id);
	forever {
	    while ( (ch=getc(sfd))==' ' );
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

static Encoding *SFDGetEncoding(FILE *sfd, char *tok, SplineFont *sf) {
    Encoding *enc = NULL;
    int encname;

    if ( getint(sfd,&encname) ) {
	if ( encname<sizeof(charset_names)/sizeof(charset_names[0])-1 )
	    enc = FindOrMakeEncoding(charset_names[encname]);
    } else {
	geteol(sfd,tok);
	enc = FindOrMakeEncoding(tok);
    }
    if ( enc==NULL )
	enc = &custom;
#if 0
    if ( enc->is_compact )
	sf->compacted = true;
#endif
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
	LogError( "Failed to find NameList: %s", tok);
    else
	sf->for_new_glyphs = nl;
}

static int SFAddScriptIndex(SplineFont *sf,uint32 *scripts,int scnt) {
    int i,j;
    struct script_record *sr;

    if ( scnt==0 )
	scripts[scnt++] = CHR('l','a','t','n');		/* Need a default script preference */
    for ( i=0; i<scnt-1; ++i ) for ( j=i+1; j<scnt; ++j ) {
	if ( scripts[i]>scripts[j] ) {
	    uint32 temp = scripts[i];
	    scripts[i] = scripts[j];
	    scripts[j] = temp;
	}
    }

    if ( sf->cidmaster ) sf = sf->cidmaster;
    if ( sf->script_lang==NULL )	/* It's an old sfd file */
	sf->script_lang = gcalloc(1,sizeof(struct script_record *));
    for ( i=0; sf->script_lang[i]!=NULL; ++i ) {
	sr = sf->script_lang[i];
	for ( j=0; sr[j].script!=0 && j<scnt &&
		sr[j].script==scripts[j]; ++j );
	if ( sr[j].script==0 && j==scnt )
return( i );
    }

    sf->script_lang = grealloc(sf->script_lang,(i+2)*sizeof(struct script_record *));
    sf->script_lang[i+1] = NULL;
    sr = sf->script_lang[i] = gcalloc(scnt+1,sizeof(struct script_record));
    for ( j = 0; j<scnt; ++j ) {
	sr[j].script = scripts[j];
	sr[j].langs = galloc(2*sizeof(uint32));
	sr[j].langs[0] = DEFAULT_LANG;
	sr[j].langs[1] = 0;
    }
return( i );
}

static void SFDParseChainContext(FILE *sfd,SplineFont *sf,FPST *fpst, char *tok) {
    int ch, i, j, k, temp;
    SplineFont *sli_sf = sf->cidmaster ? sf->cidmaster : sf;

    fpst->type = strmatch(tok,"ContextPos:")==0 ? pst_contextpos :
		strmatch(tok,"ContextSub:")==0 ? pst_contextsub :
		strmatch(tok,"ChainPos:")==0 ? pst_chainpos :
		strmatch(tok,"ChainSub:")==0 ? pst_chainsub : pst_reversesub;
    getname(sfd,tok);
    fpst->format = strmatch(tok,"glyph")==0 ? pst_glyphs :
		    strmatch(tok,"class")==0 ? pst_class :
		    strmatch(tok,"coverage")==0 ? pst_coverage : pst_reversecoverage;
    fscanf(sfd, "%hu %hu", &fpst->flags, &fpst->script_lang_index );
    if ( fpst->script_lang_index>=sli_sf->sli_cnt && fpst->script_lang_index!=SLI_NESTED ) {
	static int complained=false;
	if ( sli_sf->sli_cnt==0 )
	    IError("'%c%c%c%c' has a script index out of bounds: %d\nYou MUST fix this manually",
		    (fpst->tag>>24), (fpst->tag>>16)&0xff, (fpst->tag>>8)&0xff, fpst->tag&0xff,
		    fpst->script_lang_index );
	else if ( !complained )
	    IError("'%c%c%c%c' has a script index out of bounds: %d",
		    (fpst->tag>>24), (fpst->tag>>16)&0xff, (fpst->tag>>8)&0xff, fpst->tag&0xff,
		    fpst->script_lang_index );
	else
	    IError("'%c%c%c%c' has a script index out of bounds: %d\n",
		    (fpst->tag>>24), (fpst->tag>>16)&0xff, (fpst->tag>>8)&0xff, fpst->tag&0xff,
		    fpst->script_lang_index );
	if ( sli_sf->sli_cnt!=0 )
	    fpst->script_lang_index = sli_sf->sli_cnt-1;
	complained = true;
    }
    while ( (ch=getc(sfd))==' ' || ch=='\t' );
    if ( ch=='\'' ) {
	fpst->tag = getc(sfd)<<24;
	fpst->tag |= getc(sfd)<<16;
	fpst->tag |= getc(sfd)<<8;
	fpst->tag |= getc(sfd);
	getc(sfd);	/* Final quote */
    } else
	ungetc(ch,sfd);
    fscanf(sfd, "%hu %hu %hu %hu", &fpst->nccnt, &fpst->bccnt, &fpst->fccnt, &fpst->rule_cnt );
    if ( fpst->nccnt!=0 || fpst->bccnt!=0 || fpst->fccnt!=0 ) {
	fpst->nclass = galloc(fpst->nccnt*sizeof(char *));
	if ( fpst->nccnt!=0 ) fpst->nclass[0] = NULL;
	if (fpst->nccnt!=0 ) fpst->nclass[0] = NULL;
	if ( fpst->bccnt!=0 || fpst->fccnt!=0 ) {
	    fpst->bclass = galloc(fpst->bccnt*sizeof(char *));
	    if (fpst->bccnt!=0 ) fpst->bclass[0] = NULL;
	    fpst->fclass = galloc(fpst->fccnt*sizeof(char *));
	    if (fpst->fccnt!=0 ) fpst->fclass[0] = NULL;
	}
    }

    for ( j=0; j<3; ++j ) {
	for ( i=1; i<(&fpst->nccnt)[j]; ++i ) {
	    getname(sfd,tok);
	    getint(sfd,&temp);
	    (&fpst->nclass)[j][i] = galloc(temp+1); (&fpst->nclass)[j][i][temp] = '\0';
	    getc(sfd);	/* skip space */
	    fread((&fpst->nclass)[j][i],1,temp,sfd);
	}
    }

    fpst->rules = gcalloc(fpst->rule_cnt,sizeof(struct fpst_rule));
    for ( i=0; i<fpst->rule_cnt; ++i ) {
	switch ( fpst->format ) {
	  case pst_glyphs:
	    for ( j=0; j<3; ++j ) {
		getname(sfd,tok);
		getint(sfd,&temp);
		(&fpst->rules[i].u.glyph.names)[j] = galloc(temp+1);
		(&fpst->rules[i].u.glyph.names)[j][temp] = '\0';
		getc(sfd);	/* skip space */
		fread((&fpst->rules[i].u.glyph.names)[j],1,temp,sfd);
	    }
	  break;
	  case pst_class:
	    fscanf( sfd, "%d %d %d", &fpst->rules[i].u.class.ncnt, &fpst->rules[i].u.class.bcnt, &fpst->rules[i].u.class.fcnt );
	    for ( j=0; j<3; ++j ) {
		getname(sfd,tok);
		(&fpst->rules[i].u.class.nclasses)[j] = galloc((&fpst->rules[i].u.class.ncnt)[j]*sizeof(uint16));
		for ( k=0; k<(&fpst->rules[i].u.class.ncnt)[j]; ++k ) {
		    getusint(sfd,&(&fpst->rules[i].u.class.nclasses)[j][k]);
		}
	    }
	  break;
	  case pst_coverage:
	  case pst_reversecoverage:
	    fscanf( sfd, "%d %d %d", &fpst->rules[i].u.coverage.ncnt, &fpst->rules[i].u.coverage.bcnt, &fpst->rules[i].u.coverage.fcnt );
	    for ( j=0; j<3; ++j ) {
		(&fpst->rules[i].u.coverage.ncovers)[j] = galloc((&fpst->rules[i].u.coverage.ncnt)[j]*sizeof(char *));
		for ( k=0; k<(&fpst->rules[i].u.coverage.ncnt)[j]; ++k ) {
		    getname(sfd,tok);
		    getint(sfd,&temp);
		    (&fpst->rules[i].u.coverage.ncovers)[j][k] = galloc(temp+1);
		    (&fpst->rules[i].u.coverage.ncovers)[j][k][temp] = '\0';
		    getc(sfd);	/* skip space */
		    fread((&fpst->rules[i].u.coverage.ncovers)[j][k],1,temp,sfd);
		}
	    }
	  break;
	}
	switch ( fpst->format ) {
	  case pst_glyphs:
	  case pst_class:
	  case pst_coverage:
	    getint(sfd,&fpst->rules[i].lookup_cnt);
	    fpst->rules[i].lookups = galloc(fpst->rules[i].lookup_cnt*sizeof(struct seqlookup));
	    for ( j=0; j<fpst->rules[i].lookup_cnt; ++j ) {
		getname(sfd,tok);
		getint(sfd,&fpst->rules[i].lookups[j].seq);
		while ( (ch=getc(sfd))==' ' || ch=='\t' );
		if ( ch=='\'' ) {
		    fpst->rules[i].lookups[j].lookup_tag = getc(sfd)<<24;
		    fpst->rules[i].lookups[j].lookup_tag |= getc(sfd)<<16;
		    fpst->rules[i].lookups[j].lookup_tag |= getc(sfd)<<8;
		    fpst->rules[i].lookups[j].lookup_tag |= getc(sfd);
		    getc(sfd);	/* Final quote */
		} else
		    ungetc(ch,sfd);
	    }
	  break;
	  case pst_reversecoverage:
	    getname(sfd,tok);
	    getint(sfd,&temp);
	    fpst->rules[i].u.rcoverage.replacements = galloc(temp+1);
	    fpst->rules[i].u.rcoverage.replacements[temp] = '\0';
	    getc(sfd);	/* skip space */
	    fread(fpst->rules[i].u.rcoverage.replacements,1,temp,sfd);
	  break;
	}
    }
    getname(sfd,tok);
}

static uint32 ASM_ParseSubTag(FILE *sfd) {
    uint32 tag;
    int ch;

    while ( (ch=getc(sfd))==' ' );
    if ( ch!='\'' )
return( 0 );

    tag = getc(sfd)<<24;
    tag |= getc(sfd)<<16;
    tag |= getc(sfd)<<8;
    tag |= getc(sfd);
    getc(sfd);		/* final quote */
return( tag );
}

static void SFDParseStateMachine(FILE *sfd,SplineFont *sf,ASM *sm, char *tok) {
    int i, temp;

    sm->type = strmatch(tok,"MacIndic:")==0 ? asm_indic :
		strmatch(tok,"MacContext:")==0 ? asm_context :
		strmatch(tok,"MacLigature:")==0 ? asm_lig :
		strmatch(tok,"MacSimple:")==0 ? asm_simple :
		strmatch(tok,"MacKern:")==0 ? asm_kern : asm_insert;
    getusint(sfd,&sm->feature);
    getc(sfd);		/* Skip comma */
    getusint(sfd,&sm->setting);
    getusint(sfd,&sm->flags);
    getusint(sfd,&sm->class_cnt);
    getusint(sfd,&sm->state_cnt);

    sm->classes = galloc(sm->class_cnt*sizeof(char *));
    sm->classes[0] = sm->classes[1] = sm->classes[2] = sm->classes[3] = NULL;
    for ( i=4; i<sm->class_cnt; ++i ) {
	getname(sfd,tok);
	getint(sfd,&temp);
	sm->classes[i] = galloc(temp+1); sm->classes[i][temp] = '\0';
	getc(sfd);	/* skip space */
	fread(sm->classes[i],1,temp,sfd);
    }

    sm->state = galloc(sm->class_cnt*sm->state_cnt*sizeof(struct asm_state));
    for ( i=0; i<sm->class_cnt*sm->state_cnt; ++i ) {
	getusint(sfd,&sm->state[i].next_state);
	getusint(sfd,&sm->state[i].flags);
	if ( sm->type == asm_context ) {
	    sm->state[i].u.context.mark_tag = ASM_ParseSubTag(sfd);
	    sm->state[i].u.context.cur_tag = ASM_ParseSubTag(sfd);
	} else if ( sm->type == asm_insert ) {
	    getint(sfd,&temp);
	    if ( temp==0 )
		sm->state[i].u.insert.mark_ins = NULL;
	    else {
		sm->state[i].u.insert.mark_ins = galloc(temp+1); sm->state[i].u.insert.mark_ins[temp] = '\0';
		getc(sfd);	/* skip space */
		fread(sm->state[i].u.insert.mark_ins,1,temp,sfd);
	    }
	    getint(sfd,&temp);
	    if ( temp==0 )
		sm->state[i].u.insert.cur_ins = NULL;
	    else {
		sm->state[i].u.insert.cur_ins = galloc(temp+1); sm->state[i].u.insert.cur_ins[temp] = '\0';
		getc(sfd);	/* skip space */
		fread(sm->state[i].u.insert.cur_ins,1,temp,sfd);
	    }
	} else if ( sm->type == asm_kern ) {
	    int j;
	    getint(sfd,&sm->state[i].u.kern.kcnt);
	    if ( sm->state[i].u.kern.kcnt!=0 )
		sm->state[i].u.kern.kerns = galloc(sm->state[i].u.kern.kcnt*sizeof(int16));
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
	cur->name = pt = galloc(len+1);
	
	while ( (ch=getc(sfd))==' ');
	if ( ch=='"' )
	    ch = getc(sfd);
	while ( ch!='"' && ch!=EOF && pt<cur->name+len ) {
	    if ( ch=='\\' ) {
		*pt  = (getc(sfd)-'0')<<6;
		*pt |= (getc(sfd)-'0')<<3;
		*pt |= (getc(sfd)-'0');
	    } else
		*pt++ = ch;
	    ch = getc(sfd);
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
    char buffer[400], *sofar=gcalloc(1,1);
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
	sofar = grealloc(sofar,len+blen+1);
	strcpy(sofar+len,buffer);
	len += blen;
    }
    if ( len>0 && sofar[len-1]=='\n' )
	sofar[len-1] = '\0';
return( sofar );
}

static void SFDCleanupAnchorClasses(SplineFont *sf) {
    AnchorClass *ac;
    AnchorPoint *ap;
    int i, j, scnt;
#define S_MAX	100
    uint32 scripts[S_MAX];
    int merge=0;

    for ( ac = sf->anchor; ac!=NULL; ac=ac->next ) {
	if ( ac->script_lang_index==0xffff ) {
	    scnt = 0;
	    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
		for ( ap = sf->glyphs[i]->anchor; ap!=NULL && ap->anchor!=ac; ap=ap->next );
		if ( ap!=NULL && scnt<S_MAX ) {
		    uint32 script = SCScriptFromUnicode(sf->glyphs[i]);
		    if ( script==0 )
	    continue;
		    for ( j=0; j<scnt; ++j )
			if ( scripts[j]==script )
		    break;
		    if ( j==scnt )
			scripts[scnt++] = script;
		}
	    }
	    ac->script_lang_index = SFAddScriptIndex(sf,scripts,scnt);
	}
	if ( ac->merge_with == 0xffff )
	    ac->merge_with = ++merge;
    }
#undef S_MAX
}

enum uni_interp interp_from_encoding(Encoding *enc,enum uni_interp interp) {

    if ( enc==NULL )
return( interp );

    if ( enc->is_japanese )
	interp = ui_japanese;
    else if ( enc->is_korean )
	interp = ui_korean;
    else if ( enc->is_tradchinese )
	interp = ui_trad_chinese;
    else if ( enc->is_simplechinese )
	interp = ui_simp_chinese;
return( interp );
}

static void SFDCleanupFont(SplineFont *sf) {
    SFDCleanupAnchorClasses(sf);
    AltUniFigure(sf,sf->map);
    if ( sf->uni_interp==ui_unset )
	sf->uni_interp = interp_from_encoding(sf->map->enc,ui_none);

    /* Fixup for an old bug */
    if ( sf->pfminfo.os2_winascent < sf->ascent/4 && !sf->pfminfo.winascent_add ) {
	sf->pfminfo.winascent_add = true;
	sf->pfminfo.os2_winascent = 0;
	sf->pfminfo.windescent_add = true;
	sf->pfminfo.os2_windescent = 0;
    }
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
	map->backmap = grealloc(map->backmap,glyphcnt*sizeof(int));
	memset(map->backmap+map->backmax,-1,(glyphcnt-map->backmax)*sizeof(int));
	map->backmax = glyphcnt;
    }
    if ( enccnt>map->encmax ) {
	map->map = grealloc(map->map,enccnt*sizeof(int));
	memset(map->map+map->backmax,-1,(enccnt-map->encmax)*sizeof(int));
	map->encmax = map->enccount = enccnt;
    }
}

static SplineFont *SFD_GetFont(FILE *sfd,SplineFont *cidmaster,char *tok) {
    SplineFont *sf;
    SplineChar *sc;
    int realcnt, i, eof, mappos=-1, ch, ch2;
    struct table_ordering *lastord = NULL;
    struct ttf_table *lastttf[2];
    KernClass *lastkc=NULL, *kc, *lastvkc=NULL;
    FPST *lastfp=NULL;
    ASM *lastsm=NULL;
    struct axismap *lastaxismap = NULL;
    struct named_instance *lastnamedinstance = NULL;
    int pushedbacktok = false;
    Encoding *enc = &custom;
    struct remap *remap = NULL;

    orig_pos = 0;		/* Only used for compatibility with extremely old sfd files */

    lastttf[0] = lastttf[1] = NULL;

    sf = SplineFontEmpty();
    sf->cidmaster = cidmaster;
    sf->uni_interp = ui_unset;
    while ( 1 ) {
	if ( pushedbacktok )
	    pushedbacktok = false;
	else if ( (eof = getname(sfd,tok))!=1 ) {
	    if ( eof==-1 )
    break;
	    geteol(sfd,tok);
    continue;
	}
	if ( strmatch(tok,"FontName:")==0 ) {
	    getname(sfd,tok);
	    sf->fontname = copy(tok);
	} else if ( strmatch(tok,"FullName:")==0 ) {
	    geteol(sfd,tok);
	    sf->fullname = copy(tok);
	} else if ( strmatch(tok,"FamilyName:")==0 ) {
	    geteol(sfd,tok);
	    sf->familyname = copy(tok);
	} else if ( strmatch(tok,"Weight:")==0 ) {
	    getprotectedname(sfd,tok);
	    sf->weight = copy(tok);
	} else if ( strmatch(tok,"Copyright:")==0 ) {
	    sf->copyright = getquotedeol(sfd);
	} else if ( strmatch(tok,"Comments:")==0 ) {
	    sf->comments = getquotedeol(sfd);
	} else if ( strmatch(tok,"Version:")==0 ) {
	    geteol(sfd,tok);
	    sf->version = copy(tok);
	} else if ( strmatch(tok,"FONDName:")==0 ) {
	    geteol(sfd,tok);
	    sf->fondname = copy(tok);
	} else if ( strmatch(tok,"ItalicAngle:")==0 ) {
	    getreal(sfd,&sf->italicangle);
	} else if ( strmatch(tok,"StrokeWidth:")==0 ) {
	    getreal(sfd,&sf->strokewidth);
	} else if ( strmatch(tok,"UnderlinePosition:")==0 ) {
	    getreal(sfd,&sf->upos);
	} else if ( strmatch(tok,"UnderlineWidth:")==0 ) {
	    getreal(sfd,&sf->uwidth);
	} else if ( strmatch(tok,"PfmFamily:")==0 ) {
	    int temp;
	    getint(sfd,&temp);
	    sf->pfminfo.pfmfamily = temp;
	    sf->pfminfo.pfmset = true;
	} else if ( strmatch(tok,"LangName:")==0 ) {
	    sf->names = SFDGetLangName(sfd,sf->names);
	} else if ( strmatch(tok,"DesignSize:")==0 ) {
	    SFDGetDesignSize(sfd,sf);
	} else if ( strmatch(tok,"PfmWeight:")==0 || strmatch(tok,"TTFWeight:")==0 ) {
	    getsint(sfd,&sf->pfminfo.weight);
	    sf->pfminfo.pfmset = true;
	} else if ( strmatch(tok,"TTFWidth:")==0 ) {
	    getsint(sfd,&sf->pfminfo.width);
	    sf->pfminfo.pfmset = true;
	} else if ( strmatch(tok,"Panose:")==0 ) {
	    int temp,i;
	    for ( i=0; i<10; ++i ) {
		getint(sfd,&temp);
		sf->pfminfo.panose[i] = temp;
	    }
	    sf->pfminfo.panose_set = true;
	} else if ( strmatch(tok,"LineGap:")==0 ) {
	    getsint(sfd,&sf->pfminfo.linegap);
	    sf->pfminfo.pfmset = true;
	} else if ( strmatch(tok,"VLineGap:")==0 ) {
	    getsint(sfd,&sf->pfminfo.vlinegap);
	    sf->pfminfo.pfmset = true;
	} else if ( strmatch(tok,"HheadAscent:")==0 ) {
	    getsint(sfd,&sf->pfminfo.hhead_ascent);
	} else if ( strmatch(tok,"HheadAOffset:")==0 ) {
	    int temp;
	    getint(sfd,&temp); sf->pfminfo.hheadascent_add = temp;
	} else if ( strmatch(tok,"HheadDescent:")==0 ) {
	    getsint(sfd,&sf->pfminfo.hhead_descent);
	} else if ( strmatch(tok,"HheadDOffset:")==0 ) {
	    int temp;
	    getint(sfd,&temp); sf->pfminfo.hheaddescent_add = temp;
	} else if ( strmatch(tok,"OS2TypoLinegap:")==0 ) {
	    getsint(sfd,&sf->pfminfo.os2_typolinegap);
	} else if ( strmatch(tok,"OS2TypoAscent:")==0 ) {
	    getsint(sfd,&sf->pfminfo.os2_typoascent);
	} else if ( strmatch(tok,"OS2TypoAOffset:")==0 ) {
	    int temp;
	    getint(sfd,&temp); sf->pfminfo.typoascent_add = temp;
	} else if ( strmatch(tok,"OS2TypoDescent:")==0 ) {
	    getsint(sfd,&sf->pfminfo.os2_typodescent);
	} else if ( strmatch(tok,"OS2TypoDOffset:")==0 ) {
	    int temp;
	    getint(sfd,&temp); sf->pfminfo.typodescent_add = temp;
	} else if ( strmatch(tok,"OS2WinAscent:")==0 ) {
	    getsint(sfd,&sf->pfminfo.os2_winascent);
	} else if ( strmatch(tok,"OS2WinDescent:")==0 ) {
	    getsint(sfd,&sf->pfminfo.os2_windescent);
	} else if ( strmatch(tok,"OS2WinAOffset:")==0 ) {
	    int temp;
	    getint(sfd,&temp); sf->pfminfo.winascent_add = temp;
	} else if ( strmatch(tok,"OS2WinDOffset:")==0 ) {
	    int temp;
	    getint(sfd,&temp); sf->pfminfo.windescent_add = temp;
	} else if ( strmatch(tok,"HHeadAscent:")==0 ) {
	    getsint(sfd,&sf->pfminfo.hhead_ascent);
	} else if ( strmatch(tok,"HHeadDescent:")==0 ) {
	    getsint(sfd,&sf->pfminfo.hhead_descent);
	} else if ( strmatch(tok,"HHeadAOffset:")==0 ) {
	    int temp;
	    getint(sfd,&temp); sf->pfminfo.hheadascent_add = temp;
	} else if ( strmatch(tok,"HHeadDOffset:")==0 ) {
	    int temp;
	    getint(sfd,&temp); sf->pfminfo.hheaddescent_add = temp;
	} else if ( strmatch(tok,"MacStyle:")==0 ) {
	    getsint(sfd,&sf->macstyle);
	} else if ( strmatch(tok,"OS2SubXSize:")==0 ) {
	    getsint(sfd,&sf->pfminfo.os2_subxsize);
	    sf->pfminfo.subsuper_set = true;
	} else if ( strmatch(tok,"OS2SubYSize:")==0 ) {
	    getsint(sfd,&sf->pfminfo.os2_subysize);
	} else if ( strmatch(tok,"OS2SubXOff:")==0 ) {
	    getsint(sfd,&sf->pfminfo.os2_subxoff);
	} else if ( strmatch(tok,"OS2SubYOff:")==0 ) {
	    getsint(sfd,&sf->pfminfo.os2_subyoff);
	} else if ( strmatch(tok,"OS2SupXSize:")==0 ) {
	    getsint(sfd,&sf->pfminfo.os2_supxsize);
	} else if ( strmatch(tok,"OS2SupYSize:")==0 ) {
	    getsint(sfd,&sf->pfminfo.os2_supysize);
	} else if ( strmatch(tok,"OS2SupXOff:")==0 ) {
	    getsint(sfd,&sf->pfminfo.os2_supxoff);
	} else if ( strmatch(tok,"OS2SupYOff:")==0 ) {
	    getsint(sfd,&sf->pfminfo.os2_supyoff);
	} else if ( strmatch(tok,"OS2StrikeYSize:")==0 ) {
	    getsint(sfd,&sf->pfminfo.os2_strikeysize);
	} else if ( strmatch(tok,"OS2StrikeYPos:")==0 ) {
	    getsint(sfd,&sf->pfminfo.os2_strikeypos);
	} else if ( strmatch(tok,"OS2FamilyClass:")==0 ) {
	    getsint(sfd,&sf->pfminfo.os2_family_class);
	} else if ( strmatch(tok,"OS2Vendor:")==0 ) {
	    while ( isspace(getc(sfd)));
	    sf->pfminfo.os2_vendor[0] = getc(sfd);
	    sf->pfminfo.os2_vendor[1] = getc(sfd);
	    sf->pfminfo.os2_vendor[2] = getc(sfd);
	    sf->pfminfo.os2_vendor[3] = getc(sfd);
	    (void) getc(sfd);
	} else if ( strmatch(tok,"DisplaySize:")==0 ) {
	    getint(sfd,&sf->display_size);
	} else if ( strmatch(tok,"TopEncoding:")==0 ) {	/* Obsolete */
	    getint(sfd,&sf->top_enc);
	} else if ( strmatch(tok,"WinInfo:")==0 ) {
	    int temp1, temp2;
	    getint(sfd,&sf->top_enc);
	    getint(sfd,&temp1);
	    getint(sfd,&temp2);
	    if ( sf->top_enc<=0 ) sf->top_enc=-1;
	    if ( temp1<=0 ) temp1 = 16;
	    if ( temp2<=0 ) temp2 = 4;
	    sf->desired_col_cnt = temp1;
	    sf->desired_row_cnt = temp2;
	} else if ( strmatch(tok,"AntiAlias:")==0 ) {
	    int temp;
	    getint(sfd,&temp);
	    sf->display_antialias = temp;
	} else if ( strmatch(tok,"FitToEm:")==0 ) {
	    int temp;
	    getint(sfd,&temp);
	    sf->display_bbsized = temp;
	} else if ( strmatch(tok,"OnlyBitmaps:")==0 ) {
	    int temp;
	    getint(sfd,&temp);
	    sf->onlybitmaps = temp;
	} else if ( strmatch(tok,"Ascent:")==0 ) {
	    getint(sfd,&sf->ascent);
	} else if ( strmatch(tok,"Descent:")==0 ) {
	    getint(sfd,&sf->descent);
	} else if ( strmatch(tok,"Order2:")==0 ) {
	    int temp;
	    getint(sfd,&temp);
	    sf->order2 = temp;
	} else if ( strmatch(tok,"StrokedFont:")==0 ) {
	    int temp;
	    getint(sfd,&temp);
	    sf->strokedfont = temp;
	} else if ( strmatch(tok,"MultiLayer:")==0 ) {
	    int temp;
	    getint(sfd,&temp);
#ifdef FONTFORGE_CONFIG_TYPE3
	    sf->multilayer = temp;
#else
	    LogError( _("Warning: This version of FontForge does not contain extended type3/svg support\n needed for this font.\nReconfigure with --with-type3.\n") );
#endif
	} else if ( strmatch(tok,"NeedsXUIDChange:")==0 ) {
	    int temp;
	    getint(sfd,&temp);
	    sf->changed_since_xuidchanged = temp;
	} else if ( strmatch(tok,"VerticalOrigin:")==0 ) {
	    getint(sfd,&sf->vertical_origin);
	    sf->hasvmetrics = true;
	} else if ( strmatch(tok,"FSType:")==0 ) {
	    getsint(sfd,&sf->pfminfo.fstype);
	} else if ( strmatch(tok,"UniqueID:")==0 ) {
	    getint(sfd,&sf->uniqueid);
	} else if ( strmatch(tok,"XUID:")==0 ) {
	    geteol(sfd,tok);
	    sf->xuid = copy(tok);
	} else if ( strmatch(tok,"Encoding:")==0 ) {
	    enc = SFDGetEncoding(sfd,tok,sf);
	    if ( sf->map!=NULL ) sf->map->enc = enc;
	} else if ( strmatch(tok,"OldEncoding:")==0 ) {
	    /* old_encname =*/ (void) SFDGetEncoding(sfd,tok,sf);
	} else if ( strmatch(tok,"UnicodeInterp:")==0 ) {
	    sf->uni_interp = SFDGetUniInterp(sfd,tok,sf);
	} else if ( strmatch(tok,"NameList:")==0 ) {
	    SFDGetNameList(sfd,tok,sf);
	} else if ( strmatch(tok,"Compacted:")==0 ) {
	    int temp;
	    getint(sfd,&temp);
	    sf->compacted = temp;
	} else if ( strmatch(tok,"Registry:")==0 ) {
	    geteol(sfd,tok);
	    sf->cidregistry = copy(tok);
	} else if ( strmatch(tok,"Ordering:")==0 ) {
	    geteol(sfd,tok);
	    sf->ordering = copy(tok);
	} else if ( strmatch(tok,"Supplement:")==0 ) {
	    getint(sfd,&sf->supplement);
	} else if ( strmatch(tok,"RemapN:")==0 ) {
	    int n;
	    getint(sfd,&n);
	    remap = gcalloc(n+1,sizeof(struct remap));
	    remap[n].infont = -1;
	    mappos = 0;
	    if ( sf->map!=NULL ) sf->map->remap = remap;
	} else if ( strmatch(tok,"Remap:")==0 ) {
	    int f, l, p;
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
	    sf->grid.splines = SFDGetSplineSet(sf,sfd);
	} else if ( strmatch(tok,"ScriptLang:")==0 ) {
	    int i,j,k;
	    int imax, jmax, kmax;
	    getint(sfd,&imax);
	    sf->sli_cnt = imax;
	    sf->script_lang = galloc((imax+1)*sizeof(struct script_record *));
	    sf->script_lang[imax] = NULL;
	    for ( i=0; i<imax; ++i ) {
		getint(sfd,&jmax);
		sf->script_lang[i] = galloc((jmax+1)*sizeof(struct script_record));
		sf->script_lang[i][jmax].script = 0;
		for ( j=0; j<jmax; ++j ) {
		    while ( (ch=getc(sfd))==' ' || ch=='\t' );
		    sf->script_lang[i][j].script = ch<<24;
		    sf->script_lang[i][j].script |= getc(sfd)<<16;
		    sf->script_lang[i][j].script |= getc(sfd)<<8;
		    sf->script_lang[i][j].script |= getc(sfd);
		    getint(sfd,&kmax);
		    sf->script_lang[i][j].langs = galloc((kmax+1)*sizeof(uint32));
		    sf->script_lang[i][j].langs[kmax] = 0;
		    for ( k=0; k<kmax; ++k ) {
			while ( (ch=getc(sfd))==' ' || ch=='\t' );
			sf->script_lang[i][j].langs[k] = ch<<24;
			sf->script_lang[i][j].langs[k] |= getc(sfd)<<16;
			sf->script_lang[i][j].langs[k] |= getc(sfd)<<8;
			sf->script_lang[i][j].langs[k] |= getc(sfd);
		    }
		}
	    }
	} else if ( strmatch(tok,"MarkAttachClasses:")==0 ) {
	    getint(sfd,&sf->mark_class_cnt);
	    sf->mark_classes = galloc(sf->mark_class_cnt*sizeof(char *));
	    sf->mark_class_names = galloc(sf->mark_class_cnt*sizeof(char *));
	    sf->mark_classes[0] = NULL; sf->mark_class_names[0] = NULL;
	    for ( i=1; i<sf->mark_class_cnt; ++i ) {	/* Class 0 is unused */
		int temp;
		while ( (temp=getc(sfd))=='\n' || temp=='\r' ); ungetc(temp,sfd);
		sf->mark_class_names[i] = SFDReadUTF7Str(sfd);
		getint(sfd,&temp);
		sf->mark_classes[i] = galloc(temp+1); sf->mark_classes[i][temp] = '\0';
		getc(sfd);	/* skip space */
		fread(sf->mark_classes[i],1,temp,sfd);
	    }
	} else if ( strmatch(tok,"KernClass:")==0 || strmatch(tok,"VKernClass:")==0 ) {
	    int temp, classstart=1;
	    int isv = tok[0]=='V';
	    kc = chunkalloc(sizeof(KernClass));
	    if ( !isv ) {
		if ( lastkc==NULL )
		    sf->kerns = kc;
		else
		    lastkc->next = kc;
		lastkc = kc;
	    } else {
		if ( lastvkc==NULL )
		    sf->vkerns = kc;
		else
		    lastvkc->next = kc;
		lastvkc = kc;
	    }
	    getint(sfd,&kc->first_cnt);
	    ch=getc(sfd);
	    if ( ch=='+' )
		classstart = 0;
	    else
		ungetc(ch,sfd);
	    getint(sfd,&kc->second_cnt);
	    getint(sfd,&temp); kc->sli = temp;
	    getint(sfd,&temp); kc->flags = temp;
	    kc->firsts = galloc(kc->first_cnt*sizeof(char *));
	    kc->seconds = galloc(kc->second_cnt*sizeof(char *));
	    kc->offsets = galloc(kc->first_cnt*kc->second_cnt*sizeof(int16));
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    kc->adjusts = gcalloc(kc->first_cnt*kc->second_cnt,sizeof(DeviceTable));
#endif
	    kc->firsts[0] = NULL;
	    for ( i=classstart; i<kc->first_cnt; ++i ) {
		getint(sfd,&temp);
		kc->firsts[i] = galloc(temp+1); kc->firsts[i][temp] = '\0';
		getc(sfd);	/* skip space */
		fread(kc->firsts[i],1,temp,sfd);
	    }
	    kc->seconds[0] = NULL;
	    for ( i=1; i<kc->second_cnt; ++i ) {
		getint(sfd,&temp);
		kc->seconds[i] = galloc(temp+1); kc->seconds[i][temp] = '\0';
		getc(sfd);	/* skip space */
		fread(kc->seconds[i],1,temp,sfd);
	    }
	    for ( i=0; i<kc->first_cnt*kc->second_cnt; ++i ) {
		getint(sfd,&temp);
		kc->offsets[i] = temp;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		SFDReadDeviceTable(sfd,&kc->adjusts[i]);
#else
		SFDSkipDeviceTable(sfd);
#endif
	    }
	} else if ( strmatch(tok,"ContextPos:")==0 || strmatch(tok,"ContextSub:")==0 ||
		strmatch(tok,"ChainPos:")==0 || strmatch(tok,"ChainSub:")==0 ||
		strmatch(tok,"ReverseChain:")==0 ) {
	    FPST *fpst = chunkalloc(sizeof(FPST));
	    if ( lastfp==NULL )
		sf->possub = fpst;
	    else
		lastfp->next = fpst;
	    lastfp = fpst;
	    SFDParseChainContext(sfd,sf,fpst,tok);
	} else if ( strmatch(tok,"MacIndic:")==0 || strmatch(tok,"MacContext:")==0 ||
		strmatch(tok,"MacLigature:")==0 || strmatch(tok,"MacSimple:")==0 ||
		strmatch(tok,"MacKern:")==0 || strmatch(tok,"MacInsert:")==0 ) {
	    ASM *sm = chunkalloc(sizeof(ASM));
	    if ( lastsm==NULL )
		sf->sm = sm;
	    else
		lastsm->next = sm;
	    lastsm = sm;
	    SFDParseStateMachine(sfd,sf,sm,tok);
	} else if ( strmatch(tok,"MacFeat:")==0 ) {
	    sf->features = SFDParseMacFeatures(sfd,tok);
	} else if ( strmatch(tok,"TeXData:")==0 ) {
	    int temp;
	    getint(sfd,&temp);
	    sf->texdata.type = temp;
	    getsint(sfd,(int16 *) &sf->design_size);
	    sf->design_size = (5*sf->design_size+(1<<18))>>19;
	    for ( i=0; i<22; ++i ) {
		int foo;
		getint(sfd,&foo);
		sf->texdata.params[i]=foo;
	    }
	} else if ( strmatch(tok,"AnchorClass:")==0 ) {
	    char *name;
	    AnchorClass *lastan = NULL, *an;
	    while ( (name=SFDReadUTF7Str(sfd))!=NULL ) {
		an = chunkalloc(sizeof(AnchorClass));
		an->name = name;
		getname(sfd,tok);
		if ( tok[0]=='0' && tok[1]=='\0' )
		    an->feature_tag = 0;
		else {
		    if ( tok[1]=='\0' ) { tok[1]=' '; tok[2] = 0; }
		    if ( tok[2]=='\0' ) { tok[2]=' '; tok[3] = 0; }
		    if ( tok[3]=='\0' ) { tok[3]=' '; tok[4] = 0; }
		    an->feature_tag = (tok[0]<<24) | (tok[1]<<16) | (tok[2]<<8) | tok[3];
		}
		while ( (ch=getc(sfd))==' ' || ch=='\t' );
		ungetc(ch,sfd);
		if ( isdigit(ch)) {
		    int temp;
		    getint(sfd,&temp);
		    an->flags = temp;
		}
#if 0
		else if ( an->feature_tag==CHR('c','u','r','s'))
		    an->flags = pst_ignorecombiningmarks;
#endif
		while ( (ch=getc(sfd))==' ' || ch=='\t' );
		ungetc(ch,sfd);
		if ( isdigit(ch)) {
		    int temp;
		    getint(sfd,&temp);
		    an->script_lang_index = temp;
		} else
		    an->script_lang_index = 0xffff;		/* Will be fixed up later */
		while ( (ch=getc(sfd))==' ' || ch=='\t' );
		ungetc(ch,sfd);
		if ( isdigit(ch)) {
		    int temp;
		    getint(sfd,&temp);
		    an->merge_with = temp;
		} else
		    an->merge_with = 0xffff;			/* Will be fixed up later */
		while ( (ch=getc(sfd))==' ' || ch=='\t' );
		ungetc(ch,sfd);
		if ( isdigit(ch)) {
		    int temp;
		    getint(sfd,&temp);
		    an->type = temp;
		} else {
		    if ( an->feature_tag==CHR('c','u','r','s'))
			an->type = act_curs;
		    else if ( an->feature_tag==CHR('m','k','m','k'))
			an->type = act_mkmk;
		    else
			an->type = act_mark;
		}
		if ( lastan==NULL )
		    sf->anchor = an;
		else
		    lastan->next = an;
		lastan = an;
	    }
	} else if ( strmatch(tok,"GenTags:")==0 ) {
	    int temp; uint32 tag;
	    getint(sfd,&temp);
	    sf->gentags.tt_cur = temp;
	    sf->gentags.tt_max = sf->gentags.tt_cur+10;
	    sf->gentags.tagtype = galloc(sf->gentags.tt_max*sizeof(struct tagtype));
	    ch = getc(sfd);
	    i = 0;
	    while ( ch!='\n' && ch!='\r' ) {
		while ( ch==' ' ) ch = getc(sfd);
		if ( ch=='\n' || ch==EOF )
	    break;
		ch2 = getc(sfd);
		if ( ch=='p' && ch2=='s' ) sf->gentags.tagtype[i].type = pst_position;
		else if ( ch=='p' && ch2=='r' ) sf->gentags.tagtype[i].type = pst_pair;
		else if ( ch=='s' && ch2=='b' ) sf->gentags.tagtype[i].type = pst_substitution;
		else if ( ch=='a' && ch2=='s' ) sf->gentags.tagtype[i].type = pst_alternate;
		else if ( ch=='m' && ch2=='s' ) sf->gentags.tagtype[i].type = pst_multiple;
		else if ( ch=='l' && ch2=='g' ) sf->gentags.tagtype[i].type = pst_ligature;
		else if ( ch=='a' && ch2=='n' ) sf->gentags.tagtype[i].type = pst_anchors;
		else if ( ch=='c' && ch2=='p' ) sf->gentags.tagtype[i].type = pst_contextpos;
		else if ( ch=='c' && ch2=='s' ) sf->gentags.tagtype[i].type = pst_contextsub;
		else if ( ch=='p' && ch2=='c' ) sf->gentags.tagtype[i].type = pst_chainpos;
		else if ( ch=='s' && ch2=='c' ) sf->gentags.tagtype[i].type = pst_chainsub;
		else if ( ch=='r' && ch2=='s' ) sf->gentags.tagtype[i].type = pst_reversesub;
		else if ( ch=='n' && ch2=='l' ) sf->gentags.tagtype[i].type = pst_null;
		else ch2 = EOF;
		(void) getc(sfd);
		tag = getc(sfd)<<24;
		tag |= getc(sfd)<<16;
		tag |= getc(sfd)<<8;
		tag |= getc(sfd);
		(void) getc(sfd);
		if ( ch2!=EOF )
		    sf->gentags.tagtype[i++].tag = tag;
		ch = getc(sfd);
	    }
	} else if ( strmatch(tok,"TtfTable:")==0 ) {
	    SFDGetTtfTable(sfd,sf,lastttf);
	} else if ( strmatch(tok,"TableOrder:")==0 ) {
	    int temp;
	    struct table_ordering *ord;
	    while ((ch=getc(sfd))==' ' );
	    ord = chunkalloc(sizeof(struct table_ordering));
	    ord->table_tag = (ch<<24) | (getc(sfd)<<16);
	    ord->table_tag |= getc(sfd)<<8;
	    ord->table_tag |= getc(sfd);
	    getint(sfd,&temp);
	    ord->ordered_features = galloc((temp+1)*sizeof(uint32));
	    ord->ordered_features[temp] = 0;
	    for ( i=0; i<temp; ++i ) {
		while ( isspace((ch=getc(sfd))) );
		if ( ch=='\'' ) {
		    ch = getc(sfd);
		    ord->ordered_features[i] = (ch<<24) | (getc(sfd)<<16);
		    ord->ordered_features[i] |= (getc(sfd)<<8);
		    ord->ordered_features[i] |= getc(sfd);
		    if ( (ch=getc(sfd))!='\'') ungetc(ch,sfd);
		} else if ( ch=='<' ) {
		    int f,s;
		    fscanf(sfd,"%d,%d>", &f, &s );
		    ord->ordered_features[i] = (f<<16)|s;
		}
	    }
	    if ( lastord==NULL )
		sf->orders = ord;
	    else
		lastord->next = ord;
	    lastord = ord;
	} else if ( strmatch(tok,"BeginPrivate:")==0 ) {
	    SFDGetPrivate(sfd,sf);
	} else if ( strmatch(tok,"BeginSubrs:")==0 ) {	/* leave in so we don't croak on old sfd files */
	    SFDGetSubrs(sfd,sf);
	} else if ( strmatch(tok,"MMCounts:")==0 ) {
	    MMSet *mm = sf->mm = chunkalloc(sizeof(MMSet));
	    getint(sfd,&mm->instance_count);
	    getint(sfd,&mm->axis_count);
	    ch = getc(sfd);
	    if ( ch!=' ' )
		ungetc(ch,sfd);
	    else { int temp;
		getint(sfd,&temp);
		mm->apple = temp;
		getint(sfd,&mm->named_instance_count);
	    }
	    mm->instances = gcalloc(mm->instance_count,sizeof(SplineFont *));
	    mm->positions = galloc(mm->instance_count*mm->axis_count*sizeof(real));
	    mm->defweights = galloc(mm->instance_count*sizeof(real));
	    mm->axismaps = gcalloc(mm->axis_count,sizeof(struct axismap));
	    if ( mm->named_instance_count!=0 )
		mm->named_instances = gcalloc(mm->named_instance_count,sizeof(struct named_instance));
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
		mm->axismaps[index].blends = galloc(points*sizeof(real));
		mm->axismaps[index].designs = galloc(points*sizeof(real));
		for ( i=0; i<points; ++i ) {
		    getreal(sfd,&mm->axismaps[index].blends[i]);
		    while ( (ch=getc(sfd))!=EOF && isspace(ch));
		    ungetc(ch,sfd);
		    if ( (ch=getc(sfd))!='=' )
			ungetc(ch,sfd);
		    else if ( (ch=getc(sfd))!='>' )
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
		mm->named_instances[index].coords = galloc(mm->axis_count*sizeof(real));
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
#if defined(FONTFORGE_CONFIG_GDRAW)
	    gwwv_progress_change_stages(cnt);
#elif defined(FONTFORGE_CONFIG_GTK)
	    gwwv_progress_change_stages(cnt);
#endif
#if defined(FONTFORGE_CONFIG_GDRAW)
	    gwwv_progress_change_total(realcnt);
#elif defined(FONTFORGE_CONFIG_GTK)
	    gwwv_progress_change_total(realcnt);
#endif
	    MMInferStuff(sf->mm);
    break;
	} else if ( strmatch(tok,"BeginSubFonts:")==0 ) {
	    getint(sfd,&sf->subfontcnt);
	    sf->subfonts = gcalloc(sf->subfontcnt,sizeof(SplineFont *));
	    getint(sfd,&realcnt);
	    sf->map = EncMap1to1(realcnt);
#if defined(FONTFORGE_CONFIG_GDRAW)
	    gwwv_progress_change_stages(2);
#elif defined(FONTFORGE_CONFIG_GTK)
	    gwwv_progress_change_stages(2);
#endif
#if defined(FONTFORGE_CONFIG_GDRAW)
	    gwwv_progress_change_total(realcnt);
#elif defined(FONTFORGE_CONFIG_GTK)
	    gwwv_progress_change_total(realcnt);
#endif
    break;
	} else if ( strmatch(tok,"BeginChars:")==0 ) {
	    int charcnt;
	    getint(sfd,&charcnt);
	    if ( getint(sfd,&realcnt)!=1 || realcnt==-1 )
		realcnt = charcnt;
	    else
		++realcnt;		/* value saved is max glyph, not glyph cnt */
#if defined(FONTFORGE_CONFIG_GDRAW)
	    gwwv_progress_change_total(realcnt);
#elif defined(FONTFORGE_CONFIG_GTK)
	    gwwv_progress_change_total(realcnt);
#endif
	    sf->glyphcnt = sf->glyphmax = realcnt;
	    sf->glyphs = gcalloc(realcnt,sizeof(SplineChar *));
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

    if ( sf->subfontcnt!=0 ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	gwwv_progress_change_stages(2*sf->subfontcnt);
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_progress_change_stages(2*sf->subfontcnt);
#endif
	for ( i=0; i<sf->subfontcnt; ++i ) {
	    if ( i!=0 )
#if defined(FONTFORGE_CONFIG_GDRAW)
		gwwv_progress_next_stage();
#elif defined(FONTFORGE_CONFIG_GTK)
		gwwv_progress_next_stage();
#endif
	    sf->subfonts[i] = SFD_GetFont(sfd,sf,tok);
	}
    } else if ( sf->mm!=NULL ) {
	MMSet *mm = sf->mm;
#if defined(FONTFORGE_CONFIG_GDRAW)
	gwwv_progress_change_stages(2*(mm->instance_count+1));
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_progress_change_stages(2*(mm->instance_count+1));
#endif
	for ( i=0; i<mm->instance_count; ++i ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	    if ( i!=0 )
		gwwv_progress_next_stage();
#elif defined(FONTFORGE_CONFIG_GTK)
	    if ( i!=0 )
		gwwv_progress_next_stage();
#endif
	    mm->instances[i] = SFD_GetFont(sfd,NULL,tok);
	    EncMapFree(mm->instances[i]->map); mm->instances[i]->map=NULL;
	    mm->instances[i]->mm = mm;
	}
#if defined(FONTFORGE_CONFIG_GDRAW)
	gwwv_progress_next_stage();
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_progress_next_stage();
#endif
	mm->normal = SFD_GetFont(sfd,NULL,tok);
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
	while ( (sc = SFDGetChar(sfd,sf))!=NULL ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	    gwwv_progress_next();
#elif defined(FONTFORGE_CONFIG_GTK)
	    gwwv_progress_next();
#endif
	}
#if defined(FONTFORGE_CONFIG_GDRAW)
	gwwv_progress_next_stage();
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_progress_next_stage();
#endif
#if defined(FONTFORGE_CONFIG_GDRAW)
	gwwv_progress_change_line2(_("Interpreting Glyphs"));
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_progress_change_line2(_("Interpreting Glyphs"));
#endif
	SFDFixupRefs(sf);
    }
    while ( getname(sfd,tok)==1 ) {
	if ( strcmp(tok,"EndSplineFont")==0 || strcmp(tok,"EndSubSplineFont")==0 )
    break;
	else if ( strcmp(tok,"BitmapFont:")==0 )
	    SFDGetBitmapFont(sfd,sf);
	else if ( strmatch(tok,"DupEnc:")==0 ) {
	    int enc, orig;
	    if ( getint(sfd,&enc) && getint(sfd,&orig) && sf->map!=NULL ) {
		SFDSetEncMap(sf,orig,enc);
	    }
	}
    }
    SFDCleanupFont(sf);
return( sf );
}

static int SFDStartsCorrectly(FILE *sfd,char *tok) {
    real dval;
    int ch;

    if ( getname(sfd,tok)!=1 )
return( false );
    if ( strcmp(tok,"SplineFontDB:")!=0 )
return( false );
    if ( getreal(sfd,&dval)!=1 || (dval!=0 && dval!=1))
return( false );
    ch = getc(sfd); ungetc(ch,sfd);
    if ( ch!='\r' && ch!='\n' )
return( false );

return( true );
}

SplineFont *SFDRead(char *filename) {
    FILE *sfd = fopen(filename,"r");
    SplineFont *sf=NULL;
    char *oldloc;
    char tok[2000];
    int version;

    if ( sfd==NULL )
return( NULL );
    oldloc = setlocale(LC_NUMERIC,"C");
    gwwv_progress_change_stages(2);
    if ( (version = SFDStartsCorrectly(sfd,tok)) )
	sf = SFD_GetFont(sfd,NULL,tok);
    setlocale(LC_NUMERIC,oldloc);
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

SplineChar *SFDReadOneChar(SplineFont *cur_sf,const char *name) {
    FILE *sfd = fopen(cur_sf->filename,"r");
    SplineChar *sc=NULL;
    char *oldloc;
    char tok[2000];
    uint32 pos;
    SplineFont sf;

    if ( sfd==NULL )
return( NULL );
    oldloc = setlocale(LC_NUMERIC,"C");

    memset(&sf,0,sizeof(sf));
    sf.ascent = 800; sf.descent = 200;
    if ( cur_sf->cidmaster ) cur_sf = cur_sf->cidmaster;
    sf.sli_cnt = cur_sf->sli_cnt;
    sf.script_lang = cur_sf->script_lang;
    if ( SFDStartsCorrectly(sfd,tok) ) {
	pos = ftell(sfd);
	while ( getname(sfd,tok)!=-1 ) {
	    if ( strcmp(tok,"StartChar:")==0 ) {
		if ( getname(sfd,tok)==1 && strcmp(tok,name)==0 ) {
		    fseek(sfd,pos,SEEK_SET);
		    sc = SFDGetChar(sfd,&sf);
	break;
		}
	    } else if ( strmatch(tok,"Order2:")==0 ) {
		int order2;
		getint(sfd,&order2);
		sf.order2 = order2;
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
	    }
	    pos = ftell(sfd);
	}
    }

    setlocale(LC_NUMERIC,oldloc);
    fclose(sfd);
return( sc );
}

static int ModSF(FILE *asfd,SplineFont *sf) {
    Encoding *newmap;
    int cnt, order2=0;
#ifdef FONTFORGE_CONFIG_TYPE3
    int multilayer=0;
#endif
    char tok[200];
    int i,k;
    SplineChar *sc;
    SplineFont *ssf;
    SplineFont temp;

    memset(&temp,0,sizeof(temp));
    temp.ascent = sf->ascent; temp.descent = sf->descent;
    temp.order2 = sf->order2;
    temp.multilayer = sf->multilayer;
    temp.sli_cnt = sf->sli_cnt;
    temp.script_lang = sf->script_lang;

    if ( getname(asfd,tok)!=1 || strcmp(tok,"Encoding:")!=0 )
return(false);
    newmap = SFDGetEncoding(asfd,tok,&temp);
    if ( getname(asfd,tok)!=1 )
return( false );
    if ( strcmp(tok,"UnicodeInterp:")==0 ) {
	sf->uni_interp = SFDGetUniInterp(asfd,tok,sf);
	if ( getname(asfd,tok)!=1 )
return( false );
    }
    if ( sf->map->enc!=newmap ) {
	EncMap *map = EncMapFromEncoding(sf,newmap);
	EncMapFree(sf->map);
	sf->map = map;
    }
    temp.map = sf->map;
    if ( strcmp(tok,"Order2:")==0 ) {
	getint(asfd,&order2);
	if ( getname(asfd,tok)!=1 )
return( false );
    }
    if ( order2!=sf->order2 ) {
	if ( order2 )
	    SFConvertToOrder2(sf);
	else
	    SFConvertToOrder3(sf);
    }
#ifdef FONTFORGE_CONFIG_TYPE3
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
#endif
    if ( strcmp(tok,"BeginChars:")!=0 )
return(false);
    SFRemoveDependencies(sf);

    getint(asfd,&cnt);
    if ( cnt>sf->glyphcnt ) {
	sf->glyphs = grealloc(sf->glyphs,cnt*sizeof(SplineChar *));
	for ( i=sf->glyphcnt; i<cnt; ++i )
	    sf->glyphs[i] = NULL;
	sf->glyphcnt = sf->glyphmax = cnt;
    }
    while ( (sc = SFDGetChar(asfd,&temp))!=NULL ) {
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
    sf->sli_cnt = temp.sli_cnt;
    sf->script_lang = temp.script_lang;
    sf->changed = true;
    SFDFixupRefs(sf);
return(true);
}

static SplineFont *SlurpRecovery(FILE *asfd,char *tok,int sizetok) {
    char *pt; int ch;
    SplineFont *sf;

    ch=getc(asfd);
    ungetc(ch,asfd);
    if ( ch=='B' ) {
	if ( getname(asfd,tok)!=1 )
return(NULL);
	if ( strcmp(tok,"Base:")!=0 )
return(NULL);
	while ( isspace(ch=getc(asfd)) && ch!=EOF && ch!='\n' );
	for ( pt=tok; ch!=EOF && ch!='\n'; ch = getc(asfd) )
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

SplineFont *SFRecoverFile(char *autosavename) {
    FILE *asfd = fopen( autosavename,"r");
    SplineFont *ret;
    char *oldloc;
    char tok[1025];

    if ( asfd==NULL )
return(NULL);
    oldloc = setlocale(LC_NUMERIC,"C");
    ret = SlurpRecovery(asfd,tok,sizeof(tok));
    if ( ret==NULL ) {
	char *buts[3];
	buts[0] = "_Forget It"; buts[1] = "_Try Again"; buts[2] = NULL;
	if ( gwwv_ask(_("Recovery Failed"),(const char **) buts,0,1,_("Automagic recovery of changes to %.80s failed.\nShould FontForge try again to recover next time you start it?"),tok)==0 )
	    unlink(autosavename);
    }
    setlocale(LC_NUMERIC,oldloc);
    fclose(asfd);
    if ( ret )
	ret->autosavename = copy(autosavename);
return( ret );
}

void SFAutoSave(SplineFont *sf,EncMap *map) {
    int i, k, max;
    FILE *asfd;
    char *oldloc;
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

    oldloc = setlocale(LC_NUMERIC,"C");
    if ( !sf->new && sf->origname!=NULL )	/* might be a new file */
	fprintf( asfd, "Base: %s\n", sf->origname );
    fprintf( asfd, "Encoding: %s\n", map->enc->enc_name );
    fprintf( asfd, "UnicodeInterp: %s\n", unicode_interp_names[sf->uni_interp]);
    if ( sf->order2 )
	fprintf( asfd, "Order2: %d\n", sf->order2 );
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
	    SFDDumpChar( asfd,ssf->glyphs[i],map,NULL);
    }
    fprintf( asfd, "EndChars\n" );
    fprintf( asfd, "EndSplineFont\n" );
    fclose(asfd);
    setlocale(LC_NUMERIC,oldloc);
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
    char *oldloc;
    char tok[2000];
    char **ret = NULL;
    int eof;

    if ( sfd==NULL )
return( NULL );
    oldloc = setlocale(LC_NUMERIC,"C");
    if ( SFDStartsCorrectly(sfd,tok) ) {
	while ( !feof(sfd)) {
	    if ( (eof = getname(sfd,tok))!=1 ) {
		if ( eof==-1 )
	break;
		geteol(sfd,tok);
	continue;
	    }
	    if ( strmatch(tok,"FontName:")==0 ) {
		getname(sfd,tok);
		ret = galloc(2*sizeof(char*));
		ret[0] = copy(tok);
		ret[1] = NULL;
	break;
	    }
	}
    }
    setlocale(LC_NUMERIC,oldloc);
    fclose(sfd);
return( ret );
}
