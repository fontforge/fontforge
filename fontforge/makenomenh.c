#include <stdio.h>
#include <string.h>
#include <utype.h>

#include <sys/types.h>
#include <dirent.h>
#include <time.h>

#include <sys/stat.h>
#include <unistd.h>

#include <ggadget.h>
#include <ustring.h>
#include <charset.h>
#include <chardata.h>

static int make_po = false;

#ifdef FONTFORGE_CONFIG_NO_WINDOWING_UI
int local_encoding = e_iso8859_1;
char *iconv_local_encoding_name = NULL;
#endif

#define e_hexjis	100

static char *istandard[] = { "buttonsize", "ScaleFactor", NULL };

static char *standard[] = { "Language", "OK", "Cancel", "Open", "Save",
	"Filter", "New", "Replace", "Fileexists", "Fileexistspre",
	"Fileexistspost", "Createdir", "Dirname", "Couldntcreatedir",
	"SelectAll", "None", NULL };

static char *lc="pfaedit", *uc="PfaEdit";
static unichar_t **names, **inames, **en_strings, *en_mn;
static char *hadmn;
static int nlen=__STR_LastStd+1000, npos=__STR_LastStd+1;
static int ilen=__NUM_LastStd+1, ipos=__NUM_LastStd+1;
static int checksum;

static int isstandard(char *name) {
    int i;

    for ( i=0; standard[i]!=NULL; ++i )
	if ( strcasecmp(standard[i],name)==0 )
return( 1 );

return( 0 );
}

static int isistandard(char *name) {
    int i;

    for ( i=0; istandard[i]!=NULL; ++i )
	if ( strcasecmp(istandard[i],name)==0 )
return( 1 );

return( 0 );
}

static int lookup(char *name) {
    int i;

    for ( i=0; i<npos; ++i )
	if ( uc_strmatch(names[i],name)==0 )
return( i );

return( -1 );
}

static int ilookup(char *name) {
    int i;

    for ( i=0; i<ipos; ++i )
	if ( uc_strmatch(inames[i],name)==0 )
return( i );

return( -1 );
}

static void handleint(FILE *out,char *buffer,int off) {
    char *pt;

    if ( buffer[off]=='_' ) ++off;
    for ( pt = buffer+off; isalnum(*pt) || *pt=='_'; ++pt );
    *pt ='\0';
    if ( buffer[off]=='\0' )
return;
    if ( isistandard(buffer+off))
return;
    if ( islower(buffer[off])) buffer[off] = toupper(buffer[off]);
    fprintf( out, "#define _NUM_%s\t%d\n", buffer+off, npos );
    if ( ipos>=ilen ) {
	ilen += 1000;
	inames = grealloc(inames,ilen*sizeof(unichar_t*));
    }
    inames[ipos++] = uc_copy(buffer+off);
}

static int makenomenh() {
    char buffer[1025];
    FILE *in, *out;
    char *pt;
    int off, i;
    int ismn;
#ifdef __VMS
    stat_t stat_buf;
#else
    struct stat stat_buf;
#endif

    names = malloc(nlen*sizeof(unichar_t *));
    hadmn = calloc(nlen,sizeof(char));
    for ( i=0; standard[i]!=NULL; ++i )
	names[i] = uc_copy(standard[i]);

    inames = malloc(ilen*sizeof(unichar_t *));
    for ( i=0; istandard[i]!=NULL; ++i )
	inames[i] = uc_copy(istandard[i]);

    stat("nomen-en.c",&stat_buf);
    in = fopen("nomen-en.c","r");
    if ( in==NULL ) {
	fprintf(stderr, "Missing required input file: nomen-en.c\n" );
	exit( 1 );
    }
    out = fopen("nomen.h","w");
    fprintf( out, "#ifndef _NOMEN_H\n" );
    fprintf( out, "#define _NOMEN_H\n" );
    fprintf( out, "#include <basics.h>\n" );
    fprintf( out, "#include <stdio.h>\n" );
    fprintf( out, "#include <ggadget.h>\n\n" );

    fprintf( out, "#define __NOMEN_CHECKSUM\t%d\n\n", (int) stat_buf.st_size );
    checksum = stat_buf.st_size;

    while( fgets(buffer,sizeof(buffer),in)!=NULL ) {
	if ( (buffer[0]=='/' && buffer[1]=='*') || buffer[0]=='\n' ) {
	    fprintf( out, "%s", buffer );
    continue;
	}
	if ( strncmp(buffer,"static ",7)!=0 )
    continue;
	off = 7;
	if (strncmp(buffer+off,"const ",6)==0 )
	    off += 6;
	if ( strncmp(buffer+off,"int num_",8)==0 ) {
	    handleint(out,buffer,off+8);
    continue;
	}
	if ( strncmp(buffer+off,"unichar_t ",10)==0 )
	    off += 10;
	else if ( strncmp(buffer+off,"char ",5)==0 )
	    off += 5;
	else
    continue;
	if ( buffer[off]=='*' ) ++off;
	pt = buffer+off;
	ismn = 0;
	if ( strncmp(pt,"mnemonic_",9)==0 ) {
	    ismn = 1;
	    off += 9;
	} else if ( strncmp(pt,"str_",4)==0 )
	    off += 4;
	if ( buffer[off]=='_' ) ++off;
	for ( pt = buffer+off; isalnum(*pt) || *pt=='_'; ++pt );
	*pt ='\0';
	if ( buffer[off]=='\0' )
    continue;
	if ( ismn ) {
	    int index = lookup(buffer+off);
	    if ( index==-1 )
		fprintf( stderr, "mnemonic for %s when there's no string for it. Possibly mnemonic comes first\n in file, should follow string.\n", buffer+off );
	    else
		hadmn[index] = 1;
    continue;
	}
	if ( isstandard(buffer+off))
    continue;
	if ( islower(buffer[off])) buffer[off] = toupper(buffer[off]);
	fprintf( out, "#define _STR_%s\t%d\n", buffer+off, npos );
	if ( npos>=nlen ) {
	    nlen += 1000;
	    names = grealloc(names,nlen*sizeof(unichar_t*));
	    hadmn = grealloc(hadmn,nlen);
	    for ( i=nlen-1000; i<nlen; ++i ) hadmn[i] = 0;
	}
	names[npos++] = uc_copy(buffer+off);
    }
    fprintf( out, "\n#endif\n" );
    fflush( out );
    fclose( in );
    if ( ferror(out) || fclose(out)!=0 )
return( 1 );

return( 0 );
}

static void outpututf8(FILE *out, unichar_t *str) {
    if ( str!=NULL ) {
	putc('"',out);
	while ( *str ) {
	    if ( *str<' ' || *str=='\\' || *str=='"' || *str=='\177' ) {
		putc('\\',out);
		if ( *str=='\n' )
		    putc('n',out);
		else if ( *str=='\t' )
		    putc('t',out);
		else if ( *str=='\\' )
		    putc('\\',out);
		else if ( *str=='"' )
		    putc('"',out);
		else {
		    putc('0'+((*str&0700)>>6), out);
		    putc('0'+((*str&070)>>3), out);
		    putc('0'+(*str&07), out);
		}
	    } else if ( *str<0x80 )
		putc( *str,out);
	    else if ( *str<0x800 ) {
		putc( 0xc0 | (*str>>6), out );
		putc( 0x80 | (*str&0x3f), out );
	    } else {
		putc( 0xe0 | (*str>>12), out );
		putc( 0x80 | ((*str>>6)&0x3f), out );
		putc( 0x80 | (*str&0x3f), out );
	    }
	    ++str;
	}
	putc('"',out);
    }
    fprintf( out, "\n" );
}

static unichar_t *AddMn(unichar_t *str,unichar_t mn, int strict ) {
    unichar_t *ret, *pt, *rpt;

    if ( str==NULL )
return( NULL );
    if ( mn==0 ) {
	ret = u_copy(str);
	/* remove %hs entries */
	for ( rpt=ret; *rpt!='%' && *rpt!='\0'; ++rpt );
	if ( *rpt=='%' ) {
	    for ( ++rpt; *rpt!='\0' && *rpt!='d' && *rpt!='i' && *rpt!='o' &&
		    *rpt!='u' && *rpt!='x' && *rpt!='X' && *rpt!='e' &&
		    *rpt!='E' && *rpt!='f' && *rpt!='F' && *rpt!='g' &&
		    *rpt!='G' && *rpt!='c' && *rpt!='s' && *rpt!='%' ; ++rpt ) {
		if ( *rpt=='h' && rpt[1]=='s' ) {
		    u_strcpy(rpt,rpt+1);
	    break;
		}
	    }
	}
return( ret );
    }

    ret = malloc((u_strlen(str)+8)*sizeof(unichar_t));
    if ( islower(mn)) mn = toupper(mn);
    for ( pt = str, rpt = ret; *pt!='\0'; ) {
	int ch = islower(*pt)? toupper(*pt) : *pt;
	if ( ch==mn ) {
	    *rpt++ = '_';
	    mn = 0;
	}
	if ( ch=='_' ) {
	    fprintf( stderr, "Entry with mnemonic has underscore " );
	    outpututf8(stderr,str);
	}
	*rpt++ = *pt++;
    }
    if ( strict && mn!=0 ) {
	fprintf( stderr, "Failed to find mnemonic. " );
	outpututf8(stderr,str);
	fprintf( stderr, "\n" );
	exit(1);
    }
    if ( mn!=0 ) {
	*rpt++ = '(';
	*rpt++ = '_';
	*rpt++ = mn;
	*rpt++ = ')';
    }
    *rpt = '\0';
return( ret );
}

static void DumpPOHeader(FILE *po,char *lang) {
    time_t now;
    struct tm *tmnow;
    
    fprintf( po, "# SOME DESCRIPTIVE TITLE.\n" );
    fprintf( po, "# Copyright (C) YEAR THE PACKAGE'S COPYRIGHT HOLDER\n" );
    fprintf( po, "# This file is distributed under the same license as FontForge.\n" );
    fprintf( po, "# FIRST AUTHOR <EMAIL@ADDRESS>, YEAR.\n" );
    fprintf( po, "#\n" );
    fprintf( po, "#, fuzzy\n" );
    fprintf( po, "msgid \"\"\n" );
    fprintf( po, "msgstr \"\"\n" );
    fprintf( po, "\"Project-Id-Version: PACKAGE VERSION\\n\"\n" );
    time(&now);
    tmnow = localtime(&now);
    fprintf( po, "\"POT-Creation-Date: %d-%02d-%02d %02d:%02d+%02ld%02ld\\n\"\n",
	    tmnow->tm_year+1900, tmnow->tm_mon, tmnow->tm_mday,
	    tmnow->tm_hour, tmnow->tm_min,
	    timezone/3600, (timezone/60)%60);
    fprintf( po, "\"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\\n\"\n" );
    fprintf( po, "\"Last-Translator: FULL NAME <EMAIL@ADDRESS>\\n\"\n" );
    fprintf( po, "\"Language-Team: LANGUAGE <%s@li.org>\\n\"\n", lang );
    fprintf( po, "\"MIME-Version: 1.0\\n\"\n" );
    if ( strcmp(lang,"LANG")==0 )
	fprintf( po, "\"Content-Type: text/plain; charset=CHARSET\\n\"\n" );
    else
	fprintf( po, "\"Content-Type: text/plain; charset=UTF-8\\n\"\n" );
    fprintf( po, "\"Content-Transfer-Encoding: 8bit\\n\"\n" );
}

static void MakeGoodEnglishPO(unichar_t **en_strings,unichar_t *en_mn) {
    int i, j;
    FILE *search, *pot;
    char fn[100];

    if ( !make_po ) {
	for ( i=0; i<npos; ++i )
	    free(en_strings[i]);
	free(en_strings);
	free(en_mn);
return;
    }

    for ( i=0; i<npos; ++i ) {
	unichar_t *str = AddMn(en_strings[i],en_mn[i],true);
	free(en_strings[i]);
	en_strings[i] = str;
    }

    search = fopen("po/replacements","w");
    if ( search!=NULL ) {
	for ( i=0; i<npos; ++i ) {
	    char *str = cu_copy(names[i]);
	    fprintf( search, "_STR_%s\t", str);
	    free(str);
	    outpututf8(search,en_strings[i]);
	}
	fclose(search);
    } else
	fprintf( stderr, "Failed to create replacement file.\n" );

    for ( i=0; i<npos; ++i ) {
	if ( *en_strings[i]=='\0' )
	    en_strings[i] = NULL;
	else for ( j=i+1; j<npos; ++j ) {
	    if ( u_strcmp(en_strings[i],en_strings[j])==0 ) {
		en_strings[i] = NULL;
		fprintf( stderr, "Duplicate msg :" );
		outpututf8(stderr,en_strings[j]);
		fprintf( stderr, "\n" );
	break;
	    }
	}
    }

    sprintf(fn, "po/%s.pot", uc);
    pot = fopen(fn,"w");
    if ( pot!=NULL ) {
	DumpPOHeader(pot,"LANG");
	for ( i=0; i<npos; ++i ) if ( en_strings[i]!=NULL ) {
	    fprintf( pot, "\nmsgid " );
	    outpututf8(pot,en_strings[i]);
	    fprintf( pot, "msgstr \"\"\n" );
	}
	if ( ipos>0 ) {
	    fprintf( pot, "\nmsgid \"GGadget|ButtonSize|55\"\n" );
	    fprintf( pot, "msgstr \"\"\n");
	}
	if ( ipos>1 ) {
	    fprintf( pot, "\nmsgid \"GGadget|ScaleFactor|100\"\n" );
	    fprintf( pot, "msgstr \"\"\n");
	}
	fclose(pot);
    } else
	fprintf( stderr, "Failed to create po template file.\n" );
}

static void MakePOFile(unichar_t **en_strings,
	unichar_t **values,unichar_t *mn, int *ivalues, char *lang) {
    int i;
    char buffer[80], *pt;
    FILE *out, *full;
    unichar_t *str;

    if ( !make_po )
return;

    strcpy(buffer,"po/");
    strcat(buffer,lang);
    pt = strrchr(buffer,'.');
    if ( pt!=NULL ) *pt = '\0';
    if ( strcmp(buffer+3,"en")==0 )	/* No translation to us english needed */
return;
    strcat(buffer,".po");

    out = fopen(buffer,"w");
    if ( out==NULL ) {
	fprintf( stderr, "Failed to write %s\n", buffer);
return;
    }
    DumpPOHeader(out,lang);
    strcpy(buffer+strlen(buffer)-3, "-full.po");
    full = fopen(buffer,"w");
    if ( full!=NULL )
	DumpPOHeader(full,lang);

    for ( i=0; i<npos; ++i ) {
	if ( en_strings[i]==NULL )
    continue;
	str = AddMn(values[i],mn[i],false);
	if ( str!=NULL ) {
	    fprintf( out, "\nmsgid " );
	    outpututf8(out,en_strings[i]);
	    fprintf( out, "msgstr " );
	    outpututf8(out,str);
	}
	if ( full!=NULL ) {
	    fprintf( full, "\nmsgid " );
	    outpututf8(full,en_strings[i]);
	    fprintf( full, "msgstr " );
	    outpututf8(full,str);
	}
	free(str);
    }
    if ( ipos>0 && ivalues[0]!=-1 ) {
	fprintf( out, "\nmsgid \"GGadget|ButtonSize|55\"\n" );
	fprintf( out, "msgstr \"%d\"\n", ivalues[0]);
	if ( full!=NULL ) {
	    fprintf( full, "\nmsgid \"GGadget|ButtonSize|55\"\n" );
	    fprintf( full, "msgstr \"%d\"\n", ivalues[0]);
	}
    }
    if ( ipos>1 && ivalues[1]!=-1 ) {
	fprintf( out, "\nmsgid \"GGadget|ScaleFactor|100\"\n" );
	fprintf( out, "msgstr \"%d\"\n", ivalues[1]);
	if ( full!=NULL ) {
	    fprintf( full, "\nmsgid \"GGadget|ScaleFactor|100\"\n" );
	    fprintf( full, "msgstr \"%d\"\n", ivalues[1]);
	}
    }
    fclose(out); fclose(full);
}

static int charval(char **buffer) {
    unsigned char *bpt = (unsigned char *) *buffer;
    int val;
    
    if ( *bpt!='\\' ) {
	++*buffer;
return( *bpt );
    } else {
	++bpt;
	val = *bpt;
	if ( val=='n' ) val = '\n';
	else if ( isdigit(*bpt)) {
	    unsigned char *start = bpt;
	    val = 0;
	    while ( isdigit(*bpt) && bpt-start<3 )
		val = (val<<3) | (*bpt++-'0');
	    --bpt;
	} else if ( *bpt=='x' ) {
	    unsigned char *start = bpt;
	    val = 0;
	    while ( (isdigit(*bpt) || (*bpt>='a' && *bpt<='f') || (*bpt>='A' && *bpt<='F')) &&
		    bpt-start<3 ) {
		val <<= 4;
		if ( isdigit(*bpt))
		    val |= *bpt++-'0';
		else if ( *bpt>='a' && *bpt<='f' )
		    val |= (*bpt++-'a'+10);
		else
		    val |= (*bpt++-'A'+10);
	    }
	    --bpt;
	}
    }
    *buffer = (char *) bpt+1;
return( val );
}

static int twocharval(char **buffer,int enc) {
    /* Currently only support a few */
    int ch1, ch2;

    if ( enc==e_wansung ) {
	ch1 = charval(buffer);
	if ( ch1<0xa1 )
return( ch1 );
	ch1 -= 0xa1;
	ch2 = charval(buffer)-0xa1;
	ch1 = ch1*94 + ch2;
	ch1 = unicode_from_ksc5601[ch1];
return( ch1 );
    } else if ( enc==e_big5 ) {
	ch1 = charval(buffer);
	if ( ch1<0xa1 )
return( ch1 );
	ch2 = charval(buffer);
	ch1 = (ch1<<8) + ch2;
	ch1 = unicode_from_big5[ch1-0xa100];
return( ch1 );
    } else if ( enc==e_johab ) {
	ch1 = charval(buffer);
	if ( ch1<0xa1 )
return( ch1 );
	ch2 = charval(buffer);
	ch1 = (ch1<<8) + ch2;
	ch1 = unicode_from_johab[ch1-0x8400];
return( ch1 );
    } else if ( enc==e_sjis ) {
	ch1 = charval(buffer);
	if ( ch1<0x80 )
return( ch1 );
	else if ( ch1>=161 && ch1<=223 )
	    /* Katakana */
return( unicode_from_jis201[ch1]);
	ch2 = charval(buffer);
	if ( ch1 >= 129 && ch1<= 159 )
	    ch1 -= 112;
	else
	    ch1 -= 176;
	ch1 <<= 1;
	if ( ch2>=159 )
	    ch2-= 126;
	else if ( ch2>127 ) {
	    --ch1;
	    ch2 -= 32;
	} else {
	    --ch1;
	    ch2 -= 31;
	}
return( unicode_from_jis208[(ch1-0x21)*94+(ch2-0x21)]);
    } else if ( enc == e_euc ) {
	ch1 = charval(buffer);
	if ( ch1 < 0x80 )
return( ch1 );
	ch2 = charval(buffer);
	if ( ch1 == 0x8e )	/* SS1: katakana */
return( unicode_from_jis201[ch2] );
	else if ( ch1 != 0x8f )
return( unicode_from_jis208[(ch1-0xa1)*94+(ch2-0xa1)]);
	else {			/* SS2: suppl. Kanji */
            ch1 = ch2;
	    ch2 = charval(buffer);
return( unicode_from_jis212[(ch1-0xa1)*94+(ch2-0xa1)]);
	}
    } else {
	fprintf( stderr, "Don't support this encoding\n" );
	exit( 1 );
    }
return( -1 );
}

static unichar_t *slurpchars(char *filename, char *name,int enc,char *buffer) {
    unichar_t space[1024], *pt;
    const unichar_t *table = unicode_from_alphabets[enc==e_utf8?e_iso8859_1:enc];

    while ( isspace( *buffer )) ++buffer;
    if ( *buffer=='{' ) ++ buffer;
    while ( isspace( *buffer )) ++buffer;
    if ( *buffer=='\'' ) {
	++buffer;
	space[0] = table[charval(&buffer)];
	space[1] = '\0';
    } else if ( *buffer=='"' ) {
	++buffer;
	pt = space;
	if ( enc==e_utf8 ) {
	    while ( *buffer!='"' && *buffer!= '\0' ) {
		int ch1, ch2;
		ch1 = charval(&buffer);
		if ( ch1<=0x7f )
		    *pt++ = ch1;
		else if ( (ch1&0xf0)==0xc0 ) {
		    *pt++ = (ch1&0x1f)<<6 | (charval(&buffer)&0x3f);
		} else {
		    ch2 = charval(&buffer);
		    *pt++ = (ch1&0xf)<<6 | ((ch2&0x3f)<<6) | (charval(&buffer)&0x3f);
		}
	    }
	} else {
	    while ( *buffer!='"' && *buffer!= '\0' ) {
		*pt++ = table[charval(&buffer)];
	    }
	}
	*pt = 0;
    } else {
	fprintf( stderr, "Could not parse initializer for %s in %s\n", name, filename );
	space[0] = 0;
    }
return( u_copy(space));
}

static unichar_t *slurp2bytes(char *filename, char *name,int enc,char *buffer) {
    unichar_t space[1024], *pt;

    while ( isspace( *buffer )) ++buffer;
    if ( *buffer=='{' ) ++ buffer;
    while ( isspace( *buffer )) ++buffer;
    if ( *buffer=='\'' ) {
	++buffer;
	space[0] = twocharval(&buffer,enc);
	space[1] = '\0';
    } else if ( *buffer=='"' ) {
	++buffer;
	pt = space;
	while ( *buffer!='"' && *buffer!= '\0' ) {
	    *pt++ = twocharval(&buffer,enc);
	}
	*pt = 0;
    } else {
	fprintf( stderr, "Could not parse initializer for %s in %s\n", name, filename );
	space[0] = 0;
    }
return( u_copy(space));
}

static unichar_t *slurpunichars(char *filename, char *name,char *buffer) {
    unichar_t space[1024], *pt;
    char *end;

    while ( isspace( *buffer )) ++buffer;
    if ( *buffer=='{' ) ++ buffer;
    pt = space;
    while ( 1 ) {
	while ( isspace( *buffer ) || *buffer==',' ) ++buffer;
	if ( *buffer=='}' || *buffer==';' || *buffer=='\0' )
    break;
	if ( *buffer=='\'' ) {
	    ++buffer;
	    while ( *buffer!='\'' && *buffer!='\0' )
		*pt++ = charval(&buffer);
	    if ( *buffer=='\'' ) ++buffer;
	} else {
	    *pt++ = strtol(buffer,&end,0);
	    if ( buffer==end ) {
		fprintf( stderr, "Could not parse initializer for %s in %s\n", name, filename );
return( NULL );
	    }
	    buffer = end;
	}
    }
    *pt = '\0';
return( u_copy(space));
}

static void handleint2(char *filename,int *ivalues,char *buffer,int off) {
    char *pt, *end;
    int ch, index;

    if ( buffer[off]=='_' ) ++off;
    for ( pt = buffer+off; isalnum(*pt) || *pt=='_'; ++pt );
    ch = *pt;
    *pt ='\0';
    if ( buffer[off]=='\0' )
return;
    index = ilookup(buffer+off);
    if ( index==-1 ) {
	fprintf( stderr, "Item num_%s does not exist in the base set of integers, but does in %s\n",
		buffer+off, filename );
return;
    }
    *pt = ch;
    while ( isspace( *pt )) ++pt;
    if ( *pt=='=' ) ++pt;
    ivalues[index] = strtol(pt,&end,0);
    if ( end==buffer ) 
	fprintf( stderr, "Bad numeric value for num_%s in %s\n",
		buffer+off, filename );
}

static int getencoding(char *str) {
    static struct encdata {
	int val;
	char *name;
    } encdata[] = {
	{ e_iso8859_1, "e_iso8859_1" },
	{ e_iso8859_1, "iso8859_1" },
	{ e_iso8859_1, "isolatin1" },
	{ e_iso8859_1, "latin1" },
	{ e_iso8859_2, "e_iso8859_2" },
	{ e_iso8859_2, "latin2" },
	{ e_iso8859_3, "e_iso8859_3" },
	{ e_iso8859_3, "latin3" },
	{ e_iso8859_4, "e_iso8859_4" },
	{ e_iso8859_4, "latin4" },
	{ e_iso8859_5, "e_iso8859_5" },
	{ e_iso8859_5, "isocyrillic" },
	{ e_iso8859_6, "e_iso8859_6" },
	{ e_iso8859_6, "isoarabic" },
	{ e_iso8859_7, "e_iso8859_7" },
	{ e_iso8859_7, "isogreek" },
	{ e_iso8859_8, "e_iso8859_8" },
	{ e_iso8859_8, "isohebrew" },
	{ e_iso8859_9, "e_iso8859_9" },
	{ e_iso8859_9, "latin5" },
	{ e_iso8859_10, "e_iso8859_10" },
	{ e_iso8859_10, "latin6" },
	{ e_iso8859_13, "e_iso8859_13" },
	{ e_iso8859_13, "latin7" },
	{ e_iso8859_14, "e_iso8859_14" },
	{ e_iso8859_14, "latin8" },
	{ e_iso8859_15, "e_iso8859_15" },
	{ e_iso8859_15, "latin0" },
	{ e_iso8859_15, "latin9" },
	{ e_koi8_r, "e_koi8_r" },
	{ e_jis201, "e_jis201" },
	{ e_win, "e_win" },
	{ e_mac, "e_mac" },
	{ e_utf8, "e_utf8" },
	{ e_wansung, "e_wansung" },
	{ e_big5, "e_big5" },
	{ e_johab, "e_johab" },
	{ e_sjis, "e_sjis" },
	{ e_euc, "e_euc" },
	{ e_hexjis, "e_hexjis" },
	{ 0, NULL}};
    int i;
    char *pt;

    while ( isspace(*str)) ++str;
    for ( pt=str; isalnum(*pt) || *pt=='_'; ++pt );
    *pt = '\0';

    for ( i=0; encdata[i].name!=NULL; ++i )
	if ( strmatch(encdata[i].name,str)==0 )
return( encdata[i].val );

return( -1 );
}

static void putshort(FILE *file,int sh) {
    putc((sh>>8)&0xff,file);
    putc(sh&0xff,file);
}

static void putint(FILE *file,int sh) {
    putc((sh>>24)&0xff,file);
    putc((sh>>16)&0xff,file);
    putc((sh>>8)&0xff,file);
    putc(sh&0xff,file);
}

static void ProcessNames(char *filename,char *lc,char *uc) {
    FILE *namef, *out;
    unichar_t **values, *mn, *init;
    int *ivalues;
    char buffer[1025];
    char *pt, *bpt, *npt;
    int off, i, j;
    int isuni, ismn, index, ch;
    int enc=0;
    int missing;
#ifdef __VMS
    stat_t stat_buf;
#else
    struct stat stat_buf;
#endif

    values = calloc(npos+1,sizeof(unichar_t *));
    mn = calloc(npos,sizeof(unichar_t));
    ivalues = malloc((ipos+1)*sizeof(int));
    for ( i=0; i<ipos; ++i ) ivalues[i] = 0x80000000;

    namef = fopen( filename,"r" );
    if ( namef==NULL ) {
	fprintf( stderr, "Could not open file %s for reading\n", filename );
return;
    }

    while( fgets(buffer,sizeof(buffer),namef)!=NULL ) {
	if ( strncmp(buffer,"static ",7)!=0 )
    continue;
	pt = buffer+7;
	if ( (npt = strstr(pt,"const "))!=NULL )
	    pt = npt+6;
	if ( (npt = strstr(pt,"enum "))!=NULL ) {
	    if ( (npt = strstr(npt+5,"encoding "))==NULL )
    continue;
	    if ( (npt = strstr(npt+9,"enc "))==NULL )
    continue;
	    pt = npt+4;
	    while ( isspace(*pt)) ++pt;
	    if ( *pt=='=' ) ++pt;
	    enc = getencoding(pt);
	    if ( enc==-1 ) {
		fprintf(stderr, "Invalid encoding line: %s\n", buffer );
		fclose(namef);
return;
	    }
    continue;
	}
	off = pt-buffer;
	if ( strncmp(buffer+off,"int num_",8)==0 ) {
	    handleint2(filename,ivalues,buffer,off+8);
    continue;
	}
	isuni = 0;
	if ( strncmp(buffer+off,"unichar_t ",10)==0 ) {
	    off += 10;
	    isuni = 1;
	} else if ( strncmp(buffer+off,"char ",5)==0 )
	    off += 5;
	else
    continue;
	if ( buffer[off]=='*' ) ++off;
	ismn = 0;
	if ( strncmp(buffer+off,"mnemonic_",8)==0 ) {
	    off += 8;
	    ismn = 1;
	} else if ( strncmp(buffer+off,"str_",4)==0 )
	    off += 4;
	if ( buffer[off]=='_' ) ++off;
	for ( pt = buffer+off; isalnum(*pt) || *pt=='_'; ++pt );
	ch = *pt;
	*pt ='\0';
	if ( buffer[off]=='\0' )
    continue;
	index = lookup(buffer+off);
	if ( index==-1 ) {
	    fprintf( stderr, "Item %s does not exist in the base set of strings, but does in %s\n",
		    buffer+off, filename );
    continue;
	}
	*pt = ch;
	bpt = pt;
	while ( isspace( *pt )) ++pt;
	if ( *pt=='[' ) {
	    ++pt;
	    while ( isspace( *pt )) ++pt;
	    while ( isspace( *pt )|| isdigit(*pt)) ++pt;
	    if ( *pt==']' ) ++pt;
	    while ( isspace( *pt )) ++pt;
	}
	if ( *pt!='=' ) {
	    *bpt = '\0';
	    fprintf( stderr, "Item %s has no initializer in %s\n",
		    buffer+off, filename );
    continue;
	}
	++pt;
	*bpt = '\0';
	if ( isuni )
	    init = slurpunichars(filename,buffer+off,pt);
	else if ( enc>=e_first2byte )
	    init = slurp2bytes(filename,buffer,enc,pt);
	else
	    init = slurpchars(filename,buffer,enc,pt);
	if ( init==NULL )
    continue;
	if ( ismn ) {
	    mn[index] = init[0];
	    if ( init[1]!='\0' ) fprintf( stderr, "Too many initializers for %s in %s\n", buffer+off, filename );
	    free(init);
	} else
	    values[index] = init;
    }
    fclose(namef);

    if ( values[0]==NULL )
	fprintf( stderr, "No language entry in %s\n", filename );

    for ( i=0; i<npos; ++i )
	if ( values[i]==NULL )
    break;
    for ( j=0; j<ipos; ++j )
	if ( ivalues[j]==0x80000000 )
    break;

    missing = (i!=npos) || (j!=ipos);
    if ( !missing ) {
	/* If the list isn't complete, then can't use it as a fallback */
	strcpy(buffer,lc);
	strcat(buffer,"-ui");
	strcat(buffer,filename+5);
	out = fopen(buffer,"w");
	if ( out==NULL )
	    fprintf( stderr, "Could not open %s for writing\n", buffer);
	else {
	    fprintf( out, "#include \"%sui.h\"\n\n", lc );
	    for ( i=0; i<npos; ++i ) {
		if ( values[i]!=NULL ) {
		    fprintf( out, "static const unichar_t str%d[] = { ", i );
		    for ( j=0; values[i][j]!=0; ++j )
			if ( values[i][j]<127 && values[i][j]>=32 &&
				values[i][j]!='\\' && values[i][j]!='\'' )
			    fprintf( out, "'%c', ", values[i][j]);
			else
			    fprintf( out, "0x%x, ", values[i][j]);
		    fprintf( out, " 0 };\n" );
		}
	    }
	    fprintf( out, "\nstatic const unichar_t *%s_ui_strings[] = {\n", lc );
	    for ( i=0; i<npos; ++i ) {
		if ( values[i]!=NULL )
		    fprintf( out, "\tstr%d,\n", i);
		else
		    fprintf( out, "\tNULL,\n" );
	    }
	    fprintf( out, "\tNULL};\n\n" );
	    fprintf( out, "static const unichar_t %s_ui_mnemonics[] = {", lc );
	    for ( i=0; i<npos; ++i ) {
		if ( (i&0x7)==0 )
		    fprintf( out, "\n\t" );
		if ( mn[i]<127 && mn[i]>=32 && mn[i]!='\\' && mn[i]!='\'' )
		    fprintf( out, "'%c',    ", mn[i]);
		else
		    fprintf( out, "0x%04x, ", mn[i]);
	    }
	    fprintf( out, "\n\t0};\n\n" );
	    fprintf( out, "static const int %s_ui_num[] = {", lc );
	    for ( i=0; i<ipos; ++i ) {
		if ( (i&7)==0 )
		    fprintf(out,"\n    ");
		fprintf( out, "%d, ", ivalues[i] );
	    }
	    fprintf( out, "\n    0x80000000\n};\n\n" );
	    fprintf( out, "void %sSetFallback(void) {\n", uc );
	    fprintf( out, "    GStringSetFallbackArray(%s_ui_strings,%s_ui_mnemonics,%s_ui_num);\n",lc,lc,lc );
	    fprintf( out, "}\n\n" );

	    stat("nomen-en.c",&stat_buf);
	    fprintf( out, "int %sNomenChecksum = %d;\n", uc, (int) stat_buf.st_size );
	    fclose(out);
	}
    }

    /* Make a clean copy of the file */
    buffer[0]= '_';
    strcpy(buffer+1,filename);
    out = fopen(buffer,"w");
    if ( out!=NULL ) {
	fprintf( out, "#include \"nomen.h\"\n\n" );
	fprintf( out, "static enum encoding enc = e_iso8859_1;\n\n" );
	for ( i=0; i<npos; ++i ) if ( values[i]!=NULL ) {
	    for ( j=0; values[i][j]<256 && values[i][j]!=0; ++j );
	    if ( values[i][j]==0 ) {
		fprintf( out, "static char str_%s[] = \"", cu_copy(names[i]));
		for ( j=0; values[i][j]<256 && values[i][j]!=0; ++j ) {
		    if (( (values[i][j]>=32 && values[i][j]<127) ||
			    (values[i][j]>=0xa0 && values[i][j]<256)) &&
			    values[i][j]!='"' && values[i][j]!='\\' )
			putc(values[i][j],out);
		    else
			fprintf(out,"\\%03o", values[i][j]);
		}
		fprintf( out, "\";\n" );
	    } else {
		fprintf( out, "static unichar_t str_%s[] = { ", cu_copy(names[i]));
		for ( j=0; values[i][j]!=0; ++j ) {
		    if (( (values[i][j]>=32 && values[i][j]<127) ||
			    (values[i][j]>=0xa0 && values[i][j]<256)) &&
			    values[i][j]!='"' && values[i][j]!='\\' )
			fprintf(out, "'%c', ", values[i][j]);
		    else
			fprintf(out,"0x%x, ", values[i][j]);
		}
		fprintf( out, " 0 };\n" );
	    }
	    if ( mn[i]!=0 ) {
		fprintf( out, "static unichar_t mnemonic_%s[] = ", cu_copy(names[i]));
		if (( (mn[i]>=32 && mn[i]<127) ||
			(mn[i]>=0xa0 && mn[i]<256)) &&
			mn[i]!='"' && mn[i]!='\\' )
		    fprintf(out, "'%c';\n", mn[i]);
		else
		    fprintf(out,"0x%x;\n", mn[i]);
	    }
	}
	putc('\n',out);
	for ( i=0; i<ipos; ++i ) if ( ivalues[i]!=0x80000000 ) {
	    fprintf( out, "static int num_%s = %d;\n", cu_copy(inames[i]),
		    ivalues[i]);
	}
	if ( missing ) {
	    fprintf( out, "\n\t/* ************** Missing strings ************** */\n\n" );
	    for ( i=0; i<npos; ++i ) {
		if ( values[i]==NULL )
		    fprintf( out, "static unichar_t *str_%s;\n", cu_copy(names[i]));
		if ( hadmn[i] && mn[i]=='\0' )
		    fprintf( out, "static unichar_t mnemonic_%s;\n", cu_copy(names[i]));
	    }
	    putc('\n',out);
	    for ( i=0; i<ipos; ++i ) {
		if ( ivalues[i]==0x80000000 )
		    fprintf( out, "static int num_%s;\n", cu_copy(inames[i]));
	    }
	}
	fclose(out);
    }

    strcpy(buffer,lc);
    strncat(buffer,filename+5,3);
    strcat(buffer,".ui");
    out = fopen(buffer,"wb");
    if ( out==NULL )
	fprintf( stderr, "Could not open %s for writing\n", buffer);
    else {
	int last = -1, ilast = -1;
	putint(out,checksum);
	for ( i=0; i<npos; ++i )
	    if ( values[i]!=NULL ) last = i;
	putshort(out,last+1);
	for ( i=0; i<ipos; ++i )
	    if ( ivalues[i]!=0x80000000 ) ilast = i;
	putshort(out,ilast+1);
	for ( i=0; i<=last; ++i ) if ( values[i]!=NULL ) {
	    putshort( out,i);
	    if ( mn[i]!=0 ) {
		putshort( out,u_strlen(values[i])|0x8000);
		putshort( out, mn[i]);
	    } else
		putshort( out,u_strlen(values[i]));
	    for ( j=0; values[i][j]!=0; ++j )
		putshort(out,values[i][j]);
	}
	for ( i=0; i<=ilast; ++i ) if ( ivalues[i]!=0x80000000 ) {
	    putshort( out,i);
	    putint( out,ivalues[i]);
	}
	fclose(out);
    }

    if ( en_strings==NULL ) {
	en_strings = values;
	en_mn = mn;
	MakeGoodEnglishPO(en_strings,en_mn);
    } else {
	MakePOFile(en_strings,values,mn,ivalues,filename+6);
	for ( i=0; i<npos; ++i ) free( values[i] );
	free( values);
	free( mn );
	free( ivalues);
    }
}

int main(int argc, char **argv) {
    DIR *here;
    struct dirent *file;
    int len;
    int i, pos=0;

    for ( i=1; i<argc; ++i ) {
	if ( strcmp(argv[i],"-po")==0 )
	    make_po = true;
	else if ( pos==0 ) {
	    lc = argv[i];
	    pos++;
	} else
	     uc = argv[i];
     }

    if ( makenomenh())
return( 1 );

    /* Process the english file first to get a list of strings for the po file(s) */
    /*  That means we'll process English twice, but so what? */
    ProcessNames("nomen-en.c",lc,uc);
    if ( make_po )
	mkdir("po",0755);

    /* read all nomen-??*.c files in the current directory */
    here = opendir(".");
    if ( here==NULL )
return( 1 );
    while ( (file = readdir(here))!=NULL ) {
	if ( strncmp(file->d_name,"nomen-",6)!=0 )
    continue;
	len = strlen(file->d_name);
	if ( len<strlen("nomen-en.c") )
    continue;
	if ( strcmp(file->d_name+len-2,".c")!=0 )
    continue;
	ProcessNames(file->d_name,lc,uc);
    }
    closedir(here);

return( 0 );
}
