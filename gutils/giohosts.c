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
#include "giofuncP.h"
#include "gfile.h"
#include "ustring.h"
#include "utype.h"

#if !defined(__MINGW32__)
#include <netdb.h>
#endif

char *_GIO_decomposeURL(const unichar_t *url,char **host, int *port, char **username,
	char **password) {
    unichar_t *pt, *pt2, *upt, *ppt;
    char *path;
    char proto[40];
    /* ftp://[user[:password]@]ftpserver[:port]/url-path */

    *username = NULL; *password = NULL; *port = -1;
    pt = uc_strstr(url,"://");
    if ( pt==NULL ) {
	*host = NULL;
return( cu_copy(url));
    }
    cu_strncpy(proto,url,(size_t)(pt-url)<sizeof(proto)?(size_t)(pt-url):sizeof(proto));
    pt += 3;

    pt2 = u_strchr(pt,'/');
    if ( pt2==NULL ) {
	pt2 = pt+u_strlen(pt);
	path = copy("/");
    } else {
	path = cu_copy(pt2);
    }

    upt = u_strchr(pt,'@');
    if ( upt!=NULL && upt<pt2 ) {
	ppt = u_strchr(pt,':');
	if ( ppt==NULL )
	    *username = cu_copyn(pt,upt-pt);
	else {
	    *username = cu_copyn(pt,ppt-pt);
	    *password = cu_copyn(ppt+1,upt-ppt-1);
	}
	pt = upt+1;
    }

    ppt = u_strchr(pt,':');
    if ( ppt!=NULL && ppt<pt2 ) {
	char *temp = cu_copyn(ppt+1,pt2-ppt-1), *end;
	*port = strtol(temp,&end,10);
	if ( *end!='\0' )
	    *port = -2;
	free(temp);
	pt2 = ppt;
    }
    *host = cu_copyn(pt,pt2-pt);
    if ( *username )
	*password = GIO_PasswordCache(proto,*host,*username,*password);
return( path );
}

struct passwd_cache {
    char *proto;
    char *host;
    char *username;
    char *password;
};
static int pc_cnt = 0, pc_max=0;
struct passwd_cache *pc = NULL;

char *GIO_PasswordCache(char *proto,char *host,char *username,char *password) {
    int i;
#ifdef HAVE_PTHREAD_H
    static pthread_mutex_t mymutex = PTHREAD_MUTEX_INITIALIZER;
#endif

    if ( proto==NULL || host==NULL || username==NULL )
return( password );

#ifdef HAVE_PTHREAD_H
	pthread_mutex_lock(&mymutex);
#endif

    for ( i=0; i<pc_cnt; ++i ) {
	if ( strcasecmp(proto,pc[i].proto)==0 &&
		strcasecmp(host,pc[i].host)==0 &&
		strcmp(username,pc[i].username)==0 ) {
	    if ( password==NULL ) {
		password = copy( pc[i].password );
 goto leave;
	    }
	    if ( strcmp(password,pc[i].password)!=0 ) {
		free( pc[i].password );
		pc[i].password = copy( password );
	    }
 goto leave;
	}
    }

    if ( password==NULL )
 goto leave;

    if ( pc_cnt>=pc_max )
	pc = realloc(pc,(pc_max+=10)*sizeof(struct passwd_cache));
    pc[pc_cnt].proto = copy( proto );
    pc[pc_cnt].host  = copy( host  );
    pc[pc_cnt].username = copy( username );
    pc[pc_cnt].password = copy( password );
    ++pc_cnt;
 leave:
#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&mymutex);
#endif

return( password );
}

/* simple hash tables */
static struct hostdata *names[26], *numbers[10];

struct hostdata *_GIO_LookupHost(char *host) {
#if defined(__MINGW32__)
return NULL;
#else

    struct hostdata **base, *cur;
#ifdef HAVE_PTHREAD_H
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#endif
    int i;

#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&mutex);
#endif
    if ( isdigit(host[0]))
	base = &numbers[host[0]-'0'];
    else if ( isupper(host[0]) && host[0]<127 )
	base = &names[host[0]-'A'];
    else if ( islower(host[0]) && host[0]<127 )
	base = &names[host[0]-'a'];
    else
	base = &names['z'-'a'];

    for ( cur= *base; cur!=NULL && strmatch(cur->hostname,host)!=0; cur = cur->next );
    if ( cur!=NULL ) {
#ifdef HAVE_PTHREAD_H
	pthread_mutex_unlock(&mutex);
#endif
return( cur );
    }

    cur = calloc(1,sizeof(struct hostdata));
    cur->addr.sin_family = AF_INET;
    cur->addr.sin_port = 0;
    if ( isdigit(host[0])) {
    	if ( !inet_aton(host,&cur->addr.sin_addr)) {
	    free(cur);
#ifdef HAVE_PTHREAD_H
	    pthread_mutex_unlock(&mutex);
#endif
return( NULL );
	}
    } else {
	struct hostent *he;
	he = gethostbyname(host);
	if ( he==NULL ) {
	    free(cur);
#ifdef HAVE_PTHREAD_H
	    pthread_mutex_unlock(&mutex);
#endif
return( NULL );
	}
	for ( i=0; he->h_addr_list[i]!=NULL; ++i );
	memcpy(&cur->addr.sin_addr,he->h_addr_list[rand()%(i+1)],he->h_length);
    }
    cur->hostname = copy(host);
    cur->next = *base;
    *base = cur;
#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&mutex);
#endif
return( cur );
#endif
}
