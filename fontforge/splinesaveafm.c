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

#include <fontforge-config.h>

#include "splinesaveafm.h"

#include "autohint.h"
#include "featurefile.h"
#include "fontforgevw.h"		/* For Error */
#include "fvcomposite.h"
#include "fvfonts.h"
#include "gutils.h"
#include "lookups.h"
#include "macbinary.h"
#include "mem.h"
#include "mm.h"
#include "namelist.h"
#include "splinefont.h"
#include "splinesave.h"
#include "splineutil.h"
#include "tottf.h"
#include "tottfgpos.h"
#include "ttf.h"		/* For AnchorClassDecompose */
#include "ustring.h"
#include "utype.h"
#include "zapfnomen.h"

#include <math.h>
#include <stdio.h>
#include <time.h>

#include <sys/stat.h>
#include <sys/types.h>		/* For stat */
#include <unistd.h>

#ifdef __CygWin
 #include <sys/stat.h>
 #include <sys/types.h>
 #include <unistd.h>
#endif

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

/* ************************************************************************** */
/* **************************** Reading AFM files *************************** */
/* ************************************************************************** */
static void KPInsert( SplineChar *sc1, SplineChar *sc2, int off, int isv ) {
    KernPair *kp;
    int32_t script;

    if ( sc1!=NULL && sc2!=NULL ) {
	for ( kp=sc1->kerns; kp!=NULL && kp->sc!=sc2; kp = kp->next );
	if ( kp!=NULL )
	    kp->off = off;
	else if ( off!=0 ) {
	    kp = chunkalloc(sizeof(KernPair));
	    kp->sc = sc2;
	    kp->off = off;
	    script = SCScriptFromUnicode(sc1);
	    if ( script==DEFAULT_SCRIPT )
		script = SCScriptFromUnicode(sc2);
	    kp->subtable = SFSubTableFindOrMake(sc1->parent,
		    isv?CHR('v','k','r','n'):CHR('k','e','r','n'),
		    script, gpos_pair);
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
    char name[44], second[44], lig[44], buf2[100];
    PST *liga;
    bigreal scale = (sf->ascent+sf->descent)/1000.0;

    if ( file==NULL )
return( 0 );
    ff_progress_change_line2(_("Reading AFM file"));
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
	    KPInsert(sc1,sc2,rint(off*scale),isv);
	} else if ( buffer[0]=='C' && isspace(buffer[1])) {
	    char *pt;
	    sc2 = NULL;
	    for ( pt= strchr(buffer,';'); pt!=NULL; pt=strchr(pt+1,';') ) {
		if ( sscanf( pt, "; N %40s", name )==1 )
		    sc2 = SFGetChar(sf,-1,name);
		else if ( sc2!=NULL &&
			sscanf( pt, "; L %40s %40s", second, lig)==2 ) {
		    sc1 = SFGetChar(sf,-1,lig);
		    if ( sc1!=NULL ) {
			sprintf( buf2, "%s %s", name, second);
			for ( liga=sc1->possub; liga!=NULL; liga=liga->next ) {
			    if ( liga->type == pst_ligature && strcmp(liga->u.lig.components,buf2)==0 )
			break;
			}
			if ( liga==NULL ) {
			    liga = chunkalloc(sizeof(PST));
			    liga->subtable = SFSubTableFindOrMake(sf,
				    CHR('l','i','g','a'),SCScriptFromUnicode(sc2),
				    gsub_ligature);
			    liga->subtable->lookup->store_in_afm = true;
			    liga->type = pst_ligature;
			    liga->next = sc1->possub;
			    sc1->possub = liga;
			    liga->u.lig.lig = sc1;
			    liga->u.lig.components = copy( buf2 );
			}
		    }
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

    temp = malloc(strlen(amfm_filename)+strlen(fontname)+strlen(".afm")+1);
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
    char buffer[280], *pt, lastname[257];
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

    ff_progress_change_line2(_("Reading AFM file"));
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

int CheckAfmOfPostScript(SplineFont *sf,char *psname) {
    char *new, *pt;
    int ret;
    int wasuc=false;

    new = malloc(strlen(psname)+6);
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

void SubsNew(SplineChar *to,enum possub_type type,int tag,char *components,
	    SplineChar *default_script) {
    PST *pst;

    pst = chunkalloc(sizeof(PST));
    pst->type = type;
    pst->subtable = SFSubTableFindOrMake(to->parent,tag,SCScriptFromUnicode(default_script),
	    type==pst_substitution ? gsub_single :
	    type==pst_alternate ? gsub_alternate :
	    type==pst_multiple ? gsub_multiple : gsub_ligature );
    pst->u.alt.components = components;
    if ( type == pst_ligature ) {
	pst->u.lig.lig = to;
	pst->subtable->lookup->store_in_afm = true;
    }
    pst->next = to->possub;
    to->possub = pst;
}

void PosNew(SplineChar *to,int tag,int dx, int dy, int dh, int dv) {
    PST *pst;

    pst = chunkalloc(sizeof(PST));
    pst->type = pst_position;
    pst->subtable = SFSubTableFindOrMake(to->parent,tag,SCScriptFromUnicode(to),
	    gpos_single );
    pst->u.pos.xoff = dx;
    pst->u.pos.yoff = dy;
    pst->u.pos.h_adv_off = dh;
    pst->u.pos.v_adv_off = dv;
    pst->next = to->possub;
    to->possub = pst;
}

static void LigatureNew(SplineChar *sc3,SplineChar *sc1,SplineChar *sc2) {
    char *components = malloc(strlen(sc1->name)+strlen(sc2->name)+2);
    strcpy(components,sc1->name);
    strcat(components," ");
    strcat(components,sc2->name);
    SubsNew(sc3,pst_ligature,CHR('l','i','g','a'),components,sc1);
}

/* ************************************************************************** */
/* **************************** Reading TFM files *************************** */
/* ************************************************************************** */
struct tfmdata {
    int file_len;
    int head_len;
    int first, last;
    int width_size;
    int height_size;
    int depth_size;
    int italic_size;
    int ligkern_size;
    int kern_size;
    int esize;
    int param_size;

    uint8_t *kerntab;
    uint8_t *ligkerntab;
    uint8_t *ext;
    uint8_t *ictab;
    uint8_t *dptab;
    uint8_t *httab;
    uint8_t *widtab;

    int *charlist;
};

static void tfmDoLigKern(SplineFont *sf, int enc, int lk_index,
	struct tfmdata *tfmd,EncMap *map) {
    int enc2, k_index;
    SplineChar *sc1, *sc2, *sc3;
    real off;

    if ( enc>=map->enccount || map->map[enc]==-1 || (sc1=sf->glyphs[map->map[enc]])==NULL )
return;
    if ( enc<tfmd->first || enc>tfmd->last )
return;
    if ( lk_index>=tfmd->ligkern_size )
return;

    if ( tfmd->ligkerntab[lk_index*4+0]>128 )		/* Special case to get big indeces */
	lk_index = 256*tfmd->ligkerntab[lk_index*4+2] + tfmd->ligkerntab[lk_index*4+3];

    while ( 1 ) {
	if ( lk_index>=tfmd->ligkern_size )
return;
	enc2 = tfmd->ligkerntab[lk_index*4+1];
	if ( enc2>=map->enccount || map->map[enc2]==-1 || (sc2=sf->glyphs[map->map[enc2]])==NULL ||
		enc2<tfmd->first || enc2>tfmd->last )
	    /* Not much we can do. can't kern to a non-existant char */;
	else if ( tfmd->ligkerntab[lk_index*4+2]>=128 ) {
	    k_index = ((tfmd->ligkerntab[lk_index*4+2]-128)*256+
		    tfmd->ligkerntab[lk_index*4+3])*4;
	    if ( k_index>4*tfmd->kern_size )
return;
	    off = (sf->ascent+sf->descent) *
		    ((tfmd->kerntab[k_index]<<24) + (tfmd->kerntab[k_index+1]<<16) +
			(tfmd->kerntab[k_index+2]<<8) + tfmd->kerntab[k_index+3])/
		    (bigreal) 0x100000;
 /* printf( "%s(%d) %s(%d) -> %g\n", sc1->name, sc1->enc, sc2->name, sc2->enc, off); */
	    KPInsert(sc1,sc2,rint(off),false);
	} else if ( tfmd->ligkerntab[lk_index*4+2]==0 &&
		tfmd->ligkerntab[lk_index*4+3]<map->enccount &&
		tfmd->ligkerntab[lk_index*4+3]>=tfmd->first &&
		tfmd->ligkerntab[lk_index*4+3]<=tfmd->last &&
		map->map[tfmd->ligkerntab[lk_index*4+3]]!=-1 &&
		(sc3=sf->glyphs[map->map[tfmd->ligkerntab[lk_index*4+3]]])!=NULL ) {
	    LigatureNew(sc3,sc1,sc2);
	}
	if ( tfmd->ligkerntab[lk_index*4]>=128 )
    break;
	lk_index += tfmd->ligkerntab[lk_index*4] + 1;
    }
}

int doesGlyphExpandHorizontally(SplineChar *UNUSED(sc)) {
return( false );
}

static void tfmDoCharList(SplineFont *sf,int i,struct tfmdata *tfmd,EncMap *map) {
    int used[256], ucnt, len, was;
    char *components;
    SplineChar *sc;
    struct glyphvariants **gvbase;

    if ( i>=map->enccount || map->map[i]==-1 || sf->glyphs[map->map[i]]==NULL ||
	    i<tfmd->first || i>tfmd->last )
return;

    ucnt = 0, len=0;
    while ( i!=-1 ) {
	if ( i<map->enccount && map->map[i]!=-1 && sf->glyphs[map->map[i]]!=NULL &&
		i>=tfmd->first && i<=tfmd->last ) {
	    used[ucnt++] = map->map[i];
	    len += strlen(sf->glyphs[map->map[i]]->name)+1;
	    /*sf->glyphs[map->map[i]]->is_extended_shape = true;*/	/* MS does not do this in their fonts */
	}
	was = i;
	i = tfmd->charlist[i];
	tfmd->charlist[was] = -1;
    }
    if ( ucnt<=1 )
return;

    sc = sf->glyphs[used[0]];
    if ( sc==NULL )
return;
    components = malloc(len+1); components[0] = '\0';
    for ( i=1; i<ucnt; ++i ) {
	strcat(components,sf->glyphs[used[i]]->name);
	if ( i!=ucnt-1 )
	    strcat(components," ");
    }
    gvbase = doesGlyphExpandHorizontally(sc)?&sc->horiz_variants: &sc->vert_variants;
    if ( *gvbase==NULL )
	*gvbase = chunkalloc(sizeof(struct glyphvariants));
    (*gvbase)->variants = components;
}

static void tfmDoExten(SplineFont *sf,int i,struct tfmdata *tfmd,int left,EncMap *map) {
    int j, k, gid, gid2, cnt;
    SplineChar *bits[4], *bats[5];
    uint8_t *ext;
    SplineChar *sc;
    struct glyphvariants **gvbase;
    int is_horiz;

    if ( i>=map->enccount || (gid=map->map[i])==-1 || sf->glyphs[gid]==NULL )
return;
    if ( left>=tfmd->esize )
return;

    ext = tfmd->ext + 4*left;

    sc = sf->glyphs[gid];
    if ( sc==NULL )
return;
    is_horiz = doesGlyphExpandHorizontally(sc);
    gvbase = is_horiz?&sc->horiz_variants: &sc->vert_variants;
    if ( *gvbase==NULL )
	*gvbase = chunkalloc(sizeof(struct glyphvariants));

    memset(bits,0,sizeof(bits));
    for ( j=0; j<4; ++j ) {
	k = ext[j];
	if ( k!=0 && k<map->enccount && (gid2 = map->map[k])!=-1 && sf->glyphs[gid2]!=NULL ) {
	    bits[j] = sf->glyphs[gid2];
	    /* bits[j]->is_extended_shape = true;*/	/* MS does not do this in their fonts */
	}
    }
    /* 0=>bottom, 1=>middle, 2=>top, 3=>extender */
    cnt = 0;
    if ( bits[0]!=NULL )
	bats[cnt++] = bits[0];
    if ( bits[3]!=NULL )
	bats[cnt++] = bits[3];
    if ( bits[1]!=NULL ) {
	bats[cnt++] = bits[1];
	if ( bits[3]!=NULL )
	    bats[cnt++] = bits[3];
    }
    if ( bits[2]!=NULL )
	bats[cnt++] = bits[2];

    (*gvbase)->part_cnt = cnt;
    (*gvbase)->parts = calloc(cnt,sizeof(struct gv_part));
    for ( j=0; j<cnt; ++j ) {
	DBounds b;
	bigreal len;
	SplineCharFindBounds(bats[j],&b);
	if ( is_horiz )
	    len = b.maxx;
	else
	    len = b.maxy;
	(*gvbase)->parts[j].component = copy(bats[j]->name);
	(*gvbase)->parts[j].is_extender = bats[j]==bits[3];
	(*gvbase)->parts[j].startConnectorLength = len/4;
	(*gvbase)->parts[j].endConnectorLength = len/4;
	(*gvbase)->parts[j].fullAdvance = len;
    }
}

#define BigEndianWord(pt) ((((uint8_t *) pt)[0]<<24) | (((uint8_t *) pt)[1]<<16) | (((uint8_t *) pt)[2]<<8) | (((uint8_t *) pt)[3]))

int LoadKerningDataFromTfm(SplineFont *sf, char *filename,EncMap *map) {
    FILE *file = fopen(filename,"rb");
    int i, tag, left, ictag;
    struct tfmdata tfmd;
    int charlist[256];
    int height, depth;
    real scale = (sf->ascent+sf->descent)/(bigreal) (1<<20);

    if ( file==NULL )
return( 0 );
    tfmd.file_len = getushort(file);
    tfmd.head_len = getushort(file);
    tfmd.first = getushort(file);
    tfmd.last = getushort(file);
    tfmd.width_size = getushort(file);
    tfmd.height_size = getushort(file);
    tfmd.depth_size = getushort(file);
    tfmd.italic_size = getushort(file);
    tfmd.ligkern_size = getushort(file);
    tfmd.kern_size = getushort(file);
    tfmd.esize = getushort(file);
    tfmd.param_size = getushort(file);
    if ( tfmd.first-1>tfmd.last || tfmd.last>=256 ) {
	fclose(file);
return( 0 );
    }
    tfmd.kerntab = calloc(tfmd.kern_size,4*sizeof(uint8_t));
    tfmd.ligkerntab = calloc(tfmd.ligkern_size,4*sizeof(uint8_t));
    tfmd.ext = calloc(tfmd.esize,4*sizeof(uint8_t));
    tfmd.ictab = calloc(tfmd.italic_size,4*sizeof(uint8_t));
    tfmd.dptab = calloc(tfmd.depth_size,4*sizeof(uint8_t));
    tfmd.httab = calloc(tfmd.height_size,4*sizeof(uint8_t));
    tfmd.widtab = calloc(tfmd.width_size,4*sizeof(uint8_t));
    tfmd.charlist = charlist;

    fseek( file,(6+1)*sizeof(int32_t),SEEK_SET);
    sf->design_size = (5*getlong(file)+(1<<18))>>19;	/* TeX stores as <<20, adobe in decipoints */
    fseek( file,
	    (6+tfmd.head_len+(tfmd.last-tfmd.first+1))*sizeof(int32_t),
	    SEEK_SET);
    fread( tfmd.widtab,1,tfmd.width_size*sizeof(int32_t),file);
    fread( tfmd.httab,1,tfmd.height_size*sizeof(int32_t),file);
    fread( tfmd.dptab,1,tfmd.depth_size*sizeof(int32_t),file);
    fread( tfmd.ictab,1,tfmd.italic_size*sizeof(int32_t),file);
    fread( tfmd.ligkerntab,1,tfmd.ligkern_size*sizeof(int32_t),file);
    fread( tfmd.kerntab,1,tfmd.kern_size*sizeof(int32_t),file);
    fread( tfmd.ext,1,tfmd.esize*sizeof(int32_t),file);
    for ( i=0; i<22 && i<tfmd.param_size; ++i )
	sf->texdata.params[i] = getlong(file);
    if ( tfmd.param_size==22 ) sf->texdata.type = tex_math;
    else if ( tfmd.param_size==13 ) sf->texdata.type = tex_mathext;
    else if ( tfmd.param_size>=7 ) sf->texdata.type = tex_text;

    memset(charlist,-1,sizeof(charlist));

    fseek( file, (6+tfmd.head_len)*sizeof(int32_t), SEEK_SET);
    for ( i=tfmd.first; i<=tfmd.last; ++i ) {
	/* width = */ getc(file);
	height = getc(file);
	depth = height&0xf; height >>= 4;
	ictag = getc(file);
	tag = (ictag&3);
	ictag>>=2;
	left = getc(file);
	if ( tag==1 )
	    tfmDoLigKern(sf,i,left,&tfmd,map);
	else if ( tag==2 )
	    charlist[i] = left;
	else if ( tag==3 )
	    tfmDoExten(sf,i,&tfmd,left,map);
	if ( map->map[i]!=-1 && sf->glyphs[map->map[i]]!=NULL ) {
	    SplineChar *sc = sf->glyphs[map->map[i]];
/* TFtoPL says very clearly: The actual width of a character is */
/*  width[width_index], in design-size units. It is not in design units */
/*  it is stored as a fraction of the design size in a fixed word */
	    if ( height<tfmd.height_size )
		sc->tex_height = BigEndianWord(tfmd.httab+4*height)*scale;
	    if ( depth<tfmd.depth_size )
		sc->tex_depth = BigEndianWord(tfmd.dptab+4*depth)*scale;
	    if ( ictag!=0 && ictag<tfmd.italic_size )
		sc->italic_correction = BigEndianWord(tfmd.ictab+4*ictag)*scale;
	}
    }

    for ( i=tfmd.first; i<=tfmd.last; ++i ) {
	if ( charlist[i]!=-1 )
	    tfmDoCharList(sf,i,&tfmd,map);
    }

    free( tfmd.ligkerntab); free(tfmd.kerntab); free(tfmd.ext); free(tfmd.ictab);
    free( tfmd.dptab ); free( tfmd.httab ); free( tfmd.widtab );
    fclose(file);
return( 1 );
}

/* ************************************************************************** */
/* **************************** Reading OFM files *************************** */
/* ************************************************************************** */
#define LKShort(index,off) (((tfmd->ligkerntab+8*index+2*off)[0]<<8)|(tfmd->ligkerntab+8*index+2*off)[1])

/* almost the same as the tfm version except that we deal with shorts rather */
/*  than bytes */
static void ofmDoLigKern(SplineFont *sf, int enc, int lk_index,
	struct tfmdata *tfmd,EncMap *map) {
    int enc2, k_index;
    SplineChar *sc1, *sc2, *sc3;
    real off;

    if ( enc>=map->enccount || map->map[enc]==-1 || (sc1=sf->glyphs[map->map[enc]])==NULL )
return;
    if ( enc<tfmd->first || enc>tfmd->last )
return;
    if ( lk_index>=2*tfmd->ligkern_size )
return;

    if ( LKShort(lk_index,0)>128 )		/* Special case to get big indeces */
	lk_index = 65536*LKShort(lk_index,2) + LKShort(lk_index,3);

    while ( 1 ) {
	if ( lk_index>=2*tfmd->ligkern_size )
return;
	enc2 = LKShort(lk_index,1);
	if ( enc2>=map->enccount || map->map[enc2]==-1 || (sc2=sf->glyphs[map->map[enc2]])==NULL ||
		enc2<tfmd->first || enc2>tfmd->last )
	    /* Not much we can do. can't kern to a non-existant char */;
	else if ( LKShort(lk_index,2)>=128 ) {
	    k_index = ((LKShort(lk_index,2)-128)*65536+LKShort(lk_index,3))*4;
	    if ( k_index>tfmd->kern_size )
return;
	    off = (sf->ascent+sf->descent) *
		    ((tfmd->kerntab[k_index]<<24) + (tfmd->kerntab[k_index+1]<<16) +
			(tfmd->kerntab[k_index+2]<<8) + tfmd->kerntab[k_index+3])/
		    (bigreal) 0x100000;
 /* printf( "%s(%d) %s(%d) -> %g\n", sc1->name, sc1->enc, sc2->name, sc2->enc, off); */
	    KPInsert(sc1,sc2,rint(off),false);
	} else if ( LKShort(lk_index,2)==0 &&
		LKShort(lk_index,3)<map->enccount &&
		LKShort(lk_index,3)>=tfmd->first &&
		LKShort(lk_index,3)<=tfmd->last &&
		map->map[LKShort(lk_index,3)]!=-1 &&
		(sc3=sf->glyphs[map->map[LKShort(lk_index,3)]])!=NULL ) {
	    LigatureNew(sc3,sc1,sc2);
	}
	if ( LKShort(lk_index,0)>=128 )
    break;
	lk_index += LKShort(lk_index,0) + 1;
    }
}
#undef LKShort


#define ExtShort(off) (((ext+2*off)[0]<<8)|(ext+2*off)[1])

/* almost the same as the tfm version except that we deal with shorts rather */
/*  than bytes */
static void ofmDoExten(SplineFont *sf,int i,struct tfmdata *tfmd,int left,EncMap *map) {
    int j, k, gid, gid2, cnt;
    uint8_t *ext;
    SplineChar *sc, *bits[4], *bats[4];
    struct glyphvariants **gvbase;
    int is_horiz;

    if ( i>=map->enccount || (gid=map->map[i])==-1 || sf->glyphs[gid]==NULL )
return;
    if ( left>=2*tfmd->esize )
return;

    ext = tfmd->ext + 4*left;

    sc = sf->glyphs[gid];
    if ( sc==NULL )
return;
    is_horiz = doesGlyphExpandHorizontally(sc);
    gvbase = is_horiz?&sc->horiz_variants: &sc->vert_variants;
    if ( *gvbase==NULL )
	*gvbase = chunkalloc(sizeof(struct glyphvariants));

    memset(bits,0,sizeof(bits));
    for ( j=0; j<4; ++j ) {
	k = ExtShort(j);
	if ( k!=0 && k<map->enccount && (gid2 = map->map[k])!=-1 && sf->glyphs[gid2]!=NULL ) {
	    bits[j] = sf->glyphs[gid2];
	    /* bits[j]->is_extended_shape = true;*/	/* MS does not do this in their fonts */
	}
    }
    /* 0=>bottom, 1=>middle, 2=>top, 3=>extender */
    cnt = 0;
    if ( bits[0]!=NULL )
	bats[cnt++] = bits[0];
    if ( bits[3]!=NULL )
	bats[cnt++] = bits[3];
    if ( bits[1]!=NULL ) {
	bats[cnt++] = bits[1];
	if ( bits[3]!=NULL )
	    bats[cnt++] = bits[3];
    }
    if ( bits[2]!=NULL )
	bats[cnt++] = bits[1];

    (*gvbase)->part_cnt = cnt;
    (*gvbase)->parts = calloc(cnt,sizeof(struct gv_part));
    for ( j=0; j<cnt; ++j ) {
	DBounds b;
	bigreal len;
	SplineCharFindBounds(bats[j],&b);
	if ( is_horiz )
	    len = b.maxx;
	else
	    len = b.maxy;
	(*gvbase)->parts[j].component = copy(bats[j]->name);
	(*gvbase)->parts[j].is_extender = bats[j]==bits[3];
	(*gvbase)->parts[j].startConnectorLength = len/4;
	(*gvbase)->parts[j].endConnectorLength = len/4;
	(*gvbase)->parts[j].fullAdvance = len;
    }
}

int LoadKerningDataFromOfm(SplineFont *sf, char *filename,EncMap *map) {
    FILE *file = fopen(filename,"rb");
    int i, tag, left, ictag;
    int level;
    int height, depth;
    real scale = (sf->ascent+sf->descent)/(bigreal) (1<<20);
    struct tfmdata tfmd;

    if ( file==NULL )
return( 0 );
    /* No one bothered to mention this in the docs, but there appears to be */
    /*  an initial 0 in an ofm file. I wonder if that is the level? */
    /* according to the ofm2opl source, it is the level */
    level = getlong(file);
    tfmd.file_len = getlong(file);
    tfmd.head_len = getlong(file);
    tfmd.first = getlong(file);
    tfmd.last = getlong(file);
    tfmd.width_size = getlong(file);
    tfmd.height_size = getlong(file);
    tfmd.depth_size = getlong(file);
    tfmd.italic_size = getlong(file);
    tfmd.ligkern_size = getlong(file);
    tfmd.kern_size = getlong(file);
    tfmd.esize = getlong(file);
    tfmd.param_size = getlong(file);
    /* font_dir = */ getlong(file);
    if ( tfmd.first-1>tfmd.last || tfmd.last>=65536 ) {
	fclose(file);
return( 0 );
    }
    if ( tfmd.file_len!=14+tfmd.head_len+2*(tfmd.last-tfmd.first+1)+tfmd.width_size+tfmd.height_size+tfmd.depth_size+
	    tfmd.italic_size+2*tfmd.ligkern_size+tfmd.kern_size+2*tfmd.esize+tfmd.param_size || level!=0 ) {
	int ncw, nki, nwi, nkf, nwf, nkm,nwm, nkr, nwr, nkg, nwg, nkp, nwp;
	int level1=0;
	/* nco = */ getlong(file);
	ncw = getlong(file);
	/* npc = */ getlong(file);
	nki = getlong(file);
	nwi = getlong(file);
	nkf = getlong(file);
	nwf = getlong(file);
	nkm = getlong(file);
	nwm = getlong(file);
	nkr = getlong(file);
	nwr = getlong(file);
	nkg = getlong(file);
	nwg = getlong(file);
	nkp = getlong(file);
	nwp = getlong(file);
	level1 = tfmd.file_len==29+tfmd.head_len+ncw+tfmd.width_size+tfmd.height_size+tfmd.depth_size+
	    tfmd.italic_size+2*tfmd.ligkern_size+tfmd.kern_size+2*tfmd.esize+tfmd.param_size +
	    nki + nwi + nkf + nwf + nkm + nwm + nkr + nwr + nkg + nwg + nkp + nwp;
	/* Level 2 appears to have the same structure as level 1 */
	if ( level1 || level==1 || level==2 )
	    ff_post_error(_("Unlikely Ofm File"),_("This looks like a level1 (or level2) ofm. FontForge only supports level0 files, and can't read a real level1 file."));
	else
	    ff_post_error(_("Unlikely Ofm File"),_("This doesn't look like an ofm file, I don't know how to read it."));
	fclose(file);
return( 0 );
    }

    tfmd.kerntab = calloc(tfmd.kern_size,4*sizeof(uint8_t));
    tfmd.ligkerntab = calloc(tfmd.ligkern_size,4*2*sizeof(uint8_t));
    tfmd.ext = calloc(tfmd.esize,4*2*sizeof(uint8_t));
    tfmd.ictab = calloc(tfmd.italic_size,4*sizeof(uint8_t));
    tfmd.dptab = calloc(tfmd.depth_size,4*sizeof(uint8_t));
    tfmd.httab = calloc(tfmd.height_size,4*sizeof(uint8_t));
    tfmd.widtab = calloc(tfmd.width_size,4*sizeof(uint8_t));
    fseek( file,(14+1)*sizeof(int32_t),SEEK_SET);
    sf->design_size = (5*getlong(file)+(1<<18))>>19;	/* TeX stores as <<20, adobe in decipoints */
    fseek( file,
	    (14+tfmd.head_len+2*(tfmd.last-tfmd.first+1))*sizeof(int32_t),
	    SEEK_SET);
    fread( tfmd.widtab,1,tfmd.width_size*sizeof(int32_t),file);
    fread( tfmd.httab,1,tfmd.height_size*sizeof(int32_t),file);
    fread( tfmd.dptab,1,tfmd.depth_size*sizeof(int32_t),file);
    fread( tfmd.ictab,1,tfmd.italic_size*sizeof(int32_t),file);
    fread( tfmd.ligkerntab,1,tfmd.ligkern_size*2*sizeof(int32_t),file);
    fread( tfmd.kerntab,1,tfmd.kern_size*sizeof(int32_t),file);
    fread( tfmd.ext,1,tfmd.esize*2*sizeof(int32_t),file);
    for ( i=0; i<22 && i<tfmd.param_size; ++i )
	sf->texdata.params[i] = getlong(file);
    if ( tfmd.param_size==22 ) sf->texdata.type = tex_math;
    else if ( tfmd.param_size==13 ) sf->texdata.type = tex_mathext;
    else if ( tfmd.param_size>=7 ) sf->texdata.type = tex_text;

    tfmd.charlist = malloc(65536*sizeof(int32_t));
    memset(tfmd.charlist,-1,65536*sizeof(int32_t));

    fseek( file, (14+tfmd.head_len)*sizeof(int32_t), SEEK_SET);
    for ( i=tfmd.first; i<=tfmd.last; ++i ) {
	/* width = */ getushort(file);
	height = getc(file);
	depth = getc(file);
	ictag = getc(file);
	tag = getc(file)&0x3;		/* Remaining 6 bytes are "reserved for future use" I think */
	left = getushort(file);
	if ( tag==1 )
	    ofmDoLigKern(sf,i,left,&tfmd,map);
	else if ( tag==2 )
	    tfmd.charlist[i] = left;
	else if ( tag==3 )
	    ofmDoExten(sf,i,&tfmd,left,map);
	if ( map->map[i]!=-1 && sf->glyphs[map->map[i]]!=NULL ) {
	    SplineChar *sc = sf->glyphs[map->map[i]];
/* TFtoPL says very clearly: The actual width of a character is */
/*  width[width_index], in design-size units. It is not in design units */
/*  it is stored as a fraction of the design size in a fixed word */
	    if ( height<tfmd.height_size )
		sc->tex_height = BigEndianWord(tfmd.httab+4*height)*scale;
	    if ( depth<tfmd.depth_size )
		sc->tex_depth = BigEndianWord(tfmd.dptab+4*depth)*scale;
	    if ( ictag!=0 && ictag<tfmd.italic_size )
		sc->italic_correction = BigEndianWord(tfmd.ictab+4*ictag)*scale;
	}
    }

    for ( i=tfmd.first; i<=tfmd.last; ++i ) {
	if ( tfmd.charlist[i]!=-1 )
	    tfmDoCharList(sf,i,&tfmd,map);
    }

    free( tfmd.ligkerntab); free(tfmd.kerntab); free(tfmd.ext); free(tfmd.ictab);
    free( tfmd.dptab ); free( tfmd.httab ); free( tfmd.widtab );
    free( tfmd.charlist );
    fclose(file);
return( 1 );
}
/* ************************************************************************** */

const char *EncodingName(Encoding *map) {
    const char *name = map->iconv_name != NULL ? map->iconv_name : map->enc_name;
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

static void AfmLigOut(FILE *afm, SplineChar *sc) {
    LigList *ll;
    PST *lig;
    char *pt, *eos;

    for ( ll=sc->ligofme; ll!=NULL; ll=ll->next ) {
	lig = ll->lig;
	if ( !lig->subtable->lookup->store_in_afm )
    continue;
	pt = strchr(lig->u.lig.components,' ');
	eos = strrchr(lig->u.lig.components,' ');
	if ( pt!=NULL && eos==pt )
	    /* AFM files don't seem to support 3 (or more) letter combos */
	    fprintf( afm, " L %s %s ;", pt+1, lig->u.lig.lig->name );
    }
}

static void AfmSplineCharX(FILE *afm, SplineChar *sc, int enc, int layer) {
    DBounds b;
    int em = (sc->parent->ascent+sc->parent->descent);

    SplineCharLayerFindBounds(sc,layer,&b);
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
    ff_progress_next();
}

static void AfmZapfCharX(FILE *afm, int zi) {

    /*fprintf( afm, "CX <%04x> ; WX %d ; N %s ; B %d %d %d %d ;\n",*/
    fprintf( afm, "C %d ; WX %d ; N %s ; B %d %d %d %d ;\n",
	    0x2700+zi, zapfwx[zi], zapfnomen[zi],
	    zapfbb[zi][0], zapfbb[zi][1], zapfbb[zi][2], zapfbb[zi][3]);
}

static void AfmSplineChar(FILE *afm, SplineChar *sc, int enc,int layer) {
    DBounds b;
    int em = (sc->parent->ascent+sc->parent->descent);

    SplineCharLayerFindBounds(sc,layer,&b);
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
    ff_progress_next();
}

static void AfmCIDChar(FILE *afm, SplineChar *sc, int enc,int layer) {
    DBounds b;
    int em = (sc->parent->ascent+sc->parent->descent);

    SplineCharLayerFindBounds(sc,layer,&b);
    fprintf( afm, "C -1 ; WX %d ; ", sc->width*1000/em );
    if ( sc->parent->hasvmetrics )
	fprintf( afm, "WY %d ; ", sc->vwidth*1000/em );
    fprintf( afm, "N %d ; B %d %d %d %d ;",
	    enc,
	    (int) floor(b.minx*1000/em), (int) floor(b.miny*1000/em),
	    (int) ceil(b.maxx*1000/em), (int) ceil(b.maxy*1000/em) );
    putc('\n',afm);
    ff_progress_next();
}

static int anykerns(SplineFont *sf,int isv) {
    int i, cnt = 0;
    KernPair *kp;

    for ( i=0; i<sf->glyphcnt; ++i ) {
	if ( sf->glyphs[i]!=NULL && strcmp(sf->glyphs[i]->name,".notdef")!=0 ) {
	    for ( kp = isv ? sf->glyphs[i]->vkerns : sf->glyphs[i]->kerns; kp!=NULL; kp = kp->next )
		if ( kp->off!=0 && strcmp(kp->sc->name,".notdef")!=0 &&
			(kp->sc->parent==sf || sf->cidmaster!=NULL) )
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
	if ( kp->sc->parent!=sc->parent && sc->parent->cidmaster==NULL )
    continue;		/* Can happen when saving multiple pfbs */
	if ( strcmp(kp->sc->name,".notdef")!=0 && kp->off!=0 ) {
	    if ( isv )
		fprintf( afm, "KPY %s %s %d\n", sc->name, kp->sc->name, kp->off*1000/em );
	    else
		fprintf( afm, "KPX %s %s %d\n", sc->name, kp->sc->name, kp->off*1000/em );
	}
    }
}

int SFFindNotdef(SplineFont *sf, int fixed) {
    int notdefpos = -1, i, width=-1;
    /* If all glyphs have the same known width set fixed to it */
    /* If glyphs are proportional set fixed to -1 */
    /* If we don't know yet, set fixed to -2 */

    if ( fixed==-2 ) {		/* Unknown */
	for ( i=0; i<sf->glyphcnt; ++i ) if ( SCWorthOutputting(sf->glyphs[i]) ) {
	    if ( strcmp(sf->glyphs[i]->name,".notdef")==0 ) {
		if ( notdefpos==-1 ) notdefpos = i;
	    } else if ( width==-1 )
		width = sf->glyphs[i]->width;
	    else if ( width!=sf->glyphs[i]->width )
		width = -2;
	}
	if ( width>=0 && sf->glyphcnt>2 && notdefpos>=0) {
	    if ( width!=sf->glyphs[notdefpos]->width ) {
		notdefpos = -1;
		for ( i=0; i<sf->glyphcnt; ++i ) if ( SCWorthOutputting(sf->glyphs[i]) ) {
		    if ( strcmp(sf->glyphs[i]->name,".notdef")==0 &&
			    sf->glyphs[i]->width == width ) {
			notdefpos = i;
		break;
		    }
		}
	    }
	}
    } else if ( fixed>=0 ) {		/* Known, fixed width */
	for ( i=0; i<sf->glyphcnt; ++i ) if ( SCWorthOutputting(sf->glyphs[i]) ) {
	    if ( strcmp(sf->glyphs[i]->name,".notdef")==0 &&
		    sf->glyphs[i]->width == fixed )
return( i );
	}
    } else {				/* Known, variable width */
	for ( i=0; i<sf->glyphcnt; ++i ) if ( SCWorthOutputting(sf->glyphs[i]) ) {
	    if ( strcmp(sf->glyphs[i]->name,".notdef")==0 )
return( i );
	}
    }

return( notdefpos );
}

int SCDrawsSomething(SplineChar *sc) {
    int layer,l;
    RefChar *ref;

    if ( sc==NULL )
return( false );
    for ( layer = 0; layer<sc->layer_cnt; ++layer ) if ( !sc->layers[layer].background ) {
	if ( sc->layers[layer].splines!=NULL || sc->layers[layer].images!=NULL )
return( true );
	for ( ref = sc->layers[layer].refs; ref!=NULL; ref=ref->next )
	    for ( l=0; l<ref->layer_cnt; ++l )
		if ( ref->layers[l].splines!=NULL )
return( true );
    }
return( false );
}

int SCDrawsSomethingOnLayer(SplineChar *sc, int layer) {
    int l;
    RefChar *ref;

    if ( sc==NULL )
return( false );
    if (layer<sc->layer_cnt) {
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
	    sc->dependents!=NULL /*||
	    sc->width!=sc->parent->ascent+sc->parent->descent*/ ) );
}

int SCHasData(SplineChar *sc) {
    int layer;

    if ( sc==NULL )
return( false );
    for ( layer = 0; layer<sc->layer_cnt; ++layer ) if ( sc->layers[layer].python_persistent ) return true;
    return false;
}

int LayerWorthOutputting(SplineFont *sf, int layer) {
  int glyphpos = 0;
  for (glyphpos = 0; glyphpos < sf->glyphcnt; glyphpos++) {
    if (SCDrawsSomethingOnLayer(sf->glyphs[glyphpos], layer)) return 1;
  }
  return 0;
}

int SCLWorthOutputtingOrHasData(SplineChar *sc, int layer) {
  if (sc == NULL) return 0;
  if (layer >= sc->layer_cnt) return 0;
  if (SCDrawsSomethingOnLayer(sc, layer)) return 1;
  if (sc->layers[layer].python_persistent) return 1;
  return 0;
}

int CIDWorthOutputting(SplineFont *cidmaster, int enc) {
    int i;

    if ( enc<0 )
return( -1 );

    if ( cidmaster->subfontcnt==0 )
return( enc>=cidmaster->glyphcnt?-1:SCWorthOutputting(cidmaster->glyphs[enc])?0:-1 );

    for ( i=0; i<cidmaster->subfontcnt; ++i )
	if ( enc<cidmaster->subfonts[i]->glyphcnt &&
		SCWorthOutputting(cidmaster->subfonts[i]->glyphs[enc]))
return( i );

return( -1 );
}

static void AfmSplineFontHeader(FILE *afm, SplineFont *sf, int formattype,
	EncMap *map, SplineFont *fullsf,int layer) {
    DBounds b;
    real width;
    int i, j, cnt, max;
    bigreal caph, xh, ash, dsh;
    int iscid = ( formattype==ff_cid || formattype==ff_otfcid );
    int ismm = ( formattype==ff_mma || formattype==ff_mmb );
    time_t now;
    SplineChar *sc;
    int em = (sf->ascent+sf->descent);

    if ( iscid && sf->cidmaster!=NULL ) sf = sf->cidmaster;

    max = sf->glyphcnt;
    if ( iscid ) {
	max = 0;
	for ( i=0; i<sf->subfontcnt; ++i )
	    if ( sf->subfonts[i]->glyphcnt>max )
		max = sf->subfonts[i]->glyphcnt;
    }
    caph = SFCapHeight(sf,layer,true);
    xh   = SFXHeight(sf,layer,true);
    ash  = SFAscender(sf,layer,true);
    dsh  = SFDescender(sf,layer,true);
    cnt = 0;
    for ( i=0; i<max; ++i ) {
	sc = NULL;
	if ( iscid ) {
	    for ( j=0; j<sf->subfontcnt; ++j )
		if ( i<sf->subfonts[j]->glyphcnt && sf->subfonts[j]->glyphs[i]!=NULL ) {
		    sc = sf->subfonts[j]->glyphs[i];
	    break;
		}
	} else
	    sc = sf->glyphs[i];
	if ( sc!=NULL ) {
	    if ( SCWorthOutputting(sc) || (iscid && i==0 && sc!=NULL))
		++cnt;
	}
    }

    fprintf( afm, ismm ? "StartMasterFontMetrics 4.0\n" :
		  iscid ? "StartFontMetrics 4.1\n" :
			  "StartFontMetrics 2.0\n" );
    fprintf( afm, "Comment Generated by FontForge %s\n", FONTFORGE_VERSION );
    now = GetTime();
    fprintf(afm,"Comment Creation Date: %s", ctime(&now));
    fprintf( afm, "FontName %s\n", sf->fontname );
    if ( sf->fullname!=NULL ) fprintf( afm, "FullName %s\n", sf->fullname );
    if ( sf->familyname!=NULL ) fprintf( afm, "FamilyName %s\n", sf->familyname );
    if ( sf->weight!=NULL ) fprintf( afm, "Weight %s\n", sf->weight );
    /* AFM lines are limited to 256 characters and US ASCII */
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
	fprintf( afm, "Version %g\n", (double)sf->cidversion );
	fprintf( afm, "CharacterSet %s-%s-%d\n", sf->cidregistry, sf->ordering, sf->supplement );
	fprintf( afm, "IsBaseFont true\n" );
	fprintf( afm, "IsCIDFont true\n" );
    }
    fprintf( afm, "ItalicAngle %g\n", (double) sf->italicangle );
    width = CIDOneWidth(sf);
    fprintf( afm, "IsFixedPitch %s\n", width==-1?"false":"true" );
    fprintf( afm, "UnderlinePosition %g\n", (double) sf->upos );
    fprintf( afm, "UnderlineThickness %g\n", (double) sf->uwidth );
    if ( !iscid ) {
	if ( sf->version!=NULL ) fprintf( afm, "Version %s\n", sf->version );
	fprintf( afm, "EncodingScheme %s\n", EncodingName(map->enc));
    }
    if ( iscid ) CIDLayerFindBounds(sf,layer,&b); else SplineFontLayerFindBounds(fullsf!=NULL?fullsf:sf,layer,&b);
    fprintf( afm, "FontBBox %d %d %d %d\n",
	    (int) floor(b.minx*1000/em), (int) floor(b.miny*1000/em),
	    (int) ceil(b.maxx*1000/em), (int) ceil(b.maxy*1000/em) );
    if ( caph!=-1e23 )
	fprintf( afm, "CapHeight %d\n", (int) rint(caph*1000/em) );
    if ( xh!=-1e23 )
	fprintf( afm, "XHeight %d\n", (int) rint(xh*1000/em) );
    if ( ash!=-1e23 )
	fprintf( afm, "Ascender %d\n", (int) rint(ash*1000/em) );
    if ( dsh!=1e23 )
	fprintf( afm, "Descender %d\n", (int) rint(dsh*1000/em) );
}

struct cc_accents {
    SplineChar *accent;
    real xoff, yoff;
    struct cc_accents *next;
};

struct cc_data {
    char *name;
    SplineChar *base;
    int acnt;
    struct cc_accents *accents;
};

struct cc_container {
    struct cc_data *ccs;
    int cnt, max;	 /* total number of ccs used/allocated */
    SplineChar ***marks; /* For each ac this is a list of all mark glyphs in it */
    int *mcnt;		 /* Count of the above list */
    int *mpos;
    SplineFont *sf;
};

#define AC_MAX	5	/* At most 5 Anchor classes may be used/glyph */

static int FigureName(int *unicode,const char *name,int u) {
    char *upt, *start, *end, ch;

    start = copy(name);
    if ( strchr(start,'_')!=NULL ) {
	while ( (upt=strchr(start,'_'))!=NULL ) {
	    *upt='\0';
	    u = FigureName(unicode,start,u);
	    *upt = '_';
	    if ( u==-1 )
return( -1 );
	    start = upt+1;
	}
    }
    if ( strncmp(start,"uni",3)==0 && (strlen(start)-3)%4==0 ) {
	start+=3;
	while ( *start ) {
	    ch = start[4]; start[4] = '\0';
	    unicode[u++] = strtol(start,&end,16);
	    start[4] = ch;
	    if ( *end!='\0' )
return( -1 );
	    start += 4;
	}
return( u );
    }
    unicode[u] = UniFromName(start,ui_none,&custom);
    if ( unicode[u]==-1 )
return( -1 );

return( u+1 );
}

static int FigureUnicodes(int *unicode,SplineChar *sc,int u) {
    const unichar_t *upt;

    if ( u==-1 )
return( -1 );
    if ( sc->unicodeenc==-1 )
return( FigureName(unicode,sc->name,u));
    if ( isdecompositionnormative(sc->unicodeenc) &&
	    (upt = unialt(sc->unicodeenc))!=NULL ) {
	while ( *upt!='\0' )
	    unicode[u++] = *upt++;
    } else
	unicode[u++] = sc->unicodeenc;
return( u );
}

static int FindDecomposition(int *unicode, int u) {
    int uni;
    const unichar_t *upt;
    int i;

    for ( uni=0; uni<0x10000; ++uni ) {
	if ( (upt = unialt(uni))!=NULL ) {
	    for ( i=0; *upt!='\0' && i<u && *upt==(unichar_t)unicode[i]; ++i, ++upt );
	    if ( *upt=='\0' && i==u )
return( uni );
	}
    }
return( -1 );
}

static void sortunis(int *unicode,int u) {
    int i, j;

    for ( i=0; i<u; ++i ) {
	unicode[i] = CanonicalCombiner(unicode[i]);
    }

    for ( i=0; i<u; ++i ) for ( j=i+1; j<u; ++j ) {
	if ( unicode[i]>unicode[j] ) {
	    int temp = unicode[i];
	    unicode[i] = unicode[j];
	    unicode[j] = temp;
	}
    }
}

static char *NameFrom(struct cc_data *this,int *unicode,int u,int uni) {
    char *ret, *pt;
    char buffer[60];
    struct cc_accents *cca;
    int i,len;

    if ( uni!=-1 )
return( copy( StdGlyphName(buffer,uni,ui_none,NULL)) );
    if ( u!=-1 && (unicode[0]<0x370 || unicode[0]>0x3ff) ) {
	/* Don't use the unicode decomposition to get a name for greek */
	/*  glyphs. We'd get acute for tonos, etc. */
	ret = malloc(4+4*u);
	strcpy(ret,"uni");
	pt = ret+3;
	for ( i=0; i<u; ++i ) {
	    sprintf( pt, "%04X", unicode[i] );
	    pt += 4;
	}
return( ret );
    }
    len = strlen( this->base->name ) +1;
    for ( cca = this->accents; cca!=NULL; cca = cca->next )
	len += strlen( cca->accent->name ) +1;
    ret = malloc(len);
    strcpy(ret,this->base->name);
    pt = ret + strlen(ret);
    for ( cca = this->accents; cca!=NULL; cca = cca->next ) {
	*pt++ = '_';
	strcpy(pt,cca->accent->name);
	pt = pt + strlen(pt);
    }
return( ret );
}

static int AfmBuildCCName(struct cc_data *this,struct cc_container *cc) {
    int unicode[4*AC_MAX];
    int u;
    int uni;
    struct cc_accents *cca;

    u = FigureUnicodes(unicode,this->base,0);
    for ( cca = this->accents; cca!=NULL; cca = cca->next )
	u=FigureUnicodes(unicode,cca->accent,u);
    if ( u!=-1 ) {
	unicode[u]= -1;
	if ( unicode[0]==0x131 ) unicode[0] = 'i';
	if ( unicode[0]==0x237 || unicode[0]==0xf6be ) unicode[0] = 'j';
	sortunis(unicode+1,u-1);		/* Only sort the accents */
    }
    uni = FindDecomposition(unicode,u);
    if ( uni!=-1 )
	if ( SFGetChar(cc->sf,uni,NULL)!=NULL )
return( false );		/* Character already exists in font */
    this->name = NameFrom(this,unicode,u,uni);
    if ( SFGetChar(cc->sf,-1,this->name)!=NULL ) {
	free(this->name);
return( false );		/* Character already exists in font */
    }
return( true );
}

static void AfmBuildMarkCombos(SplineChar *sc,AnchorPoint *ap, struct cc_container *cc) {
    if ( ap==NULL ) {
	AnchorPoint *ap;
	struct cc_data *this = &cc->ccs[cc->cnt++];
	int acnt=0;
	AnchorPoint *map;

	this->base = sc;
	this->accents = NULL;
	for ( ap=sc->anchor; ap!=NULL; ap=ap->next ) if ( ap->ticked ) {
	    struct cc_accents *cca = chunkalloc(sizeof(struct cc_accents));
	    cca->accent = cc->marks[ap->anchor->ac_num][cc->mpos[ap->anchor->ac_num]];
	    for ( map = cca->accent->anchor; map->anchor!=ap->anchor || map->type!=at_mark;
		    map = map->next );
	    if ( map!=NULL ) {
		cca->xoff = ap->me.x - map->me.x;
		cca->yoff = ap->me.y - map->me.y;
	    }
	    cca->next = this->accents;
	    this->accents = cca;
	    ++acnt;
	}
	if ( !AfmBuildCCName(this,cc)) {
	    struct cc_accents *cca, *next;
	    --cc->cnt;
	    for ( cca = this->accents; cca!=NULL; cca = next ) {
		next = cca->next;
		chunkfree(cca,sizeof(struct cc_accents));
	    }
	} else
	    this->acnt = acnt;
    } else if ( ap->ticked ) {
	int ac_num = ap->anchor->ac_num;
	for ( cc->mpos[ac_num] = 0; cc->mpos[ac_num]<cc->mcnt[ac_num]; ++cc->mpos[ac_num] )
	    AfmBuildMarkCombos(sc,ap->next,cc);
    } else {
	AfmBuildMarkCombos(sc,ap->next,cc);
    }
}

static void AfmBuildCombos(SplineChar *sc,AnchorPoint *ap, struct cc_container *cc) {

    if ( ap!=NULL ) {
	AfmBuildCombos(sc,ap->next,cc);
	if ( ap->type==at_basechar ) {
	    ap->ticked = true;
	    AfmBuildCombos(sc,ap->next,cc);
	    ap->ticked = false;
	}
    } else {
	int ticks, cnt;
	AnchorPoint *ap;

	for ( ap=sc->anchor, ticks=0, cnt=1; ap!=NULL; ap=ap->next ) {
	    if ( ap->ticked ) {
		++ticks;
		cnt *= cc->mcnt[ap->anchor->ac_num];
	    }
	}
	if ( ticks==0 )		  /* No accents classes selected */
return;
	if ( ticks>AC_MAX || cnt>200 ) /* Too many selected. I fear combinatorial explosion */
return;
	if ( cc->cnt+cnt >= cc->max )
	    cc->ccs = realloc(cc->ccs,(cc->max += cnt+200)*sizeof(struct cc_data));
	AfmBuildMarkCombos(sc,sc->anchor,cc);
    }
}

static struct cc_data *AfmFigureCCdata(SplineFont *sf,int *total) {
    int ac_cnt, i;
    AnchorClass *ac;
    AnchorPoint *ap;
    SplineChar *sc;
    int *mmax;
    struct cc_container cc;

    memset(&cc,0,sizeof(cc));
    cc.sf = sf;
    for ( ac=sf->anchor, ac_cnt=0; ac!=NULL; ac=ac->next, ++ac_cnt)
	ac->ac_num = ac_cnt;
    cc.mcnt = calloc(ac_cnt,sizeof(int));
    cc.mpos = calloc(ac_cnt,sizeof(int));
    mmax = calloc(ac_cnt,sizeof(int));
    cc.marks = calloc(ac_cnt,sizeof(SplineChar **));
    for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc = sf->glyphs[i])!=NULL ) {
	for ( ap = sc->anchor; ap!=NULL; ap=ap->next ) if ( ap->type==at_mark )
	    ++mmax[ap->anchor->ac_num];
    }
    for ( i=0; i<ac_cnt; ++i )
	cc.marks[i] = calloc(mmax[i],sizeof(SplineChar *));
    for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc = sf->glyphs[i])!=NULL ) {
	for ( ap = sc->anchor; ap!=NULL; ap=ap->next ) if ( ap->type==at_mark )
	    cc.marks[ap->anchor->ac_num][cc.mcnt[ap->anchor->ac_num]++] = sc;
    }
    for ( i=0; i<sf->glyphcnt; ++i )
	if ( (sc = sf->glyphs[i])!=NULL && sc->anchor!=NULL ) {
	    for ( ap = sc->anchor; ap!=NULL; ap=ap->next )
		ap->ticked = false;
	    AfmBuildCombos(sc,sc->anchor,&cc);
	}
    if ( cc.cnt+1 >= cc.max )
	cc.ccs = realloc(cc.ccs,(cc.max += 1)*sizeof(struct cc_data));
    cc.ccs[cc.cnt].base = NULL;		/* End of list mark */
    for ( i=0; i<ac_cnt; ++i )
	free(cc.marks[i]);
    free(cc.marks);
    free(cc.mcnt);
    free(cc.mpos);
    free(mmax);
    *total = cc.cnt;
return( cc.ccs );
}

static void CCFree(struct cc_data *cc) {
    int i;
    struct cc_accents *cca, *next;

    if ( cc==NULL )
return;
    for ( i=0; cc[i].base!=NULL; ++i ) {
	free(cc[i].name);
	for ( cca = cc[i].accents; cca!=NULL; cca = next ) {
	    next = cca->next;
	    chunkfree(cca,sizeof(struct cc_accents));
	}
    }
    free( cc );
}

static void AfmComposites(FILE *afm, SplineFont *sf, struct cc_data *cc, int cc_cnt) {
    int i;
    struct cc_accents *cca;
    bigreal em = (sf->ascent+sf->descent);

    fprintf( afm, "StartComposites %d\n", cc_cnt );
    for ( i=0; i<cc_cnt; ++i ) {
	fprintf( afm, "CC %s %d; PCC %s 0 0;",
		cc[i].name, cc[i].acnt+1, cc[i].base->name );
	for ( cca = cc[i].accents; cca!=NULL; cca = cca->next ) {
	    fprintf( afm, " PCC %s %g %g;",
		    cca->accent->name, rint(1000.0*cca->xoff/em), rint(1000.0*cca->yoff/em) );
	}
	putc('\n',afm);
    }
    fprintf( afm, "EndComposites\n" );
}

static void AfmCompositeChar(FILE *afm,struct cc_data *cc,int layer) {
    DBounds b, b1;
    SplineChar *sc = cc->base;
    SplineFont *sf = sc->parent;
    int em = (sf->ascent+sf->descent);
    struct cc_accents *cca;

    SplineCharLayerFindBounds(sc,layer,&b);
    for ( cca = cc->accents; cca!=NULL; cca = cca->next ) {
	SplineCharLayerFindBounds(cca->accent,layer,&b1);
	if ( b1.minx+cca->xoff<b.minx ) b.minx = b1.minx+cca->xoff;
	if ( b1.maxx+cca->xoff>b.maxx ) b.maxx = b1.maxx+cca->xoff;
	if ( b1.miny+cca->yoff<b.miny ) b.miny = b1.miny+cca->yoff;
	if ( b1.maxy+cca->yoff>b.maxy ) b.maxy = b1.maxy+cca->yoff;
    }
    fprintf( afm, "C %d ; WX %d ; ", -2, sc->width*1000/em );
    if ( sf->hasvmetrics )
	fprintf( afm, "WY %d ; ", sc->vwidth*1000/em );
    fprintf( afm, "N %s ; B %d %d %d %d ;",
	    cc->name,
	    (int) floor(b.minx*1000/em), (int) floor(b.miny*1000/em),
	    (int) ceil(b.maxx*1000/em), (int) ceil(b.maxy*1000/em) );
    putc('\n',afm);
}

static void LigatureClosure(SplineFont *sf);

int AfmSplineFont(FILE *afm, SplineFont *sf, int formattype,EncMap *map,
	int docc, SplineFont *fullsf, int layer) {
    int i, j, cnt, vcnt;
    int type0 = ( formattype==ff_ptype0 );
    int otf = (formattype==ff_otf);
    int encmax=(!type0 && !otf)?256:65536;
    int anyzapf;
    int iscid = ( formattype==ff_cid || formattype==ff_otfcid );
    SplineChar *sc;
    int cc_cnt = 0;
    struct cc_data *cc= NULL;

    SFLigaturePrepare(sf);
    LigatureClosure(sf);		/* Convert 3 character ligs to a set of two character ones when possible */
    SFKernClassTempDecompose(sf,false);	/* Undoes kern classes */
    SFKernClassTempDecompose(sf,true);
    if ( sf->anchor!=NULL && docc )
	cc = AfmFigureCCdata(sf,&cc_cnt);

    if ( iscid && sf->cidmaster!=NULL ) sf = sf->cidmaster;

    AfmSplineFontHeader(afm,sf,formattype,map,fullsf,layer);

    cnt = 0;
    for ( i=0; i<map->enccount; ++i ) {
	int gid = map->map[i];
	if ( gid==-1 )
    continue;
	sc = NULL;
	if ( iscid ) {
	    for ( j=0; j<sf->subfontcnt; ++j )
		if ( gid<sf->subfonts[j]->glyphcnt && sf->subfonts[j]->glyphs[gid]!=NULL ) {
		    sc = sf->subfonts[j]->glyphs[gid];
	    break;
		}
	} else
	    sc = sf->glyphs[gid];
	if ( SCWorthOutputting(sc) || (iscid && i==0 && sc!=NULL))
	    ++cnt;
    }

    anyzapf = false;
    if ( type0 && (map->enc->is_unicodebmp ||
	    map->enc->is_unicodefull)) {
	for ( i=0x2700; i<map->enccount && i<encmax && i<=0x27ff; ++i ) {
	    int gid = map->map[i];
	    if ( gid!=-1 && SCWorthOutputting(sf->glyphs[gid]) )
		anyzapf = true;
	}
	if ( !anyzapf )
	    for ( i=0; i<0xc0; ++i )
		if ( zapfexists[i])
		    ++cnt;
    }

    fprintf( afm, "StartCharMetrics %d\n", cnt+cc_cnt );
    if ( iscid ) {
	for ( i=0; i<map->enccount; ++i ) {
	    int gid = map->map[i];
	    if ( gid==-1 )
	continue;
	    sc = NULL;
	    for ( j=0; j<sf->subfontcnt; ++j )
		if ( gid<sf->subfonts[j]->glyphcnt && sf->subfonts[j]->glyphs[gid]!=NULL ) {
		    sc = sf->subfonts[j]->glyphs[gid];
	    break;
		}
	    if ( SCWorthOutputting(sc) || (i==0 && sc!=NULL) )
		AfmCIDChar(afm, sc, i,layer);
	}
    } else if ( type0 && (map->enc->is_unicodebmp ||
	    map->enc->is_unicodefull)) {
	for ( i=0; i<map->enccount && i<encmax && i<0x2700; ++i ) {
	    int gid = map->map[i];
	    if ( gid!=-1 && SCWorthOutputting(sf->glyphs[gid]) ) {
		AfmSplineCharX(afm,sf->glyphs[gid],i,layer);
	    }
	}
	if ( !anyzapf ) {
	    for ( i=0; i<0xc0; ++i ) if ( zapfexists[i] )
		AfmZapfCharX(afm,i);
	    i += 0x2700;
	}
	for ( ; i<map->enccount && i<encmax; ++i ) {
	    int gid = map->map[i];
	    if ( gid!=-1 && SCWorthOutputting(sf->glyphs[gid]) ) {
		AfmSplineCharX(afm,sf->glyphs[gid],i,layer);
	    }
	}
    } else if ( type0 ) {
	for ( i=0; i<map->enccount && i<encmax; ++i ) {
	    int gid = map->map[i];
	    if ( gid!=-1 && SCWorthOutputting(sf->glyphs[gid]) ) {
		AfmSplineCharX(afm,sf->glyphs[gid],i,layer);
	    }
	}
    } else {
	for ( i=0; i<map->enccount && i<encmax; ++i ) {
	    int gid = map->map[i];
	    if ( gid!=-1 && SCWorthOutputting(sf->glyphs[gid]) ) {
		AfmSplineChar(afm,sf->glyphs[gid],i,layer);
	    }
	}
    }
    for ( ; i<map->enccount ; ++i ) {
	int gid = map->map[i];
	if ( gid!=-1 && SCWorthOutputting(sf->glyphs[gid]) ) {
	    AfmSplineChar(afm,sf->glyphs[gid],-1,layer);
	}
    }
    for ( i=0; i<cc_cnt; ++i )
	AfmCompositeChar(afm,&cc[i],layer);
    fprintf( afm, "EndCharMetrics\n" );
    vcnt = anykerns(sf,true);
    if ( (cnt = anykerns(sf,false))>0 || vcnt>0 ) {
	fprintf( afm, "StartKernData\n" );
	if ( cnt>0 ) {
	    fprintf( afm, "StartKernPairs%s %d\n", vcnt==0?"":"0", cnt );
	    for ( i=0; i<map->enccount ; ++i ) {
		int gid = map->map[i];
		if ( gid!=-1 && SCWorthOutputting(sf->glyphs[gid]) ) {
		    AfmKernPairs(afm,sf->glyphs[gid],false);
		}
	    }
	    fprintf( afm, "EndKernPairs\n" );
	}
	if ( vcnt>0 ) {
	    fprintf( afm, "StartKernPairs1 %d\n", vcnt );
	    for ( i=0; i<map->enccount ; ++i ) {
		int gid = map->map[i];
		if ( gid!=-1 && SCWorthOutputting(sf->glyphs[gid]) ) {
		    AfmKernPairs(afm,sf->glyphs[gid],true);
		}
	    }
	    fprintf( afm, "EndKernPairs\n" );
	}
	fprintf( afm, "EndKernData\n" );
    }
    if ( cc!=NULL )
	AfmComposites(afm,sf,cc,cc_cnt);
    fprintf( afm, "EndFontMetrics\n" );

    SFLigatureCleanup(sf);
    SFKernCleanup(sf,false);
    SFKernCleanup(sf,true);
    CCFree(cc);

return( !ferror(afm));
}

int AmfmSplineFont(FILE *amfm, MMSet *mm, int formattype,EncMap *map,int layer) {
    int i,j;

    AfmSplineFontHeader(amfm,mm->normal,formattype,map,NULL,layer);
    fprintf( amfm, "Masters %d\n", mm->instance_count );
    fprintf( amfm, "Axes %d\n", mm->axis_count );

    fprintf( amfm, "WeightVector [%g", (double) mm->defweights[0] );
    for ( i=1; i<mm->instance_count; ++i )
	fprintf( amfm, " %g", (double) mm->defweights[i]);
    fprintf( amfm, "]\n" );

    fprintf( amfm, "BlendDesignPositions [" );
    for ( i=0; i<mm->instance_count; ++i ) {
	fprintf(amfm, "[%g", (double) mm->positions[i*mm->axis_count+0]);
	for ( j=1; j<mm->axis_count; ++j )
	    fprintf( amfm, " %g", (double) mm->positions[i*mm->axis_count+j]);
	fprintf(amfm, i==mm->instance_count-1 ? "]" : "] " );
    }
    fprintf( amfm, "]\n" );

    fprintf( amfm, "BlendDesignMap [" );
    for ( i=0; i<mm->axis_count; ++i ) {
	putc('[',amfm);
	for ( j=0; j<mm->axismaps[i].points; ++j )
	    fprintf(amfm, "[%g %g]", (double) mm->axismaps[i].designs[j], (double) mm->axismaps[i].blends[j]);
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

    if (sf->internal_temp)
return;

    for ( j=0; j<sf->glyphcnt; ++j ) if ( sf->glyphs[j]!=NULL ) {
	for ( l = sf->glyphs[j]->ligofme; l!=NULL; l = next ) {
	    next = l->next;
	    for ( scl = l->components; scl!=NULL; scl = sclnext ) {
		sclnext = scl->next;
		chunkfree(scl,sizeof(struct splinecharlist));
	    }
	    if ( l->lig->temporary ) {
		free(l->lig->u.lig.components);
		chunkfree(l->lig,sizeof(PST));
	    }
	    free( l );
	}
	sf->glyphs[j]->ligofme = NULL;
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
    LigList **all = malloc(lmax*sizeof(LigList *));

    /* First clear out any old stuff */
    for ( j=0; j<sf->glyphcnt; ++j ) if ( sf->glyphs[j]!=NULL )
	sf->glyphs[j]->ligofme = NULL;

    /* Attach all the ligatures to the first character of their components */
    /* Figure out what the components are, and if they all exist */
    /* we're only interested in the lig if all components are worth outputting */
    for ( i=0 ; i<sf->glyphcnt; ++i ) if ( SCWorthOutputting(sf->glyphs[i]) && sf->glyphs[i]->possub!=NULL ) {
	for ( lig = sf->glyphs[i]->possub; lig!=NULL; lig=lig->next ) if ( lig->type==pst_ligature ) {
	    ligstart = lig->u.lig.components;
	    last = head = NULL; sc = NULL;
	    for ( pt = ligstart; *pt!='\0'; ) {
		char *start = pt;
		for ( ; *pt!='\0' && *pt!=' '; ++pt );
		ch = *pt; *pt = '\0';
		tsc = SFGetChar(sf,-1,start);
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
		ll = malloc(sizeof(LigList));
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
    for ( i=0 ; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL && sc->ligofme!=NULL ) {
	for ( ll=sc->ligofme, lcnt=0; ll!=NULL; ll=ll->next, ++lcnt );
	/* Finally, order the list so that the longest ligatures are first */
	if ( lcnt>1 ) {
	    if ( lcnt>=lmax )
		all = realloc(all,(lmax=lcnt+30)*sizeof(LigList *));
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

    for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL ) {
	for ( l=sc->ligofme; l!=NULL; l=l->next ) if ( l->lig->subtable->lookup->store_in_afm ) {
	    if ( l->ccnt==3 ) {
		/* we've got a 3 character ligature */
		for ( l2=l->next; l2!=NULL; l2=l2->next ) {
		    if ( l2->lig->subtable==l->lig->subtable &&
			    l2->ccnt == 2 && l2->components->sc==l->components->sc ) {
			/* We've found a two character sub ligature */
			sublig = l2->lig->u.lig.lig;
			for ( l3=sublig->ligofme; l3!=NULL; l3=l3->next ) {
			    if ( l3->ccnt==2 && l3->components->sc==l->components->next->sc &&
				    l3->lig->subtable->lookup->store_in_afm)
			break;
			}
			if ( l3!=NULL )	/* The ligature we want to add already exists */
		break;
			lig = chunkalloc(sizeof(PST));
			*lig = *l->lig;
			lig->temporary = true;
			lig->next = NULL;
			lig->u.lig.components = malloc(strlen(sublig->name)+
					strlen(l->components->next->sc->name)+
					2);
			sprintf(lig->u.lig.components,"%s %s",sublig->name,
				l->components->next->sc->name);
			l3 = malloc(sizeof(LigList));
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
    OTLookup *otl, *otlp, *otln;

    if (sf->internal_temp)
return;

    if ( (!isv && sf->kerns==NULL) || (isv && sf->vkerns==NULL) )	/* can't have gotten messed up */
return;
    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	for ( kp = isv ? sf->glyphs[i]->vkerns : sf->glyphs[i]->kerns, p=NULL; kp!=NULL; kp = n ) {
	    n = kp->next;
	    if ( kp->kcid!=0 ) {
		if ( p!=NULL )
		    p->next = n;
		else if ( isv )
		    sf->glyphs[i]->vkerns = n;
		else
		    sf->glyphs[i]->kerns = n;
		chunkfree(kp,sizeof(*kp));
	    } else
		p = kp;
	}
    }
    for ( otl=sf->gpos_lookups, otlp = NULL; otl!=NULL; otl = otln ) {
	otln = otl->next;
	if ( otl->temporary_kern ) {
	    if ( otlp!=NULL )
		otlp->next = otln;
	    else
		sf->gpos_lookups = otln;
	    OTLookupFree(otl);
	} else
	    otlp = otl;
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

    scs = malloc(cnt*sizeof(SplineChar **));
    for ( i=1; i<cnt; ++i ) {
	for ( pt=classnames[i]-1, j=0; pt!=NULL; pt=strchr(pt+1,' ') )
	    ++j;
	scs[i] = malloc((j+1)*sizeof(SplineChar *));
	for ( pt=classnames[i], j=0; *pt!='\0'; pt=end+1 ) {
	    end = strchr(pt,' ');
	    if ( end==NULL )
		end = pt+strlen(pt);
	    ch = *end;
	    *end = '\0';
	    sc = SFGetChar(sf,-1,pt);
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
	int16_t offset, struct lookup_subtable *sub,uint16_t kcid,int isv) {
    KernPair *kp;

    for ( kp=first->kerns; kp!=NULL; kp=kp->next )
	if ( kp->sc == second )
    break;
    if ( kp==NULL ) {
	kp = chunkalloc(sizeof(KernPair));
	kp->sc = second;
	kp->off = offset;
	kp->subtable = sub;
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

void SFKernClassTempDecompose(SplineFont *sf,int isv) {
    KernClass *kc, *head= isv ? sf->vkerns : sf->kerns;
    KernPair *kp;
    SplineChar ***first, ***last;
    int i, j, k, l;
    OTLookup *otl;

    /* Make sure the temporary field is cleaned up. Otherwise we may lose kerning data */
    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	for ( kp = isv ? sf->glyphs[i]->vkerns : sf->glyphs[i]->kerns; kp!=NULL; kp = kp->next ) {
	    kp->kcid = false;
	}
    }
    for ( kc = head, i=0; kc!=NULL; kc = kc->next )
	kc->kcid = ++i;
    for ( kc = head; kc!=NULL; kc = kc->next ) {

	otl = chunkalloc(sizeof(OTLookup));
	otl->next = sf->gpos_lookups;
	sf->gpos_lookups = otl;
	otl->lookup_type = gpos_pair;
	otl->lookup_flags = kc->subtable->lookup->lookup_flags;
	otl->features = FeatureListCopy(kc->subtable->lookup->features);
	otl->lookup_name = copy(_("<Temporary kerning>"));
	otl->temporary_kern = otl->store_in_afm = true;
	otl->subtables = chunkalloc(sizeof(struct lookup_subtable));
	otl->subtables->lookup = otl;
	otl->subtables->per_glyph_pst_or_kern = true;
	otl->subtables->subtable_name = copy(_("<Temporary kerning>"));

	first = KernClassToSC(sf,kc->firsts,kc->first_cnt);
	last = KernClassToSC(sf,kc->seconds,kc->second_cnt);
	for ( i=1; i<kc->first_cnt; ++i ) for ( j=1; j<kc->second_cnt; ++j ) {
	    if ( kc->offsets[i*kc->second_cnt+j]!=0 ) {
		for ( k=0; first[i][k]!=NULL; ++k )
		    for ( l=0; last[j][l]!=NULL; ++l )
			AddTempKP(first[i][k],last[j][l],
				kc->offsets[i*kc->second_cnt+j],
			        otl->subtables,kc->kcid,isv);
	    }
	}
	KCSfree(first,kc->first_cnt);
	KCSfree(last,kc->second_cnt);
    }
}

/* ************************************************************************** */
/* **************************** Writing PFM files *************************** */
/* ************************************************************************** */
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

static const unsigned short local_unicode_from_win[] = {
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

static int inwin(SplineFont *sf, int winmap[256]) {
    int i, j;
    int cnt;

    memset(winmap,-1,sizeof(int[256]));
    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	int unienc = sf->glyphs[i]->unicodeenc;
	if ( unienc==-1 || unienc>0x3000 )
    continue;
	for ( j=255; j>=0; --j ) {
	    if ( local_unicode_from_win[j]==unienc ) {
		winmap[j] = i;
	break;
	    }
	}
    }
    for ( i=0x80, cnt=0; i<0x100; ++i )
	if ( winmap[i]!=-1 )
	    ++cnt;
return( cnt>64 );
}

static int revwinmap(int winmap[256], int gid) {
    int i;
    for ( i=255; i>=0; --i )
	if ( winmap[i]==gid )
    break;
return( i );
}

int PfmSplineFont(FILE *pfm, SplineFont *sf, EncMap *map,int layer) {
    int caph=0, xh=0, ash=0, dsh=0, cnt=0, first=-1, samewid=-1, maxwid= -1, last=0, wid=0, ymax=0, ymin=0;
    int kerncnt=0, spacepos=0x20;
    int i, ii;
    const char *pt;
    KernPair *kp;
    int winmap[256];
    /* my docs imply that pfm files can only handle 1byte fonts */
    /* I think they must be output as if the font were encoded in winansi */
    /* Ah, but Adobe's technical note 5178.PFM.PDF gives the two byte format */
    long size, devname, facename, extmetrics, exttable, driverinfo, kernpairs, pos;
    DBounds b;
    int style;
    int windows_encoding;

    if ( map->enc->is_japanese ||
	    (sf->cidmaster!=NULL && strnmatch(sf->cidmaster->ordering,"Japan",5)==0 ))
	windows_encoding = 128;
    else if ( map->enc->is_korean ||
	    (sf->cidmaster!=NULL && strnmatch(sf->cidmaster->ordering,"Korea",5)==0 ))
	windows_encoding = 129;
    else if ( map->enc->is_tradchinese ||
	    (sf->cidmaster!=NULL && strnmatch(sf->cidmaster->ordering,"CNS",3)==0 ))
	windows_encoding = 136;
    else if ( map->enc->is_simplechinese ||
	    (sf->cidmaster!=NULL && strnmatch(sf->cidmaster->ordering,"GB",2)==0 ))
	windows_encoding = 134;
    else if ( map->enc->is_custom )
	windows_encoding = 2;
    else
	windows_encoding = strmatch(map->enc->enc_name,"symbol")==0?2:0;
    if ( windows_encoding==0 )
	if ( !inwin(sf,winmap))
	    windows_encoding = 0xff;
    if ( windows_encoding!=0 ) {
	memset(winmap,-1,sizeof(winmap));
	for ( i=0; i<sf->glyphcnt; ++i )
	    if ( (ii=map->backmap[i])!=-1 && ii<256 )
		winmap[ii] = i;
    }

    SFKernClassTempDecompose(sf,false);
    for ( ii=0; ii<256; ++ii ) if ( (i=winmap[ii])!=-1 ) {
	if ( sf->glyphs[i]!=NULL && sf->glyphs[i]->unicodeenc==' ' )
	    spacepos = ii;
	if ( SCWorthOutputting(sf->glyphs[i]) ) {
	    ++cnt;
	    if ( sf->glyphs[i]->unicodeenc=='I' || sf->glyphs[i]->unicodeenc=='x' ||
		    sf->glyphs[i]->unicodeenc=='H' || sf->glyphs[i]->unicodeenc=='d' ||
		    sf->glyphs[i]->unicodeenc=='p' || sf->glyphs[i]->unicodeenc=='l' ) {
		SplineCharLayerFindBounds(sf->glyphs[i],layer,&b);
		if ( ymax<b.maxy ) ymax = b.maxy;
		if ( ymin>b.miny ) ymin = b.miny;
		if ( sf->glyphs[i]->unicodeenc=='I' || sf->glyphs[i]->unicodeenc=='H' )
		    caph = b.maxy;
		else if ( sf->glyphs[i]->unicodeenc=='x' )
		    xh = b.maxy;
		else if ( sf->glyphs[i]->unicodeenc=='d' || sf->glyphs[i]->unicodeenc=='l' )
		    ash = b.maxy;
		else
		    dsh = b.miny;
	    }
	    if ( samewid==-1 )
		samewid = sf->glyphs[i]->width;
	    else if ( samewid!=sf->glyphs[i]->width )
		samewid = -2;
	    if ( maxwid<sf->glyphs[i]->width )
		maxwid = sf->glyphs[i]->width;
	    if ( first==-1 )
		first = ii;
	    wid += sf->glyphs[i]->width;
	    last = ii;
	    for ( kp=sf->glyphs[i]->kerns; kp!=NULL; kp = kp->next )
		if ( revwinmap(winmap,kp->sc->orig_pos)!=-1 )
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
    for ( pt="PostScript"/*"\273PostScript\253"*/; *pt; ++pt )
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
	if ( (i=winmap[ii])==-1 || sf->glyphs[i]==NULL )
	    putlshort(0,pfm);
	else
	    putlshort(sf->glyphs[i]->width,pfm);
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
	for ( ii=first; ii<last; ++ii ) if ( (i=winmap[ii])!=-1 && sf->glyphs[i]!=NULL ) {
	    if ( SCWorthOutputting(sf->glyphs[i]) ) {
		for ( kp=sf->glyphs[i]->kerns; kp!=NULL; kp = kp->next )
		    if ( kerncnt<512 && revwinmap(winmap,kp->sc->orig_pos)!=-1 ) {
			putc(ii,pfm);
			putc(revwinmap(winmap,kp->sc->orig_pos),pfm);
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

/* ************************************************************************** */
/* **************************** Reading PFM files *************************** */
/* ************************************************************************** */
int LoadKerningDataFromPfm(SplineFont *sf, char *filename,EncMap *map) {
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
	else {
	    for ( i=0; i<256 && i<map->enccount; ++i )
		winmap[i] = map->map[i];
	    for ( i=0; i<256; ++i )
		winmap[i] = -1;
	}
	kerncnt = getlshort(file);
	/* Docs say at most 512 kerning pairs. Not worth posting a progress */
	/*  dlg */
	for ( i=0; i<kerncnt; ++i ) {
	    ch1 = getc(file);
	    ch2 = getc(file);
	    offset = (short) getlshort(file);
	    if ( !feof(file) && winmap[ch1]!=-1 && winmap[ch2]!=-1 )
		KPInsert(sf->glyphs[winmap[ch1]],sf->glyphs[winmap[ch2]],offset,false);
		/* No support for vertical kerning that I'm aware of */
	}
    }
    fclose(file);
return( true );
}

/* ************************************************************************** */
/* **************************** Writing TFM files *************************** */
/* ************************************************************************** */
typedef uint32_t fix_word;

struct tfm_header {
    uint32_t checksum;	/* don't know how to calculate this, use 0 to ignore it */
    fix_word design_size;	/* in points (10<<20 seems to be default) */
    char encoding[40];	/* first byte is length, rest are a string that names the encoding */
	/* ASCII, TeX text, TeX math extension, XEROX, GRAPHIC, UNSPECIFIED */
    char family[20];	/* Font Family, preceded by a length byte */
    uint8_t seven_bit_safe_flag;
    uint8_t ignored[2];
    uint8_t face; 	/* 0=>roman, 1=>italic */
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
/* tfm files use uint8s, ofm files use uint16s */
struct ligkern {
    uint16_t skip;
    uint16_t other_char;
    uint16_t op;
    uint16_t remainder;
    struct ligkern *next;
};
struct extension {
    uint16_t extens[4];	/* top, mid, bottom & repeat */
};

static struct ligkern *TfmAddKern(KernPair *kp,struct ligkern *last,double *kerns,
	int *_kcnt, EncMap *map,int maxc) {
    struct ligkern *new = calloc(1,sizeof(struct ligkern));
    int i;

    new->other_char = map->backmap[kp->sc->orig_pos];
    for ( i=*_kcnt-1; i>=0 ; --i )
	if ( kerns[i] == kp->off )
    break;
    if ( i<0 ) {
	i = (*_kcnt)++;
	kerns[i] = kp->off;
    }
    if ( maxc==256 ) {
	new->remainder = i&0xff;
	new->op = 128 + (i>>8);
    } else {
	new->remainder = i&0xffff;
	new->op = 128 + (i>>16);
    }
    new->next = last;
return( new );
}

static struct ligkern *TfmAddLiga(LigList *l,struct ligkern *last,EncMap *map,
	int maxc) {
    struct ligkern *new;

    if ( !l->lig->subtable->lookup->store_in_afm )
return( last );
    if ( map->backmap[l->lig->u.lig.lig->orig_pos]>=maxc )
return( last );
    if ( l->components==NULL ||  map->backmap[l->components->sc->orig_pos]>=maxc ||
	    l->components->next!=NULL )
return( last );
    new = calloc(1,sizeof(struct ligkern));
    new->other_char = map->backmap[l->components->sc->orig_pos];
    new->remainder = map->backmap[l->lig->u.lig.lig->orig_pos];
    new->next = last;
    new->op = 0*4 + 0*2 + 0;
	/* delete next char, delete current char, start over and check the resultant ligature for more ligs */
return( new );
}

static void FindCharlists(SplineFont *sf,int *charlistindex,EncMap *map,int maxc) {
    int i, last, ch;
    char *pt, *end, *variants;

    memset(charlistindex,-1,(maxc+1)*sizeof(int));
    for ( i=0; i<maxc && i<map->enccount; ++i ) if ( map->map[i]!=-1 && SCWorthOutputting(sf->glyphs[map->map[i]])) {
	SplineChar *sc = sf->glyphs[map->map[i]];

	variants = NULL;
	if ( sc->vert_variants!=NULL && sc->vert_variants->variants!=NULL )
	    variants = sc->vert_variants->variants;
	else if ( sc->horiz_variants!=NULL && sc->horiz_variants->variants!=NULL )
	    variants = sc->horiz_variants->variants;
	if ( variants!=NULL ) {
	    last = i;
	    for ( pt=variants; *pt; pt = end ) {
		end = strchr(pt,' ');
		if ( end==NULL )
		    end = pt+strlen(pt);
		ch = *end;
		*end = '\0';
		sc = SFGetChar(sf,-1,pt);
		*end = ch;
		while ( *end==' ' ) ++end;
		if ( sc!=NULL && map->backmap[sc->orig_pos]<maxc ) {
		    charlistindex[last] = map->backmap[sc->orig_pos];
		    last = map->backmap[sc->orig_pos];
		}
	    }
	}
    }
}

static int FindExtensions(SplineFont *sf,struct extension *extensions,int *extenindex,
	EncMap *map,int maxc) {
    int i;
    int j,k;
    char *foundnames[4];
    int16_t founds[4]; int ecnt=0;

    memset(extenindex,-1,(maxc+1)*sizeof(int));
    for ( i=0; i<maxc && i<map->enccount; ++i ) if ( map->map[i]!=-1 && SCWorthOutputting(sf->glyphs[map->map[i]])) {
	SplineChar *sc = sf->glyphs[map->map[i]];
	struct glyphvariants *gv;

	gv=NULL;
	if ( sc->vert_variants!=NULL && sc->vert_variants->part_cnt>0 )
	    gv = sc->vert_variants;
	else if ( sc->horiz_variants!=NULL && sc->horiz_variants->part_cnt>0 )
	    gv = sc->horiz_variants;
	if ( gv!=NULL ) {
	    foundnames[0] = foundnames[1] = foundnames[2] = foundnames[3] = NULL;
	    for ( j=k=0; j<gv->part_cnt; ++j ) {
		if ( !gv->parts[j].is_extender ) {
		    if ( k>=3 ) {
			k=4;
	    break;
		    } else if ( k==1 && j==gv->part_cnt-1 )
			foundnames[2] = gv->parts[j].component;
		    else
			foundnames[k++] = gv->parts[j].component;
		} else {
		    if ( foundnames[3]==NULL || strcmp(foundnames[3],gv->parts[j].component)==0 ) {
			foundnames[3] = gv->parts[j].component;
			if ( k==0 )
			    k=1;		/* Can't be first */
		    } else {
			k = 4;
	    break;
		    }
		}
	    }
	    founds[0] = founds[1] = founds[2] = founds[3] = 0;
	    for ( j=0; j<4; ++j ) if ( foundnames[j]!=NULL ) {
		sc = SFGetChar(sf,-1,foundnames[j]);
		if ( sc==NULL )
		    k=4;
		else if ( map->backmap[sc->orig_pos]<maxc &&
			map->backmap[sc->orig_pos]!=-1 )
		    founds[j] = map->backmap[sc->orig_pos];
		else
		    k=4;
	    }
	    if ( k!=4 ) {
		extenindex[i] = ecnt;
		memcpy(extensions[ecnt].extens,founds,sizeof(founds));
		++ecnt;
	    }
	}
    }
return( ecnt );
}

static int CoalesceValues(double *values,int max,int *index,int maxc) {
    int i,j,k,top, offpos,diff,eoz;
    int _backindex[257];
    double off, test;
    double _topvalues[257], _totvalues[257], *topvalues, *totvalues;
    int _cnt[257];
    int *backindex, *cnt;

    if ( maxc<=256 ) {
	backindex = _backindex;
	topvalues = _topvalues;
	totvalues = _totvalues;
	cnt = _cnt;
    } else {
	backindex = malloc((maxc+1)*sizeof(int));
	topvalues = malloc((maxc+1)*sizeof(double));
	totvalues = malloc((maxc+1)*sizeof(double));
	cnt = malloc((maxc+1)*sizeof(int));
    }

    values[maxc] = 0;
    for ( i=0; i<=maxc; ++i )
	backindex[i] = i;

    /* Coalesce zeroes first. This speeds up the sort immensely */
    eoz = 0;
    for ( i=1; i<maxc; ++i ) {
	if ( values[i]==0 ) {
	    int l = backindex[i];
	    backindex[i] = backindex[eoz];
	    values[i] = values[eoz];
	    backindex[eoz] = l;
	    values[eoz] = 0;
	    eoz++;
	}
    }
    /* sort */
    for ( i=eoz; i<maxc; ++i ) for ( j=i+1; j<=maxc; ++j ) {
	if ( values[i]>values[j] ) {
	    int l = backindex[i];
	    double val = values[i];
	    backindex[i] = backindex[j];
	    values[i] = values[j];
	    backindex[j] = l;
	    values[j] = val;
	}
    }
    for ( i=0; i<=maxc; ++i )
	index[backindex[i]] = i;
    top = maxc+1;
    for ( i=0; i<top; ++i ) {
	for ( j=i+1; j<top && values[i]==values[j]; ++j );
	if ( j>i+1 ) {
	    int diff = j-i-1;
	    for ( k=i+1; k+diff<top; ++k )
		values[k] = values[k+diff];
	    for ( k=0; k<=maxc; ++k ) {
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
		for ( k=0; k<=maxc; ++k ) {
		    if ( index[k]==0 ) index[k] = i;
		    else if ( index[k]==i ) index[k] = 0;
		}
	    }
	}
	if ( maxc>256 ) {
	    free(backindex);
	    free(topvalues);
	    free(totvalues);
	    free(cnt);
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
	for ( k=0; k<=maxc; ++k ) {
	    if ( index[k]>offpos )
		index[k] -= diff;
	}
	top -= diff;
    }
    values[0] = 0;
    for ( i=1; i<top; ++i )
	values[i] = totvalues[i]/cnt[i];
    if ( maxc>256 ) {
	free(backindex);
	free(topvalues);
	free(totvalues);
	free(cnt);
    }
return( top );
}

void TeXDefaultParams(SplineFont *sf) {
    int i, spacew;
    BlueData bd;

    if ( sf->texdata.type!=tex_unset )
return;

    spacew = rint(.33*(1<<20));	/* 1/3 em for a space seems like a reasonable size */
    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL && sf->glyphs[i]->unicodeenc==' ' ) {
	spacew = rint((sf->glyphs[i]->width<<20)/(sf->ascent+sf->descent));
    break;
    }
    QuickBlues(sf,ly_fore,&bd);

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

static int _OTfmSplineFont(FILE *tfm, SplineFont *sf,EncMap *map,int maxc,int layer) {
    struct tfm_header header;
    char *full=NULL;
    const char *encname;
    int i;
    DBounds b;
    struct ligkern *_ligkerns[256], **ligkerns, *lk, *lknext;
    double _widths[257], _heights[257], _depths[257], _italics[257];
    double *widths, *heights, *depths, *italics;
    uint8_t _tags[256], *tags;
    uint16_t _lkindex[256], *lkindex;
    int _former[256], *former;
    struct extension _extensions[257], *extensions;
    int _charlistindex[257], _extenindex[257];
    int *charlistindex, *extenindex;
    int _widthindex[257], _heightindex[257], _depthindex[257], _italicindex[257];
    int *widthindex, *heightindex, *depthindex, *italicindex;
    double *kerns;
    int widcnt, hcnt, dcnt, icnt, kcnt, lkcnt, pcnt, ecnt, sccnt;
    int first, last;
    KernPair *kp;
    LigList *l;
    int style, any;
    uint32_t *lkarray;
    struct ligkern *o_lkarray=NULL;
    char *familyname;
    int anyITLC;
    real scale = (1<<20)/(double) (sf->ascent+sf->descent);
    int toobig_warn = false;

    if ( maxc==256 ) {
	ligkerns = _ligkerns;
	widths = _widths;
	heights = _heights;
	depths = _depths;
	italics = _italics;
	tags = _tags;
	lkindex = _lkindex;
	former = _former;
	charlistindex = _charlistindex;
	extensions = _extensions;
	extenindex = _extenindex;
	widthindex = _widthindex;
	heightindex = _heightindex;
	depthindex = _depthindex;
	italicindex = _italicindex;
    } else {
	ligkerns = malloc(maxc*sizeof(struct ligkern *));
	widths = malloc((maxc+1)*sizeof(double));
	heights = malloc((maxc+1)*sizeof(double));
	depths = malloc((maxc+1)*sizeof(double));
	italics = malloc((maxc+1)*sizeof(double));
	tags = malloc(maxc*sizeof(uint8_t));
	lkindex = malloc(maxc*sizeof(uint16_t));
	former = malloc(maxc*sizeof(int));
	charlistindex = malloc((maxc+1)*sizeof(int));
	extensions = malloc((maxc+1)*sizeof(struct extension));
	extenindex = malloc((maxc+1)*sizeof(int));
	widthindex = malloc((maxc+1)*sizeof(int));
	heightindex = malloc((maxc+1)*sizeof(int));
	depthindex = malloc((maxc+1)*sizeof(int));
	italicindex = malloc((maxc+1)*sizeof(int));
    }
    SFLigaturePrepare(sf);
    LigatureClosure(sf);		/* Convert 3 character ligs to a set of two character ones when possible */
    SFKernClassTempDecompose(sf,false);	/* Undoes kern classes */
    TeXDefaultParams(sf);

    memset(&header,0,sizeof(header));
    header.checksum = 0;		/* don't check checksum (I don't know how to calculate it) */
    header.design_size = ((sf->design_size<<19)+2)/5;
    if ( header.design_size==0 ) header.design_size = 10<<20;
    encname=NULL;
    /* These first two encoding names appear magic to txtopl */
    /* I tried checking that the encodings were correct, name by name, but */
    /*  that doesn't work. People use non-standard names */
    if ( sf->texdata.type==tex_math ) encname = "TEX MATH SYMBOLS";
    else if ( sf->texdata.type==tex_mathext ) encname = "TEX MATH EXTENSION";
    else if ( sf->subfontcnt==0 &&  map->enc!=&custom )
	encname = EncodingName( map->enc );
    if ( encname==NULL ) {
	full = malloc(strlen(sf->fontname)+10);
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
    if ( header.family[0]>=19 ) {
	header.family[0] = 19;
	memcpy(header.family+1,familyname,19);
    } else
	strcpy(header.family+1,familyname);
    for ( i=128; i<map->enccount && i<maxc; ++i )
	if ( map->map[i]!=-1 && SCWorthOutputting(sf->glyphs[map->map[i]]))
    break;
    if ( i>=map->enccount || i>=maxc )
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

    memset(widths,0,maxc*sizeof(double));
    memset(heights,0,maxc*sizeof(double));
    memset(depths,0,maxc*sizeof(double));
    memset(italics,0,maxc*sizeof(double));
    memset(tags,0,maxc*sizeof(uint8_t));
    first = last = -1;
    /* Note: Text fonts for TeX and math fonts use the italic correction & width */
    /*  fields to mean different things */
    anyITLC = false;
    for ( i=0; i<maxc && i<map->enccount; ++i )
	if ( map->map[i]!=-1 && SCWorthOutputting(sf->glyphs[map->map[i]])) {
	    if ( sf->glyphs[map->map[i]]->italic_correction!=TEX_UNDEF ) {
		anyITLC = true;
	break;
	    }
	}
    for ( i=0; i<maxc && i<map->enccount; ++i ) {
	if ( map->map[i]!=-1 && SCWorthOutputting(sf->glyphs[map->map[i]])) {
	    SplineChar *sc = sf->glyphs[map->map[i]];
	    if ( sc->tex_height==TEX_UNDEF || sc->tex_depth==TEX_UNDEF ||
		    sc->italic_correction==TEX_UNDEF ) {
		SplineCharLayerFindBounds(sc,layer,&b);
		heights[i] = b.maxy*scale; depths[i] = -b.miny*scale;
	    }
	    if ( sc->tex_height!=TEX_UNDEF )
		heights[i] = sc->tex_height*scale;
	    if ( sc->tex_depth!=TEX_UNDEF )
		depths[i] = sc->tex_depth*scale;
	    if ( depths[i]<0 ) depths[i] = 0;		/* Werner says depth should never be negative. Something about how accents are positioned */
	    if ( sc->width==0 )
		widths[i] = 1;	/* TeX/Omega use a 0 width as a flag for non-existant character. Stupid. So zero width glyphs must be given the smallest possible non-zero width, to ensure they exists. GRRR. */
	    else if ( sc->width*scale >= (16<<20) ) {
		LogError( _("The width of %s is too big to fit in a tfm fix_word, it shall be truncated to the largest size allowed."), sc->name );
		widths[i] = (16<<20)-1;
	    } else
		widths[i] = sc->width*scale;
	    if ( sc->italic_correction!=TEX_UNDEF )
		italics[i] = sc->italic_correction*scale;
	    else if ( (style&sf_italic) && b.maxx>sc->width && !anyITLC )
		italics[i] = ((b.maxx-sc->width) +
			    (sf->ascent+sf->descent)/16.0)*scale;
					/* With a 1/16 em white space after it */
	    if ( widths[i]<0 ) widths[i] = 0;
	    if ( first==-1 ) first = i;
	    last = i;
	    if ( widths[i]>=(16<<20) || heights[i]>=(16<<20) ||
		    depths[i]>=(16<<20) || italics[i]>=(16<<20) ) {
		if ( !toobig_warn ) {
		    ff_post_error(_("Value exceeds tfm limitations"),_("The width, height, depth or italic correction of %s is too big. Tfm files may not contain values bigger than 16 times the em-size of the font. Width=%g, height=%g, depth=%g, italic correction=%g"),
			sc->name, widths[i]/(1<<20), heights[i]/(1<<20), depths[i]/(1<<20), italics[i]/(1<<20) );
		    toobig_warn = true;
		}
		if ( widths[i]>(16<<20)-1 )
		    widths[i] = (16<<20)-1;
		if ( heights[i]>(16<<20)-1 )
		    heights[i] = (16<<20)-1;
		if ( depths[i]>(16<<20)-1 )
		    depths[i] = (16<<20)-1;
		if ( italics[i]>(16<<20)-1 )
		    italics[i] = (16<<20)-1;
	    }
	}
    }
    widcnt = CoalesceValues(widths,maxc,widthindex,maxc);
    hcnt = CoalesceValues(heights,maxc<=256?16:256,heightindex,maxc);
    dcnt = CoalesceValues(depths,maxc<=256?16:256,depthindex,maxc);
    icnt = CoalesceValues(italics,maxc<=256?64:256,italicindex,maxc);
    if ( last==-1 ) { first = 1; last = 0; }

    FindCharlists(sf,charlistindex,map,maxc);
    ecnt = FindExtensions(sf,extensions,extenindex,map,maxc);

    kcnt = 0;
    for ( i=0; i<maxc && i<map->enccount; ++i ) {
	if ( map->map[i]!=-1 && SCWorthOutputting(sf->glyphs[map->map[i]])) {
	    SplineChar *sc = sf->glyphs[map->map[i]];
	    for ( kp=sc->kerns; kp!=NULL; kp=kp->next )
		if ( kp->sc->orig_pos<sf->glyphcnt &&	/* Can happen when saving multiple pfbs */
			map->backmap[kp->sc->orig_pos]<maxc ) ++kcnt;
	}
    }
    kerns = NULL;
    if ( kcnt!=0 )
	kerns = malloc(kcnt*sizeof(double));
    kcnt = lkcnt = 0;
    memset(ligkerns,0,maxc*sizeof(struct ligkern *));
    for ( i=0; i<maxc && i<map->enccount; ++i ) {
	if ( map->map[i]!=-1 && SCWorthOutputting(sf->glyphs[map->map[i]])) {
	    SplineChar *sc = sf->glyphs[map->map[i]];
	    for ( kp=sc->kerns; kp!=NULL; kp=kp->next )
		if ( kp->sc->orig_pos<sf->glyphcnt &&	/* Can happen when saving multiple pfbs */
			map->backmap[kp->sc->orig_pos]<maxc )
		    ligkerns[i] = TfmAddKern(kp,ligkerns[i],kerns,&kcnt,map,maxc);
	    for ( l=sc->ligofme; l!=NULL; l=l->next )
		ligkerns[i] = TfmAddLiga(l,ligkerns[i],map,maxc);
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
    }
    for ( i=sccnt=0; i<maxc && i<map->enccount; ++i )
	if ( ligkerns[i]!=NULL )
	    ++sccnt;
    if ( sccnt>=128 )	/* We need to use the special extension mechanism */
	lkcnt += sccnt;
    memset(former,-1,maxc*sizeof(int));
    memset(lkindex,0,maxc*sizeof(uint16_t));
    if ( maxc==256 ) {
	lkarray = malloc(lkcnt*sizeof(uint32_t));
	if ( sccnt<128 ) {
	    lkcnt = 0;
	    do {
		any = false;
		for ( i=0; i<maxc; ++i ) if ( ligkerns[i]!=NULL ) {
		    lk = ligkerns[i];
		    ligkerns[i] = lk->next;
		    if ( former[i]==-1 )
			lkindex[i] = lkcnt;
		    else {
			lkarray[former[i]] |= (lkcnt-former[i]-1)<<24;
			if ( lkcnt-former[i]-1 >= 128 )
			    IError( " generating lig/kern array, jump too far.\n" );
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
		    lkarray[lkcnt2++] = ((lknext==NULL?128:0)<<24) |
					(lk->other_char<<16) |
					(lk->op<<8) |
					lk->remainder;
		    free( lk );
		}
	    }
	    if ( lkcnt>sccnt )
		IError( "lig/kern mixup\n" );
	    lkcnt = lkcnt2;
	}
    } else {
	o_lkarray = calloc(lkcnt,sizeof(struct ligkern));
	if ( sccnt<128 ) {
	    lkcnt = 0;
	    do {
		any = false;
		for ( i=0; i<maxc; ++i ) if ( ligkerns[i]!=NULL ) {
		    lk = ligkerns[i];
		    ligkerns[i] = lk->next;
		    if ( former[i]==-1 )
			lkindex[i] = lkcnt;
		    else {
			o_lkarray[former[i]].skip |= lkcnt-former[i]-1;
			if ( lkcnt-former[i]-1 >= 128 )
			    IError( " generating lig/kern array, jump too far.\n" );
		    }
		    former[i] = lkcnt;
		    o_lkarray[lkcnt].skip = lk->next==NULL ? 128U : 0U;
		    o_lkarray[lkcnt].other_char = lk->other_char;
		    o_lkarray[lkcnt].op = lk->op;
		    o_lkarray[lkcnt++].remainder = lk->remainder;
		    free( lk );
		    any = true;
		}
	    } while ( any );
	} else {
	    int lkcnt2 = sccnt;
	    lkcnt = 0;
	    for ( i=0; i<maxc; ++i ) if ( ligkerns[i]!=NULL ) {
		lkindex[i] = lkcnt;
		/* long pointer into big ligkern array */
		o_lkarray[lkcnt].skip = 128+1;
		o_lkarray[lkcnt].other_char = 0;
		o_lkarray[lkcnt].op = lkcnt2>>16;
		o_lkarray[lkcnt++].remainder = lkcnt2&0xffff;
		for ( lk = ligkerns[i]; lk!=NULL; lk=lknext ) {
		    lknext = lk->next;
		    /* Here we will always skip to the next record, so the	*/
		    /* skip_byte will always be 0 (well, or 128 for stop)	*/
		    former[i] = lkcnt;
		    o_lkarray[lkcnt2].skip = (lknext==NULL?128:0);
		    o_lkarray[lkcnt2].other_char = lk->other_char;
		    o_lkarray[lkcnt2].op = lk->op;
		    o_lkarray[lkcnt2++].remainder = lk->remainder;
		    free( lk );
		}
	    }
	    if ( lkcnt>sccnt )
		IError( "lig/kern mixup\n" );
	    lkcnt = lkcnt2;
	}
    }

/* Now output the file */
	/* Table of contents */
    if ( maxc==256 ) {
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
    } else {
	int font_dir = 0;
	    /* According to ofm2opl sources fontdir is: */
	    /*  0=>TL, 1=>LT, 2=>TR, 3=>LB, 4=>BL, 5=>RT, 6=>BR, 7=>RB */
	    /* And if bit 3 (8) is set then we have something called NFONTDIR */
	    /*  no idea what that means */
	    /* TL Means start at the top left of the page (standard L 2 R) */
	    /* TR Means start at the top right of the page (standard R 2 L) */
	    /* And I think RT is appropriate for vertical Japanese */
	    /* (bit three might mean the reversed direction of English inside */
	    /*  mongolian. It doesn't seem appropriate in a font though) */
	    /* HOWEVER, all the ofm files I've seen, including arabic only ones*/
	    /*  set fontdir to 0 (TL). So I shall too */
	putlong(tfm,0);		/* Undocumented entry. Perhaps the level? */
				/* ofm2opl says it is the level */
	putlong(tfm,
		14+		/* Table of contents size */
		18 +		/* header size */
		2*(last-first+1) +  /* Per glyph data */
		widcnt +	/* entries in the width table */
		hcnt +		/* entries in the height table */
		dcnt +		/* entries in the depth table */
		icnt +		/* entries in the italic correction table */
		2*lkcnt +	/* entries in the lig/kern table */
		kcnt +		/* entries in the kern table */
		2*ecnt +	/* No extensible characters here */
		pcnt);		/* font parameter words */
	putlong(tfm,18);
	putlong(tfm,first);
	putlong(tfm,last);
	putlong(tfm,widcnt);
	putlong(tfm,hcnt);
	putlong(tfm,dcnt);
	putlong(tfm,icnt);
	putlong(tfm,lkcnt);
	putlong(tfm,kcnt);
	putlong(tfm,ecnt);
	putlong(tfm,pcnt);
	putlong(tfm,font_dir);
    }
	    /* header */
    putlong(tfm,header.checksum);
    putlong(tfm,header.design_size);
    fwrite(header.encoding,1,sizeof(header.encoding),tfm);
    fwrite(header.family,1,sizeof(header.family),tfm);
    fwrite(&header.seven_bit_safe_flag,1,4,tfm);
	    /* per-glyph data */
    for ( i=first; i<=last ; ++i ) {
	if ( map->map[i]!=-1 && SCWorthOutputting(sf->glyphs[map->map[i]])) {
	    if ( maxc==256 ) {
		putc(widthindex[i],tfm);
		putc((heightindex[i]<<4)|depthindex[i],tfm);
		putc((italicindex[i]<<2)|tags[i],tfm);
		putc(tags[i]==0?0 :
			tags[i]==1?lkindex[i] :
			tags[i]==2?charlistindex[i] :
			extenindex[i],tfm);
	    } else {
		putshort(tfm,widthindex[i]);
		putc(heightindex[i],tfm);
		putc(depthindex[i],tfm);
		putc(italicindex[i],tfm);
		putc(tags[i],tfm);
		putshort(tfm,tags[i]==0?0 :
			tags[i]==1?lkindex[i] :
			tags[i]==2?charlistindex[i] :
			extenindex[i]);
	    }
	} else {
	    putlong(tfm,0);
	    if ( maxc>256 )
		putlong(tfm,0);
	}
    }
	    /* width table */
    for ( i=0; i<widcnt; ++i )
	putlong(tfm,widths[i]);
	    /* height table */
    for ( i=0; i<hcnt; ++i )
	putlong(tfm,heights[i]);
	    /* depth table */
    for ( i=0; i<dcnt; ++i )
	putlong(tfm,depths[i]);
	    /* italic correction table */
    for ( i=0; i<icnt; ++i )
	putlong(tfm,italics[i]);

	    /* lig/kern table */
    if ( maxc==256 ) {
	for ( i=0; i<lkcnt; ++i )
	    putlong(tfm,lkarray[i]);
    } else {
	for ( i=0; i<lkcnt; ++i ) {
	    putshort(tfm,o_lkarray[i].skip);
	    putshort(tfm,o_lkarray[i].other_char);
	    putshort(tfm,o_lkarray[i].op);
	    putshort(tfm,o_lkarray[i].remainder);
	}
    }

	    /* kern table */
    for ( i=0; i<kcnt; ++i )
	putlong(tfm,(kerns[i]*(1<<20))/(sf->ascent+sf->descent));

	    /* extensible table */
    if ( maxc==256 ) {
	for ( i=0; i<ecnt; ++i ) {
	    putc(extensions[i].extens[0],tfm);
	    putc(extensions[i].extens[1],tfm);
	    putc(extensions[i].extens[2],tfm);
	    putc(extensions[i].extens[3],tfm);
	}
    } else {
	for ( i=0; i<ecnt; ++i ) {
	    putshort(tfm,extensions[i].extens[0]);
	    putshort(tfm,extensions[i].extens[1]);
	    putshort(tfm,extensions[i].extens[2]);
	    putshort(tfm,extensions[i].extens[3]);
	}
    }

	    /* font parameters */
    for ( i=0; i<pcnt; ++i )
	putlong(tfm,sf->texdata.params[i]);

    SFLigatureCleanup(sf);
    SFKernCleanup(sf,false);

    if ( maxc>256 ) {
	free( o_lkarray );
	free( ligkerns );
	free( widths );
	free( heights );
	free( depths );
	free( italics );
	free( tags );
	free( lkindex );
	free( former );
	free( charlistindex );
	free( extensions );
	free( extenindex );
	free( widthindex );
	free( heightindex );
	free( depthindex );
	free( italicindex );
    } else
	free( lkarray );
return( !ferror(tfm));
}

int TfmSplineFont(FILE *tfm, SplineFont *sf, EncMap *map,int layer) {
return( _OTfmSplineFont(tfm,sf,map,256,layer));
}
/* ************************************************************************** */
/* **************************** Writing OFM files *************************** */
/* ************************************************************************** */

int OfmSplineFont(FILE *tfm, SplineFont *sf, EncMap *map,int layer) {
return( _OTfmSplineFont(tfm,sf,map,65536,layer));
}

/* ************************************************************************** */

enum metricsformat { mf_none, mf_afm, mf_amfm, mf_tfm, mf_ofm, mf_pfm, mf_feat };

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

    if ( len >= 48 && sb.st_size == 4*BigEndianWord(buffer+4) &&
	    BigEndianWord(buffer) == 0 &&
	    BigEndianWord(buffer+4) == 14 +
		    BigEndianWord(buffer+8) +
		    2*( BigEndianWord(buffer+16) - BigEndianWord(buffer+12) + 1 ) +
		    BigEndianWord(buffer+20) +
		    BigEndianWord(buffer+24) +
		    BigEndianWord(buffer+28) +
		    BigEndianWord(buffer+32) +
		    2*BigEndianWord(buffer+36) +
		    BigEndianWord(buffer+40) +
		    2*BigEndianWord(buffer+44) +
		    BigEndianWord(buffer+48) )
return( mf_ofm );

    if ( len>= 6 && buffer[0]==0 && buffer[1]==1 &&
	    (buffer[2]|(buffer[3]<<8)|(buffer[4]<<16)|(buffer[5]<<24))== sb.st_size )
return( mf_pfm );

    /* I don't see any distinguishing marks for a feature file */

    if ( strstrmatch(filename,".afm")!=NULL )
return( mf_afm );
    if ( strstrmatch(filename,".amfm")!=NULL )
return( mf_amfm );
    if ( strstrmatch(filename,".tfm")!=NULL )
return( mf_tfm );
    if ( strstrmatch(filename,".ofm")!=NULL )
return( mf_ofm );
    if ( strstrmatch(filename,".pfm")!=NULL )
return( mf_pfm );
    if ( strstrmatch(filename,".fea")!=NULL )
return( mf_feat );

return( mf_none );
}

int LoadKerningDataFromMetricsFile(SplineFont *sf,char *filename,EncMap *map, bool ignore_invalid_replacement) {
    int ret;

    switch ( MetricsFormatType(filename)) {
      case mf_afm:
	ret = LoadKerningDataFromAfm(sf,filename);
      break;
      case mf_amfm:
	ret = LoadKerningDataFromAmfm(sf,filename);
      break;
      case mf_tfm:
	ret = LoadKerningDataFromTfm(sf,filename,map);
      break;
      case mf_ofm:
	ret = LoadKerningDataFromOfm(sf,filename,map);
      break;
      case mf_pfm:
	ret = LoadKerningDataFromPfm(sf,filename,map);
      break;
      case mf_feat:
	SFApplyFeatureFilename(sf,filename, ignore_invalid_replacement);
	ret = true;
      break;
      case mf_none:
      default:
	/* If all else fails try a mac resource */
	/* mac resources can be in so many different formats and even files */
	/*  that I'm not even going to try to check for it here */
	ret = LoadKerningDataFromMacFOND(sf,filename,map);
      break;
    }
    if ( ret ) {
	FontInfo_Destroy(sf);
	MVReKernAll(sf);
    }
return( ret );
}
