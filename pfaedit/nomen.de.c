#include "nomen.h"

static enum encoding enc = e_iso8859_1;

static char str_Language[] = "Deutsch";
static char str_Cancel[] = "Abbrechen";
static unichar_t mnemonic_Cancel[] = 'A';
static char str_Open[] = "Öffenen";
static unichar_t mnemonic_Open[] = 'f';
static char str_Save[] = "Speichem";
static unichar_t mnemonic_Save[] = 'S';
static char str_New[] = "Neu";
static unichar_t mnemonic_New[] = 'N';
static char str_File[] = "Datei";
static unichar_t mnemonic_File[] = 'D';
static char str_Edit[] = "Bearbeiten";
static unichar_t mnemonic_Edit[] = 'B';
static char str_View[] = "Ansicht";
static unichar_t mnemonic_View[] = 'A';
static char str_Help[] = "Hilfe";
static unichar_t mnemonic_Help[] = 'H';
static char str_Print[] = "Drucken...";
static unichar_t mnemonic_Print[] = 'D';
static char str_Revertfile[] = "Neu laden";
static unichar_t mnemonic_Revertfile[] = 'l';
static char str_Saveas[] = "Speichem unter...";
static unichar_t mnemonic_Saveas[] = 'u';
static char str_Close[] = "Schließen";
static unichar_t mnemonic_Close[] = 'c';
static char str_Prefs[] = "Einstellungen...";
static unichar_t mnemonic_Prefs[] = 'E';
static char str_Quit[] = "Beenden";
static unichar_t mnemonic_Quit[] = 'B';

static char str_Undo[] = "Rückgängig";
static unichar_t mnemonic_Undo[] = 'R';
static char str_Redo[] = "Wiederstellen";
static unichar_t mnemonic_Redo[] = 'W';
static char str_Cut[] = "Ausschneiden";
static unichar_t mnemonic_Cut[] = 'A';
static char str_Copy[] = "Kopieren";
static unichar_t mnemonic_Copy[] = 'K';
static char str_Paste[] = "Einfügen";
static unichar_t mnemonic_Paste[] = 'f';
static char str_Selectall[] = "Alles auswählen";
static unichar_t mnemonic_Selectall[] = 'n';

static char str_Allfonts[] = "Alles Schriftart";
static unichar_t mnemonic_Allfonts[] = 'A';
static char str_Fontmodifiers[] = "Schriftschnitt:";
static unichar_t mnemonic_Fontmodifiers[] = 'S';
static char str_Fontmodifiers[] = "Font Modifiers:";
static unichar_t mnemonic_Fontmodifiers[] = 'M';
static char *str_Styles = "Schriftschnitt";

static char str_Yes[] = "Ja";
static unichar_t mnemonic_Yes[] = 'J';
static char str_No[] = "Nein";
static unichar_t mnemonic_No[] = 'N';

static int num_buttonsize = 60;
