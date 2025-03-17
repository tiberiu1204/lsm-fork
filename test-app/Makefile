
build: analyzer mmaping mprotecting

clean:
	rm -f analyzer mmaping mprotecting

analyzer: analyzer.c
	$(CC) -static -o $@ $< -I /usr/include/libnl3 -lnl-3 -lnl-genl-3

mmaping: mmaping.c
	$(CC) -o $@ $<

mprotecting: mprotecting.c
	$(CC) -o $@ $<