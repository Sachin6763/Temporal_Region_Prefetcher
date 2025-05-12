# Temporal_Region_Prefetcher

## 📘 Description

This project explores the effectiveness of **perceptron-based filtering** to enhance cache prefetching performance. Through a thorough study of various research papers and performance comparisons, we observed how integrating perceptron logic into traditional prefetchers can lead to measurable improvements.

We studied and compared the performance of:

- `Next-line` vs `Next-line with Perceptron`
- `IP-stride` vs `IP-stride with Perceptron`
- `SPP-Dev` vs `SPP-Dev with Perceptron`

After understanding these, we developed a **new baseline prefetcher** called `Temporal Region Prefetcher`, and then applied **perceptron filtering** over it. Our results indicated notable improvements in cache performance, demonstrating how simple neural techniques like perceptrons can benefit low-level system optimizations.

This work brings together **hardware-level prefetching** strategies with **deep learning concepts**, aiming for practical improvements in system design.

---


## 🛠️ Installation

Follow these steps to set up the project:

1. **Clone the repository:**

   Clone the `Temporal_Region_Prefetcher` repository to your local machine:

   ```bash
   git clone https://github.com/Sachin6763/Temporal_Region_Prefetcher.git
2. **Download DPC-3 trace:**

   Professor Daniel Jimenez at Texas A&M University kindly provided traces for DPC-3. Use the following script to download these traces (~20GB size and max simpoint only).

   ```bash
   cd Scripts
   ./download_dpc3_traces.sh

---

## 🚀 How to Run

There are 3 main Python scripts located in the `Scripts` folder. Execute them **in the following order** to simulate and collect results:

### ✅ Step 1: `test.py`

This script compiles the simulator and executes all the prefetchers on the provided traces. It performs:
- Configuration update for each prefetcher.
- Simulator rebuild.
- Trace-wise simulation run.
- IPC and cache hit ratio extraction.
- Result storage in CSV format.


   ```bash
  cd Scripts
  python3 test.py
### ✅ Step 2: `test_fetch_existing_results.py`

This python script does the following:
-    **Configures a list of prefetchers and ChampSim traces** to run simulations on.
    
-    **Ensures result directories (`prefetch_results/` and CSV subfolder)** exist.
    
-    **Updates the `my_config.json` file** to set the current prefetcher in each iteration.
    
-    **Runs `make clean`** and rebuilds ChampSim for each prefetcher using the provided config.
    
-    **Simulates each trace** for every prefetcher using `bin/champsim` with fixed warmup and simulation instruction counts.
    
-    **Captures raw output** of each simulation and saves it in `prefetch_results/<prefetcher>/`.
    
-    **Parses the output** to extract IPC, L1D, L2C, and LLC hit ratios.
    
-    **Writes the extracted metrics into per-trace CSV files** under `prefetch_results/csv/`.
    
-    **Closes all CSV files** after execution is complete.
    
-    **Prints a completion message** indicating where to find the results.


      ```bash
     cd Scripts
     python3 test_fetch_existing_results.py
### ✅ Step 3: `test_average.py`

This script aggregates and averages simulation results for different prefetchers across multiple benchmark traces.

-   **Data Collection:**
    
    -   Iterates over `.csv` files in the input directory.
        
    -   Reads non-empty files into pandas DataFrames and appends to `all_data`.
        
-   **Processing:**
    
    -   Combines all DataFrames into one (`combined_df`).
        
    -   Checks for missing required columns.
        
    -   Averages numeric columns grouped by "Prefetcher".
        
-   **Output:**
    
    -   Reorders columns and prints averaged data.
        
    -   Saves the result to `avg.csv` and displays a confirmation message.

## 📊 Results

To be added

---

## 🧠 Authors

- **Shreyash Thok**  
- **Sachin Sugadare**
- **Jishan Pathan**  
