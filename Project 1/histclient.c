#include <stdlib.h>
#include <mqueue.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "shareddef.h"

int main(int argc, char **argv)
{
    struct info message;
    message.intervalCount = atoi(argv[1]);
    message.intervalWidth = atoi(argv[2]);
    message.intervalStart = atoi(argv[3]);

    mqd_t mqSendToServer;
    int nSendToServer;

    mqSendToServer = mq_open("/toServer", O_RDWR);
    if (mqSendToServer == -1)
    {
        perror("can not open msg queue\n");
        exit(1);
    }
    printf("mq opened, mq id = %d\n", (int)mqSendToServer);

    nSendToServer = mq_send(mqSendToServer, (char *)&message, sizeof(struct info), 0);

    if (nSendToServer == -1)
    {
        perror("mq_send failed\n");
        exit(1);
    }

    printf("intervalCount : %d\n", message.intervalCount);
    printf("intervalWidth : %d\n", message.intervalWidth);
    printf("intervalStart : %d\n", message.intervalStart);

    mq_close(mqSendToServer);

    mqd_t mqReceive;
    struct mq_attr mq_attrReceive;
    struct itemArray *itemptr;
    int m;
    char *bufptr;
    int buflen;

    mqReceive = mq_open("/toClient", O_RDWR | O_CREAT, 0666, NULL);
    if (mqReceive == -1)
    {
        perror("can not create msg queue\n");
        exit(1);
    }
    printf("mq created, mq id = %d\n", (int)mqReceive);

    mq_getattr(mqReceive, &mq_attrReceive);
    printf("mq maximum msgsize = %d\n", (int)mq_attrReceive.mq_msgsize);

    /* allocate large enough space for the buffer to store
        an incoming message */
    buflen = mq_attrReceive.mq_msgsize;
    printf("buflen : %d/n", buflen);
    bufptr = (char *)malloc(buflen);

    m = mq_receive(mqReceive, (char *)bufptr, buflen, NULL);
    if (m == -1)
    {
        perror("mq_receive failed\n");
        exit(1);
    }

    printf("mq_receive success, message size = %d\n", m);

    itemptr = (struct itemArray *)bufptr;

    for (int i = 0; i < 10; i++)
    {

        printf("TOTAL %d :  %d \n", i, itemptr->counter[i]);
    }

    int size = message.intervalCount;
    int start = message.intervalStart;
    int width = message.intervalWidth;
    int b = start;

    for (int i = 0; i < size; i++)
    {

        printf("[ %d , %d ] : %d\n", b, b + width, itemptr->counter[i]);
        b = b + width;
    }

    mq_close(mqReceive);
    return 0;
}