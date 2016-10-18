#ifndef FONTFORGE_ENCODING_H
#define FONTFORGE_ENCODING_H

#include "baseviews.h"
#include "splinefont.h"

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
extern char *ParseEncodingFile(char *filename, char *encodingname);
extern char *SFEncodingName(SplineFont *sf, EncMap *map);
extern const char *FindUnicharName(void);
extern EncMap *CompactEncMap(EncMap *map, SplineFont *sf);
extern EncMap *EncMapFromEncoding(SplineFont *sf, Encoding *enc);
extern Encoding *FindOrMakeEncoding(const char *name);
extern Encoding *_FindOrMakeEncoding(const char *name, int make_it);
extern int32 EncFromName(const char *name, enum uni_interp interp, Encoding *encname);
extern int32 EncFromUni(int32 uni, Encoding *enc);
extern int32 UniFromEnc(int enc, Encoding *encname);

/* The "Encoding" here is a little different from what you normally see*/
/*  It isn't a mapping from a byte stream to unicode, but from an int */
/*  to unicode. If we have an 8/16 encoding (EUC or SJIS) then the */
/*  single byte entries will be numbers less than <256 and the */
/*  multibyte entries will be numbers >=256. So an encoding might be */
/*  valid for the domain [0x20..0x7f] [0xa1a1..0xfefe] */
/* In other words, we're interested in the ordering displayed in the */
/*  fontview. Nothing else */
/* The max value need not be exact (though it should be at least as big)*/
/*  if you create a new font with the given encoding, then the font will */
/*  have max slots in it by default */
/* A return value of -1 (from an EncFunc) indicates no mapping */
/* AddEncoding returns 1 if the encoding was added, 2 if it replaced */
/*  an existing encoding, 0 if you attempt to replace a builtin */
/*  encoding */
typedef int (*EncFunc)(int);
extern int AddEncoding(char *name, EncFunc enc_to_uni, EncFunc uni_to_enc, int max);

extern int CID2NameUni(struct cidmap *map, int cid, char *buffer, int len);
extern int CID2Uni(struct cidmap *map, int cid);
extern int CIDFromName(char *name, SplineFont *cidmaster);
extern int CountOfEncoding(Encoding *encoding_name);
extern int MaxCID(struct cidmap *map);
extern int NameUni2CID(struct cidmap *map, int uni, const char *name);
extern int SFFlattenByCMap(SplineFont *sf, char *cmapname);
extern int SFForceEncoding(SplineFont *sf, EncMap *old, Encoding *new_enc);
extern int SFReencode(SplineFont *sf, const char *encname, int force);
extern SplineFont *CIDFlatten(SplineFont *cidmaster, SplineChar **glyphs, int charcnt);
extern SplineFont *MakeCIDMaster(SplineFont *sf, EncMap *oldmap, int bycmap, char *cmapfilename, struct cidmap *cidmap);
extern struct altuni *CIDSetAltUnis(struct cidmap *map, int cid);
extern struct cidmap *FindCidMap(char *registry, char *ordering, int supplement, SplineFont *sf);
extern struct cidmap *LoadMapFromFile(char *file, char *registry, char *ordering, int supplement);
extern void BDFOrigFixup(BDFFont *bdf, int orig_cnt, SplineFont *sf);
extern void CIDMasterAsDes(SplineFont *sf);
extern void DumpPfaEditEncodings(void);
extern void FVAddEncodingSlot(FontViewBase *fv, int gid);
extern void LoadPfaEditEncodings(void);
extern void MMMatchGlyphs(MMSet *mm);
extern void SFAddEncodingSlot(SplineFont *sf, int gid);
extern void SFAddGlyphAndEncode(SplineFont *sf, SplineChar *sc, EncMap *basemap, int baseenc);
extern void SFEncodeToMap(SplineFont *sf, struct cidmap *map);
extern void SFExpandGlyphCount(SplineFont *sf, int newcnt);
extern void SFMatchGlyphs(SplineFont *sf, SplineFont *target, int addempties);
extern void SFRemoveGlyph(SplineFont *sf, SplineChar *sc);

#endif /* FONTFORGE_ENCODING_H */
