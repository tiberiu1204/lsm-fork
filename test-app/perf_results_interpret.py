import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

# # Load CSV
# df = pd.read_csv("perf_results.csv")

# # Group by the relevant identifying columns
# group_cols = ["test_type", "test_no", "config", "analyzer_type", "iterations", "mem_pages_no"]

# # Compute averages for wall_time and sys_time
# averaged = (
#     df.groupby(group_cols)
#     .agg({
#         "wall_time_ms": "mean",
#         "sys_time_ms": "mean"
#     })
#     .reset_index()
# )

# # Save and display
# print(averaged)
# averaged.to_csv("averaged_results.csv", index=False)

# Load the averaged CSV
df = pd.read_csv("averaged_results.csv")

# Optional: sort for consistent plotting
df = df.sort_values(by=["test_type", "mem_pages_no", "iterations", "config"])

# Combine analyzer into config for clearer legends (optional)
df["config_combined"] = df["config"] + "-" + df["analyzer_type"]

# Plot for each test_type
for test in df["test_type"].unique():
    subset = df[df["test_type"] == test]
    
    # For each mem_pages_no value, generate one plot
    for pages in sorted(subset["mem_pages_no"].unique()):
        pages_df = subset[subset["mem_pages_no"] == pages]

        # ---- Wall Time Plot ----
        plt.figure(figsize=(10, 6))
        sns.lineplot(
            data=pages_df,
            x="iterations",
            y="wall_time_ms",
            hue="config_combined",
            marker="o"
        )
        plt.title(f"{test.upper()} – Wall Time vs Iterations (pages={pages})")
        plt.xlabel("Iterations")
        plt.ylabel("Wall Time (ms)")
        plt.grid(True)
        plt.legend(title="Config")
        plt.tight_layout()
        plt.savefig(f"{test}_wall_time_pages_{pages}.png")
        plt.close()

        # ---- Sys Time Plot ----
        plt.figure(figsize=(10, 6))
        sns.lineplot(
            data=pages_df,
            x="iterations",
            y="sys_time_ms",
            hue="config_combined",
            marker="o"
        )
        plt.title(f"{test.upper()} – Sys Time vs Iterations (pages={pages})")
        plt.xlabel("Iterations")
        plt.ylabel("System Time (ms)")
        plt.grid(True)
        plt.legend(title="Config")
        plt.tight_layout()
        plt.savefig(f"{test}_sys_time_pages_{pages}.png")
        plt.close()
