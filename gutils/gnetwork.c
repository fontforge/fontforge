/* Copyright (C) 2013 by Ben Martin */
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

#include "inc/gnetwork.h"
#include "inc/ustring.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#if defined(__MINGW32__)
#  include <winsock2.h>
#  include <windows.h>
#  include <plibc.h>
#else
extern int h_errno;
#  include <netdb.h>
#  include <arpa/inet.h>
#endif



char* getNetworkAddress( char* outstring )
{
    char hostname[PATH_MAX+1];
    int rc = 0;

    rc = gethostname( hostname, PATH_MAX );
    if( rc == -1 )
    {
	return 0;
    }
    printf("hostname: %s\n", hostname );
    struct hostent * he = gethostbyname( hostname );
    if( !he )
    {
	return 0;
    }
    if( he->h_addrtype != AF_INET && he->h_addrtype != AF_INET6 )
    {
	return 0;
    }
    
    inet_ntop( he->h_addrtype, he->h_addr_list[0],
	       outstring, IPADDRESS_STRING_LENGTH_T-1 );

    return outstring;
}

char* HostPortPack( char* hostname, int port )
{
    static char ret[PATH_MAX+1];
    snprintf(ret,PATH_MAX,"%s:%d",hostname,port);
    return ret;
}

char* HostPortUnpack( char* packed, int* port, int port_default )
{
    char* colon = str_rfind( packed, ':' );
    if( !colon )
    {
	*port = port_default;
	return packed;
    }

    *colon = '\0';
    *port = atoi(colon+1);
    return packed;
}



