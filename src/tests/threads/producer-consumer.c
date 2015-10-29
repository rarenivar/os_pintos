// producer consumer program
#include <stdio.h>
#include "threads/thread.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "tests/threads/tests.h"

#define PRIORITY 1
#define DATASIZE 5

// condition variables structs, defined in synch.h
static struct condition bufferNotFull;
static struct condition bufferNotEmpty;
// for the buffer
static char buffer[DATASIZE];
// data buffer
char consumerData[] = "abcde";
// buffer positions
static int head = 0;
static int tail = 0;
static int wrap = 0;
// lock
static struct lock mutex;

void GenerateProducerConsumer(int numberOfProducers,int numberOfConsumers);
void Producer_func(void *aux UNUSED);
void Consumer_func(void *aux UNUSED);

// test method
void test_producer_consumer(void) {

    // initializing lock and conditions
    lock_init(&mutex);
    cond_init(&bufferNotEmpty);
    cond_init(&bufferNotFull);

    //GenerateProducerConsumer(2, 7);
    GenerateProducerConsumer(2, 2);

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
    char character;
    for(;;) {
        // acquire lock
        lock_acquire(&mutex);
        msg("\n---------------------------------");
        msg("Consumer ID %i acquired lock, tail %d, wrap %d", thread_tid(), tail, wrap);
        // make sure there is data in the buffer
        while (head == tail && wrap == 0)
        {
            msg("Consumer ID %i, broadcasting there is no data!", thread_tid());
            // wakes up all threadsbuth
            cond_broadcast(&bufferNotFull, &mutex);
            cond_wait(&bufferNotEmpty, &mutex);
        }
        // grab data from the buffer and update tail pointer
        character = buffer[tail];
        tail = (tail+1) % DATASIZE;
        if(tail == 0) {
            wrap -= 1;
        }
        msg("Consumer grabbed: %c", character);
        // broadcast that the buffer is not full and release lock
        cond_broadcast(&bufferNotFull, &mutex);
        msg("About to release lock, tail %d, wrap %d", tail, wrap);
        lock_release(&mutex);
        msg("----------------------------");
    }
};

// Function that creates consumer threads
void
Producer_func(void *aux UNUSED) {
    int index = 0;
    char character;
    // check to see that there is a character in the data array
    while( consumerData[index] != '\0'){

        // acquire lock
        lock_acquire(&mutex);
        msg("\n*********************************");
        msg("Producer ID %i acquired lock, index %d, head %d, wrap %d", thread_tid(), index, head, wrap);
        // broadcast if the buffer is not empty
        // so the consumer can take an item
        while (head == tail && wrap == 1)
        {
            msg("Producer ID %i, broadcasting...buffer is not empty", thread_tid());
            cond_broadcast(&bufferNotEmpty, &mutex);
            cond_wait(&bufferNotFull, &mutex);
        }
        // save the data to the buffer
        character =  consumerData[index];
        buffer[head] = character;
        // update the pointers
        index += 1;
        head = (head + 1) % DATASIZE;
        msg("Producer produced %c", character);
        /* check if we have a wrap-around */
        if(head == 0) {
            wrap += 1;
        }
        // need to broadcast that the buffer is not empty
        cond_broadcast(&bufferNotEmpty, &mutex);
        // release the lock
        msg("About to release lock, index %d, head %d, wrap %d", index, head, wrap);
        lock_release(&mutex);
        msg("****************************");
    }
};