# KCF tracker – parallel and PREM implementations

The goal of this project is modify KCF tracker for use in the [HERCULES](http://hercules2020.eu/) project, where it will run on NVIDIA TX2 board. To achieve the needed performance we try various ways of parallelization of the algorithm including execution on the GPU. The aim is also to modify the code according to the PRedictable Execution Model (PREM).

Stable version of the tracker is available from [CTU server](http://rtime.felk.cvut.cz/gitweb/hercules2020/kcf.git.), development happens at [Github](https://github.com/Shanigen/kcf.).

## Prerequisites

The code depends on OpenCV 2.4 (3.0+ for CUDA-based version) library and cmake is used for building. Depending on the version to be compiled you have to have [FFTW](http://www.fftw.org/), [CUDA](https://developer.nvidia.com/cuda-downloads) or [OpenMP](http://www.openmp.org/) installed.

SSE instructions were used in the original code and these are only supported on x86 architecture. Thanks to the [SSE2NEON](https://github.com/jratcliff63367/sse2neon) code, we now support both ARM and x86 architectures.

## Compilation

There are multiple ways how to compile the code.

### Compile all supported versions

``` shellsession
$ git submodule update --init
$ make -k
```

This will create several `build-*` directories and compile different
versions in them. If prerequisites of some builds are missing, the
`-k` option ensures that the errors are ignored. This uses [Ninja](https://ninja-build.org/) build system, which is useful when building naively on TX2, because builds with `ninja` are faster (better parallelized) than with `make`. To build only specific version run:

``` shellsession
make <version>
```

### Using cmake gui

```shellsession
$ git submodule update --init
$ mkdir build
$ cmake-gui .
```

- Use the just created build directory as "Where to build the binaries".
- Press "Configure". 
- Choose desired build options. Each option has a comment  briefly explaining what it does.
- Press "Generate" and close the window. 

```shellsession
$ make -C build
```
### Command line

```shellsession
$ git submodule update --init
$ mkdir build
$ cd build
$ cmake [options] ..
```

The `cmake`  options below allow to select, which version to build.

The following table shows how to configure different FFT implementations.

|Option| Description |
| --- | --- |
| `-DFFT=OpenCV` | Use OpenCV to calculate FFT.|
| `-DFFT=fftw` | Use fftw and its `plan_many` and "New-array execute" functions.|
| `-DFFT=cuFFTW` | Use cuFFTW interface to cuFFT library.|
| `-DFFT=cuFFT` | Use cuFFT. This version also uses pure CUDA implementation of `ComplexMat` class and Gaussian correlation.|

With all of these FFT version additional options can be added:

|Option| Description |
| --- | --- |
| `-DASYNC=ON` | Use C++ `std::async` to run computations for different scales in parallel. This doesn't work with `BIG_BATCH` mode.|
| `-DOPENMP=ON` | FIXME: Fftw version of FFT using plan many and new execute functions. If not selected with big batch mode, it also is used to compute all response maps for all scales in parallel. If used with big batch mode it parallelize extraction of features for all scales and selection of scale with highest response.|
| `-DBIG_BATCH=ON` | Concatenate matrices of different scales to one big matrix and perform all computations on this matrix. This mode doesn't work for OpenCV FFT.|
| `-DCUDA_DEBUG=ON` | This mode adds CUDA error checking for all kernels and CUDA runtime libraries. Only works with cuFFT version.|

Finally call make:
```
$ make
```

### Compilation for non-TX2 CUDA

The CuFFT version is set up to run on NVIDIA Jetson TX2. If you want to run it on different architecture, change the `--gpu-architecture sm_62` NVCC flag in **/src/CMakeLists.txt** to your architecture of NVIDIA GPU. To find what SM variation you architecture has look [here](http://arnon.dk/matching-sm-architectures-arch-and-gencode-for-various-nvidia-cards/).

## Running

No matter which method is used to compile the code, the results will be `kcf_vot` binary.

It operates on an image sequence created according to [VOT 2014 methodology](http://www.votchallenge.net/). You can find some image sequences in [vot2016 datatset](http://www.votchallenge.net/vot2016/dataset.html).

The binary can be run as follows:

1. `./kcf_vot [options]`

   The program looks for `groundtruth.txt` or `region.txt` and `images.txt` files in current directory.
   - `images.txt` contains a list of images to process, each on a separate line.
   - `groundtruth.txt` contains the correct location of the tracked object in each image as four corner points listed clockwise starting from bottom left corner. Only the first line from this file is used.
   - `region.txt` is an alternative way of specifying the location of the object to track via its bounding box (top_left_x, top_left_y, width, height) in the first frame.

2. `./kcf_vot [options] <directory>`

   Looks for `groundtruth.txt` or `region.txt` and `images.txt` files in the given `directory`.

3. `./kcf_vot [options] <path/to/region.txt or groundtruth.txt> <path/to/images.txt> [path/to/output.txt]`

By default the program generates file `output.txt` containing the bounding boxes of the tracked object in the format "top_left_x, top_left_y, width, height".

### Options

| Options | Description |
| ------- | ----------- |
| --visualize, -v [delay_ms] | Visualize the output, optionally with specified delay. If the delay is 0 the program will wait for a key press. |
| --output, -o <output.txt>	 | Specify name of output file. |
| --debug, -d				 | Generate debug output. |
| --fit, -f [W[xH]] | Specifies the dimension to which the extracted patch should be scaled. It should be divisible by 4. No dimension is the same as `128x128`, a single dimension `W` will result in patch size of `W`×`W`. |


## Authors
* Vít Karafiát, Michal Sojka

Original C++ implementation of KCF tracker was written by Tomas Vojir [here](https://github.com/vojirt/kcf/blob/master/README.md) and is reimplementation of algorithm presented in "High-Speed Tracking with Kernelized Correlation Filters" paper [1].

## References

[1] João F. Henriques, Rui Caseiro, Pedro Martins, Jorge Batista, “High-Speed Tracking with Kernelized Correlation Filters“,
IEEE Transactions on Pattern Analysis and Machine Intelligence, 2015

## License

Copyright (c) 2014, Tomáš Vojíř\
Copyright (c) 2018, Vít Karafiát\
Copyright (c) 2018, Michal Sojka

Permission to use, copy, modify, and distribute this software for research
purposes is hereby granted, provided that the above copyright notice and
this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
