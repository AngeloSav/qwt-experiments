# How to Use

This project contains the C++ code for our paper "Faster Wavelet Tree Queries".
Note that this project depends on the [SDSL](https://github.com/simongog/sdsl-lite/), which should be installed on the system.
To build the project, simply run

```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j
```

This will build the project.
Then you have four binaries:

1. `bit_vector_benchmark` generates the results for pasta_bv and sdsl_bv in Figure 5.
3. `wavelet_tree_benchmark` generates the results for pasta_wm and sdsl_wm in Figure 3, 5, and 6.
   Requires a text as input file.
4. `wavelet_tree_construction_benchmark` generates the results for pasta_wm and sdsl_wm in Figure 4.
   Requires a text as input.


## Scripts

All the scripts can be found in the `scripts/` folder. The scripts are the following:

- `experiment_latency.sh` can be used to run the latency experiments
- `experiment_bv.sh` can be used to run the benchmarks on our bitvector and quad vector
- `experiment_construction.sh` can be used to run the benchmarks on our wavelet trees construction time


All scripts used for the evaluation are available in the `scripts` folder.
All the scripts have been run usins `run_perf.sh` for better stability