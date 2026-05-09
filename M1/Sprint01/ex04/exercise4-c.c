#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int
main(void)
{
  int i;
  int status;
  pid_t pids[4];

  for (i = 0; i < 4; i++) {
    pids[i] = fork();

    if (pids[i] == 0) {
      sleep(1);
      exit(0);
    }
  }

  for (i = 0; i < 4; i++) {
    if (pids[i] % 2 != 0) {
      printf("Waiting for odd child with PID: %d\n", pids[i]);
      waitpid(pids[i], &status, 0);
    }
  }

  printf("End of execution!\n");
  return 0;
}
