tc_srcs = BinaryOp.cc ToString.cc

bin_PROGRAMS = tc
tc_SOURCES = tc.cc $(tc_srcs)

check_PROGRAMS = tc_test
tc_test_SOURCES = tc_test.cc $(tc_srcs)
tc_test_SOURCES += ToStringTest.cc

TESTS = $(check_PROGRAMS)


