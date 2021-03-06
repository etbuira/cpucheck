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

#define TEST_COUNT 8

struct bt {
	uint64_t bit_index;
	unsigned short int set:1;
	uint64_t toggled;
	uint64_t cleared;
	uint64_t set_ts;
};

struct elt {
	uint64_t a;
	struct bt tests[TEST_COUNT];
};

struct comp_bt {
	uint8_t set_t;
	uint8_t set_tc;
	uint64_t res_tc;
	uint8_t set_tr;
	uint64_t res_tr;
	uint8_t set_ts;
	uint64_t res_ts;
};

struct comp {
	struct comp_bt res[TEST_COUNT];
};

static int init(void * const config, void * const table, const size_t table_size)
{
	size_t i, j;
	struct elt * const elts = table;

	srandom(time(NULL));

	for(i=0 ; i<table_size ; i++) {
		elts[i].a = u64random();
		for(j=0 ; j<sizeof(elts[i].tests)/sizeof(elts[i].tests[0]) ; j++) {
			elts[i].tests[j].bit_index = random()%64;
			elts[i].tests[j].set = !! (elts[i].a & 1ULL<<elts[i].tests[j].bit_index);
			elts[i].tests[j].toggled = elts[i].a ^ 1ULL<<elts[i].tests[j].bit_index;
			elts[i].tests[j].cleared = elts[i].a & ~(1ULL<<elts[i].tests[j].bit_index);
			elts[i].tests[j].set_ts = elts[i].a | 1ULL<<elts[i].tests[j].bit_index;
		}
	}

	return 0;
}

static int check_item(void * const comp, void const * const config, void const * const table_element)
{
	struct elt const * const elt = table_element;
	struct comp * const c = comp;
	size_t j;

	for(j=0 ; j<sizeof(elt->tests)/sizeof(elt->tests[0]) ; j++) {
		asm("btq %[bidx], %[a] \n\t"
			"setcb %[set_t] \n\t"
			"movq %[a], %[res_tc] \n\t"
			"btcq %[bidx], %[res_tc] \n\t"
			"setcb %[set_tc] \n\t"
			"movq %[a], %[res_tr] \n\t"
			"btrq %[bidx], %[res_tr] \n\t"
			"setcb %[set_tr] \n\t"
			"movq %[a], %[res_ts] \n\t"
			"btsq %[bidx], %[res_ts] \n\t"
			"setcb %[set_ts] \n\t"
		: [set_t] "=&r" (c->res[j].set_t),
		  [set_tc] "=&r" (c->res[j].set_tc), [res_tc] "=&r" (c->res[j].res_tc),
		  [set_tr] "=&r" (c->res[j].set_tr), [res_tr] "=&r" (c->res[j].res_tr),
		  [set_ts] "=&r" (c->res[j].set_ts), [res_ts] "=&r" (c->res[j].res_ts)
		: [a] "mr" (elt->a),
		  [bidx] "r" (elt->tests[j].bit_index)
		: "cc"
		);
	}

	for(j=0 ; j<sizeof(elt->tests)/sizeof(elt->tests[0]) ; j++) {
		if (c->res[j].set_t != elt->tests[j].set
				|| c->res[j].set_t != c->res[j].set_tc
				|| c->res[j].set_tc != c->res[j].set_tr
				|| c->res[j].set_tr != c->res[j].set_ts
				|| c->res[j].res_tc != elt->tests[j].toggled
				|| c->res[j].res_tr != elt->tests[j].cleared
				|| c->res[j].res_ts != elt->tests[j].set_ts)
			return 1;
	}

	return 0;
}

static void report_error(FILE *out, void const * const config, void const * const table_element, void const * const comp)
{
	struct elt const * const elt = table_element;
	struct comp const * const c = comp;
	size_t j;

	fprintf(out, "a=%" PRIx64 "\n", elt->a);
	for(j=0 ; j<sizeof(elt->tests)/sizeof(elt->tests[0]) ; j++) {
		fprintf(out, "Bit index=%" PRIx64 "\n", elt->tests[j].bit_index);
		fprintf(out, "Bit set, expected: %s, got (bt): %s, got (btc): %s, got (btr): %s, got (bts): %s\n",
					elt->tests[j].set?"yes":"no", c->res[j].set_t?"yes":"no",
					c->res[j].set_tc?"yes":"no", c->res[j].set_tr?"yes":"no",
					c->res[j].set_ts?"yes":"no");
		fprintf(out, "btc result, expected=%" PRIx64 ", got=%" PRIx64 "\n",
					elt->tests[j].toggled, c->res[j].res_tc);
		fprintf(out, "btr result, expected=%" PRIx64 ", got=%" PRIx64 "\n",
					elt->tests[j].cleared, c->res[j].res_tr);
		fprintf(out, "bts result, expected=%" PRIx64 ", got=%" PRIx64 "\n",
					elt->tests[j].set_ts, c->res[j].res_ts);
	}
}

CPUCHECK_CHECKER(bittest, "Performs bit testing (bt, btc, btr, bts)", 0, sizeof(struct elt), sizeof(struct comp), init, check_item, report_error, NULL)

#endif
