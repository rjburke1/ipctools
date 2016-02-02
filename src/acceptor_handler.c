#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "acceptor_handler.h"

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
int handle_input(ipt_event_handler_t *eh_ptr, ipt_handle_t h)
{
struct sockaddr_in cli_addr;
int sockfd, clilen;
ipt_acceptor_handler_t *ah_ptr = (ipt_acceptor_handler_t*)eh_ptr;
ipt_event_handler_t *con_ptr;


	printf("acceptor:handle_input -> connection request from client\n");

 	clilen = sizeof(cli_addr);
   
	if ( (sockfd = accept(h, (struct sockaddr *)&cli_addr, &clilen)) < 0 )
	{
		printf("Failed to accept\n");
		return 0;
	}

	if ( ah_ptr->num_cons  > MAX_NUM_CONNECTIONS )
	{
		printf("Exceeded max number of connections. Closing connection.\n");
		close(sockfd);
		return 0;
	}
	/* Callback here */
	if ( !ah_ptr->create_conn )
	{
		printf("Cannot create a connection. The callback is NULL\n");
		return 0;
	}

	/* Now we need to create the connection handler */
	if ( (con_ptr = ah_ptr->create_conn(ah_ptr)) == NULL )
	{
		printf("Failed to create connection handler\n");
		return 0;
	}
	
	/* Assign the socket to the event handler */
	con_ptr->set_handle(con_ptr, sockfd);

	/* Register with the reactor */
	ipt_reactor_t *r_ptr = ((ipt_acceptor_handler_t *)eh_ptr)->reactor;

 	if ( r_ptr->register_handler(r_ptr, con_ptr, EVENT_HANDLER_READ_MASK) < 0 )
        {
                printf("failed to register connection handler\n");
		close(sockfd);
                return 1;
        }

	/* Increment number of connections */
	ah_ptr->num_cons++;

	return 0;
}
ipt_acceptor_handler_t *ipt_acceptor_handler_create(ipt_reactor_t *reactor, ipt_event_handler_t *(*create_conn)(ipt_acceptor_handler_t *) )
{
        ipt_acceptor_handler_t *this = malloc(sizeof(struct ipt_acceptor_handler_t));

        if ( this == NULL )
        {
                return NULL;
        }

        this->eh.handle_input =  (int (*)(ipt_event_handler_t *, ipt_handle_t)) handle_input;
        this->eh.set_handle = (int (*)(ipt_event_handler_t *,ipt_handle_t)) set_handle;
        this->eh.get_handle = (ipt_handle_t (*)(ipt_event_handler_t*)) get_handle;

	this->reactor = reactor;
	this->num_cons = 0;

	this->create_conn = (ipt_event_handler_t*(*)(ipt_acceptor_handler_t *)) create_conn;

        return this;
}

