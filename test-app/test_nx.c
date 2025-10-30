#define _GNU_SOURCE
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static void sigsegv_handler(int sig, siginfo_t *si, void *unused) {
  (void)unused;
  fprintf(
      stderr,
      "Caught SIGSEGV (address=%p). Execution may have been blocked by NX.\n",
      si->si_addr);
  _exit(126);
}

int main(void) {
  /* Install SIGSEGV handler so we can report if execution is blocked */
  struct sigaction sa;
  sa.sa_sigaction = sigsegv_handler;
  sa.sa_flags = SA_SIGINFO;
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIGSEGV, &sa, NULL) == -1) {
    perror("sigaction");
    return 2;
  }

  /* Page size and allocation size */
  long pagesz = sysconf(_SC_PAGESIZE);
  if (pagesz <= 0)
    pagesz = 4096;
  size_t size = (size_t)pagesz;

  /* Request RWX mapping */
  void *p = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC,
                 MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if (p == MAP_FAILED) {
    fprintf(stderr, "mmap(RWX) failed: %s\n", strerror(errno));
    fprintf(stderr, "If your system denies RWX mappings, try the variant that "
                    "mprotect()s to RX.\n");
    return 3;
  }

  printf("mmap succeeded at %p (size %zu). Writing NOPs and RET, then "
         "calling...\n",
         p, size);

  /* Fill with NOPs and place a RET at the end.
     x86_64: NOP = 0x90, RET = 0xC3 */
  unsigned char *code = (unsigned char *)p;
  size_t i;
  for (i = 0; i < 128 && i < size - 1; ++i)
    code[i] = 0x90; /* NOP */
  code[i] = 0xC3;   /* RET */

  /* Cast and call */
  typedef void (*fn_t)(void);
  fn_t fn = (fn_t)p;

  printf("Calling the mapped code...\n");
  fflush(stdout);

  /* Call in a way that returns here on success */
  fn();

  printf("Returned from mapped code successfully — execution allowed.\n");

  /* Clean up */
  if (munmap(p, size) == -1) {
    perror("munmap");
    return 4;
  }

  return 0;
}
