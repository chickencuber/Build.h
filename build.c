#define BUILD_IMPLEMENTATION 
#include "./build.h"

int main() { 
    string deps[] = {"./main.c"};
    Build.build("main", deps, 1);
    return 0;
}
