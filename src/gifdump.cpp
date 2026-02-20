#include "config.h"
#include "gif_parse.h"
#include "gif_reverse.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <getopt.h>

struct Options {
    const char *filename = nullptr;
    const char *output_name = nullptr;
    bool reverse_mode = false;
};

static void print_help(const char *prog_name) {
    fprintf(stderr, "Usage: %s [options] <filename>\n", prog_name);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -v, --verbose    Enable verbose output\n");
    fprintf(stderr, "  -B               Dump block data\n");
    fprintf(stderr, "  -r               Reverse the GIF\n");
    fprintf(stderr, "  -o <filename>    Output file\n");
    fprintf(stderr, "  -h, --help       Show this help message\n");
}

static bool parse_args(int argc, char **argv, Config &config, Options &opts) {
    static struct option long_options[] = {{"verbose", no_argument, nullptr, 'v'},
                                           {"help", no_argument, nullptr, 'h'},
                                           {nullptr, 0, nullptr, 0}};

    int opt;
    while ((opt = getopt_long(argc, argv, "vBo:rh", long_options, nullptr)) != -1) {
        switch (opt) {
            case 'v':
                config.verbose = true;
                break;
            case 'B':
                config.dump_blocks = true;
                break;
            case 'o':
                opts.output_name = optarg;
                break;
            case 'r':
                opts.reverse_mode = true;
                break;
            case 'h':
                print_help(argv[0]);
                return false;
            default:
                print_help(argv[0]);
                return false;
        }
    }

    if (optind >= argc) {
        print_help(argv[0]);
        return false;
    }

    opts.filename = argv[optind];
    return true;
}

int main(int argc, char **argv) {
    Config config;
    Options opts;

    if (!parse_args(argc, argv, config, opts)) {
        return 1;
    }

    FILE *file = fopen(opts.filename, "rb");
    if (!file) {
        fprintf(stderr, "Error: Could not open file %s\n", opts.filename);
        return 1;
    }

    FILE *output_file = stdout;

    if (opts.output_name) {
        output_file = fopen(opts.output_name, "wb"); // 必须用 wb
        if (!output_file) {
            fprintf(stderr, "Error: Could not open output file %s\n", opts.output_name);
            fclose(file);
            return 1;
        }
    }

    if (opts.reverse_mode) {
        reverse(file, output_file, config);
    } else {
        dump_gif(file, output_file, config);
    }

    fclose(file);

    if (output_file != stdout) {
        fclose(output_file);
    }

    return 0;
}