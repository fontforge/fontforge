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
#include <sstream>
#include <regex>
#include <fontconfig/fontconfig.h>

#include "gresource.h"
#include "css_builder.hpp"

extern GResInfo gdraw_ri;
extern "C" void GResEditDoInit(GResInfo* ri);

namespace ff::app {

Glib::RefPtr<Gtk::Application> GtkApp() {
    // Unique instance mode doesn't work well as long as the startup sequence is
    // handled in the legacy code. It would be possible to enable it once the
    // application main loop is started with Gtk::Application::run().
    Gio::ApplicationFlags app_flags = Gio::APPLICATION_NON_UNIQUE;

    static auto app = Gtk::Application::create("org.fontforge", app_flags);
    static bool initialized = false;

    if (!initialized) {
        if (app) {
            // TODO(iorsh): Registration call sometimes freezes or crashes in
            // Windows, we shall enable it after the GTK code takes over the
            // main loop.
            //
            // app->register_application();
        } else {
            std::cerr << "Error: Failed to create Gtk::Application instance"
                      << std::endl;
        }
        load_legacy_style();
        register_stretched_ui_fonts();

        initialized = true;
    }

    return app;
}

void load_legacy_style() {
    static Glib::RefPtr<Gtk::CssProvider> css_provider =
        Gtk::CssProvider::create();
    if (!css_provider)
        std::cerr << "Error: Failed to create CSS provider" << std::endl;
    static bool initialized = false;

    if (!initialized) {
        // Add CSS provider to the screen, so that it applies to all windows.
        auto screen = Gdk::Screen::get_default();
        if (!screen)
            std::cerr << "Error: Failed to retrieve GDK screen" << std::endl;

        // User-defined CSS should usually go with USER priority, but we reduce
        // it by 2 so that widget-specific customizations and the GTK inspector,
        // which also applies USER, would have priority over it.
        if (screen && css_provider) {
            Gtk::StyleContext::add_provider_for_screen(
                screen, css_provider, GTK_STYLE_PROVIDER_PRIORITY_USER - 2);
        }

        initialized = true;
    }

    for (GResInfo* re = &gdraw_ri; re != NULL; re = re->next)
        GResEditDoInit(re);
    std::string style = build_styles(&gdraw_ri);

    // Load CSS styles
    try {
        css_provider && css_provider->load_from_data(style);
    } catch (const Glib::Error& ex) {
        std::cerr << "Failed CSS data: " << std::endl << style << std::endl;
        std::cerr << "Error loading CSS: " << ex.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown error occurred while loading CSS." << std::endl;
    }
}

ColorManager::ColorManager() {
    style_ctx_ = Gtk::StyleContext::create();

    Gtk::WidgetPath path;
    path.path_append_type(GTK_TYPE_WINDOW);
    style_ctx_->set_path(path);
    style_ctx_->set_screen(Gdk::Screen::get_default());

    custom_color_provider_ = Gtk::CssProvider::create();
    style_ctx_->add_provider(custom_color_provider_,
                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

ColorManager& ColorManager::instance() {
    static ColorManager manager;
    return manager;
}

void ColorManager::register_colors(const ColorMap& colors) {
    std::ostringstream css;
    for (const auto& [ff_color_name, theme_color_pair] : colors) {
        const std::string& theme_color_name = theme_color_pair.first;
        Gdk::RGBA resolved;
        if (!style_ctx_->lookup_color(theme_color_name, resolved)) {
            // Fallback to hardcoded color if the theme color is not found.
            resolved = theme_color_pair.second;
            std::cerr << "In the declaration of \"" << ff_color_name
                      << "\": GTK theme color " << theme_color_name
                      << " not found, using fallback." << std::endl;
        }
        css << "@define-color " << ff_color_name << " " << resolved.to_string()
            << ";\n";
    }

    custom_color_provider_->load_from_data(css.str());
}

void ColorManager::set_color_in_context(const Cairo::RefPtr<Cairo::Context>& cr,
                                        const std::string& color_name) const {
    Gdk::RGBA color;
    // Fallback to Pantone 448 C, the ugliest color in the world.
    color.set_rgba_u(0x4A00, 0x4100, 0x2A00, 0xFFFF);

    bool result = style_ctx_->lookup_color(color_name, color);
    if (!result) {
        std::cerr << "CSS color " << color_name
                  << " not registered, using fallback." << std::endl;
    }

    cr->set_source_rgba(color.get_red(), color.get_green(), color.get_blue(),
                        color.get_alpha());
}

void register_stretched_ui_fonts() {
    FcConfig* current_config = FcConfigGetCurrent();
    std::string match = "<fontconfig>";
    std::regex re_name("WIDTH_NAME");
    std::regex re_value("WIDTH_VALUE");

    std::string match_template =
        "<match target=\"pattern\">                                "
        "  <test name=\"width\" compare=\"eq\">                    "
        "    <const>WIDTH_NAME</const>                             "
        "  </test>                                                 "
        "  <edit name=\"matrix\" mode=\"assign\" binding=\"same\"> "
        "    <matrix>                                              "
        "      <double>WIDTH_VALUE</double>                        "
        "      <double>0.0</double>                                "
        "      <double>0.0</double>                                "
        "      <double>1.0</double>                                "
        "    </matrix>                                             "
        "  </edit>                                                 "
        "</match>                                                  ";

    std::vector<std::pair<std::string, double>> width_values{
        {"ultracondensed", 0.5}, {"extracondensed", 0.625},
        {"condensed", 0.75},     {"semicondensed", 0.875},
        {"semiexpanded", 1.125}, {"expanded", 1.25},
        {"extraexpanded", 1.5},  {"ultraexpanded", 2.0}};
    for (auto& [name, value] : width_values) {
        std::string match_subst =
            std::regex_replace(match_template, re_name, name);
        match_subst =
            std::regex_replace(match_subst, re_value, std::to_string(value));

        match += match_subst;
    }
    match += "</fontconfig>";
    FcConfigParseAndLoadFromMemory(current_config, (FcChar8*)match.c_str(),
                                   true);
}

}  // namespace ff::app
