#include "skat/command.h"
#include "skat/util.h"
#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int
command_create(command **const cmd, const char *const line, size_t length) {
  if (line[length - 1] == '\n')
	length--;

  char *buf = malloc(length + 1);
  memcpy(buf, line, length);
  buf[length] = '\0';

  char *save_ptr = NULL, *token = strtok_r(buf, " ", &save_ptr);
  if (token == NULL)
	return 1;

  char *command_str = strdup(token);

  size_t current_args_length = 16;
  char **args = malloc(current_args_length * sizeof(char *));

  size_t args_length = 0;
  while ((token = strtok_r(NULL, " ", &save_ptr)) != NULL) {
	if (args_length >= current_args_length) {
	  size_t new_args_length = 2 * current_args_length;
	  args = realloc(args, new_args_length * sizeof(char *));
	}

	args[args_length++] = strdup(token);
  }

  args = realloc(args, args_length * sizeof(char *));

  command *cmd_ = malloc(sizeof(command) + args_length * sizeof(char *));
  if (cmd_ == NULL)
	return 2;

  cmd_->command = command_str;
  cmd_->args_length = args_length;
  if (args_length > 0)
	memcpy(cmd_->args, args, args_length * sizeof(char *));

  *cmd = cmd_;
  return 0;
}

int
command_free(command *const cmd) {
  free(cmd->command);
  for (size_t i = 0; i < cmd->args_length; i++)
	free(cmd->args[i]);
  free(cmd);
  return 0;
}

int
command_equals(const command *cmd, int *result, size_t name_count, ...) {
  int equals = 0;

  va_list ap;
  va_start(ap, name_count);
  for (size_t i = 0; i < name_count; ++i) {
	const char *const str = va_arg(ap, char *);
	equals = !strcmp(cmd->command, str);
	if (equals)
	  break;
  }
  va_end(ap);

  *result = equals;
  return 0;
}

int
command_arg_equals(const command *cmd, int print_errors, size_t index,
				   int *result, size_t name_count, ...) {
  if (index >= cmd->args_length) {
	if (print_errors)
	  fprintf(stderr, "Invalid arg index: got %zu but length was %zu\n", index,
			  cmd->args_length);
	return 1;
  }

  int equals = 0;

  va_list ap;
  va_start(ap, name_count);
  for (size_t i = 0; i < name_count; ++i) {
	const char *const str = va_arg(ap, char *);
	equals = !strcmp(cmd->command, str);
	if (equals)
	  break;
  }
  va_end(ap);

  *result = equals;
  return 0;
}

int
command_check_arg_length(const command *cmd, size_t expected_args,
						 int *result) {
  *result = cmd->args_length == expected_args;
  return 0;
}

int
command_parse_arg_u64(const command *cmd, int print_errors, size_t index,
					  uint64_t min, uint64_t max, uint64_t *result) {
  if (index >= cmd->args_length) {
	fprintf(stderr, "Invalid arg index: got %zu but length was %zu\n", index,
			cmd->args_length);
	return 1;
  }

  const char *const arg = cmd->args[index];

  if (arg[0] == '\0') {
	if (print_errors)
	  fprintf(stderr, "Invalid arg, got NULL\n");
	return 2;
  }

  errno = 0;
  char *end;
  unsigned long long int result_ = strtoull(arg, &end, 10);

  if (errno != 0) {
	if (print_errors)
	  fprintf(stderr, "Unable to parse arg '%s': %s\n", arg, strerror(errno));
	return 3;
  } else if (end[0] != '\0') {
	if (print_errors)
	  fprintf(stderr,
			  "Unable to parse arg '%s' fully, still left to parse: '%s'\n",
			  arg, end);
	return 4;
  } else if (result_ < min || result_ > max) {
	if (print_errors)
	  fprintf(stderr,
			  "Parsed arg '%llu' is out of range, min is %" PRIu64
			  " and max is %" PRIu64 "\n",
			  result_, min, max);
	return 5;
  }

  *result = (uint64_t) result_;
  return 0;
}

int
command_parse_arg_u32(const command *cmd, int print_errors, size_t index,
					  uint32_t min, uint32_t max, uint32_t *result) {
  uint64_t result_;
  int error =
		  command_parse_arg_u64(cmd, print_errors, index, min, max, &result_);
  if (error)
	return error;

  *result = (uint32_t) result_;
  return 0;
}

int
command_parse_arg_u16(const command *const cmd, int print_errors,
					  const size_t index, const uint16_t min,
					  const uint16_t max, uint16_t *const result) {
  uint64_t result_;
  int error =
		  command_parse_arg_u64(cmd, print_errors, index, min, max, &result_);
  if (error)
	return error;

  *result = (uint16_t) result_;
  return 0;
}

int
command_parse_arg_u8(const command *const cmd, int print_errors,
					 const size_t index, const uint8_t min, const uint8_t max,
					 uint8_t *const result) {
  uint64_t result_;
  int error =
		  command_parse_arg_u64(cmd, print_errors, index, min, max, &result_);
  if (error)
	return error;

  *result = (uint8_t) result_;
  return 0;
}

int
command_parse_arg_i64(const command *cmd, int print_errors, size_t index,
					  int64_t min, int64_t max, int64_t *result) {
  if (index >= cmd->args_length) {
	fprintf(stderr, "Invalid arg index: got %zu but length was %zu\n", index,
			cmd->args_length);
	return 1;
  }

  const char *const arg = cmd->args[index];

  if (arg[0] == '\0') {
	if (print_errors)
	  fprintf(stderr, "Invalid arg, got NULL\n");
	return 2;
  }

  errno = 0;
  char *end;
  long long int result_ = strtoll(arg, &end, 10);

  if (errno != 0) {
	if (print_errors)
	  fprintf(stderr, "Unable to parse arg '%s': %s\n", arg, strerror(errno));
	return 3;
  } else if (end[0] != '\0') {
	if (print_errors)
	  fprintf(stderr,
			  "Unable to parse arg '%s' fully, still left to parse: '%s'\n",
			  arg, end);
	return 4;
  } else if (result_ < min || result_ > max) {
	if (print_errors)
	  fprintf(stderr,
			  "Parsed arg '%lld' is out of range, min is %" PRId64
			  " and max is %" PRId64 "\n",
			  result_, min, max);
	return 5;
  }

  *result = (int64_t) result_;
  return 0;
}

int
command_parse_arg_i32(const command *const cmd, int print_errors,
					  const size_t index, const int32_t min, const int32_t max,
					  int32_t *const result) {
  int64_t result_;
  int error =
		  command_parse_arg_i64(cmd, print_errors, index, min, max, &result_);
  if (error)
	return error;

  *result = (int32_t) result_;
  return 0;
}

int
command_parse_arg_i16(const command *const cmd, int print_errors,
					  const size_t index, const int16_t min, const int16_t max,
					  int16_t *const result) {
  int64_t result_;
  int error =
		  command_parse_arg_i64(cmd, print_errors, index, min, max, &result_);
  if (error)
	return error;

  *result = (int16_t) result_;
  return 0;
}

int
command_parse_arg_i8(const command *const cmd, int print_errors,
					 const size_t index, const int8_t min, const int8_t max,
					 int8_t *const result) {
  int64_t result_;
  int error =
		  command_parse_arg_i64(cmd, print_errors, index, min, max, &result_);
  if (error)
	return error;

  *result = (int8_t) result_;
  return 0;
}

int
command_parse_arg_ld(const command *const cmd, int print_errors,
					 const size_t index, const long double min,
					 const long double max, long double *const result) {
  if (index >= cmd->args_length) {
	if (print_errors)
	  fprintf(stderr, "Invalid arg index: got %zu but length was %zu\n", index,
			  cmd->args_length);
	return 1;
  }

  const char *const arg = cmd->args[index];

  if (arg[0] == '\0') {
	if (print_errors)
	  fprintf(stderr, "Invalid arg, got NULL\n");
	return 2;
  }

  errno = 0;
  char *end;
  long double result_ = strtold(arg, &end);

  if (errno != 0) {
	if (print_errors)
	  fprintf(stderr, "Unable to parse arg '%s': %s\n", arg, strerror(errno));
	return 3;
  } else if (end[0] != '\0') {
	if (print_errors)
	  fprintf(stderr,
			  "Unable to parse arg '%s' fully, still left to parse: '%s'\n",
			  arg, end);
	return 4;
  } else if (result_ < min || result_ > max) {
	if (print_errors)
	  fprintf(stderr,
			  "Parsed arg '%Lf' is out of range, min is %Lf and max is %Lf\n",
			  result_, min, max);
	return 5;
  }

  *result = result_;
  return 0;
}

int
command_parse_arg_d(const command *const cmd, int print_errors,
					const size_t index, const double min, const double max,
					double *const result) {
  long double result_;
  int error =
		  command_parse_arg_ld(cmd, print_errors, index, min, max, &result_);
  if (error)
	return error;

  *result = (double) result_;
  return 0;
}


int
command_parse_arg_f(const command *const cmd, int print_errors,
					const size_t index, const float min, const float max,
					float *const result) {
  long double result_;
  int error =
		  command_parse_arg_ld(cmd, print_errors, index, min, max, &result_);
  if (error)
	return error;

  *result = (float) result_;
  return 0;
}