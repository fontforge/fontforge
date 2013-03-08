/* strlist.h
 *
 * A general-purpose singly-linked list of C strings.
 */
#ifndef _STRLIST_H_
#define _STRLIST_H_

struct string_list {
    struct string_list *next;
    char *str;
};
extern int string_list_count(const struct string_list *);
extern void delete_string_list(struct string_list *);
extern struct string_list *new_string_list(const char *);
extern struct string_list *prepend_string_list(struct string_list*, const char *);
extern struct string_list *append_string_list(struct string_list*, const char *);
extern struct string_list *sort_string_list(struct string_list*);

#endif /* _STRLIST_H */
