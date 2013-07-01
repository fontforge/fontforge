/* Copyright (C) 2008-2012 by George Williams */
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
#include <gkeysym.h>
#include <unistd.h>
#include <sys/stat.h>
#include "gfile.h"

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#ifndef __VMS
#if !defined(__MINGW32__)
#   include <sched.h>
#endif
#endif

#include <setjmp.h>
#include <signal.h>

#if defined(__MINGW32__)
#include <windows.h>
#define sleep(n) Sleep(1000 * (n))
#endif

extern GBox _ggadget_Default_Box;
#define ACTIVE_BORDER   (_ggadget_Default_Box.active_border)
#define MAIN_FOREGROUND (_ggadget_Default_Box.main_foreground)

#if defined(__MINGW32__)
# define SIGUSR1 18
#endif

#if defined(__MINGW32__)

void OFLibBrowse(void) {
}
int oflib_automagic_preview = false;

#else

/* A dialog for browsing through fonts on the Open Font Library website */


/* The OFL returns fonts 10 at a time. The biggest page so far is 52949 bytes */
/*  (16 June 2008). We might as well just plan to read each page entirely into*/
/*  memory. Not that big, and makes life easier */

#define OFL_FONT_URL	"http://openfontlibrary.org/media/view/media/fonts"
/* That gets the first 10. Subsequent request have "?offset=10" (or 20, 30, etc) appended */
/* Fonts come out "most recent first" */

static int fakemons[] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static char *monnames[] = { N_("January"), N_("February"), N_("March"), N_("April"), N_("May"), N_("June"), N_("July"), N_("August"), N_("September"), N_("October"), N_("November"), N_("December") };

enum ofl_license { ofll_ofl, ofll_pd };

struct ofl_download_urls {
    char *url;
    char *comment;
    uint8 selected;
    struct ofl_download_urls *next;
};

struct ofl_font_info {
    char *name;
    char *author;
    int date;	/* Not real date. Minutes since 2005, assuming Feb has 29 days*/
    char *taglist;
    enum ofl_license license;
    struct ofl_download_urls *urls;
    /* If they've looked at the preview image in the past, cache that locally */
    char *preview_filename;
    GImage *preview;
    /* If they interrupted the download after this font info (so there might  */
    /*  be a gap after it) set the following */
    uint8 potential_gap_after_me;
    uint8 open;		/* Display download URLS in UI */
    uint8 selected;
    uint8 downloading_in_background;
};

struct ofl_state {
    struct ofl_font_info *fonts;
    int fcnt, fmax;
    int complete;		/* True if there is no font on the website */
		/* older than those we've got. Since we download fonts in */
		/* reverse chronological order, and the user can interrupt */
		/* the process then we might have several gaps */
    /* Fonts info will be retained in reverse chronological order */
    /* it may be displayed quite differently */
};

static char *strconcat_free(char *str1, char *str2) {
    char *ret;
    int len;

    if ( str1==NULL )
return( str2 );
    if ( str2==NULL )
return( str1 );

    len = strlen(str1);
    ret = galloc(len+strlen(str2)+1);
    strcpy(ret,str1);
    strcpy(ret+len,str2);
    free(str1); free(str2);
return( ret );
}

static char *despace(char *str) {
    char *to = str, *orig = str;

    while ( *str ) {
	*to++ = *str;
	if ( isspace(*str)) {
	    while ( isspace(*str)) ++str;
	} else
	    ++str;
    }
    *to='\0';
return( orig );
}

static int parseOFLibDate(char *str) {
    char *end;
    int days, hours, min, i;
    /* OFLib dates look like "October 23, 2006 02:16 am" */

    while ( isspace(*str)) ++str;
    days = 0;
    for ( i=0; i<12; ++i ) {
	if ( strncmp(str,monnames[i],strlen(monnames[i]))==0 )
    break;
	days += fakemons[i];
    }
    if ( i!=12 )
	str += strlen(monnames[i]);
    while ( isspace(*str)) ++str;
    days += strtol(str,&end,10)-1;

    str = end;
    while ( *str==',' || isspace(*str)) ++str;
    days += 366 * (strtol(str,&end,10)-2005);

    str = end;
    while ( isspace(*str)) ++str;
    hours = strtol(str,&end,10);
    if ( hours==12 ) hours=0;
    str = end;
    if ( *str==':' ) ++str;
    min = strtol(str,&end,10);
    str = end;

    while ( isspace(*str)) ++str;
    if ( strncmp(str,"pm",2)==0 )
	hours += 12;
return( days*24*60 + hours*60 + min );
}

static char *formatOFLibDate(char *buffer, int blen, int date) {
    int year, mon, day, hours, min, am;

    min = date%60;
    date /= 60;
    hours = date%24;
    date /= 24;
    day = date % 366;
    date /= 366;
    year = 2005 + date;

    if ( hours==0 && min==0 )
	am = -1;		/* midnight */
    else if ( hours == 12 && min==0 )
	am = -2;		/* noon */
    else if ( hours<12 )
	am = true;
    else
	am = false;
    for ( mon=0 ; mon<12; ++mon ) {
	if ( day<fakemons[mon] )
    break;
	day -= fakemons[mon];
    }
    if ( am<0 )
	snprintf( buffer, blen, "%d %s %d %s", day+1, _(monnames[mon]), year,
		am==-1 ? _("Midnight") : _("Noon") );
    else
	snprintf( buffer, blen, "%d %s %d %d:%02d %s", day+1, _(monnames[mon]), year,
		hours, min, am ? _("AM") : _("PM") );
return( buffer );
}

static char *skip_over_plain_text(char *start) {
    char *end = strchr(start,'<');
    if ( end == NULL )
return( start+strlen(start));

return( end );
}

static char *skip_to_plain_text(char *start) {

    while ( *start=='<' || isspace(*start)) {
	if ( *start=='<' ) {
	    while( *start!='\0' && *start!='>' ) {
		if ( *start=='"' || *start=='\'' ) {
		    char ch = *start;
		    ++start;
		    while ( *start!='\0' && *start!=ch )
			++start;
		    if ( *start==ch ) ++start;
		} else
		    ++start;
	    }
	    if ( *start=='>' ) ++start;
	} else
	    ++start;
    }
return( start );
}

static struct ofl_font_info *ParseOFLFontPage(char *page, int *more) {
    char *pt, *start, *test, *end, *temp, *ptend, *url;
    struct ofl_font_info *block, *cur;
    struct ofl_download_urls *duhead, *dulast, *ducur;
    char *name, *author, *taglist;
    int date, license, index;

    *more = false;

    pt = strstr(page,"<!-- CONTENT STARTS -->");
    if ( pt==NULL )
return( NULL );

    index = 0;
    block = gcalloc(21,sizeof(struct ofl_font_info));

    while ( (start=strstr(pt,"<div id=\"cc_record_listing\">"))!=NULL ) {
	if ( (start = strstr(start,"<h2>"))==NULL )
    break;
	if ( (start = skip_to_plain_text(start))==NULL )
    break;
	end = skip_over_plain_text(start);
	name = copyn(start,end-start);
	start = end;

	if ( (start = strstr(start,"<th>by:</th>"))==NULL )
    break;
	start += strlen("<th>by:</th>");
	if ( (start = skip_to_plain_text(start))==NULL )
    break;
	end = skip_over_plain_text(start);
	author = copyn(start,end-start);
	start = end;

	if ( (start = strstr(start,"<th>date:</th>"))==NULL )
    break;
	start += strlen("<th>date:</th>");
	if ( (start = skip_to_plain_text(start))==NULL )
    break;
	end = skip_over_plain_text(start);
	temp = copyn(start,end-start);
	start = end;
	date = parseOFLibDate(temp);
	free(temp);

	if ( (start = strstr(start,"<th>tags:</th>"))==NULL )
    break;
	start += strlen("<th>tags:</th>");
	if ( (start = strstr(start,"<td>"))==NULL )
    break;
	if ( (end = strstr(start,"</td>"))==NULL )
    break;
	taglist = NULL;
	forever {
	    test = skip_to_plain_text(start);
	    if ( test==NULL || test>end )
	break;
	    ptend = skip_over_plain_text(test);
	    temp = despace(copyn(test,ptend-test));
	    test = ptend;
	    taglist = strconcat_free(taglist,temp);
	    /* Plain text alternates between tag and comma, we want both */
	    test = skip_over_plain_text(test);
	    if ( test==NULL )
	break;
	    start = test;
	}

	if ( (start = strstr(start,"<th>license:</th>"))==NULL )
    break;
	license = ofll_pd;
	test = strstr(start,"<img src=\"");
	if ( test==NULL )
    break;
	test = strstr(test,"lics/");
	if ( test==NULL )
    break;
	test += strlen("lics/");
	if ( strncmp(test,"small-", strlen("small-"))==0 )
	    test += strlen("small-");
	if ( test[0]=='p' && test[1]=='d' )
	    license = ofll_pd;
	else if ( test[0]=='o' && test[1]=='f' && test[2]=='l' )
	    license = ofll_ofl;

	if ( (start = strstr(start,"<div class=\"cc_file_download_popup\""))==NULL )
    break;
	if ( (end = strstr(start,"</div>"))==NULL )
    break;
	duhead = dulast = NULL;
	forever {
	    if ( (test = strstr(start,"<a href=\""))==NULL )
	break;
	    if ( test>end )
	break;
	    test += strlen("<a href=\"");
	    temp = strchr(test,'"');
	    if ( temp==NULL || temp>end )
	break;
	    url = copyn(test,temp-test);
	    test = strchr(temp,'>');
	    if ( test==NULL )
	break;
	    test = skip_to_plain_text(test+1);
	    if ( test==NULL )
	break;
	    ducur = chunkalloc(sizeof( struct ofl_download_urls ));
	    ducur->url = url;
	    ptend = skip_over_plain_text(test);
	    ducur->comment = copyn(test,ptend-test);
	    test = ptend;
	    if ( duhead==NULL )
		duhead = ducur;
	    else
		dulast->next = ducur;
	    dulast = ducur;
	    start = test;
	}
	pt = end;

	if ( duhead==NULL )
    continue;

	if ( index>20 )
    break;
	cur = &block[index];
	memset(cur,0,sizeof(*cur));
	cur->name = name;
	cur->author = author;
	cur->date = date;
	cur->taglist = taglist;
	cur->license = license;
	cur->urls = duhead;

	++index;
    }

    *more = false;
    if ( pt!=NULL ) {
	*more = strstr(pt,"More &gt;&gt;&gt;")!=NULL;
	if ( index>0 )
	    block[index-1].potential_gap_after_me = *more;
    }

return( block );
}

static void oflfiFreeContents(struct ofl_font_info *oflfi) {
    struct ofl_download_urls *du, *next;

    free(oflfi->name);
    free(oflfi->author);
    free(oflfi->taglist);
    for ( du=oflfi->urls; du!=NULL; du=next ) {
	next = du->next;
	free(du->comment);
	free(du->url);
	chunkfree(du,sizeof(*du));
    }
    free(oflfi->preview_filename);
    if ( oflfi->preview!=NULL )
	GImageDestroy(oflfi->preview);
}
    
static int OflInfoMerge(struct ofl_state *all,struct ofl_font_info *block) {
    int i,j,k,l,tot,lastj,anymatches;
    /* We return whether any of the new font_infos were the same as */
    /* any of the old */

    if ( block[0].name==NULL ) {
	free(block);
return( false );
    }

    for ( i=0; block[i].name!=NULL; ++i );
    tot = i;

    if ( all->fonts==NULL ) {
	all->fonts = block;
	all->fcnt = all->fmax = tot;
return( false );
    } else {
	anymatches = false;
	lastj = 0;
	for ( i=0; block[i].name!=NULL; ) {
	    for ( j=lastj; j<all->fcnt; ++j )
		if ( block[i].date >= all->fonts[j].date )
	    break;
	    lastj = j;
	    if ( j>0 )
		all->fonts[j-1].potential_gap_after_me = false;
	    if ( j>=all->fcnt ) {
		if ( all->fcnt+tot-i > all->fmax )
		    all->fonts = grealloc(all->fonts,(all->fmax += (tot-i+70))*sizeof(struct ofl_font_info));
		memcpy(all->fonts+all->fcnt,block+i,(tot-i)*sizeof(struct ofl_font_info));
		all->fcnt += (tot-i);
return( anymatches );
	    } else if ( block[i].date == all->fonts[j].date ) {
		oflfiFreeContents(&block[i]);
		anymatches = true;
		++i;
	    } else {
		for ( k=i; block[k].name!=NULL; ++k )
		    if ( block[k].date <= all->fonts[j].date )
		break;
		if ( all->fcnt+k-i > all->fmax )
		    all->fonts = grealloc(all->fonts,(all->fmax += (k-i+70))*sizeof(struct ofl_font_info));
		for ( l=all->fcnt-1; l>=j; --l )
		    all->fonts[l+(k-i)] = all->fonts[l];
		memcpy(all->fonts+j,block+i,(k-i)*sizeof(struct ofl_font_info));
		all->fcnt += (k-i);
		i=k;
	    }
	}
return( anymatches );
    }
}

enum tok_type { tok_int, tok_str, tok_name, tok_EOF };

struct tokbuf {
    char *buf;
    int buf_max;
    enum tok_type type;
    int ival;
};

static enum tok_type ofl_gettoken(FILE *ofl, struct tokbuf *tok) {
    int ch;

    while ( isspace( ch=getc(ofl)) );

    if ( isdigit(ch) ) {
	char numbuf[40], *pt;

	tok->type = tok_int;
	pt = numbuf;
	while ( isdigit(ch) && pt<numbuf+sizeof(numbuf)-2) {
	    *pt++ = ch;
	    ch = getc(ofl);
	}
	*pt ='\0';
	ungetc(ch,ofl);
	tok->ival = strtol(numbuf,NULL,10);
    } else if ( ch==EOF ) {
	tok->type = tok_EOF;
    } else if ( ch=='"' ) {
	char *pt, *end;
	tok->type = tok_str;
	pt = tok->buf; end = pt+tok->buf_max;
	while ( (ch=getc(ofl))!=EOF && ch!='"' ) {
	    if ( pt>=end ) {
		int off = pt-tok->buf;
		tok->buf = grealloc(tok->buf,tok->buf_max = (end-tok->buf)+200);
		pt = tok->buf+off;
		end = tok->buf+tok->buf_max;
	    }
	    *pt++ = ch;
	}
	if ( pt>=end ) {
	    int off = pt-tok->buf;
	    tok->buf = grealloc(tok->buf,tok->buf_max = (pt-end)+200);
	    pt = tok->buf+off;
	}
	*pt++ = '\0';
    } else {
	char *pt, *end;
	tok->type = tok_name;
	pt = tok->buf; end = pt+tok->buf_max;
	while ( ch!=EOF && ch!='"' && !isspace(ch) ) {
	    if ( pt>=end ) {
		int off = pt-tok->buf;
		tok->buf = grealloc(tok->buf,tok->buf_max = (end-tok->buf)+200);
		pt = tok->buf+off;
		end = tok->buf+tok->buf_max;
	    }
	    *pt++ = ch;
	    ch = getc(ofl);
	}
	if ( pt>=end ) {
	    int off = pt-tok->buf;
	    tok->buf = grealloc(tok->buf,tok->buf_max = (pt-end)+200);
	    pt = tok->buf+off;
	}
	*pt++ = '\0';
    }
return( tok->type );
}

static char *getOFLibDir(void) {
    static char *oflibdir=NULL;
    char buffer[1025];

    if ( oflibdir!=NULL )
return( oflibdir );
    if ( getPfaEditDir(buffer)==NULL )
return( NULL );
    sprintf(buffer,"%s/OFLib", getPfaEditDir(buffer));
    if ( access(buffer,F_OK)==-1 )
	if ( GFileMkDir(buffer)==-1 )
return( NULL );

    oflibdir = copy(buffer);
return( oflibdir );
}

static void LoadOFLibState(struct ofl_state *all) {
    FILE *file;
    int ch;
    struct ofl_download_urls *du, *last;
    struct ofl_font_info *cur;
    struct tokbuf tok;
    char buffer[1025];

    if ( getOFLibDir()==NULL )
return;

    sprintf( buffer,"%s/State", getOFLibDir());
    file = fopen(buffer,"r");
    if ( file==NULL )
return;

    memset(&tok,0,sizeof(tok));

    if ( ofl_gettoken(file,&tok)!= tok_name || strcmp(tok.buf,"OFLibState")!=0 ) {
	/* Not an OFLibState file */
	fclose(file);
	free(tok.buf);
return;
    }
    cur = NULL;
    forever {
	if ( ofl_gettoken(file,&tok)!= tok_name )
    break;
	if ( strcmp(tok.buf,"Count:")==0 ) {
	    if ( ofl_gettoken(file,&tok)!= tok_int )
    break;
	    all->fmax = tok.ival;
	    all->fcnt = 0;
	    all->fonts = gcalloc(all->fmax,sizeof(struct ofl_font_info));
	} else if ( strcmp(tok.buf,"Complete:")==0 ) {
	    if ( ofl_gettoken(file,&tok)!= tok_int )
    break;
	    all->complete = tok.ival;
	} else if ( strcmp(tok.buf,"Font:")==0 ) {
	    cur = &all->fonts[all->fcnt];
	    if ( ofl_gettoken(file,&tok)!= tok_str )
    break;
	    cur->name = copy(tok.buf);
	    if ( ofl_gettoken(file,&tok)!= tok_str )
    break;
	    cur->author = copy(tok.buf);
	    if ( ofl_gettoken(file,&tok)!= tok_int )
    break;
	    cur->date = tok.ival;
	    if ( ofl_gettoken(file,&tok)!= tok_str )
    break;
	    cur->taglist = copy(tok.buf);
	    if ( ofl_gettoken(file,&tok)!= tok_str )
    break;
	    cur->license = strcmp(tok.buf,"OFL")==0 ? ofll_ofl : ofll_pd;
	    if ( ofl_gettoken(file,&tok)!= tok_str )
    break;
	    cur->preview_filename = tok.buf[0]=='\0' ? NULL : copy(tok.buf);
	    if ( ofl_gettoken(file,&tok)!= tok_int )
    break;
	    cur->potential_gap_after_me = tok.ival;
	    ++(all->fcnt);
	    last = NULL;
	} else if ( strcmp(tok.buf,"URL:")==0 ) {
	    du = chunkalloc(sizeof( struct ofl_download_urls ));
	    if ( ofl_gettoken(file,&tok)!= tok_str )
    break;
	    du->comment = copy(tok.buf);
	    if ( ofl_gettoken(file,&tok)!= tok_str )
    break;
	    du->url = copy(tok.buf);
	    if ( cur==NULL )
    break;
	    if ( last==NULL )
		cur->urls = du;
	    else
		last->next = du;
	    last = du;
	}
	while ( (ch=getc(file))!=EOF && ch!='\n' );
    }
    free(tok.buf);
    fclose(file);
}

static void DumpOFLibState(struct ofl_state *all) {
    FILE *file;
    int i;
    struct ofl_download_urls *du;
    struct ofl_font_info *cur;
    char buffer[1025];

    if ( getOFLibDir()==NULL )
return;

    sprintf( buffer,"%s/State", getOFLibDir());
    file = fopen(buffer,"w");
    if ( file==NULL )
return;

    fprintf( file, "OFLibState\n" );
    fprintf( file, "Count: %d\n", all->fcnt );
    fprintf( file, "Complete: %d\n", all->complete );	/* Well, it was complete when saved, might be new things since, but there are no holes */
    for ( i=0; i<all->fcnt; ++i ) {
	cur = &all->fonts[i];
	fprintf( file, "Font: \"%s\" \"%s\" %d \"%s\" \"%s\" \"%s\" %d\n",
		cur->name, cur->author, cur->date, cur->taglist,
		cur->license==ofll_ofl ? "OFL" : "PD",
		cur->preview_filename == NULL ? "" : cur->preview_filename,
		cur->potential_gap_after_me );
	for ( du = cur->urls; du!=NULL; du=du->next ) {
	    fprintf( file, " URL: \"%s\" \"%s\"\n", du->comment, du->url );
	}
    }
    fclose(file);
}

/* ************************************************************************** */
/* ********************************* Dialog ********************************* */
/* ************************************************************************** */

int oflib_automagic_preview = false;

typedef struct previewthread {
    struct previewthread *next;
    pthread_t preview_thread;
    pthread_mutex_t *http_thread_can_do_stuff;
    int done;
    struct ofl_font_info *fi;
    struct ofl_download_urls *active;
    int is_image;
    FILE *result;
} PreviewThread;

typedef struct oflibdlg {
    GWindow gw;
    struct ofl_state all;
    int scnt, smax;
    struct ofl_font_info **show;
    int ltot, lsize, l_off_top;
    int margin;
    int fh,as;
    GFont *font;
    int lheight, lwidth;
    int pheight, pwidth;
    int p_off_top, p_off_left;
    GImage *preview_image;

  /* Spawn separate thread for downloading fonts information */
  /* (Not for the downloads the user asks for, but the background download */
  /*  to check that our database is up-to-date, with luck, user won't notice) */

    pthread_t http_thread;
    pthread_mutex_t http_thread_can_do_stuff;	/* Like allocate memory, call non-thread-safe routines */
    pthread_mutex_t http_thread_done;		/* child releases this when it has finished its current task */
    GTimer *http_timer;
    char *databuf;
    int datalen;
    int nextfont;
    int done;
    int amount_read;
    int any_new_stuff;
    int die;
    GWindow alert;
    int background_preview_requests;
    PreviewThread *active;
} OFLibDlg;
static OFLibDlg *active;
static pthread_key_t jump_key;

enum searchtype { st_showall, st_author, st_name, st_tag, st_license };
static int initted = false;
static GTextInfo searchtypes[] = {
    { (unichar_t *) N_("Show All"), NULL, 0, 0, (void *) st_showall, NULL, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0'},	/* Selected, One byte */
    { (unichar_t *) N_("Designer"), NULL, 0, 0, (void *) st_author, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Name"), NULL, 0, 0, (void *) st_name, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Tag(s)"), NULL, 0, 0, (void *) st_tag, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("License"), NULL, 0, 0, (void *) st_license, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};

#define CID_SortDate	1001
#define CID_SortAuthor	1002
#define CID_SortName	1003
#define CID_SortReverse	1004

#define CID_SearchType	1011
#define CID_SearchString 1012

#define CID_Fonts	1021
#define CID_FontSB	1022

#define CID_PreviewW	1031
#define CID_PreviewVSB	1032
#define CID_PreviewHSB	1033
#define CID_BackgroundPreview 1034

#define CID_Open	1040
#define CID_Preview	1041
#define CID_Done	1042

#define CID_TopBox	1051

static void OFLibInit(void) {
    int i;
    initted = true;
    for ( i=0; searchtypes[i].text!=NULL; ++i )
	searchtypes[i].text = (unichar_t *) _((char *) searchtypes[i].text);
}

static struct ofl_download_urls *OFLibHasImage(OFLibDlg *d,int sel_font) {
    struct ofl_download_urls *du;

    if ( d->show[sel_font]->open ) {
	/* If an image is selected use it */
	for ( du=d->show[sel_font]->urls; du!=NULL; du=du->next ) {
	    char *ext = strrchr(du->url,'.');
	    if ( ext==NULL || !du->selected)
	continue;
	    if (
#ifndef _NO_LIBPNG
		    strcasecmp(ext,".png")==0 ||
#endif
#ifndef _NO_LIBJPEG
		    strcasecmp(ext,".jpeg")==0 || strcasecmp(ext,".jpg")==0 ||
#endif
#ifndef _NO_LIBTIFF
		    strcasecmp(ext,".tiff")==0 || strcasecmp(ext,".tif")==0 ||
#endif
#ifndef _NO_LIBUNGIF
		    strcasecmp(ext,".gif")==0 ||
#endif
		    strcasecmp(ext,".bmp")==0 )
return( du );
	}
    }

    /* Otherwise use an unselected image (if present) */
    for ( du=d->show[sel_font]->urls; du!=NULL; du=du->next ) {
	char *ext = strrchr(du->url,'.');
	if ( ext==NULL )
    continue;
	if (
#ifndef _NO_LIBPNG
		    strcasecmp(ext,".png")==0 ||
#endif
#ifndef _NO_LIBJPEG
		    strcasecmp(ext,".jpeg")==0 || strcasecmp(ext,".jpg")==0 ||
#endif
#ifndef _NO_LIBTIFF
		    strcasecmp(ext,".tiff")==0 || strcasecmp(ext,".tif")==0 ||
#endif
#ifndef _NO_LIBUNGIF
		    strcasecmp(ext,".gif")==0 ||
#endif
		    strcasecmp(ext,".bmp")==0 )
return( du );
    }
return( NULL );
}

static struct ofl_download_urls *OFLibHasFont(OFLibDlg *d,int sel_font) {
    struct ofl_download_urls *du;
		struct ofl_download_urls *otf=NULL, *pf=NULL, *zip=NULL, *sfd=NULL;

    for ( du=d->show[sel_font]->urls; du!=NULL; du=du->next ) {
	char *ext = strrchr(du->url,'.');
	if ( ext==NULL )
    continue;
	if ( strcasecmp(ext,".zip")==0 )
	    zip = du;
	else if ( strcasecmp(ext,".pfa")==0 || strcasecmp(ext,".pfb")==0 ||
		strcasecmp(ext,".cff")==0 )
	    pf = du;
	else if ( strcasecmp(ext,".ttf")==0 || strcasecmp(ext,".otf")==0 ||
		strcasecmp(ext,".ttc")==0 )
	    otf = du;
	else if ( strcasecmp(ext,".sfd")==0 )
	    sfd = du;
    }
    if ( sfd!=NULL )
return( sfd );
    else if ( otf!=NULL )
return( otf );
    else if ( pf!=NULL )
return( pf );
    else
return( zip );
}

static void OFLibSetSb(OFLibDlg *d) {
    GGadget *sb = GWidgetGetControl(d->gw,CID_FontSB);
    struct ofl_download_urls *du;
    int i, tot;

    /* Number of lines visible */
    for ( i=tot=0; i<d->scnt; ++i ) {
	++tot;
	if ( d->show[i]->open ) {
	    for ( du=d->show[i]->urls; du!=NULL; du=du->next )
		++tot;
	}
    }

    d->ltot = tot;
    d->lsize = (d->lheight-2*d->margin)/d->fh;
    GScrollBarSetBounds(sb,0,d->ltot,d->lsize);
    if ( d->l_off_top+d->lsize > d->ltot )
	d->l_off_top = d->ltot-d->lsize;
    if ( d->l_off_top < 0 )
	d->l_off_top = 0;
    GScrollBarSetPos(sb,d->l_off_top);
}

static int sortrevdate(const void *_op1, const void *_op2) {
    const struct ofl_font_info *op1 = *(const struct ofl_font_info **) _op1;
    const struct ofl_font_info *op2 = *(const struct ofl_font_info **) _op2;
return( op2->date - op1->date );
}

static int sortdate(const void *_op1, const void *_op2) {
    const struct ofl_font_info *op1 = *(const struct ofl_font_info **) _op1;
    const struct ofl_font_info *op2 = *(const struct ofl_font_info **) _op2;
return( op1->date - op2->date );
}

static int sortrevauthor(const void *_op1, const void *_op2) {
    const struct ofl_font_info *op1 = *(const struct ofl_font_info **) _op1;
    const struct ofl_font_info *op2 = *(const struct ofl_font_info **) _op2;
return( strcasecmp(op2->author, op1->author) );
}

static int sortauthor(const void *_op1, const void *_op2) {
    const struct ofl_font_info *op1 = *(const struct ofl_font_info **) _op1;
    const struct ofl_font_info *op2 = *(const struct ofl_font_info **) _op2;
return( strcasecmp(op1->author, op2->author) );
}

static int sortrevname(const void *_op1, const void *_op2) {
    const struct ofl_font_info *op1 = *(const struct ofl_font_info **) _op1;
    const struct ofl_font_info *op2 = *(const struct ofl_font_info **) _op2;
return( strcasecmp(op2->name, op1->name) );
}

static int sortname(const void *_op1, const void *_op2) {
    const struct ofl_font_info *op1 = *(const struct ofl_font_info **) _op1;
    const struct ofl_font_info *op2 = *(const struct ofl_font_info **) _op2;
return( strcasecmp(op1->name, op2->name) );
}

static void OFLibSortSearch(OFLibDlg *d) {
    int reverse = GGadgetIsChecked( GWidgetGetControl(d->gw,CID_SortReverse));
    enum searchtype st = (intpt) GGadgetGetListItemSelected(GWidgetGetControl(d->gw,CID_SearchType))->userdata;
    char *search = GGadgetGetTitle8( GWidgetGetControl(d->gw,CID_SearchString));
    int i, s;
    int isofl = false, ispd = false;
    char *end, *start, *found, ch;

    if ( d->smax<d->all.fcnt )
	d->show = grealloc(d->show,(d->smax = d->all.fcnt+20)*sizeof(struct ofl_font_info *));
    if ( st==st_license ) {
	isofl = strcasecmp(search,"ofl")==0;
	ispd  = strcasecmp(search,"pd")==0;
    }
    for ( i=s=0; i<d->all.fcnt ; ++i ) {
	int include = false;
	switch ( st ) {
	  case st_showall:
	    include = true;
	  break;
	  case st_author:
	    include = strstrmatch(d->all.fonts[i].author,search)!=NULL;
	  break;
	  case st_name:
	    include = strstrmatch(d->all.fonts[i].name,search)!=NULL;
	  break;
	  case st_license:
	    include = (isofl && d->all.fonts[i].license==ofll_ofl) ||
			(ispd && d->all.fonts[i].license==ofll_pd);
	  break;
	  case st_tag:	/* check that each tag is found */
	    start = search;
	    include = true;
	    while ( *start ) {
		while ( *start==',' || isspace(*start)) ++start;
		if ( *start=='\0' )
	    break;
		for ( end=start; *end!=',' && *end!='\0'; ++end );
		ch = *end; *end = '\0';
		found = strstrmatch(d->all.fonts[i].taglist, start);
		*end = ch;
		if ( found==NULL ) {
		    include = false;
	    break;
		} else if ( found==d->all.fonts[i].taglist ||
			found[-1] == ',' ||
			isspace(found[-1]) ) {
		    /* Looks good */
		} else {
		    include = false;
	    break;
		}
		ch = found[ end-start ];
		if ( ch=='\0' || ch==',' || isspace(ch)) {
		    /* Is good */
		} else {
		    include = false;
	    break;
		}
		start = end;
	    }
	  break;
	  default:
	    include = false;
	  break;
	}
	if ( include )
	    d->show[s++] = &d->all.fonts[i];
    }
    d->scnt = s;

    if ( s!=0 ) {
	qsort(d->show,s,sizeof(d->show[0]),
	    GGadgetIsChecked( GWidgetGetControl(d->gw,CID_SortDate)) ?
		    (reverse ? sortrevdate : sortdate) :
	    GGadgetIsChecked( GWidgetGetControl(d->gw,CID_SortAuthor)) ?
		    (reverse ? sortrevauthor : sortauthor) :
		    (reverse ? sortrevname : sortname) );
    }
    OFLibSetSb(d);
    GGadgetRedraw( GWidgetGetControl(d->gw,CID_Fonts));
}

static int OFL_SortBy(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	OFLibDlg *d = GDrawGetUserData(GGadgetGetWindow(g));
	OFLibSortSearch(d);
    }
return( true );
}

static int OFL_SearchFor(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	OFLibDlg *d = GDrawGetUserData(GGadgetGetWindow(g));
	OFLibSortSearch(d);
    }
return( true );
}

static int OFL_SearchStringChanged(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	OFLibDlg *d = GDrawGetUserData(GGadgetGetWindow(g));
	OFLibSortSearch(d);
    }
return( true );
}

static void PreviewThreadsKill(OFLibDlg *d) {
    PreviewThread *next, *cur;
    void *status;

    for ( cur = d->active; cur!=NULL; cur = next ) {
	next = cur->next;
#ifdef __VMS
       pthread_cancel(cur->preview_thread);
#else
       pthread_kill(cur->preview_thread,SIGUSR1);	/* I want to use pthread_cancel, but that seems to send a SIG32, (only 0-31 are defined) which can't be trapped */
#endif
       pthread_join(cur->preview_thread,&status);
	if ( cur->result!=NULL )
	    fclose(cur->result);
	chunkfree(cur,sizeof(*cur));
    }
    d->active = NULL;
}

static void HttpThreadKill(OFLibDlg *d) {
    /* This should be called in the parent, obviously */

    if ( pthread_equal(pthread_self(),d->http_thread) ) {
	char *msg = "Bad call to HttpThreadKill\n";
	write(2,msg,strlen(msg));
pthread_exit(NULL);
    }

    /* Ok, we've read everything, don't need the timer any more */
    if ( d->http_timer )
	GDrawCancelTimer(d->http_timer);
    d->http_timer=NULL;

    d->die = true;
#if defined(__MINGW32__)
    if ( d->http_thread.x )
#else
    if ( d->http_thread )
#endif
    {
	void *status;
	pthread_mutex_unlock(&d->http_thread_done);
	pthread_mutex_unlock(&d->http_thread_can_do_stuff);
#ifdef __VMS
       pthread_cancel(d->http_thread);
#else
       pthread_kill(d->http_thread,SIGUSR1);	/* I want to use pthread_cancel, but that seems to send a SIG32, (only 0-31 are defined) which can't be trapped */
#endif
	pthread_join(d->http_thread,&status);
	pthread_mutex_destroy(&d->http_thread_can_do_stuff);
	pthread_mutex_destroy(&d->http_thread_done);
    }
    free(d->databuf);
    d->databuf = NULL;
    d->datalen = 0;
    d->done = 0;
    d->nextfont = 0;
    d->amount_read = 0;
    d->any_new_stuff = 0;
    d->die = false;

    /* Remove the notice window (if it's still around) */
    ff_post_notice(NULL,NULL);
    
}

static void cancel_handler(int sig) {
    longjmp(pthread_getspecific(jump_key),1);
}

static void *StartHttpThread(void *_d) {
    OFLibDlg *d = (OFLibDlg *) _d;
    jmp_buf abort_env;

    pthread_setspecific(jump_key,abort_env);
    if ( setjmp(abort_env) ) {
	signal(SIGUSR1,SIG_DFL);		/* Restore normal behavior */
return(NULL);
    }
    signal(SIGUSR1,cancel_handler);

    forever {
	if ( d->nextfont==0 )
	    d->amount_read = HttpGetBuf( OFL_FONT_URL, d->databuf, &d->datalen, &d->http_thread_can_do_stuff );
	else {
	    sprintf( d->databuf, OFL_FONT_URL "?offset=%d", d->nextfont );
	    d->amount_read = HttpGetBuf( d->databuf, d->databuf, &d->datalen, &d->http_thread_can_do_stuff );
	}
	d->done = true;
	if ( d->die ) {
	    signal(SIGUSR1,SIG_DFL);		/* Restore normal behavior */
return(NULL);
	}
	forever {
	    pthread_mutex_unlock(&d->http_thread_done);
	    sleep(1);
	    /* Give the parent a chance to run. */
	    if ( d->die ) {
		signal(SIGUSR1,SIG_DFL);	/* Restore normal behavior */
return(NULL);
	    }
	    pthread_mutex_lock(&d->http_thread_done);
	    if ( !d->done ) {	/* If we aren't done, then parent has given us*/
				/* a new task */
	break;
	    }
	    /* Otherwise it isn't ready to run (and process what we got), just*/
	    /*  keep giving it some time */
	}
	/* OK, see what the parent wants us to do now... */
    }
}

static void *StartPreviewThread(void *_p) {
    PreviewThread *p = (PreviewThread *) _p;
    jmp_buf abort_env;

    pthread_setspecific(jump_key,abort_env);
    if ( setjmp(abort_env) ) {
	signal(SIGUSR1,SIG_DFL);		/* Restore normal behavior */
return(NULL);
    }
    signal(SIGUSR1,cancel_handler);
    p->result = URLToTempFile(p->active->url,p->http_thread_can_do_stuff);
    p->done = true;
return(NULL);
}

static void OFLibEnableButtons(OFLibDlg *d);

static void ProcessPreview(OFLibDlg *d,PreviewThread *cur) {
    char *pt, *name;
    FILE *final;
    int ch;

    cur->fi->downloading_in_background = false;
    if ( cur->result==NULL )		/* Finished, but didn't work */
return;
    rewind(cur->result);

    pt = strrchr(cur->active->url,'/');
    if ( pt==NULL ) {	/* Can't happen */
	fclose(cur->result);
return;
    }
    name = galloc(strlen(getOFLibDir()) + strlen(pt) + 10 );
    sprintf( name,"%s%s", getOFLibDir(), pt);
    final = fopen(name,"w");
    if ( final==NULL ) {
	fclose(cur->result);
return;
    }
    GDrawSetCursor(d->gw,ct_watch);

    if ( cur->is_image ) {
	while ( (ch=getc(cur->result))!=EOF )
	    putc(ch,final);
	fclose(final);
	fclose(cur->result);
    } else {
	SplineFont *sf = _ReadSplineFont(cur->result,cur->active->url,0);
	/* The above routine closes cur->result */
	if ( sf==NULL ) {
	    fclose(final);
	    unlink(name);
	    free(name);
	    GDrawSetCursor(d->gw,ct_mypointer);
return;
	}
	pt = strrchr(name,'.');
	if ( pt==NULL || pt<strrchr(name,'/') )
	    strcat(name,".png");
	else
	    strcpy(pt,".png");
	SFDefaultImage(sf,name);
	SplineFontFree(sf);
    }
    cur->fi->preview_filename = copy(strrchr(name,'/')+1);
    free(name);

    OFLibEnableButtons(d);		/* This will load the image */
    DumpOFLibState(&d->all);
    GDrawSetCursor(d->gw,ct_mypointer);
}

static void BackgroundPreviewRequest(OFLibDlg *d,int onefont) {
    struct ofl_download_urls *du;
    PreviewThread *newp;
    int is_image = true;

    if ( d->show[onefont]->downloading_in_background )
return;
    if ( getOFLibDir()==NULL )
return;
    du = OFLibHasImage(d,onefont);
    if ( du==NULL ) {
	du = OFLibHasFont(d,onefont);
	is_image = false;
    }
    if ( du==NULL )
return;

    newp = chunkalloc(sizeof(PreviewThread));
    newp->fi = d->show[onefont];
    newp->fi->downloading_in_background = true;
    newp->active = du;
    newp->is_image = is_image;
    newp->next = d->active;
    newp->http_thread_can_do_stuff = &d->http_thread_can_do_stuff;
    d->active = newp;
    pthread_create( &newp->preview_thread, NULL, StartPreviewThread, newp);
}

static void CheckPreviewActivity(OFLibDlg *d) {
    PreviewThread *prev, *next, *cur;

    for ( prev=NULL, cur = d->active; cur!=NULL; cur = next ) {
	next = cur->next;
	if ( cur->done ) {
	    void *status;
	    ProcessPreview(d,cur);
	    cur->fi->downloading_in_background = false;
	    pthread_join(cur->preview_thread,&status);
	    if ( prev==NULL )
		d->active = next;
	    else
		prev->next = next;
	    chunkfree(cur,sizeof(*cur));
	} else {
	    prev = cur;
	}
    }
}

static void HttpListStuff(OFLibDlg *d) {
    /* We use timers to manage a pthread which does HTTP calls to the open */
    /*  font library to grab a chunk of data containing some font info */
    /* First time we are called, we start the thread up */
    /* Thereafter we test if the thread has finished its current task */
    /* If it hasn't finished, we release a mutex and the processor to give the*/
    /*  child a chance to do stuff which (like allocate memory) which might */
    /*  cause contention with our own thread */
    /* If it has finished, we parse what came back, merge it in with what we've*/
    /*  got so far, resort, etc. and redisplay the list of fonts. Then we check*/
    /*  to see if there is any more fontinfo on the site. If there is, we start*/
    /*  the child up again with a new starting font number, and ask for another*/
    /*  chunk of data. If there isn't we kill the child and destroy the timer */

    if ( d->databuf==NULL ) {
	/* We're just starting up */
	d->datalen = 128*1024;		/* More than twice the size of the largest page on OFLib */
	d->databuf = galloc(d->datalen);
	pthread_mutex_init(&d->http_thread_can_do_stuff,NULL);
	pthread_mutex_init(&d->http_thread_done,NULL);
	pthread_mutex_lock(&d->http_thread_can_do_stuff);
	d->nextfont = 0;
	d->done = 0;
	d->any_new_stuff = 0;
	pthread_create( &d->http_thread, NULL, StartHttpThread, d);
    } else if ( d->done && pthread_mutex_trylock(&d->http_thread_done) ) {
	int len, more, i, any = false;
	struct ofl_font_info *block;
	len = d->amount_read;
	if ( len<=0 ) {
	    /* Either we got an error, or the user aborted. In either case, stop */
	    HttpThreadKill(d);
	    d->all.complete = false;
	    if ( any )
		DumpOFLibState(&d->all);
return;
	}
	block = ParseOFLFontPage(d->databuf, &more);
	OflInfoMerge(&d->all,block);
	d->any_new_stuff = true;
	OFLibSortSearch(d);	/* And display the new stuff */

	for ( i=0; i<d->all.fcnt; ++i )
	    if ( d->all.fonts[i].potential_gap_after_me )
	break;
	if ( i==d->all.fcnt ) {
	    d->all.complete = true;
	    DumpOFLibState(&d->all);
	    HttpThreadKill(d);
return;
	}
	d->nextfont = i;
	d->done = false;
	pthread_mutex_unlock(&d->http_thread_done);	/* Start the thread up again */
    }

    /* There might also be preview download threads active */
    CheckPreviewActivity(d);

    pthread_mutex_unlock(&d->http_thread_can_do_stuff);
    sched_yield();			/* Give the child a chance to allocate memory when we won't be */
    pthread_mutex_lock(&d->http_thread_can_do_stuff);
}

static int OFL_Done(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	OFLibDlg *d = GDrawGetUserData(GGadgetGetWindow(g));
	GDrawDestroyWindow(d->gw);
    }
return( true );
}

static int OFLibParseSelection(OFLibDlg *d, int *any) {
    int i, anyfont, onefont, hiddenonefont;
    struct ofl_download_urls *du;

    anyfont = 0; onefont = -1; hiddenonefont = -1;
    for ( i=0; i<d->scnt; ++i ) {
	if ( d->show[i]->open ) {
	    for ( du = d->show[i]->urls; du!=NULL; du=du->next ) {
		if ( du->selected ) {
		    anyfont = true;
		    if ( onefont==-1 )
			onefont=i;
		    else if ( onefont!=i )
			onefont = -2;
		}
	    }
	    if ( d->show[i]->selected ) {
		if ( hiddenonefont==-1 )
		    hiddenonefont = i;
		else
		    hiddenonefont = -2;
	    }
	} else if ( d->show[i]->selected ) {
	    anyfont = true;
	    if ( onefont==-1 )
		onefont=i;
	    else if ( onefont!=i )
		onefont = -2;
	}
    }
    if ( onefont==-1 && hiddenonefont>=0 )
	onefont = hiddenonefont;
    if ( any!=NULL ) *any = anyfont;
return( onefont );
}

static void OFLibEnableButtons(OFLibDlg *d) {
    int anyfont, onefont;
    int refigure;
    char buffer[1025];

    onefont = OFLibParseSelection(d,&anyfont);

    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Open),anyfont);
    if ( onefont>=0 && d->show[onefont]->preview==NULL &&
	    d->show[onefont]->preview_filename!=NULL ) {
	snprintf( buffer, sizeof(buffer), "%s/%s", getOFLibDir(), d->show[onefont]->preview_filename );
	if ( access(buffer,R_OK)!=-1 )
	    d->show[onefont]->preview = GImageRead(buffer);
	if ( d->show[onefont]->preview==NULL ) {
	    free(d->show[onefont]->preview_filename);
	    d->show[onefont]->preview_filename = NULL;
	}
    }
    if ( onefont>=0 && d->show[onefont]->preview!=NULL ) {
	int same, width, height, nh;
	GRect r;
	GGadget *hsb = GWidgetGetControl(d->gw,CID_PreviewHSB);
	GGadget *vsb = GWidgetGetControl(d->gw,CID_PreviewVSB);
	GGadget *pre = GWidgetGetControl(d->gw,CID_PreviewW);

	width = GImageGetWidth(d->show[onefont]->preview);
	height = GImageGetHeight(d->show[onefont]->preview);
	if ( height<20 )
	    nh = 20;
	else if ( height<400 )
	    nh = height;
	else
	    nh = 400;
	r.x=r.y=0; r.width = -1; r.height = nh;
	GGadgetSetDesiredSize(pre,NULL,&r);

	refigure = d->preview_image==NULL || nh!= d->pheight;
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Preview),false);
	GGadgetSetVisible(pre,true);
	GGadgetSetVisible(vsb,true);
	GGadgetSetVisible(hsb,true);
	same = d->preview_image == d->show[onefont]->preview;
	d->preview_image = d->show[onefont]->preview;
	if ( !same ) {
	    GScrollBarSetPos(hsb,0); d->p_off_left = 0;
	    GScrollBarSetPos(vsb,0); d->p_off_top  = 0;
	    GScrollBarSetBounds(hsb,0,width,d->pwidth);
	    GScrollBarSetBounds(vsb,0,height,d->pheight);
	    GDrawRequestExpose(GDrawableGetWindow(pre), NULL,true);
	}
    } else if ( onefont>=0 && d->background_preview_requests ) {
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Preview),false);
	BackgroundPreviewRequest(d,onefont);
    } else {
	/* Even if there is no preview image supplied, enable the button */
	/*  I generate previews if none is provided */
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Preview),onefont>=0 &&
		getOFLibDir()!=NULL );
	/* GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Preview),onefont>=0 &&
		OFLibHasImage(d,onefont)!=NULL &&
		getOFLibDir()!=NULL );*/
	GGadgetSetVisible(GWidgetGetControl(d->gw,CID_PreviewW),false);
	GGadgetSetVisible(GWidgetGetControl(d->gw,CID_PreviewVSB),false);
	GGadgetSetVisible(GWidgetGetControl(d->gw,CID_PreviewHSB),false);
	refigure = d->preview_image!=NULL;
	d->preview_image = NULL;
    }
    if ( refigure ) {
	GGadget *fonts = GWidgetGetControl(d->gw,CID_Fonts);
	GRect r;
	GGadgetGetSize(fonts,&r);
	GGadgetSetDesiredSize(fonts,&r,NULL);
	GHVBoxFitWindow(GWidgetGetControl(d->gw,CID_TopBox));
    }
}

static int OFL_Preview(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	OFLibDlg *d = GDrawGetUserData(GGadgetGetWindow(g));
	int onefont, ch;
	struct ofl_download_urls *du;
	FILE *temp, *final;
	char *name, *pt;

	if ( getOFLibDir()==NULL )
return( true );
	onefont = OFLibParseSelection(d,NULL);
	if ( onefont<0 )
return( true );
	du = OFLibHasImage(d,onefont);
	if ( du!=NULL ) {
	    /* Download the preview which the user provided */
	    pt = strrchr(du->url,'/');
	    if ( pt==NULL )
return( true );
	    name = galloc(strlen(getOFLibDir()) + strlen(pt) + 10 );
	    sprintf( name,"%s%s", getOFLibDir(), pt);
	    final = fopen(name,"w");
	    if ( final==NULL )
return( true );

	    temp = URLToTempFile(du->url,NULL);
	    if ( temp==NULL ) {
		fclose(final);
		unlink(name);
		free(name);
return( true );
	    }
	    rewind(temp);
	    while ( (ch=getc(temp))!=EOF )
		putc(ch,final);
	    fclose(temp);
	    fclose(final);
	} else {
	    /* Download a font file, and then generate a preview from that */
	    SplineFont *sf;
	    du = OFLibHasFont(d,onefont);
	    if ( du==NULL )
return( true );		/* No FONT??? */
	    sf = LoadSplineFont(du->url,0);
	    if ( sf==NULL )
return( true );
	    pt = strrchr(du->url,'/');
	    if ( pt==NULL )
return( true );
	    name = galloc(strlen(getOFLibDir()) + strlen(pt) + strlen( ".png" ) + 10 );
	    sprintf( name, "%s%s", getOFLibDir(), pt );
	    pt = strrchr(name,'.');
	    if ( pt==NULL || pt<strrchr(name,'/') )
		strcat(name,".png");
	    else
		strcpy(pt,".png");
	    SFDefaultImage(sf,name);
	    SplineFontFree(sf);
	}
	d->show[onefont]->preview_filename = copy(strrchr(name,'/')+1);
	free(name);
	OFLibEnableButtons(d);		/* This will load the image */
	DumpOFLibState(&d->all);
    }
return( true );
}

static int OFL_Open(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	OFLibDlg *d = GDrawGetUserData(GGadgetGetWindow(g));
	struct ofl_download_urls *du;
	int i;
	for ( i=0; i<d->scnt; ++i ) {
	    if ( d->show[i]->open ) {
		/* Ignore selection on top level if it is open */
		for ( du=d->show[i]->urls ; du!=NULL ; du=du->next )
		    if ( du->selected )
			ViewPostScriptFont(du->url,0);
	    } else if ( d->show[i]->selected ) {
		du = OFLibHasFont(d,i);
		if ( du!=NULL )
		    ViewPostScriptFont(du->url,0);
		else
		    GDrawBeep(NULL);
	    }
	}
    }
return( true );
}

static int OFL_BackgroundPreviewChange(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	OFLibDlg *d = GDrawGetUserData(GGadgetGetWindow(g));
	d->background_preview_requests = GGadgetIsChecked(g);
	oflib_automagic_preview = d->background_preview_requests;
	SavePrefs(true);
    }
return( true );
}

static int oflib_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	GDrawDestroyWindow(gw);
    } else if ( event->type == et_destroy ) {
	OFLibDlg *d = GDrawGetUserData(gw);
	int i;
	if ( d->http_timer )
	    GDrawCancelTimer(d->http_timer);
	d->http_timer=NULL;
	HttpThreadKill(d);
	PreviewThreadsKill(d);
	DumpOFLibState(&d->all);
	for ( i=0; i<d->all.fcnt; ++i )
	    oflfiFreeContents(&d->all.fonts[i]);
	free(d->all.fonts);
	free(d->show);
	free(d);
	active = NULL;
	pthread_key_delete(jump_key);
    } else if ( event->type == et_char ) {
return( false );
    } else if ( event->type == et_timer ) {
	HttpListStuff(GDrawGetUserData(gw));
    }
return( true );
}

static int OFLib_FontsVScroll(GGadget *g,GEvent *event) {
    OFLibDlg *d = GDrawGetUserData(GGadgetGetWindow(g));
    int newpos = d->l_off_top;
    int32 sb_min, sb_max, sb_pagesize;

    if ( event->type!=et_controlevent || event->u.control.subtype != et_scrollbarchange )
return( true );

    GScrollBarGetBounds(event->u.control.g,&sb_min,&sb_max,&sb_pagesize);
    switch( event->u.control.u.sb.type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= 9*sb_pagesize/10;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
        newpos += 9*sb_pagesize/10;
      break;
      case et_sb_bottom:
        newpos = (sb_max-sb_pagesize);
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = event->u.control.u.sb.pos;
      break;
    }
    if ( newpos>(sb_max-sb_pagesize) )
        newpos = (sb_max-sb_pagesize);
    if ( newpos<0 ) newpos = 0;
    if ( newpos!=d->l_off_top ) {
	d->l_off_top = newpos;
	GScrollBarSetPos(event->u.control.g,newpos);
	GDrawRequestExpose(GDrawableGetWindow(GWidgetGetControl(d->gw,CID_Fonts)),NULL,true);
    }
return( true );
}

static int OFLib_PreviewVScroll(GGadget *g,GEvent *event) {
    OFLibDlg *d = GDrawGetUserData(GGadgetGetWindow(g));
    int newpos = d->p_off_top;
    int32 sb_min, sb_max, sb_pagesize;

    if ( event->type!=et_controlevent || event->u.control.subtype != et_scrollbarchange )
return( true );

    GScrollBarGetBounds(event->u.control.g,&sb_min,&sb_max,&sb_pagesize);
    switch( event->u.control.u.sb.type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= 9*sb_pagesize/10;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
        newpos += 9*sb_pagesize/10;
      break;
      case et_sb_bottom:
        newpos = (sb_max-sb_pagesize);
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = event->u.control.u.sb.pos;
      break;
    }
    if ( newpos>(sb_max-sb_pagesize) )
        newpos = (sb_max-sb_pagesize);
    if ( newpos<0 ) newpos = 0;
    if ( newpos!=d->p_off_top ) {
	int diff = newpos - d->p_off_top;
	d->p_off_top = newpos;
	GScrollBarSetPos(event->u.control.g,newpos);
	GDrawScroll(GDrawableGetWindow(GWidgetGetControl(d->gw,CID_PreviewW)),NULL,
		0,diff);
    }
return( true );
}

static int OFLib_PreviewHScroll(GGadget *g,GEvent *event) {
    OFLibDlg *d = GDrawGetUserData(GGadgetGetWindow(g));
    int newpos = d->p_off_left;
    int32 sb_min, sb_max, sb_pagesize;

    if ( event->type!=et_controlevent || event->u.control.subtype != et_scrollbarchange )
return( true );

    GScrollBarGetBounds(event->u.control.g,&sb_min,&sb_max,&sb_pagesize);
    switch( event->u.control.u.sb.type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= 9*sb_pagesize/10;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
        newpos += 9*sb_pagesize/10;
      break;
      case et_sb_bottom:
        newpos = (sb_max-sb_pagesize);
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = event->u.control.u.sb.pos;
      break;
    }
    if ( newpos>(sb_max-sb_pagesize) )
        newpos = (sb_max-sb_pagesize);
    if ( newpos<0 ) newpos = 0;
    if ( newpos!=d->p_off_left ) {
	int diff = newpos - d->p_off_left;
	d->p_off_left = newpos;
	GScrollBarSetPos(event->u.control.g,newpos);
	GDrawScroll(GDrawableGetWindow(GWidgetGetControl(d->gw,CID_PreviewW)),NULL,
		-diff,0);
    }
return( true );
}

static void OFLibClearSel(OFLibDlg *d) {
    int i;
    struct ofl_download_urls *du;

    for ( i=0; i<d->all.fcnt; ++i ) {
	d->all.fonts[i].selected = false;
	for ( du = d->all.fonts[i].urls; du!=NULL; du=du->next )
	    du->selected = false;
    }
}

static int OFLib_FindLine(OFLibDlg *d,int line, struct ofl_download_urls **du ) {
    int l=0, i;

    for ( i=0; i<d->scnt; ++i ) {
	if ( l++ == line ) {
	    *du = NULL;
return( i );
	}
	if ( d->show[i]->open ) {
	    struct ofl_download_urls *urls;
	    for ( urls = d->show[i]->urls; urls!=NULL; urls = urls->next ) {
		if ( l++ == line ) {
		    *du = urls;
return( i );
		}
	    }
	}
    }
    *du = NULL;
return( -1 );
}
	
static int oflib_fonts_e_h(GWindow gw, GEvent *event) {
    OFLibDlg *d = GDrawGetUserData(gw);

    if ( event->type==et_resize ) {
	GRect size;
	GDrawGetSize(gw,&size);
	d->lheight = size.height; d->lwidth = size.width;
	OFLibSetSb(d);
	GDrawRequestExpose(gw,NULL,true);
    } else if ( event->type==et_expose ) {
	int i,x;
	struct ofl_download_urls *du;
	int index = OFLib_FindLine(d,d->l_off_top,&du);
	GRect r, box;
	char buffer[80];

	r.x = r.y = 0; r.width = d->lwidth-1; r.height = d->lheight-1;
	GDrawDrawRect(gw,&r,0x000000);

	GDrawSetFont(gw,d->font);
	for ( i=0; i<d->lsize; ++i ) {
	    if ( index==-1 || index>=d->scnt )
	break;
	    r.x = d->margin; r.width = d->lwidth-2*d->margin;
	    r.y = i*d->fh + d->margin; r.height = d->fh;

	    if ( du==NULL ) {
		/* Ignore selection on top level if it is open */
		if ( d->show[index]->selected && !d->show[index]->open )
		    GDrawFillRect(gw,&r,ACTIVE_BORDER);

		box.x = d->margin; box.width = (d->as&~1);
		box.y = d->margin+(i)*d->fh; box.height = box.width;
		GDrawDrawRect(gw,&box,0x000000);
		if ( d->show[index]->urls!=NULL ) {
		    GDrawDrawLine(gw,box.x+2,box.y+(box.height/2), box.x+box.width-2,box.y+(box.height/2), 0x000000);
		    if ( !d->show[index]->open )
			GDrawDrawLine(gw,box.x+(box.width/2),box.y+2, box.x+(box.width/2),box.y+box.height-2, 0x000000);
		}
		x = box.x + d->fh;
		x += GDrawDrawText8(gw,x, box.y+d->as,
			d->show[index]->name,-1,MAIN_FOREGROUND);
		if ( x< box.x + d->fh + 10*d->fh )
		    x = box.x + d->fh + 11*d->fh;
		else
		    x += d->as;
		x += GDrawDrawText8(gw,x, box.y+d->as,
			d->show[index]->author,-1,MAIN_FOREGROUND);
		if ( x< box.x + d->fh + 17*d->fh )
		    x = box.x + d->fh + 18*d->fh;
		else
		    x += d->as;
		x += GDrawDrawText8(gw,x, box.y+d->as,
			d->show[index]->license==ofll_ofl?"OFL":"PD",-1,MAIN_FOREGROUND);
		if ( x< box.x + d->fh + 20*d->fh )
		    x = box.x + d->fh + 21*d->fh;
		else
		    x += d->as;
		x += GDrawDrawText8(gw,x, box.y+d->as,
			d->show[index]->taglist,-1,MAIN_FOREGROUND);
		if ( x< box.x + d->fh + 42*d->fh )
		    x = box.x + d->fh + 43*d->fh;
		else
		    x += d->as;
		formatOFLibDate(buffer,sizeof(buffer), d->show[index]->date );
		x += GDrawDrawText8(gw,x, box.y+d->as,
			buffer,-1,MAIN_FOREGROUND);
		if ( d->show[index]->open && d->show[index]->urls!=NULL )
		    du = d->show[index]->urls;
		else
		    ++index;
	    } else {
		if ( du->selected )
		    GDrawFillRect(gw,&r,ACTIVE_BORDER);
		GDrawDrawText8(gw,r.x + d->fh, r.y+d->as,
			du->comment,-1,MAIN_FOREGROUND);
		if ( du->next!=NULL )
		    du = du->next;
		else {
		    ++index;
		    du = NULL;
		}
	    }
	}
    } else if ( event->type==et_mousedown ) {
	int l = (event->u.mouse.y-d->margin)/d->fh;
	int index;
	struct ofl_download_urls *du;

	if ( l<0 || l>=d->lsize || event->u.mouse.x<d->margin || event->u.mouse.x>=d->lwidth-d->margin )
return( true );
	index = OFLib_FindLine(d,d->l_off_top+l,&du);
	if ( index<0 || index>=d->scnt )
return( true );
	if ( du==NULL ) {
	    if ( event->u.mouse.x<=d->as ) {
		d->show[index]->open = !d->show[index]->open;
		OFLibSetSb(d);
		OFLibEnableButtons(d);
		GGadgetRedraw( GWidgetGetControl(d->gw,CID_Fonts));
return( true );
	    } else if ( event->u.mouse.state&(ksm_shift|ksm_alt) ) {
		d->show[index]->selected = !d->show[index]->selected;
	    } else {
		OFLibClearSel(d);
		d->show[index]->selected = true;
	    }
	} else {
	    if ( event->u.mouse.state&(ksm_shift|ksm_alt) ) {
		du->selected = !du->selected;
	    } else {
		OFLibClearSel(d);
		du->selected = true;
	    }
	}
	OFLibEnableButtons(d);
	GGadgetRedraw( GWidgetGetControl(d->gw,CID_Fonts));
    }
return( true );
}

static int oflib_preview_e_h(GWindow gw, GEvent *event) {
    OFLibDlg *d = GDrawGetUserData(gw);

    if ( event->type==et_resize ) {
	GRect size;
	GDrawGetSize(gw,&size);
	d->pheight = size.height; d->pwidth = size.width;
	if ( d->preview_image!=NULL ) {
	    GGadget *hsb = GWidgetGetControl(d->gw,CID_PreviewHSB);
	    GGadget *vsb = GWidgetGetControl(d->gw,CID_PreviewVSB);
	    GScrollBarSetBounds(hsb,0,GImageGetWidth(d->preview_image),d->pwidth);
	    GScrollBarSetBounds(vsb,0,GImageGetHeight(d->preview_image),d->pheight);
	}
    } else if ( event->type==et_expose && d->preview_image ) {
	GRect src;
	GRect *exp = &event->u.expose.rect;
	src.x = d->p_off_left+exp->x; src.y = d->p_off_top+exp->y;
	src.width = d->pwidth-exp->x; src.height = d->pheight-exp->y;
	src.width = GImageGetWidth(d->preview_image)-exp->x;
	src.height = GImageGetHeight(d->preview_image)-exp->y;
	if ( src.width>exp->width )
	    src.width = exp->width;
	if ( src.height>exp->height )
	    src.height = exp->height;
	GDrawDrawImage(gw,d->preview_image,&src,exp->x,exp->y);
    }
return( true );
}

void OFLibBrowse(void) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[20], boxes[9];
    GGadgetCreateData *hvarray[3][3], *harray1[7], *harray2[6], *harray3[4],
	    *barray[10], *varray[30], *hackarray[6];
    GTextInfo label[20];
    int j,k,l,fontlistl;
    FontRequest rq;
    int as, ds, ld;
    static GFont *oflibfont=NULL;

    if ( active!=NULL ) {
	GDrawSetVisible(active->gw,true);
	GDrawRaise( active->gw );
return;
    }
    if ( !initted )
	OFLibInit();

    active = gcalloc(1,sizeof(OFLibDlg));
    active->background_preview_requests = oflib_automagic_preview;
    pthread_key_create(&jump_key,NULL);
    LoadOFLibState(&active->all);

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    /*wattrs.mask |= wam_restrict;*/
    /*wattrs.restrict_input_to_me = 0;*/
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Browse the Open Font Library");
    pos.x = pos.y = 0;
    pos.width = 400;
    pos.height = 400;
    active->gw = gw = GDrawCreateTopWindow(NULL,&pos,oflib_e_h,active,&wattrs);

    if ( oflibfont==NULL ) {
	memset(&rq,0,sizeof(rq));
	rq.utf8_family_name = SANS_UI_FAMILIES;
	rq.point_size = 12;
	rq.weight = 400;
	oflibfont = GDrawInstanciateFont(gw,&rq);
	oflibfont = GResourceFindFont("OFLib.Font",oflibfont);
    }
    active->font = oflibfont;
    GDrawWindowFontMetrics(active->gw,active->font,&as,&ds,&ld);
    active->as = as; active->fh = as+ds;

    memset(&boxes,0,sizeof(boxes));
    memset(&gcd,0,sizeof(gcd));
    memset(&label,0,sizeof(label));
    j=k=l=0;

    label[k].text = (unichar_t *) _("Sort by:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].creator = GLabelCreate;
    harray1[0] = &gcd[k++];

    label[k].text = (unichar_t *) _("Date");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid   = CID_SortDate;
    gcd[k].gd.handle_controlevent = OFL_SortBy;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_cb_on;
    gcd[k].creator = GRadioCreate;
    harray1[1] = &gcd[k++];

    label[k].text = (unichar_t *) _("Designer");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid   = CID_SortAuthor;
    gcd[k].gd.handle_controlevent = OFL_SortBy;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].creator = GRadioCreate;
    harray1[2] = &gcd[k++];

    label[k].text = (unichar_t *) _("Name");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid   = CID_SortName;
    gcd[k].gd.handle_controlevent = OFL_SortBy;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].creator = GRadioCreate;
    harray1[3] = &gcd[k++];

    label[k].text = (unichar_t *) _("Reverse");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid   = CID_SortReverse;
    gcd[k].gd.handle_controlevent = OFL_SortBy;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_cb_on;
    gcd[k].creator = GCheckBoxCreate;
    harray1[4] = &gcd[k++]; harray1[5] = GCD_Glue; harray1[6] = NULL;

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = harray1;
    boxes[2].creator = GHBoxCreate;
    varray[l++] = &boxes[2]; varray[l++] = NULL;

    label[k].text = (unichar_t *) _("Search:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].creator = GLabelCreate;
    harray2[0] = &gcd[k++];

    gcd[k].gd.cid   = CID_SearchType;
    gcd[k].gd.u.list = searchtypes;
    gcd[k].gd.handle_controlevent = OFL_SearchFor;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].creator = GListButtonCreate;
    harray2[1] = &gcd[k++];

    gcd[k].gd.cid   = CID_SearchString;
    gcd[k].gd.handle_controlevent = OFL_SearchStringChanged;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].creator = GTextFieldCreate;
    harray2[2] = &gcd[k++]; harray2[3] = GCD_Glue; harray2[4] = NULL;

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = harray2;
    boxes[3].creator = GHBoxCreate;
    varray[l++] = &boxes[3]; varray[l++] = NULL;

    label[k].text = (unichar_t *) _("Fonts on http://openfontlibrary.org/");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].creator = GLabelCreate;
    varray[l++] = &gcd[k++]; varray[l++] = NULL;

    gcd[k].gd.pos.height = 150;
    gcd[k].gd.pos.width  = 400;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.u.drawable_e_h = oflib_fonts_e_h;
    gcd[k].gd.cid = CID_Fonts;
    gcd[k].creator = GDrawableCreate;
    harray3[0] = &gcd[k++];

    gcd[k].gd.pos.height = 150;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_sb_vert;
    gcd[k].gd.cid = CID_FontSB;
    gcd[k].gd.handle_controlevent = OFLib_FontsVScroll;
    gcd[k].creator = GScrollBarCreate;
    harray3[1] = &gcd[k++]; harray3[2] = NULL;

    boxes[4].gd.flags = gg_enabled|gg_visible;
    boxes[4].gd.u.boxelements = harray3;
    boxes[4].creator = GHBoxCreate;
    hackarray[0] = &boxes[4]; hackarray[1] = NULL;

    gcd[k].gd.pos.height = 150;
    gcd[k].gd.flags = gg_enabled;
    gcd[k].gd.u.drawable_e_h = oflib_preview_e_h;
    gcd[k].gd.cid = CID_PreviewW;
    gcd[k].creator = GDrawableCreate;
    hvarray[0][0] = &gcd[k++];

    gcd[k].gd.pos.height = 150;
    gcd[k].gd.flags = gg_enabled | gg_sb_vert;
    gcd[k].gd.cid = CID_PreviewVSB;
    gcd[k].gd.handle_controlevent = OFLib_PreviewVScroll;
    gcd[k].creator = GScrollBarCreate;
    hvarray[0][1] = &gcd[k++]; hvarray[0][2] = NULL;

    gcd[k].gd.pos.width = 150;
    gcd[k].gd.flags = gg_enabled;
    gcd[k].gd.cid = CID_PreviewHSB;
    gcd[k].gd.handle_controlevent = OFLib_PreviewHScroll;
    gcd[k].creator = GScrollBarCreate;
    hvarray[1][0] = &gcd[k++]; hvarray[1][1] = GCD_Glue; hvarray[1][2] = NULL;
    hvarray[2][0] = NULL;

    boxes[5].gd.flags = gg_enabled|gg_visible;
    boxes[5].gd.u.boxelements = hvarray[0];
    boxes[5].creator = GHVBoxCreate;
    hackarray[1] = &boxes[5]; hackarray[2] = NULL;

    boxes[6].gd.flags = gg_enabled|gg_visible;
    boxes[6].gd.u.boxelements = hackarray;
    boxes[6].creator = GVBoxCreate;
    fontlistl = l;
    varray[l++] = &boxes[6]; varray[l++] = NULL;

    label[k].text = (unichar_t *) _("Automatically download preview after selecting a font (be patient)");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid   = CID_BackgroundPreview;
    gcd[k].gd.handle_controlevent = OFL_BackgroundPreviewChange;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    if ( oflib_automagic_preview )
	gcd[k].gd.flags = gg_visible | gg_enabled | gg_cb_on;
    gcd[k].creator = GCheckBoxCreate;
    varray[l++] = &gcd[k++]; varray[l++] = NULL;

    label[k].text = (unichar_t *) _("Open Font");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid   = CID_Open;
    gcd[k].gd.handle_controlevent = OFL_Open;
    gcd[k].gd.flags = gg_visible;
    gcd[k].creator = GButtonCreate;
    barray[0] = GCD_Glue; barray[1] = &gcd[k++]; barray[2] = GCD_Glue;

    label[k].text = (unichar_t *) _("Preview");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid   = CID_Preview;
    gcd[k].gd.handle_controlevent = OFL_Preview;
    gcd[k].gd.flags = gg_visible;
    gcd[k].creator = GButtonCreate;
    barray[3] = GCD_Glue; barray[4] = &gcd[k++]; barray[5] = GCD_Glue;

    label[k].text = (unichar_t *) _("Done");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = OFL_Done;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].creator = GButtonCreate;
    barray[6] = GCD_Glue; barray[7] = &gcd[k++]; barray[8] = GCD_Glue;
    barray[9] = NULL;

    boxes[7].gd.flags = gg_enabled|gg_visible;
    boxes[7].gd.u.boxelements = barray;
    boxes[7].creator = GHBoxCreate;
    varray[l++] = &boxes[7]; varray[l++] = NULL;
    varray[l++] = NULL;

    if ( l>=sizeof(varray)/sizeof(varray[0]) )
	IError("varray too small");
    if ( k>=sizeof(gcd)/sizeof(gcd[0]) )
	IError("gcd too small");
    if ( k>=sizeof(label)/sizeof(label[0]) )
	IError("label too small");

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = varray;
    boxes[0].gd.cid = CID_TopBox;
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(gw,boxes);
    GHVBoxSetExpandableRow(boxes[0].ret,fontlistl/2);
    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[3].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[4].ret,0);
    GHVBoxSetExpandableCol(boxes[5].ret,0);
    GHVBoxSetExpandableRow(boxes[5].ret,0);
    GHVBoxSetExpandableCol(boxes[7].ret,gb_expandglue);
    GHVBoxFitWindow(boxes[0].ret);

    active->margin = 2;

    if ( active->all.fcnt!=0 )
	OFLibSortSearch(active);
    active->http_timer = GDrawRequestTimer(gw,10,300,NULL);
    /* No timeout on this notice, we will kill it ourselves when we are done */
    /*  (or the user can when s/he wants to) */
    gwwv_post_notice_timeout(-1,_("Checking for new fonts"),_("Checking server for additional fonts"));
    GDrawSetVisible(gw,true);
}
#endif
