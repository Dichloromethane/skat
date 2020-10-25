#include "skat/console_command.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

console_command *
console_command_create(char *line, size_t length) {
  if (line[length - 1] == '\n')
	length--;

  char *buf = malloc(length + 1);
  memcpy(buf, line, length);
  buf[length] = '\0';

  char *save_ptr = NULL, *token = strtok_r(buf, " ", &save_ptr);
  if (token == NULL)
	return NULL;

  char *command = strdup(token);

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

  console_command *cmd =
		  malloc(sizeof(console_command) + args_length * sizeof(char *));

  cmd->command = command;
  cmd->args_length = args_length;
  if (args_length > 0)
	memcpy(cmd->args, args, args_length * sizeof(char *));

  return cmd;
}

void
console_command_free(console_command *cmd) {
  free(cmd->command);
  for (size_t i = 0; i < cmd->args_length; i++)
	free(cmd->args[i]);
  free(cmd);
}
