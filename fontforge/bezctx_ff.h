/*
Copyright: 2007 Raph Levien
License: GPL-2+
Modified bezctx_ps.h for FontForge by George Williams - 2007
*/

#ifndef FONTFORGE_BEZCTX_FF_H
#define FONTFORGE_BEZCTX_FF_H

#include <bezctx.h>
#include <spiroentrypoints.h>

bezctx *new_bezctx_ff(void);

struct splinepointlist;

struct splinepointlist *
bezctx_ff_close(bezctx *bc);

#endif /* FONTFORGE_BEZCTX_FF_H */
