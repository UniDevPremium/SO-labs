#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

#define N 1000 // number of threads
#define M 10000 // number of iterations per thread
#define V 1 // value added to the balance by each thread at each iteration

unsigned long int* shared_variable;
int n = N, m = M, v = V;

void* thread_work(void *arg) {
	int i;
	int* obj_ptr = (int*)(arg);
	for (i = 0; i < m; i++)
		shared_variable[*obj_ptr] += v;
	return NULL;
}

int main(int argc, char **argv)
{
	if (argc > 1) n = atoi(argv[1]);
	if (argc > 2) m = atoi(argv[2]);
	if (argc > 3) v = atoi(argv[3]);
	shared_variable = (unsigned long int*)calloc(n, sizeof(unsigned long int));

	printf("Going to start %d threads, each adding %d times %d to a shared variable initialized to zero...", n, m, v); fflush(stdout);
	pthread_t* threads = (pthread_t*)malloc(n * sizeof(pthread_t)); // also calloc(n,sizeof(pthread_t))
	int i;
	int* objs = (int*)malloc(n * sizeof(int));
	for (i = 0; i < n; i++){
		objs[i] = i;
		if (pthread_create(&threads[i], NULL, thread_work, (void*)&objs[i]) != 0) {
			fprintf(stderr, "Can't create a new thread, error %d\n", errno);
			exit(EXIT_FAILURE);
		}
	}
	printf("ok\n");

	printf("Waiting for the termination of all the %d threads...", n); fflush(stdout);
	for (i = 0; i < n; i++)
		pthread_join(threads[i], NULL);
	printf("ok\n");

	unsigned long int result = 0;
	for(i = 0; i < n; i++) result += shared_variable[i];
	unsigned long int expected_value = (unsigned long int)n*m*v;
	printf("The value of the shared variable is %lu. It should have been %lu\n", result, expected_value);
	if (expected_value > result) {
		unsigned long int lost_adds = (expected_value - result) / v;
		printf("Number of lost adds: %lu\n", lost_adds);
	}

    free(threads);
	// free(shared_variable);
	// free(objs);

	return EXIT_SUCCESS;
}

