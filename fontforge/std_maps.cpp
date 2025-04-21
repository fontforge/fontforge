/* Copyright 2025 Maxim Iorsh <iorsh@users.sourceforge.net>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "std_maps.hpp"

cpp_SubtableMap* SubtableMap_new() {
    SubtableMap* st_map = new SubtableMap();
    return reinterpret_cast<cpp_SubtableMap*>(st_map);
}

void SubtableMap_delete(cpp_SubtableMap** p_map) {
    if (p_map == nullptr) {
        return;
    }

    SubtableMap* st_map = reinterpret_cast<SubtableMap*>(*p_map);
    delete st_map;
    *p_map = nullptr;
}

void SubtableMap_add_kp(cpp_SubtableMap* map, struct lookup_subtable* subtable,
                        int gid, KernPair* kp) {
    SubtableMap& st_map = *reinterpret_cast<SubtableMap*>(map);
    st_map[subtable].first.push_back({gid, kp});
}

void SubtableMap_add_pst(cpp_SubtableMap* map, struct lookup_subtable* subtable,
                         int gid, PST* pst) {
    SubtableMap& st_map = *reinterpret_cast<SubtableMap*>(map);
    st_map[subtable].second.push_back({gid, pst});
}

int SubtableMap_get_size(cpp_SubtableMap* map,
                                        struct lookup_subtable* subtable) {
    SubtableMap& st_map = *reinterpret_cast<SubtableMap*>(map);
    auto map_it = st_map.find(subtable);
    if (map_it == st_map.end()) {
        return 0;
    } else {
	return map_it->second.first.size() + map_it->second.second.size();
    }
}

struct kp_list* SubtableMap_get_kp_list(cpp_SubtableMap* map,
                                        struct lookup_subtable* subtable) {
    SubtableMap& st_map = *reinterpret_cast<SubtableMap*>(map);
    auto map_it = st_map.find(subtable);
    if (map_it == st_map.end() || map_it->second.first.empty()) {
        return nullptr;
    } else {
        // Make sure the list ends with NULL
        std::vector<kp_list>& list = map_it->second.first;
        if (list.back().kp != nullptr) {
            list.push_back({-1, nullptr});
        }
        return list.data();
    }
}

struct pst_list* SubtableMap_get_pst_list(cpp_SubtableMap* map,
                                          struct lookup_subtable* subtable) {
    SubtableMap& st_map = *reinterpret_cast<SubtableMap*>(map);
    auto map_it = st_map.find(subtable);
    if (map_it == st_map.end() || map_it->second.second.empty()) {
        return nullptr;
    } else {
        // Make sure the list ends with NULL
        std::vector<pst_list>& list = map_it->second.second;
        if (list.back().pst != nullptr) {
            list.push_back({-1, nullptr});
        }
        return list.data();
    }
}
