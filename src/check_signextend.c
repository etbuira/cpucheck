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
#include <stdint.h>
#include <inttypes.h>
#include "cpucheck.h"

struct elt {
	int8_t byte;
	int16_t byte_ex;
	int16_t word;
	int32_t word_ex;
	int16_t word_exh;
	int16_t word_exl;
	int32_t dword;
	int64_t dword_ex;
	int32_t dword_exh;
	int32_t dword_exl;
	int64_t qword;
	int64_t qword_exh;
	int64_t qword_exl;
};

struct comp {
	int16_t byte_ex;
	int32_t word_ex;
	int16_t word_exh;
	int16_t word_exl;
	int64_t dword_ex;
	int32_t dword_exh;
	int32_t dword_exl;
	int64_t qword_exh;
	int64_t qword_exl;
};

static int init(void * const config, void *table, const size_t table_size)
{
	size_t i;
	struct elt * const elts = table;

	srandom(time(NULL));

	for (i=0 ; i<table_size ; i++) {
		elts[i].byte = u64random();
		elts[i].byte_ex = elts[i].byte;
		elts[i].word = u64random();
		elts[i].word_ex = elts[i].word;
		elts[i].word_exl = elts[i].word;
		elts[i].word_exh = elts[i].word < 0 ? -1 : 0;
		elts[i].dword = u64random();
		elts[i].dword_ex = elts[i].dword;
		elts[i].dword_exl = elts[i].dword;
		elts[i].dword_exh = elts[i].dword < 0 ? -1 : 0;
		elts[i].qword = u64random();
		elts[i].qword_exl = elts[i].qword;
		elts[i].qword_exh = elts[i].qword < 0 ? -1 : 0;
	}

	return 0;
}

static int check_item(void * const comp, void const * const config, void const * const table, const size_t index)
{
	struct elt const * const elts = table;
	struct comp * const c = comp;

	asm("movb %[byte], %%al \n\t"
		"cbw \n\t"
		"movw %%ax, %[byte_ex] \n\t"
		"movw %[word], %%ax \n\t"
		"cwde \n\t"
		"cwd \n\t"
		"movl %%eax, %[word_ex] \n\t"
		"movw %%ax, %[word_exl] \n\t"
		"movw %%dx, %[word_exh] \n\t"
		"movl %[dword], %%eax \n\t"
		"cdqe \n\t"
		"cdq \n\t"
		"movq %%rax, %[dword_ex] \n\t"
		"movl %%eax, %[dword_exl] \n\t"
		"movl %%edx, %[dword_exh] \n\t"
		"movq %[qword], %%rax \n\t"
		"cqo \n\t"
		"movq %%rax, %[qword_exl] \n\t"
		"movq %%rdx, %[qword_exh] \n\t"
		: [byte_ex] "=rm" (c->byte_ex), [word_ex] "=rm" (c->word_ex),
		  [word_exl] "=rm" (c->word_exl), [word_exh] "=rm" (c->word_exh),
		  [dword_ex] "=rm" (c->dword_ex), [dword_exl] "=rm" (c->dword_exl),
		  [dword_exh] "=rm" (c->dword_exh), [qword_exl] "=rm" (c->qword_exl),
		  [qword_exh] "=rm" (c->qword_exh)
		: [byte] "rm" (elts[index].byte), [word] "rm" (elts[index].word),
		  [dword] "rm" (elts[index].dword), [qword] "rm" (elts[index].qword)
		: "rax", "rdx"
	);

	return !( c->byte_ex == elts[index].byte_ex
				&& c->word_ex == elts[index].word_ex
				&& c->word_exl == elts[index].word_exl
				&& c->word_exh == elts[index].word_exh
				&& c->dword_ex == elts[index].dword_ex
				&& c->dword_exl == elts[index].dword_exl
				&& c->dword_exh == elts[index].dword_exh
				&& c->qword_exl == elts[index].qword_exl
				&& c->qword_exh == elts[index].qword_exh );

}

static void report_error(FILE *out, void const * const config, void const * const table, const size_t index, void const * const comp)
{
	struct elt const * const elts = table;
	struct comp const * const c = comp;

	fprintf(out, "byte=0x%" PRIx8 ", expected=0x%" PRIx16 ", got=0x%" PRIx16 "\n",
			elts[index].byte, elts[index].byte_ex, c->byte_ex);
	fprintf(out, "word=0x%" PRIx16 ", expected=0x%" PRIx32 ", got=0x%" PRIx32 "\n",
			elts[index].word, elts[index].word_ex, c->word_ex);
	fprintf(out, "\tlow: expected=0x%" PRIx16 ", got=0x%" PRIx16 "\n",
			elts[index].word_exl, c->word_exl);
	fprintf(out, "\thigh: expected=0x%" PRIx16 ", got=0x%" PRIx16 "\n",
			elts[index].word_exh, c->word_exh);
	fprintf(out, "dword=0x%" PRIx32 ", expected=0x%" PRIx64 ", got=0x%" PRIx64 "\n",
			elts[index].dword, elts[index].dword_ex, c->dword_ex);
	fprintf(out, "\tlow: expected=0x%" PRIx32 ", got=0x%" PRIx32 "\n",
			elts[index].dword_exl, c->dword_exl);
	fprintf(out, "\thigh: expected=0x%" PRIx32 ", got=0x%" PRIx32 "\n",
			elts[index].dword_exh, c->dword_exh);
	fprintf(out, "qword=0x%" PRIx64 "\n", elts[index].qword);
	fprintf(out, "\tlow: expected=0x%" PRIx64 ", got=0x%" PRIx64 "\n",
			elts[index].qword_exl, c->qword_exl);
	fprintf(out, "\thigh: expected=0x%" PRIx64 ", got=0x%" PRIx64 "\n",
			elts[index].qword_exh, c->qword_exh);
}

CPUCHECK_CHECKER(signextend, "Performs sign extension (cbw, cwde, cdqe, cwd, cdq, cqo)", 0, sizeof(struct elt), sizeof(struct comp), init, check_item, report_error, NULL)

#endif /* ARCH_X86_64 */
