%compile demo.cpp file using default options:
%mex demo.cpp
%or use Makefile to get fine-grained control over
%compiler selection and passed flags

a=[1 1 1; 3 3 3];
b=[2 2 2; 0 0 0];

%call mex function
[c] = demo(a,b)

%run matlab from terminal at mac
%$ /Applications/MATLAB_R2014b.app/bin/matlab -nojvm -nodisplay -nosplash

%run matlab from terminal and execute a script mex_tutorial
%$ /Applications/MATLAB_R2014b.app/Contents/MacOS/MATLAB_maci64 -nodisplay -nosplash -nojvm -r "try, mex_tutorial, catch, end, quit"
%more: https://beagle.whoi.edu/redmine/projects/ibt/wiki/Calling_Matlab_from_the_Linux_command-line

%compile .m script as standalone Mac app
%$ /Applications/MATLAB_R2014b.app/bin/mcc -m mex_tutorial.m
%$ ./run_mex_tutorial.sh /Applications/MATLAB_R2014b.app