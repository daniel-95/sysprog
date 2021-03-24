#include "coro.h"

void __coro_sched() {
	for(int i = 1; i < coro_len; i++) {
		if(coros[(current_i + i) % coro_len]->state == CORO_SUSPENDED || coros[(current_i + i) % coro_len]->state == CORO_AWAITING) {
			struct coroutine *coro_old = current;
			current = coros[(current_i + i) % coro_len];
			current_i = (current_i + i) % coro_len;

			struct timeval tcoro_old, tdiff;
			gettimeofday(&tcoro_old, NULL);
			timersub(&tcoro_old, &current_timeval, &tdiff);
			timeradd(&coro_old->elapsed, &tdiff, &coro_old->elapsed);
			gettimeofday(&current_timeval, NULL);

			if(coro_old->state == CORO_SUSPENDED) {
				if(current->state == CORO_SUSPENDED) {
					current->state = CORO_RUNNING;
					swapcontext(&coro_old->uctx, &current->uctx);
				} else {
					swapcontext(&coro_old->uctx, &current->uctx_wait);
				}
			} else {
				if(current->state == CORO_SUSPENDED) {
					current->state = CORO_RUNNING;
					setcontext(&current->uctx);
				} else {
					setcontext(&current->uctx_wait);
				}
			}

			return;
		}
	}
}

void coro_yield() {
	if(current->state != CORO_DEAD)
		current->state = CORO_SUSPENDED;

	__coro_sched();
}

void coro_put(struct coroutine *coro_new) {
	coro_new->state = CORO_SUSPENDED;
	coros[coro_len++] = coro_new;
}

void coro_run() {
	ucontext_t uctx_current;
	getcontext(&uctx_return);

	if(is_done)
		return;

	is_done = true;

	gettimeofday(&current_timeval, NULL);
	for(int i = 0; i < coro_len; i++) {
		if(coros[i]->state == CORO_SUSPENDED) {
			coros[i]->state = CORO_RUNNING;
			current = coros[i];
			current_i = i;
			swapcontext(&uctx_current, &coros[i]->uctx);
		}
	}
}

void coro_finished() {
	current->state = CORO_DEAD;
	// sched
	coro_yield();
	// only the last coroutine will get there
	setcontext(&uctx_return);
}

void coro_prepare() {
	getcontext(&uctx_finished);
	uctx_finished.uc_stack.ss_sp = stack_finished;
	uctx_finished.uc_stack.ss_size = 16 * 1024;
	uctx_finished.uc_link = NULL;
	makecontext(&uctx_finished, coro_finished, 0);

	is_done = false;
	timerclear(&current_timeval);
}

struct coroutine *coro_init(void (*f)(void*), void *args) {
	struct coroutine *coro_new = malloc(sizeof(struct coroutine));
	memset(coro_new, 0, sizeof(struct coroutine));
	coro_new->state = CORO_SUSPENDED;
	timerclear(&coro_new->elapsed);

	getcontext(&coro_new->uctx);

	coro_new->stack = malloc(32 * 1024);
	stack_t ss;
	ss.ss_sp = coro_new->stack;
	ss.ss_size = 32 * 1024;
	ss.ss_flags = 0;
	sigaltstack(&ss, NULL);

	coro_new->uctx.uc_stack.ss_sp = coro_new->stack;
	coro_new->uctx.uc_stack.ss_size = 32 * 1024;
	coro_new->uctx.uc_link = &uctx_finished;
	makecontext(&coro_new->uctx, (void (*)(void))f, 1, args);

	return coro_new;
}

void coro_free(struct coroutine *c) {
	free(c->stack);
	free(c);
}

// wait group
struct wait_group *wg_new() {
	struct wait_group *wg = malloc(sizeof(struct wait_group));
	wg->cnt = 0;
	wg->fresh = true;
	return wg;
}

void wg_add(struct wait_group *wg) {
	wg->fresh = false;
	wg->cnt++;
}

void wg_done(struct wait_group *wg) {
	if(wg->cnt > 0)
		wg->cnt--;
}

void wg_wait(struct wait_group *wg) {
	getcontext(&current->uctx_wait);

	if(wg->fresh == false && wg->cnt == 0) {
		current->state = CORO_RUNNING;
		return;
	}

	current->state = CORO_AWAITING;
	__coro_sched();
}

