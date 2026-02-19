#include "config.h"
#include "gif_parse.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace std;

static void print_help(const char *prog_name) {
    fprintf(stderr, "Usage: %s [options] <filename>\n", prog_name);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -v, --verbose    Enable verbose output\n");
    fprintf(stderr, "  -B               Dump block data\n");
    fprintf(stderr, "  -h, --help       Show this help message\n");
}

int main(int argc, char **argv) {
    Config config;
    const char *filename = nullptr;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            config.verbose = true;
        } else if (strcmp(argv[i], "-B") == 0) {
            config.dump_blocks = true;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        } else if (argv[i][0] != '-') {
            filename = argv[i];
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return 1;
        }
    }

    if (!filename) {
        print_help(argv[0]);
        return 1;
    }

    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Error: Could not open file %s\n", filename);
        return 1;
    }
    parse_data_stream(file, config);
    fclose(file);
    return 0;
}