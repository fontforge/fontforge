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
#pragma once

struct valdev;
struct splinechar;
struct kernpair;
struct kernclass;

struct vr {
    int16_t xoff, yoff, h_adv_off, v_adv_off;
    struct valdev* adjust;
};

struct opentype_str {
    struct splinechar* sc;
    struct vr vr; /* Scaled and rounded gpos modifications (device table info
                     included in xoff, etc. not in adjusts) */
    struct kernpair* kp;
    struct kernclass* kc;
    unsigned int prev_kc0 : 1;
    unsigned int next_kc0 : 1;
    int16_t advance_width; /* Basic advance, modifications in vr, scaled and
                              rounded */
    /* Er... not actually set by ApplyLookups, but somewhere the caller */
    /*  can stash info. (Extract width from hinted bdf if possible, tt */
    /*  instructions can change it from the expected value) */
    int16_t kc_index;
    int16_t lig_pos;     /* when skipping marks to form a ligature keep track of
                            what ligature element a mark was attached to */
    int16_t context_pos; /* When doing a contextual match remember which glyphs
                            are used, and where in the match they occur. Skipped
                            glyphs have -1 */
    int32_t orig_index;
    void* fl;
    unsigned int line_break_after : 1;
    unsigned int r2l : 1;
    int16_t bsln_off;
};
