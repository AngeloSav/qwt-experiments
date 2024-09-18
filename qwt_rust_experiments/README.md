# QWT: Rust Quad Wavelet Tree

## Compiling

```
RUSTFLAGS="-C target-cpu=native" cargo build --release
```

The directory `./target/release/` will contain the compiled binaries

## Binaries
These are the binaries used to obtain the results:

1. `perf_huff_wavelet_tree` generates the results for the latency plots.
    Requires a text as input file.
2. `perf_wavelet_tree_construction` generates the results for the construction time plot.
    Requires a text as input file.
3. `perf_rs_wide` generates data related to our bitvector implementation.
    Requires a text as input.
4. `perf_rs_qvector` generates data related to our quad vector implementation.
    Requires a text as input.


## Scripts

All the scripts can be found in the `scripts/` folder. The scripts are the following:

- `experiment_latency.sh` can be used to run the latency experiments
- `experiment_bv.sh` can be used to run the benchmarks on our bitvector and quad vector
- `experiment_construction.sh` can be used to run the benchmarks on our wavelet trees construction time

All the scripts have been run using `run_perf.sh` for better stability.

> [!NOTE]
> Please run experiment_latency.sh first as it also creates all the text prefixes that will be useed in testing
