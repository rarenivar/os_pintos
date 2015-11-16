#include "tests/threads/tests.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "devices/timer.h"
#include "threads/malloc.h"
#include "lib/stdint.h"

/* 2^16. Any larger starts causing weird overflow errors or page faults. */
#define NUM_ELEMENTS 65536
#define NUM_THREADS 8

/*
 * Array & sum data. Thread tick counts are pretty low for a single pass
 * over the array. So multiple passes will up those numbers and hopefully
 * provide better data for the schedulers.
 */ 
static uint64_t *glbArray;
#define NUM_ITERATIONS 88 // TODO: up this to 256 after metrics implemented
static uint64_t glbSums[NUM_ITERATIONS];


struct threadData
{
  uint32_t threadNum;
  tid_t threadId;
};

struct threadData glbThreadInitData[NUM_THREADS];

/*
 * Synchronization variables & functions
 */
static struct lock barrierLock;
static struct condition barrierCondition;
static int barrierThreadCount;
static void barrierInit(void);
static void myBarrier(void);

static struct lock glbThreadFinishLock;
static struct condition glbThreadFinishCondition;

thread_func reductionFunction;
static void reduction(int iteration, uint32_t threadNum);

static void initializeArray(void);

void
test_parallel_reduction (void) 
{
  // Makes parallel reduction easier to code if NUM_ELEMENTS is evenly
  // divisible by NUM_THREADS.
  ASSERT(NUM_ELEMENTS % NUM_THREADS == 0);

  // Prefixing "--" to all messages to distinguish them from other PintOS
  // output.
  msg("--Starting parallel_reduction (tid == %d)...", thread_tid());

  glbArray = malloc(sizeof(unsigned long long) * NUM_ELEMENTS);
  initializeArray();

  barrierInit();
  lock_init(&glbThreadFinishLock);
  cond_init(&glbThreadFinishCondition);

  uint32_t ii;
  for (ii = 0; ii < NUM_THREADS; ++ii)
  {
    glbThreadInitData[ii].threadNum = ii;
    glbThreadInitData[ii].threadId = thread_create("ReductionThread", 0,
                                                   reductionFunction,
                                                   &(glbThreadInitData[ii]));
  }

  lock_acquire(&glbThreadFinishLock);
  cond_wait(&glbThreadFinishCondition, &glbThreadFinishLock);
  lock_release(&glbThreadFinishLock);

  uint64_t expectedSum = (NUM_ELEMENTS + 1ull) * (NUM_ELEMENTS / 2);
  msg("--Expected Sum: %u", expectedSum);
  int allSumsGood = 1;
  for (ii = 0; ii < NUM_ITERATIONS; ++ii)
  {
    if (glbSums[ii] != expectedSum)
    {
      msg("--Calculated Sum %d: %u (did not match expected)", ii, glbSums[ii]);
      allSumsGood = 0;
    }
  }

  if (allSumsGood)
  {
    msg("--All calculated sums match expected.");
  }

  thread_print_global_metrics();
  msg("--Finished parallel_reduction");
}

/**
 * Initialize glbArray (glbArray[ii] = ii)
 */
void initializeArray(void)
{
  uint32_t ii = 0;
  for (ii = 0; ii < NUM_ELEMENTS; ++ii)
  {
    glbArray[ii] = ii+1;
  }
}

/**
 * Initialize variables required to implement the barrier.
 */
static void barrierInit(void)
{
  lock_init(&barrierLock);
  cond_init(&barrierCondition);
  barrierThreadCount = 0;
}

/**
 * Actual barrier, not like the crappy one provided in synch.h. Each thread
 * that enters blocks until all threads have entered. When the last thread
 * enters, then all are released.
 */
static void myBarrier()
{
  lock_acquire(&barrierLock);
  barrierThreadCount++;

  if (barrierThreadCount < NUM_THREADS)
  {
    cond_wait(&barrierCondition, &barrierLock);
  }
  else
  {
    barrierThreadCount = 0;
    cond_broadcast(&barrierCondition, &barrierLock);
  }
  lock_release(&barrierLock);
}

/*
 * Check to determine if the current thread is the master 
 */
inline int masterThread(uint32_t theThreadNum)
{
  return theThreadNum == 0;
}

/**
 * Function to be called by each thread to do it's part in the
 * reduction.
 *
 * @param iteration
 *          which reduction iteration is being worked on.
 * @param threadNum
 *          thread number
 */
void reduction(int iteration, uint32_t threadNum)
{
  uint32_t stride = NUM_ELEMENTS / 2;
  int round = 1;
  for (;stride >= NUM_THREADS; stride /= 2, ++round)
  {
    uint32_t element = 0 + threadNum;

    for (; element < stride; element += NUM_THREADS)
    {
      glbArray[element] += glbArray[element + stride];
    }

    myBarrier();
  }

  // Have master polish off the last set of sums
  if (masterThread(threadNum))
  {
    int ii = 1;
    for (; ii < NUM_THREADS; ++ii)
    {
      glbArray[0] += glbArray[ii];
    }
    glbSums[iteration] = glbArray[0];
  }
}

/**
 * Parallel reduction thread function.
 *
 * @param aux
 *          data passed from creating thread.
 */
void reductionFunction(void *aux)
{
  struct threadData *thread = (struct threadData*) aux;

  msg("--Starting reductionFunction, thread %d", thread_tid());

  myBarrier();

  int32_t iteration = 0;
  for (; iteration < NUM_ITERATIONS; ++iteration)
  {
    if (masterThread(thread->threadNum))
    {
      initializeArray();
    }
    myBarrier();
    reduction(iteration, thread->threadNum);
    myBarrier();
  }

  // PintOS has no thread join mechanism, so in order for the
  // main function to correctly wait for threads to complete this
  // counter is required.
  lock_acquire(&glbThreadFinishLock);

  if (masterThread(thread->threadNum))
  {
    cond_broadcast(&glbThreadFinishCondition, &glbThreadFinishLock);
  }
  msg("--Finished reductionFunction, thread %d", thread_tid());
  thread_print_metrics();
  lock_release(&glbThreadFinishLock);
}
