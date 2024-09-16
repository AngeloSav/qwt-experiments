#!/bin/sh

string="./../build/bit_vector_benchmark -b ${size}"
./runPerf.sh "${string}" i10pc138 noturbo >> ../experiments/ALENEX_BVec_query_results.txt
