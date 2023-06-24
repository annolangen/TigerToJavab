# TigerToJavab

a personal extension/modification on cs350, but done with the standard library and going to java bytecode

## Getting Started

The development environment requires certain binaries, like a C++
compiler. The following list of packages is known to provide a working
environment on Debian derived systems.

```
sudo apt install clang make cmake bison flex default-jdk-headless
```

To build from scratch, run the following commands:

```
cmake -B build -S .
make -C build
```

From that point forward `make` builds the compiler and `make check` runs
tests.

## File Organization

- src holds source code for the compiler and its tests
- src/testing holds testing infrastructure code
