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
	uint64_t a;
	int zero:1;
	uint64_t ri;
	uint64_t li;
};

struct comp {
	uint64_t ri;
	uint64_t li;
	uint8_t rz;
	uint8_t lz;
};

static int init(void * const config, void * const table, const size_t table_size)
{
	size_t i;
	uint64_t j;
	struct elt * const elts = table;

	srandom(time(NULL));

	for(i=0 ; i<table_size ; i++) {
		elts[i].a = u64random();
		elts[i].zero = !elts[i].a;
		if (!elts[i].zero) {
			for(j=0 ; !((elts[i].a >> j) & 1) ; j++) ;
			elts[i].ri = j;
			for(j=0 ; !((elts[i].a << j) & (1ULL<<63)) ; j++) ;
			elts[i].li = 63-j;
		}
	}

	return 0;
}

static int check_item(void * const comp, void const * const config, void const * const table_element)
{
	struct elt const * const elt = table_element;
	struct comp * const c = comp;

	asm("bsfq %[a], %[ri] \n\t"
		"setzb %[rz] \n\t"
		"bsrq %[a], %[li] \n\t"
		"setzb %[lz] \n\t"
		: [ri] "=&r" (c->ri), [rz] "=&r" (c->rz),
		  [li] "=&r" (c->li), [lz] "=&r" (c->lz)
		: [a] "rm" (elt->a)
		: "cc"
	);

	return ! (
		elt->zero == c->rz
		&& c->rz == c->lz
		&& (c->rz || elt->ri == c->ri)
		&& (c->rz || elt->li == c->li)
	);
}

static void report_error(FILE *out, void const * const config, void const * const table_element, void const * const comp)
{
	struct elt const * const elt = table_element;
	struct comp const * const c = comp;

	fprintf(out, "a=0x%" PRIx64 "\n", elt->a);
	fprintf(out, "Right index: expected=0x%" PRIx64 ", got=0x%" PRIx64 "\n", elt->ri, c->ri);
	fprintf(out, "Left index: expected=0x%" PRIx64 ", got=0x%" PRIx64 "\n", elt->li, c->li);
	fprintf(out, "Found bit set, by right: %s, by left: %s", c->rz?"false":"true", c->lz?"false":"true");
}

CPUCHECK_CHECKER(bitscan, "Performs bit scanning (bsf/bsr)", 0, sizeof(struct elt), sizeof(struct comp), init, check_item, report_error, NULL)

#endif /* ARCH_X86_64 */

