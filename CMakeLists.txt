cmake_minimum_required(VERSION 3.22)
project(AES_C C)

set(CMAKE_C_STANDARD 99)

add_executable(AES_C main.c
        aes.c)
