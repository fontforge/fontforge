#ifndef _GWWVASK_H
#define _GWWVASK_H

extern void gwwv_post_notice(const char *title, const char *msg, ... );
extern void gwwv_error(const char *title, const char *msg, ... );
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
#endif
