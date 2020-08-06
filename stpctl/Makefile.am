INCLUDES = -I $(top_srcdir) -I ../include

bin_PROGRAMS = stpctl

if DEBUG
DBGFLAGS = -ggdb -DDEBUG
else
DBGFLAGS = -g -DNDEBUG
endif

stpctl_SOURCES = stpctl.c

