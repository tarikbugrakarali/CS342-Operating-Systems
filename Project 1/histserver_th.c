
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <mqueue.h>
#include <time.h>

#define MAXTHREADS 20  /* max number of threads */
#define MAXFILENAME 50 /* max length of a filename */

/*
   thread function will take a pointer to this structure
*/
struct itemArray
{
    int id;
    char word[30];
    int counter[1000];
    size_t sizeArray;
};
struct arg
{
    int t_index; /* the index of the created thread */
    char name[50];
};
struct info
{
    int intervalCount;
    int intervalWidth;
    int intervalStart;
};
char *fileName;
int start;
int width;
int total[1000];
/* this is function to be executed by the threads */
static void *do_task(void *arg_ptr)
{
    double time_spent = 0.0;
    clock_t begin = clock();

    int i;
    FILE *fp;
    char filename[MAXFILENAME];
    char *retreason;

    char line[512];

    // strcpy(((struct arg *)arg_ptr)->name, fileName);
    printf("Filename %s \n", ((struct arg *)arg_ptr)->name);

    printf("thread %d started\n", ((struct arg *)arg_ptr)->t_index);

    fp = fopen(((struct arg *)arg_ptr)->name, "r");
    if (!fp)
    {
        perror("do_task:");
        exit(1);
    }

    /* for (i = ((struct arg *)arg_ptr)->n;
          i <= ((struct arg *)arg_ptr)->m; ++i)
     {
         fprintf(fp, "integer = %d\n", i);
     }*/

    // fclose(fp);
    int counter = 0;
    while (fgets(line, sizeof(line), fp))
    {
        int val = atoi(line);
        printf("txt %d : %d \n", counter, val);
        int a = (val - start) / width;
        total[a]++;
    }

    fclose(fp);
    for (int k = 0; k < 14; k++)
    {
        printf("array %d : %d  \n", k, total[k]);
    }

    retreason = malloc(200);
    strcpy(retreason, "normal termination of thread");
    pthread_exit(retreason); // just tell a reason to the thread that is waiting in join

    clock_t end = clock();
    time_spent += (double)(end - begin) / CLOCKS_PER_SEC;
    printf("The elapsed time is %f seconds", time_spent);

}

int main(int argc, char *argv[])
{
    /*--------------------------------------------------------- get data from client-----------------------------------------------------------------------------*/
    mqd_t mqReceiveFromClient;
    struct mq_attr mq_attrReceiveFromClient;
    struct info *itemptrC;
    int nC;
    char *bufptrC;
    int buflenC;

    mqReceiveFromClient = mq_open("/toServerTh", O_RDWR | O_CREAT, 0666, NULL);
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

    start = itemptrC->intervalStart;
    width = itemptrC->intervalWidth;
    /**************************************************************************************************************************************************************/
    /*--------------------------------------------------------- Thread part-----------------------------------------------------------------------------------*/

    pthread_t tids[MAXTHREADS];    /*thread ids*/
    int count;                     /*number of threads*/
    struct arg t_args[MAXTHREADS]; /*thread function arguments*/

    int i;
    int ret;
    char *retmsg;

    fileName = argv[4];
    /*start = 1000;
    width = 200;*/

    for (int i = 0; i < 1000; i++)
    {
        total[i] = 0;
    }

    /* if (argc != 5)
     {
         printf("usage: cthread <numthreads> <minvalue> <maxvalue>\n");
         exit(1);
     }*/

    count = atoi(argv[1]); /* number of threads to create */

    for (i = 0; i < count; ++i)
    {
        fileName = argv[2 + i];
        strcpy(t_args[i].name, fileName);
        t_args[i].t_index = i;

        ret = pthread_create(&(tids[i]),
                             NULL, do_task, (void *)&(t_args[i]));

        if (ret != 0)
        {
            printf("thread create failed \n");
            exit(1);
        }
        printf("thread %i with tid %u created\n", i,
               (unsigned int)tids[i]);
    }

    printf("main: waiting all threads to terminate\n");
    for (i = 0; i < count; ++i)
    {
        ret = pthread_join(tids[i], (void **)&retmsg);
        if (ret != 0)
        {
            printf("thread join failed \n");
            exit(1);
        }
        printf("thread terminated, msg = %s\n", retmsg);
        // we got the reason as the string pointed by retmsg
        // space for that was allocated in thread function; now freeing.
        free(retmsg);
    }

    printf("main: all threads terminated\n");
    /**************************************************************************************************************************************************************/
    /*--------------------------------------------------------- Send to Client-----------------------------------------------------------------------------------*/
    mqd_t mqSend;
    int nSend;
    struct itemArray messageToClient;

    for (int t = 0; t < 1000; t++)
    {
        messageToClient.counter[t] = total[t];
    }

    for (int t = 0; t < 10; t++)
    {
        printf("messageToClient %d is : %d\n", t, messageToClient.counter[t]);
    }

    mqSend = mq_open("/toClientTh", O_RDWR);
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

    mq_close(mqSend);

    return 0;
}