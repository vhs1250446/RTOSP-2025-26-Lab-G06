#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <sys/wait.h>

#define BUFFER_SIZE 500
#define CHILDS 5
#define SEED 42

void
init_numbers_buffer(int *buffer, int size)
{
  srand(SEED);
  for (int i = 0; i < size; ++i) {
	buffer[i] = rand() % 255; // modulo arithmetic, ty Serge Lang
  }
}

// Errr, calling it a slice in C seems like a misnomer, ¯\_(ツ)_/¯
int*
get_slice_start(int index, int *buffer, int items_per_child)
{
  return &buffer[index * items_per_child];
}

int
find_max(int *buffer, int size)
{
  if (size <= 0) return 0;
  int max_val = buffer[0];
  for (int i = 1; i < size; ++i) {
	if (buffer[i] > max_val) {
	  max_val = buffer[i];
	}
  }
  return max_val;
}

int
sum(int *buffer, int size)
{
  int total = 0;
  for (int i = 0; i < size; ++i) {
	total += buffer[i];
  }
  return total;
}

int
main(void)
{
  int numbers[BUFFER_SIZE];
  int local_maximums[CHILDS];
  int pipefd[2];
  int items_per_child = BUFFER_SIZE / CHILDS;

  if (pipe(pipefd) == -1) {
	err(EXIT_FAILURE, "pipe");
  }

  init_numbers_buffer(numbers, BUFFER_SIZE);

  for (int i = 0; i < CHILDS; ++i) {
	pid_t pid = fork();
	if (pid == -1) {
	  err(EXIT_FAILURE, "fork");
	}

	if (pid == 0) {
	  close(pipefd[0]);

	  int *my_slice = get_slice_start(i, numbers, items_per_child);
	  int my_max = find_max(my_slice, items_per_child);

	  if (write(pipefd[1], &my_max, sizeof(int)) != sizeof(int)) {
		err(EXIT_FAILURE, "write");
	  }

	  close(pipefd[1]);
	  exit(EXIT_SUCCESS); // child must exit to not loop
	}
  }

  close(pipefd[1]);

  int received_count = 0;
  int current_max = 0;
  while (read(pipefd[0], &current_max, sizeof(int)) > 0) {
	printf("Read %d from pipe.\n", current_max);
	local_maximums[received_count++] = current_max;
  }

  for (int i = 0; i < CHILDS; i++) {
	wait(NULL);
  }

  int total_sum = sum(local_maximums, CHILDS);
  printf("Local Max Sum= %d\n", total_sum);

  close(pipefd[0]);
  return 0;
}
