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
#include "gdrawP.h"

#define	ACUTE	0x0000001
#define	GRAVE	0x0000002
#define	DIAERESIS	0x0000004
#define	CIRCUMFLEX	0x0000008
#define	TILDE	0x0000010
#define	RING	0x0000020
#define	SLASH	0x0000040
#define	BREVE	0x0000080
#define	CARON	0x0000100
#define	DOTABOVE	0x0000200
#define	DOTBELOW	0x0000400
#define	CEDILLA	0x0000800
#define	OGONEK	0x0001000
#define	MACRON	0x0002000
#define	DBLGRAVE	0x0004000
#define	DBLACUTE	0x0008000
#define	INVBREVE	0x0010000
#define	DIAERESISBELOW	0x0020000
#define	CIRCUMFLEXBELOW	0x0040000
#define	TILDEBELOW	0x0080000
#define	RINGBELOW	0x0100000
#define	LINEBELOW	0x0200000
#define	HOOKABOVE	0x0400000
#define	HORN	0x0800000
#define	GREEK	0x8000000

#define	ANY	0xfffffff

static struct gchr_transform trans_space[] = {
    { 0, 0, 0x00000a0 },
    { GREEK, 0, 0x0002001 }
};

static struct gchr_transform trans_exclam[] = {
    { 0, 0, 0x00000a1 }
};

static struct gchr_transform trans_quotedbl[] = {
    { ANY, DBLGRAVE, 0x0000000 }
};

static struct gchr_transform trans_numbersign[] = {
    { 0, 0, 0x00000a3 },
    { GREEK, 0, 0x00000a5 }
};

static struct gchr_transform trans_dollar[] = {
    { 0, 0, 0x00020ac },
    { GREEK, 0, 0x00000a2 }
};

static struct gchr_transform trans_quotesingle[] = {
    { ANY, ACUTE, 0x0000000 }
};

static struct gchr_transform trans_asterisk[] = {
    { 0, 0, 0x00000b0 },
    { GREEK, 0, 0x0002022 }
};

static struct gchr_transform trans_plus[] = {
    { 0, 0, 0x00000b1 }
};

static struct gchr_transform trans_comma[] = {
    { ANY, DOTBELOW, 0x0000000 }
};

static struct gchr_transform trans_hyphenminus[] = {
    { 0, 0, 0x00000ad },
    { GREEK, 0, 0x0002013 }
};

static struct gchr_transform trans_period[] = {
    { ANY, DOTABOVE, 0x0000000 },
    { GREEK, 0, 0x00000b7 }
};

static struct gchr_transform trans_slash[] = {
    { ANY, SLASH, 0x0000000 }
};

static struct gchr_transform trans_zero[] = {
    { ANY, RING, 0x0000000 }
};

static struct gchr_transform trans_two[] = {
    { BREVE, 0, 0x00000bd }
};

static struct gchr_transform trans_four[] = {
    { ANY, OGONEK, 0x0000000 }
};

static struct gchr_transform trans_five[] = {
    { ANY, CEDILLA, 0x0000000 }
};

static struct gchr_transform trans_six[] = {
    { ANY, CARON, 0x0000000 }
};

static struct gchr_transform trans_seven[] = {
    { ANY, BREVE, 0x0000000 }
};

static struct gchr_transform trans_colon[] = {
    { ANY, DIAERESIS, 0x0000000 }
};

static struct gchr_transform trans_semicolon[] = {
    { 0, 0, 0x0002026 }
};

static struct gchr_transform trans_less[] = {
    { 0, 0, 0x0002265 }
};

static struct gchr_transform trans_equal[] = {
    { GREEK, 0, 0x0002015 }
};

static struct gchr_transform trans_greater[] = {
    { 0, 0, 0x0002264 },
    { GREEK, 0, 0x0002023 }
};

static struct gchr_transform trans_question[] = {
    { 0, 0, 0x00000bf }
};

static struct gchr_transform trans_at[] = {
    { ANY, GREEK, 0x0000000 }
};

static struct gchr_transform trans_A[] = {
    { 0, 0, 0x00000c6 },
    { GREEK, 0, 0x0000391 },
    { DBLGRAVE|GREEK, 0, 0x0000386 },
    { BREVE|GREEK, 0, 0x0001fb8 },
    { MACRON|GREEK, 0, 0x0001fb9 },
    { GRAVE, 0, 0x00000c0 },
    { ACUTE, 0, 0x00000c1 },
    { CIRCUMFLEX, 0, 0x00000c2 },
    { TILDE, 0, 0x00000c3 },
    { DIAERESIS, 0, 0x00000c4 },
    { RING, 0, 0x00000c5 },
    { MACRON, 0, 0x0000100 },
    { BREVE, 0, 0x0000102 },
    { OGONEK, 0, 0x0000104 },
    { CARON, 0, 0x00001cd },
    { DIAERESIS|MACRON, 0, 0x00001de },
    { DOTABOVE|MACRON, 0, 0x00001e0 },
    { ACUTE|RING, 0, 0x00001fa },
    { DBLGRAVE, 0, 0x0000200 },
    { INVBREVE, 0, 0x0000202 },
    { DOTABOVE, 0, 0x0000226 },
    { SLASH, 0, 0x000023a },
    { RINGBELOW, 0, 0x0001e00 },
    { DOTBELOW, 0, 0x0001ea0 },
    { HOOKABOVE, 0, 0x0001ea2 },
    { ACUTE|CIRCUMFLEX, 0, 0x0001ea4 },
    { GRAVE|CIRCUMFLEX, 0, 0x0001ea6 },
    { CIRCUMFLEX|HOOKABOVE, 0, 0x0001ea8 },
    { CIRCUMFLEX|TILDE, 0, 0x0001eaa },
    { CIRCUMFLEX|DOTBELOW, 0, 0x0001eac },
    { ACUTE|BREVE, 0, 0x0001eae },
    { GRAVE|BREVE, 0, 0x0001eb0 },
    { BREVE|HOOKABOVE, 0, 0x0001eb2 },
    { TILDE|BREVE, 0, 0x0001eb4 },
    { BREVE|DOTBELOW, 0, 0x0001eb6 }
};

static struct gchr_transform trans_B[] = {
    { GREEK, 0, 0x0000392 },
    { SLASH, 0, 0x0000243 },
    { DOTABOVE, 0, 0x0001e02 },
    { DOTBELOW, 0, 0x0001e04 },
    { LINEBELOW, 0, 0x0001e06 }
};

static struct gchr_transform trans_C[] = {
    { 0, 0, 0x00000c7 },
    { GREEK, 0, 0x00003a7 },
    { CEDILLA, 0, 0x00000c7 },
    { ACUTE, 0, 0x0000106 },
    { CIRCUMFLEX, 0, 0x0000108 },
    { DOTABOVE, 0, 0x000010a },
    { CARON, 0, 0x000010c },
    { SLASH, 0, 0x000023b },
    { ACUTE|CEDILLA, 0, 0x0001e08 }
};

static struct gchr_transform trans_D[] = {
    { GREEK, 0, 0x0000394 },
    { CARON, 0, 0x000010e },
    { SLASH, 0, 0x0000110 },
    { DOTABOVE, 0, 0x0001e0a },
    { DOTBELOW, 0, 0x0001e0c },
    { LINEBELOW, 0, 0x0001e0e },
    { CEDILLA, 0, 0x0001e10 },
    { CIRCUMFLEXBELOW, 0, 0x0001e12 }
};

static struct gchr_transform trans_E[] = {
    { GREEK, 0, 0x0000395 },
    { DBLGRAVE|GREEK, 0, 0x0000388 },
    { GRAVE|GREEK, 0, 0x0001fc8 },
    { ACUTE|GREEK, 0, 0x0001fc9 },
    { GRAVE, 0, 0x00000c8 },
    { ACUTE, 0, 0x00000c9 },
    { CIRCUMFLEX, 0, 0x00000ca },
    { DIAERESIS, 0, 0x00000cb },
    { MACRON, 0, 0x0000112 },
    { BREVE, 0, 0x0000114 },
    { DOTABOVE, 0, 0x0000116 },
    { OGONEK, 0, 0x0000118 },
    { CARON, 0, 0x000011a },
    { DBLGRAVE, 0, 0x0000204 },
    { INVBREVE, 0, 0x0000206 },
    { CEDILLA, 0, 0x0000228 },
    { SLASH, 0, 0x0000246 },
    { GRAVE|MACRON, 0, 0x0001e14 },
    { ACUTE|MACRON, 0, 0x0001e16 },
    { CIRCUMFLEXBELOW, 0, 0x0001e18 },
    { TILDEBELOW, 0, 0x0001e1a },
    { BREVE|CEDILLA, 0, 0x0001e1c },
    { DOTBELOW, 0, 0x0001eb8 },
    { HOOKABOVE, 0, 0x0001eba },
    { TILDE, 0, 0x0001ebc },
    { ACUTE|CIRCUMFLEX, 0, 0x0001ebe },
    { GRAVE|CIRCUMFLEX, 0, 0x0001ec0 },
    { CIRCUMFLEX|HOOKABOVE, 0, 0x0001ec2 },
    { CIRCUMFLEX|TILDE, 0, 0x0001ec4 },
    { CIRCUMFLEX|DOTBELOW, 0, 0x0001ec6 }
};

static struct gchr_transform trans_F[] = {
    { GREEK, 0, 0x00003a6 },
    { DOTABOVE, 0, 0x0001e1e },
    { SLASH, 0, 0x000a798 }
};

static struct gchr_transform trans_G[] = {
    { GREEK, 0, 0x0000393 },
    { CIRCUMFLEX, 0, 0x000011c },
    { BREVE, 0, 0x000011e },
    { DOTABOVE, 0, 0x0000120 },
    { CEDILLA, 0, 0x0000122 },
    { SLASH, 0, 0x00001e4 },
    { CARON, 0, 0x00001e6 },
    { ACUTE, 0, 0x00001f4 },
    { MACRON, 0, 0x0001e20 }
};

static struct gchr_transform trans_H[] = {
    { 0, 0, 0x000261e },
    { GREEK, 0, 0x0000397 },
    { DBLGRAVE|GREEK, 0, 0x0000389 },
    { GRAVE|GREEK, 0, 0x0001fca },
    { ACUTE|GREEK, 0, 0x0001fcb },
    { CIRCUMFLEX, 0, 0x0000124 },
    { SLASH, 0, 0x0000126 },
    { CARON, 0, 0x000021e },
    { DOTABOVE, 0, 0x0001e22 },
    { DOTBELOW, 0, 0x0001e24 },
    { DIAERESIS, 0, 0x0001e26 },
    { CEDILLA, 0, 0x0001e28 },
    { BREVE, 0, 0x0001e2a }
};

static struct gchr_transform trans_I[] = {
    { GREEK, 0, 0x0000399 },
    { DBLGRAVE|GREEK, 0, 0x000038a },
    { DIAERESIS|GREEK, 0, 0x00003aa },
    { GRAVE|GREEK, 0, 0x0001f7a },
    { ACUTE|GREEK, 0, 0x0001f7b },
    { TILDE|GREEK, 0, 0x0001f78 },
    { MACRON|GREEK, 0, 0x0001f79 },
    { GRAVE, 0, 0x00000cc },
    { ACUTE, 0, 0x00000cd },
    { CIRCUMFLEX, 0, 0x00000ce },
    { DIAERESIS, 0, 0x00000cf },
    { TILDE, 0, 0x0000128 },
    { MACRON, 0, 0x000012a },
    { BREVE, 0, 0x000012c },
    { OGONEK, 0, 0x000012e },
    { DOTABOVE, 0, 0x0000130 },
    { SLASH, 0, 0x0000197 },
    { CARON, 0, 0x00001cf },
    { DBLGRAVE, 0, 0x0000208 },
    { INVBREVE, 0, 0x000020a },
    { TILDEBELOW, 0, 0x0001e2c },
    { ACUTE|DIAERESIS, 0, 0x0001e2e },
    { HOOKABOVE, 0, 0x0001ec8 },
    { DOTBELOW, 0, 0x0001eca }
};

static struct gchr_transform trans_J[] = {
    { GREEK, 0, 0x00003d1 },
    { CIRCUMFLEX, 0, 0x0000134 },
    { SLASH, 0, 0x0000248 }
};

static struct gchr_transform trans_K[] = {
    { GREEK, 0, 0x000039a },
    { CEDILLA, 0, 0x0000136 },
    { CARON, 0, 0x00001e8 },
    { ACUTE, 0, 0x0001e30 },
    { DOTBELOW, 0, 0x0001e32 },
    { LINEBELOW, 0, 0x0001e34 },
    { SLASH, 0, 0x000a740 }
};

static struct gchr_transform trans_L[] = {
    { GREEK, 0, 0x000039b },
    { ACUTE, 0, 0x0000139 },
    { CEDILLA, 0, 0x000013b },
    { CARON, 0, 0x000013d },
    { DOTABOVE, 0, 0x000013f },
    { SLASH, 0, 0x0000141 },
    { DOTBELOW, 0, 0x0001e36 },
    { DOTBELOW|MACRON, 0, 0x0001e38 },
    { LINEBELOW, 0, 0x0001e3a },
    { CIRCUMFLEXBELOW, 0, 0x0001e3c }
};

static struct gchr_transform trans_M[] = {
    { GREEK, 0, 0x000039c },
    { ACUTE, 0, 0x0001e3e },
    { DOTABOVE, 0, 0x0001e40 },
    { DOTBELOW, 0, 0x0001e42 }
};

static struct gchr_transform trans_N[] = {
    { GREEK, 0, 0x000039d },
    { TILDE, 0, 0x00000d1 },
    { ACUTE, 0, 0x0000143 },
    { CEDILLA, 0, 0x0000145 },
    { CARON, 0, 0x0000147 },
    { GRAVE, 0, 0x00001f8 },
    { DOTABOVE, 0, 0x0001e44 },
    { DOTBELOW, 0, 0x0001e46 },
    { LINEBELOW, 0, 0x0001e48 },
    { CIRCUMFLEXBELOW, 0, 0x0001e4a }
};

static struct gchr_transform trans_O[] = {
    { 0, 0, 0x0000152 },
    { GREEK, 0, 0x000039f },
    { DBLGRAVE|GREEK, 0, 0x000038c },
    { GRAVE|GREEK, 0, 0x0001ff8 },
    { ACUTE|GREEK, 0, 0x0001ff9 },
    { GRAVE, 0, 0x00000d2 },
    { ACUTE, 0, 0x00000d3 },
    { CIRCUMFLEX, 0, 0x00000d4 },
    { TILDE, 0, 0x00000d5 },
    { DIAERESIS, 0, 0x00000d6 },
    { SLASH, 0, 0x00000d8 },
    { MACRON, 0, 0x000014c },
    { BREVE, 0, 0x000014e },
    { DBLACUTE, 0, 0x0000150 },
    { HORN, 0, 0x00001a0 },
    { CARON, 0, 0x00001d1 },
    { OGONEK, 0, 0x00001ea },
    { OGONEK|MACRON, 0, 0x00001ec },
    { ACUTE|SLASH, 0, 0x00001fe },
    { DBLGRAVE, 0, 0x000020c },
    { INVBREVE, 0, 0x000020e },
    { DIAERESIS|MACRON, 0, 0x000022a },
    { TILDE|MACRON, 0, 0x000022c },
    { DOTABOVE, 0, 0x000022e },
    { DOTABOVE|MACRON, 0, 0x0000230 },
    { ACUTE|TILDE, 0, 0x0001e4c },
    { DIAERESIS|TILDE, 0, 0x0001e4e },
    { GRAVE|MACRON, 0, 0x0001e50 },
    { ACUTE|MACRON, 0, 0x0001e52 },
    { DOTBELOW, 0, 0x0001ecc },
    { HOOKABOVE, 0, 0x0001ece },
    { ACUTE|CIRCUMFLEX, 0, 0x0001ed0 },
    { GRAVE|CIRCUMFLEX, 0, 0x0001ed2 },
    { CIRCUMFLEX|HOOKABOVE, 0, 0x0001ed4 },
    { CIRCUMFLEX|TILDE, 0, 0x0001ed6 },
    { CIRCUMFLEX|DOTBELOW, 0, 0x0001ed8 },
    { ACUTE|HORN, 0, 0x0001eda },
    { GRAVE|HORN, 0, 0x0001edc },
    { HOOKABOVE|HORN, 0, 0x0001ede },
    { TILDE|HORN, 0, 0x0001ee0 },
    { DOTBELOW|HORN, 0, 0x0001ee2 }
};

static struct gchr_transform trans_P[] = {
    { 0, 0, 0x00000a7 },
    { GREEK, 0, 0x00003a0 },
    { ACUTE, 0, 0x0001e54 },
    { DOTABOVE, 0, 0x0001e56 },
    { SLASH, 0, 0x0002c63 },
    { SLASH, 0, 0x000a750 }
};

static struct gchr_transform trans_Q[] = {
    { GREEK, 0, 0x0000398 },
    { SLASH, 0, 0x000a756 }
};

static struct gchr_transform trans_R[] = {
    { GREEK, 0, 0x00003a1 },
    { ACUTE, 0, 0x0000154 },
    { CEDILLA, 0, 0x0000156 },
    { CARON, 0, 0x0000158 },
    { DBLGRAVE, 0, 0x0000210 },
    { INVBREVE, 0, 0x0000212 },
    { SLASH, 0, 0x000024c },
    { DOTABOVE, 0, 0x0001e58 },
    { DOTBELOW, 0, 0x0001e5a },
    { DOTBELOW|MACRON, 0, 0x0001e5c },
    { LINEBELOW, 0, 0x0001e5e }
};

static struct gchr_transform trans_S[] = {
    { GREEK, 0, 0x00003a3 },
    { ACUTE, 0, 0x000015a },
    { CIRCUMFLEX, 0, 0x000015c },
    { CEDILLA, 0, 0x000015e },
    { CARON, 0, 0x0000160 },
    { DOTABOVE, 0, 0x0001e60 },
    { DOTBELOW, 0, 0x0001e62 },
    { ACUTE|DOTABOVE, 0, 0x0001e64 },
    { CARON|DOTABOVE, 0, 0x0001e66 },
    { DOTABOVE|DOTBELOW, 0, 0x0001e68 }
};

static struct gchr_transform trans_T[] = {
    { GREEK, 0, 0x00003a4 },
    { CEDILLA, 0, 0x0000162 },
    { CARON, 0, 0x0000164 },
    { SLASH, 0, 0x0000166 },
    { DOTABOVE, 0, 0x0001e6a },
    { DOTBELOW, 0, 0x0001e6c },
    { LINEBELOW, 0, 0x0001e6e },
    { CIRCUMFLEXBELOW, 0, 0x0001e70 }
};

static struct gchr_transform trans_U[] = {
    { GREEK, 0, 0x00003a5 },
    { DBLGRAVE|GREEK, 0, 0x000038e },
    { DIAERESIS|GREEK, 0, 0x00003ab },
    { GRAVE|GREEK, 0, 0x0001fea },
    { ACUTE|GREEK, 0, 0x0001feb },
    { BREVE|GREEK, 0, 0x0001fe8 },
    { MACRON|GREEK, 0, 0x0001fe9 },
    { GRAVE, 0, 0x00000d9 },
    { ACUTE, 0, 0x00000da },
    { CIRCUMFLEX, 0, 0x00000db },
    { DIAERESIS, 0, 0x00000dc },
    { TILDE, 0, 0x0000168 },
    { MACRON, 0, 0x000016a },
    { BREVE, 0, 0x000016c },
    { RING, 0, 0x000016e },
    { DBLACUTE, 0, 0x0000170 },
    { OGONEK, 0, 0x0000172 },
    { HORN, 0, 0x00001af },
    { CARON, 0, 0x00001d3 },
    { DIAERESIS|MACRON, 0, 0x00001d5 },
    { ACUTE|DIAERESIS, 0, 0x00001d7 },
    { DIAERESIS|CARON, 0, 0x00001d9 },
    { GRAVE|DIAERESIS, 0, 0x00001db },
    { DBLGRAVE, 0, 0x0000214 },
    { INVBREVE, 0, 0x0000216 },
    { DIAERESISBELOW, 0, 0x0001e72 },
    { TILDEBELOW, 0, 0x0001e74 },
    { CIRCUMFLEXBELOW, 0, 0x0001e76 },
    { ACUTE|TILDE, 0, 0x0001e78 },
    { DIAERESIS|MACRON, 0, 0x0001e7a },
    { DOTBELOW, 0, 0x0001ee4 },
    { HOOKABOVE, 0, 0x0001ee6 },
    { ACUTE|HORN, 0, 0x0001ee8 },
    { GRAVE|HORN, 0, 0x0001eea },
    { HOOKABOVE|HORN, 0, 0x0001eec },
    { TILDE|HORN, 0, 0x0001eee },
    { DOTBELOW|HORN, 0, 0x0001ef0 }
};

static struct gchr_transform trans_V[] = {
    { GREEK, 0, 0x00003c2 },
    { TILDE, 0, 0x0001e7c },
    { DOTBELOW, 0, 0x0001e7e }
};

static struct gchr_transform trans_W[] = {
    { GREEK, 0, 0x00003a9 },
    { DBLGRAVE|GREEK, 0, 0x000038f },
    { GRAVE|GREEK, 0, 0x0001ffa },
    { ACUTE|GREEK, 0, 0x0001ffb },
    { CIRCUMFLEX, 0, 0x0000174 },
    { GRAVE, 0, 0x0001e80 },
    { ACUTE, 0, 0x0001e82 },
    { DIAERESIS, 0, 0x0001e84 },
    { DOTABOVE, 0, 0x0001e86 },
    { DOTBELOW, 0, 0x0001e88 }
};

static struct gchr_transform trans_X[] = {
    { GREEK, 0, 0x000039e },
    { DOTABOVE, 0, 0x0001e8a },
    { DIAERESIS, 0, 0x0001e8c }
};

static struct gchr_transform trans_Y[] = {
    { GREEK, 0, 0x00003a8 },
    { ACUTE, 0, 0x00000dd },
    { CIRCUMFLEX, 0, 0x0000176 },
    { DIAERESIS, 0, 0x0000178 },
    { MACRON, 0, 0x0000232 },
    { SLASH, 0, 0x000024e },
    { DOTABOVE, 0, 0x0001e8e },
    { GRAVE, 0, 0x0001ef2 },
    { DOTBELOW, 0, 0x0001ef4 },
    { HOOKABOVE, 0, 0x0001ef6 },
    { TILDE, 0, 0x0001ef8 }
};

static struct gchr_transform trans_Z[] = {
    { GREEK, 0, 0x0000396 },
    { ACUTE, 0, 0x0000179 },
    { DOTABOVE, 0, 0x000017b },
    { CARON, 0, 0x000017d },
    { SLASH, 0, 0x00001b5 },
    { CIRCUMFLEX, 0, 0x0001e90 },
    { DOTBELOW, 0, 0x0001e92 },
    { LINEBELOW, 0, 0x0001e94 }
};

static struct gchr_transform trans_bracketleft[] = {
    { 0, 0, 0x0002018 }
};

static struct gchr_transform trans_backslash[] = {
    { 0, 0, 0x00000ab },
    { GREEK, 0, 0x0002039 }
};

static struct gchr_transform trans_bracketright[] = {
    { 0, 0, 0x0002019 }
};

static struct gchr_transform trans_asciicircum[] = {
    { ANY, CIRCUMFLEX, 0x0000000 }
};

static struct gchr_transform trans_underscore[] = {
    { ANY, MACRON, 0x0000000 },
    { GREEK, 0, 0x0002014 }
};

static struct gchr_transform trans_grave[] = {
    { ANY, GRAVE, 0x0000000 }
};

static struct gchr_transform trans_a[] = {
    { 0, 0, 0x00000e6 },
    { GREEK, 0, 0x00003b1 },
    { DBLGRAVE|GREEK, 0, 0x00003ac },
    { GRAVE|GREEK, 0, 0x0001f70 },
    { ACUTE|GREEK, 0, 0x0001f71 },
    { BREVE|GREEK, 0, 0x0001fb0 },
    { MACRON|GREEK, 0, 0x0001fb1 },
    { TILDE|GREEK, 0, 0x0001fb6 },
    { GRAVE, 0, 0x00000e0 },
    { ACUTE, 0, 0x00000e1 },
    { CIRCUMFLEX, 0, 0x00000e2 },
    { TILDE, 0, 0x00000e3 },
    { DIAERESIS, 0, 0x00000e4 },
    { RING, 0, 0x00000e5 },
    { MACRON, 0, 0x0000101 },
    { BREVE, 0, 0x0000103 },
    { OGONEK, 0, 0x0000105 },
    { CARON, 0, 0x00001ce },
    { DIAERESIS|MACRON, 0, 0x00001df },
    { DOTABOVE|MACRON, 0, 0x00001e1 },
    { ACUTE|RING, 0, 0x00001fb },
    { DBLGRAVE, 0, 0x0000201 },
    { INVBREVE, 0, 0x0000203 },
    { DOTABOVE, 0, 0x0000227 },
    { RINGBELOW, 0, 0x0001e01 },
    { DOTBELOW, 0, 0x0001ea1 },
    { HOOKABOVE, 0, 0x0001ea3 },
    { ACUTE|CIRCUMFLEX, 0, 0x0001ea5 },
    { GRAVE|CIRCUMFLEX, 0, 0x0001ea7 },
    { CIRCUMFLEX|HOOKABOVE, 0, 0x0001ea9 },
    { CIRCUMFLEX|TILDE, 0, 0x0001eab },
    { CIRCUMFLEX|DOTBELOW, 0, 0x0001ead },
    { ACUTE|BREVE, 0, 0x0001eaf },
    { GRAVE|BREVE, 0, 0x0001eb1 },
    { BREVE|HOOKABOVE, 0, 0x0001eb3 },
    { TILDE|BREVE, 0, 0x0001eb5 },
    { BREVE|DOTBELOW, 0, 0x0001eb7 },
    { SLASH, 0, 0x0002c65 }
};

static struct gchr_transform trans_b[] = {
    { GREEK, 0, 0x00003b2 },
    { SLASH, 0, 0x0000180 },
    { DOTABOVE, 0, 0x0001e03 },
    { DOTBELOW, 0, 0x0001e05 },
    { LINEBELOW, 0, 0x0001e07 }
};

static struct gchr_transform trans_c[] = {
    { 0, 0, 0x00000e7 },
    { GREEK, 0, 0x00003c7 },
    { CEDILLA, 0, 0x00000e7 },
    { ACUTE, 0, 0x0000107 },
    { CIRCUMFLEX, 0, 0x0000109 },
    { DOTABOVE, 0, 0x000010b },
    { CARON, 0, 0x000010d },
    { SLASH, 0, 0x000023c },
    { ACUTE|CEDILLA, 0, 0x0001e09 }
};

static struct gchr_transform trans_d[] = {
    { GREEK, 0, 0x00003b4 },
    { CARON, 0, 0x000010f },
    { SLASH, 0, 0x0000111 },
    { DOTABOVE, 0, 0x0001e0b },
    { DOTBELOW, 0, 0x0001e0d },
    { LINEBELOW, 0, 0x0001e0f },
    { CEDILLA, 0, 0x0001e11 },
    { CIRCUMFLEXBELOW, 0, 0x0001e13 }
};

static struct gchr_transform trans_e[] = {
    { 0, ACUTE, 0x0000000 },
    { GREEK, 0, 0x00003b5 },
    { DBLGRAVE|GREEK, 0, 0x00003ad },
    { GRAVE|GREEK, 0, 0x0001f72 },
    { ACUTE|GREEK, 0, 0x0001f73 },
    { GRAVE, 0, 0x00000e8 },
    { ACUTE, 0, 0x00000e9 },
    { CIRCUMFLEX, 0, 0x00000ea },
    { DIAERESIS, 0, 0x00000eb },
    { MACRON, 0, 0x0000113 },
    { BREVE, 0, 0x0000115 },
    { DOTABOVE, 0, 0x0000117 },
    { OGONEK, 0, 0x0000119 },
    { CARON, 0, 0x000011b },
    { DBLGRAVE, 0, 0x0000205 },
    { INVBREVE, 0, 0x0000207 },
    { CEDILLA, 0, 0x0000229 },
    { SLASH, 0, 0x0000247 },
    { GRAVE|MACRON, 0, 0x0001e15 },
    { ACUTE|MACRON, 0, 0x0001e17 },
    { CIRCUMFLEXBELOW, 0, 0x0001e19 },
    { TILDEBELOW, 0, 0x0001e1b },
    { BREVE|CEDILLA, 0, 0x0001e1d },
    { DOTBELOW, 0, 0x0001eb9 },
    { HOOKABOVE, 0, 0x0001ebb },
    { TILDE, 0, 0x0001ebd },
    { ACUTE|CIRCUMFLEX, 0, 0x0001ebf },
    { GRAVE|CIRCUMFLEX, 0, 0x0001ec1 },
    { CIRCUMFLEX|HOOKABOVE, 0, 0x0001ec3 },
    { CIRCUMFLEX|TILDE, 0, 0x0001ec5 },
    { CIRCUMFLEX|DOTBELOW, 0, 0x0001ec7 }
};

static struct gchr_transform trans_f[] = {
    { 0, 0, 0x0002640 },
    { GREEK, 0, 0x00003c6 },
    { DOTABOVE, 0, 0x0001e1f },
    { SLASH, 0, 0x000a799 }
};

static struct gchr_transform trans_g[] = {
    { 0, 0, 0x00000a9 },
    { GREEK, 0, 0x00003b3 },
    { CIRCUMFLEX, 0, 0x000011d },
    { BREVE, 0, 0x000011f },
    { DOTABOVE, 0, 0x0000121 },
    { CEDILLA, 0, 0x0000123 },
    { SLASH, 0, 0x00001e5 },
    { CARON, 0, 0x00001e7 },
    { ACUTE, 0, 0x00001f5 },
    { MACRON, 0, 0x0001e21 }
};

static struct gchr_transform trans_h[] = {
    { SLASH|GREEK, 0, 0x000210f },
    { 0, 0, 0x000261e },
    { GREEK, 0, 0x00003b7 },
    { DBLGRAVE|GREEK, 0, 0x00003ae },
    { GRAVE|GREEK, 0, 0x0001f74 },
    { ACUTE|GREEK, 0, 0x0001f75 },
    { TILDE|GREEK, 0, 0x0001fc6 },
    { CIRCUMFLEX, 0, 0x0000125 },
    { SLASH, 0, 0x0000127 },
    { CARON, 0, 0x000021f },
    { DOTABOVE, 0, 0x0001e23 },
    { DOTBELOW, 0, 0x0001e25 },
    { DIAERESIS, 0, 0x0001e27 },
    { CEDILLA, 0, 0x0001e29 },
    { BREVE, 0, 0x0001e2b },
    { LINEBELOW, 0, 0x0001e96 }
};

static struct gchr_transform trans_i[] = {
    { 0, CIRCUMFLEX, 0x0000000 },
    { DOTABOVE, 0, 0x0000131 },
    { GREEK, 0, 0x00003b9 },
    { DBLGRAVE|GREEK, 0, 0x00003af },
    { DIAERESIS|GREEK, 0, 0x00003ca },
    { DIAERESIS|DBLGRAVE|GREEK, 0, 0x0000390 },
    { GRAVE|GREEK, 0, 0x0001f76 },
    { ACUTE|GREEK, 0, 0x0001f77 },
    { BREVE|GREEK, 0, 0x0001fd0 },
    { MACRON|GREEK, 0, 0x0001fd1 },
    { TILDE|GREEK, 0, 0x0001fd6 },
    { GRAVE|DIAERESIS|GREEK, 0, 0x0001fd2 },
    { ACUTE|DIAERESIS|GREEK, 0, 0x0001fd3 },
    { DIAERESIS|TILDE|GREEK, 0, 0x0001fd7 },
    { GRAVE, 0, 0x00000ec },
    { ACUTE, 0, 0x00000ed },
    { CIRCUMFLEX, 0, 0x00000ee },
    { DIAERESIS, 0, 0x00000ef },
    { TILDE, 0, 0x0000129 },
    { MACRON, 0, 0x000012b },
    { BREVE, 0, 0x000012d },
    { OGONEK, 0, 0x000012f },
    { CARON, 0, 0x00001d0 },
    { DBLGRAVE, 0, 0x0000209 },
    { INVBREVE, 0, 0x000020b },
    { SLASH, 0, 0x0000268 },
    { TILDEBELOW, 0, 0x0001e2d },
    { ACUTE|DIAERESIS, 0, 0x0001e2f },
    { HOOKABOVE, 0, 0x0001ec9 },
    { DOTBELOW, 0, 0x0001ecb }
};

static struct gchr_transform trans_j[] = {
    { GREEK, 0, 0x00003d5 },
    { CIRCUMFLEX, 0, 0x0000135 },
    { CARON, 0, 0x00001f0 },
    { SLASH, 0, 0x0000249 }
};

static struct gchr_transform trans_k[] = {
    { GREEK, 0, 0x00003ba },
    { CEDILLA, 0, 0x0000137 },
    { CARON, 0, 0x00001e9 },
    { ACUTE, 0, 0x0001e31 },
    { DOTBELOW, 0, 0x0001e33 },
    { LINEBELOW, 0, 0x0001e35 },
    { SLASH, 0, 0x000a741 }
};

static struct gchr_transform trans_l[] = {
    { GREEK, 0, 0x00003bb },
    { ACUTE, 0, 0x000013a },
    { CEDILLA, 0, 0x000013c },
    { CARON, 0, 0x000013e },
    { DOTABOVE, 0, 0x0000140 },
    { SLASH, 0, 0x0000142 },
    { DOTBELOW, 0, 0x0001e37 },
    { DOTBELOW|MACRON, 0, 0x0001e39 },
    { LINEBELOW, 0, 0x0001e3b },
    { CIRCUMFLEXBELOW, 0, 0x0001e3d }
};

static struct gchr_transform trans_m[] = {
    { 0, 0, 0x0002642 },
    { GREEK, 0, 0x00003bc },
    { ACUTE, 0, 0x0001e3f },
    { DOTABOVE, 0, 0x0001e41 },
    { DOTBELOW, 0, 0x0001e43 }
};

static struct gchr_transform trans_n[] = {
    { 0, TILDE, 0x0000000 },
    { GREEK, 0, 0x00003bd },
    { TILDE, 0, 0x00000f1 },
    { ACUTE, 0, 0x0000144 },
    { CEDILLA, 0, 0x0000146 },
    { CARON, 0, 0x0000148 },
    { GRAVE, 0, 0x00001f9 },
    { DOTABOVE, 0, 0x0001e45 },
    { DOTBELOW, 0, 0x0001e47 },
    { LINEBELOW, 0, 0x0001e49 },
    { CIRCUMFLEXBELOW, 0, 0x0001e4b }
};

static struct gchr_transform trans_o[] = {
    { 0, 0, 0x0001536 },
    { GREEK, 0, 0x00003bf },
    { DBLGRAVE|GREEK, 0, 0x00003cc },
    { GRAVE|GREEK, 0, 0x0001f78 },
    { ACUTE|GREEK, 0, 0x0001f79 },
    { GRAVE, 0, 0x00000f2 },
    { ACUTE, 0, 0x00000f3 },
    { CIRCUMFLEX, 0, 0x00000f4 },
    { TILDE, 0, 0x00000f5 },
    { DIAERESIS, 0, 0x00000f6 },
    { SLASH, 0, 0x00000f8 },
    { MACRON, 0, 0x000014d },
    { BREVE, 0, 0x000014f },
    { DBLACUTE, 0, 0x0000151 },
    { HORN, 0, 0x00001a1 },
    { CARON, 0, 0x00001d2 },
    { OGONEK, 0, 0x00001eb },
    { OGONEK|MACRON, 0, 0x00001ed },
    { ACUTE|SLASH, 0, 0x00001ff },
    { DBLGRAVE, 0, 0x000020d },
    { INVBREVE, 0, 0x000020f },
    { DIAERESIS|MACRON, 0, 0x000022b },
    { TILDE|MACRON, 0, 0x000022d },
    { DOTABOVE, 0, 0x000022f },
    { DOTABOVE|MACRON, 0, 0x0000231 },
    { ACUTE|TILDE, 0, 0x0001e4d },
    { DIAERESIS|TILDE, 0, 0x0001e4f },
    { GRAVE|MACRON, 0, 0x0001e51 },
    { ACUTE|MACRON, 0, 0x0001e53 },
    { DOTBELOW, 0, 0x0001ecd },
    { HOOKABOVE, 0, 0x0001ecf },
    { ACUTE|CIRCUMFLEX, 0, 0x0001ed1 },
    { GRAVE|CIRCUMFLEX, 0, 0x0001ed3 },
    { CIRCUMFLEX|HOOKABOVE, 0, 0x0001ed5 },
    { CIRCUMFLEX|TILDE, 0, 0x0001ed7 },
    { CIRCUMFLEX|DOTBELOW, 0, 0x0001ed9 },
    { ACUTE|HORN, 0, 0x0001edb },
    { GRAVE|HORN, 0, 0x0001edd },
    { HOOKABOVE|HORN, 0, 0x0001edf },
    { TILDE|HORN, 0, 0x0001ee1 },
    { DOTBELOW|HORN, 0, 0x0001ee3 }
};

static struct gchr_transform trans_p[] = {
    { 0, 0, 0x00000b6 },
    { GREEK, 0, 0x00003c0 },
    { SLASH, 0, 0x0001d7d },
    { ACUTE, 0, 0x0001e55 },
    { DOTABOVE, 0, 0x0001e57 },
    { SLASH, 0, 0x000a751 }
};

static struct gchr_transform trans_q[] = {
    { GREEK, 0, 0x00003b8 },
    { SLASH, 0, 0x000a757 }
};

static struct gchr_transform trans_r[] = {
    { 0, 0, 0x00000ae },
    { GREEK, 0, 0x00003c1 },
    { ACUTE, 0, 0x0000155 },
    { CEDILLA, 0, 0x0000157 },
    { CARON, 0, 0x0000159 },
    { DBLGRAVE, 0, 0x0000211 },
    { INVBREVE, 0, 0x0000213 },
    { SLASH, 0, 0x000024d },
    { DOTABOVE, 0, 0x0001e59 },
    { DOTBELOW, 0, 0x0001e5b },
    { DOTBELOW|MACRON, 0, 0x0001e5d },
    { LINEBELOW, 0, 0x0001e5f }
};

static struct gchr_transform trans_s[] = {
    { 0, 0, 0x00000df },
    { GREEK, 0, 0x00003c3 },
    { ACUTE, 0, 0x000015b },
    { CIRCUMFLEX, 0, 0x000015d },
    { CEDILLA, 0, 0x000015f },
    { CARON, 0, 0x0000161 },
    { DOTABOVE, 0, 0x0001e61 },
    { DOTBELOW, 0, 0x0001e63 },
    { ACUTE|DOTABOVE, 0, 0x0001e65 },
    { CARON|DOTABOVE, 0, 0x0001e67 },
    { DOTABOVE|DOTBELOW, 0, 0x0001e69 }
};

static struct gchr_transform trans_t[] = {
    { 0, 0, 0x0002122 },
    { GREEK, 0, 0x00003c4 },
    { CEDILLA, 0, 0x0000163 },
    { CARON, 0, 0x0000165 },
    { SLASH, 0, 0x0000167 },
    { DOTABOVE, 0, 0x0001e6b },
    { DOTBELOW, 0, 0x0001e6d },
    { LINEBELOW, 0, 0x0001e6f },
    { CIRCUMFLEXBELOW, 0, 0x0001e71 },
    { DIAERESIS, 0, 0x0001e97 }
};

static struct gchr_transform trans_u[] = {
    { 0, DIAERESIS, 0x0000000 },
    { GREEK, 0, 0x00003c5 },
    { DBLGRAVE|GREEK, 0, 0x00003cd },
    { DIAERESIS|GREEK, 0, 0x00003cb },
    { DIAERESIS|DBLGRAVE|GREEK, 0, 0x00003b0 },
    { GRAVE|GREEK, 0, 0x0001f7a },
    { ACUTE|GREEK, 0, 0x0001f7b },
    { BREVE|GREEK, 0, 0x0001ff0 },
    { MACRON|GREEK, 0, 0x0001fe1 },
    { GRAVE|DIAERESIS|GREEK, 0, 0x0001fe3 },
    { ACUTE|DIAERESIS|GREEK, 0, 0x0001fe4 },
    { TILDE|GREEK, 0, 0x0001fe6 },
    { DIAERESIS|TILDE|GREEK, 0, 0x0001fe7 },
    { GRAVE, 0, 0x00000f9 },
    { ACUTE, 0, 0x00000fa },
    { CIRCUMFLEX, 0, 0x00000fb },
    { DIAERESIS, 0, 0x00000fc },
    { TILDE, 0, 0x0000169 },
    { MACRON, 0, 0x000016b },
    { BREVE, 0, 0x000016d },
    { RING, 0, 0x000016f },
    { DBLACUTE, 0, 0x0000171 },
    { OGONEK, 0, 0x0000173 },
    { HORN, 0, 0x00001b0 },
    { CARON, 0, 0x00001d4 },
    { DIAERESIS|MACRON, 0, 0x00001d6 },
    { ACUTE|DIAERESIS, 0, 0x00001d8 },
    { DIAERESIS|CARON, 0, 0x00001da },
    { GRAVE|DIAERESIS, 0, 0x00001dc },
    { DBLGRAVE, 0, 0x0000215 },
    { INVBREVE, 0, 0x0000217 },
    { DIAERESISBELOW, 0, 0x0001e73 },
    { TILDEBELOW, 0, 0x0001e75 },
    { CIRCUMFLEXBELOW, 0, 0x0001e77 },
    { ACUTE|TILDE, 0, 0x0001e79 },
    { DIAERESIS|MACRON, 0, 0x0001e7b },
    { DOTBELOW, 0, 0x0001ee5 },
    { HOOKABOVE, 0, 0x0001ee7 },
    { ACUTE|HORN, 0, 0x0001ee9 },
    { GRAVE|HORN, 0, 0x0001eeb },
    { HOOKABOVE|HORN, 0, 0x0001eed },
    { TILDE|HORN, 0, 0x0001eef },
    { DOTBELOW|HORN, 0, 0x0001ef1 }
};

static struct gchr_transform trans_v[] = {
    { GREEK, 0, 0x00003d6 },
    { TILDE, 0, 0x0001e7d },
    { DOTBELOW, 0, 0x0001e7f }
};

static struct gchr_transform trans_w[] = {
    { GREEK, 0, 0x00003c9 },
    { DBLGRAVE|GREEK, 0, 0x00003ce },
    { GRAVE|GREEK, 0, 0x0001f7a },
    { ACUTE|GREEK, 0, 0x0001f7b },
    { CIRCUMFLEX, 0, 0x0000175 },
    { GRAVE, 0, 0x0001e81 },
    { ACUTE, 0, 0x0001e83 },
    { DIAERESIS, 0, 0x0001e85 },
    { DOTABOVE, 0, 0x0001e87 },
    { DOTBELOW, 0, 0x0001e89 },
    { RING, 0, 0x0001e98 }
};

static struct gchr_transform trans_x[] = {
    { GREEK, 0, 0x00003be },
    { DOTABOVE, 0, 0x0001e8b },
    { DIAERESIS, 0, 0x0001e8d }
};

static struct gchr_transform trans_y[] = {
    { GREEK, 0, 0x00003c8 },
    { ACUTE, 0, 0x00000fd },
    { DIAERESIS, 0, 0x00000ff },
    { CIRCUMFLEX, 0, 0x0000177 },
    { MACRON, 0, 0x0000233 },
    { SLASH, 0, 0x000024f },
    { DOTABOVE, 0, 0x0001e8f },
    { RING, 0, 0x0001e99 },
    { GRAVE, 0, 0x0001ef3 },
    { DOTBELOW, 0, 0x0001ef5 },
    { HOOKABOVE, 0, 0x0001ef7 },
    { TILDE, 0, 0x0001ef9 }
};

static struct gchr_transform trans_z[] = {
    { 0, 0, 0x000017f },
    { GREEK, 0, 0x00003b6 },
    { ACUTE, 0, 0x000017a },
    { DOTABOVE, 0, 0x000017c },
    { CARON, 0, 0x000017e },
    { SLASH, 0, 0x00001b6 },
    { CIRCUMFLEX, 0, 0x0001e91 },
    { DOTBELOW, 0, 0x0001e93 },
    { LINEBELOW, 0, 0x0001e95 }
};

static struct gchr_transform trans_braceleft[] = {
    { 0, 0, 0x000201c }
};

static struct gchr_transform trans_bar[] = {
    { 0, 0, 0x00000bb },
    { GREEK, 0, 0x000203a }
};

static struct gchr_transform trans_braceright[] = {
    { 0, 0, 0x000201c }
};

static struct gchr_transform trans_asciitilde[] = {
    { ANY, TILDE, 0x0000000 }
};

struct gchr_lookup _gdraw_chrlookup[95] = {
    { 2, trans_space },		/*   */
    { 1, trans_exclam },	/* ! */
    { 1, trans_quotedbl },	/* " */
    { 2, trans_numbersign },	/* # */
    { 2, trans_dollar },	/* $ */
    { 0 },			/* % */
    { 0 },			/* & */
    { 1, trans_quotesingle },	/* ' */
    { 0 },			/* ( */
    { 0 },			/* ) */
    { 2, trans_asterisk },	/* * */
    { 1, trans_plus },		/* + */
    { 1, trans_comma },		/* , */
    { 2, trans_hyphenminus },	/* - */
    { 2, trans_period },	/* . */
    { 1, trans_slash },		/* / */
    { 1, trans_zero },		/* 0 */
    { 0 },			/* 1 */
    { 1, trans_two },		/* 2 */
    { 0 },			/* 3 */
    { 1, trans_four },		/* 4 */
    { 1, trans_five },		/* 5 */
    { 1, trans_six },		/* 6 */
    { 1, trans_seven },		/* 7 */
    { 0 },			/* 8 */
    { 0 },			/* 9 */
    { 1, trans_colon },		/* : */
    { 1, trans_semicolon },	/* ; */
    { 1, trans_less },		/* < */
    { 1, trans_equal },		/* = */
    { 2, trans_greater },	/* > */
    { 1, trans_question },	/* ? */
    { 1, trans_at },		/* @ */
    { 35, trans_A },		/* A */
    { 5, trans_B },		/* B */
    { 9, trans_C },		/* C */
    { 8, trans_D },		/* D */
    { 30, trans_E },		/* E */
    { 3, trans_F },		/* F */
    { 9, trans_G },		/* G */
    { 13, trans_H },		/* H */
    { 24, trans_I },		/* I */
    { 3, trans_J },		/* J */
    { 7, trans_K },		/* K */
    { 10, trans_L },		/* L */
    { 4, trans_M },		/* M */
    { 10, trans_N },		/* N */
    { 41, trans_O },		/* O */
    { 6, trans_P },		/* P */
    { 2, trans_Q },		/* Q */
    { 11, trans_R },		/* R */
    { 10, trans_S },		/* S */
    { 8, trans_T },		/* T */
    { 37, trans_U },		/* U */
    { 3, trans_V },		/* V */
    { 10, trans_W },		/* W */
    { 3, trans_X },		/* X */
    { 11, trans_Y },		/* Y */
    { 8, trans_Z },		/* Z */
    { 1, trans_bracketleft },	/* [ */
    { 2, trans_backslash },	/* \ */
    { 1, trans_bracketright },	/* ] */
    { 1, trans_asciicircum },	/* ^ */
    { 2, trans_underscore },	/* _ */
    { 1, trans_grave },		/* ` */
    { 38, trans_a },		/* a */
    { 5, trans_b },		/* b */
    { 9, trans_c },		/* c */
    { 8, trans_d },		/* d */
    { 31, trans_e },		/* e */
    { 4, trans_f },		/* f */
    { 10, trans_g },		/* g */
    { 16, trans_h },		/* h */
    { 30, trans_i },		/* i */
    { 4, trans_j },		/* j */
    { 7, trans_k },		/* k */
    { 10, trans_l },		/* l */
    { 5, trans_m },		/* m */
    { 11, trans_n },		/* n */
    { 41, trans_o },		/* o */
    { 6, trans_p },		/* p */
    { 2, trans_q },		/* q */
    { 12, trans_r },		/* r */
    { 11, trans_s },		/* s */
    { 10, trans_t },		/* t */
    { 43, trans_u },		/* u */
    { 3, trans_v },		/* v */
    { 11, trans_w },		/* w */
    { 3, trans_x },		/* x */
    { 12, trans_y },		/* y */
    { 9, trans_z },		/* z */
    { 1, trans_braceleft },	/* { */
    { 2, trans_bar },		/* | */
    { 1, trans_braceright },	/* } */
    { 1, trans_asciitilde },	/* ~ */
};

struct gchr_accents _gdraw_accents[] = {
    { 0x0301, 0x0000001 },
    { 0x0300, 0x0000002 },
    { 0x0308, 0x0000004 },
    { 0x0302, 0x0000008 },
    { 0x0303, 0x0000010 },
    { 0x030a, 0x0000020 },
    { 0x0338, 0x0000040 },
    { 0x0306, 0x0000080 },
    { 0x030c, 0x0000100 },
    { 0x0307, 0x0000200 },
    { 0x0323, 0x0000400 },
    { 0x0327, 0x0000800 },
    { 0x0328, 0x0001000 },
    { 0x0304, 0x0002000 },
    { 0x030d, 0x8004000 },
    { 0x030b, 0x0004000 },
    { 0x030b, 0x0008000 },
    { 0x030b, 0x0010000 },
    { 0x030b, 0x0020000 },
    { 0x030b, 0x0040000 },
    { 0x030b, 0x0080000 },
    { 0x030b, 0x0100000 },
    { 0x030b, 0x0200000 },
    { 0x030b, 0x0400000 },
    { 0x030b, 0x0800000 },
    { 0, 0 },
};

uint32 _gdraw_chrs_any=ANY, _gdraw_chrs_ctlmask=GREEK, _gdraw_chrs_metamask=0;
