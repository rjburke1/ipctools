#ifndef __IPCTOOLS_EVENT_HANDLER_H__
#define __IPCTOOLS_EVENT_HANDLER_H__

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * \addtogroup Reactor
 * @{
 */

/**
  * Typedef for socket handle
  */
typedef int ipt_handle_t;

/**
  * Typedef for the event handler mask. 
  */
typedef enum ipt_event_handler_mask_t ipt_event_handler_mask_t;

/**
  * Typedef for the event handler mask used by reactor.
  */
typedef struct ipt_event_handler_t ipt_event_handler_t;

/**
  * Typedef for the timeval structure used by reactor. 
  */
typedef struct timeval ipt_time_value_t;

/**
 * Enumeration for masks used by reactor to handle events.
 */
enum ipt_event_handler_mask_t
{
	EVENT_HANDLER_READ_MASK       = 1<<0,
	EVENT_HANDLER_WRITE_MASK      = 1<<1,
	EVENT_HANDLER_EXCEPT_MASK     = 1<<2,
	EVENT_HANDLER_SIGNAL_MASK     = 1<<3,
	EVENT_HANDLER_TIMER_MASK      = 1<<4,
	EVENT_HANDLER_DONT_CALL_MASK  = 1<<5
};

/**
  * @struct ipt_event_handler_t
  *
  * @brief The Event_Handler class is an abstract class used to interface with the reactor subsystem.
  *
  */
struct ipt_event_handler_t
{

	/**
	 * Called when an input event occurs 
	 * For example, the fd would be readable.
	 *
	 * @param fd  I/O handle
	 */
	int (*handle_input)(ipt_event_handler_t *this, ipt_handle_t h);

	/**
	 * Called when output events are possible 
   	 * For example, the fd would be writable.
	 * 
	 * @param fd  I/O handle
	 */
	int (*handle_output)( ipt_event_handler_t *this, ipt_handle_t h);

	/**
	 * Called when a timer has fired.
  	 *
 	 * @param current_time The current time.
	 * @param act The asynchronous completion token passed in when timer was registered.
	 */
	int (*handle_timeout) (ipt_event_handler_t *this, const ipt_time_value_t *current_time, const void *act);

	/**
	 * Called to set the I/O handle.
	 *
	 * @param fd I/O handle.
	 */
	int (*set_handle)(ipt_event_handler_t *this, ipt_handle_t h);

	/**
	 * Called to get the I/O handle.
	 *
	 * @return I/O handle 
	 */
	int (*get_handle)(ipt_event_handler_t *this);

	/**
	 * Called when the event_handler_t has received a signal from the kernel.
	 *
	 * @param signum the signal number
	 */
	int (*handle_signal) (ipt_event_handler_t *this, int signum);

	/**
	 * Called when any of the handle_* methods return -1.
	 * Also called when the event_handler_t has been removed.
	 *
	 * @param fd I/O handle
	 * @param close_mask defines which event triggered the handle_close.
	 */
	int (*handle_close)(ipt_event_handler_t *this, ipt_handle_t h, ipt_event_handler_mask_t mask);

       /**
         * Destroy the reactor. 
         *
         * @param this The this pointer
         *
         * @retval 0 Succeeded.
         * @retval 1 Failed.
         */
	int (*destroy)( ipt_event_handler_t *this);

	/**
         *  The I/O handler. 
         */
	ipt_handle_t _h;
};

/** @{ */

#endif
