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
#include "pfaedit.h"
#include "splinefont.h"
#include "gdraw.h"
#include "ustring.h"
#include "utype.h"
#include <unistd.h>
#include <locale.h>

static void SFDDumpSplineSet(FILE *sfd,SplineSet *spl) {
    SplinePoint *first, *sp;

    for ( ; spl!=NULL; spl=spl->next ) {
	first = NULL;
	for ( sp = spl->first; ; sp=sp->next->to ) {
	    if ( first==NULL )
		fprintf( sfd, "%g %g m ", sp->me.x, sp->me.y );
	    else if ( sp->prev->islinear )
		fprintf( sfd, " %g %g l ", sp->me.x, sp->me.y );
	    else
		fprintf( sfd, " %g %g %g %g %g %g c ",
			sp->prev->from->nextcp.x, sp->prev->from->nextcp.y,
			sp->prevcp.x, sp->prevcp.y,
			sp->me.x, sp->me.y );
	    fprintf(sfd, "%d\n", sp->pointtype|(sp->selected<<2)|
			(sp->nextcpdef<<3)|(sp->prevcpdef<<4) );
	    if ( sp==first )
	break;
	    if ( first==NULL ) first = sp;
	    if ( sp->next==NULL )
	break;
	}
    }
    fprintf( sfd, "EndSplineSet\n" );
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

static void SFDDumpImage(FILE *sfd,ImageList *img) {
    GImage *image = img->image;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    struct enc85 enc;
    int i;

    fprintf(sfd, "Image: %d %d %d %d %d %x %g %g %g %g\n",
	    base->width, base->height, base->image_type,
	    base->image_type==it_true?3*base->width:base->bytes_per_line,
	    base->clut==NULL?0:base->clut->clut_len,base->trans,
	    img->xoff, img->yoff, img->xscale, img->yscale );
    memset(&enc,'\0',sizeof(enc));
    enc.sfd = sfd;
    if ( base->clut!=NULL ) {
	for ( i=0; i<base->clut->clut_len; ++i ) {
	    SFDEnc85(&enc,base->clut->clut[i]>>16);
	    SFDEnc85(&enc,(base->clut->clut[i]>>8)&0xff);
	    SFDEnc85(&enc,base->clut->clut[i]&0xff);
	}
    }
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
    SFDEnc85EndEnc(&enc);
    fprintf(sfd,"\nEndImage\n" );
}

#if 0
static void SFDDumpType1(FILE *sfd,SplineChar *sc) {
    struct enc85 enc;
    uint8 *pt, *end;

    if ( sc->origtype1!=NULL ) {
	fprintf( sfd, "OrigType1: %d\n", sc->origlen );
	memset(&enc,'\0',sizeof(enc));
	enc.sfd = sfd;
	pt = sc->origtype1;
	end = pt + sc->origlen;
	while ( pt<end ) {
	    SFDEnc85(&enc,*pt);
	    ++pt;
	}
	SFDEnc85EndEnc(&enc);
	fprintf(sfd,"\nEndType1\n" );
    }
}
#endif

static void SFDDumpHintList(FILE *sfd,char *key, StemInfo *h) {
    HintInstance *hi;

    if ( h==NULL )
return;
    fprintf(sfd, "%s", key );
    for ( ; h!=NULL; h=h->next ) {
	fprintf(sfd, "%g %g", h->start,h->width );
	if ( h->where!=NULL ) {
	    putc('<',sfd);
	    for ( hi=h->where; hi!=NULL; hi=hi->next )
		fprintf(sfd, "%g %g%c", hi->begin, hi->end, hi->next?' ':'>');
	}
	putc(h->next?' ':'\n',sfd);
    }
}

static void SFDDumpChar(FILE *sfd,SplineChar *sc) {
    RefChar *ref;
    ImageList *img;
    KernPair *kp;

    if ( sc->splines==NULL && sc->refs==NULL && sc->hstem==NULL && sc->vstem==NULL &&
	    sc->backgroundsplines==NULL && sc->backimages==NULL &&
	    !sc->widthset && sc->width==sc->parent->ascent+sc->parent->descent ) {
	if ( strcmp(sc->name,".notdef")==0 ||
		(sc->enc==sc->unicodeenc && (sc->enc<32 || (sc->enc>=0x7f && sc->enc<0xa0))) )
return;
    }
    fprintf(sfd, "StartChar: %s\n", sc->name );
    fprintf(sfd, "Encoding: %d %d\n", sc->enc, sc->unicodeenc);
    fprintf(sfd, "Width: %d\n", sc->width );
    if ( sc->changedsincelasthinted|| sc->manualhints || sc->widthset )
	fprintf(sfd, "Flags: %s%s%s\n",
		sc->changedsincelasthinted?"H":"",
		sc->manualhints?"M":"",
		sc->widthset?"W":"");
    SFDDumpHintList(sfd,"HStem: ", sc->hstem);
    SFDDumpHintList(sfd,"VStem: ", sc->vstem);
    if ( sc->splines!=NULL ) {
	fprintf(sfd, "Fore\n" );
	SFDDumpSplineSet(sfd,sc->splines);
    }
    for ( ref=sc->refs; ref!=NULL; ref=ref->next ) if ( ref->sc!=NULL ) {
	if ( ref->sc->enc==0 && ref->sc->splines==NULL )
	    fprintf( stderr, "Using a reference to character 0, removed. In %s\n", sc->name);
	else
	fprintf(sfd, "Ref: %d %c %g %g %g %g %g %g\n", ref->sc->enc,
		ref->selected?'S':'N',
		ref->transform[0], ref->transform[1], ref->transform[2],
		ref->transform[3], ref->transform[4], ref->transform[5]);
    }
#if 0
    SFDDumpType1(sfd,sc);
#endif
    if ( sc->backgroundsplines!=NULL ) {
	fprintf(sfd, "Back\n" );
	SFDDumpSplineSet(sfd,sc->backgroundsplines);
    }
    for ( img=sc->backimages; img!=NULL; img=img->next )
	SFDDumpImage(sfd,img);
    if ( sc->kerns!=NULL ) {
	fprintf( sfd, "Kerns:" );
	for ( kp = sc->kerns; kp!=NULL; kp=kp->next )
	    if ( kp->off!=0 )
		fprintf( sfd, " %d %d", kp->sc->enc, kp->off );
	fprintf(sfd, "\n" );
    }
    if ( sc->lig!=NULL )
	fprintf( sfd, "Ligature: %s\n", sc->lig->componants );
    fprintf(sfd,"EndChar\n" );
}

static void SFDDumpBitmapChar(FILE *sfd,BDFChar *bfc) {
    struct enc85 enc;
    int i;

    fprintf(sfd, "BDFChar: %d %d %d %d %d %d\n",
	    bfc->enc, bfc->width, bfc->xmin, bfc->xmax, bfc->ymin, bfc->ymax );
    memset(&enc,'\0',sizeof(enc));
    enc.sfd = sfd;
    for ( i=0; i<=bfc->ymax-bfc->ymin; ++i ) {
	uint8 *pt = (uint8 *) (bfc->bitmap + i*bfc->bytes_per_line);
	uint8 *end = pt + (bfc->xmax-bfc->xmin)/8+1;
	while ( pt<end ) {
	    SFDEnc85(&enc,*pt);
	    ++pt;
	}
    }
    SFDEnc85EndEnc(&enc);
#if 0
    fprintf(sfd,"\nEndBitmapChar\n" );
#else
    fputc('\n',sfd);
#endif
}

static void SFDDumpBitmapFont(FILE *sfd,BDFFont *bdf) {
    int i;

    GProgressNextStage();
    fprintf( sfd, "BitmapFont: %d %d %d %d\n", bdf->pixelsize, bdf->charcnt,
	    bdf->ascent, bdf->descent );
    for ( i=0; i<bdf->charcnt; ++i ) {
	if ( bdf->chars[i]!=NULL )
	    SFDDumpBitmapChar(sfd,bdf->chars[i]);
	GProgressNext();
    }
    fprintf( sfd, "EndBitmapFont\n" );
}

#if 0
static void SFDDumpSubrs(FILE *sfd,struct pschars *subrs) {
    int i;
    uint8 *pt, *end;
    struct enc85 enc;
    /* These guys are unnamed strings of random bytes */

    memset(&enc,'\0',sizeof(enc));
    enc.sfd = sfd;

    fprintf( sfd, "BeginSubrs: %d\n", subrs->next );
    for ( i=0; i<subrs->next; ++i ) {
	fprintf( sfd, "%d", subrs->lens[i]);
	if ( i==subrs->next-1 || i%30==29 )
	    putc('\n',sfd);
	else
	    putc(' ',sfd);
    }
    for ( i=0; i<subrs->next; ++i ) {
	for ( pt = subrs->values[i], end=pt+subrs->lens[i]; pt<end; ++pt )
	    SFDEnc85(&enc,*pt);
    }
    SFDEnc85EndEnc(&enc);
    fprintf( sfd, "EndSubrs\n" );
}
#endif

static void SFDDumpPrivate(FILE *sfd,struct psdict *private) {
    int i;
    char *pt;
    /* These guys should all be ascii text */
    fprintf( sfd, "BeginPrivate: %d\n", private->next );
    for ( i=0; i<private->next ; ++i ) {
	fprintf( sfd, "%s %d ", private->keys[i], strlen(private->values[i]));
	for ( pt = private->values[i]; *pt; ++pt )
	    putc(*pt,sfd);
	putc('\n',sfd);
    }
    fprintf( sfd, "EndPrivate\n" );
}

static void SFDDump(FILE *sfd,SplineFont *sf) {
    int i, realcnt;
    BDFFont *bdf;
    static unichar_t saving[] = { 'S','a','v','i','n','g','.','.','.',  '\0' };
    static unichar_t savingdb[] = { 'S','a','v','i','n','g',' ','S','p','l','i','n','e',' ','F','o','n','t',' ','D','a','t','a','b','a','s','e',  '\0' };
    static unichar_t savingoutlines[] = { 'S','a','v','i','n','g',' ','O','u','t','l','i','n','e','s',  '\0' };
    static unichar_t savingbitmaps[] = { 'S','a','v','i','n','g',' ','B','i','t','m','a','p','s',  '\0' };

    for ( i=0, bdf = sf->bitmaps; bdf!=NULL; bdf=bdf->next, ++i );
    GProgressStartIndicator(10,saving,savingdb,savingoutlines,sf->charcnt,i+1);
    GProgressEnableStop(false);
    fprintf(sfd, "SplineFontDB: %.1f\n", 1.0 );
    fprintf(sfd, "FontName: %s\n", sf->fontname );
    if ( sf->fullname!=NULL )
	fprintf(sfd, "FullName: %s\n", sf->fullname );
    if ( sf->familyname!=NULL )
	fprintf(sfd, "FamilyName: %s\n", sf->familyname );
    if ( sf->weight!=NULL )
	fprintf(sfd, "Weight: %s\n", sf->weight );
    if ( sf->copyright!=NULL )
	fprintf(sfd, "Copyright: %s\n", sf->copyright );
    if ( sf->version!=NULL )
	fprintf(sfd, "Version: %s\n", sf->version );
    fprintf(sfd, "ItalicAngle: %g\n", sf->italicangle );
    fprintf(sfd, "UnderlinePosition: %g\n", sf->upos );
    fprintf(sfd, "UnderlineWidth: %g\n", sf->uwidth );
    fprintf(sfd, "Ascent: %d\n", sf->ascent );
    fprintf(sfd, "Descent: %d\n", sf->descent );
    if ( sf->xuid!=NULL )
	fprintf(sfd, "XUID: %s\n", sf->xuid );
    if ( sf->pfminfo.pfmset ) {
	fprintf(sfd, "PfmFamily: %d\n", sf->pfminfo.family );
	fprintf(sfd, "PfmWeight: %d\n", sf->pfminfo.weight );
    }
    if ( sf->encoding_name>=em_base ) {
	Encoding *item;
	for ( item = enclist; item!=NULL && item->enc_num!=sf->encoding_name; item = item->next );
	if ( item==NULL )
	    fprintf(sfd, "Encoding: %d\n", em_none );
	else
	    fprintf(sfd, "Encoding: %s\n", item->enc_name );
    } else
	fprintf(sfd, "Encoding: %d\n", sf->encoding_name );
    if ( sf->display_size!=0 )
	fprintf( sfd, "DisplaySize: %d\n", sf->display_size );
    if ( sf->display_antialias!=0 )
	fprintf( sfd, "AntiAlias: %d\n", sf->display_antialias );
    if ( sf->onlybitmaps!=0 )
	fprintf( sfd, "OnlyBitmaps: %d\n", sf->onlybitmaps );
    if ( sf->gridsplines!=NULL ) {
	fprintf(sfd, "Grid\n" );
	SFDDumpSplineSet(sfd,sf->gridsplines);
    }
#if 0
    if ( sf->subrs!=NULL )
	SFDDumpSubrs(sfd,sf->subrs);
#endif
    if ( sf->private!=NULL )
	SFDDumpPrivate(sfd,sf->private);

    realcnt = 0;
    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL )
	++realcnt;
    fprintf(sfd, "BeginChars: %d %d\n", sf->charcnt, realcnt );
    for ( i=0; i<sf->charcnt; ++i ) {
	if ( sf->chars[i]!=NULL )
	    SFDDumpChar(sfd,sf->chars[i]);
	GProgressNext();
    }
    fprintf(sfd, "EndChars\n" );
    if ( sf->bitmaps!=NULL )
	GProgressChangeLine2(savingbitmaps);
    for ( bdf = sf->bitmaps; bdf!=NULL; bdf=bdf->next )
	SFDDumpBitmapFont(sfd,bdf);
    fprintf(sfd, "EndSplineFont\n" );
    GProgressEndIndicator();
}

int SFDWrite(char *filename,SplineFont *sf) {
    FILE *sfd = fopen(filename,"w");
    char *oldloc;

    if ( sfd==NULL )
return( 0 );
    oldloc = setlocale(LC_NUMERIC,"C");
    SFDDump(sfd,sf);
    setlocale(LC_NUMERIC,oldloc);
return( !ferror(sfd) && fclose(sfd)==0 );
}

int SFDWriteBak(SplineFont *sf) {
    char *buf, *pt, *bpt;

    buf = galloc(strlen(sf->filename)+10);
    pt = strrchr(sf->filename,'.');
    if ( pt==NULL || pt<strrchr(sf->filename,'/'))
	pt = sf->filename+strlen(sf->filename);
    strcpy(buf,sf->filename);
    bpt = buf + (pt-sf->filename);
    *bpt++ = '~';
    strcpy(bpt,pt);
    rename(sf->filename,buf);
    free(buf);

return( SFDWrite(sf->filename,sf));
}

/* ********************************* INPUT ********************************** */

static int geteol(FILE *sfd, char *tokbuf) {
    char *pt=tokbuf, *end = tokbuf+200-2; int ch;

    while ( isspace(ch = getc(sfd)) && ch!='\r' && ch!='\n' );
    while ( ch!='\n' && ch!='\r' && ch!=EOF ) {
	if ( pt<end ) *pt++ = ch;
	ch = getc(sfd);
    }
    *pt='\0';
return( pt!=tokbuf?1:ch==EOF?-1: 0 );
}

static int getname(FILE *sfd, char *tokbuf) {
    char *pt=tokbuf, *end = tokbuf+100-2; int ch;

    while ( isspace(ch = getc(sfd)));
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

static int getsint(FILE *sfd, int16 *val) {
    int val2;
    int ret = getint(sfd,&val2);
    *val = val2;
return( ret );
}

static int getdouble(FILE *sfd, double *val) {
    char tokbuf[100], ch;
    char *pt=tokbuf, *end = tokbuf+100-2, *nend;

    while ( isspace(ch = getc(sfd)));
    if ( ch!='e' && ch!='E' )		/* double's can't begin with exponants */
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

static ImageList *SFDGetImage(FILE *sfd) {
    /* We've read the image token */
    int width, height, image_type, bpl, clutlen, trans;
    struct _GImage *base;
    GImage *image;
    ImageList *img;
    struct enc85 dec;
    int i;

    memset(&dec,'\0', sizeof(dec)); dec.pos = -1;
    dec.sfd = sfd;

    getint(sfd,&width);
    getint(sfd,&height);
    getint(sfd,&image_type);
    getint(sfd,&bpl);
    getint(sfd,&clutlen);
    getint(sfd,&trans);
    image = GImageCreate(image_type,width,height);
    base = image->list_len==0?image->u.image:image->u.images[0];
    img = gcalloc(1,sizeof(ImageList));
    img->image = image;
    getdouble(sfd,&img->xoff);
    getdouble(sfd,&img->yoff);
    getdouble(sfd,&img->xscale);
    getdouble(sfd,&img->yscale);
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
    img->bb.minx = img->xoff; img->bb.maxy = img->yoff;
    img->bb.maxx = img->xoff + GImageGetWidth(img->image)*img->xscale;
    img->bb.miny = img->yoff - GImageGetHeight(img->image)*img->yscale;
return( img );
}

static void SFDGetType1(FILE *sfd, SplineChar *sc) {
    /* We've read the OrigType1 token */
    int len;
    struct enc85 dec;

    memset(&dec,'\0', sizeof(dec)); dec.pos = -1;
    dec.sfd = sfd;

    getint(sfd,&len);
#if 0
    sc->origtype1 = pt = galloc(len);
    sc->origlen = len;
    end = pt+len;
    while ( pt<end ) {
	*pt++ = Dec85(&dec);
    }
#else
    while ( --len >= 0 )
	Dec85(&dec);
#endif
}

static void SFDCloseCheck(SplinePointList *spl) {
    if ( spl->first!=spl->last &&
	    DoubleNear(spl->first->me.x,spl->last->me.x) &&
	    DoubleNear(spl->first->me.y,spl->last->me.y)) {
	SplinePoint *oldlast = spl->last;
	spl->first->prevcp = oldlast->prevcp;
	spl->first->noprevcp = false;
	oldlast->prev->from->next = NULL;
	spl->last = oldlast->prev->from;
	free(oldlast->prev);
	free(oldlast);
	SplineMake(spl->last,spl->first);
	spl->last = spl->first;
    }
}

static SplineSet *SFDGetSplineSet(FILE *sfd) {
    SplinePointList *cur=NULL, *head=NULL;
    BasePoint current;
    double stack[100];
    int sp=0;
    SplinePoint *pt;
    int ch;
    char tok[100];

    current.x = current.y = 0;
    while ( 1 ) {
	while ( getdouble(sfd,&stack[sp])==1 )
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
		pt = calloc(1,sizeof(SplinePoint));
		pt->me = current;
		pt->noprevcp = true; pt->nonextcp = true;
		if ( ch=='m' ) {
		    SplinePointList *spl = calloc(1,sizeof(SplinePointList));
		    spl->first = spl->last = pt;
		    if ( cur!=NULL ) {
			SFDCloseCheck(cur);
			cur->next = spl;
		    } else
			head = spl;
		    cur = spl;
		} else {
		    if ( cur!=NULL && cur->first!=NULL && (cur->first!=cur->last || cur->first->next==NULL) ) {
			SplineMake(cur->last,pt);
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
		    pt = calloc(1,sizeof(SplinePoint));
		    pt->prevcp.x = stack[sp-4];
		    pt->prevcp.y = stack[sp-3];
		    pt->me = current;
		    pt->nonextcp = true;
		    SplineMake(cur->last,pt);
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
	}
    }
    if ( cur!=NULL )
	SFDCloseCheck(cur);
    getname(sfd,tok);
return( head );
}

static HintInstance *SFDReadHintInstances(FILE *sfd) {
    HintInstance *head=NULL, *last=NULL, *cur;
    double begin, end;
    int ch;

    while ( (ch=getc(sfd))==' ' || ch=='\t' );
    if ( ch!='<' ) {
	ungetc(ch,sfd);
return(NULL);
    }
    while ( getdouble(sfd,&begin)==1 && getdouble(sfd,&end)) {
	cur = gcalloc(1,sizeof(HintInstance));
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
    double start, width;

    while ( getdouble(sfd,&start)==1 && getdouble(sfd,&width)) {
	cur = gcalloc(1,sizeof(StemInfo));
	cur->start = start;
	cur->width = width;
	cur->where = SFDReadHintInstances(sfd);
	if ( head == NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;
    }
return( head );
}

static RefChar *SFDGetRef(FILE *sfd) {
    RefChar *rf;
    int enc=0, ch;

    rf = gcalloc(1,sizeof(RefChar));
    getint(sfd,&enc);
    rf->local_enc = enc;
    while ( isspace(ch=getc(sfd)));
    if ( ch=='S' ) rf->selected = true;
    getdouble(sfd,&rf->transform[0]);
    getdouble(sfd,&rf->transform[1]);
    getdouble(sfd,&rf->transform[2]);
    getdouble(sfd,&rf->transform[3]);
    getdouble(sfd,&rf->transform[4]);
    getdouble(sfd,&rf->transform[5]);
return( rf );
}

static SplineChar *SFDGetChar(FILE *sfd) {
    SplineChar *sc;
    char tok[200], ch;
    RefChar *lastr=NULL, *ref;
    ImageList *lasti=NULL, *img;

    if ( getname(sfd,tok)!=1 )
return( NULL );
    if ( strcmp(tok,"StartChar:")!=0 )
return( NULL );
    if ( getname(sfd,tok)!=1 )
return( NULL );

    sc = gcalloc(1,sizeof(SplineChar));
    sc->name = copy(tok);
    while ( 1 ) {
	if ( getname(sfd,tok)!=1 )
return( NULL );
	if ( strmatch(tok,"Encoding:")==0 ) {
	    getint(sfd,&sc->enc);
	    getint(sfd,&sc->unicodeenc);
	} else if ( strmatch(tok,"Width:")==0 ) {
	    getint(sfd,&sc->width);
	} else if ( strmatch(tok,"Flags:")==0 ) {
	    while ( isspace(ch=getc(sfd)) && ch!='\n' && ch!='\r');
	    while ( ch!='\n' && ch!='\r' ) {
		if ( ch=='H' ) sc->changedsincelasthinted=true;
		else if ( ch=='M' ) sc->manualhints = true;
		else if ( ch=='W' ) sc->widthset = true;
		ch = getc(sfd);
	    }
	} else if ( strmatch(tok,"HStem:")==0 ) {
	    sc->hstem = SFDReadHints(sfd);
	    SCGuessHHintInstancesList(sc);		/* For reading in old .sfd files with no HintInstance data */
	    sc->hconflicts = StemListAnyConflicts(sc->hstem);
	} else if ( strmatch(tok,"VStem:")==0 ) {
	    sc->vstem = SFDReadHints(sfd);
	    SCGuessVHintInstancesList(sc);		/* For reading in old .sfd files */
	    sc->vconflicts = StemListAnyConflicts(sc->vstem);
	} else if ( strmatch(tok,"Fore")==0 ) {
	    sc->splines = SFDGetSplineSet(sfd);
	} else if ( strmatch(tok,"Back")==0 ) {
	    sc->backgroundsplines = SFDGetSplineSet(sfd);
	} else if ( strmatch(tok,"Ref:")==0 ) {
	    ref = SFDGetRef(sfd);
	    if ( lastr==NULL )
		sc->refs = ref;
	    else
		lastr->next = ref;
	    lastr = ref;
	} else if ( strmatch(tok,"OrigType1:")==0 ) {	/* Accept, slurp, ignore contents */
	    SFDGetType1(sfd,sc);
	} else if ( strmatch(tok,"Image:")==0 ) {
	    img = SFDGetImage(sfd);
	    if ( lasti==NULL )
		sc->backimages = img;
	    else
		lasti->next = img;
	    lasti = img;
	} else if ( strmatch(tok,"Kerns:")==0 ) {
	    KernPair *kp, *last=NULL;
	    int index, off;
	    while ( fscanf(sfd,"%d %d", &index, &off )==2 ) {
		kp = galloc(sizeof(KernPair));
		kp->sc = (SplineChar *) index;
		kp->off = off;
		kp->next = NULL;
		if ( last == NULL )
		    sc->kerns = kp;
		else
		    last->next = kp;
		last = kp;
	    }
	} else if ( strmatch(tok,"Ligature:")==0 ) {
	    geteol(sfd,tok);
	    sc->lig = gcalloc(1,sizeof(Ligature));
	    sc->lig->componants = copy(tok);
	    sc->lig->lig = sc;
	} else if ( strmatch(tok,"EndChar")==0 ) {
return( sc );
	} else {
	    geteol(sfd,tok);
	}
    }
}

static int SFDGetBitmapChar(FILE *sfd,BDFFont *bdf) {
    BDFChar *bfc;
    struct enc85 dec;
    int i;

    bfc = gcalloc(1,sizeof(BDFChar));
    
    if ( getint(sfd,&bfc->enc)!=1 || bfc->enc<0 )
return( 0 );
    if ( getsint(sfd,&bfc->width)!=1 )
return( 0 );
    if ( getsint(sfd,&bfc->xmin)!=1 )
return( 0 );
    if ( getsint(sfd,&bfc->xmax)!=1 || bfc->xmax<bfc->xmin )
return( 0 );
    if ( getsint(sfd,&bfc->ymin)!=1 )
return( 0 );
    if ( getsint(sfd,&bfc->ymax)!=1 || bfc->ymax<bfc->ymin )
return( 0 );

    bdf->chars[bfc->enc] = bfc;
    bfc->sc = bdf->sf->chars[bfc->enc];
    bfc->bytes_per_line = (bfc->xmax-bfc->xmin)/8 +1;
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
	bdf->chars[bfc->enc] = NULL;
	BDFCharFree(bfc);
    }
return( 1 );
}

static int SFDGetBitmapFont(FILE *sfd,SplineFont *sf) {
    BDFFont *bdf, *prev;
    char tok[200];

    bdf = gcalloc(1,sizeof(BDFFont));
    bdf->encoding_name = sf->encoding_name;

    if ( getint(sfd,&bdf->pixelsize)!=1 || bdf->pixelsize<=0 )
return( 0 );
    if ( getint(sfd,&bdf->charcnt)!=1 || bdf->charcnt<0 )
return( 0 );
    if ( getint(sfd,&bdf->ascent)!=1 || bdf->ascent<0 )
return( 0 );
    if ( getint(sfd,&bdf->descent)!=1 || bdf->descent<0 )
return( 0 );

    if ( sf->bitmaps==NULL )
	sf->bitmaps = bdf;
    else {
	for ( prev=sf->bitmaps; prev->next!=NULL; prev=prev->next );
	prev->next = bdf;
    }
    bdf->sf = sf;
    bdf->chars = gcalloc(bdf->charcnt,sizeof(BDFChar *));

    while ( getname(sfd,tok)==1 ) {
	if ( strcmp(tok,"BDFChar:")==0 )
	    SFDGetBitmapChar(sfd,bdf);
	else if ( strcmp(tok,"EndBitmapFont")==0 )
    break;
    }
return( 1 );
}

static void SFDFixupRef(SplineChar *sc,RefChar *ref) {
    RefChar *rf;

    for ( rf = ref->sc->refs; rf!=NULL; rf=rf->next ) {
	if ( rf->splines==NULL )
	    SFDFixupRef(ref->sc,rf);
    }
    SCReinstanciateRefChar(sc,ref);
    SCMakeDependent(sc,ref->sc);
}

static void SFDFixupRefs(SplineFont *sf) {
    int i;
    RefChar *refs;
    /*int isautorecovery = sf->changed;*/
    KernPair *kp;

    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	/* A changed character is one that has just been recovered */
	/*  unchanged characters will already have been fixed up */
	/* Er... maybe not. If the character being recovered is refered to */
	/*  by another character then we need to fix up that other char too*/
	/*if ( isautorecovery && !sf->chars[i]->changed )*/
    /*continue;*/
	for ( refs = sf->chars[i]->refs; refs!=NULL; refs=refs->next ) {
	    if ( refs->local_enc<sf->charcnt )
		refs->sc = sf->chars[refs->local_enc];
	    if ( refs->sc!=NULL ) {
		refs->unicode_enc = refs->sc->unicodeenc;
		refs->adobe_enc = getAdobeEnc(refs->sc->name);
	    }
	}
	/*if ( isautorecovery && !sf->chars[i]->changed )*/
    /*continue;*/
	for ( kp=sf->chars[i]->kerns; kp!=NULL; kp=kp->next )
	    kp->sc = sf->chars[(int) (kp->sc)];
    }
    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	for ( refs = sf->chars[i]->refs; refs!=NULL; refs=refs->next ) {
	    SFDFixupRef(sf->chars[i],refs);
	}
	GProgressNext();
    }
}

/* When we recover from an autosaved file we must be careful. If that file */
/*  contains a character that is refered to by another character then the */
/*  dependent list will contain a dead pointer without this routine. Similarly*/
/*  for kerning */
/* We might have needed to do something for references except they've already */
/*  got a local encoding field and passing through SFDFixupRefs will much their*/
/*  SplineChar pointer */
static void SFRemoveDependencies(SplineFont *sf) {
    int i;
    struct splinecharlist *dlist, *dnext;
    KernPair *kp;

    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	for ( dlist = sf->chars[i]->dependents; dlist!=NULL; dlist = dnext ) {
	    dnext = dlist->next;
	    free(dlist);
	}
	sf->chars[i]->dependents = NULL;
	for ( kp=sf->chars[i]->kerns; kp!=NULL; kp=kp->next )
	    kp->sc = (SplineChar *) (kp->sc->enc);
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
#if 0
    int i, cnt;
    struct enc85 dec;
    unsigned char *pt, *end;

    sf->subrs = gcalloc(1,sizeof(struct pschars));
    getint(sfd,&cnt);
    sf->subrs->next = sf->subrs->cnt = cnt;
    sf->subrs->values = gcalloc(cnt,sizeof(char *));
    sf->subrs->lens = gcalloc(cnt,sizeof(int));
    for ( i=0; i<cnt; ++i ) {
	getint(sfd,&sf->subrs->lens[i]);
	sf->subrs->values[i] = galloc(sf->subrs->lens[i]+1);
    }

    memset(&dec,'\0', sizeof(dec)); dec.pos = -1;
    dec.sfd = sfd;
    for ( i=0; i<cnt; ++i ) {
	for ( pt=sf->subrs->values[i], end = pt+sf->subrs->lens[i]; pt<end; ++pt )
	    *pt = Dec85(&dec);
	*pt = '\0';
    }
#else
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
#endif
}

static SplineFont *SFDGetFont(FILE *sfd) {
    SplineFont *sf;
    char tok[200];
    double dval;
    SplineChar *sc;
    int realcnt, ch;
    static unichar_t interpret[] = { 'I','n','t','e','p','r','e','t','i','n','g',' ','G','l','y','p','h','s',  '\0' };

    if ( getname(sfd,tok)!=1 )
return( NULL );
    if ( strcmp(tok,"SplineFontDB:")!=0 )
return( NULL );
    if ( getdouble(sfd,&dval)!=1 || (dval!=0 && dval!=1))
return( NULL );
    ch = getc(sfd); ungetc(ch,sfd);
    if ( ch!='\r' && ch!='\n' )
return( NULL );

    sf = gcalloc(1,sizeof(SplineFont));
    while ( 1 ) {
	if ( getname(sfd,tok)!=1 ) {
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
	    getname(sfd,tok);
	    sf->familyname = copy(tok);
	} else if ( strmatch(tok,"Weight:")==0 ) {
	    getname(sfd,tok);
	    sf->weight = copy(tok);
	} else if ( strmatch(tok,"Copyright:")==0 ) {
	    geteol(sfd,tok);
	    sf->copyright = copy(tok);
	} else if ( strmatch(tok,"Version:")==0 ) {
	    geteol(sfd,tok);
	    sf->version = copy(tok);
	} else if ( strmatch(tok,"ItalicAngle:")==0 ) {
	    getdouble(sfd,&sf->italicangle);
	} else if ( strmatch(tok,"UnderlinePosition:")==0 ) {
	    getdouble(sfd,&sf->upos);
	} else if ( strmatch(tok,"UnderlineWidth:")==0 ) {
	    getdouble(sfd,&sf->uwidth);
	} else if ( strmatch(tok,"PfmFamily:")==0 ) {
	    int temp;
	    getint(sfd,&temp);
	    sf->pfminfo.family = temp;
	    sf->pfminfo.pfmset = true;
	} else if ( strmatch(tok,"PfmWeight:")==0 ) {
	    int temp;
	    getint(sfd,&temp);
	    sf->pfminfo.weight = temp;
	    sf->pfminfo.pfmset = true;
	} else if ( strmatch(tok,"DisplaySize:")==0 ) {
	    getint(sfd,&sf->display_size);
	} else if ( strmatch(tok,"AntiAlias:")==0 ) {
	    int temp;
	    getint(sfd,&temp);
	    sf->display_antialias = temp;
	} else if ( strmatch(tok,"OnlyBitmaps:")==0 ) {
	    int temp;
	    getint(sfd,&temp);
	    sf->onlybitmaps = temp;
	} else if ( strmatch(tok,"Ascent:")==0 ) {
	    getint(sfd,&sf->ascent);
	} else if ( strmatch(tok,"Descent:")==0 ) {
	    getint(sfd,&sf->descent);
	} else if ( strmatch(tok,"XUID:")==0 ) {
	    geteol(sfd,tok);
	    sf->xuid = copy(tok);
	} else if ( strmatch(tok,"Encoding:")==0 ) {
	    if ( !getint(sfd,&sf->encoding_name) ) {
		Encoding *item;
		geteol(sfd,tok);
		for ( item = enclist; item!=NULL && strcmp(item->enc_name,tok)!=0; item = item->next );
		if ( item==NULL )
		    sf->encoding_name = em_none;
		else
		    sf->encoding_name = item->enc_num;
	    }
	} else if ( strmatch(tok,"Grid")==0 ) {
	    sf->gridsplines = SFDGetSplineSet(sfd);
	} else if ( strmatch(tok,"BeginPrivate:")==0 ) {
	    SFDGetPrivate(sfd,sf);
	} else if ( strmatch(tok,"BeginSubrs:")==0 ) {
	    SFDGetSubrs(sfd,sf);
	} else if ( strmatch(tok,"BeginChars:")==0 ) {
	    getint(sfd,&sf->charcnt);
	    GProgressChangeStages(2);
	    if ( getint(sfd,&realcnt)==1 )
		GProgressChangeTotal(realcnt);
	    else
		GProgressChangeTotal(sf->charcnt);
	    sf->chars = gcalloc(sf->charcnt,sizeof(SplineChar *));
	break;
	} else {
	    /* If we don't understand it, skip it */
	    geteol(sfd,tok);
	}
    }

    while ( (sc = SFDGetChar(sfd))!=NULL ) {
	if ( sc->enc<sf->charcnt ) {
	    sf->chars[sc->enc] = sc;
	    sc->parent = sf;
	}
	GProgressNext();
    }
    GProgressNextStage();
    GProgressChangeLine2(interpret);
    SFDFixupRefs(sf);
    while ( getname(sfd,tok)==1 ) {
	if ( strcmp(tok,"EndSplineFont")==0 )
    break;
	else if ( strcmp(tok,"BitmapFont:")==0 )
	    SFDGetBitmapFont(sfd,sf);
    }
return( sf );
}

SplineFont *SFDRead(char *filename) {
    FILE *sfd = fopen(filename,"r");
    SplineFont *sf;
    char *oldloc;

    if ( sfd==NULL )
return( NULL );
    oldloc = setlocale(LC_NUMERIC,"C");
    sf = SFDGetFont(sfd);
    setlocale(LC_NUMERIC,oldloc);
    if ( sf!=NULL )
	sf->filename = copy(filename);
    fclose(sfd);
return( sf );
}

static int ModSF(FILE *asfd,SplineFont *sf) {
    int newmap, cnt;
    char tok[200];
    int i;
    SplineChar *sc;

    if ( getname(asfd,tok)!=1 || strcmp(tok,"Encoding:")!=0 )
return(false);
    getint(asfd,&newmap);
    if ( sf->encoding_name!=newmap )
	SFReencodeFont(sf,newmap);
    if ( getname(asfd,tok)!=1 || strcmp(tok,"BeginChars:")!=0 )
return(false);
    SFRemoveDependencies(sf);

    getint(asfd,&cnt);
    if ( cnt>sf->charcnt ) {
	sf->chars = grealloc(sf->chars,cnt*sizeof(SplineChar *));
	for ( i=sf->charcnt; i<cnt; ++i )
	    sf->chars[i] = NULL;
    }
    while ( (sc = SFDGetChar(asfd))!=NULL ) {
	if ( sc->enc<sf->charcnt ) {
	    if ( sf->chars[sc->enc]!=NULL )
		SplineCharFree(sf->chars[sc->enc]);
	    sf->chars[sc->enc] = sc;
	    sc->parent = sf;
	    sc->changed = true;
	}
    }
    sf->changed = true;
    SFDFixupRefs(sf);
return(true);
}

static SplineFont *SlurpRecovery(FILE *asfd) {
    char tok[1025], *pt, ch;
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
	    if ( pt<tok+sizeof(tok)-2 )
		*pt++ = ch;
	*pt = '\0';
	sf = LoadSplineFont(tok);
    } else {
	sf = SplineFontNew();
	sf->onlybitmaps = false;
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

    if ( asfd==NULL )
return(NULL);
    oldloc = setlocale(LC_NUMERIC,"C");
    ret = SlurpRecovery(asfd);
    setlocale(LC_NUMERIC,oldloc);
    fclose(asfd);
    if ( ret )
	ret->autosavename = copy(autosavename);
return( ret );
}

void SFAutoSave(SplineFont *sf) {
    int i;
    FILE *asfd = fopen(sf->autosavename,"w");
    char *oldloc;

    if ( asfd==NULL )
return;
    oldloc = setlocale(LC_NUMERIC,"C");
    if ( sf->origname!=NULL )		/* might be a new file */
	fprintf( asfd, "Base: %s\n", sf->origname );
    fprintf( asfd, "Encoding: %d\n", sf->encoding_name );
    fprintf( asfd, "BeginChars: %d\n", sf->charcnt );
    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL && sf->chars[i]->changed )
	SFDDumpChar( asfd,sf->chars[i]);
    fprintf( asfd, "EndChars\n" );
    fprintf( asfd, "EndSplineFont\n" );
    fclose(asfd);
    setlocale(LC_NUMERIC,oldloc);
    sf->changed_since_autosave = false;
}

void SFClearAutoSave(SplineFont *sf) {
    sf->changed_since_autosave = false;
    if ( sf->autosavename==NULL )
return;
    unlink(sf->autosavename);
    free(sf->autosavename);
    sf->autosavename = NULL;
}
