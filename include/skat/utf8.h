#pragma once

#include <stddef.h>
#include <stdint.h>

typedef uint32_t unicode_codepoint_t;

int utf8_valid(const char *utf8_str);
const char *utf8_codepoint(const char *utf8_str, unicode_codepoint_t *out_cp);

void utf8_convert_unicode_codepoint(unicode_codepoint_t cp, char *out_utf8_cp);

size_t utf8_length(const char *utf8_str);
