#ifndef __IPCTOOLS_SHARED_QUEUE_H__
#define __IPCTOOLS_SHARED_QUEUE_H__

#include "allocator_shm.h"
#include "shared_in_list.h"

#include "event_handler.h"


/**
 * typedef for the shared queue node.
 */
typedef ipt_shared_in_list_node_t ipt_shared_queue_node_t;

/**
 * typdef for the shared queue node.
 */
typedef struct ipt_shared_queue_t ipt_shared_queue_t;

/**
 * @struct ipt_shared_queue_t
 *
 * @brief This structure defines a queue in memory.  
 *
 * The shared queue's linkage and content is stored in memory
 * managed by the allocator.
 *
 * The shared queue uses an intrusive list structure, so all items
 * must be of the form 
 *
 *  struct {
 *    ipt_shared_queue_node_t node;
 *    data 
 * };
 */
struct ipt_shared_queue_t
{
       /**
         * Get the file descriptor used to notify that elements have been added.
         * This file descriptor can be added to the reactor
         *
         * @param[in] this The this pointer.
         *
         * @retval > The file descriptor.
         * @retval -1  Failed.
         */
	int (*get_fd)(ipt_shared_queue_t *this);

       /**
         * Add an item to the tail of the queue. The shared queue uses the intrusive list
         * so the item must have the shared_queue_node at the top of the structure.
         *  
         * This file descriptor can be added to the reactor
         *
         * @param[in] this The this pointer.
         * @param[in] ptr The pointer to the item.
         *
         * @retval > The file descriptor.
         * @retval -1  Failed.
         */
	void (*enqueue)(ipt_shared_queue_t *this, ipt_shared_queue_node_t *ptr);

       /**
         * Remove an item from the top of the queue. The shared queue uses the intrusive list
         * so the item must have the shared_queue_node at the top of the structure.
         *
         * This will wait forever.  
         *
         * @param[in] this The this pointer.
         *
         * @retval !NULL The item removed from the queue.
         * @retval NULL  Failed.
         */
	ipt_shared_queue_node_t * (*dequeue)(ipt_shared_queue_t *this);

       /**
         * Remove an item from the top of the queue. The shared queue uses the intrusive list
         * so the item must have the shared_queue_node at the top of the structure. 
         *
         * The dequeue will wait for the specified time.
         *
         * @param[in] this The this pointer.
         * @param[in] tv The time to wait for an entry to be enqueued
         *
         * @retval !NULL The item removed from the queue.
         * @retval NULL  Failed.
         */
	ipt_shared_queue_node_t * (*dequeue_timed)(ipt_shared_queue_t *this, const ipt_time_value_t *tv);


       /**
         * Dump the queue statistics.
         *
         * @param[in] this The this pointer.
         *
         */
	void (*dump_stats)(ipt_shared_queue_t *this);
};


/**
 * The shared queue constructor.
 *
 * @param[in] name The this pointer.
 * @param[in] alloc_ptr The allocator
 *
 * @retval !NULL The shared queue.
 * @retval NULL  Failed.
 */
ipt_shared_queue_t * ipt_shared_queue_create(const char *name, ipt_allocator_t *alloc_ptr);

/**
 * The shared queue constructor
 *
 * @param[in] name The this pointer.
 * @param[in] alloc_ptr The allocator
 *
 * @retval !NULL The shared queue.
 * @retval NULL  Failed.
 */
ipt_shared_queue_t * ipt_shared_queue_attach(const char *name, ipt_allocator_t *alloc_ptr);


/**
 * The shared queue destructor 
 *
 * @param[in] this The this pointer.
 *
 */
void ipt_shared_queue_destroy(ipt_shared_queue_t *this);

/**
 * Walk through the list of items and apply the callback. 
 *
 * @param[in] this The this pointer.
 * @param[in] func The callback.
 * @param[in] in_ptr The object passed through to the callback.
 *
 */
void ipt_shared_queue_for_each(ipt_shared_queue_t *this, void (*func)(const ipt_shared_queue_node_t *const , void *), void *in_ptr);


#endif
