#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MAGIC_NUMBER 42
#define NUM_CLIENTS 100

typedef struct {
  int number;
  char name[20];
  float balance;
} Client;

Client clients[NUM_CLIENTS];
float average_balance = 0.0;

int negative_indices[NUM_CLIENTS];
int negative_count = 0;
int done_checking = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// thread 1: verifies negative balances and signals them
void*
verify_negative_balances(void* arg)
{
  for (int i = 0; i < NUM_CLIENTS; i++) {
  	if (clients[i].balance < 0.0) {
		pthread_mutex_lock(&mutex);
		negative_indices[negative_count] = i;
		negative_count++;
		pthread_cond_signal(&cond);
		pthread_mutex_unlock(&mutex);
	  }
  }

  pthread_mutex_lock(&mutex);
  done_checking = 1;
  pthread_cond_broadcast(&cond);
  pthread_mutex_unlock(&mutex);

  return NULL;
}

// thread 2: prints clients with negative balances
void*
print_negative_balances(void* arg)
{
  int printed_count = 0;

  while (1) {
  	pthread_mutex_lock(&mutex);

	  // wait until there is a new negative client to print or checking is done
	  while (printed_count == negative_count && !done_checking) {
		pthread_cond_wait(&cond, &mutex);
	  }

	  if (printed_count == negative_count && done_checking) {
		pthread_mutex_unlock(&mutex);
		break;
	  }

	  while (printed_count < negative_count) {
		int idx = negative_indices[printed_count];
		printf("[Alert] Client #%d (%s) has a negative balance: $%.2f\n",
			   clients[idx].number, clients[idx].name, clients[idx].balance);
		printed_count++;
	  }

	  pthread_mutex_unlock(&mutex);
  }
  return NULL;
}

// thread 3: computes the average balance
void*
compute_average_balance(void* arg)
{
  float sum = 0.0;
  for (int i = 0; i < NUM_CLIENTS; i++) {
  	sum += clients[i].balance;
  }
  average_balance = sum / NUM_CLIENTS;
  printf("\n---> Computed Average Balance: $%.2f\n\n", average_balance);
  return NULL;
}

// thread 4: eliminates negative balances
void*
eliminate_negative_balances(void* arg)
{
  printf("[Action] Eliminating negative balances...\n");
  for (int i = 0; i < NUM_CLIENTS; i++) {
  	if (clients[i].balance < 0.0) {
		clients[i].balance = 0.0;
	  }
  }
  printf("[Action] All negative balances have been reset to $0.00.\n");
  return NULL;
}

int
main()
{
  srand(MAGIC_NUMBER);

  for (int i = 0; i < NUM_CLIENTS; i++) {
  	clients[i].number = i + 1;
	  sprintf(clients[i].name, "Client_%d", i + 1);
	  clients[i].balance = ((float)rand() / RAND_MAX) * 2000.0 - 500.0;
  }

  pthread_t t1, t2, t3, t4;

  pthread_create(&t1, NULL, verify_negative_balances, NULL);
  pthread_create(&t2, NULL, print_negative_balances, NULL);
  pthread_create(&t3, NULL, compute_average_balance, NULL);
  pthread_join(t1, NULL);
  pthread_join(t2, NULL);
  pthread_join(t3, NULL);

  if (average_balance < 0.0) {
  	pthread_create(&t4, NULL, eliminate_negative_balances, NULL);
	  pthread_join(t4, NULL);
  } else {
  	printf("[Conformity] Statement of Conformity: The overall average balance is positive. System is stable.\n");
  }

  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&cond);

  return 0;
}
