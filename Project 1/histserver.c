
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <mqueue.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "shareddef.h"

int main(int argc, char **argv)
{
    int pNo = atoi(argv[1]);
    int total[1000];
    int result[1000];

    for(int i = 0; i < 1000; i++)
    {
        result[i] = 0;
    }
    /* int size = 5;
     int start = 1000;
     int width = 200;*/
    mqd_t mqReceiveFromClient;
    struct mq_attr mq_attrReceiveFromClient;
    struct info *itemptrC;
    int nC;
    char *bufptrC;
    int buflenC;
    int trialNO = 0;
    while (1)
    {
        mqReceiveFromClient = mq_open("/toServer", O_RDWR | O_CREAT, 0666, NULL);
        if (mqReceiveFromClient == -1)
        {
            perror("can not create msg queue\n");
            exit(1);
        }
        printf("mq created, mq id = %d\n", (int)mqReceiveFromClient);

        mq_getattr(mqReceiveFromClient, &mq_attrReceiveFromClient);

        /* allocate large enough space for the buffer to store
            an incoming message */
        buflenC = mq_attrReceiveFromClient.mq_msgsize;
        bufptrC = (char *)malloc(buflenC);

        nC = mq_receive(mqReceiveFromClient, (char *)bufptrC, buflenC, NULL);
        if (nC == -1)
        {
            perror("mq_receive failed\n");
            exit(1);
        }

        printf("mq_receive success, message size = %d\n", nC);

        itemptrC = (struct info *)bufptrC;

        printf("item->intervalCount = %d\n", itemptrC->intervalCount);
        printf("item->intervalWidth = %d\n", itemptrC->intervalWidth);
        printf("item->intervalStart = %d\n", itemptrC->intervalStart);
        printf("\n");

        int size = itemptrC->intervalCount;
        int width = itemptrC->intervalWidth;
        int start = itemptrC->intervalStart;

        pid_t process; // stores process id
        int i;
        pid_t parentid;
        int x;

        parentid = getpid();

        printf("I am parent and my pid is: %d\n", parentid);

        /*message queue for child-parent communication*/
        mqd_t mq;
        int n;

        for (i = 0; i < pNo; ++i)
        {
            process = fork();
            if (process == 0)
            {
                /* this part executed by child process*/

                FILE *file;
                char line[512];
                struct itemArray array;
                char *txtFile;
                txtFile = argv[2 + i];

                file = fopen(txtFile, "r");

                if (!file)
                    exit(1);

                int count = 0;

                // First we get how many integers we need
                // We assume that in the input file,
                // size of array is not determined
                while (fgets(line, sizeof(line), file))
                    count++;

                // write count to result
                array.sizeArray = count;

                // Return to the beginning of the file
                rewind(file);

                for (int i = 0; i < 1000; i++)
                {
                    array.counter[i] = 0;
                }

                // Read every value one by one and write it
                // to the allocated memory of result struct
                while (fgets(line, sizeof(line), file))
                {
                    int val = atoi(line);
                    int a = (val - start) / width;
                    array.counter[a]++;
                }

                printf("count : %d\n", count);
                fclose(file);
                for (int k = 0; k < count; k++)
                {
                    printf("array %d : %d  in process %d\n", k, array.counter[k], pNo);
                }

                array.word[0] = i + '0';
            
                mq = mq_open("/fork", O_RDWR);
                if (mq == -1)
                {
                    perror("can not open msg queue\n");
                    exit(1);
                }


                printf("mq opened, mq id = %d\n", (int)mq);
                int m = 0;

                int mls = 0;
                while (mls < pNo)
                {
                    array.id = m;

                    n = mq_send(mq, (char *)&array, sizeof(struct itemArray), 0);
                    if (n == -1)
                    {
                        perror("mq_send failed\n");
                        exit(1);
                    }

                    /*  printf("mq_send success, item size = %d\n",
                             (int)sizeof(struct itemArray));
                      printf("item->id   = %d\n", array.id);
                      printf("item->counter = %d , %d  , %d\n", array.counter[0], array.counter[1], array.counter[2]);
                      printf("\n");*/

                    m++;
                    mls++;
                    sleep(1);
                }

                /*mq_close(mq);*/

                exit(0); /* child is terminating */
            }
            else
            {
                /* parent process */
                printf("parent created child and child pid= %d\n", process);

                /*mqd_t mq;*/
                struct mq_attr mq_attr;
                struct itemArray *itemptr;
                int n;
                char *bufptr;
                int buflen;

                mq = mq_open("/fork", O_RDWR | O_CREAT, 0666, NULL);
                if (mq == -1)
                {
                    perror("can not create msg queue\n");
                    exit(1);
                }
                printf("mq created, mq id = %d\n", (int)mq);

                mq_getattr(mq, &mq_attr);
                printf("%d messages are currently on the queue.\n", (int)mq_attr.mq_curmsgs);
                /* printf("mq maximum msgsize = %d\n", (int)mq_attr.mq_msgsize);*/

                /* allocate large enough space for the buffer to store
                    an incoming message */
                buflen = mq_attr.mq_msgsize;
                bufptr = (char *)malloc(buflen);

                for (int t = 0; t < 10; t++)
                {
                    total[t] = 0;
                }

                int trk = 0;
                while (trk < 1)
                {
                    n = mq_receive(mq, (char *)bufptr, buflen, NULL);
                    if (n == -1)
                    {
                        perror("mq_receive failed\n");
                        exit(1);
                    }

                    printf("mq_receive success, message size = %d\n", n);

                    itemptr = (struct itemArray *)bufptr;
                    for (int t = 0; t < 1000; t++)
                    {
                        result[t] = result[t] + itemptr->counter[t];
                    }

                    printf("item->word = %s\n", itemptr->word);
                    /* printf("item->counter = %d\n", itemptr->counter[0]);
                     printf("item->id = %d\n", itemptr->id);*/
                    printf("\n");
                    trk++;
                }
                for (int t = 0; t < 10; t++)
                {
                    printf("Total %d is : %d\n", t, result[t]);
                }

                /*  printf("mq_send success, item size = %d\n",
                         (int)sizeof(struct itemArray));
                  printf("item->id   = %d\n", array.id);
                  printf("item->counter = %d , %d  , %d\n", array.counter[0], array.counter[1], array.counter[2]);
                  printf("\n");*/
            }
        }

        // wait for all children to terminate
        for (i = 0; i < pNo; ++i)
            wait(NULL);

        mq_close(mq);

        printf("all children terminated. bye... \n");
        mqd_t mqSend;
        int nSend;
        struct itemArray messageToClient;
        messageToClient.id = trialNO;
        

        for (int t = 0; t < 1000; t++)
        {
            messageToClient.counter[t] = result[t];
        }

        for (int t = 0; t < 10; t++)
        {
            printf("messageToClient %d is : %d\n", t, messageToClient.counter[t]);
        }

        mqSend = mq_open("/toClient", O_RDWR);
        if (mqSend == -1)
        {
            perror("can not open msg queue\n");
            exit(1);
        }

        printf("mq opened, mq id = %d\n", (int)mqSend);
        int m = 0;

        messageToClient.id = m;

        nSend = mq_send(mqSend, (char *)&messageToClient, sizeof(struct itemArray), 0);
        if (nSend == -1)
        {
            perror("mq_send failed\n");
            exit(1);
        }
        trialNO++;
    }
}