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
#include <string>

#include <gtkmm.h>

namespace ff::app {

Glib::RefPtr<Gtk::Application> GtkApp();

void load_legacy_style();

using ColorMap = std::map<
    std::string /*fontforge color name*/,
    std::pair<std::string /*GTK theme color name*/, Gdk::RGBA /*fallback*/>>;

class ColorManager {
 public:
    static ColorManager& instance();

    Glib::RefPtr<Gtk::StyleContext> style_context() const { return style_ctx_; }

    // Register custom colors for the dialog, which can be used in CSS. The
    // colors are inherited from the parent window.
    void register_colors(const ColorMap& colors);

    // Helper utility to set a registered or standard CSS color in Cairo.
    void set_color_in_context(const Cairo::RefPtr<Cairo::Context>& cr,
                              const std::string& color_name) const;

 private:
    ColorManager();

    Glib::RefPtr<Gtk::StyleContext> style_ctx_;
    Glib::RefPtr<Gtk::CssProvider> custom_color_provider_;
};

}  // namespace ff::app
