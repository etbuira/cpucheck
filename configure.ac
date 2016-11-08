AC_INIT([cpucheck], [0.1])
AC_PREREQ([2.69])

AC_CONFIG_SRCDIR([src/cpucheck.c])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([acm4])
AC_CONFIG_HEADERS([src/config.h])

AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])

AC_PROG_CC
AC_PROG_CC_C99
if test x$ac_cv_prog_cc_c99 = xno; then
	AC_MSG_ERROR([No C99 compiler found])
fi
AC_CHECK_PROG([UNAME], [uname], [yes], [no])
AM_PROG_CC_C_O

AC_SEARCH_LIBS([pthread_create], [pthread], [], [AC_MSG_ERROR([Could not find pthread library])])

AC_HEADER_STDC
AH_TEMPLATE([_GNU_SOURCE])
AC_CHECK_HEADERS([sched.h], [AC_DEFINE([_GNU_SOURCE])])

AC_TYPE_SIZE_T

AC_FUNC_MALLOC
AC_CHECK_FUNCS([sched_getaffinity])

AH_TEMPLATE([ARCH_X86_64])
if test x$UNAME = xyes; then
	if test x$(uname -m) = xx86_64; then
		AC_DEFINE([ARCH_X86_64])
	fi
fi

AH_TEMPLATE([RANDOM_NIBLE_SIZE])
CPUCHECK_RANDOM_NIBLE_SIZE

AC_CONFIG_FILES([Makefile])

AC_OUTPUT

