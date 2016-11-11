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

#include <config.h>
#include <stdlib.h>
#include <time.h>
#include "cpucheck.h"

struct elt {
	unsigned long int a;
	unsigned long int b;
	unsigned long int c;
	unsigned long int res;
};

struct comp {
	unsigned long int res;
};

static int init(void * const config, void * const table, const size_t table_size)
{
	size_t i;
	struct elt * const elts = table;

	srandom(time(NULL));

	for (i=0 ; i<table_size ; i++) {
		unsigned long int a, b;
		a = ulirandom();
		b = ulirandom();
		elts[i].a = a < b ? b : a;
		elts[i].b = a < b ? a : b;
		elts[i].c = random();
		elts[i].res = elts[i].a/elts[i].b*elts[i].c;
	}

	return 0;
}

static int check_item(void * const comp, void const * const config, void const * const table_element)
{
	struct elt const * const elt = table_element;
	struct comp * const c = comp;

	c->res = elt->a / elt->b * elt->c;

	return ! (c->res == elt->res);
}

static void report_error(FILE *out, void const * const config, void const * const table_element, void const * const comp)
{
	struct elt const * const elt = table_element;
	struct comp const * const c = comp;

	fprintf(out, "a=%lu, b=%lu, c=%lu ; expected=%lu, got=%lu\n", elt->a, elt->b,
			elt->c, elt->res, c->res);
}

CPUCHECK_CHECKER(muldiv, "Performs integer multiplications and divisions", 0, sizeof(struct elt), sizeof(struct comp), init, check_item, report_error, NULL)

