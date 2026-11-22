INCLUDES = -I $(top_srcdir) -I ../include

noinst_LIBRARIES = libcommonstp.a

if DEBUG
DBGFLAGS = -ggdb -DDEBUG
else
DBGFLAGS = -g -DNDEBUG
endif

libcommonstp_a_CFLAGS = -D_GNU_SOURCE $(COV_CFLAGS)
libcommonstp_a_SOURCES = avl.c bitmap.c applog.c vlan_util.c

check_PROGRAMS = bitmap_bounds_test
TESTS = $(check_PROGRAMS)
bitmap_bounds_test_SOURCES = bitmap_bounds_test.c bitmap.c applog.c
