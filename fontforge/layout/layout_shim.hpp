/* Copyright 2026 Maxim Iorsh <iorsh@users.sourceforge.net>
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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct splinefont SplineFont;

/* Dummy incomplete type which can be casted to C++ type
 * ff::layout::SplineFontProperties */
typedef struct cpp_SplineFontProperties cpp_SplineFontProperties;

/* Create a new SplineFontProperties object */
cpp_SplineFontProperties* SFGetProperties(SplineFont* sf);
cpp_SplineFontProperties* make_SplineFontProperties(int ascent, int descent,
                                                    bool italic,
                                                    int16_t os2_weight,
                                                    int16_t os2_width,
                                                    const char* styles);

#ifdef __cplusplus
}

#include <string>
#include <vector>

using ParsedTag =
    std::pair<std::string /*tag name*/, std::string /*tag value*/>;

namespace ff::layout {

struct SplineFontProperties {
    int ascent = -1, descent = -1;
    bool italic = false;
    int16_t os2_weight = -1;
    int16_t os2_width = -1;
    std::string styles;

    static SplineFontProperties from_tags(const std::vector<ParsedTag>& tags);

    // Merge properties, with the other object's meaningful fields taking
    // priority.
    void merge(const SplineFontProperties& other);

    // Despite its name, distance() is not guaranteed to be a metric in
    // mathematical sense.
    int distance(const SplineFontProperties& other) const;
};

}  // namespace ff::layout

inline ff::layout::SplineFontProperties* toCPP(cpp_SplineFontProperties* p) {
    return reinterpret_cast<ff::layout::SplineFontProperties*>(p);
}

inline cpp_SplineFontProperties* toC(ff::layout::SplineFontProperties* p) {
    return reinterpret_cast<cpp_SplineFontProperties*>(p);
}

#endif  // __cplusplus
