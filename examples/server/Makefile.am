AM_CPPFLAGS = -I$(top_srcdir)/src $(includeopt)
LDADD = -lipctools -lrt
AM_LDFLAGS = -L$(top_builddir)/src -pthread
AM_CFLAGS = -I$(top_srcdir)/src
bin_PROGRAMS = server client
server_SOURCES = server.c handler.c
client_SOURCES = client.c 
