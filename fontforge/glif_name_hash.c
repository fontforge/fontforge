#include "glif_name_hash.h"
#ifdef FF_UTHASH_GLIF_NAMES
struct glif_name * glif_name_new() {
  struct glif_name * output = malloc(sizeof(struct glif_name));
  if (output == NULL) return NULL;
  memset(output, 0, sizeof(struct glif_name));
  return output;
}
struct glif_name_index * glif_name_index_new() {
  struct glif_name_index * output = malloc(sizeof(struct glif_name_index));
  if (output == NULL) return NULL;
  memset(output, 0, sizeof(struct glif_name_index));
  return output;
}
void glif_name_hash_add(struct glif_name_index * hash, struct glif_name * item) {
  HASH_ADD(gid_hash, hash->gid_hash, gid, sizeof(item->gid), item);
  HASH_ADD_KEYPTR(glif_name_hash, hash->glif_name_hash, item->glif_name, strlen(item->glif_name), item);
}
struct glif_name * glif_name_search_gid(struct glif_name_index * hash, long int gid) {
  struct glif_name * output = NULL;
  HASH_FIND(gid_hash, hash->gid_hash, &gid, sizeof(gid), output);
  return output;
}
struct glif_name * glif_name_search_glif_name(struct glif_name_index * hash, const char * glif_name) {
  struct glif_name * output = NULL;
  HASH_FIND(glif_name_hash, hash->glif_name_hash, glif_name, strlen(glif_name), output);
  return output;
}
int glif_name_track_collide(struct glif_name_index * hash, long int gid, const char * glif_name) {
  if ((glif_name_search_gid(hash, gid) != NULL) ||
      (glif_name_search_glif_name(hash, glif_name) != NULL))
    return 1;
  return 0;
}
void glif_name_track_new(struct glif_name_index * hash, long int gid, const char * glif_name) {
  struct glif_name * new_node = glif_name_new();
  new_node->gid = gid;
  new_node->glif_name = malloc(strlen(glif_name)+1);
  strcpy(new_node->glif_name, glif_name);
  glif_name_hash_add(hash, new_node);
}
void glif_name_hash_remove(struct glif_name_index * hash, struct glif_name * item) {
  HASH_DELETE(gid_hash, hash->gid_hash, item);
  HASH_DELETE(glif_name_hash, hash->glif_name_hash, item);
}
void glif_name_hash_destroy(struct glif_name_index * hash) {
  struct glif_name *current, *tmp;
  HASH_ITER(gid_hash, hash->gid_hash, current, tmp) {
    glif_name_hash_remove(hash, current);
    if (current->glif_name != NULL) { free(current->glif_name); current->glif_name = NULL; }
    free(current); current = NULL;
  }
}
#endif

