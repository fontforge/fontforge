/* Copyright 2023 Maxim Iorsh <iorsh@users.sourceforge.net>
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

namespace ff::app {

Glib::RefPtr<Gtk::Application> GtkApp() {
    // Unique instance mode doesn't work well as long as the startup sequence is
    // handled in the legacy code. It would be possible to enable it once the
    // application main loop is started with Gtk::Application::run().
    Gio::ApplicationFlags app_flags = Gio::APPLICATION_NON_UNIQUE;

    static auto app = Gtk::Application::create("org.fontforge", app_flags);
    return app;
}

void add_top_view(Gtk::Window& window, views::UiContext& context) {
    static bool initialized = false;

    if (!initialized) {
        GtkApp()->register_application();

        auto theme = Gtk::IconTheme::get_default();
        std::string pixmap_dir = context.get_pixmap_dir();
        theme->prepend_search_path(pixmap_dir);

        initialized = true;
    }

    GtkApp()->add_window(window);
}

void remove_top_view(Gtk::Window& window) {
    GtkApp()->remove_window(window);
    GtkApp()->quit();
}

}  // namespace ff::app