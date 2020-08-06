SUBDIRS = include lib stp stpctl

INCLUDES = -I $(top_srcdir) -I ./include 

bin_PROGRAMS = stpd

if DEBUG
DBGFLAGS = -ggdb -DDEBUG
else
DBGFLAGS = -g -DNDEBUG
endif

stpd_SOURCES = stpd_main.cpp stpsync/stp_sync.cpp

stpd_CPPFLAGS = $(DBGFLAGS) $(AM_CFLAGS) $(CFLAGS_COMMON) $(COV_CFLAGS)
stpd_LDADD = -lhiredis -lswsscommon \
			 $(LIBDAEMON_LIBS) \
	         lib/libcommonstp.a \
	         stp/libstp.a \
	         lib/libcommonstp.a \
	         /usr/lib/x86_64-linux-gnu/libevent.a \
			 $(COV_LDFLAGS)

