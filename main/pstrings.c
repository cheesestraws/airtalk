#include "pstrings.h"

#include <stdint.h>
#include <string.h>

char* pstrncpy(char* dst, unsigned char* src, size_t length) {
	size_t plen = src[0];
	
	if (length < plen) {
		plen = length;
	}
	
	memmove(dst, src+1, plen);
	dst[plen] = '\0';
	
	return dst;
}

size_t pstrlen(unsigned char* s) {
	return s[0];
}
