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
#include "ttfinstrs.h"

static int pushes[] = { ttf_pushb, ttf_pushb+1, ttf_pushb+2, ttf_pushb+3,
	ttf_pushb+4, ttf_pushb+5, ttf_pushb+6, ttf_pushb+7,
	ttf_pushw, ttf_pushw+1, ttf_pushw+2, ttf_pushw+3, ttf_npushb,
	ttf_pushw+4, ttf_pushw+5, ttf_pushw+6, ttf_pushw+7, ttf_npushw, -1 };
static int stack[] = { ttf_pop, ttf_dup, ttf_swap, ttf_roll, ttf_mindex,
	ttf_cindex, ttf_clear, ttf_depth, -1 };
static int unary[] = { ttf_abs, ttf_neg, ttf_ceiling, ttf_floor,
	ttf_nround, ttf_nround+1, ttf_nround+2, ttf_round, ttf_round+1,
	ttf_round+2, -1 };
static int binary[] = { ttf_add, ttf_sub, ttf_mul, ttf_div, ttf_max, ttf_min,
	-1 };
static int logical[] = { ttf_not, ttf_or, ttf_and, ttf_even, ttf_odd,
	ttf_eq, ttf_ne, ttf_lt, ttf_lteq, ttf_gt, ttf_gteq, -1 };
static int storage[] = { ttf_rcvt, ttf_wcvtf, ttf_wcvtp, ttf_ws, ttf_rs, -1 };
static int deltas[] = { ttf_deltac1, ttf_deltac2, ttf_deltac3, ttf_deltap1,
	ttf_deltap2, ttf_deltap3, -1 };
static int conditionals[] = { ttf_if, ttf_else, ttf_eif, ttf_jmpr, ttf_jrof,
	ttf_jrot, -1 };
static int routines[] = { ttf_call, ttf_loopcall, ttf_fdef, ttf_endf, ttf_idef,
	-1 };
static int ubytes[] = { -1 };
static int sshorts[] = { -1 };
static int vectors[] = { ttf_svtca, ttf_svtca+1, ttf_sfvfs, ttf_sfvtca,
	ttf_sfvtca+1, ttf_sfvtl, ttf_sfvtl+1, ttf_sfvtpv, ttf_spvfs, ttf_spvtca,
	ttf_spvtca+1, ttf_spvtl, ttf_spvtl+1, ttf_sdpvtl, ttf_sdpvtl+1, ttf_gfv,
	ttf_gpv, -1 };
static int setstate[] = { ttf_flipoff, ttf_flipon, ttf_rdtg, ttf_roff, ttf_rtdg,
	ttf_rtg, ttf_rthg, ttf_rutg, ttf_s45round, ttf_sround, ttf_sdb, ttf_sds,
	ttf_instctrl, ttf_scanctrl, ttf_scantype, ttf_scvtci, ttf_smd, ttf_ssw,
	ttf_sswci, ttf_sloop, -1 };
static int setregisters[] = { ttf_srp0, ttf_srp1, ttf_srp2,
	ttf_szp0, ttf_szp1, ttf_szp2, ttf_szps, -1 };
static int getinfo[] = { ttf_gc, ttf_gc+1, ttf_getinfo, ttf_md, ttf_md+1,
	ttf_mppem, ttf_mps, -1 };
static int pointstate[] = { ttf_utp, ttf_flippt, ttf_fliprgoff, ttf_fliprgon, -1 };
static int movepoints[] = { ttf_alignpts, ttf_alignrp, ttf_ip, ttf_iup,
	ttf_iup+1, ttf_isect, ttf_mdap, ttf_mdap+1, ttf_miap, ttf_miap+1,
	ttf_msirp, ttf_msirp+1, ttf_scfs, ttf_shc, ttf_shc+1, ttf_shp, ttf_shp+1,
	ttf_shpix, ttf_shz, -1 };
static int mdrp[] = { ttf_mdrp, ttf_mdrp+0x01, ttf_mdrp+0x02,
	ttf_mdrp+0x04, ttf_mdrp+0x05, ttf_mdrp+0x06,
	ttf_mdrp+0x08, ttf_mdrp+0x09, ttf_mdrp+0x0a,
	ttf_mdrp+0x0c, ttf_mdrp+0x0d, ttf_mdrp+0x0e,
	ttf_mdrp+0x10, ttf_mdrp+0x11, ttf_mdrp+0x12,
	ttf_mdrp+0x14, ttf_mdrp+0x15, ttf_mdrp+0x16,
	ttf_mdrp+0x18, ttf_mdrp+0x19, ttf_mdrp+0x1a,
	ttf_mdrp+0x1c, ttf_mdrp+0x1d, ttf_mdrp+0x1e, -1 };
static int mirp[] = { ttf_mirp, ttf_mirp+0x01, ttf_mirp+0x02,
	ttf_mirp+0x04, ttf_mirp+0x05, ttf_mirp+0x06,
	ttf_mirp+0x08, ttf_mirp+0x09, ttf_mirp+0x0a,
	ttf_mirp+0x0c, ttf_mirp+0x0d, ttf_mirp+0x0e,
	ttf_mirp+0x10, ttf_mirp+0x11, ttf_mirp+0x12,
	ttf_mirp+0x14, ttf_mirp+0x15, ttf_mirp+0x16,
	ttf_mirp+0x18, ttf_mirp+0x19, ttf_mirp+0x1a,
	ttf_mirp+0x1c, ttf_mirp+0x1d, ttf_mirp+0x1e, -1 };

void InstrModCreate(struct instrinfo *ii) {
    fprintf( stderr, "NYI\n" );
}
