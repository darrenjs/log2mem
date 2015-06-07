


# NOTE: it is not good practice to add "-Wall" and "-03" etc here.  Those
# choices should be made by the user when they invoke the configure script.
AM_CPPFLAGS = -I$(top_srcdir)/liblog2mem -Wall -pthread

# NOTE: -lrt added for posix clock support
AM_LDFLAGS=-L../liblog2mem -llog2mem -lrt -pthread

noinst_PROGRAMS=example
#noinst_PROGRAMS=server_demo


example_SOURCES=example.c

