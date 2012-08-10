#include <Python.h>
#include <dynamic.h>

#if defined(__Mac)
# undef dlopen
#endif

PyMODINIT_FUNC ENTRY_POINT(void) {
    DL_CONST void *lib;
    void (*initer)(void);

    if ( (lib = dlopen("libgunicode" SO_EXT,RTLD_LAZY))==NULL ) {
#ifdef LIBDIR
	lib = dlopen( LIBDIR "libgunicode" SO_EXT,RTLD_LAZY);
#endif
    }
    if ( lib==NULL ) {
	PyErr_Format(PyExc_SystemError,"Missing library: %s", "libgunicode");
return;
    }

    if ( (lib = dlopen("libgutils" SO_EXT,RTLD_LAZY))==NULL ) {
#ifdef LIBDIR
	lib = dlopen( LIBDIR "libgutils" SO_EXT,RTLD_LAZY);
#endif
    }
    if ( lib==NULL ) {
	PyErr_Format(PyExc_SystemError,"Missing library: %s", "libgutils");
return;
    }

    if ( (lib = dlopen("libfontforge" SO_EXT,RTLD_LAZY))==NULL ) {
#ifdef LIBDIR
	lib = dlopen( LIBDIR "/lib/" "libfontforge" SO_EXT,RTLD_LAZY);
#endif
    }
    if ( lib==NULL ) {
	PyErr_Format(PyExc_SystemError,"Missing library: %s", "libfontforge");
return;
    }
    initer = dlsym(lib,"ff_init");
    if ( initer==NULL ) {
	PyErr_Format(PyExc_SystemError,"No initialization function in fontforge library");
return;
    }
    (*initer)();
}
