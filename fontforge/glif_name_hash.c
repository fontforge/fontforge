#include "glif_name_hash.h"

#include <stdlib.h>
#include <string.h>
#include <ustring.h>
#include <uthash.h>

struct glif_name_private {
  long int gid;
  char * glif_name;
  UT_hash_handle gid_hash;
  UT_hash_handle glif_name_hash;
};

struct glif_name_index {
  struct glif_name_private * gid_hash;
  struct glif_name_private * glif_name_hash;
};

static void glif_name_hash_add(struct glif_name_index * hash, struct glif_name_private * item) {
  HASH_ADD(gid_hash, hash->gid_hash, gid, sizeof(item->gid), item);
  HASH_ADD_KEYPTR(glif_name_hash, hash->glif_name_hash, item->glif_name, strlen(item->glif_name), item);
}

static void glif_name_hash_remove(struct glif_name_index * hash, struct glif_name_private * item) {
  HASH_DELETE(gid_hash, hash->gid_hash, item);
  HASH_DELETE(glif_name_hash, hash->glif_name_hash, item);
}

struct glif_name_index * glif_name_index_new() {
  return calloc(1, sizeof(struct glif_name_index));
}

struct glif_name * glif_name_search_glif_name(struct glif_name_index * hash, const char * glif_name) {
  struct glif_name_private * output = NULL;
  HASH_FIND(glif_name_hash, hash->glif_name_hash, glif_name, strlen(glif_name), output);
  return (struct glif_name *)output;
}

void glif_name_track_new(struct glif_name_index * hash, long int gid, const char * glif_name) {
  struct glif_name_private * new_node = calloc(1, sizeof(struct glif_name_private));
  new_node->gid = gid;
  new_node->glif_name = copy(glif_name);
  glif_name_hash_add(hash, new_node);
}

void glif_name_index_destroy(struct glif_name_index * hash) {
  struct glif_name_private *current, *tmp;
  HASH_ITER(gid_hash, hash->gid_hash, current, tmp) {
    glif_name_hash_remove(hash, current);
    if (current->glif_name != NULL) { free(current->glif_name); current->glif_name = NULL; }
    free(current); current = NULL;
  }
  free(hash);
}
