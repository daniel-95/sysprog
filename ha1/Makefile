all:
	gcc -Icoro main.c heap.c coro/coro.c -o run
debug:
	gcc -Icoro main.c heap.c coro/coro.c -o run -g
prof:
	gcc -Icoro main.c heap.c coro/coro.c -o run -pg
clean:
	rm run gmon.out
