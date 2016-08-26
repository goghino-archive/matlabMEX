/*********************************************************************
 * Demo.cpp
 *
 * This file shows the basics of setting up a mex file to work with
 * Matlab.  This example shows how to use 2D matricies.  This may
 * 
 * Keep in mind:
 * <> Use 0-based indexing as always in C or C++
 * <> Indexing is column-based as in Matlab (not row-based as in C)
 * <> Use linear indexing.  [x*dimy+y] instead of [x][y]
 *
 * For more information, see my site: www.shawnlankton.com
 * by: Shawn Lankton
 *
 ********************************************************************/
#include <matrix.h>
#include <mex.h>   
#include <iostream>

 #include <mpi.h>

/* Definitions to keep compatibility with earlier versions of ML */
#ifndef MWSIZE_MAX
typedef int mwSize;
typedef int mwIndex;
typedef int mwSignedIndex; 

#if (defined(_LP64) || defined(_WIN64)) && !defined(MX_COMPAT_32)
/* Currently 2^48 based on hardware limitations */
# define MWSIZE_MAX    281474976710655UL
# define MWINDEX_MAX   281474976710655UL
# define MWSINDEX_MAX  281474976710655L
# define MWSINDEX_MIN -281474976710655L
#else
# define MWSIZE_MAX    2147483647UL
# define MWINDEX_MAX   2147483647UL
# define MWSINDEX_MAX  2147483647L
# define MWSINDEX_MIN -2147483647L
#endif
#define MWSIZE_MIN    0UL
#define MWINDEX_MIN   0UL
#endif

inline void mpi_check(int mpi_call)
{
    if ((mpi_call) != 0) { 
        std::cerr << "MPI Error detected!" << std::endl;
        exit(1);
    }
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{

    mpi_check(MPI_Init(NULL, NULL));

    int mpi_rank, mpi_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    mexPrintf("Hello, I am %d out of %d\n", mpi_rank, mpi_size);


//declare variables
    mxArray *a_in_m, *b_in_m, *c_out_m, *d_out_m;
    const mwSize *dims;
    double *a, *b, *c, *d;
    int dimx, dimy, numdims;
    int i,j;

//associate inputs
    a_in_m = mxDuplicateArray(prhs[0]);
    b_in_m = mxDuplicateArray(prhs[1]);

//figure out dimensions
    dims = mxGetDimensions(prhs[0]);
    numdims = mxGetNumberOfDimensions(prhs[0]);
    dimy = (int)dims[0];
    dimx = (int)dims[1];
    mexPrintf("dimx: %d dimy %d\n", dimx, dimy);


//associate outputs
    c_out_m = plhs[0] = mxCreateDoubleMatrix(dimy,dimx,mxREAL);

//associate pointers
    a = mxGetPr(a_in_m);
    b = mxGetPr(b_in_m);
    c = mxGetPr(c_out_m);

//do something
    for (i=0;i<dimx;i++)
    {
        for (j=0;j<dimy;j++)
        {
            c[i*dimy+j] = a[i*dimy+j] + b[i*dimy+j]; 
        }
    }

    MPI_Finalize();


    return;
}
