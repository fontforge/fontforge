#include <Python.h>

#if PY_MAJOR_VERSION >= 3
#define FFPY_PYTHON_ENTRY_FUNCTION fontforge_python3_init
#else
#define FFPY_PYTHON_ENTRY_FUNCTION fontforge_python2_init
#endif
extern PyMODINIT_FUNC FFPY_PYTHON_ENTRY_FUNCTION(const char* modulename);


/**
 * These only need to be here because some of the code in python.c calls
 * these functions, if that code is moved to python-ff.c and only compiled into
 * a gui fontforge exe then we can remove these down to the next *** block
 */
typedef struct fontview {
} FontView;
int _FVMenuSaveAs(FontView *fv);
int _FVMenuSaveAs(FontView *fv) {
    return 0;
}
typedef struct charview CharView;
typedef struct splinechar SplineChar;
CharView *CharViewCreate(SplineChar *sc,FontView *fv,int enc);
CharView *CharViewCreate(SplineChar *sc,FontView *fv,int enc)
{
    return 0;
}
void MacServiceReadFDs(void);
void MacServiceReadFDs(void)
{
}

/*********************************************/
/*********************************************/
/*********************************************/


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
    return FFPY_PYTHON_ENTRY_FUNCTION("fontforge");
}
#endif
