#include "vmlinux.h"
#include <bpf/bpf_helpers.h>

SEC("iter/task")
int bpf_for_demo(void *ctx) {
  int i;
  bpf_for(i, 0, 5) { bpf_printk("Iteration: %d", i); }
  return 0;
}

char _license[] SEC("license") = "GPL";
