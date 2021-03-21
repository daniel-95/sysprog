#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

#include "int_array.h"
#include "heap.h"

#define usage() printf("usage: ./run file1 [file2 ... fileN]\n")
#define errdump(msg) printf("[ERROR] %s\n", msg);

int partition(int *nums, int low, int high) {
	int p = nums[high];
	int i = low - 1;
	int tmp;

	for(int j = low; j < high; j++) {
		if(nums[j] > p)
			continue;

		i++;
		tmp = nums[i];
		nums[i] = nums[j];
		nums[j] = tmp;
	}

	tmp = nums[i+1];
	nums[i+1] = nums[high];
	nums[high] = tmp;

	return i+1;
}

void sort_numbers_quickly(int *nums, int low, int high) {
	if(low < high) {
		int pivot = partition(nums, low, high);
		sort_numbers_quickly(nums, low, pivot-1);
		sort_numbers_quickly(nums, pivot+1, high);
	}
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

void print_int_array(struct int_array* ia) {
	for(int i = 0; i < ia->len; i++)
		printf("%d ", ia->data[i]);

	printf("\n");
}

void grand_merge(struct heap *h) {
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

	struct int_array **lists = malloc((argc - 1) * sizeof(struct int_array*));

	for(int i = 0; i < argc - 1; i++)
		lists[i] = read_file_alloc(argv[i + 1]);

	for(int i = 0; i < argc - 1; i++)
		sort_numbers_quickly(lists[i]->data, 0, lists[i]->len - 1);

	for(int i = 0; i < argc - 1; i++)
		print_int_array(lists[i]);

	printf("---------------------\n");

	struct heap *h = new_heap(argc - 1);

	for(int i = 0; i < argc - 1; i++)
		heap_add(h, lists[i]);

	build_min_heap(h);

	for(int i = 0; i < h->len; i++)
		printf("heap entry: %d %d\n", h->data[i].arr->data[h->data[i].idx], h->data[i].idx);

	printf("---------------------\n");

	grand_merge(h);

	return 0;
}

