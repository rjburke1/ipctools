#include "shared_in_list.h"
#include <semaphore.h>
#include <string.h>
#include <stdlib.h>
#include "logger.h"
typedef struct private_shared_in_list_t private_shared_in_list_t;

/**
 * @struct shared_data
 *
 * @brief The private shared data for the shared queue.
 *
 */
struct shared_data
{
	/**
 	 * Offset to the head of list.
         */
	ipt_op_t head;

	/**
         * Offset to the tail of list.
         */
	ipt_op_t tail;

	/**
 	 * Number of entries
         */
	size_t count;

	/**
 	 * Semaphore
         */
	sem_t sem;

	/**
	 * Used as null pointer.
         */
	char __null__;
};

/**
 * @struct private_shared_in_list_t
 *
 * @brief The private data structure that is allocated on the heap
 *        of the calling process.
 *
 */
struct private_shared_in_list_t
{
	/**
	 * public interface 
         */
	ipt_shared_in_list_t public;

	/**
 	 * Pointer to the shared data.
 	 */
	struct shared_data *sd_ptr;

	/**
	 * Pointer to the allocator.
         */
	ipt_allocator_t *alloc_ptr;

	/**
         * Name of the shared in list. This is used for registering with the allocator.
         * Then other processes can find this list using it's well known name.
         */ 
	char name[256];
};

static size_t 
count(private_shared_in_list_t *this)
{
	return this->sd_ptr->count;
}
static void 
private_remove(private_shared_in_list_t *this, ipt_shared_in_list_node_t *n_ptr)
{

	sem_wait(&this->sd_ptr->sem);

	if ( ipt_op_drf(&this->sd_ptr->head) == n_ptr )
	{
		ipt_op_set(&this->sd_ptr->head, ipt_op_drf(&n_ptr->next));
	}

	if ( ipt_op_drf(&this->sd_ptr->tail) == n_ptr )
	{
		ipt_op_set(&this->sd_ptr->tail, ipt_op_drf(&n_ptr->prev));
	}

	ipt_op_set(&((struct ipt_shared_in_list_node_t *)ipt_op_drf(&n_ptr->prev))->next, ipt_op_drf(&n_ptr->next));
	ipt_op_set(&((struct ipt_shared_in_list_node_t *)ipt_op_drf(&n_ptr->next))->prev, ipt_op_drf(&n_ptr->prev));

	this->sd_ptr->count--;

	sem_post(&this->sd_ptr->sem);

	return ;
}


static ipt_shared_in_list_node_t * head ( private_shared_in_list_t *this)
{
	return  ipt_op_drf(&this->sd_ptr->head) == &this->sd_ptr->__null__ ? NULL : ipt_op_drf(&this->sd_ptr->head);
}

static ipt_shared_in_list_node_t * tail ( private_shared_in_list_t *this)
{
	return  ipt_op_drf(&this->sd_ptr->tail) == &this->sd_ptr->__null__ ? NULL : ipt_op_drf(&this->sd_ptr->tail);
}

static ipt_shared_in_list_node_t * next(private_shared_in_list_t *this, ipt_shared_in_list_node_t *n_ptr)
{
	if ( n_ptr == NULL ) return NULL;

	return ipt_op_drf(&n_ptr->next) == &this->sd_ptr->__null__ ? NULL : ipt_op_drf(&n_ptr->next);
} 

static void add_head(private_shared_in_list_t *this, ipt_shared_in_list_node_t *n_ptr)
{

	sem_wait(&this->sd_ptr->sem);

	if ( ipt_op_drf(&this->sd_ptr->head) == ipt_op_drf(&this->sd_ptr->head))
	{
		ipt_op_set(&this->sd_ptr->head, n_ptr);
		ipt_op_set(&n_ptr->prev, &this->sd_ptr->__null__);
		ipt_op_set(&n_ptr->next, &this->sd_ptr->__null__);
	}
	else
	{
		ipt_op_set(&((struct ipt_shared_in_list_node_t *)ipt_op_drf(&this->sd_ptr->head))->prev, n_ptr);
		ipt_op_set(&n_ptr->next,ipt_op_drf(&this->sd_ptr->head));
		ipt_op_set(&n_ptr->prev, &this->sd_ptr->__null__);
		ipt_op_set(&this->sd_ptr->head,n_ptr);
	}

	if ( ipt_op_drf(&this->sd_ptr->tail) == ipt_op_drf(&this->sd_ptr->tail ))
	{
		ipt_op_set(&this->sd_ptr->tail, n_ptr);
	}
	this->sd_ptr->count++;
	sem_post(&this->sd_ptr->sem);
	return;
	
}
static void add_tail(private_shared_in_list_t *this, ipt_shared_in_list_node_t *n_ptr)
{
	sem_wait(&this->sd_ptr->sem);

	if ( ipt_op_drf(&this->sd_ptr->tail) == &this->sd_ptr->__null__ )
	{
		ipt_op_set(&this->sd_ptr->tail, n_ptr);
		ipt_op_set(&n_ptr->prev, &this->sd_ptr->__null__);
		ipt_op_set(&n_ptr->next, &this->sd_ptr->__null__);
	}
	else
	{
		ipt_op_set(&((struct ipt_shared_in_list_node_t *)ipt_op_drf(&this->sd_ptr->tail))->next, n_ptr);
		ipt_op_set(&n_ptr->next,&this->sd_ptr->__null__);
		ipt_op_set(&n_ptr->prev, ipt_op_drf(&this->sd_ptr->tail));
		ipt_op_set(&this->sd_ptr->tail,n_ptr);
	}

	if ( ipt_op_drf(&this->sd_ptr->head) == &this->sd_ptr->__null__)
	{
		ipt_op_set(&this->sd_ptr->head, n_ptr);
	}
	this->sd_ptr->count++;
	sem_post(&this->sd_ptr->sem);
	return;
}

ipt_shared_in_list_t *ipt_shared_in_list_create(const char *name, ipt_allocator_t *alloc_ptr)
{

	private_shared_in_list_t *this;

	if ( alloc_ptr == NULL || !strlen(name) )
	{
		return NULL;
	}

	this = malloc(sizeof(private_shared_in_list_t));

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

	if ( (this->sd_ptr = (struct shared_data *) alloc_ptr->malloc(alloc_ptr, sizeof(struct shared_data))) == NULL )
	{
		free(this);
		return NULL;
	}

	/* set allocated memory to zero */
        memset(this->sd_ptr,0, sizeof(struct shared_data));

	if ( alloc_ptr->register_object(alloc_ptr, name, this->sd_ptr) < 0 )
	{
		free(this);
		alloc_ptr->free(alloc_ptr,this->sd_ptr);	
		return NULL;
	}

	if ( sem_init(&this->sd_ptr->sem, 1, 1) < 0 )
	{
		free(this);
		alloc_ptr->free(alloc_ptr,this->sd_ptr);
		return NULL;
	}

	ipt_op_set(&this->sd_ptr->head,&this->sd_ptr->__null__);
	ipt_op_set(&this->sd_ptr->tail,&this->sd_ptr->__null__);

	this->public.add_tail = (void (*)(ipt_shared_in_list_t *,ipt_shared_in_list_node_t *)) add_tail;
	this->public.add_head = (void (*)(ipt_shared_in_list_t *, ipt_shared_in_list_node_t *)) add_head;
	this->public.head = (ipt_shared_in_list_node_t * (*)(ipt_shared_in_list_t *)) head;
	this->public.tail = (ipt_shared_in_list_node_t * (*)(ipt_shared_in_list_t *)) tail;
	this->public.next = (ipt_shared_in_list_node_t * (*)(ipt_shared_in_list_t *, ipt_shared_in_list_node_t *)) next;
	this->public.remove = (void (*)(ipt_shared_in_list_t *, ipt_shared_in_list_node_t *)) private_remove;
	this->public.count = (size_t (*)(ipt_shared_in_list_t *)) count;

	this->alloc_ptr = alloc_ptr;

	return (ipt_shared_in_list_t *) this;
}

ipt_shared_in_list_t *ipt_shared_in_list_attach(const char *name, ipt_allocator_t *alloc_ptr)
{

	private_shared_in_list_t *this;

	if ( alloc_ptr == NULL || !strlen(name)  || strlen(name) > 128 )
	{
		return NULL;
	}

	this = malloc(sizeof(private_shared_in_list_t));

	if ( this == NULL )
	{
		return NULL;
	}	
	
	/* Attempt to find ourselves first */
	if ( (this->sd_ptr = alloc_ptr->find_registered_object(alloc_ptr,name) ) == NULL )
	{
		return NULL;
	}

	this->public.add_tail = (void (*)(ipt_shared_in_list_t *, ipt_shared_in_list_node_t *)) add_tail;
	this->public.add_head = (void (*)(ipt_shared_in_list_t *, ipt_shared_in_list_node_t *)) add_head;
	this->public.head = (ipt_shared_in_list_node_t * (*)(ipt_shared_in_list_t *)) head;
	this->public.tail = (ipt_shared_in_list_node_t * (*)(ipt_shared_in_list_t *)) tail;
	this->public.next = (ipt_shared_in_list_node_t * (*)(ipt_shared_in_list_t *, ipt_shared_in_list_node_t *)) next;
	this->public.remove = (void (*)(ipt_shared_in_list_t *, ipt_shared_in_list_node_t *)) private_remove;
	this->public.count = (size_t (*)(ipt_shared_in_list_t *)) count;

	this->alloc_ptr = alloc_ptr;

	strcpy(this->name, name );

	return (ipt_shared_in_list_t *) this;
}

int ipt_shared_in_list_destroy(ipt_shared_in_list_t *this)
{
	/* this does not delete the elements in the list */
	ipt_shared_in_list_node_t *n_ptr;

	while ( (n_ptr = this->head(this)) )
	{
		 this->remove( this, n_ptr );
	}

	/* Deregister from the  allocator */
	((private_shared_in_list_t *)this)->alloc_ptr->deregister_object( 
	     ((private_shared_in_list_t *)this)->alloc_ptr, ((private_shared_in_list_t*)this)->name );

	((private_shared_in_list_t *)this)->alloc_ptr->free( 
		((private_shared_in_list_t *)this)->alloc_ptr,
		((private_shared_in_list_t *)this)->sd_ptr );

	return 0;
}

ipt_shared_in_list_node_t *
ipt_shared_in_list_find(ipt_shared_in_list_t *this, int (*compare)(ipt_shared_in_list_node_t *, void *in_ptr), void *in_ptr)
{

	private_shared_in_list_t *_this = (private_shared_in_list_t *) this;

	struct ipt_shared_in_list_node_t *n_ptr;


	for ( 	n_ptr  = (ipt_shared_in_list_node_t *)ipt_op_drf(&_this->sd_ptr->head);
		n_ptr != (ipt_shared_in_list_node_t *)&_this->sd_ptr->__null__;
		n_ptr  = (ipt_shared_in_list_node_t *)ipt_op_drf(&n_ptr->next) )
	{

		if ( (*compare)(n_ptr, in_ptr) == 0 )
		{
			return n_ptr;	
		}
		
	}

	return NULL;
}

void
ipt_shared_in_list_for_each(ipt_shared_in_list_t *this, void (*functor)(const ipt_shared_in_list_node_t *const n_ptr, void *in_ptr), void *in_ptr)
{
	private_shared_in_list_t *_this = (private_shared_in_list_t *) this;

	struct ipt_shared_in_list_node_t *n_ptr;

	for ( 	n_ptr  = (ipt_shared_in_list_node_t *)ipt_op_drf(&_this->sd_ptr->head);
		n_ptr != (ipt_shared_in_list_node_t *)&_this->sd_ptr->__null__;
		n_ptr  = (ipt_shared_in_list_node_t *)ipt_op_drf(&n_ptr->next) )
	{
		(*functor)(n_ptr, in_ptr);
	}

	return;
}
