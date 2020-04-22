cmake_minimum_required(VERSION 3.4.3)
project(toycompiler)


find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

set(CMAKE_CXX_STANDARD 17)


include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})


# Now build our tools
add_executable(toycompiler src/toy.cpp)

# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_libs core support)

# Link against LLVM libraries
target_link_libraries(toycompiler ${llvm_libs})
message(STATUS "LLVM linked to: ${llvm_libs}")
