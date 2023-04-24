#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/time.h> 
#include <math.h>
#include "dma.h"

int main(int argc, char **argv)
{

    struct timeval t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16, t17, t18;
    double elapsedTime;

    void *p1;
    void *p2;
    void *p3;
    void *p4;
    int ret;
    ret = dma_init(16); // create a segment of 1 MB

    if (ret != 0) {
        printf("something was wrong\n");
        exit(1);
    }

    gettimeofday(&t1, NULL);
    p1 = dma_alloc(100); // allocate space for 100 bytes
    gettimeofday(&t2, NULL);
    elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
    elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
    printf("%f ms.\n", elapsedTime);    

    gettimeofday(&t3, NULL);
    p2 = dma_alloc(1024);
    gettimeofday(&t4, NULL);
    elapsedTime = (t4.tv_sec - t3.tv_sec) * 1000.0;      // sec to ms
    elapsedTime += (t4.tv_usec - t3.tv_usec) / 1000.0;   // us to ms
    printf("%f ms.\n", elapsedTime); 

    gettimeofday(&t5, NULL);
    p3 = dma_alloc(64); // always check the return value
    gettimeofday(&t6, NULL);
    elapsedTime = (t6.tv_sec - t5.tv_sec) * 1000.0;      // sec to ms
    elapsedTime += (t6.tv_usec - t5.tv_usec) / 1000.0;   // us to ms
    printf("%f ms.\n", elapsedTime); 

    gettimeofday(&t7, NULL);
    p4 = dma_alloc(220);
    gettimeofday(&t8, NULL);
    elapsedTime = (t8.tv_sec - t7.tv_sec) * 1000.0;      // sec to ms
    elapsedTime += (t8.tv_usec - t7.tv_usec) / 1000.0;   // us to ms
    printf("%f ms.\n", elapsedTime); 

    dma_free(p3);

    gettimeofday(&t9, NULL);
    p3 = dma_alloc(2048);
    gettimeofday(&t10, NULL);
    elapsedTime = (t10.tv_sec - t9.tv_sec) * 1000.0;      // sec to ms
    elapsedTime += (t10.tv_usec - t9.tv_usec) / 1000.0;   // us to ms
    printf("%f ms.\n", elapsedTime); 

    dma_print_bitmap();

    dma_print_blocks();

    gettimeofday(&t11, NULL);
    dma_free(p1);
    gettimeofday(&t12, NULL);
    elapsedTime = (t12.tv_sec - t11.tv_sec) * 1000.0;      // sec to ms
    elapsedTime += (t12.tv_usec - t11.tv_usec) / 1000.0;   // us to ms
    printf("%f ms.\n", elapsedTime); 

    gettimeofday(&t13, NULL);
    dma_free(p2);
    gettimeofday(&t14, NULL);
    elapsedTime = (t14.tv_sec - t13.tv_sec) * 1000.0;      // sec to ms
    elapsedTime += (t14.tv_usec - t13.tv_usec) / 1000.0;   // us to ms
    printf("%f ms.\n", elapsedTime); 

    gettimeofday(&t15, NULL);
    dma_free(p3);
    gettimeofday(&t16, NULL);
    elapsedTime = (t16.tv_sec - t15.tv_sec) * 1000.0;      // sec to ms
    elapsedTime += (t16.tv_usec - t15.tv_usec) / 1000.0;   // us to ms
    printf("%f ms.\n", elapsedTime); 

    dma_print_blocks();

    gettimeofday(&t17, NULL);
    dma_free(p4);
    gettimeofday(&t18, NULL);
    elapsedTime = (t18.tv_sec - t17.tv_sec) * 1000.0;      // sec to ms
    elapsedTime += (t18.tv_usec - t17.tv_usec) / 1000.0;   // us to ms
    printf("%f ms.\n", elapsedTime); 

    dma_print_bitmap();
}
