AM_CPPFLAGS = -I$(top_srcdir)/src $(includeopt)
LDADD = -lipctools -lrt
AM_LDFLAGS = -L$(top_builddir)/src -pthread
AM_CFLAGS = -I$(top_srcdir)/src
bin_PROGRAMS = process_monitor process_monitored view_procs
process_monitor_SOURCES = process_monitor.c
view_procs_SOURCES = view_procs.c
process_monitored_SOURCES = process_monitored.c
