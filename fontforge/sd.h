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

#ifndef FONTFORGE_SD_H
#define FONTFORGE_SD_H

# include "gimage.h"

/* All coordinates are in millimeters */
/* they will be displayed to the user scaled by the units field of the design */

#include "splinefont.h"

struct epattern {
    struct entity *tile;
    real width, height;
    DBounds bbox;
    real transform[6];
};

typedef struct entpen {
    Color col;
    struct gradient *grad;
    struct epattern *tile;
    float scale;
    float opacity;
} Pen;

typedef struct textunit {
    unichar_t *text;
    SplineFont *sf;
    float size;			/* in points */
    float kernafter;
    Pen fill;
    struct textunit *next;
} TextUnit;

struct filledsplines {
    SplineSet *splines;
    unsigned int isfillable: 1;		/* All splinesets are closed */
    Pen fill, stroke;			/* A value of 0xffffffff means do not fill or stroke */
    float stroke_width;
    float miterlimit;
    enum linejoin join;
    enum linecap cap;
    real transform[6];			/* The stroke may be quite different depending on the transformation (ie. ellipse not circle, rotated, etc) */
};

struct text {
    TextUnit *text;
    real transform[6];
    struct entity *bound;
};

struct image {
    GImage *image;
    real transform[6];
    Color col;				/* that gets poured into imagemasks */
};

struct e_group {
    struct entity *group;
};

enum entity_type { et_splines, et_text, et_image, et_group };

typedef struct entity {
    enum entity_type type;
    union {
	struct filledsplines splines;
	struct text text;
	struct image image;
	struct e_group group;
    } u;
    SplineSet *clippath;
    DBounds bb;
    struct entity *next;
} Entity;
	    
typedef struct entlayer {
    Entity *entities;
    char *name;
    unsigned int isvisible: 1;
} EntLayer;

typedef struct tile {
    Entity *tile;
    struct tileinstance { real scale; struct gwindow *pixmap; struct tileinstance *next; }
	    *instances;
    char *name;
} Tile;

typedef struct splinedesign {
    int lcnt, lmax, active;
    EntLayer *layers;

    real width, height;		/* in millimeters */
    int16 hpages, vpages;
    real pwidth, pheight;		/* in millimeters */
    real units;			/* if user wants to see things in */
	/* centimeters then units will be 10, if inches then 25.4, if points */
	/* then 25.4/72, if 1/1200" then 25.4/1200, etc. */
    struct dview *dvs;
} SplineDesign, Design;


	/* Used for type3 fonts briefly */
/* This is not a "real" structure. It is a temporary hack that encompasses */
/*  various possibilities, the combination of which won't occur in reality */
typedef struct entitychar {
    Entity *splines;
    RefChar *refs;
    int width, vwidth;
    SplineChar *sc;
    uint8 fromtype3;
} EntityChar;


struct pskeydict {
    int16 cnt, max;
    uint8 is_executable;
    struct pskeyval *entries;
};

enum pstype { ps_void, ps_num, ps_bool, ps_string, ps_instr, ps_lit,
	      ps_mark, ps_array, ps_dict };

union vals {
    real val;
    int tf;
    char *str;
    struct pskeydict dict;		/* and for arrays too */
};

struct psstack {
    enum pstype type;
    union vals u;
};

struct pskeyval {
    enum pstype type;
    union vals u;
    char *key;
};

typedef struct retstack {
    int max;
    int cnt;
    real *stack;
} RetStack;

enum shown_params {
    sp_svg = 1,
    sp_eps = 2,
    sp_scale = 4
};

typedef struct importparams {
    // Could be bits but the python interface would be annoying
    int initialized;
    int shown_mask, show_always;

    int correct_direction;	// PS
    int simplify;
    int clip;			// SVG
    int erasers;		// PS
    int scale;			// Misc
    bigreal accuracy_target;
    bigreal default_joinlimit;
} ImportParams;

typedef struct exportparams {
    // Could be bits but the python interface would be annoying
    int initialized;
    int shown_mask, show_always;

    int use_transform;		// SVG
} ExportParams;

extern void InitImportParams(ImportParams *ip);
extern ImportParams *ImportParamsState(void);
extern void InitExportParams(ExportParams *ep);
extern ExportParams *ExportParamsState(void);

#endif /* FONTFORGE_SD_H */
