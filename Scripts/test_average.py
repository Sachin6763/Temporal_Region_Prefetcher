import os
import pandas as pd

input_dir = "prefetch_results/csv"
output_file = "prefetch_results/avg.csv"

print(f"Output file path: {output_file}")

all_data = []

pd.set_option('display.max_rows', None)

# Processing all CSV files in the input directory
for filename in os.listdir(input_dir):
    if filename.endswith(".csv"):
        filepath = os.path.join(input_dir, filename)
        print(f"\nProcessing file: {filepath}")
        df = pd.read_csv(filepath)

        if df.empty:
            print("⚠️ Skipping empty file")
            continue

        all_data.append(df)

# Combine and average
if all_data:
    combined_df = pd.concat(all_data, ignore_index=True)

    # Ensure all relevant columns are present
    required_columns = [
        "Prefetcher", "IPC", "TotalCycles", "L1I_HitRatio", 
        "L1D_HitRatio", "L2C_HitRatio", "LLC_HitRatio", 
        "L2C_PF_Requested", "L2C_PF_Issued", "L2C_PF_Useful"
    ]

    # Check if all necessary columns exist in the combined dataframe
    missing_columns = [col for col in required_columns if col not in combined_df.columns]
    if missing_columns:
        print(f"⚠️ Missing columns: {', '.join(missing_columns)}")
    else:
        # Average numeric columns grouped by prefetcher
        avg_df = combined_df.groupby("Prefetcher", as_index=False).mean(numeric_only=True)

        # Reorder columns: Prefetcher first, then the rest
        columns_order = ["Prefetcher"] + [col for col in required_columns if col != "Prefetcher"]
        avg_df = avg_df[columns_order]

        print("\nAll rows of averaged data:")
        print(avg_df.to_string())

        # Save the averaged CSV
        avg_df.to_csv(output_file, index=False)
        print(f"\n✅ Averaged metrics by prefetcher saved to: {output_file}")
else:
    print("⚠️ No valid data found in any CSV file.")
