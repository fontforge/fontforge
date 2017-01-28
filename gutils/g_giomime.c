// GIOGetMimeType: Copyright (C) 2012 Khaled Hosny
// GIOguessMimeType: Copyright (C) 2014 Jose Da Silva
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

#include "gio.h"
#include "gfile.h"
#include "ustring.h"
#include <gio/gio.h>
#include <glib.h>

const char *MimeListFromExt[] = {
/* This list is indexed from list ExtToMimeList */
	/* 0 .bmp */	"image/bmp",
	/* 1 .xbm */	"image/x-xbitmap",
	/* 2 .xpm */	"image/x-xpixmap",
	/* 3 .tif */	"image/tiff",
	/* 4 .jpg */	"image/jpeg",
	/* 5 .png */	"image/png",
	/* 6 .gif */	"image/gif",
	/* 7 .sun */	"image/x-sun-raster",
	/* 8 ,sgi */	"image/x-sgi",
	/* 9 .txt */	"text/plain",
	/*10 .htm */	"text/html",
	/*11 .xml */	"text/xml",
	/*12 .css */	"text/css",
	/*13 .c */	"text/c",
	/*14 .java */	"text/java",
	/*15 .ps */	"text/fontps",
	/*16 .pfa */	"text/ps",
	/*17 .obj */	"application/x-object",
	/*18 core */	"application/x-core",
	/*19 .tar */	"application/x-tar",
	/*20 .zip */	"application/x-compressed",
	/* vnd.font-fontforge-sfd Officially registered with IANA on 14 May 2008 */
	/*21 .sfd */	"application/vnd.font-fontforge-sfd",
	/*22 .pdf */	"application/pdf",
	/*23 .ttf */	"application/x-font-ttf",
	/*24 .otf */	"application/x-font-otf",
	/*25 ,cid */	"application/x-font-cid",
	/*26 .pcf */	"application/x-font-pcf",
	/*27 .snf */	"application/x-font-snf",
	/*28 .bdf */	"application/x-font-bdf",
	/*29 .woff */	"application/x-font-woff",
	/*30 .dfont */	"application/x-mac-dfont",
	/*31 .ffil */	"application/x-mac-suit"
};

typedef struct {
    char *ext3;	/* filename dot ext3 */
    int mime;	/* index to mime type */
} ext3mime;

const ext3mime ExtToMimeList[] = {
    {".bmp", 0},
    {".xbm", 1},
    {".xpm", 2},
    {".tiff", 3},{".tif", 3},
    {".jpeg", 4},{".jpg", 4},
    {".png", 5},
    {".gif", 6},
    {".ras", 7},{".im1", 7},{".im8", 7},{".im24", 7},
    {".im32", 7},{".rs", 7},{".sun", 7},
    {".rgb", 8},{".rgba", 8},{".sgi", 8},{".bw", 8},
    {".txt", 9},{".text", 9},{".sh", 9},{".bat", 9},
    {".html", 10},{".htm", 10},
    {".xml", 11},
    {".css", 12},
    {".c", 13},{".h", 13},
    {"java", 14},
    {".pfa", 15},{".pfb", 15},{".pt3", 15},{".cff", 15},
    {".ps", 16},{".eps", 16},
    {".o", 17},{".obj", 17},
    {".tar", 19},
    {".gz", 20},{".tgz", 20},{".z", 20},{".zip", 20},
    {".bz2", 20},{".tbz", 20},{".rpm", 20},
    {".sfd", 21},
    {".pdf", 22},
    {".ttf", 23},
    {".otf", 24},{".otb", 24},{".gai", 24},
    {".cid", 25},
    {".pcf", 26},
    {".snf", 27},
    {".bdf", 28},
    {".woff", 29},
    {".dfont", 30},
    {".ffil", 31},
    {0, 0}
};

char *GIOguessMimeType(const char *path) {
/* Try guess mime type based on filename dot3 extension	*/
/* This list is incomplete, but saves having to sniff a	*/
/* file to figure-out the file-type (if dot3 extension)	*/
/* plus, some operating systems have weak mime guessing	*/
/* routines, but have strong dot3 extension usage.	*/
    int i;
    char *pt;

    if ( (pt=strrchr(path,'.'))!=NULL )
	for (i = 0; ExtToMimeList[i].ext3; i++)
	    if ( strcasecmp(pt,ExtToMimeList[i].ext3)==0 )
		return( copy(MimeListFromExt[ExtToMimeList[i].mime]) );
    return( 0 );
}

char* GIOGetMimeType(const char *path) {
/* Get file mime type by sniffing a portion of the file */
/* If that does not work, then depend on guessing type. */
#define	sniff_length	4096
    char *content_type=NULL,*mime=0;

/*
   Doing this on Windows is horrendously slow. glib on Windows also only uses
   the sniff buffer iff MIME detection via file extension fails, and even
   then, only for determining if it /looks like/ a text file.
*/
#ifndef __MINGW32__
    FILE *fp;

    if ( (fp=fopen(path,"rb"))!=NULL ) {
	guchar sniff_buffer[sniff_length];
	gboolean uncertain;
	size_t res=fread(sniff_buffer,1,sniff_length,fp);
	int err=ferror(fp);
	fclose (fp);
	if ( !err ) {
	    // first force guessing file type from the content only by passing
	    // NULL for file name, if the result is not certain try again with
	    // file name
	    content_type=g_content_type_guess(NULL,sniff_buffer,res,&uncertain);
	    if (uncertain) {
	        if (content_type!=NULL)
                    g_free(content_type);
		content_type=g_content_type_guess(path,sniff_buffer,res,NULL);
            }
        }
    }
#endif

    if ( content_type==NULL )
	/* if sniffing failed - then try and guess the file type */
	content_type=g_content_type_guess(path,NULL,0,NULL);

    if ( content_type!=NULL ) {
	char *temp=g_content_type_get_mime_type(content_type);
	g_free(content_type);

	if ( temp!=NULL ) {
	    mime=copy(temp);	/* ...convert to generic malloc/free */
	    g_free(temp);
	}
    }

    return( mime );
}


