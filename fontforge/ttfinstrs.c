/* Copyright (C) 2001-2012 by George Williams */
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

#include "ttfinstrs.h"

#include "fontforgevw.h"
#include "ttf.h"
#include "ustring.h"
#include "utype.h"

const char *ff_ttf_instrnames[] = {
    "SVTCA[y-axis]",
    "SVTCA[x-axis]",
    "SPVTCA[y-axis]",
    "SPVTCA[x-axis]",
    "SFVTCA[y-axis]",
    "SFVTCA[x-axis]",
    "SPVTL[parallel]",
    "SPVTL[orthog]",
    "SFVTL[parallel]",
    "SFVTL[orthog]",
    "SPVFS",
    "SFVFS",
    "GPV",
    "GFV",
    "SFVTPV",
    "ISECT",
    "SRP0",
    "SRP1",
    "SRP2",
    "SZP0",
    "SZP1",
    "SZP2",
    "SZPS",
    "SLOOP",
    "RTG",
    "RTHG",
    "SMD",
    "ELSE",
    "JMPR",
    "SCVTCI",
    "SSWCI",
    "SSW",
    "DUP",
    "POP",
    "CLEAR",
    "SWAP",
    "DEPTH",
    "CINDEX",
    "MINDEX",
    "ALIGNPTS",
    "Unknown28",
    "UTP",
    "LOOPCALL",
    "CALL",
    "FDEF",
    "ENDF",
    "MDAP[no-rnd]",
    "MDAP[rnd]",
    "IUP[y]",
    "IUP[x]",
    "SHP[rp2]",
    "SHP[rp1]",
    "SHC[rp2]",
    "SHC[rp1]",
    "SHZ[rp2]",
    "SHZ[rp1]",
    "SHPIX",
    "IP",
    "MSIRP[no-rp0]",
    "MSIRP[rp0]",
    "ALIGNRP",
    "RTDG",
    "MIAP[no-rnd]",
    "MIAP[rnd]",
    "NPUSHB",
    "NPUSHW",
    "WS",
    "RS",
    "WCVTP",
    "RCVT",
    "GC[cur]",
    "GC[orig]",
    "SCFS",
    "MD[grid]",
    "MD[orig]",
    "MPPEM",
    "MPS",
    "FLIPON",
    "FLIPOFF",
    "DEBUG",
    "LT",
    "LTEQ",
    "GT",
    "GTEQ",
    "EQ",
    "NEQ",
    "ODD",
    "EVEN",
    "IF",
    "EIF",
    "AND",
    "OR",
    "NOT",
    "DELTAP1",
    "SDB",
    "SDS",
    "ADD",
    "SUB",
    "DIV",
    "MUL",
    "ABS",
    "NEG",
    "FLOOR",
    "CEILING",
    "ROUND[Grey]",
    "ROUND[Black]",
    "ROUND[White]",
    "ROUND[Undef4]",
    "NROUND[Grey]",
    "NROUND[Black]",
    "NROUND[White]",
    "NROUND[Undef4]",
    "WCVTF",
    "DELTAP2",
    "DELTAP3",
    "DELTAC1",
    "DELTAC2",
    "DELTAC3",
    "SROUND",
    "S45ROUND",
    "JROT",
    "JROF",
    "ROFF",
    "Unknown7B",
    "RUTG",
    "RDTG",
    "SANGW",
    "AA",
    "FLIPPT",
    "FLIPRGON",
    "FLIPRGOFF",
    "Unknown83",
    "Unknown84",
    "SCANCTRL",
    "SDPVTL[parallel]",
    "SDPVTL[orthog]",
    "GETINFO",
    "IDEF",
    "ROLL",
    "MAX",
    "MIN",
    "SCANTYPE",
    "INSTCTRL",
    "Unknown8F",
    "Unknown90",
    "Unknown91",
    "Unknown92",
    "Unknown93",
    "Unknown94",
    "Unknown95",
    "Unknown96",
    "Unknown97",
    "Unknown98",
    "Unknown99",
    "Unknown9A",
    "Unknown9B",
    "Unknown9C",
    "Unknown9D",
    "Unknown9E",
    "Unknown9F",
    "UnknownA0",
    "UnknownA1",
    "UnknownA2",
    "UnknownA3",
    "UnknownA4",
    "UnknownA5",
    "UnknownA6",
    "UnknownA7",
    "UnknownA8",
    "UnknownA9",
    "UnknownAA",
    "UnknownAB",
    "UnknownAC",
    "UnknownAD",
    "UnknownAE",
    "UnknownAF",
    "PUSHB_1",
    "PUSHB_2",
    "PUSHB_3",
    "PUSHB_4",
    "PUSHB_5",
    "PUSHB_6",
    "PUSHB_7",
    "PUSHB_8",
    "PUSHW_1",
    "PUSHW_2",
    "PUSHW_3",
    "PUSHW_4",
    "PUSHW_5",
    "PUSHW_6",
    "PUSHW_7",
    "PUSHW_8",
    "MDRP[grey]",
    "MDRP[black]",
    "MDRP[white]",
    "MDRP03",
    "MDRP[rnd,grey]",
    "MDRP[rnd,black]",
    "MDRP[rnd,white]",
    "MDRP07",
    "MDRP[min,grey]",
    "MDRP[min,black]",
    "MDRP[min,white]",
    "MDRP0b",
    "MDRP[min,rnd,grey]",
    "MDRP[min,rnd,black]",
    "MDRP[min,rnd,white]",
    "MDRP0f",
    "MDRP[rp0,grey]",
    "MDRP[rp0,black]",
    "MDRP[rp0,white]",
    "MDRP13",
    "MDRP[rp0,rnd,grey]",
    "MDRP[rp0,rnd,black]",
    "MDRP[rp0,rnd,white]",
    "MDRP17",
    "MDRP[rp0,min,grey]",
    "MDRP[rp0,min,black]",
    "MDRP[rp0,min,white]",
    "MDRP1b",
    "MDRP[rp0,min,rnd,grey]",
    "MDRP[rp0,min,rnd,black]",
    "MDRP[rp0,min,rnd,white]",
    "MDRP1f",
    "MIRP[grey]",
    "MIRP[black]",
    "MIRP[white]",
    "MIRP03",
    "MIRP[rnd,grey]",
    "MIRP[rnd,black]",
    "MIRP[rnd,white]",
    "MIRP07",
    "MIRP[min,grey]",
    "MIRP[min,black]",
    "MIRP[min,white]",
    "MIRP0b",
    "MIRP[min,rnd,grey]",
    "MIRP[min,rnd,black]",
    "MIRP[min,rnd,white]",
    "MIRP0f",
    "MIRP[rp0,grey]",
    "MIRP[rp0,black]",
    "MIRP[rp0,white]",
    "MIRP13",
    "MIRP[rp0,rnd,grey]",
    "MIRP[rp0,rnd,black]",
    "MIRP[rp0,rnd,white]",
    "MIRP17",
    "MIRP[rp0,min,grey]",
    "MIRP[rp0,min,black]",
    "MIRP[rp0,min,white]",
    "MIRP1b",
    "MIRP[rp0,min,rnd,grey]",
    "MIRP[rp0,min,rnd,black]",
    "MIRP[rp0,min,rnd,white]",
    "MIRP1f"
};

uint8_t *_IVParse(SplineFont *sf, char *text, int *len,
	void (*IVError)(void *,char *, int), void *iv) {
    short numberstack[256];
    int npos=0, nread, i;
    int push_left= 0, push_size=0;
    char *pt;
    char *end, *bend, *brack;
    int icnt=0, imax=strlen(text)/2, val, temp;
    uint8_t *instrs = (uint8_t*) malloc(imax);

    for ( pt = text; *pt ; ++pt ) {
	npos = 0;
	while ( npos<256 ) {
	    while ( *pt==' ' || *pt=='\t' ) ++pt;
	    if ( isdigit( *pt ) || *pt=='-' ) {
		val = strtol(pt,&end,0);
		if ( val>32767 || val<-32768 ) {
		    IVError(iv,_("A value must be between [-32768,32767]"),pt-text);
                    free(instrs);
return( NULL );
		}

		pt = end;

		if ( *pt == '@' ) { /* a delta control byte */
		    if ( val>8 || val<-8 || val==0 ) {
			IVError(iv,_("A value must be between [-8,-1] or [1,8]"),pt-text);
                        free(instrs);
return( NULL );
		    }

		    pt++;

		    if ( !isdigit( *pt ) ) {
			IVError(iv,_("Number expected"),pt-text);
                        free(instrs);
return( NULL );
		    }

		    temp = val;
		    val = strtol(pt,&end,0);

		    if ( val>15 || val<0 ) {
			IVError(iv,_("A value must be between [0,15]"),pt-text);
                        free(instrs);
return( NULL );
		    }

		    val *= 16;
		    if (temp < 0) temp += 8;
		    else temp += 7;
		    val += temp;
		    pt = end;
		}

		numberstack[npos++] = val;
	    } else if ( strnmatch(pt,"cvt",3)==0 ) {
		pt += 3;
		while ( *pt==' ' || *pt=='\t' ) ++pt;
		if ( *pt!='(' ) {
		    IVError(iv,_("Missing left parenthesis in command to get a cvt index"),pt-text);
                    free(instrs);
return( NULL );
		}
		temp = strtol(pt+1,&end,0);
		pt = end;
		while ( *pt==' ' || *pt=='\t' ) ++pt;
		if ( *pt!=')' ) {
		    IVError(iv,_("Missing right paren in command to get a cvt index"),pt-text);
                    free(instrs);
return( NULL );
		}
		numberstack[npos++] = TTF__getcvtval(sf,temp);
		++pt;
	    } else
	break;
	}
	while ( *pt==' ' || *pt=='\t' ) ++pt;
	if ( npos==0 && (*pt=='\r' || *pt=='\n' || *pt=='\0') )
    continue;
	nread = 0;
	if ( push_left==-1 ) {
	    /* we need a push count */
	    if ( npos==0 )
		IVError(iv,_("Expected a number for a push count"),pt-text);
	    else if ( numberstack[0]>255 || numberstack[0]<=0 ) {
		IVError(iv,_("The push count must be a number between 0 and 255"),pt-text);
                free(instrs);
return( NULL );
	    } else {
		nread = 1;
		instrs[icnt++] = numberstack[0];
		push_left = numberstack[0];
	    }
	}
	if ( push_left!=0 && push_left<npos-nread && (*pt=='\r' || *pt=='\n' || *pt=='\0') ) {
	    IVError(iv,_("More pushes specified than needed"),pt-text);
            free(instrs);
return( NULL );
	}
	while ( push_left>0 && nread<npos ) {
	    if ( push_size==2 ) {
		instrs[icnt++] = numberstack[nread]>>8;
		instrs[icnt++] = numberstack[nread++]&0xff;
	    } else if ( numberstack[0]>255 || numberstack[0]<0 ) {
		IVError(iv,_("A value to be pushed by a byte push must be between 0 and 255"),pt-text);
                free(instrs);
return( NULL );
	    } else
		instrs[icnt++] = numberstack[nread++];
	    --push_left;
	}
	if ( nread<npos && push_left==0 && (*pt=='\r' || *pt=='\n' || *pt=='\0')) {
	    IVError(iv,_("Unexpected number"),pt-text);
            free(instrs);
return( NULL );
	}
	if ( *pt=='\r' || *pt=='\n' || *pt=='\0' )
    continue;
	if ( push_left>0 ) {
	    IVError(iv,_("Missing pushes"),pt-text);
            free(instrs);
return( NULL );
	}
	while ( nread<npos ) {
	    i = nread;
	    if ( numberstack[nread]>=0 && numberstack[nread]<=255 ) {
		while ( i<npos && numberstack[i]>=0 && numberstack[i]<=255 )
		    ++i;
		if ( i-nread<=8 )
		    instrs[icnt++] = ttf_pushb+(i-nread)-1;
		else {
		    instrs[icnt++] = ttf_npushb;
		    instrs[icnt++] = i-nread;
		}
		while ( nread<i )
		    instrs[icnt++] = numberstack[nread++];
	    } else {
		while ( i<npos && (numberstack[i]<0 || numberstack[i]>255) )
		    ++i;
		if ( i-nread<=8 )
		    instrs[icnt++] = ttf_pushw+(i-nread)-1;
		else {
		    instrs[icnt++] = ttf_npushw;
		    instrs[icnt++] = i-nread;
		}
		while ( nread<i ) {
		    instrs[icnt++] = numberstack[nread]>>8;
		    instrs[icnt++] = numberstack[nread++]&0xff;
		}
	    }
	}
	brack = NULL;
	for ( end= pt; *end!='\r' && *end!='\n' && *end!=' ' && *end!='\0'; ++end )
	    if ( *end=='[' || *end=='_' ) brack=end;
	for ( i=0; i<256; ++i )
	    if ( strnmatch(pt,ff_ttf_instrnames[i],end-pt)==0 && end-pt==strlen(ff_ttf_instrnames[i]))
	break;
	if ( i==256 && brack!=NULL ) {
	    for ( i=0; i<256; ++i )
		if ( strnmatch(pt,ff_ttf_instrnames[i],brack-pt+1)==0 ) 
	    break;
	    val = strtol(brack+1,&bend,2);	/* Stuff in brackets should be in binary */
	    while ( *bend==' ' || *bend=='\t' ) ++bend;
	    if ( *bend!=']' ) {
		IVError(iv,_("Missing right bracket in command (or bad binary value in bracket)"),pt-text);
                free(instrs);
return( NULL );
	    }
	    if ( val>=32 ) {
		IVError(iv,_("Bracketted value is too large"),pt-text);
                free(instrs);
return( NULL );
	    }
	    i += val;
	}
	pt = end;
	instrs[icnt++] = i;
	if ( i==ttf_npushb || i==ttf_npushw || (i>=ttf_pushb && i<=ttf_pushw+7)) {
	    push_size = (i==ttf_npushb || (i>=ttf_pushb && i<=ttf_pushb+7))? 1 : 2;
	    if ( i==ttf_npushb || i==ttf_npushw )
		push_left = -1;
	    else if ( i>=ttf_pushb && i<=ttf_pushb+7 )
		push_left = i-ttf_pushb+1;
	    else
		push_left = i-ttf_pushw+1;
	}
	if ( *pt=='\0' )
    break;
    }
    *len = icnt;
return( realloc(instrs,icnt==0?1:icnt));	/* some versions of realloc abort on 0 */
}

int instr_typify(struct instrdata *id) {
    int i, len = id->instr_cnt, cnt, j, lh;
    uint8_t *instrs = id->instrs;
    uint8_t *bts;

    if ( id->bts==NULL )
	id->bts = (uint8_t*) malloc(len+1);
    bts = id->bts;
    for ( i=lh=0; i<len; ++i ) {
	bts[i] = bt_instr;
	++lh;
	if ( instrs[i]==ttf_npushb ) {
	    /* NPUSHB */
	    bts[++i] = bt_cnt;
	    cnt = instrs[i];
	    for ( j=0 ; j<cnt; ++j)
		bts[++i] = bt_byte;
	    lh += 1+cnt;
	} else if ( instrs[i]==ttf_npushw ) {
	    /* NPUSHW */
	    bts[++i] = bt_cnt; ++lh;
	    cnt = instrs[i];
	    for ( j=0 ; j<cnt; ++j) {
		bts[++i] = bt_wordhi;
		bts[++i] = bt_wordlo;
	    }
	    lh += 1+cnt;
	} else if ( (instrs[i]&0xf8) == 0xb0 ) {
	    /* PUSHB[n] */
	    cnt = (instrs[i]&7)+1;
	    for ( j=0 ; j<cnt; ++j)
		bts[++i] = bt_byte;
	    lh += cnt;
	} else if ( (instrs[i]&0xf8) == 0xb8 ) {
	    /* PUSHW[n] */
	    cnt = (instrs[i]&7)+1;
	    for ( j=0 ; j<cnt; ++j) {
		bts[++i] = bt_wordhi;
		bts[++i] = bt_wordlo;
	    }
	    lh += cnt;
	}
    }
    bts[i] = bt_impliedreturn;
return( lh );
}

char *__IVUnParseInstrs(InstrBase *iv) {
    char *ubuf, *pt;
    int i,l;

    pt = ubuf = iv->offset = iv->scroll = (char*) malloc((iv->instrdata->instr_cnt*20+1));
    for ( i=l=0; i<iv->instrdata->instr_cnt; ++i ) {
	if ( iv->lpos == l )
	    iv->scroll = pt;
	if ( iv->isel_pos == l )
	    iv->offset = pt;
	if ( iv->instrdata->bts[i]==bt_wordhi ) {
	    sprintf( pt, " %d", (short) ((iv->instrdata->instrs[i]<<8) | iv->instrdata->instrs[i+1]) );
	    ++i;
	} else if ( iv->instrdata->bts[i]==bt_cnt || iv->instrdata->bts[i]==bt_byte ) {
	    sprintf( pt, " %d", iv->instrdata->instrs[i]);
	} else {
	    strcpy(pt, ff_ttf_instrnames[iv->instrdata->instrs[i]]);
	}
	pt += strlen(pt);
	*pt++ = '\n';
	++l;
    }
    *pt = '\0';
return( ubuf );
}

char *_IVUnParseInstrs(uint8_t *instrs,int instr_cnt) {
    struct instrbase iv;
    struct instrdata id;
    char *ret;

    memset(&iv,0,sizeof(iv));
    memset(&id,0,sizeof(id));
    iv.instrdata = &id;
    id.instr_cnt = instr_cnt;
    id.instrs = instrs;
    instr_typify(&id);
    ret = __IVUnParseInstrs(&iv);
    free(id.bts);
return( ret );
}
