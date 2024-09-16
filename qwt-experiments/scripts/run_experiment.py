import os, argparse, sys, os.path, math, subprocess
from tabulate import tabulate

Description = """The script runs an experiment of qwt on a given input file. The experiment creates prefixes of the input file of cetain sizes and runs rank, select, and access queires on all those prefixes.

Use https://www.spec.org/ to get specs of your machine.
"""

rename = [
        ("&qwt::quadwt::QWaveletTree<u8,qwt::qvector::rs_qvector::RSQVector<qwt::qvector::rs_qvector::rs_support_plain::RSSupportPlain>>", "QWT256"), #it does not have the space in the type name
        ("&qwt::quadwt::QWaveletTree<u8,qwt::qvector::rs_qvector::RSQVector<qwt::qvector::rs_qvector::rs_support_plain::RSSupportPlain>,true>", "QWT256Pfs"),
        ("&qwt::quadwt::QWaveletTree<u8,qwt::qvector::rs_qvector::RSQVector<qwt::qvector::rs_qvector::rs_support_plain::RSSupportPlain<512>>>", "QWT512"),
        ("&qwt::quadwt::QWaveletTree<u8,qwt::qvector::rs_qvector::RSQVector<qwt::qvector::rs_qvector::rs_support_plain::RSSupportPlain<512>>,true>", "QWT512Pfs"),
        ("&qwt::binwt::WaveletTree<u8,qwt::bitvector::rs_wide::RSWide,true>", "HWT"),
        ("&qwt::binwt::WaveletTree<u8,qwt::bitvector::rs_wide::RSWide>", "WT"),
        ("&qwt::quadwt::huffqwt::HuffQWaveletTree<u8,qwt::qvector::rs_qvector::RSQVector<qwt::qvector::rs_qvector::rs_support_plain::RSSupportPlain>>", "HQWT256"),
        ("&qwt::quadwt::huffqwt::HuffQWaveletTree<u8,qwt::qvector::rs_qvector::RSQVector<qwt::qvector::rs_qvector::rs_support_plain::RSSupportPlain<512>>>", "HQWT512"),
        ("&qwt::quadwt::huffqwt::HuffQWaveletTree<u8,qwt::qvector::rs_qvector::RSQVector<qwt::qvector::rs_qvector::rs_support_plain::RSSupportPlain>,true>", "HQWT256Pfs"),
        ("&qwt::quadwt::huffqwt::HuffQWaveletTree<u8,qwt::qvector::rs_qvector::RSQVector<qwt::qvector::rs_qvector::rs_support_plain::RSSupportPlain<512>>,true>", "HQWT512Pfs"),
        ]

sizes_exts = [  (32*1024, "32KiB"),
                (64*1024, "64KiB"), # added
                (128*1024, "128KiB"),#
                (256*1024, "256KiB"),
                (1*1024**2, "1MiB"), #
                (4*1024**2, "4MiB"), #
                (8*1024**2, "8MiB"), #
                (16*1024**2, "16MiB"),
                (32*1024**2, "32MiB"),
                (64*1024**2, "64MiB"),
                (128*1024**2, "128MiB"),
                (256*1024**2, "256MiB"),
                (512*1024**2, "512MiB"),
                (1*1024**3, "1GiB"),
                (2*1024**3, "2GiB"),
                (4*1024**3, "4GiB"),
                (8*1024**3, "8GiB"),
                ]

machine_specs = {}

machine_specs["xor"] = """Intel(R) Core(TM) i9-9900KF CPU @ 3.60GHz
Microarchitecture: Coffee Lake
CPU Name:	Intel Core i9-9900K
  Max MHz.:	5000
  Nominal:	3600
Enabled:	8 cores, 1 chip, 2 threads/core
Orderable:	1 chip
Cache L1:	32 KB I + 32 KB D on chip per core
L2:	256 KB I+D on chip per core
L3:	16 MB I+D on chip per chip"""

machine_specs["sencha"] = """
Intel(R) Xeon(R) Gold 5318Y CPU @ 2.10GHz
Microarchitecture: Ice Lake-SP
CPU Name:	Intel Xeon Gold 5318Y
  Max MHz:	3400
  Nominal:	2100
Enabled:	48 cores, 2 chips, 2 threads/core
Orderable:	1,2 chips
Cache L1:	32 KB I + 48 KB D on chip per core
  L2:	1.25 MB I+D on chip per core
  L3:	36 MB I+D on chip per chip
"""

machine_specs["tokaji"] = """
- Intel(R) Xeon(R) Silver 4314 CPU @ 2.40GHz
- Microarchitecture: Ice Lake-SP
CPU Name:	Intel Xeon Silver 4314
  Max MHz:	3400
  Nominal:	2400
Enabled:	32 cores, 2 chips, 2 threads/core
Orderable:	1,2 chips
Cache L1:	32 KB I + 48 KB D on chip per core
  L2:	1.25 MB I+D on chip per core
  L3:	24 MB I+D on chip per chip
"""

machine_specs["i10pc128"] = """
- Intel(R) Xeon(R) E5-4640 @ 2.4GHz
- Microarchitecture: Sandy Bridge
CPU Name:       Intel Xeon E5-4640
  Max MHz:      2800
  Nominal:      2400
Enabled:        8 cores, 4 chip, 2 threads/core
Orderable:      1,2,3,4 chips
Cache L1:       32 KB I + 32 KB D on chip per core
      L2:   256 KB I+D on chip per core
      L3:   20 MB I+D on chip per chip
"""

machine_specs["i10pc138"] = """
- AMD EPYC 7713 64-Core Processor
- Microarchitecture: Zen 3
CPU Name:       AMD EPYC 7713
  Max MHz:      3560
  Nominal:      2000
Enabled:        64 cores, 2 chip, 2 threads/core
Orderable:      1,2 chips
Cache L1:       64 KB I + 64 KB D on chip per core
      L2:   512 KB I+D on chip per core
      L3:   256 MB I+D on chip per chip (32 MB per 8 cores)
"""

machine_specs["i10pc139"] = """
- Intel(R) Xeon(R) Gold 6314U CPU @ 2.30GHz
- Microarchitecture: Whitley
CPU Name:       Intel Xeon Gold 6314U
  Max MHz:      3400
  Nominal:      2300
Enabled:        32 cores, 1 chip, 2 threads/core
Orderable:      1 chips
Cache L1:       32 KB I + 48 KB D on chip per core
  L2:   1.25 KB I+D on chip per core
  L3:   48 MB I+D on chip per chip
"""

def repeat_to_length(s, wanted):
    a, b = divmod(wanted, len(s))
    return s * a + s[:b]

def clean_result(result):
    for old_name, name in rename:
        result = result.replace(old_name, name)

    res = {}
    for line in result.split("\n"):
        if line.startswith("RESULT"):
            data = line[1:-1].split(" ")
            for pair in data:
                pair = pair.strip()
                if pair.startswith("algo="):
                    ds_name = pair.split("=")[-1]
                if pair.startswith("exp="):
                    exp = pair.split("=")[-1]
                if pair.startswith("avg_time_ns="):
                    time = pair.split("=")[-1]
                if pair.startswith("space_in_bytes="):
                    space = pair.split("=")[-1]
            if ds_name not in res:
                d = {exp:time, "space":space}
                res[ds_name] = d
            else:
                d = res[ds_name]
                d[exp] = time

    return res

def exp_qwt_prefetch(folder, files, filename, machine_name, alphabet, result_folder):
    command = "../target/release/perf_huff_wavelet_tree"

    output_filename_csv = f"{filename}.{machine_name}.prefetch.results.out"
    fo = open(os.path.join(result_folder, output_filename_csv), "w")

    ## Run experiments and collect results
    for file in files:
        cur_filename = os.path.join(folder, file)
        print(f"Benchmarking file: {cur_filename}")
        result = subprocess.run([command, 
                                "--input-file", str(cur_filename), 
                                "--rank", 
                                "--select",
                                "--access", 
                                "--all-structs",
                                "--n-queries", "1000000"], 
                                check=True, 
                                stdout=subprocess.PIPE, 
                                universal_newlines=True).stdout
        

        # print(result) 
        fo.write(result)
        files[file]["result"] = clean_result(result)

    fo.close()

    # Build the table of results
    rows = []
    for file in files:
        data = files[file]["result"]
        # print(data)

        for k in data.keys():   
            ds = data[k]
            if "Pfs" not in k:
                rows.append([file, k, ds['rank_latency'], ds['access_latency'], ds['select_latency'], ds['space']])
            else:
                rows.append([file, k, ds['rank_prefetch_latency'], ds['access_latency'], ds['select_latency'], ds['space']])

        # ds = data['WT']
        # rows.append([file, "WT", ds['rank_latency'], ds['access_latency'], ds['select_latency'], ds['space']])

        # ds = data['HWT']
        # rows.append([file, "HWT", ds['rank_latency'], ds['access_latency'], ds['select_latency'], ds['space']])

        # ds = data['HQWT256']
        # rows.append([file, "HQWT256", ds['rank_latency'], ds['access_latency'], ds['select_latency'], ds['space']])

        # ds = data['QWT256']
        # rows.append([file, "QWT256", ds['rank_latency'], ds['access_latency'], ds['select_latency'], ds['space']])

#    table = tabulate(rows, headers=['File Name', 'Method', 'Rank (ns)', 'Get (ns)', 'Select (ns)', 'Space (bytes)'], tablefmt='orgtbl')

#    result_filename = f"{filename}.{machine_name}.prefetch.results"
#    f = open(os.path.join(result_folder, result_filename), "w")
#    print(filename)
#    f.write(machine_specs[machine_name]+"\n\n")
#    f.write(f"Alphabet size: {alphabet}\n\n\n")
#    f.write(table)
#    f.close()

    csv_str = 'File Name,Method,Rank (ns),Get (ns),Select (ns),Space (bytes)\n'
    for r in rows:
        csv_str += ",".join(r)
        csv_str += "\n"

    result_filename_csv = f"{filename}.{machine_name}.prefetch.results.csv"
    f = open(os.path.join(result_folder, result_filename_csv), "w")
    f.write(csv_str)
    f.close()

    print("----------------START CSV----------------")
    print(csv_str)
    print("-----------------END CSV-----------------")

#    print(table)

def main():  
    parser = argparse.ArgumentParser(description=Description, formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('input_folder', help='Input folder', type=str)
    parser.add_argument('input_filename', help='Input filename', type=str)
    parser.add_argument('--machine_name', help='Name of the machine you are using', default="unknown", type=str)
    parser.add_argument('--result_folder', help='Folder that contains results files', default="../experiments")
    parser.add_argument('--test_prefetch', help='Run experiments to compare QWT versions with or without prefetching.', action='store_true')
    args = parser.parse_args()

    folder = args.input_folder
    filename = args.input_filename
    machine_name = args.machine_name
    result_folder = args.result_folder

    if machine_name not in machine_specs:
        machine_specs[machine_name] = ""
    
    text = open(os.path.join(folder, filename), "rb").read()

    alphabet = len(set(text))
    bit_symbol = int(math.log2(alphabet-1) + 1)
    print(f"Alphabet size: {alphabet}")
    print(f"Bit per symbol: {bit_symbol}")

    bit_symbol = 8

    ## Create prefixes of the input file
    files = {}
    for size, ext in sizes_exts:
        cur_filename = filename + "." + ext
        files[cur_filename] = {"ext":ext, "size":size}
        print(f"Creating {cur_filename}")

        if os.path.isfile(os.path.join(folder,cur_filename)):
            continue
        req_size = (size * 8) // bit_symbol
        print(f"Size in symbols: {req_size}")
        if len(text) > req_size:
            t = text[:req_size]
        else:
            t = repeat_to_length(text, req_size)
        f = open(os.path.join(folder,cur_filename), "wb")
        f.write(t)
        f.close()

    
    if args.test_prefetch:
        # Perform a comparison between QWT versions with or without prefetching
        exp_qwt_prefetch(folder, files, filename, machine_name, alphabet, result_folder)

if __name__ == '__main__':
    main()
