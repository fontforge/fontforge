#include "gdraw.h"

static uint8 savedimage0_data[] = {
    0xc7, 
    0xbb, 
    0x6d, 
    0x45, 
    0x6d, 
    0xbb, 
    0xc3, 
    0xfb, 
    0xfd, 
    0xfd, 
};

static struct _GImage savedimage0_base = {
    it_mono,
    2069,8,10,1,
    (uint8 *) savedimage0_data,
    NULL,
    0xffffffff
};

GImage savedimage = { 0, &savedimage0_base };
