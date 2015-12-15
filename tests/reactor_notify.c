#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "reactor.h"

ipt_reactor_t *reactor;

#define NUMBER_OF_NOTIFICATIONS (1000000)
static int exit_flag = 0;
static int count = 0;

struct test_handler
{
	ipt_event_handler_t eh;
};

int handle_input(ipt_event_handler_t *this, ipt_event_handler_mask_t mask)
{
	ipt_time_value_t tv = {1,0};

   	if ( reactor->notify(reactor, this, EVENT_HANDLER_READ_MASK, &tv) < 0 )
   	{
      		printf("Failed to notify the reactor.\n");
      		return -1;
   	}

	if ( count++ == NUMBER_OF_NOTIFICATIONS ) 
	{
		return -1;
	}

	return 0;
}

int handle_close(ipt_event_handler_t *this, ipt_event_handler_mask_t mask)
{
	exit_flag = 1;
	return 0;
}

void test_handler_init(struct test_handler *this)
{
	this->eh.handle_input = (int (*)(ipt_event_handler_t *, ipt_handle_t )) handle_input;
	this->eh.handle_close = (int (*)(ipt_event_handler_t *, ipt_event_handler_mask_t mask)) handle_close;
}

int main(int argc , char *argv[])
{

	int pfd[2];
	pid_t cpid;

	struct test_handler th;

	test_handler_init(&th);

	reactor = ipt_reactor_create();

      	ipt_time_value_t tv = {1,0};

   	if ( reactor->notify(reactor, (ipt_event_handler_t *) &th, EVENT_HANDLER_READ_MASK, &tv) < 0 )
   	{
      		printf("Failed to notify the reactor.\n");
      		return -1;
   	}

		
   	while (!exit_flag)
   	{
      		ipt_time_value_t interval = {1,0};

      		reactor->run_event_loop(reactor, &interval);
   	}

	assert ( count == NUMBER_OF_NOTIFICATIONS + 1 );

	printf(" %s completed successfully\n", argv[0]);

   	return 0;
}
