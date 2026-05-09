#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define MAGIC_NUMBER 42
#define MATRIX_SIZE 16
#define FILL_THREADS 2
#define MULT_THREADS 8
#define ROWS_PER_THREAD (MATRIX_SIZE / MULT_THREADS)

int A[MATRIX_SIZE][MATRIX_SIZE];
int B[MATRIX_SIZE][MATRIX_SIZE];
int C[MATRIX_SIZE][MATRIX_SIZE];

void*
fill_matrix(void *arg)
{
  int matrix_id = *(int *)arg;

  for (int i = 0; i < MATRIX_SIZE; i++) {
    for (int j = 0; j < MATRIX_SIZE; j++) {
      if (matrix_id == 0) {
        A[i][j] = rand() % 10;
      } else {
        B[i][j] = rand() % 10;
      }
    }
  }
  return NULL;
}

void*
multiply_rows(void *arg)
{
  int thread_id = *(int *)arg;

  int start_row = thread_id * ROWS_PER_THREAD;
  int end_row = start_row + ROWS_PER_THREAD;

  for (int i = start_row; i < end_row; i++) {
    for (int j = 0; j < MATRIX_SIZE; j++) {
      C[i][j] = 0;
      for (int k = 0; k < MATRIX_SIZE; k++) {
        C[i][j] += A[i][k] * B[k][j];
      }
    }
  }
  return NULL;
}

int
main(void)
{
  pthread_t fill_t[FILL_THREADS];
  pthread_t mult_t[MULT_THREADS];

  int fill_args[FILL_THREADS];
  int mult_args[MULT_THREADS];

  srand(MAGIC_NUMBER);

  for (int i = 0; i < FILL_THREADS; i++) {
	fill_args[i] = i;
	if (pthread_create(&fill_t[i], NULL, fill_matrix, &fill_args[i]) != 0) {
	  perror("Failed to create fill thread");
	  return EXIT_FAILURE;
	}
  }

  for (int i = 0; i < FILL_THREADS; i++) {
	pthread_join(fill_t[i], NULL);
  }

  for (int i = 0; i < MULT_THREADS; i++) {
	mult_args[i] = i;
	if (pthread_create(&mult_t[i], NULL, multiply_rows, &mult_args[i]) != 0) {
	  perror("Failed to create mult thread");
	  return EXIT_FAILURE;
	}
  }

  for (int i = 0; i < MULT_THREADS; i++) {
	pthread_join(mult_t[i], NULL);
  }

  printf("Resulting %dx%d Matrix (A):\n", MATRIX_SIZE, MATRIX_SIZE);
  for (int i = 0; i < MATRIX_SIZE; i++) {
	for (int j = 0; j < MATRIX_SIZE; j++) {
	  printf("%4d ", A[i][j]);
	}
	printf("\n");
  }

  printf("Resulting %dx%d Matrix (B):\n", MATRIX_SIZE, MATRIX_SIZE);
  for (int i = 0; i < MATRIX_SIZE; i++) {
	for (int j = 0; j < MATRIX_SIZE; j++) {
	  printf("%4d ", B[i][j]);
	}
	printf("\n");
  }

  printf("Resulting %dx%d Matrix (C):\n", MATRIX_SIZE, MATRIX_SIZE);
  for (int i = 0; i < MATRIX_SIZE; i++) {
	for (int j = 0; j < MATRIX_SIZE; j++) {
	  printf("%4d ", C[i][j]);
	}
	printf("\n");
  }

  return EXIT_SUCCESS;
}
