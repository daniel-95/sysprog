all:
	clang -Icoro main.c heap.c var_array.c coro/coro.c -o run -Wall -Werror
debug:
	clang -Icoro main.c heap.c var_array.c coro/coro.c -o run -Wall -Werror -g
sanitize:
	clang -Icoro main.c heap.c var_array.c coro/coro.c -o run -fsanitize=address -Wall -Werror
clean:
	rm run
