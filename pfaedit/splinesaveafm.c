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
#include <stdio.h>
#include "splinefont.h"
#include <utype.h>
#include <ustring.h>
#include <time.h>
#include <math.h>

extern char *zapfnomen[];
extern short zapfwx[];
extern short zapfbb[][4];
extern char zapfexists[];

int lig_script = CHR('*',' ',' ',' ');
int lig_lang = DEFAULT_LANG;
int *lig_tags=NULL;

#define SPECIAL_TEMPORARY_LIGATURE_TAG	1
    /* This is a special tag value which I use internally. It is not a valid */
    /*  feature tag (not 4 ascii characters) so there should be no confusion */
    /* When generating an afm file and I'm given the ligatures "ffi->f f i" */
    /*  "ff->f f" then generate the third ligature "ffi->ff i" on a temporary */
    /*  basis. AFM files can't deal with three character ligatures */

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

static void KPInsert( SplineChar *sc1, SplineChar *sc2, int off ) {
    KernPair *kp;

    if ( sc1!=NULL && sc2!=NULL ) {
	for ( kp=sc1->kerns; kp!=NULL && kp->sc!=sc2; kp = kp->next );
	if ( kp!=NULL )
	    kp->off = off;
	else if ( off!=0 ) {
	    kp = chunkalloc(sizeof(KernPair));
	    kp->sc = sc2;
	    kp->off = off;
	    kp->sli = SFAddScriptLangIndex(sc1->parent,
			    SCScriptFromUnicode(sc1),DEFAULT_LANG);
	    kp->next = sc1->kerns;
	    sc1->kerns = kp;
	}
    }
}

int LoadKerningDataFromAfm(SplineFont *sf, char *filename) {
    FILE *file = fopen(filename,"r");
    char buffer[200], *pt, *ept, ch;
    SplineChar *sc1, *sc2;
    int off;
    char name[44], second[44], lig[44];
    PST *liga;

    if ( file==NULL )
return( 0 );
    GProgressChangeLine2R(_STR_ReadingAFM);
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
	    KPInsert(sc1,sc2,off);
	} else if ( sscanf( buffer, "C %*d ; WX %*d ; N %.40s ; B %*d %*d %*d %*d ; L %.40s %.40s",
		name, second, lig)==3 ) {
	    sc1 = SFFindName(sf,lig);
	    sc2 = SFFindName(sf,name);
	    if ( sc2==NULL )
		sc2 = SFFindName(sf,second);
	    if ( sc1!=NULL ) {
		sprintf( buffer, "%s %s", name, second);
		for ( liga=sc1->possub; liga!=NULL; liga=liga->next ) {
		    if ( liga->type == pst_ligature && strcmp(liga->u.lig.components,buffer)==0 )
		break;
		}
		if ( liga==NULL ) {
		    liga = chunkalloc(sizeof(PST));
		    liga->tag = CHR('l','i','g','a');
		    liga->script_lang_index = SFAddScriptLangIndex(sf,
			    SCScriptFromUnicode(sc2),DEFAULT_LANG);
		    liga->type = pst_ligature;
		    liga->next = sc1->possub;
		    sc1->possub = liga;
		    liga->u.lig.lig = sc1;
		    liga->u.lig.components = copy( buffer );
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
    int wasuc=false;

    new = galloc(strlen(psname)+5);
    strcpy(new,psname);
    pt = strrchr(new,'.');
    if ( pt==NULL ) pt = new+strlen(new);
    else wasuc = isupper(pt[1]);
    strcpy(pt,wasuc?".AFM":".afm");
    if ( !LoadKerningDataFromAfm(sf,new)) {
	strcpy(pt,wasuc?".afm":".AFM");
	ret = LoadKerningDataFromAfm(sf,new);
    } else
	ret = true;
    free(new);
return( ret );
}

static void tfmDoLigKern(SplineFont *sf, int enc, int lk_index,
	uint8 *ligkerntab, uint8 *kerntab) {
    int enc2, k_index;
    SplineChar *sc1, *sc2;
    real off;

    if ( enc>=sf->charcnt || (sc1=sf->chars[enc])==NULL )
return;

    while ( 1 ) {
	enc2 = ligkerntab[lk_index*4+1];
	if ( enc2<sf->charcnt && (sc2=sf->chars[enc2])!=NULL &&
		ligkerntab[lk_index*4+2]>=128 ) {
	    k_index = ((ligkerntab[lk_index*4+2]-128)*256+ligkerntab[lk_index*4+3])*4;
	    off = (sf->ascent+sf->descent) *
		    ((kerntab[k_index]<<24) + (kerntab[k_index+1]<<16) +
			(kerntab[k_index+2]<<8) + kerntab[k_index+3])/
		    (double) 0x100000;
 /* printf( "%s(%d) %s(%d) -> %g\n", sc1->name, sc1->enc, sc2->name, sc2->enc, off); */
	    KPInsert(sc1,sc2,off);
	}
	if ( ligkerntab[lk_index*4]>=128 )
    break;
	lk_index += ligkerntab[lk_index*4] + 1;
    }
}

int LoadKerningDataFromTfm(SplineFont *sf, char *filename) {
    FILE *file = fopen(filename,"r");
    int i, tag, left;
    uint8 *ligkerntab, *kerntab;
    int head_len, first, last, width_size, height_size, depth_size, italic_size,
	    ligkern_size, kern_size;

    if ( file==NULL )
return( 0 );
    /* file length = */ getushort(file);
    head_len = getushort(file);
    first = getushort(file);
    last = getushort(file);
    width_size = getushort(file);
    height_size = getushort(file);
    depth_size = getushort(file);
    italic_size = getushort(file);
    ligkern_size = getushort(file);
    kern_size = getushort(file);
    /* extensible char tab size = getushort(file); */
    /* font param size = getushort(file); */
    if ( first-1>last || last>=256 ) {
	fclose(file);
return( 0 );
    }
    if ( ligkern_size==0 || kern_size==0 ) {
	fclose(file);
return( 1 );	/* we successfully read no data */
    }
    kerntab = gcalloc(kern_size,sizeof(int32));
    ligkerntab = gcalloc(ligkern_size,sizeof(int32));
    fseek( file,
	    (6+head_len+(last-first+1)+width_size+height_size+depth_size+italic_size)*sizeof(int32),
	    SEEK_SET);
    fread( ligkerntab,1,ligkern_size*sizeof(int32),file);
    fread( kerntab,1,kern_size*sizeof(int32),file);

    fseek( file, (6+head_len)*sizeof(int32), SEEK_SET);
    for ( i=first; i<=last; ++i ) {
	/* width = */ getc(file);
	/* height<<4 | depth indeces = */ getc( file );
	tag = (getc(file)&3);
	left = getc(file);
	if ( tag==1 )
	    tfmDoLigKern(sf,i,left,ligkerntab,kerntab);
    }

    free( ligkerntab); free(kerntab);
    fclose(file);
return( 1 );
}

char *EncodingName(int map) {

    if ( map>=em_unicodeplanes && map<=em_unicodeplanesmax ) {
	static char space[40];
	/* What is the proper encoding for a font consisting of SMP? */
	/* I'm guessing at ISO10646-2, but who knows */
	sprintf( space, "ISO10646-%d", map-em_unicodeplanes+1 );
return( space );
    }

    switch ( map ) {
      case em_adobestandard:
return( "AdobeStandardEncoding" );
      case em_iso8859_1:
return( "ISOLatin1Encoding" );
      case em_iso8859_2:
return( "ISO8859-2" );
      case em_iso8859_3:
return( "ISO8859-3" );
      case em_iso8859_4:
return( "ISO8859-4" );
      case em_iso8859_5:
return( "ISO8859-5" );
      case em_iso8859_6:
return( "ISO8859-6" );
      case em_iso8859_7:
return( "ISO8859-7" );
      case em_iso8859_8:
return( "ISO8859-8" );
      case em_iso8859_9:
return( "ISO8859-9" );
      case em_iso8859_10:
return( "ISO8859-10" );
      case em_iso8859_11:
return( "ISO8859-11" );
      case em_iso8859_13:
return( "ISO8859-13" );
      case em_iso8859_14:
return( "ISO8859-14" );
      case em_iso8859_15:
return( "ISO8859-15" );
      case em_unicode: case em_unicode4:
return( "ISO10646-1" );
      case em_mac:
return( "MacRoman" );
      case em_win:
return( "WinRoman" );
      case em_koi8_r:
return( "KOI8R" );
      case em_jis208: case em_sjis:
return( "JISX208" );
      case em_jis212:
return( "JISX212" );
      case em_ksc5601: case em_wansung:
return( "KSC5601" );
      case em_gb2312:
return( "GB2312" );
      case em_big5:
return( "BIG5" );
      case em_big5hkscs:
return( "BIG5HKSCS" );
      case em_johab:
return( "Johab" );
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

static int ScriptLangMatchLigOuts(PST *lig,SplineFont *sf) {
    int i,j;
    struct script_record *sr;

    if ( lig_tags==NULL ) {
	lig_tags = gcalloc(3,sizeof(uint32));
	lig_tags[0] = CHR('l','i','g','a');
	lig_tags[1] = CHR('r','l','i','g');
	lig_tags[2] = 0;
    }

    if ( lig->tag==SPECIAL_TEMPORARY_LIGATURE_TAG )	/* Special hack for the temporary 2 character intermediate ligatures we generate. We've already decided to output them */
return( true );

    for ( i=0; lig_tags[i]!=0 && lig_tags[i]!=lig->tag; ++i );
    if ( lig_tags[i]==0 )
return( false );

    if ( sf->cidmaster!=NULL ) sf=sf->cidmaster;

    if ( sf->script_lang==NULL ) {
	GDrawIError("How can there be ligatures with no script/lang list?");
return( lig_script==CHR('*',' ',' ',' ') && lig_lang==CHR('*',' ',' ',' ') );
    }

    sr = sf->script_lang[lig->script_lang_index];
    for ( i=0; sr[i].script!=0; ++i ) {
	if ( sr[i].script==lig_script || lig_script==CHR('*',' ',' ',' ')) {
	    for ( j=0; sr[i].langs[j]!=0; ++j ) {
		if ( sr[i].langs[j]==lig_lang || lig_lang==CHR('*',' ',' ',' '))
return( true );
	    }
	    if ( lig_script!=CHR('*',' ',' ',' ') )
return( false );
	}
    }
return( false );
}

static void AfmLigOut(FILE *afm, SplineChar *sc) {
    LigList *ll;
    PST *lig;
    char *pt, *eos;

    for ( ll=sc->ligofme; ll!=NULL; ll=ll->next ) {
	lig = ll->lig;
	if ( !ScriptLangMatchLigOuts(lig,sc->parent))
    continue;
	pt = strchr(lig->u.lig.components,' ');
	eos = strrchr(lig->u.lig.components,' ');
	if ( pt!=NULL && eos==pt )
	    /* AFM files don't seem to support 3 (or more) letter combos */
	    fprintf( afm, " L %s %s ;", pt+1, lig->u.lig.lig->name );
    }
}

static void AfmSplineCharX(FILE *afm, SplineChar *sc, int enc) {
    DBounds b;
    int em = (sc->parent->ascent+sc->parent->descent);

    sc = SCDuplicate(sc);

    SplineCharFindBounds(sc,&b);
    /*fprintf( afm, "CX <%04x> ; WX %d ; N %s ; B %d %d %d %d ;",*/
    fprintf( afm, "C %d ; WX %d ; N %s ; B %d %d %d %d ;",
	    enc, sc->width*1000/em, sc->name,
	    (int) floor(b.minx*1000/em), (int) floor(b.miny*1000/em),
	    (int) ceil(b.maxx*1000/em), (int) ceil(b.maxy*1000/em) );
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
    int em = (sc->parent->ascent+sc->parent->descent);

    sc = SCDuplicate(sc);

    SplineCharFindBounds(sc,&b);
    fprintf( afm, "C %d ; WX %d ; N %s ; B %d %d %d %d ;",
	    enc, sc->width*1000/em, sc->name,
	    (int) floor(b.minx*1000/em), (int) floor(b.miny*1000/em),
	    (int) ceil(b.maxx*1000/em), (int) ceil(b.maxy*1000/em) );
    if (sc->ligofme!=NULL)
	AfmLigOut(afm,sc);
    putc('\n',afm);
    GProgressNext();
}

static void AfmCIDChar(FILE *afm, SplineChar *sc, int enc) {
    DBounds b;
    int em = (sc->parent->ascent+sc->parent->descent);

    SplineCharFindBounds(sc,&b);
    fprintf( afm, "C -1 ; WOX %d ; N %d ; B %d %d %d %d ;",
	    sc->width*1000/em, enc,
	    (int) floor(b.minx*1000/em), (int) floor(b.miny*1000/em),
	    (int) ceil(b.maxx*1000/em), (int) ceil(b.maxy*1000/em) );
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
    int em = (sc->parent->ascent+sc->parent->descent);

    if ( strcmp(sc->name,".notdef")==0 )
return;

    for ( kp = sc->kerns; kp!=NULL; kp=kp->next ) {
	if ( strcmp(kp->sc->name,".notdef")!=0 && kp->off!=0 )
	    fprintf( afm, "KPX %s %s %d\n", sc->name, kp->sc->name, kp->off*1000/em );
    }
}

/* Look for character duplicates, such as might be generated by having the same */
/*  glyph at two encoding slots */
/* I think it is ok if something depends on this character, because the */
/*  code that handles references will automatically unwrap it down to be base */
SplineChar *SCDuplicate(SplineChar *sc) {
    SplineChar *matched = sc;

    if ( sc==NULL || sc->parent->cidmaster!=NULL )
return( sc );		/* Can't do this in CID keyed fonts */

    while ( sc->refs!=NULL && sc->refs->next==NULL && 
	    sc->refs->transform[0]==1 && sc->refs->transform[1]==0 &&
	    sc->refs->transform[2]==0 && sc->refs->transform[3]==1 &&
	    sc->refs->transform[4]==0 && sc->refs->transform[5]==0 ) {
	char *basename = sc->refs->sc->name;
	int len = strlen(basename);
	if ( strncmp(sc->name,basename,len)==0 &&
		(sc->name[len]=='.' || sc->name[len]=='\0'))
	    matched = sc->refs->sc;
	sc = sc->refs->sc;
    }
return( matched );
}

int SCIsNotdef(SplineChar *sc,int fixed) {
return( sc!=NULL && sc->enc==0 && sc->refs==NULL && strcmp(sc->name,".notdef")==0 &&
	(sc->splines!=NULL || (sc->widthset && fixed==-1) ||
	 (sc->width!=sc->parent->ascent+sc->parent->descent && fixed==-1 ) ||
	 (sc->width==fixed && fixed!=-1 && sc->widthset)));
}

int SCWorthOutputting(SplineChar *sc) {
return( sc!=NULL &&
	( sc->splines!=NULL || sc->refs!=NULL || sc->widthset || sc->anchor!=NULL ||
#if HANYANG
	    sc->compositionunit ||
#endif
	    sc->dependents!=NULL ||
	    sc->width!=sc->parent->ascent+sc->parent->descent ) &&
	( strcmp(sc->name,".notdef")!=0 || sc->enc==0) &&
	( (strcmp(sc->name,".null")!=0 && strcmp(sc->name,"glyph1")!=0 &&
	   strcmp(sc->name,"nonmarkingreturn")!=0 && strcmp(sc->name,"glyph2")!=0) ||
	  sc->splines!=NULL ) );
}

int CIDWorthOutputting(SplineFont *cidmaster, int enc) {
    int i;

    if ( enc<0 )
return( -1 );

    if ( cidmaster->subfontcnt==0 )
return( enc>=cidmaster->charcnt?-1:SCWorthOutputting(cidmaster->chars[enc])?0:-1 );

    for ( i=0; i<cidmaster->subfontcnt; ++i )
	if ( enc<cidmaster->subfonts[i]->charcnt &&
		SCWorthOutputting(cidmaster->subfonts[i]->chars[enc]))
return( i );

return( -1 );
}

static void LigatureClosure(SplineFont *sf);

int AfmSplineFont(FILE *afm, SplineFont *sf, int formattype) {
    DBounds b;
    real width;
    int i, j, cnt, max;
    int caph, xh, ash, dsh;
    int type0 = ( formattype==ff_ptype0 );
    int encmax=!type0?256:sf->encoding_name<em_big5?94*94:65536;
    int anyzapf;
    int iscid = ( formattype==ff_cid || formattype==ff_otfcid );
    time_t now;
    SplineChar *sc;
    int em = (sf->ascent+sf->descent);

    SFLigaturePrepare(sf);
    LigatureClosure(sf);		/* Convert 3 character ligs to a set of two character ones when possible */
    SFKernPrepare(sf);			/* Undoes kern classes */

    if ( iscid && sf->cidmaster!=NULL ) sf = sf->cidmaster;

    max = sf->charcnt;
    if ( iscid ) {
	max = 0;
	for ( i=0; i<sf->subfontcnt; ++i )
	    if ( sf->subfonts[i]->charcnt>max )
		max = sf->subfonts[i]->charcnt;
    }
    caph = xh = ash = dsh = 0; cnt = 0;
    for ( i=0; i<max; ++i ) {
	sc = NULL;
	if ( iscid ) {
	    for ( j=0; j<sf->subfontcnt; ++j )
		if ( i<sf->subfonts[j]->charcnt && sf->subfonts[j]->chars[i]!=NULL ) {
		    sc = sf->subfonts[j]->chars[i];
	    break;
		}
	} else
	    sc = sf->chars[i];
	if ( sc!=NULL ) {
	    if ( sc->unicodeenc=='I' || sc->unicodeenc=='x' ||
		    sc->unicodeenc=='p' || sc->unicodeenc=='l' ||
		    sc->unicodeenc==0x399 || sc->unicodeenc==0x3c7 ||
		    sc->unicodeenc==0x3c1 ||
		    sc->unicodeenc==0x406 || sc->unicodeenc==0x445 ||
		    sc->unicodeenc==0x440 ) {
		SplineCharFindBounds(sc,&b);
		if ( sc->unicodeenc=='I' )
		    caph = b.maxy;
		else if ( caph==0 && (sc->unicodeenc==0x399 || sc->unicodeenc==0x406))
		    caph = b.maxy;
		else if ( sc->unicodeenc=='x' )
		    xh = b.maxy;
		else if ( xh==0 && (sc->unicodeenc==0x3c7 || sc->unicodeenc==0x445))
		    xh = b.maxy;
		else if ( sc->unicodeenc=='l' )	/* can't find a good equivalent in greek/cyrillic */
		    ash = b.maxy;
		else if ( sc->unicodeenc=='p' )
		    dsh = b.miny;
		else if ( dsh==0 && (sc->unicodeenc==0x3c1 || sc->unicodeenc==0x440))
		    dsh = b.miny;
	    }
	    if ( SCWorthOutputting(sc) || (iscid && i==0 && sc!=NULL)) 
		++cnt;
	}
    }

    fprintf( afm, iscid ? "StartFontMetrics 4.1\n":"StartFontMetrics 2.0\n" );
    fprintf( afm, "Comment Generated by pfaedit\n" );
    time(&now);
    fprintf(afm,"Comment Creation Date: %s", ctime(&now));
    fprintf( afm, "FontName %s\n", sf->fontname );
    if ( sf->fullname!=NULL ) fprintf( afm, "FullName %s\n", sf->fullname );
    if ( sf->familyname!=NULL ) fprintf( afm, "FamilyName %s\n", sf->familyname );
    if ( sf->weight!=NULL ) fprintf( afm, "Weight %s\n", sf->weight );
    /* AFM lines are limitted to 256 characters and US ASCII */
    if ( sf->copyright!=NULL ) {
	char *pt, *start, *p;
	for ( pt=start=sf->copyright; *pt && pt-start<200 && *pt!='\n'; ++pt );
	fprintf( afm, "Notice (" );
	for ( p=start; p<pt; ++p )
	    if ( *p=='\xa9' ) fprintf(afm,"(C)");
	    else if ( *p=='\t' || (*p>=' ' && *p<0x7f))
		putc(*p,afm);
	fprintf( afm, ")\n" );
	while ( *pt ) {
	    start = pt;
	    if ( *start=='\n' ) ++start;
	    for ( pt=start; *pt && pt-start<200 && *pt!='\n'; ++pt );
	    fprintf( afm, "Comment " );
	    for ( p=start; p<pt; ++p )
		if ( *p=='\xa9' ) fprintf(afm,"(C)");
		else if ( *p=='\t' || (*p>=' ' && *p<0x7f))
		    putc(*p,afm);
	    fprintf( afm, "\n" );
	}
    }
    if ( iscid ) {
	fprintf( afm, "Characters %d\n", cnt );
	fprintf( afm, "Version %g\n", sf->cidversion );
	fprintf( afm, "CharacterSet %s-%s-%d\n", sf->cidregistry, sf->ordering, sf->supplement );
	fprintf( afm, "IsBaseFont true\n" );
	fprintf( afm, "IsCIDFont true\n" );
    }
    fprintf( afm, "ItalicAngle %g\n", sf->italicangle );
    width = CIDOneWidth(sf);
    fprintf( afm, "IsFixedPitch %s\n", width==-1?"false":"true" );
    fprintf( afm, "UnderlinePosition %g\n", sf->upos );
    fprintf( afm, "UnderlineThickness %g\n", sf->uwidth );
    if ( !iscid ) {
	if ( sf->version!=NULL ) fprintf( afm, "Version %s\n", sf->version );
	fprintf( afm, "EncodingScheme %s\n", EncodingName(sf->encoding_name));
    }
    if ( iscid ) CIDFindBounds(sf,&b); else SplineFontFindBounds(sf,&b);
    fprintf( afm, "FontBBox %d %d %d %d\n",
	    (int) floor(b.minx*1000/em), (int) floor(b.miny*1000/em),
	    (int) ceil(b.maxx*1000/em), (int) ceil(b.maxy*1000/em) );
    if ( caph!=0 )
	fprintf( afm, "CapHeight %d\n", caph*1000/em );
    if ( xh!=0 )
	fprintf( afm, "XHeight %d\n", xh*1000/em );
    if ( ash!=0 )
	fprintf( afm, "Ascender %d\n", ash*1000/em );
    if ( dsh!=0 )
	fprintf( afm, "Descender %d\n", dsh*1000/em );

    anyzapf = false;
    if ( type0 && (sf->encoding_name==em_unicode ||
	    sf->encoding_name==em_unicode4)) {
	for ( i=0x2700; i<sf->charcnt && i<encmax && i<=0x27ff; ++i )
	    if ( SCWorthOutputting(sf->chars[i]) ) 
		anyzapf = true;
	if ( !anyzapf )
	    for ( i=0; i<0xc0; ++i )
		if ( zapfexists[i])
		    ++cnt;
    }

    fprintf( afm, "StartCharMetrics %d\n", cnt );
    if ( iscid ) {
	for ( i=0; i<max; ++i ) {
	    sc = NULL;
	    for ( j=0; j<sf->subfontcnt; ++j )
		if ( i<sf->subfonts[j]->charcnt && sf->subfonts[j]->chars[i]!=NULL ) {
		    sc = sf->subfonts[j]->chars[i];
	    break;
		}
	    if ( SCWorthOutputting(sc) || (i==0 && sc!=NULL) )
		AfmCIDChar(afm, sc, i);
	}
    } else if ( type0 && sf->encoding_name>=em_jis208 && sf->encoding_name<=em_gb2312 ) {
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
    SFKernCleanup(sf);

return( !ferror(afm));
}

void SFLigatureCleanup(SplineFont *sf) {
    LigList *l, *next;
    struct splinecharlist *scl, *sclnext;
    int j;

    for ( j=0; j<sf->charcnt; ++j ) if ( sf->chars[j]!=NULL ) {
	for ( l = sf->chars[j]->ligofme; l!=NULL; l = next ) {
	    next = l->next;
	    for ( scl = l->components; scl!=NULL; scl = sclnext ) {
		sclnext = scl->next;
		chunkfree(scl,sizeof(struct splinecharlist));
	    }
	    if ( l->lig->tag==SPECIAL_TEMPORARY_LIGATURE_TAG ) {
		free(l->lig->u.lig.components);
		chunkfree(l->lig,sizeof(PST));
	    }
	    free( l );
	}
	sf->chars[j]->ligofme = NULL;
    }
}

void SFLigaturePrepare(SplineFont *sf) {
    PST *lig;
    LigList *ll;
    int i,j,k,ch;
    char *pt, *ligstart;
    SplineChar *sc, *tsc;
    struct splinecharlist *head, *last;
    int ccnt, lcnt, lmax=20;
    LigList **all = galloc(lmax*sizeof(LigList *));

    /* First clear out any old stuff */
    for ( j=0; j<sf->charcnt; ++j ) if ( sf->chars[j]!=NULL )
	sf->chars[j]->ligofme = NULL;

    /* Attach all the ligatures to the first character of their components */
    /* Figure out what the components are, and if they all exist */
    /* we're only interested in the lig if all components are worth outputting */
    for ( i=0 ; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL && sf->chars[i]->possub!=NULL ) {
	for ( lig = sf->chars[i]->possub; lig!=NULL; lig=lig->next ) if ( lig->type==pst_ligature ) {
	    ligstart = lig->u.lig.components;
	    last = head = NULL; sc = NULL;
	    for ( pt = ligstart; *pt!='\0'; ) {
		char *start = pt;
		for ( ; *pt!='\0' && *pt!=' '; ++pt );
		ch = *pt; *pt = '\0';
		tsc = SFGetCharDup(sf,-1,start);
		*pt = ch;
		if ( tsc!=NULL ) {
		    if ( !SCWorthOutputting(tsc)) {
			sc = NULL;
	    break;
		    }
		    if ( sc==NULL ) {
			sc = tsc;
			ccnt = 1;
		    } else {
			struct splinecharlist *cur = chunkalloc(sizeof(struct splinecharlist));
			if ( head==NULL )
			    head = cur;
			else
			    last->next = cur;
			last = cur;
			cur->sc = tsc;
			cur->next = NULL;
			++ccnt;
		    }
		} else {
		    sc = NULL;
	    break;
		}
		while ( *pt==' ' ) ++pt;
	    }
	    if ( sc!=NULL ) {
		ll = galloc(sizeof(LigList));
		ll->lig = lig;
		ll->next = sc->ligofme;
		ll->first = sc;
		ll->components = head;
		ll->ccnt = ccnt;
		sc->ligofme = ll;
	    } else {
		while ( head!=NULL ) {
		    last = head->next;
		    free(head);
		    head = last;
		}
	    }
	}
    }
    for ( i=0 ; i<sf->charcnt; ++i ) if ( (sc=sf->chars[i])!=NULL && sc->ligofme!=NULL ) {
	for ( ll=sc->ligofme, lcnt=0; ll!=NULL; ll=ll->next, ++lcnt );
	/* Finally, order the list so that the longest ligatures are first */
	if ( lcnt>1 ) {
	    if ( lcnt>=lmax )
		all = grealloc(all,(lmax=lcnt+30)*sizeof(LigList *));
	    for ( ll=sc->ligofme, k=0; ll!=NULL; ll=ll->next, ++k )
		all[k] = ll;
	    for ( k=0; k<lcnt-1; ++k ) for ( j=k+1; j<lcnt; ++j )
		if ( all[k]->ccnt<all[j]->ccnt ) {
		    ll = all[k];
		    all[k] = all[j];
		    all[j] = ll;
		}
	    sc->ligofme = all[0];
	    for ( k=0; k<lcnt-1; ++k )
		all[k]->next = all[k+1];
	    all[k]->next = NULL;
	}
    }
    free( all );
}

static void LigatureClosure(SplineFont *sf) {
    /* AFM files can only deal with 2 character ligatures */
    /* Look for any three character ligatures (like ffi) which can be made up */
    /*  of a two character ligature and their final letter (like "ff" and "i")*/
    /* I'm not going to bother with 4 character ligatures which could be made */
    /*  of 2+1+1 because that doesn't happen in latin that I know of, and arabic */
    /*  really should be using a type2 font */
    int i;
    LigList *l, *l2, *l3;
    PST *lig;
    SplineChar *sc, *sublig;

    for ( i=0; i<sf->charcnt; ++i ) if ( (sc=sf->chars[i])!=NULL ) {
	for ( l=sc->ligofme; l!=NULL; l=l->next ) if ( ScriptLangMatchLigOuts(l->lig,sf)) {
	    if ( l->ccnt==3 ) {
		/* we've got a 3 character ligature */
		for ( l2=l->next; l2!=NULL; l2=l2->next ) {
		    if ( l2->lig->tag==l->lig->tag && l2->lig->script_lang_index==l->lig->script_lang_index &&
			    l2->ccnt == 2 && l2->components->sc==l->components->sc ) {
			/* We've found a two character sub ligature */
			sublig = l2->lig->u.lig.lig;
			for ( l3=sublig->ligofme; l3!=NULL; l3=l3->next ) {
			    if ( l3->ccnt==2 && l3->components->sc==l->components->next->sc &&
				    ScriptLangMatchLigOuts(l3->lig,sf))
			break;
			}
			if ( l3!=NULL )	/* The ligature we want to add already exists */
		break;
			lig = chunkalloc(sizeof(PST));
			*lig = *l->lig;
			lig->tag = SPECIAL_TEMPORARY_LIGATURE_TAG;
			lig->next = NULL;
			lig->u.lig.components = galloc(strlen(sublig->name)+
					strlen(l->components->next->sc->name)+
					2);
			sprintf(lig->u.lig.components,"%s %s",sublig->name,
				l->components->next->sc->name);
			l3 = galloc(sizeof(LigList));
			l3->lig = lig;
			l3->next = sublig->ligofme;
			l3->first = sublig;
			l3->ccnt = 2;
			sublig->ligofme = l3;
			l3->components = chunkalloc(sizeof(struct splinecharlist));
			l3->components->sc = l->components->next->sc;
		break;
		    }
		}
	    }
	}
    }
}

void SFKernCleanup(SplineFont *sf) {
    int i;
    KernPair *kp, *p, *n;

    if ( sf->kerns==NULL )	/* can't have gotten messed up */
return;
    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	for ( kp = sf->chars[i]->kerns, p=NULL; kp!=NULL; kp = n ) {
	    n = kp->next;
	    if ( kp->kcid!=0 ) {
		if ( p==NULL )
		    sf->chars[i]->kerns = n;
		else
		    p->next = n;
		chunkfree(kp,sizeof(*kp));
	    } else
		p = kp;
	}
    }
}

static void KCSfree(SplineChar ***scs,int cnt) {
    int i;
    for ( i=1; i<cnt; ++i )
	free( scs[i]);
    free(scs);
}

static SplineChar ***KernClassToSC(SplineFont *sf, char **classnames, int cnt) {
    SplineChar ***scs, *sc;
    int i,j;
    char *pt, *end, ch;

    scs = galloc(cnt*sizeof(SplineChar **));
    for ( i=1; i<cnt; ++i ) {
	for ( pt=classnames[i]-1, j=0; pt!=NULL; pt=strchr(pt+1,' ') )
	    ++j;
	scs[i] = galloc((j+1)*sizeof(SplineChar *));
	for ( pt=classnames[i], j=0; *pt!='\0'; pt=end+1 ) {
	    end = strchr(pt,' ');
	    if ( end==NULL )
		end = pt+strlen(pt);
	    ch = *end;
	    *end = '\0';
	    sc = SFGetCharDup(sf,-1,pt);
	    if ( sc!=NULL )
		scs[i][j++] = sc;
	    if ( ch=='\0' )
	break;
	    *end = ch;
	}
	scs[i][j] = NULL;
    }
return( scs );
}

static void AddTempKP(SplineChar *first,SplineChar *second,
	int16 offset, uint16 sli, uint16 flags,uint16 kcid) {
    KernPair *kp;

    for ( kp=first->kerns; kp!=NULL; kp=kp->next )
	if ( kp->sc == second )
    break;
    if ( kp==NULL ) {
	kp = chunkalloc(sizeof(KernPair));
	kp->sc = second;
	kp->off = offset;
	kp->sli = sli;
	kp->flags = flags;
	kp->kcid = kcid;
	kp->next = first->kerns;
	first->kerns = kp;
    }
}

void SFKernPrepare(SplineFont *sf) {
    KernClass *kc;
    SplineChar ***first, ***last;
    int i, j, k, l;

    for ( kc = sf->kerns, i=0; kc!=NULL; kc = kc->next )
	kc->kcid = ++i;
    for ( kc = sf->kerns; kc!=NULL; kc = kc->next ) {
	first = KernClassToSC(sf,kc->firsts,kc->first_cnt);
	last = KernClassToSC(sf,kc->seconds,kc->second_cnt);
	for ( i=1; i<kc->first_cnt; ++i ) for ( j=1; j<kc->second_cnt; ++j ) {
	    if ( kc->offsets[i*kc->second_cnt+j]!=0 ) {
		for ( k=0; first[i][k]!=NULL; ++k )
		    for ( l=0; last[j][l]!=NULL; ++l )
			AddTempKP(first[i][k],last[j][l],
				kc->offsets[i*kc->second_cnt+j],
			        kc->sli,kc->flags, kc->kcid);
	    }
	}
	KCSfree(first,kc->first_cnt);
	KCSfree(last,kc->second_cnt);
    }
}

static void putlshort(short val,FILE *pfm) {
    putc(val&0xff,pfm);
    putc(val>>8,pfm);
}

static void putlint(int val,FILE *pfm) {
    putc(val&0xff,pfm);
    putc((val>>8)&0xff,pfm);
    putc((val>>16)&0xff,pfm);
    putc((val>>24)&0xff,pfm);
}

int PfmSplineFont(FILE *pfm, SplineFont *sf, int type0) {
    int caph, xh, ash, dsh, cnt=0, first=-1, samewid=-1, maxwid= -1, last=0, wid=0;
    int kerncnt=0, spacepos=0x20;
    int i;
    char *pt;
    KernPair *kp;
    /* my docs imply that pfm files can only handle 1byte fonts */
    long size, devname, facename, extmetrics, exttable, driverinfo, kernpairs, pos;
    DBounds b;
    int style;

    SFKernPrepare(sf);
    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	if ( sf->chars[i]!=NULL && sf->chars[i]->unicodeenc==' ' )
	    spacepos = i;
	if ( SCWorthOutputting(sf->chars[i]) ) {
	    ++cnt;
	    if ( sf->chars[i]->unicodeenc=='I' || sf->chars[i]->unicodeenc=='x' ||
		    sf->chars[i]->unicodeenc=='H' || sf->chars[i]->unicodeenc=='d' || 
		    sf->chars[i]->unicodeenc=='p' || sf->chars[i]->unicodeenc=='l' ) {
		SplineCharFindBounds(sf->chars[i],&b);
		if ( sf->chars[i]->unicodeenc=='I' || sf->chars[i]->unicodeenc=='H' )
		    caph = b.maxy;
		else if ( sf->chars[i]->unicodeenc=='x' )
		    xh = b.maxy;
		else if ( sf->chars[i]->unicodeenc=='d' || sf->chars[i]->unicodeenc=='l' )
		    ash = b.maxy;
		else
		    dsh = b.miny;
	    }
	    if ( samewid==-1 )
		samewid = sf->chars[i]->width;
	    else if ( samewid!=sf->chars[i]->width )
		samewid = -2;
	    if ( maxwid<sf->chars[i]->width )
		maxwid = sf->chars[i]->width;
	    if ( first==-1 )
		first = i;
	    wid += sf->chars[i]->width;
	    last = i;
	    if ( i<256 ) for ( kp=sf->chars[i]->kerns; kp!=NULL; kp = kp->next )
		if ( kp->sc->enc<256 )
		    ++kerncnt;
	}
    }
    if ( spacepos<first ) first = spacepos;
    if ( first==-1 ) first = 0;
    if ( spacepos>last ) last = spacepos;
    if ( cnt!=0 ) wid /= cnt;
    if ( kerncnt>=512 ) kerncnt = 512;

    SFDefaultOS2Info(&sf->pfminfo,sf,sf->fontname);

    putlshort(0x100,pfm);		/* format version number */
    size = ftell(pfm);
    putlint(-1,pfm);			/* file size, fill in later */
    i=0;
    if ( sf->copyright!=NULL ) for ( pt=sf->copyright; *pt && i<60; ++i, ++pt )
	putc(*pt,pfm);
    for ( ; i<60; ++i )
	putc(' ',pfm);
    putlshort(0x81,pfm);		/* type flags */
    putlshort(10,pfm);			/* point size, not really meaningful */
    putlshort(300,pfm);			/* vert resolution */
    putlshort(300,pfm);			/* hor resolution */
    putlshort(sf->ascent,pfm);		/* ascent (is this the right defn of ascent?) */
    if ( caph==0 ) caph = ash;
    putlshort(sf->ascent+sf->descent-caph-dsh,pfm);	/* Internal leading */
    putlshort(0/*(sf->ascent+sf->descent)/8*/,pfm);	/* External leading */
    style = MacStyleCode(sf,NULL);
    putc(style&sf_italic?1:0,pfm);	/* is italic */
    putc(0,pfm);			/* underline */
    putc(0,pfm);			/* strikeout */
    putlshort(sf->pfminfo.weight,pfm);	/* weight */
    putc(sf->encoding_name==em_symbol?2:0,pfm);	/* charset. I'm always saying windows roman (ANSI) or symbol because I don't know the other choices */
    putlshort(/*samewid<0?sf->ascent+sf->descent:samewid*/0,pfm);	/* width */
    putlshort(sf->ascent+sf->descent,pfm);	/* height */
    putc(sf->pfminfo.pfmfamily,pfm);	/* family */
    putlshort(wid,pfm);			/* average width, Docs say "Width of "X", but that's wrong */
    putlshort(maxwid,pfm);		/* max width */

    if ( first>255 ) first=last = 0;
    else if ( last>255 ) last = 255;
    putc(first,pfm);			/* first char */
    putc(last,pfm);
    if ( spacepos>=first && spacepos<=last ) {
	putc(spacepos-first,pfm);	/* default char */
	putc(spacepos-first,pfm);	/* wordbreak */
    } else {
	putc(0,pfm);			/* default character. I set to space */
	putc(0,pfm);			/* word break character. I set to space */
    }
    putlshort(0,pfm);			/* width bytes. Not meaningful for ps */

    devname = ftell(pfm);
    putlint(-1,pfm);			/* offset to device name, fill in later */
    facename = ftell(pfm);
    putlint(-1,pfm);			/* offset to face name, fill in later */
    putlint(0,pfm);			/* bits pointer. not used */
    putlint(0,pfm);			/* bits offset. not used */

/* No width table */

/* extensions */
    putlshort(0x1e,pfm);		/* size of extensions table */
    extmetrics = ftell(pfm);
    putlint(-1,pfm);			/* extent metrics. fill in later */
    exttable = ftell(pfm);
    putlint(-1,pfm);			/* extent table. fill in later */
    putlint(0,pfm);			/* origin table. not used */
    kernpairs = ftell(pfm);
    putlint(kerncnt==0?0:-1,pfm);	/* kern pairs. fill in later */
    putlint(0,pfm);			/* kern track. I don't understand it so I'm leaving it out */
    driverinfo = ftell(pfm);
    putlint(-1,pfm);			/* driver info. fill in later */
    putlint(0,pfm);			/* reserved. mbz */

/* devicename */
    pos = ftell(pfm);
    fseek(pfm,devname,SEEK_SET);
    putlint(pos,pfm);
    fseek(pfm,pos,SEEK_SET);
    for ( pt="Postscript"/*"\273PostScript\253"*/; *pt; ++pt )
	putc(*pt,pfm);
    putc('\0',pfm);

/* facename */
    pos = ftell(pfm);
    fseek(pfm,facename,SEEK_SET);
    putlint(pos,pfm);
    fseek(pfm,pos,SEEK_SET);
    if ( sf->familyname!=NULL ) {
	for ( pt=sf->familyname; *pt; ++pt )
	    putc(*pt,pfm);
    } else {
	for ( pt=sf->fontname; *pt; ++pt )
	    putc(*pt,pfm);
    }
    putc('\0',pfm);

/* extmetrics */
    pos = ftell(pfm);
    fseek(pfm,extmetrics,SEEK_SET);
    putlint(pos,pfm);
    fseek(pfm,pos,SEEK_SET);
    putlshort(0x34,pfm);		/* size */
    putlshort(240,pfm);			/* 12 point in twentieths of a point */
    putlshort(0,pfm);			/* any orientation */
    putlshort(sf->ascent+sf->descent,pfm);	/* master height */
    putlshort(3,pfm);			/* min scale */
    putlshort(1000,pfm);		/* max scale */
    putlshort(sf->ascent+sf->descent,pfm);	/* master units */
    putlshort(caph,pfm);		/* cap height */
    putlshort(xh,pfm);			/* x height */
    putlshort(ash,pfm);			/* lower case ascent height */
    putlshort(-dsh,pfm);		/* lower case descent height */
    putlshort((int) (10*sf->italicangle),pfm);	/* italic angle */
    putlshort(-xh,pfm);			/* super script */
    putlshort(xh/2,pfm);		/* sub script */
    putlshort(2*(sf->ascent+sf->descent)/3,pfm);	/* super size */
    putlshort(2*(sf->ascent+sf->descent)/3,pfm);	/* sub size */
    putlshort(-sf->upos,pfm);		/* underline pos */
    putlshort(sf->uwidth,pfm);		/* underline width */
    putlshort(-sf->upos,pfm);		/* real underline pos */
    putlshort(-sf->upos+2*sf->uwidth,pfm);	/* real underline second line pos */
    putlshort(sf->uwidth,pfm);		/* underline width */
    putlshort(sf->uwidth,pfm);		/* underline width */
    putlshort((xh+sf->uwidth)/2,pfm);	/* strike out top */
    putlshort(sf->uwidth,pfm);		/* strike out width */
    putlshort(kerncnt,pfm);		/* number of kerning pairs <= 512 */
    putlshort(0,pfm);			/* kerning tracks <= 16 */

/* extent table */
    pos = ftell(pfm);
    fseek(pfm,exttable,SEEK_SET);
    putlint(pos,pfm);
    fseek(pfm,pos,SEEK_SET);
    for ( i=first; i<=last; ++i )
	if ( sf->chars[i]==NULL )
	    putlshort(0,pfm);
	else
	    putlshort(sf->chars[i]->width,pfm);

/* driver info */ /* == ps font name */
    pos = ftell(pfm);
    fseek(pfm,driverinfo,SEEK_SET);
    putlint(pos,pfm);
    fseek(pfm,pos,SEEK_SET);
    for ( pt=sf->fontname; *pt; ++pt )
	putc(*pt,pfm);
    putc('\0',pfm);

/* kern pairs */
    if ( kerncnt!=0 ) {
	pos = ftell(pfm);
	fseek(pfm,kernpairs,SEEK_SET);
	putlint(pos,pfm);
	fseek(pfm,pos,SEEK_SET);
	for ( i=first; i<last; ++i ) if ( sf->chars[i]!=NULL ) {
	    if ( SCWorthOutputting(sf->chars[i]) ) {
		for ( kp=sf->chars[i]->kerns; kp!=NULL; kp = kp->next )
		    if ( kp->sc->enc<256 ) {
			putc(i,pfm);
			putc(kp->sc->enc,pfm);
			putlshort(kp->off,pfm);
		    }
	    }
	}
    }

/* kern track */
    /* I'm ignoring this */

/* file size */
    pos = ftell(pfm);
    fseek(pfm,size,SEEK_SET);
    putlint(pos,pfm);

    SFKernCleanup(sf);

return( !ferror(pfm));
}

typedef uint32 fix_word;

struct tfm_header {
    uint32 checksum;	/* don't know how to calculate this, use 0 to ignore it */
    fix_word design_size;	/* in points (10<<20 seems to be default) */
    char encoding[40];	/* first byte is length, rest are a string that names the encoding */
	/* ASCII, TeX text, TeX math extension, XEROX, GRAPHIC, UNSPECIFIED */
    char family[20];	/* Font Family, preceded by a length byte */
    uint8 seven_bit_safe_flag;
    uint8 ignored[2];
    uint8 face; 	/* 0=>roman, 1=>italic */
    			/* 4=>light, 0=>medium, 2=>bold */
			/* 6=>condensed, 0=>regular, 12=>extended */
};

struct tfm_params {
    fix_word slant;	/* -sin(italic_angle) (a small positive number) */
    fix_word space;	/* inter-word spacing (advance width of space) */
    fix_word space_stretch;	/* inter-word glue stretching */
	/* About 1/2 space for cmr */
    fix_word space_shrink;	/* inter-word glue shrinking */
	/* About 1/3 space for cmr */
    fix_word x_height;
    fix_word quad;	/* == 1.0 */
    fix_word extra_space;	/* added at end of sentences */
	/* same as space_shrink for cmr */
/* tex math and tex math extension have extra parameters. They are not */
/*  explained (page 7) */
};
struct ligkern {
    uint8 skip;
    uint8 other_char;
    uint8 op;
    uint8 remainder;
    struct ligkern *next;
};

static struct ligkern *TfmAddKern(KernPair *kp,struct ligkern *last,double *kerns,
	int *_kcnt) {
    struct ligkern *new = gcalloc(1,sizeof(struct ligkern));
    int i;

    new->other_char = kp->sc->enc;
    for ( i=*_kcnt-1; i>=0 ; --i )
	if ( kerns[i] == kp->off )
    break;
    if ( i<0 ) {
	i = (*_kcnt)++;
	kerns[i] = kp->off;
    }
    new->remainder = i&0xff;
    new->op = 128 + (i>>8);
    new->next = last;
return( new );
}

static struct ligkern *TfmAddLiga(LigList *l,struct ligkern *last) {
    struct ligkern *new;

    if ( l->lig->u.lig.lig->enc>=256 )
return( last );
    if ( l->components==NULL || l->components->sc->enc>=256 || l->components->next!=NULL )
return( last );
    new = gcalloc(1,sizeof(struct ligkern));
    new->other_char = l->components->sc->enc;
    new->remainder = l->lig->u.lig.lig->enc;
    new->next = last;
    new->op = 0*4 + 0*2 + 0;
	/* delete next char, delete current char, start over and check the resultant ligature for more ligs */
return( new );
}

static void FindCharlists(SplineFont *sf,int *charlistindex) {
    PST *pst;
    int i, last, ch;
    char *pt, *end;
    SplineChar *sc;

    memset(charlistindex,-1,257*sizeof(int));
    for ( i=0; i<256 && i<sf->charcnt; ++i ) if ( SCWorthOutputting(sf->chars[i])) {
	for ( pst=sf->chars[i]->possub; pst!=NULL; pst=pst->next )
	    if ( pst->type == pst_alternate && pst->tag==CHR('T','C','H','L'))
	break;
	if ( pst!=NULL ) {
	    last = i;
	    for ( pt=pst->u.alt.components; *pt; pt = end ) {
		end = strchr(pt,' ');
		if ( end==NULL )
		    end = pt+strlen(pt);
		ch = *end;
		*end = '\0';
		sc = SFGetCharDup(sf,-1,pt);
		*end = ch;
		while ( *end==' ' ) ++end;
		if ( sc!=NULL && sc->enc<256 ) {
		    charlistindex[last] = sc->enc;
		    last = sc->enc;
		}
	    }
	}
    }
}

static int FindExtensions(SplineFont *sf,int *extensions,int *extenindex) {
    PST *pst;
    int i, ecnt=0, ch;
    char *pt, *end;
    int founds[4], fcnt;
    SplineChar *sc;

    memset(extenindex,-1,257*sizeof(int));
    for ( i=0; i<256 && i<sf->charcnt; ++i ) if ( SCWorthOutputting(sf->chars[i])) {
	for ( pst=sf->chars[i]->possub; pst!=NULL; pst=pst->next )
	    if ( pst->type == pst_multiple && pst->tag==CHR('T','E','X','L'))
	break;
	if ( pst!=NULL ) {
	    fcnt = 0;
	    founds[0] = founds[1] = founds[2] = founds[3] = 0;
	    for ( pt=pst->u.alt.components; *pt; pt = end ) {
		end = strchr(pt,' ');
		if ( end==NULL )
		    end = pt+strlen(pt);
		ch = *end;
		*end = '\0';
		sc = SFGetCharDup(sf,-1,pt);
		*end = ch;
		while ( *end==' ' ) ++end;
		if ( sc!=NULL && sc->enc<256 ) {
		    if ( fcnt<4 )
			founds[fcnt++] = sc->enc;
		} else
		    founds[fcnt++] = 0;
	    }
	    if ( fcnt==1 ) {
		founds[3] = founds[0];
		founds[0] = 0;
	    }
	    if ( fcnt>0 ) {
		extenindex[i] = ecnt;
		extensions[ecnt++] = (founds[0]<<24) |
			(founds[1]<<16) |
			(founds[2]<<8) |
			founds[3];
	    }
	}
    }
return( ecnt );
}

static int CoalesceValues(double *values,int max,int *index) {
    int i,j,k,top, offpos,diff;
    int backindex[257];
    double off, test;
    double topvalues[257], totvalues[257];
    int cnt[257];

    values[256] = 0;
    for ( i=0; i<257; ++i )
	backindex[i] = i;

    /* sort */
    for ( i=0; i<256; ++i ) for ( j=i+1; j<257; ++j ) {
	if ( values[i]>values[j] ) {
	    int l = backindex[i];
	    double val = values[i];
	    backindex[i] = backindex[j];
	    values[i] = values[j];
	    backindex[j] = l;
	    values[j] = val;
	}
    }
    for ( i=0; i<257; ++i )
	index[backindex[i]] = i;
    top = 257;
    for ( i=0; i<top; ++i ) {
	for ( j=i+1; j<257 && values[i]==values[j]; ++j );
	if ( j>i+1 ) {
	    int diff = j-i-1;
	    for ( k=i+1; k+diff<257; ++k )
		values[k] = values[k+diff];
	    for ( k=0; k<257; ++k ) {
		if ( index[k]>=j )
		    index[k] -= diff;
		else if ( index[k]>i )
		    index[k] = i;
	    }
	    top -= diff;
	}
	cnt[i] = j-i;
    }
    if ( top<=max )
return( top );

    for ( i=0; i<top; ++i ) {
	topvalues[i] = values[i];
	totvalues[i] = cnt[i]*values[i];
    }

    while ( top>max ) {
	off = fabs(topvalues[0]-values[1]);
	offpos = 0;
	for ( i=1; i<top-1; ++i ) {
	    test = fabs(topvalues[i]-values[i+1]);
	    if ( test<off ) {
		off = test;
		offpos = i;
	    }
	}
	topvalues[offpos] = topvalues[offpos+1];
	cnt[offpos] += cnt[offpos+1];
	totvalues[offpos] += totvalues[offpos+1];
	diff = 1;
	for ( k=offpos+1; k+diff<256; ++k ) {
	    values[k] = values[k+diff];
	    topvalues[k] = topvalues[k+diff];
	    cnt[k] = cnt[k+diff];
	    totvalues[k] = totvalues[k+diff];
	}
	for ( k=0; k<257; ++k ) {
	    if ( index[k]>offpos )
		index[k] -= diff;
	}
	top -= diff;
    }
    values[0] = 0;
    for ( i=1; i<top; ++i )
	values[i] = totvalues[i]/cnt[i];
return( top );
}

void TeXDefaultParams(SplineFont *sf) {
    int i, spacew;
    BlueData bd;

    if ( sf->texdata.generalset || sf->texdata.mathset || sf->texdata.mathextset )
return;

    spacew = .33*(1<<20);	/* 1/3 em for a space seems like a reasonable size */
    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL && sf->chars[i]->unicodeenc==' ' ) {
	spacew = (sf->chars[i]->width<<20)/(sf->ascent+sf->descent);
    break;
    }
    QuickBlues(sf,&bd);

    memset(sf->texdata.params,0,sizeof(sf->texdata.params));
    sf->texdata.params[0] = rint( -sin(sf->italicangle)*(1<<20) );	/* slant */
    sf->texdata.params[1] = spacew;			/* space */
    sf->texdata.params[2] = spacew/2;			/* stretch_space */
    sf->texdata.params[3] = spacew/3;			/* shrink space */
    if ( bd.xheight>0 )
	sf->texdata.params[4] = (bd.xheight*(1<<20))/(sf->ascent+sf->descent);
    sf->texdata.params[5] = 1<<20;			/* quad */
    sf->texdata.params[6] = spacew/3;			/* extra space after sentence period */
}

int TfmSplineFont(FILE *tfm, SplineFont *sf, int formattype) {
    struct tfm_header header;
    char *full=NULL, *encname;
    int i;
    DBounds b;
    struct ligkern *ligkerns[256], *lk;
    double widths[257], heights[257], depths[257], italics[257];
    uint8 tags[256], lkindex[256];
    int former[256];
    int charlistindex[257], extensions[257], extenindex[257];
    int widthindex[257], heightindex[257], depthindex[257], italicindex[257];
    double *kerns;
    int widcnt, hcnt, dcnt, icnt, kcnt, lkcnt, pcnt, ecnt;
    int first, last;
    KernPair *kp;
    LigList *l;
    int style, any;
    uint32 *lkarray;

    SFLigaturePrepare(sf);
    LigatureClosure(sf);		/* Convert 3 character ligs to a set of two character ones when possible */
    SFKernPrepare(sf);			/* Undoes kern classes */

    memset(&header,0,sizeof(header));
    header.checksum = 0;		/* don't check checksum (I don't know how to calculate it) */
    header.design_size = 10<<20;	/* default to 10pt */
    encname=NULL;
    if ( sf->subfontcnt==0 && sf->encoding_name!=em_custom && !sf->compacted )
	encname = EncodingName(sf->encoding_name );
    if ( encname==NULL ) {
	full = galloc(strlen(sf->fontname)+10);
	strcpy(full,sf->fontname);
	strcat(full,"-Enc");
	encname = full;
    }
    header.encoding[0] = strlen(encname);
    if ( header.encoding[0]>39 ) {
	header.encoding[0] = 39;
	memcpy(header.encoding+1,encname,39);
    } else
	strcpy(header.encoding+1,encname);
    if ( full ) free(full);

    header.family[0] = strlen(sf->familyname);
    if ( header.family[0]>19 ) {
	header.family[0] = 19;
	memcpy(header.family+1,sf->familyname,19);
    } else
	strcpy(header.family+1,sf->familyname);
    for ( i=128; i<sf->charcnt && i<256; ++i )
	if ( SCWorthOutputting(sf->chars[i]))
    break;
    if ( i>=sf->charcnt || i>=256 )
	header.seven_bit_safe_flag = true;
    style = MacStyleCode(sf,NULL);
    if ( style&sf_italic )
	header.face = 1;
    if ( style&sf_bold )
	header.face+=2;
    else if ( strstrmatch(sf->fontname,"Ligh") ||
	    strstrmatch(sf->fontname,"Thin") ||
	    strstrmatch(sf->fontname,"Maigre") ||
	    strstrmatch(sf->fontname,"Mager") )
	header.face += 4;
    if ( style&sf_condense )
	header.face += 6;
    else if ( style&sf_extend )
	header.face += 12;

    TeXDefaultParams(sf);
    pcnt = sf->texdata.mathset ? 22 : sf->texdata.mathextset ? 13 : 7;

    memset(widths,0,sizeof(widths));
    memset(heights,0,sizeof(heights));
    memset(depths,0,sizeof(depths));
    memset(italics,0,sizeof(italics));
    memset(tags,0,sizeof(tags));
    first = last = -1;
    for ( i=0; i<256 && i<sf->charcnt; ++i ) if ( SCWorthOutputting(sf->chars[i])) {
	SplineCharFindBounds(sf->chars[i],&b);
	widths[i] = sf->chars[i]->width;
	if ( b.maxy>0 )
	    heights[i] = b.maxy;
	if ( b.miny<0 )
	    depths[i] = -b.miny;
	if ( (style&sf_italic) && b.maxx>sf->chars[i]->width )
	    italics[i] = (b.maxx-sf->chars[i]->width)*(1<<20)/(sf->ascent+sf->descent) +
		    ((1<<20)>>4);	/* With a 1/16 em white space after it */
	if ( first==-1 ) first = i;
	last = i;
    }
    widcnt = CoalesceValues(widths,256,widthindex);
    hcnt = CoalesceValues(heights,16,heightindex);
    dcnt = CoalesceValues(depths,16,depthindex);
    icnt = CoalesceValues(italics,16,italicindex);
    if ( last==-1 ) { first = 1; last = 0; }

    FindCharlists(sf,charlistindex);
    ecnt = FindExtensions(sf,extensions,extenindex);

    kcnt = 0;
    for ( i=0; i<256 && i<sf->charcnt; ++i ) if ( SCWorthOutputting(sf->chars[i])) {
	SplineChar *sc = sf->chars[i];
	for ( kp=sc->kerns; kp!=NULL; kp=kp->next )
	    if ( kp->sc->enc<256 ) ++kcnt;
    }
    kerns = NULL;
    if ( kcnt!=0 )
	kerns = galloc(kcnt*sizeof(double));
    kcnt = lkcnt = 0;
    memset(ligkerns,0,sizeof(ligkerns));
    for ( i=0; i<256 && i<sf->charcnt; ++i ) if ( SCWorthOutputting(sf->chars[i])) {
	SplineChar *sc = sf->chars[i];
	for ( kp=sc->kerns; kp!=NULL; kp=kp->next )
	    if ( kp->sc->enc<256 )
		ligkerns[i] = TfmAddKern(kp,ligkerns[i],kerns,&kcnt);
	for ( l=sc->ligofme; l!=NULL; l=l->next )
	    ligkerns[i] = TfmAddLiga(l,ligkerns[i]);
	if ( ligkerns[i]!=NULL ) {
	    tags[i] = 1;
	    for ( lk=ligkerns[i]; lk!=NULL; lk=lk->next )
		++lkcnt;
	}
	if ( extenindex[i]!=-1 )
	    tags[i] = 3;
	else if ( charlistindex[i]!=-1 )
	    tags[i] = 2;
    }
    lkarray = galloc(lkcnt*sizeof(uint32));
    memset(former,-1,sizeof(former));
    memset(lkindex,0,sizeof(lkindex));
    lkcnt = 0;
    do {
	any = false;
	for ( i=0; i<256; ++i ) if ( ligkerns[i]!=NULL ) {
	    lk = ligkerns[i];
	    ligkerns[i] = lk->next;
	    if ( former[i]!=-1 )
		lkarray[former[i]] |= (lkcnt-former[i]-1)<<24;
	    else
		lkindex[i] = lkcnt;
	    former[i] = lkcnt;
	    lkarray[lkcnt++] = ((lk->next==NULL?128:0)<<24) |
				(lk->other_char<<16) |
			        (lk->op<<8) |
			        lk->remainder;
	    free( lk );
	    any = true;
	}
    } while ( any );

/* Now output the file */
	/* Table of contents */
    putshort(tfm,
	    6+			/* Table of contents size */
	    18 +		/* header size */
	    (last-first+1) +	/* Per glyph data */
	    widcnt +		/* entries in the width table */
	    hcnt +		/* entries in the height table */
	    dcnt +		/* entries in the depth table */
	    icnt +		/* entries in the italic correction table */
	    lkcnt +		/* entries in the lig/kern table */
	    kcnt +		/* entries in the kern table */
	    ecnt +		/* No extensible characters here */
	    pcnt);		/* font parameter words */
    putshort(tfm,18);
    putshort(tfm,first);
    putshort(tfm,last);
    putshort(tfm,widcnt);
    putshort(tfm,hcnt);
    putshort(tfm,dcnt);
    putshort(tfm,icnt);
    putshort(tfm,lkcnt);
    putshort(tfm,kcnt);
    putshort(tfm,ecnt);
    putshort(tfm,pcnt);
	    /* header */
    putlong(tfm,header.checksum);
    putlong(tfm,header.design_size);
    fwrite(header.encoding,1,sizeof(header.encoding),tfm);
    fwrite(header.family,1,sizeof(header.family),tfm);
    fwrite(&header.seven_bit_safe_flag,1,4,tfm);
	    /* per-glyph data */
    for ( i=first; i<=last ; ++i ) {
	if ( SCWorthOutputting(sf->chars[i])) {
	    putc(widthindex[i],tfm);
	    putc((heightindex[i]<<4)|depthindex[i],tfm);
	    putc((italicindex[i]<<2)|tags[i],tfm);
	    putc(tags[i]==0?0 :
		    tags[i]==1?lkindex[i] :
		    tags[i]==2?charlistindex[i] :
		    extenindex[i],tfm);
	} else {
	    putlong(tfm,0);
	}
    }
	    /* width table */
    for ( i=0; i<widcnt; ++i )
	putlong(tfm,(widths[i]*(1<<20))/(sf->ascent+sf->descent));
	    /* height table */
    for ( i=0; i<hcnt; ++i )
	putlong(tfm,(heights[i]*(1<<20))/(sf->ascent+sf->descent));
	    /* depth table */
    for ( i=0; i<dcnt; ++i )
	putlong(tfm,(depths[i]*(1<<20))/(sf->ascent+sf->descent));
	    /* italic correction table */
    for ( i=0; i<icnt; ++i )
	putlong(tfm,(italics[i]*(1<<20))/(sf->ascent+sf->descent));
	    /* lig/kern table */
    for ( i=0; i<lkcnt; ++i )
	putlong(tfm,lkarray[i]);
	    /* kern table */
    for ( i=0; i<kcnt; ++i )
	putlong(tfm,(kerns[i]*(1<<20))/(sf->ascent+sf->descent));
	    /* extensible table */
    for ( i=0; i<ecnt; ++i )
	putlong(tfm,extensions[i]);
	    /* font parameters */
    for ( i=0; i<pcnt; ++i )
	putlong(tfm,sf->texdata.params[i]);

    SFLigatureCleanup(sf);
    SFKernCleanup(sf);

return( !ferror(tfm));
}
