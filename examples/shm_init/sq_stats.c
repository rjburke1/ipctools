#include <stdlib.h>
#include <stdio.h>
#include "allocator_shm.h"
#include "shared_queue.h"
#include "process_monitor.h"
#include "logger.h"
#include "config.h"
#include <semaphore.h>


char *name;

help(void)
{
        fprintf(stderr,"usage : sq_state -n [name]\n");
        fprintf(stderr,"name    : Name of the shared queue registered with the allocator.\n");
}
static void
dump_entry(const ipt_shared_queue_node_t *const n_ptr, void *in_ptr)
{
	/* I know this is a logger message */
	ipt_logger_dump_message(n_ptr);	
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

int main ( int argc, char *argv[])
{
ipt_shared_queue_t *sq_ptr;

	if ( parse_and_init_args(argc,argv) )
	{
		printf("failed to parse arguments\n");
		return 1;
	}

 	ipt_allocator_t *alloc_ptr = ipt_allocator_shm_attach(IPT_TEST_ALLOCATOR_SHM_KEY);	

	if ( !(sq_ptr = ipt_shared_queue_attach(name, alloc_ptr)) )
	{
		printf("Failed to find the shared queue.\n");
		return 1;
	}
	printf("------------------------ ipt_logger_t.sq stats --------------------\n");
	sq_ptr->dump_stats(sq_ptr);

	return 0;
}

