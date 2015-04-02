### Introduction ###
This library implements volume visualization of meshless data on NVIDIA graphics hardware using CUDA. Currently, the volume visualization techniques used is Fourier volume rendering. There is support for meshless data using the SPH spline, the C2 Wendland function, and the Gaussian. Please contact us if you need other basis functions supported.

### Authors ###
  * Andrew Corrigan, acorriga@gmu.edu
  * John Wallin, jwallin@gmu.edu

### License ###
The code is licensed under the GPL.  If it would be useful to you for the code to be licensed under an alternative license, please contact us.  We ask that if you use this library for research, to please consider citing the paper: [Visualization of Meshless Simulations Using Fourier Volume Rendering](http://cds.gmu.edu/~acorriga/pubs/meshless_fvr)

### Requirements ###
  * [CUDA](http://www.nvidia.com/object/cuda_home.html)
  * [CUDPP](http://www.gpgpu.org/developer/cudpp/)
  * [GLEW](http://glew.sourceforge.net/)
  * vis\_wx: Hardware OpenGL support for OpenGL >=2.1 and the ARB\_texture\_float extension
  * vis\_wx: [wxWidgets 2.8](http://www.wxwidgets.org/)
  * vis\_wx: [Boost.Math.Quaternions](http://www.boost.org/doc/html/boost_math/quaternions.html)
  * vis\_wx, vis\_noninteractive: [Magick++](http://www.imagemagick.org/Magick++/)
  * CPU: [GCC with OpenMP support](http://gcc.gnu.org/gcc-4.2/)
  * CPU: [Single-precision, multi-threaded, FFTW 3.1.2](http://www.fftw.org/)

### Known issues ###
  * SEVERE: This code does not avoid the Windows/Linux/CUDA bug that if the graphics card takes too long to execute a kernel then the code/OS crashes, if the graphics card is being used for a display.  This happens with libMeshlessVis if you turn the settings up too high with respect to how fast your hardware is.

### Building ###
  * We recommend using our 32-bit Windows XP binaries or our 64-bit Ubuntu 7.10 binaries.
  * There are makefiles for Linux and Visual Studio solutions and projects included for Windows

### Using the CUDA library in your code ###
  1. Include meshless\_vis.h
  1. Link with the static library,
    * Windows: meshless\_vis.lib
    * Linux: libmeshless\_vis64.a
  1. Link with the CUDA libraries:
    * Windows:
      * cuda.lib, cudart.lib, cufft.lib, cutil.lib
    * Linux:
      * libcuda.a, libcudart.a, libcufft.a, libcutil.a
  1. Link with CUDPP
  1. Link with GLEW and OpenGL

### Using the CPU library in your code ###
  1. define `_LIBMESHLESSVIS_USE_CPU` and then include meshless\_vis.h
  1. Link with the static library,
    * Windows: meshless\_vis\_cpu.lib
    * Linux: libmeshless\_vis64\_cpu.a
  1. Link with FFTW (the single-precision, multi-threaded version)
  1. Link with GLEW and OpenGL
  1. When running, make sure to set OMP\_NUM\_THREADS

### What's New? ###
  * Version 0.4
    * Parallelization using partial sums
    * An alternative back end using C++, OpenMP, and multi-threaded FFTW 3.
    * Removed the Python code generation in favor of using C++-style templates
  * Version 0.3.5 - February 25, 2008 -  Performance Improvements, Renaming
    * More efficient memory access (better coalescing, better shared memory usage)
    * More optimal thread block sizes
    * Block size can be controlled at run time.
    * Throughout the library the word "interpolant" has been replaced with "data" and "fvr" with "vis".
    * Added support for the special case of no argument scaling
    * Added support for the Gaussian
  * Version 0.3 - June 22, 2007
    * The previous versions computed the projection incorrectly, this has been fixed.
    * The library has been significantly optimized (observed running 6x faster): using shared memory, 32-bit fast math functions, and a complex-to-real Fourier transform.
    * The interactive application has been redone using wxWidgets.
    * There is a Python script for generating CUDA kernels for new functions.
  * Version 0.2 - April 25, 2007
    * The interactive application is working
    * This is a source code only release (I'll provide binaries for major releases)
  * Version 0.1 - April 22, 2007
    * The entire library has only been tested when using CUDA emulation
    * The interactive application is untested
    * The next version will be released once I get a 8600 and CUDA support is released for it.