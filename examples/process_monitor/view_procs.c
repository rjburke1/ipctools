#include <stdio.h>
#include <stdlib.h>

#include "allocator_shm.h"
#include "event_handler.h"
#include "process_monitor.h"
#include "reactor.h"
#include "config.h"

#define TEST_ALLOCATOR_SHM_KEY (5000)
char *name;

ipt_process_monitor_t *pm_ptr;

static void
print_in_use_entry(const process_monitor_entry_t *const entry, const void *data)
{
	if ( !entry->in_use ) return;

	printf("Process Monitor Entry[\n");
	printf("\tname=%s\n",entry->name);
	printf("\tpid=%i\n",entry->pid);
	printf("\tupdate_time=%d\n",entry->update_time);
	printf("\texpire_interval=%d\n",entry->expire_interval);

}

static void
help(void)
{
        fprintf(stderr,"usage : vew_procs -n [name]\n");
        fprintf(stderr,"name    : Name of the process manager registered with the allocator.\n");
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
void *act;

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

        ipt_process_monitor_for_each( pm_ptr, print_in_use_entry, act);

	return 0;
}
