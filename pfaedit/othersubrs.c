#include "basics.h"

/* These subroutines are code by Adobe for this exact use (from T1_Spec.pdf) */

	/* 3 0 callothersubr pop pop setcurrentpoint return */
static const uint8 subrs0[] = { 3+139, 0+139, 12, 16, 12, 17, 12, 17, 12, 33, 11 };
	/* 0 1 callothersubr return */
static const uint8 subrs1[] = { 0+139, 1+139, 12, 16, 11 };
	/* 0 2 callothersubr return */
static const uint8 subrs2[] = { 0+139, 2+139, 12, 16, 11 };
	/* return */
static const uint8 subrs3[] = { 11 };


const uint8 *const subrs[4] = { subrs0, subrs1, subrs2, subrs3 };
const int subrslens[4] = { sizeof(subrs0), sizeof(subrs1), sizeof(subrs2), sizeof(subrs3)};

/* Assumption: We always want to do hint substitution (anyway, it's small) */
/* If we also want flex output use the "othersubrs" array. If no flex the "othersubrsnoflex" */
/* If we want to do counters, output "othersubrscounters" too */
/* Finish up with othersubrsend */
const char *othersubrs[] = {
	"% Copyright (c) 1987-1990 Adobe Systems Incorporated.",
	"% All Rights Reserved.",
	"% This code to be used for Flex and hint replacement.",
	"% Version 1.1",
	"[systemdict /internaldict known",
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
	"{gsave currentpoint newpath moveto} executeonly",
	"{currentpoint grestore gsave currentpoint newpath moveto} executeonly",
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

const char *othersubrsnoflex[] = {
	"% Copyright (c) 1987-1990 Adobe Systems Incorporated.",
	"% All Rights Reserved.",
	"% This code to be used for Hint replacement only.",
	"% Version 1.1",
	"[ {} {} {}",
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

const char *othersubrscounters[] = {
	"{}",		/* Other subr 4 */
	"{}",		/* Other subr 5 */
	"{}",		/* Other subr 6 */
	"{}",		/* Other subr 7 */
	"{}",		/* Other subr 8 */
	"{}",		/* Other subr 9 */
	"{}",		/* Other subr 10 */
	"{}",		/* Other subr 11 */
	"{}",		/* Other Subr 12, for counter hints */
	"{2 {cvi {{pop 0 lt {exit} if} loop} repeat} repeat}",	/* Other Subr 13 for counter hints */
	NULL
};

const char *othersubrsend[] = {
	"] noaccess ",
	NULL
};

/* Lives in private dictionary. Commonly used. I have no docs on it */
/* The first numbers (9.5,72) change in different uses (4.5,34), (4.5,38), (5.5,41), (6.5,50) are other combo */
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
