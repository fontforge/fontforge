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

#include <iostream>
#include <map>
#include <memory>
#include <cairomm/context.h>

#include "layout_shim.hpp"
#include "i_printer.hpp"
#include "i_shaper.hpp"
#include "shaper_shim.hpp"

typedef struct splinechar SplineChar;
typedef struct fontviewbase FontViewBase;

using ff::layout::SplineFontProperties;
using ParsedTag =
    std::pair<std::string /*tag name*/, std::string /*tag value*/>;

namespace ff::utils {

using PrintGlyphMap = std::map<int, SplineChar*>;

struct CairoFontRec {
    SplineFontProperties props;
    Cairo::RefPtr<Cairo::FtFontFace> face;
    std::shared_ptr<shapers::IShaper> shaper;
    SplineFont* sf;
};

// Several fonts comprising a family. By convention, the first element is the
// default font (it doesn't need to be the regular face). The default font is
// used when no modifiers are specified.
using CairoFontFamily = std::vector<CairoFontRec>;

using ParsedRichText =
    std::vector<std::pair<std::vector<std::string>, std::string>>;

// size_t "font_idx" is an index into CairoPainter::cairo_family_.
using RichTextLineBuffer =
    std::vector<std::tuple<std::string, size_t /*font_idx*/, double /*size*/>>;
using RichTextLayout =
    std::vector<std::pair<RichTextLineBuffer, double /*height*/>>;
using PrintGlyphVec = std::vector<std::pair<int, SplineChar*>>;

class CairoPainter;

class CairoContext : public ff::layout::PageContext {
 public:
    CairoContext(const Cairo::RefPtr<Cairo::Context>& cr,
                 const Cairo::Rectangle& printable_area)
        : cr_(cr), printable_area_(printable_area) {}

    Cairo::RefPtr<Cairo::Context> cr_;
    Cairo::Rectangle printable_area_;
};

class FullGlyphPrinter : public ff::layout::IPrinter {
 public:
    FullGlyphPrinter(const PrintGlyphVec& print_map,
                     const CairoFontRec& font_rec,
                     const std::string& scaling_option)
        : print_map_(print_map),
          font_rec_(font_rec),
          scaling_option_(scaling_option) {}

    size_t page_count() const override;
    void add_page(size_t page_number,
                  const ff::layout::PageContext* context) override;

    static const std::string kScaleToPage;
    static const std::string kScaleEmSize;
    static const std::string kScaleMaxHeight;

 private:
    // TODO(iorsh): Storing reference is dangerous, consider other non-copying
    // storage options.
    const PrintGlyphVec& print_map_;
    const CairoFontRec& font_rec_;
    std::string scaling_option_;

    // Get SplineFont ascent and descent in Cairo context units, normalized for
    // font size 1pt.
    std::pair<double, double> get_splinefont_metrics(
        const Cairo::RefPtr<Cairo::Context>& cr) const;

    std::array<double, 3> calculate_full_glyph_location(
        const std::string& scaling_option,
        const Cairo::RefPtr<Cairo::Context>& cr,
        const Cairo::Rectangle& shifted_printable_area,
        const Cairo::TextExtents& text_extents) const;
};

class FullDisplayPrinter : public ff::layout::IPrinter {
 public:
    FullDisplayPrinter(const Cairo::Rectangle& printable_area,
                       const PrintGlyphVec& print_map,
                       const CairoFontRec& font_rec, double pointsize);

    size_t page_count() const override;
    void add_page(size_t page_number,
                  const ff::layout::PageContext* context) override;

 private:
    const double left_code_area_width_ = 36;
    const double top_code_area_height_ = 12;
    double extravspace_;
    double extrahspace_;

    // A line of glyphs for full display. It has a prefix label, e.g. "05D0",
    // and a list of codepoints. All the index lists must have the same size,
    // which corresponds to the number of slots per line. An index can be -1,
    // which means no glyph should be drawn at that slot.
    struct GlyphLine {
        std::string label;
        bool encoded;
        // Unicode codepoints or TTF glyph indexes, according to the value of
        // the "encoded" flag
        std::vector<int> indexes;
    };

    const PrintGlyphVec& print_map_;
    const CairoFontRec& font_rec_;
    double pointsize_;

    // Full glyph display (grid), split into lines
    std::vector<GlyphLine> glyph_lines_;

    // Pagination list for glyph_lines_, with i-th entry designating the
    // first line of the i-th page.
    std::vector<size_t> glyph_line_pagination_;

    // Returns vector of glyph lines.
    std::vector<GlyphLine> split_to_lines(size_t max_slots) const;
    void paginate(double char_area_height, double pointsize,
                  double extravspace);
    void draw_glyphs_line(const Cairo::RefPtr<Cairo::Context>& cr,
                          const GlyphLine& glyph_line, double y_start,
                          double left_code_area_width, double pointsize) const;
};

class MultiSizePrinter : public ff::layout::IPrinter {
 public:
    MultiSizePrinter(const Cairo::Rectangle& printable_area,
                     const PrintGlyphVec& print_map,
                     const CairoFontRec& font_rec,
                     const std::vector<double>& pointsizes);

    size_t page_count() const override;
    void add_page(size_t page_number,
                  const ff::layout::PageContext* context) override;

 private:
    const PrintGlyphVec& print_map_;
    const CairoFontRec& font_rec_;
    std::vector<double> pointsizes_;

    int lines_per_page_ = 0;

    double draw_line_multisize(const Cairo::RefPtr<Cairo::Context>& cr,
                               int glyph_index, double y_start) const;
};

class SampleTextPrinter : public ff::layout::IPrinter {
 public:
    SampleTextPrinter(const Cairo::RefPtr<Cairo::Context>& cr,
                      const Cairo::Rectangle& printable_area,
                      const CairoFontFamily& cairo_family,
                      const std::string& sample_text, Tag script, Tag lang,
                      const std::map<Tag, bool>& features);

    size_t page_count() const override { return pagination_list_.size(); }
    void add_page(size_t page_number,
                  const ff::layout::PageContext* context) override;

 private:
    // TODO(iorsh): Storing reference is dangerous, consider other non-copying
    // storage options.
    // Currently active font face (for example, whose FontView invoked the Print
    // dialog).
    Cairo::RefPtr<Cairo::FtFontFace> cairo_face_;

    // All the other currently open faces from the same family.
    const CairoFontFamily& cairo_family_;

    std::string sample_text_;
    Tag script_;
    Tag lang_;
    std::map<Tag, bool> features_;
    //     const PrintGlyphVec& print_map_;
    //     const CairoFontRec& font_rec_;

    RichTextLayout full_layout_;
    // Pagination list for full_layout_, with i-th entry designating the
    // first line of the i-th page.

    // Vector of output lines with calculated height.
    std::vector<size_t> pagination_list_;

    // Calculate height of single line
    double calculate_height(const Cairo::RefPtr<Cairo::Context>& cr,
                            const RichTextLineBuffer& line_buffer);
    void calculate_layout(const Cairo::RefPtr<Cairo::Context>& cr,
                          const Cairo::Rectangle& printable_area,
                          const std::string& sample_text);
    void paginate(double layout_height);
    SplineFontProperties get_default_style(
        const ParsedRichText& rich_text) const;

    // Draw a single rich text buffer.
    void draw_line(const Cairo::RefPtr<Cairo::Context>& cr,
                   const RichTextLineBuffer& line_buffer, double width,
                   double y_baseline, Tag script, Tag lang,
                   const std::map<Tag, bool>& features);

    // Select specific face to print a text segment based on the tags which
    // apply to it. Returns an index into CairoPainter::cairo_family_.
    size_t select_face(const std::vector<ParsedTag>& parsed_tags,
                       const SplineFontProperties& default_properties) const;

    double get_size(const std::vector<std::string>& tags);
};

class CairoPainter {
 public:
    CairoPainter(SplineFont* sf, FontViewBase* fv);

    // The currently active face
    const CairoFontRec& default_rec() const { return cairo_family_[0]; }

    // Returns true if the given SplineFontProperties member has more than one
    // distinct value across the fonts in cairo_family_.
    template <typename T>
    bool family_has_multiple(T SplineFontProperties::* member) const {
        if (cairo_family_.empty()) return false;
        const T& first = cairo_family_[0].props.*member;
        for (size_t i = 1; i < cairo_family_.size(); ++i) {
            if (cairo_family_[i].props.*member != first) return true;
        }
        return false;
    }

    size_t page_count() const {
        return active_printer_ ? active_printer_->page_count() : 0;
    }
    void add_page(size_t page_number, const layout::PageContext* context) {
        if (active_printer_) {
            active_printer_->add_page(page_number, context);
        }
    }

    // Draw full font display as a character grid.
    void activate_full_display_printer(const Cairo::Rectangle& printable_area,
                                       double pointsize);

    // Draw glyphs scaled to fill the page.
    void activate_full_glyph_printer(const std::string& scaling_option);

    // Draw each glyph in multiple sizes.
    void activate_multisize_printer(const Cairo::Rectangle& printable_area,
                                    const std::vector<double>& pointsizes);

    // Draw formatted sample text.
    void activate_sample_text_printer(const Cairo::RefPtr<Cairo::Context>& cr,
                                      const Cairo::Rectangle& printable_area,
                                      const std::string& sample_text,
                                      Tag script, Tag lang,
                                      const std::map<Tag, bool>& features);

 private:
    // All the other currently open faces from the same family.
    CairoFontFamily cairo_family_;

    // Sorted with encoded glyph first (ordered by encoding), unencoded glyphs
    // second (ordered by glyph index).
    PrintGlyphVec print_map_;

    std::unique_ptr<ff::layout::IPrinter> active_printer_;

    // Cached layout data, which should be invalidated whenever the painter
    // input changes.

    void sort_glyphs(const PrintGlyphMap& print_map);
};

Cairo::RefPtr<Cairo::FtFontFace> create_cairo_face(SplineFont* sf);
CairoFontFamily create_cairo_family(SplineFont* current_sf);
PrintGlyphMap build_glyph_map(SplineFont* sf);

ParsedRichText parse_xml_stream(std::istream& input);

ParsedTag parse_tag(const std::string& complete_tag);

void setup_context(const Cairo::RefPtr<Cairo::Context>& cr);
void init_document(const Cairo::RefPtr<Cairo::Context>& cr,
                   const Cairo::Rectangle& printable_area,
                   const std::string& document_title, double top_margin);

void draw_line(const Cairo::RefPtr<Cairo::Context>& cr,
               const Cairo::Rectangle& box, double level, bool horizontal);
void draw_centered_text(const Cairo::RefPtr<Cairo::Context>& cr,
                        const Cairo::Rectangle& box, const std::string& text);
void draw_centered_glyph(const Cairo::RefPtr<Cairo::Context>& cr,
                         const Cairo::Rectangle& box,
                         unsigned long glyph_index);

}  // namespace ff::utils
