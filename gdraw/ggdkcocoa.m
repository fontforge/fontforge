#include "ggdkdrawP.h"

#ifdef FONTFORGE_CAN_USE_GDK
#include "ustring.h"
#include <Cocoa/Cocoa.h>

char *_GGDKDrawCocoa_GetClipboardText(void) {
    NSString *ns_clip = [[NSPasteboard generalPasteboard] stringForType:NSStringPboardType];
    if (ns_clip == nil) {
        return NULL;
    }

    return copy([ns_clip UTF8String]);
}

void _GGDKDrawCocoa_SetClipboardText(const char *text) {
    if (text == NULL || text[0] == '\0') {
        return;
    }

    NSString *ns_clip = [[NSString alloc] initWithUTF8String:text];
    if (ns_clip == nil) {
        return;
    }

    NSPasteboard *pb = [NSPasteboard generalPasteboard];
    [pb declareTypes:[NSArray arrayWithObject:NSStringPboardType] owner:nil];
    [pb setString:ns_clip forType:NSStringPboardType];
    [ns_clip release];
}

#endif
