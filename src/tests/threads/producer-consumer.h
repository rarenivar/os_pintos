//
// Created by arenivar on 10/19/15.
//

//#ifndef PINTOS_PRODUCER_CONSUMER_H
//#define PINTOS_PRODUCER_CONSUMER_H

//#endif //PINTOS_PRODUCER_CONSUMER_H

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"

/* Source String:
 * A producer gets a charater from the source string then puts the
 * character into the buffer */
char src[] = "HELLO WORLD";

/* The size of the source string */
#define SRC_LEN 12

/* The size of the buffer */
#define BUF_SIZE 10

char buf[BUF_SIZE];

/* Number of characters in the buffer. Range is [0, BUF_SIZ] */
unsigned int buf_n = 0;
unsigned int head = 0; // Index of next char for the producer to write
unsigned int tail = 0; // Index of next char for the consumer to get

/* Total characters produced by all producers. Updated as producers
 * produce chars and decremented as consumers consume */
unsigned int total_char = 0;

struct lock buf_lock; // monitor lock on the buffer
struct lock total_char_lock; // lock on the total_char variable
struct condition not_empty; // Signaled when the buffer is not empty
struct condition not_full; // Signaled when the buffer is not full

void initialize(void);
void producer(void *aux);
void consumer(void *aux);
void producer_consumer(unsigned int num_producer, unsigned int num_consumer);

/* Creates producers and consumers threads */
void producer_consumer(unsigned int num_producer, unsigned int num_consumer)
{
    initialize();
    lock_acquire(&total_char_lock);
    total_char += num_producer * SRC_LEN; // updates the total produced chars.
    lock_release(&total_char_lock);
    unsigned int i;
    for(i = 0; i < num_producer; i++)
    {
        thread_create ("Producer", PRI_DEFAULT, producer, 0);
    }
    for(i = 0; i < num_consumer; i++)
    {
        thread_create("Consumer", PRI_DEFAULT, consumer, 0);
    }
}

/* Initializes all locks and conditions */
void initialize(void)
{
    lock_init(&buf_lock);
    cond_init(&not_full);
    cond_init(&not_empty);
    lock_init(&total_char_lock);
}

/* Reads a charater from the source string and writes it into the buffer
 * It terminates when it reaches the end of the source string */
void producer(void *aux)
{
    /* The index of the next character to be consumed by this producer */
    unsigned int next = 0;
    while(true)
    {
        if (next >= SRC_LEN)
            return;
        lock_acquire(&buf_lock);
        while(buf_n == BUF_SIZE)
            cond_wait(&not_empty, &buf_lock);
        buf[head] = src[next];
        next++;
        buf_n++;
        head = (head + 1) % BUF_SIZE;
        cond_signal(&not_full, &buf_lock);
        lock_release(&buf_lock);
    }
}

/* Consumes a character from the shared buffer and prints it.
 * It termintates if the all produced characters have been consumed*/
void consumer(void *aux)
{
    while(true)
    {
        lock_acquire(&total_char_lock);
        lock_acquire(&buf_lock);

        /* Checks if there are still available produced characters */
        if(total_char == 0)
        {
            lock_release(&total_char_lock);
            lock_release(&buf_lock);
            return;
        }
        while(buf_n == 0)
            cond_wait(&not_full, &buf_lock);
        printf("%c", buf[tail]);
        buf_n--;
        total_char--;
        tail = (tail + 1) % BUF_SIZE;
        cond_signal(&not_empty, &buf_lock);
        lock_release(&buf_lock);
        lock_release(&total_char_lock);
    }
}