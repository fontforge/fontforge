/* Copyright (C) 2002-2009 by George Williams */
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
#include <ustring.h>

extern const char *source_modtime_str;

void doversion(void) {
    extern const char *source_version_str;
    fprintf( stderr, "Copyright \251 2002 by George Williams.\n Executable based on sources from %s.\n",
	    source_modtime_str );

    printf( "sfddiff %s\n", source_version_str );
exit(0);
}

static void dousage(void) {
    printf( "sfddiff [options] sfdfile1 sfdfile2\n" );
    printf( "\t-version\t (prints the version of sfddiff and exits)\n" );
    printf( "\t-help\t\t (prints a brief help text and exits)\n" );
    printf( "\t-ignorehints\t (differences in hints don't count)\n" );
    printf( "\t-merge out\t (merges the two, with a background copy of diffs from\n" );
    printf( "\t\t\t  font2))\n\n" );
    printf( "For more information see:\n\thttp://fontforge.sourceforge.net/\n" );
    printf( "Send bug reports to:\tgww@silcom.com\n" );
exit(0);
}

static void dohelp(void) {
    printf( "sfddiff -- compares two of fontforge's sfd files.\n" );
    printf( " It notices what characters are present in one spline font database and not\n" );
    printf( " in the other. It notices which characters have different shapes, and which\n" );
    printf( " have different hints.\n\n" );
    dousage();
}

static SplineChar *FindName(SplineFont *sf,char *name, int guess) {
    int i;

    if ( guess>=0 && guess<sf->glyphcnt && sf->glyphs[guess]!=NULL &&
	strcmp(sf->glyphs[guess]->name,name)==0 )
return( sf->glyphs[guess] );
    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	if ( strcmp(sf->glyphs[i]->name,name)==0 )
return( sf->glyphs[i] );
    }
return( NULL );
}

static int StemDiff(StemInfo *h1, StemInfo *h2) {
    while ( h1!=NULL && h2!=NULL ) {
	if ( h1->start!=h2->start || h1->width!=h2->width )
return( false );
	h1 = h1->next; h2 = h2->next;
    }
    if ( h1==NULL && h2==NULL )
return( true );

return( false );
}

static int DStemDiff(DStemInfo *h1, DStemInfo *h2) {
    while ( h1!=NULL && h2!=NULL ) {
	if (h1->left.x != h2->left.x || h1->left.y != h2->left.y ||
	    h1->right.x != h2->right.x || h1->right.y != h2->right.y ||
	    h1->unit.x!=h2->unit.x || h1->unit.y!=h2->unit.y )
return( false );
	h1 = h1->next; h2 = h2->next;
    }
    if ( h1==NULL && h2==NULL )
return( true );

return( false );
}

static int MDDiff(MinimumDistance *h1, MinimumDistance *h2) {
    while ( h1!=NULL && h2!=NULL ) {
	if ( h1->sp1==NULL && h2->sp1==NULL )
	    /* Ok so far */;
	else if ( h1->sp1==NULL || h2->sp1==NULL )
return( false );
	else if ( h1->sp1->me.x!=h2->sp1->me.x || h1->sp1->me.y!=h2->sp1->me.y )
return( false );
	if ( h1->sp2==NULL && h2->sp2==NULL )
	    /* Ok so far */;
	else if ( h1->sp2==NULL || h2->sp2==NULL )
return( false );
	else if ( h1->sp2->me.x!=h2->sp2->me.x || h1->sp2->me.y!=h2->sp2->me.y )
return( false );
	if ( h1->x!=h2->x )
return( false );
	h1 = h1->next; h2 = h2->next;
    }
    if ( h1==NULL && h2==NULL )
return( true );

return( false );
}

static int HintDiff(SplineChar *sc1,SplineChar *sc2,int preverrs) {
    if ( !StemDiff(sc1->hstem,sc2->hstem) ||
	    !StemDiff(sc1->vstem,sc2->vstem) ||
	    !DStemDiff(sc1->dstem,sc2->dstem) ||
	    !MDDiff(sc1->md,sc2->md) ) {
	if ( !preverrs ) printf( "Differences in Enc=%-5d U+%04X %s\n",
		sc1->orig_pos, sc1->unicodeenc, sc1->name );
	printf( "\tHints differ\n" );
return( true );
    }
return( preverrs );
}

static RefChar *HasRef(SplineChar *sc,RefChar *r1) {
    RefChar *r;

    /* First look for one with exactly the same transform */
    for ( r=sc->layers[ly_fore].refs; r!=NULL; r=r->next )
	if ( strcmp(r->sc->name,r1->sc->name)==0 &&
		r->transform[0]==r1->transform[0] &&
		r->transform[1]==r1->transform[1] &&
		r->transform[2]==r1->transform[2] &&
		r->transform[3]==r1->transform[3] &&
		r->transform[4]==r1->transform[4] &&
		r->transform[5]==r1->transform[5] )
return( r );

    /* If that fails try again with just same name */
    for ( r=sc->layers[ly_fore].refs; r!=NULL; r=r->next )
	if ( strcmp(r->sc->name,r1->sc->name)==0 )
return( r );

return( NULL );
}

static int RefsDiff(SplineChar *sc1, SplineChar *sc2, int preverrs, int firstpass ) {
    RefChar *r1, *r2;

    for ( r1=sc1->layers[ly_fore].refs; r1!=NULL; r1=r1->next ) {
	r2 = HasRef(sc2,r1);
	if ( r2==NULL ) {
	    if ( !preverrs ) printf( "Differences in Enc=%-5d U+%04X %s\n",
		    sc1->orig_pos, sc1->unicodeenc, sc1->name );
	    preverrs = true;
	    printf( "\tFont%d refers to Enc=%-5d U+%04X %s\n",
		    firstpass?1:2,r1->sc->orig_pos, r1->sc->unicodeenc, r1->sc->name);
	} else if ( firstpass &&
		(r1->transform[0]!=r2->transform[0] ||
		 r1->transform[1]!=r2->transform[1] ||
		 r1->transform[2]!=r2->transform[2] ||
		 r1->transform[3]!=r2->transform[3] ||
		 r1->transform[4]!=r2->transform[4] ||
		 r1->transform[5]!=r2->transform[5] )) {
	    if ( !preverrs ) printf( "Differences in Enc=%-5d U+%04X %s\n",
		    sc1->orig_pos, sc1->unicodeenc, sc1->name );
	    preverrs = true;
	    printf( "\tDifferent transform in reference to Enc=%-5d U+%04X %s\n",
		    r1->sc->orig_pos, r1->sc->unicodeenc, r1->sc->name);
	    printf( "\t\t[%g,%g,%g,%g,%g,%g] vs. [%g,%g,%g,%g,%g,%g]\n",
		r1->transform[0], r1->transform[1], r1->transform[2],
		r1->transform[3], r1->transform[4], r1->transform[5],
		r2->transform[0], r2->transform[1], r2->transform[2],
		r2->transform[3], r2->transform[4], r2->transform[5] );
	}
    }
return( preverrs );
}

static int SSMatch(SplinePoint *first1, SplinePoint *first2) {
    SplinePoint *sp1, *sp2;

    for ( sp1=first1, sp2=first2; ; ) {
	if ( sp1->me.x!=sp2->me.x ||
		sp1->me.y!=sp2->me.y ||
		sp1->nextcp.x!=sp2->nextcp.x ||
		sp1->nextcp.y!=sp2->nextcp.y ||
		sp1->prevcp.x!=sp2->prevcp.x ||
		sp1->prevcp.y!=sp2->prevcp.y )
return( false );
	if ( sp1->next==NULL && sp2->next==NULL )
return( true );
	if ( sp1->next==NULL || sp2->next==NULL )
return( false );
	sp1 = sp1->next->to;
	sp2 = sp2->next->to;
	if ( sp1==first1 && sp2==first2 )
return( true );
	if ( sp1==first1 || sp2==first2 )
return( false );
    }
}

static int SSBackMatch(SplinePoint *first1, SplinePoint *first2) {
    SplinePoint *sp1, *sp2;

    for ( sp1=first1, sp2=first2; ; ) {
	if ( sp1->me.x!=sp2->me.x ||
		sp1->me.y!=sp2->me.y ||
		sp1->nextcp.x!=sp2->prevcp.x ||
		sp1->nextcp.y!=sp2->prevcp.y ||
		sp1->prevcp.x!=sp2->nextcp.x ||
		sp1->prevcp.y!=sp2->nextcp.y )
return( false );
	if ( sp1->next==NULL && sp2->prev==NULL )
return( true );
	if ( sp1->next==NULL || sp2->prev==NULL )
return( false );
	sp1 = sp1->next->to;
	sp2 = sp2->prev->from;
	if ( sp1==first1 && sp2==first2 )
return( true );
	if ( sp1==first1 || sp2==first2 )
return( false );
    }
}

static int SSSearchMatch(SplineSet *ss1,SplineSet *ss2) {
    SplinePoint *sp2;

    for ( sp2 = ss2->first; ; ) {
	if ( SSMatch(ss1->first,sp2) )
return( 2 );
	else if ( SSBackMatch(ss1->first,sp2) )
return( 3 );
	if ( sp2->next==NULL )
return( 0 );
	sp2 = sp2->next->to;
	if ( sp2==ss2->first )
return( 0 );
    }
}

static int SSsMatch(SplineSet *ss1,SplineSet *spl) {
    int ret;

    while ( spl!=NULL ) {
	if ( spl->first->ptindex==0 ) {
	    if ( SSMatch(ss1->first,spl->first)) {
		spl->first->ptindex = 1;
return( 0 );
	    } else if ( SSBackMatch(ss1->first,spl->first))
return( 1 );
	    ret = SSSearchMatch(ss1,spl);
	    if ( ret==2 || ret==3 )
return( ret );
	}
	spl = spl->next;
    }
return( 4 );
}

static int SplineDiff(SplineChar *sc1,SplineChar *sc2,int preverrs) {
    int cnt1, cnt2, diff;
    SplineSet *ss1, *ss2;

    for ( ss1=sc1->layers[ly_fore].splines, cnt1=0; ss1!=NULL; ss1=ss1->next, ++cnt1 );
    for ( ss2=sc2->layers[ly_fore].splines, cnt2=0; ss2!=NULL; ss2=ss2->next, ++cnt2 );
    if ( cnt1!=cnt2 ) {
	if ( !preverrs ) printf( "Differences in Enc=%-5d U+%04X %s\n",
		sc1->orig_pos, sc1->unicodeenc, sc1->name );
	printf( "\tDifferent number of contours\n" );
return( true );
    }
    for ( ss1=sc1->layers[ly_fore].splines; ss1!=NULL; ss1=ss1->next ) {
	diff = SSsMatch(ss1,sc2->layers[ly_fore].splines);
	if ( diff==0 )
    continue;		/* same */
	else {
	    if ( !preverrs ) printf( "Differences in Enc=%-5d U+%04X %s\n",
		    sc1->orig_pos, sc1->unicodeenc, sc1->name );
	    if ( diff==1 )
		printf( "\tContour has changed direction\n" );
	    else if ( diff==2 )
		printf( "\tContour has changed its start point\n" );
	    else if ( diff==3 )
		printf( "\tContour has changed both its direction and its start point\n" );
	    else
		printf( "\tContours differ\n" );
return( true );
	}
    }
return( preverrs );
}

static void dodiff( SplineFont *sf1, SplineFont *sf2, int checkhints,
	char *outfilename ) {
    int i, any, adiff=0, extras, oldcnt;
    SplineChar *sc, *sc1;

    if ( sf1->map->enc != sf2->map->enc ) {
	printf( "The two fonts have different encodings\n" );
	adiff = 1;
    }
    if ( sf1->order2 != sf2->order2 ) {
	printf( "The two fonts have spline orders\n" );
	adiff = 1;
    }
    if ( strcmp(sf1->fontname,sf2->fontname)!=0 ||
	    strcmp(sf1->familyname,sf2->familyname)!=0 ||
	    strcmp(sf1->fullname,sf2->fullname)!=0 ) {
	printf( "The two fonts have different names\n" );
	++adiff;
    }
    if ( strcmp(sf1->version,sf2->version)!=0 ) {
	printf( "The two fonts have different versions\n" );
	++adiff;
    }
    if ( strcmp(sf1->weight,sf2->weight)!=0 ) {
	printf( "The two fonts have different weights\n" );
	++adiff;
    }
    if ( sf1->copyright==NULL && sf2->copyright==NULL )
	/* Match */;
    else if ( sf1->copyright==NULL || sf2->copyright==NULL ||
	    strcmp(sf1->copyright,sf2->copyright)!=0 ) {
	printf( "The two fonts have different copyrights\n" );
	++adiff;
    }
    if ( sf1->comments==NULL && sf2->comments==NULL )
	/* Match */;
    else if ( sf1->comments==NULL || sf2->comments==NULL ||
	    strcmp(sf1->comments,sf2->comments)!=0 ) {
	printf( "The two fonts have different comments\n" );
	++adiff;
    }
    if ( !RealApprox(sf1->italicangle,sf2->italicangle) ) {
	printf( "The two fonts have different italic angles\n" );
	++adiff;
    }

    /* Look for any char in sf1 not in sf2 */
    any = false;
    for ( i=0; i<sf1->glyphcnt; ++i ) if ( sf1->glyphs[i]!=NULL ) {
	sc = FindName(sf2,sf1->glyphs[i]->name,i);
	if ( sc==NULL ) {
	    ++adiff;
	    if ( !any )
		printf( "Characters in font1 not in font2\n" );
	    any = true;
	    printf( "\tEnc=%-5d U+%04X %s\n", i, sf1->glyphs[i]->unicodeenc,
		    sf1->glyphs[i]->name );
	}
    }

    /* Look for any char in sf2 not in sf1 */
    extras = 0;
    any = false;
    for ( i=0; i<sf2->glyphcnt; ++i ) if ( sf2->glyphs[i]!=NULL ) {
	sc = FindName(sf1,sf2->glyphs[i]->name,i);
	if ( sc==NULL ) {
	    ++adiff; ++extras;
	    if ( !any )
		printf( "Characters in font2 not in font1\n" );
	    any = true;
	    printf( "\tEnc=%-5d U+%04X %s\n", i, sf2->glyphs[i]->unicodeenc,
		    sf2->glyphs[i]->name );
	}
    }

    /* Look for characters present in both which are different */
    for ( i=0; i<sf1->glyphcnt; ++i ) if ( (sc1=sf1->glyphs[i])!=NULL ) {
	sc = FindName(sf2,sc1->name,i);
	if ( sc!=NULL ) {
	    any = RefsDiff(sc1,sc,false,true);
	    any = RefsDiff(sc,sc1,any,false);
	    any = SplineDiff(sc,sc1,any);
	    if ( checkhints ) {
		HintDiff(sc1,sc,any);
	    }
	    if (! ( (sc->comment==NULL && sc1->comment==NULL) ||
		    (sc->comment!=NULL && sc1->comment!=NULL &&
			    strcmp(sc->comment,sc1->comment)==0) )) {
		if ( !any ) printf( "Differences in Enc=%-5d U+%04X %s\n",
			sc1->orig_pos, sc1->unicodeenc, sc1->name );
		printf( "\tComments are different\n" );
	    }
	    if ( any ) {
		sc1->layers[ly_back].splines = sc->layers[ly_fore].splines;
		++adiff;
	    }
	}
    }

    if ( adiff!=0 && outfilename!=NULL ) {
	if ( extras!=0 ) {
	    oldcnt = sf1->glyphcnt;
	    sf1->glyphcnt += extras;
	    sf1->glyphs = grealloc(sf1->glyphs,sf1->glyphcnt*sizeof(SplineChar *));
	    for ( i=oldcnt; i<sf1->glyphcnt; ++i )
		sf1->glyphs[i] = NULL;
	    extras = oldcnt;
	    for ( i=0; i<sf2->glyphcnt; ++i ) if ( sf2->glyphs[i]!=NULL ) {
		sc = FindName(sf1,sf2->glyphs[i]->name,i);
		if ( sc==NULL ) {
		    if ( i<oldcnt && sf1->glyphs[i]==NULL &&
			    sf1->map->enc == sf2->map->enc ) {
			sf1->glyphs[i] = sf2->glyphs[i];
			sf1->glyphs[i]->parent = sf1;
		    } else {
			sf1->glyphs[extras] = sf2->glyphs[i];
			sf1->glyphs[extras]->orig_pos = extras;
			sf1->glyphs[extras++]->parent = sf1;
		    }
		    sf2->glyphs[i] = NULL;
		}
	    }
	}
	if ( !SFDWrite(outfilename,sf1,sf1->map,NULL) )
	    fprintf( stderr, "Failed to write output file: %s\n", outfilename );
    }
}

int main(int argc, char **argv) {
    SplineFont *sf[2];
    char *outfilename = NULL;
    int checkhints = true;
    int i, which=0;
    char *pt;

/*
    fprintf( stderr, "Copyright \251 2002 by George Williams.\n Executable based on sources from %s.\n",
	    source_modtime_str );
*/

    for ( i=1; i<argc; ++i ) {
	if ( *argv[i]=='-' ) {
	    pt = argv[i]+1;
	    if ( *pt=='-' )
		++pt;
	    if ( strcmp(pt,"version")==0 )
		doversion();
	    else if ( strlen(pt)<=4 && strncmp(pt,"help",strlen(pt))==0 )
		dohelp();
	    else if ( strcmp(pt,"ignorehints")==0 )
		checkhints = false;
	    else if ( strcmp(pt,"merge")==0 )
		outfilename = argv[++i];
	    else
		dousage();
	} else if ( which<2 ) {
	    sf[which] = SFDRead(argv[i]);
	    if ( sf[which]==NULL ) {
		fprintf( stderr, "Could not open font database %s\n", argv[i] );
exit(1);
	    }
	    ++which;
	} else {
	    fprintf( stderr, "Too many filenames\n" );
	    dousage();
	}
    }
    if ( which!=2 ) {
	fprintf( stderr, "Too few filenames\n" );
	dousage();
    }
    dodiff( sf[0], sf[1], checkhints, outfilename );
return( 0 );
}
