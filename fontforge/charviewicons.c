/* Copyright (C) 2000-2005 by George Williams */
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
#include "gdraw.h"
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
#include "views.h"

static GClut clut = { 2, 0, 1,
    0x0, 0xb0b0b0 };

static uint8 rightpointer0_data[] = {
    0xfb, 
    0xf3, 
    0xe3, 
    0xc3, 
    0x83, 
    0x3, 
    0xc3, 
    0xcb, 
    0x9b, 
    0x9f, 
};

static struct _GImage rightpointer0_base = {
    it_mono,
    2081,8,10,1,
    (uint8 *) rightpointer0_data,
    &clut,
    1
};

static uint8 sel2ptr0_data[] = {
    0xff, 0xe5, 
    0xff, 0x39, 
    0xf9, 0xf1, 
    0x8f, 0xe1, 
    0x77, 0xc1, 
    0x77, 0x81, 
    0x77, 0xe1, 
    0x8f, 0xe5, 
    0xff, 0xcd, 
    0xff, 0xcf, 
};

static struct _GImage sel2ptr0_base = {
    it_mono,
    2081,16,10,2,
    (uint8 *) sel2ptr0_data,
    &clut,
    1
};

static uint8 selectedpoint0_data[] = {
    0xdf, 
    0xef, 
    0xef, 
    0xc7, 
    0xbb, 
    0xbb, 
    0xbb, 
    0xc7, 
    0xef, 
    0xef, 
};

static struct _GImage selectedpoint0_base = {
    it_mono,
    2081,8,10,1,
    (uint8 *) selectedpoint0_data,
    &clut,
    1
};

static uint8 distance0_data[] = {
    0xff, 0xff, 
    0xbf, 0xfe, 
    0xbb, 0xee, 
    0xb7, 0xf6, 
    0xaa, 0xaa, 
    0xb7, 0xf6, 
    0xbb, 0xee, 
    0xbf, 0xfe, 
    0xbf, 0xfe, 
    0xff, 0xff, 
};

static struct _GImage distance0_base = {
    it_mono,
    2081,16,10,2,
    (uint8 *) distance0_data,
    &clut,
    1
};

static uint8 angle0_data[] = {
    0xff, 0xff, 
    0xff, 0xcf, 
    0xff, 0x9f, 
    0xfe, 0x6f, 
    0xfd, 0xef, 
    0xf3, 0xf7, 
    0xef, 0xf7, 
    0x9f, 0xf7, 
    0x0, 0x1, 
    0xff, 0xff, 
};

static struct _GImage angle0_base = {
    it_mono,
    2081,16,10,2,
    (uint8 *) angle0_data,
    &clut,
    1
};

static uint8 magicon0_data[] = {
    0xc7, 
    0xbb, 
    0x6d, 
    0x45, 
    0x6d, 
    0xbb, 
    0xc3, 
    0xfb, 
    0xfd, 
    0xfd, 
};

static struct _GImage magicon0_base = {
    it_mono,
    2069,8,10,1,
    (uint8 *) magicon0_data,
    &clut,
    1
};

GImage GIcon_mag = { 0, &magicon0_base };
GImage GIcon_angle = { 0, &angle0_base };
GImage GIcon_distance = { 0, &distance0_base };
GImage GIcon_selectedpoint = { 0, &selectedpoint0_base };
GImage GIcon_sel2ptr = { 0, &sel2ptr0_base };
GImage GIcon_rightpointer = { 0, &rightpointer0_base };
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
