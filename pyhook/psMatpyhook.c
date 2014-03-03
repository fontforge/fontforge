#include <fontforge-config.h>

#include <Python.h>

extern PyMODINIT_FUNC fontforge_python_init(const char* modulename);


/* Python 3 module initialization */
PyMODINIT_FUNC PyInit_psMat(void);
PyMODINIT_FUNC PyInit_psMat(void) {
    return fontforge_python_init("psMat");
}
