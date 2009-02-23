/* Copyright (C) 2005-2009 by George Williams */
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

#include <basics.h>

/* Parse GNU .la "library" files. They give us the name to use with dlopen */
/* Currently only important on _Cygwin */
#if defined(__CygWin) && !defined(NODYNAMIC)
#  include <dynamic.h>
#  include <stdio.h>
#  include <string.h>
#  include <stdlib.h>

static char *LaInDir(char *dir,const char *filename,char *buffer, int buflen) {
    char *pt, *dpt;
    FILE *test;
    char dllname[1025];

    if ( dir==NULL ) {
	strncpy(buffer,filename,buflen-3);
	buffer[buflen-3] = '\0';
    } else
	snprintf(buffer,buflen-3,"%s/%s", dir,filename);
    dpt = strrchr(buffer,'/');
    if ( dpt==NULL )
	pt = strrchr(buffer,'.');
    else
	pt = strrchr(dpt+1,'.');
    if ( pt==NULL )
	strcat(buffer,".la");
    else
	strcpy(pt,".la");

    test = fopen(buffer,"r");
    if ( test==NULL )
return( NULL );
    while ( fgets(buffer,buflen,test)!=NULL ) {
	if ( strncmp(buffer,"dlname='",8)==0 ) {
	    pt = strrchr(buffer+8,'\'');
	    if ( pt!=NULL ) {
		*pt = '\0';
		fclose(test);
		if ( buffer[8]=='\0' )
return( NULL );			/* No dlopenable version */
		if ( buffer[8]=='/' )
return( buffer+8 );
		strcpy(dllname,buffer+8);
		if ( dir==NULL ) {
		    pt = strrchr(filename,'/');
		    if ( pt==NULL )
return( buffer+8 );
		    snprintf(buffer,buflen,"%.*s/%s", pt-filename, filename, dllname );
		} else
		    snprintf(buffer,buflen,"%s/%s", dir,dllname);
return( buffer );
	    }
	}
    }
    fclose(test);
return( NULL );
}

#ifdef dlopen
# undef dlopen
#endif

void *libtool_laopen(const char *filename, int flags) {
    char *ret;
    char buffer[1025];
    char dirbuf[1025];
    char *path, *pt;

    if ( filename==NULL )
return( dlopen(NULL,flags));	/* Return magic handle to main program */

    if ( strchr(filename,'/')!=NULL )
	ret = LaInDir(NULL,filename,buffer,sizeof(buffer));
    else {
	ret = NULL;
	path = getenv("LD_LIBRARY_PATH");
	if ( path!=NULL ) {
	    while ( (pt=strchr(path,':'))!=NULL ) {
		strncpy(dirbuf,path,pt-path);
		dirbuf[pt-path] = '\0';
		ret = LaInDir(dirbuf,filename,buffer,sizeof(buffer));
		if ( ret!=NULL )
	    break;
		path = pt+1;
	    }
	    if ( ret==NULL )
		ret = LaInDir(path,filename,buffer,sizeof(buffer));
	}
	/* I should search /etc/ld.so.cache here, but I can't parse it */
	if ( ret==NULL )
	    ret = LaInDir("/lib",filename,buffer,sizeof(buffer));
	if ( ret==NULL )
	    ret = LaInDir("/usr/lib",filename,buffer,sizeof(buffer));
	if ( ret==NULL )
	    ret = LaInDir("/usr/X11R6/lib",filename,buffer,sizeof(buffer));
	if ( ret==NULL )
	    ret = LaInDir("/usr/local/lib",filename,buffer,sizeof(buffer));
    }
    if ( ret!=NULL )
return( dlopen( ret,flags ));

return( dlopen(filename,flags) );	/* This will almost certainly fail, but it will provide an error for dlerror() */
}
#elif defined( __Mac )
    /* The mac now has normal dlopen routines */
#  include <dynamic.h>
#  include <stdio.h>
#  include <string.h>

void *gwwv_dlopen(char *name,int flags) {
#undef dlopen
    void *lib = dlopen(name,flags);
    char *temp;

    if (( lib!=NULL && lib!=(void *) -1) || name==NULL || *name=='/' )
return( lib );

    temp = galloc( strlen("/sw/lib/") + strlen(name) +1 );
    strcpy(temp,"/sw/lib/");
    strcat(temp,name);
    lib = dlopen(temp,flags);
    free(temp);
    if ( lib!=NULL && lib!=(void *) -1 )
return( lib );

    temp = galloc( strlen("/opt/local/lib/") + strlen(name) +1 );
    strcpy(temp,"/opt/local/lib/");
    strcat(temp,name);
    lib = dlopen(temp,flags);
    free(temp);
return( lib );
}
#elif defined( __Mac )
#  include <basics.h>
#  include <dynamic.h>
#  include <stdio.h>
#  include <string.h>

const void *gwwv_NSAddImage(char *name,uint32_t options) {
    const void *lib = NSAddImage(name,options);
    char *temp;

    if (( lib!=NULL && lib!=(void *) -1) || name==NULL || *name=='/' )
return( lib );

    temp = galloc( strlen("/sw/lib/") + strlen(name) +1 );
    strcpy(temp,"/sw/lib/");
    strcat(temp,name);
    lib = NSAddImage(temp,options);
    free(temp);
return( lib );
}
#endif
