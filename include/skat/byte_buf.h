#pragma once

#include "skat/util.h"
#include <stddef.h>
#include <stdint.h>

#define BYTE_BUF_DEFAULT_MIN_CAPACITY (16)

typedef struct byte_buf {
  size_t pos;       // position in buffer
  size_t bytes_used;// actual buffer length
  size_t size;      // buffer size
  uint8_t *buf;
} byte_buf;

void byte_buf_create(byte_buf *this);
void byte_buf_create_size(byte_buf *this, size_t minimum_size);
void byte_buf_create_from_buf(byte_buf *this, size_t size, const uint8_t *buf);
void byte_buf_free(byte_buf *this);

void byte_buf_ensure_capacity(byte_buf *this, size_t minimum_size);
void byte_buf_trim_to_len(byte_buf *this);

void byte_buf_empty(byte_buf *this);

char *byte_buf_read_str(byte_buf *this);
void byte_buf_write_str(byte_buf *this, const char *str);
void byte_buf_write_strn(byte_buf *this, const char *str, size_t len);

#define DECL_RW_INT_FUN(name, type) \
  type CONCAT(byte_buf_read_, name)(byte_buf * this); \
  void CONCAT(byte_buf_write_, name)(byte_buf * this, type n);

DECL_RW_INT_FUN(i8, int8_t)
DECL_RW_INT_FUN(i16, int16_t)
DECL_RW_INT_FUN(i32, int32_t)
DECL_RW_INT_FUN(i64, int64_t)
DECL_RW_INT_FUN(u8, uint8_t)
DECL_RW_INT_FUN(u16, uint16_t)
DECL_RW_INT_FUN(u32, uint32_t)
DECL_RW_INT_FUN(u64, uint64_t)
