#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/bpf.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include "linux/bpf.h"
#include "bpf_insn.h"
#include <linux/if_ether.h>
#include <stddef.h>
#include <linux/ip.h>

struct bpf_prog_load_attr {
    __u32 prog_type;        // Type of eBPF program
    __u32 insn_cnt;         // Number of instructions in the eBPF program
    __aligned_u64 insns;    // Pointer to eBPF program bytecode
    __aligned_u64 license;  // Pointer to license string
    __u32 log_level;        // Log verbosity level
    __u32 log_size;         // Size of the log buffer
    __aligned_u64 log_buf;  // Log buffer for verifier output
    __u32 kern_version;     // Kernel version (used with BPF_PROG_TYPE_KPROBE)
};

int main() {

    struct bpf_insn prog[] = {
		BPF_MOV64_REG(BPF_REG_6, BPF_REG_1),
		BPF_MOV64_IMM(BPF_REG_0, 0),
		BPF_EXIT_INSN(),
	};
	size_t insns_cnt = sizeof(prog) / sizeof(struct bpf_insn);
    // Prepare the BPF program attributes
    struct bpf_prog_load_attr attr = {
        .prog_type = BPF_PROG_TYPE_SOCKET_FILTER,
        .insn_cnt = insns_cnt,  // Number of instructions
        .insns = (unsigned long)prog,      // Pointer to the bytecode
        .license = (unsigned long)"GPL",       // License string
        .log_level = 0,
        .log_size = 0,
        .log_buf = 0,
        .kern_version = 0,
    };

    // Call the bpf() syscall to load the program
    int prog_fd = syscall(SYS_bpf, BPF_PROG_LOAD, &attr, sizeof(attr));

    if (prog_fd == -1) {
        perror("Failed to load BPF program");
        return 1;
    }

    printf("Successfully loaded BPF program with fd: %d\n", prog_fd);

    close(prog_fd);
    return 0;
}
