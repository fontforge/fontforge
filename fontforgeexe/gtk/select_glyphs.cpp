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

#include "select_glyphs.hpp"

#include "intl.h"
#include "utils.hpp"

namespace ff::dlg {

SelectGlyphs::SelectGlyphs(std::shared_ptr<FVContext> context, int width,
                           int height)
    : Dialog(), fv_context(context), char_grid(context) {
    dialog.set_title(_("Glyph Set by Selection"));

    explanation.set_text(
        _("Select glyphs in the font view above.\nThe selected glyphs "
          "become your glyph class."));
    explanation.set_halign(Gtk::ALIGN_START);

    dialog.get_content_area()->pack_start(char_grid.get_top_widget());
    dialog.get_content_area()->pack_end(explanation, Gtk::PACK_SHRINK);

    dialog.add_button(_("_OK"), Gtk::RESPONSE_OK);
    dialog.add_button(_("_Cancel"), Gtk::RESPONSE_CANCEL);

    dialog.show_all();
    dialog.resize(width, height);
}

}  // namespace ff::dlg
