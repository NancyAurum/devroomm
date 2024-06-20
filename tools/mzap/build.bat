@echo off
set PATH=C:\Program Files\mingw-w64\x86_64-6.3.0-posix-seh-rt_v5-rev2\mingw64\bin;%PATH%
gcc %cflags% -D WINDOWS -o mzap ./mzap.c -lmingw32
