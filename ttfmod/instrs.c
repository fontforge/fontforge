/* Copyright (C) 2001 by George Williams */
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
#include "ttfmodui.h"
#include <gkeysym.h>
#include <ustring.h>
#include <utype.h>

extern int _GScrollBar_Width;

enum byte_types { bt_instr, bt_cnt, bt_byte, bt_wordhi, bt_wordlo };
static const char *instrs[] = {
    "SVTCA[y-axis]",
    "SVTCA[x-axis]",
    "SPVTCA[y-axis]",
    "SPVTCA[x-axis]",
    "SFVTCA[y-axis]",
    "SFVTCA[x-axis]",
    "SPVTL[parallel]",
    "SPVTL[orthog]",
    "SFVTL[parallel]",
    "SFVTL[orthog]",
    "SPVFS",
    "SFVFS",
    "GPV",
    "GFV",
    "SFVTPV",
    "ISECT",
    "SRP0",
    "SRP1",
    "SRP2",
    "SZP0",
    "SZP1",
    "SZP2",
    "SZPS",
    "SLOOP",
    "RTG",
    "RTHG",
    "SMD",
    "ELSE",
    "JMPR",
    "SCVTCI",
    "SSWCI",
    "SSW",
    "DUP",
    "POP",
    "CLEAR",
    "SWAP",
    "DEPTH",
    "CINDEX",
    "MINDEX",
    "ALIGNPTS",
    "Unknown28",
    "UTP",
    "LOOPCALL",
    "CALL",
    "FDEF",
    "ENDF",
    "MDAP[no round]",
    "MDAP[round]",
    "IUP[y]",
    "IUP[x]",
    "SHP[rp2]",
    "SHP[rp1]",
    "SHC[rp2]",
    "SHC[rp1]",
    "SHZ[rp2]",
    "SHZ[rp1]",
    "SHPIX",
    "IP",
    "MSIRP[no set rp0]",
    "MSIRP[set rp0]",
    "ALIGNRP",
    "RTDG",
    "MIAP[no round]",
    "MIAP[round]",
    "NPUSHB",
    "NPUSHW",
    "WS",
    "RS",
    "WCVTP",
    "RCVT",
    "GC[cur]",
    "GC[orig]",
    "SCFS",
    "MD[grid]",
    "MD[orig]",
    "MPPEM",
    "MPS",
    "FLIPON",
    "FLIPOFF",
    "DEBUG",
    "LT",
    "LTEQ",
    "GT",
    "GTEQ",
    "EQ",
    "NEQ",
    "ODD",
    "EVEN",
    "IF",
    "EIF",
    "AND",
    "OR",
    "NOT",
    "DELTAP1",
    "SDB",
    "SDS",
    "ADD",
    "SUB",
    "DIV",
    "MUL",
    "ABS",
    "NEG",
    "FLOOR",
    "CEILING",
    "ROUND[Grey]",
    "ROUND[Black]",
    "ROUND[White]",
    "ROUND[Undef4]",
    "NROUND[Grey]",
    "NROUND[Black]",
    "NROUND[White]",
    "NROUND[Undef4]",
    "WCVTF",
    "DELTAP2",
    "DELTAP3",
    "DELTAC1",
    "DELTAC2",
    "DELTAC3",
    "SROUND",
    "S45ROUND",
    "JROT",
    "JROF",
    "ROFF",
    "Unknown7B",
    "RUTG",
    "RDTG",
    "SANGW",
    "AA",
    "FLIPPT",
    "FLIPRGON",
    "FLIPRGOFF",
    "Unknown83",
    "Unknown84",
    "SCANCTRL",
    "SDPVTL0",
    "SDPVTL1",
    "GETINFO",
    "IDEF",
    "ROLL",
    "MAX",
    "MIN",
    "SCANTYPE",
    "INSTCTRL",
    "Unknown8F",
    "Unknown90",
    "Unknown91",
    "Unknown92",
    "Unknown93",
    "Unknown94",
    "Unknown95",
    "Unknown96",
    "Unknown97",
    "Unknown98",
    "Unknown99",
    "Unknown9A",
    "Unknown9B",
    "Unknown9C",
    "Unknown9D",
    "Unknown9E",
    "Unknown9F",
    "UnknownA0",
    "UnknownA1",
    "UnknownA2",
    "UnknownA3",
    "UnknownA4",
    "UnknownA5",
    "UnknownA6",
    "UnknownA7",
    "UnknownA8",
    "UnknownA9",
    "UnknownAA",
    "UnknownAB",
    "UnknownAC",
    "UnknownAD",
    "UnknownAE",
    "UnknownAF",
    "PUSHB [1]",
    "PUSHB [2]",
    "PUSHB [3]",
    "PUSHB [4]",
    "PUSHB [5]",
    "PUSHB [6]",
    "PUSHB [7]",
    "PUSHB [8]",
    "PUSHW [1]",
    "PUSHW [2]",
    "PUSHW [3]",
    "PUSHW [4]",
    "PUSHW [5]",
    "PUSHW [6]",
    "PUSHW [7]",
    "PUSHW [8]",
    "MDRP[grey]",
    "MDRP[black]",
    "MDRP[white]",
    "MDRP03",
    "MDRP[round, grey]",
    "MDRP[round, black]",
    "MDRP[round, white]",
    "MDRP07",
    "MDRP[minimum, grey]",
    "MDRP[minimum, black]",
    "MDRP[minimum, white]",
    "MDRP0b",
    "MDRP[minimum, round, grey]",
    "MDRP[minimum, round, black]",
    "MDRP[minimum, round, white]",
    "MDRP0f",
    "MDRP[rp0, grey]",
    "MDRP[rp0, black]",
    "MDRP[rp0, white]",
    "MDRP13",
    "MDRP[rp0, round, grey]",
    "MDRP[rp0, round, black]",
    "MDRP[rp0, round, white]",
    "MDRP17",
    "MDRP[rp0, minimum, grey]",
    "MDRP[rp0, minimum, black]",
    "MDRP[rp0, minimum, white]",
    "MDRP1b",
    "MDRP[rp0, minimum, round, grey]",
    "MDRP[rp0, minimum, round, black]",
    "MDRP[rp0, minimum, round, white]",
    "MDRP1f",
    "MIRP[grey]",
    "MIRP[black]",
    "MIRP[white]",
    "MIRP03",
    "MIRP[round, grey]",
    "MIRP[round, black]",
    "MIRP[round, white]",
    "MIRP07",
    "MIRP[minimum, grey]",
    "MIRP[minimum, black]",
    "MIRP[minimum, white]",
    "MIRP0b",
    "MIRP[minimum, round, grey]",
    "MIRP[minimum, round, black]",
    "MIRP[minimum, round, white]",
    "MIRP0f",
    "MIRP[rp0, grey]",
    "MIRP[rp0, black]",
    "MIRP[rp0, white]",
    "MIRP13",
    "MIRP[rp0, round, grey]",
    "MIRP[rp0, round, black]",
    "MIRP[rp0, round, white]",
    "MIRP17",
    "MIRP[rp0, minimum, grey]",
    "MIRP[rp0, minimum, black]",
    "MIRP[rp0, minimum, white]",
    "MIRP1b",
    "MIRP[rp0, minimum, round, grey]",
    "MIRP[rp0, minimum, round, black]",
    "MIRP[rp0, minimum, round, white]",
    "MIRP1f"
};

static unichar_t *instrhelppopup[256];
static void ihaddr(int bottom,int top,char *msg) {
    unichar_t *um = uc_copy(msg);
    while ( bottom<=top )
	instrhelppopup[bottom++] = um;
}

static void ihadd(int p,char *msg) {
    ihaddr(p,p,msg);
}

static void instrhelpsetup(void) {
    if ( instrhelppopup[0]!=NULL )
return;
    ihadd(0x7f,"Adjust Angle\nObsolete instruction\nPops one value");
    ihadd(0x64,"ABSolute Value\nReplaces top of stack with its abs");
    ihadd(0x60,"ADD\nPops two 26.6 fixed numbers from stack\nadds them, pushes result");
    ihadd(0x27,"ALIGN PoinTS\nAligns (&pops) the two points which are on the stack\nby moving along freedom vector to the average of their\npositions on projection vector");
    ihadd(0x3c,"ALIGN to Reference Point\nPops as many points as specified in loop counter\nAligns points with RP0 by moving each\nalong freedom vector until distance to\nRP0 on projection vector is 0");
    ihadd(0x5a,"logical AND\nPops two values, ands them, pushes result");
    ihadd(0x2b,"CALL function\nPops a value, calls the function represented by it");
    ihadd(0x67,"CEILING\nPops one 26.6 value, rounds upward to an int\npushes result");
    ihadd(0x25,"Copy INDEXed element to stack\nPops an index & copies stack\nelement[index] to top of stack");
    ihadd(0x22,"CLEAR\nPops all elements on stack");
    ihadd(0x4f,"DEBUG call\nPops a value and executes a debugging interpreter\n(if available)");
    ihadd(0x73,"DELTA exception C1\nPops a value n & then n exception specifications & cvt entries\nchanges each cvt entry at a given size by the pixel amount");
    ihadd(0x74,"DELTA exception C2\nPops a value n & then n exception specifications & cvt entries\nchanges each cvt entry at a given size by the amount");
    ihadd(0x75,"DELTA exception C3\nPops a value n & then n exception specifications & cvt entries\nchanges each cvt entry at a given size by the amount");
    ihadd(0x5D,"DELTA exception P1\nPops a value n & then n exception specifications & points\nmoves each point at a given size by the amount");
    ihadd(0x71,"DELTA exception P2\nPops a value n & then n exception specifications & points\nmoves each point at a given size by the amount");
    ihadd(0x72,"DELTA exception P3\nPops a value n & then n exception specifications & points\nmoves each point at a given size by the amount");
    ihadd(0x24,"DEPTH of stack\nPushes the number of elements on the stack");
    ihadd(0x62,"DIVide\nPops two 26.6 numbers, divides them, pushes result");
    ihadd(0x20,"DUPlicate top stack element\nPushes the top stack element again");
    ihadd(0x59,"End IF\nEnds and IF or IF-ELSE sequence");
    ihadd(0x1b,"ELSE clause\nStart of Else clause of preceding IF");
    ihadd(0x2d,"END Function definition");
    ihadd(0x54,"EQual\nPops two values, tests for equality, pushes result(0/1)");
    ihadd(0x57,"EVEN\nPops one value, rounds it and tests if it is even(0/1)");
    ihadd(0x2C,"Function DEFinition\nPops a value (n) and starts the nth\nfunction definition");
    ihadd(0x4e,"set the auto FLIP boolean to OFF");
    ihadd(0x4d,"set the auto FLIP boolean to ON");
    ihadd(0x80,"FLIP PoinT\nPops as many points as specified in loop counter\nFlips whether each point is on/off curve");
    ihadd(0x82,"FLIP RanGe OFF\nPops two point numbers\nsets all points between to be off curve points");
    ihadd(0x81,"FLIP RanGe ON\nPops two point numbers\nsets all points between to be on curve points");
    ihadd(0x66,"FLOOR\nPops a value, rounds to lowest int, pushes result");
    ihaddr(0x46,0x47,"Get Coordinate[a] projected onto projection vector\n 0=>use current pos\n 1=>use original pos\nPops one point, pushes the coordinate of\nthe point along projection vector");
    ihadd(0x88,"GET INFOrmation\nPops information type, pushes result");
    ihadd(0x0d,"Get Freedom Vector\nDecomposes freedom vector, pushes its\ntwo coordinates onto stack as 2.14");
    ihadd(0x0c,"Get Projection Vector\nDecomposes projection vector, pushes its\ntwo coordinates onto stack as 2.14");
    ihadd(0x52,"Greater Than\nPops two values, pushes (0/1) if bottom el > top");
    ihadd(0x53,"Greater Than or EQual\nPops two values, pushes (0/1) if bottom el >= top");
    ihadd(0x89,"Instruction DEFinition\nPops a value which becomes the opcode\nand begins definition of new instruction");
    ihadd(0x58,"IF test\nPops an integer,\nif 0 (false) next instruction is ELSE or EIF\nif non-0 execution continues normally\n(unless there's an ELSE)");
    ihadd(0x8e,"INSTRuction execution ConTRoL\nPops a selector and value\nSets a state variable");
    ihadd(0x39,"Interpolate Point\nPops as many points as specified in loop counter\nInterpolates each point to preserve original status\nwith respect to RP1 and RP2");
    ihadd(0x0f,"moves point to InterSECTion of two lines\nPops start,end start,end points of two lines\nand a point to move. Point is moved to\nintersection");
    ihaddr(0x30,0x31,"Interpolate Untouched Points[a]\n 0=> interpolate in y direction\n 1=> x direction");
    ihadd(0x1c,"JuMP Relative\nPops offset (in bytes) to move the instruction pointer");
    ihadd(0x79,"Jump Relative On False\nPops a boolean and an offset\nChanges instruction pointer by offset bytes\nif boolean is false");
    ihadd(0x78,"Jump Relative On True\nPops a boolean and an offset\nChanges instruction pointer by offset bytes\nif boolean is true");
    ihadd(0x2a,"LOOP and CALL function\nPops a function number & count\nCalls function count times");
    ihadd(0x50,"Less Than\nPops two values, pushes (0/1) if bottom el < top");
    ihadd(0x51,"Less Than or EQual\nPops two values, pushes (0/1) if bottom el <= top");
    ihadd(0x8b,"MAXimum of top two stack entries\nPops two values, pushes the maximum back");
    ihaddr(0x49,0x4a,"Measure Distance[a]\n 0=>distance with current positions\n 1=>distance with original positions\nPops two point numbers, pushes distance between them");
    ihaddr(0x2e,0x2f,"Move Direct Absolute Point[a]\n 0=>do not round\n 1=>round\nPops a point number, touches that point\nand perhaps rounds it to the grid along\nthe projection vector. Sets rp0&rp1 to the point");
    ihaddr(0xc0,0xdf,"Move Direct Relative Point[abcde]\n a=0=>don't set rp0\n a=1=>set rp0 to p\n b=0=>do not keep distance more than minimum\n b=1=>keep distance at least minimum\n c=0 do not round\n c=1 round\n de=0 => grey distance\n de=1 => black distance\n de=2 => white distance\nPops a point moves it so that it maintains\nits original distance to the rp0. Sets\nrp1 to rp0, rp2 to point, sometimes rp0 to point");
    ihaddr(0x3e,0x3f,"Move Indirect Absolute Point[a]\n 0=>do not round, don't use cvt cutin\n 1=>round\nPops a point number & a cvt entry,\ntouches the point and moves it to the coord\nspecified in the cvt (along the projection vector).\nSets rp0&rp1 to the point");
    ihadd(0x8c,"Minimum of top two stack entries\nPops two values, pushes the minimum back");
    ihadd(0x26,"Move INDEXed element to stack\nPops an index & moves stack\nelement[index] to top of stack\n(removing it from where it was)");
    ihaddr(0xe0,0xff,"Move Indirect Relative Point[abcde]\n a=0=>don't set rp0\n a=1=>set rp0 to p\n b=0=>do not keep distance more than minimum\n b=1=>keep distance at least minimum\n c=0 do not round nor use cvt cutin\n c=1 round & use cvt cutin\n de=0 => grey distance\n de=1 => black distance\n de=2 => white distance\nPops a cvt index and a point moves it so that it\nis cvt[index] from rp0. Sets\nrp1 to rp0, rp2 to point, sometimes rp0 to point");
    ihadd(0x4b,"Measure Pixels Per EM\nPushs the pixels per em (for current rasterization)");
    ihadd(0x4c,"Measure Point Size\nPushes the current point size");
    ihaddr(0x3a,0x3b,"Move Stack Indirect Relative Point[a]\n 0=>do not set rp0\n 1=>set rp0 to point\nPops a 26.6 distance and a point\nMoves point so it is distance from rp0");
    ihadd(0x63,"MULtiply\nPops two 26.6 numbers, multiplies them, pushes result");
    ihadd(0x65,"NEGate\nNegates the top of the stack");
    ihadd(0x55,"Not EQual\nPops two values, tests for inequality, pushes result(0/1)");
    ihadd(0x5c,"logical NOT\nPops a number, if 0 pushes 1, else pushes 0");
    ihadd(0x40,"N PUSH Bytes\nReads an (unsigned) count byte from the\ninstruction stream, then reads and pushes\nthat many unsigned bytes");
    ihadd(0x41,"N PUSH Words\nReads an (unsigned) count byte from the\ninstruction stream, then reads and pushes\nthat many signed 2byte words");
    ihaddr(0x6c,0x6f,"No ROUNDing of value[ab]\n ab=0 => grey distance\n ab=1 => black distance\n ab=2 => white distance\nPops a coordinate (26.6), changes it (without\nrounding) to compensate for engine effects\npushes it back" );
    ihadd(0x56,"ODD\nPops one value, rounds it and tests if it is odd(0/1)");
    ihadd(0x5b,"logical OR\nPops two values, ors them, pushes result");
    ihadd(0x21,"POP top stack element");
    ihaddr(0xb0,0xb7,"PUSH Byte[abc]\n abc is the number-1 of bytes to push\nReads abc+1 unsigned bytes from\nthe instruction stream and pushes them");
    ihaddr(0xb8,0xbf,"PUSH Word[abc]\n abc is the number-1 of words to push\nReads abc+1 signed words from\nthe instruction stream and pushes them");
    ihadd(0x45,"Read Control Value Table entry\nPops an index to the CVT and\npushes it in 26.6 format");
    ihadd(0x7d,"Round Down To Grid\n\nSets round state to the obvious");
    ihadd(0x7a,"Round OFF\nSets round state so that no rounding occurs\nbut engine compensation does");
    ihadd(0x8a,"ROLL the top three stack elements");
    ihaddr(0x68,0x6b,"ROUND value[ab]\n ab=0 => grey distance\n ab=1 => black distance\n ab=2 => white distance\nRounds a coordinate (26.6) at top of stack\nand compensates for engine effects" );
    ihadd(0x43,"Read Store\nPops an index into store array\nPushes value at that index");
    ihadd(0x3d,"Round To Double Grid\nSets the round state (round to closest .5/int)");
    ihadd(0x18,"Round To Grid\nSets the round state");
    ihadd(0x19,"Round To Half Grid\nSets the round state (round to closest .5 not int)");
    ihadd(0x7c,"Round Up To Grid\nSets the round state");
    ihadd(0x77,"Super 45\260 ROUND\nToo complicated. Look it up");
    ihadd(0x7e,"Set ANGle Weight\nPops an int, and sets the angle\nweight state variable to it");
    ihadd(0x85,"SCAN conversion ConTRoL\nPops a number which sets the\ndropout control mode");
    ihadd(0x8d,"SCANTYPE\nPops number which sets which scan\nconversion rules to use");
    ihadd(0x48,"Sets Coordinate From Stack using projection & freedom vectors\nPops a coordinate 26.6 and a point\nMoves point to given coordinate");
    ihadd(0x1d,"Sets Control Value Table Cut-In\nPops 26.6 from stack, sets cvt cutin");
    ihadd(0x5e,"Set Delta Base\nPops value sets delta base");
    ihaddr(0x86,0x87,"Set Dual Projection Vector To Line[a]\n 0 => parallel to line\n 1=>orthogonal to line\nPops two points used to establish the line\nSets a second projection vector based on original\npositions of points");
    ihadd(0x5F,"Set Delta Shift\nPops a new value for delta shift");
    ihadd(0x0b,"Set Freedom Vector From Stack\npops 2 2.14 values (x,y) from stack\nmust be a unit vector");
    ihaddr(0x04,0x05,"Set Freedom Vector To Coordinate Axis[a]\n 0=>y axis\n 1=>x axis\n" );
    ihaddr(0x08,0x09,"Set Fredom Vector To Line[a]\n 0 => parallel to line\n 1=>orthogonal to line\nPops two points used to establish the line\nSets the freedom vector" );
    ihadd(0x0e,"Set Freedom Vector To Projection Vector");
    ihaddr(0x34,0x35,"SHift Contour using reference point[a]\n 0=>uses rp2 in zp1\n 1=>uses rp1 in zp0\nPops number of contour to be shifted\nShifts the entire contour by the amount\nreference point was shifted");
    ihaddr(0x32,0x33,"SHift Point using reference point[a]\n 0=>uses rp2 in zp1\n 1=>uses rp1 in zp0\nPops as many points as specified by the loop count\nShifts each by the amount the reference\npoint was shifted");
    ihadd(0x38,"SHift point by a PIXel amount\nPops an amount (26.6) and as many points\nas the loop counter specifies\neach point is shifted along the FREEDOM vector");
    ihaddr(0x36,0x37,"SHift Zone using reference point[a]\n 0=>uses rp2 in zp1\n 1=>uses rp1 in zp0\nPops the zone to be shifted\nShifts all points in zone by the amount\nthe reference point was shifted");
    ihadd(0x17,"Set LOOP variable\nPops the new value for the loop counter\nDefaults to 1 after each use");
    ihadd(0x1a,"Set Minimum Distance\nPops a 26.6 value from stack to be new minimum distance");
    ihadd(0x0a,"Set Projection Vector From Stack\npops 2 2.14 values (x,y) from stack\nmust be a unit vector");
    ihaddr(0x02,0x03,"Set Projection Vector To Coordinate Axis[a]\n 0=>y axis\n 1=>x axis\n" );
    ihaddr(0x06,0x07,"Set Projection Vector To Line[a]\n 0 => parallel to line\n 1=>orthogonal to line\nPops two points used to establish the line\nSets the projection vector" );
    ihadd(0x76,"Super ROUND\nToo complicated. Look it up");
    ihadd(0x10,"Set Reference Point 0\nPops a point which becomes the new rp0");
    ihadd(0x11,"Set Reference Point 1\nPops a point which becomes the new rp1");
    ihadd(0x12,"Set Reference Point 2\nPops a point which becomes the new rp2");
    ihadd(0x1f,"Set Single Width\nPops value for single width value (FUnit)");
    ihadd(0x1e,"Set Single Width Cut-In\nPops value for single width cut-in value (26.6)");
    ihadd(0x61,"SUBtract\nPops two 26.6 fixed numbers from stack\nsubtracts them, pushes result");
    ihaddr(0x00,0x01,"Set freedom & projection Vectors To Coordinate Axis[a]\n 0=>both to y axis\n 1=>both to x axis\n" );
    ihadd(0x23,"SWAP top two elements on stack");
    ihadd(0x13,"Set Zone Pointer 0\nPops the zone number into zp0");
    ihadd(0x14,"Set Zone Pointer 1\nPops the zone number into zp1");
    ihadd(0x15,"Set Zone Pointer 2\nPops the zone number into zp2");
    ihadd(0x16,"Set Zone PointerS\nPops the zone number into zp0,zp1 and zp2");
    ihadd(0x29,"UnTouch Point\nPops a point number and marks it untouched");
    ihadd(0x70,"Write Control Value Table in Funits\nPops a number(Funits) and a\nCVT index and writes the number to cvt[index]");
    ihadd(0x44,"Write Control Value Table in Pixel units\nPops a number(26.6) and a\nCVT index and writes the number to cvt[index]");
    ihadd(0x42,"Write Store\nPops a value and an index and writes the value to storage[index]");
}

typedef struct instrview /* : tableview */ {
    Table *table;
    GWindow gw, v;
    struct tableviewfuncs *virtuals;
    TtfFont *font;		/* for the encoding currently used */
    struct ttfview *owner;
/* instrs specials */
    GGadget *mb, *vsb;
    int lpos, lheight;
    int16 as, fh;
    int16 vheight, vwidth;
    int16 mbh;
    GFont *gfont;
    uint8 *bts;
} InstrView;

static void instr_typify(InstrView *iv) {
    int i, len = iv->table->newlen, cnt, j, lh;
    uint8 *instrs = iv->table->data;
    uint8 *bts;

    if ( iv->bts==NULL )
	iv->bts = galloc(len);
    bts = iv->bts;
    for ( i=lh=0; i<len; ++i ) {
	bts[i] = bt_instr;
	++lh;
	if ( instrs[i]==0x40 ) {
	    /* NPUSHB */
	    bts[++i] = bt_cnt;
	    cnt = instrs[i];
	    for ( j=0 ; j<cnt; ++j)
		bts[++i] = bt_byte;
	    lh += 1+cnt;
	} else if ( instrs[i]==0x41 ) {
	    /* NPUSHW */
	    bts[++i] = bt_cnt; ++lh;
	    cnt = instrs[i];
	    for ( j=0 ; j<cnt; ++j) {
		bts[++i] = bt_wordhi;
		bts[++i] = bt_wordlo;
	    }
	    lh += 1+cnt;
	} else if ( (instrs[i]&0xf8) == 0xb0 ) {
	    /* PUSHB[n] */
	    cnt = (instrs[i]&7)+1;
	    for ( j=0 ; j<cnt; ++j)
		bts[++i] = bt_byte;
	    lh += cnt;
	} else if ( (instrs[i]&0xf8) == 0xb8 ) {
	    /* PUSHW[n] */
	    cnt = (instrs[i]&7)+1;
	    for ( j=0 ; j<cnt; ++j) {
		bts[++i] = bt_wordhi;
		bts[++i] = bt_wordlo;
	    }
	    lh += cnt;
	}
    }
    iv->lheight = lh;
}

#define MID_Revert	2702
#define MID_Recent	2703
#define MID_Cut		2101
#define MID_Copy	2102
#define MID_Paste	2103
#define MID_SelAll	2106

static int instr_processdata(TableView *tv) {
    InstrView *iv = (InstrView *) tv;
    /* Do changes!!! */
return( true );
}

static int instr_close(TableView *tv) {
    if ( instr_processdata(tv)) {
	GDrawDestroyWindow(tv->gw);
return( true );
    }
return( false );
}

static struct tableviewfuncs instrfuncs = { instr_close, instr_processdata };

static void instr_resize(InstrView *iv,GEvent *event) {
    GRect pos;
    int lh;

    /* Multiple of the number of lines we've got */
    if ( (event->u.resize.size.height-iv->mbh-4)%iv->fh!=0 ) {
	int lc = (event->u.resize.size.height-iv->mbh+iv->fh/2)/iv->fh;
	if ( lc<=0 ) lc = 1;
	GDrawResize(iv->gw, event->u.resize.size.width,lc*iv->fh+iv->mbh+4);
return;
    }

    pos.width = GDrawPointsToPixels(iv->gw,_GScrollBar_Width);
    pos.height = event->u.resize.size.height-iv->mbh;
    pos.x = event->u.resize.size.width-pos.width; pos.y = iv->mbh;
    GGadgetResize(iv->vsb,pos.width,pos.height);
    GGadgetMove(iv->vsb,pos.x,pos.y);
    pos.width = pos.x; pos.x = 0;
    GDrawResize(iv->v,pos.width,pos.height);

    iv->vheight = pos.height; iv->vwidth = pos.width;
    lh = iv->lheight;

    GScrollBarSetBounds(iv->vsb,0,lh,iv->vheight/iv->fh);
    if ( iv->lpos + iv->vheight/iv->fh > lh )
	iv->lpos = lh-iv->vheight/iv->fh;
    GScrollBarSetPos(iv->vsb,iv->lpos);
}

static void instr_expose(InstrView *iv,GWindow pixmap,GRect *rect) {
    int low, high;
    int i,x,y;
    Table *table = iv->table;
    char loc[8], ins[8], val[8]; unichar_t uloc[8], uins[8], uname[30];
    int addr_end, num_end;
    static unichar_t nums[] = { '0', '0', '0', '0', '0', '0', '\0' };

    GDrawSetFont(pixmap,iv->gfont);
    addr_end = GDrawGetTextWidth(pixmap,nums,6,NULL)+2;
    num_end = addr_end + GDrawGetTextWidth(pixmap,nums,5,NULL)+4;

    low = ( (rect->y-2)/iv->fh ) * iv->fh +2;
    high = ( (rect->y+rect->height+iv->fh-1-2)/iv->fh ) * iv->fh +2;

    GDrawDrawLine(pixmap,addr_end,rect->y,addr_end,rect->y+rect->height,0x000000);
    GDrawDrawLine(pixmap,num_end,rect->y,num_end,rect->y+rect->height,0x000000);

    for ( i=0, y=2-iv->lpos*iv->fh; y<low && i<table->newlen; ++i ) {
	if ( iv->bts[i]==bt_wordhi )
	    ++i;
	y += iv->fh;
    }
    for ( ; y<=high && i<table->newlen; ++i ) {
	sprintf( loc, "%d", i ); uc_strcpy(uloc,loc);
	if ( iv->bts[i]==bt_wordhi ) {
	    sprintf( ins, " %02x%02x", table->data[i], table->data[i+1]); uc_strcpy(uins,ins);
	    sprintf( val, " %d", (short) ((table->data[i]<<8) | table->data[i+1]) );
	    uc_strcpy(uname,val);
	    ++i;
	} else if ( iv->bts[i]==bt_cnt || iv->bts[i]==bt_byte ) {
	    sprintf( ins, " %02x", table->data[i] ); uc_strcpy(uins,ins);
	    sprintf( val, " %d", table->data[i]);
	    uc_strcpy(uname,val);
	} else {
	    sprintf( ins, "%02x", table->data[i] ); uc_strcpy(uins,ins);
	    uc_strcpy(uname, instrs[table->data[i]]);
	}

	x = addr_end - 2 - GDrawGetTextWidth(pixmap,uloc,-1,NULL);
	GDrawDrawText(pixmap,x,y+iv->as,uloc,-1,NULL,0x000000);
	x = addr_end + 2;
	GDrawDrawText(pixmap,x,y+iv->as,uins,-1,NULL,0x000000);
	GDrawDrawText(pixmap,num_end+2,y+iv->as,uname,-1,NULL,0x000000);
	y += iv->fh;
    }
}

static void instr_mousemove(InstrView *iv,int pos) {
    int i,y;
    static unichar_t buffer[120]; const
    unichar_t *msg;

    pos = ((pos-2)/iv->fh) * iv->fh + 2;

    for ( i=0, y=2-iv->lpos*iv->fh; y<pos && i<iv->table->newlen; ++i ) {
	if ( iv->bts[i]==bt_wordhi )
	    ++i;
	y += iv->fh;
    }
    switch ( iv->bts[i] ) {
      case bt_wordhi: case bt_wordlo:
	uc_strcpy(buffer,"A short to be pushed on the stack");
	msg = buffer;
      break;
      case bt_cnt:
	uc_strcpy(buffer,"A count specifying how many bytes/shorts\nshould be pushed on the stack");
	msg = buffer;
      break;
      case bt_byte:
	uc_strcpy(buffer,"An unsigned byte to be pushed on the stack");
	msg = buffer;
      break;
      case bt_instr:
	if ( (msg = instrhelppopup[iv->table->data[i]])==NULL ) {
	    uc_strcpy(buffer,"???");
	    msg = buffer;
	}
      break;
      default:
	uc_strcpy(buffer,"???");
	msg = buffer;
      break;
    }
    GGadgetPreparePopup(iv->gw,msg);
}

static void fllistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Recent:
	    mi->ti.disabled = !RecentFilesAny();
	  break;
	}
    }
}

static void instr_scroll(InstrView *iv,struct sbevent *sb) {
    int newpos = iv->lpos;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= iv->vheight/iv->fh;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
        newpos += iv->vheight/iv->fh;
      break;
      case et_sb_bottom:
        newpos = iv->lheight-iv->vheight/iv->fh;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos>iv->lheight-iv->vheight/iv->fh )
        newpos = iv->lheight-iv->vheight/iv->fh;
    if ( newpos<0 ) newpos =0;
    if ( newpos!=iv->lpos ) {
	int diff = newpos-iv->lpos;
	iv->lpos = newpos;
	GScrollBarSetPos(iv->vsb,iv->lpos);
	GDrawScroll(iv->v,NULL,0,diff*iv->fh);
    }
}

static void InstrViewFree(InstrView *iv) {
    iv->table->tv = NULL;
    free(iv->bts);
    free(iv);
}

static int iv_v_e_h(GWindow gw, GEvent *event) {
    InstrView *iv = (InstrView *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_expose:
	instr_expose(iv,gw,&event->u.expose.rect);
      break;
      case et_char:
	if ( event->u.chr.keysym == GK_Help || event->u.chr.keysym == GK_F1 )
	    TableHelp(iv->table->name);
      break;
      case et_mousemove: case et_mousedown: case et_mouseup:
	GGadgetEndPopup();
	if ( event->type==et_mousemove )
	    instr_mousemove(iv,event->u.mouse.y);
      break;
      case et_timer:
      break;
      case et_focus:
      break;
    }
return( true );
}

static int iv_e_h(GWindow gw, GEvent *event) {
    InstrView *iv = (InstrView *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_expose:
      break;
      case et_resize:
	instr_resize(iv,event);
      break;
      case et_char:
	if ( event->u.chr.keysym == GK_Help || event->u.chr.keysym == GK_F1 )
	    TableHelp(iv->table->name);
      break;
      case et_controlevent:
	switch ( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    instr_scroll(iv,&event->u.control.u.sb);
	  break;
	}
      break;
      case et_close:
	instr_close((TableView *) iv);
      break;
      case et_destroy:
	InstrViewFree(iv);
      break;
    }
return( true );
}

static void IVMenuClose(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    InstrView *iv = (InstrView *) GDrawGetUserData(gw);

    DelayEvent((void (*)(void *)) instr_close, iv);
}

static void IVMenuSaveAs(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    TtfView *tfv = ((InstrView *) GDrawGetUserData(gw))->owner;

    _TFVMenuSaveAs(tfv);
}

static void IVMenuSave(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    TtfView *tfv = ((InstrView *) GDrawGetUserData(gw))->owner;
    _TFVMenuSave(tfv);
}

static void IVMenuRevert(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    TtfView *tfv = ((InstrView *) GDrawGetUserData(gw))->owner;
    _TFVMenuRevert(tfv);
}

static GMenuItem dummyitem[] = { { (unichar_t *) _STR_Recent, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'N' }, NULL };
static GMenuItem fllist[] = {
    { { (unichar_t *) _STR_Open, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'O' }, 'O', ksm_control, NULL, NULL, MenuOpen },
    { { (unichar_t *) _STR_Recent, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 't' }, '\0', ksm_control, dummyitem, MenuRecentBuild, NULL, MID_Recent },
    { { (unichar_t *) _STR_Close, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'C' }, 'Q', ksm_control|ksm_shift, NULL, NULL, IVMenuClose },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Save, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'S' }, 'S', ksm_control, NULL, NULL, IVMenuSave },
    { { (unichar_t *) _STR_Saveas, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'a' }, 'S', ksm_control|ksm_shift, NULL, NULL, IVMenuSaveAs },
    { { (unichar_t *) _STR_Revertfile, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'R' }, 'R', ksm_control|ksm_shift, NULL, NULL, IVMenuRevert, MID_Revert },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Prefs, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'e' }, '\0', ksm_control, NULL, NULL, MenuPrefs },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Quit, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'Q' }, 'Q', ksm_control, NULL, NULL, MenuExit },
    { NULL }
};

static GMenuItem edlist[] = {
    { { (unichar_t *) _STR_Undo, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 'U' }, 'Z', ksm_control, NULL, NULL },
    { { (unichar_t *) _STR_Redo, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 'R' }, 'Y', ksm_control, NULL, NULL },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Cut, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 't' }, 'X', ksm_control, NULL, NULL, NULL, MID_Cut },
    { { (unichar_t *) _STR_Copy, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 'C' }, 'C', ksm_control, NULL, NULL, NULL, MID_Copy },
    { { (unichar_t *) _STR_Paste, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 'P' }, 'V', ksm_control, NULL, NULL, NULL, MID_Paste },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_SelectAll, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 'A' }, 'A', ksm_control, NULL, NULL, NULL },
    { NULL }
};

extern GMenuItem helplist[];

static GMenuItem mblist[] = {
    { { (unichar_t *) _STR_File, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'F' }, 0, 0, fllist, fllistcheck },
    { { (unichar_t *) _STR_Edit, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'E' }, 0, 0, edlist },
    { { (unichar_t *) _STR_Window, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'W' }, 0, 0, NULL, WindowMenuBuild, NULL },
    { { (unichar_t *) _STR_Help, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'H' }, 0, 0, helplist, NULL },
    { NULL }
};


/* fpgm, prep */
void instrCreateEditor(Table *tab,TtfView *tfv) {
    InstrView *iv = gcalloc(1,sizeof(InstrView));
    unichar_t title[60];
    GRect pos, gsize;
    GWindow gw;
    GWindowAttrs wattrs;
    FontRequest rq;
    static unichar_t monospace[] = { 'c','o','u','r','i','e','r',',','m', 'o', 'n', 'o', 's', 'p', 'a', 'c', 'e',',','c','a','s','l','o','n',',','u','n','i','f','o','n','t', '\0' };
    int as,ds,ld, lh;
    GGadgetData gd;
    GGadget *sb;

    instrhelpsetup();

    iv->table = tab;
    iv->virtuals = &instrfuncs;
    iv->owner = tfv;
    iv->font = tfv->ttf->fonts[tfv->selectedfont];
    tab->tv = (TableView *) iv;

    TableFillup(tab);
    instr_typify(iv);

    title[0] = (tab->name>>24)&0xff;
    title[1] = (tab->name>>16)&0xff;
    title[2] = (tab->name>>8 )&0xff;
    title[3] = (tab->name    )&0xff;
    title[4] = ' ';
    u_strncpy(title+5, iv->font->fontname, sizeof(title)/sizeof(title[0])-6);

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_icon;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = title;
    wattrs.icon = ttf_icon;
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,200);
    pos.height = GDrawPointsToPixels(NULL,100);
    iv->gw = gw = GDrawCreateTopWindow(NULL,&pos,iv_e_h,iv,&wattrs);

    memset(&gd,0,sizeof(gd));
    gd.flags = gg_visible | gg_enabled;
    gd.u.menu = mblist;
    iv->mb = GMenuBarCreate( gw, &gd, NULL);
    GGadgetGetSize(iv->mb,&gsize);
    iv->mbh = gsize.height;

    gd.pos.y = iv->mbh; gd.pos.height = pos.height-iv->mbh;
    gd.pos.width = GDrawPointsToPixels(gw,_GScrollBar_Width);
    gd.pos.x = pos.width-gd.pos.width;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_sb_vert;
    iv->vsb = sb = GScrollBarCreate(gw,&gd,iv);

    wattrs.mask = wam_events|wam_cursor;
    pos.x = 0; pos.y = iv->mbh;
    pos.width = gd.pos.x; pos.height -= iv->mbh;
    iv->v = GWidgetCreateSubWindow(gw,&pos,iv_v_e_h,iv,&wattrs);
    GDrawSetVisible(iv->v,true);

    memset(&rq,0,sizeof(rq));
    rq.family_name = monospace;
    rq.point_size = -12;
    rq.weight = 400;
    iv->gfont = GDrawInstanciateFont(GDrawGetDisplayOfWindow(gw),&rq);
    GDrawSetFont(iv->v,iv->gfont);
    GDrawFontMetrics(iv->gfont,&as,&ds,&ld);
    iv->as = as+1;
    iv->fh = iv->as+ds;

    lh = iv->lheight;
    if ( lh>40 ) lh = 40;
    if ( lh<4 ) lh = 4;
    GDrawResize(iv->gw,pos.width+gd.pos.width,iv->mbh+lh*iv->fh+4);

    GDrawSetVisible(gw,true);
}
