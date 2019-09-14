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

#include <fontforge-config.h>

#include "dumppfa.h"
#include "encoding.h"
#include "fontforge.h"
#include "gfile.h"
#include "gutils.h"
#include "parsettf.h"
#include "psfont.h"
#include "psread.h"
#include "ustring.h"
#include "utype.h"

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

struct fontparse {
    FontDict *fd, *mainfd;
    /* always in font data */
    unsigned int infi:1;		/* in definition of FontInfo dict */
    unsigned int inchars:1;
    unsigned int inprivate:1;
    unsigned int insubs:1;
    unsigned int inmetrics: 1;
    unsigned int inmetrics2: 1;
    unsigned int inbb: 1;
    unsigned int inencoding: 1;
    unsigned int simpleencoding: 1;
    unsigned int multiline: 1;
    unsigned int incidsysteminfo: 1;
    unsigned int inblendfi:1;
    unsigned int inblendprivate:1;
    unsigned int skipping_mbf: 1;
    unsigned int inblend: 1;
    unsigned int iscid: 1;
    unsigned int iscff: 1;
    unsigned int useshexstrings: 1;
    unsigned int doneencoding: 1;
    unsigned int ignore: 1;
    int simple_enc_pos;
    int instring;
    int fdindex;
    char **pending_parse;
    FILE *sfnts;

    unsigned int alreadycomplained: 1;

    char *vbuf, *vmax, *vpt;
    int depth;
};

static void copyenc(char *encoding[256], const char *std[256]) {
    int i;
    for ( i=0; i<256; ++i )
	encoding[i] = copy(std[i]);
}

const char *AdobeStandardEncoding[] = {
/* 0000 */	".notdef",
/* 0001 */	".notdef",
/* 0002 */	".notdef",
/* 0003 */	".notdef",
/* 0004 */	".notdef",
/* 0005 */	".notdef",
/* 0006 */	".notdef",
/* 0007 */	".notdef",
/* 0008 */	".notdef",
/* 0009 */	".notdef",
/* 000a */	".notdef",
/* 000b */	".notdef",
/* 000c */	".notdef",
/* 000d */	".notdef",
/* 000e */	".notdef",
/* 000f */	".notdef",
/* 0010 */	".notdef",
/* 0011 */	".notdef",
/* 0012 */	".notdef",
/* 0013 */	".notdef",
/* 0014 */	".notdef",
/* 0015 */	".notdef",
/* 0016 */	".notdef",
/* 0017 */	".notdef",
/* 0018 */	".notdef",
/* 0019 */	".notdef",
/* 001a */	".notdef",
/* 001b */	".notdef",
/* 001c */	".notdef",
/* 001d */	".notdef",
/* 001e */	".notdef",
/* 001f */	".notdef",
/* 0020 */	"space",
/* 0021 */	"exclam",
/* 0022 */	"quotedbl",
/* 0023 */	"numbersign",
/* 0024 */	"dollar",
/* 0025 */	"percent",
/* 0026 */	"ampersand",
/* 0027 */	"quoteright",
/* 0028 */	"parenleft",
/* 0029 */	"parenright",
/* 002a */	"asterisk",
/* 002b */	"plus",
/* 002c */	"comma",
/* 002d */	"hyphen",
/* 002e */	"period",
/* 002f */	"slash",
/* 0030 */	"zero",
/* 0031 */	"one",
/* 0032 */	"two",
/* 0033 */	"three",
/* 0034 */	"four",
/* 0035 */	"five",
/* 0036 */	"six",
/* 0037 */	"seven",
/* 0038 */	"eight",
/* 0039 */	"nine",
/* 003a */	"colon",
/* 003b */	"semicolon",
/* 003c */	"less",
/* 003d */	"equal",
/* 003e */	"greater",
/* 003f */	"question",
/* 0040 */	"at",
/* 0041 */	"A",
/* 0042 */	"B",
/* 0043 */	"C",
/* 0044 */	"D",
/* 0045 */	"E",
/* 0046 */	"F",
/* 0047 */	"G",
/* 0048 */	"H",
/* 0049 */	"I",
/* 004a */	"J",
/* 004b */	"K",
/* 004c */	"L",
/* 004d */	"M",
/* 004e */	"N",
/* 004f */	"O",
/* 0050 */	"P",
/* 0051 */	"Q",
/* 0052 */	"R",
/* 0053 */	"S",
/* 0054 */	"T",
/* 0055 */	"U",
/* 0056 */	"V",
/* 0057 */	"W",
/* 0058 */	"X",
/* 0059 */	"Y",
/* 005a */	"Z",
/* 005b */	"bracketleft",
/* 005c */	"backslash",
/* 005d */	"bracketright",
/* 005e */	"asciicircum",
/* 005f */	"underscore",
/* 0060 */	"quoteleft",
/* 0061 */	"a",
/* 0062 */	"b",
/* 0063 */	"c",
/* 0064 */	"d",
/* 0065 */	"e",
/* 0066 */	"f",
/* 0067 */	"g",
/* 0068 */	"h",
/* 0069 */	"i",
/* 006a */	"j",
/* 006b */	"k",
/* 006c */	"l",
/* 006d */	"m",
/* 006e */	"n",
/* 006f */	"o",
/* 0070 */	"p",
/* 0071 */	"q",
/* 0072 */	"r",
/* 0073 */	"s",
/* 0074 */	"t",
/* 0075 */	"u",
/* 0076 */	"v",
/* 0077 */	"w",
/* 0078 */	"x",
/* 0079 */	"y",
/* 007a */	"z",
/* 007b */	"braceleft",
/* 007c */	"bar",
/* 007d */	"braceright",
/* 007e */	"asciitilde",
/* 007f */	".notdef",
/* 0080 */	".notdef",
/* 0081 */	".notdef",
/* 0082 */	".notdef",
/* 0083 */	".notdef",
/* 0084 */	".notdef",
/* 0085 */	".notdef",
/* 0086 */	".notdef",
/* 0087 */	".notdef",
/* 0088 */	".notdef",
/* 0089 */	".notdef",
/* 008a */	".notdef",
/* 008b */	".notdef",
/* 008c */	".notdef",
/* 008d */	".notdef",
/* 008e */	".notdef",
/* 008f */	".notdef",
/* 0090 */	".notdef",
/* 0091 */	".notdef",
/* 0092 */	".notdef",
/* 0093 */	".notdef",
/* 0094 */	".notdef",
/* 0095 */	".notdef",
/* 0096 */	".notdef",
/* 0097 */	".notdef",
/* 0098 */	".notdef",
/* 0099 */	".notdef",
/* 009a */	".notdef",
/* 009b */	".notdef",
/* 009c */	".notdef",
/* 009d */	".notdef",
/* 009e */	".notdef",
/* 009f */	".notdef",
/* 00a0 */	".notdef",
/* 00a1 */	"exclamdown",
/* 00a2 */	"cent",
/* 00a3 */	"sterling",
/* 00a4 */	"fraction",
/* 00a5 */	"yen",
/* 00a6 */	"florin",
/* 00a7 */	"section",
/* 00a8 */	"currency",
/* 00a9 */	"quotesingle",
/* 00aa */	"quotedblleft",
/* 00ab */	"guillemotleft",
/* 00ac */	"guilsinglleft",
/* 00ad */	"guilsinglright",
/* 00ae */	"fi",
/* 00af */	"fl",
/* 00b0 */	".notdef",
/* 00b1 */	"endash",
/* 00b2 */	"dagger",
/* 00b3 */	"daggerdbl",
/* 00b4 */	"periodcentered",
/* 00b5 */	".notdef",
/* 00b6 */	"paragraph",
/* 00b7 */	"bullet",
/* 00b8 */	"quotesinglbase",
/* 00b9 */	"quotedblbase",
/* 00ba */	"quotedblright",
/* 00bb */	"guillemotright",
/* 00bc */	"ellipsis",
/* 00bd */	"perthousand",
/* 00be */	".notdef",
/* 00bf */	"questiondown",
/* 00c0 */	".notdef",
/* 00c1 */	"grave",
/* 00c2 */	"acute",
/* 00c3 */	"circumflex",
/* 00c4 */	"tilde",
/* 00c5 */	"macron",
/* 00c6 */	"breve",
/* 00c7 */	"dotaccent",
/* 00c8 */	"dieresis",
/* 00c9 */	".notdef",
/* 00ca */	"ring",
/* 00cb */	"cedilla",
/* 00cc */	".notdef",
/* 00cd */	"hungarumlaut",
/* 00ce */	"ogonek",
/* 00cf */	"caron",
/* 00d0 */	"emdash",
/* 00d1 */	".notdef",
/* 00d2 */	".notdef",
/* 00d3 */	".notdef",
/* 00d4 */	".notdef",
/* 00d5 */	".notdef",
/* 00d6 */	".notdef",
/* 00d7 */	".notdef",
/* 00d8 */	".notdef",
/* 00d9 */	".notdef",
/* 00da */	".notdef",
/* 00db */	".notdef",
/* 00dc */	".notdef",
/* 00dd */	".notdef",
/* 00de */	".notdef",
/* 00df */	".notdef",
/* 00e0 */	".notdef",
/* 00e1 */	"AE",
/* 00e2 */	".notdef",
/* 00e3 */	"ordfeminine",
/* 00e4 */	".notdef",
/* 00e5 */	".notdef",
/* 00e6 */	".notdef",
/* 00e7 */	".notdef",
/* 00e8 */	"Lslash",
/* 00e9 */	"Oslash",
/* 00ea */	"OE",
/* 00eb */	"ordmasculine",
/* 00ec */	".notdef",
/* 00ed */	".notdef",
/* 00ee */	".notdef",
/* 00ef */	".notdef",
/* 00f0 */	".notdef",
/* 00f1 */	"ae",
/* 00f2 */	".notdef",
/* 00f3 */	".notdef",
/* 00f4 */	".notdef",
/* 00f5 */	"dotlessi",
/* 00f6 */	".notdef",
/* 00f7 */	".notdef",
/* 00f8 */	"lslash",
/* 00f9 */	"oslash",
/* 00fa */	"oe",
/* 00fb */	"germandbls",
/* 00fc */	".notdef",
/* 00fd */	".notdef",
/* 00fe */	".notdef",
/* 00ff */	".notdef"
};
static void setStdEnc(char *encoding[256]) {
    copyenc(encoding,AdobeStandardEncoding);
}

static void setLatin1Enc(char *encoding[256]) {
    static const char *latin1enc[] = {
/* 0000 */	".notdef",
/* 0001 */	".notdef",
/* 0002 */	".notdef",
/* 0003 */	".notdef",
/* 0004 */	".notdef",
/* 0005 */	".notdef",
/* 0006 */	".notdef",
/* 0007 */	".notdef",
/* 0008 */	".notdef",
/* 0009 */	".notdef",
/* 000a */	".notdef",
/* 000b */	".notdef",
/* 000c */	".notdef",
/* 000d */	".notdef",
/* 000e */	".notdef",
/* 000f */	".notdef",
/* 0010 */	".notdef",
/* 0011 */	".notdef",
/* 0012 */	".notdef",
/* 0013 */	".notdef",
/* 0014 */	".notdef",
/* 0015 */	".notdef",
/* 0016 */	".notdef",
/* 0017 */	".notdef",
/* 0018 */	".notdef",
/* 0019 */	".notdef",
/* 001a */	".notdef",
/* 001b */	".notdef",
/* 001c */	".notdef",
/* 001d */	".notdef",
/* 001e */	".notdef",
/* 001f */	".notdef",
/* 0020 */	"space",
/* 0021 */	"exclam",
/* 0022 */	"quotedbl",
/* 0023 */	"numbersign",
/* 0024 */	"dollar",
/* 0025 */	"percent",
/* 0026 */	"ampersand",
/* 0027 */	"quoteright",
/* 0028 */	"parenleft",
/* 0029 */	"parenright",
/* 002a */	"asterisk",
/* 002b */	"plus",
/* 002c */	"comma",
/* 002d */	"hyphen",
/* 002e */	"period",
/* 002f */	"slash",
/* 0030 */	"zero",
/* 0031 */	"one",
/* 0032 */	"two",
/* 0033 */	"three",
/* 0034 */	"four",
/* 0035 */	"five",
/* 0036 */	"six",
/* 0037 */	"seven",
/* 0038 */	"eight",
/* 0039 */	"nine",
/* 003a */	"colon",
/* 003b */	"semicolon",
/* 003c */	"less",
/* 003d */	"equal",
/* 003e */	"greater",
/* 003f */	"question",
/* 0040 */	"at",
/* 0041 */	"A",
/* 0042 */	"B",
/* 0043 */	"C",
/* 0044 */	"D",
/* 0045 */	"E",
/* 0046 */	"F",
/* 0047 */	"G",
/* 0048 */	"H",
/* 0049 */	"I",
/* 004a */	"J",
/* 004b */	"K",
/* 004c */	"L",
/* 004d */	"M",
/* 004e */	"N",
/* 004f */	"O",
/* 0050 */	"P",
/* 0051 */	"Q",
/* 0052 */	"R",
/* 0053 */	"S",
/* 0054 */	"T",
/* 0055 */	"U",
/* 0056 */	"V",
/* 0057 */	"W",
/* 0058 */	"X",
/* 0059 */	"Y",
/* 005a */	"Z",
/* 005b */	"bracketleft",
/* 005c */	"backslash",
/* 005d */	"bracketright",
/* 005e */	"asciicircum",
/* 005f */	"underscore",
/* 0060 */	"grave",
/* 0061 */	"a",
/* 0062 */	"b",
/* 0063 */	"c",
/* 0064 */	"d",
/* 0065 */	"e",
/* 0066 */	"f",
/* 0067 */	"g",
/* 0068 */	"h",
/* 0069 */	"i",
/* 006a */	"j",
/* 006b */	"k",
/* 006c */	"l",
/* 006d */	"m",
/* 006e */	"n",
/* 006f */	"o",
/* 0070 */	"p",
/* 0071 */	"q",
/* 0072 */	"r",
/* 0073 */	"s",
/* 0074 */	"t",
/* 0075 */	"u",
/* 0076 */	"v",
/* 0077 */	"w",
/* 0078 */	"x",
/* 0079 */	"y",
/* 007a */	"z",
/* 007b */	"braceleft",
/* 007c */	"bar",
/* 007d */	"braceright",
/* 007e */	"asciitilde",
/* 007f */	".notdef",
/* 0080 */	".notdef",
/* 0081 */	".notdef",
/* 0082 */	".notdef",
/* 0083 */	".notdef",
/* 0084 */	".notdef",
/* 0085 */	".notdef",
/* 0086 */	".notdef",
/* 0087 */	".notdef",
/* 0088 */	".notdef",
/* 0089 */	".notdef",
/* 008a */	".notdef",
/* 008b */	".notdef",
/* 008c */	".notdef",
/* 008d */	".notdef",
/* 008e */	".notdef",
/* 008f */	".notdef",
/* 0090 */	"dotlessi",		/* Um, Adobe's Latin1 has some extra chars */
/* 0091 */	"grave",
/* 0092 */	"accute",		/* This is a duplicate... */
/* 0093 */	"circumflex",
/* 0094 */	"tilde",
/* 0095 */	"macron",
/* 0096 */	"breve",
/* 0097 */	"dotaccent",
/* 0098 */	"dieresis",
/* 0099 */	".notdef",
/* 009a */	"ring",
/* 009b */	"cedilla",
/* 009c */	".notdef",
/* 009d */	"hungarumlaut",
/* 009e */	"ogonek",
/* 009f */	"caron",
/* 00a0 */	"space",
/* 00a1 */	"exclamdown",
/* 00a2 */	"cent",
/* 00a3 */	"sterling",
/* 00a4 */	"currency",
/* 00a5 */	"yen",
/* 00a6 */	"brokenbar",
/* 00a7 */	"section",
/* 00a8 */	"dieresis",
/* 00a9 */	"copyright",
/* 00aa */	"ordfeminine",
/* 00ab */	"guillemotleft",
/* 00ac */	"logicalnot",
/* 00ad */	"hyphen",
/* 00ae */	"registered",
/* 00af */	"macron",
/* 00b0 */	"degree",
/* 00b1 */	"plusminus",
/* 00b2 */	"twosuperior",
/* 00b3 */	"threesuperior",
/* 00b4 */	"acute",
/* 00b5 */	"mu",
/* 00b6 */	"paragraph",
/* 00b7 */	"periodcentered",
/* 00b8 */	"cedilla",
/* 00b9 */	"onesuperior",
/* 00ba */	"ordmasculine",
/* 00bb */	"guillemotright",
/* 00bc */	"onequarter",
/* 00bd */	"onehalf",
/* 00be */	"threequarters",
/* 00bf */	"questiondown",
/* 00c0 */	"Agrave",
/* 00c1 */	"Aacute",
/* 00c2 */	"Acircumflex",
/* 00c3 */	"Atilde",
/* 00c4 */	"Adieresis",
/* 00c5 */	"Aring",
/* 00c6 */	"AE",
/* 00c7 */	"Ccedilla",
/* 00c8 */	"Egrave",
/* 00c9 */	"Eacute",
/* 00ca */	"Ecircumflex",
/* 00cb */	"Edieresis",
/* 00cc */	"Igrave",
/* 00cd */	"Iacute",
/* 00ce */	"Icircumflex",
/* 00cf */	"Idieresis",
/* 00d0 */	"Eth",
/* 00d1 */	"Ntilde",
/* 00d2 */	"Ograve",
/* 00d3 */	"Oacute",
/* 00d4 */	"Ocircumflex",
/* 00d5 */	"Otilde",
/* 00d6 */	"Odieresis",
/* 00d7 */	"multiply",
/* 00d8 */	"Oslash",
/* 00d9 */	"Ugrave",
/* 00da */	"Uacute",
/* 00db */	"Ucircumflex",
/* 00dc */	"Udieresis",
/* 00dd */	"Yacute",
/* 00de */	"Thorn",
/* 00df */	"germandbls",
/* 00e0 */	"agrave",
/* 00e1 */	"aacute",
/* 00e2 */	"acircumflex",
/* 00e3 */	"atilde",
/* 00e4 */	"adieresis",
/* 00e5 */	"aring",
/* 00e6 */	"ae",
/* 00e7 */	"ccedilla",
/* 00e8 */	"egrave",
/* 00e9 */	"eacute",
/* 00ea */	"ecircumflex",
/* 00eb */	"edieresis",
/* 00ec */	"igrave",
/* 00ed */	"iacute",
/* 00ee */	"icircumflex",
/* 00ef */	"idieresis",
/* 00f0 */	"eth",
/* 00f1 */	"ntilde",
/* 00f2 */	"ograve",
/* 00f3 */	"oacute",
/* 00f4 */	"ocircumflex",
/* 00f5 */	"otilde",
/* 00f6 */	"odieresis",
/* 00f7 */	"divide",
/* 00f8 */	"oslash",
/* 00f9 */	"ugrave",
/* 00fa */	"uacute",
/* 00fb */	"ucircumflex",
/* 00fc */	"udieresis",
/* 00fd */	"yacute",
/* 00fe */	"thorn",
/* 00ff */	"ydieresis"
    };
    copyenc(encoding,latin1enc);
}

const char *AdobeExpertEncoding[] = {
/* 0000 */	".notdef",
/* 0001 */	".notdef",
/* 0002 */	".notdef",
/* 0003 */	".notdef",
/* 0004 */	".notdef",
/* 0005 */	".notdef",
/* 0006 */	".notdef",
/* 0007 */	".notdef",
/* 0008 */	".notdef",
/* 0009 */	".notdef",
/* 000a */	".notdef",
/* 000b */	".notdef",
/* 000c */	".notdef",
/* 000d */	".notdef",
/* 000e */	".notdef",
/* 000f */	".notdef",
/* 0010 */	".notdef",
/* 0011 */	".notdef",
/* 0012 */	".notdef",
/* 0013 */	".notdef",
/* 0014 */	".notdef",
/* 0015 */	".notdef",
/* 0016 */	".notdef",
/* 0017 */	".notdef",
/* 0018 */	".notdef",
/* 0019 */	".notdef",
/* 001a */	".notdef",
/* 001b */	".notdef",
/* 001c */	".notdef",
/* 001d */	".notdef",
/* 001e */	".notdef",
/* 001f */	".notdef",
/* 0020 */	"space",
/* 0021 */	"exclamsmall",
/* 0022 */	"Hungarumlautsmal",
/* 0023 */	".notdef",
/* 0024 */	"dollaroldstyle",
/* 0025 */	"dollarsuperior",
/* 0026 */	"ampersandsmall",
/* 0027 */	"Acutesmall",
/* 0028 */	"parenleftsuperior",
/* 0029 */	"parenrightsuperior",
/* 002a */	"twodotenleader",
/* 002b */	"onedotenleader",
/* 002c */	"comma",
/* 002d */	"hyphen",
/* 002e */	"period",
/* 002f */	"fraction",
/* 0030 */	"zerooldstyle",
/* 0031 */	"oneoldstyle",
/* 0032 */	"twooldstyle",
/* 0033 */	"threeoldstyle",
/* 0034 */	"fouroldstyle",
/* 0035 */	"fiveoldstyle",
/* 0036 */	"sixoldstyle",
/* 0037 */	"sevenoldstyle",
/* 0038 */	"eightoldstyle",
/* 0039 */	"nineoldstyle",
/* 003a */	"colon",
/* 003b */	"semicolon",
/* 003c */	"commasuperior",
/* 003d */	"threequartersemdash",
/* 003e */	"periodsuperior",
/* 003f */	"questionsmall",
/* 0040 */	".notdef",
/* 0041 */	"asuperior",
/* 0042 */	"bsuperior",
/* 0043 */	"centsuperior",
/* 0044 */	"dsuperior",
/* 0045 */	"esuperior",
/* 0046 */	".notdef",
/* 0047 */	".notdef",
/* 0048 */	".notdef",
/* 0049 */	"isuperior",
/* 004a */	".notdef",
/* 004b */	".notdef",
/* 004c */	"lsuperior",
/* 004d */	"msuperior",
/* 004e */	"nsuperior",
/* 004f */	"osuperior",
/* 0050 */	".notdef",
/* 0051 */	".notdef",
/* 0052 */	"rsuperior",
/* 0053 */	"ssuperior",
/* 0054 */	"tsuperior",
/* 0055 */	".notdef",
/* 0056 */	"ff",
/* 0057 */	"fi",
/* 0058 */	"fl",
/* 0059 */	"ffi",
/* 005a */	"ffl",
/* 005b */	"parenleftinferior",
/* 005c */	".notdef",
/* 005d */	"parenrightinferior",
/* 005e */	"Circumflexsmall",
/* 005f */	"hyphensuperior",
/* 0060 */	"Gravesmall",
/* 0061 */	"Asmall",
/* 0062 */	"Bsmall",
/* 0063 */	"Csmall",
/* 0064 */	"Dsmall",
/* 0065 */	"Esmall",
/* 0066 */	"Fsmall",
/* 0067 */	"Gsmall",
/* 0068 */	"Hsmall",
/* 0069 */	"Ismall",
/* 006a */	"Jsmall",
/* 006b */	"Ksmall",
/* 006c */	"Lsmall",
/* 006d */	"Msmall",
/* 006e */	"Nsmall",
/* 006f */	"Osmall",
/* 0070 */	"Psmall",
/* 0071 */	"Qsmall",
/* 0072 */	"Rsmall",
/* 0073 */	"Ssmall",
/* 0074 */	"Tsmall",
/* 0075 */	"Usmall",
/* 0076 */	"Vsmall",
/* 0077 */	"Wsmall",
/* 0078 */	"Xsmall",
/* 0079 */	"Ysmall",
/* 007a */	"Zsmall",
/* 007b */	"colonmonetary",
/* 007c */	"onefitted",
/* 007d */	"rupiah",
/* 007e */	"Tildesmall",
/* 007f */	".notdef",
/* 0080 */	".notdef",
/* 0081 */	".notdef",
/* 0082 */	".notdef",
/* 0083 */	".notdef",
/* 0084 */	".notdef",
/* 0085 */	".notdef",
/* 0086 */	".notdef",
/* 0087 */	".notdef",
/* 0088 */	".notdef",
/* 0089 */	".notdef",
/* 008a */	".notdef",
/* 008b */	".notdef",
/* 008c */	".notdef",
/* 008d */	".notdef",
/* 008e */	".notdef",
/* 008f */	".notdef",
/* 0090 */	".notdef",
/* 0091 */	".notdef",
/* 0092 */	".notdef",
/* 0093 */	".notdef",
/* 0094 */	".notdef",
/* 0095 */	".notdef",
/* 0096 */	".notdef",
/* 0097 */	".notdef",
/* 0098 */	".notdef",
/* 0099 */	".notdef",
/* 009a */	".notdef",
/* 009b */	".notdef",
/* 009c */	".notdef",
/* 009d */	".notdef",
/* 009e */	".notdef",
/* 009f */	".notdef",
/* 00a0 */	".notdef",
/* 00a1 */	"exclamdownsmall",
/* 00a2 */	"centoldstyle",
/* 00a3 */	"Lslashsmall",
/* 00a4 */	".notdef",
/* 00a5 */	".notdef",
/* 00a6 */	"Scaronsmall",
/* 00a7 */	"Zcaronsmall",
/* 00a8 */	"Dieresissmall",
/* 00a9 */	"Brevesmall",
/* 00aa */	"Caronsmall",
/* 00ab */	".notdef",
/* 00ac */	"Dotaccentsmall",
/* 00ad */	".notdef",
/* 00ae */	".notdef",
/* 00af */	"Macronsmall",
/* 00b0 */	".notdef",
/* 00b1 */	".notdef",
/* 00b2 */	"figuredash",
/* 00b3 */	"hypheninferior",
/* 00b4 */	".notdef",
/* 00b5 */	".notdef",
/* 00b6 */	"Ogoneksmall",
/* 00b7 */	"Ringsmall",
/* 00b8 */	"Cedillasmall",
/* 00b9 */	".notdef",
/* 00ba */	".notdef",
/* 00bb */	".notdef",
/* 00bc */	"onequarter",
/* 00bd */	"onehalf",
/* 00be */	"threequarters",
/* 00bf */	"questiondownsmall",
/* 00c0 */	"oneeighth",
/* 00c1 */	"threeeighths",
/* 00c2 */	"fiveeighths",
/* 00c3 */	"seveneighths",
/* 00c4 */	"onethird",
/* 00c5 */	"twothirds",
/* 00c6 */	".notdef",
/* 00c7 */	".notdef",
/* 00c8 */	"zerosuperior",
/* 00c9 */	"onesuperior",
/* 00ca */	"twosuperior",
/* 00cb */	"threesuperior",
/* 00cc */	"foursuperior",
/* 00cd */	"fivesuperior",
/* 00ce */	"sixsuperior",
/* 00cf */	"sevensuperior",
/* 00d0 */	"eightsuperior",
/* 00d1 */	"ninesuperior",
/* 00d2 */	"zeroinferior",
/* 00d3 */	"oneinferior",
/* 00d4 */	"twoinferior",
/* 00d5 */	"threeinferior",
/* 00d6 */	"fourinferior",
/* 00d7 */	"fiveinferior",
/* 00d8 */	"sixinferior",
/* 00d9 */	"seveninferior",
/* 00da */	"eightinferior",
/* 00db */	"nineinferior",
/* 00dc */	"centinferior",
/* 00dd */	"dollarinferior",
/* 00de */	"periodinferior",
/* 00df */	"commainferior",
/* 00e0 */	"Agravesmall",
/* 00e1 */	"Aacutesmall",
/* 00e2 */	"Acircumflexsmall",
/* 00e3 */	"Atildesmall",
/* 00e4 */	"Adieresissmall",
/* 00e5 */	"Aringsmall",
/* 00e6 */	"AEsmall",
/* 00e7 */	"Ccedillasmall",
/* 00e8 */	"Egravesmall",
/* 00e9 */	"Eacutesmall",
/* 00ea */	"Ecircumflexsmall",
/* 00eb */	"Edieresissmall",
/* 00ec */	"Igravesmall",
/* 00ed */	"Iacutesmall",
/* 00ee */	"Icircumflexsmall",
/* 00ef */	"Idieresissmall",
/* 00f0 */	"Ethsmall",
/* 00f1 */	"Ntildesmall",
/* 00f2 */	"Ogravesmall",
/* 00f3 */	"Oacutesmall",
/* 00f4 */	"Ocircumflexsmall",
/* 00f5 */	"Otildesmall",
/* 00f6 */	"Odieresissmall",
/* 00f7 */	"OEsmall",
/* 00f8 */	"Oslashsmall",
/* 00f9 */	"Ugravesmall",
/* 00fa */	"Uacutesmall",
/* 00fb */	"Ucircumflexsmall",
/* 00fc */	"Udieresissmall",
/* 00fd */	"Yacutesmall",
/* 00fe */	"Thornsmall",
/* 00ff */	"Ydieresissmall"
};

static struct fontdict *MakeEmptyFont(void) {
    struct fontdict *ret;

    ret = calloc(1,sizeof(struct fontdict));
    ret->fontinfo = calloc(1,sizeof(struct fontinfo));
    ret->chars = calloc(1,sizeof(struct pschars));
    ret->private = calloc(1,sizeof(struct private));
    ret->private->subrs = calloc(1,sizeof(struct pschars));
    ret->private->private = calloc(1,sizeof(struct psdict));
    ret->private->leniv = 4;
    ret->encoding_name = &custom;
    ret->fontinfo->fstype = -1;
return( ret );
}

static struct fontdict *PSMakeEmptyFont(void) {
    struct fontdict *ret = MakeEmptyFont();
    ret->charprocs = calloc(1,sizeof(struct charprocs));
return( ret );
}

static char *myfgets(char *str, int len, FILE *file) {
    char *pt, *end;
    int ch=0;

    for ( pt = str, end = str+len-1; pt<end && (ch=getc(file))!=EOF && ch!='\r' && ch!='\n';
	*pt++ = ch );
    if ( ch=='\n' )
	*pt++ = '\n';
    else if ( ch=='\r' ) {
	*pt++ = '\r';
	if ((ch=getc(file))!='\n' )
	    ungetc(ch,file);
	else
	    *pt++ = '\n';
    }
    if ( pt==str )
return( NULL );
    *pt = '\0';
return( str );
}

static char *myfgetsNoNulls(char *str, int len, FILE *file) {
    char *pt, *end;
    int ch=0;

    for ( pt = str, end = str+len-1; pt<end && (ch=getc(file))!=EOF && ch!='\r' && ch!='\n'; ) {
	if ( ch!='\0' )
	    *pt++ = ch;
    }
    if ( ch=='\n' )
	*pt++ = '\n';
    else if ( ch=='\r' ) {
	*pt++ = '\r';
	if ((ch=getc(file))!='\n' )
	    ungetc(ch,file);
	else
	    *pt++ = '\n';
    }
    if ( pt==str )
return( NULL );
    *pt = '\0';
return( str );
}

static char *getstring(char *start,FILE *in) {
    char *end, *ret;
    int parencnt=0, len=0;
    char buffer[512];

    for (;;) {
	while ( *start!='\0' && *start!='(' ) ++start;
	if ( *start=='\0' ) {
	    if ( myfgetsNoNulls(buffer,sizeof(buffer),in)==NULL )
return( copy(""));
	    start = buffer;
	} else
    break;
    }
    ++start;
    ret = NULL; len = 1;
    for (;;) {
	for ( end = start; *end!='\0' && (*end!=')' || parencnt>0); ++end ) {
	    if ( *end=='\\' && (end[1]=='(' || end[1]==')'))
		++end;
	    else if ( *end=='(' ) ++parencnt;
	    else if ( *end==')' ) --parencnt;
	}
	if ( end>start ) {
	    if ( ret==NULL )
		ret = malloc(end-start+1);
	    else
		ret = realloc(ret,len+end-start);
	    strncpy(ret+len-1,start,end-start);
	    len += end-start;
	    ret[len-1] = '\0';
	}
	if ( *end!='\0' )
    break;
	if ( myfgetsNoNulls(buffer,sizeof(buffer),in)==NULL )
return( ret );
	start = buffer;
    }
return( ret );
}

static char *gettoken(char *start) {
    char *end, *ret;

    while ( *start!='\0' && *start!='/' && *start!='(' ) ++start;
    if ( *start=='/' || *start=='(' ) ++start;
    for ( end = start; *end!='\0' && !isspace(*end) && *end!='[' && *end!='/' && *end!='{' && *end!='(' && *end!=')'; ++end );
    ret = malloc(end-start+1);
    if ( end>start )
	strncpy(ret,start,end-start);
    ret[end-start] = '\0';
return( ret );
}

static int getbool(char *start) {

    while ( isspace(*start) ) ++start;
    if ( *start=='T' || *start=='t' )
return( 1 );

return( 0 );
}

static void fillintarray(int *array,char *start,int maxentries) {
    int i;
    char *end;

    while ( *start!='\0' && *start!='[' && *start!='{' ) ++start;
    if ( *start=='[' || *start=='{' ) ++start;
    for ( i=0; i<maxentries && *start!=']' && *start!='}'; ++i ) {
	array[i] = (int) strtod(start,&end);
	if ( start==end )
return;
	start = end;
	while ( isspace(*start) ) ++start;
    }
}

static void fillrealarray(real *array,char *start,int maxentries) {
    int i;
    char *end;

    while ( *start!='\0' && *start!='[' && *start!='{' ) ++start;
    if ( *start=='[' || *start=='{' ) ++start;
    for ( i=0; i<maxentries && *start!=']' && *start!='}'; ++i ) {
	while ( isspace( *start )) ++start;
	if ( isdigit(*start) || *start=='-' || *start=='.' )
	    array[i] = strtod(start,&end);
	else if ( strncmp(start,"div",3)==0 && i>=2 ) {
	    /* Some of Luc Devroye's fonts have a "div" in the FontMatrix */
	    array[i-2] /= array[i-1];
	    i -= 2;
	    end = start+3;
	} else
return;
	if ( start==end )
return;
	start = end;
	while ( isspace(*start) ) ++start;
    }
}

static void InitDict(struct psdict *dict,char *line) {
    while ( *line!='/' && *line!='\0' ) ++line;
    while ( !isspace(*line) && *line!='\0' ) ++line;
    dict->cnt += strtol(line,NULL,10);
    if ( dict->next>0 ) { int i;		/* Shouldn't happen, but did in a bad file */
	dict->keys = realloc(dict->keys,dict->cnt*sizeof(char *));
	dict->values = realloc(dict->values,dict->cnt*sizeof(char *));
	for ( i=dict->next; i<dict->cnt; ++i ) {
	    dict->keys[i] = NULL; dict->values[i] = NULL;
	}
    } else {
	dict->keys = calloc(dict->cnt,sizeof(char *));
	dict->values = calloc(dict->cnt,sizeof(char *));
    }
}

static void InitChars(struct pschars *chars,char *line) {
    while ( *line!='/' && *line!='\0' ) ++line;
    while ( !isspace(*line) && *line!='\0' ) ++line;
    chars->cnt = strtol(line,NULL,10);
    if ( chars->cnt>0 ) {
	chars->keys = calloc(chars->cnt,sizeof(char *));
	chars->values = calloc(chars->cnt,sizeof(uint8 *));
	chars->lens = calloc(chars->cnt,sizeof(int));
	ff_progress_change_total(chars->cnt);
    }
}

static void InitCharProcs(struct charprocs *cp, char *line) {
    while ( *line!='/' && *line!='\0' ) ++line;
    while ( !isspace(*line) && *line!='\0' ) ++line;
    cp->cnt = strtol(line,NULL,10);
    if ( cp->cnt>0 ) {
	cp->keys = calloc(cp->cnt,sizeof(char *));
	cp->values = calloc(cp->cnt,sizeof(SplineChar *));
	ff_progress_change_total(cp->cnt);
    }
}

static int mycmp(char *str,char *within, char *end ) {
    while ( within<end ) {
	if ( *str!=*within )
return( *str-*within );
	++str; ++within;
    }
return( *str=='\0'?0:1 );
}

static void ContinueValue(struct fontparse *fp, struct psdict *dict, char *line) {
    int incomment = false;

    while ( *line ) {
	if ( !fp->instring && fp->depth==0 &&
		(strncmp(line,"def",3)==0 ||
		 strncmp(line,"|-",2)==0 || strncmp(line,"ND",2)==0)) {
	    while ( 1 ) {
		while ( fp->vpt>fp->vbuf+1 && isspace(fp->vpt[-1]) )
		    --fp->vpt;
		if ( fp->vpt>fp->vbuf+8 && strncmp(fp->vpt-8,"noaccess",8)==0 )
		    fp->vpt -= 8;
		else if ( fp->vpt>fp->vbuf+8 && strncmp(fp->vpt-8,"readonly",8)==0 )
		    fp->vpt -= 8;
		else if ( fp->vpt>fp->vbuf+4 && strncmp(fp->vpt-4,"bind",4)==0 )
		    fp->vpt -= 4;
		else
	    break;
	    }
	    /* In some URW fonts (Nimbus Sans L, n019003l) we get a complex */
	    /*  expression rather than just an array. This is ok. The expression */
	    /*  converts itself into an array. We could just truncate to the */
	    /*  default array, but I don't see any reason to do so */
	    if ( fp->pending_parse!=NULL ) {
		*fp->pending_parse = copyn(fp->vbuf,fp->vpt-fp->vbuf);
		fp->pending_parse = NULL;
	    } else {
		dict->values[dict->next] = copyn(fp->vbuf,fp->vpt-fp->vbuf);
		++dict->next;
	    }
	    fp->vpt = fp->vbuf;
	    fp->multiline = false;
return;
	}
	if ( fp->vpt>=fp->vmax ) {
	    int len = fp->vmax-fp->vbuf+1000, off=fp->vpt-fp->vbuf;
	    fp->vbuf = realloc(fp->vbuf,len);
	    fp->vpt = fp->vbuf+off;
	    fp->vmax = fp->vbuf+len;
	}
	if ( fp->instring ) {
	    if ( *line==')' ) --fp->instring;
	} else if ( incomment ) {
	    /* Do Nothing */;
	} else if ( *line=='(' )
	    ++fp->instring;
	else if ( *line=='%' )
	    incomment = true;
	else if ( *line=='[' || *line=='{' )
	    ++fp->depth;
	else if ( *line=='}' || *line==']' )
	    --fp->depth;
	*fp->vpt++ = *line++;
    }
}

static void AddValue(struct fontparse *fp, struct psdict *dict, char *line, char *endtok) {
    char *pt;

    if ( dict!=NULL ) {
	if ( dict->next>=dict->cnt ) {
	    dict->cnt += 10;
	    dict->keys = realloc(dict->keys,dict->cnt*sizeof(char *));
	    dict->values = realloc(dict->values,dict->cnt*sizeof(char *));
	}
	dict->keys[dict->next] = copyn(line+1,endtok-(line+1));
    }
    pt = line+strlen(line)-1;
    while ( isspace(*endtok)) ++endtok;
    while ( pt>endtok && isspace(*pt)) --pt;
    ++pt;
    if ( strncmp(pt-3,"def",3)==0 )
	pt -= 3;
    else if ( strncmp(pt-2,"|-",2)==0 || strncmp(pt-2,"ND",2)==0 )
	pt -= 2;
    else {
	fp->multiline = true;
	ContinueValue(fp,dict,endtok);
return;
    }
    for (;;) {
	while ( pt-1>endtok && isspace(pt[-1])) --pt;
	if ( pt-8>endtok && strncmp(pt-8,"noaccess",8)==0 )
	    pt -= 8;
	else if ( pt-8>endtok && strncmp(pt-8,"readonly",8)==0 )
	    pt -= 8;
	else if ( pt-4>endtok && strncmp(pt-4,"bind",4)==0 )
	    pt -= 4;
	else
	    break;
    }
    if ( dict!=NULL ) {
	dict->values[dict->next] = copyn(endtok,pt-endtok);
	++dict->next;
    } else {
	*fp->pending_parse = copyn(endtok,pt-endtok);
	fp->pending_parse = NULL;
    }
}

static int hex(int ch1, int ch2) {
/* Convert two HEX characters to one binary value. Return -1 if error */
/* NOTE: FIXME: parsepdf has an identical routine that can be merged. */

    if	    (ch1 >= '0' && ch1 <= '9') ch1 -='0';
    else if (ch1 >= 'A' && ch1 <= 'F') ch1 -=('A'-10);
    else if (ch1 >= 'a' && ch1 <= 'f') ch1 -=('a'-10);
    else return( -1 );

    if	    (ch2 >= '0' && ch2 <= '9') ch2 -='0';
    else if (ch2 >= 'A' && ch2 <= 'F') ch2 -=('A'-10);
    else if (ch2 >= 'a' && ch2 <= 'f') ch2 -=('a'-10);
    else return( -1 );

    return( (ch1<<4)|ch2 );
}

unsigned short r;
#define c1	52845
#define c2	22719

static void initcode(void) {
    r = 55665;
}

static int decode(unsigned char cypher) {
    unsigned char plain = ( cypher ^ (r>>8));
    r = (cypher + r) * c1 + c2;
return( plain );
}

static void dumpzeros(FILE *out, unsigned char *zeros, int zcnt) {
    while ( --zcnt >= 0 )
	fputc(*zeros++,out);
}

static void decodestr(unsigned char *str, int len) {
    unsigned short r = 4330;
    unsigned char plain, cypher;

    while ( len-->0 ) {
	cypher = *str;
	plain = ( cypher ^ (r>>8));
	r = (cypher + r) * c1 + c2;
	*str++ = plain;
    }
}

static void findstring(struct fontparse *fp,struct pschars *subrs,int index,char *nametok,char *str) {
    char buffer[1024], *bpt, *bs, *end = buffer+sizeof(buffer)-1;
    int val;

    while ( isspace(*str)) ++str;
    if ( *str=='(' ) {
	++str;
	bpt = buffer;
	while ( *str!=')' && *str!='\0' ) {
	    if ( *str!='\\' )
		val = *str++;
	    else {
		if ( isdigit( *++str )) {
		    val = *str++-'0';
		    if ( isdigit( *str )) {
			val = (val<<3) | (*str++-'0');
			if ( isdigit( *str ))
			    val = (val<<3) | (*str++-'0');
		    }
		} else
		    val = *str++;
	    }
	    if ( bpt<end )
		*bpt++ = val;
	}
	decodestr((unsigned char *) buffer,bpt-buffer);
	bs = buffer + fp->fd->private->leniv;
	if ( bpt<bs ) bs=bpt;		/* garbage */ 
	subrs->lens[index] = bpt-bs;
	subrs->keys[index] = copy(nametok);
	subrs->values[index] = malloc(bpt-bs);
	memcpy(subrs->values[index],bs,bpt-bs);
	if ( index>=subrs->next ) subrs->next = index+1;
    }
}

/* Type42 charstrings are actually numbers */
static void findnumbers(struct pschars *chars,char *str) {
    int val;
    char *end;

    for (;;) {
	int index = chars->next;
	char *namestrt;

	while ( isspace(*str)) ++str;
	if ( *str!='/' )
    break;
	namestrt = ++str;
	while ( isalnum(*str) || *str=='.' ) ++str;
	*str = '\0';
	index = chars->next;

	++str;
	val = strtol(str,&end,10);
	chars->lens[index] = 0;
	chars->keys[index] = copy(namestrt);
	chars->values[index] = (void *) (intpt) val;
	chars->next = index+1;
	str = end;
	while ( isspace(*str)) ++str;
	if ( str[0]=='d' && str[1]=='e' && str[2]=='f' )
	    str += 3;
    }
}

static char *rmbinary(char *line) {
    char *pt;

    for ( pt=line; *pt; ++pt ) {
	if (( *pt<' ' || *pt>=0x7f ) && *pt!='\n' ) {
	    if ( strlen(pt)>5 ) {
		pt[0] = '.';
		pt[1] = '.';
		pt[2] = '.';
		pt[3] = '\n';
		pt[4] = '\0';
	    } else {
		pt[0] = '\n';
		pt[1] = '\0';
	    }
	    break;
	}
    }
    return( line );
}

/* Does the buffer ending at "pt" end with "str"?  Look back as far as	*/
/* "n" chars. "pt" should point to final character in buffer, and not	*/
/* not a terminating null. Return 1 if matched, else 0 if error		*/
/* go see "Re: [Fontforge-devel] stuck in infinite loop", 2012August22	*/
static int matchFromBack(const char *pt, const char *str, int n) {
    int i, num_to_check = strlen(str);
    const char *strpt = str + num_to_check - 1;
    if ( n < num_to_check ) num_to_check = n;
    for (i = 0; i < num_to_check; i++) {
	if (*pt-- != *strpt--) return( 0 );
    }

    return( 1 );  /* they matched */
}

/* putBack():								*/
/* Puts "str", and "last" if not '\0' back onto front of filestream f;	*/
/* also moves "pt" back the same number of characters. This routine is	*/
/* is part of a large patch which is best described in the mailing list	*/
/* go see "Re: [Fontforge-devel] stuck in infinite loop", 2012August22	*/
static void putBack(struct fontparse *fp, FILE *f, const char *str, char last, char **pt) {
    if (last) {
	(*pt)--;
	if ( ungetc(last, f)<0 ) {
	    fp->alreadycomplained = 1;
	    LogError(_("Internal Err: Unable to put data back into file"));
	    return;
	}
    }

    const char *backpt;
    for (backpt = str + strlen(str) - 1; backpt >= str; backpt--) {
	if ( ungetc(*backpt, f)<0 ) {
	    fp->alreadycomplained = 1;
	    LogError(_("Internal Err: Unable to put data back into file"));
	    break;
	}
    }
    pt -= strlen(str);
}

static void sfnts2tempfile(struct fontparse *fp,FILE *in,char *line) {
    char *pt;
    int instring = false, firstnibble=true, sofar=0, nibble;
    int complained = false;
    int ch=0;

    fp->sfnts = GFileTmpfile();

    /* first finish off anything in the current line */
    while ( (pt=strpbrk(line,"<]" ))!=NULL ) {
	if ( *pt==']' )
  goto skip_to_eol;

	instring = true;
	for ( ++pt; *pt && *pt!='>'; ++pt ) {
	    if ( isspace(*pt))
	continue;
	    if ( isdigit(*pt))
		nibble = *pt-'0';
	    else if ( *pt>='a' && *pt<='f' )
		nibble = *pt-'a'+10;
	    else if ( *pt>='A' && *pt<='F' )
		nibble = *pt-'A'+10;
	    else {
		if ( !complained ) {
		    LogError( _("Invalid hex digit in sfnts array\n") );
		    complained = true;
		}
		++pt;
	continue;
	    }
	    if ( firstnibble ) {
		sofar = nibble<<4;
		firstnibble = false;
	    } else {
		putc(sofar|nibble,fp->sfnts);
		sofar = 0;
		firstnibble = true;
	    }
	}
	if ( *pt=='>' ) {
	    if ( ftell(fp->sfnts)&1 ) {	/* Strings must be contain an even number of bytes */
		/* But may be padded with a trailing NUL */
		fseek(fp->sfnts,-1,SEEK_CUR);
	    }
	    ++pt;
	    instring = false;
	}
	line = pt;
    }

    while ( (ch=getc(in))!=EOF ) {
	if ( ch==']' )
  goto skip_to_eol;
	if ( isspace(ch))
    continue;
	if ( !instring && ch=='<' ) {
	    instring = true;
	    firstnibble = true;
	    sofar = 0;
	} else if ( !instring ) {
	    if ( !complained ) {
		LogError( _("Invalid character outside of string in sfnts array\n") );
		complained = true;
	    }
	} else if ( instring && ch=='>' ) {
	    if ( ftell(fp->sfnts)&1 ) {	/* Strings must be contain an even number of bytes */
		/* But may be padded with a trailing NUL */
		fseek(fp->sfnts,-1,SEEK_CUR);
	    }
	    instring = false;
	} else {
	    if ( isdigit(ch))
		nibble = ch-'0';
	    else if ( ch>='a' && ch<='f' )
		nibble = ch-'a'+10;
	    else if ( ch>='A' && ch<='F' )
		nibble = ch-'A'+10;
	    else {
		if ( !complained ) {
		    LogError( _("Invalid hex digit in sfnts array\n") );
		    complained = true;
		}
    continue;
	    }
	    if ( firstnibble ) {
		sofar = nibble<<4;
		firstnibble = false;
	    } else {
		putc(sofar|nibble,fp->sfnts);
		sofar = 0;
		firstnibble = true;
	    }
	}
    }
  skip_to_eol:
    while ( ch!=EOF && ch!='\n' && ch!='\r' )
	ch = getc(in);
    rewind(fp->sfnts);
}

static void ParseSimpleEncoding(struct fontparse *fp,char *line) {
    char tok[200], *pt;

    while ( *line!='\0' && *line!=']' ) {
	while ( isspace(*line)) ++line;
	if ( *line==']' )
    break;
	if ( *line!='/' ) {
	    ++line;
    continue;
	}
	++line;
	while ( isspace(*line)) ++line;
	for ( pt=tok; !isspace(*line) && *line!='\0' && *line!='/' && *line!=']'; ) {
	    if ( pt<tok+sizeof(tok)-2 )
		*pt++ = *line++;
	    else
		++line;
	}
	*pt = '\0';
	if ( fp->simple_enc_pos<256 )
	    fp->fd->encoding[fp->simple_enc_pos++] = copy(tok);
    }
    if ( *line==']' ) {
	fp->simpleencoding = false;
	fp->inencoding = false;
    }
}

static void parseline(struct fontparse *fp,char *line,FILE *in) {
    char buffer[200], *pt, *endtok;

    while ( *line==' ' || *line=='\t' ) ++line;
    if ( line[0]=='%' && !fp->multiline )
return;

    if ( fp->simpleencoding ) {
	ParseSimpleEncoding(fp,line);
return;
    } else if (( fp->inencoding && strncmp(line,"dup",3)==0 ) ||
	    ( strncmp(line,"dup ",4)==0 && isdigit(line[4]) &&
	      strstr(line+strlen(line)-6," put")!=NULL && strchr(line,'/')!=NULL )) {
	/* Fontographer's type3 fonts claim to be standard, but then aren't */
	fp->fd->encoding_name = &custom;
	/* Metamorphasis has multiple entries on a line */
	while ( strncmp(line,"dup",3)==0 ) {
	    char *end;
	    int pos = strtol(line+3,&end,10);
	    line = end;
	    while ( isspace( *line )) ++line;
	    if ( *line=='/' ) ++line;
	    for ( pt = buffer; !isspace(*line); *pt++ = *line++ );
	    *pt = '\0';
	    if ( pos>=0 && pos<256 ) {
		free(fp->fd->encoding[pos]);
		fp->fd->encoding[pos] = copy(buffer);
	    }
	    while ( isspace(*line)) ++line;
	    if ( strncmp(line,"put",3)==0 ) line+=3;
	    while ( isspace(*line)) ++line;
	}
return;
    } else if ( fp->inencoding && strstr(line,"for")!=NULL && strstr(line,"/.notdef")!=NULL ) {
	/* the T1 spec I've got doesn't allow for this, but I've seen it anyway*/
	/* 0 1 255 {1 index exch /.notdef put} for */
	/* 0 1 31 { 1 index exch /.notdef put } bind for */
	int i;
	for ( i=0; i<256; ++i )
	    if ( fp->fd->encoding[i]==NULL )
		fp->fd->encoding[i] = copy(".notdef");
return;
    } else if ( fp->inencoding && strstr(line,"Encoding")!=NULL && strstr(line,"put")!=NULL ) {
	/* Saw a type 3 font with lines like "Encoding 1 /_a0 put" */
	char *end;
	int pos;
	while ( isspace(*line)) ++line;
	if ( strncmp(line,"Encoding ",9)==0 ) {
	    line+=9;
	    pos = strtol(line,&end,10);
	    line = end;
	    while ( isspace(*line)) ++line;
	    if ( *line=='/' ) {
		++line;
		for ( pt = buffer; !isspace(*line); *pt++ = *line++ );
		*pt = '\0';
		if ( pos>=0 && pos<256 )
		    fp->fd->encoding[pos] = copy(buffer);
	    }
	}
return;
    } else if ( fp->insubs ) {
	struct pschars *subrs = fp->fd->private->subrs;
	while ( isspace(*line)) ++line;
	if ( strncmp(line,"dup ",4)==0 ) {
	    int i;
	    char *ept;
	    for ( line += 4; *line==' '; ++line );
	    i = strtol(line,&ept,10);
	    if ( fp->ignore )
		/* Do Nothing */;
	    else if ( i<subrs->cnt ) {
		findstring(fp,subrs,i,NULL,ept);
	    } else if ( !fp->alreadycomplained ) {
		LogError( _("Index too big (must be <%d) \"%s"), subrs->cnt, rmbinary(line));
		fp->alreadycomplained = true;
	    }
	} else if ( strncmp(line, "readonly put", 12)==0 || strncmp(line, "ND", 2)==0 || strncmp(line, "|-", 2)==0 ) {
	    fp->insubs = false;
	    fp->ignore = false;
	} else if ( *line=='\n' || *line=='\0' ) {
	    /* Ignore blank lines */;
	} else if ( !fp->alreadycomplained ) {
	    LogError( _("Didn't understand \"%s\" inside subs def'n"), rmbinary(line) );
	    fp->alreadycomplained = true;
	}
    } else if ( fp->inchars ) {
	struct pschars *chars = fp->fd->chars;
	while ( isspace(*line)) ++line;
	if ( strncmp(line,"end",3)==0 )
	    fp->ignore = fp->inchars = false;
	else if ( *line=='\n' || *line=='\0' )
	    /* Ignore it */;
	else if ( *line!='/' || !(isalpha(line[1]) || line[1]=='.')) {
	    LogError( _("No name for CharStrings dictionary \"%s"), rmbinary(line) );
	    fp->alreadycomplained = true;
	} else if ( fp->ignore ) {
	    /* Do Nothing */;
	} else if ( chars->next>=chars->cnt )
	    LogError( _("Too many entries in CharStrings dictionary \"%s"), rmbinary(line) );
	else if ( fp->fd->fonttype==42 || fp->fd->fonttype==11 || fp->fd->cidfonttype==2 )
	    findnumbers(chars,line);
	else {
	    int i = chars->next;
	    char *namestrt = ++line;
	    while ( isalnum(*line) || *line=='.' ) ++line;
	    *line = '\0';
	    findstring(fp,chars,i,namestrt,line+1);
	    ff_progress_next();
	}
return;
    }
    fp->inencoding = 0;

    while ( isspace(*line)) ++line;
    endtok = NULL;
    if ( *line=='/' )
	for ( endtok=line+1; !isspace(*endtok) && *endtok!='(' && *endtok!='/' &&
		*endtok!='{' && *endtok!='[' && *endtok!='\0'; ++endtok );

    if ( strstr(line,"/shareddict")!=NULL && strstr(line,"where")!=NULL ) {
	fp->infi = fp->inbb = fp->inmetrics = fp->inmetrics2 = false;
	fp->inprivate = fp->inblendprivate = fp->inblendfi = false;
	fp->skipping_mbf = true;
return;
    }

    if ( mycmp("Encoding",line+1,endtok)==0 && !fp->doneencoding ) {
	if ( strstr(endtok,"StandardEncoding")!=NULL ) {
	    fp->fd->encoding_name = FindOrMakeEncoding("AdobeStandard");
	    setStdEnc(fp->fd->encoding);
	} else if ( strstr(endtok,"ISOLatin1Encoding")!=NULL ) {
	    fp->fd->encoding_name = FindOrMakeEncoding("ISO8859-1");
	    setLatin1Enc(fp->fd->encoding);
	} else {
	    fp->fd->encoding_name = &custom;
	    fp->inencoding = 1;
	}
	if ( fp->fd->encoding_name==NULL )
	    fp->fd->encoding_name = &custom;
	fp->infi = fp->inprivate = fp->inbb = fp->inmetrics = fp->inmetrics2 = false;
	fp->doneencoding = true;
	while ( *endtok==' ' || *endtok=='\t' ) ++endtok;
	if ( *endtok=='[' ) {	/* It's a literal array */
	    fp->simpleencoding = true;
	    fp->simple_enc_pos = 0;
	    ParseSimpleEncoding(fp,endtok+1);
	}
    } else if ( mycmp("BoundingBoxes",line+1,endtok)==0 ) {
	fp->infi = fp->inprivate = fp->inencoding = fp->inmetrics = fp->inmetrics2 = false;
	fp->inbb = true;
    } else if ( mycmp("Metrics",line+1,endtok)==0 ) {
	fp->infi = fp->inprivate = fp->inbb = fp->inencoding = fp->inmetrics2 = false;
	fp->inmetrics = true;
	fp->fd->metrics = calloc(1,sizeof(struct psdict));
	fp->fd->metrics->cnt = strtol(endtok,NULL,10);
	fp->fd->metrics->keys = malloc(fp->fd->metrics->cnt*sizeof(char *));
	fp->fd->metrics->values = malloc(fp->fd->metrics->cnt*sizeof(char *));
    } else if ( strstr(line,"/Private")!=NULL && strstr(line,"/Blend")!=NULL ) {
	fp->infi = fp->inbb = fp->inmetrics = fp->inmetrics2 = false;
	fp->inprivate = fp->inblendprivate = fp->inblendfi = false;
	fp->inblendprivate = 1;
	fp->fd->blendprivate = calloc(1,sizeof(struct psdict));
	InitDict(fp->fd->blendprivate,line);
return;
    } else if ( strstr(line,"/FontInfo")!=NULL && strstr(line,"/Blend")!=NULL ) {
	fp->infi = fp->inbb = fp->inmetrics = fp->inmetrics2 = false;
	fp->inprivate = fp->inblendprivate = fp->inblendfi = false;
	fp->inblendfi = 1;
	fp->fd->blendfontinfo = calloc(1,sizeof(struct psdict));
	InitDict(fp->fd->blendfontinfo,line);
return;
    } else if ( fp->infi ) {
	if ( fp->multiline ) {
	    ContinueValue(fp,NULL,line);
return;
	}
	if ( endtok==NULL && (strncmp(line,"end", 3)==0 || strncmp(line,">>",2)==0)) {
	    fp->infi=0;
return;
	} else if ( endtok==NULL )
return;
	if ( mycmp("version",line+1,endtok)==0 ) {
	    free(fp->fd->fontinfo->version);
	    fp->fd->fontinfo->version = getstring(endtok,in);
	} else if ( mycmp("Notice",line+1,endtok)==0 ) {
            free(fp->fd->fontinfo->notice);
	    fp->fd->fontinfo->notice = getstring(endtok,in);
	} else if ( mycmp("Copyright",line+1,endtok)==0 ) {		/* cff spec allows for copyright and notice */
            free(fp->fd->fontinfo->notice);
	    fp->fd->fontinfo->notice = getstring(endtok,in);
	} else if ( mycmp("FullName",line+1,endtok)==0 ) {
	    if ( fp->fd->fontinfo->fullname==NULL )
		fp->fd->fontinfo->fullname = getstring(endtok,in);
	    else
		free(getstring(endtok,in));
	} else if ( mycmp("FamilyName",line+1,endtok)==0 ) {
	    free( fp->fd->fontinfo->familyname );
	    fp->fd->fontinfo->familyname = getstring(endtok,in);
	} else if ( mycmp("Weight",line+1,endtok)==0 ) {
	    free( fp->fd->fontinfo->weight );
	    fp->fd->fontinfo->weight = getstring(endtok,in);
	} else if ( mycmp("ItalicAngle",line+1,endtok)==0 )
	    fp->fd->fontinfo->italicangle = strtod(endtok,NULL);
	else if ( mycmp("UnderlinePosition",line+1,endtok)==0 )
	    fp->fd->fontinfo->underlineposition = strtod(endtok,NULL);
	else if ( mycmp("UnderlineThickness",line+1,endtok)==0 )
	    fp->fd->fontinfo->underlinethickness = strtod(endtok,NULL);
	else if ( mycmp("isFixedPitch",line+1,endtok)==0 )
	    fp->fd->fontinfo->isfixedpitch = getbool(endtok);
	else if ( mycmp("em",line+1,endtok)==0 )
	    fp->fd->fontinfo->em = strtol(endtok,NULL,10);
	else if ( mycmp("ascent",line+1,endtok)==0 )
	    fp->fd->fontinfo->ascent = strtol(endtok,NULL,10);
	else if ( mycmp("descent",line+1,endtok)==0 )
	    fp->fd->fontinfo->descent = strtol(endtok,NULL,10);
	else if ( mycmp("FSType",line+1,endtok)==0 )
	    fp->fd->fontinfo->fstype = strtol(endtok,NULL,10);
	else if ( mycmp("BlendDesignPositions",line+1,endtok)==0 ) {
	    fp->pending_parse = &fp->fd->fontinfo->blenddesignpositions;
	    AddValue(fp,NULL,line,endtok);
	} else if ( mycmp("BlendDesignMap",line+1,endtok)==0 ) {
	    fp->pending_parse = &fp->fd->fontinfo->blenddesignmap;
	    AddValue(fp,NULL,line,endtok);
	} else if ( mycmp("BlendAxisTypes",line+1,endtok)==0 ) {
	    fp->pending_parse = &fp->fd->fontinfo->blendaxistypes;
	    AddValue(fp,NULL,line,endtok);
	} else if ( !fp->alreadycomplained ) {
	    LogError(_("Didn't understand \"%s\" in font info"), rmbinary(line));
	    fp->alreadycomplained = true;
	}
    } else if ( fp->inblend ) {
	if ( endtok==NULL ) {
	    if ( *line!='/' && strstr(line,"end")!=NULL )
		fp->inblend = false;
return;
	}
	/* Ignore anything in the blend dict defn */
    } else if ( fp->inblendprivate || fp->inblendfi ) {
	struct psdict *subdict = fp->inblendfi ? fp->fd->blendfontinfo : fp->fd->blendprivate;
	if ( fp->multiline ) {
	    ContinueValue(fp,subdict,line);
return;
	} else if ( endtok==NULL ) {
	    if ( *line!='/' && strstr(line,"end")!=NULL ) {
		fp->inblendprivate = fp->inblendfi = false;
		fp->inprivate = true;
	    }
return;
	} else
	    AddValue(fp,subdict,line,endtok);
    } else if ( fp->inprivate ) {
	if ( strstr(line,"/CharStrings")!=NULL && strstr(line,"dict")!=NULL ) {
	    if ( fp->fd->chars->next==0 ) {
		InitChars(fp->fd->chars,line);
		fp->ignore = false;
	    } else {
		fp->ignore = true;
		LogError( _("Ignoring duplicate /CharStrings entry\n") );
	    }
	    fp->inchars = 1;
	    fp->insubs = 0;
return;
	} else if ( strstr(line,"/Subrs")!=NULL ) {
	    if ( fp->fd->private->subrs->next>0 ) {
		fp->ignore = true;
		LogError( _("Ignoring duplicate /Subrs entry\n") );
	    } else {
		InitChars(fp->fd->private->subrs,line);
		fp->ignore = false;
	    }
	    fp->insubs = 1;
	    fp->inchars = 0;
return;
	} else if ( fp->multiline ) {
	    ContinueValue(fp,fp->fd->private->private,line);
return;
	}
	if ( endtok==NULL ) {
	    char *pt = line;
	    if ( *pt!='/' ) while ( (pt=strstr(pt,"end"))!=NULL ) {
		if ( fp->inchars ) fp->inchars = false;
		else fp->inprivate = false;
		pt += 3;
	    }
return;
	}
	if ( mycmp("ND",line+1,endtok)==0 || mycmp("|-",line+1,endtok)==0 ||
		mycmp("NP",line+1,endtok)==0 || mycmp("|",line+1,endtok)==0 ||
		mycmp("RD",line+1,endtok)==0 || mycmp("-|",line+1,endtok)==0 ||
		mycmp("password",line+1,endtok)==0 ||
		mycmp("MinFeature",line+1,endtok)==0 )
	    /* These conveigh no information, but are required */;
	else if ( mycmp("UniqueID",line+1,endtok)==0 ) {
	    if ( fp->fd->uniqueid==0 )
		fp->fd->uniqueid = strtol(endtok,NULL,10);
	} else {
	    if ( mycmp("lenIV",line+1,endtok)==0 )
		fp->fd->private->leniv = strtol(endtok,NULL,10);	/* We need this value */
	    AddValue(fp,fp->fd->private->private,line,endtok);
	}
    } else if ( fp->incidsysteminfo ) {
	if ( endtok==NULL && strncmp(line,"end", 3)==0 ) {
	    fp->incidsysteminfo=0;
return;
	} else if ( endtok==NULL )
return;
	if ( mycmp("Registry",line+1,endtok)==0 ) {
	    free( fp->fd->registry );
	    fp->fd->registry = getstring(endtok,in);
	} else if ( mycmp("Ordering",line+1,endtok)==0 ) {
	    free( fp->fd->ordering );
	    fp->fd->ordering = getstring(endtok,in);
	} else if ( mycmp("Supplement",line+1,endtok)==0 )		/* cff spec allows for copyright and notice */
	    fp->fd->supplement = strtol(endtok,NULL,0);
    } else {
	if ( strstr(line,"/Private")!=NULL && (strstr(line,"dict")!=NULL || strstr(line,"<<")!=NULL )) {
	    fp->infi = fp->inbb = fp->inmetrics = fp->inmetrics2 = false;
	    fp->inprivate = fp->inblendprivate = fp->inblendfi = false;
	    if ( strstr(line,"/Blend")!=NULL ) {
		fp->inblendprivate = 1;
		fp->fd->blendprivate = calloc(1,sizeof(struct psdict));
		InitDict(fp->fd->blendprivate,line);
	    } else {
		fp->inprivate = 1;
		InitDict(fp->fd->private->private,line);
	    }
return;
	} else if ( strstr(line,"/FontInfo")!=NULL && (strstr(line,"dict")!=NULL || strstr(line,"<<")!=NULL)) {
	    fp->inprivate = fp->inbb = fp->inmetrics = fp->inmetrics2 = false;
	    fp->infi = fp->inblendprivate = fp->inblendfi = false;
	    if ( strstr(line,"/Blend")!=NULL ) {
		fp->inblendfi = 1;
		fp->fd->blendfontinfo = calloc(1,sizeof(struct psdict));
		InitDict(fp->fd->blendfontinfo,line);
	    } else {
		if ( !strstr(line, "end") )
		    fp->infi = 1;
	    }
return;
	} else if ( strstr(line,"/Blend")!=NULL && strstr(line,"dict")!=NULL ) {
	    fp->inprivate = fp->inbb = fp->inmetrics = fp->inmetrics2 = false;
	    fp->infi = fp->inblendprivate = fp->inblendfi = false;
	    fp->inblend = true;
return;
	} else if ( strstr(line,"/sfnts")!=NULL && strstr(line,"[")!=NULL ) {
	    sfnts2tempfile(fp,in,line);
return;
	} else if ( strstr(line,"/CharStrings")!=NULL && strstr(line,"dict")!=NULL
		&& fp->fd->fonttype!=3 ) {
	    if ( fp->fd->chars->next==0 ) {
		InitChars(fp->fd->chars,line);
		fp->ignore = false;
	    } else {
		fp->ignore = true;
		LogError( _("Ignoring duplicate /CharStrings entry\n") );
	    }
	    fp->inchars = 1;
	    fp->insubs = 0;
	    fp->infi = fp->inprivate = fp->inbb = fp->inmetrics = fp->inmetrics2 = false;
	    fp->inblendprivate = fp->inblendfi = false;
return;
	} else if ( mycmp("/CharProcs",line,endtok)==0 ) {
	    InitCharProcs(fp->fd->charprocs,line);
	    fp->infi = fp->inprivate = fp->inbb = fp->inmetrics = fp->inmetrics2 = false;
	    fp->insubs = 0;
return;
	} else if ( strstr(line,"/CIDSystemInfo")!=NULL ) {
	    fp->incidsysteminfo = 1;
return;
	} else if ( fp->inmetrics ) {
	    if ( endtok!=NULL )
		AddValue(fp,fp->fd->metrics,line,endtok);
return;
	} else if ( fp->inbb ) {
	    /* Ignore it */;
return;
	}

	if ( fp->multiline ) {
	    ContinueValue(fp,NULL,line);
return;
	}
	
	if ( endtok==NULL ) {
	    if ( fp->skipping_mbf )
		;
	    else if ( fp->fdindex!=-1 && strstr(line,"end")!=NULL ) {
		if ( ++fp->fdindex>=fp->mainfd->fdcnt )
		    fp->fd = fp->mainfd;
		else
		    fp->fd = fp->mainfd->fds[fp->fdindex];
	    }
return;
	}
	if ( mycmp("FontName",line+1,endtok)==0 ) {
	    if ( fp->fd->fontname==NULL )
		fp->fd->fontname = gettoken(endtok);
	    else
		free(gettoken(endtok));	/* skip it */
	} else if ( mycmp("PaintType",line+1,endtok)==0 )
	    fp->fd->painttype = strtol(endtok,NULL,10);
	else if ( mycmp("FontType",line+1,endtok)==0 )
	    fp->fd->fonttype = strtol(endtok,NULL,10);
	else if ( mycmp("FontMatrix",line+1,endtok)==0 ) {
	    if ( fp->fd->fontmatrix[0]==0 )
		fillrealarray(fp->fd->fontmatrix,endtok,6);
	    else {
		real temp[6];
		fillrealarray(temp,endtok,6);
	    }
	} else if ( mycmp("LanguageLevel",line+1,endtok)==0 )
	    fp->fd->languagelevel = strtol(endtok,NULL,10);
	else if ( mycmp("WMode",line+1,endtok)==0 )
	    fp->fd->wmode = strtol(endtok,NULL,10);
	else if ( mycmp("FontBBox",line+1,endtok)==0 )
	     fillrealarray(fp->fd->fontbb,endtok,4);
	else if ( mycmp("UniqueID",line+1,endtok)==0 ) {
	    if ( fp->fd->uniqueid==0 )
		fp->fd->uniqueid = strtol(endtok,NULL,10);
	} else if ( mycmp("UniqueId",line+1,endtok)==0 ) {
	    LogError(_("This font contains a \"UniqueId\" variable, but the correct name for it is\n\t\"UniqueID\" (postscript is case concious)\n") );
	    if ( fp->fd->uniqueid==0 )
		fp->fd->uniqueid = strtol(endtok,NULL,10);
	} else if ( mycmp("XUID",line+1,endtok)==0 ) {
	    if ( fp->fd->xuid[0]==0 )
		fillintarray(fp->fd->xuid,endtok,20);
	} else if ( mycmp("StrokeWidth",line+1,endtok)==0 )
	    fp->fd->strokewidth = strtod(endtok,NULL);
	else if ( mycmp("WeightVector",line+1,endtok)==0 ) {
	    if ( fp->fd->weightvector==NULL ) {
		fp->pending_parse = &fp->fd->weightvector;
		AddValue(fp,NULL,line,endtok);
	    }
	} else if ( mycmp("$Blend",line+1,endtok)==0 ) {
	    fp->pending_parse = &fp->fd->blendfunc;
	    AddValue(fp,NULL,line,endtok);
	} else if ( strstr( line,"/NormalizeDesignVector" )!=NULL ) {
	    fp->pending_parse = &fp->fd->ndv;
	    AddValue(fp,NULL,line,endtok);
	} else if ( strstr( line,"/ConvertDesignVector" )!=NULL ) {
	    fp->pending_parse = &fp->fd->cdv;
	    AddValue(fp,NULL,line,endtok);
	} else if ( mycmp("BuildChar",line+1,endtok)==0 )
	    /* Do Nothing */;
	else if ( mycmp("BuildGlyph",line+1,endtok)==0 )
	    /* Do Nothing */;
	else if ( mycmp("CIDFontName",line+1,endtok)==0 ) {
	    free( fp->fd->cidfontname );
	    fp->fd->cidfontname = gettoken(endtok);
	} else if ( mycmp("CIDFontVersion",line+1,endtok)==0 ) {
	    fp->fd->cidversion = strtod(endtok,NULL);
	} else if ( mycmp("CIDFontType",line+1,endtok)==0 )
	    fp->fd->cidfonttype = strtol(endtok,NULL,10);
	else if ( mycmp("UIDBase",line+1,endtok)==0 )
	    fp->fd->uniqueid = strtol(endtok,NULL,10);
	else if ( mycmp("CIDMapOffset",line+1,endtok)==0 )
	    fp->fd->mapoffset = strtol(endtok,NULL,10);
	else if ( mycmp("FDBytes",line+1,endtok)==0 )
	    fp->fd->fdbytes = strtol(endtok,NULL,10);
	else if ( mycmp("GDBytes",line+1,endtok)==0 )
	    fp->fd->gdbytes = strtol(endtok,NULL,10);
	else if ( mycmp("CIDCount",line+1,endtok)==0 )
	    fp->fd->cidcnt = strtol(endtok,NULL,10);
	else if ( mycmp("FDArray",line+1,endtok)==0 ) { int i;
	    fp->mainfd = fp->fd;
	    fp->fd->fdcnt = strtol(endtok,NULL,10);
	    fp->fd->fds = calloc(fp->fd->fdcnt,sizeof(struct fontdict *));
	    for ( i=0; i<fp->fd->fdcnt; ++i )
		fp->fd->fds[i] = MakeEmptyFont();
	    fp->fdindex = 0;
	    fp->fd = fp->fd->fds[0];
	} else if ( mycmp("FontSetInit",line+1,endtok)==0 ) {
	    fp->iscff = true;
	    fp->iscid = false;
	} else if ( mycmp("CIDInit",line+1,endtok)==0 ) {
	    fp->iscid = true;
	    fp->iscff = false;
	} else if ( fp->skipping_mbf ) {	/* Skip over the makeblendedfont defn in a multimaster font */
	    /* Do Nothing */
	} else if ( !fp->alreadycomplained ) {
	    LogError( _("Didn't understand \"%s\" in blended font defn"), rmbinary(line) );
	    fp->alreadycomplained = true;
	}
    }
}

static void addinfo(struct fontparse *fp,char *line,char *tok,char *binstart,int binlen,FILE *in) {
    char *pt;

    decodestr((unsigned char *) binstart,binlen);
    binstart += fp->fd->private->leniv;
    binlen -= fp->fd->private->leniv;
    if ( binlen<0 ) {
	LogError( _("Bad CharString. Does not include lenIV bytes.\n") );
return;
    }

 retry:
    if ( fp->insubs ) {
	struct pschars *chars = /*fp->insubs ?*/ fp->fd->private->subrs /*: fp->fd->private->othersubrs*/;
	while ( isspace(*line)) ++line;
	if ( strncmp(line,"dup ",4)==0 ) {
	    int i = strtol(line+4,NULL,10);
	    if ( fp->ignore )
		/* Do Nothing */;
	    else if ( i<chars->cnt ) {
		if ( chars->values[i]!=NULL )
		    LogError( _("Duplicate definition of subroutine %d\n"), i );
		chars->lens[i] = binlen;
		chars->values[i] = malloc(binlen);
		memcpy(chars->values[i],binstart,binlen);
		if ( i>=chars->next ) chars->next = i+1;
	    } else if ( !fp->alreadycomplained ) {
		LogError( _("Index too big (must be <%d) \"%s"), chars->cnt, rmbinary(line));
		fp->alreadycomplained = true;
	    }
	} else if ( !fp->alreadycomplained ) {
	    LogError( _("Didn't understand \"%s\" while adding info to private subroutines"),
	                rmbinary(line) );
	    fp->alreadycomplained = true;
	}
    } else if ( fp->inchars ) {
	struct pschars *chars = fp->fd->chars;
	if ( *tok=='\0' )
	    LogError( _("No name for CharStrings dictionary \"%s"), rmbinary(line) );
	else if ( fp->ignore )
	    /* Do Nothing */;
	else if ( chars->next>=chars->cnt )
	    LogError( _("Too many entries in CharStrings dictionary \"%s"), rmbinary(line) );
	else {
	    int i = chars->next;
	    chars->lens[i] = binlen;
	    chars->keys[i] = copy(tok);
	    chars->values[i] = malloc(binlen);
	    memcpy(chars->values[i],binstart,binlen);
	    ++chars->next;
	    ff_progress_next();
	}
    } else if ( !fp->alreadycomplained ) {
	/* Special hacks for known badly formatted fonts */
	if ( strstr(line,"/CharStrings")!=NULL ) {
	    for ( pt=line; *pt!='/'; ++pt );
	    pt = strchr(pt+1,'/');
	    if ( pt!=NULL )
		*pt = '\0';
	    parseline(fp,line,in);
	    if ( pt!=NULL ) {
		*pt = '/';
		line = pt;
 goto retry;
	    }
return;
	} else if ( strstr(line,"/Subrs")!=NULL ) {
	    /* font is defining glyph on same line as Subrs array,	*/
	    /* which we need to parse using parseline;			*/
	    /* remove the binary glyph def and retry parse; then add	*/
	    /* the glyph def back and continue parsing binary stuff	*/
	    pt = binstart; /* start at binary, work backwards, find glyph def */
	    while (--pt >= line) if (!strncmp("dup", pt, 3)) break;
	    if ( pt<line ) pt = NULL;
	    if ( pt!=NULL ) *pt = '\0';
	    parseline(fp,line,in);
	    if ( pt!=NULL ) {
		*pt = 'd';
		line = pt;
 goto retry;
	    }
return;
	}
	LogError( _("Shouldn't be in addinfo \"%s"), rmbinary(line) );
	fp->alreadycomplained = true;
    }
}

/* glorpline (maybe means "glyph" or "parse" line) appears to process	*/
/*  each line of the eexec portion of a Type 1 font file.		*/
/*  Call it on each line of the file in order, in order to update "fp".	*/
/*									*/
/* In the book the token which starts a character description is always */
/*  RD but it's just the name of a subroutine which is defined in the	*/
/*  private diction and it could be anything. in one case it was a	*/
/*  "-|" (hyphen bar) so we can't just look for RD we must be a bit	*/
/*  smarter and figure out what the token is (oh. I see now. it's	*/
/*  allowed to be either one "RD" or "-|", but nothing else right)	*/
/* It's defined as {string currentfile exch readstring pop} so look for	*/
/* that Except that in gsf files we've also got				*/
/* "/-!{string currentfile exch readhexstring pop} readonly def"	*/
/*									*/
/*  NOTE: readhexstring!!!						*/
/* And in files generated by GNU fontutils				*/
static int glorpline(struct fontparse *fp, FILE *temp, char *rdtok) {
    static char *buffer=NULL, *end;
    char *pt, *binstart;
    int binlen;
    int ch;
    int innum, val=0, inbinary, cnt=0, inr, wasspace, nownum, nowr, nowspace, sptok;
    char *rdline = "{string currentfile exch readstring pop}", *rpt;
    char *rdline2 = "{string currentfile exch readhexstring pop}";
    char *tokpt = NULL, *rdpt;
    char temptok[255];
    int intok, first;
    int inPrivate = 0, inSubrs = 0;
    int wasminus=false, isminus, nibble=0, firstnibble=true, inhex;
    int willbehex = false;

    ch = getc(temp);
    if ( ch==EOF )
return( 0 );
    ungetc(ch,temp);

    if ( buffer==NULL ) {
	buffer = malloc(3000);
	end = buffer+3000;
    }
    innum = inr = 0; wasspace = 0; inbinary = 0; rpt = NULL; rdpt = NULL;
    inhex = 0;
    pt = buffer; binstart=NULL; binlen = 0; intok=0; sptok=0; first=1;
    temptok[0] = '\0';
    while ( (ch=getc(temp))!=EOF ) {
	if ( pt>=end ) {
	    char *old = buffer;
	    int len = (end-buffer)+2000;
	    buffer = realloc(buffer,len);
	    end = buffer+len;
	    pt = buffer+(pt-old);
	    if ( binstart!=NULL )
		binstart = buffer+(binstart-old);
	}
	*pt++ = ch;
	isminus = ch=='-' && wasspace;
	nownum = nowspace = nowr = 0;
	if ( rpt!=NULL && ch!=*rpt && ch=='h' && rpt-rdline>25 && rpt-rdline<30 &&
		rdline2[rpt-rdline]=='h' ) {
	    rpt = rdline2 + (rpt-rdline);
	    willbehex = true;
	}
	if ( inbinary ) {
	    if ( --cnt==0 )
		inbinary = 0;
	} else if ( inhex ) {
	    if ( ishexdigit(ch)) {
		int h;
		if ( isdigit(ch)) h = ch-'0';
		else if ( ch>='a' && ch<='f' ) h = ch-'a'+10;
		else h = ch-'A'+10;
		if ( firstnibble ) {
		    nibble = h;
		    --pt;
		} else {
		    pt[-1] = (nibble<<4)|h;
		    if ( --cnt==0 )
			inbinary = inhex = 0;
		}
		firstnibble = !firstnibble;
	    } else {
		--pt;
		/* skip everything not hex */
	    }
	} else if ( ch=='/' ) {
	    intok = 1;
	    tokpt = temptok;
	} else if ( intok && !isspace(ch) && ch!='{' && ch!='[' ) {
	    *tokpt++ = ch;
	} else if ( (intok||sptok) && (ch=='{' || ch=='[') ) {
	    *tokpt = '\0';
	    rpt = rdline+1;
	    intok = sptok = 0;
	} else if ( intok ) {
	    *tokpt = '\0';
	    intok = 0;
	    sptok = 1;
	    /* we've just read a name; ensure that we don't have two lines */
	    /* munged together that cause line-by-line parsing to fail	   */
	    if ( !strcmp("Private", temptok) ) {
		inPrivate = 1;
	    } else if ( !strcmp("Subrs", temptok) ) {
	        inSubrs = 1;
	        if ( inPrivate ) {
	            putBack(fp, temp, temptok, ch, &pt);
	            putBack(fp, temp, "/", '\0', &pt); /* starts /Subrs token */
    break;
	        }
	    } else if ( !strcmp("CharStrings", temptok) ) {
	        if (fp->insubs) { /* break CharStrings onto a seperate line */
	            putBack(fp, temp, temptok, ch, &pt);
	            putBack(fp, temp, "", '/', &pt);
		    fp->insubs = 0;
    break;
	        }
	    }
	} else if ( sptok && isspace(ch) ) {
	    nowspace = 1;
	    if ( ch=='\n' || ch=='\r' )
    break;
	} else if ( sptok && !isdigit(ch) )
	    sptok = 0;
	else if ( rpt!=NULL && ch==*rpt ) {
	    if ( *++rpt=='\0' ) {
		/* it matched the character definition string so this is the */
		/*  token we want to search for */
		strcpy(rdtok,temptok);
		fp->useshexstrings = willbehex;
		rpt = NULL;
	    }
	} else if ( rpt!=NULL && ch==' ' ) {
	    /* Extra spaces are ok */
	} else if ( rpt!=NULL ) {
	    rpt = NULL;
	    willbehex = false;
	} else if ( isdigit(ch) ) {
	    sptok = 0;
	    nownum = 1;
	    if ( innum )
		val = 10*val + ch-'0';
	    else
		val = ch-'0';
	} else if ( isspace(ch) ) {
	    nowspace = 1;
	    if ( ch=='\n' || ch=='\r' )
    break;
	    if ( inSubrs && matchFromBack(pt - 2, "array", pt - buffer - 1) )
    break;  /* Subrs may be on same line with first RD def -- seperate them */
	} else if ( wasspace && ch==*rdtok ) {
	    nowr = 1;
	    fp->useshexstrings = willbehex;
	    rdpt = rdtok+1;
	} else if ( wasspace && ch=='-' ) {	/* fonts produced by type1fix seem to define both "RD" and "-|" which confused me. so just respond to either */
	    nowr = 1;
	    fp->useshexstrings = false;
	    rdpt = "|";
	} else if ( wasspace && ch=='R' ) {	/* fonts produced by type1fix seem to define both "RD" and "-|" which confused me. so just respond to either */
	    nowr = 1;
	    fp->useshexstrings = false;
	    rdpt = "D";
	} else if ( inr && ch==*rdpt ) {
	    if ( *++rdpt =='\0' ) {
		ch = getc(temp);
		*pt++ = ch;
		if ( isspace(ch) && val!=0 ) {
		    inhex = fp->useshexstrings;
		    inbinary = !fp->useshexstrings;
		    firstnibble = true;
		    cnt = val;
		    binstart = pt;
		    binlen = val;
		}
	    } else
		nowr = 1;
	} else if ( wasminus && ch=='!' ) {
	    ch = getc(temp);
	    *pt++ = ch;
	    if ( isspace(ch) && val!=0 ) {
		inhex = 1;
		cnt = val;
		binstart = pt;
		binlen = val;
		firstnibble = true;
	    }
	}
	innum = nownum; wasspace = nowspace; inr = nowr;
	wasminus = isminus;
	first = 0;
    } /* end while */
    *pt = '\0';
    if ( binstart==NULL ) {
	parseline(fp,buffer,temp);
    } else {
	addinfo(fp,buffer,temptok,binstart,binlen,temp);
    }
return( 1 );
}

static int nrandombytes[4];
#define EODMARKLEN	16

#define bgetc(extra,in)	(*(extra)=='\0' ? getc(in) : (unsigned char ) *(extra)++ )

static void decrypteexec(FILE *in,FILE *temp, int hassectionheads,char *extra) {
    int ch1, ch2, ch3, ch4, binary;
    int zcnt;
    unsigned char zeros[EODMARKLEN+6+1];
    int sect_len=0x7fffffff;

    if ( extra==(void *) 5 ) extra = "";

    /* The PLRM defines white space to include form-feed and null. The t1_spec*/
    /*  does not. The t1_spec wins here. Someone gave me a font which began */
    /*  with a formfeed and that was part of the encrypted body */
    while ( (ch1=bgetc(extra,in))!=EOF && (ch1==' ' || ch1=='\t' || ch1=='\n' || ch1=='\r'));
    if ( ch1==0200 && hassectionheads ) {
	/* skip the 6 byte section header in pfb files that follows eexec */
	ch1 = bgetc(extra,in);
	sect_len = bgetc(extra,in);
	sect_len |= bgetc(extra,in)<<8;
	sect_len |= bgetc(extra,in)<<16;
	sect_len |= bgetc(extra,in)<<24;
	sect_len -= 3;
	ch1 = bgetc(extra,in);
    }
    ch2 = bgetc(extra,in); ch3 = bgetc(extra,in); ch4 = bgetc(extra,in);
    binary = 0;
    if ( ch1<'0' || (ch1>'9' && ch1<'A') || ( ch1>'F' && ch1<'a') || (ch1>'f') ||
	     ch2<'0' || (ch2>'9' && ch2<'A') || (ch2>'F' && ch2<'a') || (ch2>'f') ||
	     ch3<'0' || (ch3>'9' && ch3<'A') || (ch3>'F' && ch3<'a') || (ch3>'f') ||
	     ch4<'0' || (ch4>'9' && ch4<'A') || (ch4>'F' && ch4<'a') || (ch4>'f') )
	binary = 1;
    if ( ch1==EOF || ch2==EOF || ch3==EOF || ch4==EOF ) {
return;
    }

    initcode();
    if ( binary ) {
	nrandombytes[0] = decode(ch1);
	nrandombytes[1] = decode(ch2);
	nrandombytes[2] = decode(ch3);
	nrandombytes[3] = decode(ch4);
	zcnt = 0;
	while (( ch1=bgetc(extra,in))!=EOF ) {
	    --sect_len;
	    if ( hassectionheads ) {
		if ( sect_len==0 && ch1==0200 ) {
		    ch1 = bgetc(extra,in);
		    sect_len = bgetc(extra,in);
		    sect_len |= bgetc(extra,in)<<8;
		    sect_len |= bgetc(extra,in)<<16;
		    sect_len |= bgetc(extra,in)<<24;
		    sect_len += 1;
		    if ( ch1=='\1' )
	break;
		} else {
		    dumpzeros(temp,zeros,zcnt);
		    zcnt = 0;
		    putc(decode(ch1),temp);
		}
	    } else {
		if ( ch1=='0' ) ++zcnt; else {dumpzeros(temp,zeros,zcnt); zcnt = 0; }
		if ( zcnt>EODMARKLEN )
	break;
		if ( zcnt==0 )
		    putc(decode(ch1),temp);
		else
		    zeros[zcnt-1] = decode(ch1);
	    }
	}
    } else {
	nrandombytes[0] = decode(hex(ch1,ch2));
	nrandombytes[1] = decode(hex(ch3,ch4));
	ch1 = bgetc(extra,in); ch2 = bgetc(extra,in); ch3 = bgetc(extra,in); ch4 = bgetc(extra,in);
	nrandombytes[2] = decode(hex(ch1,ch2));
	nrandombytes[3] = decode(hex(ch3,ch4));
	zcnt = 0;
	while (( ch1=bgetc(extra,in))!=EOF ) {
	    while ( ch1!=EOF && isspace(ch1)) ch1 = bgetc(extra,in);
	    while ( (ch2=bgetc(extra,in))!=EOF && isspace(ch2));
	    if ( ch1=='0' && ch2=='0' ) ++zcnt; else { dumpzeros(temp,zeros,zcnt); zcnt = 0;}
	    if ( zcnt>EODMARKLEN )
	break;
	    if ( zcnt==0 )
		putc(decode(hex(ch1,ch2)),temp);
	    else
		zeros[zcnt-1] = decode(hex(ch1,ch2));
	}
    }
    while (( ch1=bgetc(extra,in))=='0' || isspace(ch1) );
    if ( ch1!=EOF ) ungetc(ch1,in);
}

static void decryptagain(struct fontparse *fp,FILE *temp,char *rdtok) {
    while ( glorpline(fp,temp,rdtok));
}

static void parsetype3(struct fontparse *fp,FILE *in) {
    PSFontInterpretPS(in,fp->fd->charprocs,fp->fd->encoding );
}

static unsigned char *readt1str(FILE *temp,int offset,int len,int leniv) {
    int i;
    unsigned char *str, *pt;
    unsigned short r = 4330;
    unsigned char plain, cypher;
    /* The CID spec doesn't mention this, but the type 1 strings are all */
    /*  eexec encrupted (with the nested encryption). Remember leniv varies */
    /*  from fd to fd (potentially) */
    /* I'm told (by Ian Kemmish) that leniv==-1 => no eexec encryption */

    fseek(temp,offset,SEEK_SET);
    if ( leniv<0 ) {
	str = pt = malloc(len+1);
	for ( i=0 ; i<len; ++i )
	    *pt++ = getc(temp);
    } else {
	for ( i=0; i<leniv; ++i ) {
	    cypher = getc(temp);
	    plain = ( cypher ^ (r>>8));
	    r = (cypher + r) * c1 + c2;
	}
	str = pt = malloc(len-leniv+1);
	for (; i<len; ++i ) {
	    cypher = getc(temp);
	    plain = ( cypher ^ (r>>8));
	    r = (cypher + r) * c1 + c2;
	    *pt++ = plain;
	}
    }
    *pt = '\0';
return( str );
}

static void figurecids(struct fontparse *fp,FILE *temp) {
    struct fontdict *fd = fp->mainfd;
    int i,j,k,val;
    int *offsets;
    int cidcnt = fd->cidcnt;
    int leniv;
    /* Some cid formats don't have any of these */

    fd->cidstrs = malloc(cidcnt*sizeof(uint8 *));
    fd->cidlens = malloc(cidcnt*sizeof(int16));
    fd->cidfds = malloc((cidcnt+1)*sizeof(int16));
    offsets = malloc((cidcnt+1)*sizeof(int));
    ff_progress_change_total(cidcnt);

    fseek(temp,fd->mapoffset,SEEK_SET);
    for ( i=0; i<=fd->cidcnt; ++i ) {
	for ( j=val=0; j<fd->fdbytes; ++j )
	    val = (val<<8) + getc(temp);
	if ( val >= fd->fdcnt && val!=255 ) {	/* 255 is a special mark */
	    LogError( _("Invalid FD (%d) assigned to CID %d.\n"), val, i );
	    val = 0;
	}
	fd->cidfds[i] = val;
	for ( j=val=0; j<fd->gdbytes; ++j )
	    val = (val<<8) + getc(temp);
	offsets[i] = val;
	if ( i!=0 ) {
	    fd->cidlens[i-1] = offsets[i]-offsets[i-1];
	    if ( fd->cidlens[i-1]<0 ) {
		LogError( _("Bad CID offset for CID %d\n"), i-1 );
		fd->cidlens[i-1] = 0;
	    }
	}
    }

    for ( i=0; i<fd->cidcnt; ++i ) {
	if ( fd->cidlens[i]== 0 )
	    fd->cidstrs[i] = NULL;
	else {
	    fd->cidstrs[i] = readt1str(temp,offsets[i],fd->cidlens[i],
		    fd->fds[fd->cidfds[i]]->private->leniv);
	    fd->cidlens[i] -= fd->fds[fd->cidfds[i]]->private->leniv;
	}
	ff_progress_next();
    }
    free(offsets);

    for ( k=0; k<fd->fdcnt; ++k ) {
	struct private *private = fd->fds[k]->private;
	char *ssubroff = PSDictHasEntry(private->private,"SubrMapOffset");
	char *ssdbytes = PSDictHasEntry(private->private,"SDBytes");
	char *ssubrcnt = PSDictHasEntry(private->private,"SubrCount");
	int subroff, sdbytes, subrcnt;

	if ( ssubroff!=NULL && ssdbytes!=NULL && ssubrcnt!=NULL &&
		(subroff=strtol(ssubroff,NULL,10))>=0 &&
		(sdbytes=strtol(ssdbytes,NULL,10))>0 &&
		(subrcnt=strtol(ssubrcnt,NULL,10))>0 ) {
	    private->subrs->cnt = subrcnt;
	    private->subrs->values = calloc(subrcnt,sizeof(uint8 *));
	    private->subrs->lens = calloc(subrcnt,sizeof(int));
	    leniv = private->leniv;
	    offsets = malloc((subrcnt+1)*sizeof(int));
	    fseek(temp,subroff,SEEK_SET);
	    for ( i=0; i<=subrcnt; ++i ) {
		for ( j=val=0; j<sdbytes; ++j )
		    val = (val<<8) + getc(temp);
		offsets[i] = val;
		if ( i!=0 )
		    private->subrs->lens[i-1] = offsets[i]-offsets[i-1];
	    }
	    for ( i=0; i<subrcnt; ++i ) {
		private->subrs->values[i] = readt1str(temp,offsets[i],
			private->subrs->lens[i],leniv);
	    }
	    private->subrs->next = i;
	    free(offsets);
	}
	PSDictRemoveEntry(private->private,"SubrMapOffset");
	PSDictRemoveEntry(private->private,"SDBytes");
	PSDictRemoveEntry(private->private,"SubrCount");
    }
}

static void dodata( struct fontparse *fp, FILE *in, FILE *temp) {
    int binary, cnt, len;
    int ch, ch2;
    char *pt;
    char fontsetname[256];

    while ( (ch=getc(in))!='(' && ch!='/' && ch!=EOF );
    if ( ch=='/' ) {
	/* There appears to be no provision for a hex encoding here */
	/* Why can't they use the same format for routines with the same name? */
	binary = true;
	for ( pt=fontsetname; (ch=getc(in))!=' ' && ch!=EOF; )
	    if ( pt<fontsetname+sizeof(fontsetname)-1 )
		*pt++= ch;
	*pt = '\0';
    } else {
	if ( (ch=getc(in))=='B' || ch=='b' ) binary = true;
	else if ( ch=='H' || ch=='h' ) binary = false;
	else {
	    binary = true;		/* Who knows? */
	    LogError( _("Failed to parse the StartData command properly\n") );
	}
	fontsetname[0] = '\0';
	while ( (ch=getc(in))!=')' && ch!=EOF );
    }
    if ( fscanf( in, "%d", &len )!=1 || len<=0 ) {
	len = 0;
	LogError( _("Failed to parse the StartData command properly, bad count\n") );
    }
    cnt = len;
    while ( isspace(ch=getc(in)) );
    ungetc(ch,in);
    for ( pt="StartData "; *pt; ++pt )
	getc(in);			/* And if it didn't match, what could I do about it? */
    if ( binary ) {
	while ( cnt>0 ) {
	    ch = getc(in);
	    putc(ch,temp);
	    --cnt;
	}
    } else {
	while ( cnt>0 ) {
	    /* Hex data are allowed to contain whitespace */
	    while ( isspace(ch=getc(in)) );
	    while ( isspace(ch2=getc(in)) );
	    ch = hex(ch,ch2);
	    putc(ch,temp);
	    --cnt;
	}
	if ( (ch=getc(in))!='>' ) ungetc(ch,in);
    }
    rewind(temp);
    if ( fp->iscid )
	figurecids(fp,temp);
    else {
	fp->fd->sf = _CFFParse(temp,len,fontsetname);
	fp->fd->wascff = true;
    }
}

static void realdecrypt(struct fontparse *fp,FILE *in, FILE *temp) {
    char buffer[1024]; /* 256 was okay, but need this much now when some lines are concatenated */
    int first, hassectionheads;
    char rdtok[20];
    int saw_blend = false;

    strcpy(rdtok,"RD");

    first = 1; hassectionheads = 0;
    while ( myfgets(buffer,sizeof(buffer),in)!=NULL ) {
	if ( strstr(buffer, "Blend")!=NULL )
	    saw_blend = true;
	if ( first && buffer[0]=='\200' ) {
	    int len = strlen( buffer );
	    hassectionheads = 1;
	    fp->fd->wasbinary = true;
	    /* if there were a newline in the section header (in the length word)*/
	    /*  we would stop at it, and not read the full header */
	    if ( len<6 )	/* eat the header */
		while ( len<6 ) { getc(in); ++len; }
	    else	/* Otherwise parse anything else on the line */
		parseline(fp,buffer+6,in);
	} else if ( strstr(buffer,"CharProcs")!=NULL && strstr(buffer,"begin")!=NULL ) {
	    parsetype3(fp,in);
return;
	} else if ( fp->fd->fonttype!=42 && strstr(buffer,"CharStrings")!=NULL && strstr(buffer,"begin")!=NULL ) {
	    /* Fontographer uses CharStrings even though they aren't */
	    parsetype3(fp,in);
return;
	} else if ( !fp->iscid ) {
	    if ( saw_blend )
		parseline(fp,buffer,in);
		/* But if it's a multi master font, don't do the special private hack */
	    else if ( strstr(buffer,"/CharStrings")!=NULL &&
		    strstr(buffer,"begin")!=NULL &&
		    (fp->fd->fonttype!=42 && fp->fd->cidfonttype!=2)) {
		/* gsf files are not eexec encoded, but the charstrings are encoded*/
		InitChars(fp->fd->chars,buffer);
		fp->inchars = 1;
		decryptagain(fp,in,rdtok);
return;
	    } else if ( strstr(buffer,"/Subrs")!=NULL && strstr(buffer,"array")!=NULL ) {
		/* Same case as above */
		InitChars(fp->fd->private->subrs,buffer);
		fp->insubs = 1;
		decryptagain(fp,in,rdtok);
return;
	    } else if ( strstr(buffer,"/Private")!=NULL && (strstr(buffer,"dict")!=NULL || strstr(buffer,"<<")!=NULL )) {
		/* files produced by GNU fontutils have some of the same issues */
		fp->inprivate = 1;
		fp->infi = false;
		decryptagain(fp,in,rdtok);
return;
	    } else
		parseline(fp,buffer,in);
	} else
	    parseline(fp,buffer,in);
	first = 0;
	if ( strstr(buffer,"%%BeginData: ")!=NULL )
    break;
	if ( strstr(buffer,"currentfile")!=NULL && strstr(buffer, "eexec")!=NULL ) {
	    fp->skipping_mbf = false;
    break;
	}
    }

    if ( strstr(buffer,"%%BeginData: ")!=NULL ) {
	/* used by both CID fonts and CFF fonts (and chameleons, whatever they are) */
	dodata(fp,in,temp);
    } else if ( strstr(buffer,"eexec")!=NULL ) {
	decrypteexec(in,temp,hassectionheads,strstr(buffer, "eexec")+5);
	rewind(temp);
	decryptagain(fp,temp,rdtok);
	while ( myfgets(buffer,sizeof(buffer),in)!=NULL ) {
	    if ( buffer[0]!='\200' || !hassectionheads )
		parseline(fp,buffer,in);
	}
    } else if (( fp->fd->fonttype==42 || fp->fd->cidfonttype==2 ) && fp->sfnts!=NULL ) {
	fp->fd->sf = _SFReadTTF(fp->sfnts,0,0,"<Temp File>",NULL,fp->fd);
	fclose(fp->sfnts);
    }
}

FontDict *_ReadPSFont(FILE *in) {
    FILE *temp;
    struct fontparse fp;
    struct stat b;

    temp = GFileTmpfile();
    if ( temp==NULL ) {
	LogError( _("Cannot open a temporary file\n") );
	fclose(in); 
return(NULL);
    }

    locale_t tmplocale; locale_t oldlocale; // Declare temporary locale storage.
    switch_to_c_locale(&tmplocale, &oldlocale); // Switch to the C locale temporarily and cache the old locale.
    memset(&fp,'\0',sizeof(fp));
    fp.fd = fp.mainfd = PSMakeEmptyFont();
    fp.fdindex = -1;
    realdecrypt(&fp,in,temp);
    free(fp.vbuf);
    switch_to_old_locale(&tmplocale, &oldlocale); // Switch to the cached locale.

    fclose(temp);

    if ( fstat(fileno(in),&b)!=-1 ) {
	fp.fd->modificationtime = GetST_MTime(b);
	fp.fd->creationtime = GetST_MTime(b);
    }
return( fp.fd );
}

FontDict *ReadPSFont(char *fontname) {
    FILE *in;
    FontDict *fd;

    in = fopen(fontname,"rb");
    if ( in==NULL ) {
	LogError( _("Cannot open %s\n"), fontname );
return(NULL);
    }
    fd = _ReadPSFont(in);
    if ( fd!=NULL )
        fclose(in);
return( fd );
}

void PSCharsFree(struct pschars *chrs) {
    int i;

    if ( chrs==NULL )
return;
    for ( i=0; i<chrs->next; ++i ) {
	if ( chrs->keys!=NULL ) free(chrs->keys[i]);
	free(chrs->values[i]);
    }
    free(chrs->lens);
    free(chrs->keys);
    free(chrs->values);
    free(chrs);
}

void PSDictFree(struct psdict *dict) {
    int i;

    if ( dict==NULL )
return;
    for ( i=0; i<dict->next; ++i ) {
	if ( dict->keys!=NULL ) free(dict->keys[i]);
	free(dict->values[i]);
    }
    free(dict->keys);
    free(dict->values);
    free(dict);
}

static void PrivateFree(struct private *prv) {
    PSCharsFree(prv->subrs);
    PSDictFree(prv->private);
    free(prv);
}

static void FontInfoFree(struct fontinfo *fi) {
    free(fi->familyname);
    free(fi->fullname);
    free(fi->notice);
    free(fi->weight);
    free(fi->version);
    free(fi->blenddesignpositions);
    free(fi->blenddesignmap);
    free(fi->blendaxistypes);
    free(fi);
}

void PSFontFree(FontDict *fd) {
    int i;

    if ( fd->encoding!=NULL )
	for ( i=0; i<256; ++i )
	    free( fd->encoding[i]);
    free(fd->fontname);
    free(fd->cidfontname);
    free(fd->registry);
    free(fd->ordering);
    FontInfoFree(fd->fontinfo);
    PSCharsFree(fd->chars);
    PrivateFree(fd->private);
    if ( fd->charprocs!=NULL ) {
	for ( i=0; i<fd->charprocs->cnt; ++i )
	    free(fd->charprocs->keys[i]);
	free(fd->charprocs->keys);
	free(fd->charprocs->values);
	free(fd->charprocs);
    }
    if ( fd->cidstrs!=NULL ) {
	for ( i=0; i<fd->cidcnt; ++i )
	    free( fd->cidstrs[i]);
	free(fd->cidstrs);
    }
    free(fd->cidlens);
    free(fd->cidfds);
    if ( fd->fds!=NULL ) {
	for ( i=0; i<fd->fdcnt; ++i )
	    PSFontFree(fd->fds[i]);
	free(fd->fds);
    }
    free(fd->blendfunc);
    free(fd->weightvector);
    free(fd->cdv);
    free(fd->ndv);

    PSDictFree(fd->blendprivate);
    PSDictFree(fd->blendfontinfo);
    
    free(fd);
}

char **_NamesReadPostScript(FILE *ps) {
    char **ret = NULL;
    char buffer[2000], *pt, *end;

    if ( ps!=NULL ) {
	while ( fgets(buffer,sizeof(buffer),ps)!=NULL ) {
	    if ( strstr(buffer,"/FontName")!=NULL ||
		    strstr(buffer,"/CIDFontName")!=NULL ) {
		pt = strstr(buffer,"FontName");
		pt += strlen("FontName");
		while ( isspace(*pt)) ++pt;
		if ( *pt=='/' ) ++pt;
		for ( end = pt; *end!='\0' && !isspace(*end); ++end );
		ret = malloc(2*sizeof(char *));
		ret[0] = copyn(pt,end-pt);
		ret[1] = NULL;
	break;
	    } else if ( strstr(buffer,"currentfile")!=NULL && strstr(buffer,"eexec")!=NULL )
	break;
	    else if ( strstr(buffer,"%%BeginData")!=NULL )
	break;
	}
	fclose(ps);
    }
return( ret );
}

char **NamesReadPostScript(char *filename) {
return( _NamesReadPostScript( fopen(filename,"rb")));
}
