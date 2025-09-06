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
#include <sstream>

extern "C" {
#include "fffreetype.h"
#include "gutils.h"
#include "splinechar.h"
#include "ustring.h"

extern SplineFont** FVCollectFamily(SplineFont* sf);
}

namespace ff::utils {

const std::string CairoPainter::kScaleToPage = "scale_to_page";
const std::string CairoPainter::kScaleEmSize = "scale_to_em_size";
const std::string CairoPainter::kScaleMaxHeight = "scale_to_max_height";

CairoPainter::CairoPainter(const CairoFontFamily& cairo_family,
                           const PrintGlyphMap& print_map,
                           const std::string& font_name)
    : cairo_face_(cairo_family[0].second),
      cairo_family_(cairo_family),
      font_name_(font_name) {
    sort_glyphs(print_map);
}

void CairoPainter::sort_glyphs(const PrintGlyphMap& print_map) {
    std::copy(print_map.begin(), print_map.end(), back_inserter(print_map_));
    // Sort encoded glyphs first, unenecoded glyphs second.
    std::sort(
        print_map_.begin(), print_map_.end(), [](const auto& a, const auto& b) {
            return (a.second->unicodeenc == -1)
                       ? ((b.second->unicodeenc == -1) ? (a.first < b.first)
                                                       : false)
                       : ((b.second->unicodeenc == -1)
                              ? true
                              : (a.second->unicodeenc < b.second->unicodeenc));
        });
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

// Draw a line across the box, at the specified horizontal or vertical position.
// TODO(iorsh): Fix draw_line() to draw a pixel-wide line on screen or bitmap,
// or a very thin line on a scalable surface (e.g. PDF, PS).
static void draw_line(const Cairo::RefPtr<Cairo::Context>& cr,
                      const Cairo::Rectangle& box, double level,
                      bool horizontal) {
    if (horizontal) {
        cr->move_to(box.x, level);
        cr->line_to(box.x + box.width, level);
    } else {
        cr->move_to(level, box.y);
        cr->line_to(level, box.y + box.height);
    }
    cr->stroke();
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

std::vector<CairoPainter::GlyphLine> CairoPainter::split_to_lines(
    size_t max_slots) const {
    std::vector<GlyphLine> glyph_lines;
    if (print_map_.empty()) {
        return glyph_lines;
    }

    size_t line_length = 0;
    bool no_encoded_glyphs = print_map_.front().second->unicodeenc == -1;

    if (no_encoded_glyphs) {
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
    PrintGlyphVec::const_iterator map_it = print_map_.begin();
    for (; map_it != print_map_.end(); ++map_it) {
        int codepoint = map_it->second->unicodeenc;
        if (codepoint == -1) {
            break;
        }

        // round to a multiple of line_length
        int prefix = codepoint / line_length * line_length;

        if (!cp_lines.count(prefix)) {
            cp_lines[prefix] = {codepoint};
        } else {
            cp_lines[prefix].push_back(codepoint);
        }
    }

    // Convert each line of codepoints into glyph line
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

    // All glyphs are encoded, nothing more to do.
    if (map_it == print_map_.end()) {
        return glyph_lines;
    }

    // Add unencoded glyphs as a running sequence
    for (size_t i = 0; map_it != print_map_.end();) {
        auto range_end = std::min(print_map_.end(), map_it + line_length);

        // Extract glyph indexes
        std::vector<int> indexes;
        std::transform(
            map_it, range_end, std::back_inserter(indexes),
            [](const PrintGlyphVec::value_type& p) { return p.first; });
        glyph_lines.emplace_back(
            GlyphLine{std::to_string(i * line_length), false, indexes});

        map_it = range_end;
    }

    return glyph_lines;
}

// Rewritten PIFontDisplay()
void CairoPainter::draw_page_full_display(
    const Cairo::RefPtr<Cairo::Context>& cr,
    const Cairo::Rectangle& printable_area, int page_nr, double pointsize) {
    init_document(cr, printable_area, "Font Display for " + font_name_,
                  top_margin_);

    double extravspace = pointsize / 6;
    double extrahspace = pointsize / 3;

    // All dimensions are in points
    double left_code_area_width = 36;
    double top_code_area_height = 12;

    double char_area_width =
        printable_area.width - margin_ * 2 - left_code_area_width;
    double char_area_height =
        printable_area.height - margin_ - top_margin_ - top_code_area_height;
    int max_slots = static_cast<int>(
        std::floor(char_area_width / (extrahspace + pointsize)));
    int max_lines = static_cast<int>(
        std::floor(char_area_height / (extravspace + pointsize)));

    cr->set_source_rgb(0, 0, 0);

    std::vector<GlyphLine> glyph_lines = split_to_lines(max_slots);
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
            Cairo::Rectangle line_slot{
                margin_ + left_code_area_width + extrahspace, y_start,
                line_length * (extrahspace + pointsize) - extrahspace, 0};
            draw_line(cr, line_slot, y_start, true);

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

void CairoPainter::draw_page_full_glyph(const Cairo::RefPtr<Cairo::Context>& cr,
                                        const Cairo::Rectangle& printable_area,
                                        int page_nr,
                                        const std::string& scaling_option) {
    // Locate the desired glyph
    if (page_nr >= print_map_.size()) {
        return;
    }
    auto glyph_it = print_map_.begin() + page_nr;

    // Print page title for glyph
    std::string page_title(glyph_it->second->name + (" from " + font_name_));
    init_document(cr, printable_area, page_title, full_glyph_top_margin_);

    // We are already in point units. Further rescale surface, exclude the top
    // 12 pt for glyph title
    cr->translate(0, full_glyph_top_margin_);
    Cairo::Rectangle shifted_printable_area{
        0, 0, printable_area.width,
        printable_area.height - full_glyph_top_margin_};

    // Retrieve the font metrics in normalized size
    auto [sf_ascent, sf_descent] = get_splinefont_metrics(cr);

    // Glyph metrics in normalized size
    static const double normalized_size = 1.0;
    cr->set_font_face(cairo_face_);
    cr->set_font_size(normalized_size);

    Cairo::Glyph glyph{(unsigned long)glyph_it->first, 0.0, 0.0};
    Cairo::TextExtents text_extents;
    cr->get_glyph_extents({glyph}, text_extents);

    double x_min = std::min(0.0, text_extents.x_bearing);
    double x_max = std::max(text_extents.x_advance,
                            text_extents.width + text_extents.x_bearing);
    double y_min =
        std::min(sf_descent, -text_extents.y_bearing - text_extents.height);
    double y_max = std::max(sf_ascent, -text_extents.y_bearing);

    double x_scale =
        (x_max > x_min) ? shifted_printable_area.width / (x_max - x_min) : 1e-5;
    double y_scale = (y_max > y_min)
                         ? shifted_printable_area.height / (y_max - y_min)
                         : 1e-5;
    double glyph_scale = std::min(x_scale, y_scale);

    cr->scale(glyph_scale, glyph_scale);
    cr->translate(-x_min, shifted_printable_area.height / glyph_scale + y_min);
    Cairo::Rectangle na{shifted_printable_area.x / glyph_scale + x_min,
                        shifted_printable_area.y / glyph_scale -
                            shifted_printable_area.height / glyph_scale - y_min,
                        shifted_printable_area.width / glyph_scale,
                        shifted_printable_area.height / glyph_scale};

    double scaled_size = glyph_scale * normalized_size;
    cr->set_font_size(normalized_size);
    cr->show_glyphs({glyph});

    cr->set_line_width(0.002);

    // TODO(iorsh): extend lines by tiny dashes
    draw_line(cr, na, 0, false);
    draw_line(cr, na, text_extents.x_advance, false);
    draw_line(cr, na, 0, true);
    draw_line(cr, na, -sf_ascent, true);
    draw_line(cr, na, -sf_descent, true);
}

void CairoPainter::calculate_layout_sample_text(
    const Cairo::RefPtr<Cairo::Context>& cr,
    const Cairo::Rectangle& printable_area, const std::string& sample_text) {
    // Recalculate the layout only if the sample text has changed.
    if (sample_text == cached_sample_text_) {
        return;
    }

    cached_sample_text_ = sample_text;
    cached_full_layout_.clear();

    setup_context(cr);

    std::istringstream stream(sample_text);
    ParsedRichText parsed_text = parse_xml_stream(stream);
    build_style_map(parsed_text);

    // Buffer of text blocks for a single line of output
    RichTextLineBuffer line_buffer;
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
                     printable_area.width);

            if (continue_filling_buffer) {
                subblock_break = space_it;
                continue;
            } else {
                // This subblock exceeds the page width, we should output the
                // current buffer and start a new line
                std::string printable_subblock(subblock_start, subblock_break);
                line_buffer.emplace_back(printable_subblock,
                                         select_face(current_tags));

                double line_height =
                    calculate_height_sample_text(cr, line_buffer);
                cached_full_layout_.emplace_back(line_buffer, line_height);

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

    // Collect leftovers from the end of text sample.
    double line_height = calculate_height_sample_text(cr, line_buffer);
    cached_full_layout_.emplace_back(line_buffer, line_height);
}

void CairoPainter::paginate_sample_text(double layout_height) {
    cached_pagination_list_ = {0};
    double block_height = 0;
    for (size_t i = 0; i < cached_full_layout_.size(); ++i) {
        double line_height = cached_full_layout_[i].second;
        if ((block_height > 0) &&
            (block_height + line_height > layout_height)) {
            // This line starts a new page
            cached_pagination_list_.push_back(i);
            block_height = 0;
        }
        block_height += line_height;
    }
}

void CairoPainter::draw_page_sample_text(
    const Cairo::RefPtr<Cairo::Context>& cr,
    const Cairo::Rectangle& printable_area, int page_nr,
    const std::string& sample_text) {
    init_document(cr, printable_area, "Sample Text from " + font_name_,
                  top_margin_);

    calculate_layout_sample_text(cr, printable_area, sample_text);
    paginate_sample_text(printable_area.height - top_margin_);

    // Check page number
    page_nr = std::clamp(page_nr, 0, (int)cached_pagination_list_.size() - 1);

    // Print sample text in black
    cr->set_source_rgb(0, 0, 0);

    // TODO(iorsh): Consider the baseline instead of height
    auto start_line_it =
        cached_full_layout_.begin() + cached_pagination_list_[page_nr];
    auto end_line_it = (page_nr == cached_pagination_list_.size() - 1)
                           ? cached_full_layout_.end()
                           : cached_full_layout_.begin() +
                                 cached_pagination_list_[page_nr + 1];

    double y_start = top_margin_;
    for (auto line_it = start_line_it; line_it != end_line_it; ++line_it) {
        double height = line_it->second;
        y_start += height;
        draw_line_sample_text(cr, line_it->first, y_start);
    }
}

double CairoPainter::calculate_height_sample_text(
    const Cairo::RefPtr<Cairo::Context>& cr,
    const RichTextLineBuffer& line_buffer) {
    double height = 0;
    for (const auto& [text, face] : line_buffer) {
        Cairo::FontExtents font_extents;
        cr->set_font_face(face);
        cr->get_font_extents(font_extents);
        height = std::max(height, font_extents.height);
    }
    return height;
}

void CairoPainter::draw_line_sample_text(
    const Cairo::RefPtr<Cairo::Context>& cr,
    const RichTextLineBuffer& line_buffer, double y_baseline) {
    // Perform the actual text drawing
    double x = 0;
    for (const auto& [text, face] : line_buffer) {
        Cairo::TextExtents text_extents;
        cr->set_font_face(face);
        cr->get_text_extents(text, text_extents);

        cr->move_to(x, y_baseline);
        cr->show_text(text);
        x += text_extents.x_advance;
    }
}

// Rewritten PIMultiSize()
void CairoPainter::draw_page_multisize(const Cairo::RefPtr<Cairo::Context>& cr,
                                       const Cairo::Rectangle& printable_area,
                                       int page_nr) {
    init_document(cr, printable_area, "Sample Sizes of " + font_name_,
                  top_margin_);

    static const std::vector<double> pointsizes{
        72, 48, 36,  24, 20,  18, 16,  15, 14,  13,  12, 11, 10,
        9,  8,  7.5, 7,  6.5, 6,  5.5, 5,  4.5, 4.2, 4,  0};
    double extravspace = pointsizes[0] / 6;

    double char_area_height = printable_area.height - margin_ - top_margin_;
    int max_lines = static_cast<int>(std::floor(
        (char_area_height + extravspace) / (pointsizes[0] + extravspace)));

    // Set the user font face
    cr->set_font_face(cairo_face_);

    double y_start = top_margin_;
    for (const auto [glyph_index, sc] : print_map_) {
        y_start += draw_line_multisize(cr, pointsizes, glyph_index, y_start);
        y_start += extravspace;
    }
}

double CairoPainter::draw_line_multisize(
    const Cairo::RefPtr<Cairo::Context>& cr,
    const std::vector<double>& pointsizes, int glyph_index, double y_start) {
    // Compute line height by maximum glyph size
    double maximum_size =
        (pointsizes.empty())
            ? 1
            : *std::max_element(pointsizes.begin(), pointsizes.end());
    Cairo::FontExtents font_extents;
    cr->set_font_size(maximum_size);
    cr->get_font_extents(font_extents);
    double height = font_extents.height;

    Cairo::Glyph glyph{(unsigned long)glyph_index, 0, y_start + height};

    for (double size : pointsizes) {
        Cairo::TextExtents text_extents;
        cr->set_font_size(size);
        cr->get_glyph_extents({glyph}, text_extents);
        cr->show_glyphs({glyph});

        glyph.x += text_extents.x_advance;
    }

    return height;
}

void CairoPainter::invalidate_cached_layouts() {
    cached_sample_text_.clear();
    cached_full_layout_.clear();
    cached_pagination_list_.clear();
}

void CairoPainter::build_style_map(const ParsedRichText&) {
    style_map_[{false, false}] = cairo_face_;

    for (const auto& [sf_properties, ref_face] : cairo_family_) {
        if (sf_properties.italic) {
            if (sf_properties.os2_weight > 500) {
                style_map_[{true, true}] = ref_face;
            } else {
                style_map_[{false, true}] = ref_face;
            }
        } else if (sf_properties.os2_weight > 500) {
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

void CairoPainter::setup_context(const Cairo::RefPtr<Cairo::Context>& cr) {
    // To ensure faithful preview, the rendering must be identical on all
    // devices and all resolutions. This requires disabling of font metrics
    // rounding.
    Cairo::FontOptions font_options;
    font_options.set_hint_metrics(Cairo::HintMetrics::HINT_METRICS_OFF);
    cr->set_font_options(font_options);
}

void CairoPainter::init_document(const Cairo::RefPtr<Cairo::Context>& cr,
                                 const Cairo::Rectangle& printable_area,
                                 const std::string& document_title,
                                 double top_margin) {
    set_surface_metadata(cr, document_title);
    setup_context(cr);

    cr->translate(printable_area.x, printable_area.y);

    // White background
    cr->set_source_rgb(1, 1, 1);
    cr->paint();

    // Set title
    cr->set_source_rgb(0, 0, 0);
    cr->select_font_face("times", Cairo::FONT_SLANT_NORMAL,
                         Cairo::FONT_WEIGHT_BOLD);
    cr->set_font_size(12.0);
    draw_centered_text(cr, {0, 0, printable_area.width, top_margin},
                       document_title);
}

std::pair<double, double> CairoPainter::get_splinefont_metrics(
    const Cairo::RefPtr<Cairo::Context>& cr) const {
    // Retrieve the font and glyph metrics in normalized size 1 pt.
    static const double normalized_size = 1.0;
    cr->set_font_face(cairo_face_);
    cr->set_font_size(normalized_size);

    // Retrieve the real ascender and descender in Cairo context units
    const SplineFontProperties& sf_properties = cairo_family_[0].first;
    Cairo::FontExtents font_extents;
    cr->get_font_extents(font_extents);

    Cairo::Matrix I = Cairo::identity_matrix();
    Cairo::RefPtr<Cairo::FtScaledFont> ft_scaled =
        Cairo::FtScaledFont::create(cairo_face_, I, I);

    // Retrieve the real ascender and descender in font units (like in
    // SplineFont)
    FT_Face face = ft_scaled->lock_face();
    FT_Short ft_ascender = face->ascender;
    FT_Short ft_descender = face->descender;
    ft_scaled->unlock_face();

    // Fontforge SplineFont::ascent doesn't always become the real ascent value
    // in the font. OS/2 metrics can modify that. Convert the FontForge ascent
    // value from font units to Cairo units:
    double sf_ascent =
        sf_properties.ascent * ((double)font_extents.ascent / ft_ascender);
    double sf_descent =
        sf_properties.descent * ((double)font_extents.descent / ft_descender);

    return {sf_ascent, sf_descent};
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
    SplineFontProperties sf_properties;
    Cairo::RefPtr<Cairo::FtFontFace> ft_face;

    CairoFontFamily family;

    // By convention, the first element is the default font
    SFGetProperties(current_sf, &sf_properties);
    ft_face = create_cairo_face(current_sf);
    family.emplace_back(sf_properties, ft_face);

    if (family_sfs) {
        for (SplineFont** sf_it = family_sfs; *sf_it != nullptr; ++sf_it) {
            SFGetProperties(*sf_it, &sf_properties);
            ft_face = create_cairo_face(*sf_it);
            family.emplace_back(sf_properties, ft_face);
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
