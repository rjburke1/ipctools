#ifndef __IPT_ACCEPTOR_HANDLER_H__
#define __IPT_ACCEPTOR_HANDLER_H__

#include "reactor.h"

typedef struct ipt_acceptor_handler_t ipt_acceptor_handler_t;
/**
 * Handle the inbound barcodes from the system driver
 */
struct ipt_acceptor_handler_t
{
	ipt_event_handler_t eh;

	/* Parent reactor */
	ipt_reactor_t *reactor;

	/* Callback to create connection */
        ipt_event_handler_t * (*create_conn)(ipt_acceptor_handler_t *this);

	/* state. number of active bitw connections */;
	unsigned short num_cons;
};

ipt_acceptor_handler_t *ipt_acceptor_handler_create(ipt_reactor_t *reactor, ipt_event_handler_t *(*create_con)(ipt_acceptor_handler_t*) );

#define MAX_NUM_CONNECTIONS (10)

#endif
