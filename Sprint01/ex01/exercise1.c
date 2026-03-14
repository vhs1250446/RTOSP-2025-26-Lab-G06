#include <stdio.h>
#include <unistd.h>

int
main(void)
{
  int x = 0;
  pid_t p;

  p = fork();

  if (p == 0) {
	x = x + 2;
	printf("Step 1. x = %d\n", x);
  } else {
	x = x - 2;
	printf("Step 2. x = %d\n", x);
	printf("Step 3. p = %d; x = %d\n", p, x);
  }
  return 0;
}
