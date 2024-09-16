#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>
#include <thread>

#include <tlx/cmdline_parser.hpp>
#include <tlx/math.hpp>

#include <pasta/utils/benchmark/do_not_optimize.hpp>
#include <pasta/utils/reduce_alphabet.hpp>
#include <pasta/wavelet_tree/wavelet_tree.hpp>

#include <sdsl/int_vector.hpp>
// #include <sdsl/wm_int.hpp>
// #include <sdsl/construct.hpp>

#include "../include/wm_int.hpp"

#include <sdsl/wt_huff.hpp>
#include <sdsl/wavelet_trees.hpp>
#include <sdsl/construct.hpp>
#include <sdsl/util.hpp>

class Benchmark
{

private:
  std::vector<uint8_t> input_;

public:
  size_t alphabet_size_;
  size_t prefix_size = {0};
  std::string input_path = "";
  size_t runs = 5;

  void run()
  {
    load_text();
    reduce_alphabet();

    tlx::Aggregate<size_t> pasta_construction_time;
    for (size_t i = 0; i < runs; ++i)
    {
      std::vector<uint8_t> v_clone = input_;
      auto const start = std::chrono::steady_clock::now();
      auto pasta_wm = pasta::make_wm<pasta::BitVector>(v_clone.begin(), v_clone.end(),
                                                       alphabet_size_);
      pasta_construction_time.add(
          std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::steady_clock::now() - start)
              .count());
      // std::cout << "pasta_wm[0] " << pasta_wm[0] << '\n';
    }
    std::cout << "RESULT algo=pasta_construction"
              << " input=" << input_path
              << " n=" << input_.size()
              << " logn=" << tlx::integer_log2_ceil(input_.size())
              << " min_construction_time_ms=" << pasta_construction_time.min()
              << " max_construction_time_ms=" << pasta_construction_time.max()
              << " avg_construction_time_ms=" << pasta_construction_time.avg()
              << " n_runs=" << runs << std::endl;

    tlx::Aggregate<size_t> sdsl_construction_time;
    for (size_t i = 0; i < runs; ++i)
    {
      sdsl::int_vector<8> sdsl_input(input_.size(), 0);
      for (size_t j = 0; j < input_.size(); ++j)
      {
        sdsl_input[j] = input_[j];
      }
      auto const start = std::chrono::steady_clock::now();
      sdsl::wm_int sdsl_wm(sdsl_input, sdsl_input.size());

      sdsl_construction_time.add(
          std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::steady_clock::now() - start)
              .count());
      // std::cout << "sdsl_wm[0] " << sdsl_wm[0] << '\n';
    }
    std::cout << "RESULT algo=sdsl_construction"
              << " input=" << input_path
              << " n=" << input_.size()
              << " logn=" << tlx::integer_log2_ceil(input_.size())
              << " min_construction_time_ms=" << sdsl_construction_time.min()
              << " max_construction_time_ms=" << sdsl_construction_time.max()
              << " avg_construction_time_ms=" << sdsl_construction_time.avg()
              << " n_runs=" << runs << std::endl;

    tlx::Aggregate<size_t> sdsl_hwt_construction_time;
    for (size_t i = 0; i < runs; ++i)
    {
      std::string input_clone(input_.begin(), input_.end());
      auto const start = std::chrono::steady_clock::now();
      sdsl::wt_huff<> sdsl_hwt;
      construct_im(sdsl_hwt, input_clone, 1);

      sdsl_hwt_construction_time.add(
          std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::steady_clock::now() - start)
              .count());
      // std::cout << "sdsl_hwt[0] " << sdsl_hwt[0] << '\n';
    }
    std::cout << "RESULT algo=sdsl_hwt_construction"
              << " input=" << input_path
              << " n=" << input_.size()
              << " logn=" << tlx::integer_log2_ceil(input_.size())
              << " min_construction_time_ms=" << sdsl_hwt_construction_time.min()
              << " max_construction_time_ms=" << sdsl_hwt_construction_time.max()
              << " avg_construction_time_ms=" << sdsl_hwt_construction_time.avg()
              << " n_runs=" << runs << std::endl;
  }

private:
  void
  load_text()
  {
    // Read prefix of file
    std::ifstream stream(input_path.c_str(), std::ios::in | std::ios::binary);
    if (!stream)
    {
      std::cerr << "File " << input_path << " not found\n";
      exit(1);
    }
    stream.seekg(0, std::ios::end);
    uint64_t size = stream.tellg();
    if (prefix_size > 0)
    {
      size = std::min(prefix_size, size);
    }
    prefix_size = size;
    stream.seekg(0);
    input_.resize(size);
    stream.read(reinterpret_cast<char *>(input_.data()), size);
    stream.close();
  }

  void reduce_alphabet()
  {
    // Compute effective alphabet and effective alphabet size
    alphabet_size_ = pasta::reduce_alphabet(input_.begin(), input_.end());
  }
}; // class Benchmark

int32_t main(int argc, char *argv[])
{
  tlx::CmdlineParser cp;

  cp.set_description("Simple Wavelet Tree/Wavelet Matrix Benchmark");

  Benchmark bench;

  cp.add_param_string("input", bench.input_path, "Path to input file.");
  cp.add_bytes('n', "size", bench.prefix_size,
               "Size (in bytes unless stated "
               "otherwise) of the string that use to test our suffix array "
               "construction algorithms.");
  cp.add_bytes('r', "runs", bench.runs, "Number of runs the benchmark is executed.");

  if (!cp.process(argc, argv))
  {
    return -1;
  }

  bench.run();

  return 0;
}

/******************************************************************************/
