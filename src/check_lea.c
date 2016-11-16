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

#if ARCH_X86_64

#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include "cpucheck.h"

struct elt {
	void *base;
	uint64_t offset;
	void const * nomul;
	void const * mul2;
	void const * mul4;
	void const * mul8;
};

struct comp {
	void const * nomul;
	void const * mul2;
	void const * mul4;
	void const * mul8;
};

static int init_table(void * const config, void * const table, const size_t table_size)
{
	size_t i;
	struct elt * const elts = table;

	srandom(time(NULL));

	for (i=0 ; i<table_size ; i++) {
		uint8_t *base;
		base = elts[i].base = (void*) random();
		elts[i].offset = random()%256;
		elts[i].nomul = base+elts[i].offset;
		elts[i].mul2 = base+elts[i].offset*2;
		elts[i].mul4 = base+elts[i].offset*4;
		elts[i].mul8 = base+elts[i].offset*8;
	}

	return 0;
}

static int check_item(void * const comp, void const * const config, void const * const table_element)
{
	struct elt const * const elt = table_element;
	struct comp * const c = comp;

	asm("leaq (%[base], %[offset]), %[nomul] \n\t"
		"leaq (%[base], %[offset], 2), %[mul2] \n\t"
		"leaq (%[base], %[offset], 4), %[mul4] \n\t"
		"leaq (%[base], %[offset], 8), %[mul8] \n\t"
		: [nomul] "=&r" (c->nomul), [mul2] "=&r" (c->mul2),
		  [mul4] "=&r" (c->mul4), [mul8] "=&r" (c->mul8)
		: [base] "r" (elt->base), [offset] "r" (elt->offset)
	);

	return ! (c->nomul == elt->nomul
				&& c->mul2 == elt->mul2
				&& c->mul4 == elt->mul4
				&& c->mul8 == elt->mul8);
}

static void report_error(FILE *out, void const * const config, void const * const table_element, void const * const comp)
{
	struct elt const * const elt = table_element;
	struct comp const * const c = comp;

	fprintf(out, "base=%p, offset=0x%" PRIx64 "\n", elt->base, elt->offset);
	fprintf(out, "nomul, expected=%p, got=%p\n", elt->nomul, c->nomul);
	fprintf(out, "mul2, expected=%p, got=%p\n", elt->mul2, c->mul2);
	fprintf(out, "mul4, expected=%p, got=%p\n", elt->mul4, c->mul4);
	fprintf(out, "mul8, expected=%p, got=%p\n", elt->mul8, c->mul8);
}

CPUCHECK_CHECKER(lea, "Performs integer additions and multiplications using lea", 0, sizeof(struct elt), sizeof(struct comp), init_table, check_item, report_error, NULL)

#endif /* ARCH_X86_64 */

