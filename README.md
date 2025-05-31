# Build.h
## What is Build.h
build.h is a header only library that acts as a simple build system inspired by [nob.h](https://www.github.com/tsoding/nob.h)
## Why make a header only C build system 
because I can
## How to use Build.h
add build.h to your projects directory, define `BUILD_IMPLEMENTATION`, and include build.h
```C
#define BUILD_IMPLEMENTATION 
#include "./build.h"

int main() { 
    string deps[] = {"./main.c"};
    Build.build("main", deps, 1);
    return 0;
}
```

## Features
* cross platform build system with windows*(untested, and no MSVC yet), linux, and mac*(untested) support
* simple api
* gcc and clang compiler support
## Planed 
- [ ] MSVC support
- [ ] including files from a remote source
- [ ] cross platform fs api
- [ ] more compilers than just gcc and clang

