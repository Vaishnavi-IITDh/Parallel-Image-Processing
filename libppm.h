#ifndef LIBPPM_H
#define LIBPPM_H
#include <cstdint>

struct image_t
{
	int width;
	int height;
	uint8_t*** image_pixels;
};

struct image_t* read_ppm_file(char* path_to_input_file);
void write_ppm_file(char* path_to_output_file, struct image_t* image);

#endif
