/* Copyright 2024 Maxim Iorsh <iorsh@users.sourceforge.net>
 *
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
#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <string>

namespace ff {

class Tag {
 public:
    Tag(uint32_t val)
        : arr{(char)((val >> 24) & 0xff), (char)((val >> 16) & 0xff),
              (char)((val >> 8) & 0xff), (char)(val & 0xff), '\0'} {}

    Tag(const char* val) : arr{val[0], val[1], val[2], val[3], '\0'} {}

    explicit operator const char*() const { return arr.data(); }
    operator uint32_t() const {
        return (arr[0] << 24) | (arr[1] << 16) | (arr[2] << 8) | arr[3];
    }

    bool operator==(const char* rhs) const {
        for (int i = 0; i < 5; ++i) {
            // We shall usually continue here.
            if (arr[i] == rhs[i]) continue;

            // If tags differ, it's either a real difference or a space vs zero
            // terminator.
            return ((arr[i] == ' ') && (rhs[i] == '\0'));
        }
        return true;
    }

    bool operator==(const std::string& rhs) const {
        return operator==(rhs.c_str());
    }

 private:
    // OpenType tags normally have four characters, the fifth is the trailing
    // zero.
    std::array<char, 5> arr;
};

const Tag REQUIRED_FEATURE = Tag(" RQD");

}  // namespace ff
