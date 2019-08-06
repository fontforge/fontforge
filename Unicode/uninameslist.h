#ifndef UN_NAMESLIST_H
# define UN_NAMESLIST_H

/* This file was generated using the program 'buildnameslist.c' */

#ifdef __cplusplus
extern "C" {
#endif

struct unicode_block {
	int start, end;
	const char *name;
};

struct unicode_nameannot {
	const char *name, *annot;
};

/* NOTE: Build your program to access the functions if using multilanguage. */

#define UNICODE_BLOCK_MAX	314
#define UNICODE_EN_BLOCK_MAX	314
extern const struct unicode_block UnicodeBlock[314];

/* NOTE: These 4 constants are correct for this version of libuninameslist, */
/* but can change for later versions of NamesList (use as an example guide) */
#define UNICODE_NAME_MAX	83
#define UNICODE_ANNOT_MAX	513
#define UNICODE_EN_NAME_MAX	83
#define UNICODE_EN_ANNOT_MAX	513
extern const struct unicode_nameannot * const *const UnicodeNameAnnot[];

/* Index by: UnicodeNameAnnot[(uni>>16)&0x1f][(uni>>8)&0xff][uni&0xff] */

/* At the beginning of lines (after a tab) within the annotation string, a: */
/*  * should be replaced by a bullet U+2022 */
/*  x should be replaced by a right arrow U+2192 */
/*  : should be replaced by an equivalent U+224D */
/*  # should be replaced by an approximate U+2245 */
/*  = should remain itself */

/* Return a pointer to the name for this unicode value */
/* This value points to a constant string inside the library */
const char *uniNamesList_name(unsigned long uni);

/* Returns pointer to the annotations for this unicode value */
/* This value points to a constant string inside the library */
const char *uniNamesList_annot(unsigned long uni);


/* These functions are available in libuninameslist-0.4.20140731 and higher */

/* Version information for this <uninameslist.h> include file */
/* Return number of blocks in this NamesList. */
int uniNamesList_blockCount(void);

/* Return block number for this unicode value (-1 if bad unicode value) */
int uniNamesList_blockNumber(unsigned long uni);

/* Return unicode value starting this Unicode block (bad uniBlock = -1) */
long uniNamesList_blockStart(int uniBlock);

/* Return unicode value ending this Unicode block (-1 if bad uniBlock). */
long uniNamesList_blockEnd(int uniBlock);

/* Return a pointer to the blockname for this unicode block. */
/* This value points to a constant string inside the library */
const char * uniNamesList_blockName(int uniBlock);

/* These functions are available in libuninameslist-20180408 and higher */

/* Return count of how many names2 are found in this version of library */
int uniNamesList_names2cnt(void);

/* Return list location for this unicode value. Return -1 if not found. */
int uniNamesList_names2getU(unsigned long uni);

/* Return unicode value with names2 (0<=count<uniNamesList_names2cnt(). */
long uniNamesList_names2val(int count);

/* Stringlength of names2. Use this if you want to truncate annotations */
int uniNamesList_names2lnC(int count);
int uniNamesList_names2lnU(unsigned long uni);

/* Return pointer to start of normalized alias names2 within annotation */
const char *uniNamesList_names2anC(int count);
const char *uniNamesList_names2anU(unsigned long uni);

#ifdef __cplusplus
}
#endif
#endif
