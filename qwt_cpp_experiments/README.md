# How to Use

This project contains the C++ code for our paper "Faster Wavelet Tree Queries".
Note that this project depends on the [SDSL](https://github.com/simongog/sdsl-lite/), which should be installed on the system.
To build the project, simply run

```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j
```

This will build the project.
Then you have four binaries:

1. `bit_vector_benchmark` generates the results for pasta_bv and sdsl_bv in Figure 5.
2. `text_statistics` generates statistics of the text, i.e., the alphabet size.
   Used to generate the data in Table 1.
3. `wavelet_tree_benchmark` generates the results for pasta_wm and sdsl_wm in Figure 3, 5, and 6.
   Requires a text as input file.
4. `wavelet_tree_construction_benchmark` generates the results for pasta_wm and sdsl_wm in Figure 4.
   Requires a text as input.

For binaries 2--4, only an input file is needed.
All other parameters correspond to the default parameters.
We used [sqlplot-tools](https://github.com/bingmann/sqlplot-tools) to generate the plots from the output of the experiments.

Note that a thorough study of the source code may reveal the authors.
We did our best to anonymize the code, however, it is possible that we missed something.
Also remember that we disabled turbo boost and pinned the thread to have a fair testing environment.
All scripts used for the evaluation are available in the `scripts` folder.
