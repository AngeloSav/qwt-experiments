#!/bin/bash

echo "starting RSWide bench (used in WT and HWT)"
../target/release/perf_rs_wide

echo "starting RSQVector bench (used in all QWT and HQWT versions)"
../target/release/perf_rs_qvector