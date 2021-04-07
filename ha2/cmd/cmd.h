#ifndef __CMD_PARSER_H
#define __CMD_PARSER_H

#include <stddef.h>
#include <stdbool.h>

#include "var_array.h"

typedef enum {
	CMD_ID_WORD,
	CMD_ID_PIPE,
	CMD_ID_WRITE,
	CMD_ID_APPEND
} cmd_lex_id;

struct cmd_token {
	cmd_lex_id clid;
	char *value;
};

struct cmd {
	const char *name;
	struct var_array *args;
	// only CMD_ID_WRITE and CMD_ID_APPEND allowed
	cmd_lex_id write_mode;
	const char *output_file;
};

struct var_array *cmds;

typedef void (*__cmd_func)();

void __cmd_about();
void __cmd_cd(char *dst);
__cmd_func __cmd_is_builtin(char *cmd_name);

void cmd_parse(const char *input, size_t input_size, struct var_array **cmds);
void cmd_execute(struct var_array *cmds);

extern const char *__cmd_builtin[];
extern __cmd_func __cmd_builtin_func[];

#define __cmd_builtin_len() (sizeof(__cmd_builtin)/sizeof(const char*))

#endif
