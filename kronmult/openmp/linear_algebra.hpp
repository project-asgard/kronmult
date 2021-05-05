#pragma once
const bool DEBUG = false;
/*
 * converts row and col indices into a single index for a matrix store in col-major
 * `stride` is usually the number of rows of the matrix
 */
inline int colmajor(const int row, const int col, const int stride)
{
    return row + col*stride;
}

/*
 * computes output = input^T
 *
 * `input` is a `matrix_size` by `matrix_size` square matrix of stride `input_stride`
 * `output` is a `matrix_size` by `matrix_size` square matrix of stride `matrix_size`
 *
 * WARNING: the matrices are assumed to be stored in col-major order
 */
template<typename T>
void transpose(const T input[], T output[], const int matrix_size, const int input_stride)
{
    for(int r = 0; r < matrix_size; r++)
    {
        for(int c = 0; c < matrix_size; c++)
        {
            output[colmajor(r, c, matrix_size)] = input[colmajor(c, r, input_stride)];
        }
    }
}

// BLAS function header: call to mkl, lapack, magma or others.
extern "C"
{
    // matric multiplication
    int dgemm_(char *transa, char *transb, int *m, int *n, int *k, double *alpha, double *A, int *lda, double *B, int *ldb, double *beta, double *C, int *ldc);
    int sgemm_(char *transa, char *transb, int *m, int *n, int *k, float *alpha, float *A, int *lda, float *B, int *ldb, float *beta, float *C, int *ldc);
}

/*
 * Computes Y = X^T * M^T
 *      <=> Y[i,j] = X[k,i] * M[j,k]
 *
 * X is a `size_M` by `nb_col_X` matrix
 * M is a `size_M` by `size_M` matrix of stride `matrix_stride`
 * Y is a `nb_col_X` by `size_M` matrix
 * M_transposed is a `size_M` by `size_M` matrix of stride `size_M` to store M^T temporarily
 *
 * WARNING: the matrices are assumed to be stored in col-major order
 */
template<typename T>
void multiply_transpose_blas(const T X[], const int nb_col_X,
                                        const T M[], const int size_M, const int stride_M,
                                        T Y[])
{
    //BLAS is in col MAJOR
    // Even if we compute
    // Y = X^T * M^T
    // when filling, m,n,k, lda,ldb, and ldc,
    // we must consider the original product: Y = M * X

    // C = alpha*A*B + beta * C
    // C (m,n), A (m,k), B (k,n)
    // Y (nbCol,sizeM), X (sizeM,nbCol), M (sizeM,sizeM)
    // m == nbCol
    // n == size_M
    // k == size_M
    // <--> C<>Y  A<>X B<M>
    // Y = X^T * M^T
    char transa = 'T'; 
    char transb = 'T';
    int m = nb_col_X;
    int n = size_M;
    int k = size_M;
    T one = 1.;
    const T *A = X;
    int lda = size_M;
    const T *B = M;
    int ldb = stride_M;
    T zero = 0.0;
    T *C = Y;
    int ldc = nb_col_X; // why size_M vector horizontal, and not nb_col_X vector vertical?
    if(DEBUG){
    std::cerr <<
        "transa(" << transa << ") "
       << " transb(" << transb << ") "
       << " m(" << m << ") "
       << " n(" << n << ") "
       << " k(" << k << ") "
       << " alpha("  << one << ") "
       << " lda(" << lda << ") "
       << " ldb(" << ldb << ") "
       << " beta(" <<zero << ") "
       << " ldc(" << ldc << ") "
        << std::endl;
    }
#define TONONCONST(X) ((T*)(size_t)(X))
    static constexpr bool value = std::is_same<T, float>::value;
    if constexpr (value)
        int res = sgemm_(&transa, &transb, &m, &n, &k, &one, TONONCONST(A), &lda, TONONCONST(B), &ldb, &zero, C, &ldc);
    else
        int res = dgemm_(&transa, &transb, &m, &n, &k, &one,  TONONCONST(A), &lda, TONONCONST(B), &ldb, &zero, C, &ldc);
}

/*
 * Computes Y = X^T * M^T
 *      <=> Y[i,j] = X[k,i] * M[j,k]
 *
 * X is a `size_M` by `nb_col_X` matrix
 * M is a `size_M` by `size_M` matrix of stride `matrix_stride`
 * Y is a `nb_col_X` by `size_M` matrix
 * M_transposed is a `size_M` by `size_M` matrix of stride `size_M` to store M^T temporarily
 *
 * WARNING: the matrices are assumed to be stored in col-major order
 */
template<typename T>
void multiply_transpose(const T X[], const int nb_col_X,
                                        const T M[], const int size_M, const int stride_M,
                                        T Y[], T M_transposed[])
{
    // transpose the matrix to get a better alignement
    transpose(M, M_transposed, size_M, stride_M);

    for(int rowM=0; rowM < size_M; rowM++)
    {
        const T* M_transposed_col = &M_transposed[colmajor(0,rowM,size_M)];
        for(int colX=0; colX < nb_col_X; colX++)
        {
            const T* X_transposed_row = &X[colmajor(0,colX,size_M)];
            T dotprod = 0.;
            #pragma omp simd reduction(+:dotprod)
            for(int k=0; k < size_M; k++)
            {
                dotprod += X_transposed_row[k] * M_transposed_col[k];
            }
            Y[colmajor(colX,rowM,nb_col_X)] = dotprod;
        }
    }
}
