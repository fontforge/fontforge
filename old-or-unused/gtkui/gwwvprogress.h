#ifndef _GWWVPROGRESS_H
#define _GWWVPROGRESS_H
extern void gwwv_progress_start_indicator(int delay, const char *title, const char *line1,
	const char *line2, int tot, int stages);
extern void gwwv_progress_end_indicator(void);
extern void gwwv_progress_show(void);
extern int gwwv_progress_next(void);
extern int gwwv_progress_increment(int cnt);
extern int gwwv_progress_next_stage(void);
extern void gwwv_progress_change_line1(const char *line1);
extern void gwwv_progress_change_line2(const char *line2);
extern void gwwv_progress_enable_stop(int enabled);
extern void gwwv_progress_change_total(int tot);
extern void gwwv_progress_change_stages(int stages);
extern void gwwv_progress_pause_timer(void);
extern void gwwv_progress_resume_timer(void);
extern int gwwv_progress_reset(void);
#endif

