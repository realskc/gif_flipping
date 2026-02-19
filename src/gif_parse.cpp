#include "gif_parse.h"
#include "config.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <vector>

enum class DumpMode { None, Hex, Text };

void read_sub_blocks(FILE *file, DumpMode mode) {
    std::vector<unsigned char> data;
    while (true) {
        int sub_block_size = fgetc(file);
        if (sub_block_size == 0) break;
        data.resize(data.size() + sub_block_size);
        fread(data.data() + data.size() - sub_block_size, 1, sub_block_size, file);
    }
    if (mode == DumpMode::Hex) {
        for (auto byte : data) {
            printf("%02X ", byte);
        }
        printf("\n");
    } else if (mode == DumpMode::Text) {
        for (auto byte : data) {
            printf("%c", byte);
        }
        printf("\n");
    }
}

void dump_header(FILE *file, const Config &config) {
    char header[6];
    fread(header, 1, 6, file);
    printf("[INFO] Header: %.6s\n", header);
}

void dump_LSD(FILE *file, const Config &config) {
    unsigned char lsd[7];
    fread(lsd, 1, 7, file);
    int width = lsd[0] | (lsd[1] << 8);
    int height = lsd[2] | (lsd[3] << 8);
    int global_color_table_flag = (lsd[4] >> 7) & 1;
    int color_resolution = ((lsd[4] >> 4) & 0x07) + 1;
    int sorted_flag = (lsd[4] >> 3) & 1;
    int size_of_global_color_table = 1 << ((lsd[4] & 0x07) + 1);
    int background_color_index = lsd[5];
    if (config.dump_blocks) {
        printf("[INFO] Logical Screen Descriptor:\n");
        printf("  Logical Screen: %d x %d\n", width, height);
        printf("  Has Global Color Table: %s\n", global_color_table_flag ? "Yes" : "No");
        printf("  Color Resolution: %d bits per primary color\n", color_resolution);
        if (global_color_table_flag) {
            printf("  Size of Global Color Table: %d\n", size_of_global_color_table);
            printf("  Is Color Table Sorted: %s\n", sorted_flag ? "Yes" : "No");
            printf("  Background Color Index: %d\n", background_color_index);
        }
    } else {
        printf("[INFO] Logical Screen Descriptor: %d x %d\n", width, height);
    }
    if (global_color_table_flag) {
        fseek(file, size_of_global_color_table * 3, SEEK_CUR);
        printf("[INFO] Global Color Table: %d bytes\n", size_of_global_color_table * 3);
    }
}

void dump_image_descriptor(FILE *file, const Config &config) {
    unsigned char img_desc[9];
    fread(img_desc, 1, 9, file);
    int img_left = img_desc[0] | (img_desc[1] << 8);
    int img_top = img_desc[2] | (img_desc[3] << 8);
    int img_width = img_desc[4] | (img_desc[5] << 8);
    int img_height = img_desc[6] | (img_desc[7] << 8);
    int local_color_table_flag = (img_desc[8] >> 7) & 1;
    int interlace_flag = (img_desc[8] >> 6) & 1;
    (void)interlace_flag; // Suppress unused variable warning
    int sorted_flag = (img_desc[8] >> 5) & 1;
    int size_of_local_color_table = 1 << ((img_desc[8] & 0x07) + 1);
    if (config.dump_blocks) {
        printf("[INFO] Image Descriptor:\n");
        printf(" Position: (%d, %d)\n", img_left, img_top);
        printf(" Size: %dx%d\n", img_width, img_height);
        printf(" Has Local Color Table: %s\n", local_color_table_flag ? "Yes" : "No");
    } else {
        printf("[INFO] Image Descriptor: Position (%d, %d), Size %dx%d\n", img_left, img_top,
               img_width, img_height);
    }
    if (local_color_table_flag) {
        printf(" Size of Local Color Table: %d\n", size_of_local_color_table);
        printf(" Is Color Table Sorted: %s\n", sorted_flag ? "Yes" : "No");
        unsigned char *local_color_table = new unsigned char[size_of_local_color_table * 3];
        fread(local_color_table, 1, size_of_local_color_table * 3, file);
        delete[] local_color_table;
    }
}

void dump_image_data(FILE *file, const Config &config) {
    int lzw_min_code_size = fgetc(file);
    if (config.dump_blocks) {
        printf(" LZW Minimum Code Size: %d\n", lzw_min_code_size);
    }
    read_sub_blocks(file, DumpMode::None);
}

void dump_gif(FILE *file, const Config &config) {
    dump_header(file, config);
    dump_LSD(file, config);
    // Data Blocks
    while (true) {
        int block_type = fgetc(file);
        switch (block_type) {
            case 0x2C: { // Image Descriptor
                dump_image_descriptor(file, config);
                dump_image_data(file, config);
                break;
            }
            case 0x21: { // Extension Blcok
                int label = fgetc(file);
                switch (label) {
                    case 0x01: { // Plain Text Extension
                        fprintf(stderr, "[Warning] Plain Text Extension: Not Implemented\n");
                        // Skip plain text extension data
                        fseek(file, 13, SEEK_CUR); // Skip fixed-size header
                        read_sub_blocks(file, config.dump_blocks ? DumpMode::Text : DumpMode::None);
                        break;
                    }
                    case 0xF9: { // Graphic Control Extension
                        unsigned char gce[6];
                        fread(gce, 1, 6, file);
                        int disposal_method = (gce[1] >> 2) & 0x07;
                        int user_input_flag = (gce[1] >> 1) & 0x01;
                        int transparent_color_flag = gce[1] & 0x01;
                        int delay_time = gce[2] | (gce[3] << 8);
                        int transparent_color_index = gce[4];
                        if (config.dump_blocks) {
                            printf("[INFO] Graphic Control Extension:\n");
                            printf(" Disposal Method: %d\n", disposal_method);
                            printf(" User Input Flag: %s\n", user_input_flag ? "Yes" : "No");
                            printf(" Transparent Color Flag: %s\n",
                                   transparent_color_flag ? "Yes" : "No");
                            if (delay_time > 0) {
                                printf(" Delay Time: %dms\n", delay_time * 10);
                            } else {
                                printf(" Delay Time: No delay\n");
                            }
                            if (transparent_color_flag) {
                                printf(" Transparent Color Index: %d\n", transparent_color_index);
                            }
                        } else {
                            printf("[INFO] Graphic Control Extension: Delay = %dms\n",
                                   delay_time * 10);
                        }
                        break;
                    }
                    case 0xFE: { // Comment Extension
                        printf("[INFO] Comment Extension\n");
                        printf(" Comment Data:\n");
                        read_sub_blocks(file, config.dump_blocks ? DumpMode::Text : DumpMode::None);
                        puts("");
                        break;
                    }
                    case 0xFF: { // Application Extension
                        int app_ext_size = fgetc(file);
                        assert(app_ext_size == 11);
                        char app_identifier[9];
                        char app_auth_code[4];
                        fread(app_identifier, 1, 8, file);
                        app_identifier[8] = '\0';
                        fread(app_auth_code, 1, 3, file);
                        app_auth_code[3] = '\0';
                        if (config.dump_blocks) {
                            printf("[INFO] Application Extension:\n");
                            printf(" Application Identifier: %s\n", app_identifier);
                            printf(" Application Authentication Code: %s\n", app_auth_code);
                            printf(" Application Data:\n");
                            read_sub_blocks(file, DumpMode::Hex);
                            puts("");
                        } else {
                            printf("[INFO] Application Extension: %s\n", app_identifier);
                            read_sub_blocks(file, DumpMode::None);
                        }
                        break;
                    }
                }
                break;
            }
            case 0x3B: { // Trailer
                printf("End of GIF Data Stream\n");
                int next_byte = fgetc(file);
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