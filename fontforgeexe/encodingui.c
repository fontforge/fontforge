/* -*- coding: utf-8 -*- */
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

#include "fontforgeui.h"
#include <ustring.h>
#include <utype.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <gfile.h>
#include <gresource.h>
#include "plugins.h"
#include "encoding.h"

static GTextInfo *EncodingList(void) {
    GTextInfo *ti;
    int i;
    Encoding *item;

    i = 0;
    for ( item=enclist; item!=NULL ; item=item->next )
	if ( !item->builtin )
	    ++i;
    ti = calloc(i+1,sizeof(GTextInfo));
    i = 0;
    for ( item=enclist; item!=NULL ; item=item->next )
	if ( !item->builtin )
	    ti[i++].text = uc_copy(item->enc_name);
    if ( i!=0 )
	ti[0].selected = true;
return( ti );
}

#define CID_Encodings	1001

static int DE_Delete(GGadget *g, GEvent *e) {
    GWindow gw;
    int *done;
    GGadget *list;
    int sel,i;
    Encoding *item;

    if ( e->type==et_controlevent &&
	    (e->u.control.subtype == et_buttonactivate ||
	     e->u.control.subtype == et_listdoubleclick )) {
	gw = GGadgetGetWindow(g);
	done = GDrawGetUserData(gw);
	list = GWidgetGetControl(gw,CID_Encodings);
	sel = GGadgetGetFirstListSelectedItem(list);
	i=0;
	for ( item=enclist; item!=NULL; item=item->next ) {
	    if ( item->builtin )
		/* Do Nothing */;
	    else if ( i==sel )
	break;
	    else
		++i;
	}
	if ( item!=NULL )
	    DeleteEncoding(item);
	*done = true;
    }
return( true );
}

static int DE_Cancel(GGadget *g, GEvent *e) {
    GWindow gw;
    int *done;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	done = GDrawGetUserData(gw);
	*done = true;
    }
return( true );
}

static int de_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	int *done = GDrawGetUserData(gw);
	*done = true;
    } else if ( event->type == et_char ) {
return( false );
    }
return( true );
}

void RemoveEncoding(void) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[5];
    GTextInfo label[5];
    Encoding *item;
    int done = 0;

    for ( item=enclist; item!=NULL && item->builtin; item=item->next );
    if ( item==NULL )
return;

    memset(&gcd,0,sizeof(gcd));
    memset(&label,0,sizeof(label));
    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict|wam_isdlg;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.is_dlg = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Remove Encoding");
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,150));
    pos.height = GDrawPointsToPixels(NULL,110);
    gw = GDrawCreateTopWindow(NULL,&pos,de_e_h,&done,&wattrs);

    gcd[0].gd.pos.x = 10; gcd[0].gd.pos.y = 6;
    gcd[0].gd.pos.width = 130; gcd[0].gd.pos.height = 5*12+10;
    gcd[0].gd.flags = gg_visible | gg_enabled;
    gcd[0].gd.cid = CID_Encodings;
    gcd[0].gd.u.list = EncodingList();
    gcd[0].gd.handle_controlevent = DE_Delete;
    gcd[0].creator = GListCreate;

    gcd[2].gd.pos.x = -10; gcd[2].gd.pos.y = gcd[0].gd.pos.y+gcd[0].gd.pos.height+5;
    gcd[2].gd.pos.width = -1; gcd[2].gd.pos.height = 0;
    gcd[2].gd.flags = gg_visible | gg_enabled | gg_but_cancel ;
    label[2].text = (unichar_t *) _("_Cancel");
    label[2].text_is_1byte = true;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.mnemonic = 'C';
    gcd[2].gd.handle_controlevent = DE_Cancel;
    gcd[2].creator = GButtonCreate;

    gcd[1].gd.pos.x = 10-3; gcd[1].gd.pos.y = gcd[2].gd.pos.y-3;
    gcd[1].gd.pos.width = -1; gcd[1].gd.pos.height = 0;
    gcd[1].gd.flags = gg_visible | gg_enabled | gg_but_default ;
    label[1].text = (unichar_t *) _("_Delete");
    label[1].text_is_1byte = true;
    label[1].text_in_resource = true;
    gcd[1].gd.mnemonic = 'D';
    gcd[1].gd.label = &label[1];
    gcd[1].gd.handle_controlevent = DE_Delete;
    gcd[1].creator = GButtonCreate;

    gcd[3].gd.pos.x = 2; gcd[3].gd.pos.y = 2;
    gcd[3].gd.pos.width = pos.width-4; gcd[3].gd.pos.height = pos.height-2;
    gcd[3].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[3].creator = GGroupCreate;

    GGadgetsCreate(gw,gcd);
    GTextInfoListFree(gcd[0].gd.u.list);

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
}

Encoding *MakeEncoding(SplineFont *sf,EncMap *map) {
    char *name;
    int i, gid;
    Encoding *item, *temp;
    SplineChar *sc;

    if ( map->enc!=&custom )
return(NULL);

    name = gwwv_ask_string(_("Please name this encoding"),NULL,_("Please name this encoding"));
    if ( name==NULL )
return(NULL);
    item = calloc(1,sizeof(Encoding));
    item->enc_name = name;
    item->only_1byte = item->has_1byte = true;
    item->char_cnt = map->enccount;
    item->unicode = calloc(map->enccount,sizeof(int32));
    for ( i=0; i<map->enccount; ++i ) if ( (gid = map->map[i])!=-1 && (sc=sf->glyphs[gid])!=NULL ) {
	if ( sc->unicodeenc!=-1 )
	    item->unicode[i] = sc->unicodeenc;
	else if ( strcmp(sc->name,".notdef")!=0 ) {
	    if ( item->psnames==NULL )
		item->psnames = calloc(map->enccount,sizeof(unichar_t *));
	    item->psnames[i] = copy(sc->name);
	}
    }
    RemoveMultiples(item);

    if ( enclist == NULL )
	enclist = item;
    else {
	for ( temp=enclist; temp->next!=NULL; temp=temp->next );
	temp->next = item;
    }
    DumpPfaEditEncodings();
return( item );
}

void LoadEncodingFile(void) {
    static char filter[] = "*{.ps,.PS,.txt,.TXT,.enc,.ENC,GlyphOrderAndAliasDB}";
    char *fn;
    char *filename;

    fn = gwwv_open_filename(_("Load Encoding"), NULL, filter, NULL);
    if ( fn==NULL )
return;
    filename = utf82def_copy(fn);
    ParseEncodingFile(filename, NULL);
    free(fn); free(filename);
    DumpPfaEditEncodings();
}

void SFFindNearTop(SplineFont *sf) {
    FontView *fv;
    EncMap *map;
    int i,k,gid;

    if ( sf->cidmaster!=NULL )
	sf = sf->cidmaster;
    if ( sf->subfontcnt==0 ) {
	for ( fv=(FontView *) sf->fv; fv!=NULL; fv=(FontView *) (fv->b.nextsame) ) {
	    map = fv->b.map;
	    fv->sc_near_top = NULL;
	    for ( i=fv->rowoff*fv->colcnt; i<map->enccount && i<(fv->rowoff+fv->rowcnt)*fv->colcnt; ++i )
		if ( (gid=map->map[i])!=-1 && sf->glyphs[gid]!=NULL ) {
		    fv->sc_near_top = sf->glyphs[gid];
	    break;
		}
	}
    } else {
	for ( fv=(FontView *) sf->fv; fv!=NULL; fv=(FontView *) (fv->b.nextsame) ) {
	    map = fv->b.map;
	    fv->sc_near_top = NULL;
	    for ( i=fv->rowoff*fv->colcnt; i<map->enccount && i<(fv->rowoff+fv->rowcnt)*fv->colcnt; ++i ) {
		for ( k=0; k<sf->subfontcnt; ++k )
		    if ( (gid=map->map[i])!=-1 &&
			    gid<sf->subfonts[k]->glyphcnt &&
			    sf->subfonts[k]->glyphs[gid]!=NULL )
			fv->sc_near_top = sf->subfonts[k]->glyphs[gid];
	    }
	}
    }
}

void SFRestoreNearTop(SplineFont *sf) {
    FontView *fv;

    for ( fv=(FontView *) sf->fv; fv!=NULL; fv=(FontView *) (fv->b.nextsame) ) if ( fv->sc_near_top!=NULL ) {
	/* Note: For CID keyed fonts, we don't care if sc is in the currently */
	/*  displayed font, all we care about is the CID */
	int enc = fv->b.map->backmap[fv->sc_near_top->orig_pos];
	if ( enc!=-1 ) {
	    fv->rowoff = enc/fv->colcnt;
	    GScrollBarSetPos(fv->vsb,fv->rowoff);
	    /* Don't ask for an expose event yet. We'll get one soon enough */
	}
    }
}

struct block {
    int cur, tot;
    char **maps;
    char **dirs;
};

static void AddToBlock(struct block *block,char *mapname, char *dir) {
    int i, val, j;
    int len = strlen(mapname);

    if ( mapname[len-7]=='.' ) len -= 7;
    for ( i=0; i<block->cur; ++i ) {
	if ( (val=strncmp(block->maps[i],mapname,len))==0 )
return;		/* Duplicate */
	else if ( val>0 )
    break;
    }
    if ( block->tot==0 ) {
	block->tot = 10;
	block->maps = malloc(10*sizeof(char *));
	block->dirs = malloc(10*sizeof(char *));
    } else if ( block->cur>=block->tot ) {
	block->tot += 10;
	block->maps = realloc(block->maps,block->tot*sizeof(char *));
	block->dirs = realloc(block->dirs,block->tot*sizeof(char *));
    }
    for ( j=block->cur; j>=i; --j ) {
	block->maps[j+1] = block->maps[j];
	block->dirs[j+1] = block->dirs[j];
    }
    block->maps[i] = copyn(mapname,len);
    block->dirs[i] = dir;
    ++block->cur;
}

static void FindMapsInDir(struct block *block,char *dir) {
    struct dirent *ent;
    DIR *d;
    int len;
    char *pt, *pt2;

    if ( dir==NULL )
return;
    /* format of cidmap filename "?*-?*-[0-9]*.cidmap" */
    d = opendir(dir);
    if ( d==NULL )
return;
    while ( (ent = readdir(d))!=NULL ) {
	if ( (len = strlen(ent->d_name))<8 )
    continue;
	if ( strcmp(ent->d_name+len-7,".cidmap")!=0 )
    continue;
	pt = strchr(ent->d_name, '-');
	if ( pt==NULL || pt==ent->d_name )
    continue;
	pt2 = strchr(pt+1, '-' );
	if ( pt2==NULL || pt2==pt+1 || !isdigit(pt2[1]))
    continue;
	AddToBlock(block,ent->d_name,dir);
    }
    closedir(d);
}

static void FindMapsInNoLibsDir(struct block *block,char *dir) {

    if ( dir==NULL || strstr(dir,"/.libs")==NULL )
return;

    dir = copy(dir);
    *strstr(dir,"/.libs") = '\0';

    FindMapsInDir(block,dir);
    free(dir);
}

struct cidmap *AskUserForCIDMap(void) {
    struct block block;
    struct cidmap *map = NULL;
    char buffer[200];
    char **choices;
    int i,ret;
    char *filename=NULL;
    char *reg, *ord, *pt;
    int supplement;

    memset(&block,'\0',sizeof(block));
    for ( map = cidmaps; map!=NULL; map = map->next ) {
	sprintf(buffer,"%s-%s-%d", map->registry, map->ordering, map->supplement);
	AddToBlock(&block,buffer,NULL);
    }
    FindMapsInDir(&block,".");
    FindMapsInDir(&block,GResourceProgramDir);
    FindMapsInNoLibsDir(&block,GResourceProgramDir);
    FindMapsInDir(&block,getFontForgeShareDir());
    FindMapsInDir(&block,"/usr/share/fontforge");

    choices = calloc(block.cur+2,sizeof(unichar_t *));
    choices[0] = copy(_("Browse..."));
    for ( i=0; i<block.cur; ++i )
	choices[i+1] = copy(block.maps[i]);
    ret = gwwv_choose(_("Find a cidmap file..."),(const char **) choices,i+1,0,_("Please select a CID ordering"));
    for ( i=0; i<=block.cur; ++i )
	free( choices[i] );
    free(choices);
    if ( ret==0 ) {
	filename = gwwv_open_filename(_("Find a cidmap file..."),NULL,
		"?*-?*-[0-9]*.cidmap",NULL);
	if ( filename==NULL )
	    ret = -1;
    }
    if ( ret!=-1 ) {
	if ( filename!=NULL )
	    /* Do nothing for now */;
	else if ( block.dirs[ret-1]!=NULL ) {
	    filename = malloc(strlen(block.dirs[ret-1])+strlen(block.maps[ret-1])+3+8);
	    strcpy(filename,block.dirs[ret-1]);
	    strcat(filename,"/");
	    strcat(filename,block.maps[ret-1]);
	    strcat(filename,".cidmap");
	}
	if ( ret!=0 )
	    reg = block.maps[ret-1];
	else {
	    reg = strrchr(filename,'/');
	    if ( reg==NULL ) reg = filename;
	    else ++reg;
	    reg = copy(reg);
	}
	pt = strchr(reg,'-');
	if ( pt==NULL )
	    ret = -1;
	else {
	    *pt = '\0';
	    ord = pt+1;
	    pt = strchr(ord,'-');
	    if ( pt==NULL )
		ret = -1;
	    else {
		*pt = '\0';
		supplement = strtol(pt+1,NULL,10);
	    }
	}
	if ( ret == -1 )
	    /* No map */;
	else if ( filename==NULL )
	    map = FindCidMap(reg,ord,supplement,NULL);
	else {
	    map = LoadMapFromFile(filename,reg,ord,supplement);
	    free(filename);
	}
	if ( ret!=0 && reg!=block.maps[ret-1] )
	    free(reg);
	/*free(filename);*/	/* Freed by loadmap */
    }
    for ( i=0; i<block.cur; ++i )
	free( block.maps[i]);
    free(block.maps);
    free(block.dirs);
return( map );
}

GTextInfo encodingtypes[] = {
    { (unichar_t *) N_("Custom"), NULL, 0, 0, (void *) "Custom", NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Encoding|Glyph Order"), NULL, 0, 0, (void *) "Original", NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { NULL, NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 1, 0, 0, 0, '\0'},	/* Line */
    { (unichar_t *) N_("ISO 8859-1  (Latin1)"), NULL, 0, 0, (void *) "iso8859-1", NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("ISO 8859-15  (Latin0)"), NULL, 0, 0, (void *) "iso8859-15", NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("ISO 8859-2  (Latin2)"), NULL, 0, 0, (void *) "iso8859-2", NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("ISO 8859-3  (Latin3)"), NULL, 0, 0, (void *) "iso8859-3", NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("ISO 8859-4  (Latin4)"), NULL, 0, 0, (void *) "iso8859-4", NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("ISO 8859-9  (Latin5)"), NULL, 0, 0, (void *) "iso8859-9", NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("ISO 8859-10  (Latin6)"), NULL, 0, 0, (void *) "iso8859-10", NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("ISO 8859-13  (Latin7)"), NULL, 0, 0, (void *) "iso8859-13", NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("ISO 8859-14  (Latin8)"), NULL, 0, 0, (void *) "iso8859-14", NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { NULL, NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 1, 0, 0, 0, '\0'},	/* Line */
    { (unichar_t *) N_("ISO 8859-5 (Cyrillic)"), NULL, 0, 0, (void *) "iso8859-5", NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("KOI8-R (Cyrillic)"), NULL, 0, 0, (void *) "koi8-r", NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("ISO 8859-6 (Arabic)"), NULL, 0, 0, (void *) "iso8859-6", NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("ISO 8859-7 (Greek)"), NULL, 0, 0, (void *) "iso8859-7", NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("ISO 8859-8 (Hebrew)"), NULL, 0, 0, (void *) "iso8859-8", NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("ISO 8859-11 (Thai)"), NULL, 0, 0, (void *) "iso8859-11", NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { NULL, NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 1, 0, 0, 0, '\0'},	/* Line */
    { (unichar_t *) N_("Macintosh Latin"), NULL, 0, 0, (void *) "mac", NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Windows Latin (\"ANSI\")"), NULL, 0, 0, (void *) "win", NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Adobe Standard"), NULL, 0, 0, (void *) "AdobeStandard", NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Symbol"), NULL, 0, 0, (void *) "Symbol", NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) NU_("ΤεΧ Base (8r)"), NULL, 0, 0, (void *) "TeX-Base-Encoding", NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { NULL, NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 1, 0, 0, 0, '\0'},	/* Line */
    { (unichar_t *) N_("ISO 10646-1 (Unicode, BMP)"), NULL, 0, 0, (void *) "UnicodeBmp", NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("ISO 10646-1 (Unicode, Full)"), NULL, 0, 0, (void *) "UnicodeFull", NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    /*{ (unichar_t *) N_("ISO 10646-? (by plane) ..."), NULL, 0, 0, (void *) em_unicodeplanes, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' }, */
    { NULL, NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 1, 0, 0, 0, '\0'},	/* Line */
    { (unichar_t *) N_("SJIS (Kanji)"), NULL, 0, 0, (void *) "sjis", NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("JIS 208 (Kanji)"), NULL, 0, 0, (void *) "jis208", NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("JIS 212 (Kanji)"), NULL, 0, 0, (void *) "jis212", NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Wansung (Korean)"), NULL, 0, 0, (void *) "wansung", NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("KSC 5601-1987 (Korean)"), NULL, 0, 0, (void *) "ksc5601", NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Johab (Korean)"), NULL, 0, 0, (void *) "johab", NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("GB 2312 (Simp. Chinese)"), NULL, 0, 0, (void *) "gb2312", NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("EUC GB 2312 (Chinese)"), NULL, 0, 0, (void *) "gb2312pk", NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Big5 (Trad. Chinese)"), NULL, 0, 0, (void *) "big5", NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Big5 HKSCS (Trad. Chinese)"), NULL, 0, 0, (void *) "big5hkscs", NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    GTEXTINFO_EMPTY
};

static void EncodingInit(void) {
    int i;
    static int done = false;

    if ( done )
return;
    done = true;
    for ( i=0; i<sizeof(encodingtypes)/sizeof(encodingtypes[0])-1; ++i ) {
	if ( !encodingtypes[i].line )
	    encodingtypes[i].text = (unichar_t *) S_((char *) encodingtypes[i].text);
    }
}

GMenuItem *GetEncodingMenu(void (*func)(GWindow,GMenuItem *,GEvent *),
	Encoding *current) {
    GMenuItem *mi;
    int i, cnt;
    Encoding *item;

    EncodingInit();

    cnt = 0;
    for ( item=enclist; item!=NULL ; item=item->next )
	if ( !item->hidden )
	    ++cnt;
    i = cnt+1;
    i += sizeof(encodingtypes)/sizeof(encodingtypes[0]);
    mi = calloc(i+1,sizeof(GMenuItem));
    for ( i=0; i<sizeof(encodingtypes)/sizeof(encodingtypes[0])-1; ++i ) {
	mi[i].ti = encodingtypes[i];
	if ( !mi[i].ti.line ) {
	    mi[i].ti.text = utf82u_copy((char *) (mi[i].ti.text));
	    mi[i].ti.checkable = true;
	    if ( strmatch(mi[i].ti.userdata,current->enc_name)==0 ||
		    (current->iconv_name!=NULL && strmatch(mi[i].ti.userdata,current->iconv_name)==0))
		mi[i].ti.checked = true;
	}
	mi[i].ti.text_is_1byte = false;
	mi[i].ti.fg = mi[i].ti.bg = COLOR_DEFAULT;
	mi[i].invoke = func;
    }
    if ( cnt!=0 ) {
	mi[i].ti.fg = mi[i].ti.bg = COLOR_DEFAULT;
	mi[i++].ti.line = true;
	for ( item=enclist; item!=NULL ; item=item->next )
	    if ( !item->hidden ) {
		mi[i].ti.text = utf82u_copy(item->enc_name);
		mi[i].ti.userdata = (void *) item->enc_name;
		mi[i].ti.fg = mi[i].ti.bg = COLOR_DEFAULT;
		mi[i].ti.checkable = true;
		if ( item==current )
		    mi[i].ti.checked = true;
		mi[i++].invoke = func;
	    }
    }
return( mi );
}

GTextInfo *GetEncodingTypes(void) {
    GTextInfo *ti;
    int i, cnt;
    Encoding *item;

    EncodingInit();

    cnt = 0;
    for ( item=enclist; item!=NULL ; item=item->next )
	if ( !item->hidden )
	    ++cnt;
    i = cnt + sizeof(encodingtypes)/sizeof(encodingtypes[0]);
    ti = calloc(i+1,sizeof(GTextInfo));
    memcpy(ti,encodingtypes,sizeof(encodingtypes)-sizeof(encodingtypes[0]));
    for ( i=0; i<sizeof(encodingtypes)/sizeof(encodingtypes[0])-1; ++i ) {
	ti[i].text = (unichar_t *) copy((char *) ti[i].text);
    }
    if ( cnt!=0 ) {
	ti[i++].line = true;
	for ( item=enclist; item!=NULL ; item=item->next )
	    if ( !item->hidden ) {
		ti[i].text = uc_copy(item->enc_name);
		ti[i++].userdata = (void *) item->enc_name;
	    }
    }
return( ti );
}

GTextInfo *EncodingTypesFindEnc(GTextInfo *encodingtypes, Encoding *enc) {
    int i;
    char *name;
    Encoding *new_enc;

    for ( i=0; encodingtypes[i].text!=NULL || encodingtypes[i].line; ++i ) {
	if ( encodingtypes[i].text==NULL )
    continue;
	name = encodingtypes[i].userdata;
	new_enc = FindOrMakeEncoding(name);
	if ( new_enc==NULL )
    continue;
	if ( enc==new_enc )
return( &encodingtypes[i] );
    }
return( NULL );
}

Encoding *ParseEncodingNameFromList(GGadget *listfield) {
    const unichar_t *name = _GGadgetGetTitle(listfield);
    int32 len;
    GTextInfo **ti = GGadgetGetList(listfield,&len);
    int i;
    Encoding *enc = NULL;

    for ( i=0; i<len; ++i ) if ( ti[i]->text!=NULL ) {
	if ( u_strcmp(name,ti[i]->text)==0 ) {
	    enc = FindOrMakeEncoding(ti[i]->userdata);
    break;
	}
    }

    if ( enc == NULL ) {
	char *temp = u2utf8_copy(name);
	enc = FindOrMakeEncoding(temp);
	free(temp);
    }
    if ( enc==NULL )
	ff_post_error(_("Bad Encoding"),_("Bad Encoding"));
return( enc );
}
