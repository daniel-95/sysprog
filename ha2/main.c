#include <stdio.h>
#include <stdlib.h>

#include "cmd.h"

int main(int argc, char *argv[]) {
	char *input = NULL;
	size_t input_size = 0;
	struct var_array *cmds;

	printf(">> ");
	while(getline(&input, &input_size, stdin) != -1) {
		cmd_parse(input, input_size, &cmds);
		cmd_execute(cmds);
		printf(">> ");
	}

	free(input);

	return 0;
}

