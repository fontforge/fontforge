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
#include <cmath>
#include <iostream>
#include <regex>
#include <set>
#include <sstream>
#include <stack>

namespace ff::layout {

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

std::map<std::string, std::string> RichTextFontSelector::map_fonts(
    const std::vector<SplineFontProperties>& imported_fonts) const {
    std::map<std::string, std::string> font_mapping;

    // Force weight remapping for imported fonts. A possible case is when the
    // available fonts have weights e.g. 500, 700 and the imported fonts have
    // weights 300, 600. The weight 600 may be mapped to either 500 or 700, and
    // there is no guarantee about it. We shall match the weights manually
    // before proceeding further.
    std::set<int16_t> available_weights;
    for (const auto& properties : properties_list_) {
        if (properties.os2_weight >= 0) {
            available_weights.insert(properties.os2_weight);
        }
    }
    std::set<int16_t> imported_weights;
    for (const auto& properties : imported_fonts) {
        if (properties.os2_weight >= 0) {
            imported_weights.insert(properties.os2_weight);
        }
    }
    std::vector<SplineFontProperties> remapped_imported_fonts = imported_fonts;
    if (available_weights.size() > 1 && imported_weights.size() > 1) {
        // Remap imported weights to available weights using linear mapping.
        // It's a hack, but it should work in most cases.
        double k =
            (double)(*available_weights.rbegin() - *available_weights.begin()) /
            (*imported_weights.rbegin() - *imported_weights.begin());
        double b = *available_weights.begin() - k * (*imported_weights.begin());
        for (auto& properties : remapped_imported_fonts) {
            if (properties.os2_weight >= 0) {
                properties.os2_weight =
                    std::round(k * properties.os2_weight + b);
            }
        }
    }

    for (const auto& imported_font : remapped_imported_fonts) {
        // Find the face with properties closest to the desired properties.
        auto closest_face = std::min_element(
            properties_list_.begin(), properties_list_.end(),
            [&imported_font](const auto& a, const auto& b) {
                return imported_font.distance(a) < imported_font.distance(b);
            });
        font_mapping[imported_font.full_name] = closest_face->full_name;
    }
    return font_mapping;
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

static bool tags_match(const std::string& opening_tag,
                       const std::string& closing_tag) {
    // Compare the opening tag without the attributes (which may be present
    // after the space delimiter or not present at all).
    size_t space_in_opening_tag = opening_tag.find(' ');
    // Compare with the closing tag without its leading slash character.
    return opening_tag.compare(0, space_in_opening_tag, closing_tag, 1,
                               std::string::npos) == 0;
}

static std::string unescape_xml(const std::string& text_in) {
    std::string text_out;
    text_out = std::regex_replace(text_in, std::regex("&lt;"), "<");
    text_out = std::regex_replace(text_out, std::regex("&quot;"), "\"");
    text_out = std::regex_replace(text_out, std::regex("&apos;"), "\'");
    text_out = std::regex_replace(text_out, std::regex("&gt;"), ">");
    text_out = std::regex_replace(text_out, std::regex("&amp;"), "&");
    return text_out;
}

ParsedRichText parse_xml_stream(std::istream& input) {
    std::string text, tag;
    // Array of text blocks as follows: (text block, list of tags applied on
    // that block)
    ParsedRichText parsed_input;
    std::vector<std::string> current_tags;

    while (std::getline(input, text, '<') && std::getline(input, tag, '>')) {
        if (!text.empty()) {
            std::string unescaped_text = unescape_xml(text);
            parsed_input.emplace_back(current_tags, unescaped_text);
        }
        if (tag.size() > 0 && tag.front() == '/') {
            if (!current_tags.empty() &&
                tags_match(tag, current_tags.back()) == 0) {
                current_tags.pop_back();
            } else {
                std::cerr << "Rich text XML parser failed at tag \"" << tag
                          << "\", position " << input.tellg() << std::endl;
                break;
            }
        } else if (tag.size() > 0 && tag.back() == '/') {
            // Self-closing tag, like <meta/>. We can ignore it, since it
            // doesn't affect the style of any text.
        } else {
            current_tags.push_back(tag);
        }
    }

    // Generally valid XML doesn't have trailing text after all tags have been
    // closed. Push back whatever we have read, just in case.
    if (!text.empty()) {
        parsed_input.emplace_back(current_tags, text);
    }

    return parsed_input;
}

ParsedTag parse_tag(const std::string& complete_tag) {
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

std::vector<SplineFontProperties> parse_manifest(const ParsedRichText& parsed) {
    std::map<std::string, SplineFontProperties> font_map;
    // Extract font name using pattern <font key="font_name">, without angular
    // brackets.
    static const std::regex font_key_regex(R"-(font\s+key="([^"]+)")-");
    std::string default_font_key;

    for (const auto& [tags, text] : parsed) {
        if (tags.size() >= 4 && tags[0] == "ff-sample" &&
            tags[1] == "manifest" && tags[2].substr(0, 5) == "font ") {
            std::smatch match;
            if (std::regex_search(tags[2], match, font_key_regex) &&
                match.size() == 2) {
                std::string font_key = match[1];
                SplineFontProperties& current_font_properties =
                    font_map[font_key];

                if (tags[2].find("default=\"true\"") != std::string::npos) {
                    default_font_key = font_key;
                }

                const std::string& style_tag = tags[3];
                if (style_tag == "family") {
                    current_font_properties.family_name = text;
                } else if (style_tag == "name") {
                    current_font_properties.full_name = text;
                } else if (style_tag == "weight") {
                    auto it = name_to_weight.find(text);
                    if (it != name_to_weight.end()) {
                        current_font_properties.os2_weight = it->second;
                    }
                } else if (style_tag == "stretch") {
                    auto it = name_to_stretch.find(text);
                    if (it != name_to_stretch.end()) {
                        current_font_properties.os2_width = it->second;
                    }
                } else if (style_tag == "style") {
                    current_font_properties.italic = (text == "italic");
                }
            }
        }
    }

    std::vector<SplineFontProperties> font_list;
    // If a default font was specified, move it to the front of the list.
    if (!default_font_key.empty()) {
        font_list.push_back(font_map[default_font_key]);
        font_map.erase(default_font_key);
    }
    for (const auto& [key, properties] : font_map) {
        font_list.push_back(properties);
    }

    return font_list;
}

ParsedRichText remap_font_names(
    const ParsedRichText& parsed,
    const std::map<std::string, std::string>& font_name_mapping) {
    static const std::regex font_value_regex(R"-(font\s+value="([^"]+)")-");

    ParsedRichText remapped = parsed;
    for (auto& [tags, text] : remapped) {
        for (std::string& tag : tags) {
            if (tag.substr(0, 5) == "font ") {
                // Remap the font name in the tag, if a mapping exists.
                std::smatch match;
                if (std::regex_search(tag, match, font_value_regex) &&
                    match.size() == 2) {
                    std::string font_key = match[1];
                    auto it = font_name_mapping.find(font_key);
                    if (it != font_name_mapping.end()) {
                        tag.replace(match.position(1), match.length(1),
                                    it->second);
                    }
                }
            }
        }
    }
    return remapped;
}

layout::ParsedRichText collapse_html_spaces(
    const layout::ParsedRichText& input) {
    layout::ParsedRichText collapsed;
    bool in_space = false;
    bool prev_in_p = false;
    auto tag_is_p = [](const std::string& tag) {
        return tag == "p" || tag.substr(0, 2) == "p ";
    };

    for (const auto& [tags, text] : input) {
        bool cur_in_p =
            std::find_if(tags.begin(), tags.end(), tag_is_p) != tags.end();
        bool new_paragraph = cur_in_p && !prev_in_p;
        prev_in_p = cur_in_p;

        std::string normalized_text;
        for (char ch : text) {
            if (std::isspace(static_cast<unsigned char>(ch))) {
                if (!in_space) {
                    normalized_text += ' ';
                    in_space = true;
                }
            } else {
                normalized_text += ch;
                in_space = false;
            }
        }

        if (new_paragraph && !collapsed.empty()) {
            normalized_text = '\n' + normalized_text;
        }
        if (!normalized_text.empty()) {
            collapsed.emplace_back(tags, normalized_text);
        }
    }

    return collapsed;
}

}  // namespace ff::layout
