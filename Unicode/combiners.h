static const int poses300[] = {
    _Above,
    _Above,
    _Above,
    _Above,
    _Above,
    _Above,
    _Above,
    _Above,
    _Above,
    _Above,
    _Above,
    _Above,
    _Above,
    _Above,
    _Above,
    _Above,
    _Above,		/* 0x310 */
    _Above,
    _Above,
    _Above,
    _Above,
    _Above|_CenterRight,
    _Below,
    _Below,
    _Below,
    _Below,
    _Above,
    _Above|_Right|_Touching,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,		/* 0x320 */
    _Below|_Touching,
    _Below|_Touching,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below|_Touching,
    _Below|_Touching,	/* 0x328 */
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,		/* 0x330 */
    _Below,
    _Below,
    _Below,
    _Overstrike,
    _Overstrike,
    _Overstrike,
    _Overstrike,
    _Overstrike,	/* 0x338 */
    _Below,
    _Below,
    _Below,
    _Below,
    _Above,
    _Above,
    _Above,
    _Above|_Left,	/* 0x340 */
    _Above|_Right,
    _Above,
    _Above,
    _Above,
    _Below,
    0
};
static const int poses360[] = {
    _Above|_Joins2,
    _Above|_Joins2,
    _Below|_Joins2,
    0
};
static const int poses385[] = {
    _Above
};
static const int poses483[] = {
    _Above|_CenterLeft,
    _Above|_CenterLeft,
    _Above|_CenterLeft,
    _Above|_CenterLeft,
    0
};

static const int poses488[] = {
    _CenteredOutside,				/* 8 down half-circles distributed in a circle around the character */
    _CenteredOutside				/* 8 commas rotated as moved around circle, bottom is normal comma */
};

static const int poses591[] = {
    _Below,
    _Above,
    _Above,
    _Above,
    _Above,
    _Below|_CenterRight,
    _Above,
    _Above,
    _Above|_LeftEdge,
    _Below|_RightEdge,
    _Below,
    _Above|_CenterRight,
    _Above|_RightEdge,
    _Above|_CenterRight,
    _Above,
    _Above|_RightEdge,			/* 05a0 */
    _Above|_LeftEdge
};

static const int poses5A3[] = {
    _Below,
    _Below,
    _Below|_CenterLeft,
    _Below|_CenterLeft,
    _Below,
    _Above|_CenterLeft,
    _Above|_LeftEdge,
    _Below,
    _Above,
    _Above,
    _Below|_RightEdge,
    _Above|_LeftEdge,
    _Above,
    _Below,				/* 05b0 */
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Above|_LeftEdge
};

static const int poses5BB[] = {
    _Below,
    _Overstrike,
    _Below
};

static const int poses5BF[] = {
    _Above
};

static const int poses5C1[] = {
    _Above|_RightEdge,
    _Above|_LeftEdge
};

static const int poses5C4[] = {
    _Above
};

static const int poses64b[] = {
    _Above,
    _Above,
    _Below,
    _Above,
    _Above,
    _Below,
    _Above,
    _Above,
    0
};

static const int poses670[] = {
    _Above
};

static const int poses6D6[] = {
    _Above,
    _Above,
    _Above,
    _Above,
    _Above,
    _Above,
    _Above,
    _Overstrike,
    _Overstrike,
    _Above,
    _Above,		/* 6e0 */
    _Above,
    _Above,
    _Below,
    _Above
};

static const int poses6E7[] = {
    _Above,
    _Above
};

static const int poses6EA[] = {
    _Below,
    _Above,
    _Above,
    _Below
};

static const int poses711[] = {
    _Above
};

static const int poses730[] = {
    _Above,
    _Below,
    _CenteredOutside,		/* Two dots, one above CenterRight, one below CenterLeft */
    _Above,
    _Below,
    _Above,
    _Above,
    _Below,
    _Below,
    _Below,
    _Above,
    _Below,
    _Below,
    _Above,
    _Below,
    _Above,
    _Above|_LeftEdge,	/* 0740 */
    _Above,
    _Below,
    _Above,
    _Below,
    _Above,
    _Below,
    _Above,
    _Below,
    _Above,
    _Above
};

static const int poses7A6[] = {
    _Above|_CenterLeft,
    _Above|_CenterLeft,
    _Below|_CenterLeft,
    _Below|_CenterLeft,
    _Above|_CenterLeft,
    _Above|_CenterLeft,
    _Above|_CenterLeft,
    _Above|_CenterLeft,
    _Above|_CenterLeft,
    _Above|_CenterLeft,
    _Above|_CenterLeft,
};

static const int poses901[] = {
    _Above,
    _Above,
    _Right
};

static const int poses93C[] = {
    _Below
};

static const int poses93E[] = {
    _Right,
    _Left,
    _Right,	/* 940 */
    _Below,
    _Below,
    _Below,
    _Below,
    _Above,
    _Above|_CenterLeft,
    _Above|_CenterLeft,
    _Above|_CenterLeft,
    _Right,
    _Right,
    _Right,
    _Right,
    _Below|_CenterRight
};

static const int poses951[] = {
    _Above,
    _Below,
    _Above,
    _Above
};

static const int poses962[] = {
    _Below,
    _Below
};

static const int poses981[] = {
    _Above,
    _Right,
    _Right
};

static const int poses9BC[] = {
    _Below
};

static const int poses9BE[] = {
    _Right,
    _Left,
    _Right,
    _Below,
    _Below,
    _Below,
    _Below|_CenterRight
};

static const int poses9C7[] = {
    _Left,
    _Left
};

static const int poses9CB[] = {
    _Overstrike,
    _Overstrike,
    _Below
};

static const int poses9D7[] = {
    _Right
};

static const int poses9E2[] = {
    _Below,
    _Below
};

static const int posesA02[] = {
    _Above
};

static const int posesA3C[] = {
    _Below
};

static const int posesA3E[] = {
    _Right,
    _Left,
    _Right,		/* 0a40 */
    _Below,
    _Below
};

static const int posesA47[] = {
    _Above|_CenterLeft,
    _Above|_CenterLeft
};

static const int posesA4B[] = {
    _Above|_CenterLeft,
    _Above|_CenterLeft,
    _Below
};

static const int posesA70[] = {
    _Above,
    _Above
};

static const int posesA81[] = {
    _Above,
    _Above,
    _Right
};

static const int posesABC[] = {
    _Below
};

static const int posesABE[] = {
    _Right,
    _Left,
    _Right,
    _Below,
    _Below,
    _Below,
    _Below,
    _Above
};

static const int posesAC7[] = {
    _Above|_CenterLeft,
    _Above|_CenterLeft,
    _Right
};

static const int posesACB[] = {
    _Right,
    _Right,
    _Below|_CenterRight
};

static const int posesB01[] = {
    _Above,
    _Above,
    _Right
};

static const int posesB3C[] = {
    _Below
};

static const int posesB3E[] = {
    _Right,
    _Above,
    _Right,		/* 0b40 */
    _Below,
    _Below,
    _Below
};

static const int posesB47[] = {
    _Left,
    _Outside|_Left|_Above
};

static const int posesB4B[] = {
    _CenteredOutside,
    _CenteredOutside,
    _Below
};

static const int posesB56[] = {
    _Above,
    _Right,
};

static const int posesB82[] = {
    _Above,
    _Right
};

static const int posesBBE[] = {
    _Right,
    _Right,
    _Above,
    _Right,
    _Right
};

static const int posesBC6[] = {
    _Left,
    _Left,
    _Left
};

static const int posesBCA[] = {
    _CenteredOutside,
    _CenteredOutside,
    _CenteredOutside,
    _Above
};

static const int posesBD7[] = {
    _Right
};

static const int posesC01[] = {
    _Right,
    _Right,
    _Right
};

static const int posesC3E[] = {
    _Above|_CenterRight,
    _Above,
    _Above,
    _Right,
    _Right,
    _Right,
    _Right
};

static const int posesC46[] = {
    _Above,
    _Above,
    _CenteredOutside
};

static const int posesC4A[] = {
    _Above,
    _Above,
    _Above,
    _Above
};

static const int posesC55[] = {
    _Above,
    _Below
};

static const int posesC82[] = {
    _Right,
    _Right
};

static const int posesCBE[] = {
    _Right,
    _Above,
    _Outside|_Above|_Right,
    _Right,
    _Right,
    _Right,
    _Right
};

static const int posesCC6[] = {
    _Above,
    _Outside|_Above|_Right,
    _Outside|_Above|_Right
};

static const int posesCCA[] = {
    _Outside|_Above|_Right,
    _Outside|_Above|_Right,
    _Above,
    _Above
};

static const int posesCD5[] = {
    _Right,
    _Right
};

static const int posesD02[] = {
    _Right,
    _Right
};

static const int posesD3E[] = {
    _Right,
    _Right,
    _Right,
    _Below|_Right,
    _Below|_Right,
    _Below
};

static const int posesD46[] = {
    _Left,
    _Left,
    _Left
};

static const int posesD4A[] = {
    _Outside|_Left|_Right,
    _Outside|_Left|_Right,
    _Outside|_Left|_Right,
    _Above|_Right
};

static const int posesD57[] = {
    _Right
};

static const int posesD82[] = {
    _Right,
    _Right
};

static const int posesDCA[] = {
    _Right
};

static const int posesDCF[] = {
    _Right,
    _Right,
    _Right,
    _Above,
    _Above,
    _Below
};

static const int posesDD6[] = {
    _Below
};

static const int posesDD8[] = {
    _Right,
    _Left,
    _CenteredOutside,
    _Left,
    _CenteredOutside,
    _CenteredOutside,
    _CenteredOutside,
    _Right
};

static const int posesDF2[] = {
    _Right,
    _Right
};

static const int posesE31[] = {
    _Above
};

static const int posesE34[] = {
    _Above,
    _Above,
    _Above,
    _Above,
    _Below|_Right,
    _Below|_CenterRight,
    _Below|_Right
};

static const int posesE47[] = {
    _Above,
    _Above|_Right,
    _Above,
    _Above,
    _Above|_CenterRight,
    _Above,
    _Above|_Right,
    _Above|_Right,
};

static const int posesEB1[] = {
    _Above
};

static const int posesEB4[] = {
    _Above,
    _Above,
    _Above,
    _Above,
    _Below,
    _Below
};

static const int posesEBB[] = {
    _Above,
    _Below
};

static const int posesEC8[] = {
    _Above,
    _Above,
    _Above,
    _Above,
    _Above,
    _Above
};

static const int posesF18[] = {
    _Below|_Right
};

static const int posesF35[] = {
    _Below
};

static const int posesF37[] = {
    _Below
};

static const int posesF39[] = {
    _Above|_Right|_Touching
};

static const int posesF3E[] = {
    _Below|_Right,
    _Below|_Left
};

static const int posesF71[] = {
    _Below,
    _Above,
    _Outside|_Above|_Below,
    _Below,
    _Below,
    _Outside|_Above|_Below,
    _Outside|_Above|_Below,
    _Outside|_Above|_Below,
    _Outside|_Above|_Below,
    _Above|_CenterLeft,
    _Above|_CenterLeft,
    _Above,
    _Above,
    _Above,
    _Right,
    _Above,		/* 0f80 */
    _Outside|_Above|_Below,
    _Above,
    _Above,
    _Below|_Left
};

static const int posesF86[] = {
    _Above,
    _Above,
    _Above,
    _Above,
    _Above,
    _Above
};

static const int posesF90[] = {
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below
};

static const int posesF99[] = {
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below,
    _Below
};

static const int posesFC6[] = {
    _Below
};

static const int poses102C[] = {
    _Right,
    _Above,
    _Above,
    _Below,
    _Below,			/* 1030 */
    _Left,
    _Above
};

static const int poses1036[] = {
    _Above,
    _Below,
    _Right,
    _Above
};

static const int poses1056[] = {
    _Right,
    _Right,
    _Below,
    _Below
};

static const int poses17B4[] = {
    _Overstrike,
    _Overstrike,
    _Right,
    _Above,
    _Above,
    _Above,
    _Above,
    _Below,
    _Below,
    _Below,
    _Outside|_Left|_Above,
    _CenteredOutside,
    _CenteredOutside,		/* 17c0 */
    _Left,
    _Left,
    _Left,
    _CenteredOutside,
    _CenteredOutside,
    _Above,
    _Right,
    _Right,
    _Above,
    _Above,
    _Above,
    _Above,
    _Above|_CenterRight,
    _Above|_CenterRight,
    _Above,
    _Above|_CenterRight,	/* 17d0 */
    _Above,
    _Below,
    _Above
};

static const int poses18A9[] = {
    _Above|_Left
};

static const int poses1FBD[] = {	/* These aren't listed as combiners, but if we don't use them as such greek fonts don't work */
    _Above,
    _Right,
    _Above,
    _Above,
    _Above
};

static const int poses1FCD[] = {	/* These aren't listed as combiners, but if we don't use them as such greek fonts don't work */
    _Above,
    _Above,
    _Above
};

static const int poses1FDD[] = {	/* These aren't listed as combiners, but if we don't use them as such greek fonts don't work */
    _Above,
    _Above,
    _Above
};

static const int poses1FED[] = {	/* These aren't listed as combiners, but if we don't use them as such greek fonts don't work */
    _Above,
    _Above,
    _Above
};

static const int poses1FFD[] = {	/* These aren't listed as combiners, but if we don't use them as such greek fonts don't work */
    _Above,
    _Above
};

static const int poses20D0[] = {
    _Above,
    _Above,
    _Overstrike,
    _Overstrike,
    _Above,
    _Above,
    _Above,
    _Above,
    _Overstrike,
    _Overstrike,
    _Overstrike,
    _Above,
    _Above,
    _CenteredOutside,
    _CenteredOutside,
    _CenteredOutside,
    _Overstrike,
    _Above,
    _CenteredOutside,
    _CenteredOutside
};

static const int poses302A[] = {
    _Below|_Left,
    _Above|_Left,
    _Above|_Right,
    _Below|_Right,
    _Left,
    _Left
};

static const int poses3099[] = {
    _Above|_Right,
    _Above|_Right,
};

static const int posesFB1E[] = {
    _Above
};

static const int posesFE20[] = {
    _Above,
    _Above,
    _Above,
    _Above
};

const static struct {
    int low, high;
    const int *pos;
} combiners[] = {
    { 0x300, 0x345, poses300 },
    { 0x360, 0x362, poses360 },
    { 0x385, 0x385, poses385 },
    { 0x483, 0x486, poses483 },
    { 0x488, 0x489, poses488 },
    { 0x591, 0x5A1, poses591 },
    { 0x5A3, 0x5B9, poses5A3 },
    { 0x5BB, 0x5BD, poses5BB },
    { 0x5BF, 0x5BF, poses5BF },
    { 0x5C1, 0x5C2, poses5C1 },
    { 0x5C4, 0x5C4, poses5C4 },
    { 0x64b, 0x655, poses64b },
    { 0x670, 0x670, poses670 },
    { 0x6D6, 0x6E4, poses6D6 },
    { 0x6E7, 0x6E8, poses6E7 },
    { 0x6EA, 0x6ED, poses6EA },
    { 0x711, 0x711, poses711 },
    { 0x730, 0x74A, poses730 },
    { 0x7A6, 0x7B0, poses7A6 },
    { 0x901, 0x903, poses901 },
    { 0x93C, 0x93C, poses93C },
    { 0x93E, 0x94D, poses93E },
    { 0x951, 0x954, poses951 },
    { 0x962, 0x963, poses962 },
    { 0x981, 0x983, poses981 },
    { 0x9BC, 0x9BC, poses9BC },
    { 0x9BE, 0x9C4, poses9BE },
    { 0x9C7, 0x9C8, poses9C7 },
    { 0x9CB, 0x9CD, poses9CB },
    { 0x9D7, 0x9D7, poses9D7 },
    { 0x9E2, 0x9E3, poses9E2 },
    { 0xA02, 0xA02, posesA02 },
    { 0xA3C, 0xA3C, posesA3C },
    { 0xA3E, 0xA42, posesA3E },
    { 0xA47, 0xA48, posesA47 },
    { 0xA4B, 0xA4D, posesA4B },
    { 0xA70, 0xA71, posesA70 },
    { 0xA81, 0xA83, posesA81 },
    { 0xABC, 0xABC, posesABC },
    { 0xABE, 0xAC5, posesABE },
    { 0xAC7, 0xAC9, posesAC7 },
    { 0xACB, 0xACD, posesACB },
    { 0xB01, 0xB03, posesB01 },
    { 0xB3C, 0xB3C, posesB3C },
    { 0xB3E, 0xB43, posesB3E },
    { 0xB47, 0xB48, posesB47 },
    { 0xB4B, 0xB4D, posesB4B },
    { 0xB56, 0xB56, posesB56 },
    { 0xB82, 0xB83, posesB82 },
    { 0xBBE, 0xBC2, posesBBE },
    { 0xBC6, 0xBC8, posesBC6 },
    { 0xBCA, 0xBCD, posesBCA },
    { 0xBD7, 0xBD7, posesBD7 },
    { 0xC01, 0xC03, posesC01 },
    { 0xC3E, 0xC44, posesC3E },
    { 0xC46, 0xC48, posesC46 },
    { 0xC4A, 0xC4D, posesC4A },
    { 0xC55, 0xC56, posesC55 },
    { 0xC82, 0xC83, posesC82 },
    { 0xCBE, 0xCC4, posesCBE },
    { 0xCC6, 0xCC8, posesCC6 },
    { 0xCCA, 0xCCD, posesCCA },
    { 0xCD5, 0xCD6, posesCD5 },
    { 0xD02, 0xD03, posesD02 },
    { 0xD3E, 0xD43, posesD3E },
    { 0xD46, 0xD48, posesD46 },
    { 0xD4A, 0xD4D, posesD4A },
    { 0xD57, 0xD57, posesD57 },
    { 0xD82, 0xD83, posesD82 },
    { 0xDCA, 0xDCA, posesDCA },
    { 0xDCF, 0xDD4, posesDCF },
    { 0xDD6, 0xDD6, posesDD6 },
    { 0xDD8, 0xDDF, posesDD8 },
    { 0xDF2, 0xDF3, posesDF2 },
    { 0xE31, 0xE31, posesE31 },
    { 0xE34, 0xE3A, posesE34 },
    { 0xE47, 0xE4e, posesE47 },
    { 0xEB1, 0xEB1, posesEB1 },
    { 0xEB4, 0xEB9, posesEB4 },
    { 0xEBB, 0xEBC, posesEBB },
    { 0xEC8, 0xECD, posesEC8 },
    { 0xF18, 0xF19, posesF18 },
    { 0xF35, 0xF35, posesF35 },
    { 0xF37, 0xF37, posesF37 },
    { 0xF39, 0xF39, posesF39 },
    { 0xF3E, 0xF3F, posesF3E },
    { 0xF71, 0xF84, posesF71 },
    { 0xF86, 0xF87, posesF86 },
    { 0xF90, 0xF97, posesF90 },
    { 0xF99, 0xFBC, posesF99 },
    { 0xFC6, 0xFC6, posesFC6 },
    { 0x102c, 0x1032, poses102C },
    { 0x1036, 0x1039, poses1036 },
    { 0x1056, 0x1059, poses1056 },
    { 0x17B4, 0x17D3, poses17B4 },
    { 0x18A9, 0x18A9, poses18A9 },
    { 0x1FBD, 0x1FC2, poses1FBD },
    { 0x1FCD, 0x1FCF, poses1FCD },
    { 0x1FDD, 0x1FDF, poses1FDD },
    { 0x1FED, 0x1FEF, poses1FED },
    { 0x1FFD, 0x1FFE, poses1FFD },
    { 0x20D0, 0x20E3, poses20D0 },
    { 0x302A, 0x302F, poses302A },
    { 0x3099, 0x309A, poses3099 },
    { 0xFB1E, 0xFB1E, posesFB1E },
    { 0xFE20, 0xFB23, posesFE20 },
    { 0 }
};
