// mmap_file_exec_test.c
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <iterations> <memory_pages_number>\n", argv[0]);
        return EXIT_FAILURE;
    }
    int iterations = atoi(argv[1]);
    int num_pages = atoi(argv[2]);
    if (num_pages <= 0) {
        fprintf(stderr, "Invalid number of pages: %d\n", num_pages);
        return EXIT_FAILURE;
    }
    if (iterations <= 0) {
        fprintf(stderr, "Invalid number of iterations: %d\n", iterations);
        return EXIT_FAILURE;
    }
    size_t page_size = sysconf(_SC_PAGESIZE);
    char template[] = "/tmp/mmaptestXXXXXX";
    int fd = mkstemp(template);

    if (fd < 0) {
        perror("mkstemp");
        exit(EXIT_FAILURE);
    }

    // Fill the file with dummy content
    ftruncate(fd, page_size * num_pages);
    char *dummy = malloc(page_size * num_pages);
    memset(dummy, 0x90, page_size * num_pages); 
    write(fd, dummy, page_size * num_pages);
    free(dummy);

    for (int i = 0; i < iterations; i++) {
        void *addr = mmap(NULL, page_size * num_pages, PROT_READ | PROT_EXEC, MAP_PRIVATE, fd, 0);
        if (addr == MAP_FAILED) {
            perror("mmap");
            exit(EXIT_FAILURE);
        }

        munmap(addr, page_size * num_pages);
    }

    close(fd);
    unlink(template);
    printf("Done file-backed mmap exec test\n");
    return 0;
}
