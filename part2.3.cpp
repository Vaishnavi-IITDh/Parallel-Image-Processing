#include <iostream>
#include <chrono>
#include <fstream>
#include <cstdint>
#include <thread>
#include <atomic>
#include <vector>
#include <iostream>
#include <chrono>
#include "libppm.h"
#include <cstdint>

using namespace std;


atomic<int> current_state{0}; 
atomic<bool> data_lock{false}; 


struct image_t *input_image = nullptr;
struct image_t *smoothened_image = nullptr;
struct image_t *details_image = nullptr;
struct image_t *sharpened_image = nullptr;

vector<int> shared_sum;
vector<int> shared_sum2;



void free_image(struct image_t *image) {
    if (image != nullptr) {
        if (image->image_pixels != nullptr) {
            for (int i = 0; i < image->height; i++) {
                if (image->image_pixels[i] != nullptr) {
                    for (int j = 0; j < image->width; j++) {
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

struct image_t* allocate_image(int width, int height) {
    struct image_t *image = new image_t;
    image->width = width;
    image->height = height;
    image->image_pixels = new uint8_t**[height];
    for (int i = 0; i < height; i++) {
        image->image_pixels[i] = new uint8_t*[width];
        for (int j = 0; j < width; j++) {
            image->image_pixels[i][j] = new uint8_t[3];
        }
    }
    return image;
}




void S1_smoothen(chrono::duration<double>& elapsed_time) {
    auto start = chrono::steady_clock::now();
    for (int i = 0; i < input_image->height; i++) {
        int sum = 0;
       
        while (current_state.load(memory_order_acquire) != 0);

        
        while (data_lock.exchange(true, memory_order_acquire));

        for (int j = 0; j < input_image->width; j++) {
            for (int k = 0; k < 3; k++) {
                int temp = 0;
                int m, n;
                int value = 0;
                for(int r = -1; r < 2; r++)
                {
                    for(int s = -1; s < 2; s++)
                    {
                        m = i + r;
                        n = j + s;
                        if(m < 0 || n < 0 || m >= smoothened_image->height || n >= smoothened_image->width)
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
                int smooth_value = value / 9;
                shared_sum[j * 3 + k] = smooth_value;
                sum = sum + smooth_value;
            }
        }
        shared_sum[input_image->width*3] = sum;

      
        data_lock.store(false, memory_order_release);

       
        current_state.store(1, memory_order_release);
    }
    auto end = chrono::steady_clock::now();
    elapsed_time += end - start;
}


void S2_find_details(chrono::duration<double>& elapsed_time) {
    auto start = chrono::steady_clock::now();
    for (int i = 0; i < input_image->height; i++) {
        int sum = 0;
        int sum2=0;
       
        while (current_state.load(memory_order_acquire) != 1);

      
        while (data_lock.exchange(true, memory_order_acquire));

        for (int j = 0; j < input_image->width; j++) {
            for (int k = 0; k < 3; k++) {
                int detail = input_image->image_pixels[i][j][k] - shared_sum[j * 3 + k];
                shared_sum2[j * 3 + k] = detail;
                sum2 +=detail;
                sum += shared_sum[j*3+k];
            }
        }
        shared_sum2[input_image->width*3] = sum2;
      
        data_lock.store(false, memory_order_release);   
        if(sum != shared_sum[input_image->width*3])
        {
            cout << "The data is corrupted in 1st connection"<<endl;
            exit(0);
        }
       
        current_state.store(2, memory_order_release);
    }
    auto end = chrono::steady_clock::now();
    elapsed_time += end - start;
}


void S3_sharpen(chrono::duration<double>& elapsed_time) {
    auto start = chrono::steady_clock::now();
    for (int i = 0; i < input_image->height; i++) {
        int sum = 0;
        
        while (current_state.load(memory_order_acquire) != 2);

        
        while (data_lock.exchange(true, memory_order_acquire));

        for (int j = 0; j < input_image->width; j++) {
            for (int k = 0; k < 3; k++) {
                
                int sharp_value = input_image->image_pixels[i][j][k] + shared_sum2[j * 3 + k];
                sharpened_image->image_pixels[i][j][k] = sharp_value;
                sum+=shared_sum2[j*3+k];
            }
        }
        
        
        data_lock.store(false, memory_order_release);
        if(sum != shared_sum2[input_image->width*3])
        {
             cout << "The data is corrupted in 2nd connection"<<endl;
            exit(0);
        }
        
        current_state.store(0, memory_order_release);
    }
    auto end = chrono::steady_clock::now();
    elapsed_time += end - start;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        cout << "usage: ./a.out <path-to-original-image> <path-to-transformed-image>\n\n";
        exit(0);
    }


    const auto start1{std::chrono::steady_clock::now()};
    input_image = read_ppm_file(argv[1]);
    const auto end1{std::chrono::steady_clock::now()};
    const std::chrono::duration<double> elapsed1{end1 - start1};

    
    smoothened_image = allocate_image(input_image->width, input_image->height);
    details_image = allocate_image(input_image->width, input_image->height);
    sharpened_image = allocate_image(input_image->width, input_image->height);

   
    shared_sum.resize(input_image->width * 3+1);
    shared_sum2.resize(input_image->width * 3+1);

    
    chrono::duration<double> smooth_t(0);
    chrono::duration<double> detail_t(0);
    chrono::duration<double> sharp_t(0);

 
   
    for (int i = 0; i < 1000; i++) {
       
        current_state.store(0, memory_order_release);

       
        thread thread1(S1_smoothen, ref(smooth_t));
        thread thread2(S2_find_details, ref(detail_t));
        thread thread3(S3_sharpen, ref(sharp_t));

       
        thread1.join();
        thread2.join();
        thread3.join();
    }


    const auto start2{std::chrono::steady_clock::now()};
    write_ppm_file(argv[2], sharpened_image);
    const auto end2{std::chrono::steady_clock::now()};
    const std::chrono::duration<double> elapsed2{end2 - start2};

 
    free_image(input_image);
    free_image(smoothened_image);
    free_image(details_image);
    free_image(sharpened_image);

    // Print timing information
    std::cout << "Read image: " << elapsed1.count() << " seconds\n";
    std::cout << "Smoothen image: " << smooth_t.count() << " seconds\n";
    std::cout << "Find details: " << detail_t.count()  << " seconds\n";
    std::cout << "Sharpen image: " << sharp_t.count() << " seconds\n";
    std::cout << "Write image: " << elapsed2.count() << " seconds\n";

    return 0;
}