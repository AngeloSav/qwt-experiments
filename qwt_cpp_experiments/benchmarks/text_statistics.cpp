#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>
#include <thread>

#include <tlx/cmdline_parser.hpp>
#include <tlx/math.hpp>

#include <pasta/utils/reduce_alphabet.hpp>


class Benchmark {

private:
  std::vector<uint8_t> input_;

public:
  size_t alphabet_size_;
  size_t prefix_size = {0};
  std::string input_path = "";

  void run() {
    load_text();
    reduce_alphabet();

    std::cout << "RESULT algo=pasta_construction"
	      << " input=" << input_path
	      << " n=" << input_.size()
	      << " logn=" << tlx::integer_log2_ceil(input_.size())
	      << " reduced_alphabet_size=" << alphabet_size_ << std::endl;
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

}; // class Benchmark


int32_t main(int argc, char *argv[]) {
tlx::CmdlineParser cp;

  cp.set_description("Simple Text Statistics");

  Benchmark bench;

  cp.add_param_string("input", bench.input_path, "Path to input file.");
  cp.add_bytes('n', "size", bench.prefix_size,
               "Size (in bytes unless stated "
               "otherwise) of the string that use to test our suffix array "
               "construction algorithms.");

  if (!cp.process(argc, argv)) {
    return -1;
  }

  bench.run();
  
  return 0;
}



/******************************************************************************/
