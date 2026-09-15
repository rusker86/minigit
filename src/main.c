#include <stdio.h>
#include <string.h>

#include "repository.h"

static void print_usage(const char *program)
{
    printf("Usage:\n");
    printf("  %s init\n", program);
    printf("  %s add <file>\n", program);
    printf("  %s cat <hash>\n", program);
    printf("  %s commit\n", program);
}

static int run_command(int argc, char *argv[])
{
    const char *command = argv[1];

    if (strcmp(command, "init") == 0 && argc == 2) {
        init_repository();
    } else if (strcmp(command, "add") == 0 && argc == 3) {
        add_file(argv[2]);
    } else if (strcmp(command, "cat") == 0 && argc == 3) {
        cat_object(argv[2]);
    } else if (strcmp(command, "commit") == 0 && argc == 2) {
        commit_repository();
    } else {
        printf("Unknown or invalid command: %s\n", command);
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    return run_command(argc, argv);
}
