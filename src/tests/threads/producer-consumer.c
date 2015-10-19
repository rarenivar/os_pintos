//
// Created by arenivar on 10/18/15.
//

#include "threads/producer-consumer.h"

void test_producer_consumer(void)
{
    msg("let's see if this gets called");
    producer_consumer(0, 0);
    //producer_consumer(1, 0);
    producer_consumer(0, 1);
    producer_consumer(1, 1);
    //producer_consumer(3, 1);
    producer_consumer(1, 3);
    producer_consumer(4, 4);
    //producer_consumer(7, 2);
    producer_consumer(2, 7);
    producer_consumer(6, 6);
    pass();
}

