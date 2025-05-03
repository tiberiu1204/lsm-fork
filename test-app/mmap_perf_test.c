// mmap_file_exec_test.c
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <iterations> <memory_pages_number_start> <memory_pages_number_end> <config>\n", argv[0]);
        return EXIT_FAILURE;
    }
    int iterations = atoi(argv[1]);
    int num_pages_start = atoi(argv[2]);
    int num_pages = atoi(argv[3]);
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

    char time_file_name[100];
    sprintf(time_file_name, "perf_results/mmap_%s_%d_%d.csv", argv[4], num_pages_start, num_pages);
    FILE *time_file = fopen(time_file_name, "w");
    if (time_file == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    fprintf(time_file, "num_pages,elapsed_time_ns\n");

    struct timespec start, end;

    for (int i = 0; i < iterations; i++) {
        printf("Iteration number %d\n", i);
        for (int np = num_pages_start; np <= num_pages / 2; np++) {
            // dummy mmap
            void *addr = mmap(NULL, page_size * np, PROT_READ | PROT_EXEC, MAP_PRIVATE, fd, 0);
            if (addr == MAP_FAILED) {
                perror("mmap");
                exit(EXIT_FAILURE);
            }
            munmap(addr, page_size * np);
        }
        for (int np = num_pages_start; np <= num_pages; np++) {

            // Measure mmap time
            clock_gettime(CLOCK_MONOTONIC, &start);
            void *addr = mmap(NULL, page_size * np, PROT_READ | PROT_EXEC, MAP_PRIVATE, fd, 0);
            clock_gettime(CLOCK_MONOTONIC, &end);
            if (addr == MAP_FAILED) {
                perror("mmap");
                exit(EXIT_FAILURE);
            }
    
            munmap(addr, page_size * np);
    
            long elapsed_time = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    
            fprintf(time_file, "%d, %ld\n", np, elapsed_time);
        }
    }

    close(fd);
    unlink(template);
    return 0;
}
