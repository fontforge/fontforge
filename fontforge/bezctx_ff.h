/*
Copyright: 2007 Raph Levien
License: GPL-2+
Modified bezctx_ps.h for FontForge by George Williams - 2007
*/

#include <spiroentrypoints.h>
#include <bezctx.h>

bezctx *new_bezctx_ff(void);

struct splinepointlist;

struct splinepointlist *
bezctx_ff_close(bezctx *bc);
