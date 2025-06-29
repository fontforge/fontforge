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
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Dummy incomplete type which can be casted to C++ type SubtableMap */
typedef struct cpp_SubtableMap cpp_SubtableMap;

typedef struct kernpair KernPair;
typedef struct generic_pst PST;

struct kp_list {
    int gid;
    KernPair* kp;
};

struct pst_list {
    int gid;
    PST* pst;
};

cpp_SubtableMap* SubtableMap_new();

void SubtableMap_delete(cpp_SubtableMap** p_map);

void SubtableMap_add_kp(cpp_SubtableMap* map, struct lookup_subtable* subtable,
                        int gid, KernPair* kp);

void SubtableMap_add_pst(cpp_SubtableMap* map, struct lookup_subtable* subtable,
                         int gid, PST* pst);

int SubtableMap_get_size(cpp_SubtableMap* map,
                         struct lookup_subtable* subtable);

struct kp_list* SubtableMap_get_kp_list(cpp_SubtableMap* map,
                                        struct lookup_subtable* subtable);

struct pst_list* SubtableMap_get_pst_list(cpp_SubtableMap* map,
                                          struct lookup_subtable* subtable);

#ifdef __cplusplus
}

#include <unordered_map>
#include <vector>

using SubtableMap = std::unordered_map<
    struct lookup_subtable*,
    std::pair<std::vector<struct kp_list>, std::vector<struct pst_list>>>;

#endif  // __cplusplus
