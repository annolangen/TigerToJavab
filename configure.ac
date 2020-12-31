# To create Makefile run `autoreconf --install --force` followed by
# `./configure`.
AC_INIT([tc], [1.0])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

# Check for C++ compiler
AC_PROG_CXX
CXXFLAGS="-Werror -std=c++17"
AC_PROG_LEX
AC_PROG_YACC
AC_PROG_RANLIB
m4_ifdef([AM_PROG_AR], [AM_PROG_AR], [AC_SUBST([AR], [ar])])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([ Makefile src/Makefile ])
AC_OUTPUT
