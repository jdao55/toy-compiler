add_executable(toycompiler misc/test.cpp lexer/lexer.cpp AST/AST.cpp)

# Link against LLVM libraries
target_link_libraries(toycompiler PRIVATE LLVM CONAN_PKG::fmt CONAN_PKG::docopt.cpp project_options project_warnings)
message(STATUS "LLVM linked to: ${llvm_libs}")
