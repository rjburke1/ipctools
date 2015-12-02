#include <semaphore.h>
#include <stdio.h>
int main (int argc, char *argv)
{
sem_t sem;
sem_init(&sem,1,1);

printf("%i\n",sem_wait(&sem));
printf("%i\n",sem_wait(&sem));

return 0;
}
