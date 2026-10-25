/* Copyright (C) 2026 by Maxim Iorsh <iorsh@users@sourceforge.net>
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

enum hist_type { hist_hstem, hist_vstem, hist_blues };
typedef struct gwindow* GWindow;
typedef struct splinefont SplineFont;
struct psdict;
typedef struct encmap EncMap;
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
void SFHistogram(GWindow parent, SplineFont* sf, int layer,
                 struct psdict* private_dict, uint8_t* selected, EncMap* map,
                 enum hist_type which);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "l10n_text.hpp"
#include "intl.h"

namespace ff::dlg {

struct HistogramBarRecord {
    // Total count of hints or blues with this value.
    unsigned int count;

    // List of characters with this hint or blue value. The number of characters
    // may be smaller than cnt, because the same character may have multiple
    // hints or blues with the same value.
    std::vector<std::string> glyph_names;
};

struct PrivateDictValues {
    std::string primary;
    std::string secondary;
};

struct HistogramData {
    enum hist_type type;
    int lower_bound;
    bool small_selection_warning;
    std::vector<HistogramBarRecord> bars;
    PrivateDictValues initial_values;
};

struct UiStrings {
    L10nText title;
    std::string primary_label;
    std::string secondary_label;
};

const std::map<hist_type, UiStrings> kHistogramUiStrings = {
    {hist_hstem, {_("Horizontal Stem Hints"), "StdHW", "StemSnapH"}},
    {hist_vstem, {_("Vertical Stem Hints"), "StdVW", "StemSnapV"}},
    {hist_blues, {_("Blues"), "BlueValues", "OtherBlues"}},
};

std::optional<PrivateDictValues> show_histogram_dialog(
    GWindow parent, const HistogramData& data);

}  // namespace ff::dlg

#endif  // __cplusplus
