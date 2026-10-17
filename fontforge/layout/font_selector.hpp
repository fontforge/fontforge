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

#include <map>
#include <vector>

#include "layout_shim.hpp"

namespace ff::layout {

// A sequence of text segments with associated tags.
using ParsedRichText = std::vector<
    std::pair<std::vector<std::string> /*tags*/, std::string /*text*/>>;

class RichTextFontSelector {
 public:
    explicit RichTextFontSelector(
        const std::vector<SplineFontProperties>& properties_list = {})
        : properties_list_(properties_list) {}

    // Select specific face to print a text segment based on the tags which
    // apply to it. Returns an index into properties_list vector.
    size_t select_face(const std::vector<ParsedTag>& parsed_tags,
                       const SplineFontProperties& default_properties) const;

    std::stringstream xml_manifest() const;

    // Compose a sample text XML document, which contains the manifest and the
    // contents of RichTextEditor text buffer with tags normalized to proper XML
    // format. While normally saved from the UI, this function can be used to
    // generate a sample text XML from any tagged text, e.g. for testing
    // purposes.
    std::string compose_sample_text_xml(const std::string& tagged_text) const;

    // Map font names imported from a sample text XML to the font names
    // available in the system.
    std::map<std::string, std::string> map_fonts(
        const std::vector<SplineFontProperties>& imported_fonts) const;

    static const std::string rich_text_mime_type;

 private:
    std::vector<SplineFontProperties> properties_list_;
};

// By convention, TextBuffer::Tag name may come in the format
// "tag_name|tag_value". This format should be dumped as <tag_name
// value="tag_value">.
void dump_tag(std::string& unicode_buffer, const std::string& tag_name,
              bool opening);

// Normalizes Gtk::TextBuffer dump to a proper XML, by ensuring that all the
// tags are properly nested and not interleaved. Tags are also normalized from
// RichTextEditor naming convention to a more human-oriented format - see
// dump_tag().
std::string normalize_text_buffer_dump(const std::string& dump);

ParsedRichText parse_xml_stream(std::istream& input);

ParsedTag parse_tag(const std::string& complete_tag);

std::vector<SplineFontProperties> parse_manifest(const ParsedRichText& parsed);

ParsedRichText remap_font_names(
    const ParsedRichText& parsed,
    const std::map<std::string, std::string>& font_name_mapping);

layout::ParsedRichText collapse_html_spaces(
    const layout::ParsedRichText& input);
}  // namespace ff::layout
