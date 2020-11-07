#!/usr/bin/env python3

from PIL import Image
import sys
import textwrap

TEMPLATE = """/* Copyright (C) 2000-2012 by George Williams 
 * Image replaced in 2019 by @ctrlcctrlv (Fred Brennan) */
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

#include <fontforge-config.h>

#include "gimage.h"

static uint8 splashimage0_data[] = {{
{data}
}};

static struct _GImage splashimage0_base = {{
    it_true,
    0,
    {width},
    {height},
    {bytes_per_line},
    (uint8 *) splashimage0_data,
    NULL,
    0xffffffff
}};

GImage splashimage = {{ 0, {{ &splashimage0_base }}, NULL }};
"""

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: {} input.png".format(sys.argv[0]))
        sys.exit(-1)

    img = Image.open(sys.argv[1]).convert("RGB")
    data = ", ".join(
        "0x{:02x}, 0x{:02x}, 0x{:02x}, 0xff".format(b, g, r)
        for r, g, b in img.getdata()
    )
    wrapper = textwrap.TextWrapper(
        width=79,
        break_long_words=False,
    )
    data = "\n".join(wrapper.wrap(data))

    with open("splashimage.c", "w") as fp:
        fp.write(
            TEMPLATE.format(
                data=data,
                width=img.width,
                height=img.height,
                bytes_per_line=img.width * 4,
            )
        )
