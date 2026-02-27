#include <fontforge-config.h>

#include <Python.h>

extern PyMODINIT_FUNC fontforge_python_init(const char* modulename);

#if !defined(_WIN32)
/* Strong symbol to override weak symbol in libfontforge on Linux/macOS */
int ff_is_pyhook_context(void) {
    return 1;
}
#endif

PyMODINIT_FUNC PyInit_fontforge(void) {
    return fontforge_python_init("fontforge");
}
