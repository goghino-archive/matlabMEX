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
#include <mex.h>   
#include <iostream>

#include <mpi.h>

#ifdef PERF_METRIC
//#include <ctime>
#endif

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
        mexErrMsgIdAndTxt("MyToolbox:arrayProduct:MPI",
                "MPI error detected\n.");
        return;
    }
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    //check number of arguments
    if (nrhs != 3)
    {
         mexErrMsgIdAndTxt("MyToolbox:arrayProduct:nrhs",
                 "Three inputs required.");
         return;
    }

    if (nlhs != 1) {
         mexErrMsgIdAndTxt("MyToolbox:arrayProduct:nlhs",
                 "One output required.");
         return;
    }

    mpi_check(MPI_Init(NULL, NULL));

    int  mpi_size;
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

    if (mpi_size != 1)
    {
        mexErrMsgIdAndTxt("MyToolbox:arrayProduct:mpiSize",
                "Please run with a single process!");
        return;
    } 

    int worker_size; 
    MPI_Comm everyone_comm;  //intercommunicator to workers
    const char* worker_program = "./worker"; //name of worker binary
    //char worker_args[] = ["100", "10"];
    
    //make sure the worker_size argument is scalar
    if( !mxIsDouble(prhs[2]) || mxGetNumberOfElements(prhs[2]) != 1 ) {
        mexErrMsgIdAndTxt("MyToolbox:arrayProduct:notScalar",
                "mpi_size must be a scalar.");
        return;
    }

    //get size of the worker pool
    worker_size = (int)mxGetScalar(prhs[2]);
    if (worker_size < 1)
    {
        mexErrMsgIdAndTxt("MyToolbox:arrayProduct:mpiSize",
                "mpi_size must be greater than 0!");
        return;
    }

   /*  
    * Now spawn the workers. Note that there is a run-time determination 
    * of what type of worker to spawn, and presumably this calculation must 
    * be done at run time and cannot be calculated before starting 
    * the program. If everything is known when the application is  
    * first started, it is generally better to start them all at once 
    * in a single MPI_COMM_WORLD.  
    */ 
 
   //mexPrintf("before spawn\n");
   MPI_Comm_spawn(worker_program, MPI_ARGV_NULL, worker_size,  
             MPI_INFO_NULL, 0, MPI_COMM_SELF, &everyone_comm,  
             MPI_ERRCODES_IGNORE);

   int size_all;
   MPI_Comm_remote_size(everyone_comm,&size_all);
   //mexPrintf("size_all...%d\n",size_all);


  /* 
    * Parallel code here. The communicator "everyone_comm" can be used 
    * to communicate with the spawned processes, which have ranks 0,.. 
    * MPI_worker_size-1 in the remote group of the intercommunicator 
    * "everyone_comm". 
    */

    //declare MEX variables
    mxArray *c_out_m;
    const mwSize *dims;
    double *a, *b, *c;
    int dimx, dimy, numdims;
   
    //check type of input matrices, must be double
    if (!mxIsDouble(prhs[0]) || !mxIsDouble(prhs[1]))
    {
        mexErrMsgIdAndTxt("MyToolbox:arrayProduct:notDouble",
                                      "Input matrices must be of type double.");
        return;
    }

    //figure out dimensions of input matrices
    dims = mxGetDimensions(prhs[0]);
    numdims = mxGetNumberOfDimensions(prhs[0]);
    dimy = (int)dims[0];
    dimx = (int)dims[1];

    //check if input matrices A and B have the same dimensions
    const mwSize *dims_tmp = mxGetDimensions(prhs[1]);
    if(dimy != (int)dims_tmp[0] || dimx != (int)dims_tmp[1])
    {
        mexErrMsgIdAndTxt("MyToolbox:arrayProduct:dimensionMismatch",
                "Dimensions of the input matrices do not match\n");
        return;
    }

    //associate outputs
    c_out_m = plhs[0] = mxCreateDoubleMatrix(dimy,dimx,mxREAL);

    //associate pointers
    a = mxGetPr(prhs[0]);
    b = mxGetPr(prhs[1]);
    c = mxGetPr(c_out_m);

#ifdef PERF_METRIC
    //clock_t clockstart;
    //clock_t cputime;
    //clockstart = clock();

    double start, finish;
    start=MPI_Wtime();
#endif
    //send chunks of the input matrices to workers, columnwise storage
    int size_x = dimx / worker_size;
    int counter = 0;
    for (int i=0; i<worker_size; i++)
    {
        //distribute the remainder - partition x dimension and send it as a single chunk
        int size = size_x;
        if(i < (dimx % worker_size))
            size++;
        size *= dimy;

        //send the size of the data first
        MPI_Send(&size, 1, MPI_INT, i, 0, everyone_comm);

        //send actual data
        MPI_Send(&a[counter], size, MPI_DOUBLE, i, 0, everyone_comm);
        MPI_Send(&b[counter], size, MPI_DOUBLE, i, 1, everyone_comm);

        //move the pointer to the next chunk
        counter += size;

    }

    if (counter != dimx*dimy)
    {
        mexErrMsgIdAndTxt("MyToolbox:arrayProduct:internalError",
                "Size of the data sent do not match actual data size\n");
        return;
    }

    //colect the result from workers
    counter = 0;
    for (int i=0; i<worker_size; i++)
    {
        //distribute the remainder - partition x dimension and send it as a single chunk
        int size = size_x;
        if(i < (dimx % worker_size))
            size++;
        size *= dimy;

        MPI_Recv(&c[counter], size, MPI_DOUBLE, i, 0, everyone_comm, MPI_STATUS_IGNORE);

        //move the pointer to the next chunk
        counter += size;

    }

#ifdef PERF_METRIC
    //cputime = ((double)clock()-clockstart)/(double)CLOCKS_PER_SEC;
    //mexPrintf("Time: %f sec\n", cputime);

    finish=MPI_Wtime(); /*stop timer*/
    mexPrintf("Parallel Elapsed time: %f seconds\n", finish-start); 
#endif
    
    MPI_Finalize();


    return;
}
