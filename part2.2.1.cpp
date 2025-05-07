/* writer */
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
        cout << "usage: ./writer <path-to-original-image> <path-to-transformed-image>\n\n";
        exit(EXIT_FAILURE);
    }

    const auto start1{std::chrono::steady_clock::now()};
    struct image_t *input_image = read_ppm_file(argv[1]);
    const auto end1{std::chrono::steady_clock::now()};
    int buf_size = (input_image->width * 3) + 1;

    const std::chrono::duration<double> elapsed_seconds1{end1 - start1};
    std::cout << "Read image: " << elapsed_seconds1.count() << " seconds\n";

    
    const char *shm_name = "/smoothened_data";
    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("shm_open");
        return 1;
    }

    size_t size = buf_size * sizeof(int);
    if (ftruncate(shm_fd, size) == -1)
    {
        perror("ftruncate");
        return 1;
    }

    void *shared_memory = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_memory == MAP_FAILED)
    {
        perror("mmap");
        return 1;
    }

   
    sem_t *empty1 = sem_open("/sem_empty1", O_CREAT, 0666, 1);
    sem_t *full1 = sem_open("/sem_full1", O_CREAT, 0666, 0);
    sem_t *lock1 = sem_open("/sem_lock1", O_CREAT, 0666, 1);

    if (empty1 == SEM_FAILED || full1 == SEM_FAILED || lock1 == SEM_FAILED)
    {
        perror("sem_open");
        return 1;
    }

    const auto start2{std::chrono::steady_clock::now()};
    for (int iter = 0; iter < 1000; iter++)
    {
        struct image_t *smoothened_image = new struct image_t;
        smoothened_image->height = input_image->height;
        smoothened_image->width = input_image->width;
        smoothened_image->image_pixels = new uint8_t **[smoothened_image->height];
        for (int i = 0; i < smoothened_image->height; i++)
        {
            smoothened_image->image_pixels[i] = new uint8_t *[smoothened_image->width];
            for (int j = 0; j < smoothened_image->width; j++)
            {
                smoothened_image->image_pixels[i][j] = new uint8_t[3];
            }
        }

        
        for (int i = 0; i < smoothened_image->height; i++)
        {
            int batch[buf_size];
            int r = 0;
            int sum = 0;

        
            for (int j = 0; j < smoothened_image->width; j++)
            {
                for (int k = 0; k < 3; k++)
                {
                    int temp = 0;
                    int m, n;
                    int value = 0;
                    for (int r = -1; r < 2; r++)
                    {
                        for (int s = -1; s < 2; s++)
                        {
                            m = i + r;
                            n = j + s;
                            if (m < 0 || n < 0 || m >= smoothened_image->height || n >= smoothened_image->width)
                            {
                                temp = 0;
                            }
                            else
                            {
                                temp = input_image->image_pixels[m][n][k];
                            }
                            value += temp;
                        }
                    }
                    smoothened_image->image_pixels[i][j][k] = value / 9;
                    batch[r] = smoothened_image->image_pixels[i][j][k];
                    sum += smoothened_image->image_pixels[i][j][k];
                    r++;
                }
            }
            batch[buf_size - 1] = sum ;

            
            sem_wait(empty1);
            sem_wait(lock1);

            
            memcpy(shared_memory, batch, buf_size * sizeof(int));
            sem_post(lock1);
            sem_post(full1);
          
        }
        free_image(smoothened_image);
    }
    const auto end2{std::chrono::steady_clock::now()};
    const std::chrono::duration<double> elapsed2{end2 - start2};
    std::cout << "Smoothen image: " << elapsed2.count() << " seconds\n";
    

    free_image(input_image);

    sem_close(empty1);
    sem_close(full1);
    sem_close(lock1);

    munmap(shared_memory, size);
    close(shm_fd);

    return 0;
}