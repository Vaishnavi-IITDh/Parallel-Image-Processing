#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <iostream>
using namespace std;
int main()
{
    sem_unlink("/sem_empty1");
     sem_unlink("/sem_full1");
      sem_unlink("/sem_lock1");

      sem_unlink("/sem_empty2");
     sem_unlink("/sem_full2");
      sem_unlink("/sem_lock2");


    shm_unlink("/smoothened_data");
    shm_unlink("/detail_data");
   // cout << "cleanup done"<<endl;
    return 0 ;   
}