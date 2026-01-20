#!/bin/bash

set -e

FILTER=./bin/filter-wx

# --- Paths ---
D8=./bin/v8/d8
SPIDERMONKEY=./bin/spidermonkey
LUAJIT=luajit
DOTNET="dotnet run -c Release --project"
WASM="cargo run --release --manifest-path"
EBPF_BIN="./bin/ebpf_demo"

# --- Demo Files ---
JS_DEMO=./test/fibonacci.js
LUA_DEMO=./test/lua_demo.lua
DOTNET_DEMO=./test/DotNET
WASM_DEMO=./test/wasmtime_jit_demo/Cargo.toml
EBPF_SRC=./test/ebpf_demo.c

# --- Compilation Step for eBPF Demo ---
# Ensure bin directory exists
mkdir -p ./bin

if [ ! -f "$EBPF_BIN" ] || [ "$EBPF_SRC" -nt "$EBPF_BIN" ]; then
    echo "Compiling eBPF demo..."
    gcc "$EBPF_SRC" -o "$EBPF_BIN"
fi

run_all() {
    # $FILTER $D8 $JS_DEMO
    $FILTER $SPIDERMONKEY $JS_DEMO
    $FILTER $LUAJIT $LUA_DEMO
    $FILTER $DOTNET $DOTNET_DEMO
    $FILTER $WASM $WASM_DEMO
    $FILTER $EBPF_BIN
}

print_help() {
    echo "Usage: $0 [options]"
    echo
    echo "Options:"
    echo "  d8            Run test using V8 D8 (will crash)"
    echo "  spidermonkey  Run test using SpiderMonkey"
    echo "  luajit        Run test using LuaJIT"
    echo "  dotnet        Run test using .NET demo"
    echo "  wasm          Run test using Wasmtime demo"
    echo "  ebpf          Run test using eBPF JIT demo (0xDEADBEEF)"
    echo "  all           Run all tests (default if no argument is given)"
    echo "  help          Show this help message"
}

# If no arguments or "all" is given, run all
if [ $# -eq 0 ] || [[ "$@" == *"all"* ]]; then
    run_all
    exit 0
fi

# If "help" is given, display help
if [[ "$@" == *"help"* ]]; then
    print_help
    exit 0
fi

# Loop through the given arguments and run the corresponding tests
for arg in "$@"; do
    case $arg in
        d8)
            $FILTER $D8 $JS_DEMO
            ;;
        spidermonkey)
            $FILTER $SPIDERMONKEY $JS_DEMO
            ;;
        luajit)
            $FILTER $LUAJIT $LUA_DEMO
            ;;
        dotnet)
            $FILTER $DOTNET $DOTNET_DEMO
            ;;
        wasm)
            $FILTER $WASM $WASM_DEMO
            ;;
        ebpf)
            # Ensure JIT is enabled, otherwise hook might skip it
            if [ "$(cat /proc/sys/net/core/bpf_jit_enable)" = "0" ]; then
                echo "WARNING: /proc/sys/net/core/bpf_jit_enable is 0."
                echo "The LSM hook requires JIT to be enabled to catch executable code."
            fi
            $FILTER $EBPF_BIN
            ;;
        *)
            echo "Unknown option: $arg"
            print_help
            ;;
    esac
done
