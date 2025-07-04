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
#pragma once

#include <gtkmm.h>

#include "widgets.hpp"

namespace ff::dlg {

class PrintPreviewWidget : public Gtk::Grid {
 public:
    PrintPreviewWidget();

    static Glib::ustring label();
    static void draw_page_cb(const Glib::RefPtr<Gtk::PrintContext>& context,
                             int page_nr);

    void update(const Glib::RefPtr<Gtk::PageSetup>& setup,
                const Glib::RefPtr<Gtk::PrintSettings>& settings);

 private:
    // Build and initialize the preview area widgets
    void build_compound_preview_area();

    // Calculate page ratio, setup can be nullptr
    static double calculate_page_ratio(
        const Glib::RefPtr<Gtk::PageSetup>& setup);

    // Calculate page preview area placement inside the wrapper
    Gtk::Allocation calculate_preview_allocation(
        double page_ratio, const Gtk::Allocation& wrapper_size);

    // Resize and relocate the preview area inside the wrapper. This function
    // should be called from event handlers.
    void resize_preview_area(const Glib::RefPtr<Gtk::PageSetup>& setup,
                             const Gtk::Allocation& wrapper_size);

    // Containers for compound preview area
    widget::FixedWithBackground fixed_wrapper;
    Gtk::Box box_wrapper;
    Gtk::Overlay overlay;
    Gtk::DrawingArea preview_area;

    Glib::RefPtr<Gtk::PageSetup> current_setup_;
    Gtk::Label dummy_label;
};

}  // namespace ff::dlg
