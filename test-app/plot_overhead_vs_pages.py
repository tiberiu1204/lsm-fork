import pandas as pd
import matplotlib.pyplot as plt

# Filenames
files = {
    "mmap_lsm": "perf_results/mmap_lsm_1_5000.csv",
    "mprotect_lsm": "perf_results/mprotect_lsm_1_3000.csv",
    "mmap_no_lsm": "perf_results/mmap_no_lsm_1_5000.csv",
    "mprotect_no_lsm": "perf_results/mprotect_no_lsm_1_3000.csv",
}

# Load and average
results = {}
for key, filename in files.items():
    df = pd.read_csv(filename)
    df_avg = df.groupby("num_pages")["elapsed_time_ns"].mean().reset_index()
    results[key] = df_avg

# Compute overheads
def compute_overhead(lsm_df, no_lsm_df):
    merged = pd.merge(lsm_df, no_lsm_df, on="num_pages", suffixes=("_lsm", "_no_lsm"))
    merged["overhead_ns"] = merged["elapsed_time_ns_lsm"] - merged["elapsed_time_ns_no_lsm"]
    return merged

mmap_overhead = compute_overhead(results["mmap_lsm"], results["mmap_no_lsm"])
mprotect_overhead = compute_overhead(results["mprotect_lsm"], results["mprotect_no_lsm"])

# Plotting function
def plot_overhead(df, title, output_file):
    plt.figure(figsize=(8, 5))
    plt.plot(df["num_pages"], df["overhead_ns"], marker="o")
    plt.title(title)
    plt.xlabel("Number of Pages")
    plt.ylabel("Overhead (ns)")
    plt.grid(True)
    plt.tight_layout()
    plt.show()

    #plt.savefig(output_file)
    plt.close()

def plot_elapsed_time(df, title, output_file):
    plt.figure(figsize=(8, 5))
    plt.plot(df["num_pages"], df["elapsed_time_ns"], marker="o")
    plt.title(title)
    plt.xlabel("Number of Pages")
    plt.ylabel("Elapsed Time (ns)")
    plt.grid(True)
    plt.tight_layout()
    plt.show()

    #plt.savefig(output_file)
    plt.close()

# Plot elapsed time for mmap
#plot_elapsed_time(results["mmap_lsm"], "mmap Elapsed Time vs. Number of Pages", "mmap_elapsed_time.png")
# Plot elapsed time for mprotect
#plot_elapsed_time(results["mprotect_lsm"], "mprotect Elapsed Time vs. Number of Pages", "mprotect_elapsed_time.png")

# Generate plots
plot_overhead(mmap_overhead, "mmap Overhead vs. Number of Pages", "mmap_overhead.png")
#plot_overhead(mprotect_overhead, "mprotect Overhead vs. Number of Pages", "mprotect_overhead.png")


# moving average function
def moving_average(data, window_size):
    return data.rolling(window=window_size).mean()
# moving average for mmap
mmap_overhead["overhead_ns"] = moving_average(mmap_overhead["overhead_ns"], 10)
# moving average for mprotect
mprotect_overhead["overhead_ns"] = moving_average(mprotect_overhead["overhead_ns"], 10)
# Plot moving average overhead for mmap
plot_overhead(mmap_overhead, "mmap Overhead vs. Number of Pages (Moving Average)", "mmap_overhead_moving_average.png")
# Plot moving average overhead for mprotect
plot_overhead(mprotect_overhead, "mprotect Overhead vs. Number of Pages (Moving Average)", "mprotect_overhead_moving_average.png")
