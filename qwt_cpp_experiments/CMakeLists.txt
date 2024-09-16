cmake_minimum_required(VERSION 3.4.1 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(qwm_cpp_experiments)

# Add tlx subdirectory only, if it has not been added by another project already
if(NOT TARGET tlx)
  add_subdirectory(extlib/tlx)
endif()

# Add pasta::utils subdirectory only, if it has not been added by another
# project already
if (NOT TARGET pasta_wavelet_tree)
  add_subdirectory(extlib/wavelet_tree)
endif()

add_executable(wavelet_tree_benchmark
  benchmarks/wavelet_tree_benchmark.cpp)
target_link_libraries(wavelet_tree_benchmark PUBLIC
  pasta_wavelet_tree
  tlx
  sdsl)

add_executable(wavelet_tree_construction_benchmark
  benchmarks/wavelet_tree_construction_benchmark.cpp)
target_link_libraries(wavelet_tree_construction_benchmark PUBLIC
  pasta_wavelet_tree
  tlx
  sdsl)

add_executable(text_statistics
  benchmarks/text_statistics.cpp)
target_link_libraries(text_statistics PUBLIC
  pasta_utils
  tlx)

add_executable(bit_vector_benchmark
  benchmarks/bit_vector_benchmark.cpp)
target_link_libraries(bit_vector_benchmark PUBLIC
  pasta_wavelet_tree
  tlx
  sdsl)
