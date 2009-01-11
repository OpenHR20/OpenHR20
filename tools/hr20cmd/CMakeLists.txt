project(hr20cmd)

set(APPLICATION_NAME "hr20cmd")
set(APPLICATION_VERSION "0.1")
set(SRCS hr20cmd.c hr20.c serial.c) 

cmake_minimum_required(VERSION 2.6)

add_executable(hr20cmd ${SRCS})
