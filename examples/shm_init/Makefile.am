LDADD = -lipctools -lrt
AM_LDFLAGS = -L$(top_builddir)/src -pthread
AM_CFLAGS = -I$(top_srcdir)/src -pthread
bin_PROGRAMS = shm_init alloc_stats sq_stats
shm_init_SOURCES = shm_init.c
alloc_stats_SOURCES = alloc_stats.c
sq_stats_SOURCES = sq_stats.c
