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
#include <stddef.h>
#include <stdarg.h>
#include "ustring.h"
#include "utype.h"

/* unicode printf. Expect arguments to be given using <num>$ notation */
/* But there's no way I'm going to implement all of printf now. I'll do what I */
/*  think is important and leave the rest for later. Maybe */
/* args begin with 1 */

enum arg_type { at_int, at_double, at_ustr, at_astr, at_iptr };

struct args {
    unsigned int is_alt:1;			/* # flag */
    unsigned int is_zeropad:1;		/* 0 flag */
    unsigned int is_leftadj:1;		/* - flag */
    unsigned int is_blank:1;		/* " " flag */
    unsigned int is_signed:1;		/* + flag */
    unsigned int is_thousand:1;		/* ' flag */
    unsigned int is_short:1;		/* h */
    unsigned int is_long:1; 		/* l */
    unsigned int hasformat:1;		/* else it's a precision/fieldwidth */
    char format;
    int fieldwidth, precision;
    enum arg_type arg_type;
    long ival;
    const unichar_t *uval;
    double dval;
};

struct state {
    int argmax;
    struct args *args;
    unichar_t *opt, *end;
    int cnt;
};

#define addchar(state,ch) (++((state)->cnt),(state)->opt<(state)->end?*((state)->opt)++ = (ch): 0)

static int isspec(int ch) {
    char *str = "%npSscaAgGfFeEouxXdi";

    while ( *str && *str!=ch ) ++str;
return( *str==ch );
}

static void padvalue(struct state *state,int arg,unichar_t *txt,int fieldwidth) {
    int len=0, padc;

    padc = state->args[arg].is_zeropad?'0':' ';
    if ( fieldwidth>0 ) {
	len = u_strlen(txt);
	if ( !state->args[arg].is_leftadj ) {
	    while ( len<fieldwidth ) {
		addchar(state,padc);
		++len;
	    }
	}
    }
    while ( *txt ) {
	addchar(state,*txt);
	++txt;
    }
    while ( len<fieldwidth ) {
	addchar(state,padc);
	++len;
    }
}

static void padstr(struct state *state,int arg,const unichar_t *txt,int fieldwidth, int precision) {
    int len=0, padc,i;

    if ( fieldwidth>0 ) {
	len = precision>0?precision:u_strlen(txt);
	padc = state->args[arg].is_zeropad?'0':' ';
	if ( !state->args[arg].is_leftadj ) {
	    while ( len<fieldwidth ) {
		addchar(state,padc);
		++len;
	    }
	}
    }
    for ( i=0; *txt && (precision==0 || i<precision); ++i, ++txt )
	addchar(state,*txt);
    while ( len<fieldwidth ) {
	addchar(state,padc);
	++len;
    }
}

static void formatarg(struct state *state,int arg) {
    static char *hex = "0123456789abcdef", *HEX="0123456789ABCDEF";
    char *trans;
    int radix, neg; unsigned long val;
    unichar_t buf[20], *pt;
    char cbuf[20];
    int i, precision, fieldwidth;

    if ( arg<0 || arg>=state->argmax )
return;
    if (( precision = state->args[arg].precision )<0 )
	precision = state->args[-state->args[arg].precision-1].ival;
    if (( fieldwidth = state->args[arg].fieldwidth )<0 )
	fieldwidth = state->args[-state->args[arg].fieldwidth-1].ival;
    if ( fieldwidth<0 ) {
	fieldwidth = -fieldwidth;
	state->args[arg].is_leftadj = true;
    }

    switch ( state->args[arg].format ) {
      case 'n':
	*((int *) (state->args[arg].uval)) = state->cnt;
      break;
      case 'c':
        buf[0] = state->args[arg].ival;
	buf[1] = '\0';
	padvalue(state,arg,buf,fieldwidth);
      break;
      case 'd': case 'i': case 'o': case 'x': case 'X': case 'u':
	trans = state->args[arg].format=='X'?HEX:hex;
	pt = buf+sizeof(buf)/sizeof(buf[0])-1;
	*pt-- = '\0';
	neg = false;
	radix = state->args[arg].format=='d' || state->args[arg].format=='i' ||
		    state->args[arg].format=='u'?10:
		state->args[arg].format=='o'?8:16;
	val = state->args[arg].ival;
	if ( state->args[arg].ival<0 &&
		(state->args[arg].format=='d' || state->args[arg].format=='i')) {
	    neg = true;
	    val = -val;
	}
	for ( i=0; val!=0 || i<precision; ++i ) {
	    if ( radix==10 && state->args[arg].is_thousand && i!=0 && i%3==0 )
		*pt-- = ',';			/* !!!!! locale !!!!! */
	    *pt-- = trans[val%radix];
	    val /= radix;
	}
	if ( state->args[arg].is_alt ) {
	    if ( radix==8 && pt[1]!='0' )
		*pt-- = '0';
	    else if ( radix==16 && state->args[arg].ival!=0 ) {
		*pt-- = state->args[arg].format;
		*pt-- = '0';
	    }
	}
	if ( state->args[arg].format=='d' || state->args[arg].format=='i' ) {
	    if ( neg )
		*pt-- = '-';
	    else if ( state->args[arg].is_signed )
		*pt-- = '+';
	    else if ( state->args[arg].is_blank )
		*pt-- = ' ';
	}
	padvalue(state,arg,pt+1,fieldwidth);
      break;
      case 's':
	if ( state->args[arg].uval == NULL ) {
	    static unichar_t null[] = { '<','n','u','l','l','>', '\0' };
	    padstr(state,arg,null,fieldwidth,precision);
	} else if ( state->args[arg].is_short ) {
	    unichar_t *temp = def2u_copy((char *) (state->args[arg].uval));
	    padstr(state,arg,temp,fieldwidth,precision);
	    free(temp);
	} else
	    padstr(state,arg,state->args[arg].uval,fieldwidth,precision);
      break;
      case 'e': case 'E': case 'f': case 'F': case 'g': case 'G': case 'a': case 'A':
	/* This doesn't really do a good job!!!! */
	switch ( state->args[arg].format ) {
	  case 'e': case 'E':
	    sprintf(cbuf,"%e",state->args[arg].dval);
	  break;
	  case 'f': case 'F':
	    sprintf(cbuf,"%f",state->args[arg].dval);
	  break;
	  case 'g': case 'G':
	    sprintf(cbuf,"%g",state->args[arg].dval);
	  break;
	  case 'a': case 'A':
	    sprintf(cbuf,"%a",state->args[arg].dval);
	  break;
        }
	uc_strcpy(buf,cbuf);
	padvalue(state,arg,buf,fieldwidth);
      break;
      /* a 'p' conversion is converted into the equivalent 'x' conversion earlier */
    }
}

int u_vsnprintf(unichar_t *str, int len, const unichar_t *format, va_list ap ) {
    struct state state;
    struct args args[20], temp;
    const unichar_t *pt;
    int argmax = 0, arg, ac, val, hadarg;

    memset(&state,'\0',sizeof(state));
    memset(args,'\0',sizeof(args));
    ac = 0;
    for ( pt=format; *pt;  ) {
	if ( *pt!='%' )
	    ++pt;
	else if ( pt[1]=='%' )
	    pt += 2;
	else {
	    for ( ++pt, arg=0; isdigit(*pt); ++pt )
		arg = 10*arg + tovalue(*pt);
	    ++ac;
	    if ( *pt=='$' ) {
		if ( arg>argmax ) argmax = arg;
	    } else {
		if ( ac>argmax ) argmax = ac;
	    }
	    while ( *pt && !isspec(*pt)) {
		if ( *pt=='*' ) {
		    ++ac;
		    ++pt;
		    for ( ++pt, arg=0; isdigit(*pt); ++pt )
			arg = 10*arg + tovalue(*pt);
		    if ( *pt=='$' ) {
			if ( arg>argmax ) argmax = arg;
		    } else {
			if ( ac>argmax ) argmax = ac;
		    }
		}
		++pt;
	    }
	}
    }
    state.argmax = argmax;
    if ( argmax>sizeof(args)/sizeof(args[0]) )
	state.args = (struct args *) calloc(argmax,sizeof(struct args));
    else
	state.args = args;
    state.opt = str; state.end = str+len;

    ac = 1;
    for ( pt=format; *pt; ) {
	if ( *pt!='%' )
	    ++pt;
	else if ( pt[1]=='%' )
	    pt+=2;
	else {
	    ++pt;
	    memset(&temp,'\0',sizeof(temp));
	    hadarg = 0;
	    if ( isdigit(*pt)) {
		for ( arg=0; isdigit(*pt); ++pt )
		    arg = 10*arg + tovalue(*pt);
		if ( *pt=='$' ) {
		    hadarg = true;
		    ++pt;
		} else
		    temp.fieldwidth = arg;
	    }
	    while ( 1 ) {
		if ( *pt=='#' ) temp.is_alt=true;
		else if ( *pt=='0' ) temp.is_zeropad=true;
		else if ( *pt=='-' ) temp.is_leftadj=true;
		else if ( *pt==' ' ) temp.is_blank=true;
		else if ( *pt=='+' ) temp.is_signed=true;
		else if ( *pt=='\'' ) temp.is_thousand=true;
		else
	    break;
		++pt;
	    }
	    if ( *pt=='*' ) {
		temp.fieldwidth = -ac++;
		for ( ++pt, val=0; isdigit(*pt); ++pt )
		    val = 10*val + tovalue(*pt);
		if ( *pt=='$' ) temp.fieldwidth = -val;
	    } else if ( isdigit(*pt)) {
		while ( isdigit(*pt)) {
		    temp.fieldwidth = 10*temp.fieldwidth + tovalue(*pt);
		    ++pt;
		}
	    }
	    temp.precision = 0x800000;
	    if ( *pt=='.' ) {
		++pt;
		if ( *pt=='*' ) {
		    temp.precision = -ac++;
		    for ( ++pt, val=0; isdigit(*pt); ++pt )
			val = 10*val + tovalue(*pt);
		    if ( *pt=='$' ) temp.precision = -val;
		} else if ( isdigit(*pt)) {
		    temp.precision = 0;
		    while ( isdigit(*pt)) {
			temp.precision = 10*temp.precision + tovalue(*pt);
			++pt;
		    }
		}
	    }
	    if ( *pt=='h' ) { temp.is_short=true; ++pt; }
	    else if ( *pt=='l' ) { temp.is_long=true; ++pt; }
	    if ( temp.fieldwidth<0 )
		state.args[-temp.fieldwidth-1].arg_type = at_int;
	    if ( temp.precision<0 )
		state.args[-temp.precision-1].arg_type = at_int;
	    temp.format = *pt++;
	    temp.hasformat = true;
	    if ( temp.format=='d' || temp.format=='i' || temp.format=='o' ||
		    temp.format=='u' || temp.format=='x' || temp.format=='X' ||
		    temp.format=='c' ) {
		temp.arg_type = at_int;
		if ( temp.precision == (int) 0x800000 ) temp.precision = 1;
	    } else if ( temp.format=='e' || temp.format=='E' || temp.format=='f' ||
		    temp.format=='F' || temp.format=='g' || temp.format=='G' ) {
		temp.arg_type = at_double;
		if ( temp.precision == (int) 0x800000 ) temp.precision = 6;
	    } else if ( temp.format=='a' || temp.format=='A' ) {
		    /* aA hex conversion of double */
		temp.arg_type = at_double;
		if ( temp.precision == (int) 0x800000 ) temp.precision = 2*sizeof(double)-2;
	    } else if ( temp.format=='s' && temp.is_short )
		temp.arg_type = at_astr;
	    else if ( temp.format=='s' )
		temp.arg_type = at_ustr;
	    else if ( temp.format=='p' ) {
		temp.arg_type = at_int;
		temp.format = 'x';
		temp.is_alt = true;
		if ( sizeof(int) < sizeof( void * ) )
		    temp.is_long = true;
	    } else if ( temp.format=='n' )
		temp.arg_type = at_iptr;
	    if ( !hadarg ) arg = ac;
	    ++ac;
	    state.args[arg-1] = temp;
	}
    }

    /* Now read the args in order */
    for ( arg=0; arg<argmax; ++arg ) {
	switch ( state.args[arg].arg_type ) {
	  case at_int:
	    if ( state.args[arg].is_long )
		state.args[arg].ival = va_arg(ap,long);
	    else
		state.args[arg].ival = va_arg(ap,int);
	  break;
	  case at_double:
	    state.args[arg].dval = va_arg(ap,double);
	  break;
	  case at_ustr:
	    state.args[arg].uval = va_arg(ap,unichar_t *);
	  break;
	  case at_astr:
	    state.args[arg].uval = (unichar_t *) va_arg(ap,char *);
	  break;
	  case at_iptr:
	    state.args[arg].uval = (unichar_t *) va_arg(ap,int *);
	  break;
	  default:
	    /* Shouldn't get here, if we do, skip one arg */
	    (void) va_arg(ap,int);
	  break;
	}
    }

    ac = 1;
    for ( pt=format; *pt; ) {
	if ( *pt!='%' ) {
	    addchar(&state,*pt);
	    ++pt;
	} else if ( pt[1]=='%' ) {
	    addchar(&state,'%');
	    pt+=2;
	} else {
	    for ( ++pt, arg=0; isdigit(*pt); ++pt )
		arg = 10*arg + tovalue(*pt);
	    if ( *pt!='$' ) {
		arg = ac;
		if ( !state.args[arg-1].hasformat ) ++arg;
		if ( !state.args[arg-1].hasformat ) ++arg;
	    }
	    if ( state.args[arg-1].fieldwidth<0 ) ++ac;
	    if ( state.args[arg-1].precision<0 && state.args[arg-1].precision!= (int) 0x800000)
		++ac;
	    ++ac;
	    while ( *pt && !isspec(*pt)) ++pt;
	    formatarg(&state,arg-1);
	    ++pt;
	}
    }
    addchar(&state,'\0');
    if ( state.args!=args ) free(state.args);
return( state.cnt-1 );		/* don't include trailing nul */
}

int u_snprintf(unichar_t *str, int len, const unichar_t *format, ... ) {
    va_list ap;
    int ret;

    va_start(ap,format);
    ret = u_vsnprintf(str,len,format,ap);
    va_end(ap);
return( ret );
}

int u_sprintf(unichar_t *str, const unichar_t *format, ... ) {
    va_list ap;
    int ret;

    va_start(ap,format);
    ret = u_vsnprintf(str,0x10000,format,ap);
    va_end(ap);
return( ret );
}
