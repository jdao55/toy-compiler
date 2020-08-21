# toy-compiler
A toy (Kaleidoscope) compiler based on llvm's guide https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/index.html

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
      
