/*
 * Tests producer/consumer communication with different numbers of threads.
 * Automatic checks only catch severe problems like crashes.
 */

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "lib/kernel/list.h"
#include "devices/timer.h"

#define PRIORITY 0
#define DATASIZE 5


void GenerateProducerConsumer(int numberOfProducers,int numberOfConsumers);
void Producer_func(void *aux UNUSED);
void Consumer_func(void *aux UNUSED);

// data buffer
char  consumerData[] = "hello";

// for the buffer
static char buffer[DATASIZE];

// buffer position
static int head = 0;
static int tail = 0;
static int wrap = 0;

// lock
static struct lock mutex;

// condition variables structs, defined in synch.h
static struct condition notFull;
static struct condition notEmpty;

// test method
void
test_producer_consumer(void)
{
    // lock
    lock_init(&mutex);
    // conditions
    cond_init(&notEmpty);
    cond_init(&notFull);

    /*GenerateProducerConsumer(0, 0);
    GenerateProducerConsumer(1, 0);
    GenerateProducerConsumer(0, 1);
    GenerateProducerConsumer(1, 1);
    GenerateProducerConsumer(3, 1);
    GenerateProducerConsumer(1, 3);
    GenerateProducerConsumer(4, 4);
    GenerateProducerConsumer(7, 2);
    GenerateProducerConsumer(2, 7);*/
    GenerateProducerConsumer(6, 6);
    pass();
}

// generate the threads for the consumers and producers
void
GenerateProducerConsumer(int numberOfProducers, int numberOfConsumers)
{
    /*
     Function:
     tid_t thread_create (const char *name, int priority, thread_func *func, void *aux)
     Creates and starts a new thread named name with the given priority, returning the new thread's tid.
     The thread executes func, passing aux as the function's single argument.

     thread_create() allocates a page for the thread's struct thread and stack and initializes its members,
     then it sets up a set of fake stack frames for it (see section A.2.3 Thread Switching). The thread is
     initialized in the blocked state, then unblocked just before returning, which allows the new thread to
     be scheduled (see Thread States).

     Type: void thread_func (void *aux)
     This is the type of the function passed to thread_create(), whose aux argument is passed along as the
     function's argument.
    */
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


/*
 * Consumes all characters placed in the buffer
 * buffer, and waits if there is nothing more to consumer.
 */
void
Consumer_func(void *aux UNUSED){
    char character;

    /* as long as there are characters in the buffer */
    while(true)
    {
        // this is the start of the thread critical section
        lock_acquire(&mutex);

        //msg("consumer_id %i", thread_tid());

        /* check if there is a character left in the buffer */
        while (head == tail && wrap == 0)
        {
            /* if no character is left, inform the producer threads
             to get work done and wait until a character has been placed */

            /* Wakes up all threads, if any, waiting on COND (protected by
               LOCK).  LOCK must be held before calling this function.

               An interrupt handler cannot acquire a lock, so it does not
               make sense to try to signal a condition variable within an
               interrupt handler. */
            cond_broadcast(&notFull, &mutex);
            /* Atomically releases LOCK and waits for COND to be signaled by
               some other piece of code.  After COND is signaled, LOCK is
               reacquired before returning.  LOCK must be held before calling
               this function.

               The monitor implemented by this function is "Mesa" style, not
               "Hoare" style, that is, sending and receiving a signal are not
               an atomic operation.  Thus, typically the caller must recheck
               the condition after the wait completes and, if necessary, wait
               again.

               A given condition variable is associated with only a single
               lock, but one lock may be associated with any number of
               condition variables.  That is, there is a one-to-many mapping
               from locks to condition variables.

               This function may sleep, so it must not be called within an
               interrupt handler.  This function may be called with
               interrupts disabled, but interrupts will be turned back on if
               we need to sleep. */
            cond_wait(&notEmpty, &mutex);
        }

        /* take a character from the buffer */
        character = buffer[tail];

        /* update read pointer */
        tail = (tail+1) % DATASIZE;

        /* check if we have had a wrap-around */
        if(tail == 0) {
            wrap--;
        }

        /* check that there isn't more than a single wrap-around
         of the head pointer */
        ASSERT(wrap == 0 || wrap == 1);

        /* inform the producer threads that the buffer is not full (anymore) */
        cond_broadcast(&notFull, &mutex);

        /* leave the critical section */
        lock_release(&mutex);
    }
};

/*
 * Produces characters based on the string text
 * and puts them into the buffer buffer.
 *
 * Terminates when all characters have been placed in the buffer.
 */
void
Producer_func(void *aux UNUSED){
    /* local variables */
    int index = 0;
    char character;

    /* as long as we have characters to produce */
    while( consumerData[index] != '\0'){

        /* enter critical section */
        lock_acquire(&mutex);

        //msg("producer_id %i", thread_tid());

        /* check if buffer is full */
        while (head == tail && wrap == 1)
        {
            /* state that the buffer is not empty and wait
              for consumer to free a slot of the buffer */
            cond_broadcast(&notEmpty, &mutex);
            cond_wait(&notFull, &mutex);
        }

        /* fetch the next character from the text
         and save it to the buffer */
        character =  consumerData[index];
        buffer[head] = character;

        /* update the writer & reader indexs */
        head = (head + 1) % DATASIZE;
        index++;

        /* check if we have a wrap-around */
        if(head == 0) {
            wrap++;
        }

        /* assert that the head can only be one
         wrap-around in front of tail */
        ASSERT(wrap == 0 || wrap == 1);

        /* wake up consumers, state that the buffer is not
         empty any more */
        cond_broadcast(&notEmpty, &mutex);

        /* leave critical section */
        lock_release(&mutex);
    }
};