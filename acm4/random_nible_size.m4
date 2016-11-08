AC_DEFUN([CPUCHECK_RANDOM_NIBLE_SIZE], [
	AC_MSG_CHECKING([random() nible size])
	AC_LANG_PUSH([C])
	AC_RUN_IFELSE([AC_LANG_PROGRAM([#include <stdio.h>
									#include <stdlib.h>

									size_t slog2(long int subject)
									{
										size_t i;

										for(i=sizeof(long int)*8 ; i ; i--)
											if (subject & (1ULL << (i-1)))
												return i;

										return 0;
									}], [
										FILE *f;

										f = fopen("conftest.out", "w");
										if (!f) return EXIT_FAILURE;

										fprintf(f, "%zu\n", slog2(RAND_MAX));
										fclose(f);
									])],
					[cpucheck_random_nible_size=$(cat conftest.out)],
					[AC_MSG_ERROR([Could not figure out random() nible size])])
	if test $cpucheck_random_nible_size -eq 0; then
		AC_MSG_ERROR([RAND_MAX is unproperly defined])
	fi
	AC_DEFINE_UNQUOTED([RANDOM_NIBLE_SIZE], [$cpucheck_random_nible_size])
	AC_LANG_POP([C])
	AC_MSG_RESULT([$cpucheck_random_nible_size])
])

