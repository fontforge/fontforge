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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utype.h"
#include "utype.c"

#define ACUTE		0x1
#define GRAVE		0x2
#define DIAERESIS	0x4
#define CIRCUMFLEX	0x8
#define TILDE		0x10
#define RING		0x20
#define SLASH		0x40
#define BREVE		0x80
#define CARON		0x100
#define DOTABOVE	0x200
#define DOTBELOW	0x400
#define CEDILLA		0x800
#define OGONEK		0x1000
#define MACRON		0x2000
#define DBLGRAVE	0x4000
#define DBLACUTE	0x8000
#define INVBREVE	0x10000
#define DIAERESISBELOW	0x20000
#define CIRCUMFLEXBELOW	0x40000
#define TILDEBELOW	0x80000
#define RINGBELOW	0x100000
#define LINEBELOW	0x200000
#define HOOKABOVE	0x400000
#define HORN		0x800000

#define GREEK		0x8000000

#define ANY		(0xfffffff)

struct names { char *name; int mask; } names[] = {
    {"ACUTE", 0x1},
    {"GRAVE", 0x2},
    {"DIAERESIS", 0x4},
    {"CIRCUMFLEX", 0x8},
    {"TILDE", 0x10},
    {"RING", 0x20},
    {"SLASH", 0x40},
    {"BREVE", 0x80},
    {"CARON", 0x100},
    {"DOTABOVE", 0x200},
    {"DOTBELOW", 0x400},
    {"CEDILLA", 0x800},
    {"OGONEK", 0x1000},
    {"MACRON", 0x2000},
    {"DBLGRAVE", 0x4000},
    {"DBLACUTE", 0x8000},
    {"INVBREVE", 0x10000},
    {"DIAERESISBELOW", 0x20000},
    {"CIRCUMFLEXBELOW", 0x40000},
    {"TILDEBELOW", 0x80000},
    {"RINGBELOW", 0x100000},
    {"LINEBELOW", 0x200000},
    {"HOOKABOVE", 0x400000},
    {"HORN",  0x800000},
    {"GREEK", 0x8000000},
    {NULL}};

struct names names2[] = {
    {"ACUTE", 0x1},
    {"GRAVE", 0x2},
    {"DIAERESIS BELOW", 0x20000},
    {"DIAERESIS", 0x4},
    {"CIRCUMFLEX BELOW", 0x40000},
    {"CIRCUMFLEX", 0x8},
    {"TILDE BELOW", 0x80000},
    {"TILDE", 0x10},
    {"RING ABOVE", 0x20},
    {"RING BELOW", 0x100000},
    {"STROKE", 0x40},
    {"SLASH", 0x40},
    {"BREVE", 0x80},
    {"CARON", 0x100},
    {"DOT ABOVE", 0x200},
    {"MIDDLE DOT", 0x200},
    {"DOT BELOW", 0x400},
    {"CEDILLA", 0x800},
    {"OGONEK", 0x1000},
    {"MACRON", 0x2000},
    {"DOUBLE GRAVE", 0x4000},
    {"DOUBLE ACUTE", 0x8000},
    {"INVERTED BREVE", 0x10000},
    {"LINE BELOW", 0x200000},
    {"HOOK ABOVE", 0x400000},
    {"HORN",  0x800000},
    {NULL}};

char *charnames[] = {
/* 0x0020 */	"space","exclam","quotedbl","numbersign","dollar","percent","ampersand","quotesingle",
/* 0x0028 */	"parenleft","parenright","asterisk","plus","comma","hyphenminus","period","slash",
/* 0x0030 */	"zero","one","two","three","four","five","six","seven",
/* 0x0038 */	"eight","nine","colon","semicolon","less","equal","greater","question",
/* 0x0040 */	"at","A","B","C","D","E","F","G",
/* 0x0048 */	"H","I","J","K","L","M","N","O",
/* 0x0050 */	"P","Q","R","S","T","U","V","W",
/* 0x0058 */	"X","Y","Z","bracketleft","backslash","bracketright","asciicircum","underscore",
/* 0x0060 */	"grave","a","b","c","d","e","f","g",
/* 0x0068 */	"h","i","j","k","l","m","n","o",
/* 0x0070 */	"p","q","r","s","t","u","v","w",
/* 0x0078 */	"x","y","z","braceleft","bar","braceright","asciitilde"
};

struct { char ch; unsigned int oldstate, newstate; unsigned short result; }
    predefined[] = {
	{ '\'', ANY, ACUTE },
	{ 'e', 0, ACUTE },
	{ '`', ANY, GRAVE },
	{ ':', ANY, DIAERESIS },
	{ 'u', 0, DIAERESIS },
	{ '^', ANY, CIRCUMFLEX },
	{ 'i', 0, CIRCUMFLEX },
	{ '~', ANY, TILDE },
	{ 'n', 0, TILDE },
	{ '0', ANY, RING },
	{ '/', ANY, SLASH },
	{ '7', ANY, BREVE },
	{ '6', ANY, CARON },
	{ '.', ANY, DOTABOVE },
	{ ',', ANY, DOTBELOW },
	{ '5', ANY, CEDILLA },
	{ '4', ANY, OGONEK },
	{ '_', ANY, MACRON },
	{ '"', ANY, DBLGRAVE },
	{ '@', ANY, GREEK },

	{ ' ', 0, 0, 0x00a0 },		/* no break space */
	{ ' ', GREEK, 0, 0x2001 },	/* em space */
	{ '!', 0, 0, 0x00a1 },		/* inverted exclaim */
	{ '#', 0, 0, 0x00a3 },		/* sterling */
	{ '#', GREEK, 0, 0x00a5 },	/* Yen */
	{ '$', 0, 0, 0x20ac },		/* Euro */
	{ '$', GREEK, 0, 0x00a2 },	/* cent */
	{ '\\',0, 0, 0x00ab },		/* guillemotleft */
	{ '\\',GREEK, 0, 0x2039 },	/* single guillemotleft */
	{ '|', 0, 0, 0x00bb },		/* guillemotright */
	{ '|', GREEK, 0, 0x203a },	/* single guillemotright */
	{ '*', 0, 0, 0x00b0 },		/* degree */
	{ '*', GREEK, 0, 0x2022 },	/* bullet */
	{ '.', GREEK, 0, 0x00b7 },	/* centered dot */
	{ '-', 0, 0, 0x00ad },		/* soft hyphen */
	{ '-', GREEK, 0, 0x2013 },	/* en dash */
	{ '_', GREEK, 0, 0x2014 },	/* em dash */
	{ '=', GREEK, 0, 0x2015 },	/* quote dash */
	{ '+', 0, 0, 0x00b1 },		/* plus minus */
	{ ';', 0, 0, 0x2026 },		/* ellipsis */
	{ '[', 0, 0, 0x2018 },		/* open quote */
	{ ']', 0, 0, 0x2019 },		/* close quote */
	{ '{', 0, 0, 0x201c },		/* open double quote */
	{ '}', 0, 0, 0x201c },		/* close double quote */
	{ '>', 0, 0, 0x2264 },		/* greater than or equal */
	{ '>', GREEK, 0, 0x2023 },	/* triangle bullet */
	{ '<', 0, 0, 0x2265 },		/* less than or equal */
	{ '?', 0, 0, 0x00bf },		/* inverted quest */
	{ 'f', 0, 0, 0x2640 },		/* female */
	{ 'g', 0, 0, 0x00a9 },		/* copyright */
	{ 'h', GREEK|SLASH, 0, 0x210f },/* hbar */
	{ 'h', 0, 0, 0x261e },		/* right hand */
	{ 'H', 0, 0, 0x261e },		/* left hand */
	{ 'm', 0, 0, 0x2642 },		/* male */
	{ 'p', 0, 0, 0x00b6 },		/* paragraph */
	{ 'P', 0, 0, 0x00a7 },		/* section */
	{ 'r', 0, 0, 0x00ae },		/* registered */
	{ 't', 0, 0, 0x2122 },		/* TM */
	{ '2', BREVE, 0, 0x00bd },	/* 1/2 */

/*	{ 'A', 0, 0, 0x00c5 },		/* A ring */
/*	{ 'a', 0, 0, 0x00e5 },		/* a ring */
	{ 'C', 0, 0, 0x00c7 },		/* C cedilla */
	{ 'c', 0, 0, 0x00e7 },		/* c cedilla */
	{ 'A', 0, 0, 0x00c6 },		/* AE */
	{ 'a', 0, 0, 0x00e6 },		/* ae */
	{ 'O', 0, 0, 0x0152 },		/* OE */
	{ 'o', 0, 0, 0x1536 },		/* oe */
	{ 's', 0, 0, 0x00df },		/* es-zet */
	{ 'z', 0, 0, 0x017f },		/* long-s */

	{ 'i', DOTABOVE, 0, 0x131 },	/* dotless i */

/* the mapping from ascii->greek follows the symbol font */
	{ 'A', GREEK, 0, 0x391 },		/* Alpha */
	{ 'B', GREEK, 0, 0x392 },		/* Beta */
	{ 'C', GREEK, 0, 0x3A7 },		/* Chi */
	{ 'D', GREEK, 0, 0x394 },		/* Delta */
	{ 'E', GREEK, 0, 0x395 },		/* Epsilon */
	{ 'F', GREEK, 0, 0x3A6 },		/* Phi */
	{ 'G', GREEK, 0, 0x393 },		/* Gamma */
	{ 'H', GREEK, 0, 0x397 },		/* Eta */
	{ 'I', GREEK, 0, 0x399 },		/* Iota */
	{ 'J', GREEK, 0, 0x3d1 },		/* Theta Symbol */
	{ 'K', GREEK, 0, 0x39A },		/* Kappa */
	{ 'L', GREEK, 0, 0x39B },		/* Lamda */
	{ 'M', GREEK, 0, 0x39C },		/* Mu */
	{ 'N', GREEK, 0, 0x39D },		/* Nu */
	{ 'O', GREEK, 0, 0x39F },		/* Omicron */
	{ 'P', GREEK, 0, 0x3A0 },		/* Pi */
	{ 'Q', GREEK, 0, 0x398 },		/* Theta */
	{ 'R', GREEK, 0, 0x3A1 },		/* Rho */
	{ 'S', GREEK, 0, 0x3A3 },		/* Sigma */
	{ 'T', GREEK, 0, 0x3A4 },		/* Tau */
	{ 'U', GREEK, 0, 0x3A5 },		/* Upsilon */
	{ 'V', GREEK, 0, 0x3c2 },		/* lowercase final sigma */
	{ 'W', GREEK, 0, 0x3A9 },		/* Omega */
	{ 'X', GREEK, 0, 0x39E },		/* Xi */
	{ 'Y', GREEK, 0, 0x3A8 },		/* Psi */
	{ 'Z', GREEK, 0, 0x396 },		/* Zeta */
	{ 'a', GREEK, 0, 0x3b1 },		/* alpha */
	{ 'b', GREEK, 0, 0x3b2 },		/* beta */
	{ 'c', GREEK, 0, 0x3c7 },		/* chi */
	{ 'd', GREEK, 0, 0x3b4 },		/* delta */
	{ 'e', GREEK, 0, 0x3b5 },		/* epsilon */
	{ 'f', GREEK, 0, 0x3c6 },		/* phi */
	{ 'g', GREEK, 0, 0x3b3 },		/* gamma */
	{ 'h', GREEK, 0, 0x3b7 },		/* eta */
	{ 'i', GREEK, 0, 0x3b9 },		/* iota */
	{ 'j', GREEK, 0, 0x3d5 },		/* phi Symbol */
	{ 'k', GREEK, 0, 0x3bA },		/* kappa */
	{ 'l', GREEK, 0, 0x3bB },		/* lamda */
	{ 'm', GREEK, 0, 0x3bC },		/* mu */
	{ 'n', GREEK, 0, 0x3bD },		/* nu */
	{ 'o', GREEK, 0, 0x3bF },		/* omicron */
	{ 'p', GREEK, 0, 0x3c0 },		/* pi */
	{ 'q', GREEK, 0, 0x3b8 },		/* theta */
	{ 'r', GREEK, 0, 0x3c1 },		/* rho */
	{ 's', GREEK, 0, 0x3c3 },		/* sigma */
	{ 't', GREEK, 0, 0x3c4 },		/* tau */
	{ 'u', GREEK, 0, 0x3c5 },		/* upsilon */
	{ 'v', GREEK, 0, 0x3D6 },		/* pi Symbol */
	{ 'w', GREEK, 0, 0x3c9 },		/* omega */
	{ 'x', GREEK, 0, 0x3bE },		/* xi */
	{ 'y', GREEK, 0, 0x3c8 },		/* psi */
	{ 'z', GREEK, 0, 0x3b6 },		/* zeta */
	{ 'A', GREEK|DBLGRAVE, 0, 0x386 },	/* Alpha tonos */
	{ 'A', GREEK|BREVE, 0, 0x1fb8 },	/* Alpha vrachy */
	{ 'A', GREEK|MACRON, 0, 0x1fb9 },	/* Alpha macron */
	{ 'a', GREEK|DBLGRAVE, 0, 0x3ac },	/* alpha tonos */
	{ 'a', GREEK|GRAVE, 0, 0x1f70 },	/* alpha varia */
	{ 'a', GREEK|ACUTE, 0, 0x1f71 },	/* alpha oxia */
	{ 'a', GREEK|BREVE, 0, 0x1fb0 },	/* alpha vrachy */
	{ 'a', GREEK|MACRON, 0, 0x1fb1 },	/* alpha macron */
	{ 'a', GREEK|TILDE, 0, 0x1fb6 },	/* alpha perispomeni */
	{ 'E', GREEK|DBLGRAVE, 0, 0x388 },	/* Epsilon tonos */
	{ 'E', GREEK|GRAVE, 0, 0x1fc8 },	/* Epsilon varia */
	{ 'E', GREEK|ACUTE, 0, 0x1fc9 },	/* Epsilon oxia */
	{ 'e', GREEK|DBLGRAVE, 0, 0x3ad },	/* epsilon tonos */
	{ 'e', GREEK|GRAVE, 0, 0x1f72 },	/* epsilon varia */
	{ 'e', GREEK|ACUTE, 0, 0x1f73 },	/* epsilon oxia */
	{ 'H', GREEK|DBLGRAVE, 0, 0x389 },	/* Eta tonos */
	{ 'H', GREEK|GRAVE, 0, 0x1fca },	/* Eta varia */
	{ 'H', GREEK|ACUTE, 0, 0x1fcb },	/* Eta oxia */
	{ 'h', GREEK|DBLGRAVE, 0, 0x3ae },	/* eta tonos */
	{ 'h', GREEK|GRAVE, 0, 0x1f74 },	/* eta varia */
	{ 'h', GREEK|ACUTE, 0, 0x1f75 },	/* eta oxia */
	{ 'h', GREEK|TILDE, 0, 0x1fc6 },	/* eta perispomeni */
	{ 'I', GREEK|DBLGRAVE, 0, 0x38A },	/* Iota tonos */
	{ 'I', GREEK|DIAERESIS, 0, 0x3AA },	/* Iota dialytika */
	{ 'I', GREEK|GRAVE, 0, 0x1f7a },	/* Iota varia */
	{ 'I', GREEK|ACUTE, 0, 0x1f7b },	/* Iota oxia */
	{ 'I', GREEK|TILDE, 0, 0x1f78 },	/* Iota perispomeni */
	{ 'I', GREEK|MACRON, 0, 0x1f79 },	/* Iota macron */
	{ 'i', GREEK|DBLGRAVE, 0, 0x3af },	/* iota tonos */
	{ 'i', GREEK|DIAERESIS, 0, 0x3ca },	/* iota dialytika */
	{ 'i', GREEK|DBLGRAVE|DIAERESIS, 0, 0x390 },/* iota dialytika tonos */
	{ 'i', GREEK|GRAVE, 0, 0x1f76 },	/* iota varia */
	{ 'i', GREEK|ACUTE, 0, 0x1f77 },	/* iota oxia */
	{ 'i', GREEK|BREVE, 0, 0x1fd0 },	/* iota vrachy */
	{ 'i', GREEK|MACRON, 0, 0x1fd1 },	/* iota macron */
	{ 'i', GREEK|TILDE, 0, 0x1fd6 },	/* iota perispomeni */
	{ 'i', GREEK|GRAVE|DIAERESIS, 0, 0x1fd2},/* iota dialytika varia */
	{ 'i', GREEK|ACUTE|DIAERESIS, 0, 0x1fd3},/* iota dialytika oxia */
	{ 'i', GREEK|TILDE|DIAERESIS, 0, 0x1fd7},/* iota dialytika perispomeni */
	{ 'O', GREEK|DBLGRAVE, 0, 0x38C },	/* Omicron tonos */
	{ 'O', GREEK|GRAVE, 0, 0x1ff8 },	/* Omicron varia */
	{ 'O', GREEK|ACUTE, 0, 0x1ff9 },	/* Omicron oxia */
	{ 'o', GREEK|DBLGRAVE, 0, 0x3cc },	/* omicron tonos */
	{ 'o', GREEK|GRAVE, 0, 0x1f78 },	/* omicron varia */
	{ 'o', GREEK|ACUTE, 0, 0x1f79 },	/* omicron oxia */
	{ 'U', GREEK|DBLGRAVE, 0, 0x38E },	/* Upsilon tonos */
	{ 'U', GREEK|DIAERESIS, 0, 0x3AB },	/* Upsilon dialytika */
	{ 'U', GREEK|GRAVE, 0, 0x1fea },	/* Upsilon varia */
	{ 'U', GREEK|ACUTE, 0, 0x1feb },	/* Upsilon oxia */
	{ 'U', GREEK|BREVE, 0, 0x1fe8 },	/* Upsilon perispomeni */
	{ 'U', GREEK|MACRON, 0, 0x1fe9 },	/* Upsilon macron */
	{ 'u', GREEK|DBLGRAVE, 0, 0x3cd },	/* upsilon tonos */
	{ 'u', GREEK|DIAERESIS, 0, 0x3cb },	/* upsilon dialytika */
	{ 'u', GREEK|DBLGRAVE|DIAERESIS, 0, 0x3b0 },/* upsilon dialytika tonos */
	{ 'u', GREEK|GRAVE, 0, 0x1f7a },	/* upsilon varia */
	{ 'u', GREEK|ACUTE, 0, 0x1f7b },	/* upsilon oxia */
	{ 'u', GREEK|BREVE, 0, 0x1ff0 },	/* upsilon perispomeni */
	{ 'u', GREEK|MACRON, 0, 0x1fe1 },	/* upsilon macron */
	{ 'u', GREEK|GRAVE|DIAERESIS, 0, 0x1fe3 },/* upsilon dialytika varia */
	{ 'u', GREEK|ACUTE|DIAERESIS, 0, 0x1fe4 },/* upsilon dialytika oxia */
	{ 'u', GREEK|TILDE, 0, 0x1fe6 },	/* upsilon perispomeni */
	{ 'u', GREEK|TILDE|DIAERESIS, 0, 0x1fe7 },/* upsilon dialytika perispomeni */
	{ 'W', GREEK|DBLGRAVE, 0, 0x38F },	/* Omega tonos */
	{ 'W', GREEK|GRAVE, 0, 0x1ffa },	/* Omega varia */
	{ 'W', GREEK|ACUTE, 0, 0x1ffb },	/* Omega oxia */
	{ 'w', GREEK|DBLGRAVE, 0, 0x3ce },	/* omega tonos */
	{ 'w', GREEK|GRAVE, 0, 0x1f7a },	/* omega varia */
	{ 'w', GREEK|ACUTE, 0, 0x1f7b },	/* omega oxia */
	{ 0 }
};

struct transform {
    uint32 oldstate;
    uint32 newstate;
    unichar_t resch;
    struct transform *next;
} *info[95] = { 0 };

int queuelen(struct transform *queue) {
    int len=0;

    while ( queue!=NULL ) {
	queue = queue->next;
	++len;
    }
return( len );
}

static char *Mask(char *buffer,int mask) {
    int i,m;
    char *bpt = buffer;

    if ( mask==0 )
return( "0" );
    if ( mask==ANY )
return("ANY");
    *buffer = '\0';
    for (i=0; names[i].name!=NULL; ++i ) {
	if ( names[i].mask&mask ) {
	    if ( bpt!=buffer )
		*bpt++ ='|';
	    strcpy(bpt,names[i].name);
	    bpt += strlen(bpt);
	}
    }
return( buffer );
}

void dumpinfo() {
    FILE *out;
    int i;
    struct transform *t;
    char buffer[400], buffer2[400];

    out = fopen("gdrawbuildchars.c","w");
    fprintf(out, "/* Copyright (C) 2000-2012 by George Williams */\n" );
    fprintf(out, "/*\n * Redistribution and use in source and binary forms, with or without\n" );
    fprintf(out, " * modification, are permitted provided that the following conditions are met:\n *\n" );
    fprintf(out, " * Redistributions of source code must retain the above copyright notice, this\n" );
    fprintf(out, " * list of conditions and the following disclaimer.\n *\n" );
    fprintf(out, " * Redistributions in binary form must reproduce the above copyright notice,\n" );
    fprintf(out, " * this list of conditions and the following disclaimer in the documentation\n" );
    fprintf(out, " * and/or other materials provided with the distribution.\n *\n" );
    fprintf(out, " * The name of the author may not be used to endorse or promote products\n" );
    fprintf(out, " * derived from this software without specific prior written permission.\n *\n" );
    fprintf(out, " * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED\n" );
    fprintf(out, " * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF\n" );
    fprintf(out, " * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO\n" );
    fprintf(out, " * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,\n" );
    fprintf(out, " * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,\n" );
    fprintf(out, " * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;\n" );
    fprintf(out, " * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,\n" );
    fprintf(out, " * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR\n" );
    fprintf(out, " * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF\n" );
    fprintf(out, " * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n */\n\n" );
    fprintf(out, "/* This file was generated using the program 'makebuildtables.c' */\n\n" );
    fprintf(out,"#include \"gdrawP.h\"\n\n" );

    for ( i=0; names[i].name!=NULL; ++i )
	fprintf( out, "#define\t%s\t%s0x%07x\n", names[i].name, \
		 strlen(names[i].name)>7?"":"\t", names[i].mask );
    fprintf(out,"\n#define\tANY\t\t0x%07x\n\n", ANY );

    for ( i=0; i<95; ++i ) if ( info[i]!=NULL ) {
	fprintf(out, "static struct gchr_transform trans_%s[] = {\n", charnames[i] );
	for ( t=info[i]; t!=NULL; t=t->next )
	    fprintf(out, "    { %s, %s, 0x%07x }%s\n", Mask(buffer,t->oldstate),
		    Mask(buffer2,t->newstate), t->resch, t->next==NULL?"":"," );
	fprintf(out,"};\n\n");
    }
    fprintf(out,"struct gchr_lookup _gdraw_chrlookup[95] = {\n" );
    for ( i=0; i<95; ++i ) {
	if ( info[i]==NULL )
	    fprintf(out, "    /* %c */ { 0 },\n", i+' ' );
	else
	    fprintf(out, "    /* %c */ { %d, trans_%s },\n", i+' ', queuelen(info[i]), charnames[i], i+' ' );
    }
    fprintf(out,"};\n\n" );

    fprintf(out, "struct gchr_accents _gdraw_accents[] = {\n" );
    fprintf(out, "    { 0x0301, 0x%07x },\n", ACUTE );
    fprintf(out, "    { 0x0300, 0x%07x },\n", GRAVE );
    fprintf(out, "    { 0x0308, 0x%07x },\n", DIAERESIS );
    fprintf(out, "    { 0x0302, 0x%07x },\n", CIRCUMFLEX );
    fprintf(out, "    { 0x0303, 0x%07x },\n", TILDE );
    fprintf(out, "    { 0x030a, 0x%07x },\n", RING );
    fprintf(out, "    { 0x0338, 0x%07x },\n", SLASH );
    fprintf(out, "    { 0x0306, 0x%07x },\n", BREVE );
    fprintf(out, "    { 0x030c, 0x%07x },\n", CARON );
    fprintf(out, "    { 0x0307, 0x%07x },\n", DOTABOVE );
    fprintf(out, "    { 0x0323, 0x%07x },\n", DOTBELOW );
    fprintf(out, "    { 0x0327, 0x%07x },\n", CEDILLA );
    fprintf(out, "    { 0x0328, 0x%07x },\n", OGONEK );
    fprintf(out, "    { 0x0304, 0x%07x },\n", MACRON );
    fprintf(out, "    { 0x030d, 0x%07x },\n", DBLGRAVE|GREEK );
    fprintf(out, "    { 0x030b, 0x%07x },\n", DBLGRAVE );
    fprintf(out, "    { 0x030b, 0x%07x },\n", DBLACUTE );
    fprintf(out, "    { 0x030b, 0x%07x },\n", INVBREVE );
    fprintf(out, "    { 0x030b, 0x%07x },\n", DIAERESISBELOW );
    fprintf(out, "    { 0x030b, 0x%07x },\n", CIRCUMFLEXBELOW );
    fprintf(out, "    { 0x030b, 0x%07x },\n", TILDEBELOW );
    fprintf(out, "    { 0x030b, 0x%07x },\n", RINGBELOW );
    fprintf(out, "    { 0x030b, 0x%07x },\n", LINEBELOW );
    fprintf(out, "    { 0x030b, 0x%07x },\n", HOOKABOVE );
    fprintf(out, "    { 0x030b, 0x%07x },\n", HORN );
    fprintf(out, "    { 0, 0 },\n" );
    fprintf(out, "};\n\n" );
    fprintf(out, "uint32 _gdraw_chrs_any=ANY, _gdraw_chrs_ctlmask=GREEK, _gdraw_chrs_metamask=0;\n" );
    fclose(out);
}

char *mygets(FILE *in,char *buffer) {
    char *bpt = buffer;
    int ch;

    while ((ch = getc(in))!=EOF && ch!='\n' )
	*bpt++ = ch;
    *bpt = '\0';
    if ( bpt==buffer && ch==EOF )
return( NULL );
return(buffer );
}

void AddTransform(int ch, uint32 oldstate, uint32 newstate, unsigned short resch ) {
    struct transform *trans;

    ch -= ' ';
    for ( trans=info[ch]; trans!=NULL; trans = trans->next )
	if ( trans->oldstate==oldstate ) {
	    fprintf(stderr, "Duplicate entry for %c(%d) at 0x%07x, 0x%07x,0x%07x and 0x%07x,0x%07x\n",
		    ch+' ', ch+' ', oldstate, trans->newstate, trans->resch, newstate, resch );
    break;
	}

    trans = calloc(1,sizeof(struct transform));
    trans->next = info[ch];
    info[ch] = trans;
    trans->oldstate = oldstate;
    trans->newstate = newstate;
    trans->resch = resch;
}

void ParseUnicodeFile(FILE *in) {
    char buffer[600];
    int ch, mask, base, lc, i;
    char *pt;

    while ( mygets(in,buffer)!=NULL ) {
	ch = strtol(buffer,NULL,16);
	if ( ch==0x1ec0 )
	    ch = 0x1ec0;
	pt = buffer+4;
	if ( strncmp(pt,";LATIN ",7)!=0 )
    continue;
	pt += 7;
	if ( strncmp(pt,"CAPITAL ",8)==0 ) {
	    lc = 0;
	    pt += 8;
	} else if ( strncmp(pt,"SMALL ",6)==0 ) {
	    lc = 1;
	    pt += 6;
	} else
    continue;
	if ( strncmp(pt,"LETTER ",7)!=0 )
    continue;
	pt += 7;
	base = *pt++;
	if ( lc ) base = tolower(base);
	if ( strncmp(pt," WITH ",6)!=0 )
    continue;
	pt += 6;
	mask = 0;
	for (;;) {
	    for ( i=0; names2[i].name!=NULL; ++i ) {
		if ( strncmp(pt,names2[i].name,strlen(names2[i].name))==0 )
	    break;
	    }
	    if ( names2[i].name==NULL || names2[i].mask==0 )
    goto continue_2_loop;
	    mask |= names2[i].mask;
	    pt += strlen(names2[i].name);
	    while ( *pt!=';' && !(*pt==' ' && pt[1]=='A' && pt[2]=='N' && pt[3]=='D' && pt[4]==' '))
		++pt;
	    if ( *pt==';' )
	break;
	    else
		pt += 5;
	}
	AddTransform(base,mask,0,ch);
    continue_2_loop:;
    }
    fclose(in);
}

void AddPredefineds() {
    int i;

    for ( i=0; predefined[i].ch!='\0'; ++i )
	AddTransform(predefined[i].ch, predefined[i].oldstate,
		predefined[i].newstate, predefined[i].result );
}

struct transform *RevQueue(struct transform *cur) {
    struct transform *prev=NULL, *next;

    if ( cur==NULL )
return( NULL );
    next = cur->next;
    while ( next!=NULL ) {
	cur->next = prev;
	prev = cur;
	cur = next;
	next = cur->next;
    }
    cur->next = prev;
return( cur );
}

int main() {
    FILE *in;
    int i;

    AddPredefineds();
    in = fopen("UnicodeData.txt","r");
    if ( in==NULL ) {
	fprintf(stderr,"Can't open UnicodeData.txt\n" );
	return( -1 );
    }
    ParseUnicodeFile(in);
    for ( i=0; i<95; ++i )
	info[i] = RevQueue(info[i]);
    dumpinfo();
    return( 0 );
}
