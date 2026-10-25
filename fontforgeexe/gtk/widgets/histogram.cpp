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

#include "histogram.hpp"

#include <algorithm>
#include <utility>

#include "../application.hpp"

namespace ff::widgets {

std::vector<double> moving_average(const std::vector<int>& series,
                                   size_t window_size) {
    const size_t safe_window = std::max((size_t)1, window_size);
    const size_t half_window = safe_window / 2;

    std::vector<double> smoothed(series.size(), 0);
    for (size_t i = 0; i < series.size(); ++i) {
        const size_t from = i < half_window ? 0 : i - half_window;
        const size_t to = std::min(series.size() - 1, i + half_window);

        double sum = 0;
        for (size_t j = from; j <= to; ++j) {
            sum += series[j];
        }
        smoothed[i] = sum / (to - from + 1);
    }

    return smoothed;
}

static constexpr int kHistogramMinWidth = 240;
static constexpr int kHistogramHeight = 180;
static constexpr int kBarGapPx = 1;
static constexpr int kOuterMarginPx = 4;
static constexpr int kAxisTickPx = 4;
static constexpr int kAxisLabelGapPx = 2;
static constexpr int kAxisLabelBottomPx = 2;

Histogram::Histogram() {
    set_hexpand(true);
    set_vexpand(true);
    set_has_tooltip(true);
    signal_query_tooltip().connect(
        sigc::mem_fun(*this, &Histogram::on_query_tooltip_event));
    add_events(Gdk::BUTTON_PRESS_MASK);
    signal_button_press_event().connect(
        sigc::mem_fun(*this, &Histogram::on_button_press_event));
    set_size_request(kHistogramMinWidth, kHistogramHeight);
}

void Histogram::set_values(const std::vector<int>& values) {
    values_ = values;
    avg_values_ = moving_average(values_, moving_average_window_);
    update_size_request();
}

void Histogram::set_bar_width(int width_px) {
    bar_width_px_ = std::max(1, width_px);
    update_size_request();
}

void Histogram::set_moving_average_window(int window_size) {
    moving_average_window_ = std::max(1, window_size);
    avg_values_ = moving_average(values_, moving_average_window_);
    queue_draw();
}

void Histogram::set_lower_bound(int lower_bound) {
    lower_bound_ = lower_bound;
    queue_draw();
}

void Histogram::set_tooltip_text_callback(
    std::function<std::string(int, double)> tooltip_text_callback) {
    tooltip_text_cb_ = std::move(tooltip_text_callback);
}

void Histogram::set_bar_click_callback(
    std::function<void(int, bool)> bar_click_callback) {
    bar_click_cb_ = std::move(bar_click_callback);
}

void Histogram::update_size_request() {
    int content_width = kHistogramMinWidth;
    if (!values_.empty()) {
        content_width =
            std::max(kHistogramMinWidth, 2 * kOuterMarginPx +
                                             static_cast<int>(values_.size()) *
                                                 (bar_width_px_ + kBarGapPx) -
                                             kBarGapPx);
    }
    set_size_request(content_width, kHistogramHeight);
    queue_draw();
}

void Histogram::draw_axis_tick(const Cairo::RefPtr<Cairo::Context>& cr,
                               double axis_y, int index) {
    const double tick_x = kOuterMarginPx +
                          (index - lower_bound_) * (bar_width_px_ + kBarGapPx) +
                          bar_width_px_ / 2.0;
    const double tick_y1 = axis_y + kAxisTickPx;
    cr->move_to(tick_x, axis_y);
    cr->line_to(tick_x, tick_y1);
    cr->stroke();

    const std::string label = std::to_string(index);
    auto label_layout = create_pango_layout(label);
    int label_width = 0;
    int label_height = 0;
    label_layout->get_pixel_size(label_width, label_height);

    const double label_x = tick_x - label_width / 2.0;
    const double label_y = tick_y1 + kAxisLabelGapPx;
    cr->move_to(label_x, label_y);
    label_layout->show_in_cairo_context(cr);
}

double Histogram::draw_axis(const Cairo::RefPtr<Cairo::Context>& cr, int width,
                            int height) {
    int axis_label_height = 0;
    int tick_step = 1000;
    if (!values_.empty()) {
        // Use the last (widest) label value to compute tick spacing.
        auto sample_layout = create_pango_layout(
            std::to_string(values_.size() - 1 + lower_bound_));
        int sample_width = 0;
        sample_layout->get_pixel_size(sample_width, axis_label_height);

        // Select sufficiently wide tick step
        for (int step : {1, 2, 5, 10, 20, 50, 100, 500, 1000}) {
            if (step * (bar_width_px_ + kBarGapPx) >= sample_width * 3 / 2) {
                tick_step = step;
                break;
            }
        }
    }

    const double axis_x0 = kOuterMarginPx;
    const double axis_x1 =
        std::max(axis_x0, static_cast<double>(width - kOuterMarginPx));
    const double axis_height = kAxisTickPx + kAxisLabelGapPx +
                               axis_label_height + kAxisLabelBottomPx + 0.5;
    const double axis_y = height - axis_height;

    app::ColorManager::instance().set_color_in_context(cr, "ff_histogram_axis");
    cr->set_line_width(1.0);
    cr->move_to(axis_x0, axis_y);
    cr->line_to(axis_x1, axis_y);
    cr->stroke();

    // Ticks at round values. Fix for rounding towards zero for integer division
    // of negative values.
    int i = (lower_bound_ > 0) ? ((lower_bound_ / tick_step + 1) * tick_step)
                               : (lower_bound_ / tick_step * tick_step);
    for (; i < (int)values_.size() + lower_bound_; i += tick_step) {
        draw_axis_tick(cr, axis_y, i);
    }

    return axis_height;
}

void Histogram::draw_bars(const Cairo::RefPtr<Cairo::Context>& cr,
                          double bar_base) {
    if (values_.empty()) {
        return;
    }

    const int max_value = *std::max_element(values_.cbegin(), values_.cend());
    if (max_value <= 0) {
        return;
    }

    const double bar_max_height = std::max(1.0, bar_base - kOuterMarginPx);

    app::ColorManager::instance().set_color_in_context(cr, "ff_histogram_bars");
    for (size_t i = 0; i < values_.size(); ++i) {
        const double norm = static_cast<double>(values_[i]) / max_value;
        const double bar_height = norm * bar_max_height;
        const double x = kOuterMarginPx + i * (bar_width_px_ + kBarGapPx);
        const double y = bar_base - bar_height;

        cr->rectangle(x, y, bar_width_px_, bar_height);
        cr->fill();
    }
}

void Histogram::draw_moving_average(const Cairo::RefPtr<Cairo::Context>& cr,
                                    double bar_base) {
    // For window size 1 the moving average is the same as the original values.
    if (values_.empty() || moving_average_window_ <= 1) {
        return;
    }

    const int max_value = *std::max_element(values_.cbegin(), values_.cend());
    if (max_value <= 0) {
        return;
    }
    const double bar_max_height = std::max(1.0, bar_base - kOuterMarginPx);

    app::ColorManager::instance().set_color_in_context(
        cr, "ff_histogram_moving_average");
    cr->set_line_width(1.5);

    bool started = false;
    for (size_t i = 0; i < avg_values_.size(); ++i) {
        const double norm = avg_values_[i] / max_value;
        const double x = kOuterMarginPx + i * (bar_width_px_ + kBarGapPx) +
                         bar_width_px_ / 2.0;
        const double y = bar_base - norm * bar_max_height;

        if (!started) {
            cr->move_to(x, y);
            started = true;
        } else {
            cr->line_to(x, y);
        }
    }
    cr->stroke();
}

bool Histogram::on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
    Gtk::Allocation allocation = get_allocation();
    const int width = allocation.get_width();
    const int height = allocation.get_height();

    app::ColorManager::instance().set_color_in_context(cr, "ff_histogram_bg");
    cr->paint();

    if (width <= 0 || height <= 0) {
        return true;
    }

    const double axis_height = draw_axis(cr, width, height);
    const double bar_base = height - axis_height;
    draw_bars(cr, bar_base);
    draw_moving_average(cr, bar_base);

    return true;
}

bool Histogram::get_bar_index(int x, int& index) const {
    const int pitch = bar_width_px_ + kBarGapPx;
    if (pitch <= 0) {
        return false;
    }

    const int relative_x = x - kOuterMarginPx;
    if (relative_x < 0) {
        return false;
    }

    const size_t candidate = static_cast<size_t>(relative_x / pitch);
    if (candidate >= values_.size()) {
        return false;
    }

    // Convert from index in the vector of values to actual bar X-value.
    index = candidate + lower_bound_;
    return true;
}

bool Histogram::on_query_tooltip_event(
    int x, int y, bool keyboard_tooltip,
    const Glib::RefPtr<Gtk::Tooltip>& tooltip) {
    if (keyboard_tooltip || values_.empty()) {
        return false;
    }

    int index = 0;
    if (!get_bar_index(x, index)) {
        return false;
    }

    const Gtk::Allocation allocation = get_allocation();
    const int height = allocation.get_height();
    if (height <= 0) {
        return false;
    }

    int axis_label_height = 0;
    auto sample_layout =
        create_pango_layout(std::to_string(values_.size() - 1));
    int sample_width = 0;
    sample_layout->get_pixel_size(sample_width, axis_label_height);

    const double axis_height = kAxisTickPx + kAxisLabelGapPx +
                               axis_label_height + kAxisLabelBottomPx + 0.5;
    const double bar_base = height - axis_height;
    if (y < 0 || y > bar_base) {
        return false;
    }

    // Make tooltip follow the mouse (the normal GTK behavior is to anchor the
    // tooltip once shown).
    const int bar_x = kOuterMarginPx + static_cast<int>(index - lower_bound_) *
                                           (bar_width_px_ + kBarGapPx);
    tooltip->set_tip_area(
        Gdk::Rectangle(bar_x, 0, bar_width_px_, static_cast<int>(height)));
    if (tooltip_text_cb_) {
        tooltip->set_text(
            tooltip_text_cb_(index, avg_values_[index - lower_bound_]));
    }
    return true;
}

bool Histogram::on_button_press_event(GdkEventButton* event) {
    if (!bar_click_cb_ || values_.empty()) {
        return false;
    }

    const int x = static_cast<int>(event->x);
    const int y = static_cast<int>(event->y);
    const bool shift_pressed = (event->state & GDK_SHIFT_MASK) != 0;

    int index = 0;
    if (!get_bar_index(x, index)) {
        return false;
    }

    const Gtk::Allocation allocation = get_allocation();
    const int height = allocation.get_height();
    if (height <= 0) {
        return false;
    }

    int axis_label_height = 0;
    auto sample_layout =
        create_pango_layout(std::to_string(values_.size() - 1 + lower_bound_));
    int sample_width = 0;
    sample_layout->get_pixel_size(sample_width, axis_label_height);

    const double axis_height = kAxisTickPx + kAxisLabelGapPx +
                               axis_label_height + kAxisLabelBottomPx + 0.5;
    const double bar_base = height - axis_height;
    if (y < 0 || y > bar_base) {
        return false;
    }

    bar_click_cb_(index, shift_pressed);
    return true;
}

}  // namespace ff::widgets
