#include <fontforge-config.h>

#include <Python.h>

extern PyMODINIT_FUNC fontforge_python_init(const char* modulename);

/* Returns 1 when running as standalone Python module (pyhook) */
int ff_is_pyhook_context(void) {
    return 1;
}

PyMODINIT_FUNC PyInit_fontforge(void) {
    return fontforge_python_init("fontforge");
}
