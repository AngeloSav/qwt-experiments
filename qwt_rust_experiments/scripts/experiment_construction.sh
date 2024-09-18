#!/bin/bash

for text in english/big_english sources/sources dna/dna commoncrawl/cc
do
	for size in 32KiB 64KiB 128KiB 256KiB 1MiB 4MiB 8MiB 16MiB 32MiB 64MiB 128MiB 256MiB 512MiB 1GiB 2GiB 4GiB 8GiB
	do
		echo "running exp for $text.$size"
		../target/release/perf_wavelet_tree_construction --input-file /data1/Texts/qwt_tests/${text}.${size}
	done
done
