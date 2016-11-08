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

#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include "cpucheck.h"

struct elt {
	uint64_t a;
	uint64_t b;
	uint64_t and;
	uint64_t or;
	uint64_t xor;
	uint64_t nota;
};

struct comp {
	uint64_t and;
	uint64_t or;
	uint64_t xor;
	uint64_t nota;
};

static int init_table(void *table, const size_t table_size)
{
	size_t i;
	struct elt * const elts = table;

	srandom(time(NULL));

	for(i=0 ; i<table_size ; i++) {
		elts[i].a = u64random();
		elts[i].b = u64random();
		elts[i].and = elts[i].a & elts[i].b;
		elts[i].or = elts[i].a | elts[i].b;
		elts[i].xor = elts[i].a ^ elts[i].b;
		elts[i].nota = ~elts[i].a;
	}

	return 0;
}

static int check_item(void * const comp, void const * const table, const size_t index)
{
	struct elt const * const elts = table;
	struct comp * const c = comp;

	c->and = elts[index].a & elts[index].b;
	c->or = elts[index].a | elts[index].b;
	c->xor = elts[index].a ^ elts[index].b;
	c->nota = ~elts[index].a;

	return ! (elts[index].and == c->and
			&& elts[index].or == c->or
			&& elts[index].xor == c->xor
			&& elts[index].nota == c->nota);
}

static void report_error(FILE *out, void const * const table, const size_t index, void const * const comp)
{
	struct elt const * const elts = table;
	struct comp const * const c = comp;

	fprintf(out, "a=0x%" PRIx64 ", b=0x%" PRIx64 "\n", elts[index].a, elts[index].b);
	fprintf(out, "and: expected=0x%" PRIx64 ", got=0x%" PRIx64 "\n", elts[index].and, c->and);
	fprintf(out, "or: expected=0x%" PRIx64 ", got=0x%" PRIx64 "\n", elts[index].or, c->or);
	fprintf(out, "xor: expected=0x%" PRIx64 ", got=0x%" PRIx64 "\n", elts[index].xor, c->xor);
	fprintf(out, "not a: expected=0x%" PRIx64 ", got=0x%" PRIx64 "\n", elts[index].nota, c->nota);
}

CPUCHECK_CHECKER(bool, "Performs boolean and, or, xor, and not", sizeof(struct elt), sizeof(struct comp), init_table, check_item, report_error, NULL)

