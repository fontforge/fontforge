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
#include "pfaeditui.h"
#include <gfile.h>
#include <math.h>
#include "utype.h"
#include "ustring.h"

static int gettoken(FILE *bdf, char *tokbuf, int size) {
    char *pt=tokbuf, *end = tokbuf+size-2; int ch;

    while ( isspace(ch = getc(bdf)));
    while ( ch!=EOF && !isspace(ch) && ch!='[' && ch!=']' && ch!='{' && ch!='}' && ch!='<' && ch!='%' ) {
	if ( pt<end ) *pt++ = ch;
	ch = getc(bdf);
    }
    if ( pt==tokbuf && ch!=EOF )
	*pt++ = ch;
    else
	ungetc(ch,bdf);
    *pt='\0';
return( pt!=tokbuf?1:ch==EOF?-1: 0 );
}

static int finduniname(char *name) {
    int i = -1;

    if ( strncmp(name,"uni",3)==0 ) { char *end;
	i = strtol(name+3,&end,16);
	if ( *end )
	    i = -1;
    }
    if ( i==-1 ) for ( i=psunicodenames_cnt-1; i>=0 ; --i ) {
	if ( psunicodenames[i]!=NULL )
	    if ( strcmp(name,psunicodenames[i])==0 )
    break;
    }
return( i );
}

static void MakeEncChar(SplineFont *sf,int enc,char *name) {
    BDFFont *b;

    if ( enc>=sf->charcnt ) {
	int i,n = enc+sf->charcnt;
	if ( n>65536 ) n=65536;
	if ( enc>n ) n = enc+1;
	sf->chars = grealloc(sf->chars,n*sizeof(SplineChar *));
	for ( i=sf->charcnt; i<n; ++i )
	    sf->chars[i] = NULL;
	sf->charcnt = n;
	for ( b=sf->bitmaps; b!=NULL; b=b->next ) if ( b->charcnt<n ) {
	    b->chars = grealloc(b->chars,n*sizeof(BDFChar *));
	    for ( i=b->charcnt; i<n; ++i )
		b->chars[i] = NULL;
	    b->charcnt = n;
	}
    }
    if ( sf->chars[enc]==NULL ) {
	sf->chars[enc] = chunkalloc(sizeof(SplineChar));
	sf->chars[enc]->parent = sf;
	sf->chars[enc]->width = sf->chars[enc]->vwidth = sf->ascent+sf->descent;
    }
    free(sf->chars[enc]->name);
    sf->chars[enc]->name = copy(name);
    sf->chars[enc]->unicodeenc = finduniname(name);
    sf->chars[enc]->enc = enc;
    /*sf->encoding_name = em_none;*/
}

static void AddBDFChar(FILE *bdf, SplineFont *sf, BDFFont *b) {
    BDFChar *bc;
    char name[40], tok[100];
    int enc=-1, width=0, xmin=0, xmax=0, ymin=0, ymax=0, hsz, vsz;
    int swidth= -1;
    int i;
    BitmapView *bv;
    uint8 *pt, *end;

    gettoken(bdf,name,sizeof(tok));
    while ( gettoken(bdf,tok,sizeof(tok))!=-1 ) {
	if ( strcmp(tok,"ENCODING")==0 )
	    fscanf(bdf,"%d",&enc);
	else if ( strcmp(tok,"DWIDTH")==0 )
	    fscanf(bdf,"%d %*d",&width);
	else if ( strcmp(tok,"SWIDTH")==0 )
	    fscanf(bdf,"%d %*d",&swidth);
	else if ( strcmp(tok,"BBX")==0 ) {
	    fscanf(bdf,"%d %d %d %d",&hsz, &vsz, &xmin, &ymin );
	    xmax = hsz+xmin-1;
	    ymax = vsz+ymin-1;
	} else if ( strcmp(tok,"BITMAP")==0 )
    break;
    }
    i = -1;
    if ( strcmp(name,".notdef")==0 ) {
	if ( enc<32 || (enc>=127 && enc<0xa0)) i=enc;
	else if ( sf->encoding_name==em_none ) i = enc;
	else if ( sf->onlybitmaps && sf->bitmaps==b && b->next==NULL ) i = enc;
	if ( i>=sf->charcnt ) i = -1;
	if ( i!=-1 && (sf->chars[i]==NULL || strcmp(sf->chars[i]->name,name)!=0 ))
	    MakeEncChar(sf,enc,name);
    } else {
	for ( i=sf->charcnt-1; i>=0 ; --i ) if ( sf->chars[i]!=NULL ) {
	    if ( strcmp(name,sf->chars[i]->name)==0 )
	break;
	}
	if ( i==-1 && (sf->encoding_name==em_unicode || sf->encoding_name==em_iso8859_1)) {
	    i = finduniname(name);
	    if ( i==-1 || i>sf->charcnt || sf->chars[i]!=NULL )
		i = -1;
	    if ( i!=-1 )
		SFMakeChar(sf,i);
	}
	if ( i==-1 && sf->onlybitmaps && sf->bitmaps==b && b->next==NULL && enc!=-1 ) {
	    MakeEncChar(sf,enc,name);
	    i = enc;
	}
	if ( i!=-1 && sf->onlybitmaps && sf->bitmaps==b && b->next==NULL && swidth!=-1 ) {
	    sf->chars[i]->width = swidth;
	    sf->chars[i]->widthset = true;
	} else if ( i!=-1 && swidth!=-1 && sf->chars[i]!=NULL &&
		sf->chars[i]->splines==NULL && sf->chars[i]->refs==NULL &&
		!sf->chars[i]->widthset ) {
	    sf->chars[i]->width = swidth;
	    sf->chars[i]->widthset = true;
	}
    }
    if ( i==-1 )	/* Can't guess the proper encoding, ignore it */
return;
    if ( i!=enc && sf->onlybitmaps && sf->bitmaps==b && b->next==NULL && sf->chars[enc]!=NULL ) {
	free(sf->chars[enc]->name);
	sf->chars[enc]->name = copy( ".notdef" );
	sf->chars[enc]->unicodeenc = -1;
    }
    if ( (bc=b->chars[i])!=NULL ) {
	free(bc->bitmap);
	BDFFloatFree(bc->selection);
    } else {
	b->chars[i] = bc = gcalloc(1,sizeof(BDFChar));
	bc->sc = sf->chars[i];
	bc->enc = i;
    }
    bc->xmin = xmin;
    bc->ymin = ymin;
    bc->xmax = xmax;
    bc->ymax = ymax;
    bc->width = width;
    bc->bytes_per_line = ((xmax-xmin)>>3) + 1;
    bc->bitmap = galloc(bc->bytes_per_line*(ymax-ymin+1));

    pt = bc->bitmap; end = pt + bc->bytes_per_line*(ymax-ymin+1);
    while ( pt<end ) {
	int ch1, ch2, val;
	while ( isspace(ch1=getc(bdf)) );
	ch2 = getc(bdf);
	val = 0;
	if ( ch1>='0' && ch1<='9' ) val = (ch1-'0')<<4;
	else if ( ch1>='a' && ch1<='f' ) val = (ch1-'a'+10)<<4;
	else if ( ch1>='A' && ch1<='F' ) val = (ch1-'A'+10)<<4;
	if ( ch2>='0' && ch2<='9' ) val |= (ch2-'0');
	else if ( ch2>='a' && ch2<='f' ) val |= (ch2-'a'+10);
	else if ( ch2>='A' && ch2<='F' ) val |= (ch2-'A'+10);
	*pt++ = val;
	if ( ch2==EOF )
    break;
    }

    for ( bv=bc->views; bv!=NULL; bv=bv->next )
	GDrawRequestExpose(bv->v,NULL,false);
}

static int slurp_header(FILE *bdf, int *_as, int *_ds, int *_enc, char *family, char *mods, char *full) {
    int pixelsize = -1;
    int ascent= -1, descent= -1, enc, cnt;
    char tok[100], encname[100], weight[100], italic[100];
    int ch;

    encname[0]= '\0'; family[0] = '\0'; weight[0]='\0'; italic[0]='\0'; full[0]='\0';
    while ( gettoken(bdf,tok,sizeof(tok))!=-1 ) {
	if ( strcmp(tok,"CHARS")==0 ) {
	    cnt=0;
	    fscanf(bdf,"%d",&cnt);
	    GProgressChangeTotal(cnt);
    break;
	}
	if ( strcmp(tok,"FONT")==0 ) {
	    fscanf(bdf," -%*[^-]-%[^-]-%[^-]-%[^-]-%*[^-]-", family, weight, italic );
	    while ( (ch = getc(bdf))!='-' && ch!=EOF );
	    fscanf(bdf,"%d", &pixelsize );
	} else if ( strcmp(tok,"SIZE")==0 && pixelsize==-1 ) {
	    int size, res;
	    fscanf(bdf, "%d %d", &size, &res );
	    pixelsize = rint( size*res/72.0 );
	} else if ( strcmp(tok,"QUAD_WIDTH")==0 )
	    fscanf(bdf, "%d", &pixelsize );
	else if ( strcmp(tok,"FONT_ASCENT")==0 )
	    fscanf(bdf, "%d", &ascent );
	else if ( strcmp(tok,"FONT_DESCENT")==0 )
	    fscanf(bdf, "%d", &descent );
	else if ( strcmp(tok,"CHARSET_REGISTRY")==0 )
	    fscanf(bdf, " \"%[^\"]", encname );
	else if ( strcmp(tok,"CHARSET_ENCODING")==0 )
	    fscanf(bdf, "%d", &enc );
	else if ( strcmp(tok,"FAMILY_NAME")==0 ) {
	    ch = getc(bdf);
	    ch = getc(bdf); ungetc(ch,bdf);
	    fscanf(bdf, " \"%[^\"]", family );
	} else if ( strcmp(tok,"FULL_NAME")==0 ) {
	    ch = getc(bdf);
	    ch = getc(bdf); ungetc(ch,bdf);
	    fscanf(bdf, " \"%[^\"]", full );
	} else if ( strcmp(tok,"WEIGHT_NAME")==0 )
	    fscanf(bdf, " \"%[^\"]", weight );
	else if ( strcmp(tok,"SLANT")==0 )
	    fscanf(bdf, " \"%[^\"]", italic );
	while ( (ch=getc(bdf))!='\n' && ch!='\r' && ch!=EOF );
    }
    if ( pixelsize==-1 && ascent!=-1 && descent!=-1 )
	pixelsize = ascent+descent;
    else if ( pixelsize!=-1 ) {
	if ( ascent==-1 && descent!=-1 )
	    ascent = pixelsize - descent;
	else if ( ascent!=-1 )
	    descent = pixelsize -ascent;
    }
    *_as = ascent; *_ds=descent;

    *_enc = em_none;
    if ( strcmp(encname,"ISO8859")==0 || strcmp(encname,"ISO-8859")==0 ) {
	*_enc = em_iso8859_1+enc;
	if ( *_enc>em_iso8859_11 )
	    --*_enc;
	if ( *_enc>em_iso8859_15 ) *_enc = em_iso8859_15;
    } else if ( strcmp(encname,"ISOLatin1Encoding")==0 ) {
	*_enc = em_iso8859_1;
    } else if ( strcmp(encname,"ISO10646")==0 || strcmp(encname,"ISO-10646")==0 ||
	    strcmp(encname,"Unicode")==0 || strcmp(encname,"UNICODE")==0 ) {
	*_enc = em_unicode;
    } else if ( strstrmatch(encname,"AdobeStandard")!=NULL ) {
	*_enc = em_adobestandard;
    } else if ( strstrmatch(encname,"Mac")!=NULL ) {
	*_enc = em_mac;
    } else if ( strstrmatch(encname,"Win")!=NULL || strstrmatch(encname,"ANSI")!=NULL ) {
	*_enc = em_win;
    } else if ( strstrmatch(encname,"koi8")!=NULL ) {
	*_enc = em_koi8_r;
    } else if ( strstrmatch(encname,"JISX208")!=NULL ) {
	*_enc = em_jis208;
    } else if ( strstrmatch(encname,"JISX212")!=NULL ) {
	*_enc = em_jis212;
    } else if ( strstrmatch(encname,"KSC5601")!=NULL ) {
	*_enc = em_ksc5601;
    } else if ( strstrmatch(encname,"GB2312")!=NULL ) {
	*_enc = em_gb2312;
    } else {
	Encoding *item;
	for ( item=enclist; item!=NULL && strstrmatch(encname,item->enc_name)==NULL; item=item->next );
	if ( item!=NULL )
	    *_enc = item->enc_num;
    }

    if ( strmatch(italic,"I")==0 )
	strcpy(italic,"Italic");
    else if ( strmatch(italic,"O")==0 )
	strcpy(italic,"Oblique");
    else if ( strmatch(italic,"R")==0 )
	strcpy(italic,"");		/* Ignore roman */
    sprintf(mods,"%s%s", weight, italic );

return( pixelsize );
}

static int askusersize(char *filename) {
    char *pt;
    int guess;
    unichar_t *ret, *end;
    char def[10];
    unichar_t udef[10];

    for ( pt=filename; *pt && !isdigit(*pt); ++pt );
    guess = strtol(pt,NULL,10);
    if ( guess!=0 )
	sprintf(def,"%d",guess);
    else
	*def = '\0';
    uc_strcpy(udef,def);
  retry:
    ret = GWidgetAskStringR(_STR_PixelSize,udef,_STR_PixelSizeFont);
    if ( ret==NULL )
	guess = -1;
    else {
	guess = u_strtol(ret,&end,10);
	free(ret);
	if ( guess<=0 || *end!='\0' ) {
	    GWidgetErrorR(_STR_BadNumber,_STR_BadNumber);
  goto retry;
	}
    }
return( guess );
}

static int alreadyexists(int pixelsize) {
    char buffer[10];
    unichar_t ubuf[200];
    const unichar_t *buts[3]; unichar_t oc[2];
    int ret;

    buts[2]=NULL;
    buts[0] = GStringGetResource( _STR_OK, &oc[0]);
    buts[1] = GStringGetResource( _STR_Cancel, &oc[1]);

    sprintf(buffer,"%d",pixelsize);
    u_strcpy(ubuf, GStringGetResource(_STR_Duppixelsizepre,NULL));
    uc_strcat(ubuf,buffer);
    u_strcat(ubuf, GStringGetResource(_STR_Duppixelsizepost,NULL));

    ret = GWidgetAsk(GStringGetResource(_STR_Duppixelsize,NULL),buts,oc,0,1,ubuf)==0;
return( ret );
}

enum pk_cmd { pk_rrr1=240, pk_rrr2, pk_rrr3, pk_rrr4, pk_yyy, pk_post, pk_no_op, pk_pre,
	pk_version_number=89 };
static void pk_skip_noops(FILE *pk) {
    uint8 cmd;
    int32 val;
    int i;

    while ( 1 ) {
	cmd = getc(pk);
	switch( cmd ) {
	  case pk_no_op:		/* One byte no-op */
	  break;
	  case pk_post:			/* Signals start of noop section */
	  break;
	  case pk_yyy:			/* followed by a 4 byte value */
	    getc(pk); getc(pk); getc(pk); getc(pk);
	  break;
	  case pk_rrr1:
	    val = getc(pk);
	    for ( i=0; i<val; ++i ) getc(pk);
	  break;
	  case pk_rrr2:
	    val = getc(pk);
	    val = (val<<8) | getc(pk);
	    for ( i=0; i<val; ++i ) getc(pk);
	  break;
	  case pk_rrr3:
	    val = getc(pk);
	    val = (val<<8) | getc(pk);
	    val = (val<<8) | getc(pk);
	    for ( i=0; i<val; ++i ) getc(pk);
	  break;
	  case pk_rrr4:
	    val = getc(pk);
	    val = (val<<8) | getc(pk);
	    val = (val<<8) | getc(pk);
	    val = (val<<8) | getc(pk);
	    for ( i=0; i<val; ++i ) getc(pk);
	  break;
	  default:
	    ungetc(cmd,pk);
return;
	}
    }
}

static int pk_header(FILE *pk, int *_as, int *_ds, int *_enc, char *family,
	char *mods, char *full, char *filename) {
    int pixelsize=-1;
    int ch,i;
    int design_size, pixels_per_point;
    double size;
    char *pt, *fpt;

    pk_skip_noops(pk);
    ch = getc(pk);
    if ( ch!=pk_pre )
return( -2 );
    ch = getc(pk);
    if ( ch!=pk_version_number )
return( -2 );
    ch = getc(pk);
    for ( i=0; i<ch; ++i ) getc(pk);		/* Skip comment. Perhaps that should be the family? */
    design_size = getlong(pk);
    /* checksum = */ getlong(pk);
    pixels_per_point = getlong(pk);
    /* vert pixels per point = */ getlong(pk);

    size = (pixels_per_point / 65536.0) * (design_size / (double) (0x100000));
    pixelsize = size+.5;
    *_enc = em_none;
    *_as = *_ds = -1;
    *mods = '\0';
    pt = strrchr(filename, '/');
    if ( pt==NULL ) pt = filename; else ++pt;
    for ( fpt=family; isalpha(*pt);)
	*fpt++ = *pt++;
    *fpt = '\0';
    strcpy(full,family);

return( pixelsize );
}

struct pkstate {
    int byte, hold;
    int rpt;
    int dyn_f;
    int cc;		/* for debug purposes */
};

static int pkgetcount(FILE *pk, struct pkstate *st) {
    int i,j;
#define getnibble(pk,st) (st->hold==1?(st->hold=0,(st->byte&0xf)):(st->hold=1,(((st->byte=getc(pk))>>4))) )

    while ( 1 ) {
	i = getnibble(pk,st);
	if ( i==0 ) {
	    j=0;
	    while ( i==0 ) { ++j; i=getnibble(pk,st); }
	    while ( j>0 ) { --j; i = (i<<4) + getnibble(pk,st); }
return( i-15 + (13-st->dyn_f)*16 + st->dyn_f );
	} else if ( i<=st->dyn_f ) {
return( i );
	} else if ( i<14 ) {
return( (i-st->dyn_f-1)*16 + getnibble(pk,st) + st->dyn_f + 1 );
	} else {
	    if ( st->rpt!=0 )
		fprintf( stderr, "Duplicate repeat row count in char %d of pk file\n", st->cc );
	    if ( i==15 ) st->rpt = 1;
	    else st->rpt = pkgetcount(pk,st);
 /*printf( "[%d]", st->rpt );*/
	}
    }
}

static int pk_char(FILE *pk, SplineFont *sf, BDFFont *b) {
    int flag = getc(pk);
    int black, size_is_2;
    int pl, cc, tfm, w, h, hoff, voff, dm, dx, dy;
    int i, ch, j,r,c,cnt;
    BDFChar *bc;
    struct pkstate st;
    char buf[20];
    int32 char_end;

    memset(&st,'\0', sizeof(st));

    /* flag byte */
    st.dyn_f = (flag>>4);
    if ( st.dyn_f==15 ) {
	ungetc(flag,pk);
return( 0 );
    }
    black = flag&8 ? 1 : 0;
    size_is_2 = flag&4 ? 1 : 0;

    if ( (flag&7)==7 ) {		/* long preamble, 4 byte sizes */
	pl = getlong(pk);
	cc = getlong(pk);
	char_end = ftell(pk) + pl;
	tfm = get3byte(pk);
	dx = getlong(pk)>>16;
	dy = getlong(pk)>>16;
	w = getlong(pk);
	h = getlong(pk);
	hoff = getlong(pk);
	voff = getlong(pk);
    } else if ( flag & 4 ) {		/* extended preamble, 2 byte sizes */
	pl = getushort(pk) + ((flag&3)<<16);
	cc = getc(pk);
	char_end = ftell(pk) + pl;
	tfm = get3byte(pk);
	dm = getushort(pk);
	dx = dm; dy = 0;
	w = getushort(pk);
	h = getushort(pk);
	hoff = (short) getushort(pk);
	voff = (short) getushort(pk);
    } else {				/* short, 1 byte sizes */
	pl = getc(pk) + ((flag&3)<<8);
	cc = getc(pk);
	char_end = ftell(pk) + pl;
	tfm = get3byte(pk);
	dm = getc(pk);
	dx = dm; dy = 0;
	w = getc(pk);
	h = getc(pk);
	hoff = (signed char) getc(pk);
	voff = (signed char) getc(pk);
    }
    st.cc = cc;			/* We can give better errors with this in st */
    /* hoff is -xmin, voff is ymax */
    /* w,h is the width,height of the bounding box */
    /* dx is the advance width? */
    /* cc is the character code */
    /* tfm is the advance width as a fraction of an em/2^20 so multiply by 1000/2^20 to get postscript values */
    /* dy is the advance height for vertical text? */

    if ( cc >= sf->charcnt ) {
	int new = ((sf->charcnt+255)>>8)<<8;
	sf->chars = grealloc(sf->chars,new*sizeof(SplineChar *));
	b->chars = grealloc(b->chars,new*sizeof(BDFChar *));
	for ( i=sf->charcnt; i<new; ++i ) {
	    sf->chars[i] = NULL;
	    b->chars[i] = NULL;
	}
	sf->charcnt = b->charcnt = new;
    }
    if ( sf->chars[cc]==NULL )
	SFMakeChar(sf,cc);
    if ( sf->onlybitmaps && sf->bitmaps==b && b->next==NULL ) {
	free(sf->chars[cc]->name);
	sprintf( buf, "enc-%d", cc);
	sf->chars[cc]->name = copy( buf );
	sf->chars[cc]->unicodeenc = -1;
    }
    if ( cc>=b->charcnt )
	bc = gcalloc(1,sizeof(BDFChar));
    else if ( (bc=b->chars[cc])!=NULL ) {
	free(bc->bitmap);
	BDFFloatFree(bc->selection);
    } else {
	b->chars[cc] = bc = gcalloc(1,sizeof(BDFChar));
	bc->sc = sf->chars[cc];
	bc->enc = cc;
    }

    bc->xmin = -hoff;
    bc->ymax = voff;
    bc->xmax = w-1-hoff;
    bc->ymin = voff-h+1;
    bc->width = dx;
    bc->bytes_per_line = ((w+7)>>3);
    bc->bitmap = gcalloc(bc->bytes_per_line*h,1);

    if ( sf->chars[cc]->splines==NULL && sf->chars[cc]->refs==NULL &&
	    !sf->chars[cc]->widthset ) {
	sf->chars[cc]->width = (sf->ascent+sf->descent)*(double) tfm/(0x100000);
	sf->chars[cc]->widthset = true;
    }

    if ( w==0 && h==0 )
	/* Nothing */;
    else if ( st.dyn_f==14 ) {
	/* We've got raster data in the file */
	for ( i=0; i<((w*h+7)>>3); ++i ) {
	    ch = getc(pk);
	    for ( j=0; j<8; ++j ) {
		r = ((i<<3)+j)/w;
		c = ((i<<3)+j)%w;
		if ( r<h && (ch&(1<<(7-j))) )
		    bc->bitmap[r*bc->bytes_per_line+(c>>3)] |= (1<<(7-(c&7)));
	    }
	}
    } else {
	/* We've got run-length encoded data */
	r = c = 0;
	while ( r<h ) {
	    cnt = pkgetcount(pk,&st);
  /* if ( black ) printf( "%d", cnt ); else printf( "(%d)", cnt );*/
	    if ( c+cnt>=w && st.rpt!=0 ) {
		if ( black ) {
		    while ( c<w ) {
			bc->bitmap[r*bc->bytes_per_line+(c>>3)] |= (1<<(7-(c&7)));
			--cnt;
			++c;
		    }
		} else
		    cnt -= (w-c);
		for ( i=0; i<st.rpt && r+i+1<h; ++i )
		    memcpy(bc->bitmap+(r+i+1)*bc->bytes_per_line,
			    bc->bitmap+r*bc->bytes_per_line,
			    bc->bytes_per_line );
		r += st.rpt+1;
		c = 0;
		st.rpt = 0;
	    }
	    while ( cnt>0 && r<h ) {
		while ( c<w && cnt>0) {
		    if ( black )
			bc->bitmap[r*bc->bytes_per_line+(c>>3)] |= (1<<(7-(c&7)));
		    --cnt;
		    ++c;
		}
		if ( c==w ) {
		    c = 0;
		    ++r;
		}
	    }
	    black = !black;
	}
    }
    if ( cc>=b->charcnt )
	BDFCharFree(bc);
    if ( ftell(pk)!=char_end ) {
	fprintf( stderr, "The character, %d, was not read properly (or pk file is in bad format)\n At %d should be %d, off by %d\n", cc, ftell(pk), char_end, ftell(pk)-char_end );
	fseek(pk,char_end,SEEK_SET);
    }
 /* printf( "\n" ); */
return( 1 );
}

BDFFont *SFImportBDF(SplineFont *sf, char *filename,int ispk, int toback) {
    FILE *bdf;
    char tok[100];
    int pixelsize, ascent, descent, enc;
    BDFFont *b;
    char family[100], mods[200], full[300];

    bdf = fopen(filename,"r");
    if ( bdf==NULL ) {
	GWidgetErrorR(_STR_CouldNotOpenFile, _STR_CouldNotOpenFileName, filename );
return( NULL );
    }
    if ( ispk ) {
	pixelsize = pk_header(bdf,&ascent,&descent,&enc,family,mods,full, filename);
	if ( pixelsize==-2 ) {
	    fclose(bdf);
	    GWidgetErrorR(_STR_NotPkFile, _STR_NotPkFileName, filename );
return( NULL );
	}
    } else {
	if ( gettoken(bdf,tok,sizeof(tok))==-1 || strcmp(tok,"STARTFONT")!=0 ) {
	    fclose(bdf);
	    GWidgetErrorR(_STR_NotBdfFile, _STR_NotBdfFileName, filename );
return( NULL );
	}
	pixelsize = slurp_header(bdf,&ascent,&descent,&enc,family,mods,full);
    }
    if ( pixelsize==-1 )
	pixelsize = askusersize(filename);
    if ( pixelsize==-1 ) {
	fclose(bdf);
return( NULL );
    }
    if ( !toback && sf->bitmaps==NULL && sf->onlybitmaps ) {
	/* Loading first bitmap into onlybitmap font sets the name and encoding */
	SFSetFontName(sf,family,mods,full);
	SFReencodeFont(sf,enc);
	sf->display_size = pixelsize;
    }

    b = NULL;
    if ( !toback )
	for ( b=sf->bitmaps; b!=NULL && b->pixelsize!=pixelsize; b=b->next );
    if ( b!=NULL ) {
	if ( !alreadyexists(pixelsize)) {
	    fclose(bdf);
return( NULL );
	}
    }
    if ( b==NULL ) {
	if ( ascent==-1 && descent==-1 )
	    ascent = rint(pixelsize*sf->ascent/(real) (sf->ascent+sf->descent));
	if ( ascent==-1 && descent!=-1 )
	    ascent = pixelsize - descent;
	else if ( ascent!=-1 )
	    descent = pixelsize -ascent;
	b = gcalloc(1,sizeof(BDFFont));
	b->sf = sf;
	b->charcnt = sf->charcnt;
	b->pixelsize = pixelsize;
	b->chars = gcalloc(sf->charcnt,sizeof(BDFChar *));
	b->ascent = ascent;
	b->descent = pixelsize-b->ascent;
	b->encoding_name = sf->encoding_name;
	if ( !toback ) {
	    b->next = sf->bitmaps;
	    sf->bitmaps = b;
	    SFOrderBitmapList(sf);
	}
    }
    if ( ispk ) {
	while ( pk_char(bdf,sf,b));
    } else {
	while ( gettoken(bdf,tok,sizeof(tok))!=-1 ) {
	    if ( strcmp(tok,"STARTCHAR")==0 ) {
		AddBDFChar(bdf,sf,b);
		GProgressNext();
	    }
	}
    }
    fclose(bdf);
    sf->changed = true;
return( b );
}

static void SFMergeBitmaps(SplineFont *sf,BDFFont *strikes) {
    BDFFont *b, *prev, *snext;

    while ( strikes ) {
	snext = strikes->next;
	strikes->next = NULL;
	for ( prev=NULL,b=sf->bitmaps; b!=NULL && b->pixelsize!=strikes->pixelsize; b=b->next );
	if ( b==NULL ) {
	    strikes->next = sf->bitmaps;
	    sf->bitmaps = strikes;
	} else if ( !alreadyexists(strikes->pixelsize)) {
	    BDFFontFree(strikes);
	} else {
	    strikes->next = b->next;
	    if ( prev==NULL )
		sf->bitmaps = strikes;
	    else
		prev->next = strikes;
	    BDFFontFree(b);
	}
	strikes = snext;
    }
}

static void FVAddToBackground(FontView *fv,BDFFont *bdf);

int FVImportBDF(FontView *fv, char *filename, int ispk, int toback) {
    BDFFont *b, *anyb=NULL;
    unichar_t ubuf[140];
    char *eod, *fpt, *file, *full;
    int fcnt, any = 0;
    int oldcharcnt = fv->sf->charcnt;

    eod = strrchr(filename,'/');
    *eod = '\0';
    fcnt = 1;
    fpt = eod+1;
    while (( fpt=strstr(fpt,"; "))!=NULL )
	{ ++fcnt; fpt += 2; }

    u_sprintf(ubuf, GStringGetResource(_STR_LoadingFrom,NULL), filename);
    GProgressStartIndicator(10,GStringGetResource(_STR_Loading,NULL),ubuf,GStringGetResource(_STR_ReadingGlyphs,NULL),0,fcnt);
    GProgressEnableStop(false);

    file = eod+1;
    do {
	fpt = strstr(file,"; ");
	if ( fpt!=NULL ) *fpt = '\0';
	full = galloc(strlen(filename)+1+strlen(file)+1);
	strcpy(full,filename); strcat(full,"/"); strcat(full,file);
	u_sprintf(ubuf, GStringGetResource(_STR_LoadingFrom,NULL), filename);
	GProgressChangeLine1(ubuf);
	b = SFImportBDF(fv->sf,full,ispk,toback);
	free(full);
	if ( fpt!=NULL ) GProgressNextStage();
	if ( b!=NULL ) {
	    anyb = b;
	    any = true;
	    if ( b==fv->show )
		GDrawRequestExpose(fv->v,NULL,false);
	}
	file = fpt+2;
    } while ( fpt!=NULL );
    GProgressEndIndicator();
    if ( oldcharcnt != fv->sf->charcnt ) {
	free(fv->selected);
	fv->selected = gcalloc(fv->sf->charcnt,sizeof(char));
	FontViewReformat(fv);
    }
    if ( anyb==NULL ) {
	GWidgetErrorR( _STR_NoBitmapFont, _STR_NoBitmapFontIn, filename );
    } else if ( toback )
	FVAddToBackground(fv,anyb);
return( any );
}

static void FVAddToBackground(FontView *fv,BDFFont *bdf) {
    SplineFont *sf = fv->sf;
    struct _GImage *base;
    GClut *clut;
    GImage *img;
    int i;
    SplineChar *sc; BDFChar *bdfc;
    real scale;

    for ( i=0; i<sf->charcnt && i<bdf->charcnt; ++i ) {
	if ( bdf->chars[i]!=NULL ) {
	    if ( sf->chars[i]==NULL )
		SFMakeChar(sf,i);
	    sc = sf->chars[i];
	    bdfc = bdf->chars[i];

	    base = gcalloc(1,sizeof(struct _GImage));
	    base->image_type = it_mono;
	    base->data = bdfc->bitmap;
	    base->bytes_per_line = bdfc->bytes_per_line;
	    base->width = bdfc->xmax-bdfc->xmin+1;
	    base->height = bdfc->ymax-bdfc->ymin+1;
	    bdfc->bitmap = NULL;

	    clut = gcalloc(1,sizeof(GClut));
	    clut->clut_len = 2;
	    clut->clut[0] = GDrawGetDefaultBackground(NULL);
	    clut->clut[1] = 0x808080;
	    clut->trans_index = 0;
	    base->trans = 0;
	    base->clut = clut;

	    img = gcalloc(1,sizeof(GImage));
	    img->u.image = base;

	    scale = (sf->ascent+sf->descent)/(double) (bdf->ascent+bdf->descent);
	    SCInsertBackImage(sc,img,scale,(bdfc->ymax+1)*scale,bdfc->xmin*scale);
	}
    }
    BDFFontFree(bdf);
}

int FVImportTTF(FontView *fv, char *filename, int toback) {
    SplineFont *strikeholder, *sf = fv->sf;
    BDFFont *strikes;
    unichar_t ubuf[100];

    u_snprintf(ubuf, sizeof(ubuf)/sizeof(ubuf[0]), GStringGetResource(_STR_LoadingFrom,NULL), filename);
    GProgressStartIndicator(10,GStringGetResource(_STR_Loading,NULL),ubuf,GStringGetResource(_STR_ReadingGlyphs,NULL),0,2);
    GProgressEnableStop(false);

    strikeholder = SFReadTTF(filename,toback?ttf_onlyonestrike|ttf_onlystrikes:ttf_onlystrikes);

    if ( strikeholder==NULL || (strikes = strikeholder->bitmaps)==NULL ) {
	SplineFontFree(strikeholder);
	GProgressEndIndicator();
return( false );
    }
    SFMatchEncoding(strikeholder,sf);
    if ( toback )
	FVAddToBackground(fv,strikes);
    else
	SFMergeBitmaps(sf,strikes);

    strikeholder->bitmaps =NULL;
    SplineFontFree(strikeholder);
    GProgressEndIndicator();
return( true );
}
