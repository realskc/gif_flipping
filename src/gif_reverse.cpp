#include "gif_reverse.h"
#include "config.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>

void copy_header(FILE *input, FILE *output, const Config &config) {
    if (config.verbose) {
        fprintf(stderr, "Copying GIF header...\n");
    }
    char header[6];
    fread(header, 1, 6, input);
    fwrite(header, 1, 6, output);
    if (config.verbose) {
        fprintf(stderr, "  Header copied: %.6s\n", header);
    }
}

void copy_LSD(FILE *input, FILE *output, const Config &config) {
    if (config.verbose) {
        fprintf(stderr, "Copying Logical Screen Descriptor...\n");
    }
    unsigned char lsd[7];
    fread(lsd, 1, 7, input);
    fwrite(lsd, 1, 7, output);
    if ((lsd[4] >> 7) & 1) {
        if (config.verbose) {
            fprintf(stderr, "  Global Color Table detected, copying...\n");
        }
        int global_color_table_size = 3 * (1 << ((lsd[4] & 0x07) + 1));
        unsigned char *global_color_table = new unsigned char[global_color_table_size];
        fread(global_color_table, 1, global_color_table_size, input);
        fwrite(global_color_table, 1, global_color_table_size, output);
        delete[] global_color_table;
    }
    if (config.verbose) {
        fprintf(stderr, "  Logical Screen Descriptor copied\n");
    }
}

void copy_image_descriptor(FILE *input, FILE *output, const Config &config) {
    if (config.verbose) {
        fprintf(stderr, "Copying Image Descriptor...\n");
    }
    unsigned char image_descriptor[9];
    fread(image_descriptor, 1, 9, input);
    fwrite(image_descriptor, 1, 9, output);
    if ((image_descriptor[8] >> 7) & 1) {
        if (config.verbose) {
            fprintf(stderr, "  Local Color Table detected, copying...\n");
        }
        int local_color_table_size = 3 * (1 << ((image_descriptor[8] & 0x07) + 1));
        unsigned char *local_color_table = new unsigned char[local_color_table_size];
        fread(local_color_table, 1, local_color_table_size, input);
        fwrite(local_color_table, 1, local_color_table_size, output);
        delete[] local_color_table;
    }
    if (config.verbose) {
        fprintf(stderr, "  Image Descriptor copied\n");
    }
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
        if (config.dump_blocks) {
            fprintf(stderr, "  Sub-block of size %d copied\n", sub_block_size);
        }
    }
}

void reverse_image_data(FILE *input, FILE *output, const Config &config) {
    // TODO: Implement LZW decompression and recompression to reverse the image data
    // For now, just copy the image data as is
    int LZW_min_code_size = fgetc(input);
    fputc(LZW_min_code_size, output);
    if (config.dump_blocks) {
        fprintf(stderr, "  LZW Minimum Code Size: %d\n", LZW_min_code_size);
    }
    copy_sub_blocks(input, output, config);
    if (config.verbose) {
        fprintf(stderr, "  Image data copied (not reversed)\n");
    }
}

void copy_extension_block(FILE *input, FILE *output, const Config &config) {
    if (config.verbose) {
        fprintf(stderr, "Copying Extension Block...\n");
    }
    int extension_label = fgetc(input);
    fputc(extension_label, output);
    copy_sub_blocks(input, output, config);
    if (config.verbose) {
        fprintf(stderr, "  Extension Block with label 0x%02X copied\n", extension_label);
    }
}

void reverse(FILE *input, FILE *output, const Config &config) {
    if (config.verbose) {
        fprintf(stderr, "Reversing GIF: %s\n", config.dump_blocks ? "(with block dumping)" : "");
    }
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
                fprintf(stderr, "End of GIF Data Stream\n");
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