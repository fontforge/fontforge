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
#include "fontforgevw.h"
#include "sd.h"
#include "xvasprintf.h"
#include "ffglib.h"

#include <unistd.h>
#ifdef __CYGWIN__
# include <sys/cygwin.h>
#endif

/* Preference variables (Global) */
int preferpotrace = false;
int autotrace_ask=0, mf_ask=0, mf_clearbackgrounds=0, mf_showerrors=0;
char *mf_args = NULL;
/* Storing autotrace args */
static char *g_autotrace_args;
static gchar **g_args=NULL;

/* Interface to Martin Weber's autotrace program   */
/*  http://homepages.go.com/~martweb/AutoTrace.htm */
/*  Oops, now at http://sourceforge.net/projects/autotrace/ */

/* Also interface to Peter Selinger's potrace program (which does the same thing */
/*  and has a cleaner interface) */
/* http://potrace.sf.net/ */

/**
 * Generates a temporary name.
 * On all platforms exept Windows (MINGW), this function behaves similar to
 * mkstemp - the return value is the file descriptor to the temporary file.
 * On Windows, the return value is non-meaningful, except to indicate success.
 * The return value should be closed by mytempnam_close.  
 * 
 * @param [out] path The location to store the name of the temporary file.
 * @return -1 on error, any other value otherwise.
 */
static int mytempnam(char **path) {
#if !defined(__MINGW32__)
    int retval = g_file_open_tmp("PfaEdXXXXXX", path, NULL);
# if defined (__CYGWIN__)
    if (retval != -1) {
        size_t r = cygwin_conv_path(CCP_POSIX_TO_WIN_W, *path, NULL, 0);
        gchar *fret = NULL;
        
        if (r >= 0) {
            wchar_t *tret = malloc(sizeof(wchar_t) * r);
            
            r = cygwin_conv_path(CCP_POSIX_TO_WIN_W, *path, tret, r);
            if (r == 0) {
                //It should work as long as a UTF-8 locale is selected...
                fret = g_utf16_to_utf8(tret, -1, NULL, NULL, NULL);
            }
            free(tret);
        }
        g_free(*path);
        *path = fret;
        
        if (fret == NULL) {
            close(retval);
            retval = -1;
        } else {
            GFileNormalizePath(fret);
        }
    }
# endif
    return retval;
#else
    wchar_t *temp = _wtempnam(NULL, L"PfaEd");
    *path = g_utf16_to_utf8(temp, -1, NULL, NULL, NULL);
    free(temp);
    if (*path) {
        GFileNormalizePath(*path);
        return 1;
    }
    return -1;
#endif
}

/**
 * Closes the file descriptor (if any) and frees the path as allocated by
 * mytempnam. 
 * 
 * @param [in] fd The return value from mytempnam.
 * @param [in] path The return value from mytempnam.
 */
static void mytempnam_close(int fd, char **path) {
    if (fd != -1) {
#if !defined(__MINGW32__)
        close(fd);
#endif
        if (path && *path) {
            g_unlink(*path);
            g_free(*path);
            *path = NULL;
        }
    }
}

/**
 * Performs a shallow delete and removal of a temporary folder.
 * This function will remove all top-level files within a folder before
 * attempting to delete the folder itself. 
 * 
 * @param [in] tempdir The folder to be deleted.
 */
static void cleantempdir(char *tempdir) {
    GDir *dir;
    const gchar *entry;
    gchar *todelete[100];
    size_t cnt = 0;

    if ((dir = g_dir_open(tempdir, 0, NULL))) {
        while ((entry = g_dir_read_name(dir)) && cnt < sizeof(todelete)) {
            /* Hmm... doing an unlink right here means changing the dir file */
            /*  which might mean we could not read it properly. So save up the*/
            /*  things we need to delete and trash them later */
            todelete[cnt++] = g_build_path("/", tempdir, entry, NULL);
        }
        g_dir_close(dir);
        
        for (size_t i = 0; i < cnt; i++) {
            g_unlink(todelete[i]);
            g_free(todelete[i]);
        }
    }
    g_rmdir(tempdir);
}

static SplinePointList *localSplinesFromEntities(Entity *ent, Color bgcol, int ispotrace) {
    Entity *enext;
    SplinePointList *head=NULL, *last, *test, *next, *prev, *new, *nlast, *temp;
    int clockwise;
    SplineChar sc;
    StrokeInfo si;
    DBounds bb, sbb;
    int removed;
    real fudge;
    Layer layers[2];

    /* We have a problem. The autotrace program includes contours for the */
    /*  background color (there's supposed to be a way to turn that off, but */
    /*  it didn't work when I tried it, so...). I don't want them, so get */
    /*  rid of them. But we must be a bit tricky. If the contour specifies a */
    /*  counter within the letter (the hole in the O for instance) then we */
    /*  do want the contour, but we want it to be counterclockwise. */
    /* So first turn all background contours counterclockwise, and flatten */
    /*  the list */
    /* potrace does not have this problem */
    /* But potrace does not close its paths (well, it closes the last one) */
    /*  and as with standard postscript fonts I need to reverse the splines */
    unsigned bgr = COLOR_RED(bgcol), bgg = COLOR_GREEN(bgcol), bgb = COLOR_BLUE(bgcol);

    memset(&sc,'\0',sizeof(sc));
    memset(layers,0,sizeof(layers));
    sc.layers = layers;
    for ( ; ent!=NULL; ent = enext ) {
	enext = ent->next;
	if ( ent->type == et_splines ) {
	    if ( /* ent->u.splines.fill.col==0xffffffff && */ ent->u.splines.stroke.col!=0xffffffff ) {
		memset(&si,'\0',sizeof(si));
		si.join = ent->u.splines.join;
		si.cap = ent->u.splines.cap;
		si.radius = ent->u.splines.stroke_width/2;
		new = NULL;
		for ( test = ent->u.splines.splines; test!=NULL; test=test->next ) {
		    temp = SplineSetStroke(test,&si,false);
		    if ( new==NULL )
			new=temp;
		    else
			nlast->next = temp;
		    for ( nlast=temp; nlast->next!=NULL; nlast=nlast->next );
		}
		SplinePointListsFree(ent->u.splines.splines);
		ent->u.splines.fill.col = ent->u.splines.stroke.col;
	    } else {
		new = ent->u.splines.splines;
	    }
	    if ( head==NULL )
		head = new;
	    else
		last->next = new;
	    if ( ispotrace ) {
		for ( test = new; test!=NULL; test=test->next ) {
		    if ( test->first!=test->last &&
			    RealNear(test->first->me.x,test->last->me.x) &&
			    RealNear(test->first->me.y,test->last->me.y)) {
			test->first->prevcp = test->last->prevcp;
			test->first->noprevcp = test->last->noprevcp;
			test->first->prevcpdef = test->last->prevcpdef;
			test->first->prev = test->last->prev;
			test->last->prev->to = test->first;
			SplinePointFree(test->last);
			test->last=test->first;
		    }
		    SplineSetReverse(test);
		    last = test;
		}
	    } else {
		for ( test = new; test!=NULL; test=test->next ) {
		    clockwise = SplinePointListIsClockwise(test)==1;
		    /* colors may get rounded a little as we convert from RGB to */
		    /*  a postscript color and back. */
		    if ( COLOR_RED(ent->u.splines.fill.col)>=bgr-2 && COLOR_RED(ent->u.splines.fill.col)<=bgr+2 &&
			    COLOR_GREEN(ent->u.splines.fill.col)>=bgg-2 && COLOR_GREEN(ent->u.splines.fill.col)<=bgg+2 &&
			    COLOR_BLUE(ent->u.splines.fill.col)>=bgb-2 && COLOR_BLUE(ent->u.splines.fill.col)<=bgb+2 ) {
			if ( clockwise )
			    SplineSetReverse(test);
		    } else {
			if ( !clockwise )
			    SplineSetReverse(test);
			clockwise = SplinePointListIsClockwise(test)==1;
		    }
		    last = test;
		}
	    }
	}
	SplinePointListsFree(ent->clippath);
	free(ent);
    }

    /* Then remove all counter-clockwise (background) contours which are at */
    /*  the edge of the character */
    if ( !ispotrace ) do {
	removed = false;
	sc.layers[ly_fore].splines = head;
	SplineCharFindBounds(&sc,&bb);
	fudge = (bb.maxy-bb.miny)/64;
	if ( (bb.maxx-bb.minx)/64 > fudge )
	    fudge = (bb.maxx-bb.minx)/64;
	for ( last=head, prev=NULL; last!=NULL; last=next ) {
	    next = last->next;
	    if ( SplinePointListIsClockwise(last)==0 ) {
		last->next = NULL;
		SplineSetFindBounds(last,&sbb);
		last->next = next;
		if ( sbb.minx<=bb.minx+fudge || sbb.maxx>=bb.maxx-fudge ||
			sbb.maxy >= bb.maxy-fudge || sbb.miny <= bb.miny+fudge ) {
		    if ( prev==NULL )
			head = next;
		    else
			prev->next = next;
		    last->next = NULL;
		    SplinePointListFree(last);
		    removed = true;
		} else
		    prev = last;
	    } else
		prev = last;
	}
    } while ( removed );
return( head );
}

static void _SCAutoTrace(SplineChar *sc, int layer, char **args) {
    ImageList *images;
    char prog[BUFSIZ];
    int changed = false;
    int ispotrace;

    if (sc->layers[ly_back].images == NULL)
        return;
    if ((ispotrace = FindAutoTrace(prog, sizeof(prog))) < 0)
        return;

    for (images = sc->layers[ly_back].images; images!=NULL; images=images->next) {
        Color bgcol;
        const char (* arglist[30]);
        char *tempname_input, *tempname_output;
        struct _GImage *ib = images->image->list_len==0 ?
                             images->image->u.image : images->image->u.images[0];
        int fd_input, fd_output;
        int status, exec_ret;
        size_t ac,i;

        if ( ib->width==0 || ib->height==0 ) {
            /* pk fonts can have 0 sized bitmaps for space characters */
            /*  but autotrace gets all snooty about being given an empty image */
            /*  so if we find one, then just ignore it. It won't produce any */
            /*  results anyway */
            continue;
        }
        if ((fd_input = mytempnam(&tempname_input)) == -1) {
            return;
        }
        if ((fd_output = mytempnam(&tempname_output)) == -1) {
            mytempnam_close(fd_input, &tempname_input);
            return;
        }
        GImageWriteBmp(images->image, tempname_input);
        if ( ib->trans==(Color)-1 )
            bgcol = 0xffffff;		/* reasonable guess */
        else if ( ib->image_type==it_true )
            bgcol = ib->trans;
        else if ( ib->clut!=NULL )
            bgcol = ib->clut->clut[ib->trans];
        else
            bgcol = 0xffffff;

        ac = 0;
        arglist[ac++] = prog;
        if (ispotrace) {
            /* If I use the long names (--cleartext) potrace hangs) */
            /*  version 1.1 */
            arglist[ac++] = "-c";
            arglist[ac++] = "--eps";
            arglist[ac++] = "-r";
            arglist[ac++] = "72";
            arglist[ac++] = "--output";
        } else {
            arglist[ac++] = "--output-format=eps";
            arglist[ac++] = "--input-format=BMP";
            arglist[ac++] = "--output-file";
        }
        arglist[ac++] = tempname_output;
        if (args) {
            for ( i=0; args[i] != NULL && ac < sizeof(arglist)/sizeof(arglist[0])-2; ++i)
                arglist[ac++] = args[i];
        }
        arglist[ac++] = tempname_input;
        arglist[ac] = NULL;
        /* We can't use AutoTrace's own "background-color" ignorer because */
        /*  it ignores counters as well as surrounds. So "O" would be a dark */
        /*  oval, etc. */
        exec_ret = g_spawn_sync(NULL, (gchar**)arglist, NULL,
            G_SPAWN_SEARCH_PATH|G_SPAWN_STDOUT_TO_DEV_NULL|G_SPAWN_STDERR_TO_DEV_NULL,
            NULL, NULL, NULL, NULL, &status, NULL);
        if (exec_ret && GFileCheckGlibSpawnStatus(status)) {
            FILE *fpo = fopen(tempname_output, "r");
            if (fpo) {
                SplineSet *new, *last;
                real transform[6];
                new = localSplinesFromEntities(EntityInterpretPS(fpo,NULL),bgcol,ispotrace);
                transform[0] = images->xscale; transform[3] = images->yscale;
                transform[1] = transform[2] = 0;
                transform[4] = images->xoff;
                transform[5] = images->yoff - images->yscale*ib->height;
                new = SplinePointListTransform(new,transform,tpt_AllPoints);
                if (sc->layers[layer].order2) {
                    SplineSet *o2 = SplineSetsTTFApprox(new);
                    SplinePointListsFree(new);
                    new = o2;
                }
                if (new != NULL) {
                    sc->parent->onlybitmaps = false;
                    if (!changed)
                        SCPreserveLayer(sc,layer,false);
                    for (last=new; last->next!=NULL; last=last->next);

                    last->next = sc->layers[layer].splines;
                    sc->layers[layer].splines = new;
                    changed = true;
                }
            }
            fclose(fpo);
        }
        mytempnam_close(fd_input, &tempname_input);
        mytempnam_close(fd_output, &tempname_output);
    }
    if (changed) {
        SCCharChangedUpdate(sc,layer);
    }
}

/**
 * Retrieves the argument list to pass to the autotracer, asking the user for
 * extra parameters (if they have chosen to do so from preferences). 
 * 
 * @return The autotrace argument list.
 */
static char **QueryAutoTraceArgs(int ask) {
    if ((ask || autotrace_ask) && !no_windowing_ui) {
        char *cret;

        cret = ff_ask_string(_("Additional arguments for autotrace program:"),
            g_autotrace_args,_("Additional arguments for autotrace program:"));
        if (cret == NULL)
            return (char **)-1;
        SetAutoTraceArgs(cret);
        SavePrefs(true);
        free(cret);
    }
    return g_args;
}

/**
 * Retrieves a copy of the current autotrace arguments.
 * This exists solely for the preferences system. 
 * 
 * @return The autotrace arguments, or NULL if none. Result must be freed.
 */
void *GetAutoTraceArgs(void) {
    return copy(g_autotrace_args);
}

/**
 * Updates the parameter list to pass to the autotrace program.
 * This function prototype is necessary as it is used by the preferences system. 
 * 
 * @param [in] a The character string of arguments.
 */
void SetAutoTraceArgs(void *a) {
    free(g_autotrace_args);
    g_strfreev(g_args);
    if (a == NULL || !g_shell_parse_argv((char *)a, NULL, &g_args, NULL)) {
        g_args = NULL;
    }
    g_autotrace_args = copy((char *)a);
}

/**
 * Traces all glyphs that have an associated image (from the font view).
 * 
 * @param [in] fv The font view to scan glyphs from.
 * @param [in] ask Whether or not to ask for extra autotrace parameters. 
 */
void FVAutoTrace(FontViewBase *fv,int ask) {
    char **args;
    int i,cnt,gid;

    if ( FindAutoTrace(NULL, 0) < 0 ) {
	ff_post_error(_("Can't find autotrace"),_("Can't find autotrace program (set AUTOTRACE environment variable) or download from:\n  http://sf.net/projects/autotrace/"));
return;
    }

    args = QueryAutoTraceArgs(ask);
    if ( args==(char **) -1 )
return;
    for ( i=cnt=0; i<fv->map->enccount; ++i )
	if ( fv->selected[i] && (gid=fv->map->map[i])!=-1 &&
		fv->sf->glyphs[gid]!=NULL &&
		fv->sf->glyphs[gid]->layers[ly_back].images )
	    ++cnt;

    ff_progress_start_indicator(10,_("Autotracing..."),_("Autotracing..."),0,cnt,1);

    SFUntickAll(fv->sf);
    for ( i=cnt=0; i<fv->map->enccount; ++i ) {
	if ( fv->selected[i] && (gid=fv->map->map[i])!=-1 &&
		fv->sf->glyphs[gid]!=NULL &&
		fv->sf->glyphs[gid]->layers[ly_back].images &&
		!fv->sf->glyphs[gid]->ticked ) {
	    _SCAutoTrace(fv->sf->glyphs[gid], fv->active_layer, args);
	    if ( !ff_progress_next())
    break;
	}
    }
    ff_progress_end_indicator();
}

/**
 * Performs an auto trace for a given character.
 * 
 * @param [in] sc The character to autotrace.
 * @param [in] layer The layer to place the trace on.
 * @param [in] ask Whether or not to ask for extra autotrace parameters.
 * @return true if the autotrace was probably started.
 */
int SCAutoTrace(SplineChar *sc,int layer, int ask) {
    char **args;

    if (sc->layers[ly_back].images == NULL) {
        ff_post_error(_("Nothing to trace"),_("Nothing to trace"));
        return false;
    } else if (FindAutoTrace(NULL, 0) < 0) {
        ff_post_error(_("Can't find autotrace"),_("Can't find autotrace program (set AUTOTRACE environment variable) or download from:\n  http://sf.net/projects/autotrace/"));
        return false;
    }

    args = QueryAutoTraceArgs(ask);
    if (args==(char **) -1)
        return false;
    _SCAutoTrace(sc, layer, args);
    return true;
}


/**
 * Finds the autotrace/potrace executable.
 * 
 * @param [out] path The buffer to store the location, or NULL if not required.
 * @param [in] bufsiz The size of the output buffer.
 * @return nonzero if potrace was found, zero if autotrace was found, negative
 *         if none was found. Note previously, FontForge determined itself if
 *         the resulting executable was potrace or autotrace, irrespective of
 *         setting AUTOTRACE or POTRACE. (i.e. AUTOTRACE=potrace was possible).
 */
int FindAutoTrace(char *path, size_t bufsiz) {
    const char * const envtraces[] = {"AUTOTRACE", "POTRACE"};
    const char * const progtraces[] = {GFILE_PROGRAMIFY("autotrace"),
                                       GFILE_PROGRAMIFY("potrace")};
    const char *location;
    int idx = (preferpotrace ? 1 : 0), ret = -1;
    
    location = g_getenv(envtraces[idx]);
    if (!location) {
        location = g_getenv(envtraces[1-idx]);
    }
    if (!location && GFileProgramExists(progtraces[idx])) {
        location = progtraces[idx];
        ret = idx;
    }
    if (!location && GFileProgramExists(progtraces[1-idx])) {
        location = progtraces[1-idx];
        ret = 1-idx;
    }
    
    if (location) {
        if (path) {
            snprintf(path, bufsiz, "%s", location);
        }
        return ret >= 0 ?  ret : (strstrmatch(location, "potrace") != NULL);
    }

    return ret;
}

/**
 * Attempts to find the MetaFont executable.
 * 
 * @return The path to the MetaFont executable, or NULL if not found.
 *         The return value should not be freed.
 */
static const char *FindMFName(void) {
    static bool searched = false;
    static char name[BUFSIZ];
    const char *pt;

    if (!searched) {
        searched = true;
        if ((pt = g_getenv("MF")) != NULL) {
            snprintf(name, sizeof(name), "%s", pt);
        } else if (GFileProgramExists(GFILE_PROGRAMIFY("mf"))) {
            snprintf(name, sizeof(name), GFILE_PROGRAMIFY("mf"));
        }
    }
    return name;
}

/**
 * Finds the output produced by running MetaFont.
 * 
 * @param [in] tempdir The directory to search.
 * @param [out] buffer The buffer to store the file location.
 * @param [in] bufsiz the size of the output buffer.
 * @return true iff found.
 */
static bool FindGfFile(char *tempdir, char *buffer, size_t bufsiz) {
    GDir *dir;
    const gchar *entry;
    bool ret = false;

    if ((dir = g_dir_open(tempdir, 0, NULL))) {
        while ((entry = g_dir_read_name(dir)) != NULL) {
            size_t elen = strlen(entry);
            if (elen > 2 && !strcmp(entry+elen-2, "gf")) {
                elen = snprintf(buffer, bufsiz, "%s/%s", tempdir, entry);
                if (elen >= 0 && elen < bufsiz) {
                    ret = true;
                }
                break;
            }
        }
        g_dir_close(dir);
    }
    return ret;
}

/**
 * Retrieves the argument string to pass to MetaFont, asking the user for
 * extra parameters (if they have chosen to do so from preferences). 
 * 
 * @return The MetaFont argument string.
 */
static char *QueryMfArgs(void) {
    MfArgsInit();

    if (mf_ask && !no_windowing_ui) {
        char *ret;

        ret = ff_ask_string(_("Additional arguments for autotrace program:"),
            mf_args,_("Additional arguments for autotrace program:"));
        if (ret == NULL)
            return (char *) -1;

        free(mf_args);
        mf_args = ret;
        SavePrefs(true);
    }
    return mf_args;
}

/**
 * Initialises the MetaFont default arguments list, if necessary.
 */
void MfArgsInit(void) {
    if (mf_args == NULL)
        mf_args = copy("\\scrollmode; mode=proof ; mag=2; input");
}

/**
 * Convert a MetaFont into a spline font.
 * 
 * @param [in] filename The path to the font.
 * @return The spline font on success, or NULL otherwise.
 */
SplineFont *SFFromMF(char *filename) {
    gchar *tempdir;
    char *arglist[8];
    int status, ac, i, spawn_flags;
    SplineFont *sf=NULL;
    SplineChar *sc;
    bool exec_ret;

    if (FindMFName() == NULL) {
        ff_post_error(_("Can't find mf"),_("Can't find mf program -- metafont (set MF environment variable) or download from:\n  http://www.tug.org/\n  http://www.ctan.org/\nIt's part of the TeX distribution"));
        return NULL;
    } else if (FindAutoTrace(NULL, 0) < 0) {
        ff_post_error(_("Can't find autotrace"),_("Can't find autotrace program (set AUTOTRACE environment variable) or download from:\n  http://sf.net/projects/autotrace/"));
        return NULL;
    }
    
    if (QueryMfArgs()==(char *) -1 || QueryAutoTraceArgs(false)==(char **) -1)
        return NULL;

    /* I don't know how to tell mf to put its files where I want them. */
    /*  so instead I create a temporary directory, cd mf there, and it */
    /*  will put the files there. */
    tempdir = g_dir_make_tmp("PfaEdXXXXXX", NULL);
    if (tempdir == NULL) {
        ff_post_error(_("Can't create temporary directory"),_("Can't create temporary directory"));
        return NULL;
    }

    ac = 0;
    arglist[ac++] = (char *)FindMFName();
    arglist[ac++] = xasprintf("%s \"%s\"", mf_args, filename);
    arglist[ac] = NULL;
    
    spawn_flags = mf_showerrors ? G_SPAWN_CHILD_INHERITS_STDIN :
                  G_SPAWN_STDOUT_TO_DEV_NULL|G_SPAWN_STDERR_TO_DEV_NULL;
    exec_ret = g_spawn_sync(tempdir, (gchar**)arglist, NULL,
        G_SPAWN_SEARCH_PATH|spawn_flags, NULL, NULL, NULL, NULL, &status, NULL);
    if (exec_ret && GFileCheckGlibSpawnStatus(status)) {
        char gffile[BUFSIZ];
        
        if (!FindGfFile(tempdir, gffile, sizeof(gffile))) {
            ff_post_error(_("Can't run mf"), _("Could not read (or perhaps find) mf output file"));
        } else {
            ff_progress_show();
            sf = SFFromBDF(gffile, 3, true);
            if (sf != NULL) {
                ff_progress_change_line1(_("Autotracing..."));
                ff_progress_change_total(sf->glyphcnt);
                for (i=0; i<sf->glyphcnt; ++i) {
                    if ((sc = sf->glyphs[i]) != NULL && sc->layers[ly_back].images) {
                        _SCAutoTrace(sc, ly_fore, g_args);
                        if (mf_clearbackgrounds) {
                            GImageDestroy(sc->layers[ly_back].images->image);
                            free(sc->layers[ly_back].images);
                            sc->layers[ly_back].images = NULL;
                        }
                    }
                    if (!ff_progress_next())
                        break;
                }
            } else {
                ff_post_error(_("Can't run mf"),_("Could not read (or perhaps find) mf output file"));
            }
        }
    } else {
        ff_post_error(_("Can't run mf"),_("Can't run mf"));
    }
    free(arglist[1]);
    cleantempdir(tempdir);
    g_free(tempdir);
    return sf;
}
