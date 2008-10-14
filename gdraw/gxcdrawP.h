#ifndef _NO_LIBCAIRO
extern int _GXCDraw_hasCairo(void);

extern void _GXCDraw_NewWindow(GXWindow nw,Color bg);
extern void _GXCDraw_ResizeWindow(GXWindow gw,GRect *rect);
extern void _GXCDraw_DestroyWindow(GXWindow gw);

extern void _GXCDraw_PushClip(GXWindow gw);
extern void _GXCDraw_PopClip(GXWindow gw);

extern void _GXCDraw_Clear(GXWindow gw, GRect *rect);
extern void _GXCDraw_DrawLine(GXWindow gw, int32 x,int32 y, int32 xend,int32 yend);
extern void _GXCDraw_DrawRect(GXWindow gw, GRect *rect);
extern void _GXCDraw_FillRect(GXWindow gw, GRect *rect);
extern void _GXCDraw_DrawEllipse(GXWindow gw, GRect *rect);
extern void _GXCDraw_FillEllipse(GXWindow gw, GRect *rect);
extern void _GXCDraw_DrawPoly(GXWindow gw, GPoint *pts, int16 cnt);
extern void _GXCDraw_FillPoly(GXWindow gw, GPoint *pts, int16 cnt);

extern void _GXCDraw_Image( GXWindow gw, GImage *image, GRect *src, int32 x, int32 y);
extern void _GXCDraw_TileImage( GXWindow gw, GImage *image, GRect *src, int32 x, int32 y);
extern void _GXCDraw_Glyph( GXWindow gw, GImage *image, GRect *src, int32 x, int32 y);;
extern void _GXCDraw_ImageMagnified(GXWindow gw, GImage *image, GRect *magsrc,
	int32 x, int32 y, int32 width, int32 height);

extern void _GXCDraw_PathStartNew(GWindow w);
extern void _GXCDraw_PathMoveTo(GWindow w,double x, double y);
extern void _GXCDraw_PathLineTo(GWindow w,double x, double y);
extern void _GXCDraw_PathCurveTo(GWindow w,
		    double cx1, double cy1,
		    double cx2, double cy2,
		    double x, double y);
extern void _GXCDraw_PathStroke(GWindow w,Color col);
extern void _GXCDraw_PathFill(GWindow w,Color col);
extern void _GXCDraw_PathFillAndStroke(GWindow w,Color fillcol, Color strokecol);

#include "fontP.h"
extern void _GXCDraw_FontMetrics(GXWindow w,GFont *fi,int *as, int *ds, int *ld);
extern int32 _GXCDraw_DoText8(GWindow gw, int32 x, int32 y,
	const char *text, int32 cnt, FontMods *mods, Color col,
	enum text_funcs drawit, struct tf_arg *arg);
extern int32 _GXCDraw_DoText(GWindow gw, int32 x, int32 y,
	unichar_t *text, int32 cnt, FontMods *mods, Color col,
	enum text_funcs drawit, struct tf_arg *arg);
#endif
