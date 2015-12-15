#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "reactor.h"

struct test_handler
{
	ipt_event_handler_t eh;
};

int handle_input(ipt_event_handler_t *this, ipt_handle_t h)
{
	/* Have the reactor call handle close */
	return -1;
}

int handle_close(ipt_event_handler_t *this, ipt_event_handler_mask_t mask)
{
	if ( mask & EVENT_HANDLER_READ_MASK )
	{
		exit ( 0 );
	}

	exit ( 1 );
}
int set_handle(ipt_event_handler_t *this, ipt_handle_t h)
{
	this->_h = h;
}

int get_handle(ipt_event_handler_t *this)
{
	return this->_h;
}

void test_handler_init(struct test_handler *this)
{
	this->eh.handle_input = (int (*)(ipt_event_handler_t *, ipt_handle_t h)) handle_input;
	this->eh.handle_close = (int (*)(ipt_event_handler_t *, ipt_event_handler_mask_t mask)) handle_close;
	this->eh.get_handle   = (ipt_handle_t (*)(ipt_event_handler_t *)) get_handle;
	this->eh.set_handle   = (ipt_handle_t (*)(ipt_event_handler_t *, ipt_handle_t h)) set_handle;
}

int test_1(ipt_reactor_t *reactor, int pfd[2], ipt_event_handler_t *eh)
{

	int status;

	if ( fork() == 0 )	
	{
		close(pfd[1]);
		if ( reactor->register_handler(reactor, eh, EVENT_HANDLER_READ_MASK) < 0 )
		{
			printf("failed to register handler\n");
			exit ( 1 );
		}	

		ipt_time_value_t tv = { 1, 0 }; 

		if ( reactor->run_event_loop(reactor, &tv) != 1 )
		{
			printf("child reactor event loop timed out.\n");
			exit ( 1 );
		}
	}
	else
	{
		close(pfd[0]);  

		if ( write(pfd[1], "This is a test message", strlen("This is a test message")) <= 0 );

		close(pfd[1]); 

		wait(&status);

		assert( WEXITSTATUS(status) == 0 );
	}

	return 0;
}

/* Make sure the reactor returns 0 when timeout fires and no event handlers are dispatched */
int test_2(ipt_reactor_t *reactor, int pfd[2], ipt_event_handler_t *eh)
{

	int status;

	if ( fork() == 0 )	
	{
		close(pfd[1]);
		if ( reactor->register_handler(reactor, eh, EVENT_HANDLER_READ_MASK) < 0 )
		{
			printf("failed to register handler\n");
			exit ( 1 );
		}	

		ipt_time_value_t tv = { 1, 0 }; 

		int rtn =   reactor->run_event_loop(reactor, &tv);
		if ( rtn == 0 )
		{
			exit ( 2 );
		}
		else
		{
			exit ( 1 );
		}
	}
	else
	{
		close(pfd[0]);  

		wait(&status);

		assert( WEXITSTATUS(status) == 2 );
	}

	return 0;
}


int main(int argc , char *argv[])
{

	int pfd[2];
	pid_t cpid;

	struct test_handler th;

	test_handler_init(&th);

	ipt_reactor_t *reactor = ipt_reactor_create();

	if ( pipe(pfd) < 0 )
	{
		printf("pipe method failed\n");
		return -1;
	}

	th.eh.set_handle((ipt_event_handler_t *)&th,pfd[0]);

	test_1 (reactor, pfd, (ipt_event_handler_t *)&th);

	if ( pipe(pfd) < 0 )
	{
		printf("pipe method failed\n");
		return -1;
	}

	test_2( reactor, pfd, (ipt_event_handler_t *)&th);

	printf(" %s completed successfully\n", argv[0]);	

	return 0;
}
