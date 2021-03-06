#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <wait.h>

#include "logger.h"
#include "allocator_shm.h"
#include "config.h"
#include "reactor.h"
#include "shared_queue.h"
#include "shared_in_list.h"

ipt_logger_t *lg_ptr;
ipt_allocator_t *alloc_ptr;

#define NUMBER_OF_MESSAGES (10000)

struct my_message
{
	ipt_shared_queue_node_t node;
	char buf[256];
};

struct my_handler
{
	ipt_event_handler_t eh;
};

static void 
test_1()
{
	if ( fork() == 0 )	
   	{
      		ipt_time_value_t tv = {1,0};
      		int count=0;

		sleep ( 2 );
      		while (1) 
      		{

			ipt_time_value_t tv = { 1, 0 };

			void *ptr = (struct my_message *)lg_ptr->dequeue_timed(lg_ptr,&tv);

			if ( ptr == NULL )
			{
				break;
			}

			alloc_ptr->free(alloc_ptr, ptr);

			count ++;
      		}
      		if (  count == NUMBER_OF_MESSAGES )
		{
			exit ( 0 );
		}
		else
		{
			exit ( 1 );
		}
	}
	else
	{
		int i =0;
      		int status = 0;

      		for (i=0; i < NUMBER_OF_MESSAGES; i++)
      		{	

			lg_ptr->enqueue(lg_ptr, IPT_MODULE_ALL,IPT_LEVEL_ALL, "parent process", "%s", "test message from parent.");

		}

      		wait(&status);

      		assert( WEXITSTATUS(status) == 0 );
   	}
}
int handle_input(ipt_event_handler_t *eh_ptr, ipt_handle_t h)
{

	static int count = 0;

	ipt_logger_message_t *msg_ptr = lg_ptr->dequeue(lg_ptr);

	/* Catch if something went wrong and message is not dequeued */
	if ( msg_ptr == NULL )
	{
		return 0;
	}

	if ( count++ == NUMBER_OF_MESSAGES - 1)
	{
		exit ( 0 );
	}

	return 0;
}

int set_handle(ipt_event_handler_t *eh_ptr, ipt_handle_t h)
{
	eh_ptr->_h = h;
}

ipt_handle_t get_handle(ipt_event_handler_t *eh_ptr)
{
        return eh_ptr->_h;
}

static void 
test_2()
{
	if ( fork() == 0 )	
   	{
		struct my_handler handler;

		handler.eh.handle_input = handle_input;
		handler.eh.get_handle = get_handle;
		handler.eh.set_handle = set_handle;

		handler.eh.set_handle(&handler.eh, lg_ptr->get_fd(lg_ptr) );

		ipt_reactor_t *reactor = ipt_reactor_create();

		if ( reactor->register_handler( reactor, &handler.eh, EVENT_HANDLER_READ_MASK)  < 0 )
		{
			exit ( 1 );
		}

      		ipt_time_value_t tv = {10,0};

      		while (1) 
      		{
			reactor->run_event_loop(reactor,&tv);
		}
	}
	else
	{
		int i =0;
      		int status = 0;

      		for (i=0; i < NUMBER_OF_MESSAGES; i++)
      		{	
			lg_ptr->enqueue(lg_ptr, IPT_MODULE_ALL,IPT_LEVEL_ALL, "parent process", "%s", "test message from parent."); 

		}

                //printf(" waiting on the chiled\n");fflush(stdout);
      		wait(&status);

      		assert( WEXITSTATUS(status) == 0 );
   	}
}

int main(int argc , char *argv[])
{
	alloc_ptr = ipt_allocator_shm_create(10*1024*1024, IPT_TEST_ALLOCATOR_SHM_KEY);

	if ( alloc_ptr == NULL )
	{
		printf("Failed to create shared memory.\n");
		return -1;
	}

	lg_ptr = ipt_logger_create("logger_queue",alloc_ptr);

	if ( lg_ptr == NULL )
	{
		printf("Failed to create logger.\n");
		return -1;
	}

	lg_ptr->set_category(lg_ptr, IPT_MODULE_ALL);
	lg_ptr->set_level(lg_ptr, IPT_LEVEL_ALL);

   	test_1();

	//test_2();

	alloc_ptr->destroy(alloc_ptr);	

	printf("%s completed successfully.\n",argv[0]);

	return 0;
}
