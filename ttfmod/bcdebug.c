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
/* Instructions we haven't dealt with yet...

 ttf_alignpts=0x27, ttf_alignrp=0x3c,
 ttf_call=0x2b, ttf_cindex=0x25, ttf_clear=0x22,
 ttf_deltac1=0x73, ttf_deltac2=0x74, ttf_deltac3=0x75,
 ttf_deltap1=0x5d, ttf_deltap2=0x71, ttf_deltap3=0x72, ttf_depth=0x24,
 ttf_dup=0x20, ttf_eif=0x59, ttf_else=0x1b, ttf_endf=0x2d,
 ttf_fdef=0x2c, ttf_flipoff=0x4e, ttf_flipon=0x4d,
 ttf_flippt=0x80, ttf_fliprgoff=0x82, ttf_fliprgon=0x81,
 ttf_gc=0x46, ttf_getinfo=0x88, ttf_gfv=0x0d, ttf_gpv=0x0c,
 ttf_idef=0x89, ttf_if=0x58, ttf_instctrl=0x8e, ttf_ip=0x39,
 ttf_isect=0x0f, ttf_iup=0x30, ttf_jmpr=0x1c, ttf_jrof=0x79, ttf_jrot=0x78,
 ttf_loopcall=0x2a, ttf_md=0x49,
 ttf_mdap=0x2e, ttf_mdrp=0xc0, ttf_miap=0x3e, ttf_mindex=0x26,
 ttf_mirp=0xe0, ttf_mppem=0x4b, ttf_mps=0x4c, ttf_msirp=0x3a, 
 ttf_rdtg=0x7d, ttf_roff=0x7a,
 ttf_roll=0x8a, ttf_rtdg=0x3d, ttf_rtg=0x18,
 ttf_rthg=0x19, ttf_rutg=0x7c, ttf_s45round=0x77, 
 ttf_scanctrl=0x85, ttf_scantype=0x8d, ttf_scfs=0x48, ttf_scvtci=0x1d,
 ttf_sdb=0x5e, ttf_sdpvtl=0x86, ttf_sds=0x5f, ttf_sfvfs=0x0b, ttf_sfvtca=0x04,
 ttf_sfvtl=0x08, ttf_sfvtpv=0x0e, ttf_shc=0x34, ttf_shp=0x32, ttf_shpix=0x38,
 ttf_shz=0x36, ttf_sloop=0x17, ttf_smd=0x1a, ttf_spvfs=0x0a, ttf_spvtca=0x02,
 ttf_spvtl=0x06, ttf_sround=0x76, ttf_srp0=0x10, ttf_srp1=0x11, ttf_srp2=0x12,
 ttf_ssw=0x1f, ttf_sswci=0x1e, ttf_svtca=0x00, ttf_swap=0x23,
 ttf_szp0=0x13, ttf_szp1=0x14, ttf_szp2=0x15, ttf_szps=0x16, ttf_utp=0x29,
*/

static void TtfExecuteInstrs(struct ttfstate *state,uint8 *data,int len) {
    int instr, n, val, i;
    struct ttfargs *args;

    state->pc = data;
    state->end_pc = data+len;

    while ( true ) {
	while ( state->pc>=state->end_pc ) {
	    if ( state->retsp==0 )
return;
	    --state->retsp;
	    state->pc = state->returns[state->retsp];
	    state->end_pc = state->ends[state->retsp];
	}
	args = NULL;
	if ( state->retsp==0 )
	    args = &state->args[(state->pc-data)];
	instr= *state->pc++;
	if ( instr>=ttf_pushb && instr<ttf_pushb+8 ) {
	    n = instr-ttf_pushb+1;
	    for ( i=0; i<n; ++i )
		push(state,*(state->pc++),args);
	} else if ( instr>=ttf_pushw && instr<ttf_pushw+8 ) {
	    n = instr-ttf_pushw+1;
	    for ( i=0; i<n; ++i ) {
		val = *(state->pc++)<<8;
		val |= *(state->pc++);
		push(state,(short) val,args);
	    }
	} else switch( instr ) {
	  case ttf_npushb:
	    n = *(instr->pc++);
	    for ( i=0; i<n; ++i )
		push(state,*(state->pc++),args);
	  break;
	  case ttf_npushw:
	    n = *(instr->pc++);
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
	  case ttf_abs: case ttf_ceiling: case ttf_debug:
	  case ttf_even: case ttf_floor: case ttf_not:
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
		if ( state->cvt==NULL || val<0 || val>=state->cvt->len/2 ) {
		    fprintf( stderr, "Attempt to read cvt out of bounds %d\n" val );
		    val = 0;
		} else
		    val = ((short) ptgetushort(state->cvt->data+2*val))*state->scale;
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
	      case ttf_sub: val -= val2; break;
	      case ttf_div: val /= val2; break;
	      case ttf_mul: val *= val2; break;
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
		if ( state->storage==NULL || val2<0 || val2>=state->store_max ) {
		    fprintf( stderr, "Attempt to read storage out of bounds %d\n" val2 );
		} else
		    state->storage[val2] = val;
	    } else {
		if ( instr==ttf_wcvtp )
		    val = (val/state->scale);
		if ( state->cvt==NULL || val2<0 || val2>=state->cvt->len/2 ) {
		    fprintf( stderr, "Attempt to read cvt out of bounds %d\n" val2 );
		} else
		    ptputushort(state->cvt->data+2*val2,val);
	    }
	  break;
	  default:
	    fprintf(stderr, "Unknown truetype instruction %x\n", instr );
	  break;
	}
}

static void init_ttfstate(CharView *cv,struct ttfstate *state) {
    Table *maxp, *fpgm, *prep, *cvt;
    TtfFont *tfont = cv->cc->parent->tfont;
    int i;

    memset(state,'\0',sizeof(struct ttfstate));
    maxp = fpgm = prep = cvt = NULL;
    for ( i=0; i<tfont->tbl_cnt; ++i ) {
	if ( tfont->tbls[i]->name==CHR('maxp'))
	    maxp = tfont->tbls[i];
	else if ( tfont->tbls[i]->name==CHR('cvt '))
	    cvt = tfont->tbls[i];
	else if ( tfont->tbls[i]->name==CHR('fpgm'))
	    fpgm = tfont->tbls[i];
	else if ( tfont->tbls[i]->name==CHR('prep'))
	    prep = tfont->tbls[i];
    }
    state->cc = cv->cc;
    state->cvt = cvt; state->maxp = maxp;
    state->tfont = tfont;

    state->scale = 64.0*cv->show.ppem/cv->cc->parent->em;

    if ( maxp==NULL ) {
	fprintf( stderr, "No maxp table, that's an error\n" );
	state->storage = 100;
	state->stack_max = 200;
	state->idefcnt = 100;
	state->fdefcnt = 100;
    } else {
	TableFillup(maxp);
	state->storage = ptgetushort(maxp->data+18);
	state->stack_max = ptgetushort(maxp->data+24);
	state->idefcnt = ptgetushort(maxp->data+22);
	state->fdefcnt = ptgetushort(maxp->data+20);
    }
    if ( state->idefcnt!=0 )
	state->idefs = gcalloc(state->idefcnt,sizeof(struct ifdef));
    if ( state->fdefcnt!=0 )
	state->fdefs = gcalloc(state->fdefcnt,sizeof(struct ifdef));
    state->stack = gcalloc(state->stack_max,sizeof(int32));
    if ( state->store_max!=0 )
	state->storage = gcalloc(state->store_max,sizeof(int32));

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
    state->args = gcalloc(cv->cc->instrdata->len,sizeof(struct ttfargs));
    state->in_glyf = true;
}
