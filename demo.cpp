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
        mexPrintf("MPI Error detected!\n");
        return;
    }
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{

    mpi_check(MPI_Init(NULL, NULL));

    int mpi_rank, mpi_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    //mexPrintf("Hello, I am %d out of %d\n", mpi_rank, mpi_size);

    int worker_size; 
    MPI_Comm everyone_comm;  //intercommunicator to workers
    const char* worker_program = "./worker"; //name of worker binary
    //const char* worker_program = argv[0];
    //char worker_args[] = ["100", "10"];

    if (mpi_size != 1)
    {
       mexPrintf("Please run with a single process!\n");
       return;
    } 

    //determine the size of worker pool
    worker_size = 2;

    if (worker_size < 1)
    {
       mexPrintf("No room to start workers!\n");
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
 
   mexPrintf("before spawn\n");
   MPI_Comm_spawn(worker_program, MPI_ARGV_NULL, worker_size,  
             MPI_INFO_NULL, 0, MPI_COMM_SELF, &everyone_comm,  
             MPI_ERRCODES_IGNORE);

   int size_all;
   MPI_Comm_remote_size(everyone_comm,&size_all);
   mexPrintf("size_all...%d\n",size_all);


  /* 
    * Parallel code here. The communicator "everyone_comm" can be used 
    * to communicate with the spawned processes, which have ranks 0,.. 
    * MPI_worker_size-1 in the remote group of the intercommunicator 
    * "everyone_comm". 
    */

  // mexPrintf("recieving...\n");
  // int info = 0;
  // for (int i=0; i<worker_size; i++)
  // {
  //    MPI_Recv(&info, 1, MPI_INT, i, 0, everyone_comm, MPI_STATUS_IGNORE);
  //    mexPrintf("Recieved info: %d\n", info); 
  // }

   // info = worker_size*1000;
   // for (int i=0; i<worker_size; i++)
   // {
   //    MPI_Send(&info, 1, MPI_INT, i, 0, everyone_comm);
   // }

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

    //check if input matrices A and B have the same dimensions
    const mwSize *dims_tmp = mxGetDimensions(prhs[1]);
    if(dimy != (int)dims_tmp[0] || dimx != (int)dims_tmp[1])
    {
        mexPrintf("ERROR: Dimension of input matrices do not match\n");
        return;
    }

    //associate outputs
    c_out_m = plhs[0] = mxCreateDoubleMatrix(dimy,dimx,mxREAL);

    //associate pointers
    a = mxGetPr(a_in_m);
    b = mxGetPr(b_in_m);
    c = mxGetPr(c_out_m);

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
        mexPrintf("ERROR: size of the data sent do not match actual data size\n");
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
    
   // for (i=0;i<dimx;i++)
   // {
   //     for (j=0;j<dimy;j++)
   //     {
   //         c[i*dimy+j] = a[i*dimy+j] + b[i*dimy+j]; 
   //     }
   // }

    MPI_Finalize();


    return;
}
