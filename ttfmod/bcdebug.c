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
#include "ttfinstrs.h"
#include <math.h>

#if TT_CONFIG_OPTION_BYTECODE_DEBUG
static int doround(struct ttfstate *state, int val) {
    double dval = val/64.0;
    switch ( state->rounding ) {
      case rnd_half: dval = rint(dval-.5)+.5; break;
      case rnd_grid: dval = rint(dval); break;
      case rnd_dbl: dval = rint(2*dval)/2; break;
      case rnd_down: dval = floor(dval); break;
      case rnd_up: dval = ceil(dval); break;
      case rnd_off: dval = dval; break;
      case rnd_sround:
      case rnd_sround45:
	/* er... this is hardish, is it ever used? */
	fprintf(stderr, "Don't support sround yet\n" );
	dval = dval;
      break;
      default:
	fprintf(stderr, "Unknown rounding mode %d\n", state->rounding );
    }
return( (int) (dval*64.0) );
}

static int pop(struct ttfstate *state,struct ttfargs *args) {
    int val;

    if ( state->sp<=0 ) {
	fprintf( stderr, "Attempt to pop something off the stack when it is empty in %s\n",
		instrs[*(state->pc-1)]);
	val = 0;
    } else
	val = state->stack[--state->sp];
    if ( args!=NULL ) {
	if ( !(args->used&ttf_sp0) ) { args->spvals[0] = val; args->used |= ttf_sp0; }
	else if ( !(args->used&ttf_sp1) ) { args->spvals[1] = val; args->used |= ttf_sp1; }
	else if ( !(args->used&ttf_sp2) ) { args->spvals[2] = val; args->used |= ttf_sp2; }
	else if ( !(args->used&ttf_sp3) ) { args->spvals[3] = val; args->used |= ttf_sp3; }
    }
return( val );
}

static void push(struct ttfstate *state,int val,struct ttfargs *args) {
    if ( args!=NULL ) {
	if ( !(args->used&ttf_pushed) ) {
	    args->used |= ttf_pushed;
	    args->pushed = val;
	} else
	    args->used |= ttf_pushedmore;
    }
    if ( state->sp>=state->stack_max ) {
	fprintf( stderr, "Stack grows bigger than indicated in maxp table\n" );
	MaxPSetStack(state->maxp,state->sp+1);
	state->stack = grealloc(state->stack,(state->sp+1)*sizeof(int32));
	state->stack_max = state->sp+1;
    }
    state->stack[state->sp++] = val;
}

static void AddAction(struct ttfstate *state, int pt,int base, int interp, int offset,
	int round, int mind, int cvt) {
    struct ttfactions *act = gcalloc(1,sizeof(struct ttfactions));
    struct ttfactions *test, *prev;
    int c;

    act->pnum = pt;
    act->basedon = base;
    act->interp = interp;
    act->distance = offset;
    act->cvt_entry = cvt;
    act->rounded = round!=0?1:0;
    act->min = mind!=0?1:0;
    act->instr = state->pc-1;
    act->freedom = state->freedom;

    c = (act->freedom.x!=0?1:0) + (act->freedom.y!=0?2:0);
    for ( prev=NULL, test=state->acts; test!=NULL && (pt>test->pnum ||
	    (pt==test->pnum && c>=(test->freedom.x!=0?1:0) + (test->freedom.y!=0?2:0)));
	    prev = test, test = test->acts );
    if ( prev==NULL )
	state->acts = act;
    else
	prev->acts = act;
    act->acts = test;
}

static void AdjustPointBy(struct ttfstate *state,int pt,int zone,int32 offset) {
    /* offset is 26.6 */
    double projection;

    if ( pt<0 || pt>= state->zones[zone].point_cnt )
	fprintf(stderr, "Point out of range: %d\n", pt );
    else if ( (projection=(state->projection.x*state->freedom.x)+(state->projection.y*state->freedom.y))>= -.00001 && projection<.00001 )
	fprintf(stderr, "Projection and freedom vectors are orthogonal\n" );
    else {
	if ( zone==1 )
	    AddAction(state,pt,pt,-1,offset,false,false,-1);
#if TT_CONFIG_OPTION_BYTECODE_INTERPRETER
/* Manipulating a point's position is protected */
	offset /= projection;
	state->zones[zone].moved[pt].x += offset*state->freedom.x;
	state->zones[zone].moved[pt].y += offset*state->freedom.y;
	if ( state->freedom.x!=0 )
	    state->zones[zone].flags[pt] |= pt_xtouched;
	if ( state->freedom.y!=0 )
	    state->zones[zone].flags[pt] |= pt_ytouched;
#endif
    }
}

static void SetPointRelativeTo(struct ttfstate *state,int pt,int zone,
	int rpt, int rzone, int32 offset,
	int round,int mind, int cvt) {
    /* offset is 26.6 */
    double projection;
#if TT_CONFIG_OPTION_BYTECODE_INTERPRETER
    double other;
    int pos;
#endif

    if ( pt<0 || pt>= state->zones[zone].point_cnt )
	fprintf(stderr, "Point out of range: %d\n", pt );
    else if ( (projection=(state->projection.x*state->freedom.x)+(state->projection.y*state->freedom.y))>= -.00001 && projection<.00001 )
	fprintf(stderr, "Projection and freedom vectors are orthogonal\n" );
    else {
	if ( zone==1 )
	    AddAction(state,pt,rpt,-1,offset,round,mind,cvt);
#if TT_CONFIG_OPTION_BYTECODE_INTERPRETER
/* Manipulating a point's position is protected */
	pos = state->zones[rzone].moved[rpt].x*state->projection.x +
		state->zones[rzone].moved[rpt].y*state->projection.y + offset;
	pos /= projection;
	other = state->zones[zone].moved[pt].x*state->freedom.y-state->zones[zone].moved[pt].y*state->freedom.x;
	state->zones[zone].moved[pt].x = pos*state->freedom.x + other*state->freedom.y;
	state->zones[zone].moved[pt].y = pos*state->freedom.y - other*state->freedom.x;
	if ( state->freedom.x!=0 )
	    state->zones[zone].flags[pt] |= pt_xtouched;
	if ( state->freedom.y!=0 )
	    state->zones[zone].flags[pt] |= pt_ytouched;
#endif
    }
}

static void InterpolatePointTo(struct ttfstate *state,int pt,int zone,int32 pos,
	int round,int cvt,int p1, int p2) {
    /* pos is 26.6 */
    /* Set the point's position so that it's projection on the pv is pos */
    double projection;
#if TT_CONFIG_OPTION_BYTECODE_INTERPRETER
    double other;
#endif

    if ( pt<0 || pt>= state->zones[zone].point_cnt )
	fprintf(stderr, "Point out of range: %d\n", pt );
    else if ( (projection=(state->projection.x*state->freedom.x)+(state->projection.y*state->freedom.y))>= -.00001 && projection<.00001 )
	fprintf(stderr, "Projection and freedom vectors are orthogonal\n" );
    else {
#if TT_CONFIG_OPTION_BYTECODE_INTERPRETER
	if ( zone==1 )
	    AddAction(state,pt,p1,p2,pos,round,false,cvt);
/* Manipulating a point's position is protected */
	pos /= projection;
	other = state->zones[zone].moved[pt].x*state->freedom.y-state->zones[zone].moved[pt].y*state->freedom.x;
	state->zones[zone].moved[pt].x = pos*state->freedom.x + other*state->freedom.y;
	state->zones[zone].moved[pt].y = pos*state->freedom.y - other*state->freedom.x;
	if ( state->freedom.x!=0 )
	    state->zones[zone].flags[pt] |= pt_xtouched;
	if ( state->freedom.y!=0 )
	    state->zones[zone].flags[pt] |= pt_ytouched;
#endif
    }
}

static void SetPointTo(struct ttfstate *state,int pt,int zone,int32 pos,
	int round,int cvt) {
    InterpolatePointTo(state,pt,zone,pos,round,cvt,-1,-1);
}

static void TtfExecuteInstrs(struct ttfstate *state,uint8 *data,int len) {
    int instr, n, val, val2, val3, i;
    struct ttfargs *args;
    double dval, dval2;

    state->pc = data;
    state->end_pc = data+len;

    while ( true ) {
	while ( state->pc>=state->end_pc ) {
	    if ( state->retsp==0 )
return;
	    if ( --state->callcnt >= 0 )
		state->pc = state->startcall;
	    else {
		if ( state->startcall==NULL )
		    state->in_instr = false;
		--state->retsp;
		state->pc = state->returns[state->retsp];
		state->end_pc = state->ends[state->retsp];
		state->callcnt = state->cnts[state->retsp];
		state->startcall = state->starts[state->retsp];
	    }
	}
	instr= *state->pc++;
	if ( state->idefs!=NULL && state->idefs[instr].data!=NULL &&
		!state->in_instr ) {
	    if ( state->retsp>=SUB_MAX )
		fprintf( stderr, "Instruction/Function calls nested too deeply, ignored (in instruction %x)\n", instr );
	    else {
		state->returns[state->retsp] = state->pc;
		state->ends[state->retsp] = state->end_pc;
		state->cnts[state->retsp] = state->callcnt;
		state->starts[state->retsp] = state->startcall;
		state->pc = state->fdefs[val].data;
		state->end_pc = state->pc + state->fdefs[val].len;
		state->callcnt = -1;
		state->startcall = NULL;
		state->in_instr = true;
		++state->retsp;
		instr= *state->pc++;
	    }
	}
	args = NULL;
	if ( state->retsp==0 && state->args!=NULL ) {
	    args = &state->args[(state->pc-1-data)];
	    if ( args->loopcnt==0 ) {
		args->rp0val = state->rp0;
		args->rp1val = state->rp1;
		args->rp2val = state->rp2;
		args->rp2val = state->rp2;
		if ( state->zp0 ) args->zs |= 1;
		if ( state->zp1 ) args->zs |= 2;
		if ( state->zp2 ) args->zs |= 4;
	    }
	    ++args->loopcnt;
	}

	switch( instr ) {
	  case ttf_pushb: case ttf_pushb+1: case ttf_pushb+2: case ttf_pushb+3:
	  case ttf_pushb+4: case ttf_pushb+5: case ttf_pushb+6: case ttf_pushb+7:
	    n = instr-ttf_pushb+1;
	    for ( i=0; i<n; ++i )
		push(state,*(state->pc++),args);
	  break;
	  case ttf_pushw: case ttf_pushw+1: case ttf_pushw+2: case ttf_pushw+3:
	  case ttf_pushw+4: case ttf_pushw+5: case ttf_pushw+6: case ttf_pushw+7:
	    n = instr-ttf_pushw+1;
	    for ( i=0; i<n; ++i ) {
		val = *(state->pc++)<<8;
		val |= *(state->pc++);
		push(state,(short) val,args);
	    }
	  break;
	  case ttf_npushb:
	    n = *(state->pc++);
	    for ( i=0; i<n; ++i )
		push(state,*(state->pc++),args);
	  break;
	  case ttf_npushw:
	    n = *(state->pc++);
	    for ( i=0; i<n; ++i ) {
		val = *(state->pc++)<<8;
		val |= *(state->pc++);
		push(state,(short) val,args);
	    }
	  break;
/* pops. some we don't implement, others obsolete */
	  case ttf_aa: case ttf_pop: case ttf_sangw: case ttf_debug:
	    pop(state,args);
	  break;
/* unary arithmetric */
	  case ttf_abs: case ttf_ceiling:
	  case ttf_even: case ttf_floor: case ttf_not:
	  case ttf_getinfo:
	  case ttf_neg: case ttf_odd: case ttf_rcvt: case ttf_rs:
	  case ttf_nround: case ttf_nround+1: case ttf_nround+2: case ttf_nround+3:
	  case ttf_round: case ttf_round+1: case ttf_round+2: case ttf_round+3:
	    val = pop(state,args);
	    switch ( instr ) {
	      case ttf_abs:
		if ( val<0 ) val = -val;
	      break;
	      case ttf_ceiling:
		val = ceil(val);
	      break;
	      case ttf_even:
		val = doround(state,val);
		if ( val&64 ) val = 0; else val = 1;
	      break;
	      case ttf_floor:
		val = floor(val);
	      break;
	      case ttf_not:
		val = !val;
	      break;
	      case ttf_nround: case ttf_nround+1: case ttf_nround+2: case ttf_nround+3:
	      break;
	      case ttf_neg:
		val = -val;
	      break;
	      case ttf_odd:
		val = doround(state,val);
		if ( val&64 ) val = 1; else val = 0;
	      break;
	      case ttf_rcvt:
		if ( state->cvt==NULL || val<0 || val>=state->cvt->newlen/2 ) {
		    fprintf( stderr, "Attempt to read cvt out of bounds %d\n", val );
		    val = 0;
		} else
		    val = ((short) state->cvtvals[val])*state->scale;
	      break;
	      /* the white, black, grey distinctions depend on engine characteristics */
	      /*  so to me they are all the same.
	      case ttf_round: case ttf_round+1: case ttf_round+2: case ttf_round+3:
		val = doround(state,val);
	      break;
	      case ttf_rs:
		if ( state->storage==NULL || val<0 || val>=state->store_max ) {
		    fprintf( stderr, "Attempt to read storage out of bounds %d\n" val );
		    val = 0;
		} else
		    val = state->storage[val];
	      break;
	      case ttf_getinfo:
		val2 = 0;
		if ( val&1 )
		    val2 = 0;		/* Engine (interpretter) version */
		if ( val&2 )
		    /* Never stretched, don't set bit 8 */
		if ( val&4 )
		    /* Never rotated, don't set bit 9 */
		val = val2;
	      break;
	    }
	    push(state,val,args);
	  break;
/* binary arithmetric */
	  case ttf_add: case ttf_and: case ttf_div: case ttf_eq: case ttf_gt:
	  case ttf_gteq: case ttf_lt: case ttf_lteq: case ttf_max: case ttf_mul:
	  case ttf_min: case ttf_neq:
	  case ttf_or: case ttf_sub:
	    val = pop(state,args);
	    val2 = pop(state,args);
	    switch ( instr ) {
	      case ttf_add: val += val2; break;
	      case ttf_sub: val = val2-val; break;
	      case ttf_div: val = rint((val2*64.0)/val); break;
	      case ttf_mul: val = rint((val/64.0)*val2); break;
	      case ttf_and: val &= val2; break;
	      case ttf_or: val |= val2; break;
	      case ttf_eq: val = (val==val2); break;
	      case ttf_gt: val = (val>val2); break;
	      case ttf_gteq: val = (val>=val2); break;
	      case ttf_lt: val = (val<val2); break;
	      case ttf_lteq: val = (val<=val2); break;
	      case ttf_neq: val = (val!=val2); break;
	      case ttf_max: if ( val<val2 ) val = val2; break;
	      case ttf_min: if ( val>val2 ) val = val2; break;
	    }
	    push(state,val,args);
	  break;
/* storing values */
	  case ttf_wcvtf: case ttf_wcvtp: case ttf_ws:
	    val = pop(state,args);
	    val2 = pop(state,args);
	    if ( instr==ttf_ws ) {
		if ( val2<0 || val2>65536 )
		    fprintf( stderr, "Attempt to write storage out of bounds %d\n", val2 );
		else {
		    if ( state->storage==NULL || val2>=state->store_max ) {
			MaxPSetStorage(state->maxp,val+1);
			if ( state->storage==NULL )
			    state->storage = gcalloc((val2+1),sizeof(int32));
			else {
			    state->storage = grealloc(state->storage,val2*sizeof(int32));
			    for ( i=state->store_max; i<=val2; ++i )
				state->storage[i] = 0;
			}
		    }
		    state->storage[val2] = val;
		}
	    } else {
		if ( instr==ttf_wcvtp )
		    val = (val/state->scale);
		if ( state->cvt==NULL || val2<0 || val2>=state->cvt->newlen/2 ) {
		    fprintf( stderr, "Attempt to write cvt out of bounds %d\n", val2 );
		} else
		    state->cvtvals[val2] = val;
	    }
	  break;
/* state setters with no values */
	  case ttf_flipoff: case ttf_flipon:
	    state->auto_flip = instr==ttf_flipon;
	  break;
	  case ttf_rdtg:
	    state->rounding = rnd_down;
	  break;
	  case ttf_roff:
	    state->rounding = rnd_off;
	  break;
	  case ttf_rtdg:
	    state->rounding = rnd_dbl;
	  break;
	  case ttf_rtg:
	    state->rounding = rnd_grid;
	  break;
	  case ttf_rthg:
	    state->rounding = rnd_half;
	  break;
	  case ttf_rutg:
	    state->rounding = rnd_up;
	  break;
	  case ttf_sfvtca:
	    state->freedom.x = 0; state->freedom.y = 1;
	  break;
	  case ttf_sfvtca+1:
	    state->freedom.x = 1; state->freedom.y = 0;
	  break;
	  case ttf_sfvtpv:
	    state->freedom = state->projection;
	  break;
	  case ttf_spvtca:
	    state->projection.x = 0; state->projection.y = 1;
	    state->dual = state->projection;
	  break;
	  case ttf_spvtca+1:
	    state->projection.x = 1; state->projection.y = 0;
	    state->dual = state->projection;
	  break;
	  case ttf_svtca:
	    state->projection.x = 0; state->projection.y = 1;
	    state->freedom = state->projection;
	    state->dual = state->projection;
	  break;
	  case ttf_svtca+1:
	    state->projection.x = 1; state->projection.y = 0;
	    state->freedom = state->projection;
	    state->dual = state->projection;
	  break;
/* state setters with one value */
	  case ttf_s45round: case ttf_scanctrl: case ttf_scantype:
	  case ttf_scvtci: case ttf_sdb: case ttf_sds: case ttf_sloop:
	  case ttf_smd: case ttf_sround:
	  case ttf_srp0: case ttf_srp1: case ttf_srp2:
	  case ttf_ssw: case ttf_sswci:
	  case ttf_szp0: case ttf_szp1: case ttf_szp2: case ttf_szps:
	    val = pop(state,args);
	    switch ( instr ) {
	      case ttf_s45round:
		state->rounding = rnd_sround45;
		state->sround_value = val;
	      break;
	      case ttf_scanctrl:
		state->scancontrol = val;
	      break;
	      case ttf_scantype:
		state->scantype = val;
	      break;
	      case ttf_scvtci:
		state->control_value_cutin = val;
	      break;
	      case ttf_sdb:
		state->delta_base = val;
	      break;
	      case ttf_sds:
		state->delta_shift = val;
	      break;
	      case ttf_sloop:
		if ( val<=0 || val>=65536 )
		    fprintf( stderr, "Attempt to set loop counter out of bounds (%d)\n", val );
		else
		    state->loop = val;
	      break;
	      case ttf_smd:
		state->min_distance = val;
	      break;
	      case ttf_sround:
		state->rounding = rnd_sround;
		state->sround_value = val;
	      break;
	      case ttf_srp0:
		if ( val<0 || val>=state->zones[state->zp0].point_cnt )
		    fprintf( stderr, "Attempt to put a number into rp0 which doesn't refer to a point (%d)\n", val );
		else
		    state->rp0 = val;
	      break;
	      case ttf_srp1:
		if ( val<0 || val>=state->zones[state->zp1].point_cnt )
		    fprintf( stderr, "Attempt to put a number into rp1 which doesn't refer to a point (%d)\n", val );
		else
		    state->rp1 = val;
	      break;
	      case ttf_srp2:
		if ( val<0 || val>=state->zones[state->zp2].point_cnt )
		    fprintf( stderr, "Attempt to put a number into rp2 which doesn't refer to a point (%d)\n", val );
		else
		    state->rp2 = val;
	      break;
	      case ttf_ssw:
		state->single_width = val;
	      break;
	      case ttf_sswci:
		state->single_width_cutin = val;
	      break;
	      case ttf_szp0:
		if ( val!=0 && val!=1 )
		    fprintf( stderr, "Invalid zone pointer (szp0)\n");
		else
		    state->zp0 = val;
	      break;
	      case ttf_szp1:
		if ( val!=0 && val!=1 )
		    fprintf( stderr, "Invalid zone pointer (szp1)\n");
		else
		    state->zp1 = val;
	      break;
	      case ttf_szp2:
		if ( val!=0 && val!=1 )
		    fprintf( stderr, "Invalid zone pointer (szp2)\n");
		else
		    state->zp2 = val;
	      break;
	      case ttf_szps:
		if ( val!=0 && val!=1 )
		    fprintf( stderr, "Invalid zone pointer (szps)\n");
		else
		    state->zp0 = state->zp1 = state->zp2 = val;
	      break;
	    }
	  break;
/* state setters with more values */
	  case ttf_instctrl:
	    if ( !state->in_prep )
		fprintf( stderr, "Attempt to execute INSTCTRL when not in 'prep' program\n" );
	    else {
		val = pop(state,args);
		val2 = pop(state,args);
		/* I'm not going to support this. It let's us turn off */
		/*  grid fitting (which would mean this exercise was useless) */
		/*  or turn off the cvt */
	    }
	  break;
	  case ttf_sfvfs:
	    val = pop(state,args);
	    val2 = pop(state,args);
	    dval = val/(double) (1<<14);
	    dval2 = val2/(double) (1<<14);
	    if ( dval*dval+dval2*dval2 < .999 || dval*dval+dval2*dval2>1.001 )
		fprintf( stderr, "Attempt to set freedom vector to something which isn't a unit vector\n\t(%g,%g)\n", dval, dval2);
	    else {
		state->freedom.x = dval2;
		state->freedom.y = dval;
	    }
	  break;
	  case ttf_spvfs:
	    val = pop(state,args);
	    val2 = pop(state,args);
	    dval = val/(double) (1<<14);
	    dval2 = val2/(double) (1<<14);
	    if ( dval*dval+dval2*dval2 < .999 || dval*dval+dval2*dval2>1.001 )
		fprintf( stderr, "Attempt to set projection vector to something which isn't a unit vector\n\t(%g,%g)\n", dval, dval2);
	    else {
		state->projection.x = dval2;
		state->projection.y = dval;
		state->dual = state->projection;
	    }
	  break;
	  case ttf_spvtl: case ttf_spvtl+1:
	  case ttf_sfvtl: case ttf_sfvtl+1:
	  case ttf_sdpvtl: case ttf_sdpvtl+1:
	    val2 = pop(state,args);
	    val = pop(state,args);
	    if ( val2<0 || val2>=state->zones[state->zp2].point_cnt )
		fprintf( stderr, "Point 2 out of bounds in set freedom/projection vector %d\n", val2 );
	    else if ( val<0 || val>=state->zones[state->zp1].point_cnt )
		fprintf( stderr, "Point 1 out of bounds in set freedom/projection vector %d\n", val );
	    else {
		IPoint *pt1, *pt2; double len; BasePoint unit;
		/* It doesn't say whether the moved or original points */
		/*  probably moved */
		pt1 = &state->zones[state->zp1].moved[val];
		pt2 = &state->zones[state->zp2].moved[val2];
		len = sqrt((pt1->x-pt2->x)*(pt1->x-pt2->x) + (pt1->y-pt2->y)*(pt1->y-pt2->y));
		if ( len==0 )
		    fprintf( stderr, "Attempt to set the freedom or projection vector to a 0 length vector\n" );
		else {
		    if ( instr==ttf_spvtl || instr==ttf_sfvtl || instr==ttf_sdpvtl ) {
			unit.x = (pt1->x-pt2->x)/len;
			unit.y = (pt1->y-pt2->y)/len;
		    } else {
			unit.y = (pt1->x-pt2->x)/len;
			unit.x = -(pt1->y-pt2->y)/len;
		    }
		    if ( instr==ttf_sfvtl || instr==ttf_sfvtl+1 )
			state->freedom = unit;
		    else {
			state->projection = unit;
			state->dual = unit;
		    }
		}
	    }
	    /* In the set dual instructions we set the normal projection vector*/
	    /*  using the moved locations, and set an alternate using the old */
	    if ( instr==ttf_sdpvtl || instr==ttf_sdpvtl+1 ) {
		IPoint *pt1 = &state->zones[state->zp1].points[val];
		IPoint *pt2 = &state->zones[state->zp2].points[val2];
		BasePoint unit;
		len = sqrt((pt1->x-pt2->x)*(pt1->x-pt2->x) + (pt1->y-pt2->y)*(pt1->y-pt2->y));
		if ( len==0 )
		    fprintf( stderr, "Attempt to set the dual projection vector to a 0 length vector\n" );
		else {
		    if ( instr==ttf_sdpvtl ) {
			unit.x = (pt1->x-pt2->x)/len;
			unit.y = (pt1->y-pt2->y)/len;
		    } else {
			unit.y = (pt1->x-pt2->x)/len;
			unit.x = -(pt1->y-pt2->y)/len;
		    }
		    state->dual = unit;
		}
	    }
	  break;
/* get state values */
	  case ttf_mppem: case ttf_mps:		/* I make pointsize==pixels per em */
	    push(state,state->cv->show.ppem,args);
	  break;
	  case ttf_gfv:
	    push(state,state->freedom.x*(1<<14),args);	/* 2.14 number */
	    push(state,state->freedom.y*(1<<14),args);
	  break;
	  case ttf_gpv:
	    push(state,state->projection.x*(1<<14),args);
	    push(state,state->projection.y*(1<<14),args);
	  break;
/* get info from the points */
	  case ttf_gc: case ttf_gc+1:
	    val = pop(state,args);
	    if ( val<0 || val>=state->zones[state->zp2].point_cnt )
		fprintf( stderr, "Point out of bounds in GC: %d\n", val );
	    else {
		IPoint *p;
		if ( instr==ttf_gc )	/* original */
		    p = &state->zones[state->zp2].points[val];
		else
		    p = &state->zones[state->zp2].moved[val];
		/* Ok. We should use the dual projection vector if it has been*/
		/*  set and the pv has not be set since. I avoid the issue by */
		/*  setting the dual to the pv whenever the pv is set. So I can*/
		/*  always use the dual */
		push(state,state->dual.x*p->x+state->dual.y*p->y,args);
	    }
	  break;
	  case ttf_md: case ttf_md+1:
	    val2 = pop(state,args);
	    val = pop(state,args);
	    if ( val<0 || val>=state->zones[state->zp0].point_cnt )
		fprintf( stderr, "Point 1 out of bounds in MD: %d\n", val );
	    else if ( val2<0 || val2>=state->zones[state->zp1].point_cnt )
		fprintf( stderr, "Point 2 out of bounds in MD: %d\n", val2 );
	    else {
		/* Marvelous. Apple's docs are contradictory within the space*/
		/* of a few sentences about which measures the grid-fit distance */
		IPoint v;
		if ( instr==ttf_md ) {	/* Probably original */
		    v.x = state->zones[state->zp0].points[val].x - state->zones[state->zp1].points[val2].x;
		    v.y = state->zones[state->zp0].points[val].y - state->zones[state->zp1].points[val2].y;
		} else {
		    v.x = state->zones[state->zp0].moved[val].x - state->zones[state->zp1].moved[val2].x;
		    v.y = state->zones[state->zp0].moved[val].y - state->zones[state->zp1].moved[val2].y;
		}
		/* Ok. We should use the dual projection vector if it has been*/
		/*  set and the pv has not be set since. I avoid the issue by */
		/*  setting the dual to the pv whenever the pv is set. So I can*/
		/*  always use the dual */
		push(state,state->dual.x*v.x+state->dual.y*v.y,args);
	    }
	  break;
/* stack manipulators */
	  case ttf_clear:
	    state->sp = 0;
	  break;
	  case ttf_dup:
	    val = pop(state,args);
	    push(state,val,args);
	    push(state,val,args);
	  break;
	  case ttf_cindex:
	    val = pop(state,args);
	    if ( val<=0 || val>state->sp )
		fprintf( stderr, "Value out of bounds in cindex\n" );
	    else
		push(state,state->stack[state->sp-val],args);
	  break;
	  case ttf_mindex:
	    val = pop(state,args);
	    if ( val<=0 || val>state->sp )
		fprintf( stderr, "Value out of bounds in cindex\n" );
	    else {
		int temp = state->stack[state->sp-val];
		for ( i=val; ; --i )
		    state->stack[state->sp-val] = state->stack[state->sp-val+1];
		state->stack[state->sp-1] = temp;
	    }
	  break;
	  case ttf_roll:
	    val = pop(state,args);
	    val2 = pop(state,args);
	    val3 = pop(state,args);
	    push(state,val2,args);
	    push(state,val,args);
	    push(state,val3,args);
	  break;
	  case ttf_swap:
	    val = pop(state,args);
	    val2 = pop(state,args);
	    push(state,val,args);
	    push(state,val2,args);
	  break;
	  case ttf_depth:
	    push(state,state->sp,args);
	  break;
/* conditional */
	  case ttf_if:
	    while ( state->pc<state->end_pc && *state->pc!=ttf_eif && *state->pc!=ttf_else )
		++state->pc;
	  break;
	  case ttf_else:
	    while ( state->pc<state->end_pc && *state->pc!=ttf_eif )
		++state->pc;
	  break;
	  case ttf_eif:
	      /* No op */
	  break;
/* instruction/function definition & calls */
	  case ttf_fdef:
	    val = pop(state,args);
	    if ( state->in_glyf )
		fprintf( stderr, "Functions may only be defined in the font or cvt programs (fpgm, prep)\n" );
	    else if ( val<=0 || val>=65536 )
		fprintf( stderr, "Attempt to define a function out of bounds: %d\n", val );
	    else {
		if ( val>=state->fdefcnt ) {
		    MaxPSetFDef(state->maxp,val+1);
		    if ( state->fdefs==NULL )
			state->fdefs = gcalloc((val+1),sizeof(struct ifdef));
		    else {
			state->fdefs = grealloc(state->fdefs,(val+1)*sizeof(struct ifdef));
			for ( i=state->fdefcnt; i<=val; ++i )
			    state->fdefs[i].data = NULL;
		    }
		    state->fdefcnt = val+1;
		}
		state->fdefs[val].data = state->pc;
		while ( state->pc<state->end_pc && *state->pc!=ttf_endf );
		    ++state->pc;
		/* spec doesn't allow for nested function defs */
		state->fdefs[val].len = state->pc - state->fdefs[val].data;
		if ( state->pc==state->end_pc )
		    fprintf( stderr, "Missing endf in function definition %d\n", val );
	    }
	    ++state->pc;
	  break;
	  case ttf_idef:
	    val = pop(state,args);
	    if ( state->in_glyf )
		fprintf( stderr, "Instructions may only be defined in the font or cvt programs (fpgm, prep)\n" );
	    else if ( val<=0 || val>=0xff )
		fprintf( stderr, "Attempt to define an opcode out of bounds: %d\n", val );
	    else {
		if ( state->in_prep )
		    fprintf( stderr, "Instructions should be defined in the font rather than cvt program\n" );
		state->idefs[val].data = state->pc;
		while ( state->pc<state->end_pc && *state->pc!=ttf_endf );
		    ++state->pc;
		/* spec doesn't allow for nested defs */
		state->idefs[val].len = state->pc - state->idefs[val].data;
		if ( state->pc==state->end_pc )
		    fprintf( stderr, "Missing endf in instruction definition %d\n", val );
	    }
	    ++state->pc;
	  break;
	  case ttf_endf:	/* We should never see one of these */
	    /* but if we do, it's a noop */
	    fprintf( stderr, "We executed an ENDF instruction. That shouldn't happen.\n" );
	  break;
	  case ttf_call: case ttf_loopcall:
	    val = pop(state,args);
	    val2 = 1;
	    if ( instr == ttf_loopcall )
		val2 = pop(state,args);
	    if ( val<0 || val>=state->fdefcnt || state->fdefs[val].data==NULL )
		fprintf( stderr, "Attempt to call undefined function %d\n", val );
	    else if ( state->retsp>=SUB_MAX )
		fprintf( stderr, "Function calls nested too deeply, ignored (in call of %d)\n", val );
	    else {
		state->returns[state->retsp] = state->pc;
		state->ends[state->retsp] = state->end_pc;
		state->cnts[state->retsp] = state->callcnt;
		state->starts[state->retsp] = state->startcall;
		state->startcall = state->pc = state->fdefs[val].data;
		state->end_pc = state->pc + state->fdefs[val].len;
		state->callcnt = val2-1;
		++state->retsp;
	    }
	  break;
/* Jumps */
	  case ttf_jmpr:
	    state->pc += pop(state,args);
	    if ( state->pc>state->end_pc )
		fprintf( stderr, "Jump beyond end\n" );
	  break;
	  case ttf_jrof:
	    val = pop(state,args);
	    val2 = pop(state,args);
	    if ( val==0 ) {
		state->pc += val;
		if ( state->pc>state->end_pc )
		    fprintf( stderr, "Jump beyond end\n" );
	    }
	  break;
	  case ttf_jrot:
	    val = pop(state,args);
	    val2 = pop(state,args);
	    if ( val!=0 ) {
		state->pc += val;
		if ( state->pc>state->end_pc )
		    fprintf( stderr, "Jump beyond end\n" );
	    }
	  break;
/* Flip from on to off curve */
/*  (does not touch the points) */
	  case ttf_flippt:
	    if ( args!=NULL ) {
		args->loopcnt += state->loop-1;
		if ( state->loop>1 )
		    args->used |= ttf_inloop;
	    }
	    while ( --state->loop>=0 ) {
		val = pop(state,args);
		if ( val<0 || val>=state->zones[state->zp0].point_cnt )
		    fprintf( stderr, "Point out of bounds in FLIPPT: %d\n", val );
		else
		    state->zones[state->zp0].flags[val] ^= pt_oncurve;
	    }
	    state->loop = 1;		/* Always reset to 1 */
	  break;
	  case ttf_fliprgoff: case ttf_fliprgon:
	    val = pop(state,args);
	    val2 = pop(state,args);
	    if ( val<0 || val>=state->zones[state->zp0].point_cnt )
		fprintf( stderr, "Point 1 out of bounds in flip region: %d\n", val );
	    else if ( val2<0 || val2>=val )
		fprintf( stderr, "Point 2 out of bounds in flip region: %d\n", val2 );
	    else {
		while ( val2<=val ) {
		    if ( instr==ttf_fliprgoff )
			state->zones[state->zp0].flags[val2] &= ~pt_oncurve;
		    else
			state->zones[state->zp0].flags[val2] |= pt_oncurve;
		    ++val2;
		}
	    }
	  break;
/* deltas */
	  case ttf_deltac1: case ttf_deltac2: case ttf_deltac3: {
/* MS says smallest ppem must be deepest on stack. Apple doesn't mention this */
	    uint16 arg, c, ppem_base;
	    val = pop(state,args);
	    ppem_base = state->delta_base + (instr==ttf_deltac1?0:instr==ttf_deltac2?16:32);
	    for ( i=0; i<val; ++i ) {
		arg = pop(state,args);
		c = pop(state,args);
		if ( state->cvt==NULL || c>=state->cvt->newlen/2 )
		    fprintf( stderr, "Cvt index %d out of bounds in DELTAC instruction: %d\n", i, c);
		else if ( ppem_base + ((arg>>4)&0xf) == state->cv->show.ppem ) {
		    arg&=0xf;
		    if ( arg<=7 ) arg -= 8;
		    else arg-=7;
		    /* convert to 26.6 */
		    arg <<= 6;
		    arg >>= state->delta_shift;
/* Manipulating the cvt does not appear to be protected */
		    state->cvtvals[c] += arg/state->scale;
		}
	    }
	  } break;
	  case ttf_deltap1: case ttf_deltap2: case ttf_deltap3: {
	    uint32 arg, pt, ppem_base;
	    val = pop(state,args);
	    ppem_base = state->delta_base + (instr==ttf_deltap1?0:instr==ttf_deltap2?16:32);
	    for ( i=0; i<val; ++i ) {
		arg = pop(state,args);
		pt = pop(state,args);
		if ( pt>=state->zones[state->zp0].point_cnt )
		    fprintf( stderr, "Point %d out of bounds in DELTAP instruction: %d\n", i, pt);
		else if ( ppem_base + ((arg>>4)&0xf) == state->cv->show.ppem ) {
		    arg&=0xf;
		    if ( arg<=7 ) arg -= 8;
		    else arg-=7;
		    /* convert to 26.6 */
		    arg <<= 6;
		    arg >>= state->delta_shift;
#if TT_CONFIG_OPTION_BYTECODE_INTERPRETER
/* Manipulating a point's position is protected */
		    AdjustPointBy(state,pt,state->zp0,arg);
#endif
		}
	    }
	  } break;
/* Point movement instructions */
	  case ttf_utp:
	    val = pop(state,args);
	    if ( val<0 || val>=state->zones[state->zp0].point_cnt )
		fprintf( stderr, "Point out of bounds in UTP: %d\n", val );
	    else {
		if ( state->freedom.x!=0 )
		    state->zones[state->zp0].flags[val] &= ~pt_xtouched;
		if ( state->freedom.y!=0 )
		    state->zones[state->zp0].flags[val] &= ~pt_ytouched;
	    }
	  break;
	  case ttf_scfs:
	    val = pop(state,args);
	    val2 = pop(state,args);
	    AdjustPointBy(state,val2,state->zp2,val);	/* A noop unless TT_CONFIG_OPTION_BYTECODE_INTERPRETER */
	  break;
	  case ttf_shc: case ttf_shc+1:
	  case ttf_shz: case ttf_shz+1:
	  case ttf_shp: case ttf_shp+1: { IPoint *rpm, *rpo;
	    if ( instr==ttf_shp ) {
		if ( args!=NULL ) args->used |= ttf_rp2;
		rpm = &state->zones[state->zp1].moved[state->rp2];
		rpo = &state->zones[state->zp1].points[state->rp2];
	    } else {
		if ( args!=NULL ) args->used |= ttf_rp1;
		rpm = &state->zones[state->zp0].moved[state->rp1];
		rpo = &state->zones[state->zp0].points[state->rp1];
	    }
	    val = (rpm->x-rpo->x)*state->projection.x + (rpm->y-rpo->y)*state->projection.y;
	    if ( instr==ttf_shz || instr==ttf_shz+1 ) {
		val2 = pop(state,args);	/* zone */
		for ( i=0; i<state->zones[val2].point_cnt; ++i ) {
		    if ( &state->zones[val2].points[i]!=rpo )	/* Don't shift ref pt again */
			AdjustPointBy(state,i,val2,val);	/* A noop unless TT_CONFIG_OPTION_BYTECODE_INTERPRETER */
		}
	    } else if ( instr==ttf_shp || instr==ttf_shp + 1 ) {
		if ( args!=NULL ) {
		    args->loopcnt += state->loop-1;
		    if ( state->loop>1 )
			args->used |= ttf_inloop;
		}
		while ( --state->loop>=0 ) {
		    val2 = pop(state,args);
		    AdjustPointBy(state,val2,state->zp2,val);	/* A noop unless TT_CONFIG_OPTION_BYTECODE_INTERPRETER */
		}
		state->loop = 1;		/* Always reset to 1 */
	    } else {
		int c = pop(state,args), cp;	/* contour */
		/* I don't see how any contours get defined in zone 0 */
		for ( i=cp=0; c<cp && i<state->zones[state->zp2].point_cnt; ++i )
		    if ( state->zones[state->zp2].flags[i] & pt_endcontour )
			++cp;
		while ( i<state->zones[state->zp2].point_cnt &&
			!(state->zones[state->zp2].flags[i] & pt_endcontour ) ) {
		    if ( &state->zones[state->zp2].points[i]!=rpo )	/* Don't shift ref pt again */
			AdjustPointBy(state,i,state->zp2,val);	/* A noop unless TT_CONFIG_OPTION_BYTECODE_INTERPRETER */
		    ++i;
		}
	    }
	  } break;
	  case ttf_shpix:
	    val = pop(state,args);
	    if ( args!=NULL ) {
		args->loopcnt += state->loop-1;
		if ( state->loop>1 )
		    args->used |= ttf_inloop;
	    }
	    while ( --state->loop>=0 ) {
		val2 = pop(state,args);
		AdjustPointBy(state,val2,state->zp2,val);	/* A noop unless TT_CONFIG_OPTION_BYTECODE_INTERPRETER */
	    }
	    state->loop = 1;		/* Always reset to 1 */
	  break;
	  case ttf_alignpts: { double temp1, temp2;
	    val2 = pop(state,args);
	    val = pop(state,args);
	    temp2 = state->projection.x*state->zones[state->zp0].moved[val2].x +
		    state->projection.y*state->zones[state->zp0].moved[val2].y;
	    temp1 = state->projection.x*state->zones[state->zp1].moved[val].x +
		    state->projection.y*state->zones[state->zp1].moved[val].y;
	    temp1 = (temp1+temp2)/2;
	    SetPointTo(state,val2,state->zp0,temp1,false,-1);	/* A noop unless TT_CONFIG_OPTION_BYTECODE_INTERPRETER */
	    SetPointTo(state,val,state->zp1,temp1,false,-1);	/* A noop unless TT_CONFIG_OPTION_BYTECODE_INTERPRETER */
	  } break;
	  case ttf_alignrp:
	    if ( args!=NULL ) {
		args->loopcnt += state->loop-1;
		args->used |= ttf_rp0;
		if ( state->loop>1 )
		    args->used |= ttf_inloop;
	    }
	    val = state->projection.x*state->zones[state->zp0].moved[state->rp0].x +
		    state->projection.y*state->zones[state->zp0].moved[state->rp0].y;
	    while ( --state->loop>=0 ) {
		val2 = pop(state,args);
		SetPointTo(state,val2,state->zp1,val,false,-1);	/* A noop unless TT_CONFIG_OPTION_BYTECODE_INTERPRETER */
	    }
	    state->loop = 1;		/* Always reset to 1 */
	  break;
	  case ttf_isect: { int a0, a1, b0, b1, p;
	      /* Intersection of two lines. Ignores freedom vector */
	    a0 = pop(state,args);
	    a1 = pop(state,args);
	    b0 = pop(state,args);
	    b1 = pop(state,args);
	    p = pop(state,args);
	    if ( a0<0 || a0>= state->zones[state->zp0].point_cnt ||
		    a1<0 || a1>= state->zones[state->zp0].point_cnt ||
		    b0<0 || b0>= state->zones[state->zp1].point_cnt ||
		    b1<0 || b1>= state->zones[state->zp1].point_cnt ||
		    p<0 || p>= state->zones[state->zp2].point_cnt )
		fprintf(stderr, "At least one point out of range in ISECT: %d,%d,%d,%d,%d\n", a0,a1,b0,b1,p );
	    else {
#if TT_CONFIG_OPTION_BYTECODE_INTERPRETER
		IPoint *pa0 = &state->zones[state->zp0].moved[a0],
			*pa1 = &state->zones[state->zp0].moved[a1],
			*pb0 = &state->zones[state->zp0].moved[b0],
			*pb1 = &state->zones[state->zp0].moved[b1],
			*pp = &state->zones[state->zp0].moved[p];
		int denom = (pb1->y-pb0->y)*(pa1->x-pa0->x)-(pa1->y-pa0->y)*(pb1->x-pb0->x);
		if ( denom==0 ) {		/* parallel */
		    pp->x = (pa0->x+pa1->x+pb0->x+pb1->x)/4;
		    pp->y = (pa0->y+pa1->y+pb0->y+pb1->y)/4;
		} else {
		    pp->x = ((pa0->y-pb0->y)*(pa1->x-pa0->x)*(pb1->x-pb0->x) -
				pa0->x*(pa1->y-pa0->y)*(pb1->x-pb0->x) +
			        pb0->x*(pb1->y-pb0->y)*(pa1->x-pa0->x)) / denom;
		    if ( pb1->x==pb0->x )
			pp->y = (pb0->x-pa0->x)*(pa1->y-pa0->y)/(pa1->x-pa0->x) + pa0->y;
		    else
			pp->y = (pa0->x-pb0->x)*(pb1->y-pb0->y)/(pb1->x-pb0->x) + pb0->y;
		}
#endif
	    }
	  } break;
	  case ttf_mdap: case ttf_mdap+1:
	    val = pop(state,args);
	    if ( val<0 || val>= state->zones[state->zp0].point_cnt )
		fprintf(stderr, "Point out of range in MDAP: %d\n", val );
	    else {
		val2 = state->projection.x*state->zones[state->zp0].points[val].x +
			state->projection.y*state->zones[state->zp0].points[val].y;
		if ( instr==ttf_mdap+1 )
		    val2 = doround(state,val2);
		SetPointTo(state,val,state->zp0,val2,instr==ttf_mdap+1,-1);	/* A noop unless TT_CONFIG_OPTION_BYTECODE_INTERPRETER */
		state->rp0 = state->rp1 = val;
	    }
	  break;
	  case ttf_miap: case ttf_miap+1:
	    val2 = pop(state,args);
	    val = pop(state,args);
	    if ( val<0 || val>= state->zones[state->zp0].point_cnt )
		fprintf(stderr, "Point out of range in MIAP: %d\n", val );
	    else if ( state->cvt==NULL || val2<0 || val2>=state->cvt->newlen/2 ) {
		fprintf(stderr, "CVT entry out of range in MIAP: %d\n", val2 );
	    } else {
		val2 = state->cvtvals[val2]*state->scale;
		if ( instr==ttf_miap+1 ) {
		    int pos = state->projection.x*state->zones[state->zp0].points[val].x +
			    state->projection.y*state->zones[state->zp0].points[val].y;
		    if ( (pos<0 && val2>0 && state->auto_flip) || (pos>0 && val2<0 && state->auto_flip))
			val2 = -val2;
		    if ( val2-pos>=-state->control_value_cutin &&
			    val2-pos<=state->control_value_cutin )
			val2 = doround(state,pos);
		    else
			val2 = doround(state,val2);
		}
		SetPointTo(state,val,state->zp0,val2,instr==ttf_miap,val2);	/* A noop unless TT_CONFIG_OPTION_BYTECODE_INTERPRETER */
		state->rp0 = state->rp1 = val;
	    }
	  break;
	  case ttf_msirp: case ttf_msirp+1:
	    val2 = pop(state,args);
	    val = pop(state,args);
	    if ( args!=NULL )
		args->used |= ttf_rp0;
	    SetPointRelativeTo(state,val,state->zp1,state->rp0,state->zp0,val2,false,false,-1);	/* A noop unless TT_CONFIG_OPTION_BYTECODE_INTERPRETER */
	    if ( instr==ttf_msirp+1 )
		state->rp0 = val;
	  break;
	  case ttf_mdrp: case ttf_mdrp+1: case ttf_mdrp+2: case ttf_mdrp+3:
	  case ttf_mdrp+4: case ttf_mdrp+5: case ttf_mdrp+6: case ttf_mdrp+7:
	  case ttf_mdrp+8: case ttf_mdrp+9: case ttf_mdrp+10: case ttf_mdrp+11:
	  case ttf_mdrp+12: case ttf_mdrp+13: case ttf_mdrp+14:
	  case ttf_mdrp+15: case ttf_mdrp+16: case ttf_mdrp+17:
	  case ttf_mdrp+18: case ttf_mdrp+19: case ttf_mdrp+20:
	  case ttf_mdrp+21: case ttf_mdrp+22: case ttf_mdrp+23:
	  case ttf_mdrp+24: case ttf_mdrp+25: case ttf_mdrp+26:
	  case ttf_mdrp+27: case ttf_mdrp+28: case ttf_mdrp+29:
	  case ttf_mdrp+30: case ttf_mdrp+31:
	    val = pop(state,args);
	    if ( val<0 || val>= state->zones[state->zp1].point_cnt )
		fprintf(stderr, "Point out of range in MDRP: %d\n", val );
	    else {
		IPoint *rp = &state->zones[state->zp0].points[state->rp0],
			*pp = &state->zones[state->zp1].points[val];
		int dist = (pp->x-rp->x)*state->projection.x + (pp->y-rp->y)*state->projection.y;
		if ( args!=NULL )
		    args->used |= ttf_rp0;
		if ( dist-state->single_width>-state->single_width_cutin &&
			dist-state->single_width<state->single_width_cutin )
		    dist = state->single_width;
		if ( (instr-ttf_mdrp)&4 )
		    dist = doround(state,dist);
		if ( ((instr-ttf_mdrp)&8) && dist<state->min_distance )
		    dist = state->min_distance;
		SetPointRelativeTo(state,val,state->zp1,state->rp0,state->zp0,dist,
			(instr-ttf_mdrp)&4,(instr-ttf_mdrp)&8,-1);	/* A noop unless TT_CONFIG_OPTION_BYTECODE_INTERPRETER */
		state->rp1 = state->rp0; state->rp2 = val;
		if ( (instr-ttf_mdrp)&16 )
		    state->rp0 = val;
	    }
	  break;
	  case ttf_mirp: case ttf_mirp+1: case ttf_mirp+2: case ttf_mirp+3:
	  case ttf_mirp+4: case ttf_mirp+5: case ttf_mirp+6: case ttf_mirp+7:
	  case ttf_mirp+8: case ttf_mirp+9: case ttf_mirp+10: case ttf_mirp+11:
	  case ttf_mirp+12: case ttf_mirp+13: case ttf_mirp+14:
	  case ttf_mirp+15: case ttf_mirp+16: case ttf_mirp+17:
	  case ttf_mirp+18: case ttf_mirp+19: case ttf_mirp+20:
	  case ttf_mirp+21: case ttf_mirp+22: case ttf_mirp+23:
	  case ttf_mirp+24: case ttf_mirp+25: case ttf_mirp+26:
	  case ttf_mirp+27: case ttf_mirp+28: case ttf_mirp+29:
	  case ttf_mirp+30: case ttf_mirp+31:
	    val2 = pop(state,args);
	    val = pop(state,args);
	    if ( val<0 || val>= state->zones[state->zp0].point_cnt )
		fprintf(stderr, "Point out of range in MIRP: %d\n", val );
	    else if ( state->cvt==NULL || val2<0 || val2>=state->cvt->newlen/2 ) {
		fprintf(stderr, "CVT entry out of range in MIRP: %d\n", val2 );
	    } else {
		IPoint *rp = &state->zones[state->zp0].points[state->rp0],
			*pp = &state->zones[state->zp1].points[val];
		int dist = (pp->x-rp->x)*state->projection.x + (pp->y-rp->y)*state->projection.y;
		int cvt = val2;
		if ( args!=NULL )
		    args->used |= ttf_rp0;
		val2 = state->cvtvals[val2]*state->scale;
		if ( val2-state->single_width>-state->single_width_cutin &&
			val2-state->single_width<state->single_width_cutin )
		    val2 = state->single_width;
		if ( (instr-ttf_mirp)&4 ) {
		    if ( dist-val2<-state->control_value_cutin ||
			    dist-val2>state->control_value_cutin )
			val2 = dist;		/* Use orig distance */
		    val2 = doround(state,val2);
		}
		if ( ((instr-ttf_mirp)&8) && val2<state->min_distance )
		    val2 = state->min_distance;
		SetPointRelativeTo(state,val,state->zp1,state->rp0,state->zp0,val2,
			(instr-ttf_mirp)&4,(instr-ttf_mirp)&8,cvt);	/* A noop unless TT_CONFIG_OPTION_BYTECODE_INTERPRETER */
		state->rp1 = state->rp0; state->rp2 = val;
		if ( (instr-ttf_mirp)&16 )
		    state->rp0 = val;
	    }
	  break;
	  case ttf_iup: case ttf_iup+1:
	    /* Ignores freedom & projection vectors, either in y dir(iup) or x(iup+1) */
	    /* interpolates any untouched points which fall sequentially between */
	    /*  two touched points */ /* I think this means the end points */
	    /*  can't be interpolated */
	    /* If point is outside segment between two end points (on axis) */
	    /*  then point is shifted by closest point */
	    val = instr==ttf_iup ? pt_ytouched : pt_xtouched;
	    for ( i=1; i<state->zones[state->zp2].point_cnt; ++i ) {
		if ( (state->zones[state->zp2].flags[i-1]&val) &&
			(state->zones[state->zp2].flags[i+1]&val) &&
			!(state->zones[state->zp2].flags[i]&val) ) {
#if TT_CONFIG_OPTION_BYTECODE_INTERPRETER
		    IPoint *pre = &state->zones[state->zp2].points[i-1],
			    *cur = &state->zones[state->zp2].points[i],
			    *post = &state->zones[state->zp2].points[i+1];
		    IPoint *mpre = &state->zones[state->zp2].moved[i-1],
			    *mcur = &state->zones[state->zp2].moved[i],
			    *mpost = &state->zones[state->zp2].moved[i+1];
		    if ( instr==ttf_iup ) {
			if (( cur->y<=pre->y && pre->y<=post->y ) ||
				(cur->y>=pre->y && pre->y>=post->y )) {
			    mcur->y += (mpre->y-pre->y);
			} else if (( cur->y<=post->y && post->y<=pre->y ) ||
				(cur->y>=post->y && post->y>=pre->y )) {
			    mcur->y += (mpost->y-post->y);
			} else {
			    double mid = (cur->y-pre->y)/(post->y-pre->y);
			    mcur->y += mid*(mpost->y-post->y) +
				    (1-mid)*(mpre->y-pre->y);
			}
		    } else {
			if (( cur->x<=pre->x && pre->x<=post->x ) ||
				(cur->x>=pre->x && pre->x>=post->x )) {
			    mcur->x += (mpre->x-pre->x);
			} else if (( cur->x<=post->x && post->x<=pre->x ) ||
				(cur->x>=post->x && post->x>=pre->x )) {
			    mcur->x += (mpost->x-post->x);
			} else {
			    double mid = (cur->x-pre->x)/(post->x-pre->x);
			    mcur->x += mid*(mpost->x-post->x) +
				    (1-mid)*(mpre->x-pre->x);
			}
		    }
#endif
		}
	    }
	  break;
	  case ttf_ip: {
	    IPoint *pre = &state->zones[state->zp0].points[state->rp1],
		    *cur,
		    *post = &state->zones[state->zp1].points[state->rp2];
	    IPoint *mpre = &state->zones[state->zp0].moved[state->rp1],
		    *mpost = &state->zones[state->zp1].moved[state->rp2];
	    double dist, mdist, base;
	    /* now the above instruction (interpolate untouched points) */
	    /*  has a special case for points whose coordinates lie outside */
	    /*  the range of the two end points. None of the docs say that */
	    /*  this instruction does that. So I guess it probably doesn't */
	    /*  But I don't trust truetype docs...*/
	    /* (does this touch the point?) */
	    if ( args!=NULL ) {
		args->loopcnt += state->loop-1;
		args->used |= ttf_rp1|ttf_rp2;
		if ( state->loop>1 )
		    args->used |= ttf_inloop;
	    }
	    mdist = (mpre->x-mpost->x)*state->projection.x +
		    (mpre->y-mpost->y)*state->projection.y ;
	    dist = (pre->x-post->x)*state->projection.x +
		    (pre->y-post->y)*state->projection.y ;
	    base = post->x*state->projection.x + post->y*state->projection.y;
	    if ( dist==0 ) {
		fprintf(stderr, "Illegal IP instruction, rp1 and rp2 point to the same place on the projection.\n" );
		while ( --state->loop>=0 ) {
		    pop(state,args);
		}
	    } else {
		mdist /= dist;
		while ( --state->loop>=0 ) {
		    val = pop(state,args);
		    if ( val<0 || val>= state->zones[state->zp2].point_cnt )
			fprintf(stderr, "Point out of range in IP: %d\n", val );
		    else {
			cur = &state->zones[state->zp2].moved[val];
			val2 = (cur->x-post->x)*state->projection.x +
				(cur->y-post->y)*state->projection.y;
			InterpolatePointTo(state,val,state->zp2,mdist*val2 + base,
				false,-1,state->rp1,state->rp2);
		    }
		}
	    }
	    state->loop = 1;		/* Always reset to 1 */
	  } break;
	  default:
	    fprintf(stderr, "Unknown truetype instruction %x\n", instr );
	  break;
	}
    }
}

static void NotePoints(struct ttfstate *state,ConicPointList *head) {
    ConicPointList *cpl;
    ConicPoint *sp;
    int last=0;

    for ( cpl = head; cpl!=NULL ; cpl = cpl->next ) {
	sp = cpl->first;
	while ( 1 ) {
	    if ( sp->me.pnum!=-1 ) {
		last = sp->me.pnum;
		state->zones[1].points[last].x = sp->me.x*state->scale;
		state->zones[1].points[last].y = sp->me.y*state->scale;
		state->zones[1].flags[last] = pt_oncurve;
	    }
	    if ( sp->nextcp != NULL ) {
		last = sp->nextcp->pnum;
		state->zones[1].points[last].x = sp->nextcp->x*state->scale;
		state->zones[1].points[last].y = sp->nextcp->y*state->scale;
		state->zones[1].flags[last] = false;
	    }
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==cpl->first )
	break;
	}
	state->zones[1].flags[last] |= pt_endcontour;
    }
}

static void init_ttfstate(CharView *cv,struct ttfstate *state) {
    Table *maxp, *fpgm, *prep, *cvt;
    TtfFont *tfont = cv->cc->parent->tfont;
    int i;
    RefChar *r;

    memset(state,'\0',sizeof(struct ttfstate));
    maxp = fpgm = prep = cvt = NULL;
    for ( i=0; i<tfont->tbl_cnt; ++i ) {
	if ( tfont->tbls[i]->name==CHR('m','a','x','p'))
	    maxp = tfont->tbls[i];
	else if ( tfont->tbls[i]->name==CHR('c','v','t',' '))
	    cvt = tfont->tbls[i];
	else if ( tfont->tbls[i]->name==CHR('f','p','g','m'))
	    fpgm = tfont->tbls[i];
	else if ( tfont->tbls[i]->name==CHR('p','r','e','p'))
	    prep = tfont->tbls[i];
    }
    state->cv = cv;
    state->cc = cv->cc;
    state->cvt = cvt; state->maxp = maxp;
    state->tfont = tfont;
    if ( cvt!=NULL ) {
	TableFillup(cvt);
	state->cvtvals = galloc(cvt->newlen);
	for ( i=0; i<cvt->newlen/2; ++i )
	    state->cvtvals[i] = ptgetushort(cvt->data+2*i);
    }

    state->scale = 64.0*cv->show.ppem/cv->cc->parent->em;

    if ( maxp==NULL || maxp->newlen<26 ) {
	fprintf( stderr, "No maxp table (or too small a table), that's an error\n" );
	state->store_max = 100;
	state->stack_max = 200;
	state->idefcnt = 100;
	state->fdefcnt = 100;
	state->zones[0].point_cnt = 100;
    } else {
	TableFillup(maxp);
	state->store_max = ptgetushort(maxp->data+18);
	state->stack_max = ptgetushort(maxp->data+24);
	state->idefcnt = ptgetushort(maxp->data+22);
	state->fdefcnt = ptgetushort(maxp->data+20);
	state->zones[0].point_cnt = ptgetushort(maxp->data+16);
    }
    state->idefs = gcalloc(256,sizeof(struct ifdef));/* one for each opcode */
    if ( state->fdefcnt!=0 )
	state->fdefs = gcalloc(state->fdefcnt,sizeof(struct ifdef));
    state->stack = gcalloc(state->stack_max,sizeof(int32));
    if ( state->store_max!=0 )
	state->storage = gcalloc(state->store_max,sizeof(int32));
    if ( state->zones[0].point_cnt!=0 ) {
	state->zones[0].points = gcalloc(state->zones[0].point_cnt,sizeof(IPoint));
	state->zones[0].moved = gcalloc(state->zones[0].point_cnt,sizeof(IPoint));
	state->zones[0].flags = gcalloc(state->zones[0].point_cnt,sizeof(uint8));
    }

    state->loop = 1;		/* loop instructions default to being run once*/

    state->zp0 = state->zp1 = state->zp2 = 1;	/* Normal zone */
    /* the rp? registers default to 0 */

    /* the freedom/projection vectors are always unit vectors. defaulting to x-axis */
    state->freedom.x = state->projection.x = 1;
    state->dual.x = 1;				/* Dual has no default, It should be a unit vector though */

    state->auto_flip = true;

    state->min_distance = 64;		/* Default value is 1pixel in 26.6 */
    state->control_value_cutin = 64+4;	/* Default value is 17/16 in 26.6 */
    state->single_width_cutin = 0;
    state->single_width = 0;

    state->delta_base = 9;
    state->delta_shift = 3;

    state->instruction_control = 0;
    state->scancontrol = false;

    state->rounding = rnd_grid;

/* these two execute in a context where there is no character, so no zone1 */
    if ( fpgm!=NULL ) {
	TableFillup(fpgm);
	state->in_fpgm = true;
	TtfExecuteInstrs(state,fpgm->data,fpgm->newlen);
	state->in_fpgm = false;
    }
    if ( prep!=NULL ) {
	TableFillup(prep);
	state->in_prep = true;
	TtfExecuteInstrs(state,prep->data,prep->newlen);
	state->in_prep = false;
    }

    state->zones[1].point_cnt = state->cc->point_cnt+2;	/* Include the two virtual points */
    state->zones[1].points = gcalloc(state->zones[1].point_cnt,sizeof(IPoint));
    state->zones[1].moved = galloc(state->zones[1].point_cnt*sizeof(IPoint));
    state->zones[1].flags = gcalloc(state->zones[1].point_cnt,sizeof(uint8));
    NotePoints(state,state->cc->conics);
    for ( r = state->cc->refs; r!=NULL; r = r->next )
	NotePoints(state,r->conics);
    memcpy(state->zones[1].moved,state->zones[1].points, state->zones[1].point_cnt*sizeof(IPoint));

    state->args = gcalloc(cv->cc->instrdata.instr_cnt,sizeof(struct ttfargs));
    state->in_glyf = true;
}

static void TtfStateFreeContents(struct ttfstate *state) {
    free(state->idefs);
    free(state->fdefs);
    free(state->stack);
    free(state->storage);
    free(state->zones[0].points);
    free(state->zones[0].moved);
    free(state->zones[0].flags);
    free(state->zones[1].points);
    free(state->zones[1].moved);
    free(state->zones[1].flags);
    free(state->cvtvals);
    free(state->args);
    TtfActionsFree(state->acts);
}

void CVGenerateGloss(CharView *cv) {
    struct ttfstate state;
    struct ttfactions *acts;
    int cnt;

    free(cv->instrinfo.args);
    TtfActionsFree(cv->instrinfo.acts);
    init_ttfstate(cv,&state);
    TtfExecuteInstrs(&state,cv->cc->instrdata.instrs,cv->cc->instrdata.instr_cnt);
    cv->instrinfo.args = state.args; state.args = NULL;
    cv->instrinfo.acts = state.acts; state.acts = NULL;
    TtfStateFreeContents(&state);
    for ( cnt=0, acts=cv->instrinfo.acts; acts!=NULL; acts=acts->acts )
	++cnt;
    cv->instrinfo.act_cnt = cnt;
}
#endif

void TtfActionsFree(struct ttfactions *acts) {
    struct ttfactions *next;

    while ( acts!=NULL ) {
	next = acts->acts;
	free(acts);
	acts=next;
    }
}
