#define _GNU_SOURCE
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

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

			var_array_put(new_cmd.args, tk.value, char*);

			t_it++;
		}

		var_array_put((*cmds), new_cmd, struct cmd);
	}
}

void cmd_execute(struct var_array *cmds) {
	int fd_pipes[var_array_len(cmds) + 1][2];
	pid_t child_pids[var_array_len(cmds)];
	int status = 0;

	for(int i = 0; i < var_array_len(cmds) + 1; i++)
		pipe2(fd_pipes[i], O_CLOEXEC);

	close(fd_pipes[0][1]);
	for(int i = 0; i < var_array_len(cmds); i++) {
		struct cmd c = var_array_get(cmds, i, struct cmd);

		if((child_pids[i] = fork()) == 0) {
			dup2(fd_pipes[i][0], 0);
			dup2(fd_pipes[i+1][1], 1);
			execvp(c.name, var_array_raw(c.args, char*));
		}
	}

	// waiting for all children to complete execution
	for(int i = 0; i < var_array_len(cmds); i++) {
		waitpid(child_pids[i], &status, 0);
		close(fd_pipes[i][0]);
		close(fd_pipes[i+1][1]);
	}

	char buf[1024];
	size_t buf_size;
	while((buf_size = read(fd_pipes[var_array_len(cmds)][0], buf, 1024)) > 0)
		write(STDOUT_FILENO, buf, buf_size);

	close(fd_pipes[var_array_len(cmds)][0]);
}

