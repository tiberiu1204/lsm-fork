import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

# Load averaged results
df = pd.read_csv("averaged_results.csv")

# Prepare data for overhead computation
pivot = df.pivot_table(
    index=["test_type", "mem_pages_no", "iterations"],
    columns=["config", "analyzer_type"],
    values="wall_time_ms"
).reset_index()

# Create pivot table
pivot = df.pivot_table(
    index=["test_type", "mem_pages_no", "iterations"],
    columns=["config", "analyzer_type"],
    values="wall_time_ms"
)

# Flatten MultiIndex columns (e.g., ('lsm', 'nop') -> 'lsm_nop')
pivot.columns = ['_'.join(map(str, col)).strip() for col in pivot.columns.values]
pivot = pivot.reset_index()

# Debug: print column names to verify
print("Columns in pivot:", pivot.columns.tolist())

# Compute overheads
pivot["overhead_nop"] = pivot["lsm_nop"] - pivot["no_lsm_-"]
pivot["overhead_linear"] = pivot["lsm_liniar"] - pivot["no_lsm_-"]


# Plot overheads for each test_type and mem_pages_no
for test_type in pivot["test_type"].unique():
    test_df = pivot[pivot["test_type"] == test_type]

    for pages in sorted(test_df["mem_pages_no"].unique()):
        pages_df = test_df[test_df["mem_pages_no"] == pages]

        plt.figure(figsize=(10, 6))
        sns.lineplot(
            data=pages_df,
            x="iterations",
            y="overhead_nop",
            label="LSM - NOP",
            marker="o"
        )
        sns.lineplot(
            data=pages_df,
            x="iterations",
            y="overhead_linear",
            label="LSM - LINEAR",
            marker="o"
        )
        plt.title(f"{test_type.upper()} – Overhead vs Iterations (pages={pages})")
        plt.xlabel("Iterations")
        plt.ylabel("Overhead (ms)")
        plt.grid(True)
        plt.legend()
        plt.tight_layout()
        plt.savefig(f"{test_type}_overhead_pages_{pages}.png")
        plt.close()
