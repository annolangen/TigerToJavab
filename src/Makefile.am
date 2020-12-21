tc_srcs = BinaryOp.cc ToString.cc types.cc

bin_PROGRAMS = tc
tc_SOURCES = tc.cc $(tc_srcs)

check_PROGRAMS = tc_test
tc_test_SOURCES = tc_test.cc $(tc_srcs)
tc_test_SOURCES += ToStringTest.cc
tc_test_SOURCES += ScopedMapTest.cc
tc_test_SOURCES += utilTest.cc
tc_test_SOURCES += typesTest.cc

TESTS = $(check_PROGRAMS)


