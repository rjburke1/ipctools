AM_LDFLAGS = -L$(top_builddir)/src -pthread 
AM_CFLAGS = -I$(top_srcdir)/src -pthread 
lib_LTLIBRARIES = libipctools.la
libipctools_la_SOURCES = reactor.c allocator_malloc.c allocator_shm.c logger.c process_monitor.c support shared_in_list.c shared_queue.c support.c support.h offset_ptr.h acceptor_handler.c
include_HEADERS=reactor.h allocator.h allocator_shm.h shared_queue.h shared_in_list.h allocator_malloc.h event_handler.h process_monitor.h logger.h offset_ptr.h acceptor_handler.h
