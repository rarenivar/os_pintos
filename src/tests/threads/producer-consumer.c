// producer consumer program
//#include <stdio.h>
#include "threads/thread.h"
//#include "threads/malloc.h"
#include "threads/synch.h"
#include "tests/threads/tests.h"

#define PRIORITY 1
#define NUMPRODUCERS 5
#define NUMCONSUMERS 5
#define DATASIZE 5
#define PRODUCERDATASIZE 45

// condition variables structs, define in synch.h
static struct condition shareBufferNotFull;
static struct condition shareBufferNotEmpty;
static int shareBuffer[DATASIZE];
static int resetBufferCurrentIndex = 0;
static int consumer_digit = 0;
static int producer_digit = 0;

static int consumerData[PRODUCERDATASIZE] = {
        9,2,9,1,
        2,3,6,9,
        1,2,6,9,
        3,1,5,4,
        4,9,4,5,
        7,6,9,6,
        7,7,3,1,
        5,8,4,1,
        7,9,5,3,
        7,5,2,3,
        2,2,9,3 }; //== 215

static int headQueue = 0;
static int tailQueue = 0;
static int totalSum = 0;
static struct lock theLock;
static int theOrder = 0;
void Producer_func(void *aux);
void Consumer_func(void *aux);

// test method
void test_producer_consumer(void) {

    // initializing lock and conditions
    lock_init(&theLock);
    cond_init(&shareBufferNotEmpty);
    cond_init(&shareBufferNotFull);

    int i;
    //we'll now create the threads for the producers and consumers
    for(i = 0; i < NUMPRODUCERS; i++)
    {
        thread_create("Producer", PRIORITY, Producer_func, &theLock);
    }
    for(i = 0; i < NUMCONSUMERS; i++)
    {
        thread_create("Consumer", PRIORITY, Consumer_func, &theLock);
    }

    thread_print_global_metrics();
}


// Function that creates consumer threads
void Consumer_func(void *aux) {
    while(true) {

        lock_acquire(&theLock);

        if (totalSum == 215) {
            thread_exit();
        }

        msg("Consumer ID: %i acquired lock, tailQueue %d, resetBufferCurrentIndex %d", thread_tid(), tailQueue, resetBufferCurrentIndex);
        while (headQueue == tailQueue && resetBufferCurrentIndex == 0)
        {
            //msg("Consumer %i, butter is empty", thread_tid());
            cond_broadcast(&shareBufferNotFull, &theLock);
            cond_wait(&shareBufferNotEmpty, &theLock);
        }
        consumer_digit = shareBuffer[tailQueue];
        tailQueue = (tailQueue+1) % DATASIZE;
        totalSum += consumer_digit;
        if(tailQueue == 0) { resetBufferCurrentIndex = 0; }
        msg("Consumer %i grabbed: %d, Sum = %d", thread_tid(), consumer_digit, totalSum);
        cond_broadcast(&shareBufferNotFull, &theLock);
        thread_print_metrics();
        lock_release(&theLock);
    }
};

// Function that creates consumer threads
void Producer_func(void *aux) {
    int producer_order = 0;
    while( consumerData[producer_order] != '\0'){

        lock_acquire(&theLock);
        if (totalSum == 215) {
            thread_exit();
        }

        if (producer_order == 0) {
            msg("Producer ID %i was the %i producer to start", thread_tid(), theOrder);
            producer_order = theOrder;
            theOrder++;
        }
        while (headQueue == tailQueue && resetBufferCurrentIndex == 1)
        {
            msg("Producer ID %i, broadcasting shareBuffer is not empty...", thread_tid());
            cond_broadcast(&shareBufferNotEmpty, &theLock);
            cond_wait(&shareBufferNotFull, &theLock);
        }
        if (producer_order < (PRODUCERDATASIZE - 1)) {
            producer_digit = consumerData[producer_order];
            shareBuffer[headQueue] = producer_digit;
            //msg("It produced %i at the shareBuffer index of %i", producer_digit, headQueue);
            headQueue = (headQueue + 1) % DATASIZE;
            //msg("Producer ID %i has producer_order index of %i", thread_tid(), producer_order);
            producer_order = producer_order + NUMPRODUCERS;
            if (headQueue == 0) { resetBufferCurrentIndex = 1; }
        }
        cond_broadcast(&shareBufferNotEmpty, &theLock);
        lock_release(&theLock);
    }
    thread_print_metrics();
};