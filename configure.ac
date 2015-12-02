#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.69])
AC_INIT([ipctools], [0.3], [rjburke1@gmail.com])
AM_INIT_AUTOMAKE
LT_INIT
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lipctools':
AC_CHECK_LIB([ipctools], [main])
# FIXME: Replace `main' with a function in `-lrt':
AC_CHECK_LIB([rt], [main])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netinet/in.h stdlib.h string.h strings.h sys/file.h sys/socket.h sys/time.h unistd.h])

AC_CHECK_PROGS([DOXYGEN],[doxygen])
if test -z "$DOCYGEN";
   then AC_MSG_WARN([Doxygen not found - continuing without Doxygen support])
fi

AM_CONDITIONAL([HAVE_DOXYGEN], 
[test -n "$DOXYGEN"]) AM_COND_IF([HAVE_DOXYGEN], 
[AC_CONFIG_FILES([docs/Doxyfile])])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([clock_gettime gethostname gettimeofday memset mkfifo socket])

AC_CONFIG_FILES([Makefile
                 examples/Makefile
                 examples/logger/Makefile
                 examples/process_monitor/Makefile
                 examples/shm_init/Makefile
                 src/Makefile
                 tests/Makefile
		 docs/Makefile])
AC_OUTPUT
