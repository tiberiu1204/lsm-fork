#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <file>\n", argv[0]);
        return EXIT_FAILURE;
    }
    const char *filepath = argv[1];
    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    char *mapped = mmap(NULL, 4096, PROT_READ | PROT_EXEC, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        perror("Error mmapping the file");
        close(fd);
        return EXIT_FAILURE;
    }

    close(fd);

    printf("mapped ok\n");

    return EXIT_SUCCESS;
}