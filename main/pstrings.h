#ifndef PSTRINGS_H
#define PSTRINGS_H

#include <stddef.h>

/* pstrings.{c,h} includes utilities to help deal with Pascal strings as
   used all over the place in AppleTalk. */
   
char* pstrncpy(char* dst, unsigned char* src, size_t length);
size_t pstrlen(unsigned char* s);

#endif