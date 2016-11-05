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
	void *base;
	uint64_t offset;
	void const * nomul;
	void const * mul2;
	void const * mul4;
	void const * mul8;
};

static int init_table(void *table, const size_t table_size)
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

static int check_item(void const * const table, const size_t index)
{
	struct elt const * const elts = table;
	const void *nomul, *mul2, *mul4, *mul8;

	asm("leaq (%[base], %[offset]), %[nomul] \n\t"
		"leaq (%[base], %[offset], 2), %[mul2] \n\t"
		"leaq (%[base], %[offset], 4), %[mul4] \n\t"
		"leaq (%[base], %[offset], 8), %[mul8] \n\t"
		: [nomul] "=r" (nomul), [mul2] "=r" (mul2),
		  [mul4] "=r" (mul4), [mul8] "=r" (mul8)
		: [base] "r" (elts[index].base), [offset] "r" (elts[index].offset)
	);

	return ! (nomul == elts[index].nomul
				&& mul2 == elts[index].mul2
				&& mul4 == elts[index].mul4
				&& mul8 == elts[index].mul8);
}

CPUCHECK_CHECKER(lea, "Performs integer additions and multiplications using lea", sizeof(struct elt), init_table, check_item, NULL)

#endif /* ARCH_X86_64 */

