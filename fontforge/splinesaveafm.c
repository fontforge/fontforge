/* Copyright (C) 2000-2005 by George Williams */
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

#include <sys/types.h>		/* For stat */
#include <sys/stat.h>
#include <unistd.h>

#ifdef __CygWin
 #include <sys/types.h>
 #include <sys/stat.h>
 #include <unistd.h>
#endif

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

static void KPInsert( SplineChar *sc1, SplineChar *sc2, int off, int isv ) {
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
	    if ( isv ) {
		kp->next = sc1->vkerns;
		sc1->vkerns = kp;
	    } else {
		kp->next = sc1->kerns;
		sc1->kerns = kp;
	    }
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
#if defined(FONTFORGE_CONFIG_GDRAW)
    GProgressChangeLine2R(_STR_ReadingAFM);
#elif defined(FONTFORGE_CONFIG_GTK)
    gwwv_progress_change_line2(_("Reading AFM file"));
#endif
    while ( mygets(file,buffer,sizeof(buffer))!=NULL ) {
	if ( strncmp(buffer,"KPX",3)==0 || strncmp(buffer,"KPY",3)==0 ) {
	    int isv = strncmp(buffer,"KPY",3)==0;
	    for ( pt=buffer+3; isspace(*pt); ++pt);
	    for ( ept = pt; *ept!='\0' && !isspace(*ept); ++ept );
	    ch = *ept; *ept = '\0';
	    sc1 = SFGetChar(sf,-1,pt);
	    *ept = ch;
	    for ( pt=ept; isspace(*pt); ++pt);
	    for ( ept = pt; *ept!='\0' && !isspace(*ept); ++ept );
	    ch = *ept; *ept = '\0';
	    sc2 = SFGetChar(sf,-1,pt);
	    *ept = ch;
	    off = strtol(ept,NULL,10);
	    KPInsert(sc1,sc2,off,isv);
	} else if ( sscanf( buffer, "C %*d ; WX %*d ; N %40s ; B %*d %*d %*d %*d ; L %40s %40s",
		name, second, lig)==3 ) {
	    sc1 = SFGetChar(sf,-1,lig);
	    sc2 = SFGetChar(sf,-1,name);
	    if ( sc2==NULL )
		sc2 = SFGetChar(sf,-1,second);
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

static void CheckMMAfmFile(SplineFont *sf,char *amfm_filename,char *fontname) {
    /* the afm file should be in the same directory as the amfm file */
    /*  with the fontname as the filename */
    char *temp, *pt;

    free(sf->fontname);
    sf->fontname = copy(fontname);

    temp = galloc(strlen(amfm_filename)+strlen(fontname)+strlen(".afm")+1);
    strcpy(temp, amfm_filename);
    pt = strrchr(temp,'/');
    if ( pt==NULL ) pt = temp;
    else ++pt;
    strcpy(pt,fontname);
    pt += strlen(pt);
    strcpy(pt,".afm");
    if ( !LoadKerningDataFromAfm(sf,temp) ) {
	strcpy(pt,".AFM");
	LoadKerningDataFromAfm(sf,temp);
    }
    free(temp);
}

int LoadKerningDataFromAmfm(SplineFont *sf, char *filename) {
    FILE *file=NULL;
    char buffer[280], *pt, lastname[256];
    int index, i;
    MMSet *mm = sf->mm;

    if ( mm!=NULL )
	file = fopen(filename,"r");
    pt = strstrmatch(filename,".amfm");
    if ( pt!=NULL ) {
	char *afmname = copy(filename);
	strcpy(afmname+(pt-filename),isupper(pt[1])?".AFM":".afm");
	LoadKerningDataFromAfm(mm->normal,afmname);
	free(afmname);
    }
    if ( file==NULL )
return( 0 );

#if defined(FONTFORGE_CONFIG_GDRAW)
    GProgressChangeLine2R(_STR_ReadingAFM);
#elif defined(FONTFORGE_CONFIG_GTK)
    gwwv_progress_change_line2(_("Reading AFM file"));
#endif
    while ( fgets(buffer,sizeof(buffer),file)!=NULL ) {
	if ( strstrmatch(buffer,"StartMaster")!=NULL )
    break;
    }
    index = -1; lastname[0] = '\0';
    while ( fgets(buffer,sizeof(buffer),file)!=NULL ) {
	if ( strstrmatch(buffer,"EndMaster")!=NULL ) {
	    if ( lastname[0]!='\0' && index!=-1 && index<mm->instance_count )
		CheckMMAfmFile(mm->instances[index],filename,lastname);
	    index = -1; lastname[0] = '\0';
	} else if ( sscanf(buffer,"FontName %256s", lastname )== 1 ) {
	    /* Do Nothing, all done */
	} else if ( (pt = strstr(buffer,"WeightVector"))!=NULL ) {
	    pt += strlen("WeightVector");
	    while ( *pt==' ' || *pt=='[' ) ++pt;
	    i = 0;
	    while ( *pt!=']' && *pt!='\0' ) {
		if ( *pt=='0' )
		    ++i;
		else if ( *pt=='1' ) {
		    index = i;
	    break;
		}
		++pt;
	    }
	}
    }
    fclose(file);
return( true );
}

int CheckAfmOfPostscript(SplineFont *sf,char *psname) {
    char *new, *pt;
    int ret;
    int wasuc=false;

    new = galloc(strlen(psname)+6);
    strcpy(new,psname);
    pt = strrchr(new,'.');
    if ( pt==NULL ) pt = new+strlen(new);
    else wasuc = isupper(pt[1]);

    if ( sf->mm!=NULL ) {
	strcpy(pt,wasuc?".AMFM":".amfm");
	if ( !LoadKerningDataFromAmfm(sf,new)) {
	    strcpy(pt,wasuc?".amfm":".AMFM");
	    ret = LoadKerningDataFromAmfm(sf,new);
	} else
	    ret = true;
	/* The above routine reads from the afm file if one exist */
    } else {
	strcpy(pt,wasuc?".AFM":".afm");
	if ( !LoadKerningDataFromAfm(sf,new)) {
	    strcpy(pt,wasuc?".afm":".AFM");
	    ret = LoadKerningDataFromAfm(sf,new);
	} else
	    ret = true;
    }
    free(new);
return( ret );
}

static void SubsNew(SplineChar *to,enum possub_type type,int tag,char *components,
	    SplineChar *default_script) {
    PST *pst;

    pst = chunkalloc(sizeof(PST));
    pst->type = type;
    pst->script_lang_index = SCDefaultSLI(to->parent,default_script);
    pst->tag = tag;
    pst->u.alt.components = components;
    if ( type == pst_ligature )
	pst->u.lig.lig = to;
    SCInsertPST(to,pst);
}

static void PosNew(SplineChar *to,int tag,int dx, int dy, int dh, int dv) {
    PST *pst;

    pst = chunkalloc(sizeof(PST));
    pst->type = pst_position;
    pst->script_lang_index = SCDefaultSLI(to->parent,to);
    pst->tag = tag;
    pst->u.pos.xoff = dx;
    pst->u.pos.yoff = dy;
    pst->u.pos.h_adv_off = dh;
    pst->u.pos.v_adv_off = dv;
    SCInsertPST(to,pst);
}

static void LigatureNew(SplineChar *sc3,SplineChar *sc1,SplineChar *sc2) {
    char *components = galloc(strlen(sc1->name)+strlen(sc2->name)+2);
    strcpy(components,sc1->name);
    strcat(components," ");
    strcat(components,sc2->name);
    SubsNew(sc3,pst_ligature,CHR('l','i','g','a'),components,sc1);
}

static void tfmDoLigKern(SplineFont *sf, int enc, int lk_index,
	uint8 *ligkerntab, uint8 *kerntab) {
    int enc2, k_index;
    SplineChar *sc1, *sc2, *sc3;
    real off;

    if ( enc>=sf->charcnt || (sc1=sf->chars[enc])==NULL )
return;

    if ( ligkerntab[lk_index*4+0]>128 )		/* Special case to get big indeces */
	lk_index = 256*ligkerntab[lk_index*4+2] + ligkerntab[lk_index*4+3];

    while ( 1 ) {
	enc2 = ligkerntab[lk_index*4+1];
	if ( enc2>=sf->charcnt || (sc2=sf->chars[enc2])==NULL )
	    /* Not much we can do. can't kern to a non-existant char */;
	else if ( ligkerntab[lk_index*4+2]>=128 ) {
	    k_index = ((ligkerntab[lk_index*4+2]-128)*256+ligkerntab[lk_index*4+3])*4;
	    off = (sf->ascent+sf->descent) *
		    ((kerntab[k_index]<<24) + (kerntab[k_index+1]<<16) +
			(kerntab[k_index+2]<<8) + kerntab[k_index+3])/
		    (double) 0x100000;
 /* printf( "%s(%d) %s(%d) -> %g\n", sc1->name, sc1->enc, sc2->name, sc2->enc, off); */
	    KPInsert(sc1,sc2,off,false);
	} else if ( ligkerntab[lk_index*4+2]==0 &&
		ligkerntab[lk_index*4+3]<sf->charcnt &&
		(sc3=sf->chars[ligkerntab[lk_index*4+3]])!=NULL ) {
	    LigatureNew(sc3,sc1,sc2);
	}
	if ( ligkerntab[lk_index*4]>=128 )
    break;
	lk_index += ligkerntab[lk_index*4] + 1;
    }
}

static void tfmDoCharList(SplineFont *sf,int i,int *charlist) {
    int used[256], ucnt, len, was;
    char *components;

    if ( i>=sf->charcnt || sf->chars[i]==NULL )
return;

    ucnt = 0, len=0;
    while ( i!=-1 ) {
	if ( i<sf->charcnt && sf->chars[i]!=NULL ) {
	    used[ucnt++] = i;
	    len += strlen(sf->chars[i]->name)+1;
	}
	was = i;
	i = charlist[i];
	charlist[was] = -1;
    }
    if ( ucnt<=1 )
return;

    components = galloc(len+1); components[0] = '\0';
    for ( i=1; i<ucnt; ++i ) {
	strcat(components,sf->chars[used[i]]->name);
	if ( i!=ucnt-1 )
	    strcat(components," ");
    }
    SubsNew(sf->chars[used[0]],pst_alternate,CHR('T','C','H','L'),components,
	    sf->chars[used[0]]);
}

static void tfmDoExten(SplineFont *sf,int i,uint8 *ext) {
    int j, len, k;
    char *names[4], *components;

    if ( i>=sf->charcnt || sf->chars[i]==NULL )
return;

    names[0] = names[1] = names[2] = names[3] = ".notdef";
    for ( j=len=0; j<4; ++j ) {
	k = ext[j];
	if ( k<sf->charcnt && sf->chars[k]!=NULL )
	    names[j] = sf->chars[k]->name;
	len += strlen(names[j])+1;
    }
    components = galloc(len); components[0] = '\0';
    for ( j=0; j<4; ++j ) {
	strcat(components,names[j]);
	if ( j!=3 )
	    strcat(components," ");
    }
    SubsNew(sf->chars[i],pst_multiple,CHR('T','E','X','L'),components,
	    sf->chars[i]);
}

#define BigEndianWord(pt) ((((uint8 *) pt)[0]<<24) | (((uint8 *) pt)[1]<<16) | (((uint8 *) pt)[2]<<8) | (((uint8 *) pt)[3]))

static void tfmDoItalicCor(SplineFont *sf,int i,real italic_cor) {

    if ( i>=sf->charcnt || sf->chars[i]==NULL )
return;

    PosNew(sf->chars[i],CHR('I','T','L','C'),0,0,italic_cor,0);
}

int LoadKerningDataFromTfm(SplineFont *sf, char *filename) {
    FILE *file = fopen(filename,"rb");
    int i, tag, left, ictag;
    uint8 *ligkerntab, *kerntab, *ext, *ictab, *httab, *dptab, *widtab;
    int head_len, first, last, width_size, height_size, depth_size, italic_size,
	    ligkern_size, kern_size, esize, param_size;
    int charlist[256], ept[256];
    int is_math;
    int width, height, depth;
    real scale = (sf->ascent+sf->descent)/(double) (1<<20);

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
    esize = getushort(file);
    param_size = getushort(file);
    if ( first-1>last || last>=256 ) {
	fclose(file);
return( 0 );
    }
    kerntab = gcalloc(kern_size,sizeof(int32));
    ligkerntab = gcalloc(ligkern_size,sizeof(int32));
    ext = gcalloc(esize,sizeof(int32));
    ictab = gcalloc(italic_size,sizeof(int32));
    dptab = gcalloc(depth_size,sizeof(int32));
    httab = gcalloc(height_size,sizeof(int32));
    widtab = gcalloc(width_size,sizeof(int32));
    fseek( file,(6+1)*sizeof(int32),SEEK_SET);
    sf->design_size = (5*getlong(file)+(1<<18))>>19;	/* TeX stores as <<20, adobe in decipoints */
    fseek( file,
	    (6+head_len+(last-first+1))*sizeof(int32),
	    SEEK_SET);
    fread( widtab,1,width_size*sizeof(int32),file);
    fread( httab,1,height_size*sizeof(int32),file);
    fread( dptab,1,depth_size*sizeof(int32),file);
    fread( ictab,1,italic_size*sizeof(int32),file);
    fread( ligkerntab,1,ligkern_size*sizeof(int32),file);
    fread( kerntab,1,kern_size*sizeof(int32),file);
    fread( ext,1,esize*sizeof(int32),file);
    for ( i=0; i<22 && i<param_size; ++i )
	sf->texdata.params[i] = getlong(file);
    if ( param_size==22 ) sf->texdata.type = tex_math;
    else if ( param_size==13 ) sf->texdata.type = tex_mathext;
    else if ( param_size>=7 ) sf->texdata.type = tex_text;

    /* Fields in tfm files have different meanings for math fonts */
    is_math = sf->texdata.type == tex_mathext || sf->texdata.type == tex_math;

    memset(charlist,-1,sizeof(charlist));
    memset(ept,-1,sizeof(ept));

    fseek( file, (6+head_len)*sizeof(int32), SEEK_SET);
    for ( i=first; i<=last; ++i ) {
	width = getc(file);
	height = getc(file);
	depth = height&0xf; height >>= 4;
	ictag = getc(file);
	tag = (ictag&3);
	ictag>>=2;
	left = getc(file);
	if ( tag==1 )
	    tfmDoLigKern(sf,i,left,ligkerntab,kerntab);
	else if ( tag==2 )
	    charlist[i] = left;
	else if ( tag==3 )
	    tfmDoExten(sf,i,ext+left);
	if ( sf->chars[i]!=NULL ) {
	    SplineChar *sc = sf->chars[i];
/* TFtoPL says very clearly: The actual width of a character is */
/*  width[width_index], in design-size units. It is not in design units */
/*  it is stored as a fraction of the design size in a fixed word */
	    sc->tex_height = BigEndianWord(httab+4*height)*scale;
	    sc->tex_depth = BigEndianWord(dptab+4*depth)*scale;
	    if ( !is_math ) {
		if ( ictag!=0 )
		    tfmDoItalicCor(sf,i,BigEndianWord(ictab+4*ictag)*scale);
		/* we ignore the width. I trust the real width in the file more */
	    } else {
		sc->tex_sub_pos = BigEndianWord(widtab+4*width)*scale;
		sc->tex_super_pos = BigEndianWord(ictab+4*ictag)*scale;
	    }
	}
    }

    for ( i=first; i<=last; ++i ) {
	if ( charlist[i]!=-1 )
	    tfmDoCharList(sf,i,charlist);
    }
    
    free( ligkerntab); free(kerntab); free(ext); free(ictab);
    fclose(file);
return( 1 );
}

char *EncodingName(Encoding *map) {
    char *name = map->iconv_name != NULL ? map->iconv_name : map->enc_name;
    int len = strlen(name);
    char *pt;

    if ( strmatch(name,"AdobeStandard")==0 )
return( "AdobeStandardEncoding" );
    if (( strstr(name,"8859")!=NULL && name[len-1]=='1' &&
	     (!isdigit(name[len-2]) || name[len-2]=='9') ) ||
	    strstrmatch(name,"latin1")!=NULL )
return( "ISOLatin1Encoding" );
    else if ( map->is_unicodebmp || map->is_unicodefull )
return( "ISO10646-1" );
    else if ( strmatch(name,"mac")==0 || strmatch(name,"macintosh")==0 ||
	    strmatch(name,"macroman")==0 )
return( "MacRoman" );
    else if ( strmatch(name,"ms-ansi")==0 || strstrmatch(name,"1252")!=NULL )
return( "WinRoman" );
    else if ( strmatch(name,"sjis")==0 ||
	    ((pt = strstrmatch(name,"jp"))!=NULL && pt[2]=='\0' &&
		    strstr(name,"646")==NULL ))
return( "JISX0208.1997" );
    else if ( map->is_japanese )
return( "JISX0212.1990" );
    else if ( strmatch(name,"johab")==0 )
return( "Johab" );
    else if ( map->is_korean )
return( "KSC5601.1992" );
    else if ( map->is_simplechinese )
return( "GB2312.1980" );
    else if ( strstrmatch(name,"hkscs")!=NULL )
return( "BIG5HKSCS.2001" );
    else if ( map->is_tradchinese )
return( "BIG5" );			/* 2002? */

    if ( map->is_custom || map->is_original || map->is_compact )
return( "FontSpecific" );

return( name );
}

static char *_EncodingName(SplineFont *sf) {
    int i;
    char *name = sf->encoding_name->iconv_name != NULL ? sf->encoding_name->iconv_name : sf->encoding_name->enc_name;

    if ( strmatch(name,"AdobeStandard")==0 ) {
	for ( i=0; i<256 && i<sf->charcnt; ++i ) {
	    if ( !SCWorthOutputting(sf->chars[i]))
		/* That'll do */;
	    else if ( strcmp(sf->chars[i]->name,AdobeStandardEncoding[i])!=0 )
return( "FontSpecific" );
	}
return( "AdobeStandardEncoding" );
    }
return( EncodingName(sf->encoding_name ));
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
    else if ( sf->mm!=NULL ) sf=sf->mm->normal;

    if ( sf->script_lang==NULL ) {
	IError("How can there be ligatures with no script/lang list?");
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
    fprintf( afm, "C %d ; WX %d ; ", enc, sc->width*1000/em );
    if ( sc->parent->hasvmetrics )
	fprintf( afm, "WY %d ; ", sc->vwidth*1000/em );
    fprintf( afm, "N %s ; B %d %d %d %d ;",
	    sc->name,
	    (int) floor(b.minx*1000/em), (int) floor(b.miny*1000/em),
	    (int) ceil(b.maxx*1000/em), (int) ceil(b.maxy*1000/em) );
    if (sc->ligofme!=NULL)
	AfmLigOut(afm,sc);
    putc('\n',afm);
#if defined(FONTFORGE_CONFIG_GDRAW)
    GProgressNext();
#elif defined(FONTFORGE_CONFIG_GTK)
    gwwv_progress_next();
#endif
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
    fprintf( afm, "C %d ; WX %d ; ", enc, sc->width*1000/em );
    if ( sc->parent->hasvmetrics )
	fprintf( afm, "WY %d ; ", sc->vwidth*1000/em );
    fprintf( afm, "N %s ; B %d %d %d %d ;",
	    sc->name,
	    (int) floor(b.minx*1000/em), (int) floor(b.miny*1000/em),
	    (int) ceil(b.maxx*1000/em), (int) ceil(b.maxy*1000/em) );
    if (sc->ligofme!=NULL)
	AfmLigOut(afm,sc);
    putc('\n',afm);
#if defined(FONTFORGE_CONFIG_GDRAW)
    GProgressNext();
#elif defined(FONTFORGE_CONFIG_GTK)
    gwwv_progress_next();
#endif
}

static void AfmCIDChar(FILE *afm, SplineChar *sc, int enc) {
    DBounds b;
    int em = (sc->parent->ascent+sc->parent->descent);

    SplineCharFindBounds(sc,&b);
    fprintf( afm, "C -1 ; WX %d ; ", sc->width*1000/em );
    if ( sc->parent->hasvmetrics )
	fprintf( afm, "WY %d ; ", sc->vwidth*1000/em );
    fprintf( afm, "N %d ; B %d %d %d %d ;",
	    enc,
	    (int) floor(b.minx*1000/em), (int) floor(b.miny*1000/em),
	    (int) ceil(b.maxx*1000/em), (int) ceil(b.maxy*1000/em) );
    putc('\n',afm);
#if defined(FONTFORGE_CONFIG_GDRAW)
    GProgressNext();
#elif defined(FONTFORGE_CONFIG_GTK)
    gwwv_progress_next();
#endif
}

static int anykerns(SplineFont *sf,int isv) {
    int i, cnt = 0;
    KernPair *kp;

    for ( i=0; i<sf->charcnt; ++i ) {
	if ( sf->chars[i]!=NULL && strcmp(sf->chars[i]->name,".notdef")!=0 ) {
	    for ( kp = isv ? sf->chars[i]->vkerns : sf->chars[i]->kerns; kp!=NULL; kp = kp->next )
		if ( kp->off!=0 && strcmp(kp->sc->name,".notdef")!=0 )
		    ++cnt;
	}
    }
return( cnt );
}

static void AfmKernPairs(FILE *afm, SplineChar *sc, int isv) {
    KernPair *kp;
    int em = (sc->parent->ascent+sc->parent->descent);

    if ( strcmp(sc->name,".notdef")==0 )
return;

    for ( kp = isv ? sc->vkerns : sc->kerns; kp!=NULL; kp=kp->next ) {
	if ( strcmp(kp->sc->name,".notdef")!=0 && kp->off!=0 ) {
	    if ( isv )
		fprintf( afm, "KPY %s %s %d\n", sc->name, kp->sc->name, kp->off*1000/em );
	    else
		fprintf( afm, "KPX %s %s %d\n", sc->name, kp->sc->name, kp->off*1000/em );
	}
    }
}

/* Look for character duplicates, such as might be generated by having the same */
/*  glyph at two encoding slots */
/* I think it is ok if something depends on this character, because the */
/*  code that handles references will automatically unwrap it down to be base */
SplineChar *SCDuplicate(SplineChar *sc) {
    SplineChar *matched = sc;

    if ( sc==NULL || sc->parent==NULL || sc->parent->cidmaster!=NULL )
return( sc );		/* Can't do this in CID keyed fonts */

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

int SCIsNotdef(SplineChar *sc,int fixed) {
return( sc!=NULL && sc->enc==0 && sc->layers[ly_fore].refs==NULL && strcmp(sc->name,".notdef")==0 &&
	(sc->layers[ly_fore].splines!=NULL || (sc->widthset && fixed==-1) ||
	 (sc->width!=sc->parent->ascent+sc->parent->descent && fixed==-1 ) ||
	 (sc->width==fixed && fixed!=-1 && sc->widthset)));
}

int SCDrawsSomething(SplineChar *sc) {
    int layer,l;
    RefChar *ref;

    if ( sc==NULL )
return( false );
    for ( layer = ly_fore; layer<sc->layer_cnt; ++layer ) {
	if ( sc->layers[layer].splines!=NULL || sc->layers[layer].images!=NULL )
return( true );
	for ( ref = sc->layers[layer].refs; ref!=NULL; ref=ref->next )
	    for ( l=0; l<ref->layer_cnt; ++l )
		if ( ref->layers[l].splines!=NULL )
return( true );
    }
return( false );
}

int SCWorthOutputting(SplineChar *sc) {
return( sc!=NULL &&
	( SCDrawsSomething(sc) || sc->widthset || sc->anchor!=NULL ||
#if HANYANG
	    sc->compositionunit ||
#endif
	    sc->dependents!=NULL ) &&
	( strcmp(sc->name,".notdef")!=0 || sc->enc==0) &&
	( (strcmp(sc->name,".null")!=0 && strcmp(sc->name,"glyph1")!=0 &&
	   strcmp(sc->name,"nonmarkingreturn")!=0 && strcmp(sc->name,"glyph2")!=0) ||
	  sc->layers[ly_fore].splines!=NULL || sc->dependents!=NULL || sc->layers[ly_fore].refs!=NULL ) );
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

static void AfmSplineFontHeader(FILE *afm, SplineFont *sf, int formattype) {
    DBounds b;
    real width;
    int i, j, cnt, max;
    int caph, xh, ash, dsh;
    int iscid = ( formattype==ff_cid || formattype==ff_otfcid );
    int ismm = ( formattype==ff_mma || formattype==ff_mmb );
    time_t now;
    SplineChar *sc;
    int em = (sf->ascent+sf->descent);

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

    fprintf( afm, ismm ? "StartMasterFontMetrics 4.0\n" :
		  iscid ? "StartFontMetrics 4.1\n" :
			  "StartFontMetrics 2.0\n" );
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
	fprintf( afm, "EncodingScheme %s\n", _EncodingName(sf));
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
}

static void LigatureClosure(SplineFont *sf);

int AfmSplineFont(FILE *afm, SplineFont *sf, int formattype) {
    int i, j, cnt, max, vcnt;
    int type0 = ( formattype==ff_ptype0 );
    int encmax=!type0?256:65536;
    int anyzapf;
    int iscid = ( formattype==ff_cid || formattype==ff_otfcid );
    SplineChar *sc;

    SFLigaturePrepare(sf);
    LigatureClosure(sf);		/* Convert 3 character ligs to a set of two character ones when possible */
    SFKernPrepare(sf,false);		/* Undoes kern classes */
    SFKernPrepare(sf,true);

    if ( iscid && sf->cidmaster!=NULL ) sf = sf->cidmaster;

    AfmSplineFontHeader(afm,sf,formattype);

    max = sf->charcnt;
    if ( iscid ) {
	max = 0;
	for ( i=0; i<sf->subfontcnt; ++i )
	    if ( sf->subfonts[i]->charcnt>max )
		max = sf->subfonts[i]->charcnt;
    }
    cnt = 0;
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
	if ( SCWorthOutputting(sc) || (iscid && i==0 && sc!=NULL)) 
	    ++cnt;
    }

    anyzapf = false;
    if ( type0 && (sf->encoding_name->is_unicodebmp ||
	    sf->encoding_name->is_unicodefull)) {
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
    } else if ( type0 && (sf->encoding_name->is_unicodebmp ||
	    sf->encoding_name->is_unicodefull)) {
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
    } else if ( type0 ) {
	for ( i=0; i<sf->charcnt && i<encmax; ++i )
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
    vcnt = anykerns(sf,true);
    if ( (cnt = anykerns(sf,false))>0 || vcnt>0 ) {
	fprintf( afm, "StartKernData\n" );
	if ( cnt>0 ) {
	    fprintf( afm, "StartKernPairs%s %d\n", vcnt==0?"":"0", cnt );
	    for ( i=0; i<sf->charcnt ; ++i )
		if ( SCWorthOutputting(sf->chars[i]) ) {
		    AfmKernPairs(afm,sf->chars[i],false);
		}
	    fprintf( afm, "EndKernPairs\n" );
	}
	if ( vcnt>0 ) {
	    fprintf( afm, "StartKernPairs1 %d\n", vcnt );
	    for ( i=0; i<sf->charcnt ; ++i )
		if ( SCWorthOutputting(sf->chars[i]) ) {
		    AfmKernPairs(afm,sf->chars[i],true);
		}
	    fprintf( afm, "EndKernPairs\n" );
	}
	fprintf( afm, "EndKernData\n" );
    }
    fprintf( afm, "EndFontMetrics\n" );

    SFLigatureCleanup(sf);
    SFKernCleanup(sf,false);
    SFKernCleanup(sf,true);

return( !ferror(afm));
}

int AmfmSplineFont(FILE *amfm, MMSet *mm, int formattype) {
    int i,j;

    AfmSplineFontHeader(amfm,mm->normal,formattype);
    fprintf( amfm, "Masters %d\n", mm->instance_count );
    fprintf( amfm, "Axes %d\n", mm->axis_count );

    fprintf( amfm, "WeightVector [%g", mm->defweights[0] );
    for ( i=1; i<mm->instance_count; ++i )
	fprintf( amfm, " %g", mm->defweights[i]);
    fprintf( amfm, "]\n" );

    fprintf( amfm, "BlendDesignPositions [" );
    for ( i=0; i<mm->instance_count; ++i ) {
	fprintf(amfm, "[%g", mm->positions[i*mm->axis_count+0]);
	for ( j=1; j<mm->axis_count; ++j )
	    fprintf( amfm, " %g", mm->positions[i*mm->axis_count+j]);
	fprintf(amfm, i==mm->instance_count-1 ? "]" : "] " );
    }
    fprintf( amfm, "]\n" );

    fprintf( amfm, "BlendDesignMap [" );
    for ( i=0; i<mm->axis_count; ++i ) {
	putc('[',amfm);
	for ( j=0; j<mm->axismaps[i].points; ++j )
	    fprintf(amfm, "[%g %g]", mm->axismaps[i].designs[j], mm->axismaps[i].blends[j]);
	fprintf(amfm, i==mm->axis_count-1 ? "]" : "] " );
    }
    fprintf( amfm, "]\n" );

    fprintf( amfm, "BlendAxisTypes [/%s", mm->axes[0] );
    for ( j=1; j<mm->axis_count; ++j )
	fprintf( amfm, " /%s", mm->axes[j]);
    fprintf( amfm, "]\n" );

    for ( i=0; i<mm->axis_count; ++i ) {
	fprintf( amfm, "StartAxis\n" );
	fprintf( amfm, "AxisType %s\n", mm->axes[i] );
	fprintf( amfm, "AxisLabel %s\n", MMAxisAbrev(mm->axes[i]) );
	fprintf( amfm, "EndAxis\n" );
    }

    for ( i=0; i<mm->instance_count; ++i ) {
	fprintf( amfm, "StartMaster\n" );
	fprintf( amfm, "FontName %s\n", mm->instances[i]->fontname );
	if ( mm->instances[i]->fullname!=NULL )
	    fprintf( amfm, "FullName %s\n", mm->instances[i]->fullname );
	if ( mm->instances[i]->familyname!=NULL )
	    fprintf( amfm, "FamilyName %s\n", mm->instances[i]->familyname );
	if ( mm->instances[i]->version!=NULL )
	    fprintf( amfm, "Version %s\n", mm->instances[i]->version );
	fprintf( amfm, "WeightVector [%d", i==0 );
	for ( j=1; j<mm->instance_count; ++j )
	    fprintf( amfm, " %d", i==j );
	fprintf( amfm, "]\n" );
	fprintf( amfm, "EndMaster\n" );
    }
    fprintf( amfm, "EndMasterFontMetrics\n" );
return( !ferror(amfm));
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
    for ( i=0 ; i<sf->charcnt; ++i ) if ( SCWorthOutputting(sf->chars[i]) && sf->chars[i]->possub!=NULL ) {
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
		    chunkfree(head,sizeof(*head));
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

void SFKernCleanup(SplineFont *sf,int isv) {
    int i;
    KernPair *kp, *p, *n;

    if ( (!isv && sf->kerns==NULL) || (isv && sf->vkerns==NULL) )	/* can't have gotten messed up */
return;
    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	for ( kp = isv ? sf->chars[i]->vkerns : sf->chars[i]->kerns, p=NULL; kp!=NULL; kp = n ) {
	    n = kp->next;
	    if ( kp->kcid!=0 ) {
		if ( p!=NULL )
		    p->next = n;
		else if ( isv )
		    sf->chars[i]->vkerns = n;
		else
		    sf->chars[i]->kerns = n;
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
	int16 offset, uint16 sli, uint16 flags,uint16 kcid,int isv) {
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
	if ( isv ) {
	    kp->next = first->vkerns;
	    first->vkerns = kp;
	} else {
	    kp->next = first->kerns;
	    first->kerns = kp;
	}
    }
}

void SFKernPrepare(SplineFont *sf,int isv) {
    KernClass *kc, *head= isv ? sf->vkerns : sf->kerns;
    SplineChar ***first, ***last;
    int i, j, k, l;

    for ( kc = head, i=0; kc!=NULL; kc = kc->next )
	kc->kcid = ++i;
    for ( kc = head; kc!=NULL; kc = kc->next ) {
	first = KernClassToSC(sf,kc->firsts,kc->first_cnt);
	last = KernClassToSC(sf,kc->seconds,kc->second_cnt);
	for ( i=1; i<kc->first_cnt; ++i ) for ( j=1; j<kc->second_cnt; ++j ) {
	    if ( kc->offsets[i*kc->second_cnt+j]!=0 ) {
		for ( k=0; first[i][k]!=NULL; ++k )
		    for ( l=0; last[j][l]!=NULL; ++l )
			AddTempKP(first[i][k],last[j][l],
				kc->offsets[i*kc->second_cnt+j],
			        kc->sli,kc->flags, kc->kcid,isv);
	    }
	}
	KCSfree(first,kc->first_cnt);
	KCSfree(last,kc->second_cnt);
    }
}

static int getlshort(FILE *pfm) {
    int ch1;
    ch1 = getc(pfm);
return( (getc(pfm)<<8)|ch1 );
}

static int getlint(FILE *pfm) {
    int ch1, ch2, ch3;
    ch1 = getc(pfm);
    ch2 = getc(pfm);
    ch3 = getc(pfm);
return( (getc(pfm)<<24)|(ch3<<16)|(ch2<<8)|ch1 );
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

static const unsigned short unicode_from_win[] = {
  0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
  0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f,
  0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017,
  0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f,
  0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
  0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
  0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
  0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
  0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
  0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
  0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
  0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
  0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
  0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
  0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
  0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f,
  0x20ac, 0x0081, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021,
  0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008d, 0x008e, 0x008f,
  0x0090, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
  0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0x009d, 0x009e, 0x0178,
  0x00a0, 0x00a1, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7,
  0x00a8, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00af,
  0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7,
  0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf,
  0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7,
  0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
  0x00d0, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7,
  0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x00dd, 0x00de, 0x00df,
  0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7,
  0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
  0x00f0, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f7,
  0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x00fd, 0x00fe, 0x00ff
};

static void inwin(SplineFont *sf, int winmap[256]) {
    int i, j;

    memset(winmap,-1,sizeof(int[256]));
    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	int unienc = sf->chars[i]->unicodeenc;
	if ( unienc==-1 || unienc>0x3000 )
    continue;
	for ( j=255; j>=0; --j ) {
	    if ( unicode_from_win[j]==unienc ) {
		winmap[j] = i;
	break;
	    }
	}
    }
}

static int revwinmap(int winmap[256], int enc) {
    int i;
    for ( i=255; i>=0; --i )
	if ( winmap[i]==enc )
    break;
return( i );
}

int PfmSplineFont(FILE *pfm, SplineFont *sf, int type0) {
    int caph=0, xh=0, ash=0, dsh=0, cnt=0, first=-1, samewid=-1, maxwid= -1, last=0, wid=0, ymax=0, ymin=0;
    int kerncnt=0, spacepos=0x20;
    int i, ii;
    char *pt;
    KernPair *kp;
    int winmap[256];
    /* my docs imply that pfm files can only handle 1byte fonts */
    /* I think they must be output as if the font were encoded in winansi */
    /* Ah, but Adobe's technical note 5178.PFM.PDF gives the two byte format */
    long size, devname, facename, extmetrics, exttable, driverinfo, kernpairs, pos;
    DBounds b;
    int style;
    int windows_encoding;

    if ( sf->encoding_name->is_japanese ||
	    (sf->cidmaster!=NULL && strnmatch(sf->cidmaster->ordering,"Japan",5)==0 ))
	windows_encoding = 128;
    else if ( sf->encoding_name->is_korean ||
	    (sf->cidmaster!=NULL && strnmatch(sf->cidmaster->ordering,"Korea",5)==0 ))
	windows_encoding = 129;
    else if ( sf->encoding_name->is_tradchinese ||
	    (sf->cidmaster!=NULL && strnmatch(sf->cidmaster->ordering,"CNS",3)==0 ))
	windows_encoding = 136;
    else if ( sf->encoding_name->is_simplechinese ||
	    (sf->cidmaster!=NULL && strnmatch(sf->cidmaster->ordering,"GB",2)==0 ))
	windows_encoding = 134;
    else
	windows_encoding = strmatch(sf->encoding_name->enc_name,"symbol")==0?2:0;
    if ( windows_encoding==0 )
	inwin(sf,winmap);
    else {
	for ( i=0; i<256 && i<sf->charcnt; ++i )
	    winmap[i] = i;
	for ( ; i<256; ++i )
	    winmap[i] = -1;
    }

    SFKernPrepare(sf,false);
    for ( ii=0; ii<256; ++ii ) if ( (i=winmap[ii])!=-1 ) {
	if ( sf->chars[i]!=NULL && sf->chars[i]->unicodeenc==' ' )
	    spacepos = ii;
	if ( SCWorthOutputting(sf->chars[i]) ) {
	    ++cnt;
	    if ( sf->chars[i]->unicodeenc=='I' || sf->chars[i]->unicodeenc=='x' ||
		    sf->chars[i]->unicodeenc=='H' || sf->chars[i]->unicodeenc=='d' || 
		    sf->chars[i]->unicodeenc=='p' || sf->chars[i]->unicodeenc=='l' ) {
		SplineCharFindBounds(sf->chars[i],&b);
		if ( ymax<b.maxy ) ymax = b.maxy;
		if ( ymin>b.miny ) ymin = b.miny;
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
		first = ii;
	    wid += sf->chars[i]->width;
	    last = ii;
	    for ( kp=sf->chars[i]->kerns; kp!=NULL; kp = kp->next )
		if ( revwinmap(winmap,kp->sc->enc)!=-1 )
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
    putlshort(ymax,pfm);		/* ascent (Adobe says the "ascent" is the max high in the font's bounding box) */
    if ( caph==0 ) caph = ash;
    if ( ymax-ymin>=sf->ascent+sf->descent )
	putlshort(0,pfm);		/* Adobe says so */
    else
	putlshort(sf->ascent+sf->descent-(ymax-ymin),pfm);	/* Internal leading */
    putlshort(196*(sf->ascent+sf->descent)/1000,pfm);	/* External leading, Adobe says 196 */
    style = MacStyleCode(sf,NULL);
    putc(style&sf_italic?1:0,pfm);	/* is italic */
    putc(0,pfm);			/* underline */
    putc(0,pfm);			/* strikeout */
    putlshort(sf->pfminfo.weight,pfm);	/* weight */
    putc(windows_encoding,pfm);		/* Some indication of the encoding */
    putlshort(/*samewid<0?sf->ascent+sf->descent:samewid*/0,pfm);	/* width */
    putlshort(sf->ascent+sf->descent,pfm);	/* height */
    putc(sf->pfminfo.pfmfamily,pfm);	/* family */
    putlshort(wid,pfm);			/* average width, Docs say "Width of "X", but that's wrong */
    putlshort(maxwid,pfm);		/* max width */

    if ( first>255 ) first=last = 0;
    else if ( last>255 ) { first=32; last = 255;}/* Yes, even for 2 byte fonts we do this (Adobe says so)*/
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
    for ( ii=first; ii<=last; ++ii ) {
	if ( (i=winmap[ii])==-1 || sf->chars[i]==NULL )
	    putlshort(0,pfm);
	else
	    putlshort(sf->chars[i]->width,pfm);
    }

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
	putlshort(kerncnt,pfm);		/* number of kerning pairs <= 512 */
	kerncnt = 0;
	for ( ii=first; ii<last; ++ii ) if ( (i=winmap[ii])!=-1 && sf->chars[i]!=NULL ) {
	    if ( SCWorthOutputting(sf->chars[i]) ) {
		for ( kp=sf->chars[i]->kerns; kp!=NULL; kp = kp->next )
		    if ( kerncnt<512 && revwinmap(winmap,kp->sc->enc)!=-1 ) {
			putc(ii,pfm);
			putc(revwinmap(winmap,kp->sc->enc),pfm);
			putlshort(kp->off,pfm);
			++kerncnt;
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

    SFKernCleanup(sf,false);

#ifdef __CygWin
    /* Modern versions of windows want the execute bit set on a pfm file */
    /* I've no idea what this corresponds to in windows, nor any idea on */
    /*  how to set it from the windows UI, but this seems to work */
    {
	struct stat buf;
	fstat(fileno(pfm),&buf);
	fchmod(fileno(pfm),S_IXUSR | buf.st_mode );
    }
#endif

return( !ferror(pfm));
}

int LoadKerningDataFromPfm(SplineFont *sf, char *filename) {
    FILE *file = fopen(filename,"rb");
    int widthbytes, kernoff, i, kerncnt;
    int ch1, ch2, offset;
    int winmap[256];
    int encoding;

    if ( file==NULL )
return( 0 );
    if ( getlshort(file)!=0x100 ) {
	fclose(file);
return( false );
    }
    /* filesize = */ getlint(file);
    for ( i=0; i<60; ++i ) getc(file);	/* Skip the copyright */
    /* flags = */	getlshort(file);
    /* ptsize = */	getlshort(file);
    /* vertres = */	getlshort(file);
    /* horres = */	getlshort(file);
    /* ascent = */	getlshort(file);
    /* int leading = */	getlshort(file);
    /* ext leading = */	getlshort(file);
    /* italic = */	getc(file);
    /* underline = */	getc(file);
    /* strikeout = */	getc(file);
    /* weight = */	getlshort(file);
    encoding =		getc(file);
    /* width = */	getlshort(file);
    /* height = */	getlshort(file);
    /* family = */	getc(file);
    /* avgwid = */	getlshort(file);
    /* maxwid = */	getlshort(file);
    /* first = */	getc(file);
    /* last = */	getc(file);
    /* space = */	getc(file);
    /* word break = */	getc(file);
    widthbytes =	getlshort(file);
    /* off to devname */getlint(file);
    /* off to facename */getlint(file);
    /* bitspointer */	getlint(file);
    /* bitsoffset */	getlint(file);

    for ( i=0; i<widthbytes; ++i )	/* Ignore the width table */
	getc(file);
    if ( getlshort(file)<0x12 )
	kernoff = 0;			/* Extensions table is too short to hold a kern pointer */
    else {
	/* extmetrics = */	getlint(file);
	/* exttable = */	getlint(file);
	/* origin table = */	getlint(file);
	kernoff =		getlint(file);
    }
    if ( kernoff!=0 && !feof(file) ) {
	fseek(file,kernoff,SEEK_SET);
	if ( encoding==0 )
	    inwin(sf,winmap);
	else
	    for ( i=0; i<256; ++i )
		winmap[i] = i;
	kerncnt = getlshort(file);
	/* Docs say at most 512 kerning pairs. Not worth posting a progress */
	/*  dlg */
	for ( i=0; i<kerncnt; ++i ) {
	    ch1 = getc(file);
	    ch2 = getc(file);
	    offset = (short) getlshort(file);
	    if ( !feof(file) && winmap[ch1]!=-1 && winmap[ch2]!=-1 )
		KPInsert(sf->chars[winmap[ch1]],sf->chars[winmap[ch2]],offset,false);
		/* No support for vertical kerning that I'm aware of */
	}
    }
    fclose(file);
return( true );
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
	for ( j=i+1; j<top && values[i]==values[j]; ++j );
	if ( j>i+1 ) {
	    int diff = j-i-1;
	    for ( k=i+1; k+diff<top; ++k )
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
    if ( top<=max ) {
	if ( values[0]!=0 ) {
	    for ( i=0; i<top && values[i]!=0; ++i );
	    if ( i==top )
		IError("zero must be present in tfm arrays");
	    else {
		values[i] = values[0];
		values[0] = 0;
		for ( k=0; k<257; ++k ) {
		    if ( index[k]==0 ) index[k] = i;
		    else if ( index[k]==i ) index[k] = 0;
		}
	    }
	}
return( top );
    }

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
	for ( k=offpos+1; k+diff<top; ++k ) {
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

    if ( sf->texdata.type!=tex_unset )
return;

    spacew = rint(.33*(1<<20));	/* 1/3 em for a space seems like a reasonable size */
    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL && sf->chars[i]->unicodeenc==' ' ) {
	spacew = rint((sf->chars[i]->width<<20)/(sf->ascent+sf->descent));
    break;
    }
    QuickBlues(sf,&bd);

    memset(sf->texdata.params,0,sizeof(sf->texdata.params));
    sf->texdata.params[0] = rint( -sin(sf->italicangle)*(1<<20) );	/* slant */
    sf->texdata.params[1] = spacew;			/* space */
    sf->texdata.params[2] = rint(spacew/2);		/* stretch_space */
    sf->texdata.params[3] = rint(spacew/3);		/* shrink space */
    if ( bd.xheight>0 )
	sf->texdata.params[4] = rint((((double) bd.xheight)*(1<<20))/(sf->ascent+sf->descent));
    sf->texdata.params[5] = 1<<20;			/* quad */
    sf->texdata.params[6] = rint(spacew/3);		/* extra space after sentence period */

    /* Let's provide some vaguely reasonable defaults for math fonts */
    /*  I'll ignore math ext fonts */
    /* hmm. TeX math fonts have space, stretch, shrink set to 0 */
    sf->texdata.params[7] = rint(.747*(1<<20));
    sf->texdata.params[8] = rint(.424*(1<<20));
    sf->texdata.params[9] = rint(.474*(1<<20));
    sf->texdata.params[10] = rint(.756*(1<<20));
    sf->texdata.params[11] = rint(.375*(1<<20));
    sf->texdata.params[12] = rint(.413*(1<<20));
    sf->texdata.params[13] = rint(.363*(1<<20));
    sf->texdata.params[14] = rint(.289*(1<<20));
    sf->texdata.params[15] = rint(.15*(1<<20));
    sf->texdata.params[16] = rint(.309*(1<<20));
    sf->texdata.params[17] = rint(.386*(1<<20));
    sf->texdata.params[18] = rint(.05*(1<<20));
    sf->texdata.params[19] = rint(2.39*(1<<20));
    sf->texdata.params[20] = rint(1.01*(1<<20));
    sf->texdata.params[21] = rint(.25*(1<<20));
}

int TfmSplineFont(FILE *tfm, SplineFont *sf, int formattype) {
    struct tfm_header header;
    char *full=NULL, *encname;
    int i;
    DBounds b;
    struct ligkern *ligkerns[256], *lk, *lknext;
    double widths[257], heights[257], depths[257], italics[257];
    uint8 tags[256], lkindex[256];
    int former[256];
    int charlistindex[257], extensions[257], extenindex[257];
    int widthindex[257], heightindex[257], depthindex[257], italicindex[257];
    double *kerns;
    int widcnt, hcnt, dcnt, icnt, kcnt, lkcnt, pcnt, ecnt, sccnt;
    int first, last;
    KernPair *kp;
    LigList *l;
    int style, any;
    uint32 *lkarray;
    PST *pst;
    char *familyname;
    int anyITLC;
    int is_math = sf->texdata.type==tex_math || sf->texdata.type==tex_mathext;
    real scale = (1<<20)/(double) (sf->ascent+sf->descent);

    SFLigaturePrepare(sf);
    LigatureClosure(sf);		/* Convert 3 character ligs to a set of two character ones when possible */
    SFKernPrepare(sf,false);		/* Undoes kern classes */
    TeXDefaultParams(sf);

    memset(&header,0,sizeof(header));
    header.checksum = 0;		/* don't check checksum (I don't know how to calculate it) */
    header.design_size = ((sf->design_size<<19)+2)/5;
    if ( header.design_size==0 ) header.design_size = 10<<20;
    encname=NULL;
    /* These first two encoding names appear magic to txtopl */
    /* I tried checking the encodings were correct, name by name, but */
    /*  that doesn't work. People use non-standard names */
    if ( sf->texdata.type==tex_math ) encname = "TEX MATH SYMBOLS";
    else if ( sf->texdata.type==tex_mathext ) encname = "TEX MATH EXTENSION";
    else if ( sf->subfontcnt==0 && sf->encoding_name!=&custom && !sf->compacted )
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

    familyname = sf->cidmaster ? sf->cidmaster->familyname : sf->familyname;
    header.family[0] = strlen(familyname);
    if ( header.family[0]>19 ) {
	header.family[0] = 19;
	memcpy(header.family+1,familyname,19);
    } else
	strcpy(header.family+1,familyname);
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

    pcnt = sf->texdata.type==tex_math ? 22 : sf->texdata.type==tex_mathext ? 13 : 7;

    memset(widths,0,sizeof(widths));
    memset(heights,0,sizeof(heights));
    memset(depths,0,sizeof(depths));
    memset(italics,0,sizeof(italics));
    memset(tags,0,sizeof(tags));
    first = last = -1;
    /* Note: Text fonts for TeX and math fonts use the italic correction & width */
    /*  fields to mean different things */
    anyITLC = false;
    for ( i=0; i<256 && i<sf->charcnt; ++i ) if ( SCWorthOutputting(sf->chars[i])) {
	if ( (pst=SCFindPST(sf->chars[i],pst_position,CHR('I','T','L','C'),-1,-1))!=NULL ) {
	    anyITLC = true;
    break;
	}
    }
    for ( i=0; i<256 && i<sf->charcnt; ++i ) if ( SCWorthOutputting(sf->chars[i])) {
	SplineChar *sc = sf->chars[i];
	if ( sc->tex_height==TEX_UNDEF || sc->tex_depth==TEX_UNDEF )
	    SplineCharFindBounds(sc,&b);
	heights[i] = (sc->tex_height==TEX_UNDEF ? b.maxy : sc->tex_height)*scale;
	depths[i] = (sc->tex_height==TEX_UNDEF ? -b.miny : sc->tex_depth)*scale;
	if ( !is_math ) {
	    widths[i] = sc->width*scale;
	    if ( (pst=SCFindPST(sc,pst_position,CHR('I','T','L','C'),-1,-1))!=NULL )
		italics[i] = pst->u.pos.h_adv_off*scale;
	    else if ( (style&sf_italic) && b.maxx>sc->width && !anyITLC )
		italics[i] = ((b.maxx-sc->width) +
			    (sf->ascent+sf->descent)/16.0)*scale;
					/* With a 1/16 em white space after it */
	} else {
	    if ( sc->tex_sub_pos!=TEX_UNDEF )
		widths[i] = sc->tex_sub_pos*scale;
	    else
		widths[i] = sc->width*scale;
	    if ( sc->tex_super_pos!=TEX_UNDEF )
		italics[i] = sc->tex_super_pos*scale;
	    else if ( sc->tex_sub_pos!=TEX_UNDEF )
		italics[i] = (sc->width - sc->tex_sub_pos)*scale;
	    else
		italics[i] = 0;
	}
	if ( first==-1 ) first = i;
	last = i;
    }
    widcnt = CoalesceValues(widths,256,widthindex);
    hcnt = CoalesceValues(heights,16,heightindex);
    dcnt = CoalesceValues(depths,16,depthindex);
    icnt = CoalesceValues(italics,64,italicindex);
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
    for ( i=sccnt=0; i<256 && i<sf->charcnt; ++i )
	if ( ligkerns[i]!=NULL )
	    ++sccnt;
    if ( sccnt>=128 )	/* We need to use the special extension mechanism */
	lkcnt += sccnt;
    lkarray = galloc(lkcnt*sizeof(uint32));
    memset(former,-1,sizeof(former));
    memset(lkindex,0,sizeof(lkindex));
    if ( sccnt<128 ) {
	lkcnt = 0;
	do {
	    any = false;
	    for ( i=0; i<256; ++i ) if ( ligkerns[i]!=NULL ) {
		lk = ligkerns[i];
		ligkerns[i] = lk->next;
		if ( former[i]==-1 )
		    lkindex[i] = lkcnt;
		else {
		    lkarray[former[i]] |= (lkcnt-former[i]-1)<<24;
		    if ( lkcnt-former[i]-1 >= 128 )
			fprintf( stderr, "Internal error generating lig/kern array, jump too far.\n" );
		}
		former[i] = lkcnt;
		lkarray[lkcnt++] = ((lk->next==NULL?128U:0U)<<24) |
				    (lk->other_char<<16) |
				    (lk->op<<8) |
				    lk->remainder;
		free( lk );
		any = true;
	    }
	} while ( any );
    } else {
	int lkcnt2 = sccnt;
	lkcnt = 0;
	for ( i=0; i<256; ++i ) if ( ligkerns[i]!=NULL ) {
	    lkindex[i] = lkcnt;
	    /* long pointer into big ligkern array */
	    lkarray[lkcnt++] = (129U<<24) |
				(0<<16) |
			        lkcnt2;
	    for ( lk = ligkerns[i]; lk!=NULL; lk=lknext ) {
		lknext = lk->next;
		/* Here we will always skip to the next record, so the	*/
		/* skip_byte will always be 0 (well, or 128 for stop)	*/
		former[i] = lkcnt;
		lkarray[lkcnt2++] = ((lknext==NULL?128:0)<<24) |
				    (lk->other_char<<16) |
				    (lk->op<<8) |
				    lk->remainder;
		free( lk );
	    }
	}
	if ( lkcnt>sccnt )
	    fprintf( stderr, "Internal error, lig/kern mixup\n" );
	lkcnt = lkcnt2;
    }

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
    SFKernCleanup(sf,false);

return( !ferror(tfm));
}

enum metricsformat { mf_none, mf_afm, mf_amfm, mf_tfm, mf_pfm };

static enum metricsformat MetricsFormatType(char *filename) {
    FILE *file = fopen(filename,"rb");
    unsigned char buffer[200];
    struct stat sb;
    int len;

    if ( file==NULL )
return( mf_none );

    len = fread(buffer,1,sizeof(buffer)-1,file);
    buffer[len] = '\0';
    fstat(fileno(file),&sb);
    fclose(file);

    if ( strstr((char *) buffer,"StartFontMetrics")!=NULL )
return( mf_afm );
    if ( strstr((char *) buffer,"StartMasterFontMetrics")!=NULL ||
	    strstr((char *) buffer,"StarMasterFontMetrics")!=NULL )	/* ff had a bug and used this file header by mistake */
return( mf_amfm );
    
    if ( len >= 48 && sb.st_size == 4*((buffer[0]<<8)|buffer[1]) &&
	    ((buffer[0]<<8)|buffer[1]) == 6 +
		    ((buffer[2]<<8)|buffer[3]) +
		    ( ((buffer[6]<<8)|buffer[7]) - ((buffer[4]<<8)|buffer[5]) + 1 ) +
		    ((buffer[8]<<8)|buffer[9]) +
		    ((buffer[10]<<8)|buffer[11]) +
		    ((buffer[12]<<8)|buffer[13]) +
		    ((buffer[14]<<8)|buffer[15]) +
		    ((buffer[16]<<8)|buffer[17]) +
		    ((buffer[18]<<8)|buffer[19]) +
		    ((buffer[20]<<8)|buffer[21]) +
		    ((buffer[22]<<8)|buffer[23]) )
return( mf_tfm );

    if ( len>= 6 && buffer[0]==0 && buffer[1]==1 &&
	    (buffer[2]|(buffer[3]<<8)|(buffer[4]<<16)|(buffer[5]<<24))== sb.st_size )
return( mf_pfm );

    if ( strstrmatch(filename,".afm")!=NULL )
return( mf_afm );
    if ( strstrmatch(filename,".amfm")!=NULL )
return( mf_amfm );
    if ( strstrmatch(filename,".tfm")!=NULL )
return( mf_tfm );
    if ( strstrmatch(filename,".pfm")!=NULL )
return( mf_pfm );

return( mf_none );
}

int LoadKerningDataFromMetricsFile(SplineFont *sf,char *filename) {

    switch ( MetricsFormatType(filename)) {
      case mf_afm:
return( LoadKerningDataFromAfm(sf,filename));
      case mf_amfm:
return( LoadKerningDataFromAmfm(sf,filename));
      case mf_tfm:
return( LoadKerningDataFromTfm(sf,filename));
      case mf_pfm:
return( LoadKerningDataFromPfm(sf,filename));

      case mf_none:
      default:
	/* If all else fails try a mac resource */
	/* mac resources can be in so many different formats and even files */
	/*  that I'm not even going to try to check for it here */
return( LoadKerningDataFromMacFOND(sf,filename));
    }
}
