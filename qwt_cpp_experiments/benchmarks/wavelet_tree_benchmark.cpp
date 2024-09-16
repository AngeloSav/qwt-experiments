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
// #include <sdsl/wt_huff.hpp>
// #include <sdsl/construct.hpp>

#include "../include/wm_int.hpp"

#include <sdsl/wt_huff.hpp>
#include <sdsl/wavelet_trees.hpp>
#include <sdsl/construct.hpp>
#include <sdsl/util.hpp>


class Benchmark {

private:
  std::vector<uint8_t> input_;
  size_t alphabet_size_;
  size_t number_queries_;

public:

  size_t prefix_size = {0};
  std::string input_path = "";
  size_t number_queries = 10'000'000;
  size_t runs = 10;

  void run() {
    load_text();
    reduce_alphabet();

    auto const access_queries = generate_queries(number_queries, input_);
    auto const rank_queries = generate_rank_queries(number_queries, input_);
    auto const select_queries = generate_select_queries(number_queries, input_);

    std::cout << "rank_queries.size() " << rank_queries.size() << '\n';
    
    auto pasta_wm = pasta::make_wm<pasta::BitVector>(input_.begin(), input_.end(),
                                                     alphabet_size_);

    run_experiments_latency(pasta_wm, access_queries, rank_queries, select_queries,
			    "pasta_wm", pasta_wm.space_usage());
    // run_experiments_throughput(pasta_wm, access_queries, rank_queries, select_queries,
		// 	       "pasta_wm", pasta_wm.space_usage());


    sdsl::int_vector<8> sdsl_input(input_.size(), 0);
    for (size_t i = 0; i < input_.size(); ++i) {
      sdsl_input[i] = input_[i];
    }
    
    sdsl::wm_int sdsl_wm(sdsl_input, sdsl_input.size());

    run_experiments_latency(sdsl_wm, access_queries, rank_queries, select_queries,
		    "sdsl_wm", sdsl::size_in_bytes(sdsl_wm));
    // run_experiments_throughput(sdsl_wm, access_queries, rank_queries, select_queries,
		// 	    "sdsl_wm", sdsl::size_in_bytes(sdsl_wm));

    std::string input_clone(input_.begin(), input_.end());
    sdsl::wt_huff<> sdsl_hwt;
    construct_im(sdsl_hwt, input_clone, 1);

    run_experiments_latency(sdsl_hwt, access_queries, rank_queries, select_queries,
		    "sdsl_huffwt", sdsl::size_in_bytes(sdsl_hwt));
    // run_experiments_throughput(sdsl_hwt, access_queries, rank_queries, select_queries,
		// 	    "sdsl_huffwt", sdsl::size_in_bytes(sdsl_hwt));

    sdsl::wt_huff<sdsl::rrr_vector<>> sdsl_hwt2;
    construct_im(sdsl_hwt2, input_clone, 1);

    run_experiments_latency(sdsl_hwt2, access_queries, rank_queries, select_queries,
		    "sdsl_huffwt_rrr", sdsl::size_in_bytes(sdsl_hwt2));
  }
    
private:

  void load_text() {
    // Read prefix of file
    std::ifstream stream(input_path.c_str(), std::ios::in | std::ios::binary);
    if (!stream) {
      std::cerr << "File " << input_path << " not found\n";
      exit(1);
    }
    stream.seekg(0, std::ios::end);
    uint64_t size = stream.tellg();
    if (prefix_size > 0) {
      size = std::min(prefix_size, size);
    }
    prefix_size = size;
    stream.seekg(0);
    input_.resize(size);
    stream.read(reinterpret_cast<char *>(input_.data()), size);
    stream.close();
  }

  void reduce_alphabet() {
    // Compute effective alphabet and effective alphabet size
    alphabet_size_ = pasta::reduce_alphabet(input_.begin(), input_.end());
  }

  std::vector<size_t>  generate_queries(size_t const number_queries,
                                        std::vector<uint8_t> const& input) {
    std::random_device rnd_device;
    std::mt19937 mersenne_engine(rnd_device());
    std::uniform_int_distribution<uint64_t> dist(0, input.size());

    std::vector<size_t> random_queries(number_queries);
    for (size_t i = 0; i < number_queries; ++i) {
      random_queries[i] = dist(mersenne_engine);
    }
    return random_queries;
  }

  std::vector<std::pair<size_t, uint8_t>> generate_rank_queries(size_t const number_queries,
                                                                std::vector<uint8_t> const& input) {
    std::random_device rnd_device;
    std::mt19937 mersenne_engine(rnd_device());
    std::uniform_int_distribution<uint64_t> dist(0, input.size());

    std::vector<std::pair<size_t, uint8_t>> random_queries(number_queries);
    for (size_t i = 0; i < number_queries; ++i) {
      random_queries[i] = std::make_pair(dist(mersenne_engine), input[dist(mersenne_engine)]);
    }
    return random_queries;
  }

  std::vector<std::pair<size_t, uint8_t>> generate_select_queries(size_t const number_queries,
                                                                  std::vector<uint8_t> const& input) {
    pasta::Histogram hist(input.begin(), input.end());

    std::random_device rnd_device;
    std::mt19937 mersenne_engine(rnd_device());
    std::uniform_int_distribution<uint64_t> dist(0, input.size());

    std::vector<std::pair<size_t, uint8_t>> random_queries(number_queries);
    for (size_t i = 0; i < number_queries; ++i) {
      uint8_t const random_char = input[dist(mersenne_engine)];

      std::uniform_int_distribution<uint64_t> dist_hist(1, hist[random_char]);
      random_queries[i] = std::make_pair(dist_hist(mersenne_engine), random_char);
    }
    return random_queries;
  }

  template <typename WaveletMatrix, typename AccessQueries,
	    typename RankQueries, typename SelectQueries>
  void run_experiments_latency(WaveletMatrix& wm, AccessQueries& access_queries,
			       RankQueries& rank_queries, SelectQueries& select_queries,
			       std::string name, size_t space) {

    tlx::Aggregate<size_t> time_access;
    tlx::Aggregate<size_t> time_rank;
    tlx::Aggregate<size_t> time_select;
    for (size_t i = 0; i < runs; ++i) {
      {
        size_t result = 0;
        auto const start = std::chrono::steady_clock::now();
        for (size_t i = 0; i < access_queries.size(); ++i) {
          size_t const pos = (access_queries[i] * (result + 42)) % prefix_size;
          result = wm[pos];
        }

        time_access.add(
          std::chrono::duration_cast<std::chrono::nanoseconds>(
                       std::chrono::steady_clock::now() - start)
          .count());
	std::cout << "result " << result << '\n';
      }

      {
        size_t result = 0;
        auto const start = std::chrono::steady_clock::now();
        for (size_t i = 0; i < rank_queries.size(); ++i) {
          size_t const pos = (rank_queries[i].first + result) % prefix_size;
          result = wm.rank(pos, rank_queries[i].second);
        }        
        time_rank.add(
          std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                               std::chrono::steady_clock::now() - start)
          .count());
	std::cout << "result " << result << '\n';
      }

      
      {
        size_t result = 0;
        auto const start = std::chrono::steady_clock::now();
        for (size_t i = 0; i < rank_queries.size(); ++i) {
          size_t pos = (select_queries[i].first - 1 + (result % 2));
          pos = std::max(size_t{1}, pos);
          result = wm.select(pos, select_queries[i].second);
        }
        
        time_select.add(
          std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                               std::chrono::steady_clock::now() - start)
          .count());
	std::cout << "result " << result << '\n';
      }
      
    }

    std::cout << "RESULT algo=" << name
	      << " exp=" << "access_latency"
	      << " input=" << input_path
	      << " n=" << input_.size()
	      << " logn=" << tlx::integer_log2_ceil(input_.size())
	      << " min_time_ns=" << time_access.min() / access_queries.size()
	      << " max_time_ns=" << time_access.max() / access_queries.size()
	      << " avg_time_ns=" << time_access.avg() / access_queries.size()
	      << " space_in_bytes=" << space
	      << " space_in_mib=" << (space / 1024.0 / 1024.0)
	      << " n_queries=" << access_queries.size()
	      << " n_runs=" << runs << std::endl;

    std::cout << "RESULT algo=" << name
	      << " exp=" << "rank_latency"
	      << " input=" << input_path
	      << " n=" << input_.size()
	      << " logn=" << tlx::integer_log2_ceil(input_.size())
	      << " min_time_ns=" << time_rank.min() / rank_queries.size()
	      << " max_time_ns=" << time_rank.max() / rank_queries.size()
	      << " avg_time_ns=" << time_rank.avg() / rank_queries.size()
	      << " space_in_bytes=" << space
	      << " space_in_mib=" << (space / 1024.0 / 1024.0)
	      << " n_queries=" << rank_queries.size()
	      << " n_runs=" << runs << std::endl;;

    std::cout << "RESULT algo=" << name
	      << " exp=" << "select_latency"
	      << " input=" << input_path
	      << " n=" << input_.size()
	      << " logn=" << tlx::integer_log2_ceil(input_.size())
	      << " min_time_ns=" << time_select.min() / select_queries.size()
	      << " max_time_ns=" << time_select.max() / select_queries.size()
	      << " avg_time_ns=" << time_select.avg() / select_queries.size()
	      << " space_in_bytes=" << space
	      << " space_in_mib=" << (space / 1024.0 / 1024.0)
	      << " n_queries=" << select_queries.size()
	      << " n_runs=" << runs << std::endl;
  }

  template <typename WaveletMatrix, typename AccessQueries,
	    typename RankQueries, typename SelectQueries>
  void run_experiments_throughput(WaveletMatrix& wm, AccessQueries& access_queries,
				  RankQueries& rank_queries, SelectQueries& select_queries,
				  std::string name, size_t space) {

    tlx::Aggregate<size_t> time_access;
    tlx::Aggregate<size_t> time_rank;
    tlx::Aggregate<size_t> time_select;
    for (size_t i = 0; i < runs; ++i) {
      {
        size_t result = 0;
        auto const start = std::chrono::steady_clock::now();
        for (size_t i = 0; i < access_queries.size(); ++i) {
          result += wm[access_queries[i]];
        }

        time_access.add(
          std::chrono::duration_cast<std::chrono::nanoseconds>(
                       std::chrono::steady_clock::now() - start)
          .count());
	std::cout << "result " << result << '\n';
      }

      {
        size_t result = 0;
        auto const start = std::chrono::steady_clock::now();
        for (size_t i = 0; i < rank_queries.size(); ++i) {
          result += wm.rank(rank_queries[i].first, rank_queries[i].second);
        }        
        time_rank.add(
          std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                               std::chrono::steady_clock::now() - start)
          .count());
	std::cout << "result " << result << '\n';
      }

      
      {
        size_t result = 0;
        auto const start = std::chrono::steady_clock::now();
        for (size_t i = 0; i < rank_queries.size(); ++i) {
          result += wm.select(select_queries[i].first, select_queries[i].second);
        }
        
        time_select.add(
          std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                               std::chrono::steady_clock::now() - start)
          .count());
	std::cout << "result " << result << '\n';
      }
      
    }

    std::cout << "RESULT algo=" << name
	      << " exp=" << "access_throughput"
	      << " input=" << input_path
	      << " n=" << input_.size()
	      << " logn=" << tlx::integer_log2_ceil(input_.size())
	      << " min_throughput_ms=" << access_queries.size() / (time_access.max() / 1000.0 / 1000.0)
	      << " max_throughput_ms=" << access_queries.size() / (time_access.min() / 1000.0 / 1000.0)
	      << " avg_throughput_ms=" << access_queries.size() / (time_access.avg() / 1000.0 / 1000.0)
	      << " space_in_bytes=" << space
	      << " space_in_mib=" << (space / 1024.0 / 1024.0)
	      << " n_queries=" << access_queries.size()
	      << " n_runs=" << runs << std::endl;

    std::cout << "RESULT algo=" << name
	      << " exp=" << "rank_throughput"
	      << " input=" << input_path
	      << " n=" << input_.size()
	      << " logn=" << tlx::integer_log2_ceil(input_.size())
	      << " min_throughput_ms=" << rank_queries.size() / (time_rank.max() / 1000.0 / 1000.0)
	      << " max_throughput_ms=" << rank_queries.size() / (time_rank.min() / 1000.0 / 1000.0)
	      << " avg_throughput_ms=" << rank_queries.size() / (time_rank.avg() / 1000.0 / 1000.0)
	      << " space_in_bytes=" << space
	      << " space_in_mib=" << (space / 1024.0 / 1024.0)
	      << " n_queries=" << rank_queries.size()
	      << " n_runs=" << runs << std::endl;;

    std::cout << "RESULT algo=" << name
	      << " exp=" << "select_throughput"
	      << " input=" << input_path
	      << " n=" << input_.size()
	      << " logn=" << tlx::integer_log2_ceil(input_.size())
	      << " min_throughput_ms=" << select_queries.size() / (time_select.max() / 1000.0 / 1000.0)
	      << " max_throughput_ms=" << select_queries.size() / (time_select.min() / 1000.0 / 1000.0)
	      << " avg_throughput_ms=" << select_queries.size() / (time_select.avg() / 1000.0 / 1000.0)
	      << " space_in_bytes=" << space
	      << " space_in_mib=" << (space / 1024.0 / 1024.0)
	      << " n_queries=" << select_queries.size()
	      << " n_runs=" << runs << std::endl;    
  }

  
}; // class Benchmark


int32_t main(int argc, char *argv[]) {
tlx::CmdlineParser cp;

  cp.set_description("Simple Wavelet Tree/Wavelet Matrix Benchmark");

  Benchmark bench;

  cp.add_param_string("input", bench.input_path, "Path to input file.");
  cp.add_bytes('n', "size", bench.prefix_size,
               "Size (in bytes unless stated "
               "otherwise) of the string that use to test our suffix array "
               "construction algorithms.");
  cp.add_bytes('q', "queries", bench.number_queries,
               "Number of queries tested. "
               "Default is 10'000'000.");
  cp.add_bytes('r', "runs", bench.runs, "Number of runs the benchmark is executed.");

  if (!cp.process(argc, argv)) {
    return -1;
  }

  bench.run();
  
  return 0;
}



/******************************************************************************/
