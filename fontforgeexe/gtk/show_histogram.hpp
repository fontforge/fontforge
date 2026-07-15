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
#pragma once

#include "dialog_base.hpp"
#include "show_histogram_shim.hpp"
#include "widgets/histogram.hpp"
#include "widgets/verified_entry.hpp"

namespace ff::dlg {

class ShowHistogramDlg final : public DialogBase {
 private:
    ShowHistogramDlg(GWindow parent, const HistogramData& data);

    Gtk::Box* build_control_box();
    Gtk::Box* build_entry_box(const Glib::ustring& label,
                              const Glib::ustring& value,
                              widgets::VerifiedEntry& entry,
                              Glib::RefPtr<Gtk::SizeGroup> label_group);

    // bar_index is the actual bar X-value, not the index in the bars vector.
    std::string get_tooltip_text(int bar_index, double average) const;
    void on_stem_bar_click(int bar_index, bool shift_pressed);
    void on_blues_bar_click(int bar_index, bool /*shift_pressed*/);

    HistogramData data_;
    unsigned int max_value_ = 0;

    widgets::Histogram histogram_;
    Gtk::SpinButton average_entry_;
    widgets::VerifiedEntry primary_entry_;
    widgets::VerifiedEntry secondary_entry_;

    static bool dict_value_verifier(const Glib::ustring& text, int& start_pos,
                                    int& end_pos);

 public:
    static std::optional<PrivateDictValues> show(GWindow parent,
                                                 const HistogramData& data);
};

}  // namespace ff::dlg
