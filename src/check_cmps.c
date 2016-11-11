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
#include <time.h>
#include "cpucheck.h"

#define MISMATCH_COUNT 5
#define MAXSTRLEN 256

struct elt {
	size_t len;
	char *a;
	char *b;
	int mismatch_byte[MISMATCH_COUNT];
	int mismatch_word[MISMATCH_COUNT];
	int mismatch_double[MISMATCH_COUNT];
	int mismatch_quad[MISMATCH_COUNT];
};

struct comp {
	int mismatch_byte[MISMATCH_COUNT];
	int mismatch_word[MISMATCH_COUNT];
	int mismatch_double[MISMATCH_COUNT];
	int mismatch_quad[MISMATCH_COUNT];
	int too_much;
};

static void fill_mismatch(int *mismatches,
		struct elt const * const elt,
		size_t word_size)
{
	size_t i, mmidx;

	for (i=0, mmidx=0 ; i<elt->len/word_size ; i++) {
		int mm;
		size_t k;

		for(k=0, mm=0 ; k<word_size ; k++)
			mm |= elt->a[i*word_size+k] != elt->b[i*word_size+k];

		if (mm)
			mismatches[mmidx++] = i;
	}

	for ( ; mmidx<MISMATCH_COUNT ; mmidx++)
		mismatches[mmidx] = -1;
}

static void introduce_mismatches(struct elt * const elt)
{
	size_t i, stri;

	for(i=0, stri=0 ; i<MISMATCH_COUNT && elt->len-stri ; i++) {
		size_t curoff = random() % (elt->len-stri);

		while(elt->a[stri+curoff] == elt->b[stri+curoff])
			elt->b[stri+curoff] = random();
		stri += curoff+1;
	}
}

static int init(void * const config, void *table, const size_t table_size)
{
	size_t i, j;
	struct elt * const elts = table;
	int r;

	srandom(time(NULL));

	for(i=0 ; i<table_size ; i++) {
		elts[i].len = random()%MAXSTRLEN;
		if (! (elts[i].a = malloc(elts[i].len))) {
			r = -1;
			goto err;
		}
		if (! (elts[i].b = malloc(elts[i].len))) {
			free(elts[i].a);
			r = -1;
			goto err;
		}

		for(j=0 ; j<elts[i].len ; j++)
			elts[i].a[j] = elts[i].b[j] = random();

		introduce_mismatches(&elts[i]);

		fill_mismatch(elts[i].mismatch_byte, &elts[i], 1);
		fill_mismatch(elts[i].mismatch_word, &elts[i], 2);
		fill_mismatch(elts[i].mismatch_double, &elts[i], 4);
		fill_mismatch(elts[i].mismatch_quad, &elts[i], 8);
	}

	return 0;

err:
	for(j=0 ; j<i ; j++) {
		free(elts[j].a);
		free(elts[j].b);
	}
	return r;
}

#define COMP_MISMATCH(arg_mm, arg_word_size, arg_cmp_op) do { \
	size_t stridx, mmidx; \
	const size_t len = elt->len/(arg_word_size); \
	\
	for (stridx=0, mmidx=0 ; stridx < len ; ) { \
		uint64_t rem_len = len-stridx; \
		uint8_t zf; \
		\
		asm("repz " arg_cmp_op " \n\t" \
			"setzb %[zf] \n\t" \
			: "+c" (rem_len), [zf] "=mr" (zf) \
			: "S" (elt->a+stridx*(arg_word_size)), \
			  "D" (elt->b+stridx*(arg_word_size)) \
			: "cc" \
		); \
		\
		stridx += len-stridx-rem_len; \
		\
		if (!zf) { \
			if (mmidx == MISMATCH_COUNT) \
				c->too_much = 1; \
			else \
				(arg_mm)[mmidx++] = stridx-1; \
		} \
	} \
	\
	for ( ; mmidx < MISMATCH_COUNT ; mmidx++) \
		(arg_mm)[mmidx] = -1; \
} while (0);
static int check_item(void * const comp, void const * const config, void const * const table_element)
{
	struct elt const * const elt = table_element;
	struct comp * const c = comp;
	size_t i;

	c->too_much = 0;

	COMP_MISMATCH(c->mismatch_byte, 1, "cmpsb")
	COMP_MISMATCH(c->mismatch_word, 2, "cmpsw")
	COMP_MISMATCH(c->mismatch_double, 4, "cmpsd")
	COMP_MISMATCH(c->mismatch_quad, 8, "cmpsq")

	for(i=0 ; i<MISMATCH_COUNT ; i++) {
		if (c->mismatch_byte[i] != elt->mismatch_byte[i])
			return 1;
		if (c->mismatch_word[i] != elt->mismatch_word[i])
			return 1;
		if (c->mismatch_double[i] != elt->mismatch_double[i])
			return 1;
		if (c->mismatch_quad[i] != elt->mismatch_quad[i])
			return 1;
	}
	if (c->too_much)
		return 1;

	return 0;
}
#undef COMP_MISMATCH

#define PRINT_MM(arg_what, arg_mm_expected, arg_mm_got) do { \
	size_t i; \
	fprintf(out, "%s (expected): ", arg_what); \
	for (i=0 ; i<MISMATCH_COUNT ; i++) \
		fprintf(out, "%d%s", (arg_mm_expected)[i], i==MISMATCH_COUNT-1 ? "\n" : ", "); \
	fprintf(out, "%s (got): ", arg_what); \
	for (i=0 ; i<MISMATCH_COUNT ; i++) \
		fprintf(out, "%d%s", (arg_mm_got)[i], i==MISMATCH_COUNT-1 ? "\n" : ", "); \
} while (0);
static void report_error(FILE *out, void const * const config, void const * const table_element, void const * const comp)
{
	struct elt const * const elt = table_element;
	struct comp const * const c = comp;

	hex_dump(out, "a=", elt->a, elt->len);
	hex_dump(out, "b=", elt->b, elt->len);

	PRINT_MM("byte", elt->mismatch_byte, c->mismatch_byte)
	PRINT_MM("word", elt->mismatch_word, c->mismatch_word)
	PRINT_MM("double", elt->mismatch_double, c->mismatch_double)
	PRINT_MM("quad", elt->mismatch_quad, c->mismatch_quad)
	fprintf(out, "too much found: %s\n", c->too_much ? "yes" : "no");
}
#undef PRINT_MM

static void delete(void * const config, void *table, const size_t table_size)
{
	struct elt * const elts = table;
	size_t i;

	for (i=0 ; i<table_size ; i++) {
		free(elts[i].a);
		free(elts[i].b);
	}
}

CPUCHECK_CHECKER(cmps, "Performs string comparisons on different word sizes (cmpsb, cmpsw, cmpsd, cmpsq)", 0, sizeof(struct elt), sizeof(struct comp), init, check_item, report_error, delete)

#endif	/* ARCH_X86_64 */
