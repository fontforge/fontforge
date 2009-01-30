/* Copyright (C) 2000-2009 by George Williams */
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
#include "gimage.h"
/*#include "gxdrawP.h"*/
#include "string.h"
#include "utype.h"

/* X Pixmap format
These are c files, all the significant data resides in strings, everything
else may be ignored (well, the first line must be the comment / * XPM * /)

    / * XPM * /
    static char * plaid[] =
    {
    / * plaid pixmap * /
    / * width height ncolors chars_per_pixel * /
    "22 22 4 2 0 0 XPMEXT",
    / * colors * /
    "   c red       m white  s light_color",
    "Y  c green     m black  s ines_in_mix",
    "+  c yellow    m white  s lines_in_dark ",
    "x              m black  s dark_color ",
    / * pixels * /
    "x   x   x x x   x   x x x x x x + x x x x x ",
    "  x   x   x   x   x   x x x x x x x x x x x ",
    "x   x   x x x   x   x x x x x x + x x x x x ",

The first string has 4 interesting numbers in it: width, height, ncolors,
characters per pixel, these may be followed by the hot spot location (2 nums)
or the text XPMEXT which means there are extensions later.

The next few strings (there will be ncolors of them) provide a mapping between
character(s) and colors.  Above the "  " combination maps to red on a color
display, white on monochrome and is called light_color.

After that come lines of pixels.  Each line is width*chars_per_pixel long and
there are height of them.

color names may be: a name in the x database (ie sky blue), #xxxxxx (hex rgb),
%xxxxxx (hex hsb), None==transparent, "a symbolic name" and I don't know what
good they are.
*/
/* There seems to be another, similar format XPM2 which is the same except
    nothing is in a string.  Example:
! XPM2  
64 64 2 1
  c #FFFFFFFFFFFF
. c #000000000000
                     ..       .   .                             
 .                                    .  ..                     
  .                                                             
     .                 .                                        
        ..                                  .                   
                                                                
                                                                
                                                .               
                            .                                   
*/

static long LookupXColorName(char *name) {
#if 0 && !defined( X_DISPLAY_MISSING )
    XColor ret;
    Display *display;

    if ( screen_display==NULL )
return( COLOR_UNKNOWN );

    display = ((GXDisplay *) screen_display)->display;
    if ( XParseColor(display,DefaultColormap(display,DefaultScreen(display)),
	    name,&ret))
return( ((ret.red>>8)<<16) | (ret.green&0xff00) | (ret.blue>>8) );
#endif	/* NO X */

return( COLOR_UNKNOWN );
}

/* Don't handle backslash sequences, nor concatenated strings */
static int getstring(unsigned char *buf, int sz, FILE *fp) {
    int ch, incomment=0;

    while ((ch=getc(fp))!=EOF ) {
	if ( ch=='"' && !incomment )
    break;
	if ( !incomment && ch=='/' ) {
	    ch = getc(fp);
	    if ( ch=='*' ) incomment=true;
	    else ungetc(ch,fp);
	} else if ( incomment && ch=='*' ) {
	    ch = getc(fp);
	    if ( ch=='/' ) incomment = false;
	    else ungetc(ch,fp);
	}
    }
    if ( ch==EOF )
return( 0 );
    while ( (ch=getc(fp))!=EOF && ch!='"' ) {
	if ( --sz>0 )
	    *buf++ = ch;
    }
    *buf = '\0';
return(1 );
}

static int gww_getline(unsigned char *buf, int sz, FILE *fp) {
    int ch;
    unsigned char *pt=buf;

    while ((ch=getc(fp))!=EOF && ch!='\n' && ch!='\r')
	*pt++ = ch;
    if ( ch=='\r' ) {
	if ((ch = getc(fp))!='\n' )
	    ungetc(ch,fp);
    }
    *pt = '\0';
    if ( ch==EOF && pt==buf )
return( 0 );
return(1 );
}

#define TRANS 0x1000000
union hash {
    long color;
    union hash *table;
};

static void freetab(union hash *tab, int nchars) {
    int i;

    if ( nchars>1 ) {
	for ( i=0; i<256; ++i )
	    if ( tab[i].table!=NULL )
		freetab(tab[i].table,nchars-1);
    }
    gfree(tab);
}
	
static int fillupclut(Color *clut, union hash *tab,int index,int nchars) {
    int i;

    if ( nchars==1 ) {
	for ( i=0; i<256; ++i )
	    if ( tab[i].color!=-1 ) {
		if ( tab[i].color == TRANS ) {
		    clut[256] = index;
		    tab[i].color = 0;
		}
		clut[index] = tab[i].color;
		tab[i].color = index++;
	    }
    } else {
	for ( i=0; i<256; ++i )
	    if ( tab[i].table!=NULL )
		index = fillupclut(clut,tab[i].table,index,nchars-1);
    }
return( index );
}
	    
static long parsecol(char *start, char *end) {
    long ret = -1;
    int ch;
    extern long LookupXColorName(char *);

    while ( !isspace(*start) && *start!='\0' ) ++start;
    while ( isspace(*start)) ++start;
    while ( end>start && isspace(end[-1])) --end;
    ch = *end; *end = '\0';

    if ( strcmp(start,"None")==0 )
	ret = TRANS;
    else if ( *start=='#' || *start=='%') {
	if ( end-start==4 ) {
	    sscanf(start+1,"%lx",&ret);
	    ret = ((ret&0xf00)<<12) | ((ret&0xf0)<<8) | ((ret&0xf)<<4);
	} else if ( end-start==7 )
	    sscanf(start+1,"%lx",&ret);
	else if ( end-start==13 ) {
	    int r,g,b;
	    sscanf(start+1,"%4x%4x%4x",&r,&g,&b);
	    ret = ((r>>8)<<16) | ((g>>8)<<8) | (b>>8);
	}
	if ( *start=='%' )
	    /* How do I translate from HSB to RGB???? */;
    } else if (( ret=LookupXColorName(start))!=-1 ) {
    } else if ( strcmp(start,"white")==0 ) {
	ret = COLOR_CREATE(255,255,255);
    } else {
	ret = 0;
    }

    *end = ch;
return ret;
}

static char *findnextkey(char *str) {
    int oktostart=1;

    while ( *str ) {
	if ( isspace(*str)) oktostart=true;
	else if ( oktostart ) {
	    if (( *str=='c' && isspace(str[1])) ||
		    (*str=='m' && isspace(str[1])) ||
		    (*str=='g' && isspace(str[1])) ||
		    (*str=='g' && str[1]=='4' && isspace(str[2])) ||
		    (*str=='s' && isspace(str[1])) )
return( str );
	    oktostart = false;
	}
	++str;
    }
return( str );
}

static long findcol(char *str) {
    char *pt, *end;
    char *try = "cgm";		/* Try in this order to find something */

    while ( *try ) {
	pt = findnextkey(str);
	while ( *pt ) {
	    end = findnextkey(pt+2);
	    if ( *pt==*try )
return( parsecol(pt,end));
	    pt = end;
	}
	++try;
    }
	
return 0;
}

static union hash *parse_colors(FILE *fp,unsigned char *line, int lsiz, int ncols, int nchars,
	int (*getdata)(unsigned char *,int,FILE *)) {
    union hash *tab = (union hash *) galloc(256*sizeof(union hash));
    union hash *sub;
    int i, j;

    if ( nchars==1 )
	memset(tab,-1,256*sizeof(union hash));
    for ( i=0; i<ncols; ++i ) {
	if ( !getdata(line,lsiz,fp)) {
	    freetab(tab,nchars);
return( NULL );
	}
	sub = tab;
	for ( j=0; j<nchars-1; ++j ) {
	    if ( sub[line[j]].table==NULL ) {
		sub[line[j]].table = (union hash *) galloc(256*sizeof(union hash));
		if ( j==nchars-2 )
		    memset(sub[line[j]].table,-1,256*sizeof(union hash));
	    }
	    sub = sub[line[j]].table;
	}
	sub[line[j]].color = findcol((char *) line+j+1);
    }
return( tab );
}

GImage *GImageReadXpm(char * filename) {
   FILE *fp;
   GImage *ret=NULL;
   struct _GImage *base;
   int width, height, cols, nchar;
   unsigned char buf[80];
   unsigned char *line, *lpt;
   int y,j, lsiz;
   union hash *tab, *sub;
   unsigned char *pt, *end; unsigned long *ipt;
   int (*getdata)(unsigned char *,int,FILE *) = NULL;

   fp = fopen(filename, "r");
   if (!fp)
return( NULL );

    fgets((char *) buf,sizeof(buf),fp);
    if ( strstr((char *) buf,"XPM2")!=NULL )
	getdata = gww_getline;
    else if ( strstr((char *) buf,"/*")!=NULL && strstr((char *) buf,"XPM")!=NULL && strstr((char *) buf,"*/")!=NULL )
	getdata = getstring;
    if ( getdata==NULL ||
	    !getdata( buf,sizeof(buf),fp) ||
	    sscanf((char *) buf,"%d %d %d %d", &width, &height, &cols, &nchar)!=4 ) {
	fclose(fp);
return( NULL );
    }
    line = galloc(lsiz = nchar*width+20);
    tab = parse_colors(fp,line,lsiz,cols,nchar,getdata);
    if ( cols<=256 ) {
	Color clut[257];
	clut[256] = COLOR_UNKNOWN;
	fillupclut(clut,tab,0,nchar);
	ret = GImageCreate(it_index,width,height);
	ret->u.image->clut->clut_len = cols;
	memcpy(ret->u.image->clut->clut,clut,cols*sizeof(Color));
	ret->u.image->trans = clut[256];
	ret->u.image->clut->trans_index = clut[256];
    } else {
	ret = GImageCreate(it_true,width,height);
	ret->u.image->trans = TRANS;		/* TRANS isn't a valid Color, but it fits in our 32 bit pixels */
    }
    base = ret->u.image;
    for ( y=0; y<height; ++y ) {
	if ( !getdata(line,lsiz,fp)) {
	    GImageDestroy(ret);
	    freetab(tab,nchar);
	    fclose(fp);
return( NULL );
	}
	pt = (uint8 *) (base->data+y*base->bytes_per_line); ipt = NULL; end = pt+width;
	if ( cols>256 )
	    ipt = (unsigned long *) pt;
	for ( lpt=line; *line && pt<end; ) {
	    sub = tab;
	    for ( j=0; *lpt && j<nchar-1; ++j, ++lpt )
		if ( sub!=NULL )
		    sub = sub[*lpt].table;
	    if ( sub!=NULL ) {
		if ( cols<=256 )
		    *pt = sub[*lpt].color;
		else
		    *ipt = sub[*lpt].color;
	    }
	    ++pt; ++ipt; ++lpt;
	}
    }
    freetab(tab,nchar);
    fclose(fp);
return( ret );
}
