#include "var_array.h"
#include "macro.h"

struct var_array * var_array_init(size_t reserve, size_t el_size) {
	struct var_array *new = malloc(sizeof(struct var_array));
	memcheck(new, "couldn't allocate memory for new");

	new->data = malloc(reserve * el_size);
	memcheck(new->data, "couldn't allocate memory for new->data");

	new->el_size = el_size;
	new->cap = reserve;
	new->len = 0;

	return new;
}


