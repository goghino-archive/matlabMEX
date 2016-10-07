%% Matlab using parallel MPI solver
% The demo() function is a mex function which spawns new processes,
% distributes the matrices A,B and computes the operations on them locally.
% The result is then reduced to master process running mexFunction() and 
% returned to user.

%%
N = 10;
a = 2*triu(ones(N));
b = 3*a';

%call mex function
mpi_size = 2;
[c] = demo(a,b,mpi_size)

if sum(sum((c - (a+b)))) == 0
    disp('Correct');
else
    disp('Error');
end

%run matlab from terminal at mac
%$ /Applications/MATLAB_R2014b.app/bin/matlab -nojvm -nodisplay -nosplash

%run matlab from terminal and execute a script mex_tutorial
%$ /Applications/MATLAB_R2014b.app/Contents/MacOS/MATLAB_maci64 -nodisplay -nosplash -nojvm -r "try, mex_tutorial, catch, end, quit"
%more: https://beagle.whoi.edu/redmine/projects/ibt/wiki/Calling_Matlab_from_the_Linux_command-line

%compile .m script as standalone Mac app
%$ /Applications/MATLAB_R2014b.app/bin/mcc -m mex_tutorial.m
%$ ./run_mex_tutorial.sh /Applications/MATLAB_R2014b.app
