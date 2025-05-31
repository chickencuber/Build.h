// types
#ifdef BUILD_IMPLEMENTATION
#define EXTERN
#else
#define EXTERN extern
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

typedef const char* string;

#define BufferSize 1024

EXTERN struct {
    void (*build)(string file, string dep[], size_t dep_length);
    bool (*str_ends_with)(string str, string suffix);
} Build;

// implementation
#ifdef BUILD_IMPLEMENTATION

#ifdef _WIN32
void __Build_Switch_New__() {
    // We need to run a command to rename the file, then launch the new executable
    system("start \"\" /B cmd /C \"timeout /t 1 >nul && move /Y build.new build.exe && build.exe\"");
    exit(0); // Exit the current process
}
bool __Build_needs_rebuild__(const char* output, const char* sources[], size_t n) {
    HANDLE outFile = CreateFileA(output, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (outFile == INVALID_HANDLE_VALUE) return true;

    FILETIME outTime;
    GetFileTime(outFile, NULL, NULL, &outTime);
    CloseHandle(outFile);

    for (size_t i = 0; i < n; i++) {
        HANDLE srcFile = CreateFileA(sources[i], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (srcFile == INVALID_HANDLE_VALUE) return true;

        FILETIME srcTime;
        GetFileTime(srcFile, NULL, NULL, &srcTime);
        CloseHandle(srcFile);

        if (CompareFileTime(&srcTime, &outTime) == 1) return true; // src newer than out
    }

    return false;
}
#else
void __Build_Switch_New__() {
    sleep(1);
    rename("./build.new", "./build");
    char *new_argv[] = {"./build", NULL};
    execvp(new_argv[0], new_argv);
    perror("execvp failed");
    exit(1);
}
bool __Build_needs_rebuild__(const char* output, const char* sources[], size_t n) {
    struct stat out_stat;
    if (stat(output, &out_stat) != 0) return true;
    for (size_t i = 0; i < n; i++) {
        struct stat src_stat;
        if (stat(sources[i], &src_stat) != 0) return true; 
        if (src_stat.st_mtime > out_stat.st_mtime) return true;
    }
    return false;
}
#endif

bool __Build_Ends_With__(const char *str, const char *suffix) {
    if (!str || !suffix)
        return false;

    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);

    if (suffix_len > str_len)
        return false;

    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

void __Build_Bootstrap__() { 
    string deps[] = {
        "build.c",
        "build.h",
    };
    if(__Build_needs_rebuild__("./build", deps, 2)) {
        Build.build("build.new", deps, 2); 
        __Build_Switch_New__();
    } else {
        printf("not rebuilding build\n");
    }
}

#ifdef __clang__
void __Build_Build__(string file, string dep[], size_t dep_length) {
    if(!__Build_needs_rebuild__(file, dep, dep_length)) {
        printf("not rebuilding %s\n", file);
        return;
    }
    char cmd[BufferSize] = {'\0'};
    strcat(cmd, "clang ");
    for(size_t i = 0; i < dep_length; i++) {
        if(!Build.str_ends_with(dep[i], ".c")){ //ignore anything that isnt a .c
            continue;
        }
        strcat(cmd, dep[i]);
        strcat(cmd, " ");
    }
    strcat(cmd, "-o ");
    strcat(cmd, file);
    printf("running cmd %s\n", cmd);
    system(cmd);
    printf("done\n");
}
#elif defined(__GNUC__)
void __Build_Build__(string file, string dep[], size_t dep_length) {
    if(!__Build_needs_rebuild__(file, dep, dep_length)) {
        printf("not rebuilding %s\n", file);
        return;
    }
    char cmd[BufferSize] = {'\0'};
    strcat(cmd, "gcc ");
    for(size_t i = 0; i < dep_length; i++) {
        if(Build.str_ends_with(dep[i], ".h") || Build.str_ends_with(dep[i], ".hpp")){ //ignore anything that isnt a .c
            continue;
        }
        strcat(cmd, dep[i]);
        strcat(cmd, " ");
    }
    strcat(cmd, "-o ");
    strcat(cmd, file);
    printf("running cmd %s\n", cmd);
    system(cmd);
    printf("done\n");
}
#else 
void __Build_Build__(string file, string dep[], long dep_length) {
    fprintf(stderr, "\033[31munknown compiler\033[0m\n");
    exit(1);
}
#endif

int __Build_Main__(int argc, char **argv);
#define main(...) \
    main(int argc, char **argv) { \
        Build.build =  __Build_Build__; \
        Build.str_ends_with = __Build_Ends_With__; \
        __Build_Bootstrap__(); \
        return __Build_Main__(argc, argv); \
    }; \
int __Build_Main__(int argc, char **argv)

#endif
