Following instructions need to be followed for running the program successfully.

1) MD5 hashing algorithm has been used in the program and "hashlib++" has been used for this purpose, thus you may need to install "hashlib++" library.

2) Use make file to compile the program:

Shell Command: 

To run OPENMP program: 
make OMP 

To run Pthreads program: 
make pthread 

3) Command Line arguments to be given:

For OPENMP program:
./showOMP <number of file systems> <filesystem1> <filesystem2> .....<filesystem- K>.

Example:
./showOMP 2 ~/Desktop/Testcases/A/ ~/Desktop/Testcases/B/

Note: File Systems name should end with "/"(backslash).

For PTHREADS program:
./showPthread <number of file systems> <filesystem1> <filesystem2> .....<filesystem- K>.

Example:
./showPthread 2 ~/Desktop/Testcases/A/ ~/Desktop/Testcases/B/

Note: File Systems name should end with "/"(backslash).