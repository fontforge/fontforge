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

#include "dialog_base.hpp"

extern "C" {
#include "gutils.h"
}

#include "intl.h"
#include "gimage.h"
#include "utils.hpp"

typedef struct gevent GEvent;

namespace ff::dlg {

static bool on_help_key_press(GdkEventKey* event, const std::string& file,
                              const std::string& section) {
    if (event->keyval == GDK_KEY_F1 || event->keyval == GDK_KEY_Help) {
        help(file.c_str(), section.c_str());
        return true;
    }

    return false;
}

static GdkWindow* get_toplevel_gdk_window(GWindow gwin) {
    // Redeclare GWindow to avoid dependency on the legacy headers.
    struct ggdkwindow_local { /* :GWindow */
        // Inherit GWindow start
        void* ggc;
        void* display;
        int (*eh)(GWindow, GEvent*);
        GRect pos;
        struct ggdkwindow_local* parent;
        void* user_data;
        void* widget_data;
        GdkWindow* w;
    };

    if (gwin) {
        GdkWindow* gdk_win = ((ggdkwindow_local*)gwin)->w;
        return gdk_window_get_effective_toplevel(gdk_win);
    } else {
        return nullptr;
    }
}

void install_help_key_handler(Gtk::Widget* widget, const std::string& file,
                              const std::string& section) {
    if (file.empty() || widget == nullptr) {
        return;
    }

    widget->signal_key_press_event().connect(
        [file, section](GdkEventKey* event) {
            return on_help_key_press(event, file, section);
        },
        false);
}

DialogBase::DialogBase(GWindow parent_gwin) : parent_gwindow_(parent_gwin) {
    auto ok_button = add_button(_("_OK"), Gtk::RESPONSE_OK);
    ok_button->set_name("ok");

    auto cancel_button = add_button(_("_Cancel"), Gtk::RESPONSE_CANCEL);
    cancel_button->set_name("cancel");

    set_default_response(Gtk::RESPONSE_OK);
    set_position(Gtk::WIN_POS_CENTER);
}

DialogBase::~DialogBase() {
    if (parent_gwindow_) {
        // Unblock the parent GDraw window
        GdkWindow* parent_gdk_window = get_toplevel_gdk_window(parent_gwindow_);
        g_object_set_data(G_OBJECT(parent_gdk_window), "GTKModalBlock", NULL);
    }
}

Gtk::ResponseType DialogBase::run() {
    if (parent_gwindow_) {
        GdkWindow* parent_gdk_window = get_toplevel_gdk_window(parent_gwindow_);
        GdkWindow* this_window = get_window().get()->gobj();
        gdk_window_set_transient_for(this_window, parent_gdk_window);

        // Mark the parent window as blocked, this would cause GDraw to discard
        // its events.
        g_object_set_data(G_OBJECT(parent_gdk_window), "GTKModalBlock",
                          (gpointer) true);
    }

    return (Gtk::ResponseType)Gtk::Dialog::run();
}

void DialogBase::set_help_context(const std::string& file,
                                  const std::string& section) {
    install_help_key_handler(this, file, section);
}

void DialogBase::set_hints_horizontal_resize_only() {
    Gdk::Geometry geometry;
    // Height can't grow beyond the necessary minimum
    geometry.min_height = 1;
    geometry.max_height = 1;
    // Width is not limited
    geometry.min_width = 1;
    geometry.max_width = 10000;
    set_geometry_hints(*this, geometry,
                       Gdk::HINT_MIN_SIZE | Gdk::HINT_MAX_SIZE);
}

void DialogBase::remove_cancel_button() {
    auto cancel_button = get_widget_for_response(Gtk::RESPONSE_CANCEL);
    if (cancel_button) {
        get_action_area()->remove(*cancel_button);
    }
}

}  // namespace ff::dlg
