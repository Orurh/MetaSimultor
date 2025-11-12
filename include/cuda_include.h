#pragma once

#ifdef __CUDACC__
    #include <cuda_runtime.h>
#else
    #define __host__
    #define __device__
    #define __global__
    #define __shared__
    #define __constant__
    #define __launch_bounds__(...)
#endif