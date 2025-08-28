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

#include "cairo_painter.hpp"

#include <array>
#include <set>
#include <sstream>

extern "C" {
#include "fffreetype.h"
#include "gutils.h"
#include "splinechar.h"
#include "ustring.h"

extern SplineFont** FVCollectFamily(SplineFont* sf);
}

namespace ff::utils {

void CairoPainter::draw_page(const Cairo::RefPtr<Cairo::Context>& cr,
                             double scale,
                             const Cairo::Rectangle& printable_area,
                             int page_nr, const std::string& sample_text,
                             double font_size) {
    cr->translate(printable_area.x, printable_area.y);
    cr->scale(scale, scale);

    // Set the desired font face
    cr->set_font_face(cairo_face_);
    cr->set_font_size(font_size);
    Cairo::FontExtents extents;
    cr->get_font_extents(extents);

    // Print sample text in black
    cr->move_to(10.0, 10.0 + extents.height);
    cr->set_source_rgb(0, 0, 0);
    cr->show_text(sample_text);

    // Print horizontal reference mark in yellow
    cr->set_source_rgb(1.0, 1.0, 0.0);
    cr->rectangle(0, 0, 100, 10);
    cr->fill();
    cr->set_source_rgb(0.0, 0.0, 0.0);
    cr->rectangle(99, 0, 1, 10);
    cr->fill();

    // Print vertical reference mark in red
    cr->set_source_rgb(1.0, 0.0, 0.0);
    cr->rectangle(0, 0, 10, 100 * printable_area.height / printable_area.width);
    cr->fill();
    cr->set_source_rgb(0.0, 0.0, 0.0);
    cr->rectangle(0, 100 * printable_area.height / printable_area.width - 1, 10,
                  1);
    cr->fill();
}

static void set_surface_metadata(const Cairo::RefPtr<Cairo::Context>& cr,
                                 const std::string& title) {
    std::string author = GetAuthor();
    Cairo::RefPtr<Cairo::Surface> surface = cr->get_target();
    cairo_surface_t* c_surface = static_cast<cairo_surface_t*>(surface->cobj());

    Cairo::RefPtr<Cairo::PdfSurface> pdf_surface =
        Cairo::RefPtr<Cairo::PdfSurface>::cast_dynamic(surface);
    if (pdf_surface) {
        cairo_pdf_surface_set_metadata(c_surface, CAIRO_PDF_METADATA_TITLE,
                                       title.c_str());
        cairo_pdf_surface_set_metadata(c_surface, CAIRO_PDF_METADATA_AUTHOR,
                                       author.c_str());
        cairo_pdf_surface_set_metadata(c_surface, CAIRO_PDF_METADATA_CREATOR,
                                       "FontForge");
    }
    Cairo::RefPtr<Cairo::PsSurface> ps_surface =
        Cairo::RefPtr<Cairo::PsSurface>::cast_dynamic(surface);
    if (ps_surface) {
        cairo_ps_surface_dsc_comment(c_surface, ("%%Title: " + title).c_str());
        cairo_ps_surface_dsc_comment(c_surface, "%%Creator: FontForge");
        cairo_ps_surface_dsc_comment(c_surface, ("%%For: " + author).c_str());
    }
}

static void draw_centered_text(const Cairo::RefPtr<Cairo::Context>& cr,
                               const Cairo::Rectangle& box,
                               const std::string& text) {
    Cairo::FontExtents extents;
    cr->get_font_extents(extents);

    Cairo::TextExtents text_extents;
    cr->get_text_extents(text, text_extents);

    // The text is aligned vertically so that its ascent and descent are
    // together centered around the box horizontal middle line. This ensures
    // that for multiple aligned boxes the text in them would also be aligned.
    cr->move_to(box.x + (box.width - text_extents.width) / 2,
                box.y + (box.height + extents.ascent) / 2);
    cr->show_text(text);
}

static void draw_centered_glyph(const Cairo::RefPtr<Cairo::Context>& cr,
                                const Cairo::Rectangle& box,
                                unsigned long glyph_index) {
    Cairo::FontExtents extents;
    cr->get_font_extents(extents);

    Cairo::Glyph glyph{glyph_index, 0.0, 0.0};
    Cairo::TextExtents text_extents;
    cr->get_glyph_extents({glyph}, text_extents);

    // The text is aligned vertically so that its ascent and descent are
    // together centered around the box horizontal middle line. This ensures
    // that for multiple aligned boxes the text in them would also be aligned.
    glyph.x = box.x + (box.width - text_extents.width) / 2;
    glyph.y = box.y + (box.height + extents.ascent) / 2;
    cr->show_glyphs({glyph});
}

// Returns vector of glyph lines. Each line has a prefix label, e.g. "05D0", and
// a list of codepoints. All the index lists must have the same size, which
// corresponds to the number of slots per line. An index can be -1, which means
// no glyph should be drawn at that slot.
struct GlyphLine {
    std::string label;
    bool encoded;
    // Unicode codepoints or TTF glyph indexes, according to the value of the
    // "encoded" flag
    std::vector<int> indexes;
};

std::vector<GlyphLine> split_to_lines(const PrintGlyphMap& print_map,
                                      size_t max_slots) {
    std::set<int> encoded_glyphs;
    std::vector<int> unencoded_glyphs;
    for (const auto& glyph_item : print_map) {
        if (glyph_item.second->unicodeenc == -1) {
            unencoded_glyphs.push_back(glyph_item.first);
        } else {
            encoded_glyphs.insert(glyph_item.second->unicodeenc);
        }
    }

    size_t line_length = 0;
    if (encoded_glyphs.empty()) {
        line_length = (max_slots >= 20)   ? 20
                      : (max_slots >= 10) ? 10
                      : (max_slots >= 5)  ? 5
                      : (max_slots >= 2)  ? 2
                                          : 1;
    } else {
        // We want 2^n slots in a line, for the simplicity of hexadecimal
        // representation.
        line_length = (max_slots >= 16)  ? 16
                      : (max_slots >= 8) ? 8
                      : (max_slots >= 4) ? 4
                      : (max_slots >= 2) ? 2
                                         : 1;
    }

    // Group encoded glyphs into lines.
    std::map<int, std::vector<int>> cp_lines;
    for (int codepoint : encoded_glyphs) {
        // round to a multiple of line_length
        int prefix = codepoint / line_length * line_length;

        if (!cp_lines.count(prefix)) {
            cp_lines[prefix] = {codepoint};
        } else {
            cp_lines[prefix].push_back(codepoint);
        }
    }

    // Convert each line of codepoints into glyph line
    std::vector<GlyphLine> glyph_lines;
    for (const auto& cp_line : cp_lines) {
        // Format prefix into 4-digit hex label
        char hex_label[5] = "\0";
        sprintf(hex_label, "%04X", cp_line.first);

        // Pad vector of codepoints with missing slots
        std::vector<int> slots(line_length, -1);
        for (int cp : cp_line.second) {
            slots[cp % line_length] = cp;
        }

        glyph_lines.emplace_back(GlyphLine{hex_label, true, slots});
    }

    // Add unencoded glyphs as a running sequence
    for (size_t i = 0; i <= (unencoded_glyphs.size() - 1) / line_length; ++i) {
        auto range_begin = unencoded_glyphs.begin() + i * line_length;
        auto range_end =
            std::min(unencoded_glyphs.end(),
                     unencoded_glyphs.begin() + (i + 1) * line_length);

        glyph_lines.emplace_back(
            GlyphLine{std::to_string(i * 10), false,
                      std::vector<int>(range_begin, range_end)});
    }

    return glyph_lines;
}

// Rewritten PIFontDisplay()
void CairoPainter::draw_page_full_display(
    const Cairo::RefPtr<Cairo::Context>& cr, double scale,
    const Cairo::Rectangle& printable_area, int page_nr, double pointsize) {
    init_document(cr, scale, printable_area, "Font Display for " + font_name_);

    Cairo::Rectangle scaled_printable_area{0, 0, printable_area.width / scale,
                                           printable_area.height / scale};

    double extravspace = pointsize / 6;
    double extrahspace = pointsize / 3;

    // All dimensions are in points
    double left_code_area_width = 36;
    double top_code_area_height = 12;

    double char_area_width =
        scaled_printable_area.width - margin_ * 2 - left_code_area_width;
    double char_area_height = scaled_printable_area.height - margin_ -
                              top_margin_ - top_code_area_height;
    int max_slots = static_cast<int>(
        std::floor(char_area_width / (extrahspace + pointsize)));
    int max_lines = static_cast<int>(
        std::floor(char_area_height / (extravspace + pointsize)));

    cr->set_source_rgb(0, 0, 0);

    std::vector<GlyphLine> glyph_lines = split_to_lines(print_map_, max_slots);
    size_t line_length =
        glyph_lines.empty() ? 16 : glyph_lines[0].indexes.size();

    static const std::array<std::string, 16> slot_labels = {
        "0", "1", "2", "3", "4", "5", "6", "7",
        "8", "9", "A", "B", "C", "D", "E", "F"};
    for (size_t i = 0; i < line_length; ++i) {
        Cairo::Rectangle slot{margin_ + left_code_area_width + extrahspace +
                                  i * (extrahspace + pointsize),
                              top_margin_, pointsize, top_code_area_height};
        draw_centered_text(cr, slot, slot_labels[i]);
    }

    double y_start = top_margin_ + top_code_area_height + extravspace;

    for (size_t i = 0; i < max_lines; ++i) {
        if (i >= glyph_lines.size()) {
            break;
        }
        const GlyphLine& glyph_line = glyph_lines[i];

        // Draw a ruler between encoded and unencoded glyphs, if necessary.
        if (i > 0 && !glyph_line.encoded && glyph_lines[i - 1].encoded) {
            cr->move_to(margin_ + left_code_area_width + extrahspace, y_start);
            cr->line_to(margin_ + left_code_area_width +
                            line_length * (extrahspace + pointsize),
                        y_start);
            cr->stroke();

            // This provides a vertical shift after we have drawn a ruler
            // between encoded and unencoded glyphs.
            y_start += extravspace;
        }

        draw_line_full_display(cr, glyph_line, y_start, left_code_area_width,
                               pointsize);

        y_start += (extravspace + pointsize);
    }
}

void CairoPainter::draw_line_full_display(
    const Cairo::RefPtr<Cairo::Context>& cr, const GlyphLine& glyph_line,
    double y_start, double left_code_area_width, double pointsize) {
    double extrahspace = pointsize / 3;
    Cairo::Rectangle slot{margin_, y_start, left_code_area_width, pointsize};

    // Draw line label
    cr->select_font_face("times", Cairo::FONT_SLANT_NORMAL,
                         Cairo::FONT_WEIGHT_BOLD);
    cr->set_font_size(12.0);
    draw_centered_text(cr, slot, glyph_line.label);

    // Set the user font face
    cr->set_font_face(cairo_face_);
    cr->set_font_size(pointsize);

    for (size_t j = 0; j < glyph_line.indexes.size(); ++j) {
        int codepoint = glyph_line.indexes[j];
        if (codepoint == -1) {
            continue;
        }

        unichar_t glyph_unistr[2] = {0, 0};
        glyph_unistr[0] = (unichar_t)codepoint;
        char* glyph_utf8 = u2utf8_copy(glyph_unistr);

        // Print sample glyph
        Cairo::Rectangle glyph_slot{margin_ + left_code_area_width +
                                        extrahspace +
                                        j * (extrahspace + pointsize),
                                    y_start, pointsize, pointsize};
        if (glyph_line.encoded) {
            draw_centered_text(cr, glyph_slot, glyph_utf8);
        } else {
            draw_centered_glyph(cr, glyph_slot, codepoint);
        }
    }
}

void CairoPainter::draw_page_sample_text(
    const Cairo::RefPtr<Cairo::Context>& cr, double scale,
    const Cairo::Rectangle& printable_area, int page_nr,
    const std::string& sample_text) {
    init_document(cr, scale, printable_area, "Sample Text from " + font_name_);

    Cairo::Rectangle scaled_printable_area{0, 0, printable_area.width / scale,
                                           printable_area.height / scale};

    // Set the desired font face
    cr->set_font_face(cairo_face_);
    cr->set_font_size(20);
    Cairo::FontExtents extents;
    cr->get_font_extents(extents);

    // Print sample text in black
    cr->set_source_rgb(0, 0, 0);

    std::istringstream stream(sample_text);
    ParsedRichText parsed_text = parse_xml_stream(stream);
    build_style_map(parsed_text);

    double y_start = top_margin_;

    // Buffer of text blocks for a single line of output
    LineBuffer line_buffer;
    double line_buffer_width = 0;

    for (const auto& [current_tags, text] : parsed_text) {
        cr->set_font_face(select_face(current_tags));

        // Iterator inside the currently processed block, which can be broken at
        // word boundaries.
        std::string::const_iterator space_it = text.begin();

        // Iterator which keeps the beginning of the current block, which can
        // differ from text.begin() if a part of text has already fiiled a line
        // and went to output.
        std::string::const_iterator subblock_start = text.begin();

        // The end of the largest printable subblock, which can be printed
        // without overflow. To prevent infinite loop, we must output at least
        // some text on each line.
        std::string::const_iterator subblock_break = text.begin();

        do {
            space_it =
                std::find_if(++space_it, text.end(),
                             [](unsigned char c) { return std::isspace(c); });
            std::string subblock(subblock_start, space_it);

            Cairo::TextExtents block_extents;
            cr->get_text_extents(subblock, block_extents);

            // When to continue filling the current line buffer:
            //  * The buffer doesn't end with user linebreak and...
            //  * The line buffer is empty - we always want to output something,
            //    so the first subblock always goes into empty buffer, even if
            //    it's too long.
            //  * The new subblock is short enough, so it still fits the page
            //    width together with the buffer contents.
            bool continue_filling_buffer =
                (*subblock_break != '\n') &&
                ((line_buffer.empty() && subblock_start == subblock_break) ||
                 (line_buffer_width + block_extents.width) <
                     scaled_printable_area.width);

            if (continue_filling_buffer) {
                subblock_break = space_it;
                continue;
            } else {
                // This subblock exceeds the page width, we should output the
                // current buffer and start a new line
                std::string printable_subblock(subblock_start, subblock_break);
                line_buffer.emplace_back(printable_subblock,
                                         select_face(current_tags));

                y_start += draw_line_sample_text(cr, line_buffer, y_start);
                line_buffer.clear();
                line_buffer_width = 0;

                // The line break consumes all the whitespace that was at the
                // breaking position
                subblock_break = std::find_if_not(
                    ++subblock_break, text.end(), [](unsigned char c) {
                        return std::isspace(c) && c != '\n';
                    });
                subblock_start = space_it = subblock_break;
            }
        } while (space_it != text.end());

        std::string printable_subblock(subblock_start, text.end());
        Cairo::TextExtents block_extents;
        cr->get_text_extents(printable_subblock, block_extents);

        line_buffer.emplace_back(std::string(subblock_start, text.end()),
                                 select_face(current_tags));
        line_buffer_width += block_extents.x_advance;
    }
    draw_line_sample_text(cr, line_buffer, y_start);
}

double CairoPainter::draw_line_sample_text(
    const Cairo::RefPtr<Cairo::Context>& cr, const LineBuffer& line_buffer,
    double y_start) {
    // Determine the line height
    double height = 0;
    for (const auto& [text, face] : line_buffer) {
        Cairo::FontExtents font_extents;
        cr->set_font_face(face);
        cr->get_font_extents(font_extents);
        height = std::max(height, font_extents.height);
    }

    // Perform the actual text drawing
    double x = 0, y = y_start + height;
    for (const auto& [text, face] : line_buffer) {
        Cairo::TextExtents text_extents;
        cr->set_font_face(face);
        cr->get_text_extents(text, text_extents);

        cr->move_to(x, y);
        cr->show_text(text);
        x += text_extents.x_advance;
    }

    return height;
}

void CairoPainter::build_style_map(const ParsedRichText&) {
    style_map_[{false, false}] = cairo_face_;

    for (const auto& [modifiers, ref_face] : cairo_family_) {
        if (modifiers.italic) {
            if (modifiers.os2_weight > 500) {
                style_map_[{true, true}] = ref_face;
            } else {
                style_map_[{false, true}] = ref_face;
            }
        } else if (modifiers.os2_weight > 500) {
            style_map_[{true, false}] = ref_face;
        }
    }
}

Cairo::RefPtr<Cairo::FtFontFace> CairoPainter::select_face(
    const std::vector<std::string>& tags) const {
    bool has_bold = std::count(tags.begin(), tags.end(), "bold");
    bool has_italic = std::count(tags.begin(), tags.end(), "italic");

    auto face_it = style_map_.find({has_bold, has_italic});
    if (face_it != style_map_.end()) {
        return face_it->second;
    } else {
        return cairo_face_;
    }
}

void CairoPainter::init_document(const Cairo::RefPtr<Cairo::Context>& cr,
                                 double scale,
                                 const Cairo::Rectangle& printable_area,
                                 const std::string& document_title) {
    set_surface_metadata(cr, document_title);

    // To ensure faithful preview, the rendering must be identical on all
    // devices and all resolutions. This requires disabling of font metrics
    // rounding.
    Cairo::FontOptions font_options;
    font_options.set_hint_metrics(Cairo::HintMetrics::HINT_METRICS_OFF);
    cr->set_font_options(font_options);

    cr->translate(printable_area.x, printable_area.y);
    cr->scale(scale, scale);

    // White background
    cr->set_source_rgb(1, 1, 1);
    cr->paint();

    // Set title
    cr->set_source_rgb(0, 0, 0);
    cr->select_font_face("times", Cairo::FONT_SLANT_NORMAL,
                         Cairo::FONT_WEIGHT_BOLD);
    cr->set_font_size(12.0);
    draw_centered_text(cr, {0, 0, printable_area.width / scale, top_margin_},
                       document_title);
}

Cairo::RefPtr<Cairo::FtFontFace> create_cairo_face(SplineFont* sf) {
    FTC* ftc = (FTC*)_FreeTypeFontContext(sf, NULL, NULL, ly_fore, ff_ttf,
                                          ttf_flag_otmode, NULL);
    FT_Face face = ftc->face;

    // TODO: Correctly release face - see Cairo docs
    auto cairo_face = Cairo::FtFontFace::create(face, 0);
    return cairo_face;
}

CairoFontFamily create_cairo_family(SplineFont* current_sf) {
    SplineFont** family_sfs = FVCollectFamily(current_sf);
    SplineFontModifiers modifiers;
    Cairo::RefPtr<Cairo::FtFontFace> ft_face;

    CairoFontFamily family;

    // By convention, the first element is the default font
    SFGetProperties(current_sf, &modifiers);
    ft_face = create_cairo_face(current_sf);
    family.emplace_back(modifiers, ft_face);

    if (family_sfs) {
        for (SplineFont** sf_it = family_sfs; *sf_it != nullptr; ++sf_it) {
            SFGetProperties(*sf_it, &modifiers);
            ft_face = create_cairo_face(*sf_it);
            family.emplace_back(modifiers, ft_face);
        }
    }

    free(family_sfs);
    return family;
}

ParsedRichText parse_xml_stream(std::istream& input) {
    std::string text, tag;
    // Array of text blocks as follows: (text block, list of tags applied on
    // that block)
    ParsedRichText parsed_input;
    std::vector<std::string> current_tags;

    while (std::getline(input, text, '<') && std::getline(input, tag, '>')) {
        if (!text.empty()) {
            parsed_input.emplace_back(current_tags, text);
        }
        if (tag.size() > 0 && tag.front() == '/') {
            if (!current_tags.empty() &&
                (tag.compare(1, std::string::npos, current_tags.back()) == 0)) {
                current_tags.pop_back();
            } else {
                std::cerr << "Rich text XML parser failed at tag \"" << tag
                          << "\", position " << input.tellg() << std::endl;
                break;
            }
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

}  // namespace ff::utils
