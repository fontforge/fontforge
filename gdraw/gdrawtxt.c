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

#include "fontP.h"
#include "gdrawP.h"
#include "gxcdrawP.h"
#include "ustring.h"

FontInstance *GDrawSetFont(GWindow gw, FontInstance *fi) {
    FontInstance *old = gw->ggc->fi;
    gw->ggc->fi = fi;
return( old );
}

FontInstance *GDrawInstanciateFont(GWindow gw, FontRequest *rq) {
    GDisplay *gdisp;
    FState *fs;
    struct font_instance *fi;

    if (gw == NULL)
	gw = GDrawGetRoot(NULL);

    gdisp = gw->display;
    fs = gdisp->fontstate;

    if ( rq->point_size<0 )	/* It's in pixels, not points, convert to points */
	rq->point_size = PixelToPoint(-rq->point_size,fs->res);

    fi = calloc(1,sizeof(struct font_instance));
    fi->rq = *rq;
    fi->rq.family_name = u_copy( fi->rq.family_name );
    fi->rq.utf8_family_name = copy( fi->rq.utf8_family_name );

return( fi );
}

FontInstance *GDrawAttachFont(GWindow gw, FontRequest *rq) {
    struct font_instance *fi = GDrawInstanciateFont(gw,rq);

    if ( fi!=NULL )
	GDrawSetFont(gw, fi);
return( fi );
}

FontRequest *GDrawDecomposeFont(FontInstance *fi, FontRequest *rq) {
    *rq = fi->rq;
return( rq );
}

/* ************************************************************************** */

int32 GDrawDrawText(GWindow gw, int32 x, int32 y, const unichar_t *text, int32 cnt, Color col) {
    struct tf_arg arg;
    glong realcnt;
    gchar *temp = g_ucs4_to_utf8((gunichar*)text, cnt, NULL, &realcnt, NULL);
    int width = 0;

    if (temp != NULL) {
        width = gw->display->funcs->doText8(gw,x,y,temp,realcnt,col,tf_drawit,&arg);
        g_free(temp);
    }

    return width;
}

int32 GDrawGetTextWidth(GWindow gw,const unichar_t *text, int32 cnt) {
    struct tf_arg arg;
    glong realcnt;
    gchar *temp = g_ucs4_to_utf8((gunichar*)text, cnt, NULL, &realcnt, NULL);
    int width = 0;

    if (temp != NULL) {
        width = gw->display->funcs->doText8(gw,0,0,temp,realcnt,0x0,tf_width,&arg);
        g_free(temp);
    }
    return width;
}

int32 GDrawGetTextBounds(GWindow gw,const unichar_t *text, int32 cnt, GTextBounds *bounds) {
    int ret = 0;
    struct tf_arg arg = {0};
    glong realcnt;
    gchar *temp = g_ucs4_to_utf8((gunichar*)text, cnt, NULL, &realcnt, NULL);

    if (temp != NULL) {
        arg.first = true;
        ret = gw->display->funcs->doText8(gw,0,0,temp,realcnt,0x0,tf_rect,&arg);
        *bounds = arg.size;
        g_free(temp);
    }
    return ret;
}

/* UTF8 routines */

int32 GDrawDrawText8(GWindow gw, int32 x, int32 y, const char *text, int32 cnt, Color col) {
    struct tf_arg arg;

return( gw->display->funcs->doText8(gw,x,y,text,cnt,col,tf_drawit,&arg));
}

int32 GDrawGetText8Width(GWindow gw, const char *text, int32 cnt) {
    struct tf_arg arg;

return( gw->display->funcs->doText8(gw,0,0,text,cnt,0x0,tf_width,&arg));
}

int32 GDrawGetText8Height(GWindow gw, const char *text, int32 cnt)
{
    GTextBounds bounds;
    int32 ret = GDrawGetText8Bounds( gw, text, cnt, &bounds );
    ret = bounds.as + bounds.ds;
    return ret;
}


int32 GDrawGetText8Bounds(GWindow gw,const char *text, int32 cnt, GTextBounds *bounds) {
    int ret;
    struct tf_arg arg;

    memset(&arg,'\0',sizeof(arg));
    arg.first = true;
    ret = gw->display->funcs->doText8(gw,0,0,text,cnt,0x0,tf_rect,&arg);
    *bounds = arg.size;
return( ret );
}

/* End UTF8 routines */
