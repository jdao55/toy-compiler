# toy-compiler
A toy (Kaleidoscope) compiler based on llvm's guide https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/index.html, 
currently compiles to object files, which can be linked using clang/gcc

# Building
## Dependencies
- cmake
- C++ 20 compiler 
- conan
- Flex
- llvm
## Build using Cmake
1. Clone dirrectory 
``` 
git clone https://github.com/jdao55/toy-compiler && cd toy-compiler
```
2. Create build directory
```
mkdir build && cd build
```
3. Run cmake
```
cmake ../
```
4. Build using make 
```
make
```
# Usage
```
Usage:
  toycomp <filename> [--out=filename] [--opt=level]
  toycomp (-h | --help)

Options:
  -h --help                       Show this screen.
  -o filname --out=filename       Specify output object file name
  -O level --opt=level            Specify optimization level [1,2,3])";
```
# Example
Kaleidoscope program test.toy
```python
def add(x y)
   x + y

def add3(x y z)
   x + y + z
```
Compile with toycompiler
```toycompiler test.toy -out=add.o```

## Calling Kaleidoscope from c++
sample c++ program example.cpp
```C++
#include <iostream>
extern "C" {
  double add(double, double);
  double add3(double, double, double);
}
int main()

{
  std::cout << add(1.2, 2.3) << std::endl;
  std::cout << add3(1.0, 2.0, 3.0) << std::endl;
}
```
Compile and link using gcc/clang
```g++ example.cpp add.o -o example```
# TODO
## Language features
- arrays
- data types (WIP see [types-ext branch](https://github.com/jdao55/toy-compiler/tree/types-ext))
  - integers
  - strings
- arrays
- main function
## Compiler features
- add optimization passes (WIP [see optimzer branch](https://github.com/jdao55/toy-compiler/tree/optimizer))
- add compilation targets
- refactor main?
      
