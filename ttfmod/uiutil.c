/* Copyright (C) 2001 by George Williams */
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
#include <time.h>

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

int GetHexR(GWindow gw,int cid,int namer,int *err) {
    const unichar_t *txt; unichar_t *end;
    int val;

    txt = _GGadgetGetTitle(GWidgetGetControl(gw,cid));
    if ( *txt=='U' && txt[1]=='+' )
	txt += 2;
    val = u_strtoul(txt,&end,16);
    if ( *end!='\0' ) {
	ProtestR(namer);
	*err = true;
    }
return( val );
}

int GetListR(GWindow gw,int cid,int namer,int *err) {
    int val;
    GTextInfo *ti;

    ti = GGadgetGetListItemSelected(GWidgetGetControl(gw,cid));
    val = (int) (ti->userdata);
return( val );
}

void GetDateR(GWindow gw,int cid,int namer,uint32 date[2],int *err) {
    const unichar_t *txt; unichar_t *end;
    struct tm tm, *test;
    time_t t;

    txt = _GGadgetGetTitle(GWidgetGetControl(gw,cid));
    while ( *txt==' ' ) ++txt;
    if ( uc_strmatch(txt,"now")==0 ) {
	time(&t);
    } else {
	memset(&tm,'\0',sizeof(tm));
	tm.tm_year = u_strtol(txt,&end,10)-1900;
	if ( *end=='-' ) ++end;
	tm.tm_mon = u_strtol(end,&end,10)-1;
	if ( *end=='-' ) ++end;
	tm.tm_mday = u_strtol(end,&end,10);
	while ( *end==' ' ) ++end;

	tm.tm_hour = u_strtol(end,&end,10);
	if ( *end==':' ) ++end;
	tm.tm_min = u_strtol(end,&end,10);
	if ( *end==':' ) ++end;
	tm.tm_sec = u_strtol(end,&end,10);
	t = mktime(&tm);		/* mktime is not document to deal with dst correctly */
	test = localtime(&t);
	if ( test->tm_isdst ) {
	    tm.tm_isdst = true;
	    t = mktime(&tm);
	}
    }

    TimeTToQuad(t,date);
}

void TimeTToQuad(time_t t, uint32 date[2]) {
    uint32 date1904[4];
    uint32 year[2];
    int i;

    if ( sizeof(time_t)>32 ) {
	/* as unixes switch over to 64 bit times, this will be the better */
	/*  solution */
	
	t += ((time_t) 60)*60*24*365*(70-4);
	t += 60*60*24*(70-4)/4;		/* leap years */
	date[0] = ((t>>16)>>16);
	date[1] = t&0xffffffff;
    } else {
	date1904[0] = date1904[1] = date1904[2] = date1904[3] = 0;
	year[0] = 60*60*24*365;
	year[1] = year[0]>>16; year[0] &= 0xffff;
	for ( i=4; i<70; ++i ) {
	    date1904[3] += year[0];
	    date1904[2] += year[1];
	    if ( (i&3)==0 )
		date1904[3] += 60*60*24;
	    date1904[2] += date1904[3]>>16;
	    date1904[3] &= 0xffff;
	    date1904[1] += date1904[2]>>16;
	    date1904[2] &= 0xffff;
	}
	date1904[3] += t&0xffff;
	date1904[2] += t>>16;
	date1904[2] += date1904[3]>>16;
	date1904[3] &= 0xffff;
	date1904[1] += date1904[2]>>16;
	date1904[2] &= 0xffff;
	date[0] = (date1904[0]<<16) | date1904[1];
	date[1] = (date1904[2]<<16) | date1904[3];
    }
}
