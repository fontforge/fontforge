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

#include <fontforge-config.h>

#include "encoding.h"
#include "fontforgevw.h"
#include "macenc.h"
#include "ttf.h"
#include "ustring.h"

/*
 The original data for these mappings may be found at
    http://www.unicode.org/Public/MAPPINGS/VENDORS/APPLE/
 unfortunately this site does not contain all the macintosh encodings
 so we leave some blank
*/
/* Response (indirectly) from charsets@apple.com when asked about the missing
 encodings:

    > I don't believe any of those additional scripts are actually
    > defined as character encodings. He can safely ignore anything
    > that's not in the Apple folder on the Unicode site.
    >
    > I monitor charsets@apple.com and don't recall seeing any e-mail
    > on this subject. It's possible it got lost in the voluminous
    > spam the address receives.
    >
    > Deborah
 I find this perplexing (unless the script is defined but unused, how can
 it fail to have a defined encoding), but will accept it.
*/

/* Macintosh 1 byte encodings */
static unichar_t arabic[256] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 
    0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f, 
    0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 
    0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f, 
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 
    0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f, 
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 
    0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f, 
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 
    0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f, 
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 
    0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f, 
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 
    0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f, 
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 
    0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f, 
    0x00c4, 0x00a0, 0x00c7, 0x00c9, 0x00d1, 0x00d6, 0x00dc, 0x00e1, 
    0x00e0, 0x00e2, 0x00e4, 0x06ba, 0x00ab, 0x00e7, 0x00e9, 0x00e8, 
    0x00ea, 0x00eb, 0x00ed, 0x2026, 0x00ee, 0x00ef, 0x00f1, 0x00f3, 
    0x00bb, 0x00f4, 0x00f6, 0x00f7, 0x00fa, 0x00f9, 0x00fb, 0x00fc, 
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x066a, 0x0026, 0x0027, 
    0x0028, 0x0029, 0x002a, 0x002b, 0x060c, 0x002d, 0x002e, 0x002f, 
    0x0660, 0x0661, 0x0662, 0x0663, 0x0664, 0x0665, 0x0666, 0x0667, 
    0x0668, 0x0669, 0x003a, 0x061b, 0x003c, 0x003d, 0x003e, 0x061f, 
    0x274a, 0x0621, 0x0622, 0x0623, 0x0624, 0x0625, 0x0626, 0x0627, 
    0x0628, 0x0629, 0x062a, 0x062b, 0x062c, 0x062d, 0x062e, 0x062f, 
    0x0630, 0x0631, 0x0632, 0x0633, 0x0634, 0x0635, 0x0636, 0x0637, 
    0x0638, 0x0639, 0x063a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f, 
    0x0640, 0x0641, 0x0642, 0x0643, 0x0644, 0x0645, 0x0646, 0x0647, 
    0x0648, 0x0649, 0x064a, 0x064b, 0x064c, 0x064d, 0x064e, 0x064f, 
    0x0650, 0x0651, 0x0652, 0x067e, 0x0679, 0x0686, 0x06d5, 0x06a4, 
    0x06af, 0x0688, 0x0691, 0x007b, 0x007c, 0x007d, 0x0698, 0x06d2 
};

static unichar_t centeuro[256] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 
    0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f, 
    0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 
    0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f, 
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 
    0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f, 
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 
    0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f, 
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 
    0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f, 
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 
    0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f, 
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 
    0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f, 
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 
    0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f, 
    0x00c4, 0x0100, 0x0101, 0x00c9, 0x0104, 0x00d6, 0x00dc, 0x00e1, 
    0x0105, 0x010c, 0x00e4, 0x010d, 0x0106, 0x0107, 0x00e9, 0x0179, 
    0x017a, 0x010e, 0x00ed, 0x010f, 0x0112, 0x0113, 0x0116, 0x00f3, 
    0x0117, 0x00f4, 0x00f6, 0x00f5, 0x00fa, 0x011a, 0x011b, 0x00fc, 
    0x2020, 0x00b0, 0x0118, 0x00a3, 0x00a7, 0x2022, 0x00b6, 0x00df, 
    0x00ae, 0x00a9, 0x2122, 0x0119, 0x00a8, 0x2260, 0x0123, 0x012e, 
    0x012f, 0x012a, 0x2264, 0x2265, 0x012b, 0x0136, 0x2202, 0x2211, 
    0x0142, 0x013b, 0x013c, 0x013d, 0x013e, 0x0139, 0x013a, 0x0145, 
    0x0146, 0x0143, 0x00ac, 0x221a, 0x0144, 0x0147, 0x2206, 0x00ab, 
    0x00bb, 0x2026, 0x00a0, 0x0148, 0x0150, 0x00d5, 0x0151, 0x014c, 
    0x2013, 0x2014, 0x201c, 0x201d, 0x2018, 0x2019, 0x00f7, 0x25ca, 
    0x014d, 0x0154, 0x0155, 0x0158, 0x2039, 0x203a, 0x0159, 0x0156, 
    0x0157, 0x0160, 0x201a, 0x201e, 0x0161, 0x015a, 0x015b, 0x00c1, 
    0x0164, 0x0165, 0x00cd, 0x017d, 0x017e, 0x016a, 0x00d3, 0x00d4, 
    0x016b, 0x016e, 0x00da, 0x016f, 0x0170, 0x0171, 0x0172, 0x0173, 
    0x00dd, 0x00fd, 0x0137, 0x017b, 0x0141, 0x017c, 0x0122, 0x02c7 
};

static unichar_t croatian[256] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 
    0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f, 
    0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 
    0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f, 
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 
    0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f, 
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 
    0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f, 
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 
    0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f, 
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 
    0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f, 
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 
    0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f, 
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 
    0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f, 
    0x00c4, 0x00c5, 0x00c7, 0x00c9, 0x00d1, 0x00d6, 0x00dc, 0x00e1, 
    0x00e0, 0x00e2, 0x00e4, 0x00e3, 0x00e5, 0x00e7, 0x00e9, 0x00e8, 
    0x00ea, 0x00eb, 0x00ed, 0x00ec, 0x00ee, 0x00ef, 0x00f1, 0x00f3, 
    0x00f2, 0x00f4, 0x00f6, 0x00f5, 0x00fa, 0x00f9, 0x00fb, 0x00fc, 
    0x2020, 0x00b0, 0x00a2, 0x00a3, 0x00a7, 0x2022, 0x00b6, 0x00df, 
    0x00ae, 0x0160, 0x2122, 0x00b4, 0x00a8, 0x2260, 0x017d, 0x00d8, 
    0x221e, 0x00b1, 0x2264, 0x2265, 0x2206, 0x00b5, 0x2202, 0x2211, 
    0x220f, 0x0161, 0x222b, 0x00aa, 0x00ba, 0x03a9, 0x017e, 0x00f8, 
    0x00bf, 0x00a1, 0x00ac, 0x221a, 0x0192, 0x2248, 0x0106, 0x00ab, 
    0x010c, 0x2026, 0x00a0, 0x00c0, 0x00c3, 0x00d5, 0x0152, 0x0153, 
    0x0110, 0x2014, 0x201c, 0x201d, 0x2018, 0x2019, 0x00f7, 0x25ca, 
    0xf8ff, 0x00a9, 0x2044, 0x20ac, 0x2039, 0x203a, 0x00c6, 0x00bb, 
    0x2013, 0x00b7, 0x201a, 0x201e, 0x2030, 0x00c2, 0x0107, 0x00c1, 
    0x010d, 0x00c8, 0x00cd, 0x00ce, 0x00cf, 0x00cc, 0x00d3, 0x00d4, 
    0x0111, 0x00d2, 0x00da, 0x00db, 0x00d9, 0x0131, 0x02c6, 0x02dc, 
    0x00af, 0x03c0, 0x00cb, 0x02da, 0x00b8, 0x00ca, 0x00e6, 0x02c7 
};

static unichar_t cyrillic[256] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 
    0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f, 
    0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 
    0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f, 
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 
    0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f, 
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 
    0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f, 
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 
    0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f, 
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 
    0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f, 
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 
    0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f, 
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 
    0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f, 
    0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417, 
    0x0418, 0x0419, 0x041a, 0x041b, 0x041c, 0x041d, 0x041e, 0x041f, 
    0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427, 
    0x0428, 0x0429, 0x042a, 0x042b, 0x042c, 0x042d, 0x042e, 0x042f, 
    0x2020, 0x00b0, 0x0490, 0x00a3, 0x00a7, 0x2022, 0x00b6, 0x0406, 
    0x00ae, 0x00a9, 0x2122, 0x0402, 0x0452, 0x2260, 0x0403, 0x0453, 
    0x221e, 0x00b1, 0x2264, 0x2265, 0x0456, 0x00b5, 0x0491, 0x0408, 
    0x0404, 0x0454, 0x0407, 0x0457, 0x0409, 0x0459, 0x040a, 0x045a, 
    0x0458, 0x0405, 0x00ac, 0x221a, 0x0192, 0x2248, 0x2206, 0x00ab, 
    0x00bb, 0x2026, 0x00a0, 0x040b, 0x045b, 0x040c, 0x045c, 0x0455, 
    0x2013, 0x2014, 0x201c, 0x201d, 0x2018, 0x2019, 0x00f7, 0x201e, 
    0x040e, 0x045e, 0x040f, 0x045f, 0x2116, 0x0401, 0x0451, 0x044f, 
    0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437, 
    0x0438, 0x0439, 0x043a, 0x043b, 0x043c, 0x043d, 0x043e, 0x043f, 
    0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447, 
    0x0448, 0x0449, 0x044a, 0x044b, 0x044c, 0x044d, 0x044e, 0x20ac 
};

static unichar_t devanagari[256] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 
    0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f, 
    0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 
    0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f, 
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 
    0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f, 
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 
    0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f, 
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 
    0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f, 
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 
    0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f, 
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 
    0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f, 
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 
    0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f, 
    0x00d7, 0x2212, 0x2013, 0x2014, 0x2018, 0x2019, 0x2026, 0x2022, 
    0x00a9, 0x00ae, 0x2122, 0x008b, 0x008c, 0x008d, 0x008e, 0x008f, 
    0x0965, 0x0970, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097, 
    0x0098, 0x0099, 0x009a, 0x009b, 0x009c, 0x009d, 0x009e, 0x009f, 
    0x00a0, 0x0901, 0x0902, 0x0903, 0x0905, 0x0906, 0x0907, 0x0908, 
    0x0909, 0x090a, 0x090b, 0x090e, 0x090f, 0x0910, 0x090d, 0x0912, 
    0x0913, 0x0914, 0x0911, 0x0915, 0x0916, 0x0917, 0x0918, 0x0919, 
    0x091a, 0x091b, 0x091c, 0x091d, 0x091e, 0x091f, 0x0920, 0x0921, 
    0x0922, 0x0923, 0x0924, 0x0925, 0x0926, 0x0927, 0x0928, 0x0929, 
    0x092a, 0x092b, 0x092c, 0x092d, 0x092e, 0x092f, 0x095f, 0x0930, 
    0x0931, 0x0932, 0x0933, 0x0934, 0x0935, 0x0936, 0x0937, 0x0938, 
    0x0939, 0x200e, 0x093e, 0x093f, 0x0940, 0x0941, 0x0942, 0x0943, 
    0x0946, 0x0947, 0x0948, 0x0945, 0x094a, 0x094b, 0x094c, 0x0949, 
    0x094d, 0x093c, 0x0964, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef, 
    0x00f0, 0x0966, 0x0967, 0x0968, 0x0969, 0x096a, 0x096b, 0x096c, 
    0x096d, 0x096e, 0x096f, 
};

static unichar_t farsi[256] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 
    0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f, 
    0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 
    0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f, 
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 
    0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f, 
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 
    0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f, 
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 
    0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f, 
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 
    0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f, 
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 
    0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f, 
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 
    0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f, 
    0x00c4, 0x00a0, 0x00c7, 0x00c9, 0x00d1, 0x00d6, 0x00dc, 0x00e1, 
    0x00e0, 0x00e2, 0x00e4, 0x06ba, 0x00ab, 0x00e7, 0x00e9, 0x00e8, 
    0x00ea, 0x00eb, 0x00ed, 0x2026, 0x00ee, 0x00ef, 0x00f1, 0x00f3, 
    0x00bb, 0x00f4, 0x00f6, 0x00f7, 0x00fa, 0x00f9, 0x00fb, 0x00fc, 
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x066a, 0x0026, 0x0027, 
    0x0028, 0x0029, 0x002a, 0x002b, 0x060c, 0x002d, 0x002e, 0x002f, 
    0x06f0, 0x06f1, 0x06f2, 0x06f3, 0x06f4, 0x06f5, 0x06f6, 0x06f7, 
    0x06f8, 0x06f9, 0x003a, 0x061b, 0x003c, 0x003d, 0x003e, 0x061f, 
    0x274a, 0x0621, 0x0622, 0x0623, 0x0624, 0x0625, 0x0626, 0x0627, 
    0x0628, 0x0629, 0x062a, 0x062b, 0x062c, 0x062d, 0x062e, 0x062f, 
    0x0630, 0x0631, 0x0632, 0x0633, 0x0634, 0x0635, 0x0636, 0x0637, 
    0x0638, 0x0639, 0x063a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f, 
    0x0640, 0x0641, 0x0642, 0x0643, 0x0644, 0x0645, 0x0646, 0x0647, 
    0x0648, 0x0649, 0x064a, 0x064b, 0x064c, 0x064d, 0x064e, 0x064f, 
    0x0650, 0x0651, 0x0652, 0x067e, 0x0679, 0x0686, 0x06d5, 0x06a4, 
    0x06af, 0x0688, 0x0691, 0x007b, 0x007c, 0x007d, 0x0698, 0x06d2 
};

static unichar_t greek[256] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 
    0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f, 
    0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 
    0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f, 
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 
    0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f, 
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 
    0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f, 
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 
    0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f, 
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 
    0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f, 
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 
    0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f, 
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 
    0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f, 
    0x00c4, 0x00b9, 0x00b2, 0x00c9, 0x00b3, 0x00d6, 0x00dc, 0x0385, 
    0x00e0, 0x00e2, 0x00e4, 0x0384, 0x00a8, 0x00e7, 0x00e9, 0x00e8, 
    0x00ea, 0x00eb, 0x00a3, 0x2122, 0x00ee, 0x00ef, 0x2022, 0x00bd, 
    0x2030, 0x00f4, 0x00f6, 0x00a6, 0x20ac, 0x00f9, 0x00fb, 0x00fc, 
    0x2020, 0x0393, 0x0394, 0x0398, 0x039b, 0x039e, 0x03a0, 0x00df, 
    0x00ae, 0x00a9, 0x03a3, 0x03aa, 0x00a7, 0x2260, 0x00b0, 0x00b7, 
    0x0391, 0x00b1, 0x2264, 0x2265, 0x00a5, 0x0392, 0x0395, 0x0396, 
    0x0397, 0x0399, 0x039a, 0x039c, 0x03a6, 0x03ab, 0x03a8, 0x03a9, 
    0x03ac, 0x039d, 0x00ac, 0x039f, 0x03a1, 0x2248, 0x03a4, 0x00ab, 
    0x00bb, 0x2026, 0x00a0, 0x03a5, 0x03a7, 0x0386, 0x0388, 0x0153, 
    0x2013, 0x2015, 0x201c, 0x201d, 0x2018, 0x2019, 0x00f7, 0x0389, 
    0x038a, 0x038c, 0x038e, 0x03ad, 0x03ae, 0x03af, 0x03cc, 0x038f, 
    0x03cd, 0x03b1, 0x03b2, 0x03c8, 0x03b4, 0x03b5, 0x03c6, 0x03b3, 
    0x03b7, 0x03b9, 0x03be, 0x03ba, 0x03bb, 0x03bc, 0x03bd, 0x03bf, 
    0x03c0, 0x03ce, 0x03c1, 0x03c3, 0x03c4, 0x03b8, 0x03c9, 0x03c2, 
    0x03c7, 0x03c5, 0x03b6, 0x03ca, 0x03cb, 0x0390, 0x03b0, 0x00ad 
};

static unichar_t gujarati[256] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 
    0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f, 
    0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 
    0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f, 
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 
    0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f, 
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 
    0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f, 
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 
    0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f, 
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 
    0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f, 
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 
    0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f, 
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 
    0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f, 
    0x00d7, 0x2212, 0x2013, 0x2014, 0x2018, 0x2019, 0x2026, 0x2022, 
    0x00a9, 0x00ae, 0x2122, 0x008b, 0x008c, 0x008d, 0x008e, 0x008f, 
    0x0965, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097, 
    0x0098, 0x0099, 0x009a, 0x009b, 0x009c, 0x009d, 0x009e, 0x009f, 
    0x00a0, 0x0a81, 0x0a82, 0x0a83, 0x0a85, 0x0a86, 0x0a87, 0x0a88, 
    0x0a89, 0x0a8a, 0x0a8b, 0x00ab, 0x0a8f, 0x0a90, 0x0a8d, 0x00af, 
    0x0a93, 0x0a94, 0x0a91, 0x0a95, 0x0a96, 0x0a97, 0x0a98, 0x0a99, 
    0x0a9a, 0x0a9b, 0x0a9c, 0x0a9d, 0x0a9e, 0x0a9f, 0x0aa0, 0x0aa1, 
    0x0aa2, 0x0aa3, 0x0aa4, 0x0aa5, 0x0aa6, 0x0aa7, 0x0aa8, 0x00c7, 
    0x0aaa, 0x0aab, 0x0aac, 0x0aad, 0x0aae, 0x0aaf, 0x00ce, 0x0ab0, 
    0x00d0, 0x0ab2, 0x0ab3, 0x00d3, 0x0ab5, 0x0ab6, 0x0ab7, 0x0ab8, 
    0x0ab9, 0x200e, 0x0abe, 0x0abf, 0x0ac0, 0x0ac1, 0x0ac2, 0x0ac3, 
    0x00e0, 0x0ac7, 0x0ac8, 0x0ac5, 0x00e4, 0x0acb, 0x0acc, 0x0ac9, 
    0x0acd, 0x0abc, 0x0964, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef, 
    0x00f0, 0x0ae6, 0x0ae7, 0x0ae8, 0x0ae9, 0x0aea, 0x0aeb, 0x0aec, 
    0x0aed, 0x0aee, 0x0aef, 
};

static unichar_t gurmukhi[256] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 
    0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f, 
    0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 
    0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f, 
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 
    0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f, 
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 
    0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f, 
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 
    0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f, 
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 
    0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f, 
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 
    0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f, 
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 
    0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f, 
    0x00d7, 0x2212, 0x2013, 0x2014, 0x2018, 0x2019, 0x2026, 0x2022, 
    0x00a9, 0x00ae, 0x2122, 0x008b, 0x008c, 0x008d, 0x008e, 0x008f, 
    0x0a71, 0x0a5c, 0x0a73, 0x0a72, 0x0a74, 0x0095, 0x0096, 0x0097, 
    0x0098, 0x0099, 0x009a, 0x009b, 0x009c, 0x009d, 0x009e, 0x009f, 
    0x00a0, 0x00a1, 0x0a02, 0x00a3, 0x0a05, 0x0a06, 0x0a07, 0x0a08, 
    0x0a09, 0x0a0a, 0x00aa, 0x00ab, 0x0a0f, 0x0a10, 0x00ae, 0x00af, 
    0x0a13, 0x0a14, 0x00b2, 0x0a15, 0x0a16, 0x0a17, 0x0a18, 0x0a19, 
    0x0a1a, 0x0a1b, 0x0a1c, 0x0a1d, 0x0a1e, 0x0a1f, 0x0a20, 0x0a21, 
    0x0a22, 0x0a23, 0x0a24, 0x0a25, 0x0a26, 0x0a27, 0x0a28, 0x00c7, 
    0x0a2a, 0x0a2b, 0x0a2c, 0x0a2d, 0x0a2e, 0x0a2f, 0x00ce, 0x0a30, 
    0x00d0, 0x0a32, 0x00d2, 0x00d3, 0x0a35, 0xf860, 0x00d6, 0x0a38, 
    0x0a39, 0x200e, 0x0a3e, 0x0a3f, 0x0a40, 0x0a41, 0x0a42, 0x00df, 
    0x00e0, 0x0a47, 0x0a48, 0x00e3, 0x00e4, 0x0a4b, 0x0a4c, 0x00e7, 
    0x0a4d, 0x0a3c, 0x0964, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef, 
    0x00f0, 0x0a66, 0x0a67, 0x0a68, 0x0a69, 0x0a6a, 0x0a6b, 0x0a6c, 
    0x0a6d, 0x0a6e, 0x0a6f, 
};

static unichar_t hebrew[256] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 
    0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f, 
    0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 
    0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f, 
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 
    0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f, 
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 
    0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f, 
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 
    0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f, 
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 
    0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f, 
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 
    0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f, 
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 
    0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f, 
    0x00c4, 0x05f2, 0x00c7, 0x00c9, 0x00d1, 0x00d6, 0x00dc, 0x00e1, 
    0x00e0, 0x00e2, 0x00e4, 0x00e3, 0x00e5, 0x00e7, 0x00e9, 0x00e8, 
    0x00ea, 0x00eb, 0x00ed, 0x00ec, 0x00ee, 0x00ef, 0x00f1, 0x00f3, 
    0x00f2, 0x00f4, 0x00f6, 0x00f5, 0x00fa, 0x00f9, 0x00fb, 0x00fc, 
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x20aa, 0x0027, 
    0x0029, 0x0028, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f, 
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 
    0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f, 
    0xf86a, 0x201e, 0xf89b, 0xf89c, 0xf89d, 0xf89e, 0x05bc, 0xfb4b, 
    0xfb35, 0x2026, 0x00a0, 0x05b8, 0x05b7, 0x05b5, 0x05b6, 0x05b4, 
    0x2013, 0x2014, 0x201c, 0x201d, 0x2018, 0x2019, 0xfb2a, 0xfb2b, 
    0x05bf, 0x05b0, 0x05b2, 0x05b1, 0x05bb, 0x05b9, 0x05b8, 0x05b3, 
    0x05d0, 0x05d1, 0x05d2, 0x05d3, 0x05d4, 0x05d5, 0x05d6, 0x05d7, 
    0x05d8, 0x05d9, 0x05da, 0x05db, 0x05dc, 0x05dd, 0x05de, 0x05df, 
    0x05e0, 0x05e1, 0x05e2, 0x05e3, 0x05e4, 0x05e5, 0x05e6, 0x05e7, 
    0x05e8, 0x05e9, 0x05ea, 0x007d, 0x005d, 0x007b, 0x005b, 0x007c 
};

static unichar_t iceland[256] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 
    0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f, 
    0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 
    0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f, 
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 
    0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f, 
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 
    0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f, 
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 
    0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f, 
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 
    0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f, 
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 
    0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f, 
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 
    0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f, 
    0x00c4, 0x00c5, 0x00c7, 0x00c9, 0x00d1, 0x00d6, 0x00dc, 0x00e1, 
    0x00e0, 0x00e2, 0x00e4, 0x00e3, 0x00e5, 0x00e7, 0x00e9, 0x00e8, 
    0x00ea, 0x00eb, 0x00ed, 0x00ec, 0x00ee, 0x00ef, 0x00f1, 0x00f3, 
    0x00f2, 0x00f4, 0x00f6, 0x00f5, 0x00fa, 0x00f9, 0x00fb, 0x00fc, 
    0x00dd, 0x00b0, 0x00a2, 0x00a3, 0x00a7, 0x2022, 0x00b6, 0x00df, 
    0x00ae, 0x00a9, 0x2122, 0x00b4, 0x00a8, 0x2260, 0x00c6, 0x00d8, 
    0x221e, 0x00b1, 0x2264, 0x2265, 0x00a5, 0x00b5, 0x2202, 0x2211, 
    0x220f, 0x03c0, 0x222b, 0x00aa, 0x00ba, 0x03a9, 0x00e6, 0x00f8, 
    0x00bf, 0x00a1, 0x00ac, 0x221a, 0x0192, 0x2248, 0x2206, 0x00ab, 
    0x00bb, 0x2026, 0x00a0, 0x00c0, 0x00c3, 0x00d5, 0x0152, 0x0153, 
    0x2013, 0x2014, 0x201c, 0x201d, 0x2018, 0x2019, 0x00f7, 0x25ca, 
    0x00ff, 0x0178, 0x2044, 0x20ac, 0x00d0, 0x00f0, 0x00de, 0x00fe, 
    0x00fd, 0x00b7, 0x201a, 0x201e, 0x2030, 0x00c2, 0x00ca, 0x00c1, 
    0x00cb, 0x00c8, 0x00cd, 0x00ce, 0x00cf, 0x00cc, 0x00d3, 0x00d4, 
    0xf8ff, 0x00d2, 0x00da, 0x00db, 0x00d9, 0x0131, 0x02c6, 0x02dc, 
    0x00af, 0x02d8, 0x02d9, 0x02da, 0x00b8, 0x02dd, 0x02db, 0x02c7 
};

static unichar_t romanian[256] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 
    0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f, 
    0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 
    0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f, 
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 
    0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f, 
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 
    0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f, 
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 
    0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f, 
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 
    0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f, 
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 
    0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f, 
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 
    0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f, 
    0x00c4, 0x00c5, 0x00c7, 0x00c9, 0x00d1, 0x00d6, 0x00dc, 0x00e1, 
    0x00e0, 0x00e2, 0x00e4, 0x00e3, 0x00e5, 0x00e7, 0x00e9, 0x00e8, 
    0x00ea, 0x00eb, 0x00ed, 0x00ec, 0x00ee, 0x00ef, 0x00f1, 0x00f3, 
    0x00f2, 0x00f4, 0x00f6, 0x00f5, 0x00fa, 0x00f9, 0x00fb, 0x00fc, 
    0x2020, 0x00b0, 0x00a2, 0x00a3, 0x00a7, 0x2022, 0x00b6, 0x00df, 
    0x00ae, 0x00a9, 0x2122, 0x00b4, 0x00a8, 0x2260, 0x0102, 0x0218, 
    0x221e, 0x00b1, 0x2264, 0x2265, 0x00a5, 0x00b5, 0x2202, 0x2211, 
    0x220f, 0x03c0, 0x222b, 0x00aa, 0x00ba, 0x03a9, 0x0103, 0x0219, 
    0x00bf, 0x00a1, 0x00ac, 0x221a, 0x0192, 0x2248, 0x2206, 0x00ab, 
    0x00bb, 0x2026, 0x00a0, 0x00c0, 0x00c3, 0x00d5, 0x0152, 0x0153, 
    0x2013, 0x2014, 0x201c, 0x201d, 0x2018, 0x2019, 0x00f7, 0x25ca, 
    0x00ff, 0x0178, 0x2044, 0x20ac, 0x2039, 0x203a, 0x021a, 0x021b, 
    0x2021, 0x00b7, 0x201a, 0x201e, 0x2030, 0x00c2, 0x00ca, 0x00c1, 
    0x00cb, 0x00c8, 0x00cd, 0x00ce, 0x00cf, 0x00cc, 0x00d3, 0x00d4, 
    0xf8ff, 0x00d2, 0x00da, 0x00db, 0x00d9, 0x0131, 0x02c6, 0x02dc, 
    0x00af, 0x02d8, 0x02d9, 0x02da, 0x00b8, 0x02dd, 0x02db, 0x02c7 
};

unichar_t MacRomanEnc[256] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 
    0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f, 
    0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 
    0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f, 
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 
    0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f, 
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 
    0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f, 
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 
    0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f, 
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 
    0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f, 
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 
    0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f, 
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 
    0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f, 
    0x00c4, 0x00c5, 0x00c7, 0x00c9, 0x00d1, 0x00d6, 0x00dc, 0x00e1, 
    0x00e0, 0x00e2, 0x00e4, 0x00e3, 0x00e5, 0x00e7, 0x00e9, 0x00e8, 
    0x00ea, 0x00eb, 0x00ed, 0x00ec, 0x00ee, 0x00ef, 0x00f1, 0x00f3, 
    0x00f2, 0x00f4, 0x00f6, 0x00f5, 0x00fa, 0x00f9, 0x00fb, 0x00fc, 
    0x2020, 0x00b0, 0x00a2, 0x00a3, 0x00a7, 0x2022, 0x00b6, 0x00df, 
    0x00ae, 0x00a9, 0x2122, 0x00b4, 0x00a8, 0x2260, 0x00c6, 0x00d8, 
    0x221e, 0x00b1, 0x2264, 0x2265, 0x00a5, 0x00b5, 0x2202, 0x2211, 
    0x220f, 0x03c0, 0x222b, 0x00aa, 0x00ba, 0x03a9, 0x00e6, 0x00f8, 
    0x00bf, 0x00a1, 0x00ac, 0x221a, 0x0192, 0x2248, 0x2206, 0x00ab, 
    0x00bb, 0x2026, 0x00a0, 0x00c0, 0x00c3, 0x00d5, 0x0152, 0x0153, 
    0x2013, 0x2014, 0x201c, 0x201d, 0x2018, 0x2019, 0x00f7, 0x25ca, 
    0x00ff, 0x0178, 0x2044, 0x20ac, 0x2039, 0x203a, 0xfb01, 0xfb02, 
    0x2021, 0x00b7, 0x201a, 0x201e, 0x2030, 0x00c2, 0x00ca, 0x00c1, 
    0x00cb, 0x00c8, 0x00cd, 0x00ce, 0x00cf, 0x00cc, 0x00d3, 0x00d4, 
    0xf8ff, 0x00d2, 0x00da, 0x00db, 0x00d9, 0x0131, 0x02c6, 0x02dc, 
    0x00af, 0x02d8, 0x02d9, 0x02da, 0x00b8, 0x02dd, 0x02db, 0x02c7 
};

static unichar_t thai[256] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 
    0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f, 
    0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 
    0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f, 
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 
    0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f, 
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 
    0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f, 
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 
    0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f, 
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 
    0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f, 
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 
    0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f, 
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 
    0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f, 
    0x00ab, 0x00bb, 0x2026, 0x0e48, 0x0e49, 0x0e4a, 0x0e4b, 0x0e4c, 
    0x0e48, 0x0e49, 0x0e4a, 0x0e4b, 0x0e4c, 0x201c, 0x201d, 0x0e4d, 
    0x0090, 0x2022, 0x0e31, 0x0e47, 0x0e34, 0x0e35, 0x0e36, 0x0e37, 
    0x0e48, 0x0e49, 0x0e4a, 0x0e4b, 0x0e4c, 0x2018, 0x2019, 0x009f, 
    0x00a0, 0x0e01, 0x0e02, 0x0e03, 0x0e04, 0x0e05, 0x0e06, 0x0e07, 
    0x0e08, 0x0e09, 0x0e0a, 0x0e0b, 0x0e0c, 0x0e0d, 0x0e0e, 0x0e0f, 
    0x0e10, 0x0e11, 0x0e12, 0x0e13, 0x0e14, 0x0e15, 0x0e16, 0x0e17, 
    0x0e18, 0x0e19, 0x0e1a, 0x0e1b, 0x0e1c, 0x0e1d, 0x0e1e, 0x0e1f, 
    0x0e20, 0x0e21, 0x0e22, 0x0e23, 0x0e24, 0x0e25, 0x0e26, 0x0e27, 
    0x0e28, 0x0e29, 0x0e2a, 0x0e2b, 0x0e2c, 0x0e2d, 0x0e2e, 0x0e2f, 
    0x0e30, 0x0e31, 0x0e32, 0x0e33, 0x0e34, 0x0e35, 0x0e36, 0x0e37, 
    0x0e38, 0x0e39, 0x0e3a, 0x2060, 0x200b, 0x2013, 0x2014, 0x0e3f, 
    0x0e40, 0x0e41, 0x0e42, 0x0e43, 0x0e44, 0x0e45, 0x0e46, 0x0e47, 
    0x0e48, 0x0e49, 0x0e4a, 0x0e4b, 0x0e4c, 0x0e4d, 0x2122, 0x0e4f, 
    0x0e50, 0x0e51, 0x0e52, 0x0e53, 0x0e54, 0x0e55, 0x0e56, 0x0e57, 
    0x0e58, 0x0e59, 0x00ae, 0x00a9, 
};

static unichar_t turkish[256] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 
    0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f, 
    0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 
    0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f, 
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 
    0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f, 
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 
    0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f, 
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 
    0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f, 
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 
    0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f, 
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 
    0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f, 
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 
    0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f, 
    0x00c4, 0x00c5, 0x00c7, 0x00c9, 0x00d1, 0x00d6, 0x00dc, 0x00e1, 
    0x00e0, 0x00e2, 0x00e4, 0x00e3, 0x00e5, 0x00e7, 0x00e9, 0x00e8, 
    0x00ea, 0x00eb, 0x00ed, 0x00ec, 0x00ee, 0x00ef, 0x00f1, 0x00f3, 
    0x00f2, 0x00f4, 0x00f6, 0x00f5, 0x00fa, 0x00f9, 0x00fb, 0x00fc, 
    0x2020, 0x00b0, 0x00a2, 0x00a3, 0x00a7, 0x2022, 0x00b6, 0x00df, 
    0x00ae, 0x00a9, 0x2122, 0x00b4, 0x00a8, 0x2260, 0x00c6, 0x00d8, 
    0x221e, 0x00b1, 0x2264, 0x2265, 0x00a5, 0x00b5, 0x2202, 0x2211, 
    0x220f, 0x03c0, 0x222b, 0x00aa, 0x00ba, 0x03a9, 0x00e6, 0x00f8, 
    0x00bf, 0x00a1, 0x00ac, 0x221a, 0x0192, 0x2248, 0x2206, 0x00ab, 
    0x00bb, 0x2026, 0x00a0, 0x00c0, 0x00c3, 0x00d5, 0x0152, 0x0153, 
    0x2013, 0x2014, 0x201c, 0x201d, 0x2018, 0x2019, 0x00f7, 0x25ca, 
    0x00ff, 0x0178, 0x011e, 0x011f, 0x0130, 0x0131, 0x015e, 0x015f, 
    0x2021, 0x00b7, 0x201a, 0x201e, 0x2030, 0x00c2, 0x00ca, 0x00c1, 
    0x00cb, 0x00c8, 0x00cd, 0x00ce, 0x00cf, 0x00cc, 0x00d3, 0x00d4, 
    0xf8ff, 0x00d2, 0x00da, 0x00db, 0x00d9, 0xf8a0, 0x02c6, 0x02dc, 
    0x00af, 0x02d8, 0x02d9, 0x02da, 0x00b8, 0x02dd, 0x02db, 0x02c7 
};

enum script_codes {
    sm_roman, sm_japanese, sm_tradchinese, sm_korean,
    sm_arabic, sm_hebrew, sm_greek, sm_cyrillic,
    sm_rsymbol, sm_devanagari, sm_gurmukhi, sm_gujarati,
    sm_oriya, sm_bengali, sm_tamil, sm_telugu,
    sm_kannada, sm_malayalam, sm_sinhalese, sm_burmese,
    sm_khmer, sm_thai, sm_laotian, sm_georgian,
    sm_armenian, sm_simpchinese, sm_tibetan, sm_mongolian,
    sm_geez, sm_slavic, sm_vietnamese, sm_sindhi,
    sm_max };

static unichar_t *macencodings[] = {
	MacRomanEnc,
	NULL/*Essentially SJIS*/,
	NULL/*Essentially Big 5*/,
	NULL/*Essentially Wansung*/,
	arabic,
	hebrew,
	greek,
	cyrillic,
	NULL,		/* rsymbol, whatever that is */
	devanagari,
/*10*/	gurmukhi,
	gujarati,
	NULL,		/* oriya */
	NULL,		/* bengali */
	NULL,		/* Tamil */
	NULL,		/* Telugu */
	NULL,		/* Kannada */
	NULL,		/* Malayalam */
	NULL,		/* Sinhalese */
	NULL,		/* Burmese */
/*20*/	NULL,		/* Khmer */
	thai,
	NULL,		/* Lao */
	NULL,		/* Georgian */
	NULL,		/* Armenian */
/*25*/	NULL,/* SimpChinese, GB2312 offset by 0x8080 to 0xa1a1 */
	NULL,		/* Tibetan */
	NULL,		/* Mongolian */
	NULL,		/* Geex/Ethiopic */
	centeuro,	/* Baltic/Slavic */
/*30*/	NULL,		/* Vietnamese */
	NULL,		/* Extended Arabic for Sindhi */
	NULL		/* Uninterpreted */
};

/* The icelandic encoding also uses 0 (mac roman) encoding even though it's not*/
/* The turkish encoding also uses 0 (mac roman) encoding even though it's not*/
/* The croatian encoding also uses 0 (mac roman) encoding even though it's not*/
/* The romanian encoding also uses 0 (mac roman) encoding even though it's not*/

/* I've no idea what encoding code farsi uses, it isn't documented to be arabic 4, nor is it documented to have its own code */

static uint8_t _MacScriptFromLanguage[] = {
	sm_roman,		/* English */
	sm_roman,		/* French */
	sm_roman,		/* German */
	sm_roman,		/* Italian */
	sm_roman,		/* Dutch */
	sm_roman,		/* Swedish */
	sm_roman,		/* Spanish */
	sm_roman,		/* Danish */
	sm_roman,		/* Portuguese */
	sm_roman,		/* Norwegian */
/*10*/	sm_hebrew,		/* Hebrew */
	sm_japanese,		/* Japanese */
	sm_arabic,		/* Arabic */
	sm_roman,		/* Finnish */
	sm_greek,		/* Greek */
	sm_roman,		/* Icelandic */	/* Modified roman */
	sm_roman,		/* Maltese */
	sm_roman,		/* Turkish */	/* Modified roman */
	sm_roman,		/* Croatian */	/* Modified roman */
	sm_tradchinese,		/* Traditional Chinese */
/*20*/	sm_arabic,		/* Urdu (I assume arabic) */
	sm_devanagari,		/* Hindi (I assume) */
	sm_thai,		/* Thai */
	sm_korean,		/* Korean */
	sm_slavic,		/* Lithuanian */
	sm_slavic,		/* Polish */
	sm_slavic,		/* Hungarian */
	sm_slavic,		/* Estonian */
	sm_slavic,		/* Latvian */
	sm_roman,		/* Sami (Lappish) */
/*30*/	sm_roman,		/* Faroese (Icelandic) */	/* Modified roman */
	sm_arabic,		/* Farsi/Persian */	/* Modified Arabic */
	sm_cyrillic,		/* Russian */
	sm_simpchinese,		/* Simplified Chinese */
	sm_roman,		/* Flemish */
	sm_roman,		/* Irish Gaelic */
	sm_roman,		/* albanian (???) */
	sm_roman,		/* Romanian */	/* Modified roman */
	sm_slavic,		/* Czech */
	sm_slavic,		/* Slovak */
/*40*/	sm_slavic,		/* Slovenian */
	sm_roman,		/* Yiddish */
	sm_cyrillic,		/* Serbian */
	sm_cyrillic,		/* Macedonian */
	sm_cyrillic,		/* Bulgarian */
	sm_cyrillic,		/* Ukrainian */
	sm_cyrillic,		/* Byelorussian */
	sm_cyrillic,		/* Uzbek */
	sm_cyrillic,		/* Kazakh */
	sm_cyrillic,		/* Axerbaijani (Cyrillic) */
/*50*/	sm_arabic,		/* Axerbaijani (Arabic) */
	sm_armenian,		/* Armenian */
	sm_georgian,		/* Georgian */
	sm_cyrillic,		/* Moldavian */
	sm_cyrillic,		/* Kirghiz */
	sm_cyrillic,		/* Tajiki */
	sm_cyrillic,		/* Turkmen */
	sm_mongolian,		/* Mongolian (Mongolian) */
	sm_cyrillic,		/* Mongolian (cyrillic) */
	sm_arabic,		/* Pashto */
/*60*/	sm_arabic,		/* Kurdish */
	sm_devanagari,		/* Kashmiri (???) */
	sm_sindhi,		/* Sindhi */
	sm_tibetan,		/* Tibetan */
	sm_tibetan,		/* Nepali (???) */
	sm_devanagari,		/* Sanskrit */
	sm_devanagari,		/* Marathi */
	sm_bengali,		/* Bengali */
	sm_bengali,		/* Assamese (???) */
	sm_gujarati,		/* Gujarati */
/*70*/	sm_gujarati,		/* Punjabi (???) */
	sm_oriya,		/* Oriya */
	sm_malayalam,		/* Malayalam */
	sm_kannada,		/* Kannada */
	sm_tamil,		/* Tamil */
	sm_telugu,		/* Telugu */
	sm_sinhalese,		/* Sinhalese */
	sm_burmese,		/* Burmese */
	sm_khmer,		/* Khmer */
	sm_laotian,		/* Lao */
/*80*/	sm_vietnamese,		/* Vietnamese */
	sm_arabic,		/* Indonesian */
	sm_roman,		/* Tagalog (???) */
	sm_roman,		/* Malay (roman) */
	sm_arabic,		/* Malay (arabic) */
	sm_roman,		/* Amharic (???) */
	sm_roman,		/* Tigrinya (???) */
	sm_roman,		/* Galla (???) */
	sm_roman,		/* Somali (???) */
	sm_roman,		/* Swahili (???) */
/*90*/	sm_roman,		/* Kinyarwanda/Ruanda (???) */
	sm_roman,		/* Rundi (???) */
	sm_roman,		/* Nyanja/Chewa (???) */
	sm_roman,		/* Malagasy */
/*94*/	sm_roman,		/* Esperanto */
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
/*100*/	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
/*110*/	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
/*120*/	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
/*128*/	sm_roman,		/* Welsh */
	sm_roman,		/* Basque */
/*130*/	sm_roman,		/* Catalan */
	sm_roman,		/* Latin */
	sm_roman,		/* Quechua (???) */
	sm_roman,		/* Guarani (???) */
	sm_roman,		/* Aymara (???) */
	sm_cyrillic,		/* Tatar (???) */
	sm_cyrillic,		/* Uighur (???) */
	sm_cyrillic,		/* Dzongkha (???) */
	sm_roman,		/* Javanese (roman) */
	sm_roman,		/* Sundanese (roman) */
/*140*/	sm_roman,		/* Galician */
	sm_roman,		/* Afrikaans */
	sm_roman,		/* Breton */
	sm_roman,		/* Inuktitut */
	sm_roman,		/* Scottish Gaelic */
	sm_roman,		/* Manx Gaelic */
	sm_roman,		/* Irish Gaelic (with dot) */
	sm_roman,		/* Tongan */
	sm_greek,		/* Greek (polytonic) */
	sm_roman,		/* Greenlandic */	/* Presumably icelandic? */
/*150*/	sm_roman,		/* Azebaijani (roman) */
	0xff
};

static uint16_t _WinLangFromMac[] = {
	0x409,		/* English */
	0x40c,		/* French */
	0x407,		/* German */
	0x410,		/* Italian */
	0x413,		/* Dutch */
	0x41d,		/* Swedish */
	0x40a,		/* Spanish */
	0x406,		/* Danish */
	0x416,		/* Portuguese */
	0x414,		/* Norwegian */
/*10*/	0x40d,		/* Hebrew */
	0x411,		/* Japanese */
	0x401,		/* Arabic */
	0x40b,		/* Finnish */
	0x408,		/* Greek */
	0x40f,		/* Icelandic */
	0x43a,		/* Maltese */
	0x41f,		/* Turkish */
	0x41a,		/* Croatian */
	0x404,		/* Traditional Chinese */
/*20*/	0x420,		/* Urdu */
	0x439,		/* Hindi */
	0x41e,		/* Thai */
	0x412,		/* Korean */
	0x427,		/* Lithuanian */
	0x415,		/* Polish */
	0x40e,		/* Hungarian */
	0x425,		/* Estonian */
	0x426,		/* Latvian */
	0x43b,		/* Sami (Lappish) */
/*30*/	0x438,		/* Faroese (Icelandic) */
	0x429,		/* Farsi/Persian */
	0x419,		/* Russian */
	0x804,		/* Simplified Chinese */
	0x813,		/* Flemish */
	0x43c,		/* Irish Gaelic */
	0x41c,		/* albanian */
	0x418,		/* Romanian */
	0x405,		/* Czech */
	0x41b,		/* Slovak */
/*40*/	0x424,		/* Slovenian */
	0x43d,		/* Yiddish */
	0xc1a,		/* Serbian */
	0x42f,		/* Macedonian */
	0x402,		/* Bulgarian */
	0x422,		/* Ukrainian */
	0x423,		/* Byelorussian */
	0x843,		/* Uzbek */
	0x43f,		/* Kazakh */
	0x42c,		/* Azerbaijani (Cyrillic) */
/*50*/	0x82c,		/* Azerbaijani (Arabic) */
	0x42b,		/* Armenian */
	0x437,		/* Georgian */
	0x818,		/* Moldavian */
	0x440,		/* Kirghiz */
	0x428,		/* Tajiki */
	0x442,		/* Turkmen */
	0x450,		/* Mongolian (Mongolian) */
	0x850,		/* Mongolian (cyrillic) */
	0x463,		/* Pashto */
/*60*/	0xffff,		/* Kurdish */
	0x860,		/* Kashmiri */
	0x459,		/* Sindhi */
	0xffff,		/* Tibetan */
	0x461,		/* Nepali */
	0x44f,		/* Sanskrit */
	0x44e,		/* Marathi */
	0x445,		/* Bengali */
	0x44d,		/* Assamese */
	0x447,		/* Gujarati */
/*70*/	0x446,		/* Punjabi */
	0x448,		/* Oriya */
	0x44c,		/* Malayalam */
	0x44b,		/* Kannada */
	0x449,		/* Tamil */
	0x44a,		/* Telugu */
	0x45b,		/* Sinhalese */
	0x455,		/* Burmese */
	0x453,		/* Khmer */
	0x454,		/* Lao */
/*80*/	0x42a,		/* Vietnamese */
	0x421,		/* Indonesian */
	0x464,		/* Tagalog */
	0x43e,		/* Malay (latin) */
	0x83e,		/* Malay (arabic) */
	0x45e,		/* Amharic */
	0x473,		/* Tigrinya */
	0x472,		/* Galla, oromo, afan */
	0x477,		/* Somali */
	0x441,		/* Swahili */
/*90*/	0xffff,		/* Kinyarwanda/Ruanda */
	0xffff,		/* Rundi/Kirundi */
	0xffff,		/* Nyanja/Chewa */
	0xffff,		/* Malagasy */
/*94*/	0xffff,		/* Esperanto */
	0xffff,
	0xffff,
	0xffff,
	0xffff,
	0xffff,
/*100*/	0xffff,
	0xffff,
	0xffff,
	0xffff,
	0xffff,
	0xffff,
	0xffff,
	0xffff,
	0xffff,
	0xffff,
/*110*/	0xffff,
	0xffff,
	0xffff,
	0xffff,
	0xffff,
	0xffff,
	0xffff,
	0xffff,
	0xffff,
	0xffff,
/*120*/	0xffff,
	0xffff,
	0xffff,
	0xffff,
	0xffff,
	0xffff,
	0xffff,
	0xffff,
/*128*/	0x452,		/* Welsh */
	0x42d,		/* Basque */
/*130*/	0x403,		/* Catalan */
	0x476,		/* Latin */
	0xffff,		/* Quechua */
	0x474,		/* Guarani */
	0xffff,		/* Aymara */
	0x444,		/* Tatar */
	0xffff,		/* Uighur */
	0xffff,		/* Dzongkha/Bhutani */
	0xffff,		/* Javanese (roman) */
	0xffff,		/* Sundanese (roman) */
/*140*/	0x456,		/* Galician */
	0x436,		/* Afrikaans */
	0xffff,		/* Breton */
	0x45d,		/* Inuktitut */
	0x43c,		/* Scottish Gaelic */
	0xc3c,		/* Manx Gaelic */
	0x83c,		/* Irish Gaelic (with dot) */
	0xffff,		/* Tongan */
	0xffff,		/* Greek (polytonic) */
	0xffff,		/* Greenlandic */	/* Presumably icelandic? */
/*150*/	0x42c,		/* Azebaijani (roman) */
	0xffff
};

static char *LanguageCodesFromMacLang[] = {
	"en",		/* English */
	"fr",		/* French */
	"de",		/* German */
	"it",		/* Italian */
	"nl",		/* Dutch */
	"sv",		/* Swedish */
	"es",		/* Spanish */
	"da",		/* Danish */
	"pt",		/* Portuguese */
	"no",		/* Norwegian */
/*10*/	"he",		/* Hebrew */
	"ja",		/* Japanese */
	"ar",		/* Arabic */
	"fi",		/* Finnish */
	"el",		/* Greek */
	"is",		/* Icelandic */
	"ml",		/* Maltese */
	"tr",		/* Turkish */
	"hr",		/* Croatian */
	"zh_TW",	/* Traditional Chinese */	/* zh_HK */
/*20*/	"ur",		/* Urdu */
	"hi",		/* Hindi */
	"th",		/* Thai */
	"ko",		/* Korean */
	"lt",		/* Lithuanian */
	"pl",		/* Polish */
	"hu",		/* Hungarian */
	"et",		/* Estonian */
	"lv",		/* Latvian */
	"smi",		/* Sami (Lappish) */
/*30*/	"fo",		/* Faroese (Icelandic) */
	"fa",		/* Farsi/Persian */
	"ru",		/* Russian */
	"zh_CN",	/* Simplified Chinese */
	"nl_BE",	/* Flemish */	/* Flemish doesn't rate a language code, use dutch */
	"ga",		/* Irish Gaelic */
	"sq",		/* albanian */
	"ro",		/* Romanian */
	"cs",		/* Czech */
	"sk",		/* Slovak */
/*40*/	"sl",		/* Slovenian */
	"yi",		/* Yiddish */
	"sr",		/* Serbian */
	"mk",		/* Macedonian */
	"bg",		/* Bulgarian */
	"uk",		/* Ukrainian */
	"be",		/* Byelorussian */
	"uz",		/* Uzbek */
	"kk",		/* Kazakh */
	"az",		/* Axerbaijani (Cyrillic) */
/*50*/	"az",		/* Axerbaijani (Arabic) */
	"hy",		/* Armenian */
	"ka",		/* Georgian */
	"mo",		/* Moldavian */
	"ky",		/* Kirghiz */
	"tg",		/* Tajiki */
	"tk",		/* Turkmen */
	"mn",		/* Mongolian (Mongolian) */
	"mn",		/* Mongolian (cyrillic) */
	"ps",		/* Pashto */
/*60*/	"ku",		/* Kurdish */
	"ks",		/* Kashmiri */
	"sd",		/* Sindhi */
	"bo",		/* Tibetan */
	"ne",		/* Nepali */
	"sa",		/* Sanskrit */
	"mr",		/* Marathi */
	"bn",		/* Bengali */
	"as",		/* Assamese */
	"gu",		/* Gujarati */
/*70*/	"pa",		/* Punjabi */
	"or",		/* Oriya */
	"mal",		/* Malayalam */
	"kn",		/* Kannada */
	"ta",		/* Tamil */
	"te",		/* Telugu */
	"si",		/* Sinhalese */
	"my",		/* Burmese */
	"km",		/* Khmer */
	"lo",		/* Lao */
/*80*/	"vi",		/* Vietnamese */
	"id",		/* Indonesian */
	"tl",		/* Tagalog */
	"ms",		/* Malay (roman) */
	"ms",		/* Malay (arabic) */
	"am",		/* Amharic */
	"ti",		/* Tigrinya */
	"om",		/* Galla */
	"so",		/* Somali */
	"sw",		/* Swahili */
/*90*/	"rw",		/* Kinyarwanda/Ruanda */
	"rn",		/* Rundi */
	"nya",		/* Nyanja/Chewa */
	"mg",		/* Malagasy */
/*94*/	"eo",		/* Esperanto */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
/*100*/	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
/*110*/	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
/*120*/	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
/*128*/	"cy",		/* Welsh */
	"eu",		/* Basque */
/*130*/	"ca",		/* Catalan */
	"la",		/* Latin */
	"qu",		/* Quechua */
	"gn",		/* Guarani */
	"ay",		/* Aymara */
	"tt",		/* Tatar */
	"ug",		/* Uighur */
	"dz",		/* Dzongkha */
	"jv",		/* Javanese (roman) */
	"su",		/* Sundanese (roman) */
/*140*/	"gl",		/* Galician */
	"af",		/* Afrikaans */
	"be",		/* Breton */
	"iu",		/* Inuktitut */
	"gd",		/* Scottish Gaelic */
	"gv",		/* Manx Gaelic */
	NULL,		/* Irish Gaelic (with dot) */
	"to",		/* Tongan */
	"grc",		/* Greek (polytonic) */
	"kl",		/* Greenlandic */	/* Presumably icelandic? */
/*150*/	"az",		/* Azebaijani (roman) */
	NULL
};

static const unichar_t *MacEncLangToTable(int macenc,int maclang) {
    const unichar_t *table = macencodings[macenc];

    if ( maclang==15 /* Icelandic */ ||
	    maclang==30 /* Faroese */ ||
	    maclang==149 /* Greenlandic */ )
	table = iceland;
    else if ( maclang == 17 /* turkish */ )
	table = turkish;
    else if ( maclang == 18 /* croatian */ )
	table = croatian;
    else if ( maclang == 37 /* romanian */ )
	table = romanian;
    else if ( maclang == 31 /* Farsi/Persian */ )
	table = farsi;
return( table );
}

char *MacStrToUtf8(const char *str,int macenc,int maclang) {
    const unichar_t *table;
    char *ret, *rpt;
    const uint8_t *ustr = (uint8_t *) str;

    if ( str==NULL )
return( NULL );

    if ( macenc==sm_japanese || macenc==sm_korean || macenc==sm_tradchinese ||
	    macenc == sm_simpchinese ) {
	Encoding *enc = FindOrMakeEncoding(macenc==sm_japanese ? "Sjis" :
					    macenc==sm_korean ? "EUC-KR" :
			                    macenc==sm_tradchinese ? "Big5" :
			                      "EUC-CN" );
	iconv_t *toutf8;
	ICONV_CONST char *in;
	char *out;
	size_t inlen, outlen;
	if ( enc==NULL )
return( NULL );
	toutf8 = iconv_open("UTF-8",enc->iconv_name!=NULL?enc->iconv_name:enc->enc_name);
	if ( toutf8==(iconv_t) -1 || toutf8==NULL )
return( NULL );
	in = (char *) str;
	inlen = strlen(in);
	outlen = (inlen+1)*4;
	out = (char *) (ret = malloc(outlen+2));
	iconv(toutf8,&in,&inlen,&out,&outlen);
	out[0] = '\0';
	iconv_close(toutf8);
return( ret );
    }

    if ( macenc<0 || macenc>31 ) {
	IError( "Invalid mac encoding %d.\n", macenc );
return( NULL );
    }
    table = MacEncLangToTable(macenc,maclang);

    if ( table==NULL )
return( NULL );

    ret = malloc(strlen(str)*4+1);
    for ( rpt = ret; *ustr; ++ustr ) {
	int ch = table[*ustr];
	rpt = utf8_idpb(rpt,ch,0);
    }
    *rpt = '\0';
return( ret );
}

char *Utf8ToMacStr(const char *ustr,int macenc,int maclang) {
    char *ret, *rpt;
    const unichar_t *table;
    int i, ch;

    if ( ustr==NULL )
return( NULL );

    if ( macenc==sm_japanese || macenc==sm_korean || macenc==sm_tradchinese ||
	    macenc == sm_simpchinese ) {
	Encoding *enc = FindOrMakeEncoding(macenc==sm_japanese ? "Sjis" :
					    macenc==sm_korean ? "EUC-KR" :
			                    macenc==sm_tradchinese ? "Big5" :
			                      "EUC-CN" );
	iconv_t fromutf8;
	ICONV_CONST char *in;
	char *out;
	size_t inlen, outlen;
	if ( enc==NULL )
return( NULL );
	fromutf8 = iconv_open(enc->iconv_name!=NULL?enc->iconv_name:enc->enc_name,"UTF-8");
	if ( fromutf8==(iconv_t) -1 || fromutf8==NULL )
return( NULL );
	in = (char *) ustr;
	inlen = strlen(ustr);
	outlen = sizeof(unichar_t)*strlen(ustr);
	out = ret = malloc(outlen+sizeof(unichar_t));
	iconv(fromutf8,&in,&inlen,&out,&outlen);
	out[0] = out[1] = '\0';
	out[2] = out[3] = '\0';
	iconv_close(fromutf8);
return( ret );
    }

    table = macencodings[macenc];

    if ( maclang==15 /* Icelandic */ ||
	    maclang==30 /* Faroese */ ||
	    maclang==149 /* Greenlandic */ )
	table = iceland;
    else if ( maclang == 17 /* turkish */ )
	table = turkish;
    else if ( maclang == 18 /* croatian */ )
	table = croatian;
    else if ( maclang == 37 /* romanian */ )
	table = romanian;
    else if ( maclang == 31 /* Farsi/Persian */ )
	table = farsi;

    if ( table==NULL )
return( NULL );

    ret = malloc(strlen(ustr)+1);
    for ( rpt = ret; (ch=utf8_ildb(&ustr)); ) {
	for ( i=0; i<256; ++i )
	    if ( table[i]==ch ) {
		*rpt++ = i;
	break;
	    }
    }
    *rpt = '\0';
return( ret );
}

uint8_t MacEncFromMacLang(int maclang) {
    if ( maclang<0 || maclang>=sizeof(_MacScriptFromLanguage)/sizeof(_MacScriptFromLanguage[0]))
return( 0xff );

return( _MacScriptFromLanguage[maclang] );
}

uint16_t WinLangFromMac(int maclang) {

    if ( maclang<0 || maclang>=sizeof(_WinLangFromMac)/sizeof(_WinLangFromMac[0]))
return( 0xffff );

return( _WinLangFromMac[maclang] );
}

uint16_t WinLangToMac(int winlang) {
    int i;

    for ( i=0; i<sizeof(_WinLangFromMac)/sizeof(_WinLangFromMac[0]); ++i )
	if ( _WinLangFromMac[i] == winlang )
return( i );

    winlang &= 0xff;
    for ( i=0; i<sizeof(_WinLangFromMac)/sizeof(_WinLangFromMac[0]); ++i )
	if ( (_WinLangFromMac[i]&0xff) == winlang )
return( i );

return( 0xffff );
}

int CanEncodingWinLangAsMac(int winlang) {
    int maclang = WinLangToMac(winlang);
    int macenc = MacEncFromMacLang(maclang);

    if ( macenc==0xff )
return( false );
    if ( macencodings[macenc]==NULL )
return( false );

return( true );
}

const int32_t *MacEncToUnicode(int script,int lang) {
    static int32_t temp[256];
    int i;
    const unichar_t *table;

    table = MacEncLangToTable(script,lang);
    if ( table==NULL )
return( NULL );
    for ( i=0; i<256; ++i )
	temp[i] = table[i];
return( temp );
}

int MacLangFromLocale(void) {
    /*const char *loc = setlocale(LC_MESSAGES,NULL);*/ /* This always returns "C" for me, even when it shouldn't be */
    const char *loc;
    static int found=-1;
    int i;

    if ( found!=-1 )
return( found );

    loc = getenv("LC_ALL");
    if ( loc==NULL ) loc = getenv("LC_MESSAGES");
    if ( loc==NULL ) loc = getenv("LANG");

    if ( loc==NULL ) {
	found=0;		/* Default to english */
return(found);
    }
    if ( strncmp(loc,"nl_BE",5)==0 ) {
	found = 34;
return( found );
    }
    for ( i=0; i<sizeof(LanguageCodesFromMacLang)/sizeof(LanguageCodesFromMacLang[0]); ++i ) {
	if ( LanguageCodesFromMacLang[i]!=NULL &&
		strncmp(loc,LanguageCodesFromMacLang[i],strlen(LanguageCodesFromMacLang[i]))==0 ) {
	    found = i;
return( found );
	}
    }
    if ( strncmp(loc,"zh_HK",2)==0 )	/* I think there are other traditional locales than Hong Kong and Taiwan (?Singapore?) so any chinese we don't recognize */
	found = 19;
    else
	found = 0;
return( found );
}

char *PickNameFromMacName(struct macname *mn) {
    int lang = MacLangFromLocale();
    struct macname *first=mn, *english=NULL;

    while ( mn!=NULL ) {
	if ( mn->lang==lang )
    break;
	else if ( mn->lang==0 )
	    english = mn;
	mn = mn->next;
    }
    if ( mn==NULL )
	mn = english;
    if ( mn==NULL )
	mn = first;
    if ( mn==NULL )
return( NULL );

return( MacStrToUtf8(mn->name,mn->enc,mn->lang));
}

char *FindEnglishNameInMacName(struct macname *mn) {

    while ( mn!=NULL ) {
	if ( mn->lang==0 )
    break;
	mn = mn->next;
    }
    if ( mn==NULL )
return( NULL );

return( MacStrToUtf8(mn->name,mn->enc,mn->lang));
}

MacFeat *FindMacFeature(SplineFont *sf, int feat, MacFeat **secondary) {
    MacFeat *from_f, *from_p;

    for ( from_f = sf->features; from_f!=NULL && from_f->feature!=feat; from_f=from_f->next );
    for ( from_p = default_mac_feature_map; from_p!=NULL && from_p->feature!=feat; from_p=from_p->next );
    if ( from_f!=NULL ) {
	if ( secondary!=NULL ) *secondary = from_p;
return( from_f );
    }
    if ( secondary!=NULL ) *secondary = NULL;
return( from_p );
}

struct macsetting *FindMacSetting(SplineFont *sf, int feat, int set,
	struct macsetting **secondary) {
    MacFeat *from_f, *from_p;
    struct macsetting *s_f, *s_p;

    if ( sf!=NULL )
	for ( from_f = sf->features; from_f!=NULL && from_f->feature!=feat; from_f=from_f->next );
    else
	from_f = NULL;
    for ( from_p = default_mac_feature_map; from_p!=NULL && from_p->feature!=feat; from_p=from_p->next );
    s_f = s_p = NULL;
    if ( from_f!=NULL )
	for ( s_f = from_f->settings; s_f!=NULL && s_f->setting!=set; s_f=s_f->next );
    if ( from_p!=NULL )
	for ( s_p = from_p->settings; s_p!=NULL && s_p->setting!=set; s_p=s_p->next );
    if ( s_f!=NULL ) {
	if ( secondary!=NULL ) *secondary = s_p;
return( s_f );
    }
    if ( secondary!=NULL ) *secondary = NULL;
return( s_p );
}

struct macname *FindMacSettingName(SplineFont *sf, int feat, int set) {
    MacFeat *from_f, *from_p;
    struct macsetting *s;

    if ( sf != NULL )
	for ( from_f = sf->features; from_f!=NULL && from_f->feature!=feat; from_f=from_f->next );
    else
	from_f = NULL;
    for ( from_p = default_mac_feature_map; from_p!=NULL && from_p->feature!=feat; from_p=from_p->next );
    if ( set==-1 ) {
	if ( from_f!=NULL && from_f->featname!=NULL )
return( from_f->featname );
	else if ( from_p!=NULL )
return( from_p->featname );
return( NULL );
    }
    s = NULL;
    if ( from_f!=NULL )
	for ( s = from_f->settings; s!=NULL && s->setting!=set; s=s->next );
    if ( (s==NULL || s->setname==NULL) && from_p!=NULL )
	for ( s = from_p->settings; s!=NULL && s->setting!=set; s=s->next );
    if ( s!=NULL )
return( s->setname );

return( NULL );
}

struct macsettingname macfeat_otftag[] = {
    { 1, 0, CHR('r','l','i','g') },	/* Required ligatures */
    { 1, 2, CHR('l','i','g','a') },	/* Common ligatures */
    { 1, 4, CHR('d','l','i','g') },	/* rare ligatures => discretionary */
    /* { 1, 4, CHR('h','l','i','g') },	/\* rare ligatures => historic *\/ */
    /* { 1, 4, CHR('a','l','i','g') },	/\* rare ligatures => ?ancient? *\/ */
    /* 2, 1, partially connected cursive */
    { 2, 2, CHR('i','s','o','l') },	/* Arabic forms */
    { 2, 2, CHR('c','a','l','t') },	/* ??? */
    /* 3, 1, all caps */
    /* 3, 2, all lower */
    { 3, 3, CHR('s','m','c','p') },	/* small caps */
    /* 3, 4, initial caps */
    /* 3, 5, initial caps, small caps */
    { 4, 0, CHR('v','r','t','2') },	/* vertical forms => vertical rotation */
    /* { 4, 0, CHR('v','k','n','a') },	/\* vertical forms => vertical kana *\/ */
    { 6, 0, CHR('t','n','u','m') },	/* monospace numbers => Tabular numbers */
    { 10, 1, CHR('s','u','p','s') },	/* superior vertical position => superscript */
    { 10, 2, CHR('s','u','b','s') },	/* inferior vertical position => subscript */
    /* { 10, 3, CHR('s','u','p','s') },	/\* ordinal vertical position => superscript *\/ */
    { 11, 1, CHR('a','f','r','c') },	/* vertical fraction => fraction ligature */
    { 11, 2, CHR('f','r','a','c') },	/* diagonal fraction => fraction ligature */
    { 16, 1, CHR('o','r','n','m') },	/* vertical fraction => fraction ligature */
    { 20, 0, CHR('t','r','a','d') },	/* traditional characters => traditional forms */
    /* { 20, 0, CHR('t','n','a','m') },	/\* traditional characters => traditional names *\/ */
    { 20, 1, CHR('s','m','p','l') },	/* simplified characters */
    { 20, 2, CHR('j','p','7','8') },	/* jis 1978 */
    { 20, 3, CHR('j','p','8','3') },	/* jis 1983 */
    { 20, 4, CHR('j','p','9','0') },	/* jis 1990 */
    { 21, 0, CHR('o','n','u','m') },	/* lower case number => old style numbers */
    { 22, 0, CHR('p','w','i','d') },	/* proportional text => proportional widths */
    { 22, 2, CHR('h','w','i','d') },	/* half width text => half widths */
    { 22, 3, CHR('f','w','i','d') },	/* full width text => full widths */
    { 25, 0, CHR('f','w','i','d') },	/* full width kana => full widths */
    { 25, 1, CHR('p','w','i','d') },	/* proportional kana => proportional widths */
    { 26, 0, CHR('f','w','i','d') },	/* full width ideograph => full widths */
    { 26, 1, CHR('p','w','i','d') },	/* proportional ideograph => proportional widths */
    { 103, 0, CHR('h','w','i','d') },	/* half width cjk roman => half widths */
    { 103, 1, CHR('p','w','i','d') },	/* proportional cjk roman => proportional widths */
    { 103, 3, CHR('f','w','i','d') },	/* full width cjk roman => full widths */
    { 0, 0, 0 }
}, *user_macfeat_otftag;

static struct macname fs_names[] = {
	{ &fs_names[146], 0, 0, "All Typographic Features" },
	{ &fs_names[147], 0, 0, "All Type Features" },
	{ &fs_names[148], 0, 0, "Ligatures" },
	{ &fs_names[149], 0, 0, "Required Ligatures" },
	{ &fs_names[150], 0, 0, "Common Ligatures" },
	{ &fs_names[151], 0, 0, "Rare Ligatures" },
	{ &fs_names[152], 0, 0, "Logo Ligatures" },
	{ &fs_names[153], 0, 0, "Rebus Ligatures" },
	{ &fs_names[154], 0, 0, "Diphthong Ligatures" },
	{ &fs_names[155], 0, 0, "Squared Ligatures" },
	{ &fs_names[156], 0, 0, "Abbreviated Squared Ligatures" },
	{ &fs_names[157], 0, 0, "Cursive connection" },
	{ &fs_names[158], 0, 0, "Unconnected" },
	{ &fs_names[159], 0, 0, "Partially connected" },
	{ &fs_names[160], 0, 0, "Cursive" },
	{ &fs_names[161], 0, 0, "Letter Case" },
	{ &fs_names[162], 0, 0, "Upper & Lower Case" },
	{ &fs_names[163], 0, 0, "All Capitals" },
	{ &fs_names[164], 0, 0, "All Lower Case" },
	{ &fs_names[165], 0, 0, "Small Caps" },
	{ &fs_names[166], 0, 0, "Initial Caps" },
	{ &fs_names[167], 0, 0, "Initial and Small Caps" },
	{ &fs_names[168], 0, 0, "Vertical Substitution" },
	{ &fs_names[169], 0, 0, "Vertical Substitution" },
	{ &fs_names[170], 0, 0, "No Vertical Substitution" },
	{ &fs_names[171], 0, 0, "Linguistic Rearrangement" },
	{ &fs_names[172], 0, 0, "Linguistic Rearrangement" },
	{ &fs_names[173], 0, 0, "No Linguistic Rearrangement" },
	{ &fs_names[174], 0, 0, "Number Spacing" },
	{ &fs_names[175], 0, 0, "Monospaced Numbers" },
	{ &fs_names[176], 0, 0, "Proportional Numbers" },
	{ &fs_names[177], 0, 0, "Smart Swashes" },
	{ &fs_names[178], 0, 0, "Word Initial Swashes" },
	{ &fs_names[179], 0, 0, "Word Final Swashes" },
	{ &fs_names[180], 0, 0, "Line Initial Swashes" },
	{ &fs_names[181], 0, 0, "Line Final Swashes" },
	{ &fs_names[182], 0, 0, "Non-Final Swashes" },
	{ &fs_names[183], 0, 0, "Diacritics" },
	{ &fs_names[184], 0, 0, "Show Diacritics" },
	{ &fs_names[185], 0, 0, "Hide Diacritics" },
	{ &fs_names[186], 0, 0, "Decompose Diacritics" },
	{ &fs_names[187], 0, 0, "Vertical Position" },
	{ &fs_names[188], 0, 0, "Normal Vertical Position" },
	{ &fs_names[189], 0, 0, "Superiors" },
	{ &fs_names[190], 0, 0, "Inferiors" },
	{ &fs_names[191], 0, 0, "Ordinals" },
	{ &fs_names[192], 0, 0, "Fractions" },
	{ &fs_names[193], 0, 0, "No Fractions" },
	{ &fs_names[194], 0, 0, "Vertical Fractions" },
	{ &fs_names[195], 0, 0, "Diagonal Fractions" },
	{ &fs_names[196], 0, 0, "Overlapping Characters" },
	{ &fs_names[197], 0, 0, "Prevent Overlap" },
	{ &fs_names[198], 0, 0, "Allow Overlap" },
	{ &fs_names[199], 0, 0, "Typographic Extras" },
	{ &fs_names[200], 0, 0, "Hyphens to Em-dash" },
	{ &fs_names[201], 0, 0, "Hyphen to En-dash" },
	{ &fs_names[202], 0, 0, "Unslashed Zero" },
	{ &fs_names[203], 0, 0, "Form Interrobang" },
	{ &fs_names[204], 0, 0, "Smart Quotes" },
	{ &fs_names[205], 0, 0, "Periods to Ellipsis" },
	{ &fs_names[206], 0, 0, "Mathematical Extras" },
	{ &fs_names[207], 0, 0, "Hyphen to Minus" },
	{ &fs_names[208], 0, 0, "Asterisk to Multiply" },
	{ &fs_names[209], 0, 0, "Slash to Divide" },
	{ &fs_names[210], 0, 0, "Inequality Ligatures" },
	{ &fs_names[211], 0, 0, "Exponents" },
	{ &fs_names[212], 0, 0, "Ornament Sets" },
	{ &fs_names[213], 0, 0, "No Ornaments" },
	{ &fs_names[214], 0, 0, "Dingbats" },
	{ &fs_names[215], 0, 0, "Pi Characters" },
	{ &fs_names[216], 0, 0, "Fleurons" },
	{ &fs_names[217], 0, 0, "Decorative Borders" },
	{ &fs_names[218], 0, 0, "International Symbols" },
	{ &fs_names[219], 0, 0, "Math Symbols" },
	{ &fs_names[220], 0, 0, "Character Alternates" },
	{ &fs_names[221], 0, 0, "No Alternates" },
	{ &fs_names[222], 0, 0, "Alternate Characters" },
	{ &fs_names[223], 0, 0, "Other Alternates" },
	{ &fs_names[224], 0, 0, "Design Complexity" },
	{ &fs_names[225], 0, 0, "Design Level 1" },
	{ &fs_names[226], 0, 0, "Design Level 2" },
	{ &fs_names[227], 0, 0, "Design Level 3" },
	{ &fs_names[228], 0, 0, "Design Level 4" },
	{ &fs_names[229], 0, 0, "Design Level 5" },
	{ &fs_names[230], 0, 0, "Style Options" },
	{ &fs_names[231], 0, 0, "No Style Options" },
	{ &fs_names[232], 0, 0, "Display Text" },
	{ &fs_names[233], 0, 0, "Engraved Text" },
	{ &fs_names[234], 0, 0, "Illuminated Caps" },
	{ &fs_names[235], 0, 0, "Titling Caps" },
	{ &fs_names[236], 0, 0, "Tall Caps" },
	{ &fs_names[237], 0, 0, "Character Shape" },
	{ &fs_names[238], 0, 0, "Traditional" },
	{ &fs_names[239], 0, 0, "Simplified" },
	{ &fs_names[240], 0, 0, "jis 1978" },
	{ &fs_names[241], 0, 0, "jis 1983" },
	{ &fs_names[242], 0, 0, "jis 1990" },
	{ &fs_names[243], 0, 0, "Traditional Alt 1" },
	{ &fs_names[244], 0, 0, "Traditional Alt 2" },
	{ &fs_names[245], 0, 0, "Traditional Alt 3" },
	{ &fs_names[246], 0, 0, "Traditional Alt 4" },
	{ &fs_names[247], 0, 0, "Traditional Alt 5" },
	{ &fs_names[248], 0, 0, "Expert" },
	{ &fs_names[249], 0, 0, "Number Case" },
	{ &fs_names[250], 0, 0, "Lower Case Numbers" },
	{ &fs_names[251], 0, 0, "Upper Case Numbers" },
	{ &fs_names[252], 0, 0, "Text Spacing" },
	{ &fs_names[253], 0, 0, "Proportional" },
	{ &fs_names[254], 0, 0, "Monospace" },
	{ &fs_names[255], 0, 0, "Transliteration" },
	{ &fs_names[256], 0, 0, "No Transliteration" },
	{ &fs_names[257], 0, 0, "Hanja To Hangul" },
	{ &fs_names[258], 0, 0, "Hiragana to Katakana" },
	{ &fs_names[259], 0, 0, "Katakana to Hiragana" },
	{ &fs_names[260], 0, 0, "Katakana to Roman" },
	{ &fs_names[261], 0, 0, "Roman to Hiragana" },
	{ &fs_names[262], 0, 0, "Roman to Katakana" },
	{ &fs_names[263], 0, 0, "Hanja To Hangul Alt 1" },
	{ &fs_names[264], 0, 0, "Hanja To Hangul Alt 2" },
	{ &fs_names[265], 0, 0, "Hanja To Hangul Alt 3" },
	{ &fs_names[266], 0, 0, "Annotation" },
	{ &fs_names[267], 0, 0, "No Annotation" },
	{ &fs_names[268], 0, 0, "Box Annotation" },
	{ &fs_names[269], 0, 0, "Rounded Box Annotation" },
	{ &fs_names[270], 0, 0, "Circle Annotation" },
	{ &fs_names[271], 0, 0, "Inverted Circle Annotation" },
	{ &fs_names[272], 0, 0, "Parenthesized Annotation" },
	{ &fs_names[273], 0, 0, "Period Annotation" },
	{ &fs_names[274], 0, 0, "Roman Numeral Annotation" },
	{ &fs_names[275], 0, 0, "Diamond Annotation" },
	{ &fs_names[276], 0, 0, "Kana Spacing" },
	{ &fs_names[277], 0, 0, "Full-Width" },
	{ &fs_names[278], 0, 0, "Proportional" },
	{ &fs_names[136], 0, 0, "Ideographic Spacing" },
	{ &fs_names[137], 0, 0, "Full-Width" },
	{ &fs_names[138], 0, 0, "Proportional" },
	{ &fs_names[279], 0, 0, "Ideographic Spacing" },
	{ &fs_names[280], 0, 0, "Full-Width" },
	{ &fs_names[281], 0, 0, "Proportional" },
	{ &fs_names[282], 0, 0, "CJK Roman Spacing" },
	{ &fs_names[283], 0, 0, "Half-Width" },
	{ &fs_names[284], 0, 0, "Proportional" },
	{ &fs_names[285], 0, 0, "Default" },
	{ &fs_names[286], 0, 0, "Full-Width" },
	{ &fs_names[287], 0, 0, "Unicode Decomposition" },
	{ &fs_names[288], 0, 0, "Canonical Decomposition" },
	{ &fs_names[289], 0, 1, "Fonctions typographiques" },
	{ &fs_names[290], 0, 1, "Toutes fonctions typographiques" },
	{ &fs_names[291], 0, 1, "Ligatures" },
	{ &fs_names[397], 0, 1, "Ligatures Requises" },
	{ &fs_names[292], 0, 1, "Ligatures Usuelles" },
	{ &fs_names[293], 0, 1, "Ligatures Rares" },
	{ &fs_names[400], 0, 1, "Ligatures Logos" },
	{ &fs_names[401], 0, 1, "Ligatures R\216bus" },
	{ &fs_names[334], 0, 1, "Ligatures Diphtongues" },
	{ &fs_names[403], 0, 1, "Ligatures Carr\216es" },
	{ &fs_names[404], 0, 1, "Ligatures Carr\216es Abr\217g\216es" },
	{ &fs_names[405], 0, 1, "Connection des Cursives" },
	{ &fs_names[406], 0, 1, "Non connect\216es" },
	{ &fs_names[407], 0, 1, "Partiellement connect\216es" },
	{ &fs_names[408], 0, 1, "Pleinement connect\216es" },
	{ &fs_names[409], 0, 1, "Casse" },
	{ &fs_names[295], 0, 1, "Majuscules & Minuscules" },
	{ &fs_names[296], 0, 1, "Tout Majuscule" },
	{ &fs_names[412], 0, 1, "Tout Minuscule" },
	{ &fs_names[297], 0, 1, "Petites Majuscules" },
	{ &fs_names[414], 0, 1, "Initiales Majuscules" },
	{ &fs_names[415], 0, 1, "Initiales + Petites Majuscules" },
	{ &fs_names[416], 0, 1, "Substitution Verticale" },
	{ &fs_names[417], 0, 1, "Substitution vertical" },
	{ &fs_names[418], 0, 1, "Aucun Substitution vertical" },
	{ &fs_names[419], 0, 1, "R\216arrangement Linguistique" },
	{ &fs_names[420], 0, 1, "Avec R\216arrangement Linguistique" },
	{ &fs_names[421], 0, 1, "Pas de R\216arrangement Linguistique" },
	{ &fs_names[422], 0, 1, "Espacement des chiffres" },
	{ &fs_names[299], 0, 1, "Chiffres de largeur fixe" },
	{ &fs_names[300], 0, 1, "Chiffres Proportionnels" },
	{ &fs_names[301], 0, 1, "Parafes" },
	{ &fs_names[304], 0, 1, "Parafes en d\216but de mot" },
	{ &fs_names[305], 0, 1, "Parafes en fin de mot" },
	{ &fs_names[303], 0, 1, "Parafes en d\216but de ligne" },
	{ &fs_names[302], 0, 1, "Parafes en fin de ligne" },
	{ &fs_names[306], 0, 1, "Autres Parafes" },
	{ &fs_names[431], 0, 1, "Signes Diacritiques" },
	{ &fs_names[339], 0, 1, "Montrer les Signes Diacritiques" },
	{ &fs_names[433], 0, 1, "Cacher les Signes Diacritiques" },
	{ &fs_names[337], 0, 1, "D\216composer les Signes Diacritiques" },
	{ &fs_names[435], 0, 1, "Position Verticale" },
	{ &fs_names[309], 0, 1, "Position Verticale Normale" },
	{ &fs_names[308], 0, 1, "Position Sup\216rieure" },
	{ &fs_names[310], 0, 1, "Position Inf\216rieure" },
	{ &fs_names[311], 0, 1, "Position Sup\216rieure Contextuelle (Num\216rique)" },
	{ &fs_names[440], 0, 1, "Fractions" },
	{ &fs_names[313], 0, 1, "Pas de Fractions" },
	{ &fs_names[442], 0, 1, "Fractions Verticales" },
	{ &fs_names[314], 0, 1, "Fractions en Diagonale" },
	{ &fs_names[444], 0, 1, "Chevauchement des caract\217res" },
	{ &fs_names[316], 0, 1, "\203viter le chevauchement" },
	{ &fs_names[446], 0, 1, "Laisser le Chevauchement" },
	{ &fs_names[317], 0, 1, "Extras Typographiques" },
	{ &fs_names[448], 0, 1, "Tirets vers Tiret Large" },
	{ &fs_names[449], 0, 1, "Tiret vers Tiret Moyen" },
	{ &fs_names[450], 0, 1, "Z\216ro non Barr\216" },
	{ &fs_names[451], 0, 1, "?! vers InterroExclam" },
	{ &fs_names[336], 0, 1, "Apostrophes Intelligentes" },
	{ &fs_names[453], 0, 1, "... vers Ellipse" },
	{ &fs_names[318], 0, 1, "Extras Math\216matiques" },
	{ &fs_names[319], 0, 1, "Tiret vers Moins" },
	{ &fs_names[320], 0, 1, "\203toile vers Multipli\216" },
	{ &fs_names[457], 0, 1, "Barre pench\216e vers Divis\216" },
	{ &fs_names[458], 0, 1, "Ligatures pour In\216galit\216s" },
	{ &fs_names[459], 0, 1, "Passage en Exposant" },
	{ &fs_names[460], 0, 1, "Ensembles Ornementaux" },
	{ &fs_names[322], 0, 1, "Pas d'Ornements" },
	{ &fs_names[462], 0, 1, "Dingbats" },
	{ &fs_names[463], 0, 1, "Symboles Sp\216cifiques \210 un Domaine" },
	{ &fs_names[323], 0, 1, "Fleurons" },
	{ &fs_names[465], 0, 1, "Bordures D\216coratives" },
	{ &fs_names[466], 0, 1, "Symboles Internationaux" },
	{ &fs_names[467], 0, 1, "Symboles Math\216matiques" },
	{ &fs_names[468], 0, 1, "Caract\217res Alternatifs" },
	{ &fs_names[325], 0, 1, "Sans Caract\217res Alternatifs" },
	{ &fs_names[470], 0, 1, "Avec Caract\217res Alternatifs" },
	{ &fs_names[471], 0, 1, "Autres Caract\217res Alternatifs" },
	{ &fs_names[472], 0, 1, "Complexit\216 du Dessin" },
	{ &fs_names[327], 0, 1, "Dessin Niveau 1" },
	{ &fs_names[328], 0, 1, "Dessin Niveau 2" },
	{ &fs_names[329], 0, 1, "Dessin Niveau 3" },
	{ &fs_names[330], 0, 1, "Dessin Niveau 4" },
	{ &fs_names[477], 0, 1, "Dessin Niveau 5" },
	{ &fs_names[478], 0, 1, "Options de Style" },
	{ &fs_names[479], 0, 1, "Texte Ordinaire" },
	{ &fs_names[480], 0, 1, "Texte Majeur" },
	{ &fs_names[481], 0, 1, "Texte en Relief" },
	{ &fs_names[482], 0, 1, "Majuscules Enlumin\216es" },
	{ &fs_names[483], 0, 1, "Majuscules de Titrage" },
	{ &fs_names[484], 0, 1, "Majuscules avec Descendantes" },
	{ &fs_names[485], 0, 1, "Forme des Caract\217res" },
	{ &fs_names[486], 0, 1, "Traditionelle" },
	{ &fs_names[487], 0, 1, "Simplifi\216e" },
	{ &fs_names[488], 0, 1, "jis 1978" },
	{ &fs_names[489], 0, 1, "jis 1983" },
	{ &fs_names[490], 0, 1, "jis 1990" },
	{ &fs_names[491], 0, 1, "Traditionelle Alt 1" },
	{ &fs_names[492], 0, 1, "Traditionelle Alt 2" },
	{ &fs_names[493], 0, 1, "Traditionelle Alt 3" },
	{ &fs_names[494], 0, 1, "Traditionelle Alt 4" },
	{ &fs_names[495], 0, 1, "Traditionelle Alt 5" },
	{ &fs_names[496], 0, 1, "Expert" },
	{ &fs_names[497], 0, 1, "Style des Chiffres" },
	{ &fs_names[332], 0, 1, "Chiffres Anciens (bas de casse)" },
	{ &fs_names[333], 0, 1, "Chiffres Conventionnels (alignants)" },
	{ &fs_names[500], 0, 1, "Espacement du Texte" },
	{ &fs_names[501], 0, 1, "Proportionel" },
	{ &fs_names[502], 0, 1, "Fixe" },
	{ &fs_names[503], 0, 1, "Translitt\216ration" },
	{ &fs_names[504], 0, 1, "Sans Translitt\216ration" },
	{ &fs_names[505], 0, 1, "Hanja vers Hangul" },
	{ &fs_names[506], 0, 1, "Hiragana vers Katakana" },
	{ &fs_names[507], 0, 1, "Katakana vers Hiragana" },
	{ &fs_names[508], 0, 1, "Katakana vers Roman" },
	{ &fs_names[509], 0, 1, "Roman vers Hiragana" },
	{ &fs_names[510], 0, 1, "Roman vers Katakana" },
	{ &fs_names[511], 0, 1, "Hanja vers Hangul Alt 1" },
	{ &fs_names[512], 0, 1, "Hanja vers Hangul Alt 2" },
	{ &fs_names[513], 0, 1, "Hanja vers Hangul Alt 3" },
	{ &fs_names[514], 0, 1, "Annotations" },
	{ &fs_names[515], 0, 1, "Sans Annotations" },
	{ &fs_names[516], 0, 1, "Annotations Encadr\216es" },
	{ &fs_names[517], 0, 1, "Annotations en Cadres arrondis" },
	{ &fs_names[518], 0, 1, "Annotations dans des Cercles" },
	{ &fs_names[519], 0, 1, "Annotations dans des Cercles inverses" },
	{ &fs_names[520], 0, 1, "Annotations Parenth\217s\216es" },
	{ &fs_names[521], 0, 1, "Annotations avec des ." },
	{ &fs_names[522], 0, 1, "Annotations en Chiffres Romains" },
	{ &fs_names[523], 0, 1, "Annotations Diamant" },
	{ &fs_names[524], 0, 1, "Espacement Kana" },
	{ &fs_names[525], 0, 1, "Pleine Taille" },
	{ &fs_names[526], 0, 1, "Proportionnel" },
	{ &fs_names[527], 0, 1, "Espacement des Id\216ogrammes" },
	{ &fs_names[528], 0, 1, "Pleine Taille" },
	{ &fs_names[529], 0, 1, "Proportionnel" },
	{ &fs_names[533], 0, 1, "Espacement des CJK romains" },
	{ &fs_names[534], 0, 1, "Pleine Taille" },
	{ &fs_names[535], 0, 1, "Proportionnel" },
	{ &fs_names[536], 0, 1, "Romains par D\216faut" },
	{ &fs_names[537], 0, 1, "Romains Pleine Taille" },
	{ &fs_names[340], 0, 1, "D\216composition Unicode" },
	{ &fs_names[341], 0, 1, "D\216composition Canonique" },
	{ &fs_names[342], 0, 2, "Alle typografischen M\232glichkeiten" },
	{ &fs_names[343], 0, 2, "Alle Auszeichnungsarten" },
	{ &fs_names[344], 0, 2, "Ligaturen" },
	{ &fs_names[346], 0, 2, "Normale Ligaturen" },
	{ &fs_names[345], 0, 2, "Seltene Ligaturen" },
	{ &fs_names[347], 0, 2, "Schreibweise" },
	{ &fs_names[348], 0, 2, "Gro\247/Klein" },
	{ &fs_names[349], 0, 2, "Gro\247" },
	{ &fs_names[350], 0, 2, "Kapit\212lchen" },
	{ &fs_names[351], 0, 2, "Ziffernabst\212nde" },
	{ &fs_names[352], 0, 2, "Tabellenziffern" },
	{ &fs_names[353], 0, 2, "Proportionalziffern" },
	{ &fs_names[354], 0, 2, "Zierbuchstabe" },
	{ &fs_names[355], 0, 2, "Zierbuchstabe Zeilenende" },
	{ &fs_names[356], 0, 2, "Zierbuchstabe Zeilenanfang" },
	{ &fs_names[357], 0, 2, "Zierbuchstabe Wortanfang" },
	{ &fs_names[358], 0, 2, "Zierbuchstabe Wortende" },
	{ &fs_names[359], 0, 2, "Zierbuchstabe Beliebig" },
	{ &fs_names[360], 0, 2, "Hoch-/Tiefstellen" },
	{ &fs_names[361], 0, 2, "Hochgestellt" },
	{ &fs_names[362], 0, 2, "Normal" },
	{ &fs_names[363], 0, 2, "Tiefgestellt" },
	{ &fs_names[364], 0, 2, "Ordnungszahlen" },
	{ &fs_names[365], 0, 2, "Br\237che" },
	{ &fs_names[367], 0, 2, "Kein Bruche" },
	{ &fs_names[366], 0, 2, "Diagonaler Bruch" },
	{ &fs_names[368], 0, 2, "\206berlappen" },
	{ &fs_names[369], 0, 2, "\206berlappen  vermeiden" },
	{ &fs_names[335], 0, 2, "Typographische Extras" },
	{ &fs_names[370], 0, 2, "Mathematische Sonderzeichen" },
	{ &fs_names[371], 0, 2, "Minuszeichen" },
	{ &fs_names[372], 0, 2, "Malzeichen" },
	{ &fs_names[373], 0, 2, "Sonderzeichen" },
	{ &fs_names[374], 0, 2, "Keine Sonderzeichen" },
	{ &fs_names[375], 0, 2, "Pflanzenornamente" },
	{ &fs_names[376], 0, 2, "Alternative Zeichen" },
	{ &fs_names[377], 0, 2, "Keine Alternativ-Figuren" },
	{ &fs_names[378], 0, 2, "Modifikationsgrad" },
	{ &fs_names[379], 0, 2, "Design Stufe 1" },
	{ &fs_names[380], 0, 2, "Design Stufe 2" },
	{ &fs_names[381], 0, 2, "Design Stufe 3" },
	{ &fs_names[382], 0, 2, "Design Stufe 4" },
	{ &fs_names[383], 0, 2, "Zahlendarstellung" },
	{ &fs_names[384], 0, 2, "Medi\276val-Ziffern" },
	{ &fs_names[385], 0, 2, "Normale Ziffern" },
	{ &fs_names[386], 0, 2, "Diphtong Ligaturen" },
	{ &fs_names[387], 0, 2, "Typografische Extras" },
	{ &fs_names[388], 0, 2, "Ersetzen mit geschwungenen Anf\237hrungszeichen" },
	{ &fs_names[389], 0, 2, "Keine Ver\212nderung" },
	{ &fs_names[390], 0, 2, "Diakritische Zeichen" },
	{ &fs_names[391], 0, 2, "Diakritische Zeichen zeigen" },
	{ &fs_names[392], 0, 2, "In Unicode zerlegen" },
	{ &fs_names[393], 0, 2, "anerkannte Komposition" },
	{ &fs_names[394], 0, 3, "Funzioni Tipografiche" },
	{ &fs_names[395], 0, 3, "Tutte le Funzioni" },
	{ &fs_names[396], 0, 3, "Legature" },
	{ &fs_names[399], 0, 3, "Legature Rare" },
	{ &fs_names[398], 0, 3, "Legature pi\235 Comuni" },
	{ NULL, 0, 3, "Maiuscolo o Minuscolo" },
	{ &fs_names[410], 0, 3, "Maiuscolo & minuscolo" },
	{ &fs_names[411], 0, 3, "Tutto in Maiuscolo" },
	{ &fs_names[413], 0, 3, "Maiuscoletto" },
	{ NULL, 0, 3, "Spaziatura numeri" },
	{ &fs_names[423], 0, 3, "Monospaziata" },
	{ &fs_names[424], 0, 3, "Proporzionale" },
	{ &fs_names[425], 0, 3, "Lettere Ornate" },
	{ &fs_names[429], 0, 3, "Fine Riga" },
	{ &fs_names[428], 0, 3, "Inizio Riga" },
	{ &fs_names[426], 0, 3, "All'inizio" },
	{ &fs_names[427], 0, 3, "Alla Fine" },
	{ &fs_names[430], 0, 3, "All'interno" },
	{ NULL, 0, 3, "Posizione Verticale" },
	{ &fs_names[437], 0, 3, "Apice" },
	{ &fs_names[436], 0, 3, "Posizione Normale" },
	{ &fs_names[438], 0, 3, "Pedice" },
	{ &fs_names[439], 0, 3, "Ordinali" },
	{ NULL, 0, 3, "Frazioni" },
	{ &fs_names[443], 0, 3, "Frazioni Diagonali" },
	{ &fs_names[441], 0, 3, "Nessuna Frazione" },
	{ NULL, 0, 3, "Caratteri Sovrapposti" },
	{ &fs_names[445], 0, 3, "Nessuna Sovrapposizione" },
	{ &fs_names[454], 0, 3, "Conversioni Matematiche" },
	{ &fs_names[455], 0, 3, "Trattino per Sottrazione" },
	{ &fs_names[456], 0, 3, "Asterisco per Moltiplicazione" },
	{ NULL, 0, 3, "Impostazione Ornamenti" },
	{ &fs_names[461], 0, 3, "Nessun Ornamento" },
	{ &fs_names[464], 0, 3, "Fleurons" },
	{ NULL, 0, 3, "Caratteri Alternativi" },
	{ &fs_names[469], 0, 3, "Nessuna alternativa" },
	{ NULL, 0, 3, "Design Complexity" },
	{ &fs_names[473], 0, 3, "Livello 1" },
	{ &fs_names[474], 0, 3, "Livello 2" },
	{ &fs_names[475], 0, 3, "Livello 3" },
	{ &fs_names[476], 0, 3, "Livello 4" },
	{ NULL, 0, 3, "Posizione Numeri" },
	{ &fs_names[498], 0, 3, "Sopra la Linea Base" },
	{ &fs_names[499], 0, 3, "Tradizionale" },
	{ &fs_names[402], 0, 3, "Legature dittonghi" },
	{ &fs_names[447], 0, 3, "Extra tipografici" },
	{ &fs_names[452], 0, 3, "Virgolette eleganti" },
	{ &fs_names[434], 0, 3, "Nessuna modifica" },
	{ NULL, 0, 3, "Diacritici" },
	{ &fs_names[432], 0, 3, "Mostra diacritici" },
	{ &fs_names[538], 0, 3, "Scomposizione unicode" },
	{ &fs_names[539], 0, 3, "Composizione canonica" },
	{ NULL, 0, 4, "Alle typografische kenmerken" },
	{ NULL, 0, 4, "Alle typekenmerken" },
	{ NULL, 0, 4, "Ligaturen" },
	{ NULL, 0, 4, "Vereiste ligaturen" },
	{ NULL, 0, 4, "Gemeenschappelijke Ligaturen" },
	{ NULL, 0, 4, "Zeldzame ligaturen" },
	{ NULL, 0, 4, "Logoligaturen" },
	{ NULL, 0, 4, "Rebusligaturen" },
	{ NULL, 0, 4, "Tweeklankligaturen" },
	{ NULL, 0, 4, "Vierkante ligaturen" },
	{ NULL, 0, 4, "Afgekorte vierkante ligatures" },
	{ NULL, 0, 4, "Cursieve verbinding" },
	{ NULL, 0, 4, "Niet verbonden" },
	{ NULL, 0, 4, "Gedeeltelijk verbonden" },
	{ NULL, 0, 4, "Cursief" },
	{ NULL, 0, 4, "Hoofd/kleine letters" },
	{ NULL, 0, 4, "Hoofd- en kleine letters" },
	{ NULL, 0, 4, "Alles in hoofdletters" },
	{ NULL, 0, 4, "Alles in kleine letters" },
	{ NULL, 0, 4, "Kleine hoofdletters" },
	{ NULL, 0, 4, "Eerste hoofdletters" },
	{ NULL, 0, 4, "Eerste en kleine hoofdletters" },
	{ NULL, 0, 4, "Verticale vervanging" },
	{ NULL, 0, 4, "Verticale vervanging" },
	{ NULL, 0, 4, "Geen verticale vervanging" },
	{ NULL, 0, 4, "Taalkundige herschikking" },
	{ NULL, 0, 4, "Taalkundige herschikking" },
	{ NULL, 0, 4, "Geen taalkundige herschikking" },
	{ NULL, 0, 4, "Nummerafstanden" },
	{ NULL, 0, 4, "Vaste nummerafstanden" },
	{ NULL, 0, 4, "Proportionele nummers" },
	{ NULL, 0, 4, "Slimme versieringingen" },
	{ NULL, 0, 4, "Woordbegin-versieringingen" },
	{ NULL, 0, 4, "Woordeinde-versieringingen" },
	{ NULL, 0, 4, "Regelbegin-versieringingen" },
	{ NULL, 0, 4, "Regeleinde-versieringingen" },
	{ NULL, 0, 4, "Niet-einde-versieringingen" },
	{ NULL, 0, 4, "Accenten" },
	{ NULL, 0, 4, "Accenten tonen" },
	{ NULL, 0, 4, "Accenten verbergen" },
	{ NULL, 0, 4, "Accenten ontleden" },
	{ NULL, 0, 4, "Verticale positie" },
	{ NULL, 0, 4, "Normale verticale positie" },
	{ NULL, 0, 4, "Superieuren" },
	{ NULL, 0, 4, "Inferieuren" },
	{ NULL, 0, 4, "Ordinalen" },
	{ NULL, 0, 4, "Breuken" },
	{ NULL, 0, 4, "Geen breuken" },
	{ NULL, 0, 4, "Verticale breuken" },
	{ NULL, 0, 4, "Diagonale breuken" },
	{ NULL, 0, 4, "Overlappende tekens" },
	{ NULL, 0, 4, "Overlap voorkomen" },
	{ NULL, 0, 4, "Overlap toestaan" },
	{ NULL, 0, 4, "Typografische extras" },
	{ NULL, 0, 4, "Koppelteken naar em-streep" },
	{ NULL, 0, 4, "Koppelteken naar en-streepje" },
	{ NULL, 0, 4, "Nul zonder schuine streep" },
	{ NULL, 0, 4, "Vorm interrobang" },
	{ NULL, 0, 4, "Slimme aanhalingstekens" },
	{ NULL, 0, 4, "Punten naar ellipsen" },
	{ NULL, 0, 4, "Wiskundige extras" },
	{ NULL, 0, 4, "Koppelteken naar minteken" },
	{ NULL, 0, 4, "Sterretje naar multiplicatieteken" },
	{ NULL, 0, 4, "Schuine streep naar deelteken" },
	{ NULL, 0, 4, "Ongelijkheidsligaturen" },
	{ NULL, 0, 4, "Exponenten aan" },
	{ NULL, 0, 4, "Ornamentenverzamelingen" },
	{ NULL, 0, 4, "Geen ornamenten" },
	{ NULL, 0, 4, "Dingbats" },
	{ NULL, 0, 4, "Pi-tekens" },
	{ NULL, 0, 4, "Fleurons" },
	{ NULL, 0, 4, "Decoratieve randen" },
	{ NULL, 0, 4, "Internationale symbolen" },
	{ NULL, 0, 4, "Wiskundige Symbolen" },
	{ NULL, 0, 4, "Tekenalternatieven" },
	{ NULL, 0, 4, "Geen alternatieven" },
	{ NULL, 0, 4, "Alternatieve tekens" },
	{ NULL, 0, 4, "Andere alternatieven" },
	{ NULL, 0, 4, "Ontwepcomplexiteit" },
	{ NULL, 0, 4, "Ontwerpniveau 1" },
	{ NULL, 0, 4, "Ontwerpniveau 2" },
	{ NULL, 0, 4, "Ontwerpniveau 3" },
	{ NULL, 0, 4, "Ontwerpniveau 4" },
	{ NULL, 0, 4, "Ontwerpniveau 5" },
	{ NULL, 0, 4, "Stijlopties" },
	{ NULL, 0, 4, "Geen stijl ptions" },
	{ NULL, 0, 4, "Tekst tonen" },
	{ NULL, 0, 4, "Gegraveerde tekst" },
	{ NULL, 0, 4, "Uitgelichte koppen" },
	{ NULL, 0, 4, "Titelkoppen" },
	{ NULL, 0, 4, "Eindkoppen" },
	{ NULL, 0, 4, "Tekenvorm" },
	{ NULL, 0, 4, "Traditioneel" },
	{ NULL, 0, 4, "Vereenvoudigd" },
	{ NULL, 0, 4, "jis 1978" },
	{ NULL, 0, 4, "jis 1983" },
	{ NULL, 0, 4, "jis 1990" },
	{ NULL, 0, 4, "Traditioneel Alt 1" },
	{ NULL, 0, 4, "Traditioneel Alt 2" },
	{ NULL, 0, 4, "Traditioneel Alt 3" },
	{ NULL, 0, 4, "Traditioneel Alt 4" },
	{ NULL, 0, 4, "Traditioneel Alt 5" },
	{ NULL, 0, 4, "Expert" },
	{ NULL, 0, 4, "Nummerhoogte" },
	{ NULL, 0, 4, "Kleine nummers" },
	{ NULL, 0, 4, "Grote nummers" },
	{ NULL, 0, 4, "TekstspatiQring" },
	{ NULL, 0, 4, "Proportioneel" },
	{ NULL, 0, 4, "Gelijk gespatieerd" },
	{ NULL, 0, 4, "Transliteratie" },
	{ NULL, 0, 4, "Geen transliteratie" },
	{ NULL, 0, 4, "Hanja naar Hangul" },
	{ NULL, 0, 4, "Hiragana naar Katakana" },
	{ NULL, 0, 4, "Katakana naar Hiragana" },
	{ NULL, 0, 4, "Katakana naar Romeins" },
	{ NULL, 0, 4, "Romeins naar Hiragana" },
	{ NULL, 0, 4, "Romeins naar Katakana" },
	{ NULL, 0, 4, "Hanja naar Hangul Alt 1" },
	{ NULL, 0, 4, "Hanja naar Hangul Alt 2" },
	{ NULL, 0, 4, "Hanja naar Hangul Alt 3" },
	{ NULL, 0, 4, "Annotatie" },
	{ NULL, 0, 4, "Geen annotatie" },
	{ NULL, 0, 4, "Vierkantannotatie" },
	{ NULL, 0, 4, "Ronde-vierkantannotatie" },
	{ NULL, 0, 4, "Cirkelannotatie" },
	{ NULL, 0, 4, "Omgekeerde cirkelannotatie" },
	{ NULL, 0, 4, "Aanhalingstekenannotatie" },
	{ NULL, 0, 4, "Puntannotatie" },
	{ NULL, 0, 4, "Romeinse-cijferannotatie" },
	{ NULL, 0, 4, "Diamantannotatie" },
	{ NULL, 0, 4, "Kana spatiQring" },
	{ NULL, 0, 4, "Volledige breedte" },
	{ NULL, 0, 4, "Proportioneel" },
	{ &fs_names[530], 0, 4, "Ideographische spatiQring" },
	{ &fs_names[531], 0, 4, "Volledige breedte" },
	{ &fs_names[532], 0, 4, "Proportioneel" },
	{ NULL, 0, 4, "IdeograafspatiQring" },
	{ NULL, 0, 4, "Volledige breedte" },
	{ NULL, 0, 4, "Proportioneel" },
	{ NULL, 0, 4, "CJK Romeinse spatiQring" },
	{ NULL, 0, 4, "Halve breedte" },
	{ NULL, 0, 4, "Proportioneel" },
	{ NULL, 0, 4, "Default" },
	{ NULL, 0, 4, "Volledige breedte" },
	{ NULL, 0, 4, "Unicodeontleding" },
	{ NULL, 0, 4, "Canonieke ontleding" },
	{ NULL, 0, 0, NULL }
};

static struct macsetting fs_settings[] = {
	{ NULL, 0, 0, &fs_names[1], 0 },
	{ NULL, 14, 0, &fs_names[10], 0 },
	{ &fs_settings[1], 12, 0, &fs_names[9], 0 },
	{ &fs_settings[2], 10, 0, &fs_names[8], 0 },
	{ &fs_settings[3], 8, 0, &fs_names[7], 0 },
	{ &fs_settings[4], 6, 0, &fs_names[6], 0 },
	{ &fs_settings[5], 4, 0, &fs_names[5], 0 },
	{ &fs_settings[6], 2, 0, &fs_names[4], 1 },
	{ &fs_settings[7], 0, 0, &fs_names[3], 1 },
	{ NULL, 2, 0, &fs_names[14], 0 },
	{ &fs_settings[9], 1, 0, &fs_names[13], 0 },
	{ &fs_settings[10], 0, 0, &fs_names[12], 1 },
	{ NULL, 5, 0, &fs_names[21], 0 },
	{ &fs_settings[12], 4, 0, &fs_names[20], 0 },
	{ &fs_settings[13], 3, 0, &fs_names[19], 0 },
	{ &fs_settings[14], 2, 0, &fs_names[18], 0 },
	{ &fs_settings[15], 1, 0, &fs_names[17], 0 },
	{ &fs_settings[16], 0, 0, &fs_names[16], 1 },
	{ NULL, 1, 0, &fs_names[24], 0 },
	{ &fs_settings[18], 0, 0, &fs_names[23], 1 },
	{ NULL, 1, 0, &fs_names[27], 0 },
	{ &fs_settings[20], 0, 0, &fs_names[26], 1 },
	{ NULL, 1, 0, &fs_names[30], 0 },
	{ &fs_settings[22], 0, 0, &fs_names[29], 1 },
	{ NULL, 8, 0, &fs_names[36], 0 },
	{ &fs_settings[24], 6, 0, &fs_names[35], 0 },
	{ &fs_settings[25], 4, 0, &fs_names[34], 0 },
	{ &fs_settings[26], 2, 0, &fs_names[33], 0 },
	{ &fs_settings[27], 0, 0, &fs_names[32], 0 },
	{ NULL, 2, 0, &fs_names[40], 0 },
	{ &fs_settings[29], 1, 0, &fs_names[39], 0 },
	{ &fs_settings[30], 0, 0, &fs_names[38], 1 },
	{ NULL, 3, 0, &fs_names[45], 0 },
	{ &fs_settings[32], 2, 0, &fs_names[44], 0 },
	{ &fs_settings[33], 1, 0, &fs_names[43], 0 },
	{ &fs_settings[34], 0, 0, &fs_names[42], 1 },
	{ NULL, 2, 0, &fs_names[49], 0 },
	{ &fs_settings[36], 1, 0, &fs_names[48], 0 },
	{ &fs_settings[37], 0, 0, &fs_names[47], 1 },
	{ NULL, 1, 0, &fs_names[52], 0 },
	{ &fs_settings[39], 0, 0, &fs_names[51], 1 },
	{ NULL, 10, 0, &fs_names[59], 0 },
	{ &fs_settings[41], 8, 0, &fs_names[58], 0 },
	{ &fs_settings[42], 6, 0, &fs_names[57], 0 },
	{ &fs_settings[43], 4, 0, &fs_names[56], 0 },
	{ &fs_settings[44], 2, 0, &fs_names[55], 0 },
	{ &fs_settings[45], 0, 0, &fs_names[54], 0 },
	{ NULL, 8, 0, &fs_names[65], 0 },
	{ &fs_settings[47], 6, 0, &fs_names[64], 0 },
	{ &fs_settings[48], 4, 0, &fs_names[63], 0 },
	{ &fs_settings[49], 2, 0, &fs_names[62], 0 },
	{ &fs_settings[50], 0, 0, &fs_names[61], 0 },
	{ NULL, 6, 0, &fs_names[73], 0 },
	{ &fs_settings[52], 5, 0, &fs_names[72], 0 },
	{ &fs_settings[53], 4, 0, &fs_names[71], 0 },
	{ &fs_settings[54], 3, 0, &fs_names[70], 0 },
	{ &fs_settings[55], 2, 0, &fs_names[69], 0 },
	{ &fs_settings[56], 1, 0, &fs_names[68], 0 },
	{ &fs_settings[57], 0, 0, &fs_names[67], 1 },
	{ NULL, 2, 0, &fs_names[77], 0 },
	{ &fs_settings[59], 1, 0, &fs_names[76], 0 },
	{ &fs_settings[60], 0, 0, &fs_names[75], 1 },
	{ NULL, 4, 0, &fs_names[83], 0 },
	{ &fs_settings[62], 3, 0, &fs_names[82], 0 },
	{ &fs_settings[63], 2, 0, &fs_names[81], 0 },
	{ &fs_settings[64], 1, 0, &fs_names[80], 0 },
	{ &fs_settings[65], 0, 0, &fs_names[79], 1 },
	{ NULL, 5, 0, &fs_names[90], 0 },
	{ &fs_settings[67], 4, 0, &fs_names[89], 0 },
	{ &fs_settings[68], 3, 0, &fs_names[88], 0 },
	{ &fs_settings[69], 2, 0, &fs_names[87], 0 },
	{ &fs_settings[70], 1, 0, &fs_names[86], 0 },
	{ &fs_settings[71], 0, 0, &fs_names[85], 1 },
	{ NULL, 10, 0, &fs_names[102], 0 },
	{ &fs_settings[73], 9, 0, &fs_names[101], 0 },
	{ &fs_settings[74], 8, 0, &fs_names[100], 0 },
	{ &fs_settings[75], 7, 0, &fs_names[99], 0 },
	{ &fs_settings[76], 6, 0, &fs_names[98], 0 },
	{ &fs_settings[77], 5, 0, &fs_names[97], 0 },
	{ &fs_settings[78], 4, 0, &fs_names[96], 0 },
	{ &fs_settings[79], 3, 0, &fs_names[95], 0 },
	{ &fs_settings[80], 2, 0, &fs_names[94], 0 },
	{ &fs_settings[81], 1, 0, &fs_names[93], 0 },
	{ &fs_settings[82], 0, 0, &fs_names[92], 1 },
	{ NULL, 1, 0, &fs_names[105], 1 },
	{ &fs_settings[84], 0, 0, &fs_names[104], 0 },
	{ NULL, 1, 0, &fs_names[108], 0 },
	{ &fs_settings[86], 0, 0, &fs_names[107], 1 },
	{ NULL, 9, 0, &fs_names[119], 0 },
	{ &fs_settings[88], 8, 0, &fs_names[118], 0 },
	{ &fs_settings[89], 7, 0, &fs_names[117], 0 },
	{ &fs_settings[90], 6, 0, &fs_names[116], 0 },
	{ &fs_settings[91], 5, 0, &fs_names[115], 0 },
	{ &fs_settings[92], 4, 0, &fs_names[114], 0 },
	{ &fs_settings[93], 3, 0, &fs_names[113], 0 },
	{ &fs_settings[94], 2, 0, &fs_names[112], 0 },
	{ &fs_settings[95], 1, 0, &fs_names[111], 0 },
	{ &fs_settings[96], 0, 0, &fs_names[110], 1 },
	{ NULL, 8, 0, &fs_names[129], 0 },
	{ &fs_settings[98], 7, 0, &fs_names[128], 0 },
	{ &fs_settings[99], 6, 0, &fs_names[127], 0 },
	{ &fs_settings[100], 5, 0, &fs_names[126], 0 },
	{ &fs_settings[101], 4, 0, &fs_names[125], 0 },
	{ &fs_settings[102], 3, 0, &fs_names[124], 0 },
	{ &fs_settings[103], 2, 0, &fs_names[123], 0 },
	{ &fs_settings[104], 1, 0, &fs_names[122], 0 },
	{ &fs_settings[105], 0, 0, &fs_names[121], 1 },
	{ NULL, 1, 0, &fs_names[132], 0 },
	{ &fs_settings[107], 0, 0, &fs_names[131], 1 },
	{ NULL, 1, 0, &fs_names[135], 0 },
	{ &fs_settings[109], 0, 0, &fs_names[134], 1 },
	{ NULL, 0, 0, &fs_names[145], 0 },
	{ NULL, 3, 0, &fs_names[143], 0 },
	{ &fs_settings[112], 2, 0, &fs_names[142], 0 },
	{ &fs_settings[113], 1, 0, &fs_names[141], 0 },
	{ &fs_settings[114], 0, 0, &fs_names[140], 1 },
	{ NULL, 0, 0, NULL, 0 }
};

static MacFeat fs_features[] = {
	{ NULL, 103, 1, 0, 0, &fs_names[139], &fs_settings[115] },
	{ &fs_features[0], 27, 0, 0, 0, &fs_names[144], &fs_settings[111] },
	{ &fs_features[1], 26, 1, 0, 0, &fs_names[133], &fs_settings[110] },
	{ &fs_features[2], 25, 1, 0, 0, &fs_names[130], &fs_settings[108] },
	{ &fs_features[3], 24, 1, 0, 0, &fs_names[120], &fs_settings[106] },
	{ &fs_features[4], 23, 1, 0, 0, &fs_names[109], &fs_settings[97] },
	{ &fs_features[5], 22, 1, 0, 0, &fs_names[106], &fs_settings[87] },
	{ &fs_features[6], 21, 1, 1, 0, &fs_names[103], &fs_settings[85] },
	{ &fs_features[7], 20, 1, 0, 0, &fs_names[91], &fs_settings[83] },
	{ &fs_features[8], 19, 1, 0, 0, &fs_names[84], &fs_settings[72] },
	{ &fs_features[9], 18, 1, 0, 0, &fs_names[78], &fs_settings[66] },
	{ &fs_features[10], 17, 1, 0, 0, &fs_names[74], &fs_settings[61] },
	{ &fs_features[11], 16, 1, 0, 0, &fs_names[66], &fs_settings[58] },
	{ &fs_features[12], 15, 0, 0, 0, &fs_names[60], &fs_settings[51] },
	{ &fs_features[13], 14, 0, 0, 0, &fs_names[53], &fs_settings[46] },
	{ &fs_features[14], 13, 1, 0, 0, &fs_names[50], &fs_settings[40] },
	{ &fs_features[15], 11, 1, 0, 0, &fs_names[46], &fs_settings[38] },
	{ &fs_features[16], 10, 1, 0, 0, &fs_names[41], &fs_settings[35] },
	{ &fs_features[17], 9, 1, 0, 0, &fs_names[37], &fs_settings[31] },
	{ &fs_features[18], 8, 0, 0, 0, &fs_names[31], &fs_settings[28] },
	{ &fs_features[19], 6, 1, 0, 0, &fs_names[28], &fs_settings[23] },
	{ &fs_features[20], 5, 1, 0, 0, &fs_names[25], &fs_settings[21] },
	{ &fs_features[21], 4, 1, 0, 0, &fs_names[22], &fs_settings[19] },
	{ &fs_features[22], 3, 1, 0, 0, &fs_names[15], &fs_settings[17] },
	{ &fs_features[23], 2, 1, 0, 0, &fs_names[11], &fs_settings[11] },
	{ &fs_features[24], 1, 0, 0, 0, &fs_names[2], &fs_settings[8] },
	{ &fs_features[25], 0, 0, 0, 0, &fs_names[0], &fs_settings[0] },
	{ NULL, 0, 0, 0, 0, NULL, NULL }
};

MacFeat *default_mac_feature_map = &fs_features[26],
	*builtin_mac_feature_map=&fs_features[26],
	*user_mac_feature_map;

static int MacNamesDiffer(struct macname *mn, struct macname *mn2) {

    for ( ; mn!=NULL && mn2!=NULL; mn=mn->next, mn2 = mn2->next ) {
	if ( mn->lang != mn2->lang || mn->enc!=mn2->enc ||
		strcmp(mn->name,mn2->name)!=0 )
return( true );
    }
    if ( mn==mn2 )		/* Both NULL */
return( false );

return( true );
}

static int MacSettingsDiffer(struct macsetting *ms, struct macsetting *ms2) {

    for ( ; ms!=NULL && ms2!=NULL; ms=ms->next, ms2 = ms2->next ) {
	if ( ms->setting != ms2->setting ||
		ms->initially_enabled != ms2->initially_enabled ||
		MacNamesDiffer(ms->setname,ms2->setname) )
return( true );
    }
    if ( ms==ms2 )		/* Both NULL */
return( false );

return( true );
}

int UserFeaturesDiffer(void) {
    MacFeat *mf, *mf2;
    if ( user_mac_feature_map==NULL )
return( false );
    for ( mf=builtin_mac_feature_map, mf2=user_mac_feature_map;
	    mf!=NULL && mf2!=NULL; mf=mf->next, mf2 = mf2->next ) {
	if ( mf->feature != mf2->feature || mf->ismutex != mf2->ismutex ||
		mf->default_setting != mf2->default_setting ||
		MacNamesDiffer(mf->featname,mf2->featname) ||
		MacSettingsDiffer(mf->settings,mf2->settings))
return( true );
    }
    if ( mf==mf2 )		/* Both NULL */
return( false );

return( true );
}

struct macname *MacNameCopy(struct macname *mn) {
    struct macname *head=NULL, *last, *cur;

    while ( mn!=NULL ) {
	cur = chunkalloc(sizeof(struct macname));
	cur->enc = mn->enc;
	cur->lang = mn->lang;
	cur->name = copy(mn->name);
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;
	mn = mn->next;
    }
return( head );
}

/* Sigh. This list is duplicated in macencui.c */
static struct { char *name; int code; } localmaclang[] = {
    {N_("English"), 0},
    {N_("French"), 1},
    {N_("German"), 2},
    {N_("Italian"), 3},
    {N_("Dutch"), 4},
    {N_("Swedish"), 5},
    {N_("Spanish"), 6},
    {N_("Danish"), 7},
    {N_("Portuguese"), 8},
    {N_("Norwegian"), 9},
    {N_("Lang|Hebrew"), 10},
    {N_("Japanese"), 11},
    {N_("Lang|Arabic"), 12},
    {N_("Finnish"), 13},
    {N_("Lang|Greek"), 14},
    {N_("Icelandic"), 15},
    {N_("Maltese"), 16},
    {N_("Turkish"), 17},
    {N_("Croatian"), 18},
    {N_("Traditional Chinese"), 19},
    {N_("Urdu"), 20},
    {N_("Hindi"), 21},
    {N_("Lang|Thai"), 22},
    {N_("Korean"), 23},
    {N_("Lithuanian"), 24},
    {N_("Polish"), 25},
    {N_("Hungarian"), 26},
    {N_("Estonian"), 27},
    {N_("Latvian"), 28},
    {N_("Sami (Lappish)"), 29},
    {N_("Faroese (Icelandic)"), 30},
/* GT: See the long comment at "Property|New" */
/* GT: The msgstr should contain a translation of "Farsi/Persian"), ignore "Lang|" */
    {N_("Lang|Farsi/Persian"), 31},
    {N_("Russian"), 32},
    {N_("Simplified Chinese"), 33},
    {N_("Flemish"), 34},
    {N_("Irish Gaelic"), 35},
    {N_("Albanian"), 36},
    {N_("Romanian"), 37},
    {N_("Czech"), 38},
    {N_("Slovak"), 39},
    {N_("Slovenian"), 40},
    {N_("Yiddish"), 41},
    {N_("Serbian"), 42},
    {N_("Macedonian"), 43},
    {N_("Bulgarian"), 44},
    {N_("Ukrainian"), 45},
    {N_("Byelorussian"), 46},
    {N_("Uzbek"), 47},
    {N_("Kazakh"), 48},
    {N_("Axerbaijani (Cyrillic)"), 49},
    {N_("Axerbaijani (Arabic)"), 50},
    {N_("Lang|Armenian"), 51},
    {N_("Lang|Georgian"), 52},
    {N_("Moldavian"), 53},
    {N_("Kirghiz"), 54},
    {N_("Tajiki"), 55},
    {N_("Turkmen"), 56},
    {N_("Mongolian (Mongolian)"), 57},
    {N_("Mongolian (cyrillic)"), 58},
    {N_("Pashto"), 59},
    {N_("Kurdish"), 60},
    {N_("Kashmiri"), 61},
    {N_("Sindhi"), 62},
    {N_("Lang|Tibetan"), 63},
    {N_("Nepali"), 64},
    {N_("Sanskrit"), 65},
    {N_("Marathi"), 66},
    {N_("Lang|Bengali"), 67},
    {N_("Assamese"), 68},
    {N_("Lang|Gujarati"), 69},
    {N_("Punjabi"), 70},
    {N_("Lang|Oriya"), 71},
    {N_("Lang|Malayalam"), 72},
    {N_("Lang|Kannada"), 73},
    {N_("Lang|Tamil"), 74},
    {N_("Lang|Telugu"), 75},
    {N_("Lang|Sinhalese"), 76},
    {N_("Burmese"), 77},
    {N_("Lang|Khmer"), 78},
    {N_("Lang|Lao"), 79},
    {N_("Vietnamese"), 80},
    {N_("Indonesian"), 81},
    {N_("Lang|Tagalog"), 82},
    {N_("Malay (roman)"), 83},
    {N_("Malay (arabic)"), 84},
    {N_("Lang|Amharic"), 85},
    {N_("Tigrinya"), 86},
    {N_("Galla"), 87},
    {N_("Somali"), 88},
    {N_("Swahili"), 89},
    {N_("Kinyarwanda/Ruanda"), 90},
    {N_("Rundi"), 91},
    {N_("Nyanja/Chewa"), 92},
    {N_("Malagasy"), 93},
    {N_("Esperanto"), 94},
    {N_("Welsh"), 128},
    {N_("Basque"), 129},
    {N_("Catalan"), 130},
    {N_("Lang|Latin"), 131},
    {N_("Quechua"), 132},
    {N_("Guarani"), 133},
    {N_("Aymara"), 134},
    {N_("Tatar"), 135},
    {N_("Lang|Uighur"), 136},
    {N_("Dzongkha"), 137},
    {N_("Javanese (roman)"), 138},
    {N_("Sundanese (roman)"), 139},
    {N_("Galician"), 140},
    {N_("Afrikaans"), 141},
    {N_("Breton"), 142},
    {N_("Inuktitut"), 143},
    {N_("Scottish Gaelic"), 144},
    {N_("Manx Gaelic"), 145},
    {N_("Irish Gaelic (with dot)"), 146},
    {N_("Tongan"), 147},
    {N_("Greek (polytonic)"), 148},
    {N_("Greenlandic"), 149},
    {N_("Azebaijani (roman)"), 150},
    { NULL, 0 }
};

char *MacLanguageFromCode(int code) {
    int i;

    if ( code==-1 )
return( _("Unspecified Language") );

    for ( i=0; localmaclang[i].name!=NULL; ++i )
	if ( code == localmaclang[i].code )
return( S_(localmaclang[i].name) );

return( _("Unknown Language"));
}
