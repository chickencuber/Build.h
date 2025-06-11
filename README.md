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
    Build.build("main", StringArray("main.c"), 1, FlagArray(), 0);
    return 0;
}
```

another example can be found in the build.c file

## Features
* cross platform build system with windows*(untested), linux, and mac*(untested) support
* simple api
* gcc and clang compiler support
* simple flag support
## Planned 
- [x] MSVC support(untested)
- [ ] including dependencies from a remote source
    - [x] implement remote fetching(requires git)
    - [ ] add autobuilding(not sure how Ill do this one)

- [x] cross platform fs api
- [ ] more compilers than just gcc, clang, and msvc

