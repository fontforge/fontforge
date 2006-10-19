/* Vague support for several different shared library formats */
/* Everyone but the mac uses a varient of dlopen, while the mac uses NSAddImage */
/* Under cygwin the shared libs have very strange names which bare little */
/*  resemblance to what we expect. GNU creates a .la file that tells us what */
/*  the name should be, so indirect through that */
#ifndef __DYNAMIC_H
# define __DYNAMIC_H

#  ifdef __Mac
#   include <mach-o/dyld.h>
extern const void *gwwv_NSAddImage(char *name,uint32_t options);
#   define SO_EXT	".dylib"
/*   man NSModule */
#   define dlopen(name,foo) gwwv_NSAddImage(name,NSADDIMAGE_OPTION_WITH_SEARCHING|NSADDIMAGE_OPTION_RETURN_ON_ERROR)
/*   It would have been nice if the Mac's docs had mentioned that the linker adds*/
/*   an underscore to symbol names.... */
#   define dlsym(image,symname) NSAddressOfSymbol(NSLookupSymbolInImage(image,"_" symname,NSLOOKUPSYMBOLINIMAGE_OPTION_BIND|NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR))
#   define DL_CONST	const
#   define dlclose(image_ptr)	/* Don't know how to do this on mac */
#   define dlerror()		"Error when loading dynamic library"
#  else
#   include <dlfcn.h>
#   ifdef __CygWin
#    define dlopen(name,foo) libtool_laopen(name,foo)
void *libtool_laopen(const char *filename, int flags);
#   endif
#ifdef __VMS
# define SO_EXT	".exe"
#else
# define SO_EXT	".so"
#endif
#define DL_CONST
#  endif

#endif
