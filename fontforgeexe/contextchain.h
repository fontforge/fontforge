#ifndef FONTFORGE_FONTFORGEEXE_CONTEXTCHAIN_H
#define FONTFORGE_FONTFORGEEXE_CONTEXTCHAIN_H

extern char *cu_copybetween(const unichar_t *start, const unichar_t *end);
extern void ContextChainEdit(SplineFont *sf,FPST *fpst,
	struct gfi_data *gfi,unichar_t *newname,int layer);

#endif /* FONTFORGE_FONTFORGEEXE_CONTEXTCHAIN_H */
