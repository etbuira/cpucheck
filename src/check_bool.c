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

static int init(void * const config, void * const table, const size_t table_size)
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

static int check_item(void * const comp, void const * const config, void const * const table_element)
{
	struct elt const * const elt = table_element;
	struct comp * const c = comp;

	c->and = elt->a & elt->b;
	c->or = elt->a | elt->b;
	c->xor = elt->a ^ elt->b;
	c->nota = ~elt->a;

	return ! (elt->and == c->and
			&& elt->or == c->or
			&& elt->xor == c->xor
			&& elt->nota == c->nota);
}

static void report_error(FILE *out, void const * const config, void const * const table_element, void const * const comp)
{
	struct elt const * const elt = table_element;
	struct comp const * const c = comp;

	fprintf(out, "a=0x%" PRIx64 ", b=0x%" PRIx64 "\n", elt->a, elt->b);
	fprintf(out, "and: expected=0x%" PRIx64 ", got=0x%" PRIx64 "\n", elt->and, c->and);
	fprintf(out, "or: expected=0x%" PRIx64 ", got=0x%" PRIx64 "\n", elt->or, c->or);
	fprintf(out, "xor: expected=0x%" PRIx64 ", got=0x%" PRIx64 "\n", elt->xor, c->xor);
	fprintf(out, "not a: expected=0x%" PRIx64 ", got=0x%" PRIx64 "\n", elt->nota, c->nota);
}

CPUCHECK_CHECKER(bool, "Performs boolean and, or, xor, and not", 0, sizeof(struct elt), sizeof(struct comp), init, check_item, report_error, NULL)

