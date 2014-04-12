#include <basics.h>
#include <string.h>
#include "flaglist.h"

int FindFlagByName( struct flaglist *flags, const char *name )
{
    int i;
    for ( i=0; flags[i].name!=NULL; ++i ) {
	if ( strcmp(name, flags[i].name)==0 )
return( flags[i].flag );
    }
return FLAG_UNKNOWN;
}

const char *FindNameOfFlag( struct flaglist *flags, int flag )
{
    int i;
    for ( i=0; flags[i].name!=NULL; ++i ) {
	if ( flags[i].flag == flag )
return flags[i].name;
    }
return NULL;
}
