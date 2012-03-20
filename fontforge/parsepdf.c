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
#include "fontforge.h"
#include <chardata.h>
#include <utype.h>
#include <ustring.h>
#include <math.h>
#include <locale.h>
#include <gwidget.h>
#include "psfont.h"
#include "sd.h"

#ifdef HAVE_IEEEFP_H
# include <ieeefp.h>		/* Solaris defines finite in ieeefp rather than math.h */
#endif

struct pdfcontext {
    char *tokbuf;
    int tblen;
    FILE *pdf;
    FILE *compressed;
    struct psdict pdfdict;
    long *objs;
    long *subindex;
    int ocnt;
    long *fontobjs;
    char **fontnames;		/* theoretically in utf-8 */
    long *cmapobjs;
    int *cmap_from_cid;
    int fcnt;
    enum openflags openflags;
    int encrypted;
    int enc_dict;
    int pcnt;
    long *pages;
    int root;
};

static long FindXRef(FILE *pdf) {
    int ch;
    long xrefpos;

    fseek(pdf,-5-2-8-2-10-2,SEEK_END);
    forever {
	while ( (ch=getc(pdf))!=EOF ) {
	    if ( ch=='s' )
	break;
	}
	if ( ch==EOF )
return( -1 );
	while ( ch=='s' ) {
	    if ( (ch=getc(pdf))!='t' )
	continue;
	    if ( (ch=getc(pdf))!='a' )
	continue;
	    if ( (ch=getc(pdf))!='r' )
	continue;
	    if ( (ch=getc(pdf))!='t' )
	continue;
	    if ( (ch=getc(pdf))!='x' )
	continue;
	    if ( (ch=getc(pdf))!='r' )
	continue;
	    if ( (ch=getc(pdf))!='e' )
	continue;
	    if ( (ch=getc(pdf))!='f' )
	continue;
	    if ( fscanf(pdf,"%ld",&xrefpos)!=1 )
return( -1 );

return( xrefpos );
	}
    }
}

static int findkeyword(FILE *pdf, char *keyword, char *end) {
    char buffer[60];
    int len = strlen( keyword );
    int end_len = end==NULL ? 0 : strlen(end);
    int ch, i;

    for ( i=0; i<len; ++i )
	buffer[i] = ch = getc(pdf);
    if ( ch==EOF )
return( false );
    buffer[i] = 0;
    forever {
	if ( strcmp(buffer,keyword)==0 )
return( true );
	if ( strncmp(buffer,end,end_len)==0 )
return( false );
	for ( i=1; i<len; ++i )
	    buffer[i-1] = buffer[i];
	buffer[len-1] = ch = getc(pdf);
	if ( ch==EOF )
return( false );
    }
}

static int seektrailer(FILE *pdf, int *start, int *num, struct pdfcontext *pc) {
    int prev_xref;
    int pos;

    if ( !findkeyword(pdf,"trailer",NULL))
return( false );
    pos = ftell(pdf);
    if ( findkeyword(pdf,"/Encrypt",">>") ) {
	int bar;
	if ( fscanf( pdf, "%d %d", &pc->enc_dict, &bar )==2 )
	    pc->encrypted = true;
    }
    if ( pc->root == 0 ) {
	fseek(pdf,pos,SEEK_SET);
	if ( findkeyword(pdf,"/Root",">>") ) {
	    int bar;
	    fscanf( pdf, "%d %d", &pc->root, &bar );
	}
    }
    fseek(pdf,pos,SEEK_SET);
    if ( !findkeyword(pdf,"/Prev",">>"))
return( false );
    if ( fscanf( pdf, "%d", &prev_xref )!=1 )
return( false );
    fseek(pdf, prev_xref, SEEK_SET );
    if ( fscanf(pdf, "xref %d %d", start, num )!=2 )
return( false );

return( true );
}

static long *FindObjectsFromXREFObject(struct pdfcontext *pc, long prev_xref);

static long *FindObjects(struct pdfcontext *pc) {
    FILE *pdf = pc->pdf;
    long xrefpos = FindXRef(pdf);
    long *ret=NULL;
    int *gen=NULL;
    int cnt = 0, i, start, num, ch;
    long offset; int gennum; char f;

    if ( xrefpos == -1 )
return( NULL );
    fseek(pdf, xrefpos,SEEK_SET );

    if ( fscanf(pdf, "xref %d %d", &start, &num )!=2 ) {
	int foo, bar;
	fseek(pdf, xrefpos,SEEK_SET );
	if ( fscanf(pdf, "%d %d", &foo, &bar )!=2 )
return( NULL );
	while ( isspace(ch=getc(pdf)));
	if ( ch!='o' )
return( NULL );
	if ( getc(pdf)!='b' )
return( NULL );
	if ( getc(pdf)!='j' )
return( NULL );
	if ( !isspace(getc(pdf)) )
return( NULL );

return( FindObjectsFromXREFObject(pc,xrefpos));
    }

    forever {
	if ( start+num>cnt ) {
	    ret = grealloc(ret,(start+num+1)*sizeof(long));
	    memset(ret+cnt,-1,sizeof(long)*(start+num-cnt));
	    gen = grealloc(gen,(start+num)*sizeof(int));
	    memset(gen+cnt,-1,sizeof(int)*(start+num-cnt));
	    cnt = start+num;
	    pc->ocnt = cnt;
	    ret[cnt] = -2;
	}
	for ( i=start; i<start+num; ++i ) {
	    if ( fscanf(pdf,"%ld %d %c", &offset, &gennum, &f )!=3 ) {
		free(gen);
return( ret );
	    }
	    if ( f=='f' ) {
		if ( gennum > gen[i] ) {
		    ret[i] = -1;
		    gen[i] = gennum;
		}
	    } else if ( f=='n' ) {
		if ( gennum > gen[i] ) {
		    ret[i] = offset;
		    gen[i] = gennum;
		}
	    } else {
		free(gen);
return( ret );
		}
	}
	if ( fscanf(pdf, "%d %d", &start, &num )!=2 )
	    if ( !seektrailer(pdf, &start, &num, pc)) {
	    free(gen);
return( ret );
	}
    }
}

#define pdf_space(ch) (ch=='\0' || ch=='\t' || ch=='\n' || ch=='\r' || ch=='\f' || ch==' ' )
#define pdf_oper(ch) (ch=='(' || ch==')' || ch=='<' || ch=='>' || ch=='[' || ch==']' || ch=='{' || ch=='}' || ch=='/' || ch=='%' )

static int pdf_peekch(FILE *pdf) {
    int ch = getc(pdf);
    ungetc(ch,pdf);
return( ch );
}

static void pdf_skipwhitespace(struct pdfcontext *pc) {
    FILE *pdf =  pc->compressed ? pc->compressed : pc->pdf;
    int ch;

    forever {
	ch = getc(pdf);
	if( ch=='%' ) {
	    while ( (ch=getc(pdf))!=EOF && ch!='\n' && ch!='\r' );
	} else if ( !pdf_space(ch) )
    break;
    }
    ungetc(ch,pdf);
}

static char *pdf_getname(struct pdfcontext *pc) {
    FILE *pdf = pc->compressed ? pc->compressed : pc->pdf;
    int ch;
    char *pt = pc->tokbuf, *end = pc->tokbuf+pc->tblen;

    pdf_skipwhitespace(pc);
    ch = getc(pdf);
    if ( ch!='/' ) {
	ungetc(ch,pdf);
return( NULL );
    }
    for ( ch=getc(pdf) ;; ch=getc(pdf) ) {
	if ( pt>=end ) {
	    char *temp = grealloc(pc->tokbuf,(pc->tblen+=300));
	    pt = temp + (pt-pc->tokbuf);
	    pc->tokbuf = temp;
	    end = temp+pc->tblen;
	}
	if ( pdf_space(ch) || pdf_oper(ch) ) {
	    ungetc(ch,pdf);
	    *pt = '\0';
return( pc->tokbuf );
	}
	*pt++ = ch;
    }
}

static char *pdf_getdictvalue(struct pdfcontext *pc) {
    FILE *pdf = pc->compressed ? pc->compressed : pc->pdf;
    int ch;
    char *pt = pc->tokbuf, *end = pc->tokbuf+pc->tblen;
    int dnest=0, anest=0, strnest;

    pdf_skipwhitespace(pc);
    ch = getc(pdf);
    forever {
	if ( pt>=end ) {
	    char *temp = grealloc(pc->tokbuf,(pc->tblen+=300));
	    pt = temp + (pt-pc->tokbuf);
	    pc->tokbuf = temp;
	    end = temp+pc->tblen;
	}
	*pt++ = ch;
	if ( ch=='(' ) {
	    strnest = 0;
	    while ( (ch=getc(pdf))!=EOF ) {
		if ( pt>=end ) {
		    char *temp = grealloc(pc->tokbuf,(pc->tblen+=300));
		    pt = temp + (pt-pc->tokbuf);
		    pc->tokbuf = temp;
		    end = temp+pc->tblen;
		}
		*pt++ = ch;
		if ( ch=='(' ) ++strnest;
		else if ( ch==')' && strnest==0 )
	    break;
		else if ( ch==')' ) --strnest;
	    }
	} else if ( ch=='[' )
	    ++ anest;
	else if ( ch==']' && anest>0 )
	    -- anest;
	else if ( ch=='<' && pdf_peekch(pdf)=='<' )
	    ++dnest;
	else if ( ch=='>' && pdf_peekch(pdf)=='>' ) {
	    if ( dnest==0 ) {
		ungetc(ch,pdf);
		pt[-1] = '\0';
		if ( pt>pc->tokbuf+1 && pt[-2]==' ' ) pt[-2] = '\0';
return( pc->tokbuf );
	    }
	    --dnest;
	} else if ( ch=='/' && anest==0 && dnest==0 && pt!=pc->tokbuf+1 ) {
	    /* A name token may be a value if it is the first thing */
	    /*  otherwise it is the start of the next key */
	    ungetc(ch,pdf);
	    pt[-1] = '\0';
	    if ( pt>pc->tokbuf+1 && pt[-2]==' ' ) pt[-2] = '\0';
return( pc->tokbuf );
	} else if ( ch=='%' || pdf_space(ch) ) {
	    pt[-1] = ' ';
	    ungetc(ch,pdf);
	    pdf_skipwhitespace(pc);
	} else if ( ch==EOF ) {
	    pt[-1] = '\0';
return( pc->tokbuf );
	}
	ch = getc(pdf);
    }
}   

static void PSDictClear(struct psdict *dict) {
    int i;

    for ( i=0; i<dict->next; ++i ) {
	free(dict->keys[i]);
	free(dict->values[i]);
    }
    dict->next = 0;
}

static int pdf_readdict(struct pdfcontext *pc) {
    FILE *pdf = pc->compressed ? pc->compressed : pc->pdf;
    char *key, *value;
    int ch;

    PSDictClear(&pc->pdfdict);

    pdf_skipwhitespace(pc);
    ch = getc(pdf);
    if ( ch!='<' || pdf_peekch(pdf)!='<' )
return( false );
    getc(pdf);		/* Eat the second '<' */

    forever {
	key = copy(pdf_getname(pc));
	if ( key==NULL ) {
	    if ( pc->compressed!=NULL ) {	/* We've read the whole object*/
		fclose(pc->compressed);		/* so close the compressed */
		pc->compressed = NULL;		/* stream in which it lives */
	    }
return( true );
	}
	value = copy(pdf_getdictvalue(pc));
	if ( value==NULL || strcmp(value,"null")==0 )
	    free(key);
	else {
	    if ( pc->pdfdict.next>=pc->pdfdict.cnt ) {
		pc->pdfdict.keys = grealloc(pc->pdfdict.keys,(pc->pdfdict.cnt+=20)*sizeof(char *));
		pc->pdfdict.values = grealloc(pc->pdfdict.values,pc->pdfdict.cnt*sizeof(char *));
	    }
	    pc->pdfdict.keys  [pc->pdfdict.next] = key  ;
	    pc->pdfdict.values[pc->pdfdict.next] = value;
	    ++pc->pdfdict.next;
	}
    }
}

static void pdf_skipobjectheader(struct pdfcontext *pc) {

    fscanf( pc->pdf, "%*d %*d obj" );
}

static int hex(int ch1, int ch2) {
    int val;

    if ( ch1<='9' )
	val = (ch1-'0')<<4;
    else if ( ch1>='a' && ch1<='f' )
	val = (ch1-'a'+10)<<4;
    else
	val = (ch1-'A'+10)<<4;
    if ( ch2<='9' )
	val |= (ch2-'0');
    else if ( ch2>='a' && ch2<='f' )
	val |= (ch2-'a'+10);
    else
	val |= (ch2-'A'+10);
return( val );
}

static int pdf_getprotectedtok(FILE *stream, char *tokbuf) {
    char *pt=tokbuf, *end = tokbuf+100-2; int ch;

    while ( isspace(ch = getc(stream)) );
    while ( ch!=EOF && !isspace(ch) && ch!='[' && ch!=']' && ch!='{' && ch!='}' && ch!='<' && ch!='>' ) {
	if ( pt<end ) *pt++ = ch;
	ch = getc(stream);
    }
    if ( pt==tokbuf && ch!=EOF )
	*pt++ = ch;
    else
	ungetc(ch,stream);
    *pt='\0';
return( pt!=tokbuf?1:ch==EOF?-1: 0 );
}

static int pdf_skip_brackets(FILE *stream, char *tokbuf) {
    int ch, ret;
    
    while ( isspace(ch = getc(stream)) );
    if (ch != '<')
return 0;

    ret = pdf_getprotectedtok(stream, tokbuf);
    ch = getc(stream);

return ret && ch == '>';
}

static FILE *pdf_defilterstream(struct pdfcontext *pc);
static int pdf_getinteger(char *pt,struct pdfcontext *pc);

static int pdf_findobject(struct pdfcontext *pc, int num) {
    int first_offset, n, i, o, offset, container;
    FILE *data;
    char *pt;

    if ( pc->compressed!=NULL ) {
	fclose( pc->compressed );
	pc->compressed = NULL;
    }
    if ( num<0 || num>=pc->ocnt )
return( false );
    if ( pc->subindex==NULL || pc->subindex[num]==-1 ) {
	if ( pc->objs[num]==-1 )
return( false );
	fseek(pc->pdf,pc->objs[num],SEEK_SET);
	pdf_skipobjectheader(pc);
return( true );
    } else {
	container = pc->objs[num];
	while ( container!=-1 ) {
	    if ( pc->subindex[container]!=-1 ) {
		LogError(_("Compressed object container is itself a compressed object"));
return( false );
	    }
	    fseek(pc->pdf,pc->objs[container],SEEK_SET);
	    pdf_skipobjectheader(pc);
	    if ( !pdf_readdict(pc) )
return( false );
	    if ( (pt=PSDictHasEntry(&pc->pdfdict,"Type"))==NULL || strcmp(pt,"/ObjStm")!=0 )
return( false );
	    if ( (pt=PSDictHasEntry(&pc->pdfdict,"N"))==NULL )
return( false );
	    n = pdf_getinteger(pt,pc);
	    if ( (pt=PSDictHasEntry(&pc->pdfdict,"First"))==NULL )
return( false );
	    first_offset = pdf_getinteger(pt,pc);
	    container = -1;
	    if ( (pt=PSDictHasEntry(&pc->pdfdict,"Extends"))!=NULL )
		container = strtol(pt,NULL,0);
	    data = pdf_defilterstream(pc);
	    if ( data==NULL )
return( false );
	    rewind(data);
	    for ( i=0; i<n; ++i ) {
		fscanf( data, "%d %d", &o, &offset );
		if ( o==num )
	    break;
	    }
	    if ( i<n ) {
		fseek( data, first_offset+offset, SEEK_SET );
		pc->compressed = data;
return( true );
	    }
	    fclose(data);
	}
	/* Not found in any extents */
return( false );
    }
}

static int pdf_getdescendantfont(struct pdfcontext *pc, int num) {
    char *pt;
    int nnum;

    if ( pdf_findobject(pc,num) && pdf_readdict(pc) ) {
	if ( (pt=PSDictHasEntry(&pc->pdfdict,"Type"))!=NULL && strcmp(pt,"/Font")==0 &&
		PSDictHasEntry(&pc->pdfdict,"FontDescriptor")!=NULL &&
		(pt=PSDictHasEntry(&pc->pdfdict,"BaseFont"))!=NULL ) {
return( num );
	}
    }
    if ( (pt = pdf_getdictvalue(pc)) != NULL && sscanf(pt,"%d",&nnum) && nnum > 0 && nnum < pc->ocnt )
return( pdf_getdescendantfont(pc, nnum) );

return( -1 );
}

static int pdf_findfonts(struct pdfcontext *pc) {
    int i, j, k=0, dnum, cnum;
    char *pt, *tpt, *cmap, *desc;

    pc->fontobjs = galloc(pc->ocnt*sizeof(long));
    pc->cmapobjs = galloc(pc->ocnt*sizeof(long));
    pc->cmap_from_cid = gcalloc(pc->ocnt,sizeof(int));
    memset(pc->cmapobjs,-1,sizeof(long));
    pc->fontnames = galloc(pc->ocnt*sizeof(char *));
    /* First look for CID-keyed fonts with a pointer to a ToUnicode CMap */
    for ( i=1; i<pc->ocnt; ++i ) if ( pc->objs[i]!=-1 ) {	/* Object 0 is always unused */
	if ( pdf_findobject(pc,i) && pdf_readdict(pc) ) {
	    if ((pt=PSDictHasEntry(&pc->pdfdict,"Type"))!=NULL && strcmp(pt,"/Font")==0 &&
		    (pt=PSDictHasEntry(&pc->pdfdict,"Subtype"))!=NULL && strcmp(pt,"/Type0")==0 &&
		    (cmap=PSDictHasEntry(&pc->pdfdict,"ToUnicode"))!=NULL &&
		    (desc=PSDictHasEntry(&pc->pdfdict,"DescendantFonts"))!=NULL &&
		    (pt=PSDictHasEntry(&pc->pdfdict,"BaseFont"))!=NULL) {
	    
		if (*cmap == '[') cmap++;
		if (*desc == '[') desc++;
		sscanf(cmap, "%d", &cnum);
		sscanf(desc, "%d", &dnum);
		if ( *pt=='/' || *pt=='(' )
		    ++pt;
		tpt = copy(pt);
		
		dnum = pdf_getdescendantfont( pc,dnum );
		if ( dnum > 0 ) {
		    pc->fontobjs[k] = dnum;
		    pc->cmapobjs[k] = cnum;
		    pc->fontnames[k] = tpt;
		    /* Store a flag indicating this particular CMap comes from a CID-keyed */
		    /* font. We can't determine this later just by examining sf->subfontcnt, */
		    /* as FF flattens TTF CID fonts at the time they are loaded, so that */
		    /* they no longer look as CID-keyed fonts */
		    pc->cmap_from_cid[k] = 1;
		    k++;
		}
	    }
	}
    }

    /* List other fonts, skipping those detected at the first pass */
    for ( i=1; i<pc->ocnt; ++i ) if ( pc->objs[i]!=-1 ) {	/* Object 0 is always unused */
	if ( pdf_findobject(pc,i) && pdf_readdict(pc) ) {
	    if ( (pt=PSDictHasEntry(&pc->pdfdict,"Type"))!=NULL && strcmp(pt,"/Font")==0 &&
		    (PSDictHasEntry(&pc->pdfdict,"FontDescriptor")!=NULL ||
		     ((pt=PSDictHasEntry(&pc->pdfdict,"Subtype"))!=NULL && strcmp(pt,"/Type3")==0)) &&
		    ((pt=PSDictHasEntry(&pc->pdfdict,"BaseFont"))!=NULL ||
		    /* Type3 fonts are named by "Name" rather than BaseFont */
			(pt=PSDictHasEntry(&pc->pdfdict,"Name"))!=NULL) ) {
		
		for (j=0; j<k && pc->fontobjs[j] != i; j++);
		if (j < k )
    continue;
		    
		if ((cmap=PSDictHasEntry(&pc->pdfdict,"ToUnicode"))!=NULL) {
		    if (*cmap == '[') cmap++;
		    sscanf(cmap, "%d", &cnum);
		    pc->cmapobjs[k] = cnum;
		}
		pc->fontobjs[k] = i;
		if ( *pt=='/' || *pt=='(' )
		    ++pt;
		pc->fontnames[k++] = tpt = copy(pt);
		for ( pt=tpt; *pt; ++pt ) {
		    if ( *pt=='#' && ishexdigit(pt[1]) && ishexdigit(pt[2])) {
			*tpt++ = hex(pt[1],pt[2]);
			pt += 2;
		    } else
			*tpt++ = *pt;
		}
		*tpt = '\0';
	    }
	}
    }
    pc->fcnt = k;
return( k>0 );
}

static int pdf_getinteger(char *pt,struct pdfcontext *pc) {
    int val,ret;
    long here;
    FILE *pdf;

    if ( pt==NULL )
return( 0 );
    val = strtol(pt,NULL,10);
    if ( pt[strlen(pt)-1]!='R' )
return( val );
    if ( val<0 || val>=pc->ocnt || pc->objs[val]==-1 )
return( 0 );
    here = ftell(pc->pdf);
    if ( !pdf_findobject(pc,val))
return( 0 );
    pdf = pc->compressed ? pc->compressed : pc->pdf;
    ret = fscanf(pdf,"%d",&val);
    if ( pc->compressed ) {
	fclose(pc->compressed );
	pc->compressed = NULL;
    }
    fseek(pc->pdf,here,SEEK_SET);
    if ( ret!=1 )
return( 0 );
return( val );
}

static void pdf_addpages(struct pdfcontext *pc, int obj) {
    /* Object is either a page or a page catalog */
    char *pt, *end;

    if ( pdf_findobject(pc,obj) && pdf_readdict(pc) ) {
	if ( (pt=PSDictHasEntry(&pc->pdfdict,"Type"))!=NULL ) {
	    if ( strcmp(pt,"/Page")==0 ) {
		pc->pages[pc->pcnt++] = obj;
	    } else if ( strcmp(pt,"/Pages")==0 ) {
		if ( (pt=PSDictHasEntry(&pc->pdfdict,"Kids"))!=NULL ) {
		    char *kids = copy(pt);
		    for ( pt = kids; *pt!=']' && *pt!='\0' ;  ) {
			if ( *pt=='[' || isspace(*pt)) {
			    ++pt;
			} else {
			    int o = strtol(pt,&end,10);
			    int r;
			    r = strtol(end,&end,10);
			    if ( pt==end )
return;
			    pt = end;
			    while ( isspace( *pt )) ++pt;
			    if ( *pt=='R' )
				++pt;
			    pdf_addpages(pc,o);
			}
		    }
		    free(kids);
		}
	    }
	}
    }
}

static int pdf_findpages(struct pdfcontext *pc) {
    FILE *pdf = pc->pdf;
    int top_ref;
    /* I could just find all the Page objects, but they would not be in order then */

    if ( pc->root==0 )
return( 0 );

    fseek(pdf,pc->objs[pc->root],SEEK_SET);
    if ( !findkeyword(pdf,"/Pages",">>"))
return( 0 );
    if ( fscanf( pdf, "%d", &top_ref )!=1 )
return( 0 );
    pc->pages = galloc(pc->ocnt*sizeof(long));
    pdf_addpages(pc,top_ref);
return( pc->pcnt );
}

/* ************************************************************************** */
/* *********************** Simplistic filter decoders *********************** */
/* ************************************************************************** */
static void pdf_hexfilter(FILE *to,FILE *from) {
    int ch1, ch2;

    rewind(from);
    while ( (ch1=getc(from))!=EOF ) {
	while ( !ishexdigit(ch1) && ch1!=EOF ) ch1 = getc(from);
	while ( (ch2=getc(from))!=EOF && !ishexdigit(ch2));
	if ( ch2==EOF )
    break;
	putc(hex(ch1,ch2),to);
    }
}

static void pdf_85filter(FILE *to,FILE *from) {
    int ch1, ch2, ch3, ch4, ch5;
    unsigned int val;
    int cnt;

    rewind(from);
    forever {
	while ( isspace(ch1=getc(from)));
	if ( ch1==EOF || ch1=='~' )
    break;
	if ( ch1=='z' ) {
	    putc(0,to);
	    putc(0,to);
	    putc(0,to);
	    putc(0,to);
	} else {
	    while ( isspace(ch2=getc(from)));
	    while ( isspace(ch3=getc(from)));
	    while ( isspace(ch4=getc(from)));
	    while ( isspace(ch5=getc(from)));
	    cnt = 4;
	    if ( ch3=='~' && ch4=='>' ) {
		cnt=1;
		ch3 = ch4 = ch5 = '!';
	    } else if ( ch4=='~' && ch5=='>' ) {
		cnt = 2;
		ch4 = ch5 = '!';
	    } else if ( ch5=='~' ) {
		cnt = 3;
		ch5 = '!';
	    }
	    val = ((((ch1-'!')*85+ ch2-'!')*85 + ch3-'!')*85 + ch4-'!')*85 + ch5-'!';
	    putc(val>>24,to);
	    if ( cnt>1 )
		putc((val>>16)&0xff,to);
	    if ( cnt>2 )
		putc((val>>8)&0xff,to);
	    if ( cnt>3 )
		putc(val&0xff,to);
	    if ( cnt!=4 )
    break;
	}
    }
}

#ifdef _NO_LIBPNG
static int haszlib(void) {
return( false );
}

static void pdf_zfilter(FILE *to,FILE *from) {
}
#else
# if !defined(_STATIC_LIBPNG) && !defined(NODYNAMIC)	/* I don't know how to deal with dynamic libs on mac OS/X, hence this */
#  include <dynamic.h>
# endif
# include <zlib.h>

# if !defined(_STATIC_LIBPNG) && !defined(NODYNAMIC)

static DL_CONST void *zlib=NULL;
static int (*_inflateInit_)(z_stream *,const char *,int);
static int (*_inflate)(z_stream *,int flags);
static int (*_inflateEnd)(z_stream *);

static int haszlib(void) {
    if ( zlib!=NULL )
return( true );

    if ( (zlib = dlopen("libz" SO_EXT,RTLD_GLOBAL|RTLD_LAZY))==NULL ) {
	LogError( "%s", dlerror());
return( false );
    }
    _inflateInit_ = (int (*)(z_stream *,const char *,int)) dlsym(zlib,"inflateInit_");
    _inflate = (int (*)(z_stream *,int )) dlsym(zlib,"inflate");
    _inflateEnd = (int (*)(z_stream *)) dlsym(zlib,"inflateEnd");
    if ( _inflateInit_==NULL || _inflate==NULL || _inflateEnd==NULL ) {
	LogError( "%s", dlerror());
	dlclose(zlib); zlib=NULL;
return( false );
    }
return( true );
}

/* Grump. zlib defines this as a macro */
#define _inflateInit(strm) \
        _inflateInit_((strm),                ZLIB_VERSION, sizeof(z_stream))

# else
/* Either statically linked, or loaded at start up */
static int haszlib(void) {
return( true );
}

#define _inflateInit	inflateInit
#define _inflate	inflate
#define _inflateEnd	inflateEnd

# endif /* !defined(_STATIC_LIBPNG) && !defined(NODYNAMIC) */

#define Z_CHUNK	65536
/* Copied with few mods from the zlib howto */
static int pdf_zfilter(FILE *to,FILE *from) {
    char *in;
    char *out;
    z_stream strm;
    int ret;

	/* Initialize */
    rewind(from);
    memset(&strm,0,sizeof(strm));
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = _inflateInit(&strm);
    if (ret != Z_OK) {
	LogError( _("Flate decompression failed.\n") );
return ret;
    }
    in = galloc(Z_CHUNK); out = galloc(Z_CHUNK);

    do {
	strm.avail_in = fread(in,1,Z_CHUNK,from);
	if ( strm.avail_in==0 )
    break;
	strm.next_in = (uint8 *) in;
	do {
	    strm.avail_out = Z_CHUNK;
	    strm.next_out = (uint8 *) out;
	    ret = _inflate(&strm, Z_NO_FLUSH);
	    if ( ret==Z_NEED_DICT || ret==Z_DATA_ERROR || ret==Z_MEM_ERROR ) {
		(void)_inflateEnd(&strm);
		LogError( _("Flate decompression failed.\n") );
return ret;
	    }
	    fwrite(out,1,Z_CHUNK-strm.avail_out,to);
	} while ( strm.avail_out == 0 );
    } while ( ret != Z_STREAM_END );
    (void)_inflateEnd(&strm);
    free(in); free(out);
return( ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR );
}
#endif /* _NO_LIBPNG */

static void pdf_rlefilter(FILE *to,FILE *from) {
    int ch1, ch2, i;

    rewind(from);
    while ( (ch1=getc(from))!=EOF && ch1!=0x80 ) {	/* 0x80 => EOD */
	if ( ch1<=127 ) {
	    for ( i=0; i<=ch1; ++i ) {	/* copy ch1+1 bytes directly */
		ch2 = getc(from);
		if ( ch2!=EOF )
		    putc(ch2,to);
	    }
	} else {			/* copy the next by 257-ch1 times */
	    ch2 = getc(from);
	    if ( ch2!=EOF )
		for ( i=0; i<257-ch1; ++i )
		    putc(ch2,to);
	}
    }
}

/* Filters I shall support: ASCIIHexDecode ASCII85Decode FlateDecode RunLengthDecode */
static FILE *pdf_defilterstream(struct pdfcontext *pc) {
    /* First copy the stream data into a file. This isn't efficient, but */
    /*  we can live with that */
    /* Then apply each de-filter sequentially reading from one file, writing */
    /*  to another */
    FILE *res, *old, *pdf = pc->pdf;
    int i,length,ch;
    char *pt, *end;

    if ( pc->compressed!=NULL ) {
	LogError( _("A pdf stream object may not be a compressed object"));
return( NULL );
    }
    if ( (pt=PSDictHasEntry(&pc->pdfdict,"Length"))==NULL ) {
	LogError( _("A pdf stream object is missing a Length attribute"));
return( NULL );
    }
    length = pdf_getinteger(pt,pc);

    while ( (ch=getc(pdf))!=EOF && ch!='m' );	/* Skip over >>\nstream */
    if ( (ch=getc(pdf))=='\r' ) ch = getc(pdf);	/* Skip the newline */

    res = tmpfile();
    for ( i=0; i<length; ++i ) {
	if ( (ch=getc(pdf))!=EOF )
	    putc(ch,res);
    }
    rewind(res);

    if ( (pt=PSDictHasEntry(&pc->pdfdict,"Filter"))==NULL )
return( res );
    while ( *pt==' ' || *pt=='[' || *pt==']' || *pt=='/' ) ++pt;	/* Yes, I saw a null array once */
    while ( *pt!='\0' ) {
	for ( end=pt; isalnum(*end); ++end );
	ch = *end; *end = '\0';
	old = res;
	res = tmpfile();
	if ( strmatch("ASCIIHexDecode",pt)==0 ) {
	    pdf_hexfilter(res,old);
	    pt += strlen("ASCIIHexDecode");
	} else if ( strmatch("ASCII85Decode",pt)==0 ) {
	    pdf_85filter(res,old);
	    pt += strlen("ASCII85Decode");
	} else if ( strmatch("FlateDecode",pt)==0 && haszlib()) {
	    pdf_zfilter(res,old);
	    pt += strlen("FlateDecode");
	} else if ( strmatch("RunLengthDecode",pt)==0 ) {
	    pdf_rlefilter(res,old);
	    pt += strlen("RunLengthDecode");
	} else {
	    LogError( _("Unsupported filter: %s"), pt );
	    fclose(old); fclose(res);
return( NULL );
	}
	*end = ch;
	pt = end;
	while ( *pt==' ' || *pt==']' || *pt=='/' ) ++pt;
	fclose(old);
    }
return( res );
}
/* ************************************************************************** */
/* ****************************** End filters ******************************* */
/* ************************************************************************** */

/* ************************************************************************** */
/* ****************************** xref streams ****************************** */
/* ************************************************************************** */
static long getuvalue(FILE *f,int len) {
    long val;
    int ch, i;

    val = 0;
    for ( i=0; i<len; ++i ) {
	ch = getc(f);
	val = (val<<8) | ch;
    }
return( val );
}

static long *FindObjectsFromXREFObject(struct pdfcontext *pc, long prev_xref) {
    char *pt;
    long *ret=NULL;
    int *gen=NULL;
    int cnt = 0, i, start, num;
    int bar;
    int typewidth, offwidth, genwidth;
    long type, offset, gennum;
    FILE *xref_stream, *pdf = pc->pdf;

    while ( prev_xref!=-1 ) {
	fseek(pdf,prev_xref,SEEK_SET);
	pdf_skipobjectheader(pc);
	if ( !pdf_readdict(pc))
return( NULL );
	if ( (pt=PSDictHasEntry(&pc->pdfdict,"Type"))==NULL || strcmp(pt,"/XRef")!=0 )
return( NULL );
	if ( (pt=PSDictHasEntry(&pc->pdfdict,"Size"))==NULL )
return( NULL );
	else {
	    start = 0;
	    num = pdf_getinteger(pt,pc);
	}
	if ( (pt=PSDictHasEntry(&pc->pdfdict,"Index"))!=NULL ) {
	    if ( sscanf(pt,"[%d %d]", &start, &num )!=2 )
return( NULL );
	}
	if ( (pt=PSDictHasEntry(&pc->pdfdict,"W"))==NULL )
return( NULL );
	else {
	    if ( sscanf(pt,"[%d %d %d]", &typewidth, &offwidth, &genwidth )!=3 )
return( NULL );
	}
	if ( (pt=PSDictHasEntry(&pc->pdfdict,"Encrypt"))!=NULL ) {
	    if ( sscanf( pt, "%d %d", &pc->enc_dict, &bar )==2 )
		pc->encrypted = true;
	}
	if ( pc->root == 0 && (pt=PSDictHasEntry(&pc->pdfdict,"Root"))!=NULL ) {
	    fscanf( pdf, "%d %d", &pc->root, &bar );
	}
	prev_xref = -1;
	if ( (pt=PSDictHasEntry(&pc->pdfdict,"Prev"))!=NULL ) {
	    prev_xref = strtol(pt,NULL,0);
	}
	/* I ignore Info */

	if ( start+num>cnt ) {
	    ret = grealloc(ret,(start+num+1)*sizeof(long));
	    memset(ret+cnt,-1,sizeof(long)*(start+num-cnt));
	    pc->subindex = grealloc(pc->subindex,(start+num+1)*sizeof(long));
	    memset(pc->subindex+cnt,-1,sizeof(long)*(start+num-cnt));
	    gen = grealloc(gen,(start+num)*sizeof(int));
	    memset(gen+cnt,-1,sizeof(int)*(start+num-cnt));
	    cnt = start+num;
	    pc->ocnt = cnt;
	    ret[cnt] = -2;
	}
	/* Now gather the cross references from their stream */
	xref_stream = pdf_defilterstream(pc);
	if ( xref_stream==NULL )
return( NULL );
	rewind(xref_stream);
	for ( i=start; i<start+num; ++i ) {
	    type   = getuvalue(xref_stream,typewidth);
	    offset = getuvalue(xref_stream,offwidth);
	    gennum = getuvalue(xref_stream,genwidth);
	    if ( feof(xref_stream)) {
		fclose(xref_stream);
return( NULL );
	    }
	    if ( type==0 ) {
		if ( gennum > gen[i] ) {
		    ret[i] = -1;
		    gen[i] = gennum;
		}
	    } else if ( type==1 ) {
		if ( gennum > gen[i] ) {
		    ret[i] = offset;
		    gen[i] = gennum;
		}
	    } else if ( type==2 ) {
		if ( 0 > gen[i] ) {
		    ret[i] = offset;	/* containing object # */
		    pc->subindex[i] = gennum;
		    gen[i] = 0;
		}
	    }
	}
	fclose(xref_stream);
    }
    free( gen );
return( ret );
}
/* ************************************************************************** */
/* **************************** End xref streams **************************** */
/* ************************************************************************** */

/* ************************************************************************** */
/* *********************** pdf graphics interpretter ************************ */
/* ************************************************************************** */

/* Stolen from the PS interpreter and then modified beyond all recognition */
enum pstoks { pt_eof=-1, pt_moveto, pt_lineto, pt_curveto, pt_vcurveto,
    pt_ycurveto, pt_closepath, pt_rect,
    pt_gsave, pt_grestore, pt_concat, pt_setlinewidth, pt_setlinecap, pt_setlinejoin,
    pt_setmiterlimit, pt_setdash,
    pt_stroke, pt_closestroke, pt_fillnz, pt_filleo, pt_fillstrokenz,
    pt_fillstrokeeo, pt_closefillstrokenz, pt_closefillstrokeeo, pt_paintnoop,
    pt_setcachedevice, pt_setcharwidth,
    pt_strokecolor, pt_fillcolor, pt_setgreystroke, pt_setgreyfill,
    pt_setrgbstroke, pt_setrgbfill,

    pt_true, pt_false,

    pt_opencurly, pt_closecurly, pt_openarray, pt_closearray, pt_string,
    pt_number, pt_unknown, pt_namelit
 };

static char *toknames[] = { "m", "l", "c", "v",
	"y", "h", "re",
	"q", "Q", "cm", "w", "j", "J",
	"M", "d",
	"S", "s", "f" /* "F" is an alternate form for "f"*/, "f*", "B"
	"B*", "b", "b*", "n",
	"d1", "d0",
	"SC", "sc", "G", "g",
	"RG", "rg",

	"true", "false",

	"opencurly", "closecurly", "openarray", "closearray", "string",
	"number", "unknown", "namelit", "=", "==",
	NULL };

static int nextpdftoken(FILE *file, real *val, char *tokbuf, int tbsize) {
    int ch, r, i;
    char *pt, *end;

    /* Eat whitespace and comments. Comments last to eol */
    while ( 1 ) {
	while ( isspace(ch = getc(file)) );
	if ( ch!='%' )
    break;
	while ( (ch=getc(file))!=EOF && ch!='\r' && ch!='\n' );
    }

    if ( ch==EOF )
return( pt_eof );

    pt = tokbuf;
    end = pt+tbsize-1;
    *pt++ = ch; *pt='\0';

    if ( ch=='(' ) {
	int nest=1, quote=0;
	while ( (ch=getc(file))!=EOF ) {
	    if ( pt<end ) *pt++ = ch;
	    if ( quote )
		quote=0;
	    else if ( ch=='(' )
		++nest;
	    else if ( ch==')' ) {
		if ( --nest==0 )
	break;
	    } else if ( ch=='\\' )
		quote = 1;
	}
	*pt='\0';
return( pt_string );
    } else if ( ch=='<' ) {
	ch = getc(file);
	if ( pt<end ) *pt++ = ch;
	if ( ch=='>' )
	    /* Done */;
	else if ( ch!='~' ) {
	    while ( (ch=getc(file))!=EOF && ch!='>' )
		if ( pt<end ) *pt++ = ch;
	} else {
	    int twiddle=0;
	    while ( (ch=getc(file))!=EOF ) {
		if ( pt<end ) *pt++ = ch;
		if ( ch=='~' ) twiddle = 1;
		else if ( twiddle && ch=='>' )
	    break;
		else twiddle = 0;
	    }
	}
	*pt='\0';
return( pt_string );
    } else if ( ch==')' || ch=='>' || ch=='[' || ch==']' || ch=='{' || ch=='}' ) {
	if ( ch=='{' )
return( pt_opencurly );
	else if ( ch=='}' )
return( pt_closecurly );
	if ( ch=='[' )
return( pt_openarray );
	else if ( ch==']' )
return( pt_closearray );

return( pt_unknown );	/* single character token */
    } else if ( ch=='/' ) {
	pt = tokbuf;
	while ( (ch=getc(file))!=EOF && !isspace(ch) && ch!='%' &&
		ch!='(' && ch!=')' && ch!='<' && ch!='>' && ch!='[' && ch!=']' &&
		ch!='{' && ch!='}' && ch!='/' )
	    if ( pt<tokbuf+tbsize-2 )
		*pt++ = ch;
	*pt = '\0';
	ungetc(ch,file);
return( pt_namelit );	/* name literal */
    } else {
	while ( (ch=getc(file))!=EOF && !isspace(ch) && ch!='%' &&
		ch!='(' && ch!=')' && ch!='<' && ch!='>' && ch!='[' && ch!=']' &&
		ch!='{' && ch!='}' && ch!='/' ) {
	    if ( pt<tokbuf+tbsize-2 )
		*pt++ = ch;
	}
	*pt = '\0';
	ungetc(ch,file);
	r = strtol(tokbuf,&end,10);
	pt = end;
	if ( *pt=='\0' ) {		/* It's a normal integer */
	    *val = r;
return( pt_number );
	} else if ( *pt=='#' ) {
	    r = strtol(pt+1,&end,r);
	    if ( *end=='\0' ) {		/* It's a radix integer */
		*val = r;
return( pt_number );
	    }
	} else {
	    *val = strtod(tokbuf,&end);
	    if ( !finite(*val) ) {
/* GT: NaN is a concept in IEEE floating point which means "Not a Number" */
/* GT: it is used to represent errors like 0/0 or sqrt(-1). */
		LogError( _("Bad number, infinity or nan: %s\n"), tokbuf );
		*val = 0;
	    }
	    if ( *end=='\0' )		/* It's a real */
return( pt_number );
	}
	/* It's not a number */
	for ( i=0; toknames[i]!=NULL; ++i )
	    if ( strcmp(tokbuf,toknames[i])==0 )
return( i );

return( pt_unknown );
    }
}

static void Transform(BasePoint *to, BasePoint *from, real trans[6]) {
    to->x = trans[0]*from->x+trans[2]*from->y+trans[4];
    to->y = trans[1]*from->x+trans[3]*from->y+trans[5];
}

static Entity *EntityCreate(SplinePointList *head,int linecap,int linejoin,
	real linewidth, real *transform) {
    Entity *ent = gcalloc(1,sizeof(Entity));
    ent->type = et_splines;
    ent->u.splines.splines = head;
    ent->u.splines.cap = linecap;
    ent->u.splines.join = linejoin;
    ent->u.splines.stroke_width = linewidth;
    ent->u.splines.fill.col = 0xffffffff;
    ent->u.splines.stroke.col = 0xffffffff;
    ent->u.splines.fill.opacity = 1.0;
    ent->u.splines.stroke.opacity = 1.0;
    memcpy(ent->u.splines.transform,transform,6*sizeof(real));
return( ent );
}

static void ECCatagorizePoints( EntityChar *ec ) {
    Entity *ent;

    for ( ent=ec->splines; ent!=NULL; ent=ent->next ) if ( ent->type == et_splines ) {
	SPLCatagorizePoints( ent->u.splines.splines );
    }
}

static void dictfree(struct pskeydict *dict) {
    int i;

    for ( i=0; i<dict->cnt; ++i ) {
	if ( dict->entries[i].type==ps_string || dict->entries[i].type==ps_instr ||
		dict->entries[i].type==ps_lit )
	    free(dict->entries[i].u.str);
	else if ( dict->entries[i].type==ps_array || dict->entries[i].type==ps_dict )
	    dictfree(&dict->entries[i].u.dict);
    }
}

static void freestuff(struct psstack *stack, int sp) {
    int i;

    for ( i=0; i<sp; ++i ) {
	if ( stack[i].type==ps_string || stack[i].type==ps_instr ||
		stack[i].type==ps_lit )
	    free(stack[i].u.str);
	else if ( stack[i].type==ps_array || stack[i].type==ps_dict )
	    dictfree(&stack[i].u.dict);
    }
}

static void _InterpretPdf(FILE *in, struct pdfcontext *pc, EntityChar *ec) {
    SplinePointList *cur=NULL, *head=NULL;
    BasePoint current;
    int tok, i, j;
    struct psstack stack[100];
    real dval;
    int sp=0;
    SplinePoint *pt;
    real transform[6], t[6];
    struct graphicsstate {
	real transform[6];
	BasePoint current;
	real linewidth;
	int linecap, linejoin;
	Color fore_stroke, fore_fill;
	DashType dashes[DASH_MAX];
    } gsaves[30];
    int gsp = 0;
    Color fore_stroke=COLOR_INHERITED, fore_fill=COLOR_INHERITED;
    int linecap=lc_inherited, linejoin=lj_inherited; real linewidth=WIDTH_INHERITED;
    DashType dashes[DASH_MAX];
    int dash_offset = 0;
    Entity *ent;
    char oldloc[24];
    char tokbuf[100];
    const int tokbufsize = 100;

    strcpy( oldloc,setlocale(LC_NUMERIC,NULL) );
    setlocale(LC_NUMERIC,"C");

    transform[0] = transform[3] = 1.0;
    transform[1] = transform[2] = transform[4] = transform[5] = 0;
    current.x = current.y = 0;
    dashes[0] = 0; dashes[1] = DASH_INHERITED;

    while ( (tok = nextpdftoken(in,&dval,tokbuf,tokbufsize))!=pt_eof ) {
	switch ( tok ) {
	  case pt_number:
	    if ( sp<sizeof(stack)/sizeof(stack[0]) ) {
		stack[sp].type = ps_num;
		stack[sp++].u.val = dval;
	    }
	  break;
	  case pt_string:
	    if ( sp<sizeof(stack)/sizeof(stack[0]) ) {
		stack[sp].type = ps_string;
		stack[sp++].u.str = copyn(tokbuf+1,strlen(tokbuf)-2);
	    }
	  break;
	  case pt_namelit:
	    if ( sp<sizeof(stack)/sizeof(stack[0]) ) {
		stack[sp].type = ps_lit;
		stack[sp++].u.str = copy(tokbuf);
	    }
	  break;
	  case pt_true: case pt_false:
	    if ( sp<sizeof(stack)/sizeof(stack[0]) ) {
		stack[sp].type = ps_bool;
		stack[sp++].u.tf = tok==pt_true;
	    }
	  break;
	  case pt_openarray:
	    if ( sp<sizeof(stack)/sizeof(stack[0]) ) {
		stack[sp++].type = ps_mark;
	    }
	  break;
	  case pt_closearray:
	    for ( i=0; i<sp; ++i )
		if ( stack[sp-1-i].type==ps_mark )
	    break;
	    if ( i==sp )
		LogError( _("No mark in ] (close array)\n") );
	    else {
		struct pskeydict dict;
		dict.cnt = dict.max = i;
		dict.entries = gcalloc(i,sizeof(struct pskeyval));
		for ( j=0; j<i; ++j ) {
		    dict.entries[j].type = stack[sp-i+j].type;
		    dict.entries[j].u = stack[sp-i+j].u;
		    /* don't need to copy because the things on the stack */
		    /*  are being popped (don't need to free either) */
		}
		sp = sp-i;
		stack[sp-1].type = ps_array;
		stack[sp-1].u.dict = dict;
	    }
	  break;
	  case pt_setcachedevice:
	    if ( sp>=6 ) {
		ec->width = stack[sp-6].u.val;
		ec->vwidth = stack[sp-5].u.val;
		/* I don't care about the bounding box */
		sp-=6;
	    }
	  break;
	  case pt_setcharwidth:
	    if ( sp>=2 )
		ec->width = stack[sp-=2].u.val;
	  break;
	  case pt_concat:
	    if ( sp>=1 ) {
		if ( stack[sp-1].type==ps_array ) {
		    if ( stack[sp-1].u.dict.cnt==6 && stack[sp-1].u.dict.entries[0].type==ps_num ) {
			--sp;
			t[5] = stack[sp].u.dict.entries[5].u.val;
			t[4] = stack[sp].u.dict.entries[4].u.val;
			t[3] = stack[sp].u.dict.entries[3].u.val;
			t[2] = stack[sp].u.dict.entries[2].u.val;
			t[1] = stack[sp].u.dict.entries[1].u.val;
			t[0] = stack[sp].u.dict.entries[0].u.val;
			dictfree(&stack[sp].u.dict);
			MatMultiply(t,transform,transform);
		    }
		}
	    }
	  break;
	  case pt_setmiterlimit:
	    sp = 0;		/* don't interpret, just ignore */
	  break;
	  case pt_setlinecap:
	    if ( sp>=1 )
		linecap = stack[--sp].u.val;
	  break;
	  case pt_setlinejoin:
	    if ( sp>=1 )
		linejoin = stack[--sp].u.val;
	  break;
	  case pt_setlinewidth:
	    if ( sp>=1 )
		linewidth = stack[--sp].u.val;
	  break;
	  case pt_setdash:
	    if ( sp>=2 && stack[sp-1].type==ps_num && stack[sp-2].type==ps_array ) {
		sp -= 2;
		dash_offset = stack[sp+1].u.val;
		for ( i=0; i<DASH_MAX && i<stack[sp].u.dict.cnt; ++i )
		    dashes[i] = stack[sp].u.dict.entries[i].u.val;
		dictfree(&stack[sp].u.dict);
	    }
	  break;
	  case pt_setgreystroke:
	    if ( sp>=1 ) {
		fore_stroke = stack[--sp].u.val*255;
		fore_stroke *= 0x010101;
	    }
	  break;
	  case pt_setgreyfill:
	    if ( sp>=1 ) {
		fore_fill = stack[--sp].u.val*255;
		fore_fill *= 0x010101;
	    }
	  break;
	  case pt_setrgbstroke:
	    if ( sp>=3 ) {
		fore_stroke = (((int) (stack[sp-3].u.val*255))<<16) +
			(((int) (stack[sp-2].u.val*255))<<8) +
			(int) (stack[sp-1].u.val*255);
		sp -= 3;
	    }
	  break;
	  case pt_setrgbfill:
	    if ( sp>=3 ) {
		fore_fill = (((int) (stack[sp-3].u.val*255))<<16) +
			(((int) (stack[sp-2].u.val*255))<<8) +
			(int) (stack[sp-1].u.val*255);
		sp -= 3;
	    }
	  break;
	  case pt_lineto:
	  case pt_moveto:
	    if ( sp>=2 ) {
		current.x = stack[sp-2].u.val;
		current.y = stack[sp-1].u.val;
		sp -= 2;
		pt = chunkalloc(sizeof(SplinePoint));
		Transform(&pt->me,&current,transform);
		pt->noprevcp = true; pt->nonextcp = true;
		if ( tok==pt_moveto ) {
		    SplinePointList *spl = chunkalloc(sizeof(SplinePointList));
		    spl->first = spl->last = pt;
		    if ( cur!=NULL )
			cur->next = spl;
		    else
			head = spl;
		    cur = spl;
		} else {
		    if ( cur!=NULL && cur->first!=NULL && (cur->first!=cur->last || cur->first->next==NULL) ) {
			SplineMake3(cur->last,pt);
			cur->last = pt;
		    }
		}
	    } else
		sp = 0;
	  break;
	  case pt_curveto: case pt_vcurveto: case pt_ycurveto:
	    if ( (sp>=6 && tok==pt_curveto) || (sp>=4 && tok!=pt_curveto)) {
		BasePoint ncp, pcp, to;
		to.x  = stack[sp-2].u.val;
		to.y  = stack[sp-1].u.val;
		if ( tok==pt_curveto ) {
		    ncp.x = stack[sp-6].u.val;
		    ncp.y = stack[sp-5].u.val;
		    pcp.x = stack[sp-4].u.val;
		    pcp.y = stack[sp-3].u.val;
		} else if ( tok==pt_vcurveto ) {
		    ncp = current;
		    pcp.x = stack[sp-4].u.val;
		    pcp.y = stack[sp-3].u.val;
		} else if ( tok==pt_ycurveto ) {
		    pcp = to;
		    ncp.x = stack[sp-4].u.val;
		    ncp.y = stack[sp-3].u.val;
		}
		current = to;
		if ( cur!=NULL && cur->first!=NULL && (cur->first!=cur->last || cur->first->next==NULL) ) {
		    Transform(&cur->last->nextcp,&ncp,transform);
		    cur->last->nonextcp = false;
		    pt = chunkalloc(sizeof(SplinePoint));
		    Transform(&pt->prevcp,&pcp,transform);
		    Transform(&pt->me,&current,transform);
		    pt->nonextcp = true;
		    SplineMake3(cur->last,pt);
		    cur->last = pt;
		}
	    }
	    sp = 0;
	  break;
	  case pt_rect:
	    if ( sp>=4 ) {
		SplinePointList *spl = chunkalloc(sizeof(SplinePointList));
		SplinePoint *first, *second, *third, *fourth;
		BasePoint temp1, temp2;
		spl->first = spl->last = pt;
		if ( cur!=NULL )
		    cur->next = spl;
		else
		    head = spl;
		cur = spl;
		temp1.x = stack[sp-4].u.val; temp1.y = stack[sp-3].u.val;
		Transform(&temp2,&temp1,transform);
		first = SplinePointCreate(temp2.x,temp2.y);
		temp1.x += stack[sp-2].u.val;
		Transform(&temp2,&temp1,transform);
		second = SplinePointCreate(temp2.x,temp2.y);
		temp1.y += stack[sp-3].u.val;
		Transform(&temp2,&temp1,transform);
		third = SplinePointCreate(temp2.x,temp2.y);
		temp1.x = stack[sp-4].u.val;
		Transform(&temp2,&temp1,transform);
		fourth = SplinePointCreate(temp2.x,temp2.y);
		cur->first = cur->last = first;
		SplineMake3(first,second);
		SplineMake3(second,third);
		SplineMake3(third,fourth);
		SplineMake3(fourth,first);
		current = temp1;
	    }
	    sp = 0;
	  break;
	  case pt_closepath:
	  case pt_stroke: case pt_closestroke: case pt_fillnz: case pt_filleo:
	  case pt_fillstrokenz: case pt_fillstrokeeo: case pt_closefillstrokenz:
	  case pt_closefillstrokeeo: case pt_paintnoop:
	    if ( tok==pt_closepath || tok==pt_closestroke ||
		    tok==pt_closefillstrokenz || tok==pt_closefillstrokeeo ) {
		if ( cur!=NULL && cur->first!=NULL && cur->first!=cur->last ) {
		    if ( cur->first->me.x==cur->last->me.x && cur->first->me.y==cur->last->me.y ) {
			SplinePoint *oldlast = cur->last;
			cur->first->prevcp = oldlast->prevcp;
			cur->first->noprevcp = false;
			oldlast->prev->from->next = NULL;
			cur->last = oldlast->prev->from;
			SplineFree(oldlast->prev);
			SplinePointFree(oldlast);
		    }
		    SplineMake3(cur->last,cur->first);
		    cur->last = cur->first;
		}
	    }
	    if ( tok==pt_closepath )
	  break;
	    else if ( tok==pt_paintnoop ) {
		SplinePointListsFree(head);
		head = cur = NULL;
	  break;
	    }
	    ent = EntityCreate(head,linecap,linejoin,linewidth,transform);
	    ent->next = ec->splines;
	    ec->splines = ent;
	    if ( tok==pt_stroke || tok==pt_closestroke || tok==pt_fillstrokenz ||
		    tok==pt_fillstrokeeo || tok==pt_closefillstrokenz ||
		    tok==pt_closefillstrokeeo )
		ent->u.splines.stroke.col = fore_stroke;
	    if ( tok==pt_fillnz || tok==pt_filleo || tok==pt_fillstrokenz ||
		    tok==pt_fillstrokeeo || tok==pt_closefillstrokenz ||
		    tok==pt_closefillstrokeeo )
		ent->u.splines.fill.col = fore_fill;
	    head = NULL; cur = NULL;
	  break;
	  case pt_gsave:
	    if ( gsp<30 ) {
		memcpy(gsaves[gsp].transform,transform,sizeof(transform));
		gsaves[gsp].current = current;
		gsaves[gsp].linewidth = linewidth;
		gsaves[gsp].linecap = linecap;
		gsaves[gsp].linejoin = linejoin;
		gsaves[gsp].fore_stroke = fore_stroke;
		gsaves[gsp].fore_fill = fore_fill;
		++gsp;
		/* Unlike PS does not! save current path */
	    }
	  break;
	  case pt_grestore:
	    if ( gsp>0 ) {
		--gsp;
		memcpy(transform,gsaves[gsp].transform,sizeof(transform));
		current = gsaves[gsp].current;
		linewidth = gsaves[gsp].linewidth;
		linecap = gsaves[gsp].linecap;
		linejoin = gsaves[gsp].linejoin;
		fore_stroke = gsaves[gsp].fore_stroke;
		fore_fill = gsaves[gsp].fore_fill;
	    }
	  break;
	  default:
	    sp=0;
	  break;
	}
    }
    freestuff(stack,sp);
    if ( head!=NULL ) {
	ent = EntityCreate(head,linecap,linejoin,linewidth,transform);
	ent->next = ec->splines;
	ec->splines = ent;
    }
    ECCatagorizePoints(ec);
    setlocale(LC_NUMERIC,oldloc);
}

static SplineChar *pdf_InterpretSC(struct pdfcontext *pc,char *glyphname,
	char *objnum, int *flags) {
    int gn = strtol(objnum,NULL,10);
    EntityChar ec;
    FILE *glyph_stream;
    SplineChar *sc;

    if ( gn<=0 || gn>=pc->ocnt || pc->objs[gn]==-1 )
  goto fail;
    if ( pdf_findobject(pc,gn) && !pdf_readdict(pc) )
  goto fail;
    glyph_stream = pdf_defilterstream(pc);
    if ( glyph_stream==NULL )
return( NULL );
    rewind(glyph_stream);

    memset(&ec,'\0',sizeof(ec));
    ec.fromtype3 = true;
    ec.sc = sc = SplineCharCreate(2);
    sc->name = copy(glyphname);

    _InterpretPdf(glyph_stream,pc,&ec);
    sc->width = ec.width;
#ifdef FONTFORGE_CONFIG_TYPE3
    sc->layer_cnt = 1;
    SCAppendEntityLayers(sc,ec.splines);
    if ( sc->layer_cnt==1 ) ++sc->layer_cnt;
#else
    sc->layers[ly_fore].splines = SplinesFromEntityChar(&ec,flags,false);
#endif

    fclose(glyph_stream);
return( sc );

  fail:
    LogError( _("Syntax error while parsing type3 glyph: %s"), glyphname );
return( NULL );
}

static Entity *pdf_InterpretEntity(struct pdfcontext *pc,int page_num) {
    EntityChar ec;
    SplineChar dummy;
    FILE *glyph_stream;
    char *pt;
    int content;

    if ( !pdf_findobject(pc,pc->pages[page_num]) || !pdf_readdict(pc) ) {
	LogError( _("Syntax error while parsing pdf graphics"));
return( NULL );
    }
    if ( (pt=PSDictHasEntry(&pc->pdfdict,"Contents"))==NULL ||
	    sscanf(pt,"%d",&content)!=1 ) {
	LogError( _("Syntax error while parsing pdf graphics: Page with no Contents"));
return( NULL );
    }
    if ( !pdf_findobject(pc,content) || !pdf_readdict(pc) ) {
	LogError( _("Syntax error while parsing pdf graphics"));
return( NULL );
    }
    glyph_stream = pdf_defilterstream(pc);
    if ( glyph_stream==NULL )
return( NULL );
    rewind(glyph_stream);

    memset(&ec,'\0',sizeof(ec));
    memset(&dummy,0,sizeof(dummy));
    ec.fromtype3 = true;
    ec.sc = &dummy;
    dummy.name = "Nameless glyph";

    _InterpretPdf(glyph_stream,pc,&ec);

    fclose(glyph_stream);
return( ec.splines );
}
    
/* ************************************************************************** */
/* ****************************** End graphics ****************************** */
/* ************************************************************************** */

static void add_mapping(SplineFont *basesf, long *mappings, int *uvals, int nuni, int gid, int cmap_from_cid, int cur) {
    int i, ndups, pos;
    char suffix[8], *name, *nname, buffer[400];
    SplineFont *sf = basesf->subfontcnt > 0 ? basesf->subfonts[0] : basesf;
    struct altuni *altuni, *prev;
    SplineChar *sc;

    name = copy(StdGlyphName(buffer,uvals[0],sf->uni_interp,sf->for_new_glyphs));
    name = grealloc(name,strlen(name)+8);
    for (i = 1; i<nuni; i++) {
	nname = copy(StdGlyphName(buffer,uvals[i],sf->uni_interp,sf->for_new_glyphs));
	name = grealloc(name,strlen(name)+strlen(nname)+10);
	strcat(name, "_");
	strcat(name, nname);
	free(nname);
    }
    ndups = 0;
    for (i=0; i < cur; i++) {
	if (mappings[i] == mappings[cur]) ndups++;
    }
    if (ndups) {
	sprintf(suffix, ".alt%d", ndups);
	strcat(name, suffix);
    }
    
    /* embedded TTF fonts may contain a 8-bit cmap table, denoted as platform ID 1 format 0 */
    /* (Apple). In fact this mapping has nothing to do both with Unicode and Apple, and rather */
    /* stores a custom order used to refer to glyphs from this particular PDF. */
    /* If such a mapping is present, then GIDs used in the ToUnicode Cmap array will correspond */
    /* to "Unicode" values it specifies rather than to the real order in which the glyphs are */
    /* stored in the file */
    pos = cmap_from_cid || sf->map == NULL ? gid : sf->map->map[gid];
    sc = sf->glyphs[pos];
    
    if (pos >= 0 && pos < sf->glyphcnt && (sc->unicodeenc != uvals[0] || nuni > 1)) {
	/* Sometimes FF instead of assigning proper Unicode values to TTF glyphs keeps */
	/* them encoded to the same codepoint, but creates for each glyph an alternate */
	/* encoding, corresponding to the position this glyph has in the font's encoding */
	/* map. As we are going to reencode the glyph anyway, we should remove those weird */
	/* AltUni's first */
	if (!cmap_from_cid) {
	    for ( altuni = sc->altuni, prev = NULL; altuni!=NULL; prev = altuni, altuni = altuni->next ) {
		if ( altuni->vs == -1 && altuni->unienc == gid)
	    break;
	    }
	    if ( altuni ) {
		if ( prev==NULL )
		    sc->altuni = altuni->next;
		else
		    prev->next = altuni->next;
		altuni->next = NULL;
		AltUniFree(altuni);
	    }
	}
	free(sc->name);
	sc->name = name;
	sc->unicodeenc = UniFromName(name,sf->uni_interp,&custom);
    }
}

static void pdf_getcmap(struct pdfcontext *pc, SplineFont *basesf, int font_num) {
    FILE *file;
    int i, j, gid, start, end, uni, cur=0, nuni, nhex, nchars, lo, *uvals;
    long *mappings;
    char tok[200], *ccval, prevtok[200];
    SplineFont *sf = basesf->subfontcnt > 0 ? basesf->subfonts[0] : basesf;

    if ( !pdf_findobject(pc,pc->cmapobjs[font_num]) || !pdf_readdict(pc) )
return;
    file = pdf_defilterstream(pc);
    if ( file==NULL )
return;
    rewind(file);
    
    mappings = gcalloc(sf->glyphcnt,sizeof(long));
    while ( pdf_getprotectedtok(file,tok) >= 0 ) {
	if ( strcmp(tok,"beginbfchar") == 0 && sscanf(prevtok,"%d",&nchars)) {
	    for (i=0; i<nchars; i++) {
		if (pdf_skip_brackets(file,tok) >= 0 && sscanf(tok,"%x",&gid) &&
		    pdf_skip_brackets(file,tok) >= 0 && sscanf(tok,"%lx",&mappings[cur])) {
		    
		    /* Values we store in the 'mappings' array are just unique identifiers, */
		    /* so they should not necessarily correspond to any valid Unicode codepoints. */
		    /* In order to get the real Unicode value mapped to a glyph we should parse the */
		    /* hex string once again, dividing it into hex quartets */
		    nhex = (strlen(tok))/4;
		    nuni = 1;
		    uvals = gcalloc(nhex,sizeof(int));
		    sscanf(tok,"%4x", &uvals[0]);
		    ccval = tok + 4;
		    /* If a single glyph is mapped to a sequence of Unicode characters, then the */
		    /* CMap mapping will contain two or more hex quartets. However a pair of such */
		    /* quartets may also represent a single Unicode character encoded with */
		    /* a surrogate pair */
		    for (j = 1; j<nhex && strlen(ccval) >= 4; j++) {
			sscanf(ccval,"%4x", &lo);
			if (uvals[nuni-1] >= 0xD800 && uvals[nuni-1] <= 0xDBFF && lo >= 0xDC00 && lo <= 0xDFFF )
			    uvals[nuni-1] = 0x10000 + (uvals[nuni-1] - 0xD800) * 0x400 + (lo - 0xDC00);
			else
			    uvals[nuni++] = lo;
			ccval += 4;
		    }
		    add_mapping(basesf, mappings, uvals, nuni, gid, pc->cmap_from_cid[font_num], cur);
		    free(uvals);
		    cur++;
		} else
  goto fail;
	    }
	    if ( pdf_getprotectedtok(file,tok) <= 0 || strcmp(tok,"endbfchar") != 0 )
  goto fail;
	} else if ( strcmp(tok,"beginbfrange") == 0 && sscanf(prevtok,"%d",&nchars)) {
	    for (i=0; i<nchars; i++) {
		if (pdf_skip_brackets(file,tok) >= 0 && sscanf(tok,"%x",&start) &&
		    pdf_skip_brackets(file,tok) >= 0 && sscanf(tok,"%x",&end) &&
		    pdf_skip_brackets(file,tok) >= 0 && sscanf(tok,"%lx",&mappings[cur])) {

		    uvals = gcalloc(1,sizeof(int));
		    sscanf(tok,"%4x", &uni);
		    /* For CMap values defining a character range we assume they should always */
		    /* correspond to a single Unicode character (either a BMP character or a surrogate pair) */
		    if (strlen(tok) >= 8) {
			sscanf(tok+4,"%4x", &lo);
			if (uni >= 0xD800 && uni <= 0xDBFF && lo >= 0xDC00 && lo <= 0xDFFF )
			    uni = 0x10000 + (uni - 0xD800) * 0x400 + (lo - 0xDC00);
		    }

		    for (gid=start; gid<=end; gid++) {
			mappings[cur] = uvals[0] = uni++;
			add_mapping(basesf, mappings, uvals, 1, gid, pc->cmap_from_cid[font_num], cur);
			cur++;
		    }
		    free(uvals);
		} else
  goto fail;
	    }
	    if ( pdf_getprotectedtok(file,tok) <= 0 || strcmp(tok,"endbfrange") != 0 )
  goto fail;
	} else
	    memcpy(prevtok,tok,200);
    }
    fclose(file);
    /* If this is not a cid font, then regenerate the font encoding (so that it is no */
    /* longer identified as MacRoman) */
    if ( sf->map != NULL && basesf == sf ) {
	EncMapFree( sf->map );
	sf->map = EncMapFromEncoding(sf,FindOrMakeEncoding("Original"));
    }
return;
  fail:
    LogError( _("Syntax errors while parsing ToUnicode CMap") );
}

static int pdf_getcharprocs(struct pdfcontext *pc,char *charprocs) {
    int cp = strtol(charprocs,NULL,10);
    FILE *temp, *pdf = pc->pdf;
    int ret;

    /* An indirect reference? */
    if ( cp!=0 ) {
	if ( !pdf_findobject(pc,cp) )
return( false );
return( pdf_readdict(pc));
    }
    temp = tmpfile();
    if ( temp==NULL )
return( false );
    while ( *charprocs ) {
	putc(*charprocs,temp);
	++charprocs;
    }
    rewind(temp);
    pc->pdf = temp;
    ret = pdf_readdict(pc);
    pc->pdf = pdf;
    fclose(temp);
return( ret );
}

static SplineFont *pdf_loadtype3(struct pdfcontext *pc) {
    char *enc, *cp, *fontmatrix, *name;
    double emsize;
    SplineFont *sf;
    int flags = -1;
    int i;
    struct psdict *charprocdict;

    name=PSDictHasEntry(&pc->pdfdict,"Name");
    if ( name==NULL )
	name=PSDictHasEntry(&pc->pdfdict,"BaseFont");
    if ( (enc=PSDictHasEntry(&pc->pdfdict,"Encoding"))==NULL )
  goto fail;
    if ( (cp=PSDictHasEntry(&pc->pdfdict,"CharProcs"))==NULL )
  goto fail;
    if ( (fontmatrix=PSDictHasEntry(&pc->pdfdict,"FontMatrix"))==NULL )
  goto fail;
    if ( sscanf(fontmatrix,"[%lg",&emsize)!=1 || emsize==0 )
  goto fail;
    emsize = 1.0/emsize;
    enc = copy(enc);
    name = copy(name+1);

    if ( !pdf_getcharprocs(pc,cp))
  goto fail;
    charprocdict = PSDictCopy(&pc->pdfdict);

    sf = SplineFontBlank(charprocdict->next);
    if ( name!=NULL ) {
	free(sf->fontname); free(sf->fullname); free(sf->familyname);
	sf->fontname = name;
	sf->familyname = copy(name);
	sf->fullname = copy(name);
    }
    free(sf->copyright); sf->copyright = NULL;
    free(sf->comments); sf->comments = NULL;
    sf->ascent = .8*emsize;
    sf->descent = emsize - sf->ascent;
#ifdef FONTFORGE_CONFIG_TYPE3
    sf->multilayer = true;
#endif

    for ( i=0; i<charprocdict->next; ++i ) {
	sf->glyphs[i] = pdf_InterpretSC(pc,charprocdict->keys[i],
		charprocdict->values[i],&flags);
	if ( sf->glyphs[i]!=NULL ) {
	    sf->glyphs[i]->orig_pos = i;
	    sf->glyphs[i]->parent = sf;
	    sf->glyphs[i]->vwidth = emsize;
	    sf->glyphs[i]->unicodeenc = UniFromName(sf->glyphs[i]->name,sf->uni_interp,&custom);
	}
    }
    sf->glyphcnt = charprocdict->next;
    PSDictFree(charprocdict);

    /* I'm going to ignore the encoding vector for now, and just return original */
    free(enc);
    sf->map = EncMapFromEncoding(sf,FindOrMakeEncoding("Original"));

return( sf );

  fail:
    LogError( _("Syntax errors while parsing Type3 font headers") );
return( NULL );
}

static FILE *pdf_insertpfbsections(FILE *file,struct pdfcontext *pc) {
    /* I don't need this. Type1 fonts provide us with the same info */
    /*  about cleartext length, binary length, cleartext length that */
    /*  the pfb section headings do. But the info isn't important in */
    /*  parsing the pfb file, so we can just ignore it */
return( file );
}

static SplineFont *pdf_loadfont(struct pdfcontext *pc,int font_num) {
    char *pt;
    int fd, type, ff;
    FILE *file;
    SplineFont *sf;

    if ( !pdf_findobject(pc,pc->fontobjs[font_num]) || !pdf_readdict(pc) )
return( NULL );

    if ( (pt=PSDictHasEntry(&pc->pdfdict,"Subtype"))!=NULL && strcmp(pt,"/Type3")==0 )
return( pdf_loadtype3(pc));

    if ( (pt=PSDictHasEntry(&pc->pdfdict,"FontDescriptor"))==NULL )
  goto fail;
    fd = strtol(pt,NULL,10);

    if ( !pdf_findobject(pc,fd) || !pdf_readdict(pc) )
  goto fail;

    if ( (pt=PSDictHasEntry(&pc->pdfdict,"FontFile"))!=NULL )
	type = 1;
    else if ( (pt=PSDictHasEntry(&pc->pdfdict,"FontFile2"))!=NULL )
	type = 2;
    else if ( (pt=PSDictHasEntry(&pc->pdfdict,"FontFile3"))!=NULL )
	type = 3;
    else {
	LogError( _("The font %s is one of the standard fonts. It isn't actually in the file."), pc->fontnames[font_num]);
return( NULL );
    }
    ff = strtol(pt,NULL,10);
    if ( !pdf_findobject(pc,ff) || !pdf_readdict(pc) )
  goto fail;
    file = pdf_defilterstream(pc);
    if ( file==NULL )
return( NULL );
    rewind(file);
    if ( type==1 ) {
	FontDict *fd;
	file = pdf_insertpfbsections(file,pc);
	fd = _ReadPSFont(file);
	sf = SplineFontFromPSFont(fd);
	PSFontFree(fd);
    } else if ( type==2 ) {
	sf = _SFReadTTF(file,0,pc->openflags,pc->fontnames[font_num],NULL);
    } else {
	int len;
	fseek(file,0,SEEK_END);
	len = ftell(file);
	rewind(file);
	sf = _CFFParse(file,len,pc->fontnames[font_num]);
    }
    fclose(file);
    /* Don't attempt to parse CMaps for Type 1 fonts: they already have glyph names */
    /* which are usually more meaningful */
    if (pc->cmapobjs[font_num] != -1 && type > 1)
	pdf_getcmap(pc, sf, font_num);
return( sf );

  fail:
    LogError( _("Unable to parse the pdf objects that make up %s"),pc->fontnames[font_num]);
return( NULL );
}

static void pcFree(struct pdfcontext *pc) {
    int i;

    PSDictClear(&pc->pdfdict);
    free(pc->pdfdict.keys);
    free(pc->pdfdict.values);
    free(pc->objs);
    for ( i=0; i<pc->fcnt; ++i )
	free(pc->fontnames[i]);
    free(pc->fontnames);
    free(pc->fontobjs);
    free(pc->cmapobjs);
    free(pc->cmap_from_cid);
    free(pc->pages);
    free(pc->tokbuf);
}

char **NamesReadPDF(char *filename) {
    struct pdfcontext pc;
    char oldloc[24];
    int i;
    char **list;

    strcpy( oldloc,setlocale(LC_NUMERIC,NULL) );
    setlocale(LC_NUMERIC,"C");
    memset(&pc,0,sizeof(pc));
    pc.pdf = fopen(filename,"r");
    if ( pc.pdf==NULL )
return( NULL );
    if ( (pc.objs = FindObjects(&pc))==NULL ) {
	LogError( _("Doesn't look like a valid pdf file, couldn't find xref section") );
	fclose(pc.pdf);
	pcFree(&pc);
	setlocale(LC_NUMERIC,oldloc);
return( NULL );
    }
    if ( pc.encrypted ) {
	LogError( _("This pdf file contains an /Encrypt dictionary, and FontForge does not currently\nsupport pdf encryption" ));
	fclose(pc.pdf);
	pcFree(&pc);
	setlocale(LC_NUMERIC,oldloc);
return( NULL );
    }
    if ( pdf_findfonts(&pc)==0 ) {
	fclose(pc.pdf);
	pcFree(&pc);
	setlocale(LC_NUMERIC,oldloc);
return( NULL );
    }
    list = galloc((pc.fcnt+1)*sizeof(char *));
    for ( i=0; i<pc.fcnt; ++i )
	list[i] = copy( pc.fontnames[i]);
    list[i] = NULL;
    fclose(pc.pdf);
    pcFree(&pc);
    setlocale(LC_NUMERIC,oldloc);
return( list );
}

SplineFont *_SFReadPdfFont(FILE *pdf,char *filename,char *select_this_font,
	enum openflags openflags) {
    struct pdfcontext pc;
    SplineFont *sf = NULL;
    char oldloc[24];
    int i;

    strcpy( oldloc,setlocale(LC_NUMERIC,NULL) );
    setlocale(LC_NUMERIC,"C");
    memset(&pc,0,sizeof(pc));
    pc.pdf = pdf;
    pc.openflags = openflags;
    if ( (pc.objs = FindObjects(&pc))==NULL ) {
	LogError( _("Doesn't look like a valid pdf file, couldn't find xref section") );
	pcFree(&pc);
	setlocale(LC_NUMERIC,oldloc);
return( NULL );
    }
    if ( pc.encrypted ) {
	LogError( _("This pdf file contains an /Encrypt dictionary, and FontForge does not currently\nsupport pdf encryption" ));
	pcFree(&pc);
	setlocale(LC_NUMERIC,oldloc);
return( NULL );
    }
    if ( pdf_findfonts(&pc)==0 ) {
	LogError( _("This pdf file has no fonts"));
	pcFree(&pc);
	setlocale(LC_NUMERIC,oldloc);
return( NULL );
    }
    if ( pc.fcnt==1 ) {
	sf = pdf_loadfont(&pc,0);
    } else if ( select_this_font!=NULL ) {
	for ( i=0; i<pc.fcnt; ++i ) {
	    if ( strcmp(pc.fontnames[i],select_this_font)==0 )
	break;
	}
	if ( i<pc.fcnt )
	    sf = pdf_loadfont(&pc,i);
	else
	    ff_post_error(_("Not in Collection"),_("%s is not in %.100s"),
		    select_this_font, filename);
    } else {
	char **names;
	int choice;
	names = galloc((pc.fcnt+1)*sizeof(unichar_t *));
	for ( i=0; i<pc.fcnt; ++i )
	    names[i] = copy(pc.fontnames[i]);
	names[i] = NULL;
	if ( no_windowing_ui )
	    choice = 0;
	else
	    choice = ff_choose(_("Pick a font, any font..."),(const char **) names,pc.fcnt,0,_("There are multiple fonts in this file, pick one"));
	for ( i=0; i<pc.fcnt; ++i )
	    free(names[i]);
	free(names);
	if ( choice!=-1 )
	    sf = pdf_loadfont(&pc,choice);
    }
    setlocale(LC_NUMERIC,oldloc);
    pcFree(&pc);
return( sf );
}

SplineFont *SFReadPdfFont(char *filename,enum openflags openflags) {
    char *pt, *freeme=NULL, *freeme2=NULL, *select_this_font=NULL;
    SplineFont *sf;
    FILE *pdf;

    if ( (pt=strchr(filename,'('))!=NULL ) {
	freeme = filename = copyn(filename,pt-filename);
	freeme2 = select_this_font = copy(pt+1);
	if ( (pt=strchr(select_this_font,')'))!=NULL )
	    *pt = '\0';
    }

    pdf = fopen(filename,"r");
    if ( pdf==NULL )
	sf = NULL;
    else {
	sf = _SFReadPdfFont(pdf,filename,select_this_font,openflags);
	fclose(pdf);
    }
    free(freeme); free(freeme2);
return( sf );
}

Entity *EntityInterpretPDFPage(FILE *pdf,int select_page) {
    struct pdfcontext pc;
    char oldloc[24];
    Entity *ent;
    char *ret;
    int choice;

    strcpy( oldloc,setlocale(LC_NUMERIC,NULL) );
    setlocale(LC_NUMERIC,"C");
    memset(&pc,0,sizeof(pc));
    pc.pdf = pdf;
    pc.openflags = 0;
    if ( (pc.objs = FindObjects(&pc))==NULL ) {
	LogError( _("Doesn't look like a valid pdf file, couldn't find xref section") );
	pcFree(&pc);
	setlocale(LC_NUMERIC,oldloc);
return( NULL );
    }
    if ( pc.encrypted ) {
	LogError( _("This pdf file contains an /Encrypt dictionary, and FontForge does not currently\nsupport pdf encryption" ));
	pcFree(&pc);
	setlocale(LC_NUMERIC,oldloc);
return( NULL );
    }
    if ( pdf_findpages(&pc)==0 ) {
	LogError( _("This pdf file has no pages"));
	pcFree(&pc);
	setlocale(LC_NUMERIC,oldloc);
return( NULL );
    }
    if ( pc.pcnt==1 ) {
	ent = pdf_InterpretEntity(&pc,0);
    } else if ( select_page>=0 && select_page<pc.pcnt ) {
	ent = pdf_InterpretEntity(&pc,select_page);
    } else {
	if ( no_windowing_ui )
	    choice = 0;
	else {
	    char buffer[200];
	    snprintf( buffer, sizeof(buffer), _("There are %d pages in this file, which do you want?"), pc.pcnt );
	    ret = ff_ask_string(_("Pick a page"),"1",buffer);
	    if ( ret==NULL ) {
		pcFree(&pc);
		setlocale(LC_NUMERIC,oldloc);
return( NULL );
	    }
	    choice = strtol(ret,NULL,10)-1;
	    if ( choice<0 || choice>=pc.pcnt ) {
		pcFree(&pc);
		setlocale(LC_NUMERIC,oldloc);
return( NULL );
	    }
	}
	ent = pdf_InterpretEntity(&pc,choice);
    }
    setlocale(LC_NUMERIC,oldloc);
    pcFree(&pc);
return( ent );
}

