import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# Filepaths to the CSV files
mmap_csv_file = "/home/alexandru/licenta/licenta-lsm/test-app/perf_results/perf_output_mmap.csv"
mprotect_csv_file = "/home/alexandru/licenta/licenta-lsm/test-app/perf_results/perf_output_mprotect.csv"

# Read the CSV files into DataFrames
mmap_df = pd.read_csv(mmap_csv_file)
mprotect_df = pd.read_csv(mprotect_csv_file)

# Rename "mmap_hook" and "mprotect_hook" to "mmap/mprotect_hook"
mmap_df['function'] = mmap_df['function'].replace({'mmap_hook': 'mmap/mprotect_hook'})
mprotect_df['function'] = mprotect_df['function'].replace({'mprotect_hook': 'mmap/mprotect_hook'})

# Calculate the mean fraction_of_total for each function
mmap_means = mmap_df.groupby('function')['fraction_of_total'].mean()
mprotect_means = mprotect_df.groupby('function')['fraction_of_total'].mean()

# Combine the data into a single DataFrame for plotting
combined_df = pd.DataFrame({
    'mmap': mmap_means,
    'mprotect': mprotect_means
}).reset_index()

# Plot the data
x = np.arange(len(combined_df['function']))  # the label locations
width = 0.35  # the width of the bars

fig, ax = plt.subplots(figsize=(10, 6))
bars1 = ax.bar(x - width/2, combined_df['mmap'], width, label='mmap', color='blue')
bars2 = ax.bar(x + width/2, combined_df['mprotect'], width, label='mprotect', color='orange')

# Add labels, title, and legend
ax.set_xlabel('Function')
ax.set_ylabel('Mean Fraction of Total (%)')
ax.set_title('Mean Fraction of Total by Function for mmap and mprotect')
ax.set_xticks(x)
ax.set_xticklabels(combined_df['function'], rotation=45, ha='right')
ax.legend()

# Add value labels to the bars
for bars in [bars1, bars2]:
    ax.bar_label(bars, fmt='%.2f', padding=3)

# Adjust layout and show the plot
plt.tight_layout()
plt.show()