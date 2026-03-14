#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MAGIC_NUMBER 42
#define ARRAY_SIZE 1000
#define NUM_THREADS 10
#define CHUNK_SIZE (ARRAY_SIZE / NUM_THREADS)

typedef struct {
  int thread_id;
  int *data_array;
  int *results_array;
} ThreadArgs;

void*
find_local_max(void *arg)
{
  ThreadArgs *args = (ThreadArgs *)arg;
  int id = args->thread_id;

  int start_index = id * CHUNK_SIZE;
  int end_index = start_index + CHUNK_SIZE;

  int local_max = args->data_array[start_index];
  for (int i = start_index + 1; i < end_index; ++i) {
    if (args->data_array[i] > local_max) {
      local_max = args->data_array[i];
    }
  }

  args->results_array[id] = local_max;

  return NULL;
}

int
main(void)
{
  int large_array[ARRAY_SIZE];
  int small_array[NUM_THREADS];
  pthread_t threads[NUM_THREADS];
  ThreadArgs thread_args[NUM_THREADS];

  srand(MAGIC_NUMBER);
  for (int i = 0; i < ARRAY_SIZE; ++i) {
    large_array[i] = rand() % 256;
  }

  for (int i = 0; i < NUM_THREADS; ++i) {
    thread_args[i].thread_id = i;
    thread_args[i].data_array = large_array;
    thread_args[i].results_array = small_array;

    if (pthread_create(&threads[i], NULL, find_local_max, &thread_args[i]) != 0) {
      perror("Failed to create thread");
      return EXIT_FAILURE;
    }
  }

  for (int i = 0; i < NUM_THREADS; ++i) {
    if (pthread_join(threads[i], NULL) != 0) {
      perror("Failed to join thread");
      return EXIT_FAILURE;
    }
  }

  int global_max = small_array[0];
  for (int i = 1; i < NUM_THREADS; ++i) {
    if (small_array[i] > global_max) {
      global_max = small_array[i];
    }
  }

  printf("The global maximum value is: %d\n", global_max);

  return EXIT_SUCCESS;
}
