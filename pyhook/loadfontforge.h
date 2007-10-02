#include "Python.h"
#include "../inc/dynamic.h"
#ifdef __Mac
# undef dlopen
# define dlopen(name,foo) NSAddImage(name,NSADDIMAGE_OPTION_WITH_SEARCHING|NSADDIMAGE_OPTION_RETURN_ON_ERROR)
#endif

PyMODINIT_FUNC ENTRY_POINT(void) {
    DL_CONST void *lib;
    void (*initer)(void);

    if ( (lib = dlopen("libgunicode" SO_EXT,RTLD_LAZY))==NULL ) {
#ifdef PREFIX
	lib = dlopen( PREFIX "/lib/" "libgunicode" SO_EXT,RTLD_LAZY);
printf( "This: %s\n", PREFIX "/lib/" "libgunicode" SO_EXT );
#endif
    }
    if ( lib==NULL ) {
	PyErr_Format(PyExc_SystemError,"Missing library: %s", "libgunicode");
return;
    }

    if ( (lib = dlopen("libgdraw" SO_EXT,RTLD_LAZY))==NULL ) {
#ifdef PREFIX
	lib = dlopen( PREFIX "/lib/" "libgdraw" SO_EXT,RTLD_LAZY);
#endif
    }
    if ( lib==NULL ) {
	PyErr_Format(PyExc_SystemError,"Missing library: %s", "libgdraw");
return;
    }

    if ( (lib = dlopen("libfontforge" SO_EXT,RTLD_LAZY))==NULL ) {
#ifdef PREFIX
	lib = dlopen( PREFIX "/lib/" "libfontforge" SO_EXT,RTLD_LAZY);
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
