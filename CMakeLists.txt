cmake_minimum_required(VERSION 3.15)
project(simple_connect C)

# make sure to Change REDIS and HIREDIS to your version of the hiredis C library
# also make sure that they point to your version of the compiled library, not just the
# source code.

set(CMAKE_C_STANDARD 11)
set(REDIS /home/wyatt/Desktop/SONIC/)
set(HIREDIS ${REDIS}/hiredis)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include_directories(${HIREDIS})
add_executable(simple_connect main.c)

target_link_libraries(simple_connect ${HIREDIS}/libhiredis.a)
target_link_libraries(simple_connect Threads::Threads)

