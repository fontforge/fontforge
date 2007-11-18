#ifndef _ENCODING_H
#define _ENCODING_H

struct cidmap {
    char *registry, *ordering;
    int supplement, maxsupple;
    int cidmax;			/* Max cid found in the charset */
    int namemax;		/* Max cid with useful info */
    uint32 *unicode;
    char **name;
    struct cidmap *next;
};

extern struct cidmap *cidmaps;

extern void DeleteEncoding(Encoding *me);
extern void RemoveMultiples(Encoding *item);
#endif
