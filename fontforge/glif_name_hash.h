#ifndef FONTFORGE_GLIF_NAME_HASH_H
#define FONTFORGE_GLIF_NAME_HASH_H

#include <fontforge-config.h>

struct glif_name_index;

struct glif_name {
  long int gid;
  char * glif_name;
};

struct glif_name_index * glif_name_index_new();
void glif_name_index_destroy(struct glif_name_index * hash);

void glif_name_track_new(struct glif_name_index * hash, long int gid, const char * glif_name);
struct glif_name * glif_name_search_glif_name(struct glif_name_index * hash, const char * glif_name);

#endif /* FONTFORGE_GLIF_NAME_HASH_H */
