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
    void (*build)(string file, string dep[], size_t dep_length, string flag[], size_t flag_length);
    bool (*str_ends_with)(string str, string suffix);
    struct {
        bool (*exists)(string path);
        bool (*is_file)(string path);
        bool (*is_dir)(string path);
        void (*copy)(string from, string to);
        void (*move)(string from, string to);
        void (*mkdir)(string path);
        void (*remove)(string path);
    } fs;
} Build;

// implementation
#ifdef BUILD_IMPLEMENTATION

#ifdef _WIN32
void __Build_Switch_New__() {
    system("start \"\" /B cmd /C \"timeout /t 1 >nul && move /Y build.new build.exe && build.exe\"");
    exit(0);
}
bool __Build_needs_rebuild__(string output, string sources[], size_t n) {
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
bool __BUILD__FS_exists(string file) {
    DWORD attr = GetFileAttributesA(file);
    return (attr != INVALID_FILE_ATTRIBUTES);
}

bool __BUILD__FS_is_file(string path) {
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES) && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

bool __BUILD__FS_is_dir(string path) {
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

void __BUILD__FS_copy(string from, string to) {
    CopyFileA(from, to, FALSE);
}

void __BUILD__FS_fs_move(string from, string to) {
    MoveFileExA(from, to, MOVEFILE_REPLACE_EXISTING);
}

void __BUILD__FS_mkdir(string path) {
    CreateDirectoryA(path, NULL);
}

void __BUILD__FS_remove(string path) {
    if (__BUILD__FS_is_dir(path)) {
        RemoveDirectoryA(path);
    } else {
        DeleteFileA(path);
    }
}
#else
bool __BUILD__FS_exists(string file) {
    struct stat st;
    return stat(file, &st) == 0;
}
bool __BUILD__FS_is_file(string path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}
bool __BUILD__FS_is_dir(string path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}
void __BUILD__FS_copy(string from, string to) {
    char cmd[BufferSize];
    snprintf(cmd, sizeof(cmd), "cp \"%s\" \"%s\"", from, to);
    system(cmd);
}
void __BUILD__FS_fs_move(string from, string to) {
    rename(from, to);
}
void __BUILD__FS_mkdir(string path) {
    mkdir(path, 0755);
}
void __BUILD__FS_remove(string path) {
    unlink(path);
}

void __Build_Switch_New__() {
    sleep(1);
    rename("./build.new", "./build");
    char *new_argv[] = {"./build", NULL};
    execvp(new_argv[0], new_argv);
    perror("execvp failed");
    exit(1);
}
bool __Build_needs_rebuild__(string output, string sources[], size_t n) {
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

bool __Build_Ends_With__(string str, string suffix) {
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
        Build.build("build.new", deps, 2, (string[]) {}, 0); 
        __Build_Switch_New__();
    } else {
        printf("not rebuilding build\n");
    }
}

#ifdef __GNUC__
void __Build_Build__(string file, string dep[], size_t dep_length, string flags[], size_t flag_length) {
    if(!__Build_needs_rebuild__(file, dep, dep_length)) {
        printf("not rebuilding %s\n", file);
        return;
    }
    char cmd[BufferSize] = {'\0'};
#ifdef __clang__
    strcat(cmd, "clang ");
#else
    strcat(cmd, "gcc ");
#endif
    for(size_t i = 0; i < flag_length; i++) {
        strcat(cmd, "-");
        strcat(cmd, flags[i]);
        strcat(cmd, " ");
    }
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
void __Build_Build__(string file, string dep[], size_t dep_length, string flags[], size_t flag_length) {
    fprintf(stderr, "\033[31munknown compiler\033[0m\n");
    exit(1);
}
#endif

int __Build_Main__(int argc, char **argv);
#define main(...) \
    main(int argc, char **argv) { \
        Build.build =  __Build_Build__; \
        Build.str_ends_with = __Build_Ends_With__; \
        Build.fs.mkdir = __BUILD__FS_mkdir; \
        Build.fs.exists = __BUILD__FS_exists; \
        Build.fs.is_dir = __BUILD__FS_is_dir; \
        Build.fs.is_file = __BUILD__FS_is_file; \
        Build.fs.copy = __BUILD__FS_copy; \
        Build.fs.move = __BUILD__FS_fs_move; \
        Build.fs.remove = __BUILD__FS_remove; \
        __Build_Bootstrap__(); \
        return __Build_Main__(argc, argv); \
    }; \
int __Build_Main__(int argc, char **argv)

#endif
