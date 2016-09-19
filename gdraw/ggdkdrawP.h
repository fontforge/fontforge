/**
 *  \file  ggdkdrawP.h
 *  \brief Private header file for the GDK backend.
 */

#ifndef _GGDKDRAWP_H
#define _GGDKDRAWP_H

#include "fontforge-config.h"
#ifdef FONTFORGE_CAN_USE_GDK

#include <ffgdk.h>
#include "gdrawP.h"
#include "fontP.h"

#if (((GDK_MAJOR_VERSION == 3) && (GDK_MINOR_VERSION >= 20)) || (GDK_MAJOR_VERSION > 3))
#    define GGDKDRAW_GDK_3_20
#endif

#if !defined(GDK_MAJOR_VERSION) || (GDK_MAJOR_VERSION <= 2)
#    define GGDKDRAW_GDK_2
#endif

#define GGDKDRAW_ADDREF(x) do { \
    assert((x)->reference_count >= 0); \
    (x)->reference_count++; \
} while(0)

#define GGDKDRAW_DECREF(x,y) do { \
    assert((x)->reference_count > 0); \
    (x)->reference_count--; \
    if ((x)->reference_count == 0) { \
        y(x); \
    } \
} while(0)

// Logging
//To get around a 'pedantic' C99 rule that you must have at least 1 variadic arg, combine fmt into that.
#define Log(level, ...) LogEx(level, __func__, __FILE__, __LINE__, __VA_ARGS__)

/** An enum to make the severity of log messages human readable in code **/
enum {LOGERR = 0, LOGWARN = 1, LOGNOTE = 2, LOGINFO = 3, LOGDEBUG = 4};

extern void LogEx(int level, const char *funct, const char *file, int line,  ...);   // General function for printing log messages to stderr
extern const char *GdkEventName(int code);
// End logging

// Astyle has issues with having 'enum visibility_state' in the function definition...
typedef enum visibility_state VisibilityState;
typedef struct ggdkwindow *GGDKWindow;

// Really GTimer should be opaque...
typedef struct ggdktimer { // :GTimer
    long time_sec;             // longs not int32s to match timeval
    long time_usec;
    int32 repeat_time;          // 0 == one shot (run once)
    GWindow owner;
    void *userdata;
    struct gtimer *next;       // Unused in favour of a GLib list
    unsigned int active: 1;
    // Extensions below
    unsigned int stopped: 1;
    unsigned int has_differing_repeat_time: 1;
    int reference_count;
    guint glib_timeout_id;

} GGDKTimer;

typedef struct ggdkbuttonstate {
    uint32 last_press_time;
    GGDKWindow release_w;
    int16 release_x, release_y;
    int16 release_button;
    int16 cur_click;
    int16 double_time;		// max milliseconds between release & click
    int16 double_wiggle;	// max pixel wiggle allowed between release&click
} GGDKButtonState;

typedef struct ggdkkeystate {
    GdkEventType type;
    guint state;
    guint keyval;
} GGDKKeyState;

typedef struct ggdkdndcontext {
    GGDKWindow w;
    int x, y;
    int rx, ry;
} GGDKDNDContext;

struct seldata {
    int32 typeatom;
    int32 cnt;
    int32 unitsize;
    void *data;
    void *(*gendata)(void *, int32 *len);
    /* Either the data are stored here, or we use this function to generate them on the fly */
    void (*freedata)(void *);
    struct seldata *next;
};

typedef struct ggdkselectiondata {
    /** The atom identifying the type of data held e.g. UTF8_STRING **/
    GdkAtom type_atom;
    /** Number of elements held **/
    int32_t cnt;
    /** Size per element held **/
    int32_t unit_size;
    /** Data of selection, or context data if generator is provided **/
    void *data;
    /** Function to generate selection data, or NULL if provided already **/
    void *(*gendata)(void *, int32_t *len);
    /** Method by which to free the data element **/
    void (*freedata)(void *);
} GGDKSelectionData;

typedef struct ggdkselectioninfo {
    /** The atom identifying this selection e.g. XA_PRIMARY/CLIPBOARD */
    GdkAtom sel_atom;
    /** The owner of the selection **/
    GGDKWindow owner;
    /** Time of owning the selection **/
    guint32 timestamp;
    /** Types of selection data made available **/
    GList_Glib *datalist;
} GGDKSelectionInfo;

typedef struct ggdkselectiontypes {
    /** Time at which this value was updated **/
    guint32 timestamp;
    /** List of types available (targets) **/
    GdkAtom *types;
    /** Number of targets available **/
    int32_t len;
    /** The selection for which we're getting targets for **/
    GdkAtom sel_atom;
} GGDKSelectionTypes;

typedef struct ggdkdisplay { /* :GDisplay */
    // Inherit GDisplay start
    struct displayfuncs *funcs;
    void *semaphore;
    struct font_state *fontstate;
    int16 res;
    int16 scale_screen_by;
    GGDKWindow groot;
    Color def_background, def_foreground;
    uint16 mykey_state;
    uint16 mykey_keysym;
    uint16 mykey_mask;
    fd_callback_t fd_callbacks[ gdisplay_fd_callbacks_size ];
    int fd_callbacks_last;
    unsigned int mykeybuild: 1;
    unsigned int default_visual: 1;
    unsigned int do_dithering: 1;
    unsigned int focusfollowsmouse: 1;
    unsigned int top_offsets_set: 1;
    unsigned int wm_breaks_raiseabove: 1;
    unsigned int wm_raiseabove_tested: 1;
    unsigned int endian_mismatch: 1;
    unsigned int macosx_cmd: 1;     /* if set then map state=0x20 to control */
    unsigned int twobmouse_win: 1;  /* if set then map state=0x40 to mouse button 2 */
    unsigned int devicesinit: 1;    /* the devices structure has been initialized. Else call XListInputDevices */
    unsigned int expecting_core_event: 1; /* when we move an input extension device we generally get two events, one for the device, one later for the core device. eat the core event */
    unsigned int has_xkb: 1;        /* we were able to initialize the XKB extension */
    unsigned int supports_alpha_images: 1;
    unsigned int supports_alpha_windows: 1;
    int err_flag;
    char *err_report;
    // Inherit GDisplay end

    unsigned int is_space_pressed: 1; // Used for GGDKDrawKeyState. We cheat!
    unsigned int is_dying: 1; // Flag to indicate if we're cleaning up the display.

    int     top_window_count;
    guint32 last_event_time;

    GGDKSelectionInfo selinfo[sn_max]; // We implement the clipboard using the selections model
    GGDKSelectionTypes seltypes;
    int sel_notify_timeout;
    GGDKDNDContext last_dd; // Drag and drop

    GPtrArray *cursors; // List of cursors that the user made.
    GGDKWindow dirty_window; // Window which called drawing functions outside of an expose event.
    GList_Glib *timers; //List of GGDKTimer's
    GHashTable *windows; // List of windows.

    GGDKButtonState bs;
    GGDKKeyState ks;
    GGDKWindow default_icon;
    GGDKWindow last_nontransient_window;

    GMainLoop  *main_loop;
    GdkDisplay *display;
    GdkScreen  *screen;
    GdkWindow  *root;

    PangoContext *pangoc_context;
} GGDKDisplay;

struct ggdkwindow { /* :GWindow */
    // Inherit GWindow start
    GGC *ggc;
    GGDKDisplay *display;
    int (*eh)(GWindow, GEvent *);
    GRect pos;
    struct ggdkwindow *parent;
    void *user_data;
    void *widget_data;
    GdkWindow *w;
    unsigned int is_visible: 1;
    unsigned int is_pixmap: 1;
    unsigned int is_toplevel: 1;
    unsigned int visible_request: 1;
    unsigned int is_dying: 1;
    unsigned int is_popup: 1;
    unsigned int disable_expose_requests: 1;
    unsigned int usecairo: 1;
    char *window_type_name;
    //char pad[4];
    // Inherit GWindow end
    unsigned int is_dlg: 1;
    unsigned int not_restricted: 1;
    unsigned int was_positioned: 1;
    unsigned int restrict_input_to_me: 1;/* for dialogs, no input outside of dlg */
    unsigned int redirect_chars_to_me: 1;/* ditto, we get any input outside of us */
    unsigned int istransient: 1;	/* has transient for hint set */
    unsigned int isverytransient: 1;
    unsigned int is_cleaning_up: 1; //Are we running cleanup?
    unsigned int is_centered: 1;
    unsigned int is_waiting_for_selection: 1;
    unsigned int is_notified_of_selection: 1;
    unsigned int is_in_paint: 1; // Have we called gdk_window_begin_paint_region?
    unsigned int has_had_faked_configure: 1;

    int reference_count; // Knowing when to destroy is tricky...
    GPtrArray *transient_childs; // Handling transients is tricky...
    guint resize_timeout; // Resizing is tricky...

    GWindow redirect_from;		/* only redirect input from this window and its children */
    struct ggdkwindow *transient_owner;

    char *window_title;
    GCursor current_cursor;

    cairo_region_t *expose_region;
    cairo_surface_t *cs;
    cairo_t *cc;
    PangoLayout *pango_layout;
};

// Functions in ggdkcdraw.c

bool _GGDKDraw_InitPangoCairo(GGDKWindow gw);
void _GGDKDraw_CleanupAutoPaint(GGDKDisplay *gdisp);

#ifdef GGDKDRAW_GDK_2
GdkPixbuf *_GGDKDraw_Cairo2Pixbuf(cairo_surface_t *cs);
#else
cairo_region_t *_GGDKDraw_CalculateDrawableRegion(GGDKWindow gw, bool force);
void _GGDKDraw_ClipToRegion(GGDKWindow gw, cairo_region_t *r);
#endif

void GGDKDrawPushClip(GWindow w, GRect *rct, GRect *old);
void GGDKDrawPopClip(GWindow gw, GRect *old);
void GGDKDrawSetDifferenceMode(GWindow gw);
void GGDKDrawClear(GWindow gw, GRect *rect);
void GGDKDrawDrawLine(GWindow w, int32 x, int32 y, int32 xend, int32 yend, Color col);
void GGDKDrawDrawArrow(GWindow gw, int32 x, int32 y, int32 xend, int32 yend, int16 arrows, Color col);
void GGDKDrawDrawRect(GWindow gw, GRect *rect, Color col);
void GGDKDrawFillRect(GWindow gw, GRect *rect, Color col);
void GGDKDrawFillRoundRect(GWindow gw, GRect *rect, int radius, Color col);
void GGDKDrawDrawEllipse(GWindow gw, GRect *rect, Color col);
void GGDKDrawFillEllipse(GWindow gw, GRect *rect, Color col);
void GGDKDrawDrawArc(GWindow gw, GRect *rect, int32 sangle, int32 eangle, Color col);
void GGDKDrawDrawPoly(GWindow gw, GPoint *pts, int16 cnt, Color col);
void GGDKDrawFillPoly(GWindow gw, GPoint *pts, int16 cnt, Color col);
void GGDKDrawDrawImage(GWindow gw, GImage *gimg, GRect *src, int32 x, int32 y);
void GGDKDrawDrawGlyph(GWindow gw, GImage *gimg, GRect *src, int32 x, int32 y);
void GGDKDrawDrawImageMagnified(GWindow gw, GImage *gimg, GRect *src, int32 x, int32 y, int32 width, int32 height);

void    GGDKDrawDrawPixmap(GWindow gw1, GWindow gw2, GRect *src, int32 x, int32 y);

enum gcairo_flags GGDKDrawHasCairo(GWindow w);

void GGDKDrawPathStartNew(GWindow w);
void GGDKDrawPathClose(GWindow w);
void GGDKDrawPathMoveTo(GWindow w, double x, double y);
void GGDKDrawPathLineTo(GWindow w, double x, double y);
void GGDKDrawPathCurveTo(GWindow w, double cx1, double cy1, double cx2, double cy2, double x, double y);
void GGDKDrawPathStroke(GWindow w, Color col);
void GGDKDrawPathFill(GWindow w, Color col);
void GGDKDrawPathFillAndStroke(GWindow w, Color fillcol, Color strokecol);
void GGDKDrawStartNewSubPath(GWindow w);
int  GGDKDrawFillRuleSetWinding(GWindow w);
int  GGDKDrawDoText8(GWindow w, int32 x, int32 y, const char *text, int32 cnt, Color col, enum text_funcs drawit, struct tf_arg *arg);

void GGDKDrawPushClipOnly(GWindow w);
void GGDKDrawClipPreserve(GWindow w);

void GGDKDrawGetFontMetrics(GWindow gw, GFont *fi, int *as, int *ds, int *ld);
void GGDKDrawLayoutInit(GWindow w, char *text, int cnt, GFont *fi);
void GGDKDrawLayoutDraw(GWindow w, int32 x, int32 y, Color fg);
void GGDKDrawLayoutIndexToPos(GWindow w, int index, GRect *pos);
int  GGDKDrawLayoutXYToIndex(GWindow w, int x, int y);
void GGDKDrawLayoutExtents(GWindow w, GRect *size);
void GGDKDrawLayoutSetWidth(GWindow w, int width);
int  GGDKDrawLayoutLineCount(GWindow w);
int  GGDKDrawLayoutLineStart(GWindow w, int l);

// END functions in ggdkcdraw.c

// Functions in ggdkcocoa.m

char *_GGDKDrawCocoa_GetClipboardText(void);
void _GGDKDrawCocoa_SetClipboardText(const char *text);

// END functions in ggdkcocoa.m

#endif // FONTFORGE_CAN_USE_GDK

#endif // _GGDKDRAWP_H
