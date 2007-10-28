#include "spiroentrypoints.h"
#include "bezctx.h"

bezctx *new_bezctx_ff(void);

struct splinepointlist;

struct splinepointlist *
bezctx_ff_close(bezctx *bc);
