#include <Python.h>
#include <splinefont.h>

PyMODINIT_FUNC initfontforge(void);

PyMODINIT_FUNC initfontforge(void) {
    ff_init();
}
