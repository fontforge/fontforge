/* Copyright (C) 2001-2012 by George Williams */
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

#include "fontforge.h"		/* For LogError */
#include "othersubrs.h"
#include "splinefont.h"
#include "ustring.h"

/* These subroutines are code by Adobe for this exact use (from T1_Spec.pdf) */

	/* 3 0 callothersubr pop pop setcurrentpoint return */
static const uint8_t subrs0[] = { 3+139, 0+139, 12, 16, 12, 17, 12, 17, 12, 33, 11 };
	/* 0 1 callothersubr return */
static const uint8_t subrs1[] = { 0+139, 1+139, 12, 16, 11 };
	/* 0 2 callothersubr return */
static const uint8_t subrs2[] = { 0+139, 2+139, 12, 16, 11 };
	/* return */
static const uint8_t subrs3[] = { 11 };
	/* This one I created myself to do hint substitution */
	/* <subr number presumed to be on stack> 1 3 callother pop callsubr */
static const uint8_t subrs4[] = { 1+139, 3+139, 12, 16, 12, 17, 10, 11 };

	/* These others from adobe for multiple master */
	/* They need some fix up before they are used (the stack count depends on the # instances). */
	/* <n> 14 callothersubr pop return */
static const uint8_t subrs5[] = { 139, 14+139, 12, 16, 12, 17, 11 };
	/* 2*<n> 15 callothersubr pop pop return */
static const uint8_t subrs6[] = { 139, 15+139, 12, 16, 12, 17, 12, 17, 11 };
	/* 3*<n> 16 callothersubr pop pop pop return */
static const uint8_t subrs7[] = { 139, 16+139, 12, 16, 12, 17, 12, 17, 12, 17, 11 };
	/* 4*<n> 17 callothersubr pop pop pop pop return */
static const uint8_t subrs8[] = { 139, 17+139, 12, 16, 12, 17, 12, 17, 12, 17, 12, 17, 11 };
	/* 6*<n> 18 callothersubr pop pop pop pop  pop pop return */
static const uint8_t subrs9[] = { 139, 18+139, 12, 16, 12, 17, 12, 17, 12, 17, 12, 17, 12, 17, 12, 17, 11 };


const uint8_t *const subrs[] = { subrs0, subrs1, subrs2, subrs3, subrs4,
	subrs5, subrs6, subrs7, subrs8, subrs9 };
const int subrslens[] = { sizeof(subrs0), sizeof(subrs1), sizeof(subrs2),
	sizeof(subrs3), sizeof(subrs4), sizeof(subrs5), sizeof(subrs6),
	sizeof(subrs7), sizeof(subrs8), sizeof(subrs9) };

	/* This code to be used for Flex and hint replacement. */
	/* Version 1.1 */
static const char *copyright[] = {
	"% Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/).",
	"% All Rights Reserved.",
	"% This software is licensed as OpenSource, under the Apache License, Version 2.0.",
	"% This license is available at: http://opensource.org/licenses/Apache-2.0.",
	NULL
};

static const char *othersubrs0[] = {
	"systemdict /internaldict known",
	"{1183615869 systemdict /internaldict get exec",
	"/FlxProc known {save true} {false} ifelse}",
	"{userdict /internaldict known not {",
	"userdict /internaldict",
	"{count 0 eq",
	"{/internaldict errordict /invalidaccess get exec} if",
	"dup type /integertype ne",
	"{/internaldict errordict /invalidaccess get exec} if",
	"dup 1183615869 eq",
	"{pop 0}",
	"{/internaldict errordict /invalidaccess get exec}",
	"ifelse",
	"}",
	"dup 14 get 1 25 dict put",
	"bind executeonly put",
	"} if",
	"1183615869 userdict /internaldict get exec",
	"/FlxProc known {save true} {false} ifelse}",
	"ifelse",
	"[",
	"systemdict /internaldict known not",
	"{100 dict /begin cvx /mtx matrix /def cvx} if",
	"systemdict /currentpacking known {currentpacking true setpacking} if",
	"{",
	"systemdict /internaldict known {",
	"1183615869 systemdict /internaldict get exec",
	"dup /$FlxDict known not {",
	"dup dup length exch maxlength eq",
	"{pop userdict dup /$FlxDict known not",
	"{100 dict begin /mtx matrix def",
	"dup /$FlxDict currentdict put end} if}",
	"{100 dict begin /mtx matrix def",
	"dup /$FlxDict currentdict put end}",
	"ifelse",
	"} if",
	"/$FlxDict get begin",
	"} if",
	"grestore",
	"/exdef {exch def} def",
	"/dmin exch abs 100 div def",
	"/epX exdef /epY exdef",
	"/c4y2 exdef /c4x2 exdef /c4y1 exdef /c4x1 exdef /c4y0 exdef /c4x0 exdef",
	"/c3y2 exdef /c3x2 exdef /c3y1 exdef /c3x1 exdef /c3y0 exdef /c3x0 exdef",
	"/c1y2 exdef /c1x2 exdef /c2x2 c4x2 def /c2y2 c4y2 def",
	"/yflag c1y2 c3y2 sub abs c1x2 c3x2 sub abs gt def",
	"/PickCoords {",
	"{c1x0 c1y0 c1x1 c1y1 c1x2 c1y2 c2x0 c2y0 c2x1 c2y1 c2x2 c2y2}",
	"{c3x0 c3y0 c3x1 c3y1 c3x2 c3y2 c4x0 c4y0 c4x1 c4y1 c4x2 c4y2}",
	"ifelse",
	"/y5 exdef /x5 exdef /y4 exdef /x4 exdef /y3 exdef /x3 exdef",
	"/y2 exdef /x2 exdef /y1 exdef /x1 exdef /y0 exdef /x0 exdef",
	"} def",
	"mtx currentmatrix pop",
	"mtx 0 get abs .00001 lt mtx 3 get abs .00001 lt or",
	"{/flipXY -1 def}",
	"{mtx 1 get abs .00001 lt mtx 2 get abs .00001 lt or",
	"{/flipXY 1 def}",
	"{/flipXY 0 def}",
	"ifelse}",
	"ifelse",
	"/erosion 1 def",
	"systemdict /internaldict known {",
	"1183615869 systemdict /internaldict get exec dup",
	"/erosion known",
	"{/erosion get /erosion exch def}",
	"{pop}",
	"ifelse",
	"} if",
	"yflag",
	"{flipXY 0 eq c3y2 c4y2 eq or",
	"{false PickCoords}",
	"{/shrink c3y2 c4y2 eq",
	"{0}{c1y2 c4y2 sub c3y2 c4y2 sub div abs} ifelse def",
	"/yshrink {c4y2 sub shrink mul c4y2 add} def",
	"/c1y0 c3y0 yshrink def /c1y1 c3y1 yshrink def",
	"/c2y0 c4y0 yshrink def /c2y1 c4y1 yshrink def",
	"/c1x0 c3x0 def /c1x1 c3x1 def /c2x0 c4x0 def /c2x1 c4x1 def",
	"/dY 0 c3y2 c1y2 sub round",
	"dtransform flipXY 1 eq {exch} if pop abs def",
	"dY dmin lt PickCoords",
	"y2 c1y2 sub abs 0.001 gt {",
	"c1x2 c1y2 transform flipXY 1 eq {exch} if",
	"/cx exch def /cy exch def",
	"/dY 0 y2 c1y2 sub round dtransform flipXY 1 eq {exch}",
	"if pop def",
	"dY round dup 0 ne",
	"{/dY exdef}",
	"{pop dY 0 lt {-1}{1} ifelse /dY exdef}",
	"ifelse",
	"/erode PaintType 2 ne erosion 0.5 ge and def",
	"erode {/cy cy 0.5 sub def} if",
	"/ey cy dY add def",
	"/ey ey ceiling ey sub ey floor add def",
	"erode {/ey ey 0.5 add def} if",
	"ey cx flipXY 1 eq {exch} if itransform exch pop",
	"y2 sub /eShift exch def",
	"/y1 y1 eShift add def /y2 y2 eShift add def /y3 y3",
	"eShift add def",
	"} if",
	"} ifelse",
	"}",
	"{flipXY 0 eq c3x2 c4x2 eq or",
	"{false PickCoords}",
	"{/shrink c3x2 c4x2 eq",
	"{0}{c1x2 c4x2 sub c3x2 c4x2 sub div abs} ifelse def",
	"/xshrink {c4x2 sub shrink mul c4x2 add} def",
	"/c1x0 c3x0 xshrink def /c1x1 c3x1 xshrink def",
	"/c2x0 c4x0 xshrink def /c2x1 c4x1 xshrink def",
	"/c1y0 c3y0 def /c1y1 c3y1 def /c2y0 c4y0 def /c2y1 c4y1 def",
	"/dX c3x2 c1x2 sub round 0 dtransform",
	"flipXY -1 eq {exch} if pop abs def",
	"dX dmin lt PickCoords",
	"x2 c1x2 sub abs 0.001 gt {",
	"c1x2 c1y2 transform flipXY -1 eq {exch} if",
	"/cy exch def /cx exch def",
	"/dX x2 c1x2 sub round 0 dtransform flipXY -1 eq {exch} if pop def",
	"dX round dup 0 ne",
	"{/dX exdef}",
	"{pop dX 0 lt {-1}{1} ifelse /dX exdef}",
	"ifelse",
	"/erode PaintType 2 ne erosion .5 ge and def",
	"erode {/cx cx .5 sub def} if",
	"/ex cx dX add def",
	"/ex ex ceiling ex sub ex floor add def",
	"erode {/ex ex .5 add def} if",
	"ex cy flipXY -1 eq {exch} if itransform pop",
	"x2 sub /eShift exch def",
	"/x1 x1 eShift add def /x2 x2 eShift add def /x3 x3 eShift add def",
	"} if",
	"} ifelse",
	"} ifelse",
	"x2 x5 eq y2 y5 eq or",
	"{x5 y5 lineto}",
	"{x0 y0 x1 y1 x2 y2 curveto",
	"x3 y3 x4 y4 x5 y5 curveto}",
	"ifelse",
	"epY epX",
	"}",
	"systemdict /currentpacking known {exch setpacking} if",
	"/exec cvx /end cvx ] cvx",
	"executeonly",
	"exch",
	"{pop true exch restore}",
	"{",
	"systemdict /internaldict known not",
	"{1183615869 userdict /internaldict get exec",
	"exch /FlxProc exch put true}",
	"{1183615869 systemdict /internaldict get exec",
	"dup length exch maxlength eq",
	"{false}",
	"{1183615869 systemdict /internaldict get exec",
	"exch /FlxProc exch put true}",
	"ifelse}",
	"ifelse}",
	"ifelse",
	"{systemdict /internaldict known",
	"{{1183615869 systemdict /internaldict get exec /FlxProc get exec}}",
	"{{1183615869 userdict /internaldict get exec /FlxProc get exec}}",
	"ifelse executeonly",
	"} if",
	NULL
};

static const char *othersubrs1[] = {
	"{gsave currentpoint newpath moveto} executeonly",
	NULL
};

static const char *othersubrs2[] = {
	"{currentpoint grestore gsave currentpoint newpath moveto} executeonly",
	NULL
};

static const char *othersubrs3[] = {
	"{systemdict /internaldict known not",
	"{pop 3}",
	"{1183615869 systemdict /internaldict get exec",
	"dup /startlock known",
	"{/startlock get exec}",
	"{dup /strtlck known",
	"{/strtlck get exec}",
	"{pop 3}",
	"ifelse}",
	"ifelse}",
	"ifelse",
	"} executeonly",
	NULL
};

static const char *othersubrs4_12[] = {
	"{}",
	NULL
};

static const char *othersubrs13[] = {
	"{2 {cvi {{pop 0 lt {exit} if} loop} repeat} repeat}",	/* Other Subr 13 for counter hints */
	NULL
};

const char **othersubrs_copyright[] = { copyright };
const char **othersubrs[] = { othersubrs0, othersubrs1, othersubrs2, othersubrs3,
	othersubrs4_12, othersubrs4_12, othersubrs4_12, othersubrs4_12, 
	othersubrs4_12, othersubrs4_12, othersubrs4_12, othersubrs4_12,
	othersubrs4_12, othersubrs13,
/* code for other subrs 14-18 must be done at run time as it depends on */
/*  the number of font instances in the mm set */
	NULL
};
static const char **default_othersubrs[] = { othersubrs0, othersubrs1, othersubrs2, othersubrs3,
	othersubrs4_12, othersubrs4_12, othersubrs4_12, othersubrs4_12, 
	othersubrs4_12, othersubrs4_12, othersubrs4_12, othersubrs4_12,
	othersubrs4_12, othersubrs13,
	NULL
};

/* from Adobe Technical Specification #5014, Adobe CMap and CIDFont Files */
/* Specification, Version 1.0. */
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */
const char *cid_othersubrs[] = {
	"[ {} {} {}",
	"  { systemdict /internaldict known not",
	"    { pop 3 }",
	"    { 1183615869 systemdict /internaldict get exec dup",
	"      /startlock known",
	"      { /startlock get exec }",
	"      { dup /strlck known",
	"        { /strlck get exec }",
	"        { pop 3 }",
	"        ifelse",
	"      }",
	"      ifelse",
	"    }",
	"    ifelse",
	"  } bind",
	"  {} {} {} {} {} {} {} {} {}",
	"  { 2 { cvi { { pop 0 lt { exit } if } loop } repeat }",
	"       repeat } bind",
	"]",
	NULL
};

static const char **CopyLines(char **lines, int l,int is_copyright) {
    const char **ret;
    int i;

    if ( l==0 && !is_copyright ) {
	ret = malloc(2*sizeof(char *));
	ret[0] = copy("{}");
	ret[1] = NULL;
return( ret );
    }

    ret = malloc((l+1)*sizeof(char *));
    for ( i=0; i<l; ++i )
	ret[i] = lines[i];
    ret[l] = NULL;
return( ret );
}

void DefaultOtherSubrs(void) {
    int i,j;

    if ( othersubrs_copyright[0]!=copyright ) {
	for ( i=0; othersubrs_copyright[0][i]!=NULL; ++i )
	    free( (char *) othersubrs_copyright[0][i]);
	free(othersubrs_copyright[0]);
	othersubrs_copyright[0] = copyright;
    }
    for ( j=0; j<=13; ++j ) {
	if ( othersubrs[j]!=default_othersubrs[j] ) {
	    for ( i=0; othersubrs[j][i]!=NULL; ++i )
		free( (char *) othersubrs[j][i]);
	    free(othersubrs[j]);
	   othersubrs[j] = default_othersubrs[j];
	}
    }
}

int ReadOtherSubrsFile(char *filename) {
    FILE *os = fopen(filename,"r");
    char buffer[500];
    char **lines=NULL;
    int l=0, lmax=0;
    int sub_num = -1;
    const char **co=NULL, **osubs[14];
    int i;

    if ( os==NULL )
return( false );
    while ( fgets(buffer,sizeof(buffer),os)!=NULL ) {
	int len = strlen(buffer);
	if ( len>0 && (buffer[len-1]=='\r' || buffer[len-1]=='\n')) {
	    if ( len>1 && (buffer[len-2]=='\r' || buffer[len-2]=='\n'))
		buffer[len-2] = '\0';
	    else
		buffer[len-1] = '\0';
	}
	if ( buffer[0]=='%' && buffer[1]=='%' && buffer[2]=='%' && buffer[3]=='%' ) {
	    if ( sub_num == -1 )
		co = CopyLines(lines,l,true);
	    else if ( sub_num<14 )
		osubs[sub_num] = CopyLines(lines,l,false);
	    else if ( sub_num==14 )
		LogError( _("Too many subroutines. We can deal with at most 14 (0-13)") );
	    ++sub_num;
	    l = 0;
	} else {
	    if ( l>=lmax ) {
		lmax += 100;
		lines = realloc(lines,lmax*sizeof(char *));
	    }
	    lines[l++] = copy(buffer);
	}
    }
    fclose( os );
    /* we just read a copyright notice? That's no use */
    if ( sub_num<=0 ) {
        if (co) {
            for ( i=0; co[i]!=NULL; i++)
                free((char*) co[i]);
	    free(co);
        }
        if (lines) {
            for ( i=0; i<l; i++)
                free(lines[i]);
	    free(lines);
        }
return( false );
    }
    while ( sub_num<14 ) {
	osubs[sub_num] = calloc(2,sizeof(char *));
	osubs[sub_num][0] = copy("{}");
	++sub_num;
    }
    DefaultOtherSubrs();
    othersubrs_copyright[0] = co;
    for ( i=0; i<14; ++i )
	othersubrs[i] = osubs[i];
    if (lines) {
        for ( i=0; i<l; i++)
            free(lines[i]);
        free(lines);
    }
return( true );
}
	
