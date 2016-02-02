#ifndef __IPT_BITW_HANDLER_H__
#define __IPT_BITW_HANDLER_H__

#include "shared_queue.h"
#include "acceptor_handler.h"
#include "event_handler.h"

#define MY_STORE_ID 808
#define INVALID_STORE_COUPON "INVALIDSTORECOUPON"
#define REDEEMED_COUPON "REDEEMEDCOUPON"

typedef struct ipt_msg_handler_t ipt_msg_handler_t;

/**
 * Handle the inbound barcodes from the system driver
 */
struct ipt_msg_handler_t 
{
	ipt_event_handler_t eh;
	ipt_acceptor_handler_t *ah_ptr;
};

ipt_msg_handler_t *ipt_msg_handler_create(ipt_acceptor_handler_t *);


/**
 * Store 1d barcode envelope
 */
struct ipt_koupon_1dbarcode
{
	int bc1;
	int bc0;
	int store_id;
	int serial_no;
};

#endif
