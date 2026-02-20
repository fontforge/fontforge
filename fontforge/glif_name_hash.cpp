#include <fontforge-config.h>

#include "glif_name_hash.h"

#include <stdlib.h>
#include <string.h>
#include <string>
#include <unordered_map>

// Internal C++ implementation using std::unordered_map
struct glif_name_index {
    std::unordered_map<std::string, struct glif_name*> map;

    ~glif_name_index() {
        for (auto& pair : map) {
            free(pair.second->glif_name);
            free(pair.second);
        }
    }
};

extern "C" {

struct glif_name_index* glif_name_index_new() { return new glif_name_index(); }

struct glif_name* glif_name_search_glif_name(struct glif_name_index* hash,
                                             const char* glif_name) {
    if (hash == NULL || glif_name == NULL) {
        return NULL;
    }

    auto it = hash->map.find(glif_name);
    if (it != hash->map.end()) {
        return it->second;
    }
    return NULL;
}

void glif_name_track_new(struct glif_name_index* hash, long int gid,
                         const char* glif_name) {
    if (hash == NULL || glif_name == NULL) {
        return;
    }

    // Check if entry already exists and remove it
    auto it = hash->map.find(glif_name);
    if (it != hash->map.end()) {
        free(it->second->glif_name);
        free(it->second);
        hash->map.erase(it);
    }

    struct glif_name* new_node =
        (struct glif_name*)calloc(1, sizeof(struct glif_name));
    new_node->gid = gid;
    new_node->glif_name = strdup(glif_name);

    hash->map[new_node->glif_name] = new_node;
}

void glif_name_index_destroy(struct glif_name_index* hash) { delete hash; }

}  // extern "C"
