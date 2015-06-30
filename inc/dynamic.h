/* Vague support for several different shared library formats */
/* Everyone but the mac uses a varient of dlopen, while the mac uses NSAddImage */
/* Under cygwin the shared libs have very strange names which bare little */
/*  resemblance to what we expect. GNU creates a .la file that tells us what */
/*  the name should be, so indirect through that */
#ifndef __DYNAMIC_H
# define __DYNAMIC_H

#  if defined(__Mac)
/* In 10.3 the mac got normal dlopen routines */
#   include <dlfcn.h>
#   define SO_EXT	".dylib"
#   define SO_0_EXT	".0.dylib"
#   define SO_1_EXT	".1.dylib"
#   define SO_2_EXT	".2.dylib"
#   define SO_6_EXT	".6.dylib"
#   define DL_CONST	
#   define dlopen(name,foo) gwwv_dlopen(name,foo)
extern void *gwwv_dlopen(char *name,int flags);
#  elif defined(__Mac)
#   include <mach-o/dyld.h>
extern const void *gwwv_NSAddImage(char *name,uint32_t options);
#   define SO_EXT	".dylib"
#   define SO_0_EXT	".0.dylib"
#   define SO_1_EXT	".1.dylib"
#   define SO_2_EXT	".2.dylib"
#   define SO_6_EXT	".6.dylib"
/*   man NSModule */
#   define dlopen(name,foo) gwwv_NSAddImage(name,NSADDIMAGE_OPTION_WITH_SEARCHING|NSADDIMAGE_OPTION_RETURN_ON_ERROR)
/*   It would have been nice if the Mac's docs had mentioned that the linker adds*/
/*   an underscore to symbol names.... */
#   define dlsym(image,symname) NSAddressOfSymbol(NSLookupSymbolInImage(image,"_" symname,NSLOOKUPSYMBOLINIMAGE_OPTION_BIND|NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR))
#   define dlsymmod(symname) ("_" symname)
#   define dlsymbare(image,symname) NSAddressOfSymbol(NSLookupSymbolInImage(image,symname,NSLOOKUPSYMBOLINIMAGE_OPTION_BIND|NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR))
#   define DL_CONST	const
#   define dlclose(image_ptr)	/* Don't know how to do this on mac */
#   define dlerror()		"Error when loading dynamic library"
#  elif defined(__Mac)
#  elif defined(__MINGW32__)
#  else
#   include <dlfcn.h>
#define SO_EXT		".so"
#define SO_0_EXT	".so.0"
#define SO_1_EXT	".so.1"
#define SO_2_EXT	".so.2"
#define SO_6_EXT	".so.6"
#define DL_CONST
#  endif

# ifndef dlsymmod
#   define dlsymmod(symname) (symname)
#   define dlsymbare(image,symname) dlsym(image,symname)
# endif

#endif
