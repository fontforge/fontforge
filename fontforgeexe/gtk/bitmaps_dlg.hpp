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
#pragma once

#include "dialog.hpp"

#include "simple_dialogs.hpp"
#include "widgets/verified_entry.hpp"

namespace ff::dlg {

using BitmapSize = std::pair<uint16_t /*size*/, uint16_t /*depth*/>;
using BitmapSizes = std::vector<BitmapSize>;

class BitmapsDlg final : public Dialog {
 public:
    BitmapsDlg(GWindow parent, BitmapsDlgMode mode, const BitmapSizes& sizes,
               bool bitmaps_only, bool has_current_char);

    bool show();
    BitmapSizes get_sizes() const;
    bool get_rasterize() const;
    Glib::ustring get_active_scope() const;

 private:
    Gtk::ComboBoxText glyphs_combo_;
    widgets::VerifiedEntry pixels_entry_;
    Gtk::CheckButton rasterize_check_;

    Gtk::ComboBoxText build_glyphs_combo(bool has_current_char) const;

    static bool pixel_size_verifier(const Glib::ustring& text, int& start_pos,
                                    int& end_pos);
};

}  // namespace ff::dlg
