all:
	clang -Icoro -Ivar_array main.c heap.c var_array/var_array.c coro/coro.c -o run -Wall -Werror -lrt
debug:
	clang -Icoro -Ivar_array main.c heap.c var_array/var_array.c coro/coro.c -o run -Wall -Werror -lrt -g
sanitize:
	clang -Icoro -Ivar_array main.c heap.c var_array/var_array.c coro/coro.c -fsanitize=address -Wall -Werror -lrt
clean:
	rm run
