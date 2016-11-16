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
#include <inttypes.h>
#include <time.h>
#include "cpucheck.h"

struct elt {
	uint64_t subject;
	uint64_t res;
	uint8_t zf;
	uint8_t cf;
};

struct comp {
	uint64_t res;
	uint8_t zf;
	uint8_t cf;
};

static int check_availability(void)
{
	uint8_t ret;
	uint32_t tmp;

	asm("movl $0x80000000, %%eax \n\t"
		"cpuid \n\t"
		: "=a" (tmp)
		:
		: "ebx", "ecx", "edx"
	);

	if (tmp <= 0x80000000)
		return -1;
	
	asm("movl $0x80000001, %%eax \n\t"
		"cpuid \n\t"
		"test $0x20, %%ecx \n\t"
		"setzb %[ret] \n\t"
		: [ret] "=rm" (ret)
		:
		: "eax", "ebx", "ecx", "edx", "cc"
	);

	return ret;
}

static int init(void * const config, void * const table, const size_t table_size)
{
	size_t i;
	struct elt * const elts = table;

	if (check_availability())
		return -1;

	srandom(time(NULL));

	for (i=0 ; i<table_size ; i++) {
		elts[i].subject = u64random();
		elts[i].cf = !elts[i].subject;
		if (!elts[i].subject) {
			elts[i].res = 64;
		} else {
			for (elts[i].res=63 ; !(elts[i].subject >> elts[i].res & 1) ; elts[i].res--) ;
			elts[i].res = 63-elts[i].res;
		}
		elts[i].zf = !elts[i].res;
	}

	return 0;
}

static int check_item(void * const comp, void const * const config, void const * const table_element)
{
	struct elt const * const elt = table_element;
	struct comp * const c = comp;

	asm("lzcntq %[subj], %[res] \n\t"
		"setzb %[zf] \n\t"
		"setcb %[cf] \n\t"
		: [res] "=&r" (c->res), [zf] "=&rm" (c->zf), [cf] "=&rm" (c->cf)
		: [subj] "rm" (elt->subject)
		: "cc"
	);

	return ! ( c->res == elt->res
			&& c->zf == elt->zf
			&& c->cf == elt->cf
	);
}

static void report_error(FILE *out, void const * const config, void const * const table_element, void const * const comp)
{
	struct elt const * const elt = table_element;
	struct comp const * const c = comp;

	fprintf(out, "subject=0x%" PRIx64 "\n", elt->subject);
	fprintf(out, "res, expected=0x%" PRIx64 ", got=0x%" PRIx64 "\n", elt->res, c->res);
	fprintf(out, "zf, expected=%s, got=%s\n", elt->zf?"yes":"no", c->zf?"yes":"no");
	fprintf(out, "cf, expected=%s, got=%s\n", elt->cf?"yes":"no", c->cf?"yes":"no");
}

CPUCHECK_CHECKER(lzcnt, "Count number of leading zeroes using lzcnt", 0, sizeof(struct elt), sizeof(struct comp), init, check_item, report_error, NULL)

#endif	/* ARCH_X86_64 */

