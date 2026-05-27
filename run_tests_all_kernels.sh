#!/bin/bash

set -e

# 1. Prepare Host Environment
echo "[*] Compiling tcp-server..."
cd test-app/tcp-server
make build
cd ../../

echo "[*] Caching sudo credentials..."
sudo -v

# Keep sudo alive in background
while true; do sudo -n true; sleep 60; kill -0 "$$" || exit; done 2>/dev/null &
SUDO_PID=$!

# Ensure all_dumps exists
mkdir -p all_dumps

# 2. Collect Kernel Tags
echo "[*] Collecting kernel tags..."
cd linux

# Check if we have upstream tags. If not, add remote and fetch them.
if ! git remote -v | grep -q 'upstream'; then
    echo "[*] Adding upstream kernel remote and fetching tags (this may take a while)..."
    git remote add upstream https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git
fi
echo "[*] Fetching upstream tags..."
git fetch upstream --tags

# Get all mainline tags matching vX.Y, sort them, and filter for >= 4.18
# We use a refined regex to ensure we get versions like v4.18, v5.0, v6.1 etc.
# and then use a version-aware comparison to filter from v4.18 onwards.
TAGS=$(git tag | grep -E '^v[0-9]+\.[0-9]+$' | sort -V | sed -n '/^v6.12$/,$p')
cd ..

# Check if expect is installed
if ! command -v expect &> /dev/null; then
    echo "[!] expect is not installed. Please install it with 'sudo apt-get install expect'."
    kill $SUDO_PID
    exit 1
fi

# 3. Execution Loop
for TAG in $TAGS; do
    echo "=================================================================="
    echo "[*] Processing Kernel version: $TAG"
    echo "=================================================================="

    echo "[*] Checking out $TAG in linux/"
    cd linux
    # Clean previous build artifacts and abort any stuck am
    git am --abort 2>/dev/null || true
    git clean -fdx
    git checkout -f "$TAG"

    echo "[*] Applying custom patches..."
    if ! git am ../custom_patches/*.patch; then
        echo "[!] Failed to apply patches for $TAG. Aborting patch apply and skipping version..."
        git am --abort
        cd ..
        continue
    fi

    echo "[*] Fixing libbpf Makefile to prevent Werror issues..."
    sed -i 's/-Werror //g' tools/lib/bpf/Makefile || true

    cd ..

    echo "[*] Building kernel $TAG..."
    # Build the kernel (this uses make build_linux from the root Makefile)
    if ! make build_linux; then
        echo "[!] Build failed for $TAG. Skipping to next version..."
        continue
    fi

    echo "[*] Starting tcp_server in background..."
    # Remove any stale dumps directory
    rm -rf dumps
    ./test-app/tcp-server/bin/tcp_server > tcp_server.log 2>&1 &
    TCP_SERVER_PID=$!

    # Small delay to ensure server is ready
    sleep 2

    echo "[*] Running expect script to boot VM and run payload..."
    LOG_FILE="${TAG}.log"
    
    # Use expect to automate VM interaction
    expect <<EOF
set timeout -1
log_file -noappend $LOG_FILE
spawn ./run_safe.sh

expect "login:" {
    send "root\r"
}
expect "Password:" {
    send "1234\r"
}
expect "# " {
    send "cd /root/share\r"
}
expect "# " {
    send "./run_all_bad_bpf.sh\r"
}
expect "All programs finished." {
    # Let it poweroff automatically as run_all_bad_bpf.sh calls poweroff
}
expect eof
EOF

    echo "[*] VM stopped. Cleaning up tcp_server..."
    kill $TCP_SERVER_PID || true
    wait $TCP_SERVER_PID 2>/dev/null || true

    echo "[*] Moving dumps to all_dumps/dumps_${TAG}..."
    if [ -d "dumps" ]; then
        mv dumps "all_dumps/dumps_${TAG}"
    else
        echo "[!] No dumps/ directory found for $TAG. Creating empty dir."
        mkdir -p "all_dumps/dumps_${TAG}"
    fi

    # Move logs
    if [ -f "$LOG_FILE" ]; then
        mv "$LOG_FILE" "all_dumps/dumps_${TAG}/"
    fi
    if [ -f "tcp_server.log" ]; then
        mv "tcp_server.log" "all_dumps/dumps_${TAG}/"
    fi

    echo "[*] Done with $TAG"
done

# Cleanup sudo keepalive
kill $SUDO_PID || true

echo "=================================================================="
echo "[*] All kernel versions processed."
echo "=================================================================="
