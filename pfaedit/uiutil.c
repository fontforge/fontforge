/* Copyright (C) 2000,2001 by George Williams */
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
#include "pfaeditui.h"
#include <ustring.h>
#include <gfile.h>

void Protest(char *label) {
    char buffer[80];
    unichar_t ubuf[80];
    sprintf( buffer, "Bad Number in %s", label );
    uc_strcpy(ubuf,buffer);
    GWidgetPostNotice(ubuf,ubuf);
}

real GetReal(GWindow gw,int cid,char *name,int *err) {
    const unichar_t *txt; unichar_t *end;
    double val;

    txt = _GGadgetGetTitle(GWidgetGetControl(gw,cid));
    val = u_strtod(txt,&end);
    if ( *end!='\0' ) {
	Protest(name);
	*err = true;
    }
return( val );
}

int GetInt(GWindow gw,int cid,char *name,int *err) {
    const unichar_t *txt; unichar_t *end;
    int val;

    txt = _GGadgetGetTitle(GWidgetGetControl(gw,cid));
    val = u_strtol(txt,&end,10);
    if ( *end!='\0' ) {
	Protest(name);
	*err = true;
    }
return( val );
}

void ProtestR(int labelr) {
    unichar_t ubuf[80];
    u_strcpy(ubuf,GStringGetResource(_STR_Badnumberin,NULL));
    u_strcat(ubuf,GStringGetResource(labelr,NULL));
    if ( ubuf[u_strlen(ubuf)-1]==' ' )
	ubuf[u_strlen(ubuf)-1]='\0';
    if ( ubuf[u_strlen(ubuf)-1]==':' )
	ubuf[u_strlen(ubuf)-1]='\0';
    GWidgetPostNotice(ubuf,ubuf);
}

real GetRealR(GWindow gw,int cid,int namer,int *err) {
    const unichar_t *txt; unichar_t *end;
    real val;

    txt = _GGadgetGetTitle(GWidgetGetControl(gw,cid));
    val = u_strtod(txt,&end);
    if ( *end!='\0' ) {
	ProtestR(namer);
	*err = true;
    }
return( val );
}

int GetIntR(GWindow gw,int cid,int namer,int *err) {
    const unichar_t *txt; unichar_t *end;
    int val;

    txt = _GGadgetGetTitle(GWidgetGetControl(gw,cid));
    val = u_strtol(txt,&end,10);
    if ( *end!='\0' ) {
	ProtestR(namer);
	*err = true;
    }
return( val );
}

static char browser[1025];

static void findbrowser(void) {
    static char *stdbrowsers[] = { "netscape", "opera", "mozilla", "mosaic",
	"kfmclient", /*"grail",*/ "lynx", NULL };
    int i;
    char *path;

    if ( getenv("BROWSER")!=NULL ) {
	strcpy(browser,getenv("BROWSER"));
	if ( strcmp(browser,"kde")==0 || strcmp(browser,"kfm")==0 || strcmp(browser,"kfmclient")==0 )
	    strcpy(browser,"kfmclient openURL");
return;
    }
    for ( i=0; stdbrowsers[i]!=NULL; ++i ) {
	if ( (path=_GFile_find_program_dir(stdbrowsers[i]))!=NULL ) {
	    if ( strcmp(stdbrowsers[i],"kfmclient")==0 )
		strcpy(browser,"kfmclient openURL");
	    else
		strcpy(browser,stdbrowsers[i]);
return;
	}
    }
}

void help(char *file) {
    char fullspec[1024], *temp;

    if ( browser[0]=='\0' )
	findbrowser();
    if ( browser[0]=='\0' ) {
	GDrawError("Could not find a browser. Set the BROWSER environment variable to point to one" );
return;
    }

    if ( strstr(file,"http://")==NULL ) {
	fullspec[0] = 0;
	if ( *file!='/' )
	    strcpy(fullspec,"/usr/share/doc/pfaedit/");
	strcat(fullspec,file);
	if ( !GFileReadable( fullspec )) {
	    strcpy(fullspec,"http://pfaedit.sf.net/");
	    strcat(fullspec,file);
	}
    } else
	strcpy(fullspec,file);
    temp = galloc(strlen(browser) + strlen(fullspec) + 20);
    sprintf( temp, "%s %s &", browser, fullspec );
    system(temp);
    free(temp);
}
