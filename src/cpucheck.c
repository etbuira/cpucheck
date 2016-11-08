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
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <limits.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#if HAVE_SCHED_H
#include <sched.h>
#endif
#include "cpucheck.h"

#define min(a, b) ((a)<(b)?(a):(b))

static volatile int *should_stop;

static void shouldstop_sig_handler(int signum)
{
	if (should_stop)
		*should_stop = 1;
}

extern struct cpucheck_checker cpucheck_checker_addsub;
#if ARCH_X86_64
extern struct cpucheck_checker cpucheck_checker_bitscan;
extern struct cpucheck_checker cpucheck_checker_bittest;
#endif
extern struct cpucheck_checker cpucheck_checker_bool;
extern struct cpucheck_checker cpucheck_checker_muldiv;
#if ARCH_X86_64
extern struct cpucheck_checker cpucheck_checker_signextend;
#endif

static struct cpucheck_checker const * const checkers[] = {
	&cpucheck_checker_addsub,
#if ARCH_X86_64
	&cpucheck_checker_bitscan,
	&cpucheck_checker_bittest,
#endif
	&cpucheck_checker_bool,
	&cpucheck_checker_muldiv,
#if ARCH_X86_64
	&cpucheck_checker_signextend,
#endif
	NULL
};

#define NIBLE_COUNT(tofill, niblesz) ( (tofill)/(niblesz) + !!((tofill)%(niblesz)) )

unsigned long int ulirandom(void)
{
	size_t i;
	unsigned long int res;

	for(i=res=0 ; i<NIBLE_COUNT(sizeof(unsigned long int)*8, RANDOM_NIBLE_SIZE) ; i++)
		res |= random() << (i*RANDOM_NIBLE_SIZE);

	return res;
}

uint64_t u64random(void)
{
	size_t i;
	uint64_t res;

	for(i=res=0 ; i<NIBLE_COUNT(sizeof(uint64_t)*8, RANDOM_NIBLE_SIZE) ; i++)
		res |= (uint64_t) random() << (i*RANDOM_NIBLE_SIZE);

	return res;
}

struct args {
	unsigned long table_size;
	unsigned int nb_threads;
	struct cpucheck_checker const * checker;
};

static unsigned int get_core_count(void)
{
#if HAVE_SCHED_GETAFFINITY
	cpu_set_t cs;
	memset(&cs, 0, sizeof(cs));
	if (sched_getaffinity(0, sizeof(cs), &cs))
		return 1;
	return CPU_COUNT(&cs);
#else
	return 1;
#endif
}

static void args_init(struct args * const args)
{
	args->table_size = 65535;
	args->nb_threads = get_core_count();
	args->checker = checkers[0];
}

struct thread_state {
	pthread_t thread;
	struct state * state;
	size_t start_idx;
	void *comp;
	unsigned long int inconsistencies;
	unsigned long int checks;
};

struct state {
	size_t table_size;
	void * table;
	struct cpucheck_checker const * const checker;
	struct thread_state *threads;
	volatile int should_exit;
	pthread_mutex_t output;
};

static void * thread_func(void *arg)
{
	struct thread_state * const thrd = arg;
	size_t idx;
	unsigned short int start;

	for (start=1 ; !thrd->state->should_exit ; start=0) {
		for (idx = start ? thrd->start_idx : 0 ; idx<thrd->state->table_size && !thrd->state->should_exit ; idx++) {
			if (thrd->state->checker->check_item(thrd->comp, thrd->state->table, idx)) {
				pthread_mutex_lock(&thrd->state->output);
				fprintf(stderr, "Inconsistency detected...\n");
				if (thrd->state->checker->report_error)
					thrd->state->checker->report_error(stderr, thrd->state->table, idx, thrd->comp);
				pthread_mutex_unlock(&thrd->state->output);
				if (ULONG_MAX-thrd->inconsistencies)
					thrd->inconsistencies++;
			}
			if (thrd->checks < ULONG_MAX)
				thrd->checks++;
		}
	}

	return NULL;
}

static int init_state(struct state *state, struct args const * const args)
{
	unsigned int tno;

	if (SIZE_MAX/args->checker->table_elt_size < args->table_size) {
		fprintf(stderr, "Requested table size is too big\n");
		return -1;
	}
	if (SIZE_MAX/sizeof(*state->threads) < args->nb_threads) {
		fprintf(stderr, "Requested thread count is too big\n");
		return -1;
	}

	state->table = malloc(args->table_size*args->checker->table_elt_size);
	if (!state->table) {
		fprintf(stderr, "Could not allocate table\n");
		return -1;
	}
	state->table_size = args->table_size;

	if (args->checker->init_table(state->table, args->table_size)) {
		fprintf(stderr, "Error while initialising table\n");
		goto err_table;
	}

	state->threads = malloc(sizeof(*state->threads) * args->nb_threads);
	if (!state->threads) {
		fprintf(stderr, "Could not allocate threads states\n");
		goto err_table;
	}

	for (tno=0 ; tno<args->nb_threads ; tno++) {
		state->threads[tno].state = state;
		state->threads[tno].start_idx = random()%args->table_size;
		state->threads[tno].comp = malloc(args->checker->comp_elt_size);
		if (!state->threads[tno].comp) {
			unsigned int tno2;

			for (tno2=0 ; tno2<tno ; tno++)
				free(state->threads[tno2].comp);
			goto err_threads;
		}
		state->threads[tno].inconsistencies = 0;
		state->threads[tno].checks = 0;
	}

	if (pthread_mutex_init(&state->output, NULL)) {
		fprintf(stderr, "Could not allocate output mutex\n");
		goto err_threads;
	}

	return 0;

err_threads:
	free(state->threads);
err_table:
	if (args->checker->delete_table)
		args->checker->delete_table(state->table, args->table_size);
	free(state->table);
	return -1;
}

static int run(struct args const * const args)
{
	struct state state = { .checker = args->checker, .should_exit = 0 };
	size_t tno;
	unsigned long int inc_cnt, check_cnt;
	struct sigaction sa;
	int r;

	if (init_state(&state, args))
		return -1;

	should_stop = &state.should_exit;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = shouldstop_sig_handler;
	sigaction(SIGINT, &sa, NULL);

	for (tno=0 ; tno < args->nb_threads ; tno++) {
		if (pthread_create(&state.threads[tno].thread, NULL, thread_func, &state.threads[tno])) {
			size_t tnob;
			pthread_mutex_lock(&state.output);
			fprintf(stderr, "Issue when spawning thread\n");
			pthread_mutex_unlock(&state.output);
			state.should_exit = 1;
			for (tnob=0 ; tnob < tno ; tnob++) {
				pthread_join(state.threads[tno].thread, NULL);
			}
			r = -1;
			goto err_mutex;
		}
	}

	for (inc_cnt=0, check_cnt=0, tno=0 ; tno<args->nb_threads ; tno++) {
		pthread_join(state.threads[tno].thread, NULL);
		inc_cnt += min(ULONG_MAX-inc_cnt, state.threads[tno].inconsistencies);
		check_cnt += min(ULONG_MAX-check_cnt, state.threads[tno].checks);
	}

	fprintf(stdout, "Detected %s%lu inconsistencies over %s%lu tests\n",
			inc_cnt==ULONG_MAX?"possibly more than ":"", inc_cnt,
			check_cnt==ULONG_MAX?"possibly more than ":"", check_cnt);
	r = 0;

err_mutex:
	pthread_mutex_destroy(&state.output);
	should_stop = NULL;
	for (tno=0 ; tno<args->nb_threads ; tno++)
		free(state.threads[tno].comp);
	free(state.threads);
	if (args->checker->delete_table)
		args->checker->delete_table(state.table, args->table_size);
	free(state.table);

	return r;
}

static void print_usage(char const * const progname, struct args const * const args)
{
	struct cpucheck_checker const * const * tmpcheck;

	fprintf(stderr, "Usage: %s [-c <checker>] [-s <tableSize>] [-t <nbThreads>]\n", progname);
	fprintf(stderr, "\n");
	fprintf(stderr, "\t-c checker: Sets the checker to use (see below for list) [%s]\n", args->checker->name);
	fprintf(stderr, "\t-s tableSize: Sets the table size to tableSize elements [%lu]\n", args->table_size);
	fprintf(stderr, "\t-t nbThreads: Sets the number of checker threads [%u]\n", args->nb_threads);
	fprintf(stderr, "\n");
	fprintf(stderr, "Checkers:\n");
	for (tmpcheck = checkers ; *tmpcheck ; tmpcheck++)
		fprintf(stderr, "\t%s: %s\n", (*tmpcheck)->name, (*tmpcheck)->description);
}

static int parse_args(struct args * const args, int argc, char *argv[])
{
	int opt;
	char const * const progname = argv[0];
	unsigned long tmpul;
	char *tmpcp;
	struct cpucheck_checker const * const * tmpcheck;

	while ((opt = getopt(argc, argv, "c:hs:t:")) != -1) {
		switch(opt) {
			case 'c':
				for (tmpcheck = checkers ; *tmpcheck && strcmp(optarg, (*tmpcheck)->name) ; tmpcheck++) ;
				if (!tmpcheck) {
					fprintf(stderr, "Checker %s not found\n", optarg);
					return -1;
				}
				args->checker = *tmpcheck;
				break;
			case 'h':
			case '?':
			case ':':
				print_usage(progname, args);
				return -1;
			case 's':
				errno = 0;
				tmpul = strtoul(optarg, &tmpcp, 0);
				if (errno || *tmpcp) {
					fprintf(stderr, "Could not parse %s as integer\n", optarg);
					return -1;
				}
				if (!tmpul) {
					fprintf(stderr, "Needs a non-null table size\n");
					return -1;
				}
				args->table_size = tmpul;
				break;
			case 't':
				errno = 0;
				tmpul = strtoul(optarg, &tmpcp, 0);
				if (errno || *tmpcp || tmpul > UINT_MAX) {
					fprintf(stderr, "Could not parse %s as integer\n", optarg);
					return -1;
				}
				args->nb_threads = tmpul;
				break;
			default:
				fprintf(stderr, "Looks like '%c' is unhandled\n", opt);
				return -1;
		}
	}

	return 0;
}

int main(int argc, char *argv[])
{
	struct args args;

	args_init(&args);

	if (parse_args(&args, argc, argv))
		return EXIT_FAILURE;

	srandom(time(NULL));

	if (run(&args))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}
