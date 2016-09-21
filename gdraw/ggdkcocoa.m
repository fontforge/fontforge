/* Copyright (C) 2016 by Jeremy Tan */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file ggdkcocoa.m
 * @brief macOS specific functions, written in Objective C.
 */

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
