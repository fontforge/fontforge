/* Copyright (C) 2001 by George Williams */
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
#include "ttfmodui.h"
#include <ustring.h>
#include <utype.h>
#include <gfile.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

static FILE *makebackup(TtfFile *ttf, int linkcnt) {
    /* Traditionally we only make a backup on the first save. But due to my */
    /*  data structures I need to make a backup before each save */
    /* (while I'm saving the current file I still need some place where I can */
    /*  look up the data which are referenced by pointers into that file. */
    /* If we return successfully then ttf->file points to the backup file */
    /*  (which has all the right offsets), and returns a FILE pointing to */
    /*  the file to be saved */
    struct stat info;
    char *backupname;
    int len;
    FILE *backup, *temp;
    char *buffer;

    backupname = galloc((len = strlen(ttf->filename))+2);
    strcpy(backupname,ttf->filename);
    backupname[len] = ttf->backedup?'+':'~';
    backupname[len+1]= '\0';

    if ( linkcnt==-1 ) {
	fstat(fileno(ttf->file),&info);
	linkcnt = info.st_nlink;
    }
    if ( linkcnt==1 ) {
	int ret = rename(ttf->filename,backupname);
	free(backupname);
	if ( ret==-1 ) {
	    GWidgetErrorR(_STR_NoBackup,_STR_NoBackup);
return( NULL );
	}
	temp = fopen(ttf->filename,"w+");
	if ( temp==NULL ) {
	    GWidgetErrorR(_STR_CouldntReopenFile,_STR_CouldntReopenFile);
exit( 1 );
	}
return( temp );
    }

    backup = fopen(backupname,"w+");
    free(backupname);
    if ( backup==NULL ) {
	GWidgetErrorR(_STR_NoBackup,_STR_NoBackup);
return( NULL );
    }

    buffer = galloc(8192);
    rewind(ttf->file);
    while ( (len=read(fileno(ttf->file),buffer,8192))>0 ) {
	if ( write(fileno(backup),buffer,len)!=len ) {
	    len = -1;
    break;
	}
    }
    free(buffer);
    if ( len<0 ) {
	GWidgetErrorR(_STR_CouldntWriteBackup,_STR_CouldntWriteBackup);
return( NULL );
    }
    temp = ttf->file;
    ttf->file = backup;
    lseek(fileno(temp),0,SEEK_SET);
    ftruncate(fileno(temp),0);

return( temp );
}

static void RestoreFromBackup(TtfFile *ttf,int linkcnt) {
    FILE *orig;
    char *backupname;
    struct stat info;
    int len;
    char *buffer;

    backupname = galloc((len = strlen(ttf->filename))+2);
    strcpy(backupname,ttf->filename);
    backupname[len] = ttf->backedup?'+':'~';
    backupname[len+1]= '\0';

    if ( linkcnt==-1 ) {
	stat(ttf->filename,&info);
	linkcnt = info.st_nlink;
    }
    if ( linkcnt==1 ) {
	if ( rename(backupname,ttf->filename)==-1 ) {
	    GWidgetErrorR(_STR_SaveFailed,_STR_CantRecoverFromSave,GFileNameTail(backupname));
exit(1);
	}
	free(backupname);
    } else {
	orig = fopen(ttf->filename,"w+");
	if ( orig==NULL ) {
	    GWidgetErrorR(_STR_SaveFailed,_STR_CantRecoverFromSave,GFileNameTail(backupname));
exit(1);
	}

	buffer = galloc(8192);
	rewind(ttf->file);
	while ( (len=read(fileno(ttf->file),buffer,8192))>0 ) {
	    if ( write(fileno(orig),buffer,len)!=len ) {
		len = -1;
	break;
	    }
	}
	free(buffer);
	if ( len<0 ) {
	    GWidgetErrorR(_STR_SaveFailed,_STR_CantRecoverFromSave,GFileNameTail(backupname));
exit(1);
	}
	fclose( ttf->file );
	ttf->file = orig;
	lseek(fileno(orig),0,SEEK_SET);
    }
}

static int32 filecheck(FILE *file) {
    uint32 sum = 0, chunk;

    rewind(file);
    while ( 1 ) {
	chunk = getlong(file);
	if ( feof(file))
    break;
	sum += chunk;
    }
return( sum );
}

static int32 figurecheck(FILE *file,int32 start, int32 lcnt) {
    uint32 sum = 0, chunk;

    fseek(file,start,SEEK_SET);
    while ( --lcnt>=0 ) {
	chunk = getlong(file);
	if ( feof(file))
    break;
	sum += chunk;
    }
return( sum );
}

static void ttfdumpfontheader(FILE *newf, TtfFont *font) {
    int bit, i;
    Table *tab;

    font->version_pos = ftell(newf);
    putlong(newf,font->version);
    putshort(newf,font->tbl_cnt);
    for ( i= -1, bit = 1; bit<font->tbl_cnt; bit<<=1, ++i );
    bit>>=1;
    putshort(newf,bit*16);
    putshort(newf,i);
    putshort(newf,(font->tbl_cnt-bit)*16);
    for ( i=0; i<font->tbl_cnt; ++i ) {
	tab = font->tbls[i];
	putlong(newf,tab->name);
	putlong(newf,tab->newchecksum);
	putlong(newf,tab->newstart);
	putlong(newf,tab->newlen);
    }
}

static int tcomp(const void *_t1, const void *_t2) {
    Table *t1 = *((Table **) _t1), *t2 = *((Table **) _t2);
return( t1->orderingval - t2->orderingval );
}

static void ttfdumpfonttables(FILE *newf,TtfFile *ttf) {
    int i, j, cnt, bsize, len, tot;
    char *buf=NULL;
    FILE *old = ttf->file;
    Table *tab, **ordered;

    for ( i=cnt=0; i<ttf->font_cnt; ++i ) cnt += ttf->fonts[i]->tbl_cnt;
    ordered = galloc(cnt*sizeof(Table *));
    for ( i=cnt=0; i<ttf->font_cnt; ++i ) {
	for ( j=0; j<ttf->fonts[i]->tbl_cnt; ++j ) {
	    tab = ttf->fonts[i]->tbls[j];
	    if ( !tab->inserted ) {
		ordered[cnt++] = tab;
		tab->inserted = true;
	    }
	}
    }
    qsort(ordered,cnt,sizeof(Table *),tcomp);

    for ( i=0; i<cnt; ++i ) {
	tab = ordered[i];
	if ( tab->newstart!=0 )		/* Save by some earlier font in a ttc */
    continue;			/* Don't save again */
	tab->newstart = ftell(newf);
	tab->newchecksum = 0;
	if ( tab->td_changed )
	    (tab->write_tabledata)(newf,tab);
	else if ( tab->data )
	    fwrite(tab->data,1,tab->newlen,newf);
	else {
	    /* I wonder... Are modern buses limited to transfers of 32768 the */
	    /*  way the dear old unibus was? I'm probably way out of date here*/
	    bsize = tab->len<=32768? tab->len : 32768;
	    if ( buf==NULL )
		buf = galloc(32768);
	    fseek(old,tab->start,SEEK_SET);
	    tot = tab->len;
	    while ( (len = fread(buf,1,bsize,old))>0 ) {
		fwrite(buf,1,len,newf);
		tot -= len;
		if ( tot==0 )
	    break;
		if ( bsize>tot ) bsize = tot;
	    }
	    tab->newchecksum = tab->oldchecksum;
	}
	tab->newlen = ftell(newf)-tab->newstart;
	if ( tab->newlen&1 )
	    putc('\0',newf);
	if ( (tab->newlen+1)&2 )
	    putshort(newf,0);
    }
    free(buf);
    for ( i=0; i<cnt; ++i ) {
	tab = ordered[i];
	if ( tab->newchecksum!=0 )
    continue;
	tab->newchecksum = figurecheck(newf,tab->newstart,(tab->newlen+3)>>2);
    }

    free(ordered);
}

static void ttfwrite(FILE *newf, TtfFile *ttf, TtfFont *font) {
    ttfdumpfontheader(newf,font);		/* Placeholder */
    ttfdumpfonttables(newf,ttf);
    fseek(newf,0,SEEK_SET);
    ttfdumpfontheader(newf,font);		/* Fillin */
}

static void ttcwrite(FILE *newf, TtfFile *ttf) {
    int32 pos;
    int i;

    putlong(newf,CHR('t','t','c','f'));
    putlong(newf,ttf->font_cnt);
    for ( i=0; i<ttf->font_cnt; ++i )
	putlong(newf,0);				/* Placeholder */
    for ( i=0; i<ttf->font_cnt; ++i )
	ttfdumpfontheader(newf,ttf->fonts[i]);		/* Also Placeholders */
    pos = ftell(newf);
    fseek(newf,2*sizeof(int32),SEEK_SET);
    for ( i=0; i<ttf->font_cnt; ++i )
	putlong(newf,ttf->fonts[i]->version_pos);	/* Fill in first set of placeholders */
    fseek(newf,pos,SEEK_SET);
    ttfdumpfonttables(newf,ttf);
    fseek(newf,ttf->fonts[0]->version_pos,SEEK_SET);
    for ( i=0; i<ttf->font_cnt; ++i )
	ttfdumpfontheader(newf,ttf->fonts[i]);		/* Fill in final set */
}

int TtfSave(TtfFile *ttf,char *newpath) {
    int same = false;
    struct stat old, new;
    FILE *newf;
    int linkcnt = -1;
    uint32 date[2];
    time_t now;
    Table *tab, *a_head=NULL;
    int i,j;
    int32 checksum;

    if ( strcmp(ttf->filename,newpath)==0 )
	same = true;
    else {	/* things will get really screwed up if I save onto another link to myself */
	fstat(fileno(ttf->file),&old);
	stat(newpath,&new);
	if ( new.st_dev==old.st_dev && new.st_ino==old.st_ino )
	    same = true;
	linkcnt = old.st_nlink;
    }

    if ( same ) {
	newf = makebackup(ttf,linkcnt);
	if ( newf==NULL )
return( false );
    } else {
	newf = fopen(newpath, "w+");
	if ( newf==NULL ) {
	    GWidgetErrorR(_STR_CantCreate,_STR_CantCreateFile, GFileNameTail(newpath));
return( false );
	}
    }

    /* Mark all tables as unsaved */
    for ( i=0; i<ttf->font_cnt; ++i )
	for ( j=0; j<ttf->fonts[i]->tbl_cnt; ++j ) {
	    Table *tab = ttf->fonts[i]->tbls[j];
	    tab->newstart = 0;
	    tab->inserted = 0;
/* This is the ordering of tables in ARIAL. I've no idea why it makes a */
/*  difference to order them, time to do a seek seems likely to be small, but */
/*  other people make a big thing about ordering them so I'll do it. */
/* I got bored after glyph. Interestingly enough, Adobe follows the same */
/*  scheme for their otf fonts */
	    tab->orderingval = tab->name==CHR('h','e','a','d')? 1 :
			       tab->name==CHR('h','h','e','a')? 2 :
			       tab->name==CHR('m','a','x','p')? 3 :
			       tab->name==CHR('O','S','/','2')? 4 :
			       tab->name==CHR('g','a','s','p')? 5 :
			       tab->name==CHR('n','a','m','e')? 6 :
			       tab->name==CHR('c','m','a','p')? 7 :
			       tab->name==CHR('l','o','c','a')? 8 :
			       tab->name==CHR('C','F','F',' ')? 8 :
			       tab->name==CHR('L','T','S','H')? 9 :
			       tab->name==CHR('V','D','M','X')? 10 :
			       tab->name==CHR('p','r','e','p')? 11 :
			       tab->name==CHR('f','p','g','m')? 12 :
			       tab->name==CHR('c','v','t',' ')? 13 :
			       tab->name==CHR('h','m','t','x')? 14 :
			       tab->name==CHR('h','m','d','x')? 15 :
			       tab->name==CHR('g','l','y','f')? 16 :
			       17;
       }

    /* Zero out the checksumadjust field of all headers, and set the modification time to now */
    time(&now);
    TimeTToQuad(now,date);
    for ( i=0; i<ttf->font_cnt; ++i ) {
	for ( j=0; j<ttf->fonts[i]->tbl_cnt; ++j ) {
	    tab = ttf->fonts[i]->tbls[j];
	    if ( tab->name==CHR('h','e','a','d')) {
		TableFillup(tab);
		ptputlong(tab->data+8,0);
		ptputlong(tab->data+28,date[0]);
		ptputlong(tab->data+32,date[1]);
		if ( a_head==NULL ) a_head = tab;
	    }
	}
    }

    if ( ttf->is_ttc )
	ttcwrite(newf,ttf);
    else
	ttfwrite(newf,ttf,ttf->fonts[0]);

    if ( a_head!=NULL ) {
	checksum = filecheck(newf);
	if ( ttf->is_ttc )
	    checksum = 0xdcd07d3e - checksum;
	else
	    checksum = 0xb1b0afba - checksum;
	fseek(newf,a_head->newstart+2*sizeof(int32),SEEK_SET);
	putlong(newf,checksum);
	ptputlong(a_head->data+2*sizeof(int32),checksum);
    }
    fflush(newf);
    if ( ferror(newf)) {
	fclose(newf);
	GWidgetErrorR(_STR_SaveFailed,_STR_SaveFailedOn,GFileNameTail(newpath));
	RestoreFromBackup(ttf,linkcnt);
return( false );
    }

    fclose(ttf->file);
    ttf->file = newf;

    for ( i=0; i<ttf->font_cnt; ++i ) {
	for ( j=0; j<ttf->fonts[i]->tbl_cnt; ++j ) {
	    tab = ttf->fonts[i]->tbls[j];
	    tab->start = tab->newstart;
	    tab->len = tab->newlen;
	    tab->oldchecksum = tab->newchecksum;
	    if ( tab->name==CHR('h','e','a','d'))
		headViewUpdateModifiedCheck(tab);	/* Redisplay modified, checksumadjust fields */
	    if ( tab->td_changed ) {
		free(tab->data); tab->data=NULL;	/* it will be wrong */
		tab->td_changed = false;
	    }
	    tab->changed = false;
	}
    }
    ttf->changed = false;
    ttf->backedup = false;
return(true);
}
