/* Copyright (C) 2001-2002 by George Williams */
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
#include <math.h>
#include "ttfinstrs.h"

static const int pushes[] = { ttf_pushb, ttf_pushb+1, ttf_pushb+2, ttf_pushb+3,
	ttf_pushb+4, ttf_pushb+5, ttf_pushb+6, ttf_pushb+7,
	ttf_pushw, ttf_pushw+1, ttf_pushw+2, ttf_pushw+3, ttf_npushb,
	ttf_pushw+4, ttf_pushw+5, ttf_pushw+6, ttf_pushw+7, ttf_npushw, -1 };
static const int stack[] = { ttf_pop, ttf_dup, ttf_swap, ttf_roll, ttf_mindex,
	ttf_cindex, ttf_clear, ttf_depth, -1 };
static const int unary[] = { ttf_abs, ttf_neg, ttf_ceiling, ttf_floor,
	ttf_nround, ttf_nround+1, ttf_nround+2, ttf_round, ttf_round+1,
	ttf_round+2, -1 };
static const int binary[] = { ttf_add, ttf_sub, ttf_mul, ttf_div, ttf_max, ttf_min,
	-1 };
static const int logical[] = { ttf_not, ttf_or, ttf_and, ttf_even, ttf_odd,
	ttf_eq, ttf_neq, ttf_lt, ttf_lteq, ttf_gt, ttf_gteq, -1 };
static const int storage[] = { ttf_rcvt, ttf_wcvtf, ttf_wcvtp, ttf_ws, ttf_rs, -1 };
static const int deltas[] = { ttf_deltac1, ttf_deltac2, ttf_deltac3, ttf_deltap1,
	ttf_deltap2, ttf_deltap3, -1 };
static const int conditionals[] = { ttf_if, ttf_else, ttf_eif, ttf_jmpr, ttf_jrof,
	ttf_jrot, -1 };
static const int routines[] = { ttf_call, ttf_loopcall, ttf_fdef, ttf_endf, ttf_idef,
	-1 };
static const int ubytes[] = { -1 };
static const int sshorts[] = { -1 };
static const int vectors[] = { ttf_svtca, ttf_svtca+1, ttf_sfvfs, ttf_sfvtca,
	ttf_sfvtca+1, ttf_sfvtl, ttf_sfvtl+1, ttf_sfvtpv, ttf_spvfs, ttf_spvtca,
	ttf_spvtca+1, ttf_spvtl, ttf_spvtl+1, ttf_sdpvtl, ttf_sdpvtl+1, ttf_gfv,
	ttf_gpv, -1 };
static const int ttsetstate[] = { ttf_flipoff, ttf_flipon, ttf_rdtg, ttf_roff, ttf_rtdg,
	ttf_rtg, ttf_rthg, ttf_rutg, ttf_s45round, ttf_sround, ttf_sdb, ttf_sds,
	ttf_instctrl, ttf_scanctrl, ttf_scantype, ttf_scvtci, ttf_smd, ttf_ssw,
	ttf_sswci, ttf_sloop, -1 };
static const int setregisters[] = { ttf_srp0, ttf_srp1, ttf_srp2,
	ttf_szp0, ttf_szp1, ttf_szp2, ttf_szps, -1 };
static const int getinfo[] = { ttf_gc, ttf_gc+1, ttf_getinfo, ttf_md, ttf_md+1,
	ttf_mppem, ttf_mps, -1 };
static const int pointstate[] = { ttf_utp, ttf_flippt, ttf_fliprgoff, ttf_fliprgon, -1 };
static const int movepoints[] = { ttf_alignpts, ttf_alignrp, ttf_ip, ttf_iup,
	ttf_iup+1, ttf_isect, ttf_mdap, ttf_mdap+1, ttf_miap, ttf_miap+1,
	ttf_msirp, ttf_msirp+1, ttf_scfs, ttf_shc, ttf_shc+1, ttf_shp, ttf_shp+1,
	ttf_shpix, ttf_shz, -1 };
static const int mdrp[] = { ttf_mdrp, ttf_mdrp+0x01, ttf_mdrp+0x02,
	ttf_mdrp+0x04, ttf_mdrp+0x05, ttf_mdrp+0x06,
	ttf_mdrp+0x08, ttf_mdrp+0x09, ttf_mdrp+0x0a,
	ttf_mdrp+0x0c, ttf_mdrp+0x0d, ttf_mdrp+0x0e,
	ttf_mdrp+0x10, ttf_mdrp+0x11, ttf_mdrp+0x12,
	ttf_mdrp+0x14, ttf_mdrp+0x15, ttf_mdrp+0x16,
	ttf_mdrp+0x18, ttf_mdrp+0x19, ttf_mdrp+0x1a,
	ttf_mdrp+0x1c, ttf_mdrp+0x1d, ttf_mdrp+0x1e, -1 };
static const int mirp[] = { ttf_mirp, ttf_mirp+0x01, ttf_mirp+0x02,
	ttf_mirp+0x04, ttf_mirp+0x05, ttf_mirp+0x06,
	ttf_mirp+0x08, ttf_mirp+0x09, ttf_mirp+0x0a,
	ttf_mirp+0x0c, ttf_mirp+0x0d, ttf_mirp+0x0e,
	ttf_mirp+0x10, ttf_mirp+0x11, ttf_mirp+0x12,
	ttf_mirp+0x14, ttf_mirp+0x15, ttf_mirp+0x16,
	ttf_mirp+0x18, ttf_mirp+0x19, ttf_mirp+0x1a,
	ttf_mirp+0x1c, ttf_mirp+0x1d, ttf_mirp+0x1e, -1 };

static GTextInfo instrtypes[] = {
    { (unichar_t *) _STR_Pushes, NULL, 0, 0, (void *) pushes, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_UBytes, NULL, 0, 0, (void *) ubytes, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Shorts, NULL, 0, 0, (void *) sshorts, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Stack, NULL, 0, 0, (void *) stack, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_StorageOps, NULL, 0, 0, (void *) storage, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Unary, NULL, 0, 0, (void *) unary, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Binary, NULL, 0, 0, (void *) binary, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Logical, NULL, 0, 0, (void *) logical, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Conditional, NULL, 0, 0, (void *) conditionals, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Routines, NULL, 0, 0, (void *) routines, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Vectors, NULL, 0, 0, (void *) vectors, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SetState, NULL, 0, 0, (void *) ttsetstate, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SetRegisters, NULL, 0, 0, (void *) setregisters, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_GetInfo, NULL, 0, 0, (void *) getinfo, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_PointState, NULL, 0, 0, (void *) pointstate, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_MovePoints, NULL, 0, 0, (void *) movepoints, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_MDRP, NULL, 0, 0, (void *) mdrp, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_MIRP, NULL, 0, 0, (void *) mirp, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Deltas, NULL, 0, 0, (void *) deltas, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }
};

static GTextInfo *number[] = {
    NULL
};

#define CID_InstrClasses	1001		/* List of types of instrs */
#define CID_Instrs		1002		/* List of instrs */
#define CID_Instr		1003		/* tf of current instruction */

typedef struct instrmod {
    GWindow gw;
    struct instrinfo *ii;
    struct instrdata *id;
} InstrMod;

static GTextInfo **NewInstrList(const int *vals) {
    GTextInfo **list;
    int i;

    for ( i=0; vals[i]!=-1; ++i );
    list = galloc((i+1)*sizeof(GTextInfo *));
    list[i] = NULL;
    for ( i=0; vals[i]!=-1; ++i ) {
	list[i] = gcalloc(1,sizeof(GTextInfo));
	list[i]->fg = list[i]->bg = COLOR_DEFAULT;
	list[i]->userdata = (void *) vals[i];
	list[i]->text = uc_copy(instrs[vals[i]]);
    }
return( list );
}

static const int *FindInstrList(uint8 instr,int *which, int *subwhich) {
    const int *instrs;
    int i,j;

    for ( i=0; instrtypes[i].text!=NULL; ++i ) {
	instrs = instrtypes[i].userdata;
	for ( j=0; instrs[j]!=-1; ++j )
	    if ( instrs[j]==instr ) {
		*which = i;
		*subwhich = j;
return( instrs );
	    }
    }
    /* some instructions are unassigned (8F), others (like ROUND[3]) have no meaning */
return( NULL );
}

int InstrsFindAddr(struct instrinfo *ii) { return( 0 ); }

static void IMSetToInstr(InstrMod *im) {
    int where, subwhere, addr;
    const int *list;
    const char *msg;
    unichar_t *temp;
    char buffer[8];

    addr = InstrsFindAddr(im->ii);		/* index of isel_pos in instrs array */
    if ( im->ii->isel_pos==-1 || im->id->instr_cnt==0 ) {
	where = 0; subwhere = 0;
	list = pushes;
	msg = instrs[ttf_pushb];
    } else if ( im->id->bts[addr]==bt_instr ) {
	list = FindInstrList(im->id->instrs[addr],&where,&subwhere);
	msg = instrs[im->id->instrs[addr]];
    } else if ( im->id->bts[addr]==bt_cnt || im->id->bts[addr]==bt_byte ) {
	list = ubytes;
	where = 1; subwhere = 0;
	sprintf( buffer, "%d", im->id->instrs[addr]);
	msg = buffer;
    } else {
	list = sshorts;
	where = 1; subwhere = 0;
	sprintf( buffer, "%d", (short) ((im->id->instrs[addr]<<8)|im->id->instrs[addr+1]));
	msg = buffer;
    }

    if ( list==ubytes || list==sshorts ) {
	GGadgetSetList(GWidgetGetControl(im->gw,CID_Instrs),number,true);
    } else {
	GTextInfo **tlist = NewInstrList(list);
	tlist[subwhere]->selected = true;
	GGadgetSetList(GWidgetGetControl(im->gw,CID_Instrs),tlist,false);
    }
    GGadgetSetTitle(GWidgetGetControl(im->gw,CID_InstrClasses),
	    GStringGetResource((int) instrtypes[where].text,NULL));
    temp = uc_copy(msg);
    GGadgetSetTitle(GWidgetGetControl(im->gw,CID_Instr),temp);
    free(temp);
}

/* A line can contain a list of numbers and one instruction, or (if ubyte or shorts is set) */
/*  exactly one number and no instructions */
/* If a number has a decimal point it will be interpreted as 10.6 (ie a 16 bit*/
/*  version of 26.6) */
static uint8 *BuildInstrs(InstrMod *im, int *_cnt) {
    const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(im->gw,CID_Instr));
    unichar_t *end, *pt;
    int16 *numbs = galloc((u_strlen(ret)/2+1)*sizeof(int16));
    int ncnt=0;
    long val;
    double dval;
    int instr,i;

    while ( isspace( *ret )) ++ret;
    if ( *ret=='\0' ) {
	GWidgetErrorR(_STR_NoInstruction,_STR_NoInstruction);
	free( numbs );
return( NULL );
    }

    while ( *ret ) {
	val = u_strtol(ret,&end,0);
	if ( *end=='.' ) {
	    dval = u_strtod(ret,&end);
	    val = rint(dval*64);
	}
	if ( end==ret )
    break;
	if ( val>=32768 || val<-32768 ) {
	    GWidgetErrorR(_STR_ValueOutOfBounds,_STR_ValueMustBeShort,val);
	    free( numbs );
return( NULL );
	}
	numbs[ncnt++] = val;
	ret = end;
    }

    while ( isspace( *ret )) ++ret;
    instr = -1;
    if ( *ret!='\0' ) {
	for ( i=0; instrs[i]!=NULL; ++i )
	    if ( uc_strcmp(ret,instrs[i])==0 )
	break;
	if ( instrs[i]!=NULL )
	    instr = i;
	else if ( (pt=u_strchr(ret,'['))!=NULL ) {
	    for ( i=0; instrs[i]!=NULL; ++i )
		if ( uc_strncmp(ret,instrs[i],pt+1-ret)==0 )
	    break;
	    if ( instrs[i]!=NULL ) {
		val = u_strtol(pt+1,&end,2);
		if ( *end!=']' ) {
		    val = u_strtol(pt+1,&end,10);
		    if ( *end!=']' )
			val = u_strtol(pt+1,&end,16);
		}
		if ( *end==']' )
		    instr = i+val;
	    }
	}
	if ( instr==-1 ) {
	    GWidgetErrorR(_STR_NoInstruction,_STR_CouldntParseInstr, ret);
	    free( numbs );
return( NULL );
	}
    }
	
}

void InstrModCreate(struct instrinfo *ii) {
    InstrMod *im;

    fprintf( stderr, "NYI\n" );
}
