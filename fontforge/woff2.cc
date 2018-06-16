/* Copyright (C) 2018 by Jeremy Tan */
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

#include <stdint.h>
#include <string>
#include <vector>
#include <woff2/decode.h>
#include <woff2/encode.h>

/**
 * Fontforge's headers aren't C++ includable, so forward-declare
 * everything ourselves.
 */
extern "C" {
typedef struct splinefont SplineFont;
typedef struct encmap EncMap;
enum fontformat {};
enum bitmapformat {};
enum openflags {};

SplineFont *_SFReadTTF(FILE *ttf, int flags, enum openflags openflags, char *filename, struct fontdict *fd);
int _WriteTTFFont(FILE *ttf, SplineFont *sf, enum fontformat format, int32_t *bsizes, enum bitmapformat bf, int flags, EncMap *map, int layer);

int WriteWOFF2Font(char *fontname, SplineFont *sf, enum fontformat format, int32_t *bsizes, enum bitmapformat bf, int flags, EncMap *enc, int layer);
int _WriteWOFF2Font(FILE *fp, SplineFont *sf, enum fontformat format, int32_t *bsizes, enum bitmapformat bf, int flags, EncMap *enc, int layer);
SplineFont *_SFReadWOFF2(FILE *fp, int flags, enum openflags openflags, char *filename, struct fontdict *fd);
}

/**
 * Read the contents of fp into buf
 */
static bool ReadFile(FILE *fp, std::vector<uint8_t> &buf)
{
    if (fseek(fp, 0, SEEK_END) < 0) {
        return false;
    }

    long length = ftell(fp);
    if (length <= 0 || fseek(fp, 0, SEEK_SET) < 0) {
        return false;
    }

    buf.resize(length);
    size_t actual = fread(&buf[0], 1, length, fp);
    if (fgetc(fp) != EOF) {
        return false;
    }
    buf.resize(actual);
    return true;
}

/**
 * Write the contents of buf into fp. 
 * On success, the returned file pointer is equal to fp.
 * It will be positioned at the start of the file.
 */
static FILE *WriteToFile(FILE *fp, const std::string &buf)
{
    if (fp && fwrite(&buf[0], 1, buf.size(), fp) == buf.size()) {
        if (fseek(fp, 0, SEEK_SET) >= 0) {
            return fp;
        }
    }
    return nullptr;
}

/**
 * Write the contents of buf into a temporary file.
 * On success, the returned file pointer is at the start of the file.
 * The caller is responsible for closing it.
 */
static FILE *WriteToFile(const std::string &buf)
{
    FILE *fp = tmpfile();
    if (!WriteToFile(fp, buf)) {
        fclose(fp);
        return nullptr;
    }
    return fp;
}

/**
 * Convert fp (woff2) to ttf, stored in a temporary file.
 * The caller is responsible for closing the returned file.
 */
static FILE *WOFF2ToTTF(FILE *fp)
{
    try {
        std::vector<uint8_t> raw_input;
        if (!ReadFile(fp, raw_input)) {
            return nullptr;
        }

        std::string ttfBuffer(std::min(woff2::ComputeWOFF2FinalSize(&raw_input[0], raw_input.size()),
                                       woff2::kDefaultMaxSize),
                              0);
        woff2::WOFF2StringOut output(&ttfBuffer);

        if (!woff2::ConvertWOFF2ToTTF(&raw_input[0], raw_input.size(), &output)) {
            return nullptr;
        }
        ttfBuffer.resize(output.Size());

        return WriteToFile(ttfBuffer);
    }
    catch (std::exception) {
        return nullptr;
    }
}

/**
 * Convert ttf to woff.
 * Returns true on success.
 */
static bool TTFToWOFF2(FILE *ttf, FILE *woff)
{
    try {
        std::vector<uint8_t> raw_input;
        if (!ReadFile(ttf, raw_input)) {
            return false;
        }

        std::string woffBuffer(woff2::MaxWOFF2CompressedSize(&raw_input[0], raw_input.size()), 0);
        size_t woffActualSize = woffBuffer.size();
        if (!woff2::ConvertTTFToWOFF2(&raw_input[0], raw_input.size(), reinterpret_cast<uint8_t *>(&woffBuffer[0]), &woffActualSize)) {
            return false;
        }
        woffBuffer.resize(woffActualSize);

        return WriteToFile(woff, woffBuffer) != nullptr;
    }
    catch (std::exception) {
        return false;
    }
}

int WriteWOFF2Font(char *fontname, SplineFont *sf, enum fontformat format, int32_t *bsizes, enum bitmapformat bf, int flags, EncMap *enc, int layer)
{
    FILE *woff = fopen(fontname, "wb");
    int ret = 0;
    if (woff) {
        ret = _WriteWOFF2Font(woff, sf, format, bsizes, bf, flags, enc, layer);
        fclose(woff);
    }
    return ret;
}

int _WriteWOFF2Font(FILE *fp, SplineFont *sf, enum fontformat format, int32_t *bsizes, enum bitmapformat bf, int flags, EncMap *enc, int layer)
{
    FILE *tmp = tmpfile();
    int ret = 0;
    if (tmp) {
        if (_WriteTTFFont(tmp, sf, format, bsizes, bf, flags, enc, layer)) {
            if (TTFToWOFF2(tmp, fp)) {
                ret = 1;
            }
        }
        fclose(tmp);
    }
    return ret;
}

SplineFont *_SFReadWOFF2(FILE *fp, int flags, enum openflags openflags, char *filename, struct fontdict *fd)
{
    FILE *ttf = WOFF2ToTTF(fp);
    if (ttf) {
        SplineFont *ret = _SFReadTTF(ttf, flags, openflags, filename, fd);
        fclose(ttf);
        return ret;
    }
    return NULL;
}
