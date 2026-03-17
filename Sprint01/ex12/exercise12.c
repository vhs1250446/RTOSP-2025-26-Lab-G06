#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

/* ========================================================================= *
 * PART 1: UNBOUNDED BUFFER (LINKED LIST) *
 * ========================================================================= */

struct Node {
    int data;
    struct Node* next;
};

struct Node* head = NULL;
struct Node* tail = NULL;

pthread_mutex_t unb_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t unb_cond = PTHREAD_COND_INITIALIZER;

void* unbounded_producer(void* arg) {
    for (int i = 1; i <= 5; i++) {
        struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
        newNode->data = i;
        newNode->next = NULL;

        pthread_mutex_lock(&unb_mutex);
        
        if (tail == NULL) {
            head = tail = newNode;
        } else {
            tail->next = newNode;
            tail = newNode;
        }
        printf("[Unbounded] Producer generated: %d\n", i);
        
        pthread_cond_signal(&unb_cond);
        pthread_mutex_unlock(&unb_mutex);
        
        sleep(1);
    }
    pthread_exit(NULL);
}

void* unbounded_consumer(void* arg) {
    for (int i = 1; i <= 5; i++) {
        pthread_mutex_lock(&unb_mutex);
        
        while (head == NULL) {
            pthread_cond_wait(&unb_cond, &unb_mutex);
        }
        
        struct Node* temp = head;
        int data = temp->data;
        head = head->next;
        if (head == NULL) {
            tail = NULL;
        }
        free(temp);
        
        printf("[Unbounded] Consumer consumed: %d\n", data);
        pthread_mutex_unlock(&unb_mutex);
        
        sleep(1);
    }
    pthread_exit(NULL);
}


/* ========================================================================= *
 * PART 2: BOUNDED BUFFER (FIXED ARRAY) *
 * ========================================================================= */

#define BUFFER_SIZE 3

int buffer[BUFFER_SIZE];
int in = 0;
int out = 0;

sem_t empty_slots;
sem_t full_slots;
pthread_mutex_t bnd_mutex;

void* bounded_producer(void* arg) {
    for (int i = 1; i <= 5; i++) {
        sem_wait(&empty_slots);
        pthread_mutex_lock(&bnd_mutex);
        
        buffer[in] = i;
        printf("[Bounded]   Producer generated: %d at index %d\n", i, in);
        in = (in + 1) % BUFFER_SIZE;
        
        pthread_mutex_unlock(&bnd_mutex);
        sem_post(&full_slots);
        
        sleep(1);
    }
    pthread_exit(NULL);
}

void* bounded_consumer(void* arg) {
    for (int i = 1; i <= 5; i++) {
        sem_wait(&full_slots);
        pthread_mutex_lock(&bnd_mutex);
        
        int item = buffer[out];
        printf("[Bounded]   Consumer consumed: %d from index %d\n", item, out);
        out = (out + 1) % BUFFER_SIZE;
        
        pthread_mutex_unlock(&bnd_mutex);
        sem_post(&empty_slots);
        
        sleep(1);
    }
    pthread_exit(NULL);
}


/* ========================================================================= *
 * MAIN PROGRAM *
 * ========================================================================= */

int main() {
    pthread_t prod_thread, cons_thread;

    printf("--- STARTING UNBOUNDED BUFFER SIMULATION ---\n");
    pthread_create(&prod_thread, NULL, unbounded_producer, NULL);
    pthread_create(&cons_thread, NULL, unbounded_consumer, NULL);
    
    pthread_join(prod_thread, NULL);
    pthread_join(cons_thread, NULL);
    
    pthread_mutex_destroy(&unb_mutex);
    pthread_cond_destroy(&unb_cond);
    printf("--- UNBOUNDED BUFFER SIMULATION COMPLETE ---\n\n");

    sleep(1);

    printf("--- STARTING BOUNDED BUFFER SIMULATION ---\n");
    sem_init(&empty_slots, 0, BUFFER_SIZE);
    sem_init(&full_slots, 0, 0);
    pthread_mutex_init(&bnd_mutex, NULL);
    
    pthread_create(&prod_thread, NULL, bounded_producer, NULL);
    pthread_create(&cons_thread, NULL, bounded_consumer, NULL);
    
    pthread_join(prod_thread, NULL);
    pthread_join(cons_thread, NULL);
    
    sem_destroy(&empty_slots);
    sem_destroy(&full_slots);
    pthread_mutex_destroy(&bnd_mutex);
    printf("--- BOUNDED BUFFER SIMULATION COMPLETE ---\n");

    return 0;
}