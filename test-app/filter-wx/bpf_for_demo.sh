#!/bin/bash

set -e

bpftool btf dump file /sys/kernel/btf/vmlinux format c > test/vmlinux.h

clang -target bpf -Wall -O2 -g \
  -I./test \
  -I/usr/include \
  -c test/bpf_for_demo.c -o bin/bpf_for_demo.o

bpftool prog load bin/bpf_for_demo.o /sys/fs/bpf/bpf_for_demo type raw_tracepoint

bpftool prog run pinned /sys/fs/bpf/bpf_for_demo repeat 0

cat /sys/kernel/debug/tracing/trace

rm -f test/vmlinux.h
rm -f /sys/fs/bpf/bpf_for_demo
