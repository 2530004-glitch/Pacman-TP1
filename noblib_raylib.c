/*
    For git repo: https://github.com/raysan5/raylib.git
    git commit:  688a81d
    This one should work with 5.6 dev+ usually. I even tested with other versions of raylib 5.0. 
    If you have bugs, report them to the specified authorities, that's me.
*/
#ifndef NOB_H_
#define NOB_STRIP_PREFIX
#include "nob.h"
#endif


static const char *raylib_modules[] = {
    "rcore",
    "raudio",
    "rglfw",
    "rmodels",
    "rshapes",
    "rtext",
    "rtextures",
    "utils",
};

#ifndef RAYLIB_PATH
#define RAYLIB_PATH "Libraries/raylib"
#endif

#ifndef BUILD_FOLDER
#define BUILD_FOLDER "build"
#endif

const char* RAYLIB_FILE = __FILE__;
#define RAYLIB_INCLUDES "-I"RAYLIB_PATH "/src", "-I"RAYLIB_PATH "/src/external/glfw/include"
#ifdef _WIN32
    #define RAYLIB_LFLAGS "-lm", "-lgdi32","-lwinmm"
#else
    #define RAYLIB_LFLAGS "-lm"
#endif


#define RAYLIB_BUILD_PATH BUILD_FOLDER"/raylib"
bool build_raylib(File_Paths* end_cmd)
{
    bool result = true;
    Cmd cmd = {0};

    if (!mkdir_if_not_exists(RAYLIB_BUILD_PATH)) {
        return_defer(false);
    }

    Procs procs = {0};
    for (size_t i = 0; i < ARRAY_LEN(raylib_modules); ++i) {
        const char *input_path = temp_sprintf(RAYLIB_PATH"/src/%s.c", raylib_modules[i]);
        const char *output_path = temp_sprintf("%s/%s.o", RAYLIB_BUILD_PATH, raylib_modules[i]);
        output_path = temp_sprintf("%s/%s.o", RAYLIB_BUILD_PATH, raylib_modules[i]);
        cmd_append((Nob_Cmd*)end_cmd,output_path);

        if (needs_rebuild(output_path, &input_path, 1)) {
            cmd.count = 0;
            nob_cc(&cmd);
            cmd_append(&cmd, "-ggdb3");
            #ifdef __ZIGC__
                cmd_append(&cmd, "-fno-sanitize=undefined","-fno-omit-frame-pointer");
                cmd_append(&cmd, "-target");
                #ifdef _WIN32
                cmd_append(&cmd, "x86_64-windows");
                #endif
                #ifdef __linux__
                cmd_append(&cmd, "x86_64-linux-gnu");
                #endif
            #endif //__ZIGC__
            
            #ifdef __linux__
            cmd_append(&cmd, "-D_GLFW_X11");
            cmd_append(&cmd, "-lX11");
            cmd_append(&cmd, "-I/usr/include");
            cmd_append(&cmd, "-L/usr/lib");
            #endif
            cmd_append(&cmd, "-DPLATFORM_DESKTOP","-fPIC");
            cmd_append(&cmd, RAYLIB_INCLUDES);
            cmd_append(&cmd, "-c", input_path);
            cmd_append(&cmd, "-o", output_path);

            Proc proc = nob_cmd_run_async(cmd);
            da_append(&procs, proc);
        }
    }
    cmd.count = 0;

    if (!procs_wait(procs)) return_defer(false);

defer:
    cmd_free(cmd);
    return result;
}
