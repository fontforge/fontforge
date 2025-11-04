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

#include "intl.h"

namespace ff::dlg {

FindProblemsDlg::FindProblemsDlg(GWindow parent,
                                 const std::vector<ProblemTab>& pr_tabs)
    : Dialog(parent) {
    set_title(_("Find Problems"));
    set_resizable(false);
    // set_help_context("ui/dialogs/problems.html", nullptr);

    auto tabs = build_notebook(pr_tabs);
    get_content_area()->pack_start(*tabs);

    auto button_box = Gtk::make_managed<Gtk::HBox>();
    auto clear_all_button = Gtk::make_managed<Gtk::Button>(_("Clear All"));
    auto set_all_button = Gtk::make_managed<Gtk::Button>(_("Set All"));
    button_box->pack_start(*clear_all_button, Gtk::PACK_SHRINK);
    button_box->pack_start(*set_all_button, Gtk::PACK_SHRINK);
    get_content_area()->pack_start(*button_box);

    get_content_area()->pack_start(*Gtk::make_managed<Gtk::HSeparator>(),
                                   Gtk::PACK_SHRINK, 5);

    auto near_value_box = Gtk::make_managed<Gtk::HBox>();
    auto near_value_entry = Gtk::make_managed<Gtk::Entry>();
    near_value_entry->set_width_chars(6);
    near_value_box->pack_start(
        *Gtk::make_managed<Gtk::Label>(U_("¹ \"Near\" means within")),
        Gtk::PACK_SHRINK);
    near_value_box->pack_start(*near_value_entry, Gtk::PACK_SHRINK);
    near_value_box->pack_start(*Gtk::make_managed<Gtk::Label>(_("em-units")),
                               Gtk::PACK_SHRINK);
    get_content_area()->pack_start(*near_value_box);

    show_all();
}

Gtk::Notebook* FindProblemsDlg::build_notebook(
    const std::vector<ProblemTab>& pr_tabs) {
    auto tabs = Gtk::make_managed<Gtk::Notebook>();

    for (const ProblemTab& tab : pr_tabs) {
        auto record_page = Gtk::make_managed<Gtk::VBox>();
        for (const ProblemRecord& record : tab.records) {
            auto record_box = Gtk::make_managed<Gtk::HBox>();

            Gtk::CheckButton record_check(record.label, true);
            record_check.set_tooltip_text(record.tooltip);
            record_check.set_active(record.active);
            record_box->pack_start(record_check, Gtk::PACK_SHRINK);

            widgets::NumericalEntry* record_entry = nullptr;
            if (!std::holds_alternative<std::monostate>(record.value)) {
                if (std::holds_alternative<int>(record.value)) {
                    auto int_entry = Gtk::make_managed<widgets::IntegerEntry>();
                    int_entry->set_value(std::get<int>(record.value));
                    record_entry = int_entry;
                } else {
                    auto double_entry =
                        Gtk::make_managed<widgets::DoubleEntry>();
                    double_entry->set_value(std::get<double>(record.value));
                    record_entry = double_entry;
                }
                record_entry->set_width_chars(6);
                record_box->pack_start(*record_entry, Gtk::PACK_SHRINK);
            }

            record_page->pack_start(*record_box, Gtk::PACK_SHRINK);
            widget_map_.emplace(
                record.cid,
                std::make_pair(std::move(record_check), record_entry));
        }
        tabs->append_page(*record_page, tab.label);
    }
    return tabs;
}

ProblemRecordsOut FindProblemsDlg::show(
    GWindow parent, const std::vector<ProblemTab>& pr_tabs) {
    FindProblemsDlg dialog(parent, pr_tabs);

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

            ProblemRecordValue new_value = record.value;
            if (std::holds_alternative<int>(record.value)) {
                auto int_entry = dynamic_cast<widgets::IntegerEntry*>(entry);
                if (int_entry) {
                    new_value = int_entry->get_value();
                }
            } else if (std::holds_alternative<double>(record.value)) {
                auto double_entry = dynamic_cast<widgets::DoubleEntry*>(entry);
                if (double_entry) {
                    new_value = double_entry->get_value();
                }
            }
            records_out.emplace(record.cid, new_value);
        }
    }

    return records_out;
}

}  // namespace ff::dlg
