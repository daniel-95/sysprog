#ifndef __CORO_H
#define __CORO_H

#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <string.h>
#include <stdbool.h>

// coroutine

typedef enum {
	RUNNING,
	SUSPENDED,
	AWAITING,
	DEAD
} coro_state;

struct coroutine {
	char name[64];
	char stack[128 * 1024];
	ucontext_t uctx;
	ucontext_t uctx_wait;
	coro_state state;
};

static struct coroutine *current;
static struct coroutine *coros[64];
static int current_i = 0;
static int coro_len = 0;
ucontext_t uctx_finished, uctx_return;
char stack_finished[32 * 1024];
static bool is_done = false;

void __coro_sched();
void coro_yield();
void coro_put(struct coroutine *coro_new);
void coro_run();
void coro_finished();
void coro_prepare();
struct coroutine * coro_init(void (*f)(void*), void *args, char *name);

// waitgroup

struct wait_group {
	int cnt;
};

struct wait_group *wg_new();
void wg_add(struct wait_group *wg);
void wg_done(struct wait_group *wg);
void wg_wait(struct wait_group *wg);

#endif
