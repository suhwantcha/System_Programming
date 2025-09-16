cmake_minimum_required(VERSION 3.27)
project(20222089 C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(20222089
        bitmap.c
        bitmap.h
        debug.c
        debug.h
        hash.c
        hash.h
        hex_dump.c
        hex_dump.h
        limits.h
        list.c
        list.h
        round.h
        main.c
        hash.c
        hash.c
        list.c
        bitmap.c)
