#include "skat/utf8.h"
#include "skat/util.h"
#include <stdlib.h>

int
utf8_valid(const char *utf8_str) {
  const unsigned char *str = (const unsigned char *) utf8_str;

  // better be safe than sorry ;)
  if (!str)
	return 0;
  else if ((str[0] & 0b11111000u) == 0b11110000u
		   && (str[1] & 0b11000000u) == 0b10000000u
		   && (str[2] & 0b11000000u) == 0b10000000u
		   && (str[3] & 0b11000000u) == 0b10000000u)// 4 bytes
	return 1;
  else if ((str[0] & 0b11110000u) == 0b11100000u
		   && (str[1] & 0b11000000u) == 0b10000000u
		   && (str[2] & 0b11000000u) == 0b10000000u)// 3 bytes
	return 1;
  else if ((str[0] & 0b11100000u) == 0b11000000u
		   && (str[1] & 0b11000000u) == 0b10000000u)// 2 bytes
	return 1;
  else if ((str[0] & 0b10000000u) == 0b00000000u)// 1 byte
	return 1;
  else// invalid utf8 codepoint
	return 0;
}

const char *
utf8_codepoint(const char *utf8_str, unicode_codepoint_t *out_cp) {
  const unsigned char *str = (const unsigned char *) utf8_str;

  // better be safe than sorry ;)
  if (!str) {
	DERROR_PRINTF("null pointer");
	exit(1);
  } else if ((str[0] & 0b11111000u) == 0b11110000u
			 && (str[1] & 0b11000000u) == 0b10000000u
			 && (str[2] & 0b11000000u) == 0b10000000u
			 && (str[3] & 0b11000000u) == 0b10000000u) {// 4 bytes
	if (out_cp)
	  *out_cp = ((str[0] & 0b00000111u) << 18u)
				| ((str[1] & 0b00111111u) << 12u)
				| ((str[2] & 0b00111111u) << 6u) | (str[3] & 0b00111111u);
	utf8_str += 4;
  } else if ((str[0] & 0b11110000u) == 0b11100000u
			 && (str[1] & 0b11000000u) == 0b10000000u
			 && (str[2] & 0b11000000u) == 0b10000000u) {// 3 bytes
	if (out_cp)
	  *out_cp = ((str[0] & 0b00001111u) << 12u) | ((str[1] & 0b00111111u) << 6u)
				| (str[2] & 0b00111111u);
	utf8_str += 3;
  } else if ((str[0] & 0b11100000u) == 0b11000000u
			 && (str[1] & 0b11000000u) == 0b10000000u) {// 2 bytes
	if (out_cp)
	  *out_cp = ((str[0] & 0b00011111u) << 6u) | (str[1] & 0b00111111u);
	utf8_str += 2;
  } else if ((str[0] & 0b10000000u) == 0b00000000u) {// 1 byte
	if (out_cp)
	  *out_cp = (str[0] & 0b01111111u);
	utf8_str++;
  } else {// invalid utf8 codepoint
	DERROR_PRINTF("Found an invalid utf8 codepoint");
	exit(1);
  }

  return utf8_str;
}

void
utf8_convert_unicode_codepoint(unicode_codepoint_t cp, char *out_utf8_cp) {
  if (!out_utf8_cp) {
	DERROR_PRINTF("null pointer");
	exit(1);
  } else if ((cp & 0xffe00000u) != 0) {// >21 bits, invalid
	DERROR_PRINTF("Given codepoint is invalid");
	exit(1);
  } else if ((cp & 0x001f0000u) != 0) {// 17-21 bits, 4 bytes
	out_utf8_cp[0] = (char) (0b11110000u | ((cp >> 18u) & 0b00000111u));
	out_utf8_cp[1] = (char) (0b10000000u | ((cp >> 12u) & 0b00111111u));
	out_utf8_cp[2] = (char) (0b10000000u | ((cp >> 6u) & 0b00111111u));
	out_utf8_cp[3] = (char) (0b10000000u | (cp & 0b00111111u));
  } else if ((cp & 0x0000f800u) != 0) {// 12-16 bits, 3 bytes
	out_utf8_cp[0] = (char) (0b11100000u | ((cp >> 12u) & 0b00001111u));
	out_utf8_cp[1] = (char) (0b10000000u | ((cp >> 6u) & 0b00111111u));
	out_utf8_cp[2] = (char) (0b10000000u | (cp & 0b00111111u));
	out_utf8_cp[3] = '\0';
  } else if ((cp & 0x00000e80u) != 0) {// 8-11 bits, 2 bytes
	out_utf8_cp[0] = (char) (0b11000000u | ((cp >> 6u) & 0b00011111u));
	out_utf8_cp[1] = (char) (0b10000000u | (cp & 0b00111111u));
	out_utf8_cp[2] = out_utf8_cp[3] = '\0';
  } else {// 0-7 bits, 1 byte
	out_utf8_cp[0] = (char) (0b00000000u | (cp & 0b01111111u));
	out_utf8_cp[1] = out_utf8_cp[2] = out_utf8_cp[3] = '\0';
  }
}

size_t
utf8_length(const char *utf8_str) {
  if (!utf8_str) {
	DERROR_PRINTF("null pointer");
	exit(1);
  }

  size_t length = 0;

  for (const char *next = utf8_str; *next; length++)
	next = utf8_codepoint(next, NULL);

  return length;
}