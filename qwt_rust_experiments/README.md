# QWT: Rust Quad Wavelet Tree

## Compiling

```
RUSTFLAGS="-C target-cpu=native" cargo build --release
```

## Experiments

All experiments have been run using the `scripts/run_perf.sh` command.

For instance, the latency performance has been obtained by running from the `scripts` directory: 

```
./run_perf.sh ./experiment.sh vesuvio noturbo
```

on our machine, while the bitvector performance benchmarks has been run, from the `scripts` directory, using

```
./run_perf.sh ../target/release/perf_rs_qvector vesuvio noturbo
```

## Scripts

All the scripts can be found in the `scripts/` folder. The scripts are the following:

- `experiment_latency.sh` can be used to run the latency experiments
- `experiment_bv.sh` can be used to run the benchmarks on our bitvector and quad vector
- `experiment_construction.sh` can be used to run the benchmarks on our wavelet trees construction time

All the scripts have been run using `run_perf.sh` for better stability