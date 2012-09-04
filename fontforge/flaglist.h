/* flaglist.h */
#ifndef _FLAGLIST_H_
#define _FLAGLIST_H_

struct flaglist { const char *name; int flag; };
#define FLAGLIST_EMPTY { NULL, 0 }
#define FLAG_UNKNOWN ((int32)0x80000000)

int FindFlagByName( struct flaglist *flaglist, const char *name );
const char *FindNameOfFlag( struct flaglist *flaglist, int flag );

#endif
