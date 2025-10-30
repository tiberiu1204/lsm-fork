#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

int main(void) {
  void *buf = mmap(NULL, 4096, PROT_READ | PROT_WRITE | PROT_EXEC,
                   MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if (buf == MAP_FAILED) {
    fprintf(stderr, "mmap(RWX) failed: %s\n", strerror(errno));
    return 0;
  }

  memset(buf, 0x90, 4096);
  ((char *)buf)[4095] = 0xC3;

  ((void (*)())buf)();

  printf("Returned from mapped code successfully; execution allowed.\n");

  return 0;
}
