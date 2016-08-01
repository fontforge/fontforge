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
#include <stdlib.h>
#include "gdraw.h"
#include "ggadgetP.h"
#include "utype.h"
#include "ustring.h"
#include "gresource.h"
#include "gresourceP.h"
#include "hotkeys.h"
#include "gkeysym.h"

/////////////////////////////////////////////////////////////////
// The below keys are from this file, when/if we move to GTK+
// then perhaps we should include this file instead of the inline
// definitions
//#include <gdk/gdkkeysyms.h>
#define GDK_KEY_Tab 0xff09
#define GDK_KEY_ISO_Left_Tab 0xfe20
/////////////////////////////////////////////////////////////////

int GTextInfoGetWidth(GWindow base,GTextInfo *ti,FontInstance *font) {
    int width=0;
    int iwidth=0;
    int skip = 0;

    if ( ti->text!=NULL ) {
	if ( ti->font!=NULL )
	    font = ti->font;

	if ( font!=NULL )
	    GDrawSetFont(base,font);
	width = GDrawGetTextWidth(base,ti->text, -1);
    }
    if ( ti->image!=NULL ) {
	iwidth = GImageGetScaledWidth(base,ti->image);
	if ( ti->text!=NULL )
	    skip = GDrawPointsToPixels(base,6);
    }
return( width+iwidth+skip );
}

int GTextInfoGetMaxWidth(GWindow base,GTextInfo **ti,FontInstance *font) {
    int width=0, temp;
    int i;

    for ( i=0; ti[i]->text!=NULL || ti[i]->image!=NULL; ++i ) {
	if (( temp= GTextInfoGetWidth(base,ti[i],font))>width )
	    width = temp;
    }
return( width );
}

int GTextInfoGetHeight(GWindow base,GTextInfo *ti,FontInstance *font) {
    int fh=0, as=0, ds=0, ld;
    int iheight=0;
    int height;
    GTextBounds bounds;

    if ( ti->font!=NULL )
	font = ti->font;
    GDrawWindowFontMetrics(base,font,&as, &ds, &ld);
    if ( ti->text!=NULL ) {
	GDrawSetFont(base,font);
	GDrawGetTextBounds(base,ti->text, -1, &bounds);
	if ( as<bounds.as ) as = bounds.as;
	if ( ds<bounds.ds ) ds = bounds.ds;
    }
    fh = as+ds;
    if ( ti->image!=NULL ) {
	iheight = GImageGetScaledHeight(base,ti->image);
	iheight += 1;
    }
    if ( (height = fh)<iheight ) height = iheight;
return( height );
}

int GTextInfoGetMaxHeight(GWindow base,GTextInfo **ti,FontInstance *font,int *allsame) {
    int height=0, temp, same=1;
    int i;

    for ( i=0; ti[i]->text!=NULL || ti[i]->image!=NULL; ++i ) {
	temp= GTextInfoGetHeight(base,ti[i],font);
	if ( height!=0 && height!=temp )
	    same = 0;
	if ( height<temp )
	    height = temp;
    }
    *allsame = same;
return( height );
}

int GTextInfoGetAs(GWindow base,GTextInfo *ti, FontInstance *font) {
    int fh=0, as=0, ds=0, ld;
    int iheight=0;
    int height;
    GTextBounds bounds;

    GDrawWindowFontMetrics(base,font,&as, &ds, &ld);
    if ( ti->text!=NULL ) {
	GDrawSetFont(base,font);
	GDrawGetTextBounds(base,ti->text, -1, &bounds);
	if ( as<bounds.as ) as = bounds.as;
	if ( ds<bounds.ds ) ds = bounds.ds;
    }
    fh = as+ds;
    if ( ti->image!=NULL ) {
	iheight = GImageGetScaledHeight(base,ti->image);
    }
    if ( (height = fh)<iheight ) height = iheight;

    if ( ti->text!=NULL )
return( as+(height>fh?(height-fh)/2:0) );

return( iheight );
}

int GTextInfoDraw(GWindow base,int x,int y,GTextInfo *ti,
	FontInstance *font,Color fg, Color sel, int ymax) {
    int fh=0, as=0, ds=0, ld;
    int iwidth=0, iheight=0;
    int height, skip = 0;
    GTextBounds bounds;
    GRect r, old;

    GDrawWindowFontMetrics(base,font,&as, &ds, &ld);
    if ( ti->text!=NULL ) {
	if ( ti->font!=NULL )
	    font = ti->font;
	if ( ti->fg!=COLOR_DEFAULT && ti->fg!=COLOR_UNKNOWN )
	    fg = ti->fg;

	GDrawSetFont(base,font);
	GDrawGetTextBounds(base,ti->text, -1, &bounds);
	if ( as<bounds.as ) as = bounds.as;
	if ( ds<bounds.ds ) ds = bounds.ds;
    }
    fh = as+ds;
    if ( fg == COLOR_DEFAULT )
	fg = GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(base));
    if ( ti->image!=NULL ) {
	iwidth = GImageGetScaledWidth(base,ti->image);
	iheight = GImageGetScaledHeight(base,ti->image)+1;
	if ( ti->text!=NULL )
	    skip = GDrawPointsToPixels(base,6);
    }
    if ( (height = fh)<iheight ) height = iheight;

    r.y = y; r.height = height;
    r.x = 0; r.width = 10000;

    if ( ti->line ) {
	_GGroup_Init();
	GDrawGetClip(base,&r);
	r.x += GDrawPointsToPixels(base,2); r.width -= 2*GDrawPointsToPixels(base,2);
	GDrawPushClip(base,&r,&old);
	r.y = y; r.height = height;
	r.x = x; r.width = 10000;
	GBoxDrawHLine(base,&r,&_GGroup_LineBox);
	GDrawPopClip(base,&old);
    } else {
	if (( ti->selected && sel!=COLOR_DEFAULT ) || ( ti->bg!=COLOR_DEFAULT && ti->bg!=COLOR_UNKNOWN )) {
	    Color bg = ti->bg;
	    if ( ti->selected ) {
		if ( sel==COLOR_DEFAULT )
		    sel = fg;
		bg = sel;
		if ( sel==fg ) {
		    fg = ti->bg;
		    if ( fg==COLOR_DEFAULT || fg==COLOR_UNKNOWN )
			fg = GDrawGetDefaultBackground(GDrawGetDisplayOfWindow(base));
		}
	    }
	    GDrawFillRect(base,&r,bg);
	}

	if ( ti->image!=NULL && ti->image_precedes ) {
	    GDrawDrawScaledImage(base,ti->image,x,y + (iheight>as?0:as-iheight));
	    x += iwidth + skip;
	}
	if ( ti->text!=NULL ) {
	    int ypos = y+as+(height>fh?(height-fh)/2:0);
	    int width = GDrawDrawText(base,x,ypos,ti->text,-1,fg);
	    _ggadget_underlineMnemonic(base,x,ypos,ti->text,ti->mnemonic,fg,ymax);
	    x += width + skip;
	}
	if ( ti->image!=NULL && !ti->image_precedes )
	    GDrawDrawScaledImage(base,ti->image,x,y + (iheight>as?0:as-iheight));
    }

return( height );
}

unichar_t *utf82u_mncopy(const char *utf8buf,unichar_t *mn) {
    int len = strlen(utf8buf);
    unichar_t *ubuf = malloc((len+1)*sizeof(unichar_t));
    unichar_t *upt=ubuf, *uend=ubuf+len;
    const uint8 *pt = (const uint8 *) utf8buf, *end = pt+strlen(utf8buf);
    int w;
    int was_mn = false;

    *mn = '\0';
    while ( pt<end && *pt!='\0' && upt<uend ) {
	if ( *pt<=127 ) {
	    if ( *pt!='_' )
		*upt = *pt++;
	    else {
		was_mn = 2;
		++pt;
		--upt;
	    }
	} else if ( *pt<=0xdf ) {
	    *upt = ((*pt&0x1f)<<6) | (pt[1]&0x3f);
	    pt += 2;
	} else if ( *pt<=0xef ) {
	    *upt = ((*pt&0xf)<<12) | ((pt[1]&0x3f)<<6) | (pt[2]&0x3f);
	    pt += 3;
	} else if ( upt+1<uend ) {
	    /* Um... I don't support surrogates */
	    w = ( ((*pt&0x7)<<2) | ((pt[1]&0x30)>>4) )-1;
	    *upt++ = 0xd800 | (w<<6) | ((pt[1]&0xf)<<2) | ((pt[2]&0x30)>>4);
	    *upt   = 0xdc00 | ((pt[2]&0xf)<<6) | (pt[3]&0x3f);
	    pt += 4;
	} else {
	    /* no space for surrogate */
	    pt += 4;
	}
	++upt;
	if ( was_mn==1 ) {
	    *mn = upt[-1];
	    if ( islower(*mn) ) *mn = toupper(*mn);
	}
	--was_mn;
    }
    *upt = '\0';
return( ubuf );
}

GTextInfo *GTextInfoCopy(GTextInfo *ti) {
    GTextInfo *copy;

    copy = malloc(sizeof(GTextInfo));
    *copy = *ti;
    copy->text_is_1byte = false;
    if ( copy->fg == 0 && copy->bg == 0 ) {
	copy->fg = copy->bg = COLOR_UNKNOWN;
    }
    if ( ti->text!=NULL ) {
	if ( ti->text_is_1byte && ti->text_in_resource ) {
	    copy->text = utf82u_mncopy((char *) copy->text,&copy->mnemonic);
	    copy->text_in_resource = false;
	    copy->text_is_1byte = false;
	} else if ( ti->text_in_resource ) {
	    copy->text = u_copy((unichar_t *) GStringGetResource((intpt) copy->text,&copy->mnemonic));
	    copy->text_in_resource = false;
	} else if ( ti->text_is_1byte ) {
	    copy->text = utf82u_copy((char *) copy->text);
	    copy->text_is_1byte = false;
	} else
	    copy->text = u_copy(copy->text);
    }
return( copy);
}

static const char *imagedir_default = "fontforge-pixmaps";
static char *imagedir = NULL;	/* This is the system pixmap directory */
static char **imagepath = NULL;			/* May contain user directories too */
static size_t imagepathlenmax = 0;

struct image_bucket {
    struct image_bucket *next;
    char *filename, *absname;
    GImage *image;
};

#define IC_SIZE	127
static struct image_bucket *imagecache[IC_SIZE];

void InitImageCache() {
  memset(imagecache, 0, IC_SIZE * sizeof(struct image_bucket *));
}

void ClearImageCache() {
      for ( int i=0; i<IC_SIZE; ++i ) {
        struct image_bucket * bucket;
        struct image_bucket * nextbucket;
	for ( bucket = imagecache[i]; bucket!=NULL; bucket=nextbucket ) {
          nextbucket = bucket->next;
          if (bucket->filename != NULL) { free(bucket->filename); bucket->filename = NULL; }
          if (bucket->absname != NULL) { free(bucket->absname); bucket->absname = NULL; }
          if (bucket->image != NULL) { GImageDestroy(bucket->image); bucket->image = NULL; }
          free(bucket);
        }
        imagecache[i] = NULL;
     }
}

static int hash_filename(const char *_pt ) {
    const unsigned char *pt = (const unsigned char *) _pt;
    int val = 0;

    while ( *pt ) {
	val <<= 1;
	if ( val & 0x8000 ) {
	    val &= ~0x8000;
	    val ^= 1;
	}
	val ^= *pt++;
    }
return( val%IC_SIZE );
}

static void ImagePathDefault(void) {
    if ( imagepath==NULL ) {
	imagepath = malloc(2*sizeof(void *));
	imagepath[0] = (imagedir == NULL) ? copy(imagedir_default) : copy(imagedir);
	imagepath[1] = NULL;
	imagepathlenmax = strlen(imagepath[0]);
	if (_GGadget_ImagePath != NULL) free(_GGadget_ImagePath);
	_GGadget_ImagePath = copy("=");
    }
}

/**
 * \return The image path. The return value should not be freed or modified.
 */
const char* const* _GGadget_GetImagePath(void) {
    ImagePathDefault();
    return (const char* const*) imagepath;
}

int _GGadget_ImageInCache(GImage *image) {
    int i;
    struct image_bucket *bucket;

    for ( i=0; i<IC_SIZE; ++i ) {
	for ( bucket=imagecache[i]; bucket!=NULL; bucket=bucket->next )
	    if ( bucket->image==image )
return( true );
    }
return( false );
}

static void ImageCacheReload(void) {
    int i,k;
    struct image_bucket *bucket;
    char *path=NULL;
    size_t pathlen;
    GImage *temp, hold;

    ImagePathDefault();

    /* Try to reload the cache from the new directory */
    /* If a file doesn't exist in the new dir when it did in the old then */
    /*  retain the old copy (people may hold pointers to it) */
    pathlen = imagepathlenmax+270; path = malloc(pathlen);
    for ( i=0; i<IC_SIZE; ++i ) {
	for ( bucket = imagecache[i]; bucket!=NULL; bucket=bucket->next ) {
	    if ( strlen(bucket->filename)+imagepathlenmax+3 > pathlen ) {
		pathlen = strlen(bucket->filename)+imagepathlenmax+20;
		path = realloc(path,pathlen);
	    }
	    for ( k=0; imagepath[k]!=NULL; ++k ) {
		sprintf( path,"%s/%s", imagepath[k], bucket->filename );
		temp = GImageRead(path);
		if ( temp!=NULL )
	    break;
	    }
	    if ( temp!=NULL ) {
		if ( bucket->image==NULL )
		    bucket->image = temp;
		else {
		    /* Need to retain the GImage point, but update its */
		    /*  contents, and free the old stuff */
		    hold = *(bucket->image);
		    *bucket->image = *temp;
		    *temp = hold;
		    GImageDestroy(temp);
		}
		free( bucket->absname );
		bucket->absname = copy( path );
	    }
	}
    }
    free(path);
}

void GGadgetSetImageDir(char *dir) {
    int k;
    char *ptr = imagedir;
    //Check if imagedir has been initialised
    if (ptr == NULL) {
        //We shall check later if ptr should be freed or not
        ptr = (char*) imagedir_default;
    }
    
    if (dir != NULL && strcmp(ptr,dir) != 0) {
        imagedir = copy(dir);
        if (imagepath != NULL) {
            for (k=0; imagepath[k] != NULL; ++k) {
                if (strcmp(imagepath[k],ptr) == 0) {
                    break;
                }
            }

            if (ptr != imagedir_default) {
                free(ptr);
            }
            if (imagepath[k] != NULL) {
                free(imagepath[k]);
                imagepath[k] = copy(imagedir);
                ImageCacheReload();
            }
            if (_GGadget_ImagePath != NULL) free(_GGadget_ImagePath);
            _GGadget_ImagePath = copy("=");
        }
    }
}

static char *ImagePathFigureElement(char *start, int len) {
    if ( *start=='=' && len==1 ) {
        if (imagedir == NULL) {
            return copy(imagedir_default);
        }
        return copy(imagedir);
    }
    else if ( *start=='~' && start[1]=='/' && len>=2 && getenv("HOME")!=NULL ) {
	int hlen = strlen(getenv("HOME"));
	char *absname = malloc( hlen+len+8 );
	strcpy(absname,getenv("HOME"));
	strncpy(absname+hlen,start+1,len-1);
	absname[hlen+len-1] = '\0';
return( absname );
    } else
return( copyn(start,len));
}

#ifndef PATH_SEPARATOR
# define PATH_SEPARATOR ':'
#endif

void GGadgetSetImagePath(char *path) {
    int cnt, k;
    char *pt, *end;
    extern char *_GGadget_ImagePath;

    if ( path==NULL )
return;
    if (_GGadget_ImagePath != NULL) free( _GGadget_ImagePath );

    if ( imagepath!=NULL ) {
	for ( k=0; imagepath[k]!=NULL; ++k )
	    free( imagepath[k] );
	free( imagepath );
    }
    for ( cnt=0, pt = path; (end = strchr(pt,PATH_SEPARATOR))!=NULL; ++cnt, pt = end+1 );
    imagepath = malloc((cnt+2)*sizeof(char *));
    for ( cnt=0, pt = path; (end = strchr(pt,PATH_SEPARATOR))!=NULL; ++cnt, pt = end+1 )
	imagepath[cnt] = ImagePathFigureElement(pt,end-pt);
    imagepath[cnt] = ImagePathFigureElement(pt,strlen(pt));
    imagepath[cnt+1] = NULL;
    imagepathlenmax = 0;
    for ( cnt=0; imagepath[cnt]!=NULL; ++cnt )
	if ( strlen(imagepath[cnt]) > imagepathlenmax )
	    imagepathlenmax = strlen(imagepath[cnt]);
    ImageCacheReload();
    _GGadget_ImagePath = copy(path);
}

static GImage *_GGadgetImageCache(const char *filename, char **foundname) {
    int index = hash_filename(filename);
    struct image_bucket *bucket;
    char *path;
    int k;

    for ( bucket = imagecache[index]; bucket!=NULL; bucket = bucket->next ) {
	if ( strcmp(bucket->filename,filename)==0 ) {
	    if ( foundname!=NULL ) *foundname = copy( bucket->absname );
return( bucket->image );
	}
    }
    bucket = calloc(1,sizeof(struct image_bucket));
    bucket->next = imagecache[index];
    imagecache[index] = bucket;
    bucket->filename = copy(filename);

    ImagePathDefault();

    path = malloc(strlen(filename)+imagepathlenmax+10 );
    for ( k=0; imagepath[k]!=NULL; ++k ) {
	sprintf( path,"%s/%s", imagepath[k], filename );
	bucket->image = GImageRead(path);
	if ( bucket->image!=NULL ) {
	    bucket->absname = copy(path);
    break;
	}
    }
    free(path);
    if ( bucket->image!=NULL ) {
	/* Play with the clut to make white be transparent */
	struct _GImage *base = bucket->image->u.image;
	if ( base->image_type==it_mono && base->clut==NULL )
	    base->trans = 1;
	else if ( base->image_type!=it_true && base->clut!=NULL && base->trans==0xffffffff ) {
	    int i;
	    for ( i=0 ; i<base->clut->clut_len; ++i ) {
		if ( base->clut->clut[i]==0xffffff ) {
		    base->trans = i;
	    break;
		}
	    }
	}
    }
    if ( foundname!=NULL && bucket->image!=NULL )
	*foundname = copy( bucket->absname );
return(bucket->image);
}

GImage *GGadgetImageCache(const char *filename) {
return( _GGadgetImageCache(filename,NULL));
}

/* Substitutes an image contents with what's found in cache. */
/* That is, unless there is nothing found in the cache.      */
int TryGGadgetImageCache(GImage *image, const char *name) {
    GImage *loaded = GGadgetImageCache(name);
    if (loaded != NULL) *image = *loaded;
return (loaded != NULL);
}

GResImage *GGadgetResourceFindImage(char *name, GImage *def) {
    GImage *ret;
    char *fname;
    GResImage *ri;

    fname = GResourceFindString(name);
    if ( fname==NULL && def==NULL )
return( NULL );
    ri = calloc(1,sizeof(GResImage));
    ri->filename = fname;
    ri->image = def;
    if ( fname==NULL )
return( ri );

    if ( *fname=='/' )
	ret = GImageRead(fname);
    else if ( *fname=='~' && fname[1]=='/' && getenv("HOME")!=NULL ) {
	char *absname = malloc( strlen(getenv("HOME"))+strlen(fname)+8 );
	strcpy(absname,getenv("HOME"));
	strcat(absname,fname+1);
	ret = GImageRead(absname);
	free(fname);
	ri->filename = fname = absname;
    } else {
	char *absname;
	ret = _GGadgetImageCache(fname,&absname);
	if ( ret ) {
	    free(fname);
	    ri->filename = fname = absname;
	}
    }
    if ( ret==NULL ) {
	ri->filename = NULL;
	free(fname);
    } else
	ri->image = ret;

return( ri );
}
    
static void GTextInfoImageLookup(GTextInfo *ti) {
    char *pt;
    int any;

    if ( ti->image==NULL )
return;

    /* Image might be an image pointer, or it might be a filename we want to */
    /*  read and convert into an image. If it's an image it will begin with */
    /*  a short containing a small number (usually 1), which won't look like */
    /*  a filename */
    any = 0;
    for ( pt = (char *) (ti->image); *pt!='\0'; ++pt ) {
	if ( *pt<' ' || *pt>=0x7f )
return;
	if ( *pt=='.' )
	    any = 1;
    }
    if ( !any )		/* Must have an extension */
return;

    ti->image = GGadgetImageCache((char *) (ti->image));
}

GTextInfo **GTextInfoArrayFromList(GTextInfo *ti, uint16 *cnt) {
    int i;
    GTextInfo **arr;

    i = 0;
    if ( ti!=NULL )
	for ( ; ti[i].text!=NULL || ti[i].image!=NULL || ti[i].line; ++i );
    if ( i==0 ) {
	arr = malloc(sizeof(GTextInfo *));
	i =0;
    } else {
	arr = malloc((i+1)*sizeof(GTextInfo *));
	for ( i=0; ti[i].text!=NULL || ti[i].image!=NULL || ti[i].line; ++i )
	    arr[i] = GTextInfoCopy(&ti[i]);
    }
    arr[i] = calloc(1,sizeof(GTextInfo));
    if ( cnt!=NULL ) *cnt = i;
return( arr );
}

GTextInfo **GTextInfoArrayCopy(GTextInfo **ti) {
    int i;
    GTextInfo **arr;

    if ( ti==NULL || (ti[0]->image==NULL && ti[0]->text==NULL && !ti[0]->line) ) {
	arr = malloc(sizeof(GTextInfo *));
	i =0;
    } else {
	for ( i=0; ti[i]->text!=NULL || ti[i]->image!=NULL || ti[i]->line; ++i );
	arr = malloc((i+1)*sizeof(GTextInfo *));
	for ( i=0; ti[i]->text!=NULL || ti[i]->image!=NULL || ti[i]->line; ++i )
	    arr[i] = GTextInfoCopy(ti[i]);
    }
    arr[i] = calloc(1,sizeof(GTextInfo));
return( arr );
}

int GTextInfoArrayCount(GTextInfo **ti) {
    int i;

    for ( i=0; ti[i]->text || ti[i]->image || ti[i]->line; ++i );
return( i );
}

void GTextInfoFree(GTextInfo *ti) {
    if ( !ti->text_in_resource )
	free(ti->text);
    free(ti);
}

void GTextInfoListFree(GTextInfo *ti) {
    int i;

    if ( ti==NULL )
return;

    for ( i=0; ti[i].text!=NULL || ti[i].image!=NULL || ti[i].line; ++i )
	if ( !ti[i].text_in_resource )
	    free(ti[i].text);
    free(ti);
}

/* The list is terminated with an empty entry. Not a NULL pointer, but 
 * rather an empty entry terminates lists of GTextInfo entries.  (!)
 */
void GTextInfoArrayFree(GTextInfo **ti) {
    int i;

    if ( ti == NULL )
return;
    for ( i=0; ti[i]->text || ti[i]->image || ti[i]->line; ++i )
	GTextInfoFree(ti[i]);
    GTextInfoFree(ti[i]);	/* And free the null entry at end */
    free(ti);
}

int GTextInfoCompare(GTextInfo *ti1, GTextInfo *ti2) {
    GTextInfo2 *_ti1 = (GTextInfo2 *) ti1, *_ti2 = (GTextInfo2 *) ti2;

    if ( _ti1->sort_me_first_in_list != _ti2->sort_me_first_in_list ) {
	if ( _ti1->sort_me_first_in_list )
return( -1 );
	else
return( 1 );
    }

    if ( ti1->text == NULL && ti2->text==NULL )
return( 0 );
    else if ( ti1->text==NULL )
return( -1 );
    else if ( ti2->text==NULL )
return( 1 );
    else {
	char *t1, *t2;
	int ret;
	t1 = u2utf8_copy(ti1->text);
	t2 = u2utf8_copy(ti2->text);
	ret = strcoll(t1,t2);
	free(t1); free(t2);
return( ret );
    }
}

GTextInfo **GTextInfoFromChars(char **array, int len) {
    int i;
    GTextInfo **ti;

    if ( array==NULL || len==0 )
return( NULL );
    if ( len==-1 ) {
	for ( len=0; array[len]!=NULL; ++len );
    } else {
	for ( i=0; i<len && array[i]!=NULL; ++i );
	len = i;
    }
    ti = malloc((len+1)*sizeof(GTextInfo *));
    for ( i=0; i<len; ++i ) {
	ti[i] = calloc(1,sizeof(GTextInfo));
	ti[i]->text = uc_copy(array[i]);
	ti[i]->fg = ti[i]->bg = COLOR_DEFAULT;
    }
    ti[i] = calloc(1,sizeof(GTextInfo));
return( ti );
}

void GMenuItemArrayFree(GMenuItem *mi) {
    int i;

    if ( mi == NULL )
return;
    for ( i=0; mi[i].ti.text || mi[i].ti.image || mi[i].ti.line; ++i ) {
	GMenuItemArrayFree(mi[i].sub);
	free(mi[i].ti.text);
    }
    free(mi);
}

GMenuItem *GMenuItemArrayCopy(GMenuItem *mi, uint16 *cnt) {
    int i;
    GMenuItem *arr;

    if ( mi==NULL )
return( NULL );
    for ( i=0; mi[i].ti.text!=NULL || mi[i].ti.image!=NULL || mi[i].ti.line; ++i );
    if ( i==0 )
return( NULL );
    arr = malloc((i+1)*sizeof(GMenuItem));
    for ( i=0; mi[i].ti.text!=NULL || mi[i].ti.image!=NULL || mi[i].ti.line; ++i ) {
	arr[i] = mi[i];
	GTextInfoImageLookup(&arr[i].ti);
	if ( mi[i].ti.text!=NULL ) {
	    if ( mi[i].ti.text_in_resource && mi[i].ti.text_is_1byte )
		arr[i].ti.text = utf82u_mncopy((char *) mi[i].ti.text,&arr[i].ti.mnemonic);
	    else if ( mi[i].ti.text_in_resource )
		arr[i].ti.text = u_copy((unichar_t *) GStringGetResource((intpt) mi[i].ti.text,&arr[i].ti.mnemonic));
	    else if ( mi[i].ti.text_is_1byte )
		arr[i].ti.text = utf82u_copy((char *) mi[i].ti.text);
	    else
		arr[i].ti.text = u_copy(mi[i].ti.text);
	    arr[i].ti.text_in_resource = arr[i].ti.text_is_1byte = false;
	}
	if ( islower(arr[i].ti.mnemonic))
	    arr[i].ti.mnemonic = toupper(arr[i].ti.mnemonic);
	if ( islower(arr[i].shortcut))
	    arr[i].shortcut = toupper(arr[i].shortcut);
	if ( mi[i].sub!=NULL )
	    arr[i].sub = GMenuItemArrayCopy(mi[i].sub,NULL);
    }
    memset(&arr[i],'\0',sizeof(GMenuItem));
    if ( cnt!=NULL ) *cnt = i;
return( arr );
}

int GMenuItemArrayMask(GMenuItem *mi) {
    int mask = 0;
    int i;

    for ( i=0; mi[i].ti.text!=NULL || mi[i].ti.image!=NULL || mi[i].ti.line; ++i ) {
	if ( mi[i].sub!=NULL )
	    mask |= GMenuItemArrayMask(mi[i].sub);
	else
	    mask |= mi[i].short_mask;
    }
return( mask );
}

int GMenuItemArrayAnyUnmasked(GMenuItem *mi) {
    int i;

    for ( i=0; mi[i].ti.text!=NULL || mi[i].ti.image!=NULL || mi[i].ti.line; ++i ) {
	if ( mi[i].sub!=NULL ) {
	    if ( GMenuItemArrayAnyUnmasked(mi[i].sub) )
return( true );
	} else {
	    if ( (mi[i].short_mask&~ksm_shift)==0 && mi[i].shortcut!=0 )
return( true );
	}
    }
return( false );
}

void GMenuItem2ArrayFree(GMenuItem2 *mi) {
    int i;

    if ( mi == NULL )
return;
    for ( i=0; mi[i].ti.text || mi[i].ti.image || mi[i].ti.line; ++i ) {
	GMenuItem2ArrayFree(mi[i].sub);
	free(mi[i].ti.text);
    }
    free(mi);
}

static char *shortcut_domain = "shortcuts";

void GMenuSetShortcutDomain(char *domain) {
    shortcut_domain = domain;
}

const char *GMenuGetShortcutDomain(void) {
return(shortcut_domain);
}

static struct { char *modifier; int mask; char *alt; } modifiers[] = {
    { "Ctl+", ksm_control, NULL },
    { "Ctrl+", ksm_control, NULL },
    { "Control+", ksm_control, NULL },
    { "Shft+", ksm_shift, NULL },
    { "Shift+", ksm_shift, NULL },
    { "CapsLk+", ksm_capslock, NULL },
    { "CapsLock+", ksm_capslock, NULL },
    { "Meta+", ksm_meta, NULL },
    { "Alt+", ksm_meta, NULL },
    { "Esc+", ksm_meta, NULL },
    { "Flag0x01+", 0x01, NULL },
    { "Flag0x02+", 0x02, NULL },
    { "Flag0x04+", 0x04, NULL },
    { "Flag0x08+", 0x08, NULL },
    { "Flag0x10+", 0x10, NULL },
    { "Flag0x20+", 0x20, NULL },
    { "Flag0x40+", 0x40, NULL },
    { "Flag0x80+", 0x80, NULL },
    { "Opt+", ksm_meta, NULL },
    { "Option+", ksm_meta, NULL },
    /* We used to map command to control on the mac, no longer, let it be itself */
    { "Command+", ksm_cmdmacosx, NULL },
    { "Cmd+", ksm_cmdmacosx, NULL },
    { "NumLk+", ksm_cmdmacosx, NULL },    /* This is unfortunate. Numlock should be ignored, Command should not */
    { "NumLock+", ksm_cmdmacosx, NULL },
    { "numlock+", ksm_cmdmacosx, NULL },
    { "numberlock+", ksm_cmdmacosx, NULL },
    { NULL, 0, NULL }
    /* Windows flag key=Super (keysym ffeb/ffec) key maps to 0x40 on my machine */
};

static void initmods(void) {
    if ( modifiers[0].alt==NULL ) {
	int i;
	for ( i=0; modifiers[i].modifier!=NULL; ++i )
	    modifiers[i].alt = dgettext(shortcut_domain,modifiers[i].modifier);
    }
}

int GMenuItemParseMask(char *shortcut) {
    char *pt, *sh;
    int mask, temp, i;

    sh = dgettext(shortcut_domain,shortcut);
    if ( sh==shortcut && strlen(shortcut)>2 && shortcut[2]=='*' ) {
	sh = dgettext(shortcut_domain,shortcut+3);
	if ( sh==shortcut+3 )
	    sh = shortcut;
    }
    pt = strchr(sh,'|');
    if ( pt!=NULL )
	sh = pt+1;
    if ( *sh=='\0' || strcmp(sh,"No Shortcut")==0 || strcmp(sh,"None")==0 )
return(0);

    initmods();

    mask = 0;
    for (;;) {
	pt = strchr(sh,'+');
	if ( pt==sh || *sh=='\0' )
return( mask );
	if ( pt==NULL )
	    pt = sh+strlen(sh);
	for ( i=0; modifiers[i].modifier!=NULL; ++i ) {
	    if ( strncasecmp(sh,modifiers[i].modifier,pt-sh)==0 )
	break;
	}
	if ( modifiers[i].modifier==NULL ) {
	    for ( i=0; modifiers[i].alt!=NULL; ++i ) {
		if ( strncasecmp(sh,modifiers[i].alt,pt-sh)==0 )
	    break;
	    }
	}
	if ( modifiers[i].modifier!=NULL )
	    mask |= modifiers[i].mask;
	else if ( sscanf( sh, "0x%x", &temp)==1 )
	    mask |= temp;
	else {
	    fprintf( stderr, "Could not parse short cut: %s\n", shortcut );
return(0);
	}
	sh = pt+1;
    }
}

void HotkeyParse( Hotkey* hk, const char *shortcut ) {
    char *pt;
    const char *sh;
    int mask, temp, i;

    hk->state  = 0;
    hk->keysym = 0;
    strncpy( hk->text, shortcut, HOTKEY_TEXT_MAX_SIZE );

    sh = dgettext(shortcut_domain,shortcut);
    /* shortcut might be "Open|Ctl+O" meaning the Open menu item is bound to ^O */
    /*  or "CV*Open|Ctl+O" meaning that in the charview the Open menu item ...*/
    /*  but if CV*Open|Ctl+O isn't found then check simple "Open|Ctl+O" as a default */
    if ( sh==shortcut && strlen(shortcut)>2 && shortcut[2]=='*' ) {
	sh = dgettext(shortcut_domain,shortcut+3);
	if ( sh==shortcut+3 )
	    sh = shortcut;
    }
    pt = strchr(sh,'|');
    if ( pt!=NULL )
	sh = pt+1;
    if ( *sh=='\0' || strcmp(sh,"No Shortcut")==0 || strcmp(sh,"None")==0 )
	return;

    initmods();

    mask = 0;
    while ( (pt=strchr(sh,'+'))!=NULL && pt!=sh ) {	/* A '+' can also occur as the short cut char itself */
	for ( i=0; modifiers[i].modifier!=NULL; ++i ) {
	    if ( strncasecmp(sh,modifiers[i].modifier,pt-sh)==0 )
	break;
	}
	if ( modifiers[i].modifier==NULL ) {
	    for ( i=0; modifiers[i].alt!=NULL; ++i ) {
		if ( strncasecmp(sh,modifiers[i].alt,pt-sh)==0 )
	    break;
	    }
	}
	if ( modifiers[i].modifier!=NULL )
	    mask |= modifiers[i].mask;
	else if ( sscanf( sh, "0x%x", &temp)==1 )
	    mask |= temp;
	else {
	    fprintf( stderr, "Could not parse short cut: %s\n", shortcut );
	    return;
	}
	sh = pt+1;
    }
    hk->state = mask;
    for ( i=0; i<0x100; ++i ) {
	if ( GDrawKeysyms[i]!=NULL && uc_strcmp(GDrawKeysyms[i],sh)==0 ) {
	    hk->keysym = 0xff00 + i;
	    break;
	}
    }
    if ( i==0x100 ) {
	hk->keysym = utf8_ildb((const char **) &sh);
	if ( *sh!='\0' ) {
	    fprintf( stderr, "Unexpected characters at end of short cut: %s\n", shortcut );
	    return;
	}
    }
    //
    // The user really means lower case keys unless they have
    // given the "shift" modifier too. Like: Ctl+Shft+L
    //
//    fprintf(stderr,"HotkeyParse(1) spec:%d hk->keysym:%d shortcut:%s\n", GK_Special, hk->keysym, shortcut );
    if( hk->keysym < GK_Special ) {
	hk->keysym = tolower(hk->keysym);
	if( hk->state & ksm_shift ) {
	    hk->keysym = toupper(hk->keysym);
	}
    }
    if( hk->keysym == GDK_KEY_Tab ) {
#ifndef __Mac
	if( hk->state & ksm_shift ) {
	    hk->keysym = GDK_KEY_ISO_Left_Tab;
	}
#endif
    }
    
//    fprintf(stderr,"HotkeyParse(end) spec:%d hk->state:%d hk->keysym:%d shortcut:%s\n", GK_Special, hk->state, hk->keysym, shortcut );
}
    
void GMenuItemParseShortCut(GMenuItem *mi,char *shortcut) {
    char *pt, *sh;
    int mask, temp, i;

    mi->short_mask = 0;
    mi->shortcut = '\0';

    sh = dgettext(shortcut_domain,shortcut);
    /* shortcut might be "Open|Ctl+O" meaning the Open menu item is bound to ^O */
    /*  or "CV*Open|Ctl+O" meaning that in the charview the Open menu item ...*/
    /*  but if CV*Open|Ctl+O isn't found then check simple "Open|Ctl+O" as a default */
    if ( sh==shortcut && strlen(shortcut)>2 && shortcut[2]=='*' ) {
	sh = dgettext(shortcut_domain,shortcut+3);
	if ( sh==shortcut+3 )
	    sh = shortcut;
    }
    pt = strchr(sh,'|');
    if ( pt!=NULL )
	sh = pt+1;
    if ( *sh=='\0' || strcmp(sh,"No Shortcut")==0 || strcmp(sh,"None")==0 )
return;

    initmods();

    mask = 0;
    while ( (pt=strchr(sh,'+'))!=NULL && pt!=sh ) {	/* A '+' can also occur as the short cut char itself */
	for ( i=0; modifiers[i].modifier!=NULL; ++i ) {
	    if ( strncasecmp(sh,modifiers[i].modifier,pt-sh)==0 )
	break;
	}
	if ( modifiers[i].modifier==NULL ) {
	    for ( i=0; modifiers[i].alt!=NULL; ++i ) {
		if ( strncasecmp(sh,modifiers[i].alt,pt-sh)==0 )
	    break;
	    }
	}
	if ( modifiers[i].modifier!=NULL )
	    mask |= modifiers[i].mask;
	else if ( sscanf( sh, "0x%x", &temp)==1 )
	    mask |= temp;
	else {
	    fprintf( stderr, "Could not parse short cut: %s\n", shortcut );
return;
	}
	sh = pt+1;
    }
    mi->short_mask = mask;
    for ( i=0; i<0x100; ++i ) {
	if ( GDrawKeysyms[i]!=NULL && uc_strcmp(GDrawKeysyms[i],sh)==0 ) {
	    mi->shortcut = 0xff00 + i;
    break;
	}
    }
    if ( i==0x100 ) {
	if ( mask==0 ) {
	    static int first = true;
	    if ( first ) {
		fprintf( stderr, "Warning: No modifiers in short cut: %s\n", shortcut );
		first = false;
	    }
	}
	mi->shortcut = utf8_ildb((const char **) &sh);
	if ( *sh!='\0' ) {
	    fprintf( stderr, "Unexpected characters at end of short cut: %s\n", shortcut );
return;
	}
    }
}

GMenuItem *GMenuItem2ArrayCopy(GMenuItem2 *mi, uint16 *cnt) {
    int i;
    GMenuItem *arr;

    if ( mi==NULL )
return( NULL );
    for ( i=0; mi[i].ti.text!=NULL || mi[i].ti.image!=NULL || mi[i].ti.line; ++i );
    if ( i==0 )
return( NULL );
    arr = calloc((i+1),sizeof(GMenuItem));
    for ( i=0; mi[i].ti.text!=NULL || mi[i].ti.image!=NULL || mi[i].ti.line; ++i ) {
	arr[i].ti = mi[i].ti;
	GTextInfoImageLookup(&arr[i].ti);
	arr[i].moveto = mi[i].moveto;
	arr[i].invoke = mi[i].invoke;
	arr[i].mid = mi[i].mid;
	if ( mi[i].shortcut!=NULL )
	    GMenuItemParseShortCut(&arr[i],mi[i].shortcut);
	if ( mi[i].ti.text!=NULL ) {
	    if ( mi[i].ti.text_in_resource && mi[i].ti.text_is_1byte )
		arr[i].ti.text = utf82u_mncopy((char *) mi[i].ti.text,&arr[i].ti.mnemonic);
	    else if ( mi[i].ti.text_in_resource )
		arr[i].ti.text = u_copy((unichar_t *) GStringGetResource((intpt) mi[i].ti.text,&arr[i].ti.mnemonic));
	    else if ( mi[i].ti.text_is_1byte )
		arr[i].ti.text = utf82u_copy((char *) mi[i].ti.text);
	    else
		arr[i].ti.text = u_copy(mi[i].ti.text);
	    arr[i].ti.text_in_resource = arr[i].ti.text_is_1byte = false;
	}
	if ( islower(arr[i].ti.mnemonic))
	    arr[i].ti.mnemonic = toupper(arr[i].ti.mnemonic);
	if ( islower(arr[i].shortcut))
	    arr[i].shortcut = toupper(arr[i].shortcut);
	if ( mi[i].sub!=NULL )
	    arr[i].sub = GMenuItem2ArrayCopy(mi[i].sub,NULL);
    }
    memset(&arr[i],'\0',sizeof(GMenuItem));
    if ( cnt!=NULL ) *cnt = i;
return( arr );
}

/* **************************** String Resources **************************** */

/* This is obsolete now. I use gettext instead */

/* A string resource file should begin with two shorts, the first containing
the number of string resources, and the second the number of integer resources.
(not the number of resources in the file, but the maximum resource index+1)
String resources look like
    <resource number-short> <flag,length-short> <mnemonics?> <unichar_t string>
Integer resources look like:
    <resource number-short> <resource value-int>
(numbers are stored with the high byte first)

We include a resource number because translations may not be provided for all
strings, so we will need to skip around. The flag,length field is a short where
the high-order bit is a flag indicating whether a mnemonic is present and the
remaining 15 bits containing the length of the following char string. If a
mnemonic is present it follows immediately after the flag,length short. After
that comes the string. After that a new string.
After all strings comes the integer list.

By convention string resource 0 should always be present and should be the
name of the language (or some other identifying name).
The first 10 or so resources are used by gadgets and containers and must be
 present even if the program doesn't set any resources itself.
Resource 1 should be the translation of "OK"
Resource 2 should be the translation of "Cancel"
   ...
Resource 7 should be the translation of "Replace"
   ...
*/
static unichar_t lang[] = { 'E', 'n', 'g', 'l', 'i', 's', 'h', '\0' };
static unichar_t ok[] = { 'O', 'k', '\0' };
static unichar_t cancel[] = { 'C', 'a', 'n', 'c', 'e', 'l', '\0' };
static unichar_t _open[] = { 'O', 'p', 'e', 'n', '\0' };
static unichar_t save[] = { 'S', 'a', 'v', 'e', '\0' };
static unichar_t filter[] = { 'F', 'i', 'l', 't', 'e', 'r', '\0' };
static unichar_t new[] = { 'N', 'e', 'w', '.', '.', '.', '\0' };
static unichar_t replace[] = { 'R', 'e', 'p', 'l', 'a', 'c', 'e', '\0' };
static unichar_t fileexists[] = { 'F','i','l','e',' ','E','x','i','s','t','s',  '\0' };
/* "File, %s, exists. Replace it?" */
static unichar_t fileexistspre[] = { 'F','i','l','e',',',' ',  '\0' };
static unichar_t fileexistspost[] = { ',',' ','e','x','i','s','t','s','.',' ','R','e','p','l','a','c','e',' ','i','t','?',  '\0' };
static unichar_t createdir[] = { 'C','r','e','a','t','e',' ','d','i','r','e','c','t','o','r','y','.','.','.',  '\0' };
static unichar_t dirname_[] = { 'D','i','r','e','c','t','o','r','y',' ','n','a','m','e','?',  '\0' };
static unichar_t couldntcreatedir[] = { 'C','o','u','l','d','n','\'','t',' ','c','r','e','a','t','e',' ','d','i','r','e','c','t','o','r','y',  '\0' };
static unichar_t selectall[] = { 'S','e','l','e','c','t',' ','A','l','l',  '\0' };
static unichar_t none[] = { 'N','o','n','e',  '\0' };
static const unichar_t *deffall[] = { lang, ok, cancel, _open, save, filter, new,
	replace, fileexists, fileexistspre, fileexistspost, createdir,
	dirname_, couldntcreatedir, selectall, none, NULL };
static const unichar_t deffallmn[] = { 0, 'O', 'C', 'O', 'S', 'F', 'N', 'R', 0, 0, 0, 'A', 'N' };
static const int deffallint[] = { 55, 100 };

static unichar_t **strarray=NULL; static const unichar_t **fallback=deffall;
static unichar_t *smnemonics=NULL; static const unichar_t *fmnemonics=deffallmn;
static int *intarray; static const int *fallbackint = deffallint;
static int slen=0, flen=sizeof(deffall)/sizeof(deffall[0])-1, ilen=0, filen=sizeof(deffallint)/sizeof(deffallint[0]);

const unichar_t *GStringGetResource(int index,unichar_t *mnemonic) {
    if ( index<0 || (index>=slen && index>=flen ))
return( NULL );
    if ( index<slen && strarray[index]!=NULL ) {
	if ( mnemonic!=NULL ) *mnemonic = smnemonics[index];
return( strarray[index]);
    }
    if ( mnemonic!=NULL && fmnemonics!=NULL )
	*mnemonic = fmnemonics[index];
return( fallback[index]);
}

int GIntGetResource(int index) {
    if ( _ggadget_use_gettext && index<2 ) {
	static int gt_intarray[2];
	if ( gt_intarray[0]==0 ) {
	    char *pt, *end;
/* GT: This is an unusual string. It is used to get around a limitation in */
/* GT: FontForge's widget set. You should put a number here (do NOT translate */
/* GT: "GGadget|ButtonSize|", that's only to provide context. The number should */
/* GT: be the number of points used for a standard sized button. It should be */
/* GT: big enough to contain "OK", "Cancel", "New...", "Edit...", "Delete" */
/* GT: (in their translated forms of course). */
	    pt = S_("GGadget|ButtonSize|55");
	    gt_intarray[0] = strtol(pt,&end,10);
	    if ( pt==end || gt_intarray[0]<20 || gt_intarray[0]>4000 )
		gt_intarray[0]=55;
/* GT: This is an unusual string. It is used to get around a limitation in */
/* GT: FontForge's widget set. You should put a number here (do NOT translate */
/* GT: "GGadget|ScaleFactor|", that's only to provide context. The number should */
/* GT: be a percentage and indicates the ratio of the length of a string in */
/* GT: your language to the same string's length in English. */
/* GT: Suppose it takes 116 pixels to say "Ne pas enregistrer" in French but */
/* GT: only 67 pixels to say "Don't Save" in English. Then a value for ScaleFactor */
/* GT: might be 116*100/67 = 173 */
	    pt = S_("GGadget|ScaleFactor|100");
	    gt_intarray[1] = strtol(pt,&end,10);
	    if ( pt==end || gt_intarray[1]<20 || gt_intarray[1]>4000 )
		gt_intarray[1]=100;
	}
return( gt_intarray[index] );
    }

    if ( index<0 || (index>=ilen && index>=filen ))
return( -1 );
    if ( index<ilen && intarray[index]!=0x80000000 ) {
return( intarray[index]);
    }
return( fallbackint[index]);
}

static int getushort(FILE *file) {
    int ch;

    ch = getc(file);
    if ( ch==EOF )
return( EOF );
return( (ch<<8)|getc(file));
}

static int getint(FILE *file) {
    int ch;

    ch = getc(file);
    if ( ch==EOF )
return( EOF );
    ch = (ch<<8)|getc(file);
    ch = (ch<<8)|getc(file);
return( (ch<<8)|getc(file));
}

int GStringSetResourceFileV(char *filename,uint32 checksum) {
    FILE *res;
    int scnt, icnt;
    int strlen;
    int i,j;

    if ( filename==NULL ) {
	if ( strarray!=NULL )
	    for ( i=0; i<slen; ++i ) free( strarray[i]);
	free(strarray); free(smnemonics); free(intarray);
	strarray = NULL; smnemonics = NULL; intarray = NULL;
	slen = ilen = 0;
return( 1 );
    }

    res = fopen(filename,"r");
    if ( res==NULL )
return( 0 );

    if ( getint(res)!=checksum && checksum!=0xffffffff ) {
	fprintf( stderr, "Warning: The checksum of the resource file\n\t%s\ndoes not match the expected checksum.\nA set of fallback resources will be used instead.\n", filename );
	fclose(res);
return( 0 );
    }

    scnt = getushort(res);
    icnt = getushort(res);
    if ( strarray!=NULL )
	for ( i=0; i<slen; ++i ) free( strarray[i]);
    free(strarray); free(smnemonics); free(intarray);
    strarray = calloc(scnt,sizeof(unichar_t *));
    smnemonics = calloc(scnt,sizeof(unichar_t));
    intarray = malloc(icnt*sizeof(int));
    for ( i=0; i<icnt; ++i ) intarray[i] = 0x80000000;
    slen = ilen = 0;

    i = -1;
    while ( i+1<scnt ) {
	i = getushort(res);
	if ( i>=scnt || i==EOF ) {
	    fclose(res);
return( 0 );
	}
	strlen = getushort(res);
	if ( strlen&0x8000 ) {
	    smnemonics[i] = getushort(res);
	    strlen &= ~0x8000;
	}
	strarray[i] = malloc((strlen+1)*sizeof(unichar_t));
	for ( j=0; j<strlen; ++j )
	    strarray[i][j] = getushort(res);
	strarray[i][j] = '\0';
    }

    i = -1;
    while ( i+1<icnt ) {
	i = getushort(res);
	if ( i>=icnt || i==EOF ) {
	    fclose(res);
return( 0 );
	}
	intarray[i] = getint(res);
    }
    fclose(res);
    slen = scnt; ilen = icnt;

return( true );
}

int GStringSetResourceFile(char *filename) {
return( GStringSetResourceFileV(filename,0xffffffff));
}

/* Read a resource from a file without loading the file */
/*  I suspect this will just be used to get the language from the file */
unichar_t *GStringFileGetResource(char *filename, int index,unichar_t *mnemonic) {
    int scnt;
    FILE *res;
    int i,j, strlen;
    unichar_t *str;

    if ( filename==NULL )
return( uc_copy("Default"));

    res = fopen(filename,"r");
    if ( res==NULL )
return( 0 );

    scnt = getushort(res);
    /* icnt = */getushort(res);
    if ( index<0 || index>=scnt ) {
	fclose(res);
return( NULL );
    }

    i = -1;
    while ( i+1<=scnt ) {
	i = getushort(res);
	if ( i>=scnt ) {
	    fclose(res);
return( NULL );
	}
	strlen = getushort(res);
	if ( i==index ) {
	    if ( strlen&0x8000 ) {
		int temp = getushort(res);
		if ( mnemonic!=NULL ) *mnemonic = temp;
		strlen &= ~0x8000;
	    }
	    str = malloc((strlen+1)*sizeof(unichar_t));
	    for ( j=0; j<strlen; ++j )
		str[j] = getushort(res);
	    str[j] = '\0';
	    fclose( res );
return( str );
	} else {
	    if ( strlen&0x8000 ) {
		getushort(res);
		strlen &= ~0x8000;
	    }
	    for ( j=0; j<strlen; ++j )
		getushort(res);
	}
    }
    fclose( res );
return( NULL );
}
    
void GStringSetFallbackArray(const unichar_t **array,const unichar_t *mn,const int *ires) {
    int i=0;

    if ( array!=NULL ) while ( array[i]!=NULL ) ++i;
    flen = i;
    fallback = array;
    fmnemonics = mn;

    i=0;
    if ( ires!=NULL ) while ( ires[i]!=0x80000000 ) ++i;
    filen = i;

}

int _ggadget_use_gettext = false;
void GResourceUseGetText(void) {
    _ggadget_use_gettext = true;
}
