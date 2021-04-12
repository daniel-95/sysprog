#include "coro.h"
#include "macro.h"

// __coro_sched performs round-robin coroutines scheduling
void __coro_sched() {
	int coro_len = var_array_len(coros);
	for(int i = 1; i < coro_len; i++) {
		struct coroutine *coro_it = var_array_get(coros, ((current_i + i) % coro_len), struct coroutine*);
		// suspended and awaiting coroutines are candidates to be awaken
		if(coro_it->state == CORO_SUSPENDED || coro_it->state == CORO_AWAITING) {
			struct coroutine *coro_old = current;
			current = coro_it;
			current_i = (current_i + i) % coro_len;

			// computing the time this coroutine took from previous yield to current one
			struct timeval tcoro_old, tdiff;
			gettimeofday(&tcoro_old, NULL);
			timersub(&tcoro_old, &current_timeval, &tdiff);
			timeradd(&coro_old->elapsed, &tdiff, &coro_old->elapsed);
			gettimeofday(&current_timeval, NULL);

			// there are 4 possible cases:
			// SUSPENDED - SUSPENDED - previous coroutine was suspended by yield so as the new one
			// SUSPENDED - AWAITING - previous coroutine was suspended but the new one is waiting on wait_group
			// AWAITING - SUSPENDED - previous coroutine was awaiting on wait_group, the new one was suspended by yield
			// AWAITING - AWAITING - both previous and current coroutines are awaiting wait_group
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
	// dead coros must not be awaken again
	if(current->state != CORO_DEAD)
		current->state = CORO_SUSPENDED;

	__coro_sched();
}

void coro_put(struct coroutine *coro_new) {
	// new coros are suspended by default and awaken later by __coro_sched
	coro_new->state = CORO_SUSPENDED;
	var_array_put(coros, coro_new, struct coroutine*);
}

void coro_run() {
	// this context indicates the ending point to which execution will get after all coroutines are dead
	ucontext_t uctx_current;
	getcontext(&uctx_return);

	// this flag helps to avoid infinite looping over the dead coroutines
	if(is_done)
		return;

	is_done = true;

	gettimeofday(&current_timeval, NULL);
	int coro_len = var_array_len(coros);
	for(int i = 0; i < coro_len; i++) {
		struct coroutine *coro_it = var_array_get(coros, i, struct coroutine*);
		if(coro_it->state == CORO_SUSPENDED) {
			coro_it->state = CORO_RUNNING;
			current = coro_it;
			current_i = i;
			swapcontext(&uctx_current, &coro_it->uctx);
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
	uctx_finished.uc_stack.ss_size = CORO_STACK_SIZE;
	uctx_finished.uc_link = NULL;
	makecontext(&uctx_finished, coro_finished, 0);

	is_done = false;
	timerclear(&current_timeval);

	coros = var_array_init(32, sizeof(struct coroutine*));
}

struct coroutine *coro_init(void (*f)(void*), void *args) {
	struct coroutine *coro_new = malloc(sizeof(struct coroutine));
	memcheck(coro_new, "couldn't allocate memory for coro_new");

	memset(coro_new, 0, sizeof(struct coroutine));
	coro_new->state = CORO_SUSPENDED;
	timerclear(&coro_new->elapsed);

	getcontext(&coro_new->uctx);

	coro_new->stack = malloc(CORO_STACK_SIZE);
	memcheck(coro_new->stack, "couldn't allocate memory for coro_new->stack");

	stack_t ss;
	ss.ss_sp = coro_new->stack;
	ss.ss_size = CORO_STACK_SIZE;
	ss.ss_flags = 0;
	sigaltstack(&ss, NULL);

	coro_new->uctx.uc_stack.ss_sp = coro_new->stack;
	coro_new->uctx.uc_stack.ss_size = CORO_STACK_SIZE;
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
	memcheck(wg, "couldn't allocate memory for wg");
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

