#include <stdio.h>
#include <unistd.h>

int
main(void)
{
  fork();
  fork();
  printf("MESCC\n");
  fork();
  printf("MESCC\n");
  return 0;
}
