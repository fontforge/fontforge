/* I can't find the mac "Files.h" include file under Darwin, so... */
/* Nor the CoreServices/CoreServices.h file that is supposed to replace it */

#define noErr		0
#define eofErr		(-39)

#define fsRdPerm	1

typedef char Str255[256];
typedef char Str63[64];
typedef struct {
    short volume;
    int dirID;
    Str63 name;
} FSSpec;

typedef struct {
    int fdType;
    int fdCreator;
    uint16 fdFlags;
    int16 Point_x, Point_y;
    int16 fdFldr;
    int pad;		/* Just in case I've screwed up the alignment */
} FInfo;

short FSMakeFSSpec(short volumn, int dirID, Str255 name, FSSpec *spec);
short FSpOpenRF(FSSpec *spec,char permission,short *refNum);
short FSRead(short refNum,int *cnt,char *buf);
short FSClose(short refNum );
short FSpGetFInfo(const FSSpec *spec,FInfo *fndrInfo);
