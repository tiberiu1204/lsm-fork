#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

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
    void *addr = mmap(NULL, page_size * num_pages, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (addr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Initialize the memory with some data
    for (size_t i = 0; i < page_size * num_pages; i++) {
        ((char *)addr)[i] = (char)i;
    }

    for (int i = 0; i < iterations; i++) {
        if (mprotect(addr, page_size * num_pages, PROT_READ | PROT_EXEC) < 0) {
            perror("mprotect exec");
            exit(EXIT_FAILURE);
        }

        if (mprotect(addr, page_size * num_pages, PROT_READ | PROT_WRITE) < 0) {
            perror("mprotect write");
            exit(EXIT_FAILURE);
        }

        ((char *)addr)[0] = (char)i; // Write to the memory
    }

    munmap(addr, page_size * num_pages);
    printf("Done anonymous mprotect exec test\n");
    return 0;
}
