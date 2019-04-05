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
#include "gioftpP.h"
#include "ustring.h"
#include "utype.h"
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>

#if defined(__MINGW32__)
void* GIO_dispatch(GIOControl* gc) { return 0;}
void GIO_cancel(GIOControl* gc) {}
void GIO_init(void* handle, struct stdfuncs* _stdfuncs, int index) {}
void GIO_term(void) {}
#else

#include <sys/socket.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL        0x0		/* linux man page for "send" implies MSG_NOSIGNAL is in sys/socket.h, but it ain't implemente for Mac and Solaris */
#endif
#include <errno.h>

int ftp_protocol_index;
struct stdfuncs *stdfuncs;
void *ftplib_handle;

static unichar_t err401[] = { ' ','U','n','a','u','t','h','o','r','i','z','e','d', '\0' };
static unichar_t err404[] = { ' ','N','o','t',' ','F','o','u','n','d', '\0' };
/*static unichar_t err501[] = { ' ','N','o','t',' ','I','m','p','l','e','m','e','n','t','e','d', '\0' };*/

static void set_status(GIOControl *gc, char *prefix, char *rest) {
    int len = strlen(prefix);
    uc_strncpy(gc->status,prefix,sizeof(gc->status)-1);
    if ( len<sizeof(gc->status) && rest!=NULL )
	uc_strncpy(gc->status+len,rest,sizeof(gc->status)-1-len);
}

/* this excedingly complex routine tries to send data on a socket. */
/*  first it waits until the socket is ready to accept data (there is no time-*/
/*  out on send/write), checking to see if the user has aborted the operation */
/*  if an interrupt happens which kills the select call then restart the call */
/*  if we get an select error we assume the connection died. If we time-out we*/
/*  essentially assume the same. If the send fails, ditto. If everything succedes */
/*  record that we sent the command */
static int ftpsend(GIOControl *gc,int ctl, char *cmd) {
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
	    uc_strcpy(gc->status, "Connection closed by foreign host");
	    gc->return_code = 600;
return( -1 );
	} else if ( gc->abort ) {
return( -1 );
	} else if ( ret>0 )
    break;
	++i;
    }
    if ( ret==0 ) {
	uc_strcpy(gc->status, "Connection timed out");
	gc->return_code = 601;
return( -1 );
    }
    if ( send(ctl,cmd,strlen(cmd),MSG_NOSIGNAL)<=0 ) {
	if ( errno==EINTR )	/* interrupted, try again */
  goto restart;
	uc_strcpy(gc->status, "FTP connection closed by foreign host");
	gc->return_code = 600;
return( -1 );
    }
    if ( stdfuncs->reportheaders!=NULL )
	(stdfuncs->reportheaders)( "%s", cmd );
return( true );
}

/* Similar thing for read */
static int getresponse(GIOControl *gc,int ctl) {
    struct timeval tv;
    fd_set rds;
    int len;
    int ret=false, i;
    struct ftpconnectiondata *ftp = (struct ftpconnectiondata *) (gc->connectiondata);
    char *pt;

  restart:
    while ( 1 ) {
	if ( ftp->plen<ftp->rlen ) {
	    pt = strchr(ftp->buffer+ftp->plen,'\n');
	    if ( pt!=NULL ) {
		int ch = pt[1]; pt[1] = '\0';
		if ( stdfuncs->reportheaders!=NULL )
		    (stdfuncs->reportheaders)( "%s", ftp->buffer+ftp->plen );
		uc_strncpy(gc->status,ftp->buffer+ftp->plen,sizeof(gc->status)/sizeof(gc->status[0])-1);
		gc->return_code = strtol(ftp->buffer+ftp->plen,NULL,0);
		pt[1]=ch;
		ftp->base = ftp->plen;
		ftp->plen = pt+1-ftp->buffer;
		if ( ftp->buffer[ftp->base+3]==' ' )
return( ftp->buffer[ftp->base]=='2' || ftp->buffer[ftp->base]=='1' );
    continue;
	    }
	    strcpy(ftp->buffer,ftp->buffer+ftp->plen);
	    ftp->rlen -= ftp->plen;
	    ftp->plen = 0;
	}
	FD_ZERO(&rds);
	i = 0;
	while ( i<60 ) {
	    FD_SET(ctl,&rds);
	    tv.tv_sec = 4; tv.tv_usec = 0;
	    if (( ret = select(ctl+1,&rds,NULL,NULL,&tv))<0 ) {
		if ( errno==EINTR )
  goto restart;
		uc_strcpy(gc->status, "Connection closed by foreign host");
		gc->return_code = 600;
return( -1 );
	    } else if ( gc->abort ) {
return( -1 );
	    } else if ( ret>0 )
	break;
	    ++i;
	}
	if ( ret==0 ) {
	    uc_strcpy(gc->status, "Connection timed out");
	    gc->return_code = 601;
return( -1 );
	}
	len = read(ctl,ftp->buffer+ftp->rlen,sizeof(ftp->buffer)-1-ftp->rlen);
	if ( len==0 ) {
	    uc_strcpy(gc->status, "Connection closed by foreign host");
	    gc->return_code = 600;
return( -1 );
	}
	ftp->buffer[ftp->rlen+len]='\0';
	ftp->rlen += len;
    }
}

static int ftpsendr(GIOControl *gc,int ctl, char *cmd) {

    if ( ftpsend(gc,ctl,cmd)==-1 )
return( -1 );
return( getresponse(gc,ctl));
}

static int ftpsendpassive(GIOControl *gc,int ctl, struct sockaddr_in *data_addr) {
    char *pt;
    struct ftpconnectiondata *ftp = (struct ftpconnectiondata *) (gc->connectiondata);

    if ( ftpsend(gc,ctl,"PASV\r\n")==-1 )
return( -1 );
    if ( getresponse(gc,ctl)== -1 )
return( -1 );

    if ( gc->return_code == 227 ) {
	/* 227 Entering Passive Mode (h1,h2,h3,h4,p1,p2) */
	int h1,h2,h3,h4,p1,p2;
	for ( pt=ftp->buffer+ftp->base+4; !isdigit(*pt) && *pt!='\n' ; ++pt );
	if ( *pt=='\n' )
return( -2 );		/* can't parse it */
	sscanf(pt,"%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2 );
	data_addr->sin_family = AF_INET;
	data_addr->sin_port = htons((p1<<8)|p2);
	data_addr->sin_addr.s_addr = htonl((h1<<24)|(h2<<16)|(h3<<8)|h4);
return( 1 );
    } else if ( ftp->buffer[ftp->base]=='4' || ftp->buffer[ftp->base]=='5' )
return( 0 );

return( 0 );	/* shouldn't get here */
}

typedef GDirEntry *(*dirparser)(char *line, GDirEntry *last, int tzdiff);

static GDirEntry *parsedosdir(char *line, GDirEntry *last, int tzdiff) {
    GDirEntry *cur;
    int m,d,y,h,min; char ampm;
    struct tm when;
    char *pt, *end;

    cur = calloc(1,sizeof(GDirEntry));
    cur->hasdir = 1;
    cur->hasexe = 0;
    cur->haslnk = 0;
    cur->hasmode = 0;
    cur->hassize = 1;
    cur->hastime = 1;
    cur->mode = 644;

    sscanf(line,"%d-%d-%d %d:%d%cM", &m, &d, &y, &h, &min, &ampm );
    when.tm_mon = m-1;
    when.tm_mday = d;
    if ( y<70 ) y+=100;
    when.tm_year = y;
    when.tm_hour = h+(ampm=='P'?12:0);
    if ( tzdiff!= -9999 )
	when.tm_hour += tzdiff;
    when.tm_min = min;
    when.tm_sec = 0;
    when.tm_yday = when.tm_wday = 0; when.tm_isdst = -1;
    cur->modtime = mktime(&when);

    pt = strchr(line,'M')+1;
    while ( *pt==' ' || *pt=='\t' ) ++pt;
    if ( strncmp(pt,"<DIR>",5)==0 ) {
	cur->isdir = 1;
	cur->mode |= S_IFDIR;
	cur->size = 0;
	pt += 5;
    } else {
	cur->size = strtol(pt,&end,10);
	pt = end;
    }

    while ( *pt==' ' || *pt=='\t' ) ++pt;
    if ( pt>line+39 ) pt = line+39;
    cur->name = uc_copy(pt);
    if ( last!=NULL )
	last->next = cur;
return( cur );
}

static GDirEntry *parseunix_ldir(char *line, GDirEntry *last, int tzdiff) {
    GDirEntry *cur;
    int m,d,y,h,min;
    struct tm when;
    char *pt, *end;
    static char *mons[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", NULL };
/*
ls -l comes in various shapes and sizes
sometimes we get . and .., sometimes we don't
sometimes whe get a group, sometimes we don't, sometimes it's a number
dates sometimes include a year but no time, or a time but no year.

dr-xr-xr-x   8 root     0            512 Jul  7 17:09 .
-r--r--r--   1 root     other    45486151 Jul 20 02:19 IAFA-LISTINGS
drwxr-xr-x   2 gww      gww          1024 Jul  6  1999 whales
-rw-rw----   1 mail       32474 Jul 20 10:01 Mailbox
drwxr-xr-x   7 1            512 Jul 19 22:12 Apple_Support_Area
*/

    if ( strncmp(line,"total ",6)==0 && strtol(line+6,&end,10)>=0 && *end=='\0' )
return( last );

    cur = calloc(1,sizeof(GDirEntry));
    cur->hasdir = 1;
    cur->hasexe = 1;
    cur->haslnk = 1;
    cur->hasmode = 1;
    cur->hassize = 1;
    cur->hastime = 1;

    cur->mode = 0;
    if ( *line=='d' ) { cur->mode |= S_IFDIR; cur->isdir = 1; }
    #if !defined(__MINGW32__)
    if ( *line=='l' ) {
	cur->mode |= S_IFLNK;
	pt = strstr(line," -> ");
	if ( pt!=NULL ) *pt = '\0';
	cur->islnk = 1;
    }
    #endif
    if ( line[1]=='r' ) cur->mode |= 0400;
    if ( line[2]=='w' ) cur->mode |= 0200;
    if ( line[3]=='x' ) { cur->mode |= 0100; if ( !cur->isdir ) cur->isexe = 1; }
    if ( line[4]=='r' ) cur->mode |= 040;
    if ( line[5]=='w' ) cur->mode |= 020;
    if ( line[6]=='x' ) cur->mode |= 010;
    if ( line[7]=='r' ) cur->mode |= 04;
    if ( line[8]=='w' ) cur->mode |= 02;
    if ( line[9]=='x' ) cur->mode |= 01;

    for ( pt = line+10; *pt==' '; ++pt );
    strtol(pt,&end,10);			/* skip over the number of links */
    for ( pt = end; *pt==' '; ++pt );
    while ( *pt!=' ' && *pt!='\0' ) ++pt;	/* skip over the user */
    while ( *pt==' ' ) ++pt;
    if ( !isdigit( *pt )) {
	while ( *pt!=' ' && *pt!='\0' ) ++pt;	/* skip over the group */
	cur->size = strtol(pt,&end,10);
    } else {
	long temp = strtol(pt,&end,10);
	for ( pt = end; *pt==' '; ++pt );
	if ( !isdigit(*pt)) {
	    /* Then we don't have a group, and pt now points to the month */
	    cur->size = temp;
	    end = pt;
	} else {
	    /* Then we had a numeric group, and pt now points to the size */
	    cur->size = strtol(pt,&end,10);
	}
    }

    for ( pt=end; *pt==' '; ++pt );
    for ( m=0; m<12; ++m )
	if ( strncmp(pt,mons[m],3)==0 )
    break;
    d = strtol(pt+3,&end,10);
    if ( end[3]!=':' ) {
	y = strtol(end,&end,10)-1900;
	h = min = 0;
    } else {
	time_t now; struct tm *tm;
	h = strtol(end,&end,10);
	min = strtol(end+1,&end,10);
	time(&now);
	tm = gmtime(&now);
	if ( m>tm->tm_mon )
	    y = tm->tm_year-1;
	else
	    y = tm->tm_year;
    }
    when.tm_mon = m;
    when.tm_mday = d;
    when.tm_year = y;
    if ( tzdiff!= -9999 )
	h += tzdiff;
    when.tm_hour = h;
    when.tm_min = min;
    when.tm_sec = 0;
    when.tm_yday = when.tm_wday = 0; when.tm_isdst = -1;
    cur->modtime = mktime(&when);

    cur->name = uc_copy(end+1);
    if ( last!=NULL )
	last->next = cur;
return( cur );
}

static GDirEntry *parseunix_Fdir(char *line, GDirEntry *last, int tzdiff) {
    GDirEntry *cur;
    char *pt;

    cur = calloc(1,sizeof(GDirEntry));
    cur->hasdir = 1;
    cur->hasexe = 1;
    cur->haslnk = 1;
    cur->hasmode = 0;
    cur->hassize = 0;
    cur->hastime = 0;
    cur->mode = 644;

    pt = line+strlen(line)-1;
    if ( *pt == '/' ) {
	cur->isdir = 1;
	*pt = '\0';
    } else if ( *pt=='*' ) {
	cur->isexe = 1;
	*pt = '\0';
    } else if ( *pt=='@' ) {
	cur->islnk = 1;
	*pt = '\0';
    } else if ( *pt=='|' || *pt=='=' ) {
	/* sockets and named pipes */
	*pt = '\0';
    }
    cur->name = uc_copy(pt);
    if ( last!=NULL )
	last->next = cur;
return( cur );
}

static dirparser guessdirtype(char *line) {
    char *end;

    if ( isdigit(line[0]) && isdigit(line[1]) && line[2]=='-' &&
	 isdigit(line[3]) && isdigit(line[4]) && line[5]=='-' &&
	 isdigit(line[6]) && isdigit(line[7]) && line[8]==' ' && line[9]==' ' &&
	 isdigit(line[10]) && isdigit(line[11]) && line[12]==':' &&
	 isdigit(line[13]) && isdigit(line[14]) && line[16]=='M' && line[17]==' ' )
return( parsedosdir );

    /* It's probably unix-l, and this line is junk, so let's check the next line */
    if ( strncmp(line,"total ",6)==0 && strtol(line+6,&end,10)>=0 && *end=='\0' )
return( NULL );

    if ( (line[0]=='-' || line[0]=='d' || line[0]=='l' || line[0]=='p' || line[0]=='b' || line[0]=='c' || line[0]=='s' ) &&
	(line[1]=='-' || line[1]=='r' ) &&
	 (line[2]=='-' || line[2]=='w' ) &&
	 (line[3]=='-' || line[3]=='x' || line[3]=='s' || line[3]=='S' || line[3]=='t' || line[3]=='T' ) &&
        (line[4]=='-' || line[4]=='r' ) &&
	 (line[5]=='-' || line[5]=='w' ) &&
	 (line[6]=='-' || line[6]=='x' || line[6]=='s' || line[6]=='S' || line[6]=='t' || line[6]=='T' ) &&
        (line[7]=='-' || line[7]=='r' ) &&
	 (line[8]=='-' || line[8]=='w' ) &&
	 (line[9]=='-' || line[9]=='x' || line[9]=='s' || line[9]=='S' || line[9]=='t' || line[9]=='T' ) &&
	strlen(line)>30 )
return( parseunix_ldir );

return( parseunix_Fdir );
}

static int setnopipe(int soc) {
#if MSG_NOSIGNAL==0 && defined(SO_NOSIGPIPE)
    int value=1;
    socklen_t len = sizeof(value);
    
    if ( soc==-1 )
return( -1 );

return( setsockopt(soc,SOL_SOCKET,SO_NOSIGPIPE,&value,len));
#else
return( 0 );
#endif
}

static int ftpgetdir(GIOControl *gc,int ctl,char *dirname,int tzdiff) {
    struct sockaddr_in data_addr;
    int data, len, ret, i;
    char *buf, *line, *bpt, *start, *lpt;
    GDirEntry *head=NULL, *last=NULL;
    dirparser parser;
    struct timeval tv;
    fd_set rds;

/* Sadly on some systems "LIST ." is different from "LIST". The first does not
  give us files beginning with . and it also opens all sub-directories and lists
  them. This is not useful. Hence the complication of cding, and then dir
*/
    buf = malloc(1025);
    line = malloc(1025);

    sprintf( buf, "CWD %s\r\n", dirname);
    if ( (ret=ftpsendr(gc,ctl,buf))<= 0 ) {
	/* if we can't cd to it, we can't look at it */
	free(buf); free(line);
	if ( ret==0 ) gc->return_code = 401;		/* Unauthorized */
return( ret );
    }
    if ( (ret = ftpsendpassive(gc,ctl,&data_addr))<= 0 ) {
	free(buf); free(line);
return( ret );
    }
    if (( data = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP))==-1 ||
	    setnopipe(data)==-1 ||
	    connect(data,(struct sockaddr *) &data_addr,sizeof(data_addr))== -1 ) {
	if ( data!=-1 )
	    close(data);
	free(buf); free(line);
	gc->return_code = 602;
	uc_strcpy(gc->status,"FTP Data Connect failed" );
return( 0 );
    }

    if ( (ret = ftpsendr(gc,ctl,"LIST\r\n"))<=0 ) {
	close(data); free(buf); free(line);
return( ret );
    }

    lpt = line;
    while ( 1 ) {
	FD_ZERO(&rds);
	i = 0;
	while ( i<60 ) {
  restart:
	    FD_SET(data,&rds);
	    tv.tv_sec = 4; tv.tv_usec = 0;
	    if (( ret = select(data+1,&rds,NULL,NULL,&tv))<0 ) {
		if ( errno==EINTR )
  goto restart;
		close(data); free(buf); free(line);
		(stdfuncs->FreeDirEntries)(last);
		uc_strcpy(gc->status, "Connection closed by foreign host");
		gc->return_code = 600;
return( -1 );
	    } else if ( gc->abort ) {
		(stdfuncs->FreeDirEntries)(last);
		close(data); free(buf); free(line);
return( -1 );
	    } else if ( ret>0 )
	break;
	    ++i;
	}
	if ( ret==0 ) {
	    (stdfuncs->FreeDirEntries)(last);
	    close(data); free(buf); free(line);
	    uc_strcpy(gc->status, "Connection timed out");
	    gc->return_code = 601;
return( -1 );
	}
	if (( len=read(data,buf,1024))<=0 )
    break;
	buf[len]='\0';
	start = buf;
	while (( bpt = strchr(start,'\n'))!=NULL ) {
	    strncpy(lpt,start,bpt-start);
	    lpt[bpt-start]= '\0';
	    if ( lpt[bpt-start-1]=='\r' )
		lpt[bpt-start-1]= '\0';
	    if ( parser==NULL )
		parser = guessdirtype(line);
	    if ( parser!=NULL && *line )
		last = (*parser)(line,last,tzdiff);
	    if ( head==NULL ) head = last;
	    lpt = line;
	    start = bpt+1;
	}
	strcpy(line,start);
	lpt = line+strlen(line);
	if ( head!=NULL && gc->receiveintermediate ) {
	    /* report intermediate results */
	    gc->iodata = head;
	    (stdfuncs->PostInter)(gc);
	}
    }
    close(data);
    if ( len==-1 ) {
	(stdfuncs->FreeDirEntries)(last);
	free(buf); free(line);
return( -1 );
    }
    getresponse(gc,ctl);
    gc->done = true;
    gc->iodata = head;
    gc->direntrydata = true;
    free(buf); free(line);
return( 1 );
}

static int ftpmkdir(GIOControl *gc,int ctl,char *dirname) {
    char buf[300];
    sprintf( buf, "MKD %s\r\n", dirname);
return( ftpsendr(gc,ctl,buf));
}

static int ftpdeldir(GIOControl *gc,int ctl,char *dirname) {
    char buf[300];
    sprintf( buf, "RMD %s\r\n", dirname);
return( ftpsendr(gc,ctl,buf));
}

static int ftpdelfile(GIOControl *gc,int ctl,char *filename) {
    char buf[300];
    sprintf( buf, "DELE %s\r\n", filename);
return( ftpsendr(gc,ctl,buf));
}

static int ftprenamefile(GIOControl *gc,int ctl,char *oldname,char *newname) {
    char buf[300];
    int ret;

    sprintf( buf, "RNFR %s\r\n", oldname);
    if ( (ret = ftpsendr(gc,ctl,buf))<=0 )
return( ret );
    sprintf( buf, "RNTO %s\r\n", newname);
return( ftpsendr(gc,ctl,buf));
}

void *GIO_dispatch(GIOControl *gc) {
    char *username, *password, *host, *path, *temppath;
    int port;
    struct sockaddr_in locaddr;
    struct hostdata *addr;
    int ctl= -1, ret;
    char cmd[300];
    struct ftpconnectiondata *ftp;
    int tzoff= -9999;
    struct ftphostaccessdata *had;

    path = (stdfuncs->decomposeURL)(gc->path,&host,&port,&username,&password);
    if ( host==NULL || path==NULL ) {
	gc->return_code = 404;
	gc->error = err404;
	set_status(gc,"Bad URL","");
  goto leave;
    }

    set_status(gc,"Looking up ",host);
    addr = (stdfuncs->LookupHost)(host);
#ifdef IPPORT_FTP
    if ( port==-1 ) port = IPPORT_FTP;
#else
    if ( port==-1 ) port = 21;
#endif
    if ( addr==NULL ) {
	gc->return_code = 404;
	gc->error = err404;
	set_status(gc,"Bad Hostname: ",host);
	(stdfuncs->PostError)(gc);
    } else {
	ftp = calloc(1,sizeof(struct ftpconnectiondata));
	gc->connectiondata = (struct gio_connectiondata *) ftp;
	pthread_mutex_lock(&stdfuncs->hostacccess_mutex);
	for ( had=(struct ftphostaccessdata *) (addr->had); had!=NULL &&
		(had->port!=port || had->protocol_index!=ftp_protocol_index);
		had=had->next );
	if ( had==NULL ) {
	    had = calloc(1,sizeof(struct ftphostaccessdata));
	    had->port = port;
	    had->protocol_index = ftp_protocol_index;
	    had->tzoff = -9999;
	}
	pthread_mutex_unlock(&stdfuncs->hostacccess_mutex);
	ftp->ctl = ctl = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
	setnopipe(ctl);
	locaddr = addr->addr;
	locaddr.sin_port = htons(port);
	if ( connect(ctl,(struct sockaddr *) &locaddr,sizeof(addr->addr))== -1 ) {
	    gc->return_code = 404;
	    gc->error = err404;
	    set_status(gc,"Connection to host failed: ",host);
	    (stdfuncs->PostError)(gc);
	} else {
	    if ( getresponse(gc,ctl)<=0 )
  goto leave;
	    if ( username==NULL ) {
		username=copy("anonymous");
		if ( password==NULL )
		    password=copy(stdfuncs->useragent);
	    } else if ( password==NULL )
		password = copy("");
	    sprintf(cmd,"USER %s\r\n", username);
	    if ( ftpsendr(gc,ctl,cmd)== -1 )
  goto leave;
	    sprintf(cmd,"PASS %s\r\n", password);
	    ret = ftpsendr(gc,ctl,cmd);
	    if ( ret==-1 )
  goto leave;
	    if ( ret==0 ) {
		gc->return_code = 401;
		gc->error = err401;
		set_status(gc,"Bad username/password","");
  goto leave;
	    }

	    if ( ftpsendr(gc,ctl,"TYPE I\r\n")==-1 )
  goto leave;
	}
	temppath = path;
	switch ( gc->gf ) {
	  case gf_dir:
	    ret = ftpgetdir(gc,ctl,temppath,tzoff);
	  break;
	  case gf_statfile:
	    ret = ftpgetdir(gc,ctl,temppath,tzoff);
	  break;
	  case gf_mkdir:
	    ret = ftpmkdir(gc,ctl,temppath);
	  break;
	  case gf_deldir:
	    ret = ftpdeldir(gc,ctl,temppath);
	  break;
	  case gf_delfile:
	    ret = ftpdelfile(gc,ctl,temppath);
	  break;
	  case gf_renamefile: {
	    char *topath, *tohost, *tousername, *topassword; int toport;
	    topath = (stdfuncs->decomposeURL)(gc->topath,&tohost,&toport,&tousername,&topassword);
	    ret = ftprenamefile(gc,ctl,path,topath);
	    free(topath); free(tohost); free(tousername); free(topassword);
	  } break;
	}
    }
    if ( ret<=0 )
  goto leave;
    (stdfuncs->PostSuccess)(gc);
    if ( ctl!=-1 ) close(ctl);
    free(path);
    free(host); free(username); free(password);
return( NULL );
  leave:
    gc->done = true;
    (stdfuncs->PostError)(gc);
    if ( ctl!=-1 ) close(ctl);
    free(path);
    free(host); free(username); free(password);
return( NULL );
}

void GIO_cancel(GIOControl *gc) {
    free(gc->connectiondata);
}

void GIO_init(void *handle,struct stdfuncs *_stdfuncs,int index) {
    ftplib_handle = handle;
    stdfuncs = _stdfuncs;
    ftp_protocol_index = index;
}

void GIO_term(void) {
}
#endif /* !__MINGW32__ */
