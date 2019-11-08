@echo off
set MAKEFILE=%~1
set TARGET_BUILD=%~2
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
nmake -nologo -f %MAKEFILE% %TARGET_BUILD%
