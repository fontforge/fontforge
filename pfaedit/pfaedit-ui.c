#include "pfaeditui.h"

static const unichar_t str0[] = { 'E', 'n', 'g', 'l', 'i', 's', 'h',  0 };
static const unichar_t str1[] = { 'O', 'K',  0 };
static const unichar_t str2[] = { 'C', 'a', 'n', 'c', 'e', 'l',  0 };
static const unichar_t str3[] = { 'O', 'p', 'e', 'n',  0 };
static const unichar_t str4[] = { 'S', 'a', 'v', 'e',  0 };
static const unichar_t str5[] = { 'F', 'i', 'l', 't', 'e', 'r',  0 };
static const unichar_t str6[] = { 'N', 'e', 'w',  0 };
static const unichar_t str7[] = { 'R', 'e', 'p', 'l', 'a', 'c', 'e',  0 };
static const unichar_t str8[] = { 'F', 'i', 'l', 'e', ' ', 'E', 'x', 'i', 's', 't', 's',  0 };
static const unichar_t str9[] = { 'F', 'i', 'l', 'e', ',', ' ',  0 };
static const unichar_t str10[] = { ',', ' ', 'e', 'x', 'i', 's', 't', 's', '.', ' ', 'R', 'e', 'p', 'l', 'a', 'c', 'e', ' ', 'i', 't', '?',  0 };
static const unichar_t str11[] = { 'C', 'r', 'e', 'a', 't', 'e', ' ', 'd', 'i', 'r', 'e', 'c', 't', 'o', 'r', 'y', '.', '.', '.',  0 };
static const unichar_t str12[] = { 'D', 'i', 'r', 'e', 'c', 't', 'o', 'r', 'y', ' ', 'n', 'a', 'm', 'e', '?',  0 };
static const unichar_t str13[] = { 'C', 'o', 'u', 'l', 'd', 'n', 0x27, 't', ' ', 'c', 'r', 'e', 'a', 't', 'e', ' ', 'd', 'i', 'r', 'e', 'c', 't', 'o', 'r', 'y',  0 };
static const unichar_t str14[] = { 'F', 'i', 'l', 'e',  0 };
static const unichar_t str15[] = { 'E', 'd', 'i', 't',  0 };
static const unichar_t str16[] = { 'P', 'o', 'i', 'n', 't',  0 };
static const unichar_t str17[] = { 'E', 'l', 'e', 'm', 'e', 'n', 't',  0 };
static const unichar_t str18[] = { 'H', 'i', 'n', 't', 's',  0 };
static const unichar_t str19[] = { 'V', 'i', 'e', 'w',  0 };
static const unichar_t str20[] = { 'M', 'e', 't', 'r', 'i', 'c', 's',  0 };
static const unichar_t str21[] = { 'W', 'i', 'n', 'd', 'o', 'w',  0 };
static const unichar_t str22[] = { 'H', 'e', 'l', 'p',  0 };
static const unichar_t str23[] = { 'R', 'e', 'c', 'e', 'n', 't',  0 };
static const unichar_t str24[] = { 'O', 'p', 'e', 'n', ' ', 'O', 'u', 't', 'l', 'i', 'n', 'e',  0 };
static const unichar_t str25[] = { 'O', 'p', 'e', 'n', ' ', 'B', 'i', 't', 'm', 'a', 'p',  0 };
static const unichar_t str26[] = { 'O', 'p', 'e', 'n', ' ', 'M', 'e', 't', 'r', 'i', 'c', 's',  0 };
static const unichar_t str27[] = { 'P', 'r', 'i', 'n', 't', '.', '.', '.',  0 };
static const unichar_t str28[] = { 'R', 'e', 'v', 'e', 'r', 't', ' ', 'F', 'i', 'l', 'e',  0 };
static const unichar_t str29[] = { 'S', 'a', 'v', 'e', ' ', 'a', 's', '.', '.', '.',  0 };
static const unichar_t str30[] = { 'G', 'e', 'n', 'e', 'r', 'a', 't', 'e', ' ', 'F', 'o', 'n', 't', 's', '.', '.', '.',  0 };
static const unichar_t str31[] = { 'I', 'm', 'p', 'o', 'r', 't', '.', '.', '.',  0 };
static const unichar_t str32[] = { 'C', 'l', 'o', 's', 'e',  0 };
static const unichar_t str33[] = { 'P', 'r', 'e', 'f', 'e', 'r', 'e', 'n', 'c', 'e', 's', '.', '.', '.',  0 };
static const unichar_t str34[] = { 'Q', 'u', 'i', 't',  0 };
static const unichar_t str35[] = { 'F', 'i', 't',  0 };
static const unichar_t str36[] = { 'Z', 'o', 'o', 'm', ' ', 'i', 'n',  0 };
static const unichar_t str37[] = { 'Z', 'o', 'o', 'm', ' ', 'o', 'u', 't',  0 };
static const unichar_t str38[] = { 'N', 'e', 'x', 't', ' ', 'C', 'h', 'a', 'r',  0 };
static const unichar_t str39[] = { 'P', 'r', 'e', 'v', ' ', 'C', 'h', 'a', 'r',  0 };
static const unichar_t str40[] = { 'G', 'o', 't', 'o',  0 };
static const unichar_t str41[] = { 'H', 'i', 'd', 'e', ' ', 'P', 'o', 'i', 'n', 't', 's',  0 };
static const unichar_t str42[] = { 'S', 'h', 'o', 'w', ' ', 'P', 'o', 'i', 'n', 't', 's',  0 };
static const unichar_t str43[] = { 'H', 'i', 'd', 'e', ' ', 'R', 'u', 'l', 'e', 'r', 's',  0 };
static const unichar_t str44[] = { 'S', 'h', 'o', 'w', ' ', 'R', 'u', 'l', 'e', 'r', 's',  0 };
static const unichar_t str45[] = { 'N', 'e', 'x', 't', ' ', 'P', 'o', 'i', 'n', 't',  0 };
static const unichar_t str46[] = { 'P', 'r', 'e', 'v', ' ', 'P', 'o', 'i', 'n', 't',  0 };
static const unichar_t str47[] = { 'F', 'i', 'l', 'l',  0 };
static const unichar_t str48[] = { 'U', 'n', 'd', 'o',  0 };
static const unichar_t str49[] = { 'R', 'e', 'd', 'o',  0 };
static const unichar_t str50[] = { 'C', 'u', 't',  0 };
static const unichar_t str51[] = { 'C', 'o', 'p', 'y',  0 };
static const unichar_t str52[] = { 'C', 'o', 'p', 'y', ' ', 'W', 'i', 'd', 't', 'h',  0 };
static const unichar_t str53[] = { 'C', 'o', 'p', 'y', ' ', 'R', 'e', 'f', 'e', 'r', 'e', 'n', 'c', 'e',  0 };
static const unichar_t str54[] = { 'P', 'a', 's', 't', 'e',  0 };
static const unichar_t str55[] = { 'C', 'l', 'e', 'a', 'r',  0 };
static const unichar_t str56[] = { 'M', 'e', 'r', 'g', 'e',  0 };
static const unichar_t str57[] = { 'S', 'e', 'l', 'e', 'c', 't', ' ', 'A', 'l', 'l',  0 };
static const unichar_t str58[] = { 'U', 'n', 'l', 'i', 'n', 'k', ' ', 'R', 'e', 'f', 'e', 'r', 'e', 'n', 'c', 'e',  0 };
static const unichar_t str59[] = { 'F', 'o', 'n', 't', ' ', 'I', 'n', 'f', 'o', '.', '.', '.',  0 };
static const unichar_t str60[] = { 'F', 'i', 'n', 'd', ' ', 'P', 'r', 'o', 'b', 'l', 'e', 'm', 's', '.', '.', '.',  0 };
static const unichar_t str61[] = { 'G', 'e', 't', ' ', 'I', 'n', 'f', 'o', '.', '.', '.',  0 };
static const unichar_t str62[] = { 'B', 'i', 't', 'm', 'a', 'p', 's', ' ', 'A', 'v', 'a', 'i', 'l', 'a', 'b', 'l', 'e', '.', '.', '.',  0 };
static const unichar_t str63[] = { 'R', 'e', 'g', 'e', 'n', 'e', 'r', 'a', 't', 'e', ' ', 'B', 'i', 't', 'm', 'a', 'p', 's', '.', '.', '.',  0 };
static const unichar_t str64[] = { 'A', 'u', 't', 'o', 't', 'r', 'a', 'c', 'e',  0 };
static const unichar_t str65[] = { 'T', 'r', 'a', 'n', 's', 'f', 'o', 'r', 'm', '.', '.', '.',  0 };
static const unichar_t str66[] = { 'E', 'x', 'p', 'a', 'n', 'd', ' ', 'S', 't', 'r', 'o', 'k', 'e', '.', '.', '.',  0 };
static const unichar_t str67[] = { 'R', 'e', 'm', 'o', 'v', 'e', ' ', 'O', 'v', 'e', 'r', 'l', 'a', 'p',  0 };
static const unichar_t str68[] = { 'S', 'i', 'm', 'p', 'l', 'i', 'f', 'y',  0 };
static const unichar_t str69[] = { 'C', 'l', 'e', 'a', 'n', 'u', 'p', ' ', 'C', 'h', 'a', 'r', 's',  0 };
static const unichar_t str70[] = { 'R', 'o', 'u', 'n', 'd', ' ', 't', 'o', ' ', 'i', 'n', 't',  0 };
static const unichar_t str71[] = { 'B', 'u', 'i', 'l', 'd', ' ', 'A', 'c', 'c', 'e', 'n', 't', 'e', 'd', ' ', 'C', 'h', 'a', 'r',  0 };
static const unichar_t str72[] = { 'B', 'u', 'i', 'l', 'd', ' ', 'C', 'o', 'm', 'p', 'o', 's', 'i', 't', 'e', ' ', 'C', 'h', 'a', 'r',  0 };
static const unichar_t str73[] = { 'C', 'l', 'o', 'c', 'k', 'w', 'i', 's', 'e',  0 };
static const unichar_t str74[] = { 'C', 'o', 'u', 'n', 't', 'e', 'r', ' ', 'C', 'l', 'o', 'c', 'k', 'w', 'i', 's', 'e',  0 };
static const unichar_t str75[] = { 'C', 'o', 'r', 'r', 'e', 'c', 't', ' ', 'D', 'i', 'r', 'e', 'c', 't', 'i', 'o', 'n',  0 };
static const unichar_t str76[] = { 'C', 'o', 'r', 'n', 'e', 'r',  0 };
static const unichar_t str77[] = { 'C', 'u', 'r', 'v', 'e',  0 };
static const unichar_t str78[] = { 'T', 'a', 'n', 'g', 'e', 'n', 't',  0 };
static const unichar_t str79[] = { 'A', 'u', 't', 'o', 'H', 'i', 'n', 't',  0 };
static const unichar_t str80[] = { 'F', 'u', 'l', 'l', ' ', 'A', 'u', 't', 'o', 'H', 'i', 'n', 't',  0 };
static const unichar_t str81[] = { 'C', 'l', 'e', 'a', 'r', ' ', 'H', 'S', 't', 'e', 'm',  0 };
static const unichar_t str82[] = { 'C', 'l', 'e', 'a', 'r', ' ', 'V', 'S', 't', 'e', 'm',  0 };
static const unichar_t str83[] = { 'C', 'l', 'e', 'a', 'r', ' ', 'D', 'S', 't', 'e', 'm',  0 };
static const unichar_t str84[] = { 'A', 'd', 'd', ' ', 'H', 'H', 'i', 'n', 't',  0 };
static const unichar_t str85[] = { 'A', 'd', 'd', ' ', 'V', 'H', 'i', 'n', 't',  0 };
static const unichar_t str86[] = { 'A', 'd', 'd', ' ', 'D', 'H', 'i', 'n', 't',  0 };
static const unichar_t str87[] = { 'C', 'r', 'e', 'a', 't', 'e', ' ', 'H', 'H', 'i', 'n', 't', '.', '.', '.',  0 };
static const unichar_t str88[] = { 'C', 'r', 'e', 'a', 't', 'e', ' ', 'V', 'H', 'i', 'n', 't', '.', '.', '.',  0 };
static const unichar_t str89[] = { 'R', 'e', 'v', 'i', 'e', 'w', ' ', 'H', 'i', 'n', 't', 's', '.', '.', '.',  0 };
static const unichar_t str90[] = { 'E', 'x', 'p', 'o', 'r', 't', '.', '.', '.',  0 };
static const unichar_t str91[] = { 'P', 'a', 'l', 'e', 't', 't', 'e', 's',  0 };
static const unichar_t str92[] = { 'T', 'o', 'o', 'l', 's',  0 };
static const unichar_t str93[] = { 'L', 'a', 'y', 'e', 'r', 's',  0 };
static const unichar_t str94[] = { 'C', 'e', 'n', 't', 'e', 'r', ' ', 'i', 'n', ' ', 'W', 'i', 'd', 't', 'h',  0 };
static const unichar_t str95[] = { 'T', 'h', 'i', 'r', 'd', 's', ' ', 'i', 'n', ' ', 'W', 'i', 'd', 't', 'h',  0 };
static const unichar_t str96[] = { 'S', 'e', 't', ' ', 'W', 'i', 'd', 't', 'h', '.', '.', '.',  0 };
static const unichar_t str97[] = { 'S', 'e', 't', ' ', 'L', 'B', 'e', 'a', 'r', 'i', 'n', 'g', '.', '.', '.',  0 };
static const unichar_t str98[] = { 'S', 'e', 't', ' ', 'R', 'B', 'e', 'a', 'r', 'i', 'n', 'g', '.', '.', '.',  0 };
static const unichar_t str99[] = { 'R', 'e', 'm', 'o', 'v', 'e', ' ', 'K', 'e', 'r', 'n', ' ', 'P', 'a', 'i', 'r', 's',  0 };
static const unichar_t str100[] = { 'M', 'e', 'r', 'g', 'e', ' ', 'K', 'e', 'r', 'n', ' ', 'I', 'n', 'f', 'o', '.', '.', '.',  0 };
static const unichar_t str101[] = { '2', '4', ' ', 'p', 'i', 'x', 'e', 'l', ' ', 'o', 'u', 't', 'l', 'i', 'n', 'e',  0 };
static const unichar_t str102[] = { '3', '6', ' ', 'p', 'i', 'x', 'e', 'l', ' ', 'o', 'u', 't', 'l', 'i', 'n', 'e',  0 };
static const unichar_t str103[] = { '4', '8', ' ', 'p', 'i', 'x', 'e', 'l', ' ', 'o', 'u', 't', 'l', 'i', 'n', 'e',  0 };
static const unichar_t str104[] = { 'A', 'n', 't', 'i', ' ', 'A', 'l', 'i', 'a', 's',  0 };
static const unichar_t str105[] = { 'C', 'h', 'a', 'r', ' ', 'I', 'n', 'f', 'o', '.', '.', '.',  0 };
static const unichar_t str106[] = { 'M', 'e', 'r', 'g', 'e', ' ', 'F', 'o', 'n', 't', 's', '.', '.', '.',  0 };
static const unichar_t str107[] = { 'I', 'n', 't', 'e', 'r', 'p', 'o', 'l', 'a', 't', 'e', ' ', 'F', 'o', 'n', 't', 's', '.', '.', '.',  0 };
static const unichar_t str108[] = { 'C', 'o', 'p', 'y', ' ', 'F', 'r', 'o', 'm',  0 };
static const unichar_t str109[] = { 'A', 'l', 'l', ' ', 'F', 'o', 'n', 't', 's',  0 };
static const unichar_t str110[] = { 'D', 'i', 's', 'p', 'l', 'a', 'y', 'e', 'd', ' ', 'F', 'o', 'n', 't',  0 };
static const unichar_t str111[] = { 'A', 'u', 't', 'o', ' ', 'K', 'e', 'r', 'n', '.', '.', '.',  0 };
static const unichar_t str112[] = { 'A', 'u', 't', 'o', ' ', 'W', 'i', 'd', 't', 'h', '.', '.', '.',  0 };
static const unichar_t str113[] = { 'R', 'e', 'm', 'o', 'v', 'e', ' ', 'A', 'l', 'l', ' ', 'K', 'e', 'r', 'n', ' ', 'P', 'a', 'i', 'r', 's',  0 };
static const unichar_t str114[] = { 'O', 'u', 't', 'l', 'i', 'n', 'e',  0 };
static const unichar_t str115[] = { 'S', 'h', 'o', 'w', ' ', 'G', 'r', 'i', 'd',  0 };
static const unichar_t str116[] = { 'H', 'i', 'd', 'e', ' ', 'G', 'r', 'i', 'd',  0 };
static const unichar_t str117[] = { 'B', 'i', 'g', 'g', 'e', 'r', ' ', 'P', 'i', 'x', 'e', 'l', ' ', 'S', 'i', 'z', 'e',  0 };
static const unichar_t str118[] = { 'S', 'm', 'a', 'l', 'l', 'e', 'r', ' ', 'P', 'i', 'x', 'e', 'l', ' ', 'S', 'i', 'z', 'e',  0 };
static const unichar_t str119[] = { 'F', 'l', 'i', 'p', ' ', 'H', 'o', 'r', 'i', 'z', 'o', 'n', 't', 'a', 'l', 'l', 'y',  0 };
static const unichar_t str120[] = { 'F', 'l', 'i', 'p', ' ', 'V', 'e', 'r', 't', 'i', 'c', 'a', 'l', 'l', 'y',  0 };
static const unichar_t str121[] = { 'R', 'o', 't', 'a', 't', 'e', ' ', '9', '0', 0xb0, ' ', 'C', 'W',  0 };
static const unichar_t str122[] = { 'R', 'o', 't', 'a', 't', 'e', ' ', '9', '0', 0xb0, ' ', 'C', 'C', 'W',  0 };
static const unichar_t str123[] = { 'R', 'o', 't', 'a', 't', 'e', ' ', '1', '8', '0', 0xb0,  0 };
static const unichar_t str124[] = { 'S', 'k', 'e', 'w', '.', '.', '.',  0 };
static const unichar_t str125[] = { 'M', 'a', 'k', 'e', ' ', 'F', 'i', 'r', 's', 't',  0 };
static const unichar_t str126[] = { 'C', 'u', 's', 't', 'o', 'm',  0 };
static const unichar_t str127[] = { 'I', 'S', 'O', ' ', '8', '8', '5', '9', '-', '1', ' ', ' ', '(', 'L', 'a', 't', 'i', 'n', '1', ')',  0 };
static const unichar_t str128[] = { 'I', 'S', 'O', ' ', '8', '8', '5', '9', '-', '1', '5', ' ', ' ', '(', 'L', 'a', 't', 'i', 'n', '0', ')',  0 };
static const unichar_t str129[] = { 'I', 'S', 'O', ' ', '8', '8', '5', '9', '-', '2', ' ', ' ', '(', 'L', 'a', 't', 'i', 'n', '2', ')',  0 };
static const unichar_t str130[] = { 'I', 'S', 'O', ' ', '8', '8', '5', '9', '-', '3', ' ', ' ', '(', 'L', 'a', 't', 'i', 'n', '3', ')',  0 };
static const unichar_t str131[] = { 'I', 'S', 'O', ' ', '8', '8', '5', '9', '-', '4', ' ', ' ', '(', 'L', 'a', 't', 'i', 'n', '4', ')',  0 };
static const unichar_t str132[] = { 'I', 'S', 'O', ' ', '8', '8', '5', '9', '-', '9', ' ', ' ', '(', 'L', 'a', 't', 'i', 'n', '5', ')',  0 };
static const unichar_t str133[] = { 'I', 'S', 'O', ' ', '8', '8', '5', '9', '-', '1', '0', ' ', ' ', '(', 'L', 'a', 't', 'i', 'n', '6', ')',  0 };
static const unichar_t str134[] = { 'I', 'S', 'O', ' ', '8', '8', '5', '9', '-', '1', '3', ' ', ' ', '(', 'L', 'a', 't', 'i', 'n', '7', ')',  0 };
static const unichar_t str135[] = { 'I', 'S', 'O', ' ', '8', '8', '5', '9', '-', '1', '4', ' ', ' ', '(', 'L', 'a', 't', 'i', 'n', '8', ')',  0 };
static const unichar_t str136[] = { 'I', 'S', 'O', ' ', '8', '8', '5', '9', '-', '5', ' ', '(', 'C', 'y', 'r', 'i', 'l', 'l', 'i', 'c', ')',  0 };
static const unichar_t str137[] = { 'K', 'O', 'I', '8', '-', 'R', ' ', '(', 'C', 'y', 'r', 'i', 'l', 'l', 'i', 'c', ')',  0 };
static const unichar_t str138[] = { 'I', 'S', 'O', ' ', '8', '8', '5', '9', '-', '6', ' ', '(', 'A', 'r', 'a', 'b', 'i', 'c', ')',  0 };
static const unichar_t str139[] = { 'I', 'S', 'O', ' ', '8', '8', '5', '9', '-', '7', ' ', '(', 'G', 'r', 'e', 'e', 'k', ')',  0 };
static const unichar_t str140[] = { 'I', 'S', 'O', ' ', '8', '8', '5', '9', '-', '8', ' ', '(', 'H', 'e', 'b', 'r', 'e', 'w', ')',  0 };
static const unichar_t str141[] = { 'I', 'S', 'O', ' ', '8', '8', '5', '9', '-', '1', '1', ' ', '(', 'T', 'h', 'a', 'i', ')',  0 };
static const unichar_t str142[] = { 'M', 'a', 'c', 'i', 'n', 't', 'o', 's', 'h', ' ', 'L', 'a', 't', 'i', 'n',  0 };
static const unichar_t str143[] = { 'W', 'i', 'n', 'd', 'o', 'w', 's', ' ', 'L', 'a', 't', 'i', 'n', ' ', '(', '"', 'A', 'N', 'S', 'I', '"', ')',  0 };
static const unichar_t str144[] = { 'A', 'd', 'o', 'b', 'e', ' ', 'S', 't', 'a', 'n', 'd', 'a', 'r', 'd',  0 };
static const unichar_t str145[] = { 'S', 'y', 'm', 'b', 'o', 'l',  0 };
static const unichar_t str146[] = { 0x3a4, 0x3b5, 0x3a7, ' ',  0 };
static const unichar_t str147[] = { 'I', 'S', 'O', ' ', '1', '0', '6', '4', '6', '-', '1', ' ', '(', 'U', 'n', 'i', 'c', 'o', 'd', 'e', ')',  0 };
static const unichar_t str148[] = { 'J', 'I', 'S', ' ', '2', '0', '8', ' ', '(', 'K', 'a', 'n', 'j', 'i', ')',  0 };
static const unichar_t str149[] = { 'J', 'I', 'S', ' ', '2', '1', '2', ' ', '(', 'K', 'a', 'n', 'j', 'i', ')',  0 };
static const unichar_t str150[] = { 'K', 'S', 'C', ' ', '5', '6', '0', '1', ' ', '(', 'K', 'o', 'r', 'e', 'a', 'n', ')',  0 };
static const unichar_t str151[] = { 'G', 'B', ' ', '2', '3', '1', '2', ' ', '(', 'C', 'h', 'i', 'n', 'e', 's', 'e', ')',  0 };
static const unichar_t str152[] = { 'Y', 'o', 'u', ' ', 'a', 'r', 'e', ' ', 'r', 'e', 'd', 'u', 'c', 'i', 'n', 'g', ' ', 't', 'h', 'e', ' ', 'n', 'u', 'm', 'b', 'e', 'r', ' ', 'o', 'f', ' ', 'c', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', 's', ' ', 'b', 'e', 'l', 'o', 'w', ' ', 't', 'h', 'e', 0xa, 'c', 'u', 'r', 'r', 'e', 'n', 't', ' ', 'n', 'u', 'm', 'b', 'e', 'r', '.', ' ', 'T', 'h', 'i', 's', ' ', 'w', 'i', 'l', 'l', ' ', 'd', 'e', 'l', 'e', 't', 'e', ' ', 's', 'o', 'm', 'e', ' ', 'c', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', 's', '.', 0xa, 'I', 's', ' ', 't', 'h', 'a', 't', ' ', 'w', 'h', 'a', 't', ' ', 'y', 'o', 'u', ' ', 'w', 'i', 's', 'h', ' ', 't', 'o', ' ', 'd', 'o', '?',  0 };
static const unichar_t str153[] = { 'T', 'o', 'o', ' ', 'F', 'e', 'w', ' ', 'C', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', 's',  0 };
static const unichar_t str154[] = { 'B', 'a', 'd', ' ', 'F', 'a', 'm', 'i', 'l', 'y', ' ', 'N', 'a', 'm', 'e', ',', ' ', 'm', 'u', 's', 't', ' ', 'b', 'e', 'g', 'i', 'n', ' ', 'w', 'i', 't', 'h', ' ', 'a', 'n', ' ', 'a', 'l', 'p', 'h', 'a', 'b', 'e', 't', 'i', 'c', ' ', 'c', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', '.',  0 };
static const unichar_t str155[] = { 'B', 'a', 'd', ' ', 'F', 'a', 'm', 'i', 'l', 'y', ' ', 'N', 'a', 'm', 'e',  0 };
static const unichar_t str156[] = { 'A', 's', 'c', 'e', 'n', 't', ' ', 'a', 'n', 'd', ' ', 'D', 'e', 's', 'c', 'e', 'n', 't', ' ', 'm', 'u', 's', 't', ' ', 'b', 'e', ' ', 'p', 'o', 's', 'i', 't', 'i', 'v', 'e', ' ', 'a', 'n', 'd', ' ', 't', 'h', 'e', 'i', 'r', ' ', 's', 'u', 'm', ' ', 'l', 'e', 's', 's', ' ', 't', 'h', 'a', 'n', ' ', '1', '6', '3', '8', '4',  0 };
static const unichar_t str157[] = { 'B', 'a', 'd', ' ', 'A', 's', 'c', 'e', 'n', 't', '/', 'D', 'e', 's', 'c', 'e', 'n', 't',  0 };
static const unichar_t str158[] = { 'F', 'o', 'n', 't', ' ', 'I', 'n', 'f', 'o', 'r', 'm', 'a', 't', 'i', 'o', 'n',  0 };
static const unichar_t str159[] = { 'F', 'a', 'm', 'i', 'l', 'y', ' ', 'N', 'a', 'm', 'e', ':',  0 };
static const unichar_t str160[] = { 'F', 'o', 'n', 't', ' ', 'M', 'o', 'd', 'i', 'f', 'i', 'e', 'r', 's', ':',  0 };
static const unichar_t str161[] = { 'N', 'a', 'm', 'e', ' ', 'F', 'o', 'r', ' ', 'H', 'u', 'm', 'a', 'n', 's', ':',  0 };
static const unichar_t str162[] = { 'E', 'n', 'c', 'o', 'd', 'i', 'n', 'g', ':',  0 };
static const unichar_t str163[] = { 'L', 'o', 'a', 'd',  0 };
static const unichar_t str164[] = { 'M', 'a', 'k', 'e', ' ', 'F', 'r', 'o', 'm', ' ', 'F', 'o', 'n', 't',  0 };
static const unichar_t str165[] = { 'R', 'e', 'm', 'o', 'v', 'e',  0 };
static const unichar_t str166[] = { 'I', 't', 'a', 'l', 'i', 'c', ' ', 'A', 'n', 'g', 'l', 'e', ':',  0 };
static const unichar_t str167[] = { 'U', 'n', 'd', 'e', 'r', 'l', 'i', 'n', 'e', ' ', 'P', 'o', 's', 'i', 't', 'i', 'o', 'n', ':',  0 };
static const unichar_t str168[] = { 'H', 'e', 'i', 'g', 'h', 't', ':',  0 };
static const unichar_t str169[] = { 'A', 's', 'c', 'e', 'n', 't', ':',  0 };
static const unichar_t str170[] = { 'D', 'e', 's', 'c', 'e', 'n', 't', ':',  0 };
static const unichar_t str171[] = { 'C', 'o', 'p', 'y', 'r', 'i', 'g', 'h', 't', ':',  0 };
static const unichar_t str172[] = { 'X', 'U', 'I', 'D', ':',  0 };
static const unichar_t str173[] = { 'N', 'u', 'm', 'b', 'e', 'r', ' ', 'o', 'f', ' ', 'C', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', 's', ':',  0 };
static const unichar_t str174[] = { 'G', 'u', 'e', 's', 's',  0 };
static const unichar_t str175[] = { 'A', ' ', 'F', 'o', 'n', 't', ' ', 'F', 'a', 'm', 'i', 'l', 'y', ' ', 'n', 'a', 'm', 'e', ' ', 'i', 's', ' ', 'r', 'e', 'q', 'u', 'i', 'r', 'e', 'd',  0 };
static const unichar_t str176[] = { 'A', ' ', 'P', 'o', 's', 't', 's', 'c', 'r', 'i', 'p', 't', ' ', 'n', 'a', 'm', 'e', ' ', 'm', 'a', 'y', ' ', 'n', 'o', 't', ' ', 'b', 'e', ' ', 'a', ' ', 'n', 'u', 'm', 'b', 'e', 'r',  0 };
static const unichar_t str177[] = { 'B', 'a', 'd', ' ', 'F', 'o', 'n', 't', ' ', 'F', 'a', 'm', 'i', 'l', 'y', ' ', 'N', 'a', 'm', 'e',  0 };
static const unichar_t str178[] = { 'B', 'a', 'd', ' ', 'F', 'o', 'n', 't', ' ', 'M', 'o', 'd', 'i', 'f', 'i', 'e', 'r', ' ', 'N', 'a', 'm', 'e',  0 };
static const unichar_t str179[] = { 'A', ' ', 'P', 'o', 's', 't', 's', 'c', 'r', 'i', 'p', 't', ' ', 'n', 'a', 'm', 'e', ' ', 's', 'h', 'o', 'u', 'l', 'd', ' ', 'b', 'e', ' ', 'A', 'S', 'C', 'I', 'I', 0xa, 'a', 'n', 'd', ' ', 'm', 'u', 's', 't', ' ', 'n', 'o', 't', ' ', 'c', 'o', 'n', 't', 'a', 'i', 'n', ' ', '(', ')', '{', '}', '[', ']', '<', '>', '%', '/', ' ', 'o', 'r', ' ', 's', 'p', 'a', 'c', 'e',  0 };
static const unichar_t str180[] = { 'N', 'a', 'm', 'e', 's',  0 };
static const unichar_t str181[] = { 'E', 'n', 'c', 'o', 'd', 'i', 'n', 'g',  0 };
static const unichar_t str182[] = { 'P', 'S', ' ', 'G', 'e', 'n', 'e', 'r', 'a', 'l',  0 };
static const unichar_t str183[] = { 'P', 'S', ' ', 'P', 'r', 'i', 'v', 'a', 't', 'e',  0 };
static const unichar_t str184[] = { 'T', 'T', 'F', ' ', 'N', 'a', 'm', 'e', 's',  0 };
static const unichar_t str185[] = { 'T', 'T', 'F', ' ', 'V', 'a', 'l', 'u', 'e', 's',  0 };
static const unichar_t str186[] = { 'P', 'a', 'n', 'o', 's', 'e',  0 };
static const unichar_t str187[] = { 'I', 'n', 'v', 'a', 'l', 'i', 'd', ' ', 'E', 'n', 'c', 'o', 'd', 'i', 'n', 'g',  0 };
static const unichar_t str188[] = { 'P', 'r', 'i', 'v', 'a', 't', 'e', ' ', 'K', 'e', 'y',  0 };
static const unichar_t str189[] = { 'K', 'e', 'y', ' ', '(', 'i', 'n', ' ', 'P', 'r', 'i', 'v', 'a', 't', 'e', ' ', 'd', 'i', 'c', 't', 'i', 'o', 'n', 'a', 'r', 'y', ')',  0 };
static const unichar_t str190[] = { 'A', 'd', 'd',  0 };
static const unichar_t str191[] = { 'U', 'l', 't', 'r', 'a', '-', 'C', 'o', 'n', 'd', 'e', 'n', 's', 'e', 'd', ' ', '(', '5', '0', '%', ')',  0 };
static const unichar_t str192[] = { 'E', 'x', 't', 'r', 'a', '-', 'C', 'o', 'n', 'd', 'e', 'n', 's', 'e', 'd', ' ', '(', '6', '2', '.', '5', '%', ')',  0 };
static const unichar_t str193[] = { 'C', 'o', 'n', 'd', 'e', 'n', 's', 'e', 'd', ' ', '(', '7', '5', '%', ')',  0 };
static const unichar_t str194[] = { 'S', 'e', 'm', 'i', '-', 'C', 'o', 'n', 'd', 'e', 'n', 's', 'e', 'd', ' ', '(', '8', '7', '.', '5', '%', ')',  0 };
static const unichar_t str195[] = { 'M', 'e', 'd', 'i', 'u', 'm', ' ', '(', '1', '0', '0', '%', ')',  0 };
static const unichar_t str196[] = { 'S', 'e', 'm', 'i', '-', 'E', 'x', 'p', 'a', 'n', 'd', 'e', 'd', ' ', '(', '1', '1', '2', '.', '5', '%', ')',  0 };
static const unichar_t str197[] = { 'E', 'x', 'p', 'a', 'n', 'd', 'e', 'd', ' ', '(', '1', '2', '5', '%', ')',  0 };
static const unichar_t str198[] = { 'E', 'x', 't', 'r', 'a', '-', 'E', 'x', 'p', 'a', 'n', 'd', 'e', 'd', ' ', '(', '1', '5', '0', '%', ')',  0 };
static const unichar_t str199[] = { 'U', 'l', 't', 'r', 'a', '-', 'E', 'x', 'p', 'a', 'n', 'd', 'e', 'd', ' ', '(', '2', '0', '0', '%', ')',  0 };
static const unichar_t str200[] = { '1', '0', '0', ' ', 'T', 'h', 'i', 'n',  0 };
static const unichar_t str201[] = { '2', '0', '0', ' ', 'E', 'x', 't', 'r', 'a', '-', 'L', 'i', 'g', 'h', 't',  0 };
static const unichar_t str202[] = { '3', '0', '0', ' ', 'L', 'i', 'g', 'h', 't',  0 };
static const unichar_t str203[] = { '4', '0', '0', ' ', 'B', 'o', 'o', 'k',  0 };
static const unichar_t str204[] = { '5', '0', '0', ' ', 'M', 'e', 'd', 'i', 'u', 'm',  0 };
static const unichar_t str205[] = { '6', '0', '0', ' ', 'D', 'e', 'm', 'i', '-', 'B', 'o', 'l', 'd',  0 };
static const unichar_t str206[] = { '7', '0', '0', ' ', 'B', 'o', 'l', 'd',  0 };
static const unichar_t str207[] = { '8', '0', '0', ' ', 'H', 'e', 'a', 'v', 'y',  0 };
static const unichar_t str208[] = { '9', '0', '0', ' ', 'B', 'l', 'a', 'c', 'k',  0 };
static const unichar_t str209[] = { 'S', 'e', 'r', 'i', 'f',  0 };
static const unichar_t str210[] = { 'S', 'a', 'n', 's', '-', 'S', 'e', 'r', 'i', 'f',  0 };
static const unichar_t str211[] = { 'M', 'o', 'n', 'o', 's', 'p', 'a', 'c', 'e',  0 };
static const unichar_t str212[] = { 'S', 'c', 'r', 'i', 'p', 't',  0 };
static const unichar_t str213[] = { 'D', 'e', 'c', 'o', 'r', 'a', 't', 'i', 'v', 'e',  0 };
static const unichar_t str214[] = { 'A', 'n', 'y',  0 };
static const unichar_t str215[] = { 'N', 'o', ' ', 'F', 'i', 't',  0 };
static const unichar_t str216[] = { 'T', 'e', 'x', 't', ' ', '&', ' ', 'D', 'i', 's', 'p', 'l', 'a', 'y',  0 };
static const unichar_t str217[] = { 'P', 'i', 'c', 't', 'o', 'r', 'a', 'l',  0 };
static const unichar_t str218[] = { 'C', 'o', 'v', 'e',  0 };
static const unichar_t str219[] = { 'O', 'b', 't', 'u', 's', 'e', ' ', 'C', 'o', 'v', 'e',  0 };
static const unichar_t str220[] = { 'S', 'q', 'u', 'a', 'r', 'e', ' ', 'C', 'o', 'v', 'e',  0 };
static const unichar_t str221[] = { 'O', 'b', 't', 'u', 's', 'e', ' ', 'S', 'q', 'u', 'a', 'r', 'e', ' ', 'C', 'o', 'v', 'e',  0 };
static const unichar_t str222[] = { 'S', 'q', 'u', 'a', 'r', 'e',  0 };
static const unichar_t str223[] = { 'T', 'h', 'i', 'n',  0 };
static const unichar_t str224[] = { 'B', 'o', 'n', 'e',  0 };
static const unichar_t str225[] = { 'E', 'x', 'a', 'g', 'g', 'e', 'r', 'a', 't', 'e', 'd',  0 };
static const unichar_t str226[] = { 'T', 'r', 'i', 'a', 'n', 'g', 'l', 'e',  0 };
static const unichar_t str227[] = { 'N', 'o', 'r', 'm', 'a', 'l', ' ', 'S', 'a', 'n', 's',  0 };
static const unichar_t str228[] = { 'O', 'b', 't', 'u', 's', 'e', ' ', 'S', 'a', 'n', 's',  0 };
static const unichar_t str229[] = { 'P', 'e', 'r', 'p', ' ', 'S', 'a', 'n', 's',  0 };
static const unichar_t str230[] = { 'F', 'l', 'a', 'r', 'e', 'd',  0 };
static const unichar_t str231[] = { 'R', 'o', 'u', 'n', 'd', 'e', 'd',  0 };
static const unichar_t str232[] = { 'V', 'e', 'r', 'y', ' ', 'L', 'i', 'g', 'h', 't',  0 };
static const unichar_t str233[] = { 'L', 'i', 'g', 'h', 't',  0 };
static const unichar_t str234[] = { 'B', 'o', 'o', 'k',  0 };
static const unichar_t str235[] = { 'M', 'e', 'd', 'i', 'u', 'm',  0 };
static const unichar_t str236[] = { 'D', 'e', 'm', 'i',  0 };
static const unichar_t str237[] = { 'B', 'o', 'l', 'd',  0 };
static const unichar_t str238[] = { 'H', 'e', 'a', 'v', 'y',  0 };
static const unichar_t str239[] = { 'B', 'l', 'a', 'c', 'k',  0 };
static const unichar_t str240[] = { 'N', 'o', 'r', 'd',  0 };
static const unichar_t str241[] = { 'O', 'l', 'd', ' ', 'S', 't', 'y', 'l', 'e',  0 };
static const unichar_t str242[] = { 'M', 'o', 'd', 'e', 'r', 'n',  0 };
static const unichar_t str243[] = { 'E', 'v', 'e', 'n', ' ', 'W', 'i', 'd', 't', 'h',  0 };
static const unichar_t str244[] = { 'E', 'x', 'p', 'a', 'n', 'd', 'e', 'd',  0 };
static const unichar_t str245[] = { 'C', 'o', 'n', 'd', 'e', 'n', 's', 'e', 'd',  0 };
static const unichar_t str246[] = { 'V', 'e', 'r', 'y', ' ', 'E', 'x', 'p', 'a', 'n', 'd', 'e', 'd',  0 };
static const unichar_t str247[] = { 'V', 'e', 'r', 'y', ' ', 'C', 'o', 'n', 'd', 'e', 'n', 's', 'e', 'd',  0 };
static const unichar_t str248[] = { 'M', 'o', 'n', 'o', 's', 'p', 'a', 'c', 'e', 'd',  0 };
static const unichar_t str249[] = { 'N', 'o', 'n', 'e',  0 };
static const unichar_t str250[] = { 'V', 'e', 'r', 'y', ' ', 'L', 'o', 'w',  0 };
static const unichar_t str251[] = { 'L', 'o', 'w',  0 };
static const unichar_t str252[] = { 'M', 'e', 'd', 'i', 'u', 'm', ' ', 'L', 'o', 'w',  0 };
static const unichar_t str253[] = { 'M', 'e', 'd', 'i', 'u', 'm', ' ', 'H', 'i', 'g', 'h',  0 };
static const unichar_t str254[] = { 'H', 'i', 'g', 'h',  0 };
static const unichar_t str255[] = { 'V', 'e', 'r', 'y', ' ', 'H', 'i', 'g', 'h',  0 };
static const unichar_t str256[] = { 'G', 'r', 'a', 'd', 'u', 'a', 'l', '/', 'D', 'i', 'a', 'g', 'o', 'n', 'a', 'l',  0 };
static const unichar_t str257[] = { 'G', 'r', 'a', 'd', 'u', 'a', 'l', '/', 'T', 'r', 'a', 'n', 's', 'i', 't', 'i', 'o', 'n', 'a', 'l',  0 };
static const unichar_t str258[] = { 'G', 'r', 'a', 'd', 'u', 'a', 'l', '/', 'V', 'e', 'r', 't', 'i', 'c', 'a', 'l',  0 };
static const unichar_t str259[] = { 'G', 'r', 'a', 'd', 'u', 'a', 'l', '/', 'H', 'o', 'r', 'i', 'z', 'o', 'n', 't', 'a', 'l',  0 };
static const unichar_t str260[] = { 'R', 'a', 'p', 'i', 'd', '/', 'V', 'e', 'r', 't', 'i', 'c', 'a', 'l',  0 };
static const unichar_t str261[] = { 'R', 'a', 'p', 'i', 'd', '/', 'H', 'o', 'r', 'i', 'z', 'o', 'n', 't', 'a', 'l',  0 };
static const unichar_t str262[] = { 'I', 'n', 's', 't', 'a', 'n', 't', '/', 'V', 'e', 'r', 't', 'i', 'c', 'a', 'l',  0 };
static const unichar_t str263[] = { 'N', 'o', 'r', 'm', 'a', 'l', '/', 'C', 'o', 'n', 't', 'a', 'c', 't',  0 };
static const unichar_t str264[] = { 'N', 'o', 'r', 'm', 'a', 'l', '/', 'W', 'e', 'i', 'g', 'h', 't', 'e', 'd',  0 };
static const unichar_t str265[] = { 'N', 'o', 'r', 'm', 'a', 'l', '/', 'B', 'o', 'x', 'e', 'd',  0 };
static const unichar_t str266[] = { 'N', 'o', 'r', 'm', 'a', 'l', '/', 'F', 'l', 'a', 't', 't', 'e', 'n', 'e', 'd',  0 };
static const unichar_t str267[] = { 'N', 'o', 'r', 'm', 'a', 'l', '/', 'R', 'o', 'u', 'n', 'd', 'e', 'd',  0 };
static const unichar_t str268[] = { 'N', 'o', 'r', 'm', 'a', 'l', '/', 'O', 'f', 'f', '-', 'C', 'e', 'n', 't', 'e', 'r',  0 };
static const unichar_t str269[] = { 'N', 'o', 'r', 'm', 'a', 'l', '/', 'S', 'q', 'u', 'a', 'r', 'e',  0 };
static const unichar_t str270[] = { 'O', 'b', 'l', 'i', 'q', 'u', 'e', '/', 'C', 'o', 'n', 't', 'a', 'c', 't',  0 };
static const unichar_t str271[] = { 'O', 'b', 'l', 'i', 'q', 'u', 'e', '/', 'W', 'e', 'i', 'g', 'h', 't', 'e', 'd',  0 };
static const unichar_t str272[] = { 'O', 'b', 'l', 'i', 'q', 'u', 'e', '/', 'B', 'o', 'x', 'e', 'd',  0 };
static const unichar_t str273[] = { 'O', 'b', 'l', 'i', 'q', 'u', 'e', '/', 'F', 'l', 'a', 't', 't', 'e', 'n', 'e', 'd',  0 };
static const unichar_t str274[] = { 'O', 'b', 'l', 'i', 'q', 'u', 'e', '/', 'R', 'o', 'u', 'n', 'd', 'e', 'd',  0 };
static const unichar_t str275[] = { 'O', 'b', 'l', 'i', 'q', 'u', 'e', '/', 'O', 'f', 'f', '-', 'C', 'e', 'n', 't', 'e', 'r',  0 };
static const unichar_t str276[] = { 'O', 'b', 'l', 'i', 'q', 'u', 'e', '/', 'S', 'q', 'u', 'a', 'r', 'e',  0 };
static const unichar_t str277[] = { 'S', 't', 'r', 'a', 'i', 'g', 'h', 't', ' ', 'A', 'r', 'm', 's', '/', 'H', 'o', 'r', 'i', 'z', 'o', 'n', 't', 'a', 'l',  0 };
static const unichar_t str278[] = { 'S', 't', 'r', 'a', 'i', 'g', 'h', 't', ' ', 'A', 'r', 'm', 's', '/', 'W', 'e', 'd', 'g', 'e',  0 };
static const unichar_t str279[] = { 'S', 't', 'r', 'a', 'i', 'g', 'h', 't', ' ', 'A', 'r', 'm', 's', '/', 'V', 'e', 'r', 't', 'i', 'c', 'a', 'l',  0 };
static const unichar_t str280[] = { 'S', 't', 'r', 'a', 'i', 'g', 'h', 't', ' ', 'A', 'r', 'm', 's', '/', 'S', 'i', 'n', 'g', 'l', 'e', ' ', 'S', 'e', 'r', 'i', 'f',  0 };
static const unichar_t str281[] = { 'S', 't', 'r', 'a', 'i', 'g', 'h', 't', ' ', 'A', 'r', 'm', 's', '/', 'D', 'o', 'u', 'b', 'l', 'e', ' ', 'S', 'e', 'r', 'i', 'f',  0 };
static const unichar_t str282[] = { 'N', 'o', 'n', '-', 'S', 't', 'r', 'a', 'i', 'g', 'h', 't', ' ', 'A', 'r', 'm', 's', '/', 'H', 'o', 'r', 'i', 'z', 'o', 'n', 't', 'a', 'l',  0 };
static const unichar_t str283[] = { 'N', 'o', 'n', '-', 'S', 't', 'r', 'a', 'i', 'g', 'h', 't', ' ', 'A', 'r', 'm', 's', '/', 'W', 'e', 'd', 'g', 'e',  0 };
static const unichar_t str284[] = { 'N', 'o', 'n', '-', 'S', 't', 'r', 'a', 'i', 'g', 'h', 't', ' ', 'A', 'r', 'm', 's', '/', 'V', 'e', 'r', 't', 'i', 'c', 'a', 'l',  0 };
static const unichar_t str285[] = { 'N', 'o', 'n', '-', 'S', 't', 'r', 'a', 'i', 'g', 'h', 't', ' ', 'A', 'r', 'm', 's', '/', 'S', 'i', 'n', 'g', 'l', 'e', ' ', 'S', 'e', 'r', 'i', 'f',  0 };
static const unichar_t str286[] = { 'N', 'o', 'n', '-', 'S', 't', 'r', 'a', 'i', 'g', 'h', 't', ' ', 'A', 'r', 'm', 's', '/', 'D', 'o', 'u', 'b', 'l', 'e', ' ', 'S', 'e', 'r', 'i', 'f',  0 };
static const unichar_t str287[] = { 'S', 't', 'a', 'n', 'd', 'a', 'r', 'd', '/', 'T', 'r', 'i', 'm', 'm', 'e', 'd',  0 };
static const unichar_t str288[] = { 'S', 't', 'a', 'n', 'd', 'a', 'r', 'd', '/', 'P', 'o', 'i', 'n', 't', 'e', 'd',  0 };
static const unichar_t str289[] = { 'S', 't', 'a', 'n', 'd', 'a', 'r', 'd', '/', 'S', 'e', 'r', 'i', 'f', 'e', 'd',  0 };
static const unichar_t str290[] = { 'H', 'i', 'g', 'h', '/', 'T', 'r', 'i', 'm', 'm', 'e', 'd',  0 };
static const unichar_t str291[] = { 'H', 'i', 'g', 'h', '/', 'P', 'o', 'i', 'n', 't', 'e', 'd',  0 };
static const unichar_t str292[] = { 'H', 'i', 'g', 'h', '/', 'S', 'e', 'r', 'i', 'f', 'e', 'd',  0 };
static const unichar_t str293[] = { 'C', 'o', 'n', 's', 't', 'a', 'n', 't', '/', 'T', 'r', 'i', 'm', 'm', 'e', 'd',  0 };
static const unichar_t str294[] = { 'C', 'o', 'n', 's', 't', 'a', 'n', 't', '/', 'P', 'o', 'i', 'n', 't', 'e', 'd',  0 };
static const unichar_t str295[] = { 'C', 'o', 'n', 's', 't', 'a', 'n', 't', '/', 'S', 'e', 'r', 'i', 'f', 'e', 'd',  0 };
static const unichar_t str296[] = { 'L', 'o', 'w', '/', 'T', 'r', 'i', 'm', 'm', 'e', 'd',  0 };
static const unichar_t str297[] = { 'L', 'o', 'w', '/', 'P', 'o', 'i', 'n', 't', 'e', 'd',  0 };
static const unichar_t str298[] = { 'L', 'o', 'w', '/', 'S', 'e', 'r', 'i', 'f', 'e', 'd',  0 };
static const unichar_t str299[] = { 'C', 'o', 'n', 's', 't', 'a', 'n', 't', '/', 'S', 'm', 'a', 'l', 'l',  0 };
static const unichar_t str300[] = { 'C', 'o', 'n', 's', 't', 'a', 'n', 't', '/', 'S', 't', 'a', 'n', 'd', 'a', 'r', 'd',  0 };
static const unichar_t str301[] = { 'C', 'o', 'n', 's', 't', 'a', 'n', 't', '/', 'L', 'a', 'r', 'g', 'e',  0 };
static const unichar_t str302[] = { 'D', 'u', 'c', 'k', 'i', 'n', 'g', '/', 'S', 'm', 'a', 'l', 'l',  0 };
static const unichar_t str303[] = { 'D', 'u', 'c', 'k', 'i', 'n', 'g', '/', 'S', 't', 'a', 'n', 'd', 'a', 'r', 'd',  0 };
static const unichar_t str304[] = { 'D', 'u', 'c', 'k', 'i', 'n', 'g', '/', 'L', 'a', 'r', 'g', 'e',  0 };
static const unichar_t str305[] = { 'W', 'i', 'd', 't', 'h', ' ', 'C', 'l', 'a', 's', 's',  0 };
static const unichar_t str306[] = { 'W', 'e', 'i', 'g', 'h', 't', ' ', 'C', 'l', 'a', 's', 's',  0 };
static const unichar_t str307[] = { 'P', 'F', 'M', ' ', 'F', 'a', 'm', 'i', 'l', 'y',  0 };
static const unichar_t str308[] = { 'F', 'a', 'm', 'i', 'l', 'y',  0 };
static const unichar_t str309[] = { 'S', 'e', 'r', 'i', 'f', 's',  0 };
static const unichar_t str310[] = { 'W', 'e', 'i', 'g', 'h', 't',  0 };
static const unichar_t str311[] = { 'P', 'r', 'o', 'p', 'o', 'r', 't', 'i', 'o', 'n',  0 };
static const unichar_t str312[] = { 'C', 'o', 'n', 't', 'r', 'a', 's', 't',  0 };
static const unichar_t str313[] = { 'S', 't', 'r', 'o', 'k', 'e', ' ', 'V', 'a', 'r', 'i', 'a', 't', 'i', 'o', 'n',  0 };
static const unichar_t str314[] = { 'A', 'r', 'm', ' ', 'S', 't', 'y', 'l', 'e',  0 };
static const unichar_t str315[] = { 'L', 'e', 't', 't', 'e', 'r', 'f', 'o', 'r', 'm',  0 };
static const unichar_t str316[] = { 'M', 'i', 'd', 'l', 'i', 'n', 'e',  0 };
static const unichar_t str317[] = { 'X', '-', 'H', 'e', 'i', 'g', 'h', 't',  0 };
static const unichar_t str318[] = { 'S', 't', 'y', 'l', 'e', 's',  0 };
static const unichar_t str319[] = { 'U', 'n', 'i', 'q', 'u', 'e', 'I', 'D',  0 };
static const unichar_t str320[] = { 'V', 'e', 'r', 's', 'i', 'o', 'n',  0 };
static const unichar_t str321[] = { 'F', 'u', 'l', 'l', 'n', 'a', 'm', 'e',  0 };
static const unichar_t str322[] = { 'T', 'r', 'a', 'd', 'e', 'm', 'a', 'r', 'k',  0 };
static const unichar_t str323[] = { 'M', 'a', 'n', 'u', 'f', 'a', 'c', 't', 'u', 'r', 'e', 'r',  0 };
static const unichar_t str324[] = { 'D', 'e', 's', 'i', 'g', 'n', 'e', 'r',  0 };
static const unichar_t str325[] = { 'D', 'e', 's', 'c', 'r', 'i', 'p', 't', 'o', 'r',  0 };
static const unichar_t str326[] = { 'V', 'e', 'n', 'd', 'e', 'r', ' ', 'U', 'R', 'L',  0 };
static const unichar_t str327[] = { 'D', 'e', 's', 'i', 'g', 'n', 'e', 'r', ' ', 'U', 'R', 'L',  0 };
static const unichar_t str328[] = { 'L', 'i', 'c', 'e', 'n', 's', 'e',  0 };
static const unichar_t str329[] = { 'L', 'i', 'c', 'e', 'n', 's', 'e', ' ', 'U', 'R', 'L',  0 };
static const unichar_t str330[] = { 'P', 'r', 'e', 'f', 'e', 'r', 'r', 'e', 'd', ' ', 'F', 'a', 'm', 'i', 'l', 'y',  0 };
static const unichar_t str331[] = { 'P', 'r', 'e', 'f', 'e', 'r', 'r', 'e', 'd', ' ', 'S', 't', 'y', 'l', 'e', 's',  0 };
static const unichar_t str332[] = { 'C', 'o', 'm', 'p', 'a', 't', 'a', 'b', 'l', 'e', ' ', 'F', 'u', 'l', 'l',  0 };
static const unichar_t str333[] = { 'S', 'a', 'm', 'p', 'l', 'e', ' ', 'T', 'e', 'x', 't',  0 };
static const unichar_t str334[] = { 'A', 'l', 'b', 'a', 'n', 'i', 'a', 'n', ' ', 's', 'q', '_', 'A', 'L',  0 };
static const unichar_t str335[] = { 'A', 'r', 'a', 'b', 'i', 'c', ' ', 'a', 'r',  0 };
static const unichar_t str336[] = { 'B', 'a', 's', 'q', 'u', 'e', ' ', 'e', 'u',  0 };
static const unichar_t str337[] = { 'B', 'y', 'e', 'l', 'o', 'r', 'u', 's', 's', 'i', 'a', 'n', ' ', 'b', 'e', '_', 'B', 'Y',  0 };
static const unichar_t str338[] = { 'B', 'u', 'l', 'g', 'a', 'r', 'i', 'a', 'n', ' ', 'b', 'g', '_', 'B', 'G',  0 };
static const unichar_t str339[] = { 'C', 'a', 't', 'a', 'l', 'a', 'n', ' ', 'c', 'a',  0 };
static const unichar_t str340[] = { 'C', 'h', 'i', 'n', 'e', 's', 'e', ' ', 'z', 'h', '_', 'C', 'N',  0 };
static const unichar_t str341[] = { 'C', 'r', 'o', 'a', 't', 'i', 'a', 'n', ' ', 'h', 'r',  0 };
static const unichar_t str342[] = { 'C', 'z', 'e', 'c', 'h', ' ', 'c', 's', '_', 'C', 'Z',  0 };
static const unichar_t str343[] = { 'D', 'a', 'n', 's', 'k', ' ', 'd', 'a', '_', 'D', 'K',  0 };
static const unichar_t str344[] = { 'N', 'e', 'd', 'e', 'r', 'l', 'a', 'n', 'd', 's', ' ', 'n', 'l', '_', 'N', 'L',  0 };
static const unichar_t str345[] = { 'N', 'e', 'd', 'e', 'r', 'l', 'a', 'n', 'd', 's', ' ', 'n', 'l', '_', 'B', 'E',  0 };
static const unichar_t str346[] = { 'B', 'r', 'i', 't', 'i', 's', 'h', ' ', 'E', 'n', 'g', 'l', 'i', 's', 'h', ' ', 'e', 'n', '_', 'U', 'K',  0 };
static const unichar_t str347[] = { 'A', 'm', 'e', 'r', 'i', 'c', 'a', 'n', ' ', 'E', 'n', 'g', 'l', 'i', 's', 'h', ' ', 'e', 'n', '_', 'U', 'S',  0 };
static const unichar_t str348[] = { 'C', 'a', 'n', 'a', 'd', 'i', 'a', 'n', ' ', 'E', 'n', 'g', 'l', 'i', 's', 'h', ' ', 'e', 'n', '_', 'C', 'A',  0 };
static const unichar_t str349[] = { 'A', 'u', 's', 't', 'r', 'a', 'l', 'i', 'a', 'n', ' ', 'E', 'n', 'g', 'l', 'i', 's', 'h', ' ', 'e', 'n', '_', 'A', 'U',  0 };
static const unichar_t str350[] = { 'N', 'e', 'w', ' ', 'Z', 'e', 'e', 'l', 'a', 'n', 'd', ' ', 'E', 'n', 'g', 'l', 'i', 's', 'h', ' ', 'e', 'n', '_', 'N', 'Z',  0 };
static const unichar_t str351[] = { 'I', 'r', 'i', 's', 'h', ' ', 'E', 'n', 'g', 'l', 'i', 's', 'h', ' ', 'e', 'n', '_', 'I', 'E',  0 };
static const unichar_t str352[] = { 'E', 's', 't', 'o', 'n', 'i', 'a', 'n', ' ', 'e', 't', '_', 'E', 'E',  0 };
static const unichar_t str353[] = { 'S', 'u', 'o', 'm', 'i', ' ', 'f', 'i', '_', 'F', 'I',  0 };
static const unichar_t str354[] = { 'F', 'r', 'a', 'n', 0xe7, 'a', 'i', 's', ' ', 'F', 'r', 'a', 'n', 0xe7, 'a', 'i', 's', ' ', 'f', 'r', '_', 'F', 'R',  0 };
static const unichar_t str355[] = { 'F', 'r', 'a', 'n', 0xe7, 'a', 'i', 's', ' ', 'B', 'e', 'l', 'g', 'i', 'q', 'u', 'e', ' ', 'f', 'r', '_', 'B', 'E',  0 };
static const unichar_t str356[] = { 'F', 'r', 'a', 'n', 0xe7, 'a', 'i', 's', ' ', 'C', 'a', 'n', 'a', 'd', 'i', 'e', 'n', ' ', 'f', 'r', '_', 'C', 'A',  0 };
static const unichar_t str357[] = { 'F', 'r', 'a', 'n', 0xe7, 'a', 'i', 's', ' ', 'S', 'u', 'i', 's', 's', ' ', 'f', 'r', '_', 'C', 'H',  0 };
static const unichar_t str358[] = { 'F', 'r', 'a', 'n', 0xe7, 'a', 'i', 's', ' ', 'd', 'e', ' ', 'L', 'u', 'x', 'e', 'm', 'b', 'o', 'u', 'r', 'g', ' ', 'f', 'r', '_', 'L', 'U',  0 };
static const unichar_t str359[] = { 'D', 'e', 'u', 't', 's', 'c', 'h', ' ', 'D', 'e', 'u', 't', 's', 'c', 'h', ' ', 'd', 'e', '_', 'D', 'E',  0 };
static const unichar_t str360[] = { 'S', 'c', 'h', 'w', 'e', 'i', 'z', 'e', 'r', ' ', 'D', 'e', 'u', 't', 's', 'c', 'h', ' ', 'd', 'e', '_', 'C', 'H',  0 };
static const unichar_t str361[] = { 0xd6, 's', 't', 'e', 'r', 'r', 'e', 'i', 'c', 'h', 'i', 's', 'c', 'h', ' ', 'D', 'e', 'u', 't', 's', 'c', 'h', ' ', 'd', 'e', '_', 'A', 'T',  0 };
static const unichar_t str362[] = { 'L', 'u', 'x', 'e', 'm', 'b', 'o', 'u', 'r', 'g', ' ', 'D', 'e', 'u', 't', 's', 'c', 'h', ' ', 'd', 'e', '_', 'L', 'U',  0 };
static const unichar_t str363[] = { 'L', 'i', 'e', 'c', 'h', 't', 'e', 'n', 's', 't', 'e', 'i', 'n', ' ', 'D', 'e', 'u', 't', 's', 'c', 'h', ' ', 'd', 'e', '_', 'L', 'I',  0 };
static const unichar_t str364[] = { 'G', 'r', 'e', 'e', 'k', ' ', 'e', 'l', '_', 'G', 'R',  0 };
static const unichar_t str365[] = { 'H', 'e', 'b', 'r', 'e', 'w', ' ', 'h', 'e', '_', 'I', 'L',  0 };
static const unichar_t str366[] = { 'H', 'u', 'n', 'g', 'a', 'r', 'i', 'a', 'n', ' ', 'h', 'u', '_', 'H', 'U',  0 };
static const unichar_t str367[] = { 'I', 'c', 'e', 'l', 'a', 'n', 'd', 'i', 'c', ' ', 'i', 's', '_', 'I', 'S',  0 };
static const unichar_t str368[] = { 'I', 't', 'a', 'l', 'i', 'a', 'n', 'o', ' ', 'i', 't', '_', 'I', 'T',  0 };
static const unichar_t str369[] = { 'I', 't', 'a', 'l', 'i', 'a', 'n', 'o', ' ', 'S', 'w', 'i', 's', 's', ' ', 'i', 't', '_', 'C', 'H',  0 };
static const unichar_t str370[] = { 'J', 'a', 'p', 'a', 'n', 'e', 's', 'e', ' ', 'j', 'p', '_', 'J', 'P',  0 };
static const unichar_t str371[] = { 'L', 'a', 't', 'v', 'i', 'a', 'n', ' ', 'l', 'v', '_', 'L', 'V',  0 };
static const unichar_t str372[] = { 'L', 'i', 't', 'h', 'u', 'a', 'n', 'i', 'a', 'n', ' ', 'l', 't', '_', 'L', 'T',  0 };
static const unichar_t str373[] = { 'N', 'o', 'r', 's', 'k', ' ', 'B', 'o', 'k', 'm', 'a', 'l', ' ', 'n', 'o', '_', 'N', 'O',  0 };
static const unichar_t str374[] = { 'N', 'o', 'r', 's', 'k', ' ', 'N', 'y', 'n', 'o', 'r', 's', 'k', ' ', 'n', 'o', '_', 'N', 'O',  0 };
static const unichar_t str375[] = { 'P', 'o', 'l', 'i', 's', 'h', ' ', 'p', 'l', '_', 'P', 'L',  0 };
static const unichar_t str376[] = { 'P', 'o', 'r', 't', 'u', 'g', 'u', 0xea, 's', ' ', 'P', 'o', 'r', 't', 'u', 'g', 'u', 0xea, 's', ' ', 'p', 't', '_', 'P', 'T',  0 };
static const unichar_t str377[] = { 'P', 'o', 'r', 't', 'u', 'g', 'u', 0xea, 's', ' ', 'B', 'r', 'a', 's', 'i', 'l', ' ', 'p', 't', '_', 'B', 'R',  0 };
static const unichar_t str378[] = { 'R', 'o', 'm', 'a', 'n', 'i', 'a', 'n', ' ', 'r', 'o', '_', 'R', 'O',  0 };
static const unichar_t str379[] = { 0x420, 0x443, 0x441, 0x441, 0x43a, 0x438, 0x439, ' ',  0 };
static const unichar_t str380[] = { 'S', 'l', 'o', 'v', 'a', 'k', ' ', 's', 'k', '_', 'S', 'K',  0 };
static const unichar_t str381[] = { 'S', 'l', 'o', 'v', 'e', 'n', 'i', 'a', 'n', ' ', 's', 'l', '_', 'S', 'I',  0 };
static const unichar_t str382[] = { 'E', 's', 'p', 'a', 0xf1, 'o', 'l', ' ', 'E', 's', 'p', 'a', 0xf1, 'o', 'l', ' ', 'e', 's', '_', 'E', 'S',  0 };
static const unichar_t str383[] = { 'E', 's', 'p', 'a', 0xf1, 'o', 'l', ' ', 'M', 0xe9, 'j', 'i', 'c', 'o', ' ', 'e', 's', '_', 'M', 'X',  0 };
static const unichar_t str384[] = { 'E', 's', 'p', 'a', 0xf1, 'o', 'l', ' ', 'E', 's', 'p', 'a', 0xf1, 'o', 'l', ' ', 'e', 's', '_', 'E', 'S',  0 };
static const unichar_t str385[] = { 'S', 'v', 'e', 'n', 's', 'k', ' ', 's', 'v', '_', 'S', 'E',  0 };
static const unichar_t str386[] = { 'T', 'u', 'r', 'k', 'i', 's', 'h', ' ', 't', 'r', '_', 'T', 'R',  0 };
static const unichar_t str387[] = { 'U', 'k', 'r', 'a', 'i', 'n', 'i', 'a', 'n', ' ', 'u', 'k', '_', 'U', 'A',  0 };
static const unichar_t str388[] = { 'F', 'o', 'r', 'm', 'a', 't', ':',  0 };
static const unichar_t str389[] = { 'S', 'a', 'v', 'e', ' ', 'F', 'a', 'i', 'l', 'e', 'd',  0 };
static const unichar_t str390[] = { 'O', 'u', 't', 'p', 'u', 't', ' ', 'A', 'F', 'M',  0 };
static const unichar_t str391[] = { 'O', 'u', 't', 'p', 'u', 't', ' ', 'P', 'F', 'M',  0 };
static const unichar_t str392[] = { 'N', 'o', ' ', 'O', 'u', 't', 'l', 'i', 'n', 'e', ' ', 'F', 'o', 'n', 't',  0 };
static const unichar_t str393[] = { 'N', 'o', ' ', 'B', 'i', 't', 'm', 'a', 'p', ' ', 'F', 'o', 'n', 't', 's',  0 };
static const unichar_t str394[] = { 'A', 'f', 'm', ' ', 'S', 'a', 'v', 'e', ' ', 'F', 'a', 'i', 'l', 'e', 'd',  0 };
static const unichar_t str395[] = { 'P', 'f', 'm', ' ', 'S', 'a', 'v', 'e', ' ', 'F', 'a', 'i', 'l', 'e', 'd',  0 };
static const unichar_t str396[] = { 'B', 'a', 'd', ' ', 'N', 'u', 'm', 'b', 'e', 'r', ' ', 'i', 'n', ' ',  0 };
static const unichar_t str397[] = { 'E', 'n', 't', 'e', 'r', ' ', 't', 'h', 'e', ' ', 'n', 'a', 'm', 'e', ' ', 'o', 'f', ' ', 'a', ' ', 'c', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', ' ', 'i', 'n', ' ', 't', 'h', 'e', ' ', 'f', 'o', 'n', 't',  0 };
static const unichar_t str398[] = { 'C', 'o', 'u', 'l', 'd', ' ', 'n', 'o', 't', ' ', 'f', 'i', 'n', 'd', ' ', 't', 'h', 'e', ' ', 'c', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', ':', ' ',  0 };
static const unichar_t str399[] = { 'D', 'o', 'n', 0x27, 't', ' ', 'S', 'a', 'v', 'e',  0 };
static const unichar_t str400[] = { 'F', 'o', 'n', 't', ' ', 'c', 'h', 'a', 'n', 'g', 'e', 'd',  0 };
static const unichar_t str401[] = { 'F', 'o', 'n', 't', ' ',  0 };
static const unichar_t str402[] = { ' ', 'i', 'n', ' ', 'f', 'i', 'l', 'e', ' ',  0 };
static const unichar_t str403[] = { ' ', 'h', 'a', 's', ' ', 'b', 'e', 'e', 'n', ' ', 'c', 'h', 'a', 'n', 'g', 'e', 'd', '.', 0xa, 'D', 'o', ' ', 'y', 'o', 'u', ' ', 'w', 'a', 'n', 't', ' ', 't', 'o', ' ', 's', 'a', 'v', 'e', ' ', 'i', 't', '?',  0 };
static const unichar_t str404[] = { ' ', 'h', 'a', 's', ' ', 'b', 'e', 'e', 'n', ' ', 'c', 'h', 'a', 'n', 'g', 'e', 'd', '.', 0xa, 'R', 'e', 'v', 'e', 'r', 't', 'i', 'n', 'g', ' ', 't', 'h', 'e', ' ', 'f', 'i', 'l', 'e', ' ', 'w', 'i', 'l', 'l', ' ', 'l', 'o', 's', 'e', ' ', 't', 'h', 'o', 's', 'e', ' ', 'c', 'h', 'a', 'n', 'g', 'e', 's', '.', 0xa, 'I', 's', ' ', 't', 'h', 'a', 't', ' ', 'w', 'h', 'a', 't', ' ', 'y', 'o', 'u', ' ', 'w', 'a', 'n', 't', '?',  0 };
static const unichar_t str405[] = { 'R', 'e', 'v', 'e', 'r', 't',  0 };
static const unichar_t str406[] = { 'M', 'a', 'n', 'y', ' ', 'W', 'i', 'n', 'd', 'o', 'w', 's',  0 };
static const unichar_t str407[] = { 'T', 'h', 'i', 's', ' ', 'i', 'n', 'v', 'o', 'l', 'v', 'e', 's', ' ', 'o', 'p', 'e', 'n', 'i', 'n', 'g', ' ', 'm', 'o', 'r', 'e', ' ', 't', 'h', 'a', 'n', ' ', '1', '0', ' ', 'w', 'i', 'n', 'd', 'o', 'w', 's', '.', 0xa, 'I', 's', ' ', 't', 'h', 'a', 't', ' ', 'r', 'e', 'a', 'l', 'l', 'y', ' ', 'w', 'h', 'a', 't', ' ', 'y', 'o', 'u', ' ', 'w', 'a', 'n', 't', '?',  0 };
static const unichar_t str408[] = { 'M', 'e', 'r', 'g', 'e', ' ', 'K', 'e', 'r', 'n', ' ', 'I', 'n', 'f', 'o',  0 };
static const unichar_t str409[] = { 'O', 'p', 'e', 'n', ' ', 'P', 'o', 's', 't', 's', 'c', 'r', 'i', 'p', 't', ' ', 'F', 'o', 'n', 't',  0 };
static const unichar_t str410[] = { 'L', 'o', 'a', 'd', 'i', 'n', 'g', '.', '.', '.',  0 };
static const unichar_t str411[] = { 'L', 'o', 'a', 'd', 'i', 'n', 'g', ' ', 'f', 'o', 'n', 't', ' ', 'f', 'r', 'o', 'm', ' ',  0 };
static const unichar_t str412[] = { 'R', 'e', 'a', 'd', 'i', 'n', 'g', ' ', 'G', 'l', 'y', 'p', 'h', 's',  0 };
static const unichar_t str413[] = { 'I', 'n', 't', 'e', 'r', 'p', 'r', 'e', 't', 'i', 'n', 'g', ' ', 'G', 'l', 'y', 'p', 'h', 's',  0 };
static const unichar_t str414[] = { 'C', 'o', 'u', 'l', 'd', 'n', 0x27, 't', ' ', 'o', 'p', 'e', 'n', ' ', 'f', 'o', 'n', 't', ' ', '(', 'o', 'r', ' ', 'i', 'n', ' ', 'b', 'a', 'd', ' ', 'f', 'o', 'r', 'm', 'a', 't', ')', ':', ' ',  0 };
static const unichar_t str415[] = { 'S', 'a', 'v', 'e', ' ', 'F', 'a', 'i', 'l', 'e', 'd',  0 };
static const unichar_t str416[] = { 'T', 'r', 'a', 'n', 's', 'f', 'o', 'r', 'm', 'i', 'n', 'g', '.', '.', '.',  0 };
static const unichar_t str417[] = { 'S', 'i', 'm', 'p', 'l', 'i', 'f', 'y', 'i', 'n', 'g', '.', '.', '.',  0 };
static const unichar_t str418[] = { 'R', 'e', 'm', 'o', 'v', 'i', 'n', 'g', ' ', 'o', 'v', 'e', 'r', 'l', 'a', 'p', 's', '.', '.', '.',  0 };
static const unichar_t str419[] = { 'C', 'o', 'r', 'r', 'e', 'c', 't', 'i', 'n', 'g', ' ', 'D', 'i', 'r', 'e', 'c', 't', 'i', 'o', 'n', '.', '.', '.',  0 };
static const unichar_t str420[] = { 'R', 'o', 'u', 'n', 'd', 'i', 'n', 'g', ' ', 't', 'o', ' ', 'i', 'n', 't', 'e', 'g', 'e', 'r', '.', '.', '.',  0 };
static const unichar_t str421[] = { 'B', 'u', 'i', 'l', 'd', 'i', 'n', 'g', ' ', 'a', 'c', 'c', 'e', 'n', 't', 'e', 'd', ' ', 'l', 'e', 't', 't', 'e', 'r', 's',  0 };
static const unichar_t str422[] = { 'R', 'e', 'p', 'l', 'a', 'c', 'e', ' ', 0xc5,  0 };
static const unichar_t str423[] = { 'A', 'r', 'e', ' ', 'y', 'o', 'u', ' ', 's', 'u', 'r', 'e', ' ', 'y', 'o', 'u', ' ', 'w', 'a', 'n', 't', ' ', 't', 'o', ' ', 'r', 'e', 'p', 'l', 'a', 'c', 'e', ' ', 0xc5, '?', 0xa, 'T', 'h', 'e', ' ', 'r', 'i', 'n', 'g', ' ', 'w', 'i', 'l', 'l', ' ', 'n', 'o', 't', ' ', 'j', 'o', 'i', 'n', ' ', 't', 'o', ' ', 't', 'h', 'e', ' ', 'A', '.',  0 };
static const unichar_t str424[] = { 'Y', 'e', 's',  0 };
static const unichar_t str425[] = { 'N', 'o',  0 };
static const unichar_t str426[] = { 'R', 'e', 'c', 't', 'a', 'n', 'g', 'l', 'e',  0 };
static const unichar_t str427[] = { 'F', 'i', 'l', 'l', 'e', 'd', ' ', 'R', 'e', 'c', 't', 'a', 'n', 'g', 'l', 'e',  0 };
static const unichar_t str428[] = { 'E', 'l', 'i', 'p', 's', 'e',  0 };
static const unichar_t str429[] = { 'F', 'i', 'l', 'l', 'e', 'd', ' ', 'E', 'l', 'i', 'p', 's', 'e',  0 };
static const unichar_t str430[] = { 'M', 'u', 'l', 't', 'i', 'p', 'l', 'e',  0 };
static const unichar_t str431[] = { 'T', 'h', 'e', 'r', 'e', ' ', 'i', 's', ' ', 'a', 'l', 'r', 'e', 'a', 'd', 'y', ' ', 'a', ' ', 'c', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', ' ', 'w', 'i', 't', 'h', ' ', 't', 'h', 'i', 's', ' ', 'U', 'n', 'i', 'c', 'o', 'd', 'e', ' ', 'e', 'n', 'c', 'o', 'd', 'i', 'n', 'g', ',', 0xa, '(', 'n', 'a', 'm', 'e', 'd', ' ',  0 };
static const unichar_t str432[] = { ' ', 'a', 't', ' ', 'l', 'o', 'c', 'a', 'l', ' ', 'e', 'n', 'c', 'o', 'd', 'i', 'n', 'g', ' ',  0 };
static const unichar_t str433[] = { ')', 0xa, 'I', 's', ' ', 't', 'h', 'a', 't', ' ', 'w', 'h', 'a', 't', ' ', 'y', 'o', 'u', ' ', 'w', 'a', 'n', 't', '?',  0 };
static const unichar_t str434[] = { 'T', 'h', 'e', 'r', 'e', ' ', 'i', 's', ' ', 'a', 'l', 'r', 'e', 'a', 'd', 'y', ' ', 'a', ' ', 'c', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', ' ', 'w', 'i', 't', 'h', ' ', 't', 'h', 'i', 's', ' ', 'n', 'a', 'm', 'e', ',', 0xa, 'd', 'o', ' ', 'y', 'o', 'u', ' ', 'w', 'a', 'n', 't', ' ', 't', 'o', ' ', 's', 'w', 'a', 'p', ' ', 'n', 'a', 'm', 'e', 's', '?',  0 };
static const unichar_t str435[] = { 'T', 'h', 'e', 'r', 'e', ' ', 'i', 's', ' ', 'a', 'l', 'r', 'e', 'a', 'd', 'y', ' ', 'a', ' ', 'l', 'i', 'g', 'a', 't', 'u', 'r', 'e', ' ', 'm', 'a', 'd', 'e', ' ', 'f', 'r', 'o', 'm', ' ', 't', 'h', 'e', 's', 'e', ' ', 'c', 'o', 'm', 'p', 'o', 'n', 'a', 'n', 't', 's', ',', 0xa, '(', 'n', 'a', 'm', 'e', 'd', ' ',  0 };
static const unichar_t str436[] = { ' ', 'a', 't', ' ', 'l', 'o', 'c', 'a', 'l', ' ', 'e', 'n', 'c', 'o', 'd', 'i', 'n', 'g', ' ',  0 };
static const unichar_t str437[] = { ')', 0xa, 'I', 's', ' ', 't', 'h', 'a', 't', ' ', 'w', 'h', 'a', 't', ' ', 'y', 'o', 'u', ' ', 'w', 'a', 'n', 't', '?',  0 };
static const unichar_t str438[] = { 'A', ' ', 'l', 'i', 'g', 'a', 't', 'u', 'r', 'e', ' ', 'm', 'a', 'y', ' ', 'n', 'o', 't', ' ', 'b', 'e', ' ', 'm', 'a', 'd', 'e', ' ', 'u', 'p', ' ', 'o', 'f', ' ', 'i', 't', 's', 'e', 'l', 'f',  0 };
static const unichar_t str439[] = { 'T', 'h', 'e', ' ', 'c', 'o', 'm', 'p', 'o', 'n', 'a', 'n', 't', ' ',  0 };
static const unichar_t str440[] = { ' ', 'i', 's', ' ', 'n', 'o', 't', ' ', 'i', 'n', ' ', 't', 'h', 'i', 's', ' ', 'f', 'o', 'n', 't', ',', 0xa, 'I', 's', ' ', 't', 'h', 'a', 't', ' ', 'w', 'h', 'a', 't', ' ', 'y', 'o', 'u', ' ', 'w', 'a', 'n', 't', '?',  0 };
static const unichar_t str441[] = { 'D', 'o', 'n', 'e',  0 };
static const unichar_t str442[] = { 'I', 'f', ' ', 't', 'h', 'i', 's', ' ', 'c', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', ' ', 'i', 's', ' ', 'a', ' ', 'l', 'i', 'g', 'a', 't', 'u', 'r', 'e', ',', 0xa, 't', 'h', 'e', 'n', ' ', 'e', 'n', 't', 'e', 'r', ' ', 't', 'h', 'e', ' ', 'n', 'a', 'm', 'e', 's', ' ', 'o', 'f', ' ', 't', 'h', 'e', ' ', 'c', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', 's', 0xa, 'i', 'n', 't', 'o', ' ', 'w', 'h', 'i', 'c', 'h', ' ', 'i', 't', ' ', 'd', 'e', 'c', 'o', 'm', 'p', 'o', 's', 'e', 's',  0 };
static const unichar_t str443[] = { 'S', 'h', 'o', 'w',  0 };
static const unichar_t str444[] = { 'D', 'u', 'p', 'l', 'i', 'c', 'a', 't', 'e', ' ', 'p', 'i', 'x', 'e', 'l', 's', 'i', 'z', 'e',  0 };
static const unichar_t str445[] = { 'T', 'h', 'e', ' ', 'f', 'o', 'n', 't', ' ', 'd', 'a', 't', 'a', 'b', 'a', 's', 'e', ' ', 'a', 'l', 'r', 'e', 'a', 'd', 'y', ' ', 'c', 'o', 'n', 't', 'a', 'i', 'n', 's', ' ', 'a', ' ', 'b', 'i', 't', 'm', 'a', 'p', 0xa, 'f', 'o', 'n', 't', ' ', 'w', 'i', 't', 'h', ' ', 't', 'h', 'i', 's', ' ', 'p', 'i', 'x', 'e', 'l', 's', 'i', 'z', 'e', ' ', '(',  0 };
static const unichar_t str446[] = { ')', 0xa, 'D', 'o', ' ', 'y', 'o', 'u', ' ', 'w', 'a', 'n', 't', ' ', 't', 'o', ' ', 'o', 'v', 'e', 'r', 'w', 'r', 'i', 't', 'e', ' ', 'i', 't', '?',  0 };
static const unichar_t str447[] = { 'T', 'h', 'i', 's', ' ', 'w', 'i', 'l', 'l', ' ', 'c', 'h', 'a', 'n', 'g', 'e', ' ', 'b', 'o', 't', 'h', ' ', 'B', 'l', 'u', 'e', 'V', 'a', 'l', 'u', 'e', 's', ' ', 'a', 'n', 'd', ' ', 'O', 't', 'h', 'e', 'r', 'B', 'l', 'u', 'e', 's', '.', 0xa, 'D', 'o', ' ', 'y', 'o', 'u', ' ', 'w', 'a', 'n', 't', ' ', 't', 'o', ' ', 'c', 'o', 'n', 't', 'i', 'n', 'u', 'e', '?',  0 };
static const unichar_t str448[] = { 'T', 'h', 'i', 's', ' ', 'w', 'i', 'l', 'l', ' ', 'c', 'h', 'a', 'n', 'g', 'e', ' ', 'b', 'o', 't', 'h', ' ', 'S', 't', 'd', 'H', 'W', ' ', 'a', 'n', 'd', ' ', 'S', 't', 'e', 'm', 'S', 'n', 'a', 'p', 'H', '.', 0xa, 'D', 'o', ' ', 'y', 'o', 'u', ' ', 'w', 'a', 'n', 't', ' ', 't', 'o', ' ', 'c', 'o', 'n', 't', 'i', 'n', 'u', 'e', '?',  0 };
static const unichar_t str449[] = { 'T', 'h', 'i', 's', ' ', 'w', 'i', 'l', 'l', ' ', 'c', 'h', 'a', 'n', 'g', 'e', ' ', 'b', 'o', 't', 'h', ' ', 'S', 't', 'd', 'V', 'W', ' ', 'a', 'n', 'd', ' ', 'S', 't', 'e', 'm', 'S', 'n', 'a', 'p', 'V', '.', 0xa, 'D', 'o', ' ', 'y', 'o', 'u', ' ', 'w', 'a', 'n', 't', ' ', 't', 'o', ' ', 'c', 'o', 'n', 't', 'i', 'n', 'u', 'e', '?',  0 };
static const unichar_t str450[] = { 'E', 'x', 'p', 'e', 'c', 't', 'e', 'd', ' ', 'a', 'r', 'r', 'a', 'y', 0xa, 'P', 'r', 'o', 'c', 'e', 'd', 'e', ' ', 'a', 'n', 'y', 'w', 'a', 'y', '?',  0 };
static const unichar_t str451[] = { 'E', 'x', 'p', 'e', 'c', 't', 'e', 'd', ' ', 'n', 'u', 'm', 'b', 'e', 'r', 0xa, 'P', 'r', 'o', 'c', 'e', 'd', 'e', ' ', 'a', 'n', 'y', 'w', 'a', 'y', '?',  0 };
static const unichar_t str452[] = { 'E', 'x', 'p', 'e', 'c', 't', 'e', 'd', ' ', 'b', 'o', 'o', 'l', 'e', 'a', 'n', 0xa, 'P', 'r', 'o', 'c', 'e', 'd', 'e', ' ', 'a', 'n', 'y', 'w', 'a', 'y', '?',  0 };
static const unichar_t str453[] = { 'E', 'x', 'p', 'e', 'c', 't', 'e', 'd', ' ', 'c', 'o', 'd', 'e', 0xa, 'P', 'r', 'o', 'c', 'e', 'd', 'e', ' ', 'a', 'n', 'y', 'w', 'a', 'y', '?',  0 };
static const unichar_t str454[] = { 'B', 'a', 'd', ' ', 't', 'y', 'p', 'e',  0 };
static const unichar_t str455[] = { 'D', 'e', 'l', 'e', 't', 'e',  0 };
static const unichar_t str456[] = { 'D', 'e', 'f', 'a', 'u', 'l', 't',  0 };
static const unichar_t str457[] = { 'B', 'a', 's', 'e', ':',  0 };
static const unichar_t str458[] = { 'S', 'i', 'z', 'e', ':',  0 };
static const unichar_t str459[] = { 'H', 'S', 't', 'e', 'm',  0 };
static const unichar_t str460[] = { 'V', 'S', 't', 'e', 'm',  0 };
static const unichar_t str461[] = { 'C', 'r', 'e', 'a', 't', 'e',  0 };
static const unichar_t str462[] = { '<', ' ', 'P', 'r', 'e', 'v',  0 };
static const unichar_t str463[] = { 'N', 'e', 'x', 't', ' ', '>',  0 };
static const unichar_t str464[] = { 'E', 'n', 't', 'e', 'r', ' ', 't', 'w', 'o', ' ', 'c', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', ' ', 'r', 'a', 'n', 'g', 'e', 's',  0 };
static const unichar_t str465[] = { 't', 'o', ' ', 'b', 'e', ' ', 'a', 'd', 'j', 'u', 's', 't', 'e', 'd', '.',  0 };
static const unichar_t str466[] = { 'C', 'h', 'a', 'r', 's', ' ', 'o', 'n', ' ', 'L', 'e', 'f', 't',  0 };
static const unichar_t str467[] = { 'C', 'h', 'a', 'r', 's', ' ', 'o', 'n', ' ', 'R', 'i', 'g', 'h', 't',  0 };
static const unichar_t str468[] = { 'A', 'l', 'l',  0 };
static const unichar_t str469[] = { 'A',  0 };
static const unichar_t str470[] = { 'S', 'e', 'l', 'e', 'c', 't', 'e', 'd',  0 };
static const unichar_t str471[] = { 'S', 'p', 'a', 'c', 'i', 'n', 'g',  0 };
static const unichar_t str472[] = { 'T', 'o', 't', 'a', 'l', ' ', 'K', 'e', 'r', 'n', 's', ':',  0 };
static const unichar_t str473[] = { 'T', 'h', 'r', 'e', 's', 'h', 'o', 'l', 'd', ':',  0 };
static const unichar_t str474[] = { 'N', 'o', ' ', 'c', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', 's', ' ', 's', 'e', 'l', 'e', 'c', 't', 'e', 'd', '.',  0 };
static const unichar_t str475[] = { 'N', 'o', 't', 'h', 'i', 'n', 'g', ' ', 't', 'o', ' ', 't', 'r', 'a', 'c', 'e',  0 };
static const unichar_t str476[] = { 'C', 'a', 'n', 0x27, 't', ' ', 'f', 'i', 'n', 'd', ' ', 'a', 'u', 't', 'o', 't', 'r', 'a', 'c', 'e',  0 };
static const unichar_t str477[] = { 'C', 'a', 'n', 0x27, 't', ' ', 'f', 'i', 'n', 'd', ' ', 'a', 'u', 't', 'o', 't', 'r', 'a', 'c', 'e', ' ', 'p', 'r', 'o', 'g', 'r', 'a', 'm', ' ', '(', 's', 'e', 't', ' ', 'A', 'U', 'T', 'O', 'T', 'R', 'A', 'C', 'E', ' ', 'e', 'n', 'v', 'i', 'r', 'o', 'n', 'm', 'e', 'n', 't', ' ', 'v', 'a', 'r', 'i', 'a', 'b', 'l', 'e', ')',  0 };
static const unichar_t str478[] = { 'A', 'l', 'l', ' ', 'C', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', 's',  0 };
static const unichar_t str479[] = { 'S', 'e', 'l', 'e', 'c', 't', 'e', 'd', ' ', 'C', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', 's',  0 };
static const unichar_t str480[] = { 'C', 'u', 'r', 'r', 'e', 'n', 't', ' ', 'C', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r',  0 };
static const unichar_t str481[] = { 'A', 't', 't', 'e', 'm', 'p', 't', ' ', 't', 'o', ' ', 'r', 'e', 'g', 'e', 'n', 'e', 'r', 'a', 't', 'e', ' ', 'a', ' ', 'p', 'i', 'x', 'e', 'l', ' ', 's', 'i', 'z', 'e', ' ', 't', 'h', 'a', 't', ' ', 'h', 'a', 's', ' ', 'n', 'o', 't', ' ', 'b', 'e', 'e', 'n', ' ', 'c', 'r', 'e', 'a', 't', 'e', 'd', ':', ' ',  0 };
static const unichar_t str482[] = { 'P', 'i', 'x', 'e', 'l', ' ', 'S', 'i', 'z', 'e', 's', ':',  0 };
static const unichar_t str483[] = { 'T', 'h', 'e', ' ', 'l', 'i', 's', 't', ' ', 'o', 'f', ' ', 'c', 'u', 'r', 'r', 'e', 'n', 't', ' ', 'p', 'i', 'x', 'e', 'l', ' ', 'b', 'i', 't', 'm', 'a', 'p', ' ', 's', 'i', 'z', 'e', 's',  0 };
static const unichar_t str484[] = { ' ', 'R', 'e', 'm', 'o', 'v', 'i', 'n', 'g', ' ', 'a', ' ', 's', 'i', 'z', 'e', ' ', 'w', 'i', 'l', 'l', ' ', 'd', 'e', 'l', 'e', 't', 'e', ' ', 'i', 't', '.',  0 };
static const unichar_t str485[] = { ' ', 'A', 'd', 'd', 'i', 'n', 'g', ' ', 'a', ' ', 's', 'i', 'z', 'e', ' ', 'w', 'i', 'l', 'l', ' ', 'c', 'r', 'e', 'a', 't', 'e', ' ', 'i', 't', '.',  0 };
static const unichar_t str486[] = { ' ', 'A', 'd', 'd', 'i', 'n', 'g', ' ', 'a', ' ', 's', 'i', 'z', 'e', ' ', 'w', 'i', 'l', 'l', ' ', 'c', 'r', 'e', 'a', 't', 'e', ' ', 'i', 't', ' ', 'b', 'y', ' ', 's', 'c', 'a', 'l', 'i', 'n', 'g', '.',  0 };
static const unichar_t str487[] = { 'S', 'p', 'e', 'c', 'i', 'f', 'y', ' ', 'b', 'i', 't', 'm', 'a', 'p', ' ', 's', 'i', 'z', 'e', 's', ' ', 't', 'o', ' ', 'b', 'e', ' ', 'r', 'e', 'g', 'e', 'n', 'e', 'r', 'a', 't', 'e', 'd',  0 };
static const unichar_t str488[] = { 'P', 'o', 'i', 'n', 't', ' ', 's', 'i', 'z', 'e', 's', ' ', 'o', 'n', ' ', 'a', ' ', '7', '5', ' ', 'd', 'p', 'i', ' ', 's', 'c', 'r', 'e', 'e', 'n',  0 };
static const unichar_t str489[] = { 'P', 'o', 'i', 'n', 't', ' ', 's', 'i', 'z', 'e', 's', ' ', 'o', 'n', ' ', 'a', ' ', '1', '0', '0', ' ', 'd', 'p', 'i', ' ', 's', 'c', 'r', 'e', 'e', 'n',  0 };
static const unichar_t str490[] = { ' ', 'a', 't', ' ',  0 };
static const unichar_t str491[] = { ' ', 'f', 'r', 'o', 'm', ' ',  0 };
static const unichar_t str492[] = { 'R', 'e', 'c', 'a', 'l', 'c', 'u', 'l', 'a', 't', 'e', ' ', 'B', 'i', 't', 'm', 'a', 'p', 's',  0 };
static const unichar_t str493[] = { 'E', 'n', 't', 'e', 'r', ' ', 't', 'h', 'e', ' ', 'r', 'a', 't', 'i', 'o', ' ', 'o', 'f', ' ', 'x', '-', 's', 'k', 'e', 'w', ' ', 't', 'o', ' ', 'y', ' ', 'r', 'i', 's', 'e',  0 };
static const unichar_t str494[] = { 'B', 'a', 'd', ' ', 'N', 'u', 'm', 'b', 'e', 'r',  0 };
static const unichar_t str495[] = { ' ', 'f', 'r', 'o', 'm', ' ',  0 };
static const unichar_t str496[] = { 'P', 'i', 'x', 'e', 'l', ' ', 's', 'i', 'z', 'e', '?',  0 };
static const unichar_t str497[] = { 'P', 'i', 'x', 'e', 'l', ' ', 's', 'i', 'z', 'e', ':',  0 };
static const unichar_t str498[] = { 'B', 'i', 't', 's', '/', 'P', 'i', 'x', 'e', 'l', ':',  0 };
static const unichar_t str499[] = { 'T', 'h', 'e', ' ', 'o', 'n', 'l', 'y', ' ', 'v', 'a', 'l', 'i', 'd', ' ', 'v', 'a', 'l', 'u', 'e', 's', ' ', 'f', 'o', 'r', ' ', 'b', 'i', 't', 's', '/', 'p', 'i', 'x', 'e', 'l', ' ', 'a', 'r', 'e', ' ', '1', ',', ' ', '2', ',', ' ', '4', ' ', 'o', 'r', ' ', '8',  0 };
static const unichar_t str500[] = { 'C', 'a', 'n', 0x27, 't', ' ', 'f', 'i', 'n', 'd', ' ', 't', 'h', 'e', ' ', 'f', 'i', 'l', 'e',  0 };
static const unichar_t str501[] = { 'B', 'a', 'd', ' ', 'x', 'f', 'i', 'g', ' ', 'f', 'i', 'l', 'e',  0 };
static const unichar_t str502[] = { 'B', 'a', 'd', ' ', 'i', 'm', 'a', 'g', 'e', ' ', 'f', 'i', 'l', 'e',  0 };
static const unichar_t str503[] = { 'B', 'a', 'd', ' ', 'i', 'm', 'a', 'g', 'e', ' ', 'f', 'i', 'l', 'e', ',', ' ', 'n', 'o', 't', ' ', 'a', ' ', 'b', 'i', 't', 'm', 'a', 'p',  0 };
static const unichar_t str504[] = { 'I', 'm', 'a', 'g', 'e',  0 };
static const unichar_t str505[] = { 'S', 'i', 'z', 'e', ' ', 'o', 'f', ' ', 'P', 'o', 'i', 'n', 't', 's',  0 };
static const unichar_t str506[] = { 'R', 'e', 'g', 'u', 'l', 'a', 'r',  0 };
static const unichar_t str507[] = { 'P', 'o', 'i', 'n', 't', 's', ':',  0 };
static const unichar_t str508[] = { 'R', 'o', 'u', 'n', 'd', ' ', 'R', 'e', 'c', 't', 'a', 'n', 'g', 'l', 'e', ' ', 'R', 'a', 'd', 'i', 'u', 's',  0 };
static const unichar_t str509[] = { 'P', 'o', 'l', 'y', 'g', 'o', 'n',  0 };
static const unichar_t str510[] = { 'S', 't', 'a', 'r',  0 };
static const unichar_t str511[] = { 'N', 'u', 'm', 'b', 'e', 'r', ' ', 'o', 'f', ' ', 's', 't', 'a', 'r', ' ', 'p', 'o', 'i', 'n', 't', 's', '/', 'P', 'o', 'l', 'y', 'g', 'o', 'n', ' ', 'v', 'e', 'r', 't', 'e', 'c', 'e', 's',  0 };
static const unichar_t str512[] = { 'V',  0 };
static const unichar_t str513[] = { 'E',  0 };
static const unichar_t str514[] = { 'L', 'a', 'y', 'e', 'r',  0 };
static const unichar_t str515[] = { 'I', 's', ' ', 'L', 'a', 'y', 'e', 'r', ' ', 'E', 'd', 'i', 't', 'a', 'b', 'l', 'e', '?',  0 };
static const unichar_t str516[] = { 'I', 's', ' ', 'L', 'a', 'y', 'e', 'r', ' ', 'V', 'i', 's', 'i', 'b', 'l', 'e', '?',  0 };
static const unichar_t str517[] = { 'F', 'o', 'r', 'e',  0 };
static const unichar_t str518[] = { 'B', 'a', 'c', 'k',  0 };
static const unichar_t str519[] = { 'G', 'u', 'i', 'd', 'e',  0 };
static const unichar_t str520[] = { 'H', 'H', 'i', 'n', 't', 's',  0 };
static const unichar_t str521[] = { 'V', 'H', 'i', 'n', 't', 's',  0 };
static const unichar_t str522[] = { 'D', 'H', 'i', 'n', 't', 's',  0 };
static const unichar_t str523[] = { 'B', 'i', 't', 'm', 'a', 'p',  0 };
static const unichar_t str524[] = { 'S', 'h', 'a', 'p', 'e', ' ', 'T', 'y', 'p', 'e',  0 };
static const unichar_t str525[] = { 'P', 'o', 'i', 'n', 't', 'e', 'r',  0 };
static const unichar_t str526[] = { 'M', 'a', 'g', 'n', 'i', 'f', 'y', ' ', '(', 'M', 'i', 'n', 'i', 'f', 'y', ' ', 'w', 'i', 't', 'h', ' ', 'a', 'l', 't', ')',  0 };
static const unichar_t str527[] = { 'A', 'd', 'd', ' ', 'a', ' ', 'c', 'u', 'r', 'v', 'e', ' ', 'p', 'o', 'i', 'n', 't',  0 };
static const unichar_t str528[] = { 'A', 'd', 'd', ' ', 'a', ' ', 'c', 'o', 'r', 'n', 'e', 'r', ' ', 'p', 'o', 'i', 'n', 't',  0 };
static const unichar_t str529[] = { 'A', 'd', 'd', ' ', 'a', ' ', 't', 'a', 'n', 'g', 'e', 'n', 't', ' ', 'p', 'o', 'i', 'n', 't',  0 };
static const unichar_t str530[] = { 'A', 'd', 'd', ' ', 'a', ' ', 'p', 'o', 'i', 'n', 't', ',', ' ', 't', 'h', 'e', 'n', ' ', 'd', 'r', 'a', 'g', ' ', 'o', 'u', 't', ' ', 'i', 't', 's', ' ', 'c', 'o', 'n', 't', 'r', 'o', 'l', ' ', 'p', 'o', 'i', 'n', 't', 's',  0 };
static const unichar_t str531[] = { 'C', 'u', 't', ' ', 's', 'p', 'l', 'i', 'n', 'e', 's', ' ', 'i', 'n', ' ', 't', 'w', 'o',  0 };
static const unichar_t str532[] = { 'M', 'e', 'a', 's', 'u', 'r', 'e', ' ', 'd', 'i', 's', 't', 'a', 'n', 'c', 'e', ',', ' ', 'a', 'n', 'g', 'l', 'e', ' ', 'b', 'e', 't', 'w', 'e', 'e', 'n', ' ', 'p', 'o', 'i', 'n', 't', 's',  0 };
static const unichar_t str533[] = { 'S', 'c', 'a', 'l', 'e', ' ', 't', 'h', 'e', ' ', 's', 'e', 'l', 'e', 'c', 't', 'i', 'o', 'n',  0 };
static const unichar_t str534[] = { 'F', 'l', 'i', 'p', ' ', 't', 'h', 'e', ' ', 's', 'e', 'l', 'e', 'c', 't', 'i', 'o', 'n',  0 };
static const unichar_t str535[] = { 'R', 'o', 't', 'a', 't', 'e', ' ', 't', 'h', 'e', ' ', 's', 'e', 'l', 'e', 'c', 't', 'i', 'o', 'n',  0 };
static const unichar_t str536[] = { 'S', 'k', 'e', 'w', ' ', 't', 'h', 'e', ' ', 's', 'e', 'l', 'e', 'c', 't', 'i', 'o', 'n',  0 };
static const unichar_t str537[] = { 'R', 'e', 'c', 't', 'a', 'n', 'g', 'l', 'e', ' ', 'o', 'r', ' ', 'E', 'l', 'i', 'p', 's', 'e',  0 };
static const unichar_t str538[] = { 'P', 'o', 'l', 'y', 'g', 'o', 'n', ' ', 'o', 'r', ' ', 'S', 't', 'a', 'r',  0 };
static const unichar_t str539[] = { 'S', 'e', 't', '/', 'C', 'l', 'e', 'a', 'r', ' ', 'P', 'i', 'x', 'e', 'l', 's',  0 };
static const unichar_t str540[] = { 'D', 'r', 'a', 'w', ' ', 'a', ' ', 'L', 'i', 'n', 'e',  0 };
static const unichar_t str541[] = { 'S', 'h', 'i', 'f', 't', ' ', 'E', 'n', 't', 'i', 'r', 'e', ' ', 'B', 'i', 't', 'm', 'a', 'p',  0 };
static const unichar_t str542[] = { 'S', 'c', 'r', 'o', 'l', 'l', ' ', 'B', 'i', 't', 'm', 'a', 'p',  0 };
static const unichar_t str543[] = { 'B', 'i', 't', 'm', 'a', 'p', ' ', 'P', 'a', 's', 't', 'e',  0 };
static const unichar_t str544[] = { 'Y', 'e', 's', ' ', 't', 'o', ' ', 'A', 'l', 'l',  0 };
static const unichar_t str545[] = { 'T', 'h', 'e', ' ', 'c', 'l', 'i', 'p', 'b', 'o', 'a', 'r', 'd', ' ', 'c', 'o', 'n', 't', 'a', 'i', 'n', 's', ' ', 'a', ' ', 'b', 'i', 't', 'm', 'a', 'p', ' ', 'c', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', ' ', 'o', 'f', ' ', 's', 'i', 'z', 'e', ' ',  0 };
static const unichar_t str546[] = { ',', 0xa, 'a', ' ', 's', 'i', 'z', 'e', ' ', 'w', 'h', 'i', 'c', 'h', ' ', 'i', 's', ' ', 'n', 'o', 't', ' ', 'i', 'n', ' ', 'y', 'o', 'u', 'r', ' ', 'd', 'a', 't', 'a', 'b', 'a', 's', 'e', '.', 0xa, 'W', 'o', 'u', 'l', 'd', ' ', 'y', 'o', 'u', ' ', 'l', 'i', 'k', 'e', ' ', 't', 'o', ' ', 'c', 'r', 'e', 'a', 't', 'e', ' ', 'a', ' ', 'b', 'i', 't', 'm', 'a', 'p', ' ', 'f', 'o', 'n', 't', ' ', 'o', 'f', ' ', 't', 'h', 'a', 't', ' ', 's', 'i', 'z', 'e', ',', 0xa, 'o', 'r', ' ', 'i', 'g', 'n', 'o', 'r', 'e', ' ', 't', 'h', 'i', 's', ' ', 'c', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', '?',  0 };
static const unichar_t str547[] = { 'P', 'a', 's', 't', 'i', 'n', 'g', '.', '.', '.',  0 };
static const unichar_t str548[] = { 'S', 'e', 'l', 'f', '-', 'r', 'e', 'f', 'e', 'r', 'e', 'n', 't', 'i', 'a', 'l', ' ', 'c', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r',  0 };
static const unichar_t str549[] = { 'A', 't', 't', 'e', 'm', 'p', 't', ' ', 't', 'o', ' ', 'm', 'a', 'k', 'e', ' ', 'a', ' ', 'c', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', ' ', 't', 'h', 'a', 't', ' ', 'r', 'e', 'f', 'e', 'r', 's', ' ', 't', 'o', ' ', 'i', 't', 's', 'e', 'l', 'f',  0 };
static const unichar_t str550[] = { 'C', 'o', 'n', 'v', 'e', 'r', 't', 'i', 'n', 'g', ' ', 'P', 'o', 's', 't', 's', 'c', 'r', 'i', 'p', 't',  0 };
static const unichar_t str551[] = { 'S', 'a', 'v', 'i', 'n', 'g', ' ', 'P', 'o', 's', 't', 's', 'c', 'r', 'i', 'p', 't', ' ', 'F', 'o', 'n', 't',  0 };
static const unichar_t str552[] = { 'B', 'a', 'd', ' ', 'e', 'n', 'c', 'o', 'd', 'i', 'n', 'g', ' ', 'f', 'i', 'l', 'e', ' ', 'f', 'o', 'r', 'm', 'a', 't',  0 };
static const unichar_t str553[] = { 'P', 'l', 'e', 'a', 's', 'e', ' ', 'n', 'a', 'm', 'e', ' ', 't', 'h', 'i', 's', ' ', 'e', 'n', 'c', 'o', 'd', 'i', 'n', 'g',  0 };
static const unichar_t str554[] = { 'P', 'l', 'e', 'a', 's', 'e', ' ', 'n', 'a', 'm', 'e', ' ', 't', 'h', 'e', ' ',  0 };
static const unichar_t str555[] = { ' ', 'e', 'n', 'c', 'o', 'd', 'i', 'n', 'g', ' ', 'i', 'n', ' ', 't', 'h', 'i', 's', ' ', 'f', 'i', 'l', 'e',  0 };
static const unichar_t str556[] = { 'F', 'i', 'r', 's', 't',  0 };
static const unichar_t str557[] = { 'S', 'e', 'c', 'o', 'n', 'd',  0 };
static const unichar_t str558[] = { 'T', 'h', 'i', 'r', 'd',  0 };
static const unichar_t str559[] = { 't', 'h',  0 };
static const unichar_t str560[] = { 'R', 'e', 'm', 'o', 'v', 'e', ' ', 'E', 'n', 'c', 'o', 'd', 'i', 'n', 'g',  0 };
static const unichar_t str561[] = { 'L', 'o', 'a', 'd', ' ', 'E', 'n', 'c', 'o', 'd', 'i', 'n', 'g',  0 };
static const unichar_t str562[] = { 'T', 'h', 'e', ' ', 's', 'e', 'l', 'e', 'c', 't', 'e', 'd', ' ', 'p', 'a', 't', 'h', ' ', 'd', 'o', 'e', 's', ' ', 'n', 'o', 't', ' ', 'f', 'o', 'r', 'm', ' ', 'a', ' ', 'c', 'l', 'o', 's', 'e', 'd', ' ', 'l', 'o', 'o', 'p',  0 };
static const unichar_t str563[] = { 'T', 'h', 'e', ' ', 's', 'e', 'l', 'e', 'c', 't', 'e', 'd', ' ', 'p', 'o', 'i', 'n', 't', 's', ' ', 'a', 'r', 'e', ' ', 't', 'o', 'o', ' ', 'c', 'l', 'o', 's', 'e', ' ', 't', 'o', ' ', 'e', 'a', 'c', 'h', ' ', 'o', 't', 'h', 'e', 'r',  0 };
static const unichar_t str564[] = { 'T', 'h', 'e', ' ', 's', 'e', 'l', 'e', 'c', 't', 'e', 'd', ' ', 's', 'p', 'l', 'i', 'n', 'e', ' ', 'd', 'o', 'e', 's', ' ', 'n', 'o', 't', ' ', 'h', 'a', 'v', 'e', ' ', 'a', ' ', 'p', 'o', 'i', 'n', 't', ' ', 'a', 't', ' ', 'i', 't', 's', ' ', 'e', 'x', 't', 'r', 'e', 'm', 'u', 'm', '(', 'a', ')',  0 };
static const unichar_t str565[] = { 'T', 'h', 'e', ' ', 'x', ' ', 'c', 'o', 'o', 'r', 'd', ' ', 'o', 'f', ' ', 't', 'h', 'e', ' ', 's', 'e', 'l', 'e', 'c', 't', 'e', 'd', ' ', 'p', 'o', 'i', 'n', 't', ' ', 'i', 's', ' ', 'n', 'e', 'a', 'r', ' ', 't', 'h', 'e', ' ', 's', 'p', 'e', 'c', 'i', 'f', 'i', 'e', 'd', ' ', 'v', 'a', 'l', 'u', 'e',  0 };
static const unichar_t str566[] = { 'T', 'h', 'e', ' ', 'y', ' ', 'c', 'o', 'o', 'r', 'd', ' ', 'o', 'f', ' ', 't', 'h', 'e', ' ', 's', 'e', 'l', 'e', 'c', 't', 'e', 'd', ' ', 'p', 'o', 'i', 'n', 't', ' ', 'i', 's', ' ', 'n', 'e', 'a', 'r', ' ', 't', 'h', 'e', ' ', 's', 'p', 'e', 'c', 'i', 'f', 'i', 'e', 'd', ' ', 'v', 'a', 'l', 'u', 'e',  0 };
static const unichar_t str567[] = { 'T', 'h', 'e', ' ', 'y', ' ', 'c', 'o', 'o', 'r', 'd', ' ', 'o', 'f', ' ', 't', 'h', 'e', ' ', 's', 'e', 'l', 'e', 'c', 't', 'e', 'd', ' ', 'p', 'o', 'i', 'n', 't', ' ', 'i', 's', ' ', 'n', 'e', 'a', 'r', ' ', 't', 'h', 'e', ' ', 'b', 'a', 's', 'e', 'l', 'i', 'n', 'e',  0 };
static const unichar_t str568[] = { 'T', 'h', 'e', ' ', 'y', ' ', 'c', 'o', 'o', 'r', 'd', ' ', 'o', 'f', ' ', 't', 'h', 'e', ' ', 's', 'e', 'l', 'e', 'c', 't', 'e', 'd', ' ', 'p', 'o', 'i', 'n', 't', ' ', 'i', 's', ' ', 'n', 'e', 'a', 'r', ' ', 't', 'h', 'e', ' ', 'x', 'h', 'e', 'i', 'g', 'h', 't',  0 };
static const unichar_t str569[] = { 'T', 'h', 'e', ' ', 'y', ' ', 'c', 'o', 'o', 'r', 'd', ' ', 'o', 'f', ' ', 't', 'h', 'e', ' ', 's', 'e', 'l', 'e', 'c', 't', 'e', 'd', ' ', 'p', 'o', 'i', 'n', 't', ' ', 'i', 's', ' ', 'n', 'e', 'a', 'r', ' ', 't', 'h', 'e', ' ', 'c', 'a', 'p', ' ', 'h', 'e', 'i', 'g', 'h', 't',  0 };
static const unichar_t str570[] = { 'T', 'h', 'e', ' ', 'y', ' ', 'c', 'o', 'o', 'r', 'd', ' ', 'o', 'f', ' ', 't', 'h', 'e', ' ', 's', 'e', 'l', 'e', 'c', 't', 'e', 'd', ' ', 'p', 'o', 'i', 'n', 't', ' ', 'i', 's', ' ', 'n', 'e', 'a', 'r', ' ', 't', 'h', 'e', ' ', 'a', 's', 'c', 'e', 'n', 'd', 'e', 'r', ' ', 'h', 'e', 'i', 'g', 'h', 't',  0 };
static const unichar_t str571[] = { 'T', 'h', 'e', ' ', 'y', ' ', 'c', 'o', 'o', 'r', 'd', ' ', 'o', 'f', ' ', 't', 'h', 'e', ' ', 's', 'e', 'l', 'e', 'c', 't', 'e', 'd', ' ', 'p', 'o', 'i', 'n', 't', ' ', 'i', 's', ' ', 'n', 'e', 'a', 'r', ' ', 't', 'h', 'e', ' ', 'd', 'e', 's', 'c', 'e', 'n', 'd', 'e', 'r', ' ', 'h', 'e', 'i', 'g', 'h', 't',  0 };
static const unichar_t str572[] = { 'T', 'h', 'e', ' ', 's', 'e', 'l', 'e', 'c', 't', 'e', 'd', ' ', 'l', 'i', 'n', 'e', ' ', 's', 'e', 'g', 'm', 'e', 'n', 't', ' ', 'i', 's', ' ', 'n', 'e', 'a', 'r', 'l', 'y', ' ', 'h', 'o', 'r', 'i', 'z', 'o', 'n', 't', 'a', 'l',  0 };
static const unichar_t str573[] = { 'T', 'h', 'e', ' ', 's', 'e', 'l', 'e', 'c', 't', 'e', 'd', ' ', 'l', 'i', 'n', 'e', ' ', 's', 'e', 'g', 'm', 'e', 'n', 't', ' ', 'i', 's', ' ', 'n', 'e', 'a', 'r', 'l', 'y', ' ', 'v', 'e', 'r', 't', 'i', 'c', 'a', 'l',  0 };
static const unichar_t str574[] = { 'T', 'h', 'e', ' ', 's', 'e', 'l', 'e', 'c', 't', 'e', 'd', ' ', 'l', 'i', 'n', 'e', ' ', 's', 'e', 'g', 'm', 'e', 'n', 't', ' ', 'i', 's', ' ', 'n', 'e', 'a', 'r', ' ', 't', 'h', 'e', ' ', 'i', 't', 'a', 'l', 'i', 'c', ' ', 'a', 'n', 'g', 'l', 'e',  0 };
static const unichar_t str575[] = { 'T', 'h', 'e', ' ', 's', 'e', 'l', 'e', 'c', 't', 'e', 'd', ' ', 'p', 'o', 'i', 'n', 't', ' ', 'i', 's', ' ', 'n', 'e', 'a', 'r', ' ', 'a', ' ', 'h', 'o', 'r', 'i', 'z', 'o', 'n', 't', 'a', 'l', ' ', 's', 't', 'e', 'm', ' ', 'h', 'i', 'n', 't',  0 };
static const unichar_t str576[] = { 'T', 'h', 'e', ' ', 's', 'e', 'l', 'e', 'c', 't', 'e', 'd', ' ', 'p', 'o', 'i', 'n', 't', ' ', 'i', 's', ' ', 'n', 'e', 'a', 'r', ' ', 'a', ' ', 'v', 'e', 'r', 't', 'i', 'c', 'a', 'l', ' ', 's', 't', 'e', 'm', ' ', 'h', 'i', 'n', 't',  0 };
static const unichar_t str577[] = { 'T', 'h', 'i', 's', ' ', 'c', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', ' ', 'c', 'o', 'n', 't', 'a', 'i', 'n', 's', ' ', 'a', ' ', 'h', 'o', 'r', 'i', 'z', 'o', 'n', 't', 'a', 'l', ' ', 'h', 'i', 'n', 't', ' ', 'n', 'e', 'a', 'r', ' ', 't', 'h', 'e', ' ', 's', 'p', 'e', 'c', 'i', 'f', 'i', 'e', 'd', ' ', 'w', 'i', 'd', 't', 'h',  0 };
static const unichar_t str578[] = { 'T', 'h', 'i', 's', ' ', 'c', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', ' ', 'c', 'o', 'n', 't', 'a', 'i', 'n', 's', ' ', 'a', ' ', 'v', 'e', 'r', 't', 'i', 'c', 'a', 'l', ' ', 'h', 'i', 'n', 't', ' ', 'n', 'e', 'a', 'r', ' ', 't', 'h', 'e', ' ', 's', 'p', 'e', 'c', 'i', 'f', 'i', 'e', 'd', ' ', 'w', 'i', 'd', 't', 'h',  0 };
static const unichar_t str579[] = { 'T', 'h', 'i', 's', ' ', 'h', 'i', 'n', 't', ' ', 'd', 'o', 'e', 's', ' ', 'n', 'o', 't', ' ', 'c', 'o', 'n', 't', 'r', 'o', 'l', ' ', 'a', 'n', 'y', ' ', 'p', 'o', 'i', 'n', 't', 's',  0 };
static const unichar_t str580[] = { 'T', 'h', 'i', 's', ' ', 'p', 'a', 't', 'h', ' ', 's', 'h', 'o', 'u', 'l', 'd', ' ', 'h', 'a', 'v', 'e', ' ', 'b', 'e', 'e', 'n', ' ', 'd', 'r', 'a', 'w', 'n', ' ', 'i', 'n', ' ', 'a', ' ', 'c', 'o', 'u', 'n', 't', 'e', 'r', '-', 'c', 'l', 'o', 'c', 'k', 'w', 'i', 's', 'e', ' ', 'd', 'i', 'r', 'e', 'c', 't', 'i', 'o', 'n',  0 };
static const unichar_t str581[] = { 'T', 'h', 'i', 's', ' ', 'p', 'a', 't', 'h', ' ', 's', 'h', 'o', 'u', 'l', 'd', ' ', 'h', 'a', 'v', 'e', ' ', 'b', 'e', 'e', 'n', ' ', 'd', 'r', 'a', 'w', 'n', ' ', 'i', 'n', ' ', 'a', ' ', 'c', 'l', 'o', 'c', 'k', 'w', 'i', 's', 'e', ' ', 'd', 'i', 'r', 'e', 'c', 't', 'i', 'o', 'n',  0 };
static const unichar_t str582[] = { 'P', 'r', 'o', 'b', 'l', 'e', 'm', ' ', 'e', 'x', 'p', 'l', 'a', 'n', 'a', 't', 'i', 'o', 'n',  0 };
static const unichar_t str583[] = { 'S', 't', 'o', 'p',  0 };
static const unichar_t str584[] = { 'N', 'e', 'x', 't',  0 };
static const unichar_t str585[] = { 'O', 'p', 'e', 'n', ' ', 'P', 'a', 't', 'h', 's',  0 };
static const unichar_t str586[] = { 'A', 'l', 'l', ' ', 'p', 'a', 't', 'h', 's', ' ', 's', 'h', 'o', 'u', 'l', 'd', ' ', 'b', 'e', ' ', 'c', 'l', 'o', 's', 'e', 'd', ' ', 'l', 'o', 'o', 'p', 's', ',', ' ', 't', 'h', 'e', 'r', 'e', ' ', 's', 'h', 'o', 'u', 'l', 'd', ' ', 'b', 'e', ' ', 'n', 'o', ' ', 'e', 'x', 'p', 'o', 's', 'e', 'd', ' ', 'e', 'n', 'd', 'p', 'o', 'i', 'n', 't', 's',  0 };
static const unichar_t str587[] = { 'P', 'o', 'i', 'n', 't', 's', ' ', 't', 'o', 'o', ' ', 'c', 'l', 'o', 's', 'e',  0 };
static const unichar_t str588[] = { 'I', 'f', ' ', 't', 'w', 'o', ' ', 'a', 'd', 'j', 'a', 'c', 'e', 'n', 't', ' ', 'p', 'o', 'i', 'n', 't', 's', ' ', 'o', 'n', ' ', 't', 'h', 'e', ' ', 's', 'a', 'm', 'e', ' ', 'p', 'a', 't', 'h', ' ', 'a', 'r', 'e', ' ', 'l', 'e', 's', 's', ' ', 't', 'h', 'a', 'n', ' ', 'a', ' ', 'f', 'e', 'w', 0xa, 'e', 'm', 'u', 'n', 'i', 't', 's', ' ', 'a', 'p', 'a', 'r', 't', ' ', 't', 'h', 'e', 'y', ' ', 'w', 'i', 'l', 'l', ' ', 'c', 'a', 'u', 's', 'e', ' ', 'p', 'r', 'o', 'b', 'l', 'e', 'm', 's', ' ', 'f', 'o', 'r', ' ', 's', 'o', 'm', 'e', ' ', 'o', 'f', ' ', 'p', 'f', 'a', 'e', 'd', 'i', 't', 0x27, 's', 0xa, 'c', 'o', 'm', 'm', 'a', 'n', 'd', 's', '.', ' ', 'P', 'o', 's', 't', 'S', 'c', 'r', 'i', 'p', 't', ' ', 's', 'h', 'o', 'u', 'l', 'd', 'n', 0x27, 't', ' ', 'c', 'a', 'r', 'e', ' ', 't', 'h', 'o', 'u', 'g', 'h', '.',  0 };
static const unichar_t str589[] = { 'X', ' ', 'n', 'e', 'a', 'r',  0 };
static const unichar_t str590[] = { 'A', 'l', 'l', 'o', 'w', 's', ' ', 'y', 'o', 'u', ' ', 't', 'o', ' ', 'c', 'h', 'e', 'c', 'k', ' ', 't', 'h', 'a', 't', ' ', 'v', 'e', 'r', 't', 'i', 'c', 'a', 'l', ' ', 's', 't', 'e', 'm', 's', ' ', 'i', 'n', ' ', 's', 'e', 'v', 'e', 'r', 'a', 'l', 0xa, 'c', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', 's', ' ', 's', 't', 'a', 'r', 't', ' ', 'a', 't', ' ', 't', 'h', 'e', ' ', 's', 'a', 'm', 'e', ' ', 'l', 'o', 'c', 'a', 't', 'i', 'o', 'n', '.',  0 };
static const unichar_t str591[] = { 'Y', ' ', 'n', 'e', 'a', 'r',  0 };
static const unichar_t str592[] = { 'A', 'l', 'l', 'o', 'w', 's', ' ', 'y', 'o', 'u', ' ', 't', 'o', ' ', 'c', 'h', 'e', 'c', 'k', ' ', 't', 'h', 'a', 't', ' ', 'h', 'o', 'r', 'i', 'z', 'o', 'n', 't', 'a', 'l', ' ', 's', 't', 'e', 'm', 's', ' ', 'i', 'n', ' ', 's', 'e', 'v', 'e', 'r', 'a', 'l', 0xa, 'c', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', 's', ' ', 's', 't', 'a', 'r', 't', ' ', 'a', 't', ' ', 't', 'h', 'e', ' ', 's', 'a', 'm', 'e', ' ', 'l', 'o', 'c', 'a', 't', 'i', 'o', 'n', '.',  0 };
static const unichar_t str593[] = { 'Y', ' ', 'n', 'e', 'a', 'r', ' ', 's', 't', 'a', 'n', 'd', 'a', 'r', 'd', ' ', 'h', 'e', 'i', 'g', 'h', 't', 's',  0 };
static const unichar_t str594[] = { 'A', 'l', 'l', 'o', 'w', 's', ' ', 'y', 'o', 'u', ' ', 't', 'o', ' ', 'f', 'i', 'n', 'd', ' ', 'p', 'o', 'i', 'n', 't', 's', ' ', 'w', 'h', 'i', 'c', 'h', ' ', 'a', 'r', 'e', ' ', 's', 'l', 'i', 'g', 'h', 't', 'l', 'y', 0xa, 'o', 'f', 'f', ' ', 'f', 'r', 'o', 'm', ' ', 't', 'h', 'e', ' ', 'b', 'a', 's', 'e', 'l', 'i', 'n', 'e', ',', ' ', 'x', 'h', 'e', 'i', 'g', 'h', 't', ',', 'c', 'a', 'p', ' ', 'h', 'e', 'i', 'g', 'h', 't', ',', 0xa, 'a', 's', 'c', 'e', 'n', 'd', 'e', 'r', ',', ' ', 'd', 'e', 's', 'c', 'e', 'n', 'd', 'e', 'r', ' ', 'h', 'e', 'i', 'g', 'h', 't', 's', '.',  0 };
static const unichar_t str595[] = { 'E', 'd', 'g', 'e', 's', ' ', 'n', 'e', 'a', 'r', ' ', 'h', 'o', 'r', 'i', 'z', 'o', 'n', 't', 'a', 'l', '/', 'v', 'e', 'r', 't', 'i', 'c', 'a', 'l',  0 };
static const unichar_t str596[] = { 'E', 'd', 'g', 'e', 's', ' ', 'n', 'e', 'a', 'r', ' ', 'h', 'o', 'r', 'i', 'z', 'o', 'n', 't', 'a', 'l', '/', 'v', 'e', 'r', 't', 'i', 'c', 'a', 'l', '/', 'i', 't', 'a', 'l', 'i', 'c',  0 };
static const unichar_t str597[] = { 'A', 'l', 'l', 'o', 'w', 's', ' ', 'y', 'o', 'u', ' ', 't', 'o', ' ', 'f', 'i', 'n', 'd', ' ', 'l', 'i', 'n', 'e', 's', ' ', 'w', 'h', 'i', 'c', 'h', ' ', 'a', 'r', 'e', ' ', 'a', 'l', 'm', 'o', 's', 't', ',', 0xa, 'b', 'u', 't', ' ', 'n', 'o', 't', ' ', 'q', 'u', 'i', 't', 'e', ' ', 'h', 'o', 'r', 'i', 'z', 'o', 'n', 't', 'a', 'l', ' ', 'o', 'r', ' ', 'v', 'e', 'r', 't', 'i', 'c', 'a', 'l', 0xa, '(', 'o', 'r', ' ', 'a', 't', ' ', 't', 'h', 'e', ' ', 'i', 't', 'a', 'l', 'i', 'c', ' ', 'a', 'n', 'g', 'l', 'e', ')', '.',  0 };
static const unichar_t str598[] = { 'H', 'i', 'n', 't', 's', ' ', 'c', 'o', 'n', 't', 'r', 'o', 'l', 'l', 'i', 'n', 'g', ' ', 'n', 'o', ' ', 'p', 'o', 'i', 'n', 't', 's',  0 };
static const unichar_t str599[] = { 'G', 'h', 'o', 's', 't', 'v', 'i', 'e', 'w', ' ', '(', 'p', 'e', 'r', 'h', 'a', 'p', 's', ' ', 'o', 't', 'h', 'e', 'r', ' ', 'i', 'n', 't', 'e', 'r', 'p', 'r', 'e', 't', 'e', 'r', 's', ')', ' ', 'h', 'a', 's', ' ', 'a', ' ', 'p', 'r', 'o', 'b', 'l', 'e', 'm', ' ', 'w', 'h', 'e', 'n', ' ', 'a', 0xa, 'h', 'i', 'n', 't', ' ', 'e', 'x', 'i', 's', 't', 's', ' ', 'w', 'i', 't', 'h', 'o', 'u', 't', ' ', 'a', 'n', 'y', ' ', 'p', 'o', 'i', 'n', 't', 's', ' ', 't', 'h', 'a', 't', ' ', 'l', 'i', 'e', ' ', 'o', 'n', ' ', 'i', 't', '.',  0 };
static const unichar_t str600[] = { 'P', 'o', 'i', 'n', 't', 's', ' ', 'n', 'e', 'a', 'r', ' ', 'h', 'i', 'n', 't', ' ', 'e', 'd', 'g', 'e', 's',  0 };
static const unichar_t str601[] = { 'O', 'f', 't', 'e', 'n', ' ', 'i', 'f', ' ', 'a', ' ', 'p', 'o', 'i', 'n', 't', ' ', 'i', 's', ' ', 's', 'l', 'i', 'g', 'h', 't', 'l', 'y', ' ', 'o', 'f', 'f', ' ', 'f', 'r', 'o', 'm', ' ', 'a', ' ', 'h', 'i', 'n', 't', 0xa, 'i', 't', ' ', 'i', 's', ' ', 'b', 'e', 'c', 'a', 'u', 's', 'e', ' ', 'a', ' ', 's', 't', 'e', 'm', ' ', 'i', 's', ' ', 'm', 'a', 'd', 'e', ' ', 'u', 'p', 0xa, 'o', 'f', ' ', 's', 'e', 'v', 'e', 'r', 'a', 'l', ' ', 's', 'e', 'g', 'm', 'e', 'n', 't', 's', ',', ' ', 'a', 'n', 'd', ' ', 'o', 'n', 'e', ' ', 'o', 'f', ' ', 't', 'h', 'e', 'm', 0xa, 'h', 'a', 's', ' ', 't', 'h', 'e', ' ', 'w', 'r', 'o', 'n', 'g', ' ', 'w', 'i', 'd', 't', 'h', '.',  0 };
static const unichar_t str602[] = { 'H', 'i', 'n', 't', ' ', 'W', 'i', 'd', 't', 'h', ' ', 'N', 'e', 'a', 'r',  0 };
static const unichar_t str603[] = { 'A', 'l', 'l', 'o', 'w', 's', ' ', 'y', 'o', 'u', ' ', 't', 'o', ' ', 'c', 'h', 'e', 'c', 'k', ' ', 't', 'h', 'a', 't', ' ', 's', 't', 'e', 'm', 's', ' ', 'h', 'a', 'v', 'e', ' ', 'c', 'o', 'n', 's', 'i', 's', 't', 'a', 'n', 't', ' ', 'w', 'i', 'd', 't', 'h', 's', '.', '.',  0 };
static const unichar_t str604[] = { 'C', 'h', 'e', 'c', 'k', ' ', 'o', 'u', 't', 'e', 'r', 'm', 'o', 's', 't', ' ', 'p', 'a', 't', 'h', 's', ' ', 'c', 'l', 'o', 'c', 'k', 'w', 'i', 's', 'e',  0 };
static const unichar_t str605[] = { 'P', 'o', 's', 't', 's', 'c', 'r', 'i', 'p', 't', ' ', 'a', 'n', 'd', ' ', 'T', 'r', 'u', 'e', 'T', 'y', 'p', 'e', ' ', 'r', 'e', 'q', 'u', 'i', 'r', 'e', ' ', 't', 'h', 'a', 't', ' ', 'p', 'a', 't', 'h', 's', ' ', 'b', 'e', ' ', 'd', 'r', 'a', 'w', 'n', 0xa, 'i', 'n', ' ', 'a', ' ', 'c', 'l', 'o', 'c', 'k', 'w', 'i', 's', 'e', ' ', 'd', 'i', 'r', 'e', 'c', 't', 'i', 'o', 'n', '.', ' ', 'T', 'h', 'i', 's', ' ', 'l', 'e', 't', 's', ' ', 'y', 'o', 'u', ' ', 'c', 'h', 'e', 'c', 'k', ' ', 't', 'h', 'a', 't', ' ', 't', 'h', 'e', 'y', 0xa, 'a', 'r', 'e', '.',  0 };
static const unichar_t str606[] = { 'P', 'o', 'i', 'n', 't', 's', ' ', 'a', 'r', 'e', ' ', '"', 'N', 'e', 'a', 'r', '"', ' ', 'i', 'f', ' ', 'w', 'i', 't', 'h', 'i', 'n',  0 };
static const unichar_t str607[] = { 'N', 'e', 'a', 'r',  0 };
static const unichar_t str608[] = { 'S', 't', 'o', 'p', ' ', 'a', 'f', 't', 'e', 'r', ' ', 'e', 'a', 'c', 'h', ' ', 'e', 'r', 'r', 'o', 'r', ' ', 'a', 'n', 'd', ' ', 'e', 'x', 'p', 'l', 'a', 'i', 'n',  0 };
static const unichar_t str609[] = { 'I', 'g', 'n', 'o', 'r', 'e', ' ', 't', 'h', 'i', 's', ' ', 'p', 'r', 'o', 'b', 'l', 'e', 'm', ' ', 'i', 'n', ' ', 't', 'h', 'e', ' ', 'f', 'u', 't', 'u', 'r', 'e',  0 };
static const unichar_t str610[] = { 'F', 'o', 'u', 'n', 'd', ' ',  0 };
static const unichar_t str611[] = { ',', ' ', 'e', 'x', 'p', 'e', 'c', 't', 'e', 'd', ' ',  0 };
static const unichar_t str612[] = { 'M', 'e', 't', 'a', ' ', 'F', 'o', 'n', 't', '.', '.', '.',  0 };
static const unichar_t str613[] = { 'C', 'o', 'u', 'n', 't', 'e', 'r', ' ', 'T', 'o', 'o', ' ', 'S', 'm', 'a', 'l', 'l',  0 };
static const unichar_t str614[] = { 'A', ' ', 'c', 'o', 'u', 'n', 't', 'e', 'r', ' ', 'w', 'a', 's', ' ', 'r', 'e', 'q', 'u', 'e', 's', 't', 'e', 'd', ' ', 't', 'o', ' ', 'b', 'e', ' ', 't', 'o', 'o', ' ', 's', 'm', 'a', 'l', 'l', ',', ' ', 'i', 't', ' ', 'h', 'a', 's', ' ', 'b', 'e', 'e', 'n', ' ', 'p', 'e', 'g', 'g', 'e', 'd', ' ', 'a', 't', ' ', 'i', 't', 's', ' ', 'm', 'i', 'n', 'i', 'm', 'u', 'm', ' ', 'v', 'a', 'l', 'u', 'e',  0 };
static const unichar_t str615[] = { 'S', 'i', 'm', 'p', 'l', 'e',  0 };
static const unichar_t str616[] = { 'A', 'd', 'v', 'a', 'n', 'c', 'e', 'd',  0 };
static const unichar_t str617[] = { 'E', 'm', 'b', 'o', 'l', 'd', 'e', 'n',  0 };
static const unichar_t str618[] = { 'C', 'o', 'n', 'd', 'e', 'n', 's', 'e',  0 };
static const unichar_t str619[] = { 'E', 'x', 'p', 'a', 'n', 'd',  0 };
static const unichar_t str620[] = { 'S', 't', 'e', 'm', 's',  0 };
static const unichar_t str621[] = { 'H', ' ', 'C', 'o', 'u', 'n', 't', 'e', 'r', 's',  0 };
static const unichar_t str622[] = { 'V', ' ', 'C', 'o', 'u', 'n', 't', 'e', 'r', 's',  0 };
static const unichar_t str623[] = { 'S', 'c', 'a', 'l', 'e', ' ', 'S', 't', 'e', 'm', 's', ' ', 'B', 'y', ':',  0 };
static const unichar_t str624[] = { 'S', 'c', 'a', 'l', 'e', ' ', 'C', 'o', 'u', 'n', 't', 'e', 'r', 's', ' ', 'B', 'y', ':',  0 };
static const unichar_t str625[] = { 'X', 'H', 'e', 'i', 'g', 'h', 't', ' ', 'F', 'r', 'o', 'm', ':',  0 };
static const unichar_t str626[] = { 'T', 'o', ':',  0 };
static const unichar_t str627[] = { 'M', 'e', 't', 'a', 'm', 'o', 'r', 'p', 'h', 'o', 's', 'i', 'n', 'g', ' ', 'F', 'o', 'n', 't', '.', '.', '.',  0 };
static const unichar_t str628[] = { 'P', 'i', 'c', 'k', ' ', 'a', ' ', 'f', 'o', 'n', 't', ',', ' ', 'a', 'n', 'y', ' ', 'f', 'o', 'n', 't', '.', '.', '.',  0 };
static const unichar_t str629[] = { 'T', 'h', 'e', 'r', 'e', ' ', 'a', 'r', 'e', ' ', 'm', 'u', 'l', 't', 'i', 'p', 'l', 'e', ' ', 'f', 'o', 'n', 't', 's', ' ', 'i', 'n', ' ', 't', 'h', 'i', 's', ' ', 'f', 'i', 'l', 'e', ',', ' ', 'p', 'i', 'c', 'k', ' ', 'o', 'n', 'e',  0 };
static const unichar_t str630[] = { 'A', 'u', 't', 'o', ' ', 'H', 'i', 'n', 't', 'i', 'n', 'g', ' ', 'F', 'o', 'n', 't', '.', '.', '.',  0 };
static const unichar_t str631[] = { 'S', 'a', 'v', 'i', 'n', 'g', ' ', 'O', 'p', 'e', 'n', 'T', 'y', 'p', 'e', ' ', 'F', 'o', 'n', 't',  0 };
static const unichar_t str632[] = { 'R', 'a', 's', 't', 'e', 'r', 'i', 'z', 'i', 'n', 'g', '.', '.', '.',  0 };
static const unichar_t str633[] = { 'G', 'e', 'n', 'e', 'r', 'a', 't', 'i', 'n', 'g', ' ', 'b', 'i', 't', 'm', 'a', 'p', ' ', 'f', 'o', 'n', 't',  0 };
static const unichar_t str634[] = { 'G', 'e', 'n', 'e', 'r', 'a', 't', 'i', 'n', 'g', ' ', 'a', 'n', 't', 'i', '-', 'a', 'l', 'i', 'a', 's', ' ', 'f', 'o', 'n', 't',  0 };
static const unichar_t str635[] = { ' ', 'p', 'i', 'x', 'e', 'l', 's',  0 };
static const unichar_t str636[] = { 'P', 'r', 'i', 'n', 't', 'i', 'n', 'g', ' ', 'F', 'o', 'n', 't',  0 };
static const unichar_t str637[] = { 'G', 'e', 'n', 'e', 'r', 'a', 't', 'i', 'n', 'g', ' ', 'P', 'o', 's', 't', 's', 'c', 'r', 'i', 'p', 't', ' ', 'F', 'o', 'n', 't',  0 };
static const unichar_t str638[] = { 'F', 'a', 'i', 'l', 'e', 'd', ' ', 't', 'o', ' ', 'g', 'e', 'n', 'e', 'r', 'a', 't', 'e', ' ', 'p', 'o', 's', 't', 's', 'c', 'r', 'i', 'p', 't', ' ', 'f', 'o', 'n', 't',  0 };
static const unichar_t str639[] = { 'P', 'a', 'g', 'e', ' ', 'S', 'e', 't', 'u', 'p',  0 };
static const unichar_t str640[] = { 'S', 'e', 't', 'u', 'p',  0 };
static const unichar_t str641[] = { 'T', 'o', ' ', 'F', 'i', 'l', 'e',  0 };
static const unichar_t str642[] = { 'P', 'a', 'g', 'e', 'S', 'i', 'z', 'e', ':',  0 };
static const unichar_t str643[] = { 'C', 'o', 'p', 'i', 'e', 's', ':',  0 };
static const unichar_t str644[] = { 'P', 'r', 'i', 'n', 't', 'e', 'r', ':',  0 };
static const unichar_t str645[] = { 'P', 'r', 'i', 'n', 't', ' ', 'T', 'o', ' ', 'F', 'i', 'l', 'e', '.', '.', '.',  0 };
static const unichar_t str646[] = { 'P', 'o', 'i', 'n', 't', 's', 'i', 'z', 'e', ':',  0 };
static const unichar_t str647[] = { 'F', 'u', 'l', 'l', ' ', 'F', 'o', 'n', 't', ' ', 'D', 'i', 's', 'p', 'l', 'a', 'y',  0 };
static const unichar_t str648[] = { 'F', 'u', 'l', 'l', ' ', 'P', 'a', 'g', 'e', ' ', 'C', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r',  0 };
static const unichar_t str649[] = { 'F', 'u', 'l', 'l', ' ', 'P', 'a', 'g', 'e', ' ', 'C', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', 's',  0 };
static const unichar_t str650[] = { 'S', 'a', 'm', 'p', 'l', 'e', ' ', 'T', 'e', 'x', 't', ':',  0 };
static const unichar_t str651[] = { 'F', 'a', 'i', 'l', 'e', 'd', ' ', 't', 'o', ' ', 'o', 'p', 'e', 'n', ' ', 't', 'e', 'm', 'p', 'o', 'r', 'a', 'r', 'y', ' ', 'o', 'u', 't', 'p', 'u', 't', ' ', 'f', 'i', 'l', 'e',  0 };

static const unichar_t *pfaedit_ui_strings[] = {
	str0,
	str1,
	str2,
	str3,
	str4,
	str5,
	str6,
	str7,
	str8,
	str9,
	str10,
	str11,
	str12,
	str13,
	str14,
	str15,
	str16,
	str17,
	str18,
	str19,
	str20,
	str21,
	str22,
	str23,
	str24,
	str25,
	str26,
	str27,
	str28,
	str29,
	str30,
	str31,
	str32,
	str33,
	str34,
	str35,
	str36,
	str37,
	str38,
	str39,
	str40,
	str41,
	str42,
	str43,
	str44,
	str45,
	str46,
	str47,
	str48,
	str49,
	str50,
	str51,
	str52,
	str53,
	str54,
	str55,
	str56,
	str57,
	str58,
	str59,
	str60,
	str61,
	str62,
	str63,
	str64,
	str65,
	str66,
	str67,
	str68,
	str69,
	str70,
	str71,
	str72,
	str73,
	str74,
	str75,
	str76,
	str77,
	str78,
	str79,
	str80,
	str81,
	str82,
	str83,
	str84,
	str85,
	str86,
	str87,
	str88,
	str89,
	str90,
	str91,
	str92,
	str93,
	str94,
	str95,
	str96,
	str97,
	str98,
	str99,
	str100,
	str101,
	str102,
	str103,
	str104,
	str105,
	str106,
	str107,
	str108,
	str109,
	str110,
	str111,
	str112,
	str113,
	str114,
	str115,
	str116,
	str117,
	str118,
	str119,
	str120,
	str121,
	str122,
	str123,
	str124,
	str125,
	str126,
	str127,
	str128,
	str129,
	str130,
	str131,
	str132,
	str133,
	str134,
	str135,
	str136,
	str137,
	str138,
	str139,
	str140,
	str141,
	str142,
	str143,
	str144,
	str145,
	str146,
	str147,
	str148,
	str149,
	str150,
	str151,
	str152,
	str153,
	str154,
	str155,
	str156,
	str157,
	str158,
	str159,
	str160,
	str161,
	str162,
	str163,
	str164,
	str165,
	str166,
	str167,
	str168,
	str169,
	str170,
	str171,
	str172,
	str173,
	str174,
	str175,
	str176,
	str177,
	str178,
	str179,
	str180,
	str181,
	str182,
	str183,
	str184,
	str185,
	str186,
	str187,
	str188,
	str189,
	str190,
	str191,
	str192,
	str193,
	str194,
	str195,
	str196,
	str197,
	str198,
	str199,
	str200,
	str201,
	str202,
	str203,
	str204,
	str205,
	str206,
	str207,
	str208,
	str209,
	str210,
	str211,
	str212,
	str213,
	str214,
	str215,
	str216,
	str217,
	str218,
	str219,
	str220,
	str221,
	str222,
	str223,
	str224,
	str225,
	str226,
	str227,
	str228,
	str229,
	str230,
	str231,
	str232,
	str233,
	str234,
	str235,
	str236,
	str237,
	str238,
	str239,
	str240,
	str241,
	str242,
	str243,
	str244,
	str245,
	str246,
	str247,
	str248,
	str249,
	str250,
	str251,
	str252,
	str253,
	str254,
	str255,
	str256,
	str257,
	str258,
	str259,
	str260,
	str261,
	str262,
	str263,
	str264,
	str265,
	str266,
	str267,
	str268,
	str269,
	str270,
	str271,
	str272,
	str273,
	str274,
	str275,
	str276,
	str277,
	str278,
	str279,
	str280,
	str281,
	str282,
	str283,
	str284,
	str285,
	str286,
	str287,
	str288,
	str289,
	str290,
	str291,
	str292,
	str293,
	str294,
	str295,
	str296,
	str297,
	str298,
	str299,
	str300,
	str301,
	str302,
	str303,
	str304,
	str305,
	str306,
	str307,
	str308,
	str309,
	str310,
	str311,
	str312,
	str313,
	str314,
	str315,
	str316,
	str317,
	str318,
	str319,
	str320,
	str321,
	str322,
	str323,
	str324,
	str325,
	str326,
	str327,
	str328,
	str329,
	str330,
	str331,
	str332,
	str333,
	str334,
	str335,
	str336,
	str337,
	str338,
	str339,
	str340,
	str341,
	str342,
	str343,
	str344,
	str345,
	str346,
	str347,
	str348,
	str349,
	str350,
	str351,
	str352,
	str353,
	str354,
	str355,
	str356,
	str357,
	str358,
	str359,
	str360,
	str361,
	str362,
	str363,
	str364,
	str365,
	str366,
	str367,
	str368,
	str369,
	str370,
	str371,
	str372,
	str373,
	str374,
	str375,
	str376,
	str377,
	str378,
	str379,
	str380,
	str381,
	str382,
	str383,
	str384,
	str385,
	str386,
	str387,
	str388,
	str389,
	str390,
	str391,
	str392,
	str393,
	str394,
	str395,
	str396,
	str397,
	str398,
	str399,
	str400,
	str401,
	str402,
	str403,
	str404,
	str405,
	str406,
	str407,
	str408,
	str409,
	str410,
	str411,
	str412,
	str413,
	str414,
	str415,
	str416,
	str417,
	str418,
	str419,
	str420,
	str421,
	str422,
	str423,
	str424,
	str425,
	str426,
	str427,
	str428,
	str429,
	str430,
	str431,
	str432,
	str433,
	str434,
	str435,
	str436,
	str437,
	str438,
	str439,
	str440,
	str441,
	str442,
	str443,
	str444,
	str445,
	str446,
	str447,
	str448,
	str449,
	str450,
	str451,
	str452,
	str453,
	str454,
	str455,
	str456,
	str457,
	str458,
	str459,
	str460,
	str461,
	str462,
	str463,
	str464,
	str465,
	str466,
	str467,
	str468,
	str469,
	str470,
	str471,
	str472,
	str473,
	str474,
	str475,
	str476,
	str477,
	str478,
	str479,
	str480,
	str481,
	str482,
	str483,
	str484,
	str485,
	str486,
	str487,
	str488,
	str489,
	str490,
	str491,
	str492,
	str493,
	str494,
	str495,
	str496,
	str497,
	str498,
	str499,
	str500,
	str501,
	str502,
	str503,
	str504,
	str505,
	str506,
	str507,
	str508,
	str509,
	str510,
	str511,
	str512,
	str513,
	str514,
	str515,
	str516,
	str517,
	str518,
	str519,
	str520,
	str521,
	str522,
	str523,
	str524,
	str525,
	str526,
	str527,
	str528,
	str529,
	str530,
	str531,
	str532,
	str533,
	str534,
	str535,
	str536,
	str537,
	str538,
	str539,
	str540,
	str541,
	str542,
	str543,
	str544,
	str545,
	str546,
	str547,
	str548,
	str549,
	str550,
	str551,
	str552,
	str553,
	str554,
	str555,
	str556,
	str557,
	str558,
	str559,
	str560,
	str561,
	str562,
	str563,
	str564,
	str565,
	str566,
	str567,
	str568,
	str569,
	str570,
	str571,
	str572,
	str573,
	str574,
	str575,
	str576,
	str577,
	str578,
	str579,
	str580,
	str581,
	str582,
	str583,
	str584,
	str585,
	str586,
	str587,
	str588,
	str589,
	str590,
	str591,
	str592,
	str593,
	str594,
	str595,
	str596,
	str597,
	str598,
	str599,
	str600,
	str601,
	str602,
	str603,
	str604,
	str605,
	str606,
	str607,
	str608,
	str609,
	str610,
	str611,
	str612,
	str613,
	str614,
	str615,
	str616,
	str617,
	str618,
	str619,
	str620,
	str621,
	str622,
	str623,
	str624,
	str625,
	str626,
	str627,
	str628,
	str629,
	str630,
	str631,
	str632,
	str633,
	str634,
	str635,
	str636,
	str637,
	str638,
	str639,
	str640,
	str641,
	str642,
	str643,
	str644,
	str645,
	str646,
	str647,
	str648,
	str649,
	str650,
	str651,
	NULL};

static const unichar_t pfaedit_ui_mnemonics[] = {
	0x0000, 'O',    'C',    'O',    'S',    'F',    'N',    0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 'F',    'E',    
	'P',    'l',    'i',    'V',    'M',    'W',    'H',    't',    
	'u',    'B',    'M',    'P',    'R',    'a',    'G',    'I',    
	'c',    'e',    'Q',    'F',    'i',    'o',    'N',    'P',    
	'G',    'o',    'o',    'R',    'R',    'e',    'v',    'l',    
	'U',    'R',    't',    'C',    'W',    'o',    'P',    'l',    
	'M',    'A',    'U',    'F',    'o',    'I',    'A',    'B',    
	'r',    'T',    'E',    'v',    'S',    'S',    'I',    'u',    
	'u',    'o',    'n',    'C',    'o',    'C',    'T',    'H',    
	'H',    'C',    'V',    'D',    'A',    's',    't',    'r',    
	'e',    'R',    't',    'P',    'T',    'L',    'C',    'T',    
	'W',    'L',    'R',    'v',    'M',    '2',    '3',    '4',    
	'A',    'I',    'M',    'I',    'F',    'A',    'D',    'K',    
	'A',    'P',    'O',    'G',    'G',    'B',    'S',    'H',    
	'V',    'R',    '9',    '1',    'S',    'r',    0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 'F',    
	'M',    's',    'E',    'L',    'k',    'M',    'I',    'P',    
	'H',    'A',    'D',    'r',    'X',    'N',    'G',    0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 'A',    0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 'C',    'W',    'F',    'F',    'S',    'W',    'P',    
	'C',    'V',    'A',    'L',    'M',    'X',    0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 'S',    0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 'D',    
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 'R',    0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	'Y',    'N',    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 'D',    0x0000, 'S',    0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 'D',    
	0x0000, 'B',    'S',    'H',    'V',    'e',    'P',    'N',    
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	'A',    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 'S',    
	'N',    'P',    0x0000, 't',    0x0000, 'X',    0x0000, 'Y',    
	0x0000, 'S',    0x0000, 'E',    0x0000, 0x0000, 'H',    0x0000, 
	'P',    0x0000, 'W',    0x0000, 'O',    0x0000, 'N',    0x0000, 
	'A',    0x0000, 0x0000, 0x0000, 'M',    0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	'e',    'F',    'S',    'C',    'P',    0x0000, 'P',    'F',    
	'C',    'C',    'T',    0x0000, 
	0};

static const int pfaedit_ui_num[] = {
    55, 
    0x80000000
};

void PfaEditSetFallback(void) {
    GStringSetFallbackArray(pfaedit_ui_strings,pfaedit_ui_mnemonics,pfaedit_ui_num);
}
