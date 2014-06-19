#ifndef _GLIF_NAME_HASH_H
#define _GLIF_NAME_HASH_H
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef FF_UTHASH_GLIF_NAMES
#include "uthash.h"

struct glif_name {
  long int gid;
  char * glif_name;
  UT_hash_handle gid_hash;
  UT_hash_handle glif_name_hash;
};
struct glif_name_index {
  struct glif_name * gid_hash;
  struct glif_name * glif_name_hash;
};
struct glif_name * glif_name_new();
struct glif_name_index * glif_name_index_new();
void glif_name_hash_add(struct glif_name_index * hash, struct glif_name * item);
struct glif_name * glif_name_search_gid(struct glif_name_index * hash, long int gid);
struct glif_name * glif_name_search_glif_name(struct glif_name_index * hash, const char * glif_name);
int glif_name_track_collide(struct glif_name_index * hash, long int gid, const char * glif_name);
void glif_name_track_new(struct glif_name_index * hash, long int gid, const char * glif_name);
void glif_name_hash_remove(struct glif_name_index * hash, struct glif_name * item);
void glif_name_hash_destroy(struct glif_name_index * hash);
#endif
#endif
