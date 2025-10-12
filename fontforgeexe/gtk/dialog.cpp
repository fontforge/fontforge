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

#include "dialog.hpp"

#include "intl.h"
#include "gimage.h"
#include "utils.hpp"

typedef struct gevent GEvent;

namespace ff::dlg {

Dialog::Dialog(GWindow parent_gwin) : parent_gwindow_(parent_gwin) {
    auto ok_button = add_button(_("_OK"), Gtk::RESPONSE_OK);
    ok_button->set_name("ok");

    auto cancel_button = add_button(_("_Cancel"), Gtk::RESPONSE_CANCEL);
    cancel_button->set_name("cancel");

    set_default_response(Gtk::RESPONSE_OK);
    set_position(Gtk::WIN_POS_CENTER);
}

Gtk::ResponseType Dialog::run() {
    if (parent_gwindow_) {
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

        GdkWindow* parent_gdk_window = ((ggdkwindow_local*)parent_gwindow_)->w;
        GdkWindow* this_window = get_window().get()->gobj();
        gdk_window_set_transient_for(this_window, parent_gdk_window);
    }

    return (Gtk::ResponseType)Gtk::Dialog::run();
}

}  // namespace ff::dlg
