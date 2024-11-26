/* Copyright 2023 Maxim Iorsh <iorsh@users.sourceforge.net>
 *
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
#include "shaper_shim.hpp"

#include <memory>
#include <stddef.h>
#include <string.h>

#include "intl.h"
#include "builtin.hpp"
#include "harfbuzz.hpp"

const ShaperDef* get_shaper_defs() {
    static ShaperDef shaper_defs[] = {{"builtin", N_("Built-in shaper")},
#ifdef ENABLE_HARFBUZZ
                                      {"harfbuzz", N_("HarfBuzz")},
#endif
                                      {NULL, NULL}};

    return shaper_defs;
}

const char* get_default_shaper() {
#ifdef ENABLE_HARFBUZZ
    return "harfbuzz";
#endif
    return "builtin";
}

void* shaper_factory(const char* name, ShaperContext* r_context) {
    // Take ownership of r_context
    std::shared_ptr<ShaperContext> context(r_context);

#ifdef ENABLE_HARFBUZZ
    if (strcmp(name, "harfbuzz") == 0) {
        return new ff::shapers::HarfBuzzShaper(context);
    }
#endif
    if (strcmp(name, "builtin") == 0) {
        return new ff::shapers::BuiltInShaper(context);
    } else {
        return nullptr;
    }
}

void shaper_free(void** p_shaper) {
    if (p_shaper == nullptr) {
        return;
    }

    ff::shapers::IShaper* shaper =
        static_cast<ff::shapers::IShaper*>(*p_shaper);
    delete shaper;
    *p_shaper = nullptr;
}

const char* shaper_name(void* shaper) {
    ff::shapers::IShaper* ishaper = static_cast<ff::shapers::IShaper*>(shaper);

    if (shaper) {
        return ishaper->name();
    } else {
        return "";
    }
}
