#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include "process_monitor.h"
#include "allocator_shm.h"



typedef struct private_process_monitor_t private_process_monitor_t;
typedef struct shared_data_t shared_data_t;

#define MAX_NUMBER_PROCESS_ENTRIES (16)
#define	PROCESS_MONITOR_DEFAULT_EXPIRE_INTERVAL (16)

/**
 * @struct shared_data_t
 *
 * @brief Private class that represents the memory block.
 *
 */

struct shared_data_t
{
	/**
         * List of process entries. We used a fixed size.
         */
	struct process_monitor_entry_t entries[MAX_NUMBER_PROCESS_ENTRIES];

	/**
         * Semaphore for the shared data access.
         */
	sem_t sem;
};

/**
 * @struct private_process_monitor_t
 *
 * @brief Private class that represents the process monitor. 
 *
 * This private structure is always allocated off the heap of the 
 * calling process. It is not allocated via an allocator.
 *
 */
struct private_process_monitor_t
{

	/**
         * public interface
         */
	ipt_process_monitor_t public;

	/**
         * Shared data structure
         */
	shared_data_t *sd_ptr;

	/**
         * Allocator used.
         */
	ipt_allocator_t *alloc_ptr;
};

static int match_free(const process_monitor_entry_t *entry, void *data)
{
	return entry->in_use == 0 ? 1 : 0;
}

static int match_process_name(const process_monitor_entry_t *entry, void *data)
{
	return entry->in_use && !strcmp(entry->name,(char *)data);
}

static void 
print_in_use_entry(const process_monitor_entry_t *const entry, const void *data)
{

	if ( !entry->in_use ) 
	{
		return;
	}

	printf("Process Monitor Entry[\n");
	printf("\tname=%s\n",entry->name);
	printf("\tpid=%i\n",entry->pid);
	printf("\tupdate_time=%d\n",entry->update_time);
	printf("\texpire_interval=%d\n",entry->expire_interval);

	return;
}

void
ipt_process_monitor_for_each(ipt_process_monitor_t *this, void (*func)(const process_monitor_entry_t *const entry, const void *data), const void *data)
{

	int i;

	for ( i=0; i < MAX_NUMBER_PROCESS_ENTRIES; i++)
	{
		/* this is really not a good idea. need to refactor this. if a core dump in upcall, then the semaphore will be locked */
		sem_wait(&((private_process_monitor_t *)this)->sd_ptr->sem);
		(*func)(&((private_process_monitor_t*)this)->sd_ptr->entries[i],data) ;
		sem_post(&((private_process_monitor_t *)this)->sd_ptr->sem);
	}

	return;
}

static process_monitor_entry_t * 
find_entry(private_process_monitor_t *this, int (*compare)(const process_monitor_entry_t *entry, void *data), void *data)
{
	int i;

	for (i = 0; i < MAX_NUMBER_PROCESS_ENTRIES; i ++)
	{

		if ((*compare)(&this->sd_ptr->entries[i],data) )
		{
			return  &this->sd_ptr->entries[i]; 
		}
	}

	return NULL;
}

static int 
remove_process(private_process_monitor_t *this, const char *process_name)
{
	sem_wait(&this->sd_ptr->sem);

	process_monitor_entry_t *e_ptr = find_entry(this, match_process_name, (void *)process_name);

	if ( e_ptr == NULL )
	{
		sem_post(&this->sd_ptr->sem);
		return -1;
	}

	e_ptr->in_use = 0;

	sem_post(&this->sd_ptr->sem);

	return 0;
}

static int 
register_process(private_process_monitor_t *this, const char *name, const char *path, const char *const argv[])
{

	process_monitor_entry_t *e_ptr;

	/* Sanity check */
	if ( name == NULL || strlen(name) + 1 > 64 || path == NULL || strlen(path) + 1 > 256 )
	{
		return -1;
	}

	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC,&tp);

	sem_wait(&this->sd_ptr->sem);

	/* If already exists, use it */
	if ( (e_ptr = find_entry(this, match_process_name, (void *)name)) != NULL)
	{
		e_ptr->update_time = tp.tv_sec;
		e_ptr->pid = getpid();
		e_ptr->in_use = 1;
		sem_post(&this->sd_ptr->sem);
		return 0;
	}

	/* Get next free entry */
	e_ptr = find_entry(this, match_free, NULL);

	if ( e_ptr == NULL )
	{
		sem_post(&this->sd_ptr->sem);
		return -1;
	}

	strcpy(e_ptr->name,name);
	strcpy(e_ptr->path,path);

	int i;
	for ( i=0; argv[i] ; i++ )
	{
		strcpy(e_ptr->argv[i],argv[i]);
	}

	e_ptr->update_time = tp.tv_sec;
	e_ptr->pid = getpid();
	e_ptr->in_use = 1;

	sem_post(&this->sd_ptr->sem);

	return 0;
}
static int
set_expire_interval(private_process_monitor_t *this, const char *process_name, unsigned int expire_interval)
{
	if ( process_name == NULL || strlen(process_name) + 1 > 256 )
	{
		return -1;
	}

	sem_wait(&this->sd_ptr->sem);

	process_monitor_entry_t *e_ptr = find_entry(this, match_process_name, (void*)process_name);

	if ( e_ptr == NULL )
	{
		sem_post(&this->sd_ptr->sem);
		return -1;
	}

	e_ptr->expire_interval = expire_interval;

        printf("set_expire_interval int(%i) name(%s)\n", e_ptr->expire_interval, e_ptr->name);

	sem_post(&this->sd_ptr->sem);

	return 0;
	
}
static ipt_handle_t
handle_timeout(ipt_event_handler_t *this, const ipt_time_value_t *tv, const void *act)
{

	char *process_name = (char *) act;

	if ( 	process_name == NULL || strlen(process_name) +1 > 256 )
	{
		return 0;	
	}

	sem_wait(&((private_process_monitor_t *)this)->sd_ptr->sem);

	process_monitor_entry_t *e_ptr = find_entry((private_process_monitor_t*)this, match_process_name, process_name);

	if ( e_ptr == NULL )
	{
		sem_post(&((private_process_monitor_t *)this)->sd_ptr->sem);
		return 0;
	}

	struct timespec tp;

	clock_gettime(CLOCK_MONOTONIC,&tp);

	e_ptr->update_time = tp.tv_sec;

	sem_post(&((private_process_monitor_t *)this)->sd_ptr->sem);

	return 0;
}

static void
dump_stats(private_process_monitor_t *this)
{
	ipt_process_monitor_for_each((ipt_process_monitor_t *)this,print_in_use_entry, NULL);
}

ipt_process_monitor_t * ipt_process_monitor_create(const char *name, ipt_allocator_t *alloc_ptr)
{

	if ( alloc_ptr == NULL || name == NULL || strlen(name) +1 > 256 )
	{
		return NULL;
	}

	private_process_monitor_t *this = malloc(sizeof(private_process_monitor_t));

	if ( this == NULL )
	{
		return NULL;
	}

	/* if we are already in the allocator return error */
        if ( (this->sd_ptr = alloc_ptr->find_registered_object(alloc_ptr,name) ) != NULL )
        {
                free (this);
                return NULL;
        }

        if ( (this->sd_ptr = (shared_data_t *) alloc_ptr->malloc(alloc_ptr, sizeof(shared_data_t))) == NULL )
        {
                free(this);
                return NULL;
        }

	memset(this->sd_ptr, 0, sizeof(shared_data_t));

	this->alloc_ptr = alloc_ptr;

        if ( alloc_ptr->register_object(alloc_ptr, name, this->sd_ptr) < 0 )
        {
                free(this);
                alloc_ptr->free(alloc_ptr,this->sd_ptr);
                return NULL;
        }

	int i;
	for (i = 0; i < MAX_NUMBER_PROCESS_ENTRIES; i ++)
	{
		this->sd_ptr->entries[i].expire_interval = PROCESS_MONITOR_DEFAULT_EXPIRE_INTERVAL;
	}

	this->public.set_expire_interval = (int (*)(ipt_process_monitor_t *this, const char *, unsigned int)) set_expire_interval;
	this->public.register_process    = (int (*)(ipt_process_monitor_t *this,const char *, const char *, const char *const[])) register_process;
	this->public.remove_process      = (int (*)(ipt_process_monitor_t *this, const char *)) remove_process;
	this->public.dump_stats          = (void (*)(ipt_process_monitor_t *this)) dump_stats;

	return (ipt_process_monitor_t *)this;
}

ipt_process_monitor_t * ipt_process_monitor_attach(const char *name, ipt_allocator_t *alloc_ptr)
{

	if ( alloc_ptr == NULL || name == NULL || strlen(name) +1 > 256 )
	{
		return NULL;
	}

	private_process_monitor_t *this = malloc(sizeof(private_process_monitor_t));

	if ( this == NULL )
	{
		return NULL;
	}

	this->alloc_ptr = alloc_ptr;

        /* Attempt to find ourselves first */
        if ( (this->sd_ptr = alloc_ptr->find_registered_object(alloc_ptr,name) ) == NULL )
        {
                return NULL;
        }

	if ( this->sd_ptr == NULL )
	{
		free(this);
		return NULL;
	}

	if ( sem_init(&this->sd_ptr->sem, 1, 1 ) < 0 )
        {
                alloc_ptr->free(alloc_ptr,this->sd_ptr);
                free(this);
                return NULL;
        }

	this->public.set_expire_interval  = (int (*)(ipt_process_monitor_t *this, const char *, unsigned int)) set_expire_interval;
	this->public.register_process     = (int (*)(ipt_process_monitor_t *this,const char *, const char *, const char*const [])) register_process;
	this->public.remove_process       = (int (*)(ipt_process_monitor_t *this, const char *)) remove_process;
	this->public.dump_stats           = (void (*)(ipt_process_monitor_t *this)) dump_stats;
	this->public.eh.handle_timeout    = (ipt_handle_t (*)(ipt_event_handler_t *this, const ipt_time_value_t *, const void *)) handle_timeout;
	
	return (ipt_process_monitor_t *)this;
}
