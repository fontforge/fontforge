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

#include <assert.h>
#include <dirent.h>
#include <errno.h>

#include "ffglib.h"
#include "gdraw.h"
#include "gdrawP.h"
#include "gfile.h"
#include "ggadgetP.h"
#include "gicons.h"
#include "gwidgetP.h"
#include "ustring.h"
#include "utype.h"
#ifdef WIN32
#include <windows.h>

// Returns "CDQ" if C:\, D:\ and Q:\ are available.
// This could probably be put in fsys.c, but we don't need it anywhere else,
// and shouldn't ever.
static const int WIN32_DRIVES_MAXLEN = 26;
static char* _DriveLetters() {
    int idx = -1;
    char* ret = calloc(WIN32_DRIVES_MAXLEN+1,sizeof(char));
    DWORD drives = GetLogicalDrives();
    assert(drives > 0);
    for (int d = 0; d < WIN32_DRIVES_MAXLEN; d++) {
        if ( (drives&(1 << d)) ) {
            ret[++idx] = 'A'+d;
        }
    }
    return ret;
}
#endif

/* This isn't really a gadget, it's just a collection of gadgets with some glue*/
/*  to make them work together. Therefore there are no expose routines here, */
/*  nor mouse, nor key. That's all handled by the individual gadgets themselves*/
/* Instead we've got a bunch of routines that make them work as a whole */
static GBox gfilechooser_box = GBOX_EMPTY; /* no box */
static unichar_t *lastdir;
static int showhidden = false;
static enum { dirs_mixed, dirs_first, dirs_separate } dir_placement = dirs_mixed;
static unichar_t **bookmarks = NULL;
static void *prefs_changed_data = NULL;
static void (*prefs_changed)(void *) = NULL;

static unichar_t *SubMatch(unichar_t *pattern, unichar_t *eop, unichar_t *name,int ignorecase) {
    unichar_t ch, *ppt, *npt, *ept, *eon;

    while ( pattern<eop && ( ch = *pattern)!='\0' ) {
	if ( ch=='*' ) {
	    if ( pattern[1]=='\0' )
return( name+u_strlen(name));
	    for ( npt=name; ; ++npt ) {
		if ( (eon = SubMatch(pattern+1,eop,npt,ignorecase))!= NULL )
return( eon );
		if ( *npt=='\0' )
return( NULL );
	    }
	} else if ( ch=='?' ) {
	    if ( *name=='\0' )
return( NULL );
	    ++name;
	} else if ( ch=='[' ) {
	    /* [<char>...] matches the chars
	     * [<char>-<char>...] matches any char within the range (inclusive)
	     * the above may be concattenated and the resultant pattern matches
	     *		anything thing which matches any of them.
	     * [^<char>...] matches any char which does not match the rest of
	     *		the pattern
	     * []...] as a special case a ']' immediately after the '[' matches
	     *		itself and does not end the pattern
	     */
	    int found = 0, not=0;
	    ++pattern;
	    if ( pattern[0]=='^' ) { not = 1; ++pattern; }
	    for ( ppt = pattern; (ppt!=pattern || *ppt!=']') && *ppt!='\0' ; ++ppt ) {
		ch = *ppt;
		if ( ppt[1]=='-' && ppt[2]!=']' && ppt[2]!='\0' ) {
		    unichar_t ch2 = ppt[2];
		    if ( (*name>=ch && *name<=ch2) ||
			    (ignorecase && islower(ch) && islower(ch2) &&
				    *name>=toupper(ch) && *name<=toupper(ch2)) ||
			    (ignorecase && isupper(ch) && isupper(ch2) &&
				    *name>=tolower(ch) && *name<=tolower(ch2))) {
			if ( !not ) {
			    found = 1;
	    break;
			}
		    } else {
			if ( not ) {
			    found = 1;
	    break;
			}
		    }
		    ppt += 2;
		} else if ( ch==*name || (ignorecase && tolower(ch)==tolower(*name)) ) {
		    if ( !not ) {
			found = 1;
	    break;
		    }
		} else {
		    if ( not ) {
			found = 1;
	    break;
		    }
		}
	    }
	    if ( !found )
return( NULL );
	    while ( *ppt!=']' && *ppt!='\0' ) ++ppt;
	    pattern = ppt;
	    ++name;
	} else if ( ch=='{' ) {
	    /* matches any of a comma separated list of substrings */
	    for ( ppt = pattern+1; *ppt!='\0' ; ppt = ept ) {
		for ( ept=ppt; *ept!='}' && *ept!=',' && *ept!='\0'; ++ept );
		npt = SubMatch(ppt,ept,name,ignorecase);
		if ( npt!=NULL ) {
		    unichar_t *ecurly = ept;
		    while ( *ecurly!='}' && ecurly<eop && *ecurly!='\0' ) ++ecurly;
		    if ( (eon=SubMatch(ecurly+1,eop,npt,ignorecase))!=NULL )
return( eon );
		}
		if ( *ept=='}' )
return( NULL );
		if ( *ept==',' ) ++ept;
	    }
	} else if ( ch==*name ) {
	    ++name;
	} else if ( ignorecase && tolower(ch)==tolower(*name)) {
	    ++name;
	} else
return( NULL );
	++pattern;
    }
    if (pattern > eop) {
        return NULL;
    }
return( name );
}

/* Handles *?{}[] wildcards */
int GGadgetWildMatch(unichar_t *pattern, unichar_t *name,int ignorecase) {
    if ( pattern==NULL )
return( true );

    unichar_t *eop = pattern + u_strlen(pattern);

    name = SubMatch(pattern,eop,name,ignorecase);
    if ( name==NULL )
return( false );
    if ( *name=='\0' )
return( true );

return( false );
}

static int GGadgetWildMatch8(unichar_t *pattern, const char *name,int ignorecase) {
    unichar_t *uname = utf82u_copy(name);
    int ret = GGadgetWildMatch(pattern, uname, ignorecase);
    free(uname);
    return ret;
}

enum fchooserret GFileChooserDefFilter(GGadget *g,const struct gdirentry *ent,const char *dir) {
    GFileChooser *gfc = (GFileChooser *) g;
    int i;

    if ( strcmp(ent->name,".")==0 )	/* Don't show the current directory entry */
	return( fc_hide );
    if ( gfc->wildcard!=NULL && *gfc->wildcard=='.' )
	/* If they asked for hidden files, show them */;
    else if ( !showhidden && ent->name[0]=='.' && strcmp(ent->name,"..")!=0 )
	return( fc_hide );
    if ( ent->isdir )			/* Show all other directories */
	return( fc_show );
    if ( gfc->wildcard==NULL && gfc->mimetypes==NULL )
	return( fc_show );
    /* If we've got a wildcard, and it matches, then show */
    if ( gfc->wildcard!=NULL && GGadgetWildMatch8(gfc->wildcard,ent->name,true))
	return( fc_show );
    /* If no mimetypes then we're done */
    if ( gfc->mimetypes==NULL )
	return( fc_hide );
    /* match the mimetypes */
    if ( ent->mimetype ) {
	for ( i=0; gfc->mimetypes[i]!=NULL; ++i )
	    if ( strcasecmp(u_to_c(gfc->mimetypes[i]),ent->mimetype)==0 ) {
		return( fc_show );
	    }
    }

    return( fc_hide );
}

static GImage *GFileChooserPickIcon(const struct gdirentry *e) {
    InitChooserIcons();

    if ( e->isdir ) {
	if ( !strcmp(e->name,"..") )
	    return( &_GIcon_updir );
	return( &_GIcon_dir );
    } else if ( !e->mimetype ) {
	return( &_GIcon_unknown );
    }
    if (strncasecmp("text/", e->mimetype, 5) == 0) {
	if (strcasecmp("text/html", e->mimetype) == 0)
	    return( &_GIcon_texthtml );
	if (strcasecmp("text/xml", e->mimetype) == 0)
	    return( &_GIcon_textxml );
	if (strcasecmp("text/css", e->mimetype) == 0)
	    return( &_GIcon_textcss );
	if (strcasecmp("text/c", e->mimetype) == 0)
	    return( &_GIcon_textc );
	if (strcasecmp("text/java", e->mimetype) == 0)
	    return( &_GIcon_textjava );
	if (strcasecmp("text/x-makefile", e->mimetype) == 0)
	    return( &_GIcon_textmake );
	if (strcasecmp("application/x-font-type1", e->mimetype) == 0)
	    return( &_GIcon_textfontps );
	if (strcasecmp("text/font", e->mimetype) == 0)
	    return( &_GIcon_textfontbdf );
	if (strcasecmp("text/ps", e->mimetype) == 0)
	    return( &_GIcon_textps );

	return( &_GIcon_textplain );
    }

    if (strncasecmp("image/", e->mimetype, 6) == 0)
	return( &_GIcon_image );
    if (strncasecmp("video/", e->mimetype, 6) == 0)
	return( &_GIcon_video );
    if (strncasecmp("audio/", e->mimetype, 6) == 0)
	return( &_GIcon_audio );
    if (strcasecmp("application/x-navidir", e->mimetype) == 0 ||
	strcasecmp("inode/directory", e->mimetype) == 0)
	return( &_GIcon_dir );
    if (strcasecmp("application/x-object", e->mimetype) == 0)
	return( &_GIcon_object );
    if (strcasecmp("application/x-core", e->mimetype) == 0)
	return( &_GIcon_core );
    if (strcasecmp("application/x-tar", e->mimetype) == 0)
	return( &_GIcon_tar );
    if (strcasecmp("application/x-compressed", e->mimetype) == 0)
	return( &_GIcon_compressed );
    if (strcasecmp("application/pdf", e->mimetype) == 0)
	return( &_GIcon_texthtml );
    if (strcasecmp("application/vnd.font-fontforge-sfd", e->mimetype) == 0)
	return( &_GIcon_textfontsfd );
    if (strcasecmp("application/x-font-type1", e->mimetype) == 0)
	return( &_GIcon_textfontps );
    if (strcasecmp("application/x-font-ttf", e->mimetype) == 0 ||
	strcasecmp("application/x-font-otf", e->mimetype) == 0 ||
	strcasecmp("font/woff", e->mimetype) == 0 ||
	strcasecmp("font/woff2", e->mimetype) == 0 ||
	strcasecmp("font/ttf", e->mimetype) == 0 ||
	strcasecmp("font/otf", e->mimetype) == 0) {
	return( &_GIcon_ttf );
    }
    if (strcasecmp("application/x-font-cid", e->mimetype) == 0 )
	return( &_GIcon_cid );
    if (strcasecmp("application/x-macbinary", e->mimetype) == 0 ||
	strcasecmp("application/x-mac-binhex40", e->mimetype) == 0 ) {
	return( &_GIcon_mac );
    }
    if (strcasecmp("application/x-mac-dfont", e->mimetype) == 0 ||
	strcasecmp("application/x-mac-suit", e->mimetype) == 0 ) {
	return( &_GIcon_macttf );
    }
    if (strcasecmp("application/x-font-pcf", e->mimetype) == 0 ||
	strcasecmp("application/x-font-snf", e->mimetype) == 0) {
	return( &_GIcon_textfontbdf );
    }

    return( &_GIcon_unknown );
}

static void GFileChooserFillList(GFileChooser *gfc, const unichar_t *dirname_) {
    DIR *dir;
    char *dirname = u2def_copy(dirname_);
    struct dirent *dent;

    if (!dirname || (dir = opendir(dirname)) == NULL) {
        GTextInfo *ti[2], _ti[2] = { 0 };
        static unichar_t nullstr[] = { 0 };

        _ti[0].text = def2u_copy(strerror(errno));
        _ti[0].fg = _ti[0].bg = COLOR_DEFAULT;
        ti[0] = _ti; ti[1] = _ti+1;
        GGadgetSetEnabled(&gfc->files->g, false);
        GGadgetSetList(&gfc->files->g, ti, true);
        GGadgetSetEnabled(&gfc->subdirs->g, false);
        GGadgetSetList(&gfc->subdirs->g, ti, true);
        free(_ti[0].text);
        if (gfc->lastname != NULL) {
            GGadgetSetTitle(&gfc->name->g, gfc->lastname);
            free(gfc->lastname);
            gfc->lastname=NULL;
        } else {
            GGadgetSetTitle(&gfc->name->g, nullstr);
        }

        if (gfc->filterb != NULL && gfc->ok != NULL) {
            _GWidget_MakeDefaultButton(&gfc->ok->g);
        }
        free(dirname);
        return;
    }

    if (gfc->lastname!=NULL) {
        GGadgetSetTitle(&gfc->name->g, gfc->lastname);
        free(gfc->lastname);
        gfc->lastname=NULL;
    }

    GPtrArray *ti = g_ptr_array_new();
    GPtrArray *dti = (dir_placement == dirs_separate) ? g_ptr_array_new() : NULL;
    while ((dent = readdir(dir)) != NULL) {
        struct gdirentry ent;
        if (!strcmp(dent->d_name, ".")) {
            continue;
        }
        ent.fullpath = smprintf("%s/%s", dirname, dent->d_name);
        ent.name = dent->d_name;
        ent.mimetype = GFileMimeType(ent.fullpath);
        ent.isdir = GFileIsDir(ent.fullpath);

        int fcdata = (gfc->filter)(&gfc->g, &ent, dirname);
        if (fcdata != fc_hide) {
            GTextInfo *me = calloc(1, sizeof(GTextInfo));
            me->text = def2u_copy(dent->d_name);
            me->image = GFileChooserPickIcon(&ent);
            me->fg = COLOR_DEFAULT;
            me->bg = COLOR_DEFAULT;
            me->font = NULL;
            me->disabled = fcdata == fc_showdisabled;
            me->image_precedes = true;
            me->checked = ent.isdir;
            if (dir_placement == dirs_first && ent.isdir) {
                ((GTextInfo2 *)me)->sort_me_first_in_list = true;
            }
            g_ptr_array_add((dti && ent.isdir) ? dti : ti, me);
        }
        free((char*)ent.fullpath);
        free((char*)ent.mimetype);
    }
    closedir(dir);

    GTextInfo **rti = malloc((ti->len + 1) * sizeof(GTextInfo*));
    memcpy(rti, ti->pdata, ti->len * sizeof(GTextInfo*));
    rti[ti->len] = calloc(1, sizeof(GTextInfo));
    GGadgetSetList(&gfc->files->g, rti, false);
    g_ptr_array_free(ti, false);

    if (dti) {
        GTextInfo **rdti = malloc((dti->len + 1) * sizeof(GTextInfo*));
        memcpy(rdti, dti->pdata, dti->len * sizeof(GTextInfo*));
        rdti[dti->len] = calloc(1, sizeof(GTextInfo));
        GGadgetSetList(&gfc->subdirs->g, rdti, false);
        g_ptr_array_free(dti, false);
    }

    GGadgetScrollListToText(&gfc->files->g,u_GFileNameTail(_GGadgetGetTitle(&gfc->name->g)),true);
    GGadgetSetEnabled(&gfc->files->g, true);
    GGadgetSetEnabled(&gfc->subdirs->g, true);
    free(dirname);
}

static void GFileChooserScanDir(GFileChooser *gfc,unichar_t *dir) {
    GTextInfo **ti=NULL;
    int cnt, tot=0;
#ifdef _WIN32
    char* drives = _DriveLetters();
    int dcnt = strlen(drives);
#endif
    unichar_t *pt, *ept, *freeme;

    dir = u_GFileNormalize(dir);
    while ( 1 ) {
	ept = dir;
	cnt = 0;
	if ( (pt=uc_strstr(dir,"://"))!=NULL ) {
	    ept = u_strchr(pt+3,'/');
	    if ( ept==NULL )
		ept = pt+u_strlen(pt);
	    else
		++ept;
	} else if ( *dir=='/' )
	    ept = dir+1;
	if ( ept!=dir ) {
	    if ( ti!=NULL ) {
		ti[tot-cnt] = calloc(1,sizeof(GTextInfo));
		ti[tot-cnt]->text = u_copyn(dir,ept-dir);
		ti[tot-cnt]->fg = ti[tot-cnt]->bg = COLOR_DEFAULT;
	    }
	    cnt = 1;
	}
	for ( pt = ept; *pt!='\0'; pt = ept ) {
	    for ( ept = pt; *ept!='/' && *ept!='\0'; ++ept );
	    if ( ti!=NULL ) {
		ti[tot-cnt] = calloc(1,sizeof(GTextInfo));
		ti[tot-cnt]->text = u_copyn(pt,ept-pt);
		ti[tot-cnt]->fg = ti[tot-cnt]->bg = COLOR_DEFAULT;
	    }
	    ++cnt;
	    if ( *ept=='/' ) ++ept;
	}
	if ( ti!=NULL )
    break;
	ti = malloc((cnt+1)*sizeof(GTextInfo *));
	tot = cnt-1;
    }

#ifdef _WIN32
    ti = realloc(ti, (dcnt+cnt+1)*sizeof(GTextInfo *));

    for (char* c = drives; *c != '\0'; c++) {
        // Don't put the root twice if we're already on it.
        if ((unichar_t)*c == toupper(dir[0])) continue;

        ti[cnt] = calloc(1,sizeof(GTextInfo));
        unichar_t* utemp = calloc(3,sizeof(unichar_t));
        utemp[0] = *c; utemp[1] = ':'; //utemp[2] = '\\'; // utemp[3] = 0;
        ti[cnt]->text = utemp;
        ti[cnt]->fg = ti[cnt]->bg = COLOR_DEFAULT;
        // We don't want to add a new field to GTextInfo as it's used in a lot of places.
        // This is the next best thing AFAICT.
        ti[cnt]->userdata = (void*)1;
        cnt++;
    }
    free(drives);
#endif
    ti[cnt] = calloc(1,sizeof(GTextInfo));

    GGadgetSetList(&gfc->directories->g,ti,false);
    GGadgetSelectOneListItem(&gfc->directories->g,0);

    freeme = NULL;
    if ( dir[u_strlen(dir)-1]!='/' ) {
	freeme = malloc((u_strlen(dir)+3)*sizeof(unichar_t));
	u_strcpy(freeme,dir);
	uc_strcat(freeme,"/");
	dir = freeme;
    }
    if ( gfc->hpos+1>=gfc->hmax ) {
	gfc->hmax = gfc->hmax+20;
	gfc->history = realloc(gfc->history,(gfc->hmax)*sizeof(unichar_t *));
    }
    if ( gfc->hcnt==0 ) {
	gfc->history[gfc->hcnt++] = u_copy(dir);
    } else if ( u_strcmp(gfc->history[gfc->hpos],dir)==0 )
	/* Just a refresh */;
    else {
	gfc->history[++gfc->hpos] = u_copy(dir);
	gfc->hcnt = gfc->hpos+1;
    }

    GFileChooserFillList(gfc, dir);
    free(freeme);
}

/* Handle events from the text field */
static int GFileChooserTextChanged(GGadget *t,GEvent *e) {
    GFileChooser *gfc;
    GGadget *g = (GGadget *)GGadgetGetUserData(t);

    const unichar_t *pt, *spt;
    unichar_t * pt_toFree = 0, *local_toFree = 0;
    if ( e->type!=et_controlevent || e->u.control.subtype!=et_textchanged )
return( true );
    spt = pt = _GGadgetGetTitle(t);
#ifdef _WIN32
    local_toFree = u_GFileNormalizePath(u_copy(spt));
    pt = spt = local_toFree;
#endif    
    
    if ( pt==NULL )
return( true );
    gfc = (GFileChooser *) GGadgetGetUserData(t);

    if( GFileChooserGetInputFilenameFunc(g)(g, &pt,gfc->inputfilenameprevchar)) {
	pt_toFree = (unichar_t*)pt;
	spt = pt;
	GGadgetSetTitle(g, pt);
    }

    while ( *pt && *pt!='*' && *pt!='?' && *pt!='[' && *pt!='{' )
	++pt;
    if ( *spt!='\0' && spt[u_strlen(spt)-1]=='/' )
	pt = spt+u_strlen(spt)-1;

    /* if there are no wildcards and no directories in the filename */
    /*  then as it gets changed the list box should show the closest */
    /*  approximation to it */
    if ( *pt=='\0' ) {
	GGadgetScrollListToText(&gfc->files->g,spt,true);
	if ( gfc->filterb!=NULL && gfc->ok!=NULL )
	    _GWidget_MakeDefaultButton(&gfc->ok->g);
    } else if ( *pt=='/' && e->u.control.u.tf_changed.from_pulldown!=-1 ) {
	GEvent e;
	e.type = et_controlevent;
	e.u.control.subtype = et_buttonactivate;
	e.u.control.g = (gfc->ok!=NULL?&gfc->ok->g:&gfc->g);
	e.u.control.u.button.clicks = 0;
	e.w = e.u.control.g->base;
	if ( e.u.control.g->handle_controlevent != NULL )
	    (e.u.control.g->handle_controlevent)(e.u.control.g,&e);
	else
	    GDrawPostEvent(&e);
    } else {
	if ( gfc->filterb!=NULL && gfc->ok!=NULL )
	    _GWidget_MakeDefaultButton(&gfc->filterb->g);
    }
    free(gfc->lastname); gfc->lastname = NULL;
    if(pt_toFree)
	free(pt_toFree);
    
    if (local_toFree)
        free(local_toFree);

    if(gfc->inputfilenameprevchar)
	free(gfc->inputfilenameprevchar);
    gfc->inputfilenameprevchar = u_copy(_GGadgetGetTitle(t));

return( true );
}

static unichar_t **GFileChooserCompletion(GGadget *t,int from_tab) {
    GFileChooser *gfc;
    const unichar_t *pt, *spt; unichar_t **ret;
    GTextInfo **ti;
    int32_t len;
    int i, cnt, doit, match_len;

    pt = spt = _GGadgetGetTitle(t);
    if ( pt==NULL )
return( NULL );
    while ( *pt && *pt!='/' && *pt!='*' && *pt!='?' && *pt!='[' && *pt!='{' )
	++pt;
    if ( *pt!='\0' )
return( NULL );		/* Can't complete if not in cur directory or has wildcards */

    match_len = u_strlen(spt);
    gfc = (GFileChooser *) GGadgetGetUserData(t);
    ti = GGadgetGetList(&gfc->files->g,&len);
    ret = NULL;
    for ( doit=0; doit<2; ++doit ) {
	cnt=0;
	for ( i=0; i<len; ++i ) {
	    if ( u_strncmp(ti[i]->text,spt,match_len)==0 ) {
		if ( doit ) {
		    if ( ti[i]->checked /* isdirectory */ ) {
			int len = u_strlen(ti[i]->text);
			ret[cnt] = malloc((len+2)*sizeof(unichar_t));
			u_strcpy(ret[cnt],ti[i]->text);
			ret[cnt][len] = '/';
			ret[cnt][len+1] = '\0';
		    } else
			ret[cnt] = u_copy(ti[i]->text);
		}
		++cnt;
	    }
	}
	if ( doit )
	    ret[cnt] = NULL;
	else if ( cnt==0 )
return( NULL );
	else
	    ret = malloc((cnt+1)*sizeof(unichar_t *));
    }
return( ret );
}

static unichar_t *GFileChooserGetCurDir(GFileChooser *gfc,int dirindex) {
    GTextInfo **ti;
    int32_t len; int j, cnt;
    unichar_t *dir, *pt;

    ti = GGadgetGetList(&gfc->directories->g,&len);
    if ( dirindex==-1 )
        dirindex = 0;
    dirindex = dirindex;

    for ( j=len-1,cnt=0; j>=dirindex; --j ) {
        if (ti[j]->userdata != NULL) 
            continue;
        cnt += u_strlen(ti[j]->text)+1;
    }
    pt = dir = malloc((cnt+1)*sizeof(unichar_t));
    for ( j=len-1; j>=dirindex; --j ) {
        if (ti[j]->userdata != NULL) 
            continue;
        u_strcpy(pt,ti[j]->text);
        pt += u_strlen(pt);
        if ( pt[-1]!='/' )
            *pt++ = '/';
    }
    *pt = '\0';
return( dir );
}

static void GFileChooserRefresh(GFileChooser *gfc) {
    unichar_t *dir;

    dir = GFileChooserGetCurDir(gfc,-1);
    GFileChooserScanDir(gfc,dir);
    free(dir);
}

static void GFileChooserSetTitle(GGadget *g,const unichar_t *tit);

/* Handle events from the directory dropdown list */
static int GFileChooserDListChanged(GGadget *pl,GEvent *e) {
    GFileChooser *gfc;
    int i;
    unichar_t *dir;

    if ( e->type!=et_controlevent || e->u.control.subtype!=et_listselected )
return( true );
    i = GGadgetGetFirstListSelectedItem(pl);
    if ( i==-1 )
return(true);
    gfc = (GFileChooser *) GGadgetGetUserData(pl);
#ifdef _WIN32
    int len;
    GTextInfo **ti;
    ti = GGadgetGetList(pl,&len);
    if (ti[i]->userdata != NULL) {
        GFileChooserSetTitle((GGadget*)gfc, ti[i]->text);
        GFileChooserRefresh(gfc);
return( true );
    }
#endif
    if ( i==0 )		/* Nothing changed */
return( true );
    dir = GFileChooserGetCurDir(gfc,i);
    GFileChooserScanDir(gfc,dir);
    free(dir);
return( true );
}

/* Handle events from the file list list */
static int GFileChooserFListSelected(GGadget *gl,GEvent *e) {
    GFileChooser *gfc;
    int i;
    int32_t listlen; int len, cnt, dirpos, apos;
    unichar_t *dir, *newdir;
    GTextInfo *ti, **all;

    if ( e->type!=et_controlevent ||
	    ( e->u.control.subtype!=et_listselected &&
	      e->u.control.subtype!=et_listdoubleclick ))
return( true );
    if ( ((GList *) gl)->multiple_sel ) {
	all = GGadgetGetList(gl,&listlen);
	len = cnt = 0;
	dirpos = apos = -1;
	for ( i=0; i<listlen; ++i ) {
	    if ( !all[i]->selected )
		/* Not selected? ignore it */;
	    else if ( all[i]->checked )		/* Directory */
		dirpos = i;
	    else {
		len += u_strlen( all[i]->text ) + 2;
		++cnt;
		apos = i;
	    }
	}
	if ( dirpos!=-1 && cnt>0 ) {
	    /* If a directory was selected, clear everything else */
	    for ( i=0; i<listlen; ++i ) if ( i!=dirpos )
		all[i]->selected = false;
	    _ggadget_redraw(gl);
	}
	if ( dirpos!=-1 ) { cnt = 1; apos = dirpos; }
    } else {
	cnt = 1;
	apos = GGadgetGetFirstListSelectedItem(gl);
    }
    if ( apos==-1 )
return(true);
    gfc = (GFileChooser *) GGadgetGetUserData(gl);
    ti = GGadgetGetListItem(gl,apos);
    if ( e->u.control.subtype==et_listselected && cnt==1 ) {
	/* Nope, quite doesn't work. Goal is to remember first filename. But */
	/*  if user types into the list box we'll (probably) get several diff*/
	/*  filenames before we hit the directory. So we just ignore the typ-*/
	/*  ing case for now. */
	if ( ti->checked /* it's a directory */ && e->u.control.u.list.from_mouse &&
		gfc->lastname==NULL )
	    gfc->lastname = GGadgetGetTitle(&gfc->name->g);
	if ( ti->checked ) {
	    unichar_t *val = malloc((u_strlen(ti->text)+2)*sizeof(unichar_t));
	    u_strcpy(val,ti->text);
	    uc_strcat(val,"/");
	    GGadgetSetTitle(&gfc->name->g,val);
	    free(val);
	    if ( gfc->filterb!=NULL && gfc->ok!=NULL )
		_GWidget_MakeDefaultButton(&gfc->filterb->g);
	} else {
	    GGadgetSetTitle(&gfc->name->g,ti->text);
	    if ( gfc->filterb!=NULL && gfc->ok!=NULL )
		_GWidget_MakeDefaultButton(&gfc->ok->g);
	    free(gfc->lastname);
	    gfc->lastname = NULL;
	}
    } else if ( e->u.control.subtype==et_listselected ) {
	unichar_t *val, *upt = malloc((len+1)*sizeof(unichar_t));
	val = upt;
	for ( i=0; i<listlen; ++i ) {
	    if ( all[i]->selected ) {
		u_strcpy(upt,all[i]->text);
		upt += u_strlen(upt);
		if ( --cnt>0 ) {
		    uc_strcpy(upt,"; ");
		    upt += 2;
		}
	    }
	}
	GGadgetSetTitle(&gfc->name->g,val);
	free(val);
    } else if ( ti->checked /* it's a directory */ ) {
	dir = GFileChooserGetCurDir(gfc,-1);
	newdir = u_GFileAppendFile(dir,ti->text,true);
	GFileChooserScanDir(gfc,newdir);
	free(dir); free(newdir);
    } else {
	/* Post the double click (on a file) to the parent */
	/*  if we know what the ok button is then pretend it got pressed */
	/*  otherwise just send a list double click from ourselves */
	if ( gfc->ok!=NULL ) {
	    e->u.control.subtype = et_buttonactivate;
	    e->u.control.g = &gfc->ok->g;
	    e->u.control.u.button.clicks = 0;
	    e->w = e->u.control.g->base;
	} else
	    e->u.control.g = &gfc->g;
	if ( e->u.control.g->handle_controlevent != NULL )
	    (e->u.control.g->handle_controlevent)(e->u.control.g,e);
	else
	    GDrawPostEvent(e);
    }
return( true );
}

static void GFCHideToggle(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    GFileChooser *gfc = (GFileChooser *) (mi->ti.userdata);
    unichar_t *dir;

    showhidden = !showhidden;

    dir = GFileChooserGetCurDir(gfc,-1);
    GFileChooserScanDir(gfc,dir);
    free(dir);

    if ( prefs_changed!=NULL )
	(prefs_changed)(prefs_changed_data);
}

static void GFCRemetric(GFileChooser *gfc) {
    GRect size;

    GGadgetGetSize(&gfc->topbox->g,&size);
    GGadgetResize(&gfc->topbox->g,size.width,size.height);
}

static void GFCDirsAmidToggle(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    GFileChooser *gfc = (GFileChooser *) (mi->ti.userdata);
    unichar_t *dir;

    if ( dir_placement==dirs_separate ) {
	GGadgetSetVisible(&gfc->subdirs->g,false);
	GFCRemetric(gfc);
    }
    dir_placement = dirs_mixed;

    dir = GFileChooserGetCurDir(gfc,-1);
    GFileChooserScanDir(gfc,dir);
    free(dir);

    if ( prefs_changed!=NULL )
	(prefs_changed)(prefs_changed_data);
}

static void GFCDirsFirstToggle(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    GFileChooser *gfc = (GFileChooser *) (mi->ti.userdata);
    unichar_t *dir;

    if ( dir_placement==dirs_separate ) {
	GGadgetSetVisible(&gfc->subdirs->g,false);
	GFCRemetric(gfc);
    }
    dir_placement = dirs_first;

    dir = GFileChooserGetCurDir(gfc,-1);
    GFileChooserScanDir(gfc,dir);
    free(dir);

    if ( prefs_changed!=NULL )
	(prefs_changed)(prefs_changed_data);
}

static void GFCDirsSeparateToggle(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    GFileChooser *gfc = (GFileChooser *) (mi->ti.userdata);
    unichar_t *dir;

    if ( dir_placement!=dirs_separate ) {
	GGadgetSetVisible(&gfc->subdirs->g,true);
	GFCRemetric(gfc);
    }
    dir_placement = dirs_separate;

    dir = GFileChooserGetCurDir(gfc,-1);
    GFileChooserScanDir(gfc,dir);
    free(dir);

    if ( prefs_changed!=NULL )
	(prefs_changed)(prefs_changed_data);
}

static void GFCRefresh(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    GFileChooser *gfc = (GFileChooser *) (mi->ti.userdata);
    GFileChooserRefresh(gfc);
}

static int GFileChooserHome(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	unichar_t* homedir = u_GFileGetHomeDocumentsDir();
	if ( homedir==NULL )
	    GGadgetSetEnabled(g,false);
	else {
	    GFileChooser *gfc = (GFileChooser *) GGadgetGetUserData(g);
	    GFileChooserScanDir(gfc,homedir);
	    free(homedir);
	}
    }
return( true );
}

static int GFileChooserUpDirButton(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GFileChooser *gfc = (GFileChooser *) GGadgetGetUserData(g);
	unichar_t *dir, *newdir;
	static unichar_t dotdot[] = { '.', '.',  0 };
	dir = GFileChooserGetCurDir(gfc,-1);
	newdir = u_GFileAppendFile(dir,dotdot,true);
	GFileChooserScanDir(gfc,newdir);
	free(dir); free(newdir);
    }
return( true );
}

static GMenuItem gfcpopupmenu[] = {
    { { (unichar_t *) N_("Show Hidden Files"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 0, 0, 'H' }, '\0', ksm_control, NULL, NULL, GFCHideToggle, 0 },
    { { (unichar_t *) N_("Directories Amid Files"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 0, 0, 'H' }, '\0', ksm_control, NULL, NULL, GFCDirsAmidToggle, 0 },
    { { (unichar_t *) N_("Directories First"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 0, 0, 'H' }, '\0', ksm_control, NULL, NULL, GFCDirsFirstToggle, 0 },
    { { (unichar_t *) N_("Directories Separate"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 0, 0, 'H' }, '\0', ksm_control, NULL, NULL, GFCDirsSeparateToggle, 0 },
    { { (unichar_t *) N_("Refresh File List"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, 'H' }, '\0', ksm_control, NULL, NULL, GFCRefresh, 0 },
    GMENUITEM_EMPTY
};
static int gotten=false;

static void GFCPopupMenu(GGadget *g, GEvent *e) {
    int i;
    // The reason this casting works is a bit of a hack.
    // If this was initiated from a right click, `g` will be the GFC GGadget.
    // Then its userdata would be the initiator's, and NOT the GFC struct.
    // Thus we cannot call GGadgetGetUserData.
    // However, since in the GFC struct, its GGadget comes first, effectively
    // the pointer to its GGadget will equal the pointer to its GFC struct.
    // I have ensured that all calls to this function pass the GFC GGadget.
    GFileChooser *gfc = (GFileChooser *) g;

    for ( i=0; gfcpopupmenu[i].ti.text!=NULL || gfcpopupmenu[i].ti.line; ++i )
	gfcpopupmenu[i].ti.userdata = gfc;
    gfcpopupmenu[0].ti.checked = showhidden;
    gfcpopupmenu[1].ti.checked = dir_placement == dirs_mixed;
    gfcpopupmenu[2].ti.checked = dir_placement == dirs_first;
    gfcpopupmenu[3].ti.checked = dir_placement == dirs_separate;
    if ( !gotten ) {
	gotten = true;
	for ( i=0; gfcpopupmenu[i].ti.text!=NULL || gfcpopupmenu[i].ti.line; ++i )
	    gfcpopupmenu[i].ti.text = (unichar_t *) _( (char *) gfcpopupmenu[i].ti.text);
    }
    GMenuCreatePopupMenu(g->base,e, gfcpopupmenu);
}

static int GFileChooserConfigure(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonpress ) {
	GEvent fake;
	GRect pos;
	GGadgetGetSize(g,&pos);
	memset(&fake,0,sizeof(fake));
	fake.type = et_mousedown;
	fake.w = g->base;
	fake.u.mouse.x = pos.x;
	fake.u.mouse.y = pos.y+pos.height;
	GFCPopupMenu((GGadget*)GGadgetGetUserData(g),&fake);
    }
return( true );
}

static void GFCBack(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    GFileChooser *gfc = (GFileChooser *) (mi->ti.userdata);

    if ( gfc->hpos<=0 )
return;
    --gfc->hpos;
    GFileChooserScanDir(gfc,gfc->history[gfc->hpos]);
}

static void GFCForward(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    GFileChooser *gfc = (GFileChooser *) (mi->ti.userdata);

    if ( gfc->hpos+1>=gfc->hcnt )
return;
    ++gfc->hpos;
    GFileChooserScanDir(gfc,gfc->history[gfc->hpos]);
}

static void GFCAddCur(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    GFileChooser *gfc = (GFileChooser *) (mi->ti.userdata);
    unichar_t *dir;
    int bcnt;

    dir = GFileChooserGetCurDir(gfc,-1);
    bcnt = 0;
    if ( bookmarks!=NULL )
	for ( ; bookmarks[bcnt]!=NULL; ++bcnt );
    bookmarks = realloc(bookmarks,(bcnt+2)*sizeof(unichar_t *));
    bookmarks[bcnt] = dir;
    bookmarks[bcnt+1] = NULL;

    if ( prefs_changed!=NULL )
	(prefs_changed)(prefs_changed_data);
}

static void GFCRemoveBook(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    int i,bcnt;
    char **books;
    char *sel;
    char *buts[2];

    if ( bookmarks==NULL || bookmarks[0]==NULL )
return;		/* All gone */
    for ( bcnt=0; bookmarks[bcnt]!=NULL; ++bcnt );
    sel = calloc(bcnt,1);
    books = calloc(bcnt+1,sizeof(char *));
    for ( bcnt=0; bookmarks[bcnt]!=NULL; ++bcnt )
	books[bcnt] = u2utf8_copy(bookmarks[bcnt]);
    books[bcnt] = NULL;
    buts[0] = _("_Remove");
    buts[1] = _("_Cancel");
    if ( GWidgetChoicesBM8( _("Remove bookmarks"),(const char **) books,sel,bcnt,buts,
	    _("Remove selected bookmarks"))==0 ) {
	for ( i=bcnt=0; bookmarks[bcnt]!=NULL; ++bcnt ) {
	    if ( sel[bcnt] ) {
		free(bookmarks[bcnt]);
	    } else {
		bookmarks[i++] = bookmarks[bcnt];
	    }
	}
	bookmarks[i] = NULL;

	if ( prefs_changed!=NULL )
	    (prefs_changed)(prefs_changed_data);
    }
    for ( i=0; books[i]!=NULL; ++i )
	free(books[i]);
    free(books);
    free(sel);
}

static void GFCBookmark(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    GFileChooser *gfc = (GFileChooser *) (mi->ti.userdata);
    char *home;

    if ( *bookmarks[mi->mid]=='~' && bookmarks[mi->mid][1]=='/' &&
	    (home = getenv("HOME"))!=NULL ) {
	unichar_t *space;
	space = malloc((strlen(home)+u_strlen(bookmarks[mi->mid])+2)*sizeof(unichar_t));
	uc_strcpy(space,home);
	u_strcat(space,bookmarks[mi->mid]+1);
	GFileChooserScanDir(gfc,space);
	free(space);
    } else
	GFileChooserScanDir(gfc,bookmarks[mi->mid]);
}

static void GFCPath(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    GFileChooser *gfc = (GFileChooser *) (mi->ti.userdata);
    char *home;

    if ( *gfc->paths[mi->mid]=='~' && gfc->paths[mi->mid][1]=='/' &&
	    (home = getenv("HOME"))!=NULL ) {
	unichar_t *space;
	space = malloc((strlen(home)+u_strlen(bookmarks[mi->mid])+2)*sizeof(unichar_t));
	uc_strcpy(space,home);
	u_strcat(space,gfc->paths[mi->mid]+1);
	GFileChooserScanDir(gfc,space);
	free(space);
    } else
	GFileChooserScanDir(gfc,gfc->paths[mi->mid]);
}

static GMenuItem gfcbookmarkmenu[] = {
    { { (unichar_t *) N_("Directory|Back"), &_GIcon_backarrow, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 0, 0, '\0' }, '\0', ksm_control, NULL, NULL, GFCBack, 0 },
    { { (unichar_t *) N_("Directory|Forward"), &_GIcon_forwardarrow, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 0, 0, '\0' }, '\0', ksm_control, NULL, NULL, GFCForward, 0 },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, 0, '\0' }, '\0', 0, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Bookmark Current Dir"), &_GIcon_bookmark, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 0, 0, '\0' }, '\0', ksm_control, NULL, NULL, GFCAddCur, 0 },
    { { (unichar_t *) N_("Remove Bookmark..."), &_GIcon_nobookmark, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 0, 0, '\0' }, '\0', ksm_control, NULL, NULL, GFCRemoveBook, 0 },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, 0, '\0' }, '\0', 0, NULL, NULL, NULL, 0 }, /* line */
    GMENUITEM_EMPTY
};
static int bgotten=false;

static int GFileChooserBookmarks(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonpress ) {
	GFileChooser *gfc = (GFileChooser *) GGadgetGetUserData(g);
	GMenuItem *mi;
	int i, bcnt, mcnt, pcnt;
	GEvent fake;
	GRect pos;

	if ( !bgotten ) {
	    bgotten = true;
	    for ( i=0; gfcbookmarkmenu[i].ti.text!=NULL || gfcbookmarkmenu[i].ti.line; ++i )
		if ( gfcbookmarkmenu[i].ti.text!=NULL )
		    gfcbookmarkmenu[i].ti.text = (unichar_t *) S_( (char *) gfcbookmarkmenu[i].ti.text);
	}
	for ( mcnt=0; gfcbookmarkmenu[mcnt].ti.text!=NULL || gfcbookmarkmenu[mcnt].ti.line; ++mcnt );
	bcnt = 0;
	if ( bookmarks!=NULL )
	    for ( ; bookmarks[bcnt]!=NULL; ++bcnt );
	if ( gfc->paths!=NULL ) {
	    for ( pcnt=0; gfc->paths[pcnt]!=NULL; ++pcnt );
	    if ( pcnt!=0 && bcnt!=0 ) ++pcnt;
	    bcnt += pcnt;
	}
	mi = calloc((mcnt+bcnt+1),sizeof(GMenuItem));
	for ( mcnt=0; gfcbookmarkmenu[mcnt].ti.text!=NULL || gfcbookmarkmenu[mcnt].ti.line; ++mcnt ) {
	    mi[mcnt] = gfcbookmarkmenu[mcnt];
	    mi[mcnt].ti.text = (unichar_t *) copy( (char *) mi[mcnt].ti.text);
	    mi[mcnt].ti.userdata = gfc;
	}
	if ( gfc->hpos==0 )
	    mi[0].ti.disabled = true;		/* can't go further back */
	if ( gfc->hpos+1>=gfc->hcnt )
	    mi[1].ti.disabled = true;		/* can't go further forward */
	if ( bookmarks==NULL )
	    mi[4].ti.disabled = true;		/* can't remove bookmarks, already none */
	else {
	    if ( gfc->paths!=NULL ) {
		for ( bcnt=0; gfc->paths[bcnt]!=NULL; ++bcnt ) {
		    mi[mcnt+bcnt].ti.text = u_copy(gfc->paths[bcnt]);
		    mi[mcnt+bcnt].ti.fg = mi[mcnt+bcnt].ti.bg = COLOR_DEFAULT;
		    mi[mcnt+bcnt].ti.userdata = gfc;
		    mi[mcnt+bcnt].mid = bcnt;
		    mi[mcnt+bcnt].invoke = GFCPath;
		}
		mcnt+=bcnt;
		if ( bookmarks!=NULL && bookmarks[0]!=NULL ) {
		    mi[mcnt].ti.line = true;
		    mi[mcnt].ti.fg = mi[mcnt].ti.bg = COLOR_DEFAULT;
		    ++mcnt;
		}
	    }
	    for ( bcnt=0; bookmarks[bcnt]!=NULL; ++bcnt ) {
		mi[mcnt+bcnt].ti.text = u_copy(bookmarks[bcnt]);
		mi[mcnt+bcnt].ti.fg = mi[mcnt+bcnt].ti.bg = COLOR_DEFAULT;
		mi[mcnt+bcnt].ti.userdata = gfc;
		mi[mcnt+bcnt].mid = bcnt;
		mi[mcnt+bcnt].invoke = GFCBookmark;
	    }
	}
	GGadgetGetSize(g,&pos);
	memset(&fake,0,sizeof(fake));
	fake.type = et_mousedown;
	fake.w = g->base;
	fake.u.mouse.x = pos.x;
	fake.u.mouse.y = pos.y+pos.height;
	GMenuCreatePopupMenu(gfc->g.base,&fake, mi);
	GMenuItemArrayFree(mi);
    }
return( true );
}

/* Routine to be called as the mouse moves across the dlg */
void GFileChooserPopupCheck(GGadget *g,GEvent *e) {
    GFileChooser *gfc = (GFileChooser *) g;
    int inside=false;

    if ( e->type == et_mousemove && (e->u.mouse.state&ksm_buttons)==0 ) {
	GGadgetEndPopup();
	for ( g=((GContainerD *) (gfc->g.base->widget_data))->gadgets; g!=NULL; g=g->prev ) {
	    if ( g!=(GGadget *) gfc && g!=(GGadget *) (gfc->filterb) &&
		     g!=(GGadget *) (gfc->files) &&
		     g->takes_input &&
		     e->u.mouse.x >= g->r.x &&
		     e->u.mouse.x<g->r.x+g->r.width &&
		     e->u.mouse.y >= g->r.y &&
		     e->u.mouse.y<g->r.y+g->r.height ) {
		inside = true;
	break;
	    }
	}
	if ( !inside )
	    GGadgetPreparePopup(gfc->g.base,gfc->wildcard);
    } else if ( e->type == et_mousedown && e->u.mouse.button==3 ) {
	GFCPopupMenu(g,e);
    }
}

/* Routine to be called by the filter button */
void GFileChooserFilterIt(GGadget *g) {
    GFileChooser *gfc = (GFileChooser *) g;
    unichar_t *pt, *spt, *slashpt, *dir, *temp;
    unichar_t *tofree = NULL;
    int wasdir;

    wasdir = gfc->lastname!=NULL;

    spt = (unichar_t *) _GGadgetGetTitle(&gfc->name->g);
#ifdef _WIN32
    spt = tofree = u_GFileNormalizePath(u_copy(spt));
#endif
    if ( *spt=='\0' ) {		/* Werner tells me that pressing the Filter button with nothing should show the default filter mask */
	if ( gfc->wildcard!=NULL )
	    GGadgetSetTitle(&gfc->name->g,gfc->wildcard);
return;
    }

    if (( slashpt = u_strrchr(spt,'/'))==NULL )
	slashpt = spt;
    else
	++slashpt;
    pt = slashpt;
    while ( *pt && *pt!='*' && *pt!='?' && *pt!='[' && *pt!='{' )
	++pt;
    if ( *pt!='\0' ) {
	free(gfc->wildcard);
	gfc->wildcard = u_copy(slashpt);
    } else if ( gfc->lastname==NULL )
	gfc->lastname = u_copy(slashpt);
    if( u_GFileIsAbsolute(spt) )
	dir = u_copyn(spt,slashpt-spt);
    else {
	unichar_t *curdir = GFileChooserGetCurDir(gfc,-1);
	if ( slashpt!=spt ) {
	    temp = u_copyn(spt,slashpt-spt);
	    dir = u_GFileAppendFile(curdir,temp,true);
	    free(temp);
	} else if ( wasdir && *pt=='\0' )
	    dir = u_GFileAppendFile(curdir,spt,true);
	else
	    dir = curdir;
	if ( dir!=curdir )
	    free(curdir);
    }
    GFileChooserScanDir(gfc,dir);
    free(dir);
    free(tofree);
}

/* A function that may be connected to a filter button as its handle_controlevent */
int GFileChooserFilterEh(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate )
	GFileChooserFilterIt(GGadgetGetUserData(g));
return( true );
}

void GFileChooserConnectButtons(GGadget *g,GGadget *ok, GGadget *filter) {
    GFileChooser *gfc = (GFileChooser *) g;
    gfc->ok = (GButton *) ok;
    gfc->filterb = (GButton *) filter;
}

void GFileChooserSetFilterText(GGadget *g,const unichar_t *wildcard) {
    GFileChooser *gfc = (GFileChooser *) g;
    free(gfc->wildcard);
    gfc->wildcard = u_copy(wildcard);
}

void GFileChooserSetFilterText8(GGadget *g,const char *wildcard) {
    GFileChooser *gfc = (GFileChooser *) g;
    free(gfc->wildcard);
    gfc->wildcard = utf82u_copy(wildcard);
}

unichar_t *GFileChooserGetFilterText(GGadget *g) {
    GFileChooser *gfc = (GFileChooser *) g;
return( gfc->wildcard );
}

char *GFileChooserGetFilterText8(GGadget *g) {
    GFileChooser *gfc = (GFileChooser *) g;
return( u2utf8_copy(gfc->wildcard) );
}


void GFileChooserSetFilterFunc(GGadget *g,GFileChooserFilterType filter) {
    GFileChooser *gfc = (GFileChooser *) g;
    if ( filter==NULL )
	filter = GFileChooserDefFilter;
    gfc->filter = filter;
}

GFileChooserFilterType GFileChooserGetFilterFunc(GGadget *g) {
    GFileChooser *gfc = (GFileChooser *) g;
return( gfc->filter );
}

/**
 * no change to the current filename by default
 *
 * if a change is desired, then currentFilename should point to the
 * new string and return 1 to allow the caller to free this new
 * string.
 */
int GFileChooserDefInputFilenameFunc( GGadget *g,
				      const unichar_t ** currentFilename,
				      unichar_t* oldfilename ) {
    return 0;
}

void GFileChooserSetInputFilenameFunc(GGadget *g,GFileChooserInputFilenameFuncType func) {
    GFileChooser *gfc = (GFileChooser *) g;
    if ( func==NULL )
	func = GFileChooserDefInputFilenameFunc;
    gfc->inputfilenamefunc = func;
}

GFileChooserInputFilenameFuncType GFileChooserGetInputFilenameFunc(GGadget *g) {
    GFileChooser *gfc = (GFileChooser *) g;
    if ( gfc->inputfilenamefunc==NULL )
	return GFileChooserDefInputFilenameFunc;
    return( gfc->inputfilenamefunc );
}


void GFileChooserSetFilename(GGadget *g,const unichar_t *defaultfile)
{
    GFileChooser *gfc = (GFileChooser *) g;

    GGadgetSetTitle(g,defaultfile);

    // if this is the first time we are here, we assume
    // the current filename is what it was before. Less NULL
    // checks in the callback function below.
    if(!gfc->inputfilenameprevchar)
	gfc->inputfilenameprevchar = u_copy(_GGadgetGetTitle(&gfc->name->g));

}


void GFileChooserSetMimetypes(GGadget *g,unichar_t **mimetypes) {
    GFileChooser *gfc = (GFileChooser *) g;
    int i;

    if ( gfc->mimetypes ) {
	for ( i=0; gfc->mimetypes[i]!=NULL; ++i )
	    free( gfc->mimetypes[i]);
	free(gfc->mimetypes);
    }
    if ( mimetypes ) {
	for ( i=0; mimetypes[i]!=NULL; ++i );
	gfc->mimetypes = malloc((i+1)*sizeof(unichar_t *));
	for ( i=0; mimetypes[i]!=NULL; ++i )
	    gfc->mimetypes[i] = u_copy(mimetypes[i]);
	gfc->mimetypes[i] = NULL;
    } else
	gfc->mimetypes = NULL;
}

unichar_t **GFileChooserGetMimetypes(GGadget *g) {
    GFileChooser *gfc = (GFileChooser *) g;
return( gfc->mimetypes );
}

/* Change the current file, or the current directory/file */
static void GFileChooserSetTitle(GGadget *g,const unichar_t *tit) {
    GFileChooser *gfc = (GFileChooser *) g;
    unichar_t *pt, *curdir, *temp, *dir, *base;

    if ( tit==NULL ) {
	curdir = GFileChooserGetCurDir(gfc,-1);
	GFileChooserScanDir(gfc,curdir);
	free(curdir);
return;
    }

    pt = u_strrchr(tit,'/');
    free(gfc->lastname);
    gfc->lastname = NULL;

    if ( u_GFileIsAbsolute(tit) ){
	base = uc_strstr(tit, "://");
	if(!base) base = (unichar_t*) tit;
	if(pt > base && pt[1] && (pt[1]!='.' || pt[2]!='\0')){
	    gfc->lastname = u_copy(pt+1);
	    dir = u_copyn(tit, pt-tit);
	}
	else{
	    dir = u_copy(tit);
	}
	GFileChooserScanDir(gfc,dir);
	free(dir);
    } else if ( pt==NULL ) {
	GGadgetSetTitle(&gfc->name->g,tit);
	curdir = GFileChooserGetCurDir(gfc,-1);
	GFileChooserScanDir(gfc,curdir);
	free(curdir);
    } else {
	curdir = GFileChooserGetCurDir(gfc,-1);
	temp = u_copyn(tit,pt-tit);
	dir = u_GFileAppendFile(curdir,temp,true);
	free(temp); free(curdir);
	free(gfc->lastname);
	if ( pt[1]!='\0' )
	    gfc->lastname = u_copy(pt+1);
	GFileChooserScanDir(gfc,dir);
	free(dir);
    }
}

void GFileChooserRefreshList(GGadget *g) {
    GFileChooser *gfc = (GFileChooser *) g;
    unichar_t *curdir;

    curdir = GFileChooserGetCurDir(gfc,-1);
    GFileChooserScanDir(gfc,curdir);
    free(curdir);
}

/* Get the current directory/file */
static unichar_t *GFileChooserGetTitle(GGadget *g) {
    GFileChooser *gfc = (GFileChooser *) g;
    unichar_t *spt, *curdir, *file;

    spt = u_GFileNormalizePath(u_copy((unichar_t *)_GGadgetGetTitle(&gfc->name->g)));
    if ( u_GFileIsAbsolute(spt) )
	file = spt;
    else {
	curdir = GFileChooserGetCurDir(gfc,-1);
	file = u_GFileAppendFile(curdir,spt,gfc->lastname!=NULL);
	free(curdir);
    }
    if (file != spt) {
        free(spt);
    }
return( file );
}

static void GFileChooser_destroy(GGadget *g) {
    GFileChooser *gfc = (GFileChooser *) g;
    int i;

    free(lastdir);
    lastdir = GFileChooserGetCurDir(gfc,-1);

    GGadgetDestroy(&gfc->topbox->g);	/* destroys everything */
    if ( gfc->paths!=NULL ) {
	for ( i=0; gfc->paths[i]!=NULL; ++i )
	    free(gfc->paths[i]);
	free(gfc->paths);
    }
    free(gfc->wildcard);
    free(gfc->lastname);
    if ( gfc->mimetypes ) {
	for ( i=0; gfc->mimetypes[i]!=NULL; ++i )
	    free( gfc->mimetypes[i]);
	free(gfc->mimetypes);
    }
    for ( i=0; i<gfc->hcnt; ++i )
	free(gfc->history[i]);
    free(gfc->history);
    _ggadget_destroy(&gfc->g);
}

static int gfilechooser_expose(GWindow pixmap, GGadget *g, GEvent *event) {
return( true );
}

static int gfilechooser_mouse(GGadget *g, GEvent *event) {
    GFileChooser *gfc = (GFileChooser *) g;

    if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button>=4 && event->u.mouse.button<=7) ) {
	if ( gfc->files->vsb!=NULL )
return( GGadgetDispatchEvent(&gfc->files->vsb->g,event));
	else
return( true );
    }

return( false );
}

static int gfilechooser_noop(GGadget *g, GEvent *event) {
return( false );
}

static int gfilechooser_timer(GGadget *g, GEvent *event) {
return( false );
}

static void gfilechooser_move(GGadget *g, int32_t x, int32_t y ) {
    GFileChooser *gfc = (GFileChooser *) g;

    GGadgetMove(&gfc->topbox->g,x,y);
    _ggadget_move(g,x,y);
}

static void gfilechooser_resize(GGadget *g, int32_t width, int32_t height ) {
    GFileChooser *gfc = (GFileChooser *) g;

    GGadgetResize(&gfc->topbox->g,width,height);
    _ggadget_resize(g,width,height);
}

static void gfilechooser_setvisible(GGadget *g, int visible ) {
    GFileChooser *gfc = (GFileChooser *) g;
    GGadgetSetVisible(&gfc->files->g,visible);
    GGadgetSetVisible(&gfc->directories->g,visible);
    GGadgetSetVisible(&gfc->name->g,visible);
    GGadgetSetVisible(&gfc->up->g,visible);
    GGadgetSetVisible(&gfc->home->g,visible);
    GGadgetSetVisible(&gfc->bookmarks->g,visible);
    GGadgetSetVisible(&gfc->config->g,visible);
    GGadgetSetVisible(&gfc->topbox->g,visible);
    _ggadget_setvisible(g,visible);
}

static void gfilechooser_setenabled(GGadget *g, int enabled ) {
    GFileChooser *gfc = (GFileChooser *) g;
    GGadgetSetEnabled(&gfc->files->g,enabled);
    GGadgetSetEnabled(&gfc->subdirs->g,enabled);
    GGadgetSetEnabled(&gfc->directories->g,enabled);
    GGadgetSetEnabled(&gfc->name->g,enabled);
    GGadgetSetEnabled(&gfc->up->g,enabled);
    GGadgetSetEnabled(&gfc->home->g,enabled);
    GGadgetSetEnabled(&gfc->bookmarks->g,enabled);
    GGadgetSetEnabled(&gfc->config->g,enabled);
    GGadgetSetEnabled(&gfc->topbox->g,enabled);
    _ggadget_setenabled(g,enabled);
}

static void GFileChooserGetDesiredSize(GGadget *g,GRect *outer,GRect *inner) {
    if ( inner!=NULL ) {
	int bp = GBoxBorderWidth(g->base,g->box);
	inner->x = inner->y = 0;
	inner->width = g->desired_width - 2*bp;
	inner->height = g->desired_height - 2*bp;
    }
    if ( outer!=NULL ) {
	outer->x = outer->y = 0;
	outer->width = g->desired_width;
	outer->height = g->desired_height;
    }
}

static int gfilechooser_FillsWindow(GGadget *g) {
return( g->prev==NULL &&
	(_GWidgetGetGadgets(g->base)==g ||
	 _GWidgetGetGadgets(g->base)==(GGadget *) ((GFileChooser *) g)->up ));
}

struct gfuncs GFileChooser_funcs = {
    0,
    sizeof(struct gfuncs),

    gfilechooser_expose,
    gfilechooser_mouse,
    gfilechooser_noop,
    NULL,
    NULL,
    gfilechooser_timer,
    NULL,

    _ggadget_redraw,
    gfilechooser_move,
    gfilechooser_resize,
    gfilechooser_setvisible,
    gfilechooser_setenabled,
    _ggadget_getsize,
    _ggadget_getinnersize,

    GFileChooser_destroy,

    GFileChooserSetTitle,
    NULL,
    GFileChooserGetTitle,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    GFileChooserGetDesiredSize,
    _ggadget_setDesiredSize,
    gfilechooser_FillsWindow,
    NULL
};

static void GFileChooserCreateChildren(GFileChooser *gfc, int flags) {
    GGadgetCreateData gcd[9], boxes[4], *varray[9], *harray[12], *harray2[4];
    GTextInfo label[9];
    int k=0, l=0, homek, upk, bookk, confk, dirsk, subdirsk, filesk, textk;

    memset(&gcd,'\0',sizeof(gcd));
    memset(&boxes,'\0',sizeof(boxes));
    memset(&label,'\0',sizeof(label));

    gcd[k].gd.flags = gg_visible|gg_enabled;
    gcd[k].gd.popup_msg = _("Home Folder");
    label[k].image = &_GIcon_homefolder;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = GFileChooserHome;
    homek = k;
    gcd[k++].creator = GButtonCreate;
    harray[l++] = &gcd[k-1]; harray[l++] = GCD_Glue;

    gcd[k].gd.flags = gg_visible|gg_enabled;
    gcd[k].gd.popup_msg = _("Bookmarks");
    label[k].image = &_GIcon_bookmark;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = GFileChooserBookmarks;
    bookk = k;
    gcd[k++].creator = GButtonCreate;
    harray[l++] = &gcd[k-1]; harray[l++] = GCD_Glue;

    gcd[k].gd.flags = gg_visible|gg_enabled|gg_list_exactlyone;
    gcd[k].gd.handle_controlevent = GFileChooserDListChanged;
    dirsk = k;
    gcd[k++].creator = GListButtonCreate;
    harray[l++] = &gcd[k-1]; harray[l++] = GCD_Glue;

    gcd[k].gd.flags = gg_visible|gg_enabled;
    gcd[k].gd.popup_msg = _("Parent Folder");
    label[k].image = &_GIcon_updir;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = GFileChooserUpDirButton;
    upk = k;
    gcd[k++].creator = GButtonCreate;
    harray[l++] = &gcd[k-1]; harray[l++] = GCD_Glue;

    gcd[k].gd.flags = gg_visible|gg_enabled;
    gcd[k].gd.popup_msg = _("Configure");
    label[k].image = &_GIcon_configtool;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = GFileChooserConfigure;
    confk = k;
    gcd[k++].creator = GButtonCreate;
    harray[l++] = &gcd[k-1]; harray[l++] = NULL;

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = harray;
    boxes[2].creator = GHBoxCreate;

    l=0;
    varray[l++] = &boxes[2];

    if ( dir_placement==dirs_separate )
	gcd[k].gd.flags = gg_visible|gg_enabled|gg_list_alphabetic|gg_list_exactlyone;
    else
	gcd[k].gd.flags =            gg_enabled|gg_list_alphabetic|gg_list_exactlyone;
    gcd[k].gd.handle_controlevent = GFileChooserFListSelected;
    subdirsk = k;
    gcd[k++].creator = GListCreate;
    harray2[0] = &gcd[k-1];

    if ( flags & gg_file_multiple )
	gcd[k].gd.flags = gg_visible|gg_enabled|gg_list_alphabetic|gg_list_multiplesel;
    else
	gcd[k].gd.flags = gg_visible|gg_enabled|gg_list_alphabetic|gg_list_exactlyone;
    gcd[k].gd.handle_controlevent = GFileChooserFListSelected;
    filesk = k;
    gcd[k++].creator = GListCreate;
    harray2[1] = &gcd[k-1]; harray2[2] = NULL;

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = harray2;
    boxes[3].creator = GHBoxCreate;
    varray[l++] = &boxes[3];

    gcd[k].gd.flags = gg_visible|gg_enabled;
    gcd[k].gd.handle_controlevent = GFileChooserTextChanged;
    textk = k;
    if ( flags&gg_file_pulldown )
	gcd[k++].creator = GListFieldCreate;
    else
	gcd[k++].creator = GTextCompletionCreate;
    varray[l++] = &gcd[k-1]; varray[l] = NULL;

    boxes[0].gd.pos.x = gfc->g.r.x;
    boxes[0].gd.pos.y = gfc->g.r.y;
    boxes[0].gd.pos.width  = gfc->g.r.width;
    boxes[0].gd.pos.height = gfc->g.r.height;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = varray;
    boxes[0].creator = GVBoxCreate;

    for ( l=0; l<k; ++l )
	gcd[l].data = gfc;

    GGadgetsCreate(gfc->g.base,boxes);

    gfc->topbox      = (GHVBox *)      boxes[0].ret;
    gfc->home        = (GButton *)     gcd[homek].ret;
    gfc->bookmarks   = (GButton *)     gcd[bookk].ret;
    gfc->directories = (GListButton *) gcd[dirsk].ret;
    gfc->up          = (GButton *)     gcd[upk  ].ret;
    gfc->config      = (GButton *)     gcd[confk].ret;
    gfc->subdirs     = (GList *)       gcd[subdirsk].ret;
    gfc->files       = (GList *)       gcd[filesk].ret;
    gfc->name        = (GTextField *)  gcd[textk].ret;

    gfc->home->g.contained = true;
    gfc->bookmarks->g.contained = true;
    gfc->directories->g.contained = true;
    gfc->up->g.contained = true;
    gfc->config->g.contained = true;
    gfc->subdirs->g.contained = true;
    gfc->files->g.contained = true;
    gfc->name->g.contained = true;
    gfc->topbox->g.contained = true;

    GCompletionFieldSetCompletion(&gfc->name->g,GFileChooserCompletion);
    GCompletionFieldSetCompletionMode(&gfc->name->g,true);

    GHVBoxSetExpandableRow(boxes[0].ret,1);
    GHVBoxSetExpandableCol(boxes[2].ret,4);
    if ( boxes[0].gd.pos.width!=0 && boxes[0].gd.pos.height!=0 )
	GGadgetResize(boxes[0].ret,boxes[0].gd.pos.width,boxes[0].gd.pos.height);
}

GGadget *GFileChooserCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    GFileChooser *gfc = (GFileChooser *) calloc(1,sizeof(GFileChooser));

    gfc->g.funcs = &GFileChooser_funcs;
    _GGadget_Create(&gfc->g,base,gd,data,&gfilechooser_box);
    gfc->g.takes_input = gfc->g.takes_keyboard = false; gfc->g.focusable = false;
    if ( gfc->g.r.width == 0 )
	gfc->g.r.width = GGadgetScale(GDrawPointsToPixels(base,140));
    if ( gfc->g.r.height == 0 )
	gfc->g.r.height = GDrawPointsToPixels(base,100);
    gfc->g.desired_width = gfc->g.r.width;
    gfc->g.desired_height = gfc->g.r.height;
    gfc->g.inner = gfc->g.r;
    _GGadget_FinalPosition(&gfc->g,base,gd);

    GFileChooserCreateChildren(gfc, gd->flags);
    gfc->filter = GFileChooserDefFilter;
    GFileChooserSetInputFilenameFunc( (GGadget*)gfc, 0 );
    if ( gd->flags & gg_group_end )
	_GGadgetCloseGroup(&gfc->g);

    if ( lastdir==NULL ) {
	lastdir = def2u_copy(GFileGetAbsoluteName("./"));
    }
    if ( gd->label==NULL || gd->label->text==NULL )
	GFileChooserSetTitle(&gfc->g,lastdir);
    else if ( u_GFileIsAbsolute(gd->label->text) )
	GFileChooserSetTitle(&gfc->g,gd->label->text);
    else {
	unichar_t *temp = u_GFileAppendFile(lastdir,gd->label->text,false);
	temp = u_GFileNormalize(temp);
	GFileChooserSetTitle(&gfc->g,temp);
	free(temp);
    }

return( &gfc->g );
}

void GFileChooserSetDir(GGadget *g,unichar_t *dir) {
    GFileChooserScanDir((GFileChooser *) g,dir);
}

unichar_t *GFileChooserGetDir(GGadget *g) {
return( GFileChooserGetCurDir((GFileChooser *) g,-1));
}

void GFileChooserGetChildren(GGadget *g,GGadget **pulldown, GGadget **list, GGadget **tf) {
    GFileChooser *gfc = (GFileChooser *)g;

    if ( pulldown!=NULL ) *pulldown = &gfc->directories->g;
    if ( tf!=NULL )	  *tf = &gfc->name->g;
    if ( list!=NULL )	  *list = &gfc->files->g;
}

int GFileChooserPosIsDir(GGadget *g, int pos) {
    GFileChooser *gfc = (GFileChooser *)g;
    GGadget *list = &gfc->files->g;
    GTextInfo *ti;

    ti = GGadgetGetListItem(list,pos);
    if ( ti==NULL )
return( false );

return( ti->checked );
}

unichar_t *GFileChooserFileNameOfPos(GGadget *g, int pos) {
    GFileChooser *gfc = (GFileChooser *)g;
    GGadget *list = &gfc->files->g;
    GTextInfo *ti;
    unichar_t *curdir, *file;

    ti = GGadgetGetListItem(list,pos);
    if ( ti==NULL )
return( NULL );

    curdir = GFileChooserGetCurDir(gfc,-1);
    file = u_GFileAppendFile(curdir,ti->text,false);
    free(curdir);
return( file );
}

void GFileChooserSetShowHidden(int sh) {
    showhidden = sh;
}

int GFileChooserGetShowHidden(void) {
return( showhidden );
}

void GFileChooserSetDirectoryPlacement(int dp) {
    dir_placement = dp;
}

int GFileChooserGetDirectoryPlacement(void) {
return( dir_placement );
}

void GFileChooserSetBookmarks(unichar_t **b) {
    if ( bookmarks!=NULL && bookmarks!=b ) {
	int i;

	for ( i=0; bookmarks[i]!=NULL; ++i )
	    free(bookmarks[i]);
	free(bookmarks);
    }
    bookmarks = b;
}

unichar_t **GFileChooserGetBookmarks(void) {
return( bookmarks );
}

void GFileChooserSetPrefsChangedCallback(void *data, void (*p_c)(void *)) {
    prefs_changed = p_c;
    prefs_changed_data = data;
}

void GFileChooserSetPaths(GGadget *g, const char* const* path) {
    unichar_t **dirs = NULL;
    int dcnt;
    GFileChooser *gfc = (GFileChooser *) g;

    if ( gfc->paths!=NULL ) {
	for ( dcnt=0; gfc->paths[dcnt]!=NULL; ++dcnt )
	    free( gfc->paths[dcnt] );
	free(gfc->paths);
	gfc->paths = NULL;
    }
    if ( path==NULL || path[0]==NULL )
return;

    for ( dcnt=0; path[dcnt]!=NULL; ++dcnt );
    gfc->paths = dirs = malloc((dcnt+1)*sizeof(unichar_t *));
    for ( dcnt=0; path[dcnt]!=NULL; ++dcnt )
	dirs[dcnt] = utf82u_copy(path[dcnt]);
    dirs[dcnt] = NULL;
}
