#include <stdio.h>
int main(int argc, char *argv[])
{

int num = atol(argv[1]);

printf("%i %s\n",num, argv[1]);

int aligned = num % 8 == 0 ? num : num - num % 8 + 8;
printf ("%ld %ld\n", num, aligned);
return 0;
}
