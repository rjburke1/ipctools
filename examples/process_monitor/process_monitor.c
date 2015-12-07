#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include "allocator_shm.h"
#include "event_handler.h"
#include "process_monitor.h"
#include "reactor.h"
#include "config.h"

#define TEST_ALLOCATOR_SHM_KEY (5000)
char *name;
ipt_process_monitor_t *pm_ptr;

struct handler
{
	ipt_event_handler_t eh;
};

static void
print_in_use_entry(const process_monitor_entry_t *const entry, const void *data)
{
	if ( !entry->in_use ) return;

        struct timespec tp;
        clock_gettime(CLOCK_MONOTONIC,&tp);

	printf("Process Monitor Entry[\n");
	printf("\tname=%s\n",entry->name);
	printf("\tpid=%i\n",entry->pid);
	printf("\tupdate_time=%d\n",entry->update_time);
	printf("\texpire_interval=%d\n",entry->expire_interval);
	printf("\ttotal time since last update %ld\n", tp.tv_sec - entry->update_time);
	printf("]\n");
}

static int
handle_timeout(ipt_event_handler_t *this, const ipt_time_value_t *curr_time, const void *act)
{
	ipt_process_monitor_for_each( pm_ptr, print_in_use_entry, act);

	return 0;
}
static void
help(void)
{
        fprintf(stderr,"usage : process_monitor -n [name]\n");
        fprintf(stderr,"name  : Name of the logger registered with the allocator.\n");
}
static int
parse_and_init_args(int argc, char *argv[])
{
        int c;
        while ((c = getopt (argc, argv, "n:?")) != -1)
         {
                switch (c)
                {
                        case 'n':
                                name = optarg;
                        break;

                        case '?':
                                help();
                                exit(1);
                        break;
                }
        }

        if ( !name ) return 1;

        return 0;
}

int main(int argc, char *argv[])
{

	struct handler handler;

	handler.eh.handle_timeout = handle_timeout;

	ipt_allocator_t *alloc_ptr;

	if ( parse_and_init_args(argc,argv) )
	{
	   help();
	   return 1;
	}
	alloc_ptr = (ipt_allocator_t *)ipt_allocator_shm_attach(IPT_TEST_ALLOCATOR_SHM_KEY);

	if ( alloc_ptr == NULL )
	{
		return -1;
	}

	/* Process Montior was created by the shm_init utility */
	pm_ptr = ipt_process_monitor_attach(name,alloc_ptr);

	if ( pm_ptr == NULL )
	{
		printf("failed to attach\n");
		return -1;
	}

	ipt_reactor_t *reactor = ipt_reactor_create();

	if ( reactor == NULL )
	{
		return -1;
	}

	ipt_time_value_t tv = { 1, 0 };

	if ( reactor->schedule_timer(reactor,(ipt_event_handler_t *) &handler, NULL, &tv, NULL ) < 0 )
	{
		return -1;
	}

	while ( 1 )
	{
		reactor->run_event_loop(reactor,&tv);
	}

	return 0;
}
