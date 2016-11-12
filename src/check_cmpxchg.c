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

struct cmpxchg {
	uint64_t a;
	uint64_t b;
	uint64_t c;
	uint64_t res_m;
	uint64_t res_rax;
	uint8_t zf;
};

struct cmpxchg8b {
	uint32_t ahi;
	uint32_t alo;
	uint64_t b;
	uint32_t chi;
	uint32_t clo;
	uint8_t zf;
	uint32_t edx;
	uint32_t eax;
	uint64_t res_m;
};

struct cmpxchg16b {
	uint64_t ahi;
	uint64_t alo;
	uint64_t bhi;
	uint64_t blo;
	uint64_t chi;
	uint64_t clo;
	uint8_t zf;
	uint64_t rdx;
	uint64_t rax;
	uint64_t res_m[2];
};

struct elt {
	struct cmpxchg cmpxchg;
	struct cmpxchg8b cmpxchg8b;
	struct cmpxchg16b cmpxchg16b;
};

struct comp_cmpxchg {
	uint64_t m;
	uint64_t rax;
	uint8_t zf;
};

struct comp_cmpxchg8b {
	uint64_t m;
	uint32_t edx;
	uint32_t eax;
	uint8_t zf;
};

struct comp_cmpxchg16b {
	uint64_t mbuf[4];
	uint64_t *m;
	uint64_t rdx;
	uint64_t rax;
	uint8_t zf;
};

struct comp {
	struct comp_cmpxchg cmpxchg;
	struct comp_cmpxchg8b cmpxchg8b;
	struct comp_cmpxchg16b cmpxchg16b;
};

struct config {
	unsigned int cmpxchg8b:1;
	unsigned int cmpxchg16b:1;
};

static void init_config(struct config * const cfg)
{
	uint8_t cmpxchg8b, cmpxchg16b;

	asm("movl $1, %%eax \n\t"
		"cpuid \n\t"
		"testl $0x80, %%edx \n\t"
		"setnzb %[cmpxchg8b] \n\t"
		"testl $0x2000, %%ecx \n\t"
		"setnzb %[cmpxchg16b] \n\t"
		: [cmpxchg8b] "=rm" (cmpxchg8b), [cmpxchg16b] "=rm" (cmpxchg16b)
		:
		: "eax", "ebx", "ecx", "edx", "cc"
	);

	cfg->cmpxchg8b = cmpxchg8b;
	cfg->cmpxchg16b = cmpxchg16b;
}

static void init_cmpxchg(struct cmpxchg * const elt)
{
	elt->a = u64random();
	elt->b = random()%2 ? elt->a : u64random();
	elt->c = u64random();
	elt->zf = elt->a == elt->b;
	elt->res_m = elt->zf ? elt->c : elt->b;
	elt->res_rax = elt->zf ? elt->a : elt->b;
}

static void init_cmpxchg8b(struct cmpxchg8b * const elt)
{
	elt->ahi = u64random();
	elt->alo = u64random();
	elt->b = random()%2 ? (uint64_t)elt->ahi<<32|elt->alo : u64random();
	elt->chi = u64random();
	elt->clo = u64random();
	elt->zf = ((uint64_t)elt->ahi<<32|elt->alo) == elt->b;
	elt->edx = elt->zf ? elt->ahi : elt->b>>32;
	elt->eax = elt->zf ? elt->alo : elt->b;
	elt->res_m = elt->zf ? (uint64_t)elt->chi<<32|elt->clo : elt->b;
}

static void init_cmpxchg16b(struct cmpxchg16b * const elt)
{
	elt->ahi = u64random();
	elt->alo = u64random();
	if (random()%2) {
		elt->bhi = u64random();
		elt->blo = u64random();
	} else {
		elt->bhi = elt->ahi;
		elt->blo = elt->alo;
	}
	elt->chi = u64random();
	elt->clo = u64random();
	elt->zf = elt->ahi == elt->bhi && elt->alo == elt->blo;
	elt->rdx = elt->zf ? elt->ahi : elt->bhi;
	elt->rax = elt->zf ? elt->alo : elt->blo;
	elt->res_m[0] = elt->zf ? elt->clo : elt->blo;
	elt->res_m[1] = elt->zf ? elt->chi : elt->bhi;
}

static int init(void * const config, void * const table, const size_t table_size)
{
	size_t i;
	struct elt * const elts = table;
	struct config * const cfg = config;

	init_config(cfg);

	srandom(time(NULL));

	for(i=0 ; i<table_size ; i++) {
		init_cmpxchg(&elts[i].cmpxchg);

		if (cfg->cmpxchg8b)
			init_cmpxchg8b(&elts[i].cmpxchg8b);

		if (cfg->cmpxchg16b)
			init_cmpxchg16b(&elts[i].cmpxchg16b);
	}

	return 0;
}

static int comp_cmpxchg(struct comp_cmpxchg * const c, struct cmpxchg const * const elt)
{
	c->rax = elt->a;
	c->m = elt->b;

	asm("cmpxchgq %[c], %[b] \n\t"
		"setzb %[zf] \n\t"
		: "+a" (c->rax), [b] "+rm" (c->m),
		  [zf] "=rm" (c->zf)
		: [c] "r" (elt->c)
		: "cc"
	);

	return c->m == elt->res_m
		&& c->rax == elt->res_rax
		&& c->zf == elt->zf;
}

static int comp_cmpxchg8b(struct comp_cmpxchg8b * const c, struct cmpxchg8b const * const elt)
{
	c->edx = elt->ahi;
	c->eax = elt->alo;
	c->m = elt->b;

	asm("cmpxchg8b %[mem] \n\t"
		"setzb %[zf] \n\t"
		: [zf] "=rm" (c->zf), "+d" (c->edx),
		  "+a" (c->eax)
		: [mem] "m" (c->m), "c" (elt->chi),
		  "b" (elt->clo)
		: "cc", "memory"
	);

	return c->m == elt->res_m
			&& c->edx == elt->edx
			&& c->eax == elt->eax
			&& c->zf == elt->zf;
}

static int comp_cmpxchg16b(struct comp_cmpxchg16b * const c, struct cmpxchg16b const * const elt)
{
	c->rdx = elt->ahi;
	c->rax = elt->alo;
	c->m = (uint64_t*) (((uintptr_t)c->mbuf+15)/16*16);
	c->m[0] = elt->blo;
	c->m[1] = elt->bhi;

	asm("cmpxchg16b %[mem] \n\t"
		"setzb %[zf] \n\t"
		: [zf] "=rm" (c->zf), "+d" (c->rdx),
		  "+a" (c->rax)
		: [mem] "o" (*c->m), "c" (elt->chi),
		  "b" (elt->clo)
		: "cc", "memory");

	return c->m[0] == elt->res_m[0] && c->m[1] == elt->res_m[1]
			&& c->rdx == elt->rdx
			&& c->rax == elt->rax
			&& c->zf == elt->zf;
}

static int check_item(void * const comp, void const * const config, void const * const table_element)
{
	struct elt const * const elt = table_element;
	struct comp * const c = comp;
	struct config const * const cfg = config;
	int ok;

	ok = comp_cmpxchg(&c->cmpxchg, &elt->cmpxchg);

	if (cfg->cmpxchg8b)
		ok &= comp_cmpxchg8b(&c->cmpxchg8b, &elt->cmpxchg8b);

	if (cfg->cmpxchg16b)
		ok &= comp_cmpxchg16b(&c->cmpxchg16b, &elt->cmpxchg16b);

	return !ok;
}

static void report_error(FILE *out, void const * const config, void const * const table_element, void const * const comp)
{
	struct elt const * const elt = table_element;
	struct comp const * const c = comp;
	struct config const * const cfg = config;

	fprintf(out, "a=0x%" PRIx64 ", b=0x%" PRIx64 ", c=0x%" PRIx64 "\n", elt->cmpxchg.a, elt->cmpxchg.b, elt->cmpxchg.c);
	fprintf(out, "mem expected=0x%" PRIx64 ", got=0x%" PRIx64 "\n", elt->cmpxchg.res_m, c->cmpxchg.m);
	fprintf(out, "rax expected=0x%" PRIx64 ", got=0x%" PRIx64 "\n", elt->cmpxchg.res_rax, c->cmpxchg.rax);
	fprintf(out, "zf expected=%s, got=%s\n", elt->cmpxchg.zf?"yes":"no", c->cmpxchg.zf?"yes":"no");

	if (cfg->cmpxchg8b) {
		fprintf(out, "cmpxchg8b: ahi=0x%" PRIx32 ", alo=0x%" PRIx32 ", b=0x%" PRIx64 ", chi=0x%" PRIx32 ", clo=0x%" PRIx32 "\n",
					elt->cmpxchg8b.ahi, elt->cmpxchg8b.alo, elt->cmpxchg8b.b, elt->cmpxchg8b.chi, elt->cmpxchg8b.clo);
		fprintf(out, "zf expected=%s, got=%s\n", elt->cmpxchg8b.zf?"yes":"no", c->cmpxchg8b.zf?"yes":"no");
		fprintf(out, "edx expected=0x%" PRIx32 ", got=0x%" PRIx32 "\n", elt->cmpxchg8b.edx, c->cmpxchg8b.edx);
		fprintf(out, "eax expected=0x%" PRIx32 ", got=0x%" PRIx32 "\n", elt->cmpxchg8b.eax, c->cmpxchg8b.eax);
		fprintf(out, "mem expected=0x%" PRIx64 ", got=0x%" PRIx64 "\n", elt->cmpxchg8b.res_m, c->cmpxchg8b.m);
	}

	if (cfg->cmpxchg16b) {
		fprintf(out, "cmpxchg16b: ahi=0x%" PRIx64 ", alo=0x%" PRIx64 ", bhi=0x%" PRIx64 ", blo=0x%" PRIx64 ", chi=0x%"
					PRIx64 ", clo=0x%" PRIx64 "\n",
					elt->cmpxchg16b.ahi, elt->cmpxchg16b.alo, elt->cmpxchg16b.bhi, elt->cmpxchg16b.blo,
					elt->cmpxchg16b.chi, elt->cmpxchg16b.clo);
		fprintf(out, "zf expected=%s, got=%s\n", elt->cmpxchg16b.zf?"yes":"no", c->cmpxchg16b.zf?"yes":"no");
		fprintf(out, "rdx expected=0x%" PRIx64 ", got=0x%" PRIx64 "\n", elt->cmpxchg16b.rdx, c->cmpxchg16b.rdx);
		fprintf(out, "rax expected=0x%" PRIx64 ", got=0x%" PRIx64 "\n", elt->cmpxchg16b.rax, c->cmpxchg16b.rax);
		fprintf(out, "mem expected[0]=0x%" PRIx64 ", expected[1]=0x%" PRIx64 ", got[0]=0x%" PRIx64 ", got[1]=0x%" PRIx64 "\n",
					elt->cmpxchg16b.res_m[0], elt->cmpxchg16b.res_m[1], c->cmpxchg16b.m[0], c->cmpxchg16b.m[1]);
	}
}

CPUCHECK_CHECKER(cmpxchg, "Performs comparisons and moves using cmpxchg, cmpxchg8b, cmpxchg16b", sizeof(struct config), sizeof(struct elt), sizeof(struct comp), init, check_item, report_error, NULL)

#endif	/* ARCH_X86_64 */
