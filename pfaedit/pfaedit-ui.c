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
static const unichar_t str60[] = { 'P', 'r', 'i', 'v', 'a', 't', 'e', ' ', 'I', 'n', 'f', 'o', '.', '.', '.',  0 };
static const unichar_t str61[] = { 'F', 'i', 'n', 'd', ' ', 'P', 'r', 'o', 'b', 'l', 'e', 'm', 's', '.', '.', '.',  0 };
static const unichar_t str62[] = { 'G', 'e', 't', ' ', 'I', 'n', 'f', 'o', '.', '.', '.',  0 };
static const unichar_t str63[] = { 'B', 'i', 't', 'm', 'a', 'p', 's', ' ', 'A', 'v', 'a', 'i', 'l', 'a', 'b', 'l', 'e', '.', '.', '.',  0 };
static const unichar_t str64[] = { 'R', 'e', 'g', 'e', 'n', 'e', 'r', 'a', 't', 'e', ' ', 'B', 'i', 't', 'm', 'a', 'p', 's', '.', '.', '.',  0 };
static const unichar_t str65[] = { 'A', 'u', 't', 'o', 't', 'r', 'a', 'c', 'e',  0 };
static const unichar_t str66[] = { 'T', 'r', 'a', 'n', 's', 'f', 'o', 'r', 'm', '.', '.', '.',  0 };
static const unichar_t str67[] = { 'E', 'x', 'p', 'a', 'n', 'd', ' ', 'S', 't', 'r', 'o', 'k', 'e', '.', '.', '.',  0 };
static const unichar_t str68[] = { 'R', 'e', 'm', 'o', 'v', 'e', ' ', 'O', 'v', 'e', 'r', 'l', 'a', 'p',  0 };
static const unichar_t str69[] = { 'S', 'i', 'm', 'p', 'l', 'i', 'f', 'y',  0 };
static const unichar_t str70[] = { 'R', 'o', 'u', 'n', 'd', ' ', 't', 'o', ' ', 'i', 'n', 't',  0 };
static const unichar_t str71[] = { 'B', 'u', 'i', 'l', 'd', ' ', 'A', 'c', 'c', 'e', 'n', 't', 'e', 'd', ' ', 'C', 'h', 'a', 'r',  0 };
static const unichar_t str72[] = { 'C', 'l', 'o', 'c', 'k', 'w', 'i', 's', 'e',  0 };
static const unichar_t str73[] = { 'C', 'o', 'u', 'n', 't', 'e', 'r', ' ', 'C', 'l', 'o', 'c', 'k', 'w', 'i', 's', 'e',  0 };
static const unichar_t str74[] = { 'C', 'o', 'r', 'r', 'e', 'c', 't', ' ', 'D', 'i', 'r', 'e', 'c', 't', 'i', 'o', 'n',  0 };
static const unichar_t str75[] = { 'C', 'o', 'r', 'n', 'e', 'r',  0 };
static const unichar_t str76[] = { 'C', 'u', 'r', 'v', 'e',  0 };
static const unichar_t str77[] = { 'T', 'a', 'n', 'g', 'e', 'n', 't',  0 };
static const unichar_t str78[] = { 'A', 'u', 't', 'o', 'H', 'i', 'n', 't',  0 };
static const unichar_t str79[] = { 'C', 'l', 'e', 'a', 'r', ' ', 'H', 'S', 't', 'e', 'm',  0 };
static const unichar_t str80[] = { 'C', 'l', 'e', 'a', 'r', ' ', 'V', 'S', 't', 'e', 'm',  0 };
static const unichar_t str81[] = { 'C', 'l', 'e', 'a', 'r', ' ', 'D', 'S', 't', 'e', 'm',  0 };
static const unichar_t str82[] = { 'A', 'd', 'd', ' ', 'H', 'H', 'i', 'n', 't',  0 };
static const unichar_t str83[] = { 'A', 'd', 'd', ' ', 'V', 'H', 'i', 'n', 't',  0 };
static const unichar_t str84[] = { 'A', 'd', 'd', ' ', 'D', 'H', 'i', 'n', 't',  0 };
static const unichar_t str85[] = { 'C', 'r', 'e', 'a', 't', 'e', ' ', 'H', 'H', 'i', 'n', 't',  0 };
static const unichar_t str86[] = { 'C', 'r', 'e', 'a', 't', 'e', ' ', 'V', 'H', 'i', 'n', 't',  0 };
static const unichar_t str87[] = { 'R', 'e', 'v', 'i', 'e', 'w', ' ', 'H', 'i', 'n', 't', 's',  0 };
static const unichar_t str88[] = { 'E', 'x', 'p', 'o', 'r', 't', '.', '.', '.',  0 };
static const unichar_t str89[] = { 'P', 'a', 'l', 'e', 't', 't', 'e', 's',  0 };
static const unichar_t str90[] = { 'T', 'o', 'o', 'l', 's',  0 };
static const unichar_t str91[] = { 'L', 'a', 'y', 'e', 'r', 's',  0 };
static const unichar_t str92[] = { 'C', 'e', 'n', 't', 'e', 'r', ' ', 'i', 'n', ' ', 'W', 'i', 'd', 't', 'h',  0 };
static const unichar_t str93[] = { 'T', 'h', 'i', 'r', 'd', 's', ' ', 'i', 'n', ' ', 'W', 'i', 'd', 't', 'h',  0 };
static const unichar_t str94[] = { 'S', 'e', 't', ' ', 'W', 'i', 'd', 't', 'h', '.', '.', '.',  0 };
static const unichar_t str95[] = { 'S', 'e', 't', ' ', 'L', 'B', 'e', 'a', 'r', 'i', 'n', 'g', '.', '.', '.',  0 };
static const unichar_t str96[] = { 'S', 'e', 't', ' ', 'R', 'B', 'e', 'a', 'r', 'i', 'n', 'g', '.', '.', '.',  0 };
static const unichar_t str97[] = { 'R', 'e', 'm', 'o', 'v', 'e', ' ', 'K', 'e', 'r', 'n', ' ', 'P', 'a', 'i', 'r', 's',  0 };
static const unichar_t str98[] = { 'M', 'e', 'r', 'g', 'e', ' ', 'K', 'e', 'r', 'n', ' ', 'I', 'n', 'f', 'o', '.', '.', '.',  0 };
static const unichar_t str99[] = { '2', '4', ' ', 'p', 'i', 'x', 'e', 'l', ' ', 'o', 'u', 't', 'l', 'i', 'n', 'e',  0 };
static const unichar_t str100[] = { '3', '6', ' ', 'p', 'i', 'x', 'e', 'l', ' ', 'o', 'u', 't', 'l', 'i', 'n', 'e',  0 };
static const unichar_t str101[] = { '4', '8', ' ', 'p', 'i', 'x', 'e', 'l', ' ', 'o', 'u', 't', 'l', 'i', 'n', 'e',  0 };
static const unichar_t str102[] = { 'A', 'n', 't', 'i', ' ', 'A', 'l', 'i', 'a', 's',  0 };
static const unichar_t str103[] = { 'C', 'h', 'a', 'r', ' ', 'I', 'n', 'f', 'o', '.', '.', '.',  0 };
static const unichar_t str104[] = { 'M', 'e', 'r', 'g', 'e', ' ', 'F', 'o', 'n', 't', 's', '.', '.', '.',  0 };
static const unichar_t str105[] = { 'I', 'n', 't', 'e', 'r', 'p', 'o', 'l', 'a', 't', 'e', ' ', 'F', 'o', 'n', 't', 's', '.', '.', '.',  0 };
static const unichar_t str106[] = { 'C', 'o', 'p', 'y', ' ', 'F', 'r', 'o', 'm',  0 };
static const unichar_t str107[] = { 'A', 'l', 'l', ' ', 'F', 'o', 'n', 't', 's',  0 };
static const unichar_t str108[] = { 'D', 'i', 's', 'p', 'l', 'a', 'y', 'e', 'd', ' ', 'F', 'o', 'n', 't',  0 };
static const unichar_t str109[] = { 'A', 'u', 't', 'o', ' ', 'K', 'e', 'r', 'n', '.', '.', '.',  0 };
static const unichar_t str110[] = { 'A', 'u', 't', 'o', ' ', 'W', 'i', 'd', 't', 'h', '.', '.', '.',  0 };
static const unichar_t str111[] = { 'R', 'e', 'm', 'o', 'v', 'e', ' ', 'A', 'l', 'l', ' ', 'K', 'e', 'r', 'n', ' ', 'P', 'a', 'i', 'r', 's',  0 };
static const unichar_t str112[] = { 'O', 'u', 't', 'l', 'i', 'n', 'e',  0 };
static const unichar_t str113[] = { 'S', 'h', 'o', 'w', ' ', 'G', 'r', 'i', 'd',  0 };
static const unichar_t str114[] = { 'H', 'i', 'd', 'e', ' ', 'G', 'r', 'i', 'd',  0 };
static const unichar_t str115[] = { 'B', 'i', 'g', 'g', 'e', 'r', ' ', 'P', 'i', 'x', 'e', 'l', ' ', 'S', 'i', 'z', 'e',  0 };
static const unichar_t str116[] = { 'S', 'm', 'a', 'l', 'l', 'e', 'r', ' ', 'P', 'i', 'x', 'e', 'l', ' ', 'S', 'i', 'z', 'e',  0 };
static const unichar_t str117[] = { 'F', 'l', 'i', 'p', ' ', 'H', 'o', 'r', 'i', 'z', 'o', 'n', 't', 'a', 'l', 'l', 'y',  0 };
static const unichar_t str118[] = { 'F', 'l', 'i', 'p', ' ', 'V', 'e', 'r', 't', 'i', 'c', 'a', 'l', 'l', 'y',  0 };
static const unichar_t str119[] = { 'R', 'o', 't', 'a', 't', 'e', ' ', '9', '0', 0xb0, ' ', 'C', 'W',  0 };
static const unichar_t str120[] = { 'R', 'o', 't', 'a', 't', 'e', ' ', '9', '0', 0xb0, ' ', 'C', 'C', 'W',  0 };
static const unichar_t str121[] = { 'R', 'o', 't', 'a', 't', 'e', ' ', '1', '8', '0', 0xb0,  0 };
static const unichar_t str122[] = { 'S', 'k', 'e', 'w', '.', '.', '.',  0 };
static const unichar_t str123[] = { 'C', 'u', 's', 't', 'o', 'm',  0 };
static const unichar_t str124[] = { 'I', 'S', 'O', ' ', '8', '8', '5', '9', '-', '1', ' ', ' ', '(', 'L', 'a', 't', 'i', 'n', '1', ')',  0 };
static const unichar_t str125[] = { 'I', 'S', 'O', ' ', '8', '8', '5', '9', '-', '1', '5', ' ', ' ', '(', 'L', 'a', 't', 'i', 'n', '0', ')',  0 };
static const unichar_t str126[] = { 'I', 'S', 'O', ' ', '8', '8', '5', '9', '-', '2', ' ', ' ', '(', 'L', 'a', 't', 'i', 'n', '2', ')',  0 };
static const unichar_t str127[] = { 'I', 'S', 'O', ' ', '8', '8', '5', '9', '-', '3', ' ', ' ', '(', 'L', 'a', 't', 'i', 'n', '3', ')',  0 };
static const unichar_t str128[] = { 'I', 'S', 'O', ' ', '8', '8', '5', '9', '-', '4', ' ', ' ', '(', 'L', 'a', 't', 'i', 'n', '4', ')',  0 };
static const unichar_t str129[] = { 'I', 'S', 'O', ' ', '8', '8', '5', '9', '-', '9', ' ', ' ', '(', 'L', 'a', 't', 'i', 'n', '5', ')',  0 };
static const unichar_t str130[] = { 'I', 'S', 'O', ' ', '8', '8', '5', '9', '-', '1', '0', ' ', ' ', '(', 'L', 'a', 't', 'i', 'n', '6', ')',  0 };
static const unichar_t str131[] = { 'I', 'S', 'O', ' ', '8', '8', '5', '9', '-', '1', '3', ' ', ' ', '(', 'L', 'a', 't', 'i', 'n', '7', ')',  0 };
static const unichar_t str132[] = { 'I', 'S', 'O', ' ', '8', '8', '5', '9', '-', '1', '4', ' ', ' ', '(', 'L', 'a', 't', 'i', 'n', '8', ')',  0 };
static const unichar_t str133[] = { 'I', 'S', 'O', ' ', '8', '8', '5', '9', '-', '5', ' ', '(', 'C', 'y', 'r', 'i', 'l', 'l', 'i', 'c', ')',  0 };
static const unichar_t str134[] = { 'K', 'O', 'I', '8', '-', 'R', ' ', '(', 'C', 'y', 'r', 'i', 'l', 'l', 'i', 'c', ')',  0 };
static const unichar_t str135[] = { 'I', 'S', 'O', ' ', '8', '8', '5', '9', '-', '6', ' ', '(', 'A', 'r', 'a', 'b', 'i', 'c', ')',  0 };
static const unichar_t str136[] = { 'I', 'S', 'O', ' ', '8', '8', '5', '9', '-', '7', ' ', '(', 'G', 'r', 'e', 'e', 'k', ')',  0 };
static const unichar_t str137[] = { 'I', 'S', 'O', ' ', '8', '8', '5', '9', '-', '8', ' ', '(', 'H', 'e', 'b', 'r', 'e', 'w', ')',  0 };
static const unichar_t str138[] = { 'M', 'a', 'c', 'i', 'n', 't', 'o', 's', 'h', ' ', 'L', 'a', 't', 'i', 'n',  0 };
static const unichar_t str139[] = { 'W', 'i', 'n', 'd', 'o', 'w', 's', ' ', 'L', 'a', 't', 'i', 'n', ' ', '(', '"', 'N', 'S', 'I', '"',  0 };
static const unichar_t str140[] = { 'A', 'd', 'o', 'b', 'e', ' ', 'S', 't', 'a', 'n', 'd', 'a', 'r', 'd',  0 };
static const unichar_t str141[] = { 'S', 'y', 'm', 'b', 'o', 'l',  0 };
static const unichar_t str142[] = {  0 };
static const unichar_t str143[] = { 'I', 'S', 'O', ' ', '1', '0', '6', '4', '6', '-', '1', ' ', '(', 'U', 'n', 'i', 'c', 'o', 'd', 'e', ')',  0 };
static const unichar_t str144[] = { 'J', 'I', 'S', ' ', '2', '0', '8', ' ', '(', 'K', 'a', 'n', 'j', 'i', ')',  0 };
static const unichar_t str145[] = { 'J', 'I', 'S', ' ', '2', '1', '2', ' ', '(', 'K', 'a', 'n', 'j', 'i', ')',  0 };
static const unichar_t str146[] = { 'K', 'S', 'C', ' ', '5', '6', '0', '1', ' ', '(', 'K', 'o', 'r', 'e', 'a', 'n', ')',  0 };
static const unichar_t str147[] = { 'G', 'B', ' ', '2', '3', '1', '2', ' ', '(', 'C', 'h', 'i', 'n', 'e', 's', 'e', ')',  0 };
static const unichar_t str148[] = { 'Y', 'o', 'u', ' ', 'a', 'r', 'e', ' ', 'r', 'e', 'd', 'u', 'c', 'i', 'n', 'g', ' ', 't', 'h', 'e', ' ', 'n', 'u', 'm', 'b', 'e', 'r', ' ', 'o', 'f', ' ', 'c', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', 's', ' ', 'b', 'e', 'l', 'o', 'w', ' ', 't', 'h', 'e', 0xa, 'u', 'r', 'r', 'e', 'n', 't', ' ', 'n', 'u', 'm', 'b', 'e', 'r', '.', ' ', 'T', 'h', 'i', 's', ' ', 'w', 'i', 'l', 'l', ' ', 'd', 'e', 'l', 'e', 't', 'e', ' ', 's', 'o', 'm', 'e', ' ', 'c', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', 's', '.', 0xa, 's', ' ', 't', 'h', 'a', 't', ' ', 'w', 'h', 'a', 't', ' ', 'y', 'o', 'u', ' ', 'w', 'i', 's', 'h', ' ', 't', 'o', ' ', 'd', 'o', '?',  0 };
static const unichar_t str149[] = { 'T', 'o', 'o', ' ', 'F', 'e', 'w', ' ', 'C', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', 's',  0 };
static const unichar_t str150[] = { 'B', 'a', 'd', ' ', 'F', 'a', 'm', 'i', 'l', 'y', ' ', 'N', 'a', 'm', 'e', ',', ' ', 'm', 'u', 's', 't', ' ', 'b', 'e', 'g', 'i', 'n', ' ', 'w', 'i', 't', 'h', ' ', 'a', 'n', ' ', 'a', 'l', 'p', 'h', 'a', 'b', 'e', 't', 'i', 'c', ' ', 'c', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', '.',  0 };
static const unichar_t str151[] = { 'B', 'a', 'd', ' ', 'F', 'a', 'm', 'i', 'l', 'y', ' ', 'N', 'a', 'm', 'e',  0 };
static const unichar_t str152[] = { 'A', 's', 'c', 'e', 'n', 't', ' ', 'a', 'n', 'd', ' ', 'D', 'e', 's', 'c', 'e', 'n', 't', ' ', 'm', 'u', 's', 't', ' ', 'b', 'e', ' ', 'p', 'o', 's', 'i', 't', 'i', 'v', 'e', ' ', 'a', 'n', 'd', ' ', 't', 'h', 'e', 'i', 'r', ' ', 's', 'u', 'm', ' ', 'l', 'e', 's', 's', ' ', 't', 'h', 'a', 'n', ' ', '1', '6', '3', '8', '4',  0 };
static const unichar_t str153[] = { 'B', 'a', 'd', ' ', 'A', 's', 'c', 'e', 'n', 't', '/', 'D', 'e', 's', 'c', 'e', 'n', 't',  0 };
static const unichar_t str154[] = { 'F', 'o', 'n', 't', ' ', 'I', 'n', 'f', 'o', 'r', 'm', 'a', 't', 'i', 'o', 'n',  0 };
static const unichar_t str155[] = { 'F', 'a', 'm', 'i', 'l', 'y', ' ', 'N', 'a', 'm', 'e', ':',  0 };
static const unichar_t str156[] = { 'F', 'o', 'n', 't', ' ', 'M', 'o', 'd', 'i', 'f', 'i', 'e', 'r', 's', ':',  0 };
static const unichar_t str157[] = { 'N', 'a', 'm', 'e', ' ', 'F', 'o', 'r', ' ', 'H', 'u', 'm', 'a', 'n', 's', ':',  0 };
static const unichar_t str158[] = { 'E', 'n', 'c', 'o', 'd', 'i', 'n', 'g', ':',  0 };
static const unichar_t str159[] = { 'L', 'o', 'a', 'd',  0 };
static const unichar_t str160[] = { 'M', 'a', 'k', 'e', ' ', 'F', 'r', 'o', 'm', ' ', 'F', 'o', 'n', 't',  0 };
static const unichar_t str161[] = { 'R', 'e', 'm', 'o', 'v', 'e',  0 };
static const unichar_t str162[] = { 'I', 't', 'a', 'l', 'i', 'c', ' ', 'A', 'n', 'g', 'l', 'e', ':',  0 };
static const unichar_t str163[] = { 'U', 'n', 'd', 'e', 'r', 'l', 'i', 'n', 'e', ' ', 'P', 'o', 's', 'i', 't', 'i', 'o', 'n', ':',  0 };
static const unichar_t str164[] = { 'H', 'e', 'i', 'g', 'h', 't', ':',  0 };
static const unichar_t str165[] = { 'A', 's', 'c', 'e', 'n', 't', ':',  0 };
static const unichar_t str166[] = { 'D', 'e', 's', 'c', 'e', 'n', 't', ':',  0 };
static const unichar_t str167[] = { 'C', 'o', 'p', 'y', 'r', 'i', 'g', 'h', 't', ':',  0 };
static const unichar_t str168[] = { 'X', 'U', 'I', 'D', ':',  0 };
static const unichar_t str169[] = { 'N', 'u', 'm', 'b', 'e', 'r', ' ', 'o', 'f', ' ', 'C', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', 's', ':',  0 };
static const unichar_t str170[] = { 'G', 'u', 'e', 's', 's',  0 };
static const unichar_t str171[] = { 'F', 'o', 'r', 'm', 'a', 't', ':',  0 };
static const unichar_t str172[] = { 'S', 'a', 'v', 'e', ' ', 'F', 'a', 'i', 'l', 'e', 'd',  0 };
static const unichar_t str173[] = { 'O', 'u', 't', 'p', 'u', 't', ' ', 'A', 'F', 'M',  0 };
static const unichar_t str174[] = { 'O', 'u', 't', 'p', 'u', 't', ' ', 'P', 'F', 'M',  0 };
static const unichar_t str175[] = { 'N', 'o', ' ', 'O', 'u', 't', 'l', 'i', 'n', 'e', ' ', 'F', 'o', 'n', 't',  0 };
static const unichar_t str176[] = { 'N', 'o', ' ', 'B', 'i', 't', 'm', 'a', 'p', ' ', 'F', 'o', 'n', 't', 's',  0 };
static const unichar_t str177[] = { 'A', 'f', 'm', ' ', 'S', 'a', 'v', 'e', ' ', 'F', 'a', 'i', 'l', 'e', 'd',  0 };
static const unichar_t str178[] = { 'P', 'f', 'm', ' ', 'S', 'a', 'v', 'e', ' ', 'F', 'a', 'i', 'l', 'e', 'd',  0 };
static const unichar_t str179[] = { 'B', 'a', 'd', ' ', 'N', 'u', 'm', 'b', 'e', 'r', ' ', 'i', 'n', ' ',  0 };
static const unichar_t str180[] = { 'E', 'n', 't', 'e', 'r', ' ', 't', 'h', 'e', ' ', 'n', 'a', 'm', 'e', ' ', 'o', 'f', ' ', 'a', ' ', 'c', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', ' ', 'i', 'n', ' ', 't', 'h', 'e', ' ', 'f', 'o', 'n', 't',  0 };
static const unichar_t str181[] = { 'C', 'o', 'u', 'l', 'd', ' ', 'n', 'o', 't', ' ', 'f', 'i', 'n', 'd', ' ', 't', 'h', 'e', ' ', 'c', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', ':', ' ',  0 };
static const unichar_t str182[] = { 'D', 'o', 'n', 0x27, 't', ' ', 'S', 'a', 'v', 'e',  0 };
static const unichar_t str183[] = { 'F', 'o', 'n', 't', ' ', 'c', 'h', 'a', 'n', 'g', 'e', 'd',  0 };
static const unichar_t str184[] = { 'F', 'o', 'n', 't', ' ',  0 };
static const unichar_t str185[] = { ' ', 'i', 'n', ' ', 'f', 'i', 'l', 'e', ' ',  0 };
static const unichar_t str186[] = { ' ', 'h', 'a', 's', ' ', 'b', 'e', 'e', 'n', ' ', 'c', 'h', 'a', 'n', 'g', 'e', 'd', '.', 0xa, 'o', ' ', 'y', 'o', 'u', ' ', 'w', 'a', 'n', 't', ' ', 't', 'o', ' ', 's', 'a', 'v', 'e', ' ', 'i', 't', '?',  0 };
static const unichar_t str187[] = { ' ', 'h', 'a', 's', ' ', 'b', 'e', 'e', 'n', ' ', 'c', 'h', 'a', 'n', 'g', 'e', 'd', '.', 0xa, 'e', 'v', 'e', 'r', 't', 'i', 'n', 'g', ' ', 't', 'h', 'e', ' ', 'f', 'i', 'l', 'e', ' ', 'w', 'i', 'l', 'l', ' ', 'l', 'o', 's', 'e', ' ', 't', 'h', 'o', 's', 'e', ' ', 'c', 'h', 'a', 'n', 'g', 'e', 's', '.', 0xa, 's', ' ', 't', 'h', 'a', 't', ' ', 'w', 'h', 'a', 't', ' ', 'y', 'o', 'u', ' ', 'w', 'a', 'n', 't', '?',  0 };
static const unichar_t str188[] = { 'R', 'e', 'v', 'e', 'r', 't',  0 };
static const unichar_t str189[] = { 'M', 'a', 'n', 'y', ' ', 'W', 'i', 'n', 'd', 'o', 'w', 's',  0 };
static const unichar_t str190[] = { 'T', 'h', 'i', 's', ' ', 'i', 'n', 'v', 'o', 'l', 'v', 'e', 's', ' ', 'o', 'p', 'e', 'n', 'i', 'n', 'g', ' ', 'm', 'o', 'r', 'e', ' ', 't', 'h', 'a', 'n', ' ', '1', '0', ' ', 'w', 'i', 'n', 'd', 'o', 'w', 's', '.', 0xa, 's', ' ', 't', 'h', 'a', 't', ' ', 'r', 'e', 'a', 'l', 'l', 'y', ' ', 'w', 'h', 'a', 't', ' ', 'y', 'o', 'u', ' ', 'w', 'a', 'n', 't', '?',  0 };
static const unichar_t str191[] = { 'B', 'u', 'i', 'l', 'd', 'i', 'n', 'g', ' ', 'a', 'c', 'c', 'e', 'n', 't', 'e', 'd', ' ', 'l', 'e', 't', 't', 'e', 'r', 's',  0 };
static const unichar_t str192[] = { 'R', 'e', 'p', 'l', 'a', 'c', 'e', ' ', 0xc5,  0 };
static const unichar_t str193[] = { 'A', 'r', 'e', ' ', 'y', 'o', 'u', ' ', 's', 'u', 'r', 'e', ' ', 'y', 'o', 'u', ' ', 'w', 'a', 'n', 't', ' ', 't', 'o', ' ', 'r', 'e', 'p', 'l', 'a', 'c', 'e', ' ', 0xc5, '?', 0xa, 'h', 'e', ' ', 'r', 'i', 'n', 'g', ' ', 'w', 'i', 'l', 'l', ' ', 'n', 'o', 't', ' ', 'j', 'o', 'i', 'n', ' ', 't', 'o', ' ', 't', 'h', 'e', ' ', 'A', '.',  0 };
static const unichar_t str194[] = { 'Y', 'e', 's',  0 };
static const unichar_t str195[] = { 'N', 'o',  0 };
static const unichar_t str196[] = { 'R', 'e', 'c', 't', 'a', 'n', 'g', 'l', 'e',  0 };
static const unichar_t str197[] = { 'F', 'i', 'l', 'l', 'e', 'd', ' ', 'R', 'e', 'c', 't', 'a', 'n', 'g', 'l', 'e',  0 };
static const unichar_t str198[] = { 'E', 'l', 'i', 'p', 's', 'e',  0 };
static const unichar_t str199[] = { 'F', 'i', 'l', 'l', 'e', 'd', ' ', 'E', 'l', 'i', 'p', 's', 'e',  0 };
static const unichar_t str200[] = { 'M', 'u', 'l', 't', 'i', 'p', 'l', 'e',  0 };
static const unichar_t str201[] = { 'T', 'h', 'e', 'r', 'e', ' ', 'i', 's', ' ', 'a', 'l', 'r', 'e', 'a', 'd', 'y', ' ', 'a', ' ', 'c', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', ' ', 'w', 'i', 't', 'h', ' ', 't', 'h', 'i', 's', ' ', 'U', 'n', 'i', 'c', 'o', 'd', 'e', ' ', 'e', 'n', 'c', 'o', 'd', 'i', 'n', 'g', ',', 0xa, 'n', 'a', 'm', 'e', 'd', ' ',  0 };
static const unichar_t str202[] = { ' ', 'a', 't', ' ', 'l', 'o', 'c', 'a', 'l', ' ', 'e', 'n', 'c', 'o', 'd', 'i', 'n', 'g', ' ',  0 };
static const unichar_t str203[] = { ')', 0xa, 's', ' ', 't', 'h', 'a', 't', ' ', 'w', 'h', 'a', 't', ' ', 'y', 'o', 'u', ' ', 'w', 'a', 'n', 't', '?',  0 };
static const unichar_t str204[] = { 'T', 'h', 'e', 'r', 'e', ' ', 'i', 's', ' ', 'a', 'l', 'r', 'e', 'a', 'd', 'y', ' ', 'a', ' ', 'c', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', ' ', 'w', 'i', 't', 'h', ' ', 't', 'h', 'i', 's', ' ', 'n', 'a', 'm', 'e', ',', 0xa, 'o', ' ', 'y', 'o', 'u', ' ', 'w', 'a', 'n', 't', ' ', 't', 'o', ' ', 's', 'w', 'a', 'p', ' ', 'n', 'a', 'm', 'e', 's', '?',  0 };
static const unichar_t str205[] = { 'T', 'h', 'e', 'r', 'e', ' ', 'i', 's', ' ', 'a', 'l', 'r', 'e', 'a', 'd', 'y', ' ', 'a', ' ', 'l', 'i', 'g', 'a', 't', 'u', 'r', 'e', ' ', 'm', 'a', 'd', 'e', ' ', 'f', 'r', 'o', 'm', ' ', 't', 'h', 'e', 's', 'e', ' ', 'c', 'o', 'm', 'p', 'o', 'n', 'a', 'n', 't', 's', ',', 0xa, 'n', 'a', 'm', 'e', 'd', ' ',  0 };
static const unichar_t str206[] = { ' ', 'a', 't', ' ', 'l', 'o', 'c', 'a', 'l', ' ', 'e', 'n', 'c', 'o', 'd', 'i', 'n', 'g', ' ',  0 };
static const unichar_t str207[] = { ')', 0xa, 's', ' ', 't', 'h', 'a', 't', ' ', 'w', 'h', 'a', 't', ' ', 'y', 'o', 'u', ' ', 'w', 'a', 'n', 't', '?',  0 };
static const unichar_t str208[] = { 'A', ' ', 'l', 'i', 'g', 'a', 't', 'u', 'r', 'e', ' ', 'm', 'a', 'y', ' ', 'n', 'o', 't', ' ', 'b', 'e', ' ', 'm', 'a', 'd', 'e', ' ', 'u', 'p', ' ', 'o', 'f', ' ', 'i', 't', 's', 'e', 'l', 'f',  0 };
static const unichar_t str209[] = { 'T', 'h', 'e', ' ', 'c', 'o', 'm', 'p', 'o', 'n', 'a', 'n', 't', ' ',  0 };
static const unichar_t str210[] = { ' ', 'i', 's', ' ', 'n', 'o', 't', ' ', 'i', 'n', ' ', 't', 'h', 'i', 's', ' ', 'f', 'o', 'n', 't', ',', 0xa, 's', ' ', 't', 'h', 'a', 't', ' ', 'w', 'h', 'a', 't', ' ', 'y', 'o', 'u', ' ', 'w', 'a', 'n', 't', '?',  0 };
static const unichar_t str211[] = { 'D', 'o', 'n', 'e',  0 };
static const unichar_t str212[] = { 'I', 'f', ' ', 't', 'h', 'i', 's', ' ', 'c', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', ' ', 'i', 's', ' ', 'a', ' ', 'l', 'i', 'g', 'a', 't', 'u', 'r', 'e', ',', 0xa, 'h', 'e', 'n', ' ', 'e', 'n', 't', 'e', 'r', ' ', 't', 'h', 'e', ' ', 'n', 'a', 'm', 'e', 's', ' ', 'o', 'f', ' ', 't', 'h', 'e', ' ', 'c', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', 's', 0xa, 'n', 't', 'o', ' ', 'w', 'h', 'i', 'c', 'h', ' ', 'i', 't', ' ', 'd', 'e', 'c', 'o', 'm', 'p', 'o', 's', 'e', 's',  0 };
static const unichar_t str213[] = { 'S', 'h', 'o', 'w',  0 };
static const unichar_t str214[] = { 'D', 'u', 'p', 'l', 'i', 'c', 'a', 't', 'e', ' ', 'p', 'i', 'x', 'e', 'l', 's', 'i', 'z', 'e',  0 };
static const unichar_t str215[] = { 'T', 'h', 'e', ' ', 'f', 'o', 'n', 't', ' ', 'd', 'a', 't', 'a', 'b', 'a', 's', 'e', ' ', 'a', 'l', 'r', 'e', 'a', 'd', 'y', ' ', 'c', 'o', 'n', 't', 'a', 'i', 'n', 's', ' ', 'a', ' ', 'b', 'i', 't', 'm', 'a', 'p', 0xa, 'o', 'n', 't', ' ', 'w', 'i', 't', 'h', ' ', 't', 'h', 'i', 's', ' ', 'p', 'i', 'x', 'e', 'l', 's', 'i', 'z', 'e', ' ', '(',  0 };
static const unichar_t str216[] = { ')', 0xa, 'o', ' ', 'y', 'o', 'u', ' ', 'w', 'a', 'n', 't', ' ', 't', 'o', ' ', 'o', 'v', 'e', 'r', 'w', 'r', 'i', 't', 'e', ' ', 'i', 't', '?',  0 };
static const unichar_t str217[] = { 'T', 'h', 'i', 's', ' ', 'w', 'i', 'l', 'l', ' ', 'c', 'h', 'a', 'n', 'g', 'e', ' ', 'b', 'o', 't', 'h', ' ', 'B', 'l', 'u', 'e', 'V', 'a', 'l', 'u', 'e', 's', ' ', 'a', 'n', 'd', ' ', 'O', 't', 'h', 'e', 'r', 'B', 'l', 'u', 'e', 's', '.', 0xa, 'o', ' ', 'y', 'o', 'u', ' ', 'w', 'a', 'n', 't', ' ', 't', 'o', ' ', 'c', 'o', 'n', 't', 'i', 'n', 'u', 'e', '?',  0 };
static const unichar_t str218[] = { 'T', 'h', 'i', 's', ' ', 'w', 'i', 'l', 'l', ' ', 'c', 'h', 'a', 'n', 'g', 'e', ' ', 'b', 'o', 't', 'h', ' ', 'S', 't', 'd', 'H', 'W', ' ', 'a', 'n', 'd', ' ', 'S', 't', 'e', 'm', 'S', 'n', 'a', 'p', 'H', '.', 0xa, 'o', ' ', 'y', 'o', 'u', ' ', 'w', 'a', 'n', 't', ' ', 't', 'o', ' ', 'c', 'o', 'n', 't', 'i', 'n', 'u', 'e', '?',  0 };
static const unichar_t str219[] = { 'T', 'h', 'i', 's', ' ', 'w', 'i', 'l', 'l', ' ', 'c', 'h', 'a', 'n', 'g', 'e', ' ', 'b', 'o', 't', 'h', ' ', 'S', 't', 'd', 'V', 'W', ' ', 'a', 'n', 'd', ' ', 'S', 't', 'e', 'm', 'S', 'n', 'a', 'p', 'V', '.', 0xa, 'o', ' ', 'y', 'o', 'u', ' ', 'w', 'a', 'n', 't', ' ', 't', 'o', ' ', 'c', 'o', 'n', 't', 'i', 'n', 'u', 'e', '?',  0 };
static const unichar_t str220[] = { 'E', 'x', 'p', 'e', 'c', 't', 'e', 'd', ' ', 'a', 'r', 'r', 'a', 'y', 0xa, 'r', 'o', 'c', 'e', 'd', 'e', ' ', 'a', 'n', 'y', 'w', 'a', 'y', '?',  0 };
static const unichar_t str221[] = { 'E', 'x', 'p', 'e', 'c', 't', 'e', 'd', ' ', 'n', 'u', 'm', 'b', 'e', 'r', 0xa, 'r', 'o', 'c', 'e', 'd', 'e', ' ', 'a', 'n', 'y', 'w', 'a', 'y', '?',  0 };
static const unichar_t str222[] = { 'E', 'x', 'p', 'e', 'c', 't', 'e', 'd', ' ', 'b', 'o', 'o', 'l', 'e', 'a', 'n', 0xa, 'r', 'o', 'c', 'e', 'd', 'e', ' ', 'a', 'n', 'y', 'w', 'a', 'y', '?',  0 };
static const unichar_t str223[] = { 'E', 'x', 'p', 'e', 'c', 't', 'e', 'd', ' ', 'c', 'o', 'd', 'e', 0xa, 'r', 'o', 'c', 'e', 'd', 'e', ' ', 'a', 'n', 'y', 'w', 'a', 'y', '?',  0 };
static const unichar_t str224[] = { 'B', 'a', 'd', ' ', 't', 'y', 'p', 'e',  0 };
static const unichar_t str225[] = { 'D', 'e', 'l', 'e', 't', 'e',  0 };
static const unichar_t str226[] = { 'D', 'e', 'f', 'a', 'u', 'l', 't',  0 };

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
	NULL};

static const unichar_t pfaedit_ui_mnemonics[] = {
	0x0000, 'O',    'C',    'O',    'S',    'F',    'N',    0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 'F',    'E',    
	'P',    'l',    'i',    'V',    'M',    'W',    'H',    't',    
	'u',    'B',    'M',    'P',    'R',    'a',    'G',    'I',    
	'c',    'e',    'Q',    'F',    'i',    'o',    'N',    'P',    
	'G',    'o',    'o',    'R',    'R',    'e',    'v',    'l',    
	'U',    'R',    't',    'C',    'W',    'o',    'P',    'l',    
	'M',    'A',    'U',    'F',    'P',    'o',    'I',    'A',    
	'B',    'r',    'T',    'E',    'v',    'S',    'I',    'u',    
	'o',    'n',    'C',    'o',    'C',    'T',    'H',    'C',    
	'V',    'D',    'A',    's',    't',    'r',    'e',    'R',    
	't',    'P',    'T',    'L',    'C',    'T',    'W',    'L',    
	'R',    'v',    'M',    '2',    '3',    '4',    'A',    'I',    
	'M',    'I',    'F',    'A',    'D',    'K',    'A',    'P',    
	'O',    'G',    'G',    'B',    'S',    'H',    'V',    'R',    
	'9',    '1',    'S',    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 'F',    'M',    's',    'E',    'L',    
	'k',    'M',    'I',    'P',    'H',    'A',    'D',    'r',    
	'X',    'N',    'G',    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 'D',    0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 'R',    0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 'N',    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 'D',    0x0000, 'S',    0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 'D',    0x0000, 
	0};

static const int pfaedit_ui_num[] = {
    55, 
    0x80000000
};

void PfaEditSetFallback(void) {
    GStringSetFallbackArray(pfaedit_ui_strings,pfaedit_ui_mnemonics,pfaedit_ui_num);
}
