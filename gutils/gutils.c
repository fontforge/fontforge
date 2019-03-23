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

#include "basics.h"
#include "gutils.h"

#include <ffglib.h>

const char *GetAuthor(void) {
    static char author[200] = {0};

    if (author[0]) {
        return author;
    } else if (getenv("SOURCE_DATE_EPOCH")) {
        const char *username = getenv("USER");
        if (username) {
            snprintf(author, sizeof(author), "%s", username);
            return author;
        }
    }

    return g_get_real_name(); // static buffer, should not be freed
}

time_t GetTime(void) {
	time_t now;
	const char *source_date_epoch = getenv("SOURCE_DATE_EPOCH");
	if (source_date_epoch) {
		now = atol(source_date_epoch);
	} else {
		now = time(NULL);
	}

	return now;
}

time_t GetST_MTime(struct stat s) {
	time_t st_time;
	if (getenv("SOURCE_DATE_EPOCH")) {
		st_time = atol(getenv("SOURCE_DATE_EPOCH"));
	} else {
		st_time = s.st_mtime;
	}

	return st_time;
}
