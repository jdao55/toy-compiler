add_executable(toycompiler toy.cpp)

# Link against LLVM libraries
target_link_libraries(toycompiler PRIVATE LLVM CONAN_PKG::fmt project_options project_warnings)
message(STATUS "LLVM linked to: ${llvm_libs}")
