#include <stdio.h>
#include <unistd.h>

int
main(void)
{
  fork();
  fork();
  printf("MESCC");
  fork();
  printf("MESCC");
  return 0;
}
