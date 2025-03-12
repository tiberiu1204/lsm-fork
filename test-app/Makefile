
build: analyzer mmaping

clean:
	rm -f analyzer mmaping

analyzer: analyzer.c
	$(CC) -static -o $@ $< -I /usr/include/libnl3 -lnl-3 -lnl-genl-3

mmaping: mmaping.c
	$(CC) -o $@ $<