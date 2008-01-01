#ifndef _GWWVASK_H
#define _GWWVASK_H

extern void gwwv_post_notice(const char *title, const char *msg, ... );
extern void gwwv_post_error(const char *title, const char *msg, ... );
extern void gwwv_ierror(const char *msg, ... );
extern int gwwv_ask(const char *title,const char **butnames, int def,int cancel,
	const char *msg, ... );
extern char *gwwv_ask_string(const char *title,const char *def,
	const char *question,...);
extern int gwwv_choose_with_buttons(const char *title,
	const char **choices, int cnt, int def,
	const char *butnames[2], const char *msg, ... );
extern int gwwv_choose(const char *title,
	const char **choices, int cnt, int def,
	const char *msg, ... );
extern int gwwv_choose_multiple(char *title,
	const char **choices, char *sel, int cnt, char **buts,
	const char *msg, ... );
extern int gwwv_wild_match(char *pattern, const char *name,int ignorecase);

struct gwwv_filter {
    char *name;
    char *wild;
    GtkFileFilterFunc filtfunc;
};

extern char *gwwv_open_filename_mult(const char *title,const char *def_name,const struct gwwv_filter *filter, int mult);
extern char *gwwv_save_filename_with_gadget(const char *title, const char *def_name,
	const struct gwwv_filter *filters, GtkWidget *extra );
extern char *gwwv_open_filename(const char *title, const char *def_name,
	const char *filter);
extern char *gwwv_saveas_filename(const char *title, const char *def_name,
	const char *filter);
#endif
