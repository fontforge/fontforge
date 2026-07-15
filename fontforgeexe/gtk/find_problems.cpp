/* Copyright (C) 2025 by Maxim Iorsh <iorsh@users@sourceforge.net>
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

#include "find_problems.hpp"

#include "application.hpp"
#include "utils.hpp"
#include "intl.h"

namespace ff::dlg {

FindProblemsDlg::FindProblemsDlg(GWindow parent,
                                 const std::vector<ProblemTab>& pr_tabs,
                                 double near)
    : DialogBase(parent) {
    set_title(_("Find Problems"));
    set_resizable(false);
    set_help_context("ui/dialogs/problems.html");

    auto tabs = build_notebook(pr_tabs);
    tabs->set_scrollable();
    get_content_area()->pack_start(*tabs);

    auto button_box = Gtk::make_managed<Gtk::HBox>();
    auto clear_all_button = Gtk::make_managed<Gtk::Button>(_("Clear All"));
    auto set_all_button = Gtk::make_managed<Gtk::Button>(_("Set All"));

    clear_all_button->signal_clicked().connect(sigc::bind(
        sigc::mem_fun(*this, &FindProblemsDlg::set_all_checkboxes), false));
    set_all_button->signal_clicked().connect(sigc::bind(
        sigc::mem_fun(*this, &FindProblemsDlg::set_all_checkboxes), true));

    button_box->pack_start(*clear_all_button, Gtk::PACK_SHRINK);
    button_box->pack_start(*set_all_button, Gtk::PACK_SHRINK);
    get_content_area()->pack_start(*button_box);

    get_content_area()->pack_start(*Gtk::make_managed<Gtk::HSeparator>(),
                                   Gtk::PACK_SHRINK, 5);

    auto near_value_box = Gtk::make_managed<Gtk::HBox>();
    near_value_entry_.set_width_chars(6);
    near_value_entry_.set_value(near);
    near_value_box->pack_start(
        *Gtk::make_managed<Gtk::Label>(U_("¹ \"Near\" means within")),
        Gtk::PACK_SHRINK);
    near_value_box->pack_start(near_value_entry_, Gtk::PACK_SHRINK);
    near_value_box->pack_start(*Gtk::make_managed<Gtk::Label>(_("em-units")),
                               Gtk::PACK_SHRINK);
    get_content_area()->pack_start(*near_value_box);

    show_all();
}

Gtk::HBox* FindProblemsDlg::build_record_box(
    const ProblemRecord& record, Glib::RefPtr<Gtk::SizeGroup> size_group) {
    auto record_box = Gtk::make_managed<Gtk::HBox>();
    bool disabled = record.disabled;

    Gtk::CheckButton record_check(record.label, true);
    record_check.set_tooltip_text(record.tooltip);
    record_check.set_active(record.active);
    if (size_group) {
        size_group->add_widget(record_check);
    }

    // This notation of parent is unrelated to parent/child widgets. It
    // denotes problem records which logically depend on other records,
    // and cannot be selected without their parent.
    if (record.parent_cid != 0) {
        Gtk::CheckButton& parent_check = widget_map_[record.parent_cid].first;
        record_check.set_margin_start(2 * ff::ui_utils::ui_font_em_size());
        disabled |= !parent_check.get_active();
        parent_check.signal_toggled().connect([&parent_check, record_box]() {
            record_box->set_sensitive(parent_check.get_active());
        });
    }

    record_box->pack_start(record_check, Gtk::PACK_SHRINK);
    record_box->set_sensitive(!disabled);

    widgets::NumericalEntry* record_entry = nullptr;
    if (!std::holds_alternative<std::monostate>(record.value)) {
        if (std::holds_alternative<int>(record.value)) {
            auto int_entry = Gtk::make_managed<widgets::IntegerEntry>();
            int_entry->set_value(std::get<int>(record.value));
            record_entry = int_entry;
        } else {
            auto double_entry = Gtk::make_managed<widgets::DoubleEntry>();
            double_entry->set_value(std::get<double>(record.value));
            record_entry = double_entry;
        }
        record_entry->set_width_chars(6);
        record_entry->set_valign(Gtk::ALIGN_END);
        record_entry->set_vexpand(false);
        record_box->pack_start(*record_entry, Gtk::PACK_SHRINK);
    }

    widget_map_.emplace(record.cid,
                        std::make_pair(std::move(record_check), record_entry));

    return record_box;
}

Gtk::Notebook* FindProblemsDlg::build_notebook(
    const std::vector<ProblemTab>& pr_tabs) {
    auto tabs = Gtk::make_managed<Gtk::Notebook>();

    for (const ProblemTab& tab : pr_tabs) {
        auto record_page = Gtk::make_managed<Gtk::VBox>();

        // If ALL the records on this page have numerical entry, we want them
        // neatly aligned.
        bool align_entries = !std::any_of(
            tab.records.cbegin(), tab.records.cend(), [](const auto& rec) {
                return std::holds_alternative<std::monostate>(rec.value);
            });
        auto width_group =
            align_entries ? Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL)
                          : (Glib::RefPtr<Gtk::SizeGroup>)nullptr;

        auto height_group = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_VERTICAL);
        for (const ProblemRecord& record : tab.records) {
            Gtk::HBox* record_box = build_record_box(record, width_group);
            height_group->add_widget(*record_box);

            record_page->pack_start(*record_box, Gtk::PACK_SHRINK);
        }
        tabs->append_page(*record_page, tab.label);
    }
    return tabs;
}

void FindProblemsDlg::set_all_checkboxes(bool status) {
    for (auto& [cid, widgets] : widget_map_) {
        widgets.first.set_active(status);
    }
}

ProblemRecordsOut FindProblemsDlg::show(GWindow parent,
                                        const std::vector<ProblemTab>& pr_tabs,
                                        double& near) {
    FindProblemsDlg dialog(parent, pr_tabs, near);

    Gtk::ResponseType result = dialog.run();
    ProblemRecordsOut records_out;

    if (result != Gtk::RESPONSE_OK) {
        // Return empty result whenever the user canceled the dialog.
        return records_out;
    }

    for (const ProblemTab& tab : pr_tabs) {
        for (const ProblemRecord& record : tab.records) {
            const auto& [checkbox, entry] = dialog.widget_map_[record.cid];
            if (!checkbox.get_active()) continue;

            NumericalValue new_value;
            if (entry) {
                new_value = entry->get_num_value();
            }
            records_out.emplace(record.cid, new_value);
        }
    }
    near = dialog.near_value_entry_.get_value();

    return records_out;
}

bool find_problems_dialog(GWindow parent, std::vector<ProblemTab>& pr_tabs,
                          double& near) {
    // To avoid instability, the GTK application is lazily initialized only when
    // a GTK window is invoked.
    ff::app::GtkApp();

    ff::dlg::ProblemRecordsOut result =
        ff::dlg::FindProblemsDlg::show(parent, pr_tabs, near);

    for (ProblemTab& tab : pr_tabs) {
        for (ProblemRecord& rec : tab.records) {
            rec.active = result.count(rec.cid);
            if (rec.active) {
                rec.value = result[rec.cid];
            }
        }
    }

    return (!result.empty());
}

}  // namespace ff::dlg
