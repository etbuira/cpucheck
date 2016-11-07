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
#include <time.h>
#include <stdint.h>
#include <inttypes.h>
#include "cpucheck.h"

struct elt {
	int8_t byte;
	int16_t byte_ex;
	int16_t word;
	int32_t word_ex;
	int32_t dword;
	int64_t dword_ex;
};

struct comp {
	int16_t byte_ex;
	int32_t word_ex;
	int64_t dword_ex;
};

static int init_table(void *table, const size_t table_size)
{
	size_t i;
	struct elt * const elts = table;

	srandom(time(NULL));

	for (i=0 ; i<table_size ; i++) {
		elts[i].byte = random();
		elts[i].byte_ex = elts[i].byte;
		elts[i].word = random();
		elts[i].word_ex = elts[i].word;
		elts[i].dword = random();
		elts[i].dword_ex = elts[i].dword;
	}

	return 0;
}

static int check_item(void * const comp, void const * const table, const size_t index)
{
	struct elt const * const elts = table;
	struct comp * const c = comp;

	asm("movb %[byte], %%al \n\t"
		"cbw \n\t"
		"movw %%ax, %[byte_ex] \n\t"
		"movw %[word], %%ax \n\t"
		"cwde \n\t"
		"movl %%eax, %[word_ex] \n\t"
		"movl %[dword], %%eax \n\t"
		"cdqe \n\t"
		"movq %%rax, %[dword_ex] \n\t"
		: [byte_ex] "=rm" (c->byte_ex), [word_ex] "=rm" (c->word_ex),
		  [dword_ex] "=rm" (c->dword_ex)
		: [byte] "rm" (elts[index].byte), [word] "rm" (elts[index].word),
		  [dword] "rm" (elts[index].dword)
		: "rax"
	);

	return !( c->byte_ex == elts[index].byte_ex
				&& c->word_ex == elts[index].word_ex
				&& c->dword_ex == elts[index].dword_ex );

}

static void report_error(FILE *out, void const * const table, const size_t index, void const * const comp)
{
	struct elt const * const elts = table;
	struct comp const * const c = comp;

	fprintf(out, "byte=0x%" PRIx8 ", expected=0x%" PRIx16 ", got=0x%" PRIx16 "\n",
			elts[index].byte, elts[index].byte_ex, c->byte_ex);
	fprintf(out, "word=0x%" PRIx16 ", expected=0x%" PRIx32 ", got=0x%" PRIx32 "\n",
			elts[index].word, elts[index].word_ex, c->word_ex);
	fprintf(out, "dword=0x%" PRIx32 ", expected=0x%" PRIx64 ", got=0x%" PRIx64 "\n",
			elts[index].dword, elts[index].dword_ex, c->dword_ex);
}

CPUCHECK_CHECKER(signextend, "Performs sign extension (cbw, cwde, cdqe)", sizeof(struct elt), sizeof(struct comp), init_table, check_item, report_error, NULL)

#endif /* ARCH_X86_64 */
