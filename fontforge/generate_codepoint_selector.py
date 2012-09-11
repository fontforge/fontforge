# -*- coding: utf-8 -*-
#
# Copyright (C) 2012 by Barry Schwartz
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# The name of the author may not be used to endorse or promote products
# derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import unicodenames
import locale, re, sys

locale.setlocale(locale.LC_ALL, 'en_US')

uniname_re_str = sys.argv[1]
uniname_re = re.compile(uniname_re_str)

function_name = sys.argv[2]

names_db = unicodenames.unicodenames(unicodenames.find_names_db())
matches = []
codepoint = 0
while codepoint <= 0x10FFFF:
    uniname = names_db.name(codepoint)
    if uniname is not None and uniname_re.search(uniname) is not None:
        matches.append(codepoint)
    codepoint += 1

print('/* This is a generated file. */')
print('')
print('#include <stdlib.h>')
print('')
print('/*')
print(' *  Codepoints whose names match the regular expression')
print(' *')
print(' *      ' + uniname_re_str)
print(' *')
print(' */')
print('static unsigned int matching_codepoints[{}] ='.format(len(matches)))
print('{')
matches_str = str(matches)
print(matches_str[1:-1])
print('};')
print('')

print('static int compare_codepoints (const void *codepoint1, const void *codepoint2)')
print('{')
print('    const unsigned int *cp1 = (const unsigned int *) codepoint1;')
print('    const unsigned int *cp2 = (const unsigned int *) codepoint2;')
print('    return ((*cp1 < *cp2) ? -1 : ((*cp1 == *cp2) ? 0 : 1));')
print('}')
print('')

print('int {}(unsigned int codepoint);'.format(function_name))
print('')

print('int {}(unsigned int codepoint)'.format(function_name))
print('{')
print('    unsigned int *p =')
print('        (unsigned int *) bsearch (&codepoint, matching_codepoints,')
print('                                  {}, sizeof (unsigned int),'.format(len(matches)))
print('                                  compare_codepoints);')
print('    return (p != (unsigned int *) 0);')
print('}')
print('')
