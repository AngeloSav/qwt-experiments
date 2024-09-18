#!/bin/sh

for x in {15..33}; do 
    echo "running test logn=$x" 
    ./../build/bit_vector_benchmark -b $((2**x)); 
done
