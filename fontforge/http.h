#ifndef FONTFORGE_HTTP_H
#define FONTFORGE_HTTP_H

#include <stdio.h>

extern FILE *URLToTempFile(char *url, void *_lock);
extern int URLFromFile(const char *url, FILE *from);

#endif /* FONTFORGE_HTTP_H */
