/* reader */
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fstream>
#include <chrono>
#include <signal.h>
#include <iostream>
#include <chrono>
#include "libppm.h"
#include <cstdint>

using namespace std;



void free_image(struct image_t *image)
{
    if (image != nullptr)
    {
        if (image->image_pixels != nullptr)
        {
            for (int i = 0; i < image->height; i++)
            {
                if (image->image_pixels[i] != nullptr)
                {
                    for (int j = 0; j < image->width; j++)
                    {
                        delete[] image->image_pixels[i][j];
                    }
                    delete[] image->image_pixels[i];
                }
            }
            delete[] image->image_pixels;
        }
        delete image;
    }
}





int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        cout << "usage: ./reader1 <path-to-original-image> <path-to-transformed-image>\n\n";
        exit(EXIT_FAILURE);
    }

  
    struct image_t *input_image = read_ppm_file(argv[1]);
  
    int buf_size = (input_image->width * 3) + 1;

    
   
    const char *shm_name1 = "/smoothened_data";
    int shm_fd1 = shm_open(shm_name1, O_CREAT | O_RDWR, 0666);

    
    const char *shm_name2 = "/detail_data";
    int shm_fd2 = shm_open(shm_name2, O_CREAT | O_RDWR, 0666);

    if (shm_fd1 == -1 || shm_fd2 == -1)
    {
        perror("shm_open");
        return 1;
    }

    size_t size = buf_size * sizeof(int);
    if (ftruncate(shm_fd1, size) == -1 || ftruncate(shm_fd2, size) == -1)
    {
        perror("ftruncate");
        return 1;
    }

    void *shared_memory1 = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd1, 0);
    void *shared_memory2 = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd2, 0);

    if (shared_memory1 == MAP_FAILED || shared_memory2 == MAP_FAILED)
    {
        perror("mmap");
        return 1;
    }

    
    sem_t *empty1 = sem_open("/sem_empty1", O_CREAT, 0666, 1);
    sem_t *full1 = sem_open("/sem_full1", O_CREAT, 0666, 0);
    sem_t *lock1 = sem_open("/sem_lock1", O_CREAT, 0666, 1);

    
    sem_t *empty2 = sem_open("/sem_empty2", O_CREAT, 0666, 1);
    sem_t *full2 = sem_open("/sem_full2", O_CREAT, 0666, 0);
    sem_t *lock2 = sem_open("/sem_lock2", O_CREAT, 0666, 1);

    if (empty1 == SEM_FAILED || full1 == SEM_FAILED || lock1 == SEM_FAILED ||
        empty2 == SEM_FAILED || full2 == SEM_FAILED || lock2 == SEM_FAILED)
    {
        perror("sem_open");
        return 1;
    }
    const auto start3{std::chrono::steady_clock::now()};
    for (int iter = 0; iter < 1000; iter++)
    {
        struct image_t *detail_image = new struct image_t;
        detail_image->height = input_image->height;
        detail_image->width = input_image->width;
        detail_image->image_pixels = new uint8_t **[detail_image->height];
        for (int i = 0; i < detail_image->height; i++)
        {
            detail_image->image_pixels[i] = new uint8_t *[detail_image->width];
            for (int j = 0; j < detail_image->width; j++)
            {
                detail_image->image_pixels[i][j] = new uint8_t[3];
            }
        }

        
        for (int i = 0; i < detail_image->height; i++)
        {
            int smooth_buf[buf_size];
            int detail_buf[buf_size];
            int sum=0;
            int sum2=0;

          
            sem_wait(full1);
            sem_wait(lock1);

            memcpy(smooth_buf, shared_memory1, buf_size * sizeof(int));

            sem_post(lock1);
            sem_post(empty1);
      
            for (int j = 0; j < detail_image->width; j++)
            {
                for (int k = 0; k < 3; k++)
                {
                    detail_image->image_pixels[i][j][k] = input_image->image_pixels[i][j][k] - smooth_buf[j * 3 + k];
                    sum+=smooth_buf[j*3+k];
                    detail_buf[j * 3 + k] = detail_image->image_pixels[i][j][k];
                    sum2+=detail_image->image_pixels[i][j][k];
                }
            }
            detail_buf[buf_size-1]=sum2;
            if(sum!=smooth_buf[buf_size-1])
            {
                cout<<"The smoothen data is corrupted"<<endl;
                exit(0);
            }
          
            sem_wait(empty2);
            sem_wait(lock2);

            memcpy(shared_memory2, detail_buf, buf_size * sizeof(int));

            sem_post(lock2);
            sem_post(full2);
            
        }

        
        free_image(detail_image);
    }
    const auto end3{std::chrono::steady_clock::now()};
    const std::chrono::duration<double> elapsed3{end3 - start3};
    usleep(5);
    std::cout << "Find details: " << elapsed3.count() << " seconds\n";
    free_image(input_image);

    sem_close(empty1);
    sem_close(full1);
    sem_close(lock1);
    sem_close(empty2);
    sem_close(full2);
    sem_close(lock2);

    munmap(shared_memory1, size);
    munmap(shared_memory2, size);
    close(shm_fd1);
    close(shm_fd2);

    return 0;
}