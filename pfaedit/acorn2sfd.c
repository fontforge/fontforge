#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "splinefont.h"
#include <ustring.h>

extern char *psunicodenames[];

#define true	1
#define false	0

struct Outlines {
    int version;
    int design_size;
    int nchunks;		/* Each chunk holds outlines for 32 characters. I think */
    int *chunk_offset;		/*  if chunk[i]==chunk[i+1] => empty. One extra chunk offset at end to give size of final chunk */
    int scaf_flags;
    int ns;
    int scaffold_size;
    int *scaffold_offset;
    char *scaffold_indexsize;
    char *fontname;
    char *metrics_fontname;
    int metrics_n;
    int *xadvance, *yadvance;
    SplineFont *sf;
};

/* Little-endian */
static int r_getushort(FILE *file) {
    int ch1;

    ch1 = getc(file);
return( (getc(file)<<8)|ch1 );
}

static int r_getshort(FILE *file) {
return( (short) r_getushort(file));
}

static int r_getint(FILE *file) {
    int ch1, ch2, ch3;

    ch1 = getc(file);
    ch2 = getc(file);
    ch3 = getc(file);
return( (((((getc(file)<<8)|ch3)<<8)|ch2)<<8)|ch1 );
}

void SplinePointFree(SplinePoint *sp) {
    free(sp);
}

static char *knownweights[] = { "Demi", "Bold", "Regu", "Medi", "Book", "Thin",
	"Ligh", "Heav", "Blac", "Ultr", "Nord", "Norm", "Gras", "Stan", "Halb",
	"Fett", "Mage", "Mitt", "Buch", NULL };
static char *realweights[] = { "Demi", "Bold", "Regular", "Medium", "Book", "Thin",
	"Light", "Heavy", "Black", "Ultra", "Nord", "Normal", "Gras", "Standard", "Halbfett",
	"Fett", "Mager", "Mittel", "Buchschrift", NULL};
static char *modifierlist[] = { "Ital", "Obli", "Kursive", "Cursive",
	"Expa", "Cond", NULL };
static char **mods[] = { knownweights, modifierlist, NULL };
#if 0
static char *modifierlistfull[] = { "Italic", "Oblique", "Kursive", "Cursive",
    "Expanded", "Condensed", NULL };
static char **fullmods[] = { realweights, modifierlistfull, NULL };
#endif


static char *GuessFamily(char *fontname) {
    char *fpt, *pt;
    int i,j;

    if ( (fpt=strchr(fontname,'-'))!=NULL && fpt!=fontname )
return( copyn(fontname,fpt-fontname));
    if ( (fpt=strchr(fontname,'.'))!=NULL && fpt!=fontname )
return( copyn(fontname,fpt-fontname));

    for ( i=0; mods[i]!=NULL; ++i ) for ( j=0; mods[i][j]!=NULL; ++j ) {
	pt = strstr(fontname,mods[i][j]);
	if ( pt!=NULL && (fpt==NULL || pt<fpt))
	    fpt = pt;
    }

    if ( fpt==NULL )
return( copy( fontname ));

return( copyn(fontname,fpt-fontname));
}

static char *GuessWeight(char *fontname) {
    int i;

    for ( i=0; knownweights[i]!=NULL; ++i )
	if ( strstr(fontname,knownweights[i])!=NULL )
return( copy( realweights[i]));

return( copy( "Regular" ));
}

static char *despace(char *fontname) {
    char *pt, *npt;

    fontname = copy(fontname);
    for ( pt=npt=fontname; *pt; ++pt )
	if ( *pt!=' ' )
	    *npt++ = *pt;
    *npt = '\0';
return( fontname );
}

static void readcoords(FILE *file,int is12, int *x, int *y) {
    int ch1, ch2, ch3;

    if ( is12 ) {
	ch1 = getc(file);
	ch2 = getc(file);
	ch3 = getc(file);
#if 0
	ch1 = (ch1<<4)|(ch2>>4);
	ch2 = ((ch2&0xf)<<8)|ch3;
#else
	ch1 = ch1|((ch2&0xf)<<8);
	ch2 = (ch2>>4)|(ch3<<4);
#endif
	*x = (ch1<<(32-12))>>(32-12);		/* Sign extend */
	*y = (ch2<<(32-12))>>(32-12);		/* Sign extend */
    } else {
	*x = (signed char) getc(file);
	*y = (signed char) getc(file);
    }
}

static int readcharindex(FILE *file,int twobyte) {
    if ( twobyte )
return( r_getushort(file));
    else
return( getc(file));
}

static SplineSet *FinishSet(SplineSet *old,SplineSet *active,int closed) {
    if ( active!=NULL ) {
	if ( active->first->me.x==active->last->me.x &&
		active->first->me.y==active->last->me.y &&
		active->first!=active->last ) {
	    active->first->prevcp = active->last->prevcp;
	    active->first->noprevcp = active->last->noprevcp;
	    active->last->prev->to = active->first;
	    active->first->prev = active->last->prev;
	    SplinePointFree(active->last);
	    active->last = active->first;
	} else if ( closed ) {
	    SplineMake(active->last,active->first);
	    active->last = active->first;
	}
	active->next = old;
return( active );
    }
return( old );
}

static SplineSet *ReadSplineSets(FILE *file,int flags,SplineSet *old,int closed) {
    int verb;
    int x1,y1, x2,y2, x3,y3;
    SplineSet *active=NULL;
    SplinePoint *next;

    while ( (verb=getc(file))!=EOF && (verb&0x3)!=0 ) {
	readcoords(file,flags&1,&x1,&y1);
	if ( (verb&0x3)==1 ) {		/* Move to */
	    old = FinishSet(old,active,closed);
	    active = gcalloc(1,sizeof(SplineSet));
	    active->first = active->last = gcalloc(1,sizeof(SplinePoint));
	    active->first->me.x = x1; active->first->me.y = y1;
	    active->first->nextcp = active->first->prevcp = active->first->me;
	    active->first->nonextcp = active->first->noprevcp = true;
	} else {
	    if ( active==NULL ) {
		fprintf( stderr, "No initial point, assuming 0,0\n" );
		active = gcalloc(1,sizeof(SplineSet));
		active->first = active->last = gcalloc(1,sizeof(SplinePoint));
		active->first->nonextcp = active->first->noprevcp = true;
	    }
	    next = gcalloc(1,sizeof(SplinePoint));
	    if ( (verb&3)==2 ) {		/* Line to */
		next->me.x = x1; next->me.y = y1;
		next->nextcp = next->prevcp = next->me;
		next->nonextcp = next->noprevcp = true;
	    } else {				/* Curve to */
		readcoords(file,flags&1,&x2,&y2);
		readcoords(file,flags&1,&x3,&y3);
		active->last->nextcp.x = x1; active->last->nextcp.y = y1;
		active->last->nonextcp = false;
		next->prevcp.x = x2; next->prevcp.y = y2;
		next->me.x = x3; next->me.y = y3;
		next->nextcp = next->me;
		next->nonextcp = true;
	    }
	    SplineMake(active->last,next);
	    active->last = next;
	}
    }
    ungetc(verb,file);
return( FinishSet(old,active,closed));
}

static SplineChar *ReadChar(FILE *file,struct Outlines *outline,int enc) {
    SplineChar *sc = SplineCharCreate();
    char buffer[12];
    int flags, x, y, verb, ch;
    RefChar *r1, *r2;

    flags = getc(file);
    if ( !(flags&(1<<3)) ) {
	fprintf( stderr, "Character %d is a bitmap character\n", enc );
return( NULL );
    }

    sc->parent = outline->sf;
    sc->enc = sc->unicodeenc = enc;
    sc->changedsincelasthinted = true; /* I don't understand the scaffold lines */
	/* which I think are the same as hints. So no hints processed. PfaEdit */
	/* should autohint the char */
    sprintf( buffer,"uni%04X", enc);
    sc->name = copy( psunicodenames[enc]==NULL ? buffer : psunicodenames[enc]);
    sc->width = sc->vwidth = sc->parent->ascent + sc->parent->descent;
    if ( outline->xadvance!=NULL )
	sc->width = outline->xadvance[enc];
    if ( outline->yadvance!=NULL )
	sc->vwidth = outline->yadvance[enc];

    if ( flags&(1<<4) ) {
	r1 = gcalloc(1,sizeof(RefChar));
	r1->transform[0] = r1->transform[3] = 1;
	r1->local_enc = readcharindex(file,flags&(1<<6));
	if ( flags&(1<<5) ) {
	    r2 = gcalloc(1,sizeof(RefChar));
	    r2->transform[0] = r2->transform[3] = 1;
	    r2->local_enc = readcharindex(file,flags&(1<<6));
	    readcoords(file,flags&1,&x,&y);
	    r2->transform[4] = x; r2->transform[5] = y;
	    r1->next = r2;
	}
	sc->refs = r1;
return(sc);
    }
    
    readcoords(file,flags&1,&x,&y);	/* bounding box, lbearing, ybase */
    readcoords(file,flags&1,&x,&y);	/* bounding box, width,height */

    sc->splines = ReadSplineSets(file,flags,NULL,true);
    verb = getc(file);
    if ( verb!=EOF && verb&(1<<2) ) {
	/* It looks to me as though the stroked paths are duplicates of the */
	/*  filled paths, I'm assuming they are some form of hinting (if pixel*/
	/*  size is too small then just stroke, otherwise fill?) */
	/* Every character has them... */
	/*fprintf( stderr, "There are some stroked paths in %s, you should run\n  Element->Expand Stroke on this character\n", sc->name );*/
	/*sc->splines =*/ ReadSplineSets(file,flags,/*sc->splines*/NULL,false);
	verb = getc(file);
    }
    if ( verb!=EOF && verb&(1<<3)) {
	while ( (ch=readcharindex(file,flags&(1<<6)))!=0 && !feof(file)) {
	    r1 = gcalloc(1,sizeof(RefChar));
	    r1->transform[0] = r1->transform[3] = 1;
	    r1->local_enc = ch;
	    readcoords(file,flags&1,&x,&y);
	    r1->transform[4] = x; r1->transform[5] = y;
	    r1->next = sc->refs;
	    sc->refs = r1;
	}
    }
return( sc );
}

static void ReadChunk(FILE *file,struct Outlines *outline,int chunk) {
    int flag=0x80000000;
    int i, offsets[32];
    int index_start;

    fseek(file,outline->chunk_offset[chunk],SEEK_SET);

    if ( outline->version>=7 )
	flag = r_getint(file);
    else if ( outline->version==6 )
	flag = (1<<7);
    if ( !flag&0x80000000 )
	fprintf( stderr, "Bad flag at beginning of chunk %d\n", chunk );

    index_start = ftell(file);
    for ( i=0; i<32; ++i )
	offsets[i] = r_getint(file);

    if ( flag & (1<<7) ) {
	/* Does this chunk have any composit characters which refer to things */
	/*  in other chunks? I don't really care so I ignore them all */
	for ( i=0; i<((outline->nchunks+7)>>3); ++i )
	    getc(file);
    }

    for ( i=0; i<32; ++i ) if ( offsets[i]!=0 && 32*chunk+i<outline->metrics_n ) {
	fseek(file,index_start+offsets[i],SEEK_SET);
	outline->sf->chars[32*chunk+i] = ReadChar(file,outline,32*chunk+i);
    }
}

static void ReadIntmetrics(char *dir, struct Outlines *outline) {
    char *filename = malloc(strlen(dir)+strlen("/Intmetrics")+3);
    FILE *file;
    int i, flags, m;
    char buffer[100];
    char *mapping=NULL, _map[256];
    static char *names[] = { "/intmetrics", "/INTMETRICS", "/IntMetrics", "/Intmetrics", NULL };

    for ( i=0, file=NULL; names[i]!=NULL && file==NULL; ++i ) {
	strcpy(filename,dir);
	strcat(filename,names[i]);
	file = fopen(filename,"r");
    }
    if ( file==NULL ) {
	fprintf(stderr,"Couldn't open %s (for advance width data)\n  Oh well, advance widths will all be wrong.\n", filename );
	free(filename);
return;
    }
    for ( i=0; i<40; ++i ) {
	buffer[i] = getc(file);
	if ( buffer[i]=='\r' ) buffer[i] = '\0';
    }
    buffer[i] = '\0';
    outline->metrics_fontname = copy(buffer);
    r_getint(file);	/* Must be 16 */
    r_getint(file);	/* Must be 16 */
    outline->metrics_n = getc(file);		/* low order byte */
    /* version number = */ getc(file);
    flags = getc(file);
    outline->metrics_n |= getc(file)<<8;	/* high order byte */
	/* This value is not reliable. In Trinity n==34, m==416 and */
	/* number of characters given in Outlines==416 */
    if ( flags&(1<<5) ) {
	m = r_getushort(file);
	if ( outline->metrics_n<m ) {
	    fprintf( stderr, "This IntMetrics file makes no sense. It claims there are %d characters in\n", outline->metrics_n );
	    fprintf( stderr, " the font, and then starts talking about %d of them. I have no idea how\n", m );
	    fprintf( stderr, " how to parse this and have given up. No advance widths are known.\n" );
	    outline->metrics_n = 0;
return;
	}
    } else
	m = 256;
    if ( m<=256 ) {
	memset(_map,0,sizeof(_map));
	for ( i=0; i<m; ++i )
	    _map[i] = getc(file);
	if ( m>0 )
	    mapping = _map;
    } else {
	/* Documentation clearly states that m must be less than 256 */
	/*  but Trinity has m of 416 */
	for ( i=0; i<m; ++i )
	    getc(file);
    }
    if ( !(flags&1) ) {
	/* I ignore bbox data */
	for ( i=0; i<4*outline->metrics_n; ++i )
	    r_getshort(file);
    }
    if ( !(flags&2) ) {
	outline->xadvance = gcalloc(outline->metrics_n,sizeof(int));
	for ( i=0; i<outline->metrics_n; ++i )
	    if ( mapping )
		outline->xadvance[mapping[i]] = r_getshort(file);
	    else
		outline->xadvance[i] = r_getshort(file);
    }
    if ( !(flags&4) ) {
	outline->yadvance = gcalloc(outline->metrics_n,sizeof(int));
	for ( i=0; i<outline->metrics_n; ++i )
	    if ( mapping )
		outline->yadvance[mapping[i]] = r_getshort(file);
	    else
		outline->yadvance[i] = r_getshort(file);
    }
    fclose(file);
    /* There might be kern data, but I shan't read them */
}

static void FixupRefs(SplineChar *sc,SplineFont *sf) {
    RefChar *rf, *prev, *next;

    if ( sc==NULL || sc->refs==NULL )
return;
    prev = NULL;
    for ( rf = sc->refs; rf!=NULL; rf=next ) {
	next = rf->next;
	if ( rf->local_enc<0 || rf->local_enc>=sf->charcnt || sf->chars[rf->local_enc]==NULL ) {
	    fprintf( stderr, "%s contains a reference to a character at index %d which does not exist.\n",
		sc->name, rf->local_enc );
	    if ( prev==NULL )
		sc->refs = next;
	    else
		prev->next = next;
	    free(rf);
	} else {
	    rf->sc = sf->chars[rf->local_enc];
	    rf->adobe_enc = getAdobeEnc(rf->sc->name);
	    prev = rf;
	}
    }
}

static SplineFont *ReadOutline(char *dir) {
    char *filename = malloc(strlen(dir)+strlen("/Outlines")+3);
    FILE *file;
    struct Outlines outline;
    int i, ch;
    char buffer[100];
    static char *names[] = { "/outlines", "/OUTLINES", "/Outlines", NULL };

    for ( i=0, file=NULL; names[i]!=NULL && file==NULL; ++i ) {
	strcpy(filename,dir);
	strcat(filename,names[i]);
	file = fopen(filename,"r");
    }
    if ( file==NULL ) {
	fprintf(stderr,"Couldn't open %s\n", filename );
	free(filename);
return( NULL );
    }

    if ( getc(file)!='F' || getc(file)!='O' || getc(file)!='N' || getc(file)!='T' ||
	    getc(file)!='\0') {	/* Final null means outline font */
	fprintf(stderr, "%s is not an acorn risc outline font file\n", filename );
	free(filename);
	fclose(file);
return( NULL );
    }

    memset(&outline,0,sizeof(outline));

    ReadIntmetrics(dir,&outline);

    outline.version = getc(file);
    outline.design_size = r_getushort(file);

    /* bounding box, I don't care */
    /* minx = */ r_getshort(file);
    /* miny = */ r_getshort(file);
    /* width = */ r_getshort(file);
    /* height = */ r_getshort(file);

    if ( outline.version<8 ) {
	outline.nchunks = 8;
	outline.chunk_offset = malloc(9*sizeof(int));
	for ( i=0; i<9; ++i )
	    outline.chunk_offset[i] = r_getint(file);
	outline.scaf_flags = 0;
	outline.ns = -1;
    } else {
	int chunk_off = r_getint(file), pos;
	outline.nchunks = r_getint(file);
	outline.ns = r_getint(file);
	outline.scaf_flags = r_getint(file);
	for ( i=0; i<5; ++i )
	    /* MBZ = */ r_getint(file);
	pos = ftell(file);
	fseek(file,chunk_off,SEEK_SET);
	outline.chunk_offset = malloc((outline.nchunks+1)*sizeof(int));
	for ( i=0; i<=outline.nchunks; ++i )
	    outline.chunk_offset[i] = r_getint(file);
	fseek(file,pos,SEEK_SET);
    }

    /* I really have no idea what these scaffold thingies are */
    outline.scaffold_size = r_getushort(file);
    if ( outline.ns==-1 )
	outline.ns = outline.scaffold_size/2;
    else if ( 2*outline.ns+1 > outline.scaffold_size )
	fprintf( stderr, "Inconsistant scaffold count\n" );
    /* I don't understand the scaffold stuff. */
    /* there should be outline.ns-1 shorts of offsets, followed by a byte */
    /*  followed by the data the offsets point to. I'm just going to skip */
    /*  all of it. */
    for ( i=0; i<outline.scaffold_size-sizeof(short); ++i )
	getc(file);
    for ( i=0; (ch=getc(file))!='\0' && ch!=EOF; )
	if ( i<sizeof(buffer)-1 ) buffer[i++] = ch;
    buffer[i] = '\0';
    outline.fontname = strdup(buffer);
    for ( i=0; (ch=getc(file))!='\0' && ch!=EOF; )
	if ( i<sizeof(buffer)-1 ) buffer[i++] = ch;
    buffer[i] = '\0';
    /* Docs say that the word "Outlines" appears here, followed by a nul */
    /* That appears to be a lie. We seem to get a random comment */
    /* or perhaps a copyright notice */
#if 0
    if ( strcmp(buffer,"Outlines")!=0 ) {
	fprintf( stderr, "Outlines keyword missing after fontname in %s\n", filename );
	free(filename);
	free(outline.chunk_offset);
	free(outline.fontname);
	fclose(file);
return( NULL );
    }
#endif
    if ( outline.metrics_fontname!=NULL &&
	    strmatch(outline.fontname,outline.metrics_fontname)!=0 )
	fprintf( stderr, "Warning: Fontname in metrics (%s) and fontname in outline (%s)\n do not match.\n", outline.metrics_fontname, outline.fontname );
    i = (outline.nchunks<8?8:outline.nchunks)*32;
    if ( outline.metrics_n==0 || outline.metrics_n>i )
	outline.metrics_n = i;

    outline.sf = SplineFontEmpty();
    outline.sf->charcnt = outline.metrics_n;
    outline.sf->chars = gcalloc(outline.sf->charcnt,sizeof(SplineChar *));
    outline.sf->fontname = despace(outline.fontname);
    outline.sf->fullname = copy(outline.fontname);
    outline.sf->familyname = GuessFamily(outline.fontname);
    outline.sf->weight = GuessWeight(outline.fontname);
    if ( strcmp(buffer,"Outlines")!=0 )
	outline.sf->copyright = copy(buffer);
    strcpy(buffer,outline.fontname);
    strcat(buffer,".sfd");
    outline.sf->filename = copy(buffer);

    outline.sf->ascent = 4*outline.design_size/5;
    outline.sf->descent = outline.design_size - outline.sf->ascent;

    outline.sf->display_antialias = true;
    outline.sf->display_size = -24;

    for ( i=0; i<outline.nchunks; ++i ) {
	if ( outline.chunk_offset[i]!=outline.chunk_offset[i+1] ) {
	    ReadChunk(file,&outline,i);
	}
    }

    for ( i=0; i<outline.sf->charcnt; ++i )
	FixupRefs(outline.sf->chars[i],outline.sf);

    free(filename);
    fclose(file);

    if ( !SFDWrite(outline.sf->filename,outline.sf) )
	fprintf( stderr, "Failed to write outputfile %s\n", outline.sf->filename );
    else
	fprintf( stderr, "Created: %s\n", outline.sf->filename );
return( outline.sf );
}

extern const char *source_modtime_str;

static void doversion(void) {
    extern const char *source_version_str;

    fprintf( stderr, "Copyright \251 2002 by George Williams.\n Executable based on sources from %s.\n",
	    source_modtime_str );

    printf( "acorn2sfd %s\n", source_version_str );
exit(0);
}

static void dousage(void) {
    printf( "acorn2sfd [options] {acorndirs}\n" );
    printf( "\t-version\t (prints the version of acorn2sfd and exits)\n" );
    printf( "\t-help\t\t (prints a brief help text and exits)\n" );
    printf( "For more information see:\n\thttp://pfaedit.sourceforge.net/\n" );
    printf( "Send bug reports to:\tgww@silcom.com\n" );
exit(0);
}

static void dohelp(void) {
    printf( "acorn2sfd -- reads an acorn RISCOS font and creates an sfd file.\n" );
    printf( " Acorn RISCOS fonts consist of two files \"Outlines\" and \"Intmetrics\" in a\n" );
    printf( " directory. This program takes a list of directory names and attempts to\n" );
    printf( " convert the font data within each directory to one of PfaEdit's sfd files\n\n" );
    dousage();
}

int main(int argc, char **argv) {
    int i, any=false;
    char *pt;
    
    for ( i=1; i<argc; ++i ) {
	if ( *argv[i]=='-' ) {
	    pt = argv[i]+1;
	    if ( *pt=='-' )
		++pt;
	    if ( strcmp(pt,"version")==0 )
		doversion();
	    else if ( strlen(pt)<=4 && strncmp(pt,"help",strlen(pt))==0 )
		dohelp();
	    else
		dousage();
	} else {
	    ReadOutline(argv[i]);
	    any = true;
	}
    }
    if ( !any )
	ReadOutline(".");
	    
return( 0 );
}
