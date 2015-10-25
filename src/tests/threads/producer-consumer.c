// producer consumer program
#include <stdio.h>
#include "threads/thread.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "tests/threads/tests.h"

#define PRIORITY 0
#define DATASIZE 5

// condition variables structs, defined in synch.h
static struct condition notFull;
static struct condition notEmpty;
// for the buffer
static char buffer[DATASIZE];
// data buffer
char consumerData[] = "hello";
// buffer positions
static int head = 0;
static int tail = 0;
// lock
static struct lock mutex;

void GenerateProducerConsumer(int numberOfProducers,int numberOfConsumers);
void Producer_func(void *aux UNUSED);
void Consumer_func(void *aux UNUSED);

// test method
void test_producer_consumer(void) {

    // initializing lock and conditions
    lock_init(&mutex);
    cond_init(&notEmpty);
    cond_init(&notFull);

    GenerateProducerConsumer(2, 7);
    GenerateProducerConsumer(6, 6);

    // if no errors, the tests passed
    pass();
}

// generate the threads for the consumers and producers
void GenerateProducerConsumer(int numberOfProducers, int numberOfConsumers) {
    int i;
    //we'll now create the threads for the producers and consumers
    for(i = 0; i < numberOfProducers; i++)
    {
        thread_create ("Producer", PRIORITY, Producer_func, &mutex);
    }
    for(i = 0; i < numberOfConsumers; i++)
    {
        thread_create ("Consumer", PRIORITY, Consumer_func, &mutex);
    }
}

// Function that creates consumer threads
void Consumer_func(void *aux UNUSED) {

    int once = 0;
    char character;

    while(true) {
        // acquire lock
        lock_acquire(&mutex);
        // make sure there is data in the buffer
        while (head == tail)
        {
            if (once == 0) {
                msg("Consumer ID: %i", thread_tid());
                once = 1;
            }
            // wakes up all threads
            cond_broadcast(&notFull, &mutex);
            cond_wait(&notEmpty, &mutex);
        }

        // grab data from the buffer and update tail pointer
        character = buffer[tail];
        tail = (tail+1) % DATASIZE;
        // broadcast that the buffer is not full and release lock
        cond_broadcast(&notFull, &mutex);
        lock_release(&mutex);
    }
};

// Function that creates consumer threads
void
Producer_func(void *aux UNUSED) {

    int once = 0;
    int index = 0;
    char character;

    // check to see that there is a character in the data array
    while( consumerData[index] != '\0'){

        // acquire lock
        lock_acquire(&mutex);

        // broadcast if the buffer is not empty
        // so the consumer can take an item
        while (head == tail)
        {
            if (once == 0) {
                msg("Producer ID: %i", thread_tid());
                once = 1;
            }
            cond_broadcast(&notEmpty, &mutex);
            cond_wait(&notFull, &mutex);
        }

        // save the data to the buffer
        character =  consumerData[index];
        buffer[head] = character;
        // update the pointers
        index++;
        head = (head + 1) % DATASIZE;
        // need to broadcast that the buffer is not empty
        cond_broadcast(&notEmpty, &mutex);
        // release the lock
        lock_release(&mutex);
    }
};