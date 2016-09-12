#ifndef FONTFORGE_FONTFORGEEXE_MACENCUI_H
#define FONTFORGE_FONTFORGEEXE_MACENCUI_H

extern int GCDBuildNames(GGadgetCreateData *gcd, GTextInfo *label, int pos, struct macname *names);
extern struct macname *NameGadgetsGetNames(GWindow gw);
extern void GCDFillMacFeat(GGadgetCreateData *mfgcd, GTextInfo *mflabels, int width, MacFeat *all, int fromprefs, GGadgetCreateData *boxes, GGadgetCreateData **array);
extern void NameGadgetsSetEnabled(GWindow gw, int enable);
extern void Prefs_ReplaceMacFeatures(GGadget *list);

#endif /* FONTFORGE_FONTFORGEEXE_MACENCUI_H */
