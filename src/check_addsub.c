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

#include <stdlib.h>
#include <time.h>
#include "cpucheck.h"

struct elt {
	unsigned long int a;
	unsigned long int b;
	unsigned long int c;
	unsigned long int res;
};

static int init_table(void *table, const size_t table_size)
{
	size_t i;
	struct elt * const elts = table;

	srandom(time(NULL));

	for(i=0 ; i<table_size ; i++) {
		elts[i].a = random();
		elts[i].b = random();
		elts[i].c = random();
		elts[i].res = elts[i].a + elts[i].b - elts[i].c;
	}

	return 0;
}

static int check_item(void const * const table, const size_t index)
{
	struct elt const * const elts = table;

	return ! (elts[index].a + elts[index].b - elts[index].c == elts[index].res);
}

CPUCHECK_CHECKER(addsub, "Performs integer addition and substractions", sizeof(struct elt), init_table, check_item, NULL)

