# redis-experiment
This is a toy program to get used to using C with the ```hiredis``` library to interact with a Redis server.
I created this project using CLion, but it is essentially a CMake project.

## Setup
First download the official hiredis C library, then compile the library using the include make file. Make sure to
change the ```CMakeLists.txt``` file in the project root folder to point to the ```hiredis``` binary to include the library
in the project. You will also need Redis installed on your machine.

## Using the Program
First make sure that a Redis server running on local host. Then when ```main.c``` is started the threads of the program
will all put the numbers 1 through 10 into a Redis List in order, then block waiting on thier respective queues to have a
"GO" string inserted. To get the threads to start again, set CNT to 0 and put ```GO``` into each thread's list. There
are more instructions on how to do this and stop the threads to exit the program within ```main.c```.
