#include "nomen.h"

static enum encoding enc = e_iso8859_1;

static char str_Language[] = "Espanole";
static char str_OK[] = "OK";
static unichar_t mnemonic_OK[] = 'O';
static char str_Cancel[] = "Cancelar";
static unichar_t mnemonic_Cancel[] = 'C';
static char str_Open[] = "Abrir";
static unichar_t mnemonic_Open[] = 'A';
static char str_Save[] = "Guardar";
static unichar_t mnemonic_Save[] = 'G';
static char str_Filter[] = "Filtro";
static unichar_t mnemonic_Filter[] = 'F';
static char str_New[] = "Nuevo";
static unichar_t mnemonic_New[] = 'N';
/* Menus ... */
static char str_File[] = "Archivo";
static unichar_t mnemonic_File[] = 'A';
static char str_Edit[] = "Modificar";
static unichar_t mnemonic_Edit[] = 'M';
static char str_Print[] = "Imprimir...";
static unichar_t mnemonic_Print[] = 'I';
static char str_Saveas[] = "Guardar como...";
static unichar_t mnemonic_Saveas[] = 'c';
static char str_SaveAll[] = "Guardar todos";
static unichar_t mnemonic_SaveAll = 'T';
static char str_Close[] = "Cerrar";
static unichar_t mnemonic_Close[] = 'c';
static char str_Prefs[] = "Preferencias...";
static unichar_t mnemonic_Prefs[] = 'e';
static char str_Quit[] = "Salir";
static unichar_t mnemonic_Quit[] = 'S';
static char str_Goto[] = "Ir a";
static unichar_t mnemonic_Goto[] = 'I';
static char str_Undo[] = "Deshacer";
static unichar_t mnemonic_Undo[] = 'D';
static char str_Redo[] = "Rehacer";
static unichar_t mnemonic_Redo[] = 'R';
static char str_Cut[] = "Cortar";
static unichar_t mnemonic_Cut[] = 't';
static char str_Copy[] = "Copiar";
static unichar_t mnemonic_Copy[] = 'C';
static char str_Paste[] = "Pegar";
static unichar_t mnemonic_Paste[] = 'P';
static char str_CopyFgToBg[] = "Copiar Fg a Bg";
static unichar_t mnemonic_CopyFgToBg[] = 'F';
static char str_SelectAll[] = "Seleccionar todos";
static unichar_t mnemonic_SelectAll[] = 't';
static char str_SelectColor[] = "Seleccionar color";
static unichar_t mnemonic_SelectColor[] = 'S';

static int num_buttonsize = 70;
static int num_ScaleFactor = 130;
