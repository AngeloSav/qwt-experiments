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

