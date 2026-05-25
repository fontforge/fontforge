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
extern "C" void GResEditDoInit(GResInfo* ri);

namespace ff::app {

Glib::RefPtr<Gtk::Application> GtkApp() {
    std::cout << "Running GtkApp()" << std::endl;
    // Unique instance mode doesn't work well as long as the startup sequence is
    // handled in the legacy code. It would be possible to enable it once the
    // application main loop is started with Gtk::Application::run().
    Gio::ApplicationFlags app_flags = Gio::APPLICATION_NON_UNIQUE;

    static auto app = Gtk::Application::create("org.fontforge", app_flags);
    static bool initialized = false;

    std::cout << "Initializing GtkApp()" << std::endl;
    if (!initialized) {
        if (app) {
            std::cout << "Registering application" << std::endl;
            app->register_application();
            std::cout << "Application registration complete" << std::endl;
        } else {
            std::cout << "Application failure: Gtk::Application::create()) "
                         "returned NULL"
                      << std::endl;
        }
        std::cout << "Loading legacy style" << std::endl;
        load_legacy_style();
        std::cout << "Legacy style loaded" << std::endl;

        initialized = true;
    }
    std::cout << "GtkApp() initialization done" << std::endl;

    return app;
}

void load_legacy_style() {
    std::cout << "Creating CSS provider" << std::endl;
    static Glib::RefPtr<Gtk::CssProvider> css_provider =
        Gtk::CssProvider::create();
    if (css_provider)
        std::cout << "CSS provider created" << std::endl;
    else
        std::cout << "CSS provider creation failed" << std::endl;
    static bool initialized = false;

    if (!initialized) {
        // Add CSS provider to the screen, so that it applies to all windows.
        std::cout << "Retrieving GDK screen" << std::endl;
        auto screen = Gdk::Screen::get_default();
        if (screen)
            std::cout << "GDK screen retrieved" << std::endl;
        else
            std::cout << "GDK screen retrieval failed" << std::endl;

        // User-defined CSS should usually go with USER priority, but we reduce
        // it by 2 so that widget-specific customizations and the GTK inspector,
        // which also applies USER, would have priority over it.
        if (screen && css_provider) {
            std::cout << "Adding CSS provider" << std::endl;
            Gtk::StyleContext::add_provider_for_screen(
                screen, css_provider, GTK_STYLE_PROVIDER_PRIORITY_USER - 2);
            std::cout << "CSS provider added" << std::endl;
        }

        initialized = true;
    }

    std::cout << "Iniializing GResInfo" << std::endl;
    for (GResInfo* re = &gdraw_ri; re != NULL; re = re->next)
        GResEditDoInit(re);
    std::cout << "Building styles" << std::endl;
    std::string style = build_styles(&gdraw_ri);
    std::cerr << "CSS data: " << std::endl << style << std::endl;

    // Load CSS styles
    try {
        std::cout << "Loading CSS" << std::endl;
        css_provider->load_from_data(style);
        std::cout << "CSS loaded" << std::endl;
    } catch (const Glib::Error& ex) {
        std::cerr << "Failed CSS data: " << std::endl << style << std::endl;
        std::cerr << "Error loading CSS: " << ex.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown error occurred while loading CSS." << std::endl;
    }
}

}  // namespace ff::app
