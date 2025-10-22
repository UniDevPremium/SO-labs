#include "common.h"

#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <pthread.h>


// data array
int * data;
int shm_fd;
sem_t *sem_req, *sem_wrk;
/** COMPLETE THE FOLLOWING CODE BLOCK
*
* Add any needed resource 
**/

int request() {
  /** COMPLETE THE FOLLOWING CODE BLOCK
  *
  * map the shared memory in the data array
  **/

  data = (int*)mmap(
    0,
    SIZE,
    PROT_WRITE,
    MAP_SHARED,
    shm_fd,
    0
  );
  if(data == MAP_FAILED)
    handle_error("[@request] unable top map shm");
  // data = (int*)tmp;
  if(close(shm_fd))
    handle_error("[@request] unable to close shm_fd");


  printf("request: mapped address: %p\n", data);

  int i;
  for (i = 0; i < NUM; ++i) {
    data[i] = i;
  }
  printf("request: data generated\n");

  /** COMPLETE THE FOLLOWING CODE BLOCK
  *
  * Signal the worker that it can start the elaboration
  * and wait it has terminated
  **/
  printf("request: acquire updated data\n");

  if(sem_post(sem_wrk))
    handle_error("[@request] unable to post sem_wrk");
  if(sem_wait(sem_req))
    handle_error("[@request] unable to wait sem_req");

  printf("request: updated data:\n");
  for (i = 0; i < NUM; ++i) {
    printf("%d\n", data[i]);
  }

  /** COMPLETE THE FOLLOWING CODE BLOCK
  *
  * Release resources
  **/
  if(munmap((void*)data, SIZE))
    handle_error("[@request] unable to unmap shm");
  if(sem_close(sem_req))
    handle_error("[@request] unable to close sem_req");
  if(sem_close(sem_wrk))
    handle_error("[@request] unable to close sem_wrk");

  return EXIT_SUCCESS;
}

int work() {

  /** COMPLETE THE FOLLOWING CODE BLOCK
  *
  * map the shared memory in the data array
  **/
  data = (int*)mmap(
    0,
    SIZE,
    PROT_WRITE | PROT_READ,
    MAP_SHARED,
    shm_fd,
    0
  );
  if(data == MAP_FAILED)
    handle_error("[@work] unable top map shm");
  if(close(shm_fd))
    handle_error("[@work] unable to close shm_fd");

  printf("worker: mapped address: %p\n", data);

  /** COMPLETE THE FOLLOWING CODE BLOCK
  *
  * Wait that the request() process generated data
  **/

  printf("worker: waiting initial data\n");

  if(sem_wait(sem_wrk))
    handle_error("[@work] unable to wait sem_wrk");

  printf("worker: initial data acquired\n");

  printf("worker: update data\n");
  int i;
  for (i = 0; i < NUM; ++i) {
    data[i] = data[i] * data[i];
  }

  printf("worker: release updated data\n");

  /** COMPLETE THE FOLLOWING CODE BLOCK
  *
  * Signal the requester that elaboration terminated
  */

  if(sem_post(sem_req))
    handle_error("[@work] unable to post sem_req");


  /** COMPLETE THE FOLLOWING CODE BLOCK
  *
  * Release resources
  **/

  if(munmap((void*)data, SIZE))
    handle_error("[@work] unable to unmap shm");
  if(sem_close(sem_req))
    handle_error("[@work] unable to close sem_req");
  if(sem_close(sem_wrk))
    handle_error("[@work] unable to close sem_wrk");

  return EXIT_SUCCESS;
}



int main(int argc, char **argv){
  /** COMPLETE THE FOLLOWING CODE BLOCK
  *
  * Create and open the needed resources 
  **/

  shm_unlink(SHM_NAME);
  sem_unlink(SEM_NAME_REQ);
  sem_unlink(SEM_NAME_WRK);

  // if(shm_unlink(SHM_NAME))
  //   handle_error("[@main] unable to unlink shm");
  // if(sem_unlink(SEM_NAME_REQ))
  //   handle_error("[@main] unable to unlink sem_req");
  // if(sem_unlink(SEM_NAME_WRK))
  //   handle_error("[@main] unable to unlink sem_wrk");


  shm_fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0600);
  if(shm_fd == -1)
    handle_error("[@main] unable to create and open shm");
  if(ftruncate(shm_fd, SIZE) == -1)
    handle_error("[@main] unable to truncate shm");
  
  sem_req = sem_open(SEM_NAME_REQ, O_CREAT | O_EXCL, 0600, 0);
  if(sem_req == SEM_FAILED)
    handle_error("[@main] unable to open sem_req");
  sem_wrk = sem_open(SEM_NAME_WRK, O_CREAT | O_EXCL, 0600, 0);
  if(sem_wrk == SEM_FAILED)
    handle_error("[@main] unable to open sem_wrk");
  

  int ret;
  pid_t pid = fork();
  if (pid == -1) {
      handle_error("main: fork");
  } else if (pid == 0) {
      work();
      _exit(EXIT_SUCCESS);
  }

  request();
  int status;
  ret = wait(&status);
  if (ret == -1)
    handle_error("main: wait");
  if (WEXITSTATUS(status)) handle_error_en(WEXITSTATUS(status), "request() crashed");


  /** COMPLETE THE FOLLOWING CODE BLOCK
  *
  * Close and release resources
  **/
  // if(sem_close(sem_req) == -1)
  //   handle_error("[@main] unable to close sem_req");
  // if(sem_close(sem_wrk))
  //   handle_error("[@main] unable to close sem_wrk");
  if(sem_unlink(SEM_NAME_REQ))
    handle_error("[@main] unable to unlink sem_req");
  if(sem_unlink(SEM_NAME_WRK))
    handle_error("[@main] unable to unlink sem_wrk");
  // if(close(shm_fd))
  //   handle_error("[@main] unable to close shm");
  // if(munmap((void*)data, SIZE))
  //   handle_error("[@main] unable to unmap shm");
  if(shm_unlink(SHM_NAME))
    handle_error("[@main] unable to unlink shm");


  _exit(EXIT_SUCCESS);

}
