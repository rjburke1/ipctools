#ifndef __IPCTOOLS_SHARED_LIST_H__
#define __IPCTOOLS_SHARED_LIST_H__

#include "allocator.h" 
#include "offset_ptr.h"


/**
 * typedef for shared_in_list structure
 */
typedef struct ipt_shared_in_list_t ipt_shared_in_list_t;
/**
 * typedef for shared_in_list node structure
 */
typedef struct ipt_shared_in_list_node_t ipt_shared_in_list_node_t;

/**
 * @struct ipt_shared_in_list_node_t
 *
 * @brief This structure defines a node in the shared list.
 *
 * The shared lists linkage and content is stored in memory
 * managed by the allocator.
 *
 * The shared list uses an intrusive list structure, so all items
 * must be of the form
 *
 *  struct {
 *    ipt_shared_in_list_node_t node;
 *    data
 * };
 */
struct ipt_shared_in_list_node_t
{
	/** previous pointer */
	ipt_op_t prev;
	/** next pointer */
	ipt_op_t next;
};

/**
 * @struct ipt_shared_in_list_t
 *
 * @brief This structure defines a shared list.
 *
 * The shared lists linkage and content is stored in memory
 * managed by the allocator.
 *
 * The shared list uses an intrusive list structure, so all items
 * must be of the form
 *
 *  struct {
 *    ipt_shared_in_list_node_t node;
 *    data
 * };
 */
struct ipt_shared_in_list_t
{
       /**
         * Add an entry to the tail of the list. 
         *
         * @param[in] this The this pointer.
         * @param[in] n_ptr The item to be added.
         *
         */
	void (*add_tail)(ipt_shared_in_list_t *this, ipt_shared_in_list_node_t *n_ptr);

       /**
         * Add an entry to the head of the list.
         *
         * @param[in] this The this pointer.
         * @param[in] n_ptr The item to be added.
         *
         */
	void (*add_head)(ipt_shared_in_list_t *this, ipt_shared_in_list_node_t *n_ptr);

       /**
         * Add an entry to the after the element.
         *
         * @param[in] this The this pointer.
         * @param[in] n_ptr The item to be added.
         *
         */
	int (*insert)(ipt_shared_in_list_t *this, ipt_shared_in_list_node_t  *n_ptr);

       /**
         * Return the total number of elements in the list.
         *
         * @param[in] this The this pointer.
         *
         * @retval size_t The number of elements in the list.
         */
	size_t (*count)(ipt_shared_in_list_t *this);

       /**
         * Remove the element from the list. 
         *
         * @param[in] this The this pointer.
         * @param[in] n_ptr The pointer to the element to be removed.
         *
         */
	void (*remove)(ipt_shared_in_list_t *this, ipt_shared_in_list_node_t *n_ptr);

       /**
         * Return the head of the list. 
         *
         * @param[in] this The this pointer.
         *
         * @retval NULL The list is empty
         * @retval !NULL The first element in the list. 
         */
	ipt_shared_in_list_node_t * (*head)(ipt_shared_in_list_t *this);

       /**
         * Return the tail of the list. 
         *
         * @param[in] this The this pointer.
         *
         * @retval NULL The list is empty
         * @retval !NULL The last element in the list.
         */
	ipt_shared_in_list_node_t * (*tail)(ipt_shared_in_list_t *this);

       /**
         * Return the next element in the list. 
         *
         * @param[in] this The this pointer.
         * @param[in] n_ptr The pointer to the current element in the list.
         *
         * @retval NULL The list is empty
         * @retval !NULL The next element in the list.
         */
	ipt_shared_in_list_node_t * (*next)(ipt_shared_in_list_t *this, ipt_shared_in_list_node_t *n_ptr);
};
/** 
 * The shared list constructor.
 *
 * @param[in] name The this pointer.
 * @param[in] alloc_ptr The allocator that will be used by the constructor.
 *
 * @retval NULL The constructor failed.
 * @retval !NULL The pointer to the newly created object.
 */
ipt_shared_in_list_t *ipt_shared_in_list_create(const char *name, ipt_allocator_t *alloc_ptr);

/** 
 * The shared list constructor.
 *
 * @param[in] name The this pointer.
 * @param[in] alloc_ptr The allocator that will be used by the constructor.
 *
 * @retval NULL The constructor failed.
 * @retval !NULL The pointer to the newly created object.
 */
ipt_shared_in_list_t *ipt_shared_in_list_attach(const char *name, ipt_allocator_t *alloc_ptr);

/**
 * The shared list destructor.
 *
 * @param[in] this The this pointer.
 *
 * @retval 0 Success.
 * @retval 1 Failure
 */
int ipt_shared_in_list_destroy(ipt_shared_in_list_t *this);

/**
 * Find an entry in the shared list.
 *
 * @param[in] this The this pointer.
 * @param[in] func A callback.
 * @param[in] in_ptr A pointer to an object that will be passed through to the callback..
 *
 * @retval !NULL A pointer to the entry.
 * @retval NULL Failure
 */
ipt_shared_in_list_node_t * ipt_shared_in_list_find(ipt_shared_in_list_t *this, int (*func)(ipt_shared_in_list_node_t *, void *), void *in_ptr);

/**
 * Walk the list of entries and execute a callback.
 *
 * @param[in] this The this pointer.
 * @param[in] func A callback.
 * @param[in] in_ptr A pointer to an object that will be passed through to the callback..
 *
 */
void ipt_shared_in_list_for_each(ipt_shared_in_list_t *this, void (*func)(const ipt_shared_in_list_node_t *const, void *), void *in_ptr);


#endif
