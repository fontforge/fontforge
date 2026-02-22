/* Copyright 2024 Maxim Iorsh <iorsh@users.sourceforge.net>
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
#pragma once

#include <map>
#include <set>
#include <vector>

#include "metrics.h"
#include "opentype_str.h"
#include "tag.hpp"

typedef uint32_t unichar_t;

typedef struct splinechar SplineChar;
typedef struct metricsview MetricsView;

namespace ff::shapers {

using ShaperOutput = std::pair<struct opentype_str*, std::vector<MetricsCore>>;

class IShaper {
 public:
    virtual ~IShaper() = default;

    // The internal shaper name is non-localizable and serves to identify the
    // shaper in the system.
    virtual const char* name() const = 0;

    // ubuf - buffer of Unicode encodings. External shapers (e.g. HarfBuzz) can
    // only accept Unicode-based input, so unencoded glyphs need to be mapped to
    // fake encodings.
    virtual std::vector<MetricsCore> apply_features(
        const std::vector<unichar_t>& ubuf,
        const std::map<Tag, bool>& feature_map, Tag script, Tag lang,
        bool vertical) = 0;

    // glyphs - a sequence of glyphs to be shaped
    // NOTE: the glyph sequence can't be passed as a Unicode string, since some
    // glyphs don't have encoding at all, and the shaper should still be able to
    // apply features which involve these glyphs.
    //
    // TODO(iorsh): eliminate the reliance on SplineChar structure when
    // modernizing the Metrics View.
    virtual ShaperOutput mv_apply_features(
        SplineChar** glyphs, const std::map<Tag, bool>& feature_map, Tag script,
        Tag lang, int pixelsize, bool vertical) = 0;

    // Scale glyph sequence metrics from font units to pixels
    virtual void scale_metrics(MetricsView* mv, MetricsCore* metrics,
                               double iscale, double scale, bool vertical) = 0;

    // OpenType features enabled by default
    virtual std::set<Tag> default_features(Tag script, Tag lang,
                                           bool vertical) const = 0;
};

}  // namespace ff::shapers
