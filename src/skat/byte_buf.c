#include "skat/byte_buf.h"
#include <stdlib.h>
#include <string.h>

void
byte_buf_create(byte_buf *this) {
  byte_buf_create_size(this, BYTE_BUF_DEFAULT_MIN_CAPACITY);
}

void
byte_buf_create_size(byte_buf *this, size_t minimum_size) {
  this->pos = this->bytes_used = 0;
  if (minimum_size == 0) {
	this->size = 0;
	this->buf = NULL;
  } else {
	this->size = round_to_next_pow2(minimum_size);
	this->buf = realloc(NULL, this->size);
  }
}

void
byte_buf_create_from_buf(byte_buf *this, size_t size, const uint8_t *buf) {
  this->pos = 0;
  this->bytes_used = size;
  if (size == 0) {
	this->size = 0;
	this->buf = NULL;
  } else {
	this->size = size;
	this->buf = realloc(NULL, this->size);
  }
  memcpy(this->buf, buf, size);
}

void
byte_buf_free(byte_buf *this) {
  if (this->size > 0) {
	this->size = 0;
	free(this->buf);
  }

  this->pos = this->bytes_used = this->size = 0;
  this->buf = NULL;
}

void
byte_buf_ensure_capacity(byte_buf *this, size_t minimum_size) {
  if (this->size < minimum_size) {
	this->size = round_to_next_pow2(minimum_size);
	this->buf = realloc(this->buf, this->size);
  }
}

void
byte_buf_trim_to_len(byte_buf *this) {
  if (this->bytes_used == 0) {
	this->size = 0;

	if (this->buf != NULL) {
	  free(this->buf);
	  this->buf = NULL;
	}
  } else {
	size_t new_size = round_to_next_pow2(this->bytes_used);
	if (this->size != new_size) {
	  this->size = new_size;
	  this->buf = realloc(this->buf, new_size);
	}
  }
}

void
byte_buf_empty(byte_buf *this) {
  this->pos = this->bytes_used = 0;
}

void
byte_buf_dump(byte_buf *this) {
  printf("byte_buf[pos=%zu,bytes_used=%zu,size=%zu]:", this->pos,
		 this->bytes_used, this->size);
  for (size_t i = 0; i < this->bytes_used; i++) {
	printf(" %02x", this->buf[i]);
  }
  printf("\n");
}

char *
byte_buf_read_str(byte_buf *this) {
  size_t len = 0;
  for (;;) {
	size_t i = this->pos + len;
	if (i >= this->bytes_used) {
	  DERROR_PRINTF("No end of string");
	  exit(EXIT_FAILURE);
	} else if (this->buf[i] == '\0') {
	  break;
	}
	len++;
  }
  char *str = strndup((char *) &this->buf[this->pos], len);
  this->pos += len + 1;
  return str;
}

void
byte_buf_write_str(byte_buf *this, const char *str) {
  byte_buf_write_strn(this, str, strlen(str));
}

void
byte_buf_write_strn(byte_buf *this, const char *str, size_t len) {
  size_t add_cap = len + 1;
  byte_buf_ensure_capacity(this, this->pos + add_cap);
  memcpy(&this->buf[this->pos], str, len);
  this->buf[this->pos + len] = '\0';
  this->bytes_used += add_cap;
  this->pos += add_cap;
}

bool
byte_buf_read_bool(byte_buf *this) {
  return byte_buf_read_i8(this) != 0;
}

void
byte_buf_write_bool(byte_buf *this, bool b) {
  byte_buf_write_i8(this, b ? 1 : 0);
}

#define DEF_READ_INT_FUN(name, ret_type, signed_type, unsigned_type) \
  ret_type name(byte_buf *this) { \
	unsigned int bits = 8 * sizeof(ret_type); \
	unsigned int max_bytes = ceil_div((int) bits, 7); \
	unsigned int superfluous_bits = (max_bytes * 7) - bits; \
	unsigned int mask = ((uint8_t) -1) << (7 - superfluous_bits); \
\
	unsigned_type zigzag = 0; \
	unsigned int used_bytes = 0; \
	uint8_t b; \
\
	do { \
	  if (this->pos >= this->bytes_used) { \
		DERROR_PRINTF("No end of var int"); \
		exit(EXIT_FAILURE); \
	  } \
	  b = this->buf[this->pos++]; \
	  uint8_t data = b & 0b01111111; \
	  if (used_bytes == max_bytes - 1 && (data & mask) != 0) { \
		DERROR_PRINTF("var int too long for " #ret_type); \
		exit(EXIT_FAILURE); \
	  } \
	  zigzag |= (((unsigned_type) data) << (7 * used_bytes++)); \
	} while ((b & 0b10000000) != 0); \
\
	return ((ret_type)(zigzag >> 1)) ^ -((ret_type)(zigzag & 1)); \
  }

#define DEF_WRITE_INT_FUN(name, param_type, signed_type, unsigned_type) \
  void name(byte_buf *this, param_type n) { \
	unsigned int bits = 8 * sizeof(n); \
\
	unsigned_type zigzag = ((unsigned_type)(((signed_type) n) >> (bits - 1))) \
						   ^ (((unsigned_type) n) << 1); \
\
	do { \
	  uint8_t b = zigzag & 0b01111111; \
	  zigzag >>= 7; \
	  if (zigzag > 0) \
		b |= 0b10000000; \
	  byte_buf_ensure_capacity(this, this->pos + 1); \
	  this->buf[this->pos++] = b; \
	  this->bytes_used++; \
	} while (zigzag > 0); \
  }

#define DEF_RW_INT_FUN(name, signed_type, unsigned_type) \
  DEF_READ_INT_FUN(CONCAT(byte_buf_read_i, name), signed_type, signed_type, \
				   unsigned_type) \
  DEF_WRITE_INT_FUN(CONCAT(byte_buf_write_i, name), signed_type, signed_type, \
					unsigned_type) \
  DEF_READ_INT_FUN(CONCAT(byte_buf_read_u, name), unsigned_type, signed_type, \
				   unsigned_type) \
  DEF_WRITE_INT_FUN(CONCAT(byte_buf_write_u, name), unsigned_type, \
					signed_type, unsigned_type)

DEF_RW_INT_FUN(8, int8_t, uint8_t)
DEF_RW_INT_FUN(16, int16_t, uint16_t)
DEF_RW_INT_FUN(32, int32_t, uint32_t)
DEF_RW_INT_FUN(64, int64_t, uint64_t)
