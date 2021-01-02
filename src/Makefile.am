AUTOMAKE_OPTIONS = subdir-objects
tc_srcs = BinaryOp.cc Expression.cc ToString.cc DebugString.cc types.cc parser.yy scanner.ll driver.cc emit.cc compiler.cc

bin_PROGRAMS = tc
tc_SOURCES = tc.cc $(tc_srcs)

check_PROGRAMS = tc_test
noinst_LIBRARIES = libtest.a
libtest_a_SOURCES = DebugStringTest.cc
libtest_a_SOURCES += ScopedMapTest.cc
libtest_a_SOURCES += NameSpaceTest.cc
libtest_a_SOURCES += ToStringTest.cc
libtest_a_SOURCES += emitTest.cc
libtest_a_SOURCES += typesTest.cc
libtest_a_SOURCES += utilTest.cc
libtest_a_SOURCES += compilerTest.cc

tc_test_SOURCES = tc_test.cc $(tc_srcs) testing/testing.cc
tc_test_LDADD = libtest.a

TESTS = $(check_PROGRAMS)
