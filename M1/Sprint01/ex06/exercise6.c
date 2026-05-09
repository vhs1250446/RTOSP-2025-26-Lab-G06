#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <time.h>

#define MAGIC_NUMBER 42
int
main(void)
{
  int status;
  pid_t pid = fork();

  if (pid < 0) {
    return 1;
  }

  if (pid == 0) { // first child
    srand(MAGIC_NUMBER);
    int r = rand() % 6; // [0;5]

    printf("Numero gerado: %d\n", r);
    exit(r);
  }
  else {
    printf("Processo pai aguarda o valor do primeiro filho...\n");
    wait(&status);

    int num_filhos = WEXITSTATUS(status);

    printf("Processo pai a criar %d processos filhos...\n", num_filhos);

    for (int i = 0; i < num_filhos; i++) {
      pid_t p_novo = fork();

      if (p_novo == 0) {
	printf("Processo filho gerado. Ordem: %d | PID: %d\n",
	       i + 1, getpid());
	exit(0);
      }
    }

    for (int i = 0; i < num_filhos; i++) {
      wait(NULL);
    }

    printf("Processo pai concluiu a execucao.\n");
  }

  return 0;
}
