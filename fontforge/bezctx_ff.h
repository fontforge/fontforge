#if 0
# include "bezctx_intf.h"
# include "bezctx.h"
#else
typedef struct _bezctx bezctx;

bezctx *
new_bezctx(void);

void
bezctx_moveto(bezctx *bc, double x, double y, int is_open);

void
bezctx_lineto(bezctx *bc, double x, double y);

void
bezctx_quadto(bezctx *bc, double x1, double y1, double x2, double y2);

void
bezctx_curveto(bezctx *bc, double x1, double y1, double x2, double y2,
	       double x3, double y3);

void
bezctx_mark_knot(bezctx *bc, int knot_idx);

struct _bezctx {
    void (*moveto)(bezctx *bc, double x, double y, int is_open);
    void (*lineto)(bezctx *bc, double x, double y);
    void (*quadto)(bezctx *bc, double x1, double y1, double x2, double y2);
    void (*curveto)(bezctx *bc, double x1, double y1, double x2, double y2,
		    double x3, double y3);
    void (*mark_knot)(bezctx *bc, int knot_idx);
};
#endif

bezctx *new_bezctx_ff(void);

struct splinepointlist;

struct splinepointlist *
bezctx_ff_close(bezctx *bc);
