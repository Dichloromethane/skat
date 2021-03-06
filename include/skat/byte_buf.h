#pragma once

#include "skat/str_buf.h"
#include "skat/util.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BYTE_BUF_DEFAULT_MIN_CAPACITY (16)

typedef enum byte_buf_type {
  BB_TYPE_INVALID = 0,
  BB_TYPE_I8,
  BB_TYPE_U8,
  BB_TYPE_VAR_I16,
  BB_TYPE_VAR_I32,
  BB_TYPE_VAR_I64,
  BB_TYPE_VAR_U16,
  BB_TYPE_VAR_U32,
  BB_TYPE_VAR_U64,
  BB_TYPE_BOOL,
  BB_TYPE_STR
} byte_buf_type;

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

void byte_buf_dump(const byte_buf *this, str_buf *buf);

char *byte_buf_read_str(byte_buf *this);
void byte_buf_write_str(byte_buf *this, const char *str);
void byte_buf_write_strn(byte_buf *this, const char *str, size_t len);

bool byte_buf_read_bool(byte_buf *this);
void byte_buf_write_bool(byte_buf *this, bool b);

int8_t byte_buf_read_i8(byte_buf *this);
void byte_buf_write_i8(byte_buf *this, int8_t n);

uint8_t byte_buf_read_u8(byte_buf *this);
void byte_buf_write_u8(byte_buf *this, uint8_t n);

#define DECL_RW_INT_FUN(name, type) \
  type CONCAT(byte_buf_read_, name)(byte_buf * this); \
  void CONCAT(byte_buf_write_, name)(byte_buf * this, type n);

DECL_RW_INT_FUN(i16, int16_t)
DECL_RW_INT_FUN(i32, int32_t)
DECL_RW_INT_FUN(i64, int64_t)
DECL_RW_INT_FUN(u16, uint16_t)
DECL_RW_INT_FUN(u32, uint32_t)
DECL_RW_INT_FUN(u64, uint64_t)
