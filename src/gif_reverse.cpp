#include "gif_reverse.h"
#include "config.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>

void copy_header(FILE *input, FILE *output, const Config &config) {
    char header[6];
    fread(header, 1, 6, input);
    fwrite(header, 1, 6, output);
}

void copy_LSD(FILE *input, FILE *output, const Config &config) {
    unsigned char lsd[7];
    fread(lsd, 1, 7, input);
    fwrite(lsd, 1, 7, output);
    if ((lsd[4] >> 7) & 1) {
        int global_color_table_size = 3 * (1 << ((lsd[4] & 0x07) + 1));
        unsigned char *global_color_table = new unsigned char[global_color_table_size];
        fread(global_color_table, 1, global_color_table_size, input);
        fwrite(global_color_table, 1, global_color_table_size, output);
        delete[] global_color_table;
    }
}

void copy_image_descriptor(FILE *input, FILE *output, const Config &config) {
    unsigned char image_descriptor[10];
    fread(image_descriptor, 1, 10, input);
    fwrite(image_descriptor, 1, 10, output);
}

void copy_sub_blocks(FILE *input, FILE *output, const Config &config) {
    while (true) {
        int sub_block_size = fgetc(input);
        fputc(sub_block_size, output);
        if (sub_block_size == 0) break;
        unsigned char *data = new unsigned char[sub_block_size];
        fread(data, 1, sub_block_size, input);
        fwrite(data, 1, sub_block_size, output);
        delete[] data;
    }
}

void reverse_image_data(FILE *input, FILE *output, const Config &config) {
    // TODO: Implement LZW decompression and recompression to reverse the image data
    // For now, just copy the image data as is
    copy_sub_blocks(input, output, config);
}

void copy_extension_block(FILE *input, FILE *output, const Config &config) {
    int extension_label = fgetc(input);
    fputc(extension_label, output);
    copy_sub_blocks(input, output, config);
}

void reverse(FILE *input, FILE *output, const Config &config) {
    copy_header(input, output, config);
    copy_LSD(input, output, config);
    // Data Blocks
    while (true) {
        int block_type = fgetc(input);
        fprintf(output, "%c", block_type);
        switch (block_type) {
            case 0x2C: { // Image Descriptor
                copy_image_descriptor(input, output, config);
                reverse_image_data(input, output, config);
                break;
            }
            case 0x21: { // Extension Blcok
                copy_extension_block(input, output, config);
                break;
            }
            case 0x3B: { // Trailer
                printf("End of GIF Data Stream\n");
                int next_byte = fgetc(input);
                if (next_byte != EOF) {
                    fprintf(stderr, "Error: Data found after GIF trailer\n");
                    exit(3);
                }
                return;
                break;
            }
            case EOF: {
                fprintf(stderr, "Error: Unexpected end of file\n");
                exit(2);
                break;
            }
            default: {
                fprintf(stderr, "Error: Unknown block type 0x%02X\n", block_type);
                exit(1);
                break;
            }
        }
    }
}