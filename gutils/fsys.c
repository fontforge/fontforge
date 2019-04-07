/* Copyright (C) 2000-2004 by George Williams */
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

#include <stdio.h>
#include "inc/basics.h"
#include "ustring.h"
#include "fileutil.h"
#include "gfile.h"
#include <fcntl.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>		/* for mkdir */
#include <unistd.h>
#include <ffglib.h>
#include <errno.h>			/* for mkdir_p */

static char dirname_[MAXPATHLEN+1];
#if !defined(__MINGW32__)
 #include <pwd.h>
#else
 #include <windows.h>
 #include <shlobj.h>
#endif

/**
 * \brief Removes the extension from a file path, if it exists.
 * This method assumes that the path is already normalized.
 * \param path The path to be modified. Is modified in-place.
 * \return A pointer to the input path.
 */
char *GFileRemoveExtension(char *path) {
    char *ext = strrchr(path, '.');
    if (ext) {
        char *fp = strrchr(path, '/');
        if (!fp || ext > fp) {
            *ext = '\0';
        }
    }
    return path;
}

/**
 * \brief Normalizes the file path as necessary.
 * On Windows, this means changing backlashes to slashes.
 *
 * \param path The file path to be modified. Is modified in-place.
 * \return A pointer to the input path
 */
char *GFileNormalizePath(char *path) {
#if defined(__MINGW32__)
    char *ptr;
    for(ptr = path; *ptr; ptr++) {
        if (*ptr == '\\') {
            *ptr = '/';
        }
    }
#endif
    return path;
}

/**
 * \brief Normalizes the file path as necessary.
 * Unicode version of GFileNormalizePath.
 *
 * \param path The file path to be modified. Is modified in-place.
 * \return A pointer to the input path
 */
unichar_t *u_GFileNormalizePath(unichar_t *path) {
#if defined(__MINGW32__)
    unichar_t *ptr;
    for (ptr = path; *ptr; ptr++) {
        if (*ptr == '\\') {
            *ptr = '/';
        }
    }
#endif
    return path;
}

/* make directories.  make parent directories as needed,  with no error if
 * the path already exists */
int mkdir_p(const char *path, mode_t mode) {
	struct stat st;
	const char *e;
	char *p = NULL;
	char tmp[1024];
	size_t len;
	int r;

	/* ensure the path is valid */
	if(!(e = strrchr(path, '/')))
return -EINVAL;
	/* ensure path is a directory */
	r = stat(path, &st);
	if (r == 0 && !S_ISDIR(st.st_mode))
return -ENOTDIR;

	/* copy the pathname */
	snprintf(tmp, sizeof(tmp),"%s", path);
	len = strlen(tmp);
	if(tmp[len - 1] == '/')
	tmp[len - 1] = 0;

	/* iterate mkdir over the path */
	for(p = tmp + 1; *p; p++)
	if(*p == '/') {
		*p = 0;
		r = GFileMkDir(tmp, mode);
		if (r < 0 && errno != EEXIST)
return -errno;
		*p = '/';
	}

	/* try to make the whole path */
	r = GFileMkDir(tmp, mode);
	if(r < 0 && errno != EEXIST)
return -errno;
	/* creation successful or the file already exists */
return EXIT_SUCCESS;
}

char *GFileGetHomeDir(void) {
#if defined(__MINGW32__)
    char* dir = getenv("HOME");
    if(!dir)
	dir = getenv("USERPROFILE");
    if(dir){
	char* buffer = copy(dir);
	GFileNormalizePath(buffer);
return buffer;
    }
return NULL;
#else
    static char *dir;
    uid_t uid;
    struct passwd *pw;

    dir = getenv("HOME");
    if ( dir!=NULL )
	return( copy(dir) );

    uid = getuid();
    while ( (pw=getpwent())!=NULL ) {
	if ( pw->pw_uid==uid ) {
	    dir = copy(pw->pw_dir);
	    endpwent();
return( dir );
	}
    }
    endpwent();
return( NULL );
#endif
}

unichar_t *u_GFileGetHomeDir(void) {
    unichar_t* dir = NULL;
    char* tmp = GFileGetHomeDir();
    if( tmp ) {
	dir = uc_copy(tmp);
	free(tmp);
    }
return dir;
}

static void savestrcpy(char *dest,const char *src) {
    for (;;) {
	*dest = *src;
	if ( *dest=='\0' )
    break;
	++dest; ++src;
    }
}

char *GFileGetAbsoluteName(const char *name, char *result, size_t rsiz) {
    /* result may be the same as name */
    char buffer[1000];

     if ( ! GFileIsAbsolute(name) ) {
	char *pt, *spt, *rpt, *bpt;

	if ( dirname_[0]=='\0' ) {
	    getcwd(dirname_,sizeof(dirname_));
	}
	strcpy(buffer,dirname_);
	if ( buffer[strlen(buffer)-1]!='/' )
	    strcat(buffer,"/");
	strcat(buffer,name);
	#if defined(__MINGW32__)
	GFileNormalizePath(buffer);
	#endif

	/* Normalize out any .. */
	spt = rpt = buffer;
	while ( *spt!='\0' ) {
	    if ( *spt=='/' )  {
		if ( *++spt=='\0' )
	break;
	    }
	    for ( pt = spt; *pt!='\0' && *pt!='/'; ++pt );
	    if ( pt==spt )	/* Found // in a path spec, reduce to / (we've*/
		savestrcpy(spt,spt+1); /*  skipped past the :// of the machine name) */
	    else if ( pt==spt+1 && spt[0]=='.' && *pt=='/' ) {	/* Noop */
		savestrcpy(spt,spt+2);
	    } else if ( pt==spt+2 && spt[0]=='.' && spt[1]=='.' ) {
		for ( bpt=spt-2 ; bpt>rpt && *bpt!='/'; --bpt );
		if ( bpt>=rpt && *bpt=='/' ) {
		    savestrcpy(bpt,pt);
		    spt = bpt;
		} else {
		    rpt = pt;
		    spt = pt;
		}
	    } else
		spt = pt;
	}
	name = buffer;
	if ( rsiz>sizeof(buffer)) rsiz = sizeof(buffer);	/* Else valgrind gets unhappy */
    }
    if (result!=name) {
	strncpy(result,name,rsiz);
	result[rsiz-1]='\0';
	#if defined(__MINGW32__)
	GFileNormalizePath(result);
	#endif
    }
return(result);
}

char *GFileMakeAbsoluteName(char *name) {
    char buffer[1025];

    GFileGetAbsoluteName(name,buffer,sizeof(buffer));
return( copy(buffer));
}

char *GFileBuildName(char *dir,char *fname,char *buffer,size_t size) {
    int len;

    if ( dir==NULL || *dir=='\0' ) {
	if ( strlen( fname )<size-1 )		/* valgrind didn't like my strncpies but this complication makes it happy */
	    savestrcpy(buffer,fname);
	else {
	    strncpy(buffer,fname,size-1);
	    buffer[size-1]='\0';
	}
    } else {
	if ( buffer!=dir ) {
	    if ( strlen( dir )<size-3 )
		strcpy(buffer,dir);
	    else {
		strncpy(buffer,dir,size-3);
		buffer[size-3]='\0';
	    }
	}
	len = strlen(buffer);
	if ( buffer[len-1]!='/' )
	    buffer[len++] = '/';
	if ( strlen( fname )<size-1 )
	    strcpy(buffer+len,fname);
	else {
	    strncpy(buffer+len,fname,size-len-1);
	    buffer[size-1]='\0';
	}
    }
return( buffer );
}

/* Given a filename in a directory, pick the directory out of it, and */
/*  create a new filename using that directory and the given nametail */
char *GFileReplaceName(char *oldname,char *fname,char *buffer,size_t size) {
    int len;
    char *dirend;

    dirend = strrchr(oldname,'/');
    if ( dirend == NULL ) {
	strncpy(buffer,fname,size-1);
	buffer[size-1]='\0';
    } else {
	*dirend = '\0';
	if ( buffer!=oldname ) {
	    strncpy(buffer,oldname,size-3);
	    buffer[size-3]='\0';
	}
	len = strlen(buffer);
	*dirend = '/';
	buffer[len++] = '/';
	strncpy(buffer+len,fname,size-len-1);
	buffer[size-1]='\0';
    }
return( buffer );
}

char *GFileNameTail(const char *oldname) {
    char *pt = 0;

    pt = strrchr(oldname,'/');

    // a final slash was found, so we know that p+1 is a valid
    // address in the string.
    if ( pt )
	return( pt+1);

    return( (char *)oldname );
}

char *GFileAppendFile(char *dir,char *name,int isdir) {
    char *ret, *pt;

    ret = (char *) malloc((strlen(dir)+strlen(name)+3));
    strcpy(ret,dir);
    pt = ret+strlen(ret);
    if ( pt>ret && pt[-1]!='/' )
	*pt++ = '/';
    strcpy(pt,name);
    if ( isdir ) {
	pt += strlen(pt);
	if ( pt>ret && pt[-1]!='/' ) {
	    *pt++ = '/';
	    *pt = '\0';
	}
    }
return(ret);
}

int GFileIsAbsolute(const char *file) {
#if defined(__MINGW32__)
    if( (file[1]==':') && (('a'<=file[0] && file[0]<='z') || ('A'<=file[0] && file[0]<='Z')) )
return ( true );
#else
    if ( *file=='/' )
return( true );
#endif
    if ( strstr(file,"://")!=NULL )
return( true );

return( false );
}

int GFileIsDir(const char *file) {
  struct stat info;
  if ( stat(file, &info)==-1 )
return 0;
  else
return( S_ISDIR(info.st_mode) );
}

int GFileExists(const char *file) {
return( access(file,0)==0 );
}

int GFileModifyable(const char *file) {
return( access(file,02)==0 );
}

int GFileModifyableDir(const char *file) {
    char buffer[1025], *pt;

    buffer[1024]=0;
    strncpy(buffer,file,1024);
    pt = strrchr(buffer,'/');
    if ( pt==NULL )
	strcpy(buffer,".");
    else
	*pt='\0';
    return( GFileModifyable(buffer) );
}

int GFileReadable(const char *file) {
return( access(file,04)==0 );
}

/**
 *  Creates a temporary file, similar to tmpfile.
 *  Used because the default tmpfile implementation on Windows is broken
 */
FILE *GFileTmpfile() {
#ifndef _WIN32
    return tmpfile();
#else
    wchar_t temp_path[MAX_PATH + 1];
    DWORD ret = GetTempPathW(MAX_PATH + 1, temp_path);
    if (!ret) {
        return NULL;
    }

    while(true) {
        wchar_t *temp_name = _wtempnam(temp_path, L"FF_");
        if (!temp_name) {
            return NULL;
        }

        HANDLE handle = CreateFileW(
            temp_name,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            CREATE_NEW,
            FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE,
            NULL
        );
        free(temp_name);

        if (handle == INVALID_HANDLE_VALUE) {
            if (GetLastError() != ERROR_FILE_EXISTS) {
                return NULL;
            }
        } else {
            int fd = _open_osfhandle((intptr_t)handle, _O_RDWR|_O_CREAT|_O_TEMPORARY|_O_BINARY);
            if (fd == -1) {
                CloseHandle(handle);
                return NULL;
            }

            FILE *fp = _fdopen(fd, "w+");
            if (!fp) {
                _close(fd);
                return NULL;
            }

            return fp;
        }
    }
    return NULL;
#endif
}

/**
 * Removes a file or folder.
 *
 * @param [in] path The path to be removed.
 * @param [in] recursive Specify true to remove a folder and all of its
 *                       sub-contents.
 * @return true if the deletion was successful or the path does not exist. It
 *         will fail if trying to remove a directory that is not empty and
 *         where `recursive` is false.
 */
int GFileRemove(const char *path, int recursive) {
    GDir *dir;
    const gchar *entry;

    if (g_remove(path) != 0) {
        if (recursive && (dir = g_dir_open(path, 0, NULL))) {
            while ((entry = g_dir_read_name(dir))) {
                gchar *fpath = g_build_filename(path, entry, NULL);
                if (g_remove(fpath) != 0 && GFileIsDir(fpath)) {
                    GFileRemove(fpath, recursive);
                }
                g_free(fpath);
            }
            g_dir_close(dir);
        }
        return (g_remove(path) == 0 || !GFileExists(path));
    }

    return true;
}

int GFileMkDir(const char *name, int mode) {
#ifndef _WIN32
	return mkdir(name, mode);
#else
	return mkdir(name);
#endif
}

int GFileRmDir(const char *name) {
return(rmdir(name));
}

int GFileUnlink(const char *name) {
return(unlink(name));
}

char *_GFile_find_program_dir(char *prog) {
    char *pt, *path, *program_dir=NULL;
    char filename[2000];

#if defined(__MINGW32__)
    char* pt1 = strrchr(prog, '/');
    char* pt2 = strrchr(prog, '\\');
    if(pt1<pt2) pt1=pt2;
    if(pt1)
	program_dir = copyn(prog, pt1-prog);
    else if( (path = getenv("PATH")) != NULL ){
	char* tmppath = copy(path);
	path = tmppath;
	for(;;){
	    pt1 = strchr(path, ';');
	    if(pt1) *pt1 = '\0';
	    sprintf(filename,"%s/%s", path, prog);
	    if ( access(filename,1)!= -1 ) {
		program_dir = copy(path);
		break;
	    }
	    if(!pt1) break;
	    path = pt1+1;
	}
	free(tmppath);
    }
#else
    if ( (pt = strrchr(prog,'/'))!=NULL )
	program_dir = copyn(prog,pt-prog);
    else if ( (path = getenv("PATH"))!=NULL ) {
	while ((pt = strchr(path,':'))!=NULL ) {
	  sprintf(filename,"%.*s/%s", (int)(pt-path), path, prog);
	    /* Under cygwin, applying access to "potrace" will find "potrace.exe" */
	    /*  no need for special check to add ".exe" */
	    if ( access(filename,1)!= -1 ) {
		program_dir = copyn(path,pt-path);
	break;
	    }
	    path = pt+1;
	}
	if ( program_dir==NULL ) {
	    sprintf(filename,"%s/%s", path, prog);
	    if ( access(filename,1)!= -1 )
		program_dir = copy(path);
	}
    }
#endif

    if ( program_dir==NULL )
return( NULL );
    GFileGetAbsoluteName(program_dir,filename,sizeof(filename));
    free(program_dir);
    program_dir = copy(filename);
return( program_dir );
}

unichar_t *u_GFileGetAbsoluteName(unichar_t *name, unichar_t *result, int rsiz) {
    /* result may be the same as name */
    unichar_t buffer[1000];

    if ( ! u_GFileIsAbsolute(name) ) {
	unichar_t *pt, *spt, *rpt, *bpt;

	if ( dirname_[0]=='\0' ) {
	    getcwd(dirname_,sizeof(dirname_));
	}
	uc_strcpy(buffer,dirname_);
	if ( buffer[u_strlen(buffer)-1]!='/' )
	    uc_strcat(buffer,"/");
	u_strcat(buffer,name);
	u_GFileNormalizePath(buffer);

	/* Normalize out any .. */
	spt = rpt = buffer;
	while ( *spt!='\0' ) {
	    if ( *spt=='/' ) ++spt;
	    for ( pt = spt; *pt!='\0' && *pt!='/'; ++pt );
	    if ( pt==spt )	/* Found // in a path spec, reduce to / (we've*/
		u_strcpy(spt,pt); /*  skipped past the :// of the machine name) */
	    else if ( pt==spt+1 && spt[0]=='.' && *pt=='/' )	/* Noop */
		u_strcpy(spt,spt+2);
	    else if ( pt==spt+2 && spt[0]=='.' && spt[1]=='.' ) {
		for ( bpt=spt-2 ; bpt>rpt && *bpt!='/'; --bpt );
		if ( bpt>=rpt && *bpt=='/' ) {
		    u_strcpy(bpt,pt);
		    spt = bpt;
		} else {
		    rpt = pt;
		    spt = pt;
		}
	    } else
		spt = pt;
	}
	name = buffer;
    }
    if (result!=name) {
	u_strncpy(result,name,rsiz);
	result[rsiz-1]='\0';
	u_GFileNormalizePath(result);
    }
return(result);
}

unichar_t *u_GFileBuildName(unichar_t *dir,unichar_t *fname,unichar_t *buffer,int size) {
    int len;

    if ( dir==NULL || *dir=='\0' ) {
	u_strncpy(buffer,fname,size-1);
	buffer[size-1]='\0';
    } else {
	if ( buffer!=dir ) {
	    u_strncpy(buffer,dir,size-3);
	    buffer[size-3]='\0';
	}
	len = u_strlen(buffer);
	if ( buffer[len-1]!='/' )
	    buffer[len++] = '/';
	u_strncpy(buffer+len,fname,size-len-1);
	buffer[size-1]='\0';
    }
return( buffer );
}

/* Given a filename in a directory, pick the directory out of it, and */
/*  create a new filename using that directory and the given nametail */
unichar_t *u_GFileReplaceName(unichar_t *oldname,unichar_t *fname,unichar_t *buffer,int size) {
    int len;
    unichar_t *dirend;

    dirend = u_strrchr(oldname,'/');
    if ( dirend == NULL ) {
	u_strncpy(buffer,fname,size-1);
	buffer[size-1]='\0';
    } else {
	*dirend = '\0';
	if ( buffer!=oldname ) {
	    u_strncpy(buffer,oldname,size-3);
	    buffer[size-3]='\0';
	}
	len = u_strlen(buffer);
	*dirend = '/';
	buffer[len++] = '/';
	u_strncpy(buffer+len,fname,size-len-1);
	buffer[size-1]='\0';
    }
return( buffer );
}

unichar_t *u_GFileNameTail(const unichar_t *oldname) {
    unichar_t *pt;

    pt = u_strrchr(oldname,'/');
    if ( pt !=NULL )
return( pt+1);
    else
return( (unichar_t *)oldname );
}

/**
 * Remove the 'root' part of the file path if it is absolute;
 * On Unix this is '/' and on Windows this is for e.g. 'C:/'
 */
static unichar_t *u_GFileRemoveRoot(unichar_t *path) {
    //May happen on Windows too e.g. CygWin
    if (*path == '/') {
        path++;
    }
#ifdef _WIN32
    //Check if it is a drive letter path
    else if (((path[0] >= 'A' && path[0] <= 'Z') ||
              (path[0] >= 'a' && path[0] <= 'z')) &&
             path[1] == ':' && path[2] == '/') {
             
        path += 3;
    }
#endif
    return path;
}

unichar_t *u_GFileNormalize(unichar_t *name) {
    unichar_t *pt, *base, *ppt;

    if ( (pt = uc_strstr(name,"://"))!=NULL ) {
	base = u_strchr(pt+3,'/');
	if ( base==NULL )
return( name );
	++base;
    }
    
    base = u_GFileRemoveRoot(name);
    for ( pt=base; *pt!='\0'; ) {
	if ( *pt=='/' )
	    u_strcpy(pt,pt+1);
	else if ( uc_strncmp(pt,"./",2)==0 )
	    u_strcpy(pt,pt+2);
	else if ( uc_strncmp(pt,"../",2)==0 ) {
	    for ( ppt=pt-2; ppt>=base && *ppt!='/'; --ppt );
	    ++ppt;
	    if ( ppt>=base ) {
		u_strcpy(ppt,pt+3);
		pt = ppt;
	    } else
		pt += 3;
	} else {
	    while ( *pt!='/' && *pt!='\0' ) ++pt;
	    if ( *pt == '/' ) ++pt;
	}
    }
return( name );
}

unichar_t *u_GFileAppendFile(unichar_t *dir,unichar_t *name,int isdir) {
    unichar_t *ret, *pt;

    ret = (unichar_t *) malloc((u_strlen(dir)+u_strlen(name)+3)*sizeof(unichar_t));
    u_strcpy(ret,dir);
    pt = ret+u_strlen(ret);
    if ( pt>ret && pt[-1]!='/' )
	*pt++ = '/';
    u_strcpy(pt,name);
    if ( isdir ) {
	pt += u_strlen(pt);
	if ( pt>ret && pt[-1]!='/' ) {
	    *pt++ = '/';
	    *pt = '\0';
	}
    }
return(ret);
}

int u_GFileIsAbsolute(const unichar_t *file) {
#if defined(__MINGW32__)
    if( (file[1]==':') && (('a'<=file[0] && file[0]<='z') || ('A'<=file[0] && file[0]<='Z')) )
return ( true );
#else
    if ( *file=='/' )
return( true );
#endif
    if ( uc_strstr(file,"://")!=NULL )
return( true );

return( false );
}

int u_GFileIsDir(const unichar_t *file) {
    char buffer[1024];
    u2def_strncpy(buffer,file,sizeof(buffer));
    return GFileIsDir(buffer);
}

int u_GFileExists(const unichar_t *file) {
    char buffer[1024];
    u2def_strncpy(buffer,file,sizeof(buffer));
return( access(buffer,0)==0 );
}

int u_GFileModifyable(const unichar_t *file) {
    char buffer[1024];
    u2def_strncpy(buffer,file,sizeof(buffer));
return( access(buffer,02)==0 );
}

int u_GFileModifyableDir(const unichar_t *file) {
    char buffer[1024], *pt;

    u2def_strncpy(buffer,file,sizeof(buffer));
    pt = strrchr(buffer,'/');
    if ( pt==NULL )
	strcpy(buffer,".");
    else
	*pt='\0';
return( GFileModifyable(buffer));
}

int u_GFileReadable(unichar_t *file) {
    char buffer[1024];
    u2def_strncpy(buffer,file,sizeof(buffer));
return( access(buffer,04)==0 );
}

int u_GFileMkDir(unichar_t *name) {
    char buffer[1024];
    u2def_strncpy(buffer,name,sizeof(buffer));
	return GFileMkDir(buffer, 0755);
}

int u_GFileRmDir(unichar_t *name) {
    char buffer[1024];
    u2def_strncpy(buffer,name,sizeof(buffer));
return(rmdir(buffer));
}

int u_GFileUnlink(unichar_t *name) {
    char buffer[1024];
    u2def_strncpy(buffer,name,sizeof(buffer));
return(unlink(buffer));
}

static char *GResourceProgramDir = 0;

char* getGResourceProgramDir(void) {
    return GResourceProgramDir;
}

char* getLibexecDir_NonWindows(void) 
{
    // FIXME this was indirectly introduced by
    // https://github.com/fontforge/fontforge/pull/1838 and is not
    // tested on Windows yet.
    //
    static char path[PATH_MAX+4];
    snprintf( path, PATH_MAX, "%s/../libexec/", getGResourceProgramDir());
    return path;
}



void FindProgDir(char *prog) {
#if defined(__MINGW32__)
    char  path[MAX_PATH+4];
    char* c = path;
    char* tail = 0;
    unsigned int  len = GetModuleFileNameA(NULL, path, MAX_PATH);
    path[len] = '\0';
    for(; *c; *c++){
    	if(*c == '\\'){
    	    tail=c;
    	    *c = '/';
    	}
    }
    if(tail) *tail='\0';
    GResourceProgramDir = copy(path);
#else
    GResourceProgramDir = _GFile_find_program_dir(prog);
    if ( GResourceProgramDir==NULL ) {
	char filename[1025];
	GFileGetAbsoluteName(".",filename,sizeof(filename));
	GResourceProgramDir = copy(filename);
    }
#endif
}

char *getShareDir(void) {
    static char *sharedir=NULL;
    static int set=false;
    char *pt;
    int len;

    if ( set )
	return( sharedir );

    set = true;

    //Assume share folder is one directory up
    pt = strrchr(GResourceProgramDir, '/');
    if ( pt==NULL ) {
#ifdef SHAREDIR
	return( sharedir = SHAREDIR );
#elif defined( PREFIX )
	return( sharedir = PREFIX "/share" );
#else
	pt = GResourceProgramDir + strlen(GResourceProgramDir);
#endif
    }
    len = (pt-GResourceProgramDir)+strlen("/share/fontforge")+1;
    sharedir = malloc(len);
    strncpy(sharedir,GResourceProgramDir,pt-GResourceProgramDir);
    strcpy(sharedir+(pt-GResourceProgramDir),"/share/fontforge");
    return( sharedir );
}


char *getLocaleDir(void) {
    static char *sharedir=NULL;
    static int set=false;

    if ( set )
	return( sharedir );

    char* prefix = getShareDir();
    int len = strlen(prefix) + strlen("/../locale") + 2;
    sharedir = malloc(len);
    strcpy(sharedir,prefix);
    strcat(sharedir,"/../locale");
    set = true;
    return sharedir;
}

char *getPixmapDir(void) {
    static char *sharedir=NULL;
    static int set=false;

    if ( set )
	return( sharedir );

    char* prefix = getShareDir();
    int len = strlen(prefix) + strlen("/pixmaps") + 2;
    sharedir = malloc(len);
    strcpy(sharedir,prefix);
    strcat(sharedir,"/pixmaps");
    set = true;
    return sharedir;
}

char *getHelpDir(void) {
    static char *sharedir=NULL;
    static int set=false;

    if ( set )
	return( sharedir );

    char* prefix = getShareDir();
#if defined(DOCDIR)
    prefix = DOCDIR;
#endif
    const char* postfix = "/../doc/fontforge/";
    int len = strlen(prefix) + strlen(postfix) + 2;
    sharedir = malloc(len);
    strcpy(sharedir,prefix);
    strcat(sharedir,postfix);
    set = true;
    return sharedir;
}

/* reimplementation of GFileGetHomeDir, avoiding copy().  Returns NULL if home
 * directory cannot be found */
char *getUserHomeDir(void) {
#if defined(__MINGW32__)
	char* dir = getenv("APPDATA");
	if( dir==NULL )
	dir = getenv("USERPROFILE");
	if( dir!=NULL ) {
	GFileNormalizePath(dir);
return dir;
	}
return NULL;
#else
	uid_t uid;
	struct passwd *pw;
	char *home = getenv("HOME");

	if( home!=NULL )
return home;

	uid = getuid();
	while( (pw=getpwent())!=NULL ) {
	if ( pw->pw_uid==uid ) {
		home = pw->pw_dir;
		endpwent();
return home;
	}
	}
	endpwent();
return NULL;
#endif
}

/* Find the directory in which FontForge places all of its configurations and
 * save files.  On Unix-likes, the argument `dir` (see the below case switch,
 * enum in inc/gfile.h) determines which directory is returned according to the
 * XDG Base Directory Specification.  On Windows, the argument is ignored--the
 * home directory as obtained by getUserHomeDir() appended with "/FontForge" is
 * returned. On error, NULL is returned.
 *
 * http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html
 */
char *getFontForgeUserDir(int dir) {
	const char *def;
	const char *home, *xdg;
	char *buf = NULL;

	/* find home directory first, it is needed if any of the xdg env vars are
	 * not set */
	if (!(home = getUserHomeDir())) {
	/* if getUserHomeDir returns NULL, pass NULL to calling function */
	fprintf(stderr, "%s\n", "cannot find home directory");
return NULL;
	}
#ifdef _WIN32
	/* Allow for preferences to be saved locally in a 'portable' configuration. */ 
	if (getenv("FF_PORTABLE") != NULL) {
		buf = smprintf("%s/preferences/", getShareDir());
	} else {
		buf = smprintf("%s/FontForge/", home);
	}
	return buf;
#else
	/* Home directory exists, so check for environment variables.  For each of
	 * XDG_{CACHE,CONFIG,DATA}_HOME, assign `def` as the corresponding fallback
	 * for if the environment variable does not exist. */
	switch(dir) {
	  case Cache:
	xdg = getenv("XDG_CACHE_HOME");
	def = ".cache";
	  break;
	  case Config:
	xdg = getenv("XDG_CONFIG_HOME");
	def = ".config";
	  break;
	  case Data:
	xdg = getenv("XDG_DATA_HOME");
	def = ".local/share";
	  break;
	  default:
	/* for an invalid argument, return NULL */
	fprintf(stderr, "%s\n", "invalid input");
return NULL;
	}
	if(xdg != NULL)
	/* if, for example, XDG_CACHE_HOME exists, assign the value
	 * "$XDG_CACHE_HOME/fontforge" */
	buf = smprintf("%s/fontforge", xdg);
	else
	/* if, for example, XDG_CACHE_HOME does not exist, instead assign
	 * the value "$HOME/.cache/fontforge" */
	buf = smprintf("%s/%s/fontforge", home, def);
	if(buf != NULL) {
	    /* try to create buf.  If creating the directory fails, return NULL
	     * because nothing will get saved into an inaccessible directory.  */
            if ( mkdir_p(buf, 0755) != EXIT_SUCCESS ) {
                free(buf);
                return NULL;
            }
            return buf;
	}
return NULL;
#endif
}

off_t GFileGetSize(char *name) {
/* Get the binary file size for file 'name'. Return -1 if error. */
    struct stat buf;
    long rc;

    if ( (rc=stat(name,&buf)) )
	return( -1 );
    return( buf.st_size );
}

char *GFileReadAll(char *name) {
/* Read file 'name' all into one large string. Return 0 if error. */
    char *ret;
    long sz;

    if ( (sz=GFileGetSize(name))>=0 && \
	 (ret=calloc(1,sz+1))!=NULL ) {
	FILE *fp;
	if ( (fp=fopen(name,"rb"))!=NULL ) {
	    size_t bread=fread(ret,1,sz,fp);
	    fclose(fp);

	    if( bread==(size_t)sz )
		return( ret );
	}
	free(ret);
    }
    return( 0 );
}

/*
 * Write char string 'data' into file 'name'. Return -1 if error.
 **/
int GFileWriteAll(char *filepath, char *data) {
    
    if( !data )
	return -1;
    
    size_t bwrite = strlen(data);
    FILE* fp;

    if ( (fp = fopen( filepath, "wb" )) != NULL ) {
	if ( (fwrite( data, 1, bwrite, fp ) == bwrite) && \
	     (fflush(fp) == 0) )
	    return( (fclose(fp) == 0 ? 0: -1) );
	fclose(fp);
    }
    return -1;
}

const char *getTempDir(void)
{
    return g_get_tmp_dir();
}

char *GFileGetHomeDocumentsDir(void)
{
    static char* ret = 0;
    if( ret )
	return ret;

#if defined(__MINGW32__)

    CHAR my_documents[MAX_PATH+2];
    HRESULT result = SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, my_documents );
    if (result != S_OK)
    {
    	fprintf(stderr,"Error: Can't get My Documents path!'\n");
        return ret;
    }
    int pos = strlen(my_documents);
    my_documents[ pos++ ] = '\\';
    my_documents[ pos++ ] = '\0';
    ret = copy( my_documents );
	GFileNormalizePath(ret);
    return ret;
#endif

    // On GNU/Linux and OSX it was decided that this should be just the
    // home directory itself.
    ret = GFileGetHomeDir();
    return ret;
}

unichar_t *u_GFileGetHomeDocumentsDir(void) {
    unichar_t* dir = NULL;
    char* tmp = GFileGetHomeDocumentsDir();
    if(tmp) {
        dir = uc_copy(tmp);
    }
    return dir;
}


char *GFileDirNameEx(const char *path, int treat_as_file)
{
    char *ret = NULL;
    if (path != NULL) {
        //Must allocate enough space to append a trailing slash.
        size_t len = strlen(path);
        ret = malloc(len + 2);
        
        if (ret != NULL) {
            char *pt;
            
            strcpy(ret, path);
            GFileNormalizePath(ret);
            if (treat_as_file || !GFileIsDir(ret)) {
                pt = strrchr(ret, '/');
                if (pt != NULL) {
                    *pt = '\0';
                }
            }
            
            //Keep only one trailing slash
            len = strlen(ret);
            for (pt = ret + len - 1; pt >= ret && *pt == '/'; pt--) {
                *pt = '\0';
            }
            *++pt = '/';
            *++pt = '\0';
        }
    }
    return ret;
}

char *GFileDirName(const char *path) {
    return GFileDirNameEx(path, 0);
}

