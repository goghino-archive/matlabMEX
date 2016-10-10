/* worker */ 
 
#include <mpi.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <stdlib.h>

using namespace std;

inline void mpi_check(int mpi_call)
{
    if ((mpi_call) != 0) { 
        cerr << "MPI error detected" << endl;
        return;
    }
}

int main(int argc, char *argv[]) 
{ 
   int err = MPI_Init(&argc, &argv); 
   mpi_check(err);
   
   int rank, mpi_size;  
   err = MPI_Comm_size(MPI_COMM_WORLD,&mpi_size);
   mpi_check(err);
   err = MPI_Comm_rank(MPI_COMM_WORLD,&rank);
   mpi_check(err);

   char name[256];
   gethostname(name, 256);
   cout << "[worker" << rank << "]Runing on node " << name << endl;

   int mpi_parent_size; 
   MPI_Comm parent_comm; 
   
   //Get inter-communicator to parent process
   err = MPI_Comm_get_parent(&parent_comm); 
   mpi_check(err);
   if (parent_comm == MPI_COMM_NULL)
    {
      cerr << "No parent!" << endl;
      exit(1);
    } 
   err = MPI_Comm_remote_size(parent_comm, &mpi_parent_size); 
   mpi_check(err);
   if (mpi_parent_size != 1)
    {
      cerr << "Something's wrong with the parent" << endl;
      exit(1);
    } 
 
   /*   
    * The manager is represented as the process with rank 0 in (the remote 
    * group of) parent_comm.  If the workers need to communicate among 
    * themselves, they can use MPI_COMM_WORLD.  
    * 
    * We however merge the parent and child communicators 
    * so that there is only one communicator containing
    * parend and all the worker processes. We will put    
    * parent in front of the workers, so he will have 
    * rank 0.
    */    

    MPI_Comm my_world_comm;
    err = MPI_Intercomm_merge(parent_comm, 1, &my_world_comm);
    mpi_check(err);

   int rank_new, mpi_size_new;  
   err = MPI_Comm_size(my_world_comm, &mpi_size_new);
   mpi_check(err);
   err = MPI_Comm_rank(my_world_comm, &rank_new);
   mpi_check(err);

   cout << "[worker" << rank << "]new rank is: " << rank_new
        << " and size is " << mpi_size << "->" << mpi_size_new << endl;

   //get data from manager
   int size;
   MPI_Recv(&size, 1, MPI_INT, 0, 0, parent_comm, MPI_STATUS_IGNORE);
   double *a = new double[size];
   double *b = new double[size];
   double *c = new double[size];
   MPI_Recv(a, size, MPI_DOUBLE, 0, 0, parent_comm, MPI_STATUS_IGNORE);
   MPI_Recv(b, size, MPI_DOUBLE, 0, 1, parent_comm, MPI_STATUS_IGNORE);
   
   //compute local portion of the result
   for (int i = 0; i < size; i++)
   {
       c[i] = a[i] + b[i];
   }

   //send something to manager using intercomm
   MPI_Send(c, size, MPI_DOUBLE, 0, 0, parent_comm);

    delete [] a;
    delete [] b;
    delete [] c;

   MPI_Comm_disconnect(&parent_comm);
   MPI_Comm_free(&my_world_comm);
   MPI_Finalize(); 
   return 0; 
} 
