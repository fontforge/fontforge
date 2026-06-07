/* Copyright 2026 Maxim Iorsh <iorsh@users.sourceforge.net>
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

#include <functional>
#include <string>
#include <vector>

#include <gtkmm.h>

namespace ff::widgets {

class Histogram : public Gtk::DrawingArea {
 public:
    Histogram();

    void set_values(const std::vector<int>& values);
    void set_bar_width(int width_px);
    void set_moving_average_window(int window_size);
    void set_lower_bound(int lower_bound);

    // int bar_index argument is the actual bar X-value, not the index in the
    // bars vector.
    void set_tooltip_text_callback(
        std::function<std::string(int bar_index, double average)>
            tooltip_text_callback);
    void set_bar_click_callback(
        std::function<void(int bar_index, bool shift_pressed)>
            bar_click_callback);

 protected:
    bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;

 private:
    // Retrieve the actual bar X-value, which may be negative.
    bool get_bar_index(int x, int& index) const;

    bool on_button_press_event(GdkEventButton* event);
    bool on_query_tooltip_event(int x, int y, bool keyboard_tooltip,
                                const Glib::RefPtr<Gtk::Tooltip>& tooltip);
    void update_size_request();
    double draw_axis(const Cairo::RefPtr<Cairo::Context>& cr, int width,
                     int height);
    void draw_axis_tick(const Cairo::RefPtr<Cairo::Context>& cr, double axis_y,
                        int index);
    void draw_bars(const Cairo::RefPtr<Cairo::Context>& cr, double bar_base);
    void draw_moving_average(const Cairo::RefPtr<Cairo::Context>& cr,
                             double bar_base);

    std::vector<int> values_;
    std::vector<double> avg_values_;
    int bar_width_px_ = 10;
    int moving_average_window_ = 1;
    int lower_bound_ = 0;
    std::function<std::string(int bar_index, double average)> tooltip_text_cb_;
    std::function<void(int bar_index, bool shift_pressed)> bar_click_cb_;
};

}  // namespace ff::widgets
