#include <sys/mman.h>

#include "util.h"

int32_t main(int32_t argc, char *argv[]) {
  /* create a rwx buffer */
  uint8_t *buff =
      (uint8_t *)mmap(NULL, 4096, PROT_READ | PROT_WRITE | PROT_EXEC,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  GOTO(buff == MAP_FAILED, try_rwnx, "failed mmap; trying mprotect (%s)",
       strerror(errno));

  /* success; go initialize nop sled on rwx buffer */
  DEBUG("mmap(rwx) succeeded");
  goto nop_sled;

try_rwnx:
  /* create a rw- buffer */
  buff = (uint8_t *)mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  DIE(buff == MAP_FAILED, "even rw- mmap failed (%s)", strerror(errno));
  DEBUG("mmap(rw-) succeeded");

nop_sled:
  /* initialize buffer with nop sled */
  memset(buff, 0x90, 4096); /* nop */
  buff[4095] = 0xc3;        /* ret */

  /* do a mprotect in case buffer is currently rw- */
  int32_t ans = mprotect(buff, 4096, PROT_READ | PROT_WRITE | PROT_EXEC);
  DIE(ans == -1, "failed mprotect (%s)", strerror(errno));
  DEBUG("mprotect(rwx) succeeded");

  /* do actual exec (not that we can stop it) */
  void (*my_func)(void) = (void *)buff;
  DEBUG("jumping onto nop sled");
  my_func();
  DEBUG("returned from nop sled");

  /* cleanup */
  munmap(buff, 4096);

  return 0;
}
