#include <errno.h>
#include <linux/bpf.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

int main() {

  char verifier_log[65536];
  memset(verifier_log, 0, sizeof(verifier_log));

  struct bpf_insn insns[] = {
      {.code = 0xb7, .dst_reg = 0, .src_reg = 0, .off = 0, .imm = 0xDEADBEEF},
      {.code = 0x95, .dst_reg = 0, .src_reg = 0, .off = 0, .imm = 0}};

  union bpf_attr attr;
  memset(&attr, 0, sizeof(attr));

  attr.prog_type = BPF_PROG_TYPE_SOCKET_FILTER;
  attr.insn_cnt = sizeof(insns) / sizeof(insns[0]);
  attr.insns = (uint64_t)(uintptr_t)insns;
  attr.license = (uint64_t)(uintptr_t)"GPL";

  attr.log_level = 1;
  attr.log_size = sizeof(verifier_log);
  attr.log_buf = (uint64_t)(uintptr_t)verifier_log;

  printf("Loading BPF program with marker 0xDEADBEEF...\n");

  int fd = syscall(__NR_bpf, BPF_PROG_LOAD, &attr, sizeof(attr));

  if (fd < 0) {
    perror("Syscall failed");

    fprintf(stderr, "--- BPF Verifier Log ---\n%s\n------------------------\n",
            verifier_log);
    return 1;
  }

  printf("Success! BPF Program loaded (FD: %d).\n", fd);

  sleep(1);
  close(fd);

  return 0;
}
