/* acorn2sfd.c.
 * This program converts outline fonts used with RISC OS (from
 * http://www.riscosopen.org or http://riscos.com) to .sfd files.
 *
 * To build, run:
 *  make acorn2sfd
 * in the fontforgeexe directory.  It has been tested on Linux. I have
 * not tried compiling it on Windows.
 *
 * To use:
 *  acorn2sfd [path_to_font_directory]
 * eg:
 *  acorn2sfd \!Fonts/NewHall/Medium/Italic
 *
 * remove any ,ff6 suffix on any files in the font directory before running
 * acorn2sfd.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <fontforge-config.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <gutils.h>

#include "splineutil.h"
#include "cvundoes.h"
#include "lookups.h"
#include "mem.h"
#include "namelist.h"
#include "sfd.h"
#include "splinefont.h"
#include "splineutil2.h"
#include "start.h"
#include "ustring.h"	/* read fontforge inc/ustring.h */

static int includestrokes = false;

extern Encoding custom;
extern NameList *namelist_for_new_fonts;

#define true	1
#define false	0

struct r_kern {
    int right;
    int amount;
    struct r_kern *next;
};

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
    int defxadvance, defyadvance;
    struct r_kern **kerns;
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

static char *get_font_filename(const char *str)
{
    char *fname = copy(str);
    char *c;

    for (c = fname; *c; c++) {
	/* Change non-breaking space into a normal space */
	if ((*c & 0xff) == 0xa0) {
	    *c = ' ';
	}
    }

    return fname;
}

static const char *modifierlist[] = { "Ital", "Obli", "Kursive", "Cursive",
	"Slanted", "Expa", "Cond", NULL };
static const char **mods[] = { knownweights, modifierlist, NULL };


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
	ch1 = ch1|((ch2&0xf)<<8);
	ch2 = (ch2>>4)|(ch3<<4);
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
	    SplineMake3(active->last,active->first);
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
	    active = calloc(1,sizeof(SplineSet));
	    active->first = active->last = calloc(1,sizeof(SplinePoint));
	    active->first->me.x = x1; active->first->me.y = y1;
	    active->first->nextcp = active->first->prevcp = active->first->me;
	    active->first->nonextcp = active->first->noprevcp = true;
	} else {
	    if ( active==NULL ) {
		fprintf( stderr, "No initial point, assuming 0,0\n" );
		active = calloc(1,sizeof(SplineSet));
		active->first = active->last = calloc(1,sizeof(SplinePoint));
		active->first->nonextcp = active->first->noprevcp = true;
	    }
	    next = calloc(1,sizeof(SplinePoint));
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
	    SplineMake3(active->last,next);
	    active->last = next;
	}
    }
    ungetc(verb,file);
return( FinishSet(old,active,closed));
}

static SplineChar *ReadChar(FILE *file,struct Outlines *outline,int enc) {
    char buffer[12];
    int flags, x, y, verb, ch;
    RefChar *r1, *r2;
    SplineFont *sf = outline->sf;
    SplineChar *sc = SFSplineCharCreate(sf);
    SplineSet * spUnused = NULL;

    flags = getc(file);
    if ( !(flags&(1<<3)) ) {
	fprintf( stderr, "Character %d is a bitmap character\n", enc );
        free(sc);
return( NULL );
    }

    sc->orig_pos = sf->glyphcnt++;
    sf->glyphs[sc->orig_pos] = sc;
    sf->map->map[enc] = sc->orig_pos;
    sf->map->backmap[sc->orig_pos] = enc;
    sc->unicodeenc = enc;
    sc->changedsincelasthinted = true; /* I don't understand the scaffold lines */
	/* which I think are the same as hints. So no hints processed. PfaEdit */
	/* should autohint the char */
    sc->name = copy( StdGlyphName(buffer,enc,ui_none,NULL));
    sc->width = (outline->defxadvance*(sf->ascent + sf->descent))/1000;
    sc->vwidth = (outline->defyadvance*(sf->ascent + sf->descent))/1000;
    sc->widthset = true;
    if ( outline->xadvance!=NULL )
	sc->width = (outline->xadvance[enc]*(sf->ascent+sf->descent))/1000;
    if ( outline->yadvance!=NULL )
	sc->vwidth = (outline->yadvance[enc]*(sf->ascent+sf->descent))/1000;

    if ( flags&(1<<4) ) {
	r1 = calloc(1,sizeof(RefChar));
	r1->transform[0] = r1->transform[3] = 1;
	r1->orig_pos = readcharindex(file,flags&(1<<6));
	if ( flags&(1<<5) ) {
	    r2 = calloc(1,sizeof(RefChar));
	    r2->transform[0] = r2->transform[3] = 1;
	    r2->orig_pos = readcharindex(file,flags&(1<<6));
	    readcoords(file,flags&1,&x,&y);
	    r2->transform[4] = x; r2->transform[5] = y;
	    r1->next = r2;
	}
	sc->layers[ly_fore].refs = r1;
return(sc);
    }
    
    readcoords(file,flags&1,&x,&y);	/* bounding box, lbearing, ybase */
    readcoords(file,flags&1,&x,&y);	/* bounding box, width,height */

    sc->layers[ly_fore].splines = ReadSplineSets(file,flags,NULL,true);
    verb = getc(file);
    if ( verb!=EOF && verb&(1<<2) ) {
	/* It looks to me as though the stroked paths are duplicates of the */
	/*  filled paths, I'm assuming they are some form of hinting (if pixel*/
	/*  size is too small then just stroke, otherwise fill?) */
	/* Every character has them... */
	/*fprintf( stderr, "There are some stroked paths in %s, you should run\n  Element->Expand Stroke on this character\n", sc->name );*/
	if ( includestrokes )
	    sc->layers[ly_fore].splines = ReadSplineSets(file,flags,sc->layers[ly_fore].splines,false);
	else {
	    spUnused = ReadSplineSets(file,flags,NULL,false);	/* read and ignore */
	    SplinePointListsFree(spUnused);
        }
	verb = getc(file);
    }
    if ( verb!=EOF && verb&(1<<3)) {
	while ( (ch=readcharindex(file,flags&(1<<6)))!=0 && !feof(file)) {
	    r1 = calloc(1,sizeof(RefChar));
	    r1->transform[0] = r1->transform[3] = 1;
	    r1->orig_pos = ch;
	    readcoords(file,flags&1,&x,&y);
	    r1->transform[4] = x; r1->transform[5] = y;
	    r1->next = sc->layers[ly_fore].refs;
	    sc->layers[ly_fore].refs = r1;
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
	flag |= (1<<7);
    if ( (~flag)&0x80000000 )
	fprintf( stderr, "Bad flag at beginning of chunk %d\n", chunk );

    index_start = ftell(file);
    for ( i=0; i<32; ++i )
	offsets[i] = r_getint(file);

    if ( flag & (1<<7) ) {
	/* Does this chunk have any composite characters which refer to things */
	/*  in other chunks? I don't really care so I ignore them all */
	for ( i=0; i<((outline->nchunks+7)>>3); ++i )
	    getc(file);
    }

    for ( i=0; i<32; ++i ) if ( offsets[i]!=0 && 32*chunk+i<outline->metrics_n ) {
	fseek(file,index_start+offsets[i],SEEK_SET);
	ReadChar(file,outline,32*chunk+i);
    }
}

/* Handles *?{}[] wildcards */
static int WildMatch(char *pattern, char *name,int ignorecase) {
    char ch, *ppt, *npt, *ept;

    if ( pattern==NULL )
return( true );

    while (( ch = *pattern)!='\0' ) {
	if ( ch=='*' ) {
	    for ( npt=name; ; ++npt ) {
		if ( WildMatch(pattern+1,npt,ignorecase))
return( true );
		if ( *npt=='\0' )
return( false );
	    }
	} else if ( ch=='?' ) {
	    if ( *name=='\0' )
return( false );
	    ++name;
	} else if ( ch=='[' ) {
	    /* [<char>...] matches the chars
	     * [<char>-<char>...] matches any char within the range (inclusive)
	     * the above may be concattenated and the resultant pattern matches
	     *		anything thing which matches any of them.
	     * [^<char>...] matches any char which does not match the rest of
	     *		the pattern
	     * []...] as a special case a ']' immediately after the '[' matches
	     *		itself and does not end the pattern */
	    int found = 0, not=0;
	    ++pattern;
	    if ( pattern[0]=='^' ) { not = 1; ++pattern; }
	    for ( ppt = pattern; (ppt!=pattern || *ppt!=']') && *ppt!='\0' ; ++ppt ) {
		ch = *ppt;
		if ( ppt[1]=='-' && ppt[2]!=']' && ppt[2]!='\0' ) {
		    int ch2 = ppt[2];
		    if ( (*name>=ch && *name<=ch2) ||
			    (ignorecase && islower(ch) && islower(ch2) &&
				    *name>=toupper(ch) && *name<=toupper(ch2)) ||
			    (ignorecase && isupper(ch) && isupper(ch2) &&
				    *name>=tolower(ch) && *name<=tolower(ch2))) {
			if ( !not ) {
			    found = 1;
	    break;
			}
		    } else {
			if ( not ) {
			    found = 1;
	    break;
			}
		    }
		    ppt += 2;
		} else if ( ch==*name || (ignorecase && tolower(ch)==tolower(*name)) ) {
		    if ( !not ) {
			found = 1;
	    break;
		    }
		} else {
		    if ( not ) {
			found = 1;
	    break;
		    }
		}
	    }
	    if ( !found )
return( false );
	    while ( *ppt!=']' && *ppt!='\0' ) ++ppt;
	    pattern = ppt;
	    ++name;
	} else if ( ch=='{' ) {
	    /* matches any of a comma separated list of substrings */
	    for ( ppt = pattern+1; *ppt!='\0' ; ppt = ept ) {
		for ( ept=ppt; *ept!='}' && *ept!=',' && *ept!='\0'; ++ept );
		for ( npt = name; ppt<ept; ++npt, ++ppt ) {
		    if ( *ppt != *npt && (!ignorecase || tolower(*ppt)!=tolower(*npt)) )
		break;
		}
		if ( ppt==ept ) {
		    char *ecurly = ept;
		    while ( *ecurly!='}' && *ecurly!='\0' ) ++ecurly;
		    if ( WildMatch(ecurly+1,npt,ignorecase))
return( true );
		}
		if ( *ept=='}' )
return( false );
		if ( *ept==',' ) ++ept;
	    }
	} else if ( ch==*name ) {
	    ++name;
	} else if ( ignorecase && tolower(ch)==tolower(*name)) {
	    ++name;
	} else
return( false );
	++pattern;
    }
    if ( *name=='\0' )
return( true );

return( false );
}

static int dirmatch(char *dirname, char *pattern,char *buffer) {
    DIR *dir;
    struct dirent *ent;

    dir = opendir(dirname);
    if ( dir==NULL )
return( -1 );		/* No dir */

    while (( ent = readdir(dir))!=NULL ) {
	if ( WildMatch(pattern,ent->d_name,true) ) {
	    strcpy(buffer,dirname);
	    strcat(buffer,"/");
	    strcat(buffer,ent->d_name);
	    closedir(dir);
return( 1 );		/* Good */
	}
    }
    closedir(dir);
return( 0 );		/* Not found */
}

static int dirfind(char *dir, char *pattern,char *buffer) {
    char *pt, *space;
    int ret = dirmatch(dir,pattern,buffer);

    if ( ret==-1 ) {
	/* Just in case the give us the pathspec for the Outlines file rather than the dir containing it */
	space = copy(dir);
	pt = strrchr(space,'/');
	if ( pt!=NULL ) {
	    *pt = '\0';
	    ret = dirmatch(space,pattern,buffer);
	}
	free( space );
    }
    if ( ret==-1 )
	ret = 0;
    if ( !ret ) {
	strcpy(buffer,dir);
	strcat(buffer,"/");
	strcat(buffer,pattern);
    }
return( ret );
}

static void ReadIntmetrics(char *dir, struct Outlines *outline) {
    char *filename = malloc(strlen(dir)+strlen("/Intmetrics")+3);
    FILE *file=NULL;
    int i, flags, m, n, left, right;
    int kern_offset, table_base, misc_offset;
    char buffer[100];
    uint8 *mapping=NULL;
    int *widths;
    struct r_kern *kern;

    if ( dirfind(dir, "IntMet?",filename) )
	file = fopen(filename,"rb");
    else if ( dirfind(dir, "Intmetric?",filename) )
	file = fopen(filename,"rb");
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
    n = getc(file);	/* low order byte */
    /* version number = */ getc(file);
    flags = getc(file);
    n |= getc(file)<<8;	/* high order byte */
    if ( flags&(1<<5) )
	m = r_getushort(file);
    else
	m = 256;
    if ( m!=0 ) {
	mapping = malloc(m);
	for ( i=0; i<m; ++i )
	    mapping[i] = getc(file);
	outline->metrics_n = m;
    } else
	outline->metrics_n = n;
    if ( !(flags&1) ) {
	/* I ignore bbox data */
	for ( i=0; i<4*n; ++i )
	    r_getshort(file);
    }
    if ( !(flags&2) ) {
	widths = malloc(n*sizeof(int));
	for ( i=0; i<n; ++i )
	    widths[i] = r_getshort(file);
	if ( mapping==0 )
	    outline->xadvance = widths;
	else {
	    outline->xadvance = calloc(outline->metrics_n,sizeof(int));
	    for ( i=0; i<m; ++i )
		outline->xadvance[i] = widths[mapping[i]];
	    free(widths);
	}
    }
    if ( !(flags&4) ) {
	widths = malloc(n*sizeof(int));
	for ( i=0; i<n; ++i )
	    widths[i] = r_getshort(file);
	if ( mapping==0 )
	    outline->yadvance = widths;
	else {
	    outline->yadvance = calloc(outline->metrics_n,sizeof(int));
	    for ( i=0; i<m; ++i )
		outline->yadvance[i] = widths[mapping[i]];
	    free(widths);
	}
    }
    if (m != 0)
        free(mapping);

    outline->defxadvance = outline->defyadvance = 1000;

    if ( flags&8 ) {
	table_base = ftell(file);
	misc_offset = r_getshort(file);
	kern_offset = r_getshort(file);
	if ( misc_offset!=0 ) {
	    fseek(file,misc_offset+table_base,SEEK_SET);
	    /* font bounding box */ r_getshort(file); r_getshort(file); r_getshort(file); r_getshort(file);
	    if ( (flags&2) )
		outline->defxadvance = r_getshort(file);
	    if ( (flags&4) )
		outline->defyadvance = r_getshort(file);
	}
	if ( kern_offset!=0 && !feof(file) && outline->metrics_n!=0 ) {
	    fseek(file,kern_offset+table_base,SEEK_SET);
	    outline->kerns = calloc(outline->metrics_n,sizeof(struct r_kern *));
	    if ( flags&(1<<6) ) {
		/* 16 bit */
		while ( (left=r_getshort(file))!=0 && !feof(file)) {
		    while ( (right=r_getshort(file))!=0 ) {
			if ( !(flags&2) && !feof(file)) {
			    kern = malloc(sizeof(struct r_kern));
			    kern->amount = r_getshort(file);
			    kern->right = right;
			    kern->next = outline->kerns[left];
			    outline->kerns[left] = kern;
			}
			if ( !(flags&4) )
			    /* I don't care about vertical kerning */ r_getshort(file);
		    }
		}
	    } else {
		while ( (left=getc(file))!=0 && !feof(file)) {
		    while ( (right=getc(file))!=0 && !feof(file)) {
			if ( !(flags&2) ) {
			    kern = malloc(sizeof(struct r_kern));
			    kern->amount = r_getshort(file);
			    kern->right = right;
			    kern->next = outline->kerns[left];
			    outline->kerns[left] = kern;
			}
			if ( !(flags&4) )
			    /* I don't care about vertical kerning */ r_getshort(file);
		    }
		}
	    }
	}
    }
    fclose(file);
    free(filename);
}

static void FixupKerns(SplineFont *sf,struct Outlines *outline) {
    int i;
    struct r_kern *kern;
    KernPair *kp;
    int em = sf->ascent+sf->descent;
    int gid1, gid2;
    struct lookup_subtable *subtable;

    if ( outline->kerns==NULL )
return;

    subtable = SFSubTableFindOrMake(sf,
		CHR('k','e','r','n'),CHR('l','a','t','n'), gpos_pair);

    for ( i=0; i<outline->metrics_n; ++i ) {
	gid1 = sf->map->map[i];
	if (gid1 == -1) {
	    continue;
	}
	for ( kern = outline->kerns[i]; kern!=NULL; kern=kern->next ) {
	    kp = calloc(1,sizeof(KernPair));
	    kp->off = em*kern->amount/1000;
	    kp->subtable = subtable;
	    gid2 = sf->map->map[kern->right];
	    if (gid2 != -1) {
		kp->sc = sf->glyphs[gid2];
	    }
	    kp->next = sf->glyphs[gid1]->kerns;
	    sf->glyphs[gid1]->kerns = kp;
	}
    }
}

static void FixupRefs(SplineChar *sc,SplineFont *sf) {
    RefChar *rf, *prev, *next;
    EncMap *map = sf->map;
    int gid;

    if ( sc==NULL || sc->layers[ly_fore].refs==NULL )
return;
    prev = NULL;
    for ( rf = sc->layers[ly_fore].refs; rf!=NULL; rf=next ) {
	next = rf->next;
	if ( rf->orig_pos<0 || rf->orig_pos>=map->enccount ||
		(gid = map->map[rf->orig_pos])==-1 ||
		sf->glyphs[gid]==NULL ) {
	    fprintf( stderr, "%s contains a reference to a character at index %d which does not exist.\n",
		sc->name, rf->orig_pos );
	    if ( prev==NULL )
		sc->layers[ly_fore].refs = next;
	    else
		prev->next = next;
	    free(rf);
	} else {
	    rf->orig_pos = gid;
	    rf->sc = sf->glyphs[gid];
	    rf->adobe_enc = getAdobeEnc(rf->sc->name);
	    prev = rf;
	}
    }
}

static void FindEncoding(SplineFont *sf,char *filename) {
    char *pt, *end;
    char pattern[12];
    char *otherdir;
    char *encfilename;
    FILE *file=NULL;
    char buffer[200];
    int pos, gid;

    strcpy(pattern,"Base *");
    pattern[4] = filename[strlen(filename)-1];
    pt = strrchr(filename,'/');
    if ( pt!=NULL )
	*pt = '\0';
    otherdir = malloc(strlen(filename)+strlen("/../Encodings")+5);
    strcpy(otherdir,filename);
    strcat(otherdir,"/../Encodings");
    encfilename = malloc(strlen(otherdir)+strlen("base0encoding")+20);

    if ( dirfind(otherdir, pattern,encfilename) )
	file = fopen(encfilename,"rb");
    else if ( dirfind(filename, pattern,encfilename) )
	file = fopen(encfilename,"rb");
    free(otherdir);

    if ( file==NULL ) {
	fprintf(stderr,"Couldn't open %s\n", encfilename );
	free(encfilename);
return;
    }

    pos = 0;
    while ( fgets(buffer,sizeof(buffer),file)!=NULL ) {
	if ( *buffer=='%' || *buffer=='\n' )
    continue;
	for ( pt = buffer; *pt!='\0' ; ) {
	    while ( isspace(*pt)) ++pt;
	    if ( *pt=='/' ) {
		for ( end = ++pt; !isspace(*end) && *end!='\0'; ++end );
		if ( (gid=sf->map->map[pos])!=-1 && sf->glyphs[gid]!=NULL ) {
		    free(sf->glyphs[gid]->name);
		    sf->glyphs[gid]->name = copyn(pt,end-pt);
		    sf->glyphs[gid]->unicodeenc = UniFromName(sf->glyphs[gid]->name,ui_none,&custom);
		}
		++pos;
		pt = end;
	    } else
	break;
	}
    }
    free(encfilename);
    fclose(file);
}

static SplineFont *ReadOutline(char *dir) {
    char *filename = malloc(strlen(dir)+strlen("/Outlines*")+3);
    FILE *file = NULL;
    struct Outlines outline;
    int i, ch;
    char buffer[100];

    if ( dirfind(dir, "Outlines*",filename) )
	file = fopen(filename,"rb");
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
    if ( outline.metrics_fontname!=NULL &&
	    strmatch(outline.fontname,outline.metrics_fontname)!=0 )
	fprintf( stderr, "Warning: Fontname in metrics (%s) and fontname in outline (%s)\n do not match.\n", outline.metrics_fontname, outline.fontname );
    i = (outline.nchunks<8?8:outline.nchunks)*32;
    if ( outline.metrics_n==0 || outline.metrics_n>i )
	outline.metrics_n = i;

    outline.sf = SplineFontEmpty();
    outline.sf->glyphmax = outline.metrics_n;
    outline.sf->glyphcnt = 0;
    outline.sf->glyphs = calloc(outline.sf->glyphmax,sizeof(SplineChar *));
    outline.sf->map = calloc(1,sizeof(EncMap));
    outline.sf->map->enc = &custom;
    outline.sf->map->encmax = outline.sf->map->enccount = outline.sf->map->backmax = outline.sf->glyphmax;
    outline.sf->map->map = malloc(outline.sf->glyphmax*sizeof(int32));
    outline.sf->map->backmap = malloc(outline.sf->glyphmax*sizeof(int32));
    memset(outline.sf->map->map,-1,outline.sf->glyphmax*sizeof(int32));
    memset(outline.sf->map->backmap,-1,outline.sf->glyphmax*sizeof(int32));
    outline.sf->for_new_glyphs = namelist_for_new_fonts;
    outline.sf->fontname = despace(outline.fontname);
    outline.sf->fullname = copy(outline.fontname);
    outline.sf->familyname = GuessFamily(outline.fontname);
    outline.sf->weight = GuessWeight(outline.fontname);
    if ( strcmp(buffer,"Outlines")!=0 )
	outline.sf->copyright = copy(buffer);
    strcpy(buffer,outline.fontname);
    strcat(buffer,".sfd");
    outline.sf->filename = get_font_filename(buffer);

    outline.sf->top_enc = -1;

    outline.sf->ascent = 4*outline.design_size/5;
    outline.sf->descent = outline.design_size - outline.sf->ascent;

    outline.sf->display_antialias = true;
    outline.sf->display_size = -24;

    for ( i=0; i<outline.nchunks; ++i ) {
	if ( outline.chunk_offset[i]!=outline.chunk_offset[i+1] ) {
	    ReadChunk(file,&outline,i);
	}
    }

    FixupKerns(outline.sf,&outline);

    for ( i=0; i<outline.sf->glyphcnt; ++i )
	FixupRefs(outline.sf->glyphs[i],outline.sf);

    if ( isdigit(filename[strlen(filename)-1]) )
	FindEncoding(outline.sf,filename);

    free(filename);
    fclose(file);

    if ( !SFDWrite(outline.sf->filename,outline.sf,outline.sf->map,NULL,false) )
	fprintf( stderr, "Failed to write outputfile %s\n", outline.sf->filename );
    else
	fprintf( stderr, "Created: %s\n", outline.sf->filename );
return( outline.sf );
}

static void dousage(void) {
    printf( "acorn2sfd [options] {acorndirs}\n" );
    printf( "\t-version\t (prints the version of acorn2sfd and exits)\n" );
    printf( "\t-help\t\t (prints a brief help text and exits)\n" );
    printf( "For more information see:\n\thttp://github.com/fontforge/\n" );
    printf( "Send bug reports to:\tfontforge-devel@lists.sourceforge.net\n" );
exit(0);
}

static void dohelp(void) {
    printf( "acorn2sfd -- reads an acorn RISCOS font and creates an sfd file.\n" );
    printf( " Acorn RISCOS fonts consist of two files \"Outlines\" and \"Intmetrics\" in a\n" );
    printf( " directory. This program takes a list of directory names and attempts to\n" );
    printf( " convert the font data within each directory to one of FontForge's sfd files\n\n" );
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
	    if ( strcmp(pt,"includestrokes")==0 )
		includestrokes = true;
	    else if ( strcmp(pt,"version")==0 )
		doversion(NULL);
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
