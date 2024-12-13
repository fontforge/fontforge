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

#include <vector>

#include "metrics.h"
#include "tag.hpp"

typedef struct splinechar SplineChar;
typedef struct metricsview MetricsView;
struct opentype_str;

namespace ff::shapers {

class IShaper {
 public:
    // The internal shaper name is non-localizable and serves to identify the
    // shaper in the system.
    virtual const char* name() const = 0;

    // glyphs - a sequence of glyphs to be shaped
    // NOTE: the glyph sequence can't be passed as a Unicode string, since some
    // glyphs don't have encoding at all, and the shaper should still be able to
    // apply features which involve these glyphs.
    virtual struct opentype_str* apply_features(
        SplineChar** glyphs, const std::vector<Tag>& feature_list, Tag script,
        Tag lang, int pixelsize) = 0;

    virtual void scale_metrics(MetricsView* mv, double iscale, double scale,
                               bool vertical) = 0;

 public:
    // Array of glyph metrics
    std::vector<ShapeMetrics> metrics;
};

}  // namespace ff::shapers
