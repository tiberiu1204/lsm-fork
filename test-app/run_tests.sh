#!/bin/bash

# Check for required CLI arguments
if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <config> <analyzer_type> <test_type>"
    echo "Example: $0 lsm hashing"
    exit 1
fi

CONFIG=$1           # e.g., "lsm" or "no_lsm"
ANALYZER_TYPE=$2    # e.g., "nop" or "liniar"
TEST_TYPE=$3        # e.g., "mmap" or "mprotect"

# Output CSV file
OUT_FILE="perf_results.csv"

# Write CSV header if file doesn't exist
if [ ! -f "$OUT_FILE" ]; then
    echo "test_no,config,analyzer_type,iterations,mem_pages_no,wall_time_ms,sys_time_ms" > "$OUT_FILE"
fi

# Parameters
ITERATIONS_LIST=(100 500 1000 2000 5000 10000)
PAGES_LIST=(4 128 256 512)

test_no=1

for iterations in "${ITERATIONS_LIST[@]}"; do
    for pages in "${PAGES_LIST[@]}"; do
        for run in {1..5}; do
            echo "Running test_no=$test_no config=$CONFIG analyzer=$ANALYZER_TYPE run=$run"

            # Run the test and capture timing info
            TIMES=$( ./time -f "%e %S" "./${TEST_TYPE}_perf_test" $iterations $pages 2>&1 > /dev/null )
            echo "TIMES: $TIMES"
            wall_time=$(echo $TIMES | awk '{print $1 * 1000}')  # seconds → ms
            sys_time=$(echo $TIMES | awk '{print $2 * 1000}')

            # Append to CSV
            echo "$test_no,$TEST_TYPE,$CONFIG,$ANALYZER_TYPE,$iterations,$pages,$wall_time,$sys_time" >> "$OUT_FILE"
        done
        test_no=$((test_no + 1))
    done
done
