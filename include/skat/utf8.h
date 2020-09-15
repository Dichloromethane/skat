#pragma once

#include <inttypes.h>
#include <stddef.h>

typedef uint32_t unicode_codepoint_t;
#define PRIUNICODE PRIu32
#define PRIxUNICODE PRIx32
#define PRIXUNICODE PRIX32

int utf8_valid(const char *utf8_str);
const char *utf8_codepoint(const char *utf8_str, unicode_codepoint_t *out_cp);

void utf8_convert_unicode_codepoint(unicode_codepoint_t cp, char *out_utf8_cp);

size_t utf8_length(const char *utf8_str);
