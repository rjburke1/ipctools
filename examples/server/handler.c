#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#include "handler.h"

extern ipt_allocator_t *alloc_ptr;
#define MAX_MSG_SIZE (256)

/**
 * Handle inbound messages 
 */
static
int handle_input(ipt_event_handler_t *eh_ptr, ipt_handle_t h)
{
int length;
char msg[MAX_MSG_SIZE];

	memset(msg, 0, MAX_MSG_SIZE);

	printf("msg::handle_input\n");

	if ( read(h,&length,sizeof(length)) != sizeof(length) )
	{
		printf("Failed to read message length\n");
		return -1;
	}
	if ( length > MAX_MSG_SIZE ) 
	{
		printf("The max message size of was exceeded. Closing connection. (%i>%i)\n",length, MAX_MSG_SIZE);
		return -1;
	}
	if ( read(h,&msg,length) != length)
	{
		printf("Failed to read message\n");
		return -1;
	}
	printf("The message is : %s\n",msg);

	return 0;

}
static 
int set_handle(ipt_event_handler_t *eh_ptr, ipt_handle_t h)
{
	eh_ptr->_h = h;
}
static 
ipt_handle_t get_handle(ipt_event_handler_t *eh_ptr)
{
	return eh_ptr->_h;
}
static
int handle_close(ipt_event_handler_t *eh_ptr, ipt_handle_t h, ipt_event_handler_mask_t mask)
{
ipt_acceptor_handler_t *ah_ptr = ((ipt_msg_handler_t*)eh_ptr)->ah_ptr;

	printf("handler::handle_close. Decrementing connections. (max=%i/active=%i)\n",MAX_NUM_CONNECTIONS,--ah_ptr->num_cons);
	close(h);
	return 0;
}
ipt_event_handler_t *ipt_msg_handler_create(ipt_acceptor_handler_t *ah_ptr)
{
  	ipt_msg_handler_t *this = malloc(sizeof(struct ipt_msg_handler_t));

	if ( this == NULL )
	{
		return NULL;
	}
 
	this->eh.handle_input =  (int (*)(ipt_event_handler_t *, ipt_handle_t)) handle_input;
	this->eh.set_handle = (int (*)(ipt_event_handler_t *,ipt_handle_t)) set_handle;
	this->eh.get_handle = (ipt_handle_t (*)(ipt_event_handler_t *)) get_handle;
	this->eh.handle_close =  (int (*)(ipt_event_handler_t *, ipt_handle_t, ipt_event_handler_mask_t)) handle_close;

	/* Access parent acceptor that created us */
	this->ah_ptr = ah_ptr;

 	return (ipt_event_handler_t*)&this->eh;
}
