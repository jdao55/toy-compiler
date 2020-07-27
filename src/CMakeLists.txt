add_executable(toycompiler toy.cpp)

# Link against LLVM libraries
target_link_libraries(toycompiler PRIVATE LLVM project_options project_warnings)
message(STATUS "LLVM linked to: ${llvm_libs}")
