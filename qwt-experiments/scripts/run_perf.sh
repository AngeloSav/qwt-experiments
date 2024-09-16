#!/bin/bash
#
# runPerf.sh, v1.1
#
# from: https://gist.github.com/grahamking/9c8c91b871843a9a6ce2bec428b8f48d
# 1) extended with NUMA support by Franco Maria Nardini (francomaria.nardini@isti.cnr.it)
# 2) it can be used on non-NUMA architectures, numactl masks this part (but still allow to bind physical CPUs)
#
# Usage: runPerf.sh ./my-benchmark-binary [tokaji|vesuvio] [turbo|noturbo]
#
# Script to run a benchmark / performance test in decent conditions. Based on:
# - https://www.llvm.org/docs/Benchmarking.html
# - "Performance Analysis and Tuning on Modern CPU" by Denis Bakhvalov, Appendix A.
# - https://github.com/andikleen/pmu-tools
#
# Note that this doesn't do any actual benchmarking, your binary must be able to do that all by itself.
# Instead, this sets up the machine to make your benchmarks reliable.
#
# Usage with rust/cargo criterion (https://github.com/bheisler/cargo-criterion):
#  Build the binary: `cargo criterion --bench my-bench --no-run`
#  Run it: `runPerf ./target/release/deps/my-bench-<hex-string> --bench`


# ---
# Phase 0: setup environment variables...
#

if [ $# -ne 3 ]; then
	>&2 echo "*** ERROR: no arguments provided!"
	>&2 echo "*** Usage: runPerf.sh './my-benchmark-binary' [tokaji|vesuvio] [turbo|noturbo]"
	exit 1
fi

COMMAND_TO_RUN=$1
MACHINE=$2
TURBO=$3

# check what are the physical available cores, not hyperthreaded
# cat /sys/devices/system/cpu/cpu0/topology/thread_siblings_list

if [[ "${MACHINE}" =~ ^(tokaji|i10pc139|i10pc128)$ ]]; then
	# on tokaji.isti.cnr.it, physical CPUs are the first 32 ones...
	PHYSICAL_CPU_IDS="0-31"
elif [[ "${MACHINE}" =~ ^(vesuvio)$ ]]; then
	# on vesuvio.isti.cnr.it and i10pc133.iti.kit.edu, physical CPUs are the first 16 ones...
	PHYSICAL_CPU_IDS="0-15"
elif [[ "${MACHINE}" =~ ^(i10pc138)$ ]]; then
	PHYSICAL_CPU_IDS="0-64"
else
	>&2 echo "*** ERROR: wrong machine provided!"
	>exit 1
fi

# ---
# Phase 1: setup of the environment...
#

echo "*** Setting up the environment..."
echo "*** WARNING: this script implement NUMA support, please consider checking your NUMA architecture before using it!"

# (optional): mount input / output files in ramdisk to eliminate disk access variability
# mount -t tmpfs -o size=<XX>g none dir_to_mount

#echo "Disable address space randomization"
#echo 0 | sudo tee /proc/sys/kernel/randomize_va_space > /dev/null

#echo "Disable kernel NMI watchdog"
#sudo sysctl -q -w kernel.nmi_watchdog=0

echo "*** Allow access to perf events"
# Sometimes we run as ./runPerf perf stat <the-binary>
echo -1 | sudo tee /proc/sys/kernel/perf_event_paranoid > /dev/null

echo "*** Setting the CPU governor to 'performance'"
sudo cpupower frequency-set -g performance > /dev/null
# If you don't have `cpupower`:
# for i in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
# do
#   echo performance > /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
# done

if [ "${TURBO}" = "noturbo" ]; then
	echo "*** Disable turbo mode (short-term higher freq)"
	if lscpu | grep -q AMD; then
		echo 0 | sudo tee /sys/devices/system/cpu/cpufreq/boost > /dev/null
	else
		if [[ "${MACHINE}" =~ ^(i10pc133)$ ]]; then
			echo 0 | sudo tee /sys/devices/system/cpu/cpufreq/boost > /dev/null
		else
			echo 1 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo > /dev/null
		fi
	fi
else
	if [ "${TURBO}" = "turbo" ]; then
	# just to be sure turbo is set in the right way...
		if lscpu | grep -q AMD; then
			echo 1 | sudo tee /sys/devices/system/cpu/cpufreq/boost > /dev/null
	        else
	                echo 0 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo > /dev/null
	        fi
	fi
fi

# (removed by FM): this is handled by numactl explicitly
#echo "Move system processes to second two cores (and their hyper-threaded pairs)"
# Adjust this if your topology is different. I have four cores / eight threads.
#
# This only moves system stuff not user stuff, so your terminal/browser/Gnome are all still using the shared CPU.
# Ideally we would also do the same for `user.slice`, but then `taskset` later can't use the reserved CPUs.
# To mitigate we use `nice` to make our job higher priority.
#sudo systemctl set-property --runtime system.slice AllowedCPUs=2,3,6,7

# (removed by FM): this is handled by numactl explicitly
#echo "Disable the hyper-threading pair of the reserved CPUs"
# Find the pair: cat /sys/devices/system/cpu/cpuN/topology/thread_siblings_list
#echo 0 | sudo tee /sys/devices/system/cpu/cpu4/online > /dev/null
#echo 0 | sudo tee /sys/devices/system/cpu/cpu5/online > /dev/null


# (removed by FM): this is handled by numactl explicitly
# Run the script
#  . on our reserved CPUs so it doesn't migrate
#  . re-niced so it doesn't get context switched
#

#taskset -c 0,1 sudo nice -n -5 runuser -u $USERNAME -- $@

# (removed by FM): this is handled by numactl explicitly
# Monitor cpu-migrations and context-switches. They should both be 0. perf must come before nice to make the benchmark higher priority.
#taskset -c 0,1 perf stat -e context-switches,cpu-migrations sudo nice -n -5 $@


# ---
# Phase 2: run whatever you need to run...
#

echo ${PHYSICAL_CPU_IDS}

echo "*** Now running: "${COMMAND_TO_RUN}"..."
echo "---"
numactl --physcpubind=${PHYSICAL_CPU_IDS} --localalloc ${COMMAND_TO_RUN}
echo "---"

# ---
# Phase 3: restore previous settings...
#

echo "*** Restoring previous settings"

#echo 1 | sudo tee /sys/devices/system/cpu/cpu4/online > /dev/null
#echo 1 | sudo tee /sys/devices/system/cpu/cpu5/online > /dev/null

#sudo systemctl set-property --runtime system.slice AllowedCPUs=0-7

if [ "${TURBO}" = "noturbo" ]; then
	if lscpu | grep -q AMD; then
		echo 1 | sudo tee /sys/devices/system/cpu/cpufreq/boost > /dev/null
	else
		if [[ "${MACHINE}" =~ ^(i10pc133)$ ]]; then
			echo 1 | sudo tee /sys/devices/system/cpu/cpufreq/boost > /dev/null
	        else
	                echo 0 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo > /dev/null
	        fi
	fi
fi

#sudo cpupower frequency-set -g performance > /dev/null

echo 0 | sudo tee /proc/sys/kernel/perf_event_paranoid > /dev/null

#sudo sudo sysctl -q -w kernel.nmi_watchdog=1

#echo 2 | sudo tee /proc/sys/kernel/randomize_va_space > /dev/null

echo "*** DONE!"

exit 0