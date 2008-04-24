/* Copyright (C) 2007,2008 by George Williams */
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
#include "fontforgevw.h"
#include <ustring.h>
#include <utype.h>
#include <gio.h>

#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

struct siteinfo {
    int cookie_cnt;
    char *cookies[30];
    int user_id;
};

enum conversation_type { ct_savecookies, ct_slurpdata, ct_getuserid };

static int findHTTPhost(struct sockaddr_in *addr, char *hostname, int port) {
    struct servent *servent;
    struct hostent *hostent;
    int i;

    memset(addr,0,sizeof(*addr));
    addr->sin_family = AF_INET;
    if ( port>=0 )
	addr->sin_port = htons(port);
    else if (( servent = getservbyname("http","tcp"))!=NULL )
	addr->sin_port = servent->s_port;	/* Already in server byteorder */
    else
	addr->sin_port = htons(80);
    endservent();
    hostent = gethostbyname(hostname);
    if ( hostent==NULL )
return( 0 );
    for ( i=0; hostent->h_addr_list[i]!=NULL; ++i );
    memcpy(&addr->sin_addr,hostent->h_addr_list[rand()%i],hostent->h_length);
    endhostent();
return( 1 );
}

/* this excedingly complex routine tries to send data on a socket. */
/*  first it waits until the socket is ready to accept data (there is no time-*/
/*  out on send/write), checking to see if the user has aborted the operation */
/*  if an interrupt happens which kills the select call then restart the call */
/*  if we get an select error we assume the connection died. If we time-out we*/
/*  essentially assume the same. If the send fails, ditto. If everything succedes */
/*  record that we sent the command */
static int ftpsend(int ctl, char *cmd) {
    struct timeval tv;
    fd_set wts;
    int i=0, ret=0;

  restart:
    FD_ZERO(&wts);
    while ( i<60 ) {
	FD_SET(ctl,&wts);
	tv.tv_sec = 4; tv.tv_usec = 0;
	if (( ret = select(ctl+1,NULL,&wts,NULL,&tv))<0 ) {
	    if ( errno==EINTR )
  goto restart;
return( -1 );
	} else if ( ret>0 )
    break;
	++i;
    }
    if ( ret==0 ) {
return( -1 );
    }
    if ( send(ctl,cmd,strlen(cmd),MSG_NOSIGNAL)<=0 ) {
	if ( errno==EINTR )	/* interrupted, try again */
  goto restart;
return( -1 );
    }
return( true );
}

/* Similar thing for read */
static int getresponse(int ctl, char *buf, int buflen) {
    struct timeval tv;
    fd_set rds;
    int len;
    int ret=false, i;
    char *pt, *start;

  restart:
    while ( 1 ) {
	FD_ZERO(&rds);
	i = 0;
	while ( i<60 ) {
	    FD_SET(ctl,&rds);
	    tv.tv_sec = 4; tv.tv_usec = 0;
	    if (( ret = select(ctl+1,&rds,NULL,NULL,&tv))<0 ) {
		if ( errno==EINTR )
  goto restart;
return( -1 );
	    } else if ( ret>0 )
	break;
	    ++i;
	}
	if ( ret==0 )		/* Time out */
return( -1 );
	len = read(ctl,buf,buflen);
	if ( len==0 )
return( -1 );
	buf[len]='\0';
	start = buf;
	while ( (pt = strchr(start,'\n' ))!=NULL ) {
	    if ( start[3]==' ' )
return(start[0]=='2' || start[0]=='1');
	    start = pt+1;
	}
    }
}

static int ftpsendr(int ctl, char *cmd, char *buf, int buflen) {

    if ( ftpsend(ctl,cmd)==-1 )
return( -1 );
return( getresponse(ctl,buf,buflen));
}

static int ftpsendpassive(int ctl, struct sockaddr_in *data_addr, char *buf, int buflen) {
    char *pt, *start;

    if ( ftpsend(ctl,"PASV\r\n")==-1 )
return( -1 );
    if ( getresponse(ctl,buf,buflen)== -1 )
return( -1 );

    start = buf;
    while ( (pt = strchr(start,'\n' ))!=NULL ) {
	if ( start[3]==' ' )
    break;
	start = pt+1;
    }
    if ( strtol(buf,NULL,10) == 227 ) {
	/* 227 Entering Passive Mode (h1,h2,h3,h4,p1,p2) */
	int h1,h2,h3,h4,p1,p2;
	for ( pt=buf+4; !isdigit(*pt) && *pt!='\n' ; ++pt );
	if ( *pt=='\n' )
return( -2 );		/* can't parse it */
	sscanf(pt,"%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2 );
	data_addr->sin_family = AF_INET;
	data_addr->sin_port = htons((p1<<8)|p2);
	data_addr->sin_addr.s_addr = htonl((h1<<24)|(h2<<16)|(h3<<8)|h4);
return( 1 );
    } else if ( *start=='4' || *start=='5' )
return( 0 );

return( 0 );	/* shouldn't get here */
}

static int findFTPhost(struct sockaddr_in *addr, char *hostname, int port) {
    struct servent *servent;
    struct hostent *hostent;
    int i;

    memset(addr,0,sizeof(*addr));
    addr->sin_family = AF_INET;
    if ( port>=0 )
	addr->sin_port = htons(port);
    else if (( servent = getservbyname("ftp","tcp"))!=NULL )
	addr->sin_port = servent->s_port;	/* Already in server byteorder */
    else
	addr->sin_port = htons(21);
    endservent();
    hostent = gethostbyname(hostname);
    if ( hostent==NULL )
return( 0 );
    for ( i=0; hostent->h_addr_list[i]!=NULL; ++i );
    memcpy(&addr->sin_addr,hostent->h_addr_list[rand()%i],hostent->h_length);
    endhostent();
return( 1 );
}

static int getTCPsocket() {
    struct protoent *protoent;
    int proto;

    if (( protoent = getprotobyname("tcp"))!=NULL )
	proto = protoent->p_proto;
    else
	proto = IPPROTO_TCP;
    endprotoent();

return( socket(PF_INET,SOCK_STREAM,proto));
}

static int makeConnection(struct sockaddr_in *addr) {
    int soc = getTCPsocket();

    if ( soc==-1 )
return( -1 );
    if ( connect(soc,(struct sockaddr *) addr,sizeof(*addr))== -1 ) {
	perror( "Connect failed?" );
	close(soc);
return( -1 );
    }
return( soc );
}

static char *UrlEncode(char *to,char *source) {
    int nibble;

    while ( *source ) {
	if ( isalnum(*source) || *source=='$' || *source=='_' )
	    *to++ = *source;
	else if ( *source==' ' )
	    *to++ = '+';
	else {
	    *to++ = '%';
	    nibble = (*source>>4)&0xf;
	    if ( nibble<10 )
		*to++ = '0'+nibble;
	    else
		*to++ = 'A'-10 + nibble;
	    nibble = *source&0xf;
	    if ( nibble<10 )
		*to++ = '0'+nibble;
	    else
		*to++ = 'A'-10 + nibble;
	}
	++source;
    }
    *to ='\0';
return( to );
}

static void ChangeLine2_8(char *str) {
    ff_progress_change_line2(str);
    ff_progress_allow_events();
}

static int Converse(int soc, char *databuf, int databuflen, FILE *msg,
	enum conversation_type ct, struct siteinfo *siteinfo ) {
    static int verbose = -1;
    const char *vtext;
    char *pt, *end, *str;
    int first, ended, code, len;
    int ch;

    if ( verbose==-1 ) {
	vtext = getenv("FONTFORGE_HTTP_VERBOSE");
	if ( vtext==NULL )
	    verbose = 0;
	else if ( *vtext=='\0' )
	    verbose = 1;
	else
	    verbose = strtol(vtext,NULL,10);
    }

    if ( verbose ) {
	if ( verbose>=2 )	/* Print the entire message */
	    write(fileno(stdout),databuf,strlen(databuf));
	else {			/* Otherwise, just headers */
	    pt = strstr(databuf,"\r\n\r\n");	/* Blank line, end of headers */
	    if ( pt==NULL )
		write(fileno(stdout),databuf,strlen(databuf));
	    else
		write(fileno(stdout),databuf,pt-databuf);
	}
	write(fileno(stdout),"\n",1);
    }

    if ( write(soc,databuf,strlen(databuf))==-1 ) {
	fprintf( stderr, "Socket write failed\n" );
	close( soc );
return( 404 );		/* Random error */
    }
    if ( msg!=NULL ) {
	/* int tot = 0;*/
	ff_progress_change_total((int) ((ftell(msg)+databuflen-1)/databuflen) );
	rewind(msg);
	while ( (len=fread(databuf,1,databuflen,msg))>0 ) {
/*fprintf( stderr, "Total read=%d\n", tot += len );*/
	    if ( write(soc,databuf,len)==-1 ) {
		fprintf( stderr, "Socket write failed3\n" );
		close( soc );
return( 404 );
	    }
	    if ( !ff_progress_next() )
return( 404 );
	    if ( verbose>=2 )
		write(fileno(stdout),databuf,len);
	}
	fclose(msg);
	if ( !ff_progress_next() )
return( 404 );
	ChangeLine2_8(_("Awaiting response"));
    }

    first = 1; ended = 0;
    code = 404;
    while ((len = read(soc,databuf,databuflen))>0 ) {
	if ( first ) {
	    sscanf(databuf,"HTTP/%*f %d", &code );
	    first = 0;
	}
	if ( !ended ) {
	    pt = databuf;
	    for ( ;; ) {
		end = strstr(pt,"\r\n");
		if ( end==NULL ) end = pt+strlen(pt);
		if ( end==pt ) {
		    ended = 1;	/* Blank line */
		    if ( verbose!=0 && verbose<2 )
			write(fileno(stdout),databuf,end-databuf);
	    break;
		}
		if ( ct == ct_savecookies ) {
		    ch = *end; *end = '\0';
		    str = "Set-Cookie: ";
		    if ( strncmp(pt,str,strlen(str))==0 ) {
			char *end2;
			pt += strlen(str);
			end2 = strstr(pt,"; ");
			if ( end2==NULL )
			    siteinfo->cookies[siteinfo->cookie_cnt++] = copy(pt);
			else {
			    *end2 = '\0';
			    siteinfo->cookies[siteinfo->cookie_cnt++] = copy(pt);
			    *end2 = ' ';
			}
		    }
		    *end = ch;
		}
		if ( *end=='\0' )
	    break;
		pt = end+2;
	    }
	}
	if ( ct== ct_getuserid && (pt=strstr(databuf,"upload_user"))!=NULL ) {
	    pt = strstr(pt,"value=\"");
	    if ( pt!=NULL )
		siteinfo->user_id = strtol(pt+7,NULL,10);
	}
	if ( verbose>=2 || ( verbose!=0 && verbose<2 && !ended) )
	    write(fileno(stdout),databuf,len);
    }
    if ( len==-1 )
	fprintf( stderr, "Socket read failed\n" );
    close( soc );
return( code );
}

static void AttachCookies(char *databuf,struct siteinfo *siteinfo) {
    int i;

    if ( siteinfo->cookie_cnt>0 ) {
	databuf += strlen( databuf );
	sprintf(databuf, "Cookie: %s", siteinfo->cookies[0] );
	for ( i=1; i<siteinfo->cookie_cnt; ++i )
	    sprintf(databuf+strlen(databuf), "; %s", siteinfo->cookies[i] );
	strcat(databuf,"\r\n");
    }
}

int OFLibUploadFont(OFLibData *oflib) {
    struct sockaddr_in addr;
    int soc;
    int datalen;
    char *databuf;
    char msg[1024], *pt;
    struct siteinfo siteinfo;
    int ch, code;
    FILE *formdata, *font;
    char boundary[80], *fontfilename;
    time_t now;
    struct tm *tm;

    ff_progress_start_indicator(0,_("Font Upload..."),_("Uploading to Open Font Library"),
	    _("Looking for openfontlibrary.org"),1,1);
    ff_progress_allow_events();
    ff_progress_allow_events();

    if ( !findHTTPhost(&addr, "openfontlibrary.org",-1)) {
	ff_progress_end_indicator();
	ff_post_error(_("Could not find host"),_("Could not find \"%s\"\nAre you connected to the internet?"), "openfontlibrary.org" );
return( false );
    }
    soc = makeConnection(&addr);
    if ( soc==-1 ) {
	ff_progress_end_indicator();
	ff_post_error(_("Could not connect to host"),_("Could not connect to \"%s\"."), "openfontlibrary.org" );
return( false );
    }

    datalen = 8*8*1024;
    databuf = galloc(datalen+1);
    memset(&siteinfo,0,sizeof(siteinfo));
    siteinfo.user_id = -1;

    ChangeLine2_8("Logging in...");
    strcpy(msg, "user_name=");
    pt = UrlEncode(msg+strlen(msg),oflib->username);
    strcpy(pt, "&user_password=");
    pt = UrlEncode(msg+strlen(msg),oflib->password);
    strcpy(pt, "&form_submit=Log+In&userlogin=classname");
    sprintf( databuf,"POST /media/login HTTP/1.1\r\n"
	"Accept: text/html,text/plain\r\n"
	"Content-Length: %d\r\n"
	"Content-Type: application/x-www-form-urlencoded\r\n"
	"Host: www.openfontlibrary.org\r\n"
	"User-Agent: FontForge\r\n"
	"Connection: close\r\n\r\n%s",
	    (int) strlen( msg ), msg );
    code = Converse( soc, databuf, datalen, NULL, ct_savecookies, &siteinfo );
    /* Amusingly a success return of 200 is actually an error */
    if ( code!=302 ) {
	free(databuf);
	ff_progress_end_indicator();
	ff_post_error(_("Login failed"),_("Could not log in.") );
return( false );
    }

    ChangeLine2_8("Gathering state info...");
    soc = makeConnection(&addr);
    if ( soc==-1 ) {
	ff_progress_end_indicator();
	free(databuf);
	ff_post_error(_("Could not connect to host"),_("Could not connect to \"%s\"."), "openfontlibrary.org" );
return( false );
    }
    sprintf( databuf,"GET /media/submit/font HTTP/1.1\r\n"
	"Host: www.openfontlibrary.org\r\n"
	"Accept: text/html,text/plain\r\n"
	"User-Agent: FontForge\r\n"
	"Connection: close\r\n" );
    AttachCookies(databuf,&siteinfo);
    strcat(databuf,"\r\n");
    code = Converse( soc, databuf, datalen, NULL, ct_getuserid, &siteinfo );
    if ( siteinfo.user_id==-1 ) {
	ff_progress_end_indicator();
	free(databuf);
	ff_post_error(_("Could not read state"),_("Could not read state.") );
return( false );
    }
    ChangeLine2_8("Preparing to transmit...");
    formdata = tmpfile();
    /* formdata = fopen("foobar","w+");	/* !!!! DEBUG */
    sprintf( boundary, "-------AaB03x-------%X-------", rand());
    fontfilename = strrchr(oflib->pathspec,'/');
    if ( fontfilename==NULL ) fontfilename = oflib->pathspec;
    else ++fontfilename;
    fprintf(formdata,"--%s\r\n", boundary );	/* Multipart data begins with a boundary */
    fprintf(formdata,"Content-Disposition: form-data; name=\"upload_name\"\r\n"
		     /*"Content-Type: text/plain; charset=UTF-8\r\n"*/"\r\n" );
    fprintf(formdata,"%s\r\n", oflib->name );
    fprintf(formdata,"--%s\r\n", boundary );
    fprintf(formdata,"Content-Disposition: form-data; name=\"upload_featuring\"\r\n"
		     /*"Content-Type: text/plain; charset=UTF-8\r\n"*/"\r\n" );
    fprintf(formdata,"%s\r\n", oflib->artists );
    fprintf(formdata,"--%s\r\n", boundary );
    fprintf(formdata,"Content-Disposition: form-data; name=\"upload_file_name\"; filename=\"%s\"\r\n"
		     "Content-Type: application/octet-stream\r\n\r\n", fontfilename );

    font = fopen( oflib->pathspec,"rb");
    if ( font==NULL ) {
	fclose(formdata);
	free(databuf);
	ff_progress_end_indicator();
	ff_post_error(_("Font file vanished"),_("The font file we just created can no longer be opened.") );
return( false );
    }
    while ( (ch=getc(font))!=EOF )
	putc(ch,formdata);
    fclose(font);
    fprintf(formdata,"\r\n");		/* Final line break not part of message (I hope) */
    fprintf(formdata,"--%s\r\n", boundary );
    fprintf(formdata,"Content-Disposition: form-data; name=\"upload_tags\"\r\n"
		     /*"Content-Type: text/plain; charset=UTF-8\r\n"*/"\r\n" );
    fprintf(formdata,"%s\r\n", oflib->tags );
    fprintf(formdata,"--%s\r\n", boundary );
    fprintf(formdata,"Content-Disposition: form-data; name=\"upload_description\"\r\n"
		     /*"Content-Type: text/plain; charset=UTF-8\r\n"*/"\r\n" );
    fprintf(formdata,"%s\r\n", oflib->description );
    fprintf(formdata,"--%s\r\n", boundary );
    if ( oflib->notsafeforwork ) {
	fprintf(formdata,"Content-Disposition: form-data; name=\"upload_license\"\r\n\r\n" );
	fprintf(formdata,"on\r\n" );
	fprintf(formdata,"--%s\r\n", boundary );
    }
    fprintf(formdata,"Content-Disposition: form-data; name=\"upload_license\"\r\n\r\n" );
    fprintf(formdata,"%s\r\n", oflib->oflicense ? "ofl_1_1" : "publicdomain" );
    fprintf(formdata,"--%s\r\n", boundary );
    fprintf(formdata,"Content-Disposition: form-data; name=\"upload_published\"\r\n\r\n" );
    fprintf(formdata,"on\r\n" );
    fprintf(formdata,"--%s\r\n", boundary );
    fprintf(formdata,"Content-Disposition: form-data; name=\"form_submit\"\r\n\r\n" );
    fprintf(formdata,"Upload\r\n" );
    fprintf(formdata,"--%s\r\n", boundary );
    fprintf(formdata,"Content-Disposition: form-data; name=\"http_referer\"\r\n\r\n" );
    /*fputs  ("http%3A%2F%2Ffontforge.sf.net%2F\r\n", formdata ); */
    fputs  ("http%3A%2F%2Fopenfontlibrary.org%2Fmedia%2Fsubmit\r\n", formdata );
    fprintf(formdata,"--%s\r\n", boundary );
    fprintf(formdata,"Content-Disposition: form-data; name=\"newupload\"\r\n\r\n" );
    fprintf(formdata,"classname\r\n" );
    fprintf(formdata,"--%s\r\n", boundary );
    fprintf(formdata,"Content-Disposition: form-data; name=\"upload_user\"\r\n\r\n" );
    fprintf(formdata,"%d\r\n", siteinfo.user_id );
    fprintf(formdata,"--%s\r\n", boundary );
    fprintf(formdata,"Content-Disposition: form-data; name=\"upload_config\"\r\n\r\n" );
    fprintf(formdata,"media\r\n" );
    fprintf(formdata,"--%s\r\n", boundary );
    fprintf(formdata,"Content-Disposition: form-data; name=\"upload_date\"\r\n\r\n" );
    time(&now);
    tm = localtime(&now);
    fprintf(formdata,"%d-%d-%d %d:%02d:%02d\r\n", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
	    tm->tm_hour, tm->tm_min, tm->tm_sec );
    fprintf(formdata,"--%s--\r\n", boundary );

    ChangeLine2_8("Transmitting font...");
#if 0
    findHTTPhost(&addr, "powerbook",-1);	/* Debug!!!! */
	/*addr.sin_port = htons(8080);		/* Debug!!!! */
#endif
    soc = makeConnection(&addr);
    if ( soc==-1 ) {
	ff_progress_end_indicator();
	free(databuf);
	fclose(formdata);
	ff_post_error(_("Could not connect to host"),_("Could not connect to \"%s\"."), "openfontlibrary.org" );
return( false );
    }
    sprintf( databuf,"POST /media/submit/font HTTP/1.1\r\n"
    /* sprintf( databuf,"POST /cgi-bin/echo HTTP/1.1\r\n"		/* Debug!!!! */
	"Host: www.openfontlibrary.org\r\n"
	"Accept: text/html,text/plain\r\n"
	"Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\n"
	"User-Agent: FontForge\r\n"
	"Content-Type: multipart/form-data; boundary=\"%s\"\r\n"
	"Content-Length: %ld\r\n"
	"Connection: close\r\n",
	    boundary ,
	    (long) ftell(formdata) );
    AttachCookies(databuf,&siteinfo);
    strcat(databuf,"\r\n");
    code = Converse( soc, databuf, datalen, formdata, ct_slurpdata, &siteinfo );
    ff_progress_end_indicator();
    free( databuf );

    /* I think the expected return code here is 200, that's what I've seen the*/
    /*  two times I've done a successful upload */
    if ( code<200 || code > 399 ) {
	ff_post_error(_("Error from openfontlibrary"),_("Server error code=%d"), code );
return( false );
    } else if ( code!=200 )
	ff_post_notice(_("Unexpected server return"),_("Unexpected server return code=%d"), code );
return( true );
}

static char *decomposeURL(const char *url,char **host, int *port, char **username,
	char **password) {
    char *pt, *pt2, *upt, *ppt;
    char *path;
    char proto[40];
    /* ftp://[user[:password]@]ftpserver[:port]/url-path */

    *username = NULL; *password = NULL; *port = -1;
    pt = strstr(url,"://");
    if ( pt==NULL ) {
	*host = NULL;
return( copy(url));
    }
    strncpy(proto,url,pt-url<sizeof(proto)?pt-url:sizeof(proto));
    proto[(pt-url)] = '\0';
    pt += 3;

    pt2 = strchr(pt,'/');
    if ( pt2==NULL ) {
	pt2 = pt+strlen(pt);
	path = copy("/");
    } else {
	path = copy(pt2);
    }

    upt = strchr(pt,'@');
    if ( upt!=NULL && upt<pt2 ) {
	ppt = strchr(pt,':');
	if ( ppt==NULL ) {
	    *username = copyn(pt,upt-pt);
	} else {
	    *username = copyn(pt,ppt-pt);
	    *password = copyn(ppt+1,upt-ppt-1);
	}
	pt = upt+1;
    }

    ppt = strchr(pt,':');
    if ( ppt!=NULL && ppt<pt2 ) {
	char *temp = copyn(ppt+1,pt2-ppt-1), *end;
	*port = strtol(temp,&end,10);
	if ( *end!='\0' )
	    *port = -2;
	free(temp);
	pt2 = ppt;
    }
    *host = copyn(pt,pt2-pt);
    if ( *username ) {
	*password = GIO_PasswordCache(proto,*host,*username,*password);
	if ( *password==NULL ) {
	    *password = ff_ask_string(_("Password?"),"",_("Enter password for %s@%s"), *username, *host );
	    *password = GIO_PasswordCache(proto,*host,*username,*password);
	}
    }
return( path );
}

static FILE *HttpURLToTempFile(char *url) {
    struct sockaddr_in addr;
    char *pt, *host, *filename, *username, *password;
    int port;
    FILE *ret;
    char buffer[300];
    int first, code;
    int soc;
    int datalen, len;
    char *databuf;

    snprintf(buffer,sizeof(buffer),_("Downloading from %s"), url);

    if ( strncasecmp(url,"http://",7)!=0 ) {
	ff_post_error(_("Could not parse URL"),_("Got something else when expecting an http URL"));
return( NULL );
    }
    filename = decomposeURL(url, &host, &port, &username, &password);
    /* I don't support username/passwords for http */
    free( username ); free( password );

    ff_progress_start_indicator(0,_("Font Download..."),buffer,
	    _("Resolving host"),1,1);
    ff_progress_enable_stop(false);
    ff_progress_allow_events();
    ff_progress_allow_events();

    if ( !findHTTPhost(&addr, host, port)) {
	ff_progress_end_indicator();
	ff_post_error(_("Could not find host"),_("Could not find \"%s\"\nAre you connected to the internet?"), host );
	free( host ); free( filename );
return( false );
    }
    soc = makeConnection(&addr);
    if ( soc==-1 ) {
	ff_progress_end_indicator();
	ff_post_error(_("Could not connect to host"),_("Could not connect to \"%s\"."), host );
	free( host ); free( filename );
return( false );
    }

    datalen = 8*8*1024;
    databuf = galloc(datalen+1);

    ChangeLine2_8(_("Requesting font..."));
    sprintf( databuf,"GET %s HTTP/1.1\r\n"
	"Host: %s\r\n"
	"User-Agent: FontForge\r\n"
	"Connection: close\r\n\r\n", filename, host );
    if ( write(soc,databuf,strlen(databuf))==-1 ) {
	ff_progress_end_indicator();
	ff_post_error(_("Could not send request"),_("Could not send request to \"%s\"."), host );
	close( soc );
	free( databuf );
	free( host ); free( filename );
return( NULL );
    }

    ChangeLine2_8(_("Downloading font..."));

    ret = tmpfile();

    first = 1;
    code = 404;
    while ((len = read(soc,databuf,datalen))>0 ) {
	if ( first ) {
	    sscanf(databuf,"HTTP/%*f %d", &code );
	    first = 0;
	    /* check for redirects */
	    if ( code>=300 && code<399 && (pt=strstr(databuf,"Location: "))!=NULL ) {
		char *newurl = pt + strlen("Location: ");
		pt = strchr(newurl,'\r');
		if ( *pt )
		    *pt = '\0';
		close( soc );
		fclose(ret);
		free(host); free( filename );
		ret = URLToTempFile(newurl);
		free(databuf);
return( ret );
	    }
	    pt = strstr(databuf,"Content-Length: ");
	    if ( pt!=NULL ) {
		pt += strlen( "Content-Length: ");
		ff_progress_change_total(strtol(pt,NULL,10));
	    }
	    pt = strstr(databuf,"\r\n\r\n");
	    if ( pt!=NULL ) {
		pt += strlen("\r\n\r\n");
		fwrite(pt,1,len-(pt-databuf),ret);
		ff_progress_increment(len-(pt-databuf));
	    }
	} else {
	    fwrite(databuf,1,len,ret);
	    ff_progress_increment(len);
	}
    }
    ff_progress_end_indicator();
    close( soc );
    free( databuf );
    free( host ); free( filename );
    if ( len==-1 ) {
	ff_post_error(_("Could not download data"),_("Could not download data.") );
	fclose(ret);
return( NULL );
    } else if ( code<200 || code>299 ) {
	ff_post_error(_("Could not download data"),_("HTTP return code: %d."), code );
	fclose(ret);
return( NULL );
    }
    rewind(ret);
return( ret );
}

static FILE *FtpURLToTempFile(char *url) {
    struct sockaddr_in addr, data_addr;
    char *host, *filename, *username, *password;
    int port;
    FILE *ret;
    char buffer[300];
    int soc, data;
    int datalen, len;
    char *databuf, *cmd;

    snprintf(buffer,sizeof(buffer),_("Downloading from %s"), url);

    if ( strncasecmp(url,"ftp://",6)!=0 ) {
	ff_post_error(_("Could not parse URL"),_("Got something else when expecting an ftp URL"));
return( NULL );
    }
    filename = decomposeURL(url, &host, &port, &username, &password);

    ff_progress_start_indicator(0,_("Font Download..."),buffer,
	    _("Resolving host"),1,1);
    ff_progress_enable_stop(false);
    ff_progress_allow_events();
    ff_progress_allow_events();

    if ( !findFTPhost(&addr, host, port)) {
	ff_progress_end_indicator();
	ff_post_error(_("Could not find host"),_("Could not find \"%s\"\nAre you connected to the internet?"), host );
	free( host ); free( filename );
return( NULL );
    }
    soc = makeConnection(&addr);
    if ( soc==-1 ) {
	ff_progress_end_indicator();
	ff_post_error(_("Could not connect to host"),_("Could not connect to \"%s\"."), host );
	free( host ); free( filename );
return( NULL );
    }

    datalen = 8*8*1024;
    databuf = galloc(datalen+1);
    cmd = databuf;

    ChangeLine2_8(_("Logging in..."));
    if ( getresponse(soc,databuf,datalen) == -1 ) {		/* ftp servers say "Hi" when then connect */
	ff_progress_end_indicator();
	ff_post_error(_("Could not connect to host"),_("Could not connect to \"%s\"."), host );
	free( host ); free( filename ); free(username); free(password);
	close( soc );
return( NULL );
    }
    free( host );

    if ( username==NULL ) {
	username=copy("anonymous");
	if ( password==NULL )
	    password=copy("FontForge");
    } else if ( password==NULL )
	password = copy("");
    
    sprintf(cmd,"USER %s\r\n", username);
    if ( ftpsendr(soc,cmd,databuf,datalen)== -1 ) {
	ff_progress_end_indicator();
	close( soc ); free(filename); free(databuf); free(username); free(password);
return( NULL );
    }
    sprintf(cmd,"PASS %s\r\n", password);
    free(username); free(password);
    if ( ftpsendr(soc,cmd,databuf,datalen)<=0 ) {
	ff_progress_end_indicator();
	LogError(_("Bad Username/Password\n"));
	close( soc ); free(filename); free(databuf);
return( NULL );
    }

    if ( ftpsendr(soc,"TYPE I\r\n",databuf,datalen)==-1 ) {	/* Binary */
	ff_progress_end_indicator();
	close( soc ); free(filename); free(databuf);
return( NULL );
    }

    ChangeLine2_8(_("Requesting font..."));
    if ( ftpsendpassive(soc,&data_addr,databuf,datalen)<= 0 ) {
	ff_progress_end_indicator();
	close( soc ); free(filename); free(databuf);
return( NULL );
    }
    if (( data = socket(PF_INET,SOCK_STREAM,0))==-1 ||
	    connect(data,(struct sockaddr *) &data_addr,sizeof(data_addr))== -1 ) {
	ff_progress_end_indicator();
	if ( data!=-1 )
	    close(data);
	close( soc ); free(filename); free(databuf);
	LogError(_("FTP passive Data Connect failed\n"));
return( 0 );
    }

    sprintf(cmd,"RETR %s\r\n", filename);
    if ( ftpsendr(soc,cmd, databuf, datalen)<=0 ) {
	ff_progress_end_indicator();
	ff_post_error(_("Could not download data"),_("Could not find file.") );
	close(data);
	close( soc ); free(filename); free(databuf);
return( ret );
    }

    ChangeLine2_8(_("Downloading font..."));

    ret = tmpfile();

    while ((len = read(data,databuf,datalen))>0 ) {
	fwrite(databuf,1,len,ret);
    }
    close( soc ); close( data );
    free( databuf );
    free( filename );
    if ( len==-1 ) {
	ff_post_error(_("Could not download data"),_("Could not download data.") );
	fclose(ret);
return( NULL );
    }
    rewind(ret);
return( ret );
}

FILE *URLToTempFile(char *url) {

    if ( strncasecmp(url,"http://",7)==0 )
return( HttpURLToTempFile(url));
    else if ( strncasecmp(url,"ftp://",6)==0 )
return( FtpURLToTempFile(url));
    else {
	ff_post_error(_("Could not parse URL"),_("FontForge only handles ftp and http URLs at the moment"));
return( NULL );
    }
}
