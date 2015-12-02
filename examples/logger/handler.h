#ifndef __IPT_LOGGER_HANDLER_H__
#define __IPT_LOGGER_HANDLER_H__

#include "event_handler.h"
#include "shared_queue.h"
struct my_handler
{
	ipt_event_handler_t eh;
	ipt_shared_queue_t *sq_ptr;
	ipt_allocator_t *alloc_ptr;
};

int handle_input(ipt_event_handler_t *eh, ipt_handle_t h);
int set_handle(ipt_event_handler_t *eh, ipt_handle_t h);
int get_handle(ipt_event_handler_t *eh);

#endif
