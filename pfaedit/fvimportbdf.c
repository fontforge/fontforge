/* Copyright (C) 2000,2001 by George Williams */
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
    } else {
	for ( i=sf->charcnt-1; i>=0 ; --i ) if ( sf->chars[i]!=NULL ) {
	    if ( strcmp(name,sf->chars[i]->name)==0 )
	break;
	}
	if ( i==-1 && (sf->encoding_name==em_unicode || sf->encoding_name==em_iso8859_1)) {
	    if ( strncmp(name,"uni",3)==0 ) { char *end;
		i = strtol(name+3,&end,16);
		if ( *end )
		    i = -1;
	    }
	    if ( i==-1 ) for ( i=sf->charcnt-1; i>=0 ; --i ) {
		if ( sf->chars[i]==NULL && i<psunicodenames_cnt && psunicodenames[i]!=NULL )
		    if ( strcmp(name,psunicodenames[i])==0 )
	    break;
	    }
	    if ( i!=-1 )
		SFMakeChar(sf,i);
	}
	if ( i!=-1 && sf->onlybitmaps && sf->bitmaps==b && b->next==NULL && swidth!=-1 )
	    sf->chars[i]->width = swidth;
    }
    if ( i==-1 )	/* Can't guess the proper encoding, ignore it */
return;
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
	
static int slurp_header(FILE *bdf, int *_as, int *_ds, int *_enc, char *family, char *mods) {
    int pixelsize = -1;
    int ascent= -1, descent= -1, enc, cnt;
    char tok[100], encname[100], weight[100], italic[100];
    int ch;

    encname[0]= '\0'; family[0] = '\0'; weight[0]='\0'; italic[0]='\0';
    while ( gettoken(bdf,tok,sizeof(tok))!=-1 ) {
	if ( strcmp(tok,"CHARS")==0 ) {
	    cnt=0;
	    fscanf(bdf,"%d",&cnt);
	    GProgressChangeTotal(cnt);
    break;
	}
	if ( strcmp(tok,"FONT")==0 )
	    fscanf(bdf,"-%*[^-]-%*[^-]-%*[^-]-%*[^-]-%*[^-]-%*[^-]-%d-", &pixelsize );
	else if ( strcmp(tok,"SIZE")==0 && pixelsize==-1 ) {
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

static unichar_t ok[] = { 'O', 'K', '\0' }, cancel[] = { 'C', 'a', 'n', 'c', 'e', 'l',  '\0' }, oc[] = { 'O', 'C' };
static unichar_t *buts[] = { ok, cancel, NULL };

static int askusersize(char *filename) {
    char *pt;
    int guess;
    unichar_t *ret, *end;
    char def[10];
    unichar_t udef[10], utit[40], uquest[80];

    for ( pt=filename; *pt && !isdigit(*pt); ++pt );
    guess = strtol(pt,NULL,10);
    if ( guess!=0 )
	sprintf(def,"%d",guess);
    else
	*def = '\0';
    uc_strcpy(udef,def);
    uc_strcpy(utit,"Pixel Size?");
    uc_strcpy(uquest,"What is the pixel size of the font in this file?");
  retry:
    ret = GWidgetAskString(utit,uquest,udef);
    if ( ret==NULL )
	guess = -1;
    else {
	guess = u_strtol(ret,&end,10);
	free(ret);
	if ( guess<=0 || *end!='\0' ) {
	    GDrawError("Bad number");
  goto retry;
	}
    }
return( guess );
}

static int alreadyexists(int pixelsize) {
    char buffer[200];
    unichar_t ubuf[200];
    unichar_t ubuf2[30];

    sprintf(buffer, "The font database already contains a bitmap\nfont with this pixelsize (%d)\nDo you want to overwrite it?", pixelsize );
    uc_strcpy(ubuf,buffer);
    uc_strcpy(ubuf2,"Duplicate pixelsize");
return( GWidgetAsk(ubuf2,ubuf,buts,oc,0,1)==0 );
}

BDFFont *SFImportBDF(SplineFont *sf, char *filename) {
    FILE *bdf;
    char tok[100];
    int pixelsize, ascent, descent, enc;
    BDFFont *b;
    char family[100], mods[200];

    bdf = fopen(filename,"r");
    if ( bdf==NULL ) {
	GDrawError("Couldn't open file %s", filename );
return( 0 );
    }
    if ( gettoken(bdf,tok,sizeof(tok))==-1 || strcmp(tok,"STARTFONT")!=0 ) {
	fclose(bdf);
	GDrawError("Not a bdf file: %s", filename );
return( NULL );
    }
    pixelsize = slurp_header(bdf,&ascent,&descent,&enc,family,mods);
    if ( pixelsize==-1 )
	pixelsize = askusersize(filename);
    if ( pixelsize==-1 ) {
	fclose(bdf);
return( NULL );
    }
    if ( sf->bitmaps==NULL && sf->onlybitmaps ) {
	/* Loading first bitmap into onlybitmap font sets the name and encoding */
	SFSetFontName(sf,family,mods);
	SFReencodeFont(sf,enc);
	sf->display_size = pixelsize;
    }

    for ( b=sf->bitmaps; b!=NULL && b->pixelsize!=pixelsize; b=b->next );
    if ( b!=NULL ) {
	if ( !alreadyexists(pixelsize)) {
	    fclose(bdf);
return( NULL );
	}
    }
    if ( b==NULL ) {
	if ( ascent==-1 && descent==-1 )
	    ascent = rint(pixelsize*sf->ascent/(double) (sf->ascent+sf->descent));
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
	b->next = sf->bitmaps;
	sf->bitmaps = b;
	SFOrderBitmapList(sf);
    }
    while ( gettoken(bdf,tok,sizeof(tok))!=-1 ) {
	if ( strcmp(tok,"STARTCHAR")==0 ) {
	    AddBDFChar(bdf,sf,b);
	    GProgressNext();
	}
    }
    fclose(bdf);
    sf->changed = true;
return( b );
}

int FVImportBDF(FontView *fv, char *filename) {
    BDFFont *b;
    static unichar_t loading[] = { 'L','o','a','d','i','n','g','.','.','.',  '\0' };
    static unichar_t reading[] = { 'R','e','a','d','i','n','g',' ','G','l','y','p','h','s',  '\0' };
    unichar_t ubuf[120];
    char buf[120];
    char *eod, *fpt, *file, *full;
    int fcnt, any = 0;

    eod = strrchr(filename,'/');
    *eod = '\0';
    fcnt = 1;
    fpt = eod+1;
    while (( fpt=strstr(fpt,"; "))!=NULL )
	{ ++fcnt; fpt += 2; }

    sprintf(buf, "Loading font from %.100s", "                                 ");
    uc_strcpy(ubuf,buf);
    GProgressStartIndicator(10,loading,ubuf,reading,0,fcnt);
    GProgressEnableStop(false);

    file = eod+1;
    do {
	fpt = strstr(file,"; ");
	if ( fpt!=NULL ) *fpt = '\0';
	full = galloc(strlen(filename)+1+strlen(file)+1);
	strcpy(full,filename); strcat(full,"/"); strcat(full,file);
	sprintf(buf, "Loading font from %.100s", file);
	uc_strcpy(ubuf,buf);
	GProgressChangeLine1(ubuf);
	b = SFImportBDF(fv->sf,full);
	free(full);
	GProgressNextStage();
	if ( b!=NULL ) {
	    any = true;
	    if ( b==fv->show )
		GDrawRequestExpose(fv->v,NULL,false);
	}
	file = fpt+2;
    } while ( fpt!=NULL );
    GProgressEndIndicator();
return( any );
}
