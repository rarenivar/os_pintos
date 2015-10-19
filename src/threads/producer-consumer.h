//
// Created by arenivar on 10/18/15.
//

//#ifndef PINTOS_PRODUCER_CONSUMER_H
//#define PINTOS_PRODUCER_CONSUMER_H

//#endif //PINTOS_PRODUCER_CONSUMER_H

#ifndef PRODUCER_CONSUMER_H
#define PRODUCER_CONSUMER_H

/* Initializes all locks and conditions */
void initialize(void);

/* Reads a charater from the source string and writes it into the buffer
 * It terminates when it reaches the end of the source string */
void producer(void *aux);

/* Consumes a character from the shared buffer and prints it.
 * It termintates if the all produced characters have been consumed*/
void consumer(void *aux);

/* Creates producers and consumers threads */
void producer_consumer(unsigned int num_producer, unsigned int num_consumer);

#endif