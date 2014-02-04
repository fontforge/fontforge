/* strlist.c */
#include <string.h>
#include <stdlib.h>
#include "basics.h"
#include "ustring.h"
#include "strlist.h"

static int compare_string_list_items(const void *p1, const void *p2);

int string_list_count(const struct string_list *list) {
    int cnt;
    for( cnt=0; list!=NULL; list=list->next, cnt++);
    return cnt;
}

void delete_string_list(struct string_list *list) {
    struct string_list *curr, *next;
    for ( curr=list; curr!=NULL; curr=next ) {
	next = curr->next;
        free(curr->str);
	free(curr);
    }
    return;
}

struct string_list *new_string_list(const char *str) {
    char *str2;
    struct string_list *list;
    str2 = copy(str);
    if ( str2==NULL )
	return NULL;
    list = malloc(sizeof(struct string_list));
    if ( list!=NULL ) {
	list->next = NULL;
	list->str = str2;
    }
    return list;
}

struct string_list *prepend_string_list(struct string_list* list, const char *str) {
    struct string_list *newlist;
    newlist = new_string_list(str);
    if ( newlist==NULL )
	return list;
    newlist->next = list;
    return newlist;
}

struct string_list *append_string_list(struct string_list* list, const char *str) {
    struct string_list *newlist;
    struct string_list *last;
    newlist = new_string_list(str);
    if ( newlist==NULL )
	return list;
    if ( list == NULL )
	return newlist;
    for ( last=list; last->next!=NULL; last=last->next );
    last->next = newlist;
    return list;
}

static int compare_string_list_items(const void *p1, const void *p2) {
    const struct string_list *s1 = *((const struct string_list**)p1);
    const struct string_list *s2 = *((const struct string_list**)p2);
    return strcmp(s1->str, s2->str);
}

struct string_list *sort_string_list(struct string_list* list) {
    int cnt, i;
    struct string_list* item;
    struct string_list** sortarray;
    if ( list==NULL ) /* cnt == 0 */
	return NULL;
    if ( list->next==NULL ) /* cnt == 1 */
	return list;

    cnt = string_list_count(list);

    sortarray = calloc(cnt,sizeof(struct string_list*));
    for ( i=0, item=list; item!=NULL; item=item->next, i++ ) {
	sortarray[i] = item;
    }

    qsort( sortarray, cnt, sizeof(struct string_list*), compare_string_list_items);
    for ( i=0; i<cnt-1; i++ ) {
	sortarray[i]->next = sortarray[i+1];
    }
    sortarray[cnt-1]->next = NULL;
    item = sortarray[0];
    free(sortarray);
    return item;
}
