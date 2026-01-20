@echo off
IF NOT EXIST "Tools\w64devkit" (
    .\Tools\wget.com https://github.com/skeeto/w64devkit/releases/download/v2.5.0/w64devkit-x64-2.5.0.7z.exe
    w64devkit-x64-2.5.0.7z.exe -o./Tools/ -y
    del .\w64devkit-x64-2.5.0.7z.exe
)