/* Copyright (C) 2021 by Jeremy Tan */
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

#include "uninames_data.h"
#include <assert.h>

static char* read_lexicon(char* dst, unsigned int idx, int* bufsiz) {
    idx = lexicon_offset[idx] + lexicon_shift[idx >> LEXICON_SHIFT];
    do {
        *dst++ = lexicon_data[idx] & 0x7f;
    } while (--*bufsiz > 1 && !(lexicon_data[idx++] & 0x80));
    assert(lexicon_data[idx-1] & 0x80);
    return dst;
}

static const unsigned char* get_phrasebook_entry(unichar_t ch) {
    int index = 0, shift_index;
    if (ch < UNICODE_MAX) {
        index = phrasebook_index1[ch >> PHRASEBOOK_SHIFT1];
        index = phrasebook_index2[(index << PHRASEBOOK_SHIFT1) + (ch & ((1 << PHRASEBOOK_SHIFT1) - 1))];
    }
    if (index == 0) {
        return NULL;
    }
    shift_index = ch >> PHRASEBOOK_SHIFT2;
    if (shift_index > PHRASEBOOK_SHIFT2_CAP) {
        shift_index = PHRASEBOOK_SHIFT2_CAP;
    }
    index += phrasebook_shift[shift_index];
    assert(index >= 0 && (size_t)index < sizeof(phrasebook_data)/sizeof(phrasebook_data[0]));
    return &phrasebook_data[index];
}

static char* prettify_annotation_start(char* dst, int src, int* bufsiz) {
    switch(src) {
    case '#': src = 0x2248; *bufsiz -= 3; break;
    case '%': src = 0x203b; *bufsiz -= 3; break;
    case '*': src = 0x2022; *bufsiz -= 3; break;
    case ':': src = 0x2261; *bufsiz -= 3; break;
    case 'x': src = 0x2192; *bufsiz -= 3; break;
    case '~': src = 0x2053; *bufsiz -= 3; break;
    default:
        assert(false && "unknown annotation start char");
        // fallthrough
    case '=': *bufsiz -= 1;
    }
    return utf8_idpb(dst, src, 0);
}

char* uniname_name(unichar_t ch) {
    const unsigned char *src;
    static char ret[MAX_NAME_LENGTH];
    int bufsiz = sizeof(ret)/sizeof(ret[0]);
    char* dst = ret;

    src = get_phrasebook_entry(ch);
    if (src == NULL || *src == '\n' || *src == '\0') {
        return get_derived_name(ch);
    }

    while (*src != '\n' && *src != '\0' && bufsiz > 4) {
        switch (*src >> 4) {
        case 8: case 9: case 10: case 11: // 0b10xx -> invalid utf-8; our escape
            dst = read_lexicon(dst, ((src[0] & 0x3f) << 7) | (src[1] & 0x7f), &bufsiz);
            src += 2;
            break;
        case 15: // 0b1111 -> 4 bytes
            *dst++ = *src++;
            --bufsiz;
            // fallthrough
        case 14: // 0b1110 -> 3 bytes
            *dst++ = *src++;
            --bufsiz;
            // fallthrough
        case 12: case 13: // 0b110x -> 2 bytes
            *dst++ = *src++;
            --bufsiz;
            // fallthrough
        default: // 0b0xxx -> 1 byte
            *dst++ = *src++;
            --bufsiz;
        }
    }

    assert(bufsiz > 4);
    if (bufsiz < 0) {
        bufsiz = 0;
    }
    return copyn(ret, (sizeof(ret)/sizeof(ret[0])) - bufsiz);
}

char* uniname_annotation(unichar_t ch, int prettify) {
    const unsigned char *src;
    char ret[MAX_ANNOTATION_LENGTH];
    int bufsiz = sizeof(ret)/sizeof(ret[0]);
    char* dst = ret;

    src = get_phrasebook_entry(ch);
    if (src == NULL) {
        return NULL;
    }

    src = (const unsigned char*)strchr((const char*)src, '\n');
    if (src == NULL) {
        return NULL;
    }
    ++src;

    if (prettify) {
        *dst++ = '\t';
        dst = prettify_annotation_start(dst, *src++, &bufsiz);
        *dst++ = ' ';
    } else {
        *dst++ = *src++;
        *dst++ = ' ';
    }
    bufsiz -= 2;

    while (*src != '\0' && bufsiz > 4) {
        switch (*src >> 4) {
        case 8: case 9: case 10: case 11: // 0b10xx -> invalid utf-8; our escape
            dst = read_lexicon(dst, ((src[0] & 0x3f) << 7) | (src[1] & 0x7f), &bufsiz);
            src += 2;
            break;
        case 15: // 0b1111 -> 4 bytes
            *dst++ = *src++;
            --bufsiz;
            // fallthrough
        case 14: // 0b1110 -> 3 bytes
            *dst++ = *src++;
            --bufsiz;
            // fallthrough
        case 12: case 13: // 0b110x -> 2 bytes
            *dst++ = *src++;
            --bufsiz;
            // fallthrough
        default: // 0b0xxx -> 1 byte
            *dst = *src++;
            --bufsiz;
            if (*dst++ == '\n') {
                if (prettify) {
                    *dst++ = '\t';
                    dst = prettify_annotation_start(dst, *src++, &bufsiz);
                    *dst++ = ' ';
                } else {
                    *dst++ = *src++;
                    *dst++ = ' ';
                }
                bufsiz -= 2;
            }
        }
    }

    assert(bufsiz > 4);
    if (bufsiz < 0) {
        bufsiz = 0;
    }
    return copyn(ret, sizeof(ret)/sizeof(ret[0]) - bufsiz);
}

char* uniname_formal_alias(unichar_t ch) {
    char *annot = uniname_annotation(ch, false), *ret = NULL;
    if (annot) {
        if (annot[0] == '%') {
            char* pt = strchr(annot, '\n');
            if (pt) {
                *pt = '\0';
            }
            ret = copy(annot + 2);
        }
        free(annot);
    }
    return ret;
}

static const struct unicode_range* find_range(unichar_t ch, const struct unicode_range* data, size_t hi) {
    size_t lo = 0, mid;

    while (lo <= hi) {
        mid = (lo+hi)/2;
        if (ch >= data[mid].start && ch <= data[mid].end) {
            return &data[mid];
        } else if (data[mid].start < ch) {
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }

    return NULL;
}

const struct unicode_range* uniname_block(unichar_t ch) {
    return find_range(ch, unicode_blocks, sizeof(unicode_blocks)/sizeof(unicode_blocks[0]));
}

const struct unicode_range* uniname_plane(unichar_t ch) {
    return find_range(ch, unicode_planes, sizeof(unicode_planes)/sizeof(unicode_planes[0]));
}

const struct unicode_range* uniname_blocks(int *sz) {
    if (sz) {
        *sz = sizeof(unicode_blocks)/sizeof(unicode_blocks[0]);
    }
    return unicode_blocks;
}

const struct unicode_range* uniname_planes(int *sz) {
    if (sz) {
        *sz = sizeof(unicode_planes)/sizeof(unicode_planes[0]);
    }
    return unicode_planes;
}
