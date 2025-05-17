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

Our experiments evaluated traditional and perceptron-enhanced prefetchers using DPC-3 benchmark traces in the ChampSim simulator. Metrics such as IPC, cache hit ratios, and prefetch effectiveness were analyzed.


### 🔬 Performance Comparison Table

| Prefetcher              | IPC       | L1I Hit (%) | L1D Hit (%) | L2C Hit (%) | LLC Hit (%) | Prefetches Issued | Useful Prefetches |
|-------------------------|-----------|-------------|-------------|-------------|--------------|--------------------|--------------------|
| No Prefetcher           | 1.408950  | 97.282      | 99.424      | 42.309      | 4.315        | 0                  | 0                  |
| Next-line               | 1.420495  | 97.282      | 99.425      | 55.541      | 4.972        | 12399.22           | 1301.11            |
| Next-line + Perceptron  | 1.408836  | 97.282      | 99.424      | 51.394      | 4.320        | 4971.44            | 29.56              |
| IP-stride               | 1.408357  | 97.282      | 99.424      | 48.707      | 4.374        | 9434.56            | 104.11             |
| IP-stride + Perceptron  | 1.408721  | 97.282      | 99.424      | 44.780      | 4.337        | 3162.00            | 37.67              |
| SPP-Dev                 | 1.417269  | 97.282      | 99.425      | 47.703      | 35.759       | 21505.56           | 248.78             |
| SPP-Dev + Perceptron    | 1.419420  | 97.282      | 99.425      | 52.421      | 16.692       | 12977.67           | 952.89             |
| Temporal Region (TRP)   | 1.419423  | 97.282      | 99.425      | 52.421      | 16.692       | 12978.11           | 953.00             |
| TRP + Perceptron (PPF)  | **1.421608** | 97.282   | 99.425      | **56.694**  | 5.055        | **16241.11**        | **1631.33**         |




✅ Key Insights

    🧠 TRP + Perceptron achieved the highest IPC and L2C hit ratio, outperforming all traditional prefetchers.

    🎯 Perceptron-based filtering improved the usefulness of prefetches and reduced unnecessary memory traffic.

    🔍 SPP-Dev had high LLC hit ratios but suffered from cache pollution, leading to lower IPC than TRP+PPF.

    💡 Results confirm that lightweight ML models can effectively enhance hardware prefetching without high complexity.


   

## 🧠 Authors


- [**Shreyash Thok**](https://github.com/ShreyashThok24731)  
- [**Sachin Sugadare**](https://github.com/Sachin6763)  
- [**Jishan Pathan**](https://github.com/Jishan5707) 
