#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <semaphore.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

// macros for error handling
#include "common.h"

#define N 10   // child process count
#define M 5    // thread per child process count
#define T 3     // time to sleep for main process

#define FILENAME	"accesses.log"

/*
 * data structure required by threads
 */
typedef struct thread_args_s {
    unsigned int child_id;
    unsigned int thread_id;
} thread_args_t;

/*
 * parameters can be set also via command-line arguments
 */
int n = N, m = M, t = T;

/* TODO: declare as many semaphores as needed to implement
 * the intended semantics, and choose unique identifiers for
 * them (e.g., "/mysem_critical_section") */

sem_t *cs_sem, *ready_sem, *start_sem, *stop_sem; // *end_sem;
const char *CS_SEMNAME = "/cs_sem", *READY_SEMNAME = "/ready_sem", *START_SEMNAME = "/start_sem", *STOP_SEMNAME = "/stop_sem"; //, *END_SEMNAME = "/end_sem";

/* TODO: declare a shared memory and the data type to be placed 
 * in the shared memory, and choose a unique identifier for
 * the memory (e.g., "/myshm") 
 * Declare any global variable (file descriptor, memory 
 * pointers, etc.) needed for its management
 */

// int *data, fd_shm, shm_size = sizeof(int);
// const char *SHM_NAME = "/myshm";

/*
 * Ensures that an empty file with given name exists.
 */
void init_file(const char *filename) {
    printf("[Main] Initializing file %s...", FILENAME);
    fflush(stdout);
    int fd = open(FILENAME, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd<0) handle_error("error while initializing file");
    close(fd);
    printf("closed...file correctly initialized!!!\n");
}

/*
 * Create a named semaphore with a given name, mode and initial value.
 * Also, tries to remove any pre-existing semaphore with the same name.
 */
sem_t *create_named_semaphore(const char *name, mode_t mode, unsigned int value) {
    printf("[Main] Creating named semaphore %s...", name);
    fflush(stdout);
    
    sem_unlink(name);

    sem_t *sem = sem_open(name, O_CREAT | O_EXCL, mode, value);
    if (sem == SEM_FAILED) handle_error("sem_open");

    printf("done!!!\n");
    return sem;
}



void parseOutput() {
    // identify the child that accessed the file most times
    int* access_stats = calloc(n, sizeof(int)); // initialized with zeros
    printf("[Main] Opening file %s in read-only mode...", FILENAME);
	fflush(stdout);
    int fd = open(FILENAME, O_RDONLY);
    if (fd < 0) handle_error("error while opening output file");
    printf("ok, reading it and updating access stats...");
	fflush(stdout);

    size_t read_bytes;
    int index;
    do {
        read_bytes = read(fd, &index, sizeof(int));
        if (read_bytes > 0)
            access_stats[index]++;
    } while(read_bytes > 0);
    printf("ok, closing it...");
	fflush(stdout);

    close(fd);
    printf("closed!!!\n");

    int max_child_id = -1, max_accesses = -1;
    for (int i = 0; i < n; i++) {
        printf("[Main] Child %d accessed file %s %d times\n", i, FILENAME, access_stats[i]);
        if (access_stats[i] > max_accesses) {
            max_accesses = access_stats[i];
            max_child_id = i;
        }
    }
    printf("[Main] ===> The process that accessed the file most often is %d (%d accesses)\n", max_child_id, max_accesses);
    free(access_stats);
}

void* thread_function(void* x) {
    thread_args_t *args = (thread_args_t*)x;

    /* TODO: protect the critical section using semaphore(s) as needed */
    sem_wait(cs_sem);

    // open file, write child identity and close file
    int fd = open(FILENAME, O_WRONLY | O_APPEND);
    if (fd < 0) handle_error("error while opening file");
    printf("[Child#%d-Thread#%d] File %s opened in append mode!!!\n", args->child_id, args->thread_id, FILENAME);	

    write(fd, &(args->child_id), sizeof(int));
    printf("[Child#%d-Thread#%d] %d appended to file %s opened in append mode!!!\n", args->child_id, args->thread_id, args->child_id, FILENAME);	

    close(fd);
    printf("[Child#%d-Thread#%d] File %s closed!!!\n", args->child_id, args->thread_id, FILENAME);

    sem_post(cs_sem);

    free(x);
    pthread_exit(NULL);
}

void mainProcess() {
    /* TODO: the main process waits for all the children to start,
     * it notifies them to start their activities, and sleeps
     * for some time t. Once it wakes up, it notifies the children
     * to end their activities, and waits for their termination.
     * Finally, it calls the parseOutput() method and releases
     * any shared resources. */

    int i;
    // wait for all children to be ready
    for (i = 0; i < n; i++) sem_wait(ready_sem);

    // notify children to start, sleep then notify to stop
    for (i = 0; i < n; i++) sem_post(start_sem);
    sleep(t);
    // *data = 1;
    sem_post(stop_sem);
    // for (i = 0; i < n; i++) sem_post(stop_sem);
    // data[0] = 1;
    // sleep(t);
    // data[1] = 1;

    // wait for children termination
    // for (i = 0; i < n; i++) sem_wait(end_sem);
    int ret, child_status;
    for (i = 0; i < n; i++) {
        ret = wait(&child_status);
        if (ret == -1) handle_error("wait");
        if (WEXITSTATUS(child_status)) {
            fprintf(stderr, "ERROR: child died with code %d\n", WEXITSTATUS(child_status));
            exit(EXIT_FAILURE);
        }
    }

    // call parsing and release
    parseOutput();

    if (sem_close(cs_sem)) handle_error("sem_close");
    if (sem_close(ready_sem)) handle_error("sem_close");
    if (sem_close(start_sem)) handle_error("sem_close");
    if (sem_close(stop_sem)) handle_error("sem_close");
    // if (sem_close(end_sem)) handle_error("sem_close");
    // if (close(fd_shm)) handle_error("close");
    // if (munmap(data, shm_size)) handle_error("munmap");

    if (sem_unlink(CS_SEMNAME)) handle_error("sem_unlink");
    if (sem_unlink(READY_SEMNAME)) handle_error("sem_unlink");
    if (sem_unlink(START_SEMNAME)) handle_error("sem_unlink");
    if (sem_unlink(STOP_SEMNAME)) handle_error("sem_unlink");
    // if (sem_unlink(END_SEMNAME)) handle_error("sem_unlink");
    // if (shm_unlink(SHM_NAME)) handle_error("munmap");
}

void childProcess(int child_id) {
    /* TODO: each child process notifies the main process that it
     * is ready, then waits to be notified from the main in order
     * to start. As long as the main process does not notify a
     * termination event [hint: use sem_getvalue() here], the child
     * process repeatedly creates m threads that execute function
     * thread_function() and waits for their completion. When a
     * notification has arrived, the child process notifies the main
     * process that it is about to terminate, and releases any
     * shared resources before exiting. */

    int i, ret, value;
    pthread_t *threads = malloc(m * sizeof(pthread_t));

    // notify ready and wait
    sem_post(ready_sem);
    sem_wait(start_sem);
    // execution
    do {
        // printf("data control = %d", *data);
        memset(threads, 0, m * sizeof(pthread_t));
        for (i = 0; i < m; i++) {
            thread_args_t *arg = malloc(sizeof(thread_args_t));
            if (arg == NULL) handle_error("malloc");
            arg->child_id = child_id;
            arg->thread_id = i;
            ret = pthread_create(&threads[i], NULL, thread_function, arg);
            if (ret) handle_error_en(ret, "pthread_create");
        }

        for (i = 0; i < m; i++) {
            ret = pthread_join(threads[i], NULL);
            if (ret) handle_error_en(ret, "pthread_join");
        }

        // verify interrupt notification event
        if(sem_getvalue(stop_sem, &value) == -1) handle_error("sem_getvalue");
    } while (value == 0);

    // notify about to interrupt and release
    free(threads);
    // sem_post(end_sem);

    if (sem_close(cs_sem)) handle_error("sem_close");
    if (sem_close(ready_sem)) handle_error("sem_close");
    if (sem_close(start_sem)) handle_error("sem_close");
    if (sem_close(stop_sem)) handle_error("sem_close");
    // if (sem_close(end_sem)) handle_error("sem_close");
    // if (close(fd_shm)) handle_error("close");
    // if (munmap(data, shm_size)) handle_error("munmap");
}

int main(int argc, char **argv) {
    // arguments
    if (argc > 1) n = atoi(argv[1]);
    if (argc > 2) m = atoi(argv[2]);
    if (argc > 3) t = atoi(argv[3]);

    // initialize the file
    init_file(FILENAME);

    /* TODO: initialize any semaphore needed in the implementation, and
     * create N children where the i-th child calls childProcess(i); 
     * initialize the shared memory (create it, set its size and map it in the 
     * memory), then the main process executes function mainProcess() once 
     * all the children have been created */

    int i, ret;
    
    // initialize semaphores to 0
    cs_sem = create_named_semaphore(CS_SEMNAME, 0600, 1);
    ready_sem = create_named_semaphore(READY_SEMNAME, 0600, 0);
    start_sem = create_named_semaphore(START_SEMNAME, 0600, 0);
    stop_sem = create_named_semaphore(STOP_SEMNAME, 0600, 0);
    // end_sem = create_named_semaphore(END_SEMNAME, 0600, 0);

    // fork N children calling childProcess(i)
    for (i = 0; i < n; i++){
        pid_t pid = fork();
        if (pid == -1) handle_error("fork");
        if (pid == 0){
            childProcess(i);
            _exit(EXIT_SUCCESS);
        }
    }

    // initialize the shm, size and map it
    // shm_unlink(SHM_NAME);
    // fd_shm = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0600);
    // if (fd_shm == -1) handle_error("shm_open");
    // if (ftruncate(fd_shm, shm_size) == -1) handle_error("ftruncate");
    // data = mmap(
    //     0,
    //     shm_size,
    //     PROT_READ | PROT_WRITE,
    //     MAP_SHARED,
    //     fd_shm,
    //     0
    // );
    // if (data == MAP_FAILED) handle_error("mmap");
    // memset(data, 0, shm_size);

    // call mainProcess()
    mainProcess();

    exit(EXIT_SUCCESS);
}