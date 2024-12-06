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

#include "kerning_format.hpp"

#include <iostream>

#include "intl.h"
#include "utils.hpp"

namespace ff::dlg {

KerningFormat::KerningFormat(std::shared_ptr<FVContext> context1,
                             std::shared_ptr<FVContext> context2, int width,
                             int height)
    : Dialog(), char_grid1(context1), char_grid2(context2) {
    dialog.set_title(_("Kerning format"));

    individual_pairs = Gtk::RadioButton(_("Use individual kerning pairs"));
    individual_pairs.set_tooltip_text(
        _("In this format you specify every kerning pair in which\n"
          "you are interested in."));

    Gtk::RadioButton::Group gr = individual_pairs.get_group();
    matrix_of_classes =
        Gtk::RadioButton(gr, _("Use a matrix of kerning classes"));
    matrix_of_classes.set_tooltip_text(
        _("In this format you define a series of glyph classes and\n"
          "specify a matrix showing how each class interacts with all\n"
          "the others."));
    matrix_of_classes.signal_toggled().connect([this]() {
        class_options_grid.set_sensitive(matrix_of_classes.get_active());
    });

    guess_classes = Gtk::CheckButton(
        _("FontForge will guess kerning classes for selected glyphs"));
    guess_classes.set_tooltip_text(
        _("FontForge will look at the glyphs selected in the font view\n"
          "and will try to find groups of glyphs which are most alike\n"
          "and generate kerning classes based on that information."));

    intra_class_dist_entry =
        widgets::NumericalEntry(_("Intra Class Distance:"));
    intra_class_dist_entry.set_tooltip_text(
        _("This is roughly (very roughly) the number off em-units\n"
          "of error that two glyphs may have to belong in the same\n"
          "class. This error is taken by comparing the two glyphs\n"
          "to all other glyphs and summing the differences.\n"
          "A small number here (like 2) means lots of small classes,\n"
          "while a larger number (like 20) will mean fewer classes,\n"
          "each with more glyphs."));

    class_options_grid.attach(guess_classes, 0, 0);
    class_options_grid.attach(intra_class_dist_entry, 0, 1);

    class_options_grid.set_sensitive(false);
    class_options_grid.set_halign(Gtk::ALIGN_START);

    default_separation =
        widgets::NumericalEntry(_("_Default Separation:"), true);
    default_separation.set_tooltip_text(
        _("Add entries to the lookup trying to make the optical\n"
          "separation between all pairs of glyphs equal to this\n"
          "value."));

    min_kern = widgets::NumericalEntry(_("_Min Kern:"), true);
    min_kern.set_tooltip_text(
        _("Any computed kerning change whose absolute value is less\n"
          "that this will be ignored."));

    touching = Gtk::CheckButton(_("_Touching"), true);
    touching.set_tooltip_text(
        "Normally kerning is based on achieving a constant (optical)\n"
        "separation between glyphs, but occasionally it is desirable\n"
        "to have a kerning table where the kerning is based on the\n"
        "closest approach between two glyphs (So if the desired separ-\n"
        "ation is 0 then the glyphs will actually be touching.");

    kern_closer = Gtk::CheckButton(_("Only kern glyphs closer"));
    kern_closer.set_tooltip_text(
        _("When doing autokerning, only move glyphs closer together,\n"
          "so the kerning offset will be negative."));

    autokern_new = Gtk::CheckButton(_("Autokern new entries"));
    autokern_new.set_tooltip_text(
        _("When adding new entries provide default kerning values."));

    general_options_grid.attach(default_separation, 0, 0);
    general_options_grid.attach(min_kern, 1, 0);
    general_options_grid.attach(touching, 2, 0);
    general_options_grid.attach(kern_closer, 0, 1);
    general_options_grid.attach(autokern_new, 1, 1, 2, 1);
    general_options_grid.set_column_spacing(ui_font_em_size());
    general_options_grid.set_margin_top(ui_font_em_size());
    general_options_grid.set_margin_bottom(ui_font_em_size());

    frame1 = Gtk::Frame(_("Select glyphs for the first part of the kern pair"));
    frame1.add(char_grid1.get_top_widget());
    frame1.set_label_align(0.2, 0.6);

    frame2 =
        Gtk::Frame(_("Select glyphs for the second part of the kern pair"));
    frame2.add(char_grid2.get_top_widget());
    frame2.set_label_align(0.2, 0.6);
    frame2.set_margin_top(ui_font_em_size());

    dialog.get_content_area()->pack_start(individual_pairs, Gtk::PACK_SHRINK);
    dialog.get_content_area()->pack_start(matrix_of_classes, Gtk::PACK_SHRINK);
    dialog.get_content_area()->pack_start(class_options_grid, Gtk::PACK_SHRINK);
    dialog.get_content_area()->pack_start(general_options_grid,
                                          Gtk::PACK_SHRINK);
    dialog.get_content_area()->pack_start(frame1);
    dialog.get_content_area()->pack_start(frame2);

    dialog.add_button(_("_OK"), Gtk::RESPONSE_OK);
    dialog.add_button(_("_Cancel"), Gtk::RESPONSE_CANCEL);

    // dialog.resize() doesn't work until after the realization, i.e. after
    // dialog.show_all(). Use the realize event to ensure reliable resizing.
    //
    // Also, the signal itself must be connected before dialog.show_all(),
    // otherwise it wouldn't work for some reason...
    dialog.signal_realize().connect([this, width, height]() {
        // Request to resize only the first character grid is misleading. In
        // practice the requested height would be divided equally between both
        // grids.
        char_grid1.resize_drawing_area(width, height);
    });

    dialog.show_all();

    // Indent kerning class options to relate them visually to the "matrix of
    // classes" check button.
    class_options_grid.set_margin_start(class_options_grid.get_margin_start() +
                                        label_offset(&matrix_of_classes));
}

Gtk::ResponseType KerningFormat::run(KFDlgData* kf_data) {
    // Initialize dialog values
    kf_data->use_individual_pairs ? individual_pairs.set_active()
                                  : matrix_of_classes.set_active();
    guess_classes.set_active(kf_data->guess_kerning_classes);
    intra_class_dist_entry.set_value(kf_data->intra_class_dist);
    default_separation.set_value(kf_data->default_separation);
    min_kern.set_value(kf_data->min_kern);
    touching.set_active(kf_data->touching);
    kern_closer.set_active(kf_data->kern_closer);
    autokern_new.set_active(kf_data->autokern_new);

    Gtk::ResponseType result = Dialog::run();

    // Read updated dialog values
    kf_data->use_individual_pairs = individual_pairs.get_active();
    kf_data->guess_kerning_classes = guess_classes.get_active();
    kf_data->intra_class_dist = intra_class_dist_entry.get_value();
    kf_data->default_separation = default_separation.get_value();
    kf_data->min_kern = min_kern.get_value();
    kf_data->touching = touching.get_active();
    kf_data->kern_closer = kern_closer.get_active();
    kf_data->autokern_new = autokern_new.get_active();

    return result;
}

}  // namespace ff::dlg
