INCLUDES = -I $(top_srcdir) -I include
INCLUDES += -I lib

noinst_LIBRARIES = libstp.a

if DEBUG
DBGFLAGS = -ggdb -DDEBUG
else
DBGFLAGS = -g -DNDEBUG
endif

libstp_a_CFLAGS = -Werror -Wno-error=address-of-packed-member $(COV_CFLAGS)
libstp_a_SOURCES = stp/stp.c stp/stp_pkt.c stp/stp_data.c stp/stp_debug.c stp/stp_intf.c stp/stp_main.c \
				   stp/stp_mgr.c stp/stp_netlink.c stp/stp_timer.c stp/stp_util.c \
                   mstp/mstp.c mstp/mstp_data.c mstp/mstp_debug.c mstp/mstp_lib.c mstp/mstp_pim.c mstp/mstp_ppm.c \
                   mstp/mstp_prs.c mstp/mstp_prt.c mstp/mstp_prx.c  mstp/mstp_pst.c mstp/mstp_ptx.c  mstp/mstp_tcm.c \
                   mstp/mstp_util.c mstp/mstp_mgr.c
