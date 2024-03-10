@echo off

setlocal

set libsdir=C:\dev\shared\libs
set incdir=C:\dev\shared\include

set rootdir=%~dp0
set bindir=%rootdir%bin\release
set srcdir=%rootdir%src

set copt_lib=/I%incdir% /LD /nologo /FC /GR- /O2 /Oi
set copt_game=/I%incdir% /MT /nologo /FC /GR- /O2 /Oi

set cwopt=/WX /W4 /wd4201 /wd4100 /wd4189 /wd4505 /wd4127

set lopt=/libpath:%libsdir% /opt:ref /incremental:no /subsystem:console

set llib_lib=SDL2_mixer.lib SDL2.lib glad-dll.lib SDL2_image.lib SDL2_ttf.lib winmm.lib  Shcore.lib
set llib_game=glad-dll.lib sav_lib.lib

pushd %bindir%

cl %srcdir%\sav_lib.cpp %copt_lib% %cwopt% /DSAV_EXPORTS /link %llib_lib% %lopt% /out:sav_lib.dll

cl %srcdir%\sou_release.cpp %srcdir%\souterrain.cpp %copt_game% %cwopt% /link %llib_game% %lopt% /out:souterrain.exe

popd

if %errorlevel% neq 0 (exit /b %errorlevel%)
