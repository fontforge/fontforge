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
static const unichar_t str84[] = { 'G', 'e', 'n', 'e', 'r', 'a', 'l',  0 };
static const unichar_t str85[] = { 'A', 'l', 'i', 'g', 'n', 'm', 'e', 'n', 't',  0 };
static const unichar_t str86[] = { 'P', 'a', 'n', 'o', 's', 'e',  0 };
static const unichar_t str87[] = { 'C', 'h', 'a', 'r', 's', 'e', 't', 's',  0 };
static const unichar_t str88[] = { 'A', 'v', 'e', 'r', 'a', 'g', 'e', ' ', 'W', 'i', 'd', 't', 'h', ':',  0 };
static const unichar_t str89[] = { 'U', 'l', 't', 'r', 'a', '-', 'C', 'o', 'n', 'd', 'e', 'n', 's', 'e', 'd', ' ', '(', '5', '0', '%', ')',  0 };
static const unichar_t str90[] = { 'E', 'x', 't', 'r', 'a', '-', 'C', 'o', 'n', 'd', 'e', 'n', 's', 'e', 'd', ' ', '(', '6', '2', '.', '5', '%', ')',  0 };
static const unichar_t str91[] = { 'C', 'o', 'n', 'd', 'e', 'n', 's', 'e', 'd', ' ', '(', '7', '5', '%', ')',  0 };
static const unichar_t str92[] = { 'S', 'e', 'm', 'i', '-', 'C', 'o', 'n', 'd', 'e', 'n', 's', 'e', 'd', ' ', '(', '8', '7', '.', '5', '%', ')',  0 };
static const unichar_t str93[] = { 'M', 'e', 'd', 'i', 'u', 'm', ' ', '(', '1', '0', '0', '%', ')',  0 };
static const unichar_t str94[] = { 'S', 'e', 'm', 'i', '-', 'E', 'x', 'p', 'a', 'n', 'd', 'e', 'd', ' ', '(', '1', '1', '2', '.', '5', '%', ')',  0 };
static const unichar_t str95[] = { 'E', 'x', 'p', 'a', 'n', 'd', 'e', 'd', ' ', '(', '1', '2', '5', '%', ')',  0 };
static const unichar_t str96[] = { 'E', 'x', 't', 'r', 'a', '-', 'E', 'x', 'p', 'a', 'n', 'd', 'e', 'd', ' ', '(', '1', '5', '0', '%', ')',  0 };
static const unichar_t str97[] = { 'U', 'l', 't', 'r', 'a', '-', 'E', 'x', 'p', 'a', 'n', 'd', 'e', 'd', ' ', '(', '2', '0', '0', '%', ')',  0 };
static const unichar_t str98[] = { '1', '0', '0', ' ', 'T', 'h', 'i', 'n',  0 };
static const unichar_t str99[] = { '2', '0', '0', ' ', 'E', 'x', 't', 'r', 'a', '-', 'L', 'i', 'g', 'h', 't',  0 };
static const unichar_t str100[] = { '3', '0', '0', ' ', 'L', 'i', 'g', 'h', 't',  0 };
static const unichar_t str101[] = { '4', '0', '0', ' ', 'B', 'o', 'o', 'k',  0 };
static const unichar_t str102[] = { '5', '0', '0', ' ', 'M', 'e', 'd', 'i', 'u', 'm',  0 };
static const unichar_t str103[] = { '6', '0', '0', ' ', 'D', 'e', 'm', 'i', '-', 'B', 'o', 'l', 'd',  0 };
static const unichar_t str104[] = { '7', '0', '0', ' ', 'B', 'o', 'l', 'd',  0 };
static const unichar_t str105[] = { '8', '0', '0', ' ', 'H', 'e', 'a', 'v', 'y',  0 };
static const unichar_t str106[] = { '9', '0', '0', ' ', 'B', 'l', 'a', 'c', 'k',  0 };
static const unichar_t str107[] = { 'C', 'a', 'n', ' ', 't', 'h', 'i', 's', ' ', 'f', 'o', 'n', 't', ' ', 'b', 'e', ' ', 'e', 'm', 'b', 'e', 'd', 'd', 'e', 'd', ' ', 'i', 'n', ' ', 'a', ' ', 'd', 'o', 'w', 'n', 'l', 'o', 'a', 'd', 'a', 'b', 'l', 'e', ' ', '(', 'p', 'd', 'f', ')', 0xa, 'd', 'o', 'c', 'u', 'm', 'e', 'n', 't', ' ', 'a', 'n', 'd', ' ', 'i', 'f', ' ', 's', 'o', ' ', 'w', 'h', 'a', 't', ' ', 'b', 'e', 'h', 'a', 'v', 'i', 'o', 'r', 's', ' ', 'a', 'r', 'e', ' ', 'p', 'e', 'r', 'm', 'i', 't', 't', 'e', 'd', ' ', 'o', 'n', 0xa, 'b', 'o', 't', 'h', ' ', 't', 'h', 'e', ' ', 'd', 'o', 'c', 'u', 'm', 'e', 'n', 't', ' ', 'a', 'n', 'd', ' ', 't', 'h', 'e', ' ', 'f', 'o', 'n', 't', '.',  0 };
static const unichar_t str108[] = { 'E', 'm', 'b', 'e', 'd', 'd', 'a', 'b', 'l', 'e', ':',  0 };
static const unichar_t str109[] = { 'N', 'e', 'v', 'e', 'r', ' ', 'E', 'm', 'b', 'e', 'd', '/', 'N', 'o', ' ', 'E', 'd', 'i', 't', 'i', 'n', 'g',  0 };
static const unichar_t str110[] = { 'P', 'r', 'i', 'n', 't', 'a', 'b', 'l', 'e', ' ', 'D', 'o', 'c', 'u', 'm', 'e', 'n', 't',  0 };
static const unichar_t str111[] = { 'E', 'd', 'i', 't', 'a', 'b', 'l', 'e', ' ', 'D', 'o', 'c', 'u', 'm', 'e', 'n', 't',  0 };
static const unichar_t str112[] = { 'I', 'n', 's', 't', 'a', 'l', 'l', 'a', 'b', 'l', 'e', ' ', 'F', 'o', 'n', 't',  0 };
static const unichar_t str113[] = { 'S', 'e', 'r', 'i', 'f',  0 };
static const unichar_t str114[] = { 'S', 'a', 'n', 's', '-', 'S', 'e', 'r', 'i', 'f',  0 };
static const unichar_t str115[] = { 'M', 'o', 'n', 'o', 's', 'p', 'a', 'c', 'e',  0 };
static const unichar_t str116[] = { 'S', 'c', 'r', 'i', 'p', 't',  0 };
static const unichar_t str117[] = { 'D', 'e', 'c', 'o', 'r', 'a', 't', 'i', 'v', 'e',  0 };
static const unichar_t str118[] = { 'A', 'n', 'y',  0 };
static const unichar_t str119[] = { 'N', 'o', ' ', 'F', 'i', 't',  0 };
static const unichar_t str120[] = { 'T', 'e', 'x', 't', ' ', '&', ' ', 'D', 'i', 's', 'p', 'l', 'a', 'y',  0 };
static const unichar_t str121[] = { 'P', 'i', 'c', 't', 'o', 'r', 'a', 'l',  0 };
static const unichar_t str122[] = { 'C', 'o', 'v', 'e',  0 };
static const unichar_t str123[] = { 'O', 'b', 't', 'u', 's', 'e', ' ', 'C', 'o', 'v', 'e',  0 };
static const unichar_t str124[] = { 'S', 'q', 'u', 'a', 'r', 'e', ' ', 'C', 'o', 'v', 'e',  0 };
static const unichar_t str125[] = { 'O', 'b', 't', 'u', 's', 'e', ' ', 'S', 'q', 'u', 'a', 'r', 'e', ' ', 'C', 'o', 'v', 'e',  0 };
static const unichar_t str126[] = { 'S', 'q', 'u', 'a', 'r', 'e',  0 };
static const unichar_t str127[] = { 'T', 'h', 'i', 'n',  0 };
static const unichar_t str128[] = { 'B', 'o', 'n', 'e',  0 };
static const unichar_t str129[] = { 'E', 'x', 'a', 'g', 'g', 'e', 'r', 'a', 't', 'e', 'd',  0 };
static const unichar_t str130[] = { 'T', 'r', 'i', 'a', 'n', 'g', 'l', 'e',  0 };
static const unichar_t str131[] = { 'N', 'o', 'r', 'm', 'a', 'l', ' ', 'S', 'a', 'n', 's',  0 };
static const unichar_t str132[] = { 'O', 'b', 't', 'u', 's', 'e', ' ', 'S', 'a', 'n', 's',  0 };
static const unichar_t str133[] = { 'P', 'e', 'r', 'p', ' ', 'S', 'a', 'n', 's',  0 };
static const unichar_t str134[] = { 'F', 'l', 'a', 'r', 'e', 'd',  0 };
static const unichar_t str135[] = { 'R', 'o', 'u', 'n', 'd', 'e', 'd',  0 };
static const unichar_t str136[] = { 'V', 'e', 'r', 'y', ' ', 'L', 'i', 'g', 'h', 't',  0 };
static const unichar_t str137[] = { 'L', 'i', 'g', 'h', 't',  0 };
static const unichar_t str138[] = { 'B', 'o', 'o', 'k',  0 };
static const unichar_t str139[] = { 'M', 'e', 'd', 'i', 'u', 'm',  0 };
static const unichar_t str140[] = { 'D', 'e', 'm', 'i',  0 };
static const unichar_t str141[] = { 'B', 'o', 'l', 'd',  0 };
static const unichar_t str142[] = { 'H', 'e', 'a', 'v', 'y',  0 };
static const unichar_t str143[] = { 'B', 'l', 'a', 'c', 'k',  0 };
static const unichar_t str144[] = { 'N', 'o', 'r', 'd',  0 };
static const unichar_t str145[] = { 'O', 'l', 'd', ' ', 'S', 't', 'y', 'l', 'e',  0 };
static const unichar_t str146[] = { 'M', 'o', 'd', 'e', 'r', 'n',  0 };
static const unichar_t str147[] = { 'E', 'v', 'e', 'n', ' ', 'W', 'i', 'd', 't', 'h',  0 };
static const unichar_t str148[] = { 'E', 'x', 'p', 'a', 'n', 'd', 'e', 'd',  0 };
static const unichar_t str149[] = { 'C', 'o', 'n', 'd', 'e', 'n', 's', 'e', 'd',  0 };
static const unichar_t str150[] = { 'V', 'e', 'r', 'y', ' ', 'E', 'x', 'p', 'a', 'n', 'd', 'e', 'd',  0 };
static const unichar_t str151[] = { 'V', 'e', 'r', 'y', ' ', 'C', 'o', 'n', 'd', 'e', 'n', 's', 'e', 'd',  0 };
static const unichar_t str152[] = { 'M', 'o', 'n', 'o', 's', 'p', 'a', 'c', 'e', 'd',  0 };
static const unichar_t str153[] = { 'N', 'o', 'n', 'e',  0 };
static const unichar_t str154[] = { 'V', 'e', 'r', 'y', ' ', 'L', 'o', 'w',  0 };
static const unichar_t str155[] = { 'L', 'o', 'w',  0 };
static const unichar_t str156[] = { 'M', 'e', 'd', 'i', 'u', 'm', ' ', 'L', 'o', 'w',  0 };
static const unichar_t str157[] = { 'M', 'e', 'd', 'i', 'u', 'm', ' ', 'H', 'i', 'g', 'h',  0 };
static const unichar_t str158[] = { 'H', 'i', 'g', 'h',  0 };
static const unichar_t str159[] = { 'V', 'e', 'r', 'y', ' ', 'H', 'i', 'g', 'h',  0 };
static const unichar_t str160[] = { 'G', 'r', 'a', 'd', 'u', 'a', 'l', '/', 'D', 'i', 'a', 'g', 'o', 'n', 'a', 'l',  0 };
static const unichar_t str161[] = { 'G', 'r', 'a', 'd', 'u', 'a', 'l', '/', 'T', 'r', 'a', 'n', 's', 'i', 't', 'i', 'o', 'n', 'a', 'l',  0 };
static const unichar_t str162[] = { 'G', 'r', 'a', 'd', 'u', 'a', 'l', '/', 'V', 'e', 'r', 't', 'i', 'c', 'a', 'l',  0 };
static const unichar_t str163[] = { 'G', 'r', 'a', 'd', 'u', 'a', 'l', '/', 'H', 'o', 'r', 'i', 'z', 'o', 'n', 't', 'a', 'l',  0 };
static const unichar_t str164[] = { 'R', 'a', 'p', 'i', 'd', '/', 'V', 'e', 'r', 't', 'i', 'c', 'a', 'l',  0 };
static const unichar_t str165[] = { 'R', 'a', 'p', 'i', 'd', '/', 'H', 'o', 'r', 'i', 'z', 'o', 'n', 't', 'a', 'l',  0 };
static const unichar_t str166[] = { 'I', 'n', 's', 't', 'a', 'n', 't', '/', 'V', 'e', 'r', 't', 'i', 'c', 'a', 'l',  0 };
static const unichar_t str167[] = { 'N', 'o', 'r', 'm', 'a', 'l', '/', 'C', 'o', 'n', 't', 'a', 'c', 't',  0 };
static const unichar_t str168[] = { 'N', 'o', 'r', 'm', 'a', 'l', '/', 'W', 'e', 'i', 'g', 'h', 't', 'e', 'd',  0 };
static const unichar_t str169[] = { 'N', 'o', 'r', 'm', 'a', 'l', '/', 'B', 'o', 'x', 'e', 'd',  0 };
static const unichar_t str170[] = { 'N', 'o', 'r', 'm', 'a', 'l', '/', 'F', 'l', 'a', 't', 't', 'e', 'n', 'e', 'd',  0 };
static const unichar_t str171[] = { 'N', 'o', 'r', 'm', 'a', 'l', '/', 'R', 'o', 'u', 'n', 'd', 'e', 'd',  0 };
static const unichar_t str172[] = { 'N', 'o', 'r', 'm', 'a', 'l', '/', 'O', 'f', 'f', '-', 'C', 'e', 'n', 't', 'e', 'r',  0 };
static const unichar_t str173[] = { 'N', 'o', 'r', 'm', 'a', 'l', '/', 'S', 'q', 'u', 'a', 'r', 'e',  0 };
static const unichar_t str174[] = { 'O', 'b', 'l', 'i', 'q', 'u', 'e', '/', 'C', 'o', 'n', 't', 'a', 'c', 't',  0 };
static const unichar_t str175[] = { 'O', 'b', 'l', 'i', 'q', 'u', 'e', '/', 'W', 'e', 'i', 'g', 'h', 't', 'e', 'd',  0 };
static const unichar_t str176[] = { 'O', 'b', 'l', 'i', 'q', 'u', 'e', '/', 'B', 'o', 'x', 'e', 'd',  0 };
static const unichar_t str177[] = { 'O', 'b', 'l', 'i', 'q', 'u', 'e', '/', 'F', 'l', 'a', 't', 't', 'e', 'n', 'e', 'd',  0 };
static const unichar_t str178[] = { 'O', 'b', 'l', 'i', 'q', 'u', 'e', '/', 'R', 'o', 'u', 'n', 'd', 'e', 'd',  0 };
static const unichar_t str179[] = { 'O', 'b', 'l', 'i', 'q', 'u', 'e', '/', 'O', 'f', 'f', '-', 'C', 'e', 'n', 't', 'e', 'r',  0 };
static const unichar_t str180[] = { 'O', 'b', 'l', 'i', 'q', 'u', 'e', '/', 'S', 'q', 'u', 'a', 'r', 'e',  0 };
static const unichar_t str181[] = { 'S', 't', 'r', 'a', 'i', 'g', 'h', 't', ' ', 'A', 'r', 'm', 's', '/', 'H', 'o', 'r', 'i', 'z', 'o', 'n', 't', 'a', 'l',  0 };
static const unichar_t str182[] = { 'S', 't', 'r', 'a', 'i', 'g', 'h', 't', ' ', 'A', 'r', 'm', 's', '/', 'W', 'e', 'd', 'g', 'e',  0 };
static const unichar_t str183[] = { 'S', 't', 'r', 'a', 'i', 'g', 'h', 't', ' ', 'A', 'r', 'm', 's', '/', 'V', 'e', 'r', 't', 'i', 'c', 'a', 'l',  0 };
static const unichar_t str184[] = { 'S', 't', 'r', 'a', 'i', 'g', 'h', 't', ' ', 'A', 'r', 'm', 's', '/', 'S', 'i', 'n', 'g', 'l', 'e', ' ', 'S', 'e', 'r', 'i', 'f',  0 };
static const unichar_t str185[] = { 'S', 't', 'r', 'a', 'i', 'g', 'h', 't', ' ', 'A', 'r', 'm', 's', '/', 'D', 'o', 'u', 'b', 'l', 'e', ' ', 'S', 'e', 'r', 'i', 'f',  0 };
static const unichar_t str186[] = { 'N', 'o', 'n', '-', 'S', 't', 'r', 'a', 'i', 'g', 'h', 't', ' ', 'A', 'r', 'm', 's', '/', 'H', 'o', 'r', 'i', 'z', 'o', 'n', 't', 'a', 'l',  0 };
static const unichar_t str187[] = { 'N', 'o', 'n', '-', 'S', 't', 'r', 'a', 'i', 'g', 'h', 't', ' ', 'A', 'r', 'm', 's', '/', 'W', 'e', 'd', 'g', 'e',  0 };
static const unichar_t str188[] = { 'N', 'o', 'n', '-', 'S', 't', 'r', 'a', 'i', 'g', 'h', 't', ' ', 'A', 'r', 'm', 's', '/', 'V', 'e', 'r', 't', 'i', 'c', 'a', 'l',  0 };
static const unichar_t str189[] = { 'N', 'o', 'n', '-', 'S', 't', 'r', 'a', 'i', 'g', 'h', 't', ' ', 'A', 'r', 'm', 's', '/', 'S', 'i', 'n', 'g', 'l', 'e', ' ', 'S', 'e', 'r', 'i', 'f',  0 };
static const unichar_t str190[] = { 'N', 'o', 'n', '-', 'S', 't', 'r', 'a', 'i', 'g', 'h', 't', ' ', 'A', 'r', 'm', 's', '/', 'D', 'o', 'u', 'b', 'l', 'e', ' ', 'S', 'e', 'r', 'i', 'f',  0 };
static const unichar_t str191[] = { 'S', 't', 'a', 'n', 'd', 'a', 'r', 'd', '/', 'T', 'r', 'i', 'm', 'm', 'e', 'd',  0 };
static const unichar_t str192[] = { 'S', 't', 'a', 'n', 'd', 'a', 'r', 'd', '/', 'P', 'o', 'i', 'n', 't', 'e', 'd',  0 };
static const unichar_t str193[] = { 'S', 't', 'a', 'n', 'd', 'a', 'r', 'd', '/', 'S', 'e', 'r', 'i', 'f', 'e', 'd',  0 };
static const unichar_t str194[] = { 'H', 'i', 'g', 'h', '/', 'T', 'r', 'i', 'm', 'm', 'e', 'd',  0 };
static const unichar_t str195[] = { 'H', 'i', 'g', 'h', '/', 'P', 'o', 'i', 'n', 't', 'e', 'd',  0 };
static const unichar_t str196[] = { 'H', 'i', 'g', 'h', '/', 'S', 'e', 'r', 'i', 'f', 'e', 'd',  0 };
static const unichar_t str197[] = { 'C', 'o', 'n', 's', 't', 'a', 'n', 't', '/', 'T', 'r', 'i', 'm', 'm', 'e', 'd',  0 };
static const unichar_t str198[] = { 'C', 'o', 'n', 's', 't', 'a', 'n', 't', '/', 'P', 'o', 'i', 'n', 't', 'e', 'd',  0 };
static const unichar_t str199[] = { 'C', 'o', 'n', 's', 't', 'a', 'n', 't', '/', 'S', 'e', 'r', 'i', 'f', 'e', 'd',  0 };
static const unichar_t str200[] = { 'L', 'o', 'w', '/', 'T', 'r', 'i', 'm', 'm', 'e', 'd',  0 };
static const unichar_t str201[] = { 'L', 'o', 'w', '/', 'P', 'o', 'i', 'n', 't', 'e', 'd',  0 };
static const unichar_t str202[] = { 'L', 'o', 'w', '/', 'S', 'e', 'r', 'i', 'f', 'e', 'd',  0 };
static const unichar_t str203[] = { 'C', 'o', 'n', 's', 't', 'a', 'n', 't', '/', 'S', 'm', 'a', 'l', 'l',  0 };
static const unichar_t str204[] = { 'C', 'o', 'n', 's', 't', 'a', 'n', 't', '/', 'S', 't', 'a', 'n', 'd', 'a', 'r', 'd',  0 };
static const unichar_t str205[] = { 'C', 'o', 'n', 's', 't', 'a', 'n', 't', '/', 'L', 'a', 'r', 'g', 'e',  0 };
static const unichar_t str206[] = { 'D', 'u', 'c', 'k', 'i', 'n', 'g', '/', 'S', 'm', 'a', 'l', 'l',  0 };
static const unichar_t str207[] = { 'D', 'u', 'c', 'k', 'i', 'n', 'g', '/', 'S', 't', 'a', 'n', 'd', 'a', 'r', 'd',  0 };
static const unichar_t str208[] = { 'D', 'u', 'c', 'k', 'i', 'n', 'g', '/', 'L', 'a', 'r', 'g', 'e',  0 };
static const unichar_t str209[] = { 'W', 'i', 'd', 't', 'h', ' ', 'C', 'l', 'a', 's', 's', ':',  0 };
static const unichar_t str210[] = { 'W', 'e', 'i', 'g', 'h', 't', ' ', 'C', 'l', 'a', 's', 's', ':',  0 };
static const unichar_t str211[] = { 'F', 'a', 'm', 'i', 'l', 'y',  0 };
static const unichar_t str212[] = { 'S', 'e', 'r', 'i', 'f', 's',  0 };
static const unichar_t str213[] = { 'W', 'e', 'i', 'g', 'h', 't',  0 };
static const unichar_t str214[] = { 'P', 'r', 'o', 'p', 'o', 'r', 't', 'i', 'o', 'n',  0 };
static const unichar_t str215[] = { 'C', 'o', 'n', 't', 'r', 'a', 's', 't',  0 };
static const unichar_t str216[] = { 'S', 't', 'r', 'o', 'k', 'e', ' ', 'V', 'a', 'r', 'i', 'a', 't', 'i', 'o', 'n',  0 };
static const unichar_t str217[] = { 'A', 'r', 'm', ' ', 'S', 't', 'y', 'l', 'e',  0 };
static const unichar_t str218[] = { 'L', 'e', 't', 't', 'e', 'r', 'f', 'o', 'r', 'm',  0 };
static const unichar_t str219[] = { 'M', 'i', 'd', 'l', 'i', 'n', 'e',  0 };
static const unichar_t str220[] = { 'X', '-', 'H', 'e', 'i', 'g', 'h', 't',  0 };
static const unichar_t str221[] = { 'S', 'u', 'b', ' ', 'X', ' ', 'S', 'i', 'z', 'e', ':',  0 };
static const unichar_t str222[] = { 'S', 'u', 'b', ' ', 'Y', ' ', 'S', 'i', 'z', 'e', ':',  0 };
static const unichar_t str223[] = { 'S', 'u', 'b', ' ', 'X', ' ', 'O', 'f', 'f', 's', 'e', 't', ':',  0 };
static const unichar_t str224[] = { 'S', 'u', 'b', ' ', 'Y', ' ', 'O', 'f', 'f', 's', 'e', 't', ':',  0 };
static const unichar_t str225[] = { 'S', 'u', 'p', ' ', 'X', ' ', 'S', 'i', 'z', 'e', ':',  0 };
static const unichar_t str226[] = { 'S', 'u', 'p', ' ', 'Y', ' ', 'S', 'i', 'z', 'e', ':',  0 };
static const unichar_t str227[] = { 'S', 'u', 'p', ' ', 'X', ' ', 'O', 'f', 'f', 's', 'e', 't', ':',  0 };
static const unichar_t str228[] = { 'S', 'u', 'p', ' ', 'Y', ' ', 'O', 'f', 'f', 's', 'e', 't', ':',  0 };
static const unichar_t str229[] = { 'S', 't', 'r', 'i', 'k', 'e', 'o', 'u', 't', ' ', 'S', 'i', 'z', 'e', ':',  0 };
static const unichar_t str230[] = { 'S', 't', 'r', 'i', 'k', 'e', 'o', 'u', 't', ' ', 'P', 'o', 's', ':',  0 };
static const unichar_t str231[] = { 'I', 'B', 'M', ' ', 'F', 'a', 'm', 'i', 'l', 'y', ':',  0 };
static const unichar_t str232[] = { 'N', 'o', ' ', 'C', 'l', 'a', 's', 's', 'i', 'f', 'i', 'c', 'a', 't', 'i', 'o', 'n',  0 };
static const unichar_t str233[] = { 'O', 'l', 'd', ' ', 'S', 't', 'y', 'l', 'e', ' ', 'S', 'e', 'r', 'i', 'f', 's',  0 };
static const unichar_t str234[] = { 'O', 'S', 'S', ' ', 'R', 'o', 'u', 'n', 'd', 'e', 'd', ' ', 'L', 'e', 'g', 'i', 'b', 'i', 'l', 'i', 't', 'y',  0 };
static const unichar_t str235[] = { 'O', 'S', 'S', ' ', 'G', 'e', 'r', 'a', 'l', 'd', 'e',  0 };
static const unichar_t str236[] = { 'O', 'S', 'S', ' ', 'V', 'e', 'n', 'e', 't', 'i', 'a', 'n',  0 };
static const unichar_t str237[] = { 'O', 'S', 'S', ' ', 'M', 'o', 'd', 'i', 'f', 'i', 'e', 'd', ' ', 'V', 'e', 'n', 'e', 't', 'i', 'a', 'n',  0 };
static const unichar_t str238[] = { 'O', 'S', 'S', ' ', 'D', 'u', 't', 'c', 'h', ' ', 'M', 'o', 'd', 'e', 'r', 'n',  0 };
static const unichar_t str239[] = { 'O', 'S', 'S', ' ', 'D', 'u', 't', 'c', 'h', ' ', 'T', 'r', 'a', 'd',  0 };
static const unichar_t str240[] = { 'O', 'S', 'S', ' ', 'C', 'o', 'n', 't', 'e', 'm', 'p', 'o', 'r', 'a', 'r', 'y',  0 };
static const unichar_t str241[] = { 'O', 'S', 'S', ' ', 'C', 'a', 'l', 'i', 'g', 'r', 'a', 'p', 'h', 'i', 'c',  0 };
static const unichar_t str242[] = { 'O', 'S', 'S', ' ', 'M', 'i', 's', 'c', 'e', 'l', 'l', 'a', 'n', 'e', 'o', 'u', 's',  0 };
static const unichar_t str243[] = { 'T', 'r', 'a', 'n', 's', 'i', 't', 'i', 'o', 'n', 'a', 'l', ' ', 'S', 'e', 'r', 'i', 'f', 's',  0 };
static const unichar_t str244[] = { 'T', 'S', ' ', 'D', 'i', 'r', 'e', 'c', 't', ' ', 'L', 'i', 'n', 'e',  0 };
static const unichar_t str245[] = { 'T', 'S', ' ', 'S', 'c', 'r', 'i', 'p', 't',  0 };
static const unichar_t str246[] = { 'T', 'S', ' ', 'M', 'i', 's', 'c', 'e', 'l', 'l', 'a', 'n', 'e', 'o', 'u', 's',  0 };
static const unichar_t str247[] = { 'M', 'o', 'd', 'e', 'r', 'n', ' ', 'S', 'e', 'r', 'i', 'f', 's',  0 };
static const unichar_t str248[] = { 'M', 'S', ' ', 'I', 't', 'a', 'l', 'i', 'a', 'n',  0 };
static const unichar_t str249[] = { 'M', 'S', ' ', 'S', 'c', 'r', 'i', 'p', 't',  0 };
static const unichar_t str250[] = { 'M', 'S', ' ', 'M', 'i', 's', 'c', 'e', 'l', 'l', 'a', 'n', 'e', 'o', 'u', 's',  0 };
static const unichar_t str251[] = { 'C', 'l', 'a', 'r', 'e', 'n', 'd', 'o', 'n', ' ', 'S', 'e', 'r', 'i', 'f', 's',  0 };
static const unichar_t str252[] = { 'C', 'S', ' ', 'C', 'l', 'a', 'r', 'e', 'n', 'd', 'o', 'n',  0 };
static const unichar_t str253[] = { 'C', 'S', ' ', 'M', 'o', 'd', 'e', 'r', 'n',  0 };
static const unichar_t str254[] = { 'C', 'S', ' ', 'T', 'r', 'a', 'd', 'i', 't', 'i', 'o', 'n', 'a', 'l',  0 };
static const unichar_t str255[] = { 'C', 'S', ' ', 'N', 'e', 'w', 's', 'p', 'a', 'p', 'e', 'r',  0 };
static const unichar_t str256[] = { 'C', 'S', ' ', 'S', 't', 'u', 'b', ' ', 'S', 'e', 'r', 'i', 'f',  0 };
static const unichar_t str257[] = { 'C', 'S', ' ', 'M', 'o', 'n', 'o', 't', 'o', 'n', 'e',  0 };
static const unichar_t str258[] = { 'C', 'S', ' ', 'T', 'y', 'p', 'e', 'w', 'r', 'i', 't', 'e', 'r',  0 };
static const unichar_t str259[] = { 'C', 'S', ' ', 'M', 'i', 's', 'c', 'e', 'l', 'l', 'a', 'n', 'e', 'o', 'u', 's',  0 };
static const unichar_t str260[] = { 'S', 'l', 'a', 'b', ' ', 'S', 'e', 'r', 'i', 'f', 's',  0 };
static const unichar_t str261[] = { 'S', 'S', ' ', 'M', 'o', 'n', 'o', 't', 'o', 'n', 'e',  0 };
static const unichar_t str262[] = { 'S', 'S', ' ', 'H', 'u', 'm', 'a', 'n', 'i', 's', 't',  0 };
static const unichar_t str263[] = { 'S', 'S', ' ', 'G', 'e', 'o', 'm', 'e', 't', 'r', 'i', 'c',  0 };
static const unichar_t str264[] = { 'S', 'S', ' ', 'S', 'w', 'i', 's', 's',  0 };
static const unichar_t str265[] = { 'S', 'S', ' ', 'T', 'y', 'p', 'e', 'w', 'r', 'i', 't', 'e', 'r',  0 };
static const unichar_t str266[] = { 'S', 'S', ' ', 'M', 'i', 's', 'c', 'e', 'l', 'l', 'a', 'n', 'e', 'o', 'u', 's',  0 };
static const unichar_t str267[] = { 'F', 'r', 'e', 'e', 'f', 'o', 'r', 'm', ' ', 'S', 'e', 'r', 'i', 'f', 's',  0 };
static const unichar_t str268[] = { 'F', 'S', ' ', 'M', 'o', 'd', 'e', 'r', 'n',  0 };
static const unichar_t str269[] = { 'F', 'S', ' ', 'M', 'i', 's', 'c', 'e', 'l', 'l', 'a', 'n', 'e', 'o', 'u', 's',  0 };
static const unichar_t str270[] = { 'S', 'S', ' ', 'I', 'B', 'M', ' ', 'N', 'e', 'o', 'G', 'r', 'o', 't', 'e', 's', 'q', 'u', 'e', ' ', 'G', 'o', 't', 'h', 'i', 'c',  0 };
static const unichar_t str271[] = { 'S', 'S', ' ', 'L', 'o', 'w', '-', 'x', ' ', 'R', 'o', 'u', 'n', 'd', ' ', 'G', 'e', 'o', 'm', 'e', 't', 'r', 'i', 'c',  0 };
static const unichar_t str272[] = { 'S', 'S', ' ', 'H', 'i', 'g', 'h', '-', 'x', ' ', 'R', 'o', 'u', 'n', 'd', ' ', 'G', 'e', 'o', 'm', 'e', 't', 'r', 'i', 'c',  0 };
static const unichar_t str273[] = { 'S', 'S', ' ', 'N', 'e', 'o', 'G', 'r', 'o', 't', 'e', 's', 'q', 'u', 'e', ' ', 'G', 'o', 't', 'h', 'i', 'c',  0 };
static const unichar_t str274[] = { 'S', 'S', ' ', 'M', 'o', 'd', 'i', 'f', 'i', 'e', 'd', ' ', 'G', 'r', 'o', 't', 'e', 's', 'q', 'u', 'e', ' ', 'G', 'o', 't', 'h', 'i', 'c',  0 };
static const unichar_t str275[] = { 'S', 'S', ' ', 'T', 'y', 'p', 'e', 'w', 'r', 'i', 't', 'e', 'r', ' ', 'G', 'o', 't', 'h', 'i', 'c',  0 };
static const unichar_t str276[] = { 'S', 'S', ' ', 'M', 'a', 't', 'r', 'i', 'x',  0 };
static const unichar_t str277[] = { 'O', 'r', 'n', 'a', 'm', 'e', 'n', 't', 'a', 'l', 's',  0 };
static const unichar_t str278[] = { 'O', ' ', 'E', 'n', 'g', 'r', 'a', 'v', 'e', 'r',  0 };
static const unichar_t str279[] = { 'O', ' ', 'B', 'l', 'a', 'c', 'k', ' ', 'L', 'e', 't', 't', 'e', 'r',  0 };
static const unichar_t str280[] = { 'O', ' ', 'D', 'e', 'c', 'o', 'r', 'a', 't', 'i', 'v', 'e',  0 };
static const unichar_t str281[] = { 'O', ' ', 'T', 'h', 'r', 'e', 'e', ' ', 'D', 'i', 'm', 'e', 'n', 's', 'i', 'o', 'n', 'a', 'l',  0 };
static const unichar_t str282[] = { 'O', ' ', 'M', 'i', 's', 'c', 'e', 'l', 'l', 'a', 'n', 'e', 'o', 'u', 's',  0 };
static const unichar_t str283[] = { 'S', 'c', 'r', 'i', 'p', 't', 's',  0 };
static const unichar_t str284[] = { 'S', ' ', 'U', 'n', 'c', 'i', 'a', 'l',  0 };
static const unichar_t str285[] = { 'S', ' ', 'B', 'r', 'u', 's', 'h', ' ', 'J', 'o', 'i', 'n', 'e', 'd',  0 };
static const unichar_t str286[] = { 'S', ' ', 'F', 'o', 'r', 'm', 'a', 'l', ' ', 'J', 'o', 'i', 'n', 'e', 'd',  0 };
static const unichar_t str287[] = { 'S', ' ', 'M', 'o', 'n', 'o', 't', 'o', 'n', 'e', ' ', 'J', 'o', 'i', 'n', 'e', 'd',  0 };
static const unichar_t str288[] = { 'S', ' ', 'C', 'a', 'l', 'i', 'g', 'r', 'a', 'p', 'h', 'i', 'c',  0 };
static const unichar_t str289[] = { 'S', ' ', 'B', 'r', 'u', 's', 'h', ' ', 'U', 'n', 'j', 'o', 'i', 'n', 'e', 'd',  0 };
static const unichar_t str290[] = { 'S', ' ', 'F', 'o', 'r', 'm', 'a', 'l', ' ', 'U', 'n', 'j', 'o', 'i', 'n', 'e', 'd',  0 };
static const unichar_t str291[] = { 'S', ' ', 'M', 'o', 'n', 'o', 't', 'o', 'n', 'e', ' ', 'U', 'n', 'j', 'o', 'i', 'n', 'e', 'd',  0 };
static const unichar_t str292[] = { 'S', ' ', 'M', 'i', 's', 'c', 'e', 'l', 'l', 'a', 'n', 'e', 'o', 'u', 's',  0 };
static const unichar_t str293[] = { 'S', 'y', 'm', 'b', 'o', 'l', 'i', 'c',  0 };
static const unichar_t str294[] = { 'S', 'y', ' ', 'M', 'i', 'x', 'e', 'd', ' ', 'S', 'e', 'r', 'i', 'f',  0 };
static const unichar_t str295[] = { 'S', 'y', ' ', 'O', 'l', 'd', ' ', 'S', 't', 'y', 'l', 'e', ' ', 'S', 'e', 'r', 'i', 'f',  0 };
static const unichar_t str296[] = { 'S', 'y', ' ', 'N', 'e', 'o', '-', 'g', 'r', 'o', 't', 'e', 's', 'q', 'u', 'e', ' ', 'S', 'a', 'n', 's', ' ', 'S', 'e', 'r', 'i', 'f',  0 };
static const unichar_t str297[] = { 'S', 'y', ' ', 'M', 'i', 's', 'c', 'e', 'l', 'l', 'a', 'n', 'e', 'o', 'u', 's',  0 };
static const unichar_t str298[] = { 'V', 'e', 'n', 'd', 'o', 'r', ' ', 'I', 'D', ':',  0 };
static const unichar_t str299[] = { 'U', 'n', 'i', 'c', 'o', 'd', 'e', ' ', 'R', 'a', 'n', 'g', 'e', 's',  0 };
static const unichar_t str300[] = { 'B', 'a', 's', 'i', 'c', ' ', 'L', 'a', 't', 'i', 'n', ' ', '0', '0', '0', '0', '-', '0', '0', '7', 'f',  0 };
static const unichar_t str301[] = { 'L', 'a', 't', 'i', 'n', '-', '1', ' ', 'S', 'u', 'p', ' ', '0', '0', '8', '0', '-', '0', '0', 'f', 'f',  0 };
static const unichar_t str302[] = { 'L', 'a', 't', 'i', 'n', ' ', 'E', 'x', 't', '-', 'A', ' ', '0', '1', '0', '0', '-', '0', '1', '7', 'f',  0 };
static const unichar_t str303[] = { 'L', 'a', 't', 'i', 'n', ' ', 'E', 'x', 't', '-', 'B', ' ', '0', '1', '8', '0', '-', '0', '2', '4', 'f',  0 };
static const unichar_t str304[] = { 'I', 'P', 'A', ' ', 'E', 'x', 't', 'e', 'n', 's', 'i', 'o', 'n', 's', ' ', '0', '2', '5', '0', '-', '0', '2', 'A', 'f',  0 };
static const unichar_t str305[] = { 'S', 'p', 'a', 'c', 'i', 'n', 'g', ' ', 'M', 'o', 'd', ' ', 'L', 'e', 't', 't', 'e', 'r', 's', ' ', '0', '2', 'B', '0', '-', '0', '2', 'F', 'F',  0 };
static const unichar_t str306[] = { 'C', 'o', 'm', 'b', 'i', 'n', 'i', 'n', 'g', ' ', 'D', 'i', 'a', 'c', 'r', 'i', 't', 'i', 'c', 's', ' ', '0', '3', '0', '0', '-', '0', '3', '6', 'F',  0 };
static const unichar_t str307[] = { 'G', 'r', 'e', 'e', 'k', ' ', '0', '3', '7', '0', '-', '0', '3', 'F', 'F',  0 };
static const unichar_t str308[] = { 'O', 'b', 's', 'o', 'l', 'e', 't', 'e', ' ', 'G', 'r', 'e', 'e', 'k', ' ', 'S', 'y', 'm', 'b', 'o', 'l', 's', ' ', '&', ' ', 'C', 'o', 'p', 't', 'i', 'c',  0 };
static const unichar_t str309[] = { 'C', 'y', 'r', 'i', 'l', 'l', 'i', 'c', ' ', '0', '4', '0', '0', '-', '0', '5', '2', 'F',  0 };
static const unichar_t str310[] = { 'A', 'r', 'm', 'e', 'n', 'i', 'a', 'n', ' ', '0', '5', '3', '0', '-', '0', '5', '8', 'F',  0 };
static const unichar_t str311[] = { 'H', 'e', 'b', 'r', 'e', 'w', ' ', '0', '5', '9', '0', '-', '0', '5', 'F', 'F',  0 };
static const unichar_t str312[] = { 'O', 'b', 's', 'o', 'l', 'e', 't', 'e', ' ', 'E', 'x', 't', 'e', 'n', 'd', 'e', 'd', ' ', 'H', 'e', 'b', 'r', 'e', 'w',  0 };
static const unichar_t str313[] = { 'A', 'r', 'a', 'b', 'i', 'c', ' ', '0', '6', '0', '0', '-', '0', '6', 'F', 'F',  0 };
static const unichar_t str314[] = { 'O', 'b', 's', 'o', 'l', 'e', 't', 'e', ' ', 'E', 'x', 't', 'e', 'n', 'd', 'e', 'd', ' ', 'A', 'r', 'a', 'b', 'i', 'c',  0 };
static const unichar_t str315[] = { 'D', 'e', 'v', 'a', 'n', 'a', 'g', 'a', 'r', 'i', ' ', '0', '9', '0', '0', '-', '0', '9', '7', 'F',  0 };
static const unichar_t str316[] = { 'B', 'e', 'n', 'g', 'a', 'l', 'i', ' ', '0', '9', '8', '0', '-', '0', '9', 'F', 'F',  0 };
static const unichar_t str317[] = { 'G', 'u', 'r', 'm', 'u', 'k', 'h', 'i', ' ', '0', 'A', '0', '0', '-', '0', 'A', '7', 'F',  0 };
static const unichar_t str318[] = { 'G', 'u', 'j', 'a', 'r', 'a', 't', 'i', ' ', '0', 'A', '8', '0', '-', '0', 'A', 'F', 'F',  0 };
static const unichar_t str319[] = { 'O', 'r', 'i', 'y', 'a', ' ', '0', 'B', '0', '0', '-', '0', 'B', '7', 'F',  0 };
static const unichar_t str320[] = { 'T', 'a', 'm', 'i', 'l', ' ', '0', 'B', '8', '0', '-', '0', 'B', 'F', 'F',  0 };
static const unichar_t str321[] = { 'T', 'e', 'l', 'e', 'g', 'u', ' ', '0', 'C', '0', '0', '-', '0', 'C', '7', 'F',  0 };
static const unichar_t str322[] = { 'K', 'a', 'n', 'n', 'a', 'd', 'a', ' ', '0', 'C', '8', '0', '-', '0', 'C', 'F', 'F',  0 };
static const unichar_t str323[] = { 'M', 'a', 'l', 'a', 'y', 'a', 'l', 'a', 'm', ' ', '0', 'D', '0', '0', '-', '0', 'D', '7', 'F',  0 };
static const unichar_t str324[] = { 'T', 'h', 'a', 'i', ' ', '0', 'E', '0', '0', '-', '0', 'E', '7', 'F',  0 };
static const unichar_t str325[] = { 'L', 'o', 'a', ' ', '0', 'E', '8', '0', '-', '0', 'E', 'F', 'F',  0 };
static const unichar_t str326[] = { 'G', 'e', 'o', 'r', 'g', 'i', 'a', 'n', ' ', '1', '0', 'A', '0', '-', '1', '0', 'F', 'F',  0 };
static const unichar_t str327[] = { 'O', 'b', 's', 'o', 'l', 'e', 't', 'e', ' ', 'E', 'x', 't', 'e', 'n', 'd', 'e', 'd', ' ', 'G', 'e', 'o', 'r', 'g', 'i', 'a', 'n',  0 };
static const unichar_t str328[] = { 'H', 'a', 'n', 'g', 'u', 'l', ' ', 'J', 'a', 'm', 'o', ' ', '1', '1', '0', '0', '-', '1', '1', 'F', 'F',  0 };
static const unichar_t str329[] = { 'L', 'a', 't', 'i', 'n', ' ', 'A', 'd', 'd', 'i', 't', 'i', 'o', 'n', 'a', 'l', ' ', '1', 'E', '0', '0', '-', '1', 'E', 'F', 'F',  0 };
static const unichar_t str330[] = { 'G', 'r', 'e', 'e', 'k', ' ', 'A', 'd', 'd', 'i', 't', 'i', 'o', 'n', 'a', 'l', ' ', '1', 'F', '0', '0', '-', '1', 'F', 'F', 'F',  0 };
static const unichar_t str331[] = { 'P', 'u', 'n', 'c', 't', 'u', 'a', 't', 'i', 'o', 'n', ' ', '2', '0', '0', '0', '-', '2', '0', '6', 'F',  0 };
static const unichar_t str332[] = { 'S', 'u', 'b', '/', 'S', 'u', 'p', 'e', 'r', ' ', 's', 'c', 'r', 'i', 'p', 't', 's', ' ', '2', '0', '7', '0', '-', '2', '0', '9', 'F',  0 };
static const unichar_t str333[] = { 'C', 'u', 'r', 'r', 'e', 'n', 'c', 'y', ' ', '2', '0', 'A', '0', '-', '2', '0', 'C', 'F',  0 };
static const unichar_t str334[] = { 'C', 'o', 'm', 'b', 'i', 'n', 'i', 'n', 'g', ' ', 'S', 'y', 'm', 'b', 'o', 'l', ' ', 'D', 'i', 'a', 'c', 'r', 'i', 't', 'i', 'c', 's', ' ', '2', '0', 'D', '0', '-', '2', '0', 'F', 'F',  0 };
static const unichar_t str335[] = { 'L', 'e', 't', 't', 'e', 'r', 'l', 'i', 'k', 'e', ' ', 'S', 'y', 'm', 'b', 'o', 'l', 's', ' ', '2', '1', '0', '0', '-', '2', '1', '4', 'F',  0 };
static const unichar_t str336[] = { 'N', 'u', 'm', 'b', 'e', 'r', ' ', 'F', 'o', 'r', 'm', 's', ' ', '2', '1', '5', '0', '-', '2', '1', '8', 'F',  0 };
static const unichar_t str337[] = { 'A', 'r', 'r', 'o', 'w', 's', ' ', '2', '1', '9', '0', '-', '2', '1', 'F', 'F',  0 };
static const unichar_t str338[] = { 'M', 'a', 't', 'h', ' ', 'O', 'p', 'e', 'r', 's', ' ', '2', '2', '0', '0', '-', '2', '2', 'F', 'F',  0 };
static const unichar_t str339[] = { 'M', 'i', 's', 'c', ' ', 'T', 'e', 'c', 'h', 'n', 'i', 'c', 'a', 'l', ' ', '2', '3', '0', '0', '-', '2', '3', 'F', 'F',  0 };
static const unichar_t str340[] = { 'C', 'o', 'n', 't', 'r', 'o', 'l', ' ', 'P', 'i', 'c', 't', 'u', 'r', 'e', 's', ' ', '2', '4', '0', '0', '-', '2', '3', '3', 'F',  0 };
static const unichar_t str341[] = { 'O', 'C', 'R', ' ', '2', '4', '4', '0', '-', '2', '4', '5', 'F',  0 };
static const unichar_t str342[] = { 'E', 'n', 'c', 'l', 'o', 's', 'e', 'd', ' ', 'A', 'l', 'p', 'h', 'a', 'n', 'u', 'm', 'e', 'r', 'i', 'c', 's', ' ', '2', '4', '6', '0', '-', '2', '4', 'F', 'F',  0 };
static const unichar_t str343[] = { 'B', 'o', 'x', ' ', 'D', 'r', 'a', 'w', 'i', 'n', 'g', ' ', '2', '5', '0', '0', '-', '2', '5', '7', 'F',  0 };
static const unichar_t str344[] = { 'B', 'l', 'o', 'c', 'k', ' ', 'E', 'l', 'e', 'm', 'e', 'n', 't', 's', ' ', '2', '5', '8', '0', '-', '2', '5', '9', 'F',  0 };
static const unichar_t str345[] = { 'G', 'e', 'o', 'm', 'e', 't', 'r', 'i', 'c', ' ', 'S', 'h', 'a', 'p', 'e', 's', ' ', '2', '5', 'A', '0', '-', '2', '5', 'F', 'F',  0 };
static const unichar_t str346[] = { 'M', 'i', 's', 'c', ' ', 'S', 'y', 'm', 'b', 'o', 'l', 's', ' ', '2', '6', '0', '0', '-', '2', '6', '7', 'F',  0 };
static const unichar_t str347[] = { 'D', 'i', 'n', 'g', 'b', 'a', 't', 's', ' ', '2', '7', '0', '0', '-', '2', '7', 'B', 'F',  0 };
static const unichar_t str348[] = { 'C', 'J', 'K', ' ', 'S', 'y', 'm', 'b', 'o', 'l', 's', ' ', '&', ' ', 'P', 'u', 'n', 'c', 't', ' ', '3', '0', '0', '0', '-', '3', '0', '3', 'F',  0 };
static const unichar_t str349[] = { 'H', 'i', 'r', 'a', 'g', 'a', 'n', 'a', ' ', '3', '0', '4', '0', '-', '3', '0', '9', 'F',  0 };
static const unichar_t str350[] = { 'K', 'a', 't', 'a', 'k', 'a', 'n', 'a', ' ', '3', '0', 'A', '0', '-', '3', '0', 'F', 'F',  0 };
static const unichar_t str351[] = { 'B', 'o', 'p', 'o', 'm', 'o', 'f', 'o', ' ', '3', '1', '0', '0', '-', '3', '1', '2', 'F', ',', ' ', '3', '1', '8', '0', '-', '3', '1', 'A', 'F',  0 };
static const unichar_t str352[] = { 'H', 'a', 'n', 'g', 'u', 'l', ' ', 'C', 'o', 'm', 'p', 'a', 't', '.', ' ', 'J', 'a', 'm', 'o', ' ', '3', '1', '3', '0', '-', '3', '1', '8', 'F',  0 };
static const unichar_t str353[] = { 'C', 'J', 'K', ' ', 'M', 'i', 's', 'c', ' ', '3', '1', 'B', '0', '-', '3', '1', 'F', 'F',  0 };
static const unichar_t str354[] = { 'E', 'n', 'c', 'l', 'o', 's', 'e', 'd', ' ', 'L', 'e', 't', 't', 'e', 'r', 's', ' ', 'M', 'o', 'n', 't', 'h', 's', ' ', '3', '2', '0', '0', '-', '3', '2', 'F', 'F',  0 };
static const unichar_t str355[] = { 'C', 'J', 'K', ' ', 'C', 'o', 'm', 'p', 'a', 't', 'a', 'b', 'i', 'l', 'i', 't', 'y', ' ', '3', '3', '0', '0', '-', '3', '3', 'F', 'F',  0 };
static const unichar_t str356[] = { 'H', 'a', 'n', 'g', 'u', 'l', ' ', 'A', 'C', '0', '0', '-', 'D', '7', 'A', 'F',  0 };
static const unichar_t str357[] = { 'S', 'u', 'r', 'r', 'o', 'g', 'a', 't', 'e', 's', ' ', '1', '0', '0', '0', '0', '-', '7', 'F', 'F', 'F', 'F', 'F', 'F', 'F',  0 };
static const unichar_t str358[] = { 'U', 'n', 'a', 's', 's', 'i', 'g', 'n', 'e', 'd', ' ', 'b', 'i', 't', ' ', '5', '8',  0 };
static const unichar_t str359[] = { 'C', 'J', 'K', ' ', '2', 'E', '8', '0', '-', '2', '9', 'F', 'F', ',', ' ', '3', '4', '0', '0', '-', '9', 'F', 'F', 'F',  0 };
static const unichar_t str360[] = { 'P', 'r', 'i', 'v', 'a', 't', 'e', ' ', 'U', 's', 'e', ' ', 'E', '0', '0', '0', '-', 'F', '8', 'F', 'F',  0 };
static const unichar_t str361[] = { 'C', 'J', 'K', ' ', 'C', 'o', 'm', 'p', 'a', 't', '.', ' ', 'I', 'd', 'e', 'o', 'g', 'r', 'a', 'p', 'h', 's', ' ', 'F', '9', '0', '0', '-', 'F', 'A', 'F', 'F',  0 };
static const unichar_t str362[] = { 'A', 'l', 'p', 'h', 'a', 'b', 'e', 't', 'i', 'c', ' ', 'F', 'o', 'r', 'm', 's', ' ', 'F', 'B', '0', '0', '-', 'F', 'B', '4', 'F',  0 };
static const unichar_t str363[] = { 'A', 'r', 'a', 'b', 'i', 'c', ' ', 'F', 'o', 'r', 'm', 's', '-', 'A', ' ', 'F', 'B', '5', '0', '-', 'F', 'D', 'F', 'F',  0 };
static const unichar_t str364[] = { 'C', 'o', 'm', 'b', 'i', 'n', 'i', 'n', 'g', ' ', 'H', 'a', 'l', 'f', ' ', 'M', 'a', 'r', 'k', 's', ' ', 'F', 'E', '2', '0', '-', 'F', 'E', '2', 'F',  0 };
static const unichar_t str365[] = { 'C', 'J', 'K', ' ', 'C', 'o', 'm', 'p', 'a', 't', 'a', 'b', 'i', 'l', 'i', 't', 'y', ' ', 'F', 'o', 'r', 'm', 's', ' ', 'F', 'E', '3', '0', '-', 'F', 'E', '4', 'F',  0 };
static const unichar_t str366[] = { 'S', 'm', 'a', 'l', 'l', ' ', 'F', 'o', 'r', 'm', 's', ' ', 'F', 'E', '5', '0', '-', 'F', 'E', '6', 'F',  0 };
static const unichar_t str367[] = { 'A', 'r', 'a', 'b', 'i', 'c', ' ', 'F', 'o', 'r', 'm', 's', '-', 'B', ' ', 'F', 'E', '7', '0', '-', 'F', 'E', 'F', 'F',  0 };
static const unichar_t str368[] = { 'H', 'a', 'l', 'f', '/', 'F', 'u', 'l', 'l', ' ', 'W', 'i', 'd', 't', 'h', ' ', 'F', 'o', 'r', 'm', 's', ' ', 'F', 'F', '0', '0', '-', 'F', 'F', 'E', 'F',  0 };
static const unichar_t str369[] = { 'S', 'p', 'e', 'c', 'i', 'a', 'l', 's', ' ', 'F', 'F', 'F', '0', '-', 'F', 'F', 'F', 'F',  0 };
static const unichar_t str370[] = { 'T', 'i', 'b', 'e', 't', 'a', 'n', ' ', '0', 'F', '0', '0', '-', '0', 'F', 'F', 'F',  0 };
static const unichar_t str371[] = { 'S', 'y', 'r', 'i', 'a', 'c', ' ', '0', '7', '0', '0', '-', '0', '7', '4', 'F',  0 };
static const unichar_t str372[] = { 'T', 'h', 'a', 'a', 'n', 'a', ' ', '0', '7', '8', '0', '-', '0', '7', 'B', 'F',  0 };
static const unichar_t str373[] = { 'S', 'i', 'n', 'h', 'a', 'l', 'a', ' ', '0', 'D', '8', '0', '-', '0', 'D', 'B', 'F',  0 };
static const unichar_t str374[] = { 'M', 'y', 'a', 'n', 'm', 'a', 'r', ' ', '1', '0', '0', '0', '-', '1', '0', '9', 'F',  0 };
static const unichar_t str375[] = { 'E', 't', 'h', 'i', 'o', 'p', 'i', 'c', ' ', '1', '2', '0', '0', '-', '1', '2', 'F', 'F',  0 };
static const unichar_t str376[] = { 'C', 'h', 'e', 'r', 'o', 'k', 'e', 'e', ' ', '1', '3', 'A', '0', '-', '1', '3', 'F', 'F',  0 };
static const unichar_t str377[] = { 'U', 'n', 'i', 't', 'e', 'd', ' ', 'C', 'a', 'n', 'a', 'd', 'i', 'a', 'n', ' ', 'S', 'y', 'l', 'l', 'a', 'b', 'i', 'c', 's', ' ', '1', '4', '0', '0', '-', '1', '6', '7', 'F',  0 };
static const unichar_t str378[] = { 'O', 'g', 'h', 'a', 'm', ' ', '1', '6', '8', '0', '-', '1', '6', '9', 'F',  0 };
static const unichar_t str379[] = { 'R', 'u', 'n', 'i', 'c', ' ', '1', '6', 'A', '0', '-', '1', '6', 'F', 'F',  0 };
static const unichar_t str380[] = { 'K', 'h', 'm', 'e', 'r', ' ', '1', '7', '8', '0', '-', '1', '7', 'F', 'F',  0 };
static const unichar_t str381[] = { 'M', 'o', 'n', 'g', 'o', 'l', 'i', 'a', 'n', ' ', '1', '8', '0', '0', '-', '1', '8', 'A', 'F',  0 };
static const unichar_t str382[] = { 'B', 'r', 'a', 'i', 'l', 'l', 'e', ' ', '2', '8', '0', '0', '-', '2', '8', 'F', 'F',  0 };
static const unichar_t str383[] = { 'Y', 'i', ' ', '&', ' ', 'Y', 'i', ' ', 'R', 'a', 'd', 'i', 'c', 'a', 'l', 's', ' ', 'A', '0', '0', '0', '-', 'A', '4', 'A', 'F',  0 };
static const unichar_t str384[] = { 'U', 'n', 'a', 's', 's', 'i', 'g', 'n', 'e', 'd', ' ', 'b', 'i', 't', ' ', '8', '4',  0 };
static const unichar_t str385[] = { 'A', 'd', 'd',  0 };
static const unichar_t str386[] = { 'R', 'e', 'm', 'o', 'v', 'e',  0 };
static const unichar_t str387[] = { 'F', 'i', 'r', 's', 't', ' ', 'U', 'n', 'i', 'c', 'o', 'd', 'e', ':',  0 };
static const unichar_t str388[] = { 'L', 'a', 's', 't', ':',  0 };
static const unichar_t str389[] = { 'S', 'e', 'l', 'e', 'c', 't', 'i', 'o', 'n', ':',  0 };
static const unichar_t str390[] = { 'I', 't', 'a', 'l', 'i', 'c',  0 };
static const unichar_t str391[] = { 'U', 'n', 'd', 'e', 'r', 's', 'c', 'o', 'r', 'e',  0 };
static const unichar_t str392[] = { 'N', 'e', 'g', 'a', 't', 'i', 'v', 'e',  0 };
static const unichar_t str393[] = { 'O', 'u', 't', 'l', 'i', 'n', 'e', 'd',  0 };
static const unichar_t str394[] = { 'S', 't', 'r', 'i', 'k', 'e', 'o', 'u', 't',  0 };
static const unichar_t str395[] = { 'R', 'e', 'g', 'u', 'l', 'a', 'r',  0 };
static const unichar_t str396[] = { 'C', 'o', 'd', 'e', 'P', 'a', 'g', 'e', 's', ':',  0 };
static const unichar_t str397[] = { '1', '2', '5', '2', ' ', 'L', 'a', 't', 'i', 'n', '1',  0 };
static const unichar_t str398[] = { '1', '2', '5', '0', ' ', 'L', 'a', 't', 'i', 'n', '2',  0 };
static const unichar_t str399[] = { '1', '2', '5', '1', ' ', 'C', 'y', 'r', 'i', 'l', 'l', 'i', 'c',  0 };
static const unichar_t str400[] = { '1', '2', '5', '3', ' ', 'G', 'r', 'e', 'e', 'k',  0 };
static const unichar_t str401[] = { '1', '2', '5', '4', ' ', 'T', 'u', 'r', 'k', 'i', 's', 'h',  0 };
static const unichar_t str402[] = { '1', '2', '5', '5', ' ', 'H', 'e', 'b', 'r', 'e', 'w',  0 };
static const unichar_t str403[] = { '1', '2', '5', '6', ' ', 'A', 'r', 'a', 'b', 'i', 'c',  0 };
static const unichar_t str404[] = { '1', '2', '5', '7', ' ', 'W', 'i', 'n', 'd', 'o', 'w', 's', ' ', 'B', 'a', 'l', 't', 'i', 'c',  0 };
static const unichar_t str405[] = { '1', '2', '5', '8', ' ', 'V', 'i', 'e', 't', 'n', 'a', 'm', 'e', 's', 'e',  0 };
static const unichar_t str406[] = { '8', '7', '4', ' ', 'T', 'h', 'a', 'i',  0 };
static const unichar_t str407[] = { '9', '3', '2', ' ', 'J', 'I', 'S', '/', 'J', 'a', 'p', 'a', 'n',  0 };
static const unichar_t str408[] = { '9', '3', '6', ' ', 'S', 'i', 'm', 'p', 'l', 'i', 'f', 'i', 'e', 'd', ' ', 'C', 'h', 'i', 'n', 'e', 's', 'e',  0 };
static const unichar_t str409[] = { '9', '4', '9', ' ', 'K', 'o', 'r', 'e', 'a', 'n', ' ', 'W', 'a', 'n', 's', 'u', 'n', 'g',  0 };
static const unichar_t str410[] = { '9', '5', '0', ' ', 'T', 'r', 'a', 'd', 'i', 't', 'i', 'o', 'n', 'a', 'l', ' ', 'C', 'h', 'i', 'n', 'e', 's', 'e',  0 };
static const unichar_t str411[] = { '1', '3', '6', '1', ' ', 'K', 'o', 'r', 'e', 'a', 'n', ' ', 'J', 'o', 'h', 'a', 'b',  0 };
static const unichar_t str412[] = { 'M', 'a', 'c', ' ', 'R', 'o', 'm', 'a', 'n',  0 };
static const unichar_t str413[] = { 'O', 'E', 'M', ' ', 'C', 'h', 'a', 'r', 's', 'e', 't',  0 };
static const unichar_t str414[] = { 'S', 'y', 'm', 'b', 'o', 'l', ' ', 'C', 'h', 'a', 'r', 's', 'e', 't',  0 };
static const unichar_t str415[] = { '8', '6', '9', ' ', 'I', 'B', 'M', ' ', 'G', 'r', 'e', 'e', 'k',  0 };
static const unichar_t str416[] = { '8', '6', '6', ' ', 'M', 'S', '/', 'D', 'O', 'S', ' ', 'R', 'u', 's', 's', 'i', 'a', 'n',  0 };
static const unichar_t str417[] = { '8', '6', '5', ' ', 'M', 'S', '/', 'D', 'O', 'S', ' ', 'N', 'o', 'r', 'd', 'i', 'c',  0 };
static const unichar_t str418[] = { '8', '6', '4', ' ', 'A', 'r', 'a', 'b', 'i', 'c',  0 };
static const unichar_t str419[] = { '8', '6', '3', ' ', 'M', 'S', '/', 'D', 'O', 'S', ' ', 'C', 'a', 'n', 'a', 'd', 'i', 'a', 'n', ' ', 'F', 'r', 'e', 'n', 'c', 'h',  0 };
static const unichar_t str420[] = { '8', '6', '2', ' ', 'H', 'e', 'b', 'r', 'e', 'w',  0 };
static const unichar_t str421[] = { '8', '6', '1', ' ', 'M', 'S', '/', 'D', 'O', 'S', ' ', 'I', 'c', 'e', 'l', 'a', 'n', 'd', 'i', 'c',  0 };
static const unichar_t str422[] = { '8', '6', '0', ' ', 'M', 'S', '/', 'D', 'O', 'S', ' ', 'P', 'o', 'r', 't', 'u', 'g', 'u', 'e', 's', 'e',  0 };
static const unichar_t str423[] = { '8', '5', '7', ' ', 'I', 'B', 'M', ' ', 'T', 'u', 'r', 'k', 'i', 's', 'h',  0 };
static const unichar_t str424[] = { '8', '5', '5', ' ', 'I', 'B', 'M', ' ', 'C', 'y', 'r', 'i', 'l', 'l', 'i', 'c',  0 };
static const unichar_t str425[] = { '8', '5', '2', ' ', 'L', 'a', 't', 'i', 'n', '2',  0 };
static const unichar_t str426[] = { '7', '7', '5', ' ', 'I', 'B', 'M', ' ', 'B', 'a', 'l', 't', 'i', 'c',  0 };
static const unichar_t str427[] = { '7', '3', '7', ' ', 'G', 'r', 'e', 'e', 'k',  0 };
static const unichar_t str428[] = { '7', '0', '8', ' ', 'A', 'r', 'a', 'b', 'i', 'c',  0 };
static const unichar_t str429[] = { '8', '5', '0', ' ', 'W', 'E', '/', 'L', 'a', 't', 'i', 'n', '1',  0 };
static const unichar_t str430[] = { '4', '3', '7', ' ', 'U', 'S',  0 };
static const unichar_t str431[] = { 'T', 'y', 'p', 'o',  0 };
static const unichar_t str432[] = { 'A', 's', 'c', 'e', 'n', 'd', 'e', 'r', ':',  0 };
static const unichar_t str433[] = { 'D', 'e', 's', 'c', 'e', 'n', 'd', 'e', 'r', ':',  0 };
static const unichar_t str434[] = { 'L', 'i', 'n', 'e', ' ', 'G', 'a', 'p', ':',  0 };
static const unichar_t str435[] = { 'W', 'i', 'n', ' ', 'A', 's', 'c', 'e', 'n', 't', ':',  0 };
static const unichar_t str436[] = { 'W', 'i', 'n', ' ', 'D', 'e', 's', 'c', 'e', 'n', 't', ':',  0 };
static const unichar_t str437[] = { 'X', '-', 'H', 'e', 'i', 'g', 'h', 't', ':',  0 };
static const unichar_t str438[] = { 'C', 'a', 'p', '-', 'H', 'e', 'i', 'g', 'h', 't', ':',  0 };
static const unichar_t str439[] = { 'D', 'e', 'f', 'a', 'u', 'l', 't', ' ', 'C', 'h', 'a', 'r', ':',  0 };
static const unichar_t str440[] = { 'B', 'r', 'e', 'a', 'k', ' ', 'C', 'h', 'a', 'r', ':',  0 };
static const unichar_t str441[] = { 'M', 'a', 'x', ' ', 'C', 'o', 'n', 't', 'e', 'x', 't', ':',  0 };
static const unichar_t str442[] = { 'U', 'n', 'i', 'c', 'o', 'd', 'e',  0 };
static const unichar_t str443[] = { 'M', 'a', 'c',  0 };
static const unichar_t str444[] = { 'I', 'S', 'O',  0 };
static const unichar_t str445[] = { 'M', 'S',  0 };
static const unichar_t str446[] = { 'D', 'e', 'f', 'a', 'u', 'l', 't',  0 };
static const unichar_t str447[] = { 'U', 'n', 'i', 'c', 'o', 'd', 'e', ' ', '1', '.', '0',  0 };
static const unichar_t str448[] = { 'U', 'n', 'i', 'c', 'o', 'd', 'e', ' ', '1', '.', '1',  0 };
static const unichar_t str449[] = { 'U', 'n', 'i', 'c', 'o', 'd', 'e', ' ', '2', '.', '0',  0 };
static const unichar_t str450[] = { 'A', 'S', 'C', 'I', 'I',  0 };
static const unichar_t str451[] = { 'I', 'S', 'O', ' ', '1', '0', '6', '4', '6', '-', '1',  0 };
static const unichar_t str452[] = { 'I', 'S', 'O', ' ', '8', '8', '5', '9', '-', '1',  0 };
static const unichar_t str453[] = { 'U', 'n', 'i', 'c', 'o', 'd', 'e',  0 };
static const unichar_t str454[] = { 'R', 'o', 'm', 'a', 'n',  0 };
static const unichar_t str455[] = { 'J', 'a', 'p', 'a', 'n', 'e', 's', 'e',  0 };
static const unichar_t str456[] = { 'C', 'h', 'i', 'n', 'e', 's', 'e',  0 };
static const unichar_t str457[] = { 'K', 'o', 'r', 'e', 'a', 'n',  0 };
static const unichar_t str458[] = { 'A', 'r', 'a', 'b', 'i', 'c',  0 };
static const unichar_t str459[] = { 'H', 'e', 'b', 'r', 'e', 'w',  0 };
static const unichar_t str460[] = { 'G', 'r', 'e', 'e', 'k',  0 };
static const unichar_t str461[] = { 'R', 'u', 's', 's', 'i', 'a', 'n',  0 };
static const unichar_t str462[] = { 'R', 'S', 'y', 'm', 'b', 'o', 'l',  0 };
static const unichar_t str463[] = { 'D', 'e', 'v', 'a', 'n', 'a', 'g', 'a', 'r', 'i',  0 };
static const unichar_t str464[] = { 'G', 'u', 'r', 'm', 'u', 'k', 'h', 'i',  0 };
static const unichar_t str465[] = { 'G', 'u', 'j', 'a', 'r', 'a', 't', 'i',  0 };
static const unichar_t str466[] = { 'O', 'r', 'i', 'y', 'a',  0 };
static const unichar_t str467[] = { 'B', 'e', 'n', 'g', 'a', 'l', 'i',  0 };
static const unichar_t str468[] = { 'T', 'a', 'm', 'i', 'l',  0 };
static const unichar_t str469[] = { 'T', 'e', 'l', 'u', 'g', 'u',  0 };
static const unichar_t str470[] = { 'K', 'a', 'n', 'n', 'a', 'd', 'a',  0 };
static const unichar_t str471[] = { 'M', 'a', 'l', 'a', 'y', 'a', 'l', 'a', 'm',  0 };
static const unichar_t str472[] = { 'S', 'i', 'n', 'h', 'a', 'l', 'e', 's', 'e',  0 };
static const unichar_t str473[] = { 'B', 'u', 'r', 'm', 'e', 's', 'e',  0 };
static const unichar_t str474[] = { 'K', 'h', 'm', 'e', 'r',  0 };
static const unichar_t str475[] = { 'T', 'h', 'a', 'i',  0 };
static const unichar_t str476[] = { 'L', 'o', 'a', 't', 'i', 'a', 'n',  0 };
static const unichar_t str477[] = { 'G', 'e', 'o', 'r', 'g', 'i', 'a', 'n',  0 };
static const unichar_t str478[] = { 'A', 'r', 'm', 'e', 'n', 'i', 'a', 'n',  0 };
static const unichar_t str479[] = { 'M', 'a', 'l', 'd', 'i', 'v', 'i', 'a', 'n',  0 };
static const unichar_t str480[] = { 'T', 'i', 'b', 'e', 't', 'a', 'n',  0 };
static const unichar_t str481[] = { 'M', 'o', 'n', 'g', 'o', 'l', 'i', 'a', 'n',  0 };
static const unichar_t str482[] = { 'G', 'e', 'e', 'z',  0 };
static const unichar_t str483[] = { 'S', 'l', 'a', 'v', 'i', 'c',  0 };
static const unichar_t str484[] = { 'V', 'i', 'e', 't', 'n', 'a', 'm', 'e', 's', 'e',  0 };
static const unichar_t str485[] = { 'S', 'i', 'n', 'd', 'h', 'i',  0 };
static const unichar_t str486[] = { 'U', 'n', 'i', 'n', 't', 'e', 'r', 'p',  0 };
static const unichar_t str487[] = { 'E', 'n', 'g', 'l', 'i', 's', 'h',  0 };
static const unichar_t str488[] = { 'F', 'r', 'e', 'n', 'c', 'h',  0 };
static const unichar_t str489[] = { 'G', 'e', 'r', 'm', 'a', 'n',  0 };
static const unichar_t str490[] = { 'I', 't', 'a', 'l', 'i', 'a', 'n',  0 };
static const unichar_t str491[] = { 'D', 'u', 't', 'c', 'h',  0 };
static const unichar_t str492[] = { 'S', 'w', 'e', 'd', 'i', 's', 'h',  0 };
static const unichar_t str493[] = { 'S', 'p', 'a', 'n', 'i', 's', 'h',  0 };
static const unichar_t str494[] = { 'D', 'a', 'n', 'i', 's', 'h',  0 };
static const unichar_t str495[] = { 'P', 'o', 'r', 't', 'u', 'g', 'u', 'e', 's', 'e',  0 };
static const unichar_t str496[] = { 'N', 'o', 'r', 'w', 'e', 'g', 'i', 'a', 'n',  0 };
static const unichar_t str497[] = { 'F', 'i', 'n', 'n', 'i', 's', 'h',  0 };
static const unichar_t str498[] = { 'I', 'c', 'e', 'l', 'a', 'n', 'd', 'i', 'c',  0 };
static const unichar_t str499[] = { 'M', 'a', 'l', 't', 'e', 's', 'e',  0 };
static const unichar_t str500[] = { 'T', 'u', 'r', 'k', 'i', 's', 'h',  0 };
static const unichar_t str501[] = { 'Y', 'u', 'g', 'o', 's', 'l', 'a', 'v', 'i', 'a', 'n',  0 };
static const unichar_t str502[] = { 'U', 'r', 'd', 'u',  0 };
static const unichar_t str503[] = { 'H', 'i', 'n', 'd', 'i',  0 };
static const unichar_t str504[] = { 'A', 'l', 'b', 'a', 'n', 'i', 'a', 'n', ' ', 's', 'q', '_', 'A', 'L',  0 };
static const unichar_t str505[] = { 'A', 'r', 'a', 'b', 'i', 'c', ' ', 'a', 'r',  0 };
static const unichar_t str506[] = { 'B', 'a', 's', 'q', 'u', 'e', ' ', 'e', 'u',  0 };
static const unichar_t str507[] = { 'B', 'y', 'e', 'l', 'o', 'r', 'u', 's', 's', 'i', 'a', 'n', ' ', 'b', 'e', '_', 'B', 'Y',  0 };
static const unichar_t str508[] = { 'B', 'u', 'l', 'g', 'a', 'r', 'i', 'a', 'n', ' ', 'b', 'g', '_', 'B', 'G',  0 };
static const unichar_t str509[] = { 'C', 'a', 't', 'a', 'l', 'a', 'n', ' ', 'c', 'a',  0 };
static const unichar_t str510[] = { 'C', 'h', 'i', 'n', 'e', 's', 'e', ' ', 'z', 'h', '_', 'C', 'N',  0 };
static const unichar_t str511[] = { 'C', 'r', 'o', 'a', 't', 'i', 'a', 'n', ' ', 'h', 'r',  0 };
static const unichar_t str512[] = { 'C', 'z', 'e', 'c', 'h', ' ', 'c', 's', '_', 'C', 'Z',  0 };
static const unichar_t str513[] = { 'D', 'a', 'n', 's', 'k', ' ', 'd', 'a', '_', 'D', 'K',  0 };
static const unichar_t str514[] = { 'N', 'e', 'd', 'e', 'r', 'l', 'a', 'n', 'd', 's', ' ', 'n', 'l', '_', 'N', 'L',  0 };
static const unichar_t str515[] = { 'N', 'e', 'd', 'e', 'r', 'l', 'a', 'n', 'd', 's', ' ', 'n', 'l', '_', 'B', 'E',  0 };
static const unichar_t str516[] = { 'B', 'r', 'i', 't', 'i', 's', 'h', ' ', 'E', 'n', 'g', 'l', 'i', 's', 'h', ' ', 'e', 'n', '_', 'U', 'K',  0 };
static const unichar_t str517[] = { 'A', 'm', 'e', 'r', 'i', 'c', 'a', 'n', ' ', 'E', 'n', 'g', 'l', 'i', 's', 'h', ' ', 'e', 'n', '_', 'U', 'S',  0 };
static const unichar_t str518[] = { 'C', 'a', 'n', 'a', 'd', 'i', 'a', 'n', ' ', 'E', 'n', 'g', 'l', 'i', 's', 'h', ' ', 'e', 'n', '_', 'C', 'A',  0 };
static const unichar_t str519[] = { 'A', 'u', 's', 't', 'r', 'a', 'l', 'i', 'a', 'n', ' ', 'E', 'n', 'g', 'l', 'i', 's', 'h', ' ', 'e', 'n', '_', 'A', 'U',  0 };
static const unichar_t str520[] = { 'N', 'e', 'w', ' ', 'Z', 'e', 'e', 'l', 'a', 'n', 'd', ' ', 'E', 'n', 'g', 'l', 'i', 's', 'h', ' ', 'e', 'n', '_', 'N', 'Z',  0 };
static const unichar_t str521[] = { 'I', 'r', 'i', 's', 'h', ' ', 'E', 'n', 'g', 'l', 'i', 's', 'h', ' ', 'e', 'n', '_', 'I', 'E',  0 };
static const unichar_t str522[] = { 'E', 's', 't', 'o', 'n', 'i', 'a', 'n', ' ', 'e', 't', '_', 'E', 'E',  0 };
static const unichar_t str523[] = { 'S', 'u', 'o', 'm', 'i', ' ', 'f', 'i', '_', 'F', 'I',  0 };
static const unichar_t str524[] = { 'F', 'r', 'a', 'n', 0xe7, 'a', 'i', 's', ' ', 'F', 'r', 'a', 'n', 0xe7, 'a', 'i', 's', ' ', 'f', 'r', '_', 'F', 'R',  0 };
static const unichar_t str525[] = { 'F', 'r', 'a', 'n', 0xe7, 'a', 'i', 's', ' ', 'B', 'e', 'l', 'g', 'i', 'q', 'u', 'e', ' ', 'f', 'r', '_', 'B', 'E',  0 };
static const unichar_t str526[] = { 'F', 'r', 'a', 'n', 0xe7, 'a', 'i', 's', ' ', 'C', 'a', 'n', 'a', 'd', 'i', 'e', 'n', ' ', 'f', 'r', '_', 'C', 'A',  0 };
static const unichar_t str527[] = { 'F', 'r', 'a', 'n', 0xe7, 'a', 'i', 's', ' ', 'S', 'u', 'i', 's', 's', ' ', 'f', 'r', '_', 'C', 'H',  0 };
static const unichar_t str528[] = { 'F', 'r', 'a', 'n', 0xe7, 'a', 'i', 's', ' ', 'd', 'e', ' ', 'L', 'u', 'x', 'e', 'm', 'b', 'o', 'u', 'r', 'g', ' ', 'f', 'r', '_', 'L', 'U',  0 };
static const unichar_t str529[] = { 'D', 'e', 'u', 't', 's', 'c', 'h', ' ', 'D', 'e', 'u', 't', 's', 'c', 'h', ' ', 'd', 'e', '_', 'D', 'E',  0 };
static const unichar_t str530[] = { 'S', 'c', 'h', 'w', 'e', 'i', 'z', 'e', 'r', ' ', 'D', 'e', 'u', 't', 's', 'c', 'h', ' ', 'd', 'e', '_', 'C', 'H',  0 };
static const unichar_t str531[] = { 0xd6, 's', 't', 'e', 'r', 'r', 'e', 'i', 'c', 'h', 'i', 's', 'c', 'h', ' ', 'D', 'e', 'u', 't', 's', 'c', 'h', ' ', 'd', 'e', '_', 'A', 'T',  0 };
static const unichar_t str532[] = { 'L', 'u', 'x', 'e', 'm', 'b', 'o', 'u', 'r', 'g', ' ', 'D', 'e', 'u', 't', 's', 'c', 'h', ' ', 'd', 'e', '_', 'L', 'U',  0 };
static const unichar_t str533[] = { 'L', 'i', 'e', 'c', 'h', 't', 'e', 'n', 's', 't', 'e', 'i', 'n', ' ', 'D', 'e', 'u', 't', 's', 'c', 'h', ' ', 'd', 'e', '_', 'L', 'I',  0 };
static const unichar_t str534[] = { 'G', 'r', 'e', 'e', 'k', ' ', 'e', 'l', '_', 'G', 'R',  0 };
static const unichar_t str535[] = { 'H', 'e', 'b', 'r', 'e', 'w', ' ', 'h', 'e', '_', 'I', 'L',  0 };
static const unichar_t str536[] = { 'H', 'u', 'n', 'g', 'a', 'r', 'i', 'a', 'n', ' ', 'h', 'u', '_', 'H', 'U',  0 };
static const unichar_t str537[] = { 'I', 'c', 'e', 'l', 'a', 'n', 'd', 'i', 'c', ' ', 'i', 's', '_', 'I', 'S',  0 };
static const unichar_t str538[] = { 'I', 't', 'a', 'l', 'i', 'a', 'n', 'o', ' ', 'i', 't', '_', 'I', 'T',  0 };
static const unichar_t str539[] = { 'I', 't', 'a', 'l', 'i', 'a', 'n', 'o', ' ', 'S', 'w', 'i', 's', 's', ' ', 'i', 't', '_', 'C', 'H',  0 };
static const unichar_t str540[] = { 'J', 'a', 'p', 'a', 'n', 'e', 's', 'e', ' ', 'j', 'p', '_', 'J', 'P',  0 };
static const unichar_t str541[] = { 'L', 'a', 't', 'v', 'i', 'a', 'n', ' ', 'l', 'v', '_', 'L', 'V',  0 };
static const unichar_t str542[] = { 'L', 'i', 't', 'h', 'u', 'a', 'n', 'i', 'a', 'n', ' ', 'l', 't', '_', 'L', 'T',  0 };
static const unichar_t str543[] = { 'N', 'o', 'r', 's', 'k', ' ', 'B', 'o', 'k', 'm', 'a', 'l', ' ', 'n', 'o', '_', 'N', 'O',  0 };
static const unichar_t str544[] = { 'N', 'o', 'r', 's', 'k', ' ', 'N', 'y', 'n', 'o', 'r', 's', 'k', ' ', 'n', 'o', '_', 'N', 'O',  0 };
static const unichar_t str545[] = { 'P', 'o', 'l', 'i', 's', 'h', ' ', 'p', 'l', '_', 'P', 'L',  0 };
static const unichar_t str546[] = { 'P', 'o', 'r', 't', 'u', 'g', 'u', 0xea, 's', ' ', 'P', 'o', 'r', 't', 'u', 'g', 'u', 0xea, 's', ' ', 'p', 't', '_', 'P', 'T',  0 };
static const unichar_t str547[] = { 'P', 'o', 'r', 't', 'u', 'g', 'u', 0xea, 's', ' ', 'B', 'r', 'a', 's', 'i', 'l', ' ', 'p', 't', '_', 'B', 'R',  0 };
static const unichar_t str548[] = { 'R', 'o', 'm', 'a', 'n', 'i', 'a', 'n', ' ', 'r', 'o', '_', 'R', 'O',  0 };
static const unichar_t str549[] = { 0x420, 0x443, 0x441, 0x441, 0x43a, 0x438, 0x439, ' ', 'r', 'u', '_', 'R', 'U',  0 };
static const unichar_t str550[] = { 'S', 'l', 'o', 'v', 'a', 'k', ' ', 's', 'k', '_', 'S', 'K',  0 };
static const unichar_t str551[] = { 'S', 'l', 'o', 'v', 'e', 'n', 'i', 'a', 'n', ' ', 's', 'l', '_', 'S', 'I',  0 };
static const unichar_t str552[] = { 'E', 's', 'p', 'a', 0xf1, 'o', 'l', ' ', 'E', 's', 'p', 'a', 0xf1, 'o', 'l', ' ', 'e', 's', '_', 'E', 'S',  0 };
static const unichar_t str553[] = { 'E', 's', 'p', 'a', 0xf1, 'o', 'l', ' ', 'M', 0xe9, 'j', 'i', 'c', 'o', ' ', 'e', 's', '_', 'M', 'X',  0 };
static const unichar_t str554[] = { 'E', 's', 'p', 'a', 0xf1, 'o', 'l', ' ', 'E', 's', 'p', 'a', 0xf1, 'o', 'l', ' ', 'e', 's', '_', 'E', 'S',  0 };
static const unichar_t str555[] = { 'S', 'v', 'e', 'n', 's', 'k', ' ', 's', 'v', '_', 'S', 'E',  0 };
static const unichar_t str556[] = { 'T', 'u', 'r', 'k', 'i', 's', 'h', ' ', 't', 'r', '_', 'T', 'R',  0 };
static const unichar_t str557[] = { 'U', 'k', 'r', 'a', 'i', 'n', 'i', 'a', 'n', ' ', 'u', 'k', '_', 'U', 'A',  0 };
static const unichar_t str558[] = { 'C', 'o', 'p', 'y', 'r', 'i', 'g', 'h', 't',  0 };
static const unichar_t str559[] = { 'S', 'u', 'b', 'F', 'a', 'm', 'i', 'l', 'y',  0 };
static const unichar_t str560[] = { 'U', 'n', 'i', 'q', 'u', 'e', 'I', 'D',  0 };
static const unichar_t str561[] = { 'F', 'u', 'l', 'l', 'n', 'a', 'm', 'e',  0 };
static const unichar_t str562[] = { 'T', 'r', 'a', 'd', 'e', 'm', 'a', 'r', 'k',  0 };
static const unichar_t str563[] = { 'P', 'S', ' ', 'F', 'o', 'n', 't', 'N', 'a', 'm', 'e',  0 };
static const unichar_t str564[] = { 'M', 'a', 'n', 'u', 'f', 'a', 'c', 't', 'u', 'r', 'e', 'r',  0 };
static const unichar_t str565[] = { 'D', 'e', 's', 'i', 'g', 'n', 'e', 'r',  0 };
static const unichar_t str566[] = { 'D', 'e', 's', 'c', 'r', 'i', 'p', 't', 'o', 'r',  0 };
static const unichar_t str567[] = { 'V', 'e', 'n', 'd', 'o', 'r', ' ', 'U', 'R', 'L',  0 };
static const unichar_t str568[] = { 'D', 'e', 's', 'i', 'g', 'n', 'e', 'r', ' ', 'U', 'R', 'L',  0 };
static const unichar_t str569[] = { 'L', 'i', 'c', 'e', 'n', 's', 'e',  0 };
static const unichar_t str570[] = { 'L', 'i', 'c', 'e', 'n', 's', 'e', ' ', 'U', 'R', 'L',  0 };
static const unichar_t str571[] = { 'P', 'r', 'e', 'f', 'e', 'r', 'r', 'e', 'd', ' ', 'F', 'a', 'm', 'i', 'l', 'y',  0 };
static const unichar_t str572[] = { 'P', 'r', 'e', 'f', 'e', 'r', 'r', 'e', 'd', ' ', 'S', 't', 'y', 'l', 'e', 's',  0 };
static const unichar_t str573[] = { 'C', 'o', 'm', 'p', 'a', 't', 'a', 'b', 'l', 'e', ' ', 'F', 'u', 'l', 'l',  0 };
static const unichar_t str574[] = { 'S', 'a', 'm', 'p', 'l', 'e', ' ', 'T', 'e', 'x', 't',  0 };

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
	'E',    'u',    'l',    'D',    0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 'E',    0x0000, 0x0000, 0x0000, 
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
	0x0000, 'C',    'W',    'F',    'S',    'W',    'P',    'C',    
	'V',    'A',    'L',    'M',    'X',    0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 'I',    
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 'V',    'U',    0x0000, 0x0000, 0x0000, 0x0000, 
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
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0};

static const int ttfmod_ui_num[] = {
    55, 
    0x80000000
};

void TtfModSetFallback(void) {
    GStringSetFallbackArray(ttfmod_ui_strings,ttfmod_ui_mnemonics,ttfmod_ui_num);
}
