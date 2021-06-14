# To create Makefile run `autoreconf --install --force` followed by
# `./configure`.
AC_INIT([tc], [1.0])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

# Check for C++ compiler
CXXFLAGS="-Werror -std=c++17 -g3 -fsanitize=address -fsanitize=undefined -fno-sanitize=vptr"
CXX=clang++
LD=clang++
AC_PROG_CXX
AC_PROG_LEX
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([ Makefile src/Makefile ])
AC_OUTPUT
