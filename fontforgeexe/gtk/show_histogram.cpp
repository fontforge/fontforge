/* Copyright (C) 2026 by Maxim Iorsh <iorsh@users@sourceforge.net>
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

#include "show_histogram.hpp"

#include <sstream>
#include <set>

#include "application.hpp"
#include "utils.hpp"

extern "C" {
#include "ustring.h"
}

namespace ff::dlg {

namespace {
int s_bar_width = 6;
int s_moving_average = 1;

std::set<int> parse_values(std::string current, int* error_pos_start = nullptr,
                           int* error_pos_end = nullptr) {
    std::set<int> values;
    bool substring = false;
    if (current.empty()) {
        if (error_pos_start) *error_pos_start = -1;
        if (error_pos_end) *error_pos_end = -1;
        return values;
    }

    if (current.front() == '[' && current.back() == ']') {
        current = current.substr(1, current.size() - 2);
        substring = true;
    }

    std::istringstream in(current);
    int value = 0;
    while (in >> value) {
        values.insert(value);
    }

    if (error_pos_start && error_pos_end) {
        if (in.fail() && !in.eof()) {
            in.clear();
            std::string token;
            *error_pos_start = static_cast<int>(in.tellg());
            in >> token;
            *error_pos_end = *error_pos_start + token.size();
            if (substring) {
                // Adjust error positions to account for removed brackets.
                *error_pos_start += 1;
                *error_pos_end += 1;
            }
        } else {
            *error_pos_start = -1;
            *error_pos_end = -1;
        }
    }

    return values;
}

std::string combine_values(const std::set<int>& values,
                           int* latest_value = nullptr) {
    std::ostringstream out;
    out << '[';
    bool first = true;
    for (int value : values) {
        if (!first) {
            out << ' ';
        }
        out << value;
        first = false;

        // Add a placeholder after the most recent value.
        if (latest_value && value == *latest_value) {
            out << " ?";
        }
    }
    out << ']';
    return out.str();
}

class ErrorLabel : public Gtk::Label {
 public:
    ErrorLabel() {
        auto error_css_provider = Gtk::CssProvider::create();
        error_css_provider->load_from_data("label { color: @error_color; }");
        get_style_context()->add_provider(error_css_provider,
                                          GTK_STYLE_PROVIDER_PRIORITY_USER - 1);
    }
};

}  // namespace

ShowHistogramDlg::ShowHistogramDlg(GWindow parent, const HistogramData& data)
    : DialogBase(parent), data_(data) {
    const UiStrings& ui_strings = kHistogramUiStrings.at(data.type);

    app::ColorManager::instance().register_colors({
        {"ff_histogram_bg", {"theme_base_color", Gdk::RGBA("white")}},
        {"ff_histogram_axis", {"theme_fg_color", Gdk::RGBA("black")}},
        {"ff_histogram_bars", {"theme_selected_bg_color", Gdk::RGBA("blue")}},
        {"ff_histogram_moving_average", {"error_color", Gdk::RGBA("red")}},
    });

    set_title(ui_strings.title);
    set_help_context("ui/dialogs/histogram.html");

    std::vector<int> bar_values;
    for (const auto& bar : data.bars) {
        bar_values.push_back(static_cast<int>(bar.count));
    }
    if (!bar_values.empty()) {
        max_value_ = *std::max_element(bar_values.cbegin(), bar_values.cend());
    }
    histogram_.set_values(bar_values);
    histogram_.set_lower_bound(data.lower_bound);
    histogram_.set_tooltip_text_callback([this](int index, double average) {
        return get_tooltip_text(index, average);
    });
    if (data.type == hist_blues) {
        histogram_.set_bar_click_callback(
            [this](int index, bool shift_pressed) {
                on_blues_bar_click(index, shift_pressed);
            });
    } else {
        histogram_.set_bar_click_callback(
            [this](int index, bool shift_pressed) {
                on_stem_bar_click(index, shift_pressed);
            });
    }
    histogram_.set_bar_width(s_bar_width);
    histogram_.set_moving_average_window(s_moving_average);

    auto histogram_scroll = Gtk::make_managed<Gtk::ScrolledWindow>();
    histogram_scroll->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_NEVER);
    histogram_scroll->set_overlay_scrolling(false);
    histogram_scroll->add(histogram_);
    get_content_area()->pack_start(*histogram_scroll, Gtk::PACK_EXPAND_WIDGET);
    get_content_area()->pack_start(*build_control_box(), Gtk::PACK_SHRINK);

    auto label_group = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
    auto primary_box =
        build_entry_box(ui_strings.primary_label, data_.initial_values.primary,
                        primary_entry_, label_group);
    get_content_area()->pack_start(*primary_box, Gtk::PACK_SHRINK);

    auto secondary_box = build_entry_box(ui_strings.secondary_label,
                                         data_.initial_values.secondary,
                                         secondary_entry_, label_group);
    get_content_area()->pack_start(*secondary_box, Gtk::PACK_SHRINK);

    if (data.small_selection_warning) {
        auto warning_label = Gtk::make_managed<ErrorLabel>();
        warning_label->set_text(
            _("There are so few glyphs selected that it seems unlikely to me "
              "that you will get a representative sample of this aspect of "
              "your font. If you deselect everything the command will apply to "
              "all glyphs in the font"));
        warning_label->set_line_wrap();
        // Prevent label from asking for a lot of horizontal space.
        warning_label->set_max_width_chars(1);
        warning_label->set_xalign(0.0);
        get_content_area()->pack_start(*warning_label, Gtk::PACK_SHRINK);
    }

    signal_response().connect(
        [this](int response_id) {
            if (response_id == Gtk::RESPONSE_OK &&
                (!primary_entry_.verify() || !secondary_entry_.verify())) {
                signal_response().emission_stop();
            }
        },
        false);

    show_all();
}

Gtk::Box* ShowHistogramDlg::build_control_box() {
    auto controls_box = Gtk::make_managed<Gtk::Box>(
        Gtk::ORIENTATION_HORIZONTAL, 0.5 * ff::ui_utils::ui_font_em_size());

    auto average_label = Gtk::make_managed<Gtk::Label>(_("Smoothing window:"));
    average_label->set_halign(Gtk::ALIGN_START);
    average_label->set_valign(Gtk::ALIGN_CENTER);
    controls_box->pack_start(*average_label, Gtk::PACK_SHRINK);

    average_entry_.configure(
        Gtk::Adjustment::create(s_moving_average, 1, 99, 2, 10, 0), 1, 0);
    average_entry_.set_numeric(true);
    average_entry_.set_snap_to_ticks(true);
    average_entry_.set_width_chars(2);
    average_entry_.set_valign(Gtk::ALIGN_CENTER);
    average_entry_.set_activates_default();
    average_entry_.signal_value_changed().connect([this]() {
        s_moving_average = average_entry_.get_value_as_int();
        histogram_.set_moving_average_window(s_moving_average);
    });
    controls_box->pack_start(average_entry_, Gtk::PACK_SHRINK);

    auto bar_width_label = Gtk::make_managed<Gtk::Label>(_("Bar width:"));
    bar_width_label->set_halign(Gtk::ALIGN_START);
    bar_width_label->set_valign(Gtk::ALIGN_CENTER);
    controls_box->pack_start(*bar_width_label, Gtk::PACK_SHRINK);

    auto bar_width_entry = Gtk::make_managed<Gtk::SpinButton>(
        Gtk::Adjustment::create(s_bar_width, 1, 100, 1, 5, 0), 1, 0);
    bar_width_entry->set_numeric(true);
    bar_width_entry->set_width_chars(2);
    bar_width_entry->set_valign(Gtk::ALIGN_CENTER);
    bar_width_entry->set_activates_default();
    bar_width_entry->signal_value_changed().connect([bar_width_entry, this]() {
        s_bar_width = bar_width_entry->get_value_as_int();
        histogram_.set_bar_width(s_bar_width);
    });
    controls_box->pack_start(*bar_width_entry, Gtk::PACK_SHRINK);

    return controls_box;
}

Gtk::Box* ShowHistogramDlg::build_entry_box(
    const Glib::ustring& label, const Glib::ustring& value,
    widgets::VerifiedEntry& entry, Glib::RefPtr<Gtk::SizeGroup> label_group) {
    auto entry_box = Gtk::make_managed<Gtk::Box>(
        Gtk::ORIENTATION_HORIZONTAL, 0.5 * ff::ui_utils::ui_font_em_size());
    auto label_widget = Gtk::make_managed<Gtk::Label>(label + ":");
    label_widget->set_xalign(0.0);
    label_widget->set_valign(Gtk::ALIGN_CENTER);
    label_group->add_widget(*label_widget);
    entry_box->pack_start(*label_widget, Gtk::PACK_SHRINK);

    entry.set_text(value);
    entry.set_hexpand(true);
    entry.set_activates_default();
    entry.set_verifier(dict_value_verifier);
    entry_box->pack_start(entry, Gtk::PACK_EXPAND_WIDGET);

    return entry_box;
}

std::string ShowHistogramDlg::get_tooltip_text(int bar_index,
                                               double average) const {
    if (bar_index >= (int)data_.bars.size() + data_.lower_bound) {
        return "";
    }
    const auto& bar = data_.bars[bar_index - data_.lower_bound];

    const char* label_fmt =
        (data_.type == hist_blues) ? _("Position: %d") : _("Width: %d");
    char* p_width_label = smprintf(label_fmt, bar_index);
    char* p_count_label;
    if (average_entry_.get_value_as_int() > 1) {
        p_count_label =
            smprintf(_("Count: %u (average: %.2f)"), bar.count, average);
    } else {
        p_count_label = smprintf(_("Count: %u"), bar.count);
    }
    std::string tooltip_text(p_width_label);
    tooltip_text += "\n" + std::string(p_count_label);
    free(p_width_label);
    free(p_count_label);

    if (data_.type != hist_blues && max_value_ > 0) {
        char* p_percentage_label =
            smprintf(_("Percentage of Max: %d%%"),
                     static_cast<int>(rint(bar.count * 100.0 / max_value_)));
        tooltip_text += "\n" + std::string(p_percentage_label);
        free(p_percentage_label);
    }

    if (!bar.glyph_names.empty()) {
        tooltip_text += "\n";
    }

    for (const auto& name : bar.glyph_names) {
        if (tooltip_text.back() != '\n') {
            tooltip_text += " ";
        }
        tooltip_text += name;
        // Limit tooltip length to prevent it from growing too big if there are
        // many glyphs.
        if (tooltip_text.size() > 300) {
            tooltip_text += " ...";
            break;
        }
    }

    return tooltip_text;
}

void ShowHistogramDlg::on_stem_bar_click(int bar_index, bool shift_pressed) {
    if (!shift_pressed) {
        const std::string clicked = combine_values({bar_index});
        primary_entry_.set_text(clicked);
        secondary_entry_.set_text(clicked);
        return;
    }

    std::set<int> values = parse_values(secondary_entry_.get_text());
    values.insert(bar_index);
    secondary_entry_.set_text(combine_values(values));
}

void ShowHistogramDlg::on_blues_bar_click(int bar_index,
                                          bool /*shift_pressed*/) {
    static const std::string placeholder = "?";
    size_t primary_placeholder = primary_entry_.get_text().find(placeholder);
    size_t secondary_placeholder =
        secondary_entry_.get_text().find(placeholder);
    // If some entry has a placeholder, the click should replace that
    // placeholder. Otherwise we consider the value sign (positive to
    // BlueValues, negative to OtherBlues).
    bool is_primary =
        primary_placeholder != std::string::npos ||
        (secondary_placeholder == std::string::npos && bar_index >= 0);

    Gtk::Entry* target_entry = is_primary ? &primary_entry_ : &secondary_entry_;
    size_t placeholder_pos =
        is_primary ? primary_placeholder : secondary_placeholder;

    histogram_.get_window()->set_cursor();
    primary_entry_.set_has_tooltip(false);
    secondary_entry_.set_has_tooltip(false);

    // Strip placeholder before parsing
    std::string current_text = target_entry->get_text();
    if (placeholder_pos != std::string::npos) {
        current_text.erase(placeholder_pos, placeholder.size());
    }
    std::set<int> values = parse_values(current_text);
    values.insert(bar_index);
    bool need_placeholder = values.size() % 2 == 1;
    target_entry->set_text(
        combine_values(values, need_placeholder ? &bar_index : nullptr));

    // Highlight the placeholder if it was added.
    int error_start = target_entry->get_text().find(placeholder);
    if (error_start != std::string::npos) {
        target_entry->select_region(error_start, error_start + 1);
        target_entry->set_tooltip_text(
            _("BlueValues come in pairs. Select another."));

        histogram_.get_window()->set_cursor(
            Gdk::Cursor::create(histogram_.get_display(), "crosshair"));
    }
}

bool ShowHistogramDlg::dict_value_verifier(const Glib::ustring& text,
                                           int& start_pos, int& end_pos) {
    parse_values(text, &start_pos, &end_pos);
    return start_pos == -1 && end_pos == -1;
}

std::optional<PrivateDictValues> ShowHistogramDlg::show(
    GWindow parent, const HistogramData& data) {
    ShowHistogramDlg dialog(parent, data);

    if (dialog.run() == Gtk::RESPONSE_OK) {
        PrivateDictValues result;
        result.primary = dialog.primary_entry_.get_text();
        result.secondary = dialog.secondary_entry_.get_text();
        return result;
    } else {
        return {};
    }
}

std::optional<PrivateDictValues> show_histogram_dialog(
    GWindow parent, const HistogramData& data) {
    ff::app::GtkApp();
    return ShowHistogramDlg::show(parent, data);
}

}  // namespace ff::dlg
