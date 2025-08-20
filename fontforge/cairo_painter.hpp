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

#include <map>
#include <cairomm/context.h>

typedef struct splinechar SplineChar;

using PrintGlyphMap = std::map<int, SplineChar*>;

namespace ff::utils {

struct GlyphLine;

class CairoPainter {
 public:
    CairoPainter(Cairo::RefPtr<Cairo::FtFontFace> cairo_face,
                 const PrintGlyphMap& print_map, const std::string& font_name)
        : cairo_face_(cairo_face),
          print_map_(print_map),
          font_name_(font_name) {}

    // Draw the entire printable page.
    void draw_page(const Cairo::RefPtr<Cairo::Context>& cr, double scale,
                   const Cairo::Rectangle& printable_area, int page_nr,
                   const std::string& sample_text, double font_size);

    // Draw full font display as a character grid.
    void draw_page_full_display(const Cairo::RefPtr<Cairo::Context>& cr,
                                double scale,
                                const Cairo::Rectangle& printable_area,
                                int page_nr, double pointsize);

    // Draw formatted sample text.
    void draw_page_sample_text(const Cairo::RefPtr<Cairo::Context>& cr,
                               double scale,
                               const Cairo::Rectangle& printable_area,
                               int page_nr, const std::string& sample_text);

 private:
    Cairo::RefPtr<Cairo::FtFontFace> cairo_face_;

    PrintGlyphMap print_map_;

    std::string font_name_;

    // All dimensions are in points
    double margin_ = 36;
    double top_margin_ = 96;

    void init_document(const Cairo::RefPtr<Cairo::Context>& cr, double scale,
                       const Cairo::Rectangle& printable_area,
                       const std::string& document_title);

    void draw_line_full_display(const Cairo::RefPtr<Cairo::Context>& cr,
                                const GlyphLine& glyph_line, double y_start,
                                double left_code_area_width, double pointsize);
};

}  // namespace ff::utils
