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

#ifndef FONTFORGE_GPROGRESS_H
#define FONTFORGE_GPROGRESS_H

#include "basics.h"
#include "intl.h"

extern void GProgressStartIndicator(
    int delay,			/* in tenths of seconds */
    const unichar_t *win_title,	/* for the window decoration */
    const unichar_t *line1,	/* First line of description */
    const unichar_t *line2,	/* Second line */
    int tot,			/* Number of sub-entities in the operation */
    int stages			/* Number of stages, each processing tot sub-entities */
);
extern void GProgressStartIndicatorR(int delay, int win_titler, int line1r,
	int line2r, int tot, int stages);
extern void GProgressEndIndicator(void);	/* Ends the topmost indicator */
extern void GProgressChangeLine1(const unichar_t *line1); /* Changes the text in the topmost */
extern void GProgressChangeLine2(const unichar_t *line2); /* Changes the text in the topmost */
extern void GProgressChangeLine1R(int line1r);		/* Changes the text in the topmost */
extern void GProgressChangeLine2R(int line2r);		/* Changes the text in the topmost */
extern void GProgressChangeTotal(int tot);		/* Changes the expected length in the topmost */
extern void GProgressChangeStages(int stages);		/* Changes the expected number of stages in the topmost */
extern void GProgressEnableStop(int enabled);		/* Allows you to disable and enable the stop button if it can't be used in a section of code */
    /* if any of the next routines returns false, then abort processing */
extern int GProgressNextStage(void);	/* Move to the next stage in the topmost indicator */
extern int GProgressNext(void);		/* Increment progress by one sub-entity */
extern int GProgressIncrementBy(int cnt);	/* Increment progress by cnt sub-entities */
extern int GProgressReset(void);	/* Set progress to 0 */
extern void GProgressPauseTimer(void);	/* Don't bring up the progress dlg because of */
extern void GProgressResumeTimer(void);	/*  time spent between a pause and resume */
extern void GProgressShow(void);	/* Display the damn thing whether we should or not */

extern void GProgressStartIndicator8(int delay, const char *title, const char *line1,
	const char *line2, int tot, int stages);
extern void GProgressChangeLine1_8(const char *line1); /* Changes the text in the topmost */
extern void GProgressChangeLine2_8(const char *line2); /* Changes the text in the topmost */

#define gwwv_progress_start_indicator	GProgressStartIndicator8
#define gwwv_progress_next		GProgressNext
#define gwwv_progress_next_stage	GProgressNextStage
#define gwwv_progress_end_indicator	GProgressEndIndicator
#define gwwv_progress_show		GProgressShow
#define gwwv_progress_change_line1	GProgressChangeLine1_8
#define gwwv_progress_change_line2	GProgressChangeLine2_8
#define gwwv_progress_change_total	GProgressChangeTotal
#define gwwv_progress_change_stages	GProgressChangeStages
#define gwwv_progress_increment		GProgressIncrementBy
#define gwwv_progress_reset		GProgressReset
#define gwwv_progress_pause_timer	GProgressPauseTimer
#define gwwv_progress_resume_timer	GProgressResumeTimer
#define gwwv_progress_enable_stop	GProgressEnableStop

#endif /* FONTFORGE_GPROGRESS_H */
