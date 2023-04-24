#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include <math.h>
#include "dma.h"

/* GLOBAL VARIABLES */

void *heap;
unsigned int size;
char *bitmap;
int bitSize;
int bits;
int pageSize;
int noOfpage;
int totalFrag;
pthread_mutex_t mutex;

/* FUNCTIONS */

int dma_init(int m)
{

    if (m < 14 || m > 22)
    {
        printf("Range not valid.\n");
        return -1;
    }

    else
    {
        pthread_mutex_init(&mutex, NULL);
        totalFrag = 0;
        size = pow(2.00, (double)m);
        // printf("%d\n", size);
        
        heap = mmap(NULL, (size_t)size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        if (heap == MAP_FAILED)
        {
            printf("Heap segment allocation unsuccessful.\n");
            return -1;
        }

        printf("%lx\n", (long)heap);

        // bitmap
        bits = pow(2.00, (double)m - 3);
        pageSize = pow(2.00, 12.00);
        noOfpage = (int)(pow(2.00, (double)m) / pageSize);
        //printf("Page number = %d", noOfpage);
        
        bitmap = (char *)heap;
        // printf("bits : %d\n", bits);

        for (int i = 0; i < bits; i++)
        {
            // printf("%d\n", i);
            bitmap[i] = '1';
        }

        return 0;
    }
}

void *dma_alloc2(int size)
{

    int a = 0;
    int b = 1;

    while (b < bits)
    {
        if (*(bitmap + a) == '1' && *(bitmap + b) == '1')
        {
            int check = a;
            int count = 0;
            while (*(bitmap + check) == '1')
            {
                count++;
                if (count == size)
                {
                    bitmap[a] = '0';
                    bitmap[b] = '1';
                    for (int i = 0; i < size - 2; i++)
                    {
                        bitmap[b + i + 1] = '0';
                    }
                    //   printf(" a : %d\n", a);
                    // printf(" In func %lx\n", (long)bitmap + a);
                    return bitmap + bits/8 + 256 + a; //allocation is done after the bitmap and 256 bytes empty area
                }
                check++;
            }
        }
        a++;
        b++;
    }

    return NULL;
}

void *dma_alloc(int size)
{
    pthread_mutex_lock(&mutex);
    int blockSize;
    if (size % 16 == 0)
    {
        blockSize = size / 16;
        // printf("Block size = %d\n", blockSize);
    }

    else
    {
        blockSize = size / 16 + 1;
        totalFrag = totalFrag + 16 * blockSize - size;
        // printf("Block size = %d\n", blockSize);
    }
    void *p = dma_alloc2(blockSize * 2);
    pthread_mutex_unlock(&mutex);

    return p;
}

void dma_free(void *p)
{
    pthread_mutex_lock(&mutex);
    char *k = (char *)p;
    // printf(" k %lx\n", (long)k);
    k = k - (bits/8 + 256);
    int a = 0;
    int b = 1;
    k[a] = '1';
    k[b] = '1';

    while (!(*(k + a) == '0' && *(k + b) == '1'))
    {
        k[a] = '1';
        k[b] = '1';
        a++;
        a++;
        b++;
        b++;
        if (!((k + b) < (bitmap + bits)))
            break;
        // printf("a = %d\n",a);
    }
    pthread_mutex_unlock(&mutex);
}

void binToHexa(int bin)
{
    long int hexadecimalval = 0, i = 1, remainder;

    while (bin != 0)

    {
        remainder = bin % 10;
        hexadecimalval = hexadecimalval + remainder * i;
        i = i * 2;
        bin = bin / 10;
    }

    printf("%lX", hexadecimalval);
}

void dma_print_page(int pno)
{
    int part = bits / noOfpage;
    // printf("part : %d\n", part);

    char subtext2[4];
    char *pageStart2 = bitmap + part * pno;
    int a = 0;
    int couunter = 0;

    while (a < part)
    {
        // printf("%lx\n", (long)pageStart2);
        memcpy(subtext2, &(pageStart2[0]), 4);
        // subtext2[7] = '\0';
        int val = atoi(subtext2);
        binToHexa(val);
        //  printf(" in counter %d = Substring is: %s \n",couunter, subtext2);
        pageStart2 = (pageStart2 + 4);
        a = a + 4;
        couunter++;
        if (couunter % 64 == 0)
            printf("\n");
    }
}

void dma_print_bitmap()
{
    int counter = 0;
    char *text = bitmap;
    char x[8];

    while (counter < bits)
    {
        memcpy(x, &(text[0]), 8);
        printf(" %s", x);
        text = text + 8;
        counter = counter + 8;
        if (counter % 64 == 0)
            printf("\n");
    }
    printf("\n");

    //    printf("%s", bitmap);
    //    printf("\n");
    // return NULL;
}

// function to convert decimal to hexadecimal
void decToHexa(int num_decimal)
{
    int a = 1, b, var;
    long int quotient;
    char hexanum_decimal[100];

    quotient = num_decimal;
    while (quotient != 0)
    {
        var = quotient % 16;
        if (var < 10)
            var = var + 48;
        else
            var = var + 55;
        hexanum_decimal[a++] = var;
        quotient = quotient / 16;
    }
    for (b = a - 1; b > 0; b--)
        printf("%c", hexanum_decimal[b]);
}

void dma_print_blocks()
{
    int a = 0;
    int b = 1;

    //  printf("bits : %d\n", bits);

    while (b < bits)
    {
        if (*(bitmap + a) == '1' && *(bitmap + b) == '1')
        {
            // printf("if free \n");
            int check = a;
            int count = 0;
            while (*(bitmap + check) == '1')
            {
                count++;
                // printf("count : %d\n", count);
                check++;
            }

            printf("F, 0x000%lx, 0x", (long)bitmap + bits/8 + 256 + a);
            decToHexa(count * 8);
            printf(" (%d) \n", count * 8);
            a = a + count;
            b = b + count;
            // printf("a : %d \n", a);
            // printf("b : %d \n", b);
        }

        if (*(bitmap + a) == '0' && *(bitmap + b) == '1')
        {
            int check = a;
            int count = 2;
            while (*(bitmap + check + 2) == '0' && *(bitmap + check + 3) == '0')
            {
                count = count + 2;
                check = check + 2;
            }

            // printf("A, 0x000%lx, %d (%d)\n", (long)bitmap + a,decToHexa(count), count);
            printf("A, 0x000%lx, 0x", (long)bitmap + bits/8 + 256+ a);
            decToHexa(count * 8);
            printf(" (%d) \n", count * 8);
            a = a + count;
            b = b + count;
            // printf("a : %d \n", a);
            // printf("b : %d \n", b);
        }
    }
}

int dma_give_intfrag()
{
    return totalFrag;
}
