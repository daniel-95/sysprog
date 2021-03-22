#include "coro.h"

void __coro_sched() {
	for(int i = 1; i < coro_len; i++) {
		if(coros[(current_i + i) % coro_len]->state == SUSPENDED || coros[(current_i + i) % coro_len]->state == AWAITING) {
			struct coroutine *coro_old = current;
			current = coros[(current_i + i) % coro_len];
			current_i = (current_i + i) % coro_len;

			if(coro_old->state == SUSPENDED) {
				if(current->state == SUSPENDED) {
					current->state = RUNNING;
					swapcontext(&coro_old->uctx, &current->uctx);
				} else {
					swapcontext(&coro_old->uctx, &current->uctx_wait);
				}
			} else {
				if(current->state == SUSPENDED) {
					current->state = RUNNING;
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
	if(current->state != DEAD)
		current->state = SUSPENDED;

	__coro_sched();
}

void coro_put(struct coroutine *coro_new) {
	coro_new->state = SUSPENDED;
	coros[coro_len++] = coro_new;
}

void coro_run() {
	ucontext_t uctx_current;
	getcontext(&uctx_return);

	if(is_done)
		return;

	is_done = true;

	for(int i = 0; i < coro_len; i++) {
		if(coros[i]->state == SUSPENDED) {
			coros[i]->state = RUNNING;
			current = coros[i];
			current_i = i;
			swapcontext(&uctx_current, &coros[i]->uctx);
		}
	}
}

void coro_finished() {
	current->state = DEAD;
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
}

struct coroutine *coro_init(void (*f)(void*), void *args, char *name) {
	struct coroutine *coro_new = malloc(sizeof(struct coroutine));
	memset(coro_new, sizeof(struct coroutine), 0);
	coro_new->state = SUSPENDED;
	strncpy(coro_new->name, name, (strlen(name) < 64) ? strlen(name) : 64);

	getcontext(&coro_new->uctx);
	coro_new->uctx.uc_stack.ss_sp = coro_new->stack;
	coro_new->uctx.uc_stack.ss_size = 32 * 1024;
	coro_new->uctx.uc_link = &uctx_finished;
	makecontext(&coro_new->uctx, f, 1, args);

	return coro_new;
}

// wait group
struct wait_group *wg_new() {
	struct wait_group *wg = malloc(sizeof(struct wait_group));
	wg->cnt = 0;
	return wg;
}

void wg_add(struct wait_group *wg) {
	wg->cnt++;
}

void wg_done(struct wait_group *wg) {
	if(wg->cnt > 0)
		wg->cnt--;
}

void wg_wait(struct wait_group *wg) {
	getcontext(&current->uctx_wait);

	if(wg->cnt == 0) {
		current->state = RUNNING;
		return;
	}

	current->state = AWAITING;
	__coro_sched();
}

