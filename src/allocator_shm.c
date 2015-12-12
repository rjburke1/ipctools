#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <stdio.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "allocator_shm.h"
#include "offset_ptr.h"
#include  <sys/file.h>

/**
 * \defgroup Private_SharedMemory Internal data structures used by the shared memory allocator (allocator_shm)
 * @{ 
 */

/**
 * @struct __node__
 *
 * @brief Internal node structure used by the allocator.
 */
struct __node__
{
	/** previous node pointer. */
	ipt_op_t prev;

	/** next node pointer.  */
	ipt_op_t next;

	/** size of this node.  */
	size_t size;
};

/**
 *
 *  Internal registration object structure used by the allocator.
 */
struct reg_obj
{
	/**
	 * Internal node.
	 */
	struct __node__ node;	

	/**
	 * Pointer ot request object.
	 */
	ipt_op_t item;	

	/**
	 * The name of the request object.
	 */
	ipt_op_t name;
};

/**
 * @struct shared_data
 *
 * @brief Shared data structure used by the allocator.
 */
struct shared_data
{
	/**
         * Offset to free list head.
         */
	ipt_op_t free_list_head;

	/**
         * Offset to free list tail.
         */
	ipt_op_t free_list_tail;

	/**
         * Offset to registered object list head.
         */
	ipt_op_t ro_list_head;

	/**
         * Offset to registered object list tail.
         */
	ipt_op_t ro_list_tail;

	/**
         * Bytes allocated.
         */
	size_t bytes_allocated;

	/**
         * Blocks allocated.
         */
	size_t num_blocks_allocated;

	/**
         * Size of memory block used by allocator.
         */
        size_t size;

	/**
         * used as null pointer.
         */
	char __null__;

	/** 
         * Semaphore.
         */
	sem_t sem;
};

typedef struct private_allocator_t private_allocator_t;

/**
 * @struct private_allocator_t
 *
 * @brief Private class that is allocated off a processes's heap and provides
 *        local access to the process manager.
 */
struct private_allocator_t
{
	/** public interface */
	ipt_allocator_t public;

	/** shared data */
	struct shared_data *sd_ptr;
};

/** @} */

static size_t 
blocks_allocated(private_allocator_t *this)
{
	return this->sd_ptr->num_blocks_allocated;
}

static size_t 
bytes_allocated(private_allocator_t *this)
{
	 return this->sd_ptr->bytes_allocated;
}

static void print_registered_object(struct reg_obj *ptr)
{
                printf("Registered Object[name:%s]\n",(char*)ipt_op_drf(&ptr->name));
}
static void print_free_block(struct __node__ *ptr)
{
		printf("Block[begin:%p, end:%p, prev:%p, next:%p size:%zu]\n",ptr, 
			ipt_add_offset((char *)ptr,ptr->size), 
			ipt_op_drf(&ptr->prev),
			ipt_op_drf(&ptr->next),
			ptr->size);
}

static void walk_ro_list(private_allocator_t *this, void (*fnc)(struct reg_obj *) )
{
	struct __node__ *cur_ptr;

	sem_wait(&this->sd_ptr->sem);

	for ( 	cur_ptr = ( struct __node__ *) ipt_op_drf(&this->sd_ptr->ro_list_head);
		cur_ptr != (struct __node__ *) &this->sd_ptr->__null__;
		cur_ptr = ( struct __node__ *) ipt_op_drf(&cur_ptr->next) )
	{
		(*fnc)((struct reg_obj *) cur_ptr);
	}

	sem_post(&this->sd_ptr->sem);
}

static void walk_free_list(private_allocator_t *this, void (*fnc)(struct __node__ *ptr))
{
	struct __node__ *cur_ptr;

	sem_wait(&this->sd_ptr->sem);

	for ( 	cur_ptr = ( struct __node__ *) ipt_op_drf(&this->sd_ptr->free_list_head);
		cur_ptr != (struct __node__ *) &this->sd_ptr->__null__; 
		cur_ptr = ( struct __node__ *) ipt_op_drf(&cur_ptr->next) )
	{
		(*fnc)(cur_ptr);
	}

	sem_post(&this->sd_ptr->sem);

	return;
}

static unsigned int ipt_allocator_overhead(void)
{
	return sizeof(struct shared_data) + sizeof(struct __node__);
}

static void * 
private_malloc(private_allocator_t *this, size_t size)
{
	struct __node__ *cur_ptr;

 	/* Align the block on 8 bytes */
        size = size % sizeof(ptrdiff_t)  == 0 ? size : size - size % sizeof(ptrdiff_t) + sizeof(ptrdiff_t) ;

	sem_wait(&this->sd_ptr->sem);

	/* Walked the free list and find a chunck big enough */
	for (   cur_ptr  = (struct __node__ *) ipt_op_drf(&this->sd_ptr->free_list_head); 
		cur_ptr != (struct __node__ *) &this->sd_ptr->__null__; 
		cur_ptr  = (struct __node__ *)ipt_op_drf(&cur_ptr->next) )
  	{
		if ( cur_ptr->size < sizeof(struct __node__) + size ) continue;

		/* Split the block by pulling chunk off bottom */	
		struct __node__ *n_ptr = (struct __node__ *) (ipt_add_offset((char *)cur_ptr, cur_ptr->size - size - sizeof( struct __node__ )));

		this->sd_ptr->bytes_allocated += size;

		this->sd_ptr->num_blocks_allocated++;

		cur_ptr->size -= (sizeof( struct __node__) + size);

		n_ptr->size = sizeof(struct __node__) + size;

      		/* Special Case where size matches exactly */
		if ( cur_ptr->size ==  sizeof(struct __node__) + size )
		{

			/*      addr a      <     addr b     <  addr c    
			 *  ---------------- ---------------- ----------------
			 * |                |                |                |
			 * |      next ---> |      next ---> |      next ---> |
			 * | <--- prev      | <--- prev      | <--- prev      |
			 * |                |                |                |
			 *  ---------------- ---------------- ----------------
			 *                         ^                 
			 *                      (cur_ptr)          
			 */
         		if ( ipt_op_drf(&cur_ptr->prev) != &this->sd_ptr->__null__ )
         		{
            			ipt_op_set(&((struct __node__ *)ipt_op_drf(&cur_ptr->prev))->next, ipt_op_drf(&cur_ptr->next));
         		}

         		if ( ipt_op_drf(&cur_ptr->next) != &this->sd_ptr->__null__)
         		{
            			ipt_op_set(&((struct __node__ *)ipt_op_drf(&cur_ptr->next))->prev, ipt_op_drf(&cur_ptr->prev)); 
         		}

			/* Update head and tail pointers  */
			if ( (struct __node__ *) ipt_op_drf(&this->sd_ptr->free_list_head) == cur_ptr )
			{
				ipt_op_set(&this->sd_ptr->free_list_head, ipt_op_drf(&cur_ptr->prev));
			}

			if ( (struct __node__ *) ipt_op_drf(&this->sd_ptr->free_list_tail) == cur_ptr )
			{
				ipt_op_set(&this->sd_ptr->free_list_tail, ipt_op_drf(&cur_ptr->next));
			}

		}
		sem_post(&this->sd_ptr->sem);
		return (void *)ipt_add_offset((char *)n_ptr, sizeof(struct __node__));
	}

	sem_post(&this->sd_ptr->sem);

	return NULL;
}
static void
private_free(private_allocator_t *this, void *ptr)
{
	sem_wait(&this->sd_ptr->sem);

	struct __node__ * n_ptr = ( struct __node__ *) ipt_sub_offset( ( char *)ptr,sizeof(struct __node__) );
	struct __node__ *cur_ptr;

	int tmp_size = n_ptr->size;

	for ( 	cur_ptr = ( struct __node__ *) ipt_op_drf(&this->sd_ptr->free_list_head);
         	cur_ptr !=  (struct __node__ *)&this->sd_ptr->__null__;
         	cur_ptr = ( struct __node__ *) ipt_op_drf(&cur_ptr->next) )
	{

		/* Check to skip block */	
		if ( ipt_op_drf(&cur_ptr->next)  != &this->sd_ptr->__null__  &&  cur_ptr  < n_ptr) 
		{
			continue;
		}

		if ( ipt_op_drf(&cur_ptr->prev) == &this->sd_ptr->__null__ &&  n_ptr <  cur_ptr )
		{
			/*      addr a      <     add b    
			 *  ---------------- ---------------
			 * |   (free node)  |                |
			 * |      next ---> |      next ---> |     
			 * | <--- prev      | <--- prev      | 
			 * |                |                |
			 *  ---------------- ----------------| 
			 *         ^                 ^
			 *      (n_ptr)          (cur_ptr)
			 *                        (head)
			 */
			ipt_op_set(&n_ptr->next,cur_ptr);
			ipt_op_set(&n_ptr->prev, &this->sd_ptr->__null__);
			ipt_op_set(&cur_ptr->prev,n_ptr);
			ipt_op_set(&this->sd_ptr->free_list_head, n_ptr);

			/* Attempt to coalesce */
			if ( (char *)cur_ptr == ipt_add_offset((char *)n_ptr,n_ptr->size) )
			{ /* Adjacent and coalesce */
				n_ptr->size += cur_ptr->size;
				ipt_op_set(&n_ptr->next,ipt_op_drf(&cur_ptr->next));
			}
			break;
		}	
		else if ( ipt_op_drf(&cur_ptr->next)  == &this->sd_ptr->__null__ &&  cur_ptr  < n_ptr)
		{
			/*      addr a      <     add b    
			 *  ---------------- ---------------
			 * |                |                |
			 * |      next ---> |      next ---> |     
			 * | <--- prev      | <--- prev      | 
			 * |                |                |
			 *  ---------------- ----------------| 
			 *         ^                 ^
			 *      (cur_ptr)         (n_ptr)
			 *       (tail)
			 */
			ipt_op_set(&n_ptr->prev,cur_ptr);
			ipt_op_set(&n_ptr->next, &this->sd_ptr->__null__);
			ipt_op_set(&cur_ptr->next, n_ptr);
			ipt_op_set(&this->sd_ptr->free_list_tail, n_ptr);

			/* Attempt to coalesce */
			if ( ipt_add_offset((char *)cur_ptr,cur_ptr->size)  == (char *)n_ptr  )
			{ /* Adjacent and coalesce */
				cur_ptr->size += n_ptr->size;
				ipt_op_set(&cur_ptr->next,ipt_op_drf(&n_ptr->next));
			}
			break;
		}
		else
		{
			/*      addr a      <     addr b     <  addr c    
			 *  ---------------- ---------------- ----------------
			 * |                |   (free node)  |                |
			 * |      next ---> |      next ---> |      next ---> |
			 * | <--- prev      | <--- prev      | <--- prev      |
			 * |                |                |                |
			 *  ---------------- ---------------- ----------------
			 *                         ^                 ^
			 *                      (n_ptr)          (cur_ptr)
			 */
			ipt_op_set( &((struct __node__ *)ipt_op_drf(&cur_ptr->prev))->next, n_ptr);
			ipt_op_set(&n_ptr->next,cur_ptr);
			ipt_op_set(&n_ptr->prev, (void *)ipt_op_drf(&cur_ptr->prev));
			ipt_op_set(&cur_ptr->prev,n_ptr);

			/* Attempt to coalesce 
			 * free node is adjacent to it's next node ( i.e current node ).
			 */
			if ( ipt_add_offset((char*)n_ptr,n_ptr->size) == (char*)cur_ptr)
			{
				n_ptr->size += cur_ptr->size;
				ipt_op_set(&n_ptr->next, ipt_op_drf(&cur_ptr->next));
				cur_ptr = n_ptr;
			}

			/* Attempt to coalesce
			 * free node is adjacent to it's previous node.
			 */
			if ( ipt_add_offset((char*)ipt_op_drf(&n_ptr->prev),((struct __node__ *)ipt_op_drf(&n_ptr->prev))->size) 
				== (char*)n_ptr)
			{
				/* Scenario where the the current node and free node have been coalesced already */
				if ( cur_ptr == n_ptr )
				{
					((struct __node__*)ipt_op_drf(&n_ptr->prev))->size += n_ptr->size;
					ipt_op_set(&((struct __node__ *)ipt_op_drf(&n_ptr->prev))->next, ipt_op_drf(&n_ptr->next));
					cur_ptr = ((struct __node__ *)ipt_op_drf(&n_ptr->prev));
				}
				else
				{
					/* Scenario where the free node has not been coalesced with the current pointer */
					((struct __node__*)ipt_op_drf(&n_ptr->prev))->size += n_ptr->size;
					ipt_op_set(&((struct __node__ *)ipt_op_drf(&n_ptr->prev))->next, cur_ptr);
					ipt_op_set(&cur_ptr->prev, ipt_op_drf(&n_ptr->prev) );
				}
			}

         		break;
		}
   	}

	/* Handle the empty list scenario */
	if ( ipt_op_drf(&this->sd_ptr->free_list_head)  == &this->sd_ptr->__null__ )
	{
		ipt_op_set(&n_ptr->prev, &this->sd_ptr->__null__);
		ipt_op_set(&n_ptr->next, &this->sd_ptr->__null__);
		ipt_op_set(&this->sd_ptr->free_list_head, n_ptr);
		ipt_op_set(&this->sd_ptr->free_list_tail, n_ptr);
	}

	this->sd_ptr->num_blocks_allocated--;

	this->sd_ptr->bytes_allocated -=  (tmp_size - sizeof(struct __node__));

	sem_post(&this->sd_ptr->sem);

	return;	
}
static int
register_object(private_allocator_t *this, const char *name, void *ptr)
{
	struct __node__  *n_ptr;
	char *name_ptr;

	/* validate inputs and make sure not already registered */
	if ( !name || !ptr || this->public.find_registered_object((ipt_allocator_t*)this,name) )
	{
		return 1;
	}
		
	/* check allocations */
	if ( (n_ptr = (struct __node__ * ) this->public.malloc(&this->public,sizeof(struct reg_obj)) ) == NULL  ||
	     (name_ptr = (char * ) this->public.malloc(&this->public,strlen(name) + 1) ) == NULL )
	{
		return 1;
	}

	sem_wait(&this->sd_ptr->sem);

	strcpy(name_ptr,name);

	/* Update the data */
	ipt_op_set(&(( struct reg_obj *)n_ptr)->item, ptr);
	ipt_op_set(&(( struct reg_obj *)n_ptr)->name, (void *)name_ptr);

	/* From here on in, update the linkage */

	/* set the head */
	if ( ipt_op_drf(&this->sd_ptr->ro_list_head) == &this->sd_ptr->__null__ ) 
        {
		ipt_op_set(&this->sd_ptr->ro_list_head,n_ptr);
        }

	/* add to the tail of the list*/
	ipt_op_set(&n_ptr->prev, ipt_op_drf(&this->sd_ptr->ro_list_tail));
	ipt_op_set(&n_ptr->next,&this->sd_ptr->__null__);

	ipt_op_set(&(( struct reg_obj *)n_ptr)->item, ptr);
	ipt_op_set(&(( struct reg_obj *)n_ptr)->name, (void *)name_ptr);

	if ( ipt_op_drf(&this->sd_ptr->ro_list_tail) != &this->sd_ptr->__null__)
	{
		ipt_op_set( &((struct __node__ *)ipt_op_drf(&this->sd_ptr->ro_list_tail))->next,n_ptr);
	}

	if ( ipt_op_drf(&this->sd_ptr->ro_list_head) == &this->sd_ptr->__null__)
	{
		ipt_op_set(&this->sd_ptr->ro_list_head,n_ptr);
	}

	ipt_op_set(&this->sd_ptr->ro_list_tail,n_ptr);

	sem_post(&this->sd_ptr->sem);

	return 0;
	
}
static void *
deregister_object(private_allocator_t *this, const char *name)
{
        struct __node__ *cur_ptr;
        void * item;

	sem_wait(&this->sd_ptr->sem);

        for (   cur_ptr = ( struct __node__ *) ipt_op_drf(&this->sd_ptr->ro_list_head);
                cur_ptr !=  (struct __node__ *) &this->sd_ptr->__null__;
                cur_ptr = ( struct __node__ *) ipt_op_drf(&cur_ptr->next) )
        {
                struct reg_obj *r_ptr = (struct reg_obj *)cur_ptr;
                if ( !strcmp(name, (char *) ipt_op_drf( &((struct reg_obj *)cur_ptr)->name) ) )
                {
                        break;
                }
        }

        if ( cur_ptr == (struct __node__ *)&this->sd_ptr->__null__ )
        {
                return NULL;
        }

        /* Update the linkage */
        if ( ipt_op_drf(&cur_ptr->prev) != &this->sd_ptr->__null__)
        {
                ipt_op_set( &((struct __node__ *)ipt_op_drf(&cur_ptr->prev))->next, ipt_op_drf(&cur_ptr->next) );
        }
        if ( ipt_op_drf(&cur_ptr->next) != &this->sd_ptr->__null__)
        {
                ipt_op_set( &((struct __node__ *)ipt_op_drf(&cur_ptr->next))->prev, ipt_op_drf(&cur_ptr->prev));
        }
	if ( ipt_op_drf(&cur_ptr->prev) == &this->sd_ptr->__null__ && ipt_op_drf(&cur_ptr->next) == &this->sd_ptr->__null__)
        { /* only a single element. Set the head and tail to null. */
		ipt_op_set(&this->sd_ptr->ro_list_head,&this->sd_ptr->__null__);
		ipt_op_set(&this->sd_ptr->ro_list_tail,&this->sd_ptr->__null__);
        }

	sem_post(&this->sd_ptr->sem);

        /* free the name */
        private_free(this,  (void *)ipt_op_drf( &((struct reg_obj *)cur_ptr)->name ) );

        /* free the registration object */
        private_free(this, (void *)cur_ptr );

        /* return the item. this is not free'd because the caller allocated it */
        return ipt_op_drf( &((struct reg_obj *)cur_ptr)->item );
}

static void * 
find_registered_object(private_allocator_t *this, const char *name)
{
	struct __node__ *cur_ptr;

	sem_wait(&this->sd_ptr->sem);

        for (   cur_ptr = ( struct __node__ *) ipt_op_drf(&this->sd_ptr->ro_list_head);
                cur_ptr !=  (struct __node__ *) &this->sd_ptr->__null__;
                cur_ptr = ( struct __node__ *) ipt_op_drf(&cur_ptr->next) )
        {
		struct reg_obj *r_ptr = (struct reg_obj *)cur_ptr;
		if ( !strcmp(name, (char *) ipt_op_drf( &((struct reg_obj *)cur_ptr)->name) ) )
		{
			sem_post(&this->sd_ptr->sem);
			return (void *) ipt_op_drf( & (( struct reg_obj *)cur_ptr)->item);
		}
        }

	sem_post(&this->sd_ptr->sem);

	return NULL;
}

static void
dump_stats(private_allocator_t *this)
{
	struct __node__ *cur_ptr;
	size_t remaining;
        size_t overhead;

	overhead = sizeof(struct __node__) * (this->sd_ptr->num_blocks_allocated);

	remaining =  this->sd_ptr->size - this->sd_ptr->bytes_allocated - overhead;

	fprintf(stdout,"Allocator[\n");
	fprintf(stdout,"\tsize = %zu\n",this->sd_ptr->size);
	fprintf(stdout,"\tbytes allocated = %zu\n",this->sd_ptr->bytes_allocated);
	fprintf(stdout,"\tblocks allocated = %zu\n",this->sd_ptr->num_blocks_allocated);
	fprintf(stdout,"\tbytes remaining = %zu\n",remaining <= 0 ? 0 : remaining);
	fprintf(stdout,"\toverhead/block = %zu bytes\n",sizeof(struct __node__));
	fprintf(stdout,"\tefficiency          = %2.2f \n", (1 - overhead*1.0/this->sd_ptr->size)*100);
	
	fprintf(stdout,"Blocks on Free List ... \n");
	walk_free_list(this, print_free_block);
	fprintf(stdout,"Blocks finished\n");

	fprintf(stdout,"Registered Objects ... \n");
	walk_ro_list(this, print_registered_object);
	fprintf(stdout,"Registered Objects finished\n");
	return;	
}
static size_t 
 get_size(private_allocator_t *this)
{
	sem_wait(&this->sd_ptr->sem);

	int overhead =  this->sd_ptr->size ;

	sem_post(&this->sd_ptr->sem);

	return overhead;
}

static void
destroy(ipt_allocator_t *this)
{
	/* RJB : TODO : Need to reference count this to destroy. */
	if ( this != NULL ) 
	{
		shmdt( ((private_allocator_t * ) this)->sd_ptr );
		free(this);
	}

	return;
}

static void * get_shared_ptr(private_allocator_t *this)
{
        return (void *)this->sd_ptr;
}

ipt_allocator_t * ipt_allocator_shm_create(size_t size, ipt_allocator_shm_key_t id)
{
int shmid;
void *base_address;

       	if ( (shmid = shmget(id, size + sizeof(struct shared_data), IPC_CREAT | 0777) ) < 0  )
       	{
               	return NULL;
       	}

      	if ( (base_address = shmat(shmid,(void *)0, 0)) == (void *) -1)

       	{
               	return NULL;
       	}

	private_allocator_t *this = malloc(sizeof(private_allocator_t));

	if ( this == NULL )
	{
		return NULL;
	}

	this->sd_ptr = (struct shared_data *) base_address;

	memset((char*)this->sd_ptr, 0, size + sizeof(struct shared_data));

	if ( sem_init(&this->sd_ptr->sem, 1, 1) < 0 )
	{
		shmdt(base_address);
		return NULL;
	}

        /* Initialize data. The node is subtraced here because it is required for each allocation. */
        this->sd_ptr->size = size;

        /* Assign public interface */
        this->public.malloc = (void * (*)(ipt_allocator_t *, size_t) ) private_malloc;
        this->public.free   = (void (*)(ipt_allocator_t *, void *) ) private_free;
        this->public.dump_stats = (void (*)(ipt_allocator_t *) ) dump_stats;
        this->public.blocks_allocated = (size_t (*)(ipt_allocator_t *) ) blocks_allocated;
        this->public.bytes_allocated = (size_t (*)(ipt_allocator_t *) ) bytes_allocated;
        this->public.register_object = (int (*)(ipt_allocator_t *, const char *name, void *) ) register_object;
        this->public.find_registered_object = (void *(*)(ipt_allocator_t *, const char *) ) find_registered_object;
        this->public.deregister_object = (void * (*)(ipt_allocator_t *, const char *) ) deregister_object;
        this->public.get_size = (size_t (*)(ipt_allocator_t *) ) get_size;
	this->public.destroy = (void (*)(ipt_allocator_t*) ) destroy;
        this->public.get_shared_ptr = (void * (*)(ipt_allocator_t*) ) get_shared_ptr;


        /* Start the free list after the private_allocator_t */
        ipt_op_set(&this->sd_ptr->free_list_head, ipt_add_offset((char *)this->sd_ptr,sizeof(struct shared_data)));

        /* Start the free list after the private_allocator_t */
        ipt_op_set(&this->sd_ptr->free_list_tail, ipt_add_offset((char *)this->sd_ptr,sizeof(struct shared_data)));

 	/* Set the node structure */
        struct __node__ *n_ptr  = (struct __node__ *) ipt_op_drf(&this->sd_ptr->free_list_head);
    
        /* Initialize the node structure */
        ipt_op_set(&n_ptr->prev, &this->sd_ptr->__null__);
        ipt_op_set(&n_ptr->next, &this->sd_ptr->__null__);

        /* Shared data is added to original malloc. Node struct is counted as overhead in free block
         *       -----------------------------------------------------
         *       |              |          |                         |
         *       | shared data  | __node__ |       remainder         |
         *       |              |          |                         |
         *       -----------------------------------------------------
         *                      <------------- free bock ------------>
         *       <------------- orginal malloced block -------------->
         */
        /* ignore the node structure and count it as part of free block */
        n_ptr->size = size;

	/* Set the registered object list to null */
	ipt_op_set(&this->sd_ptr->ro_list_head,&this->sd_ptr->__null__);
	ipt_op_set(&this->sd_ptr->ro_list_tail,&this->sd_ptr->__null__);

	return (ipt_allocator_t *) this;
}

ipt_allocator_t * ipt_allocator_shm_attach(ipt_allocator_shm_key_t key)
{
int shmid;
void *base_address;

	if ( (shmid = shmget(key, 0, 0777) ) < 0  && errno != EEXIST)
        {
                return NULL;
        }       

       	if ( (base_address = shmat(shmid,(void *)0, 0)) == (void *) -1)
       	{
               	return NULL;
       	}
	private_allocator_t *this = malloc(sizeof(private_allocator_t));

	if ( this == NULL )
	{
		return NULL;
	}

	this->sd_ptr = (struct shared_data *) base_address;

        /* Assign public interface */
        this->public.malloc = (void * (*)(ipt_allocator_t *, size_t) ) private_malloc;
        this->public.free   = (void (*)(ipt_allocator_t *, void *) ) private_free;
        this->public.dump_stats = (void (*)(ipt_allocator_t *) ) dump_stats;
        this->public.blocks_allocated = (size_t (*)(ipt_allocator_t *) ) blocks_allocated;
        this->public.bytes_allocated = (size_t (*)(ipt_allocator_t *) ) bytes_allocated;
        this->public.register_object = (int (*)(ipt_allocator_t *, const char *name, void *) ) register_object;
        this->public.find_registered_object = (void *(*)(ipt_allocator_t *, const char *) ) find_registered_object;
        this->public.deregister_object = (void * (*)(ipt_allocator_t *, const char *) ) deregister_object;
        this->public.get_size = (size_t (*)(ipt_allocator_t *) ) get_size;
	this->public.destroy = (void (*)(ipt_allocator_t*) ) destroy;
        this->public.get_shared_ptr = (void * (*)(ipt_allocator_t*) ) get_shared_ptr;


	return (ipt_allocator_t *) this;
}
/** @} */
