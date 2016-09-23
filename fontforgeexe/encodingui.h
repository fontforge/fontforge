#ifndef FONTFORGE_FONTFORGEEXE_ENCODINGUI_H
#define FONTFORGE_FONTFORGEEXE_ENCODINGUI_H

extern Encoding *MakeEncoding(SplineFont *sf,EncMap *map);
extern Encoding *ParseEncodingNameFromList(GGadget *listfield);
extern GMenuItem *GetEncodingMenu(void (*func)(GWindow,GMenuItem *,GEvent *),
	Encoding *current);

extern GTextInfo *EncodingTypesFindEnc(GTextInfo *encodingtypes, Encoding *enc);
extern GTextInfo *GetEncodingTypes(void);
extern struct cidmap *AskUserForCIDMap(void);
extern void LoadEncodingFile(void);
extern void RemoveEncoding(void);
extern void SFFindNearTop(SplineFont *sf);
extern void SFRestoreNearTop(SplineFont *sf);

#endif /* FONTFORGE_FONTFORGEEXE_ENCODINGUI_H */
