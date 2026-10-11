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
#include <iostream>
#include <map>
#include <sstream>
#include <stack>

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

std::string RichTextFontSelector::compose_sample_text_xml(
    const std::string& tagged_text) const {
    std::string manifest = xml_manifest().str();

    std::string normalized_xml =
        layout::normalize_text_buffer_dump(tagged_text);
    std::string unicode_buffer;

    dump_tag(unicode_buffer, "ff-sample", true);
    unicode_buffer += manifest;
    dump_tag(unicode_buffer, "body", true);
    unicode_buffer += normalized_xml;
    dump_tag(unicode_buffer, "body", false);
    dump_tag(unicode_buffer, "ff-sample", false);
    return unicode_buffer;
}

const std::string RichTextFontSelector::rich_text_mime_type =
    "application/vnd.fontforge.rich-text+xml";

void dump_tag(std::string& unicode_buffer, const std::string& tag_name,
              bool opening) {
    std::string name, value;
    size_t delim = tag_name.find('|');
    if (delim != std::string::npos) {
        name = tag_name.substr(0, delim);
        value = tag_name.substr(delim + 1);
    } else {
        name = tag_name;
    }

    // Check if the closing tag is directly following the opening tag, in which
    // case both should be dropped.
    if (!opening && unicode_buffer.size() >= name.size() + 2 &&
        *unicode_buffer.rbegin() == '>') {
        size_t opening_pos = unicode_buffer.rfind('<');
        if (unicode_buffer.find("<" + name + ">", opening_pos) == opening_pos ||
            unicode_buffer.find("<" + name + " ", opening_pos) == opening_pos) {
            unicode_buffer.erase(opening_pos);
            return;
        }
    }

    unicode_buffer.push_back('<');
    if (!opening) {
        unicode_buffer.push_back('/');
    }
    unicode_buffer += name;
    if (opening && !value.empty()) {
        unicode_buffer += " value=\"" + value + '\"';
    }
    unicode_buffer.push_back('>');
}

std::string normalize_text_buffer_dump(const std::string& dump) {
    std::string text, tag;
    std::istringstream input(dump);
    std::string unicode_buffer;

    // Gtk::TextBuffer doesn't enforce nested ranges, so the sequence
    // "aa<bold>bc<italic>dd</bold>efg</italic>hi" is perfectly valid. We will
    // use the tag stack to normalize opening and closing tags to follow XML
    // convention.
    std::stack<std::string> open_tags;

    while (std::getline(input, text, '<') && std::getline(input, tag, '>')) {
        // Dump the text before the tag.
        unicode_buffer += text;

        // Just dump the opening tag. All problems are resolved when we
        // encounter the closing tag.
        if (tag.size() == 0 || tag.front() != '/') {
            dump_tag(unicode_buffer, tag, true);
            open_tags.push(tag);
            continue;
        }

        // Close interleaved tags if there are any, before the pending closing
        // tag.
        std::stack<std::string> temporarily_closed_tags;

        // Try to close the tags in the reverse order of opening
        tag.erase(0, 1);  // Remove the leading '/'
        while (!open_tags.empty() && tag != open_tags.top()) {
            dump_tag(unicode_buffer, open_tags.top(), false);
            temporarily_closed_tags.push(open_tags.top());
            open_tags.pop();
        }

        if (open_tags.empty()) {
            // Closing tag is conflicting with the open tags stack
            std::cerr << "TextBuffer corruption: closing tag " << tag
                      << " doesn't match any open tags." << std::endl;
        } else {
            // Closing tag correctly corresponds to the latest open tag
            dump_tag(unicode_buffer, tag, false);
            open_tags.pop();
        }

        // Reopen the tags which were temporarily closed to resolve
        // conflicts.
        while (!temporarily_closed_tags.empty()) {
            const std::string& tag_name = temporarily_closed_tags.top();
            dump_tag(unicode_buffer, tag_name, true);
            open_tags.push(tag_name);
            temporarily_closed_tags.pop();
        }
    }

    // Push back whatever text has left.
    unicode_buffer += text;

    // Dump the remaining closing tags, if any. This is not an error.
    while (!open_tags.empty()) {
        dump_tag(unicode_buffer, open_tags.top(), false);
        open_tags.pop();
    }

    return unicode_buffer;
}

}  // namespace ff::layout
