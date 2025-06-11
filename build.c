#define BUILD_IMPLEMENTATION 
#include "./build.h"

int main() { 
    if(!Build.fs.exists("./target/")) {
        Build.fs.mkdir("./target/");
    }
    Build.fetch_git("https://github.com/tsoding/nob.h.git", false); //not an actual dep, just an example, the second arg does nothing at the moment, but for future proofing its best to make it false
    Build.build("target/main.o", StringArray("main.c"), 1, FlagArray(FLAG_COMPILE_ONLY), 1);
    Build.build("target/other.o", StringArray("other.c"), 1, FlagArray(FLAG_COMPILE_ONLY), 1);
    Build.build("main", StringArray("target/main.o", "target/other.o"), 2, FlagArray(), 0);
    return 0;
}
