/* Vague support for two different shared library formats */
/* Everyone but the mac uses a varient of dlopen, while the mac uses NSAddImage */
#ifndef __DYNAMIC_H
# define __DYNAMIC_H

#  ifdef __Mac
#   include <mach-o/dyld.h>
#   define SO_EXT	".dylib"
/*   man NSModule */
#   define dlopen(name,foo) NSAddImage(name,NSADDIMAGE_OPTION_WITH_SEARCHING|NSADDIMAGE_OPTION_RETURN_ON_ERROR)
/*   It would have been nice if the Mac's docs had mentioned that the linker adds*/
/*   an underscore to symbol names.... */
#   define dlsym(image,symname) NSAddressOfSymbol(NSLookupSymbolInImage(image,"_" symname,NSLOOKUPSYMBOLINIMAGE_OPTION_BIND|NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR))
#   define DL_CONST	const
#   define dlclose(image-ptr)	/* Don't know how to do this on mac */
#   define dlerror()		"Error when loading dynamic library"
#  else
#   include <dlfcn.h>
#   define SO_EXT	".so"
#   define DL_CONST
#  endif

#endif
