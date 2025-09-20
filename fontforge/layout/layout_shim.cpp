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
#include "layout_shim.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <regex>

namespace ff::layout {

extern "C" cpp_SplineFontProperties* make_SplineFontProperties(
    int ascent, int descent, bool italic, int16_t os2_weight, int16_t os2_width,
    const char* styles) {
    return toC(new SplineFontProperties{ascent, descent, italic, os2_weight,
                                        os2_width, styles});
}

SplineFontProperties SplineFontProperties::from_tags(
    const std::vector<std::string>& tags) {
    static const std::map<std::string, int16_t> widths{
        {"ultra-condensed", 1}, {"extra-condensed", 2}, {"condensed", 3},
        {"semi-condensed", 4},  {"medium", 5},          {"semi-expanded", 6},
        {"expanded", 7},        {"extra-expanded", 8},  {"ultra-expanded", 9},
    };
    SplineFontProperties props;
    for (const std::string& tag : tags) {
        auto [tag_name, tag_value] = parse_tag(tag);
        if (tag_name == "italic") {
            props.italic = true;
        } else if (tag_name == "bold") {
            props.os2_weight = 700;
        } else if (tag_name == "width") {
            props.os2_width = widths.at(tag_value);
        }
    }
    return props;
}

void SplineFontProperties::merge(const SplineFontProperties& other) {
    if (other.ascent != -1) ascent = other.ascent;
    if (other.descent != -1) descent = other.descent;
    italic = other.italic;
    if (other.os2_weight != -1) os2_weight = other.os2_weight;
    if (other.os2_width != -1) os2_width = other.os2_width;
    if (!other.styles.empty()) styles = other.styles;
}

int SplineFontProperties::distance(const SplineFontProperties& other) const {
    // A wildly heuristic mapping preference for width property. As a general
    // rule, we prefer exact match, of course, but if there is no exact match we
    // prefer to preserve expansion or condensing.
    static const std::map<int16_t, std::vector<int16_t>> width_mapper = {
        {1, {1, 2, 3, 4, 5, 6, 7, 8, 9}}, {2, {2, 1, 3, 4, 5, 6, 7, 8, 9}},
        {3, {3, 2, 4, 1, 5, 6, 7, 8, 9}}, {4, {4, 3, 5, 2, 1, 6, 7, 8, 9}},
        {5, {5, 4, 6, 3, 7, 2, 8, 1, 9}}, {6, {6, 7, 5, 8, 9, 4, 3, 2, 1}},
        {7, {7, 8, 6, 9, 5, 4, 3, 2, 1}}, {8, {8, 9, 7, 6, 5, 4, 3, 2, 1}},
        {9, {9, 8, 7, 6, 5, 4, 3, 2, 1}},
    };
    const std::vector<int16_t>& m = width_mapper.at(os2_width);
    int width_dist = std::find(m.begin(), m.end(), other.os2_width) - m.begin();

    return (int)(italic ^ other.italic) * 100 +
           std::fabs(os2_weight - other.os2_weight) + width_dist * 100;
}

std::pair<std::string /*tag*/, std::string /*value*/> parse_tag(
    const std::string& complete_tag) {
    // Attempt to match tag name and "value" attribute
    // The string below is equivalent to ^(.+?)\s+value=\"(.+?)\"
    std::regex re(R"-(^(.+?)\s+value=\"(.+?)\")-");
    std::smatch tag_value_match;
    if (std::regex_match(complete_tag, tag_value_match, re) &&
        (tag_value_match.size() == 3)) {
        // tag_value_match[0] holds the complete match, ignore it. We need just
        // the capturing groups.
        return {tag_value_match[1], tag_value_match[2]};
    }

    // Return tag name only, with "set" as value by convention.
    auto space_it =
        std::find_if(complete_tag.begin(), complete_tag.end(),
                     [](unsigned char c) { return std::isspace(c); });
    std::string tag_name(complete_tag.begin(), space_it);
    return {tag_name, "set"};
}

}  // namespace ff::layout
