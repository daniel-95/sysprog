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

const char *__cmd_builtin[] = {
	"about",
	"cd"
};

__cmd_func __cmd_builtin_func[] = {
	__cmd_about,
	__cmd_cd
};

void __cmd_about() {
	char *msg = 	"            ______             \n"
			"       .d$$$******$$$$c.       \n"
			"    .d$P\"            \"$$c      \n"
			"   $$$$$.           .$$$*$.    \n"
			" .$$ 4$L*$$.     .$$Pd$  '$b   \n"
			" $F   *$. \"$$e.e$$\" 4$F   ^$b  \n"
			"d$     $$   z$$$e   $$     '$. \n"
			"$P     `$L$$P` `\"$$d$\"      $$ \n"
			"$$     e$$F       4$$b.     $$ \n"
			"$b  .$$\" $$      .$$ \"4$b.  $$ \n"
			"$$e$P\"    $b     d$`    \"$$c$F \n"
			"'$P$$$$$$$$$$$$$$$$$$$$$$$$$$  \n"
			" \"$c.      4$.  $$       .$$   \n"
			"  ^$$.      $$ d$\"      d$P    \n"
			"    \"$$c.   `$b$F    .d$P\"     \n"
			"      `4$$$c.$$$..e$$P\"        \n"
			"	  `^^^^^^^`             \n";

	printf("MOSH - Minimalistic Open SHell\n\n%s\n", msg);
}

void __cmd_cd(char *dst) {
	printf("cd mocked for now\n");
}

__cmd_func __cmd_is_builtin(char *cmd_name) {
	for(int i = 0; i < __cmd_builtin_len(); i++) {
		if(strcmp(cmd_name, __cmd_builtin[i]) == 0)
			return __cmd_builtin_func[i];
	}

	return NULL;
}

void cmd_parse(const char *input, size_t input_size, struct var_array **cmds) {
	// lex_begin points to the beginning of a lexeme
	// it iterates over the input string looking for lexeme ending
	size_t lex_begin = 0;
	size_t it = 0;
	bool is_lexeme = false;

	// is_open_quote stores the type of the quoting sign
	char is_open_quote = '\0';

	*cmds = var_array_init(8, sizeof(struct cmd));
	struct var_array *token_list = var_array_init(8, sizeof(struct cmd_token));

	while(it < input_size && input[it] != '\0') {
		if(is_lexeme) {
			struct cmd_token new_token;

			// skipping to the end of current lexeme
			if(is_open_quote != '\0') {
				// lex_begin points to the open quote sign, so move it
				lex_begin++;
				while(it < input_size && input[it] != is_open_quote) it++;

				// error - couldn't find the closing quote, doing nothing
				if(it == input_size)
					return;
			} else
				while(it < input_size && !isspace(input[it])) it++;

			new_token.value = malloc(sizeof(char) * (it - lex_begin + 1));
			strncpy(new_token.value, &input[lex_begin], it - lex_begin);

			if(is_open_quote == '\0' && strcmp(new_token.value, "|") == 0)
				new_token.clid = CMD_ID_PIPE;
			else if(is_open_quote == '\0' && strcmp(new_token.value, ">") == 0)
				new_token.clid = CMD_ID_WRITE;
			else if(is_open_quote == '\0' && strcmp(new_token.value, ">>") == 0)
				new_token.clid = CMD_ID_APPEND;
			else
				new_token.clid = CMD_ID_WORD;

			var_array_put(token_list, new_token, struct cmd_token);

			is_open_quote = '\0';
			is_lexeme = false;
		} else {
			// we've just met lexeme
			if(!isspace(input[it])) {
				is_lexeme = true;
				lex_begin = it;

				if(input[it] == '\'' || input[it] == '"')
					is_open_quote = input[it];
			}

			// or we're iterating over sequence of whitespaces doing nothing
		}

		it++;
	}

	for(int i = 0; i < var_array_len(token_list); i++) {
		struct cmd_token tk = var_array_get(token_list, i, struct cmd_token);
		printf("[token] id: %d value: %s\n", tk.clid, tk.value);
	}

	int t_it = 0;
	while(t_it < var_array_len(token_list)) {
		struct cmd new_cmd = {
			.name = NULL,
			.args = var_array_init(8, sizeof(char*)),
			// let it be default value
			.write_mode = CMD_ID_WORD,
			.output_file = NULL
		};

		while(t_it < var_array_len(token_list)) {
			struct cmd_token tk = var_array_get(token_list, t_it, struct cmd_token);

			if(tk.clid == CMD_ID_PIPE) {
				t_it++;
				break;
			} else if(tk.clid == CMD_ID_WRITE || tk.clid == CMD_ID_APPEND) {
				t_it++;
				struct cmd_token tk_file = var_array_get(token_list, t_it, struct cmd_token);
				new_cmd.write_mode = tk.clid;
				new_cmd.output_file = tk_file.value;

				// skipping tokens until the next pipe
				while(t_it < var_array_len(token_list)) {
					struct cmd_token tmp = var_array_get(token_list, t_it, struct cmd_token);
					if(tmp.clid == CMD_ID_PIPE)
						break;

					t_it++;
				}

				break;
			}

			if(new_cmd.name == NULL)
				new_cmd.name = tk.value;

			var_array_put(new_cmd.args, tk.value, char*);

			t_it++;
		}

		var_array_put((*cmds), new_cmd, struct cmd);
	}

	var_array_free(token_list);
}

void cmd_execute(struct var_array *cmds) {
	for(int i = 0; i < var_array_len(cmds); i++) {
		struct cmd c = var_array_get(cmds, i, struct cmd);
		printf("[cmd] name: %s write_mode: %d\n", c.name, c.write_mode);
	}
//	int fd_pipes[var_array_len(cmds) + 1][2];
//	pid_t child_pids[var_array_len(cmds)];
//	int status = 0;
//
//	for(int i = 0; i < var_array_len(cmds) + 1; i++)
//		pipe2(fd_pipes[i], O_CLOEXEC);
//
//	close(fd_pipes[0][1]);
//	for(int i = 0; i < var_array_len(cmds); i++) {
//		struct cmd c = var_array_get(cmds, i, struct cmd);
//
//		if((child_pids[i] = fork()) == 0) {
//			dup2(fd_pipes[i][0], 0);
//			dup2(fd_pipes[i+1][1], 1);
//			execvp(c.name, var_array_raw(c.args, char*));
//		}
//	}
//
//	// waiting for all children to complete execution
//	for(int i = 0; i < var_array_len(cmds); i++) {
//		waitpid(child_pids[i], &status, 0);
//		close(fd_pipes[i][0]);
//		close(fd_pipes[i+1][1]);
//	}
//
//	char buf[1024];
//	size_t buf_size;
//	while((buf_size = read(fd_pipes[var_array_len(cmds)][0], buf, 1024)) > 0)
//		write(STDOUT_FILENO, buf, buf_size);
//
//	close(fd_pipes[var_array_len(cmds)][0]);
}

