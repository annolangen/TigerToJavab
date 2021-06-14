# TigerToJavab
a personal extension/modification on cs350, but done with the standard library and going to java bytecode

## Getting Started

The development environment requires certain binaries, like a C++
compiler. The following list of packages is known to provide a working
environment on Debian derived systems.

```
sudo apt install clang make automake flex default-jdk-headless
```

In a freshly cloned repository you need to create initial Makefiles by
executing in the root directory

```
autoreconf --install --force
./configure
```

From that point forward `make` builds the compiler and `make check` runs
tests.

## File Organization

- src holds source code for the compiler and its tests
- src/testing holds testing infrastructure code
