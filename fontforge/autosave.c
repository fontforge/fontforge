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
#include <sys/types.h>
#include <unistd.h>

int AutoSaveFrequency=5;

static char *GetAutoDirName() {
    char *userdir = getFontForgeUserDir(Config);
    char *autodir = NULL;
    
    if (userdir != NULL) {
        int ret = asprintf(&autodir, "%s/autosave", userdir);
        free(userdir);
        if (ret == -1) {
            return NULL;
        } else if (!GFileExists(autodir) && GFileMkDir(autodir) == -1) {
            free(autodir);
            return NULL;
        }
    }
    return autodir;
}

static bool MakeAutoSaveName(SplineFont *sf) {
    char buffer[1025];
    char *autosavedir;
    static int cnt=0;

    if (!sf->autosavename && (autosavedir = GetAutoDirName())) {
        while (true) {
            int nret = snprintf(buffer, sizeof(buffer), "%s/auto%06x-%d.asfd",
                                autosavedir, getpid(), ++cnt);
            if (nret < 0 || nret >= sizeof(buffer)) {
                break;
            } else if (!GFileExists(buffer)) {
                sf->autosavename = copy(buffer);
                break;
            }
        }
        free(autosavedir);
    }
    return (sf->autosavename != NULL);
}

static void _DoAutoSaves(FontViewBase *fvs) {
    FontViewBase *fv;
    SplineFont *sf;

    if (AutoSaveFrequency <= 0)
        return;

    for (fv = fvs; fv != NULL; fv = fv->next ) {
        sf = (fv->cidmaster ? fv->cidmaster : fv->sf);
        if (sf->changed_since_autosave && MakeAutoSaveName(sf)) {
            SFAutoSave(sf, fv->map);
        }
    }
}

void DoAutoSaves(void) {
    _DoAutoSaves(FontViewFirst());
}

int DoAutoRecoveryExtended(int inquire) {
    char buffer[1025];
    char *recoverdir = GetAutoDirName();
    GDir *dir;
    const gchar *entry;
    int any = false;
    SplineFont *sf;
    int inquire_state=0;

    if (recoverdir == NULL) {
        return false;
    } else if ((dir = g_dir_open(recoverdir, 0, NULL)) == NULL) {
        free(recoverdir);
        return false;
    }
    
    while ((entry = g_dir_read_name(dir)) != NULL ) {
        snprintf(buffer, sizeof(buffer), "%s/%s", recoverdir, entry);
        fprintf(stderr, "Recovering from %s... ", buffer);
        if ((sf = SFRecoverFile(buffer, inquire, &inquire_state))) {
            any = true;
            if (sf->fv == NULL) /* Doesn't work, cli arguments not parsed yet */
                FontViewCreate(sf, false);
            fprintf(stderr, " Done\n");
        }
    }
    g_dir_close(dir);
    free(recoverdir);
    return any;
}

int DoAutoRecovery(int inquire) {
    return DoAutoRecoveryExtended(inquire);
}

void CleanAutoRecovery(void) {
    char buffer[1025];
    char *recoverdir = GetAutoDirName();
    GDir *dir;
    const gchar *entry;

    if (recoverdir == NULL || (dir = g_dir_open(recoverdir, 0, NULL)) == NULL) {
        free(recoverdir);
        return;
    }

    while ((entry = g_dir_read_name(dir)) != NULL) {
        snprintf(buffer, sizeof(buffer), "%s/%s", recoverdir, entry);
        if (g_unlink(buffer) != 0) {
            fprintf(stderr, "Failed to clean ");
            perror(buffer);
        }
    }
    free(recoverdir);
    g_dir_close(dir);
}
