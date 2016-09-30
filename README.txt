1. kardos@icsmaster01 (master):~/misc/matlabMEX$ module list
   Currently Loaded Modulefiles:
     1) use.own         2) gcc/6.1.0       3) openmpi/2.0.1   4) matlab/R2016a

    OpenMPI was configured as following: --prefix=${INSTALLDIR} --enable-mpi-fortran=all --with-pmi --disable-dlopen
    The important flag when using MEX is "--disable-dlopen" which specifies that  all plugins will be slurped into Open
    MPI's libraries and it will cause that Open MPI will not look for / open any DSOs at run time 
    (https://www.open-mpi.org/faq/?category=building#avoid-dso).

2. Export path to matlab library and to slurm dependency

   kardos@archimedes:~/misc/matlabMEX$ export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/kardos/openmpi-2.0.0/lib/
   kardos@icsmaster01 (master):~/misc/matlabMEX$ export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/apps/matlab/R2016a/bin/glnxa64/
   kardos@icsmaster01 (master):~/misc/matlabMEX$ export LD_PRELOAD=/usr/lib64/libslurm.so

3. $ make 

4. $ matlab -nojvm -nodisplay -nosplash -r "matlabDemo"
    or
   $ make run
