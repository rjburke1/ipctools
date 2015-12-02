#include "offset_ptr.h"
#include <assert.h>
#include <stdio.h>

struct bob
{
	unsigned int a;
	unsigned int b;
	ipt_op_t ptr;
};

int main(int argc, char *argv[])
{
	struct bob b = {1,2};

	ipt_op_set(&b.ptr,&b.a);

	assert ( *((unsigned int *)ipt_op_drf(&b.ptr)) == 1 );

	printf(" %s completed successfully\n", argv[0]);

	return 0;
}
