@echo off

setlocal

set libsdir=C:\dev\shared\libs
set incdir=C:\dev\shared\include

set rootdir=%~dp0
set bindir=%rootdir%bin
set srcdir=%rootdir%src

set copt=/IC:\dev\shared\include /MDd /LDd /Z7 /Od /Oi /FC /GR- /nologo /Ob1
set cwopt=/WX /W4 /wd4201 /wd4100 /wd4189 /wd4505 /wd4127
set llib=SDL2_mixer.lib SDL2.lib glad-dll.lib SDL2_image.lib SDL2_ttf.lib winmm.lib
set lopt=/libpath:%libsdir% /debug /opt:ref /incremental:no

pushd %bindir%

rem /section:.shared,rw 
cl %srcdir%\sav_lib.cpp %copt% %cwopt% /DSAV_EXPORTS /DSAV_DEBUG /link %llib% %lopt%

popd

if %errorlevel% neq 0 (exit /b %errorlevel%)
