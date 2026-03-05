import os
import subprocess
import sys
import logging
import argparse
from datetime import datetime
from tqdm import tqdm

# Configuration
FILTER_WX_BIN = "./filter-wx/bin/filter-wx"
JS_BIN = "./filter-wx/bin/spidermonkey"
TARGET_DIR = "./malware_dataset"

# Generate timestamped log filename
timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
LOG_FILENAME = f"log_{timestamp}.txt"

# Setup logging
logging.basicConfig(
    filename=LOG_FILENAME,
    level=logging.INFO,
    format='%(asctime)s - %(message)s',
    datefmt='%H:%M:%S'
)


def run_samples(timeout_seconds):
    # Validation
    if not os.path.exists(TARGET_DIR):
        print(f"Error: Directory '{TARGET_DIR}' not found.")
        return

    # Get list of .js files
    files = [f for f in os.listdir(TARGET_DIR) if f.endswith('.js')]
    files.sort()

    if not files:
        print(f"No .js files found in {TARGET_DIR}")
        return

    print(f"Found {len(files)} samples.")
    print(f"Logging output to: {LOG_FILENAME}")
    print(f"Timeout set to: {timeout_seconds} seconds per sample.")
    print("Controls: [Ctrl+C] Exit script immediately")

    # Convert relative binary paths to absolute paths
    # so they don't break when the subprocess runs inside TARGET_DIR
    filter_wx_abs = os.path.abspath(FILTER_WX_BIN)
    js_bin_abs = os.path.abspath(JS_BIN)

    # Open the log file once to append subprocess output directly
    with open(LOG_FILENAME, "a", buffering=1) as log_file:

        # Tqdm progress bar
        pbar = tqdm(files, unit="file", file=sys.stderr)

        try:
            for filename in pbar:
                pbar.set_description(f"Running {filename[:20]}...")

                # Log the start of this file
                log_message = f"--- STARTED: {filename} ---\n"
                log_file.write(log_message)
                log_file.flush()

                # Construct command using absolute paths and ONLY the filename
                cmd = [filter_wx_abs, js_bin_abs, filename]

                process = None
                try:
                    # Start the process with cwd set to TARGET_DIR
                    process = subprocess.Popen(
                        cmd,
                        stdout=log_file,
                        stderr=log_file,
                        text=True,
                        cwd=TARGET_DIR  # <--- This handles the directory change cleanly
                    )

                    # Wait for it to finish with a timeout
                    process.wait(timeout=timeout_seconds)

                    # Log exit code if it finished normally
                    log_file.write(
                        f"--- FINISHED: {filename} (Exit Code: {process.returncode}) ---\n\n")

                except subprocess.TimeoutExpired:
                    # Handle the timeout case
                    process.kill()
                    log_file.write(
                        f"\n--- TIMED OUT ({timeout_seconds}s): {filename} ---\n\n")

                except Exception as e:
                    # General error handling
                    if process:
                        process.kill()
                    log_file.write(
                        f"\n--- ERROR executing {filename}: {str(e)} ---\n\n")

        except KeyboardInterrupt:
            print("\nCtrl+C detected. Exiting script...")
            # Ensure the current process is killed before exiting
            if process:
                try:
                    process.kill()
                except:
                    pass
            sys.exit(0)


if __name__ == "__main__":
    # Argument parsing
    parser = argparse.ArgumentParser(
        description="Run malware samples with a timeout.")
    parser.add_argument(
        "--timeout",
        type=int,
        default=600,
        help="Timeout in seconds for each sample (default: 600)"
    )

    args = parser.parse_args()

    run_samples(args.timeout)
