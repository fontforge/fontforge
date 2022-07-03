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

#include <fontforge-config.h>

#include "basics.h"
#include "ffglib.h"
#include "gfile.h"
#include "ustring.h"

#include <errno.h>			/* for mkdir_p */
#include <fcntl.h>
#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>		/* for mkdir */
#include <sys/types.h>
#include <unistd.h>

#if !defined(__MINGW32__)
 #include <pwd.h>
#else
 #include <shlobj.h>
 #include <windows.h>
#endif

static char *program_root = NULL;

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

char *GFileGetAbsoluteName(const char *name) {
    if (!name) {
        return NULL;
    } else if (!strncasecmp(name, "file://", 7)) {
        name += 7;
    }

#if GLIB_CHECK_VERSION(2, 58, 0)
    gchar* abs = g_canonicalize_filename(name, NULL);
    char *ret;
    // If the input ends with '/', preserve that trailing slash
    if (name && (name = strrchr(name, '/')) && name[1] == '\0') {
        ret = smprintf("%s/", abs);
    } else {
        ret = copy(abs);
    }
    g_free(abs);
    return GFileNormalizePath(ret);
#else
    char *buffer, *pt, *spt, *rpt, *bpt;

    if ( ! GFileIsAbsolute(name) ) {
	static char dirname_[MAXPATHLEN+1];

	if ( dirname_[0]=='\0' ) {
	    getcwd(dirname_,sizeof(dirname_));
	}

	buffer = smprintf("%s/%s", dirname_, name);
    } else {
	buffer = copy(name);
    }

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
	    } else if (pt==spt+1 && spt[0]=='.' && *pt=='\0') { /* Remove trailing /. */
		pt = --spt;
		*spt = '\0';
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
    return buffer;
#endif
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

void FindProgRoot(const char *prog) {
    char *tmp = NULL;
    gchar *rprog = NULL;
    if (program_root != NULL) {
        return;
    }

#ifdef _WIN32
    char path[MAX_PATH+4];
    unsigned int len = GetModuleFileNameA(NULL, path, MAX_PATH);
    path[len] = '\0';
    prog = GFileNormalizePath(path);
#endif

    if (prog != NULL) {
        if (strchr(prog, '/') == NULL) {
            prog = rprog = g_find_program_in_path(prog);
        }
        if (prog) {
            tmp = smprintf("%s/../..", prog);
        }
        program_root = GFileGetAbsoluteName(tmp);
        free(tmp);
    }

    if (program_root == NULL) {
        program_root = GFileGetAbsoluteName(FONTFORGE_INSTALL_PREFIX);
    }

    // Sigh glib doesn't provide symlink resolution
#ifdef HAVE_REALPATH
    tmp = smprintf("%s/share/fontforge", program_root);
    if (!GFileExists(tmp)) {
        free(tmp);
        tmp = realpath(prog, NULL);
        if (tmp) {
            char *real_root = smprintf("%s/../..", tmp);
            free(tmp);
            free(program_root);

            program_root = GFileGetAbsoluteName(real_root);
            free(real_root);
        }
    } else {
        free(tmp);
    }
#endif

    g_free(rprog);
    TRACE("Program root: %s\n", program_root);
}

const char *getShareDir(void) {
    static char *sharedir=NULL;
    if (!sharedir) {
        sharedir = smprintf("%s/share/fontforge", program_root);
    }
    return sharedir;
}

const char *getLocaleDir(void) {
    static char *localedir=NULL;
    if (!localedir) {
        localedir = smprintf("%s/share/locale", program_root);
    }
    return localedir;
}

const char *getPixmapDir(void) {
    static char *pixmapdir=NULL;
    if (!pixmapdir) {
        pixmapdir = smprintf("%s/pixmaps", getShareDir());
    }
    return pixmapdir;
}

const char *getHelpDir(void) {
    static char *helpdir=NULL;
    if (!helpdir) {
        helpdir = smprintf("%s/share/doc/fontforge/", program_root);
    }
    return helpdir;
}

/* reimplementation of GFileGetHomeDir, avoiding copy().  Returns NULL if home
 * directory cannot be found */
const char *getUserHomeDir(void) {
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
	const char *home;
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
	const char *def, *xdg;
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

static int mime_comp(const void *k, const void *v) {
    return strmatch((const char*)k, ((const char**)v)[0]);
}

char* GFileMimeType(const char *path) {
    char* ret, *pt;
    gboolean uncertain = false;
    gchar* res = g_content_type_guess(path, NULL, 0, &uncertain);
    gchar* mres = g_content_type_get_mime_type(res);
    g_free(res);

    if (!mres || uncertain || strstr(mres, "application/x-ext") || !strcmp(mres, "application/octet-stream")) {
        path = GFileNameTail(path);
        pt = strrchr(path, '.');

        if (pt == NULL) {
            if (!strmatch(path, "makefile") || !strmatch(path, "makefile~"))
                ret = copy("application/x-makefile");
            else if (!strmatch(path, "core"))
                ret = copy("application/x-core");
            else
                ret = copy("application/octet-stream");
        } else {
            pt = copy(pt + 1);
            int len = strlen(pt);
            if (len && pt[len - 1] == '~') {
                pt[len - 1] = '\0';
            }

            // array MUST be sorted by extension
            static const char* ext_mimes[][2] = {
                {"bdf",   "application/x-font-bdf"},
                {"bin",   "application/x-macbinary"},
                {"bz2",   "application/x-compressed"},
                {"c",     "text/c"},
                {"cff",   "application/x-font-type1"},
                {"cid",   "application/x-font-cid"},
                {"css",   "text/css"},
                {"dfont", "application/x-mac-dfont"},
                {"eps",   "text/ps"},
                {"gai",   "font/otf"},
                {"gif",   "image/gif"},
                {"gz",    "application/x-compressed"},
                {"h",     "text/h"},
                {"hqx",   "application/x-mac-binhex40"},
                {"html",  "text/html"},
                {"jpeg",  "image/jpeg"},
                {"jpg",   "image/jpeg"},
                {"mov",   "video/quicktime"},
                {"o",     "application/x-object"},
                {"obj",   "application/x-object"},
                {"otb",   "font/otf"},
                {"otf",   "font/otf"},
                {"pcf",   "application/x-font-pcf"},
                {"pdf",   "application/pdf"},
                {"pfa",   "application/x-font-type1"},
                {"pfb",   "application/x-font-type1"},
                {"png",   "image/png"},
                {"ps",    "text/ps"},
                {"pt3",   "application/x-font-type1"},
                {"ras",   "image/x-cmu-raster"},
                {"rgb",   "image/x-rgb"},
                {"rpm",   "application/x-compressed"},
                {"sfd",   "application/vnd.font-fontforge-sfd"},
                {"sgi",   "image/x-sgi"},
                {"snf",   "application/x-font-snf"},
                {"svg",   "image/svg+xml"},
                {"tar",   "application/x-tar"},
                {"tbz",   "application/x-compressed"},
                {"text",  "text/plain"},
                {"tgz",   "application/x-compressed"},
                {"ttf",   "font/ttf"},
                {"txt",   "text/plain"},
                {"wav",   "audio/wave"},
                {"woff",  "font/woff"},
                {"woff2", "font/woff2"},
                {"xbm",   "image/x-xbitmap"},
                {"xml",   "text/xml"},
                {"xpm",   "image/x-xpixmap"},
                {"z",     "application/x-compressed"},
                {"zip",   "application/x-compressed"},
            };

            const char** elem = bsearch(pt, ext_mimes,
                sizeof(ext_mimes)/sizeof(ext_mimes[0]), sizeof(ext_mimes[0]),
                mime_comp);
            ret = copy(elem ? elem[1] : "application/octet-stream");
            free(pt);
        }
    } else {
        ret = copy(mres);
    }
    g_free(mres);
    return ret;
}
