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
#include "fontforgeui.h"
#include "cvundoes.h"
#include <gkeysym.h>
#include <ustring.h>
#include <utype.h>
#include "ttf.h"
#include "ttfinstrs.h"

extern GBox _ggadget_Default_Box;
#define ACTIVE_BORDER   (_ggadget_Default_Box.active_border)
#define MAIN_FOREGROUND (_ggadget_Default_Box.main_foreground)

extern int _GScrollBar_Width;
#define EDGE_SPACING	2

#define ttf_width 16
#define ttf_height 16
static unsigned char ttf_bits[] = {
   0xff, 0x07, 0x21, 0x04, 0x20, 0x00, 0xfc, 0x7f, 0x24, 0x40, 0xf4, 0x5e,
   0xbc, 0x72, 0xa0, 0x02, 0xa0, 0x02, 0xa0, 0x02, 0xf0, 0x02, 0x80, 0x02,
   0xc0, 0x06, 0x40, 0x04, 0xc0, 0x07, 0x00, 0x00};
GWindow ttf_icon = NULL;

static char *instrhelppopup[256];

static void ihaddr(int bottom,int top,char *msg) {
    while ( bottom<=top )
        instrhelppopup[bottom++] = msg;
}

static void ihadd(int p,char *msg) {
    ihaddr(p,p,msg);
}

static void instrhelpsetup(void) {
    if ( instrhelppopup[0]!=NULL )
return;
    ihadd(0x7f,N_("Adjust Angle\nObsolete instruction\nPops one value"));
    ihadd(0x64,N_("ABSolute Value\nReplaces top of stack with its abs"));
    ihadd(0x60,N_("ADD\nPops two 26.6 fixed numbers from stack\nadds them, pushes result"));
    ihadd(0x27,N_("ALIGN PoinTS\nAligns (&pops) the two points which are on the stack\nby moving along freedom vector to the average of their\npositions on projection vector"));
    ihadd(0x3c,N_("ALIGN to Reference Point\nPops as many points as specified in loop counter\nAligns points with RP0 by moving each\nalong freedom vector until distance to\nRP0 on projection vector is 0"));
    ihadd(0x5a,N_("logical AND\nPops two values, ands them, pushes result"));
    ihadd(0x2b,N_("CALL function\nPops a value, calls the function represented by it"));
    ihadd(0x67,N_("CEILING\nPops one 26.6 value, rounds upward to an int\npushes result"));
    ihadd(0x25,N_("Copy INDEXed element to stack\nPops an index & copies stack\nelement[index] to top of stack"));
    ihadd(0x22,N_("CLEAR\nPops all elements on stack"));
    ihadd(0x4f,N_("DEBUG call\nPops a value and executes a debugging interpreter\n(if available)"));
    ihadd(0x73,N_("DELTA exception C1\nPops a value n & then n exception specifications & cvt entries\nchanges each cvt entry at a given size by the pixel amount"));
    ihadd(0x74,N_("DELTA exception C2\nPops a value n & then n exception specifications & cvt entries\nchanges each cvt entry at a given size by the amount"));
    ihadd(0x75,N_("DELTA exception C3\nPops a value n & then n exception specifications & cvt entries\nchanges each cvt entry at a given size by the amount"));
    ihadd(0x5D,N_("DELTA exception P1\nPops a value n & then n exception specifications & points\nmoves each point at a given size by the amount"));
    ihadd(0x71,N_("DELTA exception P2\nPops a value n & then n exception specifications & points\nmoves each point at a given size by the amount"));
    ihadd(0x72,N_("DELTA exception P3\nPops a value n & then n exception specifications & points\nmoves each point at a given size by the amount"));
    ihadd(0x24,N_("DEPTH of stack\nPushes the number of elements on the stack"));
    ihadd(0x62,N_("DIVide\nPops two 26.6 numbers, divides them, pushes result"));
    ihadd(0x20,N_("DUPlicate top stack element\nPushes the top stack element again"));
    ihadd(0x59,N_("End IF\nEnds an IF or IF-ELSE sequence"));
    ihadd(0x1b,N_("ELSE clause\nStart of Else clause of preceding IF"));
    ihadd(0x2d,N_("END Function definition"));
    ihadd(0x54,N_("EQual\nPops two values, tests for equality, pushes result(0/1)"));
    ihadd(0x57,N_("EVEN\nPops one value, rounds it and tests if it is even(0/1)"));
    ihadd(0x2C,N_("Function DEFinition\nPops a value (n) and starts the nth\nfunction definition"));
    ihadd(0x4e,N_("set the auto FLIP boolean to OFF"));
    ihadd(0x4d,N_("set the auto FLIP boolean to ON"));
    ihadd(0x80,N_("FLIP PoinT\nPops as many points as specified in loop counter\nFlips whether each point is on/off curve"));
    ihadd(0x82,N_("FLIP RanGe OFF\nPops two point numbers\nsets all points between to be off curve points"));
    ihadd(0x81,N_("FLIP RanGe ON\nPops two point numbers\nsets all points between to be on curve points"));
    ihadd(0x66,N_("FLOOR\nPops a value, rounds to lowest int, pushes result"));
    ihaddr(0x46,0x47,N_("Get Coordinate[a] projected onto projection vector\n 0=>use current pos\n 1=>use original pos\nPops one point, pushes the coordinate of\nthe point along projection vector"));
    ihadd(0x88,N_("GET INFOrmation\nPops information type, pushes result"));
    ihadd(0x0d,N_("Get Freedom Vector\nDecomposes freedom vector, pushes its\ntwo coordinates onto stack as 2.14"));
    ihadd(0x0c,N_("Get Projection Vector\nDecomposes projection vector, pushes its\ntwo coordinates onto stack as 2.14"));
    ihadd(0x52,N_("Greater Than\nPops two values, pushes (0/1) if bottom el > top"));
    ihadd(0x53,N_("Greater Than or EQual\nPops two values, pushes (0/1) if bottom el >= top"));
    ihadd(0x89,N_("Instruction DEFinition\nPops a value which becomes the opcode\nand begins definition of new instruction"));
    ihadd(0x58,N_("IF test\nPops an integer,\nif 0 (false) next instruction is ELSE or EIF\nif non-0 execution continues normally\n(unless there's an ELSE)"));
    ihadd(0x8e,N_("INSTRuction execution ConTRoL\nPops a selector and value\nSets a state variable"));
    ihadd(0x39,N_("Interpolate Point\nPops as many points as specified in loop counter\nInterpolates each point to preserve original status\nwith respect to RP1 and RP2"));
    ihadd(0x0f,N_("moves point to InterSECTion of two lines\nPops start,end start,end points of two lines\nand a point to move. Point is moved to\nintersection"));
    ihaddr(0x30,0x31,N_("Interpolate Untouched Points[a]\n 0=> interpolate in y direction\n 1=> x direction"));
    ihadd(0x1c,N_("JuMP Relative\nPops offset (in bytes) to move the instruction pointer"));
    ihadd(0x79,N_("Jump Relative On False\nPops a boolean and an offset\nChanges instruction pointer by offset bytes\nif boolean is false"));
    ihadd(0x78,N_("Jump Relative On True\nPops a boolean and an offset\nChanges instruction pointer by offset bytes\nif boolean is true"));
    ihadd(0x2a,N_("LOOP and CALL function\nPops a function number & count\nCalls function count times"));
    ihadd(0x50,N_("Less Than\nPops two values, pushes (0/1) if bottom el < top"));
    ihadd(0x51,N_("Less Than or EQual\nPops two values, pushes (0/1) if bottom el <= top"));
    ihadd(0x8b,N_("MAXimum of top two stack entries\nPops two values, pushes the maximum back"));
    ihaddr(0x49,0x4a,N_("Measure Distance[a]\n 0=>distance with current positions\n 1=>distance with original positions\nPops two point numbers, pushes distance between them"));
    ihaddr(0x2e,0x2f,N_("Move Direct Absolute Point[a]\n 0=>do not round\n 1=>round\nPops a point number, touches that point\nand perhaps rounds it to the grid along\nthe projection vector. Sets rp0&rp1 to the point"));
    ihaddr(0xc0,0xdf,N_("Move Direct Relative Point[abcde]\n a=0=>don't set rp0\n a=1=>set rp0 to p\n b=0=>do not keep distance more than minimum\n b=1=>keep distance at least minimum\n c=0 do not round\n c=1 round\n de=0 => grey distance\n de=1 => black distance\n de=2 => white distance\nPops a point moves it so that it maintains\nits original distance to the rp0. Sets\nrp1 to rp0, rp2 to point, sometimes rp0 to point"));
    ihaddr(0x3e,0x3f,N_("Move Indirect Absolute Point[a]\n 0=>do not round, don't use cvt cutin\n 1=>round\nPops a point number & a cvt entry,\ntouches the point and moves it to the coord\nspecified in the cvt (along the projection vector).\nSets rp0&rp1 to the point"));
    ihadd(0x8c,N_("Minimum of top two stack entries\nPops two values, pushes the minimum back"));
    ihadd(0x26,N_("Move INDEXed element to stack\nPops an index & moves stack\nelement[index] to top of stack\n(removing it from where it was)"));
    ihaddr(0xe0,0xff,N_("Move Indirect Relative Point[abcde]\n a=0=>don't set rp0\n a=1=>set rp0 to p\n b=0=>do not keep distance more than minimum\n b=1=>keep distance at least minimum\n c=0 do not round nor use cvt cutin\n c=1 round & use cvt cutin\n de=0 => grey distance\n de=1 => black distance\n de=2 => white distance\nPops a cvt index and a point moves it so that it\nis cvt[index] from rp0. Sets\nrp1 to rp0, rp2 to point, sometimes rp0 to point"));
    ihadd(0x4b,N_("Measure Pixels Per EM\nPushs the pixels per em (for current rasterization)"));
    ihadd(0x4c,N_("Measure Point Size\nPushes the current point size"));
    ihaddr(0x3a,0x3b,N_("Move Stack Indirect Relative Point[a]\n 0=>do not set rp0\n 1=>set rp0 to point\nPops a 26.6 distance and a point\nMoves point so it is distance from rp0"));
    ihadd(0x63,N_("MULtiply\nPops two 26.6 numbers, multiplies them, pushes result"));
    ihadd(0x65,N_("NEGate\nNegates the top of the stack"));
    ihadd(0x55,N_("Not EQual\nPops two values, tests for inequality, pushes result(0/1)"));
    ihadd(0x5c,N_("logical NOT\nPops a number, if 0 pushes 1, else pushes 0"));
    ihadd(0x40,N_("N PUSH Bytes\nReads an (unsigned) count byte from the\ninstruction stream, then reads and pushes\nthat many unsigned bytes"));
    ihadd(0x41,N_("N PUSH Words\nReads an (unsigned) count byte from the\ninstruction stream, then reads and pushes\nthat many signed 2byte words"));
    ihaddr(0x6c,0x6f,N_("No ROUNDing of value[ab]\n ab=0 => grey distance\n ab=1 => black distance\n ab=2 => white distance\nPops a coordinate (26.6), changes it (without\nrounding) to compensate for engine effects\npushes it back"));
    ihadd(0x56,N_("ODD\nPops one value, rounds it and tests if it is odd(0/1)"));
    ihadd(0x5b,N_("logical OR\nPops two values, ors them, pushes result"));
    ihadd(0x21,N_("POP top stack element"));
    ihaddr(0xb0,0xb7,N_("PUSH Byte[abc]\n abc is the number-1 of bytes to push\nReads abc+1 unsigned bytes from\nthe instruction stream and pushes them"));
    ihaddr(0xb8,0xbf,N_("PUSH Word[abc]\n abc is the number-1 of words to push\nReads abc+1 signed words from\nthe instruction stream and pushes them"));
    ihadd(0x45,N_("Read Control Value Table entry\nPops an index to the CVT and\npushes it in 26.6 format"));
    ihadd(0x7d,N_("Round Down To Grid\n\nSets round state to the obvious"));
    ihadd(0x7a,N_("Round OFF\nSets round state so that no rounding occurs\nbut engine compensation does"));
    ihadd(0x8a,N_("ROLL the top three stack elements"));
    ihaddr(0x68,0x6b,N_("ROUND value[ab]\n ab=0 => grey distance\n ab=1 => black distance\n ab=2 => white distance\nRounds a coordinate (26.6) at top of stack\nand compensates for engine effects"));
    ihadd(0x43,N_("Read Store\nPops an index into store array\nPushes value at that index"));
    ihadd(0x3d,N_("Round To Double Grid\nSets the round state (round to closest .5/int)"));
    ihadd(0x18,N_("Round To Grid\nSets the round state"));
    ihadd(0x19,N_("Round To Half Grid\nSets the round state (round to closest .5 not int)"));
    ihadd(0x7c,N_("Round Up To Grid\nSets the round state"));
    ihadd(0x77,N_("Super 45\302\260 ROUND\nToo complicated. Look it up"));
    ihadd(0x7e,N_("Set ANGle Weight\nPops an int, and sets the angle\nweight state variable to it\nObsolete"));
    ihadd(0x85,N_("SCAN conversion ConTRoL\nPops a number which sets the\ndropout control mode"));
    ihadd(0x8d,N_("SCANTYPE\nPops number which sets which scan\nconversion rules to use"));
    ihadd(0x48,N_("Sets Coordinate From Stack using projection & freedom vectors\nPops a coordinate 26.6 and a point\nMoves point to given coordinate"));
    ihadd(0x1d,N_("Sets Control Value Table Cut-In\nPops 26.6 from stack, sets cvt cutin"));
    ihadd(0x5e,N_("Set Delta Base\nPops value sets delta base"));
    ihaddr(0x86,0x87,N_("Set Dual Projection Vector To Line[a]\n 0 => parallel to line\n 1=>orthogonal to line\nPops two points used to establish the line\nSets a second projection vector based on original\npositions of points"));
    ihadd(0x5F,N_("Set Delta Shift\nPops a new value for delta shift"));
    ihadd(0x0b,N_("Set Freedom Vector From Stack\npops 2 2.14 values (x,y) from stack\nmust be a unit vector"));
    ihaddr(0x04,0x05,N_("Set Freedom Vector To Coordinate Axis[a]\n 0=>y axis\n 1=>x axis\n"));
    ihaddr(0x08,0x09,N_("Set Freedom Vector To Line[a]\n 0 => parallel to line\n 1=>orthogonal to line\nPops two points used to establish the line\nSets the freedom vector"));
    ihadd(0x0e,N_("Set Freedom Vector To Projection Vector"));
    ihaddr(0x34,0x35,N_("SHift Contour using reference point[a]\n 0=>uses rp2 in zp1\n 1=>uses rp1 in zp0\nPops number of contour to be shifted\nShifts the entire contour by the amount\nreference point was shifted"));
    ihaddr(0x32,0x33,N_("SHift Point using reference point[a]\n 0=>uses rp2 in zp1\n 1=>uses rp1 in zp0\nPops as many points as specified by the loop count\nShifts each by the amount the reference\npoint was shifted"));
    ihadd(0x38,N_("SHift point by a PIXel amount\nPops an amount (26.6) and as many points\nas the loop counter specifies\neach point is shifted along the FREEDOM vector"));
    ihaddr(0x36,0x37,N_("SHift Zone using reference point[a]\n 0=>uses rp2 in zp1\n 1=>uses rp1 in zp0\nPops the zone to be shifted\nShifts all points in zone by the amount\nthe reference point was shifted"));
    ihadd(0x17,N_("Set LOOP variable\nPops the new value for the loop counter\nDefaults to 1 after each use"));
    ihadd(0x1a,N_("Set Minimum Distance\nPops a 26.6 value from stack to be new minimum distance"));
    ihadd(0x0a,N_("Set Projection Vector From Stack\npops 2 2.14 values (x,y) from stack\nmust be a unit vector"));
    ihaddr(0x02,0x03,N_("Set Projection Vector To Coordinate Axis[a]\n 0=>y axis\n 1=>x axis\n" ));
    ihaddr(0x06,0x07,N_("Set Projection Vector To Line[a]\n 0 => parallel to line\n 1=>orthogonal to line\nPops two points used to establish the line\nSets the projection vector" ));
    ihadd(0x76,N_("Super ROUND\nToo complicated. Look it up"));
    ihadd(0x10,N_("Set Reference Point 0\nPops a point which becomes the new rp0"));
    ihadd(0x11,N_("Set Reference Point 1\nPops a point which becomes the new rp1"));
    ihadd(0x12,N_("Set Reference Point 2\nPops a point which becomes the new rp2"));
    ihadd(0x1f,N_("Set Single Width\nPops value for single width value (FUnit)"));
    ihadd(0x1e,N_("Set Single Width Cut-In\nPops value for single width cut-in value (26.6)"));
    ihadd(0x61,N_("SUBtract\nPops two 26.6 fixed numbers from stack\nsubtracts them, pushes result"));
    ihaddr(0x00,0x01,N_("Set freedom & projection Vectors To Coordinate Axis[a]\n 0=>both to y axis\n 1=>both to x axis\n" ));
    ihadd(0x23,N_("SWAP top two elements on stack"));
    ihadd(0x13,N_("Set Zone Pointer 0\nPops the zone number into zp0"));
    ihadd(0x14,N_("Set Zone Pointer 1\nPops the zone number into zp1"));
    ihadd(0x15,N_("Set Zone Pointer 2\nPops the zone number into zp2"));
    ihadd(0x16,N_("Set Zone PointerS\nPops the zone number into zp0,zp1 and zp2"));
    ihadd(0x29,N_("UnTouch Point\nPops a point number and marks it untouched"));
    ihadd(0x70,N_("Write Control Value Table in Funits\nPops a number(Funits) and a\nCVT index and writes the number to cvt[index]"));
    ihadd(0x44,N_("Write Control Value Table in Pixel units\nPops a number(26.6) and a\nCVT index and writes the number to cvt[index]"));
    ihadd(0x42,N_("Write Store\nPops a value and an index and writes the value to storage[index]"));
}

typedef struct instrdlg /* : InstrBase */{
    unsigned int inedit: 1;
    struct instrdata *instrdata;
    struct instrinfo instrinfo;
    int oc_height;
    GWindow gw, v;
    GGadget *ok, *cancel, *edit, *parse, *text, *topbox;
} InstrDlg;

static void instr_info_init(struct instrinfo *instrinfo) {

    instrinfo->lheight = instr_typify(instrinfo->instrdata);
    if ( instrinfo->fh!=0 ) {
	if ( instrinfo->lpos > instrinfo->lheight-instrinfo->vheight/instrinfo->fh )
	    instrinfo->lpos = instrinfo->lheight-instrinfo->vheight/instrinfo->fh;
	if ( instrinfo->lpos<0 )
	    instrinfo->lpos = 0;
    }
}

static void instr_resize(InstrDlg *iv,GEvent *event) {
    GRect size;
    int lh;
    struct instrinfo *ii = &iv->instrinfo;

    GGadgetGetSize(iv->text,&size);
    GDrawMove(ii->v,size.x,size.y);
    GDrawResize(ii->v,size.width,size.height);
    ii->vheight = size.height; ii->vwidth = size.width;
    lh = ii->lheight;

    GScrollBarSetBounds(ii->vsb,0,lh+2,ii->vheight<ii->fh ? 1 : ii->vheight/ii->fh);
    if ( ii->lpos + ii->vheight/ii->fh > lh )
	ii->lpos = lh-ii->vheight/ii->fh;
    if ( ii->lpos<0 ) ii->lpos = 0;
    GScrollBarSetPos(ii->vsb,ii->lpos);
    GDrawRequestExpose(iv->gw,NULL,false);
}

static void IVError(void *_iv,char *msg,int offset) {
    InstrDlg *iv = _iv;

    if ( iv!=NULL ) {
	GTextFieldSelect(iv->text,offset,offset);
	GTextFieldShow(iv->text,offset);
	GWidgetIndicateFocusGadget(iv->text);
    }
    ff_post_error(_("Parse Error"),msg);
}

static int IVParse(InstrDlg *iv) {
    char *text = GGadgetGetTitle8(iv->text);
    int icnt=0, i;
    uint8 *instrs;

    instrs = _IVParse(iv->instrdata->sf, text, &icnt, IVError, iv);
    free(text);

    if ( instrs==NULL )
return( false );
    if ( icnt!=iv->instrdata->instr_cnt )
	iv->instrdata->changed = true;
    else {
	for ( i=0; i<icnt; ++i )
	    if ( instrs[i]!=iv->instrdata->instrs[i])
	break;
	if ( i==icnt ) {		/* Unchanged */
	    free(instrs);
return( true );
	}
    }
    free( iv->instrdata->instrs );
    iv->instrdata->instrs = instrs;
    iv->instrdata->instr_cnt = icnt;
    iv->instrdata->max = icnt;
    iv->instrdata->changed = true;
    free(iv->instrdata->bts );
    iv->instrdata->bts = NULL;
    instr_info_init(&iv->instrinfo);
    GScrollBarSetBounds(iv->instrinfo.vsb,0,iv->instrinfo.lheight+2,
	    iv->instrinfo.vheight<iv->instrinfo.fh? 1 : iv->instrinfo.vheight/iv->instrinfo.fh);
return( true );
}

static void IVOk(InstrDlg *iv) {
    struct instrdata *id = iv->instrdata;

    /* We need to update bits like instructions_out_of_date even if they */
    /* make no change. */
    if ( /*id->changed*/true ) {
	if ( id->sc!=NULL ) {
	    SplineChar *sc = id->sc;
	    CharView *cv;
	    free(sc->ttf_instrs);
	    sc->ttf_instrs_len = id->instr_cnt;
	    if ( id->instr_cnt==0 )
		sc->ttf_instrs = NULL;
	    else {
		sc->ttf_instrs = malloc( id->instr_cnt );
		memcpy(sc->ttf_instrs,id->instrs,id->instr_cnt );
	    }
	    for ( cv=(CharView *) (sc->views); cv!=NULL; cv=(CharView *) (cv->b.next) )
		cv->showpointnumbers = false;
	    sc->instructions_out_of_date = true;
	    SCCharChangedUpdate(sc,ly_none);
	    sc->instructions_out_of_date = false;
	    FVRefreshAll(sc->parent);
	} else {
	    struct ttf_table *tab, *prev;
	    if ( id->instr_cnt==0 ) {
		for ( prev=NULL, tab=id->sf->ttf_tables; tab!=NULL && tab->tag!=id->tag; prev=tab, tab=tab->next );
		if ( tab==NULL )
		    /* Nothing to be done */;
		else if ( prev==NULL )
		    id->sf->ttf_tables = tab->next;
		else
		    prev->next = tab->next;
		if ( tab!=NULL ) {
		    tab->next = NULL;
		    TtfTablesFree(tab);
		}
	    } else {
		tab = SFFindTable(id->sf,id->tag);
		if ( tab==NULL ) {
		    tab = chunkalloc(sizeof(struct ttf_table));
		    tab->next = id->sf->ttf_tables;
		    id->sf->ttf_tables = tab;
		    tab->tag = id->tag;
		}
		free( tab->data );
		tab->data = malloc( id->instr_cnt );
		memcpy(tab->data,id->instrs,id->instr_cnt );
		tab->len = id->instr_cnt;
	    }
	}
    }
    /* Instructions get freed in et_destroy */
}

static void IVBuildEdit(InstrDlg *iv) {
    char *ret;
    ret = __IVUnParseInstrs((InstrBase *) iv);
    if ( iv->text!=NULL ) {
	GGadgetSetTitle8(iv->text,ret);
	GTextFieldSelect(iv->text,iv->instrinfo.offset-ret,iv->instrinfo.offset-ret);
	GTextFieldShow(iv->text,iv->instrinfo.scroll-ret);
    }
}

static void instr_expose(struct instrinfo *ii,GWindow pixmap,GRect *rect) {
    int low, high;
    int i,x,y;
    char loc[8], ins[8], val[8]; unichar_t uins[8], uname[30];
    int addr_end, num_end;
    static unichar_t nums[] = { '0', '0', '0', '0', '0', '0', '\0' };
    int indent;
    extern GBox _ggadget_Default_Box;

    GDrawSetFont(pixmap,ii->gfont);
    GDrawSetLineWidth(pixmap,0);
    addr_end = 0;
    if ( ii->showaddr )
	addr_end = GDrawGetTextWidth(pixmap,nums,4)+EDGE_SPACING;
    num_end = addr_end;
    if ( ii->showhex )
	num_end = addr_end + GDrawGetTextWidth(pixmap,nums,5)+4;
    else if ( addr_end<36+2*EDGE_SPACING )
	num_end = addr_end = 36+2*EDGE_SPACING;

    low = ( (rect->y-EDGE_SPACING)/ii->fh ) * ii->fh +EDGE_SPACING;
    high = ( (rect->y+rect->height+ii->fh-1-EDGE_SPACING)/ii->fh ) * ii->fh +EDGE_SPACING;

    if ( ii->isel_pos!=-1 ) {
	GRect r;
	r.x = 0; r.width = ii->vwidth;
	r.y = (ii->isel_pos-ii->lpos)*ii->fh+EDGE_SPACING; r.height = ii->fh;
	GDrawFillRect(pixmap,&r,ACTIVE_BORDER);
    }

    if ( ii->showaddr )
	GDrawDrawLine(pixmap,addr_end,rect->y,addr_end,rect->y+rect->height,0x000000);
    if ( ii->showhex )
	GDrawDrawLine(pixmap,num_end,rect->y,num_end,rect->y+rect->height,0x000000);

    indent = 0;
    for ( i=0, y=EDGE_SPACING-ii->lpos*ii->fh; y<low && i<ii->instrdata->instr_cnt; ++i ) {
	if ( ii->instrdata->bts[i]==bt_instr ) {
	    int instr = ii->instrdata->instrs[i];
	    if ( instr == ttf_if || instr==ttf_idef || instr == ttf_fdef )
		++indent;
	    else if ( instr == ttf_eif || instr==ttf_endf )
		--indent;
	} else if ( ii->instrdata->bts[i]==bt_wordhi )
	    ++i;
	y += ii->fh;
    }
    if ( y<=high && ii->instrdata->instr_cnt==0 && i==0 ) {
	if ( ii->instrdata->in_composit ) {
	    GDrawDrawText8(pixmap,num_end+EDGE_SPACING,y+ii->as,_("<instrs inherited>"),-1,0xff0000);
	    y += ii->fh;
	}
	GDrawDrawText8(pixmap,num_end+EDGE_SPACING,y+ii->as,_("<no instrs>"),-1,0xff0000);
    } else {
	int temp_indent;
	for ( ; y<=high && i<ii->instrdata->instr_cnt+1; ++i ) {
	    temp_indent = indent;
	    sprintf( loc, "%d", i );
	    if ( ii->instrdata->bts[i]==bt_wordhi ) {
		sprintf( ins, " %02x%02x", ii->instrdata->instrs[i], ii->instrdata->instrs[i+1]); uc_strcpy(uins,ins);
		sprintf( val, " %d", (short) ((ii->instrdata->instrs[i]<<8) | ii->instrdata->instrs[i+1]) );
		uc_strcpy(uname,val);
		++i;
	    } else if ( ii->instrdata->bts[i]==bt_cnt || ii->instrdata->bts[i]==bt_byte ) {
		sprintf( ins, " %02x", ii->instrdata->instrs[i] ); uc_strcpy(uins,ins);
		sprintf( val, " %d", ii->instrdata->instrs[i]);
		uc_strcpy(uname,val);
	    } else if ( ii->instrdata->bts[i]==bt_impliedreturn ) {
		uc_strcpy(uname,_("<return>"));
		uins[0] = '\0';
	    } else {
		int instr = ii->instrdata->instrs[i];
		if ( instr == ttf_eif || instr==ttf_endf )
		    --indent;
		temp_indent = indent;
		if ( instr == ttf_else )
		    --temp_indent;
		sprintf( ins, "%02x", instr ); uc_strcpy(uins,ins);
		uc_strcpy(uname, ff_ttf_instrnames[instr]);
		if ( instr == ttf_if || instr==ttf_idef || instr == ttf_fdef )
		    ++indent;
	    }

	    if ( ii->showaddr ) {
		GRect size;
		GDrawLayoutInit(pixmap,loc,-1,NULL);
		GDrawLayoutExtents(pixmap,&size);
		x = addr_end - EDGE_SPACING - size.width;
		GDrawLayoutDraw(pixmap,x,y+ii->as,MAIN_FOREGROUND);
		if ( ii->bpcheck && ii->bpcheck(ii,i))
		    GDrawDrawImage(pixmap,&GIcon_Stop,NULL,EDGE_SPACING,
			    y+(ii->fh-8)/2-5);
	    }
	    x = addr_end + EDGE_SPACING;
	    if ( ii->showhex )
		GDrawDrawText(pixmap,x,y+ii->as,uins,-1,MAIN_FOREGROUND);
	    GDrawDrawText(pixmap,num_end+EDGE_SPACING+temp_indent*4,y+ii->as,uname,-1,MAIN_FOREGROUND);
	    y += ii->fh;
	}
	if ( ii->showaddr && ii->lstopped!=-1 ) {
	    GDrawDrawImage(pixmap,&GIcon_Stopped,NULL,EDGE_SPACING,
		    (ii->lstopped-ii->lpos)*ii->fh+(ii->fh-8)/2);
	}
    }
}

static void instr_mousedown(struct instrinfo *ii,int pos) {
    int i,l;

    pos = (pos-2)/ii->fh + ii->lpos;
    if ( pos>=ii->lheight )
	pos = -1;

    for ( i=l=0; l<pos && i<ii->instrdata->instr_cnt; ++i, ++l ) {
	if ( ii->instrdata->bts[i]==bt_wordhi )
	    ++i;
    }

    ii->isel_pos=pos;
    if ( ii->selection_callback!=NULL )
	(ii->selection_callback)(ii,i);
    GDrawRequestExpose(ii->v,NULL,false);
}

static void instr_mousemove(struct instrinfo *ii,int pos) {
    int i,y;
    char *msg;

    if ( ii->mousedown ) {
	instr_mousedown(ii,pos);
return;
    }
    if ( ii->instrdata->bts==NULL )
return;

    pos = ((pos-2)/ii->fh) * ii->fh + 2;

    for ( i=0, y=2-ii->lpos*ii->fh; y<pos && i<ii->instrdata->instr_cnt; ++i ) {
	if ( ii->instrdata->bts[i]==bt_wordhi )
	    ++i;
	y += ii->fh;
    }
    switch ( ii->instrdata->bts[i] ) {
      case bt_wordhi: case bt_wordlo:
	msg = _("A short to be pushed on the stack");
      break;
      case bt_cnt:
	msg = _("A count specifying how many bytes/shorts\nshould be pushed on the stack");
      break;
      case bt_byte:
	msg = _("An unsigned byte to be pushed on the stack");
      break;
      case bt_instr:
	msg = _(instrhelppopup[ii->instrdata->instrs[i]]);
	if ( msg==NULL ) msg = "???";
      break;
      default:
	msg = "???";
      break;
    }
    GGadgetPreparePopup8(GDrawGetParentWindow(ii->v),msg);
}

void instr_scroll(struct instrinfo *ii,struct sbevent *sb) {
    int newpos = ii->lpos;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= ii->vheight/ii->fh;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
        newpos += ii->vheight/ii->fh;
      break;
      case et_sb_bottom:
        newpos = ii->lheight-ii->vheight/ii->fh;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos>ii->lheight+1-ii->vheight/ii->fh )
        newpos = ii->lheight+1-ii->vheight/ii->fh;
    if ( newpos<0 ) newpos =0;
    if ( newpos!=ii->lpos ) {
	GRect r;
	int diff = newpos-ii->lpos;
	ii->lpos = newpos;
	GScrollBarSetPos(ii->vsb,ii->lpos);
	r.x=0; r.y = EDGE_SPACING; r.width=ii->vwidth; r.height=ii->vheight-2*EDGE_SPACING;
	GDrawScroll(ii->v,&r,0,diff*ii->fh);
    }
}

static int IIChar(struct instrinfo *ii,GEvent *event) {
    int pos = ii->isel_pos;

    if ( ii->handle_char )
return( (ii->handle_char)(ii,event));

    if ( event->u.chr.keysym == GK_Up || event->u.chr.keysym == GK_KP_Up )
	--pos;
    else if ( event->u.chr.keysym == GK_Down || event->u.chr.keysym == GK_KP_Down )
	++pos;
    else if ( event->u.chr.keysym == GK_Home || event->u.chr.keysym == GK_KP_Home ||
	    event->u.chr.keysym == GK_Begin || event->u.chr.keysym == GK_KP_Begin )
	pos = 0;
    else if ( event->u.chr.keysym == GK_End || event->u.chr.keysym == GK_KP_End ) {
	pos = ii->lheight-1;
    } else
return( false );
    if ( pos==-2 ) pos = -1;
    if ( pos!=ii->isel_pos ) {
	ii->isel_pos = pos;
	if ( pos!=-1 && (pos<ii->lpos || pos>=ii->lpos+ii->vheight/ii->fh )) {
	    ii->lpos = pos-(ii->vheight/(3*ii->fh));
	    if ( ii->lpos>=ii->lheight-ii->vheight/ii->fh )
		ii->lpos = ii->lheight-ii->vheight/ii->fh-1;
	    if ( ii->lpos<0 ) ii->lpos = 0;
	    GScrollBarSetPos(ii->vsb,ii->lpos);
	}
    }
    if ( ii->selection_callback!=NULL ) {
	int i,l;
	for ( i=l=0; l<pos && i<ii->instrdata->instr_cnt; ++i, ++l ) {
	    if ( ii->instrdata->bts[i]==bt_wordhi )
		++i;
	}
	(ii->selection_callback)(ii,i);
    }
    GDrawRequestExpose(ii->v,NULL,false);
return( true );
}

int ii_v_e_h(GWindow gw, GEvent *event) {
    struct instrinfo *ii = (struct instrinfo *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_expose:
	instr_expose(ii,gw,&event->u.expose.rect);
      break;
      case et_char:
	if ( IIChar(ii,event)) {
	    /* All Done */
	    ;
	} else if ( event->u.chr.keysym == GK_Help || event->u.chr.keysym == GK_F1 ) {
	    help("ttfinstrs.html");
	}
      break;
      case et_mousemove: case et_mousedown: case et_mouseup:
	GGadgetEndPopup();
	if ( event->type==et_mousemove ) {
	    instr_mousemove(ii,event->u.mouse.y);
	} else if ( event->type==et_mousedown ) {
	    instr_mousedown(ii,event->u.mouse.y);
	    if ( event->u.mouse.clicks==2 ) {
		/*InstrModCreate(ii)*/
		;
            }
	} else {
	    instr_mousemove(ii,event->u.mouse.y);
	    ii->mousedown = false;
	}
      break;
      case et_timer:
      break;
      case et_focus:
      break;
    }
return( true );
}

static int iv_e_h(GWindow gw, GEvent *event) {
    InstrDlg *iv = (InstrDlg *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_expose:
      break;
      case et_resize:
	instr_resize(iv,event);
      break;
      case et_char:
	if ( event->u.chr.keysym == GK_Help || event->u.chr.keysym == GK_F1 )
	    help("ttfinstrs.html");
      break;
      case et_controlevent:
	switch ( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    instr_scroll(&iv->instrinfo,&event->u.control.u.sb);
	  break;
	  case et_buttonactivate:
	    if ( event->u.control.g==iv->ok || event->u.control.g==iv->cancel ) {
		if ( event->u.control.g == iv->ok ) {
		    if ( iv->inedit )
			if ( !IVParse(iv))
      break;
		    IVOk(iv);
		}
		GDrawDestroyWindow(iv->gw);
	    } else if ( event->u.control.g==iv->edit || event->u.control.g==iv->parse ) {
		int toedit = event->u.control.g==iv->edit;
		GRect size;
		if ( toedit ) {
		    IVBuildEdit(iv);
		    GGadgetGetSize(iv->instrinfo.vsb,&size);
		    size.width = -1;
		    GGadgetSetDesiredSize(iv->text,&size,NULL);
		} else if ( !IVParse(iv))
      break;
		GGadgetSetVisible(iv->parse,toedit);
		/*GGadgetSetVisible(iv->text,toedit);*/
		GGadgetSetVisible(iv->edit,!toedit);
		GGadgetSetVisible(iv->instrinfo.vsb,!toedit);
		GDrawSetVisible(iv->instrinfo.v,!toedit);
		GHVBoxFitWindow(iv->topbox);
		iv->inedit = toedit;
	    }
	  break;
	}
      break;
      case et_close:
	GDrawDestroyWindow(iv->gw);
      break;
      case et_destroy: {
        SplineFont *sf = iv->instrdata->sf;
	struct instrdata *id, *prev;
	for ( prev = NULL, id=sf->instr_dlgs; id!=iv->instrdata && id!=NULL; prev=id, id=id->next );
	if ( prev==NULL )
	    sf->instr_dlgs = iv->instrdata->next;
	else
	    prev->next = iv->instrdata->next;
	free(iv->instrdata->instrs);
	free(iv->instrdata->bts);
	free(iv->instrdata);
	free(iv);
      } break;
    }
return( true );
}

static void InstrDlgCreate(struct instrdata *id,char *title) {
    InstrDlg *iv = calloc(1,sizeof(*iv));
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    FontRequest rq;
    int as,ds,ld, lh;
    GGadgetCreateData gcd[11], *butarray[9], *harray[3], *varray[8];
    GTextInfo label[6];
    static GFont *font=NULL;

    instrhelpsetup();

    id->next = id->sf->instr_dlgs;
    id->sf->instr_dlgs = id;
    id->id = iv;

    iv->instrdata = id;
    iv->instrinfo.instrdata = id;
    iv->instrinfo.showhex = iv->instrinfo.showaddr = true;
    iv->instrinfo.lstopped = -1;
    instr_info_init(&iv->instrinfo);

    if ( ttf_icon==NULL )
	ttf_icon = GDrawCreateBitmap(NULL,ttf_width,ttf_height,ttf_bits);

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_icon;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = title;
    wattrs.icon = ttf_icon;
    pos.x = pos.y = 0;
    if ( GIntGetResource(_NUM_Buttonsize)>65 )
	pos.width = GDrawPointsToPixels(NULL,3*GIntGetResource(_NUM_Buttonsize)+40);
    else
	pos.width = GDrawPointsToPixels(NULL,250);
    iv->oc_height = GDrawPointsToPixels(NULL,37);
    pos.height = GDrawPointsToPixels(NULL,100) + iv->oc_height;
    iv->gw = gw = GDrawCreateTopWindow(NULL,&pos,iv_e_h,iv,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 105;
    gcd[0].gd.pos.width = -1;
    label[0].text = (unichar_t *) _("_OK");
    label[0].text_is_1byte = true;
    label[0].text_in_resource = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.flags = gg_visible|gg_enabled|gg_but_default;
    gcd[0].creator = GButtonCreate;
    gcd[0].data = iv;

    gcd[1].gd.pos.x = -8; gcd[1].gd.pos.y = 3;
    gcd[1].gd.pos.width = -1;
    label[1].text = (unichar_t *) _("_Cancel");
    label[1].text_is_1byte = true;
    label[1].text_in_resource = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.flags = gg_visible|gg_enabled|gg_but_cancel;
    gcd[1].creator = GButtonCreate;
    gcd[1].data = iv;

    gcd[2] = gcd[1];
    label[2].text = (unichar_t *) _("_Edit");
    label[2].text_is_1byte = true;
    label[2].text_in_resource = true;
    gcd[2].gd.flags = gg_visible|gg_enabled;
    gcd[2].gd.label = &label[2];
    gcd[2].creator = GButtonCreate;
    gcd[2].data = iv;

    gcd[3] = gcd[1];
    label[3] = label[2];
    label[3].text = (unichar_t *) _("_Parse");
    gcd[3].gd.flags = gg_enabled;
    gcd[3].gd.label = &label[3];
    gcd[3].creator = GButtonCreate;
    gcd[3].data = iv;

    butarray[0] = GCD_Glue; butarray[1] = &gcd[0];
    butarray[2] = GCD_Glue; butarray[3] = &gcd[2];
    butarray[4] = &gcd[3]; butarray[5] = GCD_Glue;
    butarray[6] = &gcd[1]; butarray[7] = GCD_Glue;
    butarray[8] = NULL;
    gcd[4].gd.flags = gg_enabled|gg_visible;
    gcd[4].gd.u.boxelements = butarray;
    gcd[4].creator = GHBoxCreate;

    gcd[5].gd.pos.x = 0; gcd[5].gd.pos.y = 0;
    gcd[5].gd.pos.width = pos.width; gcd[5].gd.pos.height = pos.height-GDrawPointsToPixels(NULL,40);
    gcd[5].gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_textarea_wrap|gg_pos_use0;
    gcd[5].creator = GTextAreaCreate;
    gcd[5].data = iv;
    harray[0] = &gcd[5];

    gcd[6].gd.pos.y = 0; gcd[6].gd.pos.height = pos.height-GDrawPointsToPixels(NULL,40);
    gcd[6].gd.pos.width = GDrawPointsToPixels(gw,_GScrollBar_Width);
    gcd[6].gd.pos.x = pos.width-gcd[6].gd.pos.width;
    gcd[6].gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_sb_vert;
    gcd[6].creator = GScrollBarCreate;
    gcd[6].data = iv;
    harray[1] = &gcd[6]; harray[2] = NULL;

    gcd[7].gd.flags = gg_enabled|gg_visible;
    gcd[7].gd.u.boxelements = harray;
    gcd[7].creator = GHBoxCreate;

    varray[0] = &gcd[7]; varray[1] = NULL;
    varray[2] = &gcd[8]; varray[3] = NULL;
    varray[4] = &gcd[4]; varray[5] = NULL;
    varray[6] = NULL;

    gcd[8].gd.flags = gg_enabled|gg_visible;
    gcd[8].gd.pos.width = 100;
    gcd[8].creator = GLineCreate;

    gcd[9].gd.pos.x = gcd[9].gd.pos.y = 2;
    gcd[9].gd.flags = gg_enabled|gg_visible;
    gcd[9].gd.u.boxelements = varray;
    gcd[9].creator = GHVGroupCreate;

    GGadgetsCreate(gw,&gcd[9]);
    GHVBoxSetExpandableRow(gcd[9].ret,0);
    GHVBoxSetExpandableCol(gcd[7].ret,0);
    GHVBoxSetExpandableCol(gcd[4].ret,gb_expandgluesame);

    iv->ok = gcd[0].ret;
    iv->cancel = gcd[1].ret;
    iv->edit = gcd[2].ret;
    iv->parse = gcd[3].ret;
    iv->text = gcd[5].ret;
    iv->instrinfo.vsb = gcd[6].ret;
    iv->topbox = gcd[9].ret;

    wattrs.mask = wam_events|wam_cursor;
    pos = gcd[5].gd.pos;
    iv->instrinfo.v = GWidgetCreateSubWindow(gw,&pos,ii_v_e_h,&iv->instrinfo,&wattrs);
    GDrawSetVisible(iv->instrinfo.v,true);

    if ( font==NULL ) {
	memset(&rq,0,sizeof(rq));
	rq.utf8_family_name = MONO_UI_FAMILIES;
	rq.point_size = -12;
	rq.weight = 400;
	font = GDrawInstanciateFont(gw,&rq);
	font = GResourceFindFont("TTInstruction.Font",font);
    }
    iv->instrinfo.gfont = font;
    GDrawSetFont(iv->instrinfo.v,iv->instrinfo.gfont);
    GGadgetSetFont(iv->text,iv->instrinfo.gfont);
    GDrawWindowFontMetrics(iv->instrinfo.v,iv->instrinfo.gfont,&as,&ds,&ld);
    iv->instrinfo.as = as+1;
    iv->instrinfo.fh = iv->instrinfo.as+ds;
    iv->instrinfo.isel_pos = -1;

    lh = iv->instrinfo.lheight;
    if ( lh>40 ) lh = 40;
    if ( lh<4 ) lh = 4;
    GDrawResize(iv->gw,pos.width+gcd[6].gd.pos.width,iv->oc_height+lh*iv->instrinfo.fh+4);

    GDrawSetVisible(gw,true);
}

void SCEditInstructions(SplineChar *sc) {
    struct instrdata *id;
    char title[100];
    CharView *cv;
    RefChar *ref;

    /* In a multiple master font, the instructions for all glyphs reside in */
    /*  the "normal" instance of the font. The instructions are the same for */
    /*  all instances (the cvt table might be different) */
    if ( sc->parent->mm!=NULL && sc->parent->mm->apple )
	sc = sc->parent->mm->normal->glyphs[sc->orig_pos];

    for ( id = sc->parent->instr_dlgs; id!=NULL && id->sc!=sc; id=id->next );
    if ( id!=NULL ) {
	GDrawSetVisible(id->id->gw,true);
	GDrawRaise(id->id->gw);
return;
    }

    if ( sc->layers[ly_fore].refs!=NULL && sc->layers[ly_fore].splines!=NULL ) {
	ff_post_error(_("Can't instruct this glyph"),
		_("TrueType does not support mixed references and contours.\nIf you want instructions for %.30s you should either:\n * Unlink the reference(s)\n * Copy the inline contours into their own (unencoded\n    glyph) and make a reference to that."),
		sc->name );
return;
    }
    for ( ref = sc->layers[ly_fore].refs; ref!=NULL; ref=ref->next ) {
	if ( ref->transform[0]>=2 || ref->transform[0]<-2 ||
		ref->transform[1]>=2 || ref->transform[1]<-2 ||
		ref->transform[2]>=2 || ref->transform[2]<-2 ||
		ref->transform[3]>=2 || ref->transform[3]<-2 )
    break;
    }
    if ( ref!=NULL ) {
	ff_post_error(_("Can't instruct this glyph"),
		_("TrueType does not support references which\nare scaled by more than 200%%.  But %1$.30s\nhas been in %2$.30s. Any instructions\nadded would be meaningless."),
		ref->sc->name, sc->name );
return;
    }

    for ( cv=(CharView *) (sc->views); cv!=NULL; cv=(CharView *) (cv->b.next) ) {
	sc = cv->b.sc;
	cv->showpointnumbers = true;
	SCNumberPoints(sc,CVLayer((CharViewBase *) cv));
	GDrawRequestExpose(cv->v,NULL,false);
    }
    id = calloc(1,sizeof(*id));
    id->instr_cnt = id->max = sc->ttf_instrs_len;
    id->sf = sc->parent;
    id->sc = sc;
    id->instrs = malloc(id->max+1);
    if ( sc->ttf_instrs!=NULL )
	memcpy(id->instrs,sc->ttf_instrs,id->instr_cnt);
    sprintf(title,_("TrueType Instructions for %.50s"),sc->name);
    InstrDlgCreate(id,title);
}

void SC_MarkInstrDlgAsChanged(SplineChar *sc) {
    struct instrdata *id;

    for ( id = sc->parent->instr_dlgs; id!=NULL && id->sc!=sc; id=id->next );
    if ( id!=NULL )
	id->changed = true;
}

void IIScrollTo(struct instrinfo *ii,int ip,int mark_stop) {
    int l, i;

    for ( i=l=0; i<ip && i<ii->instrdata->instr_cnt; ++i, ++l ) {
	if ( ii->instrdata->bts[i]==bt_wordhi || ii->instrdata->bts[i]==bt_wordlo )
	    ++i;
    }
    if ( ip==-1 )
	ii->lstopped = -1;
    else {
	if ( mark_stop )
	    ii->lstopped = l;
	if ( l<ii->lpos || l>=ii->lpos+ii->vheight/ii->fh-1 ) {
	    if ( l+ii->vheight/ii->fh-1 >= ii->lheight+1 )
		l = ii->lheight+2-(ii->vheight/ii->fh);
	    if ( l<0 )
		l = 0;
	    ii->lpos = l;
	    GScrollBarSetPos(ii->vsb,l);
	}
    }
    GDrawRequestExpose(ii->v,NULL,false);
}

void IIReinit(struct instrinfo *ii,int ip) {
    instrhelpsetup();
    free(ii->instrdata->bts);
    ii->instrdata->bts = NULL;
    instr_info_init(ii);
    GScrollBarSetBounds(ii->vsb,0,ii->lheight+2, ii->vheight<ii->fh ? 1 : ii->vheight/ii->fh);
    IIScrollTo(ii,ip,true);
}

/* ************************************************************************** */
/* **************************** CVT table editor **************************** */
/* ************************************************************************** */

#define ADDR_SPACER	4
#define EDGE_SPACER	2

typedef struct shortview /* : tableview */ {
    struct ttf_table *table;
    GWindow gw, v;
    SplineFont *sf;
    unsigned int destroyed: 1;		/* window has been destroyed */
    unsigned int changed: 1;
    GGadget *vsb, *tf;
    GGadget *ok, *cancel, *setsize;
    int lpos, lheight;
    int16 as, fh;
    int16 vheight, vwidth;
    int16 sbw, bh;
    GFont *gfont;
    int16 chrlen, addrend, valend;
    int16 active;
    int16 which;
    int16 *edits;
    char **comments;
    uint8 *data;
    int32 len;
    uint32 tag;
} ShortView;

static int sfinishup(ShortView *sv,int showerr) {
    const unichar_t *ret = _GGadgetGetTitle(sv->tf);
    unichar_t *end;
    int val, oldval;

    if ( sv->active==-1 )
return( true );

    if ( sv->which ) {
	if ( *ret=='\0' ) {
	    if ( sv->comments[sv->active]!=NULL ) {
		free(sv->comments[sv->active]);
		sv->comments[sv->active] = NULL;
		sv->changed = true;
	    }
	} else {
	    char *new = GGadgetGetTitle8(sv->tf);
	    if ( sv->comments[sv->active]==NULL ) {
		sv->changed = true;
	    } else {
		if ( strcmp(sv->comments[sv->active],new)!=0 )
		    sv->changed = true;
		free(sv->comments[sv->active]);
	    }
	    sv->comments[sv->active] = new;
	}
    } else {
	val = u_strtol(ret,&end,10);
	if ( *ret=='\0' || *end!='\0' || val<-32768 || val>32767 ) {
	    if ( showerr )
		ff_post_error(_("Bad Number"),_("Bad Number"));
return( false );
	}
	oldval = sv->edits[sv->active];
	if ( val != oldval ) {
	    sv->changed = true;
	    sv->edits[sv->active] = val;
	}
    }
    sv->active = -1;
    GGadgetMove(sv->tf,sv->addrend,-100);
return( true );
}

static void SV_SetScrollBar(ShortView *sv) {
    int lh;
    sv->lheight = lh = sv->len/2;

    GScrollBarSetBounds(sv->vsb,0,lh,sv->vheight<sv->fh ? 1 : sv->vheight/sv->fh);
    if ( sv->lpos + sv->vheight/sv->fh > lh ) {
	int lpos = lh-sv->vheight/sv->fh;
	if ( lpos<0 ) lpos = 0;
	if ( sv->lpos!=lpos && sv->active!=-1 )
	    GGadgetMove(sv->tf,sv->addrend,(sv->active-lpos)*sv->fh);
	sv->lpos = lpos;
    }
    GScrollBarSetPos(sv->vsb,sv->lpos);
}

static int SV_ChangeLength(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	ShortView *sv = GDrawGetUserData(GGadgetGetWindow(g));
	char buffer[12];
	char *ret, *e;
	int val,i;

	sprintf( buffer, "%d", (int) (sv->len/2) );
	ret = gwwv_ask_string(_("Change Length"), buffer,_("How many entries should there be in the cvt table?"));
	if ( ret==NULL )
return( true );		/* Cancelled */
	val = strtol(ret,&e,10);
	if ( *e || val<0 || val>65535 ) {
	    free(ret);
	    ff_post_error(_("Bad Number"),_("Bad Number"));
return( false );
	}
	free(ret);
	if ( val*2>sv->len ) {
	    sv->edits = realloc(sv->edits,val*2);
	    for ( i=sv->len/2; i<val; ++i )
		sv->edits[i] = 0;
	    sv->comments = realloc(sv->comments,val*sizeof(char *));
	    for ( i=sv->len/2; i<val; ++i )
		sv->comments[i] = NULL;
	} else {
	    for ( i=val; i<sv->len/2; ++i ) {
		free(sv->comments[i]);
		sv->comments[i] = NULL;
	    }
	}
	sv->len = 2*val;
	SV_SetScrollBar(sv);
	GDrawRequestExpose(sv->v,NULL,true);
    }
return( true );
}

static void _SV_DoClose(ShortView *sv) {

    sv->destroyed = true;
    GDrawDestroyWindow(sv->gw);
}

static int SV_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	ShortView *sv = GDrawGetUserData(GGadgetGetWindow(g));

	_SV_DoClose(sv);
    }
return( true );
}

static int SV_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	ShortView *sv = GDrawGetUserData(GGadgetGetWindow(g));
	int i;
	struct ttf_table *prev, *tab;
	SplineFont *sf = sv->sf;

	if ( !sfinishup(sv,true) )
return( true );
	if ( sf->cvt_names!=NULL ) {
	    for ( i=0; sf->cvt_names[i]!=END_CVT_NAMES; ++i )
		free(sf->cvt_names[i]);
	    free(sf->cvt_names);
	    sf->cvt_names = NULL;
	}
	if ( sv->len==0 ) {
	    if ( sv->table!=NULL ) {
		prev = NULL;
		for ( tab=sf->ttf_tables; tab!=NULL && tab!=sv->table; prev=tab, tab=tab->next );
		if ( prev!=NULL )
		    prev->next = tab->next;
		else
		    sf->ttf_tables = tab->next;
		free(sv->table->data);
		chunkfree(sv->table,sizeof(struct ttf_table));
		sv->table = NULL;
	    }
	} else {
	    if ( sv->table!=NULL )
		free(sv->table->data);
	    else {
		tab = chunkalloc(sizeof(struct ttf_table));
		tab->next = sf->ttf_tables;
		sf->ttf_tables = tab;
		tab->tag = sv->tag;
		sv->table = tab;
	    }
	    sv->table->data = malloc(sv->len);
	    sf->cvt_names = malloc(((sv->len>>1)+1)*sizeof(char *));
	    for ( i=0; i<sv->len/2; ++i ) {
		sv->table->data[i<<1] = (sv->edits[i]>>8)&0xff;
		sv->table->data[(i<<1)+1] = sv->edits[i]&0xff;
		sf->cvt_names[i] = sv->comments[i];
		sv->comments[i] = NULL;
	    }
	    sf->cvt_names[i] = END_CVT_NAMES;
	    sv->table->len = sv->len;
	}
	sf->changed = true;
	_SV_DoClose(sv);
    }
return( true );
}

static void short_resize(ShortView *sv,GEvent *event) {
    GRect pos, gsize;
    int width;

    /* height must be a multiple of the line height */
    if ( (event->u.resize.size.height-2*EDGE_SPACER-sv->bh)%sv->fh!=0 ||
	    (event->u.resize.size.height-2*EDGE_SPACER-sv->fh-sv->bh)<0 ) {
	int lc = (event->u.resize.size.height+sv->fh/2-EDGE_SPACER)/sv->fh;
	if ( lc<=0 ) lc = 1;
	GDrawResize(sv->gw, event->u.resize.size.width,
		lc*sv->fh+2*EDGE_SPACER+sv->bh);
return;
    }

    pos.width = GDrawPointsToPixels(sv->gw,_GScrollBar_Width);
    pos.height = event->u.resize.size.height-sv->bh-sv->fh;
    pos.x = event->u.resize.size.width-pos.width; pos.y = sv->fh;
    GGadgetResize(sv->vsb,pos.width,pos.height+1);
    GGadgetMove(sv->vsb,pos.x,pos.y);
    pos.width = pos.x; pos.x = 0;
    GDrawResize(sv->v,pos.width,pos.height);
    GDrawMove(sv->v,0,sv->fh);

    sv->vheight = pos.height; sv->vwidth = pos.width;
    SV_SetScrollBar(sv);

    width = pos.width-sv->addrend;
    if ( width < 5 ) width = 5;
    GGadgetResize(sv->tf,width,sv->fh);

    GGadgetGetSize(sv->ok,&gsize);
    GGadgetMove(sv->ok,gsize.x,event->u.resize.size.height-GDrawPointsToPixels(sv->gw,33));
    GGadgetMove(sv->cancel,event->u.resize.size.width-gsize.x-gsize.width,event->u.resize.size.height-GDrawPointsToPixels(sv->gw,30));
    GGadgetGetSize(sv->setsize,&gsize);
    GGadgetMove(sv->setsize,(event->u.resize.size.width-gsize.width)/2,
	    event->u.resize.size.height-GDrawPointsToPixels(sv->gw,60));
    
    GDrawRequestExpose(sv->gw,NULL,true);
}

static void short_expose(ShortView *sv,GWindow pixmap,GRect *rect) {
    int low, high;
    int x,y;
    char cval[8], caddr[8];
    int index;

    GDrawSetFont(pixmap,sv->gfont);

    low = ( (rect->y-EDGE_SPACER)/sv->fh ) * sv->fh + EDGE_SPACER;
    high = ( (rect->y+rect->height+sv->fh-1-EDGE_SPACER)/sv->fh ) * sv->fh +EDGE_SPACER;
    if ( high>sv->vheight-EDGE_SPACER ) high = sv->vheight-EDGE_SPACER;

    GDrawDrawLine(pixmap,sv->addrend-ADDR_SPACER/2,rect->y,sv->addrend-ADDR_SPACER/2,rect->y+rect->height,0x000000);
    GDrawDrawLine(pixmap,sv->valend-ADDR_SPACER/2,rect->y,sv->valend-ADDR_SPACER/2,rect->y+rect->height,0x000000);

    index = (sv->lpos+(low-EDGE_SPACER)/sv->fh);
    y = low;
    for ( ; y<=high && index<sv->len/2; ++index ) {
	sprintf( caddr, "%d", index );
	x = sv->addrend - ADDR_SPACER - GDrawGetText8Width(pixmap,caddr,-1);
	GDrawDrawText8(pixmap,x,y+sv->as,caddr,-1,MAIN_FOREGROUND);

	sprintf( cval, "%d", sv->edits[index] );
	GDrawDrawText8(pixmap,sv->addrend,y+sv->as,cval,-1,MAIN_FOREGROUND);

	if ( sv->comments[index]!=NULL )
	    GDrawDrawText8(pixmap,sv->valend,y+sv->as,sv->comments[index],-1,MAIN_FOREGROUND);
	y += sv->fh;
    }
}

static void short_mousemove(ShortView *sv,int pos) {
    /*GGadgetPreparePopup(sv->gw,msg);*/
}

static void short_scroll(ShortView *sv,struct sbevent *sb) {
    int newpos = sv->lpos;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= sv->vheight/sv->fh;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
        newpos += sv->vheight/sv->fh;
      break;
      case et_sb_bottom:
        newpos = sv->lheight-sv->vheight/sv->fh;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos>sv->lheight-sv->vheight/sv->fh )
        newpos = sv->lheight-sv->vheight/sv->fh;
    if ( newpos<0 ) newpos =0;
    if ( newpos!=sv->lpos ) {
	int diff = newpos-sv->lpos;
	sv->lpos = newpos;
	GScrollBarSetPos(sv->vsb,sv->lpos);
	if ( sv->active!=-1 ) {
	    GRect pos;
	    GGadgetGetSize(sv->tf,&pos);
	    GGadgetMove(sv->tf,sv->addrend,pos.y+diff*sv->fh);
	}
	GDrawScroll(sv->v,NULL,0,diff*sv->fh);
    }
}

static void ShortViewFree(ShortView *sv) {
    sv->sf->cvt_dlg = NULL;
    free(sv->edits);
    free(sv);
}

static int sv_v_e_h(GWindow gw, GEvent *event) {
    ShortView *sv = (ShortView *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_expose:
	short_expose(sv,gw,&event->u.expose.rect);
      break;
      case et_char:
	if ( event->u.chr.keysym == GK_Help || event->u.chr.keysym == GK_F1 )
	    help("ttfinstrs.html#cvt");
      break;
      case et_mousemove: case et_mousedown: case et_mouseup:
	GGadgetEndPopup();
	if ( event->type==et_mousemove )
	    short_mousemove(sv,event->u.mouse.y);
	else if ( event->type == et_mousedown ) {
	    int l = (event->u.mouse.y-EDGE_SPACER)/sv->fh + sv->lpos;
	    int which = event->u.mouse.x > sv->valend;
	    char buf[20];
	    int old = sv->active;
	    if ( sfinishup(sv,true) && event->u.mouse.x>sv->addrend &&
		    l<sv->len/2 && l!=old ) {
		sv->active = l;
		sv->which = which;
		if ( !which ) {
		    /* Change the value */
		    GGadgetResize(sv->tf,sv->valend-sv->addrend-EDGE_SPACER,sv->fh);
		    GGadgetMove(sv->tf, sv->addrend,
					    (l-sv->lpos)*sv->fh+EDGE_SPACER+1);
		    sprintf( buf, "%d", sv->edits[sv->active] );
		    GGadgetSetTitle8(sv->tf,buf);
		} else {
		    GGadgetResize(sv->tf,sv->vwidth-sv->valend-EDGE_SPACER,sv->fh);
		    GGadgetMove(sv->tf, sv->valend,
					    (l-sv->lpos)*sv->fh+EDGE_SPACER+1);
		    GGadgetSetTitle8(sv->tf,sv->comments[l]==NULL?"":sv->comments[l]);
		}
		GDrawRequestExpose(sv->v,NULL,true);
		GDrawPostEvent(event);	/* And we hope the tf catches it this time */
	    }
	}
      break;
      case et_resize:
	GDrawRequestExpose(gw,NULL,true);
      break;
      case et_timer:
      break;
      case et_focus:
      break;
    }
return( true );
}

static int sv_e_h(GWindow gw, GEvent *event) {
    ShortView *sv = (ShortView *) GDrawGetUserData(gw);
    GRect r;
    int x;

    switch ( event->type ) {
      case et_expose:
	r.x = r.y = 0; r.width = sv->vwidth+40; r.height = sv->fh-1;
	GDrawFillRect(gw,&r,0x808080);
	GDrawSetFont(gw,sv->gfont);
	x = sv->addrend - ADDR_SPACER - 2 - GDrawGetText8Width(gw,_("Index"),-1);
	GDrawDrawText8(gw,x,sv->as,_("Index"),-1,0xffffff);
	GDrawDrawText8(gw,sv->addrend,sv->as,_("Value"),-1,0xffffff);
	GDrawDrawText8(gw,sv->valend,sv->as,_("Comment"),-1,0xffffff);
	
	GDrawDrawLine(gw,0,sv->fh-1,r.width,sv->fh-1,0x000000);
	GDrawDrawLine(gw,0,sv->vheight+sv->fh,sv->vwidth,sv->vheight+sv->fh,0x000000);
      break;
      case et_resize:
	short_resize(sv,event);
      break;
      case et_char:
	if ( event->u.chr.keysym == GK_Help || event->u.chr.keysym == GK_F1 )
	    help("ttfinstrs.html#cvt");
      break;
      case et_controlevent:
	switch ( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    short_scroll(sv,&event->u.control.u.sb);
	  break;
	}
      break;
      case et_close:
	_SV_DoClose(sv);
      break;
      case et_destroy:
	ShortViewFree(sv);
      break;
    }
return( true );
}

/* cvt table */
static void cvtCreateEditor(struct ttf_table *tab,SplineFont *sf,uint32 tag) {
    ShortView *sv = calloc(1,sizeof(ShortView));
    char title[60];
    GRect pos, subpos, gsize;
    GWindow gw;
    GWindowAttrs wattrs;
    FontRequest rq;
    int as,ds,ld, lh;
    GGadgetCreateData gcd[9], *butarray[8], *harray[4], *harray2[3], *varray[7];
    GTextInfo label[5], lab;
    GGadgetData gd;
    static unichar_t num[] = { '0',  '\0' };
    int numlen;
    static GBox tfbox;
    int i;
    static GFont *font = NULL;

    sv->table = tab;
    sv->sf = sf;
    sf->cvt_dlg = sv;
    sv->tag = tag;

    if ( tab==NULL && sf->mm!=NULL && sf->mm->apple )
	tab = SFFindTable(sf->mm->normal,tag);
    if ( tab!=NULL ) {
	sv->len = tab->len;
	sv->edits = malloc(tab->len+1);
	sv->comments = calloc((tab->len/2+1),sizeof(char *));
	for ( i=0; i<tab->len/2; ++i )
	    sv->edits[i] = (tab->data[i<<1]<<8) | tab->data[(i<<1)+1];
	if ( sf->cvt_names!=NULL )
	    for ( i=0; i<tab->len/2 && sf->cvt_names[i]!=END_CVT_NAMES; ++i )
		sv->comments[i] = copy(sf->cvt_names[i]);
    } else {
	sv->edits = malloc(2);
	sv->len = 0;
	sv->comments = calloc(1,sizeof(char *));
    }

    title[0] = (tag>>24)&0xff;
    title[1] = (tag>>16)&0xff;
    title[2] = (tag>>8 )&0xff;
    title[3] = (tag    )&0xff;
    title[4] = ' ';
    strncpy(title+5, sf->fontname, sizeof(title)/sizeof(title[0])-6);

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_icon;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = title;
    wattrs.icon = ttf_icon;
    pos.x = pos.y = 0;
    if ( GIntGetResource(_NUM_Buttonsize)>60 )
	pos.width = GDrawPointsToPixels(NULL,2*GIntGetResource(_NUM_Buttonsize)+30);
    else
	pos.width = GDrawPointsToPixels(NULL,150);
    pos.height = GDrawPointsToPixels(NULL,200);
    sv->gw = gw = GDrawCreateTopWindow(NULL,&pos,sv_e_h,sv,&wattrs);

    memset(&gcd,0,sizeof(gcd));
    memset(&label,0,sizeof(label));

    gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 105;
    gcd[0].gd.pos.width = -1;
    label[0].text = (unichar_t *) _("_OK");
    label[0].text_is_1byte = true;
    label[0].text_in_resource = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.flags = gg_visible|gg_enabled|gg_but_default;
    gcd[0].creator = GButtonCreate;
    gcd[0].data = sv;
    gcd[0].gd.handle_controlevent = SV_OK;

    gcd[1].gd.pos.x = -8; gcd[1].gd.pos.y = 3;
    gcd[1].gd.pos.width = -1;
    label[1].text = (unichar_t *) _("_Cancel");
    label[1].text_is_1byte = true;
    label[1].text_in_resource = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.flags = gg_visible|gg_enabled|gg_but_cancel;
    gcd[1].creator = GButtonCreate;
    gcd[1].data = sv;
    gcd[1].gd.handle_controlevent = SV_Cancel;

    gcd[2] = gcd[1];
    label[2].text = (unichar_t *) _("Change Length");
    label[2].text_is_1byte = true;
    label[2].text_in_resource = true;
    gcd[2].gd.flags = gg_visible|gg_enabled;
    gcd[2].gd.label = &label[2];
    gcd[2].creator = GButtonCreate;
    gcd[2].data = sv;
    gcd[2].gd.handle_controlevent = SV_ChangeLength;

    butarray[0] = GCD_Glue; butarray[1] = &gcd[0]; butarray[2] = GCD_Glue; 
    butarray[3] = GCD_Glue; butarray[4] = &gcd[1]; butarray[5] = GCD_Glue;
    butarray[6] = NULL;

    harray[0] = GCD_Glue; harray[1] = &gcd[2]; harray[2] = GCD_Glue;
    harray[3] = NULL;

    gcd[3].gd.flags = gg_enabled|gg_visible;
    gcd[3].gd.u.boxelements = butarray;
    gcd[3].creator = GHBoxCreate;

    gcd[4].gd.flags = gg_enabled|gg_visible;
    gcd[4].gd.u.boxelements = harray;
    gcd[4].creator = GHBoxCreate;

    sv->bh = GDrawPointsToPixels(gw,64);

    gcd[5].gd.pos.y = sv->fh; gcd[5].gd.pos.height = pos.height-sv->bh;
    gcd[5].gd.pos.width = GDrawPointsToPixels(gw,_GScrollBar_Width);
    gcd[5].gd.pos.x = pos.width-gcd[5].gd.pos.width;
    gcd[5].gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_sb_vert;
    gcd[5].gd.handle_controlevent = NULL;
    gcd[5].creator = GScrollBarCreate;
    gcd[5].data = sv;
    harray2[0] = GCD_Glue; harray2[1] = &gcd[5]; harray2[2] = NULL;

    gcd[6].gd.flags = gg_enabled|gg_visible;
    gcd[6].gd.u.boxelements = harray2;
    gcd[6].creator = GHBoxCreate;

    varray[0] = &gcd[6]; varray[1] = NULL;
    varray[2] = &gcd[4]; varray[3] = NULL;
    varray[4] = &gcd[3]; varray[5] = NULL;
    varray[6] = NULL;

    /*gcd[7].gd.pos.x = gcd[7].gd.pos.y = 2;*/
    gcd[7].gd.flags = gg_enabled|gg_visible;
    gcd[7].gd.u.boxelements = varray;
    gcd[7].creator = GHVBoxCreate;

    GGadgetsCreate(gw,&gcd[7]);
    GHVBoxSetExpandableRow(gcd[7].ret,0);
    GHVBoxSetExpandableCol(gcd[6].ret,0);
    GHVBoxSetExpandableCol(gcd[3].ret,gb_expandgluesame);
    GHVBoxSetExpandableCol(gcd[4].ret,gb_expandglue);

    sv->vsb = gcd[5].ret;
    sv->ok = gcd[0].ret;
    sv->cancel = gcd[1].ret;
    sv->setsize = gcd[2].ret;
    GGadgetGetSize(sv->vsb,&gsize);
    sv->sbw = gsize.width;

    wattrs.mask = wam_events|wam_cursor;
    subpos.x = 0; subpos.y = sv->fh;
    subpos.width = 100; subpos.height = pos.height - sv->bh - sv->fh;
    sv->v = GWidgetCreateSubWindow(gw,&subpos,sv_v_e_h,sv,&wattrs);
    GDrawSetVisible(sv->v,true);

    if ( font==NULL ) {
	memset(&rq,0,sizeof(rq));
	rq.utf8_family_name = MONO_UI_FAMILIES;
	rq.point_size = -12;
	rq.weight = 400;
	font = GDrawInstanciateFont(gw,&rq);
	font = GResourceFindFont("CVT.Font",font);
    }
    sv->gfont = font;
    GDrawSetFont(sv->v,sv->gfont);
    GDrawSetFont(sv->gw,sv->gfont);
    GDrawWindowFontMetrics(sv->gw,sv->gfont,&as,&ds,&ld);
    sv->as = as+1;
    sv->fh = sv->as+ds;

    sv->chrlen = numlen = GDrawGetTextWidth(sv->v,num,1);
    sv->addrend = 6*numlen + ADDR_SPACER + EDGE_SPACER;
    sv->valend = sv->addrend + 7*numlen + ADDR_SPACER + EDGE_SPACER;

    tfbox.main_background = tfbox.main_foreground = COLOR_DEFAULT;
    memset(&gd,0,sizeof(gd));
    gd.pos.y = -100; gd.pos.height = sv->fh;
    gd.pos.x = sv->addrend;
    memset(&lab,'\0',sizeof(lab));
    lab.text = num+1;
    lab.font = sv->gfont;
    gd.label = &lab;
    gd.box = &tfbox;
    gd.flags = gg_visible|gg_enabled|gg_sb_vert|gg_dontcopybox;
    sv->tf = GTextFieldCreate(sv->v,&gd,NULL);
    sv->active = -1;

    lh = sv->len/2;
    if ( lh>40 ) lh = 40;
    if ( lh<4 ) lh = 4;
    if ( pos.width<sv->valend+6*numlen+EDGE_SPACER+sv->sbw )
	pos.width = sv->valend+6*numlen+EDGE_SPACER+sv->sbw;
    GDrawResize(sv->gw,pos.width,lh*sv->fh+2*EDGE_SPACER);

    GDrawSetVisible(gw,true);
}

int SF_CloseAllInstrs(SplineFont *sf) {
    struct instrdata *id, *next;
    int changed;
    char name[12], *npt;
    static char *buts[3];
    static int done = false;

    if ( !done ) {
	buts[0] = _("_OK");
	buts[1] = _("_Cancel");
	done = true;
    }

    for ( id = sf->instr_dlgs; id!=NULL; id=next ) {
	next = id->next;
	changed = id->changed;
	if ( !changed && id->id->inedit ) {
	    if ( !IVParse(id->id))
		changed = true;
	    else
		changed = id->changed;
	}
	if ( changed ) {
	    if ( id->tag==0 )
		npt = id->sc->name;
	    else {
		name[0] = name[5] = '\'';
		name[1] = id->tag>>24; name[2] = (id->tag>>16)&0xff; name[3] = (id->tag>>8)&0xff; name[4] = id->tag&0xff;
		name[6] = 0;
		npt = name;
	    }
	    GDrawRaise(id->id->gw);
	    if ( gwwv_ask(_("Instructions were changed"),(const char **) buts,0,1,_("The instructions for %.80s have changed. Do you want to lose those changes?"),npt)==1 )
return( false );
	}
	GDrawDestroyWindow(id->id->gw);
    }
    if ( sf->cvt_dlg!=NULL ) {
	if ( sf->cvt_dlg->changed ) {
	    name[0] = name[5] = '\'';
	    name[1] = id->tag>>24; name[2] = (id->tag>>16)&0xff; name[3] = (id->tag>>8)&0xff; name[4] = id->tag&0xff;
	    name[6] = 0;
	    npt = name;
	    GDrawRaise(sf->cvt_dlg->gw);
	    if ( gwwv_ask(_("Instructions were changed"),(const char **) buts,0,1,_("The instructions for %.80s have changed. Do you want to lose those changes?"),npt)==1 )
return( false );
	}
	GDrawDestroyWindow(sf->cvt_dlg->gw);
    }
    if ( !no_windowing_ui ) {
	GDrawSync(NULL);
	GDrawProcessPendingEvents(NULL);
    }
return( true );
}

/* Maxp table editor (or that subset of it that ff can't figure out) */
struct maxp_data {
    GWindow gw;
    SplineFont *sf;
    struct ttf_table *tab;
    int done;
};

#define CID_Zones	1006
#define CID_TPoints	1007
#define CID_Storage	1008
#define CID_FDefs	1009
#define CID_IDefs	1010
#define CID_SEl		1011

static void MP_DoClose(struct maxp_data *mp) {
    mp->done = true;
}

static int Maxp_Cancel(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	MP_DoClose(GDrawGetUserData(GGadgetGetWindow(g)));
    }
return( true );
}

static int Maxp_OK(GGadget *g, GEvent *e) {
    struct maxp_data *mp;
    int zones, tp, store, stack, fd, id, err=0;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	mp = GDrawGetUserData(GGadgetGetWindow(g));
	zones = GetInt8(mp->gw,CID_Zones,_("Zones"),&err);
	tp = GetInt8(mp->gw,CID_TPoints,_("Twilight Zone Point Count"),&err);
	store = GetInt8(mp->gw,CID_Storage,_("Storage"),&err);
	stack = GetInt8(mp->gw,CID_SEl,_("Max Stack Depth"),&err);
	fd = GetInt8(mp->gw,CID_FDefs,_("Max # Functions"),&err);
	id = GetInt8(mp->gw,CID_IDefs,_("Max Instruction Defines"),&err);
	if ( err )
return( true );
	mp->done = true;
	if ( mp->tab==NULL ) {
	    mp->tab = chunkalloc(sizeof(struct ttf_table));
	    mp->tab->tag = CHR('m','a','x','p');
	    mp->tab->len = 32;
	    mp->tab->data = calloc(32,1);
	    mp->tab->next = mp->sf->ttf_tables;
	    mp->sf->ttf_tables = mp->tab;
	} else if ( mp->tab->len<32 ) {
	    free(mp->tab->data);
	    mp->tab->len = 32;
	    mp->tab->data = calloc(32,1);
	}
	mp->tab->data[14] = zones>>8; mp->tab->data[15] = zones&0xff;
	mp->tab->data[16] = tp>>8; mp->tab->data[17] = tp&0xff;
	mp->tab->data[18] = store>>8; mp->tab->data[19] = store&0xff;
	mp->tab->data[20] = fd>>8; mp->tab->data[21] = fd&0xff;
	mp->tab->data[22] = id>>8; mp->tab->data[23] = id&0xff;
	mp->tab->data[24] = stack>>8; mp->tab->data[25] = stack&0xff;
	mp->sf->changed = true;
	mp->done = true;
    }
return( true );
}

static int mp_e_h(GWindow gw, GEvent *event) {
    struct maxp_data *mp = (struct maxp_data *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_char:
	if ( event->u.chr.keysym == GK_Help || event->u.chr.keysym == GK_F1 )
	    help("ttfinstrs.html#maxp");
	else
return( false );
      break;
      case et_close:
	MP_DoClose(mp);
      break;
    }
return( true );
}

static void maxpCreateEditor(struct ttf_table *tab,SplineFont *sf,uint32 tag) {
    char title[60];
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    struct maxp_data mp;
    GGadgetCreateData gcd[17], boxes[4], *hvarray[16], *butarray[8], *varray[7];
    GTextInfo label[17];
    uint8 dummy[32], *data;
    char buffer[6][20];
    int k, hv;

    if ( tab==NULL && sf->mm!=NULL && sf->mm->apple ) {
	sf = sf->mm->normal;
	tab = SFFindTable(sf,tag);
    }
    memset(&mp,0,sizeof(mp));
    mp.sf = sf;
    mp.tab = tab;
    if ( tab==NULL || tab->len<32 ) {
	memset(dummy,0,sizeof(dummy));
	dummy[15]=2;	/* default Zones to 2 */
	data = dummy;
    } else
	data = tab->data;

    title[0] = (tag>>24)&0xff;
    title[1] = (tag>>16)&0xff;
    title[2] = (tag>>8 )&0xff;
    title[3] = (tag    )&0xff;
    title[4] = ' ';
    strncpy(title+5, sf->fontname, sizeof(title)-6);

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = title;
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = GDrawPointsToPixels(NULL,260);
    pos.height = GDrawPointsToPixels(NULL,125);
    mp.gw = gw = GDrawCreateTopWindow(NULL,&pos,mp_e_h,&mp,&wattrs);

    memset(label,0,sizeof(label));
    memset(gcd,0,sizeof(gcd));
    memset(boxes,0,sizeof(boxes));

	k=hv=0;
	label[k].text = (unichar_t *) _("_Zones:");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = 16; 
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_Zones+1000;
	gcd[k++].creator = GLabelCreate;
	hvarray[hv++] = &gcd[k-1];

	sprintf( buffer[0], "%d", (data[14]<<8)|data[14+1] );
	label[k].text = (unichar_t *) buffer[0];
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 60; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-6;
	gcd[k].gd.pos.width = 50;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_Zones;
	gcd[k++].creator = GTextFieldCreate;
	hvarray[hv++] = &gcd[k-1];

	label[k].text = (unichar_t *) _("_Twilight Pnt Cnt:");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 120; gcd[k].gd.pos.y = gcd[k-2].gd.pos.y; 
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_TPoints+1000;
	gcd[k++].creator = GLabelCreate;
	hvarray[hv++] = &gcd[k-1];

	sprintf( buffer[1], "%d", (data[16]<<8)|data[16+1] );
	label[k].text = (unichar_t *) buffer[1];
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 202; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-6;
	gcd[k].gd.pos.width = 50;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_TPoints;
	gcd[k++].creator = GTextFieldCreate;
	hvarray[hv++] = &gcd[k-1]; hvarray[hv++] = NULL;

	label[k].text = (unichar_t *) _("St_orage:");
	label[k].text_in_resource = true;
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = gcd[k-4].gd.pos.x; gcd[k].gd.pos.y = gcd[k-3].gd.pos.y+24+6; 
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_Storage;
	gcd[k++].creator = GLabelCreate;
	hvarray[hv++] = &gcd[k-1];

	sprintf( buffer[2], "%d", (data[18]<<8)|data[18+1] );
	label[k].text = (unichar_t *) buffer[2];
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = gcd[k-4].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-6;
	gcd[k].gd.pos.width = 50;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_Storage;
	gcd[k++].creator = GTextFieldCreate;
	hvarray[hv++] = &gcd[k-1];

	label[k].text = (unichar_t *) _("Max _Stack Depth:");
	label[k].text_in_resource = true;
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = gcd[k-4].gd.pos.x; gcd[k].gd.pos.y = gcd[k-2].gd.pos.y; 
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_SEl+1000;
	gcd[k++].creator = GLabelCreate;
	hvarray[hv++] = &gcd[k-1];

	sprintf( buffer[3], "%d", (data[24]<<8)|data[24+1] );
	label[k].text = (unichar_t *) buffer[3];
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = gcd[k-4].gd.pos.x; gcd[k].gd.pos.y = gcd[k-2].gd.pos.y;
	gcd[k].gd.pos.width = 50;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_SEl;
	gcd[k++].creator = GTextFieldCreate;
	hvarray[hv++] = &gcd[k-1]; hvarray[hv++] = NULL;

	label[k].text = (unichar_t *) _("_FDEF");
	label[k].text_in_resource = true;
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = gcd[k-4].gd.pos.x; gcd[k].gd.pos.y = gcd[k-3].gd.pos.y+24+6; 
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_FDefs+1000;
	gcd[k++].creator = GLabelCreate;
	hvarray[hv++] = &gcd[k-1];

	sprintf( buffer[4], "%d", (data[20]<<8)|data[20+1] );
	label[k].text = (unichar_t *) buffer[4];
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = gcd[k-4].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-6;  gcd[k].gd.pos.width = 50;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_FDefs;
	gcd[k++].creator = GTextFieldCreate;
	hvarray[hv++] = &gcd[k-1];

	label[k].text = (unichar_t *) _("_IDEFs");
	label[k].text_in_resource = true;
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = gcd[k-4].gd.pos.x; gcd[k].gd.pos.y = gcd[k-2].gd.pos.y; 
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_IDefs+1000;
	gcd[k++].creator = GLabelCreate;
	hvarray[hv++] = &gcd[k-1];

	sprintf( buffer[5], "%d", (data[22]<<8)|data[22+1] );
	label[k].text = (unichar_t *) buffer[5];
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = gcd[k-4].gd.pos.x; gcd[k].gd.pos.y = gcd[k-2].gd.pos.y;  gcd[k].gd.pos.width = 50;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_IDefs;
	gcd[k++].creator = GTextFieldCreate;
	hvarray[hv++] = &gcd[k-1]; hvarray[hv++] = NULL; hvarray[hv++] = NULL;

	boxes[2].gd.flags = gg_enabled|gg_visible;
	boxes[2].gd.u.boxelements = hvarray;
	boxes[2].creator = GHVBoxCreate;
	varray[0] = &boxes[2]; varray[1] = NULL;
	varray[2] = GCD_Glue; varray[3] = NULL;

    gcd[k].gd.pos.x = 20-3; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+35-3;
    gcd[k].gd.pos.width = -1; gcd[k].gd.pos.height = 0;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[k].text = (unichar_t *) _("_OK");
    label[k].text_in_resource = true;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = Maxp_OK;
    gcd[k++].creator = GButtonCreate;
    butarray[0] = GCD_Glue; butarray[1] = &gcd[k-1]; butarray[2] = GCD_Glue;

    gcd[k].gd.pos.x = -20; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+3;
    gcd[k].gd.pos.width = -1; gcd[k].gd.pos.height = 0;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = Maxp_Cancel;
    gcd[k++].creator = GButtonCreate;
    butarray[3] = GCD_Glue; butarray[4] = &gcd[k-1]; butarray[5] = GCD_Glue;
    butarray[6] = NULL;

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = butarray;
    boxes[3].creator = GHBoxCreate;
    varray[4] = &boxes[3]; varray[5] = NULL;
    varray[6] = NULL;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = varray;
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(gw,boxes);
    GHVBoxSetExpandableRow(boxes[0].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[3].ret,gb_expandgluesame);
    GHVBoxFitWindow(boxes[0].ret);
    GDrawSetVisible(gw,true);
    while ( !mp.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
}

void SFEditTable(SplineFont *sf, uint32 tag) {
    struct instrdata *id;
    struct ttf_table *tab;
    char name[12];
    char title[100];

    /* In multiple master fonts the 'fpgm' and 'prep' tables are stored in the*/
    /*  normal instance of the font. The other instances must share it */
    /* On the other hand, everyone can have their own cvt table */
    if ( tag!=CHR('c','v','t',' ') )
	if ( sf->mm!=NULL && sf->mm->apple )
	    sf = sf->mm->normal;

    tab = SFFindTable(sf,tag);
    if ( tag==CHR('m','a','x','p') ) {
	maxpCreateEditor(tab,sf,tag);
    } else if ( tag!=CHR('c','v','t',' ') ) {
	for ( id = sf->instr_dlgs; id!=NULL && id->tag!=tag; id=id->next );
	if ( id!=NULL ) {
	    GDrawSetVisible(id->id->gw,true);
	    GDrawRaise(id->id->gw);
return;
	}

	id = calloc(1,sizeof(*id));
	id->sf = sf;
	id->tag = tag;
	id->instr_cnt = id->max = tab==NULL ? 0 : tab->len;
	id->instrs = malloc(id->max+1);
	if ( tab!=NULL && tab->data!=NULL )
	    memcpy(id->instrs,tab->data,id->instr_cnt);
	else
	    id->instrs[0]='\0';
	name[0] = name[5] = '\'';
	name[1] = tag>>24; name[2] = (tag>>16)&0xff; name[3] = (tag>>8)&0xff; name[4] = tag&0xff;
	name[6] = 0;
	sprintf(title,_("TrueType Instructions for %.50s"),name);
	InstrDlgCreate(id,title);
    } else {
	if ( sf->cvt_dlg!=NULL ) {
	    GDrawSetVisible(sf->cvt_dlg->gw,true);
	    GDrawRaise(sf->cvt_dlg->gw);
return;
	}
	cvtCreateEditor(tab,sf,tag);
    }
}
