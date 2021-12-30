#ifndef CRC32_H
#define CRC32_H

#include <stddef.h>
#include <stdint.h>

/* Unlike crc.h, crc32.{h,c} contain an orthodox CRC32.  This implementation is
   adapted from the one by Gary S. Brown from c.snippets.org, who made it
   available for use.  The original copyright statement states:
   
   Copyright (C) 1986 Gary S. Brown.  You may use this program, or
   code or tables extracted from it, as desired without restriction.
   
   Gratitude to Gary. */
   
uint32_t crc32buf(char *buf, size_t len);

#endif