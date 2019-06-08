/* Copyright (C) 2000-2012 by George Williams */
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

#ifndef FONTFORGE_SFD1_H
#define FONTFORGE_SFD1_H

/* This file contains the data structures needed to read in an old sfd file */
/* features and lookups and scripts are handled differently. That means that */
/* the KernPair, KernClass, PST, FPST, AnchorClass, StateMachine data structures */
/* are organized differently. Also we've got a script language list which */
/* doesn't exist in the new format and we don't have OTLookup */

#include "splinefont.h"

#define SLI_UNKNOWN		0xffff
#define SLI_NESTED		0xfffe

typedef struct anchorclass1 {
    AnchorClass ac;
    uint32 feature_tag;
    uint16 script_lang_index;
    uint16 flags;
    uint16 merge_with;
    uint8 has_bases;
    uint8 has_ligatures;
} AnchorClass1;

typedef struct kernpair1 {
    KernPair kp;
    uint16 sli, flags;
} KernPair1;

typedef struct kernclass1 {
    KernClass kc;
    uint16 sli;
    uint16 flags;
} KernClass1;

typedef struct generic_pst1 {
    PST pst;
    uint8 macfeature;		/* tag should be interpretted as <feature,setting> rather than 'abcd' */
    uint16 flags;
    uint16 script_lang_index;		/* 0xffff means none */
    uint32 tag;
} PST1;

typedef struct generic_fpst1 {
    FPST fpst;
    uint16 script_lang_index;
    uint16 flags;
    uint32 tag;
} FPST1;

typedef struct generic_asm1 {		/* Apple State Machine */
    ASM sm;
    uint16 feature, setting;
    uint32 opentype_tag;		/* If converted from opentype */
} ASM1;

struct table_ordering {
    uint32 table_tag;
    uint32 *ordered_features;
    struct table_ordering *next;
};

struct script_record {
    uint32 script;
    uint32 *langs;
};

struct tagtype {
    enum possub_type type;
    uint32 tag;
};

struct gentagtype {
    uint16 tt_cur, tt_max;
    struct tagtype *tagtype;
};

typedef struct splinefont1 {
    SplineFont sf;

    struct table_ordering *orders;

    /* Any GPOS/GSUB entry (PST, AnchorClass, kerns, FPST */
    /*  Has an entry saying what scripts/languages it should appear it */
    /*  Things like fractions will appear in almost all possible script/lang */
    /*  combinations, while alphabetic ligatures will only live in one script */
    /* Rather than store the complete list of possibilities in each PST we */
    /*  store all choices used here, and just store an index into this list */
    /*  in the PST. All lists are terminated by a 0 entry */
    struct script_record **script_lang;
    int16 sli_cnt;

    struct gentagtype gentags;
} SplineFont1;

extern int SFFindBiggestScriptLangIndex(SplineFont *_sf,uint32 script,uint32 lang);
extern int SFAddScriptIndex(SplineFont1 *sf,uint32 *scripts,int scnt);
extern void SFD_AssignLookups(SplineFont1 *sf);

extern enum uni_interp interp_from_encoding(Encoding *enc, enum uni_interp interp);

#endif /* FONTFORGE_SFD1_H */
