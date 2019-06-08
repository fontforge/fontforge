#include <fontforge-config.h>

#include "glif_name_hash.h"

#include "ffglib.h"

#include <stdlib.h>

static void glif_name_destroy(gpointer data) {
  struct glif_name * entry = (struct glif_name *)data;
  g_free(entry->glif_name);
  free(entry);
}

struct glif_name_index * glif_name_index_new() {
  return (struct glif_name_index *)g_hash_table_new_full(g_str_hash, g_str_equal, NULL, glif_name_destroy);
}

struct glif_name * glif_name_search_glif_name(struct glif_name_index * hash, const char * glif_name) {
  g_return_val_if_fail(hash != NULL && glif_name != NULL, NULL);

  return (struct glif_name *) g_hash_table_lookup((GHashTable *)hash, glif_name);
}

void glif_name_track_new(struct glif_name_index * hash, long int gid, const char * glif_name) {
  g_return_if_fail(hash != NULL && glif_name != NULL);

  struct glif_name * new_node = calloc(1, sizeof(struct glif_name));
  new_node->gid = gid;
  new_node->glif_name = g_strdup(glif_name);

  g_hash_table_replace((GHashTable *)hash, new_node->glif_name, new_node);
}

void glif_name_index_destroy(struct glif_name_index * hash) {
  g_hash_table_destroy((GHashTable *)hash);
}
