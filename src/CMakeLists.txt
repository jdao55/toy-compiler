
add_executable(toycompiler toy.cpp)

llvm_map_components_to_libnames(llvm_libs core support)

# Link against LLVM libraries
target_link_libraries(toycompiler ${llvm_libs})
message(STATUS "LLVM linked to: ${llvm_libs}")
