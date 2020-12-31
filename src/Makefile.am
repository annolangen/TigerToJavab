AUTOMAKE_OPTIONS = subdir-objects
tc_srcs = BinaryOp.cc ToString.cc DebugString.cc types.cc parser.yy scanner.ll driver.cc emit.cc compiler.cc

bin_PROGRAMS = tc
tc_SOURCES = tc.cc $(tc_srcs)

check_PROGRAMS = tc_test
tc_test_SOURCES = tc_test.cc $(tc_srcs) testing/testing.cc
tc_test_SOURCES += DebugStringTest.cc
tc_test_SOURCES += ScopedMapTest.cc
tc_test_SOURCES += ToStringTest.cc
tc_test_SOURCES += emitTest.cc
tc_test_SOURCES += typesTest.cc
tc_test_SOURCES += utilTest.cc
tc_test_SOURCES += compilerTest.cc
TESTS = $(check_PROGRAMS)
