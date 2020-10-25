#pragma once

#include <stddef.h>

typedef struct {
  char *command;
  size_t args_length;
  char *args[];
} console_command;

console_command *console_command_create(char *line, size_t length);
void console_command_free(console_command *cmd);
