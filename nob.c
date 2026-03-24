#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

#include "noblib_raylib.c"

#define LAB_NAME "entrypoint"

int main(int argc, char** argv)
{
    NOB_GO_REBUILD_URSELF_PLUS(argc, argv, RAYLIB_FILE);

    if (!mkdir_if_not_exists("Deployment")) return 1;
    if (!mkdir_if_not_exists("build")) return 1;

    File_Paths o_files = {0};
    if (!build_raylib(&o_files)) return 1;

    Cmd cmd = {0};
    nob_cc(&cmd);
    cmd_append(&cmd, "-std=c++17", "-ggdb3");
    nob_cc_inputs(&cmd,
                  "./src/main.cpp",
                  temp_sprintf("./src/%s.cpp", LAB_NAME),
                  "./src/Game.cpp",
                  "./src/GameManager.cpp",
                  "./src/GameObject.cpp",
                  "./src/PacMan.cpp",
                  "./src/Ghost.cpp",
                  "./src/Pellet.cpp");
    cmd_append(&cmd, RAYLIB_INCLUDES);
    for (int i = 0; i < o_files.count; ++i)
    {
        cmd_append(&cmd, o_files.items[i]);
    }
    nob_cc_output(&cmd, "./Deployment/game.exe");
    cmd_append(&cmd, RAYLIB_LFLAGS, "-lstdc++");

    if (!cmd_run_sync_and_reset(&cmd)) return 1;
    return 0;
}
