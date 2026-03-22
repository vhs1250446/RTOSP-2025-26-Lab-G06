#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

#define NUM_READERS 3
#define NUM_WRITERS 2
#define NUM_ITERATIONS 3

int sharedData = 0;

pthread_mutex_t resourceMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t readerMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t writerMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t fairnessMutex = PTHREAD_MUTEX_INITIALIZER;

sem_t readSem;
sem_t writeSem;

int readCount = 0;
int writeCount = 0;
int readerWaiting = 0;
int writerWaiting = 0;

void
get_timestamp(char* buffer)
{
  time_t now = time(NULL);
  struct tm* timeinfo = localtime(&now);
  strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", timeinfo);
}

void*
reader_thread(void* arg)
{
  int reader_id = *(int*)arg;
  free(arg);

  for (int i = 0; i < NUM_ITERATIONS; i++) {
    char timestamp[26];

    pthread_mutex_lock(&fairnessMutex);
    pthread_mutex_unlock(&fairnessMutex);

    pthread_mutex_lock(&readerMutex);
    readerWaiting++;
    readCount++;

    get_timestamp(timestamp);
    printf("[%s] Reader %d is WAITING to read (readCount: %d, writeCount: %d)\n",
	   timestamp, reader_id, readCount, writeCount);

    if (readCount == 1) {
      pthread_mutex_unlock(&readerMutex);
      pthread_mutex_lock(&resourceMutex);
      pthread_mutex_lock(&readerMutex);
    }

    readerWaiting--;
    pthread_mutex_unlock(&readerMutex);

    get_timestamp(timestamp);
    printf("[%s] Reader %d START reading. Data: %d\n",
	   timestamp, reader_id, sharedData);
    sleep(1);
    get_timestamp(timestamp);
    printf("[%s] Reader %d END reading. Data: %d\n",
	   timestamp, reader_id, sharedData);

    pthread_mutex_lock(&readerMutex);
    readCount--;

    if (readCount == 0) {
      pthread_mutex_unlock(&readerMutex);
      pthread_mutex_unlock(&resourceMutex);
    } else {
      pthread_mutex_unlock(&readerMutex);
    }
  }

  return NULL;
}

void*
writer_thread(void* arg)
{
  int writer_id = *(int*)arg;
  free(arg);

  for (int i = 0; i < NUM_ITERATIONS; i++) {
    char timestamp[26];

    pthread_mutex_lock(&writerMutex);
    writeCount++;
    writerWaiting++;

    get_timestamp(timestamp);
    printf("[%s] Writer %d is WAITING to write (readCount: %d, writeCount: %d)\n",
	   timestamp, writer_id, readCount, writeCount);

    if (writeCount == 1) {
      pthread_mutex_lock(&fairnessMutex);
    }

    pthread_mutex_unlock(&writerMutex);

    pthread_mutex_lock(&resourceMutex);

    pthread_mutex_lock(&writerMutex);
    writerWaiting--;
    pthread_mutex_unlock(&writerMutex);

    get_timestamp(timestamp);
    printf("[%s] Writer %d START writing. Data: %d -> %d\n",
	   timestamp, writer_id, sharedData, sharedData + 1);
    sharedData++;
    sleep(1);
    get_timestamp(timestamp);
    printf("[%s] Writer %d END writing. Data: %d\n",
	   timestamp, writer_id, sharedData);

    pthread_mutex_unlock(&resourceMutex);

    pthread_mutex_lock(&writerMutex);
    writeCount--;

    if (writeCount == 0) {
      pthread_mutex_unlock(&fairnessMutex);
    }

    pthread_mutex_unlock(&writerMutex);
  }

  return NULL;
}

int
main()
{
  pthread_t readers[NUM_READERS];
  pthread_t writers[NUM_WRITERS];

  printf("\n");
  printf("===== READERS/WRITERS PROBLEM - FAIRNESS AND NO STARVATION =====\n");
  printf("Readers: %d | Writers: %d | Iterations: %d\n\n",
	 NUM_READERS, NUM_WRITERS, NUM_ITERATIONS);

  sem_init(&readSem, 0, 1);
  sem_init(&writeSem, 0, 1);

  for (int i = 0; i < NUM_READERS; i++) {
    int* reader_id = (int*)malloc(sizeof(int));
    *reader_id = i + 1;
    pthread_create(&readers[i], NULL, reader_thread, reader_id);
  }

  for (int i = 0; i < NUM_WRITERS; i++) {
    int* writer_id = (int*)malloc(sizeof(int));
    *writer_id = i + 1;
    pthread_create(&writers[i], NULL, writer_thread, writer_id);
  }

  for (int i = 0; i < NUM_READERS; i++) {
    pthread_join(readers[i], NULL);
  }

  for (int i = 0; i < NUM_WRITERS; i++) {
    pthread_join(writers[i], NULL);
  }

  sem_destroy(&readSem);
  sem_destroy(&writeSem);
  pthread_mutex_destroy(&resourceMutex);
  pthread_mutex_destroy(&readerMutex);
  pthread_mutex_destroy(&writerMutex);
  pthread_mutex_destroy(&fairnessMutex);

  printf("\n===== SIMULATION COMPLETE =====\n");
  printf("Final shared data value: %d\n\n", sharedData);

  return 0;
}
