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
#include "splinefont.h"
#include "gdraw.h"
#include "ustring.h"
#include "utype.h"
#include <unistd.h>
#include <locale.h>

static const char *charset_names[] = {
    "custom",
    "iso8859_1", "iso8859_2", "iso8859_3", "iso8859_4", "iso8859_5",
    "iso8859_6", "iso8859_7", "iso8859_8", "iso8859_9", "iso8859_10",
    "iso8859_11", "iso8859_13", "iso8859_14", "iso8859_15",
    "koi8_r",
    "jis201",
    "win", "mac", "symbol", "zapfding", "adobestandard",
    "jis208", "jis212", "ksc5601", "gb2312", "big5", "johab",
    "unicode", NULL};

static void SFDDumpSplineSet(FILE *sfd,SplineSet *spl) {
    SplinePoint *first, *sp;

    for ( ; spl!=NULL; spl=spl->next ) {
	first = NULL;
	for ( sp = spl->first; ; sp=sp->next->to ) {
	    if ( first==NULL )
		fprintf( sfd, "%g %g m ", sp->me.x, sp->me.y );
	    else if ( sp->prev->islinear )		/* Don't use known linear here. save control points if there are any */
		fprintf( sfd, " %g %g l ", sp->me.x, sp->me.y );
	    else
		fprintf( sfd, " %g %g %g %g %g %g c ",
			sp->prev->from->nextcp.x, sp->prev->from->nextcp.y,
			sp->prevcp.x, sp->prevcp.y,
			sp->me.x, sp->me.y );
	    fprintf(sfd, "%d\n", sp->pointtype|(sp->selected<<2)|
			(sp->nextcpdef<<3)|(sp->prevcpdef<<4)|
			(sp->roundx<<5)|(sp->roundy<<6) );
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
    for ( ss = sc->splines; ss!=NULL; ss=ss->next ) {
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

static void SFDDumpHintList(FILE *sfd,char *key, StemInfo *h) {
    HintInstance *hi;

    if ( h==NULL )
return;
    fprintf(sfd, "%s", key );
    for ( ; h!=NULL; h=h->next ) {
	fprintf(sfd, "%g %g", h->start,h->width );
	if ( h->ghost ) putc('G',sfd);
	if ( h->where!=NULL ) {
	    putc('<',sfd);
	    for ( hi=h->where; hi!=NULL; hi=hi->next )
		fprintf(sfd, "%g %g%c", hi->begin, hi->end, hi->next?' ':'>');
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
		h->leftedgetop.x, h->leftedgetop.y,
		h->rightedgetop.x, h->rightedgetop.y,
		h->leftedgebottom.x, h->leftedgebottom.y,
		h->rightedgebottom.x, h->rightedgebottom.y );
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
    if ( sc->vwidth!=sc->parent->ascent+sc->parent->descent )
	fprintf(sfd, "VWidth: %d\n", sc->vwidth );
    if ( sc->changedsincelasthinted|| sc->manualhints || sc->widthset )
	fprintf(sfd, "Flags: %s%s%s\n",
		sc->changedsincelasthinted?"H":"",
		sc->manualhints?"M":"",
		sc->widthset?"W":"");
#if HANYANG
    if ( sc->compositionunit )
	fprintf( sfd, "CompositionUnit: %d %d\n", sc->jamo, sc->varient );
#endif
    SFDDumpHintList(sfd,"HStem: ", sc->hstem);
    SFDDumpHintList(sfd,"VStem: ", sc->vstem);
    SFDDumpDHintList(sfd,"DStem: ", sc->dstem);
    if ( sc->splines!=NULL ) {
	fprintf(sfd, "Fore\n" );
	SFDDumpSplineSet(sfd,sc->splines);
	SFDDumpMinimumDistances(sfd,sc);
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
	fprintf( sfd, "Ligature: %s\n", sc->lig->components );
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

static void utf7_encode(FILE *sfd,long ch) {
static char base64[64] = {
 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

    putc(base64[(ch>>18)&0x3f],sfd);
    putc(base64[(ch>>12)&0x3f],sfd);
    putc(base64[(ch>>6)&0x3f],sfd);
    putc(base64[ch&0x3f],sfd);
}

static void SFDDumpUTF7Str(FILE *sfd, const unichar_t *str) {
    int ch, prev_cnt=0, prev=0, in=0;

    putc('"',sfd);
    if ( str!=NULL ) while ( (ch = *str++)!='\0' ) {
	if ( ch<127 && ch!='\n' && ch!='\r' && ch!='\\' && ch!='~' &&
		ch!='+' && ch!='=' && ch!='"' ) {
	    if ( prev_cnt!=0 ) {
		prev<<= (prev_cnt==1?16:8);
		utf7_encode(sfd,prev);
		prev_cnt=prev=0;
	    }
	    if ( in ) {
		if ( inbase64[ch]!=-1 )
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

static void SFDDumpLangName(FILE *sfd, struct ttflangname *ln) {
    int i, end;
    fprintf( sfd, "LangName: %d ", ln->lang );
    for ( end = ttf_namemax; end>0 && ln->names[end-1]==NULL; --end );
    for ( i=0; i<end; ++i )
	SFDDumpUTF7Str(sfd,ln->names[i]);
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

static void SFD_Dump(FILE *sfd,SplineFont *sf) {
    int i, realcnt;
    BDFFont *bdf;
    struct ttflangname *ln;

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
    fprintf(sfd, "ItalicAngle: %g\n", sf->italicangle );
    fprintf(sfd, "UnderlinePosition: %g\n", sf->upos );
    fprintf(sfd, "UnderlineWidth: %g\n", sf->uwidth );
    fprintf(sfd, "Ascent: %d\n", sf->ascent );
    fprintf(sfd, "Descent: %d\n", sf->descent );
    if ( sf->hasvmetrics )
	fprintf(sfd, "VerticalOrigin: %d\n", sf->vertical_origin );
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
	fprintf(sfd, "Panose:" );
	for ( i=0; i<10; ++i )
	    fprintf( sfd, " %d", sf->pfminfo.panose[i]);
	fprintf(sfd, "LineGap: %d\n", sf->pfminfo.linegap );
	fprintf(sfd, "VLineGap: %d\n", sf->pfminfo.vlinegap );
	putc('\n',sfd);
    }
    for ( ln = sf->names; ln!=NULL; ln=ln->next )
	SFDDumpLangName(sfd,ln);
    if ( sf->subfontcnt!=0 ) {
	/* CID fonts have no encodings, they have registry info instead */
	fprintf(sfd, "Registry: %s\n", sf->cidregistry );
	fprintf(sfd, "Ordering: %s\n", sf->ordering );
	fprintf(sfd, "Supplement: %d\n", sf->supplement );
	fprintf(sfd, "CIDVersion: %g\n", sf->cidversion );	/* This is a number whereas "version" is a string */
    } else if ( sf->encoding_name>=em_base ) {
	Encoding *item;
	for ( item = enclist; item!=NULL && item->enc_num!=sf->encoding_name; item = item->next );
	if ( item==NULL )
	    fprintf(sfd, "Encoding: custom\n" );
	else
	    fprintf(sfd, "Encoding: %s\n", item->enc_name );
    } else
	fprintf(sfd, "Encoding: %s\n", charset_names[sf->encoding_name+1] );
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
    if ( sf->private!=NULL )
	SFDDumpPrivate(sfd,sf->private);
#if HANYANG
    if ( sf->rules!=NULL )
	SFDDumpCompositionRules(sfd,sf->rules);
#endif

    if ( sf->subfontcnt!=0 ) {
	int max;
	for ( i=max=0; i<sf->subfontcnt; ++i )
	    if ( max<sf->subfonts[i]->charcnt )
		max = sf->subfonts[i]->charcnt;
	fprintf(sfd, "BeginSubFonts: %d %d\n", sf->subfontcnt, max );
	for ( i=0; i<sf->subfontcnt; ++i )
	    SFD_Dump(sfd,sf->subfonts[i]);
	fprintf(sfd, "EndSubFonts\n" );
    } else {
	if ( sf->cidmaster!=NULL )
	    realcnt = -1;
	else {
	    realcnt = 0;
	    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL )
		++realcnt;
	}
	fprintf(sfd, "BeginChars: %d %d\n", sf->charcnt, realcnt );
	for ( i=0; i<sf->charcnt; ++i ) {
	    if ( sf->chars[i]!=NULL )
		SFDDumpChar(sfd,sf->chars[i]);
	    GProgressNext();
	}
	fprintf(sfd, "EndChars\n" );
    }
	
    if ( sf->bitmaps!=NULL )
	GProgressChangeLine2R(_STR_SavingBitmaps);
    for ( bdf = sf->bitmaps; bdf!=NULL; bdf=bdf->next )
	SFDDumpBitmapFont(sfd,bdf);
    fprintf(sfd, sf->cidmaster==NULL?"EndSplineFont\n":"EndSubSplineFont\n" );
}

static void SFDDump(FILE *sfd,SplineFont *sf) {
    int i, realcnt;
    BDFFont *bdf;

    realcnt = sf->charcnt;
    if ( sf->subfontcnt!=0 ) {
	for ( i=0; i<sf->subfontcnt; ++i )
	    if ( realcnt<sf->subfonts[i]->charcnt )
		realcnt = sf->subfonts[i]->charcnt;
    }
    for ( i=0, bdf = sf->bitmaps; bdf!=NULL; bdf=bdf->next, ++i );
    GProgressStartIndicatorR(10,_STR_Saving,_STR_SavingDb,_STR_SavingOutlines,
	    realcnt,i+1);
    GProgressEnableStop(false);
    fprintf(sfd, "SplineFontDB: %.1f\n", 1.0 );
    SFD_Dump(sfd,sf);
    GProgressEndIndicator();
}

int SFDWrite(char *filename,SplineFont *sf) {
    FILE *sfd = fopen(filename,"w");
    char *oldloc;

    if ( sfd==NULL )
return( 0 );
    oldloc = setlocale(LC_NUMERIC,"C");
    if ( sf->cidmaster!=NULL )
	sf=sf->cidmaster;
    SFDDump(sfd,sf);
    setlocale(LC_NUMERIC,oldloc);
return( !ferror(sfd) && fclose(sfd)==0 );
}

int SFDWriteBak(SplineFont *sf) {
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
    rename(sf->filename,buf);
    free(buf);

return( SFDWrite(sf->filename,sf));
}

/* ********************************* INPUT ********************************** */

static char *getquotedeol(FILE *sfd) {
    char *pt, *str, *end;
    int ch;

    pt = str = galloc(100); end = str+100;
    while ( isspace(ch = getc(sfd)) && ch!='\r' && ch!='\n' );
    while ( ch!='\n' && ch!='\r' && ch!=EOF ) {
	if ( ch=='\\' ) {
	    ch = getc(sfd);
	    if ( ch=='n' ) ch='\n';
	}
	if ( pt>=end ) {
	    pt = grealloc(str,end-str+100);
	    end = pt+(end-str)+100;
	    str = pt;
	    pt = end-100;
	}
	*pt++ = ch;
	ch = getc(sfd);
    }
    *pt='\0';
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

static int getreal(FILE *sfd, real *val) {
    char tokbuf[100], ch;
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
    getreal(sfd,&img->xoff);
    getreal(sfd,&img->yoff);
    getreal(sfd,&img->xscale);
    getreal(sfd,&img->yscale);
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
    /* We've read the OrigType1 token (this is now obselete, but parse it in case there are any old sfds) */
    int len;
    struct enc85 dec;

    memset(&dec,'\0', sizeof(dec)); dec.pos = -1;
    dec.sfd = sfd;

    getint(sfd,&len);
    while ( --len >= 0 )
	Dec85(&dec);
}

static void SFDCloseCheck(SplinePointList *spl) {
    if ( spl->first!=spl->last &&
	    RealNear(spl->first->me.x,spl->last->me.x) &&
	    RealNear(spl->first->me.y,spl->last->me.y)) {
	SplinePoint *oldlast = spl->last;
	spl->first->prevcp = oldlast->prevcp;
	spl->first->noprevcp = false;
	oldlast->prev->from->next = NULL;
	spl->last = oldlast->prev->from;
	chunkfree(oldlast->prev,sizeof(*oldlast));
	chunkfree(oldlast,sizeof(*oldlast));
	SplineMake(spl->last,spl->first);
	spl->last = spl->first;
    }
}

static SplineSet *SFDGetSplineSet(FILE *sfd) {
    SplinePointList *cur=NULL, *head=NULL;
    BasePoint current;
    real stack[100];
    int sp=0;
    SplinePoint *pt;
    int ch;
    char tok[100];

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
		    pt = chunkalloc(sizeof(SplinePoint));
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
	    pt->roundx = val&0x20?1:0;
	    pt->roundy = val&0x40?1:0;
	}
    }
    if ( cur!=NULL )
	SFDCloseCheck(cur);
    getname(sfd,tok);
return( head );
}

static void SFDGetMinimumDistances(FILE *sfd, SplineChar *sc) {
    SplineSet *ss;
    SplinePoint *sp;
    int pt,i, val, err;
    int ch;
    SplinePoint **mapping=NULL;
    MinimumDistance *last, *md;

    for ( i=0; i<2; ++i ) {
	pt = 0;
	for ( ss = sc->splines; ss!=NULL; ss=ss->next ) {
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
	    mapping = gcalloc(pt,sizeof(SplineChar *));
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
	    fprintf( stderr, "Internal Error: Minimum Distance specifies bad point (%d) in sfd file\n", val );
	    err = true;
	} else if ( val!=-1 )
	    md->sp1 = mapping[val];
	ch = getc(sfd);
	if ( ch!=',' ) {
	    fprintf( stderr, "Internal Error: Minimum Distance lacks a comma where expected\n" );
	    err = true;
	}
	getint(sfd,&val);
	if ( val<-1 || val>=pt ) {
	    fprintf( stderr, "Internal Error: Minimum Distance specifies bad point (%d) in sfd file\n", val );
	    err = true;
	} else if ( val!=-1 )
	    md->sp2 = mapping[val];
	if ( !err ) {
	    if ( last==NULL )
		sc->md = md;
	    else
		last->next = md;
	    last = md;
	} else
	    chunkfree(md,sizeof(MinimumDistance));
    }
    free(mapping);
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
return( head );
}

static RefChar *SFDGetRef(FILE *sfd) {
    RefChar *rf;
    int enc=0, ch;

    rf = chunkalloc(sizeof(RefChar));
    getint(sfd,&enc);
    rf->local_enc = enc;
    while ( isspace(ch=getc(sfd)));
    if ( ch=='S' ) rf->selected = true;
    getreal(sfd,&rf->transform[0]);
    getreal(sfd,&rf->transform[1]);
    getreal(sfd,&rf->transform[2]);
    getreal(sfd,&rf->transform[3]);
    getreal(sfd,&rf->transform[4]);
    getreal(sfd,&rf->transform[5]);
return( rf );
}

static SplineChar *SFDGetChar(FILE *sfd,SplineFont *sf) {
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

    sc = chunkalloc(sizeof(SplineChar));
    sc->name = copy(tok);
    sc->vwidth = sf->ascent+sf->descent;
    while ( 1 ) {
	if ( getname(sfd,tok)!=1 )
return( NULL );
	if ( strmatch(tok,"Encoding:")==0 ) {
	    getint(sfd,&sc->enc);
	    getint(sfd,&sc->unicodeenc);
	} else if ( strmatch(tok,"Width:")==0 ) {
	    getsint(sfd,&sc->width);
	} else if ( strmatch(tok,"VWidth:")==0 ) {
	    getsint(sfd,&sc->vwidth);
	} else if ( strmatch(tok,"Flags:")==0 ) {
	    while ( isspace(ch=getc(sfd)) && ch!='\n' && ch!='\r');
	    while ( ch!='\n' && ch!='\r' ) {
		if ( ch=='H' ) sc->changedsincelasthinted=true;
		else if ( ch=='M' ) sc->manualhints = true;
		else if ( ch=='W' ) sc->widthset = true;
		ch = getc(sfd);
	    }
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
	} else if ( strmatch(tok,"Fore")==0 ) {
	    sc->splines = SFDGetSplineSet(sfd);
	} else if ( strmatch(tok,"MinimumDistance:")==0 ) {
	    SFDGetMinimumDistances(sfd,sc);
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
	    sc->lig->components = copy(tok);
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
    int pixelsize, ascent, descent;

    bdf = gcalloc(1,sizeof(BDFFont));
    bdf->encoding_name = sf->encoding_name;

    if ( getint(sfd,&pixelsize)!=1 || pixelsize<=0 )
return( 0 );
    if ( getint(sfd,&bdf->charcnt)!=1 || bdf->charcnt<0 )
return( 0 );
    if ( getint(sfd,&ascent)!=1 || ascent<0 )
return( 0 );
    if ( getint(sfd,&descent)!=1 || descent<0 )
return( 0 );
    bdf->pixelsize = pixelsize;
    bdf->ascent = ascent;
    bdf->descent = descent;

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

static unichar_t *SFDReadUTF7Str(FILE *sfd) {
    int ch1, ch2, ch3, ch4, done;
    unichar_t buffer[1024], *pt, *end = buffer+sizeof(buffer)/sizeof(unichar_t)-1;
    int prev_cnt=0, prev=0, in=0;

    ch1 = getc(sfd);
    while ( isspace(ch1) && ch1!='\n' && ch1!='\r') ch1 = getc(sfd);
    if ( ch1=='\n' || ch1=='\r' )
	ungetc(ch1,sfd);
    if ( ch1!='"' )
return( NULL );
    pt = buffer;
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
		ch2 = inbase64[getc(sfd)];
		ch3 = inbase64[getc(sfd)];
		ch4 = inbase64[getc(sfd)];
		ch1 = (ch1<<18) | (ch2<<12) | (ch3<<6) | ch4;
		if ( prev_cnt==0 ) {
		    prev = ch1&0xff;
		    ch1 >>= 8;
		    prev_cnt = 1;
		} else /* if ( prev_cnt == 1 ) */ {
		    ch1 |= (prev<<24);
		    prev = (ch1&0xffff);
		    ch1 >>= 16;
		    prev_cnt = 2;
		}
		done = true;
	    }
	}
	if ( done && pt<end )
	    *pt++ = ch1;
	if ( prev_cnt==2 ) {
	    prev_cnt = 0;
	    if ( pt<end && prev!=0 )
		*pt++ = prev;
	}
    }
    *pt = '\0';

return( buffer[0]=='\0' ? NULL : u_copy(buffer) );
}
    
static struct ttflangname *SFDGetLangName(FILE *sfd,struct ttflangname *old) {
    struct ttflangname *cur = gcalloc(1,sizeof(struct ttflangname)), *prev;
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

static SplineFont *SFD_GetFont(FILE *sfd,SplineFont *cidmaster,char *tok) {
    SplineFont *sf;
    SplineChar *sc;
    int realcnt, i, eof;

    sf = SplineFontEmpty();
    sf->cidmaster = cidmaster;
    while ( 1 ) {
	if ( (eof = getname(sfd,tok))!=1 ) {
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
	    getname(sfd,tok);
	    sf->familyname = copy(tok);
	} else if ( strmatch(tok,"Weight:")==0 ) {
	    getname(sfd,tok);
	    sf->weight = copy(tok);
	} else if ( strmatch(tok,"Copyright:")==0 ) {
	    sf->copyright = getquotedeol(sfd);
	} else if ( strmatch(tok,"Comments:")==0 ) {
	    sf->comments = getquotedeol(sfd);
	} else if ( strmatch(tok,"Version:")==0 ) {
	    geteol(sfd,tok);
	    sf->version = copy(tok);
	} else if ( strmatch(tok,"ItalicAngle:")==0 ) {
	    getreal(sfd,&sf->italicangle);
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
	    sf->pfminfo.pfmset = true;
	} else if ( strmatch(tok,"LineGap:")==0 ) {
	    getsint(sfd,&sf->pfminfo.linegap);
	    sf->pfminfo.pfmset = true;
	} else if ( strmatch(tok,"VLineGap:")==0 ) {
	    getsint(sfd,&sf->pfminfo.vlinegap);
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
	    if ( !getint(sfd,&sf->encoding_name) ) {
		Encoding *item;
		geteol(sfd,tok);
		sf->encoding_name = em_none;
		for ( i=0; charset_names[i]!=NULL; ++i )
		    if ( strcmp(tok,charset_names[i])==0 ) {
			sf->encoding_name = i-1;
		break;
		    }
		if ( charset_names[i]==NULL ) {
		    for ( item = enclist; item!=NULL && strcmp(item->enc_name,tok)!=0; item = item->next );
		    if ( item!=NULL )
			sf->encoding_name = item->enc_num;
		}
	    }
	} else if ( strmatch(tok,"Registry:")==0 ) {
	    geteol(sfd,tok);
	    sf->cidregistry = copy(tok);
	} else if ( strmatch(tok,"Ordering:")==0 ) {
	    geteol(sfd,tok);
	    sf->ordering = copy(tok);
	} else if ( strmatch(tok,"Supplement:")==0 ) {
	    getint(sfd,&sf->supplement);
	} else if ( strmatch(tok,"CIDVersion:")==0 ) {
	    real temp;
	    getreal(sfd,&temp);
	    sf->cidversion = temp;
	} else if ( strmatch(tok,"Grid")==0 ) {
	    sf->gridsplines = SFDGetSplineSet(sfd);
#if 0
	    SFFigureGrid(sf);
#endif
	} else if ( strmatch(tok,"BeginPrivate:")==0 ) {
	    SFDGetPrivate(sfd,sf);
	} else if ( strmatch(tok,"BeginSubrs:")==0 ) {	/* leave in so we don't croak on old sfd files */
	    SFDGetSubrs(sfd,sf);
	} else if ( strmatch(tok,"BeginSubFonts:")==0 ) {
	    getint(sfd,&sf->subfontcnt);
	    sf->subfonts = gcalloc(sf->subfontcnt,sizeof(SplineFont *));
	    getint(sfd,&realcnt);
	    GProgressChangeStages(2);
	    GProgressChangeTotal(realcnt);
    break;
	} else if ( strmatch(tok,"BeginChars:")==0 ) {
	    getint(sfd,&sf->charcnt);
	    if ( getint(sfd,&realcnt)!=1 ) {
		GProgressChangeStages(2);
		GProgressChangeTotal(sf->charcnt);
	    } else if ( realcnt!=-1 ) {
		GProgressChangeStages(2);
		GProgressChangeTotal(realcnt);
	    }
	    sf->chars = gcalloc(sf->charcnt,sizeof(SplineChar *));
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
	for ( i=0; i<sf->subfontcnt; ++i )
	    sf->subfonts[i] = SFD_GetFont(sfd,sf,tok);
    } else {
	while ( (sc = SFDGetChar(sfd,sf))!=NULL ) {
	    if ( sc->enc<sf->charcnt ) {
		sf->chars[sc->enc] = sc;
		sc->parent = sf;
	    }
	    GProgressNext();
	}
	if ( cidmaster==NULL ) {
	    GProgressNextStage();
	    GProgressChangeLine2R(_STR_InterpretingGlyphs);
	}
	SFDFixupRefs(sf);
    }
    while ( getname(sfd,tok)==1 ) {
	if ( strcmp(tok,"EndSplineFont")==0 || strcmp(tok,"EndSubSplineFont")==0 )
    break;
	else if ( strcmp(tok,"BitmapFont:")==0 )
	    SFDGetBitmapFont(sfd,sf);
    }
return( sf );
}

static SplineFont *SFDGetFont(FILE *sfd) {
    char tok[2000];
    real dval;
    int ch;

    if ( getname(sfd,tok)!=1 )
return( NULL );
    if ( strcmp(tok,"SplineFontDB:")!=0 )
return( NULL );
    if ( getreal(sfd,&dval)!=1 || (dval!=0 && dval!=1))
return( NULL );
    ch = getc(sfd); ungetc(ch,sfd);
    if ( ch!='\r' && ch!='\n' )
return( NULL );

return( SFD_GetFont(sfd,NULL,tok));
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
    int i,k;
    SplineChar *sc;
    SplineFont *ssf;

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
    while ( (sc = SFDGetChar(asfd,sf))!=NULL ) {
	ssf = sf;
	for ( k=0; k<sf->subfontcnt; ++k ) {
	    if ( sc->enc<sf->subfonts[k]->charcnt ) {
		ssf = sf->subfonts[k];
		if ( SCWorthOutputting(ssf->chars[sc->enc]))
	break;
	    }
	}
	if ( sc->enc<ssf->charcnt ) {
	    if ( ssf->chars[sc->enc]!=NULL )
		SplineCharFree(ssf->chars[sc->enc]);
	    ssf->chars[sc->enc] = sc;
	    sc->parent = ssf;
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
    int i, k, max;
    FILE *asfd;
    char *oldloc;
    SplineFont *ssf;

    if ( sf->cidmaster!=NULL ) sf=sf->cidmaster;
    asfd = fopen(sf->autosavename,"w");
    if ( asfd==NULL )
return;

    max = sf->charcnt;
    for ( i=0; i<sf->subfontcnt; ++i )
	if ( sf->subfonts[i]->charcnt>max ) max = sf->subfonts[i]->charcnt;

    oldloc = setlocale(LC_NUMERIC,"C");
    if ( sf->origname!=NULL )		/* might be a new file */
	fprintf( asfd, "Base: %s\n", sf->origname );
    fprintf( asfd, "Encoding: %d\n", sf->encoding_name );
    fprintf( asfd, "BeginChars: %d\n", max );
    for ( i=0; i<max; ++i ) {
	ssf = sf;
	for ( k=0; k<sf->subfontcnt; ++k ) {
	    if ( i<sf->subfonts[k]->charcnt ) {
		ssf = sf->subfonts[k];
		if ( SCWorthOutputting(ssf->chars[i]))
	break;
	    }
	}
	if ( ssf->chars[i]!=NULL && ssf->chars[i]->changed )
	    SFDDumpChar( asfd,ssf->chars[i]);
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
