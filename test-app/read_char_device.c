#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

int main()
{
    int dev_fd = open("/dev/lsm_perf", O_RDONLY);
    if (dev_fd < 0) {
        perror("Failed to open char device");
        return 1;
    }

    FILE *outputFile = fopen("perf_results/perf_output.txt", "w");
    if (outputFile == NULL) {
        perror("Failed to open output file");
        close(dev_fd);
        return EXIT_FAILURE;
    }

    char buffer[256];
    size_t bytesRead;

    while (1) {
        bytesRead = read(dev_fd, buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';  // Null-terminate the string
            fprintf(outputFile, "%s", buffer);
            fflush(outputFile); // Ensure data is written immediately
        } else if (bytesRead == 0) {
            usleep(1000); // No data available, wait a bit
        } else {
            perror("Read error");
            break;
        }
    }
    
    fclose(outputFile);
    close(dev_fd);
    return 0;
}