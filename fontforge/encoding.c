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
#include "pfaeditui.h"
#include <ustring.h>
#include <utype.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <gfile.h>
#include <gio.h>
#include <gresource.h>

static int enc_num = em_base;

static int32 tex_base_encoding[] = {
    0x0000, 0x02d9, 0xfb01, 0xfb02, 0x2044, 0x02dd, 0x0141, 0x0142,
    0x02db, 0x02da, 0x000a, 0x02d8, 0x2212, 0x000d, 0x017d, 0x017e,
    0x02c7, 0x0131, 0xf6be, 0xfb00, 0xfb03, 0xfb04, 0x2260, 0x221e,
    0x2264, 0x2265, 0x2202, 0x2211, 0x220f, 0x03c0, 0x0060, 0x0027,
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x2019,
    0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
    0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
    0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
    0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
    0x2018, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
    0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
    0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f,
    0x20ac, 0x222b, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021,
    0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0x2126, 0x221a, 0x2248,
    0x0090, 0x0091, 0x0092, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
    0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0x2206, 0x25ca, 0x0178,
    0x0000, 0x00a1, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7,
    0x00a8, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0x002d, 0x00ae, 0x00af,
    0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7,
    0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf,
    0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7,
    0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
    0x00d0, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7,
    0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x00dd, 0x00de, 0x00df,
    0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7,
    0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
    0x00f0, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f7,
    0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x00fd, 0x00fe, 0x00ff
};
static Encoding texbase = { em_base, "TeX-Base-Encoding", 256, tex_base_encoding, NULL, NULL, 1 };
Encoding *enclist = &texbase;

static char *getPfaEditEncodings(void) {
    static char *encfile=NULL;
    char buffer[1025];

    if ( encfile!=NULL )
return( encfile );
    if ( getPfaEditDir(buffer)==NULL )
return( NULL );
    sprintf(buffer,"%s/Encodings.ps", getPfaEditDir(buffer));
    encfile = copy(buffer);
return( encfile );
}

static void EncodingFree(Encoding *item) {
    int i;

    free(item->enc_name);
    if ( item->psnames!=NULL ) for ( i=0; i<item->char_cnt; ++i )
	free(item->psnames[i]);
    free(item->psnames);
    free(item->unicode);
    free(item);
}

/* Parse a TXT file from the unicode consortium */
    /* Unicode Consortium Format A */
    /* List of lines with several fields, */
	/* first is the encoding value (in hex), second the Unicode value (in hex) */
    /* # is a comment character (to eol) */
static Encoding *ParseConsortiumEncodingFile(FILE *file) {
    char buffer[200];
    int32 encs[1024];
    int enc, unienc, max, i;
    Encoding *item;

    for ( i=0; i<1024; ++i )
	encs[i] = 0;
    for ( i=0; i<32; ++i )
	encs[i] = i;
    for ( i=127; i<0xa0; ++i )
	encs[i] = i;
    max = -1;

    while ( fgets(buffer,sizeof(buffer),file)!=NULL ) {
	if ( ishexdigit(buffer[0]) ) {
	    if ( sscanf(buffer, "%x %x", (unsigned *) &enc, (unsigned *) &unienc)==2 &&
		    enc<1024 && enc>=0 ) {
		encs[enc] = unienc;
		if ( enc>max ) max = enc;
	    }
	}
    }

    if ( max==-1 )
return( NULL );

    ++max;
    if ( max<256 ) max = 256;
    item = gcalloc(1,sizeof(Encoding));
    item->char_cnt = max;
    item->unicode = galloc(max*sizeof(int32));
    memcpy(item->unicode,encs,max*sizeof(int32));
return( item );
}

static void DeleteEncoding(Encoding *me);

static void RemoveMultiples(Encoding *item) {
    Encoding *test;

    for ( test=enclist; test!=NULL; test = test->next ) {
	if ( strcmp(test->enc_name,item->enc_name)==0 )
    break;
    }
    if ( test!=NULL )
	DeleteEncoding(test);
}

void ParseEncodingFile(char *filename) {
    FILE *file;
    char *orig = filename;
    Encoding *head, *item, *prev;
    unichar_t *name;
    char buffer[10]; unichar_t ubuf[100];
    int i,ch;

    if ( filename==NULL ) filename = getPfaEditEncodings();
    file = fopen(filename,"r");
    if ( file==NULL ) {
	if ( orig!=NULL )
	    GDrawError("Couldn't open encoding file: %s", orig);
return;
    }
    ch = getc(file);
    ungetc(ch,file);
    if ( ch=='#' || ch=='0' )
	head = ParseConsortiumEncodingFile(file);
    else
	head = PSSlurpEncodings(file);
    fclose(file);
    if ( head==NULL ) {
	GWidgetErrorR(_STR_BadEncFormat,_STR_BadEncFormat );
return;
    }

    for ( i=0, prev=NULL, item=head; item!=NULL; prev = item, item=item->next, ++i ) {
	item->enc_num = ++enc_num;
	if ( item->enc_name==NULL ) {
	    if ( item==head && item->next==NULL )
		u_strcpy(ubuf,GStringGetResource(_STR_PleaseNameEnc,NULL) );
	    else {
		u_strcpy(ubuf,GStringGetResource(_STR_PleaseNameEncPre,NULL) );
		if ( i==1 )
		    u_strcat(ubuf,GStringGetResource(_STR_First,NULL) );
		else if ( i==2 )
		    u_strcat(ubuf,GStringGetResource(_STR_Second,NULL) );
		else if ( i==3 )
		    u_strcat(ubuf,GStringGetResource(_STR_Third,NULL) );
		else {
		    sprintf(buffer,"%d", i );
		    uc_strcat(ubuf,buffer);
		    u_strcat(ubuf,GStringGetResource(_STR_Th,NULL) );
		}
		u_strcat(ubuf,GStringGetResource(_STR_PleaseNameEncPost,NULL) );
	    }
	    name = GWidgetAskString(ubuf,NULL,ubuf);
	    if ( name!=NULL ) {
		item->enc_name = cu_copy(name);
		free(name);
	    } else {
		if ( prev==NULL )
		    head = item->next;
		else
		    prev->next = item->next;
		EncodingFree(item);
		item = prev;
	    }
	}
    }
    for ( item=head; item!=NULL; item=item->next )
	RemoveMultiples(item);

    if ( enclist == NULL )
	enclist = head;
    else {
	for ( item=enclist; item->next!=NULL; item=item->next );
	item->next = head;
    }
}

void LoadPfaEditEncodings(void) {
    ParseEncodingFile(NULL);
}

void DumpPfaEditEncodings(void) {
    FILE *file;
    Encoding *item;
    int i;

    for ( item=enclist; item!=NULL && item->builtin; item=item->next );
    if ( item==NULL ) {
	unlink(getPfaEditEncodings());
return;
    }

    file = fopen( getPfaEditEncodings(), "w");
    if ( file==NULL ) {
	fprintf(stderr, "couldn't write encodings file\n" );
return;
    }

    for ( item=enclist; item!=NULL; item = item->next ) if ( !item->builtin ) {
	fprintf( file, "/%s [\n", item->enc_name );
	for ( i=0; i<item->char_cnt; ++i ) {
	    if ( item->psnames!=NULL && item->psnames[i]!=NULL )
		fprintf( file, " /%s", item->psnames[i]);
	    else if ( item->unicode[i]<' ' || (item->unicode[i]>=0x7f && item->unicode[i]<0xa0))
		fprintf( file, " /.notdef" );
	    else if ( item->unicode[i]<psunicodenames_cnt && psunicodenames[item->unicode[i]]!=NULL )
		fprintf( file, " /%s", psunicodenames[item->unicode[i]]);
	    else if ( item->unicode[i]<0x10000 )
		fprintf( file, " /uni%04X", item->unicode[i]);
	    else
		fprintf( file, " /u%04X", item->unicode[i]);
	    if ( (i&0xf)==0 )
		fprintf( file, "\t\t%% 0x%02x\n", i );
	    else
		putc('\n',file);
	}
	fprintf( file, "] def\n\n" );
    }
    fclose(file);
}

static void DeleteEncoding(Encoding *me) {
    FontView *fv;
    Encoding *prev;
    BDFFont *bdf;

    if ( me->builtin )
return;

    if ( default_encoding == me->enc_num )
	default_encoding = e_iso8859_1;
    for ( fv = fv_list; fv!=NULL; fv = fv->next ) {
	if ( fv->sf->encoding_name==me->enc_num ) {
	    fv->sf->encoding_name = em_none;
	    for ( bdf=fv->sf->bitmaps; bdf!=NULL; bdf=bdf->next )
		bdf->encoding_name = em_none;
	}
    }
    if ( me==enclist )
	enclist = me->next;
    else {
	for ( prev = enclist; prev!=NULL && prev->next!=me; prev=prev->next );
	if ( prev!=NULL ) prev->next = me->next;
    }
    EncodingFree(me);
    DumpPfaEditEncodings();
}

static GTextInfo *EncodingList(void) {
    GTextInfo *ti;
    int i;
    Encoding *item;

    i = 0;
    for ( item=enclist; item!=NULL ; item=item->next )
	if ( !item->builtin )
	    ++i;
    ti = gcalloc(i+1,sizeof(GTextInfo));
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
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = GStringGetResource(_STR_RemoveEncoding,NULL);
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
    label[2].text = (unichar_t *) _STR_Cancel;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.mnemonic = 'C';
    gcd[2].gd.handle_controlevent = DE_Cancel;
    gcd[2].creator = GButtonCreate;

    gcd[1].gd.pos.x = 10-3; gcd[1].gd.pos.y = gcd[2].gd.pos.y-3;
    gcd[1].gd.pos.width = -1; gcd[1].gd.pos.height = 0;
    gcd[1].gd.flags = gg_visible | gg_enabled | gg_but_default ;
    label[1].text = (unichar_t *) _STR_Delete;
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

Encoding *MakeEncoding(SplineFont *sf) {
    unichar_t *name;
    int i;
    Encoding *item, *temp;

    if ( sf->encoding_name!=em_none || sf->charcnt>=1500 )
return(NULL);

    name = GWidgetAskStringR(_STR_PleaseNameEnc,NULL,_STR_PleaseNameEnc);
    if ( name==NULL )
return(NULL);
    item = gcalloc(1,sizeof(Encoding));
    item->enc_num = ++enc_num;
    item->enc_name = cu_copy(name);
    free(name);
    item->char_cnt = sf->charcnt;
    item->unicode = gcalloc(sf->charcnt,sizeof(int32));
    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	if ( sf->chars[i]->unicodeenc!=-1 )
	    item->unicode[i] = sf->chars[i]->unicodeenc;
	else if ( strcmp(sf->chars[i]->name,".notdef")!=0 ) {
	    if ( item->psnames==NULL )
		item->psnames = gcalloc(sf->charcnt,sizeof(unichar_t *));
	    item->psnames[i] = copy(sf->chars[i]->name);
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
    static unichar_t filter[] = { '*','.','{','p','s',',','P','S',',','t','x','t',',','T','X','T',',','e','n','c',',','E','N','C','}',  '\0' };
    unichar_t *fn;
    char *filename;

    fn = GWidgetOpenFile(GStringGetResource(_STR_LoadEncoding,NULL), NULL, filter, NULL,NULL);
    if ( fn==NULL )
return;
    filename = u2def_copy(fn);
    ParseEncodingFile(filename);
    free(fn); free(filename);
    DumpPfaEditEncodings();
}

/* ************************************************************************** */
/* ****************************** CID Encodings ***************************** */
/* ************************************************************************** */
struct cidmap {
    char *registry, *ordering;
    int supplement, maxsupple;
    int cidmax;			/* Max cid found in the charset */
    int namemax;		/* Max cid with useful info */
    uint32 *unicode;
    char **name;
    struct cidmap *next;
};

static struct cidmap *cidmaps = NULL;

int CID2NameEnc(struct cidmap *map,int cid, char *buffer, int len) {
    int enc = -1;

#if defined( _NO_SNPRINTF ) || defined( __VMS )
    if ( map==NULL )
	sprintf(buffer,"cid-%d", cid);
    else if ( cid<map->namemax && map->name[cid]!=NULL )
	strncpy(buffer,map->name[cid],len);
    else if ( cid==0 || (cid<map->namemax && map->unicode[cid]!=0 )) {
	enc = map->unicode[cid];
	if ( enc<psunicodenames_cnt && psunicodenames[enc]!=NULL )
	    strncpy(buffer,psunicodenames[enc],len);
	else if ( enc<0x10000 )
	    sprintf(buffer,"uni%04X", enc);
	else
	    sprintf(buffer,"u%04X", enc);
    } else
	sprintf(buffer,"%s-%d", map->ordering, cid);
#else
    if ( map==NULL )
	snprintf(buffer,len,"cid-%d", cid);
    else if ( cid<map->namemax && map->name[cid]!=NULL )
	strncpy(buffer,map->name[cid],len);
    else if ( cid==0 || (cid<map->namemax && map->unicode[cid]!=0 )) {
	if ( map->unicode==NULL || map->namemax==0 )
	    enc = 0;
	else
	    enc = map->unicode[cid];
	if ( enc<psunicodenames_cnt && psunicodenames[enc]!=NULL )
	    strncpy(buffer,psunicodenames[enc],len);
	else if ( enc<0x10000 )
	    snprintf(buffer,len,"uni%04X", enc);
	else
	    snprintf(buffer,len,"u%04X", enc);
    } else
	snprintf(buffer,len,"%s-%d", map->ordering, cid);
#endif
return( enc );
}

int NameEnc2CID(struct cidmap *map,int enc, char *name) {
    int i;

    if ( map==NULL )
return( -1 );
    if ( enc!=-1 ) {
	for ( i=0; i<map->namemax; ++i )
	    if ( map->unicode[i]==enc )
return( i );
    } else {
	for ( i=0; i<map->namemax; ++i )
	    if ( map->name[i]!=NULL && strcmp(map->name[i],name)==0 )
return( i );
    }
return( -1 );
}

int MaxCID(struct cidmap *map) {
return( map->cidmax );
}

static char *SearchDirForCidMap(char *dir,char *registry,char *ordering,
	int supplement,char **maybefile) {
    char maybe[FILENAME_MAX+1];
    struct dirent *ent;
    DIR *d;
    int len, rlen = strlen(registry), olen=strlen(ordering);
    char *pt, *end, *ret;
    int test, best = -1;

    if ( dir==NULL )
return( NULL );

    if ( *maybefile!=NULL ) {
	char *pt = strrchr(*maybefile,'.');
	while ( pt>*maybefile && isdigit(pt[-1]))
	    --pt;
	best = strtol(pt,NULL,10);
    }

    d = opendir(dir);
    if ( d==NULL )
return( NULL );
    while ( (ent = readdir(d))!=NULL ) {
	if ( (len = strlen(ent->d_name))<8 )
    continue;
	if ( strcmp(ent->d_name+len-7,".cidmap")!=0 )
    continue;
	if ( strncmp(ent->d_name,registry,rlen)!=0 || ent->d_name[rlen]!='-' )
    continue;
	pt = ent->d_name+rlen+1;
	if ( strncmp(pt,ordering,olen)!=0 || pt[olen]!='-' )
    continue;
	pt += olen+1;
	if ( !isdigit(*pt))
    continue;
	test = strtol(pt,&end,10);
	if ( *end!='.' )
    continue;
	if ( test>=supplement ) {
	    ret = galloc(strlen(dir)+1+len+1);
	    strcpy(ret,dir);
	    strcat(ret,"/");
	    strcat(ret,ent->d_name);
	    closedir(d);
return( ret );
	} else if ( test>best ) {
	    best = test;
	    strcpy(maybe,ent->d_name);
	}
    }
    closedir(d);
    if ( best>-1 ) {
	ret = galloc(strlen(dir)+1+strlen(maybe)+1);
	strcpy(ret,dir);
	strcat(ret,"/");
	strcat(ret,maybe);
	*maybefile = ret;
    }
return( NULL );
}

static char *SearchNoLibsDirForCidMap(char *dir,char *registry,char *ordering,
	int supplement,char **maybefile) {
    char *ret;

    if ( dir==NULL || strstr(dir,"/.libs")==NULL )
return( NULL );

    dir = copy(dir);
    *strstr(dir,"/.libs") = '\0';

    ret = SearchDirForCidMap(dir,registry,ordering,supplement,maybefile);
    free(dir);
return( ret );
}

static struct cidmap *MakeDummyMap(char *registry,char *ordering,int supplement) {
    struct cidmap *ret = galloc(sizeof(struct cidmap));

    ret->registry = copy(registry);
    ret->ordering = copy(ordering);
    ret->supplement = ret->maxsupple = supplement;
    ret->cidmax = ret->namemax = 0;
    ret->unicode = NULL; ret->name = NULL;
    ret->next = cidmaps;
    cidmaps = ret;
return( ret );
}

static struct cidmap *LoadMapFromFile(char *file,char *registry,char *ordering,
	int supplement) {
    struct cidmap *ret = galloc(sizeof(struct cidmap));
    char *pt = strrchr(file,'.');
    FILE *f;
    int cid1, cid2, uni, cnt, i;
    char name[100];

    while ( pt>file && isdigit(pt[-1]))
	--pt;
    ret->supplement = ret->maxsupple = strtol(pt,NULL,10);
    if ( supplement>ret->maxsupple )
	ret->maxsupple = supplement;
    ret->registry = copy(registry);
    ret->ordering = copy(ordering);
    ret->next = cidmaps;
    cidmaps = ret;

    f = fopen( file,"r" );
    if ( f==NULL ) {
	GDrawError("Couldn't open %s", file );
	ret->cidmax = ret->namemax = 0;
	ret->unicode = NULL; ret->name = NULL;
    } else if ( fscanf( f, "%d %d", &ret->cidmax, &ret->namemax )!=2 ) {
	GDrawError( "%s is not a cidmap file, please download\nhttp://fontforge.sourceforge.net/cidmaps.tgz", file );
	fprintf( stderr, "%s is not a cidmap file, please download\nhttp://fontforge.sourceforge.net/cidmaps.tgz", file );
	ret->cidmax = ret->namemax = 0;
	ret->unicode = NULL; ret->name = NULL;
    } else {
	ret->unicode = gcalloc(ret->namemax+1,sizeof(uint32));
	ret->name = gcalloc(ret->namemax+1,sizeof(char *));
	while ( 1 ) {
	    cnt=fscanf( f, "%d..%d %x", &cid1, &cid2, (unsigned *) &uni );
	    if ( cnt<=0 )
	break;
	    if ( cid1>ret->namemax )
	continue;
	    if ( cnt==3 ) {
		if ( cid2>ret->namemax ) cid2 = ret->namemax;
		for ( i=cid1; i<=cid2; ++i )
		    ret->unicode[i] = uni++;
	    } else if ( cnt==1 ) {
		if ( fscanf(f,"%x", (unsigned *) &uni )==1 )
		    ret->unicode[cid1] = uni;
		else if ( fscanf(f," /%s", name )==1 )
		    ret->name[cid1] = copy(name);
	    }
	}
	fclose(f);
    }
    free(file);
return( ret );
}

struct cidmap *FindCidMap(char *registry,char *ordering,int supplement,SplineFont *sf) {
    struct cidmap *map, *maybe=NULL;
    char *file, *maybefile=NULL;
    int maybe_sup = -1;
    static int buts[] = { _STR_UseIt, _STR_Search, 0 };
    static int buts2[] = { _STR_UseIt, _STR_GiveUp, 0 };
    static int buts3[] = { _STR_Browse, _STR_GiveUp, 0 };
    unichar_t ubuf[100]; char buf[100];
    int ret;

    if ( sf!=NULL && sf->loading_cid_map )
return( NULL );

    for ( map = cidmaps; map!=NULL; map = map->next ) {
	if ( strcmp(map->registry,registry)==0 && strcmp(map->ordering,ordering)==0 ) {
	    if ( supplement<=map->supplement )
return( map );
	    else if ( maybe==NULL || maybe->supplement<map->supplement )
		maybe = map;
	}
    }
    if ( maybe!=NULL && supplement<=maybe->maxsupple )
return( maybe );	/* User has said it's ok to use maybe at this supplement level */

    file = SearchDirForCidMap(".",registry,ordering,supplement,&maybefile);
    if ( file==NULL )
	file = SearchDirForCidMap(GResourceProgramDir,registry,ordering,supplement,&maybefile);
    if ( file==NULL )
	file = SearchNoLibsDirForCidMap(GResourceProgramDir,registry,ordering,supplement,&maybefile);
#ifdef SHAREDIR
    if ( file==NULL )
	file = SearchDirForCidMap(SHAREDIR,registry,ordering,supplement,&maybefile);
#endif
    if ( file==NULL )
	file = SearchDirForCidMap(getPfaEditShareDir(),registry,ordering,supplement,&maybefile);
    if ( file==NULL )
	file = SearchDirForCidMap("/usr/share/fontforge",registry,ordering,supplement,&maybefile);

    if ( file==NULL && (maybe!=NULL || maybefile!=NULL)) {
	if ( maybefile!=NULL ) {
	    char *pt = strrchr(maybefile,'.');
	    while ( pt>maybefile && isdigit(pt[-1]))
		--pt;
	    maybe_sup = strtol(pt,NULL,10);
	    if ( maybe!=NULL && maybe->supplement >= maybe_sup ) {
		free(maybefile); maybefile = NULL;
		maybe_sup = maybe->supplement;
	    } else
		maybe = NULL;
	}
	if ( maybe!=NULL )
	    maybe_sup = maybe->supplement;
	if ( sf!=NULL ) sf->loading_cid_map = true;
	ret = GWidgetAskR(_STR_UseCidMap,buts,0,1,_STR_SearchForCIDMap,
		registry,ordering,supplement,maybe_sup);
	if ( sf!=NULL ) sf->loading_cid_map = false;
	if ( ret==0 ) {
	    if ( maybe!=NULL ) {
		maybe->maxsupple = supplement;
return( maybe );
	    } else {
		file = maybefile;
		maybefile = NULL;
	    }
	}
    }

    if ( file==NULL ) {
	unichar_t *uret;
#if defined( _NO_SNPRINTF ) || defined( __VMS )
	sprintf(buf,"%s-%s-*.cidmap", registry, ordering );
#else
	snprintf(buf,sizeof(buf),"%s-%s-*.cidmap", registry, ordering );
#endif
	uc_strcpy(ubuf,buf);
	if ( maybe==NULL && maybefile==NULL ) {
	    ret = GWidgetAskR(_STR_NoCidmap,buts3,0,1,_STR_LookForCidmap,
#ifdef SHAREDIR
		    SHAREDIR
#else
		    getPfaEditShareDir()==NULL?"/usr/share/fontforge":getPfaEditShareDir()
#endif
		    );
	    if ( ret==1 || screen_display==NULL )
		ubuf[0] = '\0';
	}
	if ( ubuf[0]=='\0' )
	    uret = NULL;
	else if ( screen_display!=NULL ) {
	    if ( sf!=NULL ) sf->loading_cid_map = true;
	    uret = GWidgetOpenFile(GStringGetResource(_STR_FindCharset,NULL),NULL,ubuf,NULL,NULL);
	    if ( sf!=NULL ) sf->loading_cid_map = false;
	}
	if ( uret==NULL ) {
	    if ( maybe==NULL && maybefile==NULL )
		/* No luck */;
	    else if ( screen_display==NULL && maybe!=NULL ) {
		maybe->maxsupple = supplement;
return( maybe );
	    } else if ( screen_display==NULL ) {
		file = maybefile;
		maybefile = NULL;
	    } else if ( GWidgetAskR(_STR_UseCidMap,buts2,0,1,_STR_AreYouSureCharset)==0 ) {
		if ( maybe!=NULL ) {
		    maybe->maxsupple = supplement;
return( maybe );
		} else {
		    file = maybefile;
		    maybefile = NULL;
		}
	    }
	} else {
	    file = u2def_copy(uret);
	    free(uret);
	}
    }

    free(maybefile);
    if ( file!=NULL )
return( LoadMapFromFile(file,registry,ordering,supplement));

return( MakeDummyMap(registry,ordering,supplement));
}

static void SFApplyEnc(SplineFont *sf, int charcnt) {
    SplineChar **chars, *sc;
    int i;
    RefChar *refs, *rnext, *rprev;
    SplineSet *new, *spl;

    /* Remove references to characters which aren't in the new map (if any) */
    /* Don't need to fix up dependencies, because we throw the char away */
    for ( i=0; i<sf->charcnt; ++i ) if ( (sc = sf->chars[i])!=NULL ) {
	for ( rprev = NULL, refs=sc->layers[ly_fore].refs; refs!=NULL; refs=rnext ) {
	    rnext = refs->next;
	    if ( refs->sc->enc==-1 ) {
		new = refs->layers[0].splines;
		if ( new!=NULL ) {
		    for ( spl = new; spl->next!=NULL; spl = spl->next );
		    spl->next = sc->layers[ly_fore].splines;
		    sc->layers[ly_fore].splines = new;
		}
		refs->layers[0].splines=NULL;
		RefCharFree(refs);
		if ( rprev==NULL )
		    sc->layers[ly_fore].refs = rnext;
		else
		    rprev->next = rnext;
	    } else
		rprev = refs;
	}
    }

    chars = gcalloc(charcnt+1,sizeof(SplineChar *));
    for ( i=0; i<sf->charcnt; ++i ) if ( (sc = sf->chars[i])!=NULL ) {
	if ( sc->enc==-1 )
	    SplineCharFree(sc);
	else
	    chars[sc->enc] = sc;
    }

    free(sf->chars);
    sf->charcnt = charcnt;
    sf->chars = chars;
}

void SFEncodeToMap(SplineFont *sf,struct cidmap *map) {
    SplineChar *sc;
    int i,max=0, anyextras=0;

    for ( i=0; i<sf->charcnt; ++i ) if ( SCWorthOutputting(sc = sf->chars[i]) ) {
	sc->enc = NameEnc2CID(map,sc->unicodeenc,sc->name);
	if ( sc->enc>max ) max = sc->enc;
	else if ( sc->enc==-1 ) ++anyextras;
    } else if ( sc!=NULL )
	sc->enc = -1;

    if ( anyextras ) {
	static int buttons[] = { _STR_Delete, _STR_Add, 0 };
	if ( GWidgetAskR(_STR_ExtraCharsTitle,buttons,0,1,_STR_ExtraChars)==1 ) {
	    if ( map!=NULL && max<map->cidmax ) max = map->cidmax;
	    anyextras = 0;
	    for ( i=0; i<sf->charcnt; ++i ) if ( SCWorthOutputting(sc = sf->chars[i]) ) {
		if ( sc->enc == -1 ) sc->enc = max + anyextras++;
	    }
	    max += anyextras;
	}
    }
    SFApplyEnc(sf, max+1);
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
	block->maps = galloc(10*sizeof(char *));
	block->dirs = galloc(10*sizeof(char *));
    } else if ( block->cur>=block->tot ) {
	block->tot += 10;
	block->maps = grealloc(block->maps,block->tot*sizeof(char *));
	block->dirs = grealloc(block->dirs,block->tot*sizeof(char *));
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
}

static void FindMapsInNoLibsDir(struct block *block,char *dir) {

    if ( dir==NULL || strstr(dir,"/.libs")==NULL )
return;

    dir = copy(dir);
    *strstr(dir,"/.libs") = '\0';

    FindMapsInDir(block,dir);
    free(dir);
}

struct cidmap *AskUserForCIDMap(SplineFont *sf) {
    struct block block;
    struct cidmap *map = NULL;
    char buffer[200];
    const unichar_t **choices;
    int i,ret;
    static unichar_t cidwild[] = { '?','*','-','?','*','-','[','0','-','9',']','*','.','c','i','d','m','a','p',  '\0' };
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
#ifdef SHAREDIR
    FindMapsInDir(&block,SHAREDIR);
#endif
    FindMapsInDir(&block,getPfaEditShareDir());
    FindMapsInDir(&block,"/usr/share/fontforge");

    choices = gcalloc(block.cur+2,sizeof(unichar_t *));
    choices[0] = u_copy(GStringGetResource(_STR_Browse,NULL));
    for ( i=0; i<block.cur; ++i )
	choices[i+1] = uc_copy(block.maps[i]);
    ret = GWidgetChoicesR(_STR_FindCharset,choices,i+1,0,_STR_SelectCIDOrdering);
    for ( i=0; i<=block.cur; ++i )
	free( (unichar_t *) choices[i] );
    free(choices);
    if ( ret==0 ) {
	unichar_t *uret = GWidgetOpenFile(GStringGetResource(_STR_FindCharset,NULL),NULL,cidwild,NULL,NULL);
	if ( uret==NULL )
	    ret = -1;
	else {
	    filename = u2def_copy(uret);
	    free(uret);
	}
    }
    if ( ret!=-1 ) {
	if ( filename!=NULL )
	    /* Do nothing for now */;
	else if ( block.dirs[ret-1]!=NULL ) {
	    filename = galloc(strlen(block.dirs[ret-1])+strlen(block.maps[ret-1])+3+8);
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
	    map = FindCidMap(reg,ord,supplement,sf);
	else
	    map = LoadMapFromFile(filename,reg,ord,supplement);
	if ( ret!=0 && reg!=block.maps[ret-1] )
	    free(reg);
	/*free(filename);*/	/* Freed by loadmap */
    }
    for ( i=0; i<block.cur; ++i )
	free( block.maps[i]);
    free(block.maps);
    free(block.dirs);
    if ( map!=NULL ) {
	sf->cidregistry = copy(map->registry);
	sf->ordering = copy(map->ordering);
	sf->supplement = map->supplement;
    }
return( map );
}

static enum fchooserret CMapFilter(GGadget *g,GDirEntry *ent,
	const unichar_t *dir) {
    enum fchooserret ret = GFileChooserDefFilter(g,ent,dir);
    char buf2[256];
    FILE *file;
    static char *cmapflag = "%!PS-Adobe-3.0 Resource-CMap";

    if ( ret==fc_show && !ent->isdir ) {
	int len = 3*(u_strlen(dir)+u_strlen(ent->name)+5);
	char *filename = galloc(len);
	u2def_strncpy(filename,dir,len);
	strcat(filename,"/");
	u2def_strncpy(buf2,ent->name,sizeof(buf2));
	strcat(filename,buf2);
	file = fopen(filename,"r");
	if ( file==NULL )
	    ret = fc_hide;
	else {
	    if ( fgets(buf2,sizeof(buf2),file)==NULL ||
		    strncmp(buf2,cmapflag,strlen(cmapflag))!=0 )
		ret = fc_hide;
	    fclose(file);
	}
	free(filename);
    }
return( ret );
}

enum cmaptype { cmt_out=-1, cmt_coderange, cmt_notdefs, cmt_cid, cmt_max };
struct cmap {
    struct {
	int n;
	struct coderange { uint32 first, last, cid; } *ranges;
    } groups[cmt_max];
    char *registry;
    char *ordering;
    int supplement;
    struct remap *remap;
    int total;
};

static void cmapfree(struct cmap *cmap) {
    free(cmap->registry);
    free(cmap->ordering);
    free(cmap->groups[cmt_coderange].ranges);
    free(cmap->groups[cmt_notdefs].ranges);
    free(cmap->groups[cmt_cid].ranges);
    free(cmap->remap);
    free(cmap);
}

static struct coderange *ExtendArray(struct coderange *ranges,int *n, int val) {
    if ( *n == 0 )
	ranges = gcalloc(val,sizeof(struct coderange));
    else {
	ranges = grealloc(ranges,(*n+val)*sizeof(struct coderange));
	memset(ranges+*n,0,val*sizeof(struct coderange));
    }
    *n += val;
return( ranges );
}

static char *readpsstr(char *str) {
    char *eos;

    while ( isspace(*str)) ++str;
    if ( *str=='(' ) ++str;
    /* Postscript strings can be more complicated than this (hex, nested parens, Enc85...) */
    /*  but none of those should show up here */
    for ( eos = str; *eos!=')' && *eos!='\0'; ++eos );
return( copyn(str,eos-str));
}
    
static struct cmap *ParseCMap(char *filename) {
    char buf2[200];
    FILE *file;
    struct cmap *cmap;
    char *end, *pt;
    int val, pos;
    enum cmaptype in;
    static const char *bcsr = "begincodespacerange", *bndr = "beginnotdefrange", *bcr = "begincidrange";
    static const char *reg = "/Registry", *ord = "/Ordering", *sup="/Supplement";

    file = fopen(filename,"r");
    if ( file==NULL )
return( NULL );

    cmap = gcalloc(1,sizeof(struct cmap));
    in = cmt_out;
    while ( fgets(buf2,sizeof(buf2),file)!=NULL ) {
	for ( pt=buf2; isspace(*pt); ++pt);
	if ( in==cmt_out ) {
	    if ( *pt=='/' ) {
		if ( strncmp(pt,reg,strlen(reg))==0 )
		    cmap->registry = readpsstr(pt+strlen(reg));
		else if ( strncmp(pt,ord,strlen(ord))==0 )
		    cmap->ordering = readpsstr(pt+strlen(ord));
		else if ( strncmp(pt,ord,strlen(ord))==0 ) {
		    for ( pt += strlen(sup); isspace(*pt); ++pt );
		    cmap->supplement = strtol(pt,NULL,10);
		}
    continue;
	    } else if ( !isdigit(*pt) )
    continue;
	    val = strtol(pt,&end,10);
	    while ( isspace(*end)) ++end;
	    if ( strncmp(end,bcsr,strlen(bcsr))==0 )
		in = cmt_coderange;
	    else if ( strncmp(end,bndr,strlen(bndr))==0 )
		in = cmt_notdefs;
	    else if ( strncmp(end,bcr,strlen(bcr))==0 )
		in = cmt_cid;
	    if ( in!=cmt_out ) {
		pos = cmap->groups[in].n;
		cmap->groups[in].ranges = ExtendArray(cmap->groups[in].ranges,&cmap->groups[in].n,val);
	    }
	} else if ( strncmp(pt,"end",3)== 0 )
	    in = cmt_out;
	else {
	    if ( *pt!='<' )
	continue;
	    cmap->groups[in].ranges[pos].first = strtoul(pt+1,&end,16);
	    if ( *end=='>' ) ++end;
	    while ( isspace(*end)) ++end;
	    if ( *end=='<' ) ++end;
	    cmap->groups[in].ranges[pos].last = strtoul(end,&end,16);
	    if ( in!=cmt_coderange ) {
		if ( *end=='>' ) ++end;
		while ( isspace(*end)) ++end;
		cmap->groups[in].ranges[pos].cid = strtol(end,&end,10);
	    }
	    ++pos;
	}
    }
    fclose(file);
return( cmap );
}

static void CompressCMap(struct cmap *cmap) {
    int32 i,j,k, pos, base;
    uint32 min, oldmax;
    /* we can't really deal with three and four byte encodings */
    /*  so if we get one arrange for the sf itself to do a remap */

    cmap->total = 0x10000;
    for ( i=0; i<cmap->groups[cmt_coderange].n; ++i )
	if ( cmap->groups[cmt_coderange].ranges[i].last>0xfffff )
    break;
    if ( i==cmap->groups[cmt_coderange].n )	/* No need to remap */
return;

    cmap->remap = gcalloc(cmap->groups[cmt_coderange].n+1,sizeof(struct remap));
    base = 0;
    for ( i=0; i<cmap->groups[cmt_coderange].n; ++i )
	if ( cmap->groups[cmt_coderange].ranges[i].last<0xffff ) {
	    base = 0x10000;
    break;
	}

    pos=0;
    oldmax = base==0?0:0xffff;
    for ( i=0; i<cmap->groups[cmt_coderange].n; ++i ) {
	min = 0xffffffff; k=-1;
	for ( j=0; j<cmap->groups[cmt_coderange].n; ++j )
	    if ( cmap->groups[cmt_coderange].ranges[j].first>oldmax &&
		    cmap->groups[cmt_coderange].ranges[j].first<min ) {
		min = cmap->groups[cmt_coderange].ranges[j].first;
		k = j;
	    }
	if ( k==-1 )
    break;
	cmap->remap[pos].firstenc = cmap->groups[cmt_coderange].ranges[k].first&~0xff;
	cmap->remap[pos].lastenc = cmap->groups[cmt_coderange].ranges[k].last|0xff;
	cmap->remap[pos].infont = base;
	base += cmap->remap[pos].lastenc-cmap->remap[pos].firstenc+1;
	oldmax = cmap->remap[pos].lastenc;
	++pos;
    }
    cmap->remap[pos].infont = -1;	/* Marks end */
    cmap->total = base;
    /* so cmap->remap will map sf indeces into the encoding in the cmap */

    /* And now we want to change the groups[cmt_cid].ranges so that they will */
    /*  map into sf indeces rather than into the encoding of the cmap */
    for ( i=0; i<cmap->groups[cmt_cid].n; ++i ) {
	for ( k=0; cmap->remap[k].infont!=-1; ++k )
	    if ( cmap->groups[cmt_cid].ranges[i].first>=cmap->remap[k].firstenc &&
		    cmap->groups[cmt_cid].ranges[i].first<=cmap->remap[k].lastenc )
	break;
	if ( cmap->remap[k].infont==-1 )
    continue;
	cmap->groups[cmt_cid].ranges[i].first += cmap->remap[k].infont-cmap->remap[k].firstenc;
	cmap->groups[cmt_cid].ranges[i].last += cmap->remap[k].infont-cmap->remap[k].firstenc;
    }
}

SplineFont *CIDFlatten(SplineFont *cidmaster,SplineChar **chars,int charcnt) {
    FontView *fvs;
    SplineFont *new;
    char buffer[20];
    BDFFont *bdf;
    int j;

    if ( cidmaster==NULL )
return(NULL);
    new = SplineFontEmpty();
    new->fontname = copy(cidmaster->fontname);
    new->fullname = copy(cidmaster->fullname);
    new->familyname = copy(cidmaster->familyname);
    new->weight = copy(cidmaster->weight);
    new->copyright = copy(cidmaster->copyright);
    sprintf(buffer,"%g", cidmaster->cidversion);
    new->version = copy(buffer);
    new->italicangle = cidmaster->italicangle;
    new->upos = cidmaster->upos;
    new->uwidth = cidmaster->uwidth;
    new->ascent = cidmaster->ascent;
    new->descent = cidmaster->descent;
    new->changed = new->changed_since_autosave = true;
    new->display_antialias = cidmaster->display_antialias;
    new->fv = cidmaster->fv;
    new->encoding_name = em_none;
    /* Don't copy the grid splines, there won't be anything meaningfull at top level */
    /*  and won't know which font to copy from below */
    new->bitmaps = cidmaster->bitmaps;		/* should already be flattened */
    cidmaster->bitmaps = NULL;			/* don't free 'em */
    for ( bdf=new->bitmaps; bdf!=NULL; bdf=bdf->next )
	bdf->sf = new;
    new->script_lang = cidmaster->script_lang;
    cidmaster->script_lang = NULL;
    new->sli_cnt = cidmaster->sli_cnt;
    new->kerns = cidmaster->kerns; new->vkerns = cidmaster->vkerns;
    cidmaster->kerns = cidmaster->vkerns = NULL;
    new->texdata = cidmaster->texdata;
    new->gentags = cidmaster->gentags;
    memset(&cidmaster->gentags,0,sizeof(cidmaster->gentags));
    new->possub = cidmaster->possub; cidmaster->possub = NULL;
    new->sm = cidmaster->sm; cidmaster->sm = NULL;
    new->features = cidmaster->features; cidmaster->features = NULL;
    new->macstyle = cidmaster->macstyle;
    new->origname = copy( cidmaster->origname );
    new->display_size = cidmaster->display_size;
    /* Don't copy private */
    new->xuid = copy(cidmaster->xuid);
    new->chars = chars;
    new->charcnt = charcnt;
    for ( j=0; j<charcnt; ++j ) if ( chars[j]!=NULL ) {
	chars[j]->parent = new;
	chars[j]->enc = j;
    }
    for ( fvs=new->fv; fvs!=NULL; fvs=fvs->nextsame ) {
	fvs->cidmaster = NULL;
	if ( fvs->sf->charcnt!=new->charcnt ) {
	    free(fvs->selected);
	    fvs->selected = gcalloc(new->charcnt,sizeof(char));
	}
	fvs->sf = new;
	FVSetTitle(fvs);
    }
    FontViewReformatAll(new);
    SplineFontFree(cidmaster);
return( new );
}

void SFFlatten(SplineFont *cidmaster) {
    SplineChar **chars;
    int i,j,max;

    if ( cidmaster==NULL )
return;
    if ( cidmaster->cidmaster!=NULL )
	cidmaster = cidmaster->cidmaster;
    /* This doesn't change the ordering, so no need for special tricks to */
    /*  preserve scrolling location. */
    for ( i=max=0; i<cidmaster->subfontcnt; ++i ) {
	if (cidmaster->subfonts[i]->compacted )
	    SFUncompactFont(cidmaster->subfonts[i]);
	if ( max<cidmaster->subfonts[i]->charcnt )
	    max = cidmaster->subfonts[i]->charcnt;
    }
    chars = gcalloc(max,sizeof(SplineChar *));
    for ( j=0; j<max; ++j ) {
	for ( i=0; i<cidmaster->subfontcnt; ++i ) {
	    if ( j<cidmaster->subfonts[i]->charcnt && cidmaster->subfonts[i]->chars[j]!=NULL ) {
		chars[j] = cidmaster->subfonts[i]->chars[j];
		cidmaster->subfonts[i]->chars[j] = NULL;
	break;
	    }
	}
    }
    CIDFlatten(cidmaster,chars,max);
}

int SFFlattenByCMap(SplineFont *sf,char *cmapname) {
    struct cmap *cmap;
    int i,j,k,l,m, extras, max, curmax, warned;
    int found[4];
    SplineChar **chars = NULL, *sc;

    if ( sf->cidmaster!=NULL )
	sf = sf->cidmaster;
    if ( sf->subfontcnt==0 ) {
	GWidgetErrorR(_STR_NotACIDFont,_STR_NotACIDFont);
return(false);
    }
    if ( cmapname==NULL ) {
	unichar_t *uret = GWidgetOpenFile(GStringGetResource(_STR_FindCMap,NULL),NULL,NULL,NULL,CMapFilter);
	cmapname = u2def_copy(uret);
	free(uret);
    }
    if ( cmapname==NULL )
return(true);
    cmap = ParseCMap(cmapname);
    if ( cmap==NULL )
return(false);
    CompressCMap(cmap);
    max = 0;
    for ( i=0; i<cmap->groups[cmt_cid].n; ++i ) {
	if ( max<cmap->groups[cmt_cid].ranges[i].last )
	    max = cmap->groups[cmt_cid].ranges[i].last;
	if ( cmap->groups[cmt_cid].ranges[i].last>0x100000 ) {
	    GWidgetErrorR(_STR_EncodingTooLarge,_STR_EncodingTooLarge);
	    cmapfree(cmap);
return(false);
	}
    }

    SFFindNearTop(sf);
    curmax = 0;
    for ( k=0; k<sf->subfontcnt; ++k ) {
	if (sf->subfonts[k]->compacted )
	    SFUncompactFont(sf->subfonts[k]);
	if ( curmax < sf->subfonts[k]->charcnt )
	    curmax = sf->subfonts[k]->charcnt;
    }

    chars = NULL;
    warned = false;
    for ( j=0; j<2; ++j ) {
	extras = 0;
	for ( i=0; i<curmax; ++i ) {
	    sc = NULL;
	    for ( k=0; k<sf->subfontcnt; ++k )
		if ( i<sf->subfonts[k]->charcnt && sf->subfonts[k]->chars[i]!=NULL ) {
		    sc = sf->subfonts[k]->chars[i];
		    if ( chars!=NULL )
			sf->subfonts[k]->chars[i] = NULL;
	    break;
		}
	    if ( sc!=NULL ) {
		m = 0;
		for ( l=0; l<cmap->groups[cmt_cid].n; ++l ) {
		    if ( i>=cmap->groups[cmt_cid].ranges[l].cid &&
			    i<=cmap->groups[cmt_cid].ranges[l].cid +
			       cmap->groups[cmt_cid].ranges[l].last -
				cmap->groups[cmt_cid].ranges[l].first ) {
			if ( m<sizeof(found)/sizeof(found[0]) )
			    found[m++] = l;
			else if ( !warned ) {
			    GWidgetPostNoticeR(_STR_MultipleEncodingIgnored,
				    _STR_CIDGlyphMultEncoded, i,
			            sizeof(found)/sizeof(found[0]),
			            sizeof(found)/sizeof(found[0]));
			    warned = true;
			}
		    }
		}
		if ( m==0 ) {
		    if ( chars!=NULL )
			chars[max+extras] = sc;
		    ++extras;
		} else {
		    if ( chars!=NULL ) {
			int p = cmap->groups[cmt_cid].ranges[found[0]].first +
				i-cmap->groups[cmt_cid].ranges[found[0]].cid;
			chars[p] = sc;
			sc->enc = p;
			for ( l=1; l<m; ++l ) {
			    int pos = cmap->groups[cmt_cid].ranges[found[l]].first +
				    i-cmap->groups[cmt_cid].ranges[found[l]].cid;
			    chars[pos] = MakeDupRef(sc,pos,-1);
			}
		    }
		}
	    }
	}
	if ( chars==NULL )
	    chars = gcalloc(max+extras,sizeof(SplineChar *));
	warned = true;
    }
    sf = CIDFlatten(sf,chars,max+extras);
    sf->remap = cmap->remap; cmap->remap = NULL;
    cmapfree(cmap);
    SFRestoreNearTop(sf);
return( true );
}

static int Enc2CMap(struct cmap *cmap,int enc) {
    int i;

    for ( i=0; i<cmap->groups[cmt_cid].n; ++i )
	if ( enc>=cmap->groups[cmt_cid].ranges[i].first &&
		enc<=cmap->groups[cmt_cid].ranges[i].last )
return( enc-cmap->groups[cmt_cid].ranges[i].first+
	    cmap->groups[cmt_cid].ranges[i].cid );

return( -1 );
}

static void SFEncodeToCMap(SplineFont *cidmaster,SplineFont *sf,struct cmap *cmap) {
    SplineChar *sc;
    int i,max=0, anyextras=0;

    cidmaster->cidregistry = cmap->registry; cmap->registry = NULL;
    cidmaster->ordering = cmap->ordering; cmap->ordering = NULL;
    cidmaster->supplement = cmap->supplement;

    for ( i=0; i<sf->charcnt; ++i ) if ( (sc = sf->chars[i])!=NULL ) {
	sc->enc = Enc2CMap(cmap,sc->enc);
	if ( sc->enc>max ) max = sc->enc;
	else if ( sc->enc==-1 ) ++anyextras;
    }

    if ( anyextras ) {
	static int buttons[] = { _STR_Delete, _STR_Add, 0 };
	if ( GWidgetAskR(_STR_ExtraCharsTitle,buttons,0,1,_STR_ExtraChars)==1 ) {
	    if ( cmap!=NULL && max<cmap->total ) max = cmap->total;
	    anyextras = 0;
	    for ( i=0; i<sf->charcnt; ++i ) if ( (sc = sf->chars[i])!=NULL ) {
		if ( sc->enc == -1 ) sc->enc = max + anyextras++;
	    }
	    max += anyextras;
	}
    }
    SFApplyEnc(sf, max);
}

SplineFont *MakeCIDMaster(SplineFont *sf,int bycmap,char *cmapfilename, struct cidmap *cidmap) {
    SplineFont *cidmaster;
    struct cidmap *map;
    struct cmap *cmap;
    FontView *fvs;
    int freeme;

    cidmaster = SplineFontEmpty();
    SFFindNearTop(sf);
    if ( bycmap ) {
	freeme = false;
	if ( cmapfilename==NULL ) {
	    unichar_t *uret = GWidgetOpenFile(GStringGetResource(_STR_FindCMap,NULL),NULL,NULL,NULL,CMapFilter);
	    cmapfilename = u2def_copy(uret);
	    freeme = true;
	    free(uret);
	}
	if ( cmapfilename==NULL ) {
	    SplineFontFree(cidmaster);
return(NULL);
	}
	cmap = ParseCMap(cmapfilename);
	if ( freeme )
	    free(cmapfilename);
	if ( cmap==NULL ) {
	    SplineFontFree(cidmaster);
return(NULL);
	}
	CompressCMap(cmap);
	SFEncodeToCMap(cidmaster,sf,cmap);
	cmapfree(cmap);
    } else {
	map = cidmap;
	if (map == NULL) {
	    map = AskUserForCIDMap(cidmaster);		/* Sets the ROS fields */
	} else {
	    cidmaster->cidregistry = copy(map->registry);
	    cidmaster->ordering = copy(map->ordering);
	    cidmaster->supplement = map->supplement;
	}
	if ( map==NULL ) {
	    SplineFontFree(cidmaster);
return(NULL);
	}
	SFEncodeToMap(sf,map);
    }
    cidmaster->fontname = copy(sf->fontname);
    cidmaster->fullname = copy(sf->fullname);
    cidmaster->familyname = copy(sf->familyname);
    cidmaster->weight = copy(sf->weight);
    cidmaster->copyright = copy(sf->copyright);
    cidmaster->cidversion = 1.0;
    cidmaster->display_antialias = sf->display_antialias;
    cidmaster->display_size = sf->display_size;
    cidmaster->ascent = sf->ascent /*880*/;
    cidmaster->descent = sf->descent /*120*/;
    cidmaster->changed = cidmaster->changed_since_autosave = true;
    for ( fvs=sf->fv; fvs!=NULL; fvs=fvs->nextsame )
	fvs->cidmaster = cidmaster;
    cidmaster->fv = sf->fv;
    sf->cidmaster = cidmaster;
    sf->compacted = false;
    cidmaster->subfontcnt = 1;
    cidmaster->subfonts = gcalloc(2,sizeof(SplineFont *));
    cidmaster->subfonts[0] = sf;
    if ( sf->private==NULL )
	sf->private = gcalloc(1,sizeof(struct psdict));
    if ( !PSDictHasEntry(sf->private,"lenIV"))
	PSDictChangeEntry(sf->private,"lenIV","1");		/* It's 4 by default, in CIDs the convention seems to be 1 */
    for ( fvs=sf->fv; fvs!=NULL; fvs=fvs->nextsame ) {
	free(fvs->selected);
	fvs->selected = gcalloc(fvs->sf->charcnt,sizeof(char));
    }
    SFRestoreNearTop(sf);
    FontViewReformatAll(sf);
return( cidmaster );
}

int CountOfEncoding(enum charset encoding_name) {
    Encoding *item=NULL;

    if ( encoding_name == em_unicode4 )
return( unicode4_size );
    if ( encoding_name == em_unicode || encoding_name == em_big5 || encoding_name==em_big5hkscs ||
	    encoding_name == em_johab || encoding_name == em_wansung ||
	    encoding_name == em_sjis || encoding_name == em_jisgb ||
	    (encoding_name >= em_unicodeplanes && encoding_name <= em_unicodeplanesmax ))
return( 65536 );
    else if ( encoding_name < em_first2byte )
return( 256 );
    else if ( encoding_name <= em_last94x94 )
return( 65536 );
    else if ( encoding_name >= em_base ) {
	for ( item=enclist; item!=NULL && item->enc_num!=encoding_name;
		item=item->next );
	if ( item!=NULL )
return( item->char_cnt );
    }
return( 0 );
}

static void BDFsToo(SplineFont *sf,int cnt) {
    BDFFont *bdf;
    BDFChar **newbc;
    int i;

    for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	newbc = gcalloc(cnt,sizeof(BDFChar *));
	for ( i=0; i<bdf->charcnt; ++i ) if ( bdf->chars[i]!=NULL ) {
	    int pos = bdf->chars[i]->sc->enc;
	    newbc[pos] = bdf->chars[i];
	    newbc[pos]->enc = pos;
	}
	free(bdf->chars);
	bdf->chars = newbc;
	bdf->charcnt = cnt;
	bdf->encoding_name = sf->encoding_name;
    }
}

static void _SFCompactFont(SplineFont *sf) {
    int i, cnt;
    SplineChar **newchars;
 
    for ( i=cnt=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	sf->chars[i]->old_enc = i;
	++cnt;
    }
    newchars = galloc(cnt*sizeof(SplineChar *));
    for ( i=cnt=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	newchars[cnt] = sf->chars[i];
	newchars[cnt]->enc = cnt;
	++cnt;
    }
    free(sf->chars);
    sf->chars = newchars;
    sf->charcnt = cnt;
    sf->old_encname = sf->encoding_name;
    sf->encoding_name = em_none;
    sf->compacted = true;
    sf->encodingchanged = true;
    BDFsToo(sf,cnt);
}

int SFCompactFont(SplineFont *sf) {
    MMSet *mm = sf->mm;
    int i;

    if ( sf->compacted )
return( false );

    SFFindNearTop(sf);

    if ( mm==NULL )
	_SFCompactFont(sf);
    else {
	for ( i=0; i<mm->instance_count; ++i )
	    _SFCompactFont(mm->instances[i]);
	_SFCompactFont(mm->normal);
    }

    SFRestoreNearTop(sf);
return( true );
}

static void _SFUncompactFont(SplineFont *sf) {
    int i,cnt;
    SplineChar **newchars;

    cnt = 0;
    for ( i=sf->charcnt-1; i>=0; --i ) if ( sf->chars[i]!=NULL ) {
	cnt = sf->chars[i]->old_enc+1;
    break;
    }
    if ( (i=CountOfEncoding(sf->old_encname))>cnt )
	cnt = i;
    if ( cnt==0 )
return;

    newchars = gcalloc(cnt,sizeof(SplineChar *));
    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	int enc = sf->chars[i]->old_enc;
	if ( enc>=cnt ) {
	    GDrawIError("Compacted font isn't" );
	    free( newchars );
return;
	}
	newchars[enc] = sf->chars[i];
	newchars[enc]->enc = enc;
    }
    free(sf->chars);
    sf->chars = newchars;
    sf->charcnt = cnt;
    sf->encoding_name = sf->old_encname;
    sf->compacted = false;
    sf->encodingchanged = true;

    BDFsToo(sf,cnt);
}

int SFUncompactFont(SplineFont *sf) {
    int i, cnt;
    MMSet *mm = sf->mm;

    if ( !sf->compacted )
return( false );
    cnt = 0;
    for ( i=sf->charcnt-1; i>=0; --i ) if ( sf->chars[i]!=NULL ) {
	cnt = sf->chars[i]->old_enc+1;
    break;
    }
    if ( cnt==0 )
return( false );

    SFFindNearTop(sf);

    if ( mm==NULL )
	_SFUncompactFont(sf);
    else {
	for ( i=0; i<mm->instance_count; ++i )
	    _SFUncompactFont(mm->instances[i]);
	_SFUncompactFont(mm->normal);
    }

    SFRestoreNearTop(sf);
return( true );
}

char *SFEncodingName(SplineFont *sf) {
    char buffer[130];

    if ( sf->cidmaster!=NULL )
	sf = sf->cidmaster;
    if ( sf->subfontcnt!=0 ) {
	sprintf( buffer, "%.50s-%.50s-%d", sf->cidregistry, sf->ordering, sf->supplement );
return( copy( buffer ));
    }
    switch ( sf->encoding_name ) {
      case em_custom:
return( copy( "Custom" ));
      case em_compacted:
return( copy( "Compacted" ));
      case em_original:
return( copy( "Original" ));
      default:
return( copy( EncodingName(sf->encoding_name)));
    }
}
