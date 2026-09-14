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

#include "font_selector.hpp"

#include <algorithm>

namespace ff::layout {

size_t select_face(const std::vector<ParsedTag>& parsed_tags,
                   const std::vector<SplineFontProperties>& properties_list,
                   const SplineFontProperties& default_properties) {
    // Desired properties are derived from the default ones, with
    // segment-specific tags overriding them when applicable.
    SplineFontProperties text_props =
        SplineFontProperties::from_tags(parsed_tags);
    SplineFontProperties desired_properties = default_properties;
    desired_properties.merge(text_props);

    // Find the face with properties closest to the desired properties.
    auto closest_face =
        std::min_element(properties_list.begin(), properties_list.end(),
                         [&desired_properties](const auto& a, const auto& b) {
                             return desired_properties.distance(a) <
                                    desired_properties.distance(b);
                         });

    return closest_face - properties_list.begin();
}

}  // namespace ff::layout
