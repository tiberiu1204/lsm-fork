#include <seccomp.h>  /* libseccomp API   */
#include <sys/mman.h> /* PROT_* constants */
#include <unistd.h>   /* exec             */

#include "util.h"

#define PROT_WX (PROT_WRITE | PROT_EXEC)

int32_t main(int32_t argc, char *argv[]) {
  char **exec_args;    /* exec cmdline arguments */
  scmp_filter_ctx ctx; /* seccomp context        */
  int32_t ans;         /* answer                 */

  /* sanity check */
  DIE(argc < 2, "please specify a program to run (and arguments)");

  /* default action if no rule is matched will be ALLOW */
  ctx = seccomp_init(SCMP_ACT_ALLOW);
  DIE(!ctx, "unable to initialize seccomp context");

  /* match mmap(PROT_WRITE | PROT_EXEC) --> EPERM error */
  ans = seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EPERM), SCMP_SYS(mmap), 1,
                         SCMP_A2(SCMP_CMP_MASKED_EQ, PROT_WX, PROT_WX));
  GOTO(ans < 0, out_clean, "unable to add rule for mmap() (%s)",
       strerror(-ans));

  /* match mprotect(PROT_WRITE | PROT_EXEC --> EPERM error */
  ans = seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EPERM), SCMP_SYS(mprotect), 1,
                         SCMP_A2(SCMP_CMP_MASKED_EQ, PROT_WX, PROT_WX));
  GOTO(ans < 0, out_clean, "unable to add rule for mprotect() (%s)",
       strerror(-ans));

  /* match pkey_mprotect(PROT_WRITE | PROT_EXEC --> EPERM error */
  ans = seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EPERM), SCMP_SYS(pkey_mprotect), 1,
                         SCMP_A2(SCMP_CMP_MASKED_EQ, PROT_WX, PROT_WX));
  GOTO(ans < 0, out_clean, "unable to add rule for mprotect() (%s)",
       strerror(-ans));

  /* load eBPF filter into the kernel */
  ans = seccomp_load(ctx);
  GOTO(ans < 0, out_clean, "unable to load filter program (%s)",
       strerror(-ans));

  /* prepare (NULL-terminated) array of cmdline arguments */
  exec_args = (char **)calloc(argc, sizeof(char *));
  GOTO(!exec_args, out_clean, "unable to allocate memory (%s)",
       strerror(errno));
  exec_args[0] = argv[1];

  WAR("command: %s", argv[1]);
  for (size_t i = 2; i < argc; i++) {
    exec_args[i - 1] = argv[i];
    WAR("    arg[%2lu]: %s", i - 1, exec_args[i - 1]);
  }

  /* exec intended command (searched in PATH) */
  ans = execvp(argv[1], exec_args);
  GOTO(ans == -1, out_clean, "unable to exec (%s)", strerror(errno));

  /* error cleanup path */
out_clean:
  seccomp_release(ctx);

  return -1;
}
