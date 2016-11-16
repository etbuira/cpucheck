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
#include <string.h>
#include "cpucheck.h"

#define MAX_STR_SZ 1024

struct elt {
	size_t len;
	char * src;
};

struct comp {
	char byte[MAX_STR_SZ];
	char word[MAX_STR_SZ];
	char dword[MAX_STR_SZ];
	char qword[MAX_STR_SZ];
};

static int init(void * const config, void * const table, const size_t table_size)
{
	size_t i, j;
	struct elt * const elts = table;

	srandom(time(NULL));

	for (i=0 ; i<table_size ; i++) {
		elts[i].len = random()%MAX_STR_SZ;

		elts[i].src = malloc(elts[i].len);
		if (!elts[i].src)
			goto err;

		for (j=0 ; j<elts[i].len ; j++)
			elts[i].src[j] = random();
	}

	return 0;

err:
	for (j=0 ; j<i ; j++)
		free(elts[j].src);
	return -1;
}

#define CHECK_COPY(arg_dst, arg_lods, arg_stos, arg_word_size) do { \
	uint64_t len = elt->len/(arg_word_size); \
	char const * src = elt->src; \
	char * dst = (arg_dst); \
 \
	if (len) { \
		asm ("0: \n\t" \
			arg_lods "\n\t" \
			arg_stos "\n\t" \
			"decq %[len] \n\t" \
			"jnz 0b \n\t" \
			: [len] "+r" (len), "+S" (src), "+D" (dst) \
			: \
			: "rax", "cc", "memory" \
		); \
\
		cmp |= len != 0 || memcmp(elt->src, (arg_dst), elt->len/(arg_word_size)); \
	} \
} while(0);
static int check_item(void * const comp, void const * const config, void const * const table_element)
{
	struct elt const * const elt = table_element;
	struct comp * const c = comp;
	int cmp = 0;

	CHECK_COPY(c->byte, "lodsb", "stosb", 1)
	CHECK_COPY(c->word, "lodsw", "stosw", 2)
	CHECK_COPY(c->dword, "lodsl", "stosl", 4)
	CHECK_COPY(c->qword, "lodsq", "stosq", 8)

	return cmp;
}
#undef CHECK_COPY

static void report_error(FILE *out, void const * const config, void const * const table_element, void const * const comp)
{
	struct elt const * const elt = table_element;
	struct comp const * const c = comp;

	hex_dump(out, "src=", elt->src, elt->len);
	hex_dump(out, "lodsb/stosb result=", c->byte, elt->len);
	hex_dump(out, "lodsw/stosw result=", c->word, elt->len/2);
	hex_dump(out, "lodsd/stosd result=", c->dword, elt->len/4);
	hex_dump(out, "lodsq/stosq result=", c->qword, elt->len/8);
}

static void delete(void * const config, void * const table, const size_t table_size)
{
	size_t i;
	struct elt * const elts = table;

	for (i=0 ; i<table_size ; i++)
		free(elts[i].src);
}

CPUCHECK_CHECKER(lodsstos, "Performs string copy using lods* and stos*", 0, sizeof(struct elt), sizeof(struct comp), init, check_item, report_error, delete)

#endif	/* ARCH_X86_64 */

