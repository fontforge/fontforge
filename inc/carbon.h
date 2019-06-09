#ifndef FONTFORGE_CARBON_H
#define FONTFORGE_CARBON_H

#ifdef __Mac

#ifdef __GNUC_STDC_INLINE__
#undef __GNUC_STDC_INLINE__
#endif

#define AnchorPoint MacAnchorPoint
#define FontInfo    MacFontInfo
#define KernPair    MacKernPair

#include <Carbon/Carbon.h>

#undef FontInfo
#undef KernPair
#undef AnchorPoint


#endif /* __Mac */

#endif /* FONTFORGE_CARBON_H */
