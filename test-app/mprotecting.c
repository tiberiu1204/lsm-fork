#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <data>\n", argv[0]);
        return EXIT_FAILURE;
    }
    const char *data = argv[1];

    char *mmaped_anon = mmap(NULL, 4096 * 10, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mmaped_anon == MAP_FAILED) {
        perror("Error mmapping anonymous memory");
        return EXIT_FAILURE;
    }

    memcpy(mmaped_anon, data, strlen(data));

    int ret = mprotect(mmaped_anon, 4096 * 10, PROT_READ | PROT_EXEC);
    if (ret) {
        perror("Error mprotecting memory");
        return EXIT_FAILURE;
    } else {
        printf("mprotect ok\n");
    }


    return EXIT_SUCCESS;
}