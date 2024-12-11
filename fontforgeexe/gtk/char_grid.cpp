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

#include "char_grid.hpp"

#include <atomic>

#include "utils.hpp"

namespace ff::views {

bool on_drawing_area_event(GdkEvent* event);
bool on_drawing_area_key(GdkEventKey* event, Gtk::DrawingArea& drawing_area);
void on_scrollbar_value_changed(std::shared_ptr<FVContext> fv_context,
                                Gtk::VScrollbar& scroller);
bool on_query_tooltip(std::shared_ptr<FVContext> fv_context, int x, int y,
                      const Glib::RefPtr<Gtk::Tooltip>& tooltip);
bool on_mouse_move(Gtk::DrawingArea& drawing_area);

// A hack to transfer motion event to tooltip query
static std::atomic<bool> mouse_moved(false);

CharGrid::CharGrid(std::shared_ptr<FVContext> context) {
    scroller.signal_value_changed().connect(
        [this, context]() { on_scrollbar_value_changed(context, scroller); });

    drawing_area.set_name("CharGrid");
    drawing_area.set_vexpand(true);
    drawing_area.set_hexpand(true);

    // Fontforge drawing area processes events in the legacy code
    // expose, keypresses, mouse etc.
    drawing_area.signal_event().connect(&on_drawing_area_event);

    // Keep the drawing area focused after mouse click, so that it can further
    // expand selection with arrow keys.
    drawing_area.signal_button_press_event().connect(
        [this](GdkEventButton* event) {
            drawing_area.grab_focus();
            return false;
        });

    // Drawing area is responsible to dispatch keypress events. Most go to the
    // legacy code.
    drawing_area.signal_key_press_event().connect([this](GdkEventKey* event) {
        return on_drawing_area_key(event, drawing_area);
    });

    // Redirect mouse scrolling events from the drawing area to the scrollbar
    auto on_drawing_area_scroll = [this](GdkEventScroll* event) {
        scroller.event((GdkEvent*)event);
        return true;
    };
    drawing_area.signal_scroll_event().connect(on_drawing_area_scroll);

    drawing_area.set_events(Gdk::ALL_EVENTS_MASK);
    drawing_area.set_can_focus(true);

    // When the pointer is over character cell, display tooltip with this
    // character's Unicode info; dismissed on mouse movement.
    drawing_area.set_has_tooltip();
    drawing_area.signal_query_tooltip().connect(
        [context](int x, int y, bool,
                  const Glib::RefPtr<Gtk::Tooltip>& tooltip) {
            return on_query_tooltip(context, x, y, tooltip);
        });
    drawing_area.signal_motion_notify_event().connect(
        [this](GdkEventMotion*) { return on_mouse_move(drawing_area); });

    make_character_info_label();

    char_grid_box.attach(character_info, 0, 0, 2, 1);
    char_grid_box.attach(drawing_area, 0, 1);
    char_grid_box.attach(scroller, 1, 1);
}

Gtk::Widget& CharGrid::get_top_widget() { return char_grid_box; }

GtkWidget* CharGrid::get_drawing_widget_c() {
    return (GtkWidget*)drawing_area.gobj();
}

void CharGrid::set_scroller_position(int32_t position) {
    if (!scroller.has_grab()) {
        // Set the scroller only if its slider is not currently grabbed with the
        // mouse.
        scroller.get_adjustment()->set_value(position);
    }
}

void CharGrid::set_scroller_bounds(int32_t sb_min, int32_t sb_max,
                                   int32_t sb_pagesize) {
    Glib::RefPtr<Gtk::Adjustment> adjustment = scroller.get_adjustment();
    // VScrollbar seems to ignore step and page increments and behaves
    // incoherently to the extent that a single click on a stepper button yields
    // a different delta each time. The values of 3, 3 are somehow okeyish.
    adjustment->configure(adjustment->get_value(), sb_min, sb_max, 3, 3,
                          sb_pagesize);
}

void CharGrid::set_character_info(const std::string& info) {
    Glib::ustring markup = "<big>" + info + "</big>";
    character_info.set_markup(markup);
}

void CharGrid::resize_drawing_area(int width, int height) {
    Gtk::Window* window =
        dynamic_cast<Gtk::Window*>(drawing_area.get_toplevel());

    // The target size is for DrawingArea, but we can resize only the top
    // window. Compute size deltas to correctly transform between drawing area
    // size and top window size.
    int delta_width =
        window->get_allocated_width() - drawing_area.get_allocated_width();
    int delta_height =
        window->get_allocated_height() - drawing_area.get_allocated_height();

    window->resize(width + delta_width, height + delta_height);
}

// Create info label at the top of the Font View, which shows name and
// properties of the most recently selected character
void CharGrid::make_character_info_label() {
    character_info.set_name("CharInfo");
    character_info.property_margin().set_value(2);
    character_info.set_margin_left(10);
    character_info.set_hexpand(true);
    character_info.set_xalign(0);               // Flush left
    character_info.set_markup("<big> </big>");  // Set height

    // Long info string will not allow us to shrink the main window, so we
    // let it be truncated dynamically with ellipsis.
    character_info.set_ellipsize(Pango::ELLIPSIZE_END);

    // We want the info to stand out, but can't hardcode a color
    // due to the use of color themes (light, dark or even something custom)
    // We use link color to make the label sufficiently distinctive.
    Glib::RefPtr<Gtk::StyleContext> context =
        character_info.get_style_context();
    Gdk::RGBA link_color = context->get_color(Gtk::STATE_FLAG_LINK);
    character_info.override_color(link_color);
}

/////////////////  EVENTS  ////////////////////

bool on_drawing_area_event(GdkEvent* event) {
    // Normally events automatically get to the main loop and picked from there
    // by the legacy GDraw handler. The DrawingArea::resize signal doesn't go
    // there for some reason. My best guess is that this signal is not emitted,
    // but the framework just invokes the handler directly. Here we catch it and
    // place into the main loop, so that it can reach the GDraw handler.
    if (event->type == GDK_CONFIGURE) {
        gdk_event_put(event);
        return true;
    }

    // Return false to allow further propagation of unhandled events
    return false;
}

bool on_drawing_area_key(GdkEventKey* event, Gtk::DrawingArea& drawing_area) {
    // All keypress events belong to the top window. Some of them must go to
    // the main loop to be picked by the legacy GDraw handler. Their window
    // must be replaced, because legacy GDraw handler picks only events which
    // belong to the drawing area window.

    // If the event already belongs to the drawing area window, and marked as
    // created in the legacy code, it should have already been processed in the
    // legacy GDraw handler. We just let it continue with the normal GTK event
    // processing.
    if (g_object_get_data(G_OBJECT(event->window), "GGDKWindow")) {
        return false;
    }

    GdkWindow* drawing_win =
        gtk_widget_get_window((GtkWidget*)(drawing_area.gobj()));

    // The GDK reference handling is very fragile, so we *slowly* replace the
    // window...
    GdkWindow* old_event_window = event->window;
    event->window = drawing_win;

    // Put a copy of the event into the main loop. The event copy handles its
    // object references by itself.
    gdk_event_put((GdkEvent*)event);

    // *Slowly* replace the window back, so that the handler caller wraps it
    // up correctly.
    event->window = old_event_window;

    // Don't handle this event any further.
    return true;
}

void on_scrollbar_value_changed(std::shared_ptr<FVContext> fv_context,
                                Gtk::VScrollbar& scroller) {
    double new_position = scroller.get_value();
    fv_context->scroll_fontview_to_position_cb(fv_context->fv, new_position);
}

bool on_query_tooltip(std::shared_ptr<FVContext> fv_context, int x, int y,
                      const Glib::RefPtr<Gtk::Tooltip>& tooltip) {
    if (mouse_moved) {
        // Mouse motion occured, dismiss the tooltip
        mouse_moved = false;
        return false;
    }

    std::string tooltip_msg =
        StringWrapper(fv_context->tooltip_message_cb)(fv_context->fv, x, y);
    if (tooltip_msg.empty()) {
        return false;
    }

    Glib::ustring text = Glib::Markup::escape_text(tooltip_msg);
    text = Glib::ustring::compose("<small>%1</small>", text);
    tooltip->set_markup(text);

    return true;
}

bool on_mouse_move(Gtk::DrawingArea& drawing_area) {
    if (!mouse_moved) {
        mouse_moved = true;
        drawing_area.trigger_tooltip_query();
    }
    return true;
}

}  // namespace ff::views
