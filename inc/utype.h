/* This is a GENERATED file - from makeutype.py with Unicode 17.0.0 */

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

#ifndef FONTFORGE_UNICODE_UTYPE2_H
#define FONTFORGE_UNICODE_UTYPE2_H

#include <ctype.h> /* Include here so we can control it. If a system header includes it later bad things happen */
#include "basics.h" /* Include here so we can use pre-defined int types to correctly size constant data arrays. */

#ifdef __cplusplus
extern "C" {
#endif

/* Unicode basics */
#define UNICODE_VERSION "17.0.0"
#define UNICODE_MAX 0x110000

/* Pose flags */
#define FF_UNICODE_NOPOSDATAGIVEN  -1
#define FF_UNICODE_ABOVE           0x100
#define FF_UNICODE_BELOW           0x200
#define FF_UNICODE_OVERSTRIKE      0x400
#define FF_UNICODE_LEFT            0x800
#define FF_UNICODE_RIGHT           0x1000
#define FF_UNICODE_JOINS2          0x2000
#define FF_UNICODE_CENTERLEFT      0x4000
#define FF_UNICODE_CENTERRIGHT     0x8000
#define FF_UNICODE_CENTEREDOUTSIDE 0x10000
#define FF_UNICODE_OUTSIDE         0x20000
#define FF_UNICODE_RIGHTEDGE       0x40000
#define FF_UNICODE_LEFTEDGE        0x80000
#define FF_UNICODE_TOUCHING        0x100000

extern int ff_unicode_isunicodepointassigned(unichar_t ch);
extern int ff_unicode_isalpha(unichar_t ch);
extern int ff_unicode_isideographic(unichar_t ch);
extern int ff_unicode_islefttoright(unichar_t ch);
extern int ff_unicode_isrighttoleft(unichar_t ch);
extern int ff_unicode_islower(unichar_t ch);
extern int ff_unicode_isupper(unichar_t ch);
extern int ff_unicode_isdigit(unichar_t ch);
extern int ff_unicode_isligvulgfrac(unichar_t ch);
extern int ff_unicode_iscombining(unichar_t ch);
extern int ff_unicode_iszerowidth(unichar_t ch);
extern int ff_unicode_iseuronumeric(unichar_t ch);
extern int ff_unicode_iseuronumterm(unichar_t ch);
extern int ff_unicode_isarabnumeric(unichar_t ch);
extern int ff_unicode_isdecompositionnormative(unichar_t ch);
extern int ff_unicode_isdecompcircle(unichar_t ch);
extern int ff_unicode_isarabinitial(unichar_t ch);
extern int ff_unicode_isarabmedial(unichar_t ch);
extern int ff_unicode_isarabfinal(unichar_t ch);
extern int ff_unicode_isarabisolated(unichar_t ch);
extern int ff_unicode_isideoalpha(unichar_t ch);
extern int ff_unicode_isalnum(unichar_t ch);
extern int ff_unicode_isspace(unichar_t ch);
extern int ff_unicode_iseuronumsep(unichar_t ch);
extern int ff_unicode_iscommonsep(unichar_t ch);
extern int ff_unicode_ishexdigit(unichar_t ch);
extern int ff_unicode_istitle(unichar_t ch);
extern int ff_unicode_pose(unichar_t ch);
extern int ff_unicode_combiningclass(unichar_t ch);
extern unichar_t ff_unicode_tolower(unichar_t ch);
extern unichar_t ff_unicode_toupper(unichar_t ch);
extern unichar_t ff_unicode_totitle(unichar_t ch);
extern unichar_t ff_unicode_tomirror(unichar_t ch);
extern int ff_unicode_hasunialt(unichar_t ch);
extern const unichar_t* ff_unicode_unialt(unichar_t ch);

#undef isunicodepointassigned
#undef isalpha
#undef isideographic
#undef islefttoright
#undef isrighttoleft
#undef islower
#undef isupper
#undef isdigit
#undef isligvulgfrac
#undef iscombining
#undef iszerowidth
#undef iseuronumeric
#undef iseuronumterm
#undef isarabnumeric
#undef isdecompositionnormative
#undef isdecompcircle
#undef isarabinitial
#undef isarabmedial
#undef isarabfinal
#undef isarabisolated
#undef isideoalpha
#undef isalnum
#undef isspace
#undef iseuronumsep
#undef iscommonsep
#undef ishexdigit
#undef istitle
#undef pose
#undef combiningclass
#undef tolower
#undef toupper
#undef totitle
#undef tomirror
#undef hasunialt
#undef unialt

#define isunicodepointassigned(ch)   ff_unicode_isunicodepointassigned((ch))
#define isalpha(ch)                  ff_unicode_isalpha((ch))
#define isideographic(ch)            ff_unicode_isideographic((ch))
#define islefttoright(ch)            ff_unicode_islefttoright((ch))
#define isrighttoleft(ch)            ff_unicode_isrighttoleft((ch))
#define islower(ch)                  ff_unicode_islower((ch))
#define isupper(ch)                  ff_unicode_isupper((ch))
#define isdigit(ch)                  ff_unicode_isdigit((ch))
#define isligvulgfrac(ch)            ff_unicode_isligvulgfrac((ch))
#define iscombining(ch)              ff_unicode_iscombining((ch))
#define iszerowidth(ch)              ff_unicode_iszerowidth((ch))
#define iseuronumeric(ch)            ff_unicode_iseuronumeric((ch))
#define iseuronumterm(ch)            ff_unicode_iseuronumterm((ch))
#define isarabnumeric(ch)            ff_unicode_isarabnumeric((ch))
#define isdecompositionnormative(ch) ff_unicode_isdecompositionnormative((ch))
#define isdecompcircle(ch)           ff_unicode_isdecompcircle((ch))
#define isarabinitial(ch)            ff_unicode_isarabinitial((ch))
#define isarabmedial(ch)             ff_unicode_isarabmedial((ch))
#define isarabfinal(ch)              ff_unicode_isarabfinal((ch))
#define isarabisolated(ch)           ff_unicode_isarabisolated((ch))
#define isideoalpha(ch)              ff_unicode_isideoalpha((ch))
#define isalnum(ch)                  ff_unicode_isalnum((ch))
#define isspace(ch)                  ff_unicode_isspace((ch))
#define iseuronumsep(ch)             ff_unicode_iseuronumsep((ch))
#define iscommonsep(ch)              ff_unicode_iscommonsep((ch))
#define ishexdigit(ch)               ff_unicode_ishexdigit((ch))
#define istitle(ch)                  ff_unicode_istitle((ch))
#define pose(ch)                     ff_unicode_pose((ch))
#define combiningclass(ch)           ff_unicode_combiningclass((ch))
#define tolower(ch)                  ff_unicode_tolower((ch))
#define toupper(ch)                  ff_unicode_toupper((ch))
#define totitle(ch)                  ff_unicode_totitle((ch))
#define tomirror(ch)                 ff_unicode_tomirror((ch))
#define hasunialt(ch)                ff_unicode_hasunialt((ch))
#define unialt(ch)                   ff_unicode_unialt((ch))

struct arabicforms {
    unsigned short initial, medial, final, isolated;
    unsigned int isletter: 1;
    unsigned int joindual: 1;
    unsigned int required_lig_with_alef: 1;
};

extern const struct arabicforms* arabicform(unichar_t ch);

struct unicode_range {
    unichar_t start;
    unichar_t end;
    unichar_t first_char;
    int num_assigned;
    const char *name;
};

extern char* uniname_name(unichar_t ch);
extern char* uniname_annotation(unichar_t ch, int prettify);
extern char* uniname_formal_alias(unichar_t ch);
extern const struct unicode_range* uniname_block(unichar_t ch);
extern const struct unicode_range* uniname_plane(unichar_t ch);
extern const struct unicode_range* uniname_blocks(int *sz);
extern const struct unicode_range* uniname_planes(int *sz);

#ifdef __cplusplus
}
#endif

#endif /* FONTFORGE_UNICODE_UTYPE2_H */
