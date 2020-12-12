#pragma once

#include <stdarg.h>
#include <stddef.h>

#define STR_BUF_DEFAULT_MIN_CAPACITY (16)

typedef struct {
  size_t len; // actual string length
  size_t size;// buffer size
  char *buf;
} str_buf;

void str_buf_new_empty(str_buf *sb);
void str_buf_new_size(str_buf *sb, size_t minimum_size);
void str_buf_new_from_char(str_buf *sb, const char *initial_str);
void str_buf_free(str_buf *sb);

void str_buf_ensure_capacity(str_buf *sb, size_t minimum_size);
void str_buf_trim_to_len(str_buf *sb);

void str_buf_empty(str_buf *sb);

size_t str_buf_utf8_length(const str_buf *sb);

/*
void str_buf_set(str_buf *sb, const char *str);
void str_buf_n_set(str_buf *sb, const char *str, size_t bytes_used);

void str_buf_replace(str_buf *sb, const char *str, size_t index);
void str_buf_n_replace(str_buf *sb, const char *str, size_t bytes_used, size_t
index);
*/

void str_buf_append(str_buf *sb_existing, const str_buf *str_buf_new);
void str_buf_append_char(str_buf *sb_existing, char c);
void str_buf_append_str(str_buf *sb_existing, const char *str);
void str_buf_append_strf(str_buf *sb_existing, const char *fmt, ...);
void str_buf_append_n_str(str_buf *sb_existing, const char *str, size_t len);

void str_buf_remove(str_buf *sb, size_t amount);
