/* worker */ 
 
#include <mpi.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>

using namespace std;

int main(int argc, char *argv[]) 
{ 
   int mpi_parent_size; 
   MPI_Comm parent_comm; 
   MPI_Init(&argc, &argv); 

   //Get inter-communicator to parent process
   MPI_Comm_get_parent(&parent_comm); 
   if (parent_comm == MPI_COMM_NULL)
    {
      cerr << "No parent!" << endl;
      exit(1);
    } 
   MPI_Comm_remote_size(parent_comm, &mpi_parent_size); 
   if (mpi_parent_size != 1)
    {
      cerr << "Something's wrong with the parent" << endl;
      exit(1);
    } 
 
   /*   
    * The manager is represented as the process with rank 0 in (the remote 
    * group of) parent_comm.  If the workers need to communicate among 
    * themselves, they can use MPI_COMM_WORLD. 
    */ 
   int rank, mpi_size;  

   MPI_Comm_size(MPI_COMM_WORLD,&mpi_size);
   MPI_Comm_rank(MPI_COMM_WORLD,&rank);

char name[256];
gethostname(name, 256);
cout << "[worker" << rank << "]Runing on node " << name << endl;


#ifdef DEBUG
    std::stringstream ss;
    ss << rank;
    std::string filename = "file" + ss.str() + ".txt";
    std::ofstream file(filename.c_str());
    file << "[worker]Hello from process " << rank << " from total of " << mpi_size << endl << std::flush;
#endif

#ifdef DEBUG
   file << "Receiving......\n" << std::flush;
#endif
   
   //get data from manager
   int size;
   MPI_Recv(&size, 1, MPI_INT, 0, 0, parent_comm, MPI_STATUS_IGNORE);
#ifdef DEBUG
   file << "The local size of the data is "<< size << std::endl << std::flush;
#endif
   double *a = new double[size];
   double *b = new double[size];
   double *c = new double[size];
   MPI_Recv(a, size, MPI_DOUBLE, 0, 0, parent_comm, MPI_STATUS_IGNORE);
   MPI_Recv(b, size, MPI_DOUBLE, 0, 1, parent_comm, MPI_STATUS_IGNORE);
   
#ifdef DEBUG   
   file << "Computing....\n" << std::flush;
#endif

   //compute local portion of the result
   for (int i = 0; i < size; i++)
   {
       c[i] = a[i] + b[i];
   }


#ifdef DEBUG
   file << "Sending......\n" << std::flush;
#endif

   //send something to manager using intercomm
   MPI_Send(c, size, MPI_DOUBLE, 0, 0, parent_comm);

#ifdef DEBUG
    file << "Info sent\n" << std::flush;
    file.close();
#endif

    delete [] a;
    delete [] b;
    delete [] c;

   MPI_Comm_disconnect(&parent_comm);
   MPI_Finalize(); 
   return 0; 
} 
