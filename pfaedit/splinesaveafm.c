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
#include <stdio.h>
#include "splinefont.h"
#include <utype.h>

extern char *zapfnomen[];
extern short zapfwx[];
extern short zapfbb[][4];
extern char zapfexists[];

static void *mygets(FILE *file,char *buffer,int size) {
    char *end = buffer+size-1;
    char *pt = buffer;
    int ch;

    while ( (ch=getc(file))!=EOF && ch!='\r' && ch!='\n' && pt<end )
	*pt++ = ch;
    *pt = '\0';
    if ( ch==EOF && pt==buffer )
return( NULL );
    if ( ch=='\r' ) {
	ch = getc(file);
	if ( ch!='\n' )
	    ungetc(ch,file);
    }
return( buffer );
}

static SplineChar *SFFindName(SplineFont *sf,char *name) {
    int i;

    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL )
	if ( strcmp(sf->chars[i]->name,name)==0 )
return( sf->chars[i] );

return( NULL );
}

int LoadKerningDataFromAfm(SplineFont *sf, char *filename) {
    FILE *file = fopen(filename,"r");
    char buffer[200], *pt, *ept, ch;
    SplineChar *sc1, *sc2;
    KernPair *kp;
    int off;

    if ( file==NULL )
return( 0 );
    while ( mygets(file,buffer,sizeof(buffer))!=NULL ) {
	if ( strncmp(buffer,"KPX",3)==0 ) {
	    for ( pt=buffer+3; isspace(*pt); ++pt);
	    for ( ept = pt; *ept!='\0' && !isspace(*ept); ++ept );
	    ch = *ept; *ept = '\0';
	    sc1 = SFFindName(sf,pt);
	    *ept = ch;
	    for ( pt=ept; isspace(*pt); ++pt);
	    for ( ept = pt; *ept!='\0' && !isspace(*ept); ++ept );
	    ch = *ept; *ept = '\0';
	    sc2 = SFFindName(sf,pt);
	    *ept = ch;
	    off = strtol(ept,NULL,10);
	    if ( sc1!=NULL && sc2!=NULL ) {
		for ( kp=sc1->kerns; kp!=NULL && kp->sc!=sc2; kp = kp->next );
		if ( kp!=NULL )
		    kp->off = off;
		else if ( off!=0 ) {
		    kp = gcalloc(1,sizeof(KernPair));
		    kp->sc = sc2;
		    kp->off = off;
		    kp->next = sc1->kerns;
		    sc1->kerns = kp;
		}
	    }
	}
    }
    fclose(file);
return( 1 );
}

int CheckAfmOfPostscript(SplineFont *sf,char *psname) {
    char *new, *pt;
    int ret;

    new = galloc(strlen(psname)+5);
    strcpy(new,psname);
    pt = strrchr(new,'.');
    if ( pt==NULL ) pt = new+strlen(new);
    strcpy(pt,".afm");
    if ( !LoadKerningDataFromAfm(sf,new)) {
	strcpy(pt,".AFM");
	ret = LoadKerningDataFromAfm(sf,new);
    } else
	ret = true;
    free(new);
return( ret );
}

char *EncodingName(int map) {
    switch ( map ) {
      case em_adobestandard:
return( "AdobeStandardEncoding" );
      case em_iso8859_1:
return( "ISOLatin1Encoding" );
      case em_iso8859_2:
return( "ISO-8859-2" );
      case em_iso8859_3:
return( "ISO-8859-3" );
      case em_iso8859_4:
return( "ISO-8859-4" );
      case em_iso8859_5:
return( "ISO-8859-5" );
      case em_iso8859_6:
return( "ISO-8859-6" );
      case em_iso8859_7:
return( "ISO-8859-7" );
      case em_iso8859_8:
return( "ISO-8859-8" );
      case em_iso8859_9:
return( "ISO-8859-9" );
      case em_iso8859_10:
return( "ISO-8859-10" );
      case em_iso8859_11:
return( "ISO-8859-11" );
      case em_iso8859_13:
return( "ISO-8859-13" );
      case em_iso8859_14:
return( "ISO-8859-14" );
      case em_iso8859_15:
return( "ISO-8859-15" );
      case em_unicode:
return( "ISO-10646-1" );
      case em_mac:
return( "MacRoman" );
      case em_win:
return( "WinRoman" );
      case em_koi8_r:
return( "KOI8R" );
      case em_jis208:
return( "JISX208" );
      case em_jis212:
return( "JISX212" );
      case em_ksc5601:
return( "KSC5601" );
      case em_gb2312:
return( "GB2312" );
      default:
	if ( map>=em_base ) {
	    Encoding *item;
	    for ( item=enclist; item!=NULL && item->enc_num!=map; item=item->next );
	    if ( item!=NULL )
return( item->enc_name );
	}
return( "FontSpecific" );
    }
}

static void AfmLigOut(FILE *afm, SplineChar *sc) {
    LigList *ll;
    Ligature *lig;
    char *pt, *eos;

    for ( ll=sc->ligofme; ll!=NULL; ll=ll->next ) {
	lig = ll->lig;
	pt = strchr(lig->componants,' ');
	eos = strrchr(lig->componants,' ');
	if ( pt!=NULL && eos==pt )
	    /* AFM files don't seem to support 3 (or more) letter combos */
	    fprintf( afm, " L %s %s ;", pt+1, lig->lig->name );
    }
}

static void AfmSplineCharX(FILE *afm, SplineChar *sc, int enc) {
    DBounds b;

    SplineCharFindBounds(sc,&b);
    /*fprintf( afm, "CX <%04x> ; WX %d ; N %s ; B %d %d %d %d ;",*/
    fprintf( afm, "C %d ; WX %d ; N %s ; B %d %d %d %d ;\n",
	    enc, sc->width, sc->name,
	    (int) b.minx, (int) b.miny, (int) b.maxx, (int) b.maxy );
    if (sc->ligofme!=NULL)
	AfmLigOut(afm,sc);
    putc('\n',afm);
    GProgressNext();
}

static void AfmZapfCharX(FILE *afm, int zi) {

    /*fprintf( afm, "CX <%04x> ; WX %d ; N %s ; B %d %d %d %d ;\n",*/
    fprintf( afm, "C %d ; WX %d ; N %s ; B %d %d %d %d ;\n",
	    0x2700+zi, zapfwx[zi], zapfnomen[zi],
	    zapfbb[zi][0], zapfbb[zi][1], zapfbb[zi][2], zapfbb[zi][3]);
}

static void AfmSplineChar(FILE *afm, SplineChar *sc, int enc) {
    DBounds b;

    SplineCharFindBounds(sc,&b);
    fprintf( afm, "C %d ; WX %d ; N %s ; B %d %d %d %d ;",
	    enc, sc->width, sc->name,
	    (int) b.minx, (int) b.miny, (int) b.maxx, (int) b.maxy );
    if (sc->ligofme!=NULL)
	AfmLigOut(afm,sc);
    putc('\n',afm);
    GProgressNext();
}

static int anykerns(SplineFont *sf) {
    int i, cnt = 0;
    KernPair *kp;

    for ( i=0; i<sf->charcnt; ++i ) {
	if ( sf->chars[i]!=NULL && strcmp(sf->chars[i]->name,".notdef")!=0 ) {
	    for ( kp = sf->chars[i]->kerns; kp!=NULL; kp = kp->next )
		if ( kp->off!=0 && strcmp(kp->sc->name,".notdef")!=0 )
		    ++cnt;
	}
    }
return( cnt );
}

static void AfmKernPairs(FILE *afm, SplineChar *sc) {
    KernPair *kp;

    for ( kp = sc->kerns; kp!=NULL; kp=kp->next ) {
	if ( strcmp(kp->sc->name,".notdef")!=0 && kp->off!=0 )
	    fprintf( afm, "KPX %s %s %d\n", sc->name, kp->sc->name, kp->off );
    }
}

int SCWorthOutputting(SplineChar *sc) {
return( sc!=NULL && 
	( sc->splines!=NULL || sc->refs!=NULL || sc->widthset ||
	    sc->width!=sc->parent->ascent+sc->parent->descent ) &&
	( strcmp(sc->name,".notdef")!=0 || sc->enc==0) );
}

int AfmSplineFont(FILE *afm, SplineFont *sf, int type0) {
    DBounds b;
    double width;
    int i, cnt;
    int caph, xh, ash, dsh;
    int encmax=!type0?256:sf->encoding_name==em_unicode?65536:94*94;
    int anyzapf;

    SFLigaturePrepare(sf);

    fprintf( afm, "StartFontMetrics 2.0\n" );
    fprintf( afm, "Comment Generated by pfaedit\n" );
    fprintf( afm, "FontName %s\n", sf->fontname );
    if ( sf->fullname!=NULL ) fprintf( afm, "FullName %s\n", sf->fullname );
    if ( sf->familyname!=NULL ) fprintf( afm, "FamilyName %s\n", sf->familyname );
    if ( sf->weight!=NULL ) fprintf( afm, "Weight %s\n", sf->weight );
    if ( sf->copyright!=NULL ) fprintf( afm, "Notice (%s)\n", sf->copyright );
    fprintf( afm, "ItalicAngle %g\n", sf->italicangle );
    width = -1;
    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL && strcmp(sf->chars[i]->name,".notdef")!=0 ) {
	if ( width==-1 ) width = sf->chars[i]->width;
	else if ( width!=sf->chars[i]->width ) {
	    width = -2;
    break;
	}
    }
    fprintf( afm, "IsFixedPitch %s\n", width==-2?"false":"true" );
    fprintf( afm, "UnderlinePosition %g\n", sf->upos );
    fprintf( afm, "UnderlineThickness %g\n", sf->uwidth );
    if ( sf->version!=NULL ) fprintf( afm, "Version %s\n", sf->version );
    fprintf( afm, "EncodingScheme %s\n", EncodingName(sf->encoding_name));
    SplineFontFindBounds(sf,&b);
    fprintf( afm, "FontBBox %d %d %d %d\n", (int) b.minx, (int) b.miny,
	    (int) b.maxx, (int) b.maxy );

    caph = xh = ash = dsh = 0; cnt = 0;
    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	if ( sf->chars[i]->unicodeenc=='I' || sf->chars[i]->unicodeenc=='x' ||
		sf->chars[i]->unicodeenc=='p' || sf->chars[i]->unicodeenc=='l' ) {
	    SplineCharFindBounds(sf->chars[i],&b);
	    if ( sf->chars[i]->unicodeenc=='I' )
		caph = b.maxy;
	    else if ( sf->chars[i]->unicodeenc=='x' )
		xh = b.maxy;
	    else if ( sf->chars[i]->unicodeenc=='l' )
		ash = b.maxy;
	    else
		dsh = b.miny;
	}
	if ( SCWorthOutputting(sf->chars[i]) ) 
	    ++cnt;
    }
    if ( caph!=0 )
	fprintf( afm, "CapHeight %d\n", caph );
    if ( xh!=0 )
	fprintf( afm, "XHeight %d\n", xh );
    if ( ash!=0 )
	fprintf( afm, "Ascender %d\n", ash );
    if ( dsh!=0 )
	fprintf( afm, "Descender %d\n", dsh );

    anyzapf = false;
    if ( type0 && sf->encoding_name==em_unicode ) {
	for ( i=0x2700; i<sf->charcnt && i<encmax && i<=0x27ff; ++i )
	    if ( SCWorthOutputting(sf->chars[i]) ) 
		anyzapf = true;
	if ( !anyzapf )
	    for ( i=0; i<0xc0; ++i )
		if ( zapfexists[i])
		    ++cnt;
    }

    fprintf( afm, "StartCharMetrics %d\n", cnt );
    if ( type0 && sf->encoding_name>=em_jis208 && sf->encoding_name<=em_gb2312 ) {
	int enc;
	for ( i=0; i<sf->charcnt && i<encmax; ++i )
	    if ( SCWorthOutputting(sf->chars[i]) ) {
		enc = (i/96 + '!')*256 + (i%96 + ' ');
		AfmSplineCharX(afm,sf->chars[i],enc);
	    }
    } else if ( type0 ) {
	for ( i=0; i<sf->charcnt && i<encmax && i<0x2700; ++i )
	    if ( SCWorthOutputting(sf->chars[i]) ) {
		AfmSplineCharX(afm,sf->chars[i],i);
	    }
	if ( !anyzapf ) {
	    for ( i=0; i<0xc0; ++i ) if ( zapfexists[i] )
		AfmZapfCharX(afm,i);
	    i += 0x2700;
	}
	for ( ; i<sf->charcnt && i<encmax; ++i )
	    if ( SCWorthOutputting(sf->chars[i]) ) {
		AfmSplineCharX(afm,sf->chars[i],i);
	    }
    } else {
	for ( i=0; i<sf->charcnt && i<encmax; ++i )
	    if ( SCWorthOutputting(sf->chars[i]) ) {
		AfmSplineChar(afm,sf->chars[i],i);
	    }
    }
    for ( ; i<sf->charcnt ; ++i )
	if ( SCWorthOutputting(sf->chars[i]) ) {
	    AfmSplineChar(afm,sf->chars[i],-1);
	}
    fprintf( afm, "EndCharMetrics\n" );
    if ( (cnt = anykerns(sf))>0 ) {
	fprintf( afm, "StartKernData\n" );
	fprintf( afm, "StartKernPairs %d\n", cnt );
	for ( i=0; i<sf->charcnt ; ++i )
	    if ( SCWorthOutputting(sf->chars[i]) ) {
		AfmKernPairs(afm,sf->chars[i]);
	    }
	fprintf( afm, "EndKernPairs\n" );
	fprintf( afm, "EndKernData\n" );
    }
    fprintf( afm, "EndFontMetrics\n" );

    SFLigatureCleanup(sf);

return( !ferror(afm));
}

void SFLigatureCleanup(SplineFont *sf) {
    LigList *l, *next;
    int j;

    for ( j=0; j<sf->charcnt; ++j ) if ( sf->chars[j]!=NULL ) {
	for ( l = sf->chars[j]->ligofme; l!=NULL; l = next ) {
	    next = l->next;
	    free( l );
	}
	sf->chars[j]->ligofme = NULL;
    }
}

void SFLigaturePrepare(SplineFont *sf) {
    Ligature *lig;
    LigList *ll;
    int i,j,ch,sch;
    char *pt, *semi, *ligstart;

    /* First clear out any old stuff */
    for ( j=0; j<sf->charcnt; ++j ) if ( sf->chars[j]!=NULL )
	sf->chars[j]->ligofme = NULL;

    /* Attach all the ligatures to the first character of their componants */
    for ( i=0 ; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL && sf->chars[i]->lig!=NULL ) {
	lig = sf->chars[i]->lig;
	ligstart = lig->componants;
	while ( *ligstart!='\0' ) {
	    semi = strchr(ligstart,';');
	    if ( semi==NULL ) semi = ligstart+strlen(ligstart);
	    sch = *semi; *semi = '\0';
	    for ( pt=ligstart; *pt!='\0' && *pt!=' '; ++pt );
	    ch = *pt; *pt = '\0';
	    for ( j=0; j<sf->charcnt; ++j )
		if ( sf->chars[j]!=NULL && strcmp(sf->chars[j]->name,ligstart)==0 )
	    break;
	    if ( j<sf->charcnt ) {
		ll = galloc(sizeof(LigList));
		ll->lig = lig;
		ll->next = sf->chars[j]->ligofme;
		sf->chars[j]->ligofme = ll;
	    }
	    *pt = ch;
	    *semi = sch;
	    if ( sch=='\0' )
	break;
	    for ( ligstart = semi+1; isspace(*ligstart); ++ligstart );
	}
    }
}
