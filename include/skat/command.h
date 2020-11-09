#pragma once

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
  char *command;
  size_t args_length;
  char *args[];
} command;

int command_create(command **cmd, const char *line, size_t length);
int command_free(command *cmd);

int command_equals(const command *cmd, int *result, size_t name_count, ...);
int command_arg_equals(const command *cmd, int print_errors, size_t index,
					   int *result, size_t name_count, ...);
int command_check_arg_length(const command *cmd, size_t expected_args,
							 int *result);
int command_args_contain(const command *cmd, size_t index, int *result,
						 size_t *result_index, size_t name_count, ...);

int command_parse_arg_u64(const command *cmd, int print_errors, size_t index,
						  uint64_t min, uint64_t max, uint64_t *result);
int command_parse_arg_u32(const command *cmd, int print_errors, size_t index,
						  uint32_t min, uint32_t max, uint32_t *result);
int command_parse_arg_u16(const command *cmd, int print_errors, size_t index,
						  uint16_t min, uint16_t max, uint16_t *result);
int command_parse_arg_u8(const command *cmd, int print_errors, size_t index,
						 uint8_t min, uint8_t max, uint8_t *result);

int command_parse_arg_i64(const command *cmd, int print_errors, size_t index,
						  int64_t min, int64_t max, int64_t *result);
int command_parse_arg_i32(const command *cmd, int print_errors, size_t index,
						  int32_t min, int32_t max, int32_t *result);
int command_parse_arg_i16(const command *cmd, int print_errors, size_t index,
						  int16_t min, int16_t max, int16_t *result);
int command_parse_arg_i8(const command *cmd, int print_errors, size_t index,
						 int8_t min, int8_t max, int8_t *result);

int command_parse_arg_ld(const command *cmd, int print_errors, size_t index,
						 long double min, long double max, long double *result);
int command_parse_arg_d(const command *cmd, int print_errors, size_t index,
						double min, double max, double *result);
int command_parse_arg_f(const command *cmd, int print_errors, size_t index,
						float min, float max, float *result);
