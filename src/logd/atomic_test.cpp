#include <atomic>
#include <thread>
#include <assert.h>
#include <stdio.h>


std::atomic<int> z;


int main(int argc, char* argv[])
{
    
    printf("z init val %d\n", z.load());
    z++;
    printf("z init val %d\n", z.load());
    return 0;
}