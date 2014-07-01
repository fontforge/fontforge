// GIOGetMimeType: Copyright (C) 2012 Khaled Hosny
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.

#include <glib.h>
#include <gio/gio.h>
#include "gio.h"
#include "ustring.h"

char* GIOGetMimeType(const char *path,int sniff_data) {
    char *content_type, *mime, *temp;
    int sniff_length=4096;
    guchar sniff_buffer[sniff_length];
    gboolean uncertain;

    content_type=g_content_type_guess(path,NULL,0,NULL);

    if ( sniff_data ) {
	FILE *fp=fopen(path,"rb");
	if ( fp!=NULL ) {
	    size_t res=fread(sniff_buffer,1,sniff_length,fp);
	    fclose (fp);
	    if (res>=0 ) {
		g_free(content_type);
		content_type=g_content_type_guess(NULL,sniff_buffer,res,&uncertain);
		if (uncertain) {
		    g_content_type_guess(path,sniff_buffer,res,NULL);
                }
            }
        }
    }

    temp=g_content_type_get_mime_type(content_type);
    g_free(content_type);

    mime=copy(temp);	/* ...convert to generic malloc/free */
    g_free(temp);

    return( mime );
}


