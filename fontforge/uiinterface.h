/* Copyright (C) 2007,2008 by George Williams */
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
#ifndef _UIINTERFACE_H
#define _UIINTERFACE_H
# include "basics.h"
/* This encapsulates a set of callbacks and stubs. The callbacks get activated*/
/*  when an event happens (a glyph in a font changes for example, then all */
/*  charviews looking at it must be updated), and the stubs provide some simple*/
/*  UI routines: Post an error, etc. */

/* ************************************************************************** */
/* Basic, low-level UI routines for events we discover deep inside script code*/
/* ************************************************************************** */

struct ui_interface {
   /* The following is used to post a fontforge internal error */
   /* currently it puts up a dlg displaying the error text */
    void (*ierror)(const char *fmt,...);

   /* The following is a simple dialog to alert the user that s/he has */
   /*  made an error. Currently it posts a modal dlg and waits for the */
   /*  user to dismiss it */
   /* The title argument is the window's title. The error argument is the */
   /*  text of the message. It may contain printf formatting. It may contain */
   /*  newlines to force line breaks -- even if it doesn't contain new lines */
   /*  the routine will wrap the text if a line is too long */
    void (*post_error)(const char *title,const char *error,...);

   /* The following is used to post a warning message in such a way that it */
   /*  will not impede the user. Currently it creates a little window at the */
   /*  bottom right of the screen and writes successive messages there */
    void (*logwarning)(const char *fmt,...);

   /* The following is another way to post a warning message in such a way */
   /*  that it will not impede the user. Currently it pops up a little */
   /*  non-modal dlg which vanishes after a minute or two (or if the user */
   /*  dismisses it, of course */
    void (*post_warning)(const char *title,const char *statement,...);

   /* Occasionally we we be deep in a non-ui routine and we find we must ask */
   /*  the user a question. In this routine the choices are displayed as */
   /*  buttons, one button is the default, another is a cancel choice */
    int (*ask)(const char *title, const char **answers,
	    int def, int cancel,const char *question,...);

   /* Similar to the above, except here the choices are presented as a */
   /*  scrolled list. Return -1 if the user cancels */
    int (*choose)(const char *title, const char **answers,
	    int def, int cancel,const char *question,...);

    /* Multiple things can be selected, sel is an in/out parameter, one byte */
    /*  per entry in the choice array. 0=> not selected, 1=>selected */
    int (*choose_multiple)(char *title, const char **choices,char *sel,
	    int cnt, char *buts[2], const char *question,...);

   /* Here we want a string. We are passed a default answer (or NULL) */
   /* The return is NULL on cancel, otherwise a string which must be freed */
    char *(*ask_string)(const char *title,
	    const char *def,const char *question,...);
   /* Same as above, except for entering a password */
    char *(*ask_password)(const char *title,
	    const char *def,const char *question,...);

   /* The next two routines are only used in the python interface to provide */
   /*  a python script running in ff a way to open a file */
   /* Arguments are a window title for the dlg, a default file (or NULL), and */
   /*  an initial filter (unix wildcards) or NULL */
    char *(*open_file)(const char *title, const char *defaultfile,
	const char *initial_filter);
    char *(*saveas_file)(const char *title, const char *defaultfile,
	const char *initial_filter);

    /* These routines are for a progress indicator */
    void (*progress_start)(int delay, const char *title, const char *line1,
	const char *line2, int tot, int stages);
    void (*progress_end)(void);
    void (*progress_show)(void);
    void (*progress_enable_stop)(int);
    int (*progress_next)(void);
    int (*progress_next_stage)(void);
    int (*progress_increment)(int);
    void (*progress_change_line1)(const char *);
    void (*progress_change_line2)(const char *);
    void (*progress_pause)(void);
    void (*progress_resume)(void);
    void (*progress_change_stages)(int);
    void (*progress_change_total)(int);
    int  (*progress_reset)(void);

    void (*allow_events)(void);

   /* These next few provide friendly names of various opentype tags */
   /*  The ui version will probably be translated, while the non-ui list */
   /*  will probably not. The distinction isn't necessary, but is present in ff*/
    const char *(*strid)(int);
    const char *(*mslang)(int);

   /* pops up a dlg asking user whether to do remove overlap (and other stuff)*/
   /*  when loading an eps file with strokes, etc. */
    int (*stroke_flags)(void);
};
extern struct ui_interface *ui_interface;

#define IError			(ui_interface->ierror)
#define LogError		(ui_interface->logwarning)
#define ff_post_notice		(ui_interface->post_warning)
#define ff_post_error		(ui_interface->post_error)
#define ff_ask			(ui_interface->ask)
#define ff_choose		(ui_interface->choose)
#define ff_choose_multiple	(ui_interface->choose_multiple)
#define ff_ask_string		(ui_interface->ask_string)
#define ff_ask_password		(ui_interface->ask_password)

#define ff_open_filename	(ui_interface->open_file)
#define ff_save_filename	(ui_interface->saveas_file)

#define ff_progress_start_indicator	(ui_interface->progress_start)
#define ff_progress_end_indicator	(ui_interface->progress_end)
#define ff_progress_show		(ui_interface->progress_show)
#define ff_progress_enable_stop		(ui_interface->progress_enable_stop)
#define ff_progress_next		(ui_interface->progress_next)
#define ff_progress_next_stage		(ui_interface->progress_next_stage)
#define ff_progress_increment		(ui_interface->progress_increment)
#define ff_progress_change_line1	(ui_interface->progress_change_line1)
#define ff_progress_change_line2	(ui_interface->progress_change_line2)
#define ff_progress_pause_timer		(ui_interface->progress_pause)
#define ff_progress_resume_timer	(ui_interface->progress_resume)
#define ff_progress_change_stages	(ui_interface->progress_change_stages)
#define ff_progress_change_total	(ui_interface->progress_change_total)
#define ff_progress_reset		(ui_interface->progress_reset)

#define ff_progress_allow_events	(ui_interface->allow_events)

#define TTFNameIds			(ui_interface->strid)
#define MSLangString			(ui_interface->mslang)

#define PsStrokeFlagsDlg		(ui_interface->stroke_flags)

void FF_SetUiInterface(struct ui_interface *uii);

/* ************************************************************************** */
/*                                Preferences                                 */
/* ************************************************************************** */
struct val;

struct prefs_interface {
    void  (*save_prefs)(int not_if_running_script);
    void  (*load_prefs)(void);
    int   (*get_prefs)(char *name,struct val *value);
    int   (*set_prefs)(char *name,struct val *val1, struct val *val2);
    char *(*get_exe_share_dir)(void);
    void  (*init_prefs)(void);
};
extern struct prefs_interface *prefs_interface;

#define SavePrefs		(prefs_interface->save_prefs)
#define LoadPrefs		(prefs_interface->load_prefs)
#define GetPrefs		(prefs_interface->get_prefs)
#define SetPrefs		(prefs_interface->set_prefs)
#define getFontForgeShareDir	(prefs_interface->get_exe_share_dir)
#define SetDefaults		(prefs_interface->init_prefs)

void FF_SetPrefsInterface(struct prefs_interface *prefsi);

/* ************************************************************************** */
/*                          Updating glyph windows                            */
/* ************************************************************************** */

struct splinechar;
struct layer;

struct sc_interface {
   /* Update all windows looking at this glyph */
    void  (*update_all)(struct splinechar *);

   /* Background images or kerning info have changed for this glyph and */
   /*  all windows displaying them need to be refreshed */
    void  (*out_of_date_background)(struct splinechar *);

   /* The name or code point or encoding of this glyph has changed */
   /*  update all window titles of any windows looking at us */
    void (*refresh_titles)(struct splinechar *);

   /* The hints of the glyph have changed */
    void (*hints_changed)(struct splinechar *);

   /* Mark the glyph as changed, and force an update */
    void (*glyph_changed_update)(struct splinechar *,int layer);

   /* As above, except this time the change might take the glyph back to */
   /*  an "unchanged" state (ie. an Undo) */
    void (*glyph__changed_update)(struct splinechar *,int layer,int);

   /* The glyph's instructions have changed, so any dlgs looking at */
   /*  our instructions need to be updated */
    void (*instructions_changed)(struct splinechar *sc);

   /* We are removing this glyph (or something like it), get rid of any */
   /* glyph outline windows which display it */
    void (*close_all_windows)(struct splinechar *);

   /* Called when a multilayered glyph increases its layer count */
   /*  the charview needs to add more layers to its layer window, etc. */
    void (*more_layers)(struct splinechar *, struct layer *);
};
extern struct sc_interface *sc_interface;

#define SCUpdateAll			(sc_interface->update_all)
#define SCOutOfDateBackground		(sc_interface->out_of_date_background)
#define SCRefreshTitles			(sc_interface->refresh_titles)
#define SCHintsChanged			(sc_interface->hints_changed)
#define _SCCharChangedUpdate		(sc_interface->glyph__changed_update)
#define SCCharChangedUpdate		(sc_interface->glyph_changed_update)
#define SCMarkInstrDlgAsChanged		(sc_interface->instructions_changed)
#define SCCloseAllViews			(sc_interface->close_all_windows)
#define SCMoreLayers			(sc_interface->more_layers)

void FF_SetSCInterface(struct sc_interface *sci);

/* ************************************************************************** */
/*                         Updating glyph windows 2                           */
/* ************************************************************************** */

struct charviewbase;
struct splinefont;

struct cv_interface {
   /* Update all windows looking at what this char window looks at */
   /*  which might be a glyph, or perhaps the grid layer */
   /* And mark as changed */
    void (*glyph_changed_update)(struct charviewbase *);
    void (*_glyph_changed_update)(struct charviewbase *, int);

   /* A glyph's name has changed find all charviews with tabs with that name */
   /*  and update those tabs */
    void (*glyph_name_change)(struct splinefont *sf, char *oldname, char *newname);

   /* We've added a layer to a font */
    void (*layer_palette_check)(struct splinefont *sf);
};
extern struct cv_interface *cv_interface;

#define CVCharChangedUpdate		(cv_interface->glyph_changed_update)
#define _CVCharChangedUpdate		(cv_interface->_glyph_changed_update)
#define CVGlyphRenameFixup		(cv_interface->glyph_name_change)
#define CVLayerPaletteCheck		(cv_interface->layer_palette_check)

void FF_SetCVInterface(struct cv_interface *cvi);

/* ************************************************************************** */
/*                         Updating bitmap windows                            */
/* ************************************************************************** */

struct bdfchar;

struct bc_interface {
   /* Update all windows looking at this bitmap glyph */
   /* And mark as changed */
    void (*glyph_changed_update)(struct bdfchar *);

   /* Force a refresh on all open bitmap windows of this glyph */
    void (*refresh_all)(struct bdfchar *);

   /* Destroy all open bitmap windows of this glyph */
    void (*destroy_all)(struct bdfchar *);
};
extern struct bc_interface *bc_interface;

#define BCCharChangedUpdate		(bc_interface->glyph_changed_update)
#define BCRefreshAll			(bc_interface->refresh_all)
#define BCDestroyAll			(bc_interface->destroy_all)

void FF_SetBCInterface(struct bc_interface *bci);

/* ************************************************************************** */
/*                          Access to metrics views                           */
/* ************************************************************************** */

struct metricsview;
struct splinefont;

struct mv_interface {
   /* Number of glyphs displayed in the view */
    int (*glyph_cnt)(struct metricsview *);

   /* Access to the i'th member */
    struct splinechar *(*get_glyph)(struct metricsview *,int);

   /* Kerning information for this font has changed. Remetric the metric views*/
   /* Now-a-days, call this when any kind of lookup information changes, not */
   /*  just kerning */
    void (*rekern)(struct splinefont *);

   /* Close any metrics views associated with this font */
    void (*sf_close_metrics)(struct splinefont *sf);
};
extern struct mv_interface *mv_interface;

#define MVGlyphCount			(mv_interface->glyph_cnt)
#define MVGlyphIndex			(mv_interface->get_glyph)
#define MVReKernAll			(mv_interface->rekern)
#define MVDestroyAll			(mv_interface->sf_close_metrics)

void FF_SetMVInterface(struct mv_interface *mvi);

/* ************************************************************************** */
/*                             Access to font info                            */
/* ************************************************************************** */
struct otlookup;

struct fi_interface {
   /* Insert a new lookup into the fontinfo lookup list */
    void (*insert_lookup)(struct splinefont *, struct otlookup *);

   /* Merge lookup in from another font */
    void (*copy_into)(struct splinefont *, struct splinefont *,
	    struct otlookup *, struct otlookup *, int, struct otlookup *);

   /* Removes any font info window for this font */
    void (*destroy)(struct splinefont *);
};
extern struct fi_interface *fi_interface;

#define FISortInsertLookup			(fi_interface->insert_lookup)
#define FIOTLookupCopyInto			(fi_interface->copy_into)
#define FontInfo_Destroy			(fi_interface->destroy)

void FF_SetFIInterface(struct fi_interface *fii);

/* ************************************************************************** */
/*                           Updating font windows                            */
/* ************************************************************************** */

struct fontviewbase;
struct bdffont;

struct fv_interface {
   /* Create a new font view. Whatever that may entail */
    struct fontviewbase *(*create)(struct splinefont *,int hide);

   /* Create a new font view but without attaching it to a window */
    struct fontviewbase *(*_create)(struct splinefont *);

   /* Free a font view (we assume all windows have already been destroyed) */
    void (*close)(struct fontviewbase *);

   /* Free a font view (we assume all windows have already been destroyed) */
    void (*free)(struct fontviewbase *);

   /* Set the window title of this fontview */
    void (*set_title)(struct fontviewbase *);

   /* Set the window title of all fontviews associated with this font */
    void (*set_titles)(struct splinefont *);

   /* Refresh all displays of all fontviews associated with this font */
    void (*refresh_all)(struct splinefont *);

   /* Reformat this particular fontview (after encoding change, etc) */
    void (*reformat_one)(struct fontviewbase *);

   /* Reformat all fontviews associated with this font */
    void (*reformat_all)(struct splinefont *);

   /* The active layer has changed. Possibly because the old one was deleted */
    void (*layer_changed)(struct fontviewbase *);

   /* toggle the change indicator of this glyph in the font view */
    void (*flag_glyph_changed)(struct splinechar *);

   /* Retrieve the window's size in rows and columns */
    int  (*win_info)(struct fontviewbase *, int *cols, int *rows);

   /* Is this font currently open? (It was open once, this check is to make   */
   /*  sure the user hasn't closed it since they copied from it -- so we can  */
   /*  follow references appropriately if the font we are pasting into doesn't*/
   /*  have the needed glyph */
    int  (*font_is_active)(struct splinefont *);

   /* Sometimes we just need a fontview, any fontview as a last resort fallback*/
    struct fontviewbase *(*first_font)(void);

   /* Append this fontview to the list of them */
    struct fontviewbase *(*append)(struct fontviewbase *);

   /* Look through all loaded fontviews and see if any contains a font */
   /*  which lives in the given filename */
    struct splinefont *(*font_of_filename)(const char *);

   /* We've just added some extra encoding slots, which means we may need */
   /*  to increase the number of rows in the fontview display and perhaps */
   /*  adjust its scrollbar */
    void (*extra_enc_slots)(struct fontviewbase *,int new_enc_max);

   /* My fontviews contain a glyph cache (a BDFPieceMeal font) whenever */
   /*  more glyphs are added to the font, more bitmap glyph slots need to */
   /*  be added to the font cache */
    void (*bigger_glyph_cache)(struct fontviewbase *,int new_glyph_cnt);

   /* If we want to change the font displayed in a fontview */
    void (*change_display_bitmap)(struct fontviewbase *, struct bdffont *);

   /* We just deleted the active bitmap, so switch to a rasteriztion of the outlines */
    void (*display_filled)(struct fontviewbase *);

   /* When we revert a font we need to change the alegence of all outline */
   /*  glyph windows to the new value of the font */
    void (*reattach_cvs)(struct splinefont *old, struct splinefont *new);

   /* deselect any selected glyphs */
    void (*deselect_all)(struct fontviewbase *);

   /* Scroll (or whatever) the fontview so that the desired */
   /*  gid is displayed */
    void (*display_gid)(struct fontviewbase *,int gid);

   /* Scroll (or whatever) the fontview so that the desired */
   /*  encoding is displayed */
    void (*display_enc)(struct fontviewbase *,int enc);

   /* Scroll (or whatever) the fontview so that the desired */
   /*  glyph is displayed */
    void (*select_gid)(struct fontviewbase *,int gid);

   /* Close any open glyph instruction windows in the font */
    int (*close_all_instrs)(struct splinefont *);
};
extern struct fv_interface *fv_interface;

#define FontViewCreate		(fv_interface->create)
#define _FontViewCreate		(fv_interface->_create)
#define FontViewClose		(fv_interface->close)
#define FontViewFree		(fv_interface->free)
#define FVSetTitle		(fv_interface->set_title)
#define FVSetTitles		(fv_interface->set_titles)
#define FVRefreshAll		(fv_interface->refresh_all)
#define FontViewReformatOne	(fv_interface->reformat_one)
#define FontViewReformatAll	(fv_interface->reformat_all)
#define FontViewLayerChanged	(fv_interface->layer_changed)
#define FVToggleCharChanged	(fv_interface->flag_glyph_changed)
#define FVWinInfo		(fv_interface->win_info)
#define SFIsActive		(fv_interface->font_is_active)
#define FontViewFirst		(fv_interface->first_font)
#define FVAppend		(fv_interface->append)
#define FontWithThisFilename	(fv_interface->font_of_filename)
#define FVAdjustScrollBarRows	(fv_interface->extra_enc_slots)
#define FVBiggerGlyphCache	(fv_interface->bigger_glyph_cache)
#define FVChangeDisplayBitmap	(fv_interface->change_display_bitmap)
#define FVShowFilled		(fv_interface->display_filled)
#define FVReattachCVs		(fv_interface->reattach_cvs)
#define FVDisplayGID		(fv_interface->display_gid)
#define FVDisplayEnc		(fv_interface->display_enc)
#define FVChangeGID		(fv_interface->select_gid)
#define SFCloseAllInstrs	(fv_interface->close_all_instrs)

void FF_SetFVInterface(struct fv_interface *fvi);

/* ************************************************************************** */
/*                       Clibboard access (copy/paste)                        */
/* ************************************************************************** */

struct clip_interface {
   /* Announce we own the clipboard selection */
    void  (*grab_clip)(void);
   /* Either place data in the clipboard of a given type, or */
   /*  provide a routine to call which will give data on demand */
   /*  (and another routine to clean things up) */
    void  (*add_data_type)(const char *type, void *data, int cnt, int size,
	void *(*gendata)(void *,int32 *len), void (*freedata)(void *));
   /* Does the clipboard contain something of the given type? */
    int   (*clip_has_type)(const char *mimetype);
   /* Ask for the clipboard, and waits (and returns) for the response */
    void *(*request_clip)(const char *mimetype,int *len);
   
};
extern struct clip_interface *clip_interface;

#define ClipboardGrab		(clip_interface->grab_clip)
#define ClipboardAddDataType	(clip_interface->add_data_type)
#define ClipboardRequest	(clip_interface->request_clip)
#define ClipboardHasType	(clip_interface->clip_has_type)

void FF_SetClipInterface(struct clip_interface *clipi);

extern const char *NOUI_TTFNameIds(int id);
extern const char *NOUI_MSLangString(int language);
#endif
