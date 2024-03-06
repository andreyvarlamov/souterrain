@echo off

setlocal

set libsdir=C:\dev\shared\libs
set incdir=C:\dev\shared\include

set rootdir=%~dp0
set bindir=%rootdir%bin
set srcdir=%rootdir%src

set copt=/TP /IC:\dev\shared\include /MDd /LDd /Z7 /Od /Oi /FC /GR- /nologo /Ob1
set cwopt=/WX /W4 /wd4201 /wd4100 /wd4189 /wd4505 /wd4127
set llib=sav_lib.lib
set lopt=/libpath:%libsdir% /debug /opt:ref /incremental:no /noexp /dynamicbase:no /fixed /base:0x190000000

pushd %bindir%

rem /section:.shared,rw 
cl %srcdir%\sou_templates.h %copt% %cwopt% /DTEMPLATE_EXPORTS /link %llib% %lopt%

popd

if %errorlevel% neq 0 (exit /b %errorlevel%)
