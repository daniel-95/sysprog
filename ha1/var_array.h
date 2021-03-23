#ifndef __VAR_ARRAY_H
#define __VAR_ARRAY_H

struct var_array {
	void *data;
	int len;
	int cap;
	int el_size;
};

static struct var_array * var_array_init(size_t reserve, size_t el_size) {
	struct var_array *new = malloc(sizeof(struct var_array));
	new->data = malloc(reserve * el_size);
	new->el_size = el_size;
	new->cap = reserve;
	new->len = 0;

	return new;
}

#define var_array_put(va, el, type) {                                \
	if(va->len >= va->cap) {                                     \
		va->cap *= 2;                                        \
		va->data = realloc(va->data, va->cap * va->el_size); \
	}                                                            \
	((type*)va->data)[va->len++] = el;                           \
}

#define var_array_len(va) (va->len)
#define var_array_get(va, idx, type) ((type)((type*)va->data)[idx])
#define var_array_set(va, idx, type, val) (((type*)va->data)[idx] = val)

#define var_array_swap(va, idx_a, idx_b, type) {                        \
	type tmp = var_array_get(va, idx_a, type);                      \
	var_array_set(va, idx_a, type, var_array_get(va, idx_b, type)); \
	var_array_set(va, idx_b, type, tmp);                            \
}

#define var_array_free(va) { \
	free(va->data);      \
	free(va);            \
}

#endif
