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

#ifndef FONTFORGE_GRESOURCE_H
#define FONTFORGE_GRESOURCE_H

#include "gimage.h"

#include <iconv.h>

typedef struct ggadgetcreatedata GGadgetCreateData;
typedef struct _PangoFontDescription PangoFontDescription;

enum border_type { bt_none, bt_box, bt_raised, bt_lowered, bt_engraved,
	    bt_embossed, bt_double };
enum border_shape { bs_rect, bs_roundrect, bs_elipse, bs_diamond };
enum box_flags {
    box_foreground_border_inner = 1,	/* 1 point line */
    box_foreground_border_outer = 2,	/* 1 point line */
    box_active_border_inner = 4,		/* 1 point line */
    box_foreground_shadow_outer = 8,	/* 1 point line, bottom&right */
    box_do_depressed_background = 0x10,
    box_draw_default = 0x20,	/* if a default button draw a depressed rect around button */
    box_generate_colors = 0x40,	/* use border_brightest to compute other border cols */
    box_gradient_bg = 0x80,
    box_flag_mask = 0xFF
    };
typedef struct gbox {
    unsigned char border_type;	
    unsigned char border_shape;	
    unsigned char border_width;	/* In points */
    unsigned char padding;	/* In points */
    unsigned char rr_radius;	/* In points */
    unsigned char flags;
    Color border_brightest;		/* used for left upper part of elipse */
    Color border_brighter;
    Color border_darkest;		/* used for right lower part of elipse */
    Color border_darker;
    Color main_background;
    Color main_foreground;
    Color disabled_background;
    Color disabled_foreground;
    Color active_border;
    Color depressed_background;
    Color gradient_bg_end;
    Color border_inner;
    Color border_outer;
} GBox;

#define GBOX_EMPTY { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0 ,0 ,0 }

enum font_style { fs_none, fs_italic=1, fs_smallcaps=2, fs_condensed=4, fs_extended=8, fs_vertical=16 };
enum font_type { ft_unknown, ft_serif, ft_sans, ft_mono, ft_cursive, ft_max };

typedef struct {
    const char *utf8_family_name;	/* may be more than one */
    int16_t point_size;			/* negative values are in pixels */
    int16_t weight;
    enum font_style style;
} FontRequest;

typedef struct font_instance {
    FontRequest rq;		/* identification of this instance */
    PangoFontDescription *pango_fd;
    PangoFontDescription *pangoc_fd;
} FontInstance, GFont;

enum res_type { rt_int, rt_double, rt_bool/* int */, rt_color, rt_string, rt_image, rt_font, rt_stringlong, rt_coloralpha };

struct gimage_cache_bucket;

typedef struct gresimage {
    const char *ini_name;
    struct gimage_cache_bucket *bucket;
} GResImage;

typedef struct gresfont {
    GFont *fi;
    const char *rstr;
    uint8_t can_free_name;
} GResFont;

/* Locate and override the "windows-cjk-workaround" font alias, because the
   native Pango alias "system-ui" doesn't support CJK scripts on Windows */
void fix_CJK_UI_font(GResFont* font);

#define GRESIMAGE_INIT(defstr) { (defstr), NULL }
#define GRESFONT_INIT(defstr) { NULL, (defstr), false }

GImage *GResImageGetImage(GResImage *);

typedef struct gresstruct {
    const char *resname;
    enum res_type type;
    void *val;
    void *(*cvt)(char *,void *);	/* converts a string into a whatever */
    int found;
} GResStruct;

#define GRESSTRUCT_EMPTY { NULL, 0, NULL, NULL, 0 }

struct resed {
    const char *name, *resname;
    enum res_type type;
    void *val;
    char *popup;
    void *(*cvt)(char *,void *);
    union { int ival; double dval; char *sval; GResFont fontval; GResImage imageval; } orig;
    int cid;
    int found;
};

#define RESED_EMPTY { NULL, NULL, 0, NULL, NULL, NULL, { 0 }, 0, 0 }

typedef struct gresinfo {
    struct gresinfo *next;
    struct gresinfo *inherits_from;
    struct gresinfo *seealso1, *seealso2;
    GBox *boxdata;
    GResFont *font;
    GGadgetCreateData *examples;
    struct resed *extras;
    char *name;
    char *initialcomment;
    char *resname;
    char *progname;
    uint8_t is_button;		/* Activate default button border flag */
    uint8_t is_initialized;
    uint32_t override_mask;
    GBox overrides;
    GBox orig_state;
    void (*refresh)(void);	/* Called when user OKs the resource editor dlg */
    int (*initialize)(struct gresinfo *);
    void *reserved_for_future_use2;
} GResInfo;

extern char *GResourceProgramName;

int ResStrToFontRequest(const char *resname, FontRequest *rq);

void GResourceSetProg(const char *prog);
void GResourceAddResourceFile(const char *filename,const char *prog,int warn);
void GResourceAddResourceString(const char *string,const char *prog);

#endif /* FONTFORGE_GRESOURCE_H */
