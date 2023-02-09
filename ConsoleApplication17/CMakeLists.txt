cmake_minimum_required(VERSION 3.22)
project(ConsoleApplication17)

set(CMAKE_CXX_STANDARD 17)

add_executable(ConsoleApplication17 ConsoleApplication17.cpp Compiler.h Compiler.cpp Lexeme.h Lexeme.cpp Parser.h Parser.cpp Stream.h Stream.cpp)