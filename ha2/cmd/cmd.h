#ifndef __CMD_PARSER_H
#define __CMD_PARSER_H

#include <stddef.h>
#include <stdbool.h>

#include "var_array.h"

struct cmd {
	const char *name;
	struct var_array *args;
};

struct var_array *cmds;

typedef enum {
	CMD_ID_WORD,
	CMD_ID_PIPE
} cmd_lex_id;

struct cmd_token {
	cmd_lex_id clid;
	char *value;
};

void cmd_parse(const char *input, size_t input_size, struct cmds **cmds);
void cmd_execute(struct cmds *cmds);

#endif
