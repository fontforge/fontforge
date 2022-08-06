#ifndef FONTFORGE_SFD_H
#define FONTFORGE_SFD_H

#include <fontforge-config.h>

#include "splinefont.h"

typedef struct sfd_getfontmetadatadata {
	// these indicate if we saw some metadata or not.
	// perhaps the caller wants to do something special
	// if the metadata was present/missing.
	int hadtimes;
	int had_layer_cnt;

	// state that is mostly interesting to SFD_GetFontMetaData() only
	struct Base*            last_base;
	struct basescript*      last_base_script;
	OTLookup*               lastpotl;
	OTLookup*               lastsotl;
	KernClass*              lastkc;
	KernClass*              lastvkc;
	struct ff_glyphclasses* lastgroup;
	struct ff_rawoffsets*   lastgroupkern;
	struct ff_rawoffsets*   lastgroupvkern;
	FPST*                   lastfp;
	ASM*                    lastsm;
	struct ttf_table*       lastttf[2];
} SFD_GetFontMetaDataData;

extern void SFD_GetFontMetaDataData_Init(SFD_GetFontMetaDataData* d);
extern bool SFD_GetFontMetaData(FILE *sfd, char *tok, SplineFont *sf, SFD_GetFontMetaDataData* d);
extern void SFD_GetFontMetaDataVoid(FILE *sfd, char *tok, SplineFont *sf, void* d);

/**
 * Create, open and unlink a new temporary file. This allows the
 * caller to write to and read from the file without needing to worry
 * about cleaning up the filesystem at all.
 *
 * On Linux, this will create a new file in /tmp with a secure name,
 * open it, and delete the file from the filesystem. The application
 * can still happily use the file as it has it open, but once it is
 * closed or the application itself closes (or crashes) then the file
 * will be expunged for you by the kernel.
 *
 * The caller can fclose() the returned file. Other applications will
 * not be able to find the file by name anymore when this call
 * returns.
 *
 * This function returns 0 if error encountered.
 */
extern FILE* MakeTemporaryFile(void);

/*
 * Convert the contents of a File* to a newly allocated string
 * The caller needs to free the returned string.
 */
extern char* FileToAllocatedString(FILE *f);
extern char *getquotedeol(FILE *sfd);
extern int getname(FILE *sfd, char *tokbuf);
extern void SFDGetKerns(FILE *sfd, SplineChar *sc, char* ttok);
extern void SFDGetPSTs(FILE *sfd, SplineChar *sc, char* ttok);

/**
 * Move the sfd file pointer to the start of the next glyph. Return 0
 * if there is no next glyph. Otherwise, a copy of the string on the
 * line of the SFD file that starts the glyph is returned. The caller
 * should return the return value of this function.
 *
 * If the glyph starts with:
 * StartChar: a
 * The the return value with be "a" (sans the quotes).
 *
 * This is handy if the caller is done with a glyph and just wants to
 * skip to the start of the next one.
 */
extern char* SFDMoveToNextStartChar(FILE* sfd);

/**
 * Some references in the SFD file are to a numeric glyph ID. As a
 * sneaky method to handle that, fontforge will load these glyph
 * numbers into the pointers which should refer to the glyph. For
 * example, in kerning, instead of pointing to the splinechar for the
 * "v" glyph, the ID might be stored there, say the number 143. This
 * fixup function will convert such 143 references to being pointers
 * to the splinechar with a numeric ID of 143. It is generally a good
 * idea to do this, as some fontforge code will of course assume a
 * pointer to a splinechar is a pointer to a splinechar and not just
 * the glyph index of that splinechar.
 *
 * MIQ updated this in Oct 2012 to be more forgiving when called twice
 * or on a splinefont which has some of it's references already fixed.
 * This was to allow partial updates of data structures from SFD
 * fragments and the fixup to operate just on those references which
 * need to be fixed.
 */
extern void SFDFixupRefs(SplineFont *sf);

/**
 * Dump a single undo for the given splinechar to the file at "sfd".
 * The keyPrefix can be either Undo or Redo to generate the correct XML
 * element, and idx is the index into the undoes list that 'u' was found at
 * so that a stream of single undo/redo elements can be saved and reloaded
 * in the correct order.
 */
extern void SFDDumpUndo(FILE *sfd, SplineChar *sc, Undoes *u, const char* keyPrefix, int idx);

extern char **NamesReadSFD(char *filename);
extern char *utf7toutf8_copy(const char *_str);
extern char *utf8toutf7_copy(const char *_str);
extern const char *EncName(Encoding *encname);
extern int SFDDoesAnyBackupExist(char* filename);
extern int SFD_DumpSplineFontMetadata(FILE *sfd, SplineFont *sf);
extern int SFDWriteBak(char *filename, SplineFont *sf, EncMap *map, EncMap *normal);
extern int SFDWriteBakExtended(char* locfilename, SplineFont *sf, EncMap *map, EncMap *normal, int s2d, int localPrefMaxBackupsToKeep);
extern MacFeat *SFDParseMacFeatures(FILE *sfd, char *tok);
extern SplineChar *SFDReadOneChar(SplineFont *cur_sf, const char *name);
extern SplineFont *SFDirRead(char *filename);
extern SplineFont *_SFDRead(char *filename, FILE *sfd);
extern SplineFont *SFRecoverFile(char *autosavename, int inquire, int *state);
extern Undoes *SFDGetUndo(FILE *sfd, SplineChar *sc, const char* startTag, int current_layer);
extern void SFAutoSave(SplineFont *sf, EncMap *map);
extern void SFClearAutoSave(SplineFont *sf);
extern void SFDDumpCharStartingMarker(FILE *sfd, SplineChar *sc);
extern void SFD_DumpKerns(FILE *sfd, SplineChar *sc, int *newgids);
extern void SFD_DumpLookup(FILE *sfd, SplineFont *sf);
extern void SFDDumpMacFeat(FILE *sfd, MacFeat *mf);
extern void SFD_DumpPST(FILE *sfd, SplineChar *sc);
extern void SFTimesFromFile(SplineFont *sf, FILE *file);
extern SplineFont *SFDRead(char *filename);
extern int SFDWrite(char *filename,SplineFont *sf,EncMap *map,EncMap *normal, int todir);

typedef void (*visitSFDFragmentFunc)(FILE *sfd, char *tokbuf, SplineFont *sf, void* udata);
extern void visitSFDFragment(FILE *sfd, SplineFont *sf, visitSFDFragmentFunc ufunc, void* udata);

extern void SFDDumpUTF7Str(FILE *sfd, const char *_str);
extern char *SFDReadUTF7Str(FILE *sfd);

#endif /* FONTFORGE_SFD_H */
