import os
import json
import subprocess
import re
import csv

# CONFIGURATION
prefetchers = [
    "no", "next_line", "ppf_next_line_5", "ip_stride", "ppf_ip_stride", "spp_dev", "ppf_spp_dev_3", "temporal_region_prefeching_baseline", "temporal_region_prefeching_baseline_ppf_advanced_modular_2", 
]
traces = [
    "600.perlbench_s-210B.champsimtrace.xz",  "603.bwaves_s-3699B.champsimtrace.xz", "621.wrf_s-575B.champsimtrace.xz", "625.x264_s-18B.champsimtrace.xz", "631.deepsjeng_s-928B.champsimtrace.xz", "641.leela_s-800B.champsimtrace.xz", "644.nab_s-5853B.champsimtrace.xz", "648.exchange2_s-1699B.champsimtrace.xz", "657.xz_s-3167B.champsimtrace.xz"
]
trace_dir = "dpc3_traces"
results_dir = "prefetch_results"
csv_dir = os.path.join(results_dir, "csv")  # All CSVs go here
warmup_instr = "20000"
sim_instr = "50000"
config_file = "my_config.json"

# Ensure base results and CSV directory exist
os.makedirs(results_dir, exist_ok=True)
os.makedirs(csv_dir, exist_ok=True)

# Function to update prefetcher in config
def update_config(prefetcher):
    with open(config_file, 'r') as f:
        config = json.load(f)
    config["L2C"]["prefetcher"] = prefetcher
    with open(config_file, 'w') as f:
        json.dump(config, f, indent=4)

def compute_hit_ratio(hit, access):
    try:
        return round(100 * int(hit) / int(access), 4)
    except ZeroDivisionError:
        return "0.0"

# Function to extract IPC and cache hit ratios
def parse_final_output(output):
    metrics = {
        "IPC": "N/A",
        "L1D_HitRatio": "N/A",
        "L2C_HitRatio": "N/A",
        "LLC_HitRatio": "N/A"
    }

    ipc_match = re.search(r"CPU 0 cumulative IPC: ([0-9.]+)", output)
    if ipc_match:
        metrics["IPC"] = ipc_match.group(1)

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

    return metrics

# Create CSV writers for each trace
csv_writers = {}
csv_files = {}

for trace in traces:
    trace_csv_path = os.path.join(csv_dir, f"{trace}.csv")
    f = open(trace_csv_path, 'w', newline='')
    writer = csv.writer(f)
    writer.writerow(["Prefetcher", "IPC", "L1D_HitRatio", "L2C_HitRatio", "LLC_HitRatio"])
    csv_writers[trace] = writer
    csv_files[trace] = f

# Main execution loop
for prefetcher in prefetchers:
    print(f"\n=== Running for prefetcher: {prefetcher} ===")
    update_config(prefetcher)

    print("Running make clean")
    subprocess.run(["make", "clean"])
    print("Running config script")
    subprocess.run(["python3", "config.py", config_file])
    print("Running make")
    subprocess.run(["make", "-j"])

    prefetcher_result_dir = os.path.join(results_dir, prefetcher)
    os.makedirs(prefetcher_result_dir, exist_ok=True)

    for trace in traces:
        trace_path = os.path.join(trace_dir, trace)
        print(f"\n--- Running trace: {trace} ---")

        out_file = os.path.join(prefetcher_result_dir, f"{trace}.txt")

        # Final Champsim execution command
        cmd = [
            "bin/champsim",
            "--warmup_instructions", warmup_instr,
            "--simulation_instructions", sim_instr,
            trace_path
        ]

        result = subprocess.run(cmd, capture_output=True, text=True)

        # Save raw output
        with open(out_file, 'w+') as f:
            f.write(result.stdout)

        # Parse output for metrics
        metrics = parse_final_output(result.stdout)
        csv_writers[trace].writerow([
            prefetcher,
            metrics["IPC"], metrics["L1D_HitRatio"],
            metrics["L2C_HitRatio"], metrics["LLC_HitRatio"]
        ])

# Close all CSV files
for f in csv_files.values():
    f.close()

print(f"\nDone. CSVs are in '{csv_dir}/' and trace outputs per prefetcher are in '{results_dir}/<prefetcher>/'")
