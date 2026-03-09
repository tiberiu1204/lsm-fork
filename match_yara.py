import os
import sys
import gzip
import logging
import re
from datetime import datetime
from collections import defaultdict
import yara
from tqdm import tqdm

# --- Configuration ---
DUMPS_DIR = "test-app/tcp-server/"
RULES_DIR = "rules/"


def setup_logging():
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    log_filename = f"yara_logs_{timestamp}.txt"
    report_filename = f"yara_report_{timestamp}.txt"

    # Configure logging to go ONLY to the file, keeping stdout clean
    logging.basicConfig(
        filename=log_filename,
        level=logging.INFO,
        format='%(asctime)s [%(levelname)s] %(message)s'
    )
    return report_filename


def compile_yara_rules(rules_dir):
    valid_filepaths = {}
    failed_rules = []

    # 1. Test compile each rule individually to isolate syntax errors
    for root, _, files in os.walk(rules_dir):
        for file in files:
            if file.endswith(('.yar', '.yara', '.rule')):
                full_path = os.path.join(root, file)
                try:
                    # Attempt isolated compilation
                    yara.compile(filepaths={full_path: full_path})
                    # If successful, add to our valid batch
                    valid_filepaths[full_path] = full_path
                except yara.SyntaxError as e:
                    error_msg = str(e)
                    failed_rules.append((full_path, error_msg))
                    logging.error(f"Syntax Error skipping {
                                  full_path}: {error_msg}")
                except Exception as e:
                    error_msg = str(e)
                    failed_rules.append((full_path, error_msg))
                    logging.error(f"Unexpected Error skipping {
                                  full_path}: {error_msg}")

    if not valid_filepaths:
        logging.error(f"No valid YARA rules remained in {
                      rules_dir} after filtering errors.")
        sys.exit(1)

    try:
        # 2. Compile all valid rules together into a single highly-efficient scanner
        compiled_rules = yara.compile(filepaths=valid_filepaths)
        return compiled_rules, failed_rules
    except Exception as e:
        logging.error(
            f"Unexpected error compiling the final valid YARA rule batch: {e}")
        sys.exit(1)


def extract_pid(filename):
    # Matches the specific format: ..._pid_{pid}_...
    match = re.search(r'_pid_([^_]+)_', filename)
    if match:
        return match.group(1)
    logging.warning(f"Could not extract PID from filename: {filename}")
    return "UNKNOWN"


def main():
    report_filename = setup_logging()

    if not os.path.exists(DUMPS_DIR):
        logging.error(f"Dumps directory not found: {DUMPS_DIR}")
        sys.exit(1)

    rules, failed_rules = compile_yara_rules(RULES_DIR)

    # Gather dump files and pre-calculate totals for statistics
    dump_files = []
    all_pids = set()

    for root, _, files in os.walk(DUMPS_DIR):
        for file in files:
            if file.endswith('.dump.gz'):
                full_path = os.path.join(root, file)
                pid = extract_pid(file)
                dump_files.append((full_path, pid))
                if pid != "UNKNOWN":
                    all_pids.add(pid)

    total_dumps = len(dump_files)
    total_pids = len(all_pids)

    if total_dumps == 0:
        logging.warning("No .dump.gz files found to scan.")
        sys.exit(0)

    # Dictionary to track matches: { rule_name: {'dumps': set(), 'pids': set()} }
    stats = defaultdict(lambda: {'dumps': set(), 'pids': set()})

    # Initialize tqdm progress bar targeting stdout
    progress_bar = tqdm(
        dump_files,
        desc="Scanning Memory Dumps",
        unit="dump",
        file=sys.stdout,
        leave=True
    )

    for file_path, pid in progress_bar:
        try:
            # Decompress in memory to pass bytes directly to YARA
            with gzip.open(file_path, 'rb') as f:
                data = f.read()

            matches = rules.match(data=data)

            for match in matches:
                rule_name = match.rule
                stats[rule_name]['dumps'].add(file_path)
                if pid != "UNKNOWN":
                    stats[rule_name]['pids'].add(pid)

        except gzip.BadGzipFile:
            logging.error(f"Corrupted or invalid gzip file: {file_path}")
        except Exception as e:
            logging.error(f"Error processing {file_path}: {e}")

    # Generate the statistical report
    with open(report_filename, 'w') as report:
        report.write("YARA Memory Dump Scan Report\n")
        report.write("=" * 60 + "\n")
        report.write(f"Scan Completed:     {
                     datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        report.write(f"Total Dumps:        {total_dumps}\n")
        report.write(f"Total Distinct PIDs:{total_pids}\n")
        report.write("=" * 60 + "\n\n")

        # --- New Section for Failed Rules ---
        if failed_rules:
            report.write("### Compilation Failures\n")
            report.write(f"Total Rules Skipped: {len(failed_rules)}\n")
            for failed_file, error_msg in failed_rules:
                report.write(f"  - {failed_file}\n    Reason: {error_msg}\n")
            report.write("\n" + "=" * 60 + "\n\n")

        if not stats:
            report.write(
                "No YARA rule matches were found across any memory dumps.\n")
        else:
            report.write("### Match Statistics\n")
            for rule_name, match_data in sorted(stats.items()):
                matched_dumps_count = len(match_data['dumps'])
                matched_pids_count = len(match_data['pids'])

                dump_pct = (matched_dumps_count / total_dumps) * 100
                pid_pct = (matched_pids_count / total_pids) * \
                    100 if total_pids > 0 else 0

                report.write(f"Rule: {rule_name}\n")
                report.write(
                    f"  - Matched Dumps: {matched_dumps_count:4d} / {total_dumps:<4d} ({dump_pct:>6.2f}%)\n")
                report.write(
                    f"  - Matched PIDs:  {matched_pids_count:4d} / {total_pids:<4d} ({pid_pct:>6.2f}%)\n")
                report.write("-" * 60 + "\n")


if __name__ == "__main__":
    main()
