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
#ifndef _GWIDGET_H
#define _GWIDGET_H

#include <stdarg.h>
#include "gdraw.h"
#include "gprogress.h"
#include "ggadget.h"

struct ggadget;
struct ggadgetcreatedata;
struct gtimer;

typedef GWindow GWidget;

typedef struct gwidgetcreatedata {
    GRect r;
    struct ggadgetcreatedata *gcd;
    struct gwidgetcreatedata *wcd;
    unichar_t *title;
    unsigned int trap_input: 1;
    unsigned int tab_navigation: 1;
    unsigned int arrow_navigation: 1;
    unsigned int do_default: 1;
    unsigned int do_cancel: 1;
    Color fore, back;
    void (*e_h)(GWindow, GEvent *);		/* User's event function for window, our eh will call it */
} GWidgetData;

typedef struct gwidgetcreatordata {
    GWidget *(*creator)(GWidget *parent, GWidgetData *, void *data);
    GWidgetData wd;
    void *data;
} GWidgetCreateData;

extern GWindow GWidgetCreateTopWindow(GDisplay *gdisp, GRect *pos, int (*eh)(GWindow,GEvent *), void *user_data, GWindowAttrs *wattrs);
extern GWindow GWidgetCreateSubWindow(GWindow w, GRect *pos, int (*eh)(GWindow,GEvent *), void *user_data, GWindowAttrs *wattrs);
extern GWindow GWidgetCreatePalette(GWindow w, GRect *pos, int (*eh)(GWindow,GEvent *), void *user_data, GWindowAttrs *wattrs);

GWindow GWindowGetCurrentFocusTopWindow(void);
GWindow GWidgetGetCurrentFocusWindow(void);
GWindow GWidgetGetPreviousFocusTopWindow(void);
struct ggadget *GWindowGetCurrentFocusGadget(void);
struct ggadget *GWindowGetFocusGadgetOfWindow(GWindow gw);
void GWindowClearFocusGadgetOfWindow(GWindow gw);
void GWidgetIndicateFocusGadget(struct ggadget *g);
void GWidgetNextFocus(GWindow);
void GWidgetPrevFocus(GWindow);
void GWidgetRequestVisiblePalette(GWindow palette,int visible);
void GWidgetHidePalettes(void);
void GWidgetReparentWindow(GWindow child,GWindow newparent, int x,int y);

struct ggadget *GWidgetGetControl(GWindow gw, int cid);
struct ggadget *_GWidgetGetGadgets(GWindow gw);
GWindow GWidgetGetParent(GWindow gw);
GWindow GWidgetGetTopWidget(GWindow gw);
extern GDrawEH GWidgetGetEH(GWindow w);
extern void GWidgetSetEH(GWindow w,GDrawEH e_h);
extern void GWidgetFlowGadgets(GWindow gw);
extern void GWidgetToDesiredSize(GWindow gw);

	/* Built in dialogs */
unichar_t *GWidgetOpenFile(const unichar_t *title, const unichar_t *defaultfile,
	const unichar_t *initial_filter, unichar_t **mimetypes,GFileChooserFilterType filter);
unichar_t *GWidgetSaveAsFile(const unichar_t *title, const unichar_t *defaultfile,
	const unichar_t *initial_filter, unichar_t **mimetypes,GFileChooserFilterType filter );
unichar_t *GWidgetSaveAsFileWithGadget(const unichar_t *title, const unichar_t *defaultfile,
				       const unichar_t *initial_filter, unichar_t **mimetypes,
				       GFileChooserFilterType filter,
				       GFileChooserInputFilenameFuncType filenamefunc,
				       GGadgetCreateData *optional_gcd);
char *GWidgetOpenFile8(const char *title, const char *defaultfile,
	const char *initial_filter, char **mimetypes,GFileChooserFilterType filter);
char *GWidgetOpenFileWPath8(const char *title, const char *defaultfile,
	const char *initial_filter, char **mimetypes,GFileChooserFilterType filter, const char* const* path);
char *GWidgetSaveAsFileWithGadget8(const char *title, const char *defaultfile,
				   const char *initial_filter, char **mimetypes,
				   GFileChooserFilterType filter,
				   GFileChooserInputFilenameFuncType filenamefunc,
				   GGadgetCreateData *optional_gcd);
char *GWidgetSaveAsFile8(const char *title, const char *defaultfile,
	const char *initial_filter, char **mimetypes,GFileChooserFilterType filter );
int GWidgetAsk(const unichar_t *title, const unichar_t **answers, const unichar_t *mn,
	int def, int cancel,const unichar_t *question,...);
void GWidgetError(const unichar_t *title,const unichar_t *statement,...);
unichar_t *GWidgetAskStringR(int title, const unichar_t *def,int question,...);
int GWidgetAsk8(const char *title, const char **answers,
	int def, int cancel,const char *question,...);
int GWidgetAskCentered8(const char *title,
	const char ** answers, int def, int cancel,const char *question,...);
char *GWidgetAskString8(const char *title,
	const char *def,const char *question,...);
char *GWidgetAskPassword8(const char *title,
	const char *def,const char *question,...);
void GWidgetPostNotice8(const char *title,const char *statement,...);
void _GWidgetPostNotice8(const char *title,const char *statement,va_list ap,int timeout);
void GWidgetPostNoticeTimeout8(int timeout, const char *title,const char *statement,...);
int GWidgetPostNoticeActive8(const char *title);
void GWidgetError8(const char *title,const char *statement,...);

int GWidgetChoices8(const char *title, const char **choices,int cnt, int def,
	const char *question,...);
int GWidgetChoicesB8(char *title, const char **choices, int cnt, int def,
	char *buts[2], const char *question,...);
int GWidgetChoicesBM8(char *title, const char **choices,char *sel,
	int cnt, char *buts[2], const char *question,...);

extern struct hslrgb GWidgetColor(const char *title,struct hslrgb *defcol,struct hslrgb fontcols[6]);
extern struct hslrgba GWidgetColorA(const char *title,struct hslrgba *defcol,struct hslrgba fontcols[6]);

#define gwwv_choose_multiple	GWidgetChoicesBM8
#define gwwv_choose_with_buttons	GWidgetChoicesB8
#define gwwv_choose		GWidgetChoices8
#define gwwv_ask_string		GWidgetAskString8
#define gwwv_ask_password	GWidgetAskPassword8
#define gwwv_ask		GWidgetAsk8
#define gwwv_ask_centered	GWidgetAskCentered8
#define gwwv_post_error		GWidgetError8
#define gwwv_post_notice	GWidgetPostNotice8
#define gwwv_post_notice_timeout	GWidgetPostNoticeTimeout8
#define gwwv_open_filename(tit,def,filter,filtfunc)	GWidgetOpenFile8(tit,def,filter,NULL,filtfunc)
#define gwwv_open_filename_with_path(tit,def,filter,filtfunc,path)	GWidgetOpenFileWPath8(tit,def,filter,NULL,filtfunc,path)
#define gwwv_save_filename(tit,def,filter)		GWidgetSaveAsFile8(tit,def,filter,NULL,NULL)
#define gwwv_save_filename_with_gadget(tit,def,filter,gcd)		GWidgetSaveAsFileWithGadget8(tit,def,filter,NULL,NULL,NULL,gcd)

void GWidgetCreateInsChar(void);	/* takes input even when a modal dlg is active */
		/* but is not modal itself */
void GInsCharSetChar(unichar_t ch);	/* Sets current selection in ins char dlg */

extern GIC *GWidgetCreateInputContext(GWindow w,enum gic_style def_style);
extern GIC *GWidgetGetInputContext(GWindow w);


#endif
