SUBDIRS = include lib stpctl

INCLUDES = -I $(top_srcdir) -I ./include -I lib 

noinst_LIBRARIES = libstp.a
bin_PROGRAMS = stpd

if DEBUG
DBGFLAGS = -ggdb -DDEBUG
else
DBGFLAGS = -g -DNDEBUG
endif

libstp_a_CFLAGS = -Werror -Wno-error=address-of-packed-member $(COV_CFLAGS)
libstp_a_SOURCES = stp/stp.c stp/stp_pkt.c stp/stp_data.c stp/stp_debug.c stp/stp_intf.c stp/stp_main.c \
				   stp/stp_mgr.c stp/stp_netlink.c stp/stp_timer.c stp/stp_util.c \
                   mstp/mstp_data.c mstp/mstp_lib.c mstp/mstp_debug.c mstp/mstp_util.c mstp/mstp_mgr.c \
				   mstp/mstp_pim.c mstp/mstp_ppm.c mstp/mstp_prs.c mstp/mstp_prt.c mstp/mstp_prx.c \
				   mstp/mstp_ptx.c mstp/mstp_pst.c mstp/mstp_tcm.c mstp/mstp.c
stpd_SOURCES = stpd_main.cpp stpsync/stp_sync.cpp

stpd_CPPFLAGS = $(DBGFLAGS) $(AM_CFLAGS) $(CFLAGS_COMMON) $(COV_CFLAGS)
stpd_LDADD = -lhiredis -lswsscommon -lcrypto \
			 $(LIBDAEMON_LIBS) \
	         lib/libcommonstp.a \
	         libstp.a \
	         lib/libcommonstp.a \
			 -lcrypto \
	         -levent \
			 $(COV_LDFLAGS)

# Unit tests. check_PROGRAMS is only built by `make check', so a normal
# build is unaffected; dh_auto_test picks them up on a package build.
# They link the real stp/mstp objects against an in-memory model of
# APPL_DB instead of stpsync/stp_sync.cpp.
if GTEST
check_PROGRAMS = stp_tests
TESTS = stp_tests

stp_tests_SOURCES = tests/stpd_driver.c tests/fake_kernel.c \
                    tests/fake_appdb.cpp tests/test_bridge_disable.cpp \
                    tests/test_bpdu_tagging.cpp tests/test_bpdu_rx.cpp \
                    tests/test_kernel_vlan.cpp tests/test_path_cost.cpp

stp_tests_CPPFLAGS = -I $(top_srcdir) -I ./include -I lib -I tests
stp_tests_CFLAGS = -Wall -Wno-error=address-of-packed-member
stp_tests_CXXFLAGS = -std=c++17 -Wall

# tests/fake_kernel.c stands in for both of these.
stp_tests_LDFLAGS = -Wl,--wrap=system -Wl,--wrap=stp_pkt_sock_create

stp_tests_LDADD = libstp.a \
                  lib/libcommonstp.a \
                  -levent -lcrypto -lgtest -lgtest_main -lpthread
endif
