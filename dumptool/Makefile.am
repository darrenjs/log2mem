


# NOTE: it is not good practice to add "-Wall" and "-03" etc here.  Those
# choices should be made by the user when they invoke the configure script.
AM_CPPFLAGS = -I$(top_srcdir)/liblog2mem -Wall

AM_LDFLAGS=

LDADD = -L../liblog2mem -llog2mem -lrt

bin_PROGRAMS=log2mem-dump
#noinst_PROGRAMS=server_demo

# link statically for the dump tool, so that it is easier to copy around
log2mem_dump_LDFLAGS=-static
log2mem_dump_SOURCES=log2memdump.c

# server_dem
#server_demo_SOURCES=server_demo.cc