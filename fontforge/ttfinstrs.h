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

#ifndef FONTFORGE_TTFINSTRS_H
#define FONTFORGE_TTFINSTRS_H

enum ttf_instructions {
 ttf_npushb=0x40, ttf_npushw=0x41, ttf_pushb=0xb0, ttf_pushw=0xb8,
 ttf_aa=0x7f, ttf_abs=0x64, ttf_add=0x60, ttf_alignpts=0x27, ttf_alignrp=0x3c,
 ttf_and=0x5a, ttf_call=0x2b, ttf_ceiling=0x67, ttf_cindex=0x25, ttf_clear=0x22,
 ttf_debug=0x4f, ttf_deltac1=0x73, ttf_deltac2=0x74, ttf_deltac3=0x75,
 ttf_deltap1=0x5d, ttf_deltap2=0x71, ttf_deltap3=0x72, ttf_depth=0x24,
 ttf_div=0x62, ttf_dup=0x20, ttf_eif=0x59, ttf_else=0x1b, ttf_endf=0x2d,
 ttf_eq=0x54, ttf_even=0x57, ttf_fdef=0x2c, ttf_flipoff=0x4e, ttf_flipon=0x4d,
 ttf_flippt=0x80, ttf_fliprgoff=0x82, ttf_fliprgon=0x81, ttf_floor=0x66,
 ttf_gc=0x46, ttf_getinfo=0x88, ttf_gfv=0x0d, ttf_gpv=0x0c, ttf_gt=0x52,
 ttf_gteq=0x53, ttf_idef=0x89, ttf_if=0x58, ttf_instctrl=0x8e, ttf_ip=0x39,
 ttf_isect=0x0f, ttf_iup=0x30, ttf_jmpr=0x1c, ttf_jrof=0x79, ttf_jrot=0x78,
 ttf_loopcall=0x2a, ttf_lt=0x50, ttf_lteq=0x51, ttf_max=0x8b, ttf_md=0x49,
 ttf_mdap=0x2e, ttf_mdrp=0xc0, ttf_miap=0x3e, ttf_min=0x8c, ttf_mindex=0x26,
 ttf_mirp=0xe0, ttf_mppem=0x4b, ttf_mps=0x4c, ttf_msirp=0x3a, ttf_mul=0x63,
 ttf_neg=0x65, ttf_neq=0x55, ttf_not=0x5c, ttf_nround=0x6c, ttf_odd=0x56,
 ttf_or=0x5b, ttf_pop=0x21, ttf_rcvt=0x45, ttf_rdtg=0x7d, ttf_roff=0x7a,
 ttf_roll=0x8a, ttf_round=0x68, ttf_rs=0x43, ttf_rtdg=0x3d, ttf_rtg=0x18,
 ttf_rthg=0x19, ttf_rutg=0x7c, ttf_s45round=0x77, ttf_sangw=0x7e,
 ttf_scanctrl=0x85, ttf_scantype=0x8d, ttf_scfs=0x48, ttf_scvtci=0x1d,
 ttf_sdb=0x5e, ttf_sdpvtl=0x86, ttf_sds=0x5f, ttf_sfvfs=0x0b, ttf_sfvtca=0x04,
 ttf_sfvtl=0x08, ttf_sfvtpv=0x0e, ttf_shc=0x34, ttf_shp=0x32, ttf_shpix=0x38,
 ttf_shz=0x36, ttf_sloop=0x17, ttf_smd=0x1a, ttf_spvfs=0x0a, ttf_spvtca=0x02,
 ttf_spvtl=0x06, ttf_sround=0x76, ttf_srp0=0x10, ttf_srp1=0x11, ttf_srp2=0x12,
 ttf_ssw=0x1f, ttf_sswci=0x1e, ttf_sub=0x61, ttf_svtca=0x00, ttf_swap=0x23,
 ttf_szp0=0x13, ttf_szp1=0x14, ttf_szp2=0x15, ttf_szps=0x16, ttf_utp=0x29,
 ttf_wcvtf=0x70, ttf_wcvtp=0x44, ttf_ws=0x42
};

extern const char *ff_ttf_instrnames[];

typedef struct instrbase {
    unsigned int inedit: 1;
    struct instrdata *instrdata;
    int isel_pos;
    int16 lheight,lpos;
    char *scroll, *offset;
} InstrBase;

extern char *__IVUnParseInstrs(InstrBase *iv);
extern int instr_typify(struct instrdata *);

#endif /* FONTFORGE_TTFINSTRS_H */
