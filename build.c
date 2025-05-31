#define BUILD_IMPLEMENTATION 
#include "./build.h"

int main() { 
    Build.build("./target/main.o", (string[]){"./main.c"}, 1, (string[]) {"c"}, 1);
    Build.build("./target/other.o", (string[]){"./other.c"}, 1, (string[]) {"c"}, 1);
    Build.build(
            "./main", 
            (string[]) {
            "./target/main.o",
            "./target/other.o",
            }, 
            2, 
            (string[]){}, 0
            );
    return 0;
}
