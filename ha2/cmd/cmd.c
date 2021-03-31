#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "cmd.h"
#include "var_array.h"

void cmd_parse(const char *input, size_t input_size, struct cmds **cmds) {
	// p points to the beginning of a lexeme
	// i iterates over the input string looking for lexeme ending
	size_t lex_begin = 0;
	size_t it = 0;
	bool is_lexeme = false;

	struct var_array *token_list = var_array_init(8, sizeof(struct cmd_token));

	while(it < input_size && input[it] != '\0') {
		if(is_lexeme) {
			// skipping to the end of current lexeme
			while(it < input_size && !isspace(input[it])) it++;

			struct cmd_token new_token;
			new_token.value = malloc(sizeof(char) * (it - lex_begin + 1));
			strncpy(new_token.value, &input[lex_begin], it - lex_begin);

			if(strcmp(new_token.value, "|") == 0)
				new_token.clid = CMD_ID_PIPE;
			else
				new_token.clid = CMD_ID_WORD;

			var_array_put(token_list, new_token, struct cmd_token);

			is_lexeme = false;
		} else {
			// we've just met lexeme
			if(!isspace(input[it])) {
				is_lexeme = true;
				lex_begin = it;
			}

			// or we're iterating over sequence of whitespaces doing nothing
		}

		it++;
	}

	if(var_array_len(token_list) == 0)
		return;

	*cmds = var_array_new(8, struct cmd);
	var_array_put(cmds, struct cmd {}, struct cmd);

	for(int i = 0; i < var_array_len(token_list); i++) {
		struct cmd_token it = var_array_get(token_list, i, struct cmd_token);
		printf("[token] %s:%s\n", (it.clid == CMD_ID_WORD) ? "word" : "pipe", it.value);

		
	}
}

void cmd_execute(struct cmds *cmds) {
}
