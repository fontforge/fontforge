/* I can't find the mac "Files.h" include file under Darwin, so... */
/* Nor the CoreServices/CoreServices.h file that is supposed to replace it */

#define noErr		0
#define eofErr		(-39)
#define fnfErr		(-43)
#define dupFNErr	(-48)

#define fsRdPerm	1
#define fsWrPerm	2

#define smSystemScript	(-1)

#define kFSCatInfoNodeID 16

typedef unsigned char Str255[256];
typedef unsigned char Str63[64];
typedef struct {		/* gcc misaligns this */
    short vRefNum;
    int dirID;
    Str63 name;
} FSSpec;

typedef struct {
    uint8 hidden[80];
} FSRef;

typedef struct {
    int fdType;
    int fdCreator;
    uint16 fdFlags;
    int16 Point_x, Point_y;
    int16 fdFldr;
    int pad;		/* Just in case I've screwed up the alignment */
} FInfo;

typedef struct FSCatalogInfo {
    uint16 nodeFlags;
    int16 volume;
    uint32 parentDirID;
    uint32 nodeID;
    int32 padding[50];		/* There's a bunch more junk that I don't care about */
} FSCatalogInfo;

short FSpOpenRF(const FSSpec *spec,char permission,short *refNum);
short FSpCreateResFile(const FSSpec *spec,int creator,int type,int16 script);
short FSRead(short refNum,int *cnt,char *buf);
short FSWrite(short refNum,int *cnt,char *buf);
short FSClose(short refNum );
short FSpGetFInfo(const FSSpec *spec,FInfo *fndrInfo);
short SetEOF(int16 refNum,int32 eofpos);

short FSMakeFSSpec(int16 volume, int32 dirid,uint8 *,FSSpec *spec);
short FSPathMakeRef(const uint8 *path,FSRef *ref,int *isDir);
short FSGetCatalogInfo(FSRef *ref,int whichinfo,FSCatalogInfo *info,void *null2,FSSpec *spec,void *null3);
