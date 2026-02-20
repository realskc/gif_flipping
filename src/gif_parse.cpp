#include "gif_parse.h"
#include "config.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <vector>

enum class DumpMode { None, Hex, Text };

void read_sub_blocks(FILE *file, FILE *output_file, DumpMode mode) {
    std::vector<unsigned char> data;
    while (true) {
        int sub_block_size = fgetc(file);
        if (sub_block_size == 0) break;
        data.resize(data.size() + sub_block_size);
        fread(data.data() + data.size() - sub_block_size, 1, sub_block_size, file);
    }
    if (mode == DumpMode::Hex) {
        for (auto byte : data) {
            fprintf(output_file, "%02X ", byte);
        }
        fprintf(output_file, "\n");
    } else if (mode == DumpMode::Text) {
        for (auto byte : data) {
            fprintf(output_file, "%c", byte);
        }
        fprintf(output_file, "\n");
    }
}

void dump_header(FILE *file, FILE *output_file, const Config &config) {
    char header[6];
    fread(header, 1, 6, file);
    fprintf(output_file, "[INFO] Header: %.6s\n", header);
}

void dump_LSD(FILE *file, FILE *output_file, const Config &config) {
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
        fprintf(output_file, "[INFO] Logical Screen Descriptor:\n");
        fprintf(output_file, "  Logical Screen: %d x %d\n", width, height);
        fprintf(output_file, "  Has Global Color Table: %s\n",
                global_color_table_flag ? "Yes" : "No");
        fprintf(output_file, "  Color Resolution: %d bits per primary color\n", color_resolution);
        if (global_color_table_flag) {
            fprintf(output_file, "  Size of Global Color Table: %d\n", size_of_global_color_table);
            fprintf(output_file, "  Is Color Table Sorted: %s\n", sorted_flag ? "Yes" : "No");
            fprintf(output_file, "  Background Color Index: %d\n", background_color_index);
        }
    } else {
        fprintf(output_file, "[INFO] Logical Screen Descriptor: %d x %d\n", width, height);
    }
    if (global_color_table_flag) {
        fseek(file, size_of_global_color_table * 3, SEEK_CUR);
        fprintf(output_file, "[INFO] Global Color Table: %d bytes\n",
                size_of_global_color_table * 3);
    }
}

void dump_image_descriptor(FILE *file, FILE *output_file, const Config &config) {
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
        fprintf(output_file, "[INFO] Image Descriptor:\n");
        fprintf(output_file, " Position: (%d, %d)\n", img_left, img_top);
        fprintf(output_file, " Size: %dx%d\n", img_width, img_height);
        fprintf(output_file, " Has Local Color Table: %s\n", local_color_table_flag ? "Yes" : "No");
    } else {
        fprintf(output_file, "[INFO] Image Descriptor: Position (%d, %d), Size %dx%d\n", img_left,
                img_top, img_width, img_height);
    }
    if (local_color_table_flag) {
        fprintf(output_file, " Size of Local Color Table: %d\n", size_of_local_color_table);
        fprintf(output_file, " Is Color Table Sorted: %s\n", sorted_flag ? "Yes" : "No");
        unsigned char *local_color_table = new unsigned char[size_of_local_color_table * 3];
        fread(local_color_table, 1, size_of_local_color_table * 3, file);
        delete[] local_color_table;
    }
}

void dump_image_data(FILE *file, FILE *output_file, const Config &config) {
    int lzw_min_code_size = fgetc(file);
    if (config.dump_blocks) {
        fprintf(output_file, " LZW Minimum Code Size: %d\n", lzw_min_code_size);
    }
    read_sub_blocks(file, output_file, DumpMode::None);
}

void dump_extension_block(FILE *file, FILE *output_file, const Config &config) {
    int label = fgetc(file);
    switch (label) {
        case 0x01: { // Plain Text Extension
            fprintf(stderr, "[Warning] Plain Text Extension: Not Implemented\n");
            // Skip plain text extension data
            fseek(file, 13, SEEK_CUR); // Skip fixed-size header
            read_sub_blocks(file, output_file,
                            config.dump_blocks ? DumpMode::Text : DumpMode::None);
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
                fprintf(output_file, "[INFO] Graphic Control Extension:\n");
                fprintf(output_file, " Disposal Method: %d\n", disposal_method);
                fprintf(output_file, " User Input Flag: %s\n", user_input_flag ? "Yes" : "No");
                fprintf(output_file, " Transparent Color Flag: %s\n",
                        transparent_color_flag ? "Yes" : "No");
                if (delay_time > 0) {
                    fprintf(output_file, " Delay Time: %dms\n", delay_time * 10);
                } else {
                    fprintf(output_file, " Delay Time: No delay\n");
                }
                if (transparent_color_flag) {
                    fprintf(output_file, " Transparent Color Index: %d\n", transparent_color_index);
                }
            } else {
                fprintf(output_file, "[INFO] Graphic Control Extension: Delay = %dms\n",
                        delay_time * 10);
            }
            break;
        }
        case 0xFE: { // Comment Extension
            fprintf(output_file, "[INFO] Comment Extension\n");
            fprintf(output_file, " Comment Data:\n");
            read_sub_blocks(file, output_file,
                            config.dump_blocks ? DumpMode::Text : DumpMode::None);
            fputs("\n", output_file);
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
                fprintf(output_file, "[INFO] Application Extension:\n");
                fprintf(output_file, " Application Identifier: %s\n", app_identifier);
                fprintf(output_file, " Application Authentication Code: %s\n", app_auth_code);
                fprintf(output_file, " Application Data:\n");
                read_sub_blocks(file, output_file, DumpMode::Hex);
                fputs("\n", output_file);
            } else {
                fprintf(output_file, "[INFO] Application Extension: %s\n", app_identifier);
                read_sub_blocks(file, output_file, DumpMode::None);
            }
            break;
        }
    }
}

void dump_gif(FILE *file, FILE *output_file, const Config &config) {
    dump_header(file, output_file, config);
    dump_LSD(file, output_file, config);
    // Data Blocks
    while (true) {
        int block_type = fgetc(file);
        switch (block_type) {
            case 0x2C: { // Image Descriptor
                dump_image_descriptor(file, output_file, config);
                dump_image_data(file, output_file, config);
                break;
            }
            case 0x21: { // Extension Blcok
                dump_extension_block(file, output_file, config);
                break;
            }
            case 0x3B: { // Trailer
                fprintf(output_file, "End of GIF Data Stream\n");
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