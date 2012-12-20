#ifdef __Mac

#ifdef __GNUC_STDC_INLINE__
#undef __GNUC_STDC_INLINE__
#endif

#define FontInfo MacFontInfo
#define KernPair MacKernPair

#include <Carbon/Carbon.h>

#undef FontInfo
#undef KernPair



#endif /* __Mac */

