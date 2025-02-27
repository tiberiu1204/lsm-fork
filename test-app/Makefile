
build: netlink_test

clean:
	rm -f netlink_test

netlink_test: netlink_test.c
	$(CC) -static -o $@ $< -I /usr/include/libnl3 -lnl-3 -lnl-genl-3