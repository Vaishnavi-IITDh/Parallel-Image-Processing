#include <iostream>
#include <chrono>
#include "libppm.h"
#include <cstdint>

using namespace std;

struct image_t* S1_smoothen(struct image_t *input_image)
{
    // TODO
    struct image_t* smoothened_image = new struct image_t;
    smoothened_image->height = input_image->height;
    smoothened_image->width = input_image->width;
    smoothened_image->image_pixels = new uint8_t**[smoothened_image->height];
    for(int i = 0; i < smoothened_image->height; i++)
    {
        smoothened_image->image_pixels[i] = new uint8_t*[smoothened_image->width];
        for(int j = 0; j < smoothened_image->width; j++)
        {
            smoothened_image->image_pixels[i][j] = new uint8_t[3];
        }
    }
    for(int i = 0; i < smoothened_image->height; i++)
    {
        for(int j = 0; j < smoothened_image->width; j++)
        {
            for(int k = 0; k < 3; k++)
            {
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
                smoothened_image->image_pixels[i][j][k] = value / 9;
            }
        }
    }
    return smoothened_image;
}

struct image_t* S2_find_details(struct image_t *input_image, struct image_t *smoothened_image)
{
    // TODO
    struct image_t* detail_image = new struct image_t;
    detail_image->height = input_image->height;
    detail_image->width = input_image->width;
    detail_image->image_pixels = new uint8_t**[detail_image->height];
    for(int i = 0; i < detail_image->height; i++)
    {
        detail_image->image_pixels[i] = new uint8_t*[detail_image->width];
        for(int j = 0; j < detail_image->width; j++)
        {
            detail_image->image_pixels[i][j] = new uint8_t[3];
        }
    }
    for(int i = 0; i < detail_image->height; i++)
    {
        for(int j = 0; j < detail_image->width; j++)
        {
            for(int k = 0; k < 3; k++)
            {
                detail_image->image_pixels[i][j][k] = input_image->image_pixels[i][j][k] - smoothened_image->image_pixels[i][j][k];
            }
        }
    }
    return detail_image;
}

struct image_t* S3_sharpen(struct image_t *input_image, struct image_t *details_image)
{
    // TODO
    struct image_t* sharp_image = new struct image_t;
    sharp_image->height = input_image->height;
    sharp_image->width = input_image->width;
    sharp_image->image_pixels = new uint8_t**[sharp_image->height];
    for(int i = 0; i < sharp_image->height; i++)
    {
        sharp_image->image_pixels[i] = new uint8_t*[sharp_image->width];
        for(int j = 0; j < sharp_image->width; j++)
        {
            sharp_image->image_pixels[i][j] = new uint8_t[3];
        }
    }
    for(int i = 0; i < input_image->height; i++)
    {
        for(int j = 0; j < input_image->width; j++)
        {
            for(int k = 0; k < 3; k++)
            {
                sharp_image->image_pixels[i][j][k] = input_image->image_pixels[i][j][k] + details_image->image_pixels[i][j][k];
            }
        }
    }
    return sharp_image;
}
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
int main(int argc, char **argv)
{
    if(argc != 3)
    {
        cout << "usage: ./a.out <path-to-original-image> <path-to-transformed-image>\n\n";
        exit(0);
    }

    const auto start1{std::chrono::steady_clock::now()};
    struct image_t *input_image = read_ppm_file(argv[1]);
    const auto end1{std::chrono::steady_clock::now()};
    const std::chrono::duration<double> elapsed_seconds1{end1 - start1};
    

    const auto start2{std::chrono::steady_clock::now()};
    struct image_t *smoothened_image = S1_smoothen(input_image);
    const auto end2{std::chrono::steady_clock::now()};
    const std::chrono::duration<double> elapsed_seconds2{end2 - start2};

    const auto start3{std::chrono::steady_clock::now()};
    struct image_t *details_image = S2_find_details(input_image, smoothened_image);
    const auto end3{std::chrono::steady_clock::now()};
    const std::chrono::duration<double> elapsed_seconds3{end3 - start3};

    const auto start4{std::chrono::steady_clock::now()};
    struct image_t *sharpened_image = S3_sharpen(input_image, details_image);
    const auto end4{std::chrono::steady_clock::now()};
    const std::chrono::duration<double> elapsed_seconds4{end4 - start4};

     auto elapsed2 = elapsed_seconds2;
    auto elapsed3 = elapsed_seconds3;
    auto elapsed4 = elapsed_seconds4;

    for(int i=0;i<999;i++)
    {
    const auto start2{std::chrono::steady_clock::now()};
    struct image_t *smoothened_image = S1_smoothen(input_image);
    const auto end2{std::chrono::steady_clock::now()};
    const std::chrono::duration<double> elapsed_seconds2{end2 - start2};
    elapsed2 +=elapsed_seconds2;
    const auto start3{std::chrono::steady_clock::now()};
    struct image_t *details_image = S2_find_details(input_image, smoothened_image);
    const auto end3{std::chrono::steady_clock::now()};
    const std::chrono::duration<double> elapsed_seconds3{end3 - start3};
    elapsed3 +=elapsed_seconds3;
    const auto start4{std::chrono::steady_clock::now()};
    struct image_t *sharpened_image = S3_sharpen(input_image, details_image);
    const auto end4{std::chrono::steady_clock::now()};
    const std::chrono::duration<double> elapsed_seconds4{end4 - start4};
    elapsed4 +=elapsed_seconds4;
    free_image(smoothened_image);
    free_image(details_image);
    free_image(sharpened_image);
    
    }

    const auto start5{std::chrono::steady_clock::now()};
    write_ppm_file(argv[2], sharpened_image);
    const auto end5{std::chrono::steady_clock::now()};
    const std::chrono::duration<double> elapsed_seconds5{end5 - start5};

    std::cout << "Read image: " << elapsed_seconds1.count() << " seconds\n";
    std::cout << "Smoothen image: " << elapsed2.count() << " seconds\n";
    std::cout << "Find details: " << elapsed3.count() << " seconds\n";
    std::cout << "Sharpen image: " << elapsed4.count() << " seconds\n";
    std::cout << "Write image: " << elapsed_seconds5.count() << " seconds\n";

   

    return 0;
}
