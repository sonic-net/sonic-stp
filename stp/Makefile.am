INCLUDES = -I $(top_srcdir) -I ../include
INCLUDES += -I ../lib

noinst_LIBRARIES = libstp.a

if DEBUG
DBGFLAGS = -ggdb -DDEBUG
else
DBGFLAGS = -g -DNDEBUG
endif

#libstp_a_CFLAGS = -Werror $(COV_CFLAGS)
libstp_a_CFLAGS =  $(COV_CFLAGS)
libstp_a_SOURCES = stp.c stp_pkt.c stp_data.c stp_debug.c stp_intf.c stp_main.c stp_mgr.c stp_netlink.c stp_timer.c stp_util.c 
