#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

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
      /*
	From man wait(2):
	If  wstatus  is not NULL, wait() and waitpid() store status information
	in the int to which it points.  This integer can be inspected with  the
	following  macros  (which take the integer itself as an argument, not a
	pointer to it, as is done in wait() and waitpid()!):

	WIFEXITED(wstatus)
	returns true if the child terminated normally, that is, by call‐
	ing exit(3) or _exit(2), or by returning from main().

	WEXITSTATUS(wstatus)
	returns the exit status of the  child.   This  consists  of  the
	least  significant  8 bits of the status argument that the child
	specified in a call to exit(3) or _exit(2) or  as  the  argument
	for a return statement in main().  This macro should be employed
	only if WIFEXITED returned true.
	,*/
      exit(i + 1); 
    }
  }

  for (i = 0; i < 4; i++) {
    if (pids[i] % 2 != 0) {
      waitpid(pids[i], &status, 0);

      if (WIFEXITED(status)) {
	int ordem = WEXITSTATUS(status);
	printf("Filho PID %d terminou. Ordem de criação: %d\n",
	       pids[i], ordem);
      }
    }
  }

  printf("End of execution!\n");
}
