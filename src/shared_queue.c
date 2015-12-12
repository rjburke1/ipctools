#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>

#include "shared_in_list.h"
#include "shared_queue.h"
#include "support.h"
#include "event_handler.h"
#include "logger.h"

typedef struct private_shared_queue_t private_shared_queue_t;

/**
 * @struct shared_data 
 *
 * @brief The private shared data for the shared queue.
 *
 */
struct shared_data
{
	/**
         * whether a doorbell is active.
         */
	unsigned long active_doorbell;

	/**
 	 * Number of doorbells written to the internal named pipe.
         */
	size_t writes;

	/**
         * Number of doorbells read from the internal named pipe.
	 */
	size_t reads;

	/**
         * Difference between writes and reads to the internal named pipe.
         */
	size_t high_water;

	/**
         * Semaphore
         */
	sem_t sem;
};

/**
 *
 *The private data structure for the shared queue allocated on the heap
 *        of the calling process. 
 *
 */
struct private_shared_queue_t
{
	/**
         * public interface.
         */
	ipt_shared_queue_t public;

	/**
 	 * Pointer to the shared list.
         */
	ipt_shared_in_list_t *sl_ptr;
	
	/**
         * Pointer to the shared data.
         */
	struct shared_data *sd_ptr;

	/**
         * Pointer to the allocator
         */
	ipt_allocator_t *alloc_ptr;

	/**
         * Doorbell descriptor.
         */
	int doorbell_fd;

	/**
         * The name of the named pipe used to notify
         * listeners that an entry has been added to the queue.
         */
	char name[128];
};

static void
dump_stats(private_shared_queue_t *this)
{
	printf("doorbell active [%s]\n",this->sd_ptr->active_doorbell ? "true": "false");
	printf("number writes %zu\n",this->sd_ptr->writes);
	printf("number reads  %zu\n",this->sd_ptr->reads);
	printf("high water  (%zu,%zu)\n",this->sd_ptr->high_water, this->sd_ptr->writes - this->sd_ptr->reads);
	printf("number elements %zu\n",this->sl_ptr->count(this->sl_ptr));
}

static void 
update_doorbell(private_shared_queue_t *this)
{

	if ( this->sd_ptr->active_doorbell == 0 && this->sl_ptr->head(this->sl_ptr) != NULL )
	{
		char doorbell;
		if ( write(this->doorbell_fd, (void *) &doorbell, sizeof(doorbell)) <= 0 )
                {
		   return;
                }
      		this->sd_ptr->active_doorbell = 1;
		this->sd_ptr->writes++; 
		this->sd_ptr->high_water++;
	}
	
	return;
}

static void enqueue(private_shared_queue_t *this, ipt_shared_queue_node_t *ptr)
{
	sem_wait(&this->sd_ptr->sem);

	this->sl_ptr->add_tail(this->sl_ptr, ptr);

	update_doorbell(this);

	sem_post(&this->sd_ptr->sem);
}

static int get_fd(private_shared_queue_t *this)
{
	return this->doorbell_fd;
}

static ipt_shared_queue_node_t * dequeue(private_shared_queue_t *this)
{

	char doorbell;

	/* Wait on doorbell */
	if  ( read(this->doorbell_fd,&doorbell, sizeof(doorbell)) <= 0 )
        {
	   printf("failed to read the named pipe\n");fflush(stdout);
	   return NULL;
        }

	sem_wait(&this->sd_ptr->sem);

   	this->sd_ptr->active_doorbell = 0;

	this->sd_ptr->reads++; 
	this->sd_ptr->high_water--;

	struct ipt_shared_in_list_node_t *n_ptr = this->sl_ptr->head(this->sl_ptr);

	if ( n_ptr == NULL ) 
	{
		printf("shared_queue::dequeue -> failed to get head\n");
		sem_post(&this->sd_ptr->sem);
		return NULL;
	}
	
	this->sl_ptr->remove(this->sl_ptr, n_ptr); 

	update_doorbell(this);

	sem_post(&this->sd_ptr->sem);

	return n_ptr; 
}

static ipt_shared_queue_node_t *
dequeue_timed(private_shared_queue_t *this, ipt_time_value_t *tv)
{
   if ( handle_is_read_ready(this->doorbell_fd,tv) < 0 )
   {
      return NULL;
   }
   return this->public.dequeue((ipt_shared_queue_t *)this);
}

void
ipt_shared_queue_for_each(ipt_shared_queue_t *this, void (*func)(const ipt_shared_queue_node_t *const, void *), void *in_ptr)
{
	sem_wait(&((private_shared_queue_t*)this)->sd_ptr->sem);
	ipt_shared_in_list_for_each(((private_shared_queue_t *)this)->sl_ptr, func, in_ptr);		
	sem_post(&((private_shared_queue_t*)this)->sd_ptr->sem);
}

void ipt_shared_queue_destroy(ipt_shared_queue_t *this)
{
	char tmp[256];

	/* Destroy the shared list */
	ipt_shared_in_list_destroy(((private_shared_queue_t *)this)->sl_ptr);

	/* Deregister object from allocator */
	((private_shared_queue_t *)this)->alloc_ptr->deregister_object( ((private_shared_queue_t *)this)->alloc_ptr, ((private_shared_queue_t*)this)->name);

	/* Free the shared data */
	((private_shared_queue_t *)this)->alloc_ptr->free( ((private_shared_queue_t *)this)->alloc_ptr, ((private_shared_queue_t*)this)->sd_ptr);

	/* Remove the named pipe for doorbells */
	sprintf(tmp,"/tmp/%s.db", ((private_shared_queue_t *)this)->name);

	unlink(tmp);

	free( (private_shared_queue_t *)this );

	return;
	
}
ipt_shared_queue_t * ipt_shared_queue_create(const char *name, ipt_allocator_t *alloc_ptr)
{
	struct shared_data sd; 
	private_shared_queue_t *this;
	char tmp[256];

        if ( alloc_ptr == NULL || !strlen(name) || strlen(name) > 128 )
        {
                return NULL;
        }

        this = malloc(sizeof(private_shared_queue_t));

        if ( this == NULL )
        {
                return NULL;
        }

	strcpy(this->name, name);

	/* Attempt to make the named pipe for handling the doorbell */
	sprintf(tmp,"/tmp/%s.db",this->name);

	if ( mkfifo(tmp,0777) < 0 ) 
	{
		free(this);
		return NULL;
	}

	if ( (this->doorbell_fd = open(tmp,O_RDWR)) < 0 )
	{
		free(this);
		unlink(tmp);
		return NULL;
	}

	/* TODO: Set descriptor to non-blocking */

	/* Create shared data */ 
	if ( (this->sd_ptr = (struct shared_data *) alloc_ptr->malloc(alloc_ptr, 64/*sizeof(struct shared_data)*/)) == NULL )
        {
		free(this);
		unlink(tmp);
                return NULL;
        }

	/* set allocated memory to zero */
	memset(this->sd_ptr,0, sizeof(struct shared_data));

	/* initialize the semaphore */
	if ( sem_init(&this->sd_ptr->sem, 1, 1 ) < 0 )
	{
		free(this);
		unlink(tmp);
		return NULL;
	}

	/* Register shared data */
	if ( alloc_ptr->register_object(alloc_ptr, this->name, this->sd_ptr) < 0 )
        {
                free(this);
                alloc_ptr->free(alloc_ptr,this->sd_ptr);
                return NULL;
        }

	sprintf(tmp,"%s.sl",this->name);

	/* Create the shared list */	
	if ( ( this->sl_ptr = ipt_shared_in_list_create(tmp,alloc_ptr) ) == NULL )
	{
		alloc_ptr->free(alloc_ptr,this->sd_ptr);
		free(this);
		return NULL;
	}	

	this->sd_ptr->active_doorbell = 0;

   	this->public.enqueue         = (void (*)(ipt_shared_queue_t *, ipt_shared_queue_node_t *)) enqueue;
  	this->public.dequeue         = (ipt_shared_queue_node_t * (*)(ipt_shared_queue_t *)) dequeue;
   	this->public.dequeue_timed   = (ipt_shared_queue_node_t * (*)(ipt_shared_queue_t *, const ipt_time_value_t *tv)) dequeue_timed;
  	this->public.get_fd          = (int (*)(ipt_shared_queue_t *)) get_fd;
	this->public.dump_stats = (void (*)(ipt_shared_queue_t *)) dump_stats;

	this->alloc_ptr = alloc_ptr;

        return (ipt_shared_queue_t *) this;
}

ipt_shared_queue_t * ipt_shared_queue_attach(const char *name, ipt_allocator_t *alloc_ptr)
{
 
	private_shared_queue_t *this;
	char tmp[256];

        if ( alloc_ptr == NULL || !strlen(name) || strlen(name) > 128 )
        {
                return NULL;
        }

        this = malloc(sizeof(private_shared_queue_t));

	
        if ( this == NULL )
        {
                return NULL;
        }

	/* Attempt to make the named pipe for handling the doorbell */
	sprintf(tmp,"/tmp/%s.db",name);

	/* Attempt to open pipe */
	if ( (this->doorbell_fd = open(tmp,O_RDWR)) < 0 )
	{
		free(this);
		return NULL;
	}

	/* Attach to shared data */ 
	if ( (this->sd_ptr = (struct shared_data *) alloc_ptr->find_registered_object(alloc_ptr, name)) == NULL )
        {
		free(this);
                return NULL;
        }

	sprintf(tmp,"%s.sl",name);

 	/* Attach to  the shared list */
        if ( ( this->sl_ptr = ipt_shared_in_list_attach(tmp,alloc_ptr) ) == NULL )
        {
                free(this);
                alloc_ptr->free(alloc_ptr,this->sd_ptr);
                return NULL;
        }

	/* 
 	 * Prime the pump.
	 * Force an update_doorebell when we first attach
	 * 
	 * Now the reader and writer can start in any order
         */
	this->sd_ptr->active_doorbell = 0;

	update_doorbell(this);

	this->public.enqueue= (void (*)(ipt_shared_queue_t *, ipt_shared_queue_node_t *)) enqueue;
	this->public.dequeue = (ipt_shared_queue_node_t * (*)(ipt_shared_queue_t *)) dequeue;
	this->public.dequeue_timed = (ipt_shared_queue_node_t * (*)(ipt_shared_queue_t *, const ipt_time_value_t *tv)) dequeue_timed;
	this->public.get_fd = (int (*)(ipt_shared_queue_t *)) get_fd;
	this->public.dump_stats = (void (*)(ipt_shared_queue_t *)) dump_stats;

	this->alloc_ptr = alloc_ptr;

        return (ipt_shared_queue_t *) this;
}

