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

void signal_handler(int signum)
{
    std::cout << "Parent: Received signal from child. Continuing execution..." << std::endl;
}

void S1_smoothen(struct image_t *input_image, int buf_size, int pipefd[2])
{
    for (int ir = 0; ir < 1000; ir++)
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
            int* batch= new int[buf_size];
            int r1 = 0;
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
                    batch[r1] = smoothened_image->image_pixels[i][j][k];
                    sum += smoothened_image->image_pixels[i][j][k];

                    r1++;
                }
            }
            batch[buf_size - 1] = sum;

            // Complete write operation
            ssize_t total_written = 0;
            ssize_t bytes_to_write = buf_size * sizeof(int);
            char *write_ptr = (char *)batch;
            while (total_written < bytes_to_write)
            {
                ssize_t bytes_written = write(pipefd[1], write_ptr + total_written, bytes_to_write - total_written);
                if (bytes_written <= 0)
                {
                    perror("write");
                    exit(EXIT_FAILURE);
                }
                total_written += bytes_written;
            }
            delete[]  batch;
        }
        free_image(smoothened_image);
    }
}

void S2_find_details(struct image_t *input_image, int buf_size, int pipefd[2], int pipefd2[2], pid_t cpid)
{
    for (int ir = 0; ir < 1000; ir++)
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

        for (int i = 0; i < input_image->height; i++)
        {
            int* buf = new int[buf_size];
            int sum = 0;
            int sum2 = 0;

            // Complete read operation
            ssize_t total_read = 0;
            ssize_t bytes_to_read = buf_size * sizeof(int);
            char *read_ptr = (char *)buf;
            while (total_read < bytes_to_read)
            {
                ssize_t bytes_read = read(pipefd[0], read_ptr + total_read, bytes_to_read - total_read);
                if (bytes_read <= 0)
                {
                    perror("read");
                    exit(EXIT_FAILURE);
                }
                total_read += bytes_read;
            }

            
            for (int j = 0; j < input_image->width; j++)
            {
                for (int k = 0; k < 3; k++)
                {
                    sum += buf[j * 3 + k];
                    detail_image->image_pixels[i][j][k] = input_image->image_pixels[i][j][k] - buf[j * 3 + k];
                    sum2 += detail_image->image_pixels[i][j][k];
                }
            }

            int* batch = new int[buf_size];
            for (int j = 0; j < input_image->width; j++)
            {
                for (int k = 0; k < 3; k++)
                {
                    batch[j * 3 + k] = detail_image->image_pixels[i][j][k];
                }
            }
            batch[buf_size - 1] = sum2;

            // Complete write operation
            ssize_t total_written = 0;
            ssize_t bytes_to_write = buf_size * sizeof(int);
            char *write_ptr = (char *)batch;
            while (total_written < bytes_to_write)
            {
                ssize_t bytes_written = write(pipefd2[1], write_ptr + total_written, bytes_to_write - total_written);
                if (bytes_written <= 0)
                {
                    perror("write");
                    exit(EXIT_FAILURE);
                }
                total_written += bytes_written;
            }

            if (buf[buf_size - 1] != sum)
            {
                cout << "Corruption detected 1. Expected: " << (sum) << ", Actual: " << buf[buf_size - 1] << endl;
                kill(getppid(), SIGKILL);
                kill(cpid, SIGKILL);
                exit(EXIT_FAILURE);
            }
            delete[] buf;
            delete[] batch;
        }
        free_image(detail_image);
    }
}

struct image_t *S3_sharpen(struct image_t *input_image, int buf_size, int pipefd2[2])
{
    struct image_t *sharp_image;
    for (int ir = 0; ir < 1000; ir++)
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
            int* buf = new int[buf_size];
            int sum = 0;

            // Complete read operation
            ssize_t total_read = 0;
            ssize_t bytes_to_read = buf_size * sizeof(int);
            char *read_ptr = (char *)buf;
            while (total_read < bytes_to_read)
            {
                ssize_t bytes_read = read(pipefd2[0], read_ptr + total_read, bytes_to_read - total_read);
                if (bytes_read <= 0)
                {
                    perror("read");
                    exit(EXIT_FAILURE);
                }
                total_read += bytes_read;
            }

            
            for (int j = 0; j < input_image->width; j++)
            {
                for (int k = 0; k < 3; k++)
                {
                    sharp_image->image_pixels[i][j][k] = input_image->image_pixels[i][j][k] + buf[j * 3 + k];
                    sum += buf[j * 3 + k];
                }
            }
            if (buf[buf_size - 1] != sum)
            {
                cout << "Corruption detected 2. Expected: " << (sum) << ", Actual: " << buf[buf_size - 1] << endl;
                exit(EXIT_FAILURE);
            }
            delete[] buf;
        }
        if(ir<999)
        free_image(sharp_image);
    }
    return sharp_image;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        cout << "usage: ./a.out <path-to-original-image> <path-to-transformed-image>\n\n";
        exit(EXIT_FAILURE);
    }

    signal(SIGUSR1, signal_handler);
    const auto start1{std::chrono::steady_clock::now()};
    struct image_t *input_image = read_ppm_file(argv[1]);
    const auto end1{std::chrono::steady_clock::now()};
    int buf_size = (input_image->width * 3) + 1;

    const std::chrono::duration<double> elapsed_seconds1{end1 - start1};
    std::cout << "Read image: " << elapsed_seconds1.count() << " seconds\n";
    int pipefd[2];
    int pipefd2[2];
    int sync_pip1[2];
    int sync_pip2[2];
    pid_t pid = getpid();
    pid_t cpid;
    pid_t gcpid;

    if (pipe(pipefd) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    if (pipe(pipefd2) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    if (pipe(sync_pip1) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    if (pipe(sync_pip2) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    cpid = fork();
    if (cpid == -1)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (cpid == 0)
    { /* Child process */
        gcpid = fork();
        if (gcpid == -1)
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (gcpid == 0)
        { /* Grandchild process */
            close(pipefd[0]);
            close(pipefd[1]);
            close(pipefd2[1]);
            close(sync_pip1[0]);
            close(sync_pip2[0]);

            struct image_t *sharp_image;
            const auto start4{std::chrono::steady_clock::now()};
            sharp_image = S3_sharpen(input_image, buf_size, pipefd2);
            const auto end4{std::chrono::steady_clock::now()};
            const std::chrono::duration<double> elapsed4{end4 - start4};
            std::cout << "Sharpen image: " << elapsed4.count() << " seconds\n";

            const auto start5{std::chrono::steady_clock::now()};
            write_ppm_file(argv[2], sharp_image);
            const auto end5{std::chrono::steady_clock::now()};
            const std::chrono::duration<double> elapsed_seconds5{end5 - start5};
            std::cout << "Write image: " << elapsed_seconds5.count() << " seconds\n";
            close(pipefd2[0]);
            close(sync_pip1[1]);
            close(sync_pip2[1]);
            free_image(sharp_image);
            exit(EXIT_SUCCESS);
        }
        else
        {
            close(pipefd[1]);
            close(pipefd2[0]);
            close(sync_pip1[1]);
            close(sync_pip1[0]);
            close(sync_pip2[1]);
            const auto start3{std::chrono::steady_clock::now()};
            S2_find_details(input_image, buf_size, pipefd, pipefd2, cpid);
            const auto end3{std::chrono::steady_clock::now()};
            const std::chrono::duration<double> elapsed3{end3 - start3};
            std::cout << "Find details: " << elapsed3.count() << " seconds\n";
            close(pipefd[0]);
            close(pipefd2[1]);
            close(sync_pip2[0]);
            wait(NULL);
            exit(EXIT_SUCCESS);
        }
    }
    else
    { /* Parent process */

        close(sync_pip2[1]);
        close(sync_pip2[0]);
        close(sync_pip1[1]);
        close(pipefd[0]);
        close(pipefd2[0]);
        close(pipefd2[1]);

        const auto start2{std::chrono::steady_clock::now()};
        S1_smoothen(input_image, buf_size, pipefd);
        const auto end2{std::chrono::steady_clock::now()};
        const std::chrono::duration<double> elapsed2{end2 - start2};
        std::cout << "Smoothen image: " << elapsed2.count() << " seconds\n";
        close(pipefd[1]);
        close(sync_pip1[0]); /* Reader will see EOF */
        wait(NULL); /* Wait for child */
    }
    free_image(input_image);
}