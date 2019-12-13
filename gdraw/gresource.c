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

#include <fontforge-config.h>

#include "charset.h"
#include "fontP.h"
#include "gdraw.h"
#include "gfile.h"
#include "gresource.h"
#include "gresourceP.h"
#include "ustring.h"
#include "utype.h"

char *GResourceProgramName;
char *usercharset_names;
/* int local_encoding = e_iso8859_1;*/

static int rcur, rmax=0;
static int rbase = 0, rsummit=0, rskiplen=0;	/* when restricting a search */
struct _GResource_Res *_GResource_Res;

static int rcompar(const void *_r1,const void *_r2) {
    const struct _GResource_Res *r1 = _r1, *r2 = _r2;
return( strcmp(r1->res,r2->res));
}

int _GResource_FindResName(const char *name) {
    int top=rsummit, bottom = rbase;
    int test, cmp;

    if ( rcur==0 )
return( -1 );

    for (;;) {
	if ( top==bottom )
return( -1 );
	test = (top+bottom)/2;
	cmp = strcmp(name,_GResource_Res[test].res+rskiplen);
	if ( cmp==0 )
return( test );
	if ( test==bottom )
return( -1 );
	if ( cmp>0 )
	    bottom=test+1;
	else
	    top = test;
    }
}

static int GResourceRestrict(char *prefix) {
    int top=rcur, bottom = 0;
    int test, cmp;
    int plen;
    int oldtest, oldtop;

    if ( prefix==NULL || *prefix=='\0' ) {
	rbase = rskiplen = 0; rsummit = rcur;
return( rcur==0?-1:0 );
    }
    if ( rcur==0 )
return( -1 );

    plen = strlen(prefix);

    for (;;) {
	test = (top+bottom)/2;
	cmp = strncmp(prefix,_GResource_Res[test].res,plen);
	if ( cmp==0 )
    break;
	if ( test==bottom )
return( -1 );
	if ( cmp>0 ) {
	    bottom=test+1;
	    if ( bottom==top )
return( -1 );
	} else
	    top = test;
    }
    /* at this point the resource at test begins with the prefix */
    /* we want to find the first and last resources that do */
    oldtop = top; oldtest = top = test;		/* find the first resource */
    for (;;) {
	test = (top+bottom)/2;
	cmp = strncmp(prefix,_GResource_Res[test].res,plen);
	if ( cmp<0 ) {
	    GDrawIError("Resource list out of order");
return( -1 );
	}
	if ( test==bottom ) {
	    if ( cmp!=0 ) ++test;
    break;
	}
	if ( cmp>0 ) {
	    bottom=++test;
	    if ( bottom==top )
    break;
	} else
	    top = test;
    }
    rbase = test;

    top = oldtop; bottom = oldtest+1;		/* find the last resource */
    if ( bottom == top )
	test = top;
    else for (;;) {
	test = (top+bottom)/2;
	cmp = strncmp(prefix,_GResource_Res[test].res,plen);
	if ( cmp>0 ) {
	    GDrawIError("Resource list out of order");
return( -1 );
	}
	if ( test==bottom ) {
	    if ( cmp==0 ) ++test;
    break;
	}
	if ( cmp==0 ) {
	    bottom=++test;
	    if ( bottom==top )
    break;
	} else
	    top = test;
    }
    rsummit = test;
    rskiplen = plen;
return( 0 );
}

void GResourceSetProg(char *prog) {
    char filename[1025], *pt;
    extern char *_GFile_find_program_dir(char *prog);

    if ( prog!=NULL ) {
	if ( GResourceProgramName!=NULL && strcmp(prog,GResourceProgramName)==0 )
return;
	free(GResourceProgramName);
	if (( pt=strrchr(prog,'/'))!=NULL )
	    ++pt;
	else
	    pt = prog;
	GResourceProgramName = copy(pt);
    } else if ( GResourceProgramName==NULL )
	GResourceProgramName = copy("gdraw");
    else
return;
}

void GResourceAddResourceString(char *string,char *prog) {
    char *pt, *ept, *next;
    int cnt, plen;
    struct _GResource_Res temp;
    int i,j,k, off;

    GResourceSetProg(prog);
    plen = strlen(GResourceProgramName);

    if ( string==NULL )
return;

    cnt = 0;
    pt = string;
    while ( *pt!='\0' ) {
	next = strchr(pt,'\n');
	if ( next==NULL ) next = pt+strlen(pt);
	else ++next;
	if ( strncmp(pt,"Gdraw.",6)==0 ) ++cnt;
	else if ( strncmp(pt,GResourceProgramName,plen)==0 && pt[plen]=='.' ) ++cnt;
	else if ( strncmp(pt,"*",1)==0 ) ++cnt;
	pt = next;
    }
    if ( cnt==0 )
return;

    if ( rcur+cnt>=rmax ) {
	if ( cnt<10 ) cnt = 10;
	if ( rmax==0 )
	    _GResource_Res = malloc(cnt*sizeof(struct _GResource_Res));
	else
	    _GResource_Res = realloc(_GResource_Res,(rcur+cnt)*sizeof(struct _GResource_Res));
	rmax += cnt;
    }

    pt = string;
    while ( *pt!='\0' ) {
	next = strchr(pt,'\n');
	if ( next==NULL ) next = pt+strlen(pt);
	if ( strncmp(pt,"Gdraw.",6)==0 || strncmp(pt,"*",1)==0 ||
		(strncmp(pt,GResourceProgramName,plen)==0 && pt[plen]=='.' )) {
	    temp.generic = false;
	    if ( strncmp(pt,"Gdraw.",6)==0 ) {
		temp.generic = true;
		off = 6;
	    } else if ( strncmp(pt,"*",1)==0 ) {
		temp.generic = true;
		off = 1;
	    } else
		off = plen+1;
	    ept = strchr(pt+off,':');
	    if ( ept==NULL )
	goto bad;
	    temp.res = copyn(pt+off,ept-(pt+off));
	    pt = ept+1;
	    while ( isspace( *pt ) && pt<next ) ++pt;
	    temp.val = copyn(pt,next-pt);
	    temp.new = true;
	    _GResource_Res[rcur++] = temp;
	}
	bad:
	if ( *next ) ++next;
	pt = next;
    }

    if ( rcur!=0 )
	qsort(_GResource_Res,rcur,sizeof(struct _GResource_Res),rcompar);

    for ( i=j=0; i<rcur; ) {
	if ( i!=j )
	    _GResource_Res[j] = _GResource_Res[i];
	for ( k=i+1; k<rcur && strcmp(_GResource_Res[j].res,_GResource_Res[k].res)==0; ++k ) {
	    if (( !_GResource_Res[k].generic && (_GResource_Res[i].generic || _GResource_Res[i+1].new)) ||
		    (_GResource_Res[k].generic && _GResource_Res[i].generic && _GResource_Res[i+1].new)) {
		free(_GResource_Res[j].res); free(_GResource_Res[j].val);
		_GResource_Res[i].res=NULL;
		_GResource_Res[j] = _GResource_Res[k];
	    } else {
		free(_GResource_Res[k].res); free(_GResource_Res[k].val);
		_GResource_Res[k].res=NULL;
	    }
	}
	i = k; ++j;
    }
    rcur = rsummit = j;
    for ( i=0; i<j; ++i )
	_GResource_Res[i].new = false;

    if ( (i=_GResource_FindResName("LocalCharSet"))!=-1 ) {
	local_encoding = _GDraw_ParseMapping(c_to_u(_GResource_Res[i].val));
	if ( local_encoding==em_none )
	    local_encoding = em_iso8859_1;
	local_encoding += e_iso8859_1-em_iso8859_1;
    }
}

void GResourceAddResourceFile(char *filename,char *prog,int warn) {
    FILE *file;
    char buffer[1000];

    file = fopen(filename,"r");
    if ( file==NULL ) {
	if ( warn )
	    fprintf( stderr, "Failed to open resource file: %s\n", filename );
return;
    }
    while ( fgets(buffer,sizeof(buffer),file)!=NULL )
	GResourceAddResourceString(buffer,prog);
    fclose(file);
}

void GResourceFind( GResStruct *info,char *prefix) {
    int pos;

    if ( GResourceRestrict(prefix)== -1 ) {
	rbase = rskiplen = 0; rsummit = rcur;
return;
    }
    while ( info->resname!=NULL ) {
	pos = _GResource_FindResName(info->resname);
	info->found = (pos!=-1);
	if ( pos==-1 )
	    /* Do Nothing */;
	else if ( info->type == rt_string ) {
	    if ( info->cvt!=NULL )
		*(void **) (info->val) = (info->cvt)( _GResource_Res[pos].val, *(void **) (info->val) );
	    else
		*(char **) (info->val) = copy( _GResource_Res[pos].val );
	} else if ( info->type == rt_color ) {
	    Color temp = _GImage_ColourFName(_GResource_Res[pos].val );
	    if ( temp==-1 ) {
		fprintf( stderr, "Can't convert %s to a Color for resource: %s\n",
			_GResource_Res[pos].val, info->resname );
		info->found = false;
	    } else
		*(Color *) (info->val) = temp;
	} else if ( info->type == rt_int ) {
	    char *end;
	    int val = strtol(_GResource_Res[pos].val,&end,0);
	    if ( *end!='\0' ) {
		fprintf( stderr, "Can't convert %s to an int for resource: %s\n",
			_GResource_Res[pos].val, info->resname );
		info->found = false;
	    } else
		*(int *) (info->val) = val;
	} else if ( info->type == rt_bool ) {
	    int val = -1;
	    if ( strmatch(_GResource_Res[pos].val,"true")==0 ||
		    strmatch(_GResource_Res[pos].val,"on")==0 || strcmp(_GResource_Res[pos].val,"1")==0 )
		val = 1;
	    else if ( strmatch(_GResource_Res[pos].val,"false")==0 ||
		    strmatch(_GResource_Res[pos].val,"off")==0 || strcmp(_GResource_Res[pos].val,"0")==0 )
		val = 0;
	    if ( val==-1 ) {
		fprintf( stderr, "Can't convert %s to a boolean for resource: %s\n",
			_GResource_Res[pos].val, info->resname );
		info->found = false;
	    } else
		*(int *) (info->val) = val;
	} else if ( info->type == rt_double ) {
	    char *end;
	    double val = strtod(_GResource_Res[pos].val,&end);
	    if ( *end=='.' || *end==',' ) {
		*end = (*end==',')?'.':',';
		val = strtod(_GResource_Res[pos].val,&end);
	    }
	    if ( *end!='\0' ) {
		fprintf( stderr, "Can't convert %s to a double for resource: %s\n",
			_GResource_Res[pos].val, info->resname );
		info->found = false;
	    } else
		*(double *) (info->val) = val;
	} else {
	    fprintf( stderr, "Invalid resource type for: %s\n", info->resname );
	    info->found = false;
	}
	++info;
    }
    rbase = rskiplen = 0; rsummit = rcur;
}

char *GResourceFindString(char *name) {
    int pos;

    pos = _GResource_FindResName(name);
    if ( pos==-1 )
return( NULL );
    else
return( copy(_GResource_Res[pos].val));
}

int GResourceFindBool(char *name, int def) {
    int pos;
    int val = -1;

    pos = _GResource_FindResName(name);
    if ( pos==-1 )
return( def );

    if ( strmatch(_GResource_Res[pos].val,"true")==0 ||
	    strmatch(_GResource_Res[pos].val,"on")==0 || strcmp(_GResource_Res[pos].val,"1")==0 )
	val = 1;
    else if ( strmatch(_GResource_Res[pos].val,"false")==0 ||
	    strmatch(_GResource_Res[pos].val,"off")==0 || strcmp(_GResource_Res[pos].val,"0")==0 )
	val = 0;

    if ( val==-1 )
return( def );

return( val );
}

long GResourceFindInt(char *name, long def) {
    int pos;
    char *end;
    long ret;

    pos = _GResource_FindResName(name);
    if ( pos==-1 )
return( def );

    ret = strtol(_GResource_Res[pos].val,&end,10);
    if ( *end!='\0' )
return( def );

return( ret );
}

Color GResourceFindColor(char *name, Color def) {
    int pos;
    Color ret;

    pos = _GResource_FindResName(name);
    if ( pos==-1 )
return( def );

    ret = _GImage_ColourFName(_GResource_Res[pos].val );
    if ( ret==-1 )
return( def );

return( ret );
}
