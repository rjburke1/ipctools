#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <semaphore.h>
#include "logger.h"
#include "shared_queue.h"
#include <errno.h>

typedef struct private_logger_t private_logger_t;
typedef struct shared_data_t shared_data_t;

#define MAX_NUMBER_LOGGER_SINGKS (16)
#define MAX_MESSAGE_SIZE (256)

/**
 * @struct shared_data_t
 *
 * @brief Private class that is contained in the allocator and accessable
 *        from multiple processes.
 *
 */
struct shared_data_t
{
	/**
         * The category mask
         */
	ipt_log_category_mask_t category_mask;

	/**
         * The level mask
         */
	ipt_log_level_mask_t level_mask;

	/**
         * Semaphore
         */
	sem_t sem;
};

/**
 * @struct private_logger_t
 *
 * @brief Private class that is allocated off a processes's heap and provides
 *        local access to the process manager. 
 */
struct private_logger_t
{
	/**
         * public interface.
         */
	ipt_logger_t public;
	
	/**
         * The process manager uses a shared queue.
         */
	ipt_shared_queue_t *sq_ptr;

	/**
         * Pointer to the shared data.
         */
	shared_data_t *sd_ptr;

	/**
         * Allocator used by the logger.
         */
	ipt_allocator_t *alloc_ptr;
};

static ipt_allocator_t* get_allocator(ipt_logger_t *this)
{
   private_logger_t* ptr = (private_logger_t*)this;

   if (ptr == NULL)
   {
      return NULL;
   }

   return ptr->alloc_ptr;
}

void
ipt_logger_for_each(ipt_logger_t *this, void (*func)(const ipt_logger_message_t *const , void *), void *in_ptr)
{
        ipt_shared_queue_for_each(((private_logger_t *)this)->sq_ptr, 
		 (void (*)(const ipt_shared_queue_node_t *const,void *)) func , in_ptr);
}

static 
int syslog_priority(private_logger_t *this, ipt_logger_message_t *msg)
{
	/* All Compiled Network messages are mapped to Facility 2 ( user-messages ) defined in RFC 3164 
	 *
	 * Return the highest severity.
	 */

	if ( msg->lmask & IPT_LEVEL_EMERG )
	{
		return 1 * 8 + 0;
	}
	else if ( msg->lmask & IPT_LEVEL_ALERT )
	{
		return 1 * 8 + 1;
	}
	else if ( msg->lmask & IPT_LEVEL_CRIT )
	{
		return 1 * 8 + 2;
	}
	else if ( msg->lmask & IPT_LEVEL_ERROR )
	{
		return 1 * 8 + 3;
	}
	else if ( msg->lmask & IPT_LEVEL_WARNING)
	{
		return 1 * 8 + 4;
	}
	else if ( msg->lmask & IPT_LEVEL_NOTICE)
	{
		return 1 * 8 + 5;
	}
	else if ( msg->lmask & IPT_LEVEL_INFO)
	{
		return 1 * 8 + 6;
	}
	else if ( msg->lmask & IPT_LEVEL_DEBUG )
	{
		return 1 * 8 + 7;
	}

	/* Return IPT_LEVEL_DEBUG if not match */
	return 1 * 8 + 7;
}

static int 
enqueue(private_logger_t *this, ipt_log_category_mask_t category_mask, ipt_log_level_mask_t level_mask, const char *originator, const char *fmt, ...)
{

	sem_wait(&this->sd_ptr->sem);

	if ( !(this->sd_ptr->category_mask & category_mask)  || !(this->sd_ptr->level_mask & level_mask) )
	{
		sem_post(&this->sd_ptr->sem);
		return -1;
	}

	ipt_logger_message_t *ptr = this->alloc_ptr->malloc(this->alloc_ptr, sizeof(ipt_logger_message_t) );

	if ( ptr == NULL )
	{
		sem_post(&this->sd_ptr->sem);
		return -1;
	}

	memset(ptr,0,sizeof(ipt_logger_message_t));

   	ptr->cmask = category_mask;
   	ptr->lmask = level_mask;

   	struct timeval tv;
   	struct tm *ptm;

   	gettimeofday(&tv,NULL);

   	ptm = localtime(&tv.tv_sec);

   	strftime(ptr->time, sizeof(ptr->time), "%Y-%m-%d %H:%M:%S",ptm);

   	if ( strlen(ptr->time) < 32 )
   	{
      		sprintf(ptr->time + strlen(ptr->time),":%lu",tv.tv_usec/1000);
   	}

	va_list ap;
	va_start(ap,fmt);
	unsigned int size = vsnprintf(ptr->message, MAX_MESSAGE_SIZE, fmt, ap);
	va_end(ap);

	ptr->message[MAX_MESSAGE_SIZE -1] = '\0';

   	if ( originator != NULL )
   	{
         	strncpy(ptr->originator,originator,32);
   	}
   	else
   	{
      		strcpy(ptr->originator,"unknown");
   	}

   	ptr->originator[31] = '\0';

	this->sq_ptr->enqueue(this->sq_ptr, (ipt_logger_node_t *)ptr);

	sem_post(&this->sd_ptr->sem);

	return 0;
}	

static ipt_logger_message_t *
dequeue_timed(private_logger_t *this, ipt_time_value_t *tv)
{
	return (ipt_logger_message_t *)this->sq_ptr->dequeue_timed(this->sq_ptr, tv);
}

static ipt_logger_message_t *
dequeue(private_logger_t *this)
{
	return (ipt_logger_message_t *) this->sq_ptr->dequeue(this->sq_ptr);
}

static int
get_category_mask(private_logger_t *this)
{
	return this->sd_ptr->category_mask;
}


static int
get_level_mask(private_logger_t *this)
{
	return this->sd_ptr->level_mask;
}

static int
is_category_set(private_logger_t *this, ipt_log_category_mask_t mask)
{
	return this->sd_ptr->category_mask & mask;
}

static int
is_level_set(private_logger_t *this, ipt_log_level_mask_t mask)
{
	return this->sd_ptr->level_mask & mask;
}

static void 
set_category(private_logger_t *this, ipt_log_category_mask_t mask)
{
	this->sd_ptr->category_mask |= mask;	
}

static void
set_level(private_logger_t *this, ipt_log_level_mask_t mask)
{
	this->sd_ptr->level_mask |= mask;
}
static void
unset_category(private_logger_t *this, ipt_log_category_mask_t mask)
{
	this->sd_ptr->category_mask & ~mask;
}
static void
unset_level(private_logger_t *this, ipt_log_level_mask_t mask)
{
	this->sd_ptr->level_mask & ~ mask;
}

static int
get_fd(private_logger_t *this)
{
	return this->sq_ptr->get_fd(this->sq_ptr);
}

static void
private_free(private_logger_t *this, void *ptr)
{
	this->alloc_ptr->free(this->alloc_ptr, ptr);
}
void
ipt_logger_dump_message(ipt_logger_message_t *msg)
{
	if ( !msg ) return;

	fprintf(stdout,"%s %s %s\n",msg->time, msg->originator, msg->message);
}
ipt_logger_t *ipt_logger_create(const char *name, ipt_allocator_t *alloc_ptr)
{

	if ( alloc_ptr == NULL || name == NULL || strlen(name) + 1 > 256 )
	{
		return NULL;
	}
	private_logger_t *this = malloc(sizeof(private_logger_t));
	if ( this == NULL )
	{
		return NULL;
	}

	this->alloc_ptr = alloc_ptr;
	if ( (this->sd_ptr = (shared_data_t *)alloc_ptr->malloc(alloc_ptr,sizeof(shared_data_t))) == NULL )
	{
		free(this);
		return NULL;
	}
	memset(this->sd_ptr, 0, sizeof(shared_data_t));

	if ( alloc_ptr->register_object(alloc_ptr, name, this->sd_ptr) < 0 )
   	{
      		free(this);
      		alloc_ptr->free(alloc_ptr,this->sd_ptr);
      		return NULL;
   	}

	char tmp[512];
	sprintf(tmp,"%s.sq",name);

	if ( (this->sq_ptr = ipt_shared_queue_create(tmp,alloc_ptr) ) == NULL)
	{
		alloc_ptr->free(alloc_ptr,this->sd_ptr);
		free(this);
		return NULL;
	}

	if ( sem_init(&this->sd_ptr->sem, 1, 1 ) < 0 )
	{
		alloc_ptr->free(alloc_ptr,this->sd_ptr);
		free(this);
		return NULL;
	}

	this->public.enqueue = (int (*)(ipt_logger_t *this, ipt_log_category_mask_t, ipt_log_level_mask_t, const char *, const char *fmt, ...)) enqueue;
	this->public.dequeue = (ipt_logger_message_t * (*)(ipt_logger_t *this)) dequeue;
	this->public.dequeue_timed      = (ipt_logger_message_t *(*)(ipt_logger_t *this, ipt_time_value_t *)) dequeue_timed;
	this->public.is_category_set = (int (*)(ipt_logger_t *this, enum ipt_log_category_mask_t)) is_category_set;
	this->public.is_level_set = (int (*)(ipt_logger_t *this, enum ipt_log_level_mask_t)) is_level_set;
	this->public.set_category = (void (*)(ipt_logger_t *this, enum ipt_log_category_mask_t)) set_category;
	this->public.set_level = (void (*)(ipt_logger_t *this, enum ipt_log_level_mask_t)) set_level;
	this->public.unset_category = (void (*)(ipt_logger_t *this, enum ipt_log_category_mask_t)) unset_category;
	this->public.unset_level = (void (*)(ipt_logger_t *this, enum ipt_log_level_mask_t)) unset_level;
	this->public.get_fd      = (int (*)(ipt_logger_t *this)) get_fd;
	this->public.free    = (void (*)(ipt_logger_t *this, void *)) private_free;
   	this->public.get_allocator = (ipt_allocator_t* (*)(ipt_logger_t*))get_allocator;

	return (ipt_logger_t *) this;
}

ipt_logger_t *ipt_logger_attach(const char *name, ipt_allocator_t *alloc_ptr)
{

	if ( alloc_ptr == NULL || name == NULL || strlen(name) + 1 > 256 )
	{
		return NULL;
	}

	private_logger_t *this = malloc(sizeof(private_logger_t));


	if ( this == NULL )
	{
		return NULL;
	}


	this->alloc_ptr = alloc_ptr;
	
	if ( (this->sd_ptr = alloc_ptr->find_registered_object(alloc_ptr,name)) == NULL )
	{
		free(this);
		return NULL;
	}

	char tmp[512];
	sprintf(tmp,"%s.sq",name);

	if ( (this->sq_ptr = ipt_shared_queue_attach(tmp,alloc_ptr) ) == NULL )
	{
		free(this);
		return NULL;
	}

	this->public.enqueue            = (int (*)(ipt_logger_t *this, ipt_log_category_mask_t, ipt_log_level_mask_t, const char *, const char *fmt, ...)) enqueue;
	this->public.dequeue            = (ipt_logger_message_t * (*)(ipt_logger_t *this)) dequeue;
	this->public.dequeue_timed      = (ipt_logger_message_t *(*)(ipt_logger_t *this, ipt_time_value_t *)) dequeue_timed;
	this->public.is_category_set    = (int (*)(ipt_logger_t *this, enum ipt_log_category_mask_t)) is_category_set;
	this->public.is_level_set       = (int (*)(ipt_logger_t *this, enum ipt_log_level_mask_t)) is_level_set;
	this->public.set_category       = (void (*)(ipt_logger_t *this, enum ipt_log_category_mask_t)) set_category;
	this->public.get_category_mask  = (enum ipt_log_category_mask_t (*)(ipt_logger_t *this)) get_category_mask;
	this->public.get_level_mask     = (enum ipt_log_level_mask_t (*)(ipt_logger_t *this))  get_level_mask;
	this->public.set_level          = (void (*)(ipt_logger_t *this, enum ipt_log_level_mask_t)) set_level;
	this->public.unset_category     = (void (*)(ipt_logger_t *this, enum ipt_log_category_mask_t)) unset_category;
	this->public.unset_level        = (void (*)(ipt_logger_t *this, enum ipt_log_level_mask_t)) unset_level;
	this->public.get_fd             = (int (*)(ipt_logger_t *this)) get_fd;
	this->public.syslog_priority    = (int (*)(ipt_logger_t *this, ipt_logger_message_t *)) syslog_priority;
	this->public.free               = (void (*)(ipt_logger_t *this, void *)) private_free;
   
   	this->public.get_allocator = (ipt_allocator_t* (*)(ipt_logger_t*))get_allocator;
	
	return (ipt_logger_t *) this;
}
