#!/bin/sh

for text in big_english enwiki-20230601-pages-articles-multistream0108.xml-p1-p2936260 dna.txt cc.txt.0
do
	for size in 16KiB 32KiB 64KiB 128KiB 256KiB 512KiB 1MiB 2MiB 4MiB 8MiB 16MiB 32MiB 64MiB 128MiB 256MiB 512MiB 1GiB 2GiB 4GiB 8GiB
	do
		string="./../build/text_statistics /workspace/<anonymized>/shared_data_sets/${text}.${size}"
		./runPerf.sh "${string}" i10pc138 noturbo >> ../experiments/ALENEX_text_statistics.txt
	done
done
