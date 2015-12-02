#ifndef __IPCTOOLS_REACTOR_H__
#define __IPCTOOLS_REACTOR_H__

#include "event_handler.h"

/**
 * \defgroup Reactor The components used to support asynchronous, Inversion of Control programming.
 *
 * The reactor collection of classes includes a reactor pattern and event handlers.
 * @{
 */

/** 
 * typdef for the reactor structure
 */
typedef struct ipt_reactor_t ipt_reactor_t;

/** 
 * The reactor structure.
 */
struct ipt_reactor_t
{
        /**
         * Run the event loop. 
         *
         * @param[in] this The reactor's this pointer.
         * @param[in] tv  The time to stay in the reactor waiting for an event.
         *
         * @retval >=0 The number of events dispatched.
         * @retval -1  The reactor failed.
         */
	int (*run_event_loop)(ipt_reactor_t *this, struct timeval *tv);

        /**
         * Register an event handler with the reactor.
         *
         * @param[in] this The reactor's this pointer.
         * @param[in] eh_ptr The event handler that will be dispatched  
         * @param[in] mask The event mask tells the reactor which events to associate with the handler.  
         *
         * @retval 0 Registration succeeded.
         * @retval -1 Registration failed. 
         */
	int (*register_handler)(ipt_reactor_t *this, ipt_event_handler_t *eh_ptr, ipt_event_handler_mask_t mask);

        /**
         * Register a signal handler with the reactor.
         *
         * @param[in] this The reactor's this pointer.
         * @param[in] eh_ptr The event handler that will be dispatched  
         * @param[in] signum The signum tells the reactor which signal to associate with the handler.
         *
         * @retval 0 Registration succeeded.
         * @retval -1 Registration failed.
         */
	int (*register_sig_handler)(ipt_reactor_t *this, ipt_event_handler_t *eh_ptr, int signum);

        /**
         * Remove a handler from the reactor.
         *
         * @param[in] this The reactor's this pointer.
         * @param[in] eh_ptr The event handler that will be dispatched
         * @param[in] signum The mask tells the reactor which event is associated with the handler.
         *
         * @retval 0 Removal succeeded.
         * @retval -1 Removal failed.
         */
	int (*remove_handler)(ipt_reactor_t *this, ipt_event_handler_t *eh_ptr, ipt_event_handler_mask_t mask);

        /**
         * Schedule timer with reactor.
         *
         * @param[in] this The reactor's this pointer.
         * @param[in] eh_ptr The event handler that will be dispatched
         * @param[in] one_shot If this is not NULL, the timer will be executed once. 
         * @param[in] interval If this is not NULL, the timer will be executed on a fixed interval. 
         * @param[in] act  This will be passed to the handle_timeout method of the event handler.
         *
         * @retval 0 Scheduling succeeded.
         * @retval -1 Scheduling failed.
         */
	int (*schedule_timer)(ipt_reactor_t *this, ipt_event_handler_t *eh_ptr, const ipt_time_value_t *one_shot, const ipt_time_value_t *interval, const void * act);

        /**
         * Remove timer from reactor.
         *
         * @param[in] this The reactor's this pointer.
         * @param[in] eh_ptr The event handler that will be dispatched
         *
         * @retval 0 Removal succeeded.
         * @retval -1 Removal failed.
         */
	int (*remove_timer)(ipt_reactor_t *this, ipt_event_handler_t *eh_ptr);

        /**
         * Register a notification with the reactor.
         *
         * @param[in] this The reactor's this pointer.
         * @param[in] eh_ptr The event handler that will be dispatched
         * @param[in] mask The mask is used to indicate which event will be notified.
         * @param[in] tv The tv is used to indicate when the notification will occur.
         *
         * @retval 0 Removal succeeded.
         * @retval -1 Removal failed.
         */
	int (*notify)(ipt_reactor_t *this, ipt_event_handler_t *eh_ptr, ipt_event_handler_mask_t mask,ipt_time_value_t *tv);

        /**
         * Destroy the reactor. All memory will be cleaned up.
         *
         * @param[in] this The reactor's this pointer.
         *
         */
	void (*destroy)(ipt_reactor_t *this);

};

/**
 * Create a reactor. 
 *
 * @retval !NULL the pointer to the new reactor.
 * @retval NULL  The constructor failed.
 */
ipt_reactor_t * ipt_reactor_create(void);

/** @} */
#endif
