#include "allocator_shm.h"
#include "shared_in_list.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "config.h"

struct my_struct
{
	ipt_shared_in_list_node_t node;
	unsigned int a;
	unsigned int b;
	char name[256];
};

int main (int argc, char *argv[])
{
	char *ptr;

	/* Create an allocator */
	ipt_allocator_t *alloc_ptr = ipt_allocator_shm_create( 1024 * 1024, IPT_TEST_ALLOCATOR_SHM_KEY );

	if ( alloc_ptr == NULL )
	{
		printf("Failed to create the allocator.\n");
		return -1;
	}

	
	/* Create the shared list */	
	ipt_shared_in_list_t *sl_ptr = ipt_shared_in_list_create("My Shared Intrusive List", alloc_ptr);


	if ( sl_ptr == NULL )
	{
		printf("Failed to creat the shared instrusive list\n");
		return -1;
	}

	/* Check to see if the object has been registed with the allocator */
	if ( ( ptr = alloc_ptr->find_registered_object(alloc_ptr,"My Shared Intrusive List")) == NULL )
	{
		printf("Failed to find the registed shared list in the allocator\n");
		return -1;
	}

	/* Must use the same allocator for both */
	struct my_struct *m_ptr = alloc_ptr->malloc( alloc_ptr, sizeof(struct my_struct) );

	if ( m_ptr == NULL )
	{
		printf("Failed to allocate initial test structure\n");	
		return -1;
	}

	m_ptr->a = 1;
	m_ptr->b = 2;

	strcpy(m_ptr->name,"My Test Object");

	sl_ptr->add_tail( sl_ptr, (ipt_shared_in_list_node_t *) m_ptr ); 

	m_ptr = (struct my_struct *) sl_ptr->head( sl_ptr );

	if ( m_ptr->a != 1 )
	{
		printf("add_tail and head methods failed to validate.\n");
		return -1;
	}

	/* 
	 * Now move the shared list and verify that everything still works 
	 */

	char *dst_ptr = malloc(alloc_ptr->get_size(alloc_ptr)*2);
        void *sd_ptr;

	if ( ( sd_ptr = alloc_ptr->get_shared_ptr(alloc_ptr) ) == NULL )
        {
	      printf("Failed to access the shared_ptr\n");
	      return -1;
        }

	memmove(dst_ptr,sd_ptr, alloc_ptr->get_size(alloc_ptr) );

        alloc_ptr =  ipt_allocator_malloc_attach(dst_ptr);

	/* Check to see if the object has been registed with the allocator */
	if ( ( ptr = alloc_ptr->find_registered_object(alloc_ptr,"My Shared Intrusive List")) == NULL )
	{
		printf("Failed to find the registed shared list in the allocator\n");
		return -1;
	}

	/* attached to memory */	
	sl_ptr = ipt_shared_in_list_attach("My Shared Intrusive List", alloc_ptr);


	m_ptr = (struct my_struct *) sl_ptr->head(sl_ptr);

	if ( m_ptr->a != 1  || strcmp(m_ptr->name,"My Test Object") )
	{
		printf("memmove test failed. Relative pointers must not be working\n");
		return -1;
	}	

        printf(" %s completed successfully\n", argv[0]);

	return 0;
}
