/* Copyright (C) 2007 by George Williams */
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
/* This encapsulates a set of callbacks and stubs. The callbacks get activated*/
/*  when an event happens (a glyph in a font changes for example, then all */
/*  charviews looking at it must be updated), and the stubs provide some simple*/
/*  UI routines: Post an error, etc. */

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
    void (*progress_change_line1)(const char *);
    void (*progress_change_line2)(const char *);
    void (*progress_pause)(void);
    void (*progress_resume)(void);
    void (*progress_change_stages)(int);
    void (*progress_change_total)(int);

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

#define ff_open_filename	(ui_interface->open_file)
#define ff_save_filename	(ui_interface->saveas_file)

#define ff_progress_start_indicator	(ui_interface->progress_start)
#define ff_progress_end_indicator	(ui_interface->progress_end)
#define ff_progress_show		(ui_interface->progress_show)
#define ff_progress_enable_stop		(ui_interface->progress_enable_stop)
#define ff_progress_next		(ui_interface->progress_next)
#define ff_progress_next_stage		(ui_interface->progress_next_stage)
#define ff_progress_change_line1	(ui_interface->progress_change_line1)
#define ff_progress_change_line2	(ui_interface->progress_change_line2)
#define ff_progress_pause_timer		(ui_interface->progress_pause)
#define ff_progress_resume_timer	(ui_interface->progress_resume)
#define ff_progress_change_stages	(ui_interface->progress_change_stages)
#define ff_progress_change_total	(ui_interface->progress_change_total)

void FF_SetUiInterface(struct ui_interface *uii);
#endif
