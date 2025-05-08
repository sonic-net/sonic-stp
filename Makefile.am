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
				   stp/stp_mgr.c stp/stp_netlink.c stp/stp_timer.c stp/stp_util.c 
				   
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