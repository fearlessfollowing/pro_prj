#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

pthread_key_t key;

struct test_struct {
    int i;
    float k;
};

void *child1(void* arg)
{
    struct test_struct struct_data;
    struct_data.i = 10;
    struct_data.k = 3.14f;

    pthread_setspecific(key, &struct_data);

    printf("address of struct_data: 0x%p\n", &struct_data);
    printf("in child1, pthread_getspecific(key) pointer: 0x%p\n", (struct test_struct*)pthread_getspecific(key));
    printf("struct_data.i = %d, struct_data.k = %f\n", ((struct test_struct*)pthread_getspecific(key))->i, 
                                                        ((struct test_struct*)pthread_getspecific(key))->k);
}


void *child2(void* arg)
{
    int temp = 20;
    sleep(2);
    pthread_setspecific(key, &temp);

    printf("address of temp: 0x%p\n", &temp);
    printf("in child2, pthread_getspecific(key) pointer: 0x%p\n", (int*)pthread_getspecific(key));
    printf("struct_data.i = %d\n", *((int*)pthread_getspecific(key)));
}

int main(void)
{
    pthread_t tid1, tid2;
    pthread_key_create(&key, NULL);

    pthread_create(&tid1, NULL, (void*)child1, NULL);
    pthread_create(&tid2, NULL, (void*)child2, NULL);

    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);

    pthread_key_delete(key);

    return 0;
}