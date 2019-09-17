INCLUDES = -I $(top_srcdir) -I ../include

noinst_LIBRARIES = libcommonstp.a

if DEBUG
DBGFLAGS = -ggdb -DDEBUG
else
DBGFLAGS = -g -DNDEBUG
endif

libcommonstp_a_CFLAGS = -D_GNU_SOURCE $(COV_CFLAGS)
libcommonstp_a_SOURCES = avl.c bitmap.c applog.c
