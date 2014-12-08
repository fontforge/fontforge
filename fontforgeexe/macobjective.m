/* Copyright (C) 2000-2012 by George Williams */
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

#include "macobjective.h"
#include "fontforge-config.h"

#ifdef USE_BREAKPAD

static BreakpadRef InitBreakpad(void) {
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  BreakpadRef breakpad = 0;
  NSDictionary *plist = [[NSBundle mainBundle] infoDictionary];
printf("xxx initBreakpad\n\n\n\n");
  if (plist) {
    // Note: version 1.0.0.4 of the framework changed the type of the argument 
    // from CFDictionaryRef to NSDictionary * on the next line:
printf("xxx initBreakpad 0 plist:%p\n", plist );
    breakpad = BreakpadCreate(plist);
printf("xxx initBreakpad  2\n");
printf("xxx initBreakpad 3  %p\n", breakpad );
  }
  [pool release];
  return breakpad;
}

@implementation BreakpadTest

- (void)awakeFromNib {
  breakpad = InitBreakpad();
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender {
printf("fdfdsfdfdsfdsfdsssssssssfdsfsd\n");
  BreakpadRelease(breakpad);
  return NSTerminateNow;
}

@end

@implementation MyApplication

- (void)awakeFromNib {
printf("awakeFromNib...\n\n");
  breakpad = InitBreakpad();
}



- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender {
  BreakpadRelease(breakpad);
  return NSTerminateNow;
}

@end


void setup_cocoa_app() 
{
printf("setup_cocoa_app()\n\n");
    [MyApplication sharedApplication];
  BreakpadTest* bp = [BreakpadTest alloc];
[bp awakeFromNib];
  //  [NSApplication sharedApplication];
}

#else

void setup_cocoa_app()
{
    [NSApplication sharedApplication];
}

#endif
