#include <stdlib.h>

#include "heap.h"
#include "var_array.h"

struct heap *new_heap(int size) {
	struct heap *h = malloc(sizeof(struct heap));
	h->len = 0;
	h->cap = size;
	h->data = malloc(size * sizeof(struct heap_entry));

	return h;
}

void heap_free(struct heap *h) {
	free(h->data);
	free(h);
}

void heapify(struct heap *h, int i) {
	int l = left(i);
	int r = right(i);
	int smallest;

	struct heap_entry he_lt = (l < h->len) ? h->data[l] : (struct heap_entry){};
	struct heap_entry he_rt = (r < h->len) ? h->data[r] : (struct heap_entry){};
	struct heap_entry he_i = h->data[i];

	if(l < h->len && var_array_get(he_lt.arr, he_lt.idx, int) < var_array_get(he_i.arr, he_i.idx, int))
		smallest = l;
	else
		smallest = i;

	struct heap_entry he_smallest = h->data[smallest];

	if(r < h->len && var_array_get(he_rt.arr, he_rt.idx, int) < var_array_get(he_smallest.arr, he_smallest.idx, int)) {
		smallest = r;
		he_smallest = he_rt;
	}

	if(smallest != i) {
		struct heap_entry tmp;

		tmp = h->data[i];
		h->data[i] = h->data[smallest];
		h->data[smallest] = tmp;

		heapify(h, smallest);
	}
}

void build_min_heap(struct heap *h) {
	for(int p = h->cap / 2; p >= 0; p--)
		heapify(h, p);
}

void heap_add(struct heap *h, struct var_array *ia) {
	if(h->len >= h->cap)
		return;

	h->data[h->len++] = (struct heap_entry){
		.arr = ia,
		.idx = 0
	};
}

