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

#include "application.hpp"

#include <iostream>
#include "gresource.h"
#include "css_builder.hpp"

extern GResInfo gdraw_ri;

namespace ff::app {

Glib::RefPtr<Gtk::Application> GtkApp() {
    // Unique instance mode doesn't work well as long as the startup sequence is
    // handled in the legacy code. It would be possible to enable it once the
    // application main loop is started with Gtk::Application::run().
    Gio::ApplicationFlags app_flags = Gio::APPLICATION_NON_UNIQUE;

    static auto app = Gtk::Application::create("org.fontforge", app_flags);
    static bool initialized = false;

    if (!initialized) {
        app->register_application();
        load_legacy_style();

        initialized = true;
    }

    return app;
}

void load_legacy_style() {
    static Glib::RefPtr<Gtk::CssProvider> css_provider =
        Gtk::CssProvider::create();
    static bool initialized = false;

    if (!initialized) {
        // Add CSS provider to the screen, so that it applies to all windows.
        auto screen = Gdk::Screen::get_default();

        // User-defined CSS should usually go with USER priority, but we reduce
        // it by 2 so that widget-specific customizations and the GTK inspector,
        // which also applies USER, would have priority over it.
        Gtk::StyleContext::add_provider_for_screen(
            screen, css_provider, GTK_STYLE_PROVIDER_PRIORITY_USER - 2);

        initialized = true;
    }

    std::string style = build_styles(&gdraw_ri);

    // Load CSS styles
    try {
        css_provider->load_from_data(style);
    } catch (const Glib::Error& ex) {
        std::cerr << "Failed CSS data: " << std::endl << style << std::endl;
        std::cerr << "Error loading CSS: " << ex.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown error occurred while loading CSS." << std::endl;
    }
}

}  // namespace ff::app
