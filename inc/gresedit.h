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

#ifndef FONTFORGE_GRESEDIT_H
#define FONTFORGE_GRESEDIT_H

#include "basics.h"
#include "ggadget.h"
#include "gresource.h"

enum res_type2 { rt_stringlong = rt_string+1, rt_coloralpha, rt_image, rt_font };

struct resed {
    const char *name, *resname;
    enum res_type type;
    void *val;
    char *popup;
    void *(*cvt)(char *,void *);
    union { int ival; double dval; char *sval; GFont *fontval; } orig;
    int cid;
    int found;
};

#define RESED_EMPTY { NULL, NULL, 0, NULL, NULL, NULL, { 0 }, 0, 0 }


typedef struct gresinfo {
    struct gresinfo *next;
    struct gresinfo *inherits_from;
    struct gresinfo *seealso1, *seealso2;
    GBox *boxdata;
    GFont **font;
    GGadgetCreateData *examples;
    struct resed *extras;
    char *name;
    char *initialcomment;
    char *resname;
    char *progname;
    uint8 is_button;		/* Activate default button border flag */
    uint32 override_mask;
    GBox *overrides;
    GBox orig_state;
    void (*refresh)(void);	/* Called when user OKs the resource editor dlg */
    void *reserved_for_future_use1;
    void *reserved_for_future_use2;
} GResInfo;

enum override_mask_flags {
    /* First 8 flags are the enum box_flags */
    omf_border_type	= 0x100,
    omf_border_shape	= 0x200,
    omf_border_width	= 0x400,
    omf_padding		= 0x800,
    omf_rr_radius	= 0x1000,

    omf_refresh         = 0x2000, /* Not a real override flag. A hack */
				/* I use it to indicate that the refresh field*/
			        /* is meaningful. That field didn't exist in  */
			        /* old versions of the library, adding this   */
			        /* mask means we don't need to break binary   */
			        /* compatibility */

    omf_main_foreground		= 0x10000,
    omf_disabled_foreground	= 0x20000,
    omf_main_background		= 0x40000,
    omf_disabled_background	= 0x80000,
    omf_depressed_background	= 0x100000,
    omf_gradient_bg_end		= 0x200000,
    omf_border_brightest	= 0x400000,
    omf_border_brighter		= 0x800000,
    omf_border_darkest		= 0x1000000,
    omf_border_darker		= 0x2000000,
    omf_active_border		= 0x4000000,

    omf_font			= 0x80000000
};

extern void GResEdit(GResInfo *additional,const char *def_res_file,void (*change_res_filename)(const char *));
extern void GResEditFind( struct resed *resed, char *prefix);

#endif /* FONTFORGE_GRESEDIT_H */
