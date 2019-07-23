#ifndef FONTFORGE_UNICODE_COMBINERS_H
#define FONTFORGE_UNICODE_COMBINERS_H

static const int poses300[] = {
    FF_UNICODE_Above,	/* 0x300 */
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
    FF_UNICODE_Above,	/* 0x310 */
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above|FF_UNICODE_Right,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Above|FF_UNICODE_Right,
    FF_UNICODE_Above|FF_UNICODE_Right|FF_UNICODE_Touching,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,	/* 0x320 */
    FF_UNICODE_Below|FF_UNICODE_Touching,
    FF_UNICODE_Below|FF_UNICODE_Touching,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below|FF_UNICODE_Touching,
    FF_UNICODE_Below|FF_UNICODE_Touching,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,	/* 0x330 */
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Overstrike,
    FF_UNICODE_Overstrike,
    FF_UNICODE_Overstrike,
    FF_UNICODE_Overstrike,
    FF_UNICODE_Overstrike,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,	/* 0x340 */
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Below,
    FF_UNICODE_Above,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    0,
    FF_UNICODE_Above,	/* 0x350 */
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Above,
    FF_UNICODE_Above|FF_UNICODE_Right,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Above,
    FF_UNICODE_Below|FF_UNICODE_Joins2,
    FF_UNICODE_Above|FF_UNICODE_Joins2,
    FF_UNICODE_Above|FF_UNICODE_Joins2,
    FF_UNICODE_Below|FF_UNICODE_Joins2,
    FF_UNICODE_Above|FF_UNICODE_Joins2,	/* 0x360 */
    FF_UNICODE_Above|FF_UNICODE_Joins2,
    FF_UNICODE_Below|FF_UNICODE_Joins2,
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
};
static const int poses385[] = {
    FF_UNICODE_Above
};
static const int poses483[] = {
    FF_UNICODE_Above|FF_UNICODE_CenterLeft,
    FF_UNICODE_Above|FF_UNICODE_CenterLeft,
    FF_UNICODE_Above|FF_UNICODE_CenterLeft,
    FF_UNICODE_Above|FF_UNICODE_CenterLeft,
    FF_UNICODE_Above,
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
    FF_UNICODE_Above|FF_UNICODE_LeftEdge,
    FF_UNICODE_Below,
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
    FF_UNICODE_Above|FF_UNICODE_LeftEdge,
    FF_UNICODE_Above|FF_UNICODE_LeftEdge,
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
    FF_UNICODE_Above,
    FF_UNICODE_Above
};

static const int poses5C7[] = {
    FF_UNICODE_Below
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
    0,
    0,
    0 /* FIXME this is broken, what is this block even corresponding to? */
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

static const int poses7EB[] = {
    FF_UNICODE_Above,	/* 0x7EB */
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Below,
    FF_UNICODE_Above,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,	/* 0x7FB */
    0,
    FF_UNICODE_Below,
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

static const int posesC00[] = {
    FF_UNICODE_Above,
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
    FF_UNICODE_Below,
    FF_UNICODE_Below,
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

static const int poses135E[] = {
    FF_UNICODE_Above,
    FF_UNICODE_Above
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

static const int poses1A7F[] = {
    FF_UNICODE_Below
};

static const int poses1AB0[] = {
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below|FF_UNICODE_CenterLeft,
    FF_UNICODE_Below|FF_UNICODE_CenterLeft,
    FF_UNICODE_Above|FF_UNICODE_Left|FF_UNICODE_Right,
    FF_UNICODE_Above|FF_UNICODE_Left|FF_UNICODE_Right,
    FF_UNICODE_Below|FF_UNICODE_Left|FF_UNICODE_Right,
    FF_UNICODE_Left|FF_UNICODE_Right
};

static const int poses1B6B[] = {
    FF_UNICODE_Above,
    FF_UNICODE_Below,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above
};

static const int poses1DC0[] = {
    FF_UNICODE_Above,	/* 0x1DC0 */
    FF_UNICODE_Above,
    FF_UNICODE_Below,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Below,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above|FF_UNICODE_Joins2,
    FF_UNICODE_Above|FF_UNICODE_Touching,
    FF_UNICODE_Below,
    FF_UNICODE_Below|FF_UNICODE_Touching,	/* 0x1DD0 */
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
    FF_UNICODE_Above,	/* 0x1DE0 */
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
    FF_UNICODE_Above,	/* 0x1DF0 */
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above|FF_UNICODE_Right,
    FF_UNICODE_Above|FF_UNICODE_Left,
    FF_UNICODE_Above|FF_UNICODE_Left,
    FF_UNICODE_Below,
    0,
    FF_UNICODE_Above,
    FF_UNICODE_Below|FF_UNICODE_Joins2,
    FF_UNICODE_Below,
    FF_UNICODE_Above,
    FF_UNICODE_Below,
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
    FF_UNICODE_Overstrike,	/* 20E0 */
    FF_UNICODE_Above,
    FF_UNICODE_CenteredOutside,
    FF_UNICODE_CenteredOutside,
    FF_UNICODE_CenteredOutside,
    FF_UNICODE_Overstrike,
    FF_UNICODE_Overstrike,
    FF_UNICODE_Above,
    FF_UNICODE_Below,
    FF_UNICODE_Above,
    FF_UNICODE_Overstrike,
    FF_UNICODE_Overstrike,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Above
};

static const int poses2CEF[] = {
    FF_UNICODE_Above|FF_UNICODE_Right,
    FF_UNICODE_Above,
    FF_UNICODE_Above
};

static const int poses2DE0[] = {
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
    FF_UNICODE_Above,		/* 2DF0 */
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
    FF_UNICODE_Above
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

static const int posesA66F[] = {
    FF_UNICODE_Above,
    FF_UNICODE_CenteredOutside,
    FF_UNICODE_CenteredOutside,
    FF_UNICODE_CenteredOutside
};

static const int posesA674[] = {
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above
};

static const int posesA69E[] = {
    FF_UNICODE_Above,
    FF_UNICODE_Above
};

static const int posesA6F0[] = {
    FF_UNICODE_Above,
    FF_UNICODE_Above
};

static const int posesA8E0[] = {
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
    FF_UNICODE_Above,
    FF_UNICODE_Above
};

static const int posesFB1E[] = {
    FF_UNICODE_Above
};

static const int posesFE20[] = {
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Above,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Below,
    FF_UNICODE_Above,
    FF_UNICODE_Above
};

static const struct {
    long low, high;
    const int *pos;
    size_t sz;
} combiners[] = {
    { 0x300, 0x36F, poses300, sizeof(poses300)/sizeof(int) },
    { 0x385, 0x385, poses385, sizeof(poses385)/sizeof(int) },
    { 0x483, 0x489, poses483, sizeof(poses483)/sizeof(int) },
    { 0x591, 0x5BD, poses591, sizeof(poses591)/sizeof(int) },
    { 0x5BF, 0x5BF, poses5BF, sizeof(poses5BF)/sizeof(int) },
    { 0x5C1, 0x5C2, poses5C1, sizeof(poses5C1)/sizeof(int) },
    { 0x5C4, 0x5C5, poses5C4, sizeof(poses5C4)/sizeof(int) },
    { 0x5C7, 0x5C7, poses5C7, sizeof(poses5C7)/sizeof(int) },
    { 0x64b, 0x655, poses64b, sizeof(poses64b)/sizeof(int) },
    { 0x670, 0x670, poses670, sizeof(poses670)/sizeof(int) },
    { 0x6D6, 0x6E4, poses6D6, sizeof(poses6D6)/sizeof(int) },
    { 0x6E7, 0x6E8, poses6E7, sizeof(poses6E7)/sizeof(int) },
    { 0x6EA, 0x6ED, poses6EA, sizeof(poses6EA)/sizeof(int) },
    { 0x711, 0x711, poses711, sizeof(poses711)/sizeof(int) },
    { 0x730, 0x74A, poses730, sizeof(poses730)/sizeof(int) },
    { 0x7A6, 0x7B0, poses7A6, sizeof(poses7A6)/sizeof(int) },
    { 0x7EB, 0x7FD, poses7EB, sizeof(poses7EB)/sizeof(int) },
    { 0x901, 0x903, poses901, sizeof(poses901)/sizeof(int) },
    { 0x93C, 0x93C, poses93C, sizeof(poses93C)/sizeof(int) },
    { 0x93E, 0x94D, poses93E, sizeof(poses93E)/sizeof(int) },
    { 0x951, 0x954, poses951, sizeof(poses951)/sizeof(int) },
    { 0x962, 0x963, poses962, sizeof(poses962)/sizeof(int) },
    { 0x981, 0x983, poses981, sizeof(poses981)/sizeof(int) },
    { 0x9BC, 0x9BC, poses9BC, sizeof(poses9BC)/sizeof(int) },
    { 0x9BE, 0x9C4, poses9BE, sizeof(poses9BE)/sizeof(int) },
    { 0x9C7, 0x9C8, poses9C7, sizeof(poses9C7)/sizeof(int) },
    { 0x9CB, 0x9CD, poses9CB, sizeof(poses9CB)/sizeof(int) },
    { 0x9D7, 0x9D7, poses9D7, sizeof(poses9D7)/sizeof(int) },
    { 0x9E2, 0x9E3, poses9E2, sizeof(poses9E2)/sizeof(int) },
    { 0xA02, 0xA02, posesA02, sizeof(posesA02)/sizeof(int) },
    { 0xA3C, 0xA3C, posesA3C, sizeof(posesA3C)/sizeof(int) },
    { 0xA3E, 0xA42, posesA3E, sizeof(posesA3E)/sizeof(int) },
    { 0xA47, 0xA48, posesA47, sizeof(posesA47)/sizeof(int) },
    { 0xA4B, 0xA4D, posesA4B, sizeof(posesA4B)/sizeof(int) },
    { 0xA70, 0xA71, posesA70, sizeof(posesA70)/sizeof(int) },
    { 0xA81, 0xA83, posesA81, sizeof(posesA81)/sizeof(int) },
    { 0xABC, 0xABC, posesABC, sizeof(posesABC)/sizeof(int) },
    { 0xABE, 0xAC5, posesABE, sizeof(posesABE)/sizeof(int) },
    { 0xAC7, 0xAC9, posesAC7, sizeof(posesAC7)/sizeof(int) },
    { 0xACB, 0xACD, posesACB, sizeof(posesACB)/sizeof(int) },
    { 0xB01, 0xB03, posesB01, sizeof(posesB01)/sizeof(int) },
    { 0xB3C, 0xB3C, posesB3C, sizeof(posesB3C)/sizeof(int) },
    { 0xB3E, 0xB43, posesB3E, sizeof(posesB3E)/sizeof(int) },
    { 0xB47, 0xB48, posesB47, sizeof(posesB47)/sizeof(int) },
    { 0xB4B, 0xB4D, posesB4B, sizeof(posesB4B)/sizeof(int) },
    /*{ 0xB56, 0xB56, posesB56, sizeof(posesB56)/sizeof(int) },*/ /* FIXME this looks broken? */
    { 0xB82, 0xB83, posesB82, sizeof(posesB82)/sizeof(int) },
    { 0xBBE, 0xBC2, posesBBE, sizeof(posesBBE)/sizeof(int) },
    { 0xBC6, 0xBC8, posesBC6, sizeof(posesBC6)/sizeof(int) },
    { 0xBCA, 0xBCD, posesBCA, sizeof(posesBCA)/sizeof(int) },
    { 0xBD7, 0xBD7, posesBD7, sizeof(posesBD7)/sizeof(int) },
    { 0xC00, 0xC03, posesC00, sizeof(posesC00)/sizeof(int) },
    { 0xC3E, 0xC44, posesC3E, sizeof(posesC3E)/sizeof(int) },
    { 0xC46, 0xC48, posesC46, sizeof(posesC46)/sizeof(int) },
    { 0xC4A, 0xC4D, posesC4A, sizeof(posesC4A)/sizeof(int) },
    { 0xC55, 0xC56, posesC55, sizeof(posesC55)/sizeof(int) },
    { 0xC82, 0xC83, posesC82, sizeof(posesC82)/sizeof(int) },
    { 0xCBE, 0xCC4, posesCBE, sizeof(posesCBE)/sizeof(int) },
    { 0xCC6, 0xCC8, posesCC6, sizeof(posesCC6)/sizeof(int) },
    { 0xCCA, 0xCCD, posesCCA, sizeof(posesCCA)/sizeof(int) },
    { 0xCD5, 0xCD6, posesCD5, sizeof(posesCD5)/sizeof(int) },
    { 0xD02, 0xD03, posesD02, sizeof(posesD02)/sizeof(int) },
    { 0xD3E, 0xD43, posesD3E, sizeof(posesD3E)/sizeof(int) },
    { 0xD46, 0xD48, posesD46, sizeof(posesD46)/sizeof(int) },
    { 0xD4A, 0xD4D, posesD4A, sizeof(posesD4A)/sizeof(int) },
    { 0xD57, 0xD57, posesD57, sizeof(posesD57)/sizeof(int) },
    { 0xD82, 0xD83, posesD82, sizeof(posesD82)/sizeof(int) },
    { 0xDCA, 0xDCA, posesDCA, sizeof(posesDCA)/sizeof(int) },
    { 0xDCF, 0xDD4, posesDCF, sizeof(posesDCF)/sizeof(int) },
    { 0xDD6, 0xDD6, posesDD6, sizeof(posesDD6)/sizeof(int) },
    { 0xDD8, 0xDDF, posesDD8, sizeof(posesDD8)/sizeof(int) },
    { 0xDF2, 0xDF3, posesDF2, sizeof(posesDF2)/sizeof(int) },
    { 0xE31, 0xE31, posesE31, sizeof(posesE31)/sizeof(int) },
    { 0xE34, 0xE3A, posesE34, sizeof(posesE34)/sizeof(int) },
    { 0xE47, 0xE4e, posesE47, sizeof(posesE47)/sizeof(int) },
    { 0xEB1, 0xEB1, posesEB1, sizeof(posesEB1)/sizeof(int) },
    { 0xEB4, 0xEB9, posesEB4, sizeof(posesEB4)/sizeof(int) },
    { 0xEBB, 0xEBC, posesEBB, sizeof(posesEBB)/sizeof(int) },
    { 0xEC8, 0xECD, posesEC8, sizeof(posesEC8)/sizeof(int) },
    { 0xF18, 0xF19, posesF18, sizeof(posesF18)/sizeof(int) },
    { 0xF35, 0xF35, posesF35, sizeof(posesF35)/sizeof(int) },
    { 0xF37, 0xF37, posesF37, sizeof(posesF37)/sizeof(int) },
    { 0xF39, 0xF39, posesF39, sizeof(posesF39)/sizeof(int) },
    { 0xF3E, 0xF3F, posesF3E, sizeof(posesF3E)/sizeof(int) },
    { 0xF71, 0xF84, posesF71, sizeof(posesF71)/sizeof(int) },
    { 0xF86, 0xF87, posesF86, sizeof(posesF86)/sizeof(int) },
    { 0xF90, 0xF97, posesF90, sizeof(posesF90)/sizeof(int) },
    { 0xF99, 0xFBC, posesF99, sizeof(posesF99)/sizeof(int) },
    { 0xFC6, 0xFC6, posesFC6, sizeof(posesFC6)/sizeof(int) },
    { 0x102c, 0x1032, poses102C, sizeof(poses102C)/sizeof(int) },
    { 0x1036, 0x1039, poses1036, sizeof(poses1036)/sizeof(int) },
    { 0x1056, 0x1059, poses1056, sizeof(poses1056)/sizeof(int) },
    { 0x135E, 0x135F, poses135E, sizeof(poses135E)/sizeof(int) },
    { 0x17B4, 0x17D3, poses17B4, sizeof(poses17B4)/sizeof(int) },
    { 0x18A9, 0x18A9, poses18A9, sizeof(poses18A9)/sizeof(int) },
    { 0x1A7F, 0x1A7F, poses1A7F, sizeof(poses1A7F)/sizeof(int) },
    { 0x1AB0, 0x1ABE, poses1AB0, sizeof(poses1AB0)/sizeof(int) },
    { 0x1B6B, 0x1B73, poses1B6B, sizeof(poses1B6B)/sizeof(int) },
    { 0x1DC0, 0x1DFF, poses1DC0, sizeof(poses1DC0)/sizeof(int) },
    { 0x1FBD, 0x1FC1, poses1FBD, sizeof(poses1FBD)/sizeof(int) },
    { 0x1FCD, 0x1FCF, poses1FCD, sizeof(poses1FCD)/sizeof(int) },
    { 0x1FDD, 0x1FDF, poses1FDD, sizeof(poses1FDD)/sizeof(int) },
    { 0x1FED, 0x1FEF, poses1FED, sizeof(poses1FED)/sizeof(int) },
    { 0x1FFD, 0x1FFE, poses1FFD, sizeof(poses1FFD)/sizeof(int) },
    { 0x20D0, 0x20F0, poses20D0, sizeof(poses20D0)/sizeof(int) },
    { 0x2CEF, 0x2CF1, poses2CEF, sizeof(poses2CEF)/sizeof(int) },
    { 0x2DE0, 0x2DFF, poses2DE0, sizeof(poses2DE0)/sizeof(int) },
    { 0x302A, 0x302F, poses302A, sizeof(poses302A)/sizeof(int) },
    { 0x3099, 0x309A, poses3099, sizeof(poses3099)/sizeof(int) },
    { 0xA66F, 0xA672, posesA66F, sizeof(posesA66F)/sizeof(int) },
    { 0xA674, 0xA67D, posesA674, sizeof(posesA674)/sizeof(int) },
    { 0xA69E, 0xA69F, posesA69E, sizeof(posesA69E)/sizeof(int) },
    { 0xA6F0, 0xA6F1, posesA6F0, sizeof(posesA6F0)/sizeof(int) },
    { 0xA8E0, 0xA8F1, posesA8E0, sizeof(posesA8E0)/sizeof(int) },
    { 0xFB1E, 0xFB1E, posesFB1E, sizeof(posesFB1E)/sizeof(int) },
    { 0xFE20, 0xFE2F, posesFE20, sizeof(posesFE20)/sizeof(int) },
    { -1 }
};

#endif /* FONTFORGE_UNICODE_COMBINERS_H */
