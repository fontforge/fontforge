#include "ttfmodui.h"

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
static const unichar_t str16[] = { 'W', 'i', 'n', 'd', 'o', 'w',  0 };
static const unichar_t str17[] = { 'H', 'e', 'l', 'p',  0 };
static const unichar_t str18[] = { 'R', 'e', 'c', 'e', 'n', 't',  0 };
static const unichar_t str19[] = { 'R', 'e', 'v', 'e', 'r', 't', ' ', 'F', 'i', 'l', 'e',  0 };
static const unichar_t str20[] = { 'S', 'a', 'v', 'e', ' ', 'a', 's', '.', '.', '.',  0 };
static const unichar_t str21[] = { 'C', 'l', 'o', 's', 'e',  0 };
static const unichar_t str22[] = { 'P', 'r', 'e', 'f', 'e', 'r', 'e', 'n', 'c', 'e', 's', '.', '.', '.',  0 };
static const unichar_t str23[] = { 'Q', 'u', 'i', 't',  0 };
static const unichar_t str24[] = { 'U', 'n', 'd', 'o',  0 };
static const unichar_t str25[] = { 'R', 'e', 'd', 'o',  0 };
static const unichar_t str26[] = { 'C', 'u', 't',  0 };
static const unichar_t str27[] = { 'C', 'o', 'p', 'y',  0 };
static const unichar_t str28[] = { 'P', 'a', 's', 't', 'e',  0 };
static const unichar_t str29[] = { 'C', 'l', 'e', 'a', 'r',  0 };
static const unichar_t str30[] = { 'S', 'e', 'l', 'e', 'c', 't', ' ', 'A', 'l', 'l',  0 };
static const unichar_t str31[] = { 'T', 't', 'f', 'M', 'o', 'd',  0 };
static const unichar_t str32[] = { 'O', 'p', 'e', 'n', ' ', 'T', 'T', 'F', '.', '.', '.',  0 };
static const unichar_t str33[] = { 'B', 'a', 'd', ' ', 'n', 'u', 'm', 'b', 'e', 'r', ' ', 'i', 'n', ' ',  0 };
static const unichar_t str34[] = { 'C', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', ' ', 't', 'o', ' ', 'g', 'l', 'y', 'p', 'h', ' ', 'm', 'a', 'p', 'p', 'i', 'n', 'g', 0xa, 'S', 'p', 'e', 'c', 'i', 'f', 'i', 'e', 's', ' ', 't', 'h', 'e', ' ', 'f', 'o', 'n', 't', 0x27, 's', ' ', 'e', 'n', 'c', 'o', 'd', 'i', 'n', 'g',  0 };
static const unichar_t str35[] = { 'G', 'l', 'y', 'p', 'h', ' ', 'd', 'a', 't', 'a', 0xa, 'S', 'p', 'e', 'c', 'i', 'f', 'i', 'e', 's', ' ', 't', 'h', 'e', ' ', 'g', 'l', 'y', 'p', 'h', 's', ' ', 'i', 'n', ' ', 't', 'h', 'e', ' ', 'f', 'o', 'n', 't',  0 };
static const unichar_t str36[] = { 'F', 'o', 'n', 't', ' ', 'h', 'e', 'a', 'd', 'e', 'r', 0xa, 'G', 'e', 'n', 'e', 'r', 'a', 'l', ' ', 'i', 'n', 'f', 'o', 'r', 'm', 'a', 't', 'i', 'o', 'n', ' ', 'a', 'b', 'o', 'u', 't', ' ', 't', 'h', 'e', ' ', 'f', 'o', 'n', 't',  0 };
static const unichar_t str37[] = { 'H', 'o', 'r', 'i', 'z', 'o', 'n', 't', 'a', 'l', ' ', 'h', 'e', 'a', 'd', 'e', 'r', 0xa, 'H', 'i', 'g', 'h', ' ', 'l', 'e', 'v', 'e', 'l', ' ', 'i', 'n', 'f', 'o', 'r', 'm', 'a', 't', 'i', 'o', 'n', ' ', 'a', 'b', 'o', 'u', 't', ' ', 't', 'h', 'e', ' ', 'f', 'o', 'n', 't', 0x27, 's', 0xa, 'h', 'o', 'r', 'i', 'z', 'o', 'n', 't', 'a', 'l', ' ', 'm', 'e', 't', 'r', 'i', 'c', 's',  0 };
static const unichar_t str38[] = { 'H', 'o', 'r', 'i', 'z', 'o', 'n', 't', 'a', 'l', ' ', 'm', 'e', 't', 'r', 'i', 'c', 's', 0xa, 'H', 'o', 'r', 'i', 'z', 'o', 'n', 't', 'a', 'l', ' ', 'm', 'e', 't', 'r', 'i', 'c', ' ', '(', 'l', 'b', 'e', 'a', 'r', 'i', 'n', 'g', '/', 'w', 'i', 'd', 't', 'h', ')', 0xa, 'i', 'n', 'f', 'o', 'r', 'm', 'a', 't', 'i', 'o', 'n', ' ', 'f', 'o', 'r', ' ', 'e', 'a', 'c', 'h', ' ', 'c', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r',  0 };
static const unichar_t str39[] = { 'I', 'n', 'd', 'e', 'x', ' ', 't', 'o', ' ', 'l', 'o', 'c', 'a', 't', 'i', 'o', 'n', 0xa, 'P', 'o', 'i', 'n', 't', 'e', 'r', 's', ' ', 's', 'p', 'e', 'c', 'i', 'f', 'y', 'i', 'n', 'g', ' ', 'w', 'h', 'e', 'r', 'e', ' ', 'e', 'a', 'c', 'h', ' ', 'g', 'l', 'y', 'p', 'h', 0xa, 'b', 'e', 'g', 'i', 'n', 's', ' ', 'i', 'n', ' ', 't', 'h', 'e', ' ', 'g', 'l', 'y', 'f', ' ', 't', 'a', 'b', 'l', 'e',  0 };
static const unichar_t str40[] = { 'M', 'a', 'x', 'i', 'm', 'u', 'm', ' ', 'p', 'r', 'o', 'f', 'i', 'l', 'e', 0xa, 'S', 'p', 'e', 'c', 'i', 'f', 'i', 'e', 's', ' ', 'v', 'a', 'r', 'i', 'o', 'u', 's', ' ', 'm', 'a', 'x', 'i', 'm', 'a', ' ', 'o', 'f', ' ', 't', 'h', 'e', ' ', 'f', 'o', 'n', 't',  0 };
static const unichar_t str41[] = { 'N', 'a', 'm', 'i', 'n', 'g', ' ', 't', 'a', 'b', 'l', 'e', 0xa, 'S', 'p', 'e', 'c', 'i', 'f', 'i', 'e', 's', ' ', 't', 'h', 'e', ' ', 'f', 'o', 'n', 't', ' ', 'n', 'a', 'm', 'e', ',', ' ', 'c', 'o', 'p', 'y', 'r', 'i', 'g', 'h', 't', ',', ' ', 'e', 't', 'c', '.',  0 };
static const unichar_t str42[] = { 'P', 'o', 's', 't', 'S', 'c', 'r', 'i', 'p', 't', ' ', 'd', 'a', 't', 'a', 0xa, 'S', 'p', 'e', 'c', 'i', 'f', 'i', 'e', 's', ' ', 'P', 'o', 's', 't', 'S', 'c', 'r', 'i', 'p', 't', ' ', 's', 'p', 'e', 'c', 'i', 'f', 'i', 'c', ' ', 'd', 'a', 't', 'a', 0xa, 'a', 'n', 'd', ' ', 'P', 'o', 's', 't', 'S', 'c', 'r', 'i', 'p', 't', ' ', 'g', 'l', 'y', 'p', 'h', ' ', 'n', 'a', 'm', 'e', 's',  0 };
static const unichar_t str43[] = { 'O', 'S', '/', '2', ' ', 'a', 'n', 'd', ' ', 'W', 'i', 'n', 'd', 'o', 'w', 's', ' ', 's', 'p', 'e', 'c', 'i', 'f', 'i', 'c', ' ', 'm', 'e', 't', 'r', 'i', 'c', 's',  0 };
static const unichar_t str44[] = { 'C', 'o', 'n', 't', 'r', 'o', 'l', ' ', 'V', 'a', 'l', 'u', 'e', ' ', 'T', 'a', 'b', 'l', 'e', 0xa, 'V', 'a', 'r', 'i', 'o', 'u', 's', ' ', 'i', 'n', 't', 'e', 'g', 'e', 'r', 's', ' ', 'u', 's', 'e', 'd', ' ', 'f', 'o', 'r', ' ', 'i', 'n', 's', 't', 'r', 'u', 'c', 't', 'i', 'n', 'g', 0xa, '(', 'h', 'i', 'n', 't', 'i', 'n', 'g', ')', ' ', 't', 'h', 'e', ' ', 'f', 'o', 'n', 't',  0 };
static const unichar_t str45[] = { 'E', 'm', 'b', 'e', 'd', 'd', 'e', 'd', ' ', 'b', 'i', 't', 'm', 'a', 'p', ' ', 'd', 'a', 't', 'a',  0 };
static const unichar_t str46[] = { 'E', 'm', 'b', 'e', 'd', 'd', 'e', 'd', ' ', 'b', 'i', 't', 'm', 'a', 'p', ' ', 'l', 'o', 'c', 'a', 't', 'i', 'o', 'n', ' ', 'd', 'a', 't', 'a',  0 };
static const unichar_t str47[] = { 'E', 'm', 'b', 'e', 'd', 'd', 'e', 'd', ' ', 'b', 'i', 't', 'm', 'a', 'p', ' ', 's', 'c', 'a', 'l', 'i', 'n', 'g', ' ', 'd', 'a', 't', 'a',  0 };
static const unichar_t str48[] = { 'F', 'o', 'n', 't', ' ', 'P', 'r', 'o', 'g', 'r', 'a', 'm', 0xa, 'I', 'n', 'v', 'o', 'k', 'e', 'd', ' ', 'b', 'e', 'f', 'o', 'r', 'e', ' ', 't', 'h', 'e', ' ', 'f', 'i', 'r', 's', 't', ' ', 'u', 's', 'e', ' ', 'o', 'f', ' ', 't', 'h', 'e', ' ', 'f', 'o', 'n', 't', '.',  0 };
static const unichar_t str49[] = { 'g', 'r', 'i', 'd', '-', 'f', 'i', 't', 't', 'i', 'n', 'g', ' ', 'a', 'n', 'd', ' ', 's', 'c', 'a', 'n', ' ', 'c', 'o', 'n', 'v', 'e', 'r', 's', 'i', 'o', 'n', ' ', 'p', 'r', 'o', 'c', 'e', 'd', 'u', 'r', 'e', ' ', '(', 'g', 'r', 'e', 'y', 's', 'c', 'a', 'l', 'e', ')',  0 };
static const unichar_t str50[] = { 'H', 'o', 'r', 'i', 'z', 'o', 'n', 't', 'a', 'l', ' ', 'd', 'e', 'v', 'i', 'c', 'e', ' ', 'm', 'e', 't', 'r', 'i', 'c', 's', 0xa, 'P', 'r', 'e', 'c', 'o', 'm', 'p', 'u', 't', 'e', 'd', ' ', 'm', 'e', 't', 'r', 'i', 'c', 's', ' ', 'f', 'o', 'r', ' ', 'c', 'e', 'r', 't', 'a', 'i', 'n', ' ', 'p', 'o', 'i', 'n', 't', '-', 's', 'i', 'z', 'e', 's',  0 };
static const unichar_t str51[] = { 'K', 'e', 'r', 'n', 'i', 'n', 'g', 0xa, 'K', 'e', 'r', 'n', 'i', 'n', 'g', ' ', 'p', 'a', 'i', 'r', 's',  0 };
static const unichar_t str52[] = { 'L', 'i', 'n', 'e', 'a', 'r', ' ', 't', 'h', 'r', 'e', 's', 'h', 'o', 'l', 'd', ' ', 't', 'a', 'b', 'l', 'e', 0xa, 'S', 'p', 'e', 'c', 'i', 'f', 'i', 'e', 's', ' ', 'p', 'o', 'i', 'n', 't', ' ', 'a', 't', ' ', 'w', 'h', 'i', 'c', 'h', ' ', 'm', 'e', 't', 'r', 'i', 'c', 's', ' ', 'm', 'a', 'y', ' ', 'b', 'e', 0xa, 'a', 's', 's', 'u', 'm', 'e', 'd', ' ', 't', 'o', ' ', 's', 'c', 'a', 'l', 'e', ' ', 'f', 'r', 'o', 'm', ' ', 'h', 't', 'm', 'x', ' ', 't', 'a', 'b', 'l', 'e',  0 };
static const unichar_t str53[] = { 'C', 'V', 'T', ' ', 'P', 'r', 'o', 'g', 'r', 'a', 'm', 0xa, 'I', 'n', 'v', 'o', 'k', 'e', 'd', ' ', 'b', 'e', 'f', 'o', 'r', 'e', ' ', 'e', 'a', 'c', 'h', ' ', 't', 'i', 'm', 'e', ' ', 't', 'h', 'e', ' ', 'f', 'o', 'n', 't', ' ', 'i', 's', ' ', 'r', 'a', 's', 't', 'e', 'r', 'i', 'z', 'e', 'd', '.',  0 };
static const unichar_t str54[] = { 'P', 'C', 'L', '5', 0xa, 'I', 'n', 'f', 'o', 'r', 'm', 'a', 't', 'i', 'o', 'n', ' ', 'f', 'o', 'r', ' ', 'H', 'P', ' ', 'p', 'r', 'i', 'n', 't', 'e', 'r', 's',  0 };
static const unichar_t str55[] = { 'V', 'e', 'r', 't', 'i', 'c', 'a', 'l', ' ', 'D', 'e', 'v', 'i', 'c', 'e', ' ', 'M', 'e', 't', 'r', 'i', 'c', 's', ' ', 't', 'a', 'b', 'l', 'e', 0xa, 'P', 'r', 'e', 'c', 'o', 'm', 'p', 'u', 't', 'e', 'd', ' ', 'm', 'e', 't', 'r', 'i', 'c', 's', ' ', 'f', 'o', 'r', ' ', 'c', 'e', 'r', 't', 'a', 'i', 'n', ' ', 'p', 'o', 'i', 'n', 't', '-', 's', 'i', 'z', 'e', 's',  0 };
static const unichar_t str56[] = { 'V', 'e', 'r', 't', 'i', 'c', 'a', 'l', ' ', 'h', 'e', 'a', 'd', 'e', 'r', 0xa, 'H', 'i', 'g', 'h', ' ', 'l', 'e', 'v', 'e', 'l', ' ', 'i', 'n', 'f', 'o', 'r', 'm', 'a', 't', 'i', 'o', 'n', ' ', 'a', 'b', 'o', 'u', 't', ' ', 't', 'h', 'e', ' ', 'f', 'o', 'n', 't', 0x27, 's', 0xa, 'v', 'e', 'r', 't', 'i', 'c', 'a', 'l', ' ', 'm', 'e', 't', 'r', 'i', 'c', 's', ' ', '(', 'f', 'o', 'r', ' ', 'C', 'J', 'K', ' ', 'f', 'o', 'n', 't', 's', ')',  0 };
static const unichar_t str57[] = { 'V', 'e', 'r', 't', 'i', 'c', 'a', 'l', ' ', 'm', 'e', 't', 'r', 'i', 'c', 's',  0 };
static const unichar_t str58[] = { 'C', 'o', 'm', 'p', 'a', 'c', 't', ' ', 'F', 'o', 'n', 't', ' ', 'F', 'o', 'r', 'm', 'a', 't', 0xa, 'P', 'o', 's', 't', 's', 'c', 'r', 'i', 'p', 't', ' ', 'o', 'u', 't', 'l', 'i', 'n', 'e', 's',  0 };
static const unichar_t str59[] = { 'O', 'b', 's', 'o', 'l', 'e', 't', 'e', ' ', 'm', 'u', 'l', 't', 'i', 'p', 'l', 'e', ' ', 'm', 'a', 's', 't', 'e', 'r', ' ', 't', 'a', 'b', 'l', 'e',  0 };
static const unichar_t str60[] = { 'O', 'b', 's', 'o', 'l', 'e', 't', 'e', ' ', 'm', 'u', 'l', 't', 'i', 'p', 'l', 'e', ' ', 'm', 'a', 's', 't', 'e', 'r', ' ', 't', 'a', 'b', 'l', 'e',  0 };
static const unichar_t str61[] = { 'O', 'b', 's', 'o', 'l', 'e', 't', 'e', ' ', 'm', 'u', 'l', 't', 'i', 'p', 'l', 'e', ' ', 'm', 'a', 's', 't', 'e', 'r', ' ', 't', 'a', 'b', 'l', 'e',  0 };
static const unichar_t str62[] = { 'B', 'a', 's', 'e', 'l', 'i', 'n', 'e', ' ', 'd', 'a', 't', 'a', 0xa, 'S', 'p', 'e', 'c', 'i', 'f', 'i', 'e', 's', ' ', 'h', 'o', 'w', ' ', 't', 'o', ' ', 'a', 'l', 'i', 'g', 'n', ' ', 'g', 'l', 'y', 'p', 'h', 's', ' ', 'f', 'r', 'o', 'm', ' ', 'd', 'i', 'f', 'f', 'e', 'r', 'e', 'n', 't', ' ', 's', 'c', 'r', 'i', 'p', 't', 's',  0 };
static const unichar_t str63[] = { 'G', 'l', 'y', 'p', 'h', ' ', 'd', 'e', 'f', 'i', 'n', 'i', 't', 'i', 'o', 'n', ' ', 'd', 'a', 't', 'a', 0xa, 'S', 'p', 'e', 'c', 'i', 'f', 'i', 'e', 's', ' ', 'c', 'a', 'r', 'e', 't', ' ', 'l', 'o', 'c', 'a', 't', 'i', 'o', 'n', 's', ' ', 'i', 'n', 's', 'i', 'd', 'e', ' ', 'l', 'i', 'g', 'a', 't', 'u', 'r', 'e', 's', 0xa, 'a', 'n', 'd', ' ', 'a', 't', 't', 'a', 'c', 'h', 'm', 'e', 'n', 't', ' ', 'p', 'o', 'i', 'n', 't', 's',  0 };
static const unichar_t str64[] = { 'G', 'l', 'y', 'p', 'h', ' ', 'p', 'o', 's', 'i', 't', 'i', 'o', 'n', 'i', 'n', 'g', ' ', 'd', 'a', 't', 'a', 0xa, 'S', 'p', 'e', 'c', 'i', 'f', 'i', 'e', 's', ' ', 'h', 'o', 'w', ' ', 't', 'o', ' ', 'p', 'o', 's', 'i', 't', 'i', 'o', 'n', 's', ' ', 'g', 'l', 'y', 'p', 'h', 's', ' ', 'w', 'i', 't', 'h', 0xa, 'r', 'e', 's', 'p', 'e', 'c', 't', ' ', 't', 'o', ' ', 'e', 'a', 'c', 'h', ' ', 'o', 't', 'h', 'e', 'r', ' ', 'i', 'n', ' ', 'c', 'a', 'l', 'i', 'g', 'r', 'a', 'p', 'h', 'i', 'c', ' ', 'a', 'n', 'd', ' ', 'o', 't', 'h', 'e', 'r', 0xa, 'c', 'o', 'm', 'p', 'l', 'e', 'x', ' ', 's', 'c', 'r', 'i', 'p', 't', 's',  0 };
static const unichar_t str65[] = { 'G', 'l', 'y', 'p', 'h', ' ', 's', 'u', 'b', 's', 't', 'i', 't', 'u', 't', 'i', 'o', 'n', ' ', 'd', 'a', 't', 'a', 0xa, 'S', 'p', 'e', 'c', 'i', 'f', 'i', 'e', 's', ' ', 'l', 'i', 'g', 'a', 't', 'u', 'r', 'e', 's', ',', ' ', 'a', 'l', 't', 'e', 'r', 'n', 'a', 't', 'e', ' ', 'g', 'l', 'y', 'p', 'h', 's', ',', 0xa, 'p', 'o', 's', 'i', 't', 'i', 'o', 'n', 'a', 'l', ' ', 'v', 'a', 'r', 'i', 'a', 'n', 't', 's', ',', ' ', 'c', 'o', 'n', 't', 'e', 'x', 't', 'u', 'a', 'l', ' ', 's', 'u', 'b', 's', 't', 'i', 't', 'u', 't', 'i', 'o', 'n', 's',  0 };
static const unichar_t str66[] = { 'J', 'u', 's', 't', 'i', 'f', 'i', 'c', 'a', 't', 'i', 'o', 'n', ' ', 'd', 'a', 't', 'a', 0xa, 'A', 'n', 'o', 't', 'h', 'e', 'r', ' ', 's', 'e', 't', ' ', 'o', 'f', ' ', 'g', 'l', 'y', 'p', 'h', ' ', 's', 'u', 'b', 's', 't', 'i', 't', 'u', 't', 'i', 'o', 'n', 's', 0xa, 's', 'p', 'e', 'c', 'i', 'f', 'i', 'c', 'a', 'l', 'l', 'y', ' ', 'f', 'o', 'r', ' ', 'j', 'u', 's', 't', 'i', 'f', 'i', 'c', 'a', 't', 'i', 'o', 'n',  0 };
static const unichar_t str67[] = { 'D', 'i', 'g', 'i', 't', 'a', 'l', ' ', 'S', 'i', 'g', 'n', 'a', 't', 'u', 'r', 'e',  0 };
static const unichar_t str68[] = { 'V', 'e', 'r', 't', 'i', 'c', 'a', 'l', ' ', 'O', 'r', 'i', 'g', 'i', 'n',  0 };
static const unichar_t str69[] = { 'V', 'e', 'r', 's', 'i', 'o', 'n', ':',  0 };
static const unichar_t str70[] = { 'G', 'l', 'y', 'p', 'h', 's', ':',  0 };
static const unichar_t str71[] = { 'P', 'o', 'i', 'n', 't', 's', ':',  0 };
static const unichar_t str72[] = { 'C', 'o', 'n', 't', 'o', 'u', 'r', 's', ':',  0 };
static const unichar_t str73[] = { 'C', 'o', 'm', 'p', 'o', 's', 'i', 't', ' ', 'P', 't', 's', ':',  0 };
static const unichar_t str74[] = { 'C', 'o', 'm', 'p', 'o', 's', 'i', 't', ' ', 'C', 'n', 't', 'r', 's', ':',  0 };
static const unichar_t str75[] = { 'Z', 'o', 'n', 'e', 's', ':',  0 };
static const unichar_t str76[] = { 'T', 'w', 'i', 'l', 'i', 'g', 'h', 't', ' ', 'P', 't', 's', ':',  0 };
static const unichar_t str77[] = { 'S', 't', 'o', 'r', 'a', 'g', 'e', ':',  0 };
static const unichar_t str78[] = { 'F', 'D', 'E', 'F', 's', ':',  0 };
static const unichar_t str79[] = { 'I', 'D', 'E', 'F', 's', ':',  0 };
static const unichar_t str80[] = { 'S', 't', 'a', 'c', 'k', ' ', 'E', 'l', 's', ':',  0 };
static const unichar_t str81[] = { 'S', 'i', 'z', 'e', ' ', 'O', 'f', ' ', 'I', 'n', 's', 't', 'r', 's', ':',  0 };
static const unichar_t str82[] = { 'C', 'o', 'm', 'p', 'o', 'n', 'e', 'n', 't', ' ', 'E', 'l', 's', ':',  0 };
static const unichar_t str83[] = { 'C', 'D', 'e', 'p', 't', 'h', ':',  0 };

static const unichar_t *ttfmod_ui_strings[] = {
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
	NULL};

static const unichar_t ttfmod_ui_mnemonics[] = {
	0x0000, 'O',    'C',    'O',    'S',    'F',    'N',    0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 'F',    'E',    
	'W',    'H',    't',    'R',    'a',    'c',    'e',    'Q',    
	'U',    'R',    't',    'C',    'P',    'l',    'A',    0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 'V',    'G',    'P',    
	'C',    'n',    'r',    'Z',    'T',    'S',    'F',    'I',    
	'E',    'u',    'l',    'D',    
	0};

static const int ttfmod_ui_num[] = {
    55, 
    0x80000000
};

void TtfModSetFallback(void) {
    GStringSetFallbackArray(ttfmod_ui_strings,ttfmod_ui_mnemonics,ttfmod_ui_num);
}
