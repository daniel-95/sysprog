#include <stdlib.h>

#include "heap.h"
#include "int_array.h"

struct heap *new_heap(int size) {
	struct heap *h = malloc(sizeof(struct heap));
	h->len = 0;
	h->cap = size;
	h->data = malloc(size * sizeof(struct heap_entry));
}

void heapify(struct heap *h, int i) {
	int l = left(i);
	int r = right(i);
	int smallest;

	struct heap_entry he_lt = h->data[l];
	struct heap_entry he_rt = h->data[r];
	struct heap_entry he_i = h->data[i];

	if(l < h->len && he_lt.arr->data[he_lt.idx] < he_i.arr->data[he_i.idx])
		smallest = l;
	else
		smallest = i;

	struct heap_entry he_smallest = h->data[smallest];

	if(r < h->len && he_rt.arr->data[he_rt.idx] < he_smallest.arr->data[he_smallest.idx]) {
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

void heap_add(struct heap *h, struct int_array *ia) {
	if(h->len >= h->cap)
		return;

	h->data[h->len++] = (struct heap_entry){
		.arr = ia,
		.idx = 0
	};
}
