/* reader2*/
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
        cout << "usage: ./reader2 <path-to-original-image> <path-to-transformed-image>\n\n";
        exit(EXIT_FAILURE);
    }

   
    struct image_t *input_image = read_ppm_file(argv[1]);
  
    int buf_size = (input_image->width * 3) + 1;

   

   
    const char *shm_name = "/detail_data";
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

   
    sem_t *empty2 = sem_open("/sem_empty2", O_CREAT, 0666, 1);
    sem_t *full2 = sem_open("/sem_full2", O_CREAT, 0666, 0);
    sem_t *lock2 = sem_open("/sem_lock2", O_CREAT, 0666, 1);

    if (empty2 == SEM_FAILED || full2 == SEM_FAILED || lock2 == SEM_FAILED)
    {
        perror("sem_open");
        return 1;
    }
    struct image_t *sharp_image;

    const auto start4{std::chrono::steady_clock::now()};
    for (int iter = 0; iter < 1000; iter++)
    {
        sharp_image = new struct image_t;
        sharp_image->height = input_image->height;
        sharp_image->width = input_image->width;
        sharp_image->image_pixels = new uint8_t **[sharp_image->height];
        for (int i = 0; i < sharp_image->height; i++)
        {
            sharp_image->image_pixels[i] = new uint8_t *[sharp_image->width];
            for (int j = 0; j < sharp_image->width; j++)
            {
                sharp_image->image_pixels[i][j] = new uint8_t[3];
            }
        }
        for (int i = 0; i < input_image->height; i++)
        {
            int sharp_buf[buf_size];
            int sum=0;
            sem_wait(full2);
            sem_wait(lock2);

            memcpy(sharp_buf, shared_memory, buf_size * sizeof(int));

            sem_post(lock2);
            sem_post(empty2);
        
            for (int j = 0; j < input_image->width; j++)
            {
                for (int k = 0; k < 3; k++)
                {
                    sharp_image->image_pixels[i][j][k] = input_image->image_pixels[i][j][k] + sharp_buf[j * 3 + k];
                    sum+=sharp_buf[j*3+k];
                }
            }
            if(sum!=sharp_buf[buf_size-1])
            {
                cout<<"The detail data is corrupted"<<endl;
                exit(0);
            }
        }
        if(iter<999)
        free_image(sharp_image);
    }
    const auto end4{std::chrono::steady_clock::now()};
    const std::chrono::duration<double> elapsed4{end4 - start4};
    usleep(2000);
    std::cout << "Sharpen image: " << elapsed4.count() << " seconds\n";

    const auto start5{std::chrono::steady_clock::now()};
    write_ppm_file(argv[2], sharp_image);
    const auto end5{std::chrono::steady_clock::now()};
    const std::chrono::duration<double> elapsed_seconds5{end5 - start5};
    std::cout << "Write image: " << elapsed_seconds5.count() << " seconds\n";
    
    free_image(sharp_image);
    free_image(input_image);

    sem_close(empty2);
    sem_close(full2);
    sem_close(lock2);

    munmap(shared_memory, size);

    close(shm_fd);

    return 0;
}
