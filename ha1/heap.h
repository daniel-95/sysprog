#ifndef __HEAP_H
#define __HEAP_H

#define parent(i) ((i - 1) / 2)
#define left(i)   (2 * i + 1)
#define right(i)  (2 * i + 2)

#include "int_array.h"

struct heap_entry {
	struct int_array *arr;
	int idx;
};

struct heap {
	int len;
	int cap;
	struct heap_entry *data;
};

struct heap *new_heap(int size);
void heapify(struct heap *h, int i);
void build_min_heap(struct heap *h);
void heap_add(struct heap *h, struct int_array *ia);

#endif
