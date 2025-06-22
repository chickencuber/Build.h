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
#define _OBJ ".obj"
#define _EXE ".exe"
#define ROOT "C:/"
#else
#include <unistd.h>
#include <sys/stat.h>
#define _OBJ ".o"
#define _EXE ""
#define ROOT "/"
#endif


#define EXECUTABLE(name) name _EXE
#define OBJECT(name) name _OBJ

typedef const char* string;

#define BufferSize 1024

typedef enum {
    __FLAG_OPTIMIZE_SPEED,
    __FLAG_OPTIMIZE_SIZE,
    __FLAG_DEBUG,
    __FLAG_WARNINGS_LEVEL_3,
    __FLAG_WARNINGS_LEVEL_4,
    __FLAG_INCLUDE_PATH,
    __FLAG_DEFINE_MACRO,
    __FLAG_COMPILE_ONLY,
    __FLAG_LANGUAGE_STANDARD,
    __FLAG_RAW,//not cross platform
} FlagType;

#define FLAG_OPTIMIZE_SPEED     (Flag){ .type = __FLAG_OPTIMIZE_SPEED }
#define FLAG_OPTIMIZE_SIZE (Flag){ .type = __FLAG_OPTIMIZE_SIZE }
#define FLAG_DEBUG (Flag){ .type = __FLAG_DEBUG }
#define FLAG_WARNINGS_LEVEL_3 (Flag){ .type = __FLAG_WARNINGS_LEVEL_3 }
#define FLAG_WARNINGS_LEVEL_4 (Flag){ .type = __FLAG_WARNINGS_LEVEL_4 }
#define FLAG_COMPILE_ONLY  (Flag){ .type = __FLAG_COMPILE_ONLY }

#define FLAG_RAW(raw_flag) (Flag){.type=__FLAG_RAW, .str_value=raw_flag}
#define FLAG_INCLUDE_PATH(path) (Flag){ .type = __FLAG_INCLUDE_PATH, .str_value = path }
#define FLAG_DEFINE_MACRO(macro) (Flag){ .type = __FLAG_DEFINE_MACRO, .str_value = macro }
#define FLAG_LANGUAGE_STANDARD(std) (Flag){ .type = __FLAG_LANGUAGE_STANDARD, .str_value = std }

#define MAX_FLAG_STRINGS 4 // one flag can expand to multiple strings

typedef struct {
    FlagType type;
    string str_value;
} Flag;

typedef struct {
    string data[MAX_FLAG_STRINGS];
    size_t count;
} FlagStringList;

#define StringArray(...) ((string[]) {__VA_ARGS__})
#define FlagArray(...) ((Flag[]) {__VA_ARGS__})


EXTERN struct {
    void (*build)(string file, string dep[], size_t dep_length, Flag flags[], size_t flag_length);
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
    void (*fetch_git)(string url, bool build); //build does nothing at the moment
} Build;

// implementation
#ifdef BUILD_IMPLEMENTATION

#ifdef _WIN32
void __Build_Switch_New__() {
    system("start \"\" /B cmd /C \"timeout /t 1 >nul && move /Y build.new.exe build.exe && build.exe\"");
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

        if (CompareFileTime(&srcTime, &outTime) == 0) return true; // src newer than out
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
        "./build.c",
        "./build.h",
    };
    if(__Build_needs_rebuild__(EXECUTABLE("./build"), deps, 2)) {
        Build.build(EXECUTABLE("./build.new"), deps, 2, (Flag[]) {}, 0); 
        __Build_Switch_New__();
    } else {
        printf("not rebuilding build\n");
    }
}

#ifdef __GNUC__
FlagStringList flag_to_strings(Flag flag) {
    FlagStringList result = {0};
    switch (flag.type) {
        case __FLAG_OPTIMIZE_SPEED:
            result.data[0] = "-O2";
            result.count = 1;
            break;
        case __FLAG_OPTIMIZE_SIZE:
            result.data[0] = "-Os";
            result.count = 1;
            break;
        case __FLAG_DEBUG:
            result.data[0] = "-g";
            result.count = 1;
            break;
        case __FLAG_WARNINGS_LEVEL_3:
            result.data[0] = "-Wall";
            result.count = 1;
            break;
        case __FLAG_WARNINGS_LEVEL_4:
            result.data[0] = "-Wall";
            result.data[1] = "-Wextra";
            result.count = 2;
            break;
        case __FLAG_INCLUDE_PATH:
            result.data[0] = "-I";
            result.data[1] = flag.str_value;
            result.count = 2;
            break;
        case __FLAG_DEFINE_MACRO:
            result.data[0] = "-D";
            result.data[1] = flag.str_value;
            result.count = 2;
            break;
        case __FLAG_COMPILE_ONLY:
            result.data[0] = "-c";
            result.count = 1;
            break;
        case __FLAG_LANGUAGE_STANDARD:
            result.data[0] = "-std";
            result.data[1] = flag.str_value; // e.g., "c99", "gnu11"
            result.count = 2;
            break;
        case __FLAG_RAW:
            result.count = 1;
            result.data[0] = flag.str_value;
            break;
        default:
            // ignore unknown flags silently
            break;
    }
    return result;
}
void __Build_Build__(string file, string dep[], size_t dep_length, Flag flags[], size_t flag_length) {
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
        FlagStringList f = flag_to_strings(flags[i]);
        for(size_t ii = 0; ii < f.count; ii++) {
            strcat(cmd, f.data[ii]);
            strcat(cmd, " ");
        }
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
#elif defined(_MSC_VER)

FlagStringList flag_to_strings(Flag flag, bool* comp_only) {
    FlagStringList result = {0};
    switch (flag.type) {
        case __FLAG_OPTIMIZE_SPEED:
            result.data[0] = "/O2";    // optimize for speed
            result.count = 1;
            break;
        case __FLAG_OPTIMIZE_SIZE:
            result.data[0] = "/O1";    // optimize for size
            result.count = 1;
            break;
        case __FLAG_DEBUG:
            result.data[0] = "/Zi";    // generate debug info
            result.count = 1;
            break;
        case __FLAG_WARNINGS_LEVEL_3:
            result.data[0] = "/W3";    // warning level 3
            result.count = 1;
            break;
        case __FLAG_WARNINGS_LEVEL_4:
            result.data[0] = "/W4";    // warning level 4 (more verbose)
            result.count = 1;
            break;
        case __FLAG_INCLUDE_PATH:
            result.data[0] = "/I";
            result.data[1] = flag.str_value;  // path right after /I
            result.count = 2;
            break;
        case __FLAG_DEFINE_MACRO:
            result.data[0] = "/D";
            result.data[1] = flag.str_value;
            result.count = 2;
            break;
        case __FLAG_COMPILE_ONLY:
            result.data[0] = "/c";     // compile only, don’t link
            *comp_only = true;
            result.count = 1;
            break;
        case __FLAG_LANGUAGE_STANDARD:
            // MSVC doesn't support setting C standard like gcc/clang;
            // typically defaults to latest supported or use /std:c11 (VS2019+)
            // so we can do a simple check or ignore for older
            if (strcmp(flag.str_value, "c11") == 0) {
                result.data[0] = "/std:c11";
                result.count = 1;
            } else if (strcmp(flag.str_value, "c17") == 0) {
                result.data[0] = "/std:c17";
                result.count = 1;
            } else {
                // default or ignore for unsupported standards
                result.count = 0;
            }
            break;
        case __FLAG_RAW:
            result.count = 1;
            result.data[0] = flag.str_value;
            break;
        default:
            // unknown flags ignored silently
            break;
    }
    return result;
}
void __Build_Build__(string file, string dep[], size_t dep_length, Flag flags[], size_t flag_length) {
    if(!__Build_needs_rebuild__(file, dep, dep_length)) {
        printf("not rebuilding %s\n", file);
        return;
    }
    bool comp_only = false;
    char cmd[BufferSize] = {'\0'};
    strcat(cmd, "cl ");
    for(size_t i = 0; i < flag_length; i++) {
        FlagStringList f = flag_to_strings(flags[i], &comp_only);
        for(size_t ii = 0; ii < f.count; ii++) {
            strcat(cmd, f.data[ii]);
        }
        strcat(cmd, " ");
    }
    for(size_t i = 0; i < dep_length; i++) {
        if(Build.str_ends_with(dep[i], ".h") || Build.str_ends_with(dep[i], ".hpp")){ //ignore anything that isnt a .c
            continue;
        }
        strcat(cmd, dep[i]);
        strcat(cmd, " ");
    }
    if(comp_only) {
        strcat(cmd, "/FAsc");
    } else {
        strcat(cmd, "/Fe");
    }
    strcat(cmd, file);
    printf("running cmd %s\n", cmd);
    system(cmd);
    printf("done\n");
}
#else
void __Build_Build__(string file, string dep[], size_t dep_length, Flag flags[], size_t flag_length) {
    fprintf(stderr, "\033[31munknown compiler\033[0m\n");
    exit(1);
}
#endif

void __Build_fetch_git(string url, bool build) { //build does nothing at the moment
    if(!Build.fs.exists("./deps/")) {
        Build.fs.mkdir("./deps/");
    }
    printf("fetching %s\n", url);
    char cmd[BufferSize] = {'\0'};
    strcat(cmd, "git -C ./deps/ clone --depth=1 --single-branch ");
    strcat(cmd, url);
    system(cmd);
    printf("done\n");
}

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
        Build.fetch_git = __Build_fetch_git; \
        __Build_Bootstrap__(); \
        return __Build_Main__(argc, argv); \
    }; \
int __Build_Main__(int argc, char **argv)

#endif
