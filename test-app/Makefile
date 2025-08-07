JIT_SPRAY_POC = jit-spray-poc-for-ksp

build: analyzer mmaping mprotecting bpf mprotect_perf_test mmap_perf_test read_char_device

clean:
	rm -f analyzer mmaping mprotecting bpf mprotect_perf_test mmap_perf_test read_char_device test_nx

analyzer: analyzer.c
	$(CC) -o $@ $< yara/libyara.a -static -I yara/include -I /usr/include/libnl3 -lnl-3 -lnl-genl-3 -lm
mmaping: mmaping.c
	$(CC) -o $@ $<

mprotecting: mprotecting.c
	$(CC) -o $@ $<

bpf: bpf_example.c
	$(CC) -o $@ $<

mprotect_perf_test: mprotect_perf_test.c
	$(CC) -o $@ $<

mmap_perf_test: mmap_perf_test.c
	$(CC) -o $@ $<

read_char_device: read_char_device.c
	$(CC) -o $@ $<

test_nx: test_nx.c
	$(CC) -o $@ $<

jit_spray_poc_for_ksp: $(JIT_SPRAY_POC)/alameda.c
	cd $(JIT_SPRAY_POC) && \
	$(CC) -Wall -O2 -o alameda alameda.c

start_jit_spray_poc: $(JIT_SPRAY_POC)/alameda
	cd $(JIT_SPRAY_POC)/ko && \
	$(MAKE) && \
	insmod jump.ko && \
	echo 1 | sudo tee /proc/sys/net/core/bpf_jit_enable && \
	echo 0 | sudo tee /proc/sys/kernel/kptr_restrict && \
	../alameda
	

