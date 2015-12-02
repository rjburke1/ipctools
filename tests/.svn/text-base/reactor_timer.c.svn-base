#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "reactor.h"

static unsigned int timers_fired = 0;
struct test_handler
{
	ipt_event_handler_t eh;
};

int handle_timeout(ipt_event_handler_t *this, const ipt_time_value_t *tv, const void *act)
{
	timers_fired++;
	return 0;
}

test_handler_init(struct test_handler *this)
{
	this->eh.handle_timeout = (int (*)(ipt_event_handler_t *, const ipt_time_value_t *tv, const void *act)) handle_timeout;
}

int main(int argc , char *argv[])
{

	int pfd[2];
	pid_t cpid;

	struct test_handler th;

	test_handler_init(&th);

	ipt_reactor_t *reactor = ipt_reactor_create();

   	ipt_time_value_t one_shot = {1,0};
   
   	ipt_time_value_t interval = {0,100000};

   if ( reactor->schedule_timer(reactor, (ipt_event_handler_t *) &th, &one_shot, &interval, NULL) < 0 )
   {
      printf("Failed to schedule timer.\n");
      return -1;
   }

   int loops=0;
   while (loops++ < 5)
   {
      ipt_time_value_t interval = {2,0};

      reactor->run_event_loop(reactor, &interval);
   }

   /* The timers cause the run_event_loop to exit. so the numbers should be equal if timers are working. */
   assert( timers_fired == --loops );

   printf("%s completed successfully. %d timers fired\n",argv[0], timers_fired);

   return 0;
}
