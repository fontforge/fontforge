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
#include "gfile.h"
#include "xvasprintf.h"
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
 * Checks the return status from the g_spawn family of functions.
 * 
 * @param [in] status The return status to check.
 * @return true iff the status indicates the program exited normally. 
 */
int GFileCheckGlibSpawnStatus(int status) {
#if GLIB_CHECK_VERSION(2,34,0)
    return g_spawn_check_exit_status(status, NULL);
#else
# ifdef __MINGW32__
    return status == 0;
# else
    return WIFEXITED(status);
# endif
#endif
}

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

const char *GFileGetHomeDir(void) {
    static gchar *home_dir = NULL;
    
    if (g_once_init_enter(&home_dir)) {
        gchar *tmp = g_strdup(g_get_home_dir());
        if (tmp) {
            GFileNormalizePath(tmp);
        }
        g_once_init_leave(&home_dir, tmp);
    }
    return home_dir;
}

unichar_t *u_GFileGetHomeDir(void) {
    unichar_t* dir = NULL;
    const char* tmp = GFileGetHomeDir();
    if( tmp ) {
	dir = fsys2u_copy(tmp);
    }
return dir;
}

/**
 * Checks if the specified program exists in the current PATH.
 * On Windows (MINGW), the executable name *must* include the '.exe' extension.
 * 
 * @param [in] prog The program to check its existence for.
 * @return true iff the program exists in the PATH.
 */
int GFileProgramExists(const char *prog) {
    char *path, *entry, *buffer, *pt;
#ifdef __MINGW32__
    const char sep = ';';
#else
    const char sep = ':';
#endif
    bool at_end = false, prog_found = false;

    if ((path = entry = copy(g_getenv("PATH"))) == NULL) {
        return false;
    }

    while (!at_end && !prog_found) {
        pt = strchr(entry, sep);
        if (pt == NULL) {
            at_end = true;
            pt = entry+strlen(entry);
        } else {
            *pt = '\0';
        }
        
        if ((buffer = g_build_filename(entry, prog, NULL))) {
            if (g_file_test(buffer, G_FILE_TEST_IS_EXECUTABLE)) {
                prog_found = true;
            }
            g_free(buffer);
        }
        entry = pt+1;
    }
    
    free(path);
    return prog_found;
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

char *GFileBuildName(const char *dir, const char *fname, char *buffer, size_t size) {
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
    return g_file_test(file, G_FILE_TEST_IS_DIR);
}

int GFileExists(const char *file) {
    return g_file_test(file, G_FILE_TEST_EXISTS);
}

int GFileReadable(const char *file) {
    return (g_access(file, R_OK) == 0);
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

int GFileMkDir(const char *name) {
return( g_mkdir(name,0755));
}

char *_GFile_find_program_dir(char *prog) {
    char *pt, *program_dir=NULL;
    const char *path;
    char filename[2000];

#if defined(__MINGW32__)
    char* pt1 = strrchr(prog, '/');
    char* pt2 = strrchr(prog, '\\');
    if(pt1<pt2) pt1=pt2;
    if(pt1)
	program_dir = copyn(prog, pt1-prog);
    else if( (path = g_getenv("PATH")) != NULL ){
	char* tmppath = copy(path);
	path = tmppath;
	for(;;){
	    pt1 = strchr(path, ';');
	    if(pt1) *pt1 = '\0';
	    sprintf(filename,"%s/%s", path, prog);
	    if ( g_file_test(filename, G_FILE_TEST_IS_EXECUTABLE) ) {
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
    else if ( (path = g_getenv("PATH"))!=NULL ) {
	while ((pt = strchr(path,':'))!=NULL ) {
	  sprintf(filename,"%.*s/%s", (int)(pt-path), path, prog);
	    /* Under cygwin, applying access to "potrace" will find "potrace.exe" */
	    /*  no need for special check to add ".exe" */
	    if ( g_file_test(filename, G_FILE_TEST_IS_EXECUTABLE) ) {
		program_dir = copyn(path,pt-path);
	break;
	    }
	    path = pt+1;
	}
	if ( program_dir==NULL ) {
	    sprintf(filename,"%s/%s", path, prog);
	    if ( g_file_test(filename, G_FILE_TEST_IS_EXECUTABLE) )
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
    u2fsys_strncpy(buffer,file,sizeof(buffer));
    return GFileIsDir(buffer);
}

int u_GFileExists(const unichar_t *file) {
    char buffer[1024];
    u2fsys_strncpy(buffer,file,sizeof(buffer));
return( g_file_test(buffer, G_FILE_TEST_EXISTS) );
}

int u_GFileReadable(unichar_t *file) {
    char buffer[1024];
    u2fsys_strncpy(buffer,file,sizeof(buffer));
return( g_access(buffer, R_OK)==0 );
}

int u_GFileMkDir(unichar_t *name) {
    char buffer[1024];
    u2fsys_strncpy(buffer,name,sizeof(buffer));
return( mkdir(buffer,0755));
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
    static gchar *program_dir = NULL;
    
    if (g_once_init_enter(&program_dir)) {
        gchar *ret = NULL;
#if defined(__MINGW32__)
        wchar_t path[MAX_PATH];
        DWORD len = GetModuleFileNameW(NULL, path, MAX_PATH);
        
        if (len < MAX_PATH) {
            gchar *ptr;
            
            ret = g_utf16_to_utf8(path, -1, NULL, NULL, NULL);
            GFileNormalizePath(ret);
            ptr = strrchr(ret, '/');
            if (ptr) {
                *ptr = '\0';
            }
        }
#else
        ret = _GFile_find_program_dir(prog);
        if (ret == NULL) {
            char filename[1025];
            GFileGetAbsoluteName(".", filename, sizeof(filename));
            ret = copy(filename);
        }
#endif        
        g_once_init_leave(&program_dir, ret);
    }
    
    GResourceProgramDir = program_dir;
    //return program_dir;
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

/* Find the directory in which FontForge places all of its configurations and
 * save files.  On Unix-likes, the argument `dir` (see the below case switch,
 * enum in inc/gfile.h) determines which directory is returned according to the
 * XDG Base Directory Specification.  On Windows, the argument is ignored--the
 * home directory as obtained by GFileGetHomeDir() appended with "/FontForge" is
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
	if (!(home = GFileGetHomeDir())) {
	/* if GFileGetHomeDir returns NULL, pass NULL to calling function */
	fprintf(stderr, "%s\n", "cannot find home directory");
return NULL;
	}
#ifdef _WIN32
	/* Allow for preferences to be saved locally in a 'portable' configuration. */ 
	if (g_getenv("FF_PORTABLE") != NULL) {
		buf = xasprintf("%s/preferences", getShareDir());
	} else {
        const char *appdata = g_getenv("APPDATA");
        if (appdata) {
            buf = xasprintf("%s/FontForge", appdata);
        } else {
            buf = xasprintf("%s/FontForge", home);
        }
	}
	return buf;
#else
	/* Home directory exists, so check for environment variables.  For each of
	 * XDG_{CACHE,CONFIG,DATA}_HOME, assign `def` as the corresponding fallback
	 * for if the environment variable does not exist. */
	switch(dir) {
	  case Cache:
	xdg = g_getenv("XDG_CACHE_HOME");
	def = ".cache";
	  break;
	  case Config:
	xdg = g_getenv("XDG_CONFIG_HOME");
	def = ".config";
	  break;
	  case Data:
	xdg = g_getenv("XDG_DATA_HOME");
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
	buf = xasprintf("%s/fontforge", xdg);
	else
	/* if, for example, XDG_CACHE_HOME does not exist, instead assign
	 * the value "$HOME/.cache/fontforge" */
	buf = xasprintf("%s/%s/fontforge", home, def);
	if(buf != NULL) {
	    /* try to create buf.  If creating the directory fails, return NULL
	     * because nothing will get saved into an inaccessible directory.  */
            if ( g_mkdir_with_parents(buf, 0755) != 0 ) {
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
    GStatBuf buf;
    if (g_lstat(name, &buf) == -1) {
        return -1;
    }
    return buf.st_size;
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

const char *GFileGetHomeDocumentsDir(void)
{
    static gchar *documents_dir = NULL;
    
    if (g_once_init_enter(&documents_dir)) {
        gchar *tmp = g_strconcat(g_get_user_special_dir(G_USER_DIRECTORY_DOCUMENTS),"/",NULL);
        if (tmp == NULL) {
            //The result of GFileGetHomeDir should not be freed, so no strdup.
            tmp = (gchar*)GFileGetHomeDir();
        } else {
            GFileNormalizePath(tmp);
        }
        g_once_init_leave(&documents_dir, tmp);
    }
    return documents_dir;
}

unichar_t *u_GFileGetHomeDocumentsDir(void) {
    unichar_t* dir = NULL;
    const char* tmp = GFileGetHomeDocumentsDir();
    if(tmp) {
        dir = fsys2u_copy(tmp);
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

