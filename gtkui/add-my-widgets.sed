/#include "support.h"/ a \
#include "gwwv.h" \
#include <libintl.h> \
#define _(str) gettext(str)
s/gtk_menu_bar_new/gww_vmenu_bar_new/
s/return CharViewContents;/return table13;/
s/GLADE_HOOKUP_OBJECT_NO_REF (CharViewContents, CharViewContents/GLADE_HOOKUP_OBJECT_NO_REF (table13, table13/
s/GLADE_HOOKUP_OBJECT (CharViewContents, table13, "table13")//
s/GLADE_HOOKUP_OBJECT (CharViewContents, /GLADE_HOOKUP_OBJECT (table13, /
s/create_pixmap (CharViewContents/create_pixmap (CharViewContainer/
s/create_CharViewContents (void)/create_CharViewContained ( GtkWidget *CharViewContainer )/
/CharViewContents/ d
