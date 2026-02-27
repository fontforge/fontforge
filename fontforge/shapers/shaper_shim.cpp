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

using ff::Tag;

// Default shaper, managed in preferences. Must point to one of strings in
// static get_shaper_defs::shaper_defs[].
const char* default_shaper = NULL;

const ShaperDef* get_shaper_defs() {
    static ShaperDef shaper_defs[] = {{"builtin", _("Built-in shaper")},
#ifdef ENABLE_HARFBUZZ
                                      {"harfbuzz", _("HarfBuzz")},
#endif
                                      {NULL, NULL}};

    return shaper_defs;
}

void set_default_shaper(const char* name) {
    // The desired shaper must be in the list of available shapers, otherwise it
    // is ignored
    const ShaperDef *sh_def, *shaper_defs = get_shaper_defs();
    for (sh_def = shaper_defs; sh_def->name != NULL; ++sh_def) {
        if (strcmp(name, sh_def->name) == 0) {
            break;
        }
    }

    if (sh_def->name != NULL) {
        default_shaper = sh_def->name;
    }
}

const char* get_default_shaper() {
    if (default_shaper) {
        return default_shaper;
    }

#ifdef ENABLE_HARFBUZZ
    return "harfbuzz";
#endif
    return "builtin";
}

inline ff::shapers::IShaper* toCPP(cpp_IShaper* p) {
    return reinterpret_cast<ff::shapers::IShaper*>(p);
}

inline cpp_IShaper* toC(ff::shapers::IShaper* p) {
    return reinterpret_cast<cpp_IShaper*>(p);
}

cpp_IShaper* shaper_factory(const char* name, ShaperContext* r_context) {
    // Take ownership of r_context
    std::shared_ptr<ShaperContext> context(r_context, &free);

#ifdef ENABLE_HARFBUZZ
    if (strcmp(name, "harfbuzz") == 0) {
        return toC(new ff::shapers::HarfBuzzShaper(context));
    }
#endif
    if (strcmp(name, "builtin") == 0) {
        return toC(new ff::shapers::BuiltInShaper(context));
    } else {
        return nullptr;
    }
}

void shaper_free(cpp_IShaper** p_shaper) {
    if (p_shaper == nullptr) {
        return;
    }

    ff::shapers::IShaper* shaper = toCPP(*p_shaper);
    delete shaper;
    *p_shaper = nullptr;
}

const char* shaper_name(cpp_IShaper* shaper) {
    ff::shapers::IShaper* ishaper = toCPP(shaper);

    if (shaper) {
        return ishaper->name();
    } else {
        return "";
    }
}

struct shaper_out shaper_apply_features(cpp_IShaper* shaper,
                                        SplineChar** glyphs,
                                        FeatureMap* feat_map, uint32_t script,
                                        uint32_t lang, int pixelsize,
                                        bool vertical) {
    ff::shapers::IShaper* ishaper = toCPP(shaper);
    std::map<Tag, bool> feature_map;
    for (int i = 0; feat_map[i].feature_tag != 0; ++i) {
        feature_map[feat_map[i].feature_tag] = feat_map[i].enabled;
    }

    if (shaper) {
        auto output = ishaper->mv_apply_features(
            glyphs, feature_map, Tag(script), Tag(lang), pixelsize, vertical);
        MetricsCore* metrics =
            (MetricsCore*)calloc(output.second.size() + 1, sizeof(MetricsCore));
        memcpy(metrics, output.second.data(),
               output.second.size() * sizeof(MetricsCore));
        return {output.first, metrics};
    } else {
        return {0, 0};
    }
}

void shaper_scale_metrics(cpp_IShaper* shaper, MetricsView* mv,
                          MetricsCore* metrics, double iscale, double scale,
                          bool vertical) {
    ff::shapers::IShaper* ishaper = toCPP(shaper);
    ishaper->scale_metrics(mv, metrics, iscale, scale, vertical);
}

uint32_t* shaper_default_features(cpp_IShaper* shaper, uint32_t script,
                                  uint32_t lang, bool vertical) {
    ff::shapers::IShaper* ishaper = toCPP(shaper);

    const std::set<Tag> default_feats =
        ishaper->default_features(script, lang, vertical);

    size_t n_feats = default_feats.size();
    uint32_t* stds = (uint32_t*)calloc(n_feats + 2, sizeof(uint32_t));
    std::copy(default_feats.begin(), default_feats.end(), stds);

    // By legacy FontForge convention the list of default features is terminated
    // by " RQD" and zero.
    stds[n_feats] = ff::REQUIRED_FEATURE;
    stds[n_feats + 1] = 0;

    return stds;
}
