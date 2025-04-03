
build: analyzer mmaping mprotecting bpf

clean:
	rm -f analyzer mmaping mprotecting bpf

analyzer: analyzer.c
	$(CC) -static -o $@ $< -I /usr/include/libnl3 -lnl-3 -lnl-genl-3

mmaping: mmaping.c
	$(CC) -o $@ $<

mprotecting: mprotecting.c
	$(CC) -o $@ $<

bpf: bpf_example.c
	$(CC) -o $@ $<