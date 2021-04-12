#ifndef __CORO_H
#define __CORO_H

#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/time.h>

#include "var_array.h"

#define CORO_STACK_SIZE 32*1024

// coroutine

typedef enum {
	CORO_RUNNING,
	CORO_SUSPENDED,
	CORO_AWAITING,
	CORO_DEAD
} coro_state;

struct coroutine {
	struct timeval elapsed;
	char *stack;
	ucontext_t uctx;
	ucontext_t uctx_wait;
	coro_state state;
};

// global coroutines state
struct coroutine *current;
struct var_array *coros;
int current_i;
ucontext_t uctx_finished, uctx_return;
char stack_finished[CORO_STACK_SIZE];
bool is_done;
struct timeval current_timeval;

void __coro_sched();
void coro_yield();
void coro_put(struct coroutine *coro_new);
void coro_run();
void coro_finished();
void coro_prepare();
struct coroutine * coro_init(void (*f)(void*), void *args);
void coro_free(struct coroutine *c);

// waitgroup

struct wait_group {
	// cnt indicates the number of wg_wait calls
	int cnt;
	// fresh indicates whether this wait_group was used or not
	bool fresh;
};

struct wait_group *wg_new();
void wg_add(struct wait_group *wg);
void wg_done(struct wait_group *wg);
void wg_wait(struct wait_group *wg);

#endif
