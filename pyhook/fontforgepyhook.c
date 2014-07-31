#include "fontforge-config.h"
#include <Python.h>

#if PY_MAJOR_VERSION >= 3
#define FFPY_PYTHON_ENTRY_FUNCTION fontforge_python3_init
#else
#define FFPY_PYTHON_ENTRY_FUNCTION fontforge_python2_init
#endif
extern PyMODINIT_FUNC FFPY_PYTHON_ENTRY_FUNCTION(const char* modulename);


#if PY_MAJOR_VERSION >= 3
/* Python 3 module initialization */
PyMODINIT_FUNC PyInit_fontforge(void);
PyMODINIT_FUNC PyInit_fontforge(void) {
    return FFPY_PYTHON_ENTRY_FUNCTION("fontforge");
}
#else
/* Python 2 module initialization */
PyMODINIT_FUNC initfontforge(void);
PyMODINIT_FUNC initfontforge(void) {
    FFPY_PYTHON_ENTRY_FUNCTION("fontforge");
}
#endif
