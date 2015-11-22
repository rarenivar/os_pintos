/**
 * This program creates a specific number of producers
 * and consumers. The producers take a number from the
 * consumerData and place it the the share buffer that
 * they share with the consumers. Then the consumers take
 * the data from the buffer array and added to a common
 * variable created to keep track of the addition.
 * Synchronization primitives are used to make sure
 * that the different processes keep track of the share
 * buffer (when it's empty, the producers put data in it
 * and when it's full, the consumers take data from it).
 */

#include <stdio.h>
#include "threads/thread.h"
#include "threads/synch.h"
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/malloc.h"
#include "devices/timer.h"

#define NUMPRODUCERS 1
#define NUMCONSUMERS 10
#define DATASIZE 5
#define PRODUCERDATASIZE 530
#define DEBUG 0 // true = 1, false = 0

// condition variables structs, define in synch.h
static struct condition shareBufferNotFull;
static struct condition shareBufferNotEmpty;
static int shareBuffer[DATASIZE];
static int resetBufferCurrentIndex = 0;
static int inProgress = 1;
static int numberOfThreadsFinished = 0;

static int consumerData[PRODUCERDATASIZE] = {
    9,2,9,1,2,3,6,9,1,2,6,9,3,1,5,4,4,9,4,5,
    7,6,9,6,7,7,3,1,5,8,4,1,7,9,5,3,7,5,2,3,
    2,2,9,3,9,2,9,1,2,3,6,9,1,2,6,9,3,1,5,4,
    4,9,4,5,7,6,9,6,7,7,3,1,5,8,4,1,7,9,5,3,
    7,5,2,3,2,2,9,3,9,2,9,1,2,3,6,9,1,2,6,9,
    3,1,5,4,4,9,4,5,7,6,9,6,7,7,3,1,5,8,4,1,
    7,9,5,3,7,5,2,3,2,2,9,3,9,2,9,1,2,3,6,9,
    1,2,6,9,3,1,5,4,4,9,4,5,7,6,9,6,7,7,3,1,
    5,8,4,1,7,9,5,3,7,5,2,3,2,2,9,3,9,2,9,1,
    2,3,6,9,1,2,6,9,3,1,5,4,4,9,4,5,7,6,9,6,
    7,7,3,1,5,8,4,1,7,9,5,3,7,5,2,3,2,2,9,3,
    9,2,9,1,2,3,6,9,1,2,6,9,3,1,5,4,4,9,4,5,
    7,6,9,6,7,7,3,1,5,8,4,1,7,9,5,3,7,5,2,3,
    2,2,9,3,
    9,2,9,1,2,3,6,9,1,2,6,9,3,1,5,4,4,9,4,5,
    7,6,9,6,7,7,3,1,5,8,4,1,7,9,5,3,7,5,2,3,
    2,2,9,3,9,2,9,1,2,3,6,9,1,2,6,9,3,1,5,4,
    4,9,4,5,7,6,9,6,7,7,3,1,5,8,4,1,7,9,5,3,
    7,5,2,3,2,2,9,3,9,2,9,1,2,3,6,9,1,2,6,9,
    3,1,5,4,4,9,4,5,7,6,9,6,7,7,3,1,5,8,4,1,
    7,9,5,3,7,5,2,3,2,2,9,3,9,2,9,1,2,3,6,9,
    1,2,6,9,3,1,5,4,4,9,4,5,7,6,9,6,7,7,3,1,
    5,8,4,1,7,9,5,3,7,5,2,3,2,2,9,3,9,2,9,1,
    2,3,6,9,1,2,6,9,3,1,5,4,4,9,4,5,7,6,9,6,
    7,7,3,1,5,8,4,1,7,9,5,3,7,5,2,3,2,2,9,3,
    9,2,9,1,2,3,6,9,1,2,6,9,3,1,5,4,4,9,4,5,
    7,6,9,6,7,7,3,1,5,8,4,1,7,9,5,3,7,5,2,3,
    2,2,9,3
}; // The addition of all of these number is 2580

// pointers for arrays and the lock variable
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

    // creating the producers and consumers
    for(i = 0; i < NUMPRODUCERS; i++)
    {
        thread_create("Producer", 1, Producer_func, &theLock);
    }
    for(i = 0; i < NUMCONSUMERS; i++)
    {
        thread_create("Consumer", 62, Consumer_func, &theLock);
    }
}


// Function that creates consumer threads
void Consumer_func(void *aux) {
    int consumer_digit = 0;
    msg("Consumer %i started", thread_tid());
    while(inProgress) {

        lock_acquire(&theLock);

        // once we know the sum is what we expect, print the global metrics
        if (totalSum == 2580) {
            inProgress = 0;
            msg("Correct addition of consumer data! totalSum = 2580");
        }

        // if the share buffer is not full...
        while (headQueue == tailQueue && resetBufferCurrentIndex == 0/* || totalSum == 2580*/)
        {
            //msg("waiting for the producer");
            if (totalSum == 2580 || inProgress == 0) { break; }
            cond_broadcast(&shareBufferNotFull, &theLock);
            cond_wait(&shareBufferNotEmpty, &theLock);
        }
        consumer_digit = shareBuffer[tailQueue];
        tailQueue = (tailQueue+1) % DATASIZE;
        totalSum += consumer_digit;
        if(tailQueue == 0) { resetBufferCurrentIndex = 0; }
        if (DEBUG) { msg("Consumer %i grabbed: %d, Sum = %d", thread_tid(), consumer_digit, totalSum); }
        cond_broadcast(&shareBufferNotFull, &theLock);
        lock_release(&theLock);
    }
    msg("Consumer %i thread thread terminating", thread_tid());
    numberOfThreadsFinished++;
    thread_print_metrics();
    if (numberOfThreadsFinished == (NUMCONSUMERS + NUMPRODUCERS)) {
        thread_print_global_metrics();
    }
};

// Function that creates consumer threads
void Producer_func(void *aux) {
    int producer_digit = 0;
    msg("Producer %i started", thread_tid());
    int producer_order = 0;

    while( producer_order < (PRODUCERDATASIZE-1)){

        lock_acquire(&theLock);

        if (producer_order == 0) {
            producer_order = theOrder;
            theOrder++;
        }

        //if the share buffer does not contain elements or it's been reset
        while (headQueue == tailQueue && resetBufferCurrentIndex == 1)
        {
            cond_broadcast(&shareBufferNotEmpty, &theLock);
            cond_wait(&shareBufferNotFull, &theLock);
        }
        if (producer_order < (PRODUCERDATASIZE - 1)) {
            producer_digit = consumerData[producer_order];
            shareBuffer[headQueue] = producer_digit;
            if (DEBUG) { msg("Producer %i produced %i at the shareBuffer index of %i", thread_tid(), producer_digit, headQueue); }
            headQueue = (headQueue + 1) % DATASIZE;
            producer_order = producer_order + NUMPRODUCERS;
            if (headQueue == 0) { resetBufferCurrentIndex = 1; }
        }
        cond_broadcast(&shareBufferNotEmpty, &theLock);
        lock_release(&theLock);
    }
    msg("Producer %i thread terminating", thread_tid());
    numberOfThreadsFinished++;
    thread_print_metrics();
    if (numberOfThreadsFinished == (NUMCONSUMERS + NUMPRODUCERS)) {
        thread_print_global_metrics();
    }
};