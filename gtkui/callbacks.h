#include <gtk/gtk.h>


gboolean
FontView_Char                          (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

gboolean
FontView_RequestClose                  (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

gboolean
FontView_DestroyWindow                 (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

gboolean
FontView_Resize                        (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data);

gboolean
FontView_ClearSelection                (GtkWidget       *widget,
                                        GdkEventSelection *event,
                                        gpointer         user_data);

void
FontView_Realize                       (GtkWidget       *widget,
                                        gpointer         user_data);

void
FontViewMenu_ActivateFile              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
Menu_New                               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
Menu_Open                              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
RecentMenuBuild                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_Close                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_Save                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_SaveAs                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
Menu_SaveAll                           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_Generate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_GenerateFamily            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_Import                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_MergeKern                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_RevertFile                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_RevertToBackup            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_RevertGlyph               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_Print                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_ExecScript                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
ScriptMenuBuild                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
Menu_Preferences                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
Menu_Quit                              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_ActivateEdit              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_Undo                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_Redo                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_Cut                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_Copy                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_CopyRef                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_CopyLookupData            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_CopyWidth                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_CopyVWidth                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_CopyLBearing              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_CopyRBearing              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_Paste                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_PasteInto                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_PasteAfter                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_SameGlyphAs               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_Clear                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_ClearBackground           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_CopyFg2Bg                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_Join                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_SelectAll                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_InvertSelection           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_DeselectAll               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_SelectDefault             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_SelectWhite               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_SelectRed                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_SelectGreen               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_SelectBlue                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_SelectYellow              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_SelectCyan                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_SelectMagenta             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_SelectColorPicker         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_SelectByName              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_SelectByScript            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_SelectWorthOutputting     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_SelectChangedGlyphs       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_SelectUnhintedGlyphs      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_SelectAutohintable        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_SelectByLookupSubtable    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_FindRpl                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_RplRef                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_UnlinkRef                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_ActivateCopyFrom          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_CopyFromAll               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_CopyFromDisplayed         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_CopyFromMetadata          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_CopyFromTTInstrs          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_RemoveUndoes              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_ActivateElement           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_FontInfo                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_CharInfo                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_MathInfo                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_BDFInfo                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_ActivateDependents        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_ShowDependentRefs         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_ShowDependentSubs         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_GlyphRename               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_FindProbs                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_Validate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_BitmapsAvail              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_RegenBitmaps              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_RemoveBitmapGlyphs        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_ChangeWeight              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_Oblique                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_Condense                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_Inline                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_Outline                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_Shadow                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_WireFrame                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_ActivateTransformations   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_Transform                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_PointOfView               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_NonLinearTransform        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_ExpandStroke              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_TilePath                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_RemoveOverlap             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_Intersect                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_FindInter                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_Simplify                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_SimplifyMore              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_Cleanup                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_CanonicalStart            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_CanonicalContours         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_AddExtrema                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_RoundToInt                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_RoundToHundredths         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_Cluster                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_Autotrace                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_CorrectDir                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_ActivateBuild             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_BuildAccented             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_BuildComposite            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_MergeFonts                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_Interpolate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_CompareFonts              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontView_ToolsActivate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_ActivateHints             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_AutoHint                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_AutoHintSubsPts           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_AutoCounter               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_DontAutoHint              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_AutoInstr                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_EditInstrs                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_EditTable                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_PrivateToCvt              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_ClearHints                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_ClearInstrs               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_Histograms                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_BuildReencode             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_Compact                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_BuildForceEncoding        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_AddEncodingSlots          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_RemoveUnusedSlots         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_DetachGlyphs              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_DetachAndRemoveGlyphs     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_AddEncodingName           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_LoadEncoding              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_MakeFromFont              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_RemoveEncoding            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_DisplayByGroups           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_DefineGroups              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_SaveNamelist              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_LoadNamelist              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_RenameByNamelist          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_CreateNamedGlyphs         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_ActivateView              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_ChangeChar                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_Goto                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_ShowAtt                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_DisplaySubstitutions      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_ActivateCombinations      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_KernPairs                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_ActivateAnchoredPairs     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_Ligatures                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_GlyphLabel                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_ShowMetrics               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_WindowSize                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_PixelSize                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_BitmapMagnification       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_ActivateMetrics           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_CenterWidth               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_ThirdsWidth               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_SetWidth                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_AutoWidth                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_AutoKern                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_KernClasses               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_RemoveKP                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_KernPairCloseup           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_VKernClasses              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_VKernFromH                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_RemoveVKP                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_ActivateCID               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_Convert2CID               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_ConvertByCMap             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_Flatten                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_FlattenByCMap             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_CIDInsertFont             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_CIDInsertEmptyFont        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_RemoveFontFromCID         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_ChangeSupplement          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_CIDFontInfo               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_ActivateMM                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_CreateMM                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_MMValid                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_MMInfo                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_Blend                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_ChangeDefWeights          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
Menu_ActivateWindows                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_OpenOutline               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_OpenBitmap                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_OpenMetrics               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
WindowMenu_Warnings                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_ContextualHelp            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MenuHelp_Help                          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MenuHelp_Index                         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MenuHelp_About                         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MenuHelp_License                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

gboolean
FontView_StatusExpose                  (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data);

gboolean
FontViewView_Mouse                     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
FontViewView_Resize                    (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data);

gboolean
FontViewView_Expose                    (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data);

void
FontView_VScroll                       (GtkRange        *range,
                                        gpointer         user_data);

gboolean
SplashDismiss                          (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
CharView_Char                          (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

void
CharViewMenu_ActivateFile              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Close                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Save                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_SaveAs                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Generate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_GenerateFamily            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Export                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Import                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_RevertFile                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_RevertGlyph               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Print                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Display                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_ActivateEdit              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Undo                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Redo                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Cut                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Copy                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_CopyRef                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_CopyWidth                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_CopyVWidth                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_CopyLBearing              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_CopyRBearing              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_CopyFeatures              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Paste                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Clear                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_ClearBackground           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Merge                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Elide                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Join                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_CopyFg2Bg                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_CopyGridFit               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_UnlinkRef                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_RemoveUndoes              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_ActivatePoints            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Curve                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Corner                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Tangent                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Interpolated              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_NotInterpolated           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_ActivateSelect            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_SelectAll                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_InvertSelection           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_SelectNone                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_FirstPoint                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_first_point__next_contour1_activate (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_NextPoint                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_PrevPoint                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_NextCP                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_PrevCP                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Contours                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_SelectPointAt             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_SelectAllPtsRefs          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_SelectAnchors             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_SelectWidth               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_SelectVWidth              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_SelectHM                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_MakeFirst                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_AddAnchor                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_MakeLine                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_CenterCPs                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_ActivateElement           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_FontInfo                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_CharInfo                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_GetInfo                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_ActivateDependents        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_References                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_ShowDepSubs               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_FindProbs                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_BitmapsAvail              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_RegenBitmaps              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_ActivateTransform         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Transform                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_POV                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_NonLinearTransform        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_ExpandStroke              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_TilePath                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_ActivateOverlap           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_RemoveOverlap             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Intersect                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Exclude                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_FindInter                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_ActivateSimplify          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Simplify                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_SimplifyMore              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Cleanup                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_AddExtrema                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_ActivateEffects           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Inline                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Outline                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Shadow                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Wireframe                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Metafont                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Autotrace                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_ActivateAlign             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_AveragePoints             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_SpacePoints               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_SpaceRegions              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_MakeParallel              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_RoundActivate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Round2Int                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Round2Hundredths          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Cluster                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_ActivateOrder             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_OrderFirst                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_OrderEarlier              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_OrderLater                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_OrderLast                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Clockwise                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_CounterClockwise          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_CorrectDir                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_BuildAccented             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_BuildComposite            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_ActivateHints             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_AutoHint                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_HintSubsPts               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_AutoCounter               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_DontAutoHint              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_AutoInstr                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_EditInstrs                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Debug                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_ClearHStem                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_ClearVStem                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_ClearInstrs               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_AddHHint                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_AddVHint                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_CreateHHint               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_CreateVHint               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_ReviewHints               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_ActivateView              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Fit                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_ZoomOut                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_ZoomIn                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_NextChar                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_PrevChar                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_NextDefChar               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_PrevDefChar               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_FormerGlyph               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Goto                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_FindInFontView            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_ShowPoints                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_ActiveNumPoints           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_NumPtsNone                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_NumPtsTrueType            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_NumPtsPS                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_NumPtsSvg                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_MarkExtrema               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_MarkPOI                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Fill                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_GridFit                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_ActivateCombinations      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_KernPairs                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_AllAnchoredPairs          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Ligatures                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_ActivatePalettes          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_ToolPalettes              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_LayersPalettes            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_DockPalettes              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_ShowRulers                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_ActivateMetrics           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_CenterWidth               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Thirds                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_SetWidth                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_SetLBearing               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_SetRBearing               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_SetVAdvance               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_RemoveKP                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_RemoveVKP                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_KPCloseup                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_AnchorControl             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_MMActivate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_Reblend                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_MMViewActivate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_OpenOutline               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_OpenBitmap                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
CharViewMenu_OpenMetrics               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

gboolean
CharView_StatusExpose                  (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data);

gboolean
CVTools_Expose                         (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data);

gboolean
CVTools_Char                           (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

gboolean
CVTools_Mouse                          (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
CVTools_MouseMotion                    (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data);

void
CharView_ForeVisible                   (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
CharView_BackVisible                   (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
CharView_GuideVisible                  (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
CharView_HHintsVisible                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
MetricsViewMenu_ActivateFile           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_Close                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_Save                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_SaveAs                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_Generate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_GenerateFamily         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_MergeKern              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_Print                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_Display                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_ActivateEdit           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_Undo                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_Redo                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_Cut                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_Copy                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_CopyRef                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_CopyWidth              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_CopyVWidth             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_CopyLBearing           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_CopyRBearing           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_Paste                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_SaveGlyphAs            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_Clear                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_Join                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_UnlinkRef              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_ActivateElement        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_FontInfo               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_CharInfo               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_ActivateDependents     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_References             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_ShowDepSubs            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_FindProbs              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_BitmapsAvail           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_RegenBitmaps           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_ActivateTransformations
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_Transform              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_PointOfView            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_NonLinearTransform     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_ExpandStroke           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_ActivateOverlap        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_RemoveOverlap          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_Intersect              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_FindInter              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_ActivateSimplify       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_Simplify               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_SimplifyMore           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_Cleanup                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_AddExtrema             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_Round2Int              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_RoundToHundredths      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_Cluster                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_ActivateEffects        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_Inline                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_Outline                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_Shadow                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_Wireframe              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_Metafont               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_Autotrace              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_CorrectDir             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_BuildAccented          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_BuildComposite         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_ActivateView           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_ZoomOut                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_ZoomIn                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_InsertCharAfter        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_InsCharBefore          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_ReplaceGlyph           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_Subs                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_NextChar               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_PrevChar               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_NextDefChar            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_PrevDefChar            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_find_in_fontview2_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_Goto                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_ActivateCombinations   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_KernPairs              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_AllAnchoredPairs       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_Ligatures              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_ShowGrid               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_AntiAlias              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_vertical1_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_ActivateMetrics        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_CenterWidth            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_Thirds                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_KernClasses            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_VKernClasses           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_VKernFromH             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_KernPair               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_OpenOutline            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_OpenBitmap             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
MetricsViewMenu_OpenMetrics            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_ActivateFile            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_Close                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_Save                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_SaveAs                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_Generate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_GenerateFamily          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_Export                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_Import                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_RevertFile              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_ActivateEdit            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_Undo                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_Redo                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_Cut                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_Copy                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_Paste                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_Clear                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_SelectAll               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_RemoveUndoes            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_ActivateElement         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_FontInfo                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_CharInfo                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_BDFInfo                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_BitmapsAvail            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_RegenBitmaps            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_TransformActiveate      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_FlipH                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_FlipVert                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_Rotate90CW              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_Rotate90CCW             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_Rotate180               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_Skew                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_ActivateView            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_Fit                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_ZoomOut                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_ZoomIn                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_NextChar                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_PrevChar                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_NextDefChar             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_PrevDefChar             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_Goto                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_FindInFontView          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_BiggerPixel             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_SmallerPixel            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_PaletteActivate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_ToolsPalette            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_LayersPalette           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_ShaePalette             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_DockedPalette           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_ActivateMetrics         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_SetWidth                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_OpenOutline             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_OpenBitmap              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
BitmapViewMenu_OpenMetrics             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

gboolean
BitmapView_StatusExpose                (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data);

void
on_sans_serif1_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
FontViewMenu_ToolsActivate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);
