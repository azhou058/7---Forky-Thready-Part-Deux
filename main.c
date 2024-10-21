#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

// using mutex to make sure one thread updates the count of created threads at a time
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int threads_created = 0;
int total_threads = 0;

typedef struct {
    int index;
    int total;
} thread_data_t;

void random_sleep() {
    srand(time(NULL) ^ (unsigned long)pthread_self());
    int sleep_time = rand() % 8 + 1;
    printf("and will sleep for %d seconds\n", sleep_time);
    sleep(sleep_time);
}

void* thread_pattern1(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    printf("Parent: created child %d (id %lu)\n", data->index, (unsigned long)pthread_self());

    // Lock mutex to update the count of created threads
    pthread_mutex_lock(&mutex);
    threads_created++;

    // unlock mutex and signal the parent that all threads have been created
    if (threads_created == total_threads) {
        pthread_cond_signal(&cond);
    }
    pthread_mutex_unlock(&mutex);

    printf("Process %d created (id %lu) ", data->index, (unsigned long)pthread_self());
    random_sleep();
    printf("Thread %d (id %lu): exiting\n", data->index, (unsigned long)pthread_self());
    return NULL;
}

void create_threads_pattern1(int n) {
    printf("** Pattern 1: creating %d threads\n", n);
    pthread_t threads[n];
    thread_data_t data[n];
    total_threads = n;

    for (int i = 0; i < n; i++) {
        data[i].index = i;
        pthread_create(&threads[i], NULL, thread_pattern1, &data[i]);
    }

    // Waits until all threads have been created
    pthread_mutex_lock(&mutex);
    while (threads_created < n) {
        pthread_cond_wait(&cond, &mutex);
    }
    pthread_mutex_unlock(&mutex);

    printf("** Pattern 1: All threads created\n");

    for (int i = 0; i < n; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("** Pattern 1: All threads have exited\n");
}

void* thread_pattern2(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;

    printf("Thread %d (id %lu) starting\n", data->index, (unsigned long)pthread_self());

    if (data->index < data->total - 1) {
        pthread_t next_thread;
        thread_data_t next_data;
        next_data.index = data->index + 1;
        next_data.total = data->total;

        pthread_create(&next_thread, NULL, thread_pattern2, &next_data);
        printf("Thread %d (id %lu) creating thread %d (id %lu) ", data->index, (unsigned long)pthread_self(), next_data.index, (unsigned long)next_thread);
        pthread_mutex_unlock(&mutex);
        random_sleep();

        pthread_join(next_thread, NULL); // Wait for the next thread to finish
    } else {
        printf("Thread %d (id %lu) [no more threads created] ", data->index, (unsigned long)pthread_self());
        random_sleep();
    }

    printf("Thread %d (id %lu): exiting\n", data->index, (unsigned long)pthread_self());
    return NULL;
}

void create_threads_pattern2(int n) {
    printf("** Pattern 2: creating %d threads\n", n);

    pthread_t first_thread;
    thread_data_t first_data;
    first_data.index = 0;
    first_data.total = n;

    pthread_create(&first_thread, NULL, thread_pattern2, &first_data);
    printf("Parent: created thread 0\n");

    pthread_join(first_thread, NULL);

    printf("** Pattern 2: All threads have exited\n");
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <number of threads> <pattern>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    int pattern = atoi(argv[2]);

    if (n < 1 || n > 256) {
        printf("Number of threads must be between 1 and 256.\n");
        return 1;
    }

    switch (pattern) {
        case 1:
            create_threads_pattern1(n);
            break;
        case 2:
            create_threads_pattern2(n);
            break;
        default:
            printf("Invalid pattern number. Choose 1 or 2\n");
            return 1;
    }

    return 0;
}
