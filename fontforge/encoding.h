#ifndef _ENCODING_H
#define _ENCODING_H

struct cidaltuni {
    struct cidaltuni *next;
    int uni;
    int cid;
};

struct cidmap {
    char *registry, *ordering;
    int supplement, maxsupple;
    int cidmax;			/* Max cid found in the charset */
    int namemax;		/* Max cid with useful info */
    uint32 *unicode;
    char **name;
    struct cidaltuni *alts;
    struct cidmap *next;
};

extern struct cidmap *cidmaps;

extern void DeleteEncoding(Encoding *me);
extern void EncodingFree(Encoding *item);
extern void RemoveMultiples(Encoding *item);
#endif
