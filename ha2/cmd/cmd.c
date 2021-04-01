#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "cmd.h"
#include "var_array.h"

void cmd_parse(const char *input, size_t input_size, struct var_array **cmds) {
	// p points to the beginning of a lexeme
	// i iterates over the input string looking for lexeme ending
	size_t lex_begin = 0;
	size_t it = 0;
	bool is_lexeme = false;

	*cmds = var_array_init(8, sizeof(struct cmd));
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

	int t_it = 0;
	while(t_it < var_array_len(token_list)) {
		struct cmd new_cmd = {
			.name = NULL,
			.args = var_array_init(8, sizeof(char*))
		};

		while(t_it < var_array_len(token_list)) {
			struct cmd_token tk = var_array_get(token_list, t_it, struct cmd_token);

			if(tk.clid == CMD_ID_PIPE) {
				t_it++;
				break;
			}

			if(new_cmd.name == NULL)
				new_cmd.name = tk.value;
			else
				var_array_put(new_cmd.args, tk.value, char*);

			t_it++;
		}

		var_array_put((*cmds), new_cmd, struct cmd);
	}
}

void cmd_execute(struct var_array *cmds) {
	printf("parsed commnds:\n");

	for(int i = 0; i < var_array_len(cmds); i++) {
		struct cmd c = var_array_get(cmds, i, struct cmd);
		printf("name: %s\n", c.name);

		for(int j = 0; j < var_array_len(c.args); j++) {
			char *arg = var_array_get(c.args, j, char*);
			printf("%s\n", arg);
		}

		printf("--------------\n");
	}
}
