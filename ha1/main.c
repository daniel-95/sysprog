#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <string.h>

#include "int_array.h"
#include "heap.h"
#include "coro.h"

#define usage() printf("usage: ./run file1 [file2 ... fileN]\n")
#define errdump(msg) printf("[ERROR] %s\n", msg);

struct sort_args {
	int *nums;
	int low;
	int high;
	struct wait_group *wg;
};

int partition(int *nums, int low, int high) {
	int p = nums[high];
	int i = low - 1;
	int tmp;

	for(int j = low; j < high; j++) {
		if(nums[j] > p)
			continue;

		coro_yield();

		i++;
		tmp = nums[i];
		nums[i] = nums[j];
		nums[j] = tmp;
	}

	coro_yield();

	tmp = nums[i+1];
	nums[i+1] = nums[high];
	nums[high] = tmp;

	return i+1;
}

void sort_numbers_quickly(void *vargs) {
	struct sort_args *args = (struct sort_args*) vargs;
	wg_add(args->wg);

	if(args->low < args->high) {
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
		sort_numbers_quickly(&r);
	}

	wg_done(args->wg);
}

struct int_array *read_file_alloc(char *file_name) {
	FILE *fh = fopen(file_name, "r");

	if(fh == NULL) {
		errdump("couldn't open the file");
		return 0;
	}

	int in;

	struct int_array *buf = malloc(sizeof(struct int_array*));
	buf->len = 0;
	buf->cap = 8;
	buf->data = malloc(buf->cap * sizeof(int));

	while(fscanf(fh, "%d", &in) == 1) {
		if(buf->len == buf->cap) {
			buf->cap *= 2;
			buf->data = realloc(buf->data, buf->cap * sizeof(int));
		}

		buf->data[buf->len++] = in;
	}

	fclose(fh);
	return buf;
}

struct merge_args {
	struct wait_group *wg_sort_files;
	int lists_num;
	struct int_array **lists;
};

void grand_merge(void *vargs) {
	struct merge_args *args = (struct merge_args *) vargs;

	wg_wait(args->wg_sort_files);

	struct heap *h = new_heap(args->lists_num);

	for(int i = 0; i < args->lists_num; i++)
		heap_add(h, args->lists[i]);

	build_min_heap(h);

	int print_count = 0;
	while(h->len > 0/* just in case */) {
		struct heap_entry he = h->data[0];

		if(print_count == 0)
			print_count++;
		else
			printf(" ");

		printf("%d", he.arr->data[he.idx++]);
		h->data[0] = he;

		if(he.idx == he.arr->len) {
			// remove from the heap
			h->data[0] = h->data[h->len-1];
			h->len--;
		}

		if(h->len == 0)
			break;

		heapify(h, 0);
	}
}

int main(int argc, char *argv[]) {
	if(argc < 2) {
		usage();
		return 0;
	}

	coro_prepare();

	struct int_array **lists = malloc((argc - 1) * sizeof(struct int_array*));

	for(int i = 0; i < argc - 1; i++)
		lists[i] = read_file_alloc(argv[i + 1]);

	struct sort_args *d = malloc(sizeof(struct sort_args) * (argc - 1));
	struct coroutine **c = malloc(sizeof(struct coroutine*) * (argc - 1));
	struct wait_group *wg_sort_files = wg_new();
	for(int i = 0; i < argc - 1; i++) {
		char name[64];
		strncpy(name, "coro0", 5);
		name[5] = '\0';
		name[4] = '0' + i;
		d[i].nums = lists[i]->data;
		d[i].low = 0;
		d[i].high = lists[i]->len - 1;
		d[i].wg = wg_sort_files;

		c[i] = coro_init(sort_numbers_quickly, &d[i], name);
		coro_put(c[i]);
	}

	struct merge_args *ma = malloc(sizeof(struct merge_args));
	ma->wg_sort_files = wg_sort_files;
	ma->lists = lists;
	ma->lists_num = argc-1;

	struct coroutine *merge = coro_init(grand_merge, ma, "merger");
	coro_put(merge);
	coro_run();

	return 0;
}

