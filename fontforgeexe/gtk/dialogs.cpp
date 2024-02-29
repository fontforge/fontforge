/* Copyright 2023 Joey Sabey <github.com/Omnikron13>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with     •
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED “AS IS” AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <gtkmm.h>
#include <iostream>
#include "dialogs.hpp"
#include "intl.h"

// A simple dialog to query the user for a number of new encoding slots to add.
class AddEncodingSlotsDialog final : public Gtk::Dialog {
  public:
   Gtk::SpinButton *input;

   AddEncodingSlotsDialog() {
      set_title(_("Add Encoding Slots..."));

      auto label = Gtk::make_managed<Gtk::Label>(_("How many slots do you wish to add?"));
      get_content_area()->pack_start(*label);

      input = Gtk::make_managed<Gtk::SpinButton>();
      input->set_numeric(true);
      input->set_adjustment(Gtk::Adjustment::create(1, 1, 100, 1, 5, 0));
      get_content_area()->pack_start(*input);

      this->add_button(_("_Add"), Gtk::RESPONSE_OK);
      this->add_button(_("_Cancel"), Gtk::RESPONSE_CANCEL);

      get_content_area()->show_all();
   }

   int get_value() const {
      return input->get_value_as_int();
   }

   // Show the dialog and return either the entered integer, 0 if the dialog was cancelled/closed, or -1 if something strange happened
   static int show() {
      auto *dialog = new AddEncodingSlotsDialog();
      int i = -1;
      dialog->signal_response().connect([=, &i](int response_id){
         // Ok/Add returns the entered integer
         if(response_id == Gtk::RESPONSE_OK) {
            i = dialog->get_value();
         }
         // Cancel/close returns 0, which is semantically equivalent to the user actually entering 0
         else if(response_id == Gtk::RESPONSE_CANCEL || response_id == Gtk::RESPONSE_DELETE_EVENT || response_id == Gtk::RESPONSE_CLOSE) {
            i = 0;
         }
         // Any other response, which shouldn't really be possible, returns -1 to indicate an error
      });
      dialog->run();
      delete dialog;
      return i;
   }
};

// Shim for the C code to call the dialog
int add_encoding_slots_dialog() {
   return AddEncodingSlotsDialog::show();
}
