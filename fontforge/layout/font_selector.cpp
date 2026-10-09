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
#include <map>
#include <sstream>

namespace ff::layout {

size_t RichTextFontSelector::select_face(
    const std::vector<ParsedTag>& parsed_tags,
    const SplineFontProperties& default_properties) const {
    // Desired properties are derived from the default ones, with
    // segment-specific tags overriding them when applicable.
    SplineFontProperties text_props =
        SplineFontProperties::from_tags(parsed_tags);
    SplineFontProperties desired_properties = default_properties;
    desired_properties.merge(text_props);

    // Find the face with properties closest to the desired properties.
    auto closest_face =
        std::min_element(properties_list_.begin(), properties_list_.end(),
                         [&desired_properties](const auto& a, const auto& b) {
                             return desired_properties.distance(a) <
                                    desired_properties.distance(b);
                         });

    return closest_face - properties_list_.begin();
}

std::stringstream RichTextFontSelector::xml_manifest() const {
    std::stringstream unicode_buffer;
    unicode_buffer << "<manifest>";
    bool default_face_written = false;

    static const std::map<int16_t, std::string> weight_to_name = {
        {100, "thin"},    {200, "extra-light"}, {300, "light"},
        {400, "regular"}, {500, "medium"},      {600, "semi-bold"},
        {700, "bold"},    {800, "extra-bold"},  {900, "black"}};
    static const std::map<std::string, int16_t> name_to_weight = {
        {"thin", 100},    {"extra-light", 200}, {"light", 300},
        {"regular", 400}, {"medium", 500},      {"semi-bold", 600},
        {"bold", 700},    {"extra-bold", 800},  {"black", 900}};
    static const std::map<int16_t, std::string> stretch_to_name = {
        {1, "ultra-condensed"}, {2, "extra-condensed"}, {3, "condensed"},
        {4, "semi-condensed"},  {5, "normal"},          {6, "semi-expanded"},
        {7, "expanded"},        {8, "extra-expanded"},  {9, "ultra-expanded"}};
    static const std::map<std::string, int16_t> name_to_stretch = {
        {"ultra-condensed", 1}, {"extra-condensed", 2}, {"condensed", 3},
        {"semi-condensed", 4},  {"normal", 5},          {"semi-expanded", 6},
        {"expanded", 7},        {"extra-expanded", 8},  {"ultra-expanded", 9}};

    for (const auto& properties : properties_list_) {
        unicode_buffer << "<font key=\"" << properties.full_name << "\""
                       << (default_face_written ? "" : " default=\"true\"")
                       << ">";
        default_face_written = true;
        unicode_buffer << "<family>" << properties.family_name << "</family>";
        unicode_buffer << "<name>" << properties.full_name << "</name>";
        if (properties.os2_weight >= 0) {
            unicode_buffer << "<weight>"
                           << weight_to_name.at(properties.os2_weight)
                           << "</weight>";
        }
        if (properties.os2_width >= 0) {
            unicode_buffer << "<stretch>"
                           << stretch_to_name.at(properties.os2_width)
                           << "</stretch>";
        }
        if (properties.italic) {
            unicode_buffer << "<style>italic</style>";
        }
        unicode_buffer << "</font>";
    }
    unicode_buffer << "</manifest>";
    return unicode_buffer;
}

const std::string RichTextFontSelector::rich_text_mime_type =
    "application/vnd.fontforge.rich-text+xml";

}  // namespace ff::layout
