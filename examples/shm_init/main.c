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

	/*
	 * Set the umask to 0777 so any process can read and write to the queues, etc.
	 */
	umask(0000);

	ipt_allocator_t *alloc_ptr = ipt_allocator_shm_create(1024 *1024, IPT_TEST_ALLOCATOR_SHM_KEY);

	if ( alloc_ptr == NULL )
	{
		printf("Failed to allocate the shared allocator for the service engine\n");
		return -1;
	}

	ipt_logger_t *lg_ptr = ipt_logger_create("ipt_logger_t", alloc_ptr);

	if ( lg_ptr == NULL )
	{
		printf("Failed to create the logger.\n");
		return -1;
	}

	return 0;
}

