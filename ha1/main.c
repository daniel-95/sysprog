#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <string.h>

#include "var_array.h"
#include "heap.h"
#include "coro.h"

#define usage() printf("usage: ./run file1 [file2 ... fileN]\n")
#define errdump(msg) printf("[ERROR] %s\n", msg);

struct sort_args {
	struct var_array *nums;
	int low;
	int high;
	struct wait_group *wg;
};

int partition(struct var_array *nums, int low, int high) {
	int p = var_array_get(nums, high, int);
	int i = low - 1;
	int tmp;

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

void sort_numbers_quickly(void *vargs) {
	struct sort_args *args = (struct sort_args*) vargs;
	wg_add(args->wg);

	if(args->low < args->high) {
		coro_yield();
		int pivot = partition(args->nums, args->low, args->high);
		struct sort_args l = {
			.nums = args->nums,
			.low = args->low,
			.high = pivot-1,
			.wg = args->wg
		};

		coro_yield();

		struct sort_args r = {
			.nums = args->nums,
			.low = pivot+1,
			.high = args->high,
			.wg = args->wg
		};

		coro_yield();
		sort_numbers_quickly(&l);
		coro_yield();
		sort_numbers_quickly(&r);
	}

	wg_done(args->wg);
}

struct var_array *read_file_alloc(char *file_name) {
	FILE *fh = fopen(file_name, "r");

	if(fh == NULL) {
		errdump("couldn't open the file");
		return 0;
	}

	int in;

	struct var_array *buf = var_array_init(8, sizeof(int));

	while(fscanf(fh, "%d", &in) == 1)
		var_array_put(buf, in, int);

	fclose(fh);
	return buf;
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

	for(int i = 0; i < argc - 1; i++)
		lists[i] = read_file_alloc(argv[i + 1]);

	struct sort_args *d = malloc(sizeof(struct sort_args) * (argc - 1));
	struct coroutine **c = malloc(sizeof(struct coroutine*) * (argc - 1));
	struct wait_group *wg_sort_files = wg_new();
	for(int i = 0; i < argc - 1; i++) {
		d[i].nums = lists[i];
		d[i].low = 0;
		d[i].high = lists[i]->len - 1;
		d[i].wg = wg_sort_files;

		c[i] = coro_init(sort_numbers_quickly, &d[i]);
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
	for(int i = 0; i < argc - 1; i++)
		var_array_free(lists[i]);

	free(lists);
	free(d);

	for(int i = 0; i < argc - 1; i++)
		coro_free(c[i]);

	free(c);
	free(wg_sort_files);
	free(ma);

	coro_free(merge);

	return 0;
}

