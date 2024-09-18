#!/bin/sh

echo "Starting DNA bench ..."
python3 run_experiment.py --machine_name xor --result_folder /home/angelosav/results /data1/Texts/qwt_tests/dna dna --test_prefetch

echo "Starting big_english bench ..."
python3 run_experiment.py --machine_name xor --result_folder /home/angelosav/results /data1/Texts/qwt_tests/english big_english --test_prefetch

echo "Starting wiki bench ..."
python3 run_experiment.py --machine_name xor --result_folder /home/angelosav/results /data1/Texts/qwt_tests/wiki_small wiki --test_prefetch

echo "Starting sources_new bench ..."
python3 run_experiment.py --machine_name xor --result_folder /home/angelosav/results /data1/Texts/qwt_tests/sources sources --test_prefetch

echo "Starting commoncrawl bench ..."
python3 run_experiment.py --machine_name xor --result_folder /home/angelosav/results /data1/Texts/qwt_tests/commoncrawl/ cc --test_prefetch
