#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int
main(void)
{
  int i;
  int status;

  for (i = 0; i < 4; i++) {
	if (fork() == 0) {
	  sleep(1);
	  printf("End of execution!\n");
	}
  }
  return 0;
}
