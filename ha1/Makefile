all:
	clang -Icoro main.c heap.c var_array.c coro/coro.c -o run -Wall -Werror -lrt
debug:
	clang -Icoro main.c heap.c var_array.c coro/coro.c -o run -Wall -Werror -lrt -g
sanitize:
	clang -Icoro main.c heap.c var_array.c coro/coro.c -o run -fsanitize=address -Wall -Werror -lrt
clean:
	rm run
