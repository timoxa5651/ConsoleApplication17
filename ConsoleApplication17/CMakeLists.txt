cmake_minimum_required(VERSION 3.20)
project(ConsoleApplication17)

set(CMAKE_CXX_STANDARD 17)

add_executable(ConsoleApplication17 ConsoleApplication17.cpp OCompiler.h OCompiler.cpp Lexeme.h Lexeme.cpp Parser.h Parser.cpp Stream.h Stream.cpp Poliz.cpp Poliz.h Precompile.h Precompile.cpp Runtime.h Runtime.cpp)