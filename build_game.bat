@echo off

setlocal

set libsdir=C:\dev\shared\libs
set incdir=C:\dev\shared\include

set rootdir=%~dp0
set bindir=%rootdir%bin
set srcdir=%rootdir%src

set copt=/IC:\dev\shared\include /MDd /LDd /Z7 /Od /Oi /FC /GR- /nologo /Ob1
set cwopt=/WX /W4 /wd4201 /wd4100 /wd4189 /wd4505 /wd4127 /wd4702
set llib=glad-dll.lib sav_lib.lib
set lopt=/libpath:%libsdir% /debug /opt:ref /incremental:no

pushd %bindir%

copy nul souterrain.lock

rem Delete all pdbs if can (can't while still running - visual studio locks them all). And quiet the del "access is denied" output
del souterrain_*.pdb > nul 2> nul

cl %srcdir%\souterrain.cpp %copt% %cwopt% /DSAV_DEBUG /link %llib% %lopt% /pdb:souterrain_%random%.pdb

del souterrain.lock

popd

if %errorlevel% neq 0 (exit /b %errorlevel%)
