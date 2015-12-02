#include <stdlib.h>
#include <strings.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#define _XOPEN_SOURCE 600
#include <sys/select.h>
#include <sys/signal.h>
#include "reactor.h"
#include "support.h"

#define TIMER_SKEW (100) /* microseconds */
#define MAX_NUMBER_EHS (16)
#define MAX_NUMBER_TMS (16)

/**
 * typedef for private reactor structure
 */
typedef struct private_reactor_t private_reactor_t;

/**
 * typdef for notification handler used by reactor.
 */
typedef struct notify_handler_t notify_handler_t;

/**
 * typdef for notification message used by reactor.
 */
typedef struct notify_msg_t notify_msg_t;

/**
 * typdef for event node used to manage list of events.
 */
typedef struct event_node_t event_node_t;

/**
 * typdef for timer node used to manage list of timers.
 */
typedef struct timer_node_t timer_node_t;

/**
 * Storage for the pending signal set used when handling signals
 */
static sigset_t __pending_sigset;

/**
 * Standard sigaction callback function.
 */
static struct sigaction __sigaction_dfl = {SIG_DFL, 0, 0};

/**
 * Standard variable to handle pending signals
 */
static volatile int __sig_pending=0;

/**
 * Signal handler structure.
 */
static void signal_handler(int signum)
{
	sigaddset(&__pending_sigset, signum);

	return;
}

/**
 * High resolution timer specification
 */
static struct timespec timespec_zero;

/**
 * @struct timer_node_t
 *
 * @brief Private timer node structure.
 */
struct timer_node_t
{
	/**
         * Event handler to be fired when timer expired.
         */
	ipt_event_handler_t *eh_ptr;

	/**
         * In use flag indicates this node is in use.
         */	
	int in_use;

	/**
         * Expiration timer.
         */
	struct timespec expire_time;	

	/**
         * Interval time used for interval timers.
	 */
	struct timespec interval_time;	

	/**
	 * Ascynchronous callback token
         */
	const void * act;
};

/**
 * @struct event_node_t
 *
 * @brief Private event node structure.
 */
struct event_node_t
{
	/** event hander */
	ipt_event_handler_t *eh_ptr;

	/** mask */
	ipt_event_handler_mask_t mask;

	/** signal number */
	int signum;

	/** if this element is in use */
	int in_use;
};

/**
 * @struct notify_msg_t
 *
 * @brief Private notify message structure.
 */
struct notify_msg_t
{
	/** event handler */
	ipt_event_handler_t *eh_ptr;

	/** mask */
	ipt_event_handler_mask_t mask;
};

/**
 * @struct notify_handler_t
 *
 * @brief Private notify structure.
 */
struct notify_handler_t
{
	/** event hander */
	ipt_event_handler_t eh;

	/** notification handers */
	int notify_fds[2];

	/** private reactor */
	private_reactor_t *reactor;
};

/** 
 * @struct private_reactor_t 
 * 
 * @brief Private reactor structure. 
 */
struct private_reactor_t
{
	/** public interface */
	ipt_reactor_t public;

	/** fixed size array of event handers. */
	event_node_t ehs[MAX_NUMBER_EHS];

	/** fixed size array of timers */
	timer_node_t tms[MAX_NUMBER_TMS];

	/** number of active handers */
	unsigned int active_handlers;

	/** number of active timers */
	unsigned int active_timers;

	/** notification hander */
	notify_handler_t notify_handler;

	/** sigaction structure */
	struct sigaction act;

	/** active signal mask */
	sigset_t sigset;

};

static int
notify_get_handle(notify_handler_t *this)
{
	return this->notify_fds[0];
}

static int
notify_handle_input(notify_handler_t *this, ipt_handle_t h)
{

	struct notify_msg_t msg;
	int bytes_transferred;

	ipt_time_value_t tv = { 1,0 };

	if ( recv_n(h,&msg,sizeof(msg), &bytes_transferred,&tv) < 0)
	{
		printf("Catastrophic error! could not read notify queue \n");
		return 0;
	}

	if ( msg.eh_ptr->handle_input(msg.eh_ptr, h) < 0 )
	{
		msg.eh_ptr->handle_close(msg.eh_ptr, EVENT_HANDLER_READ_MASK);
	}

	return 0;
}

static struct timespec 
get_expire_time(private_reactor_t *this, struct timespec current_time)
{
	int i;

   int found = 0;

   struct timespec def = {0xffffff, 0xffffff };
   struct timespec tmp = def;

	for (i=0; i < MAX_NUMBER_TMS; i ++)
	{
		if ( !this->tms[i].in_use ) continue;

		if ( this->tms[i].expire_time.tv_sec < tmp.tv_sec )
		{
			tmp = this->tms[i].expire_time;
		}
		else if ( this->tms[i].expire_time.tv_sec == tmp.tv_sec && this->tms[i].expire_time.tv_nsec < tmp.tv_nsec)
		{
			tmp = this->tms[i].expire_time;
		}
	}

   /* Return default value if no matches found */
   if (timespec_compare(tmp,def) == 0 )
   {
      return tmp;
   }

   /* Return the time left on the timer or zero, whichever is greater */
   return timespec_max(timespec_diff(tmp,current_time), timespec_zero);
}

static ipt_handle_t 
find_max_handle(private_reactor_t *this)
{
	int i;
	ipt_handle_t max_handle = 0;

	for ( i=0; i < MAX_NUMBER_EHS; i ++ )
	{
		if ( !this->ehs[i].in_use ) continue;

		/* We must have a read or write fd */
		if (    (this->ehs[i].mask & EVENT_HANDLER_READ_MASK || this->ehs[i].mask & EVENT_HANDLER_WRITE_MASK  ) &&
			 this->ehs[i].eh_ptr->get_handle(this->ehs[i].eh_ptr) > max_handle )
		{
			max_handle = this->ehs[i].eh_ptr->get_handle(this->ehs[i].eh_ptr);
		}
	}
	
	return max_handle;
}

static void 
load_masks(private_reactor_t *this, fd_set *read_fds, fd_set *write_fds, fd_set *except_fds)
{
	int i;

	for ( i = 0; i < MAX_NUMBER_EHS; i ++)
	{
		if ( !this->ehs[i].in_use ) continue;

		if ( this->ehs[i].mask & EVENT_HANDLER_READ_MASK) 
		{
			FD_SET(this->ehs[i].eh_ptr->get_handle(this->ehs[i].eh_ptr), read_fds);
		}	

		if ( this->ehs[i].mask & EVENT_HANDLER_WRITE_MASK) 
		{
			FD_SET(this->ehs[i].eh_ptr->get_handle(this->ehs[i].eh_ptr), write_fds);
		}	

		if ( this->ehs[i].mask & EVENT_HANDLER_EXCEPT_MASK) 
		{
			FD_SET(this->ehs[i].eh_ptr->get_handle(this->ehs[i].eh_ptr), except_fds);
		}	
	}

	return;
}

static int 
dispatch_timers(private_reactor_t *this, struct timespec current_time)
{
	int i;
	int num_dispatched=0;
	struct timespec tp;

	for ( i = 0; i < MAX_NUMBER_TMS; i++)
	{
		if ( !this->tms[i].in_use ) continue;

		if ( timespec_compare(timespec_diff(this->tms[i].expire_time, current_time), timespec_zero) < 0  || timespec_diff(this->tms[i].expire_time,current_time).tv_nsec < 1000000) 
      		{
         		ipt_time_value_t tmp = { current_time.tv_sec, current_time.tv_nsec/1000};
			num_dispatched++;
			if ( this->tms[i].eh_ptr->handle_timeout(this->tms[i].eh_ptr, &tmp, this->tms[i].act) < 0 )
			{
				if ( this->tms[i].eh_ptr->handle_close )
					this->tms[i].eh_ptr->handle_close(this->ehs[i].eh_ptr, EVENT_HANDLER_TIMER_MASK);

				memset(&this->tms[i],0, sizeof(timer_node_t));;
			}

			if ( this->tms[i].interval_time.tv_sec != 0 || this->tms[i].interval_time.tv_nsec != 0 )
			{
            			struct timespec it = this->tms[i].interval_time;
            			struct timespec tp;
            			clock_gettime(CLOCK_MONOTONIC,&tp);
				this->tms[i].expire_time.tv_sec = tp.tv_sec + it.tv_sec + (it.tv_nsec + tp.tv_nsec ) /1000000000;
				this->tms[i].expire_time.tv_nsec = (it.tv_nsec + tp.tv_nsec) % 1000000000;
			}
			else
			{
				memset(&this->tms[i],0, sizeof(timer_node_t));;
			}
		}
	}

	return num_dispatched;
}

static int 
dispatch_handlers(private_reactor_t *this, fd_set *read_fds, fd_set *write_fds, fd_set *except_fds, sigset_t *sigset)
{
	int i;

	int num_dispatched = 0;

	/* check to see if any signals are pending */
	sigset_t tmp;

	for ( i = 0; i < MAX_NUMBER_EHS; i++)
	{
		if ( !this->ehs[i].in_use ) continue;

		if ( this->ehs[i].mask & EVENT_HANDLER_READ_MASK && FD_ISSET(this->ehs[i].eh_ptr->get_handle(this->ehs[i].eh_ptr), read_fds)) 
		{
			/* Handle the input */
			num_dispatched++;
			if ( this->ehs[i].eh_ptr->handle_input(this->ehs[i].eh_ptr, this->ehs[i].eh_ptr->get_handle(this->ehs[i].eh_ptr)) < 0)
			{
				if ( this->ehs[i].eh_ptr->handle_close )
					this->ehs[i].eh_ptr->handle_close(this->ehs[i].eh_ptr, EVENT_HANDLER_READ_MASK);

				this->ehs[i].in_use = 0;
			}
		}

		if ( this->ehs[i].mask & EVENT_HANDLER_WRITE_MASK && FD_ISSET(this->ehs[i].eh_ptr->get_handle(this->ehs[i].eh_ptr), write_fds)) 
		{
			/* Handle the output */
			num_dispatched++;
			if ( this->ehs[i].eh_ptr->handle_output(this->ehs[i].eh_ptr, this->ehs[i].eh_ptr->get_handle(this->ehs[i].eh_ptr)) < 0)
			{
				if ( this->ehs[i].eh_ptr->handle_close )
					this->ehs[i].eh_ptr->handle_close(this->ehs[i].eh_ptr, EVENT_HANDLER_WRITE_MASK);

				this->ehs[i].in_use = 0;
			}
		}

		if ( this->ehs[i].mask & EVENT_HANDLER_SIGNAL_MASK && sigismember(sigset,this->ehs[i].signum) )
		{
			/* Handle the signal */
			num_dispatched++;
			if ( this->ehs[i].eh_ptr->handle_signal(this->ehs[i].eh_ptr, this->ehs[i].signum) < 0 )
			{
				if ( this->ehs[i].eh_ptr->handle_close )
					this->ehs[i].eh_ptr->handle_close(this->ehs[i].eh_ptr, EVENT_HANDLER_SIGNAL_MASK);

				memset(&this->ehs[i], 0, sizeof(event_node_t));
			}
		}
	}
	return num_dispatched;
}

static int
remove_handler(private_reactor_t *this, ipt_event_handler_t *eh_ptr, ipt_event_handler_mask_t mask)
{
	int i;

	for(i=0; i < MAX_NUMBER_EHS; i++ )
	{
		if ( this->ehs[i].in_use && this->ehs[i].eh_ptr == eh_ptr) 
		{
			if (!( mask & EVENT_HANDLER_DONT_CALL_MASK) )
			{
				this->ehs[i].eh_ptr->handle_close(eh_ptr, this->ehs[i].mask);
			}

			if ( mask & EVENT_HANDLER_SIGNAL_MASK )
			{
				/* Restore the default behavior */
				//sigaction(this->ehs[i].signum,&__sigaction_dfl,0);
			}

			memset(&this->ehs[i],0,sizeof(event_node_t)) ;

			return 0;
		}	
	}
	return -1;
}

static int
run_event_loop(private_reactor_t *this, const ipt_time_value_t *tv)
{
	ipt_handle_t max_handle;
	int rtn;

	max_handle = find_max_handle(this);

	/* Load the Masks */	
	fd_set read_fds, write_fds, except_fds;

	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);
	FD_ZERO(&except_fds);

	load_masks(this, &read_fds, &write_fds, &except_fds);

   	struct timespec current_time;

   	clock_gettime(CLOCK_MONOTONIC,&current_time);

   	struct timespec  time_value = timespec_min(get_expire_time(this,current_time),ipt_time_value_to_timespec(*tv));

	errno = 0;

	/* Do not block registered signals in select loop. */

	sigset_t tmp;

	sigfillset(&tmp);

	int i;
	for ( i = 1; i <= 31; i++ )
	{
		if ( sigismember(&this->sigset, i) )
		{
			sigdelset(&tmp,i);
		}
	}

	rtn = pselect(max_handle + 1, &read_fds, &write_fds, &except_fds, &time_value, &tmp);

	/* If the rtn <= 0, then make sure the read/write 
	 * descriptors are cleared. This will occur when a signal is received during select
	 */
	if ( rtn < 0 )
	{
		FD_ZERO(&read_fds);
		FD_ZERO(&read_fds);
	}

	/* Failure */
	if ( rtn < 0  && errno != EINTR) 
	{
		return -1;
	}

   	clock_gettime(CLOCK_MONOTONIC,&current_time);

	int num_dispatched_handlers = 0;

	/* Dispatch timers */
	num_dispatched_handlers += dispatch_timers(this, current_time);

	/* Dispatch the descriptors */
	num_dispatched_handlers += dispatch_handlers(this,&read_fds, &write_fds, &except_fds, &__pending_sigset);

	/* Reset the modules pending sigset */
	sigemptyset(&__pending_sigset);

	return num_dispatched_handlers;
}

static void
destroy(ipt_reactor_t *this)
{
	if ( this ) 
	{
		free(this);
	}

	return;
}

static event_node_t * 
find_event_node_by_handler(private_reactor_t *this, ipt_event_handler_t *eh_ptr)
{

	int i;

        for(i=0; i < MAX_NUMBER_EHS; i++ )
        {
                if ( this->ehs[i].in_use && eh_ptr == this->ehs[i].eh_ptr )
                {
			return &this->ehs[i];
		}
	}

	return NULL;
}

static event_node_t * 
find_event_node_by_signum(private_reactor_t *this, int signum)
{

	int i;

        for(i=0; i < MAX_NUMBER_EHS; i++ )
        {
                if ( this->ehs[i].in_use && signum == this->ehs[i].signum )
                {
			return &this->ehs[i];
		}
	}

	return NULL;
}

static event_node_t * 
get_next_available_handler(private_reactor_t *this)
{
	int i;

	/* Register new event handler */
        for(i=0; i < MAX_NUMBER_EHS; i++ )
        {
                if ( !this->ehs[i].in_use )
                {
			return &this->ehs[i];
		}
	}

	return NULL;
}

static int
register_sig_handler(private_reactor_t *this, ipt_event_handler_t *eh_ptr, int signum)
{
	event_node_t *n_ptr;

	/* if handler exists, update it. */
	if ( ( n_ptr = find_event_node_by_handler(this, eh_ptr )) )
	{
		n_ptr->mask |= EVENT_HANDLER_SIGNAL_MASK;
		n_ptr->signum = signum;
		return 0;
	}

	/* only allow a single handler per signum */
	if ( find_event_node_by_signum(this,signum) )
	{
		return -1;
	}

	/* find a new handler */
	n_ptr = get_next_available_handler(this);

	if ( n_ptr == NULL )
	{
		return -1;
	}

	n_ptr->eh_ptr = eh_ptr;
	n_ptr->in_use = 1;
	n_ptr->mask = EVENT_HANDLER_SIGNAL_MASK;
	n_ptr->signum = signum;

	/* add to base sigset. These are signals blocked outside select. Essentially during handler upcalls. */
	sigaddset(&this->sigset,signum);

	sigprocmask(SIG_BLOCK,&this->sigset,NULL);

	return sigaction(signum,&this->act,NULL) ;
}

static int
register_handler (private_reactor_t *this, ipt_event_handler_t *eh_ptr, ipt_event_handler_mask_t mask)
{
	int i;

	event_node_t *n_ptr;

	if ((n_ptr = find_event_node_by_handler(this,eh_ptr) ) )
	{
		n_ptr->mask |= mask;
		return 0;
	}

	n_ptr = get_next_available_handler(this);

	if ( n_ptr == NULL )
	{
		return -1;
	}

	n_ptr->eh_ptr = eh_ptr;
	n_ptr->in_use = 1;
	n_ptr->mask  = mask;

	return 0;	
}

static int
remove_timer(private_reactor_t *this, ipt_event_handler_t *eh_ptr)
{
	int i;

	for ( i = 0; i < MAX_NUMBER_TMS; i++)
	{
		if ( this->tms[i].in_use && this->tms[i].eh_ptr == eh_ptr)
		{
			memset(&this->tms[i], 0, sizeof(timer_node_t));
			return 0;
		}
	}

	return -1;
}

static int
schedule_timer(private_reactor_t *this, ipt_event_handler_t *eh_ptr, ipt_time_value_t *one_shot, ipt_time_value_t *interval, const void *act)
{
	int i;

	if ( this->active_timers > MAX_NUMBER_TMS  || ( one_shot == NULL && interval == NULL ))
	{
		return -1;
	}

   	/* Convert time value to timespec and correct */

	struct timespec tp;

	if ( clock_gettime(CLOCK_MONOTONIC, &tp) < 0 ) 
	{
		return -1;
	}

   struct timespec os = timespec_zero;
   struct timespec it = timespec_zero;

   if ( one_shot != NULL )
   {
      os = ipt_time_value_to_timespec(*one_shot);
   }

   if ( interval != NULL )
   {
      it = ipt_time_value_to_timespec(*interval);
   }

	for ( i = 0; i < MAX_NUMBER_TMS; i++)
	{
		if ( !this->tms[i].in_use )
		{
			this->tms[i].eh_ptr = eh_ptr;
			this->tms[i].in_use = 1;
			this->tms[i].act = act;

         		/* Load the one_shot first if it is not null */
			if ( one_shot != NULL )
			{
				this->tms[i].expire_time.tv_sec = tp.tv_sec + os.tv_sec + (os.tv_nsec + tp.tv_nsec) / 1000000000;
				this->tms[i].expire_time.tv_nsec = (os.tv_nsec + tp.tv_nsec) % 1000000000;
			}
			else if ( interval != NULL )
			{
				this->tms[i].expire_time.tv_sec = tp.tv_sec + it.tv_sec + (it.tv_nsec + tp.tv_nsec ) /1000000000;
				this->tms[i].expire_time.tv_nsec = (it.tv_nsec + tp.tv_nsec) % 1000000000;
			}
			
			if ( interval != NULL )
			{
				this->tms[i].interval_time = it;
			}

			return 0;
		}
	}

	return -1;
}

static int
notify(private_reactor_t *this, ipt_event_handler_t *eh_ptr, ipt_event_handler_mask_t mask, ipt_time_value_t *tv)
{

	struct notify_msg_t msg = { eh_ptr, mask };

	unsigned int bytes_transferred;	

	if ( send_n(this->notify_handler.notify_fds[1], &msg, sizeof(msg), &bytes_transferred, tv ) < 0 )
	{
		return -1;
	}

	return 0;
}

ipt_reactor_t * ipt_reactor_create(void)
{

	private_reactor_t *this = malloc(sizeof(private_reactor_t));	

	memset( (char *) this, 0 , sizeof ( private_reactor_t ) );

	int i;
	
	if ( this == NULL )
	{
		return NULL;
	}

	/* Create notification pipe */
	if ( pipe(this->notify_handler.notify_fds) < 0 )
	{
		free(this);
		return NULL;
	}

	/* Create notification handler */
	this->notify_handler.reactor = this;

	this->notify_handler.eh.handle_input = (int (*)(ipt_event_handler_t *, ipt_handle_t)) notify_handle_input;
	this->notify_handler.eh.get_handle = (int (*)(ipt_event_handler_t *)) notify_get_handle;

	this->public.run_event_loop       = (int (*)(ipt_reactor_t *, ipt_time_value_t *)) run_event_loop;
	this->public.register_handler     = (int (*)(ipt_reactor_t *, ipt_event_handler_t *, ipt_event_handler_mask_t)) register_handler;
	this->public.register_sig_handler = (int (*)(ipt_reactor_t *, ipt_event_handler_t *, int)) register_sig_handler;
	this->public.remove_handler       = (int (*)(ipt_reactor_t *, ipt_event_handler_t *, ipt_event_handler_mask_t)) remove_handler;
	this->public.remove_timer         = (int (*)(ipt_reactor_t *, ipt_event_handler_t *)) remove_timer;
	this->public.schedule_timer       = (int (*)(ipt_reactor_t *, ipt_event_handler_t *, const ipt_time_value_t *, const ipt_time_value_t *,const void *)) schedule_timer;
	this->public.destroy              = (void (*)(ipt_reactor_t *this)) destroy;
	this->public.notify               = (int (*)(ipt_reactor_t *, ipt_event_handler_t *, ipt_event_handler_mask_t , ipt_time_value_t *)) notify;

	if ( this->public.register_handler((ipt_reactor_t *)this, (ipt_event_handler_t *)&this->notify_handler, EVENT_HANDLER_READ_MASK) < 0 )
	{
		free (this);
		return NULL;
	}

	this->active_handlers = 0;

	/* Fil sigset. Block all signals */
	sigfillset(&this->sigset);

	sigprocmask(SIG_BLOCK,&this->sigset,NULL);
	 
	memset (&this->act, 0, sizeof(this->act));

	this->act.sa_handler = signal_handler;

	return (ipt_reactor_t *) this;
}

