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
    omf_active_border		= 0x4000000
};

extern void GResEditDoInit(GResInfo *ri);
extern int _GResEditInitialize(GResInfo *ri);
extern void GResEdit(GResInfo *additional,const char *def_res_file,void (*change_res_filename)(const char *));
extern void GResEditFind( struct resed *resed, char *prefix);


#endif /* FONTFORGE_GRESEDIT_H */
