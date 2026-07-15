/* Copyright (C) 2024 by FontForge Authors */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.

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

/*
 * Cross-platform directory iteration using C++17 filesystem.
 * Provides extern "C" wrappers for use from C code.
 */

#include <fontforge-config.h>
#include "ffdir.h"

#include <filesystem>
#include <string>
#include <cstring>

namespace fs = std::filesystem;

struct FF_Dir {
    std::string path;
    fs::directory_iterator iter;
    fs::directory_iterator end;
    FF_DirEntry current;
    bool has_current;

    FF_Dir(const char* p)
        : path(p), iter(fs::path(p)), end(), has_current(false) {}

    void rewind() { iter = fs::directory_iterator(fs::path(path)); }
};

extern "C" {

FF_Dir* ff_opendir(const char* path) {
    try {
        if (!fs::is_directory(path)) {
            return nullptr;
        }
        return new FF_Dir(path);
    } catch (...) {
        return nullptr;
    }
}

FF_DirEntry* ff_readdir(FF_Dir* dir) {
    if (!dir) {
        return nullptr;
    }
    try {
        if (dir->iter == dir->end) {
            return nullptr;
        }
        const auto& entry = *dir->iter;
        std::string name = entry.path().filename().string();
        if (name.length() >= sizeof(dir->current.name)) {
            name = name.substr(0, sizeof(dir->current.name) - 1);
        }
        std::strncpy(dir->current.name, name.c_str(),
                     sizeof(dir->current.name) - 1);
        dir->current.name[sizeof(dir->current.name) - 1] = '\0';
        dir->has_current = true;
        ++dir->iter;
        return &dir->current;
    } catch (...) {
        return nullptr;
    }
}

void ff_closedir(FF_Dir* dir) { delete dir; }

void ff_rewinddir(FF_Dir* dir) {
    if (dir) {
        try {
            dir->rewind();
        } catch (...) {
            /* Ignore errors on rewind */
        }
    }
}

} /* extern "C" */
