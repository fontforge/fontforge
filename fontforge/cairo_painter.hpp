/* Copyright 2025 Maxim Iorsh <iorsh@users.sourceforge.net>
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

typedef struct splinechar SplineChar;
typedef struct splinefont SplineFont;

typedef struct spline_font_properties {
    int ascent, descent;
    bool italic;
    int16_t os2_weight;
    int16_t os2_width;
    const char* styles;
} SplineFontProperties;

extern void SFGetProperties(SplineFont* sf, SplineFontProperties* properties);

#ifdef __cplusplus
}

#include <iostream>
#include <map>
#include <cairomm/context.h>

using PrintGlyphMap = std::map<int, SplineChar*>;

// Several fonts comprising a family. By convention, the first element is the
// default font (it doesn't need to be the regular face). The default font is
// used when no modifiers are specified.
using CairoFontFamily = std::vector<
    std::pair<SplineFontProperties, Cairo::RefPtr<Cairo::FtFontFace>>>;

namespace ff::utils {

struct GlyphLine;

using ParsedRichText =
    std::vector<std::pair<std::vector<std::string>, std::string>>;
using LineBuffer =
    std::vector<std::tuple<std::string, Cairo::RefPtr<Cairo::FtFontFace>>>;

class CairoPainter {
 public:
    CairoPainter(const CairoFontFamily& cairo_family,
                 const PrintGlyphMap& print_map, const std::string& font_name)
        : cairo_face_(cairo_family[0].second),
          cairo_family_(cairo_family),
          print_map_(print_map),
          font_name_(font_name) {}

    // Draw full font display as a character grid.
    void draw_page_full_display(const Cairo::RefPtr<Cairo::Context>& cr,
                                double scale,
                                const Cairo::Rectangle& printable_area,
                                int page_nr, double pointsize);

    // Draw glyphs scaled to fill the page.
    void draw_page_full_glyph(const Cairo::RefPtr<Cairo::Context>& cr,
                              double scale,
                              const Cairo::Rectangle& printable_area,
                              int page_nr, const std::string& scaling_option);

    // Draw formatted sample text.
    void draw_page_sample_text(const Cairo::RefPtr<Cairo::Context>& cr,
                               double scale,
                               const Cairo::Rectangle& printable_area,
                               int page_nr, const std::string& sample_text);

    // Draw each glyph in multiple sizes.
    void draw_page_multisize(const Cairo::RefPtr<Cairo::Context>& cr,
                             double scale,
                             const Cairo::Rectangle& printable_area,
                             int page_nr);

    static const std::string kScaleToPage;
    static const std::string kScaleEmSize;
    static const std::string kScaleMaxHeight;

 private:
    Cairo::RefPtr<Cairo::FtFontFace> cairo_face_;
    CairoFontFamily cairo_family_;
    std::map<std::pair<bool /*bold*/, bool /*italic*/>,
             Cairo::RefPtr<Cairo::FtFontFace>>
        style_map_;

    PrintGlyphMap print_map_;

    std::string font_name_;

    // All dimensions are in points
    const double margin_ = 36;
    const double top_margin_ = 96;
    const double full_glyph_top_margin_ = 48;

    void build_style_map(const ParsedRichText& rich_text);
    Cairo::RefPtr<Cairo::FtFontFace> select_face(
        const std::vector<std::string>& tags) const;

    void init_document(const Cairo::RefPtr<Cairo::Context>& cr, double scale,
                       const Cairo::Rectangle& printable_area,
                       const std::string& document_title, double top_margin);

    // Get SplineFont ascent and descent in Cairo context units, normalized for
    // font size 1pt.
    std::pair<double, double> get_splinefont_metrics(
        const Cairo::RefPtr<Cairo::Context>& cr) const;

    void draw_line_full_display(const Cairo::RefPtr<Cairo::Context>& cr,
                                const GlyphLine& glyph_line, double y_start,
                                double left_code_area_width, double pointsize);

    // Draw a single rich text buffer and return the line height it occupied.
    double draw_line_sample_text(const Cairo::RefPtr<Cairo::Context>& cr,
                                 const LineBuffer& line_buffer, double y_start);

    double draw_line_multisize(const Cairo::RefPtr<Cairo::Context>& cr,
                               const std::vector<double>& pointsizes,
                               int glyph_index, double y_start);
};

Cairo::RefPtr<Cairo::FtFontFace> create_cairo_face(SplineFont* sf);
CairoFontFamily create_cairo_family(SplineFont* current_sf);

ParsedRichText parse_xml_stream(std::istream& input);

}  // namespace ff::utils

#endif  // __cplusplus
