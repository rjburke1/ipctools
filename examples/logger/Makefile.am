LDADD = -lipctools -lrt 
AM_LDFLAGS = -L$(top_builddir)/src -pthread
AM_CFLAGS = -I$(top_srcdir)/src
bin_PROGRAMS = logger enqueue_logmsg dequeue_logmsg dump_logs view_logs
logger_SOURCES = main.c handler.c
enqueue_logmsg_SOURCES = enqueue_logmsg.c 
dequeue_logmsg_SOURCES = dequeue_logmsg.c 
dump_logs_SOURCES = dump_logs.c 
view_logs_SOURCES = view_logs.c 
