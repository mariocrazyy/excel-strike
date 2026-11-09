@echo off
SETLOCAL EnableDelayedExpansion

set CC=zig c++
set BINARY=excel.exe
set TARGET=x86_64-windows-gnu

set OUTDIR=x64\%TARGET%
set OUT=%OUTDIR%\%BINARY%

if not exist "src\external\lib\libLIBCMT.a" goto :gen_libs
if not exist "src\external\lib\libOLDNAMES.a" goto :gen_libs
goto :skip_gen

:gen_libs
echo generating compat libs
%CC% -c scripts\msvc\compatibility.cpp -o msvc_compat.obj
zig ar rcs src\external\lib\libLIBCMT.a msvc_compat.obj
zig ar rcs src\external\lib\libOLDNAMES.a msvc_compat.obj
del msvc_compat.obj

:skip_gen

set INCLUDES=-Isrc ^
             -Isrc/external ^
             -Isrc/external/imgui ^
             -Isrc/external/json/include ^
             -Isrc/external/curl/include ^
             -Isrc/external/AsyncLogger/src ^
             -Isrc/external/AsyncLogger/include/AsyncLogger

set DEFINES=-D_CONSOLE -DIMGUI_DEFINE_MATH_OPERATORS -DCURL_STATICLIB -D_WIN32_WINNT=0x0a00

:: change -ld3dcompiler_47 to a different version if needed
set LIBS=src/external/lib/libcurl.lib ^
         src/external/lib/zlib.lib ^
         -ld3d11 -ldxgi -ld3dcompiler_47 -ldwmapi -lgdi32 -luser32 -lshell32 -limm32 -lole32 ^
         -lws2_32 -lwldap32 -lcrypt32 -lnormaliz -ladvapi32 -lbcrypt

set SRCS=scripts\msvc\compatibility.cpp ^
         src/common.cpp ^
         src/config/Config.cpp ^
         src/core/engine/cache/Cache.cpp ^
         src/core/engine/classes/Bomb.cpp ^
         src/core/engine/Engine.cpp ^
         src/core/engine/classes/Game.cpp ^
         src/core/engine/classes/Globals.cpp ^
         src/core/engine/classes/Player.cpp ^
         src/core/engine/classes/Weapon.cpp ^
         src/core/engine/classes/ObserverServices.cpp ^
         src/core/logger/LogHelper.cpp ^
         src/core/memory/Memory.cpp ^
         src/core/offsets/Dumper.cpp ^
         src/external/AsyncLogger/src/LogCapture.cpp ^
         src/external/AsyncLogger/src/Logger.cpp ^
         src/external/AsyncLogger/src/LogIntermediate.cpp ^
         src/external/AsyncLogger/src/LogMessage.cpp ^
         src/external/AsyncLogger/src/LogStream.cpp ^
         src/external/imgui/backends/imgui_impl_dx11.cpp ^
         src/external/imgui/backends/imgui_impl_win32.cpp ^
         src/external/imgui/imgui.cpp ^
         src/external/imgui/imgui_demo.cpp ^
         src/external/imgui/imgui_draw.cpp ^
         src/external/imgui/imgui_tables.cpp ^
         src/external/imgui/imgui_widgets.cpp ^
         src/external/timer/timer.cpp ^
         src/gui/frontend/esp/Esp.cpp ^
         src/gui/frontend/menu/Menu.cpp ^
         src/gui/frontend/overlays/Overlays.cpp ^
         src/gui/renderer/Renderer.cpp ^
         src/gui/renderer/window/Window.cpp ^
         src/main.cpp ^
         src/updater/http/HttpHelper.cpp ^
         src/updater/Updater.cpp

mkdir "%OUTDIR%" 2>nul

echo compiling resource
windres excel.rc -O coff -o "%OUTDIR%\excel.res"
if errorlevel 1 goto :build_failed

echo compiling project
%CC% -target %TARGET% -std=c++20 -O3 -Wno-date-time -fno-autolink -include src/common.hpp %DEFINES% %INCLUDES% %SRCS% "%OUTDIR%\excel.res" -o "%OUT%" %LIBS% -Lsrc/external/lib
set BUILD_RESULT=%ERRORLEVEL%
if exist "%OUTDIR%\excel.res" del /Q "%OUTDIR%\excel.res"
if not "%BUILD_RESULT%"=="0" goto :build_failed

for %%F in ("%OUTDIR%\*") do (
    if exist "%%~fF" (
        if /I not "%%~nxF"=="excel.exe" if /I not "%%~nxF"=="excel.pdb" if /I not "%%~nxF"=="config.json" if /I not "%%~xF"==".log" del /Q "%%~fF"
    )
)
for /D %%D in ("%OUTDIR%\*") do if /I not "%%~nxD"=="logs" if /I not "%%~nxD"=="OfficeCore" if exist "%%~fD" rmdir /S /Q "%%~fD"
if exist "excel.res" del /Q "excel.res"

if exist "%OUTDIR%\Excel-Strike.exe" del /Q "%OUTDIR%\Excel-Strike.exe"
if exist "%OUTDIR%\Excel-Strike.pdb" del /Q "%OUTDIR%\Excel-Strike.pdb"
if exist "%OUTDIR%\excel.ico" del /Q "%OUTDIR%\excel.ico"
echo build successful: %OUT%
goto :build_done

:build_failed
if exist "%OUTDIR%\excel.res" del /Q "%OUTDIR%\excel.res"
echo build failed.

:build_done
pause