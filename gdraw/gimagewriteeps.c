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
#include "gdraw.h"
#include <string.h>

int GImageWriteEps(GImage *gi, char *filename) {
    struct _GImage *base = gi->list_len==0?gi->u.image:gi->u.images[0];
    GPrinterAttrs pattrs;
    GWindow epsfile;

    memset(&pattrs,'\0',sizeof(pattrs));
    pattrs.mask |= pam_pagesize;
	pattrs.width = base->width/72; pattrs.height = base->height/72;
    pattrs.mask |= pam_margins;
	pattrs.lmargin = pattrs.rmargin = pattrs.tmargin = pattrs.bmargin = 0;
    pattrs.mask |= pam_scale;
	pattrs.scale = 1.0;
    pattrs.mask |= pam_res;
	pattrs.res = 72;
    pattrs.mask |= pam_color;
	pattrs.do_color = true;
	if (( base->clut==NULL && base->image_type==it_mono ) ||
		(base->clut!=NULL && GImageGreyClut(base->clut)) )
	    pattrs.do_color = false;
    pattrs.mask |= pam_queue;
	pattrs.donot_queue = true;
    pattrs.mask |= pam_eps;
	pattrs.eps = true;
    pattrs.mask |= pam_filename;
	pattrs.file_name = filename;

    epsfile = GPrinterStartJob(NULL,NULL,&pattrs);
    if ( epsfile==NULL )
return( false );
    GDrawDrawImage(epsfile,gi,NULL,0,0);
return( GPrinterEndJob(epsfile,false));
}
