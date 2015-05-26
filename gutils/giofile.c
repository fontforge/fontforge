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
#include <basics.h>
#include "giofuncP.h"
#include <gfile.h>
#include "string.h"
#include <ustring.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <glib.h>
#include <glib/gstdio.h>

/* the initial space is so that these guys will come first in ordered error */
/*  lists in the file chooser */
static unichar_t err401[] = { ' ','U','n','a','u','t','h','o','r','i','z','e','d', '\0' };
static unichar_t err403[] = { ' ','F','o','r','b','i','d','d','e','n', '\0' };
static unichar_t err404[] = { ' ','N','o','t',' ','F','o','u','n','d', '\0' };
static unichar_t err405[] = { ' ','M','e','t','h','o','d',' ','N','o','t',' ','A','l','l','o','w','e','d', '\0' };
static unichar_t err406[] = { ' ','N','o','t',' ','A','c','c','e','p','t','a','b','l','e', '\0' };
static unichar_t err409[] = { ' ','C','o','n','f','l','i','c','t', '\0' };
static unichar_t err412[] = { ' ','P','r','e','c','o','n','d','i','t','i','o','n',' ','F','a','i','l','e','d', '\0' };
static unichar_t err414[] = { ' ','R','e','q','u','e','s','t','-','U','R','I',' ','T','o','o',' ','L','o','n','g', '\0' };
static unichar_t err500[] = { ' ','I','n','t','e','r','n','a','l',' ','S','e','r','v','e','r',' ','E','r','r','o','r', '\0' };

void _GIO_reporterror(GIOControl *gc, int errn) {

    uc_strncpy(gc->status,strerror(errn),sizeof(gc->status)/sizeof(unichar_t));

    if ( errn==ENOENT || (gc->gf!=gf_dir && errn==ENOTDIR) ) {
	gc->return_code = 404;
	gc->error = err404;
    } else if ( errn==EACCES || errn==EPERM ) {
	gc->return_code = 401;
	gc->error = err401;
    } else if ( errn==EROFS || errn==ENOTEMPTY || errn==EBUSY ) {
	gc->return_code = 403;
	gc->error = err403;
    } else if ( errn==ENOTDIR || errn==EISDIR ) {
	gc->return_code = 405;
	gc->error = err405;
    } else if ( errn==EINVAL ) {
	gc->return_code = 406;
	gc->error = err406;
    } else if ( errn==EEXIST ) {
	gc->return_code = 409;
	gc->error = err409;
    } else if ( errn==ENOSPC || errn==EXDEV || errn==EMLINK) {
	gc->return_code = 412;
	gc->error = err412;
    } else if ( errn==ENAMETOOLONG ) {
	gc->return_code = 414;
	gc->error = err414;
    } else {
	gc->return_code = 500;
	gc->error = err500;
    }
    gc->done = true;
    (gc->receiveerror)(gc);
}

/**
 * Converts a GLib error code into the corresponding errno.
 * 
 * @param [in] error The GLib error struct.
 * @return The converted errno value.
 */
static int _gio_gerror_to_errno(GError *error) {
    if (error) {
        if (error->domain == G_FILE_ERROR) {
            switch(error->code) {
                case G_FILE_ERROR_EXIST: return EEXIST;
                case G_FILE_ERROR_ISDIR: return EISDIR;
                case G_FILE_ERROR_ACCES: return EACCES;
                case G_FILE_ERROR_NAMETOOLONG: return ENAMETOOLONG;
                case G_FILE_ERROR_NOENT: return ENOENT;
                case G_FILE_ERROR_NOTDIR: return ENOTDIR;
                case G_FILE_ERROR_ROFS: return EROFS;
                case G_FILE_ERROR_NOSPC: return ENOSPC;
                case G_FILE_ERROR_INVAL: return EINVAL;
                case G_FILE_ERROR_PERM: return EPERM;
            }
        }
        return ENOENT;
    }
    return 0;
}

/**
 * Create a directory entry given the path and its name.
 * 
 * @param [in] path The path to the directory containing the item
 * @param [in] name The item name within the directory specified by `path`.
 * @return The newly allocated directory entry.
 */
static GDirEntry* _gio_create_dirent(const char *path, const char *name) {
    GDirEntry *cur = (GDirEntry *) calloc(1, sizeof(GDirEntry));
    gchar *ent_path = g_build_path("/", path, name, NULL);
    GStatBuf statb;
    
    cur->name = fsys2u_copy(name);
    if (!g_stat(ent_path, &statb)) {
        cur->hasdir = cur->hasexe = cur->hasmode = cur->hassize = cur->hastime = true;
        cur->size = statb.st_size;
        cur->mode = statb.st_mode;
        cur->modtime = statb.st_mtime;
        cur->isdir = S_ISDIR(cur->mode);
        cur->isexe = !cur->isdir && (!cur->mode & 0100);
        
        // Things go badly if we open a pipe or a device. So we don't.
#ifdef __MINGW32__
        //Symlinks behave differently on Windows and are transparent, so no S_ISLNK.
        if (S_ISREG(statb.st_mode) || S_ISDIR(statb.st_mode)) {
#else
        if (S_ISREG(statb.st_mode) || S_ISDIR(statb.st_mode) || S_ISLNK(statb.st_mode)) {
#endif
            char *temp;
            // We look at the file and try to determine a MIME type.
            if ((temp=GIOguessMimeType(ent_path)) || (temp=GIOGetMimeType(ent_path))) {
                cur->mimetype = u_copy(c_to_u(temp));
                free(temp);
            }
        }
    } //g_stat
    
    g_free(ent_path);
    return cur;
}

/**
 * Scans the specified directory and updates the GIOcontrol accordingly.
 * 
 * @param [in,out] gc The control to update
 * @param [in] path The directory to scan
 */
static void _gio_file_dir(GIOControl *gc, char *path) {
    GDirEntry *head=NULL, *last=NULL, *cur;
    GDir *dir = NULL;
    GError *error;
    const gchar *ent_name;
    
    dir = g_dir_open(path, 0, &error);
    if (dir == NULL) {
        _GIO_reporterror(gc, _gio_gerror_to_errno(error));
        g_error_free(error);
        return;
    }
    
    //Only add the '..' if we're not at the root directory.
    if (!(ent_name = g_path_skip_root(path)) || *ent_name != '\0') {
        head = last = _gio_create_dirent(path, "..");
    }
    
    while ((ent_name = g_dir_read_name(dir))) {
        cur = _gio_create_dirent(path, ent_name);
        if (last == NULL) {
            head = last = cur;
        } else {
            last->next = cur;
            last = cur;
        }
    }

#if __CygWin
    /* Under cygwin we should give the user access to /cygdrive, even though */
    /*  a diropen("/") will not find it */
    if (strcmp(path, "/") == 0) {
        cur = _gio_create_dirent("/", "cygdrive");
        if (last == NULL) {
            head = last = cur;
        } else {
            last->next = cur;
            last = cur;
        }
    }
#endif
    
    g_dir_close(dir);
    
    gc->iodata = head;
    gc->direntrydata = true;
    gc->return_code = 200;
    gc->done = true;
    (gc->receivedata)(gc);
}

static void _gio_file_statfile(GIOControl *gc,char *path) {
    GDirEntry *cur;
    GStatBuf statb;

    if ( g_stat(path,&statb)==-1 ) {
	_GIO_reporterror(gc,errno);
    } else {
	cur = (GDirEntry *) calloc(1,sizeof(GDirEntry));
	cur->name = uc_copy(GFileNameTail(path));
	cur->hasdir = cur->hasexe = cur->hasmode = cur->hassize = cur->hastime = true;
	cur->size    = statb.st_size;
	cur->mode    = statb.st_mode;
	cur->modtime = statb.st_mtime;
	cur->isdir   = S_ISDIR(cur->mode);
	cur->isexe   = !cur->isdir && (cur->mode & 0100);
	gc->iodata = cur;
	gc->direntrydata = true;
	gc->return_code = 200;
	gc->done = true;
	(gc->receivedata)(gc);
    }
}

static void _gio_file_delfile(GIOControl *gc,char *path) {
    if ( g_unlink(path)==-1 ) {
	_GIO_reporterror(gc,errno);
    } else {
	gc->return_code = 201;
	gc->done = true;
	(gc->receivedata)(gc);
    }
}

static void _gio_file_deldir(GIOControl *gc,char *path) {
    if ( g_rmdir(path)==-1 ) {
	_GIO_reporterror(gc,errno);
    } else {
	gc->return_code = 201;
	gc->done = true;
	(gc->receivedata)(gc);
    }
}

static void _gio_file_renamefile(GIOControl *gc,char *path, char *topath) {
    if ( g_rename(path,topath)==-1 ) {
	_GIO_reporterror(gc,errno);
    } else {
	gc->return_code = 201;
	gc->done = true;
	(gc->receivedata)(gc);
    }
}

static void _gio_file_mkdir(GIOControl *gc,char *path) {
    if ( GFileMkDir(path)==-1 ) {
	_GIO_reporterror(gc,errno);
    } else {
	gc->return_code = 201;
	gc->done = true;
	(gc->receivedata)(gc);
    }
}

void _GIO_localDispatch(GIOControl *gc) {
    char *path = u2fsys_copy(gc->path);
    char *topath;

    switch ( gc->gf ) {
      case gf_dir:
	_gio_file_dir(gc,path);
      break;
      case gf_statfile:
	_gio_file_statfile(gc,path);
      break;
      case gf_mkdir:
	_gio_file_mkdir(gc,path);
      break;
      case gf_delfile:
	_gio_file_delfile(gc,path);
      break;
      case gf_deldir:
	_gio_file_deldir(gc,path);
      break;
      case gf_renamefile:
	topath = cu_copy(gc->topath);
	_gio_file_renamefile(gc,path,topath);
	free(topath);
      break;
      default:
      break;
    }
    free(path);
}

/* pathname preceded by "file://" just strip off the "file://" and treat as a */
/*  filename */
void *_GIO_fileDispatch(GIOControl *gc) {
    char *username, *password, *host, *path, *topath;
    int port;

    path = _GIO_decomposeURL(gc->path,&host,&port,&username,&password);
    free(host); free(username); free(password);
    switch ( gc->gf ) {
      case gf_dir:
	_gio_file_dir(gc,path);
      break;
      case gf_statfile:
	_gio_file_statfile(gc,path);
      break;
      case gf_mkdir:
	_gio_file_mkdir(gc,path);
      break;
      case gf_delfile:
	_gio_file_delfile(gc,path);
      break;
      case gf_deldir:
	_gio_file_deldir(gc,path);
      break;
      case gf_renamefile:
	topath = _GIO_decomposeURL(gc->topath,&host,&port,&username,&password);
	free(host); free(username); free(password);
	_gio_file_renamefile(gc,path,topath);
	free(topath);
      break;
      default:
      break;
    }
    free(path);
return( NULL );
}
