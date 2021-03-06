/* Copyright Etienne Buira
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 */

#ifndef CPUCHECK_H
#define CPUCHECK_H

#include <stdio.h>
#include <stdint.h>

struct cpucheck_checker {
	char const * const name;
	char const * const description;
	const size_t config_size;
	const size_t table_elt_size;
	const size_t comp_elt_size;
	int (*init)(void * const config, void * const table, const size_t table_size);
	int (*check_item)(void * const comp, void const * const config, void const * const table_element);
	void (*report_error)(FILE *out, void const * const config, void const * const table_element, void const * const comp);
	void (*delete)(void * const config, void * const table, const size_t table_size);
};

#define CPUCHECK_CHECKER(arg_name, arg_description, arg_config_size, arg_table_elt_size, arg_comp_elt_size, arg_init, arg_check_item, arg_report_error, arg_delete) \
	struct cpucheck_checker cpucheck_checker_##arg_name = { \
		.name = #arg_name, \
		.description = arg_description, \
		.config_size = arg_config_size, \
		.table_elt_size = arg_table_elt_size, \
		.comp_elt_size = arg_comp_elt_size, \
		.init = arg_init, \
		.check_item = arg_check_item, \
		.report_error = arg_report_error, \
		.delete = arg_delete, \
	};

unsigned long int ulirandom(void);
uint64_t u64random(void);

void hex_dump(FILE *out, char const * const what, char const * const todump, const size_t len);

#endif
