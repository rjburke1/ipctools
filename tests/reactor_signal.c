#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "reactor.h"

#define REACTOR_INTERVAL (1)
#define NUMBER_OF_SIGNALS (1000)
#define WAIT_TIME_BEFORE_SHUTDOWN (10*REACTOR_INTERVAL)

ipt_reactor_t *reactor;

static int exit_flag = 0;
static int count = 0;

struct test_handler
{
	ipt_event_handler_t eh;
};

int handle_timeout(ipt_event_handler_t *this, const ipt_time_value_t *tv, const void *act)
{
	exit_flag = 1;
	return 0;
}

int handle_signal(ipt_event_handler_t *this, int signum)
{
	if ( count ++ == NUMBER_OF_SIGNALS ) 
	{
		return -1;
	}

	return 0;
}

int handle_close(ipt_event_handler_t *this, ipt_handle_t h, ipt_event_handler_mask_t mask)
{
	ipt_time_value_t one_shot = {WAIT_TIME_BEFORE_SHUTDOWN,0};

	if ( reactor->schedule_timer(reactor, this, &one_shot, NULL, NULL) < 0 )
	{
		printf("Failed to schedule timer.\n");
		return -1;
	}
			                                                                      
	return 0;
}

ipt_handle_t get_handle(ipt_event_handler_t *this)
{
	return -1;
}

void test_handler_init(struct test_handler *this)
{
	this->eh.handle_signal  = (int (*)(ipt_event_handler_t *, int )) handle_signal;
	this->eh.handle_close   = (int (*)(ipt_event_handler_t *, ipt_handle_t, ipt_event_handler_mask_t mask)) handle_close;
	this->eh.get_handle     = (ipt_handle_t (*)(ipt_event_handler_t *)) get_handle;
	this->eh.handle_timeout = (int (*)(ipt_event_handler_t *, const ipt_time_value_t *tv, const void *act)) handle_timeout;
}

int main(int argc , char *argv[])
{

	int pfd[2];
	pid_t cpid;

	struct test_handler th;

	test_handler_init(&th);

	reactor = ipt_reactor_create();

	if ( reactor == NULL )
	{
		fprintf(stderr,"Failed to create the reactor.\n");
		return -1;
	}

	if ( reactor->register_sig_handler(reactor,(ipt_event_handler_t *) &th, SIGTERM) < 0 )
	{
		fprintf(stderr,"Failed to register signal handler.\n");
		return -1;
	}

	ipt_time_value_t tv = {REACTOR_INTERVAL,0};

	while (!exit_flag)
	{
		reactor->run_event_loop(reactor, &tv);
		raise(SIGTERM);
	}

	assert( count == (NUMBER_OF_SIGNALS + 1) );

	printf("Successfully completed %s\n",argv[0]);

   	return 0;
}
