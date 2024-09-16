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

#include "../include/wm_int.hpp"

class Benchmark {

public:
  size_t bit_size = 10'000'000;
  size_t number_queries = 10'000'000;
  size_t runs = 10;

  void run() {

    auto const access_queries = generate_queries(number_queries);
    auto const rank_queries = generate_queries(number_queries);
    auto const select_queries = generate_queries(number_queries);

    std::cout << "rank_queries.size() " << rank_queries.size() << '\n';

    std::random_device rd;
    std::mt19937 gen(rd());
    
    pasta::BitVector pasta_bv(bit_size, 0);
    std::uniform_int_distribution<size_t> bit_dist(0, 99);
    for (size_t i = 0; i < bit_size; ++i) {
      pasta_bv[i] = bit_dist(gen) % 2;
    }


    run_experiments_pasta_latency(pasta_bv, access_queries, rank_queries, select_queries,
			    "pasta_bv");

    sdsl::bit_vector sdsl_bv(bit_size, 0);
    for (size_t i = 0; i < bit_size; ++i) {
      sdsl_bv[i] = bit_dist(gen) % 2;
    }

    run_experiments_sdsl_latency(sdsl_bv, access_queries, rank_queries, select_queries,
		    "sdsl_bv");
  }
    
private:

  std::vector<size_t> generate_queries(size_t const number_queries) {
    std::random_device rnd_device;
    std::mt19937 mersenne_engine(rnd_device());
    std::uniform_int_distribution<uint64_t> dist(0, bit_size);

    std::vector<size_t> random_queries(number_queries);
    for (size_t i = 0; i < number_queries; ++i) {
      random_queries[i] = dist(mersenne_engine);
    }
    return random_queries;
  }


  template <typename WaveletMatrix, typename AccessQueries,
	    typename RankQueries, typename SelectQueries>
  void run_experiments_pasta_latency(WaveletMatrix& bv, AccessQueries& access_queries,
			       RankQueries& rank_queries, SelectQueries& select_queries,
			       std::string name) {

    pasta::FlatRankSelect<> pasta_rs(bv);

    
    tlx::Aggregate<size_t> time_access;
    tlx::Aggregate<size_t> time_rank;
    tlx::Aggregate<size_t> time_select;
    for (size_t i = 0; i < runs; ++i) {
      {
        size_t result = 0;
        auto const start = std::chrono::steady_clock::now();
        for (size_t i = 0; i < access_queries.size(); ++i) {
          size_t const pos = (access_queries[i] + result) % bit_size;
          result = bv[pos];
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
          size_t const pos = (rank_queries[i] + result) % bit_size;
	  if (rank_queries[i] % 2 == 0) {
	    result = pasta_rs.rank0(pos);
	  } else {
	    result = pasta_rs.rank1(pos);
	  }
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
          size_t pos = (select_queries[i] + (result % 2)) % (bit_size / 3) ;
          pos = std::max(size_t{1}, pos);
	  if (select_queries[i] % 2 == 0) {
	    result = pasta_rs.select0(pos);
	  } else {
	    result = pasta_rs.select1(pos);
	  }
        }
        
        time_select.add(
          std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                               std::chrono::steady_clock::now() - start)
          .count());
	std::cout << "result " << result << '\n';
      }
      
    }

    size_t space = pasta_rs.space_usage() + bv.space_usage();
    
    std::cout << "RESULT algo=" << name
	      << " exp=" << "access_latency"
	      << " n=" << bit_size
	      << " logn=" << tlx::integer_log2_ceil(bit_size)
	      << " min_time_ns=" << time_access.min() / access_queries.size()
	      << " max_time_ns=" << time_access.max() / access_queries.size()
	      << " avg_time_ns=" << time_access.avg() / access_queries.size()
	      << " space_in_bytes=" << space
	      << " space_in_mib=" << (space / 1024.0 / 1024.0)
	      << " n_queries=" << access_queries.size()
	      << " n_runs=" << runs << std::endl;

    std::cout << "RESULT algo=" << name
	      << " exp=" << "rank_latency"
	      << " n=" << bit_size
	      << " logn=" << tlx::integer_log2_ceil(bit_size)
	      << " min_time_ns=" << time_rank.min() / rank_queries.size()
	      << " max_time_ns=" << time_rank.max() / rank_queries.size()
	      << " avg_time_ns=" << time_rank.avg() / rank_queries.size()
	      << " space_in_bytes=" << space
	      << " space_in_mib=" << (space / 1024.0 / 1024.0)
	      << " n_queries=" << rank_queries.size()
	      << " n_runs=" << runs << std::endl;;

    std::cout << "RESULT algo=" << name
	      << " exp=" << "select_latency"
	      << " n=" << bit_size
	      << " logn=" << tlx::integer_log2_ceil(bit_size)
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
  void run_experiments_sdsl_latency(WaveletMatrix& bv, AccessQueries& access_queries,
			       RankQueries& rank_queries, SelectQueries& select_queries,
			       std::string name) {

    sdsl::bit_vector::rank_0_type sdsl_rank0_support(&bv);
    sdsl::bit_vector::select_0_type sdsl_select0_support(&bv);
    sdsl::bit_vector::rank_1_type sdsl_rank1_support(&bv);
    sdsl::bit_vector::select_1_type sdsl_select1_support(&bv);
    
    
    tlx::Aggregate<size_t> time_access;
    tlx::Aggregate<size_t> time_rank;
    tlx::Aggregate<size_t> time_select;
    for (size_t i = 0; i < runs; ++i) {
      {
        size_t result = 0;
        auto const start = std::chrono::steady_clock::now();
        for (size_t i = 0; i < access_queries.size(); ++i) {
          size_t const pos = (access_queries[i] + result) % bit_size;
          result = bv[pos];
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
          size_t const pos = (rank_queries[i] + result) % bit_size;
	  if (rank_queries[i] % 2 == 0) {
	    result = sdsl_rank0_support.rank(pos);
	  } else {
	    result = sdsl_rank1_support.rank(pos);
	  }
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
          size_t pos = (select_queries[i] + (result % 2)) % (bit_size / 3);
          pos = std::max(size_t{1}, pos);
	  if (select_queries[i] % 2 == 0) {
	    result = sdsl_select0_support.select(pos);
	  } else {
	    result = sdsl_select1_support.select(pos);
	  }
        }
        
        time_select.add(
          std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                               std::chrono::steady_clock::now() - start)
          .count());
	std::cout << "result " << result << '\n';
      }
      
    }

    size_t space = sdsl::size_in_bytes(bv) + sdsl::size_in_bytes(sdsl_rank0_support)
      + sdsl::size_in_bytes(sdsl_select0_support) + sdsl::size_in_bytes(sdsl_select1_support);

    std::cout << "RESULT algo=" << name
	      << " exp=" << "access_latency"
	      << " n=" << bit_size
	      << " logn=" << tlx::integer_log2_ceil(bit_size)
	      << " min_time_ns=" << time_access.min() / access_queries.size()
	      << " max_time_ns=" << time_access.max() / access_queries.size()
	      << " avg_time_ns=" << time_access.avg() / access_queries.size()
	      << " space_in_bytes=" << space
	      << " space_in_mib=" << (space / 1024.0 / 1024.0)
	      << " n_queries=" << access_queries.size()
	      << " n_runs=" << runs << std::endl;

    std::cout << "RESULT algo=" << name
	      << " exp=" << "rank_latency"
	      << " n=" << bit_size
	      << " logn=" << tlx::integer_log2_ceil(bit_size)
	      << " min_time_ns=" << time_rank.min() / rank_queries.size()
	      << " max_time_ns=" << time_rank.max() / rank_queries.size()
	      << " avg_time_ns=" << time_rank.avg() / rank_queries.size()
	      << " space_in_bytes=" << space
	      << " space_in_mib=" << (space / 1024.0 / 1024.0)
	      << " n_queries=" << rank_queries.size()
	      << " n_runs=" << runs << std::endl;;

    std::cout << "RESULT algo=" << name
	      << " exp=" << "select_latency"
	      << " n=" << bit_size
	      << " logn=" << tlx::integer_log2_ceil(bit_size)
	      << " min_time_ns=" << time_select.min() / select_queries.size()
	      << " max_time_ns=" << time_select.max() / select_queries.size()
	      << " avg_time_ns=" << time_select.avg() / select_queries.size()
	      << " space_in_bytes=" << space
	      << " space_in_mib=" << (space / 1024.0 / 1024.0)
	      << " n_queries=" << select_queries.size()
	      << " n_runs=" << runs << std::endl;
  }
  
}; // class Benchmark


int32_t main(int argc, char *argv[]) {
tlx::CmdlineParser cp;

  cp.set_description("Simple bit vector Benchmark");

  Benchmark bench;

  cp.add_bytes('b', "bit_size", bench.bit_size, "Number of bits in the bit vector.");
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
