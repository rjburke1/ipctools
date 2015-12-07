#include <stdio.h>
#include <stdlib.h>

#include "allocator_shm.h"
#include "event_handler.h"
#include "process_monitor.h"
#include "reactor.h"
#include "config.h"

#define TEST_ALLOCATOR_SHM_KEY (5000)

char *name;
char *pname;
int update_time = 1;

ipt_process_monitor_t *pm_ptr;

struct handler
{
	ipt_event_handler_t eh;
};

static int
handle_timeout(ipt_event_handler_t *this, const ipt_time_value_t *curr_time, const void *act)
{
static int count = 0;

	printf("handl_timeout: update timer\n");

	((ipt_process_monitor_t*)act)->still_alive((ipt_process_monitor_t*)act,pname);

	return 0;
}
static void
help(void)
{
        fprintf(stderr,"usage : process_monitored -n [name] -p [pname] -t [utime]\n");
        fprintf(stderr,"name    : Name of the process manager registered with the allocator.\n");
        fprintf(stderr,"pname   : Name of the process manager registered with the allocator.\n");
        fprintf(stderr,"utime   : The interval that the process updates its activity timer.\n");
}
static int
parse_and_init_args(int argc,  char * const argv[])
{
        int c;
        while ((c = getopt (argc, argv, "n:p:t:?")) != -1)
         {
                switch (c)
                {
                        case 'n':
                                name = optarg;
                        break;

                        case 'p':
                                pname = optarg;
                        break;

                        case 't':
                                update_time = atol(optarg);
                        break;

                        case '?':
                                help();
                                exit(1);
                        break;
                }
        }

        if ( !name || !pname) return 1;

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

	pm_ptr = ipt_process_monitor_attach(name,alloc_ptr);

	if ( pm_ptr == NULL )
	{
		printf("failed to attach\n");
		return -1;
	}

	/* register process */
	pm_ptr->register_process(pm_ptr,pname,"path", (const char * const *  )argv);

	ipt_reactor_t *reactor = ipt_reactor_create();

	if ( reactor == NULL )
	{
		return -1;
	}

	ipt_time_value_t tv = { update_time, 0 };

	if ( reactor->schedule_timer(reactor,(ipt_event_handler_t *) &handler, NULL, &tv, pm_ptr ) < 0 )
	{
		return -1;
	}

	while ( 1 )
	{
		reactor->run_event_loop(reactor,&tv);
	}

	return 0;
}
