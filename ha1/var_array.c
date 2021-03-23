#include "var_array.h"

struct var_array * var_array_init(size_t reserve, size_t el_size) {
	struct var_array *new = malloc(sizeof(struct var_array));
	new->data = malloc(reserve * el_size);
	new->el_size = el_size;
	new->cap = reserve;
	new->len = 0;

	return new;
}


