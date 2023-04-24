#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#define READY 0
#define RUNNING 1
#define WAITING 2
#define SLEEP 0
#define WAKE 1
/*             Structs                        */
struct PCB
{
    int pid;
    pthread_t thread_id;
    int state;
    double nextBurstLength;
    int burst;
    int remainingBurst;
    int noOfCpuBurst;
    long waitingTime;
    int arrv;
    int noOfDevice1;
    int noOfDevice2;
    long startTime;
    long finishTime;
    long totalTimeinCpu;
};

struct io
{
    char name[50];
    struct Queue *waitingQueue;
};

/***************************************************************************************/
/*             Queue Implementation                        */
struct QNode
{
    struct PCB key;
    struct QNode *next;
};

struct Queue
{
    struct QNode *front, *rear;
    int size;
};

struct QNode *newNode(struct PCB k)
{
    struct QNode *temp = (struct QNode *)malloc(sizeof(struct QNode));
    temp->key = k;
    temp->next = NULL;
    return temp;
}

struct Queue *createQueue()
{
    struct Queue *q = (struct Queue *)malloc(sizeof(struct Queue));
    q->front = q->rear = NULL;
    q->size = 0;
    return q;
}
void size(struct Queue *q)
{
    // If queue is empty, return NULL.
    if (q->front == NULL)
    {
        printf("Size : 0\n");

        return;
    }

    // Store previous front and move front one node ahead
    struct QNode *temp = q->front;
    int counter = 0;
    while (temp->next != NULL)
    {
        counter++;
        temp = temp->next;
    }
    printf("Size : %d\n", counter + 1);
    // free(temp);
}

void enQueue(struct Queue *q, struct PCB k)
{
    // Create a new LL node
    struct QNode *temp = newNode(k);

    // If queue is empty, then new node is front and rear both
    if (q->rear == NULL)
    {
        q->front = q->rear = temp;
        return;
    }

    // Add the new node at the end of queue and change rear
    q->rear->next = temp;
    q->rear = temp;
    q->size++;
}

void deQueue(struct Queue *q)
{
    // If queue is empty, return NULL.
    if (q->front == NULL)
        return;

    // Store previous front and move front one node ahead
    struct QNode *temp = q->front;

    q->front = q->front->next;

    // If front becomes NULL, then change rear also as NULL
    if (q->front == NULL)
        q->rear = NULL;

    q->size--;
    // free(temp);
}
int findSize(struct Queue *q)
{
    // If queue is empty, return NULL.
    if (q->front == NULL)
    {

        return 0;
    }

    // Store previous front and move front one node ahead
    struct QNode *temp = q->front;
    int counter = 0;
    while (temp->next != NULL)
    {
        counter++;
        temp = temp->next;
    }
    return counter + 1;
    // free(temp);
}

void print(struct Queue *q)
{
    // If queue is empty, return NULL.
    if (q->front == NULL)
    {
        printf("EMPTY QUEUE");
        return;
    }

    // Store previous front and move front one node ahead
    struct QNode *temp = q->front;
    while (temp->next != NULL)
    {
        printf(" pid: %d  burst: %d -> ", temp->key.pid, temp->key.remainingBurst);
        temp = temp->next;
    }
    printf(" pid: %d  burst: %d\n", temp->key.pid, temp->key.remainingBurst);
    // free(temp);
}
void sort(struct Queue *queue)
{

    int i, j, k;
    struct PCB *tempKey, tempData;
    struct QNode *current;
    struct QNode *next;

    int size = findSize(queue);
    k = size;

    for (i = 0; i < size - 1; i++, k--)
    {
        current = queue->front;
        next = queue->front->next;

        for (j = 1; j < k; j++)
        {

            if (current->key.remainingBurst > next->key.remainingBurst)
            {
                tempData = current->key;
                current->key = next->key;
                next->key = tempData;

                /*tempKey = current->key;
                current->key = next->key;
                next->key = tempKey;*/
            }

            current = current->next;
            next = next->next;
        }
    }
}
/***************************************************************************************/
/*  global variables */
int maxp;
int allp;
char *burst_dist;
int burstlen;
int min_burst;
int max_burst;
double p0;
double p1;
double p2;
int io1Time;
char *alg;
int quantum;
int totalProceessCounter;
int workingProcess;
int arriveTime;
int outputMode;

int schedulerNo = 0;
int operationNo = -1;
int schedulerControl = SLEEP;
int io1QueueSize;
int io2QueueSize;
int t1;
int t2;
int maxp;
double pg;

int count;
long i2 = 0;
struct Queue *readyQueue;
struct io io_1;
struct io io_2;

pthread_t threads2[1000];
pthread_mutex_t mutex;
pthread_mutex_t mutexScheduler;
pthread_mutex_t mutexio1;
pthread_mutex_t mutexio2;
pthread_mutex_t mutexProcessNo;
pthread_mutex_t mutexWorkingProcess;

pthread_cond_t condReadyQueue;
pthread_cond_t condScheduler;
pthread_cond_t condio1;
pthread_cond_t condio2;
pthread_cond_t condWorkingProcess;

/***************************************************************************************/
void *thread_task(void *x)
{
    struct timeval creationTime, finishTime, cpuStart, cpuFinish, currentTime;
    double elapsedTime;

    gettimeofday(&creationTime, NULL);

    struct PCB pcb;
    long cpu = 0;

    if (strcmp(burst_dist, "fixed") == 0)
    {
        pcb.remainingBurst = burstlen;
        pcb.burst = burstlen;
    }

    if (strcmp(burst_dist, "uniform") == 0)
    {
        double myRand = rand() / (1.0 + RAND_MAX);
        int range = max_burst - min_burst + 1;
        int uniform = (myRand * range) + min_burst;
        pcb.remainingBurst = uniform;
        pcb.burst = uniform;
    }

    if (strcmp(burst_dist, "exponential") == 0)
    {
        printf("here\n");
        int expo;
        double u;
        int range = max_burst - min_burst + 1;
        u = rand() / (RAND_MAX + 1.0);
        expo = (int)(burstlen * exp(-burstlen * u * u * u * u * u * u));
        printf("expo : %f\n", burstlen * exp(-burstlen * u * u * u * u * u * u));
        if (expo < max_burst && expo > min_burst)
        {
            pcb.remainingBurst = expo;
            pcb.burst = expo;
        }
        else
        {
            while (expo > max_burst || expo < min_burst)
            {
                u = rand() / (RAND_MAX + 1.0);
                expo = (int)(burstlen * exp(-burstlen * u * u * u * u * u * u));
            }
            pcb.remainingBurst = expo;
            pcb.burst = expo;
        }
    }
    pthread_mutex_lock(&mutex);

    if (outputMode == 3)
        printf("I am thread: %d\n", count);

    count++;
    pcb.thread_id = count;
    pcb.pid = count;
    pcb.state = READY;
    pcb.noOfDevice1 = 0;
    pcb.noOfDevice2 = 0;
    pcb.arrv = arriveTime;
    enQueue(readyQueue, pcb);

    gettimeofday(&currentTime, NULL);
    if (outputMode == 2)
        printf("%ld  %d  READY\n", currentTime.tv_usec, pcb.pid);

    // sort(readyQueue);

    pthread_mutex_unlock(&mutex);

    pthread_mutex_lock(&mutex);

    while (1)
    {
        if (pcb.pid == schedulerNo)
        {
            if (operationNo == 1)
            {
                pthread_mutex_lock(&mutexio1);

                if (outputMode == 3)
                    printf("Process with pid %d in I/O %d\n", pcb.pid, 1);

                pcb.noOfDevice1++;
                while (io1QueueSize > 0)
                {
                    pthread_cond_wait(&condio1, &mutexio1);
                }
                io1QueueSize++;
                pthread_mutex_unlock(&mutexio1);
                usleep(t1);
                pthread_mutex_lock(&mutexio1);
                io1QueueSize--;
                pthread_mutex_unlock(&mutexio1);

                gettimeofday(&currentTime, NULL);
                if (outputMode == 2)
                    printf("%ld  %d  USING DEVICE1\n", currentTime.tv_usec, pcb.pid);

                pthread_cond_signal(&condio1);
                enQueue(readyQueue, pcb);
                // sort(readyQueue);
                // print(readyQueue);

                pthread_cond_signal(&condScheduler);
            }
            if (operationNo == 2)
            {
                if (outputMode == 3)
                    printf("-----------------------------I / O 2 --------------------------------------------------------\n");

                pthread_mutex_lock(&mutexio2);

                if (outputMode == 3)
                    printf("Process with pid %d in I/O %d\n", pcb.pid, 2);

                pcb.noOfDevice2++;

                while (io2QueueSize > 0)
                {
                    pthread_cond_wait(&condio2, &mutexio2);
                }
                io2QueueSize++;
                pthread_mutex_unlock(&mutexio2);
                usleep(t2);
                pthread_mutex_lock(&mutexio2);
                io2QueueSize--;
                pthread_mutex_unlock(&mutexio2);

                gettimeofday(&currentTime, NULL);
                if (outputMode == 2)
                    printf("%ld  %d  USING DEVICE2\n", currentTime.tv_usec, pcb.pid);

                pthread_cond_signal(&condio2);
                // print(readyQueue);
                enQueue(readyQueue, pcb);
                // print(readyQueue);
                // sort(readyQueue);
                // print(readyQueue);
                if (outputMode == 3)
                    printf("-----------------------------I / O 2 END--------------------------------------------------------\n");
                pthread_cond_signal(&condScheduler);
            }

            if (operationNo == 0)
            {
                if (strcmp(alg, "RR") == 0)
                {
                    if (outputMode == 3)
                        printf("-----------------------------CPU--------------------------------------------------------\n");

                    if (pcb.remainingBurst > quantum)
                    {
                        pcb.remainingBurst = pcb.remainingBurst - quantum;
                        pcb.noOfCpuBurst++;

                        gettimeofday(&cpuStart, NULL);
                        usleep(quantum);
                        gettimeofday(&cpuFinish, NULL);
                        long tt = cpuFinish.tv_usec - cpuStart.tv_usec;
                        cpu = cpu + tt;

                        gettimeofday(&currentTime, NULL);
                        if (outputMode == 2)
                            printf("%ld  %d  RUNNING\n", currentTime.tv_usec, pcb.pid);

                        enQueue(readyQueue, pcb);

                        if (outputMode == 3)
                            printf("-----------------------------END CPU--------------------------------------------------------\n");

                        pthread_cond_signal(&condScheduler);
                    }
                    else
                    {
                        pcb.remainingBurst = 0;
                        pcb.noOfCpuBurst++;
                        usleep(pcb.remainingBurst);
                        gettimeofday(&currentTime, NULL);
                        if (outputMode == 2)
                            printf("%ld  %d  RUNNING\n", currentTime.tv_usec, pcb.pid);
                        if (outputMode == 3)
                            printf("-----------------------------END CPU AND FINISH --------------------------------------------------------\n");

                        break;
                    }
                }
                else
                {
                    pcb.noOfCpuBurst++;
                    break;
                }
            }
        }

        pthread_cond_wait(&condReadyQueue, &mutex);
    }
    pthread_mutex_unlock(&mutex);

    if (strcmp(alg, "RR") == 1)
    {
        gettimeofday(&cpuStart, NULL);
        usleep(pcb.remainingBurst);
        gettimeofday(&cpuFinish, NULL);
        cpu = cpuFinish.tv_usec - cpuStart.tv_usec;
        gettimeofday(&currentTime, NULL);
        if (outputMode == 2)
            printf("%ld  %d  RUNNING\n", currentTime.tv_usec, pcb.pid);
    }

    pcb.totalTimeinCpu = cpu;
    gettimeofday(&finishTime, NULL);
    pcb.finishTime = finishTime.tv_usec;
    pcb.startTime = creationTime.tv_usec;

    if (outputMode == 3)
        printf("Finish process with pid : %d Number of device 1 use : %d Number of device 2 use : %d Cpu time : %ld\n", pcb.pid, pcb.noOfDevice1, pcb.noOfDevice2, pcb.totalTimeinCpu);

    pthread_mutex_lock(&mutexProcessNo);
    totalProceessCounter++;
    // printf("total process : %d\n", totalProceessCounter);
    pthread_mutex_unlock(&mutexProcessNo);
    pthread_cond_signal(&condScheduler);

    pthread_mutex_lock(&mutexWorkingProcess);
    workingProcess--;
    pthread_mutex_unlock(&mutexWorkingProcess);
    pthread_cond_signal(&condWorkingProcess);

    if (outputMode == 1)
        printf("%d   %d   %ld   %ld   %ld   %ld   %d   %d   %d\n", pcb.pid, pcb.arrv, pcb.finishTime - pcb.startTime + pcb.arrv, pcb.totalTimeinCpu, pcb.finishTime - pcb.startTime - pcb.totalTimeinCpu, pcb.finishTime - pcb.startTime, pcb.burst - pcb.remainingBurst, pcb.noOfDevice1, pcb.noOfDevice2);

    pthread_exit(NULL);
}

void *processGenerator(void *x)
{
    int result2;

    if (allp < 10)
    {
        // printf("allp < 10 \n");
        for (int m = 0; m < allp; m++)
        {

            result2 = pthread_create(&threads2[i2 + m], NULL, thread_task, (void *)i2);
            if (result2 != 0)
            {
                printf("can not create thread\n");
                exit(1);
            }
        }

        for (int m = 0; m < allp; ++m)
        {
            pthread_join(threads2[m], NULL);
        }
    }

    else
    {
        for (int m = 0; m < 10; m++)
        {

            result2 = pthread_create(&threads2[i2 + m], NULL, thread_task, (void *)i2);
            if (result2 != 0)
            {
                printf("can not create thread\n");
                exit(1);
            }
        }

        i2 = i2 + 10;

        pthread_mutex_lock(&mutexWorkingProcess);
        while (i2 < allp)
        {
            if (workingProcess < maxp)
            {
                double random = (double)rand() / RAND_MAX;
                // printf("random : %f\n", random);

                if (random < pg) // generates new thread with probability pg
                {
                    result2 = pthread_create(&threads2[i2], NULL, thread_task, (void *)i2);
                    if (result2 != 0)
                    {
                        printf("can not create thread\n");
                        exit(1);
                    }

                    i2++;
                    workingProcess++;

                    arriveTime = arriveTime + 5;
                    usleep(5);
                }
            }
            else
                pthread_cond_wait(&condWorkingProcess, &mutexWorkingProcess);
        }
        pthread_mutex_unlock(&mutexWorkingProcess);

        for (int m = 0; m < i2; ++m)
        {
            if (m > 9)
                usleep(5);

            pthread_join(threads2[m], NULL);
        }
    }

    if (outputMode == 3)
        printf("generator finish \n");

    pthread_exit(NULL);
}

void *scheduler(void *x)
{
    int index = 0;
    int loop = 0;
    usleep(5);
    while (1)
    {
        if (outputMode == 3)
            printf("-----------------------------SCHEDULER--------------------------------------------------------\n");
        // print(readyQueue);
        pthread_mutex_lock(&mutexProcessNo);
        loop = totalProceessCounter;
        pthread_mutex_unlock(&mutexProcessNo);

        // if (readyQueue->front != NULL)
        if (loop < allp)
        {

            pthread_mutex_lock(&mutexScheduler);

            pthread_mutex_lock(&mutex);

            if (strcmp(alg, "SJF") == 0)
            {
                sort(readyQueue);
                // printf("SORT\n");
            }

            double random;
            random = (double)rand() / RAND_MAX;

            if (random < p0)
            {
                operationNo = 0;
            }

            else if (random > p0 && random <= p0 + p1)
            {
                operationNo = 1;
            }

            // process goes I/O device 2 with probability p2
            else
            {
                operationNo = 2;
            }

            int time = readyQueue->front->key.remainingBurst;
            readyQueue->front->key.state = RUNNING;
            schedulerNo = readyQueue->front->key.pid;

            if (outputMode == 3)
                printf("SCHEDULER CHHOSE PID : %d Operation no : %d\n", schedulerNo, operationNo);

            // printf("In Cpu thread with pid : %d is running and its burst length : %d and state : %d\n", readyQueue->front->key.pid, readyQueue->front->key.remainingBurst, readyQueue->front->key.state);

            // size(readyQueue);

            deQueue(readyQueue);
            // print(readyQueue);

            // printf("index %d\n", index);
            pthread_mutex_unlock(&mutex);
            pthread_cond_broadcast(&condReadyQueue);

            pthread_cond_wait(&condScheduler, &mutexScheduler);
            pthread_mutex_unlock(&mutexScheduler);

            index++;
            if (outputMode == 3)
                printf("-----------------------------SCHEDULER END--------------------------------------------------------\n");
        }
        else
            break;
        // printf("NULL\n");
    }
    if (outputMode == 3)
        printf("Scheduler finish\n");
    pthread_exit(NULL);
}

int main(int argc, char **argv)
{
    /*------------------------------ Argument initialization -----------------------------------*/
   
    alg = "RR";
    quantum = 5;
    t1 = 50;
    t2 = 70;
    burst_dist = "uniform";
    burstlen = 3;
    max_burst = 50;
    min_burst = 1;
    p0 = 0.7;
    p1 = 0.2;
    p2 = 0.1;
    pg = 0.5;
    maxp = 9;
    allp = 20;
    outputMode = 3;

    io1Time = 3;
    io1QueueSize = 0;
    io2QueueSize = 0;
    totalProceessCounter = 0;
    arriveTime = 0;

    readyQueue = createQueue();

    pthread_cond_init(&condReadyQueue, NULL);
    pthread_cond_init(&condScheduler, NULL);
    pthread_cond_init(&condio1, NULL);
    pthread_cond_init(&condio2, NULL);
    pthread_cond_init(&condWorkingProcess, NULL);

    /*------------------------------ Process Generator initialization -----------------------------------*/

    int processGeneratorThread;
    int cpuScheduler;
    int io1;
    pthread_t threads[100];
    pthread_mutex_init(&mutex, NULL);
    pthread_mutex_init(&mutexScheduler, NULL);
    pthread_mutex_init(&mutexio1, NULL);
    pthread_mutex_init(&mutexio2, NULL);
    pthread_mutex_init(&mutexProcessNo, NULL);
    pthread_mutex_init(&mutexWorkingProcess, NULL);

    long i = 0;
    count = 0;

    if (outputMode == 1)
        printf("pid   arv   dept   cpu   waitr   turna   n-bursts   n-d1   nd2\n");

    processGeneratorThread = pthread_create(&threads[i], NULL, processGenerator, (void *)i);
    if (processGeneratorThread != 0)
    {
        printf("can not create thread\n");
        exit(1);
    }

    i++;
    cpuScheduler = pthread_create(&threads[i], NULL, scheduler, (void *)i);
    if (cpuScheduler != 0)
    {
        printf("can not create thread\n");
        exit(1);
    }

    i = 0;
    pthread_join(threads[i], NULL);
    i++;
    pthread_join(threads[i], NULL);

    // print(readyQueue);
    //  printf("hhhh\n");

    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&mutexScheduler);
    pthread_mutex_destroy(&mutexio1);
    pthread_mutex_destroy(&mutexio2);
    pthread_mutex_destroy(&mutexProcessNo);

    pthread_cond_destroy(&condio1);
    pthread_cond_destroy(&condio2);
    pthread_cond_destroy(&condReadyQueue);
    pthread_cond_destroy(&condScheduler);
    pthread_cond_destroy(&condWorkingProcess);
}
