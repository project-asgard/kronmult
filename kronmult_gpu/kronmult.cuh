#pragma once
#include <cuda.h>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include "linear_algebra.cuh"

/*
 * computes number^power for integers
 * does not care about performances
 * does not use std::pow as it does an implicit float conversion that could lead to rounding errors for high
 * numbers
 */
__host__ int pow_int(const int number, const int power);

/*
 * Computes output += kron(matrix_list) * input
 *
 * `matrix_list` is an array containing pointers to `matrix_number` square matrices of size `matrix_size` by `matrix_size` and stride `matrix_stride`
 * `input` is a `size_input`=`matrix_size`^`matrix_number` elements vector
 * `output` is a `size_input` elements vector, where the output will be stored
 * `workspace` is a `size_input` elements vector, to be used as workspace
 * `transpose_workspace` is a vector of size `matrix_size`*`matrix_size` to store transposed matrices temporarily
 *
 * WARNINGS:
 * `input` and `workspace` will be used as temporary workspaces and thus modified
 * the matrices should be stored in col-major order
 */
template<typename T>
__device__ void cuda_kronmult(const int matrix_count, const int matrix_size, T const * const matrix_list[], const int matrix_stride,
              T input[], const int size_input,
              T output[], T workspace[])
{
    // how many column should `input` have for the multiplications to be legal
    const int nb_col_input = size_input / matrix_size;

    // iterates on the matrices from the last to the one just before first
    for(int i = matrix_count-1; i >= 0; i--)
    {
        // takes `matrix` into account and put the result in `workspace` (use `output` as a workspace if needed)
        T const * const matrix = matrix_list[i];
        multiply_transpose<T>(input, nb_col_input, matrix, matrix_size, matrix_stride, workspace);
        // swap `input` and `workspace` such that `input` contains once again the input
        // note that, while they have the same size flattened, the shape (nb_columns and nb_rows) of `input` and `workspace` are different
        // this is on purpose and equivalent to a reshape operation that is actually needed by the algorithm
        T* temp = input;
        input = workspace;
        workspace = temp;
    }

    // reduce in a threadsafe way
    for(int i = 0; i < size_input; i++)
    {
        atomicAdd(&output[i], input[i]);
    }
}

/*
 * Computes output[K] += kron(matrix_list[K]) * input[K] for 0 <= k < batchCount
 *
 * `matrix_list_batched` is an array of `nb_batch`*`matrix_count` pointers to square matrices of size `matrix_size` by `matrix_size` and stride `matrix_stride`
 * `input_batched` is an array of `nb_batch` vectors of size `matrix_size`^`matrix_count`
 * `output_batched` is an array of `nb_batch` vectors of size `matrix_size`^`matrix_count`, where the outputs will be stored
 * `workspace` is an array of `nb_batch` vectors of size `matrix_size`^`matrix_count`, to be used as workspaces
 *
 * WARNINGS:
 * `input_batched` and `workspace_batched` will be used as temporary workspaces and thus modified
 * the matrices should be stored in col-major order
 */
template<typename T>
__global__ void cuda_kronmult_batched(const int matrix_count, const int matrix_size, T const * const matrix_list_batched[], const int matrix_stride,
                                      T* input_batched[], const int size_input,
                                      T* output_batched[], T* workspace_batched[],
                                      const int nb_batch)
{
    // each thread get a single batch
    const int batchId = blockIdx.x * blockDim.x + threadIdx.x;
    if(batchId < nb_batch)
    {
        // computes kronmult
        T const * const * matrix_list = &matrix_list_batched[batchId*matrix_count];
        T* input = input_batched[batchId];
        T* output = output_batched[batchId];
        T* workspace = workspace_batched[batchId];
        // result is stored in `workspace` if `matrix_count` is odd and `input` if it is even
        cuda_kronmult<T>(matrix_count, matrix_size, matrix_list, matrix_stride, input, size_input, output, workspace);
    }
}

/*
 * Calls the cuda kernel with proper thread parameters.
 * This function expects its inputs to already be on the device (GPU).
 * needs to be in defined in a .cu file for <<<>>>
 */
template<typename T>
__host__ int kronmult_batched(const int matrix_count, const int matrix_size, T const * const matrix_list_batched[], const int matrix_stride,
                      T* input_batched[],
                      T* output_batched[], T* workspace_batched[],
                      const int nb_batch);
