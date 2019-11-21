#include "kroncommon.hpp"
#include "kgemm_nn_batched.hpp"




void kgemm_nn_batched( int const mm, int const nn, int const kk, 
                       double const alpha, 
                       double* const Aarray_[], 
                       int const ldAarray_[], 
                       double* const Barray_[], 
                       int const ldBarray_[], 
                       double const beta,  
                       double* const Carray_[], 
                       int const ldCarray_[], 
                       int const batchCount)
{
#ifdef USE_GPU
        dim3 grid(batchCount,1,1);
        dim3 block(16,16,1);

        kgemm_nn_batched<double><<< grid, block>>>( mm,nn,kk,
                          alpha,
                          Aarray_, ldAarray_,
                          Barray_, ldBarray_,
                          beta,
                          Carray_, ldCarray_,
                          batchCount );
#else
        kgemm_nn_batched<double>( mm,nn,kk,
                          alpha,
                          Aarray_, ldAarray_,
                          Barray_, ldBarray_,
                          beta,
                          Carray_, ldCarray_,
                          batchCount );
#endif
}
