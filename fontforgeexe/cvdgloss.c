/* Copyright (C) 2005-2012 by George Williams */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * dercved from this software without specific prior written permission.

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
#include <math.h>
#include <gkeysym.h>
#include <ustring.h>
#include <stdarg.h>

extern GBox _ggadget_Default_Box;
#define MAIN_FOREGROUND (_ggadget_Default_Box.main_foreground)

#ifdef FREETYPE_HAS_DEBUGGER

#include <ft2build.h>
#include FT_FREETYPE_H

#include <internal/internal.h>
#include "ttinterp.h"

#define PPEMX(exc)	((exc)->size->root.metrics.x_ppem)
#define PPEMY(exc)	((exc)->size->root.metrics.y_ppem)

struct scr {
    int y, fh;
    int lines;
    GWindow pixmap;
};

static int ttround(int val, TT_ExecContext exc) {
    if ( val<0 )
return( -ttround(-val,exc));

    switch ( exc->GS.round_state ) {
      case TT_Round_To_Grid:
	val = 64*((val+32)/64);
      break;
      case TT_Round_Down_To_Grid:
	val = 64*(val/64);
      break;
      case TT_Round_Up_To_Grid:
	val = 64*((val+63)/64);
      break;
      case TT_Round_To_Half_Grid:
	val = 64*(val/64)+32;
      break;
      case TT_Round_Super:
	val = (val - exc->phase + exc->threshold) & -exc->period;
	val += exc->phase;
      break;
      case TT_Round_Super_45:
	val = ((val - exc->phase + exc->threshold) / exc->period)*exc->period;
	val += exc->phase;
      break;
    }
return(val);
}

static void scrprintf(struct scr *scr, char *format, ... ) {
    va_list ap;
    char buffer[100];

    va_start(ap,format);
    vsnprintf(buffer,sizeof(buffer),format,ap);
    GDrawDrawText8(scr->pixmap,3,scr->y,buffer,-1,MAIN_FOREGROUND);
    scr->y += scr->fh;
    ++scr->lines;
    va_end(ap);
}

static void scrrounding(struct scr *scr, TT_ExecContext exc ) {
    scrprintf(scr, "RndState: %s",
	    exc->GS.round_state==TT_Round_To_Half_Grid? "To Half Grid" :
	    exc->GS.round_state==TT_Round_To_Grid? "To Grid" :
	    exc->GS.round_state==TT_Round_To_Double_Grid? "To Double Grid" :
	    exc->GS.round_state==TT_Round_Down_To_Grid? "Down To Grid" :
	    exc->GS.round_state==TT_Round_Up_To_Grid? "Up To Grid" :
	    exc->GS.round_state==TT_Round_Off? "Off" :
	    exc->GS.round_state==TT_Round_Super? "Super" :
	    exc->GS.round_state==TT_Round_Super_45? "Super45" :
		"Unknown" );
}

static void scrfree(struct scr *scr, TT_ExecContext exc ) {
    scrprintf(scr, "freeVec: %g,%g", (((int)exc->GS.freeVector.x<<16)>>(16+14)) + ((exc->GS.freeVector.x&0x3fff)/16384.0),
	    (((int)exc->GS.freeVector.y<<16)>>(16+14)) + ((exc->GS.freeVector.y&0x3fff)/16384.0) );
}

static void scrproj(struct scr *scr, TT_ExecContext exc ) {
    scrprintf(scr,"projVec: %g,%g", (((int)exc->GS.projVector.x<<16)>>(16+14)) + ((exc->GS.projVector.x&0x3fff)/16384.0),
	    (((int)exc->GS.projVector.y<<16)>>(16+14)) + ((exc->GS.projVector.y&0x3fff)/16384.0) );
}

static int DVGlossExpose(GWindow pixmap,DebugView *dv,GEvent *event) {
    TT_ExecContext exc = DebuggerGetEContext(dv->dc);
    CharView *cv = dv->cv;
    long val1, val2, ret, i, cnt, off, a1, a2, b1, b2;
    int operator;
    BasePoint freedom;
    struct scr scr;
    int base;

    GDrawFillRect(pixmap,&event->u.expose.rect,GDrawGetDefaultBackground(screen_display));
    GDrawSetFont(pixmap,dv->ii.gfont);
    scr.pixmap = pixmap;
    scr.y = 3+dv->ii.as - dv->gloss_offtop*dv->ii.fh;
    scr.lines = 0;
    scr.fh = dv->ii.fh;

    if ( exc==NULL ) {
	scrprintf(&scr,"<not running>");
return(1);
    }
    if ( exc->IP>=exc->codeSize || exc->code==NULL ) {
	scrprintf(&scr,"<at end>");
return(1);
    }

    operator = ((uint8 *) exc->code)[exc->IP];
    if ( operator>=0xc0 && operator <= 0xdf ) {
        scrprintf(&scr," MDRP: Move Direct Relative Point");
	if ( operator&0x10 )
	    scrprintf(&scr,(operator&0x10)?"  Set rp0 to point":"  don't set rp0 to point" );
	scrprintf(&scr,(operator&8)?"  Keep distance>=min dist":"  don't keep distance>=min dist");
	scrprintf(&scr,(operator&4)?"  Round distance":"  don't round distance");
	scrprintf(&scr,(operator&3)==0?"  Grey":(operator&3)==1?"  Black":(operator&3)==2?"  White":"  Undefined Rounding");
	scrprintf(&scr,"Move point so cur distance to rp0 is");
	scrprintf(&scr," the same as the original distance between");
	val1 = exc->stack[exc->top-1];
	scrprintf(&scr,"Pops: %d (point (%.2f,%.2f))", val1,
		exc->zp1.cur[val1].x/64.0,exc->zp1.cur[val1].y/64.0 ); 
	scrprintf(&scr," (in zone from zp1: %s)", exc->GS.gep1?"Normal":"Twilight" ); 
	val2 = exc->GS.rp0;
	scrprintf(&scr,"Reference Point rp0: %d (%.2f,%.2f)", val2,
		exc->zp0.cur[val2].x/64.0,exc->zp0.cur[val2].y/64.0 ); 
	scrprintf(&scr," (in zone from zp0: %s)", exc->GS.gep0?"Normal":"Twilight" ); 
	scrprintf(&scr,"Sets rp1: %d (rp0)", val2);
	scrprintf(&scr,"Sets rp2: %d (point)", val1);
	scrprintf(&scr, "MinDist: %.2f", exc->GS.minimum_distance/64.0 );
	scrprintf(&scr, "SingWidVal: %.2f", exc->GS.single_width_value/64.0 );
	scrprintf(&scr, "SingWidCut: %.2f", exc->GS.single_width_cutin/64.0 );
	scrrounding(&scr,exc);
	scrfree(&scr,exc);
	scrproj(&scr,exc);
    } else if ( operator>=0xe0 && operator <= 0xff ) {
        scrprintf(&scr," MIRP: Move Indirect Relative Point");
	if ( operator&0x10 )
	    scrprintf(&scr,(operator&0x10)?"  Set rp0 to point":"  don't set rp0 to point" );
	scrprintf(&scr,(operator&8)?"  Keep distance>=min dist":"  don't keep distance>=min dist");
	scrprintf(&scr,(operator&4)?"  Round distance/use cvt cutin":"  don't round distance nor use cvt cutin");
	scrprintf(&scr,(operator&3)==0?"  Grey":(operator&3)==1?"  Black":(operator&3)==2?"  White":"  Undefined Rounding");
	scrprintf(&scr,"Move point along freedom vector so distance");
	scrprintf(&scr," measured along projection to rp0 is the");
	scrprintf(&scr," value in the cvt table");
	val2 = exc->stack[exc->top-1];
	val1 = exc->stack[exc->top-2];
	scrprintf(&scr,"Pops: %d (point (%.2f,%.2f))", val1,
		exc->zp1.cur[val1].x/64.0,exc->zp1.cur[val1].y/64.0 ); 
	scrprintf(&scr," (in zone from zp1: %s)", exc->GS.gep1?"Normal":"Twilight" ); 
	scrprintf(&scr,"Pop: %d (cvt %.2f)", val2, exc->cvt[val2]/64.0 );
	val2 = exc->GS.rp0;
	scrprintf(&scr,"Reference Point rp0: %d (%.2f,%.2f)", val2,
		exc->zp0.cur[val2].x/64.0,exc->zp0.cur[val2].y/64.0 ); 
	scrprintf(&scr," (in zone from zp0: %s)", exc->GS.gep0?"Normal":"Twilight" ); 
	scrprintf(&scr,"Sets rp1: %d (rp0)", val2);
	scrprintf(&scr,"Sets rp2: %d (point)", val1);
	scrprintf(&scr, "MinDist: %.2f", exc->GS.minimum_distance/64.0 );
	scrprintf(&scr, "CvtCutin: %.2f", exc->GS.control_value_cutin/64.0 );
	scrprintf(&scr, "SingWidVal: %.2f", exc->GS.single_width_value/64.0 );
	scrprintf(&scr, "SingWidCut: %.2f", exc->GS.single_width_cutin/64.0 );
	scrprintf(&scr, "AutoFlip: %s", exc->GS.auto_flip?"True": "False" );
	scrrounding(&scr,exc);
	scrfree(&scr,exc);
	scrproj(&scr,exc);
    } else if ( operator>=0xb0 && operator <= 0xb7 ) {
	scrprintf(&scr," Pushes %d byte%s", operator+1-0xb0, operator==0xb0?"":"s" );
    } else if ( operator>=0xb8 && operator <= 0xbf ) {
	scrprintf(&scr," Pushes %d word%s", operator+1-0xb8, operator==0xb8?"":"s" );
    } else switch ( operator ) {
      case 0x0:
        scrprintf(&scr," Set Freedom/Projection vector to y axis");
      break;
      case 0x1:
        scrprintf(&scr," Set Freedom/Projection vector to x axis");
      break;
      case 0x2:
        scrprintf(&scr," Set Projection vector to y axis");
      break;
      case 0x3:
        scrprintf(&scr," Set Projection vector to x axis");
      break;
      case 0x4:
        scrprintf(&scr," Set Freedom vector to y axis");
      break;
      case 0x5:
        scrprintf(&scr," Set Freedom vector to x axis");
      break;
      case 0xb: case 0xa:
        scrprintf(&scr," Set %s vector from stack", operator==0xb?"Freedom":"Projection");
	val1 = exc->stack[exc->top-2];
	val2 = exc->stack[exc->top-1];
	scrprintf(&scr,"Pops: %.4f (x-coord)", val1/16384.0 );
	scrprintf(&scr,"Pops: %.4f (y-coord)", val2/16384.0 );
	scrprintf(&scr,"Sets: %s Vector", operator==0xb?"Freedom":"Projection" );
      break;
      case 0xe:
        scrprintf(&scr," Set Freedom vector to Projection vector");
	scrprintf(&scr,"projVec: %g,%g", (((int)exc->GS.projVector.x<<16)>>(16+14)) + ((exc->GS.projVector.x&0x3fff)/16384.0),
		(((int)exc->GS.projVector.y<<16)>>(16+14)) + ((exc->GS.projVector.y&0x3fff)/16384.0) );
	scrprintf(&scr,"Sets: Freedom Vector" );
      break;
      case 0xc: case 0xd:
        scrprintf(&scr,operator==0xc?" Get Projection Vector":" Get Freedom Vector"); 
	scrprintf(&scr,operator==0xc?"Pushes: Projection Vector":"Pushes: Freedom Vector"); 
	if ( operator==0xd )
	    scrfree(&scr,exc);
	else
	    scrproj(&scr,exc);
      break;
      case 0xf:
        scrprintf(&scr," Moves point to intersection of two lines"); 
	val1 = exc->stack[exc->top-5];
	a1 = exc->stack[exc->top-4];
	a2 = exc->stack[exc->top-3];
	b1 = exc->stack[exc->top-2];
	b2 = exc->stack[exc->top-1];

	scrprintf(&scr,"Pops: %d (point (%.2f,%.2f))", val1,
		exc->zp2.cur[val1].x/64.0,exc->zp2.cur[val1].y/64.0 ); 
	scrprintf(&scr," (in zone from zp2: %s)", exc->GS.gep2?"Normal":"Twilight" ); 

	scrprintf(&scr,"Pops: %d (line1.s (%.2f,%.2f))", a1,
		exc->zp0.cur[a1].x/64.0,exc->zp0.cur[a1].y/64.0 ); 
	scrprintf(&scr,"Pops: %d (line1.e (%.2f,%.2f))", a2,
		exc->zp0.cur[a2].x/64.0,exc->zp0.cur[a2].y/64.0 ); 
	scrprintf(&scr," (in zone from zp0: %s)", exc->GS.gep0?"Normal":"Twilight" ); 

	scrprintf(&scr,"Pops: %d (line2.s (%.2f,%.2f))", b1,
		exc->zp1.cur[b1].x/64.0,exc->zp1.cur[b1].y/64.0 ); 
	scrprintf(&scr,"Pops: %d (line2.e (%.2f,%.2f))", b2,
		exc->zp1.cur[b2].x/64.0,exc->zp1.cur[b2].y/64.0 ); 
	scrprintf(&scr," (in zone from zp1: %s)", exc->GS.gep1?"Normal":"Twilight" ); 

	scrprintf(&scr,"(ignores freedom vector)"); 
      break;
      case 0x10:
	scrprintf(&scr," Set Reference Point 0");
	val1 = exc->stack[exc->top-1];
	scrprintf(&scr,"Pops: %d (point = new rp0)", val1 );
      break;
      case 0x11:
	scrprintf(&scr," Set Reference Point 1");
	val1 = exc->stack[exc->top-1];
	scrprintf(&scr,"Pops: %d (point = new rp1)", val1 );
      break;
      case 0x12:
	scrprintf(&scr," Set Reference Point 2");
	val1 = exc->stack[exc->top-1];
	scrprintf(&scr,"Pops: %d (point = new rp2)", val1 );
      break;
      case 0x13:
	scrprintf(&scr," Set Zone Pointer 0");
	val1 = exc->stack[exc->top-1];
	scrprintf(&scr,"Pops: %d (new zp0 %s)", val1, val1?"Normal":"Twilight" );
      break;
      case 0x14:
	scrprintf(&scr," Set Zone Pointer 1");
	val1 = exc->stack[exc->top-1];
	scrprintf(&scr,"Pops: %d (new zp1 %s)", val1, val1?"Normal":"Twilight" );
      break;
      case 0x15:
	scrprintf(&scr," Set Zone Pointer 2");
	val1 = exc->stack[exc->top-1];
	scrprintf(&scr,"Pops: %d (new zp2 %s)", val1, val1?"Normal":"Twilight" );
      break;
      case 0x16:
	scrprintf(&scr," Set Zone Pointers");
	val1 = exc->stack[exc->top-1];
	scrprintf(&scr,"Pops: %d (new zp0,zp1,zp2 %s)", val1, val1?"Normal":"Twilight" );
      break;
      case 0x17:
	scrprintf(&scr," Set Loop variable");
	val1 = exc->stack[exc->top-1];
	scrprintf(&scr,"Pops: %d (new loop count)", val1 );
      break;
      case 0x1A:
	scrprintf(&scr," Set Minimum Distance");
	val1 = exc->stack[exc->top-1];
	scrprintf(&scr,"Pops: %d (new minimum distance)", val1/64.0 );
      break;
      case 0x1B: case 0x2D: case 0x59:
	switch( operator ) {
	  case 0x1B:
	    scrprintf(&scr," Else");
	  break;
	  case 0x2D:
	    scrprintf(&scr," End Function Definition (return)");
	  break;
	  case 0x59:
	    scrprintf(&scr," End If");
	  break;
	}
      break;
      case 0x1C:
	scrprintf(&scr," Jump Relative (to here)");
	val1 = exc->stack[exc->top-1];
	scrprintf(&scr,"Pops: %d (byte offset)", val1 );
      break;
      case 0x1D:
	scrprintf(&scr," Set Control Value Table Cut-In");
	val1 = exc->stack[exc->top-1];
	scrprintf(&scr,"Pops: %.2f (new cvt cut-in value)", val1/64.0 );
      break;
      case 0x1E:
	scrprintf(&scr," Set Single Width Cut-In");
	val1 = exc->stack[exc->top-1];
	scrprintf(&scr,"Pops: %.2f (new single width cut-in value)", val1/64.0 );
      break;
      case 0x1F:
	scrprintf(&scr," Set Single Width");
	val1 = exc->stack[exc->top-1];
	scrprintf(&scr,"Pops: %.2f (new single width value)", val1/64.0 );
      break;
      case 0x20:
	scrprintf(&scr," Duplicate TOS");
	scrprintf(&scr,"Pushes: %d", exc->stack[exc->top-1] );
      break;
      case 0x21:
	scrprintf(&scr," Pop TOS");
	scrprintf(&scr,"Pops: %d", exc->stack[exc->top-1] );
      break;
      case 0x22:
	scrprintf(&scr," Clear Stack");
	scrprintf(&scr,"Pops everything" );
      break;
      case 0x23:
	scrprintf(&scr," Swap top of stack");
	scrprintf(&scr,"Pops: top two elements" );
	scrprintf(&scr,"Pushs: top two elements in opposite order" );
      break;
      case 0x24:
	scrprintf(&scr," Depth of Stack");
	scrprintf(&scr,"Pushes: %d", exc->top );
      break;
      case 0x25: case 0x26:
	val1 = exc->stack[exc->top-1];
	scrprintf(&scr,operator==0x25?" Copy Indexed element to TOS":" Move Indexed element to TOS");
	scrprintf(&scr,"Pops: %d", val1 );
	if ( val1<exc->top ) {
	    val2 = exc->stack[exc->top-1-val1];
	    scrprintf(&scr,"Pushes: %.2f (%d)", val2/64.0, val2 );
	    if ( operator==0x26 )
		scrprintf(&scr,"(and removes it from further down the stack)" );
	} else
	    scrprintf(&scr,"*** Stack underflow ***" );
      break;
      case 0x27:
	scrprintf(&scr," Align Points");
	val2 = exc->stack[exc->top-1];
	val1 = exc->stack[exc->top-2];
	scrprintf(&scr,"Pop1: %d (point (%.2f,%.2f))", val1,
		exc->zp1.cur[val1].x/64.0,exc->zp1.cur[val1].y/64.0 );
	scrprintf(&scr," (in zone from zp1: %s)", exc->GS.gep1?"Normal":"Twilight" );
	scrprintf(&scr,"Pop2: %d (point (%.2f,%.2f))", val2,
		exc->zp0.cur[val2].x/64.0,exc->zp0.cur[val2].y/64.0 );
	scrprintf(&scr," (in zone from zp0: %s)", exc->GS.gep0?"Normal":"Twilight" );
	scrfree(&scr,exc);
	scrproj(&scr,exc);
      break;
      case 0x28:
	scrprintf(&scr," Untouch Point");
	val1 = exc->stack[exc->top-1];
	scrprintf(&scr,"Pop1: %d (point (%.2f,%.2f))", val1,
		exc->zp0.cur[val1].x/64.0,exc->zp0.cur[val1].y/64.0 );
	scrprintf(&scr," (in zone from zp0: %s)", exc->GS.gep0?"Normal":"Twilight" );
	scrfree(&scr,exc);
      break;
      case 0x2A:
      case 0x2B:
      case 0x2C:
        switch ( operator ) {
	  case 0x2A:
	    scrprintf(&scr," Loop Call Function");
	  break;
	  case 0x2B:
	    scrprintf(&scr," Call Function");
	  break;
	  case 0x2C:
	    scrprintf(&scr," Function Definition");
	  break;
	}
	val1 = exc->stack[exc->top-1];
	scrprintf(&scr,"Pops: %d (function number)", val1 );
	if ( operator==0x2a ) {
	    val2 = exc->stack[exc->top-2];
	    scrprintf(&scr,"Pops: %d (count)", val2 );
	}
      break;
      case 0x2E: case 0x2F:
	scrprintf(&scr, operator==0x2E?" MDAP (touch point)":" MDAP (round & touch point)");
	val1 = exc->stack[exc->top-1];
	scrprintf(&scr,"Pops: %d (point (%.2f,%.2f))", val1,
		exc->zp0.cur[val1].x/64.0,exc->zp0.cur[val1].y/64.0 );
	scrprintf(&scr," (in zone from zp0: %s)", exc->GS.gep0?"Normal":"Twilight" );
	scrprintf(&scr,"Sets: rp0,rp1 to %d", val1 );
	if ( operator==0x2F )
	    scrrounding(&scr,exc);
	scrfree(&scr,exc);
	scrproj(&scr,exc);
      break;
      case 0x30: case 0x31:
        switch ( operator ) {
	  case 0x30:
	    scrprintf(&scr," Interpolate Untouched Points in y");
	  break;
	  case 0x31:
	    scrprintf(&scr," Interpolate Untouched Points in x");
	  break;
	}
	scrprintf(&scr," (in zone from zp2: %s)", exc->GS.gep2?"Normal":"Twilight" );
      break;
      case 0x32: case 0x33:
        scrprintf(&scr," Shift point by amount ref point shifted");
	if ( operator==0x33 ) {
	    scrprintf(&scr, "Reference point in rp1: %d (point (%.2f,%.2f))", exc->GS.rp2,
		    exc->zp0.cur[exc->GS.rp1].x/64.0,exc->zp0.cur[exc->GS.rp1].y/64.0 );
	    scrprintf(&scr," (in zone from zp0: %s)", exc->GS.gep0?"Normal":"Twilight" );
	} else {
	    scrprintf(&scr, "Reference point in rp2: %d (point (%.2f,%.2f))", exc->GS.rp2,
		    exc->zp1.cur[exc->GS.rp2].x/64.0,exc->zp1.cur[exc->GS.rp2].y/64.0 );
	    scrprintf(&scr," (in zone from zp1: %s)", exc->GS.gep1?"Normal":"Twilight" );
	}
	scrprintf(&scr, "loop: %ld", exc->GS.loop );
	for ( val1=1; val1<=exc->GS.loop && val1<exc->top; ++val1 ) {
	    val2 = exc->stack[exc->top-val1];
	    scrprintf(&scr,"Pop%d: %d (point (%.2f,%.2f))", val1, val2,
		    exc->zp2.cur[val2].x/64.0,exc->zp2.cur[val2].y/64.0 );
	}
	scrprintf(&scr," (in zone from zp2: %s)", exc->GS.gep2?"Normal":"Twilight" );
	scrfree(&scr,exc);
	scrproj(&scr,exc);
      break;
      case 0x34: case 0x35:
        scrprintf(&scr," Shift contour by amount ref point shifted");
	if ( operator==0x35 ) {
	    scrprintf(&scr, "Reference point in rp1: %d (point (%.2f,%.2f))", exc->GS.rp2,
		    exc->zp0.cur[exc->GS.rp1].x/64.0,exc->zp0.cur[exc->GS.rp1].y/64.0 );
	    scrprintf(&scr," (in zone from zp0: %s)", exc->GS.gep0?"Normal":"Twilight" );
	} else {
	    scrprintf(&scr, "Reference point in rp2: %d (point (%.2f,%.2f))", exc->GS.rp2,
		    exc->zp1.cur[exc->GS.rp2].x/64.0,exc->zp1.cur[exc->GS.rp2].y/64.0 );
	    scrprintf(&scr," (in zone from zp1: %s)", exc->GS.gep1?"Normal":"Twilight" );
	}
	scrprintf(&scr, "Pops: %d (contour index)", exc->stack[exc->top-1]);
	scrprintf(&scr," (in zone from zp2: %s)", exc->GS.gep2?"Normal":"Twilight" );
	scrfree(&scr,exc);
	scrproj(&scr,exc);
      break;
      case 0x36: case 0x37:
        scrprintf(&scr," Shift zone by amount ref point shifted");
	if ( operator==0x37 ) {
	    scrprintf(&scr, "Reference point in rp1: %d (point (%.2f,%.2f))", exc->GS.rp2,
		    exc->zp0.cur[exc->GS.rp1].x/64.0,exc->zp0.cur[exc->GS.rp1].y/64.0 );
	    scrprintf(&scr," (in zone from zp0: %s)", exc->GS.gep0?"Normal":"Twilight" );
	} else {
	    scrprintf(&scr, "Reference point in rp2: %d (point (%.2f,%.2f))", exc->GS.rp2,
		    exc->zp1.cur[exc->GS.rp2].x/64.0,exc->zp1.cur[exc->GS.rp2].y/64.0 );
	    scrprintf(&scr," (in zone from zp1: %s)", exc->GS.gep1?"Normal":"Twilight" );
	}
	scrprintf(&scr, "Pops: %d (%s zone)", exc->stack[exc->top-1],
		exc->stack[exc->top-1]?"Normal":"Twilight" );
	scrfree(&scr,exc);
	scrproj(&scr,exc);
      break;
      case 0x38:
        scrprintf(&scr," Shift point by pixel amount");
	val1 = exc->stack[exc->top-1];
	scrprintf(&scr, "Pops: %.2f (pixel amount)", val1/64.0 );
	scrprintf(&scr, "loop: %ld", exc->GS.loop );
	for ( val1=2; val1<=exc->GS.loop+1 && val1<exc->top; ++val1 ) {
	    val2 = exc->stack[exc->top-val1];
	    scrprintf(&scr,"Pop%d: %d (point (%.2f,%.2f))", val1, val2,
		    exc->zp2.cur[val2].x/64.0,exc->zp2.cur[val2].y/64.0 );
	}
	scrprintf(&scr," (in zone from zp2: %s)", exc->GS.gep2?"Normal":"Twilight" );
	scrfree(&scr,exc);
	scrproj(&scr,exc);
      break;
      case 0x39:
	scrprintf(&scr," Interpolated Point");
	scrprintf(&scr, "Reference point in rp1: %d (point (%.2f,%.2f))", exc->GS.rp1,
		exc->zp0.cur[exc->GS.rp1].x/64.0,exc->zp0.cur[exc->GS.rp1].y/64.0 );
	scrprintf(&scr," (in zone from zp0: %s)", exc->GS.gep0?"Normal":"Twilight" );
	scrprintf(&scr, "Reference point in rp2: %d (point (%.2f,%.2f))", exc->GS.rp2,
		exc->zp1.cur[exc->GS.rp2].x/64.0,exc->zp1.cur[exc->GS.rp2].y/64.0 );
	scrprintf(&scr," (in zone from zp1: %s)", exc->GS.gep1?"Normal":"Twilight" );
	scrprintf(&scr, "loop: %ld", exc->GS.loop );
	for ( val1=1; val1<=exc->GS.loop && val1<exc->top; ++val1 ) {
	    val2 = exc->stack[exc->top-val1];
	    scrprintf(&scr,"Pop%d: %d (point (%.2f,%.2f))", val1, val2,
		    exc->zp2.cur[val2].x/64.0,exc->zp2.cur[val2].y/64.0 );
	}
	scrprintf(&scr," (in zone from zp2: %s)", exc->GS.gep2?"Normal":"Twilight" );
	scrfree(&scr,exc);
	scrproj(&scr,exc);
      break;
      case 0x3A: case 0x3B:
	scrprintf(&scr," MSIRP Move Stack Indirect Relative Point"); 
	scrprintf(&scr,operator&1?"  set rp0 to point":
		    "  don't set rp0"); 
	scrprintf(&scr,"moves point along freedom vector");
	scrprintf(&scr," until distance from rp0 along");
	scrprintf(&scr," projection vector is value from stack");
	val2 = exc->stack[exc->top-1];
	val1 = exc->stack[exc->top-2];
	scrprintf(&scr,"Pop: %d (point (%.2f,%.2f))", val1,
		exc->zp1.cur[val1].x/64.0,exc->zp1.cur[val1].y/64.0 );
	scrprintf(&scr," (in zone from zp1: %s)", exc->GS.gep1?"Normal":"Twilight" );
	scrprintf(&scr, "Reference point in rp0: %d (point (%.2f,%.2f))", exc->GS.rp0,
		exc->zp0.cur[exc->GS.rp0].x/64.0,exc->zp0.cur[exc->GS.rp0].y/64.0 );
	scrprintf(&scr," (in zone from zp0: %s)", exc->GS.gep0?"Normal":"Twilight" );
	scrprintf(&scr,"Pop: %.2f (distance)", val2/64.0 );
	scrfree(&scr,exc);
	scrproj(&scr,exc);
      break;
      case 0x3C:
	scrprintf(&scr," Align to Reference Point"); 
	scrprintf(&scr, "Reference point in rp0: %d (point (%.2f,%.2f))", exc->GS.rp0,
		exc->zp0.cur[exc->GS.rp0].x/64.0,exc->zp0.cur[exc->GS.rp0].y/64.0 );
	scrprintf(&scr," (in zone from zp0: %s)", exc->GS.gep0?"Normal":"Twilight" );
	scrprintf(&scr, "loop: %ld", exc->GS.loop ); 
	for ( val1=1; val1<=exc->GS.loop && val1<exc->top; ++val1 ) {
	    val2 = exc->stack[exc->top-val1];
	    scrprintf(&scr,"Pop%d: %d (point (%.2f,%.2f))", val1, val2,
		    exc->zp1.cur[val2].x/64.0,exc->zp1.cur[val2].y/64.0 );
	}
	scrprintf(&scr," (in zone from zp1: %s)", exc->GS.gep1?"Normal":"Twilight" );
	scrfree(&scr,exc);
	scrproj(&scr,exc);
      break;
      case 0x3E: case 0x3F:
	scrprintf(&scr," MIAP Move Indirect Absolute Point"); 
	scrprintf(&scr,operator&1?"  round distance and look at cvt cutin":
		    "  don't round distance and look at cvt cutin"); 
	scrprintf(&scr,"moves point to cvt value along freedom vector"); 
	val2 = exc->stack[exc->top-1];
	val1 = exc->stack[exc->top-2];
	scrprintf(&scr,"Pop: %d (point (%.2f,%.2f))", val1,
		exc->zp0.cur[val1].x/64.0,exc->zp0.cur[val1].y/64.0 );
	scrprintf(&scr," (in zone from zp0: %s)", exc->GS.gep0?"Normal":"Twilight" );
	scrprintf(&scr,"Pop: %d (cvt %.2f)", val2, exc->cvt[val2]/64.0 );
	scrprintf(&scr,"Sets: rp0 and rp1 to point %d", val1 );
	scrfree(&scr,exc);
	scrproj(&scr,exc);
	scrprintf(&scr, "CvtCutin: %.2f", exc->GS.control_value_cutin/64.0 );
      break;
      case 0x40:
	scrprintf(&scr," Push %d Bytes", ((uint8 *) exc->code)[exc->IP+1]);
      break;
      case 0x41:
	scrprintf(&scr," Push %d Words", ((uint8 *) exc->code)[exc->IP+1]);
      break;
      case 0x42:
        scrprintf(&scr," Write Store" );
	val2 = exc->stack[exc->top-1];
	val1 = exc->stack[exc->top-2];
	scrprintf(&scr,"Pops: %d (%.2f)", val2, val2/64.0 );
	scrprintf(&scr,"Pops: %d (store index)", val1 );
      break;
      case 0x43:
	scrprintf(&scr," Read Store" );
	val1 = exc->stack[exc->top-1];
	scrprintf(&scr,"Pop: %d (store index)", val1 );
	scrprintf(&scr,"Pushes: %d (%.2f)", exc->storage[val1], exc->storage[val1]/64.0 );
      break;
      case 0x44:
        scrprintf(&scr," Write CVT entry in Pixels" );
	val2 = exc->stack[exc->top-1];
	val1 = exc->stack[exc->top-2];
	scrprintf(&scr,"Pops: %.2f", val2/64.0 );
	scrprintf(&scr,"Pops: %d (cvt index)", val1 );
      break;
      case 0x45:
	scrprintf(&scr,"Read Control Value Table entry" ); 
	val1 = exc->stack[exc->top-1];
	scrprintf(&scr,"Pop: %d (cvt index)", val1 );
	scrprintf(&scr,"Pushes: %.2f (%d)", exc->cvt[val1]/64.0, exc->cvt[val1] );
      break;
      case 0x46: case 0x47:
	scrprintf(&scr,"Get %s point coord projected on projection vector",
		operator==0x46 ? "current" : "original" ); 
	val1 = exc->stack[exc->top-1];
	if ( operator==0x46 ) {
	    scrprintf(&scr,"Pop: %d (cur point (%.2f,%.2f))", val1,
		    exc->zp2.cur[val1].x/64.0,exc->zp2.cur[val1].y/64.0 ); 
	    scrprintf(&scr," (in zone from zp2: %s)", exc->GS.gep2?"Normal":"Twilight" ); 
	    scrprintf(&scr, "projVec: %g,%g", (((int)exc->GS.projVector.x<<16)>>(16+14)) + ((exc->GS.projVector.x&0x3fff)/16384.0),
		    (((int)exc->GS.projVector.y<<16)>>(16+14)) + ((exc->GS.projVector.y&0x3fff)/16384.0) );
	} else {
	    scrprintf(&scr,"Pop: %d (orig point (%.2f,%.2f))", val1,
		    exc->zp2.org[val1].x/64.0,exc->zp2.org[val1].y/64.0 ); 
	    scrprintf(&scr," (in zone from zp2: %s)", exc->GS.gep2?"Normal":"Twilight" ); 
	    scrprintf(&scr,"dualVec: %g,%g", (((int)exc->GS.dualVector.x<<16)>>(16+14)) + ((exc->GS.dualVector.x&0x3fff)/16384.0),
		    (((int)exc->GS.dualVector.y<<16)>>(16+14)) + ((exc->GS.dualVector.y&0x3fff)/16384.0) );
	}
	scrprintf(&scr,"Pushes: projection" ); 
      break;
      case 0x48:
	scrprintf(&scr," Sets coordinate from stack using proj & free vectors" );
	val2 = exc->stack[exc->top-1];
	val1 = exc->stack[exc->top-2];
	scrprintf(&scr,"Moves point along freedom vector until its" );
	scrprintf(&scr," projection on the projection vector is the" );
	scrprintf(&scr," desired amount" );
	scrprintf(&scr,"Pop: %d (cur point (%.2f,%.2f))", val1,
		exc->zp2.cur[val1].x/64.0,exc->zp2.cur[val1].y/64.0 ); 
	scrprintf(&scr," (in zone from zp2: %s)", exc->GS.gep2?"Normal":"Twilight" ); 
	scrfree(&scr,exc);
	scrproj(&scr,exc);
      break;
      case 0x49: case 0x4A:
        switch ( operator ) {
	  case 0x49:
	    scrprintf(&scr," Measure Distance (current)");
	  break;
	  case 0x4A:
	    scrprintf(&scr," Measure Distance (original)");
	  break;
	}

	val1 = exc->stack[exc->top-1];
	val2 = exc->stack[exc->top-2];
	if ( operator==0x49 ) {
	    scrprintf(&scr,"Pop: %d (cur point (%.2f,%.2f))", val1,
		    exc->zp1.cur[val1].x/64.0,exc->zp1.cur[val1].y/64.0 ); 
	    scrprintf(&scr," (in zone from zp1: %s)", exc->GS.gep1?"Normal":"Twilight" ); 
	    scrprintf(&scr,"Pop: %d (cur point (%.2f,%.2f))", val2,
		    exc->zp0.cur[val2].x/64.0,exc->zp0.cur[val2].y/64.0 ); 
	    scrprintf(&scr," (in zone from zp0: %s)", exc->GS.gep0?"Normal":"Twilight" ); 
	    scrprintf(&scr, "projVec: %g,%g", (((int)exc->GS.projVector.x<<16)>>(16+14)) + ((exc->GS.projVector.x&0x3fff)/16384.0),
		    (((int)exc->GS.projVector.y<<16)>>(16+14)) + ((exc->GS.projVector.y&0x3fff)/16384.0) );
	} else {
	    scrprintf(&scr,"Pop: %d (orig point (%.2f,%.2f))", val1,
		    exc->zp1.org[val1].x/64.0,exc->zp1.org[val1].y/64.0 ); 
	    scrprintf(&scr," (in zone from zp1: %s)", exc->GS.gep1?"Normal":"Twilight" ); 
	    scrprintf(&scr,"Pop: %d (orig point (%.2f,%.2f))", val2,
		    exc->zp0.org[val2].x/64.0,exc->zp0.org[val2].y/64.0 ); 
	    scrprintf(&scr," (in zone from zp0: %s)", exc->GS.gep0?"Normal":"Twilight" ); 
	    scrprintf(&scr, "dualVec: %g,%g", (((int)exc->GS.dualVector.x<<16)>>(16+14)) + ((exc->GS.dualVector.x&0x3fff)/16384.0),
		    (((int)exc->GS.dualVector.y<<16)>>(16+14)) + ((exc->GS.dualVector.y&0x3fff)/16384.0) );
	}
	scrprintf(&scr,"Pushes: distance" ); 
      break;
      case 0x4B:
        scrprintf(&scr," MPPEM Push Pixels Per Em");
	scrprintf(&scr,"Pushes: %d", PPEMY(exc));	/* Actually this depends on the projection vector. But if xppem==yppem it will make no difference */
      break;
      case 0x4C:
        scrprintf(&scr," Push Pointsize");
        scrprintf(&scr,"(one might assume this returns the pointsize, %d", cv->ft_pointsizey);
        scrprintf(&scr," as it is documented to do, but instead it");
        scrprintf(&scr," returns ppem)");
	scrprintf(&scr,"Pushes: %d", PPEMY(exc) );
      break;
      case 0x4D: case 0x4E:
        switch ( operator ) {
	  case 0x4D:
	    scrprintf(&scr," set auto Flip On");
	  break;
	  case 0x4E:
	    scrprintf(&scr," set auto Flip Off");
	  break;
	}
      break;
      case 0x4F:
	val1 = exc->stack[exc->top-1];
	scrprintf(&scr," Debug"); 
	scrprintf(&scr,"Pops: %d (debug hook)", val1 ); 
      break;
      case 0x58:
	val1 = exc->stack[exc->top-1];
	scrprintf(&scr," If"); 
	scrprintf(&scr,"Pops: %d (condition)", val1 ); 
      break;
      case 0x5e:
	scrprintf(&scr," Set Delta Base");
	val1 = exc->stack[exc->top-1];
	scrprintf(&scr,"Pops: %d (new delta base)", val1 );
      break;
      case 0x5f:
	scrprintf(&scr," Set Delta Shift");
	val1 = exc->stack[exc->top-1];
	scrprintf(&scr,"Pops: %d (new delta shift)", val1 );
      break;
      case 0x8B: case 0x8C:
      case 0x60: case 0x61: case 0x62: case 0x63:
      case 0x5A: case 0x5B:
      case 0x55: case 0x54: case 0x53: case 0x52: case 0x51: case 0x50:
	val2 = exc->stack[exc->top-1];
	val1 = exc->stack[exc->top-2];
	switch( operator ) {
	  case 0x50:
	    scrprintf(&scr," Less Than");
	    ret = val1<val2;
	  break;
	  case 0x51:
	    scrprintf(&scr," Less Than Or Equal");
	    ret = val1<=val2;
	  break;
	  case 0x52:
	    scrprintf(&scr," Greater Than");
	    ret = val1>val2;
	  break;
	  case 0x53:
	    scrprintf(&scr," Greater Than or Equal");
	    ret = val1>=val2;
	  break;
	  case 0x54:
	    scrprintf(&scr," Equal");
	    ret = val1==val2;
	  break;
	  case 0x55:
	    scrprintf(&scr," Not Equal");
	    ret = val1!=val2;
	  break;
	  case 0x5A:
	    scrprintf(&scr," And");
	    ret = val1 & val2;
	  break;
	  case 0x5B:
	    scrprintf(&scr," Or");
	    ret = val1 | val2;
	  break;
	  case 0x60:
	    scrprintf(&scr," Add");
	    ret = val1 + val2;
	  break;
	  case 0x61:
	    scrprintf(&scr," Sub");
	    ret = val1 - val2;
	  break;
	  case 0x62:
	    scrprintf(&scr," Divide");
	    if ( val2!=0 )
		ret = val1*64/val2;
	    else
		ret = 0x7fffffff;
	  break;
	  case 0x63:
	    scrprintf(&scr," Multiply");
	    ret = val1*val2/64.0;
	  break;
	  case 0x8B:
	    scrprintf(&scr," Max");
	    ret = val1>val2 ? val1 : val2;
	  break;
	  case 0x8C:
	    scrprintf(&scr," Min");
	    ret = val1<val2 ? val1 : val2;
	  break;
	}
	scrprintf(&scr,"Pop1: %.2f (%d)", val1/64.0, val1 ); 
	scrprintf(&scr,"Pop2: %.2f (%d)", val2/64.0, val2 ); 
	scrprintf(&scr,"Pushes: %.2f (%d)", ret/64.0, ret ); 
      break;
      case 0x64: case 0x65: case 0x66: case 0x67:
      case 0x57: case 0x5C:
	val2 = val1 = exc->stack[exc->top-1];
	switch( operator ) {
	  case 0x64:
	    scrprintf(&scr," Absolute Value");
	    if ( val2<0 ) val2 = -val2;
	  break;
	  case 0x65:
	    scrprintf(&scr," Negate");
	    val2 = -val1;
	  break;
	  case 0x66:
	    scrprintf(&scr," Floor");
	    val2 = 64*(val2/64);
	  break;
	  case 0x67:
	    scrprintf(&scr," Ceiling");
	    val2 = 64*((val2+63)/64);
	  break;
	  case 0x5C:
	    scrprintf(&scr," Not");
	    val2 = !val1;
	  break;
	  case 0x56:
	    scrprintf(&scr," Odd (after rounding)");
	    ret = ttround(val1,exc);
	    if ( ret&64 ) ret = 0; else ret = 1;
	  break;
	  case 0x57:
	    scrprintf(&scr," Even (after rounding)");
	    ret = ttround(val1,exc);
	    if ( ret&64 ) ret = 0; else ret = 1;
	  break;
	}
	scrprintf(&scr,"Pops: %.2f (%d)", val1/64.0, val1 ); 
	scrprintf(&scr,"Pushes: %.2f (%d)", val2/64.0, val2 ); 
      break;
      case 0x68: case 0x69: case 0x6A: case 0x6B:
	scrprintf(&scr," Round & adjust for engine characteristics" ); 
	scrprintf(&scr,(operator&3)==0?"  Grey":(operator&3)==1?"  Black":(operator&3)==2?"  White":"  Undefined Rounding");
	val1 = exc->stack[exc->top-1];
	scrprintf(&scr,"Pops: %.2f", val1/64.0 );
	val1 = ttround(val1,exc);
	scrprintf(&scr,"Pushes: %.2f", val1/64.0 ); 
      break;
      case 0x6C: case 0x6D: case 0x6E: case 0x6F:
	scrprintf(&scr," Adjust for engine characteristics without rounding" ); 
	scrprintf(&scr,(operator&3)==0?"  Grey":(operator&3)==1?"  Black":(operator&3)==2?"  White":"  Undefined Rounding");
	val1 = exc->stack[exc->top-1];
	scrprintf(&scr,"Pops: %.2f", val1/64.0 ); 
	scrprintf(&scr,"Pushes: %.2f", val1/64.0 ); 
      break;
      case 0x5D: case 0x71: case 0x72:
        base = operator==0x5D?1:operator-0x6F;
	scrprintf(&scr," Delta Point%d", base ); 
	freedom.x = (((int)exc->GS.freeVector.x<<16)>>(16+14)) + ((exc->GS.freeVector.x&0x3fff)/16384.0);
	freedom.y = (((int)exc->GS.freeVector.y<<16)>>(16+14)) + ((exc->GS.freeVector.y&0x3fff)/16384.0);
	cnt = exc->stack[exc->top-1];
	scrprintf(&scr,"Pops: %d (count)", cnt ); 
	for ( i=0; i<cnt; ++i ) {
	    if ( 2*i+3 > exc->top ) {
		scrprintf(&scr,"*** Stack underflow ***"); 
	break;
	    }
	    val1 = exc->stack[exc->top-2-2*i];
	    val2 = exc->stack[exc->top-3-2*i];
	    if ( (val2&0xf)<=7 )
		off = -8+(val2&0xf);
	    else
		off = -7+(val2&0xf);
	    off *= 64 / (1L << exc->GS.delta_shift);
	    scrprintf(&scr,"Pops: %d (point (%.2f,%.2f))", val1,
		    exc->zp0.cur[val1].x/64.0,exc->zp0.cur[val1].y/64.0 ); 
	    scrprintf(&scr,"Pops: %d => at %d ppem, move (%.2f,%.2f)",
		val2, exc->GS.delta_base+(base-1)*16+((val2>>4)&0xf),
		freedom.x*off/64.0, freedom.y*off/64.0 ); 
	}
	scrprintf(&scr," (in zone from zp0: %s)", exc->GS.gep0?"Normal":"Twilight" );
	scrprintf(&scr, "DeltaBase: %d", exc->GS.delta_base ); 
	scrprintf(&scr, "DeltaShift: %d", exc->GS.delta_shift ); 
	scrprintf(&scr, "freeVec: %g,%g", freedom.x, freedom.y); 
      break;
      case 0x70:
        scrprintf(&scr," Write CVT entry in Funits" );
	val2 = exc->stack[exc->top-1];
	val1 = exc->stack[exc->top-2];
	scrprintf(&scr,"Pops: %d (em-units=%.2fpixels)", val2,
		val2*PPEMX(exc)*64.0/
		    (cv->b.sc->parent->ascent+cv->b.sc->parent->descent));
	scrprintf(&scr,"Pops: %d (cvt index)", val1 );
      break;
      case 0x73: case 0x74: case 0x75:
        base = operator-0x72;
	scrprintf(&scr," Delta Point%d", base ); 
	cnt = exc->stack[exc->top-1];
	scrprintf(&scr,"Pops: %d (count)", cnt ); 
	for ( i=0; i<cnt; ++i ) {
	    if ( 2*cnt+3 > exc->top ) {
		scrprintf(&scr,"*** Stack underflow ***"); 
	break;
	    }
	    val1 = exc->stack[exc->top-2-2*i];
	    val2 = exc->stack[exc->top-3-2*i];
	    if ( (val2&0xf)<=7 )
		off = -8+(val2&0xf);
	    else
		off = -7+(val2&0xf);
	    off *= 64 / (1L << exc->GS.delta_shift);
	    scrprintf(&scr,"Pops: %d (cvt index (%d,%.2f))", val1,
		    exc->cvt[val1],exc->cvt[val1]/64.0 ); 
	    scrprintf(&scr,"Pops: %d => at %d ppem, change by %.2f",
		val2, exc->GS.delta_base+(base-1)*16+((val2>>4)&0xf),
		off/64.0 ); 
	}
	scrprintf(&scr, "DeltaBase: %d", exc->GS.delta_base ); 
	scrprintf(&scr, "DeltaShift: %d", exc->GS.delta_shift ); 
      break;
      case 0x78: case 0x79:
	scrprintf(&scr,operator==0x78?" Jump Relative (to here) on True":" Jump Relative (to here) on False");
	val1 = exc->stack[exc->top-1];
	val2 = exc->stack[exc->top-2];
	scrprintf(&scr,"Pops: %d (condition)", val1 ); 
	scrprintf(&scr,"Pops: %d (byte offset)", val2 ); 
      break;
      case 0x18: case 0x19:
      case 0x3D:
      case 0x76: case 0x77: case 0x7a: case 0x7c: case 0x7d:
	scrprintf(&scr, operator==0x7a?" set Rounding off":
			operator==0x7c?" set Rounding up to grid":
			operator==0x77?" set Rounding to Super 45":
			operator==0x7d?" set Rounding down to grid":
			operator==0x18?" set Rounding to grid":
			operator==0x3D?" set Rounding to double grid":
			operator==0x19?" set Rounding to half grid":
			  " set Rounding to Super");
	scrprintf(&scr,"Sets: Rounding state");
      break;
      case 0x7e:
	scrprintf(&scr," Set Angle Weight (obsolete)"); 
	scrprintf(&scr,"Pops: an ignored value"); 
      break;
      case 0x7f:
	scrprintf(&scr," Adjust Angle"); 
	scrprintf(&scr,"Pops: %d (point)", exc->stack[exc->top-1]); 
      break;
      case 0x80:
	scrprintf(&scr," Flip Points"); 
	scrprintf(&scr, "loop: %ld", exc->GS.loop ); 
	for ( val1=1; val1<=exc->GS.loop && val1<exc->top; ++val1 ) {
	    val2 = exc->stack[exc->top-val1];
	    scrprintf(&scr,"Pop%d: %d (point (%.2f,%.2f))", val1, val2,
		    exc->zp0.cur[val2].x/64.0,exc->zp0.cur[val2].y/64.0 ); 
	    scrprintf(&scr," was %s-curve, becomes %s-curve",
		    exc->pts.tags[val2] & FT_CURVE_TAG_ON ? "on" : "off",
		    exc->pts.tags[val2] & FT_CURVE_TAG_ON ? "off" : "on" ); 
	}
	scrprintf(&scr," (in zone from zp0: %s)", exc->GS.gep0?"Normal":"Twilight" );
      break;
      case 0x81: case 0x82:
	val2 = exc->stack[exc->top-1];
	val1 = exc->stack[exc->top-2];
	scrprintf(&scr,operator==0x81?" Flip point range On":" Flip point range Off");

	for ( i=val2; i<=val2; ++i ) {
	    scrprintf(&scr," change point %d (%.2f,%.2f)", i,
		    exc->zp0.cur[i].x/64.0,exc->zp0.cur[i].y/64.0 ); 
	    scrprintf(&scr," was %s-curve, becomes %s-curve",
		    exc->pts.tags[val2] & FT_CURVE_TAG_ON ? "on" : "off",
		    operator==0x82 ? "off" : "on" ); 
	}
	scrprintf(&scr," (in zone from zp0: %s)", exc->GS.gep0?"Normal":"Twilight" );
      break;
      case 0x85:
	scrprintf(&scr," SCANCTRL Set dropout control" );
	val1 = exc->stack[exc->top-1];
	scrprintf(&scr,"Pops: %d (flags)",val1 );
	if ( val1==0 )
	    scrprintf(&scr,"Turn off dropout control");
	else {
	    if ( val1&0x100 ) {
		if ( (val1&0xff)==0xff )
		    scrprintf(&scr,"set dropout control for all ppem");
		else
		    scrprintf(&scr,"set dropout control for ppem <= %d", (val1&0xff));
	    }
	    if ( val1&0x800 ) {
		if ( (val1&0xff)==0xff )
		    scrprintf(&scr,"<I can't figure this combination out>");
		else
		    scrprintf(&scr,"unset dropout control unless ppem <= %d", (val1&0xff));
	    }
	    if ( val1&0x200 )
		scrprintf(&scr,"set dropout control if glyph rotated");
	    if ( val1&0x1000 )
		scrprintf(&scr,"unset dropout control unless glyph rotated");
	    if ( val1&0x400 )
		scrprintf(&scr,"set dropout control if glyph stretched");
	    if ( val1&0x2000 )
		scrprintf(&scr,"unset dropout control unless glyph stretched");
	}
      break;
      case 0x06: case 0x07:
      case 0x08: case 0x09:
      case 0x86: case 0x87:
	scrprintf(&scr,operator<0x8?" Sets projection vector from line":
			operator<0x86?" Sets freedom vector from line":
			" Set dual projection vector from line");
	scrprintf(&scr,(operator&1)?"orthogonal to line":"parallel to line");
	val1 = exc->stack[exc->top-2];
	val2 = exc->stack[exc->top-1];
	scrprintf(&scr,"Pops: %d (point (%.2f,%.2f))", val1,
		exc->zp1.cur[val1].x/64.0,exc->zp1.cur[val1].y/64.0 ); 
	scrprintf(&scr," (in zone from zp1: %s)", exc->GS.gep1?"Normal":"Twilight" );
	scrprintf(&scr,"Pops: %d (point (%.2f,%.2f))", val2,
		exc->zp2.cur[val2].x/64.0,exc->zp2.cur[val2].y/64.0 ); 
	scrprintf(&scr," (in zone from zp2: %s)", exc->GS.gep2?"Normal":"Twilight" );
	if ( operator>=0x86 ) {
	    scrprintf(&scr,"Sets: Project vector based on current positions" );
	    scrprintf(&scr,"Sets: Dual proj vector based on original positions" );
	} else
	    scrprintf(&scr,"Sets: %s vector", operator<8?"Projection":"Freedom" );
      break;
      case 0x88:
	val1 = exc->stack[exc->top-1];
	scrprintf(&scr," Get Information"); 
	scrprintf(&scr, "Pops: %d %s%s%s%s%s%s%s%s%s%s", val1,
		val1&(1<<0) ? "version (result in bits 0-7) " : "",
		val1&(1<<1) ? "rotated (result in bit 8)" : "",
		val1&(1<<2) ? "stretched (result in bit 9)" : "",
		val1&(1<<3) ? "Undocumented Apple ?variations? (result in bit 10)" : "",
		val1&(1<<4) ? "Undocumented Apple ?vertical metrics? (result in bit 11)" : "",
		val1&(1<<5) ? "greyscale (result in bit 12)" : "", 
		val1&(1<<6) ? "ClearType (result in bit 13)" : "", 
		val1&(1<<7) ? "CT widths compat (result in bit 14)" : "", 
		val1&(1<<8) ? "CT symetrical smoothing (result in bit 15)" : "",
		val1&(1<<9) ? "CT processes in BGR(1) or RGB(0) (result in bit 16)" : "");
	if ( val1&1 ) {
	    scrprintf(&scr, "  Versions: 1 => Mac OS 6" );
	    scrprintf(&scr, "            2 => Mac OS 7" );
	    scrprintf(&scr, "            3 => Win 3.1" );
	    scrprintf(&scr, "            33=> Win rasterizer 1.5" );
	    scrprintf(&scr, "            34=> Win rasterizer 1.6" );
	    scrprintf(&scr, "            35=> Win rasterizer 1.7" );
	    scrprintf(&scr, "            37=> Win rasterizer 1.8" );
	    scrprintf(&scr, "            38=> Win rasterizer 1.9" );
	}
	scrprintf(&scr,"Pushes: result"); 
	scrprintf(&scr,"FreeType returns: %s%s%s%s",
	    (val1&1) ? "(Win 1.7) | ": "",
	    (val1&2) ? exc->tt_metrics.rotated ? "(rotated) | ": "(not rotated) | " : "",
	    (val1&4) ? exc->tt_metrics.stretched ? "(stretched) | ": "(not stretched) | " : "",
	    (val1&32) ? exc->grayscale ? "(grey scale)": "(black/white)" : ""
	); 
      break;
      case 0x89:
	scrprintf(&scr," Instruction Definition"); 
	val1 = exc->stack[exc->top-1];
	scrprintf(&scr,"Pops: %d (opcode)", val1 ); 
      break;
      case 0x8a:
	scrprintf(&scr," Roll top three stack elements"); 
      break;
      case 0x8D:
	scrprintf(&scr," SCANTYPE Set dropout control" );
	val1 = exc->stack[exc->top-1];
	scrprintf(&scr,"Pops: %d (mode)",val1 );
	if ( val1==0 )
	    scrprintf(&scr,"simple dropout control scan conversion including stubs (rules 1,2,3)");
	else if ( val1==1 )
	    scrprintf(&scr,"simple dropout control scan conversion excluding stubs (rules 1,2,4)");
	else if ( val1==2 || val1==3 || val1==6 || val1==7 )
	    scrprintf(&scr,"fast scan conversion; dropout control turned off (rule 1,2)");
	else if ( val1==4 )
	    scrprintf(&scr,"smart dropout control scan conversion including stubs (rule 1,2,5)");
	else if ( val1==5 )
	    scrprintf(&scr,"smart dropout control scan conversion excluding stubs (rule 1,2,6)");
	else
	    scrprintf(&scr,"*** Unknown mode ***");
      break;
      case 0x8E:
	val2 = exc->stack[exc->top-1];
	val1 = exc->stack[exc->top-2];
	scrprintf(&scr," Instruction Control"); 
	scrprintf(&scr,"Pops: %d (selector)", val1 ); 
	scrprintf(&scr,"Pops: %d (value)", val1 ); 
	if ( val1==1 )
	    scrprintf(&scr, " => grid fitting %s", val2 ? "inhibited" : "normal" );
	else if ( val1==2 )
	    scrprintf(&scr, " => cvt parameters %s", val2 ? "ignored" : "normal" );
	else
	    scrprintf(&scr, "Unknown selector");
      break;
    }
return( scr.lines );
}

static void dvgloss_scroll(DebugView *dv,struct sbevent *sb) {
    int newpos = dv->cvt_offtop;
    GRect size;
    int min, max, page;
    extern int _GScrollBar_Width;

    GScrollBarGetBounds(dv->glosssb,&min,&max,&page);
    GDrawGetSize(dv->gloss,&size);
    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= size.height/dv->ii.fh;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
        newpos += size.height/dv->ii.fh;
      break;
      case et_sb_bottom:
        newpos = max-size.height/dv->ii.fh;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos>max-size.height/dv->ii.fh )
        newpos = max-size.height/dv->ii.fh;
    if ( newpos<0 ) newpos =0;
    if ( newpos!=dv->gloss_offtop ) {
	int diff = newpos-dv->gloss_offtop;
	dv->gloss_offtop = newpos;
	GScrollBarSetPos(dv->glosssb,dv->gloss_offtop);
	size.x = size.y = 0;
	size.width -= GDrawPointsToPixels(dv->gloss,_GScrollBar_Width);
	GDrawScroll(dv->gloss,&size,0,diff*dv->ii.fh);
    }
}

static void DVGlossExposeSize(GWindow gw,DebugView *dv,GEvent *event) {
    int len = DVGlossExpose(gw,dv,event);
    GRect size;
    int min, max, page, offtop;

    GScrollBarGetBounds(dv->glosssb,&min,&max,&page);
    GGadgetGetSize(dv->glosssb,&size);
    size.height /= dv->ii.fh;

    if ( len!=max || page!=size.height ) {
	GScrollBarSetBounds(dv->glosssb,0,len,size.height);
	offtop = dv->gloss_offtop;
	if ( offtop+size.height > len )
	    offtop = len-size.height;
	if ( offtop < 0 )
	    offtop = 0;
	if ( offtop!=dv->gloss_offtop ) {
	    dv->gloss_offtop = offtop;
	    GScrollBarSetPos(dv->glosssb,dv->gloss_offtop);
	    DVGlossExpose(gw,dv,event);
	}
    }
}

static int dvgloss_e_h(GWindow gw, GEvent *event) {
    DebugView *dv = (DebugView *) GDrawGetUserData(gw);
    GRect r,g;
    extern int debug_wins;

    if ( dv==NULL )
return( true );

    switch ( event->type ) {
      case et_expose:
	DVGlossExposeSize(gw,dv,event);
      break;
      case et_char:
return( DVChar(dv,event));
      break;
      case et_controlevent:
	switch ( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    dvgloss_scroll(dv,&event->u.control.u.sb);
	  break;
	}
      break;
      case et_resize:
	GDrawGetSize(gw,&r);
	GGadgetGetSize(dv->glosssb,&g);
	GGadgetMove(dv->glosssb,r.width-g.width,0);
	GGadgetResize(dv->glosssb,g.width,r.height);
	GDrawRequestExpose(dv->gloss,NULL,false);
      break;
      case et_close:
	GDrawDestroyWindow(dv->gloss);
	debug_wins &= ~dw_gloss;
      break;
      case et_destroy:
	dv->gloss = NULL;
      break;
      case et_mouseup: case et_mousedown:
      case et_mousemove:
	GGadgetEndPopup();
      break;
    }
return( true );
}

void DVCreateGloss(DebugView *dv) {
    GWindowAttrs wattrs;
    GRect pos;
    GGadgetData gd;
    extern int _GScrollBar_Width;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
    wattrs.utf8_window_title = _("Instruction Gloss (TrueType)");
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,230)); pos.height = 169;
    pos.x = CVXPos(dv,143,pos.width); pos.y = 302;
    dv->gloss = GDrawCreateTopWindow(NULL,&pos,dvgloss_e_h,dv,&wattrs);

    memset(&gd,0,sizeof(gd));

    gd.pos.y = 0; gd.pos.height = pos.height;
    gd.pos.width = GDrawPointsToPixels(dv->gloss,_GScrollBar_Width);
    gd.pos.x = pos.width-gd.pos.width;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_sb_vert;
    dv->glosssb = GScrollBarCreate(dv->gloss,&gd,dv);

    GDrawSetVisible(dv->gloss,true);
}

/* ************************************************************************** */
/*  Variant of the gloss window: Mark the points to show what the next        */
/*  instruction will do to them. (Change (usually move) them, or use them     */
/*  or ignore them                                                            */
/* ************************************************************************** */

static SplinePoint *FindPoint(SplineSet *ss,int ptnum) {
    SplineSet *spl;
    SplinePoint *sp;

    for ( spl=ss; spl!=NULL ; spl=spl->next ) {
	for ( sp = spl->first; ; ) {
	    if ( sp->ttfindex==ptnum || sp->nextcpindex==ptnum )
return( sp );
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
    }
return( NULL );
}

static void SetBasisPoint(SplineSet *ss, int ptnum) {
    SplinePoint *sp = FindPoint(ss,ptnum);
    if ( sp==NULL )
return;
    /* A roundx point or flexx nextcp means this is the reference */
    /*  point, etc. */
    if ( sp->ttfindex==ptnum )
	sp->roundx = true;
    else
	sp->flexx = true;
}

static void SetChangingPoint(SplineSet *ss, int ptnum) {
    SplinePoint *sp = FindPoint(ss,ptnum);
    if ( sp==NULL )
return;
    /* I reuse these flags. A "selected" point is one which will be */
    /*  moved by the instruction. A "flexy"ed point means its nextcp */
    /*  will be moved by the next instruction */
    if ( sp->ttfindex==ptnum )
	sp->selected = true;
    else
	sp->flexy = true;
}

void DVMarkPts(DebugView *dv,SplineSet *ss) {
    TT_ExecContext exc = DebuggerGetEContext(dv->dc);
    long changing_point, basis_point;
    int i,cnt,top;
    int operator;
    SplineSet *spl;
    SplinePoint *sp;

    for ( spl=ss; spl!=NULL ; spl=spl->next ) {
	for ( sp = spl->first; ; ) {
	    sp->selected = false;
	    sp->roundx = sp->roundy = false;
	    sp->flexx = sp->flexy = false;
	    /* I reuse these flags. A "selected" point is one which will be */
	    /*  moved by the instruction. A "flexy"ed point means its nextcp */
	    /*  will be moved by the next instruction */
	    /* A roundx point or flexx nextcp means this is the reference */
	    /*  point, etc. */
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
    }

    if ( exc==NULL )
return;		/* Not running */
    if ( exc->IP>=exc->codeSize || exc->code==NULL )
return;		/* At end */

    operator = ((uint8 *) exc->code)[exc->IP];
    changing_point = -1;
    basis_point = -1;

    if ( operator>=0xc0 && operator <= 0xdf ) {
        /* MDRP */
	if ( exc->GS.gep1 )	/* No good way to mark twilight points, so only mark normal ones */
	    changing_point = exc->stack[exc->top-1];
	if ( exc->GS.gep0 )
	    basis_point = exc->GS.rp0;
    } else if ( operator>=0xe0 && operator <= 0xff ) {
        /* MIRP */
	if ( exc->GS.gep1 )	/* No good way to mark twilight points, so only mark normal ones */
	    changing_point = exc->stack[exc->top-2];
	if ( exc->GS.gep0 )
	    basis_point = exc->GS.rp0;
    } else if ( operator>=0xb0 && operator <= 0xbf ) {
	/* Push */
    } else switch ( operator ) {
      case 0xf:
        /* Moves point to intersection of two lines */
	if ( exc->GS.gep2 )	/* No good way to mark twilight points, so only mark normal ones */
	    changing_point = exc->stack[exc->top-5];
	if ( exc->GS.gep0 ) {
	    SetBasisPoint(ss,exc->stack[exc->top-4]);
	    SetBasisPoint(ss,exc->stack[exc->top-3]);
	}
	if ( exc->GS.gep1 ) {
	    SetBasisPoint(ss,exc->stack[exc->top-2]);
	    SetBasisPoint(ss,exc->stack[exc->top-1]);
	}
      break;
      case 0x10: case 0x11: case 0x12:
	/* Set Reference Point ? */
	basis_point = exc->stack[exc->top-1];
      break;
      case 0x27:
	/* Align Points */
	if ( exc->GS.gep0 )
	    SetChangingPoint(ss,exc->stack[exc->top-1]);
	if ( exc->GS.gep1 )
	    SetChangingPoint(ss,exc->stack[exc->top-2]);
      break;
      case 0x28:
	/* Untouch point */
	if ( exc->GS.gep0 )
	    changing_point = exc->stack[exc->top-1];
      break;
      case 0x2E:
	/* Touch point */
	if ( exc->GS.gep0 )
	    changing_point = exc->stack[exc->top-1];
      break;
      case 0x2F:
	/* Round and Touch point */
	if ( exc->GS.gep0 )
	    changing_point = exc->stack[exc->top-1];
      break;
      case 0x30:
	/* Interpolate Untouched Points in y */
	if ( exc->GS.gep2 ) {
	    TT_GlyphZoneRec *r = &exc->pts;
	    for ( spl=ss; spl!=NULL ; spl=spl->next ) {
		for ( sp = spl->first; ; ) {
		    if ( sp->ttfindex<r->n_points &&
			    !(r->tags[sp->ttfindex]&FT_Curve_Tag_Touch_Y) )
			sp->selected = true;
		    if ( sp->nextcpindex<r->n_points &&
			    !(r->tags[sp->nextcpindex]&FT_Curve_Tag_Touch_Y) )
			sp->flexy = true;
		    /* I reuse these flags. A "selected" point is one which will be */
		    /*  moved by the instruction. A "flexy"ed point means its nextcp */
		    /*  will be moved by the next instruction */
		    if ( sp->next==NULL )
		break;
		    sp = sp->next->to;
		    if ( sp==spl->first )
		break;
		}
	    }
	}
      break;
      case 0x31:
	/* Interpolate Untouched Points in x */
	if ( exc->GS.gep2 ) {
	    TT_GlyphZoneRec *r = &exc->pts;
	    for ( spl=ss; spl!=NULL ; spl=spl->next ) {
		for ( sp = spl->first; ; ) {
		    if ( sp->ttfindex<r->n_points &&
			    !(r->tags[sp->ttfindex]&FT_Curve_Tag_Touch_X) )
			sp->selected = true;
		    if ( sp->nextcpindex<r->n_points &&
			    !(r->tags[sp->nextcpindex]&FT_Curve_Tag_Touch_X) )
			sp->flexy = true;
		    /* I reuse these flags. A "selected" point is one which will be */
		    /*  moved by the instruction. A "flexy"ed point means its nextcp */
		    /*  will be moved by the next instruction */
		    if ( sp->next==NULL )
		break;
		    sp = sp->next->to;
		    if ( sp==spl->first )
		break;
		}
	    }
	}
      break;
      case 0x32: case 0x33:
        /* Shift point by amount ref point shifted */
	if ( operator==0x33 ) {
	    if ( exc->GS.gep0 )
		basis_point = exc->GS.rp1;
	} else {
	    if ( exc->GS.gep1 )
		basis_point = exc->GS.rp2;
	}
	if ( exc->GS.gep2 )
	    for ( i=1; i<=exc->GS.loop && i<exc->top; ++i ) {
		SetChangingPoint(ss,exc->stack[exc->top-i]);
	    }
      break;
      case 0x34: case 0x35:
        /* Shift contour by amount ref point shifted */
	if ( operator==0x35 ) {
	    if ( exc->GS.gep0 )
		basis_point = exc->stack[exc->GS.rp1];
	} else {
	    if ( exc->GS.gep1 )
		basis_point = exc->stack[exc->GS.rp2];
	}
	if ( exc->GS.gep2 ) {
	    for ( spl=ss, i=exc->stack[exc->top-1]; i>0 && spl!=NULL; --i, spl=spl->next );
	    if ( spl!=NULL ) {
		for ( sp = spl->first; ; ) {
		    if ( sp->ttfindex<0xfff0 )
			sp->selected = true;
		    if ( sp->nextcpindex<0xfff0 )
			sp->flexy = true;
		    /* I reuse these flags. A "selected" point is one which will be */
		    /*  moved by the instruction. A "flexy"ed point means its nextcp */
		    /*  will be moved by the next instruction */
		    if ( sp->next==NULL )
		break;
		    sp = sp->next->to;
		    if ( sp==spl->first )
		break;
		}
	    }
	}
      break;
      case 0x36: case 0x37:
	if ( operator==0x37 ) {
	    if ( exc->GS.gep0 )
		basis_point = exc->stack[exc->GS.rp1];
	} else {
	    if ( exc->GS.gep1 )
		basis_point = exc->stack[exc->GS.rp2];
	}
	if ( exc->stack[exc->top-1] ) {
	    for ( spl=ss; spl!=NULL; spl=spl->next ) {
		for ( sp = spl->first; ; ) {
		    if ( sp->ttfindex<0xfff0 )
			sp->selected = true;
		    if ( sp->nextcpindex<0xfff0 )
			sp->flexy = true;
		    /* I reuse these flags. A "selected" point is one which will be */
		    /*  moved by the instruction. A "flexy"ed point means its nextcp */
		    /*  will be moved by the next instruction */
		    if ( sp->next==NULL )
		break;
		    sp = sp->next->to;
		    if ( sp==spl->first )
		break;
		}
	    }
	}
      break;
      case 0x38:
        /* Shift point by pixel amount */
	if ( exc->GS.gep2 ) {
	    for ( i=2; i<=exc->GS.loop+1 && i<exc->top; ++i )
		SetChangingPoint(ss,exc->stack[exc->top-i]);
	}
      break;
      case 0x39:
	/* Interpolated Point */
	if ( exc->GS.gep0 )
	    SetBasisPoint(ss,exc->GS.rp1);
	if ( exc->GS.gep1 )
	    SetBasisPoint(ss,exc->GS.rp2);
	if ( exc->GS.gep2 ) {
	    for ( i=1; i<=exc->GS.loop+1 && i<exc->top; ++i )
		SetChangingPoint(ss,exc->stack[exc->top-i]);
	}
      break;
      case 0x3A: case 0x3B:
	/* MSIRP Move Stack Indirect Relative Point */
	if ( exc->GS.gep1 )
	    changing_point = exc->stack[exc->top-2];
	if ( exc->GS.gep0 )
	    basis_point = exc->GS.rp0;
      break;
      case 0x3C:
	/* Align to Reference Point */
	if ( exc->GS.gep0 )
		basis_point = exc->GS.rp0;
	if ( exc->GS.gep1 )
	    for ( i=1; i<=exc->GS.loop && i<exc->top; ++i ) {
		SetChangingPoint(ss,exc->stack[exc->top-i]);
	    }
      break;
      case 0x3E: case 0x3F:
	/* MIAP Move Indirect Absolute Point */
	if ( exc->GS.gep0 )
	    changing_point = exc->stack[exc->top-2];
      break;
      case 0x46: case 0x47:
	/* Get current/original point coord projected on projection vector */
	if ( exc->GS.gep2 )
	    basis_point = exc->stack[exc->top-1];
      break;
      case 0x48:
	/* Sets coordinate from stack using proj & free vectors */
	if ( exc->GS.gep2 )
	    changing_point = exc->stack[exc->top-2];
      break;
      case 0x49: case 0x4A:
	/* Measure Distance */
	if ( exc->GS.gep0 )
	    SetBasisPoint( ss, exc->stack[exc->top-1] );
	if ( exc->GS.gep1 )
	    SetBasisPoint( ss, exc->stack[exc->top-2] );
      break;
      case 0x5D: case 0x71: case 0x72:
      case 0x73: case 0x74: case 0x75:
        /* Delta Point */
	cnt = exc->stack[exc->top-1];
	for ( i=0; i<cnt; ++i ) {
	    if ( 2*i+3 > exc->top )
	break;
	    if ( exc->GS.gep0 )
		SetChangingPoint(ss, exc->stack[exc->top-2-2*i]);
	}
      break;
      case 0x80:
	/* Flip Points */
	if ( exc->GS.gep0 )
	    for ( i=1; i<=exc->GS.loop && i<exc->top; ++i ) {
		SetChangingPoint(ss,exc->stack[exc->top-i]);
	    }
      break;
      case 0x81: case 0x82:
	i = exc->stack[exc->top-1];
	top = exc->stack[exc->top-2];
	if ( exc->GS.gep0 )
	    for ( ; i<=top; ++i ) {
		SetChangingPoint(ss,exc->stack[exc->top-i]);
	    }
      break;
      case 0x06: case 0x07:
      case 0x08: case 0x09:
      case 0x86: case 0x87:
        /* Sets vector from line */
	if ( exc->GS.gep1 )
	    SetBasisPoint(ss,exc->stack[exc->top-2]);
	if ( exc->GS.gep2 )
	    SetBasisPoint(ss,exc->stack[exc->top-1]);
      break;
      default:
	/* Many instructions don't refer to points */
      break;
    }
    if ( changing_point!=-1 )
	SetChangingPoint(ss,changing_point);
    if ( basis_point!=-1 )
	SetBasisPoint(ss,basis_point);
}

#else
void DVCreateGloss(DebugView *dv) {
}

void DVMarkPts(DebugView *dv,SplineSet *ss) {
}
#endif /* Has Debugger */
