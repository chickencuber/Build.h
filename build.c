#define BUILD_IMPLEMENTATION 
#include "./build.h"

int main() { 
    if(!Build.fs.exists("./target/")) {
        Build.fs.mkdir("./target/");
    }
    Build.fetch_git("https://github.com/tsoding/nob.h.git"); //not an actual dep, just an example
    Build.build("target/main.o", StringArray("main.c"), 1, FlagArray(FLAG_COMPILE_ONLY), 1);
    Build.build("target/other.o", StringArray("other.c"), 1, FlagArray(FLAG_COMPILE_ONLY), 1);
    Build.build("main", StringArray("target/main.o", "target/other.o"), 2, FlagArray(), 0);
    return 0;
}
