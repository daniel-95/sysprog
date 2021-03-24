#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <string.h>
#include <aio.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "var_array.h"
#include "heap.h"
#include "coro.h"

#define usage() printf("usage: ./run file1 [file2 ... fileN]\n")
#define errdump(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)

struct sort_args {
	struct var_array *nums;
	struct wait_group *wg_sort;
	struct wait_group *wg_read;
};

int partition(struct var_array *nums, int low, int high) {
	int p = var_array_get(nums, high, int);
	int i = low - 1;

	coro_yield();

	for(int j = low; j < high; j++) {
		if(var_array_get(nums, j, int) > p) {
			coro_yield();
			continue;
		}

		coro_yield();

		i++;
		coro_yield();
		var_array_swap(nums, i, j, int);
	}

	coro_yield();
	var_array_swap(nums, i+1, high, int);

	return i+1;
}

void sort_numbers_quickly(struct var_array *nums, int low, int high, struct wait_group *wg_sort) {
	wg_add(wg_sort);

	if(low < high) {
		coro_yield();
		int pivot = partition(nums, low, high);
		coro_yield();
		sort_numbers_quickly(nums, low, pivot-1, wg_sort);
		coro_yield();
		sort_numbers_quickly(nums, pivot+1, high, wg_sort);
	}

	wg_done(wg_sort);
}

void sort(void *vargs) {
	struct sort_args *args = (struct sort_args*) vargs;
	wg_wait(args->wg_read);

	sort_numbers_quickly(args->nums, 0, var_array_len(args->nums) - 1, args->wg_sort);
}

struct read_args {
	char *file_name;
	struct wait_group *wg_read;
	struct var_array *list;
};

struct aio_ctx {
	struct aiocb aiocb;
	struct read_args *ra;
};

void sigusr1_read_handler(int sig, siginfo_t *si, void *ptr) {
	struct aio_ctx *ctx = (struct aio_ctx*)si->si_value.sival_ptr;
	int in, offset;
	char *data = (char *)ctx->aiocb.aio_buf;

	while(sscanf(data, "%d%n", &in, &offset) == 1) {
		var_array_put(ctx->ra->list, in, int);
		data += offset;
	}

	wg_done(ctx->ra->wg_read);
	free(si->si_value.sival_ptr);
}

void read_file_async(void *vargs) {
	struct read_args *args = (struct read_args *) vargs;
	wg_add(args->wg_read);

	FILE *fh = fopen(args->file_name, "r");

	coro_yield();
	if(fh == NULL)
		errdump("couldn't open the file");

	struct aio_ctx *ctx = malloc(sizeof(struct aio_ctx));
	struct sigaction sa;

	coro_yield();
	memset(&ctx->aiocb, 0, sizeof(struct aiocb));
	ctx->ra = args;

	coro_yield();
	sa.sa_flags = SA_RESTART | SA_SIGINFO;
	sa.sa_sigaction = sigusr1_read_handler;

	if(sigaction(SIGUSR1, &sa, NULL) == -1)
		errdump("sigaction\n");

	coro_yield();
	ctx->aiocb.aio_fildes = open(args->file_name, O_RDONLY);

	if(ctx->aiocb.aio_fildes == -1)
		errdump("couldn't read input file");

	coro_yield();
	ctx->aiocb.aio_buf = calloc(1, 32 * 1024);
	ctx->aiocb.aio_nbytes = 32 * 1024;
	ctx->aiocb.aio_reqprio = 0;
	ctx->aiocb.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
	ctx->aiocb.aio_sigevent.sigev_signo = SIGUSR1;
	ctx->aiocb.aio_sigevent.sigev_value.sival_ptr = ctx;

	coro_yield();
	aio_read(&ctx->aiocb);
}

struct merge_args {
	struct wait_group *wg_sort_files;
	int lists_num;
	struct var_array **lists;
};

void grand_merge(void *vargs) {
	struct merge_args *args = (struct merge_args *) vargs;

	wg_wait(args->wg_sort_files);

	struct heap *h = new_heap(args->lists_num);

	for(int i = 0; i < args->lists_num; i++)
		heap_add(h, args->lists[i]);

	build_min_heap(h);

	int print_count = 0;
	coro_yield();

	while(h->len > 0/* just in case */) {
		struct heap_entry he = h->data[0];

		if(print_count == 0)
			print_count++;
		else
			printf(" ");

		coro_yield();

		printf("%d", var_array_get(he.arr, he.idx++, int));
		h->data[0] = he;

		if(he.idx == he.arr->len) {
			// remove from the heap
			h->data[0] = h->data[h->len-1];
			h->len--;
		}

		coro_yield();

		if(h->len == 0)
			break;

		heapify(h, 0);
	}

	heap_free(h);
}

int main(int argc, char *argv[]) {
	if(argc < 2) {
		usage();
		return 0;
	}

	coro_prepare();

	struct var_array **lists = malloc((argc - 1) * sizeof(struct var_array*));
	struct read_args *ra = malloc((argc - 1) * sizeof(struct read_args));
	struct wait_group **wg_reads = malloc((argc - 1) * sizeof(struct wait_group*));
	struct coroutine **c_read = malloc(sizeof(struct coroutine*) * (argc - 1));

	for(int i = 0; i < argc - 1; i++) {
		wg_reads[i] = wg_new();
		lists[i] = var_array_init(8, sizeof(int));
		ra[i].file_name = argv[i + 1];
		ra[i].wg_read = wg_reads[i];
		ra[i].list = lists[i];

		c_read[i] = coro_init(read_file_async, &ra[i]);
		coro_put(c_read[i]);
	}

	struct sort_args *d = malloc(sizeof(struct sort_args) * (argc - 1));
	struct coroutine **c = malloc(sizeof(struct coroutine*) * (argc - 1));
	struct wait_group *wg_sort_files = wg_new();
	for(int i = 0; i < argc - 1; i++) {
		d[i].nums = lists[i];
		d[i].wg_sort = wg_sort_files;
		d[i].wg_read = wg_reads[i];

		c[i] = coro_init(sort, &d[i]);
		coro_put(c[i]);
	}

	struct merge_args *ma = malloc(sizeof(struct merge_args));
	ma->wg_sort_files = wg_sort_files;
	ma->lists = lists;
	ma->lists_num = argc-1;

	struct coroutine *merge = coro_init(grand_merge, ma);
	coro_put(merge);
	coro_run();

	// releasing memory
	for(int i = 0; i < argc - 1; i++) {
		var_array_free(lists[i]);
		free(wg_reads[i]);
	}

	free(lists);
	free(ra);
	free(wg_reads);
	free(d);

	for(int i = 0; i < argc - 1; i++) {
		coro_free(c_read[i]);
		coro_free(c[i]);
	}

	free(c);
	free(c_read);
	free(wg_sort_files);
	free(ma);

	coro_free(merge);

	return 0;
}

