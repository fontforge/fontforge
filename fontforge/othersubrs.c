#include "othersubrs.h"

#include "fontforge.h"		/* For LogError */

/* These subroutines are code by Adobe for this exact use (from T1_Spec.pdf) */

	/* 3 0 callothersubr pop pop setcurrentpoint return */
static const uint8 subrs0[] = { 3+139, 0+139, 12, 16, 12, 17, 12, 17, 12, 33, 11 };
	/* 0 1 callothersubr return */
static const uint8 subrs1[] = { 0+139, 1+139, 12, 16, 11 };
	/* 0 2 callothersubr return */
static const uint8 subrs2[] = { 0+139, 2+139, 12, 16, 11 };
	/* return */
static const uint8 subrs3[] = { 11 };
	/* This one I created myself to do hint substitution */
	/* <subr number presumed to be on stack> 1 3 callother pop callsubr */
static const uint8 subrs4[] = { 1+139, 3+139, 12, 16, 12, 17, 10, 11 };

	/* These others from adobe for multiple master */
	/* They need some fix up before they are used (the stack count depends on the # instances). */
	/* <n> 14 callothersubr pop return */
static const uint8 subrs5[] = { 139, 14+139, 12, 16, 12, 17, 11 };
	/* 2*<n> 15 callothersubr pop pop return */
static const uint8 subrs6[] = { 139, 15+139, 12, 16, 12, 17, 12, 17, 11 };
	/* 3*<n> 16 callothersubr pop pop pop return */
static const uint8 subrs7[] = { 139, 16+139, 12, 16, 12, 17, 12, 17, 12, 17, 11 };
	/* 4*<n> 17 callothersubr pop pop pop pop return */
static const uint8 subrs8[] = { 139, 17+139, 12, 16, 12, 17, 12, 17, 12, 17, 12, 17, 11 };
	/* 6*<n> 18 callothersubr pop pop pop pop  pop pop return */
static const uint8 subrs9[] = { 139, 18+139, 12, 16, 12, 17, 12, 17, 12, 17, 12, 17, 12, 17, 12, 17, 11 };


const uint8 *const subrs[] = { subrs0, subrs1, subrs2, subrs3, subrs4,
	subrs5, subrs6, subrs7, subrs8, subrs9 };
const int subrslens[] = { sizeof(subrs0), sizeof(subrs1), sizeof(subrs2),
	sizeof(subrs3), sizeof(subrs4), sizeof(subrs5), sizeof(subrs6),
	sizeof(subrs7), sizeof(subrs8), sizeof(subrs9) };

static const char *copyright[] = {
	"% Copyright (c) 1987-1990 Adobe Systems Incorporated.",
	"% All Rights Reserved.",
	"% This code to be used for Flex and hint replacement.",
	"% Version 1.1",
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

/* Lives in private dictionary. Commonly used. I have no docs on it */
/* (and Adobe's answer when asked about it was "try it and figure out what it does" */
/*  The first numbers (9.5,72) change in different uses (4.5,34), (4.5,38), (5.5,41), (6.5,50) are other combo */
/* James Cloos notes that the second number is /StdVW, and the first number is */
/*  StdVW 8 idiv .5 add */
/* 
const char *erode[] = {
	"/Erode",
	"{ 9.5 dup 3 -1 roll 0.1 mul exch 0.5 sub mul cvi sub dup mul",
	"  72 0 dtransform dup mul exch dup mul add le",
	"    { pop pop 1.0 1.0 }",
	"    { pop pop 0.0 1.5 }",
	"  ifelse",
	"} bind def",
	NULL
};
*/

/* from Adobe Technical Specification #5014, Adobe CMap and CIDFont Files */
/* Specification, Version 1.0. */
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


const char *makeblendedfont[] = {
	"% Copyright (c) 1990-1994 Adobe Systems Incorporated.",
	"% All Rights Reserved.",
/* Adobe has posted a copyright notice in 5015.Type1_Supp.pdf with the */
/*  wrong comment here. I've changed it */
	"% This code to be used for multiple master fonts.",
	"% Version 11",
	"/shareddict where",
	"{ pop currentshared { setshared } true setshared shareddict }",
	"{ {} userdict } ifelse dup",
	"/makeblendedfont where {/makeblendedfont get dup type /operatortype eq {",
	"pop false} { 0 get dup type /integertype ne",
	"{pop false} {11 lt} ifelse} ifelse } {true}ifelse",
	"{/makeblendedfont {",
	"11 pop",
	"2 copy length exch /WeightVector get length eq",
	"{ dup 0 exch {add} forall 1 sub abs .001 gt }",
	"{ true } ifelse",
	"{ /makeblendedfont cvx errordict /rangecheck get exec } if",
	"exch dup dup maxlength dict begin {",
	"false {/FID /UniqueID /XUID } { 3 index eq or } forall",
	" { pop pop } { def } ifelse",
	"} forall",
	"/XUID 2 copy known{",
	"get dup length 2 index length sub dup 0 gt{",
	"exch dup length array copy",
	"exch 2 index{65536 mul cvi 3 copy put pop 1 add}forall pop/XUID exch def",
	"}{pop pop}ifelse",
	"}{pop pop}ifelse",
	"{ /Private /FontInfo } {",
	"dup load dup maxlength dict begin {",
	"false { /UniqueID /XUID } { 3 index eq or } forall",
	"{ pop pop }{ def } ifelse } forall currentdict end def",
	"} forall",
	"dup /WeightVector exch def",
	"dup /$Blend exch [",
	"exch false exch",
	"dup length 1 sub -1 1 {",
	"1 index dup length 3 -1 roll sub get",
	"dup 0 eq {",
	"pop 1 index {/exch load 3 1 roll} if",
	"/pop load 3 1 roll",
	"} {dup 1 eq {pop}",
	"{2 index {/exch load 4 1 roll} if",
	"3 1 roll /mul load 3 1 roll } ifelse",
	"1 index {/add load 3 1 roll} if",
	"exch pop true exch} ifelse",
	"} for",
	"pop { /add load } if",
	"] cvx def",
	"{2 copy length exch length ne {/makeblendedfont cvx errordict /typecheck get exec}if",
	"0 0 1 3 index length 1 sub {",
	"dup 4 index exch get exch 3 index exch get mul add",
	"} for",
	"exch pop exch pop}",
	"{{dup type dup dup /arraytype eq exch /packedarraytype eq or {",
	"  pop 1 index /ForceBold eq {",
	"  5 index 0 0 1 3 index length 1 sub {",
	"  dup 4 index exch get {2 index exch get add } {pop} ifelse",
	"  } for exch pop exch pop",
	"  2 index /ForceBoldThreshold get gt 3 copy} {",
	"{length 1 index length ne { pop false } {",
	"true exch { type dup /integertype eq exch /realtype eq exch or and } forall",
	"} ifelse }",
	"2 copy 8 index exch exec {pop 5 index 5 index exec}",
	"{exch dup length array 1 index xcheck { cvx } if",
	"dup length 1 sub 0 exch 1 exch {",
	"dup 3 index exch get dup type dup /arraytype eq exch /packedarraytype eq or {",
	"dup 10 index 6 index exec {",
	"9 index exch 9 index exec} if } if 2 index 3 1 roll put",
	"} for exch pop exch pop",
	"} ifelse 3 copy",
	"1 index dup /StemSnapH eq exch /StemSnapV eq or {",
	"dup length 1 sub {dup 0 le { exit } if",
	"dup dup 1 sub 3 index exch get exch 3 index exch get 2 copy eq {",
	"pop 2 index 2 index 0 put 0 } if le {1 sub}",
	"{dup dup 1 sub 3 index exch get exch 3 index exch get",
	"3 index exch 3 index 1 sub exch put",
	"3 copy put pop",
	"2 copy exch length 1 sub lt {1 add} if} ifelse} loop pop",
	"dup 0 get 0 le {",
	"dup 0 exch {0 gt { exit } if 1 add} forall",
	"dup 2 index length exch sub getinterval} if } if } ifelse put }",
	"{/dicttype eq {6 copy 3 1 roll get exch 2 index exec}",
	"{/makeblendedfont cvx errordict /typecheck get exec} ifelse",
	"} ifelse pop pop } forall pop pop pop pop }",
	"currentdict Blend 2 index exec",
	"currentdict end",
	"} bind put",
	"/$fbf {FontDirectory counttomark 3 add -1 roll known {",
	"cleartomark pop findfont}{",
	"] exch findfont exch makeblendedfont",
	"dup /Encoding currentfont /Encoding get put definefont",
	"} ifelse currentfont /ScaleMatrix get makefont setfont",
	"} bind put } { pop pop } ifelse exec",
	NULL
};

const char *mmfindfont[] = {
	"/$mmff_origfindfont where {",
	"  pop save { restore } { pop pop }",
	"} { {} { def } } ifelse",
	"/setshared where { pop true } { false } ifelse",
	"/findfont where pop dup systemdict eq {",
	"pop { currentshared {{}} { true setshared { false setshared } } ifelse shareddict",
	"} {{} userdict } ifelse begin",
	"} { begin { currentdict scheck } { false } ifelse {",
	"currentshared {{}} { true setshared { false setshared } } ifelse",
	"} { {} } ifelse } ifelse",
	"/$mmff_origfindfont /findfont load 3 index exec",
	"/findfont {",
	"dup FontDirectory exch known",
	"{ dup FontDirectory exch get /FontType get 3 ne}",
	"{ dup SharedFontDirectory exch known",
	"{ dup SharedFontDirectory exch get /FontType get 3 ne}",
	"{ false} ifelse} ifelse",
	"{$mmff_origfindfont} { dup dup length string cvs (_) search {",
	"cvn dup dup FontDirectory exch known exch SharedFontDirectory exch known or {",
	"true} {dup length 7 add string dup 0 (%font%) putinterval",
	"dup 2 index 6 exch dup length string cvs putinterval",
	"{ status } stopped { pop false } if {",
	"pop pop pop pop true} {false} ifelse} ifelse {",
	"$mmff_origfindfont begin pop",
	"[ exch { (_) search  { { cvr } stopped { pop pop } {",
	"exch pop exch } ifelse",
	"} { pop exit } ifelse } loop false /FontInfo where {",
	"pop FontInfo /BlendAxisTypes 2 copy known {",
	"get length counttomark 2 sub eq exch pop",
	"} { pop pop } ifelse } if {",
	"NormalizeDesignVector",
	"ConvertDesignVector",
	"] currentdict exch makeblendedfont",
	"2 copy exch /FontName exch put",
	"definefont} { cleartomark $mmff_origfindfont } ifelse end",
	"} { pop pop pop $mmff_origfindfont } ifelse",
	"} { pop $mmff_origfindfont } ifelse } ifelse",
	"} bind 3 index exec",
	"/SharedFontDirectory dup where { pop pop } { 0 dict 3 index exec } ifelse",
	"end exec pop exec",
NULL
};

#include "splinefont.h"
#include <string.h>
#include <ustring.h>

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
		LogError( _("Too many subroutines. We can deal with at most 14 (0-13)\n") );
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
	
