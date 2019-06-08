/* Copyright (C) 2005-2012 by George Williams */
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

#ifndef FONTFORGE_GROUPS_H
#define FONTFORGE_GROUPS_H


typedef struct ffgroup {
    char *name;			/* The name of this group (utf8) */
    struct ffgroup *parent;	/* parent of this group (NULL for group root) */
    int kid_cnt;		/* Number of sub-groups */
    struct ffgroup **kids;	/* The sub-groups */
    char *glyphs;		/* Or, if a terminal node, a list of glyph names/Unicodes */
    unsigned int unique: 1;	/* If set => set in all kids & a glyph name may only appear once in all terminal groups */
/* Used by the dialog */
    unsigned int open: 1;
    unsigned int selected : 1;
    int lpos;
} Group;

extern Group *group_root;

struct fontview;

void SaveGroupList(void);
void LoadGroupList(void);
Group *GroupCopy(Group *g);
void GroupFree(Group *g);

#endif /* FONTFORGE_GROUPS_H */
