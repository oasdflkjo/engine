# toolchain.cmake
set(CMAKE_C_COMPILER "C:/Program Files/LLVM/bin/clang.exe" CACHE STRING "C compiler")
set(CMAKE_CXX_COMPILER "C:/Program Files/LLVM/bin/clang++.exe" CACHE STRING "C++ compiler")
set(CMAKE_LINKER "C:/Program Files/LLVM/bin/lld-link.exe" CACHE STRING "Linker")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld")
