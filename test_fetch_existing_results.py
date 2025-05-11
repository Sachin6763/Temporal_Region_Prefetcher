import os
import re
import csv

# CONFIGURATION
prefetchers = [
    "next_line", "ppf_next_line_5", "ip_stride", "ppf_ip_stride", "ppf_spp_dev_3", 
    "temporal_region_prefeching_baseline", "temporal_region_prefeching_baseline_ppf_advanced_modular_2", 
]
traces = [
    "600.perlbench_s-210B.champsimtrace.xz", "602.gcc_s-734B.champsimtrace.xz"
]
results_dir = "prefetch_results"
csv_dir = os.path.join(results_dir, "csv")

# Ensure CSV directory exists
os.makedirs(csv_dir, exist_ok=True)

def compute_hit_ratio(hit, access):
    try:
        return round(100 * int(hit) / int(access), 4)
    except ZeroDivisionError:
        return "0.0"

# Function to extract IPC, hit ratios, and total cycles
def parse_final_output(output):
    metrics = {
        "IPC": "N/A",
        "L1I_HitRatio": "N/A",
        "L1D_HitRatio": "N/A",
        "L2C_HitRatio": "N/A",
        "LLC_HitRatio": "N/A",
        "TotalCycles": "N/A",
        "L2C_PF_Requested": "N/A",
        "L2C_PF_Issued": "N/A",
        "L2C_PF_Useful": "N/A",
    }

    ipc_match = re.search(r"CPU 0 cumulative IPC: ([0-9.]+).*?cycles: (\d+)", output)
    if ipc_match:
        metrics["IPC"] = ipc_match.group(1)
        metrics["TotalCycles"] = ipc_match.group(2)

    l1i_match = re.search(r"cpu0_L1I TOTAL\s+ACCESS:\s+(\d+)\s+HIT:\s+(\d+)", output)
    if l1i_match:
        access, hit = l1i_match.groups()
        metrics["L1I_HitRatio"] = compute_hit_ratio(hit, access)

    l1d_match = re.search(r"cpu0_L1D TOTAL\s+ACCESS:\s+(\d+)\s+HIT:\s+(\d+)", output)
    if l1d_match:
        access, hit = l1d_match.groups()
        metrics["L1D_HitRatio"] = compute_hit_ratio(hit, access)

    l2c_match = re.search(r"cpu0_L2C TOTAL\s+ACCESS:\s+(\d+)\s+HIT:\s+(\d+)", output)
    if l2c_match:
        access, hit = l2c_match.groups()
        metrics["L2C_HitRatio"] = compute_hit_ratio(hit, access)

    llc_match = re.search(r"LLC TOTAL\s+ACCESS:\s+(\d+)\s+HIT:\s+(\d+)", output)
    if llc_match:
        access, hit = llc_match.groups()
        metrics["LLC_HitRatio"] = compute_hit_ratio(hit, access)

    # Extracting L2C Prefetch statistics
    l2c_prefetch_match = re.search(r"cpu0_L2C PREFETCH\s+REQUESTED:\s+(\d+)\s+ISSUED:\s+(\d+)\s+USEFUL:\s+(\d+)", output)
    if l2c_prefetch_match:
        requested, issued, useful = l2c_prefetch_match.groups()
        metrics["L2C_PF_Requested"] = requested
        metrics["L2C_PF_Issued"] = issued
        metrics["L2C_PF_Useful"] = useful

    return metrics

# Create CSV writers for each trace
csv_writers = {}
csv_files = {}

for trace in traces:
    trace_csv_path = os.path.join(csv_dir, f"{trace}.csv")
    f = open(trace_csv_path, 'w', newline='')
    writer = csv.writer(f)
    writer.writerow([
        "Prefetcher", "IPC", "TotalCycles", "L1I_HitRatio", 
        "L1D_HitRatio", "L2C_HitRatio", "LLC_HitRatio", 
        "L2C_PF_Requested", "L2C_PF_Issued", "L2C_PF_Useful"
    ])
    csv_writers[trace] = writer
    csv_files[trace] = f

# Read and parse existing output files
for prefetcher in prefetchers:
    print(f"\n=== Processing results for prefetcher: {prefetcher} ===")
    prefetcher_result_dir = os.path.join(results_dir, prefetcher)

    for trace in traces:
        out_file = os.path.join(prefetcher_result_dir, f"{trace}.txt")
        print(f"--- Parsing: {out_file} ---")

        if not os.path.exists(out_file):
            print(f"⚠️ Output file not found: {out_file}")
            continue

        with open(out_file, 'r') as f:
            output = f.read()

        metrics = parse_final_output(output)
        csv_writers[trace].writerow([
            prefetcher,
            metrics["IPC"], metrics["TotalCycles"], metrics["L1I_HitRatio"],
            metrics["L1D_HitRatio"], metrics["L2C_HitRatio"], metrics["LLC_HitRatio"],
            metrics["L2C_PF_Requested"], metrics["L2C_PF_Issued"], metrics["L2C_PF_Useful"]
        ])

# Close all CSV files
for f in csv_files.values():
    f.close()

print(f"\n✅ Done. CSVs with extended metrics saved in '{csv_dir}/'")
