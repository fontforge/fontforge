/* I can't find the mac "Files.h" include file under Darwin, so... */
/* Nor the CoreServices/CoreServices.h file that is supposed to replace it */

#define noErr		0
#define eofErr		(-39)
#define fnfErr		(-43)
#define dupFNErr	(-48)

#define fsRdPerm	1
#define fsWrPerm	2

#define smSystemScript	(-1)

#define kFSCatInfoFinderInfo          0x00000800

typedef unsigned char Str255[256];

typedef struct {
    unsigned char hidden[80];
} FSRef;

typedef unichar_t UniChar;
typedef struct HFSUniStr255 {
    uint16 length;
    UniChar unicode[255];
} HFSUniStr255;

typedef struct {
    int fdType;
    int fdCreator;
    unsigned short fdFlags;
    short Point_x, Point_y;
    short fdFldr;
    int pad;		/* Just in case I've screwed up the alignment */
} FInfo;

typedef struct FSCatalogInfo {
    unsigned short nodeFlags;
    short volume;
    unsigned int parentDirID;
    unsigned int nodeID;
  uint8               sharingFlags;           /* kioFlAttribMountedBit and kioFlAttribSharePointBit */
  uint8               userPrivileges;         /* user's effective AFP privileges (same as ioACUser) */
  uint8               reserved1;
  uint8               reserved2;
  uint32         createDate[2];             /* date and time of creation */
  uint32         contentModDate[2];         /* date and time of last fork modification */
  uint32         attributeModDate[2];       /* date and time of last attribute modification */
  uint32         accessDate[2];             /* date and time of last access (for Mac OS X) */
  uint32         backupDate[2];             /* date and time of last backup */

  uint32              permissions[4];         /* permissions (for Mac OS X) */

  uint8               finderInfo[16];         /* Finder information part 1 */
  uint8               extFinderInfo[16];      /* Finder information part 2 */

  uint32              dataLogicalSize[2];        /* files only */
  uint32              dataPhysicalSize[2];       /* files only */
  uint32              rsrcLogicalSize[2];        /* files only */
  uint32              rsrcPhysicalSize[2];       /* files only */

  uint32              valence;                /* folders only */
  uint32        textEncodingHint;
} FSCatalogInfo;

short FSRead(short refNum,long *cnt,char *buf);
short FSWrite(short refNum,long *cnt,char *buf);
short FSClose(short refNum );
short SetEOF(short refNum,int eofpos);

short FSPathMakeRef(const unsigned char *path,FSRef *ref,int *isDir);
short FSGetResourceForkName(HFSUniStr255 *resname);
short FSCreateFork(const FSRef *,uint16 length,const UniChar *);
short FSOpenFork(const FSRef *,uint16 length,const UniChar *,int8 perm,int16 *forkrefnum);
short FSCreateFileUnicode(const FSRef *,uint16 len, const UniChar *,
    		uint32, const FSCatalogInfo *, FSRef *, struct FSSpec *);
