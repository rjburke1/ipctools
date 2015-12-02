#include <stdlib.h>
#include <stdio.h>
#include "allocator_shm.h"
#include "shared_queue.h"
#include "process_monitor.h"
#include "logger.h"
#include "config.h"
#include <semaphore.h>
struct shared_data
{
sem_t sem;
};

int main ( int argc, char *argv[])
{

	ipt_allocator_t *alloc_ptr = ipt_allocator_shm_attach(IPT_TEST_ALLOCATOR_SHM_KEY);

	if ( alloc_ptr == NULL )
	{
		printf("Failed to attache the shared allocator.\n");
		return -1;
	}

	alloc_ptr->dump_stats(alloc_ptr);

	return 0;
}

