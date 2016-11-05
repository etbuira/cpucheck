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

#if ARCH_X86_64

#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "cpucheck.h"

struct elt {
	uint64_t a;
	int zero:1;
	uint64_t ri;
	uint64_t li;
};

static int init_table(void *table, const size_t table_size)
{
	size_t i;
	uint64_t j;
	struct elt * const elts = table;

	srandom(time(NULL));

	for(i=0 ; i<table_size ; i++) {
		elts[i].a = random();
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

static int check_item(void const * const table, const size_t index)
{
	struct elt const * const elts = table;
	uint64_t ri, li;
	uint8_t rz, lz;

	asm("bsfq %[a], %[ri] \n\t"
		"setzb %[rz] \n\t"
		"bsrq %[a], %[li] \n\t"
		"setzb %[lz] \n\t"
		: [ri] "=r" (ri), [rz] "=r" (rz),
		  [li] "=r" (li), [lz] "=r" (lz)
		: [a] "rm" (elts[index].a)
		: "cc"
	);

	return ! (
		elts[index].zero == rz
		&& rz == lz
		&& (rz || elts[index].ri == ri)
		&& (rz || elts[index].li == li)
	);
}

CPUCHECK_CHECKER(bitscan, "Performs bit scanning (bsf/bsr)", sizeof(struct elt), init_table, check_item, NULL)

#endif /* ARCH_X86_64 */

