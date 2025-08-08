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

namespace ff::utils {

void CairoPainter::draw_page(const Cairo::RefPtr<Cairo::Context>& cr,
                             double scale,
                             const Cairo::Rectangle& printable_area,
                             int page_nr, const std::string& sample_text,
                             double font_size) {
    cr->translate(printable_area.x, printable_area.y);
    cr->scale(scale, scale);

    // White background
    cr->set_source_rgb(1, 1, 1);
    cr->paint();

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

}  // namespace ff::utils
