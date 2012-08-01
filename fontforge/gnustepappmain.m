/* Copyright (C) 2003-2012 by George Williams */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.

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

#import <Foundation/Foundation.h>
#undef _
#include "fontforge.h"
#include "fontforgeui.h"

static void early_abort(void) {    
    fprintf(stderr, "\nFailure during start-up. Perhaps you are out of memory?\n");
    abort();
}

static void sharedir_not_found(void) {
    fprintf(stderr, "\nCannot find \"share\" in the application's resources.\n"
            "Maybe the FontForgeApp installation is corrupted.\n\n");
    abort();
}

int main( int argc, char **argv ) {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    if (!pool)
        early_abort();

    NSString *sharedir = [[NSBundle mainBundle] pathForResource:@"share" ofType:@""];
    if (!sharedir)
        sharedir_not_found();
    NSString *pixmaps =
        [NSString pathWithComponents:[NSArray arrayWithObjects:sharedir, @"pixmaps", (void*)nil]];
    if (!pixmaps)
        early_abort();    
    NSString *htdocs =
        [NSString pathWithComponents:[NSArray arrayWithObjects:sharedir, @"doc", @"fontforge", (void*)nil]];
    if (!htdocs)
        early_abort();

    // FIXME: It would be nice to have the ability to do locales as
    // add-on bundles. This may be especially so for key remappings --
    // assuming we keep the current scheme for those. (Whatever scheme
    // we did adopt should include the undocumented support for
    // Barry’s special hand-disability remappings, of course. :) )
    NSString *localedir =
        [NSString pathWithComponents:[NSArray arrayWithObjects:sharedir, @"locale", (void*)nil]];
    if (!localedir)
        early_abort();

    gnustep_resources_sharedir = strdup([sharedir UTF8String]);
    if (!gnustep_resources_sharedir)
        early_abort();
    gnustep_resources_pixmaps = strdup([pixmaps UTF8String]);
    if (!gnustep_resources_pixmaps)
        early_abort();
    gnustep_resources_htdocs = strdup([htdocs UTF8String]);
    if (!gnustep_resources_htdocs)
        early_abort();
    gnustep_resources_localedir = strdup([localedir UTF8String]);
    if (!gnustep_resources_localedir)
        early_abort();

    int retval = fontforge_main( argc, argv );

    free(gnustep_resources_localedir);
    free(gnustep_resources_htdocs);
    free(gnustep_resources_pixmaps);
    free(gnustep_resources_sharedir);

    [pool drain];

    return retval;
}
