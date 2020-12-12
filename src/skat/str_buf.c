#include "skat/str_buf.h"
#include "skat/utf8.h"
#include "skat/util.h"
#include <stdlib.h>
#include <string.h>

void
str_buf_new_empty(str_buf *sb) {
  str_buf_new_size(sb, STR_BUF_DEFAULT_MIN_CAPACITY);
}

void
str_buf_new_size(str_buf *sb, size_t minimum_size) {
  sb->len = 0;
  if (minimum_size == 0) {
	sb->size = 0;
	sb->buf = NULL;
  } else {
	sb->size = round_to_next_pow2(minimum_size);
	sb->buf = realloc(NULL, sb->size * sizeof(char));
	sb->buf[0] = '\0';
  }
}

void
str_buf_new_from_char(str_buf *sb, const char *initial_str) {
  sb->len = strlen(initial_str);
  sb->size = round_to_next_pow2(sb->len + 1);
  sb->buf = realloc(NULL, sb->size * sizeof(char));

  strncpy(sb->buf, initial_str, sb->len);
  sb->buf[sb->len] = '\0';
}

void
str_buf_free(str_buf *sb) {
  if (sb->size > 0) {
	sb->size = 0;
	free(sb->buf);
  }

  sb->len = sb->size = 0;
  sb->buf = NULL;
}

void
str_buf_ensure_capacity(str_buf *sb, size_t minimum_size) {
  if (sb->size < minimum_size) {
	sb->size = round_to_next_pow2(minimum_size);
	sb->buf = realloc(sb->buf, sb->size * sizeof(char));
  }
}

void
str_buf_trim_to_len(str_buf *sb) {
  if (sb->len == 0) {
	sb->size = 0;

	if (sb->buf != NULL) {
	  free(sb->buf);
	  sb->buf = NULL;
	}
  } else {
	size_t new_size = round_to_next_pow2(sb->len + 1);
	if (sb->size != new_size) {
	  sb->size = new_size;
	  sb->buf = realloc(sb->buf, new_size * sizeof(char));
	}
  }
}

void
str_buf_empty(str_buf *sb) {
  sb->len = 0;
  sb->buf[0] = '\0';
}

size_t
str_buf_utf8_length(const str_buf *sb) {
  return utf8_length(sb->buf);
}

/*
void
str_buf_set(str_buf *sb, const char *str) {
  str_buf_n_set(sb, str, strlen(str));
}

void
str_buf_n_set(str_buf *sb, const char *str, size_t bytes_used) {
  sb->bytes_used = bytes_used;

  str_buf_ensure_capacity(sb, sb->bytes_used + 1);

  if (bytes_used > 0)
	strncpy(sb->buf, str, sb->bytes_used);
  sb->buf[sb->bytes_used] = '\0';
}

void
str_buf_replace(str_buf *sb, const char *str, size_t index) {
  str_buf_n_replace(sb, str, strlen(str), index);
}

void
str_buf_n_replace(str_buf *sb, const char *str, size_t bytes_used, size_t index)
{ if (index >= sb->bytes_used) { DERROR_PRINTF("Cannot replace outside of
str_buf"); exit(EXIT_FAILURE);
  }

  if (bytes_used == 0)
	return;

  sb->bytes_used = MAX(sb->bytes_used, index + bytes_used);

  str_buf_ensure_capacity(sb, sb->bytes_used + 1);

  strncpy(&sb->buf[index], str, bytes_used);
  sb->buf[sb->bytes_used] = '\0';
}
*/

void
str_buf_append(str_buf *sb_existing, const str_buf *str_buf_new) {
  str_buf_append_n_str(sb_existing, str_buf_new->buf, str_buf_new->len);
}

void
str_buf_append_char(str_buf *sb_existing, char c) {
  str_buf_append_n_str(sb_existing, &c, 1);
}

void
str_buf_append_str(str_buf *sb_existing, const char *str) {
  str_buf_append_n_str(sb_existing, str, strlen(str));
}

void
str_buf_append_strf(str_buf *sb_existing, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  char *str;
  size_t len = vasprintf(&str, fmt, ap);
  va_end(ap);

  str_buf_append_n_str(sb_existing, str, len);
}

void
str_buf_append_n_str(str_buf *sb_existing, const char *str, size_t len) {
  size_t i;
  for (i = 0; i < len && str[i] != '\0'; i++) {
	size_t minimum_size = sb_existing->len + i + 1;
	str_buf_ensure_capacity(sb_existing, minimum_size);

	sb_existing->buf[sb_existing->len + i] = str[i];
  }

  sb_existing->len += i;
  sb_existing->buf[sb_existing->len] = '\0';
}

void
str_buf_remove(str_buf *sb, size_t amount) {
  for (size_t i = 0; i < amount; i++) {
	unsigned int j;
	for (j = 1; j <= 4; j++) {
	  if (utf8_valid(&sb->buf[sb->len - j]))
		break;
	}

	if (j > sb->len) {
	  DERROR_PRINTF("Cannot remove more than buffer length");
	  exit(EXIT_FAILURE);
	}

	sb->len -= j;
	sb->buf[sb->len] = '\0';
  }
}
