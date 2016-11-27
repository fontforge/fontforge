#ifndef FONTFORGE_UNICODE_COMBINERS_H
#define FONTFORGE_UNICODE_COMBINERS_H

static const int poses300[] = {
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,		/* 0x310 */
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above|FF_UNICODE_CenterRight,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Above,
    FF_UNICODE_Above|FF_UNICODE_Right|FF_UNICODE_Touching,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,		/* 0x320 */
    FF_UNICODE_Below|FF_UNICODE_Touching,
    FF_UNICODE_Below|FF_UNICODE_Touching,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below|FF_UNICODE_Touching,
    FF_UNICODE_Below|FF_UNICODE_Touching, /* 0x328 */
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,		/* 0x330 */
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Overstrike,
    FF_UNICODE_Overstrike,
    FF_UNICODE_Overstrike,
    FF_UNICODE_Overstrike,
    FF_UNICODE_Overstrike,	/* 0x338 */
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above|FF_UNICODE_Left, /* 0x340 */
    FF_UNICODE_Above|FF_UNICODE_Right,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Below,
    0
};
static const int poses360[] = {
    FF_UNICODE_Above|FF_UNICODE_Joins2,
    FF_UNICODE_Above|FF_UNICODE_Joins2,
    FF_UNICODE_Below|FF_UNICODE_Joins2,
    0
};
static const int poses385[] = {
    FF_UNICODE_Above
};
static const int poses483[] = {
    FF_UNICODE_Above|FF_UNICODE_CenterLeft,
    FF_UNICODE_Above|FF_UNICODE_CenterLeft,
    FF_UNICODE_Above|FF_UNICODE_CenterLeft,
    FF_UNICODE_Above|FF_UNICODE_CenterLeft,
    0
};

static const int poses488[] = {
    FF_UNICODE_CenteredOutside,	/* 8 down half-circles distributed in a circle around the character */
    FF_UNICODE_CenteredOutside	/* 8 commas rotated as moved around circle, bottom is normal comma */
};

static const int poses591[] = {
    FF_UNICODE_Below,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Below|FF_UNICODE_CenterRight,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above|FF_UNICODE_LeftEdge,
    FF_UNICODE_Below|FF_UNICODE_RightEdge,
    FF_UNICODE_Below,
    FF_UNICODE_Above|FF_UNICODE_CenterRight,
    FF_UNICODE_Above|FF_UNICODE_RightEdge,
    FF_UNICODE_Above|FF_UNICODE_CenterRight,
    FF_UNICODE_Above,
    FF_UNICODE_Above|FF_UNICODE_RightEdge, /* 05a0 */
    FF_UNICODE_Above|FF_UNICODE_LeftEdge
};

static const int poses5A3[] = {
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below|FF_UNICODE_CenterLeft,
    FF_UNICODE_Below|FF_UNICODE_CenterLeft,
    FF_UNICODE_Below,
    FF_UNICODE_Above|FF_UNICODE_CenterLeft,
    FF_UNICODE_Above|FF_UNICODE_LeftEdge,
    FF_UNICODE_Below,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Below|FF_UNICODE_RightEdge,
    FF_UNICODE_Above|FF_UNICODE_LeftEdge,
    FF_UNICODE_Above,
    FF_UNICODE_Below,		/* 05b0 */
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,		/* 05b8 */
    FF_UNICODE_Above|FF_UNICODE_LeftEdge
};

static const int poses5BB[] = {
    FF_UNICODE_Below,
    FF_UNICODE_Overstrike,
    FF_UNICODE_Below
};

static const int poses5BF[] = {
    FF_UNICODE_Above
};

static const int poses5C1[] = {
    FF_UNICODE_Above|FF_UNICODE_RightEdge,
    FF_UNICODE_Above|FF_UNICODE_LeftEdge
};

static const int poses5C4[] = {
    FF_UNICODE_Above
};

static const int poses64b[] = {
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Below,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Below,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    0
};

static const int poses670[] = {
    FF_UNICODE_Above
};

static const int poses6D6[] = {
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Overstrike,
    FF_UNICODE_Overstrike,
    FF_UNICODE_Above,
    FF_UNICODE_Above,		/* 6e0 */
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Below,
    FF_UNICODE_Above
};

static const int poses6E7[] = {
    FF_UNICODE_Above,
    FF_UNICODE_Above
};

static const int poses6EA[] = {
    FF_UNICODE_Below,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Below
};

static const int poses711[] = {
    FF_UNICODE_Above
};

static const int poses730[] = {
    FF_UNICODE_Above,
    FF_UNICODE_Below,
    FF_UNICODE_CenteredOutside,	/* Two dots, one above CenterRight, one below CenterLeft */
    FF_UNICODE_Above,
    FF_UNICODE_Below,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Above,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Above,
    FF_UNICODE_Below,
    FF_UNICODE_Above,
    FF_UNICODE_Above|FF_UNICODE_LeftEdge, /* 0740 */
    FF_UNICODE_Above,
    FF_UNICODE_Below,
    FF_UNICODE_Above,
    FF_UNICODE_Below,
    FF_UNICODE_Above,
    FF_UNICODE_Below,
    FF_UNICODE_Above,
    FF_UNICODE_Below,
    FF_UNICODE_Above,
    FF_UNICODE_Above
};

static const int poses7A6[] = {
    FF_UNICODE_Above|FF_UNICODE_CenterLeft,
    FF_UNICODE_Above|FF_UNICODE_CenterLeft,
    FF_UNICODE_Below|FF_UNICODE_CenterLeft,
    FF_UNICODE_Below|FF_UNICODE_CenterLeft,
    FF_UNICODE_Above|FF_UNICODE_CenterLeft,
    FF_UNICODE_Above|FF_UNICODE_CenterLeft,
    FF_UNICODE_Above|FF_UNICODE_CenterLeft,
    FF_UNICODE_Above|FF_UNICODE_CenterLeft,
    FF_UNICODE_Above|FF_UNICODE_CenterLeft,
    FF_UNICODE_Above|FF_UNICODE_CenterLeft,
    FF_UNICODE_Above|FF_UNICODE_CenterLeft,
};

static const int poses901[] = {
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Right
};

static const int poses93C[] = {
    FF_UNICODE_Below
};

static const int poses93E[] = {
    FF_UNICODE_Right,
    FF_UNICODE_Left,
    FF_UNICODE_Right,		/* 940 */
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Above,
    FF_UNICODE_Above|FF_UNICODE_CenterLeft,
    FF_UNICODE_Above|FF_UNICODE_CenterLeft,
    FF_UNICODE_Above|FF_UNICODE_CenterLeft,
    FF_UNICODE_Right,
    FF_UNICODE_Right,
    FF_UNICODE_Right,
    FF_UNICODE_Right,
    FF_UNICODE_Below|FF_UNICODE_CenterRight
};

static const int poses951[] = {
    FF_UNICODE_Above,
    FF_UNICODE_Below,
    FF_UNICODE_Above,
    FF_UNICODE_Above
};

static const int poses962[] = {
    FF_UNICODE_Below,
    FF_UNICODE_Below
};

static const int poses981[] = {
    FF_UNICODE_Above,
    FF_UNICODE_Right,
    FF_UNICODE_Right
};

static const int poses9BC[] = {
    FF_UNICODE_Below
};

static const int poses9BE[] = {
    FF_UNICODE_Right,
    FF_UNICODE_Left,
    FF_UNICODE_Right,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below|FF_UNICODE_CenterRight
};

static const int poses9C7[] = {
    FF_UNICODE_Left,
    FF_UNICODE_Left
};

static const int poses9CB[] = {
    FF_UNICODE_Overstrike,
    FF_UNICODE_Overstrike,
    FF_UNICODE_Below
};

static const int poses9D7[] = {
    FF_UNICODE_Right
};

static const int poses9E2[] = {
    FF_UNICODE_Below,
    FF_UNICODE_Below
};

static const int posesA02[] = {
    FF_UNICODE_Above
};

static const int posesA3C[] = {
    FF_UNICODE_Below
};

static const int posesA3E[] = {
    FF_UNICODE_Right,
    FF_UNICODE_Left,
    FF_UNICODE_Right,		/* 0a40 */
    FF_UNICODE_Below,
    FF_UNICODE_Below
};

static const int posesA47[] = {
    FF_UNICODE_Above|FF_UNICODE_CenterLeft,
    FF_UNICODE_Above|FF_UNICODE_CenterLeft
};

static const int posesA4B[] = {
    FF_UNICODE_Above|FF_UNICODE_CenterLeft,
    FF_UNICODE_Above|FF_UNICODE_CenterLeft,
    FF_UNICODE_Below
};

static const int posesA70[] = {
    FF_UNICODE_Above,
    FF_UNICODE_Above
};

static const int posesA81[] = {
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Right
};

static const int posesABC[] = {
    FF_UNICODE_Below
};

static const int posesABE[] = {
    FF_UNICODE_Right,
    FF_UNICODE_Left,
    FF_UNICODE_Right,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Above
};

static const int posesAC7[] = {
    FF_UNICODE_Above|FF_UNICODE_CenterLeft,
    FF_UNICODE_Above|FF_UNICODE_CenterLeft,
    FF_UNICODE_Right
};

static const int posesACB[] = {
    FF_UNICODE_Right,
    FF_UNICODE_Right,
    FF_UNICODE_Below|FF_UNICODE_CenterRight
};

static const int posesB01[] = {
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Right
};

static const int posesB3C[] = {
    FF_UNICODE_Below
};

static const int posesB3E[] = {
    FF_UNICODE_Right,
    FF_UNICODE_Above,
    FF_UNICODE_Right,		/* 0b40 */
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below
};

static const int posesB47[] = {
    FF_UNICODE_Left,
    FF_UNICODE_Outside|FF_UNICODE_Left|FF_UNICODE_Above
};

static const int posesB4B[] = {
    FF_UNICODE_CenteredOutside,
    FF_UNICODE_CenteredOutside,
    FF_UNICODE_Below
};

static const int posesB56[] = {
    FF_UNICODE_Above,
    FF_UNICODE_Right,
};

static const int posesB82[] = {
    FF_UNICODE_Above,
    FF_UNICODE_Right
};

static const int posesBBE[] = {
    FF_UNICODE_Right,
    FF_UNICODE_Right,
    FF_UNICODE_Above,
    FF_UNICODE_Right,
    FF_UNICODE_Right
};

static const int posesBC6[] = {
    FF_UNICODE_Left,
    FF_UNICODE_Left,
    FF_UNICODE_Left
};

static const int posesBCA[] = {
    FF_UNICODE_CenteredOutside,
    FF_UNICODE_CenteredOutside,
    FF_UNICODE_CenteredOutside,
    FF_UNICODE_Above
};

static const int posesBD7[] = {
    FF_UNICODE_Right
};

static const int posesC01[] = {
    FF_UNICODE_Right,
    FF_UNICODE_Right,
    FF_UNICODE_Right
};

static const int posesC3E[] = {
    FF_UNICODE_Above|FF_UNICODE_CenterRight,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Right,
    FF_UNICODE_Right,
    FF_UNICODE_Right,
    FF_UNICODE_Right
};

static const int posesC46[] = {
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_CenteredOutside
};

static const int posesC4A[] = {
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above
};

static const int posesC55[] = {
    FF_UNICODE_Above,
    FF_UNICODE_Below
};

static const int posesC82[] = {
    FF_UNICODE_Right,
    FF_UNICODE_Right
};

static const int posesCBE[] = {
    FF_UNICODE_Right,
    FF_UNICODE_Above,
    FF_UNICODE_Outside|FF_UNICODE_Above|FF_UNICODE_Right,
    FF_UNICODE_Right,
    FF_UNICODE_Right,
    FF_UNICODE_Right,
    FF_UNICODE_Right
};

static const int posesCC6[] = {
    FF_UNICODE_Above,
    FF_UNICODE_Outside|FF_UNICODE_Above|FF_UNICODE_Right,
    FF_UNICODE_Outside|FF_UNICODE_Above|FF_UNICODE_Right
};

static const int posesCCA[] = {
    FF_UNICODE_Outside|FF_UNICODE_Above|FF_UNICODE_Right,
    FF_UNICODE_Outside|FF_UNICODE_Above|FF_UNICODE_Right,
    FF_UNICODE_Above,
    FF_UNICODE_Above
};

static const int posesCD5[] = {
    FF_UNICODE_Right,
    FF_UNICODE_Right
};

static const int posesD02[] = {
    FF_UNICODE_Right,
    FF_UNICODE_Right
};

static const int posesD3E[] = {
    FF_UNICODE_Right,
    FF_UNICODE_Right,
    FF_UNICODE_Right,
    FF_UNICODE_Below|FF_UNICODE_Right,
    FF_UNICODE_Below|FF_UNICODE_Right,
    FF_UNICODE_Below
};

static const int posesD46[] = {
    FF_UNICODE_Left,
    FF_UNICODE_Left,
    FF_UNICODE_Left
};

static const int posesD4A[] = {
    FF_UNICODE_Outside|FF_UNICODE_Left|FF_UNICODE_Right,
    FF_UNICODE_Outside|FF_UNICODE_Left|FF_UNICODE_Right,
    FF_UNICODE_Outside|FF_UNICODE_Left|FF_UNICODE_Right,
    FF_UNICODE_Above|FF_UNICODE_Right
};

static const int posesD57[] = {
    FF_UNICODE_Right
};

static const int posesD82[] = {
    FF_UNICODE_Right,
    FF_UNICODE_Right
};

static const int posesDCA[] = {
    FF_UNICODE_Right
};

static const int posesDCF[] = {
    FF_UNICODE_Right,
    FF_UNICODE_Right,
    FF_UNICODE_Right,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Below
};

static const int posesDD6[] = {
    FF_UNICODE_Below
};

static const int posesDD8[] = {
    FF_UNICODE_Right,
    FF_UNICODE_Left,
    FF_UNICODE_CenteredOutside,
    FF_UNICODE_Left,
    FF_UNICODE_CenteredOutside,
    FF_UNICODE_CenteredOutside,
    FF_UNICODE_CenteredOutside,
    FF_UNICODE_Right
};

static const int posesDF2[] = {
    FF_UNICODE_Right,
    FF_UNICODE_Right
};

static const int posesE31[] = {
    FF_UNICODE_Above
};

static const int posesE34[] = {
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Below|FF_UNICODE_Right,
    FF_UNICODE_Below|FF_UNICODE_CenterRight,
    FF_UNICODE_Below|FF_UNICODE_Right
};

static const int posesE47[] = {
    FF_UNICODE_Above,
    FF_UNICODE_Above|FF_UNICODE_Right,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above|FF_UNICODE_CenterRight,
    FF_UNICODE_Above,
    FF_UNICODE_Above|FF_UNICODE_Right,
    FF_UNICODE_Above|FF_UNICODE_Right,
};

static const int posesEB1[] = {
    FF_UNICODE_Above
};

static const int posesEB4[] = {
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Below,
    FF_UNICODE_Below
};

static const int posesEBB[] = {
    FF_UNICODE_Above,
    FF_UNICODE_Below
};

static const int posesEC8[] = {
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above
};

static const int posesF18[] = {
    FF_UNICODE_Below|FF_UNICODE_Right
};

static const int posesF35[] = {
    FF_UNICODE_Below
};

static const int posesF37[] = {
    FF_UNICODE_Below
};

static const int posesF39[] = {
    FF_UNICODE_Above|FF_UNICODE_Right|FF_UNICODE_Touching
};

static const int posesF3E[] = {
    FF_UNICODE_Below|FF_UNICODE_Right,
    FF_UNICODE_Below|FF_UNICODE_Left
};

static const int posesF71[] = {
    FF_UNICODE_Below,
    FF_UNICODE_Above,
    FF_UNICODE_Outside|FF_UNICODE_Above|FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Outside|FF_UNICODE_Above|FF_UNICODE_Below,
    FF_UNICODE_Outside|FF_UNICODE_Above|FF_UNICODE_Below,
    FF_UNICODE_Outside|FF_UNICODE_Above|FF_UNICODE_Below,
    FF_UNICODE_Outside|FF_UNICODE_Above|FF_UNICODE_Below,
    FF_UNICODE_Above|FF_UNICODE_CenterLeft,
    FF_UNICODE_Above|FF_UNICODE_CenterLeft,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Right,
    FF_UNICODE_Above,		/* 0f80 */
    FF_UNICODE_Outside|FF_UNICODE_Above|FF_UNICODE_Below,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Below|FF_UNICODE_Left
};

static const int posesF86[] = {
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above
};

static const int posesF90[] = {
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below
};

static const int posesF99[] = {
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below
};

static const int posesFC6[] = {
    FF_UNICODE_Below
};

static const int poses102C[] = {
    FF_UNICODE_Right,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Below,
    FF_UNICODE_Below,		/* 1030 */
    FF_UNICODE_Left,
    FF_UNICODE_Above
};

static const int poses1036[] = {
    FF_UNICODE_Above,
    FF_UNICODE_Below,
    FF_UNICODE_Right,
    FF_UNICODE_Above
};

static const int poses1056[] = {
    FF_UNICODE_Right,
    FF_UNICODE_Right,
    FF_UNICODE_Below,
    FF_UNICODE_Below
};

static const int poses17B4[] = {
    FF_UNICODE_Overstrike,
    FF_UNICODE_Overstrike,
    FF_UNICODE_Right,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Outside|FF_UNICODE_Left|FF_UNICODE_Above,
    FF_UNICODE_CenteredOutside,
    FF_UNICODE_CenteredOutside,	/* 17c0 */
    FF_UNICODE_Left,
    FF_UNICODE_Left,
    FF_UNICODE_Left,
    FF_UNICODE_CenteredOutside,
    FF_UNICODE_CenteredOutside,
    FF_UNICODE_Above,
    FF_UNICODE_Right,
    FF_UNICODE_Right,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above|FF_UNICODE_CenterRight,
    FF_UNICODE_Above|FF_UNICODE_CenterRight,
    FF_UNICODE_Above,
    FF_UNICODE_Above|FF_UNICODE_CenterRight, /* 17d0 */
    FF_UNICODE_Above,
    FF_UNICODE_Below,
    FF_UNICODE_Above
};

static const int poses18A9[] = {
    FF_UNICODE_Above|FF_UNICODE_Left
};

static const int poses1FBD[] = {	/* These aren't listed as combiners, but if we don't use them as such greek fonts don't work */
    FF_UNICODE_Above,
    FF_UNICODE_Right,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above
};

static const int poses1FCD[] = {	/* These aren't listed as combiners, but if we don't use them as such greek fonts don't work */
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above
};

static const int poses1FDD[] = {	/* These aren't listed as combiners, but if we don't use them as such greek fonts don't work */
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above
};

static const int poses1FED[] = {	/* These aren't listed as combiners, but if we don't use them as such greek fonts don't work */
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above
};

static const int poses1FFD[] = {	/* These aren't listed as combiners, but if we don't use them as such greek fonts don't work */
    FF_UNICODE_Above,
    FF_UNICODE_Above
};

static const int poses20D0[] = {
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Overstrike,
    FF_UNICODE_Overstrike,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Overstrike,
    FF_UNICODE_Overstrike,
    FF_UNICODE_Overstrike,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_CenteredOutside,
    FF_UNICODE_CenteredOutside,
    FF_UNICODE_CenteredOutside,
    FF_UNICODE_Overstrike,
    FF_UNICODE_Above,
    FF_UNICODE_CenteredOutside,
    FF_UNICODE_CenteredOutside
};

static const int poses302A[] = {
    FF_UNICODE_Below|FF_UNICODE_Left,
    FF_UNICODE_Above|FF_UNICODE_Left,
    FF_UNICODE_Above|FF_UNICODE_Right,
    FF_UNICODE_Below|FF_UNICODE_Right,
    FF_UNICODE_Left,
    FF_UNICODE_Left
};

static const int poses3099[] = {
    FF_UNICODE_Above|FF_UNICODE_Right,
    FF_UNICODE_Above|FF_UNICODE_Right,
};

static const int posesFB1E[] = {
    FF_UNICODE_Above
};

static const int posesFE20[] = {
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above
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

#endif /* FONTFORGE_UNICODE_COMBINERS_H */
