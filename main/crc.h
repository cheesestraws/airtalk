#ifndef CRC_H
#define CRC_H

#include <stdbool.h>
#include <stdint.h>

/* crc.h and crc.c implement the eccentric version of the CRC algorithm as
   specified by SDLC and thus LLAP.  This implementation is an almost direct
   crib from Tashtari's example implementation from tashtalkd. */

typedef uint16_t crc_state_t;

// crc_state_init initialises, *but does not allocate*, a CRC state.  Create
// your crc_state_t by another means (e.g. on the stack) first.
void crc_state_init(crc_state_t* state);

// crc_state_append and crc_state_append_all add either a single character or
// a string to the CRC state, respectively
void crc_state_append(crc_state_t* state, unsigned char b);
void crc_state_append_all(crc_state_t* state, unsigned char* buf, int len);

// crc_state_ok returns true if the characters appended to the CRC state make
// up a valid LLAP frame
bool crc_state_ok(crc_state_t* state);

// crc_state_byte_1 and _2 return the two bytes of the two-byte LLAP CRC of the
// payload in the CRC state.
unsigned char crc_state_byte_1(crc_state_t* state);
unsigned char crc_state_byte_2(crc_state_t* state);

#endif