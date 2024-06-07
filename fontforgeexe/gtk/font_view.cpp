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

#include "font_view.hpp"

#include "application.hpp"
#include "menu_builder.hpp"
#include "utils.hpp"

namespace ff::views {

FontViewUiContext::FontViewUiContext(Gtk::Window& window,
                                     std::shared_ptr<FVContext> fv_context)
    : UiContext(window), legacy_context(fv_context) {
    accel_group = Gtk::AccelGroup::create();
}

ActivateCB FontViewUiContext::get_activate_cb(int mid) const {
    FVMenuAction* callback_set =
        find_legacy_callback_set(mid, legacy_context->actions);

    if (callback_set != NULL && callback_set->action != NULL) {
        void (*action)(::FontView*, int) = callback_set->action;
        ::FontView* fv = legacy_context->fv;
        return [action, fv, mid](const UiContext&) { action(fv, mid); };
    } else {
        return NoAction;
    }
}

EnabledCB FontViewUiContext::get_enabled_cb(int mid) const {
    FVMenuAction* callback_set =
        find_legacy_callback_set(mid, legacy_context->actions);

    if (callback_set != NULL && callback_set->is_disabled != NULL) {
        bool (*disabled_cb)(::FontView*, int) = callback_set->is_disabled;
        ::FontView* fv = legacy_context->fv;
        return [disabled_cb, fv, mid](const UiContext&) {
            return !disabled_cb(fv, mid);
        };
    } else {
        return AlwaysEnabled;
    }
}

CheckedCB FontViewUiContext::get_checked_cb(int mid) const {
    FVMenuAction* callback_set =
        find_legacy_callback_set(mid, legacy_context->actions);

    if (callback_set != NULL && callback_set->is_checked != NULL) {
        bool (*checked_cb)(::FontView*, int) = callback_set->is_checked;
        ::FontView* fv = legacy_context->fv;
        return [checked_cb, fv, mid](const UiContext&) {
            return checked_cb(fv, mid);
        };
    } else {
        return NotCheckable;
    }
}

bool on_button_press_event(GdkEventButton* event, Gtk::Menu& pop_up);
bool on_font_view_event(GdkEvent* event);

FontView::FontView(std::shared_ptr<FVContext> fv_context, int width, int height)
    : context(window, fv_context), char_grid(fv_context) {
    ff::app::add_top_view(context);

    window.signal_delete_event().connect([this](GdkEventAny*) {
        ff::app::remove_top_view(window);
        return false;
    });

    // dialog.resize() doesn't work until after the realization, i.e. after
    // dialog.show_all(). Use the realize event to ensure reliable resizing.
    //
    // Also, the signal itself must be connected before dialog.show_all(),
    // otherwise it wouldn't work for some reason...
    window.signal_realize().connect([this, width, height]() {
        char_grid.resize_drawing_area(width, height);
    });

    window.signal_event().connect(&on_font_view_event);

    top_bar = build_menu_bar(top_menu, context);

    font_view_grid.attach(top_bar, 0, 0);
    font_view_grid.attach(h_sep, 0, 1);
    font_view_grid.attach(char_grid.get_top_widget(), 0, 2);

    window.add(font_view_grid);
    window.show_all();

    pop_up = std::move(*build_menu(popup_menu, context));

    char_grid.get_top_widget().signal_button_press_event().connect(
        [this](GdkEventButton* event) {
            return on_button_press_event(event, pop_up);
        });

    // TODO(iorsh): review this very stragne hack.
    // For reasons beyond my comprehension, DrawingArea fails to catch events
    // before this function is called. A mere traverasal of widget tree should
    // theoretically have no side efects, but it does.
    gtk_find_child(&window, "");

    window.add_accel_group(context.get_accel_group());
}

/////////////////  EVENTS  ////////////////////

bool on_button_press_event(GdkEventButton* event, Gtk::Menu& pop_up) {
    if (event->button == GDK_BUTTON_SECONDARY) {
        pop_up.show_all();
        pop_up.popup(event->button, event->time);
        return true;
    }
    return false;
}

static void dismiss_menus(GdkEvent* event) {
    Gtk::Widget* grabber = Gtk::Widget::get_current_modal_grab();
    while (grabber) {
        Gtk::Menu* menu = dynamic_cast<Gtk::Menu*>(grabber);
        if (menu) {
            grabber = menu->get_parent_shell();
            menu->get_parent()->hide();
        } else {
            grabber = nullptr;
        }
    }
}

bool on_font_view_event(GdkEvent* event) {
    // Wayland fails to dismiss open menus when the user moves the window or
    // changes the focus to another application. This hack dismisses the menus
    // manually, but it would be nice if someday Wayland does it itself.
    if (event->type == GDK_WINDOW_STATE || event->type == GDK_CONFIGURE) {
        dismiss_menus(event);
    }

    return false;
}

}  // namespace ff::views
