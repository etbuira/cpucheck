#ifndef CPUCHECK_H
#define CPUCHECK_H

struct cpucheck_checker {
	char const * const name;
	char const * const description;
	size_t table_elt_size;
	int (*init_table)(void *table, const size_t table_size);
	int (*check_item)(void const * const table, const size_t index);
	void (*delete_table)(void *table, const size_t table_size);
};

#define CPUCHECK_CHECKER(arg_name, arg_description, arg_table_elt_size, arg_init_table, arg_check_item, arg_delete_table) \
	struct cpucheck_checker cpucheck_checker_##arg_name = { \
		.name = #arg_name, \
		.description = arg_description, \
		.table_elt_size = arg_table_elt_size, \
		.init_table = arg_init_table, \
		.check_item = arg_check_item, \
		.delete_table = arg_delete_table, \
	};

#endif