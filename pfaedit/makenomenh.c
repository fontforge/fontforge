#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <sys/types.h>
#include <dirent.h>

#include <ggadget.h>
#include <ustring.h>
#include <charset.h>
#include <chardata.h>

static char *istandard[] = { "buttonsize", NULL };

static char *standard[] = { "Language", "OK", "Cancel", "Open", "Save",
	"Filter", "New", "Replace", "Fileexists", "Fileexistspre",
	"Fileexistspost", "Createdir", "Dirname", "Couldntcreatedir", NULL };

static unichar_t **names, **inames;
static char *hadmn;
static int nlen=__STR_LastStd+1000, npos=__STR_LastStd+1;
static int ilen=__NUM_LastStd+1, ipos=__NUM_LastStd+1;

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
	inames = realloc(inames,ilen*sizeof(unichar_t*));
    }
    inames[ipos++] = uc_copy(buffer+off);
}

static int makenomenh() {
    char buffer[1025];
    FILE *in, *out;
    char *pt;
    int off, i;
    int ismn;

    names = malloc(nlen*sizeof(unichar_t *));
    hadmn = calloc(nlen,sizeof(char));
    for ( i=0; standard[i]!=NULL; ++i )
	names[i] = uc_copy(standard[i]);

    inames = malloc(ilen*sizeof(unichar_t *));
    for ( i=0; istandard[i]!=NULL; ++i )
	inames[i] = uc_copy(istandard[i]);

    in = fopen("nomen.en.c","r");
    out = fopen("nomen.h","w");
    fprintf( out, "#ifndef _NOMEN_H\n" );
    fprintf( out, "#define _NOMEN_H\n" );
    fprintf( out, "#include <basics.h>\n" );
    fprintf( out, "#include <stdio.h>\n" );
    fprintf( out, "#include <ggadget.h>\n\n" );
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
	    names = realloc(names,nlen*sizeof(unichar_t*));
	    hadmn = realloc(hadmn,nlen);
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
	}
    }
    *buffer = (char *) bpt+1;
return( val );
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
		*pt++ = charval(&buffer);
	    }
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
	    *pt++ = charval(&buffer);
	    while ( *buffer!='\'' && *buffer!='\0' ) ++buffer;
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

static void ProcessNames(char *filename) {
    FILE *namef, *out;
    unichar_t **values, *mn, *init;
    int *ivalues;
    char buffer[1025];
    char *pt, *bpt;
    int off, i, j;
    int isuni, ismn, index, ch;
    int enc=0;
    int missing;

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
	off=7;
	if ( strncmp(buffer+off,"const ",6)==0 )
	    off += 6;
	if ( strncmp(buffer+off,"enum encoding enc =",19)==0 ) {
	    enc = getencoding(buffer+off+19);
	    if ( enc==-1 ) {
		fprintf(stderr, "Invalid encoding line: %s", buffer );
		fclose(namef);
return;
	    }
    continue;
	}
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
	strcpy(buffer,"pfaedit-ui");
	strcat(buffer,filename+5);
	out = fopen(buffer,"w");
	if ( out==NULL )
	    fprintf( stderr, "Could not open %s for writing\n", buffer);
	else {
	    fprintf( out, "#include \"pfaeditui.h\"\n\n" );
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
	    fprintf( out, "\nstatic const unichar_t *pfaedit_ui_strings[] = {\n" );
	    for ( i=0; i<npos; ++i ) {
		if ( values[i]!=NULL )
		    fprintf( out, "\tstr%d,\n", i);
		else
		    fprintf( out, "\tNULL,\n" );
	    }
	    fprintf( out, "\tNULL};\n\n" );
	    fprintf( out, "static const unichar_t pfaedit_ui_mnemonics[] = {" );
	    for ( i=0; i<npos; ++i ) {
		if ( (i&0x7)==0 )
		    fprintf( out, "\n\t" );
		if ( mn[i]<127 && mn[i]>=32 && mn[i]!='\\' && mn[i]!='\'' )
		    fprintf( out, "'%c',    ", mn[i]);
		else
		    fprintf( out, "0x%04x, ", mn[i]);
	    }
	    fprintf( out, "\n\t0};\n\n" );
	    fprintf( out, "static const int pfaedit_ui_num[] = {" );
	    for ( i=0; i<ipos; ++i ) {
		if ( (i&7)==0 )
		    fprintf(out,"\n    ");
		fprintf( out, "%d, ", ivalues[i] );
	    }
	    fprintf( out, "\n    0x80000000\n};\n\n" );
	    fprintf( out, "void PfaEditSetFallback(void) {\n" );
	    fprintf( out, "    GStringSetFallbackArray(pfaedit_ui_strings,pfaedit_ui_mnemonics,pfaedit_ui_num);\n" );
	    fprintf( out, "}\n" );
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
		for ( j=0; values[i][j]<256 && values[i][j]!=0; ++j ) {
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
		if ( values[i]!=NULL )
		    fprintf( out, "static unichar_t *str_%s;\n", cu_copy(names[i]));
		if ( hadmn[i] && mn[i]=='\0' )
		    fprintf( out, "static unichar_t mnemonic_%s;\n", cu_copy(names[i]));
	    }
	    putc('\n',out);
	    for ( i=0; i<ipos; ++i ) {
		if ( ivalues[i]!=0x80000000 )
		    fprintf( out, "static int num_%s;\n", cu_copy(inames[i]));
	    }
	}
	fclose(out);
    }

    strcpy(buffer,"pfaedit");
    strncat(buffer,filename+5,3);
    strcat(buffer,".ui");
    out = fopen(buffer,"wb");
    if ( out==NULL )
	fprintf( stderr, "Could not open %s for writing\n", buffer);
    else {
	int last = -1, ilast = -1;
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

    for ( i=0; i<npos; ++i ) free( values[i] );
    free( values);
    free( mn );
    free( ivalues);
}

int main() {
    DIR *here;
    struct dirent *file;
    int len;

    if ( makenomenh())
return( 1 );

    /* read all nomen.??*.c files in the current directory */
    here = opendir(".");
    if ( here==NULL )
return( 1 );
    while ( (file = readdir(here))!=NULL ) {
	if ( strncmp(file->d_name,"nomen.",6)!=0 )
    continue;
	len = strlen(file->d_name);
	if ( len<strlen("nomen.en.c") )
    continue;
	if ( strcmp(file->d_name+len-2,".c")!=0 )
    continue;
	ProcessNames(file->d_name);
    }
    closedir(here);

return( 0 );
}
